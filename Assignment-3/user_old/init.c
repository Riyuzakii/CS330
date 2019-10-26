#include<init.h>
#include<memory.h>
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

static void signal(u64 signum, void* addr)
{
  _syscall2(SYSCALL_SIGNAL, signum,(long)addr) ;
}
static void alarm(u32 num)
{
  _syscall1(SYSCALL_ALARM, num) ;
}
static void sleep(u32 ticks)
{
  _syscall1(SYSCALL_SLEEP, ticks) ;
}
static void clone(void* func_addr,void* st_addr)
{
  _syscall2(SYSCALL_CLONE, (long)func_addr,(long)st_addr) ;
}
static char* expand(u64 size,u64 flags)
{
  return (char*)_syscall2(SYSCALL_EXPAND,size,flags);
}

static void fun2(){
  write("Bye Bye\n",9);
  exit(0);
}

static void fun1(){
  write("Hello Hello\n",12);
  exit(0);
}

static void fun3(){
  write("Once Again\n",12);
  exit(0);
}

static int main()
{
  char* ptr1 = expand(20,MAP_WR);
 // char* ptr2 = expand(20,MAP_WR);
  // char* ptr3 = expand(20,MAP_WR); 
  clone(&fun1,(char*)(((((u64)ptr1)>>12)+2)<<12));
 // clone(&fun2,(char*)(((((u64)ptr2)>>12)+2)<<12)-1);
 // clone(&fun3,(char*)(((((u64)ptr3)>>12)+2)<<12)-1);
  write("back to main\n",20);
  exit(0);

  //signal(SIGFPE,&fun);
  //int a =1/0;
  //exit(0);
  //char *ptr = (char *)expand(20, MAP_WR);
  //clone(&fun,ptr);
  //write("a\n",2);
  //return 0;
  /*unsigned long i, j;
  unsigned long buff[4096];
  i = getpid();

  for(i=0; i<4096; ++i){
      j = buff[i];
  }
  i=0x100034;
  j = i / (i-0x100034);
  exit(-5);*/
}
