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

  free(g->kbdlayout);
  
  free(g->xkblayout);
  
  free(g->xkbmodel);
  
  free(g->xkbvariant);
  
  free(g->xkboptions);
  
  if(g->fstabdata != 0)
  {
    for( p = g->fstabdata ; *p != 0 ; ++p )
      free(*p);
  
    free(g->fstabdata);
  }

  free(g->groups);
  
  memset(g,0,sizeof(struct global));
}

static void setup_logpath(const char *s)
{
  char buf[_POSIX_ARG_MAX] = {0};
  char path[PATH_MAX] = {0};
  
  strfcpy(buf,sizeof(buf),"%s",s);
  
  strfcpy(path,sizeof(path),"/var/log/%s.log",basename(buf));
  
  g->logpath = strdup(path);
}

extern int main(int argc,char **argv)
{
  int code = 0;

  if(getuid() != 0)
  {
    printf("You must run this as root.\n");

    return EXIT_FAILURE;
  }

  g->seed = time(0);

  setup_logpath(*argv);

  remove(g->logpath);

  if((g->logfile = fopen(g->logpath,"a")) == 0)
  {
    perror("main");

    return EXIT_FAILURE;
  }

  setbuf(g->logfile,0);

  if(strcmp(g->logpath,"/var/log/fwsetup.log") == 0)
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

struct module *utilities[] =
{
  0
};
