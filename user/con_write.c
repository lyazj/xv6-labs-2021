#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user.h"

int main(void)
{
  int fd, pid;
  char buf[200];

  if((fd = open("con_write.txt", O_WRONLY | O_CREATE)) < 0)
  {
    fprintf(2, "open failed\n");
    exit(1);
  }
  if((pid = fork()) < 0)
  {
    fprintf(2, "fork failed\n");
    exit(1);
  }
  memset(buf, '0' + (pid == 0), sizeof buf);
  if(write(fd, buf, sizeof buf) != sizeof buf)
  {
    fprintf(2, "write failed\n");
    exit(1);
  }
  exit(0);
}
