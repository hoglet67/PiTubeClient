
enum TrapTypes
{
   BreakPointHit = BIT(0),

   ReservedAddressingMode = BIT(7),
   UnknownFormat = BIT(8),
   UnknownInstruction = BIT(9),
   DivideByZero = BIT(10),
   IllegalImmediate = BIT(11),
   IllegalDoubleIndexing = BIT(12),
   IllegalSpecialReading = BIT(13),
   IllegalSpecialWriting = BIT(14),
   IllegalWritingImmediate = BIT(15),
};

extern uint32_t TrapFlags;
#define SET_TRAP(in) TrapFlags |= (in)
