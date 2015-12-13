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

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#include "rpi-aux.h"
#include "rpi-armtimer.h"
#include "rpi-gpio.h"
#include "rpi-interrupts.h"
#include "rpi-systimer.h"

#include "debug.h"
#include "startup.h"
#include "swi.h"
#include "spi.h"
#include "tube-lib.h"
#include "tube-env.h"
#include "tube-swi.h"
#include "tube-isr.h"

unsigned int defaultEscapeFlag;

Environment_type defaultEnvironment;

ErrorBuffer_type defaultErrorBuffer;

Environment_type *env = &defaultEnvironment;

const char *banner = "Raspberry Pi ARMv6 Co Processor 900MHz\r\n\n";

const char *prompt = "arm>*";

/***********************************************************
 * Default Handlers
 ***********************************************************/

// Note: this will be executed in user mode
void defaultErrorHandler(ErrorBuffer_type *eb) {
  // TODO: Consider resetting the user stack?
  if (DEBUG) {
    printf("Error = %p %02x %s\r\n", eb->errorAddr, eb->errorBlock.errorNum, eb->errorBlock.errorMsg);
  }
  sendString(R1, 0x00, eb->errorBlock.errorMsg);
  sendString(R1, 0x00, "\n\r");
  SWI_OS_Exit;
}

// Entered with R11 bit 6 as escape status
// R12 contains 0/-1 if not in/in the kernal presently
// R11 and R12 may be altered. Return with MOV PC,R14
// If R12 contains 1 on return then the Callback will be used

// Note, the way we invoke this via the _escape_handler_wrapper will
// work, because the flag will also still be in r0.
void defaultEscapeHandler(unsigned int flag) {
  if (DEBUG) {
    printf("Escape flag = %02x\r\n", flag);
  }
  *((unsigned int *)(env->handler[ESCAPE_HANDLER].address)) = flag;
}

// Entered with R0, R1 and R2 containing the A, X and Y parameters. R0,
// R1, R2, R11 and R12 may be altered. Return with MOV PC, R14
// R12 contains 0/-1 if not in/in the kernal presently
// R13 contains the IRQ handling routine stack. When you return to the
// system LDMFD R13!, (R0,R1,R2,R11,R12,PC}^ will be executed. If R12
// contains 1 on return then the Callback will be used.
void defaultEventHandler(unsigned int a, unsigned int x, unsigned int y) {
  if (DEBUG) {
    printf("Event: A=%02x X=%02x Y=%02x\r\n", a, x, y);
  }
}

// This should be called in USR mode whenever OS_Exit or OS_ExitAndDie is called.
void defaultExitHandler() {
  if (DEBUG) {
    printf("Invoking default exit handler\r\n");
  }
  SWI_OS_EnterOS;
}

void defaultUndefinedInstructionHandler() {
  handler_not_implemented("UNDEFINED_INSTRUCTION_HANDLER");
}

void defaultPrefetchAbortHandler() {
  handler_not_implemented("PREFETCH_ABORT_HANDLER");
}

void defaultDataAbortHandler() {
  handler_not_implemented("DATA_ABORT_HANDLER");
}

void defaultAddressExceptionHandler() {
  handler_not_implemented("ADDRESS_EXCEPTION_HANDLER");
}

void defaultOtherExceptionHandler() {
  handler_not_implemented("OTHER_EXCEPTIONS_HANDLER");
}

void defaultCallbackHandler() {
  handler_not_implemented("CALLBACK_HANDLER");
}

void defaultUnusedSWIHandler() {
  handler_not_implemented("UNUSED_SWI_HANDLER");
}

void defaultExceptionRegistersHandler() {
  handler_not_implemented("EXCEPTION_REGISTERS_HANDLER");
}

void defaultUpcallHandler() {
  handler_not_implemented("UPCALL_HANDLER");
}

/***********************************************************
 * Initialize the envorinment
 ***********************************************************/

void initEnv() {
  defaultEscapeFlag = 0;
  int i;
  for (i = 0; i < sizeof(env->commandBuffer); i++) {
    env->commandBuffer[i] = 0;
  }
  for (i = 0; i < sizeof(env->timeBuffer); i++) {
    env->timeBuffer[i] = 0;
  }
  for (i = 0; i < NUM_HANDLERS; i++) {
    env->handler[i].address = (void *)0;
    env->handler[i].r12 = 0xAAAAAAAA;
  }
  // Handlers that are code points
  env->handler[  UNDEFINED_INSTRUCTION_HANDLER].handler = defaultUndefinedInstructionHandler;
  env->handler[         PREFETCH_ABORT_HANDLER].handler = defaultPrefetchAbortHandler;
  env->handler[             DATA_ABORT_HANDLER].handler = defaultDataAbortHandler;
  env->handler[      ADDRESS_EXCEPTION_HANDLER].handler = defaultAddressExceptionHandler;
  env->handler[       OTHER_EXCEPTIONS_HANDLER].handler = defaultOtherExceptionHandler;
  env->handler[                  ERROR_HANDLER].handler = defaultErrorHandler;
  env->handler[                  ERROR_HANDLER].address = &defaultErrorBuffer;
  env->handler[               CALLBACK_HANDLER].handler = defaultCallbackHandler;
  env->handler[                 ESCAPE_HANDLER].handler = defaultEscapeHandler;
  env->handler[                 ESCAPE_HANDLER].address = &defaultEscapeFlag;
  env->handler[                  EVENT_HANDLER].handler = defaultEventHandler;
  env->handler[                   EXIT_HANDLER].handler = defaultExitHandler;
  env->handler[             UNUSED_SWI_HANDLER].handler = defaultUnusedSWIHandler;
  env->handler[    EXCEPTION_REGISTERS_HANDLER].handler = defaultExceptionRegistersHandler;
  env->handler[                 UPCALL_HANDLER].handler = defaultUpcallHandler;

  // Handlers where the handler is just data
  env->handler[           MEMORY_LIMIT_HANDLER].handler = (EnvironmentHandler_type) (2 * 1024 * 1024);
  env->handler[      APPLICATION_SPACE_HANDLER].handler = (EnvironmentHandler_type) (3 * 1024 * 1024);
  env->handler[CURRENTLY_ACTIVE_OBJECT_HANDLER].handler = (EnvironmentHandler_type) (0);



}

/***********************************************************
 * Initialize the hardware
 ***********************************************************/

void initHardware() {
  /* Write 1 to the LED init nibble in the Function Select GPIO
     peripheral register to enable LED pin as an output */
  RPI_GetGpio()->LED_GPFSEL |= LED_GPFBIT;

  /* Configure our pins as inputs */
  RPI_SetGpioPinFunction(IRQ_PIN, FS_INPUT);
  RPI_SetGpioPinFunction(NMI_PIN, FS_INPUT);
  RPI_SetGpioPinFunction(RST_PIN, FS_INPUT);

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
  setjmp(enterOS);

  // Send reset message
  tube_Reset();

  while( 1 ) {

    // When SWI OS_EnterOS is called, we return here
    setjmp(enterOS);

    // Enable interrupts!
    _enable_interrupts();

    if (DEBUG) {
      printf("%08x\r\n", _get_cpsr());
    }

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
      tube_CLI(reg);
    }
  }

  // This never gets called
  spi_end();

}
