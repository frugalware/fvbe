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

static inline bool preconfig_copy_fdb(const char *fdb)
{
  char old[PATH_MAX] = {0};
  char new[PATH_MAX] = {0};
  
  strfcpy(old,sizeof(old),"%s/packages/%s.fdb",ISO_ROOT,fdb);
  
  strfcpy(new,sizeof(new),"%s/var/lib/pacman-g2/%s.fdb",g->guestroot,fdb);
  
  return copy(old,new);
}

static bool preconfig_create_paths(void)
{
  size_t i = 0;
  static const char *paths[] =
  {
    "/sys",
    "/proc",
    "/dev",
    "/tmp",
    "/var/tmp",
    "/var/lib/pacman-g2",
    "/var/cache/pacman-g2/pkg",
    "/var/log",
    "/etc/X11/xorg.conf.d",
    0
  };  
  char path[PATH_MAX] = {0};

  for( ; paths[i] != 0 ; ++i )
  {
    strfcpy(path,sizeof(path),"%s%s",g->guestroot,paths[i]);
    
    if(!mkdir_recurse(path))
      return false;
  }

  return true;
}

extern bool preconfig_mount_extra(void)
{
  char path[PATH_MAX] = {0};

  strfcpy(path,sizeof(path),"%s/dev",g->guestroot);

  if(mount("none",path,"devtmpfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(path,sizeof(path),"%s/proc",g->guestroot);

  if(mount("none",path,"proc",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(path,sizeof(path),"%s/sys",g->guestroot);

  if(mount("none",path,"sysfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(path,sizeof(path),"%s/tmp",g->guestroot);

  if(mount("none",path,"tmpfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(path,sizeof(path),"%s/var/tmp",g->guestroot);
  
  if(mount("none",path,"tmpfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  return true;
}


static bool preconfig_prepare_source(void)
{
  char path[PATH_MAX] = {0};
  struct stat st = {0};
  char groups[LINE_MAX] = {0};
  const char *source = "unknown";

  if(g->infvbe && stat(ISO_ROOT "/packages",&st) == 0)
  {
    file2str(ISO_ROOT "/packages/groups",groups,sizeof(groups));

    if(strlen(groups) == 0)
    {
      error("no groups file or empty group files");
      return false;
    }
    
    g->groups = strdup(groups);
      
    strfcpy(path,sizeof(path),"%s/var/cache/pacman-g2/pkg",g->guestroot);
     
    if(mount(ISO_ROOT "/packages",path,"",MS_BIND,0) == -1)
    {
      error(strerror(errno));
      error("failed to mount iso package cache");
      return false;
    }
      
    if(!preconfig_copy_fdb("frugalware") && !preconfig_copy_fdb("frugalware-current"))
    {
      error("failed to find the fdb");
      return false;
    }
      
    source = "iso";
  }
  else if(g->infvbe)
  {
    error("no packages on the iso");
    
    source = "network";
  }
  
  eprintf("%s: using %s for package source\n",__func__,source);
  
  return true;
}

static bool preconfig_run(void)
{
  if(!preconfig_create_paths())
    return false;

  if(!preconfig_mount_extra())
    return false;

  if(!preconfig_prepare_source())
    return false;

  return true;
}

static void preconfig_reset(void)
{
}

struct module preconfig_module =
{
  preconfig_run,
  preconfig_reset,
  __FILE__
};
