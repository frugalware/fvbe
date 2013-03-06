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

#pragma once

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <wchar.h>
#include <errno.h>
#include <limits.h>
#include <glob.h>
#include <regex.h>
#include <locale.h>
#include <time.h>
#include <libgen.h>
#include <blkid.h>
#include "text.h"

#ifdef UI_NEWT
#define NEWT_WIDTH 70
#define NEWT_HEIGHT 21
#endif

#define LOGFILE "/var/log/fwsetup.log"
#define INSTALL_ROOT "/mnt/install"
#define SPACE_CHARS " \t\r\n\v\f"
#define TEXT_MAX 8192
#define KIBIBYTE (1LL << 10LL)
#define MEBIBYTE (1LL << 20LL)
#define GIBIBYTE (1LL << 30LL)
#define TEBIBYTE (1LL << 40LL)
#define error(S) fprintf(logfile,"%s: %s\n",__func__,S)
#define eprintf(...) fprintf(logfile,__VA_ARGS__)

struct global
{
  char *kbdlayout;
  char *xkblayout;
  char *xkbmodel;
  char *xkbvariant;
  char *xkboptions;
  char **fstabdata;
};

struct device;

struct disk;

struct format
{
  char *devicepath;
  char *size;
  char *filesystem;
  bool format;
  char *newfilesystem;
  char *options;
  char *mountpath;
};

struct install
{
  char *name;
  bool checked;
};

struct account
{
  char *name;
  char *user;
  char *password;
  char *group;
  char *groups;
  char *home;
  char *shell;
};

struct module
{
  bool (*run) (void);
  void (*reset) (void);
  const char *name;
};

extern void fetch_real_devices(const char *base,char *s,size_t n);
extern bool areweinvc(void);
extern void file2str(const char *path,char *s,size_t n);
extern void strfcpy(char *s,size_t n,const char *fmt,...) __attribute__((format(printf,3,4)));
extern void strfcat(char *s,size_t n,const char *fmt,...) __attribute__((format(printf,3,4)));
extern bool mount_special(void);
extern void umount_all(void);
extern bool isrootpath(const char *path);
extern bool isasciistring(const char *s);
extern bool mkdir_recurse(const char *path);
extern bool size_to_string(char *s,size_t n,long long size,bool pad);
extern long long string_to_size(const char *s);
extern int get_text_length(const char *s);
extern bool execute(const char *command,const char *root,pid_t *cpid);
extern void *memdup(const void *mem,size_t size);
extern void *malloc0(size_t size);
extern int get_text_screen_width(const char *s);
extern bool get_text_screen_size(const char *text,int *width,int *height);
extern bool get_button_screen_size(const char *text,int *width,int *height);
extern bool get_label_screen_size(const char *text,int *width,int *height);
extern bool get_checkbox_screen_size(const char *text,int *width,int *height);
static inline long min(long a,long b) { return (a < b) ? a : b; }
static inline long max(long a,long b) { return (a > b) ? a : b; }
static inline long minv(long *v,size_t size)
{
  long i = 0;

  for( size_t n = 0 ; n < size ; ++n )
    i = min(i,v[n]);

  return i;
}
static inline long maxv(long *v,size_t size)
{
  long i = 0;

  for( size_t n = 0 ; n < size ; ++n )
    i = max(i,v[n]);

  return i;
}
static inline const char *strng(const char *s) { return (s == 0) ? "" : s; }
static inline bool areweinfwlive(void)
{
  const char *env = getenv("HOSTNAME");
  
  return (env != 0 && strcmp(env,"fwlive") == 0);
}
static inline bool areweinx11(void)
{
  const char *env = getenv("DISPLAY");
  
  return (env != 0 && strlen(env) > 0);
}
extern struct device **device_probe_all(bool disk,bool raid);
extern struct device *device_open(const char *path);
extern const char *device_get_path(struct device *device);
extern long long device_get_size(struct device *device);
extern const char *device_get_type(struct device *device);
extern void device_close(struct device *device);
extern struct disk *disk_open(struct device *device);
extern struct disk *disk_open_empty(struct device *device,const char *type);
extern const char *disk_get_type(struct disk *disk);
extern long long disk_get_free_size(struct disk *disk);
extern void disk_new_table(struct disk *disk,const char *type);
extern bool disk_has_extended_partition(struct disk *disk);
extern bool disk_can_store_bios_grub(struct disk *disk);
extern int disk_create_partition(struct disk *disk,long long size);
extern int disk_create_extended_partition(struct disk *disk);
extern int disk_create_logical_partition(struct disk *disk,long long size);
extern void disk_partition_set_purpose(struct disk *disk,int n,const char *purpose);
extern void disk_partition_set_active(struct disk *disk,int n,bool active);
extern void disk_partition_set_name(struct disk *disk,int n,const char *name);
extern int disk_partition_get_count(struct disk *disk);
extern const char *disk_partition_get_purpose(struct disk *disk,int n);
extern bool disk_partition_get_active(struct disk *disk,int n);
extern const char *disk_partition_get_name(struct disk *disk,int n);
extern int disk_partition_get_number(struct disk *disk,int n);
extern long long disk_partition_get_size(struct disk *disk,int n);
extern void disk_delete_partition(struct disk *disk);
extern bool disk_flush(struct disk *disk);
extern void disk_close(struct disk *disk);
extern int ui_main(int argc,char **argv);
extern void ui_dialog_text(const char *title,const char *text);
extern bool ui_dialog_yesno(const char *title,const char *text,bool defaultno);
extern bool ui_dialog_progress(const char *title,const char *text,int percent);
extern bool ui_window_list(const char *title,const char *text,char **entries,char **entry);
extern void ui_window_text(const char *title,const char *text);
extern bool ui_window_partition(struct device **devices,struct disk **disks);
extern bool ui_window_format(struct format **targets);
extern bool ui_window_root(struct account *data);
extern bool ui_window_user(struct account *data);
extern bool ui_window_time(char **data,char **zone,bool *utc);
extern bool ui_window_install(struct install *groups);
extern FILE *logfile;
extern int main(int argc,char **argv);

extern struct global *g;
extern struct module locale_module;
extern struct module layout_module;
extern struct module greeter_module;
extern struct module partition_module;
extern struct module format_module;
extern struct module install_module;
extern struct module postconfig_module;
extern struct module finale_module;
extern struct module *modules[];
extern unsigned int seed;
