#include <hash.h>

#ifndef VM_FRAME_H
#define VM_FRAME_H

static struct lock frame_lock;
static struct hash frame_hash;

struct frame_table_entry
{
	uint32_t* frame; //20 bits to frame num, 12 bits to offset=> frame = kernel VA
	struct thread* owner; //pointer to the user
	struct sup_page_table_entry* spte;// pointer to the user (virtual memory) page= user VA
    bool pinned; // if true -> never evicted
    struct hash_elem helem;
};

void frame_init (void);
struct frame_table_entry* allocate_frame (void *addr, bool flag);
bool free_frame(void *frame) ;
struct frame_table_entry * find_frame(void * frame) ;
struct frame_table_entry* evict(void);

static unsigned fr_hash_func(const struct hash_elem *elem, void *aux);
static bool fr_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux );

#endif /* vm/frame.h */
