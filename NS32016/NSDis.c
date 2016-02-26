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
   // FORMAT 0
	"BEQ", "BNE", "BH", "BLS", "BGT", "BLE", "BFS", "BFC",
   "BLO", "BHS", "BLT", "BGE", "BR", "TRAP", "TRAP", "TRAP",

   // FORMAT 1	
   "BSR", "RET", "CXP", "RXP", "RETT", "RETI", "SAVE", "RESTORE",
   "ENTER", "EXIT", "NOP", "WAIT", "DIA", "FLAG", "SVC", "BPT",

   // FORMAT 2
	"ADDQ", "CMPQ", "SPR", "Scond", "ACB", "MOVQ", "LPR", "TRAP",
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 3
	"CXPD", "BICPSR", "JUMP", "BISPSR", "TRAP", "ADJSP", "JSR", "CASE",																										            // Format 3
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 4
	"ADD", "CMP", "BIC", "ADDC", "MOV", "OR", "SUB", "ADDR",
   "AND", "SUBC", "TBIT", "XOR",	"TRAP", "TRAP", "TRAP", "TRAP",

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

void ShowInstruction(uint32_t pc, uint32_t opcode, uint8_t Function, uint8_t Size)
{
	if (pc < MEG16)
	{
      printf("#%08u PC:%06X INST:%08X %s%s", ++OpCount, pc, opcode, InstuctionLookup(Function), SizeLookup(Size));
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


void n32016_build_matrix()
{
   uint32_t Index;

   memset(mat, 0xFF, sizeof(mat));

   for (Index = 0; Index < 256; Index++)
   {
#if 0
      if ((Index & 0x0F) == 0x0A)
      {
         mat[Index].p.Function = Index >> 4;
         mat[Index].p.Format = Format0;
      }
      else
#endif
      {
         switch (Index)
         {
            case 0x0A: // BEQ
            {
               mat[Index].p.Function = BEQ;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0x1A: // BNE
            {
               mat[Index].p.Function = BNE;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0x4A: // BH
            {
               mat[Index].p.Function = BH;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0x5A: // BLS
            {
               mat[Index].p.Function = BLS;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0x6A: // BGT
            {
               mat[Index].p.Function = BGT;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0x7A: // BLE
            {
               mat[Index].p.Function = BLE;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0x8A: // BFS
            {
               mat[Index].p.Function = BFS;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0x9A: // BFC
            {
               mat[Index].p.Function = BFC;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0xAA: // BLO
            {
               mat[Index].p.Function = BLO;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0xBA: // BHS
            {
               mat[Index].p.Function = BHS;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0xCA: // BLT
            {
               mat[Index].p.Function = BLT;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0xDA: // BGE
            {
               mat[Index].p.Function = BGE;
               mat[Index].p.Format = Format0;
            }
            break;

            case 0xEA: // BR
            {
               mat[Index].p.Function = BR;
               mat[Index].p.Format = Format0;
            }
            break;

          case 0x02:		// BSR
            {
               mat[Index].p.Function = BSR;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x12: // RET
            {
               mat[Index].p.Function = RET;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x22: // CXP
            {
               mat[Index].p.Function = CXP;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x32: // RXP
            {
               mat[Index].p.Function = RXP;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x42: // RETT
            {
               mat[Index].p.Function = RETT;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x52: // RETI
            {
               mat[Index].p.Function = RETI;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x62: // SAVE
            {
               mat[Index].p.Function = SAVE;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x72: // RESTORE
            {
               mat[Index].p.Function = RESTORE;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x82: // ENTER
            {
               mat[Index].p.Function = ENTER;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0x92: // EXIT
            {
               mat[Index].p.Function = EXIT;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0xA2: // NOP
            {
               mat[Index].p.Function = NOP;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0xE2: // SVC
            {
               mat[Index].p.Function = SVC;
               mat[Index].p.Format = Format1;
            }
            break;

            case 0xF2: // BPT
            {
               mat[Index].p.Function = BPT;
               mat[Index].p.Format = Format1;
            }
            break;

            CASE2(0x0C)		// ADDQ byte
            CASE2(0x0D)		// ADDQ word
            CASE2(0x0F)		// ADDQ dword
            {
               mat[Index].p.Function = ADDQ;
               mat[Index].p.Format = Format2;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE2(0x1C)		// CMPQ byte
            CASE2(0x1D)		// CMPQ word
            CASE2(0x1F)		// CMPQ dword
            {
               mat[Index].p.Function = CMPQ;
               mat[Index].p.Format = Format2;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE2(0x2C)		// SPR byte
            CASE2(0x2D)		// SPR word
            CASE2(0x2F)		// SPR dword
            {
               mat[Index].p.Function = SPR;
               mat[Index].p.Format = Format2;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE2(0x3C)		// Scond Byte
            CASE2(0x3D)		// Scond Word
            CASE2(0x3F)		// Scond DWord
            {
               mat[Index].p.Function = Scond;
               mat[Index].p.Format = Format2;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE2(0x4C)		// ACBB Byte
            CASE2(0x4D)		// ACCB Word
            CASE2(0x4F)		// ACCB DWord
            {
               mat[Index].p.Function = ACB;
               mat[Index].p.Format = Format2;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE2(0x5C)		// MOVQ byte
            CASE2(0x5D)		// MOVQ word
            CASE2(0x5F)		// MOVQ dword
            {
               mat[Index].p.Function = MOVQ;
               mat[Index].p.Format = Format2;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE2(0x6C)		// LPR byte
            CASE2(0x6D)		// LPR word
            CASE2(0x6F)		// LPR dword
            {
               mat[Index].p.Function = LPR;
               mat[Index].p.Format = Format2;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE2(0x7C)		// Type 3 byte
            CASE2(0x7D)		// Type 3 word
            CASE2(0x7F)		// Type 3 word
            {
               mat[Index].p.Format = Format3;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x00)		// ADD byte
            CASE4(0x01)		// ADD word
            CASE4(0x03)		// ADD dword
            {
               mat[Index].p.Function = ADD;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x04)		// CMP byte
            CASE4(0x05)		// CMP word
            CASE4(0x07)		// CMP dword
            {
               mat[Index].p.Function = CMP;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x08)		// BIC byte
            CASE4(0x09)		// BIC word
            CASE4(0x0B)		// BIC dword
            {
               mat[Index].p.Function = BIC;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x10)		// ADDC byte
            CASE4(0x11)		// ADDC word
            CASE4(0x13)		// ADDC dword
            {
               mat[Index].p.Function = ADDC;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x14)		// MOV byte
            CASE4(0x15)		// MOV word
            CASE4(0x17)		// MOV dword
            {
               mat[Index].p.Function = MOV;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x18)		// OR byte
            CASE4(0x19)		// OR word
            CASE4(0x1B)		// OR dword
            {
               mat[Index].p.Function = OR;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x20)		// SUB byte
            CASE4(0x21)		// SUB word
            CASE4(0x23)		// SUB dword
            {
               mat[Index].p.Function = SUB;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x24)		// ADDR byte
            CASE4(0x25)		// ADDR word
            CASE4(0x27)		// ADDR dword
            {
               mat[Index].p.Function = ADDR;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x28)		// AND byte
            CASE4(0x29)		// AND word
            CASE4(0x2B)		// AND dword
            {
               mat[Index].p.Function = AND;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x30)		// SUBC byte
            CASE4(0x31)		// SUBC word
            CASE4(0x33)		// SUBC dword
            {
               mat[Index].p.Function = SUBC;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x34)		// TBIT byte
            CASE4(0x35)		// TBIT word
            CASE4(0x37)		// TBIT dword
            {
               mat[Index].p.Function = TBIT;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            CASE4(0x38)		// XOR byte
            CASE4(0x39)		// XOR word
            CASE4(0x3B)		// XOR dword
            {
               mat[Index].p.Function = XOR;
               mat[Index].p.Format = Format4;
               mat[Index].p.Size = Index & 3;
            }
            break;

            case 0x0E: // String instruction
            {
               mat[Index].p.Format = Format5;
            }
            break;

            case 0x4E:		// Type 6
            {
               mat[Index].p.Format = Format6;
            }
            break;

            case 0xCE:		// Format 7
            {
               mat[Index].p.Format = Format7;
            }
            break;

            CASE4(0x2E)		// Type 8
            {
               mat[Index].p.Format = Format8;
            }
            break;

            case 0x3E:	/// TODO Check this looks wrong!
            {
               mat[Index].p.Format = Format0;
            }
            break;
      }
      }

      switch (mat[Index].p.Format)
      {
         case Format0:
         case Format1:
         default:
         {
            mat[Index].p.BaseSize = 1;
         }
         break;

         case Format2:
         case Format3:
         case Format4:
         {
            mat[Index].p.BaseSize = 2;
         }
         break;

         case Format5:
         case Format6:
         case Format7:
         case Format8:
         {
            mat[Index].p.BaseSize = 3;
         }
         break;
      }
   }
}
