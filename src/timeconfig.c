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

static const char *tzfile = "etc/localtime";
static const char *tzdir = "usr/share/zoneinfo";
static const char *tzsearchdir = "usr/share/zoneinfo/posix";

static size_t size = 0;
static size_t count = 0;
static char **data = 0;
static char *zone = 0;
static bool utc = true;

static int nftw_callback(const char *path,const struct stat *st,int type,struct FTW *fb)
{
  if(type == FTW_D || type == FTW_DP)
    return 0;

  if(data == 0)
  {
    ++size;
  }
  else
  {
    data[count] = strdup(path + strlen(tzsearchdir) + 1);
    ++count;
  }

  return 0;
}

static int qsort_callback(const void *a,const void *b)
{
  const char *A = *(const char **) a;
  const char *B = *(const char **) b;

  return strcmp(A,B);
}

static bool timeconfig_setup(void)
{
  if(nftw(tzsearchdir,nftw_callback,512,FTW_DEPTH|FTW_PHYS) == -1)
  {
    error(strerror(errno));
    return false;
  }

  data = malloc0(sizeof(char *) * (size + 1));

  if(nftw(tzsearchdir,nftw_callback,512,FTW_DEPTH|FTW_PHYS) == -1)
  {
    error(strerror(errno));
    return false;
  }

  qsort(data,size,sizeof(char *),qsort_callback);

  return true;
}

static bool timeconfig_action(const char *zone,bool utc)
{
  char buf[_POSIX_ARG_MAX] = {0};

  if(unlink(tzfile) == -1 && errno != ENOENT)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(buf,sizeof(buf),"%s%s/%s",g->hostroot,tzdir,zone);

  if(symlink(buf,tzfile) == -1)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(buf,sizeof(buf),"hwclock --systohc %s",(utc) ? "--utc" : "--localtime");

  if(!execute(buf,g->guestroot,0))
    return false;

  return true;
}

static bool timeconfig_start(void)
{
  if(!timeconfig_setup())
    return false;

  if(!ui_window_time(data,&zone,&utc))
    return false;

  return true;
}

static bool timeconfig_finish(void)
{
  bool success = true;

  if(zone)
    success = timeconfig_action(zone,utc);

  size = 0;

  count = 0;

  if(data != 0)
  {
    for( size_t i = 0 ; data[i] != 0 ; ++i )
      free(data[i]);

    free(data);

    data = 0;
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
