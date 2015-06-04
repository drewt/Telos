#!/bin/bash

#
# mkfs.sh: Create an ext2 filesystem image from a directory.
#
# The created image may be loaded as a boot module and mounted in Telos.
#

if [[ -z "$1"  || -z $2 ]]
then
	echo "usage: $0 <src> <dst>"
	exit 1
fi

if [[ `file -b "$1"` != "directory" ]]
then
	echo "$0: $1 is not a directory"
	exit 1
fi

if [[ "`id -u`" != "0" ]]
then
	echo "$0: this script must be run as root"
	exit 1
fi

# calculate size of disk image
FUDGE=1000
SIZE=`du "$1" | awk '{ print $1 }'`
SIZE=`echo "$SIZE $FUDGE + p" | dc`

dd if=/dev/zero of="$2" count=$SIZE \
	&& mkfs.ext2 -r 0 "$2" \
	&& mount -o loop "$2" scripts/mnt \
	&& cp -r "$1"/* scripts/mnt/

sleep 1
umount scripts/mnt/
