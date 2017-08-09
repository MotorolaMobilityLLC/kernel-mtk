#ifndef __MTK_MEMCFG_H__
#define __MTK_MEMCFG_H__
#include <linux/fs.h>

/* late warning flags */
#define WARN_MEMBLOCK_CONFLICT	(1 << 0)	/* memblock overlap */
#define WARN_MEMSIZE_CONFLICT	(1 << 1)	/* dram info missing */
#define WARN_API_NOT_INIT	(1 << 2)	/* API is not initialized */

#ifdef CONFIG_MTK_MEMCFG

#define MTK_MEMCFG_LOG_AND_PRINTK(fmt, arg...)  \
	do {    \
		printk(fmt, ##arg); \
		mtk_memcfg_write_memory_layout_buf(fmt, ##arg); \
	} while (0)

extern void mtk_memcfg_write_memory_layout_buf(char *, ...);
extern void mtk_memcfg_late_warning(unsigned long);

#ifdef CONFIG_SLUB_DEBUG
extern int slabtrace_open(struct inode *inode, struct file *file);
#endif /* end of CONFIG_SLUB_DEBUG */

#if defined(CONFIG_MTK_FB)
extern unsigned int DISP_GetVRamSizeBoot(char *cmdline);
#endif	/* end of CONFIG_MTK_FB */

#ifdef CONFIG_OF
extern phys_addr_t mtkfb_get_fb_base(void);
#endif	/* end of CONFIG_OF */

#ifdef CONFIG_HIGHMEM
extern unsigned long totalhigh_pages;
#endif /* end of CONFIG_HIGHMEM */

extern void split_page(struct page *page, unsigned int order);

#else

#define MTK_MEMCFG_LOG_AND_PRINTK(fmt, arg...) printk(fmt, ##arg)

#define mtk_memcfg_get_force_inode_gfp_lowmem()  do { } while (0)
#define mtk_memcfg_set_force_inode_gfp_lowmem(flag)  do { } while (0)
#define mtk_memcfg_get_bypass_slub_debug_flag()  do { } while (0)
#define mtk_memcfg_set_bypass_slub_debug_flag(flag)  do { } while (0)
#define mtk_memcfg_write_memory_layout_buf(fmt, arg...) do { } while (0)
#define mtk_memcfg_late_warning(flag) do { } while (0)

#endif /* end CONFIG_MTK_MEMCFG */

#endif /* end __MTK_MEMCFG_H__ */
