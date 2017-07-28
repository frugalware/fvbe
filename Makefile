include fvbe.conf

ARCH    := x86_64
RELEASE := $(subst ",,$(FVBE_ISO_RELEASE))
TYPE    := $(subst ",,$(FVBE_ISO_TYPE))

ISO     := frugalware-$(RELEASE)-$(TYPE)-$(ARCH).iso


$(ISO): bin/fvbe-make-iso
	bin/fvbe-make-iso

iso: $(ISO)

clean:
	rm -rf root vmlinuz-* initrd-* rootfs.img squashfs.img pacman-g2.conf locales layouts unicode.pf2 $(ISO) *.fdb var *.cfg
