#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/log_events.h"

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "usage: log off|all|syscall|irq|proc|exec [ticks]\n");
    exit(1);
  }

  int classes = 0;
  int nticks = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "off") == 0) {
      setlog(0, 0);
      exit(0);
    } else if (strcmp(argv[i], "all") == 0) {
      classes = LOG_SYSCALL | LOG_IRQ | LOG_PROC | LOG_EXEC;
    } else if (strcmp(argv[i], "syscall") == 0) {
      classes |= LOG_SYSCALL;
    } else if (strcmp(argv[i], "irq") == 0) {
      classes |= LOG_IRQ;
    } else if (strcmp(argv[i], "proc") == 0) {
      classes |= LOG_PROC;
    } else if (strcmp(argv[i], "exec") == 0) {
      classes |= LOG_EXEC;
    } else {
      nticks = atoi(argv[i]);
    }
  }

  setlog(classes, nticks);
  exit(0);
}
