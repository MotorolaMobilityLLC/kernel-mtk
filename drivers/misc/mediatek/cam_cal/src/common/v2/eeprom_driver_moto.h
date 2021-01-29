#ifndef __EEPROM_DRIVER_OTP_H
#define __EEPROM_DRIVER_OTP_H

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/file.h>
#include <linux/unistd.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "eeprom_driver.h"
#include "eeprom_i2c_common_driver.h"
#include "cam_cal_list.h"

#include "kd_camera_feature.h"
#include "kd_imgsensor.h"

#define MAIN_OTP_DUMP 1

#define DEV_NAME_FMT_DEV "/dev/camera_eeprom%u"

#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...) pr_err(PFX "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...) pr_err(PFX "[%s] " format, __func__, ##args)

#if MAIN_OTP_DUMP
void dumpEEPROMData(int u4Length,u8* pu1Params);
#endif

int imgSensorCheckEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData, struct stCAM_CAL_CHECKSUM_STRUCT* cData);
int imgSensorReadEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData, struct stCAM_CAL_CHECKSUM_STRUCT* checkData);
int imgSensorSetEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData);

#endif
