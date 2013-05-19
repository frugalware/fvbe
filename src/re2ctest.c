#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "scanner.h"

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

static struct re2ctest utf8[] =
{
  { "en_US.utf8", true  },
  {      "en_US", false },
  { "fr_FR.utf8", true  },
  {      "fr_FR", false },
  {      "wa_BE", false },
  { "wa_BE.utf8", true  },
  {           "", false },
  {     "fggfdf", false },
  {            0, false }
};

static struct re2ctest raid[] =
{
  {   "md0", true  },
  {  "md60", true  },
  { "md200", true  },
  { "md300", false },
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
  { "md300", false },
  {       0, false }
};

static struct re2ctest gpt[] =
{
  {                "GFBY%YT%$^&**&*^^^%^@$", true  },
  {  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", true  },
  { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", false },
  {                           "Shut Me Up!", true  },
  {                                  "\x80", false },
  {                                  "\x19", false },
  {                                  "\x15", false },
  {                                  "\x90", false },
  {                                      "", true  },
  {                                       0, false }
};

static struct re2ctest user[] =
{
  {                              "ryuo", true  },
  {                             "ryuo$", true  },
  {                          "fun_user", true  },
  {                          "fun-user", true  },
  {                         "fun_user$", true  },
  {                         "fun-user$", true  },
  {                   "_4544-_--dfds-$", true  },
  {                    "_4544-_--dfds-", true  },
  {                   "a4544-_--dfds-$", true  },
  {                    "a4544-_--dfds-", true  },
  {  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", true  },
  { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", false },
  {                                  "", false },
  {                          "extreme!", false },
  {                                   0, false }
};

static struct re2ctest dnslabel[] =
{
  {  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", true  },
  { "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", false },
  {                                                          "0foobar0", true  },
  {                                                          "afoobara", true  },
  {                                                         "0foo-bar0", true  },
  {                                                         "afoo-bara", true  },
  {                                                            "foobar", true  },
  {                                                           "-foobar", false },
  {                                                           "foobar-", false },
  {                                                          "-foobar-", false },
  {                                                                  "", false },
  {                                                                   0, false }
};

static struct re2ctest v4[] =
{
  {       "127.0.0.1", true  },
  { "255.255.255.255", true  },
  { "256.255.255.255", false },
  { "255.256.255.255", false },
  { "255.255.256.255", false },
  { "255.255.255.256", false },
  {         "0.0.0.0", true  },
  {     "192.168.0.1", true  },
  {     "-1.-1.-1.-1", false },
  {               "0", false },
  {             "0.0", false },
  {           "0.0.0", false },
  {      "Whats up!?", false },
  {                "", false },
  {                 0, false }
};

static struct re2ctest v6[] =
{
  {                                       "::ffff", true  },
  {                                  "::ffff:ffff", true  },
  {                             "::ffff:ffff:ffff", true  },
  {                        "::ffff:ffff:ffff:ffff", true  },
  {                   "::ffff:ffff:ffff:ffff:ffff", true  },
  {              "::ffff:ffff:ffff:ffff:ffff:ffff", true  },
  {         "::ffff:ffff:ffff:ffff:ffff:ffff:ffff", true  },
  {          "ffff::ffff:ffff:ffff:ffff:ffff:ffff", true  },
  {          "ffff:ffff::ffff:ffff:ffff:ffff:ffff", true  },
  {          "ffff:ffff:ffff::ffff:ffff:ffff:ffff", true  },
  {          "ffff:ffff:ffff:ffff::ffff:ffff:ffff", true  },
  {          "ffff:ffff:ffff:ffff:ffff::ffff:ffff", true  },
  {          "ffff:ffff:ffff:ffff:ffff:ffff::ffff", true  },
  {      "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", true  },
  { "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", false },
  {    "::ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff::ffff:ffff:ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff::ffff:ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff::ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff:ffff::ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff:ffff:ffff::ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff:ffff:ffff:ffff::ffff:ffff", false },
  { "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff::ffff:ffff:ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff::ffff:ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff::ffff:ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff:ffff::ffff:ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff:ffff:ffff::ffff:ffff:ffff", false },
  {     "ffff:ffff:ffff:ffff:ffff:ffff::ffff:ffff", false },
  {     "ffff:ffff:ffff:ffff:ffff:ffff:ffff::ffff", false },
  { "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", false },
  {                                          "::x", false },
  {                                 "::ffff::ffff", false },
  {                                  "192.168.0.1", false },
  {                                   "What's up?", false },
  {                                             "", false },
  {                                              0, false }
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

extern int main(void)
{
  RE2CTEST(root,is_root_path);

  RE2CTEST(utf8,is_utf8_locale);

  RE2CTEST(raid,is_raid_device);

  RE2CTEST(disk,is_disk_device);

  RE2CTEST(gpt,is_partition_name);

  RE2CTEST(user,is_user_name);

  RE2CTEST(dnslabel,is_dns_label);

  RE2CTEST(v4,is_ip_v4);

  RE2CTEST(v6,is_ip_v6);

  printf("re2c has passed all tests\n");
  
  return EXIT_SUCCESS;
}
