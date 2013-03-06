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

static struct format **targets = 0;

static inline void free_target(struct format *p)
{
  free(p->devicepath);

  free(p->size);

  free(p->filesystem);

  free(p->newfilesystem);

  free(p->options);

  free(p->mountpath);

  free(p);
}

static inline void add_target(struct format *p,int *n,int *size)
{
  if(*n == *size)
  {
    *size *= 2;
    targets = realloc(targets,*size * sizeof(struct format *));
  }

  targets[*n] = p;

  *n += 1;
}

static inline void probe_filesystem(struct format *target)
{
  blkid_probe probe = 0;
  static char *filesystems[] =
  {
    "ext2",
    "ext3",
    "ext4",
    "reiserfs",
    "jfs",
    "xfs",
    "btrfs",
    "swap",
    0
  };
  const char *filesystem = "unknown";
  const char *result = 0;

  if((probe = blkid_new_probe_from_filename(target->devicepath)) == 0)
    goto bail;

  if(blkid_probe_enable_superblocks(probe,true) == -1)
    goto bail;

  if(blkid_probe_filter_superblocks_type(probe,BLKID_FLTR_ONLYIN,filesystems) == -1)
    goto bail;

  if(blkid_probe_set_superblocks_flags(probe,BLKID_SUBLKS_TYPE) == -1)
    goto bail;

  if(blkid_do_probe(probe) == -1)
    goto bail;

  if(blkid_probe_lookup_value(probe,"TYPE",&result,0) == -1)
    goto bail;

  filesystem = result;

bail:

  target->filesystem = strdup(filesystem);

  if(probe != 0)
    blkid_free_probe(probe);
}

static bool format_setup(void)
{
  struct device **devices = 0;
  struct device **p = 0;
  int n = 0;
  int size = 128;
  int i = 0;
  int j = 0;

  if((devices = device_probe_all(true,true)) == 0)
    return false;

  targets = malloc0(size * sizeof(struct format *));

  for( p = devices ; *p != 0 ; ++p )
  {
    struct device *device = *p;
    struct disk *disk = disk_open(device);
    struct format *target = 0;
    char buf[PATH_MAX] = {0};

    if(disk != 0)
    {
      for( i = 0, j = disk_partition_get_count(disk) ; i < j ; ++i )
      {
        const char *purpose = disk_partition_get_purpose(disk,i);

        if(
          strcmp(purpose,"data") != 0 &&
          strcmp(purpose,"swap") != 0 &&
          strcmp(purpose,"efi")  != 0
        )
          continue;

        target = malloc0(sizeof(struct format));

        add_target(target,&n,&size);

        strfcpy(buf,sizeof(buf),"%s%d",device_get_path(device),disk_partition_get_number(disk,i));

        target->devicepath = strdup(buf);

        size_to_string(buf,sizeof(buf),disk_partition_get_size(disk,i),false);

        target->size = strdup(buf);

        probe_filesystem(target);
      }
    }

    disk_close(disk);

    device_close(device);
  }

  add_target(0,&n,&size);

  free(devices);

  targets = realloc(targets,n * sizeof(struct format *));

  return (targets[0] != 0);
}

static void format_filter_devices(void)
{
  size_t i = 0;
  size_t j = 0;

  for( ; targets[i] != 0 ; ++i )
  {
    struct format *p = targets[i];

    if(p->newfilesystem == 0 && p->options == 0 && p->mountpath == 0)
    {
      free_target(p);
      continue;
    }

    targets[j++] = p;
  }

  targets[j++] = 0;

  targets = realloc(targets,j * sizeof(struct format *));
}

static bool format_sort_devices(void)
{
  struct format **p = targets;
  struct format *t = 0;

  for( ; *p != 0 ; ++p )
  {
    struct format *target = *p;

    if(strcmp(target->newfilesystem,"swap") != 0 && strcmp(target->mountpath,"/") == 0)
      break;
  }

  if(*p == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  t = targets[0];

  targets[0] = *p;

  *p = t;

  return true;
}

static bool format_process_devices(void)
{
  int i = 0;
  int j = 0;
  int padding = 0;
  char text[256] = {0};
  int percent = 0;
  const char *program = 0;
  char command[_POSIX_ARG_MAX] = {0};
  char path[PATH_MAX] = {0};

  for( ; targets[j] != 0 ; ++j )
    ;

  if(j < 10)
    padding = 1;
  else if(j < 100)
    padding = 2;
  else if(j < 1000)
    padding = 3;
  else if(j < 10000)
    padding = 4;

  for( ; i < j ; ++i )
  {
    struct format *target = targets[i];

    strfcpy(text,sizeof(text),"(%*d/%d) - %-8s - %-8s",padding,i+1,j,target->devicepath,target->newfilesystem);

    percent = (float) (i+1) / j * 100;

    ui_dialog_progress(_("Formatting"),text,percent);

    if(target->format)
    {
      if(strcmp(target->newfilesystem,"ext2") == 0)
        program = "mkfs.ext2";
      else if(strcmp(target->newfilesystem,"ext3") == 0)
        program = "mkfs.ext3";
      else if(strcmp(target->newfilesystem,"ext4") == 0)
        program = "mkfs.ext4";
      else if(strcmp(target->newfilesystem,"reiserfs") == 0)
        program = "mkfs.reiserfs -q";
      else if(strcmp(target->newfilesystem,"jfs") == 0)
        program = "mkfs.jfs -q";
      else if(strcmp(target->newfilesystem,"xfs") == 0)
        program = "mkfs.xfs -f";
      else if(strcmp(target->newfilesystem,"btrfs") == 0)
        program = "mkfs.btrfs";
      else if(strcmp(target->newfilesystem,"swap") == 0)
        program = "mkswap";

      strfcpy(command,sizeof(command),"%s %s %s",program,target->options,target->devicepath);

      if(!execute(command,"/",0))
      {
        ui_dialog_progress(0,0,-1);
        return false;
      }
    }

    if(strcmp(target->newfilesystem,"swap") == 0)
    {
      strfcpy(command,sizeof(command),"swapon %s",target->devicepath);

      if(!execute(command,"/",0))
      {
        ui_dialog_progress(0,0,-1);
        return false;
      }
    }
    else
    {
      strfcpy(path,sizeof(path),"%s%s",INSTALL_ROOT,target->mountpath);

      if(!mkdir_recurse(path))
      {
        ui_dialog_progress(0,0,-1);
        return false;
      }

      if(mount(target->devicepath,path,target->newfilesystem,0,0) == -1)
      {
        error(strerror(errno));
        ui_dialog_progress(0,0,-1);
        return false;
      }
    }
  }

  ui_dialog_progress(0,0,-1);

  return true;
}

static bool format_create_paths(void)
{
  size_t i = 0;
  static const char *paths[] =
  {
    "/sys",
    "/proc",
    "/dev",
    "/tmp",
    "/var/tmp",
    "/var/cache/pacman-g2",
    "/var/log",
    "/etc/X11/xorg.conf.d",
    0
  };  
  char path[PATH_MAX] = {0};

  for( ; paths[i] != 0 ; ++i )
  {
    strfcpy(path,sizeof(path),INSTALL_ROOT "%s",paths[i]);
    
    if(!mkdir_recurse(path))
      return false;
  }

  return true;
}

static void format_prepare_fstab(void)
{
  size_t i = 0;
  char text[TEXT_MAX] = {0};
  
  for( ; targets[i] != 0 ; ++i )
    ;
  
  g->fstabdata = malloc0(sizeof(char *) * (i+1));

  g->fstabdata[i] = 0;

  do
  {
    --i;
    strfcpy(text,sizeof(text),"%s:%s:%s",targets[i]->devicepath,targets[i]->mountpath,targets[i]->newfilesystem);
    g->fstabdata[i] = strdup(text);
  } while(i > 0);
}

static bool format_run(void)
{
  if(!format_setup())
    return false;

  if(!ui_window_format(targets))
    return false;

  format_filter_devices();

  if(!format_sort_devices())
    return false;

  if(!format_process_devices())
    return false;

  if(!format_create_paths())
    return false;

  if(!mount_special())
    return false;

  format_prepare_fstab();

  return true;
}

static void format_reset(void)
{
  struct format **p = 0;

  if(targets != 0)
  {
    for( p = targets ; *p != 0 ; ++p )
    {
      free_target(*p);
    }

    free(targets);

    targets = 0;
  }
}

struct module format_module =
{
  format_run,
  format_reset,
  __FILE__
};
