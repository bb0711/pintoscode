#include "vm/frame.h"
#include "vm/page.h"

#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/pte.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"


#define FRAME_OFFSET 0x00000fff    /* offset */
#define FRAME_NUM  0xfffff000   /* Frame number */

static struct lock frame_lock;
static struct lock evict_lock;
static struct hash frame_hash;

static unsigned fr_hash_func(const struct hash_elem *elem, void *aux UNUSED)
{
  struct frame_table_entry *fte = hash_entry(elem, struct frame_table_entry, helem);
  return hash_bytes( &fte->frame, sizeof fte->frame );
}

static bool fr_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  struct frame_table_entry *small = hash_entry(a, struct frame_table_entry, helem);
  struct frame_table_entry *big = hash_entry(b, struct frame_table_entry, helem);
  return ((small->frame) < (big->frame));
}

/*
 * Initialize frame table
 */
void 
frame_init (void)
{
    lock_init(&frame_lock);
    lock_init(&evict_lock)
    hash_init(&frame_hash, fr_hash_func, fr_less_func, NULL);

}


/* 
 * Make a new frame table entry for addr.
 */

 //check the addr is not used already!!
 //: user virtual memory(addr) -> kernel va(frame)
struct frame_table_entry*
allocate_frame (void *addr, bool flag)
{
    lock_acquire(&frame_lock);

    struct frame_table_entry *fte = malloc(sizeof(struct frame_table_entry));

    if (fte== NULL){
        lock_release(&frame_lock);
        return NULL;
    }
    //void *p =palloc_get_page(PAL_USER);// returns kernel virtual addr for page of user pool.
    void *p = palloc_get_page(PAL_USER | (flag ? PAL_ZERO: 0)
    if( p == NULL){
        printf("no memory, we should evict\n");
        fte=evict();
        if(fte==NULL){
            PANIC("really problem, eviction also made an error");
        }

        // we have to evict until getting the frame
        //after the eviction, if we cannot allocate page, that have to return false
    }else{
        struct hash_elem *h =hash_insert(&frame_hash, &fte->helem);
        if (h ==NULL){
            lock_release(&frame_lock);
            return NULL;
        }
    }
    fte->owner = thread_current();
    //fte->frame = p;
    fte->pinned = true;
    fte->spte = NULL;
    lock_release(&frame_lock);
    return fte;
    }

bool
free_frame(void *frame) // frame = kernel va
{
    ASSERT(pg_ofs(frame) == 0); // frame should be aligned

    struct frame_table_entry *fte2;
    fte2-> frame = frame;

    lock_acquire(&frame_lock);
    struct hash_elem *target_fr = hash_find(&frame_hash, &fte2->hash_elem);
    if(target_fr !=NULL){
        struct frame_table_entry *fte = hash_entry(target_fr, struct frame_table_entry, helem);
        palloc_free_page(frame);
        hash_delete(&frame_hash, &fte->helem);
        free(fte);
        free(fte2);
        lock_release(&frame_lock);
        return true;
    }else{
        lock_release(&frame_lock);
        return false;
    }
}

struct frame_table_entry *
find_frame(void * frame) //frame = kernel VA
{
    struct frame_table_entry * fte;
    struct frame_table_entry * tar_fte;
    struct hash_elem *tar;
    fte->frame = frame;
    tar = hash_find(&frame_hash, &fte->helem);
    if(tar != NULL){
         tar_fte= hash_entry(tar, struct frame_table_entry, helem);
         return tar_fte;
    }else{
        return NULL;
    }
}

//eviction must be executed before the frame allocation, not after frame allocation.
struct frame_table_entry*
evict(){
    struct hash_iterator it;
    //void * frame = NULL;
    //size_t hashSize = hash_size(&frame_hash);
    struct frame_table_entry *f = NULL;
    int i;
    lock_acquire(&evict_lock);
    //please prevent infinite loop
    /* second chance page replacement*/
    for(i=0; i<2; i++){
        hash_first (&it, &frame_hash);
        while (hash_next (&it))
        {
            f = hash_entry (hash_cur (&i), struct frame_table_entry, helem);
            if(f->pinned){
                continue;
            }else if(f->spte->accessed == true){
                //for next turn, so you can use it
                // can I change it by myself??
                //before this check we can swap!
                f->spte->accessed = false;
                continue;
            }
            f->spte = NULL;
            f->owner= thread_current();
            lock_release(&evict_lock);
            return f;
        }
    };
    lock_release(&evict_lock);
    return NULL;
}

