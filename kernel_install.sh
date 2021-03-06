#!/bin/sh

#for building a new kernel. -Tim
#
#Steps:
# 1. ensure FSTAB_PATH exists with a legit fstab. this is what wolf gave us:
#       # HEADER: This file was autogenerated at Mon Sep 28 22:21:27 -0400 2009
#       # HEADER: by puppet.  While it can still be managed manually, it
#       # HEADER: is definitely not recommended.
#       tmpfs	/dev/shm	tmpfs	defaults	0	0
#       devpts	/dev/pts	devpts	gid=5,mode=620	0	0
#       sysfs	/sys	sysfs	defaults	0	0
#       proc	/proc	proc	defaults	0	0
#       zefram:/export/factor_homes	/nethome	nfs	defaults,intr	0	0
# 2. extract a kernel
# 3. in the kernel's toplevel Makefile, give EXTRAVERSION a descriptive unique name
#     this will effect the KERNEL variable below
# 4. place this file inside the extracted kernel directory
# 5. run this as root
# 6. don't worry about the errors
# 7. edit grub.conf with a new entry, like this:
#       title My Project #1
#       root (hd0,0)
#       kernel /vmlinuz-X.Y.Z-whatever ro
#       initrd /initrd-X.Y.Z-whatever.img

KERNEL=2.6.24.6-lfrng
FSTAB_PATH=/nethome/tmartin9/kernels/fstab
FACTOR=factor004

make && make modules

make modules_install install

/sbin/mkinitrd --rootfs nfs --omit-scsi-modules --fstab=$FSTAB_PATH --rootdev 130.207.21.7:/export/$FACTOR -f /boot/initrd-$KERNEL.img $KERNEL

#cleaning out an old/unneeded kernel can be done as follows:
# 1. make distclean in the build dir (not sure if this is needed)
# 2. rm -r /lib/modules/X.Y.Z-whatever
# 3. rm /boot/initrd-X.Y.Z-whatever.img
# 4. rm /boot/vmlinuz-X.Y.Z-whatever
# 5. remove the grub.conf entry
