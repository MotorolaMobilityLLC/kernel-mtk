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

struct mutex scp_awake_mutexs[SCP_CORE_TOTAL];
int scp_awake_counts[SCP_CORE_TOTAL];
unsigned int scp_ipi_awake_number[SCP_CORE_TOTAL] = {SCP_A_IPI_AWAKE_NUM, SCP_B_IPI_AWAKE_NUM};
unsigned int scp_deep_sleep_bits[SCP_CORE_TOTAL] = {SCP_A_DEEP_SLEEP_BIT, SCP_B_DEEP_SLEEP_BIT};
unsigned int scp_semaphore_flags[SCP_CORE_TOTAL] = {SEMAPHORE_SCP_A_AWAKE, SEMAPHORE_SCP_B_AWAKE};
struct wake_lock scp_awake_wakelock[SCP_CORE_TOTAL];

/*
 * acquire scp lock flag, keep scp awake
 * @param scp_core_id: scp core id
 * return  0 :get lock success
 *        -1 :get lock timeout
 */
int scp_awake_lock(scp_core_id scp_id)
{
	int status_read_back;
	unsigned long spin_flags;
	struct mutex *scp_awake_mutex;
	int scp_semaphore_flag;
	char *core_id;
	unsigned int scp_ipi_awake_num;
	unsigned int scp_deep_sleep_bit;
	int *scp_awake_count;
	int count = 0;
	int ret = -1;
	int scp_awake_repeat = 0;

	if (scp_id >= SCP_CORE_TOTAL) {
		pr_err("scp_awake_lock: SCP ID >= SCP_CORE_TOTAL\n");
		return ret;
	}

	scp_awake_mutex = &scp_awake_mutexs[scp_id];
	scp_semaphore_flag = scp_semaphore_flags[scp_id];
	scp_ipi_awake_num = scp_ipi_awake_number[scp_id];
	scp_deep_sleep_bit = scp_deep_sleep_bits[scp_id];
	scp_awake_count = (int *)&scp_awake_counts[scp_id];
	core_id = core_ids[scp_id];

	if (is_scp_ready(scp_id) == 0) {
		pr_err("scp_awake_lock: %s not enabled\n", core_id);
		return ret;
	}

	/* scp unlock awake */
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	if (*scp_awake_count > 0) {
		*scp_awake_count = *scp_awake_count + 1;
		spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
		return 0;
	}
	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);

	if (mutex_trylock(scp_awake_mutex) == 0) {
		/*avoid scp ipi send log print too much*/
		pr_err("scp_awake_lock: %s mutex_trylock busy..\n", core_id);
		return ret;
	}

	/* get scp sema to keep scp awake*/
	ret = get_scp_semaphore(scp_semaphore_flag);
	if (ret == -1) {
		*scp_awake_count = *scp_awake_count + 1;
		pr_err("scp_awake_lock: %s get sema fail, scp_awake_count=%d\n", core_id, *scp_awake_count);
		mutex_unlock(scp_awake_mutex);
		return ret;
	}

	/* spinlock context safe */
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);

	/* WE1: set a direct IPI to awake SCP */
	/*pr_debug("scp_awake_lock: try to awake %s\n", core_id);*/
	writel((1 << scp_ipi_awake_num), SCP_GIPC_REG);

	/*
	 * check SCP awake status
	 * SCP_CPU_SLEEP_STATUS
	 * bit[1] cpu_a_deepsleep = 0 ,means scp active now
	 * bit[3] cpu_b_deepsleep = 0 ,means scp active now
	 * wait 1 us and insure scp activing
	 */
	status_read_back = (readl(SCP_CPU_SLEEP_STATUS) >> scp_deep_sleep_bit) & 0x1;

	scp_awake_repeat = 0;
	count = 0;
	while (count != SCP_AWAKE_TIMEOUT) {
		if (status_read_back == 0)
			scp_awake_repeat++;

		udelay(10);
		status_read_back = (readl(SCP_CPU_SLEEP_STATUS) >> scp_deep_sleep_bit) & 0x1;
		if (status_read_back == 0 && scp_awake_repeat > 0) {
			ret = 0;
			break;
		}
		count++;
	}

	/* scp lock awake success*/
	if (ret != -1)
		*scp_awake_count = *scp_awake_count + 1;

	/* spinlock context safe */
	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);

	if (ret == -1) {
		pr_err("scp_awake_lock: awake %s fail..\n", core_id);
		WARN_ON(1);
	}

	/* scp awake */
	mutex_unlock(scp_awake_mutex);
	/*pr_debug("scp_awake_lock: %s lock, count=%d\n", core_id, *scp_awake_count);*/
	return ret;
}
EXPORT_SYMBOL_GPL(scp_awake_lock);


/*
 * release scp awake lock flag
 * @param scp_core_id: scp core id
 * return  0 :release lock success
 *        -1 :release lock fail
 */
int scp_awake_unlock(scp_core_id scp_id)
{
	struct mutex *scp_awake_mutex;
	unsigned long spin_flags;
	int scp_semaphore_flag;
	int *scp_awake_count;
	char *core_id;
	int ret = -1;
	unsigned int scp_ipi_awake_num;

	scp_ipi_awake_num = scp_ipi_awake_number[scp_id];
	if (scp_id >= SCP_CORE_TOTAL) {
		pr_err("scp_awake_unlock: SCP ID >= SCP_CORE_TOTAL\n");
		return ret;
	}

	scp_awake_mutex = &scp_awake_mutexs[scp_id];
	scp_semaphore_flag = scp_semaphore_flags[scp_id];
	scp_awake_count = (int *)&scp_awake_counts[scp_id];
	core_id = core_ids[scp_id];

	if (is_scp_ready(scp_id) == 0) {
		pr_err("scp_awake_unlock: %s not enabled\n", core_id);
		return ret;
	}

	/* scp unlock awake */
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	if (*scp_awake_count > 1) {
		*scp_awake_count = *scp_awake_count - 1;
		spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
		return 0;
	}
	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);

	if (mutex_trylock(scp_awake_mutex) == 0) {
		/*avoid scp ipi send log print too much*/
		pr_err("scp_awake_unlock: %s mutex_trylock busy..\n", core_id);
		return ret;
	}

	/* get scp sema to keep scp awake*/
	ret = release_scp_semaphore(scp_semaphore_flag);
	if (ret == -1) {
		pr_err("scp_awake_unlock: %s release sema. fail, scp_awake_count=%d\n", core_id, *scp_awake_count);
		mutex_unlock(scp_awake_mutex);
		WARN_ON(1);
		return ret;
	}
	ret = 0;

	/* spinlock context safe */
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);

	/* WE1: set a direct IPI to awake SCP */
	/*pr_debug("scp_awake_unlock: try to awake %s\n", core_id);*/
	writel((1 << scp_ipi_awake_num), SCP_GIPC_REG);

	/* scp unlock awake success*/
	if (ret != -1) {
		*scp_awake_count = *scp_awake_count - 1;
		if (*scp_awake_count < 0)
			pr_err("scp_awake_unlock:%s scp_awake_count=%d NOT SYNC!\n", core_id, *scp_awake_count);
	}

	/* spinlock context safe */
	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);

	mutex_unlock(scp_awake_mutex);
	/*pr_debug("scp_awake_unlock: %s unlock, count=%d\n", core_id, *scp_awake_count);*/
	return ret;
}
EXPORT_SYMBOL_GPL(scp_awake_unlock);

void scp_awake_init(void)
{
	int i = 0;
	/* scp ready static flag initialise */
	for (i = 0; i < SCP_CORE_TOTAL ; i++)
		scp_awake_counts[i] = 0;

	for (i = 0; i < SCP_CORE_TOTAL ; i++) {
		mutex_init(&scp_awake_mutexs[i]);
		wake_lock_init(&scp_awake_wakelock[i], WAKE_LOCK_SUSPEND, "scp awakelock");
	}
}


