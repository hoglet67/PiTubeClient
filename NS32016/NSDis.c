#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"

const char* SizeLookup(uint8_t Size)
{
	switch (Size)
	{
		case sz8:	return "B x08";
		case sz16:	return "W x16";
		case sz32:	return "D x32";
		default:	break;
	}

	return "";
}

const char InstuctionText[InstructionCount][16] =
{
	"BEQ", "BNE", "BH", "BLS", "BGT", "BLE", "BFS", "BFC", "BLO", "BHS", "BLT", "BGE", "BR",																          // Format 0
	"BSR", "RET", "CXP", "RXP", "RETT", "RETI", "SAVE", "RESTORE", "ENTER", "EXIT", "NOP", "WAIT", "DIA", "FLAG", "SVC", "BPT",				// Format 1
	"ADDQ", "CMPQ", "SPR", "Scond", "ACB", "MOVQ", "LPR",																												                      // Format 2
	"TYPE3", "TYPE3MKII",																																						                                  // Stop Gap
	"CXPD", "BICPSR", "JUMP", "BISPSR", "ADJSP", "JSR", "CASE",																										                    // Format 3
	"ADD", "CMP", "BIC", "ADDC", "MOV", "OR", "SUB", "ADDR", "AND", "SUBC", "TBIT", "XOR",																	          // Format 4
	"MOVS", "CMPS", "SETCFG", "SKPS",                                                                                                 // Format 5
	"TYPE6",																																											                                    // Format 6
	"MOVM", "CMPM", "INSS", "EXTS", "MOVXBW", "MOVZBW", "MOVZiD", "MOVXiD", "MUL", "MEI", "Trap", "DEI", "QUO", "REM", "MOD", "DIV",	// Format 7
	"TYPE8",																																											                                      // Format 8
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

void ShowInstruction(uint32_t pc, uint8_t Function, uint8_t Size)
{
	if (pc < MEG16)
	{
		uint8_t opcode = ns32016ram[pc];
		uint8_t opcode2 = ns32016ram[pc + 1];
		printf("PC:%06X INST:%02X [%02X] %s%s\n", pc, opcode, opcode2, InstuctionLookup(Function), SizeLookup(Size));
		return;
	}

	printf("PC is :%08X ?????\n", pc);
}