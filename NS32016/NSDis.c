#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"

const char* SizeLookup(uint8_t* pPC)
{
	uint8_t OpCode = *pPC;

	switch (mat[OpCode].p.Size)
	{
		case sz8:	return " Byte";
		case sz16:	return " Word";
		case sz32:	return " Dword";
		default:	break;
	}

	return "";
}

const char* InstuctionLookup(uint8_t* pPC)
{
	uint8_t OpCode = *pPC;
	
	switch (mat[OpCode].p.Function)
	{
		case ADDQ:	return "ADDQ";
		case CMPQ:	return "CMPQ";
		case SPR:	return "SPR";
		case Scond:	return "Scond";
		case ACB:	return "ACB";
		case MOVQ:	return "MOVQ";
		case LPR:	return "LPR";
		case ADD:	return "ADD";
		case CMP:	return "CMP";
		case BIC:	return "BIC";
		case MOV:	return "MOV";
		case OR:		return "OR";
		case SUB:	return "SUB";
		case ADDR:	return "ADDR";
		case AND:	return "AND";
		case TYPE8:	return "Type 8";
		case TBIT:	return "TBIT";
		case XOR:	return "XOR";
		case BSR:	return "BSR";
		case BEQ:	return "BEQ";
		case StrI:	return "String instruction";
		case RET:	return "RET";
	case BNE:		return "BNE";
	case CXP:		return "CXP";
	case RXP:		return "RXP";
	case RETT:		return "RETT";
	case BH:			return "BH";
	case TYPE6:		return "Type 6";
	case BLS:		return "BLS";
	case BGT:		return "BGT";
	case BLE:		return "BLE";
	case TYPE3:		return "Type 3";
	case FORMAT7:	return "Format 7";
	case SAVE:		return "SAVE";
	case RESTORE:	return "RESTORE";
	case ENTER:		return "ENTER";
	case BFS:		return "BFS";
	case EXIT:		return "EXIT";
	case BFC:		return "BFC";
	case SVC:		return "SVC";
	case BLO:		return "BLO";
	case BHS:		return "BHS";
	case BLT:		return "BLT";
	case BGE:		return "BGE";
	case BR:			return "BR";
	default:			break;
	}

	return "Bad NS32016 opcode";
}

void ShowInstruction(uint32_t startpc)
{
	uint8_t opcode = ns32016ram[startpc];
	uint8_t opcode2 = ns32016ram[startpc + 1];

	printf("PC:%06X INST:%02X [%02X] %s%s\n", startpc, opcode, opcode2, InstuctionLookup(&ns32016ram[startpc]), SizeLookup(&ns32016ram[startpc]));
}