#pragma once

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define le16(x) (x)
#define le32(x) (x)
#else
#define le16(x) __builtin_bswap16(x)
#define le32(x) __builtin_bswap32(x)
#endif

#define EXT2_MAGIC 0xEF53
#define EXT2_NDIRECT 12
#define EXT2_SB_OFFSET 1024

struct ext2_superblock {
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_r_blocks_count;
  uint32_t s_free_blocks_count;
  uint32_t s_free_inodes_count;
  uint32_t s_first_data_block;
  uint32_t s_log_block_size;
  uint32_t s_log_frag_size;
  uint32_t s_blocks_per_group;
  uint32_t s_frags_per_group;
  uint32_t s_inodes_per_group;
  uint32_t s_mtime;
  uint32_t s_wtime;
  uint16_t s_mnt_count;
  uint16_t s_max_mnt_count;
  uint16_t s_magic;
  uint16_t s_state;
  uint16_t s_errors;
  uint16_t s_minor_rev_level;
  uint32_t s_lastcheck;
  uint32_t s_checkinterval;
  uint32_t s_creator_os;
  uint32_t s_rev_level;
  uint16_t s_def_resuid;
  uint16_t s_def_resgid;
  uint32_t s_first_ino;
  uint16_t s_inode_size;
} __attribute__((packed));

struct ext2_group_desc {
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_bitmap;
  uint32_t bg_inode_table;
  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs_count;
  uint16_t bg_pad;
  uint8_t bg_reserved[12];
} __attribute__((packed));

struct ext2_inode {
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size;
  uint32_t i_atime;
  uint32_t i_ctime;
  uint32_t i_mtime;
  uint32_t i_dtime;
  uint16_t i_gid;
  uint16_t i_links_count;
  uint32_t i_blocks;
  uint32_t i_flags;
  uint32_t i_osd1;
  uint32_t i_block[15];
  uint32_t i_generation;
  uint32_t i_file_acl;
  uint32_t i_dir_acl;
  uint32_t i_faddr;
  uint8_t i_osd2[12];
} __attribute__((packed));

struct ext2_dir_entry {
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char name[];
} __attribute__((packed));

typedef struct {
  int fd;
  uint32_t block_size;
  uint32_t inode_size;
  uint32_t inodes_per_group;
  struct ext2_superblock sb;
} Ext2;

static inline Ext2 ext2_open(const char *path) {
  Ext2 e;
  e.fd = open(path, O_RDONLY);
  if (e.fd < 0) {
    perror(path);
    exit(1);
  }
  if (pread(e.fd, &e.sb, sizeof(e.sb), EXT2_SB_OFFSET) !=
      (ssize_t)sizeof(e.sb)) {
    fprintf(stderr, "cannot read superblock\n");
    exit(1);
  }
  if (le16(e.sb.s_magic) != EXT2_MAGIC) {
    fprintf(stderr, "%s: not an ext2 filesystem\n", path);
    exit(1);
  }
  e.block_size = 1024u << le32(e.sb.s_log_block_size);
  e.inodes_per_group = le32(e.sb.s_inodes_per_group);
  e.inode_size = (le32(e.sb.s_rev_level) >= 1) ? le16(e.sb.s_inode_size) : 128;
  return e;
}

static inline void read_block(Ext2 *e, uint32_t bnum, void *buf) {
  if (bnum == 0) {
    memset(buf, 0, e->block_size);
    return;
  }
  if (pread(e->fd, buf, e->block_size, (off_t)bnum * e->block_size) !=
      (ssize_t)e->block_size) {
    fprintf(stderr, "cannot read block %u\n", bnum);
    exit(1);
  }
}

static inline struct ext2_inode read_inode(Ext2 *e, uint32_t inum) {
  uint32_t group = (inum - 1) / e->inodes_per_group;
  uint32_t index = (inum - 1) % e->inodes_per_group;
  uint32_t gdt_block = (e->block_size == 1024) ? 2 : 1;

  struct ext2_group_desc gd;
  off_t gd_off = (off_t)gdt_block * e->block_size + (off_t)group * sizeof(gd);
  if (pread(e->fd, &gd, sizeof(gd), gd_off) != (ssize_t)sizeof(gd)) {
    fprintf(stderr, "cannot read group descriptor\n");
    exit(1);
  }

  struct ext2_inode ino;
  off_t ino_off = (off_t)le32(gd.bg_inode_table) * e->block_size +
                  (off_t)index * e->inode_size;
  if (pread(e->fd, &ino, sizeof(ino), ino_off) != (ssize_t)sizeof(ino)) {
    fprintf(stderr, "cannot read inode %u\n", inum);
    exit(1);
  }
  return ino;
}
