// Pandora.c
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "32016.h"
#include "mem32016.h"

void tubeWrite(unsigned char Address, unsigned char Data)
{
	printf("tubeWrite(%02X, %02X)\n", Address, Data);
}

unsigned char tubeRead(unsigned char Address)
{
	printf("tubeRead(%02X)\n", Address);

	return 0;
}

int main(int argc, char* argv[])
{
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

