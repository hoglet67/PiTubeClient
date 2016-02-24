#define IO_BASE 0xFFFFF0
#define PANDORA_ROM_PAGE_OUT
#define LITTLE_ENDIAN

extern uint8_t		read_x8(uint32_t addr);
extern uint16_t	read_x16(uint32_t addr);
extern uint32_t	read_x32(uint32_t addr);

extern void write_x8(uint32_t addr, uint8_t val);
extern void write_x16(uint32_t addr, uint16_t val);
extern void write_x32(uint32_t addr, uint32_t val);
extern void write_Arbitary(uint32_t addr, uint8_t* pData, uint32_t Size);
