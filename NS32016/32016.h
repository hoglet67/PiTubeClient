extern void n32016_init();
extern void n32016_reset();
extern void n32016_exec(uint32_t tubecycles);
extern void n32016_close();

void ShowInstruction(uint32_t startpc);

#define MEG16 0x1000000
extern uint8_t ns32016ram[MEG16];

#define CASE2(in) case (in): case ((in) | 0x80)
#define CASE4(in) case (in): case ((in) | 0x40): case ((in) | 0x80): case ((in) | 0xC0)

