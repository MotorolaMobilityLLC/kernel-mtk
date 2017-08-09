/*****************************************************************************
 *
 * Filename:
 * ---------
 *    pmic_irq.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines PMIC functions
 *
 * Author:
 * -------
 * Jimmy-YJ Huang
 *
 ****************************************************************************/
#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#endif
#include <linux/uaccess.h>

/*#include <mach/eint.h> TBD*/
#include <mach/mt_pmic_wrap.h>
#include "pwrap_hal.h"
#include "include/sub_upmu_hw.h"
#include "include/sub_pmic.h"
#include "include/sub_pmic_irq.h"

/*---IPI Mailbox define---*/
/*
#define IPIMB
*/

/*****************************************************************************
 * Global variable
 ******************************************************************************/
int g_mt6337_irq;
unsigned int g_eint_mt6337_num = 177;
unsigned int g_cust_eint_mt6337_debounce_cn = 1;
unsigned int g_cust_eint_mt6337_type = 4;
unsigned int g_cust_eint_mt6337_debounce_en = 1;

/*****************************************************************************
 * PMIC extern function
 ******************************************************************************/

#define PMIC_DEBUG_PR_DBG
/*
#define PMICTAG                "[PMIC] "
#if defined PMIC_DEBUG_PR_DBG
#define PMICLOG(fmt, arg...)   pr_err(PMICTAG fmt, ##arg)
#else
#define PMICLOG(fmt, arg...)
#endif
*/

/*****************************************************************************
 * interrupt Setting
 ******************************************************************************/
static struct mt6337_interrupt_bit interrupt_status0[] = {
	MT6337_S_INT_GEN(RG_INT_EN_THR_H),
	MT6337_S_INT_GEN(RG_INT_EN_THR_L),
	MT6337_S_INT_GEN(RG_INT_EN_AUDIO),
	MT6337_S_INT_GEN(RG_INT_EN_MAD),
	MT6337_S_INT_GEN(RG_INT_EN_ACCDET),
	MT6337_S_INT_GEN(RG_INT_EN_ACCDET_EINT),
	MT6337_S_INT_GEN(RG_INT_EN_ACCDET_EINT1),
	MT6337_S_INT_GEN(RG_INT_EN_ACCDET_NEGV),
	MT6337_S_INT_GEN(RG_INT_EN_PMU_THR),
	MT6337_S_INT_GEN(RG_INT_EN_LDO_VA18_OC),
	MT6337_S_INT_GEN(RG_INT_EN_LDO_VA25_OC),
	MT6337_S_INT_GEN(RG_INT_EN_LDO_VA18_PG),
	MT6337_S_INT_GEN(NO_USE),/*RG_INT_EN_CON0*/
	MT6337_S_INT_GEN(NO_USE),/*RG_INT_EN_CON0*/
	MT6337_S_INT_GEN(NO_USE),/*RG_INT_EN_CON0*/
	MT6337_S_INT_GEN(NO_USE),/*RG_INT_EN_CON0*/
};

struct mt6337_interrupts sub_interrupts[] = {
	MT6337_M_INTS_GEN(MT6337_INT_STATUS0, MT6337_INT_CON0,
			MT6337_INT_CON1, interrupt_status0)
};

/*int interrupts_size = ARRAY_SIZE(interrupts);*/

/*****************************************************************************
 * PMIC Interrupt service
 ******************************************************************************/
static DEFINE_MUTEX(mt6337_mutex);
struct task_struct *mt6337_thread_handle;

#if !defined CONFIG_HAS_WAKELOCKS
struct wakeup_source pmicThread_lock_mt6337;
#else
struct wake_lock pmicThread_lock_mt6337;
#endif

void wake_up_pmic_mt6337(void)
{
	PMICLOG("[wake_up_pmic_mt6337]\r\n");
	wake_up_process(mt6337_thread_handle);

#if !defined CONFIG_HAS_WAKELOCKS
	__pm_stay_awake(&pmicThread_lock_mt6337);
#else
	wake_lock(&pmicThread_lock_mt6337);
#endif
}
EXPORT_SYMBOL(wake_up_pmic_mt6337);

/*
#ifdef CONFIG_MTK_LEGACY
void mt6337_eint_irq(void)
{
	PMICLOG("[mt6337_eint_irq] receive interrupt\n");
	wake_up_pmic_mt6337();
	return;
}
#else
*/
irqreturn_t mt6337_eint_irq(int irq, void *desc)
{
	/*PMICLOG("[mt6337_eint_irq] receive interrupt\n");*/
	wake_up_pmic_mt6337();
	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}
/*#endif*/

void mt6337_enable_interrupt(MT6337_IRQ_ENUM intNo, unsigned int en, char *str)
{
	unsigned int shift, no;

	shift = intNo / MT6337_INT_WIDTH;
	no = intNo % MT6337_INT_WIDTH;

	if (shift >= ARRAY_SIZE(sub_interrupts)) {
		PMICLOG("[mt6337_enable_interrupt] fail intno=%d \r\n", intNo);
		return;
	}

	PMICLOG("[mt6337_enable_interrupt] intno=%d en=%d str=%s shf=%d no=%d [0x%x]=0x%x\r\n",
		intNo, en, str, shift, no, sub_interrupts[shift].en,
		mt6337_upmu_get_reg_value(sub_interrupts[shift].en));

	if (en == 1)
		mt6337_config_interface(sub_interrupts[shift].set, 0x1, 0x1, no);
	else if (en == 0)
		mt6337_config_interface(sub_interrupts[shift].clear, 0x1, 0x1, no);

	PMICLOG("[mt6337_enable_interrupt] after [0x%x]=0x%x\r\n",
		sub_interrupts[shift].en, mt6337_upmu_get_reg_value(sub_interrupts[shift].en));

}

void mt6337_mask_interrupt(MT6337_IRQ_ENUM intNo, char *str)
{
	unsigned int shift, no;

	shift = intNo / MT6337_INT_WIDTH;
	no = intNo % MT6337_INT_WIDTH;

	if (shift >= ARRAY_SIZE(sub_interrupts)) {
		PMICLOG("[mt6337_mask_interrupt] fail intno=%d \r\n", intNo);
		return;
	}

	/*---the mask in MT6337 needs 'logical not'---*/
	PMICLOG("[mt6337_mask_interrupt] intno=%d str=%s shf=%d no=%d [0x%x]=0x%x\r\n",
		intNo, str, shift, no, sub_interrupts[shift].mask,
		~(mt6337_upmu_get_reg_value(sub_interrupts[shift].mask)));

	/*---To mask interrupt needs to clear mask bit---*/
	mt6337_config_interface(sub_interrupts[shift].mask_clear, 0x1, 0x1, no);

	/*---the mask in MT6337 needs 'logical not'---*/
	PMICLOG("[mt6337_mask_interrupt] after [0x%x]=0x%x\r\n",
		sub_interrupts[shift].mask_set, ~(mt6337_upmu_get_reg_value(sub_interrupts[shift].mask)));
}

void mt6337_unmask_interrupt(MT6337_IRQ_ENUM intNo, char *str)
{
	unsigned int shift, no;

	shift = intNo / MT6337_INT_WIDTH;
	no = intNo % MT6337_INT_WIDTH;

	if (shift >= ARRAY_SIZE(sub_interrupts)) {
		PMICLOG("[mt6337_unmask_interrupt] fail intno=%d \r\n", intNo);
		return;
	}

	/*---the mask in MT6337 needs 'logical not'---*/
	PMICLOG("[mt6337_unmask_interrupt] intno=%d str=%s shf=%d no=%d [0x%x]=0x%x\r\n",
		intNo, str, shift, no, sub_interrupts[shift].mask,
		~(mt6337_upmu_get_reg_value(sub_interrupts[shift].mask)));

	/*---To unmask interrupt needs to set mask bit---*/
	mt6337_config_interface(sub_interrupts[shift].mask_set, 0x1, 0x1, no);

	/*---the mask in MT6337 needs 'logical not'---*/
	PMICLOG("[mt6337_mask_interrupt] after [0x%x]=0x%x\r\n",
		sub_interrupts[shift].mask_set, ~(mt6337_upmu_get_reg_value(sub_interrupts[shift].mask)));
}

void mt6337_register_interrupt_callback(MT6337_IRQ_ENUM intNo, void (EINT_FUNC_PTR) (void))
{
	unsigned int shift, no;

	shift = intNo / MT6337_INT_WIDTH;
	no = intNo % MT6337_INT_WIDTH;

	if (shift >= ARRAY_SIZE(sub_interrupts)) {
		PMICLOG("[mt6337_register_interrupt_callback] fail intno=%d\r\n", intNo);
		return;
	}

	PMICLOG("[mt6337_register_interrupt_callback] intno=%d\r\n", intNo);

	sub_interrupts[shift].interrupts[no].callback = EINT_FUNC_PTR;

}

void MT6337_EINT_SETTING(void)
{
	struct device_node *node = NULL;
	int ret = 0;
	u32 ints[2] = { 0, 0 };
/*
	mt6337_upmu_set_reg_value(MT6337_INT_CON0, 0);
	mt6337_register_interrupt_callback(0, pwrkey_int_handler);
	mt6337_enable_interrupt(1, 1, "PMIC");
*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6337-pmic");
	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));

		g_mt6337_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(g_mt6337_irq, (irq_handler_t) mt6337_eint_irq,
			IRQF_TRIGGER_NONE, "mt6337-eint", NULL);
		if (ret > 0)
			PMICLOG("EINT IRQ LINENNOT AVAILABLE\n");
		enable_irq(g_mt6337_irq);
	} else
		PMICLOG("can't find compatible node\n");

	PMICLOG("[CUST_EINT_MT6337] CUST_EINT_MT_PMIC_MT6337_NUM=%d\n", g_eint_mt6337_num);
	PMICLOG("[CUST_EINT_MT6337] CUST_EINT_PMIC_DEBOUNCE_CN=%d\n", g_cust_eint_mt6337_debounce_cn);
	PMICLOG("[CUST_EINT_MT6337] CUST_EINT_PMIC_TYPE=%d\n", g_cust_eint_mt6337_type);
	PMICLOG("[CUST_EINT_MT6337] CUST_EINT_PMIC_DEBOUNCE_EN=%d\n", g_cust_eint_mt6337_debounce_en);

}

static void mt6337_int_handler(void)
{
	unsigned char i, j;
	unsigned int ret;

	for (i = 0; i < ARRAY_SIZE(sub_interrupts); i++) {
		unsigned int int_status_val = 0;

		if (sub_interrupts[i].address != 0) {
			int_status_val = mt6337_upmu_get_reg_value(sub_interrupts[i].address);
			PMICLOG("[MT6337_INT] addr[0x%x]=0x%x\n", sub_interrupts[i].address, int_status_val);

			for (j = 0; j < MT6337_INT_WIDTH; j++) {
				if ((int_status_val) & (1 << j)) {
					PMICLOG("[MT6337_INT][%s]\n", sub_interrupts[i].interrupts[j].name);
					if (sub_interrupts[i].interrupts[j].callback != NULL) {
						sub_interrupts[i].interrupts[j].callback();
						sub_interrupts[i].interrupts[j].times++;
					}
				ret = mt6337_config_interface(sub_interrupts[i].address, 0x1, 0x1, j);
				}
			}
		}
	}
}

int mt6337_thread_kthread(void *x)
{
	unsigned int i;
	unsigned int int_status_val = 0;
#ifdef IPIMB
#else
	unsigned int pwrap_eint_status = 0;
#endif
	struct sched_param param = {.sched_priority = 98 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	PMICLOG("[MT6337_INT] enter\n");

	/* Run on a process content */
	while (1) {
		mutex_lock(&mt6337_mutex);
#ifdef IPIMB
#else
		pwrap_eint_status = pmic_wrap_eint_status();
		PMICLOG("[MT6337_INT] pwrap_eint_status=0x%x\n", pwrap_eint_status);
#endif

		mt6337_int_handler();

#ifdef IPIMB
#else
		pmic_wrap_eint_clr(0x0);
#endif
		/*PMICLOG("[MT6337_INT] pmic_wrap_eint_clr(0x0);\n");*/

		for (i = 0; i < ARRAY_SIZE(sub_interrupts); i++) {
			if (sub_interrupts[i].address != 0) {
				int_status_val = mt6337_upmu_get_reg_value(sub_interrupts[i].address);
				PMICLOG("[MT6337_INT] after ,int_status_val[0x%x]=0x%x\n",
					sub_interrupts[i].address, int_status_val);
			}
		}


		mdelay(1);

		mutex_unlock(&mt6337_mutex);
#if !defined CONFIG_HAS_WAKELOCKS
		__pm_relax(&pmicThread_lock_mt6337);
#else
		wake_unlock(&pmicThread_lock_mt6337);
#endif

		set_current_state(TASK_INTERRUPTIBLE);
/*
#ifdef CONFIG_MTK_LEGACY
		mt_eint_unmask(g_eint_mt6337_num);
#else
*/
		if (g_mt6337_irq != 0)
			enable_irq(g_mt6337_irq);
/*#endif*/
		schedule();
	}

	return 0;
}


MODULE_AUTHOR("Jimmy-YJ Huang");
MODULE_DESCRIPTION("MT PMIC Interrupt Driver");
MODULE_LICENSE("GPL");
