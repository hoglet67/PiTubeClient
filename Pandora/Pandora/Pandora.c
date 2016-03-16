// Pandora.c
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "32016.h"
#include "mem32016.h"
#include "Profile.h"

#ifdef TRACE_TO_FILE
FILE* pTraceFile = NULL;
#endif

int tubecycles = 0;
int tube_irq = 0;

void tubeWrite(unsigned char Address, unsigned char Data)
{
   unsigned char temp = (Data < ' ') ? ' ' : Data;
   PiTRACE("tubeWrite(%02X, %02X) %c\n", Address, Data, temp);
   
#ifdef TRACE_TO_FILE
   putchar(temp);
#endif
}

unsigned char tubeRead(unsigned char Address)
{
	PiTRACE("tubeRead(%02X)\n", Address);

   return 0x40;
}

void OpenTrace(const char *pFileName)
{
#ifdef TRACE_TO_FILE
   pTraceFile = fopen(pFileName, "wb");
#endif
}

void CloseTrace(void)
{
#ifdef TRACE_TO_FILE
   if (pTraceFile)
   {
      fclose(pTraceFile);
      pTraceFile = 0;
   }
#endif
}

int main(int argc, char* argv[])
{
   OpenTrace("PandoraTrace.txt");
   ProfileInit();
   n32016_init();

#ifdef INSTRUCTION_PROFILING
   memset(IP, 0, sizeof(IP));
#endif

#ifdef PANDORA_BASE
   n32016_reset_addr(PANDORA_BASE);
#else
   n32016_reset_addr(0);
#endif

#if 0
   uint32_t Start = 0x400;
   uint32_t End = LoadBinary("C:/Panos.bin", Start);
   if (End)
   {
      Disassemble(Start, End);
   }
#else

	while (1)
	{
		tubecycles = 8;
		n32016_exec();
	}
#endif

	return 0;
}

