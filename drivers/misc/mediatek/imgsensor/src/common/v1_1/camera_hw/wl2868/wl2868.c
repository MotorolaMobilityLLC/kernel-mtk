/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include "wl2868.h"
//#include "../../../../../../camera_ldo/camera_ldo.h"
//#include "camera_ldo.h"
//extern void camera_ldo_set_ldo_value(CAMERA_LDO_SELECT ldonum,unsigned int value);
//extern void camera_ldo_set_en_ldo(CAMERA_LDO_SELECT ldonum,unsigned int en);
#include "imgsensor_hw.h"
#include "imgsensor_common.h"

typedef enum  {
	EXTLDO_REGULATOR_VOLTAGE_0    = 0,
	EXTLDO_REGULATOR_VOLTAGE_1000 =1000,
	EXTLDO_REGULATOR_VOLTAGE_1100=1100,
	EXTLDO_REGULATOR_VOLTAGE_1200=1200,
	EXTLDO_REGULATOR_VOLTAGE_1210=1210,
	EXTLDO_REGULATOR_VOLTAGE_1220=1220,
	EXTLDO_REGULATOR_VOLTAGE_1500=1500,
	EXTLDO_REGULATOR_VOLTAGE_1800=1800,
	EXTLDO_REGULATOR_VOLTAGE_2200=2200,
	EXTLDO_REGULATOR_VOLTAGE_2500=2500,
	EXTLDO_REGULATOR_VOLTAGE_2800=2800,
	EXTLDO_REGULATOR_VOLTAGE_2900=2900,
}EXTLDO_REGULATOR_VOLTAGE;

struct WL2868_LDOMAP{
	enum IMGSENSOR_SENSOR_IDX idx;
	enum IMGSENSOR_HW_PIN hwpin;
	CAMERA_LDO_SELECT wl2868ldo;
};
/*
typedef enum {
	CAMERA_LDO_DVDD1=0,
	CAMERA_LDO_DVDD2,
	CAMERA_LDO_VDDAF,
	CAMERA_LDO_AVDD1,
	CAMERA_LDO_AVDD3,
	CAMERA_LDO_AVDD2,
	CAMERA_LDO_VDDIO,//CAMERA_LDO_VDDIO
	CAMERA_LDO_MAX
} CAMERA_LDO_SELECT;
*/
/*

enum IMGSENSOR_RETURN imgsensor_hw_wl2868_open(
	struct IMGSENSOR_HW_DEVICE **pdevice);
*/
struct WL2868_LDOMAP  ldolist[] = {
	{IMGSENSOR_SENSOR_IDX_MAIN,   DVDD,    CAMERA_LDO_DVDD1},//for main(DVDD)
	{IMGSENSOR_SENSOR_IDX_SUB,    DVDD,    CAMERA_LDO_DVDD2},//for main3(AVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN2,  DVDD,    CAMERA_LDO_DVDD2},//for main2(DOVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN,   AFVDD,   CAMERA_LDO_VDDAF},//for sub2(AVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN,   DOVDD,   CAMERA_LDO_VDDIO},//for main(DOVDD)
	{IMGSENSOR_SENSOR_IDX_SUB,    DOVDD,   CAMERA_LDO_VDDIO},//for sub(DOVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN2,  DOVDD,   CAMERA_LDO_VDDIO},//for main2(DOVDD)
	{IMGSENSOR_SENSOR_IDX_SUB2,   DOVDD,   CAMERA_LDO_VDDIO},//for sub2(DOVDD)
	{IMGSENSOR_SENSOR_IDX_SUB2,   AVDD,    CAMERA_LDO_AVDD3},//for sub(AVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN2,  AVDD,    CAMERA_LDO_AVDD3},//for sub(AVDD)
	{IMGSENSOR_SENSOR_IDX_SUB,    AVDD,    CAMERA_LDO_AVDD2},//for sub(DVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN,   AVDD,    CAMERA_LDO_AVDD1},//for main2(DVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN3,  AVDD,    CAMERA_LDO_AVDD3},//for main3(DVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN3,  DOVDD,   CAMERA_LDO_VDDIO},//for main3(DOVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN4,  AVDD,    CAMERA_LDO_AVDD3},//for main3(DVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN4,  DOVDD,   CAMERA_LDO_VDDIO},//for main3(DOVDD)
	{IMGSENSOR_SENSOR_IDX_MAIN4,  DVDD,    CAMERA_LDO_DVDD2},//for main2(DOVDD)	
};

static const int extldo_regulator_voltage[] = {
	EXTLDO_REGULATOR_VOLTAGE_0,
	EXTLDO_REGULATOR_VOLTAGE_1000,
	EXTLDO_REGULATOR_VOLTAGE_1100,
	EXTLDO_REGULATOR_VOLTAGE_1200,
	EXTLDO_REGULATOR_VOLTAGE_1210,
	EXTLDO_REGULATOR_VOLTAGE_1220,
	EXTLDO_REGULATOR_VOLTAGE_1500,
	EXTLDO_REGULATOR_VOLTAGE_1800,
	EXTLDO_REGULATOR_VOLTAGE_2200,
	EXTLDO_REGULATOR_VOLTAGE_2500,
	EXTLDO_REGULATOR_VOLTAGE_2800,
	EXTLDO_REGULATOR_VOLTAGE_2900,
};


static enum IMGSENSOR_RETURN wl2868_init(void *instance,struct IMGSENSOR_HW_DEVICE_COMMON *pcommon)
{
    pr_err("sss123 --->inter wl2868_init\n");
	return IMGSENSOR_RETURN_SUCCESS;
}
static enum IMGSENSOR_RETURN wl2868_release(void *instance)
{
    pr_err("sss123 --->inter wl2868_release\n");
	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN wl2868_set(
	void *pinstance,
	enum IMGSENSOR_SENSOR_IDX   sensor_idx,
	enum IMGSENSOR_HW_PIN       pin,
	enum IMGSENSOR_HW_PIN_STATE pin_state)
{
int i,ret;
pr_err("%s stoneadd hwpin=%d idx=%d pinstate=%d\n", __func__, pin,sensor_idx, pin_state);

pr_err("sss123 --->inter wl2868_set\n");

if (pin < IMGSENSOR_HW_PIN_AVDD ||
	   pin > IMGSENSOR_HW_PIN_DOVDD ||
	   pin_state < IMGSENSOR_HW_PIN_STATE_LEVEL_0 ||
	   pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH)
		ret = IMGSENSOR_RETURN_ERROR;

for(i=0;i<(sizeof(ldolist)/sizeof(ldolist[0]));i++)
{

	if ((pin == ldolist[i].hwpin) && (sensor_idx == ldolist[i].idx))
	{
	     	pr_err("%s stoneadd got the wl2868 ldo(%d) to %d mV ! \n",__func__, ldolist[i].wl2868ldo,extldo_regulator_voltage[pin_state-EXTLDO_REGULATOR_VOLTAGE_0]);

		if(pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_0)
		{
			pr_err("%s stoneadd power on ++++++wl2868  ldo(%d) to  %d mV !\n",__func__,
					ldolist[i].wl2868ldo,
					extldo_regulator_voltage[pin_state-EXTLDO_REGULATOR_VOLTAGE_0]);

			camera_ldo_set_ldo_value(ldolist[i].wl2868ldo,extldo_regulator_voltage[pin_state-EXTLDO_REGULATOR_VOLTAGE_0]);
			camera_ldo_set_en_ldo(ldolist[i].wl2868ldo,1);
		}
		else
		{
			pr_err("%s stoneadd power off ++++++wl2868  ldo(%d) \n",__func__, ldolist[i].wl2868ldo);
			camera_ldo_set_en_ldo(ldolist[i].wl2868ldo,0);
		}
		break;
	}
}
ret = IMGSENSOR_RETURN_SUCCESS;

	return ret;
}

static struct IMGSENSOR_HW_DEVICE device = {
	.init      = wl2868_init,
	.set       = wl2868_set,
	.release   = wl2868_release,
	.id        = IMGSENSOR_HW_ID_WL2868
};

enum IMGSENSOR_RETURN imgsensor_hw_wl2868_open(
	struct IMGSENSOR_HW_DEVICE **pdevice)
{
	*pdevice = &device;
	pr_err("sss123 --->inter imgsensor_hw_wl2868_open\n");
	return IMGSENSOR_RETURN_SUCCESS;
}

