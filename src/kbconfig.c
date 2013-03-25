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

struct layout
{
  char *kbdlayout;
  char *xkblayout;
  char *xkbmodel;
  char *xkbvariant;
  char *xkboptions;
};

static struct layout **layouts = 0;
static char **entries = 0;
static size_t count = 0;
static struct layout *layout = 0;

static inline bool get_token(char *in,char **out)
{
  return (*out = strtok(in,SPACE_CHARS)) != 0;
}

static inline void put_token(char *in,char **out)
{
  *out = (strcmp(in,"-") == 0) ? 0 : strdup(in);
}

static inline void put_xkb(FILE *file,char *s1,char *s2)
{
  if(s1 == 0 || strlen(s1) == 0)
    return;

  fprintf(file,"%8cOption \"%s\" \"%s\"\n",' ',s2,s1);
}

static int qsort_compare(const void *A,const void *B)
{
  const struct layout *a = *(struct layout **) A;  
  const struct layout *b = *(struct layout **) B;
  
  return strcmp(a->kbdlayout,b->kbdlayout);
}

static int bsearch_compare(const void *A,const void *B)
{
  const char *a = (char *) A;
  const struct layout *b = *(struct layout **) B;

  return strcmp(a,b->kbdlayout);
}

static bool write_vconsole_conf(const struct layout *layout)
{
  FILE *file = 0;
  
  if(layout->kbdlayout == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  if((file = fopen("etc/vconsole.conf","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  fprintf(file,
    "KEYMAP=%s\n"
    "FONT=%s\n",
    layout->kbdlayout,
    "ter-v16b"
  );
  
  fclose(file);
  
  return true;
}

static bool write_keyboard_conf(const struct layout *layout)
{
  FILE *file = 0;
  
  if((file = fopen("etc/X11/xorg.conf.d/00-keyboard.conf","wb")) == 0)
  {
    error(strerror(errno));
    return false;
  }

  fprintf(file,
    "Section \"InputClass\"\n"
    "%8cIdentifier \"system-keyboard\"\n"
    "%8cMatchIsKeyboard \"on\"\n",
    ' ',
    ' '
  );

  put_xkb(file,layout->xkblayout,"XkbLayout");

  put_xkb(file,layout->xkbmodel,"XkbModel");
  
  put_xkb(file,layout->xkbvariant,"XkbVariant");
  
  put_xkb(file,layout->xkboptions,"XkbOptions");

  fprintf(file,
    "EndSection\n"
  );

  fclose(file);

  return true;
}

static bool kbconfig_setup(void)
{
  FILE *file = 0;
  size_t i = 0;
  size_t size = 4096;
  char line[LINE_MAX] = {0};
  char *kbdlayout = 0;
  char *xkblayout = 0;
  char *xkbmodel = 0;
  char *xkbvariant = 0;
  char *xkboptions = 0;
  struct layout *layout = 0;
  
  if((file = fopen("/usr/share/systemd/kbd-model-map","rb")) == 0)
  {
    error(strerror(errno));
    return false;
  }
  
  layouts = alloc(struct layout *,size);
  
  while(fgets(line,sizeof(line),file) != 0)
  {
    if(
      i == size - 1               ||
      *line == '#'                ||
      !get_token(line,&kbdlayout) ||
      !get_token(0,&xkblayout)    ||
      !get_token(0,&xkbmodel)     ||
      !get_token(0,&xkbvariant)   ||
      !get_token(0,&xkboptions)
    )
      continue;
    
    layout = alloc(struct layout,1);
    
    put_token(kbdlayout,&layout->kbdlayout);

    put_token(xkblayout,&layout->xkblayout);
    
    put_token(xkbmodel,&layout->xkbmodel);
    
    put_token(xkbvariant,&layout->xkbvariant);
    
    put_token(xkboptions,&layout->xkboptions);
    
    layouts[i++] = layout;
  }

  layouts[i] = 0;
  
  layouts = realloc(layouts,sizeof(struct layout *) * (i+1));

  qsort(layouts,i,sizeof(struct layout *),qsort_compare);

  entries = alloc(char *,i + 1);

  entries[i] = 0;

  count = i;
  
  do
  {
    --i;
    entries[i] = layouts[i]->kbdlayout;
  }
  while(i > 0);

  fclose(file);

  return true;
}

static bool kbconfig_update(const struct layout *layout)
{
  char command[_POSIX_ARG_MAX] = {0};
  
  strfcpy(command,sizeof(command),"loadkeys '%s'",layout->kbdlayout);
  
  if(!execute(command,g->hostroot,0))
    return false;

  if(areweinx11())
  {
    strfcpy(command,sizeof(command),"setxkbmap -layout '%s' -model '%s' -variant '%s' -option '' -option '%s'",
      strng(layout->xkblayout),
      strng(layout->xkbmodel),
      strng(layout->xkbvariant),
      strng(layout->xkboptions)
    );
    
    if(!execute(command,g->hostroot,0))
      return false;
  }
  
  return true;
}

static bool update_via_old(const struct layout *layout)
{
  if(!write_vconsole_conf(layout))
    return false;
  
  if(!write_keyboard_conf(layout))
    return false;
  
  return true;
}

static bool update_via_new(const struct layout *layout)
{
  char command[_POSIX_ARG_MAX] = {0};

  strfcpy(command,sizeof(command),"localectl --no-convert set-keymap '%s'",layout->kbdlayout);

  if(!execute(command,g->guestroot,0))
    return false;

  strfcpy(command,sizeof(command),"localectl --no-convert set-x11-keymap '%s' '%s' '%s' '%s'",
    strng(layout->xkblayout),
    strng(layout->xkbmodel),
    strng(layout->xkbvariant),
    strng(layout->xkboptions)
  );

  if(!execute(command,g->guestroot,0))
    return false;

  return true;
}

static bool kbconfig_action(const struct layout *layout)
{
  if(g->insetup)
    return update_via_old(layout);

  return update_via_new(layout);
}

static bool kbconfig_start(void)
{
  char *entry = 0;

  if(!kbconfig_setup())
    return false;

  if(!ui_window_list(LAYOUT_TITLE,LAYOUT_TEXT,entries,&entry))
    return false;

  if(
    (layout = bsearch(entry,layouts,count,sizeof(struct layout *),bsearch_compare)) == 0 ||
    (layout = *(struct layout **) layout) == 0
  )
  {
    error("no matching layout");
    return false;
  }

  if(!kbconfig_update(layout))
    return false;

  return true;
}

static bool kbconfig_finish(void)
{
  bool success = true;
  
  if(layout != 0)
    success = kbconfig_action(layout);

  if(layouts != 0)
  {
    for( size_t i = 0 ; layouts[i] != 0 ; ++i )
    {
      free(layouts[i]->kbdlayout);
      
      free(layouts[i]->xkblayout);
      
      free(layouts[i]->xkbmodel);
      
      free(layouts[i]->xkbvariant);
      
      free(layouts[i]->xkboptions);
      
      free(layouts[i]);
    }
    
    free(layouts);
    
    layouts = 0;
  }

  free(entries);

  entries = 0;
  
  count = 0;
  
  layout = 0;

  return success;
}

struct tool kbconfig_tool =
{
  kbconfig_start,
  kbconfig_finish,
  __FILE__
};
