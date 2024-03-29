#ifndef __INIT_H_
#define __INIT_H_
#include <types.h>

#define SYSCALL_EXIT   1
#define SYSCALL_GETPID 2
#define SYSCALL_WRITE  3
#define SYSCALL_EXPAND    4
#define SYSCALL_SHRINK    5


extern void init_start(void);
// extern int do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4);
extern long do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4);
extern int handle_div_by_zero(void);
extern int handle_page_fault(void);

#endif
