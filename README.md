xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.828/, which
provides pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Silas Boyd-Wickizer, Anton Burtsev, Dan Cross, Cody Cutler, Mike CAT,
Tej Chajed, Asami Doi, eyalz800, , Nelson Elhage, Saar Ettinger, Alice
Ferrazzi, Nathaniel Filardo, Peter Froehlich, Yakir Goaron,Shivam
Handa, Bryan Henry, jaichenhengjie, Jim Huang, Alexander Kapshuk,
Anders Kaseorg, kehao95, Wolfgang Keller, Jonathan Kimmitt, Eddie
Kohler, Austin Liew, Imbar Marinescu, Yandong Mao, Matan Shabtay,
Hitoshi Mitake, Carmi Merimovich, Mark Morrissey, mtasm, Joel Nider,
Greg Price, Ayan Shafqat, Eldar Sehayek, Yongming Shen, Fumiya
Shigemitsu, Takahiro, Cam Tenny, tyfkda, Rafael Ubal, Warren Toomey,
Stephen Tu, Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas
Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy Zheng, and Zou Chang
Wei.

The code in the files that constitute xv6 is
Copyright 2006-2020 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
operating system for MIT's 6.S081, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu. Once they are installed, and in your shell
search path, you can run "make qemu".



- [ ] System call tracing
- [ ] Sysinfo
- [ ] print system call arguments(optional)
- [ ] Compute the load average and export it throygh sysinfo



### System call tracing ✅

The step of system call:

ecall -> trampoline -> usertrap() -> system call

* step 1
Add `$U/_trace` to UPROGS in Makefile

* step 2
add a prototype to `user.h`

```C
int trace(int);
```

* step 3
add a stub to `usys.pl` and system number to `syscall.h`

* step 4
add a filed in proc to store the mask passed by the user

* step 5
modify `syscall()` to print the message



### Sysinfo
* step 1
Add `$U/_sysinfotest` to UPROGS in Makefile

* step 2
add prototype 

* step 3
add `cal_freemem` function in `kalloc.c`

```C
uint64 cal_freemem(){
  struct run *r;
  uint64 freemem=0;
  r = kmem.freelist;
  acquire(&kmem.lock);
  while(r){
    freemem += PGSIZE;
    r = r->next;
  }
  release(&kmem.lock);
  return freemem;
}
```

* step 4
add `call_freemem` and `call_nproc` in `defs.h`

* step 5
copy out and need roubustness
use local variable instead of pointer to withdraw the value passed by caller.



记录一些问题：
* 为什么从内核态到用户态需要用到copyout？
因为内核态的页表和用户态的不是一样的，用户态页表和内核态的都需要进行映射，而用户态传过来的地址全部都是虚拟地址，所以需要手动转化为物理地址，再copy。

* 为什么我们在系统调用中需要那么麻烦的将传递的参数用很多函数比如`argaddr`中取出来呢？

因为在系统调用的整个过程，会存在很多的指令，所以我们先把从用户态传递过来的参数保存到trapframe里面去，因为如果不保存的话寄存器会被覆盖掉，而参数的传递一般在a0-a5寄存器中。


