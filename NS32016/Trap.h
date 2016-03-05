
enum TrapTypes
{
   BreakPointHit = BIT(0),
   BreakPointTrap = BIT(1),
   ReservedAddressingMode = BIT(2),
   UnknownFormat = BIT(3),
   UnknownInstruction = BIT(4),
   DivideByZero = BIT(5),
   IllegalImmediate = BIT(6),
   IllegalDoubleIndexing = BIT(7),
   IllegalSpecialReading = BIT(8),
   IllegalSpecialWriting = BIT(9),
   IllegalWritingImmediate = BIT(10),
};

#define TrapCount 10
extern uint32_t TrapFlags;
#define CLEAR_TRAP() TrapFlags = 0
#define SET_TRAP(in) TrapFlags |= (in)
