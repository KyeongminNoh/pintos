#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

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
  return;
};
void sys_exit (int status){
  struct thread *t = thread_current();
  struct list_elem *e;
  struct child *cData;
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

  printf("%s: exit(%d)\n", thread_current()->name, status);
  lock_release(&t->self_info->lock_wait);
  thread_exit();
};
pid_t sys_exec (const char *cmdline){
  return -1;
};
int sys_wait (pid_t pid){
  return process_wait(pid);
};
bool sys_create(const char* filename, unsigned initial_size){
  return false;
};
bool sys_remove(const char* filename){
  return false;
};
int sys_open(const char* file){
  return -1;
};
int sys_filesize(int fd){
  return -1;
};
void sys_seek(int fd, unsigned position){
  return -1;
};
unsigned sys_tell(int fd){
  return 0;
};
void sys_close(int fd){
   return;
};
int sys_read(int fd, void *buffer, unsigned size){
  return -1;
};
int sys_write(int fd, const void *buffer, unsigned size){
  if(fd == 1){
    putbuf(buffer, size);
    return size;
  }


  return -1;
};
