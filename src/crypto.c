#include <windows.h>
#include <winsock2.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/crypto.h"

#pragma comment(lib, "bcrypt.lib")

// Session state
static BCRYPT_ALG_HANDLE hAesAlg = NULL;
static BCRYPT_KEY_HANDLE hAesKey = NULL;
static unsigned char session_key[CRYPTO_KEY_SIZE];
static unsigned char session_iv[CRYPTO_IV_SIZE];
static int crypto_active = 0;

static int generate_random(unsigned char *buffer, int len)
{
    BCRYPT_ALG_HANDLE hRngAlg;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hRngAlg, BCRYPT_RNG_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    status = BCryptGenRandom(hRngAlg, buffer, len, 0);
    BCryptCloseAlgorithmProvider(hRngAlg, 0);

    return BCRYPT_SUCCESS(status) ? 0 : -1;
}

static int sha256_hash(const unsigned char *input, int input_len, unsigned char *output)
{
    BCRYPT_ALG_HANDLE hHashAlg;
    BCRYPT_HASH_HANDLE hHash;
    NTSTATUS status;

    status = BCryptOpenAlgorithmProvider(&hHashAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    status = BCryptCreateHash(hHashAlg, &hHash, NULL, 0, NULL, 0, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hHashAlg, 0);
        return -1;
    }

    status = BCryptHashData(hHash, (PUCHAR)input, input_len, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hHashAlg, 0);
        return -1;
    }

    status = BCryptFinishHash(hHash, output, 32, 0);

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hHashAlg, 0);

    return BCRYPT_SUCCESS(status) ? 0 : -1;
}

int crypto_init(void)
{
    NTSTATUS status;

    // Open AES algorithm provider
    status = BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    // Set CBC mode
    status = BCryptSetProperty(hAesAlg, BCRYPT_CHAINING_MODE,
                               (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                               sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAesAlg, 0);
        hAesAlg = NULL;
        return -1;
    }

    return 0;
}

void crypto_cleanup(void)
{
    if (hAesKey)
    {
        BCryptDestroyKey(hAesKey);
        hAesKey = NULL;
    }

    if (hAesAlg)
    {
        BCryptCloseAlgorithmProvider(hAesAlg, 0);
        hAesAlg = NULL;
    }

    SecureZeroMemory(session_key, sizeof(session_key));
    SecureZeroMemory(session_iv, sizeof(session_iv));
    crypto_active = 0;
}

// Initialize AES key from shared secret
static int init_aes_key(const unsigned char *shared_secret, int secret_len)
{
    NTSTATUS status;

    // Derive key by hashing the shared secret
    unsigned char derived_key[32];
    if (sha256_hash(shared_secret, secret_len, derived_key) != 0)
    {
        return -1;
    }

    // Copy to session key
    memcpy(session_key, derived_key, CRYPTO_KEY_SIZE);

    // Generate random IV
    if (generate_random(session_iv, CRYPTO_IV_SIZE) != 0)
    {
        return -1;
    }

    // Destroy old key if exists
    if (hAesKey)
    {
        BCryptDestroyKey(hAesKey);
    }

    // Generate symmetric key
    status = BCryptGenerateSymmetricKey(hAesAlg, &hAesKey, NULL, 0,
                                         session_key, CRYPTO_KEY_SIZE, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    crypto_active = 1;
    SecureZeroMemory(derived_key, sizeof(derived_key));

    return 0;
}

int perform_key_exchange_client(SOCKET sock)
{
    BCRYPT_ALG_HANDLE hEcdhAlg = NULL;
    BCRYPT_KEY_HANDLE hMyKey = NULL;
    BCRYPT_SECRET_HANDLE hSecret = NULL;
    NTSTATUS status;
    int result = -1;

    // Initialize AES if not done
    if (hAesAlg == NULL && crypto_init() != 0)
    {
        return -1;
    }

    // Open ECDH algorithm provider
    status = BCryptOpenAlgorithmProvider(&hEcdhAlg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Generate ECDH key pair
    status = BCryptGenerateKeyPair(hEcdhAlg, &hMyKey, 256, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    status = BCryptFinalizeKeyPair(hMyKey, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Export public key
    DWORD pubKeySize = 0;
    status = BCryptExportKey(hMyKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &pubKeySize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    unsigned char *myPubKey = (unsigned char*)malloc(pubKeySize);
    status = BCryptExportKey(hMyKey, NULL, BCRYPT_ECCPUBLIC_BLOB, myPubKey, pubKeySize, &pubKeySize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        free(myPubKey);
        goto cleanup;
    }

    // Send public key to server
    int sent = send(sock, (char*)myPubKey, pubKeySize, 0);
    if (sent != pubKeySize)
    {
        free(myPubKey);
        goto cleanup;
    }
    free(myPubKey);

    // Receive server's public key
    unsigned char *serverPubKey = (unsigned char*)malloc(pubKeySize);
    int received = recv(sock, (char*)serverPubKey, pubKeySize, 0);
    if (received != pubKeySize)
    {
        free(serverPubKey);
        goto cleanup;
    }

    // Import server's public key
    BCRYPT_KEY_HANDLE hServerKey = NULL;
    status = BCryptImportKeyPair(hEcdhAlg, NULL, BCRYPT_ECCPUBLIC_BLOB,
                                  &hServerKey, serverPubKey, pubKeySize, 0);
    free(serverPubKey);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Derive shared secret
    status = BCryptSecretAgreement(hMyKey, hServerKey, &hSecret, 0);
    BCryptDestroyKey(hServerKey);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Derive key from secret
    DWORD secretSize = 0;
    status = BCryptDeriveKey(hSecret, BCRYPT_KDF_HASH, NULL, NULL, 0, &secretSize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    unsigned char *sharedSecret = (unsigned char*)malloc(secretSize);
    status = BCryptDeriveKey(hSecret, BCRYPT_KDF_HASH, NULL, sharedSecret, secretSize, &secretSize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        free(sharedSecret);
        goto cleanup;
    }

    // Initialize AES with derived key
    result = init_aes_key(sharedSecret, secretSize);
    SecureZeroMemory(sharedSecret, secretSize);
    free(sharedSecret);

cleanup:
    if (hSecret) BCryptDestroySecret(hSecret);
    if (hMyKey) BCryptDestroyKey(hMyKey);
    if (hEcdhAlg) BCryptCloseAlgorithmProvider(hEcdhAlg, 0);

    return result;
}

int perform_key_exchange_server(SOCKET sock)
{
    BCRYPT_ALG_HANDLE hEcdhAlg = NULL;
    BCRYPT_KEY_HANDLE hMyKey = NULL;
    BCRYPT_SECRET_HANDLE hSecret = NULL;
    NTSTATUS status;
    int result = -1;

    // Initialize AES if not done
    if (hAesAlg == NULL && crypto_init() != 0)
    {
        return -1;
    }

    // Open ECDH algorithm provider
    status = BCryptOpenAlgorithmProvider(&hEcdhAlg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Generate ECDH key pair
    status = BCryptGenerateKeyPair(hEcdhAlg, &hMyKey, 256, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    status = BCryptFinalizeKeyPair(hMyKey, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Export public key
    DWORD pubKeySize = 0;
    status = BCryptExportKey(hMyKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &pubKeySize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Receive client's public key first
    unsigned char *clientPubKey = (unsigned char*)malloc(pubKeySize);
    int received = recv(sock, (char*)clientPubKey, pubKeySize, 0);
    if (received != pubKeySize)
    {
        free(clientPubKey);
        goto cleanup;
    }

    // Send our public key
    unsigned char *myPubKey = (unsigned char*)malloc(pubKeySize);
    status = BCryptExportKey(hMyKey, NULL, BCRYPT_ECCPUBLIC_BLOB, myPubKey, pubKeySize, &pubKeySize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        free(clientPubKey);
        free(myPubKey);
        goto cleanup;
    }

    int sent = send(sock, (char*)myPubKey, pubKeySize, 0);
    free(myPubKey);
    if (sent != pubKeySize)
    {
        free(clientPubKey);
        goto cleanup;
    }

    // Import client's public key
    BCRYPT_KEY_HANDLE hClientKey = NULL;
    status = BCryptImportKeyPair(hEcdhAlg, NULL, BCRYPT_ECCPUBLIC_BLOB,
                                  &hClientKey, clientPubKey, pubKeySize, 0);
    free(clientPubKey);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Derive shared secret
    status = BCryptSecretAgreement(hMyKey, hClientKey, &hSecret, 0);
    BCryptDestroyKey(hClientKey);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Derive key from secret
    DWORD secretSize = 0;
    status = BCryptDeriveKey(hSecret, BCRYPT_KDF_HASH, NULL, NULL, 0, &secretSize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        goto cleanup;
    }

    unsigned char *sharedSecret = (unsigned char*)malloc(secretSize);
    status = BCryptDeriveKey(hSecret, BCRYPT_KDF_HASH, NULL, sharedSecret, secretSize, &secretSize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        free(sharedSecret);
        goto cleanup;
    }

    // Initialize AES with derived key
    result = init_aes_key(sharedSecret, secretSize);
    SecureZeroMemory(sharedSecret, secretSize);
    free(sharedSecret);

cleanup:
    if (hSecret) BCryptDestroySecret(hSecret);
    if (hMyKey) BCryptDestroyKey(hMyKey);
    if (hEcdhAlg) BCryptCloseAlgorithmProvider(hEcdhAlg, 0);

    return result;
}

int crypto_encrypt(const unsigned char *plaintext, int plaintext_len,
                   unsigned char *ciphertext)
{
    if (!crypto_active || hAesKey == NULL)
    {
        return -1;
    }

    // Calculate padded length (PKCS7 padding)
    int block_size = 16;
    int padded_len = ((plaintext_len / block_size) + 1) * block_size;

    // Create padded plaintext
    unsigned char *padded = (unsigned char*)malloc(padded_len);
    memcpy(padded, plaintext, plaintext_len);

    // Add PKCS7 padding
    int pad_value = padded_len - plaintext_len;
    memset(padded + plaintext_len, pad_value, pad_value);

    // Copy IV to output (first 16 bytes)
    memcpy(ciphertext, session_iv, CRYPTO_IV_SIZE);

    // Make a copy of IV for encryption (it gets modified)
    unsigned char iv_copy[CRYPTO_IV_SIZE];
    memcpy(iv_copy, session_iv, CRYPTO_IV_SIZE);

    // Encrypt
    ULONG result_len;
    NTSTATUS status = BCryptEncrypt(hAesKey, padded, padded_len, NULL,
                                     iv_copy, CRYPTO_IV_SIZE,
                                     ciphertext + CRYPTO_IV_SIZE, padded_len,
                                     &result_len, 0);

    free(padded);

    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    // Generate new IV for next message
    generate_random(session_iv, CRYPTO_IV_SIZE);

    return CRYPTO_IV_SIZE + result_len;
}

int crypto_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                   unsigned char *plaintext)
{
    if (!crypto_active || hAesKey == NULL)
    {
        return -1;
    }

    if (ciphertext_len < CRYPTO_IV_SIZE + 16)
    {
        return -1;  // Too short
    }

    // Extract IV from ciphertext
    unsigned char iv[CRYPTO_IV_SIZE];
    memcpy(iv, ciphertext, CRYPTO_IV_SIZE);

    int encrypted_len = ciphertext_len - CRYPTO_IV_SIZE;

    // Decrypt
    ULONG result_len;
    NTSTATUS status = BCryptDecrypt(hAesKey,
                                     (PUCHAR)(ciphertext + CRYPTO_IV_SIZE), encrypted_len,
                                     NULL, iv, CRYPTO_IV_SIZE,
                                     plaintext, encrypted_len,
                                     &result_len, 0);

    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    // Remove PKCS7 padding
    if (result_len > 0)
    {
        int pad_value = plaintext[result_len - 1];
        if (pad_value > 0 && pad_value <= 16)
        {
            result_len -= pad_value;
        }
    }

    return result_len;
}

int secure_send(SOCKET sock, const char *data, int len)
{
    if (!crypto_active)
    {
        // Fallback to unencrypted
        return send(sock, data, len, 0);
    }

    // Allocate buffer for encrypted data
    int max_encrypted_len = len + 32 + CRYPTO_IV_SIZE;  // Padding + IV
    unsigned char *encrypted = (unsigned char*)malloc(max_encrypted_len);

    int encrypted_len = crypto_encrypt((unsigned char*)data, len, encrypted);
    if (encrypted_len < 0)
    {
        free(encrypted);
        return -1;
    }

    // Send length header (4 bytes, network byte order)
    unsigned int net_len = htonl(encrypted_len);
    int sent = send(sock, (char*)&net_len, 4, 0);
    if (sent != 4)
    {
        free(encrypted);
        return -1;
    }

    // Send encrypted data
    sent = send(sock, (char*)encrypted, encrypted_len, 0);
    free(encrypted);

    return (sent == encrypted_len) ? len : -1;
}

int secure_recv(SOCKET sock, char *buffer, int max_len)
{
    if (!crypto_active)
    {
        // Fallback to unencrypted
        return recv(sock, buffer, max_len, 0);
    }

    // Receive length header
    unsigned int net_len;
    int received = recv(sock, (char*)&net_len, 4, 0);
    if (received != 4)
    {
        return -1;
    }

    int encrypted_len = ntohl(net_len);
    if (encrypted_len <= 0 || encrypted_len > max_len + 32 + CRYPTO_IV_SIZE)
    {
        return -1;  // Invalid length
    }

    // Receive encrypted data
    unsigned char *encrypted = (unsigned char*)malloc(encrypted_len);
    received = recv(sock, (char*)encrypted, encrypted_len, 0);
    if (received != encrypted_len)
    {
        free(encrypted);
        return -1;
    }

    // Decrypt
    int decrypted_len = crypto_decrypt(encrypted, encrypted_len, (unsigned char*)buffer);
    free(encrypted);

    if (decrypted_len < 0 || decrypted_len > max_len)
    {
        return -1;
    }

    buffer[decrypted_len] = '\0';
    return decrypted_len;
}

int crypto_is_active(void)
{
    return crypto_active;
}
