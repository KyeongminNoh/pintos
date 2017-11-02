#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "filesys/off_t.h"

typedef int pid_t;

static void syscall_handler (struct intr_frame *);

/* Check user memory valid */
void * esp_val(void *esp, int n);
bool check_user_mem(void *esp, int n);

/* System Calls */
void sys_halt (void);
void sys_exit (int);
pid_t sys_exec (const char *cmdline);
int sys_wait (pid_t pid);

bool sys_create(const char* filename, unsigned initial_size);
bool sys_remove(const char* filename);
int sys_open(const char* file);
int sys_filesize(int fd);
void sys_seek(int fd, unsigned position);
unsigned sys_tell(int fd);
void sys_close(int fd);
int sys_read(int fd, void *buffer, unsigned size);
int sys_write(int fd, const void *buffer, unsigned size);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_num ;
  void *esp = f->esp;
  int ret_val;
  syscall_num = *(int *)esp_val(esp, 0);

  if(!pagedir_get_page(thread_current()->pagedir, esp)) sys_exit(-1);


  switch( syscall_num) {
    case SYS_HALT:
      sys_halt();
      break;

    case SYS_EXIT:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1); 
      sys_exit( *(int *)esp_val(esp, 1) );
      break;

    case SYS_EXEC:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1);
      ret_val = sys_exec(* (const char **)esp_val(esp, 1) ); 
      break;

    case SYS_WAIT:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1);
      ret_val = sys_wait( * (pid_t *) esp_val(esp, 1) );
      break;

    case SYS_CREATE:
      if(!check_user_mem(esp, 2)) 
        sys_exit(-1);
      ret_val = sys_create(* (const char **) esp_val(esp, 1),* (unsigned*) esp_val(esp, 2) );
      break;

    case SYS_REMOVE:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1);
      ret_val = sys_remove(* (const char **) esp_val(esp, 1) );
      break;

    case SYS_OPEN:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1);
      ret_val = sys_open(* (const char **) esp_val(esp, 1) );
      break;

    case SYS_FILESIZE:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1);
      ret_val = sys_filesize(* (int*)esp_val(esp, 1)); 
      break;

    case SYS_READ:
      if(!check_user_mem(esp, 3)) 
        sys_exit(-1);
      ret_val = sys_read(* (int*) esp_val(esp, 1),* (void **)esp_val(esp, 2),* (unsigned*) esp_val(esp, 3) );
      break;

    case SYS_WRITE:
      if(!check_user_mem(esp, 3)) 
        sys_exit(-1);
      ret_val = sys_write(* (int*) esp_val(esp, 1),* (void **) esp_val(esp, 2), * (unsigned*) esp_val(esp, 3) );
      break;

    case SYS_SEEK:
      if(!check_user_mem(esp, 2)) 
        sys_exit(-1);
      sys_seek( *(int*) esp_val(esp, 1),* (unsigned*) esp_val(esp, 2));
      break;

    case SYS_TELL:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1);
      ret_val = sys_tell(*(unsigned *) esp_val(esp, 1));
      break;

    case SYS_CLOSE:
      if(!check_user_mem(esp, 1)) 
        sys_exit(-1);
      sys_close( * (int *) esp_val(esp, 1));
      break;

    default:
      sys_exit(-1);
      break;

  }
  f->eax = ret_val;

}

void * esp_val(void *esp, int n){
  return esp+4*n;
}

bool check_user_mem(void *esp, int n){
  return esp+4*n < PHYS_BASE;
}

/* Implement System Call functions */
void sys_halt (void){
   shutdown_power_off();
};
void sys_exit (int status){
  struct thread *t = thread_current();
  struct list_elem *e;
  struct child *cData;
  int i;
  if(t->self_info == NULL){
    printf("not self info\n");
    thread_exit();
  }
  t->self_info->status = status;

  for (e = list_begin(&t->child_list);e != list_end(&t->child_list);){
   
       cData = list_entry (e, struct child, child_elem);
       sys_wait(cData->tid);
   
       e = list_next(e);
       free(cData);
  } 
  for(i=2;i<128;i++)
  {
    free(t->thread_fd[i]);
  }

  printf("%s: exit(%d)\n", thread_current()->name, status);
  lock_release(&t->self_info->lock_wait);
  thread_exit();
};
pid_t sys_exec (const char *cmdline){
    
    pid_t pid;

    if(cmdline == NULL)
      return -1;
    pid = process_execute (cmdline);
    return pid;
};
int sys_wait (pid_t pid){
  return process_wait(pid);
};
bool sys_create(const char* filename, unsigned initial_size){

    if(filename == NULL)
        sys_exit(-1);

    return filesys_create(filename, initial_size);
};
bool sys_remove(const char* filename){
    
    if(filename == NULL)
        sys_exit(-1);
    
    return filesys_remove(filename);
};
int sys_open(const char* file){

    struct thread_fd* fd;
    struct thread *cur = thread_current();
    int i;

    if(file == NULL)
        sys_exit(-1);
    
    fd = calloc(1, sizeof(struct thread_fd)); 
    fd->file = filesys_open (file);//open
    
    if(fd->file == NULL)
    {
        free(fd);
        return -1;
    }
    for(i=2;i<128;i++)
    {
        if(cur->thread_fd[i] == NULL)
            break;
    }//find empty fd
    
    if(i == 128)//full
    return -1;
    
    fd->fd = i;
    cur->thread_fd[i] = fd;
    
    return i;
};
int sys_filesize(int fd){

    struct file * file ;
    struct thread * cur = thread_current();
    
    if(cur->thread_fd[fd] == NULL)
        return -1;

    file = cur->thread_fd[fd]->file;
    return file_length(file);
};
void sys_seek(int fd, unsigned position){
  
    struct thread *cur = thread_current();
    file_seek(cur->thread_fd[fd]->file, (off_t)position);//find page with filename, position 
};
unsigned sys_tell(int fd){
  
    struct thread * cur = thread_current();
    return file_tell(cur->thread_fd[fd]->file);  
};
void sys_close(int fd){
   struct thread * cur = thread_current();
    
    if(fd>128 || fd<2)
       sys_exit(-1);
    
   if(cur->thread_fd[fd] == NULL)
       return;

   //file_close(cur->pData->[fd]->file);
   // free(t->file_des[fd]);
   //t->file_des[fd] = NULL;
};
int sys_read(int fd, void *buffer, unsigned size){
  int read_size = 0;
  
  if(!(buffer < PHYS_BASE) || !((buffer+size-1) < PHYS_BASE))
      sys_exit(-1);

  if(fd<0 || fd>128)
      sys_exit(-1);

  if(thread_current()->thread_fd[fd] == NULL)
      return -1;

  if(fd == 0)
      for(read_size=0; read_size < size; read_size++)
      {
          *((char*)buffer++) = input_getc ();
      }
  else 
      {
        read_size = file_read(thread_current()->thread_fd[fd]->file, buffer, size);
      } 
  return read_size;
};

int sys_write(int fd, const void *buffer, unsigned size){
  int write_size =0;
  
  if(!(buffer < PHYS_BASE) || !((buffer+size-1) < PHYS_BASE))
    sys_exit(-1);

  if(fd<0 || fd>128)
    sys_exit(-1);

  if (fd == 1){
    putbuf(buffer, size);
    return size;
  }


  if(thread_current()->thread_fd[fd] == NULL)
    return -1;

  write_size = file_write(thread_current()->thread_fd[fd]->file, buffer, size);
   
  return write_size;
};
