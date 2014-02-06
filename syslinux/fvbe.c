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

typedef enum
{
  BOXCHAR_UL,
  BOXCHAR_UR,
  BOXCHAR_LL,
  BOXCHAR_LR,
  BOXCHAR_HLINE,
  BOXCHAR_VLINE
} boxchar;

typedef struct menu
{
  struct menu *prev;
  struct menu *next;
  char *text;
  bool selected;
} menu;

static int rows = 0;
static int columns = 0;

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

  printf(CSI "H");
}

static inline void show_cursor(void)
{
  printf(CSI "?25h");
}

static inline void hide_cursor(void)
{
  printf(CSI "?25l");
}

static inline void render_top(void)
{
  int i;

  printf(CSI "G");

  printf("%s",get_boxchar(BOXCHAR_UL));

  // Start at 2 because of reserved spaces on both screen edges.
  for( i = 2 ; i < columns ; ++i )
    printf("%s",get_boxchar(BOXCHAR_HLINE));

  printf("%s",get_boxchar(BOXCHAR_UR));

  printf(CSI "E");
}

static inline void render_line(const char *line,bool selected)
{
  int i;

  printf(CSI "G");

  printf("%s",get_boxchar(BOXCHAR_VLINE));

  if(selected)
    printf(CSI "7m");

  // Start at 2 because of reserved spaces on both screen edges.
  for( i = 2 ; i < columns ; ++i )
    printf("%c",(*line == '\n' || *line == 0) ? ' ' : *line++);

  if(selected)
    printf(CSI "27m");

  printf("%s",get_boxchar(BOXCHAR_VLINE));

  printf(CSI "E");
}

static inline void render_bottom(void)
{
  int i;

  printf(CSI "G");

  printf("%s",get_boxchar(BOXCHAR_LL));

  // Start at 2 because of reserved spaces on both screen edges.
  for( i = 2 ; i < columns ; ++i )
    printf("%s",get_boxchar(BOXCHAR_HLINE));

  printf("%s",get_boxchar(BOXCHAR_LR));

  printf(CSI "E");
}

static inline void gotoyx(int y,int x)
{
  printf(CSI "%d;%dH",y+1,x+1);
}

static inline menu *menu_first_item(menu *m)
{
  for( ; m->prev != 0 ; m = m->prev )
    ;

  return m;
}

static menu *menu_add_item(menu *m,const char *text)
{
  if(m == 0)
  {
    m = malloc(sizeof(menu));
    m->prev = 0;
    m->next = 0;
    m->text = strdup(text);
    m->selected = false;
  }
  else
  {
    m->next = malloc(sizeof(menu));
    m->next->prev = m;
    m->next->next = 0;
    m->next->text = strdup(text);
    m->next->selected = false;
    m = m->next;
  }

  return m;
}

static inline void menu_render(menu *m)
{
  int i;

  clear_screen();

  render_top();

  // Start at 2 because of reserved spaces on both screen edges.
  for( i = 2 ; i < rows ; ++i )
  {
    if(m == 0)
    {
      render_line("",false);
    }
    else
    {
      render_line(m->text,m->selected);

      m = m->next;
    }
  }

  render_bottom();
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

    // Sleep briefly before retrieving input
    msleep(100);

    // Retrieve input from stdin
    fgets(buf,sizeof(buf),stdin);

    // Parse input to get terminal dimensions
    if(sscanf(buf,CSI "%d;%dR",&rows,&columns) == 2)
      break;
  }

  return true;
#endif
}

extern int main(void)
{
  if(!open_terminal())
    return EXIT_FAILURE;

  return EXIT_FAILURE;
}
