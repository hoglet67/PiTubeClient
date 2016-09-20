#include "rpi-base.h"
#include "rpi-aux.h"
#include "rpi-gpio.h"
#include "rpi-interrupts.h"
#include "tube-isr.h"

extern uint32_t startpc;

void dump_hex(unsigned int value) {
  int i;
  for (i = 0; i < 8; i++) {   
	int c = value >> 28;
	if (c < 10) {
	  c = '0' + c;
	} else {
	  c = 'A' + c - 10;
	}
	RPI_AuxMiniUartWrite(c);
	value <<= 4;
  }
}
void dump_binary(unsigned int value) {
  int i;
  for (i = 0; i < 32; i++) {   
	int c = '0' + (value >> 31);
	RPI_AuxMiniUartWrite(c);
	value <<= 1;
  }
}

void dump_string(char *string) {
  char c;
  while ((c = *string++) != 0) {
	RPI_AuxMiniUartWrite(c);
  }	
}

// For some reason printf generally doesn't work here
void dump_info(unsigned int *context, int offset, char *type) {
  unsigned int *addr;
  unsigned int *reg;
  unsigned int flags;
  int i;
  int rstlow;
  int led;

  // Make sure we avoid unaligned accesses
  context = (unsigned int *)(((unsigned int) context) & ~3);
  // context point into the exception stack, at flags, followed by registers 0 .. 13
  reg = context + 1;
  dump_string(type);
  dump_string(" at ");
  // The stacked LR points one or two words afer the exception address
  addr = (unsigned int *)((reg[13] & ~3) - offset);
  dump_hex((unsigned int)addr);
  dump_string("\r\n");
  dump_string("Registers:\r\n");
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
  // The flags are pointed to by context, before the registers
  flags = *context;
  dump_string("Flags: \r\n  NZCV--------------------IFTMMMMM\r\n  ");
  dump_binary(flags);
  dump_string(" (");
  // The last 5 bits of the flags are the mode
  switch (flags & 0x1f) {
  case 0x10:
	dump_string("User");
	break;
  case 0x11:
	dump_string("FIQ");
	break;
  case 0x12:
	dump_string("IRQ");
	break;
  case 0x13:
	dump_string("Supervisor");
	break;
  case 0x17:
	dump_string("Abort");
	break;
  case 0x1B:
	dump_string("Undefined");
	break;
  case 0x1F:
	dump_string("System");
	break;
  default:
	dump_string("Illegal");
	break;
  };
  dump_string(" Mode)\r\n");

  dump_string("32016 PC = ");
  dump_hex(startpc);
  dump_string("\r\n");

  dump_string("32016 Opcode = ");
  dump_hex(read_x32(startpc));
  dump_string("\r\n");

  dump_string("Halted waiting for reset\r\n");
  rstlow = 0;
  led = 0;
  while (1) {
	for (i = 0; i < 1000000; i++) {
	  // look for reset being low
	  if (!(RPI_GpioBase->GPLEV0 & RST_PIN_MASK)) {
		rstlow = 1;
	  }
	  // then reset on the next rising edge
	  if (rstlow && (RPI_GpioBase->GPLEV0 & RST_PIN_MASK)) {
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

void undefined_instruction_handler(unsigned int *context) {
  dump_info(context, 4, "Undefined Instruction");
}

void prefetch_abort_handler(unsigned int *context) {
  dump_info(context, 4, "Prefetch Abort");
}

void data_abort_handler(unsigned int *context) {
  dump_info(context, 8, "Data Abort");
}

