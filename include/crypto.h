#ifndef CRYPTO_H
#define CRYPTO_H

#include <windows.h>
#include <winsock2.h>

// Key sizes
#define CRYPTO_KEY_SIZE 32       // 256-bit AES key
#define CRYPTO_IV_SIZE 16        // 128-bit IV
#define CRYPTO_PUBKEY_SIZE 65    // ECDH P-256 public key (uncompressed)

/**
 * Initialize the crypto subsystem
 * @return 0 on success, -1 on failure
 */
int crypto_init(void);

/**
 * Cleanup the crypto subsystem
 */
void crypto_cleanup(void);

/**
 * Perform Diffie-Hellman key exchange (client side)
 * Generates keypair, sends public key, receives server's public key,
 * computes shared secret and derives AES key
 * @param sock Socket connected to server
 * @return 0 on success, -1 on failure
 */
int perform_key_exchange_client(SOCKET sock);

/**
 * Perform Diffie-Hellman key exchange (server side)
 * Receives client's public key, generates keypair, sends public key,
 * computes shared secret and derives AES key
 * @param sock Socket connected to client
 * @return 0 on success, -1 on failure
 */
int perform_key_exchange_server(SOCKET sock);

/**
 * Encrypt data using established session key
 * @param plaintext Data to encrypt
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer (must be at least plaintext_len + 32)
 * @return Length of ciphertext on success, -1 on failure
 */
int crypto_encrypt(const unsigned char *plaintext, int plaintext_len,
                   unsigned char *ciphertext);

/**
 * Decrypt data using established session key
 * @param ciphertext Data to decrypt
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @return Length of plaintext on success, -1 on failure
 */
int crypto_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                   unsigned char *plaintext);

/**
 * Send data securely (encrypt and send)
 * Prepends length header for framing
 * @param sock Socket to send on
 * @param data Data to send
 * @param len Length of data
 * @return Number of bytes sent on success, -1 on failure
 */
int secure_send(SOCKET sock, const char *data, int len);

/**
 * Receive data securely (receive and decrypt)
 * Handles length-prefixed framing
 * @param sock Socket to receive from
 * @param buffer Buffer to store decrypted data
 * @param max_len Maximum buffer size
 * @return Number of bytes received on success, -1 on failure
 */
int secure_recv(SOCKET sock, char *buffer, int max_len);

/**
 * Check if encryption is active
 * @return 1 if encryption is active, 0 otherwise
 */
int crypto_is_active(void);

#endif // CRYPTO_H
