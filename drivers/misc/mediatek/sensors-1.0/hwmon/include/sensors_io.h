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

/*
* Copyright(C)2014 MediaTek Inc.
* Modification based on code covered by the below mentioned copyright
* and/or permission notice(S).
*/

#ifndef SENSORS_IO_H
#define SENSORS_IO_H

#include <linux/ioctl.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

struct GSENSOR_VECTOR3D {
	unsigned short x;		/**< X axis */
	unsigned short y;		/**< Y axis */
	unsigned short z;		/**< Z axis */
};

struct SENSOR_DATA {
	int x;
	int y;
	int z;
};


#define GSENSOR								0x85
#define GSENSOR_IOCTL_INIT					_IO(GSENSOR,  0x01)
#define GSENSOR_IOCTL_READ_CHIPINFO			_IOR(GSENSOR, 0x02, int)
#define GSENSOR_IOCTL_READ_SENSORDATA		_IOR(GSENSOR, 0x03, int)
#define GSENSOR_IOCTL_READ_OFFSET			_IOR(GSENSOR, 0x04, struct GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_GAIN				_IOR(GSENSOR, 0x05, struct GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_RAW_DATA			_IOR(GSENSOR, 0x06, int)
#define GSENSOR_IOCTL_SET_CALI				_IOW(GSENSOR, 0x06, struct SENSOR_DATA)
#define GSENSOR_IOCTL_GET_CALI				_IOW(GSENSOR, 0x07, struct SENSOR_DATA)
#define GSENSOR_IOCTL_CLR_CALI				_IO(GSENSOR, 0x08)
#define GSENSOR_IOCTL_ENABLE_CALI			_IO(GSENSOR, 0x09)

#ifdef CONFIG_COMPAT
#define COMPAT_GSENSOR_IOCTL_INIT				_IO(GSENSOR,  0x01)
#define COMPAT_GSENSOR_IOCTL_READ_CHIPINFO		_IOR(GSENSOR, 0x02, compat_int_t)
#define COMPAT_GSENSOR_IOCTL_READ_SENSORDATA	_IOR(GSENSOR, 0x03, compat_int_t)
#define COMPAT_GSENSOR_IOCTL_READ_OFFSET		_IOR(GSENSOR, 0x04, struct GSENSOR_VECTOR3D)
#define COMPAT_GSENSOR_IOCTL_READ_GAIN			_IOR(GSENSOR, 0x05, struct GSENSOR_VECTOR3D)
#define COMPAT_GSENSOR_IOCTL_READ_RAW_DATA		_IOR(GSENSOR, 0x06, compat_int_t)
#define COMPAT_GSENSOR_IOCTL_SET_CALI			_IOW(GSENSOR, 0x06, struct SENSOR_DATA)
#define COMPAT_GSENSOR_IOCTL_GET_CALI			_IOW(GSENSOR, 0x07, struct SENSOR_DATA)
#define COMPAT_GSENSOR_IOCTL_CLR_CALI			_IO(GSENSOR, 0x08)
#define COMPAT_GSENSOR_IOCTL_ENABLE_CALI		_IO(GSENSOR, 0x09)
#endif

/* IOCTLs for Msensor misc. device library */
#define MSENSOR									0x83
#define MSENSOR_IOCTL_INIT						_IO(MSENSOR, 0x01)
#define MSENSOR_IOCTL_READ_CHIPINFO				_IOR(MSENSOR, 0x02, int)
#define MSENSOR_IOCTL_READ_SENSORDATA			_IOR(MSENSOR, 0x03, int)
#define MSENSOR_IOCTL_READ_POSTUREDATA			_IOR(MSENSOR, 0x04, int)
#define MSENSOR_IOCTL_READ_CALIDATA				_IOR(MSENSOR, 0x05, int)
#define MSENSOR_IOCTL_READ_CONTROL				_IOR(MSENSOR, 0x06, int)
#define MSENSOR_IOCTL_SET_CONTROL				_IOW(MSENSOR, 0x07, int)
#define MSENSOR_IOCTL_SET_MODE					_IOW(MSENSOR, 0x08, int)
#define MSENSOR_IOCTL_SET_POSTURE				_IOW(MSENSOR, 0x09, int)
#define MSENSOR_IOCTL_SET_CALIDATA				_IOW(MSENSOR, 0x0a, int)
#define MSENSOR_IOCTL_SENSOR_ENABLE				_IOW(MSENSOR, 0x51, int)
#define MSENSOR_IOCTL_READ_FACTORY_SENSORDATA	_IOW(MSENSOR, 0x52, int)

/* IOCTLs for akm09911 device */
#define ECS_IOCTL_GET_INFO			_IOR(MSENSOR, 0x27, unsigned char[AKM_SENSOR_INFO_SIZE])
#define ECS_IOCTL_GET_CONF			_IOR(MSENSOR, 0x28, unsigned char[AKM_SENSOR_CONF_SIZE])
#define ECS_IOCTL_SET_YPR_09911               _IOW(MSENSOR, 0x29, int[26])
#define ECS_IOCTL_GET_DELAY_09911             _IOR(MSENSOR, 0x30, int64_t[3])
#define	ECS_IOCTL_GET_LAYOUT_09911			_IOR(MSENSOR, 0x31, char)

#ifdef CONFIG_COMPAT
/*COMPACT IOCTL for 64bit kernel running 32bit daemon*/
#define COMPAT_MSENSOR_IOCTL_INIT					_IO(MSENSOR, 0x01)
#define COMPAT_MSENSOR_IOCTL_READ_CHIPINFO			_IOR(MSENSOR, 0x02, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_READ_SENSORDATA		_IOR(MSENSOR, 0x03, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_READ_POSTUREDATA		_IOR(MSENSOR, 0x04, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_READ_CALIDATA			_IOR(MSENSOR, 0x05, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_READ_CONTROL			_IOR(MSENSOR, 0x06, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_SET_CONTROL			_IOW(MSENSOR, 0x07, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_SET_MODE			    _IOW(MSENSOR, 0x08, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_SET_POSTURE		    _IOW(MSENSOR, 0x09, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_SET_CALIDATA		    _IOW(MSENSOR, 0x0a, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_SENSOR_ENABLE          _IOW(MSENSOR, 0x51, compat_int_t)
#define COMPAT_MSENSOR_IOCTL_READ_FACTORY_SENSORDATA  _IOW(MSENSOR, 0x52, compat_int_t)

/*COMPAT IOCTLs for AKM library */
#define COMPAT_ECS_IOCTL_WRITE                 _IOW(MSENSOR, 0x0b, compat_uptr_t)
#define COMPAT_ECS_IOCTL_READ                  _IOWR(MSENSOR, 0x0c, compat_uptr_t)
#define COMPAT_ECS_IOCTL_RESET		           _IO(MSENSOR, 0x0d)	/* NOT used in AK8975 */
#define COMPAT_ECS_IOCTL_SET_MODE              _IOW(MSENSOR, 0x0e, compat_short_t)
#define COMPAT_ECS_IOCTL_GETDATA               _IOR(MSENSOR, 0x0f, char[SENSOR_DATA_SIZE])
#define COMPAT_ECS_IOCTL_SET_YPR               _IOW(MSENSOR, 0x10, compat_short_t[12])
#define COMPAT_ECS_IOCTL_GET_OPEN_STATUS       _IOR(MSENSOR, 0x11, compat_int_t)
#define COMPAT_ECS_IOCTL_GET_CLOSE_STATUS      _IOR(MSENSOR, 0x12, compat_int_t)
#define COMPAT_ECS_IOCTL_GET_OSENSOR_STATUS	   _IOR(MSENSOR, 0x13, compat_int_t)
#define COMPAT_ECS_IOCTL_GET_DELAY             _IOR(MSENSOR, 0x14, compat_short_t)
#define COMPAT_ECS_IOCTL_GET_PROJECT_NAME      _IOR(MSENSOR, 0x15, char[64])
#define COMPAT_ECS_IOCTL_GET_MATRIX            _IOR(MSENSOR, 0x16, compat_short_t [4][3][3])
#define	COMPAT_ECS_IOCTL_GET_LAYOUT			   _IOR(MSENSOR, 0x17, compat_int_t[3])

/*COMPAT IOCTLs for akm09911 device */
#define COMPAT_ECS_IOCTL_GET_INFO			   _IOR(MSENSOR, 0x27, unsigned char[AKM_SENSOR_INFO_SIZE])
#define COMPAT_ECS_IOCTL_GET_CONF			   _IOR(MSENSOR, 0x28, unsigned char[AKM_SENSOR_CONF_SIZE])
#define COMPAT_ECS_IOCTL_SET_YPR_09911         _IOW(MSENSOR, 0x29, compat_int_t[26])
#define COMPAT_ECS_IOCTL_GET_DELAY_09911       _IOR(MSENSOR, 0x30, int64_t[3])
#define	COMPAT_ECS_IOCTL_GET_LAYOUT_09911	   _IOR(MSENSOR, 0x31, char)
#endif

#define ALSPS							0X84
#define ALSPS_SET_PS_MODE				_IOW(ALSPS, 0x01, int)
#define ALSPS_GET_PS_MODE				_IOR(ALSPS, 0x02, int)
#define ALSPS_GET_PS_DATA				_IOR(ALSPS, 0x03, int)
#define ALSPS_GET_PS_RAW_DATA			_IOR(ALSPS, 0x04, int)
#define ALSPS_SET_ALS_MODE				_IOW(ALSPS, 0x05, int)
#define ALSPS_GET_ALS_MODE				_IOR(ALSPS, 0x06, int)
#define ALSPS_GET_ALS_DATA				_IOR(ALSPS, 0x07, int)
#define ALSPS_GET_ALS_RAW_DATA			_IOR(ALSPS, 0x08, int)

/*-------------------MTK add-------------------------------------------*/
#define ALSPS_GET_PS_TEST_RESULT		_IOR(ALSPS, 0x09, int)
#define ALSPS_GET_ALS_TEST_RESULT		_IOR(ALSPS, 0x0A, int)
#define ALSPS_GET_PS_THRESHOLD_HIGH		_IOR(ALSPS, 0x0B, int)
#define ALSPS_GET_PS_THRESHOLD_LOW		_IOR(ALSPS, 0x0C, int)
#define ALSPS_GET_ALS_THRESHOLD_HIGH	_IOR(ALSPS, 0x0D, int)
#define ALSPS_GET_ALS_THRESHOLD_LOW		_IOR(ALSPS, 0x0E, int)
#define ALSPS_IOCTL_CLR_CALI			_IOW(ALSPS, 0x0F, int)
#define ALSPS_IOCTL_GET_CALI			_IOR(ALSPS, 0x10, int)
#define ALSPS_IOCTL_SET_CALI			_IOW(ALSPS, 0x11, int)
#define ALSPS_SET_PS_THRESHOLD			_IOW(ALSPS, 0x12, int)
#define ALSPS_SET_ALS_THRESHOLD			_IOW(ALSPS, 0x13, int)
#define AAL_SET_ALS_MODE				_IOW(ALSPS, 0x14, int)
#define AAL_GET_ALS_MODE				_IOR(ALSPS, 0x15, int)
#define AAL_GET_ALS_DATA				_IOR(ALSPS, 0x16, int)
#define ALSPS_ALS_ENABLE_CALI			_IO(ALSPS, 0x17)
#define ALSPS_PS_ENABLE_CALI			_IO(ALSPS, 0x18)
#ifdef CONFIG_COMPAT
#define COMPAT_ALSPS_SET_PS_MODE				_IOW(ALSPS, 0x01, compat_int_t)
#define COMPAT_ALSPS_GET_PS_MODE				_IOR(ALSPS, 0x02, compat_int_t)
#define COMPAT_ALSPS_GET_PS_DATA				_IOR(ALSPS, 0x03, compat_int_t)
#define COMPAT_ALSPS_GET_PS_RAW_DATA			_IOR(ALSPS, 0x04, compat_int_t)
#define COMPAT_ALSPS_SET_ALS_MODE				_IOW(ALSPS, 0x05, compat_int_t)
#define COMPAT_ALSPS_GET_ALS_MODE				_IOR(ALSPS, 0x06, compat_int_t)
#define COMPAT_ALSPS_GET_ALS_DATA				_IOR(ALSPS, 0x07, compat_int_t)
#define COMPAT_ALSPS_GET_ALS_RAW_DATA			_IOR(ALSPS, 0x08, compat_int_t)

/*-------------------MTK add-------------------------------------------*/
#define COMPAT_ALSPS_GET_PS_TEST_RESULT			_IOR(ALSPS, 0x09, compat_int_t)
#define COMPAT_ALSPS_GET_ALS_TEST_RESULT		_IOR(ALSPS, 0x0A, compat_int_t)
#define COMPAT_ALSPS_GET_PS_THRESHOLD_HIGH		_IOR(ALSPS, 0x0B, compat_int_t)
#define COMPAT_ALSPS_GET_PS_THRESHOLD_LOW		_IOR(ALSPS, 0x0C, compat_int_t)
#define COMPAT_ALSPS_GET_ALS_THRESHOLD_HIGH		_IOR(ALSPS, 0x0D, compat_int_t)
#define COMPAT_ALSPS_GET_ALS_THRESHOLD_LOW		_IOR(ALSPS, 0x0E, compat_int_t)
#define COMPAT_ALSPS_IOCTL_CLR_CALI				_IOW(ALSPS, 0x0F, compat_int_t)
#define COMPAT_ALSPS_IOCTL_GET_CALI				_IOR(ALSPS, 0x10, compat_int_t)
#define COMPAT_ALSPS_IOCTL_SET_CALI				_IOW(ALSPS, 0x11, compat_int_t)
#define COMPAT_ALSPS_SET_PS_THRESHOLD			_IOW(ALSPS, 0x12, compat_int_t)
#define COMPAT_ALSPS_SET_ALS_THRESHOLD			_IOW(ALSPS, 0x13, compat_int_t)
#define COMPAT_AAL_SET_ALS_MODE					_IOW(ALSPS, 0x14, compat_int_t)
#define COMPAT_AAL_GET_ALS_MODE					_IOR(ALSPS, 0x15, compat_int_t)
#define COMPAT_AAL_GET_ALS_DATA					_IOR(ALSPS, 0x16, compat_int_t)
#define COMPAT_ALSPS_ALS_ENABLE_CALI			_IO(ALSPS, 0x17)
#define COMPAT_ALSPS_PS_ENABLE_CALI				_IO(ALSPS, 0x18)
#endif

#define GYROSCOPE							0X86
#define GYROSCOPE_IOCTL_INIT				_IO(GYROSCOPE, 0x01)
#define GYROSCOPE_IOCTL_SMT_DATA			_IOR(GYROSCOPE, 0x02, int)
#define GYROSCOPE_IOCTL_READ_SENSORDATA		_IOR(GYROSCOPE, 0x03, int)
#define GYROSCOPE_IOCTL_SET_CALI			_IOW(GYROSCOPE, 0x04, struct SENSOR_DATA)
#define GYROSCOPE_IOCTL_GET_CALI			_IOW(GYROSCOPE, 0x05, struct SENSOR_DATA)
#define GYROSCOPE_IOCTL_CLR_CALI			_IO(GYROSCOPE, 0x06)
#define GYROSCOPE_IOCTL_READ_SENSORDATA_RAW	_IOR(GYROSCOPE, 0x07, int)
#define GYROSCOPE_IOCTL_READ_TEMPERATURE	_IOR(GYROSCOPE, 0x08, int)
#define GYROSCOPE_IOCTL_GET_POWER_STATUS	_IOR(GYROSCOPE, 0x09, int)
#define GYROSCOPE_IOCTL_ENABLE_CALI			_IO(GYROSCOPE, 0x10)
#ifdef CONFIG_COMPAT
#define COMPAT_GYROSCOPE_IOCTL_INIT					_IO(GYROSCOPE, 0x01)
#define COMPAT_GYROSCOPE_IOCTL_SMT_DATA				_IOR(GYROSCOPE, 0x02, compat_int_t)
#define COMPAT_GYROSCOPE_IOCTL_READ_SENSORDATA		_IOR(GYROSCOPE, 0x03, compat_int_t)
#define COMPAT_GYROSCOPE_IOCTL_SET_CALI				_IOW(GYROSCOPE, 0x04, struct SENSOR_DATA)
#define COMPAT_GYROSCOPE_IOCTL_GET_CALI				_IOW(GYROSCOPE, 0x05, struct SENSOR_DATA)
#define COMPAT_GYROSCOPE_IOCTL_CLR_CALI				_IO(GYROSCOPE, 0x06)
#define COMPAT_GYROSCOPE_IOCTL_READ_SENSORDATA_RAW	_IOR(GYROSCOPE, 0x07, compat_int_t)
#define COMPAT_GYROSCOPE_IOCTL_READ_TEMPERATURE		_IOR(GYROSCOPE, 0x08, compat_int_t)
#define COMPAT_GYROSCOPE_IOCTL_GET_POWER_STATUS		_IOR(GYROSCOPE, 0x09, compat_int_t)
#define COMPAT_GYROSCOPE_IOCTL_ENABLE_CALI			_IO(GYROSCOPE, 0x10)
#endif
#define BROMETER							0X87
#define BAROMETER_IOCTL_INIT				_IO(BROMETER, 0x01)
#define BAROMETER_GET_PRESS_DATA			_IOR(BROMETER, 0x02, int)
#define BAROMETER_GET_TEMP_DATA			    _IOR(BROMETER, 0x03, int)
#define BAROMETER_IOCTL_READ_CHIPINFO		_IOR(BROMETER, 0x04, int)
#define BAROMETER_IOCTL_ENABLE_CALI			_IO(BROMETER, 0x05)
#ifdef CONFIG_COMPAT
#define COMPAT_BAROMETER_IOCTL_INIT				_IO(BROMETER, 0x01)
#define COMPAT_BAROMETER_GET_PRESS_DATA			_IOR(BROMETER, 0x02, compat_int_t)
#define COMPAT_BAROMETER_GET_TEMP_DATA			_IOR(BROMETER, 0x03, compat_int_t)
#define COMPAT_BAROMETER_IOCTL_READ_CHIPINFO	_IOR(BROMETER, 0x04, compat_int_t)
#define COMPAT_BAROMETER_IOCTL_ENABLE_CALI		_IO(BROMETER, 0x05)
#endif

#endif
