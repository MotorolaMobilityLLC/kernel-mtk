#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/platform_device.h>

#include "mach/fliper.h"
#include "mach/mt_vcorefs_governor.h"

#include "mach/mt_emi_bm.h"

#define SEQ_printf(m, x...)\
	do {\
		if (m)\
			seq_printf(m, x);\
		else\
			pr_emerg(x);\
	} while (0)
#undef TAG
#define TAG "[SOC DVFS FLIPER]"


static int cg_lpm_bw_threshold, cg_hpm_bw_threshold;
static int total_lpm_bw_threshold, total_hpm_bw_threshold;
static int cg_fliper_enabled, total_fliper_enabled;
static int fliper_debug;

/******* FLIPER SETTING *********/

void enable_cg_fliper(int enable)
{
	if (enable) {
		vcorefs_enable_perform_bw(true);
		pr_emerg(TAG"CG fliper enabled\n");
		cg_fliper_enabled = 1;
	} else {
		vcorefs_enable_perform_bw(false);
		pr_emerg(TAG"CG fliper disabled\n");
		cg_fliper_enabled = 0;
	}
}

void enable_total_fliper(int enable)
{
	if (enable) {
		vcorefs_enable_total_bw(true);
		pr_emerg(TAG"TOTAL fliper enabled\n");
		total_fliper_enabled = 1;
	} else {
		vcorefs_enable_total_bw(false);
		pr_emerg(TAG"TOTAL fliper disabled\n");
		total_fliper_enabled = 0;
	}
}

int cg_set_threshold(int bw1, int bw2)
{
	int lpm_threshold, hpm_threshold;

	if (bw1 <= BW_THRESHOLD_MAX && bw1 >= BW_THRESHOLD_MIN &&
			bw2 <= BW_THRESHOLD_MAX && bw2 >= BW_THRESHOLD_MIN) {
		lpm_threshold = bw1 * THRESHOLD_SCALE / LPM_MAX_BW + 1;
		hpm_threshold = bw2 * THRESHOLD_SCALE / HPM_MAX_BW;
		if (lpm_threshold > 127 || lpm_threshold < 1 || hpm_threshold > 127 || hpm_threshold < 1) {
			pr_emerg(TAG"error set threshold out of range\n");
			return 0;
		}
		vcorefs_set_perform_bw_threshold(lpm_threshold, hpm_threshold);
		pr_emerg(TAG"Set CG bdw threshold %d %d -> %d %d\n",
				cg_lpm_bw_threshold, cg_hpm_bw_threshold, bw1, bw2);
		pr_emerg(TAG"CG lpm: %d hpm: %d\n", lpm_threshold, hpm_threshold);
		cg_lpm_bw_threshold = bw1;
		cg_hpm_bw_threshold = bw2;
	} else {
		pr_emerg(TAG"Set CG bdw threshold Error: (MAX:%d, MIN:%d)\n",
			BW_THRESHOLD_MAX, BW_THRESHOLD_MIN);
	}

	return 0;
}

int total_set_threshold(int bw1, int bw2)
{
	int lpm_threshold, hpm_threshold;

	pr_emerg(TAG"arg bw1=%d bw2=%d\n", bw1, bw2);
	if (bw1 <= BW_THRESHOLD_MAX && bw1 >= BW_THRESHOLD_MIN &&
			bw2 <= BW_THRESHOLD_MAX && bw2 >= BW_THRESHOLD_MIN) {
		lpm_threshold = bw1 * THRESHOLD_SCALE / LPM_MAX_BW;
		hpm_threshold = bw2 * THRESHOLD_SCALE / HPM_MAX_BW;
		if (lpm_threshold > 127 || lpm_threshold < 1 || hpm_threshold > 127 || hpm_threshold < 1) {
			pr_emerg(TAG"error set threshold out of range\n");
			return 0;
		}
		vcorefs_set_total_bw_threshold(lpm_threshold, hpm_threshold);
		pr_emerg(TAG"Set TOTAL bdw threshold %d %d-> %d %d\n",
			total_lpm_bw_threshold, total_hpm_bw_threshold, bw1, bw2);
		pr_emerg(TAG"TOTAL lpm: %d hpm: %d\n", lpm_threshold, hpm_threshold);
		total_lpm_bw_threshold = bw1;
		total_hpm_bw_threshold = bw2;
	} else {
		pr_emerg(TAG"Set TOTAL bdw threshold Error: (MAX:%d, MIN:%d)\n",
			BW_THRESHOLD_MAX, BW_THRESHOLD_MIN);
	}

	return 0;
}

int cg_restore_threshold(void)
{
	cg_set_threshold(CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD);
	pr_emerg(TAG"Restore CG bdw threshold %d %d -> %d %d\n",
		cg_lpm_bw_threshold, cg_hpm_bw_threshold, CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD);
	cg_lpm_bw_threshold = CG_LPM_BW_THRESHOLD;
	cg_hpm_bw_threshold = CG_HPM_BW_THRESHOLD;
	return 0;
}

int total_restore_threshold(void)
{
	total_set_threshold(TOTAL_LPM_BW_THRESHOLD, TOTAL_HPM_BW_THRESHOLD);
	pr_emerg(TAG"Restore TOTAL bdw threshold %d %d -> %d %d\n",
		total_lpm_bw_threshold, total_hpm_bw_threshold,
		TOTAL_LPM_BW_THRESHOLD, TOTAL_HPM_BW_THRESHOLD);
	total_lpm_bw_threshold = TOTAL_LPM_BW_THRESHOLD;
	total_hpm_bw_threshold = TOTAL_HPM_BW_THRESHOLD;
	return 0;
}

int getTotal(void)
{
	return BM_GetBWST();
}

int getCG(void)
{
	return BM_GetBWST1();
}

int setTotal(const unsigned int value)
{
	return BM_SetBW(value);
}

int setCG(const unsigned int value)
{
	return BM_SetBW1(value);
}

static ssize_t mt_fliper_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret;
	unsigned long arg1, arg2;
	char option[64], arg[10];
	int i, j;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	pr_emerg(TAG"option buffer is %s\n", buf);

	/* get option */
	for (i = 0; i < cnt && buf[i] != ' '; i++)
		option[i] = buf[i];
	option[i] = '\0';
	pr_emerg(TAG"option string is %s\n", option);

	/* get arg1 */
	for (; i < cnt && buf[i] == ' '; i++)
		;
	for (j = 0; i < cnt && buf[i] != ' '; i++, j++)
		arg[j] = buf[i];
	arg[j] = '\0';
	pr_emerg(TAG"arg1 %s end", arg);
	if (j > 0) {
		ret = kstrtoul(arg, 0, (unsigned long *)&arg1);
		if (ret < 0) {
			pr_emerg(TAG"1 ret of kstrtoul is broke\n");
			return ret;
		}
	}

	/* get arg2 */
	for (; i < cnt && buf[i] == ' '; i++)
		;
	for (j = 0; i < cnt && buf[i] != ' '; i++, j++)
		arg[j] = buf[i];
	arg[j] = '\0';
	if (j > 0) {
		ret = kstrtoul(arg, 0, (unsigned long *)&arg2);
		if (ret < 0) {
			pr_emerg(TAG"1 ret of kstrtoul is broke\n");
			return ret;
		}
	}

	if (strncmp(option, "ENABLE_CG", 9) == 0) {
		enable_cg_fliper(arg1);
	} else if (strncmp(option, "ENABLE_TOTAL", 12) == 0) {
		enable_total_fliper(arg1);
	} else if (strncmp(option, "SET_CG_THRES", 12) == 0) {
		cg_set_threshold(arg1, arg2);
	} else if (strncmp(option, "SET_TOTAL_THRES", 15) == 0) {
		total_set_threshold(arg1, arg2);
	} else if (strncmp(option, "RESTORE_CG", 10) == 0) {
		cg_restore_threshold();
	} else if (strncmp(option, "RESTORE_TOTAL", 13) == 0) {
		total_restore_threshold();
	}	else if (strncmp(option, "SET_CG", 6) == 0) {
		setCG(arg1);
	}	else if (strncmp(option, "SET_TOTAL", 9) == 0) {
		setTotal(arg1);
	} else if (strncmp(option, "GET_CG", 6) == 0) {
		pr_emerg(TAG"%d", getCG());
	}	else if (strncmp(option, "GET_TOTAL", 9) == 0) {
		pr_emerg(TAG"%d", getTotal());
	}  else if (strncmp(option, "POWER_MODE", 10) == 0) {
			if (arg1 == Default) {
				enable_cg_fliper(1);
				cg_set_threshold(CG_DEFAULT_LPM, CG_DEFAULT_HPM);
			} else if (arg1 == Low_Power_Mode) {
				enable_cg_fliper(0);
			} else if (arg1 == Just_Make_Mode) {
				enable_cg_fliper(1);
				cg_set_threshold(CG_JUST_MAKE_LPM, CG_JUST_MAKE_HPM);
			} else if (arg1 == Performance_Mode) {
				enable_cg_fliper(1);
				cg_set_threshold(CG_PERFORMANCE_LPM, CG_PERFORMANCE_HPM);
			}
	} else if (strncmp(option, "DEBUG", 5) == 0) {
		fliper_debug = arg1;
	} else {
		pr_emerg(TAG"unknown option\n");
	}

	return cnt;
}

static int mt_fliper_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "----------------------------------------\n");
	SEQ_printf(m, "CG Fliper Enabled:%d, bw threshold:%d %dMB/s\n",
			cg_fliper_enabled, cg_lpm_bw_threshold, cg_hpm_bw_threshold);
	SEQ_printf(m, "TOTAL Fliper Enabled:%d, bw threshold:%d %dMB/s\n",
			total_fliper_enabled, total_lpm_bw_threshold, total_hpm_bw_threshold);
	SEQ_printf(m, "CG History: 0x%08x\n", BM_GetBWST1());
	SEQ_printf(m, "TOTAL History: 0x%08x\n", BM_GetBWST());
	SEQ_printf(m, "----------------------------------------\n");
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_fliper_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_fliper_show, inode->i_private);
}

static const struct file_operations mt_fliper_fops = {
	.open = mt_fliper_open,
	.write = mt_fliper_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*--------------------INIT------------------------*/

static int __init init_fliper(void)
{
	struct proc_dir_entry *pe;

	pr_emerg(TAG"init fliper driver start\n");
	pe = proc_create("fliper", 0644, NULL, &mt_fliper_fops);
	if (!pe)
		return -ENOMEM;

#if 1
	setCG(0x210d80);
	setTotal(0x210de0);
	cg_lpm_bw_threshold = CG_LPM_BW_THRESHOLD;
	cg_hpm_bw_threshold = CG_HPM_BW_THRESHOLD;
	total_lpm_bw_threshold = TOTAL_LPM_BW_THRESHOLD;
	total_hpm_bw_threshold = TOTAL_HPM_BW_THRESHOLD;
	cg_set_threshold(CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD);
	total_set_threshold(TOTAL_LPM_BW_THRESHOLD, TOTAL_HPM_BW_THRESHOLD);
	enable_cg_fliper(1);
	enable_total_fliper(1);
	cg_fliper_enabled = 1;
	total_fliper_enabled = 1;
#else

	setCG(0x210d80);
	setTotal(0x210d80);
	cg_lpm_bw_threshold = CG_LPM_BW_THRESHOLD;
	cg_hpm_bw_threshold = CG_HPM_BW_THRESHOLD;
	total_lpm_bw_threshold = TOTAL_LPM_BW_THRESHOLD;
	total_hpm_bw_threshold = TOTAL_HPM_BW_THRESHOLD;
	cg_set_threshold(CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD);
	total_set_threshold(TOTAL_LPM_BW_THRESHOLD, TOTAL_HPM_BW_THRESHOLD);
	enable_cg_fliper(0);
	enable_total_fliper(0);
	cg_fliper_enabled = 0;
	total_fliper_enabled = 0;
#endif
	fliper_debug = 0;

	pr_emerg(TAG"init fliper driver done\n");

	return 0;
}
device_initcall(init_fliper);

/*MODULE_LICENSE("GPL");*/
/*MODULE_AUTHOR("MTK");*/
/*MODULE_DESCRIPTION("The fliper function");*/
