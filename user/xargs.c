#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXLINE (8192)

char *fgets(int fd, char *buf)
{
  int i;
  int r;

  for(i = 0; i < MAXLINE; ++i)
  {
    r = read(fd, &buf[i], 1);
    if(r < 0)
    {
      fprintf(2, "%s: read failed\n", __func__);
      exit(1);
    }
    if(r == 0)
    {
      if(i)
      {
        buf[i] = 0;
        return buf;
      }
      return (char *)0;
    }
    if(buf[i] == '\n')
    {
      buf[i] = 0;
      return buf;
    }
  }
  fprintf(2, "%s: line too long\n", __func__);
  exit(1);
}

void run(char *path, char *argv[])
{
  int pid;

  pid = fork();
  if(pid < 0)
  {
    fprintf(2, "%s: fork failed\n", __func__);
    exit(1);
  }
  if(pid == 0)
  {
    exec(path, argv);
    fprintf(2, "%s: exec failed\n", __func__);
    exit(1);
  }
  if(wait((int *)0) < 0)
  {
    fprintf(2, "%s: wait failed\n", __func__);
    exit(1);
  }
}

void xargs(int argc, char *argv[], char *line)
{
  char *argv_new[MAXARG + 1], **argv_cur, **argv_end;
  char *ptr;

  memcpy(argv_new, argv, argc * sizeof *argv_new);
  argv_cur = &argv_new[argc];
  argv_end = &argv_new[MAXARG];
  ptr = line;
  while(1)
  {
    while(*ptr && *ptr == ' ')
      ++ptr;
    if(!*ptr)
      break;
    if(argv_cur == argv_end)
    {
      fprintf(2, "%s: too many arguments\n", __func__);
      exit(1);
    }
    *argv_cur++ = ptr++;
    while(*ptr && *ptr != ' ')
      ++ptr;
    if(!*ptr)
      break;
    *ptr++ = 0;
  }
  if(argv_cur == argv_new)
    return;
  *argv_cur = (char *)0;

  run(argv_new[0], argv_new);
}

int
main(int argc, char *argv[])
{
  char buf[MAXLINE];

  while(fgets(0, buf))
    xargs(argc - 1, &argv[1], buf);
  exit(0);
}
