#include <stdio.h>
#include "rpi-aux.h"
#include "rpi-gpio.h"

void dump_info(unsigned int *reg) {
  unsigned int *addr;
  int i;
  for (i = 0; i <= 13; i++) {
	printf("reg[%d] = %0x\r\n", i, reg[i]);
  }
  addr = (unsigned int *)(reg[13] & ~3);
  for (i = -4; i <= 4; i++) {
	printf("%08x = %08x\n", (unsigned int) (addr + i), *(addr + i)); 
  }
  printf("Halted\r\n");
  while (1) {
	for (i = 0; i < 10000000; i++);
	LED_ON();
	for (i = 0; i < 10000000; i++);
	LED_OFF();
  }

}
void undefined_instruction_handler(unsigned int *reg) {
  RPI_AuxMiniUartWrite('*');
  RPI_AuxMiniUartWrite('U');
  RPI_AuxMiniUartWrite('I');
  RPI_AuxMiniUartWrite('*');
  RPI_AuxMiniUartWrite('\r');
  RPI_AuxMiniUartWrite('\n');
  dump_info(reg);
}

void prefetch_abort_handler(unsigned int *reg) {
  RPI_AuxMiniUartWrite('*');
  RPI_AuxMiniUartWrite('P');
  RPI_AuxMiniUartWrite('A');
  RPI_AuxMiniUartWrite('*');
  RPI_AuxMiniUartWrite('\r');
  RPI_AuxMiniUartWrite('\n');
  dump_info(reg);
}

void data_abort_handler(unsigned int *reg) {
  RPI_AuxMiniUartWrite('*');
  RPI_AuxMiniUartWrite('D');
  RPI_AuxMiniUartWrite('A');
  RPI_AuxMiniUartWrite('*');
  RPI_AuxMiniUartWrite('\r');
  RPI_AuxMiniUartWrite('\n');
  dump_info(reg);
}

