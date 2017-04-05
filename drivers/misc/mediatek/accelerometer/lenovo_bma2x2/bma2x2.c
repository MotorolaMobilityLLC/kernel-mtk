/* BMA255 motion sensor driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 *
 * VERSION: V1.7
 * HISTORY:
 * V1.0 --- Driver creation
 * V1.1 --- Add share I2C address function
 * V1.2 --- Fix the bug that sometimes sensor is stuck after system resume.
 * V1.3 --- Add FIFO interfaces.
 * V1.4 --- Use basic i2c function to read fifo data instead of i2c DMA mode.
 * V1.5 --- Add compensated value performed by MTK acceleration calibration.
 * V1.6 --- Add walkaround for powermode change.
 * V1.7 --- Add walkaround for fifo.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>

#include <cust_acc.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <accel.h>

#include <linux/wakelock.h>
#include <linux/jiffies.h>

#include "bma2x2.h"

/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_BMA2x2 255
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
/*apply low pass filter on output*/
/*#define CONFIG_BMA2x2_LOWPASS*/
#define SW_CALIBRATION
#define CONFIG_I2C_BASIC_FUNCTION

static struct mutex sensor_data_mutex;
static DECLARE_WAIT_QUEUE_HEAD(uplink_event_flag_wq);
static int CHIP_TYPE;

/*tad3sgh add --*/
#define MAX_FIFO_F_LEVEL 32
#define MAX_FIFO_F_BYTES 6
#define READFIFOCOUNT
#define DMAREAD
/*----------------------------------------------------------------------------*/
#define BMA2x2_AXIS_X          0
#define BMA2x2_AXIS_Y          1
#define BMA2x2_AXIS_Z          2
#define BMA2x2_AXES_NUM        3
#define BMA2x2_DATA_LEN        6
#define BMA2x2_DEV_NAME        "BMA255"

#define BMA2x2_MODE_NORMAL      0
#define BMA2x2_MODE_LOWPOWER    1
#define BMA2x2_MODE_SUSPEND     2

/*for bma222e chip*/
#define BMA222E_ACC_X_LSB__POS          0x00
#define BMA222E_ACC_X_LSB__LEN          0x00
#define BMA222E_ACC_X_LSB__MSK          0x00

#define BMA222E_ACC_X_MSB__POS           0
#define BMA222E_ACC_X_MSB__LEN           8
#define BMA222E_ACC_X_MSB__MSK           0xFF

#define BMA222E_ACC_Y_LSB__POS          0x00
#define BMA222E_ACC_Y_LSB__LEN          0x00
#define BMA222E_ACC_Y_LSB__MSK          0x00

#define BMA222E_ACC_Y_MSB__POS           0
#define BMA222E_ACC_Y_MSB__LEN           8
#define BMA222E_ACC_Y_MSB__MSK           0xFF

#define BMA222E_ACC_Z_LSB__POS          0x00
#define BMA222E_ACC_Z_LSB__LEN          0x00
#define BMA222E_ACC_Z_LSB__MSK          0x00

#define BMA222E_ACC_Z_MSB__POS           0
#define BMA222E_ACC_Z_MSB__LEN           8
#define BMA222E_ACC_Z_MSB__MSK           0xFF

/*for bma250e chip*/
#define BMA250E_ACC_X_LSB__POS          6
#define BMA250E_ACC_X_LSB__LEN          2
#define BMA250E_ACC_X_LSB__MSK          0xC0

#define BMA250E_ACC_X_MSB__POS           0
#define BMA250E_ACC_X_MSB__LEN           8
#define BMA250E_ACC_X_MSB__MSK           0xFF

#define BMA250E_ACC_Y_LSB__POS          6
#define BMA250E_ACC_Y_LSB__LEN          2
#define BMA250E_ACC_Y_LSB__MSK          0xC0

#define BMA250E_ACC_Y_MSB__POS           0
#define BMA250E_ACC_Y_MSB__LEN           8
#define BMA250E_ACC_Y_MSB__MSK           0xFF

#define BMA250E_ACC_Z_LSB__POS          6
#define BMA250E_ACC_Z_LSB__LEN          2
#define BMA250E_ACC_Z_LSB__MSK          0xC0

#define BMA250E_ACC_Z_MSB__POS           0
#define BMA250E_ACC_Z_MSB__LEN           8
#define BMA250E_ACC_Z_MSB__MSK           0xFF

/*for bma255 chip*/
#define BMA255_ACC_X_LSB__POS           4
#define BMA255_ACC_X_LSB__LEN           4
#define BMA255_ACC_X_LSB__MSK           0xF0

#define BMA255_ACC_X_MSB__POS           0
#define BMA255_ACC_X_MSB__LEN           8
#define BMA255_ACC_X_MSB__MSK           0xFF


#define BMA255_ACC_Y_LSB__POS           4
#define BMA255_ACC_Y_LSB__LEN           4
#define BMA255_ACC_Y_LSB__MSK           0xF0


#define BMA255_ACC_Y_MSB__POS           0
#define BMA255_ACC_Y_MSB__LEN           8
#define BMA255_ACC_Y_MSB__MSK           0xFF


#define BMA255_ACC_Z_LSB__POS           4
#define BMA255_ACC_Z_LSB__LEN           4
#define BMA255_ACC_Z_LSB__MSK           0xF0


#define BMA255_ACC_Z_MSB__POS           0
#define BMA255_ACC_Z_MSB__LEN           8
#define BMA255_ACC_Z_MSB__MSK           0xFF

/*for bma280*/
#define BMA280_ACC_X_LSB__POS           2
#define BMA280_ACC_X_LSB__LEN           6
#define BMA280_ACC_X_LSB__MSK           0xFC

#define BMA280_ACC_X_MSB__POS           0
#define BMA280_ACC_X_MSB__LEN           8
#define BMA280_ACC_X_MSB__MSK           0xFF

#define BMA280_ACC_Y_LSB__POS           2
#define BMA280_ACC_Y_LSB__LEN           6
#define BMA280_ACC_Y_LSB__MSK           0xFC

#define BMA280_ACC_Y_MSB__POS           0
#define BMA280_ACC_Y_MSB__LEN           8
#define BMA280_ACC_Y_MSB__MSK           0xFF

#define BMA280_ACC_Z_LSB__POS           2
#define BMA280_ACC_Z_LSB__LEN           6
#define BMA280_ACC_Z_LSB__MSK           0xFC

#define BMA280_ACC_Z_MSB__POS           0
#define BMA280_ACC_Z_MSB__LEN           8
#define BMA280_ACC_Z_MSB__MSK           0xFF

#define BMA2x2_EN_LOW_POWER__POS          6
#define BMA2x2_EN_LOW_POWER__LEN          1
#define BMA2x2_EN_LOW_POWER__MSK          0x40
#define BMA2x2_EN_LOW_POWER__REG          BMA2x2_REG_POWER_CTL

#define BMA2x2_EN_SUSPEND__POS            7
#define BMA2x2_EN_SUSPEND__LEN            1
#define BMA2x2_EN_SUSPEND__MSK            0x80
#define BMA2x2_EN_SUSPEND__REG            BMA2x2_REG_POWER_CTL

/* fifo mode*/
#define BMA2x2_FIFO_MODE__POS                 6
#define BMA2x2_FIFO_MODE__LEN                 2
#define BMA2x2_FIFO_MODE__MSK                 0xC0
#define BMA2x2_FIFO_MODE__REG                 BMA2x2_FIFO_MODE_REG

#define BMA2x2_FIFO_FRAME_COUNTER_S__POS             0
#define BMA2x2_FIFO_FRAME_COUNTER_S__LEN             7
#define BMA2x2_FIFO_FRAME_COUNTER_S__MSK             0x7F
#define BMA2x2_FIFO_FRAME_COUNTER_S__REG             BMA2x2_STATUS_FIFO_REG

#define BMA2x2_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define BMA2x2_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))


//add for combine BMI160, bma253  by cly
#ifdef CONFIG_LCT_BOOT_REASON
extern int lct_get_sku(void);
static int sku = 0;

#endif 

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS 
static struct platform_device *gsensorPltFmDev;
extern struct platform_device *get_gsensor_platformdev(void);
#endif 

#define ENABLE_ISR_DEBUG_MSG
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
#define ACC_NAME  "ACC"
#ifdef ENABLE_ISR_DEBUG_MSG
#define ISR_INFO(dev, fmt, arg...) dev_err(dev, fmt, ##arg)
#else
#define ISR_INFO(dev, fmt, arg...)
#endif

#define SENSOR_NAME                 "bma25x-accel"
#define ABSMIN                      -512
#define ABSMAX                      512
#define SLOPE_THRESHOLD_VALUE       32
#define SLOPE_DURATION_VALUE        1
#define INTERRUPT_LATCH_MODE        13
#define INTERRUPT_ENABLE            1
#define INTERRUPT_DISABLE           0
#define MAP_SLOPE_INTERRUPT         2
#define SLOPE_X_INDEX               5
#define SLOPE_Y_INDEX               6
#define SLOPE_Z_INDEX               7
#define BMA25X_RANGE_SET            3 /* +/- 2G */
#define BMA25X_RANGE_SHIFT          4 /* shift 4 bits for 2G */
#define BMA25X_BW_SET               12 /* 125HZ  */

#define I2C_RETRY_DELAY()           usleep_range(1000, 2000)
/* wait 2ms for calibration ready */
#define WAIT_CAL_READY()            usleep_range(2000, 2500)
/* >3ms wait device ready */
#define WAIT_DEVICE_READY()         usleep_range(3000, 5000)
/* >5ms for device reset */
#define RESET_DELAY()               usleep_range(5000, 10000)
/* wait 10ms for self test  done */
#define SELF_TEST_DELAY()           usleep_range(10000, 15000)

#define LOW_G_INTERRUPT            ABS_DISTANCE
#define HIGH_G_INTERRUPT            REL_HWHEEL
#define SLOP_INTERRUPT              REL_DIAL
#define DOUBLE_TAP_INTERRUPT        REL_WHEEL
#define SINGLE_TAP_INTERRUPT        REL_MISC
#define ORIENT_INTERRUPT            ABS_PRESSURE
#define FLAT_INTERRUPT               REL_Z
#define SLOW_NO_MOTION_INTERRUPT    REL_Y
#define REL_TIME_SEC		REL_RX
#define REL_TIME_NSEC	REL_RY
#define REL_FLUSH	REL_RZ
#define REL_INT_FLUSH	REL_X

#define HIGH_G_INTERRUPT_X_HAPPENED                 1
#define HIGH_G_INTERRUPT_Y_HAPPENED                 2
#define HIGH_G_INTERRUPT_Z_HAPPENED                 3
#define HIGH_G_INTERRUPT_X_NEGATIVE_HAPPENED        4
#define HIGH_G_INTERRUPT_Y_NEGATIVE_HAPPENED        5
#define HIGH_G_INTERRUPT_Z_NEGATIVE_HAPPENED        6
#define SLOPE_INTERRUPT_X_HAPPENED                  7
#define SLOPE_INTERRUPT_Y_HAPPENED                  8
#define SLOPE_INTERRUPT_Z_HAPPENED                  9
#define SLOPE_INTERRUPT_X_NEGATIVE_HAPPENED         10
#define SLOPE_INTERRUPT_Y_NEGATIVE_HAPPENED         11
#define SLOPE_INTERRUPT_Z_NEGATIVE_HAPPENED         12
#define DOUBLE_TAP_INTERRUPT_HAPPENED               13
#define SINGLE_TAP_INTERRUPT_HAPPENED               14
#define UPWARD_PORTRAIT_UP_INTERRUPT_HAPPENED       15
#define UPWARD_PORTRAIT_DOWN_INTERRUPT_HAPPENED     16
#define UPWARD_LANDSCAPE_LEFT_INTERRUPT_HAPPENED    17
#define UPWARD_LANDSCAPE_RIGHT_INTERRUPT_HAPPENED   18
#define DOWNWARD_PORTRAIT_UP_INTERRUPT_HAPPENED     19
#define DOWNWARD_PORTRAIT_DOWN_INTERRUPT_HAPPENED   20
#define DOWNWARD_LANDSCAPE_LEFT_INTERRUPT_HAPPENED  21
#define DOWNWARD_LANDSCAPE_RIGHT_INTERRUPT_HAPPENED 22
#define FLAT_INTERRUPT_TURE_HAPPENED                23
#define FLAT_INTERRUPT_FALSE_HAPPENED               24
#define LOW_G_INTERRUPT_HAPPENED                    25
#define SLOW_NO_MOTION_INTERRUPT_HAPPENED           26

#define FLATUP_GESTURE 0xF2
#define FLATDOWN_GESTURE 0xF4
#define EXIT_FLATUP_GESTURE 0xE2
#define EXIT_FLATDOWN_GESTURE 0xE4
#define GLANCE_EXIT_FLATUP_GESTURE 0x02
#define GLANCE_EXIT_FLATDOWN_GESTURE 0x04
#define GLANCE_MOVEMENT_GESTURE 0x10

#define PAD_LOWG                    0
#define PAD_HIGHG                   1
#define PAD_SLOP                    2
#define PAD_DOUBLE_TAP              3
#define PAD_SINGLE_TAP              4
#define PAD_ORIENT                  5
#define PAD_FLAT                    6
#define PAD_SLOW_NO_MOTION          7

#define BMA25X_EEP_OFFSET                       0x16
#define BMA25X_IMAGE_BASE                       0x38
#define BMA25X_IMAGE_LEN                        22

#define BMA25X_CHIP_ID_REG                      0x00
#define BMA25X_VERSION_REG                      0x01
#define BMA25X_X_AXIS_LSB_REG                   0x02
#define BMA25X_X_AXIS_MSB_REG                   0x03
#define BMA25X_Y_AXIS_LSB_REG                   0x04
#define BMA25X_Y_AXIS_MSB_REG                   0x05
#define BMA25X_Z_AXIS_LSB_REG                   0x06
#define BMA25X_Z_AXIS_MSB_REG                   0x07
#define BMA25X_TEMPERATURE_REG                  0x08
#define BMA25X_STATUS1_REG                      0x09
#define BMA25X_STATUS2_REG                      0x0A
#define BMA25X_STATUS_TAP_SLOPE_REG             0x0B
#define BMA25X_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA25X_STATUS_FIFO_REG                  0x0E
#define BMA25X_RANGE_SEL_REG                    0x0F
#define BMA25X_BW_SEL_REG                       0x10
#define BMA25X_MODE_CTRL_REG                    0x11
#define BMA25X_LOW_NOISE_CTRL_REG               0x12
#define BMA25X_DATA_CTRL_REG                    0x13
#define BMA25X_RESET_REG                        0x14
#define BMA25X_INT_ENABLE1_REG                  0x16
#define BMA25X_INT_ENABLE2_REG                  0x17
#define BMA25X_INT_SLO_NO_MOT_REG               0x18
#define BMA25X_INT1_PAD_SEL_REG                 0x19
#define BMA25X_INT_DATA_SEL_REG                 0x1A
#define BMA25X_INT2_PAD_SEL_REG                 0x1B
#define BMA25X_INT_SRC_REG                      0x1E
#define BMA25X_INT_SET_REG                      0x20
#define BMA25X_INT_CTRL_REG                     0x21
#define BMA25X_LOW_DURN_REG                     0x22
#define BMA25X_LOW_THRES_REG                    0x23
#define BMA25X_LOW_HIGH_HYST_REG                0x24
#define BMA25X_HIGH_DURN_REG                    0x25
#define BMA25X_HIGH_THRES_REG                   0x26
#define BMA25X_SLOPE_DURN_REG                   0x27
#define BMA25X_SLOPE_THRES_REG                  0x28
#define BMA25X_SLO_NO_MOT_THRES_REG             0x29
#define BMA25X_TAP_PARAM_REG                    0x2A
#define BMA25X_TAP_THRES_REG                    0x2B
#define BMA25X_ORIENT_PARAM_REG                 0x2C
#define BMA25X_THETA_BLOCK_REG                  0x2D
#define BMA25X_THETA_FLAT_REG                   0x2E
#define BMA25X_FLAT_HOLD_TIME_REG               0x2F
#define BMA25X_FIFO_WML_TRIG                    0x30
#define BMA25X_SELF_TEST_REG                    0x32
#define BMA25X_EEPROM_CTRL_REG                  0x33
#define BMA25X_SERIAL_CTRL_REG                  0x34
#define BMA25X_EXTMODE_CTRL_REG                 0x35
#define BMA25X_OFFSET_CTRL_REG                  0x36
#define BMA25X_OFFSET_PARAMS_REG                0x37
#define BMA25X_OFFSET_X_AXIS_REG                0x38
#define BMA25X_OFFSET_Y_AXIS_REG                0x39
#define BMA25X_OFFSET_Z_AXIS_REG                0x3A
#define BMA25X_GP0_REG                          0x3B
#define BMA25X_GP1_REG                          0x3C
#define BMA25X_FIFO_MODE_REG                    0x3E
#define BMA25X_FIFO_DATA_OUTPUT_REG             0x3F

#define BMA25X_CHIP_ID__POS             0
#define BMA25X_CHIP_ID__MSK             0xFF
#define BMA25X_CHIP_ID__LEN             8
#define BMA25X_CHIP_ID__REG             BMA25X_CHIP_ID_REG

#define BMA25X_VERSION__POS          0
#define BMA25X_VERSION__LEN          8
#define BMA25X_VERSION__MSK          0xFF
#define BMA25X_VERSION__REG          BMA25X_VERSION_REG

#define BMA25X_SLO_NO_MOT_DUR__POS   2
#define BMA25X_SLO_NO_MOT_DUR__LEN   6
#define BMA25X_SLO_NO_MOT_DUR__MSK   0xFC
#define BMA25X_SLO_NO_MOT_DUR__REG   BMA25X_SLOPE_DURN_REG

#define BMA25X_NEW_DATA_X__POS          0
#define BMA25X_NEW_DATA_X__LEN          1
#define BMA25X_NEW_DATA_X__MSK          0x01
#define BMA25X_NEW_DATA_X__REG          BMA25X_X_AXIS_LSB_REG

#define BMA25X_ACC_X14_LSB__POS           2
#define BMA25X_ACC_X14_LSB__LEN           6
#define BMA25X_ACC_X14_LSB__MSK           0xFC
#define BMA25X_ACC_X14_LSB__REG           BMA25X_X_AXIS_LSB_REG

#define BMA25X_ACC_X12_LSB__POS           4
#define BMA25X_ACC_X12_LSB__LEN           4
#define BMA25X_ACC_X12_LSB__MSK           0xF0
#define BMA25X_ACC_X12_LSB__REG           BMA25X_X_AXIS_LSB_REG

#define BMA25X_ACC_X10_LSB__POS           6
#define BMA25X_ACC_X10_LSB__LEN           2
#define BMA25X_ACC_X10_LSB__MSK           0xC0
#define BMA25X_ACC_X10_LSB__REG           BMA25X_X_AXIS_LSB_REG

#define BMA25X_ACC_X8_LSB__POS           0
#define BMA25X_ACC_X8_LSB__LEN           0
#define BMA25X_ACC_X8_LSB__MSK           0x00
#define BMA25X_ACC_X8_LSB__REG           BMA25X_X_AXIS_LSB_REG

#define BMA25X_ACC_X_MSB__POS           0
#define BMA25X_ACC_X_MSB__LEN           8
#define BMA25X_ACC_X_MSB__MSK           0xFF
#define BMA25X_ACC_X_MSB__REG           BMA25X_X_AXIS_MSB_REG

#define BMA25X_NEW_DATA_Y__POS          0
#define BMA25X_NEW_DATA_Y__LEN          1
#define BMA25X_NEW_DATA_Y__MSK          0x01
#define BMA25X_NEW_DATA_Y__REG          BMA25X_Y_AXIS_LSB_REG

#define BMA25X_ACC_Y14_LSB__POS           2
#define BMA25X_ACC_Y14_LSB__LEN           6
#define BMA25X_ACC_Y14_LSB__MSK           0xFC
#define BMA25X_ACC_Y14_LSB__REG           BMA25X_Y_AXIS_LSB_REG

#define BMA25X_ACC_Y12_LSB__POS           4
#define BMA25X_ACC_Y12_LSB__LEN           4
#define BMA25X_ACC_Y12_LSB__MSK           0xF0
#define BMA25X_ACC_Y12_LSB__REG           BMA25X_Y_AXIS_LSB_REG

#define BMA25X_ACC_Y10_LSB__POS           6
#define BMA25X_ACC_Y10_LSB__LEN           2
#define BMA25X_ACC_Y10_LSB__MSK           0xC0
#define BMA25X_ACC_Y10_LSB__REG           BMA25X_Y_AXIS_LSB_REG

#define BMA25X_ACC_Y8_LSB__POS           0
#define BMA25X_ACC_Y8_LSB__LEN           0
#define BMA25X_ACC_Y8_LSB__MSK           0x00
#define BMA25X_ACC_Y8_LSB__REG           BMA25X_Y_AXIS_LSB_REG

#define BMA25X_ACC_Y_MSB__POS           0
#define BMA25X_ACC_Y_MSB__LEN           8
#define BMA25X_ACC_Y_MSB__MSK           0xFF
#define BMA25X_ACC_Y_MSB__REG           BMA25X_Y_AXIS_MSB_REG

#define BMA25X_NEW_DATA_Z__POS          0
#define BMA25X_NEW_DATA_Z__LEN          1
#define BMA25X_NEW_DATA_Z__MSK          0x01
#define BMA25X_NEW_DATA_Z__REG          BMA25X_Z_AXIS_LSB_REG

#define BMA25X_ACC_Z14_LSB__POS           2
#define BMA25X_ACC_Z14_LSB__LEN           6
#define BMA25X_ACC_Z14_LSB__MSK           0xFC
#define BMA25X_ACC_Z14_LSB__REG           BMA25X_Z_AXIS_LSB_REG

#define BMA25X_ACC_Z12_LSB__POS           4
#define BMA25X_ACC_Z12_LSB__LEN           4
#define BMA25X_ACC_Z12_LSB__MSK           0xF0
#define BMA25X_ACC_Z12_LSB__REG           BMA25X_Z_AXIS_LSB_REG

#define BMA25X_ACC_Z10_LSB__POS           6
#define BMA25X_ACC_Z10_LSB__LEN           2
#define BMA25X_ACC_Z10_LSB__MSK           0xC0
#define BMA25X_ACC_Z10_LSB__REG           BMA25X_Z_AXIS_LSB_REG

#define BMA25X_ACC_Z8_LSB__POS           0
#define BMA25X_ACC_Z8_LSB__LEN           0
#define BMA25X_ACC_Z8_LSB__MSK           0x00
#define BMA25X_ACC_Z8_LSB__REG           BMA25X_Z_AXIS_LSB_REG

#define BMA25X_ACC_Z_MSB__POS           0
#define BMA25X_ACC_Z_MSB__LEN           8
#define BMA25X_ACC_Z_MSB__MSK           0xFF
#define BMA25X_ACC_Z_MSB__REG           BMA25X_Z_AXIS_MSB_REG

#define BMA25X_TEMPERATURE__POS         0
#define BMA25X_TEMPERATURE__LEN         8
#define BMA25X_TEMPERATURE__MSK         0xFF
#define BMA25X_TEMPERATURE__REG         BMA25X_TEMP_RD_REG

#define BMA25X_LOWG_INT_S__POS          0
#define BMA25X_LOWG_INT_S__LEN          1
#define BMA25X_LOWG_INT_S__MSK          0x01
#define BMA25X_LOWG_INT_S__REG          BMA25X_STATUS1_REG

#define BMA25X_HIGHG_INT_S__POS          1
#define BMA25X_HIGHG_INT_S__LEN          1
#define BMA25X_HIGHG_INT_S__MSK          0x02
#define BMA25X_HIGHG_INT_S__REG          BMA25X_STATUS1_REG

#define BMA25X_SLOPE_INT_S__POS          2
#define BMA25X_SLOPE_INT_S__LEN          1
#define BMA25X_SLOPE_INT_S__MSK          0x04
#define BMA25X_SLOPE_INT_S__REG          BMA25X_STATUS1_REG


#define BMA25X_SLO_NO_MOT_INT_S__POS          3
#define BMA25X_SLO_NO_MOT_INT_S__LEN          1
#define BMA25X_SLO_NO_MOT_INT_S__MSK          0x08
#define BMA25X_SLO_NO_MOT_INT_S__REG          BMA25X_STATUS1_REG

#define BMA25X_DOUBLE_TAP_INT_S__POS     4
#define BMA25X_DOUBLE_TAP_INT_S__LEN     1
#define BMA25X_DOUBLE_TAP_INT_S__MSK     0x10
#define BMA25X_DOUBLE_TAP_INT_S__REG     BMA25X_STATUS1_REG

#define BMA25X_SINGLE_TAP_INT_S__POS     5
#define BMA25X_SINGLE_TAP_INT_S__LEN     1
#define BMA25X_SINGLE_TAP_INT_S__MSK     0x20
#define BMA25X_SINGLE_TAP_INT_S__REG     BMA25X_STATUS1_REG

#define BMA25X_ORIENT_INT_S__POS         6
#define BMA25X_ORIENT_INT_S__LEN         1
#define BMA25X_ORIENT_INT_S__MSK         0x40
#define BMA25X_ORIENT_INT_S__REG         BMA25X_STATUS1_REG

#define BMA25X_FLAT_INT_S__POS           7
#define BMA25X_FLAT_INT_S__LEN           1
#define BMA25X_FLAT_INT_S__MSK           0x80
#define BMA25X_FLAT_INT_S__REG           BMA25X_STATUS1_REG

#define BMA25X_FIFO_FULL_INT_S__POS           5
#define BMA25X_FIFO_FULL_INT_S__LEN           1
#define BMA25X_FIFO_FULL_INT_S__MSK           0x20
#define BMA25X_FIFO_FULL_INT_S__REG           BMA25X_STATUS2_REG

#define BMA25X_FIFO_WM_INT_S__POS           6
#define BMA25X_FIFO_WM_INT_S__LEN           1
#define BMA25X_FIFO_WM_INT_S__MSK           0x40
#define BMA25X_FIFO_WM_INT_S__REG           BMA25X_STATUS2_REG

#define BMA25X_DATA_INT_S__POS           7
#define BMA25X_DATA_INT_S__LEN           1
#define BMA25X_DATA_INT_S__MSK           0x80
#define BMA25X_DATA_INT_S__REG           BMA25X_STATUS2_REG

#define BMA25X_SLOPE_FIRST_X__POS        0
#define BMA25X_SLOPE_FIRST_X__LEN        1
#define BMA25X_SLOPE_FIRST_X__MSK        0x01
#define BMA25X_SLOPE_FIRST_X__REG        BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_SLOPE_FIRST_Y__POS        1
#define BMA25X_SLOPE_FIRST_Y__LEN        1
#define BMA25X_SLOPE_FIRST_Y__MSK        0x02
#define BMA25X_SLOPE_FIRST_Y__REG        BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_SLOPE_FIRST_Z__POS        2
#define BMA25X_SLOPE_FIRST_Z__LEN        1
#define BMA25X_SLOPE_FIRST_Z__MSK        0x04
#define BMA25X_SLOPE_FIRST_Z__REG        BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_SLOPE_SIGN_S__POS         3
#define BMA25X_SLOPE_SIGN_S__LEN         1
#define BMA25X_SLOPE_SIGN_S__MSK         0x08
#define BMA25X_SLOPE_SIGN_S__REG         BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_TAP_FIRST_X__POS        4
#define BMA25X_TAP_FIRST_X__LEN        1
#define BMA25X_TAP_FIRST_X__MSK        0x10
#define BMA25X_TAP_FIRST_X__REG        BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_TAP_FIRST_Y__POS        5
#define BMA25X_TAP_FIRST_Y__LEN        1
#define BMA25X_TAP_FIRST_Y__MSK        0x20
#define BMA25X_TAP_FIRST_Y__REG        BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_TAP_FIRST_Z__POS        6
#define BMA25X_TAP_FIRST_Z__LEN        1
#define BMA25X_TAP_FIRST_Z__MSK        0x40
#define BMA25X_TAP_FIRST_Z__REG        BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_TAP_SIGN_S__POS         7
#define BMA25X_TAP_SIGN_S__LEN         1
#define BMA25X_TAP_SIGN_S__MSK         0x80
#define BMA25X_TAP_SIGN_S__REG         BMA25X_STATUS_TAP_SLOPE_REG

#define BMA25X_HIGHG_FIRST_X__POS        0
#define BMA25X_HIGHG_FIRST_X__LEN        1
#define BMA25X_HIGHG_FIRST_X__MSK        0x01
#define BMA25X_HIGHG_FIRST_X__REG        BMA25X_STATUS_ORIENT_HIGH_REG

#define BMA25X_HIGHG_FIRST_Y__POS        1
#define BMA25X_HIGHG_FIRST_Y__LEN        1
#define BMA25X_HIGHG_FIRST_Y__MSK        0x02
#define BMA25X_HIGHG_FIRST_Y__REG        BMA25X_STATUS_ORIENT_HIGH_REG

#define BMA25X_HIGHG_FIRST_Z__POS        2
#define BMA25X_HIGHG_FIRST_Z__LEN        1
#define BMA25X_HIGHG_FIRST_Z__MSK        0x04
#define BMA25X_HIGHG_FIRST_Z__REG        BMA25X_STATUS_ORIENT_HIGH_REG

#define BMA25X_HIGHG_SIGN_S__POS         3
#define BMA25X_HIGHG_SIGN_S__LEN         1
#define BMA25X_HIGHG_SIGN_S__MSK         0x08
#define BMA25X_HIGHG_SIGN_S__REG         BMA25X_STATUS_ORIENT_HIGH_REG

#define BMA25X_ORIENT_S__POS             4
#define BMA25X_ORIENT_S__LEN             3
#define BMA25X_ORIENT_S__MSK             0x70
#define BMA25X_ORIENT_S__REG             BMA25X_STATUS_ORIENT_HIGH_REG

#define BMA25X_FLAT_S__POS               7
#define BMA25X_FLAT_S__LEN               1
#define BMA25X_FLAT_S__MSK               0x80
#define BMA25X_FLAT_S__REG               BMA25X_STATUS_ORIENT_HIGH_REG

#define BMA25X_FIFO_FRAME_COUNTER_S__POS             0
#define BMA25X_FIFO_FRAME_COUNTER_S__LEN             7
#define BMA25X_FIFO_FRAME_COUNTER_S__MSK             0x7F
#define BMA25X_FIFO_FRAME_COUNTER_S__REG             BMA25X_STATUS_FIFO_REG

#define BMA25X_FIFO_OVERRUN_S__POS             7
#define BMA25X_FIFO_OVERRUN_S__LEN             1
#define BMA25X_FIFO_OVERRUN_S__MSK             0x80
#define BMA25X_FIFO_OVERRUN_S__REG             BMA25X_STATUS_FIFO_REG

#define BMA25X_RANGE_SEL__POS             0
#define BMA25X_RANGE_SEL__LEN             4
#define BMA25X_RANGE_SEL__MSK             0x0F
#define BMA25X_RANGE_SEL__REG             BMA25X_RANGE_SEL_REG

#define BMA25X_BANDWIDTH__POS             0
#define BMA25X_BANDWIDTH__LEN             5
#define BMA25X_BANDWIDTH__MSK             0x1F
#define BMA25X_BANDWIDTH__REG             BMA25X_BW_SEL_REG

#define BMA25X_SLEEP_DUR__POS             1
#define BMA25X_SLEEP_DUR__LEN             4
#define BMA25X_SLEEP_DUR__MSK             0x1E
#define BMA25X_SLEEP_DUR__REG             BMA25X_MODE_CTRL_REG

#define BMA25X_MODE_CTRL__POS             5
#define BMA25X_MODE_CTRL__LEN             3
#define BMA25X_MODE_CTRL__MSK             0xE0
#define BMA25X_MODE_CTRL__REG             BMA25X_MODE_CTRL_REG

#define BMA25X_DEEP_SUSPEND__POS          5
#define BMA25X_DEEP_SUSPEND__LEN          1
#define BMA25X_DEEP_SUSPEND__MSK          0x20
#define BMA25X_DEEP_SUSPEND__REG          BMA25X_MODE_CTRL_REG

#define BMA25X_EN_LOW_POWER__POS          6
#define BMA25X_EN_LOW_POWER__LEN          1
#define BMA25X_EN_LOW_POWER__MSK          0x40
#define BMA25X_EN_LOW_POWER__REG          BMA25X_MODE_CTRL_REG

#define BMA25X_EN_SUSPEND__POS            7
#define BMA25X_EN_SUSPEND__LEN            1
#define BMA25X_EN_SUSPEND__MSK            0x80
#define BMA25X_EN_SUSPEND__REG            BMA25X_MODE_CTRL_REG

#define BMA25X_SLEEP_TIMER__POS          5
#define BMA25X_SLEEP_TIMER__LEN          1
#define BMA25X_SLEEP_TIMER__MSK          0x20
#define BMA25X_SLEEP_TIMER__REG          BMA25X_LOW_NOISE_CTRL_REG

#define BMA25X_LOW_POWER_MODE__POS          6
#define BMA25X_LOW_POWER_MODE__LEN          1
#define BMA25X_LOW_POWER_MODE__MSK          0x40
#define BMA25X_LOW_POWER_MODE__REG          BMA25X_LOW_NOISE_CTRL_REG

#define BMA25X_EN_LOW_NOISE__POS          7
#define BMA25X_EN_LOW_NOISE__LEN          1
#define BMA25X_EN_LOW_NOISE__MSK          0x80
#define BMA25X_EN_LOW_NOISE__REG          BMA25X_LOW_NOISE_CTRL_REG

#define BMA25X_DIS_SHADOW_PROC__POS       6
#define BMA25X_DIS_SHADOW_PROC__LEN       1
#define BMA25X_DIS_SHADOW_PROC__MSK       0x40
#define BMA25X_DIS_SHADOW_PROC__REG       BMA25X_DATA_CTRL_REG

#define BMA25X_EN_DATA_HIGH_BW__POS         7
#define BMA25X_EN_DATA_HIGH_BW__LEN         1
#define BMA25X_EN_DATA_HIGH_BW__MSK         0x80
#define BMA25X_EN_DATA_HIGH_BW__REG         BMA25X_DATA_CTRL_REG

#define BMA25X_EN_SOFT_RESET__POS         0
#define BMA25X_EN_SOFT_RESET__LEN         8
#define BMA25X_EN_SOFT_RESET__MSK         0xFF
#define BMA25X_EN_SOFT_RESET__REG         BMA25X_RESET_REG

#define BMA25X_EN_SOFT_RESET_VALUE        0xB6

#define BMA25X_EN_SLOPE_X_INT__POS         0
#define BMA25X_EN_SLOPE_X_INT__LEN         1
#define BMA25X_EN_SLOPE_X_INT__MSK         0x01
#define BMA25X_EN_SLOPE_X_INT__REG         BMA25X_INT_ENABLE1_REG

#define BMA25X_EN_SLOPE_Y_INT__POS         1
#define BMA25X_EN_SLOPE_Y_INT__LEN         1
#define BMA25X_EN_SLOPE_Y_INT__MSK         0x02
#define BMA25X_EN_SLOPE_Y_INT__REG         BMA25X_INT_ENABLE1_REG

#define BMA25X_EN_SLOPE_Z_INT__POS         2
#define BMA25X_EN_SLOPE_Z_INT__LEN         1
#define BMA25X_EN_SLOPE_Z_INT__MSK         0x04
#define BMA25X_EN_SLOPE_Z_INT__REG         BMA25X_INT_ENABLE1_REG

#define BMA25X_EN_DOUBLE_TAP_INT__POS      4
#define BMA25X_EN_DOUBLE_TAP_INT__LEN      1
#define BMA25X_EN_DOUBLE_TAP_INT__MSK      0x10
#define BMA25X_EN_DOUBLE_TAP_INT__REG      BMA25X_INT_ENABLE1_REG

#define BMA25X_EN_SINGLE_TAP_INT__POS      5
#define BMA25X_EN_SINGLE_TAP_INT__LEN      1
#define BMA25X_EN_SINGLE_TAP_INT__MSK      0x20
#define BMA25X_EN_SINGLE_TAP_INT__REG      BMA25X_INT_ENABLE1_REG

#define BMA25X_EN_ORIENT_INT__POS          6
#define BMA25X_EN_ORIENT_INT__LEN          1
#define BMA25X_EN_ORIENT_INT__MSK          0x40
#define BMA25X_EN_ORIENT_INT__REG          BMA25X_INT_ENABLE1_REG

#define BMA25X_EN_FLAT_INT__POS            7
#define BMA25X_EN_FLAT_INT__LEN            1
#define BMA25X_EN_FLAT_INT__MSK            0x80
#define BMA25X_EN_FLAT_INT__REG            BMA25X_INT_ENABLE1_REG

#define BMA25X_EN_HIGHG_X_INT__POS         0
#define BMA25X_EN_HIGHG_X_INT__LEN         1
#define BMA25X_EN_HIGHG_X_INT__MSK         0x01
#define BMA25X_EN_HIGHG_X_INT__REG         BMA25X_INT_ENABLE2_REG

#define BMA25X_EN_HIGHG_Y_INT__POS         1
#define BMA25X_EN_HIGHG_Y_INT__LEN         1
#define BMA25X_EN_HIGHG_Y_INT__MSK         0x02
#define BMA25X_EN_HIGHG_Y_INT__REG         BMA25X_INT_ENABLE2_REG

#define BMA25X_EN_HIGHG_Z_INT__POS         2
#define BMA25X_EN_HIGHG_Z_INT__LEN         1
#define BMA25X_EN_HIGHG_Z_INT__MSK         0x04
#define BMA25X_EN_HIGHG_Z_INT__REG         BMA25X_INT_ENABLE2_REG

#define BMA25X_EN_LOWG_INT__POS            3
#define BMA25X_EN_LOWG_INT__LEN            1
#define BMA25X_EN_LOWG_INT__MSK            0x08
#define BMA25X_EN_LOWG_INT__REG            BMA25X_INT_ENABLE2_REG

#define BMA25X_EN_NEW_DATA_INT__POS        4
#define BMA25X_EN_NEW_DATA_INT__LEN        1
#define BMA25X_EN_NEW_DATA_INT__MSK        0x10
#define BMA25X_EN_NEW_DATA_INT__REG        BMA25X_INT_ENABLE2_REG

#define BMA25X_INT_FFULL_EN_INT__POS        5
#define BMA25X_INT_FFULL_EN_INT__LEN        1
#define BMA25X_INT_FFULL_EN_INT__MSK        0x20
#define BMA25X_INT_FFULL_EN_INT__REG        BMA25X_INT_ENABLE2_REG

#define BMA25X_INT_FWM_EN_INT__POS        6
#define BMA25X_INT_FWM_EN_INT__LEN        1
#define BMA25X_INT_FWM_EN_INT__MSK        0x40
#define BMA25X_INT_FWM_EN_INT__REG        BMA25X_INT_ENABLE2_REG

#define BMA25X_INT_SLO_NO_MOT_EN_X_INT__POS        0
#define BMA25X_INT_SLO_NO_MOT_EN_X_INT__LEN        1
#define BMA25X_INT_SLO_NO_MOT_EN_X_INT__MSK        0x01
#define BMA25X_INT_SLO_NO_MOT_EN_X_INT__REG        BMA25X_INT_SLO_NO_MOT_REG

#define BMA25X_INT_SLO_NO_MOT_EN_Y_INT__POS        1
#define BMA25X_INT_SLO_NO_MOT_EN_Y_INT__LEN        1
#define BMA25X_INT_SLO_NO_MOT_EN_Y_INT__MSK        0x02
#define BMA25X_INT_SLO_NO_MOT_EN_Y_INT__REG        BMA25X_INT_SLO_NO_MOT_REG

#define BMA25X_INT_SLO_NO_MOT_EN_Z_INT__POS        2
#define BMA25X_INT_SLO_NO_MOT_EN_Z_INT__LEN        1
#define BMA25X_INT_SLO_NO_MOT_EN_Z_INT__MSK        0x04
#define BMA25X_INT_SLO_NO_MOT_EN_Z_INT__REG        BMA25X_INT_SLO_NO_MOT_REG

#define BMA25X_INT_SLO_NO_MOT_EN_SEL_INT__POS        3
#define BMA25X_INT_SLO_NO_MOT_EN_SEL_INT__LEN        1
#define BMA25X_INT_SLO_NO_MOT_EN_SEL_INT__MSK        0x08
#define BMA25X_INT_SLO_NO_MOT_EN_SEL_INT__REG        BMA25X_INT_SLO_NO_MOT_REG

#define BMA25X_EN_INT1_PAD_LOWG__POS        0
#define BMA25X_EN_INT1_PAD_LOWG__LEN        1
#define BMA25X_EN_INT1_PAD_LOWG__MSK        0x01
#define BMA25X_EN_INT1_PAD_LOWG__REG        BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_HIGHG__POS       1
#define BMA25X_EN_INT1_PAD_HIGHG__LEN       1
#define BMA25X_EN_INT1_PAD_HIGHG__MSK       0x02
#define BMA25X_EN_INT1_PAD_HIGHG__REG       BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_SLOPE__POS       2
#define BMA25X_EN_INT1_PAD_SLOPE__LEN       1
#define BMA25X_EN_INT1_PAD_SLOPE__MSK       0x04
#define BMA25X_EN_INT1_PAD_SLOPE__REG       BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_SLO_NO_MOT__POS        3
#define BMA25X_EN_INT1_PAD_SLO_NO_MOT__LEN        1
#define BMA25X_EN_INT1_PAD_SLO_NO_MOT__MSK        0x08
#define BMA25X_EN_INT1_PAD_SLO_NO_MOT__REG        BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_DB_TAP__POS      4
#define BMA25X_EN_INT1_PAD_DB_TAP__LEN      1
#define BMA25X_EN_INT1_PAD_DB_TAP__MSK      0x10
#define BMA25X_EN_INT1_PAD_DB_TAP__REG      BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_SNG_TAP__POS     5
#define BMA25X_EN_INT1_PAD_SNG_TAP__LEN     1
#define BMA25X_EN_INT1_PAD_SNG_TAP__MSK     0x20
#define BMA25X_EN_INT1_PAD_SNG_TAP__REG     BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_ORIENT__POS      6
#define BMA25X_EN_INT1_PAD_ORIENT__LEN      1
#define BMA25X_EN_INT1_PAD_ORIENT__MSK      0x40
#define BMA25X_EN_INT1_PAD_ORIENT__REG      BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_FLAT__POS        7
#define BMA25X_EN_INT1_PAD_FLAT__LEN        1
#define BMA25X_EN_INT1_PAD_FLAT__MSK        0x80
#define BMA25X_EN_INT1_PAD_FLAT__REG        BMA25X_INT1_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_LOWG__POS        0
#define BMA25X_EN_INT2_PAD_LOWG__LEN        1
#define BMA25X_EN_INT2_PAD_LOWG__MSK        0x01
#define BMA25X_EN_INT2_PAD_LOWG__REG        BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_HIGHG__POS       1
#define BMA25X_EN_INT2_PAD_HIGHG__LEN       1
#define BMA25X_EN_INT2_PAD_HIGHG__MSK       0x02
#define BMA25X_EN_INT2_PAD_HIGHG__REG       BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_SLOPE__POS       2
#define BMA25X_EN_INT2_PAD_SLOPE__LEN       1
#define BMA25X_EN_INT2_PAD_SLOPE__MSK       0x04
#define BMA25X_EN_INT2_PAD_SLOPE__REG       BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_SLO_NO_MOT__POS        3
#define BMA25X_EN_INT2_PAD_SLO_NO_MOT__LEN        1
#define BMA25X_EN_INT2_PAD_SLO_NO_MOT__MSK        0x08
#define BMA25X_EN_INT2_PAD_SLO_NO_MOT__REG        BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_DB_TAP__POS      4
#define BMA25X_EN_INT2_PAD_DB_TAP__LEN      1
#define BMA25X_EN_INT2_PAD_DB_TAP__MSK      0x10
#define BMA25X_EN_INT2_PAD_DB_TAP__REG      BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_SNG_TAP__POS     5
#define BMA25X_EN_INT2_PAD_SNG_TAP__LEN     1
#define BMA25X_EN_INT2_PAD_SNG_TAP__MSK     0x20
#define BMA25X_EN_INT2_PAD_SNG_TAP__REG     BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_ORIENT__POS      6
#define BMA25X_EN_INT2_PAD_ORIENT__LEN      1
#define BMA25X_EN_INT2_PAD_ORIENT__MSK      0x40
#define BMA25X_EN_INT2_PAD_ORIENT__REG      BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT2_PAD_FLAT__POS        7
#define BMA25X_EN_INT2_PAD_FLAT__LEN        1
#define BMA25X_EN_INT2_PAD_FLAT__MSK        0x80
#define BMA25X_EN_INT2_PAD_FLAT__REG        BMA25X_INT2_PAD_SEL_REG

#define BMA25X_EN_INT1_PAD_NEWDATA__POS     0
#define BMA25X_EN_INT1_PAD_NEWDATA__LEN     1
#define BMA25X_EN_INT1_PAD_NEWDATA__MSK     0x01
#define BMA25X_EN_INT1_PAD_NEWDATA__REG     BMA25X_INT_DATA_SEL_REG

#define BMA25X_EN_INT1_PAD_FWM__POS     1
#define BMA25X_EN_INT1_PAD_FWM__LEN     1
#define BMA25X_EN_INT1_PAD_FWM__MSK     0x02
#define BMA25X_EN_INT1_PAD_FWM__REG     BMA25X_INT_DATA_SEL_REG

#define BMA25X_EN_INT1_PAD_FFULL__POS     2
#define BMA25X_EN_INT1_PAD_FFULL__LEN     1
#define BMA25X_EN_INT1_PAD_FFULL__MSK     0x04
#define BMA25X_EN_INT1_PAD_FFULL__REG     BMA25X_INT_DATA_SEL_REG

#define BMA25X_EN_INT2_PAD_FFULL__POS     5
#define BMA25X_EN_INT2_PAD_FFULL__LEN     1
#define BMA25X_EN_INT2_PAD_FFULL__MSK     0x20
#define BMA25X_EN_INT2_PAD_FFULL__REG     BMA25X_INT_DATA_SEL_REG

#define BMA25X_EN_INT2_PAD_FWM__POS     6
#define BMA25X_EN_INT2_PAD_FWM__LEN     1
#define BMA25X_EN_INT2_PAD_FWM__MSK     0x40
#define BMA25X_EN_INT2_PAD_FWM__REG     BMA25X_INT_DATA_SEL_REG

#define BMA25X_EN_INT2_PAD_NEWDATA__POS     7
#define BMA25X_EN_INT2_PAD_NEWDATA__LEN     1
#define BMA25X_EN_INT2_PAD_NEWDATA__MSK     0x80
#define BMA25X_EN_INT2_PAD_NEWDATA__REG     BMA25X_INT_DATA_SEL_REG

#define BMA25X_UNFILT_INT_SRC_LOWG__POS        0
#define BMA25X_UNFILT_INT_SRC_LOWG__LEN        1
#define BMA25X_UNFILT_INT_SRC_LOWG__MSK        0x01
#define BMA25X_UNFILT_INT_SRC_LOWG__REG        BMA25X_INT_SRC_REG

#define BMA25X_UNFILT_INT_SRC_HIGHG__POS       1
#define BMA25X_UNFILT_INT_SRC_HIGHG__LEN       1
#define BMA25X_UNFILT_INT_SRC_HIGHG__MSK       0x02
#define BMA25X_UNFILT_INT_SRC_HIGHG__REG       BMA25X_INT_SRC_REG

#define BMA25X_UNFILT_INT_SRC_SLOPE__POS       2
#define BMA25X_UNFILT_INT_SRC_SLOPE__LEN       1
#define BMA25X_UNFILT_INT_SRC_SLOPE__MSK       0x04
#define BMA25X_UNFILT_INT_SRC_SLOPE__REG       BMA25X_INT_SRC_REG

#define BMA25X_UNFILT_INT_SRC_SLO_NO_MOT__POS        3
#define BMA25X_UNFILT_INT_SRC_SLO_NO_MOT__LEN        1
#define BMA25X_UNFILT_INT_SRC_SLO_NO_MOT__MSK        0x08
#define BMA25X_UNFILT_INT_SRC_SLO_NO_MOT__REG        BMA25X_INT_SRC_REG

#define BMA25X_UNFILT_INT_SRC_TAP__POS         4
#define BMA25X_UNFILT_INT_SRC_TAP__LEN         1
#define BMA25X_UNFILT_INT_SRC_TAP__MSK         0x10
#define BMA25X_UNFILT_INT_SRC_TAP__REG         BMA25X_INT_SRC_REG

#define BMA25X_UNFILT_INT_SRC_DATA__POS        5
#define BMA25X_UNFILT_INT_SRC_DATA__LEN        1
#define BMA25X_UNFILT_INT_SRC_DATA__MSK        0x20
#define BMA25X_UNFILT_INT_SRC_DATA__REG        BMA25X_INT_SRC_REG

#define BMA25X_INT1_PAD_ACTIVE_LEVEL__POS       0
#define BMA25X_INT1_PAD_ACTIVE_LEVEL__LEN       1
#define BMA25X_INT1_PAD_ACTIVE_LEVEL__MSK       0x01
#define BMA25X_INT1_PAD_ACTIVE_LEVEL__REG       BMA25X_INT_SET_REG

#define BMA25X_INT2_PAD_ACTIVE_LEVEL__POS       2
#define BMA25X_INT2_PAD_ACTIVE_LEVEL__LEN       1
#define BMA25X_INT2_PAD_ACTIVE_LEVEL__MSK       0x04
#define BMA25X_INT2_PAD_ACTIVE_LEVEL__REG       BMA25X_INT_SET_REG

#define BMA25X_INT1_PAD_OUTPUT_TYPE__POS        1
#define BMA25X_INT1_PAD_OUTPUT_TYPE__LEN        1
#define BMA25X_INT1_PAD_OUTPUT_TYPE__MSK        0x02
#define BMA25X_INT1_PAD_OUTPUT_TYPE__REG        BMA25X_INT_SET_REG

#define BMA25X_INT2_PAD_OUTPUT_TYPE__POS        3
#define BMA25X_INT2_PAD_OUTPUT_TYPE__LEN        1
#define BMA25X_INT2_PAD_OUTPUT_TYPE__MSK        0x08
#define BMA25X_INT2_PAD_OUTPUT_TYPE__REG        BMA25X_INT_SET_REG

#define BMA25X_INT_MODE_SEL__POS                0
#define BMA25X_INT_MODE_SEL__LEN                4
#define BMA25X_INT_MODE_SEL__MSK                0x0F
#define BMA25X_INT_MODE_SEL__REG                BMA25X_INT_CTRL_REG

#define BMA25X_RESET_INT__POS           7
#define BMA25X_RESET_INT__LEN           1
#define BMA25X_RESET_INT__MSK           0x80
#define BMA25X_RESET_INT__REG           BMA25X_INT_CTRL_REG

#define BMA25X_LOWG_DUR__POS                    0
#define BMA25X_LOWG_DUR__LEN                    8
#define BMA25X_LOWG_DUR__MSK                    0xFF
#define BMA25X_LOWG_DUR__REG                    BMA25X_LOW_DURN_REG

#define BMA25X_LOWG_THRES__POS                  0
#define BMA25X_LOWG_THRES__LEN                  8
#define BMA25X_LOWG_THRES__MSK                  0xFF
#define BMA25X_LOWG_THRES__REG                  BMA25X_LOW_THRES_REG

#define BMA25X_LOWG_HYST__POS                   0
#define BMA25X_LOWG_HYST__LEN                   2
#define BMA25X_LOWG_HYST__MSK                   0x03
#define BMA25X_LOWG_HYST__REG                   BMA25X_LOW_HIGH_HYST_REG

#define BMA25X_LOWG_INT_MODE__POS               2
#define BMA25X_LOWG_INT_MODE__LEN               1
#define BMA25X_LOWG_INT_MODE__MSK               0x04
#define BMA25X_LOWG_INT_MODE__REG               BMA25X_LOW_HIGH_HYST_REG

#define BMA25X_HIGHG_DUR__POS                    0
#define BMA25X_HIGHG_DUR__LEN                    8
#define BMA25X_HIGHG_DUR__MSK                    0xFF
#define BMA25X_HIGHG_DUR__REG                    BMA25X_HIGH_DURN_REG

#define BMA25X_HIGHG_THRES__POS                  0
#define BMA25X_HIGHG_THRES__LEN                  8
#define BMA25X_HIGHG_THRES__MSK                  0xFF
#define BMA25X_HIGHG_THRES__REG                  BMA25X_HIGH_THRES_REG

#define BMA25X_HIGHG_HYST__POS                  6
#define BMA25X_HIGHG_HYST__LEN                  2
#define BMA25X_HIGHG_HYST__MSK                  0xC0
#define BMA25X_HIGHG_HYST__REG                  BMA25X_LOW_HIGH_HYST_REG

#define BMA25X_SLOPE_DUR__POS                    0
#define BMA25X_SLOPE_DUR__LEN                    2
#define BMA25X_SLOPE_DUR__MSK                    0x03
#define BMA25X_SLOPE_DUR__REG                    BMA25X_SLOPE_DURN_REG

#define BMA25X_SLO_NO_MOT_DUR__POS                    2
#define BMA25X_SLO_NO_MOT_DUR__LEN                    6
#define BMA25X_SLO_NO_MOT_DUR__MSK                    0xFC
#define BMA25X_SLO_NO_MOT_DUR__REG                    BMA25X_SLOPE_DURN_REG

#define BMA25X_SLOPE_THRES__POS                  0
#define BMA25X_SLOPE_THRES__LEN                  8
#define BMA25X_SLOPE_THRES__MSK                  0xFF
#define BMA25X_SLOPE_THRES__REG                  BMA25X_SLOPE_THRES_REG

#define BMA25X_SLO_NO_MOT_THRES__POS                  0
#define BMA25X_SLO_NO_MOT_THRES__LEN                  8
#define BMA25X_SLO_NO_MOT_THRES__MSK                  0xFF
#define BMA25X_SLO_NO_MOT_THRES__REG           BMA25X_SLO_NO_MOT_THRES_REG

#define BMA25X_TAP_DUR__POS                    0
#define BMA25X_TAP_DUR__LEN                    3
#define BMA25X_TAP_DUR__MSK                    0x07
#define BMA25X_TAP_DUR__REG                    BMA25X_TAP_PARAM_REG

#define BMA25X_TAP_SHOCK_DURN__POS             6
#define BMA25X_TAP_SHOCK_DURN__LEN             1
#define BMA25X_TAP_SHOCK_DURN__MSK             0x40
#define BMA25X_TAP_SHOCK_DURN__REG             BMA25X_TAP_PARAM_REG

#define BMA25X_ADV_TAP_INT__POS                5
#define BMA25X_ADV_TAP_INT__LEN                1
#define BMA25X_ADV_TAP_INT__MSK                0x20
#define BMA25X_ADV_TAP_INT__REG                BMA25X_TAP_PARAM_REG

#define BMA25X_TAP_QUIET_DURN__POS             7
#define BMA25X_TAP_QUIET_DURN__LEN             1
#define BMA25X_TAP_QUIET_DURN__MSK             0x80
#define BMA25X_TAP_QUIET_DURN__REG             BMA25X_TAP_PARAM_REG

#define BMA25X_TAP_THRES__POS                  0
#define BMA25X_TAP_THRES__LEN                  5
#define BMA25X_TAP_THRES__MSK                  0x1F
#define BMA25X_TAP_THRES__REG                  BMA25X_TAP_THRES_REG

#define BMA25X_TAP_SAMPLES__POS                6
#define BMA25X_TAP_SAMPLES__LEN                2
#define BMA25X_TAP_SAMPLES__MSK                0xC0
#define BMA25X_TAP_SAMPLES__REG                BMA25X_TAP_THRES_REG

#define BMA25X_ORIENT_MODE__POS                  0
#define BMA25X_ORIENT_MODE__LEN                  2
#define BMA25X_ORIENT_MODE__MSK                  0x03
#define BMA25X_ORIENT_MODE__REG                  BMA25X_ORIENT_PARAM_REG

#define BMA25X_ORIENT_BLOCK__POS                 2
#define BMA25X_ORIENT_BLOCK__LEN                 2
#define BMA25X_ORIENT_BLOCK__MSK                 0x0C
#define BMA25X_ORIENT_BLOCK__REG                 BMA25X_ORIENT_PARAM_REG

#define BMA25X_ORIENT_HYST__POS                  4
#define BMA25X_ORIENT_HYST__LEN                  3
#define BMA25X_ORIENT_HYST__MSK                  0x70
#define BMA25X_ORIENT_HYST__REG                  BMA25X_ORIENT_PARAM_REG

#define BMA25X_ORIENT_AXIS__POS                  7
#define BMA25X_ORIENT_AXIS__LEN                  1
#define BMA25X_ORIENT_AXIS__MSK                  0x80
#define BMA25X_ORIENT_AXIS__REG                  BMA25X_THETA_BLOCK_REG

#define BMA25X_ORIENT_UD_EN__POS                  6
#define BMA25X_ORIENT_UD_EN__LEN                  1
#define BMA25X_ORIENT_UD_EN__MSK                  0x40
#define BMA25X_ORIENT_UD_EN__REG                  BMA25X_THETA_BLOCK_REG

#define BMA25X_THETA_BLOCK__POS                  0
#define BMA25X_THETA_BLOCK__LEN                  6
#define BMA25X_THETA_BLOCK__MSK                  0x3F
#define BMA25X_THETA_BLOCK__REG                  BMA25X_THETA_BLOCK_REG

#define BMA25X_THETA_FLAT__POS                  0
#define BMA25X_THETA_FLAT__LEN                  6
#define BMA25X_THETA_FLAT__MSK                  0x3F
#define BMA25X_THETA_FLAT__REG                  BMA25X_THETA_FLAT_REG

#define BMA25X_FLAT_HOLD_TIME__POS              4
#define BMA25X_FLAT_HOLD_TIME__LEN              2
#define BMA25X_FLAT_HOLD_TIME__MSK              0x30
#define BMA25X_FLAT_HOLD_TIME__REG              BMA25X_FLAT_HOLD_TIME_REG

#define BMA25X_FLAT_HYS__POS                   0
#define BMA25X_FLAT_HYS__LEN                   3
#define BMA25X_FLAT_HYS__MSK                   0x07
#define BMA25X_FLAT_HYS__REG                   BMA25X_FLAT_HOLD_TIME_REG

#define BMA25X_FIFO_WML_TRIG_RETAIN__POS                   0
#define BMA25X_FIFO_WML_TRIG_RETAIN__LEN                   6
#define BMA25X_FIFO_WML_TRIG_RETAIN__MSK                   0x3F
#define BMA25X_FIFO_WML_TRIG_RETAIN__REG                   BMA25X_FIFO_WML_TRIG

#define BMA25X_EN_SELF_TEST__POS                0
#define BMA25X_EN_SELF_TEST__LEN                2
#define BMA25X_EN_SELF_TEST__MSK                0x03
#define BMA25X_EN_SELF_TEST__REG                BMA25X_SELF_TEST_REG

#define BMA25X_NEG_SELF_TEST__POS               2
#define BMA25X_NEG_SELF_TEST__LEN               1
#define BMA25X_NEG_SELF_TEST__MSK               0x04
#define BMA25X_NEG_SELF_TEST__REG               BMA25X_SELF_TEST_REG

#define BMA25X_SELF_TEST_AMP__POS               4
#define BMA25X_SELF_TEST_AMP__LEN               1
#define BMA25X_SELF_TEST_AMP__MSK               0x10
#define BMA25X_SELF_TEST_AMP__REG               BMA25X_SELF_TEST_REG


#define BMA25X_UNLOCK_EE_PROG_MODE__POS     0
#define BMA25X_UNLOCK_EE_PROG_MODE__LEN     1
#define BMA25X_UNLOCK_EE_PROG_MODE__MSK     0x01
#define BMA25X_UNLOCK_EE_PROG_MODE__REG     BMA25X_EEPROM_CTRL_REG

#define BMA25X_START_EE_PROG_TRIG__POS      1
#define BMA25X_START_EE_PROG_TRIG__LEN      1
#define BMA25X_START_EE_PROG_TRIG__MSK      0x02
#define BMA25X_START_EE_PROG_TRIG__REG      BMA25X_EEPROM_CTRL_REG

#define BMA25X_EE_PROG_READY__POS          2
#define BMA25X_EE_PROG_READY__LEN          1
#define BMA25X_EE_PROG_READY__MSK          0x04
#define BMA25X_EE_PROG_READY__REG          BMA25X_EEPROM_CTRL_REG

#define BMA25X_UPDATE_IMAGE__POS                3
#define BMA25X_UPDATE_IMAGE__LEN                1
#define BMA25X_UPDATE_IMAGE__MSK                0x08
#define BMA25X_UPDATE_IMAGE__REG                BMA25X_EEPROM_CTRL_REG

#define BMA25X_EE_REMAIN__POS                4
#define BMA25X_EE_REMAIN__LEN                4
#define BMA25X_EE_REMAIN__MSK                0xF0
#define BMA25X_EE_REMAIN__REG                BMA25X_EEPROM_CTRL_REG

#define BMA25X_EN_SPI_MODE_3__POS              0
#define BMA25X_EN_SPI_MODE_3__LEN              1
#define BMA25X_EN_SPI_MODE_3__MSK              0x01
#define BMA25X_EN_SPI_MODE_3__REG              BMA25X_SERIAL_CTRL_REG

#define BMA25X_I2C_WATCHDOG_PERIOD__POS        1
#define BMA25X_I2C_WATCHDOG_PERIOD__LEN        1
#define BMA25X_I2C_WATCHDOG_PERIOD__MSK        0x02
#define BMA25X_I2C_WATCHDOG_PERIOD__REG        BMA25X_SERIAL_CTRL_REG

#define BMA25X_EN_I2C_WATCHDOG__POS            2
#define BMA25X_EN_I2C_WATCHDOG__LEN            1
#define BMA25X_EN_I2C_WATCHDOG__MSK            0x04
#define BMA25X_EN_I2C_WATCHDOG__REG            BMA25X_SERIAL_CTRL_REG

#define BMA25X_EXT_MODE__POS              7
#define BMA25X_EXT_MODE__LEN              1
#define BMA25X_EXT_MODE__MSK              0x80
#define BMA25X_EXT_MODE__REG              BMA25X_EXTMODE_CTRL_REG

#define BMA25X_ALLOW_UPPER__POS        6
#define BMA25X_ALLOW_UPPER__LEN        1
#define BMA25X_ALLOW_UPPER__MSK        0x40
#define BMA25X_ALLOW_UPPER__REG        BMA25X_EXTMODE_CTRL_REG

#define BMA25X_MAP_2_LOWER__POS            5
#define BMA25X_MAP_2_LOWER__LEN            1
#define BMA25X_MAP_2_LOWER__MSK            0x20
#define BMA25X_MAP_2_LOWER__REG            BMA25X_EXTMODE_CTRL_REG

#define BMA25X_MAGIC_NUMBER__POS            0
#define BMA25X_MAGIC_NUMBER__LEN            5
#define BMA25X_MAGIC_NUMBER__MSK            0x1F
#define BMA25X_MAGIC_NUMBER__REG            BMA25X_EXTMODE_CTRL_REG

#define BMA25X_UNLOCK_EE_WRITE_TRIM__POS        4
#define BMA25X_UNLOCK_EE_WRITE_TRIM__LEN        4
#define BMA25X_UNLOCK_EE_WRITE_TRIM__MSK        0xF0
#define BMA25X_UNLOCK_EE_WRITE_TRIM__REG        BMA25X_CTRL_UNLOCK_REG

#define BMA25X_EN_SLOW_COMP_X__POS              0
#define BMA25X_EN_SLOW_COMP_X__LEN              1
#define BMA25X_EN_SLOW_COMP_X__MSK              0x01
#define BMA25X_EN_SLOW_COMP_X__REG              BMA25X_OFFSET_CTRL_REG

#define BMA25X_EN_SLOW_COMP_Y__POS              1
#define BMA25X_EN_SLOW_COMP_Y__LEN              1
#define BMA25X_EN_SLOW_COMP_Y__MSK              0x02
#define BMA25X_EN_SLOW_COMP_Y__REG              BMA25X_OFFSET_CTRL_REG

#define BMA25X_EN_SLOW_COMP_Z__POS              2
#define BMA25X_EN_SLOW_COMP_Z__LEN              1
#define BMA25X_EN_SLOW_COMP_Z__MSK              0x04
#define BMA25X_EN_SLOW_COMP_Z__REG              BMA25X_OFFSET_CTRL_REG

#define BMA25X_FAST_CAL_RDY_S__POS             4
#define BMA25X_FAST_CAL_RDY_S__LEN             1
#define BMA25X_FAST_CAL_RDY_S__MSK             0x10
#define BMA25X_FAST_CAL_RDY_S__REG             BMA25X_OFFSET_CTRL_REG

#define BMA25X_CAL_TRIGGER__POS                5
#define BMA25X_CAL_TRIGGER__LEN                2
#define BMA25X_CAL_TRIGGER__MSK                0x60
#define BMA25X_CAL_TRIGGER__REG                BMA25X_OFFSET_CTRL_REG

#define BMA25X_RESET_OFFSET_REGS__POS           7
#define BMA25X_RESET_OFFSET_REGS__LEN           1
#define BMA25X_RESET_OFFSET_REGS__MSK           0x80
#define BMA25X_RESET_OFFSET_REGS__REG           BMA25X_OFFSET_CTRL_REG

#define BMA25X_COMP_CUTOFF__POS                 0
#define BMA25X_COMP_CUTOFF__LEN                 1
#define BMA25X_COMP_CUTOFF__MSK                 0x01
#define BMA25X_COMP_CUTOFF__REG                 BMA25X_OFFSET_PARAMS_REG

#define BMA25X_COMP_TARGET_OFFSET_X__POS        1
#define BMA25X_COMP_TARGET_OFFSET_X__LEN        2
#define BMA25X_COMP_TARGET_OFFSET_X__MSK        0x06
#define BMA25X_COMP_TARGET_OFFSET_X__REG        BMA25X_OFFSET_PARAMS_REG

#define BMA25X_COMP_TARGET_OFFSET_Y__POS        3
#define BMA25X_COMP_TARGET_OFFSET_Y__LEN        2
#define BMA25X_COMP_TARGET_OFFSET_Y__MSK        0x18
#define BMA25X_COMP_TARGET_OFFSET_Y__REG        BMA25X_OFFSET_PARAMS_REG

#define BMA25X_COMP_TARGET_OFFSET_Z__POS        5
#define BMA25X_COMP_TARGET_OFFSET_Z__LEN        2
#define BMA25X_COMP_TARGET_OFFSET_Z__MSK        0x60
#define BMA25X_COMP_TARGET_OFFSET_Z__REG        BMA25X_OFFSET_PARAMS_REG

#define BMA25X_FIFO_DATA_SELECT__POS                 0
#define BMA25X_FIFO_DATA_SELECT__LEN                 2
#define BMA25X_FIFO_DATA_SELECT__MSK                 0x03
#define BMA25X_FIFO_DATA_SELECT__REG                 BMA25X_FIFO_MODE_REG

#define BMA25X_FIFO_TRIGGER_SOURCE__POS                 2
#define BMA25X_FIFO_TRIGGER_SOURCE__LEN                 2
#define BMA25X_FIFO_TRIGGER_SOURCE__MSK                 0x0C
#define BMA25X_FIFO_TRIGGER_SOURCE__REG                 BMA25X_FIFO_MODE_REG

#define BMA25X_FIFO_TRIGGER_ACTION__POS                 4
#define BMA25X_FIFO_TRIGGER_ACTION__LEN                 2
#define BMA25X_FIFO_TRIGGER_ACTION__MSK                 0x30
#define BMA25X_FIFO_TRIGGER_ACTION__REG                 BMA25X_FIFO_MODE_REG

#define BMA25X_FIFO_MODE__POS                 6
#define BMA25X_FIFO_MODE__LEN                 2
#define BMA25X_FIFO_MODE__MSK                 0xC0
#define BMA25X_FIFO_MODE__REG                 BMA25X_FIFO_MODE_REG


#define BMA25X_STATUS1                             0
#define BMA25X_STATUS2                             1
#define BMA25X_STATUS3                             2
#define BMA25X_STATUS4                             3
#define BMA25X_STATUS5                             4


#define BMA25X_RANGE_2G                 3
#define BMA25X_RANGE_4G                 5
#define BMA25X_RANGE_8G                 8
#define BMA25X_RANGE_16G                12


#define BMA25X_BW_7_81HZ        0x08
#define BMA25X_BW_15_63HZ       0x09
#define BMA25X_BW_31_25HZ       0x0A
#define BMA25X_BW_62_50HZ       0x0B
#define BMA25X_BW_125HZ         0x0C
#define BMA25X_BW_250HZ         0x0D
#define BMA25X_BW_500HZ         0x0E
#define BMA25X_BW_1000HZ        0x0F

#define BMA25X_SLEEP_DUR_0_5MS        0x05
#define BMA25X_SLEEP_DUR_1MS          0x06
#define BMA25X_SLEEP_DUR_2MS          0x07
#define BMA25X_SLEEP_DUR_4MS          0x08
#define BMA25X_SLEEP_DUR_6MS          0x09
#define BMA25X_SLEEP_DUR_10MS         0x0A
#define BMA25X_SLEEP_DUR_25MS         0x0B
#define BMA25X_SLEEP_DUR_50MS         0x0C
#define BMA25X_SLEEP_DUR_100MS        0x0D
#define BMA25X_SLEEP_DUR_500MS        0x0E
#define BMA25X_SLEEP_DUR_1S           0x0F

#define BMA25X_LATCH_DUR_NON_LATCH    0x00
#define BMA25X_LATCH_DUR_250MS        0x01
#define BMA25X_LATCH_DUR_500MS        0x02
#define BMA25X_LATCH_DUR_1S           0x03
#define BMA25X_LATCH_DUR_2S           0x04
#define BMA25X_LATCH_DUR_4S           0x05
#define BMA25X_LATCH_DUR_8S           0x06
#define BMA25X_LATCH_DUR_LATCH        0x07
#define BMA25X_LATCH_DUR_NON_LATCH1   0x08
#define BMA25X_LATCH_DUR_250US        0x09
#define BMA25X_LATCH_DUR_500US        0x0A
#define BMA25X_LATCH_DUR_1MS          0x0B
#define BMA25X_LATCH_DUR_12_5MS       0x0C
#define BMA25X_LATCH_DUR_25MS         0x0D
#define BMA25X_LATCH_DUR_50MS         0x0E
#define BMA25X_LATCH_DUR_LATCH1       0x0F

#define BMA25X_MODE_NORMAL             0
#define BMA25X_MODE_LOWPOWER1          1
#define BMA25X_MODE_SUSPEND            2
#define BMA25X_MODE_DEEP_SUSPEND       3
#define BMA25X_MODE_LOWPOWER2          4
#define BMA25X_MODE_STANDBY            5

#define BMA25X_X_AXIS           0
#define BMA25X_Y_AXIS           1
#define BMA25X_Z_AXIS           2

#define BMA25X_Low_G_Interrupt       0
#define BMA25X_High_G_X_Interrupt    1
#define BMA25X_High_G_Y_Interrupt    2
#define BMA25X_High_G_Z_Interrupt    3
#define BMA25X_DATA_EN               4
#define BMA25X_Slope_X_Interrupt     5
#define BMA25X_Slope_Y_Interrupt     6
#define BMA25X_Slope_Z_Interrupt     7
#define BMA25X_Single_Tap_Interrupt  8
#define BMA25X_Double_Tap_Interrupt  9
#define BMA25X_Orient_Interrupt      10
#define BMA25X_Flat_Interrupt        11
#define BMA25X_FFULL_INTERRUPT       12
#define BMA25X_FWM_INTERRUPT         13

#define BMA25X_INT1_LOWG         0
#define BMA25X_INT2_LOWG         1
#define BMA25X_INT1_HIGHG        0
#define BMA25X_INT2_HIGHG        1
#define BMA25X_INT1_SLOPE        0
#define BMA25X_INT2_SLOPE        1
#define BMA25X_INT1_SLO_NO_MOT   0
#define BMA25X_INT2_SLO_NO_MOT   1
#define BMA25X_INT1_DTAP         0
#define BMA25X_INT2_DTAP         1
#define BMA25X_INT1_STAP         0
#define BMA25X_INT2_STAP         1
#define BMA25X_INT1_ORIENT       0
#define BMA25X_INT2_ORIENT       1
#define BMA25X_INT1_FLAT         0
#define BMA25X_INT2_FLAT         1
#define BMA25X_INT1_NDATA        0
#define BMA25X_INT2_NDATA        1
#define BMA25X_INT1_FWM          0
#define BMA25X_INT2_FWM          1
#define BMA25X_INT1_FFULL        0
#define BMA25X_INT2_FFULL        1

#define BMA25X_SRC_LOWG         0
#define BMA25X_SRC_HIGHG        1
#define BMA25X_SRC_SLOPE        2
#define BMA25X_SRC_SLO_NO_MOT   3
#define BMA25X_SRC_TAP          4
#define BMA25X_SRC_DATA         5

#define BMA25X_INT1_OUTPUT      0
#define BMA25X_INT2_OUTPUT      1
#define BMA25X_INT1_LEVEL       0
#define BMA25X_INT2_LEVEL       1

#define BMA25X_LOW_DURATION            0
#define BMA25X_HIGH_DURATION           1
#define BMA25X_SLOPE_DURATION          2
#define BMA25X_SLO_NO_MOT_DURATION     3

#define BMA25X_LOW_THRESHOLD            0
#define BMA25X_HIGH_THRESHOLD           1
#define BMA25X_SLOPE_THRESHOLD          2
#define BMA25X_SLO_NO_MOT_THRESHOLD     3


#define BMA25X_LOWG_HYST                0
#define BMA25X_HIGHG_HYST               1

#define BMA25X_ORIENT_THETA             0
#define BMA25X_FLAT_THETA               1

#define BMA25X_I2C_SELECT               0
#define BMA25X_I2C_EN                   1

#define BMA25X_SLOW_COMP_X              0
#define BMA25X_SLOW_COMP_Y              1
#define BMA25X_SLOW_COMP_Z              2

#define BMA25X_CUT_OFF                  0
#define BMA25X_OFFSET_TRIGGER_X         1
#define BMA25X_OFFSET_TRIGGER_Y         2
#define BMA25X_OFFSET_TRIGGER_Z         3

#define BMA25X_GP0                      0
#define BMA25X_GP1                      1

#define BMA25X_SLO_NO_MOT_EN_X          0
#define BMA25X_SLO_NO_MOT_EN_Y          1
#define BMA25X_SLO_NO_MOT_EN_Z          2
#define BMA25X_SLO_NO_MOT_EN_SEL        3

#define BMA25X_WAKE_UP_DUR_20MS         0
#define BMA25X_WAKE_UP_DUR_80MS         1
#define BMA25X_WAKE_UP_DUR_320MS                2
#define BMA25X_WAKE_UP_DUR_2560MS               3

#define BMA25X_SELF_TEST0_ON            1
#define BMA25X_SELF_TEST1_ON            2

#define BMA25X_EE_W_OFF                 0
#define BMA25X_EE_W_ON                  1

#define BMA25X_LOW_TH_IN_G(gthres, range)           ((256 * gthres) / range)


#define BMA25X_HIGH_TH_IN_G(gthres, range)          ((256 * gthres) / range)


#define BMA25X_LOW_HY_IN_G(ghyst, range)            ((32 * ghyst) / range)


#define BMA25X_HIGH_HY_IN_G(ghyst, range)           ((32 * ghyst) / range)


#define BMA25X_SLOPE_TH_IN_G(gthres, range)    ((128 * gthres) / range)


#define BMA25X_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)


#define BMA25X_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

#define BMA25X_FIFO_DAT_SEL_X                     1
#define BMA25X_FIFO_DAT_SEL_Y                     2
#define BMA25X_FIFO_DAT_SEL_Z                     3

/*! high g or slope interrupt type definition*/
/*! High G interrupt of x, y, z axis happened */
#define HIGH_G_INTERRUPT_X            HIGH_G_INTERRUPT_X_HAPPENED
#define HIGH_G_INTERRUPT_Y            HIGH_G_INTERRUPT_Y_HAPPENED
#define HIGH_G_INTERRUPT_Z            HIGH_G_INTERRUPT_Z_HAPPENED
/*! High G interrupt of x, y, z negative axis happened */
#define HIGH_G_INTERRUPT_X_N          HIGH_G_INTERRUPT_X_NEGATIVE_HAPPENED
#define HIGH_G_INTERRUPT_Y_N          HIGH_G_INTERRUPT_Y_NEGATIVE_HAPPENED
#define HIGH_G_INTERRUPT_Z_N          HIGH_G_INTERRUPT_Z_NEGATIVE_HAPPENED
/*! Slope interrupt of x, y, z axis happened */
#define SLOPE_INTERRUPT_X             SLOPE_INTERRUPT_X_HAPPENED
#define SLOPE_INTERRUPT_Y             SLOPE_INTERRUPT_Y_HAPPENED
#define SLOPE_INTERRUPT_Z             SLOPE_INTERRUPT_Z_HAPPENED
/*! Slope interrupt of x, y, z negative axis happened */
#define SLOPE_INTERRUPT_X_N           SLOPE_INTERRUPT_X_NEGATIVE_HAPPENED
#define SLOPE_INTERRUPT_Y_N           SLOPE_INTERRUPT_Y_NEGATIVE_HAPPENED
#define SLOPE_INTERRUPT_Z_N           SLOPE_INTERRUPT_Z_NEGATIVE_HAPPENED

/*BMA power supply VDD 1.62V-3.6V VIO 1.2-3.6V */
#define BMA25X_VDD_MIN_UV       2000000
#define BMA25X_VDD_MAX_UV       3400000
#define BMA25X_VIO_MIN_UV       1500000
#define BMA25X_VIO_MAX_UV       3400000

#define TEST_BIT(pos, number) (number & (1 << pos))

#define BMA25X_MAP_FLAT_TO_INT2

enum {
	FlatUp = 0,
	FlatDown,
	Motion,
	numSensors   /* This needs to be at the end of the list */
};

struct bma25xacc {
	int x;
	int y;
	int z;
};

static int bma25x_set_Int_Enable(struct i2c_client *client, unsigned char
		InterruptType , unsigned char value);

static int bma2x2_get_data(int *x , int *y, int *z, int *status);
#endif//CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
//tuwenzan@wind-mobi.com add at 20161128 begin
int cali_flag = 0;
struct i2c_client *bma2x2_client;
//tuwenzan@wind-mobi.com add at 20161128 end
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id bma2x2_i2c_id[] = {{BMA2x2_DEV_NAME, 0}, {} };
//twz modify
#ifndef CONFIG_OF
static struct i2c_board_info __initdata bma2x2_i2c_info = {
	I2C_BOARD_INFO(BMA2x2_DEV_NAME, BMA2x2_I2C_ADDR)
};
#endif
/*----------------------------------------------------------------------------*/
static int bma2x2_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
static int bma2x2_i2c_remove(struct i2c_client *client);
/*----------------------------------------------------------------------------*/
enum {
	BMA_TRC_FILTER  = 0x01,
	BMA_TRC_RAWDATA = 0x02,
	BMA_TRC_IOCTL   = 0x04,
	BMA_TRC_CALI	= 0X08,
	BMA_TRC_INFO	= 0X10,
} BMA_TRC;

enum {
	BMA25X_AOD,
	BMA25X_ACC,
};
/*----------------------------------------------------------------------------*/
struct scale_factor {
	u8  whole;
	u16  fraction; //twz  modify u8->u16
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
	struct scale_factor scalefactor;
	int                 sensitivity;
};
/*----------------------------------------------------------------------------*/
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMA2x2_AXES_NUM];
	int sum[BMA2x2_AXES_NUM];
	int num;
	int idx;
};
/*----------------------------------------------------------------------------*/
struct bma2x2_i2c_data {
	struct i2c_client *client;
	struct acc_hw *hw;
	struct hwmsen_convert   cvt;

/*misc*/
	struct data_resolution *reso;
	atomic_t                trace;
	atomic_t                suspend;
	atomic_t                selftest;
	atomic_t				filter;
	s16                     cali_sw[BMA2x2_AXES_NUM+1];
	struct mutex lock;

/*data*/
/*+1: for 4-byte alignment*/
	s8                      offset[BMA2x2_AXES_NUM+1];
	s16                     data[BMA2x2_AXES_NUM+1];
	u8                      fifo_count;

#if defined(CONFIG_BMA2x2_LOWPASS)
	atomic_t                firlen;
	atomic_t                fir_en;
	struct data_filter      fir;
#endif

/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend    early_drv;
#endif

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	int IRQ1;
	int IRQ2;
	struct mutex int_mode_mutex;
	atomic_t flat_flag;
	int mEnabled;
	int flat_threshold;
	int aod_flag;
	int flat_up_value;
	int flat_down_value;
	struct delayed_work flat_work;
	struct wake_lock aod_wakelock;
	struct input_dev *dev_interrupt;
	struct work_struct int1_irq_work;
	struct work_struct int2_irq_work;
	struct workqueue_struct *data_wq;
#endif
	int mode;
	struct mutex mode_mutex;
};

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
typedef struct bma2x2_i2c_data bma25x_data;
#endif
#ifdef BMA25X_MAP_FLAT_TO_INT2
static int bma25x_set_int2_pad_sel(struct i2c_client *client,
		unsigned char int2sel);
#endif
#if !defined(CONFIG_HAS_EARLYSUSPEND)
static int bma2x2_suspend(struct i2c_client *client, pm_message_t msg);
static int bma2x2_resume(struct i2c_client *client);
#endif
/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id gsensor_of_match[] = {
	{ .compatible = "mediatek,gsensor", },
	{},
};
#endif

static struct i2c_driver bma2x2_i2c_driver = {
	.driver = {
		.name           = BMA2x2_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = gsensor_of_match,
#endif
	},
	.probe				= bma2x2_i2c_probe,
	.remove				= bma2x2_i2c_remove,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	.suspend            = bma2x2_suspend,
	.resume             = bma2x2_resume,
#endif
	.id_table = bma2x2_i2c_id,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *bma2x2_i2c_client;
static struct acc_init_info bma2x2_init_info;
static struct bma2x2_i2c_data *obj_i2c_data;
static bool sensor_power = true;
static struct GSENSOR_VECTOR3D gsensor_gain;
//static char selftestRes[8] = {0};
static int bma2x2_init_flag = -1; /* 0<==>OK -1 <==> fail */
#define DMA_BUFFER_SIZE (32*6)
static unsigned char *I2CDMABuf_va;
static dma_addr_t I2CDMABuf_pa;

/*----------------------------------------------------------------------------*/
#define GSE_TAG                 "[Gsensor] "
#define GSE_FUN(f)              printk(GSE_TAG"%s\n", __func__)
#define GSE_ERR(fmt, args...)   \
printk(GSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)   printk(GSE_TAG fmt, ##args)


#define COMPATIABLE_NAME "mediatek,bma255"

struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_2g_data_resolution[4] = {
/*combination by {FULL_RES,RANGE}*/
/*BMA222E +/-2g  in 8-bit;  { 15, 63} = 15.63;  64 = (2^8)/(2*2)*/
	 { { 15, 63 }, 64 },
/*BMA250E +/-2g  in 10-bit;  { 3, 91} = 3.91;  256 = (2^10)/(2*2)*/
	 { { 3, 91 }, 256 },
/*BMA255 +/-2g  in 12-bit;  { 0, 98} = 0.98;  1024 = (2^12)/(2*2)*/
	 {{ 0, 98}, 1024},
/*BMA280 +/-2g  in 14-bit;  { 0, 244} = 0.244;  4096 = (2^14)/(2*2)*/
	 { { 0, 488 }, 4096 },
	 };
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_4g_data_resolution[4] = {
	/*combination by {FULL_RES,RANGE}*/
	/*BMA222E +/-4g  in 8-bit;  { 31, 25} = 31.25;  32 = (2^8)/(2*4)*/
	{ { 31, 25 }, 32 },
	/*BMA250E +/-4g  in 10-bit;  { 7, 81} = 7.81;  128 = (2^10)/(2*4)*/
	{ { 7, 81 }, 128 },
	/*BMA255 +/-4g  in 12-bit;  { 1, 95} = 1.95;  512 = (2^12)/(2*4)*/
	{ { 1, 95 }, 512 },
	/*BMA280 +/-4g  in 14-bit;  { 0, 488} = 0.488;  2048 = (2^14)/(2*4)*/
	{ { 0, 488 }, 2048 },
};
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_8g_data_resolution[4] = {
	/*combination by {FULL_RES,RANGE}*/
	/*BMA222E +/-8g  in 8-bit;  { 62, 50} = 62.5;  16 = (2^8)/(2*8)*/
	{ { 62, 50 }, 16 },
	/*BMA250E +/-8g  in 10-bit;  { 15, 63} = 15.63;  64 = (2^10)/(2*8)*/
	{ { 15, 63 }, 64 },
	/*BMA255 +/-8g  in 12-bit;  { 3, 91} = 3.91;  256 = (2^12)/(2*8)*/
	{ { 3, 91 }, 256 },
	/*BMA280 +/-8g  in 14-bit;  { 0, 977} = 0.977;  1024 = (2^14)/(2*8)*/
	{ { 0, 977 }, 1024 },
};
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_16g_data_resolution[4] = {
	/*combination by {FULL_RES,RANGE}*/
	/*BMA222E +/-16g  in 8-bit;  { 125, 0} = 125.0;  8 = (2^8)/(2*16)*/
	{ { 125, 0 }, 8 },
	/*BMA250E +/-16g  in 10-bit;  { 31, 25} = 31.25;  32 = (2^10)/(2*16)*/
	{ { 31, 25 }, 32 },
	/*BMA255 +/-16g  in 12-bit;  { 7, 81} = 7.81;  128 = (2^12)/(2*16)*/
	{ { 7, 81 }, 128 },
	/*BMA280 +/-16g  in 14-bit;  { 1, 953} = 1.953;  512 = (2^14)/(2*16)*/
	{ { 1, 953 }, 512 },
};
/*----------------------------------------------------------------------------*/

/*add DEVINFO SUPPORT BY dingleilei*/
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
#define SLT_DEVINFO_ACCEL_DEBUG
#include  "dev_info.h"
struct devinfo_struct *s_DEVINFO_acceleration;    
static void devinfo_acceleration_regchar(char *module,char * vendor,char *version,char *used)
{

	s_DEVINFO_acceleration =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_acceleration->device_type="Acceleration";
	s_DEVINFO_acceleration->device_module=module;
	s_DEVINFO_acceleration->device_vendor=vendor;
	s_DEVINFO_acceleration->device_ic="BMA253";
	s_DEVINFO_acceleration->device_info=DEVINFO_NULL;
	s_DEVINFO_acceleration->device_version=version;
	s_DEVINFO_acceleration->device_used=used;
#ifdef SLT_DEVINFO_ACCEL_DEBUG
		printk("[DEVINFO Acceleration]registe acceleration device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s>\n",
				s_DEVINFO_acceleration->device_type,s_DEVINFO_acceleration->device_module,s_DEVINFO_acceleration->device_vendor,
				s_DEVINFO_acceleration->device_ic,s_DEVINFO_acceleration->device_version,s_DEVINFO_acceleration->device_info,s_DEVINFO_acceleration->device_used);
#endif
       DEVINFO_CHECK_DECLARE(s_DEVINFO_acceleration->device_type,s_DEVINFO_acceleration->device_module,s_DEVINFO_acceleration->device_vendor,s_DEVINFO_acceleration->device_ic,s_DEVINFO_acceleration->device_version,s_DEVINFO_acceleration->device_info,s_DEVINFO_acceleration->device_used);
}
#endif
/* end add*/
static struct data_resolution bma2x2_offset_resolution;

/* I2C operation functions */
#ifdef DMA_FEATURE
static int bma_i2c_dma_read(struct i2c_client *client,
	 unsigned char regaddr, unsigned char *readbuf, int readlen)
{
#ifdef DMA_READ
	int ret = 0;
	if (readlen > DMA_BUFFER_SIZE) {
		GSE_ERR("Read length cann't exceed dma buffer size!\n");
		return -EINVAL;
	}
	mutex_lock(&i2c_lock);
	/*write the register address*/
	ret = i2c_master_send(client, &regaddr, 1);
	if (ret < 0) {
		GSE_ERR("send command error!!\n");
		return -EFAULT;
	}

	/*dma read*/
	client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
	ret = i2c_master_recv(client, I2CDMABuf_pa, readlen);
	/*clear DMA flag once transfer done*/
	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
	if (ret < 0) {
		GSE_ERR("dma receive data error!!\n");
		return -EFAULT;
	}
	memcpy(readbuf, I2CDMABuf_va, readlen);
	mutex_unlock(&i2c_lock);
	return ret;
#else

	int ret;
	s32 retry = 0;
	u8 buffer[2];

	struct i2c_msg msg[2] = {
	 {
	 .addr = (client->addr & I2C_MASK_FLAG),
	 .flags = 0,
	 .buf = buffer,
	 .len = 1,
	 },
	 {
	 .addr = (client->addr & I2C_MASK_FLAG),
	 .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
	 .flags = I2C_M_RD,
	 .buf = I2CDMABuf_pa,
	 .len = readlen,
	 },
	};

	buffer[0] = regaddr;

	if (readbuf == NULL)
		return BMA2x2_ERR_I2C;

	for (retry = 0; retry < 10; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		 if (ret < 0) {
			GSE_ERR("I2C DMA read error retry=%d", retry);
		 continue;
	 }
	 memcpy(readbuf, I2CDMABuf_va, readlen);
	 return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d",
	 regaddr, readlen, ret);
	return ret;
#endif

}
#endif

int bma_i2c_read_block(struct i2c_client *client,u8 addr, u8 *data, u8 len)  //tuwenzan@wind-mobi.com modify at 20161128
{
	u8 beg = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &beg
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2) {
		GSE_ERR("i2c_transfer error: (%d %p %d) %d\n",
			addr, data, len, err);
		err = -EIO;
	} else {
		err = 0;/*no error*/
	}

	return err;
}
#define I2C_BUFFER_SIZE 256
int bma_i2c_write_block(struct i2c_client *client, u8 addr,u8 *data, u8 len)  //tuwenzan@wind-mobi.com modify at 20161128
{
	/*
	*because address also occupies one byte,
	*the maximum length for write is 7 bytes
	*/
	int err, idx = 0, num = 0;
	char buf[32];

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		GSE_ERR("send command error!!\n");
		return -EFAULT;
	} else {
		err = 0;/*no error*/
	}

	return err;
}


/*-----BMA255 power control function------------------------*/
static void BMA2x2_power(struct acc_hw *hw, unsigned int on)
{
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int BMA2x2_SetDataResolution(struct bma2x2_i2c_data *obj, u8 dataformat)
{

/*set g sensor dataresolution here*/

/*BMA255 only can set to 10-bit dataresolution*/
/*so do nothing in bma255 driver here*/

/*end of set dataresolution*/

/*we set measure range from -4g to +4g in
*BMA2x2_SetDataFormat(client, BMA2x2_RANGE_2G),
*and set 10-bit dataresolution BMA2x2_SetDataResolution()*/

	if (CHIP_TYPE >= 0) {
		switch (dataformat) {
		case 0x03:
		/*2g range*/
			obj->reso = &bma2x2_2g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_2g_data_resolution[CHIP_TYPE];
			break;
		case 0x05:
		/*4g range*/
			obj->reso = &bma2x2_4g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_4g_data_resolution[CHIP_TYPE];
			break;
		case 0x08:
		/*8g range*/
			obj->reso = &bma2x2_8g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_8g_data_resolution[CHIP_TYPE];
			break;
		case 0x0C:
		/*16g range*/
			obj->reso = &bma2x2_16g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_16g_data_resolution[CHIP_TYPE];
			break;
		default:
			obj->reso = &bma2x2_2g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_2g_data_resolution[CHIP_TYPE];
		}

	}
	return 0;

/*if you changed the measure range, for example call:
*BMA2x2_SetDataFormat(client, BMA2x2_RANGE_4G),
*you must set the right value to bma2x2_data_resolution*/

}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadData(struct i2c_client *client, s16 data[BMA2x2_AXES_NUM])
{
	u8 addr = BMA2x2_REG_DATAXLOW;
	u8 buf[BMA2x2_DATA_LEN] = {0};
	int err = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
#ifdef CONFIG_BMA2x2_LOWPASS
	int num = 0;
	int X = BMA2x2_AXIS_X;
	int Y = BMA2x2_AXIS_Y;
	int Z = BMA2x2_AXIS_Z;
#endif
	if (NULL == client)
		err = -EINVAL;
	mutex_lock(&obj->lock);
	err = bma_i2c_read_block(client, addr, buf, BMA2x2_DATA_LEN);
	mutex_unlock(&obj->lock);
	if (err) {
		GSE_ERR("error: %d\n", err);
	} else {
		if (CHIP_TYPE == BMA255_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA255_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA255_ACC_X_MSB) << BMA255_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA255_ACC_X_LSB__LEN
			 + BMA255_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA255_ACC_X_LSB__LEN
			 + BMA255_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA255_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA255_ACC_Y_MSB) << BMA255_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA255_ACC_Y_LSB__LEN
			 + BMA255_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA255_ACC_Y_LSB__LEN
			 + BMA255_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA255_ACC_Z_LSB)
			| (BMA2x2_GET_BITSLICE(buf[5],
			 BMA255_ACC_Z_MSB) << BMA255_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA255_ACC_Z_LSB__LEN
			 + BMA255_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA255_ACC_Z_LSB__LEN
			 + BMA255_ACC_Z_MSB__LEN));
		} else if (CHIP_TYPE == BMA222E_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA222E_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA222E_ACC_X_MSB) << BMA222E_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA222E_ACC_X_LSB__LEN
			 + BMA222E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA222E_ACC_X_LSB__LEN
			 + BMA222E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA222E_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA222E_ACC_Y_MSB) << BMA222E_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA222E_ACC_Y_LSB__LEN
			 + BMA222E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA222E_ACC_Y_LSB__LEN
			 + BMA222E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA222E_ACC_Z_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[5],
			 BMA222E_ACC_Z_MSB) << BMA222E_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA222E_ACC_Z_LSB__LEN
			 + BMA222E_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA222E_ACC_Z_LSB__LEN
			 + BMA222E_ACC_Z_MSB__LEN));
		} else if (CHIP_TYPE == BMA250E_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA250E_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA250E_ACC_X_MSB) << BMA250E_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA250E_ACC_X_LSB__LEN
			 + BMA250E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA250E_ACC_X_LSB__LEN
			 + BMA250E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA250E_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA250E_ACC_Y_MSB) << BMA250E_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA250E_ACC_Y_LSB__LEN
			 + BMA250E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA250E_ACC_Y_LSB__LEN
			 + BMA250E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA250E_ACC_Z_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[5],
			 BMA250E_ACC_Z_MSB) << BMA250E_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA250E_ACC_Z_LSB__LEN
			 + BMA250E_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA250E_ACC_Z_LSB__LEN
			 + BMA250E_ACC_Z_MSB__LEN));
		} else if (CHIP_TYPE == BMA280_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA280_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA280_ACC_X_MSB) << BMA280_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA280_ACC_X_LSB__LEN
			 + BMA280_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA280_ACC_X_LSB__LEN
			 + BMA280_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA280_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA280_ACC_Y_MSB) << BMA280_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA280_ACC_Y_LSB__LEN
			 + BMA280_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA280_ACC_Y_LSB__LEN
			 + BMA280_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA280_ACC_Z_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[5],
			 BMA280_ACC_Z_MSB) << BMA280_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA280_ACC_Z_LSB__LEN
			 + BMA280_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA280_ACC_Z_LSB__LEN
			 + BMA280_ACC_Z_MSB__LEN));
		}

#ifdef CONFIG_BMA2x2_LOWPASS
		if (atomic_read(&priv->filter)) {
			if (atomic_read(&priv->fir_en) &&
			 !atomic_read(&priv->suspend)) {
				int idx, firlen = atomic_read(&priv->firlen);
				num = priv->fir.num;
				if (priv->fir.num < firlen) {
					priv->fir.raw[num][BMA2x2_AXIS_X]
					 = data[BMA2x2_AXIS_X];
					priv->fir.raw[num][BMA2x2_AXIS_Y]
					 = data[BMA2x2_AXIS_Y];
					priv->fir.raw[num][BMA2x2_AXIS_Z]
					 = data[BMA2x2_AXIS_Z];
					priv->fir.sum[BMA2x2_AXIS_X] +=
					 data[BMA2x2_AXIS_X];
					priv->fir.sum[BMA2x2_AXIS_Y] +=
					 data[BMA2x2_AXIS_Y];
					priv->fir.sum[BMA2x2_AXIS_Z] +=
					 data[BMA2x2_AXIS_Z];
					if (atomic_read(&priv->trace) &
					 BMA_TRC_FILTER) {
					/*	GSE_LOG(
						"add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n",
						 priv->fir.num,
						 priv->fir.raw[num][X],
						 priv->fir.raw[num][Y],
						 priv->fir.raw[num][Z],
						 priv->fir.sum[BMA2x2_AXIS_X],
						 priv->fir.sum[BMA2x2_AXIS_Y],
						 priv->fir.sum[BMA2x2_AXIS_Z]); */
					}
					priv->fir.num++;
					priv->fir.idx++;
				} else {
					idx = priv->fir.idx % firlen;
					priv->fir.sum[BMA2x2_AXIS_X] -=
					 priv->fir.raw[idx][BMA2x2_AXIS_X];
					priv->fir.sum[BMA2x2_AXIS_Y] -=
					 priv->fir.raw[idx][BMA2x2_AXIS_Y];
					priv->fir.sum[BMA2x2_AXIS_Z] -=
					 priv->fir.raw[idx][BMA2x2_AXIS_Z];
					priv->fir.raw[idx][BMA2x2_AXIS_X]
					 = data[BMA2x2_AXIS_X];
					priv->fir.raw[idx][BMA2x2_AXIS_Y]
					 = data[BMA2x2_AXIS_Y];
					priv->fir.raw[idx][BMA2x2_AXIS_Z]
					 = data[BMA2x2_AXIS_Z];
					priv->fir.sum[BMA2x2_AXIS_X] +=
					 data[BMA2x2_AXIS_X];
					priv->fir.sum[BMA2x2_AXIS_Y] +=
					 data[BMA2x2_AXIS_Y];
					priv->fir.sum[BMA2x2_AXIS_Z] +=
					 data[BMA2x2_AXIS_Z];
					priv->fir.idx++;
					data[BMA2x2_AXIS_X] =
					 priv->fir.sum[BMA2x2_AXIS_X]/firlen;
					data[BMA2x2_AXIS_Y] =
					 priv->fir.sum[BMA2x2_AXIS_Y]/firlen;
					data[BMA2x2_AXIS_Z] =
					 priv->fir.sum[BMA2x2_AXIS_Z]/firlen;
					if (atomic_read(&priv->trace) &
					 BMA_TRC_FILTER) {
						/*GSE_LOG(
						"[%2d][%5d %5d %5d][%5d %5d %5d]:[%5d %5d %5d]\n",
						 idx,
						 priv->fir.raw[idx][X],
						 priv->fir.raw[idx][Y],
						 priv->fir.raw[idx][Z],
						 priv->fir.sum[BMA2x2_AXIS_X],
						 priv->fir.sum[BMA2x2_AXIS_Y],
						 priv->fir.sum[BMA2x2_AXIS_Z],
						 data[BMA2x2_AXIS_X],
						 data[BMA2x2_AXIS_Y],
						 data[BMA2x2_AXIS_Z]);  */
					}
				}
			}
		}
#endif
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadOffset(struct i2c_client *client, s8 ofs[BMA2x2_AXES_NUM])
{
	int err = 0;
#ifndef SW_CALIBRATION
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
#endif
#ifdef SW_CALIBRATION
	ofs[0] = ofs[1] = ofs[2] = 0x0;
#else
	mutex_lock(&obj->lock);
	err = bma_i2c_read_block(client, BMA2x2_REG_OFSX, ofs, BMA2x2_AXES_NUM);
	mutex_unlock(&obj->lock);
	if (err)
		GSE_ERR("error: %d\n", err);

#endif
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ResetCalibration(struct i2c_client *client)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;

	#ifdef SW_CALIBRATION
	#else
	err = bma_i2c_write_block(client, BMA2x2_REG_OFSX, ofs, 4);
	if (err)
		GSE_ERR("error: %d\n", err);

	#endif
	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadCalibration(struct i2c_client *client,
	 int dat[BMA2x2_AXES_NUM])
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int mul;

	#ifdef SW_CALIBRATION
	/*only SW Calibration, disable HW Calibration*/
	mul = 0;
	#else
	err = BMA2x2_ReadOffset(client, obj->offset);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}
	mul = obj->reso->sensitivity/bma2x2_offset_resolution.sensitivity;
	#endif

	dat[obj->cvt.map[BMA2x2_AXIS_X]] =
	 obj->cvt.sign[BMA2x2_AXIS_X] *
	 (obj->offset[BMA2x2_AXIS_X]*mul + obj->cali_sw[BMA2x2_AXIS_X]);
	dat[obj->cvt.map[BMA2x2_AXIS_Y]] =
	 obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (obj->offset[BMA2x2_AXIS_Y]*mul + obj->cali_sw[BMA2x2_AXIS_Y]);
	dat[obj->cvt.map[BMA2x2_AXIS_Z]] =
	 obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (obj->offset[BMA2x2_AXIS_Z]*mul + obj->cali_sw[BMA2x2_AXIS_Z]);
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadCalibrationEx(struct i2c_client *client,
	 int act[BMA2x2_AXES_NUM], int raw[BMA2x2_AXES_NUM])
{
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
#ifndef SW_CALIBRATION
	int err = 0;
#endif
	int mul;

	#ifdef SW_CALIBRATION
		/*only SW Calibration, disable HW Calibration*/
		mul = 0;
	#else
		err = BMA2x2_ReadOffset(client, obj->offset);
		if (err) {
			GSE_ERR("read offset fail, %d\n", err);
			return err;
		}
		mul = obj->reso->sensitivity /
		 bma2x2_offset_resolution.sensitivity;
	#endif

	raw[BMA2x2_AXIS_X] =
	 obj->offset[BMA2x2_AXIS_X]*mul + obj->cali_sw[BMA2x2_AXIS_X];
	raw[BMA2x2_AXIS_Y] =
	 obj->offset[BMA2x2_AXIS_Y]*mul + obj->cali_sw[BMA2x2_AXIS_Y];
	raw[BMA2x2_AXIS_Z] =
	 obj->offset[BMA2x2_AXIS_Z]*mul + obj->cali_sw[BMA2x2_AXIS_Z];

	act[obj->cvt.map[BMA2x2_AXIS_X]] =
	 obj->cvt.sign[BMA2x2_AXIS_X]*raw[BMA2x2_AXIS_X];
	act[obj->cvt.map[BMA2x2_AXIS_Y]] =
	 obj->cvt.sign[BMA2x2_AXIS_Y]*raw[BMA2x2_AXIS_Y];
	act[obj->cvt.map[BMA2x2_AXIS_Z]] =
	 obj->cvt.sign[BMA2x2_AXIS_Z]*raw[BMA2x2_AXIS_Z];
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_WriteCalibration(struct i2c_client *client,
	 int dat[BMA2x2_AXES_NUM])
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[BMA2x2_AXES_NUM], raw[BMA2x2_AXES_NUM];
	err = BMA2x2_ReadCalibrationEx(client, cali, raw);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG(
	 "OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
	 raw[BMA2x2_AXIS_X], raw[BMA2x2_AXIS_Y], raw[BMA2x2_AXIS_Z],
	 obj->offset[BMA2x2_AXIS_X],
	 obj->offset[BMA2x2_AXIS_Y],
	 obj->offset[BMA2x2_AXIS_Z],
	 obj->cali_sw[BMA2x2_AXIS_X],
	 obj->cali_sw[BMA2x2_AXIS_Y],
	 obj->cali_sw[BMA2x2_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[BMA2x2_AXIS_X] += dat[BMA2x2_AXIS_X];
	cali[BMA2x2_AXIS_Y] += dat[BMA2x2_AXIS_Y];
	cali[BMA2x2_AXIS_Z] += dat[BMA2x2_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n",
	dat[BMA2x2_AXIS_X], dat[BMA2x2_AXIS_Y], dat[BMA2x2_AXIS_Z]);

#ifdef SW_CALIBRATION
	obj->cali_sw[BMA2x2_AXIS_X] = obj->cvt.sign[BMA2x2_AXIS_X] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_X]]);
	obj->cali_sw[BMA2x2_AXIS_Y] = obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Y]]);
	obj->cali_sw[BMA2x2_AXIS_Z] = obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Z]]);
#else
	obj->offset[BMA2x2_AXIS_X] = (s8)(obj->cvt.sign[BMA2x2_AXIS_X] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_X]])/(divisor));
	obj->offset[BMA2x2_AXIS_Y] = (s8)(obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Y]])/(divisor));
	obj->offset[BMA2x2_AXIS_Z] = (s8)(obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[BMA2x2_AXIS_X] = obj->cvt.sign[BMA2x2_AXIS_X] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_X]])%(divisor);
	obj->cali_sw[BMA2x2_AXIS_Y] = obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Y]])%(divisor);
	obj->cali_sw[BMA2x2_AXIS_Z] = obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Z]])%(divisor);

	GSE_LOG(
	"NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
	obj->offset[BMA2x2_AXIS_X]*divisor + obj->cali_sw[BMA2x2_AXIS_X],
	obj->offset[BMA2x2_AXIS_Y]*divisor + obj->cali_sw[BMA2x2_AXIS_Y],
	obj->offset[BMA2x2_AXIS_Z]*divisor + obj->cali_sw[BMA2x2_AXIS_Z],
	obj->offset[BMA2x2_AXIS_X],
	 obj->offset[BMA2x2_AXIS_Y],
	 obj->offset[BMA2x2_AXIS_Z],
	obj->cali_sw[BMA2x2_AXIS_X],
	 obj->cali_sw[BMA2x2_AXIS_Y],
	 obj->cali_sw[BMA2x2_AXIS_Z]);
	err = bma_i2c_write_block(obj->client,
	 BMA2x2_REG_OFSX, obj->offset, BMA2x2_AXES_NUM);
	if (err) {
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
#endif
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_CheckDeviceID(struct i2c_client *client)
{
	u8 data;
	int res = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client, BMA2x2_REG_DEVID, &data, 0x01);
	res = bma_i2c_read_block(client, BMA2x2_REG_DEVID, &data, 0x01);
	mutex_unlock(&obj->lock);
	if (res < 0)
		goto exit_BMA2x2_CheckDeviceID;

	switch (data) {
	case BMA222E_CHIPID:
		CHIP_TYPE = BMA222E_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	case BMA250E_CHIPID:
		CHIP_TYPE = BMA250E_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	case BMA255_CHIPID:
		CHIP_TYPE = BMA255_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	case BMA280_CHIPID:
		CHIP_TYPE = BMA280_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	default:
		CHIP_TYPE = UNKNOWN_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d failt!\n ", data);
		break;
	}

exit_BMA2x2_CheckDeviceID:
	if (res < 0)
		return BMA2x2_ERR_I2C;

	return BMA2x2_SUCCESS;
}
/*----------------------------------------------------------------------------*/
int BMA2x2_DoSetPowerMode(struct i2c_client *client, bool enable) //tuwenzan@wind-mobi.com modify at 20161128
{
	u8 databuf[2] = {0};
	int res = 0;
	u8 temp, temp0, temp1;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	u8 actual_power_mode = 0;
	int count = 0;
	if (enable == sensor_power) {
		GSE_LOG("Sensor power status is newest!\n");
		return BMA2x2_SUCCESS;
	}

	mutex_lock(&obj->lock);
	if (enable == true)
		actual_power_mode = BMA2x2_MODE_NORMAL;
	else
		actual_power_mode = BMA2x2_MODE_SUSPEND;
	res = bma_i2c_read_block(client, 0x3E, &temp, 0x1);
	udelay(1000);
	if (res < 0)
		GSE_LOG("read  config failed!\n");
	switch (actual_power_mode) {
	case BMA2x2_MODE_NORMAL:
		databuf[0] = 0x00;
		databuf[1] = 0x00;
		while (count < 10) {
			res = bma_i2c_write_block(client,
				BMA2x2_MODE_CTRL_REG, &databuf[0], 1);
			udelay(1000);
			if (res < 0)
				GSE_LOG("write MODE_CTRL_REG failed!\n");
			res = bma_i2c_write_block(client,
				BMA2x2_LOW_POWER_CTRL_REG, &databuf[1], 1);
			udelay(2000);
			if (res < 0)
				GSE_LOG("write LOW_POWER_CTRL_REG failed!\n");
			res =
			 bma_i2c_read_block(client,
			 BMA2x2_MODE_CTRL_REG, &temp0, 0x1);
			if (res < 0)
				GSE_LOG("read MODE_CTRL_REG failed!\n");
			res =
			 bma_i2c_read_block(client,
			 BMA2x2_LOW_POWER_CTRL_REG, &temp1, 0x1);
			if (res < 0)
				GSE_LOG("read LOW_POWER_CTRL_REG failed!\n");
			if (temp0 != databuf[0]) {
				GSE_LOG("readback MODE_CTRL failed!\n");
				count++;
				continue;
			} else if (temp1 != databuf[1]) {
				GSE_LOG("readback LOW_POWER_CTRL failed!\n");
				count++;
				continue;
			} else {
				GSE_LOG("configure powermode success\n");
				break;
			}
		}
		udelay(1000);
		break;
	case BMA2x2_MODE_SUSPEND:
		databuf[0] = 0x80;
		databuf[1] = 0x00;
		while (count < 10) {
			res = bma_i2c_write_block(client,
				BMA2x2_LOW_POWER_CTRL_REG, &databuf[1], 1);
			udelay(1000);
			if (res < 0)
				GSE_LOG("write LOW_POWER_CTRL_REG failed!\n");
			res = bma_i2c_write_block(client,
				BMA2x2_MODE_CTRL_REG, &databuf[0], 1);
			udelay(1000);
			res = bma_i2c_write_block(client, 0x3E, &temp, 0x1);
			if (res < 0)
				GSE_LOG("write  config failed!\n");
			udelay(1000);
			res = bma_i2c_write_block(client, 0x3E, &temp, 0x1);
			if (res < 0)
				GSE_LOG("write  config failed!\n");
			udelay(2000);
			if (res < 0)
				GSE_LOG("write BMA2x2_MODE_CTRL_REG failed!\n");
			res =
			bma_i2c_read_block(client,
			 BMA2x2_MODE_CTRL_REG, &temp0, 0x1);
			if (res < 0)
				GSE_LOG("read BMA2x2_MODE_CTRL_REG failed!\n");
			res =
			bma_i2c_read_block(client,
			 BMA2x2_LOW_POWER_CTRL_REG, &temp1, 0x1);
			if (res < 0)
				GSE_LOG("read BLOW_POWER_CTRL_REG failed!\n");
			if (temp0 != databuf[0]) {
				GSE_LOG("readback MODE_CTRL failed!\n");
				count++;
				continue;
			} else if (temp1 != databuf[1]) {
				GSE_LOG("readback LOW_POWER_CTRL failed!\n");
				count++;
				continue;
			} else {
				GSE_LOG("configure powermode success\n");
				break;
			}
		}
		udelay(1000);
	break;
	case BMA2x2_MODE_LOWPOWER:
		databuf[0] = 0x40;
		databuf[1] = 0x00;
		while (count < 10) {
			res = bma_i2c_write_block(client,
				BMA2x2_LOW_POWER_CTRL_REG, &databuf[1], 1);
			udelay(1000);
			if (res < 0)
				GSE_LOG("write LOW_POWER_CTRL_REG failed!\n");
			res = bma_i2c_write_block(client,
				BMA2x2_MODE_CTRL_REG, &databuf[0], 1);
			udelay(1000);
			res = bma_i2c_write_block(client, 0x3E, &temp, 0x1);
			if (res < 0)
				GSE_LOG("write  config failed!\n");
			udelay(1000);
			res = bma_i2c_write_block(client, 0x3E, &temp, 0x1);
			if (res < 0)
				GSE_LOG("write  config failed!\n");
			udelay(2000);
			if (res < 0)
				GSE_LOG("write BMA2x2_MODE_CTRL_REG failed!\n");
			res =
			bma_i2c_read_block(client,
			 BMA2x2_MODE_CTRL_REG, &temp0, 0x1);
			if (res < 0)
				GSE_LOG("read BMA2x2_MODE_CTRL_REG failed!\n");
			res =
			bma_i2c_read_block(client,
			 BMA2x2_LOW_POWER_CTRL_REG, &temp1, 0x1);
			if (res < 0)
				GSE_LOG("read BLOW_POWER_CTRL_REG failed!\n");
			if (temp0 != databuf[0]) {
				GSE_LOG("readback MODE_CTRL failed!\n");
				count++;
				continue;
			} else if (temp1 != databuf[1]) {
				GSE_LOG("readback LOW_POWER_CTRL failed!\n");
				count++;
				continue;
			} else {
				GSE_LOG("configure powermode success\n");
				break;
			}
		}
		udelay(1000);
	break;
	}

	if (res < 0) {
		GSE_ERR("set power mode failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMA2x2_ERR_I2C;
	}
	sensor_power = enable;
	mutex_unlock(&obj->lock);
	return BMA2x2_SUCCESS;
}

int BMA2x2_SetPowerMode(struct i2c_client *client, bool enable, int who)
{
	unsigned char mode = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);

	mutex_lock(&obj->mode_mutex);
	mode = obj->mode;
	if (enable)
		mode |= (1<<who);
	else
		mode &= (~(1<<who));
	if (mode && !obj->mode) {
		GSE_LOG("enter normal mode\n");
		BMA2x2_DoSetPowerMode(client, true);
	} else if (!mode && obj->mode) {
		GSE_LOG("enter suspend mode\n");
		BMA2x2_DoSetPowerMode(client, false);
	}
	obj->mode = mode;
	mutex_unlock(&obj->mode_mutex);

	return BMA2x2_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int BMA2x2_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[2] = {0};
	int res = 0;

	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client,
	BMA2x2_RANGE_SEL_REG, &databuf[0], 1);
	databuf[0] = BMA2x2_SET_BITSLICE(databuf[0],
	 BMA2x2_RANGE_SEL, dataformat);
	res += bma_i2c_write_block(client,
	 BMA2x2_RANGE_SEL_REG, &databuf[0], 1);
	udelay(500);

	if (res < 0) {
		GSE_ERR("set data format failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMA2x2_ERR_I2C;
	}
	mutex_unlock(&obj->lock);
	return BMA2x2_SetDataResolution(obj, dataformat);
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[2] = {0};
	int res = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);

	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client,
	 BMA2x2_BANDWIDTH__REG, &databuf[0], 1);
	databuf[0] = BMA2x2_SET_BITSLICE(databuf[0],
	 BMA2x2_BANDWIDTH, bwrate);
	res += bma_i2c_write_block(client,
	 BMA2x2_BANDWIDTH__REG, &databuf[0], 1);
	udelay(500);

	if (res < 0) {
		GSE_ERR("set bandwidth failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMA2x2_ERR_I2C;
	}
	mutex_unlock(&obj->lock);

	return BMA2x2_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	int res = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);

	mutex_lock(&obj->lock);
	res = bma_i2c_write_block(client, BMA2x2_INT_REG_1, &intenable, 0x01);
	udelay(500);
	if (res != BMA2x2_SUCCESS) {
		mutex_unlock(&obj->lock);
		return res;
	}

	res = bma_i2c_write_block(client, BMA2x2_INT_REG_2, &intenable, 0x01);
	udelay(500);
	if (res != BMA2x2_SUCCESS) {
		mutex_unlock(&obj->lock);
		return res;
	}
	mutex_unlock(&obj->lock);
	GSE_LOG("BMA255 disable interrupt ...\n");
	return BMA2x2_SUCCESS;
}

static int BMA2x2_DisableTempSensor(struct i2c_client *client)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	u8 temp = 0;
	mutex_lock(&obj->lock);
	temp = 0xCA;
	res += bma_i2c_write_block(client,
		0x35, &temp, 1);
	udelay(1000);
	res += bma_i2c_write_block(client,
		0x35, &temp, 1);
	udelay(1000);
	temp = 0x00;
	res += bma_i2c_write_block(client,
		0x4F, &temp, 1);
	udelay(1000);
	temp = 0x0A;
	res += bma_i2c_write_block(client,
		0x35, &temp, 1);
	udelay(1000);
	mutex_unlock(&obj->lock);
	return res;
}

/*----------------------------------------------------------------------------*/
static int bma2x2_init_client(struct i2c_client *client, int reset_cali)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	GSE_LOG("bma2x2_init_client\n");

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	if (obj->mEnabled) {/* aod is on */
		GSE_LOG("bma2x2_init_client: skip for aod is on\n");
		return BMA2x2_SUCCESS;
	}
#endif

	/*should add normal mode setting*/
	res = BMA2x2_SetPowerMode(client, true, BMA25X_ACC);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetPowerMode to normal OK!\n");

	res = BMA2x2_DisableTempSensor(client);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("temperature sensor disabled!\n");

	res = BMA2x2_CheckDeviceID(client);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_CheckDeviceID ok\n");

	res = BMA2x2_SetBWRate(client, BMA2x2_BW_100HZ);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetBWRate OK!\n");

	res = BMA2x2_SetDataFormat(client, BMA2x2_RANGE_2G);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetDataFormat OK!\n");

	gsensor_gain.x = gsensor_gain.y =
	 gsensor_gain.z = obj->reso->sensitivity;

	res = BMA2x2_SetIntEnable(client, 0x00);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA255 disable interrupt function!\n");

	res = BMA2x2_SetPowerMode(client, false, BMA25X_ACC);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetPowerMode to suspend OK!\n");

	if (0 != reset_cali) {
		/*reset calibration only in power on*/
		res = BMA2x2_ResetCalibration(client);
		if (res != BMA2x2_SUCCESS)
			return res;
	}

	GSE_LOG("bma2x2_init_client OK!\n");
#ifdef CONFIG_BMA2x2_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	msleep(20);

	return BMA2x2_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadChipInfo(struct i2c_client *client,
	 char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8)*10);

	if ((NULL == buf) || (bufsize <= 30))
		return BMA2x2_ERR_I2C;

	if (NULL == client) {
		*buf = 0;
		return BMA2x2_ERR_CLIENT;
	}

	switch (CHIP_TYPE) {
	case BMA222E_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA222E Chip");
		break;
	case BMA250E_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA250E Chip");
		break;
	case BMA255_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA255 Chip");
		break;
	case BMA280_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA280 Chip");
		break;
	default:
		snprintf(buf, BMA2x2_BUFSIZE, "UNKOWN Chip");
		break;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_CompassReadData(struct i2c_client *client,
	 char *buf, int bufsize)
{
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);
	int acc[BMA2x2_AXES_NUM];
	int res = 0;
	s16 databuf[BMA2x2_AXES_NUM];

	if (NULL == buf)
		return BMA2x2_ERR_I2C;

	if (NULL == client) {
		*buf = 0;
		return BMA2x2_ERR_CLIENT;
	}

	if (sensor_power == false) {
		res = BMA2x2_SetPowerMode(client, true, BMA25X_ACC);
		if (res)
			GSE_ERR("Power on bma255 error %d!\n", res);

	}
	res = BMA2x2_ReadData(client, databuf);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return BMA2x2_ERR_STATUS;
	} else {
		/* Add compensated value performed by MTK calibration process*/
		databuf[BMA2x2_AXIS_X] += obj->cali_sw[BMA2x2_AXIS_X];
		databuf[BMA2x2_AXIS_Y] += obj->cali_sw[BMA2x2_AXIS_Y];
		databuf[BMA2x2_AXIS_Z] += obj->cali_sw[BMA2x2_AXIS_Z];

		/*remap coordinate*/
		acc[obj->cvt.map[BMA2x2_AXIS_X]] =
		 obj->cvt.sign[BMA2x2_AXIS_X]*databuf[BMA2x2_AXIS_X];
		acc[obj->cvt.map[BMA2x2_AXIS_Y]] =
		 obj->cvt.sign[BMA2x2_AXIS_Y]*databuf[BMA2x2_AXIS_Y];
		acc[obj->cvt.map[BMA2x2_AXIS_Z]] =
		 obj->cvt.sign[BMA2x2_AXIS_Z]*databuf[BMA2x2_AXIS_Z];

		snprintf(buf, BMA2x2_BUFSIZE, "%d %d %d",
		 (s16)acc[BMA2x2_AXIS_X],
		 (s16)acc[BMA2x2_AXIS_Y],
		 (s16)acc[BMA2x2_AXIS_Z]);
		if (atomic_read(&obj->trace) & BMA_TRC_IOCTL)
			GSE_LOG("gsensor data for compass: %s!\n", buf);

	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadSensorData(struct i2c_client *client,
	 char *buf, int bufsize)
{
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);
	int acc[BMA2x2_AXES_NUM];
	int res = 0;
	s16 databuf[BMA2x2_AXES_NUM];

	if (NULL == buf)
		return BMA2x2_ERR_I2C;

	if (NULL == client) {
		*buf = 0;
		return BMA2x2_ERR_CLIENT;
	}

	if (sensor_power == false) {
		res = BMA2x2_SetPowerMode(client, true, BMA25X_ACC);
		if (res)
			GSE_ERR("Power on bma255 error %d!\n", res);
	}
	res = BMA2x2_ReadData(client, databuf);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return BMA2x2_ERR_STATUS;
	} else {
		databuf[BMA2x2_AXIS_X] += obj->cali_sw[BMA2x2_AXIS_X];
		databuf[BMA2x2_AXIS_Y] += obj->cali_sw[BMA2x2_AXIS_Y];
		databuf[BMA2x2_AXIS_Z] += obj->cali_sw[BMA2x2_AXIS_Z];

		/*remap coordinate*/
		acc[obj->cvt.map[BMA2x2_AXIS_X]] =
		 obj->cvt.sign[BMA2x2_AXIS_X]*databuf[BMA2x2_AXIS_X];
		acc[obj->cvt.map[BMA2x2_AXIS_Y]] =
		 obj->cvt.sign[BMA2x2_AXIS_Y]*databuf[BMA2x2_AXIS_Y];
		acc[obj->cvt.map[BMA2x2_AXIS_Z]] =
		 obj->cvt.sign[BMA2x2_AXIS_Z]*databuf[BMA2x2_AXIS_Z];
		/*GSE_LOG("cvt x=%d, y=%d, z=%d\n",
		 obj->cvt.sign[BMA2x2_AXIS_X],
		 obj->cvt.sign[BMA2x2_AXIS_Y],
		 obj->cvt.sign[BMA2x2_AXIS_Z]);  */

		acc[BMA2x2_AXIS_X] =
		 acc[BMA2x2_AXIS_X] * GRAVITY_EARTH_1000
		 / obj->reso->sensitivity;
		acc[BMA2x2_AXIS_Y] =
		 acc[BMA2x2_AXIS_Y] * GRAVITY_EARTH_1000
		 / obj->reso->sensitivity;
		acc[BMA2x2_AXIS_Z] =
		 acc[BMA2x2_AXIS_Z] * GRAVITY_EARTH_1000
		 / obj->reso->sensitivity;

		/*GSE_ERR("Mapped gsensor data: %d, %d, %d!\n",
		 acc[BMA2x2_AXIS_X],
		 acc[BMA2x2_AXIS_Y],
		 acc[BMA2x2_AXIS_Z]); */
		snprintf(buf, BMA2x2_BUFSIZE, "%04x %04x %04x",
		 acc[BMA2x2_AXIS_X],
		 acc[BMA2x2_AXIS_Y],
		 acc[BMA2x2_AXIS_Z]);
		if (atomic_read(&obj->trace) & BMA_TRC_IOCTL)
			GSE_LOG("gsensor data: %s!\n", buf);
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadRawData(struct i2c_client *client, char *buf)
{
	int res = 0;
	s16 databuf[BMA2x2_AXES_NUM];

	if (!buf || !client)
		return -EINVAL;

	res = BMA2x2_ReadData(client, databuf);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	} else {
		snprintf(buf, BMA2x2_BUFSIZE,
		 "BMA2x2_ReadRawData %04x %04x %04x", databuf[BMA2x2_AXIS_X],
		 databuf[BMA2x2_AXIS_Y], databuf[BMA2x2_AXIS_Z]);
	}
	return 0;
}
int bma2x2_softreset(void)
{
	unsigned char temp = BMA2x2_SOFTRESET;
	int ret = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (bma2x2_i2c_client == NULL)
		return -EINVAL;
	mutex_lock(&obj->lock);
	ret = bma_i2c_write_block(bma2x2_i2c_client,
		BMA2x2_SOFTRESET_REG, &temp, 1);
	mutex_unlock(&obj->lock);
	msleep(30);
	if (ret < 0)
		GSE_ERR("bma2x2_softreset failed");
	return ret;

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_mode(struct i2c_client *client, unsigned char *mode)
{
	int comres = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);

	if (client == NULL)
		return BMA2x2_ERR_I2C;
	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
	 BMA2x2_EN_LOW_POWER__REG, mode, 1);
	mutex_unlock(&obj->lock);
	*mode  = (*mode) >> 6;

	return comres;
}

/*----------------------------------------------------------------------------*/
int bma2x2_set_range(struct i2c_client *client, unsigned char range) //tuwenzan@wind-mobi.com modify at 20161128
{
	int comres = 0;
	unsigned char data[2] = {BMA2x2_RANGE_SEL__REG};
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);

	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
			BMA2x2_RANGE_SEL__REG, data+1, 1);

	data[1]  = BMA2x2_SET_BITSLICE(data[1],
	 BMA2x2_RANGE_SEL, range);

	comres = i2c_master_send(client, data, 2);
	mutex_unlock(&obj->lock);
	if (comres <= 0)
		return BMA2x2_ERR_I2C;
	else {
		BMA2x2_SetDataResolution(obj, range);
		return comres;
	}

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_range(struct i2c_client *client, unsigned char *range)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_RANGE_SEL__REG, &data, 1);
	mutex_unlock(&obj->lock);
	*range = BMA2x2_GET_BITSLICE(data, BMA2x2_RANGE_SEL);

	return comres;
}
/*----------------------------------------------------------------------------*/
int bma2x2_set_bandwidth(struct i2c_client *client,unsigned char bandwidth)  //tuwenzan@wind-mobi.com modify at 20161128
{
	int comres = 0;
	unsigned char data[2] = {BMA2x2_BANDWIDTH__REG};
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);

	if (client == NULL)
		return BMA2x2_ERR_I2C;


	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
	 BMA2x2_BANDWIDTH__REG, data+1, 1);

	data[1] = BMA2x2_SET_BITSLICE(data[1],
	 BMA2x2_BANDWIDTH, bandwidth);

	comres = i2c_master_send(client, data, 2);
	mutex_unlock(&obj->lock);
	if (comres <= 0)
		return BMA2x2_ERR_I2C;
	else
		return comres;

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_bandwidth(struct i2c_client *client,
	 unsigned char *bandwidth)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_BANDWIDTH__REG, &data, 1);
	mutex_unlock(&obj->lock);
	data = BMA2x2_GET_BITSLICE(data, BMA2x2_BANDWIDTH);

	if (data < 0x08) {
		/*7.81Hz*/
		*bandwidth = 0x08;
	} else if (data > 0x0f) {
		/* 1000Hz*/
		*bandwidth = 0x0f;
	} else {
		*bandwidth = data;
	}
	return comres;
}

/*----------------------------------------------------------------------------*/
static int bma2x2_set_fifo_mode(struct i2c_client *client,
	 unsigned char fifo_mode)
{
	int comres = 0;
	unsigned char data[2] = {BMA2x2_FIFO_MODE__REG};
	struct bma2x2_i2c_data *obj =
	(struct bma2x2_i2c_data *)i2c_get_clientdata(client);

	if (client == NULL || fifo_mode >= 4)
		return BMA2x2_ERR_I2C;


	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
	 BMA2x2_FIFO_MODE__REG, data+1, 1);

	data[1]  = BMA2x2_SET_BITSLICE(data[1],
	 BMA2x2_FIFO_MODE, fifo_mode);

	comres = i2c_master_send(client, data, 2);
	mutex_unlock(&obj->lock);
	if (comres <= 0)
		return BMA2x2_ERR_I2C;
	else
		return comres;

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_fifo_mode(struct i2c_client *client,
	 unsigned char *fifo_mode)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_FIFO_MODE__REG, &data, 1);
	mutex_unlock(&obj->lock);
	*fifo_mode = BMA2x2_GET_BITSLICE(data, BMA2x2_FIFO_MODE);

	return comres;
}

static int bma2x2_get_fifo_framecount(struct i2c_client *client,
	 unsigned char *framecount)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_FIFO_FRAME_COUNTER_S__REG,
	 &data, 1);
	mutex_unlock(&obj->lock);
	*framecount = BMA2x2_GET_BITSLICE(data, BMA2x2_FIFO_FRAME_COUNTER_S);
	return comres;
}

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static int bma25x_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0)
		return -EIO;
	*data = dummy & 0x000000ff;

	return 0;
}

static int bma25x_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;

	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0)
		return -EIO;
	udelay(2);
	return 0;
}

static int bma25x_set_theta_flat(struct i2c_client *client, unsigned char
		thetaflat)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bma25x_smbus_read_byte(client, BMA25X_THETA_FLAT__REG, &data);
	data = BMA25X_SET_BITSLICE(data, BMA25X_THETA_FLAT, thetaflat);
	comres = bma25x_smbus_write_byte(client, BMA25X_THETA_FLAT__REG, &data);

	return comres;
}

static int bma25x_set_flat_hold_time(struct i2c_client *client, unsigned char
		holdtime)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bma25x_smbus_read_byte(client, BMA25X_FLAT_HOLD_TIME__REG,
			&data);
	data = BMA25X_SET_BITSLICE(data, BMA25X_FLAT_HOLD_TIME, holdtime);
	comres = bma25x_smbus_write_byte(client, BMA25X_FLAT_HOLD_TIME__REG,
			&data);

	return comres;
}

static int bma25x_set_slope_no_mot_duration(struct i2c_client *client,
			unsigned char duration)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bma25x_smbus_read_byte(client,
			BMA25X_SLO_NO_MOT_DUR__REG, &data);
	data = BMA25X_SET_BITSLICE(data, BMA25X_SLO_NO_MOT_DUR, duration);
	comres = bma25x_smbus_write_byte(client,
			BMA25X_SLO_NO_MOT_DUR__REG, &data);

	return comres;
}

static int bma25x_set_slope_no_mot_threshold(struct i2c_client *client,
		unsigned char threshold)
{
	int comres = 0;
	unsigned char data = 0;

	data = threshold;
	comres = bma25x_smbus_write_byte(client,
			BMA25X_SLO_NO_MOT_THRES_REG, &data);

	return comres;
}

static int bma25x_set_en_no_motion_int(bma25x_data *bma25x,
		int en)
{
	int err = 0;
	struct i2c_client *client = bma25x->client;

	if (en) {
		/*dur: 192 samples ~= 3s, threshold: 32.25mg, no motion select*/
		err = bma25x_set_slope_no_mot_duration(client, 0x02);
		err += bma25x_set_slope_no_mot_threshold(client, 0x09);
		/*Enable the interrupts*/
		err += bma25x_set_Int_Enable(client, 12, 1);/*slow/no motion X*/
		err += bma25x_set_Int_Enable(client, 13, 1);/*slow/no motion Y*/
		err += bma25x_set_Int_Enable(client, 15, 1);
	} else {
		err = bma25x_set_Int_Enable(client, 12, 0);/*slow/no motion X*/
		err += bma25x_set_Int_Enable(client, 13, 0);/*slow/no motion Y*/
	}
	return err;
}

static int bma25x_flat_update(bma25x_data *bma25x)
{
	static struct bma25xacc acc;
	int status;

	bma2x2_get_data(&acc.x , &acc.y, &acc.z, &status);

	//bma25x_read_accel_xyz(bma25x->client,
	//		bma25x->sensor_type, &acc);
	ISR_INFO(&bma25x->client->dev,
		"bma25x_flat_updatez value = %d, %d\n", acc.z, bma25x->flat_threshold);
	if (acc.z > bma25x->flat_threshold)
		bma25x->flat_up_value = FLATUP_GESTURE;
	else
		bma25x->flat_up_value = EXIT_FLATUP_GESTURE;
	if (acc.z < (-1 * bma25x->flat_threshold))
		bma25x->flat_down_value = FLATDOWN_GESTURE;
	else
		bma25x->flat_down_value = EXIT_FLATDOWN_GESTURE;

	if (TEST_BIT(FlatUp, bma25x->mEnabled)) {
		dev_info(&bma25x->client->dev,
			"update FlatUp value =%d\n", bma25x->flat_up_value);
		input_report_rel(bma25x->dev_interrupt,
				FLAT_INTERRUPT, bma25x->flat_up_value);
		input_sync(bma25x->dev_interrupt);
	}
	if (TEST_BIT(FlatDown, bma25x->mEnabled)) {
		dev_info(&bma25x->client->dev,
			"update FlatDown value =%d\n", bma25x->flat_down_value);
		input_report_rel(bma25x->dev_interrupt,
				FLAT_INTERRUPT, bma25x->flat_down_value);
		input_sync(bma25x->dev_interrupt);
	}

	return 0;
}

static int bma25x_set_en_sig_int_mode(bma25x_data *bma25x,
		int en)
{
	int err = 0;
	int newstatus = en;
#if 0
	unsigned char databuf = 0;
#endif

	ISR_INFO(&bma25x->client->dev,
			"int_mode entry value = %x  %x\n",
			bma25x->mEnabled, newstatus);
	mutex_lock(&bma25x->int_mode_mutex);

	/* handle flat up/down */
	if (!bma25x->mEnabled && newstatus) {
		/* set normal mode at first if needed */
		BMA2x2_SetPowerMode(bma25x->client, true, BMA25X_AOD);
		bma2x2_set_bandwidth(
			bma25x->client, BMA25X_BW_500HZ);
		bma25x_flat_update(bma25x);
		bma25x_set_theta_flat(bma25x->client, 0x08);
		if ((bma25x->flat_up_value == FLATUP_GESTURE) ||
			(bma25x->flat_down_value == FLATDOWN_GESTURE))
			bma25x_set_flat_hold_time(bma25x->client, 0x00);
		else
			bma25x_set_flat_hold_time(bma25x->client, 0x03);
		bma25x_set_Int_Enable(bma25x->client, 11, 1);
	} else if (bma25x->mEnabled && !newstatus) {
		bma25x_set_Int_Enable(bma25x->client, 11, 0);
		cancel_delayed_work_sync(&bma25x->flat_work);
	}
	if (TEST_BIT(FlatUp, newstatus) &&
			!TEST_BIT(FlatUp, bma25x->mEnabled)) {
		ISR_INFO(&bma25x->client->dev,
		"int_mode FlatUp value =%d\n", bma25x->flat_up_value);
		input_report_rel(bma25x->dev_interrupt,
		FLAT_INTERRUPT, bma25x->flat_up_value);
		input_sync(bma25x->dev_interrupt);
	}
	if (TEST_BIT(FlatDown, newstatus) &&
			!TEST_BIT(FlatDown, bma25x->mEnabled)) {
		ISR_INFO(&bma25x->client->dev,
		"int_mode FlatDown value =%d\n", bma25x->flat_down_value);
		input_report_rel(bma25x->dev_interrupt,
		FLAT_INTERRUPT, bma25x->flat_down_value);
		input_sync(bma25x->dev_interrupt);
	}

	/* handle motion */
	if (TEST_BIT(Motion, newstatus) &&
			!TEST_BIT(Motion, bma25x->mEnabled))
		err = bma25x_set_en_no_motion_int(bma25x, 1);
	else if (!TEST_BIT(Motion, newstatus) &&
			TEST_BIT(Motion, bma25x->mEnabled))
		err = bma25x_set_en_no_motion_int(bma25x, 0);

	if (!bma25x->mEnabled && newstatus) {
		enable_irq_wake(bma25x->IRQ1);
		enable_irq_wake(bma25x->IRQ2);
	}
	else if (bma25x->mEnabled && !newstatus) {
		disable_irq_wake(bma25x->IRQ1);
		disable_irq_wake(bma25x->IRQ2);
		bma2x2_set_bandwidth(
			bma25x->client,BMA25X_BW_500HZ);
		BMA2x2_SetPowerMode(bma25x->client, false, BMA25X_AOD);
	}

	/* set low power mode at the end if no need */
	if (bma25x->mEnabled && !newstatus) {
		bma25x->mEnabled = newstatus;/* mEnabled will be used in below func */
		BMA2x2_SetPowerMode(bma25x->client, false, BMA25X_AOD);
	}

	bma25x->mEnabled = newstatus;
	mutex_unlock(&bma25x->int_mode_mutex);
	ISR_INFO(&bma25x->client->dev, "int_mode finished!!!\n");
	return err;
}

static int bma25x_get_orient_flat_status(struct i2c_client *client, unsigned
		char *intstatus)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bma25x_smbus_read_byte(client, BMA25X_STATUS_ORIENT_HIGH_REG,
			&data);
	data = BMA25X_GET_BITSLICE(data, BMA25X_FLAT_S);
	*intstatus = data;

	return comres;
}

static void bma25x_flat_work_func(struct work_struct *work)
{
	bma25x_data *data = container_of((struct delayed_work *)work,
		bma25x_data, flat_work);
	static struct bma25xacc acc;
	unsigned char sign_value = 0, sign_value2 = 0;
	int flat_up_value = 0;
	int flat_down_value = 0;
	int loop = 5;
	int status;
	ISR_INFO(&data->client->dev, "bma25x_flat_work_func entry: %d\n", atomic_read(&data->flat_flag));
	flat_up_value = data->flat_up_value;
	flat_down_value = data->flat_down_value;

	do{
		bma25x_get_orient_flat_status(data->client,
				&sign_value);
		ISR_INFO(&data->client->dev,
		"flat interrupt sign_value=%d\n", sign_value);
		//bma25x_read_accel_xyz(data->client,
		//		data->sensor_type, &acc);
		bma2x2_get_data(&acc.x , &acc.y, &acc.z, &status);

		ISR_INFO(&data->client->dev,
		"flat interrupt acc x,y,z=%d %d %d\n", acc.x, acc.y, acc.z);
		if (1 == sign_value) {
			if (acc.z > 0)
				data->flat_up_value = FLATUP_GESTURE;
			else
				data->flat_up_value = EXIT_FLATUP_GESTURE;
			if (acc.z < 0)
				data->flat_down_value = FLATDOWN_GESTURE;
			else
				data->flat_down_value = EXIT_FLATDOWN_GESTURE;
			if (data->flat_down_value == FLATDOWN_GESTURE)
				bma25x_set_flat_hold_time(data->client, 0x01);
			else
				bma25x_set_flat_hold_time(data->client, 0x00);
		} else {
			data->flat_up_value = EXIT_FLATUP_GESTURE;
			data->flat_down_value = EXIT_FLATDOWN_GESTURE;
			if (atomic_read(&data->flat_flag))
				bma25x_set_flat_hold_time(data->client, 0x00);
			else
				bma25x_set_flat_hold_time(data->client, 0x03);
		}
		bma25x_set_Int_Enable(data->client, 11, 1);
		bma25x_get_orient_flat_status(data->client,
				&sign_value2);
		ISR_INFO(&data->client->dev,
		"flat interrupt sign_value2=%d\n", sign_value2);
	} while (sign_value2 != sign_value && loop--);

	if (sign_value2 != sign_value)
		dev_info(&data->client->dev,
			"flat not stable\n");

	atomic_set(&data->flat_flag, 0);

	if (TEST_BIT(FlatUp, data->mEnabled) &&
		(data->flat_up_value != flat_up_value)) {
		input_report_rel(data->dev_interrupt,
				FLAT_INTERRUPT, data->flat_up_value);
		input_sync(data->dev_interrupt);
	}
	if (TEST_BIT(FlatDown, data->mEnabled) &&
		(data->flat_down_value != flat_down_value)) {
		input_report_rel(data->dev_interrupt,
				FLAT_INTERRUPT, data->flat_down_value);
		input_sync(data->dev_interrupt);
	}
	if (TEST_BIT(Motion, data->mEnabled)) {
		if ((data->flat_up_value != flat_up_value) &&
			(flat_up_value == FLATUP_GESTURE)) {
			dev_info(&data->client->dev,
				"glance exit flat up interrupt happened\n");
			input_report_rel(data->dev_interrupt,
					FLAT_INTERRUPT,
					GLANCE_EXIT_FLATUP_GESTURE);
			input_sync(data->dev_interrupt);
			wake_lock_timeout(&data->aod_wakelock, msecs_to_jiffies(100));
		}
		if ((data->flat_down_value != flat_down_value) &&
			(flat_down_value == FLATDOWN_GESTURE)) {
			dev_info(&data->client->dev,
				"glance exit flat down interrupt happened\n");
			input_report_rel(data->dev_interrupt,
					FLAT_INTERRUPT,
					GLANCE_EXIT_FLATDOWN_GESTURE);
			input_sync(data->dev_interrupt);
			wake_lock_timeout(&data->aod_wakelock, msecs_to_jiffies(100));
		}
	}
	ISR_INFO(&data->client->dev, "bma25x_flat_work_func finished\n");
}

static void bma25x_int1_irq_work_func(struct work_struct *work)
{
	bma25x_data *data = obj_i2c_data;// container_of((struct work_struct *)work,
//		bma25x_data, int1_irq_work);
	unsigned char status = 0;
	unsigned char slow_data = 0;
	bma25x_smbus_read_byte(data->client,
		BMA25X_STATUS1_REG, &status);
	ISR_INFO(&data->client->dev,
		"bma25x_int1_irq_work_func entry status=0x%x\n", status);
	if (0 == data->mEnabled) {
		dev_info(&data->client->dev,
		"flat interrupt mEnabled=%d\n", data->mEnabled);
		goto exit;
	}
	switch (status) {
#ifndef BMA25X_MAP_FLAT_TO_INT2
	case 0x80:
		queue_delayed_work(data->data_wq,
		&data->flat_work, msecs_to_jiffies(50));
		break;
	case 0x88:
#endif
	case 0x08:
		bma25x_smbus_read_byte(
		data->client, 0x18, &slow_data);
		if (TEST_BIT(3, slow_data)) {
			dev_info(&data->client->dev,
				"no motion interrupt happened\n");
			bma25x_set_Int_Enable(
				data->client, 12, 0);
			bma25x_set_Int_Enable(
				data->client, 13, 0);
			bma25x_set_slope_no_mot_duration(
				data->client, 0x00);
			bma25x_set_slope_no_mot_threshold(
				data->client, 0x32);
			bma25x_set_Int_Enable(
				data->client, 12, 1);
			bma25x_set_Int_Enable(
				data->client, 13, 1);
			bma25x_set_Int_Enable(
				data->client, 15, 0);
			cancel_delayed_work_sync(&data->flat_work);
			atomic_set(&data->flat_flag, 1);
			queue_delayed_work(data->data_wq, &data->flat_work, 0);
		} else {
			dev_info(&data->client->dev,
				"glance slow motion interrupt happened\n");
			if (data->flat_down_value != FLATDOWN_GESTURE &&
				data->flat_up_value != FLATUP_GESTURE) {
				input_report_rel(data->dev_interrupt,
					SLOW_NO_MOTION_INTERRUPT,
					GLANCE_MOVEMENT_GESTURE);
				input_sync(data->dev_interrupt);
				wake_lock_timeout(&data->aod_wakelock, msecs_to_jiffies(100));
			}
			bma25x_set_Int_Enable(
				data->client, 12, 0);
			bma25x_set_Int_Enable(
				data->client, 13, 0);
			bma25x_set_slope_no_mot_duration(
				data->client, 0x01);
			if (data->aod_flag) {
				cancel_delayed_work_sync(&data->flat_work);
				data->aod_flag = 0;
			}
			bma25x_set_slope_no_mot_threshold(
				data->client, 0x09);
			bma25x_set_Int_Enable(
				data->client, 12, 1);
			bma25x_set_Int_Enable(
				data->client, 13, 1);
			bma25x_set_Int_Enable(
				data->client, 15, 1);
		}
		break;
	default:
		break;
	}
exit:
	ISR_INFO(&data->client->dev,
		"bma25x_int1_irq_work_func finished!!\n");
}

static int bma25x_set_Int_Enable(struct i2c_client *client, unsigned char
		InterruptType , unsigned char value)
{
	int comres = 0;
	unsigned char data1 = 0;
	unsigned char data2 = 0;

	if ((11 < InterruptType) && (InterruptType < 16)) {
		switch (InterruptType) {
		case 12:
			/* slow/no motion X Interrupt  */
			comres = bma25x_smbus_read_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_X_INT__REG, &data1);
			data1 = BMA25X_SET_BITSLICE(data1,
				BMA25X_INT_SLO_NO_MOT_EN_X_INT, value);
			comres = bma25x_smbus_write_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_X_INT__REG, &data1);
			break;
		case 13:
			/* slow/no motion Y Interrupt  */
			comres = bma25x_smbus_read_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_Y_INT__REG, &data1);
			data1 = BMA25X_SET_BITSLICE(data1,
				BMA25X_INT_SLO_NO_MOT_EN_Y_INT, value);
			comres = bma25x_smbus_write_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_Y_INT__REG, &data1);
			break;
		case 14:
			/* slow/no motion Z Interrupt  */
			comres = bma25x_smbus_read_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_Z_INT__REG, &data1);
			data1 = BMA25X_SET_BITSLICE(data1,
				BMA25X_INT_SLO_NO_MOT_EN_Z_INT, value);
			comres = bma25x_smbus_write_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_Z_INT__REG, &data1);
			break;
		case 15:
			/* slow / no motion Interrupt select */
			comres = bma25x_smbus_read_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_SEL_INT__REG, &data1);
			data1 = BMA25X_SET_BITSLICE(data1,
				BMA25X_INT_SLO_NO_MOT_EN_SEL_INT, value);
			comres = bma25x_smbus_write_byte(client,
				BMA25X_INT_SLO_NO_MOT_EN_SEL_INT__REG, &data1);
		}

	return comres;
	}

	comres = bma25x_smbus_read_byte(client, BMA25X_INT_ENABLE1_REG, &data1);
	comres = bma25x_smbus_read_byte(client, BMA25X_INT_ENABLE2_REG, &data2);

	value = value & 1;
	switch (InterruptType) {
	case 0:
		/* Low G Interrupt  */
		data2 = BMA25X_SET_BITSLICE(data2, BMA25X_EN_LOWG_INT,
				value);
		break;
	case 1:
		/* High G X Interrupt */
		data2 = BMA25X_SET_BITSLICE(data2, BMA25X_EN_HIGHG_X_INT,
				value);
		break;
	case 2:
		/* High G Y Interrupt */
		data2 = BMA25X_SET_BITSLICE(data2, BMA25X_EN_HIGHG_Y_INT,
				value);
		break;
	case 3:
		/* High G Z Interrupt */
		data2 = BMA25X_SET_BITSLICE(data2, BMA25X_EN_HIGHG_Z_INT,
				value);
		break;
	case 4:
		/* New Data Interrupt  */
		data2 = BMA25X_SET_BITSLICE(data2, BMA25X_EN_NEW_DATA_INT,
				value);
		break;
	case 5:
		/* Slope X Interrupt */
		data1 = BMA25X_SET_BITSLICE(data1, BMA25X_EN_SLOPE_X_INT,
				value);
		break;
	case 6:
		/* Slope Y Interrupt */
		data1 = BMA25X_SET_BITSLICE(data1, BMA25X_EN_SLOPE_Y_INT,
				value);
		break;

	case 7:
		/* Slope Z Interrupt */
		data1 = BMA25X_SET_BITSLICE(data1, BMA25X_EN_SLOPE_Z_INT,
				value);
		break;
	case 8:
		/* Single Tap Interrupt */
		data1 = BMA25X_SET_BITSLICE(data1, BMA25X_EN_SINGLE_TAP_INT,
				value);
		break;
	case 9:
		/* Double Tap Interrupt */
		data1 = BMA25X_SET_BITSLICE(data1, BMA25X_EN_DOUBLE_TAP_INT,
				value);
		break;
	case 10:
		/* Orient Interrupt  */
		data1 = BMA25X_SET_BITSLICE(data1, BMA25X_EN_ORIENT_INT, value);
		break;

	case 11:
		/* Flat Interrupt */
		data1 = BMA25X_SET_BITSLICE(data1, BMA25X_EN_FLAT_INT, value);
		break;
	default:
		break;
	}
	comres = bma25x_smbus_write_byte(client, BMA25X_INT_ENABLE1_REG,
			&data1);
	comres = bma25x_smbus_write_byte(client, BMA25X_INT_ENABLE2_REG,
			&data2);

	return comres;
}

static int bma25x_set_Int_Mode(struct i2c_client *client, unsigned char Mode)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bma25x_smbus_read_byte(client,
			BMA25X_INT_MODE_SEL__REG, &data);
	data = BMA25X_SET_BITSLICE(data, BMA25X_INT_MODE_SEL, Mode);
	comres = bma25x_smbus_write_byte(client,
			BMA25X_INT_MODE_SEL__REG, &data);

	return comres;
}
static int bma25x_set_int1_pad_sel(struct i2c_client *client,
		unsigned char int1sel)
{
	int comres = 0;
	unsigned char data = 0;
	unsigned char state = 0;
	state = 0x01;

	switch (int1sel) {
	case 0:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_LOWG__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_LOWG,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_LOWG__REG, &data);
		break;
	case 1:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_HIGHG__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_HIGHG,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_HIGHG__REG, &data);
		break;
	case 2:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_SLOPE__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_SLOPE,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_SLOPE__REG, &data);
		break;
	case 3:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_DB_TAP__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_DB_TAP,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_DB_TAP__REG, &data);
		break;
	case 4:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_SNG_TAP__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_SNG_TAP,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_SNG_TAP__REG, &data);
		break;
	case 5:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_ORIENT__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_ORIENT,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_ORIENT__REG, &data);
		break;
	case 6:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_FLAT__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_FLAT,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_FLAT__REG, &data);
		break;
	case 7:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT1_PAD_SLO_NO_MOT__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT1_PAD_SLO_NO_MOT,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT1_PAD_SLO_NO_MOT__REG, &data);
		break;
	default:
		break;
	}

	return comres;
}

#endif//CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

#ifdef BMA25X_MAP_FLAT_TO_INT2
static int bma25x_set_int2_pad_sel(struct i2c_client *client,
		unsigned char int2sel)
{
	int comres = 0;
	unsigned char data = 0;
	unsigned char state = 0;

	state = 0x01;

	switch (int2sel) {
	case 0:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_LOWG__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_LOWG,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_LOWG__REG, &data);
		break;
	case 1:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_HIGHG__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_HIGHG,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_HIGHG__REG, &data);
		break;
	case 2:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_SLOPE__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_SLOPE,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_SLOPE__REG, &data);
		break;
	case 3:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_DB_TAP__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_DB_TAP,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_DB_TAP__REG, &data);
		break;
	case 4:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_SNG_TAP__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_SNG_TAP,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_SNG_TAP__REG, &data);
		break;
	case 5:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_ORIENT__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_ORIENT,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_ORIENT__REG, &data);
		break;
	case 6:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_FLAT__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_FLAT,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_FLAT__REG, &data);
		break;
	case 7:
		comres = bma25x_smbus_read_byte(client,
				BMA25X_EN_INT2_PAD_SLO_NO_MOT__REG, &data);
		data = BMA25X_SET_BITSLICE(data, BMA25X_EN_INT2_PAD_SLO_NO_MOT,
				state);
		comres = bma25x_smbus_write_byte(client,
				BMA25X_EN_INT2_PAD_SLO_NO_MOT__REG, &data);
		break;
	default:
		break;
	}

	return comres;
}
#endif

/*----------------------------------------------------------------------------*/

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	char strbuf[BMA2x2_BUFSIZE];
	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	BMA2x2_ReadChipInfo(client, strbuf, BMA2x2_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}


/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/
static ssize_t show_cpsopmode_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_mode(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/
static ssize_t store_cpsopmode_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;
	if (data == BMA2x2_MODE_NORMAL)
		BMA2x2_SetPowerMode(bma2x2_i2c_client, true, BMA25X_ACC);
	else if (data == BMA2x2_MODE_SUSPEND)
		BMA2x2_SetPowerMode(bma2x2_i2c_client, false, BMA25X_ACC);
	else
		GSE_ERR("invalid content: '%s'\n", buf);


	return count;
}

/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
static ssize_t show_cpsrange_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_range(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
static ssize_t store_cpsrange_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma2x2_set_range(bma2x2_i2c_client, (unsigned char) data) < 0)
		GSE_ERR("invalid content: '%s'\n", buf);


	return count;
}
/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
static ssize_t show_cpsbandwidth_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_bandwidth(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
static ssize_t store_cpsbandwidth_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma2x2_set_bandwidth(bma2x2_i2c_client, (unsigned char) data) < 0)
		GSE_ERR("invalid content: '%s'\n", buf);


	return count;
}

/*----------------------------------------------------------------------------*/
/*
g sensor data for compass tilt compensation
*/
static ssize_t show_cpsdata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	char strbuf[BMA2x2_BUFSIZE];
	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMA2x2_CompassReadData(client, strbuf, BMA2x2_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	char strbuf[BMA2x2_BUFSIZE];
	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMA2x2_ReadSensorData(client, strbuf, BMA2x2_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	struct bma2x2_i2c_data *obj;
	int err, len = 0, mul;
	int tmp[BMA2x2_AXES_NUM];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	err = BMA2x2_ReadOffset(client, obj->offset);
	if (err)
		return -EINVAL;

	err = BMA2x2_ReadCalibration(client, tmp);
	if (err)
		return -EINVAL;
	else {
		mul = obj->reso->sensitivity
		 / bma2x2_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len,
		 "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
		 mul,
		 obj->offset[BMA2x2_AXIS_X],
		 obj->offset[BMA2x2_AXIS_Y],
		 obj->offset[BMA2x2_AXIS_Z],
		 obj->offset[BMA2x2_AXIS_X],
		 obj->offset[BMA2x2_AXIS_Y],
		 obj->offset[BMA2x2_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len,
		 "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		 obj->cali_sw[BMA2x2_AXIS_X],
		 obj->cali_sw[BMA2x2_AXIS_Y],
		 obj->cali_sw[BMA2x2_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len,
		 "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
		 obj->offset[BMA2x2_AXIS_X]*mul + obj->cali_sw[BMA2x2_AXIS_X],
		 obj->offset[BMA2x2_AXIS_Y]*mul + obj->cali_sw[BMA2x2_AXIS_Y],
		 obj->offset[BMA2x2_AXIS_Z]*mul + obj->cali_sw[BMA2x2_AXIS_Z],
		 tmp[BMA2x2_AXIS_X], tmp[BMA2x2_AXIS_Y], tmp[BMA2x2_AXIS_Z]);
		return len;
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	struct i2c_client *client = bma2x2_i2c_client;
	int err, x, y, z;
	int dat[BMA2x2_AXES_NUM];

	if (!strncmp(buf, "rst", 3)) {
		err = BMA2x2_ResetCalibration(client);
		if (err)
			GSE_ERR("reset offset err = %d\n", err);

	} else if (3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z)) {
		dat[BMA2x2_AXIS_X] = x;
		dat[BMA2x2_AXIS_Y] = y;
		dat[BMA2x2_AXIS_Z] = z;
		err = BMA2x2_WriteCalibration(client, dat);
		if (err)
			GSE_ERR("write calibration err = %d\n", err);

	} else
		GSE_ERR("invalid format\n");

	return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMA2x2_LOWPASS
	int X = BMA2x2_AXIS_X;
	struct i2c_client *client = bma2x2_i2c_client;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++) {
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][X],
			 obj->fir.raw[idx][BMA2x2_AXIS_Y],
			 obj->fir.raw[idx][BMA2x2_AXIS_Z]);
		}

		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[BMA2x2_AXIS_X],
		 obj->fir.sum[BMA2x2_AXIS_Y],
		 obj->fir.sum[BMA2x2_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[X]/len,
		 obj->fir.sum[BMA2x2_AXIS_Y]/len,
		 obj->fir.sum[BMA2x2_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
#ifdef CONFIG_BMA2x2_LOWPASS
	struct i2c_client *client = bma2x2_i2c_client;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if (1 != sscanf(buf, "%11d", &firlen)) {
		GSE_ERR("invallid format\n");
	} else if (firlen > C_MAX_FIR_LENGTH) {
		GSE_ERR("exceeds maximum filter length\n");
	} else {
		atomic_set(&obj->firlen, firlen);
		if (NULL == firlen) {
			atomic_set(&obj->fir_en, 0);
		} else {
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri,
	 char *buf)
{
	ssize_t res;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	if (1 == sscanf(buf, "0x%11x", &trace))
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s'\n", buf);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf+len, PAGE_SIZE-len,
		 "CUST: %d %d (%d %d)\n",
		 obj->hw->i2c_num,
		 obj->hw->direction,
		 obj->hw->power_id,
		 obj->hw->power_vol);
	} else
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{
	if (sensor_power)
		GSE_LOG("G sensor is in work mode, sensor_power = %d\n",
		 sensor_power);
	else
		GSE_LOG("G sensor is in standby mode, sensor_power = %d\n",
		 sensor_power);

	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_fifo_mode_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_fifo_mode(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
static ssize_t store_fifo_mode_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma2x2_set_fifo_mode(bma2x2_i2c_client, (unsigned char) data) < 0)
		GSE_ERR("invalid content: '%s'\n", buf);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_fifo_framecount_value(struct device_driver *ddri,
	 char *buf)
{
	unsigned char data;

	if (bma2x2_get_fifo_framecount(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
static ssize_t store_fifo_framecount_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	mutex_lock(&obj->lock);
	obj->fifo_count = (unsigned char)data;
	mutex_unlock(&obj->lock);
	return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_fifo_data_out_frame_value(struct device_driver *ddri,
	 char *buf)
{
	unsigned char fifo_frame_cnt = 0;
	int err = 0;
	int read_cnt = 0;
	int num = 0;
	static char tmp_buf[32*6] = {0};
	char *pBuf = NULL;
	unsigned char f_len = 6;/* FIXME: ONLY USE 3-AXIS */
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(bma2x2_i2c_client);
	if (buf == NULL)
		return -EINVAL;


	err = bma2x2_get_fifo_framecount(bma2x2_i2c_client, &fifo_frame_cnt);
	if (err < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");


	if (fifo_frame_cnt == 0) {
		GSE_ERR("fifo frame count is 0!!!");
		return 0;
	}
#ifdef READFIFOCOUNT
	read_cnt = fifo_frame_cnt;
	pBuf = &tmp_buf[0];
	mutex_lock(&obj->lock);
	while (read_cnt > 0) {
		if (bma_i2c_read_block(bma2x2_i2c_client,
		 BMA2x2_FIFO_DATA_OUTPUT_REG,
		 pBuf,
		 f_len) < 0) {
			GSE_ERR("[a]fatal error\n");
			num =
			 snprintf(buf, PAGE_SIZE, "Read byte block error\n");
			 return num;
		}
		pBuf += f_len;
		read_cnt--;
		GSE_ERR("fifo_frame_cnt %d, f_len %d", fifo_frame_cnt, f_len);
	}
	mutex_unlock(&obj->lock);
	memcpy(buf, tmp_buf, fifo_frame_cnt * f_len);
#else
	if (bma_i2c_dma_read(bma2x2_i2c_client,
	 BMA2x2_FIFO_DATA_OUTPUT_REG, buf,
	 fifo_frame_cnt * f_len) < 0) {
		GSE_ERR("[a]fatal error\n");
		return snprintf(buf, PAGE_SIZE, "Read byte block error\n");
	}
#endif
	return fifo_frame_cnt * f_len;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_registers(struct device_driver *ddri, char *buf)
{
	unsigned char temp = 0;
	int count = 0;
	int num = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (buf == NULL)
		return -EINVAL;
	if (bma2x2_i2c_client == NULL)
		return -EINVAL;
	mutex_lock(&obj->lock);
	while (count <= MAX_REGISTER_MAP) {
		temp = 0;
		if (bma_i2c_read_block(bma2x2_i2c_client,
			count,
			&temp,
			1) < 0) {
			num =
			 snprintf(buf, PAGE_SIZE, "Read byte block error\n");
			return num;
		} else {
			num +=
			 snprintf(buf + num, PAGE_SIZE, "0x%x\n", temp);
		}
		count++;
	}
	mutex_unlock(&obj->lock);
	return num;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_softreset(struct device_driver *ddri, char *buf)
{
	int ret = 0;
	int num = 0;
	if (buf == NULL)
		return -EINVAL;
	if (bma2x2_i2c_client == NULL)
		return -EINVAL;
	ret = bma2x2_softreset();
	if (ret < 0)
		num =
		snprintf(buf, PAGE_SIZE, "Write byte block error\n");
	else
		num =
		snprintf(buf, PAGE_SIZE, "softreset success\n");
	ret = bma2x2_init_client(bma2x2_i2c_client, 0);
	if (ret < 0)
		GSE_ERR("bma2x2_init_client failed\n");
	else
		GSE_ERR("bma2x2_init_client succeed\n");
	ret = BMA2x2_SetPowerMode(bma2x2_i2c_client, true, BMA25X_ACC);
	if (ret < 0)
		GSE_ERR("BMA2x2_SetPowerMode failed\n");
	else
		GSE_ERR("BMA2x2_SetPowerMode succeed\n");
	return num;

}

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static int read_flag = 0;
static u8 read_reg;
static ssize_t bma25x_reg_dump_show(struct device_driver *ddri, char *buf)
{
	u8 data[20];
	char *p = buf;

	if (read_flag) {
		read_flag = 0;
		bma_i2c_read_block(bma2x2_client, read_reg, data, 1);
		p += snprintf(p, PAGE_SIZE, "%02x\n", data[0]);
		return (p-buf);
	}

	bma_i2c_read_block(bma2x2_client, 0x09, data, 4);
	p += snprintf(p, PAGE_SIZE, "INT DATA(09~0c)=%02x,%02x,%02x,%02x\n",
			data[0], data[1], data[2], data[3]);

	bma_i2c_read_block(bma2x2_client, 0x16, data, 3);
	p += snprintf(p, PAGE_SIZE, "INT EN(16~18)=%02x,%02x,%02x\n",
			data[0], data[1], data[2]);

	bma_i2c_read_block(bma2x2_client, 0x19, data, 3);
	p += snprintf(p, PAGE_SIZE, "INT MAP(19~1b)=%02x,%02x,%02x\n",
			data[0], data[1], data[2]);

	bma_i2c_read_block(bma2x2_client, 0x1e, data, 1);
	p += snprintf(p, PAGE_SIZE, "INT SRC(1e)=%02x\n",
			data[0]);

	bma_i2c_read_block(bma2x2_client, 0x20, data, 1);
	p += snprintf(p, PAGE_SIZE, "INT OUT CTRL(20)=%02x\n",
			data[0]);

	bma_i2c_read_block(bma2x2_client, 0x21, data, 1);
	p += snprintf(p, PAGE_SIZE, "INT LATCH(21)=%02x\n",
			data[0]);

	bma_i2c_read_block(bma2x2_client, 0x27, data, 3);
	p += snprintf(p, PAGE_SIZE, "SLO NOMOT SET(27~29)=%02x,%02x,%02x\n",
			data[0], data[1], data[2]);

	bma_i2c_read_block(bma2x2_client, 0x2e, data, 2);
	p += snprintf(p, PAGE_SIZE, "FLAT SET(2E~2F)=%02x,%02x\n",
			data[0], data[1]);

	return (p-buf);
}

static ssize_t bma25x_reg_dump_store(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned int val, reg, opt;

	if (sscanf(buf, "%x,%x,%x", &reg, &val, &opt) == 3) {
		read_reg = *((u8 *)&reg);
		read_flag = 1;
	} else if (sscanf(buf, "%x,%x", &reg, &val) == 2) {
		GSE_ERR("%s,reg = 0x%02x, val = 0x%02x\n",
			__func__, *(u8 *)&reg, *(u8 *)&val);
		bma_i2c_write_block(bma2x2_client, *(u8 *)&reg,
			(u8 *)&val, 1);
	}

	return count;
}

static ssize_t bma25x_flatdown_show(struct device_driver *ddri, char *buf)
{
	int databuf = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (FLATDOWN_GESTURE == obj->flat_down_value)
		databuf = 1;
	else if (EXIT_FLATDOWN_GESTURE == obj->flat_down_value)
		databuf = 0;
	return snprintf(buf, PAGE_SIZE, "%d\n", databuf);
}

static ssize_t bma25x_flat_threshold_show(struct device_driver *ddri, char *buf)
{
	struct bma2x2_i2c_data *obj = obj_i2c_data;

	return snprintf(buf, PAGE_SIZE, "%d\n", obj->flat_threshold);
}

static ssize_t bma25x_flat_threshold_store(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct bma2x2_i2c_data *obj = obj_i2c_data;

	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;
	obj->flat_threshold = data;

	return count;
}

static ssize_t bma25x_int_mode_show(struct device_driver *ddri, char *buf)
{
	struct bma2x2_i2c_data *obj = obj_i2c_data;

	return snprintf(buf, PAGE_SIZE, "%d\n", obj->mEnabled);
}

static ssize_t bma25x_int_mode_store(struct device_driver *ddri,
		const char *buf, size_t count)
{
	unsigned long data = 0;
	int error = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;

	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;
	bma25x_set_en_sig_int_mode(obj, data);

	return count;
}
#endif

//tuwenzan@wind-mobi.com add at 20161128 begin
#ifdef BMA2XX_OFFSET_CALI
static ssize_t show_calibration_state(struct device_driver *ddri, char *buf)
{
	int len = 0;
	len = sprintf(buf,"%d",cali_flag);
	printk("tuwenzan cali_flag = %d\n",cali_flag);
	return len;
}

static ssize_t store_calibration_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	int ret = 0;
	ret = simple_strtoul(buf,0,10);
	if(1 == ret){
		ret = bma2xx_offset_fast_cali(0,0,1);
		printk("tuwenzan ret = %d\n",ret);
		if(0 == ret){
			cali_flag = 1;
			printk("tuwenzan cali_flag = %d,ret = %d",cali_flag,ret);
		}else
			printk("calibration fail\n");
	}
	return count;
}
#endif
//tuwenzan@wind-mobi.com add at 20161128 end
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(cpsdata, S_IWUSR | S_IRUGO, show_cpsdata_value, NULL);
static DRIVER_ATTR(cpsopmode, S_IWUSR | S_IRUGO, show_cpsopmode_value,
	 store_cpsopmode_value);
static DRIVER_ATTR(cpsrange, S_IWUSR | S_IRUGO, show_cpsrange_value,
	 store_cpsrange_value);
static DRIVER_ATTR(cpsbandwidth, S_IWUSR | S_IRUGO, show_cpsbandwidth_value,
	 store_cpsbandwidth_value);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO, show_firlen_value,
	 store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value,
	 store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(powerstatus, S_IRUGO, show_power_status_value, NULL);
static DRIVER_ATTR(fifo_mode, S_IWUSR | S_IRUGO, show_fifo_mode_value,
	 store_fifo_mode_value);
static DRIVER_ATTR(fifo_framecount, S_IWUSR |
	 S_IRUGO, show_fifo_framecount_value,
	 store_fifo_framecount_value);
static DRIVER_ATTR(fifo_data_frame, S_IRUGO, show_fifo_data_out_frame_value,
	 NULL);
static DRIVER_ATTR(dump_registers, S_IRUGO, show_registers, NULL);
static DRIVER_ATTR(softreset, S_IRUGO, show_softreset, NULL);

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static DRIVER_ATTR(reg, S_IRUGO | S_IWUSR, bma25x_reg_dump_show, bma25x_reg_dump_store );
static DRIVER_ATTR(flatdown, S_IRUGO, bma25x_flatdown_show, NULL);
static DRIVER_ATTR(flat_threshold, S_IWUSR | S_IRUGO, bma25x_flat_threshold_show,
	 bma25x_flat_threshold_store);
static DRIVER_ATTR(int_mode, S_IWUSR | S_IRUGO, bma25x_int_mode_show,
	 bma25x_int_mode_store);
#endif
//tuwenzan@wind-mobi.com add at 20161128 begin
#ifdef BMA2XX_OFFSET_CALI
static DRIVER_ATTR(start_fast_calibration, 0644, show_calibration_state,store_calibration_value);
#endif
/*----------------------------------------------------------------------------*/
static struct driver_attribute *bma2x2_attr_list[] = {
	/*chip information*/
	 &driver_attr_chipinfo,
	/*dump sensor data*/
	 &driver_attr_sensordata,
	/*show calibration data*/
	 &driver_attr_cali,
	/*filter length: 0: disable, others: enable*/
	 &driver_attr_firlen,
	/*trace log*/
	 &driver_attr_trace,
	 &driver_attr_status,
	 &driver_attr_powerstatus,
	/*g sensor data for compass tilt compensation*/
	 &driver_attr_cpsdata,
	/*g sensor opmode for compass tilt compensation*/
	 &driver_attr_cpsopmode,
	/*g sensor range for compass tilt compensation*/
	 &driver_attr_cpsrange,
	/*g sensor bandwidth for compass tilt compensation*/
	 &driver_attr_cpsbandwidth,
	 &driver_attr_fifo_mode,
	 &driver_attr_fifo_framecount,
	 &driver_attr_fifo_data_frame,
	 &driver_attr_dump_registers,
	 &driver_attr_softreset,
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	 &driver_attr_reg,
	 &driver_attr_flatdown,
	 &driver_attr_flat_threshold,
	 &driver_attr_int_mode,
#endif
#ifdef BMA2XX_OFFSET_CALI
	 &driver_attr_start_fast_calibration,
#endif
};
//tuwenzan@wind-mobi.com add at 20161128 end
/*----------------------------------------------------------------------------*/
static int bma2x2_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma2x2_attr_list)/sizeof(bma2x2_attr_list[0]));
	if (driver == NULL)
		return -EINVAL;


	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, bma2x2_attr_list[idx]);
		if (err) {
			GSE_ERR("driver_create_file (%s) = %d\n",
			 bma2x2_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma2x2_attr_list)/sizeof(bma2x2_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;


	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, bma2x2_attr_list[idx]);


	return err;
}

/******************************************************************************
* Function Configuration
******************************************************************************/
static int bma2x2_open(struct inode *inode, struct file *file)
{
	file->private_data = bma2x2_i2c_client;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int bma2x2_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static long bma2x2_unlocked_ioctl(struct file *file,
	 unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct bma2x2_i2c_data *obj =
		 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);
	char strbuf[BMA2x2_BUFSIZE];
	void __user *data;
	struct SENSOR_DATA sensor_data;
	long err = 0;
	int cali[3];

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg,
		 _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg,
		 _IOC_SIZE(cmd));


	if (err) {
		GSE_ERR("access error: %08X, (%2d, %2d)\n",
		 cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		bma2x2_init_client(client, 0);
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		BMA2x2_ReadChipInfo(client, strbuf, BMA2x2_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_SENSORDATA:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		BMA2x2_ReadSensorData(client, strbuf, BMA2x2_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_GAIN:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_to_user(data, &gsensor_gain,
		 sizeof(struct GSENSOR_VECTOR3D))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_RAW_DATA:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		BMA2x2_ReadRawData(client, strbuf);
		if (copy_to_user(data, &strbuf, strlen(strbuf)+1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_SET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&sensor_data, data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		if (atomic_read(&obj->suspend)) {
			GSE_ERR("Perform calibration in suspend state!!\n");
			err = -EINVAL;
		} else {
			cali[BMA2x2_AXIS_X] =
			 sensor_data.x * obj->reso->sensitivity /
			 GRAVITY_EARTH_1000;
			cali[BMA2x2_AXIS_Y] =
			 sensor_data.y * obj->reso->sensitivity /
			 GRAVITY_EARTH_1000;
			cali[BMA2x2_AXIS_Z] =
			 sensor_data.z * obj->reso->sensitivity /
			 GRAVITY_EARTH_1000;
			err = BMA2x2_WriteCalibration(client, cali);
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		err = BMA2x2_ResetCalibration(client);
		break;

	case GSENSOR_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		err = BMA2x2_ReadCalibration(client, cali);
		if (err)
			break;


		sensor_data.x =
		 cali[BMA2x2_AXIS_X] * GRAVITY_EARTH_1000 /
		 obj->reso->sensitivity;
		sensor_data.y =
		 cali[BMA2x2_AXIS_Y] * GRAVITY_EARTH_1000 /
		 obj->reso->sensitivity;
		sensor_data.z =
		 cali[BMA2x2_AXIS_Z] * GRAVITY_EARTH_1000 /
		 obj->reso->sensitivity;
		if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		break;
	default:
		GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;
	}

	return err;
}

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static irqreturn_t gsensor_eint_func1(int irq, void *desc)
{
#ifdef DEBUG
	int64_t ns;
	struct timespec time;

	time.tv_sec = time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	ns = time.tv_sec * 1000000000LL + time.tv_nsec;
	ISR_INFO(&obj_i2c_data->client->dev, "tick0:%lx", (long unsigned int)ns);
#endif
	schedule_work((struct work_struct *)desc);
	return IRQ_HANDLED;
}

static irqreturn_t gsensor_eint_func2(int irq, void *desc)
{
	struct bma2x2_i2c_data *data = obj_i2c_data;
#ifdef DEBUG
	int64_t ns;
	struct timespec time;

	time.tv_sec = time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	ns = time.tv_sec * 1000000000LL + time.tv_nsec;
	ISR_INFO(&obj_i2c_data->client->dev, "tick0:%lx", (long unsigned int)ns);
#endif
	if (data == NULL)
		return IRQ_HANDLED;
	if (data->client == NULL)
		return IRQ_HANDLED;

	queue_delayed_work(data->data_wq,
		&data->flat_work, msecs_to_jiffies(50));
	wake_lock_timeout(&data->aod_wakelock, msecs_to_jiffies(60));

	return IRQ_HANDLED;
}

int bma253_setup_int1(struct i2c_client *client)
{
	int ret;
	u32 debounce[2] = {0, 0};	
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_cfg;
	struct device_node *node = NULL;

	GSE_FUN();

	/* parse irq */
	node = of_find_compatible_node(NULL, NULL, "mediatek, gse_1-eint");
	if (node) {
		/* parse irq num */
		obj_i2c_data->IRQ1 = irq_of_parse_and_map(node, 0);
		if (!obj_i2c_data->IRQ1) {
			GSE_ERR("can't parse irq num for gse_1-eint\n");
			return -EINVAL;
		}
		GSE_LOG("gse_1-eint = %d\n", obj_i2c_data->IRQ1);

		/* parse debounce settings */
		of_property_read_u32_array(node, "debounce", debounce, ARRAY_SIZE(debounce));
		gpio_request(debounce[0], "gse_1-eint");
		gpio_set_debounce(debounce[0], debounce[1]);
		GSE_LOG("gse_1-eint:gpio = %d, debounce = %d\n", debounce[0], debounce[1]);
	} else {
		GSE_ERR("can't find node for gse_1-eint\n");
		return -EINVAL;
	}

	/* parse pinctrl */
	gsensorPltFmDev = get_gsensor_platformdev();
	pinctrl = devm_pinctrl_get(&gsensorPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		GSE_ERR("can't find pinctrl for gse_1-eint\n");
		return ret;
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_state_int1");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		GSE_ERR("can't find pin_cfg for gse_1-eint\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);

	/* request irq for gse_1-eint */
	if (request_irq(obj_i2c_data->IRQ1, gsensor_eint_func1, IRQF_TRIGGER_RISING, "gse_1-eint", &obj_i2c_data->int1_irq_work)) {
		GSE_ERR("request irq for gse_1-eint\n");
		return -EINVAL;
	}

	/* enable irq */
	//enable_irq(obj_i2c_data->IRQ1);

	return 0;
}

int bma253_setup_int2(struct i2c_client *client)
{
	int ret;
	u32 debounce[2] = {0, 0};	
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_cfg;
	struct device_node *node = NULL;

	GSE_FUN();

	/* parse irq */
	node = of_find_compatible_node(NULL, NULL, "mediatek, gse_2-eint");
	if (node) {
		/* parse irq num */
		obj_i2c_data->IRQ2 = irq_of_parse_and_map(node, 0);
		if (!obj_i2c_data->IRQ2) {
			GSE_ERR("can't parse irq num for gse_2-eint\n");
			return -EINVAL;
		}
		GSE_LOG("gse_2-eint = %d\n", obj_i2c_data->IRQ2);

		/* parse debounce settings */
		of_property_read_u32_array(node, "debounce", debounce, ARRAY_SIZE(debounce));
		gpio_request(debounce[0], "gse_2-eint");
		gpio_set_debounce(debounce[0], debounce[1]);
		GSE_LOG("gse_2-eint:gpio = %d, debounce = %d\n", debounce[0], debounce[1]);
	} else {
		GSE_ERR("can't find node for gse_2-eint\n");
		return -EINVAL;
	}

	/* parse pinctrl */
	gsensorPltFmDev = get_gsensor_platformdev();
	pinctrl = devm_pinctrl_get(&gsensorPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		GSE_ERR("can't find pinctrl for gse_2-eint\n");
		return ret;
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_state_int2");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		GSE_ERR("can't find pin_cfg for gse_2-eint\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);

#ifdef BMA25X_MAP_FLAT_TO_INT2
	/* request irq for gse_2-eint */
	if (request_irq(obj_i2c_data->IRQ2, gsensor_eint_func2, IRQF_TRIGGER_RISING, "gse_2-eint", &obj_i2c_data->int2_irq_work)) {
		GSE_ERR("request irq for gse_2-eint\n");
		return -EINVAL;
	}

	/* enable irq */
	enable_irq(obj_i2c_data->IRQ2);
#else
	disable_irq(obj_i2c_data->IRQ2);
#endif
	return 0;
}
#endif
/*************/

/*----------------------------------------------------------------------------*/
static const struct file_operations bma2x2_fops = {
	.open = bma2x2_open,
	.release = bma2x2_release,
	.unlocked_ioctl = bma2x2_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice bma2x2_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &bma2x2_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int bma2x2_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	GSE_FUN();

	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);
		err = BMA2x2_SetPowerMode(obj->client, false, BMA25X_ACC);
		if (err) {
			GSE_ERR("write power control fail!!\n");
			return err;
		}
		BMA2x2_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_resume(struct i2c_client *client)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	GSE_FUN();

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	BMA2x2_power(obj->hw, 1);
	err = bma2x2_init_client(client, 0);
	if (err) {
		GSE_ERR("initialize client fail!!\n");
		return err;
	}

	atomic_set(&obj->suspend, 0);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void bma2x2_early_suspend(struct early_suspend *h)
{
	struct bma2x2_i2c_data *obj =
	 container_of(h, struct bma2x2_i2c_data, early_drv);
	int err;

	GSE_FUN();

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1);
	err = BMA2x2_SetPowerMode(obj->client, false, BMA25X_ACC);

	if (err) {
		GSE_ERR("write power control fail!!\n");
		return;
	}

	BMA2x2_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void bma2x2_late_resume(struct early_suspend *h)
{
	struct bma2x2_i2c_data *obj =
	 container_of(h, struct bma2x2_i2c_data, early_drv);
	int err;

	GSE_FUN();

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}

	BMA2x2_power(obj->hw, 1);
	err = bma2x2_init_client(obj->client, 0);
	if (err) {
		GSE_ERR("initialize client fail!!\n");
		return;
	}
	atomic_set(&obj->suspend, 0);
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/

/* if use  this typ of enable ,*/
/*Gsensor should report inputEvent(x, y, z ,stats, div) to HAL*/
static int bma2x2_open_report_data(int open)
{
/*should queuq work to report event if  is_report_input_direct=true*/
	return 0;
}

/* if use  this typ of enable ,*/
/*Gsensor only enabled but not report inputEvent to HAL*/
static int bma2x2_enable_nodata(int en)
{
	int err = 0;

	if (((en == 0) && (sensor_power == false))
	 || ((en == 1) && (sensor_power == true))) {
		GSE_LOG("Gsensor device have updated!\n");
	} else {
		err = BMA2x2_SetPowerMode(obj_i2c_data->client, !sensor_power, BMA25X_ACC);
	}

	return err;
}

static int bma2x2_set_delay(u64 ns)
{
	int err = 0;

	int value = (int)ns / 1000 / 1000;


	if (err != BMA2x2_SUCCESS)
		GSE_ERR("Set delay parameter error!\n");


	if (value >= 50)
		atomic_set(&obj_i2c_data->filter, 0);
	else {
#if defined(CONFIG_BMA2x2_LOWPASS)
	obj_i2c_data->fir.num = 0;
	obj_i2c_data->fir.idx = 0;
	obj_i2c_data->fir.sum[BMA2x2_AXIS_X] = 0;
	obj_i2c_data->fir.sum[BMA2x2_AXIS_Y] = 0;
	obj_i2c_data->fir.sum[BMA2x2_AXIS_Z] = 0;
	atomic_set(&obj_i2c_data->filter, 1);
#endif
	}

	return 0;
}

static int bma2x2_get_data(int *x , int *y, int *z, int *status)
{
	char buff[BMA2x2_BUFSIZE];
	/* use acc raw data for gsensor */
	BMA2x2_ReadSensorData(obj_i2c_data->client, buff, BMA2x2_BUFSIZE);

	sscanf(buff, "%11x %11x %11x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}


/*----------------------------------------------------------------------------*/
static int bma2x2_i2c_probe(struct i2c_client *client,
	 const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct bma2x2_i2c_data *obj;
	struct acc_control_path ctl = {0};
	struct acc_data_path data = {0};
	int err = 0;
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	struct input_dev *dev_interrupt;
#endif

	GSE_FUN();
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct bma2x2_i2c_data));

	obj->hw = hw;
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
	bma2x2_client = client; //tuwenzan@wind-mobi.com add at 20161128
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);
#ifdef DMAREAD
	/*allocate DMA buffer*/
	//twz modify I2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, DMA_BUFFER_SIZE,&I2CDMABuf_pa, GFP_KERNEL);
    client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	I2CDMABuf_va = (u8 *)dma_zalloc_coherent(&client->dev, DMA_BUFFER_SIZE,
	 &I2CDMABuf_pa, GFP_KERNEL);
	if (I2CDMABuf_va == NULL) {
		err = -ENOMEM;
		GSE_ERR("Allocate DMA I2C Buffer failed! error = %d\n", err);
	}
#endif
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	mutex_init(&obj->lock);

	mutex_init(&sensor_data_mutex);
	mutex_init(&obj->mode_mutex);

#ifdef CONFIG_BMA2x2_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);


	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);


#endif

	bma2x2_i2c_client = new_client;
	err = bma2x2_init_client(new_client, 1);
	if (err)
		goto exit_init_failed;

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	dev_interrupt = input_allocate_device();
	if (!dev_interrupt) {
		kfree(obj);
		return -ENOMEM;
	}

	/* all interrupt generated events are moved to interrupt input devices*/
	dev_interrupt->name = "bma25x-interrupt";
	dev_interrupt->id.bustype = BUS_I2C;
	input_set_abs_params(dev_interrupt, ABS_X, ABSMIN, ABSMAX, 0, 0);
	input_set_abs_params(dev_interrupt, ABS_Y, ABSMIN, ABSMAX, 0, 0);
	input_set_abs_params(dev_interrupt, ABS_Z, ABSMIN, ABSMAX, 0, 0);
	input_set_capability(dev_interrupt, EV_REL,
		SLOW_NO_MOTION_INTERRUPT);
	input_set_capability(dev_interrupt, EV_ABS,
		ORIENT_INTERRUPT);
	input_set_capability(dev_interrupt, EV_REL,
		FLAT_INTERRUPT);
	input_set_capability(dev_interrupt, EV_REL,
		REL_INT_FLUSH);
	input_set_drvdata(dev_interrupt, obj);
	err = input_register_device(dev_interrupt);
	if (err < 0)
		goto exit_register_input_device_interrupt_failed;

	obj->dev_interrupt = dev_interrupt;
	obj->flat_threshold = 962*GRAVITY_EARTH_1000/obj->reso->sensitivity;
	obj->aod_flag = 0;
	obj->flat_up_value = 0;
	obj->flat_down_value = 0;
	obj->mEnabled = 0;
	wake_lock_init(&obj->aod_wakelock, WAKE_LOCK_SUSPEND, "aod wakelock");
	atomic_set(&obj->flat_flag, 0);
	mutex_init(&obj->int_mode_mutex);

#ifdef BMA25X_MAP_FLAT_TO_INT2
	bma25x_set_int2_pad_sel(client, PAD_FLAT);
#else
	bma25x_set_int1_pad_sel(client, PAD_FLAT);
#endif
	bma25x_set_int1_pad_sel(client, PAD_SLOP);
	bma25x_set_int1_pad_sel(client, PAD_SLOW_NO_MOTION);
	bma25x_set_Int_Mode(client, 0x01);/*latch interrupt 250ms*/

	INIT_WORK(&obj->int1_irq_work, bma25x_int1_irq_work_func);
	INIT_DELAYED_WORK(&obj->flat_work, bma25x_flat_work_func);

	obj->data_wq = create_freezable_workqueue("bma25x_aod_work");
	if (!obj->data_wq) {
		dev_err(&client->dev, "Cannot get create workqueue!\n");
		goto exit_create_aod_workqueue_failed;
	}

	bma253_setup_int1(bma2x2_i2c_client); 
	bma253_setup_int2(bma2x2_i2c_client); 
#endif//CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

	err = misc_register(&bma2x2_device);
	if (err) {
		GSE_ERR("bma2x2_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err =
	 bma2x2_create_attr(&(bma2x2_init_info.platform_diver_addr->driver));
	if (err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data = bma2x2_open_report_data;
	ctl.enable_nodata = bma2x2_enable_nodata;
	ctl.set_delay  = bma2x2_set_delay;
	ctl.is_report_input_direct = false;

	err = acc_register_control_path(&ctl);
	if (err) {
		GSE_ERR("register acc control path err\n");
		goto exit_kfree;
	}

	data.get_data = bma2x2_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if (err) {
		GSE_ERR("register acc data path err\n");
		goto exit_kfree;
	}



#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = bma2x2_early_suspend,
	obj->early_drv.resume   = bma2x2_late_resume,
	register_early_suspend(&obj->early_drv);
#endif

#ifdef CONFIG_LCT_DEVINFO_SUPPORT
	devinfo_acceleration_regchar("BMA253","Bosch","1.0",DEVINFO_USED);
#endif

	bma2x2_init_flag = 0;
	GSE_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&bma2x2_device);
exit_misc_device_register_failed:

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
exit_create_aod_workqueue_failed:
	input_unregister_device(dev_interrupt);
exit_register_input_device_interrupt_failed:
	input_free_device(dev_interrupt);
#endif

exit_init_failed:
exit_kfree:
	kfree(obj);
exit:
	GSE_ERR("%s: err = %d\n", __func__, err);
	bma2x2_init_flag = -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int bma2x2_i2c_remove(struct i2c_client *client)
{
	int err = 0;
	err =
	 bma2x2_delete_attr(&(bma2x2_init_info.platform_diver_addr->driver));
	if (err)
		GSE_ERR("bma150_delete_attr fail: %d\n", err);

	err = misc_deregister(&bma2x2_device);
	if (err)
		GSE_ERR("misc_deregister fail: %d\n", err);

#ifdef DMAREAD
	if (I2CDMABuf_va != NULL) {
		dma_free_coherent(NULL,
		 DMA_BUFFER_SIZE, I2CDMABuf_va, I2CDMABuf_pa);
		I2CDMABuf_va = NULL;
	}
	I2CDMABuf_pa = 0;
#endif
	bma2x2_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_local_init(void)
{
	GSE_FUN();

	BMA2x2_power(hw, 1);
	if (i2c_add_driver(&bma2x2_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return BMA2x2_ERR_I2C;
	}
	if (-1 == bma2x2_init_flag)
		return BMA2x2_ERR_I2C;

	return 0;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_remove(void)
{
	GSE_FUN();
	BMA2x2_power(hw, 0);
	i2c_del_driver(&bma2x2_i2c_driver);
	return 0;
}

static struct acc_init_info bma2x2_init_info = {
	.name = "bma255",
	.init = bma2x2_local_init,
	.uninit = bma2x2_remove,
};

/*----------------------------------------------------------------------------*/
static int __init bma2x2_init(void)
{
	hw = get_accel_dts_func(COMPATIABLE_NAME, hw);

/*sku  A01,B01,D01,not support bmi160,   D01,E01 support bmi160  */
#ifdef CONFIG_LCT_BOOT_REASON

        sku = lct_get_sku()%5;

        if (3==sku || 4==sku){
           GSE_ERR(" bma253 not support ");
           return 0;
        }

#endif 
        
	GSE_FUN();
	#ifndef CONFIG_OF
	i2c_register_board_info(hw->i2c_num, &bma2x2_i2c_info, 1);
	#endif
	GSE_ERR("!!!!bma2x2_init i2c_register_board_info finishes\n");
	acc_driver_add(&bma2x2_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit bma2x2_exit(void)
{
	GSE_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(bma2x2_init);
module_exit(bma2x2_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BMA2x2 ACCELEROMETER SENSOR DRIVER");
MODULE_AUTHOR("contact@bosch-sensortec.com");
