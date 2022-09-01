#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *argv0;
int retval;

char*
fmtname(const char *path)
{
  const char *p;

  // Find first character after last slash.
  for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  return (char *)++p;
}

void
find(const char *path, const char *name, int force_rec)
{
  char buf[512], *p, *path_fmt;
  int fd;
  struct dirent de;
  struct stat st;
  uint len, len_cat;
  int r;

  fd = open(path, 0);
  if(fd < 0)
  {
    fprintf(2, "%s: cannot open %s\n", argv0, path);
    retval = 1;
    return;
  }

  if(fstat(fd, &st) < 0)
  {
    fprintf(2, "%s: cannot stat %s\n", argv0, path);
    close(fd);
    retval = 1;
    return;
  }

  path_fmt = fmtname(path);
  switch(st.type)
  {
  case T_FILE:
    if(!strcmp(path_fmt, name))
      printf("%s\n", path);
    break;

  case T_DIR:
    if(!force_rec)
    {
      if(!strcmp(path_fmt, "."))
        break;
      if(!strcmp(path_fmt, ".."))
        break;
    }
    if(!strcmp(path_fmt, name))
      printf("%s\n", path);
    strcpy(buf, path);
    len = strlen(buf);
    p = buf + len++;
    *p++ = '/';
    while(1)
    {
      r = read(fd, &de, sizeof(de));
      if(r == 0)
        break;
      if(r != sizeof(de))
      {
        fprintf(2, "%s: error reading %s\n", argv0, path);
        close(fd);
        retval = 1;
        return;
      }
      if(de.inum == 0)
        continue;
      len_cat = strlen(de.name);
      if(len + len_cat > sizeof buf - 1)
      {
        fprintf(2, "%s: pathname too long\n", argv0);
        retval = 1;
        continue;
      }
      strcpy(p, de.name);
      find(buf, name, 0);
    }
    break;
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  argv0 = fmtname(argv[0]);
  if(argc != 3)
  {
    fprintf(2, "usage: %s <dir> <name>\n", argv0);
    exit(1);
  }
  find(argv[1], argv[2], 1);

  exit(retval);
}
