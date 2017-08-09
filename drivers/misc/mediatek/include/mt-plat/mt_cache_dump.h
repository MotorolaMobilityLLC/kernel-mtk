#ifndef __MT_CACHE_DUMP_H__
#define __MT_CACHE_DUMP_H__

#ifdef CONFIG_MTK_CACHE_DUMP
extern int mt_icache_dump(void);
#else
static inline unsigned int mt_icache_dump(void)
{
	return 0;
}
#endif


#endif
