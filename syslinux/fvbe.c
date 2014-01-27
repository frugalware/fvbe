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

static inline void write_serial_string(const char *s)
{
	while(*s != 0)
		write_serial(*s++);
}

extern int main(void)
{
  return EXIT_FAILURE;
}
