#ifndef _PROC_H_
#define _PROC_H_

#include "types.h"
#include "param.h"
#include "mmu.h"

#include "pstat.h"
#include "spinlock.h"

// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};

struct ptable_t {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct pstat pstats;
  int ticket_count;
};

extern struct ptable_t ptable;

int          cpuid(void);
void         exit(void);
int          fork(void);
int          growproc(int);
int          kill(int);
struct cpu*  mycpu(void);
struct proc* myproc();
void         pinit(void);
void         procdump(void);
void         scheduler(void) __attribute__((noreturn));
void         sched(void);
void         setproc(struct proc*);
void         sleep(void*, struct spinlock*);
void         userinit(void);
int          wait(void);
void         wakeup(void*);
void         yield(void);

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

#endif
