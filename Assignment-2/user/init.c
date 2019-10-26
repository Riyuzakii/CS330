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
static long _syscall2(int syscall_num, char *buf, int length)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");

  return 0;   /*gcc shutup!*/
}
static long _syscall3(int syscall_num, u32 size, int flags)
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

/*My write function implementation*/ 
static long write(char *buf, int length)
{
  return(_syscall2(SYSCALL_WRITE, buf, length));
}

static void *expand(u32 size, int flags)
{
  return(_syscall3(SYSCALL_EXPAND, size, flags));
}

static void *shrink(u32 size, int flags)
{
  return(_syscall3(SYSCALL_SHRINK, size, flags)); 
}
static int main()
{
  //   char* ptra = ( char* ) expand(8, MAP_WR);
  // *(ptra + 4096) = 'X';
  // write(ptra+4096,1);
  // shrink(8, MAP_WR);
  // *(ptra + 4096) = 'A';
  // write(ptra+4096,1);


  void *ptr1;
  char *ptr = (char *) expand(20, MAP_WR);
  
  if(ptr == NULL)
              write("Expand FAILED\n", 7);
  
  *(ptr + 8192) = 'A';   /*Page fault will occur and handled successfully*/
  
  write(ptr+8192,1); /*check if value is written properly */
            // printf("are we there yet\n");
  write("are we there yet\n", 18);
  ptr1 = (char *) shrink(100, MAP_WR);

  if(ptr1 != NULL)
      write("Shrink not working\n",18);
  write("are we there not\n", 18);


  *ptr = 'A';          /*Page fault will occur and handled successfully*/

  ptr1 = (char*)shrink(20,MAP_WR);


  if(ptr1==NULL)
      write("shrink not working\n",18);

    // Page fault will occur and PF handler should termminate 
                   // the process (gemOS shell should be back) by printing an error message
  char *ptr2 = (char *) expand(12, MAP_RD);

  if(ptr2==NULL)
    write("expand not working\n",18);

  ptr = (char*)expand(5,MAP_WR);

  long k =write(ptr+1,1);   //write should get failed

  if(k!=-1)
    write("write not working\n",17);

  k=write("Hello\n",7);

  if(k!=-1)
    write("write working\n",14);

  u64* t = (u64*)(0x7FF000000+16);
  *t = 64;

  write("stack page fault handled\n",25);

  int* ptr3 = (int *) expand(20, MAP_RD);

  int x=*(ptr3+1);

  if(x==0){
    write("read only page fault handled\n",29);
  }
// x=2/0;
  *(ptr3 + 1024)  = 100;   /*page fault will occur and not handled successfully"*/

  exit(0);
  return 0;
}
