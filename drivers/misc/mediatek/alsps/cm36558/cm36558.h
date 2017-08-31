/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*
 * Definitions for CM36558 als/ps sensor chip.
 */
#ifndef __CM36558_H__
#define __CM36558_H__

#include <linux/ioctl.h>

/*CM36558 als/ps sensor register related macro*/
#define CM36558_REG_ALS_UV_CONF		0X00
#define CM36558_REG_ALS_THDH		0X01
#define CM36558_REG_ALS_THDL		0X02
#define CM36558_REG_PS_CONF1_2		0X03
#define CM36558_REG_PS_CONF3_MS		0X04
#define CM36558_REG_PS_CANC			0X05
#define CM36558_REG_PS_THDL			0X06
#define CM36558_REG_PS_THDH			0X07
#define CM36558_REG_PS_DATA			0X08
#define CM36558_REG_ALS_DATA		0X09
#define CM36558_REG_UVAS_DATA		0X0B
#define CM36558_REG_UVBS_DATA		0X0C
#define CM36558_REG_INT_FLAG		0X0D
#define CM36558_REG_ID				0X0E

/*CM36558 related driver tag macro*/
#define CM36558_SUCCESS						0
#define CM36558_ERR_I2C						-1
#define CM36558_ERR_STATUS					-3
#define CM36558_ERR_SETUP_FAILURE			-4
#define CM36558_ERR_GETGSENSORDATA			-5
#define CM36558_ERR_IDENTIFICATION			-6

/*----------------------------------------------------------------------------*/
enum CM36558_NOTIFY_TYPE {
	CM36558_NOTIFY_PROXIMITY_CHANGE = 1,
};
/*----------------------------------------------------------------------------*/
enum CM36558_CUST_ACTION {
	CM36558_CUST_ACTION_SET_CUST = 1,
	CM36558_CUST_ACTION_CLR_CALI,
	CM36558_CUST_ACTION_SET_CALI,
	CM36558_CUST_ACTION_SET_PS_THRESHODL,
	CM36558_CUST_ACTION_SET_EINT_INFO,
	CM36558_CUST_ACTION_GET_ALS_RAW_DATA,
	CM36558_CUST_ACTION_GET_PS_RAW_DATA,
};
/*----------------------------------------------------------------------------*/
struct CM36558_CUST {
	uint16_t action;
};
/*----------------------------------------------------------------------------*/
struct CM36558_SET_CUST {
	uint16_t action;
	uint16_t part;
	int32_t data[0];
};
/*----------------------------------------------------------------------------*/
struct CM36558_SET_CALI {
	uint16_t action;
	int32_t cali;
};
/*----------------------------------------------------------------------------*/
struct CM36558_SET_PS_THRESHOLD {
	uint16_t action;
	int32_t threshold[2];
};
/*----------------------------------------------------------------------------*/
struct CM36558_SET_EINT_INFO {
	uint16_t action;
	uint32_t gpio_pin;
	uint32_t gpio_mode;
	uint32_t eint_num;
	uint32_t eint_is_deb_en;
	uint32_t eint_type;
};
/*----------------------------------------------------------------------------*/
struct CM36558_GET_ALS_RAW_DATA {
	uint16_t action;
	uint16_t als;
};
/*----------------------------------------------------------------------------*/
struct CM36558_GET_PS_RAW_DATA {
	uint16_t action;
	uint16_t ps;
};
/*----------------------------------------------------------------------------*/
union CM36558_CUST_DATA {
	uint32_t data[10];
	struct CM36558_CUST cust;
	struct CM36558_SET_CUST setCust;
	struct CM36558_CUST clearCali;
	struct CM36558_SET_CALI setCali;
	struct CM36558_SET_PS_THRESHOLD setPSThreshold;
	struct CM36558_SET_EINT_INFO setEintInfo;
	struct CM36558_GET_ALS_RAW_DATA getALSRawData;
	struct CM36558_GET_PS_RAW_DATA getPSRawData;
};
/*----------------------------------------------------------------------------*/

extern struct platform_device *get_alsps_platformdev(void);
#endif
