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
	cd src; cc $(CFLAGS) -c $(subst src/,,$<) -o $(subst src/,,$@)

rootfs: bin/create-rootfs bin/fwsetup
	bin/create-rootfs
	touch rootfs

iso: rootfs bin/create-iso bin/resolvegroups
	bin/create-iso
	touch iso

bin/fwsetup: $(OBJECTS)
	cc $(LDFLAGS) $^ -o $@

bin/resolvegroups: src/resolvegroups.o
	cc $(LDFLAGS) $^ -o $@

setup: bin/fwsetup

clean:
	rm -rf $(OBJECTS) src/fwsetup vmlinuz initrd rootfs.img squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(ISO) root $(FDB)
