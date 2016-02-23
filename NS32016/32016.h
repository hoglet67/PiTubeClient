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

#define CASE2(in) case (in): case ((in) | 0x80):
#define CASE4(in) case (in): case ((in) | 0x40): case ((in) | 0x80): case ((in) | 0xC0):

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
	FormatBad = 0xFF
};

enum Functions
{
	BEQ,					// Format 0
	BNE,
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

	BSR,					// Format 1
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

	ADDQ,					// Format 2
	CMPQ,
	SPR,
	Scond,
	ACB,
	MOVQ,
	LPR,

	CXPD,					// Format 3
	BICPSR,
	JUMP,
	BISPSR,
  TRAP_F3_1000,
  ADJSP,
	JSR,
	CASE,

	ADD,					// Format 4
	CMP,
	BIC,
	ADDC,
	MOV,
	OR,
	SUB,
	ADDR,
	AND,
	SUBC,
	TBIT,
	XOR,

	MOVS,					// Format 5
	CMPS,
	SETCFG,
	SKPS,

  ROT,          // Format 6
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
		
	MOVM,					// Format 7
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
	
  EXT,				  // Format 8
  CVTP,
  INS,
  CHECK,
  INDEX,
  FFS,
  MOVUS,
  MOVSU,

  TRAP,
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
		uint8_t Format;
		uint8_t Size;
		uint8_t Function;
	} p;

	uint32_t Whole;
} DecodeMatrix;

extern void n32016_init();
extern void n32016_reset();
extern void n32016_exec(uint32_t tubecycles);
extern void ShowInstruction(uint32_t pc, uint8_t Function, uint8_t Size);
extern void n32016_dumpregs();

extern uint8_t ns32016ram[MEG16 + 8];
extern uint32_t tube_irq;
extern DecodeMatrix mat[256];
