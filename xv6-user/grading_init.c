#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"

char *argv[] = { "sh", 0 };

int main() {
    int pid, wpid;
    dev(O_RDWR, CONSOLE, 0);
    dup(0);
    dup(0);
    pid = fork();
    if (pid == 0) {
        argv[0] = "test_read";
        exec("test_read", argv);
        exit(0);
    } else {
        pid = fork();
        if (pid == 0) {
            argv[0] = "test_uname";
            exec("test_uname",argv);
            exit(0);
        }
    }

    while (1);
    return 0;
}