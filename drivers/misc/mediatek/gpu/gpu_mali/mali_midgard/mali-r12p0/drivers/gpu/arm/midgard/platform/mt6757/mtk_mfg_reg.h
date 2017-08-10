#ifndef __MTK_MFG_REG_H__
#define __MTK_MFG_REG_H__

/* MFG reg */
#define MFG_DEBUG_SEL   0x180
#define MFG_DEBUG_A 0x184
#define MFG_DEBUG_IDEL	(1 << 2)

extern volatile void *g_MFG_base;

#define base_write32(addr, value)	writel(value, addr)
#define base_read32(addr)			readl(addr)
#define MFG_write32(addr, value)	base_write32(g_MFG_base+addr, value)
#define MFG_read32(addr)			base_read32(g_MFG_base+addr)

#endif /* __MTK_MFG_REG_H__ */