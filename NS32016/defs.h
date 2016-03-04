#define BYTE_SWAP

#ifdef WIN32
#define SWAP16 _byteswap_ushort
#define SWAP32 _byteswap_ulong
#else
#define SWAP16 __builtin_bswap16
#define SWAP32 __builtin_bswap32
#endif

typedef union
{
   uint32_t	u32;
   int32_t s32;
   
   struct
   {
      uint8_t DoNotUseByte1;
      uint8_t DoNotUseByte2;
      uint8_t DoNotUseByte3;
      uint8_t Value;
   } u8;

   struct
   {
      int8_t DoNotUseByte1;
      int8_t DoNotUseByte2;
      int8_t DoNotUseByte3;
      int8_t Value;
   } s8;

   struct
   {
      uint16_t DoNotUse;
      uint16_t Value;
   } u16;

   struct
   {
      int16_t DoNotUse;
      int16_t Value;
   } s16;
} MultiReg;
