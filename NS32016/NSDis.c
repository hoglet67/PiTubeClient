#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"
#include "mem32016.h"

uint32_t OpCount = 0;
uint8_t Regs[2];
uint8_t FunctionLookup[256];

void StoreRegisters(uint8_t Index, uint8_t Value)
{
   Regs[Index] = Value;
}

void ClearRegs(void)
{
   Regs[0] = 
   Regs[1] = 0xFF;
}

const char* PostfixLookup(uint8_t Postfix)
{
   switch (Postfix)
	{
		case sz8:	            return "B";
		case sz16:	            return "W";
      case Translating:       return "T";
		case sz32:	            return "D";
		default:                break;
	}

	return "";
}

const char InstuctionText[InstructionCount][8] =
{
   // FORMAT 0
	"BEQ", "BNE", "TRAP", "TRAP", "BH", "BLS", "BGT", "BLE",
   "BFS", "BFC", "BLO", "BHS", "BLT", "BGE", "BR", "TRAP", 

   // FORMAT 1	
   "BSR", "RET", "CXP", "RXP", "RETT", "RETI", "SAVE", "RESTORE",
   "ENTER", "EXIT", "NOP", "WAIT", "DIA", "FLAG", "SVC", "BPT",

   // FORMAT 2
	"ADDQ", "CMPQ", "SPR", "Scond", "ACB", "MOVQ", "LPR", "TRAP",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 3
   "CXPD", "TRAP", "BICPSR", "TRAP", "JUMP", "TRAP", "BISPSR", "TRAP",
   "TRAP", "TRAP", "ADJSP", "TRAP", "JSR", "TRAP", "CASE", "TRAP",

   // FORMAT 4
	"ADD", "CMP", "BIC", "TRAP", "ADDC", "MOV", "OR",  "TRAP",
   "SUB", "ADDR","AND", "TRAP", "SUBC", "TBIT", "XOR","TRAP",

   // FORMAT 5
	"MOVS", "CMPS", "SETCFG", "SKPS", "TRAP", "TRAP", "TRAP", "TRAP",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 6
   "ROT", "ASH", "CBIT", "CBITI", "TRAP", "LSH", "SBIT", "SBITI",
   "NEG", "NOT", "TRAP", "SUBP", "ABS", "COM", "IBIT", "ADDP",
	
   // FORMAT 7
   "MOVM", "CMPM", "INSS", "EXTS", "MOVXBW", "MOVZBW", "MOVZiD", "MOVXiD",
   "MUL", "MEI", "Trap", "DEI", "QUO", "REM", "MOD", "DIV", "TRAP"
  
   // FORMAT 8
   "EXT", "CVTP", "INS", "CHECK", "INDEX", "FFS", "MOVUS", "MOVSU",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 9
   "MOVif","LFSR","MOVLF","MOVFL","ROUND","TRUNC","SFSR", "FLOOR",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 10
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 11
   "ADDf", "MOVf", "CMPf", "TRAP", "SUBf", "NEGf", "TRAP", "TRAP",
   "DIVf", "TRAP", "TRAP", "TRAP", "MULf", "ABSf", "TRAP", "TRAP",

   // FORMAT 12
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 13
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 14
   "RDVAL", "WRVAL", "LMR", "SMR", "TRAP", "TRAP", "TRAP", "TRAP",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // Illegal
   "TRAP"
};

const char* InstuctionLookup(uint8_t Function)
{
	if (Function < InstructionCount)
	{

		return InstuctionText[Function];
	}

	return "Bad NS32016 opcode";
}

void RegLookUp(void)
{
   uint32_t Index;

   for (Index = 0; Index < 2; Index++)
   {
      if (Regs[Index] < 32)
      {
         if (Regs[Index] < 8)
         {
            if ((Index == 1) && (Regs[0] < 8))        // Hack for now
            {
               PiTRACE(",");
            }

            PiTRACE("R%u", Regs[Index]);
         }
         else if (Regs[Index] < 16)
         {

         }
         else if (Regs[Index] == 0x17)
         {
            PiTRACE("TOS");
         }
#if 0
         else
         {
            if (gentype[Index] == Memory)
            {
               uint32_t  Address = genaddr[Index];
               PiTRACE(" &%06"PRIX32"=", Address);
               switch (LookUp.p.Size)
               {
                  case sz8:
                  {
                     PiTRACE("%02"PRIX8, read_x8(Address));
                  }
                  break;

                  case sz16:
                  {
                     PiTRACE("%04"PRIX16, read_x16(Address));
                  }
                  break;

                  case sz32:
                  {
                     PiTRACE("%08"PRIX32, read_x32(Address));
                  }
                  break; 
               }
            }
         }
#endif
      }
   }
}

void ShowInstruction(uint32_t pc, uint32_t opcode, DecodeMatrix* pInstruction, uint32_t Disp)
{
	if (pc < MEG16)
	{
      const char* pText = "Bad NS32016 opcode";
      uint32_t Postfix = pInstruction->p.Size;

      if (pInstruction->p.Function < InstructionCount)
      {
         pText = InstuctionText[pInstruction->p.Function];

         if ((opcode & 0x80FF) == 0x800E)
         {
            Postfix = Translating;
         }
      }

      //PiTRACE("#%08"PRIu32" ", ++OpCount);
      PiTRACE("PC: % 06"PRIX32" ", pc);
      PiTRACE("INST: % 08"PRIX32" ", opcode);
      PiTRACE("%s%s", pText, PostfixLookup(Postfix));

      RegLookUp();

      if (pInstruction->p.Function <= BSR)
      {
         uint32_t Address = pc + Disp;
         PiTRACE(" &%06"PRIX32" ", Address);
      }

      PiTRACE("\n");

#ifndef TEST_SUITE
      if (OpCount >= 10000)
      {
         n32016_dumpregs("Lots of trace data here!");
      }
#endif

		return;
	}

	PiTRACE("PC is :%08"PRIX32" ?????\n", pc);
}

#ifdef SHOW_REG_WRITES
void ShowRegisterWrite(uint32_t Index, uint32_t Value)
{
   if (Regs[Index] < 8)
   {
      PiTRACE(" R%u = %08"PRIX32"\n", Regs[Index], Value);
   }
}
#endif

const uint8_t FormatSizes[FormatCount + 1] =
{
   1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0
};

#define FUNC(FORMAT, OFFSET) (((FORMAT) << 4) + (OFFSET)) 

uint8_t GetFunction(uint8_t FirstByte)
{
   switch (FirstByte & 0x0F)
   {
      case 0x0A:     return FUNC(Format0, FirstByte >> 4);
      case 0x02:     return FUNC(Format1, FirstByte >> 4);
   
      case 0x0C:
      case 0x0D:
      case 0x0F:
      {
         if ((FirstByte & 0x70) != 0x70)
         {
            return FUNC(Format2, (FirstByte >> 4) & 0x07);
         }
         
         return FUNC(Format3, 0);
      }
      // No break due to return
   }

   if ((FirstByte & 0x03) != 0x02)
   {
      return FUNC(Format4, (FirstByte >> 2) & 0x0F);
   }

   switch (FirstByte)
   {
      case 0x0E:     return FUNC(Format5, 0);          // String instruction
      case 0x4E:     return FUNC(Format6, 0);
      case 0xCE:     return FUNC(Format7, 0);
      CASE4(0x2E):   return FUNC(Format8, 0);
      case 0x3E:     return FUNC(Format9, 0);
      case 0x7E:     return FUNC(Format10, 0);
      case 0xBE:     return FUNC(Format11, 0);
      case 0xFE:     return FUNC(Format12, 0);
      case 0x9E:     return FUNC(Format13, 0);
      case 0x1E:     return FUNC(Format14, 0);
   }

   return FormatBad;
}

void n32016_build_matrix()
{
   uint32_t Index;

   memset(FunctionLookup, FormatBad, sizeof(FunctionLookup));

   for (Index = 0; Index < 256; Index++)
   {
      FunctionLookup[Index] = GetFunction(Index);
   }
}
