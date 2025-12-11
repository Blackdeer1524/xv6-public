#include "user.h"

void 
foo(const int tickets_count) 
{
    const int setting_res = setticketscount(tickets_count); 
    if (setting_res < 0) {
        printf(2, "couldn't set new quota\n");
        exit();
    }

    const int limit = 2100000000;
    for (int i = 0; i < limit; ++i) {
        asm volatile ("" ::: "memory");
    }

    struct pstat stats;
    const int res = getpinfo(&stats);
    if (res < 0) {
        printf(2, "couldn't get processes' info\n");
        exit();
    }

    exit();
}

int 
main(int argc, char *argv[]) 
{
    if (argc != 1) {
        printf(2, "no args required\n");
        exit();
    }

    int pid = fork();
    if (pid < 0) {
        printf(2, "first fork has failed\n");
        exit();
    } else if (pid == 0) {
        foo(10);
        exit();
    }

    pid = fork();
    if (pid < 0) {
        printf(2, "second fork has failed\n");
        exit();
    } else if (pid == 0) {
        foo(20);
        exit();
    }

    pid = fork();
    if (pid < 0) {
        printf(2, "third fork has failed\n");
        exit();
    } else if (pid == 0) {
        foo(40);
        exit();
    }

    exit();
}
