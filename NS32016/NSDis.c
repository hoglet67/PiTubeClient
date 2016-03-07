#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "32016.h"
#include "mem32016.h"
#include "Profile.h"
#include "Trap.h"

uint32_t OpCount = 0;
uint8_t FunctionLookup[256];

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
	"BEQ", "BNE", "BCS", "BCC", "BH", "BLS", "BGT", "BLE",
   "BFS", "BFC", "BLO", "BHS", "BLT", "BGE", "BR", "BN", 

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
   "TRAP", "CINV",  "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

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
      if (Regs[Index] < 0xFFFF)
      {
         if ((Index == 1) && (Regs[0] < 0xFFFF))        // Hack for now
         {
            PiTRACE(",");
         }

         if (Regs[Index] < 8)
         {
            PiTRACE("R%u", Regs[Index]);
         }
         else if (Regs[Index] < 16)
         {
            PiTRACE("(R%u)", (Regs[Index] & 7));
         }
         else
         {
            switch (Regs[Index] & 0x1F)
            {
               case FrameRelative:
               {
                  PiTRACE("FrameR");
               }
               break;
               
               case StackRelative:
               {
                  PiTRACE("StackR");
               }
               break;

               case StaticRelative:
               {
                  PiTRACE("StaticR");
               }
               break;

               case IllegalOperand:
               {
                  PiTRACE("IllegalOperand");
               }
               break; 
 
               case Immediate:
               {
                  //PiTRACE("Immediate");
                  PiTRACE("x'%"PRIX32, genaddr[Index]);
               }
               break;

               case Absolute:
               {
                  PiTRACE("Absolute");
               }
               break;

               case External:
               {
                  PiTRACE("External");
               }
               break;
 
               case TopOfStack:
               {
                  PiTRACE("TOS");
               }
               break;

               case FpRelative:
               {
                  PiTRACE("FpRelative");
               }
               break;

               case SpRelative:
               {
                  PiTRACE("SpRelative");
               }
               break;

               case SbRelative:
               {
                  PiTRACE("SbRelative");
               }
               break;

               case PcRelative:
               {
                  PiTRACE("PcRelative");
               }
               break;

               case EaPlusRn:
               {
                  PiTRACE("EaPlusRn");
               }
               break;

               case EaPlus2Rn:
               {
                  PiTRACE("EaPlus2Rn");
               }
               break;

               case EaPlus4Rn:
               {
                  PiTRACE("EaPlus4Rn");
               }
               break;

               case EaPlus8Rn:
               {
                  PiTRACE("EaPlus8Rn");
               }
               break;
            }
         }

#if 0
         else
         {
            if (gentype[Index] == Memory)
            {
               uint32_t  Address = genaddr[Index];
               PiTRACE(" &%06"PRIX32"=", Address);
               switch (OpSize.Op[0])
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

void BreakPoint(uint32_t pc, uint32_t opcode)
{
#if 1
#ifndef TEST_SUITE
   // Exec address of Bas32
   if (pc == 0x000200)
   {
      printf("Entering Bas32\n");
      ProfileInit();
   }
   // Exec address of Panos
   if (pc == 0x000400)
   {
      printf("Entering Panos\n");
      ProfileInit();
   }
   // Address of SVC &11 (OS_EXIT)
   if (pc == 0xF007BB)
   {
      n32016_dumpregs("Retuning to Pandora");
      ProfileInit();
   }
#endif

// Useful way to be able to get a breakpoint on a particular instruction
//if (startpc == 0)
#if 0     
   if (startpc == 0xF001E9)
   {
      printf("Here!\n");
      //n32016_dumpregs("Oops how did I get here!");
   }


   if ((opcode == 0) && (startpc < 20))
   {
      n32016_dumpregs("Oops how did I get here!");
      //Trace = 1;
   }
#endif
#endif
}

#ifdef SHOW_INSTRUCTIONS
void ShowInstruction(uint32_t pc, uint32_t opcode, uint32_t Function, uint32_t OperandSize, uint32_t Disp)
{
   static uint32_t old_pc = 0xFFFFFFFF;

	if (pc < MEG16)
	{
      if (pc != old_pc)
      {
         old_pc = pc;
         const char* pText = "Bad NS32016 opcode";
         uint32_t Postfix = OperandSize;
         uint32_t Format = Function >> 4;

         //PiTRACE("#%08"PRIu32" ", ++OpCount);
         PiTRACE("&%06"   PRIX32 " ", pc);
         PiTRACE("[%08" PRIX32 "] ", opcode);
         PiTRACE("F%01" PRIu32 " ", Format);

         if (Function < InstructionCount)
         {
            pText = InstuctionText[Function];

            if ((opcode & 0x80FF) == 0x800E)
            {
               Postfix = Translating;
            }
         }

         if (Function == Scond)
         {
            uint32_t Condition = ((opcode >> 7) & 0x0F);
            PiTRACE("S%s%s ", &InstuctionText[Condition][1], PostfixLookup(Postfix));            // Offset by 1 to loose the 'B'
         }
         else
         {
            PiTRACE("%s%s ", pText, PostfixLookup(Postfix));
         }

         switch (Function)
         {
            case ADDQ:
            case CMPQ:
            case MOVQ:
            {
               int32_t Value = (opcode >> 7) & 0xF;
               NIBBLE_EXTEND(Value);
               PiTRACE("%" PRId32 ", ", Value);
            }
            break;

            case LPR:
            {
               int32_t Value = (opcode >> 7) & 0xF;
               if (Value == 9)
               {
                  PiTRACE("SP,", Value);
               }
               else
               {
                  PiTRACE("%" PRId32 ", ", Value);
               }
            }
            break;
         }

         RegLookUp();

         if ((Function <= BN) || (Function == BSR))
         {
            uint32_t Address = pc + Disp;
            PiTRACE("&%06"PRIX32" ", Address);
         }

   #if 0
         switch (Function)
         {
            case ADDQ:
            case CMPQ:
            case MOVQ:
            {
               int32_t Value = (opcode >> 7) & 0xF;
               NIBBLE_EXTEND(Value);
               PiTRACE(",%" PRId32, Value);
            }
            break;
         }
   #endif

         PiTRACE("\n");

   #ifdef TEST_SUITE
         if (pc == 0x1CA9 || pc == 0x1CB2)
         {
            n32016_dumpregs("Test Suite Complete!\n");
            exit(1);
         }
   #endif

   #ifndef TEST_SUITE
         if (OpCount >= 10000)
         {
            n32016_dumpregs("Lots of trace data here!");
         }
   #endif
      }

		return;
	}

	PiTRACE("PC is :%08"PRIX32" ?????\n", pc);
}
#endif

void ShowRegisterWrite(uint32_t Index, uint32_t Value)
{
   if (Regs[Index] < 32)
   {
#ifdef SHOW_REG_WRITES
      if (Regs[Index] < 8)
      {
         PiTRACE(" R%u = %08"PRIX32"\n", Regs[Index], Value);
      }
#endif

#ifdef TEST_SUITE
      if (Regs[Index] == 7)
      {
         PiTRACE("*** TEST = %u\n", Value);

         if (Value == 137)
         {
            PiTRACE("*** BREAKPOINT\n");
         }
      }
#endif
   }
}

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
