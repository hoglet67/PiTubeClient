#include <stdio.h>
#include "debug.h"
#include "startup.h"
#include "tube-lib.h"
#include "tube-env.h"
#include "tube-swi.h"
#include "tube-isr.h"

#define NUM_SWI_HANDLERS 0x1D

const int osword_in_len[] = {
  0,  // OSWORD 0x00
  0,
  5,
  0,
  5,
  2,
  5,
  8,
  14,
  4,
  1,
  1,
  5,
  0,
  8,
  25,
  16,
  13,
  0,
  8,
  128  // OSWORD 0x14
};

const int osword_out_len[] = {
  0,  // OSWORD 0x00
  5,
  0,
  5,
  0,
  5,
  0,
  0,
  0,
  5,
  9,
  5,
  0,
  8,
  25,
  1,
  13,
  13,
  128,
  8,
  128  // OSWORD 0x14
};

SWIHandler_Type SWIHandler_Table[NUM_SWI_HANDLERS] = {
  tube_WriteC,       // &00
  tube_WriteS,       // &01
  tube_Write0,       // &02
  tube_NewLine,      // &03
  tube_ReadC,        // &04
  tube_CLI,          // &05
  tube_Byte,         // &06
  tube_Word,         // &07
  tube_File,         // &08
  tube_Args,         // &09
  tube_BGet,         // &0A
  tube_BPut,         // &0B
  tube_GBPB,         // &0C
  tube_Find,         // &0D
  tube_ReadLine,     // &0E
  tube_Control,      // &0F
  tube_GetEnv,       // &10
  tube_Exit,         // &11
  tube_SetEnv,       // &12
  tube_IntOn,        // &13
  tube_IntOff,       // &14
  tube_CallBack,     // &15
  tube_EnterOS,      // &16
  tube_BreakPt,      // &17
  tube_BreakCtrl,    // &18
  tube_UnusedSWI,    // &19
  tube_UpdateMEMC,   // &1A
  tube_SetCallBack,  // &1B
  tube_Mouse         // &1C
};


void C_SWI_Handler(unsigned int number, unsigned int *reg) {
  if (DEBUG) {
	printf("SWI %08x\r\n", number);
  }
  if (number < NUM_SWI_HANDLERS) {
	// Invoke one of the fixed handlers
	SWIHandler_Table[number](reg);
  } else if ((number & 0xFF00) == 0x0100) {
	// SWI's 0x100 - 0x1FF are OS_WriteI
    tube_WriteC(&number);
  } else {
	// TODO: Illegal SWI Handler
  }
  if (DEBUG) {
	printf("SWI %08x complete\r\n", number);
  }
}

// Helper functions

void updateCarry(unsigned char cy, unsigned int *reg) {
  // The PSW is on the stack one word before the registers
  reg--;
  // bit 29 is the carry
  if (cy & 0x80) {
	*reg |= CARRY_MASK;
  } else {
	*reg &= ~CARRY_MASK;
  }
}

void user_exec(volatile unsigned char *address) {
  if (DEBUG) {
	printf("Execution passing to %08x\r\n", (unsigned int)address);
  }
  setTubeLibDebug(1);
  // The machine code version in armc-startup.S does the real work
  // of dropping down to user mode
  _user_exec(address);
}

// Client to Host transfers
// Reference: http://mdfs.net/Software/Tube/Protocol
// OSWRCH   R1: A
// OSRDCH   R2: &00                               Cy A
// OSCLI    R2: &02 string &0D                    &7F or &80
// OSBYTELO R2: &04 X A                           X
// OSBYTEHI R2: &06 X Y A                         Cy Y X
// OSWORD   R2: &08 A in_length block out_length  block
// OSWORD0  R2: &0A block                         &FF or &7F string &0D
// OSARGS   R2: &0C Y block A                     A block
// OSBGET   R2: &0E Y                             Cy A
// OSBPUT   R2: &10 Y A                           &7F
// OSFIND   R2: &12 &00 Y                         &7F
// OSFIND   R2: &12 A string &0D                  A
// OSFILE   R2: &14 block string &0D A            A block
// OSGBPB   R2: &16 block A                       block Cy A

void tube_WriteC(unsigned int *reg) {
  sendByte(R1, (unsigned char)(*reg & 0xff));
}

void tube_WriteS(unsigned int *reg) {
  // Reg 13 is the stacked link register which points to the string
  tube_Write0(&reg[13]);
  // Make sure new value of link register is word aligned to the next word boundary
  reg[13] += 3;
  reg[13] &= ~3;
}

void tube_Write0(unsigned int *reg) {
  unsigned char *ptr = (unsigned char *)(*reg);
  unsigned char c;
  // Output characters pointed to by R0, until a terminating zero
  while ((c = *ptr++) != 0) {
	sendByte(R1, c);
  }
  // On exit, R0 points to the byte after the terminator
  *reg = (unsigned int)ptr;
}

void tube_NewLine(unsigned int *reg) {
  sendByte(R1, 0x0A);
  sendByte(R1, 0x0D);
}

void tube_ReadC(unsigned int *reg) {
  // OSRDCH   R2: &00                               Cy A
  sendByte(R2, 0x00);
  // On exit, the Carry flag indicaes validity
  updateCarry(receiveByte(R2), reg);
  // On exit, R0 contains the character
  reg[0] = receiveByte(R2);
}

void tube_CLI(unsigned int *reg) {
  char *ptr = (char *)(*reg);
  // OSCLI    R2: &02 string &0D                    &7F or &80
  sendByte(R2, 0x02);
  sendString(R2, 0x00, ptr);
  sendByte(R2, 0x0D);
  if (receiveByte(R2) & 0x80) {
	// Execution should pass to last transfer address
	user_exec(address);
  }
}

void tube_Byte(unsigned int *reg) {
  unsigned char cy;
  unsigned char a = reg[0] & 0xff;
  unsigned char x = reg[1] & 0xff;
  unsigned char y = reg[2] & 0xff;
  if (a < 128) {
	// OSBYTELO R2: &04 X A                           X
	sendByte(R2, 0x04);
	sendByte(R2, x);
	sendByte(R2, a);
	x = receiveByte(R2);
	reg[1] = x; 

  } else {
	// OSBYTEHI R2: &06 X Y A                         Cy Y X
	sendByte(R2, 0x06);
	sendByte(R2, x);
	sendByte(R2, y);
	sendByte(R2, a);
	cy = receiveByte(R2);
	y = receiveByte(R2);
	x = receiveByte(R2);
	reg[1] = x;
	reg[2] = y;
	updateCarry(cy, reg);
  }
}

void tube_Word(unsigned int *reg) {
  int in_len;
  int out_len;
  unsigned char a = reg[0] & 0xff;
  unsigned char *block; 
  // Note that call with R0b=0 (Acorn MOS RDLN) does nothing, the ReadLine call should be used instead. 
  // Ref: http://chrisacorns.computinghistory.org.uk/docs/Acorn/OEM/AcornOEM_ARMUtilitiesRM.pdf
  if (a == 0) {
	return;
  }
  // Work out block lengths
  // Ref: http://mdfs.net/Info/Comp/Acorn/AppNotes/004.pdf
  block = (unsigned char *)reg[1];
  if (a < 0x15) {
	in_len = osword_in_len[a];
	out_len = osword_out_len[a];
  } else if (a < 128) {
	in_len = 16;
	out_len = 16;
  } else {
	// TODO: Check with JGH whether it is correct to update block to exclude the lengths
	in_len = *block++;
	out_len = *block++;
  }  
  // OSWORD   R2: &08 A in_length block out_length  block
  sendByte(R2, 0x08);
  sendByte(R2, a);
  sendByte(R2, in_len);
  sendBlock(R2, in_len, block);
  sendByte(R2, out_len);
  receiveBlock(R2, out_len, block);
}

void tube_File(unsigned int *reg) {
  // start at the last param (r5)
  unsigned int *ptr = reg + 5;
  // OSFILE   R2: &14 block string &0D A            A block
  sendByte(R2, 0x14);
  sendWord(R2, *ptr--);            // r5 = attr
  sendWord(R2, *ptr--);            // r4 = leng
  sendWord(R2, *ptr--);            // r3 = exec
  sendWord(R2, *ptr--);            // r2 = load
  sendString(R2, 0x0D, (char *)*ptr--);  // r1 = filename ptr
  sendByte(R2, 0x0D);              //      filename terminator
  sendByte(R2, *ptr);              // r0 = action
  *ptr = receiveByte(R2);          // r0 = action
  ptr = reg + 5;                   // ptr = r5
  *ptr-- = receiveWord(R2);        // r5 = attr
  *ptr-- = receiveWord(R2);        // r4 = lang
  *ptr-- = receiveWord(R2);        // r3 = exec
  *ptr-- = receiveWord(R2);        // r2 = load
}

void tube_Args(unsigned int *reg) {
  // OSARGS   R2: &0C Y block A                     A block
  sendByte(R2, 0x0C);
  // Y = R1 is the file namdle
  sendByte(R2, reg[1]);
  // R2 is the 4 byte data block
  sendWord(R2, reg[2]);
  // A = R0 is the operation code
  sendByte(R2, reg[0]);
  // get back A
  reg[0] = receiveByte(R2);
  // get back 4 byte data block
  reg[2] = receiveWord(R2);
}

void tube_BGet(unsigned int *reg) {
  // OSBGET   R2: &0E Y                             Cy A
  sendByte(R2, 0x0E);
  // Y = R1 is the file namdle
  sendByte(R2, reg[1]);
  // On exit, the Carry flag indicaes validity
  updateCarry(receiveByte(R2), reg);
  // On exit, R0 contains the character
  reg[0] = receiveByte(R2);
}

void tube_BPut(unsigned int *reg) {
  // OSBPUT   R2: &10 Y A                           &7F
  sendByte(R2, 0x10);
  // Y = R1 is the file namdle
  sendByte(R2, reg[1]);
  // A = R0 is the character
  sendByte(R2, reg[0]);
  // Response is always 7F so ingnored
  receiveByte(R2);
}

void tube_GBPB(unsigned int *reg) {
  // start at the last param (r4)
  unsigned int *ptr = reg + 4;
  // OSGBPB   R2: &16 block A                       block Cy A
  sendByte(R2, 0x16);
  sendWord(R2, *ptr--);              // r4
  sendWord(R2, *ptr--);              // r3
  sendWord(R2, *ptr--);              // r2
  sendByte(R2, *ptr--);              // r1
  sendByte(R2, *ptr);                // r0
  ptr = reg + 4;
  *ptr-- = receiveWord(R2);          // r4
  *ptr-- = receiveWord(R2);          // r3
  *ptr-- = receiveWord(R2);          // r2
  *ptr-- = receiveByte(R2);          // r1
  updateCarry(receiveByte(R2), reg); // Cy  
  *ptr-- = receiveWord(R2);          // r0
}

void tube_Find(unsigned int *reg) {
  // OSFIND   R2: &12 &00 Y                         &7F
  // OSFIND   R2: &12 A string &0D                  A
  sendByte(R2, 0x12);
  // A = R0 is the operation type
  sendByte(R2, reg[0]);
  if (reg[0] == 0) {
	// Y = R1 is the file handle to close
	sendByte(R2, reg[1]);
	// Response is always 7F so ignored
	receiveByte(R2);
  } else {
	// R1 points to the string
	sendString(R2, 0x0D, (char *)reg[1]); 
	sendByte(R2, 0x0D);
	// Response is the file handle of file just opened
	reg[0] = receiveByte(R2);
  }
}

void tube_ReadLine(unsigned int *reg) {
  unsigned char resp;
  // OSWORD0  R2: &0A block                         &FF or &7F string &0D
  sendByte(R2, 0x0A);
  sendByte(R2, reg[3]);      // max ascii value
  sendByte(R2, reg[2]);      // min ascii value
  sendByte(R2, reg[1]);      // max line length
  sendByte(R2, 0x07);        // Buffer MSB - set as per Tube Ap Note 004
  sendByte(R2, 0x00);        // Buffer LSB - set as per Tube Ap Note 004
  resp = receiveByte(R2);    // 0x7F or 0xFF
  updateCarry(resp, reg);
  // Was it valid? 
  if ((resp & 0x80) == 0x00) {
	reg[1] = receiveString(R2, '\r', (char *)reg[0]);
  }
}

void tube_Control(unsigned int *reg) {
  unsigned int previous;
  // R0 = address of error handler (0 for no change)
  previous = (unsigned int)env->errorHandler;
  if (reg[0]) {
	env->errorHandler = (ErrorHandler_type) reg[0]; 
  }
  reg[0] = previous;
  // R1 = address of error buffer (0 for no change) 
  previous = (unsigned int)env->errorBuffer;
  if (reg[1]) {
	env->errorBuffer = (ErrorBuffer_type *) reg[1]; 
  }
  reg[1] = previous;
  // R2 = address of escape handler (0 for no change) 
  previous = (unsigned int)env->escapeHandler;
  if (reg[2]) {
	env->escapeHandler = (EscapeHandler_type) reg[2]; 
  }
  reg[2] = previous;
  // R3 = address of event handler (0 for no change) 
  previous = (unsigned int)env->eventHandler;
  if (reg[3]) {
	env->eventHandler = (EventHandler_type) reg[3]; 
  }
  reg[3] = previous;
}

void tube_GetEnv(unsigned int *reg) {
  // R0 address of the command string (0 terminated) which ran the program
  reg[0] = (unsigned int) env->commandBuffer;
  // R1 address of the permitted RAM limit for example &10000 for 64K machine
  reg[1] = env->memoryLimit;
  // R2 address of 5 bytes - the time the program started running
  reg[2] = (unsigned int) env->timeBuffer;
}

void tube_Exit(unsigned int *reg) {
  env->exitHandler();
}

// The below do not seem to be used in BBC Basic, so implement later on

void tube_SetEnv(unsigned int *reg) {
  unsigned int previous;
  // R0 address of exit routine for Exit above to go to (or 0 if no change)
  previous = (unsigned int)env->exitHandler;
  if (reg[0]) {
	env->exitHandler = (ExitHandler_type) reg[0]; 
  }
  reg[0] = previous;
  // R1 address of end of memory limit for GetEnv to read (or 0 if no change)
  previous = env->memoryLimit;
  if (reg[1]) {
	env->memoryLimit = reg[1];
  }
  reg[1] = previous;
  // R2 address of the real end of memory (or 0 if no change)
  previous = env->realEndOfMemory;
  if (reg[2]) {
	env->realEndOfMemory = reg[2];
  }
  reg[2] = previous;
  // R3 0 for no local buffering, 1 for local buffering (anything else no change)
  previous = env->localBuffering;
  if (reg[3]) {
	env->localBuffering = reg[3];
  }
  reg[3] = previous;
  // R4 address of routine to handle undefined instructions (or 0 if no change)
  previous = (unsigned int) env->undefinedInstructionHandler;
  if (reg[4]) {
	env->undefinedInstructionHandler = (ExceptionHandler_type) reg[4];
  }
  reg[4] = previous;
  // R5 address of routine to handle prefetch abort (or 0 if no change)
  previous = (unsigned int) env->prefetchAbortHandler;
  if (reg[5]) {
	env->prefetchAbortHandler = (ExceptionHandler_type) reg[5];
  }
  reg[5] = previous;
  // R6 address of routine to handle data abort (or 0 if no change)
  previous = (unsigned int) env->dataAbortHandler;
  if (reg[6]) {
	env->dataAbortHandler = (ExceptionHandler_type) reg[6];
  }
  reg[6] = previous;
  // R7 address of routine to handle address exception (or 0 if no change).
  previous = (unsigned int) env->addressExceptionHandler;
  if (reg[7]) {
	env->addressExceptionHandler = (ExceptionHandler_type) reg[7];
  }
  reg[7] = previous;
}

void tube_IntOn(unsigned int *reg) {
  _enable_interrupts();
}

void tube_IntOff(unsigned int *reg) {
  _disable_interrupts();
}

void tube_CallBack(unsigned int *reg) {
}

void tube_EnterOS(unsigned int *reg) {
}

void tube_BreakPt(unsigned int *reg) {
}

void tube_BreakCtrl(unsigned int *reg) {
}

void tube_UnusedSWI(unsigned int *reg) {
}

void tube_UpdateMEMC(unsigned int *reg) {
}

void tube_SetCallBack(unsigned int *reg) {
}

void tube_Mouse(unsigned int *reg) {
}
