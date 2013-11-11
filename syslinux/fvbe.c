#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/pci.h>
#include <errno.h>
#include <limits.h>
#include <cmenu.h>
#include <syslinux/loadfile.h>
#include <syslinux/linux.h>

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define MAX_MAIN     10
#define MAX_FONTS    50
#define MAX_LOCALES 300
#define MAX_LAYOUTS 100
#define error(S) fprintf(stdout,"%s: %s\n",__func__,S)

static unsigned char columns = 0;
static unsigned char rows = 0;
static char run[32]    = "";
static char font[32]   = "ter-v16b";
static char locale[32] = "en_US.utf8";
static char layout[32] = "us";

static bool text_output_setup(void)
{
  int w = 0;
  int h = 0;

  if(getscreensize(STDOUT_FILENO,&w,&h) == -1)
  {
    error(strerror(errno));
    return false;
  }

  columns = w;

  rows = h;

  return true;
}

#if 0
static bool pci_bus_probe(void)
{
  struct pci_device *device = 0;
  struct pci_domain *domain = 0;
  uint16_t vendor = 0;
  uint16_t product = 0;
  uint16_t sub_vendor = 0;
  uint16_t sub_product = 0;
  uint8_t revision = 0;
  uint32_t class = 0;

  if((domain = pci_scan()) == 0)
  {
    error("failed to probe pci bus");
    return false;
  }

  for_each_pci_func(device,domain)
  {
    vendor = device->vendor;

    product = device->product;

    sub_vendor = device->sub_vendor;

    sub_product = device->sub_product;

    revision = device->revision;

    class = (device->class[2] << 16) | (device->class[1] << 8) | (device->class[0] << 0);
  }

  free_pci_domain(domain);

  return true;
}
#endif

static t_handler_return font_menu_handler(t_menusystem *ms,t_menuitem *mi)
{
  t_handler_return rv = { .valid = 1, .refresh = 0 };

  strlcpy(font,mi->item,sizeof(font));

  return rv;
}

static bool font_menu_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};

  if((file = fopen("fonts","rb")) == 0)
  {
    error("failed to open fonts file");
    return false;
  }

  add_named_menu("font","Font Selection",MAX_FONTS);

  while(fgets(line,sizeof(line),file) != 0)
  {
    char *s = 0;
    t_menuitem *p = 0;

    if((s = strchr(line,'\n')) != 0)
      *s = 0;

    p = add_item(line,"",OPT_EXITMENU,"main",0);

    p->handler = font_menu_handler;
  }

  fclose(file);

  return true;
}

static t_handler_return locale_menu_handler(t_menusystem *ms,t_menuitem *mi)
{
  t_handler_return rv = { .valid = 1, .refresh = 0 };

  strlcpy(locale,mi->item,sizeof(locale));

  return rv;
}

static bool locale_menu_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};

  if((file = fopen("locales","rb")) == 0)
  {
    error("failed to open locales file");
    return false;
  }

  add_named_menu("locale","Locale Selection",MAX_LOCALES);

  while(fgets(line,sizeof(line),file) != 0)
  {
    char *s = 0;
    t_menuitem *p = 0;

    if((s = strchr(line,'\n')) != 0)
      *s = 0;

    p = add_item(line,"",OPT_EXITMENU,"main",0);

    p->handler = locale_menu_handler;
  }

  fclose(file);

  return true;
}

static t_handler_return layout_menu_handler(t_menusystem *ms,t_menuitem *mi)
{
  t_handler_return rv = { .valid = 1, .refresh = 0 };

  strlcpy(layout,mi->item,sizeof(layout));

  return rv;
}

static bool layout_menu_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};

  if((file = fopen("layouts","rb")) == 0)
  {
    error("failed to open layouts file");
    return false;
  }

  add_named_menu("layout","Keyboard Layout Selection",MAX_LAYOUTS);

  while(fgets(line,sizeof(line),file) != 0)
  {
    char *s = 0;
    t_menuitem *p = 0;

    if((s = strchr(line,'\n')) != 0)
      *s = 0;

    p = add_item(line,"",OPT_EXITMENU,"main",0);

    p->handler = layout_menu_handler;
  }

  fclose(file);

  return true;
}

static bool main_menu_setup(void)
{
  init_menusystem("Frugalware Versatile Bootable Environment");

  add_named_menu("main","Main Menu",MAX_MAIN);

  add_item("Boot FVBE","",OPT_RUN,"fvbe",0);

  add_item("Fonts","",OPT_SUBMENU,"font",0);

  add_item("Locales","",OPT_SUBMENU,"locale",0);

  add_item("Keyboard Layouts","",OPT_SUBMENU,"layout",0);

  if(!font_menu_setup() || !locale_menu_setup() || !layout_menu_setup())
  {
    close_menusystem();
    return false;
  }

  set_window_size(0,0,columns,rows);

  return true;
}

// No return value needed. If control returns to caller, it means this function has failed.
static void boot_fvbe(void)
{
  const char kernel[] = "vmlinuz";
  const char initrd[] = "initrd";
  void *kernel_data = 0;
  size_t kernel_size = 0;
  struct initramfs *initramfs = 0;

  printf("Loading %s... ",kernel);

  if(loadfile(kernel,&kernel_data,&kernel_size) != 0)
  {
    printf("fail\n");
    return;
  }

  printf("ok\n");

  printf("Loading %s... ",initrd);

  if((initramfs = initramfs_init()) == 0)
  {
    printf("fail\n");
    return;
  }

  if(initramfs_load_archive(initramfs,initrd) != 0)
  {
    printf("fail\n");
    return;
  }

  printf("ok\n");

  syslinux_boot_linux(kernel_data,kernel_size,initramfs,0,0);

  printf("failed to boot fvbe\n");
}

extern int main(void)
{
  if(!text_output_setup())
    return EXIT_FAILURE;

#if 0
  if(!pci_bus_probe())
    return EXIT_FAILURE;
#endif

  if(!main_menu_setup())
    return EXIT_FAILURE;

  while(true)
  {
    t_menuitem *p = showmenus(0);

    if(p != 0 && p->data != 0 && p->action == OPT_RUN)
    {
      if(strcmp(p->data,"fvbe") == 0)
        strlcpy(run,p->data,sizeof(run));

      if(strlen(run) != 0)
        break;
    }
  }

  return EXIT_SUCCESS;
}
