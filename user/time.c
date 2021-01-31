#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int
main(int argc, char *argv[]){
    uint64 time = uptime();
    printf("Time now is %l\n",time);
    exit(0);
}

