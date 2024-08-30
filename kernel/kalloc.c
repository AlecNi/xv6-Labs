// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char *ref_page;
  int page_cnt;
  char *end;
} kmem;

int page_count(void *,void *);
void incr(void *);
void decr(void *);
int page_index(uint64);

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.page_cnt = page_count(end,(void*)PHYSTOP);
  printf("page_count: %d\n",kmem.page_cnt);
  
  kmem.ref_page = end;
  for(int i=0;i<kmem.page_cnt;++i)
	  kmem.ref_page[i] = 0;
  kmem.end = kmem.page_cnt + kmem.ref_page;

  freerange(kmem.end, (void*)PHYSTOP);
}

int page_index(uint64 pa)
{
	pa = PGROUNDDOWN(pa);
	int res = (pa - (uint64)kmem.end) /PGSIZE;
	if(res < 0 || res >= kmem.page_cnt)
		panic("illegal page index!");
	return res;
}

void incr(void *pa)
{
	int index = page_index((uint64)pa);

	acquire(&kmem.lock);
	++(kmem.ref_page[index]);
	release(&kmem.lock);
}

void decr(void *pa)
{
	int index = page_index((uint64)pa);

	acquire(&kmem.lock);
	--(kmem.ref_page[index]);
	release(&kmem.lock);
}

int page_count(void *pg_start, void *pg_end)
{
	int cnt = 0;
	char *p;
       	p = (char*) PGROUNDUP((uint64) pg_start);
	while(p + PGSIZE <= (char*) pg_end){
		++cnt;
		p += PGSIZE;
	}
	return cnt;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
	int index = page_index((uint64)pa);
	if(kmem.ref_page[index] > 1){
		decr(pa);
		return;
	}
	if(kmem.ref_page[index] == 1)
		decr(pa);

  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
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
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk

    incr(r);
  }

  return (void*)r;
}
