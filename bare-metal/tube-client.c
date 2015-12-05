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

#include "spi.h"

#define BITSPERWORD 8

#define CMD_WRITE 0x80
#define CMD_READ  0xC0

#define R1 1
#define R2 2
#define R3 3
#define R4 4

#define R1_STATUS 0
#define R1_DATA   1
#define R2_STATUS 2
#define R2_DATA   3
#define R3_STATUS 4
#define R3_DATA   5
#define R4_STATUS 6
#define R4_DATA   7

#define A_BIT 0x80
#define F_BIT 0x40

#define DEBUG 1
#define DEBUGDETAIL 0

// BCM GPIO 25
#define IRQ_PIN 6

const char *banner = "Raspberry Pi ARMv6 Co Processor 900MHz\n\n\r";

void tubeWrite(unsigned char addr, unsigned char byte);
unsigned char tubeRead(unsigned char addr);
unsigned char tubeCmd(unsigned char cmd, unsigned char addr, unsigned char byte);
void sendByte(unsigned char reg, unsigned char byte);
void sendString(unsigned char reg, const volatile char *buf);
unsigned char receiveByte(unsigned char reg);
void receiveString(unsigned char reg, unsigned char terminator, volatile char *buf);
unsigned char pollForResponse(unsigned char reg);

static volatile int reset;
static volatile int error;

static volatile unsigned char escFlag;
static volatile unsigned char errNum;

static volatile char errMsg[256];

extern volatile int calculate_frame_count;




/** Main function - we'll never return from here */
void kernel_main( unsigned int r0, unsigned int r1, unsigned int atags )
{
  unsigned char resp;
  char buffer[256];

  /* Write 1 to the LED init nibble in the Function Select GPIO
	 peripheral register to enable LED pin as an output */
  RPI_GetGpio()->LED_GPFSEL |= LED_GPFBIT;

  /* Enable the timer interrupt IRQ */
  RPI_GetIrqController()->Enable_Basic_IRQs = RPI_BASIC_ARM_TIMER_IRQ;

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

  /* Print to the UART using the standard libc functions */
  printf( "Raspberry Pi ARMv6 Tube Client\r\n" );
  printf( "Initialise UART console with standard libc\r\n\n" );


  // Hook in the interrupt handler
  //if (wiringPiISR(IRQ_PIN, INT_EDGE_FALLING, &IRQInterrupt) < 0) {
  //  perror("wiringPiISR failed");
  //  exit(1);
  //}

  reset = 1;

  while( 1 ) {

	if (calculate_frame_count) {
	  printf( "Interrupt!!\r\n" );
	  calculate_frame_count = 0;
	}

    if (reset) {
      reset = 0;
      // Send the reset message
      sendString(R1, banner);
      sendByte(R1, 0x00);
      // Wait for the reponse in R2
      receiveByte(R2);
    }

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
          printf("Acknowledging Escape\n");
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

      if (error) {
        error = 0;
        sendString(R1, errMsg);
        sendString(R1, "\n\r");
      }
    }
  }
}

void IRQInterrupt(void) {
  // Check for R1 interrupt
  if (tubeRead(R1_STATUS) & A_BIT) {
    unsigned char flag = tubeRead(R1_DATA);
    if (flag & 0x80) {
      // Update escape flag
      escFlag = flag & 0x40;
      if (DEBUG) {
        printf("Escape flag = %02x\n", escFlag);
      }
    } else {
      // Event
      unsigned char y = receiveByte(R1);
      unsigned char x = receiveByte(R1);
      unsigned char a = receiveByte(R1);
      if (DEBUG) {
        printf("Event = %02x %02x %02x\n", a, x, y);
      }
    }    
  }
  // Check for R4 interrupt
  if (tubeRead(R4_STATUS) & A_BIT) {
    unsigned char type = tubeRead(R4_DATA);
    if (type == 0xff) {
      // Error
      receiveByte(R2); // 0
      errNum = receiveByte(R2);
      receiveString(R2, 0x00, errMsg);
      // Flag an error to the main process
      error = 1;
      if (DEBUG) {
        printf("Error = %02x %s\n", errNum, errMsg);
      }
    } else {
      unsigned char id = receiveByte(R4);
      if (DEBUG) {
        printf("Transfer = %02x %02x", type, id);
      }
      if (type <= 4 || type == 6 || type == 7) {
        unsigned char a3 = receiveByte(R4);
        unsigned char a2 = receiveByte(R4);
        unsigned char a1 = receiveByte(R4);
        unsigned char a0 = receiveByte(R4);        
        if (DEBUG) {
          printf(" %02x%02x%02x%02x", a3, a2, a1, a0);
        }
      }
      if (type < 4 ) {
        // Type 0 .. 3 transfers serviced by NMI
        // TODO setup NMI
      }
      if (type == 6) {
        // Type 6 transfers are 256 byte block p -> h
        // TODO write 256 bytes to R3
        sendByte(R4, 0x00);
      }
      if (type == 7) {
        // Type 7 transfers are 256 byte block h -> p
        // TODO read 256 bytes from R3 
      }
      if (type == 5) {
        // Type 5 : tube release
      } else {
        // Every thing else has a sync byte
        unsigned char sync = receiveByte(R4);
        if (DEBUG) {
          printf(" %02x", sync);
        }
      }
      if (DEBUG) {
        printf("\n");
      }
    }
  }  
}

unsigned char tubeRead(unsigned char addr) {
  return tubeCmd(CMD_READ, addr, 0xff);
}   

void tubeWrite(unsigned char addr, unsigned char byte) {
  tubeCmd(CMD_WRITE, addr, byte);
}   




unsigned char tubeCmd(unsigned char cmd, unsigned char addr, unsigned char byte) {
  unsigned char txBuf[2];
  unsigned char rxBuf[2];
  txBuf[0] = cmd | ((addr & 7) << 3);
  txBuf[1] = byte;
  spi_begin();
  spi_transfer(txBuf, rxBuf, sizeof(txBuf));
  spi_end();
  if (DEBUGDETAIL) {
    // printf("%02x %02x -> %02x %02x\n", cmd0, cmd1, data[0], data[1]);
  }
  return rxBuf[1];
}


// Reg is 1..4
void sendByte(unsigned char reg, unsigned char byte) {
  unsigned char addr = (reg - 1) * 2;
  if (!error) {
    if (DEBUGDETAIL) {
      printf("waiting for space in R%d\n", reg);
    }
    while ((!error) && (tubeRead(addr) & F_BIT) == 0x00);
    if (DEBUGDETAIL) {
      printf("done waiting for space in R%d\n", reg);
    }
  }
  if (!error) {
    tubeWrite((reg - 1) * 2 + 1, byte);
    if (DEBUGDETAIL) {
      printf("Tx: R%d = %02x\n", reg, byte);
    }
  }
}

// Reg is 1..4
unsigned char receiveByte(unsigned char reg) {
  unsigned char byte;
  unsigned char addr = (reg - 1) * 2;
  if (!error) {
    if (DEBUGDETAIL) {
      printf("waiting for data in R%d\n", reg);
    }
    while ((!error) && (tubeRead(addr) & A_BIT) == 0x00);
    if (DEBUGDETAIL) {
      printf("done waiting for data in R%d\n", reg);
    }
  }
  if (!error) {
    byte = tubeRead((reg - 1) * 2 + 1);
    if (DEBUGDETAIL) {
      printf("Rx: R%d = %02x\n", reg, byte);
    }
    return byte;
  }
  return 0;
}

// Reg is 1..4
void sendString(unsigned char reg, const volatile char *buf) {
  int i;
  for (i = 0; i < strlen(buf); i++) {
    sendByte(reg, (unsigned char)buf[i]);
  }
}

// Reg is 1..4
void receiveString(unsigned char reg, unsigned char terminator, volatile char *buf) {
  int i = 0;
  unsigned char c;
  do {
    c = receiveByte(reg);
    if (c != terminator) {
      buf[i++] = (char) c;
    }
  } while (c != terminator  && i < 100);
  buf[i++] = 0;
}

