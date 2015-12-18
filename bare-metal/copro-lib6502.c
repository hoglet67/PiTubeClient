/*
 * 6502 Co Processor Emulation
 *
 * (c) 2015 David Banks
 * 
 * based on code by
 * - Reuben Scratton.
 * - Tom Walker
 *
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
#include "lib6502.h"
#include "tuberom_6502.h"
#include "copro-lib6502.h"

int tracing=0;

void copro_lib6502_init_hardware()
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

  // Initialise the UART
  RPI_AuxMiniUartInit( 57600, 8 );

  // Setup SP
  spi_begin();
}

static void copro_lib6502_reset(M6502 *mpu) {
  memset(mpu->memory, 0, 0x10000);
  // Re-instate the Tube ROM on reset
  memcpy(mpu->memory + 0xf800, tuberom_6502_orig, 0x800);
  // Reset lib6502
  M6502_reset(mpu);
}

static int copro_lib6502_tube_read(M6502 *mpu, uint16_t addr, uint8_t data) {
  return tubeRead(addr);
}

static int copro_lib6502_tube_write(M6502 *mpu, uint16_t addr, uint8_t data)	{
  tubeWrite(addr, data);
  return 0;
}

static void copro_lib6502_poll(M6502 *mpu) {
  static unsigned int last_nmin = 1;
  static unsigned int last_rstn = 1;
  unsigned int gpio = RPI_GpioBase->GPLEV0; 
  unsigned int irqn = gpio & IRQ_PIN_MASK;
  unsigned int nmin = gpio & NMI_PIN_MASK;
  unsigned int rstn = gpio & RST_PIN_MASK;
  // Reset the 6502 on a rising edge of rstn
  if (rstn != 0 && last_rstn == 0) {
    copro_lib6502_reset(mpu);
  }
  // IRQ is level sensitive
  if (irqn == 0 ) {
    if (!(mpu->registers->p & 4)) {
      M6502_irq(mpu);
    }
  }
  // NMI is edge sensitive
  if (nmin == 0 && last_nmin != 0) {
    M6502_nmi(mpu);
  }
  last_nmin = nmin;
  last_rstn = rstn;
}

void copro_lib6502_main() {
  uint16_t addr;

  copro_lib6502_init_hardware();
  
  printf( "Raspberry Pi lib6502 Tube Client\r\n" );

  enable_MMU_and_IDCaches();
  _enable_unaligned_access();
  
  printf( "Initialise UART console with standard libc\r\n" );

  M6502 *mpu= M6502_new(0, 0, 0);

  for (addr= 0xfef8; addr <= 0xfeff; addr++) {
    M6502_setCallback(mpu, read,  addr, copro_lib6502_tube_read);
    M6502_setCallback(mpu, write, addr, copro_lib6502_tube_write);
  }

  copro_lib6502_reset(mpu);

  M6502_run(mpu, copro_lib6502_poll);
  //M6502_run(mpu);
  M6502_delete(mpu);
}
