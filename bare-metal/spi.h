// spi.h

#ifndef SPI_H
#define SPI_H

typedef unsigned char u8;
typedef unsigned char u32;
typedef volatile unsigned int v32;
#define REG32(in) *((v32*) in)

// GPIO PINS
#define gpioGPFSEL0		REG32(0x20200000)
#define gpioGPFSEL1		REG32(0x20200004)
#define gpioGPSET0 		REG32(0x2020001C)
#define gpioGPCLR0		REG32(0x20200028)
#define gpioGPLEV0		REG32(0x20200034)


// SPI0 ADDRESS
#define SPI0_CONTROL 	REG32(0x20204000)
#define SPI0_FIFO    	REG32(0x20204004)
#define SPI0_CLK     	REG32(0x20204008)
#define SPI0_DLEN    	REG32(0x2020400C)
#define SPI0_LTOH    	REG32(0x20204010)
#define SPI0_DC      	REG32(0x20204014)

// Bitfields in spi_C
#define SPI_C_LEN_LONG	25
#define SPI_C_DMA_LEN	24
#define SPI_C_CSPOL2		23
#define SPI_C_CSPOL1		22
#define SPI_C_CSPOL0		21
#define SPI_C_RX			20
#define SPI_C_RXR			19
#define SPI_C_TXD			18
#define SPI_C_RXD			17
#define SPI_C_DONE		16
#define SPI_C_LEN			13
#define SPI_C_REN			12
#define SPI_C_ADCS		11
#define SPI_C_INTR		10
#define SPI_C_INTD		 9
#define SPI_C_DMAEN		 8
#define SPI_C_TA		 	 7
#define SPI_C_CSPOL		 6
#define SPI_C_CLEAR_RX	 5
#define SPI_C_CLEAR_TX	 4
#define SPI_C_CPOL		 3
#define SPI_C_CPHA		 2
#define SPI_C_CS1			 1
#define SPI_C_CS0			 0

extern void wait(unsigned int delay);
extern void spi_begin(void);
extern void spi_transfer(u8* TxBuff, u8* RxBuff, u32 Length);
extern void spi_end(void);
#endif
