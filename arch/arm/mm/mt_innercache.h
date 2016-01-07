#ifndef __MT_INNERCACHE_H
#define __MT_INNERCACHE_H

extern void __inner_flush_dcache_all(void);
extern void __inner_flush_dcache_L1(void);
extern void __inner_flush_dcache_L2(void);

#endif	/* !__INNERCACHE_H */
