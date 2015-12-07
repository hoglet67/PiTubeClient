// tube-isr.h

#ifndef TUBE_ISR_H
#define TUBE_ISR_H

/* Connects to: Header pin 16 == BCM GPIO23 */
#define IRQ_PIN_MASK (1 << 23)

/* Connects to: Header pin 18 == BCM GPIO24 */
#define NMI_PIN_MASK (1 << 24)

/* Connects to: Header pin 22 == BCM GPIO25 */
#define RST_PIN_MASK (1 << 25)

extern volatile int in_isr;

extern volatile unsigned char *address;

void TubeInterrupt(void);

#endif
