#!/bin/bash
set -e

IMG=ext2.img
MNT=ext2
BLOCK_SIZE=2048

make

truncate --size 256M $IMG
mkfs.ext2 -b $BLOCK_SIZE $IMG > /dev/null 2>&1

mkdir -p $MNT
sudo mount -t ext2 $IMG $MNT

dd if=/dev/urandom bs=1024 count=64 of=$MNT/bigfile.bin 2>/dev/null

mkdir -p $MNT/dir1/subdir
echo "hello foo" > $MNT/dir1/foo.txt
echo "hello bar" > $MNT/dir1/subdir/bar.txt

truncate --size 5G $MNT/sparse5g.bin

INODE_BIG=$(stat  -c %i $MNT/bigfile.bin)
INODE_DIR1=$(stat -c %i $MNT/dir1)
INODE_S5G=$(stat  -c %i $MNT/sparse5g.bin)
SUM_BIG=$(sha512sum $MNT/bigfile.bin | awk '{print $1}')

sudo umount $MNT

echo "=== ext2info bigfile (inode $INODE_BIG) ==="
./ext2info $IMG $INODE_BIG

echo ""
echo "=== ext2info sparse 5G (inode $INODE_S5G) ==="
./ext2info $IMG $INODE_S5G

echo ""
echo "=== ext2cat bigfile | sha512sum ==="
./ext2cat $IMG $INODE_BIG | sha512sum
echo "expected: $SUM_BIG"

echo ""
echo "=== ext2ls root (inode 2) ==="
./ext2cat $IMG 2 | ./ext2ls

echo ""
echo "=== ext2ls dir1 (inode $INODE_DIR1) ==="
./ext2cat $IMG $INODE_DIR1 | ./ext2ls

echo ""
echo "=== loop device ==="
LOOP=$(sudo losetup -f)
sudo losetup "$LOOP" $IMG
sudo chmod 644 "$LOOP"

losetup -a
lsblk -o name,size,fstype

echo ""
./ext2cat "$LOOP" $INODE_BIG | sha512sum
echo "expected: $SUM_BIG"

echo ""
echo "=== ext2ls dir1 via loop ==="
./ext2cat "$LOOP" $INODE_DIR1 | ./ext2ls

sudo losetup -d "$LOOP"
