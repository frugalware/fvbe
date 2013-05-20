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

static char **zones = 0;
static char *zone = 0;
static bool utc = true;

static bool timeconfig_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};
  size_t i = 0;
  size_t size = 4096;
  char *p = 0;
  
  if((file = fopen("/usr/share/zoneinfo/zone.tab","rb")) == 0)
  {
    error(strerror(errno));
    return false;
  }

  zones = alloc(char *,size);

  while(fgets(line,sizeof(line),file) != 0)
  {
    if(
      i == size - 1                    ||
      *line == '#'                     ||
      strtok(line,SPACE_CHARS) == 0    ||
      strtok(0,SPACE_CHARS) == 0       ||
      (p = strtok(0,SPACE_CHARS)) == 0
    )
      continue;
    
    zones[i++] = strdup(p);
  }

  fclose(file);

  zones[i] = 0;
  
  zones = redim(zones,char *,i + 1);

  qsort(zones,i,sizeof(char *),charpp_qsort);

  return true;
}

static bool update_via_old(const char *zone,bool utc)
{
  char buf[_POSIX_ARG_MAX] = {0};

  if(unlink("etc/localtime") == -1 && errno != ENOENT)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(buf,sizeof(buf),"/usr/share/zoneinfo/%s",zone);

  if(symlink(buf,"etc/localtime") == -1)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(buf,sizeof(buf),"hwclock --systohc %s",(utc) ? "--utc" : "--localtime");

  if(!execute(buf,g->guestroot,0))
    return false;

  return true;
}

static bool update_via_new(const char *zone,bool utc)
{
  char command[_POSIX_ARG_MAX] = {0};
  struct stat st = {0};
  
  strfcpy(command,sizeof(command),"timedatectl set-timezone '%s'",shell_escape(zone,true));
  
  if(!execute(command,g->guestroot,0))
    return false;

  if(stat("etc/adjtime",&st) == -1)
  {
    strfcpy(command,sizeof(command),"hwclock --systohc %s",(utc) ? "--utc" : "--localtime");

    if(!execute(command,g->guestroot,0))
      return false;
  }

  strfcpy(command,sizeof(command),"timedatectl set-local-rtc %s",(utc) ? "0" : "1");

  if(!execute(command,g->guestroot,0))
    return false;
  
  return true;  
}

static bool timeconfig_action(const char *zone,bool utc)
{
  if(g->insetup)
    return update_via_old(zone,utc);
  
  return update_via_new(zone,utc);
}

static bool timeconfig_start(void)
{
  if(!timeconfig_setup())
    return false;

  if(!ui_window_time(zones,&zone,&utc))
    return false;

  return true;
}

static bool timeconfig_finish(void)
{
  bool success = true;

  if(zone != 0)
    success = timeconfig_action(zone,utc);

  if(zones != 0)
  {
    for( char **p = zones ; *p != 0 ; ++p )
      free(*p);

    free(zones);

    zones = 0;
  }

  zone = 0;
  
  utc = true;

  return success;
}

struct tool timeconfig_tool =
{
  timeconfig_start,
  timeconfig_finish,
  __FILE__
};
