#include <sys/stat.h>
#include <time.h>
#include "ext2.h"

static void
fmt_time(uint32_t t)
{
    time_t ts = (time_t)t;
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&ts));
    printf("%s", buf);
}

static void
fmt_mode(uint16_t mode)
{
    char t;
    if      (S_ISREG(mode))  t = '-';
    else if (S_ISDIR(mode))  t = 'd';
    else if (S_ISLNK(mode))  t = 'l';
    else if (S_ISCHR(mode))  t = 'c';
    else if (S_ISBLK(mode))  t = 'b';
    else if (S_ISFIFO(mode)) t = 'p';
    else if (S_ISSOCK(mode)) t = 's';
    else                     t = '?';
    printf("%c%c%c%c%c%c%c%c%c%c",
        t,
        (mode & S_IRUSR) ? 'r' : '-', (mode & S_IWUSR) ? 'w' : '-', (mode & S_IXUSR) ? 'x' : '-',
        (mode & S_IRGRP) ? 'r' : '-', (mode & S_IWGRP) ? 'w' : '-', (mode & S_IXGRP) ? 'x' : '-',
        (mode & S_IROTH) ? 'r' : '-', (mode & S_IWOTH) ? 'w' : '-', (mode & S_IXOTH) ? 'x' : '-');
}

static void
show_ind_ptrs(Ext2 *e, uint32_t bnum, uint8_t *buf)
{
    if (bnum == 0) return;
    read_block(e, bnum, buf);
    uint32_t *p = (uint32_t *)buf;
    uint32_t  P = e->block_size / 4;
    int cnt = 0;
    printf(" ->");
    for (uint32_t i = 0; i < P; i++) {
        if (!le32(p[i])) continue;
        if (cnt < 6) printf(" %u", le32(p[i]));
        else if (cnt == 6) printf(" ...");
        cnt++;
    }
    if (cnt == 0) printf(" (sparse)");
}

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: ext2info <image> <inode>\n");
        return 1;
    }

    Ext2 e = ext2_open(argv[1]);
    uint32_t inum = (uint32_t)atoi(argv[2]);
    struct ext2_inode ino = read_inode(&e, inum);

    uint16_t mode = le16(ino.i_mode);
    uint64_t size = le32(ino.i_size);
    if (le32(e.sb.s_rev_level) >= 1 && S_ISREG(mode))
        size |= (uint64_t)le32(ino.i_dir_acl) << 32;

    printf("Inode:       %u\n", inum);
    printf("Mode:        "); fmt_mode(mode); printf("  (%04o)\n", mode & 07777);
    printf("UID/GID:     %u/%u\n", le16(ino.i_uid), le16(ino.i_gid));
    printf("Size:        %llu\n", (unsigned long long)size);
    printf("Links:       %u\n", le16(ino.i_links_count));
    printf("Blocks:      %u  (512B units)\n", le32(ino.i_blocks));
    printf("Flags:       0x%08x\n", le32(ino.i_flags));
    printf("atime:       "); fmt_time(le32(ino.i_atime)); printf("\n");
    printf("ctime:       "); fmt_time(le32(ino.i_ctime)); printf("\n");
    printf("mtime:       "); fmt_time(le32(ino.i_mtime)); printf("\n");
    printf("dtime:       "); fmt_time(le32(ino.i_dtime)); printf("\n");

    uint8_t *buf = malloc(e.block_size);

    printf("\nBlock addresses:\n");
    for (int i = 0; i < EXT2_NDIRECT; i++)
        printf("  direct[%2d]  = %u\n", i, le32(ino.i_block[i]));

    printf("  indirect    = %u", le32(ino.i_block[12]));
    show_ind_ptrs(&e, le32(ino.i_block[12]), buf);
    printf("\n");

    printf("  dindirect   = %u", le32(ino.i_block[13]));
    show_ind_ptrs(&e, le32(ino.i_block[13]), buf);
    printf("\n");

    printf("  tindirect   = %u", le32(ino.i_block[14]));
    show_ind_ptrs(&e, le32(ino.i_block[14]), buf);
    printf("\n");

    free(buf);
    close(e.fd);
    return 0;
}
