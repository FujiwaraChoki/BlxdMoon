#ifndef PROCESS_H
#define PROCESS_H

#include <windows.h>

/**
 * List all running processes on the system
 * @return Dynamically allocated string with process list (caller must free)
 *         Format: "PID\tName\tThreads\tParentPID\n" for each process
 */
char* list_processes(void);

/**
 * Terminate a process by its PID
 * @param pid Process ID to terminate
 * @return 0 on success, -1 on failure
 */
int kill_process(DWORD pid);

/**
 * Get information about a specific process
 * @param pid Process ID to query
 * @return Dynamically allocated string with process info (caller must free)
 */
char* get_process_info(DWORD pid);

#endif // PROCESS_H
