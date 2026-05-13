#include <stdarg.h>
#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

static struct {
  struct spinlock lock;
  char buf[BUFSIZE];
  int head, tail;
} db;

void
diagbuf_init()
{
  initlock(&db.lock, "diagbuf lock");
  db.head = 0;
  db.buf[0] = '\n';
  db.tail = 1;
}

// should acquire lock for using
void
diagbuf_write(int byte)
{
  int size = BUFSIZE;
  if (db.tail - db.head == size)
    db.head++;
  db.buf[db.tail % size] = byte;
  db.tail++;
}

int
diagbuf_copyout(pagetable_t pt, uint64 dst, int n)
{
  if (n <= 0)
    return 0;

  acquire(&db.lock);

  int size = BUFSIZE;
  int start = db.head;
  int end = db.tail;
  int len = end - start;

  for (int i = 0; i < len; i++) {
    if (db.buf[(start + i) % size] == '\n') {
      start = start + i + 1;
      break;
    }
  }

  int copy_len = end - start;
  if (copy_len < 0) copy_len = 0;
  if (copy_len > n - 1) copy_len = n - 1;

  int off = start % size;
  int first = size - off;
  int ok;

  if (first >= copy_len) {
    ok = (copyout(pt, dst, db.buf + off, copy_len) == 0);
  } else {
    ok = (copyout(pt, dst, db.buf + off, first) == 0) &&
         (copyout(pt, dst + first, db.buf, copy_len - first) == 0);
  }

  char nul = '\0';
  if (ok)
    ok = (copyout(pt, dst + copy_len, &nul, 1) == 0);

  release(&db.lock);
  return ok ? copy_len : -1;
}

void
pr_msg(const char *fmt, ...)
{
  acquire(&db.lock);

  acquire(&tickslock);
  uint t = ticks;
  release(&tickslock);

  diagbuf_write('[');
  char tbuf[20];
  int i = 0;
  if (t == 0) {
    tbuf[i++] = '0';
  } else {
    while (t > 0) { tbuf[i++] = '0' + t % 10; t /= 10; }
    for (int l = 0, r = i - 1; l < r; l++, r--) {
      char tmp = tbuf[l]; tbuf[l] = tbuf[r]; tbuf[r] = tmp;
    }
  }
  for (int j = 0; j < i; j++) diagbuf_write(tbuf[j]);
  diagbuf_write(']');
  diagbuf_write(' ');

  va_list ap;
  va_start(ap, fmt);
  vprintf_to(diagbuf_write, (char*)fmt, ap);
  va_end(ap);

  diagbuf_write('\n');

  release(&db.lock);
}
