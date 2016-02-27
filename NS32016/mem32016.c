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
#include "mem32016.h"

#ifdef TEST_SUITE
#include "boot_rom.h"
#else
#include "PandoraV2_00.h"
#endif

uint8_t ns32016ram[MEG16 + 8];				// Extra 8 bytes as we can read that far off the end of RAM

void init_ram(void)
{
#ifdef TEST_SUITE
   memset(ns32016ram, 0, sizeof(ns32016ram));
   memcpy(ns32016ram, boot_rom, sizeof(boot_rom));
#elif defined(PANDORA_BASE)
   memcpy(&ns32016ram[0xF00000], PandoraV2_00, sizeof(PandoraV2_00));
#else
   uint32_t Address;

   for (Address = 0; Address < MEG16; Address += sizeof(PandoraV2_00))
   {
      memcpy(&ns32016ram[Address], PandoraV2_00, sizeof(PandoraV2_00));
   }
#endif
}

void dump_ram(void)
{
   FILE *f = fopen("32016.dmp", "wb");
   if (f)
   {
      fwrite(ns32016ram, RAM_SIZE, 1, f);
      fclose(f);
   }
}

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

uint32_t read_n(uint32_t addr, uint32_t Size)
{
   if (Size < sz32)
   {
      if (addr + Size < IO_BASE)
      {
         uint32_t Result = 0;
         memcpy(&Result, &ns32016ram[addr], (Size + 1));
         return Result;
      }
   }

   printf("Bad Read @ %06X\n", addr);
   return 0;
}

void write_x8(uint32_t addr, uint8_t val)
{
   //addr &= MEM_MASK;

   if (addr <= (RAM_SIZE - sizeof(uint8_t)))
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
#ifdef LITTLE_ENDIAN
   if (addr <= (RAM_SIZE - sizeof(uint16_t)))
   {
      *((uint16_t*) (&ns32016ram[addr])) = val;
      return;
   }
#endif

   write_x8(addr++, val & 0xFF);
   write_x8(addr, val >> 8);
}

void write_x32(uint32_t addr, uint32_t val)
{
#ifdef LITTLE_ENDIAN
   if (addr <= (RAM_SIZE - sizeof(uint32_t)))
   {
      *((uint32_t*) (&ns32016ram[addr])) = val;
      return;
   }
#endif

   write_x8(addr++, val);
   write_x8(addr++, (val >> 8));
   write_x8(addr++, (val >> 16));
   write_x8(addr, (val >> 24));
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

