#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);
    int last_write = p[1];

    for(int i = 2; i <= 35; i++) {
        write(p[1], &i, 4);
    }
    close(p[1]);

    int current = 0;
    int f;
    while(((f = fork()) == 0) && read(p[0], &current, 4) > 0) {
        int last_read = p[0];
        last_write = p[1];
        pipe(p);
        printf("prime %d\n", current);
        int next = 0;
        while(read(last_read, &next, 4) > 0) {
            if(next % current != 0) {
                write(p[1], &next, 4);
            }
        }
        close(p[1]);
        close(last_read);
    }

    close(last_write);
    wait(0);
    exit(0);
}
