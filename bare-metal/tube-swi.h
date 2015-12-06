// tube-swi.h

#ifndef TUBE_SWI_H
#define TUBE_SWI_H

// Macro allowing SWI calls to be made from C
// Note: stacking lr prevents corruption of lr when invoker in supervisor mode
#define SWI(NUM) \
  asm volatile("stmfd sp!,{lr}"); \
  asm volatile("swi "#NUM);       \
  asm volatile("ldmfd sp!,{lr}")  \

// Function prototypes
void OS_WriteC(unsigned int r0);

#endif
