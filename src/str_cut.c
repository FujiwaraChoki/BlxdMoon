#include "str_cut.h"

char *str_cut(char str[], int slice_from, int slice_to)
{
  // Check if string is empty or invalid
  if (str[0] == '\0')
    return NULL;

  // Define variables
  char *buffer;
  size_t str_len, buffer_len;

  // Slice from the end
  if (slice_to < 0 && slice_from > slice_to)
  {
    str_len = strlen(str);

    // Check if slice_from is out of bounds
    if (abs(slice_to) > str_len - 1)
      return NULL;

    // Check if slice_to is out of bounds
    if (abs(slice_from) > str_len)
      slice_from = (-1) * str_len;

    // Calculate buffer length
    buffer_len = slice_to - slice_from;
    str += (str_len + slice_from);
  }
  else if (slice_from >= 0 && slice_to > slice_from)
  {
    // Slice from the beginning
    str_len = strlen(str);

    if (slice_from > str_len - 1)
      return NULL;
    buffer_len = slice_to - slice_from;
    str += slice_from;
  }
  else
    return NULL;

  // Allocate memory for buffer
  buffer = calloc(buffer_len, sizeof(char));
  strncpy(buffer, str, buffer_len);

  // Return buffer
  return buffer;
}