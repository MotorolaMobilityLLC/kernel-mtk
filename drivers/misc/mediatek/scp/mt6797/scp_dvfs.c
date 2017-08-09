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

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include <asm/uaccess.h>
#include "scp_ipi.h"
#include "scp_helper.h"
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

#define DVFS_STATUS_OK			0
#define DVFS_STATUS_BUSY			-1
#define DVFS_REQUEST_SAME_CLOCK		-2
#define DVFS_STATUS_ERR			-3
#define DVFS_STATUS_TIMEOUT		-4
#define DVFS_CLK_ERROR			-5
#define DVFS_STATUS_CMD_FIX		-6
#define DVFS_STATUS_CMD_LIMITED		-7
#define DVFS_STATUS_CMD_DISABLE		-8

/* CLK_CTRL Registers */
#define CLK_SW_SEL		(*(volatile unsigned int *)(scpreg.clkctrl + 0x0000))  /* Clock Source Select */
#define CLK_ENABLE	(*(volatile unsigned int *)(scpreg.clkctrl + 0x0004))  /* Clock Source Enable */
#define CLK_DIV_SEL	(*(volatile unsigned int *)(scpreg.clkctrl + 0x0024))  /* Clock Divider Select */
#define SLEEP_DEBUG	(*(volatile unsigned int *)(scpreg.clkctrl + 0x0028))  /* Sleep mode debug signals */
#define CKSW_SEL_O	16
#define CKSW_DIV_SEL_O	16

typedef enum {
	IN_DEBUG_IDLE = 1,
	ENTERING_SLEEP = 2,
	IN_SLEEP = 4,
	ENTERING_ACTIVE = 8,
	IN_ACTIVE = 16,
} scp_state_enum;
static bool mt_scp_dvfs_debug = true;

/***************************
 * Operate Point Definition
 ****************************/
struct mt_scp_dvfs_table_info {
	unsigned int cur_clk;
	unsigned int cur_volt;
	unsigned int fix_clk;
	bool fix_clk_on;
	unsigned int limited_clk;
	unsigned int limited_volt;
	bool limited_clk_on;
	bool scp_dvfs_disable;
	bool scp_dvfs_sleep;
	bool scp_dvfs_wake;
};

typedef enum {
	CLK_SEL_SYS     = 0x0,
	CLK_SEL_32K     = 0x1,
	CLK_SEL_HIGH    = 0x2
} clk_sel_val;

enum {
	CLK_SYS_EN_BIT = 0,
	CLK_HIGH_EN_BIT = 1,
	CLK_HIGH_CG_BIT = 2,
	CLK_SYS_IRQ_EN_BIT = 16,
	CLK_HIGH_IRQ_EN_BIT = 17,
};
/*#ifdef CONFIG_PINCTRL_MT6797*/

typedef enum  {
	CLK_UNKNOWN = 0,
	CLK_112M,
	CLK_224M,
	CLK_354M,
	CLK_32K,
} clk_enum;

typedef enum {
	CLK_DIV_1 = 0,
	CLK_DIV_2 = 1,
	CLK_DIV_4  = 2,
	CLK_DIV_8  = 3,
	CLK_DIV_UNKNOWN,
} clk_div_enum;

typedef enum  {
	SPM_VOLTAGE_800_D = 0,
	SPM_VOLTAGE_800,
	SPM_VOLTAGE_900,
	SPM_VOLTAGE_1000,
	SPM_VOLTAGE_TYPE_NUM,
} voltage_enum;

#if 0
static struct pinctrl *scp_pctrl; /* static pinctrl instance */
/* DTS state */
typedef enum tagDTS_GPIO_STATE {
	SCP_DTS_GPIO_STATE_DEFAULT = 0,
	SCP_DTS_GPIO_STATE_AUD_OFF,
	SCP_DTS_GPIO_STATE_AUD_ON,
	SCP_DTS_GPIO_STATE_MAX,	/* for array size */
} SCP_DTS_GPIO_STATE;

/* DTS state mapping name */
static const char *scp_state_name[SCP_DTS_GPIO_STATE_MAX] = {
	"default",
	"scp_gpio_off",
	"scp_gpio_on"
};
#endif
static unsigned int scp_cg;
static unsigned int scp_irq_en;
static unsigned int scp_irq_status;
static unsigned int scp_awake_request;
static unsigned int scp_state;

static struct mt_scp_dvfs_table_info *mt_scp_dvfs_info;

static void _mt_setup_scp_dvfs_table(int num)
{
	mt_scp_dvfs_info = kzalloc((num) * sizeof(struct mt_scp_dvfs_table_info), GFP_KERNEL);
	if (mt_scp_dvfs_info == NULL) {
		scp_dvfs_err("scp dvfs table memory allocation fail\n");
		return;
	}
}

void dvfs_debug_cb_handler(int id, void *data, unsigned int len)
{
	unsigned int *scp_data = (unsigned int *)data;

	if (1 == *scp_data)
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

clk_enum get_cur_clk(void)
{
	unsigned int cur_clk, cur_div, clk_enable;

	cur_clk = get_clk_sw_select();
	cur_div = get_clk_div_select();

	clk_enable = CLK_ENABLE;
	scp_dvfs_info("cur_clk = %d, cur_div = %d, clk_enable = 0x%x\n", cur_clk, cur_div, clk_enable);
	if (cur_clk == CLK_SEL_SYS && cur_div == CLK_DIV_1 && (clk_enable & (1 << CLK_SYS_EN_BIT)) != 0)
		return CLK_354M;
	else if (cur_clk == CLK_SEL_HIGH && cur_div == CLK_DIV_1 && (clk_enable & (1 << CLK_HIGH_EN_BIT)) != 0)
		return CLK_224M;
	else if (cur_clk == CLK_SEL_HIGH && cur_div == CLK_DIV_2 && (clk_enable & (1 << CLK_HIGH_EN_BIT)) != 0)
		return CLK_112M;
	else if (cur_clk == CLK_SEL_32K)
		return CLK_32K;

	scp_dvfs_err("clk setting error (%d, %d)\n", cur_clk, cur_div);
	return CLK_UNKNOWN;
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
	char desc[32];
	int len = 0;
	int debug = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtoint(desc, 0, &debug) == 0) {
		if (debug == 0)
			mt_scp_dvfs_debug = 0;
		else if (debug == 1)
			mt_scp_dvfs_debug = 1;
		else
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else {
		scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	}

	scp_ipi_send(IPI_DVFS_DEBUG, (void *)&mt_scp_dvfs_debug, sizeof(mt_scp_dvfs_debug), 0);
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
	char desc[32];
	int len = 0;
	unsigned int limited_clk = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &limited_clk) == 0)
		scp_ipi_send(IPI_DVFS_LIMIT_OPP_SET, (void *)&limited_clk, sizeof(limited_clk), 0);
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
	char desc[32];
	int len = 0;
	unsigned int limited_clk_on = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &limited_clk_on) == 0)
		scp_ipi_send(IPI_DVFS_LIMIT_OPP_EN, (void *)&limited_clk_on, sizeof(limited_clk_on), 0);
	else
		scp_dvfs_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}

/****************************
 * show current clock frequency
 *****************************/
static int mt_scp_dvfs_cur_opp_proc_show(struct seq_file *m, void *v)
{
	clk_enum cur_clk = CLK_UNKNOWN;

	cur_clk = get_cur_clk();
	seq_printf(m, "current opp = %s\n",
		(cur_clk == CLK_112M) ? "CLK 112M, Voltage = 0.8V"
		: (cur_clk == CLK_224M) ? "CLK_224M, Voltage = 0.9V"
		: (cur_clk == CLK_354M) ? "CLK_354M, Volt = 1.0V"
		: (cur_clk == CLK_32K) ? "CLK_32K, Volt = 0.6V"
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
	char desc[32];
	int len = 0;
	unsigned int fix_clk = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &fix_clk) == 0)
		scp_ipi_send(IPI_DVFS_FIX_OPP_SET, (void *)&fix_clk, sizeof(fix_clk), 0);
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
	char desc[32];
	int len = 0;
	unsigned int fix_clk_on = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &fix_clk_on) == 0)
		scp_ipi_send(IPI_DVFS_FIX_OPP_EN, (void *)&fix_clk_on, sizeof(fix_clk_on), 0);
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
	char desc[32];
	int len = 0;
	unsigned int on = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &on) == 0) {
		if (on == 0) {
			mt_scp_dvfs_info->scp_dvfs_sleep = 0;
			scp_dvfs_warn("scp_dvfs_sleep = 0\n");
		} else if (on == 1) {
			mt_scp_dvfs_info->scp_dvfs_sleep = 1;
			scp_dvfs_warn("scp_dvfs_sleep = 1\n");
		} else {
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
		}
		scp_ipi_send(IPI_DVFS_SLEEP, (void *)&(mt_scp_dvfs_info->scp_dvfs_sleep),
				sizeof(mt_scp_dvfs_info->scp_dvfs_sleep), 0);
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
	char desc[32];
	int len = 0;
	unsigned int on = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &on) == 0) {
		if (on == 0)
			mt_scp_dvfs_info->scp_dvfs_wake = 0;
		else if (on == 1)
			mt_scp_dvfs_info->scp_dvfs_wake = 1;
		else
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

		scp_ipi_send(IPI_DVFS_WAKE, (void *)&(mt_scp_dvfs_info->scp_dvfs_wake),
				sizeof(mt_scp_dvfs_info->scp_dvfs_wake), 0);
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
	char desc[32];
	int len = 0;
	unsigned int on = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 0, &on) == 0) {
		if (on == 0)
			mt_scp_dvfs_info->scp_dvfs_disable = 0;
		else if (on == 1) {
			mt_scp_dvfs_info->scp_dvfs_disable = 1;
		} else
			scp_dvfs_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
		scp_ipi_send(IPI_DVFS_DISABLE, (void *)&(mt_scp_dvfs_info->scp_dvfs_disable),
				sizeof(mt_scp_dvfs_info->scp_dvfs_disable), 0);
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

#if 0
/* pinctrl implementation */
static long _set_state(const char *name)
{
	long ret = 0;
	struct pinctrl_state *pState = 0;

	BUG_ON(!scp_pctrl);

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
	{.compatible = "mediatek,mt6797-scpdvfs",},
	{}
};

static int mt_scp_dvfs_suspend(struct device *dev)
{
	/* _set_state(scp_state_name[SCP_DTS_GPIO_STATE_AUD_ON]);
	scp_dvfs_dbg("set scp gpio to audio clk on\n"); */
	return 0;
}

static int mt_scp_dvfs_resume(struct device *dev)
{
	/* _set_state(scp_state_name[SCP_DTS_GPIO_STATE_AUD_OFF]);
	scp_dvfs_dbg("set scp gpio to audio clk off\n"); */
	return 0;
}
static int mt_scp_dvfs_pm_restore_early(struct device *dev)
{
	return 0;
}
static int mt_scp_dvfs_pdrv_probe(struct platform_device *pdev)
{
	/* int ret = 0; */
	struct device_node *node;

	node = of_find_matching_node(NULL, scpdvfs_of_ids);
	if (!node)
		dev_err(&pdev->dev, "@%s: find SCPDVFS node failed\n", __func__);
	/* retrieve */
#if 0
	scp_pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(scp_pctrl)) {
		dev_err(&pdev->dev, "Cannot find scp pinctrl!\n");
		ret = PTR_ERR(scp_pctrl);
		goto exit;
	}
#endif
	return 0;
#if 0
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
void mt_scp_dvfs_ipi_init(void)
{
#if 0
	scp_ipi_registration(IPI_DVFS_DEBUG, dvfs_debug_cb_handler, "IPI_DVFS_DEBUG");
	scp_ipi_registration(IPI_DVFS_FIX_OPP_SET, fix_opp_set_handler, "IPI_DVFS_FIX_OPP_SET");
	scp_ipi_registration(IPI_DVFS_FIX_OPP_EN, fix_opp_en_handler, "IPI_DVFS_FIX_OPP_EN");
	scp_ipi_registration(IPI_DVFS_LIMIT_OPP_SET, limit_opp_set_handler, "IPI_DVFS_LIMIT_OPP_SET");
	scp_ipi_registration(IPI_DVFS_LIMIT_OPP_EN, limit_opp_en_handler, "IPI_DVFS_LIMIT_OPP_EN");
	scp_ipi_registration(IPI_DVFS_DISABLE, dvfs_disable_handler, "IPI_DVFS_DISABLE");
	scp_ipi_registration(IPI_DVFS_INFO_DUMP, dump_info_handler, "IPI_DVFS_INFO_DUMP");
	scp_ipi_registration(IPI_DUMP_REG, dump_reg_handler, "IPI_DUMP_REG");
	scp_ipi_registration(IPI_SCP_STATE, scp_state_handler, "IPI_SCP_STATE");
#endif
	_mt_setup_scp_dvfs_table(1);
}
#if 1
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
out:
	return ret;

}
#endif
static void __exit _mt_scp_dvfs_exit(void)
{
	platform_driver_unregister(&mt_scp_dvfs_pdrv);
	platform_device_unregister(&mt_scp_dvfs_pdev);
}

module_init(_mt_scp_dvfs_init);
module_exit(_mt_scp_dvfs_exit);

MODULE_DESCRIPTION("MediaTek SCP Freqency Scaling AP driver");
MODULE_LICENSE("GPL");
