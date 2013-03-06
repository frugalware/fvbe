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

static inline bool umount_retry(const char *path)
{
  size_t i = 0;
  
  for( ; i < 5 ; ++i )
    if(umount2(path,UMOUNT_NOFOLLOW) == -1 && errno != ENOENT)
      sleep(1);
    else
      return true;
  
  return false;  
}

extern void fetch_real_devices(const char *base,char *s,size_t n)
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

extern bool areweinvc(void)
{
  char tty[TEXT_MAX] = {0};
  regex_t re = {0};
  bool result = true;

  if(
    ttyname_r(STDIN_FILENO,tty,sizeof(tty)) != 0                ||
    regcomp(&re,"^/dev/tty[0-9]+$",REG_EXTENDED|REG_NOSUB) != 0 ||
    regexec(&re,tty,0,0,0) != 0
  )
    result = false;

  regfree(&re);
  
  return result;
}

extern void file2str(const char *path,char *s,size_t n)
{
  FILE *file = 0;

  if(path == 0 || s == 0 || n == 0)
  {
    error(strerror(errno));
    return;
  }
  
  *s = 0;
  
  if((file = fopen(path,"rb")) == 0)
  {
    error(strerror(errno));
    return;
  }
  
  fgets(s,n,file);
  
  fclose(file);
  
  if((s = strchr(s,'\n')) != 0)
    *s = 0;
}

extern void strfcpy(char *s,size_t n,const char *fmt,...)
{
  va_list args;

  if(s == 0 || n == 0 || fmt == 0)
  {
    error(strerror(errno));
    return;
  }
  
  va_start(args,fmt);
  
  vsnprintf(s,n,fmt,args);
  
  va_end(args);
}

extern void strfcat(char *s,size_t n,const char *fmt,...)
{
  va_list args;
  size_t off = 0;
  
  if(s == 0 || n == 0 || fmt == 0)
  {
    error(strerror(errno));
    return;
  }
  
  va_start(args,fmt);

  off = strlen(s);
  
  vsnprintf(s+off,n-off,fmt,args);
  
  va_end(args);
}

extern bool mount_special(void)
{
  if(mount("none",INSTALL_ROOT "/dev","devtmpfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  if(mount("none",INSTALL_ROOT "/proc","proc",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  if(mount("none",INSTALL_ROOT "/sys","sysfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  if(mount("none",INSTALL_ROOT "/tmp","tmpfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }
  
  if(mount("none",INSTALL_ROOT "/var/tmp","tmpfs",0,0) == -1)
  {
    error(strerror(errno));
    return false;
  }

  return true;
}

extern void umount_all(void)
{
  FILE *file = 0;
  size_t i = 0;
  size_t size = 4096;
  char **paths = 0;
  char line[LINE_MAX] = {0};
  char *path = 0;

  if((file = fopen("/proc/mounts","rb")) == 0)
  {
    error(strerror(errno));
    return;    
  }

  paths = malloc0(sizeof(char *) * size);

  while(fgets(line,sizeof(line),file) != 0)
  {
    if(
      i == size - 1                                          ||
      strtok(line,SPACE_CHARS) == 0                          ||
      (path = strtok(0,SPACE_CHARS)) == 0                    ||
      strncmp(path,INSTALL_ROOT,sizeof(INSTALL_ROOT)-1) != 0 ||
      strcmp(path,INSTALL_ROOT) == 0
    )
      continue;
    
    paths[i++] = strdup(path);
  }
  
  paths[i] = 0;
  
  for( i = 0 ; paths[i] != 0 ; ++i )
  {
    path = paths[i];
  
    if(!umount_retry(path))
    {
      error(strerror(errno));
      goto bail;
    }
  }

  if(!umount_retry(INSTALL_ROOT))
  {
    error(strerror(errno));
    goto bail;
  }

bail:

  if(file != 0)
    fclose(file);
  
  if(paths != 0)
  {
    for( i = 0 ; paths[i] != 0 ; ++i )
      free(paths[i]);
    
    free(paths);
  }
}

extern bool isrootpath(const char *path)
{
  regex_t re = {0};
  bool match = false;

  if(path == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(regcomp(&re,"^/[a-z]*$",REG_EXTENDED|REG_NOSUB) != 0)
  {
    error("invalid regular expression");
    return false;
  }

  match = (regexec(&re,path,0,0,0) == 0);

  regfree(&re);

  return match;
}

extern bool isasciistring(const char *s)
{
  const unsigned char *p = (unsigned char *) s;
  bool ascii = true;

  if(s == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  while(*p != 0)
  {
    if(*p > 0x7F)
    {
      ascii = false;
      break;
    }

    ++p;
  }

  return ascii;
}

extern bool mkdir_recurse(const char *path)
{
  char buf[PATH_MAX] = {0};
  char *s = buf + 1;

  if(path == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  strfcpy(buf,sizeof(buf),"%s",path);

  while((s = strchr(s,'/')) != 0)
  {
    *s = 0;

    if(mkdir(buf,0755) == -1 && errno != EEXIST)
    {
      error(strerror(errno));
      return false;
    }

    *s = '/';

    ++s;
  }

  if(mkdir(buf,0755) == -1 && errno != EEXIST)
  {
    error(strerror(errno));
    return false;
  }

  return true;
}

extern bool size_to_string(char *s,size_t n,long long size,bool pad)
{
  long long divisor = 0;
  const char *suffix = 0;

  if(s == 0 || n == 0 || size < 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(size >= TEBIBYTE)
  {
    divisor = TEBIBYTE;
    suffix = "TiB";
  }
  else if(size >= GIBIBYTE)
  {
    divisor = GIBIBYTE;
    suffix = "GiB";
  }
  else if(size >= MEBIBYTE)
  {
    divisor = MEBIBYTE;
    suffix = "MiB";
  }
  else if(size >= KIBIBYTE)
  {
    divisor = KIBIBYTE;
    suffix = "KiB";
  }
  else
  {
    divisor = 1;
    suffix = "BiB";
  }

  strfcpy(s,n,"%*.1f%s",(pad) ? 6 : 0,(double) size / divisor,suffix);

  return true;
}

extern long long string_to_size(const char *s)
{
  long double base = 0;
  int off = 0;
  const char *suffix = 0;
  long long unit = 0;

  if(s == 0 || sscanf(s,"%Lf%n",&base,&off) < 1)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  suffix = s + off;

  if(strcmp(suffix,"TiB") == 0)
    unit = TEBIBYTE;
  else if(strcmp(suffix,"GiB") == 0)
    unit = GIBIBYTE;
  else if(strcmp(suffix,"MiB") == 0)
    unit = MEBIBYTE;
  else if(strcmp(suffix,"KiB") == 0)
    unit = KIBIBYTE;
  else if(strcmp(suffix,"BiB") == 0)
    unit = 1;
  else
    unit = 1;

  return base * unit;
}

extern int get_text_length(const char *s)
{
  wchar_t wc = 0;
  size_t n = 0;
  size_t len = 0;
  mbstate_t mbs = {0};
  int l = 0;

  if(s == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  len = strlen(s);

  while(true)
  {
    n = mbrtowc(&wc,s,len,&mbs);

    if(n == (size_t) -1 || n == (size_t) -2)
    {
      error(strerror(errno));
      return -1;
    }

    if(n == 0)
      break;

    ++l;

    s += n;

    len -= n;
  }

  return l;
}

extern bool execute(const char *command,const char *root,pid_t *cpid)
{
  pid_t pid = -1;
  int status = 0;

  if(command == 0 || root == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  eprintf("Attempting to execute command '%s' with root directory '%s'.\n",command,root);

  if((pid = fork()) == -1)
  {
    error(strerror(errno));
    return false;
  }

  if(pid == 0)
  {
    int fd = open(LOGFILE,O_WRONLY|O_APPEND|O_CREAT,0644);

    if(fd == -1)
      _exit(200);

    dup2(fd,STDOUT_FILENO);

    dup2(fd,STDERR_FILENO);

    close(fd);

    if(chroot(root) == -1)
      _exit(210);

    if(chdir("/") == -1)
      _exit(220);

    execl("/bin/sh","/bin/sh","-c",command,(void *) 0);

    _exit(230);
  }

  if(cpid != 0)
  {
    *cpid = pid;
    return true;
  }

  if(waitpid(pid,&status,0) == -1 || !WIFEXITED(status))
  {
    error(strerror(errno));
    return false;
  }

  eprintf("Command '%s' which was executed with root directory '%s' has exitted with code '%d'.\n",command,root,WEXITSTATUS(status));

  return (WEXITSTATUS(status) == 0);
}

extern void *memdup(const void *mem,size_t size)
{
  if(mem == 0 || size == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  return memcpy(malloc0(size),mem,size);
}

extern void *malloc0(size_t size)
{
  if(size == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  return memset(malloc(size),0,size);
}

extern int get_text_screen_width(const char *s)
{
  wchar_t wc = 0;
  size_t n = 0;
  size_t len = 0;
  mbstate_t mbs = {0};
  int w = 0;
  int i = 0;

  if(s == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  len = strlen(s);

  while(true)
  {
    n = mbrtowc(&wc,s,len,&mbs);

    if(n == (size_t) -1 || n == (size_t) -2)
    {
      error(strerror(errno));
      return -1;
    }

    if(n == 0 || wc == L'\n')
      break;

    switch(wc)
    {
      case L'\t':
        w += 8;
        break;

      default:
        if((i = wcwidth(wc)) > 0)
          w += i;
        break;
    }

    s += n;

    len -= n;
  }

  return w;
}

extern bool get_text_screen_size(const char *text,int *width,int *height)
{
  const char *s = text;
  int cw = 0;
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  while(true)
  {
    cw = get_text_screen_width(s);

    if(cw == -1)
      return false;

    if(w < cw)
      w = cw;

    if((s = strchr(s,'\n')) == 0)
      break;

    ++h;

    ++s;
  }

  *width = w;

  *height = h;

  return true;
}

extern bool get_button_screen_size(const char *text,int *width,int *height)
{
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if((w = get_text_screen_width(text)) == -1)
    return false;

  w += 5;

  h = 4;

  *width = w;

  *height = h;

  return true;
}

extern bool get_label_screen_size(const char *text,int *width,int *height)
{
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if((w = get_text_screen_width(text)) == -1)
    return false;

  h = 1;

  *width = w;

  *height = h;

  return true;
}

extern bool get_checkbox_screen_size(const char *text,int *width,int *height)
{
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if((w = get_text_screen_width(text)) == -1)
    return false;

  w += 4;

  h = 1;

  *width = w;

  *height = h;

  return true;
}
