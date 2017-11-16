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
#include <linux/io.h>
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/printk.h>

#include <mt-plat/mt_chip.h>
#include "mt_chip_common.h"

extern u32 __attribute__((weak)) get_devinfo_with_index(u32);

void __iomem *APHW_CODE = NULL;
void __iomem *APHW_SUBCODE = NULL;
void __iomem *APHW_VER = NULL;
void __iomem *APSW_VER = NULL;

/* return hardware version */
unsigned int __chip_hw_code(void)
{
	return (APHW_CODE) ? readl(IOMEM(APHW_CODE)) : (0);
}

unsigned int __chip_hw_ver(void)
{
	return (APHW_VER) ? readl(IOMEM(APHW_VER)) : (0);
}

unsigned int __chip_sw_ver(void)
{
	return (APSW_VER) ? readl(IOMEM(APSW_VER)) : (0);
}

unsigned int __chip_hw_subcode(void)
{
	return (APHW_SUBCODE) ? readl(IOMEM(APHW_SUBCODE)) : (0);
}

unsigned int __chip_func_code(void)
{
	unsigned int val0 = get_devinfo_with_index(9) & 0xF;	/*[3:0]:FUNCTION_CODE0*/
	unsigned int val1 = get_devinfo_with_index(14) & 0xF;	/*[3:0]:FUNCTION_CODE1*/
	unsigned int val = val0 | val1 << 4;

	return val;
}

unsigned int __chip_date_code(void)
{
	unsigned int val0 = get_devinfo_with_index(7) & 0x3F;	/*[5:0]:DATE_CODE_WW*/
	unsigned int val1 = get_devinfo_with_index(8) & 0xF;	/*[3:0]:DATE_CODE_YY*/
	unsigned int val = val0 | val1 << 6;

	return val;
}

unsigned int __chip_proj_code(void)
{
	unsigned int val = get_devinfo_with_index(5) & 0x00003FFF;	/*[13:0]:PROJECT_CODE*/
	return val;
}

unsigned int __chip_fab_code(void)
{
	unsigned int val = get_devinfo_with_index(15) & 0x00000007;	/*[2:0]:FAB_CODE*/
	return val;
}

unsigned int mt_get_chip_id(void)
{
	unsigned int chip_id = __chip_hw_code();
	/*convert id if necessary */
	return chip_id;
}
EXPORT_SYMBOL(mt_get_chip_id);

unsigned int mt_get_chip_hw_code(void)
{
	return __chip_hw_code();
}
EXPORT_SYMBOL(mt_get_chip_hw_code);

unsigned int mt_get_chip_hw_ver(void)
{
	return __chip_hw_ver();
}

unsigned int mt_get_chip_hw_subcode(void)
{
	return __chip_hw_subcode();
}

unsigned int mt_get_chip_sw_ver(void)
{
	return (unsigned int) __chip_sw_ver();
}
EXPORT_SYMBOL(mt_get_chip_sw_ver);

static chip_info_cb g_cbs[CHIP_INFO_MAX] = {
	NULL,
	mt_get_chip_hw_code,
	mt_get_chip_hw_subcode,
	mt_get_chip_hw_ver,
	(chip_info_cb) mt_get_chip_sw_ver,

	__chip_hw_code,
	__chip_hw_subcode,
	__chip_hw_ver,
	__chip_sw_ver,

	__chip_func_code,
	__chip_date_code,
	__chip_proj_code,
	__chip_fab_code,
	NULL,
};

unsigned int mt_get_chip_info(unsigned int id)
{
	if ((id <= CHIP_INFO_NONE) || (id >= CHIP_INFO_MAX))
		return 0;
	else if (NULL == g_cbs[id])
		return 0;
	return g_cbs[id] ();
}
EXPORT_SYMBOL(mt_get_chip_info);

int __init chip_mod_init(void)
{
	struct mt_chip_drv *p_drv = get_mt_chip_drv();

#ifdef CONFIG_OF
	struct device_node *node = of_find_compatible_node(NULL, NULL, "mediatek,chipid");

	if (node) {
		APHW_CODE = of_iomap(node, 0);
		WARN(!APHW_CODE, "unable to map APHW_CODE registers\n");
		APHW_SUBCODE = of_iomap(node, 1);
		WARN(!APHW_SUBCODE, "unable to map APHW_SUBCODE registers\n");
		APHW_VER = of_iomap(node, 2);
		WARN(!APHW_VER, "unable to map APHW_VER registers\n");
		APSW_VER = of_iomap(node, 3);
		WARN(!APSW_VER, "unable to map APSW_VER registers\n");
	}
#endif
	pr_debug("%p %p %p %p\n", APHW_CODE, APHW_SUBCODE, APHW_VER, APSW_VER);
	pr_debug("CODE = %04x %04x %04x %04x, %04x %04x %04x %04x, %04X %04X\n",
		 __chip_hw_code(), __chip_hw_subcode(), __chip_hw_ver(),
		 __chip_sw_ver(), __chip_func_code(), __chip_proj_code(),
		 __chip_date_code(), __chip_fab_code(), mt_get_chip_hw_ver(), mt_get_chip_sw_ver());

	p_drv->info_bit_mask |= CHIP_INFO_BIT(CHIP_INFO_HW_CODE) |
	    CHIP_INFO_BIT(CHIP_INFO_HW_SUBCODE) |
	    CHIP_INFO_BIT(CHIP_INFO_HW_VER) |
	    CHIP_INFO_BIT(CHIP_INFO_SW_VER) |
	    CHIP_INFO_BIT(CHIP_INFO_FUNCTION_CODE) |
	    CHIP_INFO_BIT(CHIP_INFO_DATE_CODE) |
	    CHIP_INFO_BIT(CHIP_INFO_PROJECT_CODE) | CHIP_INFO_BIT(CHIP_INFO_FAB_CODE);

	p_drv->get_chip_info = mt_get_chip_info;

	pr_debug("CODE = %08X %p", p_drv->info_bit_mask, p_drv->get_chip_info);

	return 0;
}

core_initcall(chip_mod_init);
MODULE_DESCRIPTION("MTK Chip Information");
MODULE_LICENSE("GPL");
