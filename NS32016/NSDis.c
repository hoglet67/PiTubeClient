#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"

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

const char* SizeLookup(uint8_t Size)
{
	switch (Size)
	{
#if 1
		case sz8:	return "B";
		case sz16:	return "W";
		case sz32:	return "D";
#else
      case sz8:	return "B x08";
      case sz16:	return "W x16";
      case sz32:	return "D x32";
#endif

		default:	break;
	}

	return "";
}

const char InstuctionText[InstructionCount][16] =
{
	"BEQ", "BNE", "BH", "BLS", "BGT", "BLE", "BFS", "BFC", "BLO", "BHS", "BLT", "BGE", "BR",																          // Format 0
	"BSR", "RET", "CXP", "RXP", "RETT", "RETI", "SAVE", "RESTORE", "ENTER", "EXIT", "NOP", "WAIT", "DIA", "FLAG", "SVC", "BPT",				// Format 1
	"ADDQ", "CMPQ", "SPR", "Scond", "ACB", "MOVQ", "LPR",																												                      // Format 2
	"CXPD", "BICPSR", "JUMP", "BISPSR", "TRAP", "ADJSP", "JSR", "CASE",																										            // Format 3
	"ADD", "CMP", "BIC", "ADDC", "MOV", "OR", "SUB", "ADDR", "AND", "SUBC", "TBIT", "XOR",																	          // Format 4
	"MOVS", "CMPS", "SETCFG", "SKPS",                                                                                                 // Format 5  
  "ROT", "ASH", "CBIT", "CBITI", "TRAP", "LSH", "SBIT", "SBITI", "NEG", "NOT", "TRAP", "SUBP", "ABS", "COM", "IBIT", "ADDP",        // Format 6  
	"MOVM", "CMPM", "INSS", "EXTS", "MOVXBW", "MOVZBW", "MOVZiD", "MOVXiD", "MUL", "MEI", "Trap", "DEI", "QUO", "REM", "MOD", "DIV",	// Format 7
  "EXT", "CVTP", "INS", "CHECK", "INDEX", "FFS", "MOVUS", "MOVSU",				                                                          // Format 8
  "TRAP",
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
      if (Regs[Index] < 8)
      {
         printf(" R%u", Regs[Index]);
      }
   }
}

void ShowInstruction(uint32_t pc, uint32_t opcode, uint8_t Function, uint8_t Size)
{
	if (pc < MEG16)
	{
      printf("#%08X PC:%06X INST:%08X %s%s", ++OpCount, pc, opcode, InstuctionLookup(Function), SizeLookup(Size));
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