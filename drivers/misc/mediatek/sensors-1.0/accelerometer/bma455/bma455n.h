/**\mainpage
*
****************************************************************************
* Copyright (C) 2015 - 2016 Bosch Sensortec GmbH
*
* File : bma455n.h
*
* Date: 30 Aug 2017
*
* Revision : 1.0.2 $
*
* Usage: Sensor Driver for BMA455 sensor
*
****************************************************************************
*
* \section Disclaimer
*
* Common:
* Bosch Sensortec products are developed for the consumer goods industry.
* They may only be used within the parameters of the respective valid
* product data sheet.  Bosch Sensortec products are provided with the
* express understanding that there is no warranty of fitness for a
* particular purpose.They are not fit for use in life-sustaining,
* safety or security sensitive systems or any system or device
* that may lead to bodily harm or property damage if the system
* or device malfunctions. In addition,Bosch Sensortec products are
* not fit for use in products which interact with motor vehicle systems.
* The resale and or use of products are at the purchasers own risk and
* his own responsibility. The examination of fitness for the intended use
* is the sole responsibility of the Purchaser.
*
* The purchaser shall indemnify Bosch Sensortec from all third party
* claims, including any claims for incidental, or consequential damages,
* arising from any product use not covered by the parameters of
* the respective valid product data sheet or not approved by
* Bosch Sensortec and reimburse Bosch Sensortec for all costs in
* connection with such claims.
*
* The purchaser must monitor the market for the purchased products,
* particularly with regard to product safety and inform Bosch Sensortec
* without delay of all security relevant incidents.
*
* Engineering Samples are marked with an asterisk (*) or (e).
* Samples may vary from the valid technical specifications of the product
* series. They are therefore not intended or fit for resale to third
* parties or for use in end products. Their sole purpose is internal
* client testing. The testing of an engineering sample may in no way
* replace the testing of a product series. Bosch Sensortec assumes
* no liability for the use of engineering samples.
* By accepting the engineering samples, the Purchaser agrees to indemnify
* Bosch Sensortec from all claims arising from the use of engineering
* samples.
*
* Special:
* This software module (hereinafter called "Software") and any information
* on application-sheets (hereinafter called "Information") is provided
* free of charge for the sole purpose to support your application work.
* The Software and Information is subject to the following
* terms and conditions:
*
* The Software is specifically designed for the exclusive use for
* Bosch Sensortec products by personnel who have special experience
* and training. Do not use this Software if you do not have the
* proper experience or training.
*
* This Software package is provided `` as is `` and without any expressed
* or implied warranties,including without limitation, the implied warranties
* of merchantability and fitness for a particular purpose.
*
* Bosch Sensortec and their representatives and agents deny any liability
* for the functional impairment
* of this Software in terms of fitness, performance and safety.
* Bosch Sensortec and their representatives and agents shall not be liable
* for any direct or indirect damages or injury, except as
* otherwise stipulated in mandatory applicable law.
*
* The Information provided is believed to be accurate and reliable.
* Bosch Sensortec assumes no responsibility for the consequences of use
* of such Information nor for any infringement of patents or
* other rights of third parties which may result from its use.
* No license is granted by implication or otherwise under any patent or
* patent rights of Bosch. Specifications mentioned in the Information are
* subject to change without notice.
**************************************************************************/
/*! \file bma455n.h
	\brief Sensor Driver for BMA455 sensor for Android N */

#ifndef BMA455N_H
#define BMA455N_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bma4.h"

/**\name Chip ID of BMA455 sensor */
#define	BMA455N_CHIP_ID						UINT8_C(0x15)

/**\name Sensor feature size */
#define BMA455N_FEATURE_SIZE				UINT8_C(80)
#define BMA455N_ANYMOTION_EN_LEN			UINT8_C(2)
#define BMA455N_ANY_NO_MOTION_EN_LEN		UINT8_C(4)
#define BMA455N_RD_WR_MIN_LEN				UINT8_C(2)

/**\name Feature offset address */
#define BMA455N_ANY_MOTION_OFFSET			UINT8_C(0x00)
#define BMA455N_NO_MOTION_OFFSET			UINT8_C(0x04)
#define BMA455N_STEP_CNTR_PARAM_OFFSET		UINT8_C(0x08)
#define BMA455N_STEP_CNTR_OFFSET			UINT8_C(0x3A)
#define BMA455N_ORIENTATION_OFFSET			UINT8_C(0x3C)
#define BMA455N_SIG_MOTION_OFFSET			UINT8_C(0x40)
#define BMA455N_SIG_MOTION_CONFIG_OFFSET	UINT8_C(0x42)
#define BMA455N_CONFIG_ID_OFFSET			UINT8_C(0x4C)
#define BMA455N_AXES_REMAP_OFFSET			UINT8_C(0x4E)

/**************************************************************/
/**\name	Any Motion */
/**************************************************************/
/**\name Any motion threshold macros */
#define BMA455N_ANY_MOTION_THRES_POS		UINT8_C(0)
#define BMA455N_ANY_MOTION_THRES_MSK		UINT16_C(0x07FF)

/**\name Any motion duration macros */
#define BMA455N_ANY_MOTION_DUR_POS			UINT8_C(0)
#define BMA455N_ANY_MOTION_DUR_MSK			UINT16_C(0x1FFF)

/**\name Any motion enable macros */
#define BMA455N_ANY_MOTION_AXIS_EN_POS		UINT8_C(5)
#define BMA455N_ANY_MOTION_AXIS_EN_MSK		UINT8_C(0xE0)

/**************************************************************/
/**\name	No Motion */
/**************************************************************/
/**\name No motion threshold macros */
#define BMA455N_NO_MOTION_THRES_POS			UINT8_C(0)
#define BMA455N_NO_MOTION_THRES_MSK			UINT16_C(0x07FF)

/**\name No motion duration macros */
#define BMA455N_NO_MOTION_DUR_POS			UINT8_C(0)
#define BMA455N_NO_MOTION_DUR_MSK			UINT16_C(0x1FFF)

/**\name No motion Axes enable macros */
#define BMA455N_NO_MOTION_AXIS_EN_POS		UINT8_C(5)
#define BMA455N_NO_MOTION_AXIS_EN_MSK		UINT8_C(0xE0)

/**************************************************************/
/**\name	Step Counter & Detector */
/**************************************************************/
/**\name Step counter watermark macros */
#define BMA455N_STEP_CNTR_WM_POS			UINT8_C(0)
#define BMA455N_STEP_CNTR_WM_MSK			UINT16_C(0x03FF)

/**\name Step counter reset macros */
#define BMA455N_STEP_CNTR_RST_POS			UINT8_C(2)
#define BMA455N_STEP_CNTR_RST_MSK			UINT8_C(0x04)

/**\name Step detector enable macros */
#define BMA455N_STEP_DETECTOR_EN_POS		UINT8_C(3)
#define BMA455N_STEP_DETECTOR_EN_MSK		UINT8_C(0x08)

/**\name Step counter enable macros */
#define BMA455N_STEP_CNTR_EN_POS			UINT8_C(4)
#define BMA455N_STEP_CNTR_EN_MSK			UINT8_C(0x10)

/**\name Step count output length*/
#define BMA455N_STEP_CNTR_DATA_SIZE			UINT16_C(4)

/**\name Activity enable macros */
#define BMA455N_ACTIVITY_EN_POS				UINT8_C(5)
#define BMA455N_ACTIVITY_EN_MSK				UINT8_C(0x20)

/**************************************************************/
/**\name	Step counter parameter setting(1-25) */
/**************************************************************/
/**\name Configuration selection macros */
#define BMA455N_PHONE_CONFIG				UINT8_C(0x00)
#define BMA455N_WRIST_CONFIG				UINT8_C(0x01)

/**\name Step counter parameter setting(1-25) for phone (Default) */
#define BMA455N_PHONE_SC_PARAM_1			UINT16_C(0x132)
#define BMA455N_PHONE_SC_PARAM_2			UINT16_C(0x78E6)
#define BMA455N_PHONE_SC_PARAM_3			UINT16_C(0x84)
#define BMA455N_PHONE_SC_PARAM_4			UINT16_C(0x6C9C)
#define BMA455N_PHONE_SC_PARAM_5			UINT8_C(0x07)
#define BMA455N_PHONE_SC_PARAM_6			UINT16_C(0x7564)
#define BMA455N_PHONE_SC_PARAM_7			UINT16_C(0x7EAA)
#define BMA455N_PHONE_SC_PARAM_8			UINT16_C(0x55F)
#define BMA455N_PHONE_SC_PARAM_9			UINT16_C(0xABE)
#define BMA455N_PHONE_SC_PARAM_10			UINT16_C(0x55F)
#define BMA455N_PHONE_SC_PARAM_11			UINT16_C(0xE896)
#define BMA455N_PHONE_SC_PARAM_12			UINT16_C(0x41EF)
#define BMA455N_PHONE_SC_PARAM_13			UINT8_C(0x01)
#define BMA455N_PHONE_SC_PARAM_14			UINT8_C(0x0C)
#define BMA455N_PHONE_SC_PARAM_15			UINT8_C(0x0C)
#define BMA455N_PHONE_SC_PARAM_16			UINT8_C(0x4A)
#define BMA455N_PHONE_SC_PARAM_17			UINT8_C(0xA0)
#define BMA455N_PHONE_SC_PARAM_18			UINT8_C(0x00)
#define BMA455N_PHONE_SC_PARAM_19			UINT8_C(0x0C)
#define BMA455N_PHONE_SC_PARAM_20			UINT16_C(0x3CF0)
#define BMA455N_PHONE_SC_PARAM_21			UINT16_C(0x100)
#define BMA455N_PHONE_SC_PARAM_22			UINT8_C(0x00)
#define BMA455N_PHONE_SC_PARAM_23			UINT8_C(0x00)
#define BMA455N_PHONE_SC_PARAM_24			UINT8_C(0x00)
#define BMA455N_PHONE_SC_PARAM_25			UINT8_C(0x00)

/**\name Step counter parameter setting(1-25) for wrist */
#define BMA455N_WRIST_SC_PARAM_1			UINT16_C(0x12D)
#define BMA455N_WRIST_SC_PARAM_2			UINT16_C(0x7BD4)
#define BMA455N_WRIST_SC_PARAM_3			UINT16_C(0x13B)
#define BMA455N_WRIST_SC_PARAM_4			UINT16_C(0x7ADB)
#define BMA455N_WRIST_SC_PARAM_5			UINT8_C(0x04)
#define BMA455N_WRIST_SC_PARAM_6			UINT16_C(0x7B3F)
#define BMA455N_WRIST_SC_PARAM_7			UINT16_C(0x6CCD)
#define BMA455N_WRIST_SC_PARAM_8			UINT16_C(0x4C3)
#define BMA455N_WRIST_SC_PARAM_9			UINT16_C(0x985)
#define BMA455N_WRIST_SC_PARAM_10			UINT16_C(0x4C3)
#define BMA455N_WRIST_SC_PARAM_11			UINT16_C(0xE6EC)
#define BMA455N_WRIST_SC_PARAM_12			UINT16_C(0x460C)
#define BMA455N_WRIST_SC_PARAM_13			UINT8_C(0x01)
#define BMA455N_WRIST_SC_PARAM_14			UINT8_C(0x27)
#define BMA455N_WRIST_SC_PARAM_15			UINT8_C(0x19)
#define BMA455N_WRIST_SC_PARAM_16			UINT8_C(0x96)
#define BMA455N_WRIST_SC_PARAM_17			UINT8_C(0xA0)
#define BMA455N_WRIST_SC_PARAM_18			UINT8_C(0x01)
#define BMA455N_WRIST_SC_PARAM_19			UINT8_C(0x0C)
#define BMA455N_WRIST_SC_PARAM_20			UINT16_C(0x3CF0)
#define BMA455N_WRIST_SC_PARAM_21			UINT16_C(0x100)
#define BMA455N_WRIST_SC_PARAM_22			UINT8_C(0x01)
#define BMA455N_WRIST_SC_PARAM_23			UINT8_C(0x03)
#define BMA455N_WRIST_SC_PARAM_24			UINT8_C(0x01)
#define BMA455N_WRIST_SC_PARAM_25			UINT8_C(0x0E)

/**\name Activity recognition macros */
#define BMA455N_USER_STATIONARY				UINT8_C(0x00)
#define BMA455N_USER_WALKING				UINT8_C(0x01)
#define BMA455N_USER_RUNNING				UINT8_C(0x02)
#define BMA455N_STATE_INVALID				UINT8_C(0x03)

/**************************************************************/
/**\name	Orientation */
/**************************************************************/
#define BMA455N_ORIENTATION_OUTPUT_MASK		UINT8_C(0x07)

/**\name Orientation enable macros */
#define BMA455N_ORIENT_EN_BYTE				UINT8_C(0)
#define BMA455N_ORIENT_EN_POS				UINT8_C(0)
#define BMA455N_ORIENT_EN_MSK				UINT8_C(0x01)

/**\name Orientation upside/down detection macros */
#define BMA455N_ORIENT_UD_BYTE				UINT8_C(0)
#define BMA455N_ORIENT_UD_POS				UINT8_C(1)
#define BMA455N_ORIENT_UD_MSK				UINT8_C(0x02)

/**\name Orientation mode macros */
#define BMA455N_ORIENT_MODE_BYTE			UINT8_C(0)
#define BMA455N_ORIENT_MODE_POS				UINT8_C(2)
#define BMA455N_ORIENT_MODE_MSK				UINT8_C(0x0C)

/**\name Orientation blocking macros */
#define BMA455N_ORIENT_BLOCK_BYTE			UINT8_C(0)
#define BMA455N_ORIENT_BLOCK_POS			UINT8_C(4)
#define BMA455N_ORIENT_BLOCK_MSK			UINT8_C(0x30)

/**\name Orientation theta macros */
#define BMA455N_ORIENT_THETA_BYTE			UINT8_C(0)
#define BMA455N_ORIENT_THETA_POS			UINT8_C(6)
#define BMA455N_ORIENT_THETA_MSK			UINT16_C(0x0FC0)

/**\name Orientation hysteresis macros */
#define BMA455N_ORIENT_HYST_BYTE			UINT8_C(0)
#define BMA455N_ORIENT_HYST_POS				UINT8_C(0)
#define BMA455N_ORIENT_HYST_MSK				UINT16_C(0x07FF)

/**\name Orientation output macros */
/* Bit pos 2 reflects the face-up (0) face-down(1) only if ud_en is enabled */
#define BMA455N_FACE_UP						UINT8_C(0x00)
#define BMA455N_FACE_DOWN					UINT8_C(0x01)
/* Bit pos 0-1 reflects the have the following value */
#define BMA455N_PORTRAIT_UP_RIGHT			UINT8_C(0x00)
#define BMA455N_LANDSCAPE_LEFT				UINT8_C(0x01)
#define BMA455N_PORTRAIT_UP_DOWN			UINT8_C(0x02)
#define BMA455N_LANDSCAPE_RIGHT				UINT8_C(0x03)

/**************************************************************/
/**\name	Significant motion */
/**************************************************************/
/**\name Significant motion enable macros */
#define BMA455N_SIG_MOTION_EN_POS			UINT8_C(0)
#define BMA455N_SIG_MOTION_EN_MSK			UINT8_C(0x01)

/**\name Significant motion block size macros */
#define BMA455N_SIG_MOTION_BLOCK_POS		UINT8_C(0)
#define BMA455N_SIG_MOTION_BLOCK_MSK		UINT8_C(0xFFFF)

/**\name Significant motion peak to peak min interval macros */
#define BMA455N_SIG_MOTION_P2P_MIN_POS		UINT8_C(0)
#define BMA455N_SIG_MOTION_P2P_MIN_MSK		UINT8_C(0xFFFF)

/**\name Significant motion peak to peak max interval macros */
#define BMA455N_SIG_MOTION_P2P_MAX_POS		UINT8_C(0)
#define BMA455N_SIG_MOTION_P2P_MAX_MSK		UINT8_C(0xFFFF)

/**\name Significant motion mean crossing rate min interval macros */
#define BMA455N_SIG_MOTION_MCR_MIN_POS		UINT8_C(0)
#define BMA455N_SIG_MOTION_MCR_MIN_MSK		UINT8_C(0xFFFF)

/**\name Significant motion mean crossing rate max interval macros */
#define BMA455N_SIG_MOTION_MCR_MAX_POS		UINT8_C(0)
#define BMA455N_SIG_MOTION_MCR_MAX_MSK		UINT8_C(0xFFFF)


/**************************************************************/
/**\name	Axes Remap */
/**************************************************************/
#define BMA455N_X_AXIS_MASK					UINT8_C(0x03)
#define BMA455N_X_AXIS_SIGN_MASK			UINT8_C(0x04)
#define BMA455N_Y_AXIS_MASK					UINT8_C(0x18)
#define BMA455N_Y_AXIS_SIGN_MASK			UINT8_C(0x20)
#define BMA455N_Z_AXIS_MASK					UINT8_C(0xC0)
#define BMA455N_Z_AXIS_SIGN_MASK			UINT8_C(0x01)

/**************************************************************/
/**\name	User macros */
/**************************************************************/
/**\name Anymotion/Nomotion axis enable macros */
#define BMA455N_X_AXIS_EN					UINT8_C(0x01)
#define BMA455N_Y_AXIS_EN					UINT8_C(0x02)
#define BMA455N_Z_AXIS_EN					UINT8_C(0x04)
#define BMA455N_EN_ALL_AXIS					UINT8_C(0x07)
#define BMA455N_DIS_ALL_AXIS				UINT8_C(0x00)

/**\name Feature enable macros which are mutually exclusive */
#define BMA455N_STEP_DETR					UINT8_C(0x01)
#define BMA455N_STEP_CNTR					UINT8_C(0x02)
#define BMA455N_ACTIVITY					UINT8_C(0x04)
#define BMA455N_ORIENTATION					UINT8_C(0x08)
#define BMA455N_SIG_MOTION					UINT8_C(0x10)

/**\name Interrupt status macros */
#define	BMA455N_SIG_MOTION_INT				UINT8_C(0x01)
#define BMA455N_STEP_CNTR_INT				UINT8_C(0x02)
#define BMA455N_ACTIVITY_INT				UINT8_C(0x04)
#define BMA455N_ORIENTATION_INT				UINT8_C(0x08)
#define BMA455N_ANY_MOTION_INT				UINT8_C(0x20)
#define BMA455N_NO_MOTION_INT				UINT8_C(0x40)
#define BMA455N_ERROR_INT					UINT8_C(0x80)

/*!
 * @brief Any motion configuration
 */
struct bma455n_anymotion_config {
    /*! Expressed in 50 Hz samples (20 ms) */
	uint16_t duration;
	/*! Threshold value for Any-motion/No-motion detection in
	5.11g format */
	uint16_t threshold;
};

/*!
 * @brief No motion configuration
 */
struct bma455n_nomotion_config {
    /*! Expressed in 50 Hz samples (20 ms) */
	uint16_t duration;
	/*! Threshold value for Any-motion/No-motion detection in
	5.11g format */
	uint16_t threshold;
};

/*!
 * @brief Step counter param settings
 */
struct bma455n_stepcounter_settings {
	/*! Step Counter param 1 */
	uint16_t param1;
	/*! Step Counter param 2 */
	uint16_t param2;
	/*! Step Counter param 3 */
	uint16_t param3;
	/*! Step Counter param 4 */
	uint16_t param4;
	/*! Step Counter param 5 */
	uint16_t param5;
	/*! Step Counter param 6 */
	uint16_t param6;
	/*! Step Counter param 7 */
	uint16_t param7;
	/*! Step Counter param 8 */
	uint16_t param8;
	/*! Step Counter param 9 */
	uint16_t param9;
	/*! Step Counter param 10 */
	uint16_t param10;
	/*! Step Counter param 11 */
	uint16_t param11;
	/*! Step Counter param 12 */
	uint16_t param12;
	/*! Step Counter param 13 */
	uint16_t param13;
	/*! Step Counter param 14 */
	uint16_t param14;
	/*! Step Counter param 15 */
	uint16_t param15;
	/*! Step Counter param 16 */
	uint16_t param16;
	/*! Step Counter param 17 */
	uint16_t param17;
	/*! Step Counter param 18 */
	uint16_t param18;
	/*! Step Counter param 19 */
	uint16_t param19;
	/*! Step Counter param 20 */
	uint16_t param20;
	/*! Step Counter param 21 */
	uint16_t param21;
	/*! Step Counter param 22 */
	uint16_t param22;
	/*! Step Counter param 23 */
	uint16_t param23;
	/*! Step Counter param 24 */
	uint16_t param24;
	/*! Step Counter param 25 */
	uint16_t param25;
};

/*!
 * @brief Orientation configuration structure
 */
struct bma455n_orientation_config {
	uint8_t upside_down;/**<upside/down detection*/
	uint8_t mode;/**< symmetrical (values 0 or 3), high asymmetrical
	(value 1) or low asymmetrical (value 2)*/
	uint8_t blocking;/**<blocking mode. If blocking is set,
	no Orientation interrupt will be triggered*/
	uint8_t theta;/**<Coded value of the threshold angle with horizontal
	used in Blocking modes*/
	uint16_t hysteresis;/**<Acceleration hysteresis for Orientation
	detection expressed in 5.11g format*/
};

/*!
 * @brief Significant motion configuration
 */
struct bma455n_sig_motion_config {
    /*! Holds the block size */
	uint16_t block_size;
    /*! Holds the peak to peak minimum interval */
	uint16_t p2p_min_intvl;
	/*! Holds the peak to peak maximum interval */
	uint16_t p2p_max_intvl;
	/*! Holds the mean crossing rate minimum */
	uint16_t mcr_min;
	/*! Holds the mean crossing rate maximum */
	uint16_t mcr_max;
};

/*!
 * @brief Axes remapping configuration
 */
struct bma455n_axes_remap {
	uint8_t x_axis;
	uint8_t x_axis_sign;
	uint8_t y_axis;
	uint8_t y_axis_sign;
	uint8_t z_axis;
	uint8_t z_axis_sign;
};

/***************************************************************************/
/**\name		Function declaration
****************************************************************************/
/*!
 *	@brief This API is the entry point.
 *	Call this API before using all other APIs.
 *	This API reads the chip-id of the sensor and sets the resolution.
 *
 *	@param[in,out] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_init(struct bma4_dev *dev);

/*!
 *	@brief This API is used to upload the config file to enable
 *	the features of the sensor.
 *
 *	@param[in] dev : Structure instance of bma4_dev.
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_write_config_file(struct bma4_dev *dev);

/*!
 *	@brief This API is used to get the configuration id of the sensor.
 *
 *	@param[out] config_id : Pointer variable used to store
 *	the configuration id.
 *	@param[in] dev : Structure instance of bma4_dev.
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_get_config_id(uint16_t *config_id, struct bma4_dev *dev);

/*!
 *	@brief This API sets/unsets the user provided interrupt to
 *	either interrupt pin1 or pin2 in the sensor.
 *
 *	@param[in] int_line: Variable to select either interrupt pin1 or pin2.
 *  int_line    |   Macros
 *  ------------|-------------------
 *		0       | BMA4_INTR1_MAP
 *		1       | BMA4_INTR2_MAP
 *	@param[in] int_map : Variable to specify the interrupts.
 *	@param[in] enable : Variable to specify mapping or unmapping
 *	of interrupts.
 *  enable		|	Macros
 *	------------|-------------------
 *   0x01		|  BMA4_EN
 *   0x00		|  BMA4_DIS
 *	@param[in] dev : Structure instance of bma4_dev.
 *
 *	@note Below macros specify the interrupts.
 *	Feature Interrupts
 *	  - BMA455N_SIG_MOTION_INT
 *	  - BMA455N_STEP_CNTR_INT
 *	  - BMA455N_ACTIVITY_INT
 *	  - BMA455N_ORIENTATION_INT
 *	  - BMA455N_ANY_MOTION_INT
 *	  - BMA455N_NO_MOTION_INT
 *	  - BMA455N_ERROR_INT
 *
 *	Hardware Interrupts
 *	  - BMA4_FIFO_FULL_INT
 *	  - BMA4_FIFO_WM_INT
 *	  - BMA4_DATA_RDY_INT
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_map_interrupt(uint8_t int_line, uint16_t int_map, uint8_t enable, struct bma4_dev *dev);

/*!
 *	@brief This API reads the bma455 interrupt status from the sensor.
 *
 *	@param[out] int_status : Variable to store the interrupt status
 *	read from the sensor.
 *	@param[in] dev : Structure instance of bma4_dev.
 *
 *	@note Below macros are used to check the interrupt status.
 *	Feature Interrupts
 *	  - BMA455N_SIG_MOTION_INT
 *	  - BMA455N_STEP_CNTR_INT
 *	  - BMA455N_ACTIVITY_INT
 *	  - BMA455N_ORIENTATION_INT
 *	  - BMA455N_ANY_MOTION_INT
 *	  - BMA455N_NO_MOTION_INT
 *	  - BMA455N_ERROR_INT
 *
 *	Hardware Interrupts
 *	  - BMA4_FIFO_FULL_INT
 *	  - BMA4_FIFO_WM_INT
 *	  - BMA4_MAG_DATA_RDY_INT
 *	  - BMA4_ACCEL_DATA_RDY_INT
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_read_int_status(uint16_t *int_status, struct bma4_dev *dev);

/*!
 *	@brief @brief This API enables/disables the features of the sensor.
 *
 *	@param[in] feature : Variable to specify the features
 *	which are to be set in bma455n sensor.
 *	@param[in] enable : Variable which specifies whether to enable or
 *	disable the features in the bma455 sensor.
 *  enable		|	Macros
 *	------------|-------------------
 *   0x01		|  BMA4_EN
 *   0x00		|  BMA4_DIS
 *	@param[in] dev : Structure instance of bma4_dev.
 *
 *	@note User should use the below macros to enable or disable the
 *	features of bma455 sensor
 *
 *	- BMA455N_ANY_MOTION
 *	- BMA455N_NO_MOTION
 *	- BMA455N_STEP_DETR
 *	- BMA455N_STEP_CNTR
 *	- BMA455N_ACTIVITY
 *	- BMA455N_ORIENTATION
 *	- BMA455N_SIG_MOTION
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_feature_enable(uint8_t feature, uint8_t enable, struct bma4_dev *dev);

/*!
 *	@brief This API performs x, y and z axis remapping in the sensor.
 *
 *	@param[in] remap_data : Pointer to store axes remapping data.
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_set_remap_axes(const struct bma455n_axes_remap *remap_data, struct bma4_dev *dev);

/*!
 *	@brief This API reads the x, y and z axis remap data from the sensor.
 *
 *	@param[out] remap_data : Pointer to store axis remap data which is read
 *	from the bma455n sensor.
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_get_remap_axes(struct bma455n_axes_remap *remap_data, struct bma4_dev *dev);

/*!
 *	@brief	This API sets the configuration of significant motion
 *	in the sensor.
 *
 *	@param[in]	sig_motion : Pointer to structure variable used to
 *	specify the significant motion feature settings.
 *	structure members are provided in the table below
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_set_sig_motion_config(const struct bma455n_sig_motion_config *sig_motion, struct bma4_dev *dev);

/*!	@brief	This API gets the configuration of significant motion feature
 *	from the sensor.
 *
 *	@param[out]	sig_motion : Pointer  to structure variable used to
 *	store the significant motion feature settings read from the sensor.
 *	structure members are provided in the table below
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_get_sig_motion_config(struct bma455n_sig_motion_config *sig_motion, struct bma4_dev *dev);

/*!
 *	@brief This API sets the water mark level for step counter
 *	interrupt in the sensor.
 *
 *	@param[in] step_counter_wm : Variable which specifies water mark level
 *	count.
 *	@note Valid values are from 1 to 1023
 *	@note Value 0 is used for step detector interrupt
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_step_counter_set_watermark(uint16_t step_counter_wm, struct bma4_dev *dev);

/*!
 *	@brief This API gets the water mark level set for step counter interrupt
 *	in the sensor
 *
 *	@param[out] step_counter_wm : Pointer variable which stores
 *	the water mark level read from the sensor.
 *	@note valid values are from 1 to 1023
 *	@note value 0 is used for step detector interrupt
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_step_counter_get_watermark(uint16_t *step_counter_wm, struct bma4_dev *dev);

/*!
 *	@brief This API resets the counted steps of step counter.
 *
 *	@param[in] dev : structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_reset_step_counter(struct bma4_dev *dev);

/*!
 *	@brief This API gets the number of counted steps of step counter
 *	feature from the sensor.
 *
 *  @param[out] step_count : Pointer variable which stores counted
 *	steps read from the sensor.
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_step_counter_output(uint32_t *step_count, struct bma4_dev *dev);

/*!
 *  @brief This API gets the output for activity feature.
 *
 *  @param[out] activity : Pointer variable which stores activity output
 *  read from the sensor.
 *       activity |   State
 *  --------------|------------------------
 *        0x00    | BMA455N_USER_STATIONARY
 *        0x01    | BMA455N_USER_WALKING
 *        0x02    | BMA455N_USER_RUNNING
 *        0x03    | BMA455N_STATE_INVALID
 *
 *  @param[in] dev : Structure instance of bma4_dev
 *  @return Result of API execution status
 *  @retval 0 -> Success
 *  @retval Any non zero value -> Fail
 */
uint16_t bma455n_activity_output(uint8_t *activity, struct bma4_dev *dev);

/*!
 *  @brief This API select the platform configuration wrist(default) or phone.
 *
 *  @param[in] platform : Variable to select wrist/phone
 *
 *     platform  |   Macros
 *  -------------|------------------------
 *        0x00   | BMA455N_PHONE_CONFIG
 *        0x01   | BMA455N_WRIST_CONFIG
 *
 *  @param[in] dev : Structure instance of bma4_dev
 *  @return Result of API execution status
 *  @retval 0 -> Success
 *  @retval Any non zero value -> Fail
 */
uint16_t bma455n_select_platform(uint8_t platform, struct bma4_dev *dev);

/*!
 *	@brief This API gets the parameter1 to parameter7 settings of the
 *	step counter feature.
 *
 *	@param[out] setting : Pointer to structure variable which stores the
 *	parameter1 to parameter7 read from the sensor.
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_stepcounter_get_parameter(struct bma455n_stepcounter_settings *setting, struct bma4_dev *dev);

/*!
 *	@brief This API sets the parameter1 to parameter7 settings of the
 *	step counter feature in the sensor.
 *
 *	@param[in] setting : Pointer to structure variable which stores the
 *	parameter1 to parameter7 settings read from the sensor.
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_stepcounter_set_parameter(const struct bma455n_stepcounter_settings *setting, struct bma4_dev *dev);

/*!
 *	@brief This API enables the any motion feature according to the axis
 *	set by the user in the sensor.
 *
 *	@param[in] axis : Variable to specify the axis of the any motion feature
 *	to be enabled in the sensor.
 *  Value    |  Axis
 *  ---------|-------------------------
 *  0x00     |  BMA455N_DIS_ALL_AXIS
 *  0x01     |  BMA455N_X_AXIS_EN
 *  0x02     |  BMA455N_Y_AXIS_EN
 *  0x04     |  BMA455N_Z_AXIS_EN
 *  0x07     |  BMA455N_EN_ALL_AXIS
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_anymotion_enable_axis(uint8_t axis, struct bma4_dev *dev);

/*!	@brief	This API sets the configuration of Any motion feature in
 *	the sensor.
 *
 *	@param[in]	any_motion : Pointer to structure variable to specify
 *	the any motion feature settings.
 *	Structure members are provided in the table below
 *@verbatim
 * -------------------------------------------------------------------------
 *         Structure parameters    |        Description
 * --------------------------------|----------------------------------------
 *                                 |        Defines the number of
 *                                 |        consecutive data points for
 *                                 |        which the threshold condition
 *         duration                |        must be respected, for interrupt
 *                                 |        assertion. It is expressed in
 *                                 |        50 Hz samples (20 ms).
 *                                 |        Range is 0 to 163sec.
 *                                 |        Default value is 5 = 100ms.
 * --------------------------------|----------------------------------------
 *                                 |        Slope threshold value for
 *                                 |        Any-motion / No-motion detection
 *         threshold               |        in 5.11g format.
 *                                 |        Range is 0 to 1g.
 *                                 |        Default value is 0xAA = 83mg.
 * -------------------------------------------------------------------------
 *@endverbatim
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_set_any_motion_config(const struct bma455n_anymotion_config *any_motion, struct bma4_dev *dev);

/*!	@brief	This API gets the configuration of any motion feature from
 *	the sensor.
 *
 *	@param[out]	any_motion : Pointer to structure variable used to store
 *	the any motion feature settings read from the sensor.
 *	Structure members are provided in the table below
 *@verbatim
 * -------------------------------------------------------------------------
 *         Structure parameters    |        Description
 * --------------------------------|----------------------------------------
 *                                 |        Defines the number of
 *                                 |        consecutive data points for
 *                                 |        which the threshold condition
 *         duration                |        must be respected, for interrupt
 *                                 |        assertion. It is expressed in
 *                                 |        50 Hz samples (20 ms).
 *                                 |        Range is 0 to 163sec.
 *                                 |        Default value is 5 = 100ms.
 * --------------------------------|----------------------------------------
 *                                 |        Slope threshold value for
 *                                 |        Any-motion / No-motion detection
 *         threshold               |        in 5.11g format.
 *                                 |        Range is 0 to 1g.
 *                                 |        Default value is 0xAA = 83mg.
 * -------------------------------------------------------------------------
 *@endverbatim
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_get_any_motion_config(struct bma455n_anymotion_config *any_motion, struct bma4_dev *dev);

/*!
 *	@brief This API enables the No motion feature according to the axis
 *	set by the user in the sensor.
 *
 *	@param[in] axis : Variable to specify the axis of the No motion feature
 *	to be enabled in the sensor.
 *  Value    |  Axis
 *  ---------|-------------------------
 *  0x00     |  BMA455N_DIS_ALL_AXIS
 *  0x01     |  BMA455N_X_AXIS_EN
 *  0x02     |  BMA455N_Y_AXIS_EN
 *  0x04     |  BMA455N_Z_AXIS_EN
 *  0x07     |  BMA455N_EN_ALL_AXIS
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_no_motion_enable_axis(uint8_t axis, struct bma4_dev *dev);

/*!	@brief	This API sets the configuration of No motion feature in
 *	the sensor.
 *
 *	@param[in]	no_motion : Pointer to structure variable to specify
 *	the any motion feature settings.
 *	Structure members are provided in the table below
 *@verbatim
 * -------------------------------------------------------------------------
 *         Structure parameters    |        Description
 * --------------------------------|----------------------------------------
 *                                 |        Defines the number of
 *                                 |        consecutive data points for
 *                                 |        which the threshold condition
 *         duration                |        must be respected, for interrupt
 *                                 |        assertion. It is expressed in
 *                                 |        50 Hz samples (20 ms).
 *                                 |        Range is 0 to 163sec.
 *                                 |        Default value is 5 = 100ms.
 * --------------------------------|----------------------------------------
 *                                 |        Slope threshold value for
 *                                 |        Any-motion / No-motion detection
 *         threshold               |        in 5.11g format.
 *                                 |        Range is 0 to 1g.
 *                                 |        Default value is 0xAA = 83mg.
 * -------------------------------------------------------------------------
 *@endverbatim
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_set_no_motion_config(const struct bma455n_nomotion_config *no_motion, struct bma4_dev *dev);

/*!	@brief	This API gets the configuration of No motion feature from
 *	the sensor.
 *
 *	@param[out]	any_motion : Pointer to structure variable used to store
 *	the No motion feature settings read from the sensor.
 *	Structure members are provided in the table below
 *@verbatim
 * -------------------------------------------------------------------------
 *         Structure parameters    |        Description
 * --------------------------------|----------------------------------------
 *                                 |        Defines the number of
 *                                 |        consecutive data points for
 *                                 |        which the threshold condition
 *         duration                |        must be respected, for interrupt
 *                                 |        assertion. It is expressed in
 *                                 |        50 Hz samples (20 ms).
 *                                 |        Range is 0 to 163sec.
 *                                 |        Default value is 5 = 100ms.
 * --------------------------------|----------------------------------------
 *                                 |        Slope threshold value for
 *                                 |        Any-motion / No-motion detection
 *         threshold               |        in 5.11g format.
 *                                 |        Range is 0 to 1g.
 *                                 |        Default value is 0xAA = 83mg.
 * -------------------------------------------------------------------------
 *@endverbatim
 *	@param[in] dev : Structure instance of bma4_dev
 *
 *	@return Result of API execution status
 *	@retval 0 -> Success
 *	@retval Any non zero value -> Fail
 */
uint16_t bma455n_get_no_motion_config(struct bma455n_nomotion_config *no_motion, struct bma4_dev *dev);

/*!
 *	@brief This API gets the output of orientation feature from the sensor.
 *
 *	@param orientation_output : pointer used to store the orientation
 *	output.
 *@verbatim
 *   Bit Values  |        Description
 *---------------|--------------------------------------------------------------
 *   bit 2       | this reflects the face-up (0) face-down(1) only if ud_en is enabled
 *   bit 0-1     | this have the following value
 *               | 1. Orientation portrait upright = 0
 *               | 2. Orientation landscape left = 1
 *               | 3. Orientation portrait upside down = 2
 *               | 4. Orientation landscape right = 3
 *---------------|--------------------------------------------------------------
 *@endverbatim
 *	@param dev : Structure instance of bma4_dev
 *
 *	@return results of stream_transfer operation
 *	@retval 0 -> Success
 *	@retval Any positive value mentioned in ERROR CODES -> Fail
 *
 */
uint16_t bma455n_orientation_output(uint8_t *orientation_output,
							struct bma4_dev *dev);

/*!	@brief This API sets the configuration for orientation feature of the
 *	sensor.
 *	The orientation feature settings are mentioned in the below table.
 *
 *	@param orientation : Pointer to structure variable used to specify the
 *	settings of orientation feature in the sensor
 *@verbatim
 *************************************************************************
 *        Orientation settings    |        Description
 *********************************|***************************************
 *        upside_down             |        Enables upside/down detection,
 *                                |        if set to 1
 *********************************|***************************************
 *                                |        symmetrical (values 0 or 3),
 *        mode                    |        high asymmetrical (value 1)
 *                                |        or low asymmetrical (value 2)
 *********************************|***************************************
 *                                |        blocking mode.
 *        blocking                |        If blocking is set,
 *                                |        Orientation interrupt will
 *                                |        not be triggered
 *************************************************************************
 *                                |        Coded value of the threshold
 *        theta                   |        angle with horizontal used in
 *                                |        Blocking modes
 *********************************|***************************************
 *                                |        Acceleration hysteresis for
 *        hysteresis              |        Orientation detection
 *                                |        expressed in 5.11g format
 *************************************************************************
 *@endverbatim
 * @param dev : Structure instance of bma4_dev
 *
 *	@return status of bus communication function
 *	@retval 0 -> Success
 *	@retval Any positive value mentioned in ERROR CODES -> Fail
 *
 */
uint16_t bma455n_set_orientation_config(const struct bma455n_orientation_config *orientation,
				struct bma4_dev *dev);

/*!	@brief This API gets the configuration of orientation feature from the
 *	sensor.
 *	The orientation settings are mentioned in the below table.
 *
 *	@param orientation : Pointer to structure variable which holds the
 *	settings of orientation feature read from the sensor
 *@verbatim
 *************************************************************************
 *        Orientation settings    |        Description
 *********************************|***************************************
 *        upside_down             |        Enables upside/down detection,
 *                                |        if set to 1
 *********************************|***************************************
 *                                |        symmetrical (values 0 or 3),
 *        mode                    |        high asymmetrical (value 1)
 *                                |        or low asymmetrical (value 2)
 *********************************|***************************************
 *                                |        blocking mode.
 *        blocking                |        If blocking is set,
 *                                |        Orientation interrupt will
 *                                |        not be triggered
 *************************************************************************
 *                                |        Coded value of the threshold
 *        theta                   |        angle with horizontal used in
 *                                |        Blocking modes
 *********************************|***************************************
 *                                |        Acceleration hysteresis for
 *        hysteresis              |        Orientation detection
 *                                |        expressed in 5.11g format
 *************************************************************************
 *@endverbatim
 * @param dev : Structure instance of bma4_dev
 *
 *	@return status of bus communication function
 *	@retval 0 -> Success
 *	@retval Any positive value mentioned in ERROR CODES -> Fail
 *
 */
uint16_t bma455n_get_orientation_config(struct bma455n_orientation_config *orientation,
				struct bma4_dev *dev);

#ifdef __cplusplus
}
#endif /*End of CPP guard */

#endif /* BMA455N_H */
