#ifndef BYPASS_H
#define BYPASS_H

// Attempt to bypass UAC and execute the current process with high privileges
// Returns 0 on success (process spawned), -1 on failure
int bypass_uac(void);

#endif
