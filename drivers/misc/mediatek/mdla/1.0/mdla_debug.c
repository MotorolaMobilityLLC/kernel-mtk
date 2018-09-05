/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include "mdla.h"
#include "mdla_debug.h"
#include "mdla_hw_reg.h"

#define dump_reg_top(name) \
	mdla_debug("%s: %.8x\n", #name, mdla_reg_read(name))

#define dump_reg_cfg(name) \
	mdla_debug("%s: %.8x\n", #name, mdla_cfg_read(name))

void mdla_dump_reg(void)
{
	// TODO: too many registers, dump only debug required ones.
	dump_reg_cfg(MDLA_CG_CON);
	dump_reg_cfg(MDLA_SW_RST);
	dump_reg_cfg(MDLA_MBIST_MODE0);
	dump_reg_cfg(MDLA_MBIST_MODE1);
	dump_reg_cfg(MDLA_MBIST_CTL);
	dump_reg_cfg(MDLA_RP_OK0);
	dump_reg_cfg(MDLA_RP_OK1);
	dump_reg_cfg(MDLA_RP_OK2);
	dump_reg_cfg(MDLA_RP_OK3);
	dump_reg_cfg(MDLA_RP_FAIL0);
	dump_reg_cfg(MDLA_RP_FAIL1);
	dump_reg_cfg(MDLA_RP_FAIL2);
	dump_reg_cfg(MDLA_RP_FAIL3);
	dump_reg_cfg(MDLA_MBIST_FAIL0);
	dump_reg_cfg(MDLA_MBIST_FAIL1);
	dump_reg_cfg(MDLA_MBIST_FAIL2);
	dump_reg_cfg(MDLA_MBIST_FAIL3);
	dump_reg_cfg(MDLA_MBIST_FAIL4);
	dump_reg_cfg(MDLA_MBIST_FAIL5);
	dump_reg_cfg(MDLA_MBIST_DONE0);
	dump_reg_cfg(MDLA_MBIST_DONE1);
	dump_reg_cfg(MDLA_MBIST_DEFAULT_DELSEL);
	dump_reg_cfg(MDLA_SRAM_DELSEL0);
	dump_reg_cfg(MDLA_SRAM_DELSEL1);
	dump_reg_cfg(MDLA_RP_RST);
	dump_reg_cfg(MDLA_RP_CON);
	dump_reg_cfg(MDLA_RP_PRE_FUSE);

	dump_reg_top(MREG_TOP_G_REV);
	dump_reg_top(MREG_TOP_G_INTP0);
	dump_reg_top(MREG_TOP_G_INTP1);
	dump_reg_top(MREG_TOP_G_INTP2);
	dump_reg_top(MREG_TOP_G_CDMA0);
	dump_reg_top(MREG_TOP_G_CDMA1);
	dump_reg_top(MREG_TOP_G_CDMA2);
	dump_reg_top(MREG_TOP_G_CDMA3);
	dump_reg_top(MREG_TOP_G_CDMA4);
	dump_reg_top(MREG_TOP_G_CDMA5);
	dump_reg_top(MREG_TOP_G_CDMA6);
	dump_reg_top(MREG_TOP_G_CUR0);
	dump_reg_top(MREG_TOP_G_CUR1);
	dump_reg_top(MREG_TOP_G_FIN0);
	dump_reg_top(MREG_TOP_G_FIN1);
}

