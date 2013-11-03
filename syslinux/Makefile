ROOT   := /usr/lib/syslinux
CFLAGS := -std=gnu99
CFLAGS += -Os
CFLAGS += -Wall -Wextra
ifeq ($(SYSLINUX),bios)
CFLAGS += -m32
CFLAGS += -march=i386
CFLAGS += -mpreferred-stack-boundary=2
CFLAGS += -Wl,-m,elf_i386
CFLAGS += -Wl,-T,i386.ld
CFLAGS += -L$(ROOT)
CFLAGS += -mregparm=3
TARGET := fvbe.bios
endif
ifeq ($(SYSLINUX),efi)
CFLAGS += -m64
CFLAGS += -march=x86-64
CFLAGS += -mpreferred-stack-boundary=4
CFLAGS += -Wl,-m,elf_x86_64
CFLAGS += -Wl,-T,x86_64.ld
CFLAGS += -L$(ROOT)/efi64
TARGET := fvbe.efi
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
CFLAGS += -nodefaultlibs
CFLAGS += -iwithprefix
CFLAGS += include
CFLAGS += -Iinclude/libmenu
CFLAGS += -Iinclude/libgpl
CFLAGS += -Iinclude/libcom32
CFLAGS += -Iinclude/libutil
CFLAGS += -Iinclude/libcore
CFLAGS += -Iinclude/libobj
CFLAGS += -D__COM32__
CFLAGS += -DDYNAMIC_MODULE
CFLAGS += -Wl,-shared
CFLAGS += -Wl,--hash-style=gnu
#CFLAGS += -Wl,--as-needed
CFLAGS += -l:libmenu.c32
CFLAGS += -l:libgpl.c32
CFLAGS += -l:libcom32.c32
CFLAGS += -l:libutil.c32

$(TARGET): fvbe.c
	cc $(CFLAGS) $< -o $@
