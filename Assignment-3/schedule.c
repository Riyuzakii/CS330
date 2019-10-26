#include<context.h>
#include<memory.h>
#include<schedule.h>
#include<apic.h>
#include<lib.h>
#include<idt.h>
static u64 numticks;

static void save_current_context()
{
  /*Your code goes in here*/ 
} 

static void schedule_context(struct exec_context *next)
{
  /*Your code goes in here. get_current_ctx() still returns the old context*/
 struct exec_context *current = get_current_ctx();
 printf("schedluing: old pid = %d  new pid  = %d\n", current->pid, next->pid); /*XXX: Don't remove*/
/*These two lines must be executed*/
 set_tss_stack_ptr(next);
 set_current_ctx(next);
 return;
}
static void schedule(struct exec_context *new_ctx, int flag) {
  struct exec_context *current = get_current_ctx();
  printf("schedluing: old pid = %d  new pid  = %d\n", current->pid, new_ctx->pid); /*XXX: Don't remove*/
  
  set_tss_stack_ptr(new_ctx);
  set_current_ctx(new_ctx);
  new_ctx -> state = RUNNING;
  u64 *rbp, *rsp;
  asm volatile("mov %%rbp, %0;":"=r"(rbp));
  asm volatile("mov %%rsp, %0;":"=r"(rsp));

  if(flag == 1){
   ack_irq();  
  }
  *(rsp-1) = new_ctx->regs.entry_ss;
  *(rsp-2) = new_ctx->regs.entry_rsp;
  *(rsp-3) = new_ctx->regs.entry_rflags;
  *(rsp-4) = new_ctx->regs.entry_cs;
  *(rsp-5) = new_ctx->regs.entry_rip;
  *(rsp-6) = new_ctx->regs.rax;
  *(rsp-7) = new_ctx->regs.rbx;
  *(rsp-8) = new_ctx->regs.rcx;
  *(rsp-9) = new_ctx->regs.rdx;
  *(rsp-10) = new_ctx->regs.rsi;
  *(rsp-11) = new_ctx->regs.rdi;
  *(rsp-12) = new_ctx->regs.rbp;
  *(rsp-13) = new_ctx->regs.r15;
  *(rsp-14) = new_ctx->regs.r14;
  *(rsp-15) = new_ctx->regs.r13;
  *(rsp-16) = new_ctx->regs.r12;
  *(rsp-17) = new_ctx->regs.r11;
  *(rsp-18) = new_ctx->regs.r10;
  *(rsp-19) = new_ctx->regs.r9;
  *(rsp-20) = new_ctx->regs.r8;

  // if(flag != 1)
  //   printf("NOT_ACK\n");
  // if(flag == 1){
  //   printf("ACK_IRQ\n");
  //  ack_irq();  
  // }
  // do_exit();
  asm volatile (
    "mov %0, %%rsp;"
    "pop %%r8;"
    "pop %%r9;"
    "pop %%r10;"
    "pop %%r11;"
    "pop %%r12;"
    "pop %%r13;"
    "pop %%r14;"
    "pop %%r15;"
    "pop %%rbp;"
    "pop %%rdi;"
    "pop %%rsi;"
    "pop %%rdx;"
    "pop %%rcx;"
    "pop %%rbx;"
    "pop %%rax;"
    ::"r"(rsp - 20)
  );
  asm volatile("iretq;"
               :::"memory");
}

static struct exec_context *pick_next_context(struct exec_context *list)
{
  /*Your code goes in here*/
  struct exec_context *ctx = get_current_ctx();
  int proc[MAX_PROCESSES], cnt = 0;
  for(int i = (ctx -> pid) + 1;i < MAX_PROCESSES;i++)
    proc[cnt++] = i;
  for(int i = 1;i <= (ctx -> pid);i++)
    proc[cnt++] = i;
  proc[cnt++] = 0;

  for(int i = 0;i < MAX_PROCESSES;i++) {
    struct exec_context *new_ctx = get_ctx_by_pid(proc[i]);
    if(new_ctx -> state == READY)
      return new_ctx;
  }
  return NULL;
}


static void do_sleep_and_alarm_account()
{
  // printf("[DEBUG] In do_sleep_and_alarm_account\n");
 /*All processes in sleep() must decrement their sleep count*/ 
}

/*The five functions above are just a template. You may change the signatures as you wish*/
void handle_timer_tick()
{

  u64 *rspReg;
  u64 *rbpReg;
  asm volatile (
    "push %rax;"
    "push %rbx;"
    "push %rcx;"
    "push %rdx;"
    "push %rsi;"
    "push %rdi;"
    "push %r8;"
    "push %r9;"
    "push %r10;"
    "push %r11;"
    "push %r12;"
    "push %r13;"
    "push %r14;"
    "push %r15;"
    );
  asm volatile("mov %%rsp, %0;":"=r"(rspReg));
  asm volatile("mov %%rbp, %0;":"=r"(rbpReg));

 /*
   This is the timer interrupt handler. 
   You should account timer ticks for alarm and sleep
   and invoke schedule
 */
  printf("Got a tick. #ticks = %u\n", ++numticks);   /*XXX Do not modify this line*/ 

  struct exec_context *ctx = get_current_ctx();
  // printf("[DEBUG] rip = %x\n",(u64*)rip );
  // printf("[DEBUG] rbpRegReg = %x\n",rbpRegReg );
  // printf("[DEBUG] rspReg = %x\n",(u64*)rspReg );
  // printf("[DEBUG] rspReg = %x\n",(u64*)rspReg );
  //ALARM
  if(ctx->ticks_to_alarm > 0){
    // printf("[DEBUG] ticks_to_alarm = %d\n",ctx->ticks_to_alarm );
    ctx->ticks_to_alarm--;
    if(ctx->ticks_to_alarm == 0){
      ctx->ticks_to_alarm = ctx->alarm_config_time;
      invoke_sync_signal(SIGALRM, rbpReg+4, rbpReg+1);
      // return;
      
    } 
  }

  //SLEEP!!
  for(int i=0;i < MAX_PROCESSES;i++) {
    struct exec_context *new_ctx = get_ctx_by_pid(i);
    if((new_ctx->state) == WAITING && (new_ctx->ticks_to_sleep) > 0) {
      new_ctx->ticks_to_sleep--;
      if(new_ctx->ticks_to_sleep == 0) {
        new_ctx->state = READY;
      }
    }
  }
  ctx -> state = READY;
  struct exec_context *list = get_ctx_list();
  struct exec_context *new_ctx = pick_next_context(list);
  
  if(ctx->pid != new_ctx->pid){
    ctx->state = READY;
    ctx->regs.r15 = *(rspReg);
    ctx->regs.r14 = *(rspReg + 1);
    ctx->regs.r13 = *(rspReg + 2);
    ctx->regs.r12 = *(rspReg + 3);
    ctx->regs.r11 = *(rspReg + 4);
    ctx->regs.r10 = *(rspReg + 5);
    ctx->regs.r9 = *(rspReg + 6);
    ctx->regs.r8 = *(rspReg + 7);
    ctx->regs.rdi = *(rspReg + 8);
    ctx->regs.rsi = *(rspReg + 9);
    ctx->regs.rdx = *(rspReg + 10);
    ctx->regs.rcx = *(rspReg + 11);
    ctx->regs.rbx = *(rspReg + 12);
    ctx->regs.rax = *(rspReg + 13);
    ctx->regs.rbp = *(rbpReg);
    ctx->regs.entry_rip = *(rbpReg + 1);
    ctx->regs.entry_cs  = *(rbpReg + 2);
    ctx->regs.entry_rflags = *(rbpReg + 3);
    ctx->regs.entry_rsp = *(rbpReg + 4);
    ctx->regs.entry_ss  = *(rbpReg + 5);

    asm volatile (
      "mov %0, %%rsp;"
      "pop %%r15;"
      "pop %%r14;"
      "pop %%r13;"
      "pop %%r12;"
      "pop %%r11;"
      "pop %%r10;"
      "pop %%r9;"
      "pop %%r8;"
      "pop %%rdi;"
      "pop %%rsi;"
      "pop %%rdx;"
      "pop %%rcx;"
      "pop %%rbx;"
      "pop %%rax;"
      ::"r"(rspReg)
    );
    schedule(new_ctx, 1);

  }
  else {  
    ack_irq();  /*acknowledge the interrupt, next interrupt */
    asm volatile (
    "mov %0, %%rsp;"
    "pop %%r15;"
    "pop %%r14;"
    "pop %%r13;"
    "pop %%r12;"
    "pop %%r11;"
    "pop %%r10;"
    "pop %%r9;"
    "pop %%r8;"
    "pop %%rdi;"
    "pop %%rsi;"
    "pop %%rdx;"
    "pop %%rcx;"
    "pop %%rbx;"
    "pop %%rax;"
    ::"r"(rspReg)
  );
  asm volatile("mov %%rbp, %%rsp;"
               "pop %%rbp;"
               "iretq;"
               :::"memory");
  }
}

void do_exit()
{
  /*You may need to invoke the scheduler from here if there are
    other processes except swapper in the system. Make sure you make 
    the status of the current process to UNUSED before scheduling 
    the next process. If the only process alive in system is swapper, 
    invoke do_cleanup() to shutdown gem5 (by crashing it, huh!)
    */
  struct exec_context *ctx = get_current_ctx();
  
  os_pfn_free(OS_PT_REG, ctx->os_stack_pfn);
  ctx->state = UNUSED;

  int proc_exist = -1;
  for(int i = 1;i < MAX_PROCESSES;i++) {
    struct exec_context *new_ctx = get_ctx_by_pid(i);
    if(new_ctx -> state != UNUSED) {
      proc_exist = 1;
    }
  }
  if(proc_exist == -1) {
    do_cleanup();  /*Call this conditionally, see comments above*/
  }

  struct exec_context *list = get_ctx_list();
  struct exec_context *new_ctx = pick_next_context(list);
  
  // printf("schedluing: old pid = %d  new pid  = %d\n", ctx->pid, new_ctx->pid); /*XXX: Don't remove*/
  schedule(new_ctx, 0);
}

/*system call handler for sleep*/
long do_sleep(u32 ticks)
{
  u64 *rbpReg, *rspReg;
  asm volatile("mov %%rbp, %0;":"=r"(rbpReg));
  asm volatile("mov %%rsp, %0;":"=r"(rspReg));
  u64 *rbp = (u64 *)*(rbpReg);
  struct exec_context *ctx = get_current_ctx();
  ctx -> ticks_to_sleep = ticks;
  ctx -> state = WAITING;
  ctx->regs.r15 = *(rbp +2);
  ctx->regs.r14 = *(rbp +3);
  ctx->regs.r13 = *(rbp +4);
  ctx->regs.r12 = *(rbp +5);
  ctx->regs.r11 = *(rbp +6);
  ctx->regs.r10 = *(rbp +7);
  ctx->regs.r9 = *(rbp +8);
  ctx->regs.r8 = *(rbp +9);
  ctx->regs.rbp = *(rbp +10);
  ctx->regs.rdi = *(rbp +11);
  ctx->regs.rsi = *(rbp +12);
  ctx->regs.rdx = *(rbp +13);
  ctx->regs.rcx = *(rbp +14);
  ctx->regs.rbx = *(rbp +15);
  ctx->regs.rax = 0;
  ctx->regs.entry_rip = *(rbp +16);
  ctx->regs.entry_cs  = *(rbp +17);
  ctx->regs.entry_rflags = *(rbp +18);
  ctx->regs.entry_rsp = *(rbp +19);
  ctx->regs.entry_ss  = *(rbp +20);

  struct exec_context *list = get_ctx_list();
  struct exec_context *new_ctx = pick_next_context(list);

  schedule(new_ctx, 0);
      
}
/*
  system call handler for clone, create thread like 
  execution contexts
*/
long do_clone(void *th_func, void *user_stack)
{
  u64 *rbpReg;
  asm volatile("mov %%rbp, %0;":"=r"(rbpReg));
  struct exec_context *ctx = get_current_ctx();
  struct exec_context *new_ctx = get_new_ctx();

  // new_ctx->type = ctx->type;
  new_ctx->state = READY;

  new_ctx->os_stack_pfn = os_pfn_alloc(OS_PT_REG);
  // new_ctx->os_rsp = osmap(new_ctx->os_stack_pfn);
  
  for(int i =0;i<MAX_MM_SEGS;i++){
    new_ctx->mms[i] = ctx->mms[i];
  }
  
  char strpid[4];
  strpid[0] = '_';
  strpid[1] = '0' + (new_ctx->pid / 10);
  strpid[2] = '0' + (new_ctx->pid % 10);
  strpid[3] = 0;
  bzero(new_ctx->name, CNAME_MAX);                                                          
  memcpy(new_ctx->name, ctx->name, strlen(ctx->name));
  memcpy(new_ctx->name + (strlen(ctx->name)-1), strpid, strlen(strpid));
  
  new_ctx->regs.entry_cs = 0x23;
  new_ctx->regs.entry_ss = 0x2b;
  new_ctx->regs.entry_rsp = (u64 *)user_stack;
  new_ctx->regs.entry_rip = (u64 *)th_func;
  u64 *rbp = (u64 *)*(rbpReg);
  new_ctx->regs.entry_rflags = *(rbp +18);
  
  // new_ctx->pending_signal_bitmap = 0;
  
  // for(int i=0;i<MAX_SIGNALS;i++){
  //   new_ctx->sighandlers[i] = ctx->sighandlers[i];  
  // }

}

long invoke_sync_signal(int signo, u64 *ustackp, u64 *urip)
{

   /*If signal handler is registered, manipulate user stack and RIP to execute signal handler*/
   /*ustackp and urip are pointers to user RSP and user RIP in the exception/interrupt stack*/
   struct exec_context *current = get_current_ctx();
   printf("Called signal with ustackp=%x urip=%x\n", *ustackp, *urip);
   //printf("Signal Number is %d %x\n",signo,current->sighandlers[signo]);
   if(current->sighandlers[signo])
   {
      u64 stack = (*ustackp);
      stack = stack-8;
      *((u64 *)(stack)) = (*urip);
      (*urip) = (u64) current->sighandlers[signo];
      (*ustackp) = stack;
   }
   /*Default behavior is exit( ) if sighandler is not registered for SIGFPE or SIGSEGV.
    Ignore for SIGALRM*/
    else
    {
      if(signo != SIGALRM)
        do_exit();
    }
}
/*system call handler for signal, to register a handler*/
long do_signal(int signo, unsigned long handler)
{
  struct exec_context *ctx = get_current_ctx();
  ctx->sighandlers[signo] = (unsigned long *)handler;
  return (unsigned long *)handler;
}

/*system call handler for alarm*/
long do_alarm(u32 ticks)
{
  // printf("%d\n",ticks );
  struct exec_context *ctx = get_current_ctx();
  ctx->ticks_to_alarm = ticks;
  ctx->alarm_config_time = ticks;
    // printf("[DEBUG] ticks_to_alarm(do_alarm) = %d\n",ctx->ticks_to_alarm );
  return 0;
}
