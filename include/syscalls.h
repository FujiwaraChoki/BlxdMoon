#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <windows.h>

// Resolve System Service Number (SSN) for a function name
DWORD GetSSN(const char* funcName);

// Find 'syscall; ret' gadget address
PVOID GetSyscallGadget();

#endif
