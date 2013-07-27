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

static const char *list[] =
{
  "vim",
  "nano",
  "vile",
  "ne",
  "dex",
  "diakonos",
  "jed",
  "joe",
  "jmacs",
  "jpico",
  "jstar",
  0
};
static char **editors = 0;
static char **entries = 0;
static char *entry = 0;

static bool viconfig_setup_editors(void)
{
  const char *env = 0;
  size_t i = 0;
  const char *s = 0;
  const char *e = 0;
  char path[PATH_MAX] = {0};
  struct stat st = {0};
  size_t j = 0;

  if((env = getenv("PATH")) == 0)
  {
    error("PATH is not defined");
    return false;
  }

  env = strdupa(env);

  editors = alloc(char *,sizeof(list) / sizeof(*list));

  for( ; list[i] != 0 ; ++i )
  {
    for( s = e = env ; *e != 0 ; s = e )
    {
      if((e = strchr(e,':')) == 0)
        e = s + strlen(s);
      
      strfcpy(path,sizeof(path),"%.*s/%s",(int) (e-s),s,list[i]);
      
      if(stat(path,&st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 0755) == 0755)
      {
        editors[j++] = strdup(path);
        break;
      }
      
      if(*e == ':')
        ++e;
    }
  }

  editors[j] = 0;
  
  return true;
}

static bool viconfig_start(void)
{
  if(!viconfig_setup_editors())
    return false;

  return true;
}

static bool viconfig_finish(void)
{
  charpp_free(editors);
  
  editors = 0;

  return true;
}

struct tool viconfig_tool =
{
  viconfig_start,
  viconfig_finish,
  __FILE__
};
