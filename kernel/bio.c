// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 17U

static uint
hash_blockno(uint blockno)
{
  return blockno % NBUCKET;
}

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct {
    struct spinlock lock;
    struct buf head;
  } bucket[NBUCKET];
} bcache;

void
binit(void)
{
  int h;
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(h = 0; h < NBUCKET; ++h)
  {
    initlock(&bcache.bucket[h].lock, "bcache.bucket");
    bcache.bucket[h].head.prev = &bcache.bucket[h].head;
    bcache.bucket[h].head.next = &bcache.bucket[h].head;
  }
  for(b = bcache.buf; b != &bcache.buf[NBUF]; ++b)
    initsleeplock(&b->lock, "buffer");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  int h = hash_blockno(blockno);
  struct buf *b, *v;

  acquire(&bcache.lock);

  // Is the block already cached?
  b = bcache.bucket[h].head.next;
  for(; b != &bcache.bucket[h].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  v = bcache.buf;
  while(v != &bcache.buf[NBUF] && v->refcnt)
    ++v;
  if(v == &bcache.buf[NBUF])
    panic("bget: no buffers");
  for(b = v + 1; b != &bcache.buf[NBUF]; ++b)
    if(b->refcnt == 0 && b->stamp < v->stamp)
      v = b;
  v->dev = dev;
  v->blockno = blockno;
  v->valid = 0;
  v->refcnt = 1;
  v->prev = &bcache.bucket[h].head;
  v->next = bcache.bucket[h].head.next;
  v->prev->next = v;
  v->next->prev = v;
  release(&bcache.lock);
  acquiresleep(&v->lock);
  return v;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->stamp = ticks;
  }
  
  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


