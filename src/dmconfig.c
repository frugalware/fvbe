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

static char **managers = 0;
static char *manager = 0;

static bool dmconfig_setup_managers(void)
{
  static const char *list[] =
  {
    "none",
    "gdm",
    "kdm",
    "lightdm",
    "lxdm",
    "xdm",
    0
  };
  static const size_t list_count = (sizeof(list) / sizeof(*list)) - 1;
  size_t i = 0;
  const char *s = 0;
  char path[PATH_MAX] = {0};
  struct stat st = {0};
  size_t j = 0;

  managers = alloc(char *,list_count + 1);

  for( ; (s = list[i]) != 0 ; ++i )
  {
    if(strcmp(s,"none") == 0)
    {
      managers[j++] = strdup(s);
      continue;
    }

    strfcpy(path,sizeof(path),"lib/systemd/system/%s.service",s);
  
    if(stat(path,&st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 0644) == 0644)
      managers[j++] = strdup(s);
  }

  managers[j] = 0;

  return true;
}


static bool symlink_target(void)
{
  const char *old = (strcmp(manager,"none") == 0) ? "/lib/systemd/system/multi-user.target" : "/lib/systemd/system/graphical.target";
  const char *new = "etc/systemd/system/default.target";

  if(unlink(new) == -1 && errno != ENOENT)
  {
    error(strerror(errno));
    return false;
  }
  
  if(symlink(old,new) == -1)
  {
    error(strerror(errno));
    return false;
  }
  
  return true;
}

static bool update_via_old(void)
{
  char old[PATH_MAX] = {0};
  char *new = "etc/systemd/system/display-manager.service";
  
  if(!symlink_target())
    return false;
  
  if(strcmp(manager,"none") != 0)
    strfcpy(old,sizeof(old),"lib/systemd/system/%s.service",manager);

  if(unlink(new) == -1 && errno != ENOENT)
  {
    error(strerror(errno));
    return false;
  }
  
  if(strlen(old) > 0 && symlink(old,new) == -1)
  {
    error(strerror(errno));
    return false;
  }

  return true;
}

static bool update_via_new(void)
{
  char old[PATH_MAX] = {0};
  char new[PATH_MAX] = {0};
  char command[_POSIX_ARG_MAX] = {0};
  
  if(!symlink_target())
    return false;
  
  if(readlink0("etc/systemd/system/display-manager.service",old,sizeof(old)))
  {
    char *s = strrchr(old,'/');
    char *e = (s != 0) ? s + strlen(s) : 0;
    
    // +1 to include null terminator in the memmove().
    if(s != 0 && e != 0)
      memmove(old,s,e-s+1);
  }
  else
    *old = 0;

  if(strcmp(manager,"none") != 0)
    strfcpy(new,sizeof(new),"%s.service",manager);

  if(strlen(old) > 0)
  {
    strfcpy(command,sizeof(command),"systemctl disable '%s'",shell_escape(old));
    
    if(!execute(command,g->guestroot,0))
      return false;
  }

  if(strlen(new) > 0)
  {
    strfcpy(command,sizeof(command),"systemctl enable '%s'",shell_escape(new));
    
    if(!execute(command,g->guestroot,0))
      return false;
  }

  return true;
}

static bool dmconfig_action(void)
{
  if(g->insetup)
    return update_via_old();
  
  return update_via_new();
}

static bool dmconfig_start(void)
{
  if(!dmconfig_setup_managers())
    return false;

  if(managers[0] != 0 && managers[1] == 0)
    return true;

  if(!ui_window_list(DM_TITLE,DM_TEXT,managers,&manager))
    return false;

  return true;
}

static bool dmconfig_finish(void)
{
  bool success = true;

  if(manager != 0)
  {
    success = dmconfig_action();
  
    manager = 0;
  }

  if(managers != 0)
  {
    charpp_free(managers);
    
    managers = 0;
  }

  return success;
}

struct tool dmconfig_tool =
{
  dmconfig_start,
  dmconfig_finish,
  __FILE__
};
