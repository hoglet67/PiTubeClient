#include "rpi-base.h"
#include "rpi-aux.h"
#include "rpi-gpio.h"
#include "rpi-interrupts.h"
#include "tube-isr.h"

void dump_hex(unsigned int h) {
  int i;
  for (i = 0; i < 8; i++) {   
	int c = h >> 28;
	if (c < 10) {
	  c = '0' + c;
	} else {
	  c = 'A' + c - 10;
	}
	RPI_AuxMiniUartWrite(c);
	h <<= 4;
  }
}

void dump_string(char *string) {
  char c;
  while ((c = *string++) != 0) {
	RPI_AuxMiniUartWrite(c);
  }	
}

// For some reason printf generally doesn't work here
void dump_info(unsigned int *reg, int offset, char *type) {
  unsigned int *addr;
  int i;
  int rstlow;
  int led;
  dump_string(type);
  dump_string(" at ");
  // Make sure we avoid unaligned accesses
  reg = (unsigned int *)(((unsigned int) reg) & ~3);
  // The stacked LR points one or two words afer the exception address
  addr = (unsigned int *)((reg[13] & ~3) - offset);
  dump_hex((unsigned int)addr);
  dump_string("\r\nRegisters:\r\n");
  for (i = 0; i <= 13; i++) {
	dump_string("  r[");
	RPI_AuxMiniUartWrite('0' + (i / 10));  
	RPI_AuxMiniUartWrite('0' + (i % 10));  
	dump_string("]=");
	dump_hex(reg[i]);
	dump_string("\r\n");
  }
  dump_string("Memory:\r\n");
  for (i = -4; i <= 4; i++) {
	dump_string("  ");
	dump_hex((unsigned int) (addr + i));
	RPI_AuxMiniUartWrite('=');
	dump_hex(*(addr + i));
	if (i == 0) {
	  dump_string(" <<<<<< \r\n");
	} else {
	  dump_string("\r\n");
	}
  }
  dump_string("Stack: ");
  dump_hex((unsigned int) reg);
  dump_string("\r\nHalted waiting for reset\r\n");
  rstlow = 0;
  led = 0;
  while (1) {
	for (i = 0; i < 1000000; i++) {
	  // look for reset being low
	  if (!(RPI_GetGpio()->GPLEV0 & RST_PIN_MASK)) {
		rstlow = 1;
	  }
	  // then reset on the next rising edge
	  if (rstlow && (RPI_GetGpio()->GPLEV0 & RST_PIN_MASK)) {
		reboot_now();
	  }
	}
	if (led) {
	  LED_OFF();
	} else {
	  LED_ON();
	}
	led = ~led;
  }
}

void undefined_instruction_handler(unsigned int *reg) {
  dump_info(reg, 4, "Undefined Instruction");
}

void prefetch_abort_handler(unsigned int *reg) {
  dump_info(reg, 4, "Prefetch Abort");
}

void data_abort_handler(unsigned int *reg) {
  dump_info(reg, 8, "Data Abort");
}

