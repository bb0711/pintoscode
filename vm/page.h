#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "filesys/file.h"
#include "threads/thread.h"



enum spte_type
{
    FILE=0,
    SWAP=1,
    MMAP=2
};

struct sup_page_table_entry 
{
    enum spte_type type;
	uint32_t* user_vaddr;
	uint32_t* kernel_vaddr;
	uint64_t access_time;

    // CPU never resets dirty, accessed bits to 0, but the OS may do so
	bool dirty;     // 1: on any write
	bool accessed;  // 1: On any read or write to a page
	bool writable;

	off_t ofs;

    struct file *file;
    size_t read_bytes;
    size_t zero_bytes;

    //struct list alias_frames;
	// struct frame_table_entry *fte;

	struct hash_elem elem;
};

void sup_page_init (struct thread * t);
struct sup_page_table_entry * allocate_sup_page (void *addr);
void free_sup_page (void *addr);
struct sup_page_table_entry *find_sup_page (void *addr);
bool build_stack_spte(void * addr fault_addr);

#endif /* vm/page.h */
