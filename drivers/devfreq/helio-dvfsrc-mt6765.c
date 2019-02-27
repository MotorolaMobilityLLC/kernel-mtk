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
		{ DVFSRC_LEVEL_LABEL_0_1, 0x00010000 },
		{ DVFSRC_LEVEL_LABEL_2_3, 0x02010101 },
		{ DVFSRC_LEVEL_LABEL_4_5, 0x00120002 },
		{ DVFSRC_LEVEL_LABEL_6_7, 0x01020022 },
		{ DVFSRC_LEVEL_LABEL_8_9, 0x01220112 },
		{ DVFSRC_LEVEL_LABEL_10_11, 0x02220212 },
		{ DVFSRC_LEVEL_LABEL_12_13, 0x01230023 },
		{ DVFSRC_LEVEL_LABEL_14_15, 0x03230223 },

		{ DVFSRC_TIMEOUT_NEXTREQ, 0x00000014 },

		{ DVFSRC_RSRV_1, 0x0000000C },

		{ DVFSRC_SW_REQ, 0x0000000E },

/*
 *                 { DVFSRC_EMI_QOS0, 0x26 },
 *                 { DVFSRC_EMI_QOS1, 0x32 },
 *                 { DVFSRC_EMI_MD2SPM0, 0x38 },
 *                 { DVFSRC_EMI_MD2SPM1, 0x80C0 },
 *                 { DVFSRC_VCORE_MD2SPM0, 0x80C0 },
 *                 { DVFSRC_INT_EN, 0x2 },
 *                 { DVFSRC_EMI_REQUEST, 0x00290209 },
 *                 { DVFSRC_EMI_REQUEST2, 0 },
 *                 { DVFSRC_EMI_REQUEST3, 0x09000000 },
 *                 { DVFSRC_VCORE_REQUEST, 0x00150000 },
 */

		{ DVFSRC_QOS_EN, 0x0000407F },

		{ DVFSRC_BASIC_CONTROL, 0x0000C07A },
		{ DVFSRC_BASIC_CONTROL, 0x0000017A },

		{ -1, 0 },
	},
	/* SPMFW_LP3_1CH_1866 */
	{
		{ DVFSRC_LEVEL_LABEL_0_1, 0x00010000 },
		{ DVFSRC_LEVEL_LABEL_2_3, 0x00020101 },
		{ DVFSRC_LEVEL_LABEL_4_5, 0x01020012 },
		{ DVFSRC_LEVEL_LABEL_6_7, 0x02120112 },
		{ DVFSRC_LEVEL_LABEL_8_9, 0x00230013 },
		{ DVFSRC_LEVEL_LABEL_10_11, 0x01230113 },
		{ DVFSRC_LEVEL_LABEL_12_13, 0x02230213 },
		{ DVFSRC_LEVEL_LABEL_14_15, 0x03230323 },

		{ DVFSRC_TIMEOUT_NEXTREQ, 0x00000014 },

		{ DVFSRC_RSRV_1, 0x0000000C },

		{ DVFSRC_SW_REQ, 0x0000000E },

/*
 *                 { DVFSRC_EMI_QOS0, 0x26 },
 *                 { DVFSRC_EMI_QOS1, 0x32 },
 *                 { DVFSRC_EMI_MD2SPM0, 0x38 },
 *                 { DVFSRC_EMI_MD2SPM1, 0x80C0 },
 *                 { DVFSRC_VCORE_MD2SPM0, 0x80C0 },
 *                 { DVFSRC_INT_EN, 0x2 },
 *                 { DVFSRC_EMI_REQUEST, 0x00290209 },
 *                 { DVFSRC_EMI_REQUEST2, 0 },
 *                 { DVFSRC_EMI_REQUEST3, 0x09000000 },
 *                 { DVFSRC_VCORE_REQUEST, 0x00150000 },
 */

		{ DVFSRC_QOS_EN, 0x0000407F },

		{ DVFSRC_BASIC_CONTROL, 0x0000C07A },
		{ DVFSRC_BASIC_CONTROL, 0x0000017A },

		{ -1, 0 },
	},
	/* NULL */
	{
		{ -1, 0 },
	},
};

struct reg_config *dvfsrc_get_init_conf(void)
{
/* ToDo: fix right dram type */
	int dram_type = spm_get_spmfw_idx();

	if (dram_type < 0)
		dram_type = ARRAY_SIZE(dvfsrc_init_configs) - 1;

	return dvfsrc_init_configs[dram_type];
}
