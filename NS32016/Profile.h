#ifdef PROFILING

extern void ProfileInit(void);
extern void ProfileAdd(uint32_t Function);
extern void ProfileDump(void);

#else

#define ProfileInit()
#define ProfileAdd(a)
#define ProfileDump()

#endif