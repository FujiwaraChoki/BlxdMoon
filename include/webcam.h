#ifndef WEBCAM_H
#define WEBCAM_H

#include <windows.h>

/**
 * Initialize webcam capture subsystem
 * Must be called before any other webcam functions
 * @return 0 on success, -1 on failure
 */
int webcam_init(void);

/**
 * Cleanup webcam capture subsystem
 */
void webcam_cleanup(void);

/**
 * Capture a single frame from the default webcam
 * @param output_path Path to save the captured image (BMP format)
 * @return 0 on success, -1 on failure
 */
int capture_webcam_frame(const char *output_path);

/**
 * List available webcam devices
 * @return Dynamically allocated string with device list (caller must free)
 */
char* list_webcam_devices(void);

#endif // WEBCAM_H
