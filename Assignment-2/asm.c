#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<sys/mman.h>
#include<sched.h>
#include<fcntl.h>
#include<unistd.h>
int main()
{
    unsigned long cr3_val, cs_val, rsp_val;
    // asm volatile ("hlt;");
    asm volatile("mov %%rsp, %0;"
                  : "=r" (rsp_val)
                  :
                 );
   
    printf("RSP = %lx\n", rsp_val);
  
    // asm volatile("mov %%cr3, %0;"
    //               : "=r" (cr3_val)
    //               :
    //              );
   
    // printf("CR3 = %lx\n", cr3_val);
    asm volatile("mov %%cs, %0;"
                  : "=r" (cs_val)
                  :
                 );
    printf("CS = %lx\n", cs_val);
    // asm volatile("mov %0, %%cs;"
    //               :
    //               : "r" (cs_val)
    //              );
    return 0;
}
