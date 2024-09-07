#include "param.h"
#include "pstat.h"
#include "user.h"
#include "defs.h"

int 
main(int argc, char *argv[]) 
{
  struct pstat stats;
  const int res = getpinfo(&stats);
  if (res < 0) {
    printf(2, "couldn't get processes' info\n");
    exit();
  }

  printf(1, "(index) pid ticks tickets\n");
  for (int i = 0; i < NPROC; ++i) {
    if (!stats.inuse[i]) {
      continue;
    }
    printf(1, "(%d) %d %d %d %d\n", i, stats.pid[i], stats.ticks[i], ABS(stats.tickets[i]), stats.inuse[i]);
  }

  exit();
}
