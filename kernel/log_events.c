#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "log_events.h"

static struct {
  struct spinlock lock;
  int classes;
  uint until;
} ls;

static volatile int ls_fast;

void
loginit(void)
{
  initlock(&ls.lock, "log_events");
}

void
setlog_kernel(int classes, int nticks)
{
  uint deadline = 0;
  if (classes != 0 && nticks > 0) {
    acquire(&tickslock);
    deadline = ticks + nticks;
    release(&tickslock);
  }
  acquire(&ls.lock);
  ls.classes = classes;
  ls.until = deadline;
  ls_fast = classes;
  release(&ls.lock);
}

int
should_log(int class)
{
  if (!(ls_fast & class))
    return 0;

  acquire(&tickslock);
  uint t = ticks;
  release(&tickslock);

  acquire(&ls.lock);
  int result = 0;
  if (ls.classes & class) {
    if (ls.until == 0 || t < ls.until) {
      result = 1;
    } else {
      ls.classes = 0;
      ls.until = 0;
      ls_fast = 0;
    }
  }
  release(&ls.lock);
  return result;
}
