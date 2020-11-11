#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    char input[1];
    char current[1000];
    char *copy[MAXARG];
    int arg_count = argc - 1, start = 0, count = 0;

    for(int i = 1; i < argc; i++) {
        copy[i-1] = argv[i];
    }

    while(read(0, input, 1)) {
        if(input[0] == '\n' || input[0] == ' ') {
            current[count++] = '\0';
            copy[arg_count++] = &current[start];
            start = count;

            if(input[0] == '\n') {
                start = 0;
                count = 0;
                if(fork() > 0) {
                    wait(0);
                }
                else {
                    exec(argv[1], copy);
                }
            }
        }
        else {
            current[count++] = input[0];
        }
    }
    exit(0);
}
