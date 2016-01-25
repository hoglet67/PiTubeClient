/*
 * Arm2 Co Pro Emulation
 *
 * (c) 2015 David Banks
 * 
 * based on code by from MAME
 */

#define JTAG_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "startup.h"
#include "spi.h"
#include "tube-lib.h"
#include "tube-isr.h"
#include "rpi-gpio.h"
#include "rpi-aux.h"
#include "rpi-interrupts.h"
#include "../cpu80186/cpu80186.h"
#include "../cpu80186/mem80186.h"

uint8_t Exit;

void copro_80186_init_hardware()
{
#ifdef JTAG_DEBUG
  // See http://sysprogs.com/VisualKernel/tutorials/raspberry/jtagsetup/
  RPI_SetGpioPinFunction(RPI_GPIO4, FS_ALT5);          // TDI
  RPI_SetGpioPinFunction(RPI_GPIO22, FS_ALT4);          // nTRST
  RPI_SetGpioPinFunction(RPI_GPIO23, FS_ALT4);          // RTCK
  RPI_SetGpioPinFunction(RPI_GPIO24, FS_ALT4);          // TDO
  RPI_SetGpioPinFunction(RPI_GPIO25, FS_ALT4);          // TCK
  RPI_SetGpioPinFunction(RPI_GPIO27, FS_ALT4);          // TMS
#endif

  // Write 1 to the LED init nibble in the Function Select GPIO
  // peripheral register to enable LED pin as an output
  RPI_GpioBase->LED_GPFSEL |= LED_GPFBIT;

  // Configure our pins as inputs
  RPI_SetGpioPinFunction(IRQ_PIN, FS_INPUT);
  RPI_SetGpioPinFunction(NMI_PIN, FS_INPUT);
  RPI_SetGpioPinFunction(RST_PIN, FS_INPUT);

  // Configure GPIO to detect a falling edge of the IRQ, NMI, RST pins
  //RPI_GpioBase->GPFEN0 |= IRQ_PIN_MASK | NMI_PIN_MASK | RST_PIN_MASK;

  // Make sure there are no pending detections
  //RPI_GpioBase->GPEDS0 = IRQ_PIN_MASK | NMI_PIN_MASK | RST_PIN_MASK;

  // Enable gpio_int[0] which is IRQ 49
  //RPI_GetIrqController()->Enable_IRQs_2 = (1 << (49 - 32));

  // Setup SP
  spi_begin();
}

static void copro_i80186_reset()
{
  // Wipe memory
  Cleari80186Ram();
  RomCopy();
  reset();
  // Reset the ARM device
  //arm2_device_reset();
}

extern uint16_t ip;
extern uint16_t segregs[];

void copro_80186_main(unsigned int r0, unsigned int r1, unsigned int atags)
{
  register unsigned int gpio;
  register unsigned int last_gpio = IRQ_PIN_MASK | NMI_PIN_MASK | RST_PIN_MASK;

  RPI_AuxMiniUartInit(57600, 8);          // Initialise the UART

  printf("Pi i80186 Client\r\n");

  copro_80186_init_hardware();

  enable_MMU_and_IDCaches();
  _enable_unaligned_access();

  copro_i80186_reset();

  printf("Initialise UART console with standard libc\r\n");

  Exit = 0;
  in_isr = 1;          // This ensures interrupts are not re-enabled in tube-lib.c

  while (1)
  {

    printf("%04x:%04x\r\n", segregs[1], ip);

    exec86(16);

    gpio = RPI_GpioBase->GPLEV0;

    if (gpio == last_gpio)
    {
      continue;
    }

    if ((gpio & RST_PIN_MASK) == 0)
    {
      printf("RST!!\r\n");
      copro_i80186_reset();

      if (Exit)          // Set Exit true while in Reset to switch to another co-pro
      {
        break;
      }

      do
      {
        gpio = RPI_GpioBase->GPLEV0;
      }
      while ((gpio & RST_PIN_MASK) == 0);          // Wait for Reset to go high
    }

#if 0
    // TODO: IRQ is level sensitive
    if ((gpio ^ last_gpio) & IRQ_PIN_MASK)
    {
      i80186_execute_set_input(0, (gpio & IRQ_PIN_MASK) ? 0 : 1);
    }

    // NMI is edge sensitive
    if (((gpio & NMI_PIN_MASK) == 0) && (last_gpio & NMI_PIN_MASK))
    {
      i80186_execute_set_input(1, 1);
    }
#endif

    last_gpio = gpio;
  }
}
