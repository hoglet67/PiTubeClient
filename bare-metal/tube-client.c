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

#include "rpi-aux.h"
#include "rpi-armtimer.h"
#include "rpi-gpio.h"
#include "rpi-interrupts.h"
#include "rpi-systimer.h"

#include "debug.h"
#include "spi.h"
#include "tube-lib.h"
#include "tube-isr.h"

const char *banner = "Raspberry Pi ARMv6 Co Processor 900MHz\n\n\r";

/** Main function - we'll never return from here */
void kernel_main( unsigned int r0, unsigned int r1, unsigned int atags )
{
  unsigned char resp;
  char buffer[256];

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

  /* Print to the UART using the standard libc functions */
  printf( "Raspberry Pi ARMv6 Tube Client\r\n" );
  printf( "Initialise UART console with standard libc\r\n" );

  // Send the reset message
  printf( "Sending banner\r\n" );
  sendString(R1, banner);
  sendByte(R1, 0x00);

  // This should not be necessary, but I've seen a couple of cases
  // where R4 errors happened during the startup message
  setjmp(errorRestart);

  printf( "Banner sent, awaiting response\r\n" );

  // Wait for the reponse in R2
  receiveByte(R2);
  printf( "Received response\r\n" );

  while( 1 ) {

	// If an error is received on R4 (in the ISR) we return here
	setjmp(errorRestart);

    // Print a prompt
    sendString(R1, "arm>*");

    // Ask for user input (OSWORD 0)
    sendByte(R2, 0x0A);
    sendByte(R2, 0x7F); // max ascii value
    sendByte(R2, 0x20); // min ascii value
    sendByte(R2, 0x7F); // max line length
    sendByte(R2, 0x07); // Buffer MSB (not used)
    sendByte(R2, 0x00); // Buffer LSB (not used)

    // Read response
    resp = receiveByte(R2);

    // Was it escape
    if (resp >= 0x80) {

      // Yes, print Escape
      sendString(R1, "\n\rEscape\n\r");

      // Acknowledge escape condition
      if (escFlag) {
        if (DEBUG) {
          printf("Acknowledging Escape\r\n");
        }
        sendByte(R2, 0x04);
        sendByte(R2, 0x00);
        sendByte(R2, 0x7E);
        resp = receiveByte(R2);
      }

    } else {
      
      receiveString(R2, '\r', buffer);

      // Check for *QUIT
      if (strcasecmp(buffer, "quit") == 0) {
        // Yes, we're done
        break;
      }

      // Send string back as OSCLI
      sendByte(R2, 0x02);
      sendString(R2, buffer);
      sendByte(R2, 0x0D);

      // Wait for the reponse in R2
      receiveByte(R2);
    }
  }

  spi_end();

}
