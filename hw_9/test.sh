#!/bin/bash
set -e

PASS=0
FAIL=0
IMG=ext2.img
MNT=mnt
BLOCK_SIZE=2048

check() {
    if [ "$1" = "$2" ]; then
        echo "PASS: $3"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $3"
        echo "      expected: $2"
        echo "      got:      $1"
        FAIL=$((FAIL + 1))
    fi
}

check_contains() {
    if echo "$1" | grep -q "$2"; then
        echo "PASS: $3"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $3 (pattern '$2' not found)"
        FAIL=$((FAIL + 1))
    fi
}

echo "=== building ==="
make -C . > /dev/null

echo "=== creating image ==="
truncate --size 256M $IMG
mkfs.ext2 -b $BLOCK_SIZE $IMG > /dev/null 2>&1

echo "=== mounting and populating ==="
mkdir -p $MNT
sudo mount -t ext2 $IMG $MNT

dd if=/dev/urandom bs=1024 count=128 of=$MNT/bigfile.bin 2>/dev/null

mkdir -p $MNT/dir1/subdir
echo "content of foo" > $MNT/dir1/foo.txt
echo "content of bar" > $MNT/dir1/subdir/bar.txt
echo "hello from root" > $MNT/hello.txt

truncate --size 50M  $MNT/sparse50m.bin
truncate --size 700M $MNT/sparse700m.bin

INODE_BIG=$(stat    -c %i $MNT/bigfile.bin)
INODE_DIR1=$(stat   -c %i $MNT/dir1)
INODE_SPARSE50=$(stat  -c %i $MNT/sparse50m.bin)
INODE_SPARSE700=$(stat -c %i $MNT/sparse700m.bin)
SIZE_SPARSE50=$(stat  -c %s $MNT/sparse50m.bin)
SIZE_SPARSE700=$(stat -c %s $MNT/sparse700m.bin)

SUM_BIG=$(sha512sum $MNT/bigfile.bin | awk '{print $1}')
SUM_SPARSE50=$(sha512sum $MNT/sparse50m.bin | awk '{print $1}')

sudo umount $MNT

echo "=== testing ext2cat ==="
SUM_CAT=$(./ext2cat $IMG $INODE_BIG | sha512sum | awk '{print $1}')
check "$SUM_CAT" "$SUM_BIG" "ext2cat bigfile checksum"

SUM_CAT50=$(./ext2cat $IMG $INODE_SPARSE50 | sha512sum | awk '{print $1}')
check "$SUM_CAT50" "$SUM_SPARSE50" "ext2cat sparse50m checksum"

echo "=== testing ext2info ==="
INFO_BIG=$(./ext2info $IMG $INODE_BIG)
check_contains "$INFO_BIG" "Inode:" "ext2info inode field"
check_contains "$INFO_BIG" "indirect" "ext2info indirect block field"

INFO_SPARSE50=$(./ext2info $IMG $INODE_SPARSE50)
GOT_SIZE50=$(echo "$INFO_SPARSE50" | awk '/^Size:/{print $2}')
check "$GOT_SIZE50" "$SIZE_SPARSE50" "ext2info sparse50m size"

INFO_SPARSE700=$(./ext2info $IMG $INODE_SPARSE700)
GOT_SIZE700=$(echo "$INFO_SPARSE700" | awk '/^Size:/{print $2}')
check "$GOT_SIZE700" "$SIZE_SPARSE700" "ext2info sparse700m size"

echo "=== testing ext2ls ==="
LISTING=$(./ext2cat $IMG $INODE_DIR1 | ./ext2ls)
check_contains "$LISTING" "foo.txt" "ext2ls dir1 contains foo.txt"
check_contains "$LISTING" "subdir"  "ext2ls dir1 contains subdir"

ROOT_LISTING=$(./ext2cat $IMG 2 | ./ext2ls)
check_contains "$ROOT_LISTING" "dir1"    "ext2ls root contains dir1"
check_contains "$ROOT_LISTING" "hello.txt" "ext2ls root contains hello.txt"

echo "=== testing via loop device ==="
LOOP=$(sudo losetup -f)
sudo losetup "$LOOP" $IMG
sudo chmod 644 "$LOOP"

SUM_LOOP=$(./ext2cat "$LOOP" $INODE_BIG | sha512sum | awk '{print $1}')
check "$SUM_LOOP" "$SUM_BIG" "ext2cat loopdev checksum"

LOOP_LISTING=$(./ext2cat "$LOOP" $INODE_DIR1 | ./ext2ls)
check_contains "$LOOP_LISTING" "foo.txt" "ext2ls loopdev foo.txt"

sudo losetup -d "$LOOP"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
