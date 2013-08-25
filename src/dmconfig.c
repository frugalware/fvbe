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
    strfcpy(path,sizeof(path),"lib/systemd/system/%s.service",s);
  
    if(stat(path,&st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 0644) == 0644)
      managers[j++] = strdup(s);
  }

  managers[j] = 0;

  return true;
}


static bool dmconfig_symlink_target(void)
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

static bool dmconfig_start(void)
{
  if(!dmconfig_setup_managers())
    return false;

  if(!ui_window_list(MODE_TITLE,MODE_TEXT,managers,&manager))
    return false;

  return true;
}

static bool dmconfig_finish(void)
{
  bool success = true;

  if(manager != 0)
  {
    success = dmconfig_action(manager);
  
    manager = 0;
  }

  return success;
}

struct tool dmconfig_tool =
{
  dmconfig_start,
  dmconfig_finish,
  __FILE__
};
