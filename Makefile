CFLAGS                 ?= -O2
CFLAGS                 += -std=gnu99

ifdef RELEASE_STABLE
CFLAGS                 += -DRELEASE_STABLE
LDFLAGS                += -s
endif

ifdef RELEASE_CURRENT
CFLAGS                 += -Wall -Wextra -DRELEASE_CURRENT -ggdb3
endif

ifdef UI_NEWT
CFLAGS                 += $(shell pkg-config --cflags libnewt)
LDFLAGS                += $(shell pkg-config --libs libnewt)
SOURCES                += src/ui_newt.c
endif

CFLAGS                 += $(shell pkg-config --cflags pacman blkid)
LDFLAGS                += $(shell pkg-config --libs pacman blkid)
SOURCES                := src/main.c src/utility.c src/block.c src/locale.c src/layout.c src/greeter.c src/partition.c src/raid.c src/format.c src/preconfig.c src/install.c src/postconfig.c src/finale.c src/grubconfig.c src/hostconfig.c src/rootconfig.c src/userconfig.c src/timeconfig.c src/modeconfig.c src/langconfig.c src/kbconfig.c
OBJECTS                := $(patsubst %.c,%.o,$(SOURCES))

include fvbe.conf

FVBE_ARCH              := $(subst ",,$(FVBE_ARCH))
FVBE_ISO_TYPE          := $(subst ",,$(FVBE_ISO_TYPE))
FVBE_ROOTFS_REPOSITORY := $(subst ",,$(FVBE_ROOTFS_REPOSITORY))

all:

%.o: %.c src/text.h src/local.h
	cc $(CFLAGS) -c $< -o $@

vmlinuz initrd squashfs.img pacman-g2.conf locales layouts unicode.pf2: bin/create-rootfs
	bin/create-rootfs

fvbe-$(FVBE_ISO_TYPE)-$(FVBE_ARCH).iso: vmlinuz initrd squashfs.img pacman-g2.conf locales layouts unicode.pf2 bin/create-iso
	bin/create-iso

iso: fvbe-$(FVBE_ISO_TYPE)-$(FVBE_ARCH).iso

src/fwsetup: $(OBJECTS)
	cc $(LDFLAGS) $^ -o $@

setup: src/fwsetup

clean:
	rm -f $(OBJECTS) src/fwsetup vmlinuz initrd squashfs.img pacman-g2.conf locales layouts unicode.pf2 fvbe-$(FVBE_ISO_TYPE)-$(FVBE_ARCH).iso
