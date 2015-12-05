// tube-isr.h

#ifndef TUBE_ISR_H
#define TUBE_ISR_H

extern volatile int reset;
extern volatile int error;
extern volatile unsigned char escFlag;
extern volatile unsigned char errNum;
extern volatile char errMsg[256];

void TubeInterrupt(void);

#endif
