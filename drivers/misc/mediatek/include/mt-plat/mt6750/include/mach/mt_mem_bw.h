#ifndef __MT_MEM_BW_H__
#define __MT_MEM_BW_H__

#define DISABLE_FLIPPER_FUNC  0

typedef unsigned long long (*getmembw_func)(void);
extern void mt_getmembw_registerCB(getmembw_func pCB);

unsigned long long get_mem_bw(void);

#endif  /* !__MT_MEM_BW_H__ */
