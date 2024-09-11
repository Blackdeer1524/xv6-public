#include "types.h"
#include "string.h"
#include "log.h"
#include "kalloc.h"
#include "defs.h"
#include "param.h"
#include "lapic.h"
#include "fs.h"
#include "swtch.h"
#include "mmu.h"
#include "x86.h"
#include "vm.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"
#include "assert.h"

int IS_CHILD_THREAD(struct proc *child, struct proc *parent) {
  ASSERT(parent != 0, "parent is null!");
  return child->pgdir == parent->pgdir;
}

#define DEFAULT_TICKETS 4;

struct ptable_t ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  for (int i = 0; i < NPROC; ++i) {
    ptable.pstats.inuse[i] = 0;
  }
  /* memset(&ptable.pstats, 0, sizeof(ptable.pstats)); */
  ptable.ticket_count = 0;
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;
  int pid;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) 
    if(p->state == UNUSED) {
      goto found;
    }

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  pid = p->pid = nextpid++;

  ptable.ticket_count += DEFAULT_TICKETS;

  ptable.pstats.inuse[p - ptable.proc] = 1;
  ptable.pstats.tickets[p - ptable.proc] = DEFAULT_TICKETS;
  ptable.pstats.pid[p - ptable.proc] = pid;
  ptable.pstats.ticks[p - ptable.proc] = 0;

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    ptable.ticket_count -= DEFAULT_TICKETS;
    ptable.pstats.inuse[p - ptable.proc] = 0;
    release(&ptable.lock);
    return 0;
  }
  release(&ptable.lock);

  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = 2 * PGSIZE;
  p->tf->eip = 0x1000;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, child_pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;

    acquire(&ptable.lock);
    ptable.pstats.inuse[ptable.proc - np] = 0;
    ptable.ticket_count -= ptable.pstats.tickets[np - ptable.proc];
    release(&ptable.lock);

    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  child_pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  const int parent_tickets = ptable.pstats.tickets[curproc - ptable.proc];
  ptable.ticket_count += parent_tickets - ptable.pstats.tickets[np - ptable.proc];
  ptable.pstats.tickets[np - ptable.proc] = parent_tickets;

  release(&ptable.lock);

  return child_pid;
}

int 
clone(void(*fcn)(void*, void *), void *arg1, void *arg2, void *stack)
{
  int i, child_pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  uint sp = (uint) stack + PGSIZE;
  sp -= 3 * 4;

  *(uint*)sp = 0xffffffff;
  *(uint*)(sp + 4) = (uint) arg1;
  *(uint*)(sp + 8) = (uint) arg2;

  np->pgdir = curproc->pgdir;
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  np->tf->ebp = (uint) stack + PGSIZE;
  np->tf->esp = sp;  
  np->tf->eip = (uint) fcn;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = curproc->ofile[i];
  np->cwd = curproc->cwd;

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  child_pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  const int parent_tickets = ptable.pstats.tickets[curproc - ptable.proc];
  ASSERT(parent_tickets > 0, "parent tickets are negative! [par:%d,cur:%d], par state: %d\n", curproc->pid, child_pid, curproc->state);
  ptable.pstats.tickets[np - ptable.proc] = parent_tickets;
  ptable.ticket_count += parent_tickets - DEFAULT_TICKETS

  release(&ptable.lock);

  return child_pid;
}

int
join(void **stack) 
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children threads.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      if (IS_CHILD_THREAD(p, curproc)) {
        havekids = 1;
        if (p->state == ZOMBIE) {
          // Found one.
          pid = p->pid;

          // Assuming that stack is one page in size and is page aligned
          *stack = (void *) PGROUNDDOWN(p->tf->esp);  

          kfree(p->kstack);
          p->kstack = 0;
          p->pid = 0;
          p->parent = 0;
          p->name[0] = 0;
          p->killed = 0;
          p->state = UNUSED;

          ptable.pstats.inuse[p - ptable.proc] = 0;
          release(&ptable.lock);
          return pid;
        }
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc(); 

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd); // also needed for threads
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      if (IS_CHILD_THREAD(p, curproc)) {
        // see how wait() syscall cleans up ZOMBIE procs
        p->cwd = 0;
        kfree(p->kstack);
        p->kstack = 0;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        ptable.pstats.inuse[p - ptable.proc] = 0;
        ptable.ticket_count -= ABS(ptable.pstats.tickets[p - ptable.proc]);
      } else {
        p->parent = initproc;
        if(p->state == ZOMBIE)
          wakeup1(initproc);
      }
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  
  ptable.ticket_count -= ptable.pstats.tickets[curproc - ptable.proc];

  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      if (!IS_CHILD_THREAD(p, curproc)) {
        havekids = 1;
        if (p->state == ZOMBIE) {
          // Found one.
          pid = p->pid;
          kfree(p->kstack);
          p->kstack = 0;
          freevm(p->pgdir);
          p->pid = 0;
          p->parent = 0;
          p->name[0] = 0;
          p->killed = 0;
          p->state = UNUSED;

          ptable.pstats.inuse[p - ptable.proc] = 0;
          release(&ptable.lock);
          return pid;
        }
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

static unsigned long randstate = 1;
static unsigned int
rand()
{
  randstate = randstate * 1664525 + 1013904223;
  return randstate;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    /* cprintf("tcount:%d\n", ptable.ticket_count); */
    
    ASSERT(ptable.ticket_count >= 0, "ticket count became negative: %d!\n", ptable.ticket_count)
    if (ptable.ticket_count > 0) {
      const int res = rand() % ptable.ticket_count;
      int sum = 0;
      for(int i = 0; i < NPROC; ++i){
        p = ptable.proc + i;
        if(p->state != RUNNABLE)
          continue;

        ASSERT(ptable.pstats.tickets[i] > 0, "%d has %d tickets even though it is RUNNABLE\n", p->pid, ptable.pstats.tickets[i]);
        sum += ptable.pstats.tickets[i];
        if (sum <= res) 
          continue;

        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        ++ptable.pstats.ticks[p - ptable.proc];

        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
    }

    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  
  ASSERT(ptable.lock.locked, "ptable is not locked when going to sleep!\n");
  ASSERT(p->state != SLEEPING, "double sleep has occured!\n");
  p->state = SLEEPING;
  ptable.ticket_count -= ptable.pstats.tickets[p - ptable.proc];

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan) {
      ptable.ticket_count += ptable.pstats.tickets[p - ptable.proc];
      p->state = RUNNABLE;
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING) {
        ptable.ticket_count += ptable.pstats.tickets[p - ptable.proc];
        p->state = RUNNABLE;
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]   = "unused",
  [EMBRYO]   = "embryo",
  [SLEEPING] = "sleep ",
  [RUNNABLE] = "runble",
  [RUNNING]  = "run   ",
  [ZOMBIE]   = "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s %d", p->pid, state, p->name, ptable.pstats.tickets[p - ptable.proc]);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
  cprintf("total ticket count: %d\n", ptable.ticket_count);
}
