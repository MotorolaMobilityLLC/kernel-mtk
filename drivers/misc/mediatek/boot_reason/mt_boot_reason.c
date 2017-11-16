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

enum hw_secure_config {
	HW_SEC_SBC = 0,
	HW_SEC_DAA,
	HW_SEC_SLA,
	HW_SEC_MAX
};

char ret_hw_secure_info[64];
char ret_sw_secure_info[64];
#ifdef CONFIG_OF
static int __init dt_get_boot_reason(unsigned long node, const char *uname, int depth, void *data)
{
	char *ptr = NULL, *br_ptr = NULL;
	char *q;
	char *hw_sec_ptr = NULL, *sw_sec_ptr = NULL;
	char secure_info_temp[8]; //32bit 0xFFFFFFFF
	int hw_secure_info_num=0;
	int sw_secure_info_num=0;
	char *hw_sec_sbc = "SBC";
	char *hw_sec_daa = "DAA";
	char *hw_sec_sla = "SLA";

	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;

	ptr = (char *)of_get_flat_dt_prop(node, "bootargs", NULL);
	if (ptr) {
		br_ptr = strstr(ptr, "boot_reason=");
		if (br_ptr != 0) {
			g_boot_reason = br_ptr[12] - '0';	/* get boot reason */
			atomic_set(&g_br_status, 1);
		} else {
			pr_warn("'boot_reason=' is not found\n");
		}
		pr_debug("%s\n", ptr);
	} else{
		pr_warn("'bootargs' is not found\n");
	}

//gyw
	ptr = NULL;
	ptr = (char *)of_get_flat_dt_prop(node, "bootargs", NULL);
	if (ptr) {
		if ((hw_sec_ptr = strstr(ptr, "hwsec=0x")) != 0) {
			hw_sec_ptr += 8;
			if((hw_sec_ptr - ptr) >= (strlen(ptr)+1)) {
				pr_warn("[wt]error: can't find hwsec in cmdline\n");
			}
			q = hw_sec_ptr;
			while(*q != ' ' && *q != '\0') q++;

			memset((void*)secure_info_temp, 0, sizeof(secure_info_temp));
			strncpy((char*)secure_info_temp, (const char*)hw_sec_ptr, (int)(q-hw_sec_ptr));
			pr_warn("[wt] cmdline hwsec info = %s\n", secure_info_temp);
			sscanf(secure_info_temp, "%x", &hw_secure_info_num);
			pr_warn("[wt] cmdline hwsec info = 0x%x\n", hw_secure_info_num);

			memset(ret_hw_secure_info, 0, 64);//clear the buffer
			if (hw_secure_info_num != 0x00) {
				sprintf(ret_hw_secure_info,"%s", "HWSEC");
				if (hw_secure_info_num & (0x1 << HW_SEC_SBC)) {
					strcat(ret_hw_secure_info, " ");
					strcat(ret_hw_secure_info, hw_sec_sbc);
				}
				if (hw_secure_info_num & (0x1 << HW_SEC_DAA)) {
					strcat(ret_hw_secure_info, " ");
					strcat(ret_hw_secure_info, hw_sec_daa);
				}
				if (hw_secure_info_num & (0x1 << HW_SEC_SLA)) {
					strcat(ret_hw_secure_info, " ");
					strcat(ret_hw_secure_info, hw_sec_sla);
				}
			} else {
				sprintf(ret_hw_secure_info,"%s", "NON-HWSEC");
			}
		} else {
			pr_warn("[wt] cannot find hwsec info in cmdline");
		}

		if ((sw_sec_ptr = strstr(ptr, "swsec=0x")) != 0) {
			sw_sec_ptr += 8;
			if((sw_sec_ptr - ptr) >= (strlen(ptr)+1)) {
				pr_warn("[wt]error: can't find swsec in cmdline\n");
			}
			q = sw_sec_ptr;
			while(*q != ' ' && *q != '\0') q++;

			memset((void*)secure_info_temp, 0, sizeof(secure_info_temp));
			strncpy((char*)secure_info_temp, (const char*)sw_sec_ptr, (int)(q-sw_sec_ptr));
			pr_warn("[wt] cmdline swsec info = %s\n", secure_info_temp);
			sscanf(secure_info_temp, "%x", &sw_secure_info_num);
			pr_warn("[wt] cmdline swsec info = 0x%x\n", sw_secure_info_num);

			memset(ret_sw_secure_info, 0, 64);//clear the buffer
			if (sw_secure_info_num != 0x00) {
				sprintf(ret_sw_secure_info, "%s", "SWSEC");
			} else {
				sprintf(ret_sw_secure_info, "%s", "NON-SWSEC");
			}
		} else {
			pr_warn("[wt] cannot find swsec info in cmdline\n");
		}
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
	return 0;
}

early_initcall(boot_reason_core);
module_init(boot_reason_init);
MODULE_DESCRIPTION("Mediatek Boot Reason Driver");
MODULE_LICENSE("GPL");
