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
Filesystems can only be created from RAID devices, swap partitions, or data partitions.
--
