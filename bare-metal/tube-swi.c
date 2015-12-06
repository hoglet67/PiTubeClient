#include "tube-swi.h"
#include "tube-lib.h"

void OS_WriteC(unsigned int r0) {
  SWI(0);
}

void tube_WriteC(unsigned int r0) {
  sendByte(R1, (unsigned char)r0);
}

void C_SWI_Handler(unsigned int number, unsigned int *reg) {
  switch(number) {
  case 0:
	tube_WriteC(*reg);
	break;
  }
}

