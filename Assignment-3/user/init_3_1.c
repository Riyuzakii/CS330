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

u64 sleep(int ticks)
{
    return _syscall1(SYSCALL_SLEEP, ticks);
}

static long expand(unsigned size, int flags) {
	return (long)_syscall2(SYSCALL_EXPAND, size, flags);
}

int clone(void (func)(void), long stack_addr)
{
    return _syscall2(SYSCALL_CLONE, (u64)func, stack_addr);
}

static void signal_handler(int signal)
{
#define HANDLER "---------------- Give 0.5 marks if there was a segmentation fault prior to this and process exits with code 10. -------------------\n"
    write(HANDLER, sizeof(HANDLER));
    exit(10);
}

#define MSG "------------------------------- This line is from pid = "
#define DECLARE_MSG(pid) \
    char msg[sizeof(MSG) + 3] = MSG;    \
    msg[sizeof(MSG)-1] = '0' + (char)pid; \
    msg[sizeof(MSG)] = '\n';

static void thread(void)
{
    long pid = getpid();
    DECLARE_MSG(pid)
    while (1) {
        write(msg, sizeof(msg));
    }
}

#define MAP_WR  0x1
static int main()
{
  unsigned long i, j;
  long stack1 = expand(1, MAP_WR);
  long stack2 = expand(1, MAP_WR);
  long stack3 = expand(1, MAP_WR);
  *(long *)stack1 = *(long *)stack2 = *(long *)stack3 = 0;
  long pid = getpid();
  DECLARE_MSG(pid)
  clone(thread, stack1);
  clone(thread, stack2);
  clone(thread, stack3);
  while (1) {
      write(msg, sizeof(msg));
  }
  return 0;
}
