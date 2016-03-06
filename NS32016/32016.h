// Some meaningful memory sizes
#define K128      0x0020000
#define K256      0x0040000
#define K512      0x0080000
#define K640      0x00A0000
#define MEG1      0x0100000
#define MEG2      0x0200000
#define MEG4      0x0400000
#define MEG8      0x0800000
#define MEG15     0x0F00000
#define MEG16     0x1000000

#define MEM_MASK 0xFFFFFF

#define CASE2(in) case (in): case ((in) | 0x80)
#define CASE4(in) case (in): case ((in) | 0x40): case ((in) | 0x80): case ((in) | 0xC0)
#define BIT(in)   (1 <<(in))

#define TEST(in) ((in) ? 1 : 0)
#define C_FLAG PR.PSR.c_flag
#define T_FLAG PR.PSR.t_flag
#define L_FLAG PR.PSR.l_flag
#define V_FLAG PR.PSR.v_flag
#define F_FLAG PR.PSR.f_flag
#define Z_FLAG PR.PSR.z_flag
#define N_FLAG PR.PSR.n_flag

#define U_FLAG PR.PSR.u_flag
#define S_FLAG PR.PSR.s_flag
#define P_FLAG PR.PSR.p_flag
#define I_FLAG PR.PSR.i_flag

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

enum OpTypes
{
   Memory,
   Register,
   TOS,
   OpImmediate
};

enum Operands
{
   R0,
   R1,
   R2,
   R3,
   R4,
   R5,
   R6,
   R7,

   R0_Offset,
   R1_Offset,
   R2_Offset,
   R3_Offset,
   R4_Offset,
   R5_Offset,
   R6_Offset,
   R7_Offset,

   FrameRelative,
   StackRelative,
   StaticRelative,
   
   IllegalOperand,
   Immediate,
   Absolute,
   External,
   TopOfStack,
   FpRelative,
   SpRelative,
   SbRelative,
   PcRelative,
   EaPlusRn,
   EaPlus2Rn,
   EaPlus4Rn,
   EaPlus8Rn
};

#define F_BASE(in) ((in) << 4)
enum Functions
{
   BEQ = F_BASE(Format0),
   BNE,
   BCS,
   BCC,
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
   BN,

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

enum OperandsPostitions
{
   Oper1,
   Oper2
};

enum DataSize
{
   szVaries = 0,
	sz8 = 1,
	sz16 = 2,
   Translating = 3,
	sz32 = 4,
};

typedef union
{
   uint8_t Op[2];
   uint16_t Whole;
} OperandSizeType;

typedef union
{
   struct
   {
      unsigned c_flag   : 1;          // 0x0001
      unsigned t_flag   : 1;          // 0x0002
      unsigned l_flag   : 1;          // 0x0004
      unsigned Bit3     : 1;          // 0x0008

      unsigned v_flag   : 1;          // 0x0010
      unsigned f_flag   : 1;          // 0x0020
      unsigned z_flag   : 1;          // 0x0040
      unsigned n_flag   : 1;          // 0x0080

      unsigned u_flag   : 1;          // 0x0100
      unsigned s_flag   : 1;          // 0x0200
      unsigned p_flag   : 1;          // 0x0400
      unsigned i_flag   : 1;          // 0x0800

      unsigned NU     : 20;
   };

   struct
   {
      uint8_t lsb;
      uint8_t msb;
      uint8_t byte3;
      uint8_t byte4;
    };

   uint32_t Whole;
} PsrType;

typedef union
{
   struct
   {
      uint16_t Lower;
      uint16_t Upper;
   };

   uint32_t Whole;
} T16In32;
   


typedef union
{
   struct
   {
      uint32_t UPSR;
      uint32_t DCR;
      uint32_t BPC;
      uint32_t DSR;
      uint32_t CAR;

      uint32_t NotUsed_0101;
      uint32_t NotUsed_0110;
      uint32_t NotUsed_0111;

      uint32_t FP;
      uint32_t SP;
      uint32_t SB;
      uint32_t USP;
      uint32_t CFG;
      PsrType  PSR;
      uint32_t INTBASE;
      T16In32  MOD;
   };

   uint32_t Direct[16];
} ProcessorRegisters;


#define fp           PR.FP
#define sb           PR.SB
#define psr          PR.PSR.Whole
#define intbase      PR.INTBASE
#define mod          PR.MOD.Whole

extern uint32_t sp[2];

#define STACK_P      sp[S_FLAG]
//#define STACK_P      PR.SP
#define SET_SP(in)   STACK_P = (in);     PrintSP("Set SP:");
#define INC_SP(in)   STACK_P += (in);    PrintSP("Inc SP:");
#define DEC_SP(in)   STACK_P -= (in);    PrintSP("Dec SP:");
#define GET_SP()     STACK_P

#define SET_OP_SIZE(in) OpSize.Whole = OpSizeLookup[(in) & 0x03]

enum StringBits
{
   Translation = 15,
   Backwards   = 16,
   UntilMatch  = 17,
   WhileMatch  = 18
};

extern void n32016_init();
extern void n32016_reset(uint32_t StartAddress);
extern void n32016_exec(uint32_t tubecycles);
extern void n32016_build_matrix();
extern void BreakPoint(uint32_t pc, uint32_t opcode);

extern ProcessorRegisters PR;
extern uint32_t r[8];
extern uint32_t pc;
extern uint16_t Regs[2];

#ifdef SHOW_INSTRUCTIONS
extern void ShowInstruction(uint32_t pc, uint32_t opcode, uint32_t Function, uint32_t OperandSize, uint32_t Disp);
#else
#define ShowInstruction(...)
#endif

#if 1
extern void ShowRegisterWrite(uint32_t Index, uint32_t Value);
#else
#define ShowRegisterWrite(...)
#endif

#ifdef PC_SIMULATION
extern void CloseTrace(void);
#endif

#ifdef TRACE_TO_FILE
#define PiTRACE(...) fprintf(pTraceFile, __VA_ARGS__)
extern FILE *pTraceFile;
#elif defined(TRACE_TO_CONSOLE)
#define PiTRACE printf
#else
#define PiTRACE(...)
#endif

extern uint32_t tube_irq;
uint8_t FunctionLookup[256];
extern uint32_t genaddr[2];
extern int gentype[2];
extern const uint8_t FormatSizes[FormatCount + 1];

//#define SHOW_STACK
#ifdef SHOW_STACK
#define PrintSP(str) PiTRACE("(%u) %s %06"PRIX32"\n", __LINE__, (str), GET_SP())
#else
#define PrintSP(str)
#endif

#define READ_PC_BYTE() read_x8(pc++) 

