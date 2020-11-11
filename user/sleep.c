#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Error");
    }
    else {
        int value = atoi(argv[1]);
        printf("%d", value);
        sleep(value);
    }
    exit(0);

}