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

static bool userconfig_action(const struct account *account)
{
  char command[_POSIX_ARG_MAX] = {0};

  if(account == 0 || account->user == 0 || account->password == 0 || account->group == 0 || account->groups == 0 || account->home == 0 || account->shell == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  strfcpy(command,sizeof(command),"useradd -m");

  strfcat(command,sizeof(command)," -c '%s'",shell_escape(account->name));
  
  strfcat(command,sizeof(command)," -g '%s'",shell_escape(account->group));

  strfcat(command,sizeof(command)," -G '%s'",shell_escape(account->groups));

  strfcat(command,sizeof(command)," -d '%s'",shell_escape(account->home));

  strfcat(command,sizeof(command)," -s '%s'",shell_escape(account->shell));

  strfcat(command,sizeof(command)," '%s'",shell_escape(account->user));

  if(!execute(command,g->guestroot,0))
    return false;

  strfcpy(command,sizeof(command),"echo '%s:",shell_escape(account->user));

  strfcat(command,sizeof(command),"%s' | chpasswd",shell_escape(account->password));

  if(!execute(command,g->guestroot,0))
    return false;

  return true;
}

static bool userconfig_start(void)
{
  account = alloc(struct account,1);
  
  if(!ui_window_user(account))
    return false;

  return true;
}

static bool userconfig_finish(void)
{
  bool success = true;

  if(account != 0)
  {
    success = userconfig_action(account);
    account_free(account);
    account = 0;
  }

  return success;
}

struct tool userconfig_tool =
{
  userconfig_start,
  userconfig_finish,
  __FILE__
};
