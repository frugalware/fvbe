#include "local.h"

struct re2ctest
{
  const char *pattern;
  bool result;
};

static struct re2ctest root[] =
{
  {     "/", true  },
  { "/root", true  },
  {  "/var", true  },
  {  "/tmp", true  },
  { "/home", true  },
  {      "", false },
  { "     ", false },
  {    "/@", false },
  {  "/etc", true  },
  {     "^", false },
  {       0, false }
};

static struct re2ctest raid[] =
{
  {   "md0", true  },
  {  "md60", true  },
  { "md200", true  },
  {   "hda", false },
  {   "vdb", false },
  {   "sda", false },
  {       0, false }
};

static struct re2ctest disk[] =
{
  {   "hda", true  },
  {   "sda", true  },
  {   "vda", true  },
  {   "hdb", true  },
  {   "sdb", true  },
  {   "vdb", true  },
  {   "md0", false },
  {  "md60", false },
  { "md200", false },
  {       0, false }
};

static inline bool re2ctest(struct re2ctest *p,bool (*f) (const char *))
{
  for( ; p->pattern != 0 ; ++p )
  {
    bool result = f(p->pattern);
    
    if(result != p->result)
    {
      printf("pattern '%s' does not match as it should\n",p->pattern);
      return false;
    }
  }
  
  return true;
}

#define RE2CTEST(A,B)                    \
if(!re2ctest(A,B))                       \
{                                        \
  printf("%s failed the re2ctest\n",#B); \
  return EXIT_FAILURE;                   \
}

extern int main(int argc,char **argv)
{
  RE2CTEST(root,is_root_path);

  RE2CTEST(raid,is_raid_device);

  RE2CTEST(disk,is_disk_device);

  printf("re2c has passed all tests\n");
  
  return EXIT_SUCCESS;
}
