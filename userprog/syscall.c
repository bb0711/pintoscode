#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/init.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "vm/frame.h"
#include "vm/page.h"


static void syscall_handler (struct intr_frame *);
struct lock sys_lock;

/*this is not included in filesys header so don't erase*/
struct file
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&sys_lock);
}

/*
if we have to return, put it to eax
*/
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * fesp = f -> esp;
  //int systemcall_num = *fesp;
  printf("syscall : %d\n", *fesp);
  hex_dump(f->esp, f->esp, 100, 1);

  switch (*fesp)
  {
    case SYS_HALT:                   /* Halt the operating system. */
        halt();
        break;
    case SYS_EXIT:                   /* Terminate this process. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        exit(*(uint32_t *)(fesp+1));
        break;
    case SYS_EXEC:                   /* Start another process. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        f->eax = exec((const char *)*(uint32_t *)(fesp+1));
        break;
    case SYS_WAIT:                   /* Wait for a child process to die. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        f->eax = wait((pid_t)*(uint32_t *)(fesp+1));
        break;
    case SYS_CREATE:                 /* Create a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        is_valid_memory_access((uint32_t *) (fesp+2));
        f->eax = create((const char *)*(uint32_t *)(fesp+1), (unsigned)*(uint32_t *)(fesp+2));
        break;
    case SYS_REMOVE:                 /* Delete a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        f->eax = remove ((const char *)*(uint32_t *)(fesp+1));
        break;
    case SYS_OPEN:                   /* Open a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        f->eax = open((const char *)*(uint32_t *)(fesp+1));
        break;
    case SYS_FILESIZE:               /* Obtain a file's size. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        f->eax = filesize((int)*(uint32_t *)(fesp+1));
        break;
    case SYS_READ:                   /* Read from a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        is_valid_memory_access((uint32_t *) (fesp+2));
        is_valid_memory_access((uint32_t *) (fesp+3));
        f->eax =read ((int)*(uint32_t *)(fesp+1), (void*)*(uint32_t *)(fesp+2), (unsigned)*(uint32_t *)(fesp+3));
        break;
    case SYS_WRITE:                  /* Write to a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        is_valid_memory_access((uint32_t *) (fesp+2));
        is_valid_memory_access((uint32_t *) (fesp+3));
        f->eax=write((int)*(uint32_t *)(fesp+1), (void*)*(uint32_t *)(fesp+2), (unsigned)*(uint32_t *)(fesp+3));
        break;
    case SYS_SEEK:                   /* Change position in a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        is_valid_memory_access((uint32_t *) (fesp+2));
        seek((int)*(uint32_t *)(fesp+1), (unsigned)*(uint32_t *)(fesp+2));
        break;
    case SYS_TELL:                   /* Report current position in a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        f->eax = tell((int)*(uint32_t *)(fesp+1));
        break;
    case SYS_CLOSE:                  /* Close a file. */
        is_valid_memory_access((uint32_t *) (fesp+1));
        close ((int)*(uint32_t *) (fesp+1));
        break;
  }
}

void
halt (void)
{
  power_off();
}

void
exit (int status)
{
  struct thread * cur =thread_current();
  cur->exit_num = status;
  int i;
  for (i = 3; i < 128; i++){
    if(thread_current()->fd_list[i] != -1){
       close(i);
    }
  }
  thread_exit();
}

pid_t
exec (const char *file)
{

  lock_acquire(&sys_lock);
  pid_t pid = process_execute(file);
  lock_release(&sys_lock);
  return pid;
}

int
wait (pid_t pid)
{
  return process_wait(pid);
}

bool
create (const char *file, unsigned initial_size)
{
  if(file == NULL){
    exit(-2);
    return false;
  }
  if(strlen(file)==0)
    return false;
  return filesys_create(file, initial_size);
}

bool
remove (const char *file)
{
  /* but you have to implement that after
   all of the file descriptors referring to this file or machine shut down
   you have to remove(free) this.
  */
  lock_acquire(&sys_lock);
  if( file == NULL){
    lock_release(&sys_lock);
    return false;
  }
  bool result= filesys_remove(file);
  lock_release(&sys_lock);
  return result;
}

int
open (const char *file)
{
  //printf("in open func, file: %s\n", file);
  if (file == NULL) return -1; //-10

  lock_acquire(&sys_lock);
  struct file * openfile = filesys_open (file);
  if (openfile == NULL){   // file could not be open
    lock_release(&sys_lock);
    return -3;
  }
  int i;

  for (i = 3 ; i < 128 ; i++){
    if (thread_current()->fd_list[i] == -1){
        //printf("openfile: NULL, file could not be open. | file: %s \n", file);
        thread_current()->open_files[i] = openfile;
        if (strcmp(thread_current()->name, file) == 0) {
            file_deny_write(openfile);
        }
        thread_current()->fd_list[i] = i;
        lock_release(&sys_lock);
        return i;
    }
  }
  lock_release(&sys_lock);
  return -3;
}


int
filesize (int fd)
{
  if(!is_valid_fd(fd))
      return -1;

  return file_length(thread_current()->open_files[fd]);
}

/*Reads size bytes from the file open as fd into buffer. Returns the number of bytes
actually read (0 at end of file), or -1 if the file could not be read (due to a condition
other than end of file). Fd 0 reads from the keyboard using input_getc().*/
int
read (int fd, void *buffer, unsigned size)
{
  //get buffer from user input
  is_valid_memory_access((const uint32_t*)(buffer +size -1));
  is_valid_memory_access((const uint32_t*)(buffer));

  lock_acquire(&sys_lock);
  if (fd == 0){
    int i;
    for(i=0; i<size; i+=1){
        if(! put_user((const uint8_t*)buffer+i, input_getc())){
            lock_release(&sys_lock);
            return -1;
        }
    }
    lock_release(&sys_lock);
    return i;

  }else if (is_valid_fd(fd)){
    void * rd = pg_round_down(buffer);
    void * pg;
    unsigned size_toread = (unsigned)(rd - buffer +PGSIZE);
    unsigned read_byte=0;
    for(pg= rd; pg <= buffer+size; pg += PGSIZE){
        struct sup_page_table_entry *spte = allocate_sup_page(pg);
        if(spte == NULL){
            //have to implement
        }
    }
    off_t result = file_read (thread_current()->open_files[fd], buffer, size);
    lock_release(&sys_lock);
    return result;
  }
  lock_release(&sys_lock);
  return -1;
}

// file descriptor, valid pointer to the buffer
//return the # of bytes actually written
//write size bytes from buffer to the open file fd
int
write (int fd, const void *buffer, unsigned size)
{

  is_valid_memory_access((const uint32_t*)(buffer +size -1));
  is_valid_memory_access((const uint32_t*)(buffer));

  int test = get_user((const uint32_t*)buffer);
  if (test == -1)
     return -1;

  lock_acquire(&sys_lock);

  // write to the console
  if (fd == 1 ){
    putbuf(buffer, size);
    lock_release(&sys_lock);
    return size; // !! we have to change to "real written size" !!
  }
  else if(is_valid_fd(fd)){ // !! implement!! write to the file

    if (thread_current()->fd_list[fd] == -1){
        lock_release(&sys_lock);
        return -1;
    }
    if (thread_current()->open_files[fd]->deny_write){
        file_deny_write(thread_current()->open_files[fd]);
        lock_release(&sys_lock);
        return 0; // not available to write something -> written 0
    }
    off_t result = file_write(thread_current()->open_files[fd], buffer, size);
    lock_release(&sys_lock);
    return result;
  }
  else{
    lock_release(&sys_lock);
    return -1;
  }
}

void
seek (int fd, unsigned position)
{
  lock_acquire(&sys_lock);
  if(!is_valid_fd(fd)){
    lock_release(&sys_lock);

  }else{
    file_seek(thread_current()->open_files[fd], position);
    lock_release(&sys_lock);
  }

}


/*Returns the position of the next byte to be read or written in open file fd, expressed
in bytes from the beginning of the file.*/

unsigned
tell (int fd)
{
  lock_acquire(&sys_lock);
  if(!is_valid_fd(fd)){
    lock_release(&sys_lock);
    exit(-1);
  }
  unsigned result =file_tell(thread_current()->open_files[fd]);
  lock_release(&sys_lock);
  return result;
}

void
close (int fd)
{
  lock_acquire(&sys_lock);

  if (is_valid_fd(fd) && thread_current()->open_files[fd] != NULL){
    file_close(thread_current()->open_files[fd]);
    thread_current()->fd_list[fd] = -1;
  }
  lock_release(&sys_lock);

  
}

//put this in the first of syscall_handler
// if you want to check the user's memory access is valid
void is_valid_memory_access(const uint32_t *vaddr){
    if ((const uint32_t *) vaddr >= PHYS_BASE || vaddr == 0)
        exit(-4);
}

//read a byte at the given user virtual address UADDR.
// return the byte value if successful, -1 if a segfault occurred
static int
get_user (const uint8_t *uaddr)
{
  ASSERT ((void*) uaddr < PHYS_BASE);
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
    int error_code;
    asm ("movl $1f, %0; movb %b2, %1; 1:"
        : "=&a" (error_code), "=m" (*udst) : "r" (byte));
    return error_code != -1;
}


static bool
is_valid_fd(int fd)
{
    if( fd >= 128 || fd < 0 || thread_current()->fd_list[fd]==-1 ){
        return false;
    }
    return true;
}