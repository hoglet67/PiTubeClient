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
  while ((c = *buf++) != terminator) {
    sendByte(reg, (unsigned char)c);
  }
}

// Reg is 1..4
int receiveString(unsigned char reg, unsigned char terminator, volatile char *buf) {
  int i = 0;
  unsigned char c;
  do {
    c = receiveByte(reg);
    if (c != terminator) {
      buf[i++] = (char) c;
    }
  } while (c != terminator  && i < 100);
  buf[i] = 0;
  return i;
}

// Reg is 1..4
void sendBlock(unsigned char reg, int len, const unsigned char *buf) {
  int i;
  for (i = 0; i < len; i++) {
    sendByte(reg, (*buf++));
  }
}

// Reg is 1..4
void receiveBlock(unsigned char reg, int len, unsigned char *buf) {
  int i;
  for (i = 0; i < len; i++) {
	*buf++ = receiveByte(reg);
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
