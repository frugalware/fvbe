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

static inline char *probe_uuid(const char *path)
{
  blkid_probe probe = 0;
  const char *uuid = 0;
  
  if((probe = blkid_new_probe_from_filename(path)) == 0)
  {
    error(strerror(errno));
    goto bail;
  }
  
  if(blkid_probe_enable_superblocks(probe,true) == -1)
  {
    error("failed to configure probe");
    goto bail;
  }
  
  if(blkid_probe_set_superblocks_flags(probe,BLKID_SUBLKS_UUID) == -1)
  {
    error("failed to configure probe");
    goto bail;
  }
  
  if(blkid_do_probe(probe) == -1)
  {
    error("failed to probe");
    goto bail;
  }
  
  if(blkid_probe_lookup_value(probe,"UUID",&uuid,0) == -1)
  {
    uuid = 0;
    error("no uuid");
    goto bail;
  }

  uuid = strdup(uuid);

bail:

  if(probe != 0)
    blkid_free_probe(probe);
  
  return (char *) uuid;
}

static bool write_fstab(void)
{
  FILE *file = 0;
  char **p = 0;
  char *device = 0;
  char *path = 0;
  char *filesystem = 0;
  char *uuid = 0;
  
  if((file = fopen("etc/fstab","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,
    "none /dev devtmpfs defaults 0 0\n"
    "none /proc proc defaults 0 0\n"
    "none /sys sysfs defaults 0 0\n"
    "none /tmp tmpfs defaults 0 0\n"
    "none /var/tmp tmpfs defaults 0 0\n"
    "none /dev/pts devpts gid=5,mode=620 0 0\n"
    "none /proc/bus/usb usbfs devgid=23,devmode=664 0 0\n"
    "none /dev/shm tmpfs defaults 0 0\n"
  );

  for( p = g->fstabdata ; *p != 0 ; ++p )
  {
    if(
      (device = strtok(*p,":\n")) == 0    ||
      (path = strtok(0,":\n")) == 0       ||
      (filesystem = strtok(0,":\n")) == 0 ||
      (uuid = probe_uuid(device)) == 0
    )
    {
      errno = EINVAL;
      error(strerror(errno));
      fclose(file);
      return false;
    }

    fprintf(file,
      "UUID=%s %s %s defaults %s\n",
      uuid,
      (strcmp(filesystem,"swap") == 0) ? "swap" : path,
      filesystem,
      (strcmp(filesystem,"swap") == 0) ? "0 0" : "1 1"
    );
    
    free(uuid);
  }

  fclose(file);
  
  return true;
}

static bool postconfig_run(void)
{
  if(chdir(g->guestroot) == -1)
  {
    error(strerror(errno));
    return false;
  }
  
  if(!write_fstab())
    return false;

  if(!langconfig_tool.finish() || !kbconfig_tool.finish())
    return false;

  for( size_t i = 2 ; tools[i] != 0 ; ++i )
  {
    struct tool *tool = tools[i];
    
    if(!tool->start())
    {
      tool->finish();
      return false;
    }
    
    if(!tool->finish())
      return false;
  }

  return true;
}

static void postconfig_reset(void)
{
}

struct module postconfig_module =
{
  postconfig_run,
  postconfig_reset,
  __FILE__
};
