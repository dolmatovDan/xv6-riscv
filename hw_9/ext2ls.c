#include "ext2.h"

int
main(void)
{
    size_t   cap = 4096, len = 0;
    uint8_t *buf = malloc(cap);

    ssize_t n;
    while ((n = read(STDIN_FILENO, buf + len, cap - len)) > 0) {
        len += (size_t)n;
        if (len == cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (!buf) { perror("realloc"); return 1; }
        }
    }

    size_t off = 0;
    while (off + sizeof(struct ext2_dir_entry) <= len) {
        struct ext2_dir_entry *ent = (struct ext2_dir_entry *)(buf + off);
        uint16_t rec = le16(ent->rec_len);
        if (rec == 0) break;
        if (le32(ent->inode) != 0)
            printf("%7u  %.*s\n", le32(ent->inode), (int)ent->name_len, ent->name);
        off += rec;
    }

    free(buf);
    return 0;
}
