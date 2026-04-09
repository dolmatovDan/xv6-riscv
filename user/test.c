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
  int pid = fork();

  switch (pid) {
  case -1:
    fprintf(2, "fail to fork\n");
    exit(-1);
  case 0:
    print_info(argc, argv);
    break;
  default:
    print_info(argc, argv);
    int code;
    wait(&code);
  }

  exit(0);
}
