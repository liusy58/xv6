This repo contains all my codes and bugs I encountered during my work on mit6.s081.

Thank Robert Morris and all my friends who helped me debug and encourage me to continue.

- [x] Lab1 Xv6 and Unix utilities
- [x] Lab2
- [x] Lab3
- [x] Lab4
- [x] Lab5
- [x] Lab6
- [x] Lab7
- [x] Lab8
- [x] Lab9



## Lab1 Xv6 and Unix utilities


### sleep (easy)

---
Implement the UNIX program sleep for xv6; your sleep should pause for a user-specified number of ticks. A tick is a notion of time defined by the xv6 kernel, namely the time between two interrupts from the timer chip. Your solution should be in the file `user/sleep.c`.

---

* Obtain the command-line arguments passed to a program.


> Once I wonder why we need to obtain the command-line arguments in such a tedious way. At that time I thought all arguments are passed by stack, so we can just get them from the stack but when I look at the book I found that xv6 pass most arguments by register so a syscall cause the user application to enter the kernel but this procedure will overwrite the registers so when in kernel space we need to extract the arguments from the trapframe where all the registers are stored.

* Design user interface to call `sys_sleep`

```C
+#include "kernel/types.h"
+#include "kernel/stat.h"
+#include "user/user.h"
+
+void help(){
+    printf("Usage : sleep [ cnt ]\n");
+}
+
+int
+main(int argc, char *argv[])
+{
+  if(argc != 2){
+      help();
+      exit(0);
+  }
+  int cnt;
+  cnt = atoi(argv[1]);
+  sleep(cnt);
+  exit(0);
+}
```
> At this point you may wonder how can we design a syscall in xv6. From the `usys.pl` script, and `syscall.c` file, we can know that we use ecall instruction to sink into the kernel and distinguish syscalls by a7 register which stores the syscall number. So we need to declare a user syscall stub in  `usys.pl` in order to replace all these stubs such as sleep exit by assembly language inserted to the C files.

* Add your sleep program to UPROGS in Makefile; 

```C
+	$U/_sleep\
```


## pingpong (easy)

---

Write a program that uses UNIX system calls to ''ping-pong'' a byte between two processes over a pair of pipes, one for each direction. The parent should send a byte to the child; the child should print "<pid>: received ping", where <pid> is its process ID, write the byte on the pipe to the parent, and exit; the parent should read the byte from the child, print "<pid>: received pong", and exit. Your solution should be in the file `user/pingpong.c`.

---

> When writing this function the question puzzled me most is that How can I order the output in that order that child proccess output first and the parent later on. At last I found that the read call will lock the process if the write end doesn't write anything and not close yet.

```C
+#include "kernel/types.h"
+#include "kernel/stat.h"
+#include "user/user.h"
+
+int
+main(int argc, char *argv[])
+{
+    int p[2];
+    char buf[10];
+    pipe(p);
+    int pid;
+    pid = fork();
+    if(pid<0){
+        printf("error\n");
+        exit(0);
+    }else if(pid == 0){
+        read(p[0],buf,sizeof(buf));
+        printf("%d: received %s\n",getpid(),buf);
+        write(p[1],"pong",5);
+        close(p[0]);
+        close(p[1]);
+        exit(0);
+    }else{
+        write(p[1],"ping",5);
+        wait(0);
+        read(p[0],buf,sizeof(buf));
+        printf("%d: received %s\n",getpid(),buf);
+        close(p[0]);
+        close(p[1]);
+    }
+    exit(0);
+}

```

### primes (moderate)/(hard)

---
Write a concurrent version of prime sieve using pipes. This idea is due to Doug McIlroy, inventor of Unix pipes. The picture halfway down [this page](https://swtch.com/~rsc/thread/) and the surrounding text explain how to do it. Your solution should be in the file `user/primes.c`.

Your goal is to use pipe and fork to set up the pipeline. The first process feeds the numbers 2 through 35 into the pipeline. For each prime number, you will arrange to create one process that reads from its left neighbor over a pipe and writes to its right neighbor over another pipe. Since xv6 has limited number of file descriptors and processes, the first process can stop at 35.

---

```C
+#include "kernel/types.h"
+#include "kernel/stat.h"
+#include "user/user.h"
+
+int min = 2; 
+int
+main(int argc, char *argv[])
+{
+    int _pipe[2][2];
+    pipe(_pipe[0]);
+    int index = 0;
+    int num;
+    for(int i=min;i<35;++i){
+        write(_pipe[index][1],&i,4);
+    }
+    close(_pipe[0][1]);
+    while(fork()==0){
+        if(read(_pipe[index][0],&min,4)!=0){
+            printf("prime %d\n",min);
+        }else{
+            exit(0);
+        }
+        pipe(_pipe[index^1]);
+        while(read(_pipe[index][0],&num,4)!=0){
+            if(num%min){
+                write(_pipe[index^1][1],&num,4);
+            }
+        }
+        close(_pipe[index^1][1]);
+        index = index^1;
+    }
+    close(_pipe[index][0]);
+    wait(0);
+    exit(0);
+}
```


### find (moderate) + Support regular expressions in name matching

---
Write a simple version of the UNIX find program: find all the files in a directory tree with a specific name. Your solution should be in the file user/find.c.

---


> Very direct. So I just want to point out that my ismatch function is very nice and enjoy it~


```C
+#include "kernel/types.h"
+#include "kernel/stat.h"
+#include "user/user.h"
+#include "kernel/fs.h"
+
+int ismatch(char*s,char*p){
+  int advance = 1 ;//advance p
+  if(*p == 0)
+    return *s == 0;
+  if(*p && *(p+1) && *(p+1)=='*'){
+    if(ismatch(s,p+2))
+      return 1;
+    advance = 0;
+  }
+  if((*s&&*p=='.')||*s==*p)
+    return ismatch(s+1,p+advance);
+  return 0;
+}
+
+
+char buf[512];
+char*
+fmtname(char *path)
+{
+  static char buf[DIRSIZ+1];
+  char *p;
+
+  // Find first character after last slash.
+  for(p=path+strlen(path); p >= path && *p != '/'; p--)
+    ;
+  p++;
+
+  // Return blank-padded name.
+  if(strlen(p) >= DIRSIZ)
+    return p;
+  memmove(buf, p, strlen(p));
+  memset(buf+strlen(p), '\0', DIRSIZ-strlen(p));
+  return buf;
+}
+
+void
+find(char *path,char*name)
+{
+  char *p;
+  int fd;
+  struct dirent de;
+  struct stat st;
+
+  if((fd = open(path, 0)) < 0){
+    fprintf(2, "ls: cannot open %s\n", path);
+    return;
+  }
+
+  if(fstat(fd, &st) < 0){
+    fprintf(2, "ls: cannot stat %s\n", path);
+    close(fd);
+    return;
+  }
+
+  switch(st.type){
+  case T_FILE:
+    if(ismatch(fmtname(path),name)!=0){
+        printf("%s\n",path);
+    }
+    break;
+
+  case T_DIR:
+    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
+      printf("ls: path too long\n");
+      break;
+    }
+    strcpy(buf, path);
+    p = buf+strlen(buf);
+    *p++ = '/';
+    while(read(fd, &de, sizeof(de)) == sizeof(de)){
+      if(de.inum == 0 || strcmp(de.name,".")==0 || strcmp(de.name,"..")==0)
+        continue;
+      memmove(p, de.name, DIRSIZ);
+      p[DIRSIZ] = 0;
+      find(buf,name);
+    }
+    break;
+  }
+  close(fd);
+}
+
+int
+main(int argc, char *argv[])
+{
+  find(argv[1],argv[2]);
+
+  exit(0);
+}

```


### xargs (moderate)


---
Write a simple version of the UNIX xargs program: read lines from the standard input and run a command for each line, supplying the line as arguments to the command. Your solution should be in the file user/xargs.c.

---

> When I write this function I have nothing about xargs because I have never used it before. So I look up on the Internet and found that it just use the former command outputs as the input for the next command and the input is seperated by `\n` or blankspace

```C
+#include "kernel/types.h"
+#include "kernel/stat.h"
+#include "user/user.h"
+#include "kernel/fs.h"
+#include "kernel/param.h"
+
+int
+main(int argc, char *argv[]){
+    int index = 0;
+    int _argc = 1;
+    char *_argv[MAXARG];
+    if(strcmp(argv[1],"-n")==0){
+        index = 3;
+    }else{
+        index = 1;
+    }
+     _argv[0] = malloc(strlen(argv[index])+1);
+    strcpy(_argv[0],argv[index]);
+    for(int i=index+1;i<argc;++i){
+        //printf("--%s--\n",argv[i]);
+        _argv[_argc] = malloc(strlen(argv[i])+1);
+        strcpy(_argv[_argc++],argv[i]);
+    }
+    _argv[_argc] = malloc(128);
+
+    char buf;
+    int i =0;
+    while(read(0,&buf,1)){
+        if(buf=='\n'){
+            _argv[_argc][i++]='\0';
+            if(fork()==0){
+                exec(argv[index],_argv);
+            }else{
+                i=0;
+                wait(0);
+            }
+        }else{
+            _argv[_argc][i++]=buf;
+        }
+    }
+    exit(0);
+}
+

```











## Lab4 Trap

### Backtrace

* Add the prototype for backtrace to `kernel/defs.h`

```C
+void            backtrace();
```

*  Add the following function to `kernel/riscv.h`

```
+
+static inline uint64
+r_fp()
+{
+  uint64 x;
+  asm volatile("mv %0, s0" : "=r" (x) );
+  return x;
+}
+
```


* design backtrace function


since the stack grows to the lower address,so the stack bottom can be calculated by `PGROUNDUP(fp)` and by the xv6 memory layout we can know that  the return address lives at a fixed offset (-8) from the frame pointer of a stackframe, and that the saved frame pointer lives at fixed offset (-16) from the frame pointer.


```C
+
+void backtrace(){
+  printf("backtrace:\n");
+  uint64 fp = r_fp();
+  uint64 stack_bottom = PGROUNDUP(fp);
+  while(fp<stack_bottom){
+    //printf("fp : %p\n",fp);
+    uint64 return_address = *(uint64*)(fp-8);
+    printf("%p\n",return_address);
+    fp = *(uint64*)(fp-16);
+  }
+}

```

* add `backtrace` to `sys_sleep`

```C
+  backtrace();
```


### Alarm (hard)

* add `alarmtest.c` to `Makefile`

* add two syscall `sigalarm` and `sigreturn`
    * `syscall.c`

    ```C
    +extern uint64 sys_sigalarm(void);
    +extern uint64 sys_sigreturn(void);
    +
    +
    +[SYS_sigalarm]   sys_sigalarm,
    +[SYS_sigreturn]   sys_sigreturn,
    ```

    * `syscall.h`
    ```C
    +#define SYS_sigalarm  22
    +#define SYS_sigreturn  23
    ```
    * `user.h`
    ```C
    +int sigalarm(int ticks, void (*handler)());
    +int sigreturn(void);
    ```

    * `usys.pl`
    ```
    +entry("sigreturn");
    +entry("sigalarm"); 
    ```

* add new fields to `proc` structure
```
+  int is_sigalarm;
+  int ticks;
+  int now_ticks;
+  uint64 handler;
+  struct trapframe *trapframe_copy;
```

Why we need these new variables?

Since we need to mark how many ticks have passed we declare `now_ticks` adn we use `kicks` to store the value passed by syscall and we use `hanlder` to store the handler function address. The most hard part to understand is why we need a new `trapframe` structure. I think we can think this way, once the handler function has expired time interupt can still occur and in this way we store the variables at that time(when executing the handler function), so the variables we store to trapframe when we first expire the handler function are overwritten so we need a new trapframe to store the registers when first expire handler function.

* initilize the variables and withdraw them.
    * `proc.c`
    ```C
    allocproc:
    +  if((p->trapframe_copy = (struct trapframe *)kalloc()) == 0){
    +    release(&p->lock);
    +    return 0;
    +  }

    +  p->is_sigalarm=0;
    +  p->ticks=0;
    +  p->now_ticks=0;
    +  p->handler=0;


    freeproc:
    +  if(p->trapframe_copy)
    +    kfree((void*)p->trapframe_copy);
   p->trapframe = 0;

    +uint64 sys_sigalarm(void){
    +  int ticks;
    +  if(argint(0, &ticks) < 0)
    +    return -1;
    +  uint64 handler;
    +  if(argaddr(1, &handler) < 0)
    +    return -1;
    +  myproc()->is_sigalarm =0;
    +  myproc()->ticks = ticks;
    +  myproc()->now_ticks = 0;
    +  myproc()->handler = handler;
    +  return 0; 
    ```

    * 


*  if the process has a timer outstanding then expire the handler function.
    * trap.c
    ```C
    -  if(which_dev == 2)
    -    yield();
    +  if(which_dev == 2){
    +    p->now_ticks+=1;
    +    if(p->ticks>0&&p->now_ticks>=p->ticks&&!p->is_sigalarm){
    +      p->now_ticks = 0;
    +      p->is_sigalarm=1;
    +      *(p->trapframe_copy)=*(p->trapframe);
    +      p->trapframe->epc=p->handler;
    +    }
    +  yield();
    +  }

    ```

* design the return function

    * `sysproc.c`
    ```C
    +
    +void restore(){
    +  struct proc*p=myproc();
    +
    +  p->trapframe_copy->kernel_satp = p->trapframe->kernel_satp;
    +  p->trapframe_copy->kernel_sp = p->trapframe->kernel_sp;
    +  p->trapframe_copy->kernel_trap = p->trapframe->kernel_trap;
    +  p->trapframe_copy->kernel_hartid = p->trapframe->kernel_hartid;
    +  *(p->trapframe) = *(p->trapframe_copy);
    +}
    +
    +uint64 sys_sigreturn(void){
    +  restore();
    +  myproc()->is_sigalarm = 0;
    +  return 0;
    +}
    +
    +
    ```
we shouldn't directly restore all variables to traframe because all kernel stack and something other are used for public(becuase if we restore kernel stack we may encounter error).



## Lab6 Copy-on-Write Fork for xv6

### Implement copy-on write(hard)

---
Your task is to implement copy-on-write fork in the xv6 kernel. You are done if your modified kernel executes both the cowtest and usertests programs successfully.

---

* Modify uvmcopy() to map the parent's physical pages into the child, instead of allocating new pages. Clear PTE_W in the PTEs of both child and parent. 

---
That's to say, we need to make those pages of parents which is marked as writable unwritable and use a new bit to mark them as `COW` page. By doing so we can share readable pages between parents and children and when need to write on `COW` pages we allocate new pages.

---

```C
-  char *mem;
 
   for(i = 0; i < sz; i += PGSIZE){
     if((pte = walk(old, i, 0)) == 0)
@@ -320,18 +320,26 @@ uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
       panic("uvmcopy: page not present");
     pa = PTE2PA(*pte);
     flags = PTE_FLAGS(*pte);
-    if((mem = kalloc()) == 0)
-      goto err;
-    memmove(mem, (char*)pa, PGSIZE);
-    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
-      kfree(mem);
+    if(flags&PTE_W){
+      flags = (flags&(~PTE_W))|PTE_C;
+      *pte = PA2PTE(pa)|flags;
+    }
+    if(mappages(new, i, PGSIZE, pa, flags) != 0){
       goto err;
     }
+    inc_page_ref((void*)pa);
+    // if((mem = kalloc()) == 0)
+    //   goto err;
+    // memmove(mem, (char*)pa, PGSIZE);
+    // if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
+    //   kfree(mem);
+    //   goto err;
+    // }

```

* Modify usertrap() to recognize page faults. When a page-fault occurs on a COW page, allocate a new page with kalloc(), copy the old page to the new page, and install the new page in the PTE with PTE_W set.

---
Note, this only cares the current process so we just allocate a page for the cow page and map the new page to the pagetable. We need call  `kfree()` to free the previous page which is a cow page if no process owns it.

---


```C
-void
+// -1 means cannot alloc mem
+// -2 means the address is invalid
+// 0 means ok
+int page_fault_handler(void*va,pagetable_t pagetable){
+ 
+  struct proc* p = myproc();
+  if((uint64)va>=MAXVA||((uint64)va>=PGROUNDDOWN(p->trapframe->sp)-PGSIZE&&(uint64)va<=PGROUNDDOWN(p->trapframe->sp))){
+    return -2;
+  }
+
+  pte_t *pte;
+  uint64 pa;
+  uint flags;
+  va = (void*)PGROUNDDOWN((uint64)va);
+  pte = walk(pagetable,(uint64)va,0);
+  if(pte == 0){
+    return -1;
+  }
+  pa = PTE2PA(*pte);
+  if(pa == 0){
+    return -1;
+  }
+  flags = PTE_FLAGS(*pte);
+  if(flags&PTE_C){
+    flags = (flags|PTE_W)&(~PTE_C);
+    char*mem;
+    mem = kalloc();
+    if(mem==0){
+      return -1;
+    }
+    memmove(mem,(void*)pa,PGSIZE); 
+    *pte = PA2PTE(mem)|flags;
+    kfree((void*)pa);
+    return 0;
+  }
+  return 0;
+}
+
+
+void 
 trapinit(void)
 {
   initlock(&tickslock, "time");
@@ -67,7 +106,12 @@ usertrap(void)
     syscall();
   } else if((which_dev = devintr()) != 0){
     // ok
-  } else {
+  }else if(r_scause()==15||r_scause()==13){
+    int res = page_fault_handler((void*)r_stval(),p->pagetable);
+    if(res == -1 || res==-2){
+      p->killed=1;
+    }
+  }else {
     printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
     printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
     p->killed = 1;

```

* Ensure that each physical page is freed when the last PTE reference to it goes away -- but not before. A good way to do this is to keep, for each physical page, a "reference count" of the number of user page tables that refer to that page. Set a page's reference count to one when kalloc() allocates it. Increment a page's reference count when fork causes a child to share the page, and decrement a page's count each time any process drops the page from its page table. kfree() should only place a page back on the free list if its reference count is zero. It's OK to to keep these counts in a fixed-size array of integers. You'll have to work out a scheme for how to index the array and how to choose its size. For example, you could index the array with the page's physical address divided by 4096, and give the array a number of elements equal to highest physical address of any page placed on the free list by kinit() in kalloc.c.



```C
+struct {
+  struct spinlock lock;
+  int count[PGROUNDUP(PHYSTOP)>>12];
+} page_ref;
+
+void init_page_ref(){
+  initlock(&page_ref.lock, "page_ref");
+  acquire(&page_ref.lock);
+  for(int i=0;i<(PGROUNDUP(PHYSTOP)>>12);++i)
+    page_ref.count[i]=0;
+  release(&page_ref.lock);
+}
+
+
+void dec_page_ref(void*pa){
+  acquire(&page_ref.lock);
+  if(page_ref.count[(uint64)pa>>12]<=0){
+    panic("dec_page_ref");
+  }
+  page_ref.count[(uint64)pa>>12]-=1;
+  release(&page_ref.lock);
+}
+
+void inc_page_ref(void*pa){
+  acquire(&page_ref.lock);
+  if(page_ref.count[(uint64)pa>>12]<0){
+    panic("inc_page_ref");
+  }
+  page_ref.count[(uint64)pa>>12]+=1;
+  release(&page_ref.lock);
+}
+
+int get_page_ref(void*pa){
+  acquire(&page_ref.lock);
+  int res = page_ref.count[(uint64)pa>>12];
+  if(page_ref.count[(uint64)pa>>12]<0){
+    panic("get_page_ref");
+  }
+  release(&page_ref.lock);
+  return res;
+}
+
+
 void
 kinit()
 {
+  init_page_ref();
   initlock(&kmem.lock, "kmem");
   freerange(end, (void*)PHYSTOP);
 }
@@ -35,8 +79,10 @@ freerange(void *pa_start, void *pa_end)
 {
   char *p;
   p = (char*)PGROUNDUP((uint64)pa_start);
-  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
+  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
+    inc_page_ref(p);
     kfree(p);
+  }
 }
 
 // Free the page of physical memory pointed at by v,
@@ -47,10 +93,18 @@ void
 kfree(void *pa)
 {
   struct run *r;
-
   if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
     panic("kfree");
-
+  acquire(&page_ref.lock);
+  if(page_ref.count[(uint64)pa>>12]<=0){
+    panic("dec_page_ref");
+  }
+  page_ref.count[(uint64)pa>>12]-=1;
+  if(page_ref.count[(uint64)pa>>12]>0){
+    release(&page_ref.lock);
+    return;
+  }
+  release(&page_ref.lock);
   // Fill with junk to catch dangling refs.
   memset(pa, 1, PGSIZE);
 
@@ -76,7 +130,9 @@ kalloc(void)
     kmem.freelist = r->next;
   release(&kmem.lock);
 
-  if(r)
+  if(r){
     memset((char*)r, 5, PGSIZE); // fill with junk
+    inc_page_ref((void*)r);
+  }
   return (void*)r;
 }

```

At this point, I have made many mistakes such as:

* forget to increase the ref_count in `freerange()` since `freerange()` will call `kfree()` and `free()` will decrease the count so we need to first increase then  decrease.
* I first write a version `kfree()` as follow:
    ```C
    dec_page_ref(pa);
    if(get_page_ref(pa)>0){
        return ;
    }
    ```
this can cause many mistakes because there is a gap between the two call `dec_page_ref()` and `get_page_ref()` so if two process call `kfree()` to free the COW page but the procedure can be like this: a.dec,b.dec,b.get,b.get so this may cause the page to repeate inserting to the list.

* Modify copyout() to use the same scheme as page faults when it encounters a COW page.

```C
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
 {
-  uint64 n, va0, pa0;
-
+  uint64 n, va0, pa0,flags;
+  pte_t *pte;
   while(len > 0){
     va0 = PGROUNDDOWN(dstva);
     pa0 = walkaddr(pagetable, va0);
     if(pa0 == 0)
       return -1;
+    pte = walk(pagetable,va0,0);
+    flags=PTE_FLAGS(*pte);
+    if(flags&PTE_C){
+      page_fault_handler((void*)va0,pagetable);
+      pa0 = walkaddr(pagetable,va0);
+    }
     n = PGSIZE - (dstva - va0);
     if(n > len)
       n = len;
```

