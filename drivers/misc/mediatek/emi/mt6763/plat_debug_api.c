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

#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#include <mt-plat/mtk_ram_console.h>
#endif

#include "mt_emi.h"

static void __iomem *CEN_EMI_BASE;
static void __iomem *CHA_EMI_BASE;
static void __iomem *CHB_EMI_BASE;
static void __iomem *INFRACFG_BASE;
static void __iomem *INFRA_AO_BASE;
static void __iomem *PERICFG_BASE;

void plat_debug_api_init(void)
{
	struct device_node *node;

	CEN_EMI_BASE = mt_cen_emi_base_get();
	CHA_EMI_BASE = mt_chn_emi_base_get(0);
	CHB_EMI_BASE = mt_chn_emi_base_get(1);

	if (INFRACFG_BASE == NULL) {
		node = of_find_compatible_node(NULL, NULL,
			"mediatek,infracfg");
		if (node) {
			INFRACFG_BASE = of_iomap(node, 0);
			pr_info("[EMI DBG] INFRACFG_BASE@ %p\n",
				INFRACFG_BASE);
		} else
			pr_err("[EMI DBG] no node for INFRACFG_BASE\n");
	}

	if (INFRA_AO_BASE == NULL) {
		node = of_find_compatible_node(NULL, NULL,
			"mediatek,infracfg_ao");
		if (node) {
			INFRA_AO_BASE = of_iomap(node, 0);
			pr_info("[EMI DBG] INFRA_AO_BASE@ %p\n",
				INFRA_AO_BASE);
		} else
			pr_err("[EMI DBG] no node for INFRA_AO_BASE\n");
	}

	if (PERICFG_BASE == NULL) {
		node = of_find_compatible_node(NULL, NULL,
			"mediatek,pericfg");
		if (node) {
			PERICFG_BASE = of_iomap(node, 0);
			pr_info("[EMI DBG] PERICFG_BASE@ %p\n",
				PERICFG_BASE);
		} else
			pr_err("[EMI DBG] no node for PERICFG_BASE\n");
	}
}

static inline void aee_simple_print(const char *msg, unsigned int val)
{
	char buf[128];

	snprintf(buf, sizeof(buf), msg, val);
#ifdef CONFIG_MTK_AEE_FEATURE
	aee_sram_fiq_log(buf);
#endif
}

#define EMI_DBG_SIMPLE_RWR(msg, addr, wval)	do {\
	aee_simple_print(msg, readl(addr));	\
	writel(wval, addr);			\
	aee_simple_print(msg, readl(addr));\
	} while (0)

#define EMI_DBG_SIMPLE_R(msg, addr)		\
	aee_simple_print(msg, readl(addr))

void dump_emi_outstanding(void)
{
#if 0
	/* CEN_EMI_BASE: 0x10219000 */
	if (!CEN_EMI_BASE)
		return;

	/* CHA_EMI_BASE: 0x1022D000 */
	if (!CHA_EMI_BASE)
		return;

	/* CHB_EMI_BASE: 0x10235000 */
	if (!CHB_EMI_BASE)
		return;
#endif

	/* INFRACFG_BASE: 0x10001000 */
	if (!INFRA_AO_BASE)
		return;

	/* INFRACFG_BASE: 0x1020E000 */
	if (!INFRA_AO_BASE)
		return;

	/* INFRACFG_BASE: 0x1020E000 */
	if (!INFRACFG_BASE)
		return;

	/* PERICFG_BASE: 0x10003000 */
	if (!PERICFG_BASE)
		return;

	EMI_DBG_SIMPLE_R("[EMI] 0x10001220 = 0x%x\n",
		(INFRA_AO_BASE + 0x220));
	EMI_DBG_SIMPLE_R("[EMI] 0x10001224 = 0x%x\n",
		(INFRA_AO_BASE + 0x224));
	EMI_DBG_SIMPLE_R("[EMI] 0x10001228 = 0x%x\n",
		(INFRA_AO_BASE + 0x228));
	EMI_DBG_SIMPLE_R("[EMI] 0x10001250 = 0x%x\n",
		(INFRA_AO_BASE + 0x250));
	EMI_DBG_SIMPLE_R("[EMI] 0x10001254 = 0x%x\n",
		(INFRA_AO_BASE + 0x254));
	EMI_DBG_SIMPLE_R("[EMI] 0x10001258 = 0x%x\n",
		(INFRA_AO_BASE + 0x258));
	EMI_DBG_SIMPLE_R("[EMI] 0x10001284 = 0x%x\n",
		(INFRA_AO_BASE + 0x284));
	EMI_DBG_SIMPLE_R("[EMI] 0x10001d04 = 0x%x\n",
		(INFRA_AO_BASE + 0xd04));

	EMI_DBG_SIMPLE_R("[EMI] 0x10003500 = 0x%x\n",
		(PERICFG_BASE + 0x500));
	EMI_DBG_SIMPLE_R("[EMI] 0x10003504 = 0x%x\n",
		(PERICFG_BASE + 0x504));

	EMI_DBG_SIMPLE_R("[EMI] 0x1020e000 = 0x%x\n",
		(INFRACFG_BASE + 0x000));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e004 = 0x%x\n",
		(INFRACFG_BASE + 0x004));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e008 = 0x%x\n",
		(INFRACFG_BASE + 0x008));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e028 = 0x%x\n",
		(INFRACFG_BASE + 0x028));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e180 = 0x%x\n",
		(INFRACFG_BASE + 0x180));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e184 = 0x%x\n",
		(INFRACFG_BASE + 0x184));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e188 = 0x%x\n",
		(INFRACFG_BASE + 0x188));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e18c = 0x%x\n",
		(INFRACFG_BASE + 0x028));
	EMI_DBG_SIMPLE_R("[EMI] 0x1020e190 = 0x%x\n",
		(INFRACFG_BASE + 0x190));

#if 0
	EMI_DBG_SIMPLE_RWR("[EMI] 0x102190e8 = 0x%x\n",
		(CEN_EMI_BASE + 0x0e8), readl(CEN_EMI_BASE + 0x0e8) | 0x100);

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e100 = 0x%x\n",
		(INFRACFG_BASE + 0x100), 0x00000104 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e104 = 0x%x\n",
		(INFRACFG_BASE + 0x104), 0x00000204 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e108 = 0x%x\n",
		(INFRACFG_BASE + 0x108), 0x00000304 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e10c = 0x%x\n",
		(INFRACFG_BASE + 0x10c), 0x00000404 >> 3);
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e100 = 0x%x\n",
		(INFRACFG_BASE + 0x100), 0x00000804 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e104 = 0x%x\n",
		(INFRACFG_BASE + 0x104), 0x00000904 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e108 = 0x%x\n",
		(INFRACFG_BASE + 0x108), 0x00001204 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e10c = 0x%x\n",
		(INFRACFG_BASE + 0x10c), 0x00001104 >> 3);
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e100 = 0x%x\n",
		(INFRACFG_BASE + 0x100), 0x00001504 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e104 = 0x%x\n",
		(INFRACFG_BASE + 0x104), 0x00001604 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e108 = 0x%x\n",
		(INFRACFG_BASE + 0x108), 0x00001704 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e10c = 0x%x\n",
		(INFRACFG_BASE + 0x10c), 0x00001804 >> 3);
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e100 = 0x%x\n",
		(INFRACFG_BASE + 0x100), 0x00001904 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e104 = 0x%x\n",
		(INFRACFG_BASE + 0x104), 0x00001a04 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e108 = 0x%x\n",
		(INFRACFG_BASE + 0x108), 0x00001b04 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e10c = 0x%x\n",
		(INFRACFG_BASE + 0x10c), 0x00001c04 >> 3);
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e100 = 0x%x\n",
		(INFRACFG_BASE + 0x100), 0x00003600 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e104 = 0x%x\n",
		(INFRACFG_BASE + 0x104), 0x00003700 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e108 = 0x%x\n",
		(INFRACFG_BASE + 0x108), 0x00003800 >> 3);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1020e10c = 0x%x\n",
		(INFRACFG_BASE + 0x10c), 0x00003900 >> 3);
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));
	EMI_DBG_SIMPLE_R("[EMI] 0x102197fc = 0x%x\n",
		(CEN_EMI_BASE + 0x7fc));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da80 = 0x%x\n",
		(CHA_EMI_BASE + 0xa80), 0x00000001);

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da88 = 0x%x\n",
		(CHA_EMI_BASE + 0xa88), 0x00120000);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da8c = 0x%x\n",
		(CHA_EMI_BASE + 0xa8c), 0x00180017);
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da88 = 0x%x\n",
		(CHA_EMI_BASE + 0xa88), 0x00160015);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da8c = 0x%x\n",
		(CHA_EMI_BASE + 0xa8c), 0x00180017);
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da88 = 0x%x\n",
		(CHA_EMI_BASE + 0xa88), 0x001a0019);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da8c = 0x%x\n",
		(CHA_EMI_BASE + 0xa8c), 0x0001001b);
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da88 = 0x%x\n",
		(CHA_EMI_BASE + 0xa88), 0x00370036);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da8c = 0x%x\n",
		(CHA_EMI_BASE + 0xa8c), 0x00390038);
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da88 = 0x%x\n",
		(CHA_EMI_BASE + 0xa88), 0x00050004);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da8c = 0x%x\n",
		(CHA_EMI_BASE + 0xa8c), 0x00070006);
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da88 = 0x%x\n",
		(CHA_EMI_BASE + 0xa88), 0x000e0008);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da8c = 0x%x\n",
		(CHA_EMI_BASE + 0xa8c), 0x0010000f);
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da88 = 0x%x\n",
		(CHA_EMI_BASE + 0xa88), 0x000a0009);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1022da8c = 0x%x\n",
		(CHA_EMI_BASE + 0xa8c), 0x000c000b);
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x1022da84 = 0x%x\n",
		(CHA_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a80 = 0x%x\n",
		(CHB_EMI_BASE + 0xa80), 0x00000001);

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a88 = 0x%x\n",
		(CHB_EMI_BASE + 0xa88), 0x00120000);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a8c = 0x%x\n",
		(CHB_EMI_BASE + 0xa8c), 0x00180017);
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a88 = 0x%x\n",
		(CHB_EMI_BASE + 0xa88), 0x00160015);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a8c = 0x%x\n",
		(CHB_EMI_BASE + 0xa8c), 0x00180017);
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a88 = 0x%x\n",
		(CHB_EMI_BASE + 0xa88), 0x001a0019);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a8c = 0x%x\n",
		(CHB_EMI_BASE + 0xa8c), 0x0001001b);
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a88 = 0x%x\n",
		(CHB_EMI_BASE + 0xa88), 0x00370036);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a8c = 0x%x\n",
		(CHB_EMI_BASE + 0xa8c), 0x00390038);
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a88 = 0x%x\n",
		(CHB_EMI_BASE + 0xa88), 0x00050004);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a8c = 0x%x\n",
		(CHB_EMI_BASE + 0xa8c), 0x00070006);
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a88 = 0x%x\n",
		(CHB_EMI_BASE + 0xa88), 0x000e0008);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a8c = 0x%x\n",
		(CHB_EMI_BASE + 0xa8c), 0x0010000f);
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));

	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a88 = 0x%x\n",
		(CHB_EMI_BASE + 0xa88), 0x000a0009);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10235a8c = 0x%x\n",
		(CHB_EMI_BASE + 0xa8c), 0x000c000b);
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
	EMI_DBG_SIMPLE_R("[EMI] 0x10235a84 = 0x%x\n",
		(CHB_EMI_BASE + 0xa84));
#endif
}
