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

static struct nmdevice **nmdevices = 0;
static struct nmprofile **nmprofiles = 0;

static inline const char *nm_next_token(const char *p)
{
  char c1 = 0;
  char c2 = 0;

  if(*p == 0)
    return 0;

  for( ; (c2 = *p) != 0 ; ++p )
  {
    if(c1 != '\\' && c2 == ':')
      break;
    else if(c2 == '\n')
      break;
  
    c1 = (c1 == '\\' && c2 == '\\') ? 0 : c2;
  }

  return p;
}

static inline const char *nm_unescape_token(const char *s1,size_t n)
{
  const char *e1 = s1 + n;
  static char buf[LINE_MAX] = {0};
  char pc = 0;
  char *s2 = buf;
  char *e2 = buf + sizeof(buf) - 1;

  for( ; s1 < e1 ; pc = *s1++ )
  {
    const char cc = *s1;
    
    if(pc != '\\' && cc == '\\')
      continue;
    
    if(s2 < e2)
      *s2++ = cc;
  }
  
  *s2 = 0;
  
  return buf;
}

static inline bool nm_parse_line(const char *line,bool (*cb) (int,const char *,void *),void *data)
{
  const char *a = line;
  const char *b = 0;
  int token = 1;

  for( ; (b = nm_next_token(a)) != 0 ; a = b + ((*a == 0) ? 0 : 1), ++token )
    if(!cb(token,nm_unescape_token(a,b-a),data))
      return false;    

  return true;
}

static bool nm_parse_device(int token,const char *value,void *data)
{
  struct nmdevice *p = data;
  bool rv = true;

  switch(token)
  {
    case 1:
      rv = (strcmp(value,"GENERAL") == 0);
      break;
    
    case 2:
      p->device = strdup(value);
      break;
    
    case 3:
      p->type = strdup(value);
      break;
    
    case 4:
      p->vendor = strdup(value);
      break;
    
    case 5:
      p->product = strdup(value);
      break;
    
    case 6:
      p->driver = strdup(value);
      break;
    
    case 7:
      p->driverversion = strdup(value);
      break;
    
    case 8:
      p->firmwareversion = strdup(value);
      break;
    
    case 9:
      p->hwaddr = strdup(value);
      break;
    
    case 10:
      p->state = atoi(value);
      break;
    
    case 11:
      p->reason = atoi(value);
      break;
    
    case 12:
      p->udi = strdup(value);
      break;  
    
    case 13:
      p->ipiface = strdup(value);
      break;
    
    case 14:
      if(strcmp(value,"yes") == 0)
        p->nmmanaged = true;
      else if(strcmp(value,"no") == 0)
        p->nmmanaged = false;
      break;
    
    case 15:
      if(strcmp(value,"yes") == 0)
        p->autoconnect = true;
      else if(strcmp(value,"no") == 0)
        p->autoconnect = false;
      break;
    
    case 16:
      if(strcmp(value,"yes") == 0)
        p->firmwaremissing = true;
      else if(strcmp(value,"no") == 0)
        p->firmwaremissing = false;
      break;
    
    case 17:
      p->connection = strdup(value);
      break;
    
    default:
      eprintf("%s: unknown token value of %d\n",__func__,token);
      rv = false;
      break;
  }

  return rv;
}

static bool nm_parse_wired(int token,const char *value,void *data)
{
  struct nmdevice *p = data;
  bool rv = true;

  switch(token)
  {
    case 1:
      rv = (strcmp(value,"WIRED-PROPERTIES") == 0);
      break;
    
    case 2:
      if(strcmp(value,"on") == 0)
        p->carrier = true;
      else if(strcmp(value,"off") == 0)
        p->carrier = false;
      break;
    
    default:
      eprintf("%s: unknown token value of %d\n",__func__,token);
      rv = false;
      break;
  }

  return rv;
}

static bool nm_parse_wifi(int token,const char *value,void *data)
{
  struct nmdevice *p = data;
  bool rv = true;

  switch(token)
  {
    case 1:
      rv = (strcmp(value,"WIFI-PROPERTIES") == 0);
      break;

    case 2:
      if(strcmp(value,"yes") == 0)
        p->wep = true;
      else if(strcmp(value,"no") == 0)
        p->wep = false;
      break;

    case 3:
      if(strcmp(value,"yes") == 0)
        p->wpa = true;
      else if(strcmp(value,"no") == 0)
        p->wpa = false;
      break;

    case 4:
      if(strcmp(value,"yes") == 0)
        p->wpa2 = true;
      else if(strcmp(value,"no") == 0)
        p->wpa2 = false;
      break;

    case 5:
      if(strcmp(value,"yes") == 0)
        p->tkip = true;
      else if(strcmp(value,"no") == 0)
        p->tkip = false;
      break;

    case 6:
      if(strcmp(value,"yes") == 0)
        p->ccmp = true;
      else if(strcmp(value,"no") == 0)
        p->ccmp = false;
      break;

    case 7:
      if(strcmp(value,"yes") == 0)
        p->ap = true;
      else if(strcmp(value,"no") == 0)
        p->ap = false;
      break;

    case 8:
      if(strcmp(value,"yes") == 0)
        p->adhoc = true;
      else if(strcmp(value,"no") == 0)
        p->adhoc = false;
      break;
    
    default:
      eprintf("%s: unknown token value of %d\n",__func__,token);
      rv = false;
      break;
  }

  return rv;
}

static void nmdevice_free(struct nmdevice *p)
{
  if(p == 0)
    return;

  free(p->device);
  
  free(p->type);
  
  free(p->vendor);
  
  free(p->product);
  
  free(p->driver);
  
  free(p->driverversion);
  
  free(p->firmwareversion);
  
  free(p->hwaddr);
  
  free(p->udi);
  
  free(p->ipiface);
  
  free(p->connection);
  
  free(p);
}

static void nmprofile_free(struct nmprofile *p)
{
  if(p == 0)
    return;
  
  free(p->oldpath);
  
  free(p->newpath);
  
  if(p->data != 0)
    iniparser_freedict(p->data);
  
  free(p);
}

static bool nmconfig_setup_devices(void)
{
  FILE *pipe = 0;
  char line[LINE_MAX] = {0};
  size_t i = 0;
  size_t size = 4096;
  struct nmdevice *device = 0;
  
  if((pipe = popen("LC_ALL=C nmcli -t -m tabular -f GENERAL -e yes device list 2>&1","r")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  nmdevices = alloc(struct nmdevice *,size);
  
  while(fgets(line,sizeof(line),pipe) != 0)
  {
    if(i == size - 1)
      continue;
  
    device = alloc(struct nmdevice,1);
  
    if(!nm_parse_line(line,nm_parse_device,device))
    {
      pclose(pipe);
      nmdevice_free(device);
      return false;
    }
  
    nmdevices[i++] = device;
  }
  
  nmdevices[i] = 0;

  nmdevices = redim(nmdevices,struct nmdevice *,i + 1);
  
  if(pclose(pipe) == -1)
  {
    error(strerror(errno));
    return false;
  }
  
  return true;
}

static bool nmconfig_setup_devices_extras(void)
{
  for( struct nmdevice **p = nmdevices ; *p != 0 ; ++p )
  {
    struct nmdevice *device = *p;
    const char *field = 0;
    bool (*fun) (int,const char *,void *) = 0;
    char command[_POSIX_ARG_MAX] = {0};
    FILE *pipe = 0;
    char line[LINE_MAX] = {0};
    
    if(strcmp(device->type,WIRED_KEY) == 0)
    {
      field = "WIRED-PROPERTIES";
      fun = nm_parse_wired;
    }
    else if(strcmp(device->type,WIFI_KEY) == 0)
    {
      field = "WIFI-PROPERTIES";
      fun = nm_parse_wifi;
    }
    else
      continue;
    
    strfcpy(command,sizeof(command),"LC_ALL=C nmcli -t -m tabular -f %s -e yes device list iface %s 2>&1",field,device->device);
    
    if(
      (pipe = popen(command,"r")) == 0   ||
      fgets(line,sizeof(line),pipe) == 0 ||
      !nm_parse_line(line,fun,device)
    )
    {
      pclose(pipe);
      error("failed to probe for extra device information");
      return false;
    }
    
    pclose(pipe);
  }

  return true;
}

static bool nmconfig_setup_profiles(void)
{
  DIR *dir = 0;
  size_t i = 0;
  size_t size = 4096;
  struct dirent entry = {0};
  struct dirent *p = 0;
  char path[PATH_MAX] = {0};
  struct stat st = {0};
  dictionary *data = 0;
  struct nmprofile *profile = 0;
  
  if((dir = opendir("/etc/NetworkManager/system-connections")) == 0)
  {
    error(strerror(errno));
    return 0;
  }
  
  nmprofiles = alloc(struct nmprofile *,size);
  
  while(readdir_r(dir,&entry,&p) == 0 && p != 0)
  {
    const char *name = p->d_name;
    
    strfcpy(path,sizeof(path),"/etc/NetworkManager/system-connections/%s",name);
    
    if(
      i == size - 1            ||
      strcmp(name,".") == 0    ||
      strcmp(name,"..") == 0   ||
      stat(path,&st) == -1     ||
      st.st_uid != 0           ||
      st.st_gid != 0           ||
      (st.st_mode & 0077) != 0
    )
      continue;
    
    if((data = iniparser_load(path)) == 0)
      continue;
    
    profile = alloc(struct nmprofile,1);
    
    profile->oldpath = strdup(path);
    
    profile->data = data;
    
    nmprofiles[i++] = profile;
  }

  nmprofiles[i] = 0;
  
  nmprofiles = redim(nmprofiles,struct nmprofile *,i + 1);

  closedir(dir);

  return true;
}

static bool nmconfig_start(void)
{
  if(!nmconfig_setup_devices())
    return false;

  if(!nmconfig_setup_devices_extras())
    return false;

  if(!nmconfig_setup_profiles())
    return false;

  if(!ui_window_nm(nmdevices,&nmprofiles))
    return false;

  return true;
}

static bool nmconfig_finish(void)
{
  int i = 0;
  int j = 0;
  int k = 0;

  if(nmprofiles != 0)
  {
    for( ; nmprofiles[j] != 0 ; ++j )
      ;

    for( ; nmprofiles[i] != 0 ; ++i )
    {
      struct nmprofile *profile = nmprofiles[i];

      ui_dialog_progress(_("Writing Network Profile Changes"),"",get_percent(i,j));
    
      if(profile->data != 0 && profile->newpath != 0)
      {
        int fd = -1;
        FILE *file = 0;
      
        if(profile->oldpath != 0)
          remove(profile->oldpath);
      
        if(
          (fd = open(profile->newpath,O_CREAT|O_TRUNC|O_WRONLY,0600)) == -1 ||
          fchown(fd,0,0) == -1                                              ||
          fchmod(fd,0600) == -1                                             ||
          (file = fdopen(fd,"wb")) == 0
        )
        {
          error(strerror(errno));
          if(fd != -1)
            close(fd);
          ui_dialog_progress(0,0,-1);
          return false;
        }
      
        iniparser_dump_ini(profile->data,file);
      
        fclose(file);      
      }
      else if(profile->data == 0 && profile->oldpath != 0)
      {
        remove(profile->oldpath);
      }
  
      nmprofile_free(profile);
    }

    ui_dialog_progress(0,0,-1);

    free(nmprofiles);
  
    nmprofiles = 0;
  }

  if(nmdevices != 0)
  {
    for( ; nmdevices[k] != 0 ; ++k )
      nmdevice_free(nmdevices[k]);

    free(nmdevices);

    nmdevices = 0;
  }

  return true;
}

struct tool nmconfig_tool =
{
  nmconfig_start,
  nmconfig_finish,
  __FILE__
};
