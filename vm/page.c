#include "vm/page.h"
#include "vm/frame.h"

#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hash.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "userprog/process.h"

/*
Most importantly, on a page fault, the kernel looks up the virtual page
that faulted in the supplemental page table to find out what data should be there.
Second, the kernel consults the supplemental page table when a process terminates,
to decide what resources to free.
*/

//static struct lock sup_page_lock;
// static struct hash sup_page_hash;


static unsigned sup_page_hash_func(const struct hash_elem *h, void *aux UNUSED)
{
  struct sup_page_table_entry *spte = hash_entry(h, struct sup_page_table_entry, elem);
  return hash_bytes(spte->user_vaddr, sizeof spte->user_vaddr);
}

static bool sup_page_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  struct sup_page_table_entry *small = hash_entry(a, struct sup_page_table_entry, elem);
  struct sup_page_table_entry *big = hash_entry(b, struct sup_page_table_entry, elem);
  return (small->user_vaddr) < (big->user_vaddr);
}


/*
 * Initialize supplementary page table
 */
void 
sup_page_init (struct thread * t)
{
    //lock_init(&sup_page_lock);
    //move below to thread_create
    hash_init(&t->sup_page_table, sup_page_hash_func, sup_page_less_func, NULL);
    //hash_init(&thread_current()->sup_page_table, &sup_page_hash_func, &sup_page_less_func, NULL);
}

/*
 * Make new supplementary page table entry for addr 
 */
struct sup_page_table_entry *
allocate_sup_page (void *addr)
{
    struct sup_page_table_entry *spte = malloc(sizeof (struct sup_page_table_entry));

    if (spte == NULL) {
        return NULL;
    }

    //spte->type = t;
    spte->user_vaddr = pg_round_down(addr);
    spte->kernel_vaddr = NULL;
    // spte->access_time = NULL;
    spte->dirty = false;
    // spte->accessed = NULL;
    // spte->file = NULL;
    // spte->alias_frames = NULL;


    struct hash_elem *h = hash_insert (&thread_current()->sup_page_table, &spte->elem);
    if (h == NULL) {
        return NULL;
    }else {
        return spte;
    }

}

// bool free_sup_page (void *addr)
void free_sup_page (void *addr)
{
    //hash_delete (&thread_current()->sup_page_table, h)
    struct sup_page_table_entry *spte;
    struct hash_elem *h;
    // find and then free - change
    spte->user_vaddr = addr;
    h = hash_delete(&thread_current()->sup_page_table, &spte->elem);

    ASSERT(h != NULL);
    free(hash_entry(h, struct sup_page_table_entry, elem));

}


struct sup_page_table_entry *find_sup_page (void *addr)
{
    struct sup_page_table_entry *spte;
    struct sup_page_table_entry *hash_spte;
    struct hash_elem *h;

    spte->user_vaddr = addr;
    h = hash_find(&thread_current()->sup_page_table, &spte->elem);
    if(hash_spte != NULL){
         hash_spte= hash_entry(h, struct sup_page_table_entry, elem);
         return hash_spte;
    }else{
        return NULL;
    }
}
//user va
bool build_stack_spte(void * addr){

    ASSERT((PHYS_BASE - pg_round_down(upage)) > MAX_STACK_SIZE);

    struct frame_table_entry *fte = allocate_frame(addr, 1); //Clear the frame and then allocate
    if(fte){//add the page to the process addr space
        struct sup_page_table_entry *spte = allocate_sup_page(addr);
        if(spte){
            spte->writable = true;
            spte->kernel_vaddr = fte->frame;
            fte->spte = spte;
            fte->pinned = false;
            return true;
        }else{
            free_frame(fte->frame);
            return false;
        }
    }else{
        return false;
    }

}
