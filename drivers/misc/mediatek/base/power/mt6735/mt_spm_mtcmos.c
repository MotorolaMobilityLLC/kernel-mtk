#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>	/* udelay */
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* **** */
/* #include <mach/mt_typedefs.h> */
/* #include <mach/mt_spm_cpu.h> */
#include <mach/mt_spm_mtcmos.h>
#include <mach/mt_spm_mtcmos_internal.h>
/* #include <mach/hotplug.h> */
#include <mach/mt_clkmgr.h>
/* #include <mach/mt_dcm.h> */
/* #include <mach/mt_boot.h> //mt_get_chip_sw_ver */

/* **** */
#if 0

#endif
/**************************************
 * for non-CPU MTCMOS
 **************************************/
/* **** */
#if 0
static DEFINE_SPINLOCK(spm_noncpu_lock);

#if 0
void spm_mtcmos_noncpu_lock(unsigned long *flags)
{
	spin_lock_irqsave(&spm_noncpu_lock, *flags);
}

void spm_mtcmos_noncpu_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&spm_noncpu_lock, *flags);
}
#else

#define spm_mtcmos_noncpu_lock(flags) spin_lock_irqsave(&spm_noncpu_lock, flags)
#define spm_mtcmos_noncpu_unlock(flags) spin_unlock_irqrestore(&spm_noncpu_lock, flags)

#endif
#endif				/* end **** */


#define MD2_PWR_STA_MASK    (0x1 << 22)
#define VEN_PWR_STA_MASK    (0x1 << 8)
#define VDE_PWR_STA_MASK    (0x1 << 7)
/* #define IFR_PWR_STA_MASK    (0x1 << 6) */
#define ISP_PWR_STA_MASK    (0x1 << 5)
#define MFG_PWR_STA_MASK    (0x1 << 4)
#define DIS_PWR_STA_MASK    (0x1 << 3)
/* #define DPY_PWR_STA_MASK    (0x1 << 2) */
#define CONN_PWR_STA_MASK   (0x1 << 1)
#define MD1_PWR_STA_MASK    (0x1 << 0)

#if 0
#define PWR_RST_B           (0x1 << 0)
#define PWR_ISO             (0x1 << 1)
#define PWR_ON              (0x1 << 2)
#define PWR_ON_2ND          (0x1 << 3)
#define PWR_CLK_DIS         (0x1 << 4)
#endif

#define SRAM_PDN            (0xf << 8)	/* VDEC, VENC, ISP, DISP */
#define MFG_SRAM_PDN        (0xf << 8)
#define MD_SRAM_PDN         (0x1 << 8)	/* MD1, C2K */
#define CONN_SRAM_PDN       (0x1 << 8)

#define VDE_SRAM_ACK        (0x1 << 12)
#define VEN_SRAM_ACK        (0xf << 12)
#define ISP_SRAM_ACK        (0x3 << 12)
#define DIS_SRAM_ACK        (0x1 << 12)
/* #define MFG_SRAM_ACK        (0x3f << 16) */
/* #define MFG_SRAM_ACK        (0x1 << 16) */
#define MFG_SRAM_ACK        (0x1 << 12)

#define MD1_PROT_MASK        ((0x1<<24) | (0x1<<25) | (0x1<<26) | (0x1<<27) | (0x1<<28))	/* bit 24,25,26,27,28 */
#define MD2_PROT_MASK        ((0x1<<29) | (0x1<<30) | (0x1<<31))	/* bit 29, 30, 31 */
/* bit16 is GCE, Dram dummy read use cqdma, and it is in GCE. */
#define DISP_PROT_MASK       ((0x1<<1))	/* bit 1, 6, 16; if bit6 set, MMSYS PDN, access reg will hang, */
#define MFG_PROT_MASK        ((0x1<<14))	/* bit 14 */
#define CONN_PROT_MASK       ((0x1<<2) | (0x1<<8))	/* bit 2, 8 */

#if defined(CONFIG_ARCH_MT6735M)
/* #define MD_PWRON_BY_CPU */
#elif defined(CONFIG_ARCH_MT6753)
#define MD_PWRON_BY_CPU
#else
/* #define MD_PWRON_BY_CPU */
#endif

int spm_mtcmos_ctrl_vdec(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_cpu_init();

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_VDE_PWR_CON) & VDE_SRAM_ACK) != VDE_SRAM_ACK) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_VDE_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_VDE_PWR_CON, val);

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & VDE_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ON);
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & VDE_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_RST_B);

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_VDE_PWR_CON) & VDE_SRAM_ACK)) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_venc(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_VEN_PWR_CON) & VEN_SRAM_ACK) != VEN_SRAM_ACK) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_VEN_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_VEN_PWR_CON, val);

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & VEN_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & VEN_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ON);
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & VEN_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & VEN_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_RST_B);

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_VEN_PWR_CON) & VEN_SRAM_ACK)) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;

#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_isp(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_ISP_PWR_CON) & ISP_SRAM_ACK) != ISP_SRAM_ACK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_ISP_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_ISP_PWR_CON, val);

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & ISP_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ON);
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & ISP_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_RST_B);

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_ISP_PWR_CON) & ISP_SRAM_ACK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

#if 0
int spm_mtcmos_ctrl_disp(int state)
{
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | DISP_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) != DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | SRAM_PDN);
#if 0
		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK) != DIS_SRAM_ACK) {
			/* read hw status */
			/* until hw ack ok */
		}
#endif
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_DIS_PWR_CON);
		/* val = (val & ~PWR_RST_B) | PWR_CLK_DIS; */
		val = val | PWR_CLK_DIS;
		spm_write(SPM_DIS_PWR_CON, val);

		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~(PWR_ON | PWR_ON_2ND)); */

#if 0
		udelay(1);
		if (spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
			err = 1;
#else
		/* while ((spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK) */
		/* || (spm_read(SPM_PWR_STATUS_S) & DIS_PWR_STA_MASK)) { */
		/* } */
#endif
	} else {		/* STA_POWER_ON */
		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON); */
		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON_2ND); */
#if 0
		udelay(1);
#else
		/* while (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK) */
		/* || !(spm_read(SPM_PWR_STATUS_S) & DIS_PWR_STA_MASK)) { */
		/* } */
#endif
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_ISO);
		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_RST_B); */

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~SRAM_PDN);

#if 0
		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK)) {
			/* read hw status */
			/* until hw ack ok */
		}
#endif

#if 0
		udelay(1);
		if (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK))
			err = 1;

#endif
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~DISP_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
}

#else

int spm_mtcmos_ctrl_disp(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | DISP_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) != DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK) != DIS_SRAM_ACK) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_DIS_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_DIS_PWR_CON, val);

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_RST_B);

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK)) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~DISP_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}
#endif

int spm_mtcmos_ctrl_mfg(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MFG_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & MFG_PROT_MASK) != MFG_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | MFG_SRAM_PDN);

		while ((spm_read(SPM_MFG_PWR_CON) & MFG_SRAM_ACK) != MFG_SRAM_ACK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_MFG_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_MFG_PWR_CON, val);

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON);
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK) ||
		       !(spm_read(SPM_PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_RST_B);

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~MFG_SRAM_PDN);

		while ((spm_read(SPM_MFG_PWR_CON) & MFG_SRAM_ACK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MFG_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & MFG_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_mdsys1(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MD1_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & MD1_PROT_MASK) != MD1_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | MD_SRAM_PDN);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_ISO);

		/* enable LTE LS ISO */
		val = spm_read(C2K_SPM_CTRL);
		val |= 0x40;
		spm_write(C2K_SPM_CTRL, val);

		val = spm_read(SPM_MD_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_MD_PWR_CON, val);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & MD1_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

	} else {		/* STA_POWER_ON */

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_ON);
		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & MD1_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

#ifdef MD_PWRON_BY_CPU
		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~PWR_ISO);

		/* disable LTE LS ISO */
		val = spm_read(C2K_SPM_CTRL);
		val &= ~(0x40);
		spm_write(C2K_SPM_CTRL, val);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_RST_B);
#else
		pr_debug("MD power on by SPM\n");
		spm_write(SPM_PCM_PASR_DPD_3, 0xbeef);
		spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0x1);
		while (spm_read(SPM_PCM_PASR_DPD_3)) {
			count++;
			udelay(1);
			if (count > 1000) {
				pr_err("MD power on: SPM no response\n");
				pr_err("PCM_IM_PTR : 0x%x (%u)\n", spm_read(SPM_PCM_IM_PTR),
				       spm_read(SPM_PCM_IM_LEN));
				BUG();
			}
		}
		/* disable LTE LS ISO */
		val = spm_read(C2K_SPM_CTRL);
		val &= ~(0x40);
		spm_write(C2K_SPM_CTRL, val);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_RST_B);

#endif

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~MD_SRAM_PDN);

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MD1_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & MD1_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_mdsys2(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MD2_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & MD2_PROT_MASK) != MD2_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | MD_SRAM_PDN);

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_C2K_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_C2K_PWR_CON, val);

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & MD2_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

	} else {		/* STA_POWER_ON */

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_ON);
		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & MD2_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_RST_B);

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~MD_SRAM_PDN);

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MD2_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & MD2_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_connsys(int state)
{
/* **** */
#if 0
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | CONN_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & CONN_PROT_MASK) != CONN_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | CONN_SRAM_PDN);

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_CONN_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_CONN_PWR_CON, val);

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & CONN_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & CONN_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_ON);
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & CONN_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & CONN_PWR_STA_MASK))
			;

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_RST_B);

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~CONN_SRAM_PDN);

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~CONN_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & CONN_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_topaxi_prot(int bit, int en)
{
/* **** */
#if 0
	unsigned long flags;

	spm_mtcmos_noncpu_lock(flags);

	if (en == 1) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | (1 << bit));
		while ((spm_read(TOPAXI_PROT_STA1) & (1 << bit)) != (1 << bit)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~(1 << bit));
		while (spm_read(TOPAXI_PROT_STA1) & (1 << bit)) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return 0;
#else
	return 1;
#endif
}
