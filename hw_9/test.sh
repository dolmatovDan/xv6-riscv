#!/bin/bash
set -e

IMG=ext2.img
MNT=ext2
BLOCK_SIZE=2048
BLOCKS_PER_GROUP=2048

PASS=0
FAIL=0

check_sum() {
    local label=$1 expected=$2 actual=$3
    if [ "$actual" = "$expected" ]; then
        echo "PASS: $label"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $label"
        echo "  expected: $expected"
        echo "  got:      $actual"
        FAIL=$((FAIL + 1))
    fi
}

check_contains() {
    local label=$1 needle=$2 haystack=$3
    if echo "$haystack" | grep -q "$needle"; then
        echo "PASS: $label"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $label (not found: $needle)"
        FAIL=$((FAIL + 1))
    fi
}

make

truncate --size 256M $IMG
mkfs.ext2 -b $BLOCK_SIZE -g $BLOCKS_PER_GROUP $IMG > /dev/null 2>&1

mkdir -p $MNT
sudo mount -t ext2 $IMG $MNT
sudo chown "$(id -u):$(id -g)" $MNT

dd if=/dev/urandom bs=1024 count=64  of=$MNT/bigfile.bin  2>/dev/null
dd if=/dev/urandom bs=1024 count=2048 of=$MNT/dir1file.bin 2>/dev/null

mkdir $MNT/dir1 $MNT/dir2 $MNT/dir3
mkdir $MNT/dir1/subdir

dd if=/dev/urandom bs=1024 count=64 of=$MNT/dir1/file1.bin 2>/dev/null
dd if=/dev/urandom bs=1024 count=64 of=$MNT/dir2/file2.bin 2>/dev/null
echo "nested file" > $MNT/dir1/subdir/nested.txt
echo "dir3 file"   > $MNT/dir3/hello.txt

truncate --size 5G $MNT/sparse5g.bin

INODE_BIG=$(stat   -c %i $MNT/bigfile.bin)
INODE_D1F=$(stat   -c %i $MNT/dir1file.bin)
INODE_DIR1=$(stat  -c %i $MNT/dir1)
INODE_DIR2=$(stat  -c %i $MNT/dir2)
INODE_DIR3=$(stat  -c %i $MNT/dir3)
INODE_F1=$(stat    -c %i $MNT/dir1/file1.bin)
INODE_F2=$(stat    -c %i $MNT/dir2/file2.bin)
INODE_S5G=$(stat   -c %i $MNT/sparse5g.bin)

SUM_BIG=$(sha512sum $MNT/bigfile.bin     | awk '{print $1}')
SUM_D1F=$(sha512sum $MNT/dir1file.bin    | awk '{print $1}')
SUM_F1=$(sha512sum  $MNT/dir1/file1.bin  | awk '{print $1}')
SUM_F2=$(sha512sum  $MNT/dir2/file2.bin  | awk '{print $1}')

sudo umount $MNT

echo "=== ext2info: bigfile (inode $INODE_BIG) ==="
./ext2info $IMG $INODE_BIG

echo ""
echo "=== ext2info: dir1file 2MB (inode $INODE_D1F) ==="
./ext2info $IMG $INODE_D1F

echo ""
echo "=== ext2info: dir2/file2 (inode $INODE_F2, different group) ==="
./ext2info $IMG $INODE_F2

echo ""
echo "=== ext2info: sparse 5G (inode $INODE_S5G) ==="
./ext2info $IMG $INODE_S5G

echo ""
echo "=== checksum tests via image ==="
check_sum "bigfile"       "$SUM_BIG" "$(./ext2cat $IMG $INODE_BIG | sha512sum | awk '{print $1}')"
check_sum "dir1file 2MB"  "$SUM_D1F" "$(./ext2cat $IMG $INODE_D1F | sha512sum | awk '{print $1}')"
check_sum "dir1/file1"    "$SUM_F1"  "$(./ext2cat $IMG $INODE_F1  | sha512sum | awk '{print $1}')"
check_sum "dir2/file2"    "$SUM_F2"  "$(./ext2cat $IMG $INODE_F2  | sha512sum | awk '{print $1}')"

echo ""
echo "=== ext2ls tests ==="
ROOT_LS=$(./ext2cat $IMG 2 | ./ext2ls)
check_contains "root has bigfile.bin"  "bigfile.bin"  "$ROOT_LS"
check_contains "root has dir1"         "dir1"         "$ROOT_LS"
check_contains "root has sparse5g.bin" "sparse5g.bin" "$ROOT_LS"

DIR1_LS=$(./ext2cat $IMG $INODE_DIR1 | ./ext2ls)
check_contains "dir1 has file1.bin" "file1.bin" "$DIR1_LS"
check_contains "dir1 has subdir"    "subdir"    "$DIR1_LS"

DIR2_LS=$(./ext2cat $IMG $INODE_DIR2 | ./ext2ls)
check_contains "dir2 has file2.bin" "file2.bin" "$DIR2_LS"

DIR3_LS=$(./ext2cat $IMG $INODE_DIR3 | ./ext2ls)
check_contains "dir3 has hello.txt" "hello.txt" "$DIR3_LS"

echo ""
echo "=== loop device ==="
LOOP=$(sudo losetup -f)
sudo losetup "$LOOP" $IMG
sudo chmod 644 "$LOOP"

losetup -a
lsblk -o name,size,fstype

check_sum "bigfile via loop"    "$SUM_BIG" "$(./ext2cat "$LOOP" $INODE_BIG | sha512sum | awk '{print $1}')"
check_sum "dir2/file2 via loop" "$SUM_F2"  "$(./ext2cat "$LOOP" $INODE_F2  | sha512sum | awk '{print $1}')"

LOOP_DIR1=$(./ext2cat "$LOOP" $INODE_DIR1 | ./ext2ls)
check_contains "loop: dir1 has file1.bin" "file1.bin" "$LOOP_DIR1"

sudo losetup -d "$LOOP"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
