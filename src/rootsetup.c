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

static struct account *account = 0;

static bool rootsetup_action(const struct account *account)
{
  char command[_POSIX_ARG_MAX] = {0};

  if(account == 0 || account->user == 0 || account->password == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  strfcpy(command,sizeof(command),"echo '%s:%s' | chpasswd",account->user,account->password);

  return execute(command,g->guestroot,0);
}

static bool rootsetup_start(void)
{
  account = malloc0(sizeof(struct account));

  if(!ui_window_root(account))
    return false;

  return true;
}

static bool rootsetup_finish(void)
{
  bool success = true;
  
  if(account != 0)
  {
    success = rootsetup_action(account);
    account_free(account);
    account = 0;
  }

  return success;
}

struct tool rootsetup_tool =
{
  rootsetup_start,
  rootsetup_finish,
  __FILE__
};
