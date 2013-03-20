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

extern int main(int argc,char **argv)
{
  int code = 0;

  if(geteuid() != 0)
  {
    printf("You must run this as root.\n");

    return EXIT_FAILURE;
  }

  g->seed = time(0);

  remove(LOGFILE);

  if((g->logfile = fopen(LOGFILE,"a")) == 0)
  {
    perror("main");

    return EXIT_FAILURE;
  }

  setbuf(g->logfile,0);

  g->hostroot = "/";
  
  g->guestroot = INSTALL_ROOT;

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
