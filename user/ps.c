#include "kernel/procinfo.h"
#include "user/user.h"

char* 
wrap_string(char *s, int n)
{
  char *ret = malloc(n + 1);
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
  ret[n] = '\0';
  return ret;
}

char* get_text_state(int state) {
  switch (state) {
  case 0:
    return "  UNUSED";
  case 1:
    return "    USED";
  case 2:
    return "SLEEPING";
  case 3:
    return "RUNNABLE";
  case 4:
    return " RUNNING";
  case 5:
    return "  ZOMBIE";
  }

  return "";
}

int
main(int argc, char *argv[])
{
  int lim = ps_listinfo(0, 0);
  struct procinfo *plist = malloc(lim * sizeof(*plist));
  int cnt_proc = -1;
  while (plist && ((cnt_proc = ps_listinfo(plist, lim)) == -2)) {
    free(plist);
    lim *= 2;
    plist = malloc(lim * sizeof(*plist));
  }

  if (cnt_proc == -1) {
    fprintf(2, "fail to list info\n");
    exit(-1);
  }

  printf("cnt_proc: %d\n", cnt_proc);
  printf("     PID     NAME            STATE     PPID      PNAME\n");
  printf("------------------------------------------------------\n");
  for (struct procinfo *p = plist; p < plist + cnt_proc; ++p) {
    char* wrapped_name = wrap_string(p->name, 6);
    char* wrapped_pname = wrap_string(p->pname, 6);
    printf("       %d   %s         %s        %d     %s\n", p->pid, wrapped_name, get_text_state(p->state), p->ppid, wrapped_pname);

    if (p->pname != wrapped_pname)
      free(wrapped_pname);
    if (p->name != wrapped_name)
      free(wrapped_name);
  }
  free(plist);

  exit(0);
}
