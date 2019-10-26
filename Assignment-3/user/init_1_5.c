#include<init.h>
static void exit(int);
static int main(void);


void init_start()
{
  int retval = main();
  exit(0);
}

/*Invoke system call with no additional arguments*/
static long _syscall0(int syscall_num)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

/*Invoke system call with one argument*/

static long _syscall1(int syscall_num, int exit_code)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}
/*Invoke system call with two arguments*/

static long _syscall2(int syscall_num, u64 arg1, u64 arg2)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}


static void exit(int code)
{
  _syscall1(SYSCALL_EXIT, code); 
}

static long getpid()
{
  return(_syscall0(SYSCALL_GETPID));
}

static long write(char *ptr, int size)
{
   return(_syscall2(SYSCALL_WRITE, (u64)ptr, size));
}

u64 signal(int num, void *handler)
{
    return _syscall2(SYSCALL_SIGNAL, num, (u64)handler);
}

u64 alarm(int num)
{
    return _syscall1(SYSCALL_ALARM, num);
}

static void signal_handler(int signal)
{
#define HANDLER "-------------------- Give 0.5 marks if this line is printed EXACTLY ONCE. ---------------------\n"
    unsigned long rax, rbx, rcx, rdx, r8, r9, r10, r11, r12, r13, r14, r15;
    asm volatile(
		"mov %%rax, %0;"
		"mov %%rbx, %1;"
		"mov %%rcx, %2;"
		"mov %%rdx, %3;"
		"mov %%r8, %4;"
		"mov %%r9, %5;"
		"mov %%r10, %6;"
		"mov %%r11, %7;"
		"mov %%r12, %8;"
		"mov %%r13, %9;"
		"mov %%r14, %10;"
		"mov %%r15, %11;"
		: "=m" (rax), "=m" (rbx), "=m" (rcx), "=m" (rdx),
		  "=m" (r8), "=m" (r9), "=m" (r10), "=m" (r11),
		  "=m" (r12), "=m" (r13), "=m" (r14), "=m" (r15)
		:
		:"memory"
	    );
    write(HANDLER, sizeof(HANDLER));
    asm volatile(
		"mov %0, %%rax;"
		"mov %1, %%rbx;"
		"mov %2, %%rcx;"
		"mov %3, %%rdx;"
		"mov %4, %%r8;"
		"mov %5, %%r9;"
		"mov %6, %%r10;"
		"mov %7, %%r11;"
		"mov %8, %%r12;"
		"mov %9, %%r13;"
		"mov %10, %%r14;"
		"mov %11, %%r15;"
		:
		: "m" (rax), "m" (rbx), "m" (rcx), "m" (rdx),
		  "m" (r8), "m" (r9), "m" (r10), "m" (r11),
		  "m" (r12), "m" (r13), "m" (r14), "m" (r15)
		:"memory"
	    );
}

static int main()
{
#define NUM_TICKS 10
#define MSG ".......... wait for it ...........\n"
  unsigned long i, j;
  signal(SIGALRM, signal_handler); 
  alarm(10);
  for (i = 0; i < NUM_TICKS + 3; ++i) {
    write(MSG, sizeof(MSG));
  }
  exit(10);
  return 0;
}
