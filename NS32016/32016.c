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
#include "PandoraV0_61.h"

#define TEST_SUITE

int nsoutput = 0;
int nscfg;

uint32_t Trace = 0;
uint32_t tube_irq = 0;
uint32_t r[8];
uint32_t pc, sp[2], fp, sb, intbase;
uint16_t psr, mod;
uint32_t startpc;

uint32_t genaddr[2];
int gentype[2];

DecodeMatrix mat[256];
DecodeMatrix LookUp;

#define SP ((psr & S_FLAG) >> 9)

#define SIGN_EXTEND(reg) \
  if ((LookUp.p.Size == sz8) && (reg & 0x80)) { \
    reg |= 0xFFFFFF00; \
  } else if ((LookUp.p.Size == sz16) && (reg & 0x8000)) { \
    reg |= 0xFFFF0000; \
  }

#define NIBBLE_EXTEND(reg) \
   if (reg & 0x08) \
      reg |= 0xFFFFFFF0;

void n32016_reset(uint32_t StartAddress)
{
	pc = StartAddress;
	psr = 0;

	n32016_build_matrix();
}

void dump_mini(void)
{
   if (Trace)
   {
      printf("R0=%08X R1=%08X R2=%08X R3=%08X\n", r[0], r[1], r[2], r[3]);
      printf("R4=%08X R5=%08X R6=%08X R7=%08X\n", r[4], r[5], r[6], r[7]);
      printf("PC=%08X SB=%08X SP0=%08X SP1=%08X\n", pc, sb, sp[0], sp[1]);
      printf("FP=%08X INTBASE=%08X PSR=%04X MOD=%04X\n", fp, intbase, psr, mod);
      printf("\n");
   }
}

void n32016_dumpregs(char* pMessage)
{
   printf("%s\n", pMessage);
   printf("R0=%08X R1=%08X R2=%08X R3=%08X\n", r[0], r[1], r[2], r[3]);
   printf("R4=%08X R5=%08X R6=%08X R7=%08X\n", r[4], r[5], r[6], r[7]);
   printf("PC=%08X SB=%08X SP0=%08X SP1=%08X\n", pc, sb, sp[0], sp[1]);
   printf("FP=%08X INTBASE=%08X PSR=%04X MOD=%04X\n", fp, intbase, psr, mod);

#ifdef WIN32
   system("pause");
#endif

   exit(1);
}

static void pushw(uint16_t val)
{
   sp[SP] -= 2;
   write_x16(sp[SP], val);
}

static void pushd(uint32_t val)
{
   sp[SP] -= 4;

   write_x32(sp[SP], val);
}

static uint16_t popw()
{
   uint16_t temp = read_x16(sp[SP]);
   sp[SP] += 2;

   return temp;
}

static uint32_t popd()
{
  uint32_t temp = read_x32(sp[SP]);
  sp[SP] += 4;

  return temp;
}

static uint32_t getdisp()
{
  uint32_t addr = read_x8(pc);
  pc++;
  if (!(addr & 0x80))
  {
    return addr | ((addr & 0x40) ? 0xFFFFFF80 : 0);
  }

  if (!(addr & 0x40))
  {
    addr &= 0x3F;
    addr = (addr << 8) | read_x8(pc);
    pc++;

    return addr | ((addr & 0x2000) ? 0xFFFFC000 : 0);
  }

  addr &= 0x3F;
  addr = (addr << 24) | (read_x8(pc) << 16);
  pc++;
  addr = addr | (read_x8(pc) << 8);
  pc++;
  addr = addr | read_x8(pc);
  pc++;

  return addr | ((addr & 0x20000000) ? 0xC0000000 : 0);
}

int genindex[2];
int sdiff[2] =
{ 0, 0 };

uint32_t nsimm[2];

uint32_t Truncate(uint32_t Value, uint32_t Size)
{
   switch (Size)
   {
      case sz8:      return Value & 0x000000FF;
      case sz16:     return Value & 0x0000FFFF;
   }

   return Value;
}

uint32_t ReadGen(uint32_t c, uint32_t Size)
{
   uint32_t Temp = 0;

   if (gentype[c])
   {
      Temp = *(uint32_t*) genaddr[c];
   }
   else
   {
      Temp = read_x32(genaddr[c]);

      if (sdiff[c])
      {
         genaddr[c]  =
         sp[SP]      = (sp[SP] + sdiff[c]);
      }
   }

   return Truncate(Temp, Size);
}

static uint32_t getgen(int gen, int c)
{
   uint32_t temp, temp2;

   gen &= 0x1F;

   if ((gen & 0x1C) == 0x1C)
   {
      genindex[c] = read_x8(pc);
      pc++;
   }

   StoreRegisters(c, gen);

   switch (gen)
   {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
         gentype[c] = 1;
         genaddr[c] = (uint32_t) &r[gen & 7];
         break;

      case 8:
      case 9:
      case 0xA:
      case 0xB:
      case 0xC:
      case 0xD:
      case 0xE:
      case 0xF:
         gentype[c] = 0;
         genaddr[c] = r[gen & 7] + getdisp();
         break;

      case 0x10: // Frame memory relative
         temp = getdisp();
         temp2 = getdisp();
         genaddr[c] = read_x32(fp + temp);
         genaddr[c] += temp2;
         gentype[c] = 0;
         break;

      case 0x11: // Stack memory relative
         temp = getdisp();
         temp2 = getdisp();
         genaddr[c] = read_x32(sp[SP] + temp);
         genaddr[c] += temp2;
         gentype[c] = 0;
         break;

      case 0x12: // Static memory relative
         temp = getdisp();
         temp2 = getdisp();
         genaddr[c] = read_x32(sb + temp);
         genaddr[c] += temp2;
         gentype[c] = 0;
         break;

      case 0x14: // Immediate
      {
         gentype[c] = 0;
         gentype[c] = 1;
         genaddr[c] = (uint32_t) & nsimm[c];
         // Why can't they just decided on an endian and then stick to it?
         if (LookUp.p.Size == sz8)
            nsimm[c] = read_x8(pc);
         else if (LookUp.p.Size == sz16)
            nsimm[c] = (read_x8(pc) << 8) | read_x8(pc + 1);
         else
            nsimm[c] = (read_x8(pc) << 24) | (read_x8(pc + 1) << 16) | (read_x8(pc + 2) << 8) | read_x8(pc + 3);
         pc += (LookUp.p.Size + 1);
      }
      break;

      case 0x15: // Absolute
         gentype[c] = 0;
         genaddr[c] = getdisp();
         break;

      case 0x16: // External
         gentype[c] = 0;
         temp = read_x32(mod + 4);
         temp += getdisp();
         temp2 = read_x32(temp);
         genaddr[c] = temp2 + getdisp();
         break;

      case 0x17: // Stack
         gentype[c] = 0;
         sdiff[c] = (LookUp.p.Size + 1);
         genaddr[c] = sp[SP];
         /*                if (c)
          {
          sp[SP] -= (LookUp.p.Size + 1);
          genaddr[c]=sp[SP];
          }
          else
          {
          genaddr[c]=sp[SP];
          sp[SP] += (LookUp.p.Size + 1);
          }*/
         break;

      case 0x18: // FP relative
         gentype[c] = 0;
         genaddr[c] = getdisp() + fp;
         break;
      case 0x19: // SP relative
         gentype[c] = 0;
         genaddr[c] = getdisp() + sp[SP];
         break;
      case 0x1A: // SB relative
         gentype[c] = 0;
         genaddr[c] = getdisp() + sb;
         break;
      case 0x1B: // PC relative
         gentype[c] = 0;
         genaddr[c] = getdisp() + startpc;
         break;

      case 0x1C: // EA + Rn
      case 0x1D: // EA + Rn*2
      case 0x1E: // EA + Rn*4
      case 0x1F: // EA + Rn*8
      {
         uint32_t Shift = gen & 3;
         getgen(genindex[c] >> 3, c);
         if (!gentype[c])
         {
            genaddr[c] += (r[genindex[c] & 7] << Shift);
         }
         else
         {
            genaddr[c] = *(uint32_t*) genaddr[c] + (r[genindex[c] & 7] << Shift);
         }

         gentype[c] = 0;
      }
      break;

      default:
         n32016_dumpregs("Bad NS32016 gen mode");
         break;
   }

   return ReadGen(c, LookUp.p.Size);
}

uint64_t readgenq(uint32_t c)
{
   uint64_t temp;

   if (gentype[c])
   {
      temp = *(uint64_t *) genaddr[c];
   }
   else
   {
      temp = read_x32(genaddr[c]);
      if (sdiff[c])
         genaddr[c] = sp[SP] = sp[SP] + sdiff[c];
      temp |= read_x32(genaddr[c]);
      if (sdiff[c])
      {
         genaddr[c] = sp[SP] = sp[SP] + sdiff[c];
      }
   }

   return temp;
}

static uint16_t oldpsr;

// From: http://homepage.cs.uiowa.edu/~jones/bcd/bcd.html
static uint32_t bcd_add_16(uint32_t a, uint32_t b, uint32_t *carry) {
  uint32_t  t1, t2;              // unsigned 32-bit intermediate values
  //printf("bcd_add_16: in  %08x %08x %08x\n", a, b, *carry);
  if (*carry) {
    b++;                         // I'm 90% sure its OK to handle carry this way
  }                              // i.e. its OK if the ls digit of b becomes A
  t1 = a + 0x06666666;
  t2 = t1 ^ b;                   // sum without carry propagation
  t1 = t1 + b;                   // provisional sum
  t2 = t1 ^ t2;                  // all the binary carry bits
  t2 = ~t2 & 0x11111110;         // just the BCD carry bits
  t2 = (t2 >> 2) | (t2 >> 3);    // correction
  t2 = t1 - t2;                  // corrected BCD sum
  *carry = (t2 & 0xFFFF0000) ? 1 : 0;
  t2 &= 0xFFFF;
  //printf("bcd_add_16: out %08x %08x\n", t2, *carry);
  return t2;
}

static uint32_t bcd_sub_16(uint32_t a, uint32_t b, uint32_t *carry) {
   uint32_t  t1, t2;              // unsigned 32-bit intermediate values
   //printf("bcd_sub_16: in  %08x %08x %08x\n", a, b, *carry);
   if (*carry) {
      b++;
   }
   *carry = 0;
   t1 = 0x9999 - b;
   t2 = bcd_add_16(t1, 1, carry);
   t2 = bcd_add_16(a, t2, carry);
   *carry = 1 - *carry;
   //printf("bcd_add_16: out %08x %08x\n", t2, *carry);
   return t2;
}

static uint32_t bcd_add(uint32_t a, uint32_t b, int size, uint32_t *carry) {
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

static uint32_t bcd_sub(uint32_t a, uint32_t b, int size, uint32_t *carry) {
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

static void update_add_flags(uint32_t a, uint32_t b, uint32_t cin)
{
   // TODO: Check the carry logic here is correct
   // I suspect there is a corner case where b=&FFFFFFFF and cin=1
   uint32_t sum = a + b + cin;
   psr &= ~(C_FLAG | F_FLAG);
   switch (LookUp.p.Size)
   {
      case sz8:
      {
         if (sum & 0x100)
            psr |= C_FLAG;
         if ((a ^ sum) & (b ^ sum) & 0x80)
            psr |= F_FLAG;
      }
      break;

      case sz16:
      {
         if (sum & 0x10000)
            psr |= C_FLAG;
         if ((a ^ sum) & (b ^ sum) & 0x8000)
            psr |= F_FLAG;
      }
      break;

      case sz32:
      {
         if (sum < a)
            psr |= C_FLAG;
         if ((a ^ sum) & (b ^ sum) & 0x80000000)
            psr |= F_FLAG;
      }
      break;
   }
   //printf("ADD FLAGS: C=%d F=%d\n", (psr & C_FLAG) ? 1 : 0, (psr & F_FLAG) ? 1 : 0);
}

static void update_sub_flags(uint32_t a, uint32_t b, uint32_t cin)
{
   // TODO: Check the carry logic here is correct
   // I suspect there is a corner case where b=&FFFFFFFF and cin=1
   uint32_t diff = a - b - cin;
   psr &= ~(C_FLAG | F_FLAG);
   if (diff > a)
      psr |= C_FLAG;
   if ((b ^ a) & (b ^ diff) & 0x80000000)
      psr |= F_FLAG;
   //printf("SUB FLAGS: C=%d F=%d\n", (psr & C_FLAG) ? 1 : 0, (psr & F_FLAG) ? 1 : 0);
}

// The difference between DIV and QUO occurs when the result (the quotient) is negative
// e.g. -16 QUO 3 ===> -5
// e.g. -16 DIV 3 ===> -6
// i.e. DIV rounds down to the more negative nu,ber
// This case is detected if the sign bits of the two operands differ
static uint32_t div_operator(uint32_t a, uint32_t b)
{
   uint32_t ret;
   int signmask = BIT((LookUp.p.Size << 3) + 7);
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
   switch (LookUp.p.Size)
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

static void handle_mei_dei_upper_write(uint64_t result)
{
   uint32_t temp;
   // Handle the writing to the upper half of dst locally here
   switch (LookUp.p.Size)
   {
      case sz8:
         temp = result >> 8;
         if (gentype[1])
            *(uint8_t *) (genaddr[1] + 4) = temp;
         else
            write_x8(genaddr[1] + 4, temp);
         break;

      case sz16:
         temp = result >> 16;
         if (gentype[1])
            *(uint16_t *) (genaddr[1] + 4) = temp;
         else
            write_x16(genaddr[1] + 4, temp);
         break;

      case sz32:
         temp = result >> 32;
         if (gentype[1])
            *(uint32_t *) (genaddr[1] + 4) = temp;
         else
            write_x32(genaddr[1] + 4, temp);
         break;
   }
}

uint32_t CompareCommon(uint32_t temp, uint32_t temp2)
{
   psr &= ~(Z_FLAG | N_FLAG | L_FLAG);
   if (temp2 > temp)
      psr |= L_FLAG;

   if (LookUp.p.Size == sz8)
   {
      if (((signed char) temp2) > ((signed char) temp))
      {
         psr |= N_FLAG;
      }
   }
   else if (LookUp.p.Size == sz32)
   {
      if (((signed long) temp2) > ((signed long) temp))
      {
         psr |= N_FLAG;
      }
   }

   if (temp == temp2)
   {
      psr |= Z_FLAG;
      return 1;
   }

   return 0;
}

uint32_t StringPart2(uint32_t opcode, uint32_t Value)
{
   uint32_t Options = (opcode >> 17) & 3;

   Trace = 1;

   if (Options)
   {
      uint32_t Compare = Truncate(r[4], LookUp.p.Size);

      if (Options == 1)                                  // While match
      {
         if (Value != Compare)
         {
            psr |= F_FLAG;                               // Set PSR F Bit
            return 0;
         }   
      }

      if (Options == 3)                                  // Until Match
      {
         if (Value == Compare)
         {
            psr |= F_FLAG;                               // Set PSR F Bit
            return 0;
         }
      }
   }

   uint32_t Size = (LookUp.p.Size + 1);

   if (opcode & BIT(Backwards))                          // Adjust R1
   {
      r[1] -= Size;
   }
   else
   {
      r[1] += Size;
   }

   if (((opcode >> 10) & 0x0F) != (SKPS & 0x0F))
   {
      if (opcode & BIT(Backwards))                       // Adjust R2 for all but SKPS
      {
         r[2] -= Size;
      }
      else
      {
         r[2] += Size;
      }
   }

   r[0]--;                                               // Adjust R0
   
   return 1;
}

void n32016_exec(uint32_t tubecycles)
{
   uint32_t opcode, WriteSize, WriteIndex;
   uint32_t temp = 0, temp2, temp3, temp4;
   uint64_t temp64;


   while (tubecycles > 0)
   {
      sdiff[0] = sdiff[1] = 0;
      startpc = pc;
      ClearRegs();
      opcode = read_x32(pc);

      //if (startpc == 0x1C11)
      //{
      //   Trace = 1;
      //}

      LookUp = mat[opcode & 0xFF];
      pc += LookUp.p.BaseSize;
      WriteSize = szVaries;
      WriteIndex = 0; // default to writing operand 0

      switch (LookUp.p.Function >> 4)
      {
         case Format0:
         {
            temp = startpc + getdisp();
         }
         break;

         case Format2:
         {
            getgen(opcode >> 11, 0);
         }
         break;

         case Format3:
         {
            LookUp.p.Function += ((opcode >> 7) & 0x0F);
            getgen(opcode >> 11, 0);
         }
         break;

         case Format4:
         {
            getgen(opcode >> 11, 1);
            getgen(opcode >> 6, 0);
         }
         break;

         case Format5:
         {
            LookUp.p.Function += ((opcode >> 10) & 0x0F);
            LookUp.p.Size = (opcode & BIT(Translation)) ? sz8 : ((opcode >> 8) & 3);
            temp2 = (opcode >> 15) & 0xF;
         }
         break;

         case Format6:
         {
            LookUp.p.Function += ((opcode >> 10) & 0x0F);

            // Ordering important here, as getgen uses LookUp.p.Size
            switch (LookUp.p.Function)
            {
               case ROT:
               case ASH:
               case LSH:
               {
                  LookUp.p.Size = sz8;
               }
               break;

               default:
               {
                  LookUp.p.Size = (opcode >> 8) & 3;
               }
               break;
            }

            getgen(opcode >> 19, 0);
            LookUp.p.Size = (opcode >> 8) & 3;
            getgen(opcode >> 14, 1);
         }
         break;

         case Format7:
         {
            LookUp.p.Function += ((opcode >> 10) & 0x0F);
            LookUp.p.Size = (opcode >> 8) & 3;

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
                        LookUp.p.Function = MOVUS;
                     }
                     break;

                     case 0x1C80:
                     {
                        LookUp.p.Function = MOVSU;
                     }
                     break;

                     default:
                     {
                        LookUp.p.Function = TRAP;
                     }
                     break;
                  }
               }
               else
               {
                  LookUp.p.Function = (opcode & 0x40) ? FFS : INDEX;
               }
            }
            else
            {
               LookUp.p.Function += ((opcode >> 6) & 3);
            }

            LookUp.p.Size = (opcode >> 8) & 3;

            if (LookUp.p.Function == CVTP)
            {
               LookUp.p.Size = sz32;
            }

            getgen(opcode >> 19, 0);
            getgen(opcode >> 14, 1);
         }
         break;

         case Format9:
         {
            LookUp.p.Function += ((opcode >> 11) & 0x07);
         }
         break;

         case Format11:
         {
            LookUp.p.Function += ((opcode >> 10) & 0x0F);
         }
         break;

         case Format14:
         {
            LookUp.p.Function += ((opcode >> 10) & 0x0F);
         }
         break;
      }

      ShowInstruction(startpc, opcode, LookUp.p.Function, LookUp.p.Size);

#ifdef TEST_SUITE
      if (startpc == 0x1C95)
      {
         n32016_dumpregs("Test Suite Pass!\n");
      }

      if (startpc == 0x1CAB)
      {
         n32016_dumpregs("Test Suite Failure!\n");
      }
#endif

      switch (LookUp.p.Function)
      {
         case SETCFG:
         {
            nscfg = temp;
         }
         break;

         case ADDQ:
         {
            temp2 = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp2);
            temp = ReadGen(0, LookUp.p.Size);

            update_add_flags(temp, temp2, 0);
            temp += temp2;
            WriteSize = LookUp.p.Size;
         }
         break;

         case ADD: // ADD
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            update_add_flags(temp, temp2, 0);
            temp += temp2;
            WriteSize = LookUp.p.Size;
         }
         break;

         case CMPQ:
         {
            temp2 = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp2);
            temp = ReadGen(0, LookUp.p.Size);
            SIGN_EXTEND(temp);
            CompareCommon(temp, temp2);
         }
         break;

         case SPR:
         {
            switch ((opcode >> 7) & 0xF)
            {
               case 0x8: temp = fp;       break;
               case 0x9: temp = sp[SP];   break;
               case 0xA: temp = sb;       break;
               case 0xF: temp = mod;      break;

               default:
               {
                  n32016_dumpregs("Bad SPR reg");
               }
               break;
            }

            WriteSize = sz32;
         }
         break;

         case Scond:
            temp = 0;
            switch ((opcode >> 7) & 0xF)
            {
               case 0x0:
                  if (psr & Z_FLAG)
                     temp = 1;
                  break;
               case 0x1:
                  if (!(psr & Z_FLAG))
                     temp = 1;
                  break;
               case 0x2:
                  if (psr & C_FLAG)
                     temp = 1;
                  break;
               case 0x3:
                  if (!(psr & C_FLAG))
                     temp = 1;
                  break;
               case 0x4:
                  if (psr & L_FLAG)
                     temp = 1;
                  break;
               case 0x5:
                  if (!(psr & L_FLAG))
                     temp = 1;
                  break;
               case 0x6:
                  if (psr & N_FLAG)
                     temp = 1;
                  break;
               case 0x7:
                  if (!(psr & N_FLAG))
                     temp = 1;
                  break;
               case 0x8:
                  if (psr & F_FLAG)
                     temp = 1;
                  break;
               case 0x9:
                  if (!(psr & F_FLAG))
                     temp = 1;
                  break;
               case 0xA:
                  if (!(psr & (Z_FLAG | L_FLAG)))
                     temp = 1;
                  break;
               case 0xB:
                  if (psr & (Z_FLAG | L_FLAG))
                     temp = 1;
                  break;
               case 0xC:
                  if (!(psr & (Z_FLAG | N_FLAG)))
                     temp = 1;
                  break;
               case 0xD:
                  if (psr & (Z_FLAG | N_FLAG))
                     temp = 1;
                  break;
               case 0xE:
                  temp = 1;
                  break;
               case 0xF:
                  break;
            }
            WriteSize = LookUp.p.Size;
            break;

         case ACB: // ACB
            temp2 = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp2);
            temp = ReadGen(0, LookUp.p.Size);
            temp += temp2;
            WriteSize = LookUp.p.Size;
            temp2 = getdisp();
            if (temp & 0xFF)
               pc = startpc + temp2;
            break;

         case MOVQ:
         {
            temp = (opcode >> 7) & 0xF;
            NIBBLE_EXTEND(temp);
            WriteSize = LookUp.p.Size;
         }
         break;

         case LPR:
         {
            temp = ReadGen(0, LookUp.p.Size);

            if (LookUp.p.Size == sz8)
            {
               switch ((opcode >> 7) & 0xF)
               {
                  case 0:
                     psr = (psr & 0xFF00) | (temp & 0xFF);
                     break;
                  case 9:
                     sp[SP] = temp;
                     break;
                  default:
                     n32016_dumpregs("Bad LPRB reg");
                     break;
               }
            }
            else if (LookUp.p.Size == sz16)
            {
               switch ((opcode >> 7) & 0xF)
               {
                  case 15:
                     mod = temp;
                     break;
                  default:
                     n32016_dumpregs("Bad LPRW reg");
                     break;
               }
               break;
            }
            else
            {
               switch ((opcode >> 7) & 0xF)
               {
                  case 9:
                     sp[SP] = temp;
                     break;
                  case 0xA:
                     sb = temp;
                     break;
                  case 0xE:
                     intbase = temp; // printf("INTBASE %08X %08X\n",temp,pc); 
                     break;

                  default:
                     n32016_dumpregs("Bad LPRD reg");
                     break;
               }
            }
         }
         break;

         case CMP: // CMP
         {
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);

            CompareCommon(temp, temp2);
         }
         break;

         case BIC: // BIC
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);

            temp &= ~temp2;
            WriteSize = LookUp.p.Size;
            break;

         case ADDC: // ADDC 
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            temp3 = (psr & C_FLAG) ? 1 : 0;
            update_add_flags(temp, temp2, temp3);
            temp += temp2;
            temp += temp3;
            WriteSize = LookUp.p.Size;
            break;

         case MOV:
            temp = ReadGen(1, LookUp.p.Size);
            WriteSize = LookUp.p.Size;
            break;

         case OR:
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);
            temp |= temp2;
            WriteSize = LookUp.p.Size;
            break;

         case SUB:
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            update_sub_flags(temp, temp2, 0);
            temp -= temp2;
            WriteSize = LookUp.p.Size;
            break;

         case SUBC:
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            temp3 = (psr & C_FLAG) ? 1 : 0;
            update_sub_flags(temp, temp2, temp3);
            temp -= temp2;
            temp -= temp3;
            WriteSize = LookUp.p.Size;
            break;

         case ADDR:
            temp = genaddr[1];
            WriteSize = LookUp.p.Size;
            break;

         case AND:
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);
            temp &= temp2;
            WriteSize = LookUp.p.Size;
            break;

         case TBIT:
            temp2 = ReadGen(1, LookUp.p.Size);
            if (gentype[0])
            {
               // operand 0 is a register
               temp = ReadGen(0, sz32);
               temp2 &= 31;
            }
            else
            {
               // operand0 is memory
               // TODO: this should probably use the DIV and MOD opersator functions
               genaddr[0] += temp2 / 8;
               temp = ReadGen(0, sz8);
               temp2 %= 8;
            }
            psr &= ~F_FLAG;
            if (temp & BIT(temp2))
               psr |= F_FLAG;
            break;

         case XOR:
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);
            temp ^= temp2;
            WriteSize = LookUp.p.Size;
            break;
 
         case MOVS:
         {
            if (r[0])
            {
               temp4 = (LookUp.p.Size + 1);
               temp = read_n(r[1], LookUp.p.Size);

               if (opcode & BIT(15))                     // Translating
               {
                  temp = read_x8(r[3] + temp);           // Lookup the translation
               }

               if (opcode & BIT(17))                      // While Match
               {
                  if (temp != r[4])
                  {
                     break;
                  }
               }
               else if (opcode & BIT(18))                  // Until Match
               {
                  if (temp == r[4])
                  {
                     break;
                  }
               }

               write_Arbitary(r[2], &temp, temp4);

               if (opcode & BIT(16))                      // Backwards
               {
                  r[1] -= temp4;
                  r[2] -= temp4;
               }
               else
               {
                  r[1] += temp4;
                  r[2] += temp4;
               }

               r[0]--;
            }

            if (r[0] == 0)
            {
               break;
            }

            pc = startpc;           // Not finsihed so come back again!
         }
         break;

         case CMPS:
         {
            if (r[0] == 0)
            {
               psr &= ~F_FLAG;                              // Clear PSR F Bit
               break;
            }

            if (r[0])
            {
               temp4 = (LookUp.p.Size + 1);

               temp = read_n(r[1], LookUp.p.Size);
            
               if (opcode & BIT(15))                     // Translating
               {
                  temp = read_x8(r[3] + temp);           // Lookup the translation
               }

               if (opcode & BIT(17))                      // While Match
               {
                  if (temp != r[4])
                  {
                     break;
                  }
               }
               else if (opcode & BIT(18))                  // Until Match
               {
                  if (temp == r[4])
                  {
                     break;
                  }
               }

               temp2 = read_n(r[2], LookUp.p.Size);

               if (CompareCommon(temp, temp2) == 0)
               {
                  break;
               }

               if (opcode & BIT(16))                      // Backwards
               {
                  r[1] -= temp4;
                  r[2] -= temp4;
               }
               else
               {
                  r[1] += temp4;
                  r[2] += temp4;
               }

               r[0]--;
            }

            if (r[0] == 0)
            {
               psr |= F_FLAG; // Set PSR F Bit
               break;
            }
            
            pc = startpc;           // Not finsihed so come back again!
         }
         break;

         case SKPS:
         {
            if (r[0] == 0)
            {
               psr &= ~F_FLAG;                              // Clear PSR F Bit
               break;
            }

            temp = read_n(r[1], LookUp.p.Size);

            if (opcode & BIT(Translation))
            {
               temp = read_x8(r[3] + temp);                 // Lookup the translation
               write_x8(r[1], temp);                        // Write back
            }

            if (StringPart2(opcode, temp))
            {
               pc = startpc;                                         // Not finsihed so come back again!
            }
         }
         break;

         case ROT:
         {
            temp2 = ReadGen(0, sz8);

            switch (LookUp.p.Size)
            {
               case sz8:
               {
                  if (temp2 & 0xE0)
                  {
                     temp2 |= 0xE0;
                     temp2 = ((temp2 ^ 0xFF) + 1);
                     temp2 = 8 - temp2;
                  }
                  temp = ReadGen(1, sz8);
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
                  temp = ReadGen(1, sz16);
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
                  temp = ReadGen(1, sz32);
                  temp = (temp << temp2) | (temp >> (32 - temp2));
               }
               break;
            }

            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case ASH:
         {
            temp2 = ReadGen(0, sz8);
            temp = ReadGen(1, LookUp.p.Size);
            // Test if the shift is negative (i.e. a right shift)
            if (temp2 & 0xE0)
            {
               temp2 |= 0xE0;
               temp2 = ((temp2 ^ 0xFF) + 1);
               if (LookUp.p.Size == sz8)
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
               else if (LookUp.p.Size == sz16)
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
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case LSH:
         {
            temp2 = ReadGen(0, sz8);
            if (temp2 & 0xE0)
               temp2 |= 0xE0;
            temp = ReadGen(1, LookUp.p.Size);
            if (temp2 & 0xE0)
               temp >>= ((temp2 ^ 0xFF) + 1);
            else
               temp <<= temp2;

            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         // TODO, could merge this with TBIT if Format4 operand indexes were consistent (dst == index 0)
         case IBIT:
         case CBIT:
         case SBIT:
         // The SBITIB, SBITIW, and SBITID instructions, in addition, activate the Interlocked
         // Operation output pin on the CPU, which may be used in multiprocessor systems to
         // interlock accesses to semaphore bits. This aspect is not implemented here.
         case CBITI:
         case SBITI:
            temp2 = ReadGen(0, LookUp.p.Size);
            if (gentype[1])
            {
               // operand 0 is a register
               WriteSize = sz32;
               temp2 &= 31;
            }
            else
            {
               // operand0 is memory
               // TODO: this should probably use the DIV and MOD opersator functions
               genaddr[1] += temp2 / 8;
               WriteSize = sz32;
               temp2 %= 8;
            }
            temp = ReadGen(1, WriteSize);
            psr &= ~F_FLAG;
            if (temp & BIT(temp2))
               psr |= F_FLAG;
            if (LookUp.p.Function == IBIT)
            {
               temp ^= BIT(temp2);
            }
            else if ((LookUp.p.Function == SBIT) || (LookUp.p.Function == SBITI))
            {
               temp |= BIT(temp2);
            }
            else
            {
               temp &= ~(BIT(temp2));
            }
            WriteIndex = 1;
            break;

         case NOT:
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp ^= 1;
            WriteIndex = 1;
            WriteSize = LookUp.p.Size;
         }
         break;

         case NEG:
         {
            temp = 0;
            temp2 = ReadGen(0, LookUp.p.Size);
            update_sub_flags(temp, temp2, 0);
            temp -= temp2;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;
 
         case ABS:
         {
            temp = ReadGen(0, LookUp.p.Size);
            switch (LookUp.p.Size)
            {
               case sz8:
               {
                  if (temp == 0x80)
                  {
                     psr |= F_FLAG;
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
                     psr |= F_FLAG;
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
                     psr |= F_FLAG;
                  }
                  if (temp & 0x80000000)
                  {
                     temp = (temp ^ 0xFFFFFFFF) + 1;
                  }
               }
               break;
            }
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case COM:
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp = ~temp;
            WriteIndex = 1;
            WriteSize = LookUp.p.Size;
         }
         break;

         case CXPD:
         {
            temp = ReadGen(0, sz32);
            pushw(0);
            pushw(mod);
            pushd(pc);
            mod = temp & 0xFFFF;
            temp3 = temp >> 16;
            sb = read_x32(mod);
            temp2 = read_x32(mod + 8);
            pc = temp2 + temp3;
         }
         break;

         case BICPSR:
         {
            temp = ReadGen(0, LookUp.p.Size);
            psr &= ~temp;
         }
         break;

         case JUMP:
         {
            if (gentype[0])
               pc = *(uint32_t *) genaddr[0];
            else
               pc = genaddr[0];
         }
         break;

         case BISPSR:
         {
            temp = ReadGen(0, LookUp.p.Size);
            psr |= temp;
         }
         break;

         case ADJSP:
         {
            temp = ReadGen(0, LookUp.p.Size);
            SIGN_EXTEND(temp);
            sp[SP] -= temp;
         }
         break;

         case JSR:
         {
            pushd(pc);
            pc = ReadGen(0, sz32);
         }
         break;

         case CASE:
         {
            temp = ReadGen(0, LookUp.p.Size);

            if (temp & 0x80)
               temp |= 0xFFFFFF00;
            pc = startpc + temp;
         }
         break;

#if 0 
         //OLD 32 bit Version
         case 0xA: // ADJSP
            temp2 = ReadGen(0, sz32);
            sp[SP] -= temp2;
            break;
#endif

         case MOVM:
         {
            temp = getdisp() + (LookUp.p.Size + 1); // disp of 0 means move 1 byte
            while (temp)
            {
               temp2 = read_x8(genaddr[0]);
               genaddr[0]++;
               write_x8(genaddr[1], temp2);
               genaddr[1]++;
               temp--;
            }
         }
         break;

         case CMPM:
         {
            temp4 = (LookUp.p.Size + 1);          // disp of 0 means move 1 byte/word/dword
            temp3 = (getdisp() / temp4) + 1;

            //printf("CMP Size = %u Count = %u\n", temp4, temp3);
            while (temp3--)
            {
               switch (LookUp.p.Size)
               {
                  case sz8:
                  {
                     temp =  read_x8(genaddr[0]);
                     temp2 = read_x8(genaddr[1]);
                  }
                  break;

                  case sz16:
                  {
                     temp = read_x16(genaddr[0]);
                     temp2 = read_x16(genaddr[1]);
                  }
                  break;

                  case sz32:
                  {
                     temp = read_x16(genaddr[0]);
                     temp2 = read_x16(genaddr[1]);
                  }
                  break;
               }
               
               if (CompareCommon(temp, temp2) == 0)
               {
                  break;
               }

               genaddr[0] += temp4;
               genaddr[1] += temp4;
            }
         }
         break;

         case INSS:
         {
            int c;

            // Read the immediate offset (3 bits) / length - 1 (5 bits) from the instruction
            temp3 = read_x8(pc);
            pc++;
            temp = ReadGen(0, LookUp.p.Size);   // src operand
            temp2 = ReadGen(1, LookUp.p.Size);  // base operand
            for (c = 0; c <= (temp3 & 0x1F); c++)
            {
               temp2 &= ~(BIT((c + (temp3 >> 5)) & 31));
               if (temp & BIT(c))
                  temp2 |= BIT((c + (temp3 >> 5)) & 31);
            }
            temp = temp2;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case EXTS:
         {
            int c;

            // Read the immediate offset (3 bits) / length - 1 (5 bits) from the instruction
            temp3 = read_x8(pc);
            pc++;
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = 0;
            temp >>= (temp3 >> 5); // Shift by offset
            temp3 &= 0x1F; // Mask off the lower 5 Bits which are number of bits to extract

            temp4 = 1;
            for (c = 0; c <= temp3; c++)
            {
               if (temp & temp4) // Copy the ones
               {
                  temp2 |= temp4;
               }

               temp4 <<= 1;
            }
            temp = temp2;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case MOVXBW:
         {
            temp = ReadGen(0, sz8);
            SIGN_EXTEND(temp);                                    // Editor need the useless semicolon
            if (sdiff[1])
               sdiff[1] = 4;
            WriteSize = sz16;
            WriteIndex = 1;
         }
         break;

         case MOVXiD:
         {
            temp = ReadGen(0, LookUp.p.Size);
            SIGN_EXTEND(temp);
            if (sdiff[1])
               sdiff[1] = 4;
            WriteSize = sz32;
            WriteIndex = 1;
         }
         break;

         case MOVZBW:
         {
            temp = ReadGen(0, sz8);
            if (sdiff[1])
               sdiff[1] = 4;
            WriteSize = sz16;
            WriteIndex = 1;
         }
         break;

         case MOVZiD:
         {
            temp = ReadGen(0, LookUp.p.Size);
            if (sdiff[1])
               sdiff[1] = 4;
            WriteSize = sz32;
            WriteIndex = 1;
         }
         break;

         case MEI:
         {
            temp = ReadGen(0, LookUp.p.Size);     // src
            temp64 = ReadGen(1, LookUp.p.Size);   // dst
            temp64 *= temp;
            // Handle the writing to the upper half of dst locally here
            handle_mei_dei_upper_write(temp64);
            // Allow fallthrough write logic to write the lower half of dst
            temp = temp64;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case DEI:
         {
            int size = (LookUp.p.Size + 1) << 3;  // 8, 16  or 32 
            temp = ReadGen(0, LookUp.p.Size);     // src
            if (temp == 0)
            {
               n32016_dumpregs("Divide by zero - DEI CE");
               break;
            }
            temp64 = readgenq(1);                   // dst
            switch (LookUp.p.Size)
            {
                  case sz8:
                     temp64 = ((temp64 >> 24) & 0xFF00) | (temp64 & 0xFF);
                     break;

                  case sz16:
                     temp64 = ((temp64 >> 16) & 0xFFFF0000) | (temp64 & 0xFFFF);
                     break;
            }
            printf("temp = %08x\n", temp);
            printf("temp64 = %016" PRIu64 "\n", temp64);
            temp64 = ((temp64 / temp) << size) | (temp64 % temp);
            printf("result = %016" PRIu64 "\n", temp64);
            // Handle the writing to the upper half of dst locally here
            handle_mei_dei_upper_write(temp64);
            // Allow fallthrough write logic to write the lower half of dst
            temp = temp64;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case MUL:
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            temp *= temp2;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case QUO:
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            if (temp == 0)
            {
               n32016_dumpregs("Divide by zero - QUO CE");
               break;
            }
            switch (LookUp.p.Size)
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
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case REM:
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            if (temp == 0)
            {
               n32016_dumpregs("Divide by zero - REM CE");
               break;
            }

            switch (LookUp.p.Size)
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
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case DIV:
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            if (temp == 0)
            {
               n32016_dumpregs("Divide by zero - DIV CE");
               break;
            }
            temp = div_operator(temp2, temp);
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case MOD:
         {
            temp = ReadGen(0, LookUp.p.Size);
            temp2 = ReadGen(1, LookUp.p.Size);
            if (temp == 0)
            {
               n32016_dumpregs("Divide by zero - MOD CE");
               break;
            }
            temp = mod_operator(temp2, temp);
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case EXT:
         {
            int c;
            
            temp = r[(opcode >> 11) & 7] & 31;
            temp2 = getdisp();
            temp3 = ReadGen(0, sz32);
            temp4 = 0;
            for (c = 0; c < temp2; c++)
            {
               if (temp3 & BIT((c + temp) & 31))
                  temp4 |= BIT(c);
            }

            temp = temp4;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case CVTP:
         {      
            int32_t Offset    = r[(opcode >> 11) & 7] % 8;
            int32_t Base      = genaddr[0];
 
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
            int32_t Source = ReadGen(0, LookUp.p.Size); 
            int32_t Base   = ReadGen(1, LookUp.p.Size);

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
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case CHECK:
         {
            //temp3 = ReadGen(1, sz8);
            temp3 = ReadGen(1, LookUp.p.Size);          

            switch (LookUp.p.Size)
            {
               case sz8:
               {
                  temp = read_x8(genaddr[0]);
                  temp2 = read_x8(genaddr[0] + 1);
               }
               break;

               case sz16:
               {
                  temp = read_x16(genaddr[0]);
                  temp2 = read_x16(genaddr[0] + 2);
               }
               break;

               case sz32:
               {
                  temp = read_x32(genaddr[0]);
                  temp2 = read_x32(genaddr[0] + 4);
               }
               break;
            }
         
            //printf("Reg = %u Bounds [%u - %u] Index = %u", 0, temp, temp2, temp3);

            if (temp >= temp3 && temp3 >= temp2)
            {
               r[(opcode >> 11) & 7] = temp3 - temp2;
               psr &= ~F_FLAG;
            }
            else
               psr |= F_FLAG;
         }
         break;

         case INDEX:
         {
            // r0, r1, r2
            // 5, 7, 0x13 (19)
            // accum = accum * (length+1) + index

            temp  = r[(opcode >> 11) & 7];            // Accum
            temp2 = ReadGen(0, LookUp.p.Size) + 1;    // (length+1)
            temp3 = ReadGen(1, LookUp.p.Size);        // index

            r[(opcode >> 11) & 7] = (temp * temp2) + temp3;
         }
         break;

         case FFS:
            {
               int numbits = (LookUp.p.Size + 1) << 3;   // number of bits: 8, 16 or 32
               temp2 = ReadGen(0, LookUp.p.Size);        // base is the variable size operand being scanned
               temp  = ReadGen(1, sz8);                  // offset is always 8 bits (also the result)
               // find the first set bit, starting at offset
               for (;temp < numbits && !(temp2 & BIT(temp)); temp++);
               if (temp < numbits)
               {
                  // a set bit was found, return it in the offset operand
                  psr &= ~F_FLAG;
               }
               else
               {
                  // no set bit was found, return 0 in the offset operand
                  psr |= F_FLAG;
                  temp = 0;
               }
               WriteIndex = 1;
               WriteSize = sz8;
            }
            break;

         case BSR:
            temp = getdisp();
            pushd(pc);
            pc = startpc + temp;
            break;

         case RET:
            temp = getdisp();
            pc = popd();
            sp[SP] += temp;
            break;

         case CXP:
            temp = getdisp();
            pushw(0);
            pushw(mod);
            pushd(pc);
            temp2 = read_x32(mod + 4) + (4 * temp);
            temp = read_x32(temp2);
            mod = temp & 0xFFFF;
            sb = read_x32(mod);
            pc = read_x32(mod + 8) + (temp >> 16);
            break;

         case RXP:
            temp = getdisp();
            pc = popd();
            temp2 = popd();
            mod = temp2 & 0xFFFF;
            sp[SP] += temp;
            sb = read_x32(mod);
            break;

         case RETT:
            temp = getdisp();
            pc = popd();
            mod = popw();
            psr = popw();
            sp[SP] += temp;
            sb = read_x32(mod);
            break;

         case RETI:
            printf("RETI ????");
            break;

         case SAVE:
            {
               int c;

               temp = read_x8(pc);
               pc++;

               for (c = 0; c < 8; c++)
               {
                  if (temp & BIT(c))
                  {
                     pushd(r[c]);
                  }
               }
            }
            break;

         case RESTORE:
            {
               int c;
               temp = read_x8(pc);
               pc++;
               for (c = 0; c < 8; c++)
               {
                  if (temp & BIT(c))
                     r[c ^ 7] = popd(r[c]);
               }
            }
            break;

         case ENTER:
            {
               int c;

               temp = read_x8(pc);
               pc++;
               temp2 = getdisp();
               pushd(fp);
               fp = sp[SP];
               sp[SP] -= temp2;

               for (c = 0; c < 8; c++)
               {
                  if (temp & BIT(c))
                  {
                     pushd(r[c]);
                  }
               }
            }
            break;

         case EXIT:
            {
               int c;
               temp = read_x8(pc);
               pc++;
               for (c = 0; c < 8; c++)
               {
                  if (temp & BIT(c))
                  {
                     r[c ^ 7] = popd(r[c]);
                  }
               }
               sp[SP] = fp;
               fp = popd();
            }  
            break;

         case NOP:
            //temp = read_x8(pc);
            //pc++;
            break;

         case SVC:
            temp = psr;
            psr &= ~0x700;
            temp = read_x32(intbase + (5 * 4));
            mod = temp & 0xFFFF;
            temp3 = temp >> 16;
            sb = read_x32(mod);
            temp2 = read_x32(mod + 8);
            pc = temp2 + temp3;
            break;

         case BPT:
            //temp = read_x8(pc);
            //pc++;
            printf("BPT ??????\n");
            break;

         case BEQ: // BEQ
            if (psr & Z_FLAG)
               pc = temp;
            break;

         case BNE: // BNE
            if (!(psr & Z_FLAG))
               pc = temp;
            break;

         case BH: // BH
            if (psr & L_FLAG)
               pc = temp;
            break;

         case BLS: // BLS
            if (!(psr & L_FLAG))
               pc = temp;
            break;

         case BGT: // BGT
            if (psr & N_FLAG)
               pc = temp;
            break;

         case BLE: // BLE
            if (!(psr & N_FLAG))
               pc = temp;
            break;

         case BFS: // BFS
            if (psr & F_FLAG)
               pc = temp;
            break;

         case BFC: // BFC
            if (!(psr & F_FLAG))
               pc = temp;
            break;

         case BLO: // BLO
            if (!(psr & (L_FLAG | Z_FLAG)))
               pc = temp;
            break;

         case BHS: // BHS
            if (psr & (L_FLAG | Z_FLAG))
               pc = temp;
            break;

         case BLT: // BLT
            if (!(psr & (N_FLAG | Z_FLAG)))
               pc = temp;
            break;

         case BGE: // BGE
            if (psr & (N_FLAG | Z_FLAG))
               pc = temp;
            break;

         case BR: // BR
            pc = temp;
            break;

         case ADDP: /*ADDP*/
         {
            int carry = (psr & C_FLAG) ? 1 : 0;
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);
            temp = bcd_add(temp, temp2, LookUp.p.Size, &carry);
            if (carry)
               psr |= C_FLAG;
            else
               psr &= ~C_FLAG;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         case SUBP: /*SUBP*/
         {
            int carry = (psr & C_FLAG) ? 1 : 0;
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);
            temp = bcd_sub(temp, temp2, LookUp.p.Size, &carry);
            if (carry)
               psr |= C_FLAG;
            else
               psr &= ~C_FLAG;
            WriteSize = LookUp.p.Size;
            WriteIndex = 1;
         }
         break;

         default:
            n32016_dumpregs("Bad NS32016 opcode");
            break;
      }

      switch (WriteSize)
      {
         case sz8:
         {
            if (gentype[WriteIndex]) *((uint8_t*) genaddr[WriteIndex]) = temp;
            else
            {
               if (sdiff[WriteIndex]) genaddr[WriteIndex] = sp[SP] = sp[SP] - sdiff[WriteIndex];
               write_x8(genaddr[WriteIndex], temp);
            }
         }
         break;

         case sz16:
         {
#ifdef TEST_SUITE
            if (gentype[WriteIndex] && (genaddr[WriteIndex] == &r[7]))
            {
               printf("*** TEST = %u\n", temp);
#if 0
               if (temp == 207)
               {
                  Trace = 1;
               }
#endif
            }
#endif
            
            if (gentype[WriteIndex]) *((uint16_t*) genaddr[WriteIndex]) = temp;
            else
            {
               if (sdiff[WriteIndex]) genaddr[WriteIndex] = sp[SP] = sp[SP] - sdiff[WriteIndex];
               write_x16(genaddr[WriteIndex], temp);
            }
         }
         break;

         case sz32:
         {
            if (gentype[WriteIndex]) *((uint32_t*) genaddr[WriteIndex]) = temp;
            else
            {
               if (sdiff[WriteIndex]) genaddr[WriteIndex] = sp[SP] = sp[SP] - sdiff[WriteIndex];
               write_x32(genaddr[WriteIndex], temp);
            }
         }
         break;
      }

      dump_mini();

#if 0
      // Test Suite Diverter ;)
      if (pc == 0x001CB2)
      {
         pc = 0x001AD9;
      }
#endif

      tubecycles -= 8;
      if (tube_irq & 2)
      {
         temp = psr;
         psr &= ~0xF00;
         pushw(temp);
         pushw(mod);
         pushd(pc);
         temp = read_x32(intbase + (1 * 4));
         mod = temp & 0xFFFF;
         temp3 = temp >> 16;
         sb = read_x32(mod);
         temp2 = read_x32(mod + 8);
         pc = temp2 + temp3;
      }
      else if ((tube_irq & 1) && (psr & 0x800))
      {
         temp = psr;
         psr &= ~0xF00;
         pushw(temp);
         pushw(mod);
         pushd(pc);
         temp = read_x32(intbase);
         mod = temp & 0xFFFF;
         temp3 = temp >> 16;
         sb = read_x32(mod);
         temp2 = read_x32(mod + 8);
         pc = temp2 + temp3;
      }

      oldpsr = psr;
   }
}
