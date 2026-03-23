#pragma once
#include "kernel/proc.h"

struct procinfo {
  int pid, ppid;
  char name[16];
  char pname[16];
  enum procstate state;
};
