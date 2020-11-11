#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char current[], char target[]) {
    int fd;
    char next_directory[512], *temp;
    struct stat file_stat;
    struct dirent de;

    if((fd = open(current, 0)) < 0) {
        printf("cannot open");
    }
    if(fstat(fd, &file_stat) < 0) {
        printf("cannot stat");
        close(fd);
        return;
    }
    strcpy(next_directory, current);
    temp = next_directory + strlen(next_directory);
    *temp++ = '/';

    if(file_stat.type == T_DIR) {
        while(read(fd, &de, sizeof(de)) == sizeof(de)) {
            if(de.inum == 0) {
                continue;
            }
            if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                continue;
            }
            memmove(temp, de.name, DIRSIZ);
            temp[DIRSIZ] = 0;

            if(stat(next_directory, &file_stat) < 0) {
                printf("cannot stat next dir");
                continue;
            }
            if(file_stat.type == T_DIR) {
                find(next_directory, target);
            }
            if(file_stat.type == T_FILE && strcmp(de.name, target) == 0) {
                printf("%s\n", next_directory);
            }
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("Need more than two arguments");
    }
    else {
        find(argv[1], argv[2]);
    }
    exit(0);
}