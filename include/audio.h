#ifndef AUDIO_H
#define AUDIO_H

// Record audio for a specified duration in seconds and save to output_filename
// Returns 0 on success, -1 on failure
int record_audio(int duration_seconds, const char* output_filename);

#endif
