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
#include <bios.h>

// Exports from core/serirq.c
extern void sirq_cleanup_nowipe(void);
extern void sirq_install(void);

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define SERIAL_OUTPUT

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

static inline const char *get_boxchar(boxchar type)
{
#if defined(SERIAL_OUTPUT)
  static const unsigned char utf8[][4] =
  {
    [BOXCHAR_UL   ] = { 226, 148, 140, 0 },
    [BOXCHAR_UR   ] = { 226, 148, 144, 0 },
    [BOXCHAR_LL   ] = { 226, 148, 148, 0 },
    [BOXCHAR_LR   ] = { 226, 148, 152, 0 },
    [BOXCHAR_HLINE] = { 226, 148, 128, 0 },
    [BOXCHAR_VLINE] = { 226, 148, 130, 0 },
  };

  return (const char *) utf8[type];
#elif defined(VGA_OUTPUT) || defined(VESA_OUTPUT)
  static const unsigned char cp437[][2] =
  {
    [BOXCHAR_UL   ] = { 218, 0 },
    [BOXCHAR_UR   ] = { 191, 0 },
    [BOXCHAR_LL   ] = { 192, 0 },
    [BOXCHAR_LR   ] = { 217, 0 },
    [BOXCHAR_HLINE] = { 196, 0 },
    [BOXCHAR_VLINE] = { 179, 0 },
  };

  return (const char *) cp437[type];
#endif
}

static bool open_terminal(void)
{
#if defined(SERIAL_OUTPUT)
  static const int BaudMax = 115200;

  // COM0 serial port
  SerialPort = get_serial_port(0);

  // 115200 baud rate
  BaudDivisor = (BaudMax / 115200) & 0xffff;

  // No flow control
  FlowOutput = 0;
  FlowInput = 0;
  FlowIgnore = 0;

  sirq_cleanup_nowipe();

  outb(0x83,SerialPort + 3);
  io_delay();

  outb((BaudDivisor & 0xff),SerialPort);
  io_delay();

  outb(((BaudDivisor & 0xff00) >> 8),SerialPort + 1);
  io_delay();

  outb(0x03,SerialPort + 3);
  io_delay();

  if(inb(SerialPort + 3) != 0x03)
  {
    SerialPort = 0;
    BaudDivisor = 0;
    FlowOutput = 0;
    FlowInput = 0;
    FlowIgnore = 0;
    return false;
  }

  outb(0x01,SerialPort + 2);
  io_delay();

  if(inb(SerialPort + 2) < 0x0C0)
  {
    outb(0,SerialPort + 2);
    io_delay();
  }

  outb(FlowOutput,SerialPort + 4);
  io_delay();

  if(FlowOutput & 0x8)
    sirq_install();

  return true;
#endif
}

extern int main(void)
{
  if(!open_terminal())
    return EXIT_FAILURE;

  return EXIT_FAILURE;
}
