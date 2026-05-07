#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DMESG_BUF 16384

int
main(void)
{
  char *buf = malloc(DMESG_BUF);
  if (buf == 0) {
    fprintf(2, "dmesg: malloc failed\n");
    exit(1);
  }
  int n = dmesg(buf, DMESG_BUF);
  if (n < 0) {
    fprintf(2, "dmesg: failed\n");
    exit(1);
  }
  printf("%s", buf);
  free(buf);
  exit(0);
}
