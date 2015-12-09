/*

    Part of the Raspberry-Pi Bare Metal Tutorials
    Copyright (c) 2015, Brian Sidebotham
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>

#include "rpi-aux.h"
#include "rpi-armtimer.h"
#include "rpi-gpio.h"
#include "rpi-interrupts.h"
#include "rpi-systimer.h"

#include "debug.h"
#include "startup.h"
#include "spi.h"
#include "tube-lib.h"
#include "tube-env.h"
#include "tube-swi.h"
#include "tube-isr.h"

Environment_type defaultEnvironment;

ErrorBuffer_type defaultErrorBuffer;

Environment_type *env = &defaultEnvironment;

const char *banner = "Raspberry Pi ARMv6 Co Processor 900MHz\r\n\n";

const char *help = "ARM Tube Client 0.10\r\n";

const char *prompt = "arm>*";

char line[80];

jmp_buf errorRestart;

unsigned int memAddr;

int doCmdHelp(char *params);
int doCmdGo(char *params);
int doCmdMem(char *params);
int doCmdFill(char *params);
int doCmdCrc(char *params);

// The Atom CRC Polynomial
#define CRC_POLY          0x002d

/***********************************************************
 * Build in Commands
 ***********************************************************/

#define NUM_CMDS 5

// Must be kept in step with cmdFuncs (just below)
char *cmdStrings[NUM_CMDS] = {
  "HELP",
  "GO",
  "MEM",
  "FILL",
  "CRC"
};

int (*cmdFuncs[NUM_CMDS])(char *params) = {
  doCmdHelp,
  doCmdGo,
  doCmdMem,
  doCmdFill,
  doCmdCrc
};

int dispatchCmd(char *cmd) {
  int i;
  int c;
  int r;
  char *cmdPtr;
  char *refPtr;
  //  skip any leading space
  while (isblank((int) *cmd)) {
	cmd++;
  }
  // Match the command 
  for (i = 0; i < NUM_CMDS; i++) {
	cmdPtr = cmd;
	refPtr = cmdStrings[i];
	do {
	  c = tolower((int)*cmdPtr++);	  
	  r = tolower((int)*refPtr++);
	  if (r == 0 || c == '.') {
		// skip any trailing space becore the params
		while (isblank((int)*cmdPtr)) {
		  cmdPtr++;
		}
		return (*cmdFuncs[i])(cmdPtr);
	  }
	} while (c != 0 && c == r);
  }
  // non-zero means pass the command onto the CLI processor
  return 1;
}

int doCmdHelp(char *params) {
  int i;
  sendString(R1, 0x00, help);
  sendByte(R1, 0x00);
  if (strcasecmp(params, "ARM") == 0) {
	for (i = 0; i < NUM_CMDS; i++) {
	  sendString(R1, 0x00, "  ");
	  sendString(R1, 0x00, cmdStrings[i]);
	  sendString(R1, 0x00, "\r\n");
	  sendByte(R1, 0x00);
	}
	return 0;
  }
  // pass the command on to the CLI
  return 1;
}

int doCmdGo(char *params) {
  unsigned int address;
  sscanf(params, "%x", &address);
  // No return from here
  user_exec((unsigned char *)address);
  return 0;
}

int doCmdFill(char *params) {
  unsigned int i;
  unsigned int start;
  unsigned int end;
  unsigned int data;
  sscanf(params, "%x %x %x", &start, &end, &data);
  for (i = start; i <= end; i++) {
	*((unsigned char *)i) = data;
  }
  return 0;
}

int doCmdMem(char *params) {
  int i, j;
  unsigned char c;
  char *ptr;
  sscanf(params, "%x", &memAddr);
  for (i = 0; i < 0x100; i+= 16) {
	ptr = line;
	// Generate the address
	ptr += sprintf(ptr, "%08X ", (memAddr + i));	
	  // Generate the hex values
	for (j = 0; j < 16; j++) {
	  c = *((unsigned char *)(memAddr + i + j));
	  ptr += sprintf(ptr, "%02X ", c);
	}
	// Generate the ascii values
	for (j = 0; j < 16; j++) {
	  c = *((unsigned char *)(memAddr + i + j));
      if (c < 32 || c > 126) {
		c = '.';
      }
	  ptr += sprintf(ptr, "%c", c);
	}
	ptr += sprintf(ptr, "\r\n");
	sendString(R1, 0x00, line);
	sendByte(R1, 0x00);
  }
  memAddr += 0x100;
  return 0;
}

int doCmdCrc(char *params) {
  unsigned int i;
  unsigned int j;
  unsigned int start;
  unsigned int end;
  unsigned int data;
  unsigned long crc = 0;
  sscanf(params, "%x %x %x", &start, &end, &data);
  for (i = start; i <= end; i++) {
	data = *((unsigned char *)i);
    for (j = 0; j < 8; j++) {
      crc = crc << 1;
      crc = crc | (data & 1);
      data >>= 1;
      if (crc & 0x10000)
		crc = (crc ^ CRC_POLY) & 0xFFFF;
    }
  }
  sprintf(line, "%04X\r\n", (unsigned short)crc);
  sendString(R1, 0x00, line);
  sendByte(R1, 0x00);
  return 0;
}

/***********************************************************
 * Default Handlers
 ***********************************************************/

// TODO Register usage incorrect!!
void defaultErrorHandler() {
  ErrorBuffer_type* eb = env->errorBuffer;
  if (DEBUG) {
	printf("Error = %02x %s\r\n", eb->errorNum, eb->errorMsg);
  }
  sendString(R1, 0x00, eb->errorMsg);
  sendString(R1, 0x00, "\n\r");
  env->exitHandler();
}

// Entered with R11 bit 6 as escape status
// R12 contains 0/-1 if not in/in the kernal presently
// R11 and R12 may be altered. Return with MOV PC,R14
// If R12 contains 1 on return then the Callback will be used 
// TODO Register usage incorrect!!
void defaultEscapeHandler(unsigned int reg0) {
  if (DEBUG) {
	printf("Escape flag = %02x\r\n", reg0);
  }
  env->escapeFlag = reg0;
}

// Entered with R0, R1 and R2 containing the A, X and Y parameters. R0,
// R1, R2, R11 and R12 may be altered. Return with MOV PC, R14
// R12 contains 0/-1 if not in/in the kernal presently
// R13 contains the IRQ handling routine stack. When you return to the
// system LDMFD R13!, (R0,R1,R2,R11,R12,PC}^ will be executed. If R12
// contains 1 on return then the Callback will be used. 
// TODO Register usage incorrect!!
void defaultEventHandler(unsigned int reg0, unsigned int reg1, unsigned int reg2) {
  if (DEBUG) {
	printf("Event = %02x %02x %02x\r\n", reg0, reg1, reg2);
  }
}

void defaultExitHandler() {
  _isr_longjmp(errorRestart, 1);
}

void defaultExceptionHandler() {
}

/***********************************************************
 * Initialize the envorinment
 ***********************************************************/

void initEnv() {
  int i;
  for (i = 0; i < sizeof(env->commandBuffer); i++) {
	env->commandBuffer[i] = 0;
  }
  for (i = 0; i < sizeof(env->timeBuffer); i++) {
	env->timeBuffer[i] = 0;
  }
  env->escapeFlag                  = 0;
  env->memoryLimit                 = 2 * 1024 * 1024;
  env->realEndOfMemory             = 3 * 1024 * 1024;
  env->localBuffering              = 0;
  env->errorHandler                = defaultErrorHandler;
  env->errorBuffer                 = &defaultErrorBuffer;
  env->escapeHandler               = defaultEscapeHandler;
  env->eventHandler                = defaultEventHandler;
  env->exitHandler                 = defaultExitHandler;
  env->undefinedInstructionHandler = defaultExceptionHandler;
  env->prefetchAbortHandler        = defaultExceptionHandler;
  env->dataAbortHandler            = defaultExceptionHandler;
  env->addressExceptionHandler     = defaultExceptionHandler;
}

/***********************************************************
 * Initialize the hardware
 ***********************************************************/

void initHardware() {
  /* Write 1 to the LED init nibble in the Function Select GPIO
     peripheral register to enable LED pin as an output */
  RPI_GetGpio()->LED_GPFSEL |= LED_GPFBIT;

  /* Configure GPIO to detect a falling edge of the IRQ pin */
  RPI_GetGpio()->GPFEN0 |= IRQ_PIN_MASK;

  /* Make sure there are no pending detections */
  RPI_GetGpio()->GPEDS0 = IRQ_PIN_MASK;

  /* Configure GPIO to detect a rising edge of the RST pin */
  RPI_GetGpio()->GPREN0 |= RST_PIN_MASK;

  /* Make sure there are no pending detections */
  RPI_GetGpio()->GPEDS0 = RST_PIN_MASK;

  /* Enable gpio_int[0] which is IRQ 49 */
  RPI_GetIrqController()->Enable_IRQs_2 = (1 << (49 - 32));

  /* Enable the timer interrupt IRQ */
  // RPI_GetIrqController()->Enable_Basic_IRQs = RPI_BASIC_ARM_TIMER_IRQ;

  /* Setup the system timer interrupt */
  /* Timer frequency = Clk/256 * 0x400 */
  RPI_GetArmTimer()->Load = 0x400;

  /* Setup the ARM Timer */
  RPI_GetArmTimer()->Control =
    RPI_ARMTIMER_CTRL_23BIT |
    RPI_ARMTIMER_CTRL_ENABLE |
    RPI_ARMTIMER_CTRL_INT_ENABLE |
    RPI_ARMTIMER_CTRL_PRESCALE_256;

  /* Enable interrupts! */
  _enable_interrupts();

  /* Initialise the UART */
  RPI_AuxMiniUartInit( 57600, 8 );

  /* Setup SPI*/
  spi_begin();
}

// Tube Reset protocol
void tube_Reset() {
  // Print to the UART using the standard libc functions
  printf( "Raspberry Pi ARMv6 Tube Client\r\n" );
  printf( "Initialise UART console with standard libc\r\n" );

  // Send the reset message
  printf( "Sending banner\r\n" );
  sendString(R1, 0x00, banner);
  sendByte(R1, 0x00);
  printf( "Banner sent, awaiting response\r\n" );

  // Wait for the reponse in R2
  receiveByte(R2);
  printf( "Received response\r\n" );
}

/***********************************************************
 * Main function - we'll never return from here
 ***********************************************************/

void kernel_main( unsigned int r0, unsigned int r1, unsigned int atags )
{
  // Register block used to interact with tube_ functions
  unsigned int block[5];
  unsigned int *carry = &block[0];
  unsigned int *reg = &block[1];

  // Initialize the environment structure
  initEnv();

  // Initialize the hardware
  initHardware();

  // This should not be necessary, but I've seen a couple of cases
  // where R4 errors happened during the startup message
  setjmp(errorRestart);

  // Send reset message
  tube_Reset();

  while( 1 ) {

    // If an error is received on R4 (in the ISR) we return here
    setjmp(errorRestart);

    // Print the supervisor prompt
	reg[0] = (unsigned int) prompt;
	tube_Write0(reg);

    // Ask for user input (OSWORD 0)
	reg[0] = (unsigned int) env->commandBuffer;
	reg[1] = 0x7F; // max line length
	reg[2] = 0x20; // min ascii value
	reg[3] = 0x7F; // max ascii value
	tube_ReadLine(reg);

    // Was it escape
    if ((*carry) & CARRY_MASK) {

      // Yes, print Escape
      sendString(R1, 0x00, "\n\rEscape\n\r");

      // Acknowledge escape condition
	  if (DEBUG) {
		printf("Acknowledging Escape\r\n");
	  }
	  reg[0] = 0x7e;
	  reg[1] = 0x00;
	  tube_Byte(reg);

    } else {
      if (dispatchCmd(env->commandBuffer)) {
		// Return code 0 means the command was successfully handled locally
		tube_CLI(reg);
	  }
    }
  }

  // This never gets called
  spi_end();

}
