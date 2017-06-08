include fvbe.conf

ARCH    := x86_64
RELEASE := $(subst ",,$(FVBE_ISO_RELEASE))
TYPE    := $(subst ",,$(FVBE_ISO_TYPE))

ISO     := fvbe-$(RELEASE)-$(TYPE)-$(ARCH).iso

rootfs: bin/create-rootfs
	bin/create-rootfs
	touch rootfs

$(ISO): rootfs bin/create-iso
	bin/create-iso

iso: $(ISO)

clean:
	rm -rf root vmlinuz initrd mounts rootfs.img squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(ISO) tmp local local.lastupdate sums $(FDB) rootfs  var
