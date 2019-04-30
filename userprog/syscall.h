#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "filesys/filesys.h"
#include "userprog/process.h"
#include <stdbool.h>
#include <debug.h>
#include <list.h>

typedef int mapid_t;

struct process_file {
    struct file* fileptr;     // file pointer
    struct thread* host_thread; //host thread
    int fd;                 // file descriptor
    struct list_elem pf_elem;  // list element
};

void syscall_init (void);

void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *file);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

void is_valid_memory_access(const uint32_t *vaddr);
static bool put_user (uint8_t *udst, uint8_t byte);
static int get_user (const uint8_t *uaddr);
static bool is_valid_fd(int fd);

#endif /* userprog/syscall.h */
