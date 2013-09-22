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

static inline size_t dirs_count(const char *s)
{
  size_t n = 0;
  bool trail = false;

  if(s == 0 || s[0] != '/')
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;    
  }
  
  for( ; s[0] != 0 ; ++s )
  {
    if(s[0] == '/')
      ++n;

    if(s[0] == '/' && s[1] == 0)
      trail = true;
  }

  if(!trail)
    ++n;
  
  return n;
}

static int dirs_ascend_compare(const void *A,const void *B)
{
  const char *a = * (char **) A;
  const char *b = * (char **) B;
  size_t c = dirs_count(a);
  size_t d = dirs_count(b);
  
  return (c < d) ? -1 : (c > d) ? 1 : 0;
}

static int dirs_descend_compare(const void *A,const void *B)
{
  const char *a = * (char **) A;
  const char *b = * (char **) B;
  size_t c = dirs_count(a);
  size_t d = dirs_count(b);
  
  return (c < d) ? 1 : (c > d) ? -1 : 0;
}

static int raid_compare(const void *A,const void *B)
{
  const char *a = (const char *) A;
  struct raid *b = *(struct raid **) B;
  
  return strcmp(a,raid_get_path(b));
}

extern void iniparser_unset_section(dictionary *d,const char *s)
{
  int i = 0;
  int j = 0;
  char **p = 0;

  if(d == 0 || s == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  j = iniparser_getsecnkeys(d,(char *) s);

  p = iniparser_getseckeys(d,(char *) s);
  
  if(j == 0 || p == 0)
  {
    free(p);
    return;
  }
  
  for( ; i < j ; ++i )
    iniparser_unset(d,p[i]);
  
  iniparser_unset(d,s);
  
  free(p);
}

extern bool readlink0(const char *path,char *buf,size_t size)
{
  ssize_t n = -1;

  if(path == 0 || buf == 0 || size == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  n = readlink(path,buf,size);
  
  if(n > 0 && (size_t) n < size)
  {
    buf[n] = 0;
    return true;
  }

  if(n > 0)
    errno = ERANGE;

  error(strerror(errno));

  return false;
}

extern int charpp_qsort(const void *A,const void *B)
{
  const char *a = *(const char **) A;
  const char *b = *(const char **) B;
  
  return strcmp(a,b);
}

extern void charpp_free(char **pp)
{
  if(pp == 0)
    return;
  
  for( char **p = pp ; *p != 0 ; ++p )
    free(*p);
  
  free(pp);
}

extern char *systemd_escape(const char *in)
{
  static char out[LINE_MAX] = {0};
  char *s = out;
  char *e = out + sizeof(out) - 1;
  
  if(in == 0 || strlen(in) == 0)
  {
    *out = 0;

    return out;
  }

  for( ; *in != 0 ; ++in )
  {
    if(s < e && strchr("'\"\\`$",*in) != 0)
      *s++ = '\\';
    
    if(s < e)
      *s++ = *in;
  }
  
  *s = 0;
  
  return out;

}

extern char *shell_escape(const char *in)
{
  static char out[_POSIX_ARG_MAX] = {0};
  char *s = out;
  char *e = out + sizeof(out) - 1;
  
  if(in == 0 || strlen(in) == 0)
  {
    *out = 0;

    return out;
  }

  for( ; *in != 0 ; ++in )
  {
    if(*in == '\'')
    {
      for( const char *p = "'\\''" ; *p != 0 ; ++p )
        if(s < e)
          *s++ = *p;
    
      continue;
    }
    
    if(s < e)
      *s++ = *in;
  }
  
  *s = 0;
  
  return out;
}

extern void account_free(struct account *account)
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

  free(account);
}

extern int get_number_padding(int n)
{
  int p = 1;

  if(n < 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  for( ; (n / 10) > 0 ; n /= 10 )
    ++p;
  
  return p;
}

extern bool find_unused_raid_device(struct raid **raids,char *s,size_t n)
{
  size_t j = 0;
  struct stat st = {0};

  if(raids == 0 || s == 0 || n == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  for( ; raids[j] != 0 ; ++j )
    ;
  
  for( int i = 0 ; i < 255 ; ++i, *s = 0 )
  {
    strfcpy(s,n,"/dev/md%d",i);
    
    if(
      lfind(s,raids,&j,sizeof(struct raid *),raid_compare) != 0 ||
      stat(s,&st) == 0
    )
      continue;
    
    break;
  }
  
  return (*s != 0);
}

extern void update_raid_add(struct device ***unused,struct raid ***used,struct raid *raid)
{
  size_t unused_size = 1; // Min size = 0
  size_t used_size = 2;   // Min size = 1

  if(unused == 0 || used == 0 || raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  for( size_t i = 0 ; unused[0][i] != 0 ; ++i )
  {
    if(!raid_has_device(raid,unused[0][i]))
      unused[0][unused_size++ - 1] = unused[0][i];
  } 
  
  (*unused)[unused_size - 1] = 0;
  
  *unused = redim(*unused,struct device *,unused_size);

  for( size_t i = 0 ; used[0][i] != 0 ; ++i )
    ++used_size;

  *used = redim(*used,struct raid *,used_size);
  
  (*used)[used_size - 2] = raid;
  
  (*used)[used_size - 1] = 0;
}

extern void update_raid_remove(struct device ***unused,struct raid ***used,struct raid *raid)
{
  size_t unused_size = 1; // Min size = 0
  size_t used_size = 1;   // Min size = 0

  if(unused == 0 || used == 0 || raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  for( size_t i = 0 ; unused[0][i] != 0 ; ++i )
    ++unused_size;

  unused_size += raid_get_count(raid);

  *unused = redim(*unused,struct device *,unused_size);

  for( int i = 0, j = raid_get_count(raid) ; i < j ; ++i )
    (*unused)[unused_size - 1 - j + i] = raid_get_device(raid,i);

  (*unused)[unused_size - 1] = 0;

  for( size_t i = 0 ; used[0][i] != 0 ; ++i )
  {
    if(used[0][i] != raid)
      used[0][used_size++ - 1] = used[0][i];
  }
  
  (*used)[used_size - 1] = 0;
  
  *used = redim(*used,struct raid *,used_size);
}

extern void update_raid_stop_add(struct raid ***stop,struct raid *raid)
{
  size_t size = 2; // Min size = 1

  if(stop == 0 || raid == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  for( size_t i = 0 ; stop[0][i] != 0 ; ++i )
    ++size;

  *stop = redim(*stop,struct raid *,size);
  
  (*stop)[size - 2] = raid;
  
  (*stop)[size - 1] = 0;
}

extern bool copy(const char *old,const char *new)
{
  FILE *in = 0;
  FILE *out = 0;
  bool success = true;
  size_t n = 0;
  size_t size = 128 * KIBIBYTE;
  unsigned char *buf = 0;
  
  if(old == 0 || new == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  if((in = fopen(old,"rb")) == 0 || (out = fopen(new,"wb")) == 0 || (buf = alloc(unsigned char,size)) == 0)
  {
    error(strerror(errno));
    success = false;
    goto bail;
  }

  while(true)
  {
    if((n = fread(buf,1,size,in)) == 0)
      break;
    
    if(fwrite(buf,1,n,out) != n)
      break;
  }

  if(ferror(in) != 0 || ferror(out) != 0)
  {
    error(strerror(errno));
    success = false;
    goto bail;
  }

bail:

  if(in != 0)
    fclose(in);
  
  if(out != 0)
    fclose(out);

  free(buf);

  return success;
}

extern bool isdevicebusy(const char *path)
{
  int fd = -1;

  if(path == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  if((fd = open(path,O_RDONLY|O_EXCL)) == -1)
    return true;
  
  close(fd);
  
  return false;
}

extern size_t strpbrklen(const char *s,const char *accept)
{
  size_t n = 0;

  if(s == 0 || accept == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return 0;
  }

  for( ; (s = strpbrk(s,accept)) != 0 ; ++s, ++n )
    ;

  return n;
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

  paths = alloc(char *,size);

  while(fgets(line,sizeof(line),file) != 0)
  {
    if(
      i == size - 1                                          ||
      strtok(line,SPACE_CHARS) == 0                          ||
      (path = strtok(0,SPACE_CHARS)) == 0                    ||
      strncmp(path,g->guestroot,strlen(g->guestroot)) != 0
    )
      continue;
    
    paths[i++] = strdup(path);
  }
  
  paths[i] = 0;
  
  qsort(paths,i,sizeof(char *),dirs_descend_compare);
  
  for( i = 0 ; paths[i] != 0 ; ++i )
  {
    path = paths[i];
    
    if(umount2(path,MNT_DETACH|UMOUNT_NOFOLLOW) == -1 && errno != ENOENT)
    {
      error(strerror(errno));
      goto bail;
    }
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
    int fd = open(g->logpath,O_WRONLY|O_APPEND|O_CREAT,0600);

    if(fd == -1)
      _exit(200);

    dup2(fd,STDOUT_FILENO);

    dup2(fd,STDERR_FILENO);

    close(fd);

    if(chroot(root) == -1)
      _exit(210);

    if(chdir(g->hostroot) == -1)
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

extern const char *uuidgen(void)
{
  static char uuid[37] = {0};
  uuid_t x = {0};
  
  uuid_generate(x);
  
  uuid_unparse_lower(x,uuid);
  
  return uuid;
}
