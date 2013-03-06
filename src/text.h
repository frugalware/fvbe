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

#pragma once

#define _(S) S

#define OK_BUTTON_TEXT _("Ok")
#define CANCEL_BUTTON_TEXT _("Cancel")
#define YES_BUTTON_TEXT _("Yes")
#define NO_BUTTON_TEXT _("No")
#define PREVIOUS_BUTTON_TEXT _("Previous")
#define NEXT_BUTTON_TEXT _("Next")
#define PASSWORD_ENTER_TEXT _("Enter Password")
#define PASSWORD_CONFIRM_TEXT _("Confirm Password")
#define PASSWORD_LENGTH 6
#define PASSWORD_SHORT_TITLE _("Password Too Short")
#define PASSWORD_SHORT_TEXT _("Your password must be at least 6 characters long.\n")
#define PASSWORD_MISMATCH_TITLE _("Passwords Do Not Match")
#define PASSWORD_MISMATCH_TEXT _("The passwords you have entered do not match.\n")
#define LOCALE_TITLE _("Locale Setup")
#define LOCALE_TEXT _("Select a locale from the list below.\nThe first part of the locale is the language code.\nThe second part of the locale is the country code.\nThe third part of the locale is the encoding.\nIt will be used for the '%s' variable.\n")
#define LAYOUT_TITLE _("Keyboard Layout Setup")
#define LAYOUT_TEXT _("Select a keyboard layout from the list below.\nThis also configures your X11 keyboard layout.\n")
#define GREETER_TEXT _("Welcome to the Frugalware Linux installer.\nWe have worked hard to provide this distribution to the public.\nWe hope to provide you with an enjoyable experience.\n")
#define PARTITION_TITLE _("Partition Setup")
#define PARTITION_TEXT _("Select a disk device to setup a new partition table.\nSelect a partition to modify its parameters.\nSelect a disk device's free space to setup a new partition.\nSelect a disk device's deletion entry to delete the last partition.\n")
#define PARTITION_DIALOG_NEW_TABLE_TITLE _("New Partition Table")
#define PARTITION_DIALOG_NEW_TABLE_TEXT _("Select a partition table type from the list below.\n")
#define PARTITION_DIALOG_MODIFY_PARTITION_TITLE _("Modifying a Partition")
#define PARTITION_DIALOG_MODIFY_PARTITION_TEXT _("Select a partition name, active status, and usage purpose.\nPartition name is only used if the partition table is GPT.\n")
#define PARTITION_DIALOG_MODIFY_PARTITION_NAME_TEXT _("Partition Name")
#define PARTITION_DIALOG_MODIFY_PARTITION_ACTIVE_TEXT _("Active")
#define PARTITION_DIALOG_MODIFY_PARTITION_ERROR_TITLE _("Invalid Partition Parameters")
#define PARTITION_DIALOG_MODIFY_PARTITION_ERROR_TEXT _("You have specified invalid parameters.\nPlease check that they meet the following requirements:\n\nPartition name must contain only ASCII characters.\nPartition name must be less than 37 characters.\nPartition's purpose can only be 'bios' on GPT label.\n")
#define PARTITION_DIALOG_NEW_PARTITION_TITLE _("New Partition")
#define PARTITION_DIALOG_NEW_PARTITION_TEXT _("Select a partition size and partition type.\nIf this is a DOS label, then you need an extended\npartition to create more than 4 partitions.\n")
#define PARTITION_DIALOG_NEW_SIZE_TEXT _("Partition Size")
#define PARTITION_DIALOG_NEW_PARTITION_ERROR_TITLE _("Partition Allocation Error")
#define PARTITION_DIALOG_NEW_PARTITION_ERROR_TEXT _("An error has occurred while attempting to allocate a new partition.\nAny of the following reasons may be the cause:\n\nYou have entered an invalid size. It must use binary units.\nThe partition limit has been reached.\nThere is no space left for the partition.\n")
#define NO_PARTITION_TITLE _("No Partitions")
#define NO_PARTITION_TEXT _("None of the disk drives have any partitions.\nYou must have at least one disk drive with a partition table.\n")
#define NO_GRUB_BIOS_TITLE _("Cannot Embed GRUB")
#define NO_GRUB_BIOS_TEXT _("At least one of your partition tables lacks space to embed GRUB.\nIf it is DOS, the 1st partition must have offset of at least 1 MiB.\nIf it is GPT, the BIOS partition must be at least 1 MiB in size.\nDo you wish to proceed anyway?\n")
#define FORMAT_TITLE _("Device Setup")
#define FORMAT_TEXT _("Select a device to setup from the list below.\n")
#define FORMAT_DIALOG_TITLE _("Filesystem Setup")
#define FORMAT_DIALOG_TEXT _("Select mount path, parameters, and filesystem.\nMount path is optional if 'swap' is the selected filesystem.\nParameters are custom arguments passed to the format program.\nSelect 'noformat' if you wish to reuse the existing filesystem.\n")
#define FORMAT_PATH_TITLE _("Invalid Filesystem Parameters")
#define FORMAT_PATH_TEXT _("You have specified invalid parameters.\nPlease check that they meet the following requirements:\n\nMount path must begin with a '/'.\nMount path may only contain one directory.\nMount path must not already be in use.\n'noformat' cannot be chosen if there is no existing filesystem.\n")
#define FORMAT_MOUNT_ENTRY_TEXT _("Mount Path")
#define FORMAT_PARAMETERS_ENTRY_TEXT _("Format Parameters")
#define INSTALL_TITLE _("Package Group Selection")
#define INSTALL_TEXT _("Select which package groups you wish to install.\n")
#define NO_ROOT_TITLE _("No Root Device")
#define NO_ROOT_TEXT _("You must have at least one device marked for usage at mount path '/'.\n")
#define NO_SWAP_TITLE _("No Swap Device")
#define NO_SWAP_TEXT _("You have no devices marked for usage as 'swap'.\nIt is recommended that you have at least one.\nDo you wish to proceed without swap?\n")
#define NO_BASE_TITLE _("Group Selection Error")
#define NO_BASE_TEXT _("You must select at least the base package group.\n")
#define ROOT_TITLE _("Root Password Setup")
#define ROOT_TEXT _("Enter a password for the root user.\nIt must be at least 6 characters long.\n")
#define NAME_ENTRY_TEXT _("Real Name")
#define USER_ENTRY_TEXT _("User Name")
#define USER_TITLE _("Initial User Setup")
#define USER_TEXT _("Enter the information for an initial user.\nThe password must be at least 6 characters long.\n")
#define TIME_TITLE _("Initial Time Setup")
#define TIME_TEXT _("Select the timezone relevant to you from the list below.\n")
#define UTC_TEXT _("Universal Time Clock")
#define NO_USER_TITLE _("No User Name")
#define NO_USER_TEXT _("You must specify a user name.\n")
#define MODE_TITLE _("Boot Mode Setup")
#define MODE_TEXT _("Select a boot mode from the list below.\nThis decides whether you boot to a text console or display manager.\nThe display manager mode requires one to be installed.\n")
#define GRUB_TITLE _("GRUB Setup")
#define GRUB_TEXT _("Do you wish to install GRUB as your bootloader?\nYou need a bootloader to boot after install is complete.\n")
#define FINALE_TITLE _("Grand Finale")
#define FINALE_TEXT _("Frugalware Linux has been successfully installed.\nSelect 'Next' to exit the installer.\n")
