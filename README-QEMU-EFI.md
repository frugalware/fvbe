## testing EFI with qemu

1.  build an new ISO if you don't have one already
2.  install ofc qemu and create an hdd ( see qemu-img )
3.  create a testing dir:
    mkdir -p ~/qemu-efi/efi-bios && cd ~/qemu-efi/efi-bios
4.  download the zip and unpack in:
     [OVMF-X64-r15214.zip](https://sourceforge.net/projects/edk2/files/OVMF/OVMF-X64-r15214.zip)
5.  cd ~/qemu-efi and run qemu like this :

    qemu -bios efi-bios/OVMF.fd -cdrom YOUR_ISO -hda YOUR_QEMU_HDD -boot d -smp 2 -m 2048 -enable-kvm

You may see some warnings from DMI , ignore these ..

Install now fw and once ready remove -cdrom from qemu cmd ( or set to none ).

You should now have fw installed an running in qemu in efi mode
