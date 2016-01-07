#ifndef __CCIF_PLATFORM_H__
#define __CCIF_PLATFORM_H__
#include "ccci_core.h"
#include <mach/sync_write.h>

#define ccif_write32(b, a, v)           mt_reg_sync_writel(v, (b)+(a))
#define ccif_write16(b, a, v)           mt_reg_sync_writew(v, (b)+(a))
#define ccif_write8(b, a, v)            mt_reg_sync_writeb(v, (b)+(a))
#define ccif_read32(b, a)               ioread32((void __iomem *)((b)+(a)))
#define ccif_read16(b, a)               ioread16((void __iomem *)((b)+(a)))
#define ccif_read8(b, a)                ioread8((void __iomem *)((b)+(a)))

/*MD peripheral register: MD bank8; AP bank2*/
/*Modem WDT */
#define WDT_MD_MODE     (0x00)
#define WDT_MD_LENGTH   (0x04)
#define WDT_MD_RESTART  (0x08)
#define WDT_MD_STA      (0x0C)
#define WDT_MD_SWRST    (0x1C)
#define WDT_MD_MODE_KEY (0x0000220E)

/*CCIF */
#define APCCIF_CON    (0x00)
#define APCCIF_BUSY   (0x04)
#define APCCIF_START  (0x08)
#define APCCIF_TCHNUM (0x0C)
#define APCCIF_RCHNUM (0x10)
#define APCCIF_ACK    (0x14)
#define APCCIF_CHDATA (0x100)

/*C2K */
#define INFRA_AO_C2K_CONFIG (0x330)
#define INFRA_AO_C2K_STATUS (0x334)
#define INFRA_AO_C2K_SPM_CTRL (0x338)
#define SLEEP_CLK_CON (0x400)
#define TOP_RGU_WDT_MODE (0x0)
#define TOP_RGU_WDT_SWRST (0x14)
#define TOP_RGU_WDT_SWSYSRST (0x18)
#define TOP_RGU_WDT_NONRST_REG (0x20)

#define ETS_SEL_BIT					(0x1 << 13)

struct md_hw_info {
	/*HW info - Register Address */
	unsigned long md_rgu_base;
	unsigned long md_boot_slave_Vector;
	unsigned long md_boot_slave_Key;
	unsigned long md_boot_slave_En;
	unsigned long ap_ccif_base;
	unsigned long md_ccif_base;
	unsigned int sram_size;
	/* #ifdef CONFIG_MTK_ECCCI_C2K */
	unsigned long sleep_base;
	unsigned long infra_ao_base;
	unsigned long toprgu_base;
	unsigned long c2k_chip_id_base;
	unsigned long md1_pccif_base;
	unsigned long md3_pccif_base;
	/* #endif */

	/*HW info - Interrutpt ID */
	unsigned int ap_ccif_irq_id;
	unsigned int md_wdt_irq_id;

	/*HW info - Interrupt flags */
	unsigned long ap_ccif_irq_flags;
	unsigned long md_wdt_irq_flags;
};

extern unsigned long ccci_modem_boot_count[];

extern int md_ccif_power_off(struct ccci_modem *md, unsigned int timeout);
extern int md_ccif_power_on(struct ccci_modem *md);
extern int md_ccif_let_md_go(struct ccci_modem *md);

int md_ccif_get_modem_hw_info(struct platform_device *dev_ptr,
			      struct ccci_dev_cfg *dev_cfg,
			      struct md_hw_info *hw_info);
int md_ccif_io_remap_md_side_register(struct ccci_modem *md);
void reset_md1_md3_pccif(struct ccci_modem *md);

extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
extern void mtk_wdt_set_c2k_sysrst(unsigned int flag);
extern void ccci_mem_dump(int md_id, void *start_addr, int len);

#endif /*__CLDMA_PLATFORM_H__*/
