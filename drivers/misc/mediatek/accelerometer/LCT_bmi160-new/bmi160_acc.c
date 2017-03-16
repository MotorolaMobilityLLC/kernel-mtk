/* BMI160_ACC motion sensor driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 *
 * VERSION: V1.5
 * HISTORY: V1.0 --- Driver creation
 *          V1.1 --- Add share I2C address function
 *          V1.2 --- Fix the bug that sometimes sensor is stuck after system resume.
 *          V1.3 --- Add FIFO interfaces.
 *          V1.4 --- Use basic i2c function to read fifo data instead of i2c DMA mode.
 *          V1.5 --- Add compensated value performed by MTK acceleration calibration process.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/module.h>

#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include <accel.h>
#include <linux/kernel.h>

#include <linux/of_irq.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE

#include <cust_acc.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include "bmi160_acc.h"
#include <hwmsen_helper.h>

#include <linux/gpio.h>

//extern struct hwm_sensor_data;

/*----------------------------------------------------------------------------*/
#define DEBUG 0
#define MISC_FOR_DAEMON
/*----------------------------------------------------------------------------*/
//#define CONFIG_BMI160_ACC_LOWPASS   /*apply low pass filter on output*/
#define SW_CALIBRATION
#define FIFO_READ_USE_DMA_MODE_I2C
//tad3sgh add ++
#define BMM050_DEFAULT_DELAY	100
#define CALIBRATION_DATA_SIZE	12

#ifdef CONFIG_LCT_GSENSOR_ADD_EINT  //add cly for gsensor eint
#define CONFIG_GSENSOR_IRQ_ENABLE 
#endif
/*----------------------------------------------------------------------------*/
/*
* Enable the driver to block e-compass daemon on suspend
*/
#define BMC050_BLOCK_DAEMON_ON_SUSPEND
#undef	BMC050_BLOCK_DAEMON_ON_SUSPEND
/*
* Enable gyroscope feature with BMC050
*/
#define BMC050_M4G
//#undef BMC050_M4G
/*
* Enable rotation vecter feature with BMC050
*/
#define BMC050_VRV
#undef BMC050_VRV

/*
* Enable virtual linear accelerometer feature with BMC050
*/
#define BMC050_VLA
#undef BMC050_VLA

/*
* Enable virtual gravity feature with BMC050
*/
#define BMC050_VG
#undef BMC050_VG

#ifdef BMC050_M4G
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_GFLAG			_IOR(MSENSOR, 0x30, short)
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_GDELAY			_IOR(MSENSOR, 0x31, int)
#endif //BMC050_M4G
#ifdef BMC050_VRV
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_VRVFLAG			_IOR(MSENSOR, 0x32, short)
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_VRVDELAY			_IOR(MSENSOR, 0x33, int)
#endif //BMC050_VRV
#ifdef BMC050_VLA
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_VLAFLAG			_IOR(MSENSOR, 0x34, short)
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_VLADELAY			_IOR(MSENSOR, 0x35, int)
#endif //BMC050_VLA
#ifdef BMC050_VG
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_VGFLAG			_IOR(MSENSOR, 0x36, short)
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_VGDELAY			_IOR(MSENSOR, 0x37, int)
#endif //BMC050_VG
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define BMM_IOC_GET_EVENT_FLAG	ECOMPASS_IOC_GET_OPEN_STATUS
//add for non-block
#define BMM_IOC_GET_NONBLOCK_EVENT_FLAG _IOR(MSENSOR, 0x38, int)

#ifdef MISC_FOR_DAEMON
// calibration msensor and orientation data
static int sensor_data[CALIBRATION_DATA_SIZE];
#if defined(BMC050_M4G) || defined(BMC050_VRV)
int m4g_data[CALIBRATION_DATA_SIZE];
#endif //BMC050_M4G || BMC050_VRV
#if defined(BMC050_VLA)
static int vla_data[CALIBRATION_DATA_SIZE];
#endif //BMC050_VLA

#if defined(BMC050_VG)
static int vg_data[CALIBRATION_DATA_SIZE];
#endif //BMC050_VG
#endif

struct mutex sensor_data_mutex;
DECLARE_WAIT_QUEUE_HEAD(uplink_event_flag_wq);

#ifdef MISC_FOR_DAEMON
static int bmm050d_delay = BMM050_DEFAULT_DELAY;
#ifdef BMC050_M4G
int m4g_delay = BMM050_DEFAULT_DELAY;
#endif //BMC050_M4G
#ifdef BMC050_VRV
static int vrv_delay = BMM050_DEFAULT_DELAY;
#endif //BMC050_VRV
#ifdef BMC050_VLA
static int vla_delay = BMM050_DEFAULT_DELAY;
#endif //BMC050_VRV

#ifdef BMC050_VG
static int vg_delay = BMM050_DEFAULT_DELAY;
#endif //BMC050_VG

static atomic_t m_flag = ATOMIC_INIT(0);
static atomic_t o_flag = ATOMIC_INIT(0);
#ifdef BMC050_M4G
atomic_t g_flag = ATOMIC_INIT(0);
#endif //BMC050_M4G
#ifdef BMC050_VRV
static atomic_t vrv_flag = ATOMIC_INIT(0);
#endif //BMC050_VRV
#ifdef BMC050_VLA
static atomic_t vla_flag = ATOMIC_INIT(0);
#endif //BMC050_VLA
#ifdef BMC050_VG
static atomic_t vg_flag = ATOMIC_INIT(0);
#endif //BMC050_VG
#endif

#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
static atomic_t driver_suspend_flag = ATOMIC_INIT(0);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND

/*add for combine BMI160, bma253  by cly 20161202*/
#ifdef CONFIG_LCT_BOOT_REASON
extern int lct_get_sku(void);
static int sku = 0;
#endif  

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static struct platform_device *gsensorPltFmDev;
extern struct platform_device *get_gsensor_platformdev(void);
#endif 

struct mutex uplink_event_flag_mutex;
/* uplink event flag */
volatile u32 uplink_event_flag = 0;
/* uplink event flag bitmap */
enum {
	/* active */
	BMMDRV_ULEVT_FLAG_O_ACTIVE = 0x0001,
	BMMDRV_ULEVT_FLAG_M_ACTIVE = 0x0002,
	BMMDRV_ULEVT_FLAG_G_ACTIVE = 0x0004,
	BMMDRV_ULEVT_FLAG_VRV_ACTIVE = 0x0008,/* Virtual Rotation Vector */
	BMMDRV_ULEVT_FLAG_FLIP_ACTIVE = 0x0010,
	BMMDRV_ULEVT_FLAG_VLA_ACTIVE = 0x0020,/* Virtual Linear Accelerometer */
	BMMDRV_ULEVT_FLAG_VG_ACTIVE = 0x0040,/* Virtual Gravity */

	/* delay */
	BMMDRV_ULEVT_FLAG_O_DELAY = 0x0100,
	BMMDRV_ULEVT_FLAG_M_DELAY = 0x0200,
	BMMDRV_ULEVT_FLAG_G_DELAY = 0x0400,
	BMMDRV_ULEVT_FLAG_VRV_DELAY = 0x0800,
	BMMDRV_ULEVT_FLAG_FLIP_DELAY = 0x1000,
	BMMDRV_ULEVT_FLAG_VLA_DELAY = 0x2000,
	BMMDRV_ULEVT_FLAG_VG_DELAY = 0x4000,

	/* all */
	BMMDRV_ULEVT_FLAG_ALL = 0xffff
};

//tad3sgh add --
#define MAX_FIFO_F_LEVEL 32
#define MAX_FIFO_F_BYTES 6

/*----------------------------------------------------------------------------*/
#define BMI160_ACC_AXIS_X          0
#define BMI160_ACC_AXIS_Y          1
#define BMI160_ACC_AXIS_Z          2
#define BMI160_ACC_AXES_NUM        3
#define BMI160_ACC_DATA_LEN        6
#define BMI160_DEV_NAME        "bmi160_acc"

#define BMI160_ACC_MODE_NORMAL      0
#define BMI160_ACC_MODE_LOWPOWER    1
#define BMI160_ACC_MODE_SUSPEND     2

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

#define ENABLE_ISR_DEBUG_MSG
#ifdef ENABLE_ISR_DEBUG_MSG
#define ISR_INFO(dev, fmt, arg...) dev_err(dev, fmt, ##arg)
#else
#define ISR_INFO(dev, fmt, arg...)
#endif

#define ABSMIN                      -512
#define ABSMAX                      512

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
#define PAD_SLOW_NO_MOTION          3
#define PAD_DOUBLE_TAP              4
#define PAD_SINGLE_TAP              5
#define PAD_ORIENT                  6
#define PAD_FLAT                    7

#define BMI160_THETA_FLAT_REG                   0x67

#define BMI160_THETA_FLAT__POS                  0
#define BMI160_THETA_FLAT__LEN                  6
#define BMI160_THETA_FLAT__MSK                  0x3F
#define BMI160_THETA_FLAT__REG                  BMI160_THETA_FLAT_REG

#define BMI160_FLAT_HOLD_TIME_REG               0x68

#define BMI160_FLAT_HOLD_TIME__POS              4
#define BMI160_FLAT_HOLD_TIME__LEN              2
#define BMI160_FLAT_HOLD_TIME__MSK              0x30
#define BMI160_FLAT_HOLD_TIME__REG              BMI160_FLAT_HOLD_TIME_REG

#define BMI160_SLOPE_DURN_REG                   0x5f

#define BMI160_SLO_NO_MOT_DUR__POS   2
#define BMI160_SLO_NO_MOT_DUR__LEN   6
#define BMI160_SLO_NO_MOT_DUR__MSK   0xFC
#define BMI160_SLO_NO_MOT_DUR__REG   BMI160_SLOPE_DURN_REG

#define BMI160_SLO_NO_MOT_THRES_REG             0x61

#define BMI160_INT_SLO_NO_MOT_REG               0x52

#define BMI160_INT_SLO_NO_MOT_EN_X_INT__POS        0
#define BMI160_INT_SLO_NO_MOT_EN_X_INT__LEN        1
#define BMI160_INT_SLO_NO_MOT_EN_X_INT__MSK        0x01
#define BMI160_INT_SLO_NO_MOT_EN_X_INT__REG        BMI160_INT_SLO_NO_MOT_REG

#define BMI160_INT_SLO_NO_MOT_EN_Y_INT__POS        1
#define BMI160_INT_SLO_NO_MOT_EN_Y_INT__LEN        1
#define BMI160_INT_SLO_NO_MOT_EN_Y_INT__MSK        0x02
#define BMI160_INT_SLO_NO_MOT_EN_Y_INT__REG        BMI160_INT_SLO_NO_MOT_REG

#define BMI160_INT_SLO_NO_MOT_EN_Z_INT__POS        2
#define BMI160_INT_SLO_NO_MOT_EN_Z_INT__LEN        1
#define BMI160_INT_SLO_NO_MOT_EN_Z_INT__MSK        0x04
#define BMI160_INT_SLO_NO_MOT_EN_Z_INT__REG        BMI160_INT_SLO_NO_MOT_REG

#define BMI160_INT_MOTION_3              0X62

#define BMI160_INT_SLO_NO_MOT_EN_SEL_INT__POS      0
#define BMI160_INT_SLO_NO_MOT_EN_SEL_INT__LEN      1
#define BMI160_INT_SLO_NO_MOT_EN_SEL_INT__MSK      0x01
#define BMI160_INT_SLO_NO_MOT_EN_SEL_INT__REG      BMI160_INT_MOTION_3

#define BMI160_STATUS3_REG           0x1F

#define BMI160_FLAT_S__POS               7
#define BMI160_FLAT_S__LEN               1
#define BMI160_FLAT_S__MSK               0x80
#define BMI160_FLAT_S__REG               BMI160_STATUS3_REG

#define BMI160_STATUS1_REG                      0x1D
#define BMI160_STATUS0_REG                      0x1C

#define BMI160_INT_EN_0              0x50

#define BMI160_EN_FLAT_INT__POS            7
#define BMI160_EN_FLAT_INT__LEN            1
#define BMI160_EN_FLAT_INT__MSK            0x80
#define BMI160_EN_FLAT_INT__REG            BMI160_INT_EN_0

#define BMI160_INT_OUT_CTRL_REG    0x53

#define BMI160_INT1_EDAGE_CTRL__POS        0
#define BMI160_INT1_EDAGE_CTRL__LEN        1
#define BMI160_INT1_EDAGE_CTRL__MSK        0x01
#define BMI160_INT1_EDAGE_CTRL__REG        BMI160_INT_OUT_CTRL_REG

#define BMI160_INT1_LVL__POS               1
#define BMI160_INT1_LVL__LEN               1
#define BMI160_INT1_LVL__MSK               0x02
#define BMI160_INT1_LVL__REG               BMI160_INT_OUT_CTRL_REG

#define BMI160_INT1_OD__POS                2
#define BMI160_INT1_OD__LEN                1
#define BMI160_INT1_OD__MSK                0x04
#define BMI160_INT1_OD__REG                BMI160_INT_OUT_CTRL_REG

#define BMI160_INT1_OUTPUT_EN__POS         3
#define BMI160_INT1_OUTPUT_EN__LEN         1
#define BMI160_INT1_OUTPUT_EN__MSK         0x08
#define BMI160_INT1_OUTPUT_EN__REG         BMI160_INT_OUT_CTRL_REG

#define BMI160_INT2_EDAGE_CTRL__POS        4
#define BMI160_INT2_EDAGE_CTRL__LEN        1
#define BMI160_INT2_EDAGE_CTRL__MSK        0x10
#define BMI160_INT2_EDAGE_CTRL__REG        BMI160_INT_OUT_CTRL_REG

#define BMI160_INT2_LVL__POS               5
#define BMI160_INT2_LVL__LEN               1
#define BMI160_INT2_LVL__MSK               0x20
#define BMI160_INT2_LVL__REG               BMI160_INT_OUT_CTRL_REG

#define BMI160_INT2_OD__POS                6
#define BMI160_INT2_OD__LEN                1
#define BMI160_INT2_OD__MSK                0x40
#define BMI160_INT2_OD__REG                BMI160_INT_OUT_CTRL_REG

#define BMI160_INT2_OUTPUT_EN__POS         7
#define BMI160_INT2_OUTPUT_EN__LEN         1
#define BMI160_INT2_OUTPUT_EN__MSK         0x80
#define BMI160_INT2_OUTPUT_EN__REG         BMI160_INT_OUT_CTRL_REG

#define BMI160_INT_LATCH_REG                    0x54

#define BMI160_INT_MODE_SEL__POS                0
#define BMI160_INT_MODE_SEL__LEN                4
#define BMI160_INT_MODE_SEL__MSK                0x0F
#define BMI160_INT_MODE_SEL__REG                BMI160_INT_LATCH_REG

#define BMI160_INT_MODE_INT1_EN__POS                4
#define BMI160_INT_MODE_INT1_EN__LEN                1
#define BMI160_INT_MODE_INT1_EN__MSK                0x10
#define BMI160_INT_MODE_INT1_EN__REG                BMI160_INT_LATCH_REG

#define BMI160_INT_MODE_INT2_EN__POS                5
#define BMI160_INT_MODE_INT2_EN__LEN                1
#define BMI160_INT_MODE_INT2_EN__MSK                0x20
#define BMI160_INT_MODE_INT2_EN__REG                BMI160_INT_LATCH_REG

#define BMI160_INT1_PAD_SEL_REG                 0x55

#define BMI160_EN_INT1_PAD_LOWG__POS        0
#define BMI160_EN_INT1_PAD_LOWG__LEN        1
#define BMI160_EN_INT1_PAD_LOWG__MSK        0x01
#define BMI160_EN_INT1_PAD_LOWG__REG        BMI160_INT1_PAD_SEL_REG

#define BMI160_EN_INT1_PAD_HIGHG__POS       1
#define BMI160_EN_INT1_PAD_HIGHG__LEN       1
#define BMI160_EN_INT1_PAD_HIGHG__MSK       0x02
#define BMI160_EN_INT1_PAD_HIGHG__REG       BMI160_INT1_PAD_SEL_REG

#define BMI160_EN_INT1_PAD_SLOPE__POS       2
#define BMI160_EN_INT1_PAD_SLOPE__LEN       1
#define BMI160_EN_INT1_PAD_SLOPE__MSK       0x04
#define BMI160_EN_INT1_PAD_SLOPE__REG       BMI160_INT1_PAD_SEL_REG

#define BMI160_EN_INT1_PAD_SLO_NO_MOT__POS        3
#define BMI160_EN_INT1_PAD_SLO_NO_MOT__LEN        1
#define BMI160_EN_INT1_PAD_SLO_NO_MOT__MSK        0x08
#define BMI160_EN_INT1_PAD_SLO_NO_MOT__REG        BMI160_INT1_PAD_SEL_REG

#define BMI160_EN_INT1_PAD_DB_TAP__POS      4
#define BMI160_EN_INT1_PAD_DB_TAP__LEN      1
#define BMI160_EN_INT1_PAD_DB_TAP__MSK      0x10
#define BMI160_EN_INT1_PAD_DB_TAP__REG      BMI160_INT1_PAD_SEL_REG

#define BMI160_EN_INT1_PAD_SNG_TAP__POS     5
#define BMI160_EN_INT1_PAD_SNG_TAP__LEN     1
#define BMI160_EN_INT1_PAD_SNG_TAP__MSK     0x20
#define BMI160_EN_INT1_PAD_SNG_TAP__REG     BMI160_INT1_PAD_SEL_REG

#define BMI160_EN_INT1_PAD_ORIENT__POS      6
#define BMI160_EN_INT1_PAD_ORIENT__LEN      1
#define BMI160_EN_INT1_PAD_ORIENT__MSK      0x40
#define BMI160_EN_INT1_PAD_ORIENT__REG      BMI160_INT1_PAD_SEL_REG

#define BMI160_EN_INT1_PAD_FLAT__POS        7
#define BMI160_EN_INT1_PAD_FLAT__LEN        1
#define BMI160_EN_INT1_PAD_FLAT__MSK        0x80
#define BMI160_EN_INT1_PAD_FLAT__REG        BMI160_INT1_PAD_SEL_REG

#define BMI160_INT2_PAD_SEL_REG                 0x57

#define BMI160_EN_INT2_PAD_LOWG__POS        0
#define BMI160_EN_INT2_PAD_LOWG__LEN        1
#define BMI160_EN_INT2_PAD_LOWG__MSK        0x01
#define BMI160_EN_INT2_PAD_LOWG__REG        BMI160_INT2_PAD_SEL_REG

#define BMI160_EN_INT2_PAD_HIGHG__POS       1
#define BMI160_EN_INT2_PAD_HIGHG__LEN       1
#define BMI160_EN_INT2_PAD_HIGHG__MSK       0x02
#define BMI160_EN_INT2_PAD_HIGHG__REG       BMI160_INT2_PAD_SEL_REG

#define BMI160_EN_INT2_PAD_SLOPE__POS       2
#define BMI160_EN_INT2_PAD_SLOPE__LEN       1
#define BMI160_EN_INT2_PAD_SLOPE__MSK       0x04
#define BMI160_EN_INT2_PAD_SLOPE__REG       BMI160_INT2_PAD_SEL_REG

#define BMI160_EN_INT2_PAD_SLO_NO_MOT__POS        3
#define BMI160_EN_INT2_PAD_SLO_NO_MOT__LEN        1
#define BMI160_EN_INT2_PAD_SLO_NO_MOT__MSK        0x08
#define BMI160_EN_INT2_PAD_SLO_NO_MOT__REG        BMI160_INT2_PAD_SEL_REG

#define BMI160_EN_INT2_PAD_DB_TAP__POS      4
#define BMI160_EN_INT2_PAD_DB_TAP__LEN      1
#define BMI160_EN_INT2_PAD_DB_TAP__MSK      0x10
#define BMI160_EN_INT2_PAD_DB_TAP__REG      BMI160_INT2_PAD_SEL_REG

#define BMI160_EN_INT2_PAD_SNG_TAP__POS     5
#define BMI160_EN_INT2_PAD_SNG_TAP__LEN     1
#define BMI160_EN_INT2_PAD_SNG_TAP__MSK     0x20
#define BMI160_EN_INT2_PAD_SNG_TAP__REG     BMI160_INT2_PAD_SEL_REG

#define BMI160_EN_INT2_PAD_ORIENT__POS      6
#define BMI160_EN_INT2_PAD_ORIENT__LEN      1
#define BMI160_EN_INT2_PAD_ORIENT__MSK      0x40
#define BMI160_EN_INT2_PAD_ORIENT__REG      BMI160_INT2_PAD_SEL_REG

#define BMI160_EN_INT2_PAD_FLAT__POS        7
#define BMI160_EN_INT2_PAD_FLAT__LEN        1
#define BMI160_EN_INT2_PAD_FLAT__MSK        0x80
#define BMI160_EN_INT2_PAD_FLAT__REG        BMI160_INT2_PAD_SEL_REG

#define TEST_BIT(pos, number) (number & (1 << pos))

enum {
	FlatUp = 0,
	FlatDown,
	Motion,
	numSensors   /* This needs to be at the end of the list */
};

struct bmi160acc {
	int x;
	int y;
	int z;
};

static int bmi160_aod_set_Int_Enable(struct i2c_client *client, unsigned char
		InterruptType , unsigned char value);

#endif//CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

static int bmi160_acc_get_data(int* x ,int* y,int* z, int* status);

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id bmi160_acc_i2c_id[] = {{BMI160_DEV_NAME,0},{}};
//static struct i2c_board_info __initdata bmi160_acc_i2c_info ={ I2C_BOARD_INFO(BMI160_DEV_NAME, BMI160_I2C_ADDR)};

#define COMPATIABLE_NAME "mediatek,bmi120_new" 

/*----------------------------------------------------------------------------*/
static int bmi160_acc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int bmi160_acc_i2c_remove(struct i2c_client *client);
static int bmi160_acc_local_init(void);
static int bmi160_acc_remove(void);

/*----------------------------------------------------------------------------*/
typedef enum {
    BMA_TRC_FILTER  = 0x01,
    BMA_TRC_RAWDATA = 0x02,
    BMA_TRC_IOCTL   = 0x04,
    BMA_TRC_CALI	= 0X08,
    BMA_TRC_INFO	= 0X10,
} BMA_TRC;
/*----------------------------------------------------------------------------*/
struct scale_factor{
    u8  whole;
    u8  fraction;
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
    s16 raw[C_MAX_FIR_LENGTH][BMI160_ACC_AXES_NUM];
    int sum[BMI160_ACC_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct bmi160_acc_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;

    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[BMI160_ACC_AXES_NUM+1];
    struct mutex lock;

    /*data*/
    s8                      offset[BMI160_ACC_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[BMI160_ACC_AXES_NUM+1];
    u8			    fifo_count;

    u8  	fifo_data_sel;
    u16 	fifo_bytecount;
    struct 	odr_t odr;
    u64 	fifo_time;
    atomic_t 	layout;   

#if defined(CONFIG_BMI160_ACC_LOWPASS)
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
	struct input_dev *dev_interrupt;
	struct work_struct int1_irq_work;
	struct work_struct int2_irq_work;
	struct workqueue_struct *data_wq;
#endif
};

/*----------------------------------------------------------------------------*/
struct i2c_client *bmi160_acc_i2c_client = NULL;
static struct acc_init_info bmi160_acc_init_info;
static struct bmi160_acc_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = true;
static struct GSENSOR_VECTOR3D gsensor_gain;

static int bmi160_acc_init_flag =-1; // 0<==>OK -1 <==> fail


struct acc_hw accel_cust_bmi160;
static struct acc_hw *hw = &accel_cust_bmi160;


/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO  GSE_TAG fmt, ##args)

/*!bmi160 sensor time depends on ODR */
static const struct bmi_sensor_time_odr_tbl
                sensortime_duration_tbl[TS_MAX_HZ] = {
        {0x010000, 2560000, 0x00ffff},/*2560ms, 0.39hz, odr=resver*/
        {0x008000, 1280000, 0x007fff},/*1280ms, 0.78hz, odr_acc=1*/
        {0x004000, 640000, 0x003fff},/*640ms, 1.56hz, odr_acc=2*/
        {0x002000, 320000, 0x001fff},/*320ms, 3.125hz, odr_acc=3*/
        {0x001000, 160000, 0x000fff},/*160ms, 6.25hz, odr_acc=4*/
        {0x000800, 80000,  0x0007ff},/*80ms, 12.5hz*/
        {0x000400, 40000, 0x0003ff},/*40ms, 25hz, odr_acc = odr_gyro =6*/
        {0x000200, 20000, 0x0001ff},/*20ms, 50hz, odr = 7*/
        {0x000100, 10000, 0x0000ff},/*10ms, 100hz, odr=8*/
        {0x000080, 5000, 0x00007f},/*5ms, 200hz, odr=9*/
        {0x000040, 2500, 0x00003f},/*2.5ms, 400hz, odr=10*/
        {0x000020, 1250, 0x00001f},/*1.25ms, 800hz, odr=11*/
        {0x000010, 625, 0x00000f},/*0.625ms, 1600hz, odr=12*/

};

static unsigned char g_fifo_data_arr[2048];/*1024 + 12*4*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bmi160_acc_data_resolution[1] = {
	{{1, 0}, 16384},
};
/*----------------------------------------------------------------------------*/
static struct data_resolution bmi160_acc_offset_resolution = {{3, 9}, 256};

#ifdef FIFO_READ_USE_DMA_MODE_I2C
#include <linux/dma-mapping.h>

#ifndef I2C_MASK_FLAG
#define I2C_MASK_FLAG   (0x00ff)
#define I2C_DMA_FLAG    (0x2000)
#endif

//add by wangxiqiang
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
#define SLT_DEVINFO_ACC_DEBUG
#include  "dev_info.h"
//static char* temp_comments;
struct devinfo_struct *s_DEVINFO_acc;   
//The followd code is for GTP style
static void devinfo_acc_regchar(char *module,char * vendor,char *version,char *used)
{

	s_DEVINFO_acc =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_acc->device_type="Acceleration";
	s_DEVINFO_acc->device_module=module;
	s_DEVINFO_acc->device_vendor=vendor;
	s_DEVINFO_acc->device_ic="BMI160";
	s_DEVINFO_acc->device_info=DEVINFO_NULL;
	s_DEVINFO_acc->device_version=version;
	s_DEVINFO_acc->device_used=used;
#ifdef SLT_DEVINFO_ACC_DEBUG
		printk("[DEVINFO accel sensor]registe CTP device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s>\n",
				s_DEVINFO_acc->device_type,s_DEVINFO_acc->device_module,s_DEVINFO_acc->device_vendor,
				s_DEVINFO_acc->device_ic,s_DEVINFO_acc->device_version,s_DEVINFO_acc->device_info,s_DEVINFO_acc->device_used);
#endif
       DEVINFO_CHECK_DECLARE(s_DEVINFO_acc->device_type,s_DEVINFO_acc->device_module,s_DEVINFO_acc->device_vendor,s_DEVINFO_acc->device_ic,s_DEVINFO_acc->device_version,s_DEVINFO_acc->device_info,s_DEVINFO_acc->device_used);
}
      //devinfo_check_add_device(s_DEVINFO_ctp);


#endif




static void *I2CDMABuf_va = NULL;
static dma_addr_t I2CDMABuf_pa;

static int i2c_dma_read_fifo(struct i2c_client *client,
		uint8_t regaddr, uint8_t *readbuf, int32_t readlen)
{
	int ret;
	struct i2c_msg msg;

	ret= i2c_master_send(client, &regaddr, 1);
	if (ret < 0) {
		GSE_ERR("send command error!!\n");
		return -EFAULT;
	}

	msg.addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = readlen;
	msg.buf = (char *)I2CDMABuf_pa;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		GSE_ERR("dma receive data error!!\n");
		return -EFAULT;
	}

	memcpy(readbuf, I2CDMABuf_va, readlen);
	return ret;
}
#endif

/* I2C operation functions */
static int bma_i2c_read_block(struct i2c_client *client,
			u8 addr, u8 *data, u8 len)
{
	u8 beg = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,	.flags = 0,
			.len = 1,		.buf = &beg
		},
		{
			.addr = client->addr,	.flags = I2C_M_RD,
			.len = len,		.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

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
static int bma_i2c_write_block(struct i2c_client *client, u8 addr,
			u8 *data, u8 len)
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

/*--------------------BMI160_ACC power control function----------------------------------*/
static void BMI160_ACC_power(struct acc_hw *hw, unsigned int on)
{
	GSE_FUN();
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetDataResolution(struct bmi160_acc_i2c_data *obj)
{

/*set g sensor dataresolution here*/

/*BMI160_ACC only can set to 10-bit dataresolution, so do nothing in bmi160_acc driver here*/

/*end of set dataresolution*/

 /*we set measure range from -2g to +2g in BMI160_ACC_SetDataFormat(client, BMI160_ACC_RANGE_2G),
                                                    and set 10-bit dataresolution BMI160_ACC_SetDataResolution()*/

 /*so bmi160_acc_data_resolution[0] set value as {{ 3, 9}, 256} when declaration, and assign the value to obj->reso here*/

 	obj->reso = &bmi160_acc_data_resolution[0];
	return 0;

/*if you changed the measure range, for example call: BMI160_ACC_SetDataFormat(client, BMI160_ACC_RANGE_4G),
you must set the right value to bmi160_acc_data_resolution*/

}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadData(struct i2c_client *client, s16 data[BMI160_ACC_AXES_NUM])
{
#ifdef CONFIG_BMI160_ACC_LOWPASS
	struct bmi160_acc_i2c_data *priv = obj_i2c_data;
#endif
	u8 addr = BMI160_USER_DATA_14_ACC_X_LSB__REG;
	u8 buf[BMI160_ACC_DATA_LEN] = {0};
	int err = 0;

	if(NULL == client)
	{
		return -EINVAL;
	}

	err = bma_i2c_read_block(client, addr, buf, BMI160_ACC_DATA_LEN);
	if(err) {
		GSE_ERR("error: %d\n", err);
	}
	else
	{
		/* Convert sensor raw data to 16-bit integer */
		/* Data X */
		data[BMI160_ACC_AXIS_X] = (s16) ((((s32)((s8)buf[1]))
			  << BMI160_SHIFT_8_POSITION) | (buf[0]));
		/* Data Y */
		data[BMI160_ACC_AXIS_Y] = (s16) ((((s32)((s8)buf[3]))
			  << BMI160_SHIFT_8_POSITION) | (buf[2]));
		/* Data Z */
		data[BMI160_ACC_AXIS_Z] = (s16) ((((s32)((s8)buf[5]))
			  << BMI160_SHIFT_8_POSITION) | (buf[4]));

#ifdef CONFIG_BMI160_ACC_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);
				if(priv->fir.num < firlen)
				{
					priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_X] = data[BMI160_ACC_AXIS_X];
					priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Y] = data[BMI160_ACC_AXIS_Y];
					priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Z] = data[BMI160_ACC_AXIS_Z];
					priv->fir.sum[BMI160_ACC_AXIS_X] += data[BMI160_ACC_AXIS_X];
					priv->fir.sum[BMI160_ACC_AXIS_Y] += data[BMI160_ACC_AXIS_Y];
					priv->fir.sum[BMI160_ACC_AXIS_Z] += data[BMI160_ACC_AXIS_Z];
					if(atomic_read(&priv->trace) & BMA_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_X], priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Y], priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Z],
							priv->fir.sum[BMI160_ACC_AXIS_X], priv->fir.sum[BMI160_ACC_AXIS_Y], priv->fir.sum[BMI160_ACC_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[BMI160_ACC_AXIS_X] -= priv->fir.raw[idx][BMI160_ACC_AXIS_X];
					priv->fir.sum[BMI160_ACC_AXIS_Y] -= priv->fir.raw[idx][BMI160_ACC_AXIS_Y];
					priv->fir.sum[BMI160_ACC_AXIS_Z] -= priv->fir.raw[idx][BMI160_ACC_AXIS_Z];
					priv->fir.raw[idx][BMI160_ACC_AXIS_X] = data[BMI160_ACC_AXIS_X];
					priv->fir.raw[idx][BMI160_ACC_AXIS_Y] = data[BMI160_ACC_AXIS_Y];
					priv->fir.raw[idx][BMI160_ACC_AXIS_Z] = data[BMI160_ACC_AXIS_Z];
					priv->fir.sum[BMI160_ACC_AXIS_X] += data[BMI160_ACC_AXIS_X];
					priv->fir.sum[BMI160_ACC_AXIS_Y] += data[BMI160_ACC_AXIS_Y];
					priv->fir.sum[BMI160_ACC_AXIS_Z] += data[BMI160_ACC_AXIS_Z];
					priv->fir.idx++;
					data[BMI160_ACC_AXIS_X] = priv->fir.sum[BMI160_ACC_AXIS_X]/firlen;
					data[BMI160_ACC_AXIS_Y] = priv->fir.sum[BMI160_ACC_AXIS_Y]/firlen;
					data[BMI160_ACC_AXIS_Z] = priv->fir.sum[BMI160_ACC_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & BMA_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][BMI160_ACC_AXIS_X], priv->fir.raw[idx][BMI160_ACC_AXIS_Y], priv->fir.raw[idx][BMI160_ACC_AXIS_Z],
						priv->fir.sum[BMI160_ACC_AXIS_X], priv->fir.sum[BMI160_ACC_AXIS_Y], priv->fir.sum[BMI160_ACC_AXIS_Z],
						data[BMI160_ACC_AXIS_X], data[BMI160_ACC_AXIS_Y], data[BMI160_ACC_AXIS_Z]);
					}
				}
			}
		}
#endif
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadOffset(struct i2c_client *client, s8 ofs[BMI160_ACC_AXES_NUM])
{
	int err=0;
#ifdef SW_CALIBRATION
	ofs[0]=ofs[1]=ofs[2]=0x0;
#else
	err = bma_i2c_read_block(client, BMI160_ACC_REG_OFSX, ofs, BMI160_ACC_AXES_NUM);
	if(err) {
		GSE_ERR("error: %d\n", err);
	}
#endif
	//GSE_LOG("offesx=%x, y=%x, z=%x",ofs[0],ofs[1],ofs[2]);

	return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ResetCalibration(struct i2c_client *client)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int err=0;

	#ifdef SW_CALIBRATION

	#else
	u8 ofs[4]={0,0,0,0};
	err = bma_i2c_write_block(client, BMI160_ACC_REG_OFSX, ofs, 4);
	if(err) {
		GSE_ERR("error: %d\n", err);
	}
	#endif

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadCalibration(struct i2c_client *client, int dat[BMI160_ACC_AXES_NUM])
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int err = 0;
    int mul;

#ifdef SW_CALIBRATION
    mul = 0;//only SW Calibration, disable HW Calibration
#else
    err = BMI160_ACC_ReadOffset(client, obj->offset);
    if(err) {
	    GSE_ERR("read offset fail, %d\n", err);
	    return err;
    }
    mul = obj->reso->sensitivity/bmi160_acc_offset_resolution.sensitivity;
#endif

    dat[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*(obj->offset[BMI160_ACC_AXIS_X]*mul + obj->cali_sw[BMI160_ACC_AXIS_X]);
    dat[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*(obj->offset[BMI160_ACC_AXIS_Y]*mul + obj->cali_sw[BMI160_ACC_AXIS_Y]);
    dat[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*(obj->offset[BMI160_ACC_AXIS_Z]*mul + obj->cali_sw[BMI160_ACC_AXIS_Z]);

    return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadCalibrationEx(struct i2c_client *client, int act[BMI160_ACC_AXES_NUM], int raw[BMI160_ACC_AXES_NUM])
{
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int mul;

#ifdef SW_CALIBRATION
	mul = 0;//only SW Calibration, disable HW Calibration
#else
	int err;
	err = BMI160_ACC_ReadOffset(client, obj->offset);
	if(err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}
	mul = obj->reso->sensitivity/bmi160_acc_offset_resolution.sensitivity;
#endif

	raw[BMI160_ACC_AXIS_X] = obj->offset[BMI160_ACC_AXIS_X]*mul + obj->cali_sw[BMI160_ACC_AXIS_X];
	raw[BMI160_ACC_AXIS_Y] = obj->offset[BMI160_ACC_AXIS_Y]*mul + obj->cali_sw[BMI160_ACC_AXIS_Y];
	raw[BMI160_ACC_AXIS_Z] = obj->offset[BMI160_ACC_AXIS_Z]*mul + obj->cali_sw[BMI160_ACC_AXIS_Z];

	act[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*raw[BMI160_ACC_AXIS_X];
	act[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*raw[BMI160_ACC_AXIS_Y];
	act[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*raw[BMI160_ACC_AXIS_Z];

	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_WriteCalibration(struct i2c_client *client, int dat[BMI160_ACC_AXES_NUM])
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int err = 0;
	int cali[BMI160_ACC_AXES_NUM], raw[BMI160_ACC_AXES_NUM];
#ifndef SW_CALIBRATION
	int lsb = bmi160_acc_offset_resolution.sensitivity;
	int divisor = obj->reso->sensitivity/lsb;
#endif

	err = BMI160_ACC_ReadCalibrationEx(client, cali, raw);
	if(err)	/*offset will be updated in obj->offset*/
	{
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		raw[BMI160_ACC_AXIS_X], raw[BMI160_ACC_AXIS_Y], raw[BMI160_ACC_AXIS_Z],
		obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z],
		obj->cali_sw[BMI160_ACC_AXIS_X], obj->cali_sw[BMI160_ACC_AXIS_Y], obj->cali_sw[BMI160_ACC_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[BMI160_ACC_AXIS_X] += dat[BMI160_ACC_AXIS_X];
	cali[BMI160_ACC_AXIS_Y] += dat[BMI160_ACC_AXIS_Y];
	cali[BMI160_ACC_AXIS_Z] += dat[BMI160_ACC_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n",
		dat[BMI160_ACC_AXIS_X], dat[BMI160_ACC_AXIS_Y], dat[BMI160_ACC_AXIS_Z]);

#ifdef SW_CALIBRATION
	obj->cali_sw[BMI160_ACC_AXIS_X] = obj->cvt.sign[BMI160_ACC_AXIS_X]*(cali[obj->cvt.map[BMI160_ACC_AXIS_X]]);
	obj->cali_sw[BMI160_ACC_AXIS_Y] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Y]]);
	obj->cali_sw[BMI160_ACC_AXIS_Z] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Z]]);
#else
	obj->offset[BMI160_ACC_AXIS_X] = (s8)(obj->cvt.sign[BMI160_ACC_AXIS_X]*(cali[obj->cvt.map[BMI160_ACC_AXIS_X]])/(divisor));
	obj->offset[BMI160_ACC_AXIS_Y] = (s8)(obj->cvt.sign[BMI160_ACC_AXIS_Y]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Y]])/(divisor));
	obj->offset[BMI160_ACC_AXIS_Z] = (s8)(obj->cvt.sign[BMI160_ACC_AXIS_Z]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[BMI160_ACC_AXIS_X] = obj->cvt.sign[BMI160_ACC_AXIS_X]*(cali[obj->cvt.map[BMI160_ACC_AXIS_X]])%(divisor);
	obj->cali_sw[BMI160_ACC_AXIS_Y] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Y]])%(divisor);
	obj->cali_sw[BMI160_ACC_AXIS_Z] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Z]])%(divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		obj->offset[BMI160_ACC_AXIS_X]*divisor + obj->cali_sw[BMI160_ACC_AXIS_X],
		obj->offset[BMI160_ACC_AXIS_Y]*divisor + obj->cali_sw[BMI160_ACC_AXIS_Y],
		obj->offset[BMI160_ACC_AXIS_Z]*divisor + obj->cali_sw[BMI160_ACC_AXIS_Z],
		obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z],
		obj->cali_sw[BMI160_ACC_AXIS_X], obj->cali_sw[BMI160_ACC_AXIS_Y], obj->cali_sw[BMI160_ACC_AXIS_Z]);

	err = bma_i2c_write_block(obj->client, BMI160_ACC_REG_OFSX, obj->offset, BMI160_ACC_AXES_NUM);
	if(err) {
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
#endif

	return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[2];
	int res = 0;

	memset(databuf, 0, sizeof(u8)*2);

	res = bma_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, databuf, 0x01);
	res = bma_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, databuf, 0x01);
	if(res < 0)
		goto exit_BMI160_ACC_CheckDeviceID;

	switch (databuf[0]) {
	case SENSOR_CHIP_ID_BMI:
	case SENSOR_CHIP_ID_BMI_C2:
	case SENSOR_CHIP_ID_BMI_C3:
		GSE_LOG("BMI160_ACC_CheckDeviceID %d pass!\n ", databuf[0]);
		break;
	default:
		GSE_LOG("BMI160_ACC_CheckDeviceID %d failt!\n ", databuf[0]);
		break;
	}

	exit_BMI160_ACC_CheckDeviceID:
	if (res < 0)
	{
		return BMI160_ACC_ERR_I2C;
	}

	return BMI160_ACC_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0};
	int res = 0;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	if(enable == sensor_power )
	{
		GSE_LOG("Sensor power status is newest!\n");
		return BMI160_ACC_SUCCESS;
	}

	mutex_lock(&obj->lock);
	if(enable == true)
	{
		databuf[0] = CMD_PMU_ACC_NORMAL;
	}
	else
	{
		databuf[0] = CMD_PMU_ACC_SUSPEND;
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
		if (obj->mEnabled) {
			res = bma_i2c_read_block(client, 0x40, &databuf[1], 1);
			if (res>=0) {
				databuf[1] |= 0x80;
				res = bma_i2c_write_block(client, 0x40, &databuf[1], 1);
				if (res>=0)
					databuf[0] = CMD_PMU_ACC_LOWPOWER;
			}
		}
#endif
	}

	res = bma_i2c_write_block(client,
			BMI160_CMD_COMMANDS__REG, &databuf[0], 1);
	if(res < 0)
	{
		GSE_ERR("set power mode failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMI160_ACC_ERR_I2C;
	}

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	if (databuf[0] == CMD_PMU_ACC_NORMAL) {
		res = bma_i2c_read_block(client, 0x40, &databuf[1], 1);
		if (res>=0) {
			databuf[1] &= (u8)(~0x80);
			res = bma_i2c_write_block(client, 0x40, &databuf[1], 1);
		}
	}
#endif

	sensor_power = enable;
	mdelay(4);
	mutex_unlock(&obj->lock);

	return BMI160_ACC_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	u8 databuf[2] = {0};
	int res = 0;

	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client,
		BMI160_USER_ACC_RANGE__REG, &databuf[0], 1);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0],
		BMI160_USER_ACC_RANGE, dataformat);
	res += bma_i2c_write_block(client,
		BMI160_USER_ACC_RANGE__REG, &databuf[0], 1);
	mdelay(1);

	if(res < 0)
	{
		GSE_ERR("set data format failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMI160_ACC_ERR_I2C;
	}
	mutex_unlock(&obj->lock);

	return BMI160_ACC_SetDataResolution(obj);
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[2] = {0};
	int res = 0;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

        GSE_LOG("[%s] bandwidth = %d\n", __func__, bwrate);

	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0],
		BMI160_USER_ACC_CONF_ODR, bwrate);
	res += bma_i2c_write_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	mdelay(1);

	if(res < 0)
	{
		GSE_ERR("set bandwidth failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMI160_ACC_ERR_I2C;
	}
	mutex_unlock(&obj->lock);

	return BMI160_ACC_SUCCESS;
}

static int BMI160_ACC_SetOSR4(struct i2c_client *client)
{
	int res = 0;
	uint8_t databuf[2] = {0};
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	uint8_t bandwidth = BMI160_ACCEL_OSR4_AVG1;
	/*      0       -       enable
	 *      1       -       disable */
	uint8_t accel_undersampling_parameter = 0;

	GSE_LOG("[%s] acc_bmp %d, acc_us %d\n", __func__,
			bandwidth, accel_undersampling_parameter);

	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0],
			BMI160_USER_ACC_CONF_ACC_BWP, bandwidth);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0],
			BMI160_USER_ACC_CONF_ACC_UNDER_SAMPLING,
			accel_undersampling_parameter);
	res += bma_i2c_write_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	mdelay(1);

	if(res < 0)
	{
		GSE_ERR("set OSR failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMI160_ACC_ERR_I2C;
	}
	mutex_unlock(&obj->lock);

	return BMI160_ACC_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	int res = 0;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	mutex_lock(&obj->lock);
	res = bma_i2c_write_block(client, BMI160_USER_INT_EN_0_ADDR, &intenable, 0x01);
	mdelay(1);
	if(res != BMI160_ACC_SUCCESS)
	{
		mutex_unlock(&obj->lock);
		return res;
	}

	res = bma_i2c_write_block(client, BMI160_USER_INT_EN_1_ADDR, &intenable, 0x01);
	mdelay(1);
	if(res != BMI160_ACC_SUCCESS)
	{
		mutex_unlock(&obj->lock);
		return res;
	}

	res = bma_i2c_write_block(client, BMI160_USER_INT_EN_2_ADDR, &intenable, 0x01);
	mdelay(1);
	if(res != BMI160_ACC_SUCCESS)
	{
		mutex_unlock(&obj->lock);
		return res;
	}
	mutex_unlock(&obj->lock);
	GSE_LOG("BMI160_ACC disable interrupt ...\n");

	/*for disable interrupt function*/

	return BMI160_ACC_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int bmi160_acc_init_client(struct i2c_client *client, int reset_cali)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int res = 0;
	GSE_LOG("bmi160_acc_init_client \n");

	if (obj->mEnabled) {
		GSE_LOG("bmi160_acc_init_client: skip for aod is on\n");
		return BMI160_ACC_SUCCESS;
	}

	res = BMI160_ACC_CheckDeviceID(client);
	if(res != BMI160_ACC_SUCCESS)
	{
		return res;
	}
	GSE_LOG("BMI160_ACC_CheckDeviceID ok \n");

	res = BMI160_ACC_SetBWRate(client, BMI160_ACCEL_ODR_200HZ);
	if(res != BMI160_ACC_SUCCESS )
	{
		return res;
	}
	GSE_LOG("BMI160_ACC_SetBWRate OK!\n");

	res = BMI160_ACC_SetOSR4(client);
	if(res != BMI160_ACC_SUCCESS )
	{
		return res;
	}
	GSE_LOG("BMI160_ACC_SetOSR4 OK!\n");

	res = BMI160_ACC_SetDataFormat(client, BMI160_ACCEL_RANGE_2G);
	if(res != BMI160_ACC_SUCCESS)
	{
		return res;
	}
	GSE_LOG("BMI160_ACC_SetDataFormat OK!\n");

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

	res = BMI160_ACC_SetIntEnable(client, 0x00);
	if(res != BMI160_ACC_SUCCESS)
	{
		return res;
	}
	GSE_LOG("BMI160_ACC disable interrupt function!\n");

	res = BMI160_ACC_SetPowerMode(client, false);
	if(res != BMI160_ACC_SUCCESS)
	{
		return res;
	}
	GSE_LOG("BMI160_ACC_SetPowerMode OK!\n");

	if(0 != reset_cali)
	{
		/*reset calibration only in power on*/
		res = BMI160_ACC_ResetCalibration(client);
		if(res != BMI160_ACC_SUCCESS)
		{
			return res;
		}
	}
	GSE_LOG("bmi160_acc_init_client OK!\n");
#ifdef CONFIG_BMI160_ACC_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	mdelay(20);

	return BMI160_ACC_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8)*10);

	if((NULL == buf)||(bufsize<=30))
	{
		return -1;
	}

	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	sprintf(buf, "bmi160_acc");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_CompassReadData(struct i2c_client *client, char *buf, int bufsize)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	//u8 databuf[20];
	int acc[BMI160_ACC_AXES_NUM];
	int res = 0;
	s16 databuf[BMI160_ACC_AXES_NUM];
	//memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	if(sensor_power == false)
	{
		res = BMI160_ACC_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on bmi160_acc error %d!\n", res);
		}
	}

	res = BMI160_ACC_ReadData(client, databuf);
	if(res)
	{
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		/* Add compensated value performed by MTK calibration process*/
		databuf[BMI160_ACC_AXIS_X] += obj->cali_sw[BMI160_ACC_AXIS_X];
		databuf[BMI160_ACC_AXIS_Y] += obj->cali_sw[BMI160_ACC_AXIS_Y];
		databuf[BMI160_ACC_AXIS_Z] += obj->cali_sw[BMI160_ACC_AXIS_Z];

		/*remap coordinate*/
		acc[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*databuf[BMI160_ACC_AXIS_X];
		acc[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*databuf[BMI160_ACC_AXIS_Y];
		acc[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*databuf[BMI160_ACC_AXIS_Z];
		//GSE_LOG("cvt x=%d, y=%d, z=%d \n",obj->cvt.sign[BMI160_ACC_AXIS_X],obj->cvt.sign[BMI160_ACC_AXIS_Y],obj->cvt.sign[BMI160_ACC_AXIS_Z]);

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[BMI160_ACC_AXIS_X], acc[BMI160_ACC_AXIS_Y], acc[BMI160_ACC_AXIS_Z]);

		sprintf(buf, "%d %d %d", (s16)acc[BMI160_ACC_AXIS_X], (s16)acc[BMI160_ACC_AXIS_Y], (s16)acc[BMI160_ACC_AXIS_Z]);
		if(atomic_read(&obj->trace) & BMA_TRC_IOCTL)
		{
			GSE_LOG("gsensor data for compass: %s!\n", buf);
		}
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	//u8 databuf[20];
	int acc[BMI160_ACC_AXES_NUM];
	int res = 0;
	s16 databuf[BMI160_ACC_AXES_NUM];
	//memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	if(sensor_power == false)
	{
		res = BMI160_ACC_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on bmi160_acc error %d!\n", res);
		}
	}

	res = BMI160_ACC_ReadData(client, databuf);
	if(res)
	{
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		//GSE_LOG("raw data x=%d, y=%d, z=%d \n",obj->data[BMI160_ACC_AXIS_X],obj->data[BMI160_ACC_AXIS_Y],obj->data[BMI160_ACC_AXIS_Z]);
		databuf[BMI160_ACC_AXIS_X] += obj->cali_sw[BMI160_ACC_AXIS_X];
		databuf[BMI160_ACC_AXIS_Y] += obj->cali_sw[BMI160_ACC_AXIS_Y];
		databuf[BMI160_ACC_AXIS_Z] += obj->cali_sw[BMI160_ACC_AXIS_Z];

		//GSE_LOG("cali_sw x=%d, y=%d, z=%d \n",obj->cali_sw[BMI160_ACC_AXIS_X],obj->cali_sw[BMI160_ACC_AXIS_Y],obj->cali_sw[BMI160_ACC_AXIS_Z]);

		/*remap coordinate*/
		acc[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*databuf[BMI160_ACC_AXIS_X];
		acc[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*databuf[BMI160_ACC_AXIS_Y];
		acc[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*databuf[BMI160_ACC_AXIS_Z];
		//GSE_LOG("cvt x=%d, y=%d, z=%d \n",obj->cvt.sign[BMI160_ACC_AXIS_X],obj->cvt.sign[BMI160_ACC_AXIS_Y],obj->cvt.sign[BMI160_ACC_AXIS_Z]);

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[BMI160_ACC_AXIS_X], acc[BMI160_ACC_AXIS_Y], acc[BMI160_ACC_AXIS_Z]);

		//Out put the mg
		//GSE_LOG("mg acc=%d, GRAVITY=%d, sensityvity=%d \n",acc[BMI160_ACC_AXIS_X],GRAVITY_EARTH_1000,obj->reso->sensitivity);
		acc[BMI160_ACC_AXIS_X] = acc[BMI160_ACC_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[BMI160_ACC_AXIS_Y] = acc[BMI160_ACC_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[BMI160_ACC_AXIS_Z] = acc[BMI160_ACC_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;

		sprintf(buf, "%04x %04x %04x", acc[BMI160_ACC_AXIS_X], acc[BMI160_ACC_AXIS_Y], acc[BMI160_ACC_AXIS_Z]);
		if(atomic_read(&obj->trace) & BMA_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
		}
	}

	return 0;
}
#ifdef MISC_FOR_DAEMON
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadRawData(struct i2c_client *client, char *buf)
{
	int res = 0;
	s16 databuf[BMI160_ACC_AXES_NUM];

	if (!buf || !client)
	{
		return EINVAL;
	}

	res = BMI160_ACC_ReadData(client, databuf);
	if(res)
	{
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "BMI160_ACC_ReadRawData %04x %04x %04x", databuf[BMI160_ACC_AXIS_X],
			databuf[BMI160_ACC_AXIS_Y], databuf[BMI160_ACC_AXIS_Z]);
	}

	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
/*
static int bmi160_acc_set_mode(struct i2c_client *client, unsigned char mode)
{
	int comres = 0;
	unsigned char databuf[2] = {0};
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	if ((client == NULL) || (mode >= 3))
	{
		return -1;
	}
	mutex_lock(&obj->lock);
	switch (mode) {
	case BMI160_ACC_MODE_NORMAL:
		databuf[0] = CMD_PMU_ACC_NORMAL;
		comres += bma_i2c_write_block(client,
				BMI160_CMD_COMMANDS__REG, &databuf[0], 1);
		break;
	case BMI160_ACC_MODE_SUSPEND:
		databuf[0] = CMD_PMU_ACC_SUSPEND;
		comres += bma_i2c_write_block(client,
				BMI160_CMD_COMMANDS__REG, &databuf[0], 1);
		break;
	default:
		break;
	}

	mdelay(4);

	mutex_unlock(&obj->lock);

	if(comres <= 0)
	{
		return BMI160_ACC_ERR_I2C;
	}
	else
	{
		return comres;
	}
}
*/
/*----------------------------------------------------------------------------*/
static int bmi160_acc_get_mode(struct i2c_client *client, unsigned char *mode)
{
	int comres = 0;
	u8 v_data_u8r = C_BMI160_ZERO_U8X;

	if (client == NULL)
	{
		return -1;
	}
	comres = bma_i2c_read_block(client,
			BMI160_USER_ACC_PMU_STATUS__REG, &v_data_u8r, 1);
	*mode = BMI160_GET_BITSLICE(v_data_u8r,
			BMI160_USER_ACC_PMU_STATUS);

	return comres;
}

/*----------------------------------------------------------------------------*/
/*
static int bmi160_acc_set_range(struct i2c_client *client, unsigned char range)
{
	int comres = 0;
	unsigned char data[2] = {BMI160_USER_ACC_RANGE__REG};
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	if (client == NULL)
	{
		return -1;
	}
	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
			BMI160_USER_ACC_RANGE__REG, data+1, 1);

	data[1]  = BMI160_SET_BITSLICE(data[1],
			BMI160_USER_ACC_RANGE, range);

	comres = i2c_master_send(client, data, 2);
	mutex_unlock(&obj->lock);
	if(comres <= 0)
	{
		return BMI160_ACC_ERR_I2C;
	}
	else
	{
		return comres;
	}
}
*/
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

static irqreturn_t gsensor_eint_func(int irq, void *desc)
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

int bmi160_setup_int1(struct i2c_client *client)
{
	int ret;
	u32 debounce[2] = {0, 0};	
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_cfg;
	struct device_node *node = NULL;

	GSE_FUN();

	/* parse irq */
	node = of_find_compatible_node(NULL, NULL, "mediatek, gse_3-eint");
	if (node) {
		/* parse irq num */
		obj_i2c_data->IRQ1 = irq_of_parse_and_map(node, 0);
		if (!obj_i2c_data->IRQ1) {
			GSE_ERR("can't parse irq num for gse_3-eint\n");
			return -EINVAL;
		}
		GSE_LOG("gse_3-eint = %d\n", obj_i2c_data->IRQ1);

		/* parse debounce settings */
		of_property_read_u32_array(node, "debounce", debounce, ARRAY_SIZE(debounce));
		gpio_request(debounce[0], "gse_3-eint");
		gpio_set_debounce(debounce[0], debounce[1]);
		GSE_LOG("gse_3-eint:gpio = %d, debounce = %d\n", debounce[0], debounce[1]);
	} else {
		GSE_ERR("can't find node for gse_3-eint\n");
		return -EINVAL;
	}

	/* parse pinctrl */
	gsensorPltFmDev = get_gsensor_platformdev();
	pinctrl = devm_pinctrl_get(&gsensorPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		GSE_ERR("can't find pinctrl for gse_3-eint\n");
		return ret;
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_state_int3");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		GSE_ERR("can't find pin_cfg for gse_3-eint\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);

	/* request irq for gse_3-eint */
	if (request_irq(obj_i2c_data->IRQ1, gsensor_eint_func, IRQF_TRIGGER_RISING, "gse_3-eint", &obj_i2c_data->int1_irq_work)) {
		GSE_ERR("request irq for gse_3-eint\n");
		return -EINVAL;
	}


	return 0;
}

int bmi160_setup_int2(struct i2c_client *client)
{
	int ret;
	u32 debounce[2] = {0, 0};	
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_cfg;
	struct device_node *node = NULL;

	GSE_FUN();

	/* parse irq */
	node = of_find_compatible_node(NULL, NULL, "mediatek, gse_4-eint");
	if (node) {
		/* parse irq num */
		obj_i2c_data->IRQ2 = irq_of_parse_and_map(node, 0);
		if (!obj_i2c_data->IRQ2) {
			GSE_ERR("can't parse irq num for gse_4-eint\n");
			return -EINVAL;
		}
		GSE_LOG("gse_4-eint = %d\n", obj_i2c_data->IRQ2);

		/* parse debounce settings */
		of_property_read_u32_array(node, "debounce", debounce, ARRAY_SIZE(debounce));
		gpio_request(debounce[0], "gse_4-eint");
		gpio_set_debounce(debounce[0], debounce[1]);
		GSE_LOG("gse_4-eint:gpio = %d, debounce = %d\n", debounce[0], debounce[1]);
	} else {
		GSE_ERR("can't find node for gse_4-eint\n");
		return -EINVAL;
	}

	/* parse pinctrl */
	gsensorPltFmDev = get_gsensor_platformdev();
	pinctrl = devm_pinctrl_get(&gsensorPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		GSE_ERR("can't find pinctrl for gse_4-eint\n");
		return ret;
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_state_int4");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		GSE_ERR("can't find pin_cfg for gse_4-eint\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);

	/* request irq for gse_4-eint */
	if (request_irq(obj_i2c_data->IRQ2, gsensor_eint_func, IRQF_TRIGGER_RISING, "gse_4-eint", &obj_i2c_data->int2_irq_work)) {
		GSE_ERR("request irq for gse_4-eint\n");
		return -EINVAL;
	}


	return 0;
}
#endif 
/*************/

/*----------------------------------------------------------------------------*/
static int bmi160_acc_get_range(struct i2c_client *client, unsigned char *range)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL)
	{
		return -1;
	}

	comres = bma_i2c_read_block(client, BMI160_USER_ACC_RANGE__REG,	&data, 1);
	*range = BMI160_GET_BITSLICE(data, BMI160_USER_ACC_RANGE);

	return comres;
}
/*----------------------------------------------------------------------------*/
/*
static int bmi160_acc_set_bandwidth(struct i2c_client *client, unsigned char bandwidth)
{
	int comres = 0;
	unsigned char data[2] = {BMI160_USER_ACC_CONF_ODR__REG};
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

        GSE_LOG("[%s] bandwidth = %d\n", __func__, bandwidth);

	if (client == NULL)
	{
		return -1;
	}

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
			BMI160_USER_ACC_CONF_ODR__REG, data+1, 1);

	data[1]  = BMI160_SET_BITSLICE(data[1],
			BMI160_USER_ACC_CONF_ODR, bandwidth);

	comres = i2c_master_send(client, data, 2);
	mdelay(1);
	mutex_unlock(&obj->lock);
	if(comres <= 0)
	{
		return BMI160_ACC_ERR_I2C;
	}
	else
	{
		return comres;
	}
}
*/
/*----------------------------------------------------------------------------*/
static int bmi160_acc_get_bandwidth(struct i2c_client *client, unsigned char *bandwidth)
{
	int comres = 0;

	if (client == NULL)
	{
		return -1;
	}

	comres = bma_i2c_read_block(client, BMI160_USER_ACC_CONF_ODR__REG, bandwidth, 1);
	*bandwidth = BMI160_GET_BITSLICE(*bandwidth, BMI160_USER_ACC_CONF_ODR);

	return comres;
}

/*----------------------------------------------------------------------------*/
#ifdef MISC_FOR_DAEMON
//tad3sgh add++
// Daemon application save the data
static int ECS_SaveData(int buf[CALIBRATION_DATA_SIZE])
{
#if DEBUG
	struct bmi160_acc_i2c_data *data = obj_i2c_data;
#endif

	mutex_lock(&sensor_data_mutex);
	switch (buf[0])
	{
	case 2:	/* SENSOR_HANDLE_MAGNETIC_FIELD */
		memcpy(sensor_data+4, buf+1, 4*sizeof(int));
		break;
	case 3:	/* SENSOR_HANDLE_ORIENTATION */
		memcpy(sensor_data+8, buf+1, 4*sizeof(int));
		break;
#ifdef BMC050_M4G
	case 4:	/* SENSOR_HANDLE_GYROSCOPE */
		memcpy(m4g_data, buf+1, 4*sizeof(int));
		break;
#endif //BMC050_M4G
#ifdef BMC050_VRV
	case 11:	/* SENSOR_HANDLE_ROTATION_VECTOR */
		memcpy(m4g_data+4, buf+1, 4*sizeof(int));
		break;
#endif //BMC050_VRV
#ifdef BMC050_VLA
	case 10: /* SENSOR_HANDLE_LINEAR_ACCELERATION */
		memcpy(vla_data, buf+1, 4*sizeof(int));
		break;
#endif //BMC050_VLA
#ifdef BMC050_VG
	case 9: /* SENSOR_HANDLE_GRAVITY */
		memcpy(vg_data, buf+1, 4*sizeof(int));
		break;
#endif //BMC050_VG
	default:
		break;
	}
	mutex_unlock(&sensor_data_mutex);

#if DEBUG
	if(atomic_read(&data->trace) & BMA_TRC_INFO)
	{
		GSE_LOG("Get daemon data: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d!\n",
			sensor_data[0],sensor_data[1],sensor_data[2],sensor_data[3],
			sensor_data[4],sensor_data[5],sensor_data[6],sensor_data[7],
			sensor_data[8],sensor_data[9],sensor_data[10],sensor_data[11]);
#if defined(BMC050_M4G) || defined(BMC050_VRV)
		GSE_LOG("Get m4g data: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d!\n",
			m4g_data[0],m4g_data[1],m4g_data[2],m4g_data[3],
			m4g_data[4],m4g_data[5],m4g_data[6],m4g_data[7],
			m4g_data[8],m4g_data[9],m4g_data[10],m4g_data[11]);
#endif //BMC050_M4G || BMC050_VRV
#if defined(BMC050_VLA)
		GSE_LOG("Get vla data: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d!\n",
			vla_data[0],vla_data[1],vla_data[2],vla_data[3],
			vla_data[4],vla_data[5],vla_data[6],vla_data[7],
			vla_data[8],vla_data[9],vla_data[10],vla_data[11]);
#endif //BMC050_VLA

#if defined(BMC050_VG)
		GSE_LOG("Get vg data: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d!\n",
			vg_data[0],vg_data[1],vg_data[2],vg_data[3],
			vg_data[4],vg_data[5],vg_data[6],vg_data[7],
			vg_data[8],vg_data[9],vg_data[10],vg_data[11]);
#endif //BMC050_VG
	}
#endif

	return 0;
}
#endif

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static int bmi160_aod_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0)
		return -EIO;
	*data = dummy & 0x000000ff;

	return 0;
}

static int bmi160_aod_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;

	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0)
		return -EIO;
	udelay(2);
	return 0;
}

static int bmi160_aod_set_theta_flat(struct i2c_client *client, unsigned char
		thetaflat)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bmi160_aod_smbus_read_byte(client, BMI160_THETA_FLAT__REG, &data);
	data = BMI160_SET_BITSLICE(data, BMI160_THETA_FLAT, thetaflat);
	comres = bmi160_aod_smbus_write_byte(client, BMI160_THETA_FLAT__REG, &data);

	return comres;
}

static int bmi160_aod_set_flat_hold_time(struct i2c_client *client, unsigned char
		holdtime)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bmi160_aod_smbus_read_byte(client, BMI160_FLAT_HOLD_TIME__REG,
			&data);
	data = BMI160_SET_BITSLICE(data, BMI160_FLAT_HOLD_TIME, holdtime);
	comres = bmi160_aod_smbus_write_byte(client, BMI160_FLAT_HOLD_TIME__REG,
			&data);

	return comres;
}

static int bmi160_aod_set_slope_no_mot_duration(struct i2c_client *client,
			unsigned char duration)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bmi160_aod_smbus_read_byte(client,
			BMI160_SLO_NO_MOT_DUR__REG, &data);
	data = BMI160_SET_BITSLICE(data, BMI160_SLO_NO_MOT_DUR, duration);
	comres = bmi160_aod_smbus_write_byte(client,
			BMI160_SLO_NO_MOT_DUR__REG, &data);

	return comres;
}

static int bmi160_aod_set_slope_no_mot_threshold(struct i2c_client *client,
		unsigned char threshold)
{
	int comres = 0;
	unsigned char data = 0;

	data = threshold;
	comres = bmi160_aod_smbus_write_byte(client,
			BMI160_SLO_NO_MOT_THRES_REG, &data);

	return comres;
}

static int bmi160_aod_set_en_no_motion_int(struct bmi160_acc_i2c_data *bmi160,
		int en)
{
	int err = 0;
	struct i2c_client *client = bmi160->client;

	if (en) {
		/*dur: 192 samples ~= 3s, threshold: 32.25mg, no motion select*/
		err = bmi160_aod_set_slope_no_mot_duration(client, 0x02);
		err += bmi160_aod_set_slope_no_mot_threshold(client, 0x09);
		/*Enable the interrupts*/
		err += bmi160_aod_set_Int_Enable(client, 12, 1);/*slow/no motion X*/
		err += bmi160_aod_set_Int_Enable(client, 13, 1);/*slow/no motion Y*/
		err += bmi160_aod_set_Int_Enable(client, 15, 1);
	} else {
		err = bmi160_aod_set_Int_Enable(client, 12, 0);/*slow/no motion X*/
		err += bmi160_aod_set_Int_Enable(client, 13, 0);/*slow/no motion Y*/
	}
	return err;
}

static int bmi160_aod_flat_update(struct bmi160_acc_i2c_data *bmi160)
{
	static struct bmi160acc acc;
	int status;

	bmi160_acc_get_data(&acc.x , &acc.y, &acc.z, &status);

	//bmi160_aod_read_accel_xyz(bmi160->client,
	//		bmi160->sensor_type, &acc);
	ISR_INFO(&bmi160->client->dev,
		"bmi160_flat_updatez value = %d,%d\n", acc.z, bmi160->flat_threshold);
	if (acc.z > bmi160->flat_threshold)
		bmi160->flat_up_value = FLATUP_GESTURE;
	else
		bmi160->flat_up_value = EXIT_FLATUP_GESTURE;
	if (acc.z < (-1 * bmi160->flat_threshold))
		bmi160->flat_down_value = FLATDOWN_GESTURE;
	else
		bmi160->flat_down_value = EXIT_FLATDOWN_GESTURE;
	return 0;
}

static int bmi160_aod_set_en_sig_int_mode(struct bmi160_acc_i2c_data *bmi160,
		int en)
{
	int err = 0;
	int newstatus = en;

	ISR_INFO(&bmi160->client->dev,
			"int_mode entry value = %x  %x\n",
			bmi160->mEnabled, newstatus);
	mutex_lock(&bmi160->int_mode_mutex);
	if (!bmi160->mEnabled && newstatus) {
		/* set normal mode at first if needed */
		BMI160_ACC_SetPowerMode(bmi160->client, true);
		BMI160_ACC_SetBWRate(bmi160->client,
			BMI160_ACCEL_ODR_800HZ);
		usleep_range(5000, 5000);
		bmi160_aod_flat_update(bmi160);
		bmi160_aod_set_theta_flat(bmi160->client, 0x08);
		if ((bmi160->flat_up_value == FLATUP_GESTURE) ||
			(bmi160->flat_down_value == FLATDOWN_GESTURE))
			bmi160_aod_set_flat_hold_time(bmi160->client, 0x00);
		else
			bmi160_aod_set_flat_hold_time(bmi160->client, 0x03);
		bmi160_aod_set_Int_Enable(bmi160->client, 11, 1);
	} else if (bmi160->mEnabled && !newstatus) {
		bmi160_aod_set_Int_Enable(bmi160->client, 11, 0);
		disable_irq(bmi160->IRQ1);
		disable_irq(bmi160->IRQ2);
		BMI160_ACC_SetBWRate(bmi160->client,
			BMI160_ACCEL_ODR_200HZ);
	}
	if (TEST_BIT(FlatUp, newstatus) &&
			!TEST_BIT(FlatUp, bmi160->mEnabled)) {
		ISR_INFO(&bmi160->client->dev,
		"int_mode FlatUp value =%d\n", bmi160->flat_up_value);
		input_report_rel(bmi160->dev_interrupt,
		FLAT_INTERRUPT, bmi160->flat_up_value);
		input_sync(bmi160->dev_interrupt);
	}
	if (TEST_BIT(FlatDown, newstatus) &&
			!TEST_BIT(FlatDown, bmi160->mEnabled)) {
		ISR_INFO(&bmi160->client->dev,
		"int_mode FlatDown value =%d\n", bmi160->flat_down_value);
		input_report_rel(bmi160->dev_interrupt,
		FLAT_INTERRUPT, bmi160->flat_down_value);
		input_sync(bmi160->dev_interrupt);
	}

	if (TEST_BIT(Motion, newstatus) &&
			!TEST_BIT(Motion, bmi160->mEnabled))
		err = bmi160_aod_set_en_no_motion_int(bmi160, 1);
	else if (!TEST_BIT(Motion, newstatus) &&
			TEST_BIT(Motion, bmi160->mEnabled))
		err = bmi160_aod_set_en_no_motion_int(bmi160, 0);

	if (!bmi160->mEnabled && newstatus) {
		enable_irq(bmi160->IRQ1);
		enable_irq(bmi160->IRQ2);
	}

	/* set low power mode at the end if no need */
	if (bmi160->mEnabled && !newstatus) {
		bmi160->mEnabled = newstatus;/* mEnabled will be used in below func */
		BMI160_ACC_SetPowerMode(bmi160->client, false);
	}

	bmi160->mEnabled = newstatus;
	mutex_unlock(&bmi160->int_mode_mutex);
	ISR_INFO(&bmi160->client->dev, "int_mode finished!!!\n");
	return err;
}

static int bmi160_aod_get_flat_status(struct i2c_client *client, unsigned
		char *intstatus)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bmi160_aod_smbus_read_byte(client, BMI160_STATUS3_REG,
			&data);
	data = BMI160_GET_BITSLICE(data, BMI160_FLAT_S);
	*intstatus = data;

	return comres;
}

static void bmi160_aod_flat_work_func(struct work_struct *work)
{
	struct bmi160_acc_i2c_data *data = container_of((struct delayed_work *)work,
		struct bmi160_acc_i2c_data, flat_work);
	static struct bmi160acc acc;
	unsigned char sign_value = 0;
	int flat_up_value = 0;
	int flat_down_value = 0;
	int status;
	ISR_INFO(&data->client->dev, "bmi160_aod_flat_work_func entry\n");
	flat_up_value = data->flat_up_value;
	flat_down_value = data->flat_down_value;
	bmi160_aod_get_flat_status(data->client,
			&sign_value);
	ISR_INFO(&data->client->dev,
	"flat interrupt sign_value=%d\n", sign_value);
	//bmi160_aod_read_accel_xyz(data->client,
	//		data->sensor_type, &acc);
	bmi160_acc_get_data(&acc.x , &acc.y, &acc.z, &status);

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
		bmi160_aod_set_flat_hold_time(data->client, 0x00);
	} else {
		data->flat_up_value = EXIT_FLATUP_GESTURE;
		data->flat_down_value = EXIT_FLATDOWN_GESTURE;
		bmi160_aod_set_flat_hold_time(data->client, 0x03);
	}
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
		}
		if ((data->flat_down_value != flat_down_value) &&
			(flat_down_value == FLATDOWN_GESTURE)) {
			dev_info(&data->client->dev,
				"glance exit flat down interrupt happened\n");
			input_report_rel(data->dev_interrupt,
					FLAT_INTERRUPT,
					GLANCE_EXIT_FLATDOWN_GESTURE);
			input_sync(data->dev_interrupt);
		}
	}
	ISR_INFO(&data->client->dev, "bmi160_aod_flat_work_func finished\n");
}

static void bmi160_aod_int1_irq_work_func(struct work_struct *work)
{
	struct bmi160_acc_i2c_data *data = obj_i2c_data;
	unsigned short status = 0;
	unsigned char st1 = 0,st2 = 0;

	bmi160_aod_smbus_read_byte(data->client,
		BMI160_STATUS0_REG, &st1);
	bmi160_aod_smbus_read_byte(data->client,
		BMI160_STATUS1_REG, &st2);

	status = st1 | (st2<<8);
	ISR_INFO(&data->client->dev,
		"bmi160_aod_int2_irq_work_func entry status=0x%04x(%02x,%02x)\n", status, st1, st2);
	if (0 == data->mEnabled) {
		dev_info(&data->client->dev,
		"flat interrupt mEnabled=%d\n", data->mEnabled);
		goto exit;
	}
	queue_delayed_work(data->data_wq,
		&data->flat_work, msecs_to_jiffies(50));

exit:
	ISR_INFO(&data->client->dev,
		"bmi160_aod_int2_irq_work_func finished!!\n");
}

static void bmi160_aod_int2_irq_work_func(struct work_struct *work)
{
	struct bmi160_acc_i2c_data *data = obj_i2c_data;
	unsigned short status = 0;
	unsigned char slow_data = 0, st1 = 0,st2 = 0;

{
	int64_t ns;
	struct timespec time;

	time.tv_sec = time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	ns = time.tv_sec * 1000000000LL + time.tv_nsec;
	ISR_INFO(&data->client->dev, "tick1:%lx", (long unsigned int)ns);

}
	bmi160_aod_smbus_read_byte(data->client,
		BMI160_STATUS0_REG, &st1);
	bmi160_aod_smbus_read_byte(data->client,
		BMI160_STATUS1_REG, &st2);
#if 0
	bmi160_aod_i2c_read_block(data->client,
		BMI160_STATUS0_REG, (u8 *)&status, sizeof(status));
#endif
	status = st1 | (st2<<8);
	ISR_INFO(&data->client->dev,
		"bmi160_aod_int1_irq_work_func entry status=0x%04x(%02x,%02x)\n", status, st1, st2);
	if (0 == data->mEnabled) {
		dev_info(&data->client->dev,
		"flat interrupt mEnabled=%d\n", data->mEnabled);
		goto exit;
	}
	switch (status&0x8080) {
	case 0x0080:
		queue_delayed_work(data->data_wq,
		&data->flat_work, msecs_to_jiffies(50));
		break;
	case 0x8080:
	case 0x8000:
		bmi160_aod_smbus_read_byte(
		data->client, BMI160_INT_MOTION_3, &slow_data);
		if (TEST_BIT(0, slow_data)) {
			dev_info(&data->client->dev,
				"no motion interrupt happened\n");
			bmi160_aod_set_Int_Enable(
				data->client, 12, 0);
			bmi160_aod_set_Int_Enable(
				data->client, 13, 0);
			bmi160_aod_set_slope_no_mot_duration(
				data->client, 0x00);
			bmi160_aod_set_slope_no_mot_threshold(
				data->client, 0x32);
			bmi160_aod_set_Int_Enable(
				data->client, 12, 1);
			bmi160_aod_set_Int_Enable(
				data->client, 13, 1);
			bmi160_aod_set_Int_Enable(
				data->client, 15, 0);
			bmi160_aod_flat_update(data);
			bmi160_aod_set_flat_hold_time(
				data->client, 0x00);
			bmi160_aod_set_Int_Enable(
				data->client, 11, 1);
		} else {
			dev_info(&data->client->dev,
				"glance slow motion interrupt happened\n");
			input_report_rel(data->dev_interrupt,
				SLOW_NO_MOTION_INTERRUPT,
				GLANCE_MOVEMENT_GESTURE);
			input_sync(data->dev_interrupt);
			bmi160_aod_set_Int_Enable(
				data->client, 12, 0);
			bmi160_aod_set_Int_Enable(
				data->client, 13, 0);
			bmi160_aod_set_slope_no_mot_duration(
				data->client, 0x01);
			if (data->aod_flag) {
				cancel_delayed_work_sync(&data->flat_work);
				data->aod_flag = 0;
			}
			bmi160_aod_set_slope_no_mot_threshold(
				data->client, 0x09);
			bmi160_aod_set_Int_Enable(
				data->client, 12, 1);
			bmi160_aod_set_Int_Enable(
				data->client, 13, 1);
			bmi160_aod_set_Int_Enable(
				data->client, 15, 1);
		}
		break;
	default:
		break;
	}
exit:
	ISR_INFO(&data->client->dev,
		"bmi160_aod_int1_irq_work_func finished!!\n");
}

static int bmi160_aod_set_Int_Enable(struct i2c_client *client, unsigned char
		InterruptType , unsigned char value)
{
	int comres = 0;
	unsigned char data1 = 0;
	unsigned char data2 = 0;

	if ((11 < InterruptType) && (InterruptType < 16)) {
		switch (InterruptType) {
		case 12:
			/* slow/no motion X Interrupt  */
			comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_X_INT__REG, &data1);
			data1 = BMI160_SET_BITSLICE(data1,
				BMI160_INT_SLO_NO_MOT_EN_X_INT, value);
			comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_X_INT__REG, &data1);
			break;
		case 13:
			/* slow/no motion Y Interrupt  */
			comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_Y_INT__REG, &data1);
			data1 = BMI160_SET_BITSLICE(data1,
				BMI160_INT_SLO_NO_MOT_EN_Y_INT, value);
			comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_Y_INT__REG, &data1);
			break;
		case 14:
			/* slow/no motion Z Interrupt  */
			comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_Z_INT__REG, &data1);
			data1 = BMI160_SET_BITSLICE(data1,
				BMI160_INT_SLO_NO_MOT_EN_Z_INT, value);
			comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_Z_INT__REG, &data1);
			break;
		case 15:
			/* slow / no motion Interrupt select */
			comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_SEL_INT__REG, &data1);
			data1 = BMI160_SET_BITSLICE(data1,
				BMI160_INT_SLO_NO_MOT_EN_SEL_INT, value);
			comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT_SLO_NO_MOT_EN_SEL_INT__REG, &data1);
		}

	return comres;
	}

	comres = bmi160_aod_smbus_read_byte(client, BMI160_INT_EN_0, &data2);

	value = value & 1;
	switch (InterruptType) {
#if 0
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
#endif
	case 11:
		/* Flat Interrupt */
		data2 = BMI160_SET_BITSLICE(data2, BMI160_EN_FLAT_INT, value);
		break;
	default:
		break;
	}
	comres = bmi160_aod_smbus_write_byte(client, BMI160_INT_EN_0,
			&data2);

	return comres;
}

static int bmi160_aod_set_int_latch(struct i2c_client *client)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bmi160_aod_smbus_read_byte(client,
			BMI160_INT_MODE_SEL__REG, &data);
	data = BMI160_SET_BITSLICE(data, BMI160_INT_MODE_SEL, 0xc);/*latch interrupt 320mS*/
	comres = bmi160_aod_smbus_write_byte(client,
			BMI160_INT_MODE_SEL__REG, &data);

	return comres;
}

static int bmi160_aod_config_int1(struct i2c_client *client)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bmi160_aod_smbus_read_byte(client,
			BMI160_INT_OUT_CTRL_REG, &data);
	data = BMI160_SET_BITSLICE(data, BMI160_INT1_EDAGE_CTRL, 1);/* edage */
	data = BMI160_SET_BITSLICE(data, BMI160_INT1_LVL, 1);/* active high */
	data = BMI160_SET_BITSLICE(data, BMI160_INT1_OD, 0)/* push pull */;
	data = BMI160_SET_BITSLICE(data, BMI160_INT1_OUTPUT_EN, 1);/* enable */
	comres = bmi160_aod_smbus_write_byte(client,
			BMI160_INT_OUT_CTRL_REG, &data);

	comres = bmi160_aod_smbus_read_byte(client,
			BMI160_INT_MODE_INT1_EN__REG, &data);
	data = BMI160_SET_BITSLICE(data, BMI160_INT_MODE_INT1_EN, 0x0b);/*latch interrupt 320mS*/
	comres = bmi160_aod_smbus_write_byte(client,
			BMI160_INT_MODE_INT1_EN__REG, &data);

	return comres;
}

static int bmi160_aod_set_int1_pad_sel(struct i2c_client *client,
		unsigned char int1sel)
{
	int comres = 0;
	unsigned char data = 0;
	unsigned char state = 0;
	state = 0x01;

	switch (int1sel) {
	case PAD_LOWG:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_LOWG,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	case PAD_HIGHG:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_HIGHG,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	case PAD_SLOP:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_SLOPE,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	case PAD_SLOW_NO_MOTION:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_SLO_NO_MOT,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	case PAD_DOUBLE_TAP:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_DB_TAP,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	case PAD_SINGLE_TAP:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_SNG_TAP,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	case PAD_ORIENT:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_ORIENT,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	case PAD_FLAT:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT1_PAD_FLAT,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT1_PAD_SEL_REG, &data);
		break;
	default:
		break;
	}

	return comres;
}

//#else
static int bmi160_aod_config_int2(struct i2c_client *client)
{
	int comres = 0;
	unsigned char data = 0;

	comres = bmi160_aod_smbus_read_byte(client,
			BMI160_INT_OUT_CTRL_REG, &data);
	data = BMI160_SET_BITSLICE(data, BMI160_INT2_EDAGE_CTRL, 1);/* edage */
	data = BMI160_SET_BITSLICE(data, BMI160_INT2_LVL, 1);/* active high */
	data = BMI160_SET_BITSLICE(data, BMI160_INT2_OD, 0)/* push pull */;
	data = BMI160_SET_BITSLICE(data, BMI160_INT2_OUTPUT_EN, 1);/* enable */
	comres = bmi160_aod_smbus_write_byte(client,
			BMI160_INT_OUT_CTRL_REG, &data);

	comres = bmi160_aod_smbus_read_byte(client,
			BMI160_INT_MODE_INT2_EN__REG, &data);
	data = BMI160_SET_BITSLICE(data, BMI160_INT_MODE_INT2_EN, 0x0b);/*latch interrupt 320mS*/
	comres = bmi160_aod_smbus_write_byte(client,
			BMI160_INT_MODE_INT2_EN__REG, &data);

	return comres;
}

static int bmi160_aod_set_int2_pad_sel(struct i2c_client *client,
		unsigned char int2sel)
{
	int comres = 0;
	unsigned char data = 0;
	unsigned char state = 0;
	state = 0x01;

	switch (int2sel) {
	case PAD_LOWG:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_LOWG,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	case PAD_HIGHG:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_HIGHG,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	case PAD_SLOP:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_SLOPE,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	case PAD_SLOW_NO_MOTION:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_SLO_NO_MOT,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	case PAD_DOUBLE_TAP:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_DB_TAP,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	case PAD_SINGLE_TAP:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_SNG_TAP,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	case PAD_ORIENT:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_ORIENT,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	case PAD_FLAT:
		comres = bmi160_aod_smbus_read_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		data = BMI160_SET_BITSLICE(data, BMI160_EN_INT2_PAD_FLAT,
				state);
		comres = bmi160_aod_smbus_write_byte(client,
				BMI160_INT2_PAD_SEL_REG, &data);
		break;
	default:
		break;
	}

	return comres;
}

//#endif
#endif//CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

//tad3sgh add--
/*----------------------------------------------------------------------------*/

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bmi160_acc_i2c_client;
	char strbuf[BMI160_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	BMI160_ACC_ReadChipInfo(client, strbuf, BMI160_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/
static ssize_t show_cpsopmode_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bmi160_acc_get_mode(bmi160_acc_i2c_client, &data) < 0)
	{
		return sprintf(buf, "Read error\n");
	}
	else
	{
		return sprintf(buf, "%d\n", data);
	}
}

/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/

/*
static ssize_t store_cpsopmode_value(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
	{
		return error;
	}
	if (data == BMI160_ACC_MODE_NORMAL)
	{
		BMI160_ACC_SetPowerMode(bmi160_acc_i2c_client, true);
	}
	else if (data == BMI160_ACC_MODE_SUSPEND)
	{
		BMI160_ACC_SetPowerMode(bmi160_acc_i2c_client, false);
	}
	else if (bmi160_acc_set_mode(bmi160_acc_i2c_client, (unsigned char) data) < 0)
	{
		GSE_ERR("invalid content: '%s'\n", buf);
	}

	return count;
}
*/
/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
static ssize_t show_cpsrange_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bmi160_acc_get_range(bmi160_acc_i2c_client, &data) < 0)
	{
		return sprintf(buf, "Read error\n");
	}
	else
	{
		return sprintf(buf, "%d\n", data);
	}
}

/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
/*
static ssize_t store_cpsrange_value(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
	{
		return error;
	}
	if (bmi160_acc_set_range(bmi160_acc_i2c_client, (unsigned char) data) < 0)
	{
		GSE_ERR("invalid content: '%s'\n", buf);
	}

	return count;
}
*/
/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
static ssize_t show_cpsbandwidth_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bmi160_acc_get_bandwidth(bmi160_acc_i2c_client, &data) < 0)
	{
		return sprintf(buf, "Read error\n");
	}
	else
	{
		return sprintf(buf, "%d\n", data);
	}
}

/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
/*
static ssize_t store_cpsbandwidth_value(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
	{
		return error;
	}
	if (bmi160_acc_set_bandwidth(bmi160_acc_i2c_client, (unsigned char) data) < 0)
	{
		GSE_ERR("invalid content: '%s'\n", buf);
	}

	return count;
}
*/
/*----------------------------------------------------------------------------*/
/*
g sensor data for compass tilt compensation
*/
static ssize_t show_cpsdata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bmi160_acc_i2c_client;
	char strbuf[BMI160_BUFSIZE];

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMI160_ACC_CompassReadData(client, strbuf, BMI160_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bmi160_acc_i2c_client;
	char strbuf[BMI160_BUFSIZE];

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMI160_ACC_ReadSensorData(client, strbuf, BMI160_BUFSIZE);
	//BMI160_ACC_ReadRawData(client, strbuf);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bmi160_acc_i2c_client;
	struct bmi160_acc_i2c_data *obj;
	int err, len = 0, mul;
	int tmp[BMI160_ACC_AXES_NUM];

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = obj_i2c_data;

	err = BMI160_ACC_ReadOffset(client, obj->offset);
	if(err)
	{
		return -EINVAL;
	}

	err = BMI160_ACC_ReadCalibration(client, tmp);
	if(err) {
		return -EINVAL;
	}
	else
	{
		mul = obj->reso->sensitivity/bmi160_acc_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
			obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z],
			obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
			obj->cali_sw[BMI160_ACC_AXIS_X], obj->cali_sw[BMI160_ACC_AXIS_Y], obj->cali_sw[BMI160_ACC_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
			obj->offset[BMI160_ACC_AXIS_X]*mul + obj->cali_sw[BMI160_ACC_AXIS_X],
			obj->offset[BMI160_ACC_AXIS_Y]*mul + obj->cali_sw[BMI160_ACC_AXIS_Y],
			obj->offset[BMI160_ACC_AXIS_Z]*mul + obj->cali_sw[BMI160_ACC_AXIS_Z],
			tmp[BMI160_ACC_AXIS_X], tmp[BMI160_ACC_AXIS_Y], tmp[BMI160_ACC_AXIS_Z]);

		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = bmi160_acc_i2c_client;
	int err, x, y, z;
	int dat[BMI160_ACC_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		err = BMI160_ACC_ResetCalibration(client);
		if(err) {
			GSE_ERR("reset offset err = %d\n", err);
		}
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[BMI160_ACC_AXIS_X] = x;
		dat[BMI160_ACC_AXIS_Y] = y;
		dat[BMI160_ACC_AXIS_Z] = z;
		err = BMI160_ACC_WriteCalibration(client, dat);
		if(err)
		{
			GSE_ERR("write calibration err = %d\n", err);
		}
	}
	else
	{
		GSE_ERR("invalid format\n");
	}

	return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMI160_ACC_LOWPASS
	struct i2c_client *client = bmi160_acc_i2c_client;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	if(atomic_read(&obj->firlen))
	{
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][BMI160_ACC_AXIS_X], obj->fir.raw[idx][BMI160_ACC_AXIS_Y], obj->fir.raw[idx][BMI160_ACC_AXIS_Z]);
		}

		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[BMI160_ACC_AXIS_X], obj->fir.sum[BMI160_ACC_AXIS_Y], obj->fir.sum[BMI160_ACC_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[BMI160_ACC_AXIS_X]/len, obj->fir.sum[BMI160_ACC_AXIS_Y]/len, obj->fir.sum[BMI160_ACC_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_BMI160_ACC_LOWPASS
	struct i2c_client *client = bmi160_acc_i2c_client;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int firlen;

	if(1 != sscanf(buf, "%d", &firlen))
	{
		GSE_ERR("invallid format\n");
	}
	else if(firlen > C_MAX_FIR_LENGTH)
	{
		GSE_ERR("exceeds maximum filter length\n");
	}
	else
	{
		atomic_set(&obj->firlen, firlen);
		if(NULL == firlen)
		{
			atomic_set(&obj->fir_en, 0);
		}
		else
		{
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}
	else
	{
		GSE_ERR("invalid content: '%s'\n", buf);
	}

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if(obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n",
	            obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{
	if(sensor_power)
		GSE_LOG("G sensor is in work mode, sensor_power = %d\n", sensor_power);
	else
		GSE_LOG("G sensor is in standby mode, sensor_power = %d\n", sensor_power);

	return 0;
}

/*----------------------------------------------------------------------------*/
static void bmi_fifo_frame_bytes_extend_calc(
	struct bmi160_acc_i2c_data *client_data,
	unsigned int *fifo_frmbytes_extend)
{
	switch (client_data->fifo_data_sel) {
	case BMI_FIFO_A_SEL:
	case BMI_FIFO_G_SEL:
		*fifo_frmbytes_extend = 7;
		break;
	case BMI_FIFO_G_A_SEL:
		*fifo_frmbytes_extend = 13;
		break;
	case BMI_FIFO_M_SEL:
		*fifo_frmbytes_extend = 9;
		break;
	case BMI_FIFO_M_A_SEL:
	case BMI_FIFO_M_G_SEL:
		/*8(mag) + 6(gyro or acc) +1(head) = 15*/
		*fifo_frmbytes_extend = 15;
		break;
	case BMI_FIFO_M_G_A_SEL:
		/*8(mag) + 6(gyro or acc) + 6 + 1 = 21*/
		*fifo_frmbytes_extend = 21;
		break;
	default:
		*fifo_frmbytes_extend = 0;
		break;
	};
}

static int bmi_fifo_analysis_handle(struct bmi160_acc_i2c_data *client_data,
				u8 *fifo_data, u16 fifo_length, char *buf)
{
	u8 frame_head = 0;/* every frame head*/
	int len = 0;
	u8 acc_frm_cnt = 0;/*0~146*/
	u8 gyro_frm_cnt = 0;
	u64 fifo_time = 0;
	static u32 current_frm_ts;
	u16 fifo_index = 0;/* fifo data buff index*/
	u16 i = 0;
	s8 last_return_st = 0;
	int err = 0;
	unsigned int frame_bytes = 0;
	struct bmi160acc_t acc_frame_arr[FIFO_FRAME_CNT], acc_tmp;
	struct bmi160gyro_t gyro_frame_arr[FIFO_FRAME_CNT], gyro_tmp;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	struct odr_t odr;

	memset(&odr, 0, sizeof(odr));
	for (i = 0; i < FIFO_FRAME_CNT; i++) {
		memset(&acc_frame_arr[i], 0, sizeof(struct bmi160acc_t));
		memset(&gyro_frame_arr[i], 0, sizeof(struct bmi160gyro_t));
	}

	/* no fifo select for bmi sensor*/
	if (!client_data->fifo_data_sel) {
		GSE_ERR("No select any sensor FIFO for BMI16x\n");
		return -EINVAL;
	}

	/*driver need read acc_odr/gyro_odr/mag_odr*/
	if ((client_data->fifo_data_sel) & (1 << BMI_ACC_SENSOR))
		odr.acc_odr = client_data->odr.acc_odr;
	if ((client_data->fifo_data_sel) & (1 << BMI_GYRO_SENSOR))
		odr.gyro_odr = client_data->odr.gyro_odr;
	if ((client_data->fifo_data_sel) & (1 << BMI_MAG_SENSOR))
		odr.mag_odr = client_data->odr.mag_odr;
	bmi_fifo_frame_bytes_extend_calc(client_data, &frame_bytes);
	/* search sensor time sub function firstly */
	for (fifo_index = 0; fifo_index < fifo_length;) {

		frame_head = fifo_data[fifo_index];

		switch (frame_head) {
			/*skip frame 0x40 22 0x84*/
			case FIFO_HEAD_SKIP_FRAME:
				/*fifo data frame index + 1*/
				fifo_index = fifo_index + 1;
				if (fifo_index + 1 > fifo_length) {
					last_return_st = FIFO_SKIP_OVER_LEN;
					break;
				}
				/*skip_frame_cnt = fifo_data[fifo_index];*/
				fifo_index = fifo_index + 1;
				break;

				/*M & G & A*/
			case FIFO_HEAD_M_G_A:
				{/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;
					if (fifo_index + MGA_BYTES_FRM > fifo_length) {
						last_return_st = FIFO_M_G_A_OVER_LEN;
						break;
					}

					fifo_index = fifo_index + MGA_BYTES_FRM;
					break;
				}

			case FIFO_HEAD_M_A:
				{/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;
					if (fifo_index + MA_BYTES_FRM > fifo_length) {
						last_return_st = FIFO_M_A_OVER_LEN;
						break;
					}

					fifo_index = fifo_index + MA_BYTES_FRM;
					break;
				}

			case FIFO_HEAD_M_G:
				{/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;
					if (fifo_index + MG_BYTES_FRM > fifo_length) {
						last_return_st = FIFO_M_G_OVER_LEN;
						break;
					}

					fifo_index = fifo_index + MG_BYTES_FRM;
					break;
				}

			case FIFO_HEAD_G_A:
				{	/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;
					if (fifo_index + GA_BYTES_FRM > fifo_length) {
						last_return_st = FIFO_G_A_OVER_LEN;
						break;
					}

					gyro_tmp.x = fifo_data[fifo_index + 1] << 8 | fifo_data[fifo_index + 0];
					gyro_tmp.y = fifo_data[fifo_index + 3] << 8 | fifo_data[fifo_index + 2];
					gyro_tmp.z = fifo_data[fifo_index + 5] << 8 | fifo_data[fifo_index + 4];

					acc_tmp.x = fifo_data[fifo_index + 7] << 8 | fifo_data[fifo_index + 6];
					acc_tmp.y = fifo_data[fifo_index + 9] << 8 | fifo_data[fifo_index + 8];
					acc_tmp.z = fifo_data[fifo_index + 11] << 8 | fifo_data[fifo_index + 10];

					gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
						obj->cvt.sign[BMI160_ACC_AXIS_X]*gyro_tmp.v[BMI160_ACC_AXIS_X];
					gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Y]*gyro_tmp.v[BMI160_ACC_AXIS_Y];
					gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Z]*gyro_tmp.v[BMI160_ACC_AXIS_Z];

					acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
						obj->cvt.sign[BMI160_ACC_AXIS_X]*acc_tmp.v[BMI160_ACC_AXIS_X];
					acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Y]*acc_tmp.v[BMI160_ACC_AXIS_Y];
					acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Z]*acc_tmp.v[BMI160_ACC_AXIS_Z];

					gyro_frm_cnt++;
					acc_frm_cnt++;
					fifo_index = fifo_index + GA_BYTES_FRM;

					break;
				}
			case FIFO_HEAD_A:
				{	/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;
					if (fifo_index + A_BYTES_FRM > fifo_length) {
						last_return_st = FIFO_A_OVER_LEN;
						break;
					}

					acc_tmp.x = fifo_data[fifo_index + 1] << 8 | fifo_data[fifo_index + 0];
					acc_tmp.y = fifo_data[fifo_index + 3] << 8 | fifo_data[fifo_index + 2];
					acc_tmp.z = fifo_data[fifo_index + 5] << 8 | fifo_data[fifo_index + 4];

					acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
						obj->cvt.sign[BMI160_ACC_AXIS_X]*acc_tmp.v[BMI160_ACC_AXIS_X];
					acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Y]*acc_tmp.v[BMI160_ACC_AXIS_Y];
					acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Z]*acc_tmp.v[BMI160_ACC_AXIS_Z];

					acc_frm_cnt++;/*acc_frm_cnt*/
					fifo_index = fifo_index + A_BYTES_FRM;
					break;
				}
			case FIFO_HEAD_G:
				{	/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;
					if (fifo_index + G_BYTES_FRM > fifo_length) {
						last_return_st = FIFO_G_OVER_LEN;
						break;
					}

					gyro_tmp.x = fifo_data[fifo_index + 1] << 8 | fifo_data[fifo_index + 0];
					gyro_tmp.y = fifo_data[fifo_index + 3] << 8 | fifo_data[fifo_index + 2];
					gyro_tmp.z = fifo_data[fifo_index + 5] << 8 | fifo_data[fifo_index + 4];

					gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
						obj->cvt.sign[BMI160_ACC_AXIS_X]*gyro_tmp.v[BMI160_ACC_AXIS_X];
					gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Y]*gyro_tmp.v[BMI160_ACC_AXIS_Y];
					gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
						obj->cvt.sign[BMI160_ACC_AXIS_Z]*gyro_tmp.v[BMI160_ACC_AXIS_Z];

					gyro_frm_cnt++;/*gyro_frm_cnt*/

					fifo_index = fifo_index + G_BYTES_FRM;
					break;
				}
			case FIFO_HEAD_M:
				{	/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;
					if (fifo_index + A_BYTES_FRM > fifo_length) {
						last_return_st = FIFO_M_OVER_LEN;
						break;
					}

					fifo_index = fifo_index + M_BYTES_FRM;
					break;
				}

				/* sensor time frame*/
			case FIFO_HEAD_SENSOR_TIME:
				{
					/*fifo data frame index + 1*/
					fifo_index = fifo_index + 1;

					if (fifo_index + 3 > fifo_length) {
						last_return_st = FIFO_SENSORTIME_RETURN;
						break;
					}
					fifo_time =
						fifo_data[fifo_index + 2] << 16 |
						fifo_data[fifo_index + 1] << 8 |
						fifo_data[fifo_index + 0];

					client_data->fifo_time = fifo_time;
					/*fifo sensor time frame index + 3*/
					fifo_index = fifo_index + 3;
					break;
				}
			case FIFO_HEAD_OVER_READ_LSB:
				/*fifo data frame index + 1*/
				fifo_index = fifo_index + 1;

				if (fifo_index + 1 > fifo_length) {
					last_return_st = FIFO_OVER_READ_RETURN;
					break;
				}
				if (fifo_data[fifo_index] ==
						FIFO_HEAD_OVER_READ_MSB) {
					/*fifo over read frame index + 1*/
					fifo_index = fifo_index + 1;
					break;
				} else {
					last_return_st = FIFO_OVER_READ_RETURN;
					break;
				}

			default:
				last_return_st = 1;
				break;

		}
		if (last_return_st)
			break;
	}
	fifo_time = 0;

	/*Acc Only*/
	if (client_data->fifo_data_sel == BMI_FIFO_A_SEL) {
		for (i = 0; i < acc_frm_cnt; i++) {
			/*current_frm_ts += 256;*/
			current_frm_ts +=
				sensortime_duration_tbl[odr.acc_odr].ts_duration_us*LMADA;

			len = sprintf(buf, "%s %d %d %d %d ",
					ACC_FIFO_HEAD,
					acc_frame_arr[i].x,
					acc_frame_arr[i].y,
					acc_frame_arr[i].z,
					current_frm_ts);
			buf += len;
			err += len;
		}
	}


	/*only for G*/
	if (client_data->fifo_data_sel == BMI_FIFO_G_SEL) {
		for (i = 0; i < gyro_frm_cnt; i++) {
			/*current_frm_ts += 256;*/
			current_frm_ts +=
				sensortime_duration_tbl[odr.gyro_odr].ts_duration_us*LMADA;

			len = sprintf(buf, "%s %d %d %d %d ",
					GYRO_FIFO_HEAD,
					gyro_frame_arr[i].x,
					gyro_frame_arr[i].y,
					gyro_frame_arr[i].z,
					current_frm_ts
				     );
			buf += len;
			err += len;
		}
	}

	/*only for A G*/
	if (client_data->fifo_data_sel == BMI_FIFO_G_A_SEL) {

		for (i = 0; i < gyro_frm_cnt; i++) {
			/*sensor timeLSB*/
			/*dia(sensor_time) = fifo_time & (0xff), uint:LSB, 39.0625us*/
			/*AP tinmestamp 390625/10000 = 625 /16 */
			current_frm_ts +=
				sensortime_duration_tbl[odr.gyro_odr].ts_duration_us*LMADA;

			len = sprintf(buf,
					"%s %d %d %d %d %s %d %d %d %d ",
					GYRO_FIFO_HEAD,
					gyro_frame_arr[i].x,
					gyro_frame_arr[i].y,
					gyro_frame_arr[i].z,
					current_frm_ts,
					ACC_FIFO_HEAD,
					acc_frame_arr[i].x,
					acc_frame_arr[i].y,
					acc_frame_arr[i].z,
					current_frm_ts
				     );

			buf += len;
			err += len;
		}

	}

	return err;
}

static int bmi160_fifo_length(uint32_t *fifo_length)
{
        int comres=0;
	struct i2c_client *client = bmi160_acc_i2c_client;
        uint8_t a_data_u8r[2] = {0, 0};

	comres += bma_i2c_read_block(client, BMI160_USER_FIFO_BYTE_COUNTER_LSB__REG, a_data_u8r, 2);
	a_data_u8r[1] = BMI160_GET_BITSLICE(a_data_u8r[1], BMI160_USER_FIFO_BYTE_COUNTER_MSB);
	*fifo_length = (uint32_t)(((uint32_t)((uint8_t)(a_data_u8r[1])<<BMI160_SHIFT_8_POSITION)) | a_data_u8r[0]);

        return comres;
}

int bmi160_set_command_register(u8 cmd_reg)
{
        int comres=0;
	struct i2c_client *client = bmi160_acc_i2c_client;

	comres += bma_i2c_write_block(client, BMI160_CMD_COMMANDS__REG, &cmd_reg, 1);

        return comres;
}

static ssize_t bmi160_fifo_bytecount_show(struct device_driver *ddri, char *buf)
{
        int comres=0;
        uint32_t fifo_bytecount = 0;
        uint8_t a_data_u8r[2] = {0, 0};
	struct i2c_client *client = bmi160_acc_i2c_client;

	comres += bma_i2c_read_block(client, BMI160_USER_FIFO_BYTE_COUNTER_LSB__REG, a_data_u8r, 2);
	a_data_u8r[1] = BMI160_GET_BITSLICE(a_data_u8r[1], BMI160_USER_FIFO_BYTE_COUNTER_MSB);
	fifo_bytecount = (uint32_t)(((uint32_t)((uint8_t)(a_data_u8r[1])<<BMI160_SHIFT_8_POSITION)) | a_data_u8r[0]);

        comres = sprintf(buf, "%u\n", fifo_bytecount);
        return comres;
}

static ssize_t bmi160_fifo_bytecount_store(struct device_driver *ddri, const char *buf, size_t count)
{
        struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
        int err;
        unsigned long data;
        err = kstrtoul(buf, 10, &data);
        if (err)
                return err;
        client_data->fifo_bytecount = (unsigned int) data;

        return count;
}

static int bmi160_fifo_data_sel_get(struct bmi160_acc_i2c_data *client_data)
{
        int err = 0;
	struct i2c_client *client = bmi160_acc_i2c_client;
	unsigned char data;
        unsigned char fifo_acc_en, fifo_gyro_en, fifo_mag_en;
        unsigned char fifo_datasel;


	err = bma_i2c_read_block(client, BMI160_USER_FIFO_ACC_EN__REG, &data, 1);
	fifo_acc_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_ACC_EN);

	err += bma_i2c_read_block(client, BMI160_USER_FIFO_GYRO_EN__REG, &data, 1);
	fifo_gyro_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_GYRO_EN);

	err += bma_i2c_read_block(client, BMI160_USER_FIFO_MAG_EN__REG, &data, 1);
	fifo_mag_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_MAG_EN);

        if (err)
                return err;

        fifo_datasel = (fifo_acc_en << BMI_ACC_SENSOR) |
                        (fifo_gyro_en << BMI_GYRO_SENSOR) |
                                (fifo_mag_en << BMI_MAG_SENSOR);

	client_data->fifo_data_sel = fifo_datasel;

        return err;
}

static ssize_t bmi160_fifo_data_sel_show(struct device_driver *ddri, char *buf)
{
        int err = 0;
        struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
        err = bmi160_fifo_data_sel_get(client_data);
        if (err)
                return -EINVAL;
        return sprintf(buf, "%d\n", client_data->fifo_data_sel);
}

static ssize_t bmi160_fifo_data_sel_store(struct device_driver *ddri, const char *buf, size_t count)
{
        struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
	struct i2c_client *client = bmi160_acc_i2c_client;
        int err;
        unsigned long data;
        unsigned char fifo_datasel;
        unsigned char fifo_acc_en, fifo_gyro_en, fifo_mag_en;

        err = kstrtoul(buf, 10, &data);
        if (err)
                return err;
        /* data format: aimed 0b0000 0x(m)x(g)x(a), x:1 enable, 0:disable*/
        if (data > 7)
                return -EINVAL;


        fifo_datasel = (unsigned char)data;
	fifo_acc_en = fifo_datasel & (1 << BMI_ACC_SENSOR) ? 1 : 0;
	fifo_gyro_en = fifo_datasel & (1 << BMI_GYRO_SENSOR) ? 1 : 0;
	fifo_mag_en = fifo_datasel & (1 << BMI_MAG_SENSOR) ? 1 : 0;

	err += bma_i2c_read_block(client, BMI160_USER_FIFO_ACC_EN__REG, &fifo_datasel, 1);
	fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_ACC_EN, fifo_acc_en);
	err += bma_i2c_write_block(client, BMI160_USER_FIFO_ACC_EN__REG, &fifo_datasel, 1);

	err += bma_i2c_read_block(client, BMI160_USER_FIFO_GYRO_EN__REG, &fifo_datasel, 1);
	fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_GYRO_EN, fifo_gyro_en);
	err += bma_i2c_write_block(client, BMI160_USER_FIFO_GYRO_EN__REG, &fifo_datasel, 1);

	err += bma_i2c_read_block(client, BMI160_USER_FIFO_MAG_EN__REG, &fifo_datasel, 1);
	fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_MAG_EN, fifo_mag_en);
	err += bma_i2c_write_block(client, BMI160_USER_FIFO_MAG_EN__REG, &fifo_datasel, 1);

        if (err)
                return -EIO;

	client_data->fifo_data_sel = (unsigned char)data;
	GSE_LOG("FIFO fifo_data_sel %d, A_en:%d, G_en:%d, M_en:%d\n",
			client_data->fifo_data_sel, fifo_acc_en, fifo_gyro_en, fifo_mag_en);

        return count;
}

static ssize_t bmi160_fifo_data_out_frame_show(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bmi160_acc_i2c_client;
	struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
	int err = 0;
	unsigned int fifo_bytecount_tmp;
	if (NULL == g_fifo_data_arr) {
		GSE_ERR("no memory available in fifo_data_frame\n");
		return -ENOMEM;
	}

	if (!client_data->fifo_data_sel)
		return sprintf(buf, "no selsect sensor fifo, fifo_data_sel:%d\n",
				client_data->fifo_data_sel);

	if (client_data->fifo_bytecount == 0)
		return -EINVAL;

	//g_current_apts_us = get_current_timestamp();

	bmi160_fifo_length(&fifo_bytecount_tmp);
	if (fifo_bytecount_tmp > client_data->fifo_bytecount)
		client_data->fifo_bytecount = fifo_bytecount_tmp;
	if (client_data->fifo_bytecount > 210) {
		err += bmi160_set_command_register(CMD_CLR_FIFO_DATA);
		client_data->fifo_bytecount = 210;
	}
	if (!err) {
		memset(g_fifo_data_arr, 0, 2048);
#ifdef FIFO_READ_USE_DMA_MODE_I2C
		err = i2c_dma_read_fifo(client, BMI160_USER_FIFO_DATA__REG, 
				g_fifo_data_arr, client_data->fifo_bytecount);
#else
		err = bma_i2c_read_block(client, BMI160_USER_FIFO_DATA__REG, 
				g_fifo_data_arr, client_data->fifo_bytecount);
#endif
	} else
		GSE_ERR("read fifo leght err");
	if (err) {
		GSE_ERR("brust read fifo err\n");
		return err;
	}

#define isprint(a) ((a >=' ')&&(a <= '~'))
	if (0) {
		int len = client_data->fifo_bytecount;
		const char *ptr = g_fifo_data_arr;
		int i, j;

		for (i = 0; i < len; i += 16) {
			printk(KERN_INFO "%.8x:", i);
			for (j = 0; j < 16; j++) {
				if (!(j % 4))
					printk(" ");
				printk("%.2x", ptr[i + j]);
			}
			printk(" ");
			for (j = 0; j < 16; j++)
				printk("%c", isprint(ptr[i + j]) ? ptr[i + j] : '.');
			printk("\n");
		}
	}

	err = bmi_fifo_analysis_handle(client_data, g_fifo_data_arr,
			client_data->fifo_bytecount, buf);

	return err;
}

static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct bmi160_acc_i2c_data *data = obj_i2c_data;

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		data->hw->direction,atomic_read(&data->layout),	data->cvt.sign[0], data->cvt.sign[1],
		data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct bmi160_acc_i2c_data *data = obj_i2c_data;
	int layout = 0;

	if(1 == sscanf(buf, "%d", &layout))
	{
		atomic_set(&data->layout, layout);
		if(!hwmsen_get_convert(layout, &data->cvt))
		{
			GSE_ERR( "HWMSEN_GET_CONVERT function error!\r\n");
		}
		else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
		{
			GSE_ERR( "invalid layout: %d, restore to %d\n", layout, data->hw->direction);
		}
		else
		{
			GSE_ERR( "invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	}
	else
	{
		GSE_ERR( "invalid format = '%s'\n", buf);
	}
	
	return count;            
}

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static int read_flag = 0;
static u8 read_reg;
static ssize_t reg_dump_show(struct device_driver *ddri, char *buf)
{
	u8 data[20];
	char *p = buf;

	if (read_flag) {
		read_flag = 0;
		bma_i2c_read_block(bmi160_acc_i2c_client, read_reg, data, 1);
		p += snprintf(p, PAGE_SIZE, "%02x\n", data[0]);
		return (p-buf);
	}

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x50, data, 3);
	p += snprintf(p, PAGE_SIZE, "INT EN(50~52)=%02x,%02x,%02x\n",
			data[0], data[1], data[2]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x53, data, 1);
	p += snprintf(p, PAGE_SIZE, "INT OUT CTRL(53)=%02x\n",
			data[0]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x54, data, 1);
	p += snprintf(p, PAGE_SIZE, "INT LATCH(54)=%02x\n",
			data[0]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x55, data, 3);
	p += snprintf(p, PAGE_SIZE, "INT MAP(55~57)=%02x,%02x,%02x\n",
			data[0], data[1], data[2]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x58, data, 2);
	p += snprintf(p, PAGE_SIZE, "INT DATA(58~59)=%02x,%02x\n",
			data[0], data[1]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x5a, data, 5);
	p += snprintf(p, PAGE_SIZE, "INT LOWHIGH(5a~5e)=%02x,%02x,%02x,%02x,%02x\n",
			data[0], data[1], data[2], data[3], data[4]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x5f, data, 4);
	p += snprintf(p, PAGE_SIZE, "INT MOTION(5f~62)=%02x,%02x,%02x,%02x\n",
			data[0], data[1], data[2], data[3]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x63, data, 2);
	p += snprintf(p, PAGE_SIZE, "INT TAP(63~64)=%02x,%02x\n",
			data[0], data[1]);

	bma_i2c_read_block(bmi160_acc_i2c_client, 0x65, data, 2);
	p += snprintf(p, PAGE_SIZE, "INT ORI(65~66)=%02x,%02x\n",
			data[0], data[1]);
	bma_i2c_read_block(bmi160_acc_i2c_client, 0x67, data, 2);
	p += snprintf(p, PAGE_SIZE, "INT FLT(67~68)=%02x,%02x\n",
			data[0], data[1]);
	bma_i2c_read_block(bmi160_acc_i2c_client, 0x1b, data, 5);
	p += snprintf(p, PAGE_SIZE, "INT STS(1b~1f)=%02x,%02x,%02x,%02x,%02x\n",
			data[0], data[1], data[2], data[3], data[4]);

	return (p-buf);
}

static ssize_t reg_dump_store(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned int val, reg, opt;

	if (sscanf(buf, "%x,%x,%x", &reg, &val, &opt) == 3) {
		read_reg = *((u8 *)&reg);
		read_flag = 1;
	} else if (sscanf(buf, "%x,%x", &reg, &val) == 2) {
		GSE_ERR("%s,reg = 0x%02x, val = 0x%02x\n",
			__func__, *(u8 *)&reg, *(u8 *)&val);
		bma_i2c_write_block(bmi160_acc_i2c_client, *(u8 *)&reg,
			(u8 *)&val, 1);
	}

	return count;
}

static ssize_t bmi160_aod_flatdown_show(struct device_driver *ddri, char *buf)
{
	int databuf = 0;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	if (FLATDOWN_GESTURE == obj->flat_down_value)
		databuf = 1;
	else if (EXIT_FLATDOWN_GESTURE == obj->flat_down_value)
		databuf = 0;
	return snprintf(buf, PAGE_SIZE, "%d\n", databuf);
}

static ssize_t bmi160_aod_flat_threshold_show(struct device_driver *ddri, char *buf)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	return snprintf(buf, PAGE_SIZE, "%d\n", obj->flat_threshold);
}

static ssize_t bmi160_aod_flat_threshold_store(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;
	obj->flat_threshold = data;

	return count;
}

static ssize_t bmi160_aod_int_mode_show(struct device_driver *ddri, char *buf)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	return snprintf(buf, PAGE_SIZE, "%d\n", obj->mEnabled);
}

static ssize_t bmi160_aod_int_mode_store(struct device_driver *ddri,
		const char *buf, size_t count)
{
	unsigned long data = 0;
	int error = 0;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;

	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;
	bmi160_aod_set_en_sig_int_mode(obj, data);

	return count;
}
#endif

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo,   S_IWUSR | S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(cpsdata, 	 S_IWUSR | S_IRUGO, show_cpsdata_value,    NULL);
static DRIVER_ATTR(cpsopmode,  S_IWUSR | S_IRUGO, show_cpsopmode_value,    NULL);//store_cpsopmode_value
static DRIVER_ATTR(cpsrange, 	 S_IWUSR | S_IRUGO, show_cpsrange_value,     NULL);//store_cpsrange_value
static DRIVER_ATTR(cpsbandwidth, S_IWUSR | S_IRUGO, show_cpsbandwidth_value,   NULL );//store_cpsbandwidth_value
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
static DRIVER_ATTR(powerstatus,               S_IRUGO, show_power_status_value,        NULL);

static DRIVER_ATTR(fifo_bytecount, S_IRUGO | S_IWUSR, bmi160_fifo_bytecount_show, bmi160_fifo_bytecount_store);
static DRIVER_ATTR(fifo_data_sel, S_IRUGO | S_IWUSR, bmi160_fifo_data_sel_show, bmi160_fifo_data_sel_store);
static DRIVER_ATTR(fifo_data_frame, S_IRUGO, bmi160_fifo_data_out_frame_show, NULL);
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value );
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static DRIVER_ATTR(reg, S_IRUGO | S_IWUSR, reg_dump_show, reg_dump_store );
static DRIVER_ATTR(flatdown, S_IRUGO, bmi160_aod_flatdown_show, NULL);
static DRIVER_ATTR(flat_threshold, S_IWUSR | S_IRUGO, bmi160_aod_flat_threshold_show,
	 bmi160_aod_flat_threshold_store);
static DRIVER_ATTR(int_mode, S_IWUSR | S_IRUGO, bmi160_aod_int_mode_show,
	 bmi160_aod_int_mode_store);
#endif
/*----------------------------------------------------------------------------*/
static struct driver_attribute *bmi160_acc_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,
	&driver_attr_powerstatus,
	&driver_attr_cpsdata,	/*g sensor data for compass tilt compensation*/
	&driver_attr_cpsopmode,	/*g sensor opmode for compass tilt compensation*/
	&driver_attr_cpsrange,	/*g sensor range for compass tilt compensation*/
	&driver_attr_cpsbandwidth,	/*g sensor bandwidth for compass tilt compensation*/

	&driver_attr_fifo_bytecount,
	&driver_attr_fifo_data_sel,
	&driver_attr_fifo_data_frame,
	&driver_attr_layout,
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	&driver_attr_reg,
	&driver_attr_flatdown,
	&driver_attr_flat_threshold,
	&driver_attr_int_mode,
#endif
};
/*----------------------------------------------------------------------------*/
static int bmi160_acc_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bmi160_acc_attr_list)/sizeof(bmi160_acc_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		err = driver_create_file(driver, bmi160_acc_attr_list[idx]);
		if(err) {
			GSE_ERR("driver_create_file (%s) = %d\n", bmi160_acc_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(bmi160_acc_attr_list)/sizeof(bmi160_acc_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, bmi160_acc_attr_list[idx]);
	}

	return err;
}

/*----------------------------------------------------------------------------*/
#ifdef BMC050_VRV
int bmm050_vrv_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* vrv_data;
#if DEBUG
	struct i2c_client *client = bmi160_acc_i2c_client;
	struct bmi160_acc_i2c_data *data = obj_i2c_data;
#endif

#if DEBUG
	if(atomic_read(&data->trace) & BMA_TRC_INFO)
	{
		GSE_FUN();
	}
#endif

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR( "Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				vrv_delay = value;
				/* set the flag */
				mutex_lock(&uplink_event_flag_mutex);
				uplink_event_flag |= BMMDRV_ULEVT_FLAG_VRV_DELAY;
				mutex_unlock(&uplink_event_flag_mutex);
				/* wake up the wait queue */
				wake_up(&uplink_event_flag_wq);
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR( "Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{

				value = *(int *)buff_in;

				if(value == 1)
				{
					atomic_set(&vrv_flag, 1);
				}
				else
				{
					atomic_set(&vrv_flag, 0);
				}

				/* set the flag */
				mutex_lock(&uplink_event_flag_mutex);
				uplink_event_flag |= BMMDRV_ULEVT_FLAG_VRV_ACTIVE;
				mutex_unlock(&uplink_event_flag_mutex);
				/* wake up the wait queue */
				wake_up(&uplink_event_flag_wq);
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GSE_ERR( "get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				vrv_data = (hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);

				vrv_data->values[0] = m4g_data[4];
				vrv_data->values[1] = m4g_data[5];
				vrv_data->values[2] = m4g_data[6];
				vrv_data->status = m4g_data[7];
				vrv_data->value_divide = CONVERT_VRV_DIV;

				mutex_unlock(&sensor_data_mutex);
#if DEBUG
				if(atomic_read(&data->trace) & BMA_TRC_INFO)
				{
					GSE_LOG("Hwm get rotation vector data: %d, %d, %d. divide %d, status %d!\n",
						vrv_data->values[0],vrv_data->values[1],vrv_data->values[2],
						vrv_data->value_divide,vrv_data->status);
				}
#endif
			}
			break;
		default:
			GSE_ERR( "rotation vector operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
#endif //BMC050_VRV
/*----------------------------------------------------------------------------*/
#ifdef BMC050_VLA
int bmm050_vla_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* vla_value;
#if DEBUG
	struct i2c_client *client = bmi160_acc_i2c_client;
	struct bmi160_acc_i2c_data *data = obj_i2c_data;
#endif

#if DEBUG
	if(atomic_read(&data->trace) & BMA_TRC_INFO)
	{
		GSE_FUN();
	}
#endif

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR( "Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				vla_delay = value;
				/* set the flag */
				mutex_lock(&uplink_event_flag_mutex);
				uplink_event_flag |= BMMDRV_ULEVT_FLAG_VLA_DELAY;
				mutex_unlock(&uplink_event_flag_mutex);
				/* wake up the wait queue */
				wake_up(&uplink_event_flag_wq);
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR( "Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{

				value = *(int *)buff_in;

				if(value == 1)
				{
					atomic_set(&vla_flag, 1);
				}
				else
				{
					atomic_set(&vla_flag, 0);
				}

				/* set the flag */
				mutex_lock(&uplink_event_flag_mutex);
				uplink_event_flag |= BMMDRV_ULEVT_FLAG_VLA_ACTIVE;
				mutex_unlock(&uplink_event_flag_mutex);
				/* wake up the wait queue */
				wake_up(&uplink_event_flag_wq);
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GSE_ERR( "get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				vla_value = (hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);

				vla_value->values[0] = vla_data[0];
				vla_value->values[1] = vla_data[1];
				vla_value->values[2] = vla_data[2];
				vla_value->status = vla_data[3];
				vla_value->value_divide = CONVERT_VLA_DIV;

				mutex_unlock(&sensor_data_mutex);
#if DEBUG
				if(atomic_read(&data->trace) & BMA_TRC_INFO)
				{
					GSE_LOG("Hwm get virtual linear accelerometer data: %d, %d, %d. divide %d, status %d!\n",
						vla_value->values[0],vla_value->values[1],vla_value->values[2],
						vla_value->value_divide,vla_value->status);
				}
#endif
			}
			break;
		default:
			GSE_ERR( "virtual linear accelerometer operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
#endif //BMC050_VLA
/*----------------------------------------------------------------------------*/
#ifdef BMC050_VG
int bmm050_vg_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* vg_value;
#if DEBUG
	struct i2c_client *client = bmi160_acc_i2c_client;
	struct bmi160_acc_i2c_data *data = obj_i2c_data;
#endif

#if DEBUG
	if(atomic_read(&data->trace) & BMA_TRC_INFO)
	{
		GSE_FUN();
	}
#endif

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR( "Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				vg_delay = value;
				/* set the flag */
				mutex_lock(&uplink_event_flag_mutex);
				uplink_event_flag |= BMMDRV_ULEVT_FLAG_VG_DELAY;
				mutex_unlock(&uplink_event_flag_mutex);
				/* wake up the wait queue */
				wake_up(&uplink_event_flag_wq);
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR( "Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{

				value = *(int *)buff_in;

				if(value == 1)
				{
					atomic_set(&vg_flag, 1);
				}
				else
				{
					atomic_set(&vg_flag, 0);
				}

				/* set the flag */
				mutex_lock(&uplink_event_flag_mutex);
				uplink_event_flag |= BMMDRV_ULEVT_FLAG_VG_ACTIVE;
				mutex_unlock(&uplink_event_flag_mutex);
				/* wake up the wait queue */
				wake_up(&uplink_event_flag_wq);
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GSE_ERR( "get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				vg_value = (hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);

				vg_value->values[0] = vg_data[0];
				vg_value->values[1] = vg_data[1];
				vg_value->values[2] = vg_data[2];
				vg_value->status = vg_data[3];
				vg_value->value_divide = CONVERT_VG_DIV;

				mutex_unlock(&sensor_data_mutex);
#if DEBUG
				if(atomic_read(&data->trace) & BMA_TRC_INFO)
				{
					GSE_LOG("Hwm get virtual gravity data: %d, %d, %d. divide %d, status %d!\n",
						vg_value->values[0],vg_value->values[1],vg_value->values[2],
						vg_value->value_divide,vg_value->status);
				}
#endif
			}
			break;
		default:
			GSE_ERR( "virtual gravity operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
#endif //BMC050_VG
//tad3sgh add --

#ifdef MISC_FOR_DAEMON
/******************************************************************************
 * Function Configuration
******************************************************************************/
static int bmi160_acc_open(struct inode *inode, struct file *file)
{
	file->private_data = bmi160_acc_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static long bmi160_acc_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	char strbuf[BMI160_BUFSIZE];
	void __user *data;
	struct SENSOR_DATA sensor_data;
	struct ACC_OFFSET_DATA acc_sensor_data;
	long err = 0;
	int cali[3];
	//tad3sgh add ++
	int status; 				/* for OPEN/CLOSE_STATUS */
	short sensor_status;		/* for Orientation and Msensor status */
	int value[CALIBRATION_DATA_SIZE];			/* for SET_YPR */
	//tad3sgh add --
	//GSE_FUN(f);
	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(err)
	{
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{

		case LSM6DS3_GSENSOR_IOCTL_SET_CALI:
		     	printk("wangxiqiang add for ioctl\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			if(copy_from_user(&acc_sensor_data, data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}
			if(atomic_read(&obj->suspend))
			{
				GSE_ERR("Perform calibration in suspend state!!\n");
				err = -EINVAL;
			}
			else
			{
				cali[BMI160_ACC_AXIS_X] = acc_sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[BMI160_ACC_AXIS_Y] = acc_sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[BMI160_ACC_AXIS_Z] = acc_sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				err = BMI160_ACC_WriteCalibration(client, cali);
			}
			break;

		case GSENSOR_IOCTL_INIT:
			bmi160_acc_init_client(client, 0);
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}

			BMI160_ACC_ReadChipInfo(client, strbuf, BMI160_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}
			break;

		case GSENSOR_IOCTL_READ_SENSORDATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}

			BMI160_ACC_ReadSensorData(client, strbuf, BMI160_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}
			break;

		case GSENSOR_IOCTL_READ_GAIN:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}

			if(copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D)))
			{
				err = -EFAULT;
				break;
			}
			break;

		case GSENSOR_IOCTL_READ_RAW_DATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			BMI160_ACC_ReadRawData(client, strbuf);
			if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}
			break;

		case GSENSOR_IOCTL_SET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}
			if(atomic_read(&obj->suspend))
			{
				GSE_ERR("Perform calibration in suspend state!!\n");
				err = -EINVAL;
			}
			else
			{
				cali[BMI160_ACC_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[BMI160_ACC_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[BMI160_ACC_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				err = BMI160_ACC_WriteCalibration(client, cali);
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			err = BMI160_ACC_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			err = BMI160_ACC_ReadCalibration(client, cali);
			if(err) {
				break;
			}

			sensor_data.x = cali[BMI160_ACC_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[BMI160_ACC_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[BMI160_ACC_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}
			break;
		//tad3sgh add ++
		case BMM_IOC_GET_EVENT_FLAG:	// used by daemon only
			data = (void __user *) arg;
			/* block if no event updated */
			wait_event_interruptible(uplink_event_flag_wq, (uplink_event_flag != 0));
			mutex_lock(&uplink_event_flag_mutex);
			status = uplink_event_flag;
			mutex_unlock(&uplink_event_flag_mutex);
			if(copy_to_user(data, &status, sizeof(status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			break;

		case BMM_IOC_GET_NONBLOCK_EVENT_FLAG:	// used by daemon only
			data = (void __user *) arg;
			/* nonblock daemon process */
			//wait_event_interruptible(uplink_event_flag_wq, (uplink_event_flag != 0));
			mutex_lock(&uplink_event_flag_mutex);
			status = uplink_event_flag;
			mutex_unlock(&uplink_event_flag_mutex);
			if(copy_to_user(data, &status, sizeof(status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			break;

		case ECOMPASS_IOC_GET_DELAY:			//used by daemon
			data = (void __user *) arg;
			if(copy_to_user(data, &bmm050d_delay, sizeof(bmm050d_delay)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_M_DELAY) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_M_DELAY;
			}
			else if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_O_DELAY) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_O_DELAY;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;

		case ECOMPASS_IOC_SET_YPR:				//used by daemon
			data = (void __user *) arg;
			if(data == NULL)
			{
				GSE_ERR("invalid argument.");
				return -EINVAL;
			}
			if(copy_from_user(value, data, sizeof(value)))
			{
				GSE_ERR("copy_from_user failed.");
				return -EFAULT;
			}
			ECS_SaveData(value);
			break;

		case ECOMPASS_IOC_GET_MFLAG:		//used by daemon
			data = (void __user *) arg;
			sensor_status = atomic_read(&m_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
			if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
			{
				/* de-active m-channel when driver suspend regardless of m_flag*/
				sensor_status = 0;
			}
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_M_ACTIVE) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_M_ACTIVE;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;

		case ECOMPASS_IOC_GET_OFLAG:		//used by daemon
			data = (void __user *) arg;
			sensor_status = atomic_read(&o_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
			if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
			{
				/* de-active m-channel when driver suspend regardless of m_flag*/
				sensor_status = 0;
			}
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_O_ACTIVE) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_O_ACTIVE;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;

#ifdef BMC050_M4G
		case ECOMPASS_IOC_GET_GDELAY:			//used by daemon
			data = (void __user *) arg;
			if(copy_to_user(data, &m4g_delay, sizeof(m4g_delay)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_G_DELAY) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_G_DELAY;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;

		case ECOMPASS_IOC_GET_GFLAG:		//used by daemon
			data = (void __user *) arg;
			sensor_status = atomic_read(&g_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
			if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
			{
				/* de-active g-channel when driver suspend regardless of g_flag*/
				sensor_status = 0;
			}
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_G_ACTIVE) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_G_ACTIVE;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;
#endif //BMC050_M4G

#ifdef BMC050_VRV
		case ECOMPASS_IOC_GET_VRVDELAY:			//used by daemon
			data = (void __user *) arg;
			if(copy_to_user(data, &vrv_delay, sizeof(vrv_delay)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_VRV_DELAY) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_VRV_DELAY;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;

		case ECOMPASS_IOC_GET_VRVFLAG:		//used by daemon
			data = (void __user *) arg;
			sensor_status = atomic_read(&vrv_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
			if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
			{
				/* de-active vrv-channel when driver suspend regardless of vrv_flag*/
				sensor_status = 0;
			}
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_VRV_ACTIVE) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_VRV_ACTIVE;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;
#endif //BMC050_VRV

#ifdef BMC050_VLA
		case ECOMPASS_IOC_GET_VLADELAY: 		//used by daemon
			data = (void __user *) arg;
			if(copy_to_user(data, &vla_delay, sizeof(vla_delay)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_VLA_DELAY) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_VLA_DELAY;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;

		case ECOMPASS_IOC_GET_VLAFLAG:		//used by daemon
			data = (void __user *) arg;
			sensor_status = atomic_read(&vla_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
			if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
			{
				/* de-active vla-channel when driver suspend regardless of vla_flag*/
				sensor_status = 0;
			}
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_VLA_ACTIVE) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_VLA_ACTIVE;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;
#endif //BMC050_VLA

#ifdef BMC050_VG
		case ECOMPASS_IOC_GET_VGDELAY: 		//used by daemon
			data = (void __user *) arg;
			if(copy_to_user(data, &vg_delay, sizeof(vg_delay)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_VG_DELAY) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_VG_DELAY;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;

		case ECOMPASS_IOC_GET_VGFLAG:		//used by daemon
			data = (void __user *) arg;
			sensor_status = atomic_read(&vg_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
			if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
			{
				/* de-active vla-channel when driver suspend regardless of vla_flag*/
				sensor_status = 0;
			}
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				GSE_ERR("copy_to_user failed.");
				return -EFAULT;
			}
			/* clear the flag */
			mutex_lock(&uplink_event_flag_mutex);
			if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_VG_ACTIVE) != 0)
			{
				uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_VG_ACTIVE;
			}
			mutex_unlock(&uplink_event_flag_mutex);
			/* wake up the wait queue */
			wake_up(&uplink_event_flag_wq);
			break;
#endif //BMC050_VG
		//tad3sgh add --
		default:
			GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;

	}

	return err;
}

#ifdef CONFIG_COMPAT
static long bmi160_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {

	case LSM6DS3_COMPAT_GSENSOR_IOCTL_SET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, LSM6DS3_GSENSOR_IOCTL_SET_CALI,
					       (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
			return err;
		}
		break;

	case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_READ_SENSORDATA,
					       (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_SET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_SET_CALI unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_GET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_GET_CALI unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_CLR_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_CLR_CALI unlocked_ioctl failed.");
			return err;
		}
		break;

	default:
		GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;

	}

	return err;
}
#endif

/*----------------------------------------------------------------------------*/
static struct file_operations bmi160_acc_fops = {
	//.owner = THIS_MODULE,
	.open = bmi160_acc_open,
	.release = bmi160_acc_release,
	.unlocked_ioctl = bmi160_acc_unlocked_ioctl,

#ifdef CONFIG_COMPAT
	.compat_ioctl = bmi160_compat_ioctl,
#endif
};
/*----------------------------------------------------------------------------*/
static struct miscdevice bmi160_acc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &bmi160_acc_fops,
};
#endif
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int bmi160_acc_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int err = 0;

	GSE_FUN();

	if(msg.event == PM_EVENT_SUSPEND)
	{
		if(obj == NULL)
		{
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);
		//tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
		/* set driver suspend flag */
		atomic_set(&driver_suspend_flag, 1);
		if (atomic_read(&m_flag) == 1)
		{
			/* set the flag to block e-compass daemon*/
			mutex_lock(&uplink_event_flag_mutex);
			uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
			mutex_unlock(&uplink_event_flag_mutex);
		}
		if (atomic_read(&o_flag) == 1)
		{
			/* set the flag to block e-compass daemon*/
			mutex_lock(&uplink_event_flag_mutex);
			uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
			mutex_unlock(&uplink_event_flag_mutex);
		}
#ifdef BMC050_M4G
		if (atomic_read(&g_flag) == 1)
		{
			/* set the flag to block e-compass daemon*/
			mutex_lock(&uplink_event_flag_mutex);
			uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
			mutex_unlock(&uplink_event_flag_mutex);
		}
#endif //BMC050_M4G
#ifdef BMC050_VRV
		if (atomic_read(&vrv_flag) == 1)
		{
			/* set the flag to block e-compass daemon*/
			mutex_lock(&uplink_event_flag_mutex);
			uplink_event_flag |= BMMDRV_ULEVT_FLAG_VRV_ACTIVE;
			mutex_unlock(&uplink_event_flag_mutex);
		}
#endif //BMC050_VRV
#ifdef BMC050_VLA
		if (atomic_read(&vla_flag) == 1)
		{
			/* set the flag to block e-compass daemon*/
			mutex_lock(&uplink_event_flag_mutex);
			uplink_event_flag |= BMMDRV_ULEVT_FLAG_VLA_ACTIVE;
			mutex_unlock(&uplink_event_flag_mutex);
		}
#endif //BMC050_VLA
#ifdef BMC050_VG
		if (atomic_read(&vg_flag) == 1)
		{
			/* set the flag to block e-compass daemon*/
			mutex_lock(&uplink_event_flag_mutex);
			uplink_event_flag |= BMMDRV_ULEVT_FLAG_VG_ACTIVE;
			mutex_unlock(&uplink_event_flag_mutex);
		}
#endif //BMC050_VG

		/* wake up the wait queue */
		wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND

//tad3sgh add --
		err = BMI160_ACC_SetPowerMode(obj->client, false);
		if(err) {
			GSE_ERR("write power control fail!!\n");
			return err;
		}
		BMI160_ACC_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_resume(struct i2c_client *client)
{
	struct bmi160_acc_i2c_data *obj = obj_i2c_data;
	int err;

	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	BMI160_ACC_power(obj->hw, 1);
	err = bmi160_acc_init_client(client, 0);
	if(err) {
		GSE_ERR("initialize client fail!!\n");
		return err;
	}
	//tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
	/* clear driver suspend flag */
	atomic_set(&driver_suspend_flag, 0);
	if (atomic_read(&m_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
	if (atomic_read(&o_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#ifdef BMC050_M4G
	if (atomic_read(&g_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_M4G
#ifdef BMC050_VRV
	if (atomic_read(&vrv_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_VRV_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_VRV
#ifdef BMC050_VG
	if (atomic_read(&vg_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_VG_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_VG

	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
//tad3sgh add --

	atomic_set(&obj->suspend, 0);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void bmi160_acc_early_suspend(struct early_suspend *h)
{
	struct bmi160_acc_i2c_data *obj = container_of(h, struct bmi160_acc_i2c_data, early_drv);
	int err;

	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1);
//tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
	/* set driver suspend flag */
	atomic_set(&driver_suspend_flag, 1);
	if (atomic_read(&m_flag) == 1)
	{
		/* set the flag to block e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
	if (atomic_read(&o_flag) == 1)
	{
		/* set the flag to block e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#ifdef BMC050_M4G
	if (atomic_read(&g_flag) == 1)
	{
		/* set the flag to block e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_M4G
#ifdef BMC050_VRV
	if (atomic_read(&vrv_flag) == 1)
	{
		/* set the flag to block e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_VRV_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_VRV
#ifdef BMC050_VLA
	if (atomic_read(&vla_flag) == 1)
	{
		/* set the flag to block e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_VLA_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_VLA
#ifdef BMC050_VG
	if (atomic_read(&vg_flag) == 1)
	{
		/* set the flag to block e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_VG_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_VG

	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND

//tad3sgh add --
	err = BMI160_ACC_SetPowerMode(obj->client, false);
	if(err) {
		GSE_ERR("write power control fail!!\n");
		return;
	}

	BMI160_ACC_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void bmi160_acc_late_resume(struct early_suspend *h)
{
	struct bmi160_acc_i2c_data *obj = container_of(h, struct bmi160_acc_i2c_data, early_drv);
	int err;

	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	BMI160_ACC_power(obj->hw, 1);
	err = bmi160_acc_init_client(obj->client, 0);
	if(err) {
		GSE_ERR("initialize client fail!!\n");
		return;
	}
//tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
	/* clear driver suspend flag */
	atomic_set(&driver_suspend_flag, 0);
	if (atomic_read(&m_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
	if (atomic_read(&o_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#ifdef BMC050_M4G
	if (atomic_read(&g_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_M4G
#ifdef BMC050_VRV
	if (atomic_read(&vrv_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_VRV_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_VRV
#ifdef BMC050_VG
	if (atomic_read(&vg_flag) == 1)
	{
		/* set the flag to unblock e-compass daemon*/
		mutex_lock(&uplink_event_flag_mutex);
		uplink_event_flag |= BMMDRV_ULEVT_FLAG_VG_ACTIVE;
		mutex_unlock(&uplink_event_flag_mutex);
	}
#endif //BMC050_VG

	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
//tad3sgh add --
	atomic_set(&obj->suspend, 0);
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/

/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{},
};
#endif

 
static struct i2c_driver bmi160_acc_i2c_driver = {
    .driver = {
        .name           = BMI160_DEV_NAME,

	
	#ifdef CONFIG_OF
	.of_match_table = accel_of_match,
	#endif 
    },
	.probe      		= bmi160_acc_i2c_probe,
	.remove    			= bmi160_acc_i2c_remove,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
    .suspend            = bmi160_acc_suspend,
    .resume             = bmi160_acc_resume,
#endif
	.id_table = bmi160_acc_i2c_id,
};

// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int bmi160_acc_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int bmi160_acc_enable_nodata(int en)
{
#ifdef MISC_FOR_DAEMON
	int err = 0;

	if(((en == 0) && (sensor_power == false))
			||((en == 1) && (sensor_power == true))) {
		GSE_LOG("Gsensor device have updated!\n");
	} else {
		err = BMI160_ACC_SetPowerMode(obj_i2c_data->client, !sensor_power);
	}

	return err;
#else
	int res =0;
	int retry = 0;
	bool power=false;

	if(1==en)
	{
		power=true;
	}
	if(0==en)
	{
		power =false;
	}

	for(retry = 0; retry < 3; retry++){
		res = BMI160_ACC_SetPowerMode(obj_i2c_data->client, power);
		if(res == 0)
		{
			GSE_LOG("BMI160_ACC_SetPowerMode done\n");
			break;
		}
		GSE_LOG("BMI160_ACC_SetPowerMode fail\n");
	}

	if(res != BMI160_ACC_SUCCESS)
	{
		printk("BMI160_ACC_SetPowerMode fail!\n");
		return -1;
	}
	printk("bmi160_acc_enable_nodata OK!\n");
	return 0;
#endif
}

static int bmi160_acc_set_delay(u64 ns)
{
#ifdef MISC_FOR_DAEMON
	int err = 0;
	int value, sample_delay;

	value = (int)ns/1000/1000;
	if(value <= 5) {
		sample_delay = BMI160_ACCEL_ODR_400HZ;
	} else if(value <= 10) {
		sample_delay = BMI160_ACCEL_ODR_200HZ;
	} else {
		sample_delay = BMI160_ACCEL_ODR_100HZ;
	}

	//err = BMI160_ACC_SetBWRate(obj_i2c_data->client, sample_delay);
	if(err != BMI160_ACC_SUCCESS ) {
		GSE_ERR("Set delay parameter error!\n");
	}

	if(value >= 50) {
		atomic_set(&obj_i2c_data->filter, 0);
	} else {
#if defined(CONFIG_BMI160_ACC_LOWPASS)
		obj_i2c_data->fir.num = 0;
		obj_i2c_data->fir.idx = 0;
		obj_i2c_data->fir.sum[BMI160_ACC_AXIS_X] = 0;
		obj_i2c_data->fir.sum[BMI160_ACC_AXIS_Y] = 0;
		obj_i2c_data->fir.sum[BMI160_ACC_AXIS_Z] = 0;
		atomic_set(&obj_i2c_data->filter, 1);
#endif
	}

	return 0;
#else
	int value =0;
	int sample_delay=0;
	int err=0;
	value = (int)ns/1000/1000;
	if(value <= 5)
	{
		sample_delay = BMI160_ACCEL_ODR_400HZ;
	}
	else if(value <= 10)
	{
		sample_delay = BMI160_ACCEL_ODR_200HZ;
	}
	else
	{
		sample_delay = BMI160_ACCEL_ODR_100HZ;
	}

	err = BMI160_ACC_SetBWRate(obj_i2c_data->client, sample_delay);
	if(err != BMI160_ACC_SUCCESS ) //0x2C->BW=100Hz
	{
		GSE_ERR("bmi160_acc_set_delay Set delay parameter error!\n");
		return -1;
	}
	GSE_LOG("bmi160_acc_set_delay (%d)\n",value);
	return 0;
#endif
}

static int bmi160_acc_get_data(int* x ,int* y,int* z, int* status)
{
	char buff[BMI160_BUFSIZE];
	/* use acc raw data for gsensor */
	BMI160_ACC_ReadSensorData(obj_i2c_data->client, buff, BMI160_BUFSIZE);

	sscanf(buff, "%x %x %x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}

int bmi160_m_enable(int en)
{
	if(en == 1) {
		atomic_set(&m_flag, 1);
	} else {
		atomic_set(&m_flag, 0);
	}

	/* set the flag */
	mutex_lock(&uplink_event_flag_mutex);
	uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
	mutex_unlock(&uplink_event_flag_mutex);
	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);

	return 0;
}

int bmi160_m_set_delay(u64 ns)
{
	int value = (int)ns/1000/1000;

	bmm050d_delay = value;
	/* set the flag */
	mutex_lock(&uplink_event_flag_mutex);
	uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_DELAY;
	mutex_unlock(&uplink_event_flag_mutex);
	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);

	return 0;
}

int bmi160_m_open_report_data(int open)
{
	return 0;
}

int bmi160_m_get_data(int* x ,int* y,int* z, int* status)
{
	mutex_lock(&sensor_data_mutex);

	*x = sensor_data[4];
	*y = sensor_data[5];
	*z = sensor_data[6];
	*status = sensor_data[7];

	mutex_unlock(&sensor_data_mutex);

	return 0;
}

int bmi160_o_enable(int en)
{
	if(en == 1) {
		atomic_set(&o_flag, 1);
	} else {
		atomic_set(&o_flag, 0);
	}

	/* set the flag */
	mutex_lock(&uplink_event_flag_mutex);
	uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
	mutex_unlock(&uplink_event_flag_mutex);
	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);

	return 0;
}

int bmi160_o_set_delay(u64 ns)
{
	int value = (int)ns/1000/1000;

	bmm050d_delay = value;
	/* set the flag */
	mutex_lock(&uplink_event_flag_mutex);
	uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_DELAY;
	mutex_unlock(&uplink_event_flag_mutex);
	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);

	return 0;
}

int bmi160_o_open_report_data(int open)
{
	return 0;
}

int bmi160_o_get_data(int* x ,int* y,int* z, int* status)
{
	mutex_lock(&sensor_data_mutex);

	*x = sensor_data[8];
	*y = sensor_data[9];
	*z = sensor_data[10];
	*status = sensor_data[11];

	mutex_unlock(&sensor_data_mutex);

	return 0;
}

/*----------------------------------------------------------------------------*/
static int bmi160_acc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct bmi160_acc_i2c_data *obj;
	struct acc_control_path ctl={0};
	struct acc_data_path data={0};
#ifdef BMC050_VRV
		struct hwmsen_object sobj_vrv;
#endif //BMC050_VRV
#ifdef BMC050_VLA
		struct hwmsen_object sobj_vla;
#endif //BMC050_VLA
#ifdef BMC050_VG
		struct hwmsen_object sobj_vg;
#endif //BMC050_VG //tad3sgh add --

	int err = 0;
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	struct input_dev *dev_interrupt;
#endif

	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct bmi160_acc_i2c_data));
	

	obj->hw = hw;//get_cust_acc_hw();
	GSE_LOG("%s: i2c_number=%d,addr = 0x%x,addr1 = 0x%x\n", __func__, hw->i2c_num,*hw->i2c_addr,*obj->hw->i2c_addr);
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if(err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
       //add cly for i2c set addr follow with dts setting. 0x68
        obj->client->addr = *obj->hw->i2c_addr;
	new_client = obj->client;
	i2c_set_clientdata(new_client,obj);
    
    GSE_LOG("%s:   --new_client_addr=0x%x\n", __func__, client->addr);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	mutex_init(&obj->lock);
	//tad3sgh add ++
	mutex_init(&sensor_data_mutex);
	mutex_init(&uplink_event_flag_mutex);

	init_waitqueue_head(&uplink_event_flag_wq);
	//tad3sgh add --

#ifdef CONFIG_BMI160_ACC_LOWPASS
	if(obj->hw->firlen > C_MAX_FIR_LENGTH)
	{
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	}
	else
	{
		atomic_set(&obj->firlen, obj->hw->firlen);
	}

	if(atomic_read(&obj->firlen) > 0)
	{
		atomic_set(&obj->fir_en, 1);
	}

#endif

	bmi160_acc_i2c_client = new_client;

	err = bmi160_acc_init_client(new_client, 1);
	if(err) {
		goto exit_init_failed;
	}

#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	dev_interrupt = input_allocate_device();
	if (!dev_interrupt) {
		kfree(obj);
		return -ENOMEM;
	}

	/* all interrupt generated events are moved to interrupt input devices*/
	dev_interrupt->name = "bmi160-interrupt";
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
	obj->flat_threshold = 15400*GRAVITY_EARTH_1000/obj->reso->sensitivity;
	obj->aod_flag = 0;
	obj->flat_up_value = 0;
	obj->flat_down_value = 0;
	obj->mEnabled = 0;
	atomic_set(&obj->flat_flag, 0);
	mutex_init(&obj->int_mode_mutex);

	bmi160_aod_set_int_latch(client);
	bmi160_aod_set_int1_pad_sel(client, PAD_FLAT);
	bmi160_aod_config_int1(client);

	bmi160_aod_set_int2_pad_sel(client, PAD_SLOP);
	bmi160_aod_set_int2_pad_sel(client, PAD_SLOW_NO_MOTION);
	bmi160_aod_config_int2(client);

	INIT_WORK(&obj->int1_irq_work, bmi160_aod_int1_irq_work_func);
	INIT_WORK(&obj->int2_irq_work, bmi160_aod_int2_irq_work_func);
	INIT_DELAYED_WORK(&obj->flat_work, bmi160_aod_flat_work_func);
	obj->data_wq = create_freezable_workqueue("bmi160_acc_aod_data_work");
	if (!obj->data_wq) {
		dev_err(&client->dev, "Cannot get create workqueue!\n");
		goto exit_create_aod_workqueue_failed;
	}

	bmi160_setup_int1(bmi160_acc_i2c_client); 
	bmi160_setup_int2(bmi160_acc_i2c_client); 
#endif//CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

#ifdef MISC_FOR_DAEMON
	err = misc_register(&bmi160_acc_device);
	if(err) {
		GSE_ERR("bmi160_acc_device register failed\n");
		goto exit_misc_device_register_failed;
	}
#endif

	err = bmi160_acc_create_attr(&(bmi160_acc_init_info.platform_diver_addr->driver));
	if(err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data= bmi160_acc_open_report_data;
	ctl.enable_nodata = bmi160_acc_enable_nodata;
	ctl.set_delay  = bmi160_acc_set_delay;
	ctl.is_report_input_direct = false;

	err = acc_register_control_path(&ctl);
	if(err)
	{
		GSE_ERR("register acc control path err\n");
		goto exit_kfree;
	}

	data.get_data = bmi160_acc_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if(err)
	{
		GSE_ERR("register acc data path err\n");
		goto exit_kfree;
	}

#ifdef BMC050_VRV
	sobj_vrv.self = obj;
	sobj_vrv.polling = 1;
	sobj_vrv.sensor_operate = bmm050_vrv_operate;
	err = hwmsen_attach(ID_ROTATION_VECTOR, &sobj_vrv);
	if(err) {
		GSE_ERR( "attach fail = %d\n", err);
		goto exit_kfree;
	}
#endif //BMC050_VRV

#ifdef BMC050_VLA
	sobj_vla.self = obj;
	sobj_vla.polling = 1;
	sobj_vla.sensor_operate = bmm050_vla_operate;
	err = hwmsen_attach(ID_LINEAR_ACCELERATION, &sobj_vla);
	if(err) {
		GSE_ERR( "attach fail = %d\n", err);
		goto exit_kfree;
	}
#endif //BMC050_VLA

#ifdef BMC050_VG
	sobj_vg.self = obj;
	sobj_vg.polling = 1;
	sobj_vg.sensor_operate = bmm050_vg_operate;
	err = hwmsen_attach(ID_GRAVITY, &sobj_vg);
	if(err) {
		GSE_ERR( "attach fail = %d\n", err);
		goto exit_kfree;
	}
#endif //BMC050_VG

//add by dingleilei
   #ifdef CONFIG_LCT_DEVINFO_SUPPORT
	devinfo_acc_regchar("BMI160","BOSCH","1.5",DEVINFO_USED);

   #endif
   //end of add


//tad3sgh add --
#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = bmi160_acc_early_suspend,
	obj->early_drv.resume   = bmi160_acc_late_resume,
	register_early_suspend(&obj->early_drv);
#endif

	bmi160_acc_init_flag =0;
	GSE_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
#ifdef MISC_FOR_DAEMON
	misc_deregister(&bmi160_acc_device);
	exit_misc_device_register_failed:
#endif

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
	bmi160_acc_init_flag =-1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int bmi160_acc_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = bmi160_acc_delete_attr(&(bmi160_acc_init_info.platform_diver_addr->driver));
	if(err) {
		GSE_ERR("bma150_delete_attr fail: %d\n", err);
	}

#ifdef MISC_FOR_DAEMON
	err = misc_deregister(&bmi160_acc_device);
	if(err) {
		GSE_ERR("misc_deregister fail: %d\n", err);
	}
#endif


	bmi160_acc_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(obj_i2c_data);
	return 0;
}

static int  bmi160_acc_local_init(void)
{
	//struct acc_hw *hw = get_cust_acc_hw();
	GSE_LOG("fwq loccal init+++\n");

	BMI160_ACC_power(hw, 1);
	if(i2c_add_driver(&bmi160_acc_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	if(-1 == bmi160_acc_init_flag)
	{
		return -1;
	}
	GSE_LOG("fwq loccal init---\n");
	return 0;
}

static int bmi160_acc_remove(void)
{
	//struct acc_hw *hw = get_cust_acc_hw();

	GSE_FUN();
	BMI160_ACC_power(hw, 0);
	i2c_del_driver(&bmi160_acc_i2c_driver);
	return 0;
}

static struct acc_init_info bmi160_acc_init_info = {
	.name = "bmi160_acc",
	.init = bmi160_acc_local_init,
	.uninit = bmi160_acc_remove,
};

/*----------------------------------------------------------------------------*/
static int __init bmi160_acc_init(void)
{
	

	GSE_FUN();

	hw = get_accel_dts_func(COMPATIABLE_NAME, hw);
	if (!hw){
		GSE_ERR("get dts info fail\n");
                return 0; 
        }
	GSE_LOG("%s: i2c_number=%d,addr = 0x%x\n", __func__, hw->i2c_num,*hw->i2c_addr);
/*sku  A01,B01,D01,not support bmi160, return,  D01,E01 support bmi160*/
#ifdef CONFIG_LCT_BOOT_REASON
        sku = lct_get_sku()%5;

        if (0==sku || 1==sku|| 2==sku){
           return 0;
        }
#endif 
/*end*/
	acc_driver_add(&bmi160_acc_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit bmi160_acc_exit(void)
{
	GSE_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(bmi160_acc_init);
module_exit(bmi160_acc_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BMI160_ACC I2C driver");
MODULE_AUTHOR("hongji.zhou@bosch-sensortec.com");

