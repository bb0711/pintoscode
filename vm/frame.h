#ifndef VM_FRAME_H
#define VM_FRAME_H

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
struct frame_table_entry* evict();

#endif /* vm/frame.h */
