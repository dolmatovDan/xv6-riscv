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
    printf("fail to kalloc sleeplock\n");
    goto bad;
  }

  (*f)->type = FD_MUTEX;

  printf("allocating mutex\n");
  initsleeplock((*f)->lock, "mutex lock");

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
  printf("closing mutex\n");
  kfree((char*)(f->lock));
  printf("free mutex succesfully\n");
  return 0;
}
