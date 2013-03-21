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

static char *hostname = 0;
static char *prettyhostname = 0;

static bool update_via_old(const char *hostname,const char *prettyhostname)
{
  FILE *file = 0;
  
  if((file = fopen("etc/hostname","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,"%s\n",hostname);
  
  fclose(file);
  
  if((file = fopen("etc/machine-info","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,"PRETTY_HOSTNAME='%s'\n",prettyhostname);
  
  fclose(file);
  
  return true;
}

static bool update_via_new(const char *hostname,const char *prettyhostname)
{  
  char command[_POSIX_ARG_MAX] = {0};
  
  strfcpy(command,sizeof(command),"hostnamectl --static set-hostname '%s'",hostname);

  if(!execute(command,g->guestroot,0))
    return false;
  
  strfcpy(command,sizeof(command),"hostnamectl --pretty set-hostname '%s'",prettyhostname);

  if(!execute(command,g->guestroot,0))
    return false;
  
  return true;  
}

static bool hostconfig_action(const char *hostname,const char *prettyhostname)
{
  if(g->insetup)
    return update_via_old(hostname,prettyhostname);
  
  return update_via_new(hostname,prettyhostname);
}

static bool hostconfig_start(void)
{
  if(!ui_window_host(&hostname,&prettyhostname))
    return false;

  return true;
}

static bool hostconfig_finish(void)
{
  bool success = true;
  
  if(hostname && prettyhostname)
    success = hostconfig_action(hostname,prettyhostname);

  free(hostname);
  
  free(prettyhostname);

  hostname = 0;
  
  prettyhostname = 0;

  return success;
}

struct tool hostconfig_tool =
{
  hostconfig_start,
  hostconfig_finish,
  __FILE__
};
