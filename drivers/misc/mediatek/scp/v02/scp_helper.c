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

#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by vmalloc */
#include <linux/uaccess.h>      /* needed by copy_to_user */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/slab.h>         /* needed by kmalloc */
#include <linux/poll.h>         /* needed by poll */
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_fdt.h>
#include <linux/ioport.h>
#include <linux/wakelock.h>
#include <linux/io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/aee.h>
#include <linux/delay.h>
#include "scp_feature_define.h"
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_excep.h"
#include "scp_dvfs.h"

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#include "scp_reservedmem_define.h"
#endif


#if ENABLE_SCP_EMI_PROTECTION
#include <emi_mpu.h>
#endif

/* scp awake timout count definition*/
#define SCP_AWAKE_TIMEOUT 5000
/* scp semaphore timout count definition*/
#define SEMAPHORE_TIMEOUT 5000
#define SEMAPHORE_3WAY_TIMEOUT 5000
/* scp ready timout definition*/
#define SCP_READY_TIMEOUT (30 * HZ) /* 30 seconds*/
#define SCP_A_TIMER 0
#define SCP_B_TIMER 1

/* scp ready status for notify*/
unsigned int scp_ready[SCP_CORE_TOTAL];

/* scp enable status*/
unsigned int scp_enable[SCP_CORE_TOTAL];

/* scp dvfs variable*/
unsigned int scp_expected_freq;
unsigned int scp_current_freq;

#ifdef CFG_RECOVERY_SUPPORT
unsigned int scp_recovery_flag[SCP_CORE_TOTAL];
#define SCP_A_RECOVERY_OK	0x44
#define SCP_B_RECOVERY_OK	0x45
#endif

phys_addr_t scp_mem_base_phys;
phys_addr_t scp_mem_base_virt;
phys_addr_t scp_mem_size;
struct scp_regs scpreg;

unsigned char *scp_send_buff[SCP_CORE_TOTAL];
unsigned char *scp_recv_buff[SCP_CORE_TOTAL];

static struct workqueue_struct *scp_workqueue;
#if SCP_BOOT_TIME_OUT_MONITOR
static struct timer_list scp_ready_timer[SCP_CORE_TOTAL];
#endif
static struct scp_work_struct scp_A_notify_work;
static struct scp_work_struct scp_B_notify_work;
static struct scp_work_struct scp_timeout_work;

static DEFINE_MUTEX(scp_A_notify_mutex);
static DEFINE_MUTEX(scp_B_notify_mutex);
static DEFINE_MUTEX(scp_feature_mutex);

char *core_ids[SCP_CORE_TOTAL] = {"SCP A", "SCP B"};
DEFINE_SPINLOCK(scp_awake_spinlock);
/* set flag after driver initial done */
static bool driver_init_done;
unsigned char **scp_swap_buf;

typedef struct {
	u64 start;
	u64 size;
} mem_desc_t;

/*
 * memory copy to scp sram
 * @param trg: trg address
 * @param src: src address
 * @param size: memory size
 */
void memcpy_to_scp(void __iomem *trg, const void *src, int size)
{
	int i;
	u32 __iomem *t = trg;
	const u32 *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}


/*
 * memory copy from scp sram
 * @param trg: trg address
 * @param src: src address
 * @param size: memory size
 */
void memcpy_from_scp(void *trg, const void __iomem *src, int size)
{
	int i;
	u32 *t = trg;
	const u32 __iomem *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

/*
 * acquire a hardware semaphore
 * @param flag: semaphore id
 * return  1 :get sema success
 *        -1 :get sema timeout
 */
int get_scp_semaphore(int flag)
{
	int read_back;
	int count = 0;
	int ret = -1;
	unsigned long spin_flags;

	/* return 1 to prevent from access when driver not ready */
	if (!driver_init_done)
		return -1;

	/* spinlock context safe*/
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 0) {
		writel((1 << flag), SCP_SEMAPHORE);

		while (count != SEMAPHORE_TIMEOUT) {
			/* repeat test if we get semaphore */
			read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

			if (read_back == 1) {
				ret = 1;
				break;
			}
			writel((1 << flag), SCP_SEMAPHORE);
			count++;
		}

		if (ret < 0)
			pr_debug("[SCP] get scp sema. %d TIMEOUT...!\n", flag);
	} else {
		pr_err("[SCP] already hold scp sema. %d\n", flag);
	}

	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	return ret;
}
EXPORT_SYMBOL_GPL(get_scp_semaphore);

/*
 * release a hardware semaphore
 * @param flag: semaphore id
 * return  1 :release sema success
 *        -1 :release sema fail
 */
int release_scp_semaphore(int flag)
{
	int read_back;
	int ret = -1;
	unsigned long spin_flags;

	/* return 1 to prevent from access when driver not ready */
	if (!driver_init_done)
		return -1;

	/* spinlock context safe*/
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 1) {
		/* Write 1 clear */
		writel((1 << flag), SCP_SEMAPHORE);
		read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;
		if (read_back == 0)
			ret = 1;
		else
			pr_debug("[SCP] release scp sema. %d failed\n", flag);
	} else {
		pr_err("[SCP] try to release sema. %d not own by me\n", flag);
	}

	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	return ret;
}
EXPORT_SYMBOL_GPL(release_scp_semaphore);


/*
 * acquire a hardware semaphore 3way
 * @param flag: semaphore id
 * return  1 :get sema success
 *        -1 :get sema timeout
 */
int scp_get_semaphore_3way(int flag)
{
	int read_back;
	int count = 0;
	int ret = -1;
	unsigned long spin_flags;

	/* return 1 to prevent from access when driver not ready */
	if (!driver_init_done)
		return -1;

	if (flag >= SEMA_3WAY_TOTAL) {
		pr_err("[SCP] get sema. 3way flag=%d > total numbers ERROR\n", flag);
		return ret;
	}

	/* spinlock context safe*/
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	flag = (flag * 4) + 2;

	read_back = (readl(SCP_SEMAPHORE_3WAY) >> flag) & 0x1;

	if (read_back == 0) {
		writel((1 << flag), SCP_SEMAPHORE_3WAY);
		while (count != SEMAPHORE_3WAY_TIMEOUT) {
			/* repeat test if we get semaphore */
			read_back = (readl(SCP_SEMAPHORE_3WAY) >> flag) & 0x1;
			if (read_back == 1) {
				ret = 1;
				break;
			}
			writel((1 << flag), SCP_SEMAPHORE_3WAY);
			count++;
		}

		if (ret < 0)
			pr_debug("[SCP] get 3way sema. %d TIMEOUT\n", flag);
	} else {
		pr_err("[SCP] already hold 3way sema. %d\n", flag);
	}

	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	return ret;
}
EXPORT_SYMBOL_GPL(scp_get_semaphore_3way);

/*
 * release a hardware semaphore 3way
 * @param flag: semaphore id
 * return  1 :release sema success
 *        -1 :release sema fail
 */
int scp_release_semaphore_3way(int flag)
{
	int read_back;
	int ret = -1;
	unsigned long spin_flags;

	if (flag >= SEMA_3WAY_TOTAL) {
		pr_err("[SCP] release sema. 3way flag = %d > total numbers ERROR\n", flag);
		return ret;
	}

	/* spinlock context safe*/
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	flag = (flag * 4) + 2;

	read_back = (readl(SCP_SEMAPHORE_3WAY) >> flag) & 0x1;
	if (read_back == 1) {
		/* Write 1 clear */
		writel((1 << flag), SCP_SEMAPHORE_3WAY);
		read_back = (readl(SCP_SEMAPHORE_3WAY) >> flag) & 0x1;
		if (read_back == 0)
			ret = 1;
		else
			pr_debug("[SCP] release 3way semaphore %d failed\n", flag);
	} else {
		pr_err("[SCP] try to release 3way semaphore %d not own by me!\n", flag);
	}

	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	return ret;
}
EXPORT_SYMBOL_GPL(scp_release_semaphore_3way);

static BLOCKING_NOTIFIER_HEAD(scp_A_notifier_list);
static BLOCKING_NOTIFIER_HEAD(scp_B_notifier_list);
/*
 * register apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:   notifier block struct
 */
void scp_A_register_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_A_notify_mutex);
	blocking_notifier_chain_register(&scp_A_notifier_list, nb);

	pr_debug("[SCP] register scp A notify callback..\n");

	if (is_scp_ready(SCP_A_ID))
		nb->notifier_call(nb, SCP_EVENT_READY, NULL);
	mutex_unlock(&scp_A_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_A_register_notify);

/*
 * register apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:   notifier block struct
 */
void scp_B_register_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_B_notify_mutex);
	blocking_notifier_chain_register(&scp_B_notifier_list, nb);

	pr_debug("[SCP] register scp B notify callback..\n");

	if (is_scp_ready(SCP_B_ID))
		nb->notifier_call(nb, SCP_EVENT_READY, NULL);
	mutex_unlock(&scp_B_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_B_register_notify);

/*
 * unregister apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:     notifier block struct
 */
void scp_A_unregister_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_A_notify_mutex);
	blocking_notifier_chain_unregister(&scp_A_notifier_list, nb);
	mutex_unlock(&scp_A_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_A_unregister_notify);

/*
 * unregister apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:     notifier block struct
 */
void scp_B_unregister_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_B_notify_mutex);
	blocking_notifier_chain_unregister(&scp_B_notifier_list, nb);
	mutex_unlock(&scp_B_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_B_unregister_notify);

void scp_schedule_work(struct scp_work_struct *scp_ws)
{
	queue_work(scp_workqueue, &scp_ws->work);
}

/*
 * callback function for work struct
 * notify apps to start their tasks or generate an exception according to flag
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param ws:   work struct
 */
static void scp_A_notify_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws = container_of(ws, struct scp_work_struct, work);
	unsigned int scp_notify_flag = sws->flags;

	scp_ready[SCP_A_ID] = scp_notify_flag;

	if (scp_notify_flag) {
#ifdef CFG_RECOVERY_SUPPORT
		/* release pll lock after scp ulposc calibration */
		scp_pll_ctrl_set(0, 0);
		scp_recovery_flag[SCP_A_ID] = SCP_A_RECOVERY_OK;
#endif
		mutex_lock(&scp_A_notify_mutex);
		blocking_notifier_call_chain(&scp_A_notifier_list, SCP_EVENT_READY, NULL);
		mutex_unlock(&scp_A_notify_mutex);
	}

	if (!scp_ready[SCP_A_ID])
		scp_aed(EXCEP_RESET, SCP_A_ID);

}

/*
 * callback function for work struct
 * notify apps to start their tasks or generate an exception according to flag
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param ws:   work struct
 */
static void scp_B_notify_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws = container_of(ws, struct scp_work_struct, work);
	unsigned int scp_notify_flag = sws->flags;

	scp_ready[SCP_B_ID] = scp_notify_flag;

	if (scp_notify_flag) {
#ifdef CFG_RECOVERY_SUPPORT
		/* release pll lock after scp ulposc calibration */
		scp_pll_ctrl_set(0, 0);
		scp_recovery_flag[SCP_B_ID] = SCP_B_RECOVERY_OK;
#endif
		mutex_lock(&scp_B_notify_mutex);
		blocking_notifier_call_chain(&scp_B_notifier_list, SCP_EVENT_READY, NULL);
		mutex_unlock(&scp_B_notify_mutex);
	}


	if (!scp_ready[SCP_B_ID])
		scp_aed(EXCEP_RESET, SCP_B_ID);

}

/*
 * callback function for work struct
 * notify apps to start their tasks or generate an exception according to flag
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param ws:   work struct
 */
static void scp_timeout_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws = container_of(ws, struct scp_work_struct, work);
	unsigned int scp_timeout_id = sws->id;

	scp_aed(EXCEP_BOOTUP, scp_timeout_id);

}

/*
 * mark notify flag to 1 to notify apps to start their tasks
 */
static void scp_A_set_ready(void)
{
	pr_debug("%s()\n", __func__);
#if SCP_BOOT_TIME_OUT_MONITOR
	del_timer(&scp_ready_timer[SCP_A_ID]);
#endif
	scp_A_notify_work.flags = 1;
	scp_schedule_work(&scp_A_notify_work);
}

/*
 * mark notify flag to 1 to notify apps to start their tasks
 */
static void scp_B_set_ready(void)
{
	pr_debug("%s()\n", __func__);
#if SCP_BOOT_TIME_OUT_MONITOR
	del_timer(&scp_ready_timer[SCP_B_ID]);
#endif
	scp_B_notify_work.flags = 1;
	scp_schedule_work(&scp_B_notify_work);
}

/*
 * callback for reset timer
 * mark notify flag to 0 to generate an exception
 * @param data: unuse
 */
#if SCP_BOOT_TIME_OUT_MONITOR
static void scp_wait_ready_timeout(unsigned long data)
{
	pr_err("%s(),timer data=%lu\n", __func__, data);
	/*data=0: SCP A  ,  data=1: SCP B*/
	if (data == SCP_A_TIMER) {
		scp_timeout_work.flags = 0;
		scp_timeout_work.id = SCP_A_ID;
		scp_schedule_work(&scp_timeout_work);
	} else {
		scp_timeout_work.flags = 0;
		scp_timeout_work.id = SCP_B_ID;
		scp_schedule_work(&scp_timeout_work);
	}
}

#endif
/*
 * handle notification from scp
 * mark scp is ready for running tasks
 * @param id:   ipi id
 * @param data: ipi data
 * @param len:  length of ipi data
 */
static void scp_A_ready_ipi_handler(int id, void *data, unsigned int len)
{
	unsigned int scp_image_size = *(unsigned int *)data;

	if (!scp_ready[SCP_A_ID])
		scp_A_set_ready();

	/*verify scp image size*/
	if (scp_image_size != SCP_A_TCM_SIZE) {
		pr_err("[SCP]image size ERROR! AP=0x%x,SCP=0x%x\n", SCP_A_TCM_SIZE, scp_image_size);
		WARN_ON(1);
	}
}

/*
 * handle notification from scp
 * mark scp is ready for running tasks
 * @param id:   ipi id
 * @param data: ipi data
 * @param len:  length of ipi data
 */
static void scp_B_ready_ipi_handler(int id, void *data, unsigned int len)
{
	if (!scp_ready[SCP_B_ID])
		scp_B_set_ready();
}

/*
 * @return: 1 if scp is ready for running tasks
 */
unsigned int is_scp_ready(scp_core_id id)
{
	if (scp_ready[id])
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(is_scp_ready);

/*
 * reset scp and create a timer waiting for scp notify
 * notify apps to stop their tasks if needed
 * generate error if reset fail
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param reset:    bit[0-3]=0 for scp enable, =1 for reboot
 *                  bit[4-7]=0 for All, =1 for scp_A, =2 for scp_B
 * @return:         0 if success
 */
int reset_scp(int reset)
{
	unsigned int *reg;
#if SCP_BOOT_TIME_OUT_MONITOR
	int i;

	for (i = 0; i < SCP_CORE_TOTAL ; i++)
		del_timer(&scp_ready_timer[i]);
#endif
	/*scp_logger_stop();*/
	if (((reset & 0xf0) == 0x10) || ((reset & 0xf0) == 0x00)) { /* reset A or All*/
		/*reset scp A*/
		mutex_lock(&scp_A_notify_mutex);
		blocking_notifier_call_chain(&scp_A_notifier_list, SCP_EVENT_STOP, NULL);
		mutex_unlock(&scp_A_notify_mutex);

		reg = (unsigned int *)SCP_BASE;
		if (reset & 0x0f) { /* do reset */
			/* make sure scp is in idle state */
			int timeout = 50; /* max wait 1s */

			while (--timeout) {
#ifdef CFG_RECOVERY_SUPPORT
				if (*(unsigned int *)SCP_GPR_CM4_A_REBOOT == 0x34) {
					if (SCP_SLEEP_STATUS_REG & SCP_A_IS_SLEEP) {
						/* reset */
						*(unsigned int *)reg = 0x0;
						scp_ready[SCP_A_ID] = 0;
						*(unsigned int *)SCP_GPR_CM4_A_REBOOT = 1;
						/* lock pll for ulposc calibration */
						 /* do it only in reset */
						scp_pll_ctrl_set(1, 0);
						dsb(SY);
						break;
					}
				}
#else
				if (SCP_SLEEP_STATUS_REG & SCP_A_IS_SLEEP) {
					/* reset */
					*(unsigned int *)reg = 0x0;
					scp_ready[SCP_A_ID] = 0;
					dsb(SY);
					break;
				}
#endif
				msleep(20);
				if (timeout == 0)
					pr_debug("[SCP] wait scp A reset timeout, skip\n");
			}
			pr_debug("[SCP] wait scp A reset timeout %d\n", timeout);
		}
		if (scp_enable[SCP_A_ID]) {
			pr_debug("[SCP] reset scp A\n");
			*(unsigned int *)reg = 0x1;
			dsb(SY);
#if SCP_BOOT_TIME_OUT_MONITOR
			init_timer(&scp_ready_timer[SCP_A_ID]);
			scp_ready_timer[SCP_A_ID].expires = jiffies + SCP_READY_TIMEOUT;
			scp_ready_timer[SCP_A_ID].function = &scp_wait_ready_timeout;
			scp_ready_timer[SCP_A_ID].data = (unsigned long) SCP_A_TIMER;/*0: SCP A    1: SCP B*/
			add_timer(&scp_ready_timer[SCP_A_ID]);
#endif
		}
	}

	if (((reset & 0xf0) == 0x20) || ((reset & 0xf0) == 0x00)) { /* reset B or All*/
		/*reset scp B*/
		mutex_lock(&scp_B_notify_mutex);
		blocking_notifier_call_chain(&scp_B_notifier_list, SCP_EVENT_STOP, NULL);
		mutex_unlock(&scp_B_notify_mutex);

		reg = (unsigned int *)SCP_BASE_DUAL;
		if (reset & 0x0f) { /* do reset */
			/* make sure scp is in idle state */
			int timeout = 50; /* max wait 1s */

			while (--timeout) {
#ifdef CFG_RECOVERY_SUPPORT
				if (*(unsigned int *)SCP_GPR_CM4_B_REBOOT == 0x35) {
					if (SCP_SLEEP_STATUS_REG & SCP_B_IS_SLEEP) {
						/* reset */
						*(unsigned int *)reg = 0x0;
						scp_ready[SCP_B_ID] = 0;
						*(unsigned int *)SCP_GPR_CM4_B_REBOOT = 2;
						/* lock pll for ulposc calibration */
						 /* do it only in reset */
						scp_pll_ctrl_set(1, 0);
						dsb(SY);
						break;
					}
				}
#else
				if (SCP_SLEEP_STATUS_REG & SCP_B_IS_SLEEP) {
					/* reset */
					*(unsigned int *)reg = 0x0;
					scp_ready[SCP_B_ID] = 0;
					dsb(SY);
					break;
				}
#endif
				msleep(20);
				if (timeout == 0)
					pr_debug("[SCP] wait scp B reset timeout, skip\n");
			}
		}
		if (scp_enable[SCP_B_ID]) {
			pr_debug("[SCP] reset scp B\n");
			*(unsigned int *)reg = 0x1;
			dsb(SY);
#if SCP_BOOT_TIME_OUT_MONITOR
			init_timer(&scp_ready_timer[SCP_B_ID]);
			scp_ready_timer[SCP_B_ID].expires = jiffies + SCP_READY_TIMEOUT;
			scp_ready_timer[SCP_B_ID].function = &scp_wait_ready_timeout;
			scp_ready_timer[SCP_B_ID].data = (unsigned long) SCP_B_TIMER;/*0: SCP A    1: SCP B*/
			add_timer(&scp_ready_timer[SCP_B_ID]);
#endif
		}
	}

	pr_debug("[SCP] reset scp done\n");

	return 0;
}


/*
 * TODO: what should we do when hibernation ?
 */
static int scp_pm_event(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
	int retval;

		switch (pm_event) {
		case PM_POST_HIBERNATION:
			pr_debug("[SCP] scp_pm_event SCP reboot\n");
			retval = reset_scp(1);
			if (retval < 0) {
				retval = -EINVAL;
				pr_debug("[SCP] scp_pm_event SCP reboot Fail\n");
			}
			return NOTIFY_DONE;
		}
	return NOTIFY_OK;
}

static struct notifier_block scp_pm_notifier_block = {
	.notifier_call = scp_pm_event,
	.priority = 0,
};


static inline ssize_t scp_A_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	if (scp_ready[SCP_A_ID])
		return scnprintf(buf, PAGE_SIZE, "SCP A is ready\n");
	else
		return scnprintf(buf, PAGE_SIZE, "SCP A is not ready\n");
}

DEVICE_ATTR(scp_A_status, 0444, scp_A_status_show, NULL);

static inline ssize_t scp_B_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	if (scp_ready[SCP_B_ID])
		return scnprintf(buf, PAGE_SIZE, "SCP B is ready\n");
	else
		return scnprintf(buf, PAGE_SIZE, "SCP B is not ready\n");
}

DEVICE_ATTR(scp_B_status, 0444, scp_B_status_show, NULL);

static inline ssize_t scp_A_reg_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	int len = 0;

	scp_A_dump_regs();
	if (scp_ready[SCP_A_ID]) {
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_DEBUG_PC_REG:0x%x\n", SCP_A_DEBUG_PC_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_DEBUG_LR_REG:0x%x\n", SCP_A_DEBUG_LR_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_DEBUG_PSP_REG:0x%x\n", SCP_A_DEBUG_PSP_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_DEBUG_SP_REG:0x%x\n", SCP_A_DEBUG_SP_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_GENERAL_REG0:0x%x\n", SCP_A_GENERAL_REG0);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_GENERAL_REG1:0x%x\n", SCP_A_GENERAL_REG1);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_GENERAL_REG2:0x%x\n", SCP_A_GENERAL_REG2);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_GENERAL_REG3:0x%x\n", SCP_A_GENERAL_REG3);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_GENERAL_REG4:0x%x\n", SCP_A_GENERAL_REG4);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_A_GENERAL_REG5:0x%x\n", SCP_A_GENERAL_REG5);
		return len;
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP A is not ready\n");
}

DEVICE_ATTR(scp_A_reg_status, 0444, scp_A_reg_status_show, NULL);

static inline ssize_t scp_B_reg_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	int len = 0;

	scp_B_dump_regs();
	if (scp_ready[SCP_B_ID]) {
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_DEBUG_PC_REG:0x%x\n", SCP_B_DEBUG_PC_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_DEBUG_LR_REG:0x%x\n", SCP_B_DEBUG_LR_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_DEBUG_PSP_REG:0x%x\n", SCP_B_DEBUG_PSP_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_DEBUG_SP_REG:0x%x\n", SCP_B_DEBUG_SP_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_GENERAL_REG0:0x%x\n", SCP_B_GENERAL_REG0);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_GENERAL_REG1:0x%x\n", SCP_B_GENERAL_REG1);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_GENERAL_REG2:0x%x\n", SCP_B_GENERAL_REG2);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_GENERAL_REG3:0x%x\n", SCP_B_GENERAL_REG3);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_GENERAL_REG4:0x%x\n", SCP_B_GENERAL_REG4);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_B_GENERAL_REG5:0x%x\n", SCP_B_GENERAL_REG5);
		return len;
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP B is not ready\n");
}

DEVICE_ATTR(scp_B_reg_status, 0444, scp_B_reg_status_show, NULL);

#if SCP_VCORE_TEST_ENABLE
static inline ssize_t scp_vcore_request_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned long spin_flags;

	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	scp_current_freq = readl(CURRENT_FREQ_REG);
	scp_expected_freq = readl(EXPECTED_FREQ_REG);
	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	pr_err("[SCP] receive freq show\n");
	return scnprintf(buf, PAGE_SIZE, "SCP freq. expect=%d, cur=%d\n", scp_expected_freq, scp_current_freq);
}

static unsigned int pre_feature_req = 0xff;
static ssize_t scp_vcore_request_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int feature_req = 0;
	unsigned int tick_cnt = 0, tick_cnt_2 = 0, tick_cnt_3 = 0;

	pr_err("[SCP] receive freq req\n");
#if 0
	len = (n < (sizeof(desc) - 1)) ? n : (sizeof(desc) - 1);
	if (copy_from_user(desc, buf, len))
		return 0;

	desc[len] = '\0';
#endif
	if (kstrtouint(buf, 0, &feature_req) == 0) {
		if (pre_feature_req == 4)
			scp_deregister_feature(VCORE_TEST5_FEATURE_ID);
		if (pre_feature_req == 3)
			scp_deregister_feature(VCORE_TEST4_FEATURE_ID);
		if (pre_feature_req == 2)
			scp_deregister_feature(VCORE_TEST3_FEATURE_ID);
		if (pre_feature_req == 1)
			scp_deregister_feature(VCORE_TEST2_FEATURE_ID);
		if (pre_feature_req == 0)
			scp_deregister_feature(VCORE_TEST_FEATURE_ID);

		if (feature_req == 4) {
			scp_register_feature(VCORE_TEST5_FEATURE_ID);
			pre_feature_req = 4;
		}
		if (feature_req == 3) {
			scp_register_feature(VCORE_TEST4_FEATURE_ID);
			pre_feature_req = 3;
		}
		if (feature_req == 2) {
			scp_register_feature(VCORE_TEST3_FEATURE_ID);
			pre_feature_req = 2;
		}
		if (feature_req == 1) {
			scp_register_feature(VCORE_TEST2_FEATURE_ID);
			pre_feature_req = 1;
		}
		if (feature_req == 0) {
			scp_register_feature(VCORE_TEST_FEATURE_ID);
			pre_feature_req = 0;
		}
		pr_warn("[SCP] set expect=%d, cur=%d\n", scp_expected_freq, scp_current_freq);
		tick_cnt = mt_get_ckgen_freq(28);
		tick_cnt_2 = mt_get_abist_freq(3);
		tick_cnt_3 = mt_get_abist_freq(63);
		pr_warn("[SCP] measure tick_cnt=(pll %d scp_1 %d scp_2 %d)\n", tick_cnt, tick_cnt_2, tick_cnt_3);
	}
	return n;
}

DEVICE_ATTR(scp_vcore_request, 0644, scp_vcore_request_show, scp_vcore_request_store);
#endif

static inline ssize_t scp_A_db_test_show(struct device *kobj, struct device_attribute *attr, char *buf)
{

	if (scp_ready[SCP_A_ID]) {
		scp_aed_reset(EXCEP_RUNTIME, SCP_A_ID);
		return scnprintf(buf, PAGE_SIZE, "dumping SCP A db\n");
	} else
		scp_aed_reset(EXCEP_RUNTIME, SCP_A_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP A is not ready, try to dump EE\n");
}

DEVICE_ATTR(scp_A_db_test, 0444, scp_A_db_test_show, NULL);

static inline ssize_t scp_B_db_test_show(struct device *kobj, struct device_attribute *attr, char *buf)
{

	if (scp_ready[SCP_B_ID]) {
		scp_aed_reset(EXCEP_RUNTIME, SCP_B_ID);
		return scnprintf(buf, PAGE_SIZE, "dumping SCP B db\n");
	} else {
		scp_aed_reset(EXCEP_RUNTIME, SCP_B_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP B is not ready, try to dump EE\n");
	}
}

DEVICE_ATTR(scp_B_db_test, 0444, scp_B_db_test_show, NULL);

#ifdef CONFIG_MTK_ENG_BUILD
static inline ssize_t scp_A_awake_lock_show(struct device *kobj, struct device_attribute *attr, char *buf)
{

	if (scp_ready[SCP_A_ID]) {
		scp_awake_lock(SCP_A_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP A awake lock\n");
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP A is not ready\n");
}

DEVICE_ATTR(scp_A_awake_lock, 0444, scp_A_awake_lock_show, NULL);

static inline ssize_t scp_A_awake_unlock_show(struct device *kobj, struct device_attribute *attr, char *buf)
{

	if (scp_ready[SCP_A_ID]) {
		scp_awake_unlock(SCP_A_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP A awake unlock\n");
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP A is not ready\n");
}

DEVICE_ATTR(scp_A_awake_unlock, 0444, scp_A_awake_unlock_show, NULL);



static inline ssize_t scp_B_awake_lock_show(struct device *kobj, struct device_attribute *attr, char *buf)
{

	if (scp_ready[SCP_B_ID]) {
		scp_awake_lock(SCP_B_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP B awake lock\n");
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP B is not ready\n");
}

DEVICE_ATTR(scp_B_awake_lock, 0444, scp_B_awake_lock_show, NULL);

static inline ssize_t scp_B_awake_unlock_show(struct device *kobj, struct device_attribute *attr, char *buf)
{

	if (scp_ready[SCP_B_ID]) {
		scp_awake_unlock(SCP_B_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP B awake unlock\n");
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP B is not ready\n");
}

DEVICE_ATTR(scp_B_awake_unlock, 0444, scp_B_awake_unlock_show, NULL);

static inline ssize_t scp_ipi_test_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int value = 0x5A5A;
	ipi_status ret;

	if (scp_ready[SCP_A_ID]) {
		ret = scp_ipi_send(IPI_TEST1, &value, sizeof(value), 0, SCP_A_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP A ipi send ret=%d\n", ret);
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP A is not ready\n");
}

DEVICE_ATTR(scp_ipi_test, 0444, scp_ipi_test_show, NULL);

static inline ssize_t scp_B_ipi_test_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int value = 0x5A5A;
	ipi_status ret;

	if (scp_ready[SCP_B_ID]) {
		ret = scp_ipi_send(IPI_TEST1, &value, sizeof(value), 0, SCP_B_ID);
		return scnprintf(buf, PAGE_SIZE, "SCP B ipi send ret=%d\n", ret);
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP B is not ready\n");
}

DEVICE_ATTR(scp_B_ipi_test, 0444, scp_B_ipi_test_show, NULL);
#endif

#ifdef CFG_RECOVERY_SUPPORT
void scp_wdt_reset(scp_core_id cpu_id)
{
	switch (cpu_id) {
	case SCP_A_ID:
		writel(0x8000000f, SCP_A_WDT_REG);
		break;
	case SCP_B_ID:
		writel(0x8000000f, SCP_B_WDT_REG);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(scp_wdt_reset)

/*
 * trigger wdt manually
 * debug use
 */
static ssize_t scp_wdt_trigger(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_debug("scp_wdt_trigger: %s\n", buf);
	scp_wdt_reset(SCP_A_ID);
	return count;
}

DEVICE_ATTR(wdt_reset, S_IWUSR, NULL, scp_wdt_trigger);

/*
 * trigger wdt manually
 * debug use
 */
static ssize_t scp_B_wdt_trigger(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_debug("scp_B_wdt_trigger: %s\n", buf);
	scp_wdt_reset(SCP_B_ID);
	return count;
}
struct device_attribute dev_attr_scp_B_wdt_reset = __ATTR(wdt_reset, S_IWUSR, NULL, scp_B_wdt_trigger);

static ssize_t scp_recovery_flag_r(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", scp_recovery_flag[SCP_A_ID]);
}
static ssize_t scp_recovery_flag_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, tmp;

	ret = kstrtoint(buf, 10, &tmp);
	if (kstrtoint(buf, 10, &tmp) < 0) {
		pr_debug("scp_recovery_flag error\n");
		return count;
	}
	scp_recovery_flag[SCP_A_ID] = tmp;
	return count;
}

DEVICE_ATTR(recovery_flag, S_IRUSR|S_IWUSR, scp_recovery_flag_r, scp_recovery_flag_w);

static ssize_t scp_B_recovery_flag_r(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", scp_recovery_flag[SCP_B_ID]);
}
static ssize_t scp_B_recovery_flag_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, tmp;

	ret = kstrtoint(buf, 10, &tmp);
	if (kstrtoint(buf, 10, &tmp) < 0) {
		pr_debug("scp_recovery_flag error\n");
		return count;
	}
	scp_recovery_flag[SCP_B_ID] = tmp;
	return count;
}
struct device_attribute dev_attr_scp_B_recovery_flag =
__ATTR(recovery_flag, S_IRUSR|S_IWUSR, scp_B_recovery_flag_r, scp_B_recovery_flag_w);
#endif

static struct miscdevice scp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "scp",
	.fops = &scp_A_log_file_ops
};

static struct miscdevice scp_B_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "scp_B",
	.fops = &scp_B_log_file_ops
};

/*
 * register /dev and /sys files
 * @return:     0: success, otherwise: fail
 */
static int create_files(void)
{
	int ret;

	ret = misc_register(&scp_device);

	if (unlikely(ret != 0)) {
		pr_err("[SCP] misc register failed\n");
		return ret;
	}
	ret = misc_register(&scp_B_device);

	if (unlikely(ret != 0)) {
		pr_err("[SCP B] misc register failed\n");
		return ret;
	}
#if SCP_LOGGER_ENABLE
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_mobile_log);
	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_logger_wakeup_AP);
	if (unlikely(ret != 0))
		return ret;
#ifdef CONFIG_MTK_ENG_BUILD
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_mobile_log_UT);
	if (unlikely(ret != 0))
		return ret;
#endif
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_get_last_log);
	if (unlikely(ret != 0))
		return ret;
#endif
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_status);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_bin_file(scp_device.this_device, &bin_attr_scp_dump);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_reg_status);

	if (unlikely(ret != 0))
		return ret;

	/*only support debug db test in engineer build*/
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_db_test);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_db_test);

	if (unlikely(ret != 0))
		return ret;
#ifdef CONFIG_MTK_ENG_BUILD
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_awake_lock);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_A_awake_unlock);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_awake_lock);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_awake_unlock);
	if (unlikely(ret != 0))
		return ret;
	/* SCP IPI Debug sysfs*/
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_ipi_test);
	if (unlikely(ret != 0))
		return ret;
	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_ipi_test);
	if (unlikely(ret != 0))
		return ret;
#endif

#ifdef CFG_RECOVERY_SUPPORT
	ret = device_create_file(scp_device.this_device, &dev_attr_wdt_reset);
	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_wdt_reset);
	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_recovery_flag);
	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_recovery_flag);
	if (unlikely(ret != 0))
		return ret;
#endif

#if SCP_LOGGER_ENABLE
	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_mobile_log);
	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_logger_wakeup_AP);
	if (unlikely(ret != 0))
		return ret;
#ifdef CONFIG_MTK_ENG_BUILD
	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_mobile_log_UT);
	if (unlikely(ret != 0))
		return ret;
#endif
	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_get_last_log);
	if (unlikely(ret != 0))
		return ret;
#endif
	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_status);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_bin_file(scp_B_device.this_device, &bin_attr_scp_B_dump);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_B_device.this_device, &dev_attr_scp_B_reg_status);

	if (unlikely(ret != 0))
		return ret;

#if SCP_VCORE_TEST_ENABLE
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_vcore_request);

	if (unlikely(ret != 0))
		return ret;
#endif
	return 0;
}

#ifdef CONFIG_OF_RESERVED_MEM
#define SCP_MEM_RESERVED_KEY "mediatek,reserve-memory-scp_share"
int scp_reserve_mem_of_init(struct reserved_mem *rmem)
{
	scp_reserve_mem_id_t id;
	phys_addr_t accumlate_memory_size = 0;

	scp_mem_base_phys = (phys_addr_t) rmem->base;
	scp_mem_size = (phys_addr_t) rmem->size;
	if ((scp_mem_base_phys >= (0x90000000ULL)) || (scp_mem_base_phys <= 0x0)) {
		/*The scp remap region is fixed, only
		 * 0x4000_0000ULL~0x8FFF_FFFFULL
		 * can be accessible
		 */
		pr_err("[SCP] The allocated memory(0x%llx) is larger than expected\n", scp_mem_base_phys);
		/*should not call WARN_ON() here or there is no log, return -1
		 * instead.
		 */
		return -1;
	}


	pr_debug("[SCP] phys:0x%llx - 0x%llx (0x%llx)\n", (phys_addr_t)rmem->base,
			(phys_addr_t)rmem->base + (phys_addr_t)rmem->size, (phys_addr_t)rmem->size);
	accumlate_memory_size = 0;
	for (id = 0; id < NUMS_MEM_ID; id++) {
		scp_reserve_mblock[id].start_phys = scp_mem_base_phys + accumlate_memory_size;
		accumlate_memory_size += scp_reserve_mblock[id].size;
		pr_debug("[SCP][reserve_mem:%d]: phys:0x%llx - 0x%llx (0x%llx)\n", id,
			scp_reserve_mblock[id].start_phys,
			scp_reserve_mblock[id].start_phys+scp_reserve_mblock[id].size,
			scp_reserve_mblock[id].size);
	}
	return 0;
}

RESERVEDMEM_OF_DECLARE(scp_reserve_mem_init, SCP_MEM_RESERVED_KEY, scp_reserve_mem_of_init);
#endif
phys_addr_t scp_get_reserve_mem_phys(scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_err("[SCP] no reserve memory for %d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].start_phys;
}
EXPORT_SYMBOL_GPL(scp_get_reserve_mem_phys);

phys_addr_t scp_get_reserve_mem_virt(scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_err("[SCP] no reserve memory for %d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].start_virt;
}
EXPORT_SYMBOL_GPL(scp_get_reserve_mem_virt);

phys_addr_t scp_get_reserve_mem_size(scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_err("[SCP] no reserve memory for %d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].size;
}
EXPORT_SYMBOL_GPL(scp_get_reserve_mem_size);


static void scp_reserve_memory_ioremap(void)
{
	scp_reserve_mem_id_t id;
	phys_addr_t accumlate_memory_size;


	if ((scp_mem_base_phys >= (0x90000000ULL)) || (scp_mem_base_phys <= 0x0)) {
		/*The scp remap region is fixed, only
		 * 0x4000_0000ULL~0x8FFF_FFFFULL
		 * can be accessible
		 */
		pr_err("[SCP] The allocated memory(0x%llx) is larger than expected\n", scp_mem_base_phys);
		/*call WARN_ON() here to assert the unexpected memory allocation
		 */
		WARN_ON(1);
	}
	accumlate_memory_size = 0;
	scp_mem_base_virt = (phys_addr_t)ioremap_wc(scp_mem_base_phys, scp_mem_size);
	pr_debug("[SCP]reserve mem: virt:0x%llx - 0x%llx (0x%llx)\n", (phys_addr_t)scp_mem_base_virt,
		(phys_addr_t)scp_mem_base_virt + (phys_addr_t)scp_mem_size, scp_mem_size);
	for (id = 0; id < NUMS_MEM_ID; id++) {
		scp_reserve_mblock[id].start_virt = scp_mem_base_virt + accumlate_memory_size;
		accumlate_memory_size += scp_reserve_mblock[id].size;
	}
	/* the reserved memory should be larger then expected memory
	 * or scp_reserve_mblock does not match dts
	 */
	WARN_ON(accumlate_memory_size > scp_mem_size);
#ifdef DEBUG
	for (id = 0; id < NUMS_MEM_ID; id++) {
		pr_debug("[SCP][mem_reserve-%d] phys:0x%llx,virt:0x%llx,size:0x%llx\n",
			id, scp_get_reserve_mem_phys(id), scp_get_reserve_mem_virt(id), scp_get_reserve_mem_size(id));
	}
#endif
}

#if ENABLE_SCP_EMI_PROTECTION
void set_scp_mpu(void)
{
	unsigned int shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_id,
		     shr_mem_mpu_attr;
	shr_mem_mpu_id = MPU_REGION_ID_SCP_SMEM;
	shr_mem_phy_start = scp_mem_base_phys;
	shr_mem_phy_end = scp_mem_base_phys + scp_mem_size - 0x1;
	shr_mem_mpu_attr =
		SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
				NO_PROTECTION, FORBIDDEN, FORBIDDEN,
				NO_PROTECTION);
	pr_debug("[SCP] MPU Start protect SCP Share region<%d:%08x:%08x> %x\n",
			shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end,
			shr_mem_mpu_attr);
	emi_mpu_set_region_protection(shr_mem_phy_start,        /*START_ADDR */
			shr_mem_phy_end,  /*END_ADDR */
			shr_mem_mpu_id,   /*region */
			shr_mem_mpu_attr);

}
#endif

void scp_register_feature(feature_id_t id)
{
	uint32_t i;
	int ret = 0;

	/*prevent from access when scp is down*/
#if defined(CONFIG_MACH_MT6759)
	if (!scp_ready[SCP_A_ID]) {
		pr_err("scp_register_feature:not ready, A=%u\n", scp_ready[SCP_A_ID]);
#else
	if (!scp_ready[SCP_A_ID] || !scp_ready[SCP_B_ID]) {
		pr_err("scp_register_feature:not ready, A=%u,B=%u\n", scp_ready[SCP_A_ID], scp_ready[SCP_B_ID]);
#endif
		return;
	}

	/* because feature_table is a global variable, use mutex lock to protect it from
	 * accessing in the same time
	 */
	mutex_lock(&scp_feature_mutex);
	for (i = 0; i < NUM_FEATURE_ID; i++) {
		if (feature_table[i].feature == id)
			feature_table[i].enable = 1;
	}
	scp_expected_freq = scp_get_freq();

	/*SCP keep awake */
	scp_awake_lock(SCP_A_ID);

	scp_current_freq = readl(CURRENT_FREQ_REG);
	writel(scp_expected_freq, EXPECTED_FREQ_REG);

	/* send request only when scp is not down */
	if (scp_ready[SCP_A_ID]) {
		if (scp_current_freq != scp_expected_freq) {
			/* set scp freq. */
			ret = scp_request_freq();
			if (ret == -1) {
				pr_err("[SCP]scp_register_feature request_freq fail\n");
				WARN_ON(1);
			}
		}
	} else {
		pr_err("[SCP]Not send SCP DVFS request because SCP is down\n");
		WARN_ON(1);
	}

	/*SCP release awake */
	scp_awake_unlock(SCP_A_ID);

	mutex_unlock(&scp_feature_mutex);
}

void scp_deregister_feature(feature_id_t id)
{
	uint32_t i;
	int ret = 0;

	/* prevent from access when scp is down */
#if defined(CONFIG_MACH_MT6759)
	if (!scp_ready[SCP_A_ID])
#else
	if (!scp_ready[SCP_A_ID] || !scp_ready[SCP_B_ID])
#endif
		return;

	mutex_lock(&scp_feature_mutex);
	for (i = 0; i < NUM_FEATURE_ID; i++) {
		if (feature_table[i].feature == id)
			feature_table[i].enable = 0;
	}
	scp_expected_freq = scp_get_freq();

	/*SCP keep awake */
	scp_awake_lock(SCP_A_ID);

	scp_current_freq = readl(CURRENT_FREQ_REG);
	writel(scp_expected_freq, EXPECTED_FREQ_REG);

	/* send request only when scp is not down */
	if (scp_ready[SCP_A_ID]) {
		if (scp_current_freq != scp_expected_freq) {
			/* set scp freq. */
			ret = scp_request_freq();
			if (ret == -1) {
				pr_err("[SCP]scp_register_feature request_freq fail\n");
				WARN_ON(1);
			}
		}
	} else {
		pr_err("[SCP]Not send SCP DVFS request because SCP is down\n");
		WARN_ON(1);
	}

	/*SCP release awake */
	scp_awake_unlock(SCP_A_ID);

	mutex_unlock(&scp_feature_mutex);
}
int scp_check_resource(void)
{
	/* called by lowpower related function
	 * main purpose is to ensure main_pll is not disabled
	 * because scp needs main_pll to run at vcore 1.0 and 354Mhz
	 * return value:
	 * 1: main_pll shall be enabled, 26M shall be enabled, infra shall be enabled
	 * 0: main_pll may disable, 26M may disable, infra may disable
	 */
	int scp_resource_status = 0;
#ifdef CONFIG_MACH_MT6799
	unsigned long spin_flags;

	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	scp_current_freq = readl(CURRENT_FREQ_REG);
	scp_expected_freq = readl(EXPECTED_FREQ_REG);
	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);

	if (scp_expected_freq == FREQ_416MHZ || scp_current_freq == FREQ_416MHZ)
		scp_resource_status = 1;
	else
		scp_resource_status = 0;
#endif

	return scp_resource_status;
}

static int scp_device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	const char *core_status = NULL;
	struct device *dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	scpreg.sram = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *) scpreg.sram)) {
		pr_err("[SCP] scpreg.sram error\n");
		return -1;
	}
	scpreg.total_tcmsize = (unsigned int)resource_size(res);
	pr_debug("[SCP] sram base=0x%p %x\n", scpreg.sram, scpreg.total_tcmsize);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	scpreg.cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *) scpreg.cfg)) {
		pr_err("[SCP] scpreg.cfg error\n");
		return -1;
	}
	pr_debug("[SCP] cfg base=0x%p\n", scpreg.cfg);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	scpreg.clkctrl = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *) scpreg.clkctrl)) {
		pr_err("[SCP] scpreg.clkctrl error\n");
		return -1;
	}
	pr_debug("[SCP] clkctrl base=0x%p\n", scpreg.clkctrl);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	scpreg.clkctrldual = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *) scpreg.clkctrldual)) {
		pr_err("[SCP] scpreg.clkctrldual error\n");
		return -1;
	}
	pr_debug("[SCP] clkctrldual base=0x%p\n", scpreg.clkctrldual);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	scpreg.irq = res->start;
	pr_debug("[SCP] scpreg.irq=%d\n", scpreg.irq);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	scpreg.irq_dual = res->start;
	pr_debug("[SCP] scpreg.irq_dual=%d\n", scpreg.irq_dual);

	of_property_read_u32(pdev->dev.of_node, "scp_sramSize", &scpreg.scp_tcmsize);
	if (!scpreg.scp_tcmsize) {
		pr_err("[SCP] total_tcmsize not found\n");
		return -ENODEV;
	}
	pr_debug("[SCP] scpreg.scp_tcmsize =%d\n", scpreg.scp_tcmsize);

	/*scp core 1*/
	of_property_read_string(pdev->dev.of_node, "core_1", &core_status);
	if (strcmp(core_status, "enable") != 0)
		pr_err("[SCP] core_1 not enable\n");
	else {
		pr_debug("[SCP] core_1 enable\n");
		scp_enable[SCP_A_ID] = 1;
	}

	/*scp core 2*/
	of_property_read_string(pdev->dev.of_node, "core_2", &core_status);
	if (strcmp(core_status, "enable") != 0)
		pr_err("[SCP] core_2 not enable\n");
	else {
		pr_debug("[SCP] core_2 enable\n");
		scp_enable[SCP_B_ID] = 1;
	}
	return ret;
}

static int scp_device_remove(struct platform_device *dev)
{
	return 0;
}

static int scpsys_device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct device *dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	scpreg.scpsys = devm_ioremap_resource(dev, res);
	pr_debug("[SCP] scpreg.scpsys %p\n", scpreg.scpsys);
	if (IS_ERR((void const *) scpreg.scpsys)) {
		pr_err("[SCP] scpreg.sram error\n");
		return -1;
	}
	return ret;
}

static int scpsys_device_remove(struct platform_device *dev)
{
	return 0;
}

static const struct of_device_id scp_of_ids[] = {
	{ .compatible = "mediatek,scp", },
	{}
};

static struct platform_driver mtk_scp_device = {
	.probe = scp_device_probe,
	.remove = scp_device_remove,
	.driver = {
		.name = "scp",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = scp_of_ids,
#endif
	},
};

static const struct of_device_id scpsys_of_ids[] = {
	{ .compatible = "mediatek,scpsys", },
	{}
};

static struct platform_driver mtk_scpsys_device = {
	.probe = scpsys_device_probe,
	.remove = scpsys_device_remove,
	.driver = {
		.name = "scpsys",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = scpsys_of_ids,
#endif
	},
};

/*
 * driver initialization entry point
 */
static int __init scp_init(void)
{
	int ret = 0;
	int i = 0;

	/* scp ready static flag initialise */
	for (i = 0; i < SCP_CORE_TOTAL ; i++) {
		scp_enable[i] = 0;
		scp_ready[i] = 0;
	}

	/* scp platform initialise */
	pr_debug("[SCP] platform init\n");
	scp_awake_init();
	scp_workqueue = create_workqueue("SCP_WQ");
	ret = scp_excep_init();
	if (ret) {
		pr_err("[SCP]Excep Init Fail\n");
		return -1;
	}
	if (platform_driver_register(&mtk_scp_device))
		pr_err("[SCP] scp probe fail\n");
	if (platform_driver_register(&mtk_scpsys_device))
		pr_err("[SCP] scpsys probe fail\n");
	/* scp ipi initialise */
	pr_debug("[SCP] ipi irq init\n");
	scp_send_buff[SCP_A_ID] = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_send_buff[SCP_A_ID])
		return -1;

	scp_recv_buff[SCP_A_ID] = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_recv_buff[SCP_A_ID])
		return -1;

	scp_send_buff[SCP_B_ID] = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_send_buff[SCP_B_ID])
		return -1;

	scp_recv_buff[SCP_B_ID] = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_recv_buff[SCP_B_ID])
		return -1;

	INIT_WORK(&scp_A_notify_work.work, scp_A_notify_ws);
	INIT_WORK(&scp_B_notify_work.work, scp_B_notify_ws);
	INIT_WORK(&scp_timeout_work.work, scp_timeout_ws);


	scp_A_irq_init();
	scp_B_irq_init();
	scp_A_ipi_init();
	scp_B_ipi_init();
	scp_ipi_registration(IPI_SCP_A_READY, scp_A_ready_ipi_handler, "scp_A_ready");
	scp_ipi_registration(IPI_SCP_B_READY, scp_B_ready_ipi_handler, "scp_B_ready");

	/* scp ramdump initialise */
	pr_debug("[SCP] ramdump init\n");
	scp_ram_dump_init();
	ret = register_pm_notifier(&scp_pm_notifier_block);

	if (ret)
		pr_err("[SCP] failed to register PM notifier %d\n", ret);

	/* scp sysfs initialise */
	pr_debug("[SCP] sysfs init\n");
	ret = create_files();

	if (unlikely(ret != 0)) {
		pr_err("[SCP] create files failed\n");
		return -1;
	}

	/* scp request irq */
	pr_debug("[SCP] request_irq\n");
	ret = request_irq(scpreg.irq, scp_A_irq_handler, IRQF_TRIGGER_NONE, "SCP A IPC2HOST", NULL);
	if (ret) {
		pr_err("[SCP] CM4 A require irq failed\n");
		return -1;
	}
	pr_debug("[SCP] request_irq dual\n");
	ret = request_irq(scpreg.irq_dual, scp_B_irq_handler, IRQF_TRIGGER_NONE, "SCP B IPC2HOST", NULL);
	if (ret) {
		pr_err("[SCP] CM4 B require irq failed\n");
		return -1;
	}

	/*scp resvered memory*/
	pr_debug("[SCP] scp_reserve_memory_ioremap\n");
	scp_reserve_memory_ioremap();
#if SCP_LOGGER_ENABLE
	/* scp logger initialise */
	pr_debug("[SCP] logger init\n");
	if (scp_logger_init(scp_get_reserve_mem_virt(SCP_A_LOGGER_MEM_ID),
				scp_get_reserve_mem_size(SCP_A_LOGGER_MEM_ID)) == -1) {
		pr_err("[SCP] scp_logger_init_fail\n");
		return -1;
	}
	pr_debug("[SCP B] logger init\n");
	if (scp_B_logger_init(scp_get_reserve_mem_virt(SCP_B_LOGGER_MEM_ID),
				scp_get_reserve_mem_size(SCP_B_LOGGER_MEM_ID)) == -1) {
		pr_err("[SCP B] scp_B_logger_init_fail\n");
		return -1;
	}
#endif

#if ENABLE_SCP_EMI_PROTECTION
	set_scp_mpu();
#endif
	driver_init_done = true;
	reset_scp(SCP_ALL_ENABLE);

	return ret;
}

/*
 * driver exit point
 */
static void __exit scp_exit(void)
{
#if SCP_BOOT_TIME_OUT_MONITOR
	int i = 0;
#endif

	free_irq(scpreg.irq, NULL);
	misc_deregister(&scp_device);
	misc_deregister(&scp_B_device);
	flush_workqueue(scp_workqueue);
	/*scp_logger_cleanup();*/
	destroy_workqueue(scp_workqueue);
#if SCP_BOOT_TIME_OUT_MONITOR
	for (i = 0; i < SCP_CORE_TOTAL ; i++)
		del_timer(&scp_ready_timer[i]);
#endif
	kfree(scp_swap_buf);
}

module_init(scp_init);
module_exit(scp_exit);
