// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.


void
kinit()
{
  push_off();
  struct cpu *cpu = mycpu();
  char name[] = "    ";
  snprintf(name, 5, "kmem%d", cpuid());
  initlock(&cpu->lock, name);
  freerange(end, (void*)PHYSTOP);
  pop_off();
}

void
freerange(void *pa_start, void *pa_end)
{
  struct cpu *cpu = mycpu();
  struct run *r = cpu->freelist;
  for(int i = 0; i < NCPU; i++) {
    if(i == cpuid()) continue;
    struct cpu cpu = cpus[i];
    acquire(&cpu.lock);
    struct run *head = cpu.freelist;
    if(head) {
      r = head;
      cpu.freelist = 0;
      release(&cpu.lock);
      break;
    }
    release(&cpu.lock);
  }

  if(!r) {
    char *p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
      kfree(p);
  }

}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  struct cpu *cpu = mycpu();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();

  r->next = cpu->freelist;
  cpu->freelist = r;
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  struct run *r;
  struct cpu *cpu = mycpu();
  r = cpu->freelist;
  if(r)
    cpu->freelist = r->next;
  else {
    for(int i = 0; i < NCPU; i++) {
      if(i == cpuid()) continue;
      struct cpu *other_cpu = &cpus[i];
      acquire(&other_cpu->lock);
      struct run *head = other_cpu->freelist;
      if(head) {
        r = head;
        other_cpu->freelist = head->next;
        release(&other_cpu->lock);
        break;
      }
      release(&other_cpu->lock);
    }
  }
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
