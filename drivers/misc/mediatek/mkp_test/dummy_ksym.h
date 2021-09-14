/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc.
 */

#ifndef _DUMMY_KSYM_H_
#define _DUMMY_KSYM_H_

#include <asm/memory.h> // for MODULE_VADDR
#include <linux/arm-smccc.h>
#include <linux/cma.h>
#include <linux/err.h>
#include <linux/futex.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/of_reserved_mem.h>
#include <linux/printk.h>
#include <linux/rbtree.h>
#include <linux/random.h>
#include <linux/plist.h>
#include <linux/percpu-defs.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <uapi/linux/sched/types.h>

#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direct.h>

#define KV		kimage_vaddr
#define S_MAX		SZ_128M
#define SM_SIZE		28
#define TT_SIZE		256
#define NAME_LEN	128

void *mkp_abt_addr(void *ssa);
unsigned long *mkp_krb_addr(void);
unsigned int *mkp_km_addr(void);
u16 *mkp_kti_addr(void);
int mkp_ka_init(void);
unsigned int mkp_checking_names(unsigned int off,
					   char *namebuf, size_t buflen);
unsigned long mkp_idx2addr(int idx);
unsigned long mkp_addr_find(const char *name);
void mkp_get_krn_code(void **p_stext, void **p_etext);
void mkp_get_krn_rodata(void **p_etext, void **p__init_begin);
void mkp_get_krn_info(void **p_stext, void **p_etext, void **p__init_begin);

#endif /* _DUMMY_KSYM_H_ */

/*****************************************************
 *  debug.h //May remove later
 ******************************************************/
#define MKP_DEBUG(fmt, args...) do { pr_info("MKP_DEBUG: "fmt, ##args);} while (0)
#define MKP_ERR(fmt, args...) do { pr_info("MKP_ERR: "fmt, ##args);} while (0)
#define MKP_WARN(fmt, args...) do { pr_info("MKP_WARN: "fmt, ##args); } while (0)
#define MKP_INFO(fmt, args...) do { pr_info("MKP_INFO: "fmt, ##args); } while (0)
