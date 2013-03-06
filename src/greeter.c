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

static char *version = 0;

static bool greeter_setup(void)
{
  char line[LINE_MAX] = {0};

  file2str("/etc/frugalware-release",line,sizeof(line));

  if(strlen(line) == 0)
    return false;

  version = strdup(line);
  
  return true;
}

static bool greeter_run(void)
{
  if(!greeter_setup())
    return false;

  ui_window_text(version,GREETER_TEXT);

  return true;
}

static void greeter_reset(void)
{
  free(version);
  
  version = 0;
}

struct module greeter_module =
{
  greeter_run,
  greeter_reset,
  __FILE__
};
