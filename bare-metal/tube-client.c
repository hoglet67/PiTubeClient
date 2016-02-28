#include <stdio.h>
#include "copro-arm.h"
#include "copro-arm2.h"
#include "copro-6502.h"
#include "copro-lib6502.h"
#include "copro-65tube.h"
#include "copro-80186.h"
#include "../EconetClock/EconetClock.h"

#define COPRO_ARM     0
#define COPRO_ARM2    1
#define COPRO_6502    2
#define COPRO_LIB6502 3
#define COPRO_65TUBE  4
#define COPRO_80186   5
#define COPRO_32016   6
#define COPRO_ECO_CLK 7

//#define COPRO COPRO_80186
#define COPRO COPRO_32016

void kernel_main(unsigned int r0, unsigned int r1, unsigned int atags)
{
   if (COPRO == COPRO_ARM)
   {
      copro_arm_main(r0, r1, atags);
   }
   else if (COPRO == COPRO_ARM2)
   {
      copro_arm2_main(r0, r1, atags);
   }
   else if (COPRO == COPRO_6502)
   {
      copro_6502_main(r0, r1, atags);
   }
   else if (COPRO == COPRO_LIB6502)
   {
      copro_lib6502_main(r0, r1, atags);
   }
   else if (COPRO == COPRO_65TUBE)
   {
      copro_65tube_main(r0, r1, atags);
   }
   else if (COPRO == COPRO_80186)
   {
      copro_80186_main(r0, r1, atags);
   }
   else if (COPRO == COPRO_32016)
   {
      // Not yet!
   }
   else if (COPRO == COPRO_ECO_CLK)
   {
      EconetClock_Init();
      EconetClock_Start(100, 50);
   }
   else
   {
      printf("Co Pro %d has not been defined yet\r\n", COPRO);
   }

   printf("Halted....\r\n");
   while (1)
   {
   }
}
