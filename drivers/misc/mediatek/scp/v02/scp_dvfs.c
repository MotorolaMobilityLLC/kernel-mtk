/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <mt-plat/upmu_common.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include <linux/uaccess.h>
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_dvfs.h"

#ifndef CONFIG_MTK_CLKMGR
#include <linux/clk.h>
#endif

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#include "mtk_pmic_info.h"
/*
 * LOG
 */
#define TAG	"[Power/scp_dvfs] "

#define scp_dvfs_err(fmt, args...)	\
	pr_err(TAG"[ERROR]"fmt, ##args)
#define scp_dvfs_warn(fmt, args...)	\
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define scp_dvfs_info(fmt, args...)	\
	pr_warn(TAG""fmt, ##args)
#define scp_dvfs_dbg(fmt, args...)	\
	do {			\
		if (mt_scp_dvfs_debug)		\
			scp_dvfs_info(fmt, ##args);     \
	} while (0)
#define scp_dvfs_ver(fmt, args...)	\
	do {			\
		if (mt_scp_dvfs_debug)	\
			pr_debug(TAG""fmt, ##args);	\
	} while (0)

#define PLL_ENABLE				(1)
#define PLL_DISABLE			(0)
#define DVFS_STATUS_OK			(0)
#define DVFS_STATUS_BUSY			(-1)
#define DVFS_REQUEST_SAME_CLOCK		(-2)
#define DVFS_STATUS_ERR			(-3)
#define DVFS_STATUS_TIMEOUT		(-4)
#define DVFS_CLK_ERROR			(-5)
#define DVFS_STATUS_CMD_FIX		(-6)
#define DVFS_STATUS_CMD_LIMITED		(-7)
#define DVFS_STATUS_CMD_DISABLE		(-8)

#define TOPCK_BASE      0x10210000
/* PLL MUX CONTROL */
#define CLK_CFG_6               (TOPCK_BASE + 0x160)
#define CLK_CFG_6_SET           (TOPCK_BASE + 0x164)
#define CLK_CFG_6_CLR           (TOPCK_BASE + 0x168)

/* PLL MUX CONTROL FOR SCP */
#define CLK_TOP_SCP_SEL_MSK       0x7
#define CLK_TOP_SCP_SEL_SHFT      24

/* CLK_CTRL Registers */
#define CLK_SW_SEL		(*(volatile unsigned int *)(scpreg.clkctrl + 0x0000))  /* Clock Source Select */
#define CLK_ENABLE	(*(volatile unsigned int *)(scpreg.clkctrl + 0x0004))  /* Clock Source Enable */
#define CLK_DIV_SEL	(*(volatile unsigned int *)(scpreg.clkctrl + 0x0024))  /* Clock Divider Select */
#define SLEEP_DEBUG	(*(volatile unsigned int *)(scpreg.clkctrl + 0x0028))  /* Sleep mode debug signals */
#define CKSW_SEL_O	16
#define CKSW_DIV_SEL_O	16

#define EXPECTED_FREQ_REG SCP_GENERAL_REG3
#define CURRENT_FREQ_REG  SCP_GENERAL_REG4

#define DVFS_FIRST_VERSION

#define READ_REGISTER_UINT32(reg) \
	(*(volatile unsigned int * const)(reg))

#define WRITE_REGISTER_UINT32(reg, val) \
	((*(volatile unsigned int * const)(reg)) = (val))

#define INREG32(x)          READ_REGISTER_UINT32((unsigned int *)((void *)(x)))
#define OUTREG32(x, y)      WRITE_REGISTER_UINT32((unsigned int *)((void *)(x)), (unsigned int)(y))
#define SETREG32(x, y)      OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)      OUTREG32(x, INREG32(x)&~(y))
#define MASKREG32(x, y, z)  OUTREG32(x, (INREG32(x)&~(y))|(z))

#define DRV_Reg32(addr)             INREG32(addr)
#define DRV_WriteReg32(addr, data)  OUTREG32(addr, data)
#define DRV_SetReg32(addr, data)    SETREG32(addr, data)
#define DRV_ClrReg32(addr, data)    CLRREG32(addr, data)

/***************************
 * Operate Point Definition
 ****************************/
#if 1
static struct pinctrl *scp_pctrl; /* static pinctrl instance */
/* DTS state */
typedef enum tagDTS_GPIO_STATE {
	SCP_DTS_GPIO_STATE_DEFAULT = 0,
	SCP_DTS_VREQ_OFF,
	SCP_DTS_VREQ_ON,
	SCP_DTS_GPIO_STATE_MAX,	/* for array size */
} SCP_DTS_GPIO_STATE;

/* DTS state mapping name */
static const char *scp_state_name[SCP_DTS_GPIO_STATE_MAX] = {
	"default",
	"scp_gpio_off",
	"scp_gpio_on"
};
#endif
static bool mt_scp_dvfs_debug = true;
static unsigned int scp_cg;
static unsigned int scp_irq_en;
static unsigned int scp_irq_status;
static unsigned int scp_awake_request;
static unsigned int scp_state;
static unsigned int scp_cur_volt = -1;

static struct mt_scp_dvfs_table_info *mt_scp_dvfs_info;
static struct mt_scp_pll_t *mt_scp_pll;

unsigned int mt_abist_measure(int id)
{
	unsigned int val = 0, freq_val = 0;

	DRV_WriteReg32(0x10210520, 0x80);
	DRV_WriteReg32(0x10210414, 0x0);
	DRV_WriteReg32(0x10210210, 0x4040 | (id << 8));
	DRV_WriteReg32(0x10210520, 0x81);

	udelay(60);

	val = DRV_Reg32(0x10210524) & 0xffff;
	freq_val = val * 26 / 1024;

	return freq_val;
}

static void _mt_setup_scp_dvfs_table(int num)
{
	mt_scp_dvfs_info = kzalloc((num) * sizeof(struct mt_scp_dvfs_table_info), GFP_KERNEL);
	if (mt_scp_dvfs_info == NULL) {
		scp_dvfs_err("scp dvfs table memory allocation fail\n");
		return;
	}
}

unsigned int scp_get_dvfs_opp(void)
{
	return scp_cur_volt;
}

short  scp_set_pmic_vcore(unsigned int cur_freq)
{
	unsigned short ret_vc, ret_vs;
	short ret = 0;
	unsigned int ret_val[2];

#ifdef DVFS_FIRST_VERSION
	pmic_set_register_value(PMIC_RG_BUCK_VCORE_SSHUB_ON, 0);
	pmic_set_register_value(PMIC_RG_BUCK_VCORE_SSHUB_MODE, 0);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_ON, 0);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_MODE, 0);
	if (cur_freq > CLK_OPP3) {
		ret_vc = pmic_scp_set_vcore(900000);
		ret_vs = pmic_scp_set_vsram_vcore(1000000);
		scp_cur_volt = 0;
	} else if (cur_freq <= CLK_OPP3 && cur_freq > CLK_OPP2) {
		ret_vc = pmic_scp_set_vcore(900000);
		ret_vs = pmic_scp_set_vsram_vcore(1000000);
		scp_cur_volt = 1;
	} else if (cur_freq <= CLK_OPP2 && cur_freq > CLK_OPP1) {
		ret_vc = pmic_scp_set_vcore(800000);
		ret_vs = pmic_scp_set_vsram_vcore(900000);
		scp_cur_volt = 2;
	}  else if (cur_freq <= CLK_OPP1 && cur_freq > CLK_OPP0) {
		ret_vc = pmic_scp_set_vcore(800000);
		ret_vs = pmic_scp_set_vsram_vcore(900000);
		scp_cur_volt = 3;
	} else {
		ret_vc = pmic_scp_set_vcore(800000);
		ret_vs = pmic_scp_set_vsram_vcore(900000);
	}
	pmic_set_register_value(PMIC_RG_BUCK_VCORE_SSHUB_ON, 1);
	pmic_set_register_value(PMIC_RG_BUCK_VCORE_SSHUB_MODE, 1);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_ON, 1);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_MODE, 1);
	ret_val[0] = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_VOSEL);
	ret_val[1] = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_VOSEL);
	scp_dvfs_err("scp_cur_volt %d\n", scp_cur_volt);
	scp_dvfs_info("vcore vosel, vsram vosel = (0x%x, 0x%x)\n", ret_val[0], ret_val[1]);
#else
	if (cur_freq > CLK_OPP3) {
		ret_vc = pmic_scp_set_vcore(800000);
		ret_vs = pmic_scp_set_vsram_vcore(900000);
		scp_cur_volt = 0;
	} else if (cur_freq <= CLK_OPP3 && cur_freq > CLK_OPP2) {
		ret_vc = pmic_scp_set_vcore(750000);
		ret_vs = pmic_scp_set_vsram_vcore(850000);
		scp_cur_volt = 1;
	} else if (cur_freq <= CLK_OPP2 && cur_freq > CLK_OPP1) {
		ret_vc = pmic_scp_set_vcore(700000);
		ret_vs = pmic_scp_set_vsram_vcore(800000);
		scp_cur_volt = 2;
	}  else if (cur_freq <= CLK_OPP1 && cur_freq > CLK_OPP0) {
		ret_vc = pmic_scp_set_vcore(650000);
		ret_vs = pmic_scp_set_vsram_vcore(800000);
		scp_cur_volt = 3;
	} else {
		ret_vc = pmic_scp_set_vcore(568000);
		ret_vs = pmic_scp_set_vsram_vcore(800000);
	}
#endif
	if (ret_vc != 0 || ret_vs != 0) {
		ret = -1;
		scp_dvfs_err("scp vcore / vsram setting error, (%d, %d)", ret_vc, ret_vs);
	}

	return ret;
}

void pll_opp_switch(unsigned int opp)
{
	DRV_ClrReg32(CLK_CFG_6, CLK_TOP_SCP_SEL_MSK << CLK_TOP_SCP_SEL_SHFT);
	DRV_SetReg32(CLK_CFG_6, (opp & CLK_TOP_SCP_SEL_MSK) << CLK_TOP_SCP_SEL_SHFT);
}

int scp_pll_ctrl_set(unsigned int pll_ctrl_flag, unsigned int pll_sel)
{
	int ret = 0;

	/* scp_dvfs_info("flag = %d, sel = %d\n", pll_ctrl_flag, pll_sel); */
#if 1
	if (pll_ctrl_flag == PLL_ENABLE) {
		ret = clk_prepare_enable(mt_scp_pll->clk_mux);
		if (ret)
			scp_dvfs_err("scp dvfs cannot enable clk mux, %d\n", ret);
		scp_dvfs_err("[dvfs]clk pll set, (%d %d)", pll_ctrl_flag, pll_sel);
		switch (pll_sel) {
		case CLK_OPP0:
			ret = clk_set_parent(mt_scp_pll->clk_mux, mt_scp_pll->clk_pll0);
			break;
		case CLK_OPP1:
			ret = clk_set_parent(mt_scp_pll->clk_mux, mt_scp_pll->clk_pll6);
			break;
		case CLK_OPP2:
			ret = clk_set_parent(mt_scp_pll->clk_mux, mt_scp_pll->clk_pll1);
			break;
		case CLK_OPP3:
			ret = clk_set_parent(mt_scp_pll->clk_mux, mt_scp_pll->clk_pll2);
			break;
		case CLK_OPP4:
			ret = clk_set_parent(mt_scp_pll->clk_mux, mt_scp_pll->clk_pll4);
			break;
		default:
			break;
		}
	} else if (pll_ctrl_flag == PLL_DISABLE && pll_sel != CLK_OPP4)
		clk_disable_unprepare(mt_scp_pll->clk_mux);

#endif
	return ret;
}
void scp_pll_ctrl_handler(int id, void *data, unsigned int len)
{
	unsigned int *pll_ctrl_flag = (unsigned int *)data;
	unsigned int *pll_sel =  (unsigned int *) (data + 1);
	int ret = 0;

	ret = scp_pll_ctrl_set(*pll_ctrl_flag, *pll_sel);
}

void dvfs_debug_cb_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	if (*scp_data == 1)
		scp_dvfs_info("debug mode on\n");
	else
		scp_dvfs_info("debug mode off\n");
}

void fix_opp_set_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	mt_scp_dvfs_info->fix_clk = *scp_data;
}

void fix_opp_en_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	if (*scp_data == DVFS_STATUS_OK)
		mt_scp_dvfs_info->fix_clk_on = true;
	else
		mt_scp_dvfs_info->fix_clk_on = false;

}

void limit_opp_set_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	mt_scp_dvfs_info->limited_clk = *scp_data;
}

void limit_opp_en_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	if (*scp_data == DVFS_STATUS_OK)
		mt_scp_dvfs_info->limited_clk_on = true;
	else
		mt_scp_dvfs_info->limited_clk_on = false;
}

void dvfs_disable_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	if (*scp_data == DVFS_STATUS_OK)
		mt_scp_dvfs_info->scp_dvfs_disable = true;
	else
		mt_scp_dvfs_info->scp_dvfs_disable = false;
}

void dump_info_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	mt_scp_dvfs_info->cur_clk = scp_data[0];
	mt_scp_dvfs_info->fix_clk = scp_data[1];
	mt_scp_dvfs_info->fix_clk_on = scp_data[2];
	mt_scp_dvfs_info->limited_clk = scp_data[3];
	mt_scp_dvfs_info->limited_clk_on = scp_data[4];
	mt_scp_dvfs_info->cur_volt = scp_data[5];
	mt_scp_dvfs_info->scp_dvfs_disable = scp_data[6];
}

void dump_reg_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	scp_cg = scp_data[0];
	scp_irq_en = scp_data[1];
	scp_irq_status = scp_data[2];
	scp_awake_request = scp_data[3];
}

void scp_state_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	scp_state = *scp_data;
}

unsigned int get_clk_sw_select(void)
{
	int i, clk_val = 0;

	clk_val = CLK_SW_SEL >> CKSW_SEL_O;

	for (i = 0; i < 3; i++) {
		if (clk_val == 1 << i)
			break;
	}

	return i;
}

unsigned int get_clk_div_select(void)
{
	int i, div_val = 0;

	div_val = CLK_DIV_SEL >> CKSW_DIV_SEL_O;

	for (i = 0; i < CLK_DIV_UNKNOWN; i++) {
		if (div_val == 1 << i)
			break;
	}

	return i;
}

clk_opp_enum get_cur_clk(void)
{
	int cur_clk, ret;

	cur_clk = scp_get_dvfs_opp();
	switch (cur_clk) {
	case 0:
		ret = CLK_OPP4;
		break;
	case 1:
		ret = CLK_OPP3;
		break;
	case 2:
		ret = CLK_OPP2;
		break;
	case 3:
		ret = CLK_OPP1;
		break;
	case -1:
		ret = CLK_OPP0;
		break;
	default:
		ret = CLK_OPP0;
		break;
	}
	scp_dvfs_info("clk setting (%d, %d)\n", cur_clk, ret);
	return ret;
}

#ifdef CONFIG_PROC_FS
/*
 * PROC
 */

/***************************
 * show current debug status
 ****************************/
static int mt_scp_dvfs_debug_proc_show(struct seq_file *m, void *v)
{
	if (mt_scp_dvfs_debug)
		seq_puts(m, "scp dvfs debug enabled\n");
	else
		seq_puts(m, "scp dvfs debug disabled\n");

	return 0;
}

/***********************
 * enable debug message
 ************************/
static ssize_t mt_scp_dvfs_debug_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int debug = 0;

	if (kstrtouint(buffer, 0, &debug) == 0) {
		if (debug == 0)
			mt_scp_dvfs_debug = 0;
		else if (debug == 1)
			mt_scp_dvfs_debug = 1;
		else
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else {
		scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	}

	scp_ipi_send(IPI_DVFS_DEBUG, (void *)&mt_scp_dvfs_debug, sizeof(mt_scp_dvfs_debug), 0, SCP_A_ID);
	scp_ipi_send(IPI_DVFS_DEBUG, (void *)&mt_scp_dvfs_debug, sizeof(mt_scp_dvfs_debug), 0, SCP_B_ID);
	return count;
}

/****************************
 * show limited clock frequency
 *****************************/
static int mt_scp_dvfs_limited_opp_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "limit frequency = %d\n", mt_scp_dvfs_info->limited_clk);

	return 0;
}

/**********************************
 * write limited clock frequency
 ***********************************/
static ssize_t mt_scp_dvfs_limited_opp_proc_write(struct file *file,
				const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int limited_clk = 0;

	if (kstrtouint(buffer, 0, &limited_clk) == 0)
		scp_ipi_send(IPI_DVFS_LIMIT_OPP_SET, (void *)&limited_clk, sizeof(limited_clk), 0, SCP_A_ID);
	else
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}

/****************************
 * show limited clock frequency enable
 *****************************/
static int mt_scp_dvfs_limited_opp_on_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "limit frequency = %d\n", mt_scp_dvfs_info->limited_clk);

	return 0;
}
/**********************************
 * write limited clock frequency enable
 ***********************************/
static ssize_t mt_scp_dvfs_limited_opp_on_proc_write(struct file *file,
					const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int limited_clk_on = 0;

	if (kstrtouint(buffer, 0, &limited_clk_on) == 0)
		scp_ipi_send(IPI_DVFS_LIMIT_OPP_EN, (void *)&limited_clk_on, sizeof(limited_clk_on), 0, SCP_A_ID);
	else
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}

/****************************
 * show current clock frequency
 *****************************/
static int mt_scp_dvfs_cur_opp_proc_show(struct seq_file *m, void *v)
{
	clk_opp_enum cur_clk = FREQ_UNKNOWN;

	cur_clk = get_cur_clk();
	seq_printf(m, "current opp = %s\n",
		(cur_clk == CLK_OPP0) ? "CLK_104M, Voltage = 0.8V"
		: (cur_clk == CLK_OPP1) ? "CLK_187M, Voltage = 0.8V"
		: (cur_clk == CLK_OPP2) ? "CLK_286M, Volt = 0.8V"
		: (cur_clk == CLK_OPP3) ? "CLK_330M, Volt = 0.9V"
		: (cur_clk == CLK_OPP4) ? "CLK_416M, Volt = 0.9V"
		: "state error");
	return 0;
}

/****************************
 * show fixed clock frequency
 *****************************/
static int mt_scp_dvfs_fix_opp_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "fixed frequency set = %d\n", mt_scp_dvfs_info->fix_clk);

	return 0;
}
/**********************************
 * write fixed clock frequency
 ***********************************/
static ssize_t mt_scp_dvfs_fix_opp_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int fix_clk = 0;

	if (kstrtouint(buffer, 0, &fix_clk) == 0)
		scp_ipi_send(IPI_DVFS_FIX_OPP_SET, (void *)&fix_clk, sizeof(fix_clk), 0, SCP_A_ID);
	else
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}

/****************************
 * show fixed clock frequency enable
 *****************************/
static int mt_scp_dvfs_fix_opp_on_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "limit frequency = %d\n", mt_scp_dvfs_info->fix_clk);

	return 0;
}
/**********************************
 * write fixed clock frequency enable
 ***********************************/
static ssize_t mt_scp_dvfs_fix_opp_on_proc_write(struct file *file,
					const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int fix_clk_on = 0;

	if (kstrtouint(buffer, 0, &fix_clk_on) == 0)
		scp_ipi_send(IPI_DVFS_FIX_OPP_EN, (void *)&fix_clk_on, sizeof(fix_clk_on), 0, SCP_A_ID);
	else
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}

/****************************
 * show current vcore voltage
 *****************************/
 #if 0
static int mt_scp_dvfs_cur_volt_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "limit frequency = %d\n", mt_scp_dvfs_info->cur_clk);

	return 0;
}

/**********************************
 * write current vcore voltage
 ***********************************/
static ssize_t mt_scp_dvfs_current_volt_proc_write(struct file *file,
				const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int cur_volt = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &cur_volt) == 0)
		set_scp_dvfs_cur_volt(cur_volt);
	else
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}
#endif
/****************************
 * show limited clock frequency enable
 *****************************/
static int mt_scp_dvfs_state_proc_show(struct seq_file *m, void *v)
{
	unsigned int scp_state;

	scp_state = SLEEP_DEBUG;
	seq_printf(m, "scp status is in %s\n",
		((scp_state & IN_DEBUG_IDLE) == IN_DEBUG_IDLE) ? "idle mode"
		: ((scp_state & ENTERING_SLEEP) == ENTERING_SLEEP) ? "enter sleep"
		: ((scp_state & IN_SLEEP) == IN_SLEEP) ? "sleep mode"
		: ((scp_state & ENTERING_ACTIVE) == ENTERING_ACTIVE) ? "enter active"
		: ((scp_state & IN_ACTIVE) == IN_ACTIVE) ? "active mode" : "none of state");
	return 0;
}

/****************************
 * show current clock frequency
 *****************************/
#if 0
static int mt_scp_dvfs_info_dump_proc_show(struct seq_file *m, void *v)
{
	scp_ipi_send(IPI_DVFS_INFO_DUMP, NULL, 0, 0);
	msleep(20);
	seq_printf(m,
	" cur_clk = %d, fix_clk = %d, fix_clk_on = %d, limited_clk = %d, limited_clk_on = %d, cur_volt = %d, scp_dvfs_disable = %d\n",
		mt_scp_dvfs_info->cur_clk, mt_scp_dvfs_info->fix_clk, mt_scp_dvfs_info->fix_clk_on,
		mt_scp_dvfs_info->limited_clk, mt_scp_dvfs_info->limited_clk_on, mt_scp_dvfs_info->cur_volt,
		mt_scp_dvfs_info->scp_dvfs_disable);
	return 0;
}
#endif
/****************************
 * show scp dvfs disable
 *****************************/
static int mt_scp_dvfs_sleep_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "scp sleep state = %d\n", mt_scp_dvfs_info->scp_dvfs_sleep);

	return 0;
}

/**********************************
 * write scp dvfs disable
 ***********************************/
static ssize_t mt_scp_dvfs_sleep_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int on = 0, scp_id = SCP_A_ID;

	if (kstrtouint(buffer, 0, &on) == 0) {
		if (on == 0) {
			mt_scp_dvfs_info->scp_dvfs_sleep = 0;
			scp_dvfs_warn("scp_dvfs_sleep = 0\n");
		} else if (on == 1) {
			mt_scp_dvfs_info->scp_dvfs_sleep = 1;
			scp_dvfs_warn("scp_dvfs_sleep = 1\n");
		} else {
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
		}
		if (kstrtouint(buffer + 1, 0, &scp_id) == 0) {
			if (scp_id == SCP_A_ID)
				scp_ipi_send(IPI_DVFS_SLEEP, (void *)&(mt_scp_dvfs_info->scp_dvfs_sleep),
					sizeof(mt_scp_dvfs_info->scp_dvfs_sleep), 0, SCP_A_ID);
			else if (scp_id == SCP_B_ID)
				scp_ipi_send(IPI_DVFS_SLEEP, (void *)&(mt_scp_dvfs_info->scp_dvfs_sleep),
					sizeof(mt_scp_dvfs_info->scp_dvfs_sleep), 0, SCP_B_ID);
			else
				scp_dvfs_err("scp_id input error, %d\n", scp_id);
		} else {
			scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");

		}
	} else {
		mt_scp_dvfs_info->scp_dvfs_sleep = 0;
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");
	}
	return count;
}

/****************************
 * show scp dvfs wake up
 *****************************/
static int mt_scp_dvfs_wake_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "scp sleep state = %d\n", mt_scp_dvfs_info->scp_dvfs_wake);

	return 0;
}

/**********************************
 * write scp dvfs wake up
 ***********************************/
static ssize_t mt_scp_dvfs_wake_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int on = 0;

	if (kstrtouint(buffer, 0, &on) == 0) {
		if (on == 0)
			mt_scp_dvfs_info->scp_dvfs_wake = 0;
		else if (on == 1)
			mt_scp_dvfs_info->scp_dvfs_wake = 1;
		else
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

		scp_ipi_send(IPI_DVFS_WAKE, (void *)&(mt_scp_dvfs_info->scp_dvfs_wake),
				sizeof(mt_scp_dvfs_info->scp_dvfs_wake), 0, SCP_A_ID);
	} else {
		mt_scp_dvfs_info->scp_dvfs_wake = 0;
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");
	}
	return count;
}

/****************************
 * show scp dvfs disable
 *****************************/
static int mt_scp_dvfs_disable_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "scp disable state = %d\n", mt_scp_dvfs_info->scp_dvfs_disable);

	return 0;
}

/**********************************
 * write scp dvfs disable
 ***********************************/
static ssize_t mt_scp_dvfs_disable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	unsigned int on = 0;

	if (kstrtouint(buffer, 0, &on) == 0) {
		if (on == 0)
			mt_scp_dvfs_info->scp_dvfs_disable = 0;
		else if (on == 1)
			mt_scp_dvfs_info->scp_dvfs_disable = 1;
		else
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

		scp_ipi_send(IPI_DVFS_DISABLE, (void *)&(mt_scp_dvfs_info->scp_dvfs_disable),
				sizeof(mt_scp_dvfs_info->scp_dvfs_disable), 0, SCP_A_ID);
	} else {
		mt_scp_dvfs_info->scp_dvfs_disable = 0;
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");
	}
	return count;
}

#define PROC_FOPS_RW(name)							\
	static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {		\
	.owner		  = THIS_MODULE,				\
	.open		   = mt_ ## name ## _proc_open,	\
	.read		   = seq_read,					\
	.llseek		 = seq_lseek,					\
	.release		= single_release,				\
	.write		  = mt_ ## name ## _proc_write,				\
}

#define PROC_FOPS_RO(name)							\
	static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {		\
	.owner		  = THIS_MODULE,				\
	.open		   = mt_ ## name ## _proc_open,	\
	.read		   = seq_read,					\
	.llseek		 = seq_lseek,					\
	.release		= single_release,				\
}

#define PROC_ENTRY(name)	{__stringify(name), &mt_ ## name ## _proc_fops}

PROC_FOPS_RW(scp_dvfs_debug);
PROC_FOPS_RW(scp_dvfs_limited_opp);
PROC_FOPS_RW(scp_dvfs_limited_opp_on);
PROC_FOPS_RO(scp_dvfs_state);
PROC_FOPS_RO(scp_dvfs_cur_opp);
PROC_FOPS_RW(scp_dvfs_fix_opp);
PROC_FOPS_RW(scp_dvfs_fix_opp_on);
PROC_FOPS_RW(scp_dvfs_sleep);
PROC_FOPS_RW(scp_dvfs_wake);
PROC_FOPS_RW(scp_dvfs_disable);

static int mt_scp_dvfs_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(scp_dvfs_debug),
		PROC_ENTRY(scp_dvfs_limited_opp),
		PROC_ENTRY(scp_dvfs_limited_opp_on),
		PROC_ENTRY(scp_dvfs_state),
		PROC_ENTRY(scp_dvfs_cur_opp),
		PROC_ENTRY(scp_dvfs_fix_opp),
		PROC_ENTRY(scp_dvfs_fix_opp_on),
		PROC_ENTRY(scp_dvfs_disable),
		PROC_ENTRY(scp_dvfs_sleep),
		PROC_ENTRY(scp_dvfs_wake),
	};


	dir = proc_mkdir("scp_dvfs", NULL);

	if (!dir) {
		scp_dvfs_err("fail to create /proc/scp_dvfs @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			scp_dvfs_err("@%s: create /proc/scp_dvfs/%s failed\n", __func__, entries[i].name);
	}

	return 0;
}
#endif /* CONFIG_PROC_FS */

#if 1
/* pinctrl implementation */
static long _set_state(const char *name)
{
	long ret = 0;
	struct pinctrl_state *pState = 0;

	WARN_ON(!scp_pctrl);

	pState = pinctrl_lookup_state(scp_pctrl, name);
	if (IS_ERR(pState)) {
		pr_err("lookup state '%s' failed\n", name);
		ret = PTR_ERR(pState);
		goto exit;
	}

	/* select state! */
	pinctrl_select_state(scp_pctrl, pState);

exit:
	return ret; /* Good! */
}
#endif
static const struct of_device_id scpdvfs_of_ids[] = {
	{.compatible = "mediatek,mt6799-scpdvfs",},
	{}
};

static int mt_scp_dvfs_suspend(struct device *dev)
{
#if 1
	scp_dvfs_dbg("set scp gpio to audio clk on\n");
#endif
	return 0;
}

static int mt_scp_dvfs_resume(struct device *dev)
{
#if 1
	scp_dvfs_dbg("set scp gpio to audio clk off\n");
#endif
	return 0;
}
static int mt_scp_dvfs_pm_restore_early(struct device *dev)
{
	return 0;
}
static int mt_scp_dvfs_pdrv_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *node;

	scp_dvfs_warn("scp dvfs driver probe @ %s()\n", __func__);
	node = of_find_matching_node(NULL, scpdvfs_of_ids);
	if (!node)
		dev_err(&pdev->dev, "@%s: find SCPDVFS node failed\n", __func__);
	/* retrieve */
	mt_scp_pll = kzalloc(sizeof(struct mt_scp_pll_t), GFP_KERNEL);
	if (mt_scp_pll == NULL)
		return -ENOMEM;
	mt_scp_pll->clk_mux = devm_clk_get(&pdev->dev, "clk_mux");
	if (IS_ERR(mt_scp_pll->clk_mux)) {
		dev_err(&pdev->dev, "cannot get clock mux\n");
		return PTR_ERR(mt_scp_pll->clk_mux);
	}

	mt_scp_pll->clk_pll0 = devm_clk_get(&pdev->dev, "clk_pll_0");
	if (IS_ERR(mt_scp_pll->clk_pll0)) {
		dev_err(&pdev->dev, "cannot get 1st clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll0);
	}
	mt_scp_pll->clk_pll1 = devm_clk_get(&pdev->dev, "clk_pll_1");
	if (IS_ERR(mt_scp_pll->clk_pll1)) {
		dev_err(&pdev->dev, "cannot get 2nd clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll1);
	}
	mt_scp_pll->clk_pll2 = devm_clk_get(&pdev->dev, "clk_pll_2");
	if (IS_ERR(mt_scp_pll->clk_pll2)) {
		dev_err(&pdev->dev, "cannot get 3rd clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll2);
	}
	mt_scp_pll->clk_pll3 = devm_clk_get(&pdev->dev, "clk_pll_3");
	if (IS_ERR(mt_scp_pll->clk_pll3)) {
		dev_err(&pdev->dev, "cannot get 4th clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll3);
	}
	mt_scp_pll->clk_pll4 = devm_clk_get(&pdev->dev, "clk_pll_4");
	if (IS_ERR(mt_scp_pll->clk_pll4)) {
		dev_err(&pdev->dev, "cannot get 5th clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll4);
	}
	mt_scp_pll->clk_pll5 = devm_clk_get(&pdev->dev, "clk_pll_5");
	if (IS_ERR(mt_scp_pll->clk_pll5)) {
		dev_err(&pdev->dev, "cannot get 6th clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll5);
	}
	mt_scp_pll->clk_pll6 = devm_clk_get(&pdev->dev, "clk_pll_6");
	if (IS_ERR(mt_scp_pll->clk_pll6)) {
		dev_err(&pdev->dev, "cannot get 7th clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll6);
	}
	mt_scp_pll->clk_pll7 = devm_clk_get(&pdev->dev, "clk_pll_7");
	if (IS_ERR(mt_scp_pll->clk_pll7)) {
		dev_err(&pdev->dev, "cannot get 8th clock parent\n");
		return PTR_ERR(mt_scp_pll->clk_pll7);
	}
#if 1
	scp_pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(scp_pctrl)) {
		dev_err(&pdev->dev, "Cannot find scp pinctrl!\n");
		ret = PTR_ERR(scp_pctrl);
		goto exit;
	}
#endif
	_set_state(scp_state_name[SCP_DTS_VREQ_ON]);
	return 0;
#if 1
exit:
	return ret;
#endif
}

/***************************************
 * this function should never be called
 ****************************************/
static int mt_scp_dvfs_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct dev_pm_ops mt_scp_dvfs_pm_ops = {
	.suspend = mt_scp_dvfs_suspend,
	.resume = mt_scp_dvfs_resume,
	.restore_early = mt_scp_dvfs_pm_restore_early,
};

struct platform_device mt_scp_dvfs_pdev = {
	.name = "mt-scpdvfs",
	.id = -1,
};

static struct platform_driver mt_scp_dvfs_pdrv = {
	.probe = mt_scp_dvfs_pdrv_probe,
	.remove = mt_scp_dvfs_pdrv_remove,
	.driver = {
		.name = "scpdvfs",
		.pm = &mt_scp_dvfs_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = scpdvfs_of_ids,
		},
};
/**********************************
 * mediatek scp dvfs initialization
 ***********************************/
void mt_pmic_sshub_init(void)
{
	unsigned int ret[6];

	ret[0] = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_ON);
	ret[1] = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_MODE);
	ret[2] = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_ON);
	ret[3] = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_MODE);
	ret[4] = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_VOSEL);
	ret[5] = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_VOSEL);
	scp_dvfs_info("vcore on, mode, vsram on, mode = (0x%x, 0x%x, 0x%x, 0x%x)\n", ret[0], ret[1], ret[2], ret[3]);
	scp_dvfs_info("vcore vosel, vsram vosel = (0x%x, 0x%x)\n", ret[4], ret[5]);
	pmic_scp_set_vcore(800000);
	pmic_scp_set_vsram_vcore(900000);
	pmic_set_register_value(PMIC_RG_BUCK_VCORE_SSHUB_ON, 1);
	pmic_set_register_value(PMIC_RG_BUCK_VCORE_SSHUB_MODE, 1);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_ON, 1);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_MODE, 1);
	ret[0] = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_ON);
	ret[1] = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_MODE);
	ret[2] = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_ON);
	ret[3] = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_MODE);
	ret[4] = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_VOSEL);
	ret[5] = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_VOSEL);
	scp_dvfs_info("vcore on, mode, vsram on, mode = (0x%x, 0x%x, 0x%x, 0x%x)\n", ret[0], ret[1], ret[2], ret[3]);
	scp_dvfs_info("vcore vosel, vsram vosel = (0x%x, 0x%x)\n", ret[4], ret[5]);
}

void mt_scp_dvfs_ipi_init(void)
{
#if 0
	scp_ipi_registration(IPI_DVFS_DEBUG, dvfs_debug_cb_handler, "IPI_DVFS_DEBUG");
	scp_ipi_registration(IPI_DVFS_FIX_OPP_SET, fix_opp_set_handler, "IPI_DVFS_FIX_OPP_SET");
	scp_ipi_registration(IPI_DVFS_FIX_OPP_EN, fix_opp_en_handler, "IPI_DVFS_FIX_OPP_EN");
	scp_ipi_registration(IPI_DVFS_LIMIT_OPP_SET, limit_opp_set_handler, "IPI_DVFS_LIMIT_OPP_SET");
	scp_ipi_registration(IPI_DVFS_LIMIT_OPP_EN, limit_opp_en_handler, "IPI_DVFS_LIMIT_OPP_EN");
	scp_ipi_registration(IPI_DVFS_DISABLE, dvfs_disable_handler, "IPI_DVFS_DISABLE");
	/* scp_ipi_registration(IPI_DVFS_INFO_DUMP, dump_info_handler, "IPI_DVFS_INFO_DUMP"); */
	scp_ipi_registration(IPI_DUMP_REG, dump_reg_handler, "IPI_DUMP_REG");
	scp_ipi_registration(IPI_SCP_STATE, scp_state_handler, "IPI_SCP_STATE");
#endif
	scp_ipi_registration(IPI_SCP_PLL_CTRL, scp_pll_ctrl_handler, "IPI_SCP_PLL_CTRL");
	_mt_setup_scp_dvfs_table(1);
}

static int __init _mt_scp_dvfs_init(void)
{
	int ret = 0;

	scp_dvfs_info("@%s\n", __func__);

#ifdef CONFIG_PROC_FS

	/* init proc */
	if (mt_scp_dvfs_create_procfs())
		goto out;

#endif /* CONFIG_PROC_FS */

	/* register platform device/driver */
	ret = platform_device_register(&mt_scp_dvfs_pdev);
	if (ret) {
		scp_dvfs_err("fail to register scp dvfs device @ %s()\n", __func__);
		goto out;
	}

	ret = platform_driver_register(&mt_scp_dvfs_pdrv);
	if (ret) {
		scp_dvfs_err("fail to register scp dvfs driver @ %s()\n", __func__);
		platform_device_unregister(&mt_scp_dvfs_pdev);
		goto out;
	}
	mt_scp_dvfs_ipi_init();
	mt_pmic_sshub_init();
out:
	return ret;

}

static void __exit _mt_scp_dvfs_exit(void)
{
	platform_driver_unregister(&mt_scp_dvfs_pdrv);
	platform_device_unregister(&mt_scp_dvfs_pdev);
}

module_init(_mt_scp_dvfs_init);
module_exit(_mt_scp_dvfs_exit);

MODULE_DESCRIPTION("MediaTek SCP Freqency Scaling AP driver");
MODULE_LICENSE("GPL");
