#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int glob = 1;

int
main(int argc, char *argv[])
{
  // test pte syscall
  pte();

  char buf1[512];
  (void)buf1;
  char *buf2 = malloc(16 * 1024);
  pte();

  int err = change_flag(buf1, sizeof(buf1), 0b11000000);
  if (err == -1) {
    fprintf(2, "fail to change flag\n");
    exit(-1);
  }
  pte();

  free(buf2);
  exit(0);
}
