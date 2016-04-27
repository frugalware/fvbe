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

  free(p->mountpath);

  free(p);
}

static inline void add_target(struct format *p,int *n,int *size)
{
  if(*n == *size)
  {
    *size *= 2;
    targets = redim(targets,struct format *,*size);
  }

  targets[*n] = p;

  *n += 1;
}

static int compare_target(const void *A,const void *B)
{
  struct format *a = * (struct format **) A;
  struct format *b = * (struct format **) B;
  size_t c = dirs_count(a->mountpath);
  size_t d = dirs_count(b->mountpath);
  
  return c - d;
}

static inline void probe_filesystem(struct format *target)
{
  blkid_probe probe = 0;
  static char *filesystems[] =
  {
    "ext2",
    "ext3",
    "ext4",
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

  targets = alloc(struct format *,size);

  for( p = devices ; *p != 0 ; ++p )
  {
    struct device *device = *p;
    struct disk *disk = disk_open(device);
    const char *type = device_get_type(device);
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

        target = alloc(struct format,1);

        add_target(target,&n,&size);

        strfcpy(buf,sizeof(buf),"%s%d",device_get_path(device),disk_partition_get_number(disk,i));

        target->devicepath = strdup(buf);

        size_to_string(buf,sizeof(buf),disk_partition_get_size(disk,i),false);

        target->size = strdup(buf);

        probe_filesystem(target);
      }
    }
    else if(strcmp(type,"raid") == 0)
    {
      target = alloc(struct format,1);
      
      add_target(target,&n,&size);
      
      target->devicepath = strdup(device_get_path(device));
      
      size_to_string(buf,sizeof(buf),device_get_size(device),false);
      
      target->size = strdup(buf);
      
      probe_filesystem(target);
    }

    disk_close(disk,false);

    device_close(device);
  }

  add_target(0,&n,&size);

  free(devices);

  targets = redim(targets,struct format *,n);

  return (targets[0] != 0);
}

static void format_filter_devices(void)
{
  size_t i = 0;
  size_t j = 0;

  for( ; targets[i] != 0 ; ++i )
  {
    struct format *p = targets[i];

    if(p->newfilesystem == 0 && p->mountpath == 0)
    {
      free_target(p);
      continue;
    }

    targets[j++] = p;
  }

  targets[j++] = 0;

  targets = redim(targets,struct format *,j);
}

static bool format_process_devices(void)
{
  int i = 0;
  int j = 0;
  int padding = 0;
  char text[256] = {0};
  const char *program = 0;
  char command[_POSIX_ARG_MAX] = {0};
  char path[PATH_MAX] = {0};

  for( ; targets[j] != 0 ; ++j )
    ;

  padding = get_number_padding(j);

  qsort(targets,j,sizeof(struct format *),compare_target);

  for( ; i < j ; ++i )
  {
    struct format *target = targets[i];

    if(strcmp(target->newfilesystem,"swap") == 0)
      strfcpy(text,sizeof(text),"(%*d/%d) - %-8s - %-8s",padding,i+1,j,target->devicepath,target->newfilesystem);
    else
      strfcpy(text,sizeof(text),"(%*d/%d) - %-8s - %-8s - %-8s",padding,i+1,j,target->devicepath,target->newfilesystem,target->mountpath);

    ui_dialog_progress(_("Formatting"),text,get_percent(i+1,j));

    if(target->format)
    {
      if(strcmp(target->newfilesystem,"ext2") == 0)
        program = "mkfs.ext2";
      else if(strcmp(target->newfilesystem,"ext3") == 0)
        program = "mkfs.ext3";
      else if(strcmp(target->newfilesystem,"ext4") == 0)
        program = "mkfs.ext4";
      else if(strcmp(target->newfilesystem,"xfs") == 0)
        program = "mkfs.xfs -f";
      else if(strcmp(target->newfilesystem,"btrfs") == 0)
        program = "mkfs.btrfs";
      else if(strcmp(target->newfilesystem,"swap") == 0)
        program = "mkswap";

      strfcpy(command,sizeof(command),"%s '%s'",program,shell_escape(target->devicepath));

      if(!execute(command,g->hostroot,0))
      {
        ui_dialog_progress(0,0,-1);
        return false;
      }
    }

    if(strcmp(target->newfilesystem,"swap") == 0)
    {
      strfcpy(command,sizeof(command),"swapon '%s'",shell_escape(target->devicepath));

      if(!execute(command,g->hostroot,0))
      {
        ui_dialog_progress(0,0,-1);
        return false;
      }
    }
    else
    {
      strfcpy(path,sizeof(path),"%s%s",g->guestroot,target->mountpath);

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

static void format_prepare_fstab(void)
{
  size_t i = 0;
  char text[TEXT_MAX] = {0};
  
  for( ; targets[i] != 0 ; ++i )
    ;
  
  g->fstabdata = alloc(char *,i + 1);

  g->fstabdata[i] = 0;

  do
  {
    --i;
    const char *devicepath = targets[i]->devicepath;
    const char *mountpath = targets[i]->mountpath;
    const char *newfilesystem = targets[i]->newfilesystem;
    strfcpy(text,sizeof(text),"%s:%s:%s",devicepath,(strcmp(newfilesystem,"swap") == 0) ? "/" : mountpath,newfilesystem);
    g->fstabdata[i] = strdup(text);
    eprintf("%s: %s\n",__func__,g->fstabdata[i]);
  } while(i > 0);
}

static bool format_run(void)
{
  if(!format_setup())
    return false;

  if(!ui_window_format(targets))
    return false;

  format_filter_devices();

  if(!format_process_devices())
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
