#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
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

  // test check_flag on glob
  change_flag(&glob, sizeof(glob), PTE_A | PTE_D);
  int tmp = glob; (void)tmp;
  printf("[INFO] glob after read: A=%d D=%d\n", check_flag(&glob, sizeof(glob), PTE_A), check_flag(&glob, sizeof(glob), PTE_D));
  glob = 2;
  printf("[INFO] glob after write: A=%d D=%d\n", check_flag(&glob, sizeof(glob), PTE_A), check_flag(&glob, sizeof(glob), PTE_D));

  // test single stack var
  int x = 5;
  err = change_flag(&x, sizeof(x), PTE_A | PTE_D);
  if (err != -1) {
    fprintf(2, "fail to change flag\n");
    exit(-1);
  }
  tmp = x;
  (void)tmp;
  printf("[INFO] x after read: A=%d D=%d\n", check_flag(&x, sizeof(x), PTE_A), check_flag(&x, sizeof(x), PTE_D));
  x = 10;
  printf("[INFO] x after write: A=%d D=%d\n", check_flag(&x, sizeof(x), PTE_A), check_flag(&x, sizeof(x), PTE_D));

  free(buf2);
  printf("[INFO] Print after free\n");
  pte();

  exit(0);
}
