#include "param.h"
#include "user.h"

int 
main(int argc, char *argv[]) 
{
    if (argc != 2) {
        printf(2, "not enough args\n");
        exit();
    }
    const int tickets_count = atoi(argv[1]);
    const int setting_res = setticketscount(tickets_count); 
    if (setting_res < 0) {
        printf(2, "couldn't set new quota\n");
        exit();
    }

    const int limit = 1000000000;
    for (int i = 0; i < limit; ++i) {
        asm volatile ("" ::: "memory");
    }

    struct pstat stats;
    const int res = getpinfo(&stats);
    if (res < 0) {
        printf(2, "couldn't get processes' info\n");
        exit();
    }

    const int pid = getpid();
    for (int i = 0; i < NPROC; ++i) {
        if (stats.pid[i] == pid) {
            printf(1, "got ticks: %d\n", stats.ticks[i]);
        }
    }

    exit();
}
