#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_pte(void)
{
  struct proc *p = myproc();
  vmprint(p->pagetable, 0);
  return 0;
}

uint64
sys_change_flag(void)
{
  uint64 buf, len;
  int mask;
  pagetable_t pt = myproc()->pagetable;
  argaddr(0, &buf);
  argaddr(1, &len);
  argint(2, &mask);

  if (mask >> 8) {
    return -1;
  }

  if (mask & ~(PTE_D | PTE_A)) {
    return -1;
  }

  uint64 va;
  int n;

  while (len > 0) {
    va = PGROUNDDOWN(buf);
    pte_t *pte = walk(pt, va, 0);
    if (pte == 0 || !(*pte & PTE_V) || !(*pte & PTE_U))
      return -1;
    n = PGSIZE - (buf - va);
    if (n > len) n = len;
    buf += n;
    len -= n;
    *pte &= ~mask;
  }
  sfence_vma();

  return 0;
}

uint64
sys_check_flag(void)
{
  uint64 buf, len, mask;
  pagetable_t pt = myproc()->pagetable;
  argaddr(0, &buf);
  argaddr(1, &len);
  argint(2, (int*)&mask);

  if (mask >> 8) {
    return -1;
  }

  uint64 va;
  int n;

  int ans = 0;

  while (len > 0) {
    va = PGROUNDDOWN(buf);
    pte_t *pte = walk(pt, va, 0);
    if (!pte || !(*pte & PTE_V) || !(*pte & PTE_U))
      return -1;

    if (*pte & mask) {
      ans = 1;
    }

    n = PGSIZE - (buf - va);
    if (n > len) n = len;
    buf += n;
    len -= n;
  }

  return ans;
}
