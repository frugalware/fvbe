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

static void global_cleanup(void)
{
  char **p = 0;

  free(g->logpath);

  if(g->logfile)
    fclose(g->logfile);

  if(g->fstabdata != 0)
  {
    for( p = g->fstabdata ; *p != 0 ; ++p )
      free(*p);
  
    free(g->fstabdata);
  }

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

  if(strcmp(g->name,"fwsetup") == 0)
  {
    g->insetup = true;
  
    g->hostroot = "/";
  
    g->guestroot = "/mnt/install";
  }
  else
  {
    g->insetup = false;
  
    g->hostroot = "/";
  
    g->guestroot = "/";
  }

  code = ui_main(argc,argv);

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
  &rootconfig_tool,
  &userconfig_tool,
  &hostconfig_tool,
  &modeconfig_tool,
  &timeconfig_tool,
  &grubconfig_tool,
  0
};

const size_t tools_count = sizeof(tools) / sizeof(*tools);
