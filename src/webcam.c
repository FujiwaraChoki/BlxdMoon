#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dshow.h>
#include "../include/webcam.h"
#include "../include/uuid.h"

// We'll use a simpler approach with escapi or direct GDI capture
// For full webcam support, Media Foundation requires C++ COM interfaces

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")

#define WEBCAM_BUFFER_SIZE 4096

static int com_initialized = 0;

int webcam_init(void)
{
    if (!com_initialized)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (SUCCEEDED(hr) || hr == S_FALSE)
        {
            com_initialized = 1;
            return 0;
        }
        return -1;
    }
    return 0;
}

void webcam_cleanup(void)
{
    if (com_initialized)
    {
        CoUninitialize();
        com_initialized = 0;
    }
}

char* list_webcam_devices(void)
{
    char *result = (char*)malloc(WEBCAM_BUFFER_SIZE);
    if (result == NULL)
    {
        return NULL;
    }
    memset(result, 0, WEBCAM_BUFFER_SIZE);

    strcpy(result, "=== WEBCAM DEVICES ===\n\n");

    if (webcam_init() != 0)
    {
        strcat(result, "[-] Failed to initialize COM\n");
        return result;
    }

    // Create device enumerator
    ICreateDevEnum *pDevEnum = NULL;
    HRESULT hr = CoCreateInstance(
        &CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        &IID_ICreateDevEnum, (void**)&pDevEnum);

    if (FAILED(hr))
    {
        strcat(result, "[-] Failed to create device enumerator\n");
        return result;
    }

    // Create enumerator for video capture devices
    IEnumMoniker *pEnum = NULL;
    hr = pDevEnum->lpVtbl->CreateClassEnumerator(
        pDevEnum, &CLSID_VideoInputDeviceCategory, &pEnum, 0);

    if (hr == S_FALSE || pEnum == NULL)
    {
        strcat(result, "[*] No webcam devices found\n");
        pDevEnum->lpVtbl->Release(pDevEnum);
        return result;
    }

    // Enumerate devices
    IMoniker *pMoniker = NULL;
    int device_count = 0;
    char line[256];

    while (pEnum->lpVtbl->Next(pEnum, 1, &pMoniker, NULL) == S_OK)
    {
        IPropertyBag *pPropBag = NULL;
        hr = pMoniker->lpVtbl->BindToStorage(
            pMoniker, NULL, NULL, &IID_IPropertyBag, (void**)&pPropBag);

        if (SUCCEEDED(hr))
        {
            VARIANT var;
            VariantInit(&var);

            // Get friendly name
            hr = pPropBag->lpVtbl->Read(pPropBag, L"FriendlyName", &var, NULL);
            if (SUCCEEDED(hr))
            {
                // Convert wide string to char
                char name[256];
                WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1,
                                    name, sizeof(name), NULL, NULL);

                sprintf(line, "[%d] %s\n", device_count, name);
                strcat(result, line);
                device_count++;

                VariantClear(&var);
            }

            pPropBag->lpVtbl->Release(pPropBag);
        }

        pMoniker->lpVtbl->Release(pMoniker);
    }

    if (device_count == 0)
    {
        strcat(result, "[*] No webcam devices found\n");
    }
    else
    {
        sprintf(line, "\n[+] Total: %d device(s) found\n", device_count);
        strcat(result, line);
    }

    pEnum->lpVtbl->Release(pEnum);
    pDevEnum->lpVtbl->Release(pDevEnum);

    return result;
}

// Simplified capture using sample grabber
// Note: Full implementation requires complex DirectShow filter graph
int capture_webcam_frame(const char *output_path)
{
    if (webcam_init() != 0)
    {
        return -1;
    }

    // Create filter graph manager
    IGraphBuilder *pGraph = NULL;
    HRESULT hr = CoCreateInstance(
        &CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
        &IID_IGraphBuilder, (void**)&pGraph);

    if (FAILED(hr))
    {
        return -1;
    }

    // Create capture graph builder
    ICaptureGraphBuilder2 *pBuilder = NULL;
    hr = CoCreateInstance(
        &CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
        &IID_ICaptureGraphBuilder2, (void**)&pBuilder);

    if (FAILED(hr))
    {
        pGraph->lpVtbl->Release(pGraph);
        return -1;
    }

    pBuilder->lpVtbl->SetFiltergraph(pBuilder, pGraph);

    // Get video capture device
    ICreateDevEnum *pDevEnum = NULL;
    hr = CoCreateInstance(
        &CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        &IID_ICreateDevEnum, (void**)&pDevEnum);

    if (FAILED(hr))
    {
        pBuilder->lpVtbl->Release(pBuilder);
        pGraph->lpVtbl->Release(pGraph);
        return -1;
    }

    IEnumMoniker *pEnum = NULL;
    hr = pDevEnum->lpVtbl->CreateClassEnumerator(
        pDevEnum, &CLSID_VideoInputDeviceCategory, &pEnum, 0);

    if (hr == S_FALSE || pEnum == NULL)
    {
        pDevEnum->lpVtbl->Release(pDevEnum);
        pBuilder->lpVtbl->Release(pBuilder);
        pGraph->lpVtbl->Release(pGraph);
        return -1;  // No devices
    }

    // Get first device
    IMoniker *pMoniker = NULL;
    if (pEnum->lpVtbl->Next(pEnum, 1, &pMoniker, NULL) != S_OK)
    {
        pEnum->lpVtbl->Release(pEnum);
        pDevEnum->lpVtbl->Release(pDevEnum);
        pBuilder->lpVtbl->Release(pBuilder);
        pGraph->lpVtbl->Release(pGraph);
        return -1;
    }

    // Bind to base filter
    IBaseFilter *pCap = NULL;
    hr = pMoniker->lpVtbl->BindToObject(
        pMoniker, NULL, NULL, &IID_IBaseFilter, (void**)&pCap);

    pMoniker->lpVtbl->Release(pMoniker);
    pEnum->lpVtbl->Release(pEnum);
    pDevEnum->lpVtbl->Release(pDevEnum);

    if (FAILED(hr))
    {
        pBuilder->lpVtbl->Release(pBuilder);
        pGraph->lpVtbl->Release(pGraph);
        return -1;
    }

    // Add capture filter to graph
    hr = pGraph->lpVtbl->AddFilter(pGraph, pCap, L"Capture");
    if (FAILED(hr))
    {
        pCap->lpVtbl->Release(pCap);
        pBuilder->lpVtbl->Release(pBuilder);
        pGraph->lpVtbl->Release(pGraph);
        return -1;
    }

    // For a simple single-frame capture, we would need to:
    // 1. Add a sample grabber filter
    // 2. Configure it to grab one sample
    // 3. Render the preview
    // 4. Run the graph briefly
    // 5. Get the sample and save as BMP

    // This is complex DirectShow code - for now, return a message
    // In production, you'd implement the full filter graph

    // Cleanup
    pCap->lpVtbl->Release(pCap);
    pBuilder->lpVtbl->Release(pBuilder);
    pGraph->lpVtbl->Release(pGraph);

    // Alternative: Use a PowerShell script to capture
    // This is a simpler workaround
    char cmd[512];
    sprintf(cmd,
        "powershell -Command \""
        "Add-Type -AssemblyName System.Windows.Forms; "
        "[System.Windows.Forms.Screen]::PrimaryScreen | Out-Null; "
        "$webcam = New-Object -ComObject WIA.CommonDialog; "
        "# Webcam capture requires WIA or external tool"
        "\"");

    // For actual implementation, consider using:
    // - OpenCV (if available)
    // - ffmpeg command line
    // - ESCAPI library (simple webcam capture for C)

    return -1;  // Not fully implemented - requires complex DirectShow setup
}

// Alternative: Simple capture using command-line tool
int capture_webcam_simple(const char *output_path)
{
    // Check if ffmpeg is available
    char cmd[1024];

    // Try using ffmpeg to capture a single frame
    sprintf(cmd,
        "ffmpeg -f dshow -i video=\"Integrated Camera\" "
        "-frames:v 1 -y \"%s\" 2>nul",
        output_path);

    int result = system(cmd);

    if (result != 0)
    {
        // Try with generic device name
        sprintf(cmd,
            "ffmpeg -f dshow -list_devices true -i dummy 2>&1 | "
            "findstr /i \"video\" > nul && "
            "ffmpeg -f dshow -i video=0 -frames:v 1 -y \"%s\" 2>nul",
            output_path);

        result = system(cmd);
    }

    return (result == 0) ? 0 : -1;
}
