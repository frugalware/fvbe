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
CFLAGS  += -Wall -Wextra -DRELEASE_CURRENT -ggdb3
FDB     := frugalware-current.fdb
endif

ifeq ($(UI),newt)
CFLAGS  += -DUI_NEWT $(shell pkg-config --cflags libnewt)
LDFLAGS += $(shell pkg-config --libs libnewt)
SOURCES := src/ui_newt.c
endif

CFLAGS  += $(shell pkg-config --cflags pacman blkid)
LDFLAGS += $(shell pkg-config --libs pacman blkid)
SOURCES += src/main.c src/utility.c src/block.c src/locale.c src/layout.c src/greeter.c src/partition.c src/raid.c src/format.c src/preconfig.c src/install.c src/postconfig.c src/finale.c src/grubconfig.c src/hostconfig.c src/rootconfig.c src/userconfig.c src/timeconfig.c src/modeconfig.c src/langconfig.c src/kbconfig.c
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

all:

%.o: %.c src/text.h src/local.h
	cc $(CFLAGS) -c $< -o $@

vmlinuz initrd squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(FDB): bin/create-rootfs setup
	bin/create-rootfs

rootfs: vmlinuz initrd squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(FDB)

$(ISO): rootfs bin/create-iso
	bin/create-iso

iso: $(ISO)

src/fwsetup: $(OBJECTS)
	cc $(LDFLAGS) $^ -o $@

setup: src/fwsetup

clean:
	rm -rf $(OBJECTS) src/fwsetup vmlinuz initrd squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(ISO) root $(FDB)
