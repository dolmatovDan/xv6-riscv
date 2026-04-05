#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int glob = 1;

int
main(int argc, char *argv[])
{
  // test pte syscall
  printf("[INFO] Initial pte\n");
  pte();

  printf("[INFO] Allocate buffers\n");
  char buf1[512];
  (void)buf1;

  int N = 16 * 1024;
  char *buf2 = malloc(N);
  pte();

  printf("[INFO] Set AD to 0\n");
  int err = change_flag(buf1, sizeof(buf1), 0b11000000);
  if (err == -1) {
    fprintf(2, "fail to change flag\n");
    exit(-1);
  }
  err = change_flag(buf2, N, 0b11000000);
  if (err == -1) {
    fprintf(2, "fail to change flag\n");
    exit(-1);
  }
  pte();

  free(buf2);
  printf("[INFO] Print after free\n");
  pte();

  exit(0);
}
