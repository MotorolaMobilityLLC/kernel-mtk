/*
 * Copyright (C) 2011-2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
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
#ifdef wakelock
#include <linux/wakelock.h>
#endif
#include <linux/io.h>
//#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_secure_api.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include <linux/uaccess.h>
#include "adsp_ipi.h"
#include "adsp_helper.h"
#include "adsp_excep.h"
#include "adsp_dvfs.h"
#include "adsp_clk.h"
#include "adsp_reg.h"

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)     "[adsp_dvfs]: " fmt

#define ADSP_DBG(fmt, arg...) pr_debug(fmt, ##arg)
#define ADSP_INFO(fmt, arg...) pr_info(fmt, ##arg)

#define DRV_Reg32(addr)           readl(addr)
#define DRV_WriteReg32(addr, val) writel(val, addr)
#define DRV_SetReg32(addr, val)   DRV_WriteReg32(addr, DRV_Reg32(addr) | (val))
#define DRV_ClrReg32(addr, val)   DRV_WriteReg32(addr, DRV_Reg32(addr) & ~(val))

#if ADSP_DVFS_ENABLE

/***************************
 * Operate Point Definition
 ****************************/
/* DTS state */
enum ADSP_DTS_GPIO_STATE {
	ADSP_DTS_GPIO_STATE_DEFAULT = 0,
	ADSP_DTS_VREQ_OFF,
	ADSP_DTS_VREQ_ON,
	ADSP_DTS_GPIO_STATE_MAX,        /* for array size */
};


/* -1:ADSP DVFS OFF, 1:ADSP DVFS ON */
static int adsp_dvfs_flag = -1;

/*
 * 0: ADSP Sleep: OFF,
 * 1: ADSP Sleep: ON,
 * 2: ADSP Sleep: sleep without wakeup,
 * 3: ADSP Sleep: force to sleep
 */
static int adsp_sleep_flag = -1;

static int mt_adsp_dvfs_debug = -1;
static int adsp_cur_volt = -1;
static int pre_pll_sel = -1;
static struct mt_adsp_pll_t *mt_adsp_pll;
#ifdef wakelock
static struct wake_lock adsp_suspend_lock;
#endif
static int g_adsp_dvfs_init_flag = -1;

static unsigned int pre_feature_req = 0xff;

static void __iomem *gpio_base;
#define RG_GPIO154_MODE         (gpio_base + 0x430)
#define GPIO154_BIT                     8
#define GPIO154_MASK            0x7

unsigned int adsp_get_dvfs_opp(void)
{
	return (unsigned int)adsp_cur_volt;
}
uint32_t adsp_get_freq(void)
{
	uint32_t i;

	uint32_t sum = 0;
	uint32_t return_freq = 0;

	/*
	 * calculate adsp frequence
	 */
	for (i = 0; i < ADSP_NUM_FEATURE_ID; i++) {
		if (feature_table[i].counter == 1)
			sum += feature_table[i].freq;
	}
	/*
	 * calculate adsp sensor frequence
	 */
	for (i = 0; i < NUM_SENSOR_TYPE; i++) {
		if (sensor_type_table[i].enable == 1)
			sum += sensor_type_table[i].freq;
	}

	/*pr_debug("[ADSP] needed freq sum:%d\n",sum);*/
	if (sum <= CLK_OPP0)
		return_freq = CLK_OPP0;
	else if (sum <= CLK_OPP1)
		return_freq = CLK_OPP1;
	else if (sum <= CLK_OPP2)
		return_freq = CLK_OPP2;
	else {
		return_freq = CLK_OPP2;
		ADSP_DBG("warning: request freq %d > max opp %d\n",
			 sum, CLK_OPP2);
	}

	return return_freq;
}

void adsp_vcore_request(unsigned int clk_opp)
{
	/* Set PMIC */
	adsp_set_pmic_vcore(clk_opp);
#ifdef Liang_Check
	/* set DVFSRC_VCORE_REQUEST [31:30] */
	if (adsp_expected_freq == CLK_OPP2)
		dvfsrc_set_adsp_vcore_request(0x1);
	else
		dvfsrc_set_adsp_vcore_request(0x0);

	/* Modify adsp reg: 0xC0094 [7:0] */
	//DRV_ClrReg32(ADSP_ADSP2SPM_VOL_LV, 0xff);
	if (clk_opp == CLK_OPP0)
		//      DRV_SetReg32(ADSP_ADSP2SPM_VOL_LV, 0);
		else if (clk_opp == CLK_OPP1)
			//      DRV_SetReg32(ADSP_ADSP2SPM_VOL_LV, 1);
			else
				//      DRV_SetReg32(ADSP_ADSP2SPM_VOL_LV, 2);
#endif
}

/* adsp_request_freq
 * return :-1 means the adsp request freq. error
 * return :0  means the request freq. finished
 */
int adsp_request_freq(void)
{
	int value = 0;
	int timeout = 250;
	int ret = 0;
	unsigned long spin_flags;
	int is_increasing_freq = 0;

	if (adsp_dvfs_flag != 1) {
		ADSP_DBG("warning: ADSP DVFS is OFF\n");
		return 0;
	}

	/* because we are waiting for adsp to update register:adsp_current_freq
	 * use wake lock to prevent AP from entering suspend state
	 */
#ifdef wakelock
	wake_lock(&adsp_suspend_lock);
#endif
	if (adsp_current_freq != adsp_expected_freq) {

		/* do DVS before DFS if increasing frequency */
		if (adsp_current_freq < adsp_expected_freq) {
			adsp_vcore_request(adsp_expected_freq);
			is_increasing_freq = 1;
		}

#if ADSP_DVFS_USE_PLL
		/*  turn on PLL */
		adsp_pll_ctrl_set(PLL_ENABLE, adsp_expected_freq);
#endif

		do {
			ret = adsp_ipi_send(IPI_DVFS_SET_FREQ, (void *)&value,
					    sizeof(value), 0, ADSP_A_ID);
			if (ret != ADSP_IPI_DONE)
				ADSP_DBG("%s: ADSP send IPI fail - %d\n",
					 __func__, ret);

			mdelay(2);
			timeout -= 1; /* try 50 times, total about 100ms */
			if (timeout <= 0) {
				pr_err("%s: fail, current(%d) != expect(%d)\n",
				       __func__, adsp_current_freq,
				       adsp_expected_freq);
#ifdef wakelock
				wake_unlock(&adsp_suspend_lock);
#endif
				WARN_ON(1);
				return -1;
			}

			/* read adsp_current_freq again */
			spin_lock_irqsave(&adsp_awake_spinlock, spin_flags);
			adsp_current_freq = readl(ADSP_CURRENT_FREQ_REG);
			spin_unlock_irqrestore(&adsp_awake_spinlock,
					       spin_flags);

		} while (adsp_current_freq != adsp_expected_freq);

#if ADSP_DVFS_USE_PLL
		/* turn off PLL */
		adsp_pll_ctrl_set(PLL_DISABLE, adsp_expected_freq);
#endif

		/* do DVS after DFS if decreasing frequency */
		if (is_increasing_freq == 0)
			adsp_vcore_request(adsp_expected_freq);
	}
#ifdef wakelock
	wake_unlock(&adsp_suspend_lock);
#endif
	ADSP_DBG("[ADSP] set freq OK, %d == %d\n", adsp_expected_freq,
		 adsp_current_freq);
	return 0;
}

void wait_adsp_dvfs_init_done(void)
{
	int count = 0;

	while (g_adsp_dvfs_init_flag != 1) {
		mdelay(1);
		count++;
		if (count > 3000) {
			pr_err("ERROR: %s: ADSP dvfs driver init fail\n",
			       __func__);
			WARN_ON(1);
		}
	}
}

void adsp_pll_mux_set(unsigned int pll_ctrl_flag)
{
	int ret = 0;

	ADSP_DBG("%s(%d)\n\n", __func__, pll_ctrl_flag);

	if (pll_ctrl_flag == PLL_ENABLE) {
		ret = clk_prepare_enable(mt_adsp_pll->clk_mux);
		if (ret) {
			pr_err("EEROR:%s adsp dvfs cannot enable clk mux, %d\n",
			       __func__, ret);
			WARN_ON(1);
		}
	} else
		clk_disable_unprepare(mt_adsp_pll->clk_mux);
}

int adsp_pll_ctrl_set(unsigned int pll_ctrl_flag, unsigned int pll_sel)
{
	int ret = 0;

	ADSP_DBG("%s(%d, %d)\n", __func__, pll_ctrl_flag, pll_sel);

	if (pll_ctrl_flag == PLL_ENABLE) {
		if (pre_pll_sel != CLK_OPP2) {
			ret = clk_prepare_enable(mt_adsp_pll->clk_mux);
			if (ret) {
				pr_err("%s: clk_prepare_enable() failed, %d\n",
				       __func__, ret);
				WARN_ON(1);
			}
		} else {
			ADSP_DBG("no need to do clk_prepare_enable()\n");
		}

		switch (pll_sel) {
		case CLK_OPP0:
			ret = clk_set_parent(mt_adsp_pll->clk_mux,
					     mt_adsp_pll->clk_pll0);
			break;
		case CLK_OPP1:
			ret = clk_set_parent(mt_adsp_pll->clk_mux,
					     mt_adsp_pll->clk_pll5);
			break;
		case CLK_OPP2:
			ret = clk_set_parent(mt_adsp_pll->clk_mux,
					     mt_adsp_pll->clk_pll6);
			break;
		default:
			pr_err("ERROR: %s: not support opp freq %d\n",
			       __func__, pll_sel);
			WARN_ON(1);
			break;
		}

		if (ret) {
			ADSP_DBG("ERROR: %s: clk_set_parent() failed, opp=%d\n",
				 __func__, pll_sel);
			WARN_ON(1);
		}

		if (pre_pll_sel != pll_sel)
			pre_pll_sel = pll_sel;

	} else if (pll_ctrl_flag == PLL_DISABLE && pll_sel != CLK_OPP2) {
		clk_disable_unprepare(mt_adsp_pll->clk_mux);
		ADSP_DBG("clk_disable_unprepare()\n");
	} else {
		ADSP_DBG("no need to do clk_disable_unprepare\n");
	}

	return ret;
}

void adsp_pll_ctrl_handler(int id, void *data, unsigned int len)
{
	unsigned int *pll_ctrl_flag = (unsigned int *)data;
	unsigned int *pll_sel = (unsigned int *)(data + 1);
	int ret = 0;

	ret = adsp_pll_ctrl_set(*pll_ctrl_flag, *pll_sel);
}

#ifdef CONFIG_PROC_FS
/*
 * PROC
 */

/***************************
 * show current debug status
 ****************************/
static int mt_adsp_dvfs_debug_proc_show(struct seq_file *m, void *v)
{
	if (mt_adsp_dvfs_debug == -1)
		seq_puts(m, "mt_adsp_dvfs_debug has not been set\n");
	else
		seq_printf(m, "mt_adsp_dvfs_debug = %d\n", mt_adsp_dvfs_debug);

	return 0;
}

/***********************
 * enable debug message
 ************************/
static ssize_t mt_adsp_dvfs_debug_proc_write(struct file *file,
					     const char __user *buffer,
					     size_t count, loff_t *data)
{
	char desc[64];
	unsigned int debug = 0;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &debug) == 0) {
		if (debug == 0)
			mt_adsp_dvfs_debug = 0;
		else if (debug == 1)
			mt_adsp_dvfs_debug = 1;
		else
			ADSP_INFO("bad argument %d\n", debug);
	} else {
		ADSP_INFO("invalid command!\n");
	}

	adsp_ipi_send(IPI_DVFS_DEBUG, (void *)&mt_adsp_dvfs_debug,
		      sizeof(mt_adsp_dvfs_debug), 0, ADSP_A_ID);

	return count;
}

/****************************
 * show ADSP state
 *****************************/
static int mt_adsp_dvfs_state_proc_show(struct seq_file *m, void *v)
{
	unsigned int adsp_state;

	adsp_state = readl(ADSP_A_SLEEP_DEBUG_REG);
	seq_printf(m, "adsp status: %s\n",
		   ((adsp_state & IN_DEBUG_IDLE) == IN_DEBUG_IDLE)
		   ? "idle mode"
		   : ((adsp_state & ENTERING_SLEEP) == ENTERING_SLEEP)
		   ? "enter sleep"
		   : ((adsp_state & IN_SLEEP) == IN_SLEEP)
		   ? "sleep mode"
		   : ((adsp_state & ENTERING_ACTIVE) == ENTERING_ACTIVE)
		   ? "enter active"
		   : ((adsp_state & IN_ACTIVE) == IN_ACTIVE)
		   ? "active mode" : "none of state");
	return 0;
}

/****************************
 * show adsp dvfs sleep
 *****************************/
static int mt_adsp_dvfs_sleep_proc_show(struct seq_file *m, void *v)
{
	if (adsp_sleep_flag == -1)
		seq_puts(m,
			 "Warn: ADSP sleep not manually config by shell cmd\n");
	else if (adsp_sleep_flag == 0)
		seq_puts(m, "ADSP Sleep: OFF\n");
	else if (adsp_sleep_flag == 1)
		seq_puts(m, "ADSP Sleep: ON\n");
	else if (adsp_sleep_flag == 2)
		seq_puts(m, "ADSP Sleep: sleep without wakeup\n");
	else if (adsp_sleep_flag == 3)
		seq_puts(m, "ADSP Sleep: force to sleep\n");
	else
		seq_puts(m, "Warning: invalid ADSP Sleep configure\n");

	return 0;
}

/**********************************
 * write adsp dvfs sleep
 ***********************************/
static ssize_t mt_adsp_dvfs_sleep_proc_write(struct file *file,
					     const char __user *buffer,
					     size_t count, loff_t *data)
{
	char desc[64];
	unsigned int val = 0;
	int len = 0;
	int ret = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &val) == 0) {
		if (val >= 0  && val <= 3) {
			if (val != adsp_sleep_flag) {
				adsp_sleep_flag = val;
				ADSP_INFO("adsp_sleep_flag = %d\n",
					  adsp_sleep_flag);
				ret = adsp_ipi_send(IPI_DVFS_SLEEP,
						    (void *)&adsp_sleep_flag,
						    sizeof(adsp_sleep_flag),
						    0, ADSP_A_ID);
				if (ret != ADSP_IPI_DONE)
					ADSP_INFO("%s: ADSP send IPI fail %d\n",
						  __func__, ret);
			} else
				ADSP_INFO("sleep setting is not changed.%d\n",
					   val);
		} else {
			ADSP_INFO("Warning: invalid input value %d\n", val);
		}
	} else {
		ADSP_INFO("Warning: invalid input command, val=%d\n", val);
	}

	return count;
}

/****************************
 * show adsp dvfs ctrl
 *****************************/
static int mt_adsp_dvfs_ctrl_proc_show(struct seq_file *m, void *v)
{
	unsigned long spin_flags;
	int i;

	spin_lock_irqsave(&adsp_awake_spinlock, spin_flags);
	adsp_current_freq = readl(ADSP_CURRENT_FREQ_REG);
	adsp_expected_freq = readl(ADSP_EXPECTED_FREQ_REG);
	spin_unlock_irqrestore(&adsp_awake_spinlock, spin_flags);
	seq_printf(m, "ADSP DVFS: %s\n", (adsp_dvfs_flag == 1) ? "ON" : "OFF");
	seq_printf(m, "ADSP frequency: cur=%dMHz, expect=%dMHz\n",
		   adsp_current_freq, adsp_expected_freq);

	for (i = 0; i < ADSP_NUM_FEATURE_ID; i++)
		seq_printf(m, "feature=%d, freq=%d, enable=%d\n",
			   feature_table[i].feature, feature_table[i].freq,
			   feature_table[i].counter);

	for (i = 0; i < NUM_SENSOR_TYPE; i++)
		seq_printf(m, "sensor id=%d, freq=%d, enable=%d\n",
			   sensor_type_table[i].feature,
			   sensor_type_table[i].freq,
			   sensor_type_table[i].enable);

	return 0;
}

/**********************************
 * write adsp dvfs ctrl
 ***********************************/
static ssize_t mt_adsp_dvfs_ctrl_proc_write(struct file *file,
					    const char __user *buffer,
					    size_t count, loff_t *data)
{
	char desc[64], cmd[32];
	int len = 0;
	int req;
	int n;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	n = sscanf(desc, "%s %d", cmd, &req);
	if (n == 1 || n == 2) {
		if (!strcmp(cmd, "on")) {
			adsp_dvfs_flag = 1;
			ADSP_INFO("ADSP DVFS: ON\n");
		} else if (!strcmp(cmd, "off")) {
			adsp_dvfs_flag = -1;
			ADSP_INFO("ADSP DVFS: OFF\n");
		} else if (!strcmp(cmd, "req")) {
			if (req >= 0 && req <= 5) {
				if (pre_feature_req == 1)
					adsp_deregister_feature(
					VCORE_TEST_FEATURE_ID);
				else if (pre_feature_req == 2)
					adsp_deregister_feature(
					VCORE_TEST2_FEATURE_ID);
				else if (pre_feature_req == 3)
					adsp_deregister_feature(
					VCORE_TEST3_FEATURE_ID);
				else if (pre_feature_req == 4)
					adsp_deregister_feature(
					VCORE_TEST4_FEATURE_ID);
				else if (pre_feature_req == 5)
					adsp_deregister_feature(
					VCORE_TEST5_FEATURE_ID);

				if (req == 1)
					adsp_register_feature(
					VCORE_TEST_FEATURE_ID);
				else if (req == 2)
					adsp_register_feature(
					VCORE_TEST2_FEATURE_ID);
				else if (req == 3)
					adsp_register_feature(
					VCORE_TEST3_FEATURE_ID);
				else if (req == 4)
					adsp_register_feature(
					VCORE_TEST4_FEATURE_ID);
				else if (req == 5)
					adsp_register_feature(
					VCORE_TEST5_FEATURE_ID);

				pre_feature_req = req;
				ADSP_INFO("[ADSP] set freq: %d => %d\n",
					  adsp_current_freq,
					  adsp_expected_freq);
			} else {
				ADSP_INFO("invalid req value %d\n", req);
			}
		} else {
			ADSP_INFO("invalid command %s\n", cmd);
		}
	} else {
		ADSP_INFO("invalid length %d\n", n);
	}

	return count;
}

#define PROC_FOPS_RW(name)						\
static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {	\
	.owner		  = THIS_MODULE,				\
	.open		   = mt_ ## name ## _proc_open,	\
	.read		   = seq_read,					\
	.llseek		 = seq_lseek,					\
	.release		= single_release,			\
	.write		  = mt_ ## name ## _proc_write,			\
}

#define PROC_FOPS_RO(name)						\
static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {	\
	.owner		  = THIS_MODULE,				\
	.open		   = mt_ ## name ## _proc_open,	\
	.read		   = seq_read,					\
	.llseek		 = seq_lseek,					\
	.release		= single_release,			\
}

#define PROC_ENTRY(name)	{__stringify(name), &mt_ ## name ## _proc_fops}

#define PROC_ENTRY(name)        {__stringify(name), &mt_ ## name ## _proc_fops}

PROC_FOPS_RW(adsp_dvfs_debug);
PROC_FOPS_RO(adsp_dvfs_state);
PROC_FOPS_RW(adsp_dvfs_sleep);
PROC_FOPS_RW(adsp_dvfs_ctrl);

static int mt_adsp_dvfs_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i, ret = 0;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(adsp_dvfs_debug),
		PROC_ENTRY(adsp_dvfs_state),
		PROC_ENTRY(adsp_dvfs_sleep),
		PROC_ENTRY(adsp_dvfs_ctrl)
	};

	dir = proc_mkdir("adsp_dvfs", NULL);
	if (!dir) {
		pr_err("fail to create /proc/adsp_dvfs @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, 0664, dir,
				 entries[i].fops)) {
			pr_err("ERROR: %s: create /proc/adsp_dvfs/%s failed\n",
			       __func__, entries[i].name);
			ret = -ENOMEM;
		}
	}

	return ret;
}
#endif /* CONFIG_PROC_FS */

static const struct of_device_id adspdvfs_of_ids[] = {
	{.compatible = "mediatek,adsp_dvfs",},
	{}
};

static int mt_adsp_dvfs_suspend(struct device *dev)
{
	return 0;
}

static int mt_adsp_dvfs_resume(struct device *dev)
{
	return 0;
}

static int mt_adsp_dvfs_pm_restore_early(struct device *dev)
{
	return 0;
}

static int mt_adsp_dvfs_pdrv_probe(struct platform_device *pdev)
{
	struct device_node *node;
	unsigned int gpio_mode;

	ADSP_DBG("%s()\n", __func__);

	node = of_find_matching_node(NULL, adspdvfs_of_ids);
	if (!node) {
		dev_notice(&pdev->dev, "%s: fail to find ADSPDVFS node\n",
			   __func__);
		WARN_ON(1);
	}

	mt_adsp_pll = kzalloc(sizeof(struct mt_adsp_pll_t), GFP_KERNEL);
	if (mt_adsp_pll == NULL)
		return -ENOMEM;

	mt_adsp_pll->clk_mux = devm_clk_get(&pdev->dev, "clk_mux");
	if (IS_ERR(mt_adsp_pll->clk_mux)) {
		dev_notice(&pdev->dev, "cannot get clock mux\n");
		return PTR_ERR(mt_adsp_pll->clk_mux);
	}

	mt_adsp_pll->clk_pll0 = devm_clk_get(&pdev->dev, "clk_pll_0");
	if (IS_ERR(mt_adsp_pll->clk_pll0)) {
		dev_notice(&pdev->dev, "cannot get 1st clock parent\n");
		return PTR_ERR(mt_adsp_pll->clk_pll0);
	}
	mt_adsp_pll->clk_pll1 = devm_clk_get(&pdev->dev, "clk_pll_1");
	if (IS_ERR(mt_adsp_pll->clk_pll1)) {
		dev_notice(&pdev->dev, "cannot get 2nd clock parent\n");
		return PTR_ERR(mt_adsp_pll->clk_pll1);
	}
	mt_adsp_pll->clk_pll2 = devm_clk_get(&pdev->dev, "clk_pll_2");
	if (IS_ERR(mt_adsp_pll->clk_pll2)) {
		dev_notice(&pdev->dev, "cannot get 3rd clock parent\n");
		return PTR_ERR(mt_adsp_pll->clk_pll2);
	}
	mt_adsp_pll->clk_pll3 = devm_clk_get(&pdev->dev, "clk_pll_3");
	if (IS_ERR(mt_adsp_pll->clk_pll3)) {
		dev_notice(&pdev->dev, "cannot get 4th clock parent\n");
		return PTR_ERR(mt_adsp_pll->clk_pll3);
	}
	mt_adsp_pll->clk_pll4 = devm_clk_get(&pdev->dev, "clk_pll_4");
	if (IS_ERR(mt_adsp_pll->clk_pll4)) {
		dev_notice(&pdev->dev, "cannot get 5th clock parent\n");
		return PTR_ERR(mt_adsp_pll->clk_pll4);
	}
	mt_adsp_pll->clk_pll5 = devm_clk_get(&pdev->dev, "clk_pll_5");
	if (IS_ERR(mt_adsp_pll->clk_pll5)) {
		dev_notice(&pdev->dev, "cannot get 6th clock parent\n");
		return PTR_ERR(mt_adsp_pll->clk_pll5);
	}
	mt_adsp_pll->clk_pll6 = devm_clk_get(&pdev->dev, "clk_pll_6");
	if (IS_ERR(mt_adsp_pll->clk_pll6)) {
		dev_notice(&pdev->dev, "cannot get 7th clock parent\n");
		return PTR_ERR(mt_adsp_pll->clk_pll6);
	}

	/* get GPIO base address */
	node = of_find_compatible_node(NULL, NULL, "mediatek,gpio");
	if (!node) {
		pr_err("error: can't find GPIO node\n");
		WARN_ON(1);
		return 0;
	}

	gpio_base = of_iomap(node, 0);
	if (!gpio_base) {
		pr_err("error: iomap fail for GPIO\n");
		WARN_ON(1);
		return 0;
	}

	/* check if v_req pin is configured correctly  */
	gpio_mode = (DRV_Reg32(RG_GPIO154_MODE) >> GPIO154_BIT)&GPIO154_MASK;
	if (gpio_mode == 1)
		ADSP_DBG("ADSP v_req muxpin setting is correct\n");
	else
		pr_err("error:V_REQ muxpin setting is wrong - %d\n", gpio_mode);

	g_adsp_dvfs_init_flag = 1;

	return 0;
}

/***************************************
 * this function should never be called
 ****************************************/
static int mt_adsp_dvfs_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct dev_pm_ops mt_adsp_dvfs_pm_ops = {
	.suspend = mt_adsp_dvfs_suspend,
	.resume = mt_adsp_dvfs_resume,
	.restore_early = mt_adsp_dvfs_pm_restore_early,
};

struct platform_device mt_adsp_dvfs_pdev = {
	.name = "mt-adspdvfs",
	.id = -1,
};

static struct platform_driver mt_adsp_dvfs_pdrv = {
	.probe = mt_adsp_dvfs_pdrv_probe,
	.remove = mt_adsp_dvfs_pdrv_remove,
	.driver = {
		.name = "adspdvfs",
		.pm = &mt_adsp_dvfs_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = adspdvfs_of_ids,
	},
};

/**********************************
 * mediatek adsp dvfs initialization
 ***********************************/
void mt_adsp_dvfs_ipi_init(void)
{
	adsp_ipi_registration(IPI_ADSP_PLL_CTRL, adsp_pll_ctrl_handler,
			      "IPI_ADSP_PLL_CTRL");
}

int __init adsp_dvfs_init(void)
{
	int ret = 0;

	ADSP_DBG("%s\n", __func__);

#ifdef CONFIG_PROC_FS
	/* init proc */
	if (mt_adsp_dvfs_create_procfs()) {
		pr_err("mt_adsp_dvfs_create_procfs fail..\n");
		WARN_ON(1);
		return -1;
	}
#endif /* CONFIG_PROC_FS */

	/* register platform device/driver */
	ret = platform_device_register(&mt_adsp_dvfs_pdev);
	if (ret) {
		pr_err("fail to register adsp dvfs device @ %s()\n", __func__);
		WARN_ON(1);
		return -1;
	}

	ret = platform_driver_register(&mt_adsp_dvfs_pdrv);
	if (ret) {
		pr_err("fail to register adsp dvfs driver @ %s()\n", __func__);
		platform_device_unregister(&mt_adsp_dvfs_pdev);
		WARN_ON(1);
		return -1;
	}
#ifdef wakelock
	wake_lock_init(&adsp_suspend_lock, WAKE_LOCK_SUSPEND, "adsp wakelock");
#endif
	mt_adsp_dvfs_ipi_init();

	return ret;
}

void __exit adsp_dvfs_exit(void)
{
	platform_driver_unregister(&mt_adsp_dvfs_pdrv);
	platform_device_unregister(&mt_adsp_dvfs_pdev);
}
#else /*ADSP_DVFS_ENABLE*/

/* new implement for adsp */
static DEFINE_MUTEX(adsp_timer_mutex);
static DEFINE_MUTEX(adsp_suspend_mutex);

static struct timer_list adsp_suspend_timer;
static struct adsp_work_struct adsp_suspend_work;
static int adsp_is_suspend;

#define ADSP_DELAY_OFF_TIMEOUT          (1 * HZ) /* 1 second */
#define ADSP_DELAY_TIMER                (0)
#define FORCE_ADSPPLL_BASE	(0x10000000)
#define SPM_REQ_BASE		(0x20000000)
#define ADSPPLL_ON_BASE		(0x30000000)

/* Private Macro ---------------------------------------------------------*/
/* The register must sync with ctrl-boards.S in Tinysys */
#define CREG_BOOTUP_MARK   (ADSP_CFGREG_RSV_RW_REG0)
/* if equal, bypass clear bss and some init */
#define MAGIC_PATTERN      (0xfafafafa)

static inline ssize_t adsp_force_adsppll_store(struct device *kobj,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	uint32_t value = 0, param = 0;
	enum adsp_ipi_status  ret;

	if (kstrtoint(buf, 10, &value))
		return -EINVAL;

	switch (value) {
	case 480:
		param = FORCE_ADSPPLL_BASE+0x01E0;
		break;
	case 694:
		param = FORCE_ADSPPLL_BASE+0x02B6;
		break;
	case 800:
		param = FORCE_ADSPPLL_BASE+0x0320;
		break;
	default:
		break;
	}
	pr_debug("[%s]force in %d(%x)\n", __func__, value, param);

	if (is_adsp_ready(ADSP_A_ID)) {
		ret = adsp_ipi_send(ADSP_IPI_DVFS_SLEEP, &param,
			sizeof(param), 0, ADSP_A_ID);
	}
	return count;
}

static inline ssize_t adsp_spm_req_store(struct device *kobj,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	uint32_t value = 0, param;
	enum adsp_ipi_status  ret;

	if (kstrtoint(buf, 10, &value))
		return -EINVAL;

	param = SPM_REQ_BASE + value;
	pr_debug("[%s]set SPM req in %d(%x)\n", __func__, value, param);

	if (is_adsp_ready(ADSP_A_ID)) {
		ret = adsp_ipi_send(ADSP_IPI_DVFS_SLEEP, &param,
			sizeof(param), 0, ADSP_A_ID);
	}
	return count;
}

static inline ssize_t adsp_keep_adsppll_on_store(struct device *kobj,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	uint32_t value = 0, param;
	enum adsp_ipi_status  ret;

	if (kstrtoint(buf, 10, &value))
		return -EINVAL;

	param = ADSPPLL_ON_BASE + value;
	pr_debug("[%s] %d(%x)\n", __func__, value, param);

	if (is_adsp_ready(ADSP_A_ID)) {
		ret = adsp_ipi_send(ADSP_IPI_DVFS_SLEEP, &param,
			sizeof(param), 0, ADSP_A_ID);
	}
	return count;
}

DEVICE_ATTR(adsp_force_adsppll, 0220, NULL, adsp_force_adsppll_store);
DEVICE_ATTR(adsp_spm_req, 0220, NULL, adsp_spm_req_store);
DEVICE_ATTR(adsp_keep_adspll_on, 0220, NULL, adsp_keep_adsppll_on_store);

#define IS_ADSP_SUSPEND    ((readl(ADSP_SLEEP_STATUS_REG) \
				& (ADSP_A_IS_WFI | ADSP_A_AXI_BUS_IS_IDLE)) \
				== (ADSP_A_IS_WFI | ADSP_A_AXI_BUS_IS_IDLE))

uint32_t mt_adsp_freq_meter(void)
{
#if ADSP_VCORE_TEST_ENABLE
	return mt_get_ckgen_freq(ADSP_FREQ_METER_ID);
#else
	return 0;
#endif
}

void adsp_sw_reset(void)
{
	writel((ADSP_A_SW_RSTN | ADSP_A_SW_DBG_RSTN), ADSP_A_REBOOT);
	udelay(10);
	writel(0, ADSP_A_REBOOT);
	udelay(10);
	pr_debug("ADSP_A_REBOOT=%x", readl(ADSP_A_REBOOT));
}


void adsp_release_runstall(uint32_t release)
{
	uint32_t reg_value;

	reg_value = readl(ADSP_HIFI3_IO_CONFIG);
	if (release) {
		writel(reg_value & ~(ADSP_RELEASE_RUNSTALL),
		       ADSP_HIFI3_IO_CONFIG);
	} else {
		writel(reg_value | (ADSP_RELEASE_RUNSTALL),
		       ADSP_HIFI3_IO_CONFIG);
	}
}

void adsp_A_send_spm_request(uint32_t enable)
{
	uint32_t reg_value = 0;
	int timeout = 1000;

	pr_debug("req spm source & ddr enable(%d)\n", enable);
	if (enable) {
		/* request infra/26M/apsrc/v18/ ddr resource */
		writel(readl(ADSP_A_SPM_REQ) | ADSP_A_SPM_SRC_BITS,
		       ADSP_A_SPM_REQ);
		/* hw auto on/off ddren, disable now due to latency concern */
//		writel(ADSP_A_DDR_REQ_SEL, ADSP_A_DDREN_REQ);
		writel(readl(ADSP_A_DDREN_REQ) | ADSP_A_DDR_ENABLE,
		       ADSP_A_DDREN_REQ);

		/*make sure SPM return ack*/
		while (--timeout) {
			if (readl(ADSP_A_SPM_ACK) ==
			    ((ADSP_A_DDR_ENABLE << 4) | ADSP_A_SPM_SRC_BITS))
		/* hw auto on/off ddren */
//			if (readl(ADSP_A_SPM_ACK) == (ADSP_A_SPM_SRC_BITS))
				break;
			udelay(10);
			if (timeout == 0)
				pr_err("[ADSP] timeout: cannot get SPM ack\n");
		}
	} else {
		reg_value = readl(ADSP_A_DDREN_REQ);
		writel(readl(ADSP_A_DDREN_REQ) & ~(ADSP_A_DDR_ENABLE),
		       ADSP_A_DDREN_REQ);
		reg_value = readl(ADSP_A_SPM_REQ);
		writel(readl(ADSP_A_SPM_REQ) & ~ADSP_A_SPM_SRC_BITS,
		       ADSP_A_SPM_REQ);
		/*make sure SPM return ack*/
		while (--timeout) {
			if (readl(ADSP_A_SPM_ACK) == 0x0)
				break;
			udelay(10);
			if (timeout == 0)
				pr_err("[ADSP] timeout: cannot get SPM ack\n");
		}
	}
}

static void adsp_delay_off_handler(unsigned long data)
{
	if (!adsp_feature_is_active())
		adsp_schedule_work(&adsp_suspend_work);
}

void adsp_start_suspend_timer(void)
{
	mutex_lock(&adsp_timer_mutex);
	adsp_suspend_timer.function = adsp_delay_off_handler;
	adsp_suspend_timer.data = (unsigned long)ADSP_DELAY_TIMER;
	adsp_suspend_timer.expires =
		jiffies + ADSP_DELAY_OFF_TIMEOUT;
	if (timer_pending(&adsp_suspend_timer)) {
		pr_debug("adsp_suspend_timer has set\n");
	} else {
		pr_debug("adsp_suspend_timer add\n");
		add_timer(&adsp_suspend_timer);
	}
	mutex_unlock(&adsp_timer_mutex);
}

void adsp_stop_suspend_timer(void)
{
	mutex_lock(&adsp_timer_mutex);
	if (timer_pending(&adsp_suspend_timer))
		del_timer(&adsp_suspend_timer);
	mutex_unlock(&adsp_timer_mutex);
}

/*
 * callback function for work struct
 * @param ws:   work struct
 */
static void adsp_suspend_ws(struct work_struct *ws)
{
	struct adsp_work_struct *sws = container_of(ws, struct adsp_work_struct,
						    work);
	enum adsp_core_id core_id = (enum adsp_core_id) sws->id;

	adsp_suspend(core_id);
}
#if ADSP_ITCM_MONITOR
static unsigned int adsp_itcm_gtable[4096];
#endif
void adsp_itcm_gtable_init(void)
{
#if ADSP_ITCM_MONITOR
	int i;
	const u32 __iomem *s = (void *)(ADSP_A_ITCM);

	for (i = 0; i < 4096; i++)
		adsp_itcm_gtable[i] = *s++;
#endif
}

int adsp_itcm_gtable_check(void)
{
#if ADSP_ITCM_MONITOR
	int ret = 0;
	int i;
	u32 __iomem *s = (void *)(ADSP_A_ITCM);

	for (i = 0; i < 4096; i++) {
		if (adsp_itcm_gtable[i] != *s) {
			pr_err("[%s]adsp_itcm_gtable[%d](0x%x) != ITCM(0x%x)\n",
				 __func__, i, adsp_itcm_gtable[i], *s);
			*s = adsp_itcm_gtable[i];
			ret = -1;
		}
		s++;
	}
	return ret;
#else
	return 0;
#endif
}

int adsp_suspend_init(void)
{
	INIT_WORK(&adsp_suspend_work.work, adsp_suspend_ws);
	init_timer(&adsp_suspend_timer);
	adsp_is_suspend = 0;
	adsp_itcm_gtable_init();
	return 0;
}

/* actually execute suspend flow, which cannot be disabled.
 * retry the resume flow after adsp_ready = 0.
 */
void adsp_suspend(enum adsp_core_id core_id)
{
	enum adsp_ipi_status ret;
	int value = 0;
	int timeout = 5000;
	int itcm_ret = 0;
#if ADSP_DVFS_PROFILE
	ktime_t begin, end;
#endif
	mutex_lock(&adsp_suspend_mutex);
	if (is_adsp_ready(core_id) && !adsp_is_suspend) {
#if ADSP_DVFS_PROFILE
		begin = ktime_get();
#endif
		ret = adsp_ipi_send(ADSP_IPI_DVFS_SUSPEND, &value,
				    sizeof(value), 1, core_id);

		while (--timeout && !IS_ADSP_SUSPEND)
			udelay(10);

		if (!IS_ADSP_SUSPEND) {
			pr_warn("[%s]wait adsp suspend timeout\n", __func__);
			adsp_aed(EXCEP_RUNTIME, core_id);
		}
		adsp_release_runstall(false);
		itcm_ret = adsp_itcm_gtable_check();
		adsp_power_on(false);
		adsp_disable_clock();
		adsp_reset_ready(core_id);
		adsp_is_suspend = 1;
#if ADSP_DVFS_PROFILE
		end = ktime_get();
		pr_debug("[%s]latency = %lld us, itcm_check(%d)\n",
			 __func__, ktime_us_delta(end, begin),
			 itcm_ret);
#endif
	}
	mutex_unlock(&adsp_suspend_mutex);
}

#define WDT_EN_BIT  (1 << 31)

int adsp_resume(void)
{
	int retry = 5000;
	int ret = 0;
	int itcm_ret = 0;
#if ADSP_DVFS_PROFILE
	ktime_t begin, end;
#endif
	mutex_lock(&adsp_suspend_mutex);
	if (!is_adsp_ready(ADSP_A_ID) && adsp_is_suspend) {
#if ADSP_DVFS_PROFILE
		begin = ktime_get();
#endif
		adsp_power_on(true);
		/* To indicate only main_lite() is needed. */
		writel(MAGIC_PATTERN, CREG_BOOTUP_MARK);
		DRV_ClrReg32(ADSP_A_WDT_REG, WDT_EN_BIT);
		itcm_ret = adsp_itcm_gtable_check();
		adsp_release_runstall(true);

		/* busy waiting until adsp is ready */
		while (--retry && !is_adsp_ready(ADSP_A_ID))
			udelay(10);

		if (!is_adsp_ready(ADSP_A_ID)) {
			pr_warn("[%s]wait for adsp ready timeout\n", __func__);
			ret = -ETIME;
		}
		adsp_is_suspend = 0;
#if ADSP_DVFS_PROFILE
		end = ktime_get();
		pr_debug("[%s]latency = %lld us, itcm_check(%d)\n",
			 __func__, ktime_us_delta(end, begin),
			 itcm_ret);
#endif
	}
	mutex_unlock(&adsp_suspend_mutex);
	return ret;
}
/* new implement for adsp - end*/

void mt_adsp_dvfs_ipi_init(void)
{
}
int __init adsp_dvfs_init(void)
{
	return 0;
}
void __exit adsp_dvfs_exit(void)
{
}

#endif /*ADSP_DVFS_ENABLE*/
