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

#include <mt-plat/upmu_common.h>
#include <mt-plat/charging.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include "mtk_bif_intf.h"

#ifdef CONFIG_MTK_BIF_SUPPORT
static int bif_exist;
static int bif_checked;

/* BIF related functions*/
#define BC (0x400)
#define SDA (0x600)
#define ERA (0x100)
#define WRA (0x200)
#define RRA (0x300)
#define WD  (0x000)

/* Bus commands */
#define BUSRESET (0x00)
#define RBL2 (0x22)
#define RBL4 (0x24)

/* BIF slave address */
#define MW3790 (0x00)
#define MW3790_VBAT (0x0114)
#define MW3790_TBAT (0x0193)

static void bif_reset_irq(void)
{
	unsigned int reg_val = 0;
	unsigned int loop_i = 0;

	pmic_set_register_value(PMIC_BIF_IRQ_CLR, 1);
	reg_val = 0;
	do {
		reg_val = pmic_get_register_value(PMIC_BIF_IRQ);

		if (loop_i++ > 50) {
			battery_log(BAT_LOG_CRTI, "[BIF][reset irq]failed.PMIC_BIF_IRQ 0x%x %d\n",
				    reg_val, loop_i);
			break;
		}
	} while (reg_val != 0);
	pmic_set_register_value(PMIC_BIF_IRQ_CLR, 0);
}

static void bif_waitfor_slave(void)
{
	unsigned int reg_val = 0;
	int loop_i = 0;

	do {
		reg_val = pmic_get_register_value(PMIC_BIF_IRQ);

		if (loop_i++ > 50) {
			battery_log(BAT_LOG_FULL,
				    "[BIF][waitfor_slave] failed. PMIC_BIF_IRQ=0x%x, loop=%d\n",
				    reg_val, loop_i);
			break;
		}
	} while (reg_val == 0);

	if (reg_val == 1)
		battery_log(BAT_LOG_FULL, "[BIF][waitfor_slave]OK at loop=%d.\n", loop_i);

}

static int bif_powerup_slave(void)
{
	int bat_lost = 0;
	int total_valid = 0;
	int timeout = 0;
	int loop_i = 0;

	do {
		battery_log(BAT_LOG_CRTI, "[BIF][powerup_slave] set BIF power up register\n");
		pmic_set_register_value(PMIC_BIF_POWER_UP, 1);

		battery_log(BAT_LOG_FULL, "[BIF][powerup_slave] trigger BIF module\n");
		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 1);

		udelay(10);

		bif_waitfor_slave();

		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 0);

		pmic_set_register_value(PMIC_BIF_POWER_UP, 0);

		/* check_bat_lost(); what to do with this? */
		bat_lost = pmic_get_register_value(PMIC_BIF_BAT_LOST);
		total_valid = pmic_get_register_value(PMIC_BIF_TOTAL_VALID);
		timeout = pmic_get_register_value(PMIC_BIF_TIMEOUT);

		if (loop_i < 5) {
			loop_i++;
		} else {
			battery_log(BAT_LOG_CRTI, "[BIF][powerup_slave]Failed at loop=%d", loop_i);
			break;
		}
	} while (bat_lost == 1 || total_valid == 1 || timeout == 1);
	if (loop_i < 5) {
		battery_log(BAT_LOG_FULL, "[BIF][powerup_slave]OK at loop=%d", loop_i);
	bif_reset_irq();
		return 1;
	}

	return -1;
}

static void bif_set_cmd(int bif_cmd[], int bif_cmd_len)
{
	int i = 0;
	int con_index = 0;
	unsigned int ret = 0;

	for (i = 0; i < bif_cmd_len; i++) {
		ret = pmic_config_interface(MT6351_BIF_CON0 + con_index, bif_cmd[i], 0x07FF, 0);
		con_index += 0x2;
	}
}


static int bif_reset_slave(void)
{
	unsigned int ret = 0;
	int bat_lost = 0;
	int total_valid = 0;
	int timeout = 0;
	int bif_cmd[1] = { 0 };
	int loop_i = 0;

	/* Set command sequence */
	bif_cmd[0] = BC | BUSRESET;
	bif_set_cmd(bif_cmd, 1);

	do {
		/* Command setting : 1 write command */
		ret = pmic_set_register_value(PMIC_BIF_TRASFER_NUM, 1);
		ret = pmic_set_register_value(PMIC_BIF_COMMAND_TYPE, 0);

		/* Command set trigger */
		ret = pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 1);

		udelay(10);
		/* Command sent; wait for slave */
		bif_waitfor_slave();

		/* Command clear trigger */
		ret = pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 0);
		/* Check transaction completeness */
		bat_lost = pmic_get_register_value(PMIC_BIF_BAT_LOST);
		total_valid = pmic_get_register_value(PMIC_BIF_TOTAL_VALID);
		timeout = pmic_get_register_value(PMIC_BIF_TIMEOUT);

		if (loop_i < 50)
			loop_i++;
		else {
			battery_log(BAT_LOG_CRTI, "[BIF][bif_reset_slave]Failed at loop=%d",
				    loop_i);
			break;
		}
	} while (bat_lost == 1 || total_valid == 1 || timeout == 1);

	if (loop_i < 50) {
		battery_log(BAT_LOG_FULL, "[BIF][bif_reset_slave]OK at loop=%d", loop_i);
	/* Reset BIF_IRQ */
	bif_reset_irq();
		return 1;
	}
	return -1;
}

/* BIF write 8 transaction*/
static int bif_write8(int addr, int *data)
{
	int ret = 1;
	int era, wra;
	int bif_cmd[4] = { 0, 0, 0, 0};
	int loop_i = 0;
	int bat_lost = 0;
	int total_valid = 0;
	int timeout = 0;

	era = (addr & 0xFF00) >> 8;
	wra = addr & 0x00FF;
	battery_log(BAT_LOG_FULL, "[BIF][bif_write8]ERA=%x, WRA=%x\n", era, wra);
	/* Set command sequence */
	bif_cmd[0] = SDA | MW3790;
	bif_cmd[1] = ERA | era;	/* [15:8] */
	bif_cmd[2] = WRA | wra;	/* [ 7:0] */
	bif_cmd[3] = WD  | (*data & 0xFF);	/* data */


	bif_set_cmd(bif_cmd, 4);
	do {
		/* Command setting : 4 transactions for 1 byte write command(0) */
		pmic_set_register_value(PMIC_BIF_TRASFER_NUM, 4);
		pmic_set_register_value(PMIC_BIF_COMMAND_TYPE, 0);

		/* Command set trigger */
		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 1);

		udelay(200);
		/* Command sent; wait for slave */
		bif_waitfor_slave();

		/* Command clear trigger */
		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 0);
		/* Check transaction completeness */
		bat_lost = pmic_get_register_value(PMIC_BIF_BAT_LOST);
		total_valid = pmic_get_register_value(PMIC_BIF_TOTAL_VALID);
		timeout = pmic_get_register_value(PMIC_BIF_TIMEOUT);

		if (loop_i <= 50)
			loop_i++;
		else {
			battery_log(BAT_LOG_CRTI,
		"[BIF][bif_write8] Failed. bat_lost = %d, timeout = %d, totoal_valid = %d\n",
		bat_lost, timeout, total_valid);
			ret = -1;
			break;
		}
	} while (bat_lost == 1 || total_valid == 1 || timeout == 1);

	if (ret == 1)
		battery_log(BAT_LOG_FULL, "[BIF][bif_write8] OK for %d loop(s)\n", loop_i);
	else
		battery_log(BAT_LOG_CRTI, "[BIF][bif_write8] Failed for %d loop(s)\n", loop_i);

	/* Reset BIF_IRQ */
	bif_reset_irq();

	return ret;
}

/* BIF read 8 transaction*/
static int bif_read8(int addr, int *data)
{
	int ret = 1;
	int era, rra;
	int val = -1;
	int bif_cmd[3] = { 0, 0, 0 };
	int loop_i = 0;
	int bat_lost = 0;
	int total_valid = 0;
	int timeout = 0;

	battery_log(BAT_LOG_FULL, "[BIF][READ8]\n");

	era = (addr & 0xFF00) >> 8;
	rra = addr & 0x00FF;
	battery_log(BAT_LOG_FULL, "[BIF][bif_read8]ERA=%x, RRA=%x\n", era, rra);
	/* Set command sequence */
	bif_cmd[0] = SDA | MW3790;
	bif_cmd[1] = ERA | era;	/*[15:8] */
	bif_cmd[2] = RRA | rra;	/*[ 7:0] */

	bif_set_cmd(bif_cmd, 3);
	do {
		/* Command setting : 3 transactions for 1 byte read command(1) */
		pmic_set_register_value(PMIC_BIF_TRASFER_NUM, 3);
		pmic_set_register_value(PMIC_BIF_COMMAND_TYPE, 1);
		pmic_set_register_value(PMIC_BIF_READ_EXPECT_NUM, 1);

		/* Command set trigger */
		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 1);

		udelay(200);
		/* Command sent; wait for slave */
		bif_waitfor_slave();

		/* Command clear trigger */
		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 0);
		/* Check transaction completeness */
		bat_lost = pmic_get_register_value(PMIC_BIF_BAT_LOST);
		total_valid = pmic_get_register_value(PMIC_BIF_TOTAL_VALID);
		timeout = pmic_get_register_value(PMIC_BIF_TIMEOUT);

		if (loop_i <= 50)
			loop_i++;
		else {
			battery_log(BAT_LOG_CRTI,
		"[BIF][bif_read16] Failed. bat_lost = %d, timeout = %d, totoal_valid = %d\n",
		bat_lost, timeout, total_valid);
			ret = -1;
			break;
		}
	} while (bat_lost == 1 || total_valid == 1 || timeout == 1);

	/* Read data */
	if (ret == 1) {
		val = pmic_get_register_value(PMIC_BIF_DATA_0);
		battery_log(BAT_LOG_FULL, "[BIF][bif_read8] OK d0=0x%x, for %d loop(s)\n",
			    val, loop_i);
	} else
		battery_log(BAT_LOG_CRTI, "[BIF][bif_read8] Failed for %d loop(s)\n", loop_i);

	/* Reset BIF_IRQ */
	bif_reset_irq();

	*data = val;
	return ret;
}

/* Bif read 16 transaction*/
static int bif_read16(int addr)
{
	int ret = 1;
	int era, rra;
	int val = -1;
	int bif_cmd[4] = { 0, 0, 0, 0 };
	int loop_i = 0;
	int bat_lost = 0;
	int total_valid = 0;
	int timeout = 0;

	battery_log(BAT_LOG_FULL, "[BIF][READ]\n");

	era = (addr & 0xFF00) >> 8;
	rra = addr & 0x00FF;
	battery_log(BAT_LOG_FULL, "[BIF][bif_read16]ERA=%x, RRA=%x\n", era, rra);
	/*set command sequence */
	bif_cmd[0] = SDA | MW3790;
	bif_cmd[1] = BC | RBL2;	/* read back 2 bytes */
	bif_cmd[2] = ERA | era;	/*[15:8] */
	bif_cmd[3] = RRA | rra;	/*[ 7:0] */

	bif_set_cmd(bif_cmd, 4);
	do {
		/*command setting : 4 transactions for 2 byte read command(1) */
		pmic_set_register_value(PMIC_BIF_TRASFER_NUM, 4);
		pmic_set_register_value(PMIC_BIF_COMMAND_TYPE, 1);
		pmic_set_register_value(PMIC_BIF_READ_EXPECT_NUM, 2);

		/*Command set trigger */
		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 1);

		udelay(200);
		/*Command sent; wait for slave */
		bif_waitfor_slave();

		/*Command clear trigger */
		pmic_set_register_value(PMIC_BIF_TRASACT_TRIGGER, 0);
		/*check transaction completeness */
		bat_lost = pmic_get_register_value(PMIC_BIF_BAT_LOST);
		total_valid = pmic_get_register_value(PMIC_BIF_TOTAL_VALID);
		timeout = pmic_get_register_value(PMIC_BIF_TIMEOUT);

		if (loop_i <= 50)
			loop_i++;
		else {
			battery_log(BAT_LOG_CRTI,
		"[BIF][bif_read16] Failed. bat_lost = %d, timeout = %d, totoal_valid = %d\n",
		bat_lost, timeout, total_valid);
			ret = -1;
			break;
		}
	} while (bat_lost == 1 || total_valid == 1 || timeout == 1);

	/*Read data */
	if (ret == 1) {
		int d0, d1;

		d0 = pmic_get_register_value(PMIC_BIF_DATA_0);
		d1 = pmic_get_register_value(PMIC_BIF_DATA_1);
		val = 0xFF & d1;
		val = val | ((d0 & 0xFF) << 8);
		battery_log(BAT_LOG_FULL, "[BIF][bif_read16] OK d0=0x%x, d1=0x%x for %d loop(s)\n",
			    d0, d1, loop_i);
	} else
		battery_log(BAT_LOG_CRTI, "[BIF][bif_read16] Failed for %d loop(s)\n", loop_i);

	/*reset BIF_IRQ */
	bif_reset_irq();


	return val;
}

void bif_ADC_enable(void)
{
	int reg = 0x18;

	bif_write8(0x0110, &reg);
	mdelay(50);

	reg = 0x98;
	bif_write8(0x0110, &reg);
	mdelay(50);

}

/* BIF init function called only at the first time*/
static int bif_init(void)
{
	int pwr, rst;
	/*disable BIF interrupt */
	pmic_set_register_value(PMIC_INT_CON0_CLR, 0x4000);
	/*enable BIF clock */
	pmic_set_register_value(PMIC_TOP_CKPDN_CON2_CLR, 0x0070);

	/*enable HT protection */
	pmic_set_register_value(PMIC_RG_BATON_HT_EN, 1);

	/*change to HW control mode*/
	/*pmic_set_register_value(MT6351_PMIC_RG_VBIF28_ON_CTRL, 0);*/
	/*pmic_set_register_value(MT6351_PMIC_RG_VBIF28_EN, 1);*/
	mdelay(50);

	/*Enable RX filter function */
	pmic_set_register_value(MT6351_PMIC_BIF_RX_DEG_EN, 0x8000);
	pmic_set_register_value(MT6351_PMIC_BIF_RX_DEG_WND, 0x17);
	pmic_set_register_value(PMIC_RG_BATON_EN, 0x1);
	pmic_set_register_value(PMIC_BATON_TDET_EN, 0x1);
	pmic_set_register_value(PMIC_RG_BATON_HT_EN_DLY_TIME, 0x1);


	/*wake up BIF slave */
	pwr = bif_powerup_slave();
	mdelay(10);
	rst = bif_reset_slave();

	/*pmic_set_register_value(MT6351_PMIC_RG_VBIF28_ON_CTRL, 1);*/
	mdelay(50);

	battery_log(BAT_LOG_CRTI, "[BQ25896][BIF_init] done.");

	if (pwr + rst == 2)
		return 1;

	return -1;
}
#endif

int mtk_bif_init(void)
{
	int ret = 0;

#ifdef CONFIG_MTK_BIF_SUPPORT
	int vbat;

	vbat = 0;
	if (bif_checked != 1) {
		bif_init();
		mtk_bif_get_vbat(&vbat);
		if (vbat != 0) {
			battery_log(BAT_LOG_CRTI, "[BIF]BIF battery detected.\n");
			bif_exist = 1;
		} else
			battery_log(BAT_LOG_CRTI, "[BIF]BIF battery _NOT_ detected.\n");
		bif_checked = 1;
	}
#else
	ret = -ENOTSUPP;
#endif

	return ret;
}

int mtk_bif_get_vbat(unsigned int *vbat)
{
	int ret = 0;

#ifdef CONFIG_MTK_BIF_SUPPORT

	/* turn on VBIF28 regulator*/
	/*bif_init();*/

	/*change to HW control mode*/
	/*pmic_set_register_value(MT6351_PMIC_RG_VBIF28_ON_CTRL, 0);
	pmic_set_register_value(MT6351_PMIC_RG_VBIF28_EN, 1);*/
	if (bif_checked != 1 || bif_exist == 1) {
		bif_ADC_enable();
		*vbat = bif_read16(MW3790_VBAT);
	}

	/*turn off LDO and change SW control back to HW control */
	/*pmic_set_register_value(MT6351_PMIC_RG_VBIF28_EN, 0);
	pmic_set_register_value(MT6351_PMIC_RG_VBIF28_ON_CTRL, 1);*/
#else
	ret = -ENOTSUPP;
#endif

	return ret;
}

int mtk_bif_get_tbat(int *tbat)
{
	int ret = 0;

#ifdef CONFIG_MTK_BIF_SUPPORT
	int _tbat = 0;
	int tried = 0;

	mdelay(50);

	if (bif_exist == 1) {
		do {
			bif_ADC_enable();
			ret = bif_read8(MW3790_TBAT, &_tbat);
			tried++;
			mdelay(50);
			if (tried > 3)
				break;
		} while (ret != 1);

		if (tried <= 3)
			*tbat = _tbat;
		else
			ret = -ENOTSUPP;
	}
#else
	ret = -ENOTSUPP;
#endif

	return ret;
}
