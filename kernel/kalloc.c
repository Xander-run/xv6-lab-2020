//// Physical memory allocator, for user processes,
//// kernel stacks, page-table pages,
//// and pipe buffers. Allocates whole 4096-byte pages.
//
//#include "types.h"
//#include "param.h"
//#include "memlayout.h"
//#include "spinlock.h"
//#include "riscv.h"
//#include "defs.h"
//
//void freerange(void *pa_start, void *pa_end);
//
//extern char end[]; // first address after kernel.
//                   // defined by kernel.ld.
//int reference_bits[PHYSTOP / PGSIZE];  // todo: set a lock here, deadlock issue
//struct spinlock refbits_lock;
//
//struct run {
//  struct run *next;
//};
//
//struct {
//  struct spinlock lock;
//  struct run *freelist;
//} kmem;
//
//void ref_bits_init() {
//    initlock(&refbits_lock, "refbits");
//    acquire(&kmem.lock);
//    for (int i = 0; i < PGROUNDUP(PHYSTOP) / PGSIZE; i++)
//        reference_bits[i] = 0;
//    release(&kmem.lock);
//}
//
//void
//increase_rc(uint64 pa)
//{
//    acquire(&refbits_lock);
//    reference_bits[(uint64)pa / PGSIZE]++;
//    release(&refbits_lock);
//}
//
//void
//decrease_rc(uint64 pa)
//{
//    acquire(&refbits_lock);
//    reference_bits[pa / PGSIZE]--;
//    release(&refbits_lock);
//}
//
//void
//kinit()
//{
//    ref_bits_init();
//  initlock(&kmem.lock, "kmem");
//  freerange(end, (void*)PHYSTOP);
//}
//
//void
//freerange(void *pa_start, void *pa_end)
//{
//  char *p;
//  p = (char*)PGROUNDUP((uint64)pa_start);
//  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
//      increase_rc((uint64)p);
//      kfree(p);
//  }
//}
//
//// Free the page of physical memory pointed at by v,
//// which normally should have been returned by a
//// call to kalloc().  (The exception is when
//// initializing the allocator; see kinit above.)
//void
//kfree(void *pa)
//{
//    // acquire(&reference_bit_lock);
//    decrease_rc((uint64)pa);
//    if (reference_num((uint64)pa) > 0)
//        return;
//    // release(&reference_bit_lock);
//  struct run *r;
//
//  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
//    panic("kfree");
//
//  // Fill with junk to catch dangling refs.
//  memset(pa, 1, PGSIZE);
//
//  r = (struct run*)pa;
//
//  acquire(&kmem.lock);
//  r->next = kmem.freelist;
//  kmem.freelist = r;
//  release(&kmem.lock);
//}
//
//// Allocate one 4096-byte page of physical memory.
//// Returns a pointer that the kernel can use.
//// Returns 0 if the memory cannot be allocated.
//void *
//kalloc(void)
//{
//
//  struct run *r;
//
//  acquire(&kmem.lock);
//  r = kmem.freelist;
//  if(r)
//    kmem.freelist = r->next;
//  release(&kmem.lock);
//
//  // set the reference bit of the allocated page to one
//  // acquire(&reference_bit_lock);
//  increase_rc((uint64) r);
//  // release(&reference_bit_lock);
//
//  if(r)
//    memset((char*)r, 5, PGSIZE); // fill with junk
//  return (void*)r;
//}
//
//int
//reference_num(uint64 pa) {
//    return reference_bits[pa / PGSIZE];
//}

// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#define PA2IDX(pa) (((uint64)pa) >> 12)

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
// defined by kernel.ld.

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;

struct {
    struct spinlock lock;
    int count[PGROUNDUP(PHYSTOP) / PGSIZE];
} refcnt;

void
rcinit()
{
    initlock(&refcnt.lock, "refcnt");
    acquire(&kmem.lock);
    for (int i = 0; i < PGROUNDUP(PHYSTOP) / PGSIZE; i++)
        refcnt.count[i] = 0;
    release(&kmem.lock);
}

void
increase_rc(void *pa)
{
    acquire(&refcnt.lock);
    refcnt.count[PA2IDX(pa)]++;
    release(&refcnt.lock);
}

void
decrease_rc(void *pa)
{
    acquire(&refcnt.lock);
    refcnt.count[PA2IDX(pa)]--;
    release(&refcnt.lock);
}
int
get_rc(void *pa)
{
    acquire(&refcnt.lock);
    int rc = refcnt.count[PA2IDX(pa)];
    release(&refcnt.lock);
    return rc;
}

void
kinit()
{
    rcinit();
    initlock(&kmem.lock, "kmem");
    freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    {
        increase_rc((void*)p);
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

    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    decrease_rc(pa);
    if (get_rc(pa) > 0)
        return;
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

    if(r)
    {
        memset((char*)r, 5, PGSIZE); // fill with junk
        increase_rc((void*)r);
    }
    return (void*)r;
}