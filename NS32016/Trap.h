
enum TrapTypes
{
   BreakPointHit = BIT(0),
   ReservedAddressingMode = BIT(1),
   UnknownFormat = BIT(2),
   UnknownInstruction = BIT(3),
   DivideByZero = BIT(4),
   IllegalImmediate = BIT(5),
   IllegalDoubleIndexing = BIT(6),
   IllegalSpecialReading = BIT(7),
   IllegalSpecialWriting = BIT(8),
   IllegalWritingImmediate = BIT(9),
};

#define TrapCount 10
extern uint32_t TrapFlags;
#define CLEAR_TRAP() TrapFlags = 0
#define SET_TRAP(in) TrapFlags |= (in)
