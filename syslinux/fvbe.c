#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pci.h>
#include <errno.h>
#include <limits.h>
#include <syslinux/loadfile.h>
#include <syslinux/linux.h>
#include <syslinux/config.h>
#include <cpu.h>
#include <console.h>
#include <core.h>
#include <bios.h>
#include <getkey.h>

// Exports from core/serirq.c
extern void sirq_cleanup_nowipe(void);
extern void sirq_install(void);

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define CSI "\e["

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

static int rows = 0;
static int columns = 0;

static inline void get_input(char *buf,size_t chars)
{
  size_t i;
  int c;

  for( i = 0 ; (c = fgetc(stdin)) != EOF && i < chars ; ++i )
    buf[i] = c;

  buf[i] = 0;
}

// Borrowed code from com32/menu/drain.c
static inline void drain_keyboard(void)
{
  volatile char junk;
  int rv;

  do
  {
    rv = read(0, (char *)&junk, 1);
  }
  while (rv > 0);

  junk = 0;

  cli();
  *(volatile uint8_t *)0x419 = 0;
  *(volatile uint16_t *)0x41a = 0x1e;
  *(volatile uint16_t *)0x41c = 0x1e;
  memset((void *)0x41e, 0, 32);
  sti();
}


static inline void clear_screen(void)
{
  printf(CSI "2J");
}

static inline void gotoyx(int y,int x)
{
  printf(CSI "%d;%dH",y+1,x+1);
}

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

  // Start of borrowed code from com32/elflink/ldlinux/readconfig.c
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

  // End of borrowed code from com32/elflink/ldlinux/readconfig.c

  __syslinux_set_serial_console_info();

  if(openconsole(&dev_rawcon_r,&dev_serial_w))
    return false;

  while(true)
  {
    char buf[11];

    // Force cursor to bottom-right corner
    printf(CSI "999;999H");

    // Request cursor position
    printf(CSI "6n");
 
    // Retrieve input from stdin
    get_input(buf,10);
  
    // Parse input to get terminal dimensions
    if(sscanf(buf,CSI "%d;%dR",&rows,&columns) == 2)
      break;
    
    // Sleep and try again.
    msleep(250);
  }

  return true;
#endif
}

#if 0
static void test(void)
{
  int y;
  int x;

  clear_screen();
  
  for( y = 0 ; y < rows ; ++y )
  {
    gotoyx(y,0);
    for( x = 0 ; x < columns ; ++x )
    {
      const char *bc;
      
      if(y == 0)
      {
        if(x == 0)
          bc = get_boxchar(BOXCHAR_UL);
        else if(x+1 == columns)
          bc = get_boxchar(BOXCHAR_UR);
        else
          bc = get_boxchar(BOXCHAR_HLINE);
      }
      else if(y+1 == rows)
      {
        if(x == 0)
          bc = get_boxchar(BOXCHAR_LL);
        else if(x+1 == columns)
          bc = get_boxchar(BOXCHAR_LR);
        else
          bc = get_boxchar(BOXCHAR_HLINE);       
      }
      else if(x == 0 || x+1 == columns)
        bc = get_boxchar(BOXCHAR_VLINE);
      else
        bc = " ";
    
      printf("%s",bc);
    }
  }
}
#endif

extern int main(void)
{
  if(!open_terminal())
    return EXIT_FAILURE;

  return EXIT_FAILURE;
}
