#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "32016.h"
#include "mem32016.h"
#include "defs.h"
#include "Trap.h"

uint32_t TrapFlags;

const char TrapText[TrapCount][40] =
{
   "Break Point Hit",
   "Reserved Addressing Mode",
   "Unknown Format",
   "Unknown Instruction",
   "Divide By Zero",
   "Illegal Immediate",
   "Illegal DoubleIndexing",
   "Illegal SpecialReading",
   "Illegal SpecialWriting",
   "Illegal Writing Immediate"
};

void Dump(void)
{
   PiTRACE("R0=%08"PRIX32" R1=%08"PRIX32" R2=%08"PRIX32" R3=%08"PRIX32"\n", r[0], r[1], r[2], r[3]);
   PiTRACE("R4=%08"PRIX32" R5=%08"PRIX32" R6=%08"PRIX32" R7=%08"PRIX32"\n", r[4], r[5], r[6], r[7]);
   PiTRACE("PC=%08"PRIX32" SB=%08"PRIX32" SP=%08"PRIX32" TRAP=%08"PRIX32"\n", pc, sb, GET_SP(), TrapFlags);
   PiTRACE("FP=%08"PRIX32" INTBASE=%08"PRIX32" PSR=%04"PRIX16" MOD=%04"PRIX16"\n", fp, intbase, psr, mod);
   
   uint32_t Count, Pattern = BIT(0);
   for (Count = 0; Count < TrapCount; Count++, Pattern <<= 1)
   {
      if (TrapFlags & Pattern)
      {
         PiTRACE("%s\n", TrapText[Count]);
      }
   }

   PiTRACE("\n");
}

void n32016_dumpregs(char* pMessage)
{
   Dump();

#ifdef PC_SIMULATION
#ifdef TRACE_TO_FILE
   printf("\n");
#endif

#ifdef WIN32
   system("pause");
#endif
   CloseTrace();
   exit(1);
#endif
}
