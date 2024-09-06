#include "param.h"
#include "pstat.h"
#include "user.h"

#define ABS(x) ((x) >= 0 ? (x) : -(x))

int 
main(int argc, char *argv[]) 
{
  struct pstat stats;
  const int res = getpinfo(&stats);
  if (res < 0) {
    printf(2, "couldn't get processes' info\n");
    exit();
  }

  printf(1, "pid ticks tickets\n");
  for (int i = 0; i < NPROC; ++i) {
    if (!stats.inuse[i]) {
      continue;
    }
    printf(1, "%d %d %d\n", stats.pid[i], stats.ticks[i], ABS(stats.tickets[i]));
  }

  exit();
}
