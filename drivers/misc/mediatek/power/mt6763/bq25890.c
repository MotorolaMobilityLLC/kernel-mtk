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

#include <linux/types.h>
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <mach/mtk_charging.h>
#include <mt-plat/charging.h>
#include <mt-plat/battery_common.h>
#include "bq25890.h"
/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/

#ifdef CONFIG_OF
#else

#define bq25890_SLAVE_ADDR_WRITE   0xD6
#define bq25890_SLAVE_ADDR_Read    0xD7

#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define bq25890_BUSNUM I2C_SWITHING_CHARGER_CHANNEL
#else
#define bq25890_BUSNUM 0
#endif

#endif
static struct i2c_client *new_client;
static const struct i2c_device_id bq25890_i2c_id[] = { {"bq25890", 0}, {} };

kal_bool chargin_hw_init_done;
static int bq25890_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);


/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
unsigned char bq25890_reg[bq25890_REG_NUM] = { 0 };

static DEFINE_MUTEX(bq25890_i2c_access);
static DEFINE_MUTEX(bq25890_access_lock);

int g_bq25890_hw_exist;

/**********************************************************
  *
  *   [I2C Function For Read/Write bq25890]
  *
  *********************************************************/
#ifdef CONFIG_MTK_I2C_EXTENSION
unsigned int bq25890_read_byte(unsigned char cmd, unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&bq25890_i2c_access);

	/* new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG; */
	new_client->ext_flag =
	    ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

	cmd_buf[0] = cmd;
	ret = i2c_master_send(new_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
		new_client->ext_flag = 0;
		mutex_unlock(&bq25890_i2c_access);

		return 0;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
	new_client->ext_flag = 0;
	mutex_unlock(&bq25890_i2c_access);

	return 1;
}

unsigned int bq25890_write_byte(unsigned char cmd, unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&bq25890_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

	new_client->ext_flag = ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG;

	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		new_client->ext_flag = 0;
		mutex_unlock(&bq25890_i2c_access);
		return 0;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&bq25890_i2c_access);
	return 1;
}
#else
unsigned int bq25890_read_byte(unsigned char cmd, unsigned char *returnData)
{
	unsigned char xfers = 2;
	int ret, retries = 1;

	mutex_lock(&bq25890_i2c_access);

	do {
		struct i2c_msg msgs[2] = {
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
			}
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			battery_log(BAT_LOG_CRTI, "skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&bq25890_i2c_access);

	return ret == xfers ? 1 : -1;
}

unsigned int bq25890_write_byte(unsigned char cmd, unsigned char writeData)
{
	unsigned char xfers = 1;
	int ret, retries = 1;
	unsigned char buf[8];

	mutex_lock(&bq25890_i2c_access);

	buf[0] = cmd;
	memcpy(&buf[1], &writeData, 1);

	do {
		struct i2c_msg msgs[1] = {
			{
				.addr = new_client->addr,
				.flags = 0,
				.len = 1 + 1,
				.buf = buf,
			},
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			battery_log(BAT_LOG_CRTI, "skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&bq25890_i2c_access);

	return ret == xfers ? 1 : -1;
}
#endif
/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
unsigned int bq25890_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK,
				  unsigned char SHIFT)
{
	unsigned char bq25890_reg = 0;
	unsigned int ret = 0;

	ret = bq25890_read_byte(RegNum, &bq25890_reg);

	battery_log(BAT_LOG_FULL, "[bq25890_read_interface] Reg[%x]=0x%x\n", RegNum, bq25890_reg);

	bq25890_reg &= (MASK << SHIFT);
	*val = (bq25890_reg >> SHIFT);

	battery_log(BAT_LOG_FULL, "[bq25890_read_interface] val=0x%x\n", *val);

	return ret;
}

unsigned int bq25890_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK,
				    unsigned char SHIFT)
{
	unsigned char bq25890_reg = 0;
	unsigned char bq25890_reg_ori = 0;
	unsigned int ret = 0;

	mutex_lock(&bq25890_access_lock);
	ret = bq25890_read_byte(RegNum, &bq25890_reg);

	bq25890_reg_ori = bq25890_reg;
	bq25890_reg &= ~(MASK << SHIFT);
	bq25890_reg |= (val << SHIFT);

	ret = bq25890_write_byte(RegNum, bq25890_reg);
	mutex_unlock(&bq25890_access_lock);
	battery_log(BAT_LOG_FULL, "[bq25890_config_interface] write Reg[%x]=0x%x from 0x%x\n", RegNum,
		    bq25890_reg, bq25890_reg_ori);

	/* Check */
	/* bq25890_read_byte(RegNum, &bq25890_reg); */
	/* printk("[bq25890_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq25890_reg); */

	return ret;
}

/* write one register directly */
unsigned int bq25890_reg_config_interface(unsigned char RegNum, unsigned char val)
{
	unsigned int ret = 0;

	ret = bq25890_write_byte(RegNum, val);

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
/* CON0---------------------------------------------------- */

void bq25890_set_en_hiz(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_EN_HIZ_MASK),
				       (unsigned char) (CON0_EN_HIZ_SHIFT)
	    );
}

void bq25890_set_en_ilim(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_EN_ILIM_MASK),
				       (unsigned char) (CON0_EN_ILIM_SHIFT)
	    );
}

void bq25890_set_iinlim(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON0),
				       (val),
				       (unsigned char) (CON0_IINLIM_MASK),
				       (unsigned char) (CON0_IINLIM_SHIFT)
	    );
}

unsigned int bq25890_get_iinlim(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON0),
				     (&val),
				     (unsigned char) (CON0_IINLIM_MASK), (unsigned char) (CON0_IINLIM_SHIFT)
	    );
	return val;
}



/* CON1---------------------------------------------------- */

void bq25890_ADC_start(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_CONV_START_MASK),
				       (unsigned char) (CON2_CONV_START_SHIFT)
	    );
}

void bq25890_set_ADC_rate(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_CONV_RATE_MASK),
				       (unsigned char) (CON2_CONV_RATE_SHIFT)
	    );
}

void bq25890_set_vindpm_os(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_VINDPM_OS_MASK),
				       (unsigned char) (CON1_VINDPM_OS_SHIFT)
	    );
}

/* CON2---------------------------------------------------- */

void bq25890_set_ico_en_start(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_ICO_EN_MASK),
				       (unsigned char) (CON2_ICO_EN_RATE_SHIFT)
	    );
}



/* CON3---------------------------------------------------- */

void bq25890_wd_reset(unsigned int val)
{
	unsigned int ret = 0;


	ret = bq25890_config_interface((unsigned char) (bq25890_CON3),
				       (val),
				       (unsigned char) (CON3_WD_MASK), (unsigned char) (CON3_WD_SHIFT)
	    );

}

void bq25890_otg_en(unsigned int val)
{
	unsigned int ret = 0;


	ret = bq25890_config_interface((unsigned char) (bq25890_CON3),
				       (val),
				       (unsigned char) (CON3_OTG_CONFIG_MASK),
				       (unsigned char) (CON3_OTG_CONFIG_SHIFT)
	    );

}

void bq25890_chg_en(unsigned int val)
{
	unsigned int ret = 0;


	ret = bq25890_config_interface((unsigned char) (bq25890_CON3),
				       (val),
				       (unsigned char) (CON3_CHG_CONFIG_MASK),
				       (unsigned char) (CON3_CHG_CONFIG_SHIFT)
	    );

}

unsigned int bq25890_get_chg_en(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON3),
				     (&val),
				     (unsigned char) (CON3_CHG_CONFIG_MASK),
				     (unsigned char) (CON3_CHG_CONFIG_SHIFT)
	    );
	return val;
}


void bq25890_set_sys_min(unsigned int val)
{
	unsigned int ret = 0;


	ret = bq25890_config_interface((unsigned char) (bq25890_CON3),
				       (val),
				       (unsigned char) (CON3_SYS_V_LIMIT_MASK),
				       (unsigned char) (CON3_SYS_V_LIMIT_SHIFT)
	    );

}

/* CON4---------------------------------------------------- */

void bq25890_en_pumpx(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_EN_PUMPX_MASK),
				       (unsigned char) (CON4_EN_PUMPX_SHIFT)
	    );
}


void bq25890_set_ichg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_ICHG_MASK), (unsigned char) (CON4_ICHG_SHIFT)
	    );
}

unsigned int bq25890_get_reg_ichg(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON4),
				     (&val),
				     (unsigned char) (CON4_ICHG_MASK), (unsigned char) (CON4_ICHG_SHIFT)
	    );
	return val;
}

/* CON5---------------------------------------------------- */

void bq25890_set_iprechg(unsigned int val)
{
	unsigned int ret = 0;


	ret = bq25890_config_interface((unsigned char) (bq25890_CON5),
				       (val),
				       (unsigned char) (CON5_IPRECHG_MASK),
				       (unsigned char) (CON5_IPRECHG_SHIFT)
	    );

}

void bq25890_set_iterml(unsigned int val)
{
	unsigned int ret = 0;


	ret = bq25890_config_interface((unsigned char) (bq25890_CON5),
				       (val),
				       (unsigned char) (CON5_ITERM_MASK), (unsigned char) (CON5_ITERM_SHIFT)
	    );

}



/* CON6---------------------------------------------------- */

void bq25890_set_vreg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_VREG_MASK),
				       (unsigned char) (CON6_VREG_SHIFT)
	    );
}

unsigned int bq25890_get_vreg(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON6),
				     (&val),
				     (unsigned char) (CON6_VREG_MASK), (unsigned char) (CON6_VREG_SHIFT)
	    );
	return val;
}

void bq25890_set_batlowv(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_BATLOWV_MASK),
				       (unsigned char) (CON6_BATLOWV_SHIFT)
	    );
}

void bq25890_set_vrechg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_VRECHG_MASK),
				       (unsigned char) (CON6_VRECHG_SHIFT)
	    );
}

/* CON7---------------------------------------------------- */


void bq25890_en_term_chg(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_EN_TERM_CHG_MASK),
				       (unsigned char) (CON7_EN_TERM_CHG_SHIFT)
	    );
}

void bq25890_en_state_dis(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_STAT_DIS_MASK),
				       (unsigned char) (CON7_STAT_DIS_SHIFT)
	    );
}

void bq25890_set_wd_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_WTG_TIM_SET_MASK),
				       (unsigned char) (CON7_WTG_TIM_SET_SHIFT)
	    );
}

void bq25890_en_chg_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_EN_TIMER_MASK),
				       (unsigned char) (CON7_EN_TIMER_SHIFT)
	    );
}

unsigned int bq25890_get_chg_timer_enable(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON7),
				     &val,
				     (unsigned char) (CON7_EN_TIMER_MASK),
				     (unsigned char) (CON7_EN_TIMER_SHIFT));

	return val;
}

void bq25890_set_chg_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_SET_CHG_TIM_MASK),
				       (unsigned char) (CON7_SET_CHG_TIM_SHIFT)
	    );
}

/* CON8--------------------------------------------------- */
void bq25890_set_thermal_regulation(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON8),
				       (unsigned char) (val),
				       (unsigned char) (CON8_TREG_MASK), (unsigned char) (CON8_TREG_SHIFT)
	    );
}

void bq25890_set_VBAT_clamp(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON8),
				       (unsigned char) (val),
				       (unsigned char) (CON8_VCLAMP_MASK),
				       (unsigned char) (CON8_VCLAMP_SHIFT)
	    );
}

void bq25890_set_VBAT_IR_compensation(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON8),
				       (unsigned char) (val),
				       (unsigned char) (CON8_BAT_COMP_MASK),
				       (unsigned char) (CON8_BAT_COMP_SHIFT)
	    );
}

/* CON9---------------------------------------------------- */
void bq25890_pumpx_up(unsigned int val)
{
	unsigned int ret = 0;

	bq25890_en_pumpx(1);
	if (val == 1) {
		ret = bq25890_config_interface((unsigned char) (bq25890_CON9),
					       (unsigned char) (1),
					       (unsigned char) (CON9_PUMPX_UP),
					       (unsigned char) (CON9_PUMPX_UP_SHIFT)
		    );
	} else {
		ret = bq25890_config_interface((unsigned char) (bq25890_CON9),
					       (unsigned char) (1),
					       (unsigned char) (CON9_PUMPX_DN),
					       (unsigned char) (CON9_PUMPX_DN_SHIFT)
		    );
	}

	/* Input current limit = 500 mA, changes after PE+ detection */
	bq25890_set_iinlim(0x08);

	/* CC mode current = 2048 mA */
	bq25890_set_ichg(0x20);

	msleep(3000);
}

void bq25890_set_force_ico(void)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CON9),
				       (unsigned char) (1),
				       (unsigned char) (FORCE_ICO_MASK),
				       (unsigned char) (FORCE_ICO__SHIFT)
	    );
}

/* CONA---------------------------------------------------- */
void bq25890_set_boost_ilim(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CONA),
				       (unsigned char) (val),
				       (unsigned char) (CONA_BOOST_ILIM_MASK),
				       (unsigned char) (CONA_BOOST_ILIM_SHIFT)
	    );
}

void bq25890_set_boost_vlim(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_CONA),
				       (unsigned char) (val),
				       (unsigned char) (CONA_BOOST_VLIM_MASK),
				       (unsigned char) (CONA_BOOST_VLIM_SHIFT)
	    );
}

/* CONB---------------------------------------------------- */


unsigned int bq25890_get_vbus_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONB),
				     (&val),
				     (unsigned char) (CONB_VBUS_STAT_MASK),
				     (unsigned char) (CONB_VBUS_STAT_SHIFT)
	    );
	return val;
}


unsigned int bq25890_get_chrg_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONB),
				     (&val),
				     (unsigned char) (CONB_CHRG_STAT_MASK),
				     (unsigned char) (CONB_CHRG_STAT_SHIFT)
	    );
	return val;
}

unsigned int bq25890_get_pg_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONB),
				     (&val),
				     (unsigned char) (CONB_PG_STAT_MASK),
				     (unsigned char) (CONB_PG_STAT_SHIFT)
	    );
	return val;
}



unsigned int bq25890_get_sdp_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONB),
				     (&val),
				     (unsigned char) (CONB_SDP_STAT_MASK),
				     (unsigned char) (CONB_SDP_STAT_SHIFT)
	    );
	return val;
}

unsigned int bq25890_get_vsys_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONB),
				     (&val),
				     (unsigned char) (CONB_VSYS_STAT_MASK),
				     (unsigned char) (CONB_VSYS_STAT_SHIFT)
	    );
	return val;
}

/* CON0C---------------------------------------------------- */
unsigned int bq25890_get_wdt_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONC),
				     (&val),
				     (unsigned char) (CONB_WATG_STAT_MASK),
				     (unsigned char) (CONB_WATG_STAT_SHIFT)
	    );
	return val;
}

unsigned int bq25890_get_boost_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONC),
				     (&val),
				     (unsigned char) (CONB_BOOST_STAT_MASK),
				     (unsigned char) (CONB_BOOST_STAT_SHIFT)
	    );
	return val;
}

unsigned int bq25890_get_chrg_fault_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONC),
				     (&val),
				     (unsigned char) (CONC_CHRG_FAULT_MASK),
				     (unsigned char) (CONC_CHRG_FAULT_SHIFT)
	    );
	return val;
}

unsigned int bq25890_get_bat_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONC),
				     (&val),
				     (unsigned char) (CONB_BAT_STAT_MASK),
				     (unsigned char) (CONB_BAT_STAT_SHIFT)
	    );
	return val;
}


/* COND */
void bq25890_set_force_vindpm(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_COND),
				       (unsigned char) (val),
				       (unsigned char) (COND_FORCE_VINDPM_MASK),
				       (unsigned char) (COND_FORCE_VINDPM_SHIFT)
	    );
}

void bq25890_set_vindpm(unsigned int val)
{
	unsigned int ret = 0;

	ret = bq25890_config_interface((unsigned char) (bq25890_COND),
				       (unsigned char) (val),
				       (unsigned char) (COND_VINDPM_MASK),
				       (unsigned char) (COND_VINDPM_SHIFT)
	    );
}

unsigned int bq25890_get_vindpm(void)
{
	int ret = 0;
	unsigned char val = 0;


	ret = bq25890_read_interface((unsigned char) (bq25890_COND),
				     (&val),
				     (unsigned char) (COND_VINDPM_MASK),
				     (unsigned char) (COND_VINDPM_SHIFT));
	return val;
}

/* CONDE */
unsigned int bq25890_get_vbat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CONE),
				     (&val),
				     (unsigned char) (CONE_VBAT_MASK), (unsigned char) (CONE_VBAT_SHIFT)
	    );
	return val;
}

/* CON11 */
unsigned int bq25890_get_vbus(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON11),
				     (&val),
				     (unsigned char) (CON11_VBUS_MASK), (unsigned char) (CON11_VBUS_SHIFT)
	    );
	return val;
}

/* CON12 */
unsigned int bq25890_get_ichg(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON12),
				     (&val),
				     (unsigned char) (CONB_ICHG_STAT_MASK),
				     (unsigned char) (CONB_ICHG_STAT_SHIFT)
	    );
	return val;
}



/* CON13 /// */


unsigned int bq25890_get_idpm_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON13),
				     (&val),
				     (unsigned char) (CON13_IDPM_STAT_MASK),
				     (unsigned char) (CON13_IDPM_STAT_SHIFT)
	    );
	return val;
}

unsigned int bq25890_get_vdpm_state(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface((unsigned char) (bq25890_CON13),
				     (&val),
				     (unsigned char) (CON13_VDPM_STAT_MASK),
				     (unsigned char) (CON13_VDPM_STAT_SHIFT)
	    );
	return val;
}




/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void bq25890_hw_component_detect(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = bq25890_read_interface(0x03, &val, 0xFF, 0x0);

	if (val == 0)
		g_bq25890_hw_exist = 0;
	else
		g_bq25890_hw_exist = 1;

	pr_debug("[bq25890_hw_component_detect] exist=%d, Reg[0x03]=0x%x\n",
		 g_bq25890_hw_exist, val);
}

int is_bq25890_exist(void)
{
	pr_debug("[is_bq25890_exist] g_bq25890_hw_exist=%d\n", g_bq25890_hw_exist);

	return g_bq25890_hw_exist;
}

void bq25890_dump_register(void)
{
	unsigned char i = 0;
	unsigned char ichg = 0;
	unsigned char ichg_reg = 0;
	unsigned char iinlim = 0;
	unsigned char vbat = 0;
	unsigned char chrg_state = 0;
	unsigned char chr_en = 0;
	unsigned char vbus = 0;
	unsigned char vdpm = 0;
	unsigned char fault = 0;

	if (Enable_BATDRV_LOG > BAT_LOG_CRTI) {
		bq25890_ADC_start(1);
		for (i = 0; i < bq25890_REG_NUM; i++) {
			bq25890_read_byte(i, &bq25890_reg[i]);
			battery_log(BAT_LOG_FULL, "[bq25890 reg@][0x%x]=0x%x ", i, bq25890_reg[i]);
		}
	}
	bq25890_ADC_start(1);
	iinlim = bq25890_get_iinlim();
	chrg_state = bq25890_get_chrg_state();
	chr_en = bq25890_get_chg_en();
	ichg_reg = bq25890_get_reg_ichg();
	ichg = bq25890_get_ichg();
	vbat = bq25890_get_vbat();
	vbus = bq25890_get_vbus();
	vdpm = bq25890_get_vdpm_state();
	fault = bq25890_get_chrg_fault_state();
	battery_log(BAT_LOG_CRTI,
	"[PE+]Ibat=%d, Ilim=%d, Vbus=%d, err=%d, Ichg=%d, Vbat=%d, ChrStat=%d, CHGEN=%d, VDPM=%d\n",
	ichg_reg * 64, iinlim * 50 + 100, vbus * 100 + 2600, fault,
		    ichg * 50, vbat * 20 + 2304, chrg_state, chr_en, vdpm);

}

void bq25890_hw_init(void)
{
	/*battery_log(BAT_LOG_CRTI, "[bq25890_hw_init] After HW init\n");*/
	bq25890_dump_register();
}

static int bq25890_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	battery_log(BAT_LOG_CRTI, "[bq25890_driver_probe]\n");

	new_client = client;

	/* --------------------- */
	bq25890_hw_component_detect();
	bq25890_dump_register();
	/* bq25890_hw_init(); //move to charging_hw_xxx.c */
	chargin_hw_init_done = true;

	/* Hook chr_control_interface with battery's interface */
	battery_charging_control = chr_control_interface;
	return 0;
}

/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/
unsigned char g_reg_value_bq25890;
static ssize_t show_bq25890_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	battery_log(BAT_LOG_CRTI, "[show_bq25890_access] 0x%x\n", g_reg_value_bq25890);
	return sprintf(buf, "%u\n", g_reg_value_bq25890);
}

static ssize_t store_bq25890_access(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	battery_log(BAT_LOG_CRTI, "[store_bq25890_access]\n");

	if (buf != NULL && size != 0) {
		battery_log(BAT_LOG_CRTI, "[store_bq25890_access] buf is %s and size is %zu\n", buf,
			    size);

		pvalue = (char *)buf;
		if (size > 3) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);

		if (size > 3) {
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			battery_log(BAT_LOG_CRTI,
				    "[store_bq25890_access] write bq25890 reg 0x%x with value 0x%x !\n",
				    (unsigned int) reg_address, reg_value);
			ret = bq25890_config_interface(reg_address, reg_value, 0xFF, 0x0);
		} else {
			ret = bq25890_read_interface(reg_address, &g_reg_value_bq25890, 0xFF, 0x0);
			battery_log(BAT_LOG_CRTI,
				    "[store_bq25890_access] read bq25890 reg 0x%x with value 0x%x !\n",
				    (unsigned int) reg_address, g_reg_value_bq25890);
			battery_log(BAT_LOG_CRTI,
				    "[store_bq25890_access] Please use \"cat bq25890_access\" to get value\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(bq25890_access, 0664, show_bq25890_access, store_bq25890_access);	/* 664 */

static int bq25890_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	battery_log(BAT_LOG_CRTI, "******** bq25890_user_space_probe!! ********\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_bq25890_access);

	return 0;
}

struct platform_device bq25890_user_space_device = {
	.name = "bq25890-user",
	.id = -1,
};

static struct platform_driver bq25890_user_space_driver = {
	.probe = bq25890_user_space_probe,
	.driver = {
		   .name = "bq25890-user",
		   },
};

#ifdef CONFIG_OF
static const struct of_device_id bq25890_of_match[] = {
	{.compatible = "mediatek,swithing_charger"},
	{},
};
#else
static struct i2c_board_info i2c_bq25890 __initdata = {
	I2C_BOARD_INFO("bq25890", (bq25890_SLAVE_ADDR_WRITE >> 1))
};
#endif

static struct i2c_driver bq25890_driver = {
	.driver = {
		   .name = "bq25890",
#ifdef CONFIG_OF
		   .of_match_table = bq25890_of_match,
#endif
		   },
	.probe = bq25890_driver_probe,
	.id_table = bq25890_i2c_id,
};

static int __init bq25890_init(void)
{
	int ret = 0;

	/* i2c registeration using DTS instead of boardinfo*/
#ifdef CONFIG_OF
	battery_log(BAT_LOG_CRTI, "[bq25890_init] init start with i2c DTS");
#else
	battery_log(BAT_LOG_CRTI, "[bq25890_init] init start. ch=%d\n", bq25890_BUSNUM);
	i2c_register_board_info(bq25890_BUSNUM, &i2c_bq25890, 1);
#endif
	if (i2c_add_driver(&bq25890_driver) != 0) {
		battery_log(BAT_LOG_CRTI,
			    "[bq25890_init] failed to register bq25890 i2c driver.\n");
	} else {
		battery_log(BAT_LOG_CRTI,
			    "[bq25890_init] Success to register bq25890 i2c driver.\n");
	}

	/* bq25890 user space access interface */
	ret = platform_device_register(&bq25890_user_space_device);
	if (ret) {
		battery_log(BAT_LOG_CRTI, "****[bq25890_init] Unable to device register(%d)\n",
			    ret);
		return ret;
	}
	ret = platform_driver_register(&bq25890_user_space_driver);
	if (ret) {
		battery_log(BAT_LOG_CRTI, "****[bq25890_init] Unable to register driver (%d)\n",
			    ret);
		return ret;
	}

	return 0;
}

static void __exit bq25890_exit(void)
{
	i2c_del_driver(&bq25890_driver);
}
module_init(bq25890_init);
module_exit(bq25890_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C bq25890 Driver");
MODULE_AUTHOR("will cai <will.cai@mediatek.com>");
