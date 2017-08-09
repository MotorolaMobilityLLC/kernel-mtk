#ifndef __MT_MEMORY_H__
#define __MT_MEMORY_H__

/*
 * Define macros.
 */

/* IO_VIRT = 0xF0000000 | IO_PHYS[27:0] */
#define IO_VIRT_TO_PHYS(v) (0x10000000 | ((v) & 0x0fffffff))
#define IO_PHYS_TO_VIRT(p) (0xf0000000 | ((p) & 0x0fffffff))
#ifndef __ASSEMBLER__
static inline unsigned int enable_4G(void)
{
	return 0;
}
#endif
#endif  /* !__MT_MEMORY_H__ */
