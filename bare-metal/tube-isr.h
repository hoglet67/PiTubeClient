// tube-isr.h

#ifndef TUBE_ISR_H
#define TUBE_ISR_H

#include "rpi-gpio.h"

/* Connects to: Header pin 26 == BCM GPIO7 */
#define RST_PIN      (RPI_GPIO7)

/* Connects to: Header pin 11 == BCM GPIO17 */
#define IRQ_PIN      (RPI_GPIO17)

/* Connects to: Header pin 12 == BCM GPIO18 */
#define NMI_PIN      (RPI_GPIO18)

#define RST_PIN_MASK (1 << RST_PIN)
#define IRQ_PIN_MASK (1 << IRQ_PIN)
#define NMI_PIN_MASK (1 << NMI_PIN)

extern volatile int in_isr;

extern volatile unsigned char *address;

void TubeInterrupt(void);

#endif
