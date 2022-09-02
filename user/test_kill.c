#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  int pid, wstat;

  pid = fork();
  if(pid < 0)
  {
    fprintf(2, "%s: fork failed\n", argv[0]);
    exit(1);
  }
  if(pid == 0)
  {
    pid = fork();
    if(pid < 0)
    {
      fprintf(2, "%s: fork failed\n", argv[0]);
      exit(1);
    }
    if(pid == 0)
    {
      while(1);
      exit(0);
    }
    if(kill(pid))
    {
      fprintf(2, "%s: kill failed\n", argv[0]);
      exit(1);
    }
    if(wait(&wstat) != pid)
    {
      fprintf(2, "%s: wait failed\n", argv[0]);
      exit(1);
    }
    fprintf(2,
        "%s: grandchild %d killed, status: %d\n", argv[0], pid, wstat);
    exit(-1);
  }
  if(wait(&wstat) != pid)
  {
    fprintf(2, "%s: wait failed\n", argv[0]);
    exit(1);
  }
  fprintf(2, "%s: child %d exited, status: %d\n", argv[0], pid, wstat);
  printf("So, wait() cannot distinguish them at all!\n");
  exit(0);
}
