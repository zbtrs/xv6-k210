#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/sysinfo.h"
#include "xv6-user/user.h"

int main(){
    printf("Hello World!\n");
    int fd = 0;
    // fd = dup(1);
    fd = dup3(1, 100);
	printf("%d\n", fd);
    printf("the end!\n");
    exit(0);
}