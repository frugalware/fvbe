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
  {       0, false },
};
