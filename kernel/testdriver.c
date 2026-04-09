#include "types.h"
#include "testdriver.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "riscv.h"
#include "defs.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

struct {
  struct spinlock lock;
  uint8 seed;
  uint8 a, b;
} urand;

struct {
  struct spinlock lock;
  uint64 cnt;
} nullstat;


static uint8 next_urand() {
  return urand.seed = urand.a * urand.seed + urand.b;
}

int
testdriverread(int user_dst, uint64 dst, int n, int minor)
{
  uint64 cnt, r = 0;

  static char buf[32] = {0};

  switch (minor) {
  case TD_NULL:
    return 0;
  case TD_ZERO:
    r = n;
    while (n) {
        cnt = min(n, sizeof(buf));

        if (either_copyout(user_dst, dst, buf, cnt)) {
          return -1;
        }

        dst += cnt;
        n -= cnt;
    }
    return r;
  case TD_URANDOM:
    acquire(&urand.lock);
    r = n;

    while (n) {
        cnt = min(n, sizeof(buf));
        for (int i = 0; i < cnt; ++i) {
          buf[i] = next_urand();
        }

        if (either_copyout(user_dst, dst, buf, cnt)) {
          release(&urand.lock);
          return -1;
        }

        dst += cnt;
        n -= cnt;
    }
    release(&urand.lock);

    return r;
  case TD_NULLSTAT:
    if (n != sizeof(cnt)) {
        return -1;
    }

    acquire(&nullstat.lock);
    cnt = nullstat.cnt;
    release(&nullstat.lock);
    if (either_copyout(user_dst, dst, &cnt, sizeof(cnt)) == -1) {
      return -1;
    }

    return sizeof(cnt);
  }
  return 0;
}

int
testdriverwrite(int user_src, uint64 src, int n, int minor)
{
  switch (minor) {
  case TD_NULL:
    return n;
  case TD_ZERO:
    return -1;
  case TD_URANDOM:
    if (n != sizeof(urand.seed)) {
      return -1;
    }

    acquire(&urand.lock);
    if (either_copyin(&urand.seed, user_src, src, n) == -1) {
      release(&urand.lock);
      return -1;
    }

    release(&urand.lock);
    return n;
  case TD_NULLSTAT:
    acquire(&nullstat.lock);
    nullstat.cnt += n;
    release(&nullstat.lock);
    return n;
  }
  return 0;
}

void
testdriverinit(void)
{
  initlock(&urand.lock, "urandom lock");
  urand.a = 123;
  urand.b = 65;
  urand.seed = 13;

  initlock(&nullstat.lock, "nullstat lock");
  nullstat.cnt = 0;

  devsw[TESTDRIVER].read = testdriverread;
  devsw[TESTDRIVER].write = testdriverwrite;
}
