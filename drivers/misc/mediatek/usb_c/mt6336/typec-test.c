/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "mtk_typec.h"
#include "typec-test.h"
#include "typec-ioctl.h"

#if FPGA_PLATFORM

/*function prototypes (local)*/

static int ts_sleep(struct file *file, int argc, char **argv);

static int ts_dbg(struct file *file, int argc, char **argv);
static int ts_stat(struct file *file, int argc, char **argv);

static int ts_regw(struct file *file, int argc, char **argv);
static int ts_regr(struct file *file, int argc, char **argv);
static int ts_dump(struct file *file, int argc, char **argv);

static int ts_sw_rst(struct file *file, int argc, char **argv);

static int ts_enable(struct file *file, int argc, char **argv);
static int ts_mode(struct file *file, int argc, char **argv);
static int ts_rp(struct file *file, int argc, char **argv);

CMD_TBL_T _arPCmdTbl[] = {
	/*Misc*/
	{"typec.sleep", &ts_sleep},
	/*Debug*/
	{"typec.dbg", &ts_dbg},
	{"typec.stat", &ts_stat},
	/*Register*/
	{"typec.regw", &ts_regw},
	{"typec.regr", &ts_regr},
	{"typec.dump", &ts_dump},
	/*Reset*/
	{"typec.sw_rst", &ts_sw_rst},
	/*Control*/
	{"typec.enable", &ts_enable},
	{"typec.mode", &ts_mode},
	{"typec.rp", &ts_rp},
	#if SUPPORT_PD
	/*PD*/
	{"typec.pd", &ts_pd},
	#endif
	{NULL, NULL}
};

/*
 * put test code in kernel driver, for some considerations
 *
 * 1. no extra latency for ioctl calls. this is critical for performance measurement
 * 2. its easier to use ICE to debug kernel, instead of apps
 */
int call_function(struct file *file, char *buf)
{
	int i;
	int argc;
	char *argv[MAX_ARG_SIZE];
	struct typec_hba *hba = file->private_data;

	argc = 0;
	do {
		argv[argc] = strsep(&buf, " ");
		dev_err(hba->dev, "%d: %s\r\n", argc, argv[argc]);
		argc++;
	} while (buf);

	for (i = 0; i < sizeof(_arPCmdTbl)/sizeof(CMD_TBL_T); i++) {
		if ((!strcmp(_arPCmdTbl[i].name, argv[0])) && (_arPCmdTbl[i].cb_func != NULL))
			return _arPCmdTbl[i].cb_func(file, argc, argv);
	}

	return -EINVAL;
}

static int ts_sleep(struct file *file, int argc, char **argv)
{
	uint32_t value;
	struct typec_hba *hba = file->private_data;

	/*arguments check*/
	if (argc > 1) {
		/*value = simple_strtol(argv[1], &argv[1], 10);*/
		if (kstrtouint(argv[1], 0, &value) != 0)
			return -EINVAL;
	} else {
		dev_err(hba->dev, "function: sleep (unit: ms)\n");
		dev_err(hba->dev, "usage: %s [value]\n", argv[0]);

		return -EINVAL;
	}

	/*function body*/
	mdelay(value);

	return 0;
}

static int ts_dbg(struct file *file, int argc, char **argv)
{
	enum enum_typec_dbg_lvl lvl;
	struct typec_hba *hba = file->private_data;

	/*arguments check*/
	if (argc > 1) {
		/*lvl = simple_strtol(argv[1], &argv[1], 10);*/
		if (kstrtouint(argv[1], 0, &lvl) != 0)
			return -EINVAL;
	} else {
		dev_err(hba->dev, "function: set debug message verbose level\n");
		dev_err(hba->dev, "usage: %s [lvl]\n", argv[0]);
		dev_err(hba->dev, "lvl: debug message verbose level\n");
		dev_err(hba->dev, "%d: nothing\n", TYPEC_DBG_LVL_0);
		dev_err(hba->dev, "%d: important interrupt dump\n", TYPEC_DBG_LVL_1);
		dev_err(hba->dev, "%d: full interrupt dump\n", TYPEC_DBG_LVL_2);
		dev_err(hba->dev, "%d: function call dump\n", TYPEC_DBG_LVL_3);
		dev_err(hba->dev, "\n");
		dev_err(hba->dev, "current lvl=%d\n", hba->dbg_lvl);

		return -EINVAL;
	}

	/*function body*/
	hba->dbg_lvl = lvl;
	dev_err(hba->dev, "set lvl to %d\n", hba->dbg_lvl);

	return 0;
}

static int ts_stat(struct file *file, int argc, char **argv)
{
	int i;
	struct typec_hba *hba = file->private_data;
	struct reg_mapping dump[] = {
		{TYPE_C_CC_STATUS, RO_TYPE_C_CC_ST, RO_TYPE_C_CC_ST_OFST, "cc_st"},
		{TYPE_C_CC_STATUS, RO_TYPE_C_ROUTED_CC, RO_TYPE_C_ROUTED_CC_OFST, "routed_cc"},
		{TYPE_C_PWR_STATUS, RO_TYPE_C_CC_SNK_PWR_ST, RO_TYPE_C_CC_SNK_PWR_ST_OFST, "cc_snk_pwr_st"},
		{TYPE_C_PWR_STATUS, RO_TYPE_C_CC_PWR_ROLE, RO_TYPE_C_CC_PWR_ROLE_OFST, "cc_pwr_role"},
	};

	for (i = 0; i < sizeof(dump) / sizeof(struct reg_mapping); i++) {
		dev_err(hba->dev, "%s: %x\n", dump[i].name,
			((typec_readw(hba, dump[i].addr) & dump[i].mask) >> dump[i].ofst));
	}

	return 0;
}

static int ts_regw(struct file *file, int argc, char **argv)
{
	uint32_t addr, value;
	struct typec_hba *hba = file->private_data;

	/*arguments check*/
	if (argc > 2) {
		if (kstrtouint(argv[1], 0, &addr) != 0)
			return -EINVAL;

		if (kstrtouint(argv[2], 0, &value) != 0)
			return -EINVAL;
	} else {
		dev_err(hba->dev, "function: write register\n");
		dev_err(hba->dev, "usage: %s [addr] [value]\n", argv[0]);

		return -EINVAL;
	}

	/*function body*/
	typec_writew(hba, value, addr);

	return 0;
}

static int ts_regr(struct file *file, int argc, char **argv)
{
	uint32_t addr, value;
	struct typec_hba *hba = file->private_data;

	/*arguments check*/
	if (argc > 1) {
		/*addr = simple_strtol(argv[1], &argv[1], 16);*/

		if (kstrtouint(argv[1], 0, &addr) != 0)
			return -EINVAL;

	} else {
		dev_err(hba->dev, "function: read register\n");
		dev_err(hba->dev, "usage: %s [addr]\n", argv[0]);

		return -EINVAL;
	}

	/*function body*/
	value = typec_readw(hba, addr);
	dev_err(hba->dev, "addr: 0x%x, value: 0x%x\n", addr, value);

	return 0;
}

static int ts_dump(struct file *file, int argc, char **argv)
{
	#if SUPPORT_PD
	#define MAX_REG 0x200
	#else
	#define MAX_REG 0x100
	#endif

	uint32_t addr;
	uint32_t val1, val2, val3, val4;
	struct typec_hba *hba = file->private_data;

	/*function body*/
	dev_err(hba->dev, "        03    00 07    04 0B    08 0F    0C\n");
	dev_err(hba->dev, "===========================================\n");
	for (addr = 0; addr <= MAX_REG; addr += 0x10) {
		uint32_t real_addr = addr + CC_REG_BASE;

		val1 = typec_readw(hba, real_addr+2)<<16 | typec_readw(hba, real_addr);
		val2 = typec_readw(hba, real_addr+6)<<16 | typec_readw(hba, real_addr+4);
		val3 = typec_readw(hba, real_addr+10)<<16 | typec_readw(hba, real_addr+8);
		val4 = typec_readw(hba, real_addr+14)<<16 | typec_readw(hba, real_addr+12);

		dev_err(hba->dev, "0x%4x  %8x %8x %8x %8x\n", addr, val1, val2, val3, val4);
	}

	return 0;
}

/*IP software reset*/
static int ts_sw_rst(struct file *file, int argc, char **argv)
{
#if 0
	uint32_t reg;

	/*function body*/
	/*bit 19: HCI reset, 20: unipro*/
	reg = readl(SW_RST);
	reg = reg | (1<<19 | 1<<20);
	writel(reg, SW_RST);

	reg = reg & ~(1<<19 | 1<<20);
	writel(reg, SW_RST);

	return 0;
#endif
}

static int ts_enable(struct file *file, int argc, char **argv)
{
	int enable;
	struct typec_hba *hba = file->private_data;

	/*arguments check*/
	if (argc > 1) {
		/*enable = simple_strtol(argv[1], &argv[1], 10);*/

		if (kstrtoint(argv[1], 0, &enable) != 0)
			return -EINVAL;

	} else {
		dev_err(hba->dev, "function: enable/disable TypeC HW\n");
		dev_err(hba->dev, "usage: %s [enable]\n", argv[0]);

		return -EINVAL;
	}

	/*function body*/
	if (typec_enable(hba, enable))
		return 1;

	return 0;
}

static int ts_mode(struct file *file, int argc, char **argv)
{
	int mode, param1, param2;
	struct typec_hba *hba = file->private_data;

	/*arguments check*/
	if (argc > 1) {
		/*mode = simple_strtol(argv[1], &argv[1], 10);*/

		if (kstrtoint(argv[1], 0, &mode) != 0)
			goto err_handle;

	} else {
		goto err_handle;
	}

	if (mode == TYPEC_ROLE_SOURCE || mode == TYPEC_ROLE_SINK_W_ACTIVE_CABLE) {
		if (argc > 2) {
			if (kstrtoint(argv[2], 0, &param1) != 0)
				goto err_handle;

		} else
			goto err_handle;
	} else if (mode == TYPEC_ROLE_DRP) {
		if (argc > 3) {
			if (kstrtoint(argv[2], 0, &param1) != 0)
				goto err_handle;

			if (kstrtoint(argv[3], 0, &param2) != 0)
				goto err_handle;


		} else {
			goto err_handle;
		}
	}

	/*function body*/
	if (typec_set_mode(hba, mode, param1, param2))
		return 1;

	#if PD_DVT
	if ((mode == TYPEC_ROLE_DRP) && param2 == 1)
		hba->pd_comm_enabled = 0;
	else
		hba->pd_comm_enabled = 1;

	if (typec_enable(hba, 1))
		return 1;
	#endif

	return 0;

err_handle:
	dev_err(hba->dev, "function: set controller role and param\n");
	dev_err(hba->dev, "usage: %s [role] (param1) (param2)\n", argv[0]);
	dev_err(hba->dev, "role:\n");
	dev_err(hba->dev, "%d: %s\n", TYPEC_ROLE_SINK, "SINK");
	dev_err(hba->dev, "%d: %s\n", TYPEC_ROLE_SOURCE, "SOURCE");
	dev_err(hba->dev, "%d: %s\n", TYPEC_ROLE_DRP, "DRP");
	dev_err(hba->dev, "%d: %s\n", TYPEC_ROLE_SINK_W_ACTIVE_CABLE, "SINK with ACTIVE CABLE");
	dev_err(hba->dev, "%d: %s\n", TYPEC_ROLE_ACCESSORY_AUDIO, "AUDIO ACCESSORY");
	dev_err(hba->dev, "%d: %s\n", TYPEC_ROLE_ACCESSORY_DEBUG, "DEBUG ACCESSORY");
	dev_err(hba->dev, "%d: %s\n", TYPEC_ROLE_OPEN, "OPEN");

	dev_err(hba->dev, "[role %d, %d, %d, %d]\n",
		TYPEC_ROLE_SINK, TYPEC_ROLE_ACCESSORY_AUDIO, TYPEC_ROLE_ACCESSORY_DEBUG, TYPEC_ROLE_OPEN);
	dev_err(hba->dev, "don't care\n");

	dev_err(hba->dev, "[role %d]\n", TYPEC_ROLE_SOURCE);
	dev_err(hba->dev, "Rp:\n");
	dev_err(hba->dev, "%d: %s\n", TYPEC_RP_DFT, "DEFAULT - 36K ohm");
	dev_err(hba->dev, "%d: %s\n", TYPEC_RP_15A, "15A - 12K ohm");
	dev_err(hba->dev, "%d: %s\n", TYPEC_RP_30A, "30A - 4.7K ohm");

	dev_err(hba->dev, "[role %d]\n", TYPEC_ROLE_DRP);
	dev_err(hba->dev, "Rp:\n");
	dev_err(hba->dev, "%d: %s\n", TYPEC_RP_DFT, "DEFAULT - 36K ohm");
	dev_err(hba->dev, "%d: %s\n", TYPEC_RP_15A, "15A - 12K ohm");
	dev_err(hba->dev, "%d: %s\n", TYPEC_RP_30A, "30A - 4.7K ohm");
	dev_err(hba->dev, "try mode:\n");
	dev_err(hba->dev, "%d: %s\n", TYPEC_TRY_DISABLE, "Disable Try transitions");
	dev_err(hba->dev, "%d: %s\n", TYPEC_TRY_ENABLE, "Enable Try transition");

	dev_err(hba->dev, "[role %d]\n", TYPEC_ROLE_SINK_W_ACTIVE_CABLE);
	dev_err(hba->dev, "1: CC1=CC,CC2=Ra\n");
	dev_err(hba->dev, "2: CC1=Ra,CC2=CC\n");

	return -EINVAL;
}

static int ts_rp(struct file *file, int argc, char **argv)
{
	int rp_val;
	struct typec_hba *hba = file->private_data;

	/*arguments check*/
	if (argc > 1) {
		/*rp_val = simple_strtol(argv[1], &argv[1], 10);*/

		if (kstrtoint(argv[1], 0, &rp_val) != 0)
			return -EINVAL;

	} else {
		dev_err(hba->dev, "function: select Rp\n");
		dev_err(hba->dev, "usage: %s [Rp]\n", argv[0]);
		dev_err(hba->dev, "Rp:\n");
		dev_err(hba->dev, "%d: %s\n", TYPEC_RP_DFT, "DEFAULT - 36K ohm");
		dev_err(hba->dev, "%d: %s\n", TYPEC_RP_15A, "15A - 12K ohm");
		dev_err(hba->dev, "%d: %s\n", TYPEC_RP_30A, "30A - 4.7K ohm");

		return -EINVAL;
	}

	/*function body*/
	typec_select_rp(hba, rp_val);

	return 0;
}
#endif

