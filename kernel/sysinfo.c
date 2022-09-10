#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_sysinfo(void)
{
  uint64 addr;

  if(argaddr(0, &addr) < 0)
    return -1;
  return sysinfo(addr);
}

int
sysinfo(uint64 addr)
{
  struct sysinfo si = {
    .freemem = freemem(),
    .nproc   = procnum(),
  };
  struct proc *p = myproc();

  return copyout(p->pagetable, addr, (char *)&si, sizeof si);
}

uint64
freemem(void)
{
  return kfreepgc() * PGSIZE;
}
