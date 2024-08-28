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

extern uint ticks;

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  int size;
  struct buf buckets[NBUCKET];
  struct spinlock locks[NBUCKET];
  struct spinlock hashlock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  
  //struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  bcache.size = 0;
  initlock(&bcache.lock, "bcache");
  initlock(&bcache.hashlock,"bcache_hash");

  for(int i=0;i<NBUCKET;++i){
	  initlock(&bcache.locks[i],"bcache_hash");
  }

  // Create linked list of buffers
  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    //b->next = bcache.head.next;
    //b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    //bcache.head.next->prev = b;
    //bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
	int idx = blockno % NBUCKET;
	uint min_time_stamp;
  struct buf *b;
  struct buf *pre;
  struct buf *min;
  struct buf *min_pre;

  acquire(&bcache.locks[idx]);

  // Is the block already cached?
  for(b = bcache.buckets[idx].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  acquire(&bcache.lock);
  if(bcache.size < NBUF) {
	  b = &bcache.buf[bcache.size++];
	  release(&bcache.lock);
	  b->dev = dev;
	  b->blockno = blockno;
	  b->valid = 0;
	  b->refcnt = 1;
	  b->next = bcache.buckets[idx].next;
	  bcache.buckets[idx].next = b;
	  release(&bcache.locks[idx]);
	  acquiresleep(&b->lock);
	  return b;
  }
  release(&bcache.lock);
  release(&bcache.locks[idx]);

  acquire(&bcache.hashlock);
  for(int i = 0; i < NBUCKET; ++i) {
	  min_time_stamp = -1;
	  acquire(&bcache.locks[idx]);
	  for(pre = &bcache.buckets[idx], b = pre->next; b; pre = b, b = b->next) {	
		  if((idx == (blockno % NBUCKET)) && b->dev == dev && b->blockno == blockno){
			  b->refcnt++;
			  release(&bcache.locks[idx]);			
			  release(&bcache.hashlock);					   
			  acquiresleep(&b->lock);					  
			  return b;					      
		  }

		  if(b->refcnt == 0 && b->timestamp < min_time_stamp) {
			  min = b;
			  min_pre = pre;			
			  min_time_stamp = b->timestamp;				
		  }					     
	  }

	  if(min) {	   
	 	  min->dev = dev;		
		  min->blockno = blockno;				  
    		  min->valid = 0;					
		  min->refcnt = 1;

		  if(idx != (blockno % NBUCKET)) {              
			  min_pre->next = min->next;    		
			  release(&bcache.locks[idx]);				
		  	  idx = blockno % NBUCKET;		
			  acquire(&bcache.locks[idx]);					
			  min->next = bcache.buckets[idx].next;
			  bcache.buckets[idx].next = min;
		  }
		  release(&bcache.locks[idx]);
		  release(&bcache.hashlock);
		  acquiresleep(&min->lock);
		  return min;
	  }
	  release(&bcache.locks[idx]);
	  if(++idx == NBUCKET)
		  idx = 0;
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
	int idx;

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  idx = (b->blockno) % NBUCKET;
  acquire(&bcache.locks[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    //b->next->prev = b->prev;
    //b->prev->next = b->next;
    //b->next = bcache.head.next;
    //b->prev = &bcache.head;
    //bcache.head.next->prev = b;
    //bcache.head.next = b;
    b->timestamp = ticks;
  }
  
  release(&bcache.locks[idx]);
}

void
bpin(struct buf *b) {
	int idx = (b->blockno) % NBUCKET;
  acquire(&bcache.locks[idx]);
  b->refcnt++;
  release(&bcache.locks[idx]);
}

void
bunpin(struct buf *b) {
	int idx = (b->blockno) % NBUCKET;
  acquire(&bcache.locks[idx]);
  b->refcnt--;
  release(&bcache.locks[idx]);
}


