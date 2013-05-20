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

static void fetch_root_device(char *s,size_t n)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};
  char *device = 0;
  char *root = 0;

  if(s == 0 || n == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }
  
  *s = 0;
  
  if((file = fopen("/proc/mounts","rb")) == 0)
  {
    error(strerror(errno));
    return;
  }
  
  while(fgets(line,sizeof(line),file) != 0)
  {
    if((device = strtok(line,SPACE_CHARS)) == 0)
      continue;
    
    if((root = strtok(0,SPACE_CHARS)) == 0)
      continue;
    
    if(strcmp(root,g->guestroot) == 0)
      break;
  }
  
  if(feof(file) == 0)
    strfcpy(s,n,"%s",device);
  
  fclose(file);
}

static void fetch_real_devices(const char *base,char *s,size_t n)
{
  char buf[PATH_MAX] = {0};
  glob_t ge = {0};
  size_t i = 0;

  if(base == 0 || s == 0 || n == 0)
    return;

  strfcpy(buf,sizeof(buf),"%s/slaves/*",base);
  
  if(glob(buf,0,0,&ge) != 0)
  {
    strfcpy(buf,sizeof(buf),"%s",base);

    if(*s == 0)
      strfcpy(s,n,"/dev/%s",basename(buf));
    else
      strfcat(s,n,":/dev/%s",basename(buf));

    globfree(&ge);
    
    return;
  }

  for( ; i < ge.gl_pathc ; ++i )
    fetch_real_devices(ge.gl_pathv[i],s,n);
  
  globfree(&ge);
}


static bool grubconfig_action(void)
{
  char device[PATH_MAX] = {0};
  char path[PATH_MAX] = {0};
  char devices[PATH_MAX] = {0};
  char *s = 0;
  char *p = 0;
  int i = 0;
  int count = 0;
  int padding = 0;
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

  padding = get_number_padding(count);

  for( p = devices ; (s = strtok(p,":")) != 0 ; p = 0, ++i )
  {
    strfcpy(command,sizeof(command),"grub-install --recheck --no-floppy --boot-directory=/boot '%.8s'",shell_escape(s));
   
    strfcpy(path,sizeof(path),"(%*d/%d) - %.8s",padding,i+1,count,s);
   
    ui_dialog_progress(_("Installing GRUB"),path,get_percent(i+1,count));
   
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
