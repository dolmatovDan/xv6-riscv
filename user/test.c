#include "kernel/procinfo.h"
#include "user/user.h"

int 
main(int argc, char *argv[])
{
  // test null plist pointer
  if (ps_listinfo(0, 1) == -1) {
    fprintf(2, "ps_listinfo returns error with NULL plist\n");
    exit(-1);
  }
  printf("current process count: %d\n", ps_listinfo(0, 1));

  // test incorrect plist address
  if (ps_listinfo((struct procinfo *)-1, 5) != -1) {
    fprintf(2, "ps_listinfo should returns error when writing incorrect address\n");
    exit(-1);
  }

  // test correct plist address
  int lim = 5;
  struct procinfo *plist = malloc(lim * sizeof(*plist));
  if (plist == 0) {
    fprintf(2, "fail to malloc memory\n");
    exit(-1);
  }
  int cnt_proc = 0;
  if ((cnt_proc = ps_listinfo(plist, lim)) == -1) {
    free(plist);
    fprintf(2, "ps_listinfo should write correctly\n");
    exit(-1);
  }
  free(plist);

  struct procinfo *p = plist;
  for (int i = 0; i < cnt_proc; ++i) {
    printf("pid: (%d), name: (%s), ppid: (%d), pname: (%s), state: (%u)\n", p[i].pid, p[i].name, p[i].ppid, p[i].pname, p[i].state);
  }

  // test small buffer
  lim = 2;
  plist = malloc(lim * sizeof(*plist));
  if ((cnt_proc = ps_listinfo(plist, lim)) != -2) {
    fprintf(2, "should returns error, because buffer is small\n");
    free(plist);
    exit(-1);
  }
  free(plist);

  exit(0);
}
