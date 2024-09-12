#include "kalloc.h"
#include "mmu.h"
#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

// https://stackoverflow.com/questions/38088732/explanation-to-aligned-malloc-implementation
void* aligned_malloc(uint required_bytes, uint alignment)
{
    void* p1; // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    if ((p1 = (void*)malloc(required_bytes + offset)) == 0)
    {
       return 0;
    }
    p2 = (void**)(((uint)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}

void aligned_free(void *p)
{
    free(((void**)p)[-1]);
}

int
thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)
{
  void *stack = aligned_malloc(PGSIZE, PGSIZE); 

  int res = clone(start_routine, arg1, arg2, stack);
  if (res < 0) {
    aligned_free(stack);
  }
  return res;
}

int 
thread_join() {
  void *stack;
  int res = join(&stack);
  free(stack);
  return res;
}

void lock_init(struct lock_t *lock) {
  lock->ticket = 0;
  lock->turn = 0;
}

// see https://en.wikipedia.org/w/index.php?title=Fetch-and-add#x86_implementation
static inline int fetch_and_add(int* variable, int value)
{
    asm volatile("lock; xaddl %0, %1"
      : "+r" (value), "+m" (*variable) // input + output
      : // No input-only
      : "memory"
    );
    return value;
}

void lock(struct lock_t *lock) {
  int myturn = fetch_and_add(&lock->ticket, 1);
  while (lock->turn != myturn)
    ;
}

void unlock(struct lock_t *lock) {
  ++lock->turn;
}
