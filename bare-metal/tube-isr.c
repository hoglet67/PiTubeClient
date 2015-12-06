#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "rpi-gpio.h"
#include "tube-isr.h"
#include "tube-lib.h"

volatile unsigned char *address;

volatile unsigned char escFlag;
volatile unsigned char errNum;
volatile char errMsg[256];

// a flag set when we are executing in interrupt mode
// allows the SPI code to reliably disable/enable interrupts
volatile int in_isr;

jmp_buf errorRestart;

extern void _isr_longjmp(jmp_buf env, int val);

// single byte parasite -> host (e.g. *SAVE)
void type_0_data_transfer(void) {
  // Terminate the data transfer if IRQ falls (e.g. interrupt from tube release)
  while (1) {
    // Wait for NMI to fall
    while (RPI_GetGpio()->GPLEV0 & NMI_PIN_MASK) {
      if (!(RPI_GetGpio()->GPLEV0 & IRQ_PIN_MASK)) {
        return;
      }
    }
    // Write the R3 data register, which should also clear the NMI
    tubeWrite(R3_DATA, *address++);
    // Wait for NMI to rise
    while (!(RPI_GetGpio()->GPLEV0 & NMI_PIN_MASK)) {
      if (!(RPI_GetGpio()->GPLEV0 & IRQ_PIN_MASK)) {
        return;
      }
    }
  }
}

// single byte host -> parasite (e.g. *LOAD)
void type_1_data_transfer(void) {
  // Terminate the data transfer if IRQ falls (e.g. interrupt from tube release)
  while (1) {
    // Wait for NMI to fall
    while (RPI_GetGpio()->GPLEV0 & NMI_PIN_MASK) {
      if (!(RPI_GetGpio()->GPLEV0 & IRQ_PIN_MASK)) {
        return;
      }
    }
    // Read the R3 data register, which should also clear the NMI
    *address++ = tubeRead(R3_DATA);
    // Wait for NMI to rise
    while (!(RPI_GetGpio()->GPLEV0 & NMI_PIN_MASK)) {
      if (!(RPI_GetGpio()->GPLEV0 & IRQ_PIN_MASK)) {
        return;
      }
    }
  }
}

void type_2_data_transfer(void) {
}

void type_3_data_transfer(void) {
}

void type_6_data_transfer(void) {
}

void type_7_data_transfer(void) {
}


void TubeInterrupt(void) {

  in_isr = 1;

  // Check for R1 interrupt
  if (tubeRead(R1_STATUS) & A_BIT) {
    if (DEBUG) {
      printf("R1 irq\r\n");
    }
    unsigned char flag = tubeRead(R1_DATA);
    if (flag & 0x80) {
      // Update escape flag
      escFlag = flag & 0x40;
      if (DEBUG) {
        printf("Escape flag = %02x\r\n", escFlag);
      }
    } else {
      // Event
      unsigned char y = receiveByte(R1);
      unsigned char x = receiveByte(R1);
      unsigned char a = receiveByte(R1);
      if (DEBUG) {
        printf("Event = %02x %02x %02x\r\n", a, x, y);
      }
    }
  }

  // Check for R4 interrupt
  if (tubeRead(R4_STATUS) & A_BIT) {
    if (DEBUG) {
      printf("R4 irq\r\n");
    }
    unsigned char type = tubeRead(R4_DATA);
    if (DEBUG) {
      printf("R4 type = %02x\r\n",type);
    }
    if (type == 0xff) {
      // Error
      receiveByte(R2); // 0
      errNum = receiveByte(R2);
      receiveString(R2, 0x00, errMsg);
      if (DEBUG) {
        printf("Error = %02x %s\r\n", errNum, errMsg);
      }
      sendString(R1, errMsg);
      sendString(R1, "\n\r");
      // TODO will eventually call a propert SWI
      in_isr = 0;
      _isr_longjmp(errorRestart, 1);
    } else {
      unsigned char id = receiveByte(R4);
      if (type <= 4 || type == 6 || type == 7) {
        unsigned char a3 = receiveByte(R4);
        unsigned char a2 = receiveByte(R4);
        unsigned char a1 = receiveByte(R4);
        unsigned char a0 = receiveByte(R4);
        address = (unsigned char *)((a3 << 24) + (a2 << 16) + (a1 << 8) + a0);
        if (DEBUG) {
          printf("Transfer = %02x %02x %08x\r\n", type, id, (unsigned int)address);
        }
      } else {
        if (DEBUG) {
          printf("Transfer = %02x %02x\r\n", type, id);
        }
      }
      if (type == 5) {
        // Type 5 : tube release
      } else {
        // Every thing else has a sync byte
        receiveByte(R4);
      }
      // The data transfers are done by polling the GPIO bits for IRQ and NMI
      switch (type) {
      case 0:
        type_0_data_transfer();
        break;
      case 1:
        type_1_data_transfer();
        break;
      case 2:
        type_2_data_transfer();
        break;
      case 3:
        type_3_data_transfer();
        break;
      case 6:
        type_6_data_transfer();
        break;
      case 7:
        type_7_data_transfer();
        break;
      }
    }
  }
  in_isr = 0;
}
