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
#include "mt_clkmgr.h"
/* #include <mach/mt_dcm.h> */
/* #include <mach/mt_idvfs.h> */ /* Fix when idvfs ready */
#include "mt_spm.h"
#include <mach/mt_spm_mtcmos.h>
/* #include <mach/mt_spm_sleep.h> */
/* #include <mach/mt_gpufreq.h> */
/* #include <mach/irqs.h> */

/* #include <mach/upmu_common.h> */
/* #include <mach/upmu_sw.h> */
/* #include <mach/upmu_hw.h> */
#include "mt_freqhopping_drv.h"

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
/*void __iomem *clk_mcumixed_base;*/
void __iomem *clk_camsys_base;
void __iomem *clk_topmics_base;
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
		 .name = __stringify(MFGPLL),
		 .en_mask = 0xFC000101,
		 }, {
		     .name = __stringify(MSDCPLL),
		     .en_mask = 0x00000111,
		     }, {
			 .name = __stringify(IMGPLL),
			 .en_mask = 0x00000111,
			 }, {
			     .name = __stringify(TVDPLL),
			     .en_mask = 0x00000101,
			     }, {
				 .name = __stringify(MPLL),
				 .en_mask = 0x00000141,
				 }, {
				     .name = __stringify(CODECPLL),
				     .en_mask = 0x00000121,
				     }, {
					 .name = __stringify(VDECPLL),
					 .en_mask = 0x00000121,
					 }, {
					     .name = __stringify(APLL1),
					     .en_mask = 0x00000131,
					     }, {
						 .name = __stringify(APLL2),
						 .en_mask = 0x00000131,
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

	BUG_ON(!pll);

	clkmgr_lock(flags);
	state = pll_get_state_op(pll);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(pll_is_on);
void enable_armpll_ll(void)
{
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_PWR_CON0, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_PWR_CON0) | 0x01));
	udelay(1);
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_PWR_CON0,
		(mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_PWR_CON0) & 0xfffffffd));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_CON1, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_CON1) | 0xC0000000));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_CON0, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_CON0) | 0x01));
	udelay(200);
}
EXPORT_SYMBOL(enable_armpll_ll);
void disable_armpll_ll(void)
{
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_CON0, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_CON0) & 0xfffffffe));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_PWR_CON0,
		(mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_PWR_CON0) | 0x00000002));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_PWR_CON0,
		(mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_PWR_CON0) & 0xfffffffe));
}
EXPORT_SYMBOL(disable_armpll_ll);

void enable_armpll_l(void)
{
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_PWR_CON0, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_PWR_CON0) | 0x01));
	udelay(1);
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_PWR_CON0,
		(mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_PWR_CON0) & 0xfffffffd));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_CON1, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_CON1) | 0xC0000000));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_CON0, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_CON0) | 0x01));
	udelay(200);
}
EXPORT_SYMBOL(enable_armpll_l);
void disable_armpll_l(void)
{
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_CON0, (mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_CON0) & 0xfffffffe));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_PWR_CON0,
		(mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_PWR_CON0) | 0x00000002));
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_PWR_CON0,
		(mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_PWR_CON0) & 0xfffffffe));
}
EXPORT_SYMBOL(disable_armpll_l);
void switch_armpll_l_hwmode(int enable)
{
	/* ARM CA15*/
	if (enable == 1) {
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON0, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON0) & 0xffffdddd));
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON1, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON1) & 0xffffffdd));

	} else{
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON0, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON0) | 0x00002222));
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON1, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON1) | 0x00000022));
	}
}
EXPORT_SYMBOL(switch_armpll_l_hwmode);

void switch_armpll_ll_hwmode(int enable)
{
	/* ARM CA7*/
	if (enable == 1) {
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON0, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON0) & 0xffffeeee));
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON1, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON1) & 0xffffffee));
	} else {
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON0, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON0) | 0x00001111));
		mt6797_0x1001AXXX_reg_write(MCU_PLL_CON1, (mt6797_0x1001AXXX_reg_read(MCU_PLL_CON1) | 0x00000011));
	}
}
EXPORT_SYMBOL(switch_armpll_ll_hwmode);
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
		     .name = __stringify(SYS_ISP),
		     .sta_mask = 1U << 5,
		     }, {
			 .name = __stringify(SYS_VDE),
			 .sta_mask = 1U << 7,
			 }, {
			     .name = __stringify(SYS_MFG_CORE3),
			     .sta_mask = 1U << 8,
			     }, {
				 .name = __stringify(SYS_MFG_CORE2),
				 .sta_mask = 1U << 9,
				 }, {
				     .name = __stringify(SYS_MFG_CORE1),
				     .sta_mask = 1U << 10,
				     }, {
					 .name = __stringify(SYS_MFG_CORE0),
					 .sta_mask = 1U << 11,
					 }, {
					     .name = __stringify(SYS_MFG),
					     .sta_mask = 1U << 12,
					     }, {
						 .name = __stringify(SYS_MFG_ASYNC),
						 .sta_mask = 1U << 13,
						 }, {
						     .name = __stringify(SYS_MJC),
						     .sta_mask = 1U << 20,
						     }, {
							 .name = __stringify(SYS_VEN),
							 .sta_mask = 1U << 21,
							 }, {
							     .name = __stringify(SYS_AUDIO),
							     .sta_mask = 1U << 24,
							     }, {
								 .name = __stringify(SYS_C2K),
								 .sta_mask = 1U << 28,
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

	BUG_ON(!sys);

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
	syss[SYS_ISP].ctl_addr = ISP_PWR_CON;
	syss[SYS_VDE].ctl_addr = VDE_PWR_CON;
	syss[SYS_MFG_CORE3].ctl_addr = MFG_CORE3_PWR_CON;
	syss[SYS_MFG_CORE2].ctl_addr = MFG_CORE2_PWR_CON;
	syss[SYS_MFG_CORE1].ctl_addr = MFG_CORE1_PWR_CON;
	syss[SYS_MFG_CORE0].ctl_addr = MFG_CORE0_PWR_CON;
	syss[SYS_MFG].ctl_addr = MFG_PWR_CON;
	syss[SYS_MFG_ASYNC].ctl_addr = MFG_ASYNC_PWR_CON;
	syss[SYS_MJC].ctl_addr = MJC_PWR_CON;
	syss[SYS_VEN].ctl_addr = VEN_PWR_CON;
	syss[SYS_AUDIO].ctl_addr = AUDIO_PWR_CON;
	syss[SYS_C2K].ctl_addr = C2K_PWR_CON;
}

static void mt_plls_init(void)
{
	plls[MAINPLL].base_addr = MAINPLL_CON0;
	plls[MAINPLL].pwr_addr = MAINPLL_PWR_CON0;
	plls[UNIVPLL].base_addr = UNIVPLL_CON0;
	plls[UNIVPLL].pwr_addr = UNIVPLL_PWR_CON0;
	plls[MFGPLL].base_addr = MFGPLL_CON0;
	plls[MFGPLL].pwr_addr = MFGPLL_PWR_CON0;
	plls[MSDCPLL].base_addr = MSDCPLL_CON0;
	plls[MSDCPLL].pwr_addr = MSDCPLL_PWR_CON0;
	plls[IMGPLL].base_addr = IMGPLL_CON0;
	plls[IMGPLL].pwr_addr = IMGPLL_PWR_CON0;
	plls[TVDPLL].base_addr = TVDPLL_CON0;
	plls[TVDPLL].pwr_addr = TVDPLL_PWR_CON0;
	plls[MPLL].base_addr = MPLL_CON0;
	plls[MPLL].pwr_addr = MPLL_PWR_CON0;
	plls[CODECPLL].base_addr = CODECPLL_CON0;
	plls[CODECPLL].pwr_addr = CODECPLL_PWR_CON0;
	plls[VDECPLL].base_addr = VDECPLL_CON0;
	plls[VDECPLL].pwr_addr = VDECPLL_PWR_CON0;
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

	pr_err("[MFGPLL_CON0]=0x%08x\n", clk_readl(MFGPLL_CON0));
	pr_err("[MFGPLL_CON1]=0x%08x\n", clk_readl(MFGPLL_CON1));
	pr_err("[MFGPLL_PWR_CON0]=0x%08x\n", clk_readl(MFGPLL_PWR_CON0));

	pr_err("[MSDCPLL_CON0]=0x%08x\n", clk_readl(MSDCPLL_CON0));
	pr_err("[MSDCPLL_CON1]=0x%08x\n", clk_readl(MSDCPLL_CON1));
	pr_err("[MSDCPLL_PWR_CON0]=0x%08x\n", clk_readl(MSDCPLL_PWR_CON0));

	pr_err("[IMGPLL_CON0]=0x%08x\n", clk_readl(IMGPLL_CON0));
	pr_err("[IMGPLL_CON1]=0x%08x\n", clk_readl(IMGPLL_CON1));
	pr_err("[IMGPLL_PWR_CON0]=0x%08x\n", clk_readl(IMGPLL_PWR_CON0));

	pr_err("[CODECPLL_CON0]=0x%08x\n", clk_readl(CODECPLL_CON0));
	pr_err("[CODECPLL_CON1]=0x%08x\n", clk_readl(CODECPLL_CON1));
	pr_err("[CODECPLL_PWR_CON0]=0x%08x\n", clk_readl(CODECPLL_PWR_CON0));

	pr_err("[VDECPLL_CON0]=0x%08x\n", clk_readl(VDECPLL_CON0));
	pr_err("[VDECPLL_CON1]=0x%08x\n", clk_readl(VDECPLL_CON1));
	pr_err("[VDECPLL_PWR_CON0]=0x%08x\n", clk_readl(VDECPLL_PWR_CON0));

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

	pr_err("********** CG register dump *********\n");
	pr_err("[INFRA_SUBSYS][%p]\n", clk_infracfg_ao_base);	/* 0x10001000 */

	pr_err("[INFRA_PDN_STA0]=0x%08x\n", clk_readl(INFRA_SW_CG0_STA));
	pr_err("[INFRA_PDN_STA1]=0x%08x\n", clk_readl(INFRA_SW_CG1_STA));
	pr_err("[INFRA_PDN_STA2]=0x%08x\n", clk_readl(INFRA_SW_CG2_STA));

	pr_err("[INFRA_TOPAXI_PROTECTSTA1]=0x%08x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA1));

	pr_err("[MMSYS_SUBSYS][%p]\n", clk_mmsys_config_base);
	pr_err("[DISP_CG_CON0]=0x%08x\n", clk_readl(DISP_CG_CON0));
	pr_err("[DISP_CG_CON1]=0x%08x\n", clk_readl(DISP_CG_CON1));

	/**/
	pr_err("[MFG_SUBSYS][%p]\n", clk_mfgcfg_base);
	pr_err("[MFG_CG_CON]=0x%08x\n", clk_readl(MFG_CG_CON));

	pr_err("[AUDIO_SUBSYS][%p]\n", clk_audio_base);
	pr_err("[AUDIO_TOP_CON0]=0x%08x\n", clk_readl(AUDIO_TOP_CON0));
	pr_err("[AUDIO_TOP_CON1]=0x%08x\n", clk_readl(AUDIO_TOP_CON1));

	pr_err("[IMG_SUBSYS][%p]\n", clk_imgsys_base);
	pr_err("[IMG_CG_CON]=0x%08x\n", clk_readl(IMG_CG_CON));

	pr_err("[VDEC_SUBSYS][%p]\n", clk_vdec_gcon_base);
	pr_err("[VDEC_CKEN_SET]=0x%08x\n", clk_readl(VDEC_CKEN_SET));
	pr_err("[LARB_CKEN_SET]=0x%08x\n", clk_readl(LARB_CKEN_SET));

	pr_err("[VENC_SUBSYS][%p]\n", clk_venc_gcon_base);
	pr_err("[VENC_CG_CON]=0x%08x\n", clk_readl(VENC_CG_CON));

	pr_err("[CAM_SUBSYS][%p]\n", clk_camsys_base);
	pr_err("[CAM_CG_CON]=0x%08x\n", clk_readl(CAM_CG_CON));

	pr_err("[MJC_SUBSYS][%p]\n", clk_mjc_config_base);
	pr_err("[MJC_CG_CON]=0x%08x\n", clk_readl(MJC_CG_CON));
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

	temp = mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_PWR_CON0);
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = mt6797_0x1001AXXX_reg_read(ARMCAXPLL3_PWR_CON0);
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL3_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(UNIVPLL_PWR_CON0);
	clk_writel(UNIVPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(MFGPLL_PWR_CON0);
	clk_writel(MFGPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(MSDCPLL_PWR_CON0);
	clk_writel(MSDCPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(VDECPLL_PWR_CON0);
	clk_writel(VDECPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(IMGPLL_PWR_CON0);
	clk_writel(IMGPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(CODECPLL_PWR_CON0);
	clk_writel(CODECPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(TVDPLL_PWR_CON0);
	clk_writel(TVDPLL_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(APLL1_PWR_CON0);
	clk_writel(APLL1_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);

	temp = clk_readl(APLL2_PWR_CON0);
	clk_writel(APLL2_PWR_CON0, (temp&PLL_PWR_CON0_ISO_EN_MASK) | PLL_PWR_CON0_ISO_EN_VALUE);


    /***********************
      * xPLL Frequency Enable
      ************************/
	temp = mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_CON0);
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_CON0, PLL_CON0_VALUE);

	temp = mt6797_0x1001AXXX_reg_read(ARMCAXPLL3_CON0);
	mt6797_0x1001AXXX_reg_write(ARMCAXPLL3_CON0, PLL_CON0_VALUE);

	temp = clk_readl(UNIVPLL_CON0);
	clk_writel(UNIVPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(MFGPLL_CON0);
	clk_writel(MFGPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(MSDCPLL_CON0);
	clk_writel(MSDCPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(VDECPLL_CON0);
	clk_writel(VDECPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(CODECPLL_CON0);
	clk_writel(CODECPLL_CON0, PLL_CON0_VALUE);

	temp = clk_readl(IMGPLL_CON0);
	clk_writel(IMGPLL_CON0, PLL_CON0_VALUE);

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

static int armbpll_fsel_read(struct seq_file *m, void *v)
{
	return 0;
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

static int mfgpll_fsel_read(struct seq_file *m, void *v)
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

	for (i = 0; i < NR_PLLS; i++) {
		if (i == 6)/*skip mpll*/
			continue;
		if (pll_is_on(i))
			p += sprintf(p, "suspend warning: %s is on!!!\n", plls[i].name);
	}
	for (i = 0; i < NR_SYSS; i++) {
		if (subsys_is_on(i)) {
			if ((i != SYS_MD1) && (i != SYS_C2K))
				p += sprintf(p, "suspend warning: %s is on!!!\n", syss[i].name);
			else
				p += sprintf(p, "%s: on\n", syss[i].name);
		}
	}

	p += sprintf(p, "********** PLL register dump *********\n");

	p += sprintf(p, "[MAINPLL_CON0]=0x%08x\n", clk_readl(MAINPLL_CON0));
	p += sprintf(p, "[MAINPLL_CON1]=0x%08x\n", clk_readl(MAINPLL_CON1));
	p += sprintf(p, "[MAINPLL_PWR_CON0]=0x%08x\n", clk_readl(MAINPLL_PWR_CON0));

	p += sprintf(p, "[UNIVPLL_CON0]=0x%08x\n", clk_readl(UNIVPLL_CON0));
	p += sprintf(p, "[UNIVPLL_CON1]=0x%08x\n", clk_readl(UNIVPLL_CON1));
	p += sprintf(p, "[UNIVPLL_PWR_CON0]=0x%08x\n", clk_readl(UNIVPLL_PWR_CON0));

	p += sprintf(p, "[MFGPLL_CON0]=0x%08x\n", clk_readl(MFGPLL_CON0));
	p += sprintf(p, "[MFGPLL_CON1]=0x%08x\n", clk_readl(MFGPLL_CON1));
	p += sprintf(p, "[MFGPLL_PWR_CON0]=0x%08x\n", clk_readl(MFGPLL_PWR_CON0));

	p += sprintf(p, "[MSDCPLL_CON0]=0x%08x\n", clk_readl(MSDCPLL_CON0));
	p += sprintf(p, "[MSDCPLL_CON1]=0x%08x\n", clk_readl(MSDCPLL_CON1));
	p += sprintf(p, "[MSDCPLL_PWR_CON0]=0x%08x\n", clk_readl(MSDCPLL_PWR_CON0));

	p += sprintf(p, "[IMGPLL_CON0]=0x%08x\n", clk_readl(IMGPLL_CON0));
	p += sprintf(p, "[IMGPLL_CON1]=0x%08x\n", clk_readl(IMGPLL_CON1));
	p += sprintf(p, "[IMGPLL_PWR_CON0]=0x%08x\n", clk_readl(IMGPLL_PWR_CON0));

	p += sprintf(p, "[CODECPLL_CON0]=0x%08x\n", clk_readl(CODECPLL_CON0));
	p += sprintf(p, "[CODECPLL_CON1]=0x%08x\n", clk_readl(CODECPLL_CON1));
	p += sprintf(p, "[CODECPLL_PWR_CON0]=0x%08x\n", clk_readl(CODECPLL_PWR_CON0));

	p += sprintf(p, "[VDECPLL_CON0]=0x%08x\n", clk_readl(VDECPLL_CON0));
	p += sprintf(p, "[VDECPLL_CON1]=0x%08x\n", clk_readl(VDECPLL_CON1));
	p += sprintf(p, "[VDECPLL_PWR_CON0]=0x%08x\n", clk_readl(VDECPLL_PWR_CON0));

	p += sprintf(p, "[TVDPLL_CON0]=0x%08x\n", clk_readl(TVDPLL_CON0));
	p += sprintf(p, "[TVDPLL_CON1]=0x%08x\n", clk_readl(TVDPLL_CON1));
	p += sprintf(p, "[TVDPLL_PWR_CON0]=0x%08x\n", clk_readl(TVDPLL_PWR_CON0));

	p += sprintf(p, "[APLL1_CON0]=0x%08x\n", clk_readl(APLL1_CON0));
	p += sprintf(p, "[APLL1_CON1]=0x%08x\n", clk_readl(APLL1_CON1));
	p += sprintf(p, "[APLL1_PWR_CON0]=0x%08x\n", clk_readl(APLL1_PWR_CON0));

	p += sprintf(p, "[APLL2_CON0]=0x%08x\n", clk_readl(APLL2_CON0));
	p += sprintf(p, "[APLL2_CON1]=0x%08x\n", clk_readl(APLL2_CON1));
	p += sprintf(p, "[APLL2_PWR_CON0]=0x%08x\n", clk_readl(APLL2_PWR_CON0));

	p += sprintf(p, "[AP_PLL_CON3]=0x%08x\n", clk_readl(AP_PLL_CON3));
	p += sprintf(p, "[AP_PLL_CON4]=0x%08x\n", clk_readl(AP_PLL_CON4));

	p += sprintf(p, "********** subsys pwr sts register dump *********\n");
	p += sprintf(p, "PWR_STATUS=0x%08x, PWR_STATUS_2ND=0x%08x\n",
		   clk_readl(PWR_STATUS), clk_readl(PWR_STATUS_2ND));

	p += sprintf(p, "********** mux register dump *********\n");

	p += sprintf(p, "[CLK_CFG_0]=0x%08x\n", clk_readl(CLK_CFG_0));
	p += sprintf(p, "[CLK_CFG_1]=0x%08x\n", clk_readl(CLK_CFG_1));
	p += sprintf(p, "[CLK_CFG_2]=0x%08x\n", clk_readl(CLK_CFG_2));
	p += sprintf(p, "[CLK_CFG_3]=0x%08x\n", clk_readl(CLK_CFG_3));
	p += sprintf(p, "[CLK_CFG_4]=0x%08x\n", clk_readl(CLK_CFG_4));
	p += sprintf(p, "[CLK_CFG_5]=0x%08x\n", clk_readl(CLK_CFG_5));
	p += sprintf(p, "[CLK_CFG_6]=0x%08x\n", clk_readl(CLK_CFG_6));
	p += sprintf(p, "[CLK_CFG_7]=0x%08x\n", clk_readl(CLK_CFG_7));
	p += sprintf(p, "[CLK_CFG_8]=0x%08x\n", clk_readl(CLK_CFG_8));

	p += sprintf(p, "********** CG register dump *********\n");
	p += sprintf(p, "[INFRA_SUBSYS][%p]\n", clk_infracfg_ao_base);	/* 0x10001000 */

	p += sprintf(p, "[INFRA_PDN_STA0]=0x%08x\n", clk_readl(INFRA_SW_CG0_STA));
	p += sprintf(p, "[INFRA_PDN_STA1]=0x%08x\n", clk_readl(INFRA_SW_CG1_STA));
	p += sprintf(p, "[INFRA_PDN_STA2]=0x%08x\n", clk_readl(INFRA_SW_CG2_STA));

	p += sprintf(p, "[INFRA_TOPAXI_PROTECTSTA1]=0x%08x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA1));

	p += sprintf(p, "[MMSYS_SUBSYS][%p]\n", clk_mmsys_config_base);
	p += sprintf(p, "[DISP_CG_CON0]=0x%08x\n", clk_readl(DISP_CG_CON0));
	p += sprintf(p, "[DISP_CG_CON1]=0x%08x\n", clk_readl(DISP_CG_CON1));

	/**/
	p += sprintf(p, "[MFG_SUBSYS][%p]\n", clk_mfgcfg_base);
	p += sprintf(p, "[MFG_CG_CON]=0x%08x\n", clk_readl(MFG_CG_CON));

	p += sprintf(p, "[AUDIO_SUBSYS][%p]\n", clk_audio_base);
	p += sprintf(p, "[AUDIO_TOP_CON0]=0x%08x\n", clk_readl(AUDIO_TOP_CON0));
	p += sprintf(p, "[AUDIO_TOP_CON1]=0x%08x\n", clk_readl(AUDIO_TOP_CON1));

	p += sprintf(p, "[IMG_SUBSYS][%p]\n", clk_imgsys_base);
	p += sprintf(p, "[IMG_CG_CON]=0x%08x\n", clk_readl(IMG_CG_CON));

	p += sprintf(p, "[VDEC_SUBSYS][%p]\n", clk_vdec_gcon_base);
	p += sprintf(p, "[VDEC_CKEN_SET]=0x%08x\n", clk_readl(VDEC_CKEN_SET));
	p += sprintf(p, "[LARB_CKEN_SET]=0x%08x\n", clk_readl(LARB_CKEN_SET));

	p += sprintf(p, "[VENC_SUBSYS][%p]\n", clk_venc_gcon_base);
	p += sprintf(p, "[VENC_CG_CON]=0x%08x\n", clk_readl(VENC_CG_CON));

	p += sprintf(p, "[CAM_SUBSYS][%p]\n", clk_camsys_base);
	p += sprintf(p, "[CAM_CG_CON]=0x%08x\n", clk_readl(CAM_CG_CON));

	p += sprintf(p, "[MJC_SUBSYS][%p]\n", clk_mjc_config_base);
	p += sprintf(p, "[MJC_CG_CON]=0x%08x\n", clk_readl(MJC_CG_CON));

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);

}

static ssize_t armbpll_fsel_write(struct file *file, const char __user *buffer,
				  size_t count, loff_t *data)
{

	char desc[32];
	int len = 0;
	/*unsigned int ctrl_value = 0; */ /*enable when idvfs ready */
	unsigned int value;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 10, &value) == 0) {
		if (value < 500 || value > 3000) {
			pr_err("value invalid!\n");
			return 0;
		}
		/*SEC_BIGIDVFSPLLSETFREQ(value); */ /*enable when idvfs ready */
	}
	return 0;
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

	if (sscanf(desc, "%d %x", &div, &value) == 2) {
		ctrl_value = mt6797_0x1001AXXX_reg_read(ARMCAXPLL1_CON1);
		ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= SDM_PLL_N_INFO_CHG;
		mt6797_0x1001AXXX_reg_write(ARMCAXPLL1_CON1, ctrl_value);
		udelay(20);
		if (div == 4)
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_L_DIV_MASK_) |
				   _ARMPLL_DIV_4_ << _ARMPLL_L_DIV_BIT_);
		else if (div == 2)
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_L_DIV_MASK_) |
				   _ARMPLL_DIV_2_ << _ARMPLL_L_DIV_BIT_);
		else		/*1 */
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_L_DIV_MASK_) |
				   _ARMPLL_DIV_1_ << _ARMPLL_L_DIV_BIT_);
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

	if (sscanf(desc, "%d %x", &div, &value) == 2) {
		ctrl_value = mt6797_0x1001AXXX_reg_read(ARMCAXPLL0_CON1);
		ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= SDM_PLL_N_INFO_CHG;
		mt6797_0x1001AXXX_reg_write(ARMCAXPLL0_CON1, ctrl_value);
		udelay(20);
		if (div == 4)
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_LL_DIV_MASK_) |
				   _ARMPLL_DIV_4_ << _ARMPLL_LL_DIV_BIT_);
		else if (div == 2)
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_LL_DIV_MASK_) |
				   _ARMPLL_DIV_2_ << _ARMPLL_LL_DIV_BIT_);
		else		/*1 */
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_LL_DIV_MASK_) |
				   _ARMPLL_DIV_1_ << _ARMPLL_LL_DIV_BIT_);
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

	if (sscanf(desc, "%d %x", &div, &value) == 2) {
		ctrl_value = mt6797_0x1001AXXX_reg_read(ARMCAXPLL2_CON1);
		ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
		ctrl_value |= SDM_PLL_N_INFO_CHG;
		mt6797_0x1001AXXX_reg_write(ARMCAXPLL2_CON1, ctrl_value);
		udelay(20);
		if (div == 4)
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_CCI_DIV_MASK_) |
				   _ARMPLL_DIV_4_ << _ARMPLL_CCI_DIV_BIT_);
		else if (div == 2)
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_CCI_DIV_MASK_) |
				   _ARMPLL_DIV_2_ << _ARMPLL_CCI_DIV_BIT_);
		else		/*1 */
			mt6797_0x1001AXXX_reg_write(ARMPLLDIV_CKDIV,
				   (mt6797_0x1001AXXX_reg_read(ARMPLLDIV_CKDIV) & _ARMPLL_CCI_DIV_MASK_) |
				   _ARMPLL_DIV_1_ << _ARMPLL_CCI_DIV_BIT_);
	}
	return count;

}


static ssize_t mfgpll_fsel_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int con0, con1;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%x %x", &con0, &con1) == 2) {
		clk_writel(MFGPLL_CON1, con1);
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
		if (i == 6)/*skip mpll*/
			continue;
		if (pll_is_on(i)) {
			slp_chk_mtcmos_pll_stat = -1;
			/*pr_err("%s: on\n", plls[i].name);*/
			pr_err("suspend warning: %s is on!!!\n", plls[i].name);
			/*pr_err("warning! warning! warning! it may cause resume fail\n");*/
		}
	}
	for (i = 0; i < NR_SYSS; i++) {
		if (subsys_is_on(i)) {
			if ((i != SYS_MD1) && (i != SYS_C2K)) {
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

static int proc_armbpll_fsel_open(struct inode *inode, struct file *file)
{
	return single_open(file, armbpll_fsel_read, NULL);
}

static const struct file_operations armbpll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_armbpll_fsel_open,
	.read = seq_read,
	.write = armbpll_fsel_write,
	.release = single_release,
};

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

static int proc_mfgpll_fsel_open(struct inode *inode, struct file *file)
{
	clk_err("%s", __func__);
	return single_open(file, mfgpll_fsel_read, NULL);
}

static const struct file_operations mfgpll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_mfgpll_fsel_open,
	.read = seq_read,
	.write = mfgpll_fsel_write,
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
	    proc_create("armbpll_fsel", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&armbpll_fsel_proc_fops);
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
	    proc_create("mfgpll_fsel", S_IRUGO | S_IWUSR | S_IWGRP, clkmgr_dir,
			&mfgpll_fsel_proc_fops);
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
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6797-vdec_gcon");
	if (!node)
		clk_dbg("[CLK_VDEC_GCON] find node failed\n");
	clk_vdec_gcon_base = of_iomap(node, 0);
	if (!clk_vdec_gcon_base)
		clk_dbg("[CLK_VDEC_GCON] base failed\n");
/*venc_gcon*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6797-venc_gcon");
	if (!node)
		clk_dbg("[CLK_VENC_GCON] find node failed\n");
	clk_venc_gcon_base = of_iomap(node, 0);
	if (!clk_venc_gcon_base)
		clk_dbg("[CLK_VENC_GCON] base failed\n");

/*mcumixed*/
#if 0
	node = of_find_compatible_node(NULL, NULL, "mediatek,mcumixed");
	if (!node)
		clk_dbg("[CLK_MCUMIXED] find node failed\n");
	clk_mcumixed_base = of_iomap(node, 0);
	if (!clk_mcumixed_base)
		clk_dbg("[CLK_MCUMIXED] base failed\n");
#endif
/*cam*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,camsys_config");
	if (!node)
		clk_dbg("[CLK_CAM] find node failed\n");
	clk_camsys_base = of_iomap(node, 0);
	if (!clk_camsys_base)
		clk_dbg("[CLK_CAM] base failed\n");

/*mjc*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,mjc_config-v1");
	if (!node)
		clk_dbg("[CLK_MJC] find node failed\n");
	clk_mjc_config_base = of_iomap(node, 0);
	if (!clk_mjc_config_base)
		clk_dbg("[CLK_MJC] base failed\n");

/*topmics*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,topmisc");
	if (!node)
		clk_dbg("[CLK_TOPMISC] find node failed\n");
	clk_topmics_base = of_iomap(node, 0);
	if (!clk_topmics_base)
		clk_dbg("[CLK_TOPMISC] base failed\n");

}
#endif

#define FMETER_EN_BIT (1<<12)
void mt_disable_dbg_clk(void)
{
	clk_writel(CLK26CALI_0, clk_readl(CLK26CALI_0) & ~FMETER_EN_BIT);
	clk_writel(CLK_MISC_CFG_0, clk_readl(CLK_MISC_CFG_0) | 0xFFFF0000);
	clk_writel(CLK_MISC_CFG_1, clk_readl(CLK_MISC_CFG_1) | 0xFFFF0000);

	mt6797_0x1001AXXX_reg_write(ARMPLLDIV_ARM_K1, 0xFFFFFFFF);
	mt6797_0x1001AXXX_reg_write(ARMPLLDIV_MON_EN, 0);

	clk_writel(INFRA_AO_DBG_CON0, 1);
	clk_writel(INFRA_AO_DBG_CON1, 1);
	clk_writel(INFRA_MODULE_CLK_SEL, 0x00004c70);
	clk_writel(TOPMISC_TEST_MODE_CFG, 0x030c0000);
}

void mt_clkmgr_init(void)
{
	iomap();
	BUG_ON(initialized);
	mt_plls_init();
	mt_subsys_init();
	mt_disable_dbg_clk();
}

static int mt_clkmgr_debug_module_init(void)
{
	mt_clkmgr_debug_init();
	return 0;
}

module_init(mt_clkmgr_debug_module_init);
