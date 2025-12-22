#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include "troll.h"

#pragma comment(lib, "winmm.lib")

// Rotate screen (0, 90, 180, 270 degrees)
int troll_rotate_screen(void)
{
    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
    {
        // Toggle between landscape and portrait, flipping orientation
        DWORD temp = dm.dmPelsHeight;
        dm.dmPelsHeight = dm.dmPelsWidth;
        dm.dmPelsWidth = temp;

        dm.dmDisplayOrientation = (dm.dmDisplayOrientation + 1) % 4;

        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYORIENTATION;

        ChangeDisplaySettings(&dm, CDS_UPDATEREGISTRY);
        return 0;
    }
    return -1;
}

// Open CD Tray
int troll_open_cd_tray(void)
{
    return mciSendString("set cdaudio door open", NULL, 0, NULL);
}

// Speak text using SAPI (via PowerShell one-liner for simplicity/compatibility)
int troll_speak(const char *text)
{
    char cmd[1024];
    // Escaping quotes is tricky, keeping simple
    sprintf(cmd, "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('%s');\"", text);

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    }
    return -1;
}

// Play annoying random beeps
int troll_random_beep(void)
{
    int freq = 500 + (rand() % 2000); // 500-2500 Hz
    int duration = 100 + (rand() % 900); // 100-1000 ms
    Beep(freq, duration);
    return 0;
}
