// tube-swi.h

#ifndef TUBE_SWI_H
#define TUBE_SWI_H


// Macro allowing SWI calls to be made from C
// Note: stacking lr prevents corruption of lr when invoker in supervisor mode
#define SWI(NUM) \
  asm volatile("stmfd sp!,{lr}"); \
  asm volatile("swi "#NUM);       \
  asm volatile("ldmfd sp!,{lr}")  \

// Type definition for a SWI handler
typedef void (*SWIHandler_Type) (unsigned int *reg);

// Function prototypes
void tube_WriteC(unsigned int *reg);       // &00
void tube_WriteS(unsigned int *reg);       // &01
void tube_Write0(unsigned int *reg);       // &02
void tube_NewLine(unsigned int *reg);      // &03
void tube_ReadC(unsigned int *reg);        // &04
void tube_CLI(unsigned int *reg);          // &05
void tube_Byte(unsigned int *reg);         // &06
void tube_Word(unsigned int *reg);         // &07
void tube_File(unsigned int *reg);         // &08
void tube_Args(unsigned int *reg);         // &09
void tube_BGet(unsigned int *reg);         // &0A
void tube_BPut(unsigned int *reg);         // &0B
void tube_GBPB(unsigned int *reg);         // &0C
void tube_Find(unsigned int *reg);         // &0D
void tube_ReadLine(unsigned int *reg);     // &0E
void tube_Control(unsigned int *reg);      // &0F
void tube_GetEnv(unsigned int *reg);       // &10
void tube_Exit(unsigned int *reg);         // &11
void tube_SetEnv(unsigned int *reg);       // &12
void tube_IntOn(unsigned int *reg);        // &13
void tube_IntOff(unsigned int *reg);       // &14
void tube_CallBack(unsigned int *reg);     // &15
void tube_EnterOS(unsigned int *reg);      // &16
void tube_BreakPt(unsigned int *reg);      // &17
void tube_BreakCtrl(unsigned int *reg);    // &18
void tube_UnusedSWI(unsigned int *reg);    // &19
void tube_UpdateMEMC(unsigned int *reg);   // &1A
void tube_SetCallBack(unsigned int *reg);  // &1B
void tube_Mouse(unsigned int *reg);        // &1C

#endif
