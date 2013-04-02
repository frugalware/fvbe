#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pacman.h>

static PM_DB *syncdb = 0;

static void janitor(void)
{
  pacman_trans_release();
  
  pacman_release();
}

static void database_callback(const char *name,PM_DB *db)
{
  if(strcmp(name,"frugalware") == 0 || strcmp(name,"frugalware-current") == 0)
  {
    if(syncdb != 0)
    {
      printf("More than one valid database found in the config file, so skipping it.\n");
      return;
    }

    syncdb = db;
  }
}

static void pmerror(const char *s)
{
  fprintf(stderr,"%s: %s\n",s,pacman_strerror(pm_errno));
}

int main(int argc,char **argv)
{
  char cwd[PATH_MAX] = {0};
  PM_LIST *data = 0;

  if(argc < 2)
  {
    fprintf(stderr,"No arguments.\n");
    return EXIT_FAILURE;
  }

  if(atexit(janitor) != 0)
  {
    perror("atexit");
    return EXIT_FAILURE;
  }

  if(getcwd(cwd,sizeof(cwd)) == 0)
  {
    perror("getcwd");
    return EXIT_FAILURE;
  }

  if(pacman_initialize(cwd) == -1)
  {
    pmerror("pacman_initialize");
    return EXIT_FAILURE;
  }
  
  if(pacman_set_option(PM_OPT_DBPATH,(long) "/") == -1)
  {
    pmerror("pacman_set_option");
    return EXIT_FAILURE;
  }

  if(pacman_db_register("local") == 0)
  {
    pmerror("pacman_db_register");
    return EXIT_FAILURE;
  }
  
  if(pacman_parse_config("pacman-g2.conf",database_callback,"") == -1)
  {
    pmerror("pacman_parse_config");
    return EXIT_FAILURE;
  }
  
  if(pacman_trans_init(PM_TRANS_TYPE_SYNC,PM_TRANS_FLAG_NOCONFLICTS,0,0,0) == -1)
  {
    pmerror("pacman_trans_init");
    return EXIT_FAILURE;
  }

  for( int i = 1 ; i < argc ; ++i )
  {
    PM_GRP *grp = pacman_db_readgrp(syncdb,argv[i]);

    if(grp == 0)
    {
      pmerror("pacman_db_readgrp");
      return EXIT_FAILURE;
    }

    PM_LIST *pkgs = pacman_grp_getinfo(grp,PM_GRP_PKGNAMES);

    if(pkgs == 0)
    {
      pmerror("pacman_grp_getinfo");
      return EXIT_FAILURE;
    }

    for( pkgs = pacman_list_first(pkgs) ; pkgs ; pkgs = pacman_list_next(pkgs) )
    {
      const char *pkg = pacman_list_getdata(pkgs);
      
      if(pacman_trans_addtarget(pkg) == -1)
      {
        pmerror("pacman_trans_addtarget");
        return EXIT_FAILURE;
      }
    }
  }

  if(pacman_trans_prepare(&data) == -1)
  {
    pmerror("pacman_trans_prepare");
    pacman_list_free(data);
    return EXIT_FAILURE;
  }
  
  if((data = pacman_trans_getinfo(PM_TRANS_PACKAGES)) == 0)
  {
    pmerror("pacman_trans_getinfo");
    return EXIT_FAILURE;
  }
  
  for( data = pacman_list_first(data) ; data ; data = pacman_list_next(data) )
  {
    PM_SYNCPKG *syncpkg = pacman_list_getdata(data);
    PM_PKG *pkg = pacman_sync_getinfo(syncpkg,PM_SYNC_PKG);
    
    printf("%s-%s-%s%s:%s\n",
      (char *) pacman_pkg_getinfo(pkg,PM_PKG_NAME),
      (char *) pacman_pkg_getinfo(pkg,PM_PKG_VERSION),
      (char *) pacman_pkg_getinfo(pkg,PM_PKG_ARCH),
      PM_EXT_PKG,
      (char *) pacman_pkg_getinfo(pkg,PM_PKG_SHA1SUM)
    );
  }

  return EXIT_SUCCESS;
}