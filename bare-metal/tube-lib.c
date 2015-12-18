#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "startup.h"
#include "debug.h"
#include "spi.h"
#include "tube-lib.h"
#include "tube-isr.h"

static int debug = 0;

void setTubeLibDebug(int d) {
  debug = d;
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
  rxBuf[0] = 0x0F;
  rxBuf[1] = 0x0F;
  if (!in_isr) {
    _disable_interrupts();
  }
  spi_transfer(txBuf, rxBuf, sizeof(txBuf));
  if (!in_isr) {
    _enable_interrupts();
  }
  if (debug >= 3) {
    printf("%02x%02x%02x%02x\r\n", txBuf[0], txBuf[1], rxBuf[0], rxBuf[1]);
  }
  return rxBuf[1];
}

// Reg is 1..4
void sendByte(unsigned char reg, unsigned char byte) {
  unsigned char addr = (reg - 1) * 2;
  if (debug >= 2) {
	printf("waiting for space in R%d\r\n", reg);
  }
  while ((tubeRead(addr) & F_BIT) == 0x00);
  if (debug >= 2) {
	printf("done waiting for space in R%d\r\n", reg);
  }
  tubeWrite((reg - 1) * 2 + 1, byte);
  if (debug >= 1) {
	printf("Tx: R%d = %02x\r\n", reg, byte);
  }
}

// Reg is 1..4
unsigned char receiveByte(unsigned char reg) {
  unsigned char byte;
  unsigned char addr = (reg - 1) * 2;
  if (debug >= 2) {
	printf("waiting for data in R%d\r\n", reg);
  }
  while ((tubeRead(addr) & A_BIT) == 0x00);
  if (debug >= 2) {
	printf("done waiting for data in R%d\r\n", reg);
  }
  byte = tubeRead((reg - 1) * 2 + 1);
  if (debug >= 1) {
	printf("Rx: R%d = %02x\r\n", reg, byte);
  }
  return byte;
}

// Reg is 1..4
void sendString(unsigned char reg, unsigned char terminator, const volatile char *buf) {
  char c;
  do {
	c = *buf++;
    sendByte(reg, (unsigned char)c);
  } while (c != terminator);
}

// Reg is 1..4
int receiveString(unsigned char reg, unsigned char terminator, volatile char *buf) {
  int i = 0;
  unsigned char c;
  do {
    c = receiveByte(reg);
	buf[i++] = (char) c;
  } while (c != terminator);
  return i;
}

// Reg is 1..4
void sendBlock(unsigned char reg, int len, const unsigned char *buf) {
  // bytes in a block are transferred high downto low
  buf += len;
  while (len-- > 0) {
    sendByte(reg, (*--buf));
  }
}

// Reg is 1..4
void receiveBlock(unsigned char reg, int len, unsigned char *buf) {
  // bytes in a block are transferred high downto low
  // bytes in a block are transferred high downto low
  buf += len;
  while (len-- > 0) {
	*--buf = receiveByte(reg);
  } 
}

// Reg is 1..4
void sendWord(unsigned char reg, unsigned int word) {
  sendByte(reg, (unsigned char)(word >> 24));
  word <<= 8;
  sendByte(reg, (unsigned char)(word >> 24));
  word <<= 8;
  sendByte(reg, (unsigned char)(word >> 24));
  word <<= 8;
  sendByte(reg, (unsigned char)(word >> 24));
}

// Reg is 1..4
unsigned int receiveWord(unsigned char reg) {
  int word = receiveByte(reg);
  word <<= 8;
  word |= receiveByte(reg);
  word <<= 8;
  word |= receiveByte(reg);
  word <<= 8;
  word |= receiveByte(reg);
  return word;
}

void enable_MMU_and_IDCaches (void)
{
  static volatile __attribute__ ((aligned (0x4000))) unsigned PageTable[4096];

  unsigned base;
  for (base = 0; base < 512; base++)
    {
      // outer and inner write back, write allocate, shareable
      PageTable[base] = base << 20 | 0x1140E;
    }
  for (; base < 4096; base++)
    {
      // shared device, never execute
      PageTable[base] = base << 20 | 0x10416;
    }

  // restrict cache size to 16K (no page coloring)
  unsigned auxctrl;
  asm volatile ("mrc p15, 0, %0, c1, c0,  1" : "=r" (auxctrl));
  auxctrl |= 1 << 6;
  asm volatile ("mcr p15, 0, %0, c1, c0,  1" :: "r" (auxctrl));

  // set domain 0 to client
  asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (1));

  // always use TTBR0
  asm volatile ("mcr p15, 0, %0, c2, c0, 2" :: "r" (0));

  // set TTBR0 (page table walk inner cacheable, outer non-cacheable, shareable memory)
  asm volatile ("mcr p15, 0, %0, c2, c0, 0" :: "r" (3 | (unsigned) &PageTable));

  // invalidate data cache and flush prefetch buffer
  asm volatile ("mcr p15, 0, %0, c7, c5,  4" :: "r" (0) : "memory");
  asm volatile ("mcr p15, 0, %0, c7, c6,  0" :: "r" (0) : "memory");

  // enable MMU, L1 cache and instruction cache, L2 cache, write buffer,
  //   branch prediction and extended page table on
  unsigned mode;
  asm volatile ("mrc p15,0,%0,c1,c0,0" : "=r" (mode));
  // Bit 12 enables the L1 instruction cache
  // Bit 11 enables branch pre-fetching
  // Bit  2 enables the L1 data cache
  // Bit  0 enabled the MMU  
  // The L1 instruction cache can be used independently of the MMU
  // The L1 data cache will one be enabled if the MMU is enabled
  mode |= 0x00001805;
  asm volatile ("mcr p15,0,%0,c1,c0,0" :: "r" (mode) : "memory");
}
