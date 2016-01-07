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
#include <mach/mt_clkmgr.h>
/* #include <mach/mt_dcm.h> */
#include <mach/mt_spm_mtcmos.h>
/* #include <mach/mt_spm_sleep.h> */
/* #include <mach/mt_freqhopping.h> */
/* #include <mach/mt_gpufreq.h> */
/* #include <mach/irqs.h> */

/* #include <mach/upmu_common.h> */
/* #include <mach/upmu_sw.h> */
/* #include <mach/upmu_hw.h> */
#include "mt_spm.h"

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#ifdef CONFIG_OF
void __iomem *clk_apmixed_base;
void __iomem *clk_cksys_base;
void __iomem *clk_infracfg_ao_base;
void __iomem *clk_pericfg_base;
void __iomem *clk_audio_base;
void __iomem *clk_mfgcfg_base;
void __iomem *clk_mmsys_config_base;
void __iomem *clk_imgsys_base;
void __iomem *clk_vdec_gcon_base;
void __iomem *clk_venc_gcon_base;
#endif

/* #define CLK_LOG_TOP */
/* #define CLK_LOG */
/* #define DISP_CLK_LOG */
/* #define SYS_LOG */
/* #define MUX_LOG_TOP */
#define MUX_LOG
/* #define PLL_LOG_TOP */
#define PLL_LOG

/* **** */
#define Bring_Up

#define VLTE_SUPPORT

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

/* #define CONFIG_CLKMGR_STAT */

struct pll;
struct pll_ops {
	int (*get_state)(struct pll *pll);
	/* void (*change_mode)(int mode); */
	void (*enable)(struct pll *pll);
	void (*disable)(struct pll *pll);
	void (*fsel)(struct pll *pll, unsigned int value);
	int (*dump_regs)(struct pll *pll, unsigned int *ptr);
	/* unsigned int (*vco_calc)(struct pll *pll); */
	int (*hp_enable)(struct pll *pll);
	int (*hp_disable)(struct pll *pll);
};

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
	struct pll_ops *ops;
	unsigned int hp_id;
	int hp_switch;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};


struct subsys;
struct subsys_ops {
	int (*enable)(struct subsys *sys);
	int (*disable)(struct subsys *sys);
	int (*get_state)(struct subsys *sys);
	int (*dump_regs)(struct subsys *sys, unsigned int *ptr);
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
	/* int (*pwr_ctrl)(int state); */
	struct subsys_ops *ops;
	struct cg_grp *start;
	unsigned int nr_grps;
	struct clkmux *mux;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};


struct clkmux;
struct clkmux_ops {
	void (*sel)(struct clkmux *mux, unsigned int clksrc);
	void (*enable)(struct clkmux *mux);
	void (*disable)(struct clkmux *mux);
};

struct clkmux {
	const char *name;
	unsigned int cnt;
	void __iomem *base_addr;
	unsigned int sel_mask;
	unsigned int pdn_mask;
	unsigned int offset;
	unsigned int nr_inputs;
/* unsigned int upd_mask; */
	struct clkmux_ops *ops;
/* struct clkmux *parent; */
	struct clkmux *siblings;
	struct pll *pll;
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};


struct cg_grp;
struct cg_grp_ops {
	int (*prepare)(struct cg_grp *grp);
	int (*finished)(struct cg_grp *grp);
	unsigned int (*get_state)(struct cg_grp *grp);
	int (*dump_regs)(struct cg_grp *grp, unsigned int *ptr);
};

struct cg_grp {
	const char *name;
	void __iomem *set_addr;
	void __iomem *clr_addr;
	void __iomem *sta_addr;
	void __iomem *dummy_addr;
	void __iomem *bw_limit_addr;
	unsigned int mask;
	unsigned int state;
	struct cg_grp_ops *ops;
	struct subsys *sys;
};


struct cg_clk;
struct cg_clk_ops {
	int (*get_state)(struct cg_clk *clk);
	int (*check_validity)(struct cg_clk *clk);	/* 1: valid, 0: invalid */
	int (*enable)(struct cg_clk *clk);
	int (*disable)(struct cg_clk *clk);
};

struct cg_clk {
	int cnt;
	unsigned int state;
	unsigned int mask;
	int force_on;
	struct cg_clk_ops *ops;
	struct cg_grp *grp;
	struct clkmux *mux;
/* struct cg_clk *parent; */
#ifdef CONFIG_CLKMGR_STAT
	struct list_head head;
#endif
};


#ifdef CONFIG_CLKMGR_STAT
struct stat_node {
	struct list_head link;
	unsigned int cnt_on;
	unsigned int cnt_off;
	char name[0];
};
#endif



/************************************************
 **********      global variablies     **********
 ************************************************/

#define PWR_DOWN    0
#define PWR_ON      1

static int initialized;
/* static int es_flag = 0; */
static int slp_chk_mtcmos_pll_stat;

static struct pll plls[NR_PLLS];
static struct subsys syss[NR_SYSS];
static struct clkmux muxs[NR_MUXS];
static struct cg_grp grps[NR_GRPS];
static struct cg_clk clks[NR_CLKS];


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
 **********     clkmgr stat debug      **********
 ************************************************/

#ifdef CONFIG_CLKMGR_STAT
void update_stat_locked(struct list_head *head, char *name, int op)
{
	struct list_head *pos = NULL;
	struct stat_node *node = NULL;
	int len = strlen(name);
	int new_node = 1;

	list_for_each(pos, head) {
		node = list_entry(pos, struct stat_node, link);
		if (!strncmp(node->name, name, len)) {
			new_node = 0;
			break;
		}
	}

	if (new_node) {
		node = NULL;
		node = kzalloc(sizeof(*node) + len + 1, GFP_ATOMIC);
		if (!node) {
			clk_err("[%s]: malloc stat node for %s fail\n", __func__, name);
			goto node_error;
		} else {
			memcpy(node->name, name, len);
			list_add_tail(&node->link, head);
		}
	}

	if (op)
		node->cnt_on++;
	else
		node->cnt_off++;

node_error:
	return;
}
#endif



/************************************************
 **********    function declaration    **********
 ************************************************/

static int pll_enable_locked(struct pll *pll);
static int pll_disable_locked(struct pll *pll);

static int sys_enable_locked(struct subsys *sys);
static int sys_disable_locked(struct subsys *sys, int force_off);

static void mux_enable_locked(struct clkmux *mux);
static void mux_disable_locked(struct clkmux *mux);

static int clk_enable_locked(struct cg_clk *clk);
static int clk_disable_locked(struct cg_clk *clk);


static inline int pll_enable_internal(struct pll *pll, char *name)
{
	int err;

	err = pll_enable_locked(pll);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&pll->head, name, 1);
#endif
	return err;
}

static inline int pll_disable_internal(struct pll *pll, char *name)
{
	int err;

	err = pll_disable_locked(pll);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&pll->head, name, 0);
#endif
	return err;
}


static inline int subsys_enable_internal(struct subsys *sys, char *name)
{
	int err;

	err = sys_enable_locked(sys);
#ifdef CONFIG_CLKMGR_STAT
	/* update_stat_locked(&sys->head, name, 1); */
#endif
	return err;
}

static inline int subsys_disable_internal(struct subsys *sys, int force_off, char *name)
{
	int err;

	err = sys_disable_locked(sys, force_off);
#ifdef CONFIG_CLKMGR_STAT
	/* update_stat_locked(&sys->head, name, 0); */
#endif
	return err;
}


static inline void mux_enable_internal(struct clkmux *mux, char *name)
{
	mux_enable_locked(mux);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&mux->head, name, 1);
#endif
}

static inline void mux_disable_internal(struct clkmux *mux, char *name)
{
	mux_disable_locked(mux);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&mux->head, name, 0);
#endif
}


static inline int clk_enable_internal(struct cg_clk *clk, char *name)
{
	int err;

	err = clk_enable_locked(clk);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&clk->head, name, 1);
#endif
	return err;
}

static inline int clk_disable_internal(struct cg_clk *clk, char *name)
{
	int err;

	err = clk_disable_locked(clk);
#ifdef CONFIG_CLKMGR_STAT
	update_stat_locked(&clk->head, name, 0);
#endif
	return err;
}



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

static struct pll_ops arm_pll_ops;
static struct pll_ops sdm_pll_ops;

static struct pll plls[NR_PLLS] = {
	{
	 .name = __stringify(ARMPLL),
	 .type = PLL_TYPE_SDM,
	 .feat = HAVE_PLL_HP,
	 .en_mask = 0x00000001,
	/* .base_addr = ARMCA7PLL_CON0, */
	/* .pwr_addr = ARMCA7PLL_PWR_CON0, */
	 .ops = &arm_pll_ops,
	/* **** */
	/* .hp_id = FH_ARM_PLLID, */
	.hp_switch = 1,
	}, {
	.name = __stringify(MAINPLL),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP | HAVE_RST_BAR,
	.en_mask = 0xF0000101,
	/* .base_addr = MAINPLL_CON0, */
	/* .pwr_addr = MAINPLL_PWR_CON0, */
	.ops = &sdm_pll_ops,
	/* **** */
	/* .hp_id = FH_MAIN_PLLID, */
	.hp_switch = 1,
	}, {
	.name = __stringify(MSDCPLL),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP,
	.en_mask = 0x00000001,
	/* .base_addr = MSDCPLL_CON0, */
	/* .pwr_addr = MSDCPLL_PWR_CON0, */
	.ops = &sdm_pll_ops,
	/* **** */
	/* .hp_id = FH_MSDC_PLLID, */
	.hp_switch = 1,
	}, {
	 .name = __stringify(UNIVPLL),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_RST_BAR | HAVE_FIX_FRQ,
	.en_mask = 0xFC000001,
	/* .base_addr = UNIVPLL_CON0, */
	/* .pwr_addr = UNIVPLL_PWR_CON0, */
	.ops = &sdm_pll_ops,
	}, {
	.name = __stringify(MMPLL),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP,
	.en_mask = 0x00000001,
	/* .base_addr = MMPLL_CON0, */
	/* .pwr_addr = MMPLL_PWR_CON0, */
	.ops = &sdm_pll_ops,
	/* **** */
	/* .hp_id = FH_MM_PLLID, */
	.hp_switch = 1,
	}, {
	.name = __stringify(VENCPLL),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP,
	.en_mask = 0x00000001,
	/* .base_addr = VENCPLL_CON0, */
	/* .pwr_addr = VENCPLL_PWR_CON0, */
	.ops = &sdm_pll_ops,
	/* **** */
	/* .hp_id = FH_VENC_PLLID, */
	.hp_switch = 1,
	}, {
	.name = __stringify(TVDPLL),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP,
	.en_mask = 0x00000001,
	/* .base_addr = TVDPLL_CON0, */
	/* .pwr_addr = TVDPLL_PWR_CON0, */
	.ops = &sdm_pll_ops,
	/* **** */
	/* .hp_id = FH_TVD_PLLID, */
	.hp_switch = 1,
	}, /* {
	.name = __stringify(MPLL),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP,
	.en_mask = 0x00000001,
	.base_addr = MPLL_CON0,
	.pwr_addr = MPLL_PWR_CON0,
	.ops = &sdm_pll_ops,
	.hp_id = FH_M_PLLID,
	.hp_switch = 1,
	}, */ {
	.name = __stringify(APLL1),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP,
	.en_mask = 0x00000001,
	/* .base_addr = APLL1_CON0, */
	/* .pwr_addr = APLL1_PWR_CON0, */
	.ops = &sdm_pll_ops,
	}, {
	.name = __stringify(APLL2),
	.type = PLL_TYPE_SDM,
	.feat = HAVE_PLL_HP,
	.en_mask = 0x00000001,
	/* .base_addr = APLL2_CON0, */
	/* .pwr_addr = APLL2_PWR_CON0, */
	.ops = &sdm_pll_ops,
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
#define APLL_SDM_PLL_N_INFO_MASK 0x7fffffff
#define SDM_PLL_N_INFO_CHG  0x80000000
#define ARMPLL_POSDIV_MASK  0x07000000

static int pll_get_state_op(struct pll *pll)
{
	return clk_readl(pll->base_addr) & 0x1;
}

static void sdm_pll_enable_op(struct pll *pll)
{
#ifdef PLL_LOG
	/* clk_info("[%s]: pll->name=%s\n", __func__, pll->name); */
	clk_dbg("[%s]: pll->name=%s\n", __func__, pll->name);
#endif

	clk_setl(pll->pwr_addr, PLL_PWR_ON);
	udelay(2);
	clk_clrl(pll->pwr_addr, PLL_ISO_EN);

	clk_setl(pll->base_addr, pll->en_mask);
	udelay(20);

	if (pll->feat & HAVE_RST_BAR)
		clk_setl(pll->base_addr, RST_BAR_MASK);
}

static void sdm_pll_disable_op(struct pll *pll)
{
#ifdef PLL_LOG
	/* clk_info("[%s]: pll->name=%s\n", __func__, pll->name); */
	clk_dbg("[%s]: pll->name=%s\n", __func__, pll->name);
#endif
/* if( pll->base_addr == UNIVPLL_CON0 || pll->base_addr == VENCPLL_CON0) */
/* { */
/* printk("univpll return\n"); */
/* return;//for debug */
/* } */
	if (pll->feat & HAVE_RST_BAR)
		clk_clrl(pll->base_addr, RST_BAR_MASK);

	clk_clrl(pll->base_addr, 0x1);

	clk_setl(pll->pwr_addr, PLL_ISO_EN);
	clk_clrl(pll->pwr_addr, PLL_PWR_ON);
}

static void sdm_pll_fsel_op(struct pll *pll, unsigned int value)
{
	unsigned int ctrl_value;

	ctrl_value = clk_readl(pll->base_addr + 4);
	if (pll->base_addr == UNIVPLL_CON0) {
		ctrl_value &= ~UNIV_SDM_PLL_N_INFO_MASK;
		ctrl_value |= value & UNIV_SDM_PLL_N_INFO_MASK;
	} else if ((pll->base_addr == APLL1_CON0) || (pll->base_addr == APLL2_CON0)) {
		ctrl_value &= ~APLL_SDM_PLL_N_INFO_MASK;
		ctrl_value |= value & APLL_SDM_PLL_N_INFO_MASK;
	} else {
		ctrl_value &= ~SDM_PLL_N_INFO_MASK;
		ctrl_value |= value & SDM_PLL_N_INFO_MASK;
	}

	ctrl_value |= SDM_PLL_N_INFO_CHG;

	clk_writel(pll->base_addr + 4, ctrl_value);
	udelay(20);
}

static int sdm_pll_dump_regs_op(struct pll *pll, unsigned int *ptr)
{
	*(ptr) = clk_readl(pll->base_addr);
	*(++ptr) = clk_readl(pll->base_addr + 4);
	*(++ptr) = clk_readl(pll->pwr_addr);

	return 3;
}

static int sdm_pll_hp_enable_op(struct pll *pll)
{
	int err;

	if (!pll->hp_switch || (pll->state == PWR_DOWN))
		return 0;

#ifndef Bring_Up
	err = freqhopping_config(pll->hp_id, 0, PWR_ON);
#endif

	return err;
}

static int sdm_pll_hp_disable_op(struct pll *pll)
{
	int err;

	if (!pll->hp_switch || (pll->state == PWR_ON))
		return 0;

#ifndef Bring_Up
	err = freqhopping_config(pll->hp_id, 0, PWR_DOWN);
#endif

	return err;
}

static struct pll_ops sdm_pll_ops = {
	.get_state = pll_get_state_op,
	.enable = sdm_pll_enable_op,
	.disable = sdm_pll_disable_op,
	.fsel = sdm_pll_fsel_op,
	.dump_regs = sdm_pll_dump_regs_op,
	.hp_enable = sdm_pll_hp_enable_op,
	.hp_disable = sdm_pll_hp_disable_op,
};


static void arm_pll_fsel_op(struct pll *pll, unsigned int value)
{
	unsigned int ctrl_value;

	ctrl_value = clk_readl(pll->base_addr + 4);
	ctrl_value &= ~(SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
	ctrl_value |= value & (SDM_PLL_N_INFO_MASK | ARMPLL_POSDIV_MASK);
	ctrl_value |= SDM_PLL_N_INFO_CHG;

	clk_writel(pll->base_addr + 4, ctrl_value);
	udelay(20);
}

static struct pll_ops arm_pll_ops = {
	.get_state = pll_get_state_op,
	.enable = sdm_pll_enable_op,
	.disable = sdm_pll_disable_op,
	.fsel = arm_pll_fsel_op,
	.dump_regs = sdm_pll_dump_regs_op,
	.hp_enable = sdm_pll_hp_enable_op,
	.hp_disable = sdm_pll_hp_disable_op,
};

static int get_pll_state_locked(struct pll *pll)
{
	if (likely(initialized))
		return pll->state;
	else
		return pll->ops->get_state(pll);
}

static int pll_enable_locked(struct pll *pll)
{
	pll->cnt++;

#ifdef PLL_LOG_TOP
	clk_info("[%s]: Start. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif

	if (pll->cnt > 1)
		return 0;

	if (pll->state == PWR_DOWN) {
		pll->ops->enable(pll);
		pll->state = PWR_ON;
	}

	if (pll->ops->hp_enable)
		pll->ops->hp_enable(pll);

#ifdef PLL_LOG_TOP
	clk_info("[%s]: End. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif
	return 0;
}

static int pll_disable_locked(struct pll *pll)
{
#ifdef PLL_LOG_TOP
	clk_info("[%s]: Start. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif

	BUG_ON(!pll->cnt);
	pll->cnt--;

#ifdef PLL_LOG_TOP
	clk_info("[%s]: Start. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif

	if (pll->cnt > 0)
		return 0;

	if (pll->state == PWR_ON) {
		pll->ops->disable(pll);
		pll->state = PWR_DOWN;
	}

	if (pll->ops->hp_disable)
		pll->ops->hp_disable(pll);

#ifdef PLL_LOG_TOP
	clk_info("[%s]: End. pll->name=%s, pll->cnt=%d, pll->state=%d\n", __func__, pll->name,
		 pll->cnt, pll->state);
#endif
	return 0;
}


static int pll_fsel_locked(struct pll *pll, unsigned int value)
{
	pll->ops->fsel(pll, value);
	if (pll->ops->hp_enable)
		pll->ops->hp_enable(pll);

	return 0;
}

int pll_is_on(int id)
{
	int state;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 1;
#endif

	BUG_ON(!pll);

	clkmgr_lock(flags);
	state = get_pll_state_locked(pll);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(pll_is_on);

int enable_pll(int id, char *name)
{
	int err;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

#ifndef PLL_CLK_LINK
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);
	BUG_ON(!name);
#ifdef PLL_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
#endif
	clkmgr_lock(flags);
	err = pll_enable_internal(pll, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(enable_pll);

int disable_pll(int id, char *name)
{
	int err;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

#ifndef PLL_CLK_LINK
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);
	BUG_ON(!name);
#ifdef PLL_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
#endif
	clkmgr_lock(flags);
	err = pll_disable_internal(pll, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(disable_pll);

int pll_fsel(int id, unsigned int value)
{
	int err;
	unsigned long flags;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	clkmgr_lock(flags);
	err = pll_fsel_locked(pll, value);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(pll_fsel);


int pll_hp_switch_on(int id, int hp_on)
{
	int err = 0;
	unsigned long flags;
	int old_value;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	if (pll->type != PLL_TYPE_SDM) {
		err = -EINVAL;
		goto out;
	}

	clkmgr_lock(flags);
	old_value = pll->hp_switch;
	if (old_value == 0) {
		pll->hp_switch = 1;
		if (hp_on)
			err = pll->ops->hp_enable(pll);
	}
	clkmgr_unlock(flags);

#if 0
	clk_info("[%s]hp_switch(%d->%d), hp_on=%d\n", __func__, old_value, pll->hp_switch, hp_on);
#endif

out:
	return err;
}
EXPORT_SYMBOL(pll_hp_switch_on);

int pll_hp_switch_off(int id, int hp_off)
{
	int err = 0;
	unsigned long flags;
	int old_value;
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	if (pll->type != PLL_TYPE_SDM) {
		err = -EINVAL;
		goto out;
	}

	clkmgr_lock(flags);
	old_value = pll->hp_switch;
	if (old_value == 1) {
		if (hp_off)
			err = pll->ops->hp_disable(pll);

		pll->hp_switch = 0;
	}
	clkmgr_unlock(flags);

#if 0
	clk_info("[%s]hp_switch(%d->%d), hp_off=%d\n", __func__, old_value, pll->hp_switch, hp_off);
#endif

out:
	return err;
}
EXPORT_SYMBOL(pll_hp_switch_off);


int pll_dump_regs(int id, unsigned int *ptr)
{
	struct pll *pll = id_to_pll(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!pll);

	return pll->ops->dump_regs(pll, ptr);
}
EXPORT_SYMBOL(pll_dump_regs);

const char *pll_get_name(int id)
{
	struct pll *pll = id_to_pll(id);

	BUG_ON(!initialized);
	BUG_ON(!pll);

	return pll->name;
}

void set_mipi26m(int en)
{
	unsigned long flags;

#ifdef Bring_Up
	return;
#endif

	clkmgr_lock(flags);

	if (en)
		clk_setl(AP_PLL_CON0, 1 << 6);
	else
		clk_clrl(AP_PLL_CON0, 1 << 6);

	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(set_mipi26m);

void set_ada_ssusb_xtal_ck(int en)
{
	unsigned long flags;

#ifdef Bring_Up
	return;
#endif

	clkmgr_lock(flags);

	if (en) {
		clk_setl(AP_PLL_CON2, 1 << 0);
		udelay(100);
		clk_setl(AP_PLL_CON2, 1 << 1);
		clk_setl(AP_PLL_CON2, 1 << 2);
	} else {
		clk_clrl(AP_PLL_CON2, 0x7);
	}
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(set_ada_ssusb_xtal_ck);


/************************************************
 **********         subsys part        **********
 ************************************************/

#define SYS_TYPE_MODEM    0
#define SYS_TYPE_MEDIA    1
#define SYS_TYPE_OTHER    2
#define SYS_TYPE_CONN     3

static struct subsys_ops md1_sys_ops;
static struct subsys_ops conn_sys_ops;
static struct subsys_ops dis_sys_ops;
static struct subsys_ops mfg_sys_ops;
static struct subsys_ops isp_sys_ops;
static struct subsys_ops vde_sys_ops;
/* static struct subsys_ops mjc_sys_ops; */
static struct subsys_ops ven_sys_ops;
/* static struct subsys_ops aud_sys_ops; */
static struct subsys_ops md2_sys_ops;

static struct subsys syss[NR_SYSS] = {
	{
	.name = __stringify(SYS_MD1),
	.type = SYS_TYPE_MODEM,
	.default_sta = PWR_DOWN,
	.sta_mask = 1U << 0,
	/* .ctl_addr = SPM_MD_PWR_CON, */
	.ops = &md1_sys_ops,
	}, {
	.name = __stringify(SYS_MD2),
	.type = SYS_TYPE_MODEM,
	.default_sta = PWR_DOWN,
	.sta_mask = 1U << 22,
	/* .ctl_addr = SPM_MD2_PWR_CON, */
	.ops = &md2_sys_ops,
	}, {
	.name = __stringify(SYS_CONN),
	.type = SYS_TYPE_CONN,
	.default_sta = PWR_DOWN,
	.sta_mask = 1U << 1,
	/* .ctl_addr = SPM_CONN_PWR_CON, */
	.ops = &conn_sys_ops,
	}, {
	.name = __stringify(SYS_DIS),
	.type = SYS_TYPE_MEDIA,
	.default_sta = PWR_ON,
	.sta_mask = 1U << 3,
	/* .ctl_addr = SPM_DIS_PWR_CON, */
	.ops = &dis_sys_ops,
	.start = &grps[CG_DISP0],
	.nr_grps = 2,
	.mux = &muxs[MT_MUX_MM],
	}, {
	.name = __stringify(SYS_MFG),
	.type = SYS_TYPE_MEDIA,
	.default_sta = PWR_ON,
	.sta_mask = 1U << 4,
	/* .ctl_addr = SPM_MFG_PWR_CON, */
	.ops = &mfg_sys_ops,
	.start = &grps[CG_MFG],
	.nr_grps = 1,
	.mux = &muxs[MT_MUX_MFG],
	}, {
	.name = __stringify(SYS_ISP),
	.type = SYS_TYPE_MEDIA,
	.default_sta = PWR_ON,
	.sta_mask = 1U << 5,
	/* .ctl_addr = SPM_ISP_PWR_CON, */
	.ops = &isp_sys_ops,
	.start = &grps[CG_IMAGE],
	.nr_grps = 1,
	/* .mux = &muxs[MT_MUX_SCAM], */
	}, {
	.name = __stringify(SYS_VDE),
	.type = SYS_TYPE_MEDIA,
	.default_sta = PWR_ON,
	.sta_mask = 1U << 7,
	/* .ctl_addr = SPM_VDE_PWR_CON, */
	.ops = &vde_sys_ops,
	.start = &grps[CG_VDEC0],
	.nr_grps = 2,
	.mux = &muxs[MT_MUX_VDEC],
	}, /*{
	.name = __stringify(SYS_MJC),
	.type = SYS_TYPE_MEDIA,
	.default_sta = PWR_ON,
	.sta_mask = 1U << 20,
	// .ctl_addr = SPM_MJC_PWR_CON,
	.ops = &mjc_sys_ops,
	.start = &grps[CG_MJC],
	.nr_grps = 1,
	.mux = &muxs[MT_MUX_MJC],
	}, */ {
	.name = __stringify(SYS_VEN),
	.type = SYS_TYPE_MEDIA,
	.default_sta = PWR_ON,
	.sta_mask = 1U << 8,
	/* .ctl_addr = SPM_VEN_PWR_CON, */
	.ops = &ven_sys_ops,
	.start = &grps[CG_VENC],
	.nr_grps = 1,
	/* .mux = &muxs[MT_MUX_VENC], */
	} /*, {
	.name = __stringify(SYS_AUD),
	.type = SYS_TYPE_MEDIA,
	.default_sta = PWR_ON,
	.sta_mask = 1U << 24,
	// .ctl_addr = SPM_AUDIO_PWR_CON,
	.ops = &aud_sys_ops,
	.start = &grps[CG_AUDIO],
	.nr_grps = 1,
	.mux = &muxs[MT_MUX_AUDINTBUS],
	} */
};


static void larb_backup(int larb_idx);
static void larb_restore(int larb_idx);


static struct subsys *id_to_sys(unsigned int id)
{
	return id < NR_SYSS ? syss + id : NULL;
}

static int md1_sys_enable_op(struct subsys *sys)
{
	int err;

	err = spm_mtcmos_ctrl_mdsys1(STA_POWER_ON);
	return err;
}

static int md1_sys_disable_op(struct subsys *sys)
{
	int err;

	err = spm_mtcmos_ctrl_mdsys1(STA_POWER_DOWN);
	return err;
}

static int md2_sys_enable_op(struct subsys *sys)
{
	int err;

	err = spm_mtcmos_ctrl_mdsys2(STA_POWER_ON);
	return err;
}

static int md2_sys_disable_op(struct subsys *sys)
{
	int err;

	err = spm_mtcmos_ctrl_mdsys2(STA_POWER_DOWN);
	return err;
}

static int conn_sys_enable_op(struct subsys *sys)
{
	int err;

	err = spm_mtcmos_ctrl_connsys(STA_POWER_ON);
	return err;
}

static int conn_sys_disable_op(struct subsys *sys)
{
	int err;

	err = spm_mtcmos_ctrl_connsys(STA_POWER_DOWN);
	return err;
}

static int dis_sys_enable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_disp(STA_POWER_ON);
	clk_writel(MMSYS_DUMMY, 0xFFFFFFFF);
	larb_restore(MT_LARB_DISP);
	return err;
}

static int dis_sys_disable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_DISP);
	err = spm_mtcmos_ctrl_disp(STA_POWER_DOWN);
	return err;
}

static int mfg_sys_enable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

/* mt_gpufreq_voltage_enable_set(1); */

	/* return 0;//for debug */
/* err = spm_mtcmos_ctrl_mfg_ASYNC(STA_POWER_ON); */
	err = spm_mtcmos_ctrl_mfg(STA_POWER_ON);
	return err;
}

static int mfg_sys_disable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	/* return 0;//for debug */
	err = spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);
/* err = spm_mtcmos_ctrl_mfg_ASYNC(STA_POWER_DOWN); */

/* mt_gpufreq_voltage_enable_set(0); */
	return err;
}

static int isp_sys_enable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_isp(STA_POWER_ON);
	larb_restore(MT_LARB_IMG);
	return err;
}

static int isp_sys_disable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_IMG);
	err = spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
	return err;
}

static int vde_sys_enable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_vdec(STA_POWER_ON);
	larb_restore(MT_LARB_VDEC);
	return err;
}

static int vde_sys_disable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_VDEC);
	err = spm_mtcmos_ctrl_vdec(STA_POWER_DOWN);
	return err;
}

/*
static int mjc_sys_enable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_mjc(STA_POWER_ON);
	larb_restore(MT_LARB_MJC);
	return err;
}

static int mjc_sys_disable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_MJC);
	err = spm_mtcmos_ctrl_mjc(STA_POWER_DOWN);
	return err;
}
*/
static int ven_sys_enable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_venc(STA_POWER_ON);
	larb_restore(MT_LARB_VENC);
	return err;
}

static int ven_sys_disable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	larb_backup(MT_LARB_VENC);
	err = spm_mtcmos_ctrl_venc(STA_POWER_DOWN);
	return err;
}

/*
static int aud_sys_enable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_aud(STA_POWER_ON);
	return err;
}

static int aud_sys_disable_op(struct subsys *sys)
{
	int err;
#ifdef SYS_LOG
	clk_info("[%s]: sys->name=%s\n", __func__, sys->name);
#endif

	err = spm_mtcmos_ctrl_aud(STA_POWER_DOWN);
	return err;
}
*/

static int sys_get_state_op(struct subsys *sys)
{

#ifndef CONFIG_FPGA_EARLY_PORTING
/* **** */
/* unsigned int sta = clk_readl(SPM_PWR_STATUS); */
/* unsigned int sta_s = clk_readl(SPM_PWR_STATUS_2ND); */

/* return (sta & sys->sta_mask) && (sta_s & sys->sta_mask); */
	return 0;
#else
	return 0;
#endif

}

static int sys_dump_regs_op(struct subsys *sys, unsigned int *ptr)
{
	*(ptr) = clk_readl(sys->ctl_addr);
	return 1;
}

static struct subsys_ops md1_sys_ops = {
	.enable = md1_sys_enable_op,
	.disable = md1_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops conn_sys_ops = {
	.enable = conn_sys_enable_op,
	.disable = conn_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops dis_sys_ops = {
	.enable = dis_sys_enable_op,
	.disable = dis_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops mfg_sys_ops = {
	.enable = mfg_sys_enable_op,
	.disable = mfg_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops isp_sys_ops = {
	.enable = isp_sys_enable_op,
	.disable = isp_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static struct subsys_ops vde_sys_ops = {
	.enable = vde_sys_enable_op,
	.disable = vde_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};
/*
static struct subsys_ops mjc_sys_ops = {
	.enable = mjc_sys_enable_op,
	.disable = mjc_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};
*/
static struct subsys_ops ven_sys_ops = {
	.enable = ven_sys_enable_op,
	.disable = ven_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};
/*
static struct subsys_ops aud_sys_ops = {
	.enable = aud_sys_enable_op,
	.disable = aud_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};
*/
static struct subsys_ops md2_sys_ops = {
	.enable = md2_sys_enable_op,
	.disable = md2_sys_disable_op,
	.get_state = sys_get_state_op,
	.dump_regs = sys_dump_regs_op,
};

static int get_sys_state_locked(struct subsys *sys)
{
	if (likely(initialized))
		return sys->state;
	else
		return sys->ops->get_state(sys);
}

int subsys_is_on(int id)
{
	int state;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 1;
#endif

	BUG_ON(!sys);

	clkmgr_lock(flags);
	state = get_sys_state_locked(sys);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(subsys_is_on);

/* #define STATE_CHECK_DEBUG */

static int sys_enable_locked(struct subsys *sys)
{
	int err;
	int local_state = sys->state;	/* get_subsys_local_state(sys); */

#ifdef STATE_CHECK_DEBUG
	int reg_state = sys->ops->get_state(sys);	/* get_subsys_reg_state(sys); */

	BUG_ON(local_state != reg_state);
#endif

#ifdef SYS_LOG
	clk_info("[%s]: Start. sys->name=%s, sys->state=%d\n", __func__, sys->name, sys->state);
#endif

	if (local_state == PWR_ON)
		return 0;

	if (sys->mux)
		mux_enable_internal(sys->mux, "sys");

	err = sys->ops->enable(sys);
	WARN_ON(err);

	if (!err)
		sys->state = PWR_ON;

#ifdef SYS_LOG
	clk_info("[%s]: End. sys->name=%s, sys->state=%d\n", __func__, sys->name, sys->state);
#endif
	return err;
}

static int sys_disable_locked(struct subsys *sys, int force_off)
{
	int err;
	int local_state = sys->state;	/* get_subsys_local_state(sys); */
	int i;
	struct cg_grp *grp;

#ifdef STATE_CHECK_DEBUG
	int reg_state = sys->ops->get_state(sys);	/* get_subsys_reg_state(sys); */

	BUG_ON(local_state != reg_state);
#endif

#ifdef SYS_LOG
	clk_info("[%s]: Start. sys->name=%s, sys->state=%d, force_off=%d\n", __func__, sys->name,
		 sys->state, force_off);
#endif
	if (!force_off) {
		/* could be power off or not */
		for (i = 0; i < sys->nr_grps; i++) {
			grp = sys->start + i;
			if (grp->state)
				return 0;
		}
	}

	if (local_state == PWR_DOWN)
		return 0;

	err = sys->ops->disable(sys);
	WARN_ON(err);

	if (!err)
		sys->state = PWR_DOWN;

	if (sys->mux)
		mux_disable_internal(sys->mux, "sys");

#ifdef SYS_LOG
	clk_info("[%s]: End. sys->name=%s, sys->state=%d, force_off=%d\n", __func__, sys->name,
		 sys->state, force_off);
#endif
	return err;
}

int enable_subsys(int id, char *name)
{
	int err;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);

	clkmgr_lock(flags);
	err = subsys_enable_internal(sys, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(enable_subsys);

int disable_subsys(int id, char *name)
{
	int err;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);

	clkmgr_lock(flags);
	err = subsys_disable_internal(sys, 0, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(disable_subsys);

int disable_subsys_force(int id, char *name)
{
	int err;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

	BUG_ON(!initialized);
	BUG_ON(!sys);

	clkmgr_lock(flags);
	err = subsys_disable_internal(sys, 1, name);
	clkmgr_unlock(flags);

	return err;
}

int subsys_dump_regs(int id, unsigned int *ptr)
{

	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);

	return sys->ops->dump_regs(sys, ptr);
}
EXPORT_SYMBOL(subsys_dump_regs);

const char *subsys_get_name(int id)
{
	struct subsys *sys = id_to_sys(id);

	BUG_ON(!initialized);
	BUG_ON(!sys);

	return sys->name;
}

#define JIFFIES_PER_LOOP 10

int md_power_on(int id)
{
	int err = 0;
	unsigned long flags;

	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
#if !defined(CONFIG_MTK_FPGA)
	if (id == SYS_MD1)
		spm_mtcmos_ctrl_mdsys1(STA_POWER_ON);
	else
		spm_mtcmos_ctrl_mdsys2(STA_POWER_ON);

	clk_info("[%s]: id = %d\n", __func__, id);
#endif
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);
	BUG_ON(sys->type != SYS_TYPE_MODEM);

	clkmgr_lock(flags);
	err = subsys_enable_internal(sys, "md");
/*
	if(id == 0)
	spm_mtcmos_ctrl_mdsys1(STA_POWER_ON);
	else
	spm_mtcmos_ctrl_mdsys2(STA_POWER_ON);
*/
	clkmgr_unlock(flags);

	clk_info("[%s]: id = %d\n", __func__, id);

	WARN_ON(err);

	return err;
}
EXPORT_SYMBOL(md_power_on);


#ifndef Bring_Up
static bool(*spm_md_sleep[])(void) = {
	spm_is_md1_sleep,
	spm_is_md2_sleep,
};
#endif

int md_power_off(int id, unsigned int timeout)
{
	int err = 0;
	int cnt;
	bool slept = 1;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

#ifdef Bring_Up
#if !defined(CONFIG_MTK_FPGA)
	if (id == SYS_MD1)
		spm_mtcmos_ctrl_mdsys1(STA_POWER_DOWN);
	else
		spm_mtcmos_ctrl_mdsys2(STA_POWER_DOWN);
#endif
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);
	BUG_ON(sys->type != SYS_TYPE_MODEM);

	/* 0: not sleep, 1: sleep */
#ifndef Bring_Up
	slept = spm_md_sleep[id] ();
#endif

	cnt = (timeout + JIFFIES_PER_LOOP - 1) / JIFFIES_PER_LOOP;

	while (!slept && cnt--) {
		msleep(MSEC_PER_SEC / JIFFIES_PER_LOOP);

#ifndef Bring_Up
		slept = spm_md_sleep[id] ();
#endif
		if (slept)
			break;
	}

	clkmgr_lock(flags);
	err = subsys_disable_internal(sys, 0, "md");
/*
	if(id == 0)
	spm_mtcmos_ctrl_mdsys1(STA_POWER_DOWN);
	else
	spm_mtcmos_ctrl_mdsys2(STA_POWER_DOWN);
*/
	clkmgr_unlock(flags);

	clk_info("[%s]: id = %d\n", __func__, id);

	WARN_ON(err);

	return !slept;
}
EXPORT_SYMBOL(md_power_off);

int conn_power_on(void)
{
	int err = 0;
	unsigned long flags;
	struct subsys *sys = id_to_sys(SYS_CONN);

#ifdef Bring_Up
#if !defined(CONFIG_MTK_FPGA)
	spm_mtcmos_ctrl_connsys(STA_POWER_ON);
#endif
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);
	BUG_ON(sys->type != SYS_TYPE_CONN);

	clkmgr_lock(flags);
	/* spm_mtcmos_ctrl_connsys(STA_POWER_ON); */
	err = subsys_enable_internal(sys, "conn");
	clkmgr_unlock(flags);

	clk_info("[%s]\n", __func__);

	WARN_ON(err);

	return err;
}
EXPORT_SYMBOL(conn_power_on);

int conn_power_off(void)
{
	int err = 0;
	unsigned long flags;
	struct subsys *sys = id_to_sys(SYS_CONN);

#ifdef Bring_Up
#if !defined(CONFIG_MTK_FPGA)
	spm_mtcmos_ctrl_connsys(STA_POWER_DOWN);
#endif
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!sys);
	BUG_ON(sys->type != SYS_TYPE_CONN);

	clkmgr_lock(flags);
	/* spm_mtcmos_ctrl_connsys(STA_POWER_DOWN); */
	err = subsys_disable_internal(sys, 0, "conn");
	clkmgr_unlock(flags);

	clk_info("[%s]\n", __func__);

	WARN_ON(err);

	return err;
}
EXPORT_SYMBOL(conn_power_off);


static DEFINE_MUTEX(larb_monitor_lock);
static LIST_HEAD(larb_monitor_handlers);

void register_larb_monitor(struct larb_monitor *handler)
{
	struct list_head *pos;

#ifdef Bring_Up
	return;
#endif

	clk_info("register_larb_monitor\n");
	mutex_lock(&larb_monitor_lock);
	list_for_each(pos, &larb_monitor_handlers) {
		struct larb_monitor *l;

		l = list_entry(pos, struct larb_monitor, link);
		if (l->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	mutex_unlock(&larb_monitor_lock);
}
EXPORT_SYMBOL(register_larb_monitor);


void unregister_larb_monitor(struct larb_monitor *handler)
{
#ifdef Bring_Up
	return;
#endif

	mutex_lock(&larb_monitor_lock);
	list_del(&handler->link);
	mutex_unlock(&larb_monitor_lock);
}
EXPORT_SYMBOL(unregister_larb_monitor);

static void larb_clk_prepare(int larb_idx)
{
	switch (larb_idx) {
	case MT_LARB_DISP:
		/* display */
		clk_writel(DISP_CG_CLR0, 0x3);
		break;
	case MT_LARB_VDEC:
		/* vde */
		clk_writel(LARB_CKEN_SET, 0x1);
		break;
	case MT_LARB_IMG:
		/* isp */
		clk_writel(IMG_CG_CLR, 0x1);
		break;
	case MT_LARB_VENC:
		/* venc */
		clk_writel(VENC_CG_SET, 0x11);
		break;
	/* case MT_LARB_MJC: */
		/* mjc */
		/* clk_writel(MJC_CG_CLR, 0x21); */
		/* break; */
	default:
		BUG();
	}
}

static void larb_clk_finish(int larb_idx)
{
	switch (larb_idx) {
	case MT_LARB_DISP:
		/* display */
		clk_writel(DISP_CG_SET0, 0x3);
		break;
	case MT_LARB_VDEC:
		/* vde */
		clk_writel(LARB_CKEN_CLR, 0x1);
		break;
	case MT_LARB_IMG:
		/* isp */
		clk_writel(IMG_CG_SET, 0x1);
		break;
	case MT_LARB_VENC:
		/* venc */
		clk_writel(VENC_CG_CLR, 0x11);
		break;
	/* case MT_LARB_MJC: */
		/* mjc */
		/* clk_writel(MJC_CG_SET, 0x21); */
		/* break; */
	default:
		BUG();
	}
}

static void larb_backup(int larb_idx)
{
	struct larb_monitor *pos;

	/* clk_info("[%s]: start to backup larb%d\n", __func__, larb_idx); */
	clk_dbg("[%s]: backup larb%d\n", __func__, larb_idx);

	larb_clk_prepare(larb_idx);

	list_for_each_entry(pos, &larb_monitor_handlers, link) {
		if (pos->backup != NULL) {
			/* clk_info("[%s]: backup larb\n", __func__); */
			pos->backup(pos, larb_idx);
		}
	}

	larb_clk_finish(larb_idx);
}

static void larb_restore(int larb_idx)
{
	struct larb_monitor *pos;

	/* clk_info("[%s]: start to restore larb%d\n", __func__, larb_idx); */
	clk_dbg("[%s]: restore larb%d\n", __func__, larb_idx);

	larb_clk_prepare(larb_idx);

	list_for_each_entry(pos, &larb_monitor_handlers, link) {
		if (pos->restore != NULL) {
			/* clk_info("[%s]: restore larb\n", __func__); */
			pos->restore(pos, larb_idx);
		}
	}

	larb_clk_finish(larb_idx);
}



/************************************************
 **********         clkmux part        **********
 ************************************************/

static struct clkmux_ops clkmux_ops;
static struct clkmux_ops audio_clkmux_ops;
/* static struct clkmux_ops hd_audio_clkmux_ops; */

static struct clkmux muxs[NR_MUXS] = {
	{
	.name = __stringify(MUX_MM), /* 0 */
	/* .base_addr = CLK_CFG_0, */
	.sel_mask = 0x07000000,
	.pdn_mask = 0x80000000,
	.offset = 24,
	.nr_inputs = 8,
	.ops = &clkmux_ops,
	.pll = &plls[VENCPLL],
	}, {
	.name = __stringify(MUX_DDRPHY), /* 1 */
	/* .base_addr = CLK_CFG_0, */
	.sel_mask = 0x00010000,
	.pdn_mask = 0x00800000,
	.offset = 16,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_MEM), /* 2 */
	/* .base_addr = CLK_CFG_0, */
	.sel_mask = 0x00000100,
	.pdn_mask = 0x00008000,
	.offset = 8,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_AXI), /* 3 */
	/* .base_addr = CLK_CFG_0, */
	.sel_mask = 0x00000007,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 8,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_CAMTG), /* 4 */
	/* .base_addr = CLK_CFG_1, */
	.sel_mask = 0x07000000,
	.pdn_mask = 0x80000000,
	.offset = 24,
	.nr_inputs = 7,
	.ops = &clkmux_ops,
	.pll = &plls[UNIVPLL],
	}, {
	.name = __stringify(MUX_MFG), /* 5 */
	/* .base_addr = CLK_CFG_1, */
	.sel_mask = 0x000f0000,
	.pdn_mask = 0x00800000,
	.offset = 16,
	.nr_inputs = 14,
	.ops = &clkmux_ops,
	.siblings = &muxs[MT_MUX_MFG13M],
	.pll = &plls[MMPLL],
	}, {
	.name = __stringify(MUX_VDEC), /* 6 */
	/* .base_addr = CLK_CFG_1, */
	.sel_mask = 0x00000700,
	.pdn_mask = 0x00008000,
	.offset = 8,
	.nr_inputs = 8,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_PWM), /* 7 */
	/* .base_addr = CLK_CFG_1, */
	.sel_mask = 0x00000003,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 4,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_MSDC50_0), /* 8 */
	/* .base_addr = CLK_CFG_2, */
	.sel_mask = 0x07000000,
	.pdn_mask = 0x80000000,
	.offset = 24,
	.nr_inputs = 6,
	.ops = &clkmux_ops,
	/* .pll = &plls[MSDCPLL], */
	}, {
	.name = __stringify(MUX_USB20), /* 9 */
	/* .base_addr = CLK_CFG_2, */
	.sel_mask = 0x00030000,
	.pdn_mask = 0x00800000,
	.offset = 16,
	.nr_inputs = 3,
	.ops = &clkmux_ops,
	.pll = &plls[UNIVPLL],
	}, {
	.name = __stringify(MUX_SPI), /* 10 */
	/* .base_addr = CLK_CFG_2, */
	.sel_mask = 0x00000700,
	.pdn_mask = 0x00008000,
	.offset = 8,
	.nr_inputs = 7,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_UART), /* 11 */
	/* .base_addr = CLK_CFG_2, */
	.sel_mask = 0x00000001,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_MSDC30_3), /* 12 */
	/* .base_addr = CLK_CFG_3, */
	.sel_mask = 0x0f000000,
	.pdn_mask = 0x80000000,
	.offset = 24,
	.nr_inputs = 9,
	.ops = &clkmux_ops,
	.pll = &plls[MSDCPLL],
	}, {
	.name = __stringify(MUX_MSDC30_2), /* 13 */
	/* .base_addr = CLK_CFG_3, */
	.sel_mask = 0x00070000,
	.pdn_mask = 0x00800000,
	.offset = 16,
	.nr_inputs = 8,
	.ops = &clkmux_ops,
	.pll = &plls[MSDCPLL],
	}, {
	.name = __stringify(MUX_MSDC30_1), /* 14 */
	/* .base_addr = CLK_CFG_3, */
	.sel_mask = 0x00000700,
	.pdn_mask = 0x00008000,
	.offset = 8,
	.nr_inputs = 8,
	.ops = &clkmux_ops,
	.pll = &plls[MSDCPLL],
	}, {
	.name = __stringify(MUX_MSDC30_0), /* 15 */
	/* .base_addr = CLK_CFG_3, */
	.sel_mask = 0x0000000f,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 11,
	.ops = &clkmux_ops,
	.siblings = &muxs[MT_MUX_MSDC50_0],
	.pll = &plls[MSDCPLL],
	}, {
	.name = __stringify(MUX_SCP), /* 16 */
	/* .base_addr = CLK_CFG_4, */
	.sel_mask = 0x03000000,
	.pdn_mask = 0x80000000,
	.offset = 24,
	.nr_inputs = 4,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_PMICSPI), /* 17 */
	/* .base_addr = CLK_CFG_4, */
	.sel_mask = 0x00070000,
	.pdn_mask = 0x00800000,
	.offset = 16,
	.nr_inputs = 8,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_AUDINTBUS), /* 18 */
	/* .base_addr = CLK_CFG_4, */
	.sel_mask = 0x00000300,
	.pdn_mask = 0x00008000,
	.offset = 8,
	.nr_inputs = 4,
	.ops = &audio_clkmux_ops,
	.siblings = &muxs[MT_MUX_AUDIO],
	}, {
	.name = __stringify(MUX_AUDIO), /* 19 */
	/* .base_addr = CLK_CFG_4, */
	.sel_mask = 0x00000003,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 4,
	.ops = &audio_clkmux_ops,
	}, {
	.name = __stringify(MUX_MFG13M), /* 20 */
	/* .base_addr = CLK_CFG_5, */
	.sel_mask = 0x01000000,
	.pdn_mask = 0x80000000,
	.offset = 24,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_SCAM), /* 21 */
	/* .base_addr = CLK_CFG_5, */
	.sel_mask = 0x00030000,
	.pdn_mask = 0x00800000,
	.offset = 16,
	.nr_inputs = 4,
	.ops = &clkmux_ops,
	/* .pll = &plls[UNIVPLL], */
	}, {
	.name = __stringify(MUX_DPI0), /* 22 */
	/* .base_addr = CLK_CFG_5, */
	.sel_mask = 0x00000700,
	.pdn_mask = 0x00008000,
	.offset = 8,
	.nr_inputs = 5,
	.ops = &clkmux_ops,
	.pll = &plls[TVDPLL],
	}, {
	.name = __stringify(MUX_ATB), /* 23 */
	/* .base_addr = CLK_CFG_5, */
	.sel_mask = 0x00000003,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 4,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_IRTX), /* 24 */
	/* .base_addr = CLK_CFG_6, */
	.sel_mask = 0x01000000,
	.pdn_mask = 0x80000000,
	.offset = 24,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	}, {
	.name = __stringify(MUX_IRDA), /* 25 */
	/* .base_addr = CLK_CFG_6, */
	.sel_mask = 0x00010000,
	.pdn_mask = 0x00800000,
	.offset = 16,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	.pll = &plls[UNIVPLL],
	}, {
	.name = __stringify(MUX_AUD2), /* 26 */
	/* .base_addr = CLK_CFG_6, */
	.sel_mask = 0x00000100,
	.pdn_mask = 0x00008000,
	.offset = 8,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	.pll = &plls[APLL2],
	}, {
	.name = __stringify(MUX_AUD1), /* 27 */
	/* .base_addr = CLK_CFG_6, */
	.sel_mask = 0x00000001,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 2,
	.ops = &clkmux_ops,
	.pll = &plls[APLL1],
	}, {
	.name = __stringify(MUX_DISPPWM), /* 28 */
	/* .base_addr = CLK_CFG_7, */
	.sel_mask = 0x00000003,
	.pdn_mask = 0x00000080,
	.offset = 0,
	.nr_inputs = 4,
	.ops = &clkmux_ops,
	.pll = &plls[UNIVPLL],
	}
};


static struct clkmux *id_to_mux(unsigned int id)
{
	return id < NR_MUXS ? muxs + id : NULL;
}


#define mux_to_id(mux) (mux-muxs)

static void clkmux_sel_op(struct clkmux *mux, unsigned clksrc)
{
/* volatile unsigned int reg; */
	unsigned int id;

	id = mux_to_id(mux);

#ifdef MUX_LOG_TOP
	/* clk_info("[%s]: mux->name=%s, clksrc=%d\n", __func__, mux->name, clksrc); */
	clk_dbg("[%s]: mux->name=%s, clksrc=%d\n", __func__, mux->name, clksrc);
#endif

#if 0
	reg = clk_readl(mux->base_addr);

	reg &= ~(mux->sel_mask);
	reg |= (clksrc << mux->offset) & mux->sel_mask;

	clk_writel(mux->base_addr, reg);
#else
	clk_writel(mux->base_addr + 8, mux->sel_mask);	/* clr */
	clk_writel(mux->base_addr + 4, (clksrc << mux->offset));	/* set */

	if (id == MT_MUX_AXI) {
		if (clksrc == 2)
			clk_clrl(PERI_GLOBALCON_CKSEL, 1);	/* 218M, bit 0 set 0 */
		else
			clk_setl(PERI_GLOBALCON_CKSEL, 1);	/* 136M, bit 0 set 1 */
	}
#ifdef CONFIG_MTK_RAM_CONSOLE
	if (id < 4)
		aee_rr_rec_clk(0, clk_readl(mux->base_addr));
	else if (id < 8)
		aee_rr_rec_clk(1, clk_readl(mux->base_addr));
	else if (id < 12)
		aee_rr_rec_clk(2, clk_readl(mux->base_addr));
	else if (id < 16)
		aee_rr_rec_clk(3, clk_readl(mux->base_addr));
	else if (id < 20)
		aee_rr_rec_clk(4, clk_readl(mux->base_addr));
	else if (id < 24)
		aee_rr_rec_clk(5, clk_readl(mux->base_addr));
	else if (id < 28)
		aee_rr_rec_clk(6, clk_readl(mux->base_addr));
	else if (id < 32)
		aee_rr_rec_clk(7, clk_readl(mux->base_addr));
#endif

#endif

}

static void clkmux_enable_op(struct clkmux *mux)
{
	unsigned int id;

	id = mux_to_id(mux);

#ifdef MUX_LOG
	/* clk_info("[%s]: mux->name=%s\n", __func__, mux->name); */
	clk_dbg("[%s]: mux->name=%s\n", __func__, mux->name);
#endif


#if 0
	    clk_clrl(mux->base_addr, mux->pdn_mask);
#else
	    clk_writel(mux->base_addr + 8, mux->pdn_mask);	/* write clr reg */

#ifdef CONFIG_MTK_RAM_CONSOLE
	if (id < 4)
		aee_rr_rec_clk(0, clk_readl(mux->base_addr));
	else if (id < 8)
		aee_rr_rec_clk(1, clk_readl(mux->base_addr));
	else if (id < 12)
		aee_rr_rec_clk(2, clk_readl(mux->base_addr));
	else if (id < 16)
		aee_rr_rec_clk(3, clk_readl(mux->base_addr));
	else if (id < 20)
		aee_rr_rec_clk(4, clk_readl(mux->base_addr));
	else if (id < 24)
		aee_rr_rec_clk(5, clk_readl(mux->base_addr));
	else if (id < 28)
		aee_rr_rec_clk(6, clk_readl(mux->base_addr));
	else if (id < 32)
		aee_rr_rec_clk(7, clk_readl(mux->base_addr));
#endif

#endif
}

static void clkmux_disable_op(struct clkmux *mux)
{
	unsigned int id;

	id = mux_to_id(mux);

#ifdef MUX_LOG
	/* clk_info("[%s]: mux->name=%s\n", __func__, mux->name); */
	clk_dbg("[%s]: mux->name=%s\n", __func__, mux->name);
#endif


#if 0
	    clk_setl(mux->base_addr, mux->pdn_mask);
#else
	    clk_writel(mux->base_addr + 4, mux->pdn_mask);	/* write set reg */

#ifdef CONFIG_MTK_RAM_CONSOLE
	if (id < 4)
		aee_rr_rec_clk(0, clk_readl(mux->base_addr));
	else if (id < 8)
		aee_rr_rec_clk(1, clk_readl(mux->base_addr));
	else if (id < 12)
		aee_rr_rec_clk(2, clk_readl(mux->base_addr));
	else if (id < 16)
		aee_rr_rec_clk(3, clk_readl(mux->base_addr));
	else if (id < 20)
		aee_rr_rec_clk(4, clk_readl(mux->base_addr));
	else if (id < 24)
		aee_rr_rec_clk(5, clk_readl(mux->base_addr));
	else if (id < 28)
		aee_rr_rec_clk(6, clk_readl(mux->base_addr));
	else if (id < 32)
		aee_rr_rec_clk(7, clk_readl(mux->base_addr));
#endif

#endif
}

static struct clkmux_ops clkmux_ops = {
	.sel = clkmux_sel_op,
	.enable = clkmux_enable_op,
	.disable = clkmux_disable_op,
};

/*
static struct clkmux_ops hd_audio_clkmux_ops = {
	.enable = clkmux_enable_op,
	.disable = clkmux_disable_op,
};*/
/*
static void audio_clkmux_enable_op(struct clkmux *mux)
{
#ifdef MUX_LOG
	//clk_info("[%s]: mux->name=%s\n", __func__, mux->name);
	clk_dbg("[%s]: mux->name=%s\n", __func__, mux->name);
#endif
	clk_clrl(mux->base_addr, mux->pdn_mask);
};
*/
static struct clkmux_ops audio_clkmux_ops = {
	.sel = clkmux_sel_op,
	/* .enable = audio_clkmux_enable_op, */
	.enable = clkmux_enable_op,
	.disable = clkmux_disable_op,
};

static void clkmux_sel_locked(struct clkmux *mux, unsigned int clksrc)
{
	mux->ops->sel(mux, clksrc);
}

static void mux_enable_locked(struct clkmux *mux)
{
	mux->cnt++;

#ifdef MUX_LOG_TOP
	clk_info("[%s]: Start. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif

	if (mux->cnt > 1)
		return;

	if (mux->pll)
		pll_enable_internal(mux->pll, "mux");

/* if (mux->parent) { */
/* mux_enable_internal(mux->parent, "mux_p"); */
/* } */
	if (mux->ops)
		mux->ops->enable(mux);

	if (mux->siblings)
		mux_enable_internal(mux->siblings, "mux_s");

#ifdef MUX_LOG_TOP
	clk_info("[%s]: End. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif
}

static void mux_disable_locked(struct clkmux *mux)
{
#ifdef MUX_LOG_TOP
	clk_info("[%s]: Start. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif

	BUG_ON(!mux->cnt);

	mux->cnt--;

#ifdef MUX_LOG_TOP
	clk_info("[%s]: Start. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif

	if (mux->cnt > 0)
		return;

	if (mux->ops)
		mux->ops->disable(mux);

	if (mux->siblings)
		mux_disable_internal(mux->siblings, "mux_s");

/* if (mux->parent) { */
/* mux_disable_internal(mux->siblings, "mux_p"); */
/* } */

	if (mux->pll)
		pll_disable_internal(mux->pll, "mux");

#ifdef MUX_LOG_TOP
	clk_info("[%s]: End. mux->name=%s, mux->cnt=%d\n", __func__, mux->name, mux->cnt);
#endif
}

int clkmux_sel(int id, unsigned int clksrc, char *name)
{
	unsigned long flags;
	struct clkmux *mux = id_to_mux(id);

#ifdef Bring_Up
	unsigned int reg;

	if (id == MT_MUX_CAMTG) {
		reg = clk_readl(CLK_CFG_1);
		reg &= ~(0x07000000);
		reg |= (clksrc << 24) & 0x07000000;
		clk_writel(CLK_CFG_1, reg);
	} else if (id == MT_MUX_DPI0) {
		reg = clk_readl(CLK_CFG_5);
		reg &= ~(0x00000700);
		reg |= (clksrc << 8) & 0x00000700;
		clk_writel(CLK_CFG_5, reg);
	}
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!mux);
	BUG_ON(clksrc >= mux->nr_inputs);

	clkmgr_lock(flags);
	clkmux_sel_locked(mux, clksrc);
	clkmgr_unlock(flags);

	return 0;
}
EXPORT_SYMBOL(clkmux_sel);

void enable_mux(int id, char *name)
{
	unsigned long flags;
	struct clkmux *mux = id_to_mux(id);

#ifdef Bring_Up
	return;
#endif

#ifndef PLL_CLK_LINK
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!mux);
	BUG_ON(!name);
#ifdef MUX_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
/* #else */
/* if(id == MT_MUX_MM) */
/* clk_info("[%s]: id=%d, name=%s\n", __func__, id, name); */
#endif
	clkmgr_lock(flags);
	mux_enable_internal(mux, name);
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(enable_mux);

void disable_mux(int id, char *name)
{
	unsigned long flags;
	struct clkmux *mux = id_to_mux(id);

#ifdef Bring_Up
	return;
#endif

#ifndef PLL_CLK_LINK
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!mux);
	BUG_ON(!name);
#ifdef MUX_LOG_TOP
	clk_info("[%s]: id=%d, name=%s\n", __func__, id, name);
/* #else */
/* if(id == MT_MUX_MM) */
/* clk_info("[%s]: id=%d, name=%s\n", __func__, id, name); */
#endif
	clkmgr_lock(flags);
	mux_disable_internal(mux, name);
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(disable_mux);

/************************************************
 **********         cg_grp part        **********
 ************************************************/

static struct cg_grp_ops general_cg_grp_ops;
static struct cg_grp_ops disp0_cg_grp_ops;
static struct cg_grp_ops vdec_cg_grp_ops;
static struct cg_grp_ops venc_cg_grp_ops;

static struct cg_grp grps[NR_GRPS] = {
	{
	.name = __stringify(CG_INFRA),
/* .set_addr = INFRA_PDN_SET0,    //disable */
/* .clr_addr = INFRA_PDN_CLR0,    //enable */
/* .sta_addr = INFRA_PDN_STA0, */
	.mask = 0x00FD91FF,
	.ops = &general_cg_grp_ops,
	}, {
	.name = __stringify(CG_PERI),
/* .set_addr = INFRA_PDN_SET1,    //disable */
/* .clr_addr = INFRA_PDN_CLR1,    //enable */
/* .sta_addr = INFRA_PDN_STA1, */
	.mask = 0x3FFFFFFF,
	.ops = &general_cg_grp_ops,
	}, {
	.name = __stringify(CG_DISP0),
/* .set_addr = DISP_CG_SET0,    //disable */
/* .clr_addr = DISP_CG_CLR0,    //enable */
/* .sta_addr = DISP_CG_CON0, */
/* .dummy_addr = MMSYS_DUMMY, */
/* .bw_limit_addr = SMI_LARB_BWL_EN_REG, */
	.mask = 0x0007FFFF,
	.ops = &disp0_cg_grp_ops,
	.sys = &syss[SYS_DIS],
	}, {
	.name = __stringify(CG_DISP1),
/* .set_addr = DISP_CG_SET1,    //disable */
/* .clr_addr = DISP_CG_CLR1,    //enable */
/* .sta_addr = DISP_CG_CON1, */
	.mask = 0x0000003C,
	.ops = &general_cg_grp_ops,
	.sys = &syss[SYS_DIS],
	}, {
	.name = __stringify(CG_IMAGE),
/* .set_addr = IMG_CG_SET,        //disable */
/* .clr_addr = IMG_CG_CLR,        //enable */
/* .sta_addr = IMG_CG_CON, */
	.mask = 0x00000FE1,
	.ops = &general_cg_grp_ops,
	.sys = &syss[SYS_ISP],
	}, {
	.name = __stringify(CG_MFG),
/* .set_addr = MFG_CG_SET,        //disable */
/* .clr_addr = MFG_CG_CLR,        //enable */
/* .sta_addr = MFG_CG_CON, */
	.mask = 0x00000001,
	.ops = &general_cg_grp_ops,
	.sys = &syss[SYS_MFG],
	}, {
	.name = __stringify(CG_AUDIO),
/* .sta_addr = AUDIO_TOP_CON0, */
	.mask = 0x0F0C0344,
	.ops = &general_cg_grp_ops,
/* .sys = &syss[SYS_AUD], */
	}, {
	.name = __stringify(CG_VDEC0),
/* .set_addr = VDEC_CKEN_CLR,    //disable */
/* .clr_addr = VDEC_CKEN_SET,    //enable */
	.mask = 0x00000001,
	.ops = &vdec_cg_grp_ops,
	.sys = &syss[SYS_VDE],
	}, {
	.name = __stringify(CG_VDEC1),
/* .set_addr = LARB_CKEN_CLR,    //disable */
/* .clr_addr = LARB_CKEN_SET,    //enable */
	.mask = 0x00000001,
	.ops = &vdec_cg_grp_ops,
	.sys = &syss[SYS_VDE],
	}, /* {
	.name = __stringify(CG_MJC),
	.set_addr = MJC_CG_SET,    //disable
	.clr_addr = MJC_CG_CLR,    //enable
	.sta_addr = MJC_CG_CON,
	.mask = 0x0000002F,
	.ops = &general_cg_grp_ops,
	.sys = &syss[SYS_MJC],
	}, */ {
	.name = __stringify(CG_VENC),
	/* .set_addr = VENC_CG_CLR,    //disable */
	/* .clr_addr = VENC_CG_SET,    //enable */
	/* .sta_addr = VENC_CG_CON, */
	.mask = 0x00001111,
	.ops = &venc_cg_grp_ops,
	.sys = &syss[SYS_VEN],
	}
};

static struct cg_grp *id_to_grp(unsigned int id)
{
	return id < NR_GRPS ? grps + id : NULL;
}

static unsigned int general_grp_get_state_op(struct cg_grp *grp)
{
	volatile unsigned int val;
	struct subsys *sys = grp->sys;

	if (sys && !sys->state)
		return 0;

	val = clk_readl(grp->sta_addr);
	val = (~val) & (grp->mask);
	return val;
}

static int general_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->sta_addr);
/* *(ptr) = clk_readl(grp->sta_addr) & grp->mask; */

	return 1;
}

static struct cg_grp_ops general_cg_grp_ops = {
	.get_state = general_grp_get_state_op,
	.dump_regs = general_grp_dump_regs_op,
};

static unsigned int disp0_grp_get_state_op(struct cg_grp *grp)
{
	volatile unsigned int val;
	struct subsys *sys = grp->sys;

	if (sys && !sys->state)
		return 0;

	val = clk_readl(grp->dummy_addr);
	val = (~val) & (grp->mask);
	return val;
}

static int disp0_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->sta_addr);
	*(++ptr) = clk_readl(grp->dummy_addr);
/* *(++ptr) = clk_readl(grp->bw_limit_addr); */

	return 2;
}

static struct cg_grp_ops disp0_cg_grp_ops = {
	.get_state = disp0_grp_get_state_op,
	.dump_regs = disp0_grp_dump_regs_op,
};

static unsigned int vdec_grp_get_state_op(struct cg_grp *grp)
{
	volatile unsigned int val = 0;

	val = clk_readl(grp->set_addr);
	val &= grp->mask;
	return val;
}

static int vdec_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->set_addr);
	*(++ptr) = clk_readl(grp->clr_addr);

	return 2;
}

static struct cg_grp_ops vdec_cg_grp_ops = {
	.get_state = vdec_grp_get_state_op,
	.dump_regs = vdec_grp_dump_regs_op,
};


static unsigned int venc_grp_get_state_op(struct cg_grp *grp)
{
	volatile unsigned int val = 0;

	val = clk_readl(grp->sta_addr);
	val &= grp->mask;
	return val;
}

static int venc_grp_dump_regs_op(struct cg_grp *grp, unsigned int *ptr)
{
	*(ptr) = clk_readl(grp->sta_addr);

	return 1;
}

static struct cg_grp_ops venc_cg_grp_ops = {
	.get_state = venc_grp_get_state_op,
	.dump_regs = venc_grp_dump_regs_op,
};


/************************************************
 **********         cg_clk part        **********
 ************************************************/

static struct cg_clk_ops general_cg_clk_ops;

#if 0
static struct cg_clk_ops audio_cg_clk_ops;
#endif

static struct cg_clk_ops audsys_cg_clk_ops;	/* @audio sys */
static struct cg_clk_ops disp0_cg_clk_ops;
static struct cg_clk_ops vdec_cg_clk_ops;
static struct cg_clk_ops venc_cg_clk_ops;

static struct cg_clk clks[NR_CLKS] = {
	[CG_INFRA_FROM ... CG_INFRA_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_INFRA],
	},
	[CG_PERI_FROM ... CG_PERI_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_PERI],
	},
	[CG_DISP0_FROM ... CG_DISP0_TO] = {
		.cnt = 0,
		.ops = &disp0_cg_clk_ops,
		.grp = &grps[CG_DISP0],
	},
	[CG_DISP1_FROM ... CG_DISP1_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_DISP1],
	},
	[CG_IMAGE_FROM ... CG_IMAGE_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_IMAGE],
	},
	[CG_MFG_FROM ... CG_MFG_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_MFG],
	},
	[CG_AUDIO_FROM ... CG_AUDIO_TO] = {
		.cnt = 0,
		.ops = &audsys_cg_clk_ops,
		.grp = &grps[CG_AUDIO],
	},
	[CG_VDEC0_FROM ... CG_VDEC0_TO] = {
		.cnt = 0,
		.ops = &vdec_cg_clk_ops,
		.grp = &grps[CG_VDEC0],
	},
	[CG_VDEC1_FROM ... CG_VDEC1_TO] = {
		.cnt = 0,
		.ops = &vdec_cg_clk_ops,
		.grp = &grps[CG_VDEC1],
	},
	/* [CG_MJC_FROM ... CG_MJC_TO] = {
		.cnt = 0,
		.ops = &general_cg_clk_ops,
		.grp = &grps[CG_MJC],
	}, */
	[CG_VENC_FROM ... CG_VENC_TO] = {
		.cnt = 0,
		.ops = &venc_cg_clk_ops,
		.grp = &grps[CG_VENC],
	},
};

static struct cg_clk *id_to_clk(unsigned int id)
{
	return id < NR_CLKS ? clks + id : NULL;
}

static int general_clk_get_state_op(struct cg_clk *clk)
{
	struct subsys *sys = clk->grp->sys;

	if (sys && !sys->state)
		return PWR_DOWN;

	return (clk_readl(clk->grp->sta_addr) & (clk->mask)) ? PWR_DOWN : PWR_ON;
}

static int general_clk_check_validity_op(struct cg_clk *clk)
{
	int valid = 0;

	if (clk->mask & clk->grp->mask)
		valid = 1;

	return valid;
}

static int general_clk_enable_op(struct cg_clk *clk)
{
#ifdef CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	clk_writel(clk->grp->clr_addr, clk->mask);
	return 0;
}

static int general_clk_disable_op(struct cg_clk *clk)
{
#ifdef CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

	clk_writel(clk->grp->set_addr, clk->mask);
	return 0;
}

static struct cg_clk_ops general_cg_clk_ops = {
	.get_state = general_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = general_clk_enable_op,
	.disable = general_clk_disable_op,
};

static int disp0_clk_get_state_op(struct cg_clk *clk)
{
	struct subsys *sys = clk->grp->sys;

	if (sys && !sys->state)
		return PWR_DOWN;

	return (clk_readl(clk->grp->dummy_addr) & (clk->mask)) ? PWR_DOWN : PWR_ON;
}

static int disp0_clk_enable_op(struct cg_clk *clk)
{
#ifdef DISP_CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif

/* clk_writel(clk->grp->clr_addr, clk->mask); */
	clk_clrl(clk->grp->dummy_addr, clk->mask);

	if (clk->mask & 0x00000203)
		clk_writel(clk->grp->clr_addr, clk->mask);

	return 0;
}

static int disp0_clk_disable_op(struct cg_clk *clk)
{
#ifdef DISP_CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif
/* clk_writel(clk->grp->set_addr, clk->mask); */
	clk_setl(clk->grp->dummy_addr, clk->mask);

	if (clk->mask & 0x00000203)
		clk_writel(clk->grp->set_addr, clk->mask);

	return 0;
}

static struct cg_clk_ops disp0_cg_clk_ops = {
	.get_state = disp0_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = disp0_clk_enable_op,
	.disable = disp0_clk_disable_op,
};

#if 0
static int audio_clk_enable_op(struct cg_clk *clk)
{
#ifdef CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif
	clk_writel(clk->grp->clr_addr, clk->mask);
/* clk_setl(TOPAXI_SI0_CTL, 1U << 7); //audio not from AXI */
	return 0;
}

static int audio_clk_disable_op(struct cg_clk *clk)
{
#ifdef CLK_LOG
	clk_info("[%s]: clk->grp->name=%s, clk->mask=0x%x\n", __func__, clk->grp->name, clk->mask);
#endif
/* clk_clrl(TOPAXI_SI0_CTL, 1U << 7); //audio not from AXI */
	clk_writel(clk->grp->set_addr, clk->mask);
	return 0;
}

static struct cg_clk_ops audio_cg_clk_ops = {
	.get_state = general_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = audio_clk_enable_op,
	.disable = audio_clk_disable_op,
};
#endif

static int audsys_clk_enable_op(struct cg_clk *clk)
{
/* clk_info("[%s]: CLK_CFG_2=0x%x, CLK_CFG_3=0x%x\n", __func__, clk_readl(CLK_CFG_2),clk_readl(CLK_CFG_3)); */
	clk_clrl(clk->grp->sta_addr, clk->mask);
	return 0;
}

static int audsys_clk_disable_op(struct cg_clk *clk)
{
	clk_setl(clk->grp->sta_addr, clk->mask);
	return 0;
}

static struct cg_clk_ops audsys_cg_clk_ops = {
	.get_state = general_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = audsys_clk_enable_op,
	.disable = audsys_clk_disable_op,
};


static int vdec_clk_get_state_op(struct cg_clk *clk)
{
	return (clk_readl(clk->grp->set_addr) & (clk->mask)) ? PWR_ON : PWR_DOWN;
}

static struct cg_clk_ops vdec_cg_clk_ops = {
	.get_state = vdec_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = general_clk_enable_op,
	.disable = general_clk_disable_op,
};

static int venc_clk_get_state_op(struct cg_clk *clk)
{
	return (clk_readl(clk->grp->sta_addr) & (clk->mask)) ? PWR_ON : PWR_DOWN;
}

static struct cg_clk_ops venc_cg_clk_ops = {
	.get_state = venc_clk_get_state_op,
	.check_validity = general_clk_check_validity_op,
	.enable = general_clk_enable_op,
	.disable = general_clk_disable_op,
};


#ifdef PLL_CLK_LINK
static int power_prepare_locked(struct cg_grp *grp)
{
	int err = 0;

	if (grp->sys)
		err = subsys_enable_internal(grp->sys, "clk");

	return err;
}

static int power_finish_locked(struct cg_grp *grp)
{
	int err = 0;

	if (grp->sys)
		err = subsys_disable_internal(grp->sys, 0, "clk");

	return err;
}
#endif

static int clk_enable_locked(struct cg_clk *clk)
{
	struct cg_grp *grp = clk->grp;
	unsigned int local_state;
#ifdef STATE_CHECK_DEBUG
	unsigned int reg_state;
#endif

#ifdef PLL_CLK_LINK
	int err;
#endif

	clk->cnt++;

#ifdef CLK_LOG
	clk_info
	    ("[%s]: Start. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
	     __func__, grp->name, grp->state, clk->mask, clk->cnt, clk->state);
#endif

	if (clk->cnt > 1)
		return 0;

	local_state = clk->state;

#ifdef STATE_CHECK_DEBUG
	reg_state = grp->ops->get_state(grp, clk);
	/* BUG_ON(local_state != reg_state); */
#endif

#ifdef PLL_CLK_LINK
	if (clk->mux)
		mux_enable_internal(clk->mux, "clk");

	err = power_prepare_locked(grp);
	BUG_ON(err);
#endif

/* if (clk->parent) { */
/* clk_enable_internal(clk->parent, "clk"); */
/* } */

	if (local_state == PWR_ON)
		return 0;

	clk->ops->enable(clk);

	clk->state = PWR_ON;
	grp->state |= clk->mask;
#ifdef CLK_LOG
	clk_info
	    ("[%s]: End. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
	     __func__, grp->name, grp->state, clk->mask, clk->cnt, clk->state);
#endif

	return 0;
}

static void clk_stat_bug(void);
static int clk_disable_locked(struct cg_clk *clk)
{
	struct cg_grp *grp = clk->grp;
	unsigned int local_state;
#ifdef STATE_CHECK_DEBUG
	unsigned int reg_state;
#endif

#ifdef PLL_CLK_LINK
	int err;
#endif

#ifdef CLK_LOG
	clk_info
	    ("[%s]: Start. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
	     __func__, grp->name, grp->state, clk->mask, clk->cnt, clk->state);
#endif

	if (!clk->cnt) {
		clk_info
		    ("[%s]: grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
		     __func__, grp->name, grp->state, clk->mask, clk->cnt, clk->state);
#ifdef CONFIG_CLKMGR_STAT
		clk_stat_bug();
#endif
	}
	BUG_ON(!clk->cnt);
	clk->cnt--;

#ifdef CLK_LOG
	clk_info
	    ("[%s]: Start. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
	     __func__, grp->name, grp->state, clk->mask, clk->cnt, clk->state);
#endif

	if (clk->cnt > 0)
		return 0;

	local_state = clk->state;

#ifdef STATE_CHECK_DEBUG
	reg_state = grp->ops->get_state(grp, clk);
	/* BUG_ON(local_state != reg_state); */
#endif

	if (local_state == PWR_DOWN)
		return 0;

	if (clk->force_on)
		return 0;

	clk->ops->disable(clk);

	clk->state = PWR_DOWN;
	grp->state &= ~(clk->mask);

/* if (clk->parent) { */
/* clk_disable_internal(clk->parent, "clk"); */
/* } */

#ifdef PLL_CLK_LINK
	err = power_finish_locked(grp);
	BUG_ON(err);

	if (clk->mux)
		mux_disable_internal(clk->mux, "clk");
#endif

#ifdef CLK_LOG
	clk_info
	    ("[%s]: End. grp->name=%s, grp->state=0x%x, clk->mask=0x%x, clk->cnt=%d, clk->state=%d\n",
	     __func__, grp->name, grp->state, clk->mask, clk->cnt, clk->state);
#endif

	return 0;
}

static int get_clk_state_locked(struct cg_clk *clk)
{
	if (likely(initialized))
		return clk->state;
	else
		return clk->ops->get_state(clk);
}

int mt_enable_clock(int id, char *name)
{
	int err;
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));
	BUG_ON(!name);
#ifdef CLK_LOG_TOP
	clk_info("[%s]: id=%d, names=%s\n", __func__, id, name);
#else
/*
	if ((id == MT_CG_DISP0_SMI_COMMON))
	clk_dbg("[%s]: id=%d, names=%s\n", __func__, id, name);
*/
#endif

	clkmgr_lock(flags);
	err = clk_enable_internal(clk, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(mt_enable_clock);


int mt_disable_clock(int id, char *name)
{
	int err;
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));
	BUG_ON(!name);
#ifdef CLK_LOG_TOP
	clk_info("[%s]: id=%d, names=%s\n", __func__, id, name);
#else
/*
	if (id == MT_CG_DISP0_SMI_COMMON)
	clk_dbg("[%s]: id=%d, names=%s\n", __func__, id, name);
*/
#endif

	clkmgr_lock(flags);
	err = clk_disable_internal(clk, name);
	clkmgr_unlock(flags);

	return err;
}
EXPORT_SYMBOL(mt_disable_clock);

int enable_clock_ext_locked(int id, char *name)
{
	int err;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	BUG_ON(!clkmgr_locked());
	err = clk_enable_internal(clk, name);

	return err;
}
EXPORT_SYMBOL(enable_clock_ext_locked);


int disable_clock_ext_locked(int id, char *name)
{
	int err;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	BUG_ON(!clkmgr_locked());
	err = clk_disable_internal(clk, name);

	return err;
}
EXPORT_SYMBOL(disable_clock_ext_locked);

int clock_is_on(int id)
{
	int state;
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 1;
#endif

	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	clkmgr_lock(flags);
	state = get_clk_state_locked(clk);
	clkmgr_unlock(flags);

	return state;
}
EXPORT_SYMBOL(clock_is_on);


static void clk_set_force_on_locked(struct cg_clk *clk)
{
	clk->force_on = 1;
}

static void clk_clr_force_on_locked(struct cg_clk *clk)
{
	clk->force_on = 0;
}

void clk_set_force_on(int id)
{
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	clkmgr_lock(flags);
	clk_set_force_on_locked(clk);
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(clk_set_force_on);

void clk_clr_force_on(int id)
{
	unsigned long flags;
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	clkmgr_lock(flags);
	clk_clr_force_on_locked(clk);
	clkmgr_unlock(flags);
}
EXPORT_SYMBOL(clk_clr_force_on);

int clk_is_force_on(int id)
{
	struct cg_clk *clk = id_to_clk(id);

#ifdef Bring_Up
	return 0;
#endif

	BUG_ON(!initialized);
	BUG_ON(!clk);
	BUG_ON(!clk->grp);
	BUG_ON(!clk->ops->check_validity(clk));

	return clk->force_on;
}

int grp_dump_regs(int id, unsigned int *ptr)
{
	struct cg_grp *grp = id_to_grp(id);

#ifdef Bring_Up
	return 0;
#endif

	/* BUG_ON(!initialized); */
	BUG_ON(!grp);

	return grp->ops->dump_regs(grp, ptr);
}
EXPORT_SYMBOL(grp_dump_regs);

const char *grp_get_name(int id)
{
	struct cg_grp *grp = id_to_grp(id);

#ifdef Bring_Up
	return 0;
#endif

	/* BUG_ON(!initialized); */
	BUG_ON(!grp);

	return grp->name;
}

void print_grp_regs(void)
{
	int i;
	int cnt;
	unsigned int value[3] = {
	0, 0, 0};
	const char *name;

	for (i = 0; i < NR_GRPS; i++) {
		name = grp_get_name(i);
		cnt = grp_dump_regs(i, value);
		if (cnt == 1) {
			clk_info("[%02d][%-8s]=[0x%08x]\n", i, name, value[0]);
		} else if (cnt == 2) {
			clk_info("[%02d][%-8s]=[0x%08x][0x%08x]\n", i, name, value[0], value[1]);
		} else {
			clk_info("[%02d][%-8s]=[0x%08x][0x%08x][0x%08x]\n", i, name, value[0],
				 value[1], value[2]);
		}
	}
}



/************************************************
 **********       initialization       **********
 ************************************************/

#if 0
static void subsys_all_force_on(void)
{
	if (test_spm_gpu_power_on())
		spm_mtcmos_ctrl_mfg(STA_POWER_ON);
	else
		clk_warn("[%s]: not force to turn on MFG\n", __func__);

	spm_mtcmos_ctrl_vdec(STA_POWER_ON);
	spm_mtcmos_ctrl_venc(STA_POWER_ON);
}
#endif


#define INFRA_CG  0xFFFFFFFF
#define PERI_CG   0xFFFFFFFF
#define AUD_CG    0x0F0C0344
#define MFG_CG    0x00000001
#define DISP0_CG  0xFFFFFFFF
#define DISP1_CG  0x0000003F
#define IMG_CG    0x00000FE1
#define VDEC_CG   0x00000001
#define LARB_CG   0x00000001
#define VENC_CG   0x00001111


static void cg_all_force_on(void)
{
	/* INFRA CG */
	clk_writel(INFRA_PDN_CLR0, INFRA_CG);
	clk_writel(PERI_PDN_CLR0, PERI_CG);

	/* AUDIO */
/*	clk_clrl(AUDIO_TOP_CON0, AUD_CG); */
	/* MFG */
	clk_writel(MFG_CG_CLR, MFG_CG);
	/* DISP */
/*	clk_writel(MMSYS_DUMMY, 0); */
	/* ISP */
/*	clk_writel(IMG_CG_CLR, IMG_CG); */
	/* VDE */
/*	clk_writel(VDEC_CKEN_SET, VDEC_CG); */
/*	clk_writel(LARB_CKEN_SET, LARB_CG); */
	/* VENC */
/*	clk_writel(VENC_CG_SET, VENC_CG); */
}


static void cg_bootup_pdn(void)
{
#if 0
	/* AUDIO */
	clk_writel(AUDIO_TOP_CON0, AUD_CG);

	/* INFRA CG */
	clk_writel(INFRA_PDN_SET0, 0x008a);
	clk_writel(PERI_PDN_SET0, 0x3fc1fffc);

	/* MFG */
	clk_writel(MFG_CG_SET, MFG_CG);

	/* DISP */
/* clk_writel(DISP_CG_SET0, 0xff9ffffc); //DCM enable */
/* clk_writel(DISP_CG_SET1, 0x0000003F); // */

	/* ISP */
	clk_writel(IMG_CG_SET, IMG_CG);

	/* VDE */
	clk_writel(VDEC_CKEN_CLR, VDEC_CG);
	clk_writel(LARB_CKEN_CLR, LARB_CG);

	/* VENC */
	clk_clrl(VENC_CG_CON, VENC_CG);
#endif
}


static void mt_subsys_init(void)
{
	int i;
	struct subsys *sys;
/* **** */
/*
	syss[SYS_MD1].ctl_addr = SPM_MD_PWR_CON;
	syss[SYS_CONN].ctl_addr = SPM_CONN_PWR_CON;
	syss[SYS_DIS].ctl_addr = SPM_DIS_PWR_CON;
	syss[SYS_MFG].ctl_addr = SPM_MFG_PWR_CON;
	syss[SYS_ISP].ctl_addr = SPM_ISP_PWR_CON;
	syss[SYS_VDE].ctl_addr = SPM_VDE_PWR_CON;
	syss[SYS_VEN].ctl_addr = SPM_VEN_PWR_CON;
	syss[SYS_MD2].ctl_addr = SPM_MD2_PWR_CON;
*/
	for (i = 0; i < NR_SYSS; i++) {
		sys = &syss[i];
		sys->state = sys->ops->get_state(sys);

		if (sys->state != sys->default_sta) {
			clk_info("[%s]%s, change state: (%u->%u)\n", __func__,
				 sys->name, sys->state, sys->default_sta);
			if (sys->default_sta == PWR_DOWN)
				sys_disable_locked(sys, 1);
			else
				sys_enable_locked(sys);
		}
#ifdef CONFIG_CLKMGR_STAT
		INIT_LIST_HEAD(&sys->head);
#endif
	}
}

static void mt_plls_init(void)
{
	int i;
	struct pll *pll;

	plls[ARMPLL].base_addr = ARMPLL_CON0;
	plls[ARMPLL].pwr_addr = ARMPLL_PWR_CON0;
	plls[MAINPLL].base_addr = MAINPLL_CON0;
	plls[MAINPLL].pwr_addr = MAINPLL_PWR_CON0;
	plls[MSDCPLL].base_addr = MSDCPLL_CON0;
	plls[MSDCPLL].pwr_addr = MSDCPLL_PWR_CON0;
	plls[UNIVPLL].base_addr = UNIVPLL_CON0;
	plls[UNIVPLL].pwr_addr = UNIVPLL_PWR_CON0;
	plls[MMPLL].base_addr = MMPLL_CON0;
	plls[MMPLL].pwr_addr = MMPLL_PWR_CON0;
	plls[VENCPLL].base_addr = VENCPLL_CON0;
	plls[VENCPLL].pwr_addr = VENCPLL_PWR_CON0;
	plls[TVDPLL].base_addr = TVDPLL_CON0;
	plls[TVDPLL].pwr_addr = TVDPLL_PWR_CON0;
	plls[APLL1].base_addr = APLL1_CON0;
	plls[APLL1].pwr_addr = APLL1_PWR_CON0;
	plls[APLL2].base_addr = APLL2_CON0;
	plls[APLL2].pwr_addr = APLL2_PWR_CON0;

	for (i = 0; i < NR_PLLS; i++) {
		pll = &plls[i];
		pll->state = pll->ops->get_state(pll);

		/* clk_info("[%s]: pll->name=%s, pll->state=%d\n", __func__, pll->name, pll->state); */

#ifdef CONFIG_CLKMGR_STAT
		INIT_LIST_HEAD(&pll->head);
#endif
	}
	plls[MMPLL].cnt = 1;
	plls[VENCPLL].cnt = 1;
/* plls[UNIVPLL].cnt = 1; */
}

/*
static void mt_plls_enable_hp(void)
{
	int i;
	struct pll *pll;
	for (i = 0; i < NR_PLLS; i++) {
	pll = &plls[i];
	if (pll->ops->hp_enable) {
	    pll->ops->hp_enable(pll);
	}
	}
}
*/

static void mt_muxs_init(void)
{
	int i;
	struct clkmux *mux;

	muxs[MT_MUX_MM].base_addr = CLK_CFG_0;
	muxs[MT_MUX_DDRPHY].base_addr = CLK_CFG_0;
	muxs[MT_MUX_MEM].base_addr = CLK_CFG_0;
	muxs[MT_MUX_AXI].base_addr = CLK_CFG_0;

	muxs[MT_MUX_CAMTG].base_addr = CLK_CFG_1;
	muxs[MT_MUX_MFG].base_addr = CLK_CFG_1;
	muxs[MT_MUX_VDEC].base_addr = CLK_CFG_1;
	muxs[MT_MUX_PWM].base_addr = CLK_CFG_1;

	muxs[MT_MUX_MSDC50_0].base_addr = CLK_CFG_2;
	muxs[MT_MUX_USB20].base_addr = CLK_CFG_2;
	muxs[MT_MUX_SPI].base_addr = CLK_CFG_2;
	muxs[MT_MUX_UART].base_addr = CLK_CFG_2;

	muxs[MT_MUX_MSDC30_0].base_addr = CLK_CFG_3;
	muxs[MT_MUX_MSDC30_1].base_addr = CLK_CFG_3;
	muxs[MT_MUX_MSDC30_2].base_addr = CLK_CFG_3;
	muxs[MT_MUX_MSDC30_3].base_addr = CLK_CFG_3;

	muxs[MT_MUX_SCP].base_addr = CLK_CFG_4;
	muxs[MT_MUX_PMICSPI].base_addr = CLK_CFG_4;
	muxs[MT_MUX_AUDINTBUS].base_addr = CLK_CFG_4;
	muxs[MT_MUX_AUDIO].base_addr = CLK_CFG_4;

	muxs[MT_MUX_MFG13M].base_addr = CLK_CFG_5;
	muxs[MT_MUX_SCAM].base_addr = CLK_CFG_5;
	muxs[MT_MUX_DPI0].base_addr = CLK_CFG_5;
	muxs[MT_MUX_ATB].base_addr = CLK_CFG_5;

	muxs[MT_MUX_IRTX].base_addr = CLK_CFG_6;
	muxs[MT_MUX_IRDA].base_addr = CLK_CFG_6;
	muxs[MT_MUX_AUD2].base_addr = CLK_CFG_6;
	muxs[MT_MUX_AUD1].base_addr = CLK_CFG_6;

	muxs[MT_MUX_DISPPWM].base_addr = CLK_CFG_7;

	for (i = 0; i < NR_MUXS; i++) {
		mux = &muxs[i];
#ifdef CONFIG_CLKMGR_STAT
		INIT_LIST_HEAD(&mux->head);
#endif
	}
/* muxs[MT_MUX_AUDINTBUS].cnt = 1; */
/* muxs[MT_MUX_AUDIO].cnt = 1; */
	muxs[MT_MUX_MM].cnt = 1;
	muxs[MT_MUX_MFG].cnt = 1;
	muxs[MT_MUX_MFG13M].cnt = 1;
	muxs[MT_MUX_VDEC].cnt = 1;
/* muxs[MT_MUX_MJC].cnt = 1; */
}


static void mt_clks_init(void)
{
	int i, j;
	struct cg_grp *grp;
	struct cg_clk *clk;

	clk_writel(MMSYS_DUMMY, clk_readl(DISP_CG_CON0));

	grps[CG_INFRA].set_addr = INFRA_PDN_SET0;
	grps[CG_INFRA].clr_addr = INFRA_PDN_CLR0;
	grps[CG_INFRA].sta_addr = INFRA_PDN_STA0;
	grps[CG_PERI].set_addr = PERI_PDN_SET0;
	grps[CG_PERI].clr_addr = PERI_PDN_CLR0;
	grps[CG_PERI].sta_addr = PERI_PDN_STA0;
	grps[CG_DISP0].set_addr = DISP_CG_SET0;
	grps[CG_DISP0].clr_addr = DISP_CG_CLR0;
	grps[CG_DISP0].sta_addr = DISP_CG_CON0;
	grps[CG_DISP0].dummy_addr = MMSYS_DUMMY;
	grps[CG_DISP1].set_addr = DISP_CG_SET1;
	grps[CG_DISP1].clr_addr = DISP_CG_CLR1;
	grps[CG_DISP1].sta_addr = DISP_CG_CON1;
	grps[CG_IMAGE].set_addr = IMG_CG_SET;
	grps[CG_IMAGE].clr_addr = IMG_CG_CLR;
	grps[CG_IMAGE].sta_addr = IMG_CG_CON;
	grps[CG_MFG].set_addr = MFG_CG_SET;
	grps[CG_MFG].clr_addr = MFG_CG_CLR;
	grps[CG_MFG].sta_addr = MFG_CG_CON;
	grps[CG_AUDIO].sta_addr = AUDIO_TOP_CON0;
	grps[CG_VDEC0].clr_addr = VDEC_CKEN_SET;
	grps[CG_VDEC0].set_addr = VDEC_CKEN_CLR;
	grps[CG_VDEC1].clr_addr = LARB_CKEN_SET;
	grps[CG_VDEC1].set_addr = LARB_CKEN_CLR;
	grps[CG_VENC].clr_addr = VENC_CG_SET;
	grps[CG_VENC].set_addr = VENC_CG_CLR;
	grps[CG_VENC].sta_addr = VENC_CG_CON;


	for (i = 0; i < NR_GRPS; i++) {
		grp = &grps[i];
		grp->state = grp->ops->get_state(grp);

		/* clk_info("[%s]: grps=%d\n", __func__, i); */

		for (j = 0; j < 32; j++) {
			if (grp->mask & (1U << j)) {
				clk = &clks[i * 32 + j];
				/* clk->grp = grp; */
				/* clk->cnt = 0; */
				clk->mask = 1U << j;
				clk->state = clk->ops->get_state(clk);
				/* (grp->state & clk->mask) ? PWR_DOWN : PWR_ON; */

				/* clk_info("[%s]: clk=%d, clk->state=%d\n", __func__, j, clk->state); */

#ifdef CONFIG_CLKMGR_STAT
				INIT_LIST_HEAD(&clk->head);
#endif
			}
		}
	}
	clks[MT_CG_PERI_DISP_PWM].mux = &muxs[MT_MUX_DISPPWM];

	clks[MT_CG_PERI_USB0].mux = &muxs[MT_MUX_USB20];
	clks[MT_CG_PERI_IRDA].mux = &muxs[MT_MUX_IRDA];

	clks[MT_CG_PERI_MSDC30_0].mux = &muxs[MT_MUX_MSDC30_0];
	clks[MT_CG_PERI_MSDC30_1].mux = &muxs[MT_MUX_MSDC30_1];
	clks[MT_CG_PERI_MSDC30_2].mux = &muxs[MT_MUX_MSDC30_2];
	clks[MT_CG_PERI_MSDC30_3].mux = &muxs[MT_MUX_MSDC30_3];

	clks[MT_CG_PERI_UART0].mux = &muxs[MT_MUX_UART];
	clks[MT_CG_PERI_UART1].mux = &muxs[MT_MUX_UART];
	clks[MT_CG_PERI_UART2].mux = &muxs[MT_MUX_UART];
	clks[MT_CG_PERI_UART3].mux = &muxs[MT_MUX_UART];
	clks[MT_CG_PERI_UART4].mux = &muxs[MT_MUX_UART];

	clks[MT_CG_PERI_SPI0].mux = &muxs[MT_MUX_SPI];
	clks[MT_CG_PERI_IRTX].mux = &muxs[MT_MUX_IRTX];

	clks[MT_CG_AUDIO_AFE].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_I2S].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_22M].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_24M].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_APLL2_TUNER].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_APLL_TUNER].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_ADC].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_DAC].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_DAC_PREDIS].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_AUDIO_TML].mux = &muxs[MT_MUX_AUDINTBUS];
	clks[MT_CG_INFRA_AUDIO].mux = &muxs[MT_MUX_AUDINTBUS];

	clks[MT_CG_IMAGE_SEN_TG].mux = &muxs[MT_MUX_CAMTG];

	clks[MT_CG_DISP1_DPI_PIXEL].mux = &muxs[MT_MUX_DPI0];

	/* Don't disable these clock until it's clk_clr_force_on() is called */
	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_LARB0]);
	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_COMMON]);

}

/* #endif //#ifndef Bring_Up */

#ifdef CONFIG_OF
void iomap(void)
{
	struct device_node *node;

/* apmixed */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-apmixedsys");
	if (!node)
		pr_err("[CLK_APMIXED] find node failed\n");
	clk_apmixed_base = of_iomap(node, 0);
	if (!clk_apmixed_base)
		pr_err("[CLK_APMIXED] base failed\n");
/* cksys_base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-topckgen");
	if (!node)
		pr_err("[CLK_CKSYS] find node failed\n");
	clk_cksys_base = of_iomap(node, 0);
	if (!clk_cksys_base)
		pr_err("[CLK_CKSYS] base failed\n");
/* infracfg_ao */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-infrasys");
	if (!node)
		pr_err("[CLK_INFRACFG_AO] find node failed\n");
	clk_infracfg_ao_base = of_iomap(node, 0);
	if (!clk_infracfg_ao_base)
		pr_err("[CLK_INFRACFG_AO] base failed\n");
/* pericfg_base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-perisys");
	if (!node)
		pr_err("[PERICFG] find node failed\n");
	clk_pericfg_base = of_iomap(node, 0);
	if (!clk_pericfg_base)
		pr_err("[PERICFG] base failed\n");
/* audio */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-audiosys");
	if (!node)
		pr_err("[CLK_AUDIO] find node failed\n");
	clk_audio_base = of_iomap(node, 0);
	if (!clk_audio_base)
		pr_err("[CLK_AUDIO] base failed\n");
/* mfgcfg */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-mfgsys");
	if (!node)
		pr_err("[CLK_G3D_CONFIG] find node failed\n");
	clk_mfgcfg_base = of_iomap(node, 0);
	if (!clk_mfgcfg_base)
		pr_err("[CLK_G3D_CONFIG] base failed\n");
/* mmsys_config */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-mmsys");
	if (!node)
		pr_err("[CLK_MMSYS_CONFIG] find node failed\n");
	clk_mmsys_config_base = of_iomap(node, 0);
	if (!clk_mmsys_config_base)
		pr_err("[CLK_MMSYS_CONFIG] base failed\n");
/* imgsys */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-imgsys");
	if (!node)
		pr_err("[CLK_IMGSYS_CONFIG] find node failed\n");
	clk_imgsys_base = of_iomap(node, 0);
	if (!clk_imgsys_base)
		pr_err("[CLK_IMGSYS_CONFIG] base failed\n");
/* vdec_gcon */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-vdec_gcon");
	if (!node)
		pr_err("[CLK_VDEC_GCON] find node failed\n");
	clk_vdec_gcon_base = of_iomap(node, 0);
	if (!clk_vdec_gcon_base)
		pr_err("[CLK_VDEC_GCON] base failed\n");
/* venc_gcon */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-venc_gcon");
	if (!node)
		pr_err("[CLK_VENC_GCON] find node failed\n");
	clk_venc_gcon_base = of_iomap(node, 0);
	if (!clk_venc_gcon_base)
		pr_err("[CLK_VENC_GCON] base failed\n");
}
#endif

int mt_clkmgr_init(void)
{
	iomap();
	BUG_ON(initialized);

/*
	spm_mtcmos_ctrl_vdec(STA_POWER_DOWN);
	spm_mtcmos_ctrl_venc(STA_POWER_DOWN);
	spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);
*/
	spm_mtcmos_ctrl_vdec(STA_POWER_ON);
	spm_mtcmos_ctrl_venc(STA_POWER_ON);
	spm_mtcmos_ctrl_isp(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg(STA_POWER_ON);
/* spm_mtcmos_ctrl_connsys(STA_POWER_ON); */

	cg_all_force_on();

	cg_bootup_pdn();

	return 1;

	mt_plls_init();
	mt_subsys_init();
	mt_muxs_init();
	mt_clks_init();

	initialized = 1;
/* **** */
/* mt_freqhopping_init(); */
	print_grp_regs();

#ifndef CONFIG_ARCH_MT6753
	pr_warn("%s: CLKMGR_INCFILE_VER=%s\n", __func__, CLKMGR_INCFILE_VER);
#endif

	return 0;
}

/* **** */
/*
#ifdef CONFIG_MTK_MMC
extern void msdc_clk_status(int * status);
#else
void msdc_clk_status(int * status) { *status = 0; }
#endif


#define VEN_PWR_STA_MASK    (0x1 << 8)
#define VDE_PWR_STA_MASK    (0x1 << 7)
#define ISP_PWR_STA_MASK    (0x1 << 5)
#define MFG_PWR_STA_MASK    (0x1 << 4)

bool clkmgr_idle_can_enter(unsigned int *condition_mask, unsigned int *block_mask)
{
	int i, j;
	unsigned int sd_mask = 0;
	unsigned int cg_mask = 0;

#ifdef PLL_CLK_LINK
	unsigned int sta;
#endif
	msdc_clk_status(&sd_mask);
	if (sd_mask) {
		block_mask[CG_PERI] |= sd_mask;
		return false;
	}

	for (i = CG_INFRA; i < NR_GRPS; i++) {
		cg_mask = grps[i].state & condition_mask[i];
		if (cg_mask)
		{
			for (j = CG_INFRA; j < NR_GRPS; j++)
			{
				block_mask[j] = grps[j].state & condition_mask[j];
			}

	    //block_mask[i] |= cg_mask;
	    return false;
		}
	}

#ifdef PLL_CLK_LINK
	sta = clk_readl(SPM_PWR_STATUS);
	if (sta & (MFG_PWR_STA_MASK | ISP_PWR_STA_MASK | VDE_PWR_STA_MASK | VEN_PWR_STA_MASK))
	return false;
#endif
	return true;
}
*/
static unsigned int clk_cfg_4;
void clkmgr_faudintbus_pll2sq(void)
{
	clk_cfg_4 = clk_readl(CLK_CFG_4);
	clk_writel(CLK_CFG_4, clk_cfg_4 & 0xFFFFFCFF);
}

void clkmgr_faudintbus_sq2pll(void)
{
	clk_writel(CLK_CFG_4, clk_cfg_4);
}

/************************************************
 **********       function debug       **********
 ************************************************/

static int pll_test_read(struct seq_file *m, void *v)
{
	int i, j;
	int cnt;
	unsigned int value[3];
	const char *name;

	seq_puts(m, "********** pll register dump **********\n");
	for (i = 0; i < NR_PLLS; i++) {
		name = pll_get_name(i);
		cnt = pll_dump_regs(i, value);
		for (j = 0; j < cnt; j++)
			seq_printf(m, "[%d][%-7s reg%d]=[0x%08x]\n", i, name, j, value[j]);
	}

	seq_puts(m, "\n********** pll_test help **********\n");
	seq_puts(m, "enable  pll: echo enable  id [mod_name] > /proc/clkmgr/pll_test\n");
	seq_puts(m, "disable pll: echo disable id [mod_name] > /proc/clkmgr/pll_test\n");

	return 0;
}

static ssize_t pll_test_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data) {
	char desc[32];
	int len = 0;

	char cmd[10];
	char mod_name[10];
	int id;
	int err = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%9s %d %9s", cmd, &id, mod_name) == 3) {
		if (!strcmp(cmd, "enable"))
			err = enable_pll(id, mod_name);
		else if (!strcmp(cmd, "disable"))
			err = disable_pll(id, mod_name);
	} else if (sscanf(desc, "%9s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "enable"))
			err = enable_pll(id, "pll_test");
		else if (!strcmp(cmd, "disable"))
			err = disable_pll(id, "pll_test");
	}

	clk_info("[%s]%s pll %d: result is %d\n", __func__, cmd, id, err);

	return count;
}


static int pll_fsel_read(struct seq_file *m, void *v)
{
	int i;
	int cnt;
	unsigned int value[3] = {
	0, 0, 0};
	const char *name;

	for (i = 0; i < NR_PLLS; i++) {
		name = pll_get_name(i);
		if (pll_is_on(i)) {
			cnt = pll_dump_regs(i, value);
			if (cnt >= 2) {
				seq_printf(m, "[%d][%-7s]=[0x%08x%08x]\n", i, name, value[0],
					   value[1]);
			} else {
				seq_printf(m, "[%d][%-7s]=[0x%08x]\n", i, name, value[0]);
			}
		} else {
			seq_printf(m, "[%d][%-7s]=[-1]\n", i, name);
		}
	}

	seq_puts(m, "\n********** pll_fsel help **********\n");
	seq_puts(m, "adjust pll frequency:  echo id freq > /proc/clkmgr/pll_fsel\n");

	return 0;
}

static ssize_t pll_fsel_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data) {
	char desc[32];
	int len = 0;

	int id;
	unsigned int value;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%d %x", &id, &value) == 2)
		pll_fsel(id, value);

	return count;
}


#ifdef CONFIG_CLKMGR_STAT
static int pll_stat_read(struct seq_file *m, void *v)
{
	struct pll *pll;
	struct list_head *pos;
	struct stat_node *node;
	int i;

	seq_puts(m, "\n********** pll stat dump **********\n");
	for (i = 0; i < NR_PLLS; i++) {
		pll = id_to_pll(i);
		seq_printf(m, "[%d][%-7s]state=%u, cnt=%u", i, pll->name, pll->state, pll->cnt);
		list_for_each(pos, &pll->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)", node->name, node->cnt_on, node->cnt_off);
		}
		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** pll_dump help **********\n");

	return 0;
}
#endif


static int subsys_test_read(struct seq_file *m, void *v)
{
	int i;
	int state;
	unsigned int value = 0, sta = 0, sta_s = 0;
	const char *name;
/* **** */
/* sta = clk_readl(SPM_PWR_STATUS); */
/* sta_s = clk_readl(SPM_PWR_STATUS_2ND); */

	seq_puts(m, "********** subsys register dump **********\n");
	for (i = 0; i < NR_SYSS; i++) {
		name = subsys_get_name(i);
		state = subsys_is_on(i);
		subsys_dump_regs(i, &value);
		seq_printf(m, "[%d][%-7s]=[0x%08x], state(%u)\n", i, name, value, state);
	}
	seq_printf(m, "SPM_PWR_STATUS=0x%08x, SPM_PWR_STATUS_2ND=0x%08x\n", sta, sta_s);

	seq_puts(m, "\n********** subsys_test help **********\n");
	seq_puts(m, "enable subsys:  echo enable id > /proc/clkmgr/subsys_test\n");
	seq_puts(m, "disable subsys: echo disable id [force_off] > /proc/clkmgr/subsys_test\n");

	return 0;
}

static ssize_t subsys_test_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data) {
	char desc[32];
	int len = 0;

	char cmd[10];
	int id;
	int force_off;
	int err = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%9s %d %d", cmd, &id, &force_off) == 3) {
		if (!strcmp(cmd, "disable"))
			err = disable_subsys_force(id, "test");
	} else if (sscanf(desc, "%9s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "enable"))
			err = enable_subsys(id, "test");
		else if (!strcmp(cmd, "disable"))
			err = disable_subsys(id, "test");
	}

	clk_info("[%s]%s subsys %d: result is %d\n", __func__, cmd, id, err);

	return count;
}


#ifdef CONFIG_CLKMGR_STAT
static int subsys_stat_read(struct seq_file *m, void *v)
{
	struct subsys *sys;
	struct list_head *pos;
	struct stat_node *node;
	int i;

	seq_puts(m, "\n********** subsys stat dump **********\n");
	for (i = 0; i < NR_SYSS; i++) {
		sys = id_to_sys(i);
		seq_printf(m, "[%d][%-7s]state=%u", i, sys->name, sys->state);
		list_for_each(pos, &sys->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)", node->name, node->cnt_on, node->cnt_off);
		}
		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** subsys_dump help **********\n");

	return 0;
}
#endif


static int mux_test_read(struct seq_file *m, void *v)
{
	seq_puts(m, "********** mux register dump *********\n");
	seq_printf(m, "[CLK_CFG_0]=0x%08x\n", clk_readl(CLK_CFG_0));
	seq_printf(m, "[CLK_CFG_1]=0x%08x\n", clk_readl(CLK_CFG_1));
	seq_printf(m, "[CLK_CFG_2]=0x%08x\n", clk_readl(CLK_CFG_2));
	seq_printf(m, "[CLK_CFG_3]=0x%08x\n", clk_readl(CLK_CFG_3));
	seq_printf(m, "[CLK_CFG_4]=0x%08x\n", clk_readl(CLK_CFG_4));
	seq_printf(m, "[CLK_CFG_5]=0x%08x\n", clk_readl(CLK_CFG_5));
	seq_printf(m, "[CLK_CFG_6]=0x%08x\n", clk_readl(CLK_CFG_6));
	seq_printf(m, "[CLK_CFG_7]=0x%08x\n", clk_readl(CLK_CFG_7));

	seq_puts(m, "\n********** mux_test help *********\n");

	return 0;
}



#ifdef CONFIG_CLKMGR_STAT
static int mux_stat_read(struct seq_file *m, void *v)
{
	struct clkmux *mux;
	struct list_head *pos;
	struct stat_node *node;
	int i;

	seq_puts(m, "********** mux stat dump **********\n");
	for (i = 0; i < NR_MUXS; i++) {
		mux = id_to_mux(i);
#if 0
		seq_printf(m, "[%02d][%-14s]state=%u, cnt=%u", i, mux->name, mux->state, mux->cnt);
#else
		seq_printf(m, "[%02d][%-14s]cnt=%u", i, mux->name, mux->cnt);
#endif
		list_for_each(pos, &mux->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)", node->name, node->cnt_on, node->cnt_off);
		}
		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** mux_dump help **********\n");

	return 0;
}
#endif


static int clk_test_read(struct seq_file *m, void *v)
{
	int i;
	int cnt;
	unsigned int value[3];
	const char *name;

	seq_puts(m, "********** clk register dump **********\n");

	for (i = 0; i < NR_GRPS; i++) {
		name = grp_get_name(i);
		cnt = grp_dump_regs(i, value);
		if (cnt == 1) {
			seq_printf(m, "[%02d][%-8s]=[0x%08x]\n", i, name, value[0]);
		} else if (cnt == 2) {
			seq_printf(m, "[%02d][%-8s]=[0x%08x][0x%08x]\n", i, name, value[0],
				   value[1]);
		} else {
			seq_printf(m, "[%02d][%-8s]=[0x%08x][0x%08x][0x%08x]\n", i, name, value[0],
				   value[1], value[2]);
		}
	}

	seq_puts(m, "\n********** clk_test help **********\n");
	seq_puts(m, "enable  clk: echo enable  id [mod_name] > /proc/clkmgr/clk_test\n");
	seq_puts(m, "disable clk: echo disable id [mod_name] > /proc/clkmgr/clk_test\n");
	seq_puts(m, "read state:  echo id > /proc/clkmgr/clk_test\n");

	return 0;
}

static ssize_t clk_test_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data) {
	char desc[32];
	int len = 0;

	char cmd[10];
	char mod_name[10];
	int id;
	int err;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%9s %d %9s", cmd, &id, mod_name) == 3) {
		if (!strcmp(cmd, "enable"))
			err = enable_clock(id, mod_name);
		else if (!strcmp(cmd, "disable"))
			err = disable_clock(id, mod_name);
	} else if (sscanf(desc, "%9s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "enable"))
			err = enable_clock(id, "pll_test");
		else if (!strcmp(cmd, "disable"))
			err = disable_clock(id, "pll_test");
	}
	/* clk_info("[%s]%s clock %d: result is %d\n", __func__, cmd, id, err); */

	return count;
}


#ifdef CONFIG_CLKMGR_STAT
static int clk_stat_read(struct seq_file *m, void *v)
{
	struct cg_clk *clk;
	struct list_head *pos;
	struct stat_node *node;
	int i, grp, offset;
	int skip;

	seq_puts(m, "\n********** clk stat dump **********\n");
	for (i = 0; i < NR_CLKS; i++) {
		grp = i / 32;
		offset = i % 32;
		if (offset == 0)
			seq_printf(m, "\n*****[%02d][%-8s]*****\n", grp, grp_get_name(grp));

		clk = id_to_clk(i);
		if (!clk || !clk->grp || !clk->ops->check_validity(clk))
			continue;

		skip = (clk->cnt == 0) && (clk->state == 0) && list_empty(&clk->head);
		if (skip)
			continue;

		seq_printf(m, "[%02d]state=%u, cnt=%u", offset, clk->state, clk->cnt);
		list_for_each(pos, &clk->head) {
			node = list_entry(pos, struct stat_node, link);
			seq_printf(m, "\t(%s,%u,%u)", node->name, node->cnt_on, node->cnt_off);
		}
		seq_puts(m, "\n");
	}

	seq_puts(m, "\n********** clk_dump help **********\n");

	return 0;
}

void clk_stat_check(int id)
{
	struct cg_clk *clk;
	struct list_head *pos;
	struct stat_node *node;
	int i, j, grp, offset;
	int skip;

	if (id == SYS_DIS) {
		for (i = CG_DISP0_FROM; i <= CG_DISP0_TO; i++) {
			grp = i / 32;
			offset = i % 32;
			clk = id_to_clk(i);

			if (!clk || !clk->grp || !clk->ops->check_validity(clk))
				continue;

			skip = (clk->cnt == 0) && (clk->state == 0) && list_empty(&clk->head);
			if (skip)
				continue;

			pr_err(" [%02d]state=%u, cnt=%u", offset, clk->state, clk->cnt);

			j = 0;
			list_for_each(pos, &clk->head) {
				node = list_entry(pos, struct stat_node, link);
				pr_err(" (%s,%u,%u)", node->name, node->cnt_on, node->cnt_off);
				if (++j % 3 == 0)
					pr_err("\n \t\t\t\t ");
			}
			pr_err("\n");
		}
	}
}
EXPORT_SYMBOL(clk_stat_check);

static void clk_stat_bug(void)
{
	struct cg_clk *clk;
	struct list_head *pos;
	struct stat_node *node;
	int i, j, grp, offset;
	int skip;

	for (i = 0; i < NR_CLKS; i++) {
		grp = i / 32;
		offset = i % 32;
		if (offset == 0)
			pr_err("\n*****[%02d][%-8s]*****\n", grp, grp_get_name(grp));

		clk = id_to_clk(i);

		if (!clk || !clk->grp || !clk->ops->check_validity(clk))
			continue;

		skip = (clk->cnt == 0) && (clk->state == 0) && list_empty(&clk->head);
		if (skip)
			continue;

		pr_err(" [%02d]state=%u, cnt=%u", offset, clk->state, clk->cnt);

		j = 0;
		list_for_each(pos, &clk->head) {
			node = list_entry(pos, struct stat_node, link);
			pr_err(" (%s,%u,%u)", node->name, node->cnt_on, node->cnt_off);
			if (++j % 3 == 0)
				pr_err("\n \t\t\t\t ");
		}
		pr_err("\n");
	}
}
#endif

void slp_check_pm_mtcmos_pll(void)
{
	int i;

	slp_chk_mtcmos_pll_stat = 1;
	clk_info("[%s]\n", __func__);
	for (i = 3; i < NR_PLLS; i++) {
		if (i == 8)
			continue;
		if (pll_is_on(i)) {
			slp_chk_mtcmos_pll_stat = -1;
			clk_info("%s: on\n", plls[i].name);
			clk_info("suspend warning: %s is on!!!\n", plls[i].name);
			clk_info("warning! warning! warning! it may cause resume fail\n");
		}
	}
	for (i = 0; i < NR_SYSS; i++) {
		if (subsys_is_on(i)) {
			clk_info("%s: on\n", syss[i].name);
			if (i > SYS_CONN) {
				/* aee_kernel_warning("Suspend Warning","%s is on", subsyss[i].name); */
				slp_chk_mtcmos_pll_stat = -1;
				clk_info("suspend warning: %s is on!!!\n", syss[i].name);
				clk_info("warning! warning! warning! it may cause resume fail\n");
#ifdef CONFIG_CLKMGR_STAT
				clk_stat_bug();
#endif
			}
		}
	}
}
EXPORT_SYMBOL(slp_check_pm_mtcmos_pll);


static int clk_force_on_read(struct seq_file *m, void *v)
{
	int i;
	struct cg_clk *clk;

	seq_puts(m, "********** clk force on info dump **********\n");
	for (i = 0; i < NR_CLKS; i++) {
		clk = &clks[i];
		if (clk->force_on) {
			seq_printf(m, "clock %d (0x%08x @ %s) is force on\n", i,
				   clk->mask, clk->grp->name);
		}
	}

	seq_puts(m, "\n********** clk_force_on help **********\n");
	seq_puts(m, "set clk force on: echo set id > /proc/clkmgr/clk_force_on\n");
	seq_puts(m, "clr clk force on: echo clr id > /proc/clkmgr/clk_force_on\n");

	return 0;
}

static ssize_t clk_force_on_write(struct file *file, const char __user *buffer,
				  size_t count, loff_t *data) {
	char desc[32];
	int len = 0;

	char cmd[10];
	int id;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%9s %d", cmd, &id) == 2) {
		if (!strcmp(cmd, "set"))
			clk_set_force_on(id);
		else if (!strcmp(cmd, "clr"))
			clk_clr_force_on(id);
	}

	return count;
}

static int slp_chk_mtcmos_pll_stat_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", slp_chk_mtcmos_pll_stat);
	return 0;
}

static int armpll_ckdiv_read(struct seq_file *m, void *v)
{
	seq_printf(m, "TOP_CKDIV1 = 0x%x\n", clk_readl(TOP_CKDIV1));
	return 0;
}

static ssize_t armpll_ckdiv_write(struct file *file, const char __user *buffer,
				  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

/* char cmd[10]; */
	int id;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	/* if (sscanf(desc, "%d", &id) == 1) */
	if (kstrtoint(desc, 10, &id) == 0)
		clk_writel(TOP_CKDIV1, id);	/* CPU clock divide */

	return count;
}

/* for pll_test */
static int proc_pll_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, pll_test_read, NULL);
}

static const struct file_operations pll_test_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_pll_test_open,
	.read  = seq_read,
	.write = pll_test_write,
};

/* for pll_fsel */
static int proc_pll_fsel_open(struct inode *inode, struct file *file)
{
	return single_open(file, pll_fsel_read, NULL);
}

static const struct file_operations pll_fsel_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_pll_fsel_open,
	.read  = seq_read,
	.write = pll_fsel_write,
};

#ifdef CONFIG_CLKMGR_STAT
/* for pll_stat */
static int proc_pll_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, pll_stat_read, NULL);
}

static const struct file_operations pll_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_pll_stat_open,
	.read  = seq_read,
};
#endif

/* for subsys_test */
static int proc_subsys_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, subsys_test_read, NULL);
}

static const struct file_operations subsys_test_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_subsys_test_open,
	.read  = seq_read,
	.write = subsys_test_write
};

#ifdef CONFIG_CLKMGR_STAT
/* for subsys_stat */
static int proc_subsys_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, subsys_stat_read, NULL);
}

static const struct file_operations subsys_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_subsys_stat_open,
	.read  = seq_read,
};
#endif

/* for mux_test */
static int proc_mux_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, mux_test_read, NULL);
}

static const struct file_operations mux_test_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_mux_test_open,
	.read  = seq_read,
};

#ifdef CONFIG_CLKMGR_STAT
/* for mux_stat */
static int proc_mux_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, mux_stat_read, NULL);
}

static const struct file_operations mux_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_mux_stat_open,
	.read  = seq_read,
};
#endif

/* for clk_test */
static int proc_clk_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_test_read, NULL);
}

static const struct file_operations clk_test_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_clk_test_open,
	.read  = seq_read,
	.write = clk_test_write,
};

#ifdef CONFIG_CLKMGR_STAT
/* for clk_stat */
static int proc_clk_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_stat_read, NULL);
}

static const struct file_operations clk_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_clk_stat_open,
	.read  = seq_read,
};
#endif

/* for clk_force_on */
static int proc_clk_force_on_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_force_on_read, NULL);
}

static const struct file_operations clk_force_on_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_clk_force_on_open,
	.read  = seq_read,
	.write = clk_force_on_write,
};

/* for slp_check_pm_mtcmos_pll */
static int proc_slp_chk_mtcmos_pll_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, slp_chk_mtcmos_pll_stat_read, NULL);
}

static const struct file_operations slp_chk_mtcmos_pll_stat_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_slp_chk_mtcmos_pll_stat_open,
	.read  = seq_read,
};

/* for armpll_ckdiv */
static int proc_armpll_ckdiv_open(struct inode *inode, struct file *file)
{
	return single_open(file, armpll_ckdiv_read, NULL);
}
static const struct file_operations armpll_ckdiv_proc_fops = {
	.owner = THIS_MODULE,
	.open  = proc_armpll_ckdiv_open,
	.read  = seq_read,
	.write = armpll_ckdiv_write,
};

void mt_clkmgr_debug_init(void)
{
/* use proc_create */
	struct proc_dir_entry *entry;
	struct proc_dir_entry *clkmgr_dir;

	clkmgr_dir = proc_mkdir("clkmgr", NULL);
	if (!clkmgr_dir) {
		clk_err("[%s]: fail to mkdir /proc/clkmgr\n", __func__);
		return;
	}

	entry = proc_create("pll_test", S_IRUGO | S_IWUSR, clkmgr_dir, &pll_test_proc_fops);

	entry = proc_create("pll_fsel", S_IRUGO | S_IWUSR, clkmgr_dir, &pll_fsel_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("pll_stat", S_IRUGO, clkmgr_dir, &pll_stat_proc_fops);
#endif

	entry = proc_create("subsys_test", S_IRUGO | S_IWUSR, clkmgr_dir, &subsys_test_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("subsys_stat", S_IRUGO, clkmgr_dir, &subsys_stat_proc_fops);
#endif

	entry = proc_create("mux_test", S_IRUGO, clkmgr_dir, &mux_test_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("mux_stat", S_IRUGO, clkmgr_dir, &mux_stat_proc_fops);
#endif

	entry = proc_create("clk_test", S_IRUGO | S_IWUSR, clkmgr_dir, &clk_test_proc_fops);

#ifdef CONFIG_CLKMGR_STAT
	entry = proc_create("clk_stat", S_IRUGO, clkmgr_dir, &clk_stat_proc_fops);
#endif

	entry = proc_create("clk_force_on", S_IRUGO | S_IWUSR, clkmgr_dir, &clk_force_on_proc_fops);

	entry =
	    proc_create("slp_chk_mtcmos_pll_stat", S_IRUGO, clkmgr_dir,
			&slp_chk_mtcmos_pll_stat_proc_fops);

	entry = proc_create("armpll_ckdiv", S_IRUGO, clkmgr_dir, &armpll_ckdiv_proc_fops);
}

struct platform_device clkmgr_device = {
	.name = "CLK",
	.id   = -1,
	.dev  = {},
};

int clk_pm_restore_noirq(struct device *device)
{
	struct subsys *sys;

	sys = &syss[SYS_DIS];
	sys->state = sys->ops->get_state(sys);

	muxs[MT_MUX_MM].cnt = 1;
	plls[VENCPLL].cnt = 1;
/* es_flag = 0; */

	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_LARB0]);
	clk_set_force_on_locked(&clks[MT_CG_DISP0_SMI_COMMON]);

	clk_info("clk_pm_restore_noirq\n");

	return 0;
}

#ifdef CONFIG_PM
const struct dev_pm_ops clkmgr_pm_ops = {
	.restore_noirq = clk_pm_restore_noirq,
};
#endif
#ifdef CONFIG_OF
static const struct of_device_id mt_clkmgr_of_match[] = {
	{ .compatible = "mediatek,mt6735-apmixedsys", },
	{},
};
#endif

static struct platform_driver clkmgr_driver = {
	.driver = {
		.name = "CLK",
#ifdef CONFIG_PM
		    .pm = &clkmgr_pm_ops,
#endif
		    .owner = THIS_MODULE,
#ifdef CONFIG_OF
		    .of_match_table = mt_clkmgr_of_match,
#endif
},};


static int mt_clkmgr_debug_module_init(void)
{
	int ret;

	mt_clkmgr_debug_init();

#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&mt_clkmgr_early_suspend_handler);
#endif
#endif

	ret = platform_device_register(&clkmgr_device);
	if (ret) {
		clk_info("clkmgr_device register fail(%d)\n", ret);
		return ret;
	}

	ret = platform_driver_register(&clkmgr_driver);
	if (ret) {
		clk_info("clkmgr_driver register fail(%d)\n", ret);
		return ret;
	}

	return 0;
}

static int __init mt_clkmgr_late_init(void)
{
/* **** */
/*
	mt_enable_clock(MT_CG_DISP1_DPI_PIXEL, "clkmgr");
	mt_disable_clock(MT_CG_DISP1_DPI_PIXEL, "clkmgr");

	mt_enable_clock(MT_CG_IMAGE_LARB2_SMI, "clkmgr");
	mt_disable_clock(MT_CG_IMAGE_LARB2_SMI, "clkmgr");
	mt_enable_clock(MT_CG_VDEC0_VDEC, "clkmgr");
	mt_disable_clock(MT_CG_VDEC0_VDEC, "clkmgr");
	mt_enable_clock(MT_CG_VENC_LARB, "clkmgr");
	mt_disable_clock(MT_CG_VENC_LARB, "clkmgr");

	enable_mux(MT_MUX_AUD1, "clkmgr");
	disable_mux(MT_MUX_AUD1, "clkmgr");
	enable_mux(MT_MUX_AUD2, "clkmgr");
	disable_mux(MT_MUX_AUD2, "clkmgr");
	print_grp_regs();
*/
	return 0;
}
module_init(mt_clkmgr_debug_module_init);
late_initcall(mt_clkmgr_late_init);


void all_force_off(void)
{
/* **** */
#if 0

	clk_info("All force off\n");
	/* MTCMOS */
	spm_mtcmos_ctrl_mdsys1(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mdsys2(STA_POWER_DOWN);
	spm_mtcmos_ctrl_connsys(STA_POWER_DOWN);
	spm_mtcmos_ctrl_disp(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);
	spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
	spm_mtcmos_ctrl_vdec(STA_POWER_DOWN);
	spm_mtcmos_ctrl_venc(STA_POWER_DOWN);

	/* PLL */
	enable_pll(MSDCPLL, "clk");
	disable_pll(MSDCPLL, "clk");

	enable_pll(UNIVPLL, "clk");
	disable_pll(UNIVPLL, "clk");

	enable_pll(MMPLL, "clk");
	disable_pll(MMPLL, "clk");

	enable_pll(VENCPLL, "clk");
	disable_pll(VENCPLL, "clk");

	enable_pll(TVDPLL, "clk");
	disable_pll(TVDPLL, "clk");

	enable_pll(APLL1, "clk");
	disable_pll(APLL1, "clk");

	enable_pll(APLL2, "clk");
	disable_pll(APLL2, "clk");

/* mmpll */
	clk_clrl(MMPLL_CON0, 0x1);
	clk_setl(MMPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MMPLL_PWR_CON0, PLL_PWR_ON);

/* vencpll */
	clk_clrl(VENCPLL_CON0, 0x1);
	clk_setl(VENCPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(VENCPLL_PWR_CON0, PLL_PWR_ON);

/* UNIVPLL */
	clk_clrl(UNIVPLL_CON0, RST_BAR_MASK);
	clk_clrl(UNIVPLL_CON0, 0x1);
	clk_setl(UNIVPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(UNIVPLL_PWR_CON0, PLL_PWR_ON);

	clk_info("UNIVPLL_CON0=0x%x\n", clk_readl(UNIVPLL_CON0));
	clk_info("MMPLL_CON0=0x%x\n", clk_readl(MMPLL_CON0));
	clk_info("MSDCPLL_CON0=0x%x\n", clk_readl(MSDCPLL_CON0));
	clk_info("VENCPLL_CON0=0x%x\n", clk_readl(VENCPLL_CON0));
	clk_info("TVDPLL_CON0=0x%x\n", clk_readl(TVDPLL_CON0));
	clk_info("APLL1_CON0=0x%x\n", clk_readl(APLL1_CON0));
	clk_info("APLL2_CON0=0x%x\n", clk_readl(APLL2_CON0));

#endif
}
EXPORT_SYMBOL(all_force_off);

/*************CLKM****************/
#if 1
int clk_monitor_0(enum ckmon_sel ckmon, enum monitor_clk_sel_0 sel, int div)
{
	unsigned long flags;
	unsigned int temp;

	if ((div > 255) || (ckmon > 0)) {
		clk_info("CLK_Monitor_0 error parameter\n");
		return 1;
	}

	clkmgr_lock(flags);

	temp = clk_readl(CLK26CALI_0);
	clk_writel(CLK26CALI_0, temp | 0x80);

	clk_writel(CLK_CFG_8, sel << 8);

	temp = clk_readl(CLK_MISC_CFG_1);
	clk_writel(CLK_MISC_CFG_1, div & 0xff);

	clk_info("CLK_Monitor_0 Reg: CLK26CALI_0=0x%x, CLK_CFG_8=0x%x, CLK_MISC_CFG_1=0x%x\n",
		 clk_readl(CLK26CALI_0), clk_readl(CLK_CFG_8), clk_readl(CLK_MISC_CFG_1));

	clkmgr_unlock(flags);
	return 0;
}
EXPORT_SYMBOL(clk_monitor_0);

int clk_monitor(enum ckmon_sel ckmon, enum monitor_clk_sel sel, int div)
{
	unsigned long flags;
	unsigned int ckmon_shift = 0;
	unsigned int temp;

	if ((div > 255) || (ckmon == 0)) {
		clk_info("CLK_Monitor error parameter\n");
		return 1;
	}

	clkmgr_lock(flags);
#if 0
	if (ckmon == 1)
		ckmon_shift = 0;
	else if (ckmon == 2)
		ckmon_shift = 8;
	else if (ckmon == 3)
		ckmon_shift = 16;
#else
	ckmon_shift = (ckmon - 1) << 3;
#endif
	temp = clk_readl(CLK_CFG_10);
	temp = temp & (~(0xf << ckmon_shift));
	temp = temp | ((sel & 0xf) << ckmon_shift);
	clk_writel(CLK_CFG_10, temp);

	temp = clk_readl(CLK_CFG_11);
	temp = temp & (~(0xff << ckmon_shift));
	temp = temp | ((div & 0xff) << ckmon_shift);
	clk_writel(CLK_CFG_11, temp);

	clk_info("CLK_Monitor Reg: CLK_CFG_10=0x%x, CLK_CFG_11=0x%x\n", clk_readl(CLK_CFG_10),
		 clk_readl(CLK_CFG_11));

	clkmgr_unlock(flags);
	return 0;
}
EXPORT_SYMBOL(clk_monitor);
#endif
