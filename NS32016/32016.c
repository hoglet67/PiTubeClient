// B-em v2.2 by Tom Walker
// 32016 parasite processor emulation (not working yet)

// And Simon R. Ellwood
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "32016.h"
#include "mem32016.h"
#include "defs.h"
#include "Trap.h"

#ifdef PROFILING
#include "Profile.h"
#endif

int nsoutput = 0;

ProcessorRegisters PR;
uint32_t r[8];
uint32_t pc;
uint32_t sp[2];

uint32_t nscfg;
uint32_t Trace = 0;
uint32_t tube_irq = 0;


uint32_t startpc;

uint16_t Regs[2];
uint32_t genaddr[2];
int gentype[2];
OperandSizeType OpSize;

const uint16_t OpSizeLookup[4] =
{
   (sz8 << 8)  + sz8,
   (sz16 << 8) + sz16,
   0xFFFF,                                                     // Illegal
   (sz32 << 8) + sz32
};

void n32016_reset(uint32_t StartAddress)
{
   n32016_build_matrix();

   pc = StartAddress;
   psr = 0;

   //PR.BPC = 0x20F; //Example Breakpoint
   PR.BPC = 0xFFFFFFFF;
}

static void pushw(uint16_t val)
{
   DEC_SP(2);
   write_x16(GET_SP(), val);
}

static void pushd(uint32_t val)
{
   DEC_SP(4);
   write_x32(GET_SP(), val);
}

void PushArbitary(uint32_t Value, uint32_t Size)
{
   DEC_SP(Size);
   write_Arbitary(GET_SP(), &Value, Size);
}

static uint16_t popw()
{
   uint16_t temp = read_x16(GET_SP());
   INC_SP(2);

   return temp;
}

static uint32_t popd()
{
   uint32_t temp = read_x32(GET_SP());
   INC_SP(4);

   return temp;
}

uint32_t PopArbitary(uint32_t Size)
{
   uint32_t Result = read_n(GET_SP(), Size);
   INC_SP(Size);

   return Result;
}

#ifdef BYTE_SWAP

#if 1
static uint32_t getdisp()
{ 
   // Displacements are in Little Endian and need to be sign extended

   MultiReg Addr;
   Addr.u32 = SWAP32(read_x32(pc++));

   switch (Addr.u32 >> 29)
   {
      case 0:                                                          // 7 Bit Posative
      case 1:
      {
         return Addr.u8.Value;
      }
      // No break due to return

      case 2:                                                         // 7 Bit Negative
      case 3:
      {
         return Addr.u8.Value | 0xFFFFFF80;
      }
      // No break due to return

      case 4:                                                          // 14 Bit Posative
      {
         pc++;
         return Addr.u16.Value & 0x3FFF;
      }
      // No break due to return

      case 5:                                                          // 14 Bit Negative
      {
         pc++;
         return Addr.u16.Value | 0xFFFFC000;
      }
      // No break due to return

      case 6:                                                         // 30 Bit Posative
      {
         pc += 3;
         return Addr.u32 & 0x3FFFFFFF;
      }
      // No break due to return
   
      case 7:                                                        // 30 Bit Negative
      {
         pc += 3;
         return Addr.u32; 
      }
      // No break due to return
   }

   return 0;                                                               // Unreaable code
}

#else

static uint32_t getdisp()
{ 
   uint32_t addr = read_x32(pc++);

   if ((addr & 0x80) == 0)                                          // 8 Bit Operand
   {
      if (addr & 0x40)                                               // Negative
      {
         return addr | 0xFFFFFF80;
      }
      
      return addr & 0xFF;
   }

   addr = SWAP32(addr);                                              // Now LSB at the top
   
   if ((addr & 0x40000000) == 0)                                     // 14 Bit Operand
   {
      pc++;
      addr >>= 16;

      if (addr & 0x2000)                                             // Negative
      {
         return addr | 0xFFFFC000;
      }

      return addr & 0x3FFF;
   }

   pc += 3;                                                            // 30 Bit Operand
   if (addr & 0x20000000)                                              // Negative
   {
      return addr;                                                     // Top three bits already set!
   }
   return addr & 0x3FFFFFFF;
}
#endif
#else
static uint32_t getdisp()
{
   uint32_t addr = READ_PC_BYTE();

   if (!(addr & 0x80))
   {
      return addr | ((addr & 0x40) ? 0xFFFFFF80 : 0);
   }

   if (!(addr & 0x40))
   {
      addr &= 0x3F;
      addr = (addr << 8) | READ_PC_BYTE();

      return addr | ((addr & 0x2000) ? 0xFFFFC000 : 0);
   }

   addr &= 0x3F;
   addr = (addr << 24) | (READ_PC_BYTE() << 16);
   addr = addr | (READ_PC_BYTE() << 8);
   addr = addr | READ_PC_BYTE();

   return addr | ((addr & 0x20000000) ? 0xC0000000 : 0);
}
#endif

uint32_t Truncate(uint32_t Value, uint32_t Size)
{
   switch (Size)
   {
      case sz8:
         return Value & 0x000000FF;
      case sz16:
         return Value & 0x0000FFFF;
   }

   return Value;
}

uint32_t ReadGen(uint32_t c)
{
   uint32_t Temp = 0;

   switch (gentype[c])
   {
      case Memory:
      {
         switch (OpSize.Op[c])
         {
            case sz8:   return read_x8(genaddr[c]);
            case sz16:  return read_x16(genaddr[c]);
            case sz32:  return read_x32(genaddr[c]);
         }
      }
      break;

      case Register:
      {
         Temp = *(uint32_t*) genaddr[c];
         return Truncate(Temp, OpSize.Op[c]);
      }
      // No break due to return
     
      case TOS:
      {
         return PopArbitary(OpSize.Op[c]);
      }
      // No break due to return

      case OpImmediate:
      {
         return Truncate(genaddr[c], OpSize.Op[c]);
      }
      // No break due to return
   }

   return 0;
}

uint32_t ReadAddress(uint32_t c)
{
   if (gentype[c] == Register)
   {
      return *(uint32_t *) genaddr[c];
   }
 
   return genaddr[c];
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
            genaddr[c] = temp3.u8.Value;
         else if (OpSize.Op[c] == sz16)
            genaddr[c] = temp3.u16.Value;
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
            temp += getdisp() << 2;
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

uint64_t readgenq(uint32_t c)
{
   uint64_t temp;

   if (gentype[c] == Register)
   {
      temp = *(uint64_t *) genaddr[c];
   }
   else
   {
      temp = read_x32(genaddr[c]);              // This is bonkers!
      temp |= ((uint64_t) read_x32(genaddr[c] + 4)) << 32;
   }

   return temp;
}

// From: http://homepage.cs.uiowa.edu/~jones/bcd/bcd.html
static uint32_t bcd_add_16(uint32_t a, uint32_t b, uint32_t *carry)
{
   uint32_t t1, t2; // unsigned 32-bit intermediate values
   //PiTRACE("bcd_add_16: in  %08x %08x %08x\n", a, b, *carry);
   if (*carry)
   {
      b++; // I'm 90% sure its OK to handle carry this way
   } // i.e. its OK if the ls digit of b becomes A
   t1 = a + 0x06666666;
   t2 = t1 ^ b; // sum without carry propagation
   t1 = t1 + b; // provisional sum
   t2 = t1 ^ t2; // all the binary carry bits
   t2 = ~t2 & 0x11111110; // just the BCD carry bits
   t2 = (t2 >> 2) | (t2 >> 3); // correction
   t2 = t1 - t2; // corrected BCD sum
   *carry = (t2 & 0xFFFF0000) ? 1 : 0;
   t2 &= 0xFFFF;
   //PiTRACE("bcd_add_16: out %08x %08x\n", t2, *carry);
   return t2;
}

static uint32_t bcd_sub_16(uint32_t a, uint32_t b, uint32_t *carry)
{
   uint32_t t1, t2; // unsigned 32-bit intermediate values
   //PiTRACE("bcd_sub_16: in  %08x %08x %08x\n", a, b, *carry);
   if (*carry)
   {
      b++;
   }
   *carry = 0;
   t1 = 0x9999 - b;
   t2 = bcd_add_16(t1, 1, carry);
   t2 = bcd_add_16(a, t2, carry);
   *carry = 1 - *carry;
   //PiTRACE("bcd_add_16: out %08x %08x\n", t2, *carry);
   return t2;
}

static uint32_t bcd_add(uint32_t a, uint32_t b, int size, uint32_t *carry)
{
   if (size == sz8)
   {
      uint32_t word = bcd_add_16(a, b, carry);
      // If anything beyond bit 7 is set, then there has been a carry out
      *carry = (word & 0xFF00) ? 1 : 0;
      return word & 0xFF;
   }
   else if (size == sz16)
   {
      return bcd_add_16(a, b, carry);
   }
   else
   {
      uint32_t word0 = bcd_add_16(a & 0xFFFF, b & 0xFFFF, carry);
      uint32_t word1 = bcd_add_16(a >> 16, b >> 16, carry);
      return word0 + (word1 << 16);
   }
}

static uint32_t bcd_sub(uint32_t a, uint32_t b, int size, uint32_t *carry)
{
   if (size == sz8)
   {
      uint32_t word = bcd_sub_16(a, b, carry);
      // If anything beyond bit 7 is set, then there has been a carry out
      *carry = (word & 0xFF00) ? 1 : 0;
      return word & 0xFF;
   }
   else if (size == sz16)
   {
      return bcd_sub_16(a, b, carry);
   }
   else
   {
      uint32_t word0 = bcd_sub_16(a & 0xFFFF, b & 0xFFFF, carry);
      uint32_t word1 = bcd_sub_16(a >> 16, b >> 16, carry);
      return word0 + (word1 << 16);
   }
}

static uint32_t update_add_flags(uint32_t a, uint32_t b, uint32_t cin)
{
   uint32_t sum = a + (b + cin);

   if (b == 0xffffffff && cin == 1)
   {
      C_FLAG = 1;
      F_FLAG = 0;
   }
   else
   {
      switch (OpSize.Op[0])
      {
         case sz8:
            {
               C_FLAG = TEST(sum & 0x100);
               F_FLAG = TEST((a ^ sum) & (b ^ sum) & 0x80);
            }
            break;

         case sz16:
            {
               C_FLAG = TEST(sum & 0x10000);
               F_FLAG = TEST((a ^ sum) & (b ^ sum) & 0x8000);
            }
            break;

         case sz32:
            {
               C_FLAG = TEST(sum < a);
               F_FLAG = TEST((a ^ sum) & (b ^ sum) & 0x80000000);
            }
            break;
      }
   }
   //PiTRACE("ADD FLAGS: C=%d F=%d\n", C_FLAG, F_FLAG);

   return sum;
}

static void update_sub_flags(uint32_t a, uint32_t b, uint32_t cin)
{
   if (b == 0xffffffff && cin == 1)
   {
      C_FLAG = 1;
      F_FLAG = 0;
   }
   else
   {
      uint32_t diff = a - (b + cin);
      C_FLAG = TEST(diff > a);
      F_FLAG = ((b ^ a) & (b ^ diff) & 0x80000000);
   }
   //PiTRACE("SUB FLAGS: C=%d F=%d\n", C_FLAG, F_FLAG);
}

// The difference between DIV and QUO occurs when the result (the quotient) is negative
// e.g. -16 QUO 3 ===> -5
// e.g. -16 DIV 3 ===> -6
// i.e. DIV rounds down to the more negative nu,ber
// This case is detected if the sign bits of the two operands differ
static uint32_t div_operator(uint32_t a, uint32_t b)
{
   uint32_t ret = 0;
   int signmask = BIT(((OpSize.Op[0] - 1) << 3) + 7);
   if ((a & signmask) && !(b & signmask))
   {
      // e.g. a = -16; b =  3 ===> a becomes -18
      a -= b - 1;
   }
   else if (!(a & signmask) && (b & signmask))
   {
      // e.g. a =  16; b = -3 ===> a becomes 18
      a -= b + 1;
   }
   switch (OpSize.Op[0])
   {
      case sz8:
         ret = (int8_t) a / (int8_t) b;
      break;

      case sz16:
         ret = (int16_t) a / (int16_t) b;
      break;

      case sz32:
         ret = (int32_t) a / (int32_t) b;
      break;
   }
   return ret;
}

static uint32_t mod_operator(uint32_t a, uint32_t b)
{
   return a - div_operator(a, b) * b;
}

// Handle the writing to the upper half of mei/dei destination
static void handle_mei_dei_upper_write(uint64_t result)
{
   uint32_t temp;
   // Writing to an odd register is strictly speaking undefined
   // But BBC Basic relies on a particular behaviour that the NS32016 has in this case
   uint32_t reg_addr = genaddr[1] + ((Regs[1] & 1) ? -4 : 4);
   switch (OpSize.Op[0])
   {
      case sz8:
         temp = (uint8_t) (result >> 8);
         if (gentype[1] == Register)
            *(uint8_t *) (reg_addr) = temp;
         else
            write_x8(genaddr[1] + 4, temp);
      break;

      case sz16:
         temp = (uint16_t) (result >> 16);
         if (gentype[1] == Register)
            *(uint16_t *) (reg_addr) = temp;
         else
            write_x16(genaddr[1] + 4, temp);
      break;

      case sz32:
         temp = (uint32_t) (result >> 32);
         if (gentype[1] == Register)
            *(uint32_t *) (reg_addr) = temp;
         else
            write_x32(genaddr[1] + 4, temp);
      break;
   }
}

uint32_t CompareCommon(uint32_t temp, uint32_t temp2)
{
   L_FLAG = TEST(temp2 > temp);

   if (OpSize.Op[0] == sz8)
   {
      N_FLAG = TEST(((int8_t) temp2) > ((int8_t) temp));
   }
   else if (OpSize.Op[0] == sz16)
   {
      N_FLAG = TEST(((int16_t) temp2) > ((int16_t) temp));
   }
   else
   {
      N_FLAG = TEST(((int32_t) temp2) > ((int32_t) temp));
   }

   Z_FLAG = TEST(temp == temp2);

   return Z_FLAG;
}

uint32_t StringMatching(uint32_t opcode, uint32_t Value)
{
   uint32_t Options = (opcode >> 17) & 3;

   if (Options)
   {
      uint32_t Compare = Truncate(r[4], OpSize.Op[0]);

      if (Options == 1) // While match
      {
         if (Value != Compare)
         {
            F_FLAG = 1; // Set PSR F Bit
            return 1;
         }
      }

      if (Options == 3) // Until Match
      {
         if (Value == Compare)
         {
            F_FLAG = 1; // Set PSR F Bit
            return 1;
         }
      }
   }

   return 0;
}

void StringRegisterUpdate(uint32_t opcode)
{
   uint32_t Size = OpSize.Op[0];

   if (opcode & BIT(Backwards)) // Adjust R1
   {
      r[1] -= Size;
   }
   else
   {
      r[1] += Size;
   }

   if (((opcode >> 10) & 0x0F) != (SKPS & 0x0F))
   {
      if (opcode & BIT(Backwards)) // Adjust R2 for all but SKPS
      {
         r[2] -= Size;
      }
      else
      {
         r[2] += Size;
      }
   }

   r[0]--; // Adjust R0
}

uint32_t CheckCondition(uint32_t Pattern)
{
   uint32_t bResult = 0;

   switch (Pattern & 0xF)
   {
      case 0x0:
         if (Z_FLAG)
            bResult = 1;
         break;
      case 0x1:
         if (!Z_FLAG)
            bResult = 1;
         break;
      case 0x2:
         if (C_FLAG)
            bResult = 1;
         break;
      case 0x3:
         if (!C_FLAG)
            bResult = 1;
         break;
      case 0x4:
         if (L_FLAG)
            bResult = 1;
         break;
      case 0x5:
         if (!L_FLAG)
            bResult = 1;
         break;
      case 0x6:
         if (N_FLAG)
            bResult = 1;
         break;
      case 0x7:
         if (!N_FLAG)
            bResult = 1;
         break;
      case 0x8:
         if (F_FLAG)
            bResult = 1;
         break;
      case 0x9:
         if (!F_FLAG)
            bResult = 1;
         break;
      case 0xA:
         if (!(psr & (0x40 | 0x04)))
         //if (!(Z_FLAG || L_FLAG))
            bResult = 1;
         break;
      case 0xB:
         if (psr & (0x40 | 0x04))
         //if (Z_FLAG || L_FLAG)
            bResult = 1;
         break;
      case 0xC:
         if (!(psr & (0x40 | 0x80)))
         //if (!(Z_FLAG || N_FLAG))
            bResult = 1;
         break;
      case 0xD:
         if (psr & (0x40 | 0x80))
         //if (Z_FLAG || N_FLAG)
            bResult = 1;
         break;
      case 0xE:
         bResult = 1;
         break;
      case 0xF:
         break;
   }

   return bResult;
}

uint32_t BitPrefix(void)
{
   int32_t Offset = ReadGen(0);
   uint32_t bit;
   SIGN_EXTEND(OpSize.Op[0], Offset);

   if (gentype[1] == Register)
   {
      // operand 0 is a register
      OpSize.Op[1] = sz32;
      bit = ((uint32_t) Offset) & 31;
   }
   else
   {
      // operand0 is memory
      genaddr[1] += Offset / 8;
      OpSize.Op[1] = sz8;
      bit = ((uint32_t) Offset) & 7;
   }

   return BIT(bit);
}

void PopRegisters(void)
{
   int c;
   int32_t temp = READ_PC_BYTE();

   for (c = 0; c < 8; c++)
   {
      if (temp & BIT(c))
      {
         r[c ^ 7] = popd(r[c]);
      }
   }
}

void TakeInterrupt(uint32_t IntBase)
{
   uint32_t temp = psr;
   uint32_t temp2, temp3;

   psr &= ~0xF00;
   pushw(temp);
   pushw(mod);
   
   while (read_x8(pc) == 0xB2)                                    // Do not stack the address of a WAIT instruction!
   {
      pc++;
   }
   
   pushd(pc);
   temp = read_x32(IntBase);
   mod = temp & 0xFFFF;
   temp3 = temp >> 16;
   sb = read_x32(mod);
   temp2 = read_x32(mod + 8);
   pc = temp2 + temp3;
}

void n32016_exec(uint32_t tubecycles)
{
   uint32_t opcode, WriteSize, WriteIndex;
   uint32_t temp = 0, temp2, temp3;
   uint32_t Function;

   if (tube_irq & 2)
   {
      // NMI is edge sensitive, so it should be cleared here
      tube_irq &= ~2;
      TakeInterrupt(intbase + (1 * 4));
   }
   else if ((tube_irq & 1) && (psr & 0x800))
   {
      // IRQ is level sensitive, so the called should maintain the state
      TakeInterrupt(intbase);
   }

   while (tubecycles--)
   {
      CLEAR_TRAP();

      WriteSize      = szVaries;                                            // The size a result may be written as
      WriteIndex     = 0;                                                   // Default to writing operand 0
      OpSize.Whole   = 0;
 
      Regs[0]        =
      Regs[1]        = 0xFFFF;

      startpc  = pc;
      opcode = read_x32(pc);

      if (pc == PR.BPC)
      {
         SET_TRAP(BreakPointHit);
         goto DoTrap;
      }

      BreakPoint(startpc, opcode);

      Function = FunctionLookup[opcode & 0xFF];
      uint32_t Format   = Function >> 4;

      if (Format < (FormatCount + 1))
      {
         pc += FormatSizes[Format];                                        // Add the basic number of bytes for a particular instruction
      }

      switch (Format)
      {
         case Format0:
         {
            temp = getdisp();
         }
         break;

         case Format1:
         {
            if (Function <= RETT)
            {
               temp = getdisp();
            }
         }
         break;

         case Format2:
         {
            SET_OP_SIZE(opcode);
            WriteSize = OpSize.Op[0];
            WriteIndex = 0;
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
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
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
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;

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
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;

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

            WriteSize = OpSize.Op[1];
            WriteIndex = 1;

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

      GetGenPhase2(Regs[0], 0);
      GetGenPhase2(Regs[1], 1);


      if (TrapFlags)
      {
         DoTrap:
         HandleTrap();
         continue;
      }

      ShowInstruction(startpc, opcode, Function, OpSize.Op[0], temp);

#ifdef PROFILING
      ProfileAdd(Function, Regs[0], Regs[1]);
#endif

      switch (Function)
      {
         // Format 0 : Branches

         case BEQ:
         case BNE:
         case BCS:
         case BCC:
         case BH:
         case BLS:
         case BGT:
         case BLE:
         case BFS:
         case BFC:
         case BLO:
         case BHS:
         case BLT:
         case BGE:
         {
            if (CheckCondition(Function) == 0)
            {
               continue;
            }
         }
         // Fall Through

         case BR:
         {
            pc = startpc + temp;
            continue;
         }
         // No break due to continue

         case BN:
         {
            continue;
         }
         // No break due to continue

         // Format 1

         case BSR:
         {
            pushd(pc);
            pc = startpc + temp;
            continue;
         }
         // No break due to continue

         case RET:
         {
            pc = popd();
            INC_SP(temp);
            continue;
         }
         // No break due to continue

         case CXP:
         {
            temp2 = read_x32(mod + 4) + (4 * temp);

            temp = read_x32(temp2);   // Matching Tail with CXPD, complier do your stuff
            pushw(0);
            pushw(mod);
            pushd(pc);
            mod = temp & 0xFFFF;
            temp3 = temp >> 16;
            sb = read_x32(mod);
            temp2 = read_x32(mod + 8);
            pc = temp2 + temp3;
            continue;
         }
         // No break due to continue

         case RXP:
         {
            pc = popd();
            temp2 = popd();
            mod = temp2 & 0xFFFF;
            INC_SP(temp);
            sb = read_x32(mod);
            continue;
         }
         // No break due to continue

         case RETT:
         {
            pc = popd();
            mod = popw();
            psr = popw();
            INC_SP(temp);
            sb = read_x32(mod);
            continue;
         }
         // No break due to continue

#if 0
         case RETI:
         {
            // Also Performs either one or two "End of Interrupt" bus cycles in order to inform the appropriate Interrupt Controller(s) that this interrupt service procedure is ending.
            pc = popd();
            if (DirectExceptionMode = 0)
            {
               mod = popw();
            }
            else
            {
               popw();
            }
            psr = popw();
            INC_SP(temp);

            if (DirectExceptionMode = 0)
            {
               sb = read_x32(mod);
            }
            continue;
         }
         // No break due to continue
#endif

         case SAVE:
         {
            int c;
            temp = READ_PC_BYTE();

            for (c = 0; c < 8; c++)                             // Matching tail with ENTER
            {
               if (temp & BIT(c))
               {
                  pushd(r[c]);
               }
            }
            continue;
         }
         // No break due to continue

         case RESTORE:
         {
            PopRegisters();
            continue;
         }
         // No break due to continue

         case ENTER:
         {
            int c;
            temp = READ_PC_BYTE();
            temp2 = getdisp();
            pushd(fp);
            fp = GET_SP();
            DEC_SP(temp2);

            for (c = 0; c < 8; c++)                              // Matching tail with SAVE
            {
               if (temp & BIT(c))
               {
                  pushd(r[c]);
               }
            }
            continue;
         }
         // No break due to continue

         case EXIT:
         {
            PopRegisters();
            SET_SP(fp);
            fp = popd();
            continue;
         }
         // No break due to continue

         case NOP:
         {
            continue;
         }
         // No break due to continue

         case WAIT:                                             // Wait for interrupt then continue execution
         case DIA:                                              // Wait for interrupt and in theory never resume execution (stack manipulation would get round this)
         {
            tubecycles = 0;                                    // Exit promptly as we are waiting for an interrupt
            pc = startpc;
            continue;
         }
         // No break due to continue

         case FLAG:
         {
            GOTO_TRAP(FlagInstruction);
         }
         // No break due to goto

         case SVC:
         {
            temp = psr;
            psr &= ~0x700;
            // In SVC, the address pushed is the address of the SVC opcode
            pushw(temp);
            pushw(mod);
            pushd(startpc);
            temp = read_x32(intbase + (5 * 4));
            mod = temp & 0xFFFF;
            temp3 = temp >> 16;
            sb = read_x32(mod);
            temp2 = read_x32(mod + 8);
            pc = temp2 + temp3;
            continue;
         }
         // No break due to continue

         case BPT:
         {
            GOTO_TRAP(BreakPointTrap);
         }
         // No break due to goto

         // Format 2
         
         case ADDQ:
         {
            temp2 = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp2);
            temp = ReadGen(0);

            temp = update_add_flags(temp, temp2, 0);
         }
         break;

         case CMPQ:
         {
            temp2 = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp2);
            temp = ReadGen(0);
            SIGN_EXTEND(OpSize.Op[0], temp);
            CompareCommon(temp, temp2);
            continue;
         }
         // No break due to continue

         case SPR:
         {
            temp2 = (opcode >> 7) & 0xF;

            switch (temp2)
            {
               case 0x0:
                  temp = psr & 0xff;
               break;
               case 0x8:
                  temp = fp;
               break;
               case 0x9:
                  temp = GET_SP();
               break;
               case 0xA:
                  temp = sb;
               break;
               case 0xD:
                  temp = psr;
               break;
               case 0xF:
                  temp = mod;
               break;

               default:
               {
                  GOTO_TRAP(IllegalSpecialReading);
               }
               // No break due to goto
            }
         }
         break;

         case Scond:
         {
            temp = CheckCondition(opcode >> 7);
         }
         break;

         case ACB:
         {
            temp2 = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp2);
            temp = ReadGen(0);
            temp += temp2;
            temp2 = getdisp();
            if (Truncate(temp, OpSize.Op[0]))
               pc = startpc + temp2;
         }
         break;

         case MOVQ:
         {
            temp = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp);
         }
         break;

         case LPR:
         {
            temp  = ReadGen(0);
            temp2 = (opcode >> 7) & 0xF;

            switch (temp2)
            {
               case 0:
               {
                  psr = (psr & 0xFF00) | (temp & 0xFF);
               }
               break;

               case 5:
               case 6:
               case 7:
               {
                  GOTO_TRAP(IllegalSpecialWriting);
               }
               // No break due to goto

               case 9:
               {
                  SET_SP(temp);
               }
               break;

               default:
               {
                  PR.Direct[temp2] = temp;
               }
               break;
            }

            continue;
         }
         // No break due to continue
     
         // Format 3

         case CXPD:
         {
            temp2 = ReadAddress(0);

            temp = read_x32(temp2);   // Matching Tail with CXPD, complier do your stuff
            pushw(0);
            pushw(mod);
            pushd(pc);
            mod = temp & 0xFFFF;
            temp3 = temp >> 16;
            sb = read_x32(mod);
            temp2 = read_x32(mod + 8);
            pc = temp2 + temp3;
            continue;
         }
         // No break due to continue

         case BICPSR:
         {
            temp = ReadGen(0);
            psr &= ~temp;
            continue;
         }
         // No break due to continue

         case JUMP:
         {
            // JUMP is in access class addr, so ReadGen() cannot be used
            pc = ReadAddress(0);
            continue;
         }
         // No break due to continue

         case BISPSR:
         {
            temp = ReadGen(0);
            psr |= temp;
            continue;
         }
         // No break due to continue

         case ADJSP:
         {
            temp = ReadGen(0);
            SIGN_EXTEND(OpSize.Op[0], temp);
            DEC_SP(temp);
            continue;
         }
         // No break due to continue

         case JSR:
         {
            // JSR is in access class addr, so ReadGen() cannot be used
            pushd(pc);
            pc = ReadAddress(0);
            continue;
         }
         // No break due to continue

         case CASE:
         {
            temp = ReadGen(0);
            SIGN_EXTEND(OpSize.Op[0], temp);
            pc = startpc + temp;
            continue;
         }
         // No break due to continue

         // Format 4

         case ADD:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);

            temp = update_add_flags(temp, temp2, 0);
         }
         break;

         case CMP:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);
            CompareCommon(temp, temp2);
            continue;
         }
         // No break due to continue

         case BIC:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);
            temp &= ~temp2;
         }
         break;

         case ADDC:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);

            temp3 = C_FLAG;
            temp = update_add_flags(temp, temp2, temp3);
         }
         break;

         case MOV:
         {
            temp = ReadGen(0);
         }
         break;

         case OR:
         {
            temp = ReadGen(0);
            temp2 = ReadGen(1);
            temp |= temp2;
         }
         break;

         case SUB:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);
            update_sub_flags(temp, temp2, 0);
            temp -= temp2;
         }
         break;

         case SUBC:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);
            temp3 = C_FLAG;
            update_sub_flags(temp, temp2, temp3);
            temp -= temp2;
            temp -= temp3;
         }
         break;

         case ADDR:
         {
            temp = ReadAddress(0);
         }
         break;

         case AND:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);
            temp &= temp2;
         }
         break;

         case TBIT:
         {
            temp2 = BitPrefix();
            temp = ReadGen(1);
            F_FLAG = TEST(temp & temp2);
            continue;
         }
         // No break due to continue

         case XOR:
         {
            temp = ReadGen(0);
            temp2 = ReadGen(1);
            temp ^= temp2;
         }
         break;

         // Format 5

         case MOVS:
         {
            if (r[0] == 0)
            {
               F_FLAG = 0;
               continue;
            }

            temp = read_n(r[1], OpSize.Op[0]);

            if (opcode & BIT(15)) // Translating
            {
               temp = read_x8(r[3] + temp); // Lookup the translation
            }

            if (StringMatching(opcode, temp))
            {
               continue;
            }

            write_Arbitary(r[2], &temp, OpSize.Op[0]);

            StringRegisterUpdate(opcode);
            pc = startpc; // Not finsihed so come back again!
            continue;
         }
         // No break due to continue

         case CMPS:
         {
            if (r[0] == 0)
            {
               F_FLAG = 0;
               continue;
            }

            temp = read_n(r[1], OpSize.Op[0]);

            if (opcode & BIT(15)) // Translating
            {
               temp = read_x8(r[3] + temp);                               // Lookup the translation
            }

            if (StringMatching(opcode, temp))
            {
               continue;
            }

            temp2 = read_n(r[2], OpSize.Op[0]);

            if (CompareCommon(temp, temp2) == 0)
            {
               continue;
            }

            StringRegisterUpdate(opcode);
            pc = startpc;                                               // Not finsihed so come back again!
            continue;
         }
         // No break due to continue

         case SETCFG:
         {
            nscfg = opcode;                                             // Store the whole opcode as this includes the options
            continue;
         }
         // No break due to continue

         case SKPS:
         {
            if (r[0] == 0)
            {
               F_FLAG = 0;
               continue;
            }

            temp = read_n(r[1], OpSize.Op[0]);

            if (opcode & BIT(Translation))
            {
               temp = read_x8(r[3] + temp); // Lookup the translation
               write_x8(r[1], temp); // Write back
            }

            if (StringMatching(opcode, temp))
            {
               continue;
            }

            StringRegisterUpdate(opcode);
            pc = startpc; // Not finsihed so come back again!
            continue;
         }
         // No break due to continue

         // Format 6

         case ROT:
         {
            temp2 = ReadGen(0);
            temp  = ReadGen(1);
 
#if 1
            temp3 = OpSize.Op[1] * 8;                             // Bit size, compiler will switch to a shift all by itself ;)

            if (temp2 & 0xE0)
            {
               temp2 |= 0xE0;
               temp2 = ((temp2 ^ 0xFF) + 1);
               temp2 = temp3 - temp2;
            }
            temp = (temp << temp2) | (temp >> (temp3 - temp2));

#else
            switch (OpSize.Op[1])
            {
               case sz8:
               {
                  if (temp2 & 0xE0)
                  {
                     temp2 |= 0xE0;
                     temp2 = ((temp2 ^ 0xFF) + 1);
                     temp2 = 8 - temp2;
                  }
                  temp = (temp << temp2) | (temp >> (8 - temp2));
               }
               break;

               case sz16:
               {
                  if (temp2 & 0xE0)
                  {
                     temp2 |= 0xE0;
                     temp2 = ((temp2 ^ 0xFF) + 1);
                     temp2 = 16 - temp2;
                  }
                  temp = (temp << temp2) | (temp >> (16 - temp2));
               }
               break;

               case sz32:
               {
                  if (temp2 & 0xE0)
                  {
                     temp2 |= 0xE0;
                     temp2 = ((temp2 ^ 0xFF) + 1);
                     temp2 = 32 - temp2;
                  }
                  temp = (temp << temp2) | (temp >> (32 - temp2));
               }
               break;
            }
#endif

            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case ASH:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);

            // Test if the shift is negative (i.e. a right shift)
            if (temp2 & 0xE0)
            {
               temp2 |= 0xE0;
               temp2 = ((temp2 ^ 0xFF) + 1);
               if (OpSize.Op[1] == sz8)
               {
                  // Test if the operand is also negative
                  if (temp & 0x80)
                  {
                     // Sign extend in a portable way
                     temp = (temp >> temp2) | ((0xFF >> temp2) ^ 0xFF);
                  }
                  else
                  {
                     temp = (temp >> temp2);
                  }
               }
               else if (OpSize.Op[1] == sz16)
               {
                  if (temp & 0x8000)
                  {
                     temp = (temp >> temp2) | ((0xFFFF >> temp2) ^ 0xFFFF);
                  }
                  else
                  {
                     temp = (temp >> temp2);
                  }
               }
               else
               {
                  if (temp & 0x80000000)
                  {
                     temp = (temp >> temp2) | ((0xFFFFFFFF >> temp2) ^ 0xFFFFFFFF);
                  }
                  else
                  {
                     temp = (temp >> temp2);
                  }
               }
            }
            else
               temp <<= temp2;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case LSH:
         {
            temp2 = ReadGen(0);
            temp = ReadGen(1);

            if (temp2 & 0xE0)
            {
               temp2 |= 0xE0;
               temp >>= ((temp2 ^ 0xFF) + 1);
            }
            else
               temp <<= temp2;

            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case CBIT:
         case CBITI:
         {
            // The CBITI instructions, in addition, activate the Interlocked
            // Operation output pin on the CPU, which may be used in multiprocessor systems to
            // interlock accesses to semaphore bits. This aspect is not implemented here.
            temp2 = BitPrefix();
            temp = ReadGen(1);
            F_FLAG = TEST(temp & temp2);
            temp &= ~(temp2);
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case SBIT:
         case SBITI:
         {
            // The SBITI instructions, in addition, activate the Interlocked
            // Operation output pin on the CPU, which may be used in multiprocessor systems to
            // interlock accesses to semaphore bits. This aspect is not implemented here.
            temp2 = BitPrefix();
            temp = ReadGen(1);
            F_FLAG = TEST(temp & temp2);
            temp |= temp2;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case NEG:
         {
            temp = 0;
            temp2 = ReadGen(0);
            update_sub_flags(temp, temp2, 0);
            temp -= temp2;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case NOT:
         {
            temp = ReadGen(0);
            temp ^= 1;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case SUBP:
         {
            uint32_t carry = C_FLAG;
            temp2 = ReadGen(0);
            temp = ReadGen(1);
            temp = bcd_sub(temp, temp2, OpSize.Op[0], &carry);
            C_FLAG = TEST(carry);
            F_FLAG = 0;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case ABS:
         {
            temp = ReadGen(0);
            switch (OpSize.Op[0])
            {
               case sz8:
               {
                  if (temp == 0x80)
                  {
                     F_FLAG = 1;
                  }
                  if (temp & 0x80)
                  {
                     temp = (temp ^ 0xFF) + 1;
                  }
               }
               break;

               case sz16:
               {
                  if (temp == 0x8000)
                  {
                     F_FLAG = 1;
                  }
                  if (temp & 0x8000)
                  {
                     temp = (temp ^ 0xFFFF) + 1;
                  }
               }
               break;

               case sz32:
               {
                  if (temp == 0x80000000)
                  {
                     F_FLAG = 1;
                  }
                  if (temp & 0x80000000)
                  {
                     temp = (temp ^ 0xFFFFFFFF) + 1;
                  }
               }
               break;
            }
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case COM:
         {
            temp = ReadGen(0);
            temp = ~temp;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         // TODO, could merge this with TBIT if Format4 operand indexes were consistent (dst == index 0)
         case IBIT:
         {
            temp2 = BitPrefix();
            temp = ReadGen(1);
            F_FLAG = TEST(temp & temp2);
            temp ^= temp2;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case ADDP:
         {
            uint32_t carry = C_FLAG;
            temp2 = ReadGen(0);
            temp = ReadGen(1);
            temp = bcd_add(temp, temp2, OpSize.Op[0], &carry);
            C_FLAG = TEST(carry);
            F_FLAG = 0;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;
 
         // FORMAT 7

         case MOVM:
         {
            uint32_t First    = ReadAddress(0);
            uint32_t Second   = ReadAddress(1);
            temp = getdisp() + OpSize.Op[0];                      // disp of 0 means move 1 byte
            while (temp)
            {
               temp2 = read_x8(First);
               First++;
               write_x8(Second, temp2);
               Second++;
               temp--;
            }

            continue;
         }
         // No break due to continue

         case CMPM:
         {
            uint32_t temp4    = OpSize.Op[0];                                 // disp of 0 means move 1 byte/word/dword
            uint32_t First    = ReadAddress(0);
            uint32_t Second   = ReadAddress(1);

            temp3 = (getdisp() / temp4) + 1;

            //PiTRACE("CMP Size = %u Count = %u\n", temp4, temp3);
            while (temp3--)
            {
               temp  = read_n(First, temp4);
               temp2 = read_n(Second, temp4);
 
               if (CompareCommon(temp, temp2) == 0)
               {
                  break;
               }

               First += temp4;
               Second += temp4;
            }

            continue;
         }
         // No break due to continue

         case INSS:
         {
            uint32_t c;

            // Read the immediate offset (3 bits) / length - 1 (5 bits) from the instruction
            temp3 = READ_PC_BYTE();
            temp = ReadGen(0); // src operand
            temp2 = ReadGen(1); // base operand
            for (c = 0; c <= (temp3 & 0x1F); c++)
            {
               temp2 &= ~(BIT((c + (temp3 >> 5)) & 31));
               if (temp & BIT(c))
               {
                  temp2 |= BIT((c + (temp3 >> 5)) & 31);
               }
            }
            temp = temp2;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case EXTS:
         {
            uint32_t c;
            uint32_t temp4 = 1;

            // Read the immediate offset (3 bits) / length - 1 (5 bits) from the instruction
            temp3 = READ_PC_BYTE();
            temp = ReadGen(0);
            temp2 = 0;
            temp >>= (temp3 >> 5); // Shift by offset
            temp3 &= 0x1F; // Mask off the lower 5 Bits which are number of bits to extract

            for (c = 0; c <= temp3; c++)
            {
               if (temp & temp4) // Copy the ones
               {
                  temp2 |= temp4;
               }

               temp4 <<= 1;
            }
            temp = temp2;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case MOVXBW:
         {
            OpSize.Op[0] = sz8;
            temp = ReadGen(0);
            SIGN_EXTEND(sz8, temp); // Editor need the useless semicolon
            WriteSize = sz16;
            WriteIndex = 1;
         }
         break;

         case MOVZBW:
         {
            OpSize.Op[0] = sz8;
            temp = ReadGen(0);
            WriteSize = sz16;
            WriteIndex = 1;
         }
         break;

         case MOVZiD:
         {
            temp = ReadGen(0);
            WriteSize = sz32;
            WriteIndex = 1;
         }
         break;

         case MOVXiD:
         {
            temp = ReadGen(0);
            SIGN_EXTEND(OpSize.Op[0], temp);
            WriteSize = sz32;
            WriteIndex = 1;
         }
         break;

         case MEI:
         {
            temp = ReadGen(0); // src
            uint64_t temp64 = ReadGen(1); // dst
            temp64 *= temp;
            // Handle the writing to the upper half of dst locally here
            handle_mei_dei_upper_write(temp64);
            // Allow fallthrough write logic to write the lower half of dst
            temp = (uint32_t) temp64;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case MUL:
         {
            temp = ReadGen(0);
            temp2 = ReadGen(1);
            temp *= temp2;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case DEI:
         {
            int size = OpSize.Op[0] << 3;                      // 8, 16  or 32 
            temp = ReadGen(0); // src
            if (temp == 0)
            {
               GOTO_TRAP(DivideByZero);
            }

            uint64_t temp64 = readgenq(1); // dst
            switch (OpSize.Op[0])
            {
               case sz8:
                  temp64 = ((temp64 >> 24) & 0xFF00) | (temp64 & 0xFF);
                  break;

               case sz16:
                  temp64 = ((temp64 >> 16) & 0xFFFF0000) | (temp64 & 0xFFFF);
                  break;
            }
            // PiTRACE("temp = %08x\n", temp);
            // PiTRACE("temp64 = %016" PRIu64 "\n", temp64);
            temp64 = ((temp64 / temp) << size) | (temp64 % temp);
            //PiTRACE("result = %016" PRIu64 "\n", temp64);
            // Handle the writing to the upper half of dst locally here
            handle_mei_dei_upper_write(temp64);
            // Allow fallthrough write logic to write the lower half of dst
            temp = (uint32_t) temp64;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case QUO:
         {
            temp = ReadGen(0);
            temp2 = ReadGen(1);
            if (temp == 0)
            {
               GOTO_TRAP(DivideByZero);
            }

            switch (OpSize.Op[0])
            {
               case sz8:
                  temp = (int8_t) temp2 / (int8_t) temp;
               break;

               case sz16:
                  temp = (int16_t) temp2 / (int16_t) temp;
               break;

               case sz32:
                  temp = (int32_t) temp2 / (int32_t) temp;
               break;
            }
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case REM:
         {
            temp = ReadGen(0);
            temp2 = ReadGen(1);
            if (temp == 0)
            {
               GOTO_TRAP(DivideByZero);
            }

            switch (OpSize.Op[0])
            {
               case sz8:
                  temp = (int8_t) temp2 % (int8_t) temp;
               break;

               case sz16:
                  temp = (int16_t) temp2 % (int16_t) temp;
               break;

               case sz32:
                  temp = (int32_t) temp2 % (int32_t) temp;
               break;
            }
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case MOD:
         {
            temp = ReadGen(0);
            temp2 = ReadGen(1);
            if (temp == 0)
            {
               GOTO_TRAP(DivideByZero);
            }

            temp = mod_operator(temp2, temp);
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case DIV:
         {
            temp = ReadGen(0);
            temp2 = ReadGen(1);
            if (temp == 0)
            {
               GOTO_TRAP(DivideByZero);
            }

            temp = div_operator(temp2, temp);
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         // Format 8
 
         case EXT:
         {
            uint32_t c;
         
            int32_t Offset = r[(opcode >> 11) & 7];                              // Offset
            temp2          = getdisp();                                          // Length
            OpSize.Op[0] = sz32;
            temp3          = ReadGen(0);                                         // Base
            temp           = 0;                                                  // Result starts at zero

            if (gentype[1] != Register)                                            // If memory loaction
            {
               genaddr[WriteIndex] += Offset / 8;                                // Cast to signed as negative is allowed
               Offset %= 8;                                                      // Offset within te first byte
            }

            for (c = 0; c < temp2; c++)
            {
               if (temp3 & BIT((c + Offset) & 31))
                  temp |= BIT(c);
            }

            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case CVTP:
         {
            int32_t Offset = r[(opcode >> 11) & 7];
            int32_t Base = ReadAddress(0);

            temp = (Base * 8) + Offset;
            WriteSize = sz32;
            WriteIndex = 1;
         }
         break;

         case INS:
         {
            int c;
            int32_t Offset = r[(opcode >> 11) & 7] % 8;
            int32_t Length = getdisp();
            int32_t Source = ReadGen(0);
            int32_t Base = ReadGen(1);

            if (gentype[1] != Register)                                         // If memory loaction
            {
               genaddr[WriteIndex] += Offset / 8;                               // Cast to signed as negative is allowed
               Offset %= 8;                                                     // Offset within te first byte
            }

            for (c = 0; c < Length; c++)
            {
               if ((c + Offset) > 31)
               {
                  break;
               }

               if (Source & BIT(c))
               {
                  Base |= BIT(c + Offset);
               }
               else
               {
                  Base &= ~(BIT(c + Offset));
               }
            }

            temp = Base;
            WriteSize = OpSize.Op[1];
            WriteIndex = 1;
         }
         break;

         case CHECK:
         {
            uint32_t ad = ReadAddress(0);
            temp3 = ReadGen(1);

            // Avoid a "might be uninitialized" warning
            temp2 = 0;
            switch (OpSize.Op[0])
            {
               case sz8:
               {
                  temp = read_x8(ad);
                  temp2 = read_x8(ad + 1);
               }
               break;

               case sz16:
               {
                  temp = read_x16(ad);
                  temp2 = read_x16(ad + 2);
               }
               break;

               case sz32:
               {
                  temp = read_x32(ad);
                  temp2 = read_x32(ad + 4);
               }
               break;
            }

            //PiTRACE("Reg = %u Bounds [%u - %u] Index = %u", 0, temp, temp2, temp3);

            if ((temp >= temp3) && (temp3 >= temp2))
            {
               r[(opcode >> 11) & 7] = temp3 - temp2;
               F_FLAG = 0;
            }
            else
            {
               F_FLAG = 1;
            }

            continue;
         }
         // No break due to continue

         case INDEX:
         {
            // r0, r1, r2
            // 5, 7, 0x13 (19)
            // accum = accum * (length+1) + index

            temp = r[(opcode >> 11) & 7]; // Accum
            temp2 = ReadGen(0) + 1; // (length+1)
            temp3 = ReadGen(1); // index

            r[(opcode >> 11) & 7] = (temp * temp2) + temp3;
            // No break due to continue
         }
         // No break due to continue

         case FFS:
         {
            uint32_t numbits = OpSize.Op[0] << 3;          // number of bits: 8, 16 or 32
            temp2 = ReadGen(0); // base is the variable size operand being scanned
            OpSize.Op[1] = sz8;
            temp = ReadGen(1); // offset is always 8 bits (also the result)
            // find the first set bit, starting at offset
            for (; temp < numbits && !(temp2 & BIT(temp)); temp++)
            {
               continue;                  // No Body!
            }

            if (temp < numbits)
            {
               // a set bit was found, return it in the offset operand
               F_FLAG = 0;
            }
            else
            {
               // no set bit was found, return 0 in the offset operand
               F_FLAG = 1;
               temp = 0;
            }

            WriteSize = sz8;
            WriteIndex = 1;
         }
         break;

         default:
         {
            if (Function < TRAP)
            {
               GOTO_TRAP(UnknownInstruction);
            }

            GOTO_TRAP(UnknownFormat);         // Probably already set but belt and braces here!
         }
         // No break due to goto
      }

      if (WriteSize && (WriteSize <= sz32))
      {
         switch (gentype[WriteIndex])
         {
            case Memory:
            {
               switch (WriteSize)
               {
                  case sz8:   write_x8( genaddr[WriteIndex], temp);  break;
                  case sz16:  write_x16(genaddr[WriteIndex], temp);  break;
                  case sz32:  write_x32(genaddr[WriteIndex], temp);  break;
               }
            }
            break;
            
            case Register:
            {
               switch (WriteSize)
               {
                  case sz8:   *((uint8_t*)   genaddr[WriteIndex]) = temp;  break;
                  case sz16:  *((uint16_t*)  genaddr[WriteIndex]) = temp;  break;
                  case sz32:  *((uint32_t*)  genaddr[WriteIndex]) = temp;  break;
               }

               ShowRegisterWrite(WriteIndex, Truncate(temp, WriteSize));
            }
            break;

            case TOS:
            {
               PushArbitary(temp, WriteSize);
            }
            break;

            case OpImmediate:
            {
               GOTO_TRAP(IllegalWritingImmediate);
               goto DoTrap;
            }
         }
      }
   }
}
