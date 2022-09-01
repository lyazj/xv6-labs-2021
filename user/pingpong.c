#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void parent(int fd0, int fd1)
{
  char ch;

  if(write(fd1, "p", 1) != 1)
  {
    fprintf(2, "%s: write failed\n", __func__);
    exit(1);
  }

  if(read(fd0, &ch, 1) != 1)
  {
    fprintf(2, "%s: read failed\n", __func__);
    exit(1);
  }

  printf("%d: received pong\n", getpid());
}

void child(int fd0, int fd1)
{
  char ch;

  if(read(fd0, &ch, 1) != 1)
  {
    fprintf(2, "%s: read failed\n", __func__);
    exit(1);
  }

  printf("%d: received ping\n", getpid());

  if(write(fd1, "c", 1) != 1)
  {
    fprintf(2, "%s: write failed\n", __func__);
    exit(1);
  }
}

int
main(int argc, char *argv[])
{
  int ptoc[2], ctop[2];
  int pid;

  if(pipe(ptoc) || pipe(ctop))
  {
    fprintf(2, "%s: pipe failed\n", argv[0]);
    exit(1);
  }
  
  pid = fork();
  if(pid < 0)
  {
    fprintf(2, "%s: fork failed\n", argv[0]);
    exit(1);
  }
  if(pid == 0)
  {
    close(ptoc[1]);
    close(ctop[0]);
    child(ptoc[0], ctop[1]);
    close(ptoc[0]);
    close(ctop[1]);
  }
  else
  {
    close(ptoc[0]);
    close(ctop[1]);
    parent(ctop[0], ptoc[1]);
    close(ptoc[1]);
    close(ctop[0]);
  }

  exit(0);
}
