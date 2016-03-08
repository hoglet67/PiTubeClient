#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "32016.h"
#include "mem32016.h"
#include "Profile.h"
#include "Trap.h"
#include "defs.h"

uint32_t OpCount = 0;
uint8_t FunctionLookup[256];

const uint8_t FormatSizes[FormatCount + 1] =
{ 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1 };

const char* PostfixLookup(uint8_t Postfix)
{
   switch (Postfix)
   {
      case sz8:
         return "B";
      case sz16:
         return "W";
      case Translating:
         return "T";
      case sz32:
         return "D";
      default:
         break;
   }

   return "";
}

void AddStringFlags(uint32_t opcode)
{
   if (opcode & (BIT(Backwards) | BIT(UntilMatch) | BIT(WhileMatch)))
   {
      PiTRACE("[");
      if (opcode & BIT(Backwards))
      {
         PiTRACE("B");
      }

      uint32_t Options = (opcode >> 17) & 3;


      if (Options == 1) // While match
      {
         PiTRACE("W");
      }
      else if (Options == 3)
      {
         PiTRACE("U");
      }



      PiTRACE("]");
   }
}


const char InstuctionText[InstructionCount][8] =
{
   // FORMAT 0
   "BEQ", "BNE", "BCS", "BCC", "BH", "BLS", "BGT", "BLE", "BFS", "BFC", "BLO", "BHS", "BLT", "BGE", "BR", "BN",

   // FORMAT 1	
   "BSR", "RET", "CXP", "RXP", "RETT", "RETI", "SAVE", "RESTORE", "ENTER", "EXIT", "NOP", "WAIT", "DIA", "FLAG", "SVC", "BPT",

   // FORMAT 2
   "ADDQ", "CMPQ", "SPR", "Scond", "ACB", "MOVQ", "LPR", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 3
   "CXPD", "TRAP", "BICPSR", "TRAP", "JUMP", "TRAP", "BISPSR", "TRAP", "TRAP", "TRAP", "ADJSP", "TRAP", "JSR", "TRAP", "CASE", "TRAP",

   // FORMAT 4
   "ADD", "CMP", "BIC", "TRAP", "ADDC", "MOV", "OR", "TRAP", "SUB", "ADDR", "AND", "TRAP", "SUBC", "TBIT", "XOR", "TRAP",

   // FORMAT 5
   "MOVS", "CMPS", "SETCFG", "SKPS", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 6
   "ROT", "ASH", "CBIT", "CBITI", "TRAP", "LSH", "SBIT", "SBITI", "NEG", "NOT", "TRAP", "SUBP", "ABS", "COM", "IBIT", "ADDP",

   // FORMAT 7
   "MOVM", "CMPM", "INSS", "EXTS", "MOVXBW", "MOVZBW", "MOVZiD", "MOVXiD", "MUL", "MEI", "Trap", "DEI", "QUO", "REM", "MOD", "DIV", "TRAP"

   // FORMAT 8
   "EXT", "CVTP", "INS", "CHECK", "INDEX", "FFS", "MOVUS", "MOVSU", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 9
   "MOVif", "LFSR", "MOVLF", "MOVFL", "ROUND", "TRUNC", "SFSR", "FLOOR", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 10
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 11
   "ADDf", "MOVf", "CMPf", "TRAP", "SUBf", "NEGf", "TRAP", "TRAP", "DIVf", "TRAP", "TRAP", "TRAP", "MULf", "ABSf", "TRAP", "TRAP",

   // FORMAT 12
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 13
   "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // FORMAT 14
   "RDVAL", "WRVAL", "LMR", "SMR", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "CINV", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP", "TRAP",

   // Illegal
   "TRAP" };

const char* InstuctionLookup(uint8_t Function)
{
   if (Function < InstructionCount)
   {

      return InstuctionText[Function];
   }

   return "Bad NS32016 opcode";
}

int32_t GetDisplacement(uint32_t* pPC)
{
   // Displacements are in Little Endian and need to be sign extended
   int32_t Value;

   MultiReg Disp;
   Disp.u32 = SWAP32(read_x32(*pPC));

   switch (Disp.u32 >> 29)
      // Look at the top 3 bits
   {
      case 0: // 7 Bit Posative
      case 1:
      {
         Value = Disp.u8;
         (*pPC) += sizeof(int8_t);
      }
      break;

      case 2: // 7 Bit Negative
      case 3:
      {
         Value = (Disp.u8 | 0xFFFFFF80);
         (*pPC) += sizeof(int8_t);
      }
      break;

      case 4: // 14 Bit Posative
      {
         Value = (Disp.u16 & 0x3FFF);
         (*pPC) += sizeof(int16_t);
      }
      break;

      case 5: // 14 Bit Negative
      {
         Value = (Disp.u16 | 0xFFFFC000);
         (*pPC) += sizeof(int16_t);
      }
      break;

      case 6: // 30 Bit Posative
      {
         Value = (Disp.u32 & 0x3FFFFFFF);
         (*pPC) += sizeof(int32_t);
      }
      break;

      case 7: // 30 Bit Negative
      default: // Stop it moaning about Value not being set ;)
      {
         Value = Disp.u32;
         (*pPC) += sizeof(int32_t);
      }
      break;
   }

   return Value;
}

uint32_t RegLookUp(uint32_t Start, uint32_t Offset)
{
   uint32_t Address = Start + Offset;

   if (Regs[0] < 0xFFFF)
   {
      if (Regs[0] >= EaPlusRn)
      {
         Address++; // Extra address for EaPlusRn on first operand
      }

      if (Regs[1] < 0xFFFF)
      {
         if (Regs[1] >= EaPlusRn)
         {
            Address++; // Extra address for EaPlusRn on second operand
         }
      }

      // printf("RegLookUp(%06" PRIX32 ", %06" PRIX32 ")\n", pc, Address);
      uint32_t Index;
      for (Index = 0; Index < 2; Index++)
      {
         if (Regs[Index] < 0xFFFF)
         {
            if (Index == 1)
            {
               PiTRACE(",");
            }

            if (Regs[Index] < 8)
            {
               PiTRACE("R%u", Regs[Index]);
            }
            else if (Regs[Index] < 16)
            {
               int32_t d = GetDisplacement(&Address);
               PiTRACE("%0" PRId32 "(R%u)", d, (Regs[Index] & 7));
            }
            else
            {
               switch (Regs[Index] & 0x1F)
               {
                  case FrameRelative:
                  {
                     int32_t d1 = GetDisplacement(&Address);
                     int32_t d2 = GetDisplacement(&Address);
                     PiTRACE("FrameR {%" PRId32 "}{%" PRId32 "}", d1, d2);
                  }
                  break;

                  case StackRelative:
                  {
                     int32_t d1 = GetDisplacement(&Address);
                     int32_t d2 = GetDisplacement(&Address);
                     PiTRACE("StackR {%" PRId32 "}{%" PRId32 "}", d1, d2);
                  }
                  break;

                  case StaticRelative:
                  {
                     int32_t d1 = GetDisplacement(&Address);
                     int32_t d2 = GetDisplacement(&Address);
                     PiTRACE("StaticR {%" PRId32 "}{%" PRId32 "}", d1, d2);
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
                     int32_t d = GetDisplacement(&Address);
                     PiTRACE("Absolute {%" PRId32 "}", d);
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
                     int32_t d = GetDisplacement(&Address);
                     PiTRACE("FpRelative {%" PRId32 "}", d);
                  }
                  break;

                  case SpRelative:
                  {
                     int32_t d = GetDisplacement(&Address);
                     PiTRACE("%" PRId32 "(SP)", d);
                  }
                  break;

                  case SbRelative:
                  {
                     int32_t d = GetDisplacement(&Address);
                     PiTRACE("SbRelative {%" PRId32 "}", d);
                  }
                  break;

                  case PcRelative:
                  {
#if 1
                     Start += GetDisplacement(&Address);
                     PiTRACE("&06%" PRIX32 "[PC]", Start);
#else
                     int32_t d = GetDisplacement(&Address);
                     PiTRACE("PcRelative {&%" PRId32 " %" PRId32"}", Start, d);
#endif
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
                  uint32_t Address = genaddr[Index];
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

   return Address;
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

void ShowRegs(uint8_t Pattern)
{
   uint32_t Count;
   uint32_t First = 1;

   PiTRACE("[");

   for (Count = 0; Count < 8; Count++)
   {
      if (Pattern & BIT(Count))
      {
         if (First == 0)
         {
            PiTRACE(",");
         }

         PiTRACE("R%" PRIu8 " ", pc);
         First = 0;
      }
   }

   PiTRACE("]");
}

#ifdef SHOW_INSTRUCTIONS
void ShowInstruction(uint32_t pc, uint32_t opcode, uint32_t Function, uint32_t OperandSize)
{
   static uint32_t old_pc = 0xFFFFFFFF;

   if (pc < (IO_BASE - 64))                     // The code will not work near the IO Space as it will have side effects
   {
      if (pc != old_pc)
      {
         old_pc = pc;
         const char* pText = "Bad NS32016 opcode";
         uint32_t Postfix = OperandSize;
         uint32_t Format = Function >> 4;

         //PiTRACE("#%08"PRIu32" ", ++OpCount);
         PiTRACE("&%06" PRIX32 " ", pc);
         PiTRACE("[%08" PRIX32 "] ", opcode);
         PiTRACE("F%01" PRIu32 " ", Format);

         if (Function < InstructionCount)
         {
            pText = InstuctionText[Function];

            if ((opcode & 0x80FF) == 0x800E)
            {
               Postfix = Translating;
            }

            if (Function == Scond)
            {
               uint32_t Condition = ((opcode >> 7) & 0x0F);
               PiTRACE("S%s%s ", &InstuctionText[Condition][1], PostfixLookup(Postfix)); // Offset by 1 to loose the 'B'
            }
            else
            {
               PiTRACE("%s%s ", pText, PostfixLookup(Postfix));
            }

            switch (Function)
            {
               case ADDQ:
               case CMPQ:
               case ACB:
               case MOVQ:
               {
                  int32_t Value = (opcode >> 7) & 0xF;
                  NIBBLE_EXTEND(Value);
                  PiTRACE("%" PRId32 ",", Value);
               }
               break;

               case LPR:
               {
                  int32_t Value = (opcode >> 7) & 0xF;
                  if (Value == 9)
                  {
                     PiTRACE("SP,");
                  }
                  else
                  {
                     PiTRACE("%" PRId32 ",", Value);
                  }
               }
               break;
            }

            uint32_t Address = RegLookUp(pc, FormatSizes[Format]);

            if ((Function <= BN) || (Function == BSR))
            {
               int32_t d = GetDisplacement(&Address);
               PiTRACE("&%06"PRIX32" ", pc + d);
            }

            switch (Function)
            {
               case SAVE:
               {
                  ShowRegs(read_x8(Address++));    //Access directly we do not want tube reads!
               }
               break;

              case ENTER:
               {
                  ShowRegs(read_x8(Address++));    //Access directly we do not want tube reads!
               }
               // Fall Through

               case RET:               
               case CXP:
               case RXP:
               {
                  int32_t d = GetDisplacement(&Address);
                  PiTRACE(" x'%" PRIX32 "", d);
               }
               break;

               case MOVS:
               case CMPS:
               case SKPS:
               {
                 AddStringFlags(opcode);
               }
               break;

#if 0
               case ADDQ:
               case CMPQ:
               case MOVQ:
               {
                  int32_t Value = (opcode >> 7) & 0xF;
                  NIBBLE_EXTEND(Value);
                  PiTRACE(",%" PRId32, Value);
               }
               break;
#endif
            }


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

#define FUNC(FORMAT, OFFSET) (((FORMAT) << 4) + (OFFSET)) 

uint8_t GetFunction(uint8_t FirstByte)
{
   switch (FirstByte & 0x0F)
   {
      case 0x0A:
         return FUNC(Format0, FirstByte >> 4);
      case 0x02:
         return FUNC(Format1, FirstByte >> 4);

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
      case 0x0E:
         return FUNC(Format5, 0); // String instruction
      case 0x4E:
         return FUNC(Format6, 0);
      case 0xCE:
         return FUNC(Format7, 0);
         CASE4(0x2E)
            : return FUNC(Format8, 0);
      case 0x3E:
         return FUNC(Format9, 0);
      case 0x7E:
         return FUNC(Format10, 0);
      case 0xBE:
         return FUNC(Format11, 0);
      case 0xFE:
         return FUNC(Format12, 0);
      case 0x9E:
         return FUNC(Format13, 0);
      case 0x1E:
         return FUNC(Format14, 0);
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

static void getgen(int gen, int c)
{
   gen &= 0x1F;
   Regs[c] = gen;

   if (gen >= EaPlusRn)
   {
      Regs[c] |= READ_PC_BYTE() << 8;

      if ((Regs[c] & 0xF800) == (Immediate << 11))
      {
         SET_TRAP(IllegalImmediate);
      }

      if ((Regs[c] & 0xF800) >= (EaPlusRn << 11))
      {
         SET_TRAP(IllegalDoubleIndexing);
      }
   }
}

#if 0
static void GetGenPhase2(int gen, int c)
{
   if (gen < 0xFFFF)                                              // Does this Operand exist ?
   {
      if (gen <= R7)
      {
         genaddr[c] = (uint32_t) &r[gen];
         gentype[c] = Register;
         return;
      }

      if (gen == Immediate)
      {
         // Why can't they just decided on an endian and then stick to it?
         MultiReg temp3;

         temp3.u32 = SWAP32(read_x32(pc));
         if (OpSize.Op[c] == sz8)
            genaddr[c] = temp3.u8;
         else if (OpSize.Op[c] == sz16)
            genaddr[c] = temp3.u16;
         else
            genaddr[c] = temp3.u32;

         pc += OpSize.Op[c];
         gentype[c] = OpImmediate;
         return;
      }

      gentype[c] = Memory;

      if (gen <= R7_Offset)
      {
         genaddr[c] = r[gen & 7] + getdisp();
         return;
      }

      uint32_t temp, temp2;

      if (gen >= EaPlusRn)
      {
         temp = (gen >> 8) & 7;

         uint32_t Shift = gen & 3;
         GetGenPhase2(gen >> 11, c);

         int32_t Offset = ((int32_t) r[temp]) * (1 << Shift);
         if (gentype[c] != Register)
         {
            genaddr[c] += Offset;
         }
         else
         {
            genaddr[c] = *((uint32_t*) genaddr[c]) + Offset;
         }

         gentype[c] = Memory;                               // Force Memory
         return;
      }

      switch (gen)
      {
         case FrameRelative:
            temp = getdisp();
            temp2 = getdisp();
            genaddr[c] = read_x32(fp + temp);
            genaddr[c] += temp2;
            break;

         case StackRelative:
            temp = getdisp();
            temp2 = getdisp();
            genaddr[c] = read_x32(GET_SP() + temp);
            genaddr[c] += temp2;
            break;

         case StaticRelative:
            temp = getdisp();
            temp2 = getdisp();
            genaddr[c] = read_x32(sb + temp);
            genaddr[c] += temp2;
            break;

         case Absolute:
            genaddr[c] = getdisp();
            break;

         case External:
            temp = read_x32(mod + 4);
            temp += ((int32_t) getdisp()) * 4;
            temp2 = read_x32(temp);
            genaddr[c] = temp2 + getdisp();
            break;

         case TopOfStack:
            genaddr[c] = GET_SP();
            gentype[c] = TOS;
            break;

         case FpRelative:
            genaddr[c] = getdisp() + fp;
            break;

         case SpRelative:
            genaddr[c] = getdisp() + GET_SP();
            break;

         case SbRelative:
            genaddr[c] = getdisp() + sb;
            break;

         case PcRelative:
            genaddr[c] = getdisp() + startpc;
            break;

         default:
            n32016_dumpregs("Bad NS32016 gen mode");
            break;
      }
   }
}
#endif

uint32_t Decode(uint32_t startpc)
{
   uint32_t pc = startpc;
   uint32_t opcode = read_x32(pc);
   uint32_t Function = FunctionLookup[opcode & 0xFF];
   uint32_t Format = Function >> 4;
   OperandSizeType OpSize;

   Regs[0] =
   Regs[1] = 0xFFFF;

   if (Format < (FormatCount + 1))
   {
      pc += FormatSizes[Format];                                        // Add the basic number of bytes for a particular instruction
   }

   switch (Format)
   {
      case Format2:
      {
         SET_OP_SIZE(opcode);
         getgen(opcode >> 11, 0);
      }
      break;

      case Format3:
      {
         Function += ((opcode >> 7) & 0x0F);
         SET_OP_SIZE(opcode);
         getgen(opcode >> 11, 0);
      }
      break;

      case Format4:
      {
         SET_OP_SIZE(opcode);
         getgen(opcode >> 11, 0);
         getgen(opcode >> 6, 1);
      }
      break;

      case Format5:
      {
         Function += ((opcode >> 10) & 0x0F);
         SET_OP_SIZE(opcode >> 8);
         if (opcode & BIT(Translation))
         {
            SET_OP_SIZE(0);         // 8 Bit
         }
      }
      break;

      case Format6:
      {
         Function += ((opcode >> 10) & 0x0F);
         SET_OP_SIZE(opcode >> 8);

         // Ordering important here, as getgen uses Operand Size
         switch (Function)
         {
            case ROT:
            case ASH:
            case LSH:
            {
               OpSize.Op[0] = sz8;
            }
            break;
         }

         getgen(opcode >> 19, 0);
         getgen(opcode >> 14, 1);
      }
      break;

      case Format7:
      {
         Function += ((opcode >> 10) & 0x0F);
         SET_OP_SIZE(opcode >> 8);
         getgen(opcode >> 19, 0);
         getgen(opcode >> 14, 1);
      }
      break;

      case Format8:
      {
         if (opcode & 0x400)
         {
            if (opcode & 0x80)
            {
               switch (opcode & 0x3CC0)
               {
                  case 0x0C80:
                  {
                     Function = MOVUS;
                  }
                  break;

                  case 0x1C80:
                  {
                     Function = MOVSU;
                  }
                  break;

                  default:
                  {
                     Function = TRAP;
                  }
                  break;
               }
            }
            else
            {
               Function = (opcode & 0x40) ? FFS : INDEX;
            }
         }
         else
         {
            Function += ((opcode >> 6) & 3);
         }

         SET_OP_SIZE(opcode >> 8);

         if (Function == CVTP)
         {
            SET_OP_SIZE(3);               // 32 Bit
         }

         getgen(opcode >> 19, 0);
         getgen(opcode >> 14, 1);
      }
      break;

      case Format9:
      {
         Function += ((opcode >> 11) & 0x07);
      }
      break;

      case Format11:
      {
         Function += ((opcode >> 10) & 0x0F);
      }
      break;

      case Format14:
      {
         Function += ((opcode >> 10) & 0x0F);
      }
      break;

      default:
      {
         SET_TRAP(UnknownFormat);
      }
      break;
   }

   ShowInstruction(startpc, opcode, Function, OpSize.Op[0]);

   return pc;
}


void Disassemble(uint32_t Location, uint32_t End)
{
   do
   {
      Location = Decode(Location);
   }
   while (Location < End);

#ifdef WIN32
   system("pause");
#endif
}