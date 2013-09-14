// fwsetup - flexible installer for Frugalware
// Copyright (C) 2013 James Buren
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "local.h"

static inline bool infvbe(void)
{
  struct stat st = {0};
  
  return (stat("/run/initramfs/live/LiveOS/squashfs.img",&st) == 0 && S_ISREG(st.st_mode));
}

static void find_iso_device(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};
  char *device = 0;
  char *root = 0;

  if((file = fopen("/proc/mounts","rb")) == 0)
  {
    error(strerror(errno));
    return;
  }

  while(fgets(line,sizeof(line),file) != 0)
  {
    if((device = strtok(line,SPACE_CHARS)) == 0)
      continue;
    
    if((root = strtok(0,SPACE_CHARS)) == 0)
      continue;
    
    if(strcmp(root,ISO_ROOT) == 0)
    {
      g->isodevice = strdup(device);
      break;
    }
  }

  fclose(file);
}

static void global_cleanup(void)
{
  free(g->logpath);

  if(g->logfile)
    fclose(g->logfile);

  free(g->isodevice);

  charpp_free(g->fstabdata);

  free(g->groups);
  
  memset(g,0,sizeof(struct global));
}

extern int main(int argc,char **argv)
{
  char path[PATH_MAX] = {0};
  int code = 0;

  if(getuid() != 0)
  {
    printf("You must run this as root.\n");

    return EXIT_FAILURE;
  }

  g->seed = time(0);

  g->name = strrchr(*argv,'/');
  
  if(g->name == 0)
    g->name = *argv;
  else
    ++g->name;

  g->insetup = (strcmp(g->name,"fwsetup") == 0);

  g->infvbe = infvbe();

  if(g->insetup && !g->infvbe)
  {
    printf("You cannot run the installer outside of FVBE.\n");
    return EXIT_FAILURE;
  }

  strfcpy(path,sizeof(path),"/var/log/%s.log",g->name);

  g->logpath = strdup(path);

  remove(g->logpath);

  if((g->logfile = fopen(g->logpath,"a")) == 0)
  {
    perror(__func__);

    return EXIT_FAILURE;
  }

  setbuf(g->logfile,0);

  if(chmod(g->logpath,0600) == -1)
  {
    perror(__func__);
    
    return EXIT_FAILURE;
  }

  if(g->insetup)
  {
    g->hostroot = "/";
  
    g->guestroot = "/mnt/install";
    
    find_iso_device();
  }
  else
  {
    g->hostroot = "/";
  
    g->guestroot = "/";
  }

  if(setlocale(LC_ALL,"") == 0)
  {
    error(strerror(errno));
    return false;
  }
 
  code = ui_main(argc,argv);

  if(g->insetup)
    umount_all();

  global_cleanup();

  return code;
}

static struct global local = {0};

struct global *g = &local;

struct module *modules[] =
{
  &locale_module,
  &layout_module,
  &greeter_module,
  &information_module,
  &partition_module,
  &raid_module,
  &format_module,
  &preconfig_module,
  &install_module,
  &postconfig_module,
  &finale_module,
  0
};

struct tool *tools[] =
{
  &langconfig_tool,
  &kbconfig_tool,
  &nmconfig_tool,
  &rootconfig_tool,
  &userconfig_tool,
  &hostconfig_tool,
  &viconfig_tool,
  &dmconfig_tool,
  &timeconfig_tool,
  &grubconfig_tool,
  0
};

const size_t tools_count = sizeof(tools) / sizeof(*tools);
