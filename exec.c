#include "types.h"
#include "param.h"
#include "string.h"
#include "fs.h"
#include "console.h"
#include "vm.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "elf.h"
#include "log.h"
#include "vm.h"
#include "console.h"
#include "assert.h"

int
exec(char *path, char **argv)
{
  cprintf("execing %s...\n", path);
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf)) {
    cprintf("bad elf header\n");
    goto bad;
  }
  if(elf.magic != ELF_MAGIC) {
    cprintf("ELF_MAGIC error\n");
    goto bad;
  }

  if((pgdir = setupkvm()) == 0)  {
    cprintf("couldn't set up kvm\n");
    goto bad;
  }

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph)) {
      cprintf("couldn't read programm header\n");
      goto bad;
    }
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz) {
      cprintf("memsz < filesz\n");
      goto bad;
    }
    if(ph.vaddr + ph.memsz < ph.vaddr) {
      cprintf("memsz is too large\n");
      goto bad;
    }
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz - PGSIZE)) == 0) {
      cprintf("could't allocate uvm\n");
      goto bad;
    }
    if(ph.vaddr % PGSIZE != 0) {
      cprintf("virtual address is not page aligned\n");
      goto bad;
    }
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0) {
      cprintf("could't load from elf\n");
      goto bad;
    }
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0) {  
    cprintf("could't allocate two pages\n");
    goto bad;
  }
  clearpteu(pgdir, (char*)((sz + PGSIZE) - 2*PGSIZE));
  sp = sz + PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;  // ... & ~3 is needed for alignment?
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0) {
      cprintf("couldn't copy argv arg\n");
      goto bad;
    }
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer | *4 = *sizeof(int) ?

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0) {
    cprintf("couldn't copy from ustack\n");
    goto bad;
  }

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  freevm(oldpgdir);

  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
