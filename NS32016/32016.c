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

int nsoutput = 0;
int gentype[2];
int nscfg;

uint8_t ns32016ram[MEG16 + 8];				// Extra 8 bytes as we can read that far off the end of RAM
uint32_t tube_irq = 0;
uint32_t r[8];
uint32_t pc, sp[2], fp, sb, intbase;
uint16_t psr, mod;
uint32_t startpc;
uint32_t genaddr[2];
DecodeMatrix mat[256];
DecodeMatrix LookUp;

#define SP ((psr & S_FLAG) >> 9)

#define SIGN_EXTEND(reg) \
  if ((LookUp.p.Size == sz8) && (reg & 0x80)) { \
    reg |= 0xFFFFFF00; \
  } else if ((LookUp.p.Size == sz16) && (reg & 0x8000)) { \
    reg |= 0xFFFF0000; \
  }

void n32016_build_matrix()
{
   uint32_t Index;

   memset(mat, 0xFF, sizeof(mat));

   for (Index = 0; Index < 256; Index++)
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

void n32016_reset(uint32_t StartAddress)
{
	pc = StartAddress;
	psr = 0;

	n32016_build_matrix();
}

void n32016_dumpregs(char* pMessage)
{
   //FILE *f = fopen("32016.dmp", "wb");
   //fwrite(ns32016ram, 1024 * 1024, 1, f);
   //fclose(f);

   printf("%s\n", pMessage);
   printf("R0=%08X R1=%08X R2=%08X R3=%08X\n", r[0], r[1], r[2], r[3]);
   printf("R4=%08X R5=%08X R6=%08X R7=%08X\n", r[4], r[5], r[6], r[7]);
   printf("PC=%08X SB=%08X SP0=%08X SP1=%08X\n", pc, sb, sp[0], sp[1]);
   printf("FP=%08X INTBASE=%08X PSR=%04X MOD=%04X\n", fp, intbase, psr, mod);
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

static void getgen(int gen, int c)
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
      genaddr[c] = (uint32_t) & r[gen & 7];
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
      /*                genaddr[c]=pc;*/
       gentype[c]=0;
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
      getgen(genindex[c] >> 3, c);
      if (!gentype[c])
        genaddr[c] += r[genindex[c] & 7];
      else
        genaddr[c] = *(uint32_t *) genaddr[c] + r[genindex[c] & 7];
      gentype[c] = 0;
      break;

    case 0x1D: // EA + Rn*2
      getgen(genindex[c] >> 3, c);
      if (!gentype[c])
        genaddr[c] += (r[genindex[c] & 7] * 2);
      else
        genaddr[c] = *(uint32_t *) genaddr[c] + (r[genindex[c] & 7] * 2);
      gentype[c] = 0;
      break;

    case 0x1E: // EA + Rn*4
      getgen(genindex[c] >> 3, c);
      if (!gentype[c])
        genaddr[c] += (r[genindex[c] & 7] * 4);
      else
        genaddr[c] = *(uint32_t *) genaddr[c] + (r[genindex[c] & 7] * 4);
      gentype[c] = 0;
      break;

    case 0x1F: // EA + Rn*8
      getgen(genindex[c] >> 3, c);
      if (!gentype[c])
        genaddr[c] += (r[genindex[c] & 7] * 8);
      else
        genaddr[c] = *(uint32_t *) genaddr[c] + (r[genindex[c] & 7] * 8);
      gentype[c] = 0;
      break;

    default:
      n32016_dumpregs("Bad NS32016 gen mode");
		break;
  }
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
         sp[SP]      = sp[SP] + sdiff[c];
      }
   }

   switch (Size)
   {
      case sz8:
      {
         Temp &= 0xFF;
      }
      break;

      case sz16:
      {
         Temp &= 0xFFFF;
      }
      break;
   }

   return Temp;
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

void writegenb(uint32_t c, uint8_t temp)
{
   if (gentype[c]) *((uint8_t*) genaddr[c]) = temp;
   else
   {
      if (sdiff[c]) genaddr[c] = sp[SP] = sp[SP] - sdiff[c];
      write_x8(genaddr[c], temp);
   }
}

void writegenw(uint32_t c, uint16_t temp)
{
   if (gentype[c]) *((uint16_t*) genaddr[c]) = temp;
   else
   {
      if (sdiff[c]) genaddr[c] = sp[SP] = sp[SP] - sdiff[c];
      write_x16(genaddr[c], temp);
   }
}

void writegenl(uint32_t c, uint32_t temp)
{
   if (gentype[c]) *((uint32_t*) genaddr[c]) = temp;
   else
   {
      if (sdiff[c]) genaddr[c] = sp[SP] = sp[SP] - sdiff[c];
      write_x32(genaddr[c], temp);
   }
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
   int signmask = 1 << ((LookUp.p.Size << 3) + 7);
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

void n32016_exec(uint32_t tubecycles)
{
   uint32_t opcode, WriteSize, WriteIndex;
   uint32_t temp = 0, temp2, temp3, temp4;
   uint64_t temp64;
   int c;

   while (tubecycles > 0)
   {
      sdiff[0] = sdiff[1] = 0;
      startpc = pc;
      ClearRegs();
      opcode = read_x32(pc);

      LookUp = mat[opcode & 0xFF];
      pc += LookUp.p.BaseSize;
      WriteSize = szVaries;
      WriteIndex = 0; // default to writing operand 0

      switch (LookUp.p.Format)
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
            LookUp.p.Function = (opcode & 0x80) ? TRAP : (CXPD + ((opcode >> 8) & 7));
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
            LookUp.p.Function = (opcode & 0x30) ? TRAP : (MOVS + ((opcode >> 2) & 3));
            temp2 = (opcode >> 15) & 0xF;
         }
         break;

         case Format6:
         {
            LookUp.p.Function = ROT + ((opcode >> 10) & 15);
            LookUp.p.Size = (opcode >> 8) & 3;

            // Ordering important here, as getgen uses LookUp.p.Size
            if ((opcode & 0x3C00) == 0x0000 || (opcode & 0x3C00) == 0x0400 || (opcode & 0x3C00) == 0x1400) //  ROT/ASH/LSH 
            {
               LookUp.p.Size = sz8;
            }
            getgen(opcode >> 19, 0);
            LookUp.p.Size = (opcode >> 8) & 3;
            getgen(opcode >> 14, 1);
         }
         break;

         case Format7:
         {
            LookUp.p.Function = MOVM + ((opcode >> 10) & 15);
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
                  LookUp.p.Function = (opcode & 0x40) ? FFS : INDEX;
               }
               else
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
            }
            else
            {
               LookUp.p.Function = EXT + ((opcode >> 6) & 3);
            }

            getgen(opcode >> 19, 0);
            getgen(opcode >> 14, 1);
         }
         break;
      }

      ShowInstruction(startpc, opcode, LookUp.p.Function, LookUp.p.Size);

#if 1 
      if (startpc == 0x1CB2)
      {
         n32016_dumpregs("Test Suite Failure!\n");
      }
#endif

      switch (LookUp.p.Function)
      {
         case MOVS:
         {
            if (temp2 & 3)
            {
               n32016_dumpregs("Bad NS32016 MOVS %08X");
            }

            while (r[0])
            {
               temp = read_x8(r[1]);
               r[1]++;
               if ((temp2 & 0xC) == 0xC && temp == r[4])
               {
                  break;
               }
               if ((temp2 & 0xC) == 0x4 && temp != r[4])
               {
                  break;
               }
               write_x8(r[2], temp);
               r[2]++;
               r[0]--;
            }
         }
         break;

#if 0
         case 0x03: // MOVS dword
            if (temp2)
            {
               n32016_dumpregs("Bad NS32016 MOVS");
               break;
            }
            while (r[0])
            {
               temp = read_x32(r[1]);
               r[1] += 4;
               write_x32(r[2], temp);
               r[2] += 4;
               r[0]--;
            }
            break;
#endif

         case SETCFG:
         {
            nscfg = temp;
         }
         break;

         case ADDQ:
         {
            temp2 = (opcode >> 7) & 0xF;
            if (temp2 & 8)
               temp2 |= 0xFFFFFFF0;
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
            if (temp2 & 8)
               temp2 |= 0xFFFFFFF0;

            temp = ReadGen(0, LookUp.p.Size);

            //  Sign extend smaller quantites to 32 bits to match temp2 
            if (LookUp.p.Size == sz8) {
               temp = (signed char) temp;
            }
            else if (LookUp.p.Size == sz16) {
               temp = (signed short) temp;
            }

            psr &= ~(Z_FLAG | N_FLAG | L_FLAG);
            if (temp == temp2)
               psr |= Z_FLAG;
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
         }
         break;

         case SPR:
         {
            switch ((opcode >> 7) & 0xF)
            {
               case 0x8:
                  writegenl(0, fp);
                  break;
               case 0x9:
                  writegenl(0, sp[SP]);
                  break;
               case 0xA:
                  writegenl(0, sb);
                  break;
               case 0xF:
                  writegenl(0, mod);
                  break;
               default:
                  n32016_dumpregs("Bad SPR reg");
                  break;
            }
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
            if (temp2 & 8)
               temp2 |= 0xFFFFFFF0;
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
            if (temp & 8)
               temp |= 0xFFFFFFF0;
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
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);

            psr &= ~(Z_FLAG | N_FLAG | L_FLAG);
            if (temp == temp2)
               psr |= Z_FLAG;
            if (temp > temp2)
               psr |= L_FLAG;
            if (LookUp.p.Size == sz8)
            {
               if (((signed char) temp) > ((signed char) temp2))
                  psr |= N_FLAG;
            }
            else if (LookUp.p.Size == sz32)
            {
               if (((signed long) temp) > ((signed long) temp2))
                  psr |= N_FLAG;
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
            temp2 = ReadGen(1, sz32);
            if (gentype[0])
            {
               // operand 0 is a register
               temp = ReadGen(0, sz32);
            }
            else
            {
               // operand0 is memory
               // TODO: this should probably use the DIV and MOD opersator functions
               genaddr[0] += temp2 / 8;
               temp = ReadGen(0, sz8);
               temp2 &= temp2 % 8;
            }
            psr &= ~F_FLAG;
            if (temp & (1 << temp2))
               psr |= F_FLAG;
            break;

         case XOR:
            temp2 = ReadGen(0, LookUp.p.Size);
            temp = ReadGen(1, LookUp.p.Size);
            temp ^= temp2;
            WriteSize = LookUp.p.Size;
            break;

         case ROT:
         {
            switch (LookUp.p.Size)
            {
               case sz8:
               {
                  temp = ReadGen(0, sz8);
                  if (temp & 0xE0)
                  {
                     temp |= 0xE0;
                     temp = ((temp ^ 0xFF) + 1);
                     temp = 8 - temp;
                  }
                  temp2 = ReadGen(1, sz8);
                  temp2 = (temp2 << temp) | (temp2 >> (8 - temp));
                  writegenb(1, temp2);
               }
               break;

               case sz16:
               {
                  temp = ReadGen(0, sz8);
                  if (temp & 0xE0) 
                  {
                     temp |= 0xE0;
                     temp = ((temp ^ 0xFF) + 1);
                     temp = 16 - temp;
                  }
                  temp2 = ReadGen(1, sz16);
                  temp2 = (temp2 << temp) | (temp2 >> (16 - temp));
                  writegenw(1, temp2);
               }
               break;

               case sz32:
               {
                  temp = ReadGen(0, sz8);
                  if (temp & 0xE0)
                  {
                     temp |= 0xE0;
                     temp = ((temp ^ 0xFF) + 1);
                     temp = 32 - temp;
                  }
                  temp2 = ReadGen(1, sz32);
                  temp2 = (temp2 << temp) | (temp2 >> (32 - temp));
                  writegenl(1, temp2);
               }
               break;
            }
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

         case CBIT:
         {
            temp = ReadGen(0, sz8);
            temp &= 31;
            if (gentype[1])
            {
               temp2 = ReadGen(1, sz32);
            }
            else
            {
               temp2 = ReadGen(1, sz8);
            }
            if (temp2 & (1 << temp))
               psr |= F_FLAG;
            else
               psr &= ~F_FLAG;
            temp2 &= ~(1 << temp);
            if (gentype[1])
            {
               writegenl(1, temp2);
            }
            else
            {
               writegenb(1, temp2);
            }
         }
         break;

         case LSH:
         {
            switch (LookUp.p.Size)
            {
               case sz8:
               {
                  temp = ReadGen(0, sz8);
                  if (temp & 0xE0)
                     temp |= 0xE0;
                  temp2 = ReadGen(1, sz8);
                  if (temp & 0xE0)
                     temp2 >>= ((temp ^ 0xFF) + 1);
                  else
                     temp2 <<= temp;
                  writegenb(1, temp2);
               }
               break;

               case sz16:
               {
                  temp = ReadGen(0, sz8);
                  if (temp & 0xE0)
                     temp |= 0xE0;
                  temp2 = ReadGen(1, sz16);
                  if (temp & 0xE0)
                     temp2 >>= ((temp ^ 0xFF) + 1);
                  else
                     temp2 <<= temp;
                  writegenw(1, temp2);
               }
               break;

               case sz32:
               {
                  temp = ReadGen(0, sz8);
                  if (temp & 0xE0)
                     temp |= 0xE0;
                  temp2 = ReadGen(1, sz32);
                  if (temp & 0xE0)
                     temp2 >>= ((temp ^ 0xFF) + 1);
                  else
                     temp2 <<= temp;
                  writegenl(1, temp2);
               }
               break;
            }
         }
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

         // TODO: This is a short term implementation
         // that just sets the Z flag. To also set
         // the N and L flags correctly, the correct
         // item size must be used, together with
         // appropriated signed/unsigned comparisons.
         case CMPM:
         {
            int match = 1;
            temp = getdisp() + (LookUp.p.Size + 1); // disp of 0 means move 1 byte/word/dword
            while (temp && match)
            {
               temp2 = read_x8(genaddr[0]);
               temp3 = read_x8(genaddr[1]);
               if (temp2 != temp3)
               {
                  match = 0;
               }
               genaddr[0]++;
               genaddr[1]++;
               temp--;
            }
            if (match)
            {
               psr |= Z_FLAG;
            }
            else
            {
               psr &= ~Z_FLAG;
            }
         }
         break;

         case INSS:
         {
            temp3 = read_x8(pc);
            pc++;
            temp = ReadGen(0, sz8);
            temp2 = ReadGen(1, sz8);
            for (c = 0; c <= (temp3 & 0x1F); c++)
            {
               temp2 &= ~(1 << ((c + (temp3 >> 5)) & 7));
               if (temp & (1 << c))
                  temp2 |= (1 << ((c + (temp3 >> 5)) & 7));
            }
            writegenb(1, temp2);
         }
         break;

         case EXTS:
         {
            temp3 = read_x8(pc);
            pc++;
            temp = ReadGen(0, sz8);
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

            writegenb(1, temp2);
         }
         break;

         case MOVXBW:
         {
            temp = ReadGen(0, sz8);
            SIGN_EXTEND(temp);                                    // Editor need the useless semicolon
            if (sdiff[1])
               sdiff[1] = 4;
            writegenw(1, temp);
         }
         break;

         case MOVXiD:
         {
            temp = ReadGen(0, LookUp.p.Size);
            SIGN_EXTEND(temp);
            if (sdiff[1])
               sdiff[1] = 4;
            writegenl(1, temp);
         }
         break;

         case MOVZBW:
         {
            temp = ReadGen(0, sz8);
            if (sdiff[1])
               sdiff[1] = 4;
            writegenw(1, temp);
         }
         break;

         case MOVZiD:
         {
            temp = ReadGen(0, LookUp.p.Size);
            if (sdiff[1])
               sdiff[1] = 4;
            writegenl(1, temp);
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
            temp = r[(opcode >> 11) & 7] & 31;
            temp2 = getdisp();
            temp3 = ReadGen(0, sz32);
            temp4 = 0;
            for (c = 0; c < temp2; c++)
            {
               if (temp3 & (1 << ((c + temp) & 31)))
                  temp4 |= (1 << c);
            }
            writegenl(1, temp4);
         }
         break;

         case CHECK:
         {
            temp3 = ReadGen(1, sz8);
            temp = read_x8(genaddr[0]);
            temp2 = read_x8(genaddr[0] + 1);
            if (temp >= temp3 && temp3 >= temp2)
            {
               r[(opcode >> 11) & 7] = temp3 - temp2;
               psr &= ~F_FLAG;
            }
            else
               psr |= F_FLAG;
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
            temp = read_x8(pc);
            pc++;
            for (c = 0; c < 8; c++)
            {
               if (temp & (1 << c))
               {
                  pushd(r[c]);
               }
            }
            break;

         case RESTORE:
            temp = read_x8(pc);
            pc++;
            for (c = 0; c < 8; c++)
            {
               if (temp & (1 << c))
               {
                  r[c ^ 7] = popd(r[c]);
               }
            }
            break;

         case ENTER:
            temp = read_x8(pc);
            pc++;
            temp2 = getdisp();
            pushd(fp);
            fp = sp[SP];
            sp[SP] -= temp2;

            for (c = 0; c < 8; c++)
            {
               if (temp & (1 << c))
               {
                  pushd(r[c]);
               }
            }
            break;

         case EXIT:
            temp = read_x8(pc);
            pc++;
            for (c = 0; c < 8; c++)
            {
               if (temp & (1 << c))
               {
                  r[c ^ 7] = popd(r[c]);
               }
            }
            sp[SP] = fp;
            fp = popd();
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
            writegenb(WriteIndex, temp);
         }
         break;

         case sz16:
         {
            writegenw(WriteIndex, temp);
         }
         break;

         case sz32:
         {
            writegenl(WriteIndex, temp);
         }
         break;
      }

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
