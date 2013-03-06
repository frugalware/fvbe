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

static inline bool get_token(char *in,char **out)
{
  return (*out = strtok(in,SPACE_CHARS)) != 0;
}

static inline void put_token(char *in,char **out)
{
  *out = (strcmp(in,"-") == 0) ? 0 : strdup(in);
}

static inline void put_layout(char *in,char **out)
{
  *out = (in == 0) ? 0 : strdup(in);
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

static bool layout_setup(void)
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
  
  layouts = malloc0(sizeof(struct layout *) * size);
  
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
    
    layout = malloc0(sizeof(struct layout));
    
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

  entries = malloc0(sizeof(char *) * (i+1));

  entries[i] = 0;
  
  do
  {
    --i;
    entries[i] = layouts[i]->kbdlayout;
  }
  while(i > 0);

  fclose(file);

  return true;
}

static bool layout_do_layout(void)
{
  char *entry = 0;
  size_t i = 0;
  struct layout *layout = 0;

  if(!ui_window_list(LAYOUT_TITLE,LAYOUT_TEXT,entries,&entry))
    return false;
  
  for( ; layouts[i] != 0 ; ++i )
    ;
  
  if((layout = *(struct layout **) bsearch(entry,layouts,i,sizeof(struct layout *),bsearch_compare)) == 0)
  {
    error("no matching layout");
    return false;
  }

  put_layout(layout->kbdlayout,&g->kbdlayout);

  put_layout(layout->xkblayout,&g->xkblayout);
  
  put_layout(layout->xkbmodel,&g->xkbmodel);
  
  put_layout(layout->xkbvariant,&g->xkbvariant);
  
  put_layout(layout->xkboptions,&g->xkboptions);

  return true;
}

static bool layout_set_layout(void)
{
  char command[_POSIX_ARG_MAX] = {0};

  if(!areweinfwlive())
  {
    error("we are not in fwlive, so skip setting the keyboard layout\n");
    return true;
  }

  if(areweinvc())
    strfcpy(command,sizeof(command),"loadkeys '%s'",
      strng(g->kbdlayout)
    );
  else if(areweinx11())
    strfcpy(command,sizeof(command),"setxkbmap -layout '%s' -model '%s' -variant '%s' -option '%s'",
      strng(g->xkblayout),
      strng(g->xkbmodel),
      strng(g->xkbvariant),
      strng(g->xkboptions)
    );
  else
  {
    error("unknown environment, so skip setting the keyboard layout\n");
    return true;
  }

  return execute(command,"/",0);
}

static bool layout_run(void)
{
  if(!layout_setup())
    return false;

  if(!layout_do_layout())
    return false;

  if(!layout_set_layout())
    return false;

  return true;
}

static void layout_reset(void)
{
  size_t i = 0;
  struct layout *layout = 0;
  
  if(layouts != 0)
  {
    for( ; layouts[i] != 0 ; ++i )
    {
      layout = layouts[i];
      
      free(layout->kbdlayout);
      
      free(layout->xkblayout);
      
      free(layout->xkbmodel);
      
      free(layout->xkbvariant);
      
      free(layout->xkboptions);
      
      free(layout);
    }
    
    free(layouts);
    
    layouts = 0;
    
    free(entries);
    
    entries = 0;
  }
}

struct module layout_module =
{
  layout_run,
  layout_reset,
  __FILE__
};
