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

static bool information_run(void)
{
  if(!ui_dialog_yesno(INFO_TITLE,INFO_TEXT,false))
    return true;

  ui_window_text(INFO_MODULE_TITLE,INFO_MODULE_TEXT);

  ui_window_text(INFO_BYTES_TITLE,INFO_BYTES_TEXT);

  return true;
}

static void information_reset(void)
{
}

struct module information_module =
{
  information_run,
  information_reset,
  __FILE__
};