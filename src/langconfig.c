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

static char **locales = 0;
static const char *vars[] =
{
  "LANG",
  "LC_CTYPE",
  "LC_NUMERIC",
  "LC_TIME",
  "LC_COLLATE",
  "LC_MONETARY",
  "LC_MESSAGES",
  "LC_PAPER",
  "LC_NAME",
  "LC_ADDRESS",
  "LC_TELEPHONE",
  "LC_MEASUREMENT",
  "LC_IDENTIFICATION",
  0
};
static bool action = false;

static bool langconfig_setup(void)
{
  FILE *pipe = 0;
  size_t i = 0;
  size_t size = 4096;
  char line[LINE_MAX] = {0};
  char *locale = 0;

  if((pipe = popen("locale --all-locales","r")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  locales = alloc(char *,size);
  
  while(fgets(line,sizeof(line),pipe) != 0)
  {
    if(
      i == size - 1                            ||
      (locale = strtok(line,SPACE_CHARS)) == 0 ||
      !is_utf8_locale(locale)
    )
      continue;
  
    locales[i++] = strdup(locale);
  }

  locales[i] = 0;

  locales = redim(locales,char *,i + 1);
  
  if(pclose(pipe) == -1)
  {
    error(strerror(errno));
    return false;
  }

  qsort(locales,i,sizeof(char *),charpp_qsort);

  return true;
}

static bool update_via_old(void)
{
  const char *var = vars[0];
  const char *locale = 0;
  FILE *file = 0;
  size_t i = 1;
  
  if((locale = getenv(var)) == 0 || strlen(locale) == 0)
  {
    eprintf("%s is not defined.\n",var);
    return false;
  }
  
  if((file = fopen("etc/locale.conf","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,"%s=%s\n",var,locale);

  for( ; vars[i] != 0 ; ++i )
  {
    var = vars[i];
    
    if((locale = getenv(var)) == 0 || strlen(locale) == 0)
      continue;
    
    fprintf(file,"%s=%s\n",var,locale);
  }

  fclose(file);

  return true;
}

static bool update_via_new(void)
{
  const char *var = vars[0];
  const char *locale = 0;
  char command[_POSIX_ARG_MAX] = {0};
  size_t i = 1;

  if((locale = getenv(var)) == 0 || strlen(locale) == 0)
  {
    eprintf("%s is not defined.\n",var);
    return false;
  }

  strfcpy(command,sizeof(command),"localectl set-locale");

  strfcat(command,sizeof(command)," '%s=",shell_escape(var));

  strfcat(command,sizeof(command),"%s'",shell_escape(locale));

  for( ; vars[i] != 0 ; ++i )
  {
    var = vars[i];
    
    if((locale = getenv(var)) == 0 || strlen(locale) == 0)
      continue;

    strfcat(command,sizeof(command)," '%s=",shell_escape(var));

    strfcat(command,sizeof(command),"%s'",shell_escape(locale));
  }

  if(!execute(command,g->guestroot,0))
    return false;

  return true;
}

static bool langconfig_action(void)
{
  if(g->insetup)
    return update_via_old();

  return update_via_new();
}

static bool langconfig_start(void)
{
  char text[TEXT_MAX] = {0};
  const char *var = vars[0];
  char *locale = 0;
  size_t i = 1;
  bool customizelocale = false;

  if(!langconfig_setup())
    return false;

  if(unsetenv(var) == -1)
  {
    error(strerror(errno));
    return false;
  }

  strfcpy(text,sizeof(text),LOCALE_TEXT,var);

  if(!ui_window_list(LOCALE_TITLE,text,locales,&locale))
    return false;

  if(setenv(var,locale,true) == -1)
  {
    error(strerror(errno));
    return false;
  }

  customizelocale = ui_dialog_yesno(_("Further Locale Customizations"),_("Do you wish to further customize your locale?\n"),true);

  for( ; vars[i] != 0 ; ++i )
  {
    var = vars[i];

    if(unsetenv(var) == -1)
    {
      error(strerror(errno));
      return false;
    }
    
    if(!customizelocale)
      continue;
    
    strfcpy(text,sizeof(text),_("Do you wish to select a locale for '%s'?\n"),var);
  
    if(!ui_dialog_yesno(_("Other Locale Selections"),text,true))
      continue;
    
    strfcpy(text,sizeof(text),LOCALE_TEXT,var);
    
    if(!ui_window_list(LOCALE_TITLE,text,locales,&locale))
      return false;

    if(setenv(var,locale,true) == -1)
    {
      error(strerror(errno));
      return false;
    }    
  }

  if(g->insetup && setlocale(LC_ALL,"") == 0)
  {
    error(strerror(errno));
    return false;
  }

  action = true;

  return true;
}

static bool langconfig_finish(void)
{
  bool success = true;

  if(action)
  {
    success = langconfig_action();
    
    action = false;
  }

  if(locales != 0)
  {
    for( size_t i = 0 ; locales[i] != 0 ; ++i )
      free(locales[i]);
    
    free(locales);
    
    locales = 0;
  }

  return success;
}

struct tool langconfig_tool =
{
  langconfig_start,
  langconfig_finish,
  __FILE__
};
