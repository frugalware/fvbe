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

static bool action = false;

static bool grubconfig_action(void)
{
  char device[PATH_MAX] = {0};
  char path[PATH_MAX] = {0};
  char devices[PATH_MAX] = {0};
  char *s = 0;
  char *p = 0;
  size_t i = 0;
  size_t count = 0;
  int percent = 0;
  char command[_POSIX_ARG_MAX] = {0};
  
  fetch_root_device(device,sizeof(device));
  
  if(strlen(device) == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  strfcpy(path,sizeof(path),"/sys/class/block/%s",basename(device));
  
  fetch_real_devices(path,devices,sizeof(devices));

  if(strlen(devices) == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  count = strpbrklen(devices,":") + 1;

  for( p = devices ; (s = strtok(p,":")) != 0 ; p = 0, ++i )
  {
    strfcpy(command,sizeof(command),"grub-install --recheck --no-floppy --boot-directory=/boot '%.8s'",s);
   
    percent = (float) (i+1) / count * 100;
   
    strfcpy(path,sizeof(path),"(%zu/%zu) - %.8s",i+1,count,s);
   
    ui_dialog_progress(_("Installing GRUB"),path,percent);
   
    if(!execute(command,g->guestroot,0))
    {
      ui_dialog_progress(0,0,-1);
      return false;
    }
  }

  ui_dialog_progress(_("Generating GRUB Config"),"",100);

  if(!execute("grub-mkconfig -o /boot/grub/grub.cfg",g->guestroot,0))
  {
    ui_dialog_progress(0,0,-1);
    return false;
  }

  ui_dialog_progress(0,0,-1);

  return true;
}


static bool grubconfig_start(void)
{
  action = ui_dialog_yesno(GRUB_TITLE,GRUB_TEXT,false);

  return true;
}

static bool grubconfig_finish(void)
{
  bool success = true;

  if(action)
  {
    success = grubconfig_action();
  
    action = false;
  }

  return success;
}

struct tool grubconfig_tool =
{
  grubconfig_start,
  grubconfig_finish,
  __FILE__
};
