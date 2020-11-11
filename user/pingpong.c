#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p[2];
    char receive[1];
    pipe(p);
    if(fork() > 0) {
        write(p[1], "a", 1);
        wait((int *) 0);
        read(p[0], receive, 1);
        if(receive[0] == 'a') {
            printf("%d: received pong\n", getpid());
        }
    }
    else {
        read(p[0], receive, 1);
        if(receive[0] == 'a') {
            printf("%d: received ping\n", getpid());
            write(p[1], "a", 1);
        }

    }
    exit(0);
}