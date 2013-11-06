ifdef SYSLINUX
CFLAGS := $(shell syslinux-module-config $(SYSLINUX))
TARGET := fvbe.$(SYSLINUX)
endif

$(TARGET): fvbe.c
	cc $(CFLAGS) $< -o $@
	strip --strip-debug --strip-unneeded $@
