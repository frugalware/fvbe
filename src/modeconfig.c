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

static char *modes[] =
{
  "Text Console",
  "Display Manager",
  0
};
static char *mode = 0;

static bool modeconfig_action(const char *mode)
{
  const char *old = 0;
  const char *new = "etc/systemd/system/default.target";

  if(strcmp(mode,"Text Console") == 0)
    old = "/lib/systemd/system/multi-user.target";
  else if(strcmp(mode,"Display Manager") == 0)
    old = "/lib/systemd/system/graphical.target";
  else
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

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


static bool modeconfig_start(void)
{
  if(!ui_window_list(MODE_TITLE,MODE_TEXT,modes,&mode))
    return false;

  return true;
}

static bool modeconfig_finish(void)
{
  bool success = true;

  if(mode)
  {
    success = modeconfig_action(mode);
  
    mode = 0;
  }

  return success;
}

struct tool modeconfig_tool =
{
  modeconfig_start,
  modeconfig_finish,
  __FILE__
};
