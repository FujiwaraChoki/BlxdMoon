// Status functions (success, error, warning, info), including colors

#include <stdio.h>

void success(const char *msg)
{
  printf("\033[1;32m%s\033[0m", msg);
}

void error(const char *msg)
{
  printf("\033[1;31m%s\033[0m\n", msg);
}

void warning(const char *msg)
{
  printf("\033[1;33m%s\033[0m\n", msg);
}

void info(const char *msg)
{
  printf("\033[1;34m%s\033[0m\n", msg);
}
