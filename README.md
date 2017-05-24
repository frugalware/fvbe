== fvbe
This is FVBE (Frugalware Versatile Bootable Environment).
It is intended to be an all-in-one bootable environment
for users to perform installation, maintenance, system
rescue, and other tasks as the need is discovered.

=== Boot Menu
From this menu, you can modify the video resolution, language, region localization,
and keyboard layout that will be in use when you boot. If you choose the `RAM` boot
entry, then the ISO will load its root filesystem into `RAM`.

=== Users
The two special users are `root` and `guest`. Their passwords are both `fvbe`.

== fwsetup

=== What is modified on the host system?

.Modified File Paths
* /var/log/fwsetup.log
* /var/log/fwsetup-valgrind.log
* /dev/* (depends on what the installer is told to do)
* /mnt/install
* /mnt/iso

Host's keyboard layouts are also modified if we are running from FVBE and in an environment
where we can actually do this.

=== Things you need to know.

The design of our installer assumes that you know what you are doing in general, and serves
mainly to make the installation for experienced users easier. Below is a list of some of the
assumptions we make about your knowledge as a user. The list will never be complete, but
should serve as a reference if the installer doesn't behave as you expect. Some of these things
are included as basic sanity checks, but not all of them are because it's a normal and expected
limitation.

.Knowledge Assumptions
* A BIOS partition is needed to use GRUB in BIOS mode on GPT tables and it needs to be at least 1 MiB in size.
* The first partition needs to start at an offset of at least 1 MiB to use GRUB in BIOS mode on DOS tables.
* At least one of your physical drives must have a partition table.
* You need at least one valid device to act as your root partition.
* At least 2 RAID partitions are needed in order to create a RAID device.
* RAID devices can only be created from RAID partitions.
* The partitions used in a RAID device should be on different physical drives.
* Swap can only be created on swap partitions.
* Filesystems can only be created on RAID devices or data partitions.
* Filesystems can only be mounted at root directories.
* At least one filesystem must be mounted as '/'.
* An active and reliable internet connection is required for network installations to work.
* At least the base package group will be installed.


=== Locale and Layout Modules

These modules are used to set the language, region localization, and keyboard layout that
will be used throughout the rest of the installer. It is also used by the installation
once the installer is finished.

=== Partition Module

Our partitioner provides a simple means of modifying partition tables.
Below is a list of what you can do with our partitioner.

.Partitioner Features
* Create DOS or GPT tables.
* Create partitions at the end of the table.
* Delete partitions from the end of the table.
* Change a partition's type or purpose.
* Change a partition's name (GPT Only).
* Change a partition's active status.

If you need something more advanced, then you should use one of the programs
from the list below before starting the installer.

.Advanced Partitioners
* gdisk
* fdisk
* parted

When doing this, select `Next` from our partitioner UI without modifying
any of the tables from the UI. Our partitioner will just skip to the next
module, using your existing partition tables.

=== Format Module

Our formatter is used to format the filesystems that will be used on your
new installation. If you choose to setup swap, they will be activated before
the installation commences. If you choose to use `noformat`, then the existing
filesystem will be reused. This is only advised if you know what you are doing.

=== Install Module

If you are installing from FVBE, then your installation will be limited to
whatever package groups were provided with the ISO you downloaded and no
packages will be downloaded. If the ISO did not come with any packages then
all major package groups are available for download.

If you are installing from an existing installation, then the installer will
use your existing package cache. It will download a new database, thus a
network connection is required even if your cache is complete.

=== Postconfig Module

This module will prompt you for various configuration details it requires from
you. It should not require any further details here.