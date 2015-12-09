// spi.c

#include "spi.h"
#define BIT(in) (1 << (in)) 

void spi_pin_init(void)
{
  unsigned int var;

  var = gpioGPFSEL1;                                                                    // Read current value of GPFSEL1- pin 10 & 11

  var &=~((7 << 0) | (7 << 3));                                                     // Set as input pins GPIO = 000
  var |= ((4 << 0) | (4 << 3));                                                     // Set to alt function ALT0, GPIO = 000

  gpioGPFSEL1 = var;                                                                    // Write back updated value
    
  var = gpioGPFSEL0;                                                                    // Read current value of GPFSEL1- pin 7,8 & 9

  var &=~((7 << 21) | (7 << 24) | (7 << 27));                                   // Set as input pins GPIO = 000
  var |= ((4 << 21) | (4 << 24) | (4 << 27));                                   // Set to alt function ALT0, GPIO = 000
  gpioGPFSEL0 =var;                                                                     // Write back updated value
}

void spi_begin(void)
{   
  spi_pin_init();
        
  SPI0_CONTROL = 0;
  SPI0_CONTROL |= BIT(SPI_C_CLEAR_RX) | BIT(SPI_C_CLEAR_TX);                 // Clear TX and RX fifos
    
  SPI0_CONTROL &= ~(BIT(SPI_C_CPOL) | BIT(SPI_C_CPHA));                      // Set data mode = 0
  SPI0_CONTROL &= ~(BIT(SPI_C_CS0) | BIT(SPI_C_CS1));                        // Set to CS0 and Chip Select Polarity=LOW
  SPI0_CONTROL &= ~BIT(SPI_C_CSPOL0);

  SPI0_CLK = 16;                                                             // Set clock to ???
}

void spi_transfer(u8* TxBuff, u8* RxBuff, u32 Length)
{
  u32 remaining;
  SPI0_CONTROL |= BIT(SPI_C_CLEAR_RX) | BIT(SPI_C_CLEAR_TX) | BIT(SPI_C_TA); // Clear TX and RX FIFOs and enable SPI
  remaining = Length;
  while (remaining--)
    {
      while ((SPI0_CONTROL & BIT(SPI_C_TXD)) == 0)                           // Wait for space in the FIFO
        {
          continue;
        }

      SPI0_FIFO = *(TxBuff++);                                               // Write to TX FIFO
    }
  remaining = Length;
  while (remaining--)
    {
      while ((SPI0_CONTROL & BIT(SPI_C_RXD)) == 0)                           // Wait for data in the FIFO
        {
          continue;
        }
	  *(RxBuff++) = SPI0_FIFO;                                               // Read RX FIFO as we go
    }
  SPI0_CONTROL &= ~BIT(SPI_C_TA);                                            // Set TA = 0
}

void spi_end(void)                                                           // Set all the SPI0 pins back to input
{
  gpioGPFSEL1 &=~((7 << 0) | (7 << 3));                                      // Set as input pins GPIO = 000
  gpioGPFSEL0 &=~((7 << 21) | (7 << 24) | (7 << 27));                        // Set as input pins GPIO = 000                                                             // Write back updated value
}
