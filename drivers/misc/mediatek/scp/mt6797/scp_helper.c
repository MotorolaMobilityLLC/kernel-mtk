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
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by kmalloc */
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
#include <asm/io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/aee.h>
#include <linux/delay.h>
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_excep.h"

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#endif

#define ENABLE_SCP_EMI_PROTECTION       (1)
#if ENABLE_SCP_EMI_PROTECTION
#include <emi_mpu.h>
#endif
#define ENABLE_SCP_DRAM_FLAG_PROTECTION       (1)
#if ENABLE_SCP_DRAM_FLAG_PROTECTION
#include <mt_vcorefs_manager.h>
#endif

#define MPU_REGION_ID_SCP_SMEM       22

#define TIMEOUT 5000
#define IS_CLK_DMA_EN 0x40000
#define SCP_READY_TIMEOUT (2 * HZ) /* 2 seconds*/

#define EXPECTED_FREQ_REG SCP_GENERAL_REG3
#define CURRENT_FREQ_REG  SCP_GENERAL_REG4

phys_addr_t scp_mem_base_phys = 0x0;
phys_addr_t scp_mem_base_virt = 0x0;
phys_addr_t scp_mem_size = 0x0;
struct scp_regs scpreg;
unsigned char *scp_send_buff;
unsigned char *scp_recv_buff;
static struct workqueue_struct *scp_workqueue;
static unsigned int scp_ready;
static int scp_hold_vcore_flag;/* 1:hold vcore v1.0,  0:release vcore v1.0 */
static struct timer_list scp_ready_timer;
static struct scp_work_struct scp_notify_work;
static struct mutex scp_notify_mutex;
static struct mutex scp_feature_mutex;
unsigned char **scp_swap_buf;
static struct wake_lock scp_suspend_lock;
typedef struct {
	u64 start;
	u64 size;
} mem_desc_t;

static scp_reserve_mblock_t scp_reserve_mblock[] = {
	{
		.num = VOW_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x9000,/*36KB*/
	},
	{
		.num = SENS_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x800000,/*8MB*/
	},
	{
		.num = MP3_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x400000,/*4MB*/
	},
	{
		.num = FLP_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x1000,/*4KB*/
	},
	{
		.num = RTOS_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x100000,/*1MB*/
	},
	{
		.num = OPENDSP_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x200000,/*2MB*/
	},
	{
		.num = SENS_MEM_DIRECT_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x2000,/*8KB*/
	},
};

static scp_feature_table_t feature_table[] = {
	{
		.feature    = VOW_FEATURE_ID,
		.freq       = 80,
		.enable     = 0,
	},
	{
		.feature    = OPEN_DSP_FEATURE_ID,
		.freq       = 270,
		.enable     = 0,
	},
	{
		.feature    = SENS_FEATURE_ID,
		.freq       = 84,
		.enable     = 0,
	},
	{
		.feature    = MP3_FEATURE_ID,
		.freq       = 20,
		.enable     = 0,
	},
	{
		.feature    = FLP_FEATURE_ID,
		.freq       = 26,
		.enable     = 0,
	},
	{
		.feature    = RTOS_FEATURE_ID,
		.freq       = 0,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST_FEATURE_ID,
		.freq       = 270,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST2_FEATURE_ID,
		.freq       = 120,
		.enable     = 0,
	},

};


void memcpy_to_scp(void __iomem *trg, const void *src, int size)
{
	int i;
	u32 __iomem *t = trg;
	const u32 *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

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
 */
int get_scp_semaphore(int flag)
{
	int read_back;
	int count = 0;
	int ret = -1;

	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 0) {
		writel((1 << flag), SCP_SEMAPHORE);

		while (count != TIMEOUT) {
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
			pr_debug("[SCP] get scp semaphore %d TIMEOUT...!\n", flag);
	} else {
		pr_debug("[SCP] already hold scp semaphore %d\n", flag);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(get_scp_semaphore);

/*
 * release a hardware semaphore
 * @param flag: semaphore id
 */
int release_scp_semaphore(int flag)
{
	int read_back;
	int ret = -1;

	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 1) {
		/* Write 1 clear */
		writel((1 << flag), SCP_SEMAPHORE);
		read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;
		if (read_back == 0)
			ret = 1;
		else
			pr_debug("[SCP] release scp semaphore %d failed!\n", flag);
	} else {
		pr_debug("[SCP] try to release semaphore %d not own by me\n", flag);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(release_scp_semaphore);

static BLOCKING_NOTIFIER_HEAD(scp_notifier_list);

/*
 * register apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:   notifier block struct
 */
void scp_register_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_notify_mutex);
	blocking_notifier_chain_register(&scp_notifier_list, nb);

	pr_debug("[SCP] invoke callback now\n");

	if (is_scp_ready())
		nb->notifier_call(nb, SCP_EVENT_READY, NULL);
	mutex_unlock(&scp_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_register_notify);

/*
 * unregister apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:     notifier block struct
 */
void scp_unregister_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_notify_mutex);
	blocking_notifier_chain_unregister(&scp_notifier_list, nb);
	mutex_unlock(&scp_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_unregister_notify);

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
static void scp_notify_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws = container_of(ws, struct scp_work_struct, work);
	unsigned int scp_notify_flag = sws->flags;

	if (scp_notify_flag) {
		mutex_lock(&scp_notify_mutex);
		blocking_notifier_call_chain(&scp_notifier_list, SCP_EVENT_READY, NULL);
		mutex_unlock(&scp_notify_mutex);
	}

	scp_ready = scp_notify_flag;

	if (!scp_ready)
		scp_aed(EXCEP_RESET);

}

/*
 * mark notify flag to 1 to notify apps to start their tasks
 */
static void scp_set_ready(void)
{
	pr_debug("%s()\n", __func__);
	del_timer(&scp_ready_timer);
	scp_notify_work.flags = 1;
	scp_schedule_work(&scp_notify_work);
}

/*
 * callback for reset timer
 * mark notify flag to 0 to generate an exception
 * @param data: unuse
 */
static void scp_wait_ready_timeout(unsigned long data)
{
	pr_debug("%s()\n", __func__);
	scp_notify_work.flags = 0;
	scp_schedule_work(&scp_notify_work);
}

/*
 * handle notification from scp
 * mark scp is ready for running tasks
 * @param id:   ipi id
 * @param data: ipi data
 * @param len:  length of ipi data
 */
static void scp_ready_ipi_handler(int id, void *data, unsigned int len)
{
	if (!scp_ready)
		scp_set_ready();
}

/*
 * @return: 1 if scp is ready for running tasks
 */
unsigned int is_scp_ready(void)
{
	if (scp_ready)
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
 * @param reset:    0 for first boot, 1 for reset
 * @return:         0 if success
 */
int reset_scp(int reset)
{
	unsigned int prev_ready;
	unsigned int *reg;

	del_timer(&scp_ready_timer);

	prev_ready = scp_ready;
	scp_ready = 0;

	if (reset && prev_ready) {
		mutex_lock(&scp_notify_mutex);
		blocking_notifier_call_chain(&scp_notifier_list, SCP_EVENT_STOP, NULL);
		mutex_unlock(&scp_notify_mutex);
	}

	scp_logger_stop();
	scp_excep_reset();

	pr_debug("[SCP] reset scp\n");
	reg = (unsigned int *)scpreg.cfg;

	if (reset) {
		*(unsigned int *)reg = 0x0;
		dsb(SY);
	}
	*(unsigned int *)reg = 0x1;

	/*set timer for scp bring up time out monitor*/
	init_timer(&scp_ready_timer);
	scp_ready_timer.expires = jiffies + SCP_READY_TIMEOUT;
	scp_ready_timer.function = &scp_wait_ready_timeout;
	scp_ready_timer.data = (unsigned long) 0;
	add_timer(&scp_ready_timer);

	return 0;
}
/*
 * parse device tree and mapping iomem
 * @return: 0 if success
 */
static int scp_dt_init(void)
{
	int ret = 0;
	const char *status;
	struct device_node *node = NULL;
	struct resource res;

	node = of_find_compatible_node(NULL, NULL, "mediatek,scp");
	if (!node) {
		pr_err("[SCP] Can't find node: mediatek,scp\n");
		return -1;
	}

	status = of_get_property(node, "status", NULL);
	if (status == NULL || strcmp(status, "okay") != 0) {
		if (strcmp(status, "fail") == 0)
			scp_aed(EXCEP_LOAD_FIRMWARE);
		return -1;
	}

	scpreg.sram = of_iomap(node, 0);
	pr_debug("[SCP] scp sram base=0x%p\n", scpreg.sram);

	if (!scpreg.sram) {
		pr_err("[SCP] Unable to ioremap mregisters\n");
		return -1;
	}

	scpreg.cfg = of_iomap(node, 1);
	pr_debug("[SCP] scp irq=%d, cfgreg=0x%p\n", scpreg.irq, scpreg.cfg);

	if (!scpreg.cfg) {
		pr_err("[SCP] Unable to ioremap registers\n");
		return -1;
	}

	scpreg.clkctrl = of_iomap(node, 2);
	if (!scpreg.clkctrl) {
		pr_err("[SCP] Unable to ioremap registers\n");
		return -1;
	}

	scpreg.irq = irq_of_parse_and_map(node, 0);

	if (of_address_to_resource(node, 0, &res)) {
		pr_err("[SCP] Unable to get dts resource (TCM Size)\n");
		return -1;
	}
	scpreg.tcmsize = (unsigned int)resource_size(&res);

	if (of_address_to_resource(node, 1, &res)) {
		pr_err("[SCP] Unable to get dts resource (CFGREG Size)\n");
		return -1;
	}
	scpreg.cfgregsize = (unsigned int)resource_size(&res);

	return ret;
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


static inline ssize_t scp_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	if (scp_ready)
		return scnprintf(buf, PAGE_SIZE, "SCP is ready\n");
	else
		return scnprintf(buf, PAGE_SIZE, "SCP is not ready\n");
}

DEVICE_ATTR(scp_status, 0444, scp_status_show, NULL);

static inline ssize_t scp_reg_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	int len = 0;

	scp_dump_regs();
	if (scp_ready) {
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_DEBUG_PC_REG:0x%x\n", SCP_DEBUG_PC_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_DEBUG_LR_REG:0x%x\n", SCP_DEBUG_LR_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_DEBUG_PSP_REG:0x%x\n", SCP_DEBUG_PSP_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_DEBUG_SP_REG:0x%x\n", SCP_DEBUG_SP_REG);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_GENERAL_REG0:0x%x\n", SCP_GENERAL_REG0);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_GENERAL_REG1:0x%x\n", SCP_GENERAL_REG1);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_GENERAL_REG2:0x%x\n", SCP_GENERAL_REG2);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_GENERAL_REG3:0x%x\n", SCP_GENERAL_REG3);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_GENERAL_REG4:0x%x\n", SCP_GENERAL_REG4);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[SCP] SCP_GENERAL_REG5:0x%x\n", SCP_GENERAL_REG5);
		return len;
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP is not ready\n");
}

DEVICE_ATTR(scp_reg_status, 0444, scp_reg_status_show, NULL);

static inline ssize_t scp_vcore_request_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "SCP freq. expect=%d, cur=%d\n", EXPECTED_FREQ_REG, CURRENT_FREQ_REG);
}

static ssize_t scp_vcore_request_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	if (scp_hold_vcore_flag == 0) {
		register_feature(VCORE_TEST2_FEATURE_ID);
		register_feature(VCORE_TEST_FEATURE_ID);
		request_freq();
		pr_err("[SCP] set high freq(vcore to v1.0), expect=%d, cur=%d\n", EXPECTED_FREQ_REG, CURRENT_FREQ_REG);
	} else {
		deregister_feature(VCORE_TEST_FEATURE_ID);
		request_freq();
		pr_err("[SCP] set low freq, expect=%d, cur=%d\n", EXPECTED_FREQ_REG, CURRENT_FREQ_REG);
	}
	return n;
}

DEVICE_ATTR(scp_vcore_request, 0644, scp_vcore_request_show, scp_vcore_request_store);
#ifdef CONFIG_MT_ENG_BUILD
static inline ssize_t scp_db_test_show(struct device *kobj, struct device_attribute *attr, char *buf)
{

	if (scp_ready) {
		scp_aed_reset(EXCEP_RUNTIME);
		return scnprintf(buf, PAGE_SIZE, "dumping SCP db\n");
	} else
		return scnprintf(buf, PAGE_SIZE, "SCP is not ready\n");
}

DEVICE_ATTR(scp_db_test, 0444, scp_db_test_show, NULL);
#endif

static struct miscdevice scp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "scp",
	.fops = &scp_file_ops
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

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_log_len);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_mobile_log);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_log_flush);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_status);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_bin_file(scp_device.this_device, &bin_attr_scp_dump);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_reg_status);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_vcore_request);

	if (unlikely(ret != 0))
		return ret;
#ifdef CONFIG_MT_ENG_BUILD
	/*only support debug db test in engineer build*/
	ret = device_create_file(scp_device.this_device, &dev_attr_scp_db_test);

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
		 * can be accessible*/
		pr_err("[SCP] The allocated memory(0x%llx) is larger than expected\n", scp_mem_base_phys);
		/*should not call BUG() here or there is no log, return -1
		 * instead.*/
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
phys_addr_t get_reserve_mem_phys(scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_err("[SCP] no reserve memory for 0x%d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].start_phys;
}
EXPORT_SYMBOL_GPL(get_reserve_mem_phys);

phys_addr_t get_reserve_mem_virt(scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_err("[SCP] no reserve memory for 0x%d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].start_virt;
}
EXPORT_SYMBOL_GPL(get_reserve_mem_virt);

phys_addr_t get_reserve_mem_size(scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_err("[SCP] no reserve memory for 0x%d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].size;
}
EXPORT_SYMBOL_GPL(get_reserve_mem_size);


static void scp_reserve_memory_ioremap(void)
{
	scp_reserve_mem_id_t id;
	phys_addr_t accumlate_memory_size;


	if ((scp_mem_base_phys >= (0x90000000ULL)) || (scp_mem_base_phys <= 0x0)) {
		/*The scp remap region is fixed, only
		 * 0x4000_0000ULL~0x8FFF_FFFFULL
		 * can be accessible*/
		pr_err("[SCP] The allocated memory(0x%llx) is larger than expected\n", scp_mem_base_phys);
		/*call BUG() here to assert the unexpected memory allocation*/
		BUG();
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
	 * or scp_reserve_mblock does not match dts*/
	BUG_ON(accumlate_memory_size > scp_mem_size);
#ifdef DEBUG
	for (id = 0; id < NUMS_MEM_ID; id++) {
		pr_debug("[SCP][mem_reserve-%d] phys:0x%llx,virt:0x%llx,size:0x%llx\n",
			id, get_reserve_mem_phys(id), get_reserve_mem_virt(id), get_reserve_mem_size(id));
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
uint32_t get_freq(void)
{
	uint32_t i;
	uint32_t sum = 0;
	uint32_t return_freq = 0;

	mutex_lock(&scp_feature_mutex);
	for (i = 0; i < NUM_FEATURE_ID; i++) {
		if (feature_table[i].enable == 1)
			sum += feature_table[i].freq;
	}
	mutex_unlock(&scp_feature_mutex);
	/*pr_debug("[SCP] needed freq sum:%d\n",sum);*/
	if (sum > FREQ_224MHZ) {
		return_freq = FREQ_354MHZ;
#if ENABLE_SCP_DRAM_FLAG_PROTECTION
		/* scp request vcore to v1.0 and need to insure
		 * dram frequence has more than 800 Mhz
		 * get the dram flag*/
		if (scp_hold_vcore_flag == 0) {
			mutex_lock(&scp_feature_mutex);
			pr_debug("[SCP]req vcore 1.0, exp=%d, cur=%d\n", EXPECTED_FREQ_REG, CURRENT_FREQ_REG);
			vcorefs_request_dvfs_opp(KIR_SCP, OPPI_LOW_PWR);
			scp_hold_vcore_flag = 1;
			mutex_unlock(&scp_feature_mutex);
		}
#endif
	} else if (sum > FREQ_112MHZ)
		return_freq = FREQ_224MHZ;
	else
		return_freq = FREQ_112MHZ;

	return return_freq;
}
void register_feature(feature_id_t id)
{
	uint32_t i;
	/* because feature_table is a global variable, use mutex lock to protect it from
	 * accessing in the same time*/
	mutex_lock(&scp_feature_mutex);
	for (i = 0; i < NUM_FEATURE_ID; i++) {
		if (feature_table[i].feature == id)
			feature_table[i].enable = 1;
	}
	mutex_unlock(&scp_feature_mutex);
	EXPECTED_FREQ_REG = get_freq();
}

void deregister_feature(feature_id_t id)
{
	uint32_t i;

	mutex_lock(&scp_feature_mutex);
	for (i = 0; i < NUM_FEATURE_ID; i++) {
		if (feature_table[i].feature == id)
			feature_table[i].enable = 0;
	}
	mutex_unlock(&scp_feature_mutex);
	EXPECTED_FREQ_REG = get_freq();
}
int check_scp_resource(void)
{
	/* called by lowpower related function
	 * main purpose is to ensure main_pll is not disabled
	 * because scp needs main_pll to run at vcore 1.0 and 354Mhz
	 * return value:
	 * 1: main_pll shall be enabled, 26M shall be enabled, infra shall be enabled
	 * 0: main_pll may disable, 26M may disable, infra may disable
	 * */
	int scp_resource_status = 0;

	if (EXPECTED_FREQ_REG == FREQ_354MHZ || CURRENT_FREQ_REG == FREQ_354MHZ)
		scp_resource_status = 1;
	else
		scp_resource_status = 0;

	return scp_resource_status;
}

int scp_check_dram_resource(void)
{
	/* called by lowpower related function
	 * main purpose is to ensure when vcore is at v1.0, DRAM must not at 800Mhz
	 * return value:
	 * 1: scp hold DRAM upper 800Mhz flag
	 * 0: scp release DRAM upper 800Mhz flag
	 * */
	return scp_hold_vcore_flag;
}
int request_freq(void)
{
	int value = 0;
	int timeout = 50;

	/* because we are waiting for scp to update register:CURRENT_FREQ_REG
	 * use wake lock to prevent AP from entering suspend state
	 * */
	wake_lock(&scp_suspend_lock);

	while (CURRENT_FREQ_REG != EXPECTED_FREQ_REG) {
		scp_ipi_send(IPI_DVFS_SET_FREQ, (void *)&value, sizeof(value), 0);
		mdelay(2);
		timeout -= 1; /*try 50 times, total about 100ms*/
		if (timeout <= 0)
			goto fail_to_set_freq;
	}
#if ENABLE_SCP_DRAM_FLAG_PROTECTION
	/* scp will release vcore from v1.0 to lower stage
	 * but still hold dram vcore v1.0 flag and need to release
	 * here release the dram flag*/
	if ((scp_hold_vcore_flag == 1) && (EXPECTED_FREQ_REG != FREQ_354MHZ)) {
		mutex_lock(&scp_feature_mutex);
		pr_debug("[SCP]release vcore 1.0, exp=%d, cur=%d\n", EXPECTED_FREQ_REG, CURRENT_FREQ_REG);
		vcorefs_request_dvfs_opp(KIR_SCP, OPPI_UNREQ);
		scp_hold_vcore_flag = 0;
		mutex_unlock(&scp_feature_mutex);
	}
#endif
	wake_unlock(&scp_suspend_lock);
	pr_err("[SCP] set freq OK, %d == %d\n", EXPECTED_FREQ_REG, CURRENT_FREQ_REG);
	return 0;

fail_to_set_freq:
	scp_dump_regs();
	wake_unlock(&scp_suspend_lock);
	pr_err("[SCP] set freq fail, %d != %d\n", EXPECTED_FREQ_REG, CURRENT_FREQ_REG);
	return -1;
}

/*
 * driver initialization entry point
 */
static int __init scp_init(void)
{

	int ret = 0;

	/* static initialise */
	scp_ready = 0;
	scp_hold_vcore_flag = 0;

	mutex_init(&scp_notify_mutex);
	mutex_init(&scp_feature_mutex);
	wake_lock_init(&scp_suspend_lock, WAKE_LOCK_SUSPEND, "scp wakelock");

	scp_workqueue = create_workqueue("SCP_WQ");
	scp_excep_init();
	ret = scp_dt_init();
	if (ret) {
		pr_err("[SCP] Device Init Fail\n");
		return -1;
	}

	scp_send_buff = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_send_buff)
		return -1;

	scp_recv_buff = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_recv_buff)
		return -1;

	INIT_WORK(&scp_notify_work.work, scp_notify_ws);

	scp_irq_init();
	scp_ipi_init();

	scp_ipi_registration(IPI_SCP_READY, scp_ready_ipi_handler, "scp_ready");

	pr_debug("[SCP] helper init\n");

	if (scp_logger_init() == -1) {
		pr_err("[SCP] scp_logger_init_fail\n");
		return -1;
	}
	scp_ram_dump_init();
	ret = register_pm_notifier(&scp_pm_notifier_block);

	if (ret)
		pr_err("[SCP] failed to register PM notifier %d\n", ret);

	ret = create_files();

	if (unlikely(ret != 0)) {
		pr_err("[SCP] create files failed\n");
		return -1;
	}

	ret = request_irq(scpreg.irq, scp_irq_handler, IRQF_TRIGGER_NONE, "SCP IPC_MD2HOST", NULL);
	if (ret) {
		pr_err("[SCP] require irq failed\n");
		return -1;
	}

	scp_reserve_memory_ioremap();
#if ENABLE_SCP_EMI_PROTECTION
	set_scp_mpu();
#endif
	reset_scp(0);

	return ret;
}

/*
 * driver exit point
 */
static void __exit scp_exit(void)
{
	free_irq(scpreg.irq, NULL);
	misc_deregister(&scp_device);
	flush_workqueue(scp_workqueue);
	scp_logger_cleanup();
	destroy_workqueue(scp_workqueue);
	del_timer(&scp_ready_timer);
	kfree(scp_swap_buf);
}

module_init(scp_init);
module_exit(scp_exit);
