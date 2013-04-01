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
static struct raid **stop = 0;

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
          
          if(!isdevicebusy(path) && (unused[j] = device_open(path)) != 0)
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
        raid_close(raid,true,false);
      
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

  stop = alloc(struct raid *,1);

  return true;
}

static bool raid_flush(void)
{
  int count = 0;
  int padding = 0;
  int percent = 0;
  char text[TEXT_MAX] = {0};

  if(*stop != 0)
  {
    for( int i = 0, count = 1 ; stop[i] != 0 ; ++i )
      ++count;
    
    padding = get_number_padding(count);
    
    for( int i = 0 ; i < count ; ++i )
    {
      struct raid *raid = stop[i];
    
      strfcpy(text,sizeof(text),"(%*d/%d) - %s",padding,i+1,count,raid_get_path(raid));

      percent = (float) (i+1) / count * 100;

      ui_dialog_progress(_("Stopping RAID Devices"),text,percent);

      if(!raid_stop(raid))
      {
        ui_dialog_progress(0,0,-1);
        return false;
      }
    }
  }
  
  if(*used != 0)
  {
    for( int i = 0, count = 1 ; used[i] != 0 ; ++i )
      ++count;
    
    padding = get_number_padding(count);
    
    for( int i = 0 ; i < count ; ++i )
    {
      struct raid *raid = used[i];
    
      strfcpy(text,sizeof(text),"(%*d/%d) - %s",padding,i+1,count,raid_get_path(raid));

      percent = (float) (i+1) / count * 100;

      ui_dialog_progress(_("Starting RAID Devices"),text,percent);

      if(!raid_start(raid))
      {
        ui_dialog_progress(0,0,-1);
        return false;
      }
    }
  }
  
  return true;
}

static bool raid_run(void)
{
  if(!raid_setup())
    return false;

  if(*unused == 0 && *used == 0)
    return true;

  if(!ui_window_raid(&unused,&used,&stop))
    return false;

  if(!raid_flush())
    return false;

  return true;
}

static void raid_reset(void)
{
  if(unused != 0)
  {
    for( size_t i = 0 ; unused[i] != 0 ; ++i )
      device_close(unused[i]);
    
    free(unused);
    
    unused = 0;
  }
  
  if(used != 0)
  {
    for( size_t i = 0 ; used[i] != 0 ; ++i )
      raid_close(used[i],true,true);
    
    free(used);
    
    used = 0;
  }
  
  if(stop != 0)
  {
    for( size_t i = 0 ; stop[i] != 0 ; ++i )
      raid_close(stop[i],false,true);
    
    free(stop);
    
    stop = 0;
  }
}

struct module raid_module =
{
  raid_run,
  raid_reset,
  __FILE__
};
