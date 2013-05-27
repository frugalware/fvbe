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

static inline const char *nm_next_token(const char *p)
{
  char c1 = 0;
  char c2 = 0;

  if(*p == 0)
    return 0;

  for( ; (c2 = *p) != 0 ; ++p )
  {
    if(c1 != '\\' && c2 == ':')
      break;
    else if(c2 == '\n')
      break;
  
    c1 = (c1 == '\\' && c2 == '\\') ? 0 : c2;
  }

  return p;
}

static const char *nm_unescape_token(const char *s1,size_t n)
{
  const char *e1 = s1 + n;
  static char buf[LINE_MAX] = {0};
  char pc = 0;
  char *s2 = buf;
  char *e2 = buf + sizeof(buf) - 1;

  for( ; s1 < e1 ; pc = *s1++ )
  {
    const char cc = *s1;
    
    if(pc != '\\' && cc == '\\')
      continue;
    
    if(s2 < e2)
      *s2++ = cc;
  }
  
  *s2 = 0;
  
  return buf;
}

static bool nmconfig_start(void)
{
  return true;
}

static bool nmconfig_finish(void)
{
  return true;
}

struct tool nmconfig_tool =
{
  nmconfig_start,
  nmconfig_finish,
  __FILE__
};
