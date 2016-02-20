extern void n32016_init();
extern void n32016_reset();
extern void n32016_exec(uint32_t tubecycles);

void ShowInstruction(uint32_t startpc);

#define K640  0x00A0000
#define MEG1  0x0100000
#define MEG4  0x0400000
#define MEG16 0x1000000

extern uint8_t ns32016ram[MEG16];
extern uint32_t tube_irq;

#define CASE2(in) case (in): case ((in) | 0x80)
#define CASE4(in) case (in): case ((in) | 0x40): case ((in) | 0x80): case ((in) | 0xC0)

