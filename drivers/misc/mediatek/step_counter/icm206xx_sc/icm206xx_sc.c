/*
 * ICM206XX sensor driver
 * Copyright (C) 2016 Invensense, Inc.
 *
 *
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

#include <linux/ioctl.h>
#include "../step_counter.h"
#include "../../accelerometer/icm206xx_a/icm206xx_register_20608D.h"
#include "../../accelerometer/icm206xx_a/icm206xx_share.h"
#include "icm206xx_sc.h"

static atomic_t icm206xx_sc_trace;

static int icm206xx_sc_init_flag =  -1;

static int icm206xx_sc_local_init(void);
static int icm206xx_sc_remove(void);
static struct step_c_init_info icm206xx_sc_init_info = {
	.name = "ICM206XX_SC",
	.init = icm206xx_sc_local_init,
	.uninit = icm206xx_sc_remove,
};

#ifdef ICM206XX_DMP_DOWNLOAD_VERIFY
static unsigned char dmpVerify[ICM206XX_DMP_CODE_SIZE] = { 0 };
#endif

/*=======================================================================================*/
/* I2C Primitive Functions Section							 */
/*=======================================================================================*/

/* Share i2c function with Accelerometer		*/
/* Function is defined in accelerometer/icm206xx_a	*/

/*=======================================================================================*/
/* Vendor Specific Functions Section							 */
/*=======================================================================================*/

static int icm206xx_sc_WriteDMPImage(void)
{
	int res = 0;
	int nSize = ICM206XX_DMP_CODE_SIZE + ICM206XX_DMP_CODE_START_LOC;
	int nWritten = 0;
	int nTryToWrite = ICM206XX_DMP_CODE_BANK_SIZE;

	res = icm206xx_share_i2c_write_memory(ICM206XX_DMP_CODE_START_LOC,
					nTryToWrite - ICM206XX_DMP_CODE_START_LOC,
					&dmpMemory[0]);
	if (res < 0) {
		STEP_C_ERR("memory write error!!\n");
		return ICM206XX_ERR_I2C;
	}

	nWritten = ICM206XX_DMP_CODE_BANK_SIZE;

	while (nWritten < nSize) {
		if (nSize - nWritten < ICM206XX_DMP_CODE_BANK_SIZE)
			nTryToWrite = nSize - nWritten;

		res = icm206xx_share_i2c_write_memory(nWritten,
						nTryToWrite,
						&dmpMemory[nWritten - ICM206XX_DMP_CODE_START_LOC]);
		if (res < 0) {
			STEP_C_ERR("memory write error!!\n");
			return ICM206XX_ERR_I2C;
		}

		nWritten += nTryToWrite;
	}

	return res;
}

#ifdef ICM206XX_DMP_DOWNLOAD_VERIFY
static int icm206xx_sc_ReadDMPImage(void)
{
	int res = 0;
	int nSize = ICM206XX_DMP_CODE_SIZE + ICM206XX_DMP_CODE_START_LOC;
	int nRead = 0;
	int nTryToRead = ICM206XX_DMP_CODE_BANK_SIZE;

	res = icm206xx_share_i2c_read_memory(ICM206XX_DMP_CODE_START_LOC,
					nTryToRead - ICM206XX_DMP_CODE_START_LOC,
					&dmpVerify[0]);
	if (res < 0) {
		STEP_C_ERR("memory read error!!\n");
		return ICM206XX_ERR_I2C;
	}

	nRead = ICM206XX_DMP_CODE_BANK_SIZE;

	while (nRead < nSize) {
		if (nSize - nRead < ICM206XX_DMP_CODE_BANK_SIZE)
			nTryToRead = nSize - nRead;

		res = icm206xx_share_i2c_read_memory(nRead,
						nTryToRead,
						&dmpVerify[nRead - ICM206XX_DMP_CODE_START_LOC]);
		if (res < 0) {
			STEP_C_ERR("memory read error!!\n");
			return ICM206XX_ERR_I2C;
		}

		nRead += nTryToRead;
	}

	return res;
}

static int icm206xx_sc_VerifyDMPImage(void)
{
	int i;

	for (i = 0; i < ICM206XX_DMP_CODE_SIZE; i++) {
		if (dmpVerify[i] != dmpMemory[i])
			return ICM206XX_ERR_STATUS;
	}

	return ICM206XX_SUCCESS;
}
#endif

static int icm206xx_sc_setDMPStartAddress(int addr)
{
	int res = 0;

	u8 databuf[3] = { 0 };

	databuf[0] = (addr >> 8);
	databuf[1] = (addr & 0xFF);

	res = icm206xx_share_i2c_write_register(ICM206XX_REG_PRGM_START_ADDRH, databuf, 2);
	if (res < 0) {
		STEP_C_ERR("set DMP start address by write START_ADDRH register err!\n");
		return ICM206XX_ERR_I2C;
	}

	return res;
}

static int icm206xx_sc_startDMP(void)
{
	int res = 0;
	u8 databuf[2] = {0};

	icm206xx_sc_setDMPStartAddress(ICM206XX_DMP_START_ADDRESS);

	databuf[0] = 0x80;
	res = icm206xx_share_i2c_write_register(ICM206XX_REG_USER_CTL, databuf, 1);
	if (res < 0) {
		STEP_C_ERR("start DMP by write USER_CTL register err!\n");
		return ICM206XX_ERR_I2C;
	}

	return res;
}

static int icm206xx_sc_stopDMP(void)
{
	int res = 0;
	u8 databuf[2] = {0};

	databuf[0] = 0x00;
	res = icm206xx_share_i2c_write_register(ICM206XX_REG_USER_CTL, databuf, 1);
	if (res < 0) {
		STEP_C_ERR("stop DMP by write USER_CTL register err!\n");
		return ICM206XX_ERR_I2C;
	}

	return res;
}

static int icm206xx_sc_EnableStepCounter(u8 en)
{
	int res = 0;
	u8 databuf[2] = {0};

	icm206xx_sc_stopDMP();

	if (en == 1)
		databuf[0] = 0xF1;	/* enable */
	else
		databuf[0] = 0xFF;	/* disable */

	res = icm206xx_share_i2c_write_memory(ICM206XX_KEY_CFG_PED_ENABLE, 1, &databuf[0]);
	if (res < 0) {
		STEP_C_ERR("enable/disable pedometer err!\n");
		return ICM206XX_ERR_I2C;
	}

	/* Start DMP when step count is enabled */
	/* Doesn't need to start DMP when step count is disabled because DMP has only Step Counter */
	if (en == 1)
		icm206xx_sc_startDMP();

	return res;
}

static int icm206xx_sc_ReadSensorData(uint32_t *value)
{
	int res = 0;
	u32 d = 0;

	res = icm206xx_share_i2c_read_memory(ICM206XX_KEY_D_PEDSTD_STEPCTR, 4, (u8 *)(&d));
	if (res < 0) {
		STEP_C_ERR("read Step Counter err!\n");
		return ICM206XX_ERR_I2C;
	}

	*value = be32_to_cpup((__be32 *)(&d));

	STEP_C_LOG("Step Counter Data - %04x\n", (*value));

	return res;
}
/*----------------------------------------------------------------------------*/
static int icm206xx_sc_init_client(bool enable)
{
	int res = 0;

	/* Exit sleep mode */
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Stop DMP */
	res = icm206xx_sc_stopDMP();
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Download DMP firmware  */
	res = icm206xx_sc_WriteDMPImage();
	if (res != ICM206XX_SUCCESS)
		return res;

#ifdef ICM206XX_DMP_DOWNLOAD_VERIFY
	/* Read DMP firmware  */
	res = icm206xx_sc_ReadDMPImage();
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Compare and verify DMP firmware  */
	res = icm206xx_sc_VerifyDMPImage();
	if (res != ICM206XX_SUCCESS)
		return res;

#endif

	/* Set 5ms(200hz) sample rate - fixed rate for DMP */
	res = icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_STEP_COUNTER, 5000000, false);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Disable sensor - standby mode for step_counter(actually accelerometer) */
	res = icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_STEP_COUNTER, enable);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set power mode - sleep or normal */
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, enable);
	if (res != ICM206XX_SUCCESS)
		return res;

	STEP_C_LOG("icm206xx_sc_init_client OK!\n");

	return ICM206XX_SUCCESS;
}

/*=======================================================================================*/
/* Debug Only Functions Section								 */
/*=======================================================================================*/

#ifdef ICM206XX_DMP_DOWNLOAD_VERIFY
static char gstr_ReadDump[1024];

static void debug_ReadDump(void)
{
	int i;

	for (i = 0; i < 14; i++) {
		sprintf(&gstr_ReadDump[i * 48], "%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x\n",
				dmpVerify[i*16+0],
				dmpVerify[i*16+1],
				dmpVerify[i*16+2],
				dmpVerify[i*16+3],
				dmpVerify[i*16+4],
				dmpVerify[i*16+5],
				dmpVerify[i*16+6],
				dmpVerify[i*16+7],
				dmpVerify[i*16+8],
				dmpVerify[i*16+9],
				dmpVerify[i*16+10],
				dmpVerify[i*16+11],
				dmpVerify[i*16+12],
				dmpVerify[i*16+13],
				dmpVerify[i*16+14],
				dmpVerify[i*16+15]);
	}
}
#endif

/*=======================================================================================*/
/* Driver Attribute Section								 */
/*=======================================================================================*/

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&icm206xx_sc_trace));

	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;

	if (!kstrtoint(buf, 16, &trace))
		atomic_set(&icm206xx_sc_trace, trace);
	else
		STEP_C_ERR("invalid content: '%s', length = %d\n", buf, (int)count);

	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;

#ifdef ICM206XX_DMP_DOWNLOAD_VERIFY
	debug_ReadDump();
	res = snprintf(buf, PAGE_SIZE, "%s\n", gstr_ReadDump);
#endif
	return res;
}

#ifdef ICM206XX_DMP_STEP_COUNTER_TUNE
static ssize_t show_sb_threshold(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;

	int res = 0;
	u16 d = 0;
	uint16_t value;

	icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, true);

	res = icm206xx_share_i2c_read_memory(D_PEDSTD_SB, 2, (u8 *)(&d));
	if (res < 0) {
		STEP_C_ERR("read Step Buffer err!\n");
		return 0;
	}

	value = be16_to_cpup((__be16 *)(&d));
	len = snprintf(buf, PAGE_SIZE, "0x%02X\n", value);

	return len;
}

static ssize_t store_sb_threshold(struct device_driver *ddri, const char *buf, size_t count)
{
	int value;

	int res = 0;
	u8 databuf[3] = {0};

	icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, true);

	if (kstrtoint(buf, 16, &value)) {
		STEP_C_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
		return count;
	}

	databuf[0] = (value >> 8);
	databuf[1] = (value & 0xFF);

	res = icm206xx_share_i2c_write_memory(D_PEDSTD_SB, 2, &databuf[0]);
	if (res < 0) {
		STEP_C_ERR("write Step Buffer err!\n");
		return 0;
	}

	return count;
}

static ssize_t show_peak_threshold(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;

	int res = 0;
	u32 d = 0;
	uint32_t value;

	icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, true);

	res = icm206xx_share_i2c_read_memory(D_PEDSTD_PEAKTHRSH, 4, (u8 *)(&d));
	if (res < 0) {
		STEP_C_ERR("read Peak threshold err!\n");
		return 0;
	}

	value = be32_to_cpup((__be32 *)(&d));
	len = snprintf(buf, PAGE_SIZE, "0x%08X\n", value);

	return len;
}

static ssize_t store_peak_threshold(struct device_driver *ddri, const char *buf, size_t count)
{
	int value;

	int res = 0;
	u8 databuf[5] = {0};

	icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, true);

	if (kstrtoint(buf, 16, &value)) {
		STEP_C_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
		return count;
	}

	databuf[0] = (value >> 24);
	databuf[1] = (value >> 16);
	databuf[2] = (value >> 8);
	databuf[3] = (value & 0xFF);

	res = icm206xx_share_i2c_write_memory(D_PEDSTD_PEAKTHRSH, 4, &databuf[0]);
	if (res < 0) {
		STEP_C_ERR("write Peak threshold err!\n");
		return 0;
	}

	return count;
}
#endif

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value,   store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value,   NULL);
#ifdef ICM206XX_DMP_STEP_COUNTER_TUNE
static DRIVER_ATTR(sbth, S_IWUSR | S_IRUGO, show_sb_threshold, store_sb_threshold);
static DRIVER_ATTR(peakth, S_IWUSR | S_IRUGO, show_peak_threshold, store_peak_threshold);
#endif

static struct driver_attribute *icm206xx_sc_attr_list[] = {
	&driver_attr_trace,		/*trace log*/
	&driver_attr_status,
#ifdef ICM206XX_DMP_STEP_COUNTER_TUNE
	&driver_attr_sbth,
	&driver_attr_peakth,
#endif
};
/*----------------------------------------------------------------------------*/
static int icm206xx_sc_create_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(icm206xx_sc_attr_list)/sizeof(icm206xx_sc_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		res = driver_create_file(driver, icm206xx_sc_attr_list[idx]);
		if (0 != res) {
			STEP_C_ERR("driver_create_file (%s) = %d\n", icm206xx_sc_attr_list[idx]->attr.name, res);
			break;
		}
	}
	return res;
}

static int icm206xx_sc_delete_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(icm206xx_sc_attr_list)/sizeof(icm206xx_sc_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, icm206xx_sc_attr_list[idx]);

	return res;
}

/*=======================================================================================*/
/* Misc - Factory Mode (IOCTL) Device Driver Section					 */
/*=======================================================================================*/

static struct miscdevice icm206xx_sc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "step_counter",
};

/*=======================================================================================*/
/* Misc - I2C HAL Support Section							 */
/*=======================================================================================*/

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int icm206xx_sc_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */
static int icm206xx_sc_enable_nodata(int en)
{
	if (1 == en) {
		icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, true);
		icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_STEP_COUNTER, true);
		icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_STEP_COUNTER, 5000000, false);
		icm206xx_sc_EnableStepCounter(en);
	} else {
		icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_STEP_COUNTER, 0, false);
		icm206xx_sc_EnableStepCounter(en);
		icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_STEP_COUNTER, false);
		icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_STEP_COUNTER, false);
	}

	STEP_C_LOG("icm206xx_sc_enable_nodata OK!\n");

	return 0;
}

static int icm206xx_sc_step_c_set_delay(u64 delay)
{
	return 0;
}

static int icm206xx_sc_step_d_set_delay(u64 delay)
{
	return 0;
}

static int icm206xx_sc_enable_significant(int en)
{
	return 0;
}

static int icm206xx_sc_enable_step_detect(int en)
{
	return 0;
}

static int icm206xx_sc_get_data(uint32_t *value, int *status)
{
	uint32_t step_count = 0;

	icm206xx_sc_ReadSensorData(&step_count);

	*value = step_count;
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}

static int icm206xx_sc_get_data_step_d(uint32_t *value, int *status)
{
	return 0;
}

static int icm206xx_sc_get_data_significant(uint32_t *value, int *status)
{
	return 0;
}

/*=======================================================================================*/
/* HAL Attribute Registration Section							 */
/*=======================================================================================*/

static int icm206xx_sc_attr_create(void)
{
	struct step_c_control_path ctl = {0};
	struct step_c_data_path data = {0};
	int res = 0;

	STEP_C_LOG();

	res = icm206xx_sc_init_client(false);
	if (res)
		goto exit_init_failed;

	/* misc_register() for factory mode, engineer mode and so on */
	res = misc_register(&icm206xx_sc_device);
	if (res) {
		STEP_C_ERR("icm206xx_a_device misc register failed!\n");
		goto exit_misc_device_register_failed;
	}

	/* Crate platform_driver attribute */
	res = icm206xx_sc_create_attr(&(icm206xx_sc_init_info.platform_diver_addr->driver));
	if (res) {
		STEP_C_ERR("icm206xx_sc create attribute err = %d\n", res);
		goto exit_create_attr_failed;
	}

	/*-----------------------------------------------------------*/
	/* Fill and Register the step_c_control_path and step_c_data_path */
	/*-----------------------------------------------------------*/

	/* Fill the step_c_control_path */
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = false;
	ctl.open_report_data = icm206xx_sc_open_report_data;		/* function pointer */
	ctl.enable_nodata = icm206xx_sc_enable_nodata;			/* function pointer */
	ctl.step_c_set_delay  = icm206xx_sc_step_c_set_delay;		/* function pointer */
	ctl.step_d_set_delay  = icm206xx_sc_step_d_set_delay;		/* function pointer */
	ctl.enable_significant  = icm206xx_sc_enable_significant;	/* function pointer */
	ctl.enable_step_detect  = icm206xx_sc_enable_step_detect;	/* function pointer */

	/* Register the step_c_control_path */
	res = step_c_register_control_path(&ctl);
	if (res) {
		STEP_C_ERR("register step_c control path err\n");
		goto exit_kfree;
	}

	/* Fill the step_c_data_path */
	data.get_data = icm206xx_sc_get_data;			/* function pointer */
	data.get_data_step_d = icm206xx_sc_get_data_step_d;
	data.get_data_significant = icm206xx_sc_get_data_significant;
	data.vender_div = 1000;					/* Taly : Maybe not necessary for step counter! Check later! */

	/* Register the step_c_data_path */
	res = step_c_register_data_path(&data);
	if (res) {
		STEP_C_ERR("register step_c data path fail = %d\n", res);
		goto exit_kfree;
	}

	/* Set init_flag = 0 and return */
	icm206xx_sc_init_flag = 0;

	STEP_C_LOG("%s: OK\n", __func__);
	return 0;

exit_misc_device_register_failed:
	misc_deregister(&icm206xx_sc_device);
exit_create_attr_failed:
exit_init_failed:
exit_kfree:
/* exit: */
	icm206xx_sc_init_flag =  -1;
	STEP_C_ERR("%s: err = %d\n", __func__, res);
	return res;
}

static int icm206xx_sc_attr_remove(void)
{
	int res = 0;

	res = icm206xx_sc_delete_attr(&(icm206xx_sc_init_info.platform_diver_addr->driver));
	if (res)
		STEP_C_ERR("icm206xx_sc_delete_attr fail: %d\n", res);
	else
		res = 0;

	res = misc_deregister(&icm206xx_sc_device);
	if (res)
		STEP_C_ERR("misc_deregister fail: %d\n", res);
	else
		res = 0;

	return res;
}

/*=======================================================================================*/
/* Kernel Module Section								 */
/*=======================================================================================*/

static int icm206xx_sc_remove(void)
{
	icm206xx_sc_attr_remove();

	return 0;
}

static int icm206xx_sc_local_init(void)
{
	icm206xx_sc_attr_create();

	if (-1 == icm206xx_sc_init_flag)
		return -1;
	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init icm206xx_sc_init(void)
{
	step_c_driver_add(&icm206xx_sc_init_info);

	return 0;
}

static void __exit icm206xx_sc_exit(void)
{

}

/*----------------------------------------------------------------------------*/
module_init(icm206xx_sc_init);
module_exit(icm206xx_sc_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("icm206xx step counter driver");
