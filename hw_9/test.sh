#!/bin/bash
set -e

IMG=ext2.img
MNT=mnt
BLOCK_SIZE=2048

make

truncate --size 256M $IMG
mkfs.ext2 -b $BLOCK_SIZE $IMG > /dev/null 2>&1

mkdir -p $MNT
sudo mount -t ext2 $IMG $MNT

dd if=/dev/urandom bs=1024 count=64 of=$MNT/bigfile.bin 2>/dev/null
truncate --size 50M  $MNT/sparse50m.bin
truncate --size 700M $MNT/sparse700m.bin
mkdir -p $MNT/dir1/subdir
echo "hello foo" > $MNT/dir1/foo.txt
echo "hello bar" > $MNT/dir1/subdir/bar.txt

INODE_BIG=$(stat    -c %i $MNT/bigfile.bin)
INODE_DIR1=$(stat   -c %i $MNT/dir1)
INODE_S50=$(stat    -c %i $MNT/sparse50m.bin)
INODE_S700=$(stat   -c %i $MNT/sparse700m.bin)
SUM_BIG=$(sha512sum $MNT/bigfile.bin | awk '{print $1}')
SUM_S50=$(sha512sum $MNT/sparse50m.bin | awk '{print $1}')

sudo umount $MNT

echo "=== ext2info bigfile ==="
./ext2info $IMG $INODE_BIG

echo "=== ext2info sparse 50M ==="
./ext2info $IMG $INODE_S50

echo "=== ext2info sparse 700M ==="
./ext2info $IMG $INODE_S700

echo "=== ext2cat bigfile | sha512sum ==="
./ext2cat $IMG $INODE_BIG | sha512sum
echo "expected: $SUM_BIG"

echo "=== ext2cat sparse50m | sha512sum ==="
./ext2cat $IMG $INODE_S50 | sha512sum
echo "expected: $SUM_S50"

echo "=== ext2ls root (inode 2) ==="
./ext2cat $IMG 2 | ./ext2ls

echo "=== ext2ls dir1 ==="
./ext2cat $IMG $INODE_DIR1 | ./ext2ls

echo "=== loop device ==="
LOOP=$(sudo losetup -f)
sudo losetup "$LOOP" $IMG
sudo chmod 644 "$LOOP"
./ext2cat "$LOOP" $INODE_BIG | sha512sum
echo "expected: $SUM_BIG"
./ext2cat "$LOOP" $INODE_DIR1 | ./ext2ls
sudo losetup -d "$LOOP"
