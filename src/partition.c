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

static struct device **devices = 0;
static struct disk **disks = 0;

static bool partition_setup(void)
{
  int i = 0;

  if((devices = device_probe_all(true,false)) == 0)
    return false;

  for( ; devices[i] != 0 ; ++i )
    ;

  disks = alloc(struct disk *,i + 1);

  for( i = 0 ; devices[i] != 0 ; ++i )
    disks[i] = disk_open(devices[i]);

  return true;
}

static bool partition_flush(void)
{
  int i = 0;
  int j = 0;
  int padding = 0;
  char text[TEXT_MAX] = {0};

  for( ; devices[j] != 0 ; ++j )
    ;

  padding = get_number_padding(j);

  for( ; devices[i] != 0 ; ++i )
  {
    struct device *device = devices[i];
    struct disk *disk = disks[i];

    strfcpy(text,sizeof(text),"(%*d/%d) - %s",padding,i+1,j,device_get_path(device));

    ui_dialog_progress(_("Partitioning"),text,get_percent(i+i,j));

    if(disk && !disk_flush(disk))
    {
      ui_dialog_progress(0,0,-1);
      return false;
    }
  }

  ui_dialog_progress(0,0,-1);

  return true;
}

static bool partition_run(void)
{
  if(!partition_setup())
    return false;

  if(!ui_window_partition(devices,disks))
    return false;

  if(!partition_flush())
    return false;

  return true;
}

static void partition_reset(void)
{
  int i = 0;
  int j = 0;

  if(devices != 0)
  {
    for( ; devices[j] != 0 ; ++j )
      ;
  
    if(disks != 0)
    {
      for( i = 0 ; i < j ; ++i )
      {
        disk_close(disks[i],false);
      }

      free(disks);

      disks = 0;
    }

    for( i = 0 ; i < j ; ++i )
    {
      device_close(devices[i]);
    }

    free(devices);

    devices = 0;
  }
}

struct module partition_module =
{
  partition_run,
  partition_reset,
  __FILE__
};
