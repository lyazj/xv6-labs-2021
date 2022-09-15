// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

#define HEAPBEGIN (PGROUNDUP((uint64)end))
#define HEAPEND   (PGROUNDDOWN((uint64)PHYSTOP))

/* static */
uint64 ref[(PHYSTOP - KERNBASE) / PGSIZE];

#define HEAPREF(pa) ref[((uint64)(pa) - HEAPBEGIN) / PGSIZE]

struct run {
  struct run *next;
};

/* static */
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  struct run *r = (struct run *)HEAPBEGIN;
  kmem.freelist = r;
  while(r != (struct run *)(HEAPEND - PGSIZE))
    r = r->next = (struct run *)((uint64)r + PGSIZE);
  r->next = 0;
  initlock(&kmem.lock, "kmem");
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().
void
kfree(void *pa)
{
  struct run *r;

  if((uint64)pa & (PGSIZE - 1))
    panic("kfree: align");
  if((uint64)pa < HEAPBEGIN)
    panic("kfree: below the bound");
  if((uint64)pa >= HEAPEND)
    panic("kfree: above the bound");

  acquire(&kmem.lock);
  if(HEAPREF(pa) == 0)
    panic("kfree: double free");
  if(--HEAPREF(pa))
  {
    release(&kmem.lock);
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  r = (struct run*)pa;
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
  {
    if(HEAPREF(r)++)
      panic("kalloc: double alloc");
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Share one 4096-byte page of physical memory.
// Returns the physical address.
void *
kshare(void *pa)
{
  if((uint64)pa & (PGSIZE - 1))
    panic("kshare: align");
  if((uint64)pa < HEAPBEGIN)
    panic("kshare: below the bound");
  if((uint64)pa >= HEAPEND)
    panic("kshare: above the bound");

  acquire(&kmem.lock);
  if(HEAPREF(pa)++ == 0)
    panic("kshare: freed");
  release(&kmem.lock);
  return pa;
}

// Duplicate one 4096-byte page of physical memory.
// Returns the duplication on sucess.
// Returns 0 if kalloc() fails.
void *
kdup(void *pa)
{
  void *pa_new;

  if((uint64)pa & (PGSIZE - 1))
    panic("kdup: align");
  if((uint64)pa < HEAPBEGIN)
    panic("kdup: below the bound");
  if((uint64)pa >= HEAPEND)
    panic("kdup: above the bound");

  pa_new = kalloc();
  if(pa_new == 0)
    return 0;
  acquire(&kmem.lock);
  if(HEAPREF(pa) == 0)
    panic("kdup: freed");
  memmove(pa_new, pa, PGSIZE);
  release(&kmem.lock);
  return pa_new;
}

// Duplicate one 4096-byte page of physical memory,
// and kfree(pa) on success.
// Returns the duplication on sucess.
// Returns 0 if kalloc() fails, with pa untouched.
void *
kmove(void *pa)
{
  struct run *r;

  if((uint64)pa & (PGSIZE - 1))
    panic("kmove: align");
  if((uint64)pa < HEAPBEGIN)
    panic("kmove: below the bound");
  if((uint64)pa >= HEAPEND)
    panic("kmove: above the bound");

  acquire(&kmem.lock);
  if(HEAPREF(pa) == 0)
    panic("kmove: freed");
  if(HEAPREF(pa) == 1)
  {
    release(&kmem.lock);
    return pa;
  }
  r = kmem.freelist;
  if(r == 0)
  {
    release(&kmem.lock);
    return 0;
  }
  if(HEAPREF(r)++)
    panic("kmove: double alloc");
  kmem.freelist = r->next;
  --HEAPREF(pa);
  memmove((void *)r, pa, PGSIZE);
  release(&kmem.lock);
  return (void *)r;
}
