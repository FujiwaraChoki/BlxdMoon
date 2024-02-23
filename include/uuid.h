#ifndef UUID_GENERATOR_H
#define UUID_GENERATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Generates a UUID-like string.
char *generate_uuid()
{
  char *uuid_str = (char *)malloc(37); // Allocate memory for the UUID string

  // Get current timestamp
  time_t current_time = time(NULL);

  // Convert timestamp to string
  snprintf(uuid_str, 37, "%08lx-", (unsigned long)current_time);

  // Generate a random number
  unsigned random_num = rand();

  // Append random number to the string
  snprintf(uuid_str + 9, 9, "%04x-", random_num);

  // Add some machine-specific information
  snprintf(uuid_str + 14, 5, "%04x-", 42);

  // Append another random number
  snprintf(uuid_str + 19, 5, "%04x-", rand());

  // Generate the last random number
  snprintf(uuid_str + 24, 13, "%012llx", (unsigned long long)rand() * rand());

  // Ensure null-termination
  uuid_str[36] = '\0';

  return uuid_str;
}

#endif
