// Pandora.c
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "32016.h"
#include "mem32016.h"

#ifdef TRACE_TO_FILE
FILE *pTraceFile = NULL;
#endif

void tubeWrite(unsigned char Address, unsigned char Data)
{
   unsigned char temp = (Data < ' ') ? ' ' : Data;
   PiTRACE("tubeWrite(%02X, %02X) %c\n", Address, Data, temp);
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
   init_ram();

#ifdef PANDORA_BASE
   n32016_reset(PANDORA_BASE);
#else
   n32016_reset(0);
#endif

	while (1)
	{
		n32016_exec(1);
	}

	return 0;
}

