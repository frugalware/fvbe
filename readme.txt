= fwsetup

== What is modified on the host system?


`/var/log/fwsetup.log`
`/var/log/fwsetup-valgrind.log`
`/dev/*` (whatever the installer is told to use during the setup for a valid install target)
`/mnt/install`
`/mnt/iso`

Host's keyboard layouts are also modified if we are running from FVBE and in an environment
where we can actually do this.

== Things you need to know.

The design of our installer assumes that you know what you are doing in general, and serves
mainly to make the installation for experienced users easier. Below is a list of some of the
assumptions we make about your knowledge as a user. The list will never be complete, but
should serve as a reference if the installer doesn't behave as you expect. Some of these things
are included as basic sanity checks, but not all of them are because it's a normal and expected
limitation.

--
A BIOS partition is needed to use GRUB in BIOS mode on GPT tables and it needs to be at least 1 MiB in size.
The first partition needs to start at an offset of at least 1 MiB to use GRUB in BIOS mode on DOS tables.
At least one of your physical drives must have a partition table.
You need at least one valid device to act as your root partition.
At least 2 RAID partitions are needed in order to create a RAID device.
RAID devices can only be created from RAID partitions.
The partitions used in a RAID device should be on different physical drives.
Swap can only be created on swap partitions.
Filesystems can only be created on RAID devices or data partitions.
Filesystems can only be mounted at root directories.
At least one filesystem must be mounted as '/'.
--

== Locale and Layout Modules

These modules are used to set the language, region localization, and keyboard layout that
will be used throughout the rest of the installer. It is also used by the installation
once the installer is finished.

== Partition Module

Our partitioner provides a simple means of modifying partition tables.
With it, you can:

--
Create DOS or GPT tables.
Create partitions at the end of the table.
Delete partitions from the end of the table.
Change a partition's type or purpose.
Change a partition's name (GPT Only).
Change a partition's active status.
--

If you need something more advanced, then you should use one of these
included partitioners before starting the installer:

--
gdisk
fdisk
parted
--

When doing this, select `Next` from our partitioner UI without modifying
any of the tables from the UI. Our partitioner will just skip to the next
module, using your existing partition tables.

== Format Module

Our formatter is used to format the filesystems that will be used on your
new installation. If you choose to setup swap, they will be activated before
the installation commences. If you choose to use `noformat`, then the existing
filesystem will be reused. This is only advised if you know what you are doing.

