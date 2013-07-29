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

static char **editors = 0;
static char *editor = 0;

static bool viconfig_setup_editors(void)
{
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
  static const size_t list_count = (sizeof(list) / sizeof(*list)) - 1;
  size_t i = 0;
  const char *s = 0;
  char path[PATH_MAX] = {0};
  struct stat st = {0};
  size_t j = 0;

  editors = alloc(char *,list_count + 1);

  for( ; (s = list[i]) != 0 ; ++i )
  {
    strfcpy(path,sizeof(path),"/usr/bin/%s",s);
  
    if(stat(path,&st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 0755) == 0755)
    {
      editors[j++] = strdup(s);
      break;
    }
  }

  editors[j] = 0;
  
  return true;
}

static bool viconfig_write_profile(const char *path)
{
  int fd = -1;
  FILE *file = 0;
  
  if(!mkdir_recurse("etc/profile.d"))
    return false;

  if(
    (fd = open("etc/profile.d/viconfig.sh",O_CREAT|O_TRUNC|O_WRONLY,0755)) == -1 ||
    fchown(fd,0,0) == -1                                                         ||
    fchmod(fd,0755) == -1                                                        ||
    (file = fdopen(fd,"wb")) == 0
  )
  {
    error(strerror(errno));
    if(fd != -1)
      close(fd);
    return false;
  }

  fprintf(file,
    "VISUAL=\"%s\"\n"
    "EDITOR=\"%s\"\n"
    "\n"
    "export VISUAL\n"
    "export EDITOR\n",
    path,
    path
  );

  fclose(file);

  return true;
}

static bool viconfig_start(void)
{
  if(!viconfig_setup_editors())
    return false;

  if(!ui_window_list(VI_TITLE,VI_TEXT,editors,&editor))
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
