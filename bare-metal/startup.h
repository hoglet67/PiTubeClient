// startup.h

#include <setjmp.h>

#ifndef STARTUP_H
#define STARTUP_H

/* Found in the *start.S file, implemented in assembler */

extern void _start( void );

extern void _enable_interrupts( void );

extern void _disable_interrupts( void );

extern void _user_exec(volatile unsigned char *address);

extern void _isr_longjmp(jmp_buf env, int val);

#endif
