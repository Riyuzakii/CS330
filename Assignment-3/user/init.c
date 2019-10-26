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
/*
static void signal_handler(int signal)
{
    write(HANDLER, sizeof(HANDLER));
    exit(10);
}*/

static int main()
{
  unsigned long i, j;
#define MSG_1 "------------- Gonna sleep for 10 ticks. Check for pid = 0. -------------\n"
#define MSG_2 "------------- Gonna sleep for another 20 ticks. Check for pid = 0. ------------\n"
#define MSG_W "------------- Woke up. The process should now exit. Give 2 marks. -------------\n"
  write(MSG_1, sizeof(MSG_1));
  sleep(10);
  write(MSG_2, sizeof(MSG_2));
  sleep(20);
  write(MSG_W, sizeof(MSG_W));
  exit(10);
  return 0;
}
