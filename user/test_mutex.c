#include "kernel/types.h"
#include "user.h"

void print_info(int argc, char *argv[]) {
  int pid = getpid();
  for (int i = 0; i < argc; ++i) {
    int str_len = strlen(argv[i]);
    for (int j = 0; j < str_len; ++j) {
      printf("%d: arg %d, char '%c'\n", pid, i, argv[i][j]);
    }
  }
}

int
main(int argc, char *argv[])
{
  int mufd = mutex();
  if (mufd < 0) {
    fprintf(2, "fail to create mutex\n");
    exit(-1);
  }

  int pid = fork();
  int err;

  switch (pid) {
  case -1:
    fprintf(2, "fail to fork\n");
    exit(-1);
  case 0:
    if ((err = mutex_lock(mufd)) == -1) {
      fprintf(2, "fail to lock mutex in child\n");
      exit(-1);
    }

    print_info(argc, argv);

    if ((err = mutex_unlock(mufd)) == -1) {
      fprintf(2, "fail to unlock mutex in child\n");
      exit(-1);
    }
    break;
  default:
    if ((err = mutex_lock(mufd)) == -1) {
      fprintf(2, "fail to lock mutex in parent\n");
      exit(-1);
    }

    print_info(argc, argv);

    if ((err = mutex_unlock(mufd)) == -1) {
      fprintf(2, "fail to unlock mutex in parent\n");
      exit(-1);
    }

    int code;
    wait(&code);
  }

  exit(0);
}
