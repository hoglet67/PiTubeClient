/*
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define BITSPERWORD 8

#define CMD_WRITE 0x80
#define CMD_READ  0xC0

#define R1 1
#define R2 2
#define R3 3
#define R4 4

#define R1_STATUS 0
#define R1_DATA   1
#define R2_STATUS 2
#define R2_DATA   3
#define R3_STATUS 4
#define R3_DATA   5
#define R4_STATUS 6
#define R4_DATA   7

#define A_BIT 0x80
#define F_BIT 0x40

#define DEBUG 1
#define DEBUGDETAIL 1

// BCM GPIO 25
#define IRQ_PIN 6

const char *banner = "Raspberry Pi ARMv6 Co Processor 900MHz\n\n\r";

int spiClose(int spifd);
void tubeWrite(unsigned char addr, unsigned char byte);
unsigned char tubeRead(unsigned char addr);
unsigned char tubeCmd(unsigned char cmd, unsigned char addr, unsigned char byte);
void waitUntilSpace(unsigned char reg);
void waitUntilData(unsigned char reg);
void sendByte(unsigned char reg, unsigned char byte);
void sendString(unsigned char reg, const unsigned char *msg);
unsigned char receiveByte(unsigned char reg);
void receiveString(unsigned char reg, unsigned char terminator, unsigned char *buf);
unsigned char pollForResponse(unsigned char reg);

static volatile unsigned char escFlag;
static volatile unsigned char errNum;
static volatile unsigned char errMsg[256];

void IRQInterrupt(void) {
  // Check for R1 interrupt
  if (tubeRead(R1_STATUS) & A_BIT) {
    unsigned char flag = tubeRead(R1_DATA);
    if (flag & 0x80) {
      // Update escape flag
      escFlag = flag & 0x40;
      if (DEBUG) {
        printf("Escape flag = %02x\n", escFlag);
      }
    } else {
      // Event
      unsigned char y = receiveByte(R1);
      unsigned char x = receiveByte(R1);
      unsigned char a = receiveByte(R1);
      if (DEBUG) {
        printf("Event = %02x %02x %02x\n", a, x, y);
      }
    }    
  }
  // Check for R4 interrupt
  if (tubeRead(R4_STATUS) & A_BIT) {
    unsigned char type = tubeRead(R4_DATA);
    if (type == 0xff) {
      // Error
      receiveByte(R2); // 0
      errNum = receiveByte(R2);
      receiveString(R2, 0x00, errMsg);
      if (DEBUG) {
        printf("Error = %02x %s\n", errNum, errMsg);
      }
    } else {
      unsigned char id = receiveByte(R4);
      if (DEBUG) {
        printf("Transfer = %02x %02x\n", type, id);
      }      
      if (type < 4 ) {
	// Type 0 .. 3 transfers services by NMI
	unsigned char a0 = receiveByte(R4);
	unsigned char a1 = receiveByte(R4);
	unsigned char a2 = receiveByte(R4);
	unsigned char a3 = receiveByte(R4);
	// TODO setup NMI
	unsigned char sync = receiveByte(R4);

      } else if (type == 4) {
	// Type 4 : pass address to host
	unsigned char a0 = receiveByte(R4);
	unsigned char a1 = receiveByte(R4);
	unsigned char a2 = receiveByte(R4);
	unsigned char a3 = receiveByte(R4);
	unsigned char sync = receiveByte(R4);
	
      } else if (type == 5) {
	// Type 5 : tube release

      } else if (type == 6) {
	// Type 6 transfers are 256 byte block p -> h
	unsigned char a0 = receiveByte(R4);
	unsigned char a1 = receiveByte(R4);
	unsigned char a2 = receiveByte(R4);
	unsigned char a3 = receiveByte(R4);

	// TODO write 256 bytes to R3 

	unsigned char sync = receiveByte(R4);

      } else if (type == 7) {
	// Type 7 transfers are 256 byte block h -> p
	unsigned char a0 = receiveByte(R4);
	unsigned char a1 = receiveByte(R4);
	unsigned char a2 = receiveByte(R4);
	unsigned char a3 = receiveByte(R4);

	// TODO read 256 bytes from R3 

	unsigned char sync = receiveByte(R4);

      }
      

    }

  }
  
}

int main(int argc, char **argv) {
  unsigned char resp;
  unsigned int i;
  unsigned char buffer[256];


  int speed = 16000000;
  if (argc > 1) {
    speed = atoi(argv[1]);
  }
  
  printf("Speed = %d\n", speed);

  if (wiringPiSetup() < 0) {
    perror("wiringPiSetup failed");
    exit(1);
  }

  int spifd =  wiringPiSPISetup (0, speed) ;

  if (spifd < 0) {
    perror("wiringPiSPISetup failed");
    exit(1);
  }

  // Hook in the interrupt handler
  if (wiringPiISR(IRQ_PIN, INT_EDGE_FALLING, &IRQInterrupt) < 0) {
    perror("wiringPiISR failed");
    exit(1);
  }
  
  // Send the reset message
  sendString(R1, banner);
  sendByte(R1, 0x00);

  // Wait for the reponse in R2
  receiveByte(R2);

  while (1) {
    // Print a prompt
    sendString(R1, "arm>*");

    // Ask for user input (OSWORD 0)
    sendByte(R2, 0x0A);
    sendByte(R2, 0x7F); // max ascii value
    sendByte(R2, 0x20); // min ascii value
    sendByte(R2, 0x7F); // max line length
    sendByte(R2, 0x07); // Buffer MSB (not used)
    sendByte(R2, 0x00); // Buffer LSB (not used)

    // Read response
    resp = receiveByte(R2);

    // Was it escape
    if (resp >= 0x80) {
      sendString(R1, "\n\rEscape\n\r");

      // Acknowledge escape condition
      if (escFlag) {
	if (DEBUG) {
	  printf("Acknowledging Escape\n");
	}
	sendByte(R2, 0x04);
	sendByte(R2, 0x00);
	sendByte(R2, 0x7E);
	resp = receiveByte(R2);
      }

      continue;
    }

    receiveString(R2, '\r', buffer);

    // Check for *QUIT
    if (strcasecmp(buffer, "quit") == 0) {
      // Yes, we're done
      sendString(R1, "Goodbye....\n\n\r");
      break;
    }

    // Send string back as OSCLI
    sendByte(R2, 0x02);
    sendString(R2, buffer);
    sendByte(R2, 0x0D);

    // Wait for the reponse in R2
    receiveByte(R2);
  }

  spiClose(spifd);

  return 0;
}


int spiClose(int spifd) {
    int statusVal = -1;
    statusVal = close(spifd);
    if(statusVal < 0) {
      perror("Could not close SPI device");
      exit(1);
    }
    return statusVal;
}

unsigned char tubeRead(unsigned char addr) {
  return tubeCmd(CMD_READ, addr, 0xff);
}   

void tubeWrite(unsigned char addr, unsigned char byte) {
  tubeCmd(CMD_WRITE, addr, byte);
}   

unsigned char tubeCmd(unsigned char cmd, unsigned char addr, unsigned char byte) {
  unsigned char data[2];
  unsigned char cmd0 = cmd | ((addr & 7) << 3);
  unsigned char cmd1 = byte;
  data[0] = cmd0;
  data[1] = cmd1;
  int ret = wiringPiSPIDataRW (0, data, 2);
  if (ret < 0) {
    perror("wiringPiSPIDataRW failed");
    exit(1);
  }
  if (DEBUGDETAIL) {
    // printf("%02x %02x -> %02x %02x\n", cmd0, cmd1, data[0], data[1]);
  }
  return data[1];
}


void waitUntilSpace(unsigned char reg) {
  unsigned char addr = (reg - 1) * 2;
  if (DEBUGDETAIL) {
    printf("waiting for space in R%d\n", reg);
  }
  while ((tubeRead(addr) & F_BIT) == 0x00);
  if (DEBUGDETAIL) {
    printf("done waiting for space in R%d\n", reg);
  }
}

void waitUntilData(unsigned char reg) {
  unsigned char addr = (reg - 1) * 2;
  if (DEBUGDETAIL) {
    printf("waiting for data in R%d\n", reg);
  }
  while ((tubeRead(addr) & A_BIT) == 0x00);
  if (DEBUGDETAIL) {
    printf("done waiting for data in R%d\n", reg);
  }
}

// Reg is 1..4
void sendByte(unsigned char reg, unsigned char byte) {
  waitUntilSpace(reg);
  tubeWrite((reg - 1) * 2 + 1, byte);
  if (DEBUGDETAIL) {
    printf("Tx: R%d = %02x\n", reg, byte);
  }
}

// Reg is 1..4
void sendString(unsigned char reg, const unsigned char *msg) {
  int i;
  for (i = 0; i < strlen(msg); i++) {
    sendByte(reg, msg[i]);
  }
}

// Reg is 1..4
unsigned char receiveByte(unsigned char reg) {
  unsigned char byte;
  waitUntilData(reg);
  byte = tubeRead((reg - 1) * 2 + 1);
  if (DEBUGDETAIL) {
    printf("Rx: R%d = %02x\n", reg, byte);
  }
  return byte;
}

// Reg is 1..4
void receiveString(unsigned char reg, unsigned char terminator, unsigned char *buf) {
  int i = 0;
  unsigned char c;
  do {
    c = receiveByte(reg);
    if (c != terminator) {
      buf[i++] = c;
    }
  } while (c != terminator  && i < 100);
  buf[i++] = 0;
}
