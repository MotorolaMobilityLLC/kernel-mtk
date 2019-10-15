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

#include <linux/types.h>
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
//#include <mt-plat/charging.h>
#include <mt-plat/mtk_boot_common.h>
#include "bq2560x.h"


#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
#include <mt-plat/charger_class.h>
#include "mtk_charger_intf.h"
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */

#ifdef CONFIG_TINNO_JEITA_CURRENT
#ifdef CONFIG_TINNO_CUSTOM_CHARGER_PARA
#include "cust_charging.h"
#else
#include "cust_charger_init.h"
#endif
#endif
/**********************************************************
 *
 *   [I2C Slave Setting]
 *
 *********************************************************/
#define bq2560x_SLAVE_ADDR_WRITE   0xD6
#define bq2560x_SLAVE_ADDR_READ    0xD7

#define STATUS_OK	0
#define STATUS_FAIL	1
#define STATUS_UNSUPPORTED	-1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))

static const struct bq2560x_platform_data bq2560x_def_platform_data = {
	.chg_name = "primary_chg",
	.ichg = 1550000, /* unit: uA */
	.aicr = 500000, /* unit: uA */
	.mivr = 4500000, /* unit: uV */
	.ieoc = 150000, /* unit: uA */
	.voreg = 4350000, /* unit : uV */
	.vmreg = 4350000, /* unit : uV */
	.intr_gpio = 76,
};

/* ============================================================ // */
/* Global variable */
/* ============================================================ // */

const unsigned int VBAT_CV_VTH_2560X[] = {
	3856000, 3888000, 3920000, 3952000,
	3984000, 4016000, 4048000, 4080000,
	4112000, 4144000, 4176000, 4208000,
	4240000, 4272000, 4304000, 4336000,
	4368000, 4400000, 4432000, 4464000,
	4496000, 4528000, 4560000, 4592000,
	4624000
};

const unsigned int CS_ITERM_2560X[] = {
	60000,   120000,  180000,  240000,
	300000,  360000,  420000,  480000,
	540000,  600000,  660000,  720000,
	780000
};

const unsigned int CS_IPRECHG_2560X[] = {
	60000,   120000,  180000,  240000,
	300000,  360000,  420000,  480000,
	540000,  600000,  660000,  720000,
	780000,  840000,  900000,  960000
};

const unsigned int CS_VTH_2560X[] = {
	0,       60000,   120000,  180000,
	240000,  300000,  360000,  420000,
	480000,  540000,  600000,  660000,
	720000,  780000,  840000,  900000,
	960000,  1020000, 1080000, 1140000,
	1200000, 1260000, 1320000, 1380000,
	1440000, 1500000, 1560000, 1620000,
	1680000, 1740000, 1800000, 1860000,
	1920000, 1980000, 2040000, 2100000,
	2160000, 2220000, 2280000, 2340000,
	2400000, 2460000, 2520000, 2580000,
	2640000, 2700000, 2760000, 2820000,
	2880000, 2940000, 3000000
};

const unsigned int INPUT_CS_VTH_2560X[] = {
	100000,  200000,  300000,  400000,
	500000,  600000,  700000,  800000,
	900000,  1000000, 1100000, 1200000,
	1300000, 1400000, 1500000, 1600000,
	1700000, 1800000, 1900000, 2000000,
	2100000, 2200000, 2300000, 2400000,
	2500000, 2600000, 2700000, 2800000,
	2900000, 3000000, 3100000, 3200000
};

static struct i2c_client *new_client;
static const struct i2c_device_id bq2560x_i2c_id[] = { {"bq2560x", 0}, {} };

static bool chargin_hw_init_done = 0;
static int bq2560x_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

#ifdef CONFIG_OF
static const struct of_device_id bq2560x_of_match[] = {
	{.compatible = "mediatek,bq2560x",},		//caizhifu modified for bq2560x same with dts, 2016-10-28
	{},
};

struct bq2560x_info {
	struct device *dev;
	struct i2c_client *i2c;
#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
	struct charger_device *chg_dev;
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */
	u8 chip_rev;
};

MODULE_DEVICE_TABLE(of, bq2560x_of_match);
#endif

static struct i2c_driver bq2560x_driver = {
	.driver = {
		.name = "bq2560x",
#ifdef CONFIG_OF
		.of_match_table = bq2560x_of_match,
#endif
	},
	.probe = bq2560x_driver_probe,
	.id_table = bq2560x_i2c_id,
};

/**********************************************************
 *
 *   [Global Variable]
 *
 *********************************************************/
unsigned char bq2560x_reg[bq2560x_REG_NUM] = { 0 };

static DEFINE_MUTEX(bq2560x_i2c_access);

int g_bq2560x_hw_exist = 0;

/**********************************************************
 *
 *   [I2C Function For Read/Write bq2560x]
 *
 *********************************************************/
int bq2560x_read_byte(unsigned char cmd, unsigned char *returnData)
{
	int ret = 0;
	struct i2c_msg msgs[] =
	{
		{
			.addr = new_client->addr,
			.flags = 0,
			.len = 1,
			.buf = &cmd,
		},
		{
			.addr = new_client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = returnData,
		},
	};
	mutex_lock(&bq2560x_i2c_access);
	ret = i2c_transfer(new_client->adapter, msgs, 2);
	if(ret < 0){
		chr_err("%s: read 0x%x register failed\n",__func__,cmd);
	}
	pr_debug("%s:register = 0x%x , value = 0x%x\n",__func__,cmd,*returnData);
	mutex_unlock(&bq2560x_i2c_access);
	return 1;
}

int bq2560x_write_byte(unsigned char cmd, unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;
	struct i2c_msg msgs[] =
	{
		{
			.addr = new_client->addr,
			.flags = 0,
			.len = 2,
			.buf = write_data,
		},
	};

	write_data[0] = cmd;
	write_data[1] = writeData;
	pr_debug("%s cmd = 0x%x writeData = 0x%x\n",__func__,cmd,writeData);

	mutex_lock(&bq2560x_i2c_access);
	ret = i2c_transfer(new_client->adapter, msgs, 1);
	if (ret < 0){
		chr_err("%s: write 0x%x to 0x%x register failed\n",__func__,writeData,cmd);
		mutex_unlock(&bq2560x_i2c_access);
		return ret;
	}
	mutex_unlock(&bq2560x_i2c_access);
	return 1;
}

/**********************************************************
 *
 *   [Read / Write Function]
 *
 *********************************************************/
unsigned int bq2560x_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK,
		unsigned char SHIFT)
{
	unsigned char bq2560x_reg = 0;
	int ret = 0;


	ret = bq2560x_read_byte(RegNum, &bq2560x_reg);

	pr_debug("[bq2560x_read_interface] Reg[%x]=0x%x\n", RegNum, bq2560x_reg);

	bq2560x_reg &= (MASK << SHIFT);
	*val = (bq2560x_reg >> SHIFT);

	pr_debug("[bq2560x_read_interface] val=0x%x\n", *val);

	return ret;
}

unsigned int bq2560x_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK,
		unsigned char SHIFT)
{
	unsigned char bq2560x_reg = 0;
	int ret = 0;

	ret = bq2560x_read_byte(RegNum, &bq2560x_reg);
	pr_debug("[bq2560x_config_interface] Reg[%x]=0x%x\n", RegNum, bq2560x_reg);

	bq2560x_reg &= ~(MASK << SHIFT);
	bq2560x_reg |= (val << SHIFT);

	ret = bq2560x_write_byte(RegNum, bq2560x_reg);
	pr_debug("[bq2560x_config_interface] write Reg[%x]=0x%x\n", RegNum, bq2560x_reg);

	/* Check */
	/* bq2560x_read_byte(RegNum, &bq2560x_reg); */
	/* battery_log(BAT_LOG_FULL, "[bq2560x_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq2560x_reg); */

	return ret;
}

/* write one register directly */
unsigned int bq2560x_reg_config_interface(unsigned char RegNum, unsigned char val)
{
	unsigned int ret = 0;

	ret = bq2560x_write_byte(RegNum, val);

	return ret;
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
/* CON0---------------------------------------------------- */

void bq2560x_set_en_hiz(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON0),
			(unsigned char) (val),
			(unsigned char) (CON0_EN_HIZ_MASK),
			(unsigned char) (CON0_EN_HIZ_SHIFT)
			);
}

void bq2560x_set_iinlim(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON0),
			(unsigned char) (val),
			(unsigned char) (CON0_IINLIM_MASK),
			(unsigned char) (CON0_IINLIM_SHIFT)
			);
}


/* CON1---------------------------------------------------- */

void bq2560x_set_reg_rst(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON11),
			(unsigned char) (val),
			(unsigned char) (CON11_REG_RST_MASK),
			(unsigned char) (CON11_REG_RST_SHIFT)
			);
}

void bq2560x_set_wdt_rst(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON1),
			(unsigned char) (val),
			(unsigned char) (CON1_WDT_RST_MASK),
			(unsigned char) (CON1_WDT_RST_SHIFT)
			);
}

void bq2560x_set_otg_config(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON1),
			(unsigned char) (val),
			(unsigned char) (CON1_OTG_CONFIG_MASK),
			(unsigned char) (CON1_OTG_CONFIG_SHIFT)
			);
}


void bq2560x_set_chg_config(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON1),
			(unsigned char) (val),
			(unsigned char) (CON1_CHG_CONFIG_MASK),
			(unsigned char) (CON1_CHG_CONFIG_SHIFT)
			);
}


void bq2560x_set_sys_min(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON1),
			(unsigned char) (val),
			(unsigned char) (CON1_SYS_MIN_MASK),
			(unsigned char) (CON1_SYS_MIN_SHIFT)
			);
}

void bq2560x_set_batlowv(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON1),
			(unsigned char) (val),
			(unsigned char) (CON1_MIN_VBAT_SEL_MASK),
			(unsigned char) (CON1_MIN_VBAT_SEL_SHIFT)
			);
}



/* CON2---------------------------------------------------- */

void bq2560x_set_boost_lim(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON2),
			(unsigned char) (val),
			(unsigned char) (CON2_BOOST_LIM_MASK),
			(unsigned char) (CON2_BOOST_LIM_SHIFT)
			);
}

void bq2560x_set_ichg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON2),
			(unsigned char) (val),
			(unsigned char) (CON2_ICHG_MASK), (unsigned char) (CON2_ICHG_SHIFT)
			);
}

#if 0 //this function does not exist on bq2560x
void bq2560x_set_force_20pct(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON2),
			(unsigned char) (val),
			(unsigned char) (CON2_FORCE_20PCT_MASK),
			(unsigned char) (CON2_FORCE_20PCT_SHIFT)
			);
}
#endif
/* CON3---------------------------------------------------- */

void bq2560x_set_iprechg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON3),
			(unsigned char) (val),
			(unsigned char) (CON3_IPRECHG_MASK),
			(unsigned char) (CON3_IPRECHG_SHIFT)
			);
}

void bq2560x_set_iterm(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON3),
			(unsigned char) (val),
			(unsigned char) (CON3_ITERM_MASK), (unsigned char) (CON3_ITERM_SHIFT)
			);
}

/* CON4---------------------------------------------------- */

void bq2560x_set_vreg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON4),
			(unsigned char) (val),
			(unsigned char) (CON4_VREG_MASK), (unsigned char) (CON4_VREG_SHIFT)
			);
}

void bq2560x_set_topoff_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON4),
			(unsigned char) (val),
			(unsigned char) (CON4_TOPOFF_TIMER_MASK), (unsigned char) (CON4_TOPOFF_TIMER_SHIFT)
			);

}


void bq2560x_set_vrechg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON4),
			(unsigned char) (val),
			(unsigned char) (CON4_VRECHG_MASK),
			(unsigned char) (CON4_VRECHG_SHIFT)
			);
}

/* CON5---------------------------------------------------- */

void bq2560x_set_en_term(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON5),
			(unsigned char) (val),
			(unsigned char) (CON5_EN_TERM_MASK),
			(unsigned char) (CON5_EN_TERM_SHIFT)
			);
}



void bq2560x_set_watchdog(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON5),
			(unsigned char) (val),
			(unsigned char) (CON5_WATCHDOG_MASK),
			(unsigned char) (CON5_WATCHDOG_SHIFT)
			);
}

void bq2560x_set_en_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON5),
			(unsigned char) (val),
			(unsigned char) (CON5_EN_TIMER_MASK),
			(unsigned char) (CON5_EN_TIMER_SHIFT)
			);
}

void bq2560x_set_chg_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON5),
			(unsigned char) (val),
			(unsigned char) (CON5_CHG_TIMER_MASK),
			(unsigned char) (CON5_CHG_TIMER_SHIFT)
			);
}

/* CON6---------------------------------------------------- */
void bq2560x_set_vindpm(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON6),
			(unsigned char) (val),
			(unsigned char) (CON6_VINDPM_MASK),
			(unsigned char) (CON6_VINDPM_SHIFT)
			);
}


void bq2560x_set_ovp(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON6),
			(unsigned char) (val),
			(unsigned char) (CON6_OVP_MASK),
			(unsigned char) (CON6_OVP_SHIFT)
			);

}

void bq2560x_set_boostv(unsigned int val)
{

	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON6),
			(unsigned char) (val),
			(unsigned char) (CON6_BOOSTV_MASK),
			(unsigned char) (CON6_BOOSTV_SHIFT)
			);



}



/* CON7---------------------------------------------------- */

void bq2560x_set_tmr2x_en(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON7),
			(unsigned char) (val),
			(unsigned char) (CON7_TMR2X_EN_MASK),
			(unsigned char) (CON7_TMR2X_EN_SHIFT)
			);
}

void bq2560x_set_batfet_disable(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON7),
			(unsigned char) (val),
			(unsigned char) (CON7_BATFET_Disable_MASK),
			(unsigned char) (CON7_BATFET_Disable_SHIFT)
			);
}


void bq2560x_set_batfet_delay(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON7),
			(unsigned char) (val),
			(unsigned char) (CON7_BATFET_DLY_MASK),
			(unsigned char) (CON7_BATFET_DLY_SHIFT)
			);
}

void bq2560x_set_batfet_reset_enable(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON7),
			(unsigned char) (val),
			(unsigned char) (CON7_BATFET_RST_EN_MASK),
			(unsigned char) (CON7_BATFET_RST_EN_SHIFT)
			);
}


/* CON8---------------------------------------------------- */

unsigned int bq2560x_get_system_status(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq2560x_read_interface((unsigned char) (bq2560x_CON8),
			(&val), (unsigned char) (0xFF), (unsigned char) (0x0)
			);
	return val;
}

unsigned int bq2560x_get_vbus_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq2560x_read_interface((unsigned char) (bq2560x_CON8),
			(&val),
			(unsigned char) (CON8_VBUS_STAT_MASK),
			(unsigned char) (CON8_VBUS_STAT_SHIFT)
			);
	return val;
}

unsigned int bq2560x_get_chrg_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq2560x_read_interface((unsigned char) (bq2560x_CON8),
			(&val),
			(unsigned char) (CON8_CHRG_STAT_MASK),
			(unsigned char) (CON8_CHRG_STAT_SHIFT)
			);
	return val;
}

unsigned int bq2560x_get_vsys_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq2560x_read_interface((unsigned char) (bq2560x_CON8),
			(&val),
			(unsigned char) (CON8_VSYS_STAT_MASK),
			(unsigned char) (CON8_VSYS_STAT_SHIFT)
			);
	return val;
}

unsigned int bq2560x_get_pg_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq2560x_read_interface((unsigned char) (bq2560x_CON8),
			(&val),
			(unsigned char) (CON8_PG_STAT_MASK),
			(unsigned char) (CON8_PG_STAT_SHIFT)
			);
	return val;
}


/*CON10----------------------------------------------------------*/

void bq2560x_set_int_mask(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq2560x_config_interface((unsigned char) (bq2560x_CON10),
			(unsigned char) (val),
			(unsigned char) (CON10_INT_MASK_MASK),
			(unsigned char) (CON10_INT_MASK_SHIFT)
			);
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
bool bq2560x_hw_component_detect(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq2560x_read_interface(0x0B, &val, 0x0F, 0x03);

	if (val == 0)
		g_bq2560x_hw_exist = 0;
	else
		g_bq2560x_hw_exist = 1;
	pr_debug("[bq2560x_hw_component_detect] Reg[0x0B]=0x%x\n",val);
	ret = bq2560x_read_interface(0x0A, &val, 0xFF, 0x00);
	if (val == 0x20)
		g_bq2560x_hw_exist = 0;
	pr_debug("[bq2560x_hw_component_detect] Reg[0x0B]=0x%x\n",val);

	return g_bq2560x_hw_exist;
}

int is_bq2560x_exist(void)
{
	pr_debug("[is_bq2560x_exist] g_bq2560x_hw_exist=%d\n", g_bq2560x_hw_exist);

	return g_bq2560x_hw_exist;
}

void bq2560x_dump_register(void)
{
	int i = 0;

	for (i = 0; i < bq2560x_REG_NUM; i++) {
		bq2560x_read_byte(i, &bq2560x_reg[i]);
		chr_err("bq2560x:[0x%x]=0x%x\n", i, bq2560x_reg[i]);
	}
}

static u32 charging_parameter_to_value(const u32 *parameter, const u32 array_size, const u32 val)
{
	u32 i;

	for (i = 0; i < array_size; i++)
		if (val == *(parameter + i))
			return i;

	chr_info("NO register value match \r\n");

	return 0;
}

static u32 bmt_find_closest_level(const u32 *pList, u32 number, u32 level)
{
	u32 i;
	u32 max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = true;
	else
		max_value_in_last_element = false;

	if (max_value_in_last_element == true) {
		if(level > pList[number-1])
			level = pList[number-1];

		if(level < pList[0])
		{
			level = pList[0];
		}

		for (i = (number - 1); i != 0; i--)	/* max value in the last element */
		{
			if (pList[i] <= level)
				return pList[i];
		}

		chr_info("Can't find closest level, small value first \r\n");
		return pList[0];
		/* return CHARGE_CURRENT_0_00_MA; */
	} else {
		if(level > pList[0])
			level = pList[0];

		if(level < pList[number-1])
		{
			level = pList[number-1];
		}

		for (i = 0; i < number; i++)	/* max value in the first element */
		{
			if (pList[i] <= level)
				return pList[i];
		}

		chr_info("Can't find closest level, large value first \r\n");
		return pList[number - 1];
		/* return CHARGE_CURRENT_0_00_MA; */
	}
}

#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
static unsigned int charging_hw_init(void)
{
	unsigned int status = STATUS_OK;
	int array_size, i;
	bq2560x_set_en_hiz(0x0);
	bq2560x_set_vindpm(0x06);	/* VIN DPM check 4.6V */
	bq2560x_set_reg_rst(0x0);
	bq2560x_set_wdt_rst(0x1);	/* Kick watchdog */
	bq2560x_set_sys_min(0x5);	/* Minimum system voltage 3.5V */

	array_size = sizeof(CS_IPRECHG_2560X)/sizeof(CS_IPRECHG_2560X[0]);
	for(i=array_size-1; i>0; i--)
	{
		if(CS_IPRECHG_2560X[i] <= PRECHARGE_CURRENT){
			bq2560x_set_iprechg(i);
			break;
		}
	}
	//bq2560x_set_iprechg(0x1);	/* Precharge current 120mA */

	array_size = sizeof(CS_ITERM_2560X)/sizeof(CS_ITERM_2560X[0]);
	for(i=array_size-1; i>0; i--)
	{
		if(CS_ITERM_2560X[i] <= TERMINATION_CURRENT){
			bq2560x_set_iterm(i);
			break;
		}
	}
	//bq2560x_set_iterm(0x2);	/* Termination current 180mA */

	bq2560x_set_vreg(0x88);	/* VREG 4.4V */

	bq2560x_set_batlowv(0x1);	/* BATLOWV 3.0V */
	bq2560x_set_vrechg(0x0);	/* VRECHG 0.1V (4.300V) */
	bq2560x_set_en_term(0x1);	/* Enable termination */
	bq2560x_set_watchdog(0x0);	/* WDT 40s */
	bq2560x_set_en_timer(0x0);	/* Enable charge timer */
	//bq2560x_set_chg_timer(0x02);	/*set charge time 12h*/
	bq2560x_set_int_mask(0x0);	/* Disable fault interrupt */

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	if (wireless_charger_gpio_number != 0) {
		mt_set_gpio_mode(wireless_charger_gpio_number, 0);	/* 0:GPIO mode */
		mt_set_gpio_dir(wireless_charger_gpio_number, 0);	/* 0: input, 1: output */
	}
#endif

#ifdef CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT
	mt_set_gpio_mode(vin_sel_gpio_number, 0);	/* 0:GPIO mode */
	mt_set_gpio_dir(vin_sel_gpio_number, 0);	/* 0: input, 1: output */
#endif
	return status;
}

static int bq2560x_charger_plug_in(struct charger_device *chg_dev)
{
	//do nothing
	return 0;
}

static int bq2560x_charger_plug_out(struct charger_device *chg_dev)
{
	//do nothing
	return 0;
}

static int bq2560x_charging_enable(struct charger_device *chg_dev, bool en)
{
	unsigned int status = STATUS_OK;

	pr_debug("bq2560x_charging_enable is %d\n",en);

	if (en == true) {
		bq2560x_set_en_hiz(0x0);
		bq2560x_set_chg_config(0x1);
	} 
	else{
#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if (mt_usb_is_device())
#endif
		{
			bq2560x_set_chg_config(0x0);
			bq2560x_set_en_hiz(0x0);
		}
#if defined(CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT)
		bq2560x_set_chg_config(0x0);
		bq2560x_set_en_hiz(0x0);
#endif
	}

	return status;
}

static int bq2560x_charger_is_enabled(struct charger_device *chg_dev, bool *en)
{
	return 0;
}

static int bq2560x_charger_get_ichg(struct charger_device *chg_dev, u32 *uA)
{
	unsigned int status = STATUS_OK;
	/* unsigned int array_size; */
	/* unsigned char reg_value; */
	unsigned char ret_val = 0;;

	/* Get current level */
	bq2560x_read_interface(bq2560x_CON2, &ret_val, CON2_ICHG_MASK, CON2_ICHG_SHIFT);

	/* Parsing */
	ret_val = (ret_val * 60) ;
	*(unsigned int *)uA = ret_val;
	return status;
}

static int bq2560x_charger_set_ichg(struct charger_device *chg_dev, u32 uA)
{
	unsigned int status = STATUS_OK;
	unsigned int set_chr_current;
	unsigned int array_size=0;
	unsigned int register_value;
	unsigned int current_value = uA;
	pr_debug("bq2560x set charging current = %d\n",uA);
	array_size = GETARRAYNUM(CS_VTH_2560X);
	set_chr_current = bmt_find_closest_level(CS_VTH_2560X, array_size, current_value);

	register_value = charging_parameter_to_value(CS_VTH_2560X, array_size, set_chr_current);

	bq2560x_set_ichg(register_value);

	return status;
}

static int bq2560x_charger_get_min_ichg(struct charger_device *chg_dev, u32 *uA)
{
	*uA = 450000;
	return 0;
}

static int bq2560x_charger_set_cv(struct charger_device *chg_dev, u32 uV)
{
	unsigned int status = 0;
	unsigned int array_size = 0;
	unsigned int set_cv_voltage;
	unsigned short register_value;
	unsigned int cv_value = uV;
	static short pre_register_value = -1;
	pr_debug("bq2560x set cv = %d\n",uV);
	/* use nearest value */
	if (4200000 == cv_value)
		cv_value = 4208000;

	array_size = GETARRAYNUM(VBAT_CV_VTH_2560X);
	set_cv_voltage = bmt_find_closest_level(VBAT_CV_VTH_2560X, array_size, cv_value);
	register_value = charging_parameter_to_value(VBAT_CV_VTH_2560X, array_size, set_cv_voltage);
	pr_debug("set_cv_voltage = %d,register_value = %d\n",set_cv_voltage,register_value);

	bq2560x_set_vreg(register_value);

	/* for jeita recharging issue */
	if (pre_register_value != register_value)
		bq2560x_set_chg_config(1);

	pre_register_value = register_value;

	return status;
}

static int bq2560x_charger_get_cv(struct charger_device *chg_dev, u32 *uV)
{
	//do nothing
	return 0;
}

static int bq2560x_charger_get_aicr(struct charger_device *chg_dev, u32 *uA)
{
	//do nothing
	return 0;
}

static int bq2560x_charger_set_aicr(struct charger_device *chg_dev, u32 uA)
{
	unsigned int status = STATUS_OK;
	unsigned int  i=0;
	unsigned int  array_size=0;
	pr_debug("bq2560x_charger_set_input current = %d\n",uA);
	array_size = sizeof(INPUT_CS_VTH_2560X)/sizeof(INPUT_CS_VTH_2560X[0]);
	for(i=array_size-1; i>0; i--)
	{
		if(INPUT_CS_VTH_2560X[i] <= uA){
			bq2560x_set_iinlim(i);
			break;
		}
	}
	return status;
}

static int bq2560x_charger_get_min_aicr(struct charger_device *chg_dev, u32 *uA)
{
	//do nothing
	*uA = 100000;
	return 0;
}

static int bq2560x_charger_set_mivr(struct charger_device *chg_dev, u32 uV)
{
	//do nothing
	return 0;
}

static int bq2560x_charger_is_timer_enabled(struct charger_device *chg_dev,
		bool *en)
{
	return 0;
}

static int bq2560x_charger_enable_timer(struct charger_device *chg_dev, bool en)
{
	unsigned int status = 0;
	pr_debug("charging_reset_watch_dog_timer\r\n");

	bq2560x_set_wdt_rst(0x1);	/* Kick watchdog */

	return status;
}

static int bq2560x_charger_enable_te(struct charger_device *chg_dev, bool en)
{
	bq2560x_set_en_term(0x01);
	return 0;
}

static int bq2560x_charger_enable_otg(struct charger_device *chg_dev, bool en)
{
	if (en)
	{
		bq2560x_set_otg_config(0x1); /* OTG */
		bq2560x_set_boostv(0x7); /* boost voltage 4.998V */
		bq2560x_set_boost_lim(0x1); /* 1.5A on VBUS */
	}else{
		bq2560x_set_otg_config(0);
	}

	return 0;
}

static int bq2560x_charger_is_charging_done(struct charger_device *chg_dev,
		bool *done)
{
	unsigned char reg_val = 0;
	bq2560x_read_byte(0x08, &reg_val);
	reg_val = reg_val >> 3;
	reg_val &= 0x03;
	if(reg_val == 0x03)
		*done = 1;
	else
		*done = 0;

	return 0;
}

static int bq2560x_charger_dump_registers(struct charger_device *chg_dev)
{
	bq2560x_dump_register();
	return 0;
}

static int bq2560x_charger_do_event(struct charger_device *chg_dev, u32 event,
		u32 args)
{
	struct bq2560x_info *ri = charger_get_data(chg_dev);

	switch (event) {
		case EVENT_EOC:
			chr_info("do eoc event\n");
			charger_dev_notify(ri->chg_dev, CHARGER_DEV_NOTIFY_EOC);
			break;
		case EVENT_RECHARGE:
			chr_info("do recharge event\n");
			charger_dev_notify(ri->chg_dev, CHARGER_DEV_NOTIFY_RECHG);
			break;
		default:
			break;
	}
	return 0;
}


static const struct charger_ops bq2560x_chg_ops = {
	/* cable plug in/out */
	.plug_in = bq2560x_charger_plug_in,
	.plug_out = bq2560x_charger_plug_out,
	/* enable */
	.enable = bq2560x_charging_enable,
	.is_enabled = bq2560x_charger_is_enabled,
	/* charging current */
	.get_charging_current = bq2560x_charger_get_ichg,
	.set_charging_current = bq2560x_charger_set_ichg,
	.get_min_charging_current = bq2560x_charger_get_min_ichg,
	/* charging voltage */
	.set_constant_voltage = bq2560x_charger_set_cv,
	.get_constant_voltage = bq2560x_charger_get_cv,
	/* charging input current */
	.get_input_current = bq2560x_charger_get_aicr,
	.set_input_current = bq2560x_charger_set_aicr,
	.get_min_input_current = bq2560x_charger_get_min_aicr,
	/* charging mivr */
	.set_mivr = bq2560x_charger_set_mivr,
	/* safety timer */
	.is_safety_timer_enabled = bq2560x_charger_is_timer_enabled,
	.enable_safety_timer = bq2560x_charger_enable_timer,
	/* charing termination */
	.enable_termination = bq2560x_charger_enable_te,
	/* OTG */
	.enable_otg = bq2560x_charger_enable_otg,
	/* misc */
	.is_charging_done = bq2560x_charger_is_charging_done,
	.dump_registers = bq2560x_charger_dump_registers,
	/* event */
	.event = bq2560x_charger_do_event,
};

static const struct charger_properties bq2560x_chg_props = {
	.alias_name = "bq2560x",
};
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */

static int bq2560x_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct bq2560x_platform_data *pdata = dev_get_platdata(&client->dev);
	struct bq2560x_info *ri = NULL;
	bool use_dt = client->dev.of_node;
	int err = 0;
	pr_err("[bq2560x_driver_probe]\n");

	new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);

	if (!new_client) {
		err = -ENOMEM;
		goto exit;
	}
	memset(new_client, 0, sizeof(struct i2c_client));

	ri = devm_kzalloc(&client->dev, sizeof(*ri), GFP_KERNEL);
	if (!ri)
	{
		return -ENOMEM;
	}
	/* platform data */
	if (use_dt) {
		pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;
		memcpy(pdata, &bq2560x_def_platform_data, sizeof(*pdata));
		client->dev.platform_data = pdata;
	} else {
		if (!pdata) {
			pr_debug("no pdata specify\n");
			return -EINVAL;
		}
	}

	client->addr = 0x6b;
	new_client = client;
	ri->dev = &client->dev;
	ri->i2c = client;
	ri->chip_rev = 0;
	i2c_set_clientdata(client, ri);
	if(!bq2560x_hw_component_detect()){
		pr_err("bq2560x is not exist\n");
		err = -1;
		goto exit;
	}
	charging_hw_init();
	/* --------------------- */


#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
	/* charger class register */
	ri->chg_dev = charger_device_register(pdata->chg_name, ri->dev, ri,
			&bq2560x_chg_ops,
			&bq2560x_chg_props);
	if (IS_ERR(ri->chg_dev)) {
		pr_debug("charger device register fail\n");
		return PTR_ERR(ri->chg_dev);
	}
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */

	bq2560x_dump_register();
	chargin_hw_init_done = 1;

	return 0;

exit:
	return err;

}

/**********************************************************
 *
 *   [platform_driver API]
 *
 *********************************************************/
unsigned char g_reg_value_bq2560x = 0;
static ssize_t show_bq2560x_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i=0;
	int offset = 0;

	bq2560x_dump_register();
	for(i = 0; i<12; i++)
	{
		offset += sprintf(buf+offset, "reg[0x%x] = 0x%x\n", i, bq2560x_reg[i]);
	}

	return offset;
}

static ssize_t store_bq2560x_access(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_debug("[store_bq2560x_access]\n");

	if (buf != NULL && size != 0) {
		pr_debug("[store_bq2560x_access] buf is %s and size is %zu\n", buf, size);
		/*reg_address = kstrtoul(buf, 16, &pvalue);*/

		pvalue = (char *)buf;
		if (size > 3) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);

		if (size > 3) {
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);

			pr_debug("[store_bq2560x_access] write bq2560x reg 0x%x with value 0x%x !\n",reg_address, reg_value);
			ret = bq2560x_config_interface(reg_address, reg_value, 0xFF, 0x0);
		} else {
			ret = bq2560x_read_interface(reg_address, &g_reg_value_bq2560x, 0xFF, 0x0);
			pr_debug("[store_bq2560x_access] read bq2560x reg 0x%x with value 0x%x !\n",reg_address, g_reg_value_bq2560x);
			pr_debug("[store_bq2560x_access] Please use \"cat bq2560x_access\" to get value\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(bq2560x_access, 0664, show_bq2560x_access, store_bq2560x_access);	/* 664 */

static int bq2560x_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	pr_debug("******** bq2560x_user_space_probe!! ********\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_bq2560x_access);

	return 0;
}

struct platform_device bq2560x_user_space_device = {
	.name = "bq2560x-user",
	.id = -1,
};

static struct platform_driver bq2560x_user_space_driver = {
	.probe = bq2560x_user_space_probe,
	.driver = {
		.name = "bq2560x-user",
	},
};

static int __init bq2560x_subsys_init(void)
{
	int ret = 0;

	if (i2c_add_driver(&bq2560x_driver) != 0)
		pr_debug("[bq24261_init] failed to register bq24261 i2c driver.\n");
	else
		pr_debug("[bq24261_init] Success to register bq24261 i2c driver.\n");

	/* bq2560x user space access interface */
	ret = platform_device_register(&bq2560x_user_space_device);
	if (ret) {
		pr_debug("****[bq2560x_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&bq2560x_user_space_driver);
	if (ret) {
		pr_debug("****[bq2560x_init] Unable to register driver (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void __exit bq2560x_exit(void)
{
	i2c_del_driver(&bq2560x_driver);
}

/* module_init(bq2560x_init); */
/* module_exit(bq2560x_exit); */
subsys_initcall(bq2560x_subsys_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C bq2560x Driver");
MODULE_AUTHOR("YT Lee<yt.lee@mediatek.com>");
