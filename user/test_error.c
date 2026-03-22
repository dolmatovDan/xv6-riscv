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

  // close mutex in current process
  int pid = fork();
  int err, code;

  switch (pid) {
  case -1:
    fprintf(2, "fail to fork\n");
    exit(-1);
  case 0:
    if ((err = mutex_lock(mufd)) == -1) {
      fprintf(2, "fail to lock mutex in child\n");
      exit(-1);
    }
    close(mufd);
    exit(0);
    break;
  default:
    wait(&code);

    if ((err = mutex_lock(mufd)) == -1) {
      fprintf(2, "fail to lock mutex in parent\n");
      exit(-1);
    }
    printf("lock mutex succefully in parent after closing in child\n");
    
    if ((err = mutex_unlock(mufd)) == -1) {
      fprintf(2, "fail to mutex in other process\n");
      exit(-1);
    }
    close(mufd);
  }

  // close mutex in other process
  pid = fork();
  mufd = mutex();

  switch (pid) {
  case -1:
    fprintf(2, "fail to fork\n");
    exit(-1);
  case 0:
    if ((err = mutex_lock(mufd)) == -1) {
      fprintf(2, "fail to lock mutex in child\n");
      exit(-1);
    }
    printf("lock succefully\n");
    if ((err = mutex_unlock(mufd)) == -1) {
      fprintf(2, "fail to unlock mutex in other process\n");
      exit(-1);
    }
    printf("unlock succefully\n");
    exit(0);
    break;
  default:
    close(mufd);
    wait(&code);
  }

  // test releasing mutex in other process
  pid = fork();
  mufd = mutex();

  switch (pid) {
  case -1:
    fprintf(2, "fail to fork\n");
    exit(-1);
  case 0:
    if ((err = mutex_lock(mufd)) == -1) {
      fprintf(2, "fail to lock mutex in child\n");
      exit(-1);
    }
    exit(0);
    break;
  default:
    if ((err = mutex_unlock(mufd)) != -1) {
      fprintf(2, "should fail, unlocking mutex in other process\n");
      exit(-1);
    }

    int code;
    wait(&code);
    close(mufd);
  }

  // test exit without close
  int mufd2 = mutex();
  if (mufd2 < 0) {
    fprintf(2, "fail to create mutex\n");
    exit(-1);
  }
  pid = fork();
  switch (pid) {
  case -1:
    fprintf(2, "fail to fork\n");
    exit(-1);
  case 0:
    exit(0);
  default:
    wait(&code);
    close(mufd2);
  }

  // test exit with locked mutex
  int mufd3 = mutex();
  if (mufd3 < 0) {
    fprintf(2, "fail to create mutex\n");
    exit(-1);
  }
  pid = fork();
  switch (pid) {
  case -1:
    fprintf(2, "fail to fork\n");
    exit(-1);
  case 0:
    if ((err = mutex_lock(mufd3)) == -1) {
      fprintf(2, "fail to lock mutex in child\n");
      exit(-1);
    }
    exit(0);
  default:
    wait(&code);
    if ((err = mutex_lock(mufd3)) == -1) {
      fprintf(2, "fail to lock mutex after child exit\n");
      exit(-1);
    }
    printf("lock mutex successfully after child exit with locked mutex\n");
    if ((err = mutex_unlock(mufd3)) == -1) {
      fprintf(2, "fail to unlock mutex\n");
      exit(-1);
    }
    close(mufd3);
  }

  exit(0);
}
