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

static struct device **unused = 0;
static struct raid **used = 0;

static bool raid_setup(void)
{
  struct device **devices = 0;
  int size = 4096;
  int i = 0;
  int j = 0;
  int k = 0;
  char path[PATH_MAX] = {0};

  if((devices = device_probe_all(true,true)) == 0)
    return false;  

  unused = alloc(struct device *,size);
  
  used = alloc(struct raid *,size);

  for( ; devices[i] != 0 ; ++i )
  {
    struct device *device = devices[i];
    struct disk *disk = disk_open(device);
    struct raid *raid = raid_open(device);
    
    if(disk != 0)
    {
      for( int l = 0, m = disk_partition_get_count(disk) ; l < m ; ++l )
      {
        if(j == size - 1)
          continue;
      
        if(strcmp(disk_partition_get_purpose(disk,l),"raid") == 0)
        {
          strfcpy(path,sizeof(path),"%s%d",device_get_path(device),disk_partition_get_number(disk,l));
          
          if((unused[j] = device_open(path)) != 0)
            ++j;
        }
      }
      
      disk_close(disk,false);
      
      device_close(device);
      
      continue;
    }
    else if(raid != 0)
    {
      if(k == size - 1)
      {
        raid_close(raid,false);
      
        device_close(device);
      
        continue;
      }
    
      used[k++] = raid;
      
      continue;
    }
    else
    {
      device_close(device);
      
      continue;
    }
  }

  unused[j] = 0;
  
  unused = redim(unused,struct device *,j + 1);
  
  used[k] = 0;

  used = redim(used,struct raid *,k + 1);

  free(devices);

  return true;
}

static bool raid_run(void)
{
  if(!raid_setup())
    return false;

  return true;
}

static void raid_reset(void)
{
}

struct module raid_module =
{
  raid_run,
  raid_reset,
  __FILE__
};
