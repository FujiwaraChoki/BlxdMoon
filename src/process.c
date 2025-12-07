#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/process.h"

#define PROCESS_LIST_BUFFER_SIZE 65536
#define PROCESS_INFO_BUFFER_SIZE 1024

char* list_processes(void)
{
    char *result = (char*)malloc(PROCESS_LIST_BUFFER_SIZE);
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, PROCESS_LIST_BUFFER_SIZE);

    // Create snapshot of all processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        sprintf(result, "[-] Failed to create process snapshot (Error: %lu)\n", GetLastError());
        return result;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Header
    sprintf(result, "%-8s %-40s %-8s %-8s\n", "PID", "NAME", "THREADS", "PPID");
    strcat(result, "-------- ---------------------------------------- -------- --------\n");

    // Get first process
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        strcat(result, "[-] Failed to get first process\n");
        return result;
    }

    // Iterate through all processes
    do {
        char line[256];
        sprintf(line, "%-8lu %-40s %-8lu %-8lu\n",
                pe32.th32ProcessID,
                pe32.szExeFile,
                pe32.cntThreads,
                pe32.th32ParentProcessID);

        // Check buffer overflow
        if (strlen(result) + strlen(line) < PROCESS_LIST_BUFFER_SIZE - 1) {
            strcat(result, line);
        } else {
            strcat(result, "\n[!] Output truncated - too many processes\n");
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return result;
}

int kill_process(DWORD pid)
{
    // Don't allow killing system processes
    if (pid == 0 || pid == 4) {
        return -1;
    }

    // Open process with terminate permission
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        return -1;
    }

    // Terminate the process
    BOOL success = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);

    return success ? 0 : -1;
}

char* get_process_info(DWORD pid)
{
    char *result = (char*)malloc(PROCESS_INFO_BUFFER_SIZE);
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, PROCESS_INFO_BUFFER_SIZE);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        sprintf(result, "[-] Failed to create process snapshot\n");
        return result;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    BOOL found = FALSE;

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == pid) {
                found = TRUE;
                sprintf(result,
                        "Process Information:\n"
                        "  PID:         %lu\n"
                        "  Name:        %s\n"
                        "  Threads:     %lu\n"
                        "  Parent PID:  %lu\n"
                        "  Priority:    %ld\n",
                        pe32.th32ProcessID,
                        pe32.szExeFile,
                        pe32.cntThreads,
                        pe32.th32ParentProcessID,
                        pe32.pcPriClassBase);
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    if (!found) {
        sprintf(result, "[-] Process with PID %lu not found\n", pid);
    }

    CloseHandle(hSnapshot);
    return result;
}
