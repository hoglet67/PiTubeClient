extern void n32016_init();
extern void n32016_reset();
extern void n32016_exec(uint32_t tubecycles);
extern void n32016_close();

#define MEG16 0x1000000
extern uint8_t ns32016ram[MEG16];
