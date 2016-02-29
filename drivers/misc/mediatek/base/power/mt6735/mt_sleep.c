#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/suspend.h>
#include <linux/console.h>
/* #include <linux/aee.h> */
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#include <mt-plat/sync_write.h>
#include "mt_sleep.h"
#include "mt_spm.h"
#include "mt_spm_sleep.h"
#include "mt_spm_idle.h"
#include "mt_spm_mtcmos.h"
#include "mt_clkmgr.h"
/*
#if defined(CONFIG_ARCH_MT6735M)
#include <mt_soc_afe_control.h>
#endif
*/
/**************************************
 * only for internal debug
 **************************************/
#ifdef CONFIG_MTK_LDVT
#define SLP_SLEEP_DPIDLE_EN         1
#define SLP_REPLACE_DEF_WAKESRC     1
#define SLP_SUSPEND_LOG_EN          1
#else
/*
#if defined(CONFIG_ARCH_MT6735M)
#define SLP_SLEEP_DPIDLE_EN         1
#else
*/
#define SLP_SLEEP_DPIDLE_EN         0
/*
#endif
*/
#define SLP_REPLACE_DEF_WAKESRC     0
#define SLP_SUSPEND_LOG_EN          1
#endif

/**************************************
 * SW code for suspend
 **************************************/
#define slp_read(addr)              (*(volatile u32 *)(addr))
#define slp_write(addr, val)        mt65xx_reg_sync_writel(val, addr)
/*
#define slp_emerg(fmt, args...)     pr_debug(KERN_EMERG "[SLP] " fmt, ##args)
#define slp_alert(fmt, args...)     pr_debug(KERN_ALERT "[SLP] " fmt, ##args)
#define slp_crit(fmt, args...)      pr_debug(KERN_CRIT "[SLP] " fmt, ##args)
#define slp_crit2(fmt, args...)      pr_debug(KERN_CRIT "[SLP] " fmt, ##args)
#define slp_error(fmt, args...)     pr_err(KERN_ERR "[SLP] " fmt, ##args)
#define slp_warning(fmt, args...)   pr_debug(KERN_WARNING "[SLP] " fmt, ##args)
#define slp_notice(fmt, args...)    pr_debug(KERN_NOTICE "[SLP] " fmt, ##args)
#define slp_info(fmt, args...)      pr_debug(KERN_INFO "[SLP] " fmt, ##args)
#define slp_debug(fmt, args...)     pr_debug(KERN_DEBUG "[SLP] " fmt, ##args)
*/
#define slp_emerg(fmt, args...)     pr_debug("[SLP] " fmt, ##args)
#define slp_alert(fmt, args...)     pr_debug("[SLP] " fmt, ##args)
#define slp_crit(fmt, args...)      pr_debug("[SLP] " fmt, ##args)
#define slp_crit2(fmt, args...)     pr_debug("[SLP] " fmt, ##args)
#define slp_error(fmt, args...)     pr_err("[SLP] " fmt, ##args)
#define slp_warning(fmt, args...)   pr_debug("[SLP] " fmt, ##args)
#define slp_notice(fmt, args...)    pr_debug("[SLP] " fmt, ##args)
#define slp_info(fmt, args...)      pr_debug("[SLP] " fmt, ##args)
#define slp_debug(fmt, args...)     pr_debug("[SLP] " fmt, ##args)
static DEFINE_SPINLOCK(slp_lock);

static wake_reason_t slp_wake_reason = WR_NONE;
/* static bool slp_ck26m_on = 0; */
/* static bool slp_dump_gpio = 0; */
bool slp_ck26m_on = 0;
static bool slp_pars_dpd = 1;

static bool slp_chk_golden = 1;
bool slp_dump_gpio = 0;
static bool slp_dump_regs = 1;
static bool slp_check_mtcmos_pll = 1;

static u32 slp_spm_flags = {

#if defined(CONFIG_ARCH_MT6735)
		0
#elif defined(CONFIG_ARCH_MT6735M)
		0
#elif defined(CONFIG_ARCH_MT6753)
#ifdef SPM_LEGACY_SUSPEND
	SPM_CPU_PDN_DIS |
	SPM_INFRA_PDN_DIS |
	SPM_DDRPHY_PDN_DIS |
	SPM_PASR_DIS |
	SPM_CPU_DVS_DIS |
	SPM_DISABLE_ATF_ABORT
#else
	0
#endif
#else
/* ERROR */
#endif
};

#if SLP_SLEEP_DPIDLE_EN
static u32 slp_spm_deepidle_flags = {
	0
};
#endif

/* static u32 slp_spm_data = 0; */
u32 slp_spm_data = 0;


#if 1
static int slp_suspend_ops_valid(suspend_state_t state)
{
		return state == PM_SUSPEND_MEM;
}

static int slp_suspend_ops_begin(suspend_state_t state)
{
    /* legacy log */

		slp_notice("@@@Chip_pm_begin(%u)(%u)@@@\n", is_cpu_pdn(slp_spm_flags), is_infra_pdn(slp_spm_flags));


		slp_wake_reason = WR_NONE;

		return 0;
}

static int slp_suspend_ops_prepare(void)
{
    /* legacy log */

		slp_crit2("@@@Chip_pm_prepare@@@\n");

		return 0;
}

#ifdef CONFIG_MTKPASR
/* PASR/DPD Preliminary operations */
/* extern void mtkpasr_phaseone_ops(void); */
static int slp_suspend_ops_prepare_late(void)
{
	slp_notice("[%s]\n", __func__);
	mtkpasr_phaseone_ops();
	return 0;
}
static void slp_suspend_ops_wake(void)
{
	slp_notice("[%s]\n", __func__);
}

/* PASR/DPD SW operations */
/*
extern int configure_mrw_pasr(u32 segment_rank0, u32 segment_rank1);
extern int pasr_enter(u32 *sr, u32 *dpd);
extern int pasr_exit(void);
extern unsigned long mtkpasr_enable_sr;
extern void enter_pasr_dpd_config(unsigned char segment_rank0, unsigned char segment_rank1);
extern void exit_pasr_dpd_config(void);
*/
static int enter_pasrdpd(void)
{
	int error = 0;
	u32 sr = 0, dpd = 0;

	slp_crit2("@@@[%s]@@@\n", __func__);

	/* Setup SPM wakeup event firstly */
	spm_set_wakeup_src_check();

	/* Start PASR/DPD SW operations */
	error = pasr_enter(&sr, &dpd);

	if (error) {
		slp_crit2("[PM_WAKEUP] Failed to enter PASR!\n");
	} else {
		/* Call SPM/DPD control API */
		slp_crit2("MR17[0x%x] DPD[0x%x]\n", sr, dpd);
		/* Should configure SR */
		if (mtkpasr_enable_sr == 0) {
			sr = 0x0;
			slp_crit2("[%s][%d] No configuration on SR\n", __func__, __LINE__);
		}
		/* Configure PASR */
		enter_pasr_dpd_config((sr & 0xFF), (sr >> 0x8));
	}
	slp_crit2("Bye [%s]\n", __func__);

	return error;
}
static void leave_pasrdpd(void)
{

	slp_crit2("@@@[%s]@@@\n", __func__);


	/* Disable PASR */
	exit_pasr_dpd_config();

	slp_crit2("[%d]\n", __LINE__);
	/* End PASR/DPD SW operations */
	pasr_exit();
	slp_crit2("Bye [%s]\n", __func__);
}
#endif


static int slp_suspend_ops_enter(suspend_state_t state)
{
	int ret = 0;
/*
	#if defined(CONFIG_ARCH_MT6735M)
		int fm_radio_is_playing = 0;
			if (ConditionEnterSuspend() == true)
				fm_radio_is_playing = 0;
		else
			fm_radio_is_playing = 1;
	#endif
*/
#ifdef CONFIG_MTKPASR
    /* PASR SW operations */
		enter_pasrdpd();
#endif

    /* legacy log */

		slp_crit2("@@@Chip_pm_enter@@@\n");



#if 0
		if (slp_dump_gpio)
			gpio_dump_regs();
#endif

#if 0
		if (slp_dump_regs)
			slp_dump_pm_regs();
#endif

		if (slp_check_mtcmos_pll)
			slp_check_pm_mtcmos_pll();

		if (!spm_cpusys0_can_power_down()) {
				slp_error("CANNOT SLEEP DUE TO CPU1/2/3 PON\n");
				ret = -EPERM;
				goto LEAVE_SLEEP;
		}

		if (is_infra_pdn(slp_spm_flags) && !is_cpu_pdn(slp_spm_flags)) {
				slp_error("CANNOT SLEEP DUE TO INFRA PDN BUT CPU PON\n");
				ret = -EPERM;
				goto LEAVE_SLEEP;
		}

/*
#if defined(CONFIG_ARCH_MT6735M)
#if SLP_SLEEP_DPIDLE_EN
		if ((slp_ck26m_on) || (fm_radio_is_playing))
			slp_wake_reason = spm_go_to_sleep_dpidle(slp_spm_deepidle_flags, slp_spm_data);
		else
#endif
#endif
*/
		slp_wake_reason = spm_go_to_sleep(slp_spm_flags, slp_spm_data);

LEAVE_SLEEP:
#ifdef CONFIG_MTKPASR
    /* PASR SW operations */
		leave_pasrdpd();
#endif
		return ret;
}

static void slp_suspend_ops_finish(void)
{
    /* legacy log */

		slp_crit2("@@@Chip_pm_finish@@@\n");

}

static void slp_suspend_ops_end(void)
{
    /* legacy log */

		slp_notice("@@@Chip_pm_end@@@\n");

}

static const struct platform_suspend_ops slp_suspend_ops = {
		.valid      = slp_suspend_ops_valid,
		.begin      = slp_suspend_ops_begin,
		.prepare    = slp_suspend_ops_prepare,
		.enter      = slp_suspend_ops_enter,
		.finish     = slp_suspend_ops_finish,
		.end        = slp_suspend_ops_end,
#ifdef CONFIG_MTKPASR
		.prepare_late = slp_suspend_ops_prepare_late,
		.wake	  = slp_suspend_ops_wake,
#endif
};
#endif

__attribute__((weak)) int spm_set_dpidle_wakesrc(u32 wakesrc, bool enable, bool replace)
{
	return 0;
}

/*
 * wakesrc : WAKE_SRC_XXX
 * enable  : enable or disable @wakesrc
 * ck26m_on: if true, mean @wakesrc needs 26M to work
 */
int slp_set_wakesrc(u32 wakesrc, bool enable, bool ck26m_on)
{
		int r;
		unsigned long flags;
			slp_notice("wakesrc = 0x%x, enable = %u, ck26m_on = %u\n",
				wakesrc, enable, ck26m_on);

#if SLP_REPLACE_DEF_WAKESRC
		if (wakesrc & WAKE_SRC_CFG_KEY)
#else
		if (!(wakesrc & WAKE_SRC_CFG_KEY))
#endif
			return -EPERM;

	spin_lock_irqsave(&slp_lock, flags);

#if SLP_REPLACE_DEF_WAKESRC
		if (ck26m_on)
			r = spm_set_dpidle_wakesrc(wakesrc, enable, true);
	else
			r = spm_set_sleep_wakesrc(wakesrc, enable, true);
#else
		if (ck26m_on)
				r = spm_set_dpidle_wakesrc(wakesrc & ~WAKE_SRC_CFG_KEY, enable, false);
	else
			r = spm_set_sleep_wakesrc(wakesrc & ~WAKE_SRC_CFG_KEY, enable, false);
#endif

		if (!r)
			slp_ck26m_on = ck26m_on;
		spin_unlock_irqrestore(&slp_lock, flags);
		return r;
}

wake_reason_t slp_get_wake_reason(void)
{
		return slp_wake_reason;
}

bool slp_will_infra_pdn(void)
{
		return is_infra_pdn(slp_spm_flags);
}

/*
 * en: 1: enable pasr, 0: disable pasr
 * value: pasr setting (RK1, MR17 for RK0)
 */
void slp_pasr_en(bool en, u32 value)
{
		if (slp_pars_dpd) {
				if (en) {
						slp_spm_flags &= ~SPM_PASR_DIS;
						slp_spm_data = value;
				} else {
						slp_spm_flags |= SPM_PASR_DIS;
						slp_spm_data = 0;
				}
		}
}

void slp_module_init(void)
{
		spm_output_sleep_option();
		slp_notice("SLEEP_DPIDLE_EN:%d, REPLACE_DEF_WAKESRC:%d, SUSPEND_LOG_EN:%d\n",
			SLP_SLEEP_DPIDLE_EN, SLP_REPLACE_DEF_WAKESRC, SLP_SUSPEND_LOG_EN);
		suspend_set_ops(&slp_suspend_ops);
#if SLP_SUSPEND_LOG_EN
		console_suspend_enabled = 0;
#endif
}
/*
#ifdef CONFIG_MTK_FPGA
static int __init spm_fpga_module_init(void)
{
    spm_module_init();
    slp_module_init();
		return 0;
}
arch_initcall(spm_fpga_module_init);
#else
#endif
*/
module_param(slp_ck26m_on, bool, 0644);
module_param(slp_pars_dpd, bool, 0644);
module_param(slp_spm_flags, uint, 0644);

module_param(slp_chk_golden, bool, 0644);
module_param(slp_dump_gpio, bool, 0644);
module_param(slp_dump_regs, bool, 0644);
module_param(slp_check_mtcmos_pll, bool, 0644);

MODULE_DESCRIPTION("Sleep Driver v0.1");
