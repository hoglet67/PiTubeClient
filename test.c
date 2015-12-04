#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define BITSPERWORD 8

#define CMD_WRITE 0x00
#define CMD_READ  0x80

#define R1 1
#define R2 2
#define R3 3
#define R4 4

unsigned int speed;

const char *banner = "Raspberry Pi ARMv6 Co Processor 900MHz\n\n\r";

int spiOpen(unsigned char *devspi, unsigned char mode, unsigned char bitsPerWord);
int spiClose(int spifd);
int spiWriteRead(int spifd, unsigned char bitsPerWord, unsigned char *data, int length);
unsigned char tubeCmd(int spifd, unsigned char cmd0, unsigned char cmd1);
void waitUntilSpace(int spifd, unsigned char reg);
void waitUntilData(int spifd, unsigned char reg);
void sendByte(int spifd, unsigned char reg, unsigned char byte);
void sendString(int spifd, unsigned char reg, const unsigned char *msg);
unsigned char receiveByte(int spifd, unsigned char reg);
void receiveString(int spifd, unsigned char reg, unsigned char terminator, unsigned char *buf);
unsigned char pollForResponse(int spifd, unsigned char reg);


int main(int argc, char **argv) {
  unsigned char resp;
  unsigned int i;
  unsigned char buffer[256];


  speed = atoi(argv[1]);
  printf("Speed = %d\n", speed);
  
  int spifd = spiOpen("/dev/spidev0.0", SPI_MODE_0, BITSPERWORD);

  // Send the reset message
  sendString(spifd, R1, banner);
  sendByte(spifd, R1, 0x00);

  // Wait for the reponse in R2
  pollForResponse(spifd, R2);

  while (1) {
    // Print a prompt
    sendString(spifd, R1, "arm>*");
  
    // Ask for user input (OSWORD 0)
    sendByte(spifd, R2, 0x0A);
    sendByte(spifd, R2, 0x7F); // max ascii value
    sendByte(spifd, R2, 0x20); // min ascii value
    sendByte(spifd, R2, 0x7F); // max line length
    sendByte(spifd, R2, 0x07); // Buffer MSB (not used)
    sendByte(spifd, R2, 0x00); // Buffer LSB (not used)

    // Read response
    resp = receiveByte(spifd, R2);

    // Was it escape
    if (resp >= 0x80) {
      printf("Escape\n");
      sendString(spifd, R1, "\n\rEscape\n\r");

      // Read Escape Event
      resp = receiveByte(spifd, R1);
      printf("Escape event %02x\n", resp);

      // Acknowledge escape condition
      sendByte(spifd, R2, 0x04);
      sendByte(spifd, R2, 0x00);
      sendByte(spifd, R2, 0x7E);
      resp = receiveByte(spifd, R2);
      printf("Acknowledge escape response %02x\n", resp);

      // Read Escape Event
      resp = receiveByte(spifd, R1);
      printf("Escape event %02x\n", resp);
      
      continue;
    }

    receiveString(spifd, R2, '\r', buffer);
      
    // Check for *QUIT
    if (strcasecmp(buffer, "quit") == 0) {
      // Yes, we're done
      sendString(spifd, R1, "Goodbye....\n\n\r");
      break;
    }
    
    // Send string back as OSCLI
    sendByte(spifd, R2, 0x02);
    sendString(spifd, R2, buffer);
    sendByte(spifd, R2, 0x0D);

    // Wait for the reponse in R2
    pollForResponse(spifd, R2);
  }
  
  spiClose(spifd);
  
  return 0;
}




int spiOpen(unsigned char *devspi, unsigned char mode, unsigned char bitsPerWord) {
    int statusVal = -1;
    int spifd = open(devspi, O_RDWR);
    if(spifd < 0){
        perror("could not open SPI device");
        exit(1);
    }
 
    statusVal = ioctl (spifd, SPI_IOC_WR_MODE, &(mode));
    if(statusVal < 0){
        perror("Could not set SPIMode (WR)...ioctl fail");
        exit(1);
    }
 
    statusVal = ioctl (spifd, SPI_IOC_RD_MODE, &(mode));
    if(statusVal < 0) {
      perror("Could not set SPIMode (RD)...ioctl fail");
      exit(1);
    }
 
    statusVal = ioctl (spifd, SPI_IOC_WR_BITS_PER_WORD, &(bitsPerWord));
    if(statusVal < 0) {
      perror("Could not set SPI bitsPerWord (WR)...ioctl fail");
      exit(1);
    }
 
    statusVal = ioctl (spifd, SPI_IOC_RD_BITS_PER_WORD, &(bitsPerWord));
    if(statusVal < 0) {
      perror("Could not set SPI bitsPerWord(RD)...ioctl fail");
      exit(1);
    }  
 
    statusVal = ioctl (spifd, SPI_IOC_WR_MAX_SPEED_HZ, &(speed));    
    if(statusVal < 0) {
      perror("Could not set SPI speed (WR)...ioctl fail");
      exit(1);
    }
 
    statusVal = ioctl (spifd, SPI_IOC_RD_MAX_SPEED_HZ, &(speed));    
    if(statusVal < 0) {
      perror("Could not set SPI speed (RD)...ioctl fail");
      exit(1);
    }
    return spifd;
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
 
int spiWriteRead(int spifd, unsigned char bitsPerWord, unsigned char *data, int length){
 
  struct spi_ioc_transfer spi[length];
  int i = 0;
  int retVal = -1; 
  bzero(spi, sizeof spi); // ioctl struct must be zeroed 
 
  // one spi transfer for each byte
 
  for (i = 0 ; i < length ; i++){
    spi[i].tx_buf        = (unsigned long)(data + i); // transmit from "data"
    spi[i].rx_buf        = (unsigned long)(data + i) ; // receive into "data"
    spi[i].len           = sizeof(*(data + i)) ;
    spi[i].delay_usecs   = 0 ;
    spi[i].speed_hz      = speed ;
    spi[i].bits_per_word = bitsPerWord ;
    spi[i].cs_change = 0;
  }
 
  retVal = ioctl (spifd, SPI_IOC_MESSAGE(length), &spi) ;
 
  if(retVal < 0){
    perror("Problem transmitting spi data..ioctl");
    exit(1);
  }
 
  return retVal;
 
}

unsigned char tubeCmd(int spifd, unsigned char cmd0, unsigned char cmd1) {
  unsigned char data[3];
  data[0] = cmd0;
  data[2] = 0xff;
  if (cmd0 & 128) {
    // reads
    data[1] = 0xff;
    spiWriteRead(spifd, BITSPERWORD, data, 3 );
    
  } else {
    // writes
    data[1] = cmd1;
    spiWriteRead(spifd, BITSPERWORD, data, 2 );
  }  
  // printf("%02x %02x -> %02x %02x %02x\n", cmd0, cmd1, data[0], data[1], data[2]);
  return data[2];
}


void waitUntilSpace(int spifd, unsigned char reg) {
  unsigned char addr = (reg - 1) * 2;
  printf("waiting for space in R%d\n", reg);
  while ((tubeCmd(spifd, CMD_READ + addr, 0xff) & 0x40) == 0x00);
  printf("done waiting for space in R%d\n", reg);
}

void waitUntilData(int spifd, unsigned char reg) {
  unsigned char addr = (reg - 1) * 2;
  printf("waiting for data in R%d\n", reg);
  while ((tubeCmd(spifd, CMD_READ + addr, 0xff) & 0x80) == 0x00);
  printf("done waiting for data in R%d\n", reg);
}

// Reg is 1..4
void sendByte(int spifd, unsigned char reg, unsigned char byte) {
  waitUntilSpace(spifd, reg);
  tubeCmd(spifd, CMD_WRITE + (reg - 1) * 2 + 1, byte);
  printf("Tx: R%d = %02x\n", reg, byte);
}

// Reg is 1..4
void sendString(int spifd, unsigned char reg, const unsigned char *msg) {
  int i;
  for (i = 0; i < strlen(msg); i++) {
    sendByte(spifd, reg, msg[i]);
  }
}

// Reg is 1..4
unsigned char receiveByte(int spifd, unsigned char reg) {
  unsigned char byte;
  waitUntilData(spifd, reg);
  byte = tubeCmd(spifd, CMD_READ + (reg - 1) * 2 + 1, 0xff);
  printf("Rx: R%d = %02x\n", reg, byte);
  return byte;
}

// Reg is 1..4
void receiveString(int spifd, unsigned char reg, unsigned char terminator, unsigned char *buf) {
  int i = 0;
  unsigned char c;
  do {
    c = receiveByte(spifd, reg);
    if (c != terminator) {
      buf[i++] = c;
    }
  } while (c != terminator  && i < 100);
  buf[i++] = 0;
}

unsigned char pollForResponse(int spifd, unsigned char reg) {
  unsigned char r;
  unsigned char status;
  unsigned char byte;
  printf("Waiting for response in R%d\n", reg);
  while (1) {
    for (r = 1; r <= 4; r++) {
      status = tubeCmd(spifd, CMD_READ + (r - 1) * 2, 0xff);
      if (status & 0x80) {
	byte = tubeCmd(spifd, CMD_READ + (r - 1) * 2 + 1 , 0xff);
	printf("REG %02x = %02x\n", r, byte);
	if (r == reg) {
	  return byte;
	}
      }
    }
  }
}


