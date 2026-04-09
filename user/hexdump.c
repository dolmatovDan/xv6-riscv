
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

char byte_buf[2];
char byte_to_char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void
print_byte(uint8 b)
{
  byte_buf[0] = byte_to_char[b >> 4];
  byte_buf[1] = byte_to_char[b & 15];
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(2, "should contains 2 additional arguments\n");
    exit(-1);
  }

  int fd = open(argv[2], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "fail to open file: %s\n", argv[2]);
    exit(-1);
  }

  char buf[256];
  int to_read = min(atoi(argv[1]), (int)sizeof(buf));
  char *p = buf;
  int n = to_read;

  while (n) {
    int r = read(fd, p, n);
    if (r < 0) {
      fprintf(2, "fail to read from file\n");
      exit(-1);
    }
    if (!r)
      break;
    p += r;
    n -= r;
  }

  int total = to_read - n;
  for (int i = 0; i < total; ++i) {
    print_byte(buf[i]);
    printf("%c%c", byte_buf[0], byte_buf[1]);
    if (i + 1 < total)
      printf(" ");
  }
  printf("\n");

  exit(0);
}
