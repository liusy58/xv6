#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void help(){
    printf("Usage : sleep [ cnt ]\n");
}

int
main(int argc, char *argv[])
{
  if(argc != 2){
      help();
      exit(0);
  }
  int cnt;
  cnt = atoi(argv[1]);
  sleep(cnt);
  exit(0);
}
