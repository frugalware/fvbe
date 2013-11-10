#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/pci.h>
#include <errno.h>
#include <limits.h>
#include <cmenu.h>

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define RETURN_MAIN_MENU_TEXT "Return to Main Menu"
#define error(S) fprintf(stdout,"%s: %s\n",__func__,S)

static unsigned char columns = 0;
static unsigned char rows = 0;

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

static bool locale_menu_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};
  char *p = 0;

  if((file = fopen("locales","rb")) == 0)
  {
    error("failed to open locales file");
    return false;
  }

  add_named_menu("locale","Locale Selection",-1);

  add_item(RETURN_MAIN_MENU_TEXT,"",OPT_EXITMENU,"main",0);

  while(fgets(line,sizeof(line),file) != 0)
  {
    if((p = strchr(line,'\n')) != 0)
      *p = 0;

    add_item(line,"",OPT_EXITMENU,"main",0);
  }

  fclose(file);

  return true;
}

static bool layout_menu_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};
  char *p = 0;

  if((file = fopen("layouts","rb")) == 0)
  {
    error("failed to open layouts file");
    return false;
  }

  add_named_menu("layout","Keyboard Layout Selection",-1);

  add_item(RETURN_MAIN_MENU_TEXT,"",OPT_EXITMENU,"main",0);

  while(fgets(line,sizeof(line),file) != 0)
  {
    if((p = strchr(line,'\n')) != 0)
      *p = 0;

    add_item(line,"",OPT_EXITMENU,"main",0);
  }

  fclose(file);

  return true;
}

static bool main_menu_setup(void)
{
  init_menusystem("Frugalware Versatile Bootable Environment");

  add_named_menu("main","Main Menu",-1);

  add_item("Locales","",OPT_SUBMENU,"locale",0);

  add_item("Keyboard Layouts","",OPT_SUBMENU,"layout",0);

  if(!locale_menu_setup() || !layout_menu_setup())
  {
    close_menusystem();
    return false;
  }

  set_window_size(0,0,rows,columns);

  return true;
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

  return EXIT_SUCCESS;
}
