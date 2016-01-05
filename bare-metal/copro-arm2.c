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
#include "mame/arm.h"
#include "tuberom_arm.h"
#include "copro-arm2.h"

#define RAM_MASK8    ((UINT32) 0x003fffff)
#define ROM_MASK8    ((UINT32) 0x00003fff)
#define RAM_MASK32   ((UINT32) 0x003ffffc)
#define ROM_MASK32   ((UINT32) 0x00003ffc)

// 4MB of RAM starting at 0x00000000
UINT8  arm2_ram[1024*1024*4] __attribute__((aligned(0x10000)));

// 16KB of ROM starting at 0x03000000
UINT8  arm2_rom[0x4000] __attribute__((aligned(0x10000)));

#define R15 arm2_getR15()

UINT8  copro_arm2_read8(int addr) {
  int type = (addr >> 24) & 3;
  switch (type) {
  case 0:
    return *(UINT8*)(arm2_ram + (addr & RAM_MASK8));
  case 1:
    return tubeRead((addr >> 2) & 7);
  case 3:
    return *(UINT8*)(arm2_rom + (addr & ROM_MASK8));
  }
  return 0;

}

UINT32 copro_arm2_read32(int addr) {
  UINT32 result;
  int type = (addr >> 24) & 3;
  switch (type) {
  case 0:
    result = *(UINT32*)(arm2_ram + (addr & RAM_MASK32));
    break;
  case 1:
    result = tubeRead((addr >> 2) & 7);
    break;
  case 3:
    result = *(UINT32*)(arm2_rom + (addr & ROM_MASK32));
    break;
  default:
    result = 0;
  }
  /* Unaligned reads rotate the word, they never combine words */
  if (addr&3)
	{
      if (ARM_DEBUG_CORE && addr&1)
        logerror("%08x: Unaligned byte read %08x\n",R15,addr);
      if ((addr&3)==1)
        return ((result&0x000000ff)<<24)|((result&0xffffff00)>> 8);
      if ((addr&3)==2)
        return ((result&0x0000ffff)<<16)|((result&0xffff0000)>>16);
      if ((addr&3)==3)
        return ((result&0x00ffffff)<< 8)|((result&0xff000000)>>24);
	}
  return result;
}

void   copro_arm2_write8(int addr, UINT8 data) {
  int type = (addr >> 24) & 3;
  switch (type) {
  case 0:
    *(UINT8*)(arm2_ram + (addr & RAM_MASK8)) = data;
    break;
  case 1:
    tubeWrite((addr >> 2) & 7, data);
    break;
  }
}

void   copro_arm2_write32(int addr, UINT32 data) {
  int type = (addr >> 24) & 3;
  switch (type) {
  case 0:
    *(UINT32*)(arm2_ram + (addr & RAM_MASK32)) = data;
    break;
  case 1:
    tubeWrite((addr >> 2) & 7, data);
    break;
  }
  /* Unaligned writes are treated as normal writes */  
  if (addr&3) printf("%08x: Unaligned write %08x\n",R15,addr);
}

void copro_arm2_init_hardware()
{
#ifdef JTAG_DEBUG
  // See http://sysprogs.com/VisualKernel/tutorials/raspberry/jtagsetup/
  RPI_SetGpioPinFunction(RPI_GPIO4, FS_ALT5);		// TDI
  RPI_SetGpioPinFunction(RPI_GPIO22, FS_ALT4);		// nTRST
  RPI_SetGpioPinFunction(RPI_GPIO23, FS_ALT4);		// RTCK
  RPI_SetGpioPinFunction(RPI_GPIO24, FS_ALT4);		// TDO
  RPI_SetGpioPinFunction(RPI_GPIO25, FS_ALT4);		// TCK
  RPI_SetGpioPinFunction(RPI_GPIO27, FS_ALT4);		// TMS
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

static void copro_arm2_reset() {
  // Wipe memory
  memset(arm2_ram, 0, sizeof(arm2_ram));
  // Re-instate the Tube ROM on reset
  memcpy(arm2_ram, tuberom_arm_v100, 0x4000);
  memcpy(arm2_rom, tuberom_arm_v100, 0x4000);
  // Reset the ARM device
  arm2_device_reset();
}


void copro_arm2_main(unsigned int r0, unsigned int r1, unsigned int atags) {

  static unsigned int last_irqn = 1;
  static unsigned int last_nmin = 1;
  static unsigned int last_rstn = 1;

  // Initialise the UART
  RPI_AuxMiniUartInit( 57600, 8 );

  printf("Raspberry Pi ARM2 Client\r\n" );

  copro_arm2_init_hardware();

  enable_MMU_and_IDCaches();
  _enable_unaligned_access();

  printf("Initialise UART console with standard libc\r\n" );

  // This ensures interrupts are not re-enabled in tube-lib.c
  in_isr = 1;

  while (1) {
    unsigned int gpio = RPI_GpioBase->GPLEV0; 
    unsigned int irqn = gpio & IRQ_PIN_MASK;
    unsigned int nmin = gpio & NMI_PIN_MASK;
    unsigned int rstn = gpio & RST_PIN_MASK;

    if (rstn != 0 && last_rstn == 0) {
      printf("RST!!\r\n");
      copro_arm2_reset();
    }

    // TODO: IRQ is level sensitive
    if (irqn != last_irqn) {
      //printf("IRQ!!\r\n");
      arm2_execute_set_input(ARM_IRQ_LINE, irqn ? 0 : 1);
    }

    // NMI is edge sensitive
    if (nmin == 0 && last_nmin != 0) {
      //printf("NMI!!\r\n");
      arm2_execute_set_input(ARM_FIRQ_LINE, 1);
    }
    if (rstn != 0) {
      arm2_execute_run(0);
      //printf("%08x\r\n", R15);
    } 
    last_irqn = irqn;
    last_nmin = nmin;
    last_rstn = rstn;
  }
}
