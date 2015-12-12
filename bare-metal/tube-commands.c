#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tube-swi.h"
#include "tube-lib.h"
#include "tube-commands.h"
#include "darm/darm.h"
unsigned int memAddr;

const char *help = "ARM Tube Client 0.10\r\n";

char line[256];

/***********************************************************
 * Build in Commands
 ***********************************************************/

#define NUM_CMDS 6

// Must be kept in step with cmdFuncs (just below)
char *cmdStrings[NUM_CMDS] = {
  "HELP",
  "GO",
  "MEM",
  "DIS",
  "FILL",
  "CRC"
};

int (*cmdFuncs[NUM_CMDS])(char *params) = {
  doCmdHelp,
  doCmdGo,
  doCmdMem,
  doCmdDis,
  doCmdFill,
  doCmdCrc
};

int dispatchCmd(char *cmd) {
  int i;
  int c;
  int r;
  char *cmdPtr;
  char *refPtr;
  //  skip any leading space
  while (isblank((int) *cmd)) {
	cmd++;
  }
  // Match the command 
  for (i = 0; i < NUM_CMDS; i++) {
	cmdPtr = cmd;
	refPtr = cmdStrings[i];
	do {
	  c = tolower((int)*cmdPtr);	  
	  r = tolower((int)*refPtr);
	  if (r == 0 || c == '.') {
		// skip any trailing space becore the params
		while (isblank((int)*cmdPtr)) {
		  cmdPtr++;
		}
		return (*cmdFuncs[i])(cmdPtr);
	  }
      cmdPtr++;
      refPtr++;
	} while (c != 0 && c == r);
  }
  // non-zero means pass the command onto the CLI processor
  return 1;
}

int doCmdHelp(char *params) {
  int i;
  sendString(R1, 0x00, help);
  sendByte(R1, 0x00);
  if (strcasecmp(params, "ARM") == 0) {
	for (i = 0; i < NUM_CMDS; i++) {
	  sendString(R1, 0x00, "  ");
	  sendString(R1, 0x00, cmdStrings[i]);
	  sendString(R1, 0x00, "\r\n");
	  sendByte(R1, 0x00);
	}
	return 0;
  }
  // pass the command on to the CLI
  return 1;
}

int doCmdGo(char *params) {
  unsigned int address;
  sscanf(params, "%x", &address);
  // No return from here
  user_exec((unsigned char *)address);
  return 0;
}

int doCmdFill(char *params) {
  unsigned int i;
  unsigned int start;
  unsigned int end;
  unsigned int data;
  sscanf(params, "%x %x %x", &start, &end, &data);
  for (i = start; i <= end; i++) {
	*((unsigned char *)i) = data;
  }
  return 0;
}

int doCmdMem(char *params) {
  int i, j;
  unsigned char c;
  char *ptr;
  sscanf(params, "%x", &memAddr);
  for (i = 0; i < 0x100; i+= 16) {
	ptr = line;
	// Generate the address
	ptr += sprintf(ptr, "%08X ", (memAddr + i));	
	  // Generate the hex values
	for (j = 0; j < 16; j++) {
	  c = *((unsigned char *)(memAddr + i + j));
	  ptr += sprintf(ptr, "%02X ", c);
	}
	// Generate the ascii values
	for (j = 0; j < 16; j++) {
	  c = *((unsigned char *)(memAddr + i + j));
      if (c < 32 || c > 126) {
		c = '.';
      }
	  ptr += sprintf(ptr, "%c", c);
	}
	ptr += sprintf(ptr, "\r\n");
	sendString(R1, 0x00, line);
	sendByte(R1, 0x00);
  }
  memAddr += 0x100;
  return 0;
}

int doCmdDis(char *params) {
  darm_t d;
  darm_str_t str;
  int i;
  unsigned int opcode;
  unsigned int regs[2];
  sscanf(params, "%x", &memAddr);
  memAddr &= ~3;
  do {
    for (i = 0; i < 16; i++) {
      opcode = *(unsigned int *)memAddr;
      sprintf(line, "%08X %08X ***\r\n", memAddr, opcode);     
      if(darm_armv7_disasm(&d, opcode) == 0 && darm_str2(&d, &str, 0) == 0) {
        sprintf(line + 18, "%s\r\n", str.total);
      }
      sendString(R1, 0x00, line);
      sendByte(R1, 0x00);
      memAddr += 4;
    }
    tube_ReadC(&regs[1]);
  } while ((regs[0] & CARRY_MASK) == 0);
  return 0;
}

int doCmdCrc(char *params) {
  unsigned int i;
  unsigned int j;
  unsigned int start;
  unsigned int end;
  unsigned int data;
  unsigned long crc = 0;
  sscanf(params, "%x %x %x", &start, &end, &data);
  for (i = start; i <= end; i++) {
	data = *((unsigned char *)i);
    for (j = 0; j < 8; j++) {
      crc = crc << 1;
      crc = crc | (data & 1);
      data >>= 1;
      if (crc & 0x10000)
		crc = (crc ^ CRC_POLY) & 0xFFFF;
    }
  }
  sprintf(line, "%04X\r\n", (unsigned short)crc);
  sendString(R1, 0x00, line);
  sendByte(R1, 0x00);
  return 0;
}
