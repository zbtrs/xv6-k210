#include "include/vm.h"
#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/syscall.h"
#include "include/timer.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/uname.h"

extern int exec(char *path, char **argv);

uint64
sys_clone(void) 
{
  uint64 new_stack,new_fn;
  argaddr(1, &new_stack);
  if(new_stack == 0){
    return fork();
  }
  fetchaddr(new_stack, &new_fn);
  return clone(new_stack, new_fn);
}

uint64
sys_wait4() 
{
  uint64 addr;
  int pid,options;
  if (argint(0,&pid) < 0) {
    return -1;
  }
  if (argaddr(1,&addr) < 0) {
    return -1;
  }
  if (argint(2,&options) < 0) {
    return -1;
  }

  return wait4pid(pid,addr,options);
}

uint64
sys_exec(void)
{
  char path[FAT32_MAX_PATH], *argv[MAXARG];
  int i;
  uint64 uargv, uarg;

  if(argstr(0, path, FAT32_MAX_PATH) < 0 || argaddr(1, &uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv)){
      goto bad;
    }
    if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
      goto bad;
    }
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if(argv[i] == 0)
      goto bad;
    if(fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  int ret = exec(path, argv);

  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  return ret;

 bad:
  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);
  return -1;
}

//标准syscall参数相比于exec多了一个环境变量数组指针，因此新建了一个execve系统调用
//该函数相比于exec只是接收了环境变量数组参数，但还没有进行任何处理
uint64
sys_execve(void)
{
  char path[FAT32_MAX_PATH], *argv[MAXARG];
  // char *env[MAXARG];
  int i;
  uint64 uargv, uarg, uenv;

  if(argstr(0, path, FAT32_MAX_PATH) < 0 || argaddr(1, &uargv) < 0 || argaddr(2, &uenv)){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv)){
      goto bad;
    }
    if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
      goto bad;
    }
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if(argv[i] == 0)
      goto bad;
    if(fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  int ret = exec(path, argv);

  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  return ret;

 bad:
  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);
  return -1;
}

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64 sys_nanosleep(void) {
	uint64 addr_sec, addr_usec;

	if (argaddr(0, &addr_sec) < 0) 
		return -1;
	if (argaddr(1, &addr_usec) < 0) 
		return -1;

	struct proc *p = myproc();
	uint64 sec, usec;
	if (copyin(p->pagetable, (char*)&sec, addr_sec, sizeof(sec)) < 0) 
		return -1;
	if (copyin(p->pagetable, (char*)&usec, addr_usec, sizeof(usec)) < 0) 
		return -1;
	int n = sec * 20 + usec / 50000000;

	int mask = p->tmask;
	if (mask) {
		printf(") ...\n");
	}
	acquire(&p->lock);
	uint64 tick0 = ticks;
	while (ticks - tick0 < n / 10) {
		if (p->killed) {
			return -1;
		}
		sleep(&ticks, &p->lock);
	}
	release(&p->lock);

	return 0;
}


uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_getppid(void) 
{
  struct proc* p = myproc();
  acquire(&p->lock);
  uint64 ppid = p->parent->pid;
  release(&p->lock);

  return ppid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_brk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (n == 0)
  {
    return addr;
  }
  if (n >= addr)
  {
    if(growproc(n - addr) < 0)
      return -1;
    else return 0;
  }
  return -1;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int mask;
  if(argint(0, &mask) < 0) {
    return -1;
  }
  myproc()->tmask = mask;
  return 0;
}

uint64
sys_gettimeofday(void)
{
  uint64 tt;
  if (argaddr(0, &tt) < 0) 
		return -1;
  uint64 t = r_time();
  TimeSpec ts;
  ts.second = t / 1000000;
  ts.microSecond = t % 1000000;
  // printf("second: %d, microSecond: %d\n", ts.second, ts.microSecond);
  return copyout2(tt, (char*) &ts, sizeof(TimeSpec));
}

uint64
sys_uname(void)
{
  uint64 addr;
  if (argaddr(0, &addr) < 0) 
    return -1;
  /*
  char *sysname = "xv6";
  char *nodename = "xv6";
  char *release = "0.1";
  char *version = "0.1";
  char *machine = "x86_64";
  UtsName utsname;
  strncpy(utsname.sysname, sysname, sizeof(sysname));
  strncpy(utsname.nodename, nodename, sizeof(nodename));
  strncpy(utsname.release, release, sizeof(release));
  strncpy(utsname.version, version, sizeof(version));
  strncpy(utsname.machine, machine, sizeof(machine));
  return copyout2(addr, (char*) &utsname, sizeof(UtsName));
  */
  struct utsname *uts = (struct utsname*)addr;
  const char *sysname = "xv6-vf2";
  const char *nodename = "none";
  const char *release = "0.1";
  const char *version = "0.1";
  const char *machine = "QEMU";
  const char *domain = "none";
  if (either_copyout(1,(uint64)uts->sysname,(void*)sysname,sizeof(sysname)) < 0) {
    return -1;
  }

  if (either_copyout(1,(uint64)uts->nodename,(void*)nodename,sizeof(nodename)) < 0) {
    return -1;
  }

  if (either_copyout(1,(uint64)uts->release,(void*)release,sizeof(release)) < 0) {
    return -1;
  }

  if (either_copyout(1,(uint64)uts->version,(void*)version,sizeof(version)) < 0) {
    return -1;
  }

  if (either_copyout(1,(uint64)uts->machine,(void*)machine,sizeof(machine)) < 0) {
    return -1;
  }

  if (either_copyout(1,(uint64)uts->domainname,(void*)domain,sizeof(domain)) < 0) {
    return -1;
  }

  return 0;
}