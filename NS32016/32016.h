// Some meaningful memory sizes
#define K128      0x0020000
#define K256      0x0040000
#define K512      0x0080000
#define K640      0x00A0000
#define MEG1      0x0100000
#define MEG2      0x0200000
#define MEG4      0x0400000
// I think the 32016 CoPro could support 16256K Bytes of RAM ;)
#define MAX_RAM   (MEG16 - K128)
#define MEG16     0x1000000

#define MEM_MASK 0xFFFFFF
#define RAM_SIZE 0x100000
//#define RAM_SIZE 0x400000

#define CASE2(in) case (in): case ((in) | 0x80)
#define CASE4(in) case (in): case ((in) | 0x40): case ((in) | 0x80): case ((in) | 0xC0)

#define C_FLAG 0x01
#define T_FLAG 0x02
#define L_FLAG 0x04
#define F_FLAG 0x20
#define V_FLAG 0x20
#define Z_FLAG 0x40
#define N_FLAG 0x80

#define U_FLAG 0x100
#define S_FLAG 0x200
#define P_FLAG 0x400
#define I_FLAG 0x800

enum Formats
{
	Format0,
	Format1,
	Format2,
	Format3,
	Format4,
	Format5,
	Format6,
	Format7,
	Format8,
   Format9,
   Format10,
   Format11,
   Format12,
   Format13,
   Format14,
   FormatCount,
	FormatBad = 0xFF
};

#define F_BASE(in) ((in) << 4)
enum Functions
{
   BEQ = F_BASE(Format0),
   BNE,
   TRAP_F1_02,
   TRAP_F1_03,
   BH,
   BLS,
   BGT,
   BLE,
   BFS,
   BFC,
   BLO,
   BHS,
   BLT,
   BGE,
   BR,

   BSR = F_BASE(Format1),
   RET,
   CXP,
   RXP,
   RETT,
   RETI,
   SAVE,
   RESTORE,
   ENTER,
   EXIT,
   NOP,
   WAIT,
   DIA,
   FLAG,
   SVC,
   BPT,

   ADDQ = F_BASE(Format2),
   CMPQ,
   SPR,
   Scond,
   ACB,
   MOVQ,
   LPR,

   CXPD = F_BASE(Format3),
   TRAP_F3_0001,
   BICPSR,
   TRAP_F3_0011,
   JUMP,
   TRAP_F3_0101,
   BISPSR,
   TRAP_F3_0111,
   TRAP_F3_1000,
   TRAP_F3_1001,
   ADJSP,
   TRAP_F3_1011,
   JSR,
   TRAP_F3_1101,
   CASE,
   TRAP_F3_1111,

   ADD = F_BASE(Format4),
   CMP,
   BIC,
   TRAP_F4_0011,
   ADDC,
   MOV,
   OR,
   TRAP_F4_0111,
   SUB,
   ADDR,
   AND,
   TRAP_F4_1011,
   SUBC,
   TBIT,
   XOR,

   MOVS = F_BASE(Format5),
   CMPS,
   SETCFG,
   SKPS,

   ROT = F_BASE(Format6),
   ASH,
   CBIT,
   CBITI,
   TRAP_F5_0100,
   LSH,
   SBIT,
   SBITI,
   NEG,
   NOT,
   TRAP_F5_1010,
   SUBP,
   ABS,
   COM,
   IBIT,
   ADDP,

   MOVM = F_BASE(Format7),
   CMPM,
   INSS,
   EXTS,
   MOVXBW,
   MOVZBW,
   MOVZiD,
   MOVXiD,
   MUL,
   MEI,
   Trap,
   DEI,
   QUO,
   REM,
   MOD,
   DIV,

   EXT = F_BASE(Format8),
   CVTP,
   INS,
   CHECK,
   INDEX,
   FFS,
   MOVUS,
   MOVSU,

   MOVif = F_BASE(Format9),
   LFSR,
   MOVLF,
   MOVFL,
   ROUND,
   TRUNC,
   SFSR,
   FLOOR,
  
   ADDf = F_BASE(Format11),
   MOVf,
   CMPf,
   TRAP_F11_0011,
   SUBf,
   NEGf,
   TRAP_F11_0110,
   TRAP_F11_0111,
   DIVf,
   TRAP_F11_1001,
   TRAP_F11_1010,
   TRAP_F11_1011,
   MULf,
   ABSf,

   RDVAL = F_BASE(Format14),
   WRVAL,
   LMR,
   SMR,
   
   TRAP = F_BASE(FormatCount),
   InstructionCount,

   BAD = 0xFF
};

enum DataSize
{
	sz8 = 0,
	sz16 = 1,
	sz32 = 3,
	szVaries = 0xFF,
};

typedef union
{
	struct
	{
		uint8_t Size;
		uint8_t Function;
		uint8_t BaseSize;
		uint8_t Source;
		uint8_t Destination;		
	} p;

	uint32_t Whole;
} DecodeMatrix;

extern void n32016_init();
extern void n32016_reset(uint32_t StartAddress);
extern void n32016_exec(uint32_t tubecycles);
extern void StoreRegisters(uint8_t Index, uint8_t Value);
extern void ClearRegs(void);
extern void ShowInstruction(uint32_t pc, uint32_t opcode, uint8_t Function, uint8_t Size);
extern void n32016_dumpregs();
extern void n32016_build_matrix();

extern uint32_t tube_irq;
extern DecodeMatrix mat[256];
extern DecodeMatrix LookUp;
extern uint32_t genaddr[2];
extern int gentype[2];

