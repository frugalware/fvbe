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

#include <pacman.h>
#include "local.h"

//#define LOGMASK (PM_LOG_DEBUG | PM_LOG_ERROR | PM_LOG_WARNING | PM_LOG_FLOW1 | PM_LOG_FLOW2 | PM_LOG_FUNCTION)
#define LOGMASK (PM_LOG_ERROR | PM_LOG_WARNING)

static PM_DB *dl_database = 0;
static char dl_filename[PM_DLFNM_LEN+1] = {0};
static int dl_offset = 0;
static struct timeval dl_time0 = {0};
static struct timeval dl_time1 = {0};
static float dl_rate = 0;
static int dl_xfered1 = 0;
static unsigned int dl_eta_h = 0;
static unsigned int dl_eta_m = 0;
static unsigned int dl_eta_s = 0;
static int dl_remain = 0;
static int dl_howmany = 0;

static void install_database_callback(const char *name,PM_DB *db)
{
  if(strcmp(name,"frugalware") == 0 || strcmp(name,"frugalware-current") == 0)
  {
    if(dl_database != 0)
    {
      eprintf("More than one valid database found in the config file, so skipping it.\n");
      return;
    }

    dl_database = db;
  }
}

static void install_log_callback(unsigned short level,char *msg)
{
  // This parameter is never used.
  level = level;

  eprintf("libpacman: %s%s",msg,(strchr(msg,'\n') == 0) ? "\n" : "");
}

static int install_download_callback(PM_NETBUF *ctl,int dl_xfered0,void *arg)
{
  int dl_amount = 0;
  int dl_total = 0;
  int dl_percent = 0;
  float dl_timediff = 0;
  struct timeval dl_time2 = {0};
  char dl_eta_text[9] = {0};
  char dl_rate_text[47] = {0};
  char dl_size_text[20] = {0};
  int dl_pkg_padding = 0;
  char dl_pkg_text[10] = {0};
  char *s = 0;
  char *dl_file_text = 0;
  char dl_text[256] = {0};

  // This parameter is never used.
  ctl = ctl;

  dl_amount = dl_xfered0 + dl_offset;

  dl_total = * (int *) arg;

  dl_percent = (float) dl_amount / dl_total * 100;

  gettimeofday(&dl_time2,0);

  if(dl_amount == dl_total)
    dl_time1 = dl_time0;

  dl_timediff = (dl_time2.tv_sec - dl_time1.tv_sec) + (float) (dl_time2.tv_usec - dl_time1.tv_usec) / 1000000;

  if(dl_amount == dl_total)
  {
    dl_rate = dl_xfered0 / (dl_timediff * KIBIBYTE);
    dl_eta_h = (int) dl_timediff / 3600;
    dl_eta_m = (int) dl_timediff % 3600 / 60;
    dl_eta_s = (int) dl_timediff % 3600 % 60;
  }
  else if(dl_timediff > 1.0)
  {
    dl_rate = (dl_xfered0 - dl_xfered1) / (dl_timediff * KIBIBYTE);
    dl_xfered1 = dl_xfered0;
    gettimeofday(&dl_time1,0);
    dl_eta_h = (int) ((dl_total - dl_amount) / (dl_rate * KIBIBYTE)) / 3600;
    dl_eta_m = (int) ((dl_total - dl_amount) / (dl_rate * KIBIBYTE)) % 3600 / 60;
    dl_eta_s = (int) ((dl_total - dl_amount) / (dl_rate * KIBIBYTE)) % 3600 % 60;
  }

  strfcpy(dl_eta_text,sizeof(dl_eta_text),"%.2u:%.2u:%.2u",dl_eta_h,dl_eta_m,dl_eta_s);

  if(dl_rate > KIBIBYTE)
    strfcpy(dl_rate_text,sizeof(dl_rate_text),"%6.0fKiB/s",dl_rate);
  else
    strfcpy(dl_rate_text,sizeof(dl_rate_text),"%6.1fKiB/s",dl_rate);

  size_to_string(dl_size_text,sizeof(dl_size_text),dl_amount,true);

  strfcat(dl_size_text,sizeof(dl_size_text),"/");

  size_to_string(dl_size_text+strlen(dl_size_text),sizeof(dl_size_text)-strlen(dl_size_text),dl_total,false);

  if(dl_howmany < 10)
    dl_pkg_padding = 1;
  else if(dl_howmany < 100)
    dl_pkg_padding = 2;
  else if(dl_howmany < 1000)
    dl_pkg_padding = 3;
  else if(dl_howmany < 10000)
    dl_pkg_padding = 4;

  strfcpy(dl_pkg_text,sizeof(dl_pkg_text),"%*d/%d",dl_pkg_padding,dl_remain,dl_howmany);

  if((s = strchr(dl_filename,' ')) != 0)
    *s = 0;

  dl_file_text = dl_filename;

#ifdef UI_NEWT
  int j = NEWT_WIDTH - snprintf(0,0,"(%s) %s (%s) %s %s",dl_pkg_text,"",dl_size_text,dl_rate_text,dl_eta_text);
  int k = strlen(dl_file_text);

  if(j > 0 && j < k)
    dl_file_text[j] = 0;
#endif

  strfcpy(dl_text,sizeof(dl_text),"(%s) %s (%s) %s %s",dl_pkg_text,dl_file_text,dl_size_text,dl_rate_text,dl_eta_text);

  return ui_dialog_progress(_("Downloading"),dl_text,dl_percent);
}

static void install_event_callback(unsigned char event,void *data1,void *data2)
{
  const char *title = 0;
  int percent = -1;

  switch(event)
  {
    case PM_TRANS_EVT_RESOLVEDEPS_START:
      percent = 0;
      title = _("Resolving Dependencies");
      break;

    case PM_TRANS_EVT_RESOLVEDEPS_DONE:
      percent = 100;
      title = _("Resolving Dependencies");
      break;

    case PM_TRANS_EVT_RETRIEVE_START:
      break;

    case PM_TRANS_EVT_RETRIEVE_LOCAL:
      break;

    case PM_TRANS_EVT_INTEGRITY_START:
      percent = 0;
      title = _("Checking Package Integrity");
      break;

    case PM_TRANS_EVT_INTEGRITY_DONE:
      percent = 100;
      title = _("Checking Package Integrity");
      break;

    case PM_TRANS_EVT_INTERCONFLICTS_START:
      break;

    case PM_TRANS_EVT_INTERCONFLICTS_DONE:
      break;

    case PM_TRANS_EVT_FILECONFLICTS_START:
      break;

    case PM_TRANS_EVT_FILECONFLICTS_DONE:
      break;

    case PM_TRANS_EVT_CLEANUP_START:
      break;

    case PM_TRANS_EVT_CLEANUP_DONE:
      break;

    case PM_TRANS_EVT_ADD_START:
      break;

    case PM_TRANS_EVT_EXTRACT_DONE:
      break;

    case PM_TRANS_EVT_SCRIPTLET_INFO:
      break;

    case PM_TRANS_EVT_ADD_DONE:
      break;

    default:
      eprintf("Unhandled pacman transaction event: %hhu\n",event);
      return;
  }

  if(title != 0 && percent != -1)
    ui_dialog_progress(title,"",percent);
}

static void install_conversation_callback(unsigned char event,void *data1,void *data2,void *data3,int *response)
{
  eprintf("Unhandled pacman conversation event: %hhu\n",event);
  *response = 0;
}

static void install_progress_callback(unsigned char event,char *pkg,int percent,int howmany,int remain)
{
  char text[256] = {0};
  int padding = 0;
  const char *title = 0;

  if(howmany < 10)
    padding = 1;
  else if(howmany < 100)
    padding = 2;
  else if(howmany < 1000)
    padding = 3;
  else if(howmany < 10000)
    padding = 4;

  strfcpy(text,sizeof(text),"(%*d/%d)",padding,remain,howmany);

  if(strlen(pkg) > 0)
    strfcat(text,sizeof(text)," - %s",pkg);

  switch(event)
  {
    case PM_TRANS_PROGRESS_INTERCONFLICTS_START:
      title = _("Checking for Inter-Conflicts");
      break;

    case PM_TRANS_PROGRESS_CONFLICTS_START:
      title = _("Checking for File Conflicts");
      break;

    case PM_TRANS_PROGRESS_ADD_START:
      title = _("Installing");
      break;

    default:
      eprintf("Unhandled pacman progress event: %hhu\n",event);
      break;
  }

  if(title != 0 && percent >= 0 && percent <= 100)
    ui_dialog_progress(title,text,percent);
}

static bool install_setup(void)
{
  char path[PATH_MAX] = {0};
  const char *dbdir = 0;
  const char *cachedir = 0;
  const char *hooksdir = 0;

  if(pacman_initialize(INSTALL_ROOT) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_db_register("local") == 0)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_parse_config("/etc/pacman-g2.conf",install_database_callback,"") == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_LOGMASK,(long) LOGMASK) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_LOGCB,(long) install_log_callback) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLCB,(long) install_download_callback) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLFNM,(long) dl_filename) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLOFFSET,(long) &dl_offset) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLT0,(long) &dl_time0) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLT,(long) &dl_time1) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLRATE,(long) &dl_rate) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLXFERED1,(long) &dl_xfered1) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLETA_H,(long) &dl_eta_h) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLETA_M,(long) &dl_eta_m) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLETA_S,(long) &dl_eta_s) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLREMAIN,(long) &dl_remain) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_set_option(PM_OPT_DLHOWMANY,(long) &dl_howmany) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_get_option(PM_OPT_DBPATH,(long *) &dbdir) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_get_option(PM_OPT_CACHEDIR,(long *) &cachedir) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_get_option(PM_OPT_HOOKSDIR,(long *) &hooksdir) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  strfcpy(path,sizeof(path),"%s/%s",INSTALL_ROOT,dbdir);

  if(!mkdir_recurse(path))
    return false;

  strfcpy(path,sizeof(path),"%s/%s",INSTALL_ROOT,cachedir);

  if(!mkdir_recurse(path))
    return false;

  strfcpy(path,sizeof(path),"%s/%s",INSTALL_ROOT,hooksdir);

  if(!mkdir_recurse(path))
    return false;

  return true;
}

static bool install_database_update(void)
{
  if(dl_database == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(pacman_db_update(1,dl_database) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  return true;
}

static void install_groups_free(struct install *groups)
{
  if(groups == 0)
    return;

  for( struct install *grp = groups ; grp->name != 0 ; ++grp )
    free(grp->name);

  free(groups);
}

static bool install_groups_get(struct install **groups)
{
  size_t matches = 0;
  PM_LIST *list = 0;
  struct install *grps = 0;
  size_t j = 0;

  if(groups == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if((list = pacman_db_getgrpcache(dl_database)) == 0)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  for( ; list ; list = pacman_list_next(list) )
  {
    const char *s = (const char *) pacman_list_getdata(list);

    if(s == 0)
      continue;

    if(strcmp(s,"apps") == 0)
      ++matches;
    else if(strcmp(s,"base") == 0)
      ++matches;
    else if(strcmp(s,"devel") == 0)
      ++matches;
    else if(strcmp(s,"gnome") == 0)
      ++matches;
    else if(strcmp(s,"kde") == 0)
      ++matches;
    else if(strcmp(s,"lib") == 0)
      ++matches;
    else if(strcmp(s,"multimedia") == 0)
      ++matches;
    else if(strcmp(s,"network") == 0)
      ++matches;
    else if(strcmp(s,"x11") == 0)
      ++matches;
    else if(strcmp(s,"xapps") == 0)
      ++matches;
    else if(strcmp(s,"xfce4") == 0)
      ++matches;
    else if(strcmp(s,"xlib") == 0)
      ++matches;
    else if(strcmp(s,"xmultimedia") == 0)
      ++matches;
    else if(strstr(s,"-extra") != 0)
      ++matches;
  }

  if(matches == 0)
  {
    eprintf("Could not find any matching groups in the database.\n");
    return false;
  }

  if((list = pacman_db_getgrpcache(dl_database)) == 0)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  grps = malloc0(sizeof(struct install) * (matches + 1));

  for( ; list ; list = pacman_list_next(list) )
  {
    const char *s = (const char *) pacman_list_getdata(list);
    bool cache = false;

    if(s == 0)
      continue;

    if(strcmp(s,"apps") == 0)
      cache = true;
    else if(strcmp(s,"base") == 0)
      cache = true;
    else if(strcmp(s,"devel") == 0)
      cache = true;
    else if(strcmp(s,"gnome") == 0)
      cache = true;
    else if(strcmp(s,"kde") == 0)
      cache = true;
    else if(strcmp(s,"lib") == 0)
      cache = true;
    else if(strcmp(s,"multimedia") == 0)
      cache = true;
    else if(strcmp(s,"network") == 0)
      cache = true;
    else if(strcmp(s,"x11") == 0)
      cache = true;
    else if(strcmp(s,"xapps") == 0)
      cache = true;
    else if(strcmp(s,"xfce4") == 0)
      cache = true;
    else if(strcmp(s,"xlib") == 0)
      cache = true;
    else if(strcmp(s,"xmultimedia") == 0)
      cache = true;
    else if(strstr(s,"-extra") != 0)
      cache = true;

    if(cache)
    {
      grps[j].name = strdup(s);
      grps[j].checked = false;
      ++j;
    }
  }

  grps[j].name = 0;

  grps[j].checked = false;

  *groups = grps;

  return true;
}

static bool install_groups_install(const struct install *groups)
{
  PM_LIST *data = 0;

  if(groups == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(pacman_trans_init(PM_TRANS_TYPE_SYNC,0,install_event_callback,install_conversation_callback,install_progress_callback) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  for( const struct install *i = groups ; i->name != 0 ; ++i )
  {
    if(!i->checked)
      continue;

    PM_GRP *grp = pacman_db_readgrp(dl_database,(char *) i->name);

    if(grp == 0)
    {
      error(pacman_strerror(pm_errno));
      return false;
    }

    PM_LIST *pkgs = (PM_LIST *) pacman_grp_getinfo(grp,PM_GRP_PKGNAMES);

    if(pkgs == 0)
    {
      error(pacman_strerror(pm_errno));
      return false;
    }

    for( ; pkgs ; pkgs = pacman_list_next(pkgs) )
    {
      const char *pkg = (const char *) pacman_list_getdata(pkgs);

      if(pkg == 0)
      {
        error(pacman_strerror(pm_errno));
        return false;
      }

      if(pacman_trans_addtarget(pkg) == -1)
      {
        error(pacman_strerror(pm_errno));
        return false;
      }
    }
  }

  if(pacman_trans_prepare(&data) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  if(pacman_trans_commit(&data) == -1)
  {
    error(pacman_strerror(pm_errno));
    return false;
  }

  return true;
}

static bool install_run(void)
{
  struct install *groups = 0;

  if(!install_setup())
    return false;

  if(!install_database_update())
    return false;

  if(!install_groups_get(&groups))
    return false;

  if(!ui_window_install(groups))
  {
    install_groups_free(groups);
    return false;
  }

  if(!install_groups_install(groups))
  {
    install_groups_free(groups);
    return false;
  }

  install_groups_free(groups);

  return true;
}

static void install_reset(void)
{
  pacman_trans_release();

  pacman_release();

  ui_dialog_progress(0,0,-1);

  dl_database = 0;

  memset(dl_filename,0,sizeof(dl_filename));

  dl_offset = 0;

  memset(&dl_time0,0,sizeof(dl_time0));

  memset(&dl_time1,0,sizeof(dl_time1));

  dl_rate = 0;

  dl_xfered1 = 0;

  dl_eta_h = 0;

  dl_eta_m = 0;

  dl_eta_s = 0;

  dl_remain = 0;

  dl_howmany = 0;
}

struct module install_module =
{
  install_run,
  install_reset,
  __FILE__
};
