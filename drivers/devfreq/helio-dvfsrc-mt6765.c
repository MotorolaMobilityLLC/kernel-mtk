/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/string.h>

#include <mtk_dramc.h>
#include <mt-plat/upmu_common.h>

#include <helio-dvfsrc.h>
#include <mtk_dvfsrc_reg.h>
#include <mtk_spm_internal.h>

static struct reg_config dvfsrc_init_configs[][128] = {
	/* SPMFW_LP4X_2CH_3733 */
	{
		{ -1, 0 },
	},
	/* SPMFW_LP4X_2CH_3200 */
	{
		{ -1, 0 },
	},
	/* SPMFW_LP3_1CH_1866 */
	{
		{ DVFSRC_EMI_REQUEST,		0x00240009 },
		{ DVFSRC_EMI_REQUEST3,		0x09000000 },
		{ DVFSRC_EMI_HRT,		0x00000020 },
		{ DVFSRC_EMI_QOS0,		0x00000026 },
		{ DVFSRC_EMI_QOS1,		0x00000033 },
		{ DVFSRC_EMI_MD2SPM0,		0x0000003F },
		{ DVFSRC_EMI_MD2SPM1,		0x00000000 },
		{ DVFSRC_EMI_MD2SPM2,		0x000080C0 },
		{ DVFSRC_EMI_MD2SPM0_T,		0x00000007 },
		{ DVFSRC_EMI_MD2SPM1_T,		0x00000038 },
		{ DVFSRC_EMI_MD2SPM2_T,		0x000080C0 },

		{ DVFSRC_VCORE_REQUEST,		0x000E0000 },
		{ DVFSRC_VCORE_HRT,		0x00000020 },
		{ DVFSRC_VCORE_MD2SPM0,		0x0000003F },
		{ DVFSRC_VCORE_MD2SPM1,		0x000080C0 },
		{ DVFSRC_VCORE_MD2SPM2,		0x00000000 },
		{ DVFSRC_VCORE_MD2SPM0_T,	0x0000003F },
		{ DVFSRC_VCORE_MD2SPM1_T,	0x000080C0 },
		{ DVFSRC_VCORE_MD2SPM2_T,	0x00000000 },

		{ DVFSRC_MD_SW_CONTROL,		0x20000000 },

		{ DVFSRC_TIMEOUT_NEXTREQ,	0x00000014 },

		{ DVFSRC_LEVEL_LABEL_0_1,	0x00010000 },
		{ DVFSRC_LEVEL_LABEL_2_3,	0x00020101 },
		{ DVFSRC_LEVEL_LABEL_4_5,	0x01020012 },
		{ DVFSRC_LEVEL_LABEL_6_7,	0x02120112 },
		{ DVFSRC_LEVEL_LABEL_8_9,	0x00230013 },
		{ DVFSRC_LEVEL_LABEL_10_11,	0x01230113 },
		{ DVFSRC_LEVEL_LABEL_12_13,	0x02230213 },
		{ DVFSRC_LEVEL_LABEL_14_15,	0x03230323 },

		{ DVFSRC_FORCE,			0x02000000 },
		{ DVFSRC_RSRV_1,		0x0000000C },

		{ DVFSRC_QOS_EN,		0x0000407F },

		{ DVFSRC_BASIC_CONTROL,		0x0000C07A },

		{ DVFSRC_FORCE,			0x00000000 },
		{ DVFSRC_BASIC_CONTROL,		0x0000017A },

		{ -1, 0 },
	},
	/* NULL */
	{
		{ -1, 0 },
	},
};

struct reg_config *dvfsrc_get_init_conf(void)
{
	int spmfw_idx = spm_get_spmfw_idx();

	if (spmfw_idx < 0)
		spmfw_idx = ARRAY_SIZE(dvfsrc_init_configs) - 1;

	return dvfsrc_init_configs[spmfw_idx];
}

void get_opp_info(char *p)
{
	int pmic_val = pmic_get_register_value(PMIC_VCORE_ADDR);
	int vcore_uv = vcore_pmic_to_uv(pmic_val);
	int ddr_khz = get_dram_data_rate() * 1000;

	p += sprintf(p, "%-24s: %-8u uv  (PMIC: 0x%x)\n",
			"Vcore", vcore_uv, vcore_uv_to_pmic(vcore_uv));
	p += sprintf(p, "%-24s: %-8u khz\n", "DDR", ddr_khz);

}

void get_dvfsrc_reg(char *p)
{
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_BASIC_CONTROL",
			dvfsrc_read(DVFSRC_BASIC_CONTROL));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_LEVEL",
			dvfsrc_read(DVFSRC_LEVEL));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_SW_REQ",
			dvfsrc_read(DVFSRC_SW_REQ));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_SW_REQ2",
			dvfsrc_read(DVFSRC_SW_REQ2));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_SEC_SW_REQ",
			dvfsrc_read(DVFSRC_SEC_SW_REQ));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_VCORE_REQUEST",
			dvfsrc_read(DVFSRC_VCORE_REQUEST));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_VCORE_REQUEST2",
			dvfsrc_read(DVFSRC_VCORE_REQUEST2));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_EMI_REQUEST",
			dvfsrc_read(DVFSRC_EMI_REQUEST));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_EMI_REQUEST2",
			dvfsrc_read(DVFSRC_EMI_REQUEST2));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_EMI_REQUEST3",
			dvfsrc_read(DVFSRC_EMI_REQUEST3));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_MD_REQUEST",
			dvfsrc_read(DVFSRC_MD_REQUEST));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_INT",
			dvfsrc_read(DVFSRC_INT));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_FORCE",
			dvfsrc_read(DVFSRC_FORCE));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_RECORD_COUNT",
			dvfsrc_read(DVFSRC_RECORD_COUNT));
	p += sprintf(p, "%-24s: 0x%x\n",
			"DVFSRC_LAST",
			dvfsrc_read(DVFSRC_LAST));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_RECORD_0_1~3_1",
			dvfsrc_read(DVFSRC_RECORD_0_1),
			dvfsrc_read(DVFSRC_RECORD_1_1),
			dvfsrc_read(DVFSRC_RECORD_2_1),
			dvfsrc_read(DVFSRC_RECORD_3_1));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_RECORD_4_1~7_1",
			dvfsrc_read(DVFSRC_RECORD_4_1),
			dvfsrc_read(DVFSRC_RECORD_5_1),
			dvfsrc_read(DVFSRC_RECORD_6_1),
			dvfsrc_read(DVFSRC_RECORD_7_1));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_RECORD_0_0~3_0",
			dvfsrc_read(DVFSRC_RECORD_0_0),
			dvfsrc_read(DVFSRC_RECORD_1_0),
			dvfsrc_read(DVFSRC_RECORD_2_0),
			dvfsrc_read(DVFSRC_RECORD_3_0));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_RECORD_4_0~7_0",
			dvfsrc_read(DVFSRC_RECORD_4_0),
			dvfsrc_read(DVFSRC_RECORD_5_0),
			dvfsrc_read(DVFSRC_RECORD_6_0),
			dvfsrc_read(DVFSRC_RECORD_7_0));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_RECORD_MD_0~3",
			dvfsrc_read(DVFSRC_RECORD_MD_0),
			dvfsrc_read(DVFSRC_RECORD_MD_1),
			dvfsrc_read(DVFSRC_RECORD_MD_2),
			dvfsrc_read(DVFSRC_RECORD_MD_3));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_RECORD_MD_4~7",
			dvfsrc_read(DVFSRC_RECORD_MD_4),
			dvfsrc_read(DVFSRC_RECORD_MD_5),
			dvfsrc_read(DVFSRC_RECORD_MD_6),
			dvfsrc_read(DVFSRC_RECORD_MD_7));
}

