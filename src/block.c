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

#include <linux/major.h>
#include "local.h"

#define EMPTY_PARTITION &(struct partition) {0}
#define GPT_BOOT_FLAG (1ULL << 2ULL)

#define DOS_DATA      0x83
#define DOS_SWAP      0x82
#define DOS_RAID      0xfd
#define DOS_LVM       0x8e
#define DOS_EFI       0xef
#define DOS_EXTENDED  0x05
#define DOS_EXTENDED2 0x0f
#define DOS_EXTENDED3 0x85
#define GPT_DATA      "0fc63daf-8483-4772-8e79-3d69d8477de4"
#define GPT_SWAP      "0657fd6d-a4ab-43c4-84e5-0933c84b4f4f"
#define GPT_RAID      "a19d880f-05fc-4d3b-a006-743f0f84911e"
#define GPT_LVM       "e6d6d379-f507-44c2-a23c-238f2a3df928"
#define GPT_EFI       "c12a7328-f81f-11d2-ba4b-00a0c93ec93b"
#define GPT_BIOS      "21686148-6449-6e6f-744e-656564454649"

enum devicetype
{
  DEVICETYPE_FILE,
  DEVICETYPE_DISK,
  DEVICETYPE_RAID,
  DEVICETYPE_UNKNOWN
};

struct device
{
  char *path;
  long long size;
  long long sectorsize;
  long long alignment;
  long long sectors;
  enum devicetype type;
};

enum disktype
{
  DISKTYPE_DOS,
  DISKTYPE_GPT
};

struct partition
{
  struct disk *disk;
  int number;
  long long start;
  long long size;
  long long end;
  unsigned char dostype;
  bool dosactive;
  char gptname[37];
  char gptuuid[37];
  char gpttype[37];
  unsigned long long gptflags;
};

struct disk
{
  struct device *device;
  enum disktype type;
  long long sectors;
  bool modified;
  unsigned int dosuuid;
  unsigned char dosextended;
  char gptuuid[37];
  struct partition table[128];
  int size;
};

struct raid
{
  struct device *device;
  int level;
  int disks;
  struct device *devices[128];
  long long size;
  char *path;
  char *origin;
};

static inline bool isdisk(const struct stat *st)
{
  switch(major(st->st_rdev))
  {
    // IDE disks
    case IDE0_MAJOR:
    case IDE1_MAJOR:
    case IDE2_MAJOR:
    case IDE3_MAJOR:
    case IDE4_MAJOR:
    case IDE5_MAJOR:
    case IDE6_MAJOR:
    case IDE7_MAJOR:
    case IDE8_MAJOR:
    case IDE9_MAJOR:
      return true;

    // SCSI disks
    case SCSI_DISK0_MAJOR:
    case SCSI_DISK1_MAJOR:
    case SCSI_DISK2_MAJOR:
    case SCSI_DISK3_MAJOR:
    case SCSI_DISK4_MAJOR:
    case SCSI_DISK5_MAJOR:
    case SCSI_DISK6_MAJOR:
    case SCSI_DISK7_MAJOR:
    case SCSI_DISK8_MAJOR:
    case SCSI_DISK9_MAJOR:
    case SCSI_DISK10_MAJOR:
    case SCSI_DISK11_MAJOR:
    case SCSI_DISK12_MAJOR:
    case SCSI_DISK13_MAJOR:
    case SCSI_DISK14_MAJOR:
    case SCSI_DISK15_MAJOR:
      return true;

    // virtio disks
    case 253:
      return true;

    default:
      return false;
  }
}

static inline bool israid(const struct stat *st)
{
  return (major(st->st_rdev) == MD_MAJOR);
}

// TODO: replace this function with something better. only works on little endian cpus.
static bool putdosuuid(struct disk *disk)
{
  int fd = -1;
  unsigned int n = (disk->dosuuid == 0) ? (unsigned int) rand_r(&g->seed) : disk->dosuuid;
  
  if(
    (fd = open(disk->device->path,O_WRONLY)) == -1 ||
    lseek(fd,440,SEEK_SET) == (off_t) -1           ||
    write(fd,&n,sizeof(n)) != sizeof(n)            ||
    close(fd) == -1
  )
  {
    error(strerror(errno));
    if(fd != -1)
      close(fd);
    return false;
  }
  
  return true;
}

static bool getuuid(struct disk *disk)
{
  char command[_POSIX_ARG_MAX] = {0};
  FILE *pipe = 0;
  char line[LINE_MAX] = {0};
  bool outputsuccess = false;
  bool exitsuccess = false;
  char *p = 0;

  if(disk->type == DISKTYPE_DOS)
    strfcpy(command,sizeof(command),"export LC_ALL=C;yes | fdisk -l '%s' 2> /dev/null | sed -rn 's|^Disk identifier: 0x([0-9a-fA-F]+)$|\\1|p'",disk->device->path);
  else if(disk->type == DISKTYPE_GPT)
    strfcpy(command,sizeof(command),"export LC_ALL=C;yes | gdisk -l '%s' 2> /dev/null | sed -rn 's|^Disk identifier \\(GUID\\): ([0-9a-zA-Z-]+)$|\\1|p'",disk->device->path);
  else
    return false;

  if((pipe = popen(command,"r")) == 0)
  {
    error(strerror(errno));
    return false;
  }

  outputsuccess = (fgets(line,LINE_MAX,pipe) != 0);

  exitsuccess = (pclose(pipe) != -1);

  if((p = strchr(line,'\n')) != 0)
    *p = 0;

  if(outputsuccess && exitsuccess)
  {
    if(disk->type == DISKTYPE_DOS)
      disk->dosuuid = strtoul(line,0,16);
    else if(disk->type == DISKTYPE_GPT)
      strfcpy(disk->gptuuid,sizeof(disk->gptuuid),"%s",line);
    return true;
  }
  else
  {
    error(strerror(errno));
    return false;
  }
}

static bool zapdisk(const char *path)
{
  char command[_POSIX_ARG_MAX] = {0};

  strfcpy(command,sizeof(command),"sgdisk --zap-all '%s'",shell_escape(path));

  return execute(command,g->hostroot,0);
}

static inline long long alignsector(const struct device *device,long long sector)
{
  long long alignment = device->alignment;

  if((sector % alignment) == 0)
    return sector;

  return sector + (alignment - (sector % alignment));
}

static inline void getsectors(struct disk *disk)
{
  long long sectorsize = disk->device->sectorsize;
  long long sectors = disk->device->sectors;

  if(disk->type == DISKTYPE_GPT)
    sectors -= 1 + ((128 * 128) / sectorsize);

  disk->sectors = sectors;
}

static bool newpartition(struct disk *disk,long long size,struct partition *part)
{
  struct partition *last = 0;

  if(disk->size == 0)
  {
    part->number = 1;
    part->start = disk->device->alignment;
  }
  else
  {
    last = &disk->table[disk->size-1];
    part->number = last->number + 1;
    part->start = last->end + 1;
  }

  part->size = size / disk->device->sectorsize;

  part->end = part->start + part->size - 1;

  part->start = alignsector(disk->device,part->start);

  part->end = alignsector(disk->device,part->end) - 1;

  if(part->end >= disk->sectors)
    part->end = disk->sectors - 1;

  part->size = (part->end - part->start) + 1;

  return true;
}

static void getraidsize(struct raid *raid)
{
  int level = raid->level;
  int disks = raid->disks;
  struct device **devices = raid->devices;
  long long size = 0;

  if(level == 0)
  {
    for( int i = 0 ; i < disks ; ++i )
      size += devices[i]->size;
  }
  else if(level == 1 || level == 4 || level == 5 || level == 6 || level == 10)
  {
    long long minsize = LLONG_MAX;
    
    for( int i = 0 ; i < disks ; ++i )
      minsize = min(minsize,devices[i]->size);
    
    if(level == 1)
    {
      size = minsize;
    }
    else if(level == 4 || level == 5)
    {
      size = minsize * (disks - 1);
    }
    else if(level == 6)
    {
      size = minsize * (disks - 2);
    }
    else if(level == 10)
    {
      size = minsize * (disks / 2);
    }
  }
  
  raid->size = size;
}

static int device_compare(const void *A,const void *B)
{
  struct device *a = (struct device *) A;
  struct device *b = *(struct device **) B; 

  return strcmp(a->path,b->path);
}

extern struct device **device_probe_all(bool disk,bool raid)
{
  DIR *dir = 0;
  size_t i = 0;
  size_t size = 4096;
  struct device **devices = 0;
  struct dirent entry = {0};
  struct dirent *p = 0;
  char path[PATH_MAX] = {0};

  if(disk == false && raid == false)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  if((dir = opendir("/sys/class/block")) == 0)
  {
    error(strerror(errno));
    goto bail;
  }

  devices = alloc(struct device *,size);

  while(readdir_r(dir,&entry,&p) == 0 && p != 0)
  {
    const char *name = p->d_name;
    struct device *device = 0;
    
    if(
      i == size - 1          ||
      strcmp(name,".") == 0  ||
      strcmp(name,"..") == 0
    )
      continue;
    
    if(disk && is_disk_device(name))
      strfcpy(path,sizeof(path),"/dev/%s",name);
    else if(raid && is_raid_device(name))
      strfcpy(path,sizeof(path),"/dev/%s",name);
    else
      continue;
    
    if(strncmp(path,strng(g->isodevice),8) == 0 || (device = device_open(path)) == 0)
      continue;
    
    devices[i++] = device;
  }

  if(i == 0)
  {
    error("no devices found");
    free(devices);
    devices = 0;
    goto bail;
  }

  devices[i] = 0;

  devices = redim(devices,struct device *,i + 1);

bail:

  if(dir != 0)
    closedir(dir);

  return devices;
}

extern struct device *device_open(const char *path)
{
  int fd = -1;
  struct stat st = {0};
  long long size = 0;
  long long sectorsize = 0;
  long long alignment = 0;
  long long sectors = 0;
  enum devicetype type = DEVICETYPE_UNKNOWN;
  struct device *device = 0;

  if(path == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    goto bail;
  }

  if((fd = open(path,O_RDONLY)) == -1 || fstat(fd,&st) == -1)
  {
    error(strerror(errno));
    goto bail;
  }

  if(S_ISBLK(st.st_mode))
  {
    if(ioctl(fd,BLKGETSIZE64,&size) == -1 || ioctl(fd,BLKSSZGET,&sectorsize) == -1)
    {
      error(strerror(errno));
      goto bail;
    }
  }
  else if(S_ISREG(st.st_mode))
  {
    size = st.st_size;
    sectorsize = 512;
  }
  else
  {
    errno = EINVAL;
    error(strerror(errno));
    goto bail;
  }

  alignment = MEBIBYTE / sectorsize;

  sectors = size / sectorsize;

  if(size <= 0 || sectorsize <= 0 || (MEBIBYTE % sectorsize) != 0 || (size % sectorsize) != 0)
  {
    errno = ERANGE;
    error(strerror(errno));
    goto bail;
  }

  if(S_ISREG(st.st_mode))
    type = DEVICETYPE_FILE;
  else if(isdisk(&st))
    type = DEVICETYPE_DISK;
  else if(israid(&st))
    type = DEVICETYPE_RAID;
  else
    type = DEVICETYPE_UNKNOWN;

  device = alloc(struct device,1);

  device->path = strdup(path);

  device->size = size;

  device->sectorsize = sectorsize;

  device->alignment = alignment;

  device->sectors = sectors;

  device->type = type;

bail:

  if(fd != -1)
    close(fd);

  return device;
}

extern const char *device_get_path(struct device *device)
{
  if(device == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  return device->path;
}

extern long long device_get_size(struct device *device)
{
  if(device == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  return device->size;
}

extern const char *device_get_type(struct device *device)
{
  if(device == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  if(device->type == DEVICETYPE_FILE)
    return "file";
  else if(device->type == DEVICETYPE_DISK)
    return "disk";
  else if(device->type == DEVICETYPE_RAID)
    return "raid";
  else if(device->type == DEVICETYPE_UNKNOWN)
    return "unknown";
  else
    return 0;
}

extern void device_close(struct device *device)
{
  if(device == 0)
    return;

  free(device->path);

  free(device);
}

extern struct disk *disk_open(struct device *device)
{
  blkid_probe probe = 0;
  blkid_partlist partlist = 0;
  blkid_parttable parttable = 0;
  const char *label = 0;
  struct disk disk = {0};
  int i = 0;
  int j = 0;
  blkid_partition partition = 0;
  struct disk *result = 0;

  if(device == 0 || device->type != DEVICETYPE_DISK)
  {
    errno = EINVAL;
    error(strerror(errno));
    goto bail;
  }

  if((probe = blkid_new_probe_from_filename(device->path)) == 0)
  {
    error(strerror(errno));
    goto bail;
  }

  if(
    (partlist = blkid_probe_get_partitions(probe))   == 0 ||
    (parttable = blkid_partlist_get_table(partlist)) == 0 ||
    (label = blkid_parttable_get_type(parttable))    == 0
  )
  {
    error("no partition table");
    goto bail;
  }

  disk.device = device;

  if(strcmp(label,"dos") == 0)
    disk.type = DISKTYPE_DOS;
  else if(strcmp(label,"gpt") == 0)
    disk.type = DISKTYPE_GPT;
  else
  {
    error("unknown partition table");
    goto bail;
  }

  getsectors(&disk);

  if(disk.type == DISKTYPE_DOS)
    disk.dosextended = DOS_EXTENDED;

  if(!getuuid(&disk))
    goto bail;

  if((j = blkid_partlist_numof_partitions(partlist)) > 0)
  {
    if((disk.type == DISKTYPE_DOS && j > 60) || (disk.type == DISKTYPE_GPT && j > 128))
    {
      error("partition table too big");
      goto bail;
    }

    for( ; i < j && (partition = blkid_partlist_get_partition(partlist,i)) != 0 ; ++i )
    {
      struct partition *part = &disk.table[i];

      part->number = blkid_partition_get_partno(partition);

      part->start = blkid_partition_get_start(partition);

      part->size = blkid_partition_get_size(partition);

      part->end = part->start + part->size - 1;

      if(disk.type == DISKTYPE_DOS)
      {
        part->dostype = blkid_partition_get_type(partition);

        part->dosactive = (blkid_partition_get_flags(partition) == 0x80);

        if(part->dostype == DOS_EXTENDED2 || part->dostype == DOS_EXTENDED3)
          disk.dosextended = part->dostype;

        continue;
      }

      if(disk.type == DISKTYPE_GPT)
      {
        if(blkid_partition_get_name(partition) != 0)
        {
          strfcpy(part->gptname,sizeof(part->gptname),"%s",blkid_partition_get_name(partition));
          
          if(!is_partition_name(part->gptname))
            part->gptname[0] = 0;
        }

        strfcpy(part->gptuuid,sizeof(part->gptuuid),"%s",blkid_partition_get_uuid(partition));

        strfcpy(part->gpttype,sizeof(part->gpttype),"%s",blkid_partition_get_type_string(partition));

        part->gptflags = blkid_partition_get_flags(partition);

        continue;
      }
    }

    disk.size = j;
  }

  result = memdup(&disk,sizeof(struct disk));

  for( i = 0 ; i < j ; ++i )
    result->table[i].disk = result;

bail:

  if(probe != 0)
    blkid_free_probe(probe);

  return result;
}

extern struct disk *disk_open_empty(struct device *device,const char *type)
{
  struct disk disk = {0};

  if(device == 0 || type == 0 || (strcmp(type,"dos") != 0 && strcmp(type,"gpt") != 0))
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  disk.device = device;

  disk_new_table(&disk,type);

  return memdup(&disk,sizeof(struct disk));
}

extern const char *disk_get_type(struct disk *disk)
{
  const char *type = 0;

  if(disk == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  if(disk->type == DISKTYPE_DOS)
    type = "dos";
  else if(disk->type == DISKTYPE_GPT)
    type = "gpt";
  else
    type = "unknown";

  return type;
}

extern long long disk_get_free_size(struct disk *disk)
{
  long long size = 0;

  if(disk == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  size = disk->sectors;

  if(disk->size > 0)
  {
    struct partition *last = &disk->table[disk->size-1];

    if(disk->type == DISKTYPE_DOS && last->dostype == disk->dosextended)
      size -= last->start + disk->device->alignment;
    else if(disk->type == DISKTYPE_DOS && last->number > 4)
      size -= last->end + 1 + disk->device->alignment;
    else
      size -= last->end + 1;
  }
  else
  {
    size -= disk->device->alignment;
  }

  size *= disk->device->sectorsize;

  return size;
}

extern void disk_new_table(struct disk *disk,const char *type)
{
  struct device *device = 0;
  enum disktype disktype = 0;

  if(disk == 0 || type == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  device = disk->device;

  if(strcmp(type,"dos") == 0)
    disktype = DISKTYPE_DOS;
  else if(strcmp(type,"gpt") == 0)
    disktype = DISKTYPE_GPT;
  else
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  memset(disk,0,sizeof(struct disk));

  disk->device = device;

  disk->type = disktype;

  getsectors(disk);

  if(disk->type == DISKTYPE_DOS)
    disk->dosextended = DOS_EXTENDED;

  disk->modified = true;
}

extern bool disk_has_extended_partition(struct disk *disk)
{
  int i = 0;

  if(disk == 0 || disk->size < 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(disk->type != DISKTYPE_DOS)
    return false;

  for( ; i < disk->size ; ++i )
  {
    if(disk->table[i].dostype == disk->dosextended)
      return true;
  }

  return false;
}

extern bool disk_can_store_bios_grub(struct disk *disk)
{
  long long size = 0;
  int i = 0;
  struct partition *part = 0;

  if(disk == 0 || disk->size < 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(disk->size == 0)
    return false;

  if(disk->type == DISKTYPE_DOS)
    size = disk->table[0].start;
  else if(disk->type == DISKTYPE_GPT)
    for( ; i < disk->size ; ++i )
    {
      part = &disk->table[i];
      
      if(strcmp(part->gpttype,GPT_BIOS) == 0)
      {
        size = part->size;
        break;
      }
    }
  else
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  size *= disk->device->sectorsize;
  
  return (size >= MEBIBYTE);
}

extern int disk_create_partition(struct disk *disk,long long size)
{
  struct partition part = {0};

  if(disk == 0 || disk->size < 0 || size <= 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  if(size < MEBIBYTE)
    size = MEBIBYTE;

  if(!newpartition(disk,size,&part))
    return -1;

  if(
    (disk->type == DISKTYPE_DOS && part.number > 4)   ||
    (disk->type == DISKTYPE_GPT && part.number > 128) ||
    disk_has_extended_partition(disk)
  )
  {
    errno = ERANGE;
    error(strerror(errno));
    return -1;
  }

  if(disk->type == DISKTYPE_DOS)
    part.dostype = DOS_DATA;
  else if(disk->type == DISKTYPE_GPT)
    strfcpy(part.gpttype,sizeof(part.gpttype),"%s",GPT_DATA);

  memcpy(&disk->table[disk->size++],&part,sizeof(struct partition));

  disk->modified = true;

  return disk->size - 1;
}

extern int disk_create_extended_partition(struct disk *disk)
{
  struct partition part = {0};

  if(disk == 0 || disk->size < 0 || disk->type != DISKTYPE_DOS)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  if(disk_has_extended_partition(disk))
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  if(!newpartition(disk,disk_get_free_size(disk),&part) || part.number > 4)
    return -1;

  part.dostype = disk->dosextended;

  memcpy(&disk->table[disk->size++],&part,sizeof(struct partition));

  disk->modified = true;

  return disk->size - 1;
}

extern int disk_create_logical_partition(struct disk *disk,long long size)
{
  int i = 0;
  struct partition *ext = 0;
  struct partition *last = 0;
  struct partition part = {0};

  if(disk == 0 || disk->size < 0 || disk->type != DISKTYPE_DOS || size <= 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  if(size < MEBIBYTE)
    size = MEBIBYTE;

  for( ; i < disk->size ; ++i )
  {
    struct partition *part = &disk->table[i];

    if(part->dostype == disk->dosextended && ext == 0)
      ext = part;

    last = part;
  }

  if(ext == 0 || ext->number > 4)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  part.number = (ext == last) ? 5 : last->number + 1;

  part.start = ((ext == last) ? last->start : last->end + 1) + disk->device->alignment;

  part.size = size / disk->device->sectorsize;

  part.end = part.start + part.size - 1;

  part.start = alignsector(disk->device,part.start);

  part.end = alignsector(disk->device,part.end) - 1;

  if(part.end >= disk->sectors)
    part.end = disk->sectors - 1;

  part.size = (part.end - part.start) + 1;

  if(part.number > 60)
  {
    errno = ERANGE;
    error(strerror(errno));
    return -1;
  }

  part.dostype = DOS_DATA;

  memcpy(&disk->table[disk->size++],&part,sizeof(struct partition));

  disk->modified = true;

  return disk->size - 1;
}

extern void disk_delete_partition(struct disk *disk)
{
  struct partition *last = 0;

  if(disk == 0 || disk->size <= 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  last = &disk->table[--disk->size];

  memset(last,0,sizeof(struct partition));

  disk->modified = true;
}

extern void disk_partition_set_purpose(struct disk *disk,int n,const char *purpose)
{
  struct partition *part = 0;

  if(disk == 0 || n < 0 || n >= disk->size || purpose == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  part = &disk->table[n];

  if(disk->type == DISKTYPE_DOS)
  {
    if(strcmp(purpose,"data") == 0)
      part->dostype = DOS_DATA;
    else if(strcmp(purpose,"swap") == 0)
      part->dostype = DOS_SWAP;
    else if(strcmp(purpose,"raid") == 0)
      part->dostype = DOS_RAID;
    else if(strcmp(purpose,"lvm") == 0)
      part->dostype = DOS_LVM;
    else if(strcmp(purpose,"efi") == 0)
      part->dostype = DOS_EFI;
    else if(strcmp(purpose,"extended") == 0)
      part->dostype = disk->dosextended;
  }
  else if(disk->type == DISKTYPE_GPT)
  {
    if(strcmp(purpose,"data") == 0)
      strfcpy(part->gpttype,sizeof(part->gpttype),"%s",GPT_DATA);
    else if(strcmp(purpose,"swap") == 0)
      strfcpy(part->gpttype,sizeof(part->gpttype),"%s",GPT_SWAP);
    else if(strcmp(purpose,"raid") == 0)
      strfcpy(part->gpttype,sizeof(part->gpttype),"%s",GPT_RAID);
    else if(strcmp(purpose,"lvm") == 0)
      strfcpy(part->gpttype,sizeof(part->gpttype),"%s",GPT_LVM);
    else if(strcmp(purpose,"efi") == 0)
      strfcpy(part->gpttype,sizeof(part->gpttype),"%s",GPT_EFI);
    else if(strcmp(purpose,"bios") == 0)
      strfcpy(part->gpttype,sizeof(part->gpttype),"%s",GPT_BIOS);
  }

  disk->modified = true;
}

extern void disk_partition_set_active(struct disk *disk,int n,bool active)
{
  struct partition *part = 0;

  if(disk == 0 || n < 0 || n >= disk->size)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  part = &disk->table[n];

  if(disk->type == DISKTYPE_DOS)
    part->dosactive = active;
  else if(disk->type == DISKTYPE_GPT)
  {
    if(active)
      part->gptflags |= GPT_BOOT_FLAG;
    else if((part->gptflags & GPT_BOOT_FLAG) != 0)
      part->gptflags ^= GPT_BOOT_FLAG;
  }

  disk->modified = true;
}

extern void disk_partition_set_name(struct disk *disk,int n,const char *name)
{
  struct partition *part = 0;

  if(disk == 0 || n < 0 || n >= disk->size || disk->type != DISKTYPE_GPT || name == 0 || strlen(name) > 36)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  part = &disk->table[n];

  strfcpy(part->gptname,sizeof(part->gptname),"%s",name);

  disk->modified = true;
}

extern int disk_partition_get_count(struct disk *disk)
{
  return disk->size;
}

extern const char *disk_partition_get_purpose(struct disk *disk,int n)
{
  struct partition *part = 0;
  const char *purpose = "unknown";

  if(disk == 0 || n < 0 || n >= disk->size)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  part = &disk->table[n];

  if(disk->type == DISKTYPE_DOS)
  {
    if(part->dostype == DOS_DATA)
      purpose = "data";
    else if(part->dostype == DOS_SWAP)
      purpose = "swap";
    else if(part->dostype == DOS_RAID)
      purpose = "raid";
    else if(part->dostype == DOS_LVM)
      purpose = "lvm";
    else if(part->dostype == DOS_EFI)
      purpose = "efi";
    else if(part->dostype == disk->dosextended)
      purpose = "extended";
  }
  else if(disk->type == DISKTYPE_GPT)
  {
     if(strcmp(part->gpttype,GPT_DATA) == 0)
       purpose = "data";
     else if(strcmp(part->gpttype,GPT_SWAP) == 0)
       purpose = "swap";
     else if(strcmp(part->gpttype,GPT_RAID) == 0)
       purpose = "raid";
     else if(strcmp(part->gpttype,GPT_LVM) == 0)
       purpose = "lvm";
     else if(strcmp(part->gpttype,GPT_EFI) == 0)
       purpose = "efi";
     else if(strcmp(part->gpttype,GPT_BIOS) == 0)
       purpose = "bios";
  }

  return purpose;
}

extern bool disk_partition_get_active(struct disk *disk,int n)
{
  struct partition *part = 0;
  bool active = false;

  if(disk == 0 || n < 0 || n >= disk->size)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  part = &disk->table[n];

  if(disk->type == DISKTYPE_DOS)
    active = part->dosactive;
  else if(disk->type == DISKTYPE_GPT)
    active = (part->gptflags & GPT_BOOT_FLAG) != 0;

  return active;
}

extern const char *disk_partition_get_name(struct disk *disk,int n)
{
  struct partition *part = 0;

  if(disk == 0 || n < 0 || n >= disk->size || disk->type != DISKTYPE_GPT)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  part = &disk->table[n];

  return part->gptname;
}

extern int disk_partition_get_number(struct disk *disk,int n)
{
  struct partition *part = 0;

  if(disk == 0 || n < 0 || n >= disk->size)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  part = &disk->table[n];

  return part->number;
}

extern long long disk_partition_get_size(struct disk *disk,int n)
{
  struct partition *part = 0;

  if(disk == 0 || n < 0 || n >= disk->size)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  part = &disk->table[n];

  return part->size * disk->device->sectorsize;
}

extern bool disk_flush(struct disk *disk)
{
  char command[_POSIX_ARG_MAX] = {0};
  int i = 0;
  struct partition *part = 0;
  int j = 0;

  if(disk == 0 || (disk->type != DISKTYPE_DOS && disk->type != DISKTYPE_GPT))
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!disk->modified)
    return true;

  if(disk->type == DISKTYPE_DOS)
  {
    strfcpy(command,sizeof(command),"set -e;echo -n -e '");

    for( ; i < disk->size ; ++i )
    {
      part = &disk->table[i];
      
      strfcat(command,sizeof(command),"%lld %lld 0x%hhx %c\\n",
        part->start,
        part->size,
        part->dostype,
        (part->dosactive) ? '*' : '-'
      );
      
      if(part->dostype == disk->dosextended)
        for( j = part->number ; j < 4 ; ++j )
          strfcat(command,sizeof(command),"0 0 0x00 -\\n");
    }

    if(disk->size == 0)
      strfcat(command,sizeof(command),
        "0 0 0x00 -\\n"
        "0 0 0x00 -\\n"
        "0 0 0x00 -\\n"
        "0 0 0x00 -\\n"
      );

    strfcat(command,sizeof(command),"' | sfdisk --unit S --Linux --no-reread '%s';",
      shell_escape(disk->device->path)
    );
  }
  else if(disk->type == DISKTYPE_GPT)
  {
    strfcpy(command,sizeof(command),"set -e;sgdisk --clear --disk-guid='%s'",
      (strlen(disk->gptuuid) == 0) ? "R" : shell_escape(disk->gptuuid)
    );

    for( ; i < disk->size ; ++i )
    {
      part = &disk->table[i];

      strfcat(command,sizeof(command)," --new='%d:%lld:%lld' --attributes='%d:=:0x%.16llx'",
        part->number,
        part->start,
        part->end,
        part->number,
        part->gptflags
      );
      
      strfcat(command,sizeof(command)," --change-name='%d:%s'",part->number,shell_escape(part->gptname));
      
      strfcat(command,sizeof(command)," --partition-guid='%d:%s'",part->number,(strlen(part->gptuuid) == 0) ? "R" : shell_escape(part->gptuuid));
    
      strfcat(command,sizeof(command)," --typecode='%d:%s'",part->number,shell_escape(part->gpttype)); 
    }

    strfcat(command,sizeof(command)," '%s';",
      shell_escape(disk->device->path)
    );
  }

  if(!zapdisk(disk->device->path))
    return false;

  if(!execute(command,g->hostroot,0))
    return false;

  if(disk->type == DISKTYPE_DOS && !putdosuuid(disk))
    return false;

  return true;
}

extern void disk_close(struct disk *disk,bool closedevice)
{
  if(disk == 0)
    return;

  if(closedevice)
    device_close(disk->device);

  free(disk);
}

extern struct raid *raid_open(struct device *device)
{
  char base[PATH_MAX] = {0};
  char path[PATH_MAX] = {0};
  char buf[LINE_MAX] = {0};
  int level = 0;
  int disks = 0;
  struct device *devices[128] = {0};
  struct raid *raid = 0;

  if(device == 0 || device->type != DEVICETYPE_RAID)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  strfcpy(base,sizeof(base),"%s",device->path);

  strfcpy(path,sizeof(path),"/sys/class/block/%s/md/level",basename(base));

  file2str(path,buf,sizeof(buf));

  if(strlen(buf) == 0)
  {
    error("failed to retrieve raid level");
    return 0;
  }

  if(sscanf(buf,"raid%d",&level) != 1 || raidmindisks(level) == -1)
  {
    error("invalid raid level");
    return 0;
  }

  strfcpy(path,sizeof(path),"/sys/class/block/%s/md/raid_disks",basename(base));

  file2str(path,buf,sizeof(buf));

  if(strlen(buf) == 0)
  {
    error("failed to retrieve raid disks");
    return 0;
  }

  if(sscanf(buf,"%d",&disks) != 1 || disks < raidmindisks(level) || disks > 128)
  {
    error("invalid raid disks");
    return 0;
  }

  for( int i = 0 ; i < disks ; ++i )
  {
    strfcpy(path,sizeof(path),"/sys/class/block/%s/md/rd%d",basename(base),i);
    
    if(!readlink0(path,buf,sizeof(buf)))
    {
      for( int j = 0 ; j < i ; ++j )
        device_close(devices[j]);
 
      return 0;
    }
    
    if(strncmp(buf,"dev-",4) != 0)
    {
      error("invalid raid slave name");

      for( int j = 0 ; j < i ; ++j )
        device_close(devices[j]);
 
      return 0;
    }
    
    strfcpy(path,sizeof(path),"/dev/%s",buf+4);
    
    if((devices[i] = device_open(path)) == 0)
    {
      error("unable to open raid slave");
 
      for( int j = 0 ; j < i ; ++j )
        device_close(devices[j]);
 
      return 0;
    }
  }

  raid = alloc(struct raid,1);

  raid->device = device;
  
  raid->level = level;

  raid->disks = disks;

  memcpy(raid->devices,devices,sizeof(devices));

  getraidsize(raid);

  raid->path = strdup(device->path);

  raid->origin = strdup("device");

  return raid;
}

extern struct raid *raid_open_empty(const char *path,int level,int disks,struct device **devices)
{
  struct raid *raid = 0;

  if(path == 0 || raidmindisks(level) == -1 || disks < raidmindisks(level) || disks > 128 || devices == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  raid = alloc(struct raid,1);
  
  raid->level = level;
  
  raid->disks = disks;
  
  memcpy(raid->devices,devices,sizeof(struct device *) * disks);
  
  getraidsize(raid);

  raid->path = strdup(path);

  raid->origin = strdup("memory");

  return raid;
}

extern int raid_get_level(struct raid *raid)
{
  if(raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  return raid->level;
}

extern int raid_get_count(struct raid *raid)
{
  if(raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }
  
  return raid->disks;
}

extern struct device *raid_get_device(struct raid *raid,int n)
{
  if(raid == 0 || n >= raid->disks)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }
  
  return raid->devices[n];
}

extern long long raid_get_size(struct raid *raid)
{
  if(raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  return raid->size;
}

extern const char *raid_get_path(struct raid *raid)
{
  if(raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  return raid->path;
}

extern const char *raid_get_origin(struct raid *raid)
{
  if(raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  return raid->origin;
}

extern bool raid_has_device(struct raid *raid,struct device *device)
{
  size_t disks = 0;

  if(raid == 0 || raid->disks < 1 || device == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  disks = raid->disks;
  
  return (lfind(device,raid->devices,&disks,sizeof(struct device *),device_compare) != 0);
}

extern bool raid_start(struct raid *raid)
{
  char command[_POSIX_ARG_MAX] = {0};

  if(raid == 0 || raid->device != 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  strfcpy(command,sizeof(command),"yes 'y' | mdadm --create --level='%d' --raid-devices='%d' '%s'",raid->level,raid->disks,shell_escape(raid->path));

  for( int i = 0 ; i < raid->disks ; ++i )
    strfcat(command,sizeof(command)," '%s'",shell_escape(raid->devices[i]->path));

  if(!execute(command,g->hostroot,0))
    return false;

  raid->device = device_open(raid->path);

  return true;
}

extern bool raid_stop(struct raid *raid)
{
  char command[_POSIX_ARG_MAX] = {0};

  if(raid == 0 || raid->device == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  strfcpy(command,sizeof(command),"mdadm --stop '%s'",shell_escape(raid->path));

  if(!execute(command,g->hostroot,0))
    return false;

  device_close(raid->device);

  raid->device = 0;

  return true;
}

extern void raid_close(struct raid *raid,bool closedevices,bool closedevice)
{
  if(raid == 0)
    return;
  
  if(closedevices)
    for( int i = 0 ; i < raid->disks ; ++i )
      device_close(raid->devices[i]);
  
  if(closedevice)
    device_close(raid->device);
  
  free(raid->path);
  
  free(raid->origin);
  
  free(raid);
}
