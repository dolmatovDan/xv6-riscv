#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

int
mutexalloc(struct file **f)
{
  if ((*f = filealloc()) == 0) {
    goto bad;
  }

  if (((*f)->lock = (struct sleeplock*)kalloc()) == 0) {
    printf("[KERNEL] fail to kalloc sleeplock\n");
    goto bad;
  }

  (*f)->type = FD_MUTEX;

  initsleeplock((*f)->lock, "mutex");
  printf("[KERNEL] allocating mutex %p\n", (*f)->lock);

  return 0;

 bad:
  if (*f && (*f)->lock)
    kfree((char*)((*f)->lock));
  if (*f)
    fileclose(*f);
  return -1;
}

int
mutexclose(struct file *f)
{
  printf("[KERNEL] closing mutex %p\n", f->lock);
  kfree((char*)(f->lock));
  printf("[KERNEL] free mutex %p succesfully\n", f->lock);
  return 0;
}
