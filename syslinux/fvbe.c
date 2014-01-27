#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/pci.h>
#include <errno.h>
#include <limits.h>
#include <syslinux/loadfile.h>
#include <syslinux/linux.h>
#include <core.h>

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define error(S) fprintf(stdout,"%s: %s\n",__func__,S)

typedef enum
{
  BOXCHAR_UL,
  BOXCHAR_UR,
  BOXCHAR_LL,
  BOXCHAR_LR,
  BOXCHAR_HLINE,
  BOXCHAR_VLINE
} boxchar;

static inline void write_serial_string(const char *s)
{
	while(*s != 0)
		write_serial(*s++);
}

static inline const char *get_boxchar(bool serial,boxchar type)
{
  static const unsigned char utf8[][4] =
  {
    [BOXCHAR_UL   ] = { 226, 148, 140, 0 },
    [BOXCHAR_UR   ] = { 226, 148, 144, 0 },
    [BOXCHAR_LL   ] = { 226, 148, 148, 0 },
    [BOXCHAR_LR   ] = { 226, 148, 152, 0 },
    [BOXCHAR_HLINE] = { 226, 148, 128, 0 },
    [BOXCHAR_VLINE] = { 226, 148, 130, 0 },
  };
  static const unsigned char cp437[][2] =
  {
    [BOXCHAR_UL   ] = { 218, 0 },
    [BOXCHAR_UR   ] = { 191, 0 },
    [BOXCHAR_LL   ] = { 192, 0 },
    [BOXCHAR_LR   ] = { 217, 0 },
    [BOXCHAR_HLINE] = { 196, 0 },
    [BOXCHAR_VLINE] = { 179, 0 },
  };

  return (const char *) ((serial) ? utf8[type] : cp437[type]);
}

extern int main(void)
{
  return EXIT_FAILURE;
}
