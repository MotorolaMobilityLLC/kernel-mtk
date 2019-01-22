
#ifndef	__AFINIT__
#define __AFINIT__

extern void AfInit(unsigned char hall_bias, unsigned char hall_off);

extern void ServoOn(void);

extern unsigned int g_LC898212_SearchDir;

/*====================================================================
	Interface functions (import)
=====================================================================*/
extern void RamWriteA(unsigned short addr, unsigned short data);

extern void RamReadA(unsigned short addr, unsigned short *data);

extern void RegWriteA(unsigned short addr, unsigned char data);

extern void RegReadA(unsigned short addr, unsigned char *data);

extern void WaitTime(unsigned short msec);

#endif				/* __AFINIT__ */
