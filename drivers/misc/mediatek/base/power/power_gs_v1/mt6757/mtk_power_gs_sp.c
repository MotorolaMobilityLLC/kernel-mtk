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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include "mtk_power_gs.h"

void mt_power_gs_sp_dump(void)
{
	mt_power_gs_pmic_manual_dump();

	/* check pwr status */
	pr_debug("PWR_STATUS check:0x%08x\n",
		 _golden_read_reg(0x10006180)); /* PWR_STATUS */
	pr_debug("PWR_STATUS_2ND check:0x%08x\n",
		 _golden_read_reg(0x10006184)); /*PWR_STATUS_2ND */
	/* check ABB */
	pr_debug("0x2616B200 CHECK ABB 0x3 = 0x%08x\n",
		 _golden_read_reg(0x2616B200));
	pr_debug("0x2616B450 CHECK ABB 0x0 = 0x%08x\n",
		 _golden_read_reg(0x2616B450));
	/* pdn sram */
	pr_debug("0x10006370 = 0x%08x\n", _golden_read_reg(0x10006370));
	pr_debug("0x10006374 = 0x%08x\n", _golden_read_reg(0x10006374));
	pr_debug("0x1000635C = 0x%08x\n", _golden_read_reg(0x1000635C));
	/* BC11_SW_EN = 0   : 0x18[23] */
	/* OTG_VBUSCMP_EN = 0 0x18[20] */
	/* force_suspendm = 1   0x68[18]] */
	/* RG_SUSPENDM = 0 0x68[3] */
	pr_debug("0x11290818 USB20 = 0x%08x\n", _golden_read_reg(0x11290818));
	pr_debug("0x11290868 USB20 = 0x%08x\n", _golden_read_reg(0x11290868));
	pr_debug("0x11210818 USB20 = 0x%08x\n", _golden_read_reg(0x11210818));
	pr_debug("0x11210868 USB20 = 0x%08x\n", _golden_read_reg(0x11210868));
	/* SSUSB */
	pr_debug("0x11280000 SSUSB 0x1 = 0x%08x\n",
		 _golden_read_reg(0x11280000));
	/* ULPOSC */
	pr_debug("0x10006458 CHECK ULPOSC = 0x%08x\n",
		 _golden_read_reg(0x10006458));
	/* MEMDCM */
	pr_debug("0x10001078 MEM_DCM %x\n", _golden_read_reg(0x10001078));

	pr_debug("0x1000C000 USB %x\n", _golden_read_reg(0x1000C000));

	pr_debug("0x10001078 MEM_DCM %x\n", _golden_read_reg(0x10001078));

	pr_debug("0x11290868 USB20 = 0x%08x\n", _golden_read_reg(0x11290868));
	pr_debug("0x11280000 SSUSB = 0x%08x\n", _golden_read_reg(0x11280000));

	pr_debug("0x1000C000 AP_PLL_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000C000));
	pr_debug("0x1000c00c AP_PLL_CON3 = 0x%08x\n",
		 _golden_read_reg(0x1000c00c));
	pr_debug("0x1000c010 AP_PLL_CON4 = 0x%08x\n",
		 _golden_read_reg(0x1000c010));

	pr_debug("0x10001090 SW_CFG_0 = 0x%08x\n",
		 _golden_read_reg(0x10001090));
	pr_debug("0x10001094 SW_CFG_1 = 0x%08x\n",
		 _golden_read_reg(0x10001094));
	pr_debug("0x100010AC SW_CFG_2 = 0x%08x\n",
		 _golden_read_reg(0x100010AC));

	pr_debug("0x10000000 CLK_MODE = 0x%08x\n",
		 _golden_read_reg(0x10000000));

	pr_debug("0x10000040 CLK_CFG0 = 0x%08x\n",
		 _golden_read_reg(0x10000040));
	pr_debug("0x10000050 CLK_CFG1 = 0x%08x\n",
		 _golden_read_reg(0x10000050));
	pr_debug("0x10000060 CLK_CFG2 = 0x%08x\n",
		 _golden_read_reg(0x10000060));
	pr_debug("0x10000070 CLK_CFG3 = 0x%08x\n",
		 _golden_read_reg(0x10000070));
	pr_debug("0x10000080 CLK_CFG4 = 0x%08x\n",
		 _golden_read_reg(0x10000080));
	pr_debug("0x10000090 CLK_CFG5 = 0x%08x\n",
		 _golden_read_reg(0x10000090));
	pr_debug("0x100000a0 CLK_CFG6 = 0x%08x\n",
		 _golden_read_reg(0x100000A0));
	pr_debug("0x100000b0 CLK_CFG7 = 0x%08x\n",
		 _golden_read_reg(0x100000B0));
	pr_debug("0x100000C0 CLK_CFG8 = 0x%08x\n",
		 _golden_read_reg(0x100000C0));
	pr_debug("0x100000D0 CLK_CFG9 = 0x%08x\n",
		 _golden_read_reg(0x100000D0));

	pr_debug("0x1000c200 CLK_SCP_CFG_0 = 0x%08x\n",
		 _golden_read_reg(0x1000c200));
	pr_debug("0x1000c204 CLK_SCP_CFG_1 = 0x%08x\n",
		 _golden_read_reg(0x1000c204));
	pr_debug("0x1000c230 UNIVPLL_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c230));
	pr_debug("0x1000c23C UNIVPLL_PWR_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c23C));

	pr_debug("0x1000c240 MM_CON0 = 0x%08x\n", _golden_read_reg(0x1000c240));
	pr_debug("0x1000c24C MM_PWR_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c24C));

	pr_debug("0x1000c250 MSDCPLL_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c250));
	pr_debug("0x1000c25C MSDCPLL_PWR_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c25C));

	pr_debug("0x1000c260 VENC_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c260));
	pr_debug("0x1000c26C VENC_PWR_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c26C));

	pr_debug("0x1000c270 TVD_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c270));
	pr_debug("0x1000c27C TVD_PWR_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c27C));

	pr_debug("0x1000c2A0 APLL1_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c2A0));
	pr_debug("0x1000c2B0 APLL1_PWR_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c2B0));

	pr_debug("0x1000c2B4 APLL2_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c2B4));
	pr_debug("0x1000c2C4 APLL2_PWR_CON0 = 0x%08x\n",
		 _golden_read_reg(0x1000c2C4));

	pr_debug("0x1000c2C8 MD_CON0 = 0x%08x\n", _golden_read_reg(0x1000c2C8));

	pr_debug("0x10215080 MIPI 0x0 = 0x%08x\n",
		 _golden_read_reg(0x10215080));
	pr_debug("0x10215084 MIPI 0x0 = 0x%08x\n",
		 _golden_read_reg(0x10215084));
	pr_debug("0x10215088 MIPI 0x0 = 0x%08x\n",
		 _golden_read_reg(0x10215088));
	pr_debug("0x1021508C MIPI 0x0 = 0x%08x\n",
		 _golden_read_reg(0x1021508C));
	pr_debug("0x10215068 MIPI 0x0 = 0x%08x\n",
		 _golden_read_reg(0x1021508C));

	pr_debug("0x10006300 = 0x%08x\n", _golden_read_reg(0x10006300));
	pr_debug("0x10006304 = 0x%08x\n", _golden_read_reg(0x10006304));
	pr_debug("0x10006308 = 0x%08x\n", _golden_read_reg(0x10006308));
	pr_debug("0x1000630c = 0x%08x\n", _golden_read_reg(0x1000630c));
	pr_debug("0x10006314 = 0x%08x\n", _golden_read_reg(0x10006314));
	pr_debug("0x10006320 = 0x%08x\n", _golden_read_reg(0x10006320));
	pr_debug("0x10006328 = 0x%08x\n", _golden_read_reg(0x10006328));
	pr_debug("0x1000632c = 0x%08x\n", _golden_read_reg(0x1000632c));
	pr_debug("0x10006330 = 0x%08x\n", _golden_read_reg(0x10006330));
	pr_debug("0x10006334 = 0x%08x\n", _golden_read_reg(0x10006334));
	pr_debug("0x10006344 = 0x%08x\n", _golden_read_reg(0x10006344));
	pr_debug("0x10006350 = 0x%08x\n", _golden_read_reg(0x10006350));
	pr_debug("0x10006354 = 0x%08x\n", _golden_read_reg(0x10006354));
	pr_debug("0x1000635c = 0x%08x\n", _golden_read_reg(0x1000635c));
	pr_debug("0x10006360 = 0x%08x\n", _golden_read_reg(0x10006360));
	pr_debug("0x10006370 = 0x%08x\n", _golden_read_reg(0x10006370));
	pr_debug("0x10006374 = 0x%08x\n", _golden_read_reg(0x10006374));
	pr_debug("0x10006390 = 0x%08x\n", _golden_read_reg(0x10006390));

	pr_debug("0x10228284 = 0x%08x\n", _golden_read_reg(0x10228284));
	pr_debug("0x10230284 = 0x%08x\n", _golden_read_reg(0x10230284));
	pr_debug("0x1022828C = 0x%08x\n", _golden_read_reg(0x1022828C));
	pr_debug("0x1023028C = 0x%08x\n", _golden_read_reg(0x1023028C));
}
