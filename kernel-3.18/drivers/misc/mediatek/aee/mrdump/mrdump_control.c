/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/delay.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <mt-plat/mrdump.h>
#include "mrdump_private.h"

/*kernel-3.10 and kernel-3.18 have ARCH define style CONFIG_ARCH_MTXXXX */

#if ((defined(CONFIG_ARCH_MT6795) || defined(CONFIG_ARCH_MT6752) || defined(CONFIG_ARCH_MT6580)\
			|| defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6735M) || defined(CONFIG_ARCH_MT6753)\
			|| defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6797) || defined(CONFIG_ARCH_MT6570))\
			&& defined(CONFIG_MTK_AEE_MRDUMP))
mrdump_rsvmem_block_t __initdata rsvmem_block[16];

static __init char *find_next_mrdump_rsvmem(char *p, int len)
{
	char *tmp_p;

	tmp_p = memchr(p, ',', len);
	if (!tmp_p)
		return NULL;
	if (*(tmp_p+1) != 0) {
		tmp_p = memchr(tmp_p+1, ',', strlen(tmp_p));
		if (!tmp_p)
			return NULL;
	} else{
		return NULL;
	}
	return tmp_p + 1;
}
static int __init early_mrdump_rsvmem(char *p)
{
	unsigned long start_addr, size;
	int ret;
	char *tmp_p = p;
	int i = 0;
	int max_count = sizeof(rsvmem_block)/sizeof(mrdump_rsvmem_block_t);

	while (1) {
		if (max_count <= 0)
			break;
		ret = sscanf(tmp_p, "0x%lx,0x%lx", &start_addr, &size);
		if (ret != 2) {
			pr_alert("%s:%s reserve failed ret=%d\n", __func__, p, ret);
			return 0;
		}

		if (start_addr == 0 || size == 0) {
			pr_alert("%s:i=%d start_addr = 0x%lx size=0x%lx skip\n", __func__, i, start_addr, size);
			continue;
		}

		rsvmem_block[i].start_addr = start_addr;
		rsvmem_block[i].size = size;
		i++;
		max_count--;
		tmp_p = find_next_mrdump_rsvmem(tmp_p, strlen(tmp_p));
		if (!tmp_p)
			break;
	}

	for (i = 0; i < sizeof(rsvmem_block)/sizeof(mrdump_rsvmem_block_t); i++) {
		if (rsvmem_block[i].start_addr)
			pr_err(" mrdump region start = %pa size =%pa\n",
						&rsvmem_block[i].start_addr, &rsvmem_block[i].size);
	}

	return 0;
}

__init void mrdump_rsvmem(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (rsvmem_block[i].start_addr) {
			if (!memblock_is_region_reserved(rsvmem_block[i].start_addr, rsvmem_block[i].size))
				memblock_reserve(rsvmem_block[i].start_addr, rsvmem_block[i].size);
			else {
				pr_warn(" mrdump region start = %pa size =%pa is reserved already\n",
						&rsvmem_block[i].start_addr, &rsvmem_block[i].size);
			}
		}
	}
}

early_param("mrdump_rsvmem", early_mrdump_rsvmem);
#endif

#if !defined(CONFIG_MTK_AEE_MRDUMP)

int __init mrdump_init(void)
{
	return 0;
}

static atomic_t waiting_for_crash_ipi;

static void mrdump_stop_noncore_cpu(void *unused)
{
	local_irq_disable();
	__disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2();
	while (1)
		cpu_relax();
}

static void __mrdump_reboot_stop_all(void)
{
	unsigned long msecs;

	atomic_set(&waiting_for_crash_ipi, num_online_cpus() - 1);
	smp_call_function(mrdump_stop_noncore_cpu, NULL, false);

	msecs = 1000; /* Wait at most a second for the other cpus to stop */
	while ((atomic_read(&waiting_for_crash_ipi) > 0) && msecs) {
		mdelay(1);
		msecs--;
	}
	if (atomic_read(&waiting_for_crash_ipi) > 0)
		pr_warn("Non-crashing CPUs did not react to IPI\n");
}

void __mrdump_create_oops_dump(AEE_REBOOT_MODE reboot_mode, struct pt_regs *regs , const char *msg, ...)
{
	local_irq_disable();

#if defined(CONFIG_SMP)
	__mrdump_reboot_stop_all();
#endif
}

#endif
