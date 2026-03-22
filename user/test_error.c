#include "kernel/types.h"
#include "user.h"


int
main(int argc, char *argv[])
{
  // test read/write
  int mufd = mutex();
  if (mufd < 0) {
    fprintf(2, "fail to create mutex\n");
    exit(-1);
  }
  char buf[128];
  if ((write(mufd, buf, sizeof buf)) != -1) {
    fprintf(2, "should return error in writing to mutex\n");
    exit(-1);
  }
  if ((read(mufd, buf, sizeof buf)) != -1) {
    fprintf(2, "should return error in reading from mutex\n");
    exit(-1);
  }

  // test closing mutex in other process
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
    break;
  default:
    if ((err = mutex_unlock(mufd)) != -1) {
      fprintf(2, "should fail unlocking mutex in other process\n");
      exit(-1);
    }

    int code;
    wait(&code);
  }

  exit(0);
}
