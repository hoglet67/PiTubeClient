#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"
#include "mem32016.h"

uint32_t OpCount = 0;
uint8_t Regs[2];

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
            printf(" R%u", Regs[Index]);
         }
         else if (Regs[Index] < 16)
         {

         }
         else
         {
            if (gentype[Index] == 0)
            {
               uint32_t  Address = genaddr[Index];
               printf(" &%06X=", Address);
               switch (LookUp.p.Size)
               {
                  case sz8:
                  {
                     printf("%02X", read_x8(Address));
                  }
                  break;

                  case sz16:
                  {
                     printf("%04X", read_x16(Address));
                  }
                  break;

                  case sz32:
                  {
                     printf("%08X", read_x32(Address));
                  }
                  break; 
               }
            }
         }
      }
   }
}

void ShowInstruction(uint32_t pc, uint32_t opcode, uint8_t Function, uint8_t Postfix)
{
	if (pc < MEG16)
	{
      const char* pText = "Bad NS32016 opcode";
      if (Function < InstructionCount)
      {
         pText = InstuctionText[Function];

         if ((opcode & 0x80FF) == 0x800E)
         {
            Postfix = Translating;
         }
      }

      printf("#%08u PC:%06X INST:%08X %s%s", ++OpCount, pc, opcode, pText, PostfixLookup(Postfix));
      RegLookUp();
      printf("\n");

#if 0
      if ((OpCount & 0xF) == 0)
      {
         printf(".\n");
      }
#endif

		return;
	}


	printf("PC is :%08X ?????\n", pc);
}

const uint8_t FormatSizes[FormatCount] =
{
   1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};

#define FUNC(FORMAT, OFFSET) \
   mat[Index].p.Function   = (((FORMAT) << 4) + (OFFSET)); \
   mat[Index].p.BaseSize   = FormatSizes[FORMAT]

void n32016_build_matrix()
{
   uint32_t Index;

   memset(mat, 0xFF, sizeof(mat));

   for (Index = 0; Index < 256; Index++)
   {
      switch (Index & 0x0F)
      {
         case 0x0A:
         {
            FUNC(Format0, Index >> 4);
            continue;
         }
         // No break due to continue

         case 0x02:
         {
            FUNC(Format1, Index >> 4);
            continue;
         }
         // No break due to continue

         case 0x0C:
         case 0x0D:
         case 0x0F:
         {
            if ((Index & 0x70) != 0x70)
            {
               FUNC(Format2, (Index >> 4) & 0x07);
            }
            else
            {
               FUNC(Format3, 0);
            }

            mat[Index].p.Size = Index & 3;
            continue;
         }
         // No break due to continue
      }

      if ((Index & 0x03) != 0x02)
      {
         FUNC(Format4, (Index >> 2) & 0x0F);
         mat[Index].p.Size = Index & 3;
         continue;
      }

      switch (Index)
      {
         case 0x0E:
         {
            FUNC(Format5, 0);          // String instruction
         }
         break;

         case 0x4E:
         {
            FUNC(Format6, 0);
         }
         break;

         case 0xCE:
         {
            FUNC(Format7, 0);
         }
         break;

         CASE4(0x2E):
         {
            FUNC(Format8, 0);
         }
         break;

         case 0x3E:
         {
            FUNC(Format9, 0);
         }
         break;

         case 0x7E:
         {
            FUNC(Format10, 0);
         }
         break;

         case 0xBE:
         {
            FUNC(Format11, 0);
         }
         break;

         case 0xFE:
         {
            FUNC(Format12, 0);
         }
         break;

         case 0x9E:
         {
            FUNC(Format13, 0);
         }
         break;

         case 0x1E:
         {
            FUNC(Format14, 0);
         }
         break;
      }
   }
}
