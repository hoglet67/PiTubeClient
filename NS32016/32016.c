/*B-em v2.2 by Tom Walker
 32016 parasite processor emulation (not working yet)*/

// And Simon R. Ellwood

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "32016.h"
#include "../bare-metal/tube-lib.h"
#include "PandoraV0_61.h"

int nsoutput = 0;
int gentype[2];
int nscfg;

uint8_t ns32016ram[MEG16];
uint32_t tube_irq = 0;
uint32_t r[8];
uint32_t pc, sp[2], fp, sb, intbase;
uint16_t psr, mod;
uint32_t startpc;
uint32_t genaddr[2];
DecodeMatrix mat[256];

#define SP ((psr & S_FLAG) >> 9)

void n32016_build_matrix()
{
	uint32_t Index;

	for (Index = 0; Index < 256; Index++)
	{
		switch (Index)
		{
			case 0x0E: // String instruction
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = StrI;
			}
			break;
			
			CASE2(0x1C) : // CMPQ byte
			{
				mat[Index].p.Size			= sz8;
				mat[Index].p.Function	= CMPQ;
			}
			break;

			CASE2(0x1F) : // CMPQ dword
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = CMPQ;
			}
			break;

			CASE2(0x5C) : // MOVQ byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = MOVQ;
			}
			break;

			CASE2(0x5D) : // MOVQ word
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = MOVQ;
			}
			break;

			CASE2(0x5F) : // MOVQ dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = MOVQ;
			}
			break;

			CASE2(0x0C) : // ADDQ byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = ADDQ;
			}
			break;

			CASE2(0x0D) : // ADDQ word
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = ADDQ;
			}
			break;

			CASE2(0x0F) : // ADDQ dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = ADDQ;
			}
			break;

			CASE2(0x3C) : // ScondB
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = Scond;
			}
			break;


			CASE2(0x3D) : // ScondW
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = Scond;
			}
			break;

			CASE2(0x3F) : // ScondD
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = Scond;
			}
			break;

			CASE2(0x4C) : // ACBB
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = ACB;
			}
			break;

			CASE2(0x4F) : // ACBD
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = ACB;
			}
			break;

			CASE4(0x00) : // ADD byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = ADD;
			}
			break;

			CASE4(0x03) : // ADD dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = ADD;
			}
			break;

			CASE4(0x04) : // CMP byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = CMP;
			}
			break;

			CASE4(0x07) : // CMP dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = CMP;
			}
			break;

			CASE4(0x08) : // BIC byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = BIC;
			}
			break;

			CASE4(0x0B) : // BIC dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = BIC;
			}
			break;

			CASE4(0x14) : // MOV byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = MOV;
			}
			break;

			CASE4(0x17) : // MOV dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = MOV;
			}
			break;

			CASE4(0x18) : //OR byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = OR;
			}
			break;

			CASE4(0x19) : // OR word
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = OR;
			}
			break;

			CASE4(0x1B) : // OR dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = OR;
			}
			break;

			CASE4(0x38) : // XOR byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = XOR;
			}
			break;

			CASE4(0x39) : // XOR word
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = XOR;
			}
			break;

			CASE4(0x3B) : // XOR dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = XOR;
			}
			break;

			CASE4(0x23) : // SUB dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = SUB;
			}
			break;

			CASE4(0x27) : // ADDR dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = ADDR;
			}
			break;

			CASE4(0x28) : // AND byte
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = AND;
			}
			break;

			CASE4(0x29) : // AND word
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = AND;
			}
			break;

			CASE4(0x2B) : // AND dword
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = AND;
			}
			break;

			CASE4(0x34) : // TBITB
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = TBIT;
			}
			break;

			CASE4(0x37) : // TBITD
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = TBIT;
			}
			break;

			case 0x4E: /*Type 6*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = TYPE6;
			}
			break;

			case 0x7C: /*Type 3 byte*/
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = TYPE3;
			}
			break;

			case 0x7D: /*Type 3 word*/
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = TYPE3;
			}
			break;

			case 0x7F: /*Type 3 dword*/
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = TYPE3;
			}
			break;


			CASE2(0x2F) : // SPR
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = SPR;
			}
			break;

			CASE2(0x6C) : // LPRB
			{
				mat[Index].p.Size = sz8;
				mat[Index].p.Function = LPR;
			}
			break;

			CASE2(0x6D) : // LPRW
			{
				mat[Index].p.Size = sz16;
				mat[Index].p.Function = LPR;
			}
			break;

			CASE2(0x6F) : // LPRD
			{
				mat[Index].p.Size = sz32;
				mat[Index].p.Function = LPR;
			}
			break;

			case 0xCE: /*Format 7*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = FORMAT7;
			}
			break;

			CASE4(0x2E) : // Type 8
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = TYPE8;
			}
			break;

			case 0x02: /*BSR*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BSR;
			}
			break;

			case 0x12: /*RET*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = RET;
			}
			break;

			case 0x22: /*CXP*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = CXP;
			}
			break;

			case 0x32: /*RXP*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = RXP;
			}
			break;

			case 0x42: /*RETT*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = RETT;
			}
			break;

			case 0x62: /*SAVE*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = SAVE;
			}
			break;

			case 0x72: /*RESTORE*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = RESTORE;
			}
			break;

			case 0x82: /*ENTER*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = ENTER;
			}
			break;

			case 0x92: /*EXIT*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = ENTER;
			}
			break;

			case 0xE2: /*SVC*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = ENTER;
			}
			break;

			case 0x0A: /*BEQ*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BEQ;
			}
			break;

			case 0x1A: /*BNE*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BNE;
			}
			break;

			case 0x4A: /*BH*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BH;
			}
			break;

			case 0x5A: /*BLS*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BLS;
			}
			break;

			case 0x6A: /*BGT*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BGT;
			}
			break;

			case 0x7A: /*BLE*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BLE;
			}
			break;

			case 0x8A: /*BFS*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BFS;
			}
			break;

			case 0x9A: /*BFC*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BFC;
			}
			break;

			case 0xAA: /*BLO*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BLO;
			}
			break;

			case 0xBA: /*BHS*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BHS;
			}
			break;

			case 0xCA: /*BLT*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BLT;
			}
			break;
			case 0xDA: /*BGE*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BGE;
			}
			break;

			case 0xEA: /*BR*/
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BR;
			}
			break;

			default:
			{
				mat[Index].p.Size = szVaries;
				mat[Index].p.Function = BFS;
			}
			break;
		}
	}
}

void n32016_reset()
{
	pc = 0;
	psr = 0;

	n32016_build_matrix();
}

void n32016_dumpregs()
{
	//FILE *f = fopen("32016.dmp", "wb");
	//fwrite(ns32016ram, 1024 * 1024, 1, f);
	//fclose(f);

	printf("R0=%08X R1=%08X R2=%08X R3=%08X\n", r[0], r[1], r[2], r[3]);
	printf("R4=%08X R5=%08X R6=%08X R7=%08X\n", r[4], r[5], r[6], r[7]);
	printf("PC=%08X SB=%08X SP0=%08X SP1=%08X\n", pc, sb, sp[0], sp[1]);
	printf("FP=%08X INTBASE=%08X PSR=%04X MOD=%04X\n", fp, intbase, psr, mod);
	//exit(1);
}

uint8_t readmemb(uint32_t addr)
{
	addr &= MEM_MASK;

	//if (addr < RAM_SIZE)
	{
		return ns32016ram[addr];
	}

	if (addr >= 0xFFFFF0)
	{
		return tubeRead(addr >> 1);
	}

	printf("Bad readmemb %08X\n", addr);
	n32016_dumpregs();

	return 0;
}

static uint16_t readmemw(uint32_t addr)
{
	addr &= MEM_MASK;
	//if (addr < 0x100000)
	{
//    printf("Read %08X %04X\n",addr,ns32016ram[addr&0xFFFFF]|(ns32016ram[(addr+1)&0xFFFFF]<<8));
		return ns32016ram[addr & 0xFFFFF]
				| (ns32016ram[(addr + 1) & 0xFFFFF] << 8);
	}

	if (addr < 0x400000)
	{
		return 0;
	}

	if ((addr & ~0x7FFF) == 0xF00000)
	{
		return PandoraV0_61[addr & 0x7FFF]
				| (PandoraV0_61[(addr + 1) & 0x7FFF] << 8);
	}

	printf("Bad readmemw %08X\n", addr);
	n32016_dumpregs();

	return 0;
}

static void writememb(uint32_t addr, uint8_t val)
{
	addr &= MEM_MASK;

	if (addr < RAM_SIZE)
	{
		ns32016ram[addr] = val;
		return;
	}

	if (addr == 0xF90000)
	{
		memset(ns32016ram, 0, MEG4);
		return;
	}

	if (addr >= 0xFFFFF0)
	{
		tubeWrite(addr >> 1, val);
		return;
	}

	printf("Bad writememb %08X %02X\n", addr, val);
	n32016_dumpregs();
}

static void writememw(uint32_t addr, uint16_t val)
{
	addr &= MEM_MASK;

	if (addr < RAM_SIZE)
	{
		ns32016ram[addr] = val & 0xFF;
		ns32016ram[addr + 1] = val >> 8;
		return;
	}

	if (addr < 0x400000)
	{
		return;
	}

	printf("Bad writememw %08X %04X\n", addr, val);
	n32016_dumpregs();
}

static void pushw(uint16_t val)
{
	sp[SP] -= 2;
	writememw(sp[SP], val);
}

static void pushd(uint32_t val)
{
	sp[SP] -= 4;

	writememw(sp[SP], val);
	writememw(sp[SP] + 2, val >> 16);
}

static uint16_t popw()
{
	uint16_t temp = readmemw(sp[SP]);
	sp[SP] += 2;

	return temp;
}

static uint32_t popd()
{
	uint32_t temp = readmemw(sp[SP]) | (readmemw(sp[SP] + 2) << 16);
	sp[SP] += 4;

	return temp;
}

static uint32_t getdisp()
{
	uint32_t addr = readmemb(pc);
	pc++;
	if (!(addr & 0x80))
	{
		return addr | ((addr & 0x40) ? 0xFFFFFF80 : 0);
	}

	if (!(addr & 0x40))
	{
		addr &= 0x3F;
		addr = (addr << 8) | readmemb(pc);
		pc++;

		return addr | ((addr & 0x2000) ? 0xFFFFC000 : 0);
	}

	addr &= 0x3F;
	addr = (addr << 24) | (readmemb(pc) << 16);
	pc++;
	addr = addr | (readmemb(pc) << 8);
	pc++;
	addr = addr | readmemb(pc);
	pc++;

	return addr | ((addr & 0x20000000) ? 0xC0000000 : 0);
}

int isize = 0;
int ilook[4] =
{
	1, 2, 0, 4
};

int genindex[2];
static void getgen1(int gen, int c)
{
	if ((gen & 0x1C) == 0x1C)
	{
		genindex[c] = readmemb(pc);
		pc++;
	}
}

int sdiff[2] =
{
	0, 0
};

uint32_t nsimm[2];

static void getgen(int gen, int c)
{
	uint32_t temp, temp2;

	switch (gen & 0x1F)
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

	case 0x10: /*Frame memory relative*/
		temp = getdisp();
		temp2 = getdisp();
		genaddr[c] = readmemw(fp + temp) | (readmemw(fp + temp + 2) << 16);
		genaddr[c] += temp2;
		gentype[c] = 0;
		break;

	case 0x11: /*Stack memory relative*/
		temp = getdisp();
		temp2 = getdisp();
		genaddr[c] = readmemw(sp[SP] + temp)
				| (readmemw(sp[SP] + temp + 2) << 16);
		genaddr[c] += temp2;
		gentype[c] = 0;
		break;

	case 0x12: /*Static memory relative*/
		temp = getdisp();
		temp2 = getdisp();
		genaddr[c] = readmemw(sb + temp) | (readmemw(sb + temp + 2) << 16);
		genaddr[c] += temp2;
		gentype[c] = 0;
		break;

	case 0x14: /*Immediate*/
		/*                genaddr[c]=pc;
		 gentype[c]=0;*/
		gentype[c] = 1;
		genaddr[c] = (uint32_t) & nsimm[c];
		/*Why can't they just decided on an endian and then stick to it?*/
		if (isize == 1)
			nsimm[c] = readmemb(pc);
		else if (isize == 2)
			nsimm[c] = (readmemb(pc) << 8) | readmemb(pc + 1);
		else
			nsimm[c] = (readmemb(pc) << 24) | (readmemb(pc + 1) << 16)
					| (readmemb(pc + 2) << 8) | readmemb(pc + 3);
		pc += isize;
		break;

	case 0x15: /*Absolute*/
		gentype[c] = 0;
		genaddr[c] = getdisp();
		break;

	case 0x16: /*External*/
		gentype[c] = 0;
		temp = readmemw(mod + 4) + (readmemw(mod + 6) << 16);
		temp += getdisp();
		temp2 = readmemw(temp) + (readmemw(temp + 2) << 16);
		genaddr[c] = temp2 + getdisp();
		break;

	case 0x17: /*Stack*/
		gentype[c] = 0;
		sdiff[c] = isize;
		genaddr[c] = sp[SP];
		/*                if (c)
		 {
		 sp[SP]-=isize;
		 genaddr[c]=sp[SP];
		 }
		 else
		 {
		 genaddr[c]=sp[SP];
		 sp[SP]+=isize;
		 }*/
		break;

	case 0x18: /*FP relative*/
		gentype[c] = 0;
		genaddr[c] = getdisp() + fp;
		break;
	case 0x19: /*SP relative*/
		gentype[c] = 0;
		genaddr[c] = getdisp() + sp[SP];
		break;
	case 0x1A: /*SB relative*/
		gentype[c] = 0;
		genaddr[c] = getdisp() + sb;
		break;
	case 0x1B: /*PC relative*/
		gentype[c] = 0;
		genaddr[c] = getdisp() + startpc;
		break;

	case 0x1C: /*EA + Rn*/
		getgen(genindex[c] >> 3, c);
		if (!gentype[c])
			genaddr[c] += r[genindex[c] & 7];
		else
			genaddr[c] = *(uint32_t *) genaddr[c] + r[genindex[c] & 7];
		gentype[c] = 0;
		break;

	case 0x1D: /*EA + Rn*2*/
		getgen(genindex[c] >> 3, c);
		if (!gentype[c])
			genaddr[c] += (r[genindex[c] & 7] * 2);
		else
			genaddr[c] = *(uint32_t *) genaddr[c] + (r[genindex[c] & 7] * 2);
		gentype[c] = 0;
		break;

	default:
		printf("Bad NS32016 gen mode %02X\n", gen & 0x1F);
		n32016_dumpregs();
	}
}

#define readgenb(c,temp)        if (gentype[c]) temp=*(uint8_t *)genaddr[c]; \
                                else \
                                { \
                                        temp=readmemb(genaddr[c]); \
                                        if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]+sdiff[c]; \
                                }

#define readgenw(c,temp)        if (gentype[c]) temp=*(uint16_t *)genaddr[c]; \
                                else \
                                { \
                                        temp=readmemw(genaddr[c]); \
                                        if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]+sdiff[c]; \
                                }

#define readgenl(c,temp)        if (gentype[c]) temp=*(uint32_t *)genaddr[c]; \
                                else \
                                { \
                                        temp=readmemw(genaddr[c])|(readmemw(genaddr[c]+2)<<16); \
                                        if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]+sdiff[c]; \
                                }

#define readgenq(c,temp)        if (gentype[c]) temp=*(uint64_t *)genaddr[c]; \
                                else \
                                { \
                                        temp=readmemw(genaddr[c])|(readmemw(genaddr[c]+2)<<16); \
                                        if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]+sdiff[c]; \
                                        temp|=((readmemw(genaddr[c])|(readmemw(genaddr[c]+2)<<16))<<16); \
                                        if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]+sdiff[c]; \
                                }

#define writegenb(c,temp)       if (gentype[c]) *(uint8_t *)genaddr[c]=temp; \
                                else \
                                { \
                                        if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]-sdiff[c]; \
                                        writememb(genaddr[c],temp); \
                                }

#define writegenw(c,temp)		if (gentype[c]) *(uint16_t*) genaddr[c]=temp; \
                                else \
                                { \
											if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]-sdiff[c]; \
                                        writememw(genaddr[c],temp); \
                                }

#define writegenl(c,temp)       if (gentype[c]) *(uint32_t *)genaddr[c]=temp; \
                                else \
                                { \
                                        if (sdiff[c]) genaddr[c]=sp[SP]=sp[SP]-sdiff[c]; \
                                        writememw(genaddr[c],temp); \
                                        writememw(genaddr[c]+2,temp>>16); \
                                }

static uint16_t oldpsr;

void n32016_exec(uint32_t tubecycles)
{
	uint32_t opcode;
	uint32_t temp = 0, temp2, temp3, temp4;
	uint64_t temp64;
	int c;

	while (tubecycles > 0)
	{
		sdiff[0] = sdiff[1] = 0;
		startpc = pc;
		opcode = readmemb(pc);

		ShowInstruction(startpc);

		if (startpc == 0x1CB2)
		{
			n32016_dumpregs();
			printf("Epic Fail!\n");
		}

		pc++;
		isize = ilook[opcode & 3];
		switch (opcode)
		{
		case 0x0E: // String instruction
			opcode |= (readmemb(pc) << 8);
			pc++;
			opcode |= (readmemb(pc) << 16);
			pc++;
			temp2 = (opcode >> 15) & 0xF;
			switch ((opcode >> 8) & 0x3F)
			{
			case 0x00: /*MOVS byte*/
				if (temp2 & 3)
				{
					printf("Bad NS32016 MOVS %02X %04X %01X\n", (opcode >> 15) & 0xF,
							opcode, (opcode >> 15) & 0xF);
					n32016_dumpregs();
				}

				while (r[0])
				{
					temp = readmemb(r[1]);
					r[1]++;
					if ((temp2 & 0xC) == 0xC && temp == r[4])
					{
						break;
					}
					if ((temp2 & 0xC) == 0x4 && temp != r[4])
					{
						break;
					}
					writememb(r[2], temp);
					r[2]++;
					r[0]--;
				}
				break;

			case 0x03: /*MOVS dword*/
				if (temp2)
				{
					printf("Bad NS32016 MOVS %02X %04X %01X\n", (opcode >> 15) & 0xF,
							opcode, (opcode >> 15) & 0xF);
					n32016_dumpregs();
					break;
				}
				while (r[0])
				{
					temp = readmemw(r[1]);
					temp |= (readmemw(r[1] + 2) << 16);
					r[1] += 4;
					writememw(r[2], temp);
					writememw(r[2] + 2, temp >> 16);
					r[2] += 4;
					r[0]--;
				}
				break;

			case 0x0B: /*SETCFG*/
				nscfg = temp;
				break;

			default:
				printf("Bad NS32016 0E opcode %02X %04X %01X\n",
						(opcode >> 8) & 0x3F, opcode, (opcode >> 15) & 0xF);
				n32016_dumpregs();
				break;
			}
			break;

		CASE2(0x1C): // CMPQ byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp2 = (opcode >> 7) & 0xF;
			if (temp2 & 8)
				temp2 |= 0xFFFFFFF0;
			readgenb(0, temp);
			psr &= ~(Z_FLAG | N_FLAG | L_FLAG);
			if (temp == temp2)
				psr |= Z_FLAG;
			if (temp2 > temp)
				psr |= L_FLAG;
			if (((signed char) temp2) > ((signed char) temp))
				psr |= N_FLAG;
			break;

		CASE2(0x1F): // CMPQ dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp2 = (opcode >> 7) & 0xF;
			if (temp2 & 8)
				temp2 |= 0xFFFFFFF0;
			readgenl(0, temp);
			psr &= ~(Z_FLAG | N_FLAG | L_FLAG);
			if (temp == temp2)
				psr |= Z_FLAG;
			if (temp2 > temp)
				psr |= L_FLAG;
			if (((signed long) temp2) > ((signed long) temp))
				psr |= N_FLAG;
			break;

		CASE2(0x5C): // MOVQ byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp = (opcode >> 7) & 0xF;
			if (temp & 8)
				temp |= 0xFFFFFFF0;
			writegenb(0, temp);
			break;

		CASE2(0x5D): // MOVQ word
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp = (opcode >> 7) & 0xF;
			if (temp & 8)
				temp |= 0xFFFFFFF0;
			writegenw(0, temp);
			break;

		CASE2(0x5F): // MOVQ dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp = (opcode >> 7) & 0xF;
			if (temp & 8)
				temp |= 0xFFFFFFF0;
			writegenl(0, temp);
			break;

		CASE2(0x0C): // ADDQ byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp2 = (opcode >> 7) & 0xF;
			if (temp2 & 8)
				temp2 |= 0xFFFFFFF0;
			readgenb(0, temp);
			psr &= ~(C_FLAG | V_FLAG);
			if ((temp + temp2) & 0x100)
				psr |= C_FLAG;
			if ((temp ^ (temp + temp2)) & (temp2 ^ (temp + temp2)) & 0x80)
				psr |= V_FLAG;
			temp += temp2;
			writegenb(0, temp);
			break;

		CASE2(0x0D): // ADDQ word
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp2 = (opcode >> 7) & 0xF;
			if (temp2 & 8)
				temp2 |= 0xFFFFFFF0;
			readgenw(0, temp);
			psr &= ~(C_FLAG | V_FLAG);
			if ((temp + temp2) & 0x10000)
				psr |= C_FLAG;
			if ((temp ^ (temp + temp2)) & (temp2 ^ (temp + temp2)) & 0x8000)
				psr |= V_FLAG;
			temp += temp2;
			writegenw(0, temp);
			break;

		CASE2(0x0F): // ADDQ dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp2 = (opcode >> 7) & 0xF;
			if (temp2 & 8)
				temp2 |= 0xFFFFFFF0;
			readgenl(0, temp);
			psr &= ~(C_FLAG | V_FLAG);
			if ((temp + temp2) < temp)
				psr |= C_FLAG;
			if ((temp ^ (temp + temp2)) & (temp2 ^ (temp + temp2)) & 0x80000000)
				psr |= V_FLAG;
			temp += temp2;
			writegenl(0, temp);
			break;

		CASE2(0x3C): // ScondB
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
//                        readgenb(0,temp);
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
				if (!(psr & (L_FLAG | Z_FLAG)))
					temp = 1;
				break;
			case 0x9:
				if (psr & (L_FLAG | Z_FLAG))
					temp = 1;
				break;
			case 0xA:
				if (!(psr & (N_FLAG | Z_FLAG)))
					temp = 1;
				break;
			case 0xB:
				if (psr & (N_FLAG | Z_FLAG))
					temp = 1;
				break;
			case 0xC:
				if (psr & Z_FLAG)
					temp = 1;
				break;
			case 0xD:
				if (!(psr & Z_FLAG))
					temp = 1;
				break;
			case 0xE:
				temp = 1;
				break;
			case 0xF:
				break;
			}
			writegenb(0, temp);
			break;

		CASE2(0x3D) : // ScondW
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			//                        readgenb(0,temp);
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
				if (!(psr & (L_FLAG | Z_FLAG)))
					temp = 1;
				break;
			case 0x9:
				if (psr & (L_FLAG | Z_FLAG))
					temp = 1;
				break;
			case 0xA:
				if (!(psr & (N_FLAG | Z_FLAG)))
					temp = 1;
				break;
			case 0xB:
				if (psr & (N_FLAG | Z_FLAG))
					temp = 1;
				break;
			case 0xC:
				if (psr & Z_FLAG)
					temp = 1;
				break;
			case 0xD:
				if (!(psr & Z_FLAG))
					temp = 1;
				break;
			case 0xE:
				temp = 1;
				break;
			case 0xF:
				break;
			}
			writegenw(0, temp);
			break;

		CASE2(0x3F) : // ScondD
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			//                        readgenb(0,temp);
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
				if (!(psr & (L_FLAG | Z_FLAG)))
					temp = 1;
				break;
			case 0x9:
				if (psr & (L_FLAG | Z_FLAG))
					temp = 1;
				break;
			case 0xA:
				if (!(psr & (N_FLAG | Z_FLAG)))
					temp = 1;
				break;
			case 0xB:
				if (psr & (N_FLAG | Z_FLAG))
					temp = 1;
				break;
			case 0xC:
				if (psr & Z_FLAG)
					temp = 1;
				break;
			case 0xD:
				if (!(psr & Z_FLAG))
					temp = 1;
				break;
			case 0xE:
				temp = 1;
				break;
			case 0xF:
				break;
			}
			writegenl(0, temp);
			break;
			
		CASE2(0x4C) : // ACBB
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp2 = (opcode >> 7) & 0xF;
			if (temp2 & 8)
				temp2 |= 0xFFFFFFF0;
			readgenb(0, temp);
			temp += temp2;
			writegenb(0, temp);
			temp2 = getdisp();
			if (temp & 0xFF)
				pc = startpc + temp2;
			break;

		CASE2(0x4F): // ACBD
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			temp2 = (opcode >> 7) & 0xF;
			if (temp2 & 8)
				temp2 |= 0xFFFFFFF0;
			readgenl(0, temp);
			temp += temp2;
			writegenl(0, temp);
			temp2 = getdisp();
			if (temp)
				pc = startpc + temp2;
			break;

		CASE4(0x00): // ADD byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			readgenb(1, temp2);
			psr &= ~(C_FLAG | V_FLAG);
			if ((temp + temp2) & 0x100)
				psr |= C_FLAG;
			if ((temp ^ (temp + temp2)) & (temp2 ^ (temp + temp2)) & 0x80)
				psr |= V_FLAG;
			temp2 += temp;
			writegenb(1, temp2);
			break;

		CASE4(0x03): // ADD dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp);
			readgenl(1, temp2);
			psr &= ~(C_FLAG | V_FLAG);
			if ((temp + temp2) < temp)
				psr |= C_FLAG;
			if ((temp ^ (temp + temp2)) & (temp2 ^ (temp + temp2)) & 0x80000000)
				psr |= V_FLAG;
			temp2 += temp;
			writegenl(1, temp2);
			break;

		CASE4(0x04): // CMP byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			readgenb(1, temp2);
			psr &= ~(Z_FLAG | N_FLAG | L_FLAG);
			if (temp == temp2)
				psr |= Z_FLAG;
			if (temp > temp2)
				psr |= L_FLAG;
			if (((signed char) temp) > ((signed char) temp2))
				psr |= N_FLAG;
			break;

		CASE4(0x07): // CMP dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			nsoutput |= 2;
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			nsoutput &= ~2;
			readgenl(0, temp);
			readgenl(1, temp2);
			psr &= ~(Z_FLAG | N_FLAG | L_FLAG);
			if (temp == temp2)
				psr |= Z_FLAG;
			if (temp > temp2)
				psr |= L_FLAG;
			if (((signed long) temp) > ((signed long) temp2))
				psr |= N_FLAG;
			break;

		CASE4(0x08): // BIC byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			readgenb(1, temp2);
			temp2 &= ~temp;
			writegenb(1, temp2);
			break;

		CASE4(0x09): // BIC word
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenw(0, temp);
			readgenw(1, temp2);
			temp2 &= ~temp;
			writegenw(1, temp2);
			break;

		CASE4(0x0B): // BIC dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp);
			readgenl(1, temp2);
			temp2 &= ~temp;
			writegenl(1, temp2);
			break;

		CASE4(0x14): // MOV byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			writegenb(1, temp);
			break;
		CASE4(0x15): // MOV word
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenw(0, temp);
			writegenw(1, temp);
			break;

		CASE4(0x17): // MOV dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp);
			writegenl(1, temp);
			break;

		CASE4(0x18): //OR byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			readgenb(1, temp2);
			temp2 |= temp;
			writegenb(1, temp2);
			break;

		CASE4(0x19): // OR word
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenw(0, temp);
			readgenw(1, temp2);
			temp2 |= temp;
			writegenw(1, temp2);
			break;

		CASE4(0x1B): // OR dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp);
			readgenl(1, temp2);
			temp2 |= temp;
			writegenl(1, temp2);
			break;

		CASE4(0x38): // XOR byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			readgenb(1, temp2);
			temp2 ^= temp;
			writegenb(1, temp2);
			break;

		CASE4(0x39): // XOR word
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenw(0, temp);
			readgenw(1, temp2);
			temp2 ^= temp;
			writegenw(1, temp2);
			break;

		CASE4(0x3B) : // XOR dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp);
			readgenl(1, temp2);
			temp2 ^= temp;
			writegenl(1, temp2);
			break;

		CASE4(0x23): // SUB dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp)
			;
			readgenl(1, temp2)
			;
			psr &= ~(C_FLAG | V_FLAG);
			if ((temp + temp2) > temp)
				psr |= C_FLAG;
			if ((temp ^ temp2) & (temp ^ (temp + temp2)) & 0x80000000)
				psr |= V_FLAG;
			temp2 -= temp;
			writegenl(1, temp2)
			;
			break;

		CASE4(0x27): // ADDR dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			writegenl(1, genaddr[0]);
			break;

		CASE4(0x28): // AND byte
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			readgenb(1, temp2);
			temp2 &= temp;
			writegenb(1, temp2);
			break;

		CASE4(0x29): // AND word
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenw(0, temp);
			readgenw(1, temp2);
			temp2 &= temp;
			writegenw(1, temp2);
			break;

		CASE4(0x2B): // AND dword
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp);
			readgenl(1, temp2);
			temp2 &= temp;
			writegenl(1, temp2);
			break;

		CASE4(0x34): // TBITB
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenb(0, temp);
			readgenb(1, temp2);
			psr &= ~F_FLAG;
			temp &= 7;
			if (temp2 & (1 << temp))
				psr |= F_FLAG;
			break;

		CASE4(0x37): // TBITD
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			readgenl(0, temp);
			readgenl(1, temp2);
			psr &= ~F_FLAG;
			temp &= 31;
			if (temp2 & (1 << temp))
				psr |= F_FLAG;
			break;

		case 0x4E: /*Type 6*/
			opcode = readmemb(pc);
			pc++;
			opcode |= (readmemb(pc) << 8);
			pc++;
			isize = ilook[opcode & 3];
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			if ((opcode & 0x3F) == 0x17)
				isize = 1;
			getgen(opcode >> 11, 0);
			isize = ilook[opcode & 3];
			getgen(opcode >> 6, 1);
			switch (opcode & 0x3F)
			{
			case 8: /*CBITB*/
				readgenb(0, temp);
				temp &= 31;
				if (gentype[1])
				{
					readgenl(1, temp2);
				}
				else
				{
					readgenb(1, temp2);
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
				break;
			case 0x17: /*LSHD*/
				readgenb(0, temp);
				if (temp & 0xE0)
					temp |= 0xE0;
				readgenl(1, temp2);
				if (temp & 0xE0)
					temp2 >>= ((temp ^ 0xFF) + 1);
				else
					temp2 <<= temp;
				writegenl(1, temp2);
				break;

			case 0x24: // NOTB
				{
					readgenb(0, temp);
					temp ^= 1;
					writegenb(1, temp);
				}	
				break;

			case 0x25: // NOTW
				{
					readgenw(0, temp);
					temp ^= 1;
					writegenw(1, temp);
				}
				break;

			case 0x27: // NOTD
				{
					readgenl(0, temp);
					temp ^= 1;
					writegenl(1, temp);
				}
				break;

			case 0x30: /*ABSB*/
				readgenb(0, temp);
				if (temp & 0x80)
					temp = (temp ^ 0xFF) + 1;
				writegenb(1, temp);
				break;

			case 0x34: /*COMB*/
				readgenb(0, temp);
				writegenb(1, ~temp);
				break;

			case 0x35: // COMW
				readgenw(0, temp);
				writegenw(1, ~temp);
				break;

			case 0x37: /// COMD
				readgenl(0, temp);
				writegenl(1, ~temp);
				break;

			default:
				printf("Bad NS32016 4E opcode %04X %01X\n", opcode, opcode & 0x3F);
				n32016_dumpregs();
				break;
			}
			break;

		case 0x7C: /*Type 3 byte*/
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			switch ((opcode >> 7) & 0xF)
			{
			case 2: /*BICPSR*/
				readgenb(0, temp);
				psr &= ~temp;
				break;
			case 6: /*BISPSR*/
				readgenb(0, temp);
				psr |= temp;
				break;
			case 0xA: /*ADJSP*/
				readgenb(0, temp2);
				if (temp2 & 0x80)
					temp2 |= 0xFFFFFF00;
				sp[SP] -= temp2;
				break;
			case 0xE: /*CASE*/
				readgenb(0, temp);
				if (temp & 0x80)
					temp |= 0xFFFFFF00;
				pc = startpc + temp;
				break;

			default:
				printf("Bad NS32016 7C opcode %04X %01X\n", opcode,
						(opcode >> 7) & 0xF);
				n32016_dumpregs();
				break;
			}
			break;

		case 0x7D: /*Type 3 word*/
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			switch ((opcode >> 7) & 0xF)
			{
			case 2: /*BICPSR*/
				readgenw(0, temp);
				psr &= ~temp;
				break;
			case 6: /*BISPSR*/
				readgenw(0, temp);
				psr |= temp;
				break;
			case 0xA: /*ADJSP*/
				readgenw(0, temp2);
				if (temp & 0x8000)
					temp |= 0xFFFF0000;
				sp[SP] -= temp2;
				break;
			case 0xE: /*CASE*/
				readgenw(0, temp);
				if (temp & 0x8000)
					temp |= 0xFFFF0000;
				pc = startpc + temp;
				break;

			default:
				printf("Bad NS32016 7D opcode %04X %01X\n", opcode,
						(opcode >> 7) & 0xF);
				n32016_dumpregs();
				break;
			}
			break;

		case 0x7F: /*Type 3 dword*/
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			switch ((opcode >> 7) & 0xF)
			{
			case 0: /*CXPD*/
				readgenl(0, temp);
				pushw(0);
				pushw(mod);
				pushd(pc);
				mod = temp & 0xFFFF;
				temp3 = temp >> 16;
				sb = readmemw(mod) | (readmemw(mod + 2) << 16);
				temp2 = readmemw(mod + 8) | (readmemw(mod + 10) << 16);
				pc = temp2 + temp3;
				break;
			case 4: /*JUMP*/
				if (gentype[0])
					pc = *(uint32_t *) genaddr[0];
				else
					pc = genaddr[0];
				break;
			case 0xA: /*ADJSP*/
				readgenl(0, temp2);
				sp[SP] -= temp2;
				break;

			default:
				printf("Bad NS32016 7F opcode %04X %01X\n", opcode,
						(opcode >> 7) & 0xF);
				n32016_dumpregs();
			}
			break;

		CASE2(0x2F): // SPR*
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
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
				printf("Bad SPR reg %01X\n", (opcode >> 7) & 0xF);
				n32016_dumpregs();
			}
			break;

		CASE2(0x6C): // LPRB
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			readgenb(0, temp);
			switch ((opcode >> 7) & 0xF)
			{
			case 0:
				psr = (psr & 0xFF00) | (temp & 0xFF);
				break;
			case 9:
				sp[SP] = temp;
				break;
			default:
				printf("Bad LPRB reg %01X\n", (opcode >> 7) & 0xF);
				n32016_dumpregs();
			}
			break;

		CASE2(0x6D): // LPRW
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			readgenw(0, temp);
			switch ((opcode >> 7) & 0xF)
			{
			case 15:
				mod = temp;
				break;
			default:
				printf("Bad LPRW reg %01X\n", (opcode >> 7) & 0xF);
				n32016_dumpregs();
			}
			break;

		CASE2(0x6F): // LPRD
			opcode |= (readmemb(pc) << 8);
			pc++;
			getgen1(opcode >> 11, 0);
			getgen(opcode >> 11, 0);
			readgenl(0, temp)
			;
			switch ((opcode >> 7) & 0xF)
			{
			case 9:
				sp[SP] = temp;
				break;
			case 0xA:
				sb = temp;
				break;
			case 0xE:
				intbase = temp; /*printf("INTBASE %08X %08X\n",temp,pc); */
				break;

			default:
				printf("Bad LPRD reg %01X\n", (opcode >> 7) & 0xF);
				n32016_dumpregs();
				break;
			}
			break;

		case 0xCE: /*Format 7*/
			opcode = readmemb(pc);
			pc++;
			opcode |= (readmemb(pc) << 8);
			pc++;
			isize = ilook[opcode & 3];
			getgen1(opcode >> 11, 0);
			getgen1(opcode >> 6, 1);
			getgen(opcode >> 11, 0);
			getgen(opcode >> 6, 1);
			switch (opcode & 0x3F)
			{
			case 0x00: /*MOVMB*/
				temp = getdisp();
				while (temp)
				{
					temp2 = readmemb(genaddr[0]);
					genaddr[0]++;
					writememb(genaddr[1], temp2);
					genaddr[1]++;
					temp--;
				}
				break;
			case 0x08: /*INSSB*/
				temp3 = readmemb(pc);
				pc++;
				readgenb(0, temp);
				readgenb(1, temp2);
				for (c = 0; c <= (temp3 & 0x1F); c++)
				{
					temp2 &= ~(1 << ((c + (temp3 >> 5)) & 7));
					if (temp & (1 << c))
						temp2 |= (1 << ((c + (temp3 >> 5)) & 7));
				}
				writegenb(1, temp2);
				break;

			case 0x0C: // EXTSB
				temp3 = readmemb(pc);
				pc++;
				readgenb(0, temp);														// Load the source

				printf("EXTSB Source = %02X Shift = %u Bit Count = %u ", temp, temp3 >> 5, ((temp3 & 0x1F) + 1));
				temp2 = 0;
				temp >>= (temp3 >> 5);													// Shift by offset
				temp3 &= 0x1F;																// Mask off the lower 5 Bits which are number of bits to extract

				temp4 = 1;
				for (c = 0; c <= temp3; c++)
				{
					if (temp & temp4)														// Copy the ones
					{
						temp2 |= temp4;
					}

					temp4 <<= 1;
				}
				printf("Result = %02X\n", temp2);

				writegenb(1, temp2);
				break;

			case 0x18: /*MOVZBD*/
				readgenb(0, temp);
				if (sdiff[1])
					sdiff[1] = 4;
				writegenl(1, temp)
				break;
			case 0x19: /*MOVZWD*/
				readgenw(0, temp);
				if (sdiff[1])
					sdiff[1] = 4;
				writegenl(1, temp)
				break;

			case 0x2F: /*DEID*/
				readgenl(0, temp);
				readgenq(1, temp64);
				if (!temp)
				{
					printf("Divide by zero - DEID CE\n");
					n32016_dumpregs();
					break;
				}
				temp3 = temp64 % temp;
				writegenl(1, temp3);
				temp3 = (uint32_t) (temp64 / temp);
				if (gentype[1])
					*(uint32_t *) (genaddr[1] + 4) = temp3;
				else
				{
					writememw(genaddr[1] + 4, temp3);
					writememw(genaddr[1] + 4 + 2, temp3 >> 16);
				}
				break;

			case 0x33: /*QUOD*/
				readgenl(0, temp);
				readgenl(1, temp2);
				if (!temp)
				{
					printf("Divide by zero - QUOD CE\n");
					n32016_dumpregs();
					break;
				}
				temp2 /= temp;
				writegenl(1, temp2);
				break;

			case 0x37: /*REMD*/
				readgenl(0, temp);
				readgenl(1, temp2);

				if (!temp)
				{
					printf("Divide by zero - QUOD CE\n");
					n32016_dumpregs();
					break;
				}
				temp2 %= temp;
				writegenl(1, temp2);
				break;
			default:
				printf("Bad NS32016 CE opcode %04X %01X\n", opcode, opcode & 0x3F);
				n32016_dumpregs();
				break;
			}
			break;

		CASE4(0x2E): // Type 8
			opcode |= (readmemb(pc) << 8);
			pc++;
			opcode |= (readmemb(pc) << 16);
			pc++;
			getgen1(opcode >> 19, 0);
			getgen1(opcode >> 14, 1);
			getgen(opcode >> 19, 0);
			getgen(opcode >> 14, 1);
			temp = ((opcode >> 6) & 3) | ((opcode & 0x400) >> 8);
			temp = (temp << 2) | ((opcode >> 8) & 3);
			switch (temp)
			{
			case 0: /*EXT*/
				temp = r[(opcode >> 11) & 7] & 31;
				temp2 = getdisp();
				readgenl(0, temp3);
				temp4 = 0;
				for (c = 0; c < temp2; c++)
				{
					if (temp3 & (1 << ((c + temp) & 31)))
						temp4 |= (1 << c);
				}
				writegenl(1, temp4);
				break;
			case 0xC: /*CHECKB*/
				readgenb(1, temp3);
				temp = readmemb(genaddr[0]);
				temp2 = readmemb(genaddr[0] + 1);
				if (temp >= temp3 && temp3 >= temp2)
				{
					r[(opcode >> 11) & 7] = temp3 - temp2;
					psr &= ~F_FLAG;
				}
				else
					psr |= F_FLAG;
				break;
			default:
				printf("Bad NS32016 Type 8 opcode %04X %01X %i\n", opcode, temp,
						(opcode >> 11) & 7);
				n32016_dumpregs();
			}
			break;

		case 0x02: /*BSR*/
			temp = getdisp();
			pushd(pc);
			pc = startpc + temp;
			break;

		case 0x12: /*RET*/
			temp = getdisp();
			pc = popd();
			sp[SP] += temp;
			break;

		case 0x22: /*CXP*/
			temp = getdisp();
			pushw(0);
			pushw(mod);
			pushd(pc);
			temp2 = readmemw(mod + 4) + (readmemw(mod + 6) << 16) + (4 * temp);
			temp = readmemw(temp2) + (readmemw(temp2 + 2) << 16);
			mod = temp & 0xFFFF;
			sb = readmemw(mod) + (readmemw(mod + 2) << 16);
			pc = readmemw(mod + 8) + (readmemw(mod + 10) << 16) + (temp >> 16);
			break;

		case 0x32: /*RXP*/
			temp = getdisp();
			pc = popd();
			temp2 = popd();
			mod = temp2 & 0xFFFF;
			sp[SP] += temp;
			sb = readmemw(mod) | (readmemw(mod + 2) << 16);
			break;

		case 0x42: /*RETT*/
			temp = getdisp();
			pc = popd();
			mod = popw();
			psr = popw();
			sp[SP] += temp;
			sb = readmemw(mod) | (readmemw(mod + 2) << 16);
			break;

		case 0x62: /*SAVE*/
			temp = readmemb(pc);
			pc++;
			for (c = 0; c < 8; c++)
			{
				if (temp & (1 << c))
				{
					pushd(r[c]);
				}
			}
			break;

		case 0x72: /*RESTORE*/
			temp = readmemb(pc);
			pc++;
			for (c = 0; c < 8; c++)
			{
				if (temp & (1 << c))
				{
					r[c ^ 7] = popd(r[c]);
				}
			}
			break;

		case 0x82: /*ENTER*/
			temp = readmemb(pc);
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

		case 0x92: /*EXIT*/
			temp = readmemb(pc);
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

		case 0xE2: /*SVC*/
			temp = psr;
			psr &= ~0x700;
			temp = readmemw(intbase + (5 * 4))
					| (readmemw(intbase + (5 * 4) + 2) << 16);
			mod = temp & 0xFFFF;
			temp3 = temp >> 16;
			sb = readmemw(mod) | (readmemw(mod + 2) << 16);
			temp2 = readmemw(mod + 8) | (readmemw(mod + 10) << 16);
			pc = temp2 + temp3;
			break;

		case 0x0A: /*BEQ*/
			temp = getdisp();
			if (psr & Z_FLAG)
				pc = startpc + temp;
			break;

		case 0x1A: /*BNE*/
			temp = getdisp();
			if (!(psr & Z_FLAG))
				pc = startpc + temp;
			break;

		case 0x4A: /*BH*/
			temp = getdisp();
			if (psr & L_FLAG)
				pc = startpc + temp;
			break;

		case 0x5A: /*BLS*/
			temp = getdisp();
			if (!(psr & L_FLAG))
				pc = startpc + temp;
			break;

		case 0x6A: /*BGT*/
			temp = getdisp();
			if (psr & N_FLAG)
				pc = startpc + temp;
			break;

		case 0x7A: /*BLE*/
			temp = getdisp();
			if (!(psr & N_FLAG))
				pc = startpc + temp;
			break;

		case 0x8A: /*BFS*/
			temp = getdisp();
			if (psr & F_FLAG)
				pc = startpc + temp;
			break;

		case 0x9A: /*BFC*/
			temp = getdisp();
			if (!(psr & F_FLAG))
				pc = startpc + temp;
			break;

		case 0xAA: /*BLO*/
			temp = getdisp();
			if (!(psr & (L_FLAG | Z_FLAG)))
				pc = startpc + temp;
			break;

		case 0xBA: /*BHS*/
			temp = getdisp();
			if (psr & (L_FLAG | Z_FLAG))
				pc = startpc + temp;
			break;

		case 0xCA: /*BLT*/
			temp = getdisp();
			if (!(psr & (N_FLAG | Z_FLAG)))
				pc = startpc + temp;
			break;

		case 0xDA: /*BGE*/
			temp = getdisp();
			if (psr & (N_FLAG | Z_FLAG))
				pc = startpc + temp;
			break;

		case 0xEA: /*BR*/
			pc = startpc + getdisp();
			break;

		default:
			printf("Bad NS32016 opcode %02X\n", opcode);
			n32016_dumpregs();
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
			temp = readmemw(intbase + (1 * 4))
					| (readmemw(intbase + (1 * 4) + 2) << 16);
			mod = temp & 0xFFFF;
			temp3 = temp >> 16;
			sb = readmemw(mod) | (readmemw(mod + 2) << 16);
			temp2 = readmemw(mod + 8) | (readmemw(mod + 10) << 16);
			pc = temp2 + temp3;
		}

		if ((tube_irq & 1) && (psr & 0x800))
		{
			temp = psr;
			psr &= ~0xF00;
			pushw(temp);
			pushw(mod);
			pushd(pc);
			temp = readmemw(intbase) | (readmemw(intbase + 2) << 16);
			mod = temp & 0xFFFF;
			temp3 = temp >> 16;
			sb = readmemw(mod) | (readmemw(mod + 2) << 16);
			temp2 = readmemw(mod + 8) | (readmemw(mod + 10) << 16);
			pc = temp2 + temp3;
		}

		oldpsr = psr;
	}
}
