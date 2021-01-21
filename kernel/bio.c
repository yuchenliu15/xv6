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

#define NBUCKET 13
struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  struct buf head[NBUCKET];
} bcache;

struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct bucket table[13];

uint hash(uint blockno) {
  return (blockno % NBUCKET);
}

void
binit(void)
{
  struct buf *b;
  for(int i = 0; i < NBUCKET; i++) {
    char name[] = "       ";
    snprintf(name, 7, "bcache%d", i);
    initlock(&bcache.lock[i], name);

    struct buf *head = &bcache.head[i];
    head->prev = head;
    head->next = head;
  }

  struct buf *head = &bcache.head[0];
  for(b = bcache.buf; b < bcache.buf+NBUF; b++) {
    b->next = head->next;
    b->prev = head;
    initsleeplock(&b->lock, "buffer");
    head->next->prev = b;
    head->next = b;
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  uint index = hash(blockno);
  struct buf *b;
  struct spinlock lock = bcache.lock[index];
  acquire(&lock);
  // Is the block already cached?
  for(b = bcache.head[index].next; b != &bcache.head[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int i = index;
  for(;;) {
    i = (i + 1) % NBUCKET;
    if(i == index) continue;
    struct spinlock other_lock = bcache.lock[i];
    acquire(&other_lock);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // remove
        b->prev->next = b->next;
        b->next->prev = b->prev;
        release(&other_lock);
        // add
        struct buf *dst = &bcache.head[index];
        dst->next->prev = b;
        b->next = dst->next;
        b->prev = dst;
        dst->next = b;
        release(&lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&other_lock);
  }

  panic("bget: no buffers");
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

  uint index = hash(b->blockno);
  acquire(&bcache.lock[index]);
  struct buf *head = &bcache.head[index];
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = head->next;
    b->prev = head;
    head->next->prev = b;
    head->next = b;
  }
  
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock[hash(b->blockno)]);
  b->refcnt++;
  release(&bcache.lock[hash(b->blockno)]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock[hash(b->blockno)]);
  b->refcnt--;
  release(&bcache.lock[hash(b->blockno)]);
}


