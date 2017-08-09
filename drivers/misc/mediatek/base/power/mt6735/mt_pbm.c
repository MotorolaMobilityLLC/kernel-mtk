#define pr_fmt(fmt)		"[PBM] " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/wakelock.h>
#include <linux/kthread.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/sched/rt.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/suspend.h>
#include <linux/proc_fs.h>

#include <mach/mt_pbm.h>
#include <mach/upmu_sw.h>
#include <mt-plat/upmu_common.h>
#include <mt_cpufreq.h>
#include <mt_gpufreq.h>
#include <mach/mt_thermal.h>

#ifndef DISABLE_PBM_FEATURE

/* reference PMIC */
/* extern kal_uint32 PMIC_IMM_GetOneChannelValue(kal_uint8 dwChannel, int deCount, int trimd); */
/* #define DLPT_PRIO_PBM 0 */
/* void (*dlpt_callback)(unsigned int); */
/* void register_dlpt_notify( void (*dlpt_callback)(unsigned int), int i){} */
/* reference mt_cpufreq.h and mt_gpufreq.h */
/* unsigned int mt_cpufreq_get_leakage_mw(int i){return 111;} */
/* unsigned int mt_gpufreq_get_leakage_mw(void){return 111;} */
/* void mt_cpufreq_set_power_limit_by_pbm(unsigned int limited_power){} */
/* void mt_gpufreq_set_power_limit_by_pbm(unsigned int limited_power){} */

static bool mt_pbm_debug;

#define pbm_emerg(fmt, args...)	pr_emerg(fmt, ##args)
#define pbm_alert(fmt, args...)	pr_alert(fmt, ##args)
#define pbm_crit(fmt, args...)	pr_crit(fmt, ##args)
#define pbm_err(fmt, args...)	pr_err(fmt, ##args)
#define pbm_warn(fmt, args...)	pr_warn(fmt, ##args)
#define pbm_notice(fmt, args...)	pr_debug(fmt, ##args)
#define pbm_info(fmt, args...)	pr_debug(fmt, ##args)

#define pbm_debug(fmt, args...)	\
	do {			\
		if (mt_pbm_debug)		\
			pr_crit(fmt, ##args);	\
	} while (0)

#define BIT_CHECK(a, b) ((a) & (1<<(b)))

#define POWER_MD        1800	/* mW */
#define POWER_FLASH     2500	/* mW */

static struct hpf hpf_ctrl = {
	.switch_md1 = 1,
	.switch_md2 = 0,
	.switch_gpu = 0,
	.switch_flash = 0,

	.cpu_volt = 1000,	/* 1V = boot up voltage */
	.gpu_volt = 0,
	.cpu_num = 1,		/* default cpu0 core */

	.loading_leakage = 0,
	.loading_dlpt = 0,
	.loading_md = POWER_MD,	/* fixed */
	.loading_cpu = 0,
	.loading_gpu = 0,
	.loading_flash = POWER_FLASH,	/* fixed */
};

static struct pbm pbm_ctrl = {
	/* feature key */
	.feature_en = 1,
	.pbm_drv_done = 0,
	.hpf_en = 31,		/* bin: 11111 (Flash, GPU, CPU, MD, DLPT) */
};

int g_dlpt_need_do = 1;
static DEFINE_MUTEX(pbm_mutex);
static DEFINE_MUTEX(pbm_table_lock);
static struct task_struct *pbm_thread;
static atomic_t kthread_nreq = ATOMIC_INIT(0);
/* extern u32 get_devinfo_with_index(u32 index); */

int get_battery_volt(void)
{
	return PMIC_IMM_GetOneChannelValue(MT6328_AUX_BATSNS_AP, 5, 1);
	/* return 3900; */
}

unsigned int ma_to_mw(unsigned int val)
{
	unsigned int bat_vol = 0;
	unsigned int ret_val = 0;

	bat_vol = get_battery_volt();	/* return mV */
	ret_val = (bat_vol * val) / 1000;	/* mW = (mV * mA)/1000 */
	pbm_crit("[%s] %d(mV) * %d(mA) = %d(mW)\n", __func__, bat_vol, val, ret_val);

	return ret_val;
}

void dump_kicker_info(void)
{
	struct hpf *hpfmgr = &hpf_ctrl;

#if 1
	pbm_debug("(M/F/G)=%d,%d,%d;(C/G)=%ld,%ld\n",
		  hpfmgr->switch_md1,
		  hpfmgr->switch_flash,
		  hpfmgr->switch_gpu, hpfmgr->loading_cpu, hpfmgr->loading_gpu);
#else
	pbm_debug
	    ("[***] Switch (MD1: %d, MD2: %d, GPU: %d, Flash: %d, CPU_volt: %d, GPU_volt: %d, CPU_num: %d)\n",
	     hpfmgr->switch_md1, hpfmgr->switch_md2, hpfmgr->switch_gpu, hpfmgr->switch_flash,
	     hpfmgr->cpu_volt, hpfmgr->gpu_volt, hpfmgr->cpu_num);

	pbm_debug
	    ("[***] Resource (DLPT: %ld, Leakage: %ld, MD: %ld, CPU: %ld, GPU: %ld, Flash: %ld)\n",
	     hpfmgr->loading_dlpt, hpfmgr->loading_leakage, hpfmgr->loading_md, hpfmgr->loading_cpu,
	     hpfmgr->loading_gpu, hpfmgr->loading_flash);
#endif
}

int hpf_get_power_leakage(void)
{
	struct hpf *hpfmgr = &hpf_ctrl;
	unsigned int leakage_cpu = 0, leakage_gpu = 0;

	leakage_cpu = mt_cpufreq_get_leakage_mw(0);
	leakage_gpu = mt_gpufreq_get_leakage_mw();

	hpfmgr->loading_leakage = leakage_cpu + leakage_gpu;

	pbm_debug("[%s] %ld=%d+%d\n", __func__, hpfmgr->loading_leakage, leakage_cpu, leakage_gpu);

	return hpfmgr->loading_leakage;
}

unsigned long hpf_get_power_cpu(void)
{
	struct hpf *hpfmgr = &hpf_ctrl;

	return hpfmgr->loading_cpu;
}

unsigned long hpf_get_power_gpu(void)
{
	struct hpf *hpfmgr = &hpf_ctrl;

	if (hpfmgr->switch_gpu)
		return hpfmgr->loading_gpu;
	else
		return 0;
}

unsigned long hpf_get_power_flash(void)
{
	struct hpf *hpfmgr = &hpf_ctrl;

	if (hpfmgr->switch_flash)
		return hpfmgr->loading_flash;
	else
		return 0;
}

unsigned long hpf_get_power_dlpt(void)
{
	struct hpf *hpfmgr = &hpf_ctrl;

	return hpfmgr->loading_dlpt;
}

unsigned long hpf_get_power_md(void)
{
	struct hpf *hpfmgr = &hpf_ctrl;

	if (hpfmgr->switch_md1 | hpfmgr->switch_md2)
		return hpfmgr->loading_md;
	else
		return 0;
}

static void pbm_allocate_budget_manager(void)
{
	int _dlpt = 0, leakage = 0, md = 0, dlpt = 0, cpu = 0, gpu = 0, flash = 0;
	int tocpu = 0, togpu = 0;
	int multiple = 0;
	int cpu_lower_bound = tscpu_get_min_cpu_pwr();

	mutex_lock(&pbm_table_lock);
	/* dump_kicker_info(); */
	leakage = hpf_get_power_leakage();
	md = hpf_get_power_md();
	dlpt = hpf_get_power_dlpt();
	cpu = hpf_get_power_cpu();
	gpu = hpf_get_power_gpu();
	flash = hpf_get_power_flash();
	mutex_unlock(&pbm_table_lock);

	/* no any resource can allocate */
	if (dlpt == 0) {
		pbm_debug("DLPT=0\n");
		return;
	}

	_dlpt = dlpt - (leakage + md + flash);
	if (_dlpt < 0)
		_dlpt = 0;

	/* if gpu no need resource, so all allocate to cpu */
	if (gpu == 0) {
		tocpu = _dlpt;

		/* check CPU lower bound */
		if (tocpu < cpu_lower_bound)
			tocpu = cpu_lower_bound;

		if (tocpu <= 0)
			tocpu = 1;

		mt_cpufreq_set_power_limit_by_pbm(tocpu);
	} else {
		multiple = (_dlpt * 1000) / (cpu + gpu);

		if (multiple > 0) {
			tocpu = (multiple * cpu) / 1000;
			togpu = (multiple * gpu) / 1000;
		} else {
			tocpu = 1;
			togpu = 1;
		}

		/* check CPU lower bound */
		if (tocpu < cpu_lower_bound) {
			tocpu = cpu_lower_bound;
			togpu = _dlpt - cpu_lower_bound;
		}

		if (tocpu <= 0)
			tocpu = 1;
		if (togpu <= 0)
			togpu = 1;

		mt_cpufreq_set_power_limit_by_pbm(tocpu);
		mt_gpufreq_set_power_limit_by_pbm(togpu);
	}

	if (mt_pbm_debug) {
		pbm_debug("(C/G)=%d,%d => (D/L/M/F/C/G)=%d,%d,%d,%d,%d,%d (Multi:%d),%d\n",
			 cpu, gpu, dlpt, leakage, md, flash, tocpu, togpu, multiple, cpu_lower_bound);
	} else {
		if ((cpu > tocpu) || (gpu > togpu))
			pbm_crit("(C/G)=%d,%d => (D/L/M/F/C/G)=%d,%d,%d,%d,%d,%d (Multi:%d),%d\n",
				 cpu, gpu, dlpt, leakage, md, flash, tocpu, togpu, multiple, cpu_lower_bound);
	}
}

static bool pbm_func_enable_check(void)
{
	struct pbm *pwrctrl = &pbm_ctrl;

	if (!pwrctrl->feature_en || !pwrctrl->pbm_drv_done) {
		pbm_crit("feature_en: %d, pbm_drv_done: %d\n", pwrctrl->feature_en, pwrctrl->pbm_drv_done);
		return false;
	}

	return true;
}

static bool pbm_update_table_info(enum pbm_kicker kicker, struct mrp *mrpmgr)
{
	struct hpf *hpfmgr = &hpf_ctrl;
	bool is_update = false;

	switch (kicker) {
	case KR_DLPT:		/* kicker 0 */
		if (hpfmgr->loading_dlpt != mrpmgr->loading_dlpt) {
			hpfmgr->loading_dlpt = mrpmgr->loading_dlpt;
			is_update = true;
		}
		break;
	case KR_MD:		/* kicker 1 */
		if (mrpmgr->idMD == MD1) {
			if (hpfmgr->switch_md1 != mrpmgr->switch_md) {
				hpfmgr->switch_md1 = mrpmgr->switch_md;
				is_update = true;
			}
		}
		if (mrpmgr->idMD == MD2) {
			if (hpfmgr->switch_md2 != mrpmgr->switch_md) {
				hpfmgr->switch_md2 = mrpmgr->switch_md;
				is_update = true;
			}
		}
		break;
	case KR_CPU:		/* kicker 2 */
		hpfmgr->cpu_volt = mrpmgr->cpu_volt;
		if (hpfmgr->loading_cpu != mrpmgr->loading_cpu
		    || hpfmgr->cpu_num != mrpmgr->cpu_num) {
			hpfmgr->loading_cpu = mrpmgr->loading_cpu;
			hpfmgr->cpu_num = mrpmgr->cpu_num;
			is_update = true;
		}
		break;
	case KR_GPU:		/* kicker 3 */
		hpfmgr->gpu_volt = mrpmgr->gpu_volt;
		if (hpfmgr->switch_gpu != mrpmgr->switch_gpu
		    || hpfmgr->loading_gpu != mrpmgr->loading_gpu) {
			hpfmgr->switch_gpu = mrpmgr->switch_gpu;
			hpfmgr->loading_gpu = mrpmgr->loading_gpu;
			is_update = true;
		}
		break;
	case KR_FLASH:		/* kicker 4 */
		if (hpfmgr->switch_flash != mrpmgr->switch_flash) {
			hpfmgr->switch_flash = mrpmgr->switch_flash;
			is_update = true;
		}
		break;
	default:
		pbm_crit("[%s] ERROR, unknown kicker [%d]\n", __func__, kicker);
		is_update = false;
		break;
	}

	return is_update;
}

static void pbm_wake_up_thread(enum pbm_kicker kicker, struct mrp *mrpmgr)
{
	if (atomic_read(&kthread_nreq) <= 0) {
		atomic_inc(&kthread_nreq);
		wake_up_process(pbm_thread);
	}

	while (kicker == KR_FLASH && mrpmgr->switch_flash == 1) {
		if (atomic_read(&kthread_nreq) == 0)
			return;
	}
}

static void mtk_power_budget_manager(enum pbm_kicker kicker, struct mrp *mrpmgr)
{
	bool pbm_enable = false;
	bool pbm_update = false;

	mutex_lock(&pbm_table_lock);
	pbm_update = pbm_update_table_info(kicker, mrpmgr);
	mutex_unlock(&pbm_table_lock);
	if (!pbm_update)
		return;

	pbm_enable = pbm_func_enable_check();
	if (!pbm_enable)
		return;

	pbm_wake_up_thread(kicker, mrpmgr);
}

/*
 * kicker: 0
 * who call : PMIC
 * i_max: mA
 * condition: persentage decrease 1%, then update i_max
 */
void kicker_pbm_by_dlpt(unsigned int i_max)
{
	struct pbm *pwrctrl = &pbm_ctrl;
	struct mrp mrpmgr;

	mrpmgr.loading_dlpt = ma_to_mw(i_max);

	if (BIT_CHECK(pwrctrl->hpf_en, KR_DLPT))
		mtk_power_budget_manager(KR_DLPT, &mrpmgr);
}

/*
 * kicker: 1
 * who call : MD
 * condition: on/off
 */
void kicker_pbm_by_md(enum md_id id, bool status)
{
	struct pbm *pwrctrl = &pbm_ctrl;
	struct mrp mrpmgr;

	mrpmgr.idMD = id;
	mrpmgr.switch_md = status;

	if (BIT_CHECK(pwrctrl->hpf_en, KR_MD))
		mtk_power_budget_manager(KR_MD, &mrpmgr);
}

/*
 * kicker: 2
 * who call : CPU
 * loading: mW
 * condition: opp changed
 */
void kicker_pbm_by_cpu(unsigned int loading, int core, int voltage)
{
	struct pbm *pwrctrl = &pbm_ctrl;
	struct mrp mrpmgr;

	mrpmgr.loading_cpu = loading;
	mrpmgr.cpu_num = core;
	mrpmgr.cpu_volt = voltage;

	if (BIT_CHECK(pwrctrl->hpf_en, KR_CPU))
		mtk_power_budget_manager(KR_CPU, &mrpmgr);
}

/*
 * kicker: 3
 * who call : GPU
 * loading: mW
 * condition: opp changed
 */
void kicker_pbm_by_gpu(bool status, unsigned int loading, int voltage)
{
	struct pbm *pwrctrl = &pbm_ctrl;
	struct mrp mrpmgr;

	mrpmgr.switch_gpu = status;
	mrpmgr.loading_gpu = loading;
	mrpmgr.gpu_volt = voltage;

	if (BIT_CHECK(pwrctrl->hpf_en, KR_GPU))
		mtk_power_budget_manager(KR_GPU, &mrpmgr);
}

/*
 * kicker: 4
 * who call : Flash
 * condition: on/off
 */
void kicker_pbm_by_flash(bool status)
{
	struct pbm *pwrctrl = &pbm_ctrl;
	struct mrp mrpmgr;

	mrpmgr.switch_flash = status;

	if (BIT_CHECK(pwrctrl->hpf_en, KR_FLASH))
		mtk_power_budget_manager(KR_FLASH, &mrpmgr);
}

/* extern int g_dlpt_stop; in mt_pbm.h*/
int g_dlpt_state_sync = 0;

static int pbm_thread_handle(void *data)
{
	while (1) {

		set_current_state(TASK_INTERRUPTIBLE);

		if (kthread_should_stop())
			break;

		if (atomic_read(&kthread_nreq) <= 0) {
			schedule();
			continue;
		}

		mutex_lock(&pbm_mutex);
		if (g_dlpt_need_do == 1) {
			if (g_dlpt_stop == 0) {
				pbm_allocate_budget_manager();
				g_dlpt_state_sync = 0;
			} else {
				pbm_err("DISABLE PBM\n");

				if (g_dlpt_state_sync == 0) {
					mt_cpufreq_set_power_limit_by_pbm(0);
					mt_gpufreq_set_power_limit_by_pbm(0);
					g_dlpt_state_sync = 1;
					pbm_err("Release DLPT limit\n");
				}
			}
		}
		atomic_dec(&kthread_nreq);
		mutex_unlock(&pbm_mutex);
	}

	__set_current_state(TASK_RUNNING);

	return 0;
}

static int create_pbm_kthread(void)
{
	struct pbm *pwrctrl = &pbm_ctrl;

	pbm_thread = kthread_create(pbm_thread_handle, (void *)NULL, "pbm");
	if (IS_ERR(pbm_thread))
		return PTR_ERR(pbm_thread);

	wake_up_process(pbm_thread);
	pwrctrl->pbm_drv_done = 1;	/* avoid other hpf call thread before thread init done */

	return 0;
}

static int
_mt_pbm_pm_callback(struct notifier_block *nb,
		unsigned long action, void *ptr)
{
	switch (action) {

	case PM_SUSPEND_PREPARE:
		pbm_err("PM_SUSPEND_PREPARE:start\n");
		mutex_lock(&pbm_mutex);
		g_dlpt_need_do = 0;
		mutex_unlock(&pbm_mutex);
		pbm_err("PM_SUSPEND_PREPARE:end\n");
		break;

	case PM_HIBERNATION_PREPARE:
		break;

	case PM_POST_SUSPEND:
		pbm_err("PM_POST_SUSPEND:start\n");
		mutex_lock(&pbm_mutex);
		g_dlpt_need_do = 1;
		mutex_unlock(&pbm_mutex);
		pbm_err("PM_POST_SUSPEND:end\n");
		break;

	case PM_POST_HIBERNATION:
		break;

	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

#if 1 /* CONFIG_PBM_PROC_FS */
/*
 * show current debug status
 */
static int mt_pbm_debug_proc_show(struct seq_file *m, void *v)
{
	if (mt_pbm_debug)
		seq_puts(m, "pbm debug enabled\n");
	else
		seq_puts(m, "pbm debug disabled\n");

	return 0;
}

/*
 * enable debug message
 */
static ssize_t mt_pbm_debug_proc_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	int debug = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	/* if (sscanf(desc, "%d", &debug) == 1) { */
	if (kstrtoint(desc, 10, &debug) == 0) {
		if (debug == 0)
			mt_pbm_debug = 0;
		else if (debug == 1)
			mt_pbm_debug = 1;
		else
			pbm_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		pbm_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

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
	.owner		= THIS_MODULE,				\
	.open		= mt_ ## name ## _proc_open,	\
	.read		= seq_read,					\
	.llseek		= seq_lseek,					\
	.release	= single_release,				\
}

#define PROC_ENTRY(name)	{__stringify(name), &mt_ ## name ## _proc_fops}

PROC_FOPS_RW(pbm_debug);

static int mt_pbm_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(pbm_debug),
	};

	dir = proc_mkdir("pbm", NULL);

	if (!dir) {
		pbm_err("fail to create /proc/pbm @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
		    (entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			pbm_err("@%s: create /proc/pbm/%s failed\n", __func__,
				    entries[i].name);
	}

	return 0;
}
#endif	/* CONFIG_PBM_PROC_FS */

static int __init pbm_module_init(void)
{
	int ret = 0;

	#if 1 /* CONFIG_PBM_PROC_FS */
	mt_pbm_create_procfs();
	#endif

	pm_notifier(_mt_pbm_pm_callback, 0);

	register_dlpt_notify(&kicker_pbm_by_dlpt, DLPT_PRIO_PBM);
	ret = create_pbm_kthread();

	pbm_crit("pbm_module_init : Done\n");

	if (ret) {
		pbm_err("FAILED TO CREATE PBM KTHREAD\n");
		return ret;
	}
	return ret;
}

#else				/* #ifndef DISABLE_PBM_FEATURE */

void kicker_pbm_by_dlpt(unsigned int i_max)
{
}

void kicker_pbm_by_md(enum md_id id, bool status)
{
}

void kicker_pbm_by_cpu(unsigned int loading, int core, int voltage)
{
}

void kicker_pbm_by_gpu(bool status, unsigned int loading, int voltage)
{
}

void kicker_pbm_by_flash(bool status)
{
}

static int __init pbm_module_init(void)
{
	pr_crit("DISABLE_PBM_FEATURE is defined.\n");
	return 0;
}

#endif				/* #ifndef DISABLE_PBM_FEATURE */

static void __exit pbm_module_exit(void)
{

}
module_init(pbm_module_init);
module_exit(pbm_module_exit);

MODULE_AUTHOR("Max Yu <max.yu@mediatek.com>");
MODULE_DESCRIPTION("PBM Driver v0.1");
