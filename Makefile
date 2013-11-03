ifndef SYSLINUX
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

ifeq ($(UI),newt)
CFLAGS  += -DUI_NEWT $(shell pkg-config --cflags libnewt)
LDFLAGS += $(shell pkg-config --libs libnewt)
SOURCES := src/ui_newt.c
endif

CFLAGS  += $(shell pkg-config --cflags pacman blkid uuid)
LDFLAGS += $(shell pkg-config --libs pacman blkid uuid)
SOURCES += src/dictionary.c src/iniparser.c src/main.c src/utility.c src/block.c src/locale.c src/layout.c src/greeter.c src/partition.c src/raid.c src/format.c src/preconfig.c src/install.c src/postconfig.c src/finale.c src/grubconfig.c src/hostconfig.c src/rootconfig.c src/userconfig.c src/timeconfig.c src/dmconfig.c src/langconfig.c src/kbconfig.c src/information.c src/nmconfig.c src/viconfig.c
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

all:

src/scanner.h: src/scanner.re
	cd src; re2c -o $(subst src/,,$@) $(subst src/,,$<)

%.o: %.c src/text.h src/local.h src/scanner.h
	cd src; cc $(CFLAGS) -c $(subst src/,,$<) -o $(subst src/,,$@)

src/re2ctest: src/re2ctest.o
	cc $(LDFLAGS) $^ -o $@

rootfs: bin/create-rootfs bin/fwsetup
	bin/create-rootfs
	touch rootfs

$(ISO): rootfs bin/create-iso bin/resolvegroups
	bin/create-iso

iso: $(ISO)

bin/fwsetup: $(OBJECTS)
	cc $(LDFLAGS) $^ -o $@

bin/resolvegroups: src/resolvegroups.o
	cc $(LDFLAGS) $^ -o $@

setup: bin/fwsetup

install: bin/fwsetup
	mkdir -p $(DESTDIR)/usr/sbin
	install -m 0755 bin/fwsetup $(DESTDIR)/usr/sbin/fwsetup
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/langconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/kbconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/nmconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/rootconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/userconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/hostconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/viconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/dmconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/timeconfig
	ln -s -f fwsetup $(DESTDIR)/usr/sbin/grubconfig

clean:
	rm -rf $(OBJECTS) src/resolvegroups.o bin/resolvegroups bin/fwsetup root vmlinuz initrd mounts rootfs.img squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(ISO) tmp local local.lastupdate sums $(FDB) rootfs fvbe.conf var
else
ROOT   := /usr/lib/syslinux
CFLAGS := -std=gnu99
CFLAGS += -Os
CFLAGS += -Wall -Wextra
ifeq ($(SYSLINUX),bios)
CFLAGS += -m32
CFLAGS += -march=i386
CFLAGS += -mpreferred-stack-boundary=2
endif
ifeq ($(SYSLINUX),efi)
CFLAGS += -m64
CFLAGS += -march=x86-64
CFLAGS += -mpreferred-stack-boundary=4
endif
CFLAGS += -fomit-frame-pointer
CFLAGS += -fno-stack-protector
CFLAGS += -fwrapv
CFLAGS += -freg-struct-return
CFLAGS += -fno-exceptions
CFLAGS += -fno-asynchronous-unwind-tables
CFLAGS += -fPIC
CFLAGS += -falign-functions=0
CFLAGS += -falign-jumps=0
CFLAGS += -falign-labels=0
CFLAGS += -falign-loops=0
CFLAGS += -nostdinc
CFLAGS += -nostdlib
CFLAGS += -I$(ROOT)/com32/include
CFLAGS += -D__COM32__
CFLAGS += -DDYNAMIC_MODULE
endif
