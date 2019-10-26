#ifndef _DEBIX_LIB_H_
#define _DEBIX_LIB_H_

#include <types.h>

/*
Bit position starts from 0. For 32-bix values, 
position can be 0...31
*/

#define set_bit(x, pos)   (x) |= (1 << (pos))
#define flip_bit(x, pos)  (x) ^= (1 << (pos))
#define clear_bit(x, pos) (x) = ((x) ^ (((((x) >> (pos)) & 0x1)) << (pos)))
#define is_set(x, pos)    (((x) >> (pos)) & 0x1)

typedef __builtin_va_list va_list;
#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)

extern int strlen(char *);
extern void bzero(char *,int);
extern char* strcat(char *,char *);
extern int strcmp(char *,char *);
extern int memcmp(char *,char *,u32);
extern int memcpy(char *,char *,u32);

extern int printf(char *,...);
extern void console_init();
extern void invoke_dsh();
//extern int launch_user_app();


#define init_list(l) (l)->head = NULL;\
                     (l)->tail = NULL;\
                     (l)->size = 0
#define is_empty(l)  (!(l)->size)

extern int enqueue_tail(struct list*, u64);
extern struct node *dequeue_front(struct list *); 

#endif
