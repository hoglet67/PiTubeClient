#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"

const char* InstuctionLookup(uint8_t* pPC)
{
	switch (*pPC)
	{
	CASE2(0x0C):	return "ADDQ byte";
	CASE2(0x0D):	return "ADDQ word";
	CASE2(0x0F):	return "ADDQ dword";
	CASE2(0x1C):	return "CMPQ byte";
	CASE2(0x1F):	return "CMPQ dword";
	CASE2(0x2F):	return "SPR";
	CASE2(0x3C):	return "ScondB";
	CASE2(0x4C):	return "ACBB";
	CASE2(0x4F):	return "ACBD";
	CASE2(0x5C):	return "MOVQ byte";
	CASE2(0x5D):	return "MOVQ word";
	CASE2(0x5F):	return "MOVQ dword";
	CASE2(0x6C):	return "LPRB";
	CASE2(0x6D):	return "LPRW";
	CASE2(0x6F):	return "LPRD";

	CASE4(0x00):	return "ADD byte";
	CASE4(0x03):	return "ADD dword";
	CASE4(0x04):	return "CMP byte";
	CASE4(0x07):	return "CMP dword";
	CASE4(0x08):	return "BIC byte";
	CASE4(0x09):	return "BIC word";
	CASE4(0x0B):	return "BIC dword";
	CASE4(0x14):	return "MOV byte";
	CASE4(0x15):	return "MOV word";
	CASE4(0x17):	return "MOV dword";
	CASE4(0x18):	return "OR byte";
	CASE4(0x19):	return "OR word";
	CASE4(0x1B):	return "OR dword";
	CASE4(0x23):	return "SUB dword";
	CASE4(0x27):	return "ADDR dword";
	CASE4(0x28):	return "AND byte";
	CASE4(0x29):	return "AND word";
	CASE4(0x2B):	return "AND dword";
	CASE4(0x2E):	return "Type 8";
	CASE4(0x34):	return "TBITB";
	CASE4(0x37):	return "TBITD";
	CASE4(0x38):	return "XOR byte";
	CASE4(0x39):	return "XOR word";
	CASE4(0x3B):	return "XOR dword";

	case 0x02:		return "BSR";
	case 0x0A:		return "BEQ";
	case 0x0E:		return "String instruction";
	case 0x12:		return "RET";
	case 0x1A:		return "BNE";
	case 0x22:		return "CXP";
	case 0x32:		return "RXP";
	case 0x42:		return "RETT";
	case 0x4A:		return "BH";
	case 0x4E:		return "Type 6";
	case 0x5A:		return "BLS";
	case 0x6A:		return "BGT";
	case 0x7A:		return "BLE";
	case 0x7C:		return "Type 3 byte";
	case 0x7D:		return "Type 3 word";
	case 0x7F:		return "Type 3 dword";
	case 0xCE:		return "Format 7";
	case 0x62:		return "SAVE";
	case 0x72:		return "RESTORE";
	case 0x82:		return "ENTER";
	case 0x8A:		return "BFS";
	case 0x92:		return "EXIT";
	case 0x9A:		return "BFC";
	case 0xE2:		return "SVC";
	case 0xAA:		return "BLO";
	case 0xBA:		return "BHS";
	case 0xCA:		return "BLT";
	case 0xDA:		return "BGE";
	case 0xEA:		return "BR";

	default: break;
	}

	return "Bad NS32016 opcode";
}

void ShowInstruction(uint32_t startpc)
{
	uint8_t opcode = ns32016ram[startpc];

	printf("PC:%06X INST:%02X %s\n", startpc, opcode, InstuctionLookup(&ns32016ram[startpc]));
}