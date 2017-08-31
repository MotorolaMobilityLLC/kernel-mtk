/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/smp.h>
/* #include <linux/earlysuspend.h> */
#include <linux/io.h>

/* **** */
/* #include <mach/mt_typedefs.h> */
#include <mt-plat/sync_write.h>
#include "mtk_clkmgr.h"
/* #include <mach/mt_dcm.h> */
/* #include <mach/mt_idvfs.h> */ /* Fix when idvfs ready */
#include "mtk_spm.h"
/* #include <mach/mtk_spm_mtcmos.h> */
/* #include <mach/mt_spm_sleep.h> */
/* #include <mach/mt_gpufreq.h> */
/* #include <mach/irqs.h> */

/* #include <mach/upmu_common.h> */
/* #include <mach/upmu_sw.h> */
/* #include <mach/upmu_hw.h> */
/* #include "mt_freqhopping_drv.h" */

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#ifdef CONFIG_OF
void __iomem *clk_apmixed_base;
void __iomem *clk_cksys_base;
void __iomem *clk_infracfg_ao_base;
void __iomem *clk_audio_base;
void __iomem *clk_mfgcfg_base;
void __iomem *clk_mmsys_config_base;
void __iomem *clk_imgsys_base;
void __iomem *clk_vdec_gcon_base;
void __iomem *clk_mjc_config_base;
void __iomem *clk_venc_gcon_base;
void __iomem *clk_camsys_base;
/* void __iomem *clk_topmics_base;*/
#endif

#define Bring_Up

#define dbg_bug_size 4096
static char dbg_buf[dbg_bug_size] = { 0 };
/************************************************
 **********         log debug          **********
 ************************************************/
#define TAG     "[Power/clkmgr] "

#define clk_err(fmt, args...)       \
	pr_err(TAG fmt, ##args)
#define clk_warn(fmt, args...)      \
	pr_warn(TAG fmt, ##args)
#define clk_info(fmt, args...)      \
	pr_info(TAG fmt, ##args)
#define clk_dbg(fmt, args...)       \
	pr_debug(TAG fmt, ##args)

/************************************************
 **********      register access       **********
 ************************************************/

#define clk_readl(addr) \
	readl(addr)
    /* DRV_Reg32(addr) */

#define clk_writel(addr, val)   \
	mt_reg_sync_writel(val, addr)

#define clk_setl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) | (val), addr)

#define clk_clrl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

/************************************************
 **********      struct definition     **********
 ************************************************/
struct pll {
	const char *name;
	int type;
	int mode;
	int feat;
	int state;
	unsigned int cnt;
	unsigned int en_mask;
	void __iomem *base_addr;
	void __iomem *pwr_addr;
	unsigned int hp_id;
	int hp_switch;
};

struct subsys {
	const char *name;
	int type;
	int force_on;
	unsigned int cnt;
	unsigned int state;
	unsigned int default_sta;
	unsigned int sta_mask;	/* mask in PWR_STATUS */
	void __iomem *ctl_addr;
};

/************************************************
 **********      global variablies     **********
 ************************************************/

#define PWR_DOWN    0
#define PWR_ON      1

static int initialized;
static int slp_chk_mtcmos_pll_stat;

static struct pll plls[NR_PLLS];
static struct subsys syss[NR_SYSS];

/************************************************
 **********      spin lock protect     **********
 ************************************************/

static DEFINE_SPINLOCK(clock_lock);

#define clkmgr_lock(flags)  spin_lock_irqsave(&clock_lock, flags)

#define clkmgr_unlock(flags)  spin_unlock_irqrestore(&clock_lock, flags)

#define clkmgr_locked()  spin_is_locked(&clock_lock)

int clkmgr_is_locked(void)
{
	return clkmgr_locked();
}
EXPORT_SYMBOL(clkmgr_is_locked);

/************************************************
 **********          pll part          **********
 ************************************************/

#define PLL_TYPE_SDM    0
#define PLL_TYPE_LC     1

#define HAVE_RST_BAR    (0x1 << 0)
#define HAVE_PLL_HP     (0x1 << 1)
#define HAVE_FIX_FRQ    (0x1 << 2)
#define Others          (0x1 << 3)

#define RST_BAR_MASK    0x1000000

static struct pll plls[NR_PLLS] = {
	{
	.name = __stringify(MAINPLL),
	.en_mask = 0xF0000101,
	}, {
	.name = __stringify(UNIVPLL),
	.en_mask = 0xFE000001,
	}, {
	.name = __stringify(MSDCPLL),
	.en_mask = 0x00000101,
	}, {
	.name = __stringify(VENCPLL),
	.en_mask = 0x00000101,
	}, {
	.name = __stringify(MMPLL),
	.en_mask = 0x00000101,
	}, {
	.name = __stringify(TVDPLL),
	.en_mask = 0xc0000101,
	}, {
	.name = __stringify(APLL1),
	.en_mask = 0x00000101,
	}, {
	.name = __stringify(APLL2),
	.en_mask = 0x00000101,
	}
};

static struct pll *id_to_pll(unsigned int id)
{
	return id < NR_PLLS ? plls + id : NULL;
}

#define PLL_PWR_ON  (0x1 << 0)
#define PLL_ISO_EN  (0x1 << 1)

#define SDM_PLL_N_INFO_MASK 0x001FFFFF
#define UNIV_SDM_PLL_N_INFO_MASK 0x001fc000
#define SDM_PLL_N_INFO_CHG  0x80000000
#define ARMPLL_POSDIV_MASK  0x07000000

static int pll_get_state_op(struct pll *pll)
{
	return clk_readl(pll->base_addr) & 0x1;
}
int pll_is_on(int id)
{
	int state;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

	WARN_ON(!pll);

	clkmgr_lock(flags);
	state = pll_get_state_op(pll);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(pll_is_on);
#if 0
void enable_armpll_ll(void)
{
	clk_writel(ARMPLL_LL_PWR_CON0, (clk_readl(ARMPLL_LL_PWR_CON0) | 0x01));
	udelay(1);
	clk_writel(ARMPLL_LL_PWR_CON0,
		(clk_readl(ARMPLL_LL_PWR_CON0) & 0xfffffffd));
	clk_writel(ARMPLL_LL_CON0, (clk_readl(ARMPLL_LL_CON0) | 0x01));
	udelay(200);
}
EXPORT_SYMBOL(enable_armpll_ll);
void disable_armpll_ll(void)
{
	clk_writel(ARMPLL_LL_CON0, (clk_readl(ARMPLL_LL_CON0) & 0xfffffffe));
	clk_writel(ARMPLL_LL_PWR_CON0,
		(clk_readl(ARMPLL_LL_PWR_CON0) | 0x00000002));
	clk_writel(ARMPLL_LL_PWR_CON0,
		(clk_readl(ARMPLL_LL_PWR_CON0) & 0xfffffffe));
}
EXPORT_SYMBOL(disable_armpll_ll);

void enable_armpll_l(void)
{
	clk_writel(ARMPLL_L_PWR_CON0, (clk_readl(ARMPLL_L_PWR_CON0) | 0x01));
	udelay(1);
	clk_writel(ARMPLL_L_PWR_CON0,
		(clk_readl(ARMPLL_L_PWR_CON0) & 0xfffffffd));
	clk_writel(ARMPLL_L_CON0, (clk_readl(ARMPLL_L_CON0) | 0x01));
	udelay(200);
}
EXPORT_SYMBOL(enable_armpll_l);
void disable_armpll_l(void)
{
	clk_writel(ARMPLL_L_CON0, (clk_readl(ARMPLL_L_CON0) & 0xfffffffe));
	clk_writel(ARMPLL_L_PWR_CON0,
		(clk_readl(ARMPLL_L_PWR_CON0) | 0x00000002));
	clk_writel(ARMPLL_L_PWR_CON0,
		(clk_readl(ARMPLL_L_PWR_CON0) & 0xfffffffe));
}
EXPORT_SYMBOL(disable_armpll_l);
void switch_armpll_l_hwmode(int enable)
{
	/* ARM CA15*/
	if (enable == 1) {
		clk_writel(AP_PLL_CON3, (clk_readl(AP_PLL_CON3) & 0xFFFEEEEF));
		clk_writel(AP_PLL_CON4, (clk_readl(AP_PLL_CON4) & 0xFFFFFEFE));

	} else{
		clk_writel(AP_PLL_CON3, (clk_readl(AP_PLL_CON3) | 0x00011110));
		clk_writel(AP_PLL_CON4, (clk_readl(AP_PLL_CON4) | 0x00000101));
	}
}
EXPORT_SYMBOL(switch_armpll_l_hwmode);

void switch_armpll_ll_hwmode(int enable)
{
	/* ARM CA7*/
	if (enable == 1) {
		clk_writel(AP_PLL_CON3, (clk_readl(AP_PLL_CON3) & 0xFF87FFFF));
		clk_writel(AP_PLL_CON4, (clk_readl(AP_PLL_CON4) & 0xFFFFCFFF));
	} else {
		clk_writel(AP_PLL_CON3, (clk_readl(AP_PLL_CON3) | 0x00780000));
		clk_writel(AP_PLL_CON4, (clk_readl(AP_PLL_CON4) | 0x00003000));
	}
}
EXPORT_SYMBOL(switch_armpll_ll_hwmode);
#endif
/************************************************
 **********         subsys part        **********
 ************************************************/

#define SYS_TYPE_MODEM    0
#define SYS_TYPE_MEDIA    1
#define SYS_TYPE_OTHER    2
#define SYS_TYPE_CONN     3

static struct subsys syss[NR_SYSS] = {
	{
	.name = __stringify(SYS_MD1),
	.sta_mask = 1U << 0,
	}, {
	.name = __stringify(SYS_CONN),
	.sta_mask = 1U << 1,
	}, {
	.name = __stringify(SYS_DIS),
	.sta_mask = 1U << 3,
	}, {
	.name = __stringify(SYS_MFG),
	.sta_mask = 1U << 4,
	}, {
	.name = __stringify(SYS_ISP),
	.sta_mask = 1U << 5,
	}, {
	.name = __stringify(SYS_VDE),
	.sta_mask = 1U << 7,
	}, {
	.name = __stringify(SYS_VEN),
	.sta_mask = 1U << 21,
	}, {
	.name = __stringify(SYS_MFG_ASYNC),
	.sta_mask = 1U << 23,
	}, {
	.name = __stringify(SYS_AUDIO),
	.sta_mask = 1U << 24,
	}, {
	.name = __stringify(SYS_CAM),
	.sta_mask = 1U << 27,
	}, {
	.name = __stringify(SYS_C2K),
	.sta_mask = 1U << 28,
	}, {
	.name = __stringify(SYS_MDSYS_INTF_INFRA),
	.sta_mask = 1U << 29,
	}, {
	.name = __stringify(SYS_MFG_CORE1),
	.sta_mask = 1U << 30,
	}, {
	.name = __stringify(SYS_MFG_CORE0),
	.sta_mask = 1U << 31,
	}
};

static struct subsys *id_to_sys(unsigned int id)
{
	return id < NR_SYSS ? syss + id : NULL;
}

static int sys_get_state_op(struct subsys *sys)
{
	unsigned int sta = clk_readl(PWR_STATUS);
	unsigned int sta_s = clk_readl(PWR_STATUS_2ND);

	return (sta & sys->sta_mask) && (sta_s & sys->sta_mask);
}

int subsys_is_on(int id)
{
	int state;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

	WARN_ON(!sys);

	clkmgr_lock(flags);
	state = sys_get_state_op(sys);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(subsys_is_on);
/************************************************
 **********         cg_clk part        **********
 ************************************************/

/************************************************
 **********       initialization       **********
 ************************************************/
static void mt_subsys_init(void)
{
	syss[SYS_MD1].ctl_addr = MD1_PWR_CON;
	syss[SYS_CONN].ctl_addr = CONN_PWR_CON;
	syss[SYS_DIS].ctl_addr = DIS_PWR_CON;
	syss[SYS_MFG].ctl_addr = MFG_PWR_CON;
	syss[SYS_ISP].ctl_addr = ISP_PWR_CON;
	syss[SYS_VDE].ctl_addr = VDE_PWR_CON;
	syss[SYS_VEN].ctl_addr = VEN_PWR_CON;
	syss[SYS_MFG_ASYNC].ctl_addr = MFG_ASYNC_PWR_CON;
	syss[SYS_AUDIO].ctl_addr = AUDIO_PWR_CON;
	syss[SYS_CAM].ctl_addr = CAM_PWR_CON;
	syss[SYS_C2K].ctl_addr = C2K_PWR_CON;
	syss[SYS_MDSYS_INTF_INFRA].ctl_addr = MDSYS_INTF_INFRA_PWR_CON;
	syss[SYS_MFG_CORE1].ctl_addr = MFG_CORE1_PWR_CON;
	syss[SYS_MFG_CORE0].ctl_addr = MFG_CORE0_PWR_CON;
}

static void mt_plls_init(void)
{
	plls[MAINPLL].base_addr = MAINPLL_CON0;
	plls[MAINPLL].pwr_addr = MAINPLL_PWR_CON0;
	plls[UNIVPLL].base_addr = UNIVPLL_CON0;
	plls[UNIVPLL].pwr_addr = UNIVPLL_PWR_CON0;
	plls[MSDCPLL].base_addr = MSDCPLL_CON0;
	plls[MSDCPLL].pwr_addr = MSDCPLL_PWR_CON0;
	plls[VENCPLL].base_addr = VENCPLL_CON0;
	plls[VENCPLL].pwr_addr = VENCPLL_PWR_CON0;
	plls[MMPLL].base_addr = MMPLL_CON0;
	plls[MMPLL].pwr_addr = MMPLL_PWR_CON0;
	plls[TVDPLL].base_addr = TVDPLL_CON0;
	plls[TVDPLL].pwr_addr = TVDPLL_PWR_CON0;
	plls[APLL1].base_addr = APLL1_CON0;
	plls[APLL1].pwr_addr = APLL1_PWR_CON0;
	plls[APLL2].base_addr = APLL2_CON0;
	plls[APLL2].pwr_addr = APLL2_PWR_CON0;
}

void clk_dump(void)
{
	pr_err("********** PLL register dump *********\n");

	pr_err("[MAINPLL_CON0]=0x%08x\n", clk_readl(MAINPLL_CON0));
	pr_err("[MAINPLL_CON1]=0x%08x\n", clk_readl(MAINPLL_CON1));
	pr_err("[MAINPLL_PWR_CON0]=0x%08x\n", clk_readl(MAINPLL_PWR_CON0));

	pr_err("[UNIVPLL_CON0]=0x%08x\n", clk_readl(UNIVPLL_CON0));
	pr_err("[UNIVPLL_CON1]=0x%08x\n", clk_readl(UNIVPLL_CON1));
	pr_err("[UNIVPLL_PWR_CON0]=0x%08x\n", clk_readl(UNIVPLL_PWR_CON0));

	pr_err("[MSDCPLL_CON0]=0x%08x\n", clk_readl(MSDCPLL_CON0));
	pr_err("[MSDCPLL_CON1]=0x%08x\n", clk_readl(MSDCPLL_CON1));
	pr_err("[MSDCPLL_PWR_CON0]=0x%08x\n", clk_readl(MSDCPLL_PWR_CON0));

	pr_err("[VENCPLL_CON0]=0x%08x\n", clk_readl(VENCPLL_CON0));
	pr_err("[VENCPLL_CON1]=0x%08x\n", clk_readl(VENCPLL_CON1));
	pr_err("[VENCPLL_PWR_CON0]=0x%08x\n", clk_readl(VENCPLL_PWR_CON0));

	pr_err("[MMPLL_CON0]=0x%08x\n", clk_readl(MMPLL_CON0));
	pr_err("[MMPLL_CON1]=0x%08x\n", clk_readl(MMPLL_CON1));
	pr_err("[MMPLL_PWR_CON0]=0x%08x\n", clk_readl(MMPLL_PWR_CON0));

	pr_err("[TVDPLL_CON0]=0x%08x\n", clk_readl(TVDPLL_CON0));
	pr_err("[TVDPLL_CON1]=0x%08x\n", clk_readl(TVDPLL_CON1));
	pr_err("[TVDPLL_PWR_CON0]=0x%08x\n", clk_readl(TVDPLL_PWR_CON0));

	pr_err("[APLL1_CON0]=0x%08x\n", clk_readl(APLL1_CON0));
	pr_err("[APLL1_CON1]=0x%08x\n", clk_readl(APLL1_CON1));
	pr_err("[APLL1_PWR_CON0]=0x%08x\n", clk_readl(APLL1_PWR_CON0));

	pr_err("[APLL2_CON0]=0x%08x\n", clk_readl(APLL2_CON0));
	pr_err("[APLL2_CON1]=0x%08x\n", clk_readl(APLL2_CON1));
	pr_err("[APLL2_PWR_CON0]=0x%08x\n", clk_readl(APLL2_PWR_CON0));

	pr_err("[AP_PLL_CON3]=0x%08x\n", clk_readl(AP_PLL_CON3));
	pr_err("[AP_PLL_CON4]=0x%08x\n", clk_readl(AP_PLL_CON4));

	pr_err("********** subsys pwr sts register dump *********\n");
	pr_err("PWR_STATUS=0x%08x, PWR_STATUS_2ND=0x%08x\n",
	       clk_readl(PWR_STATUS), clk_readl(PWR_STATUS_2ND));

	pr_err("********** mux register dump *********\n");

	pr_err("[CLK_CFG_0]=0x%08x\n", clk_readl(CLK_CFG_0));
	pr_err("[CLK_CFG_1]=0x%08x\n", clk_readl(CLK_CFG_1));
	pr_err("[CLK_CFG_2]=0x%08x\n", clk_readl(CLK_CFG_2));
	pr_err("[CLK_CFG_3]=0x%08x\n", clk_readl(CLK_CFG_3));
	pr_err("[CLK_CFG_4]=0x%08x\n", clk_readl(CLK_CFG_4));
	pr_err("[CLK_CFG_5]=0x%08x\n", clk_readl(CLK_CFG_5));
	pr_err("[CLK_CFG_6]=0x%08x\n", clk_readl(CLK_CFG_6));
	pr_err("[CLK_CFG_7]=0x%08x\n", clk_readl(CLK_CFG_7));
	pr_err("[CLK_CFG_8]=0x%08x\n", clk_readl(CLK_CFG_8));
	pr_err("[CLK_CFG_9]=0x%08x\n", clk_readl(CLK_CFG_9));

	pr_err("********** CG register dump *********\n");
	pr_err("[INFRA_SUBSYS][%p]\n", clk_infracfg_ao_base);	/* 0x10001000 */

	pr_err("[INFRA_PDN_STA0]=0x%08x\n", clk_readl(INFRA_SW_CG0_STA));
	pr_err("[INFRA_PDN_STA1]=0x%08x\n", clk_readl(INFRA_SW_CG1_STA));
	pr_err("[INFRA_PDN_STA2]=0x%08x\n", clk_readl(INFRA_SW_CG2_STA));

	pr_err("[INFRA_TOPAXI_PROTECTSTA1]=0x%08x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA1));

	pr_err("[MMSYS_SUBSYS][%p]\n", clk_mmsys_config_base);
	pr_err("[DISP_CG_CON0]=0x%08x\n", clk_readl(DISP_CG_CON0));
	pr_err("[DISP_CG_CON1]=0x%08x\n", clk_readl(DISP_CG_CON1));
	pr_err("[DISP_CG_CON0_DUMMY]=0x%08x\n", clk_readl(DISP_CG_CON0_DUMMY));
	pr_err("[DISP_CG_CON1_DUMMY]=0x%08x\n", clk_readl(DISP_CG_CON1_DUMMY));

	/**/
	pr_err("[MFG_SUBSYS][%p]\n", clk_mfgcfg_base);
	pr_err("[MFG_CG_CON]=0x%08x\n", clk_readl(MFG_CG_CON));

	pr_err("[AUDIO_SUBSYS][%p]\n", clk_audio_base);
	pr_err("[AUDIO_TOP_CON0]=0x%08x\n", clk_readl(AUDIO_TOP_CON0));

	pr_err("[IMG_SUBSYS][%p]\n", clk_imgsys_base);
	pr_err("[IMG_CG_CON]=0x%08x\n", clk_readl(IMG_CG_CON));

	pr_err("[VDEC_SUBSYS][%p]\n", clk_vdec_gcon_base);
	pr_err("[VDEC_CKEN_SET]=0x%08x\n", clk_readl(VDEC_CKEN_SET));
	pr_err("[LARB_CKEN_SET]=0x%08x\n", clk_readl(LARB_CKEN_SET));

	pr_err("[VENC_SUBSYS][%p]\n", clk_venc_gcon_base);
	pr_err("[VENC_CG_CON]=0x%08x\n", clk_readl(VENC_CG_CON));

	pr_err("[CAM_SUBSYS][%p]\n", clk_camsys_base);
	pr_err("[CAM_CG_CON]=0x%08x\n", clk_readl(CAM_CG_CON));

}

void clk_misc_cfg_ops(bool flag)
{
	pr_err("[%s]: warning! warning! warning! to use this function\n", __func__);
}
EXPORT_SYMBOL(clk_misc_cfg_ops);
/************************************************
 **********       function debug       **********
 ************************************************/
/*extern void mtcmos_force_off(void);*/
#define PLL_PWR_CON0_ISO_EN_MASK 0xfffffffc
#define PLL_PWR_CON0_ISO_EN_VALUE 0x2
#define PLL_CON0_VALUE 0

void __attribute__((weak)) mtcmos_force_off(void)
{
	pr_err("[%s]: warning!! real function doesn't exist\n", __func__);
}

static void pll_force_off(void)
{
	unsigned int temp;

	temp = clk_readl(ARMPLL_L_PWR_CON0);
	clk_writel(ARMPLL_L_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(CCIPLL_PWR_CON0);
	clk_writel(CCIPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(UNIVPLL_PWR_CON0);
	clk_writel(UNIVPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(MSDCPLL_PWR_CON0);
	clk_writel(MSDCPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(VENCPLL_PWR_CON0);
	clk_writel(VENCPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(MMPLL_PWR_CON0);
	clk_writel(MMPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(TVDPLL_PWR_CON0);
	clk_writel(TVDPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(APLL1_PWR_CON0);
	clk_writel(APLL1_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(APLL2_PWR_CON0);
	clk_writel(APLL2_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);


    /***********************
      * xPLL Frequency Enable
      ************************/
	temp = clk_readl(ARMPLL_L_CON0);
	clk_writel(ARMPLL_L_CON0, PLL_CON0_VALUE);

	temp = clk_readl(CCIPLL_CON0);
	clk_writel(CCIPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(UNIVPLL_CON0);
	clk_writel(UNIVPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(MSDCPLL_CON0);
	clk_writel(MSDCPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(VENCPLL_CON0);
	clk_writel(VENCPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(MMPLL_CON0);
	clk_writel(MMPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(TVDPLL_CON0);
	clk_writel(TVDPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(APLL1_CON0);
	clk_writel(APLL1_CON0, PLL_CON0_VALUE);

	temp = clk_readl(APLL2_CON0);
	clk_writel(APLL2_CON0, PLL_CON0_VALUE);
}

#if 0
static void cg_force_off(void)
{
	/*infra*/
	clk_writel(INFRA_SW_CG0_SET, 0xFBEFFFFF); /*bit 20,26,: no use*/
	clk_writel(INFRA_SW_CG1_SET, 0xFFFF3FFF); /*bit 14, 15: no use , bit 11 is auxadc*/
	clk_writel(INFRA_SW_CG2_SET, 0xFFFFFDFF); /*bit 9: no use, bit 14 dvfs_spm0*/

	/*vdec*/
	clk_writel(VDEC_CKEN_CLR, 0x00000111);
	clk_writel(LARB_CKEN_CLR, 0x00000001);

	/*img*/
	clk_writel(IMG_CG_SET, 0x00000C41);

	/*cam*/
	clk_writel(CAM_CG_SET, 0x00000FC1);

	/*venc*/
	clk_writel(VENC_CG_SET, 0x00001111);

	/*mjc*/
	clk_writel(MJC_CG_SET, 0x0000003F);

	/*mfg*/
	clk_writel(MFG_CG_SET, 0x00000001);

	/*audio*/
	clk_writel(AUDIO_TOP_CON0, 0xFFFFFFFF);
	clk_writel(AUDIO_TOP_CON1, 0xFFFFFFFF);
}
#endif

void all_force_off(void)
{
	/*CG*/
	/*cg_force_off();*/

	/*mtcmos*/
	mtcmos_force_off();

	/*pll*/
	pll_force_off();
}

static int armlpll_fsel_read(struct seq_file *m, void *v)
{
	return 0;
}

static int armllpll_fsel_read(struct seq_file *m, void *v)
{
	return 0;
}

static int armccipll_fsel_read(struct seq_file *m, void *v)
{
	return 0;
}

static int mmpll_fsel_read(struct seq_file *m, void *v)
{
	return 0;
}

static int vencpll_fsel_read(struct seq_file *m, void *v)
{
	return 0;
}

static int _clk_debugf_open(struct seq_file *s, void *data)
{
	return 0;
}

static ssize_t clk_debugf_read(struct file *filp,
			       char __user *userbuf, size_t count, loff_t *f_pos)
{
	int len = 0;
	char *p = dbg_buf;
	int i;
	char *buff_end = dbg_buf + dbg_bug_size;

	for (i = 0; i < NR_PLLS; i++) {
		if (pll_is_on(i))
			p += snprintf(p, buff_end - p, "suspend warning: %s is on!!!\n", plls[i].name);
	}
	for (i = 0; i < NR_SYSS; i++) {
		if (subsys_is_on(i)) {
			if ((i != SYS_MD1) && (i != SYS_C2K) && (i != SYS_MDSYS_INTF_INFRA))
				p += snprintf(p, buff_end - p, "suspend warning: %s is on!!!\n", syss[i].name);
			else
				p += snprintf(p, buff_end - p, "%s: on\n", syss[i].name);
		}
	}

	p += snprintf(p, buff_end - p, "********** PLL register dump *********\n");

	p += snprintf(p, buff_end - p, "[MAINPLL_CON0]=0x%08x\n", clk_readl(MAINPLL_CON0));
	p += snprintf(p, buff_end - p, "[MAINPLL_CON1]=0x%08x\n", clk_readl(MAINPLL_CON1));
	p += snprintf(p, buff_end - p, "[MAINPLL_PWR_CON0]=0x%08x\n", clk_readl(MAINPLL_PWR_CON0));

	p += snprintf(p, buff_end - p, "[UNIVPLL_CON0]=0x%08x\n", clk_readl(UNIVPLL_CON0));
	p += snprintf(p, buff_end - p, "[UNIVPLL_CON1]=0x%08x\n", clk_readl(UNIVPLL_CON1));
	p += snprintf(p, buff_end - p, "[UNIVPLL_PWR_CON0]=0x%08x\n", clk_readl(UNIVPLL_PWR_CON0));

	p += snprintf(p, buff_end - p, "[MSDCPLL_CON0]=0x%08x\n", clk_readl(MSDCPLL_CON0));
	p += snprintf(p, buff_end - p, "[MSDCPLL_CON1]=0x%08x\n", clk_readl(MSDCPLL_CON1));
	p += snprintf(p, buff_end - p, "[MSDCPLL_PWR_CON0]=0x%08x\n", clk_readl(MSDCPLL_PWR_CON0));

	p += snprintf(p, buff_end - p, "[VENCPLL_CON0]=0x%08x\n", clk_readl(VENCPLL_CON0));
	p += snprintf(p, buff_end - p, "[VENCPLL_CON1]=0x%08x\n", clk_readl(VENCPLL_CON1));
	p += snprintf(p, buff_end - p, "[VENCPLL_PWR_CON0]=0x%08x\n", clk_readl(VENCPLL_PWR_CON0));

	p += snprintf(p, buff_end - p, "[MMPLL_CON0]=0x%08x\n", clk_readl(MMPLL_CON0));
	p += snprintf(p, buff_end - p, "[MMPLL_CON1]=0x%08x\n", clk_readl(MMPLL_CON1));
	p += snprintf(p, buff_end - p, "[MMPLL_PWR_CON0]=0x%08x\n", clk_readl(MMPLL_PWR_CON0));

	p += snprintf(p, buff_end - p, "[TVDPLL_CON0]=0x%08x\n", clk_readl(TVDPLL_CON0));
	p += snprintf(p, buff_end - p, "[TVDPLL_CON1]=0x%08x\n", clk_readl(TVDPLL_CON1));
	p += snprintf(p, buff_end - p, "[TVDPLL_PWR_CON0]=0x%08x\n", clk_readl(TVDPLL_PWR_CON0));

	p += snprintf(p, buff_end - p, "[APLL1_CON0]=0x%08x\n", clk_readl(APLL1_CON0));
	p += snprintf(p, buff_end - p, "[APLL1_CON1]=0x%08x\n", clk_readl(APLL1_CON1));
	p += snprintf(p, buff_end - p, "[APLL1_PWR_CON0]=0x%08x\n", clk_readl(APLL1_PWR_CON0));

	p += snprintf(p, buff_end - p, "[APLL2_CON0]=0x%08x\n", clk_readl(APLL2_CON0));
	p += snprintf(p, buff_end - p, "[APLL2_CON1]=0x%08x\n", clk_readl(APLL2_CON1));
	p += snprintf(p, buff_end - p, "[APLL2_PWR_CON0]=0x%08x\n", clk_readl(APLL2_PWR_CON0));

	p += snprintf(p, buff_end - p, "[AP_PLL_CON3]=0x%08x\n", clk_readl(AP_PLL_CON3));
	p += snprintf(p, buff_end - p, "[AP_PLL_CON4]=0x%08x\n", clk_readl(AP_PLL_CON4));

	p += snprintf(p, buff_end - p, "********** subsys pwr sts register dump *********\n");
	p += snprintf(p, buff_end - p, "PWR_STATUS=0x%08x, PWR_STATUS_2ND=0x%08x\n",
		   clk_readl(PWR_STATUS), clk_readl(PWR_STATUS_2ND));

	p += snprintf(p, buff_end - p, "********** mux register dump *********\n");

	p += snprintf(p, buff_end - p, "[CLK_CFG_0]=0x%08x\n", clk_readl(CLK_CFG_0));
	p += snprintf(p, buff_end - p, "[CLK_CFG_1]=0x%08x\n", clk_readl(CLK_CFG_1));
	p += snprintf(p, buff_end - p, "[CLK_CFG_2]=0x%08x\n", clk_readl(CLK_CFG_2));
	p += snprintf(p, buff_end - p, "[CLK_CFG_3]=0x%08x\n", clk_readl(CLK_CFG_3));
	p += snprintf(p, buff_end - p, "[CLK_CFG_4]=0x%08x\n", clk_readl(CLK_CFG_4));
	p += snprintf(p, buff_end - p, "[CLK_CFG_5]=0x%08x\n", clk_readl(CLK_CFG_5));
	p += snprintf(p, buff_end - p, "[CLK_CFG_6]=0x%08x\n", clk_readl(CLK_CFG_6));
	p += snprintf(p, buff_end - p, "[CLK_CFG_7]=0x%08x\n", clk_readl(CLK_CFG_7));
	p += snprintf(p, buff_end - p, "[CLK_CFG_8]=0x%08x\n", clk_readl(CLK_CFG_8));
	p += snprintf(p, buff_end - p, "[CLK_CFG_9]=0x%08x\n", clk_readl(CLK_CFG_9));

	p += snprintf(p, buff_end - p, "********** CG register dump *********\n");
	p += snprintf(p, buff_end - p, "[INFRA_SUBSYS][%p]\n", clk_infracfg_ao_base);	/* 0x10001000 */

	p += snprintf(p, buff_end - p, "[INFRA_PDN_STA0]=0x%08x\n", clk_readl(INFRA_SW_CG0_STA));
	p += snprintf(p, buff_end - p, "[INFRA_PDN_STA1]=0x%08x\n", clk_readl(INFRA_SW_CG1_STA));
	p += snprintf(p, buff_end - p, "[INFRA_PDN_STA2]=0x%08x\n", clk_readl(INFRA_SW_CG2_STA));

	p += snprintf(p, buff_end - p, "[INFRA_TOPAXI_PROTECTSTA1]=0x%08x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA1));

	p += snprintf(p, buff_end - p, "[MMSYS_SUBSYS][%p]\n", clk_mmsys_config_base);
	p += snprintf(p, buff_end - p, "[DISP_CG_CON0]=0x%08x\n", clk_readl(DISP_CG_CON0));
	p += snprintf(p, buff_end - p, "[DISP_CG_CON1]=0x%08x\n", clk_readl(DISP_CG_CON1));
	p += snprintf(p, buff_end - p, "[DISP_CG_CON0_DUMMY]=0x%08x\n", clk_readl(DISP_CG_CON0_DUMMY));
	p += snprintf(p, buff_end - p, "[DISP_CG_CON1_DUMMY]=0x%08x\n", clk_readl(DISP_CG_CON1_DUMMY));

	/**/
	p += snprintf(p, buff_end - p, "[MFG_SUBSYS][%p]\n", clk_mfgcfg_base);
	p += snprintf(p, buff_end - p, "[MFG_CG_CON]=0x%08x\n", clk_readl(MFG_CG_CON));

	p += snprintf(p, buff_end - p, "[AUDIO_SUBSYS][%p]\n", clk_audio_base);
	p += snprintf(p, buff_end - p, "[AUDIO_TOP_CON0]=0x%08x\n", clk_readl(AUDIO_TOP_CON0));

	p += snprintf(p, buff_end - p, "[IMG_SUBSYS][%p]\n", clk_imgsys_base);
	p += snprintf(p, buff_end - p, "[IMG_CG_CON]=0x%08x\n", clk_readl(IMG_CG_CON));

	p += snprintf(p, buff_end - p, "[VDEC_SUBSYS][%p]\n", clk_vdec_gcon_base);
	p += snprintf(p, buff_end - p, "[VDEC_CKEN_SET]=0x%08x\n", clk_readl(VDEC_CKEN_SET));
	p += snprintf(p, buff_end - p, "[LARB_CKEN_SET]=0x%08x\n", clk_readl(LARB_CKEN_SET));

	p += snprintf(p, buff_end - p, "[VENC_SUBSYS][%p]\n", clk_venc_gcon_base);
	p += snprintf(p, buff_end - p, "[VENC_CG_CON]=0x%08x\n", clk_readl(VENC_CG_CON));

	p += snprintf(p, buff_end - p, "[CAM_SUBSYS][%p]\n", clk_camsys_base);
	p += snprintf(p, buff_end - p, "[CAM_CG_CON]=0x%08x\n", clk_readl(CAM_CG_CON));

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);

}

static int pll_div_value_map(int index)
{
	int div = 0; /*default div 1/1*/

	switch (index) {
	case 1:
		div = 0x08;
	break;
	case 2:
		div = 0x09;
	break;
	case 3:
		div = 0x0A;
	break;
	case 4:
		div = 0x0B;
	break;
	case 5:
		div = 0x10;
	break;
	case 6:
		div = 0x11;
	break;
	case 7:
		div = 0x12;
	break;
	case 8:
		div = 0x13;
	break;
	case 9:
		div = 0x14;
	break;
	case 10:
		div = 0x18;
	break;
	case 11:
		div = 0x19;
	break;
	case 12:
		div = 0x1A;
	break;
	case 13:
		div = 0x1B;
	break;
	case 14:
		div = 0x1C;
	break;
	case 15:
		div = 0x1D;
	break;
	default:
		div = 0;
	break;
	}

	return div;
}
static ssize_t armlpll_fsel_write(struct file *file, const char __user *buffer,
				  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int ctrl_value = 0;
	int div;
	unsigned int value;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%x %x", &div, &value) == 2) {
		ctrl_value = clk_readl(ARMPLL_L_CON1);
		ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= SDM_PLL_N_INFO_CHG;
		clk_writel(ARMPLL_L_CON1, ctrl_value);
		udelay(20);
		clk_writel(INFRA_TOPCKGEN_CKDIV1_BIG, pll_div_value_map(div));
	}
	return count;
}

static ssize_t armllpll_fsel_write(struct file *file, const char __user *buffer,
				   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int ctrl_value = 0;
	int div;
	unsigned int value;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%x %x", &div, &value) == 2) {
		ctrl_value = clk_readl(ARMPLL_LL_CON1);
		ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= SDM_PLL_N_INFO_CHG;
		clk_writel(ARMPLL_LL_CON1, ctrl_value);
		udelay(20);
		clk_writel(INFRA_TOPCKGEN_CKDIV1_SML, pll_div_value_map(div));
	}
	return count;
}

static ssize_t armccipll_fsel_write(struct file *file, const char __user *buffer,
				    size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int ctrl_value = 0;
	int div;
	unsigned int value;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%x %x", &div, &value) == 2) {
		ctrl_value = clk_readl(CCIPLL_CON1);
		ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= SDM_PLL_N_INFO_CHG;
		clk_writel(CCIPLL_CON1, ctrl_value);
		udelay(20);
		clk_writel(INFRA_TOPCKGEN_CKDIV1_BUS, pll_div_value_map(div));
	}
	return count;

}

static int apmixed_pll_div_value_map(int index)
{
	int div = 0; /*default div 1/1*/

	switch (index) {
	case 1:
		div = 0x0;
	break;
	case 2:
		div = 0x10;
	break;
	case 4:
		div = 0x20;
	break;
	case 8:
		div = 0x30;
	break;
	case 16:
		div = 0x40;
	break;
	default:
		div = 0;
	break;
	}
	return div;
}

static ssize_t mmpll_fsel_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int con0, con1;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%d %x", &con0, &con1) == 2) {
		clk_writel(MMPLL_CON1, con1);
		udelay(20);
	}

	return count;

}

static ssize_t vencpll_fsel_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int con0, con1, div, value;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%x %x", &con0, &con1) == 2) {
		div = apmixed_pll_div_value_map(con0);
		value = (clk_readl(VENCPLL_CON0)&0xFFFFFF8F) | div;
		/*pr_err("vencpll con0=0x%x, con1=0x%x\n",value,con1);*/
		clk_writel(VENCPLL_CON0, value);
		clk_writel(VENCPLL_CON1, con1);
		udelay(20);
	}

	return count;

}


static int gAllowClkDump;

static ssize_t clk_fsel_write(struct file *file, const char __user *buffer,
			  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int con0, clkDump;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%d %d", &con0, &clkDump) == 2) {
		if (clkDump)
			gAllowClkDump = 1;
		else
			gAllowClkDump = 0;
	}

	return count;

}


 /*MMPLL*/
#if 0
static int sdmpll_fsel_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *data)
{
	pr_err("[%s]: warning! warning! warning! to use this function\n", __func__);
	return 0;
}
#endif

void slp_check_pm_mtcmos_pll(void)
{
	int i;

	slp_chk_mtcmos_pll_stat = 1;

	for (i = 0; i < NR_PLLS; i++) {
		if (pll_is_on(i)) {
			slp_chk_mtcmos_pll_stat = -1;
			/*pr_err("%s: on\n", plls[i].name);*/
			pr_err("suspend warning: %s is on!!!\n", plls[i].name);
			/*pr_err("warning! warning! warning! it may cause resume fail\n");*/
		}
	}
	for (i = 0; i < NR_SYSS; i++) {
		if (subsys_is_on(i)) {
			if ((i != SYS_MD1) && (i != SYS_C2K) && (i != SYS_MDSYS_INTF_INFRA)) {
				slp_chk_mtcmos_pll_stat = -1;
				pr_err("suspend warning: %s is on!!!\n", syss[i].name);
				/*pr_err("warning! warning! warning! it may cause resume fail\n");*/
			} else
				pr_err("%s: on\n", syss[i].name);
		}
	}
	if (slp_chk_mtcmos_pll_stat != 1 && gAllowClkDump)
		clk_dump();
}
EXPORT_SYMBOL(slp_check_pm_mtcmos_pll);


static int slp_chk_mtcmos_pll_stat_read(struct seq_file *m, void *v)
{
	clk_err("%s", __func__);

	seq_printf(m, "%d\n", slp_chk_mtcmos_pll_stat);
	return 0;
}

static int proc_armlpll_fsel_open(struct inode *inode, struct file *file)
{
	clk_err("%s", __func__);

	return single_open(file, armlpll_fsel_read, NULL);
}

static const struct file_operations armlpll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_armlpll_fsel_open,
	.read = seq_read,
	.write = armlpll_fsel_write,
	.release = single_release,
};

static int proc_armllpll_fsel_open(struct inode *inode, struct file *file)
{
	clk_err("%s", __func__);

	return single_open(file, armllpll_fsel_read, NULL);
}

static const struct file_operations armllpll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_armllpll_fsel_open,
	.read = seq_read,
	.write = armllpll_fsel_write,
	.release = single_release,
};

static int proc_armccipll_fsel_open(struct inode *inode, struct file *file)
{
	clk_err("%s", __func__);

	return single_open(file, armccipll_fsel_read, NULL);
}

static const struct file_operations armccipll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_armccipll_fsel_open,
	.read = seq_read,
	.write = armccipll_fsel_write,
	.release = single_release,
};

static int proc_mmpll_fsel_open(struct inode *inode, struct file *file)
{
	clk_err("%s", __func__);
	return single_open(file, mmpll_fsel_read, NULL);
}

static const struct file_operations mmpll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_mmpll_fsel_open,
	.read = seq_read,
	.write = mmpll_fsel_write,
	.release = single_release,
};

static int proc_vencpll_fsel_open(struct inode *inode, struct file *file)
{
	clk_err("%s", __func__);
	return single_open(file, vencpll_fsel_read, NULL);
}

static const struct file_operations vencpll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_vencpll_fsel_open,
	.read = seq_read,
	.write = vencpll_fsel_write,
	.release = single_release,
};

static int proc_clk_debugf_open(struct inode *inode, struct file *file)
{
	clk_err("%s", __func__);
	return single_open(file, _clk_debugf_open, NULL);
}

static const struct file_operations clk_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_clk_debugf_open,
	.read = clk_debugf_read,
	.write = clk_fsel_write,
	.release = single_release,
};

static int proc_slp_chk_mtcmos_pll_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, slp_chk_mtcmos_pll_stat_read, NULL);
}

static const struct file_operations slp_chk_mtcmos_pll_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_slp_chk_mtcmos_pll_stat_open,
	.read = seq_read,
};


#if 1 /* freq meter , PLL*/
/***************************
*For TOPCKGen Meter LDVT Test
****************************/
unsigned int ckgen_meter(int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk26cali_0 = clk_readl(CLK26CALI_0);

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);

	/*sel ckgen_cksw[0] and enable freq meter sel ckgen[13:8], 01:hd_faxi_ck*/
	clk_writel(CLK_DBG_CFG, (ID << 8) | 0x01);

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));	/*select divider?dvt set zero*/

	clk26cali_1 = clk_readl(CLK26CALI_1);
	/*clk_writel(CLK26CALI_1, 0x00ff0000); */

	/*temp = clk_readl(CLK26CALI_0);*/
	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		udelay(10);
		i++;
		if (i > 10000)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024;	/* Khz*/

	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x0000);

	if (i > 10000)
		return 0;
	else
		return output;

}

unsigned int abist_meter_32k(void)
{
	int output = 0, i = 0;
	unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);

	/*sel abist_cksw and enable freq meter sel abist*/
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFC0FFFC) | (39 << 16));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);

	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0 & 0x00FFFFFF);	/* select divider, WAIT CONFIRM*/

	clk26cali_1 = clk_readl(CLK26CALI_1);
	/*clk_writel(CLK26CALI_1, 0x00ff0000); // cycle count default 1024,[25:16]=3FF*/

	clk_writel(CLK26CALI_0, 0x0100);
	temp = clk_readl(CLK26CALI_1);
	clk_writel(CLK26CALI_1, (temp&0xfc00ffff)|(0x40<<16));

	/*temp = clk_readl(CLK26CALI_0);*/
	clk_writel(CLK26CALI_0, 0x1100);
	clk_writel(CLK26CALI_0, 0x1110);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		udelay(10);
		i++;
		if (i > 10000)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = (64*26000) / temp;

	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x0000);

	clk_writel(CLK26CALI_1, (temp&0xfc00ffff)|(0x3FF<<16));

	if (i > 10000)
		return 0;
	else
		return output;
}


unsigned int abist_meter(int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	if (ID == 39)
		return abist_meter_32k();

	clk_writel(TOP_METER, 0x0);

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);

	/*sel abist_cksw and enable freq meter sel abist*/
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFC0FFFC) | (ID << 16));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);

	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF)|0x01000000);	/* select divider, WAIT CONFIRM*/

	clk26cali_1 = clk_readl(CLK26CALI_1);
	/*clk_writel(CLK26CALI_1, 0x00ff0000); // cycle count default 1024,[25:16]=3FF*/

	/*temp = clk_readl(CLK26CALI_0);*/
	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		udelay(10);
		i++;
		if (i > 10000)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024 * 2;	/* Khz*/

	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x0000);

	if (i > 10000)
		return 0;
	else
		return output;
}

const char *ckgen_array[] = {
	"hd_faxi_ck",
	"hf_fddrphycfg_ck",
	"hf_fmm_ck",
	"f_fpwm_ck",
	"hf_fvdec_ck",
	"hf_fmfg_ck",
	"hf_fcamtg_ck",
	"f_fuart_ck",
	"hf_fspi_ck",
	"hf_fmsdc50_0_hclk_ck", /*10*/
	"hf_fmsdc50_0_ck",
	"hf_fmsdc30_1_ck",
	"hf_fmsdc30_2_ck",
	"hf_fmsdc30_3_ck",
	"hf_faudio_ck",
	"hf_faud_intbus_ck",
	"hf_fpmicspi_ck",
	"b0",
	"hf_fatb_ck",
	"hf_fmjc_ck",/*20*/
	"hf_fdpi0_ck",
	"hf_fscam_ck",
	"hf_faud_1_ck",
	"hf_faud_2_ck",
	"fmem_ck_bfe_dcm_ch0_gt",
	"fmem_ck_aft_dcm_ch0",
	"f_fdisp_pwm_ck",
	"f_fssusb_top_sys_ck",
	"f_fssusb_top_xhci_ck",
	"f_fusb_top_ck",/*30*/
	"hg_fspm_ck",
	"hf_fbsi_spi_ck",
	"f_fi2c_ck",
	"hg_fdvfsp_ck",
	"hf_fvenc_ck",
	"f52m_mfg_ck",
	"hf_fimg_ck",
	"fmem_ck_bfe_dcm_ch1_gt",
	"fmem_ck_aft_dcm_ch1"
};

const char *ckgen_abist_array[] = {
	"AD_CSI0_DELAY_TSTCLK",
	"AD_CSI1_DELAY_TSTCLK",
	"AD_PSMCUPLL_CK",
	"AD_L1MCUPLL_CK",
	"AD_C2KCPPLL_CK",
	"AD_ICCPLL_CK",
	"AD_INTFPLL_CK",
	"AD_MD2GPLL_CK",
	"AD_IMCPLL_CK",
	"AD_C2KDSPPLL_CK",/*10*/
	"AD_BRPPLL_CK",
	"AD_DFEPLL_CK",
	"AD_EQPLL_CK",
	"AD_CMPPLL_CK",
	"AD_MDPLLGP_TST_CK",
	"AD_MDPLL_FS26M_CK",
	"AD_MDPLL_FS208M_CK",
	"b0",
	"b0",
	"AD_ARMPLL_L_CK",/*20*/
	"b0",
	"AD_ARMPLL_LL_CK",
	"AD_MAINPLL_CK",
	"AD_UNIVPLL_CK",
	"AD_MMPLL_CK",
	"AD_MSDCPLL_CK",
	"AD_VENCPLL_CK",
	"AD_APLL1_CK",
	"AD_APLL2_CK",
	"AD_APPLLGP_MON_FM_CK",/*30*/
	"AD_USB20_192M_CK",
	"AD_UNIV_192M_CK",
	"AD_SSUSB_192M_CK",
	"AD_TVDPLL_CK",
	"AD_DSI0_MPPLL_TST_CK",
	"AD_DSI0_LNTC_DSICLK",
	"b0",
	"AD_OSC_CK",
	"rtc32k_ck_i",
	"armpll_hd_fram_ck_big",/*40*/
	"armpll_hd_fram_ck_sml",
	"armpll_hd_fram_ck_bus",
	"msdc01_in_ck",
	"msdc02_in_ck",
	"msdc11_in_ck",
	"msdc12_in_ck",
	"b0",
	"b0",
	"AD_CCIPLL_CK",
	"AD_MPLL_208M_CK",/*50*/
	"AD_WBG_DIG_CK_CK_416M",
	"AD_WBG_B_DIG_CK_64M",
	"AD_WBG_W_DIG_CK_160M",
	"DA_USB20_48M_DIV_CK",
	"DA_UNIV_48M_DIV_CK",
	"DA_SSUSB_48M_DIV_CK",
	"DA_MPLL_52M_DIV_CK"
};


static int ckgen_meter_read(struct seq_file *m, void *v)
{
	int i;

	for (i = 1; i <= 39; i++) {
		/*skip unknown case */
		if (i == 18)
			continue;

		seq_printf(m, "[%d] %s: %d\n", i, ckgen_array[i - 1], ckgen_meter(i));
	}
	return 0;
}

static ssize_t ckgen_meter_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	int val;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtoint(desc, 10, &val) >= 0)
		pr_debug("ckgen_meter %d is %d\n", val, ckgen_meter(val));

	return count;
}


static int abist_meter_read(struct seq_file *m, void *v)
{
	int i;

	for (i = 1; i <= 57; i++) {
		/*skip unknown case */
		if (i == 18 || i == 19 || i == 21 || i == 37 || i == 47 || i == 48)
			continue;
		if (i == 39)
			seq_printf(m, "[%d] %s: %d\n", i, ckgen_abist_array[i - 1], abist_meter_32k());
		else
			seq_printf(m, "[%d] %s: %d\n", i, ckgen_abist_array[i - 1], abist_meter(i));
	}
	return 0;
}

static ssize_t abist_meter_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	int val;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtoint(desc, 10, &val) >= 0) {
		if (val == 39)
			pr_err("abist_meter %d is %d\n", val, abist_meter_32k());
		else
			pr_err("abist_meter %d is %d\n", val, abist_meter(val));
	}
	return count;
}

static int proc_abist_meter_open(struct inode *inode, struct file *file)
{
	return single_open(file, abist_meter_read, NULL);
}

static const struct file_operations abist_meter_fops = {
	.owner = THIS_MODULE,
	.open = proc_abist_meter_open,
	.read = seq_read,
	.write = abist_meter_write,
};

static int proc_ckgen_meter_open(struct inode *inode, struct file *file)
{
	return single_open(file, ckgen_meter_read, NULL);
}

static const struct file_operations ckgen_meter_fops = {
	.owner = THIS_MODULE,
	.open = proc_ckgen_meter_open,
	.read = seq_read,
	.write = ckgen_meter_write,
};

/*********************************************************************
 * FUNCTION DEFINATIONS
 ********************************************************************/

static unsigned int mt_get_emi_freq(void)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk26cali_0 = clk_readl(CLK26CALI_0);
	/*sel abist_cksw and enable abist meter sel abist*/
	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFFFFFFC) | (50 << 16));/*AD_MPLL_208M_CK*/

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));	/* select divider*/

	clk26cali_1 = clk_readl(CLK26CALI_1);
	/*clk_writel(CLK26CALI_1, 0x00ff0000); */

	/*temp = clk_readl(CLK26CALI_0);*/
	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024;	/* Khz*/

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return output;

}
EXPORT_SYMBOL(mt_get_emi_freq);

unsigned int mt_get_bus_freq(void)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk26cali_0 = clk_readl(CLK26CALI_0);

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	clk_writel(CLK_DBG_CFG, (1 << 8) | 0x01);	/* AXI */

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));	/* select divider*/

	clk26cali_1 = clk_readl(CLK26CALI_1);
	/*clk_writel(CLK26CALI_1, 0x00ff0000); */

	/*temp = clk_readl(CLK26CALI_0);*/
	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024;	/* Khz*/

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return output;


}
EXPORT_SYMBOL(mt_get_bus_freq);


static int cpu_speed_dump_read(struct seq_file *m, void *v)
{

	seq_printf(m, "%s(LL): %d Khz\n", ckgen_abist_array[21], abist_meter(22));
	seq_printf(m, "%s(L): %d Khz\n", ckgen_abist_array[19], abist_meter(20));
	seq_printf(m, "%s(CCI): %d Khz\n", ckgen_abist_array[48], abist_meter(49));

	return 0;
}

static int mm_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s :  %d Khz\n", ckgen_abist_array[24], abist_meter(25));
	return 0;
}

static int venc_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s :  %d Khz\n", ckgen_abist_array[26], abist_meter(27));
	return 0;
}

static int emi_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_emi_freq());
	return 0;
}

static int bus_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_bus_freq());
	return 0;
}

#if 0
static int mmclk_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_mmclk_freq());
	return 0;
}

static int mfgclk_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_mfgclk_freq());
	return 0;
}
#endif

static int proc_cpu_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpu_speed_dump_read, NULL);
}

static const struct file_operations cpu_fops = {
	.owner = THIS_MODULE,
	.open = proc_cpu_open,
	.read = seq_read,
};

static int proc_mm_open(struct inode *inode, struct file *file)
{
	return single_open(file, mm_speed_dump_read, NULL);
}

static const struct file_operations mm_fops = {
	.owner = THIS_MODULE,
	.open = proc_mm_open,
	.read = seq_read,
};

static int proc_venc_open(struct inode *inode, struct file *file)
{
	return single_open(file, venc_speed_dump_read, NULL);
}

static const struct file_operations venc_fops = {
	.owner = THIS_MODULE,
	.open = proc_venc_open,
	.read = seq_read,
};

static int proc_emi_open(struct inode *inode, struct file *file)
{
	return single_open(file, emi_speed_dump_read, NULL);
}

static const struct file_operations emi_fops = {
	.owner = THIS_MODULE,
	.open = proc_emi_open,
	.read = seq_read,
};

static int proc_bus_open(struct inode *inode, struct file *file)
{
	return single_open(file, bus_speed_dump_read, NULL);
}

static const struct file_operations bus_fops = {
	.owner = THIS_MODULE,
	.open = proc_bus_open,
	.read = seq_read,
};

#if 0
static int proc_mmclk_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmclk_speed_dump_read, NULL);
}

static const struct file_operations mmclk_fops = {
	.owner = THIS_MODULE,
	.open = proc_mmclk_open,
	.read = seq_read,
};

static int proc_mfgclk_open(struct inode *inode, struct file *file)
{
	return single_open(file, mfgclk_speed_dump_read, NULL);
}

static const struct file_operations mfgclk_fops = {
	.owner = THIS_MODULE,
	.open = proc_mfgclk_open,
	.read = seq_read,
};
#endif


#endif /* freq meter */

void mt_clkmgr_debug_init(void)
{
/*use proc_create*/
	struct proc_dir_entry *entry;
	struct proc_dir_entry *clkmgr_dir;

	clkmgr_dir = proc_mkdir("clkmgr", NULL);
	if (!clkmgr_dir) {
		clk_err("[%s]: fail to mkdir /proc/clkmgr\n", __func__);
		return;
	}
	entry =
	    proc_create("armlpll_fsel", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&armlpll_fsel_proc_fops);
	entry =
	    proc_create("armllpll_fsel", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&armllpll_fsel_proc_fops);
	entry =
	    proc_create("armccipll_fsel", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&armccipll_fsel_proc_fops);
	entry =
	    proc_create("mmpll_fsel", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&mmpll_fsel_proc_fops);
	entry =
	    proc_create("vencpll_fsel", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&vencpll_fsel_proc_fops);
	entry =
	    proc_create("pll_mtcmos_clk", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&clk_fsel_proc_fops);

	/*entry = proc_create("slp_chk_mtcmos_pll_stat", S_IRUGO, clkmgr_dir, &slp_chk_mtcmos_pll_stat_proc_fops);*/
}

#ifdef CONFIG_OF
void iomap(void)
{
	struct device_node *node;

/*apmixed*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
	if (!node)
		clk_dbg("[CLK_APMIXED] find node failed\n");
	clk_apmixed_base = of_iomap(node, 0);
	if (!clk_apmixed_base)
		clk_dbg("[CLK_APMIXED] base failed\n");
/*cksys_base*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	if (!node)
		clk_dbg("[CLK_CKSYS] find node failed\n");
	clk_cksys_base = of_iomap(node, 0);
	if (!clk_cksys_base)
		clk_dbg("[CLK_CKSYS] base failed\n");
/*infracfg_ao*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
	if (!node)
		clk_dbg("[CLK_INFRACFG_AO] find node failed\n");
	clk_infracfg_ao_base = of_iomap(node, 0);
	if (!clk_infracfg_ao_base)
		clk_dbg("[CLK_INFRACFG_AO] base failed\n");
/*audio*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,audio");
	if (!node)
		clk_dbg("[CLK_AUDIO] find node failed\n");
	clk_audio_base = of_iomap(node, 0);
	if (!clk_audio_base)
		clk_dbg("[CLK_AUDIO] base failed\n");
/*mfgcfg*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,g3d_config");
	if (!node)
		clk_dbg("[CLK_MFGCFG node failed\n");
	clk_mfgcfg_base = of_iomap(node, 0);
	if (!clk_mfgcfg_base)
		clk_dbg("[CLK_G3D_CONFIG] base failed\n");
/*mmsys_config*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,mmsys_config");
	if (!node)
		clk_dbg("[CLK_MMSYS_CONFIG] find node failed\n");
	clk_mmsys_config_base = of_iomap(node, 0);
	if (!clk_mmsys_config_base)
		clk_dbg("[CLK_MMSYS_CONFIG] base failed\n");
/*imgsys*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,imgsys_config");
	if (!node)
		clk_dbg("[CLK_IMGSYS_CONFIG] find node failed\n");
	clk_imgsys_base = of_iomap(node, 0);
	if (!clk_imgsys_base)
		clk_dbg("[CLK_IMGSYS_CONFIG] base failed\n");
/*vdec_gcon*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,vdec_gcon");
	if (!node)
		clk_dbg("[CLK_VDEC_GCON] find node failed\n");
	clk_vdec_gcon_base = of_iomap(node, 0);
	if (!clk_vdec_gcon_base)
		clk_dbg("[CLK_VDEC_GCON] base failed\n");
/*venc_gcon*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,venc_gcon");
	if (!node)
		clk_dbg("[CLK_VENC_GCON] find node failed\n");
	clk_venc_gcon_base = of_iomap(node, 0);
	if (!clk_venc_gcon_base)
		clk_dbg("[CLK_VENC_GCON] base failed\n");

/*cam*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,camsys_config");
	if (!node)
		clk_dbg("[CLK_CAM] find node failed\n");
	clk_camsys_base = of_iomap(node, 0);
	if (!clk_camsys_base)
		clk_dbg("[CLK_CAM] base failed\n");

/*topmics*/
#if 0
	node = of_find_compatible_node(NULL, NULL, "mediatek,topmisc");
	if (!node)
		clk_dbg("[CLK_TOPMISC] find node failed\n");
	clk_topmics_base = of_iomap(node, 0);
	if (!clk_topmics_base)
		clk_dbg("[CLK_TOPMISC] base failed\n");
#endif
}
#endif

#define FMETER_EN_BIT (1<<12)
void mt_disable_dbg_clk(void)
{
#if 0
	clk_writel(CLK26CALI_0, clk_readl(CLK26CALI_0) & ~FMETER_EN_BIT);
	clk_writel(CLK_MISC_CFG_0, clk_readl(CLK_MISC_CFG_0) | 0xFFFF0000);
	clk_writel(CLK_MISC_CFG_1, clk_readl(CLK_MISC_CFG_1) | 0xFFFF0000);

	clk_writel(ARMPLLDIV_ARM_K1, 0xFFFFFFFF);
	clk_writel(ARMPLLDIV_MON_EN, 0);

	clk_writel(INFRA_AO_DBG_CON0, 1);
	clk_writel(INFRA_AO_DBG_CON1, 1);
	clk_writel(INFRA_MODULE_CLK_SEL, 0x00004c70);
	clk_writel(TOPMISC_TEST_MODE_CFG, 0x030c0000);
#endif
}

void mt_clkmgr_init(void)
{
	iomap();
	WARN_ON(initialized);
	mt_plls_init();
	mt_subsys_init();
	mt_disable_dbg_clk();
}

static int __init mt_clkmgr_meter_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *pm_init_dir = NULL;

	mt_clkmgr_init();

	pm_init_dir = proc_mkdir("pm_init", NULL);
	/* pm_init_dir = proc_mkdir("pm_init", NULL); */
	if (!pm_init_dir) {
		pr_debug("[%s]: mkdir /proc/pm_init failed\n", __func__);
	} else {
		entry = proc_create("cpu_speed_dump", S_IRUGO, pm_init_dir, &cpu_fops);
		entry = proc_create("mm_speed_dump", S_IRUGO | S_IWGRP, pm_init_dir, &mm_fops);
		entry = proc_create("venc_speed_dump", S_IRUGO | S_IWGRP, pm_init_dir, &venc_fops);
		entry = proc_create("emi_speed_dump", S_IRUGO | S_IWGRP, pm_init_dir, &emi_fops);
		entry =
		    proc_create("abist_meter_test", S_IRUGO | S_IWUSR, pm_init_dir,
				&abist_meter_fops);
		entry =
		    proc_create("ckgen_meter_test", S_IRUGO | S_IWUSR, pm_init_dir,
				&ckgen_meter_fops);
	}

#endif
	return 0;
}

static int mt_clkmgr_debug_module_init(void)
{
	mt_clkmgr_debug_init();
	return 0;
}

arch_initcall(mt_clkmgr_meter_init);
module_init(mt_clkmgr_debug_module_init);
