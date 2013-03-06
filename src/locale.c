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

static bool locale_setup(void)
{
  char command[_POSIX_ARG_MAX] = {0};
  FILE *pipe = 0;
  size_t i = 0;
  size_t size = 4096;
  char line[LINE_MAX] = {0};
  char *locale = 0;
  
  strfcpy(command,sizeof(command),"locale --all-locales | grep '\\.utf8$' | sort --unique");

  if((pipe = popen(command,"r")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  locales = malloc0(sizeof(char *) * size);
  
  while(fgets(line,sizeof(line),pipe) != 0)
  {
    if(
      i == size - 1                            ||
      (locale = strtok(line,SPACE_CHARS)) == 0
    )
      continue;
  
    locales[i++] = strdup(locale);
  }

  locales[i] = 0;

  locales = realloc(locales,sizeof(char *) * (i+1));
  
  if(pclose(pipe) == -1)
  {
    error(strerror(errno));
    return false;
  }
  
  return true;
}

static bool locale_do_locale(void)
{
  char text[TEXT_MAX] = {0};
  const char *var = "LANG";
  char *locale = 0;
  
  strfcpy(text,sizeof(text),LOCALE_TEXT,var);
  
  if(!ui_window_list(LOCALE_TITLE,text,locales,&locale))
    return false;

  if(setenv(var,locale,true) == -1)
  {
    error(strerror(errno));
    return false;
  }

  return true;
}

static bool locale_do_other_locales(void)
{
  char text[TEXT_MAX] = {0};
  const char *var = 0;
  static const char *vars[] =
  {
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
  size_t i = 0;
  char *locale = 0;
  
  for( ; vars[i] != 0 ; ++i )
  {
    var = vars[i];
    
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
  
  return true;
}

static bool locale_run(void)
{
  if(!locale_setup())
    return false;

  if(!locale_do_locale())
    return false;

  if(!locale_do_other_locales())
    return false;

  if(setlocale(LC_ALL,"") == 0)
  {
    error(strerror(errno));
    return false;
  }

  return true;
}

static void locale_reset(void)
{
  size_t i = 0;

  if(locales != 0)
  {
    for( ; locales[i] != 0 ; ++i )
      free(locales[i]);
    
    free(locales);
    
    locales = 0;
  }
}

struct module locale_module =
{
  locale_run,
  locale_reset,
  __FILE__
};
