/* useful debug utility */

#include<stdio.h>
#include<stdarg.h>

void
debug_msg(char *fmt, ...)
{
  va_list args;
  char msg[64];

  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg) - 1, fmt, args);
  va_end(args);
  vmsg(msg);
}

