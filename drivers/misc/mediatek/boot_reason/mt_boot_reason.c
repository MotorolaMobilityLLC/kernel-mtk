/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "["KBUILD_MODNAME"] " fmt
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#ifdef CONFIG_OF
#include <linux/of_fdt.h>
#endif
#include <asm/setup.h>
#include <linux/atomic.h>
#include <mt-plat/mt_boot_reason.h>


enum {
	BOOT_REASON_UNINIT = 0,
	BOOT_REASON_INITIALIZING = 1,
	BOOT_REASON_INITIALIZED = 2,
} BOOT_REASON_STATE;

enum boot_reason_t g_boot_reason __nosavedata = BR_UNKNOWN;

static atomic_t g_br_state = ATOMIC_INIT(BOOT_REASON_UNINIT);
static atomic_t g_br_errcnt = ATOMIC_INIT(0);
static atomic_t g_br_status = ATOMIC_INIT(0);


#ifdef CONFIG_LCT_BOOTINFO_SUPPORT
/*	Lenovo(MOTO) boot reason list	*/
#define TIME_OF_DAY_ALARM	0x00000008
#define USB_CABLE			0x00000010
#define FACTORY_CABLE		0x00000020
#define PWR_KEY_PRESS		0x00000080
#define CHARGE				0x00000100
#define POWER_CUT			0x00000200
#define SYSTEM_RESTART		0x00000800
#define SW_AP_RESET			0x00004000
#define WDOG_AP_RESET		0x00008000
#define AP_KERNEL_PANIC		0x00020000
#define HW_RESET			0x00100000
#define PMIC_WD				0x00400000
#define INVALID_TURN_ON		0x00000000
/*
typedef enum {
    BR_POWER_KEY = 0,
    BR_USB,
    BR_RTC,
    BR_WDT,
    BR_WDT_BY_PASS_PWK,
    BR_TOOL_BY_PASS_PWK,
    BR_2SEC_REBOOT,
    BR_UNKNOWN,
    BR_KERNEL_PANIC,
    BR_WDT_SW,
    BR_WDT_HW
	,BR_CHR,
	BR_PWR_RST
} boot_reason_t;
*/
static int bootinfo_map[]={
	/* Lenovo(MOTO) */		/*   Mediatek	*/
	PWR_KEY_PRESS,			/*BR_POWER_KEY*/
	USB_CABLE,				/*BR_USB*/
	TIME_OF_DAY_ALARM,		/*BR_RTC*/
	SW_AP_RESET,			/*BR_WDT*/
	SW_AP_RESET,			/*BR_WDT_BY_PASS_PWK*/
	SW_AP_RESET,			/*BR_TOOL_BY_PASS_PWK*/
	POWER_CUT,				/*BR_2SEC_REBOOT*/
	SW_AP_RESET,			/*BR_UNKNOWN*/
	AP_KERNEL_PANIC,		/*BR_KERNEL_PANIC*/
	SW_AP_RESET,			/*BR_WDT_SW*/
	SW_AP_RESET,			/*BR_WDT_HW*/
	CHARGE,					/*BR_CHR*/
	HW_RESET				/*BR_PWR_RST*/
};
extern int aee_rr_last_fiq_step(void);
static int proc_bootinfo_show(struct seq_file *m, void *v)
{
	char ret[16];
	int bootnum;
	if (BOOT_REASON_INITIALIZED != atomic_read(&g_br_state))
		return 0;
	bootnum=(int)get_boot_reason();
#ifdef CONFIG_MTK_RAM_CONSOLE
		if (aee_rr_last_fiq_step() != 0)
			bootnum = 8;//BR_KERNEL_PANIC;
#endif
	printk("boot reason num:%d\n",bootnum);
	snprintf(ret,16,"0x%08x\n",bootinfo_map[bootnum]);
	seq_puts(m, ret);

	return 0;
}

static int proc_bootinfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_bootinfo_show, NULL);
}

static const struct file_operations bootinfo_proc_fops = {
	.open = proc_bootinfo_open,
	.read = seq_read,
};

#endif


#ifdef CONFIG_OF
static int __init dt_get_boot_reason(unsigned long node, const char *uname, int depth, void *data)
{
	char *ptr = NULL, *br_ptr = NULL;

	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;

	ptr = (char *)of_get_flat_dt_prop(node, "bootargs", NULL);
	if (ptr) {
		br_ptr = strstr(ptr, "boot_reason=");
		if (br_ptr != 0) {
			g_boot_reason = br_ptr[12] - '0';	/* get boot reason */
#ifdef CONFIG_LCT_BOOTINFO_SUPPORT
			if(br_ptr[13]>='0' && br_ptr[13]<='9')
				g_boot_reason = g_boot_reason*10 + (br_ptr[13] - '0');
#endif
			atomic_set(&g_br_status, 1);
		} else {
			pr_warn("'boot_reason=' is not found\n");
		}
		pr_debug("%s\n", ptr);
	} else
		pr_warn("'bootargs' is not found\n");

	/* break now */
	return 1;
}
#endif


void init_boot_reason(unsigned int line)
{
#ifdef CONFIG_OF
	int rc;

	if (BOOT_REASON_INITIALIZING == atomic_read(&g_br_state)) {
		pr_warn("%s (%d) state(%d)\n", __func__, line, atomic_read(&g_br_state));
		atomic_inc(&g_br_errcnt);
		return;
	}

	if (BOOT_REASON_UNINIT == atomic_read(&g_br_state))
		atomic_set(&g_br_state, BOOT_REASON_INITIALIZING);
	else
		return;

	if (BR_UNKNOWN != g_boot_reason) {
		atomic_set(&g_br_state, BOOT_REASON_INITIALIZED);
		pr_warn("boot_reason = %d\n", g_boot_reason);
		return;
	}

	pr_debug("%s %d %d %d\n", __func__, line, g_boot_reason, atomic_read(&g_br_state));
	rc = of_scan_flat_dt(dt_get_boot_reason, NULL);
	if (0 != rc)
		atomic_set(&g_br_state, BOOT_REASON_INITIALIZED);
	else
		atomic_set(&g_br_state, BOOT_REASON_UNINIT);
	pr_debug("%s %d %d %d\n", __func__, line, g_boot_reason, atomic_read(&g_br_state));
#endif
}

/* return boot reason */
enum boot_reason_t get_boot_reason(void)
{
	init_boot_reason(__LINE__);
	return g_boot_reason;
}

static int __init boot_reason_core(void)
{
	init_boot_reason(__LINE__);
	return 0;
}

static int __init boot_reason_init(void)
{
	pr_debug("boot_reason = %d, state(%d,%d,%d)", g_boot_reason,
		 atomic_read(&g_br_state), atomic_read(&g_br_errcnt), atomic_read(&g_br_status));
#ifdef CONFIG_LCT_BOOTINFO_SUPPORT
	proc_create("bootinfo", S_IRUGO | S_IWUSR, NULL, &bootinfo_proc_fops);
#endif
	return 0;
}

early_initcall(boot_reason_core);
module_init(boot_reason_init);
MODULE_DESCRIPTION("Mediatek Boot Reason Driver");
MODULE_LICENSE("GPL");
