// B-em v2.2 by Tom Walker
 //32016 parasite processor emulation (not working yet)

// 32106 CoProcessor Memory Subsystem
// By Simon R. Ellwood

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "32016.h"
#include "../bare-metal/tube-lib.h"
//#include "PandoraV0_61.h"
#include "mem32016.h"

uint8_t ns32016ram[MEG16 + 8];				// Extra 8 bytes as we can read that far off the end of RAM

// Tube Access
// FFFFF0 - R1 status
// FFFFF2 - R1 data
// FFFFF4 - R2 status
// FFFFF6 - R2 data
// FFFFF8 - R3 status
// FFFFFA - R3 data
// FFFFFC - R4 status
// FFFFFE - R4 data

uint8_t read_x8(uint32_t addr)
{
  //addr &= MEM_MASK;

  if (addr < IO_BASE)
  {
    return ns32016ram[addr];
  }

  if ((addr & 0x01) == 0)
  {
    return tubeRead(addr >> 1);
  }

  printf("Bad Read @ %06X\n", addr);
  n32016_dumpregs();

  return 0;
}

uint16_t read_x16(uint32_t addr)
{
  //addr &= MEM_MASK;
  
#ifdef LITTLE_ENDIAN
  if (addr < IO_BASE)
  {
		return *((uint16_t*) &ns32016ram[addr]);
  }
#endif

  return read_x8(addr) | (read_x8(addr + 1) << 8);
}

uint32_t read_x32(uint32_t addr)
{
  //addr &= MEM_MASK;
  
#ifdef LITTLE_ENDIAN
  if (addr < IO_BASE)
  {
		return *((uint32_t*) &ns32016ram[addr]);
  }
#endif

  return read_x8(addr) | (read_x8(addr + 1) << 8) | (read_x8(addr + 2) << 16) | (read_x8(addr + 3) << 24);
}

void write_x8(uint32_t addr, uint8_t val)
{
  //addr &= MEM_MASK;

  if (addr < RAM_SIZE)
  {
    ns32016ram[addr] = val;
    return;
  }

  if ((addr >= IO_BASE) && ((addr & 0x01) == 0))
  {
    tubeWrite(addr >> 1, val);
    return;
  }

#ifdef PANDORA_ROM_PAGE_OUT
  if (addr == 0xF90000)
  {
    memset(ns32016ram, 0, MEG4);
    return;
  }
#endif  
  
  printf("Bad Write @%06X %02X\n", addr, val);
  n32016_dumpregs();
}

void write_x16(uint32_t addr, uint16_t val)
{
  //addr &= MEM_MASK;

#ifdef LITTLE_ENDIAN
  if (addr < RAM_SIZE)
  {
		*((uint16_t*) (&ns32016ram[addr])) = val;
		return;
  }
#endif

	write_x8(addr++, val & 0xFF);
	write_x8(addr  , val >> 8);	
}

void write_x32(uint32_t addr, uint32_t val)
{
  //addr &= MEM_MASK;

#ifdef LITTLE_ENDIAN
  if (addr < RAM_SIZE)
  {
     *((uint32_t*) (&ns32016ram[addr])) = val;
		return;
  }
#endif

	write_x8(addr++,	val			 );
	write_x8(addr++,	(val >> 8	));	
	write_x8(addr++,	(val >> 16	));
	write_x8(addr, 	(val >> 24	));
}

void write_Arbitary(uint32_t addr, uint8_t* pData, uint32_t Size)
{
   //addr &= MEM_MASK;

#ifdef LITTLE_ENDIAN
   if ((addr + Size) <= RAM_SIZE)
   {
      memcpy(&ns32016ram[addr], pData, Size);
      return;
   }
#endif

   while (Size--)
   {
      write_x8(addr++, *pData++);
   }
}
