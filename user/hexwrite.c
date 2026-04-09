
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char byte_to_char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

int get_byte(char *b) {
  int p1 = -1, p2 = -1;
  for (int i = 0; i < sizeof byte_to_char; ++i) {
    if (b[0] == byte_to_char[i]) {
      p1 = i;
    }
    if (b[1] == byte_to_char[i]) {
      p2 = i;
    }
  }
  if (p1 == -1 || p2 == -1)
    return -1;

  return p1 * 16 + p2;
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(2, "should contains 2 additional arguments\n");
    exit(-1);
  }

  int fd = open(argv[2], O_WRONLY);
  if (fd < 0) {
    fprintf(2, "fail to open file: %s\n", argv[2]);
    exit(-1);
  }

  char *to_write = argv[1];
  int n = strlen(to_write) >> 1;
  char buf[256];
  char *p = buf;
  for (int i = 0; i < n; ++i) {
    int b = get_byte(to_write + i * 2);
    if (b == -1) {
      fprintf(2, "incorrect format\n");
      exit(-1);
    }
    buf[i] = (char)b;
  }

  while (n) {
    int r = write(fd, p, n);
    if (r < 0) {
      fprintf(2, "fail to write to file\n");
      exit(-1);
    }
    if (!r)
      break;

    p += r;
    n -= r;
  }

  exit(0);
}
