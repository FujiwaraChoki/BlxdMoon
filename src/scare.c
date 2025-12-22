#include <windows.h>
#include <stdio.h>
#include "scare.h"

// Global for thread proc
static char *g_ScareText = NULL;

DWORD WINAPI ScareThread(LPVOID lpParam)
{
    char *text = (char*)lpParam;

    // Register class
    const char CLASS_NAME[] = "ScareClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Create full screen window
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "System Error",
        WS_POPUP | WS_VISIBLE,
        0, 0, width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwnd == NULL) return 0;

    // Hide cursor
    ShowCursor(FALSE);

    // Block input
    BlockInput(TRUE);

    // Drawing loop
    HDC hdc = GetDC(hwnd);
    SetTextColor(hdc, RGB(0, 255, 0)); // Hacker green
    SetBkColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, OPAQUE);

    // Select big font
    HFONT hFont = CreateFont(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
    SelectObject(hdc, hFont);

    RECT rect;
    GetClientRect(hwnd, &rect);

    // Calculate center positioning roughly
    int len = strlen(text);
    int x = 100;
    int y = height / 2;

    char buffer[2] = {0};

    for (int i = 0; i < len; i++)
    {
        buffer[0] = text[i];
        TextOut(hdc, x, y, buffer, 1);

        // Move X
        SIZE size;
        GetTextExtentPoint32(hdc, buffer, 1, &size);
        x += size.cx;

        // Wrap if needed
        if (x > width - 100) {
            x = 100;
            y += size.cy;
        }

        Sleep(200); // 1 char per 200ms (faster than 1s/char which is too slow)
    }

    Sleep(3000); // Hold for 3 seconds

    // Cleanup
    BlockInput(FALSE);
    ShowCursor(TRUE);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    return 0;
}

int scare_typewriter(const char *text)
{
    // Clone text for thread
    char *text_copy = strdup(text);
    HANDLE hThread = CreateThread(NULL, 0, ScareThread, text_copy, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
        return 0;
    }
    free(text_copy);
    return -1;
}
