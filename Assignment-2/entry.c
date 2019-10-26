#include<init.h>
#include<lib.h>
#include<context.h>
#include<memory.h>
// #include<stdint.h>
extern u32 os_pfn_alloc(u32);
extern void os_pfn_free(u32, u64);
#define l4_SHIFT 39
#define l3_SHIFT 30
#define l2_SHIFT 21
#define l1_SHIFT 12
#define bit_mask 0x1FF
#define pres_bit_mask 0x5
#define pbit_mask 0x1
#define reserved 0x0000FFFFFFFFFF007
#define pde_index_mask_l4 0xFF8000000000
#define pde_index_mask_l3 0x007FC0000000
#define pde_index_mask_l2 0x00003FE00000
#define pde_index_mask_l1 0x0000001FF000
void gmp_pager(struct exec_context *ctx, u64 error_code, u64 addr, u64 ripReg, int lvl) {
  u32 pfn_l4 = ctx -> pgd;
  u64 *pd_startAddr = (u64 *)osmap(pfn_l4);
  u64 *temp;
  u32 pfn;
  u64 wrbit = error_code & 2;
  temp = pd_startAddr + ((addr & pde_index_mask_l4) >> l4_SHIFT);
  if((*temp & 1) == 1) {
    pfn = (*temp >> l1_SHIFT) & 0xFFFFFFFF;
    pd_startAddr = (u64 *)osmap(pfn);
  }
  else {
    pfn = os_pfn_alloc(OS_PT_REG);
    *temp = (pfn << l1_SHIFT) | 5 | wrbit;
    pd_startAddr = osmap(pfn);
  }
  temp = pd_startAddr + ((addr & pde_index_mask_l3) >> l3_SHIFT);
  if((*temp & 1) == 1) {
    pfn = (*temp >> l1_SHIFT) & 0xFFFFFFFF;
    pd_startAddr = (u64 *)osmap(pfn);
  }
  else {
    pfn = os_pfn_alloc(OS_PT_REG);
    *temp = (pfn << l1_SHIFT) | 5 | wrbit;
    pd_startAddr = osmap(pfn);
  }
  temp = pd_startAddr + ((addr & pde_index_mask_l2) >> l2_SHIFT);
  if((*temp & 1) == 1) {
    pfn = (*temp >> l1_SHIFT) & 0xFFFFFFFF;
    pd_startAddr = (u64 *)osmap(pfn);
  }
  else {
    pfn = os_pfn_alloc(OS_PT_REG);
    *temp = (pfn << l1_SHIFT) | 5 | wrbit;
    pd_startAddr = osmap(pfn);
  }
  temp = pd_startAddr + ((addr & pde_index_mask_l1) >> l1_SHIFT);
  pfn = os_pfn_alloc(USER_REG);
  *temp = (pfn << l1_SHIFT) | 5 | wrbit;
}
/*I recieved partial help in form Raghukul Raman for the following function*/
int isValidAddr(struct exec_context *ctx, u64 addr, int lvl){
u32 pfn_l4 = ctx -> pgd;
  u64 *vaddr = (u64 *)osmap(pfn_l4);

  u64 *temp;
  u32 phy_addr;
  temp = vaddr + ((addr & pde_index_mask_l4) >> l4_SHIFT);
  phy_addr = (*temp >> l1_SHIFT) & 0xFFFFFFFF;
  vaddr = (u64 *)osmap(phy_addr);
  if(((*temp & 1) & 1) == 0 || (*temp & 4) == 0)
    return -1;

  temp = vaddr + ((addr & pde_index_mask_l3) >> l3_SHIFT);
  phy_addr = (*temp >> l1_SHIFT) & 0xFFFFFFFF;
  vaddr = (u64 *)osmap(phy_addr);
  if(((*temp & 1) & 1) == 0 || (*temp & 4) == 0)
    return -1;

  temp = vaddr + ((addr & pde_index_mask_l2) >> l2_SHIFT);
  phy_addr = (*temp >> l1_SHIFT) & 0xFFFFFFFF;
  vaddr = (u64 *)osmap(phy_addr);
  if(((*temp & 1) & 1) == 0 || (*temp & 4) == 0)
    return -1;

  temp = vaddr + ((addr & pde_index_mask_l1) >> l1_SHIFT);
  if(((*temp & 1) & 1) == 0 || (*temp & 4) == 0)
    return -1;

  return 1;
}
void gmp_cleaner(int c, u64 sa_stack, int lvl, u64 *pd_startAddr, u64 wrbit, int x, u32 apfn){
  // printf("sa_stack%x\n", sa_stack);
  if(lvl == 4){
    u64 l4_index = (sa_stack >> l4_SHIFT) & bit_mask;
    if((pd_startAddr[l4_index] & pbit_mask) == 1){
      u64 *l3 = (u64 *)osmap(((pd_startAddr[l4_index] >> 12) & 0x000000FFFFFFFFFF));
      
      if(lvl != c){
        gmp_cleaner(c,sa_stack, 3, l3, wrbit,x, apfn);return;
      }
      else{
        if(x==1){
          os_pfn_free(OS_PT_REG, (pd_startAddr[l4_index] >> 12) & 0x000000FFFFFFFFFF);
        }
      }
    }
  }
  else if(lvl == 3){
    u64 l3_index = (sa_stack >> l3_SHIFT) & bit_mask;
    if((pd_startAddr[l3_index] & pbit_mask) == 1){
      u64 *l2 = (u64 *)osmap(((pd_startAddr[l3_index] >> 12) & 0x000000FFFFFFFFFF)); 
      if(lvl != c){
        gmp_cleaner(c,sa_stack, 2, l2, wrbit,x, apfn);return;
      }
      else{
        os_pfn_free(OS_PT_REG, (pd_startAddr[l3_index] >> 12) & 0x000000FFFFFFFFFF);
      }
    }
    
  }
  else if(lvl == 2){
    u64 l2_index = (sa_stack >> l2_SHIFT) & bit_mask;
    if((pd_startAddr[l2_index] & pbit_mask) == 1){
      u64 *l1 = (u64 *)osmap(((pd_startAddr[l2_index] >> 12) & 0x000000FFFFFFFFFF)); 
      if(lvl != c){
        gmp_cleaner(c,sa_stack, 1, l1, wrbit, x,apfn);return;
      }
      else{
        os_pfn_free(OS_PT_REG, (pd_startAddr[l2_index] >> 12) & 0x000000FFFFFFFFFF);
        return;
      }
    }
    
  }
  else{
    u64 l1_index = (sa_stack >> l1_SHIFT) & bit_mask;
    if((pd_startAddr[l1_index] & pbit_mask) == 1){
      pd_startAddr[l1_index] = pd_startAddr[l1_index] & 0xFFFFFFFFFFFFFFFE;
      u32 l1_pfn = (pd_startAddr[l1_index] >> 12);
      os_pfn_free(USER_REG, l1_pfn);
      return;
    }
  }

  return;
}
/*System Call handler*/
long do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4)
{
    struct exec_context *current = get_current_ctx();
    printf("[GemOS] System call invoked. syscall no  = %d\n", syscall);
    switch(syscall)
    {
          case SYSCALL_EXIT:
                              printf("[GemOS] exit code = %d\n", (int) param1);
                              do_exit();
                              break;
          case SYSCALL_GETPID:
                              printf("[GemOS] getpid called for process %s, with pid = %d\n", current->name, current->id);
                              return current->id;      
          case SYSCALL_WRITE:
                             {  
                                     /*Your code goes  here*/
                              if(param2 > 1024 || param2 < 0)
                                return -1;
                              if(isValidAddr(current,param1, 4) == 1 && isValidAddr(current, param1 + param2 - 1, 4) == 1 )
                               { for(int i=0;i<(int) param2; i++)
                                                                 printf("%c", *(char *)(param1 + i));
                                                               return (int) param2;}
                              else
                                return -1;
                             }
          case SYSCALL_EXPAND:
                             {  
                                     /*Your code goes here*/
                              u32 size = (int)param1;
                              u64 nf = current->mms[MM_SEG_DATA].next_free;
                              if((int)param2 == 1){
                                if(current->mms[MM_SEG_DATA].next_free + size*4096> current->mms[MM_SEG_DATA].end)
                                  return 0;
                                current->mms[MM_SEG_DATA].next_free = current->mms[MM_SEG_DATA].next_free + size*4096;
                                return nf; 
                              }
                              else{
                                nf = current->mms[MM_SEG_RODATA].next_free;
                                if(current->mms[MM_SEG_RODATA].next_free+ size*4096 > current->mms[MM_SEG_RODATA].end)
                                  return 0;
                                current->mms[MM_SEG_RODATA].next_free = current->mms[MM_SEG_RODATA].next_free + size*4096;
                                return nf;
                              }
                             }
          case SYSCALL_SHRINK:
                             {  
                                     /*Your code goes here*/
                              u32 size = (int)param1;
                              if((int)param2 == 1){
                                if(current->mms[MM_SEG_DATA].next_free - size*4096 < current->mms[MM_SEG_DATA].start)
                                  {return 0;}
                                for(int i=0;i< size;i++){
                                  u64 *l4 = (u64 *)osmap(current->pgd);
                                current->mms[MM_SEG_DATA].next_free = current->mms[MM_SEG_DATA].next_free - 4096;
                                    gmp_cleaner(1, current->mms[MM_SEG_DATA].next_free, 4, l4, current->mms[MM_SEG_DATA].access_flags, 1, current->os_stack_pfn);
                                    // printf("next_free: %x\n", current->mms[MM_SEG_DATA].next_free);
                                      asm volatile ("invlpg (%0);" :: "r"(current->mms[MM_SEG_DATA].next_free) : "memory");

                                }
                                return current->mms[MM_SEG_DATA].next_free;
                              }
                              else{
                                if(current->mms[MM_SEG_RODATA].next_free- size*4096 < current->mms[MM_SEG_RODATA].start)
                                  {return 0;}
                                for(int i=0;i< size;i++){
                                   u64 *l4 = (u64 *)osmap(current->pgd);
                                   current->mms[MM_SEG_RODATA].next_free =current->mms[MM_SEG_RODATA].next_free -  4096;
                                   gmp_cleaner(1, current->mms[MM_SEG_RODATA].next_free, 4, l4, current->mms[MM_SEG_RODATA].access_flags, 1, current->os_stack_pfn);
                                   // printf("next_free: %x\n", current->mms[MM_SEG_RODATA].next_free);

                                    asm volatile ("invlpg (%0);" :: "r"(current->mms[MM_SEG_RODATA].next_free) : "memory");
                                }
                                printf("RD\n");
                                return current->mms[MM_SEG_RODATA].next_free;

                              }
                             }
                             
          default:
                              return -1;
                                
    }
    return 0;   /*GCC shut up!*/
}


extern int handle_div_by_zero(void)
{
    /*Your code goes in here*/
    u64 rbp;
    asm volatile("mov %%rbp, %0;" : "=r" (rbp));
    
    u64 *rip = (u64 *)(rbp +8);
    printf("Div-by-zero detected at %x\n", *rip);
    do_exit();
    return 0;
}

extern int handle_page_fault(void)
{
    /*Your code goes in here*/
    u64 rspReg;
    asm volatile(
      "push %%r8\n"
      "push %%r9\n"
      "push %%r10\n"
      "push %%r11\n"
      "push %%r12\n"
      "push %%r13\n"
      "push %%r14\n"
      "push %%r15\n"
      "push %%rax\n"
      "push %%rbx\n"
      "push %%rcx\n"
      "push %%rdx\n"
      "push %%rsi\n"
      "push %%rdi\n"
      "mov %%rsp, %0;"
      :"=r"(rspReg)
      :
      :"memory"
      );
    u64 cr2;
    asm volatile(
        "mov %%cr2, %0;"
    : "=r" (cr2)
    : /* no input */
    : "memory"
    );
    u64 rbp;
    asm volatile("mov %%rbp, %0;" : "=r"(rbp));
    u64 *error_code = rbp + 0x8;
    u64 *ripReg = rbp + 0x10;
    struct exec_context *ctx = get_current_ctx();

    if((cr2 >= ctx->mms[MM_SEG_DATA].start) && (cr2 <= ctx->mms[MM_SEG_DATA].end)){
      /*Create page table mapping*/
    if((cr2 >= ctx->mms[MM_SEG_DATA].start) && (cr2 < ctx->mms[MM_SEG_DATA].next_free) && ((*error_code & 0x1) == 0)){  
        gmp_pager(ctx, *error_code, cr2, *ripReg, 4);
      }  
      else{
        printf("Error in MM_SEG_DATA VA:%x, RIP:%x, error_code:%x\n",cr2, *ripReg, *error_code );
        do_exit();
      }
    }
    else if((cr2 >= ctx->mms[MM_SEG_RODATA].start) && (cr2 <= ctx->mms[MM_SEG_RODATA].end)){

      if(((*error_code & 0x2) == 0) && ((*error_code & 0x1) == 0) && (cr2 >= ctx->mms[MM_SEG_RODATA].start) && (cr2 <= ctx->mms[MM_SEG_RODATA].next_free)){
        /*Create page table mapping*/
        gmp_pager(ctx, *error_code, cr2, *ripReg, 4);
      }
      else{
        printf("Error in MM_SEG_RODATA VA:%x, RIP:%x, error_code:%x\n",cr2, *ripReg, *error_code );
        do_exit();
      }
    }
    else if((cr2 >= (ctx->mms)[MM_SEG_STACK].start) && (cr2 <= (ctx->mms)[MM_SEG_STACK].end)){
      if((cr2 >= ctx->mms[MM_SEG_STACK].start) && (cr2 < ctx->mms[MM_SEG_STACK].next_free)){
        gmp_pager(ctx, *error_code, cr2, *ripReg, 4);
      }
      else{
        printf("Error in MM_SEG_STACK VA:%x, RIP:%x, error_code:%x\n",cr2, *ripReg, *error_code );
        do_exit();
      }
    }
    else{
      printf("Segmentation Fault! VA: %x, RIP: %x, error_code: %x\n", cr2, *ripReg, *error_code);
      do_exit();
    }
    asm volatile(
      "mov %0, %%rsp;"
      "pop %%rsi\n"
      "pop %%rdi\n"
      "pop %%rdx\n"
      "pop %%rcx\n"
      "pop %%rbx\n"
      "pop %%rax\n"
      "pop %%r15\n"
      "pop %%r14\n"
      "pop %%r13\n"
      "pop %%r12\n"
      "pop %%r11\n"
      "pop %%r10\n"
      "pop %%r9\n"
      "pop %%r8\n"
      "mov %%rbp ,%%rsp\n"
      "pop %%rbp\n"
      "add $8, %%rsp\n"
      "iretq\n" :: "r"(rspReg)
      );
    return 0;
}