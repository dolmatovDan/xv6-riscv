#include "types.h"
#include "memlayout.h"

#define LowReg (*((volatile uint32 *)(TIMER_LOW)))
#define HighReg (*((volatile uint32 *)(TIMER_HIGH)))

uint64
get_time(void)
{
  uint64 res = LowReg + ((uint64)HighReg << 32);
  return res;
}
