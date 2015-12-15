// startup.h

#ifndef STARTUP_H
#define STARTUP_H

#include <setjmp.h>
#include "tube-env.h"

/* Found in the *start.S file, implemented in assembler */

extern void _start( void );

extern void _enable_interrupts( void );

extern void _disable_interrupts( void );

extern int  _user_exec(volatile unsigned char *address, unsigned int r0, unsigned int r1, unsigned int r2);

extern void _error_handler_wrapper(ErrorBuffer_type *eb, EnvironmentHandler_type errorHandler);

extern void _escape_handler_wrapper(unsigned int escapeFlag, EnvironmentHandler_type escapeHandler);

extern void _exit_handler_wrapper(unsigned int r12, EnvironmentHandler_type exitHandler);

extern unsigned int _get_cpsr();

extern unsigned int _get_stack_pointer();

extern void _enable_unaligned_access();

#endif
