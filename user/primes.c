#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUMBER_MIN (2)
#define NUMBER_MAX (35)

int
main(int argc, char *argv[])
{
  int init = 1;
  int pid;
  int ptoc[2], ctog[2];
  int n = NUMBER_MAX - NUMBER_MIN + 1;
  int num[NUMBER_MAX - NUMBER_MIN + 1];
  int n_new;
  int num_new[NUMBER_MAX - NUMBER_MIN];
  int num_cur, num_next, *num_ptr;
  int r;

  for(int i = 0; i < n; ++i)
    num[i] = NUMBER_MIN + i;

  while(1)
  {
    if(init == 1)
      init = 0;
    else
    {
      num_ptr = num;
      for(int i = 0; i < NUMBER_MAX - NUMBER_MIN; ++i)
      {
        r = read(ptoc[0], num_ptr, sizeof *num_ptr);
        if(r < 0)
        {
          fprintf(2, "%d: read failed\n", getpid());
          exit(1);
        }
        if(r == 0)
          break;
        ++num_ptr;
      }
      if(r)
      {
        fprintf(2, "%d: input too many numbers\n", getpid());
        exit(1);
      }
      n = num_ptr - num;
      close(ptoc[0]);
    }
    num_cur = num[0];
    printf("prime %d\n", num_cur);
    n_new = 0;
    for(int i = 1; i < n; ++i)
      if((num_next = num[i]) % num_cur)
        num_new[n_new++] = num_next;
    if(n_new == 0)
      exit(0);

    if(pipe(ctog))
    {
      fprintf(2, "%d: pipe failed\n", getpid());
      exit(1);
    }

    pid = fork();
    if(pid < 0)
    {
      fprintf(2, "%d: fork failed\n", getpid());
      exit(1);
    }
    if(pid == 0)
    {
      ptoc[0] = ctog[0];
      ptoc[1] = ctog[1];
      close(ptoc[1]);
    }
    else
    {
      int size;

      close(ctog[0]);
      size = n_new * sizeof *num_new;
      if(write(ctog[1], num_new, size) != size)
      {
        fprintf(2, "%d: write failed\n", getpid());
        exit(1);
      }
      close(ctog[1]);
      if(wait((int *)0) < 0)
      {
        fprintf(2, "%d: wait failed\n", getpid());
        exit(1);
      }
      exit(0);
    }
  }

  exit(0);
}
