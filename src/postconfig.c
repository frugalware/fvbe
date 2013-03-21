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

static inline void put_xkb_var(FILE *file,char *s1,char *s2)
{
  if(s1 == 0 || strlen(s1) == 0)
    return;

  fprintf(file,"%8cOption \"%s\" \"%s\"\n",' ',s2,s1);
}

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

static bool write_locale_conf(void)
{
  const char *var = "LANG";
  const char *locale = 0;
  FILE *file = 0;
  size_t i = 0;
  static const char *vars[] =
  {
    "LC_CTYPE",
    "LC_NUMERIC",
    "LC_TIME",
    "LC_COLLATE",
    "LC_MONETARY",
    "LC_MESSAGES",
    "LC_PAPER",
    "LC_NAME",
    "LC_ADDRESS",
    "LC_TELEPHONE",
    "LC_MEASUREMENT",
    "LC_IDENTIFICATION",
    0
  };
  
  if((locale = getenv(var)) == 0 || strlen(locale) == 0)
  {
    eprintf("%s is not defined.\n",var);
    return false;
  }
  
  if((file = fopen("etc/locale.conf","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,
    "LANG=%s\n",
    locale
  );

  for( ; vars[i] != 0 ; ++i )
  {
    var = vars[i];
    
    if((locale = getenv(var)) == 0 || strlen(locale) == 0)
      continue;
    
    fprintf(file,"%s=%s\n",var,locale);
  }

  fclose(file);

  return true;
}

static bool write_vconsole_conf(void)
{
  FILE *file = 0;
  
  if(g->kbdlayout == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  if((file = fopen("etc/vconsole.conf","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,
    "KEYMAP=%s\n"
    "FONT=%s\n",
    g->kbdlayout,
    "ter-v16b"
  );
  
  fclose(file);
  
  return true;
}

static bool write_keyboard_conf(void)
{
  FILE *file = 0;
  
  if((file = fopen("etc/X11/xorg.conf.d/00-keyboard.conf","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }

  fprintf(file,
    "Section \"InputClass\"\n"
    "%8cIdentifier \"system-keyboard\"\n"
    "%8cMatchIsKeyboard \"on\"\n",
    ' ',
    ' '
  );

  put_xkb_var(file,g->xkblayout,"XkbLayout");

  put_xkb_var(file,g->xkbmodel,"XkbModel");
  
  put_xkb_var(file,g->xkbvariant,"XkbVariant");
  
  put_xkb_var(file,g->xkboptions,"XkbOptions");

  fprintf(file,
    "EndSection\n"
  );

  fclose(file);

  return true;
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

static bool write_hostname(const char *hostname,const char *prettyhostname)
{
  FILE *file = 0;
  
  if((file = fopen("etc/hostname","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,"%s\n",hostname);
  
  fclose(file);
  
  if((file = fopen("etc/machine-info","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,"PRETTY_HOSTNAME='%s'\n",prettyhostname);
  
  fclose(file);
  
  return true;
}

static bool is_root_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};
  char *tmp = 0;
  char *user = 0;
  char *pwd = 0;
  bool result = false;

  file = fopen("etc/shadow","rb");

  if(file == 0)
  {
    error(strerror(errno));
    return false;
  }

  while(fgets(line,LINE_MAX,file) != 0)
  {
    tmp = line;

    if((user = strsep(&tmp,":\n")) == 0)
      continue;

    if((pwd = strsep(&tmp,":\n")) == 0)
      continue;

    if(strcmp(user,"root") == 0)
    {
      result = (strlen(pwd) > 1);
      break;
    }
  }

  fclose(file);

  return result;
}

static bool is_user_setup(void)
{
  FILE *file = 0;
  char line[LINE_MAX] = {0};
  char *tmp = 0;
  char *gid = 0;
  bool result = false;

  file = fopen("etc/passwd","rb");

  if(file == 0)
  {
    error(strerror(errno));
    return false;
  }

  while(fgets(line,LINE_MAX,file) != 0)
  {
    tmp = line;

    if(strsep(&tmp,":\n") == 0)
      continue;

    if(strsep(&tmp,":\n") == 0)
      continue;

    if(strsep(&tmp,":\n") == 0)
      continue;

    if((gid = strsep(&tmp,":\n")) == 0)
      continue;

    if(strcmp(gid,"100") == 0)
    {
      result = true;
      break;
    }
  }

  fclose(file);

  return result;
}

static bool root_action(struct account *account)
{
  char command[_POSIX_ARG_MAX] = {0};

  if(account == 0 || account->user == 0 || account->password == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  strfcpy(command,sizeof(command),"echo '%s:%s' | chpasswd",account->user,account->password);

  return execute(command,g->guestroot,0);
}

static bool user_action(struct account *account)
{
  char command[_POSIX_ARG_MAX] = {0};

  if(account == 0 || account->user == 0 || account->password == 0 || account->group == 0 || account->groups == 0 || account->home == 0 || account->shell == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  strfcpy(command,sizeof(command),"useradd -m -c '%s' -g '%s' -G '%s' -d '%s' -s '%s' '%s'",strng(account->name),account->group,account->groups,account->home,account->shell,account->user);

  if(!execute(command,g->guestroot,0))
    return false;

  strfcpy(command,sizeof(command),"echo '%s:%s' | chpasswd",account->user,account->password);

  if(!execute(command,g->guestroot,0))
    return false;

  return true;
}

static void account_free(struct account *account)
{
  if(account == 0)
    return;

  free(account->name);

  free(account->user);

  free(account->password);

  free(account->group);

  free(account->groups);

  free(account->home);

  free(account->shell);

  memset(account,0,sizeof(struct account));
}


static bool mode_action(const char *mode)
{
  const char *old = 0;
  const char *new = "etc/systemd/system/default.target";

  if(strcmp(mode,"Text Console") == 0)
    old = "/lib/systemd/system/multi-user.target";
  else if(strcmp(mode,"Display Manager") == 0)
    old = "/lib/systemd/system/graphical.target";
  else
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(unlink(new) == -1 && errno != ENOENT)
  {
    error(strerror(errno));
    return false;
  }
  
  if(symlink(old,new) == -1)
  {
    error(strerror(errno));
    return false;
  }
  
  return true;
}

static bool postconfig_run(void)
{
  char *hostname = 0;
  char *prettyhostname = 0;
  struct account account = {0};
  static char *modes[] =
  {
    "Text Console",
    "Display Manager",
    0
  };
  char *mode = 0;

  if(chdir(g->guestroot) == -1)
  {
    error(strerror(errno));
    return false;
  }

  if(!write_locale_conf())
    return false;

  if(!write_vconsole_conf())
    return false;

  if(!write_keyboard_conf())
    return false;

  if(!write_fstab())
    return false;

  if(!ui_window_host(&hostname,&prettyhostname) || !write_hostname(hostname,prettyhostname))
  {
    free(hostname);
    free(prettyhostname);
    return false;
  }

  free(hostname);
  
  free(prettyhostname);

  if(!is_root_setup() && (!ui_window_root(&account) || !root_action(&account)))
  {
    account_free(&account);
    return false;
  }

  account_free(&account);

  if(!is_user_setup() && (!ui_window_user(&account) || !user_action(&account)))
  {
    account_free(&account);
    return false;
  }

  account_free(&account);

  if(!ui_window_list(MODE_TITLE,MODE_TEXT,modes,&mode) || !mode_action(mode))
    return false;

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
