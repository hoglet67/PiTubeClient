// spi.c

#include "spi.h"
#define BIT(in) (1 << (in))

// Select whether to use SPIO_CE0_N OR SPIO_CE1_N

#define CE_NUMBER 0 // use SPIO_CE0_N
// #define CE_NUMBER 1 // use SPIO_CE1_N

// SPI0_SCK     GPIO11 FSEL1 bits 5..3
// SPI0_MOSI    GPIO10 FSEL1 bits 2..0
// SPI0_MISO    GPIO9  FSEL1 bits 29..27
// SPI0_CE0_N   CPIO8  FSEL1 bits 26..24
// SPI0_CE1_N   GPIO7  FSEL1 bits 23..21

int get_ce_offset() {
  if (CE_NUMBER == 0) {
    return 24;  // SPIO_CE0_N
  } else {
    return 21;  // SPIO_CE1_N
  }
}

void spi_pin_init(void)
{
  unsigned int var;
  int ce_offset = get_ce_offset();

  var = gpioGPFSEL1;                                                         // Read current value of GPFSEL1- GPIO 10 & 11

  var &=~((7 << 0) | (7 << 3));                                              // Set as input pins GPIO = 000
  var |= ((4 << 0) | (4 << 3));                                              // Set to alt function ALT0, GPIO = 000

  gpioGPFSEL1 = var;                                                         // Write back updated value

  var = gpioGPFSEL0;                                                         // Read current value of GPFSEL1- GPIO 7,8 & 9

  var &=~((7 << ce_offset) | (7 << 27));                                     // Set as input pins GPIO = 000
  var |= ((4 << ce_offset) | (4 << 27));                                     // Set to alt function ALT0, GPIO = 000

  gpioGPFSEL0 =var;                                                          // Write back updated value
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

// New Expirimental Faster Send, It just sends bytes without reading the Recieve FIFO as we will discard the data anyway
void spi_send(u8* TxBuff, u32 Length)
{
    SPI0_CONTROL |= BIT(SPI_C_CLEAR_RX) | BIT(SPI_C_CLEAR_TX) | BIT(SPI_C_TA);// Clear TX and RX FIFOs and enable SPI

    while (Length--)
    {
        while ((SPI0_CONTROL & BIT(SPI_C_TXD)) == 0)                          // Wait for space in the FIFO
        {
            continue;
        }
        SPI0_FIFO = *(TxBuff++);
    }

    while ((SPI0_CONTROL & BIT(SPI_C_DONE)) == 0)                             // While not done
    {
        continue;
    }

    SPI0_CONTROL |= BIT(SPI_C_CLEAR_RX) | BIT(SPI_C_CLEAR_TX);                // Clear RX FIFO as we have not read the bytes
    SPI0_CONTROL &= ~BIT(SPI_C_TA);                                           // Set TA = 0
}

void spi_end(void)                                                            // Set all the SPI0 pins back to input
{
  int ce_offset = get_ce_offset();
  gpioGPFSEL1 &=~((7 << 0) | (7 << 3));                                       // Set as input pins GPIO = 000
  gpioGPFSEL0 &=~((ce_offset) | (7 << 27));                                   // Set as input pins GPIO = 000
}
