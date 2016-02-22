// Pandora.c
#ifndef __linux
#include "stdafx.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"
#include "boot_rom.h"
#include "PandoraV2_00.h"

void tubeWrite(unsigned char Address, unsigned char Data)
{
	printf("tubeWrite(%02X, %02X)\n", Address, Data);
}

unsigned char tubeRead(unsigned char Address)
{
	printf("tubeRead(%02X)\n", Address);

	return 0;
}

void init(void)
{
#if 1
	memset(ns32016ram, 0, sizeof(ns32016ram));
	memcpy(ns32016ram, boot_rom, sizeof(boot_rom));
#else
	uint32_t Address;

	for (Address = 0; Address < MEG16; Address += sizeof(PandoraV2_00))
	{
		memcpy(&ns32016ram[Address], PandoraV2_00, sizeof(PandoraV2_00));
	}
#endif
}

#ifdef __linux
int main(int argc, char* argv[])
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
	init();
	n32016_reset();

	while (1)
	{
		n32016_exec(1);
	}

	return 0;
}

