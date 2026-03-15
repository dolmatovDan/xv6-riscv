#include "kernel/procinfo.h"
#include "user/user.h"

char* 
wrap_string(char *s, int n)
{
  char *ret = malloc(n);
  if (ret == 0)
    return s;
  int len = strlen(s);
  for (int i = 0; i < n; ++i) {
    if (i + len < n) {
      ret[i] = ' ';
    } else {
      ret[i] = s[i - (n - len)];
    }
  }
  return ret;
}

int
main(int argc, char argv[])
{
  int lim = 1;
  struct procinfo *plist = malloc(lim * sizeof(*plist));
  int cnt_proc = -1;
  while (plist && ((cnt_proc = ps_listinfo(plist, lim)) == -2)) {
    if (cnt_proc == -1) {
      fprintf(2, "fail to list info\n");
      exit(-1);
    }
    free(plist);
    lim *= 2;
    plist = malloc(lim * sizeof(*plist));
  }

  printf("cnt_proc: %d\n", cnt_proc);
  printf("     PID     NAME     STATE     PPID      PNAME\n");
  for (struct procinfo *p = plist; p < plist + cnt_proc; ++p) {
    char* wrapped_name = wrap_string(p->name, 6);
    char* wrapped_pname = wrap_string(p->pname, 6);
    printf("       %d   %s         %d        %d     %s\n", p->pid, wrapped_name, p->state, p->ppid, wrapped_pname);

    if (p->name != wrapped_pname)
      free(wrapped_pname);
    if (p->name != wrapped_name)
      free(wrapped_name);
  }

  exit(0);
}
