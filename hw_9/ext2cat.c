#include "ext2.h"
#include <errno.h>
#include <sys/stat.h>

typedef struct {
  Ext2 *e;
  uint64_t remaining;
  uint8_t *dbuf;
} Writer;

static void write_all(const void *buf, size_t n) {
  const uint8_t *p = buf;
  while (n > 0) {
    ssize_t r = write(STDOUT_FILENO, p, n);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      perror("write");
      exit(1);
    }
    p += r;
    n -= r;
  }
}

static void emit(Writer *w, uint32_t bnum) {
  if (w->remaining == 0)
    return;
  size_t n = (w->remaining < w->e->block_size) ? (size_t)w->remaining
                                               : (size_t)w->e->block_size;
  if (bnum == 0) {
    memset(w->dbuf, 0, n);
  } else {
    if (pread(w->e->fd, w->dbuf, w->e->block_size,
              (off_t)bnum * w->e->block_size) < 0) {
      perror("pread");
      exit(1);
    }
  }
  write_all(w->dbuf, n);
  w->remaining -= n;
}

static void emit_indirect(Writer *w, uint32_t bnum, uint32_t *ibuf) {
  uint32_t P = w->e->block_size / 4;
  if (bnum == 0) {
    for (uint32_t i = 0; i < P && w->remaining > 0; i++)
      emit(w, 0);
    return;
  }
  read_block(w->e, bnum, ibuf);
  for (uint32_t i = 0; i < P && w->remaining > 0; i++)
    emit(w, le32(ibuf[i]));
}

static void emit_dindirect(Writer *w, uint32_t bnum, uint32_t *ib1,
                           uint32_t *ib2) {
  uint32_t P = w->e->block_size / 4;
  if (bnum == 0) {
    for (uint32_t i = 0; i < P && w->remaining > 0; i++)
      emit_indirect(w, 0, ib2);
    return;
  }
  read_block(w->e, bnum, ib1);
  for (uint32_t i = 0; i < P && w->remaining > 0; i++)
    emit_indirect(w, le32(ib1[i]), ib2);
}

static void emit_tindirect(Writer *w, uint32_t bnum, uint32_t *ib1,
                           uint32_t *ib2, uint32_t *ib3) {
  uint32_t P = w->e->block_size / 4;
  if (bnum == 0) {
    for (uint32_t i = 0; i < P && w->remaining > 0; i++)
      emit_dindirect(w, 0, ib2, ib3);
    return;
  }
  read_block(w->e, bnum, ib1);
  for (uint32_t i = 0; i < P && w->remaining > 0; i++)
    emit_dindirect(w, le32(ib1[i]), ib2, ib3);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: ext2cat <image> <inode>\n");
    return 1;
  }

  Ext2 e = ext2_open(argv[1]);
  uint32_t inum = (uint32_t)atoi(argv[2]);
  struct ext2_inode ino = read_inode(&e, inum);

  uint16_t mode = le16(ino.i_mode);
  uint64_t size = le32(ino.i_size);
  if (le32(e.sb.s_rev_level) >= 1 && S_ISREG(mode))
    size |= (uint64_t)le32(ino.i_dir_acl) << 32;

  Writer w;
  w.e = &e;
  w.remaining = size;
  w.dbuf = malloc(e.block_size);

  uint32_t *ib1 = malloc(e.block_size);
  uint32_t *ib2 = malloc(e.block_size);
  uint32_t *ib3 = malloc(e.block_size);

  for (int i = 0; i < EXT2_NDIRECT && w.remaining > 0; i++)
    emit(&w, le32(ino.i_block[i]));

  if (w.remaining > 0)
    emit_indirect(&w, le32(ino.i_block[12]), ib1);
  if (w.remaining > 0)
    emit_dindirect(&w, le32(ino.i_block[13]), ib1, ib2);
  if (w.remaining > 0)
    emit_tindirect(&w, le32(ino.i_block[14]), ib1, ib2, ib3);

  free(w.dbuf);
  free(ib1);
  free(ib2);
  free(ib3);
  close(e.fd);
  return 0;
}
