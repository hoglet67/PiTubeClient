#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "32016.h"
#include "mem32016.h"
#include "defs.h"
//#include "Trap.h"
#include "NDis.h"
#include "Profile.h"

#ifdef PROFILING
uint32_t Frequencies[InstructionCount];

void ProfileInit(void)
{
   memset(Frequencies, 0, sizeof(Frequencies));
}

void ProfileAdd(uint32_t Function)
{
   if (Function < InstructionCount)
   {
      Frequencies[Function]++;
   }
}

void ProfileDump(void)
{
   uint32_t Function;

   printf("\"Function\", \"Name\", \"Frequecy\"\n");

   for (Function = 0; Function < InstructionCount; Function++)
   {
      if (strcmp(InstuctionText[Function], "TRAP"))
      {
         // Seperate Prints to make them wasy to comment out
         printf("%02" PRIX32 ", ", Function);
         printf("%s, ", InstuctionText[Function]);
         printf("%09" PRIu32 "\n", Frequencies[Function]);
      }
   }
}
#endif