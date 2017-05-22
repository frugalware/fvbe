include fvbe.conf

ARCH    := $(subst ",,$(FVBE_ARCH))
RELEASE := $(subst ",,$(FVBE_ISO_RELEASE))
TYPE    := $(subst ",,$(FVBE_ISO_TYPE))
BRANCH  := $(subst ",,$(FVBE_ROOTFS_REPOSITORY))
UI      := $(subst ",,$(FVBE_SETUP_UI))

ISO     := fvbe-$(RELEASE)-$(TYPE)-$(ARCH).iso

CFLAGS  ?= -O2
CFLAGS  += -std=gnu99

ifeq ($(BRANCH),stable)
CFLAGS  += -DRELEASE_STABLE
LDFLAGS += -s
FDB     := frugalware.fdb
else ifeq ($(BRANCH),current)
CFLAGS  += -Wall -Wextra -Winline -DRELEASE_CURRENT -ggdb3
FDB     := frugalware-current.fdb
endif


rootfs: bin/create-rootfs
	bin/create-rootfs
	touch rootfs

$(ISO): rootfs bin/create-iso
	bin/create-iso

iso: $(ISO)

clean:
	rm -rf root vmlinuz initrd mounts rootfs.img squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(ISO) tmp local local.lastupdate sums $(FDB) rootfs  var
