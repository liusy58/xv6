This repo contains all my codes and bugs I encountered during my work on mit6.s081.

Thank Robert Morris and all my friends who helped me debug and encourage me to continue.


## Lab4

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

