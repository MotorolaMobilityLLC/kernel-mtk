/* qmcX983.c - qmcX983 compass driver
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


#include <cust_mag.h>
#include "qmcX983.h"

#define Android_Marshmallow				//Android 6.0

#ifndef  Android_Marshmallow
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/completion.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#endif


#define QMCX983_M_NEW_ARCH
#ifdef QMCX983_M_NEW_ARCH
#include "mag.h"
#endif

//#define QST_Dummygyro		   // enable this if you need use 6D gyro

//#define QST_Dummygyro_VirtualSensors	// enable this if you need use Virtual RV/LA/GR sensors

//#define QST_VIRTUAL_SENSOR

/*----------------------------------------------------------------------------*/
#define DEBUG 1
#define QMCX983_DEV_NAME         "qmcX983"
#define DRIVER_VERSION          "3.2"
/*----------------------------------------------------------------------------*/

#define MAX_FAILURE_COUNT	3
#define QMCX983_RETRY_COUNT	3
#define	QMCX983_BUFSIZE		0x20

#define QMCX983_AD0_CMP		1

#define QMCX983_AXIS_X            0
#define QMCX983_AXIS_Y            1
#define QMCX983_AXIS_Z            2
#define QMCX983_AXES_NUM          3

#define QMCX983_DEFAULT_DELAY 100


#define QMC6983_A1_D1             0
#define QMC6983_E1		  1	
#define QMC7983                   2
#define QMC7983_LOW_SETRESET      3




#define CALIBRATION_DATA_SIZE   28

#define MSE_TAG					"[QMC-Msensor] "
#define MSE_FUN(f)				pr_info(MSE_TAG"%s\n", __FUNCTION__)
#define MSE_ERR(fmt, args...)	pr_err(MSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define MSE_LOG(fmt, args...)	pr_info(MSE_TAG fmt, ##args)


static int chip_id = QMC6983_E1;
static struct mag_hw mag_cust;
static struct mag_hw *qst_hw = &mag_cust;
static struct i2c_client *this_client = NULL;
char *msensor_name;    
static short qmcd_delay = QMCX983_DEFAULT_DELAY;

// calibration msensor and orientation data
static int sensor_data[CALIBRATION_DATA_SIZE] = {0};
static struct mutex sensor_data_mutex;
static DEFINE_MUTEX(read_i2c_xyz);

static unsigned char regbuf[2] = {0};

static DECLARE_WAIT_QUEUE_HEAD(data_ready_wq);
static DECLARE_WAIT_QUEUE_HEAD(open_wq);

static atomic_t open_flag = ATOMIC_INIT(0);
static atomic_t m_flag = ATOMIC_INIT(0);
static atomic_t o_flag = ATOMIC_INIT(0);
#ifdef QST_VIRTUAL_SENSOR
static atomic_t g_flag = ATOMIC_INIT(0);
static atomic_t gr_flag = ATOMIC_INIT(0);
static atomic_t la_flag = ATOMIC_INIT(0);
static atomic_t rv_flag = ATOMIC_INIT(0);
#endif

static unsigned char v_open_flag = 0x00;
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static const struct i2c_device_id qmcX983_i2c_id[] = {{QMCX983_DEV_NAME,0},{}};

#ifndef  Android_Marshmallow
static struct i2c_board_info __initdata i2c_qmcX983={ I2C_BOARD_INFO("qmcX983", (0X2c))};
#endif

/*----------------------------------------------------------------------------*/
static int qmcX983_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int qmcX983_i2c_remove(struct i2c_client *client);
static int qmcX983_i2c_detect(struct i2c_client *client, struct i2c_board_info *info); 

#define MTK_I2C_FUNCTION

#ifndef QMCX983_M_NEW_ARCH
static int qmc_probe(struct platform_device *pdev);
static int qmc_remove(struct platform_device *pdev);
#endif

static int qmcX983_suspend(struct device *dev);
static int qmcX983_resume(struct device *dev);


DECLARE_COMPLETION(data_updated);

//add by wangshuai
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
#define SLT_DEVINFO_ACC_DEBUG
#include  "dev_info.h"
//static char* temp_comments;
struct devinfo_struct *s_DEVINFO_mag_qmcX;   
//The followd code is for GTP style
static void devinfo_mag_qmcX_regchar(char *module,char * vendor,char *version,char *used)
{

	s_DEVINFO_mag_qmcX =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_mag_qmcX->device_type="Magnetometer";
	s_DEVINFO_mag_qmcX->device_module=module;
	s_DEVINFO_mag_qmcX->device_vendor=vendor;
	s_DEVINFO_mag_qmcX->device_ic="MMC3630X";
	s_DEVINFO_mag_qmcX->device_info=DEVINFO_NULL;
	s_DEVINFO_mag_qmcX->device_version=version;
	s_DEVINFO_mag_qmcX->device_used=used;
#ifdef SLT_DEVINFO_ACC_DEBUG
		printk("[DEVINFO magnetometer sensor]registe msensor device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s>\n",
				s_DEVINFO_mag_qmcX->device_type,s_DEVINFO_mag_qmcX->device_module,s_DEVINFO_mag_qmcX->device_vendor,
				s_DEVINFO_mag_qmcX->device_ic,s_DEVINFO_mag_qmcX->device_version,s_DEVINFO_mag_qmcX->device_info,s_DEVINFO_mag_qmcX->device_used);
#endif
       DEVINFO_CHECK_DECLARE(s_DEVINFO_mag_qmcX->device_type,s_DEVINFO_mag_qmcX->device_module,s_DEVINFO_mag_qmcX->device_vendor,s_DEVINFO_mag_qmcX->device_ic,s_DEVINFO_mag_qmcX->device_version,s_DEVINFO_mag_qmcX->device_info,s_DEVINFO_mag_qmcX->device_used);
}
      //devinfo_check_add_device(s_DEVINFO_ctp);


#endif

/*----------------------------------------------------------------------------*/
typedef enum {
    QMC_FUN_DEBUG  = 0x01,
	QMC_DATA_DEBUG = 0x02,
	QMC_HWM_DEBUG  = 0x04,
	QMC_CTR_DEBUG  = 0x08,
	QMC_I2C_DEBUG  = 0x10,
} QMC_TRC;

/*----------------------------------------------------------------------------*/
struct qmcX983_i2c_data {
    struct i2c_client *client;
    struct mag_hw *hw;
    atomic_t layout;
    atomic_t trace;
	struct hwmsen_convert   cvt;
	//add for qmcX983 start    for layout direction and M sensor sensitivity------------------------
	short xy_sensitivity;
	short z_sensitivity;
	//add for qmcX983 end-------------------------
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
};

struct delayed_work data_avg_work;
#define DATA_AVG_DELAY 6
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_OF
static const struct of_device_id mag_of_match[] = {
    { .compatible = "mediatek,MSENSOR1"},
    {},
};
#endif
#ifdef CONFIG_PM_SLEEP                                                                                                                                                                                 
static const struct dev_pm_ops qmcx983_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(qmcX983_suspend, qmcX983_resume)
};
#endif
static struct i2c_driver qmcX983_i2c_driver = {
    .driver = {
//      .owner = THIS_MODULE,
        .name  = QMCX983_DEV_NAME,
		
         #ifdef CONFIG_PM_SLEEP
         .pm   = &qmcx983_pm_ops,
         #endif

		#ifdef CONFIG_OF
		.of_match_table = mag_of_match,
		#endif
    },
	.probe      = qmcX983_i2c_probe,
	.remove     = qmcX983_i2c_remove,
	.detect     = qmcX983_i2c_detect,
//#if !defined(CONFIG_HAS_EARLYSUSPEND)
//	.suspend    = qmcX983_suspend,
//	.resume     = qmcX983_resume,
//#endif
	.id_table = qmcX983_i2c_id,
};


#ifdef QMCX983_M_NEW_ARCH
static int qmcX983_local_init(void);
static int qmcX983_remove(void);
static int qmcX983_init_flag =-1; // 0<==>OK -1 <==> fail
static struct mag_init_info qmcX983_init_info = {
        .name = "qmcX983",
        .init = qmcX983_local_init,
        .uninit = qmcX983_remove,
};
#else
#ifdef CONFIG_OF
static const struct of_device_id mmc_of_match[] = {
    { .compatible = "mediatek,msensor", },
    {},
};
#endif

static struct platform_driver qmc_sensor_driver = {
	.probe      = qmc_probe,
	.remove     = qmc_remove,
	.driver     = {
		.name  = "msensor",
        #ifdef CONFIG_OF
        .of_match_table = mmc_of_match,
        #endif
//		.owner = THIS_MODULE,
	}
};
#endif


static int mag_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	int err = 0;
	u8 beg = addr;
	struct i2c_msg msgs[2] = { {0}, {0} };

	mutex_lock(&read_i2c_xyz);
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &beg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	if (!client) {
		mutex_unlock(&read_i2c_xyz);
		return -EINVAL;
	} else if (len > C_I2C_FIFO_SIZE) {
		mutex_unlock(&read_i2c_xyz);
		MSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs) / sizeof(msgs[0]));
	if (err != 2) {
		MSE_ERR("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
		err = -EIO;
	} else {
		err = 0;
	}
	mutex_unlock(&read_i2c_xyz);
	return err;

}

static int mag_i2c_write_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{	
	/*because address also occupies one byte, the maximum length for write is 7 bytes */
	int err = 0, idx = 0, num = 0;
	char buf[C_I2C_FIFO_SIZE];

	err = 0;
	mutex_lock(&read_i2c_xyz);
	if (!client) {
		mutex_unlock(&read_i2c_xyz);
		return -EINVAL;
	} else if (len >= C_I2C_FIFO_SIZE) {
		mutex_unlock(&read_i2c_xyz);
		MSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	num = 0;
	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		mutex_unlock(&read_i2c_xyz);
		MSE_ERR("send command error!!\n");
		return -EFAULT;
	}
	mutex_unlock(&read_i2c_xyz);
	return err;
}


static int I2C_RxData(char *rxData, int length)
{
	
#ifdef MTK_I2C_FUNCTION
	struct i2c_client *client = this_client;
	int res = 0;
	char addr = rxData[0];

	if ((rxData == NULL) || (length < 1))
		return -EINVAL;
	res = mag_i2c_read_block(client, addr, rxData, length);
	if (res < 0)
		return -1;
	return 0;
#else
	uint8_t loop_i;
    int res = 0;
#if DEBUG
	int i;
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
	char addr = rxData[0];
#endif

	/* Caller should check parameter validity.*/
	if((rxData == NULL) || (length < 1))
	{
		return -EINVAL;
	}
	
	mutex_lock(&read_i2c_xyz);

	this_client->addr = (this_client->addr & I2C_MASK_FLAG) | I2C_WR_FLAG;

	for(loop_i = 0; loop_i < QMCX983_RETRY_COUNT; loop_i++)
	{
		res = i2c_master_send(this_client, (const char*)rxData, ((length<<0X08) | 0X01));
		if(res > 0)
		{
			break;
		}
		MSE_ERR("QMCX983 i2c_read retry %d times\n", QMCX983_RETRY_COUNT);
		mdelay(10);
	}
    this_client->addr = this_client->addr & I2C_MASK_FLAG;
	mutex_unlock(&read_i2c_xyz);
	
	if(loop_i >= QMCX983_RETRY_COUNT)
	{
		MSE_ERR("%s retry over %d\n", __func__, QMCX983_RETRY_COUNT);
		return -EIO;
	}
#if DEBUG
	if(atomic_read(&data->trace) & QMC_I2C_DEBUG)
	{
		MSE_LOG("RxData: len=%02x, addr=%02x\n  data=", length, addr);
		for(i = 0; i < length; i++)
		{
			MSE_LOG(" %02x", rxData[i]);
		}
	    MSE_LOG("\n");
	}
#endif
	return 0;
#endif
}

static int I2C_TxData(char *txData, int length)
{	
#ifdef MTK_I2C_FUNCTION
	struct i2c_client *client = this_client;
	int res = 0;
	char addr = txData[0];
	u8 *buff = &txData[1];

	if ((txData == NULL) || (length < 2))
		return -EINVAL;
	res = mag_i2c_write_block(client, addr, buff, (length - 1));
	if (res < 0)
		return -1;
	return 0;

#else	
	uint8_t loop_i;

#if DEBUG
	int i;
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
#endif

	/* Caller should check parameter validity.*/
	if ((txData == NULL) || (length < 2))
	{
		return -EINVAL;
	}

	mutex_lock(&read_i2c_xyz);
	this_client->addr = this_client->addr & I2C_MASK_FLAG;
	for(loop_i = 0; loop_i < QMCX983_RETRY_COUNT; loop_i++)
	{
		if(i2c_master_send(this_client, (const char*)txData, length) > 0)
		{
			break;
		}
		MSE_LOG("I2C_TxData delay!\n");
		mdelay(10);
	}
	mutex_unlock(&read_i2c_xyz);
	
	if(loop_i >= QMCX983_RETRY_COUNT)
	{
		MSE_ERR("%s retry over %d\n", __func__, QMCX983_RETRY_COUNT);
		return -EIO;
	}
#if DEBUG
	if(atomic_read(&data->trace) & QMC_I2C_DEBUG)
	{
		MSE_LOG("TxData: len=%02x, addr=%02x\n  data=", length, txData[0]);
		for(i = 0; i < (length-1); i++)
		{
			MSE_LOG(" %02x", txData[i + 1]);
		}
		MSE_LOG("\n");
	}
#endif
	return 0;
#endif
}


/* X,Y and Z-axis magnetometer data readout
 * param *mag pointer to \ref QMCX983_t structure for x,y,z data readout
 * note data will be read by multi-byte protocol into a 6 byte structure
 */

static int qmcX983_read_mag_xyz(int *data)
{
	int res;
	unsigned char mag_data[6];
	unsigned char databuf[6];
	int hw_d[3] = {0};

	int t1 = 0;
	unsigned char rdy = 0;
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *clientdata = i2c_get_clientdata(client);
	int i;

    MSE_FUN();

	/* Check status register for data availability */
	while(!(rdy & 0x07) && t1<3){
		databuf[0]=STA_REG_ONE;
		res=I2C_RxData(databuf,1);
		rdy=databuf[0];
		MSE_LOG("QMCX983 Status register is (%02X)\n", rdy);
		t1 ++;
	}

	//MSE_LOG("QMCX983 read mag_xyz begin\n");

	//mutex_lock(&read_i2c_xyz);

	databuf[0] = OUT_X_L;

	res = I2C_RxData(databuf, 6);
	if(res != 0)
    {
		//mutex_unlock(&read_i2c_xyz);
		return -EFAULT;
	}
	for(i=0;i<6;i++)
		mag_data[i]=databuf[i];
	//mutex_unlock(&read_i2c_xyz);

	MSE_LOG("QMCX983 mag_data[%02x, %02x, %02x, %02x, %02x, %02x]\n",
		mag_data[0], mag_data[1], mag_data[2],
		mag_data[3], mag_data[4], mag_data[5]);

	hw_d[0] = (short) (((mag_data[1]) << 8) | mag_data[0]);
	hw_d[1] = (short) (((mag_data[3]) << 8) | mag_data[2]);
	hw_d[2] = (short) (((mag_data[5]) << 8) | mag_data[4]);

	hw_d[0] = hw_d[0] * 1000 / clientdata->xy_sensitivity;
	hw_d[1] = hw_d[1] * 1000 / clientdata->xy_sensitivity;
	hw_d[2] = hw_d[2] * 1000 / clientdata->z_sensitivity;

	MSE_LOG("Hx=%d, Hy=%d, Hz=%d\n",hw_d[0],hw_d[1],hw_d[2]);

	data[QMCX983_AXIS_X] = clientdata->cvt.sign[QMCX983_AXIS_X]*hw_d[clientdata->cvt.map[QMCX983_AXIS_X]];
	data[QMCX983_AXIS_Y] = clientdata->cvt.sign[QMCX983_AXIS_Y]*hw_d[clientdata->cvt.map[QMCX983_AXIS_Y]];
	data[QMCX983_AXIS_Z] = clientdata->cvt.sign[QMCX983_AXIS_Z]*hw_d[clientdata->cvt.map[QMCX983_AXIS_Z]];

	MSE_LOG("QMCX983 data [%d, %d, %d] _A\n", data[0], data[1], data[2]);
	return res;
}



/* Set the Gain range */
int qmcX983_set_range(short range)
{
	int err = 0;
	unsigned char data[2];
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(this_client);

	int ran ;
	switch (range) {
	case QMCX983_RNG_2G:
		ran = RNG_2G;
		break;
	case QMCX983_RNG_8G:
		ran = RNG_8G;
		break;
	case QMCX983_RNG_12G:
		ran = RNG_12G;
		break;
	case QMCX983_RNG_20G:
		ran = RNG_20G;
		break;
	default:
		return -EINVAL;
	}

	obj->xy_sensitivity = 20000/ran;
	obj->z_sensitivity = 20000/ran;

	data[0] = CTL_REG_ONE;
	err = I2C_RxData(data, 1);

	data[0] &= 0xcf;
	data[0] |= (range << 4);
	data[1] = data[0];

	data[0] = CTL_REG_ONE;
	err = I2C_TxData(data, 2);
	return err;

}

/* Set the sensor mode */
int qmcX983_set_mode(char mode)
{
	int err = 0;

	unsigned char data[2];
	data[0] = CTL_REG_ONE;
	err = I2C_RxData(data, 1);

	data[0] &= 0xfc;
	data[0] |= mode;
	data[1] = data[0];

	data[0] = CTL_REG_ONE;
	MSE_LOG("QMCX983 in qmcX983_set_mode, data[1] = [%02x]", data[1]);
	err = I2C_TxData(data, 2);

	return err;
}

int qmcX983_set_ratio(char ratio)
{
	int err = 0;

	unsigned char data[2];
	data[0] = 0x0b;//RATIO_REG;
	data[1] = ratio;
	err = I2C_TxData(data, 2);
	return err;
}

static void qmcX983_start_measure(struct i2c_client *client)
{

	unsigned char data[2];
	int err;

	data[1] = 0x1d;
	data[0] = CTL_REG_ONE;
	err = I2C_TxData(data, 2);

}

static void qmcX983_stop_measure(struct i2c_client *client)
{

	unsigned char data[2];
	int err;

	data[1] = 0x1c;
	data[0] = CTL_REG_ONE;
	err = I2C_TxData(data, 2);
}

static int qmcX983_enable(struct i2c_client *client)
{

	#if 1  // change the peak to 1us from 2 us 
	unsigned char data[2];
	int err;

	data[1] = 0x1;
	data[0] = 0x21;
	err = I2C_TxData(data, 2);

	data[1] = 0x40;
	data[0] = 0x20;
	err = I2C_TxData(data, 2);

    //For E1 & 7983, enable chip filter & set fastest set_reset
	if(chip_id == QMC6983_E1 || chip_id == QMC7983 || chip_id == QMC7983_LOW_SETRESET)
	{

		data[1] = 0x80;
		data[0] = 0x29;
		err = I2C_TxData(data, 2); 		

		data[1] = 0x0c;
		data[0] = 0x0a;
		err = I2C_TxData(data, 2);				
	}
	
	#endif
	
	MSE_LOG("start measure!\n");
	qmcX983_start_measure(client);

	qmcX983_set_range(QMCX983_RNG_8G);
	qmcX983_set_ratio(QMCX983_SETRESET_FREQ_FAST);				//the ratio must not be 0, different with qmc5983


	return 0;
}

static int qmcX983_disable(struct i2c_client *client)
{
	MSE_LOG("stop measure!\n");
	qmcX983_stop_measure(client);

	return 0;
}

/*----------------------------------------------------------------------------*/
static atomic_t dev_open_count;

#ifdef CONFIG_HAS_EARLYSUSPEND
static int qmcX983_SetPowerMode(struct i2c_client *client, bool enable)
{
	if(enable == true)
	{
		if(qmcX983_enable(client))
		{
			MSE_LOG("qmcX983: set power mode failed!\n");
			return -EINVAL;
		}
		else
		{
			MSE_LOG("qmcX983: set power mode enable ok!\n");
		}
	}
	else
	{
		if(qmcX983_disable(client))
		{
			MSE_LOG("qmcX983: set power mode failed!\n");
			return -EINVAL;
		}
		else
		{
			MSE_LOG("qmcX983: set power mode disable ok!\n");
		}
	}

	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static void qmcX983_power(struct mag_hw *hw, unsigned int on)
{
#ifndef  Android_Marshmallow

	static unsigned int power_on = 0;
	
#ifdef __USE_LINUX_REGULATOR_FRAMEWORK__

#else
	if(hw->power_id != MT65XX_POWER_NONE)
	{
		MSE_LOG("power %s\n", on ? "on" : "off");
		if(power_on == on)
		{
			MSE_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "qmcX983"))
			{
				MSE_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "qmcX983"))
			{
				MSE_ERR("power off fail!!\n");
			}
		}
	}
#endif
	power_on = on;
	
#endif
}

// Daemon application save the data
static int QMC_SaveData(int *buf)
{
#if DEBUG
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
#endif

	mutex_lock(&sensor_data_mutex);
	memcpy(sensor_data, buf, sizeof(sensor_data));
	
	mutex_unlock(&sensor_data_mutex);

#if DEBUG
	if((data != NULL) && (atomic_read(&data->trace) & QMC_DATA_DEBUG)){
		MSE_LOG("Get daemon data: %d, %d, %d, %d\n %d, %d, %d, %d\n %d, %d, %d, %d\n %d, %d, %d, %d\n %d, %d, %d, %d\n %d, %d, %d, %d\n %d, %d, %d, %d\n",
			sensor_data[0],sensor_data[1],sensor_data[2],sensor_data[3],
			sensor_data[4],sensor_data[5],sensor_data[6],sensor_data[7],
			sensor_data[8],sensor_data[9],sensor_data[10],sensor_data[11],
			sensor_data[12],sensor_data[13],sensor_data[14],sensor_data[15],
			sensor_data[16],sensor_data[17],sensor_data[18],sensor_data[19],
			sensor_data[20],sensor_data[21],sensor_data[22],sensor_data[23],
			sensor_data[24],sensor_data[25],sensor_data[26],sensor_data[27]);
	}
#endif

	return 0;

}
//TODO
static int QMC_GetOpenStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) != 0));
	return atomic_read(&open_flag);
}
static int QMC_GetCloseStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) <= 0));
	return atomic_read(&open_flag);
}

/*----------------------------------------------------------------------------*/
static int qmcX983_ReadChipInfo(char *buf, int bufsize)
{
	if((!buf)||(bufsize <= QMCX983_BUFSIZE -1))
	{
		return -EINVAL;
	}
	if(!this_client)
	{
		*buf = 0;
		return -EINVAL;
	}
	if(chip_id == QMC7983)
	{
	   snprintf(buf, bufsize, "qmc7983 Chip");	
	}
	else if(chip_id == QMC7983_LOW_SETRESET)
	{
	   snprintf(buf, bufsize, "qmc7983 LOW SETRESET Chip");
	}
	else if(chip_id == QMC6983_E1)
	{
	   snprintf(buf, bufsize, "qmc6983 E1 Chip");
	}
	else if(chip_id == QMC6983_A1_D1)
	{
	   snprintf(buf, bufsize, "qmc6983 A1/D1 Chip");	
	}	
	
	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[QMCX983_BUFSIZE];
	qmcX983_ReadChipInfo(strbuf, QMCX983_BUFSIZE);
	return scnprintf(buf, sizeof(strbuf), "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	int sensordata[3];

	qmcX983_read_mag_xyz(sensordata);

	return scnprintf(buf, PAGE_SIZE, "%d %d %d\n", sensordata[0],sensordata[1],sensordata[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_posturedata_value(struct device_driver *ddri, char *buf)
{
	int tmp[3];

	tmp[0] = sensor_data[9] * CONVERT_O / CONVERT_O_DIV;
	tmp[1] = sensor_data[10] * CONVERT_O / CONVERT_O_DIV;
	tmp[2] = sensor_data[11] * CONVERT_O / CONVERT_O_DIV;
	
	return scnprintf(buf, PAGE_SIZE, "%d, %d, %d\n", tmp[0],tmp[1], tmp[2]);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);

	return scnprintf(buf, PAGE_SIZE, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		data->hw->direction,atomic_read(&data->layout),	data->cvt.sign[0], data->cvt.sign[1],
		data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
	int layout = 0;

	if(1 == sscanf(buf, "%d", &layout))
	{
		atomic_set(&data->layout, layout);
		if(!hwmsen_get_convert(layout, &data->cvt))
		{
			MSE_ERR("HWMSEN_GET_CONVERT function error!\r\n");
		}
		else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
		{
			MSE_ERR("invalid layout: %d, restore to %d\n", layout, data->hw->direction);
		}
		else
		{
			MSE_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	}
	else
	{
		MSE_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;

	if(data->hw)
	{
		len += scnprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n",
			data->hw->i2c_num, data->hw->direction, data->hw->power_id, data->hw->power_vol);
	}
	else
	{
		len += scnprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}

	len += scnprintf(buf+len, PAGE_SIZE-len, "OPEN: %d\n", atomic_read(&dev_open_count));
	len += scnprintf(buf+len, PAGE_SIZE-len, "open_flag = 0x%x, v_open_flag=0x%x\n",
			atomic_read(&open_flag), v_open_flag);
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(this_client);
	
	if(NULL == obj)
	{
		MSE_ERR("qmcX983_i2c_data is null!!\n");
		return -EINVAL;
	}

	res = scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(this_client);
	int trace;
	if(NULL == obj)
	{
		MSE_ERR("qmcX983_i2c_data is null!!\n");
		return -EINVAL;
	}

	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}
	else
	{
		MSE_ERR("invalid content: '%s', length = %zd\n", buf, count);
	}

	return count;
}
static ssize_t show_daemon_name(struct device_driver *ddri, char *buf)
{
	char strbuf[QMCX983_BUFSIZE];
	snprintf(strbuf, sizeof(strbuf), "qmcX983d");
	return scnprintf(buf, sizeof(strbuf), "%s", strbuf);
}
static ssize_t show_temperature_value(struct device_driver *ddri, char *buf)
{
	int res;

	unsigned char mag_temperature[2];
	unsigned char databuf[2];
	int hw_temperature=0;

	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *clientdata = i2c_get_clientdata(client);

	if ((clientdata != NULL) && (atomic_read(&clientdata->trace) & QMC_FUN_DEBUG))
		MSE_FUN();

	//mutex_lock(&read_i2c_temperature);

	databuf[0] = TEMP_L_REG;
	res = I2C_RxData(databuf, 1);
	if(res != 0){
		//mutex_unlock(&read_i2c_temperature);
		return -EFAULT;
	}
	mag_temperature[0]=databuf[0];

	databuf[0] = TEMP_H_REG;
	res = I2C_RxData(databuf, 1);
	if(res != 0){
		//mutex_unlock(&read_i2c_temperature);
		return -EFAULT;
	}
	mag_temperature[1]=databuf[0];

	//mutex_unlock(&read_i2c_temperature);

	if ((clientdata != NULL) && (atomic_read(&clientdata->trace) & QMC_FUN_DEBUG)){
		MSE_LOG("QMCX983 read_i2c_temperature[%02x, %02x]\n",
			mag_temperature[0], mag_temperature[1]);
	}
	hw_temperature = ((mag_temperature[1]) << 8) | mag_temperature[0];

	if ((clientdata != NULL) && (atomic_read(&clientdata->trace) & QMC_FUN_DEBUG))
		MSE_LOG("QMCX983 temperature = %d\n",hw_temperature);  

	return scnprintf(buf, PAGE_SIZE, "temperature = %d\n", hw_temperature);
}
static ssize_t show_WRregisters_value(struct device_driver *ddri, char *buf)
{
	int res;

	unsigned char databuf[2];

//	struct i2c_client *client = this_client;
	//struct qmcX983_i2c_data *clientdata = i2c_get_clientdata(client);

	MSE_FUN();

	databuf[0] = regbuf[0];
	res = I2C_RxData(databuf, 1);
	if(res != 0){
		return -EFAULT;
	}
			
	MSE_LOG("QMCX983 hw_registers = 0x%02x\n",databuf[0]);  

	return scnprintf(buf, PAGE_SIZE, "hw_registers = 0x%02x\n", databuf[0]);
}

static ssize_t store_WRregisters_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(this_client);
	unsigned char tempbuf[1] = {0};
	unsigned char data[2] = {0};
	int err = 0;
	if(NULL == obj)
	{
		MSE_ERR("qmcX983_i2c_data is null!!\n");
		return -EINVAL;
	}
	tempbuf[0] = *buf;
	MSE_ERR("QMC6938:store_WRregisters_value: 0x%2x \n", tempbuf[0]);
	data[1] = tempbuf[0];
	data[0] = regbuf[0];
	err = I2C_TxData(data, 2);
	if(err != 0)
	   MSE_ERR("QMC6938: write registers 0x%2x  ---> 0x%2x success! \n", regbuf[0],tempbuf[0]);

	return count;
}

static ssize_t show_registers_value(struct device_driver *ddri, char *buf)
{
	MSE_LOG("QMCX983 hw_registers = 0x%02x\n",regbuf[0]);  
	   
	return scnprintf(buf, PAGE_SIZE, "hw_registers = 0x%02x\n", regbuf[0]);
}
static ssize_t store_registers_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(this_client);

	if(NULL == obj)
	{
		MSE_ERR("qmcX983_i2c_data is null!!\n");
		return -EINVAL;
	}
	regbuf[0] = *buf;
	MSE_ERR("QMC6938: REGISTERS = 0x%2x\n", regbuf[0]);
	return count;
}

static ssize_t show_dumpallreg_value(struct device_driver *ddri, char *buf)
{
	int res;
	int i =0;
	char strbuf[300];
	char tempstrbuf[24];
	unsigned char databuf[2];
	int length=0;

	//struct i2c_client *client = this_client;
	//struct qmcX983_i2c_data *clientdata = i2c_get_clientdata(client);

	MSE_FUN();

	/* Check status register for data availability */	
	for(i =0;i<12;i++)
	{
		databuf[0] = i;
		res = I2C_RxData(databuf, 1);
		if(res < 0)
			MSE_LOG("QMCX983 dump registers 0x%02x failed !\n", i);

		length = scnprintf(tempstrbuf, sizeof(tempstrbuf), "reg[0x%2x] =  0x%2x \n",i, databuf[0]);
		snprintf(strbuf+length*i, sizeof(strbuf)-length*i, "  %s \n",tempstrbuf);
	}

	return scnprintf(buf, sizeof(strbuf), "%s\n", strbuf);
}

static int FctShipmntTestProcess_Body(void)
{
	return 1;
}

static ssize_t store_shipment_test(struct device_driver * ddri,const char * buf, size_t count)
{
	return count;            
}

static ssize_t show_shipment_test(struct device_driver *ddri, char *buf)
{
	char result[10];
	int res = 0;
	res = FctShipmntTestProcess_Body();
	if(1 == res)
	{
	   MSE_LOG("shipment_test pass\n");
	   strcpy(result,"y");
	}
	else if(-1 == res)
	{
	   MSE_LOG("shipment_test fail\n");
	   strcpy(result,"n");
	}
	else
	{
	  MSE_LOG("shipment_test NaN\n");
	  strcpy(result,"NaN");
	}
	
	return sprintf(buf, "%s\n", result);        
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(shipmenttest,S_IRUGO | S_IWUSR, show_shipment_test, store_shipment_test);
static DRIVER_ATTR(dumpallreg,  S_IRUGO , show_dumpallreg_value, NULL);
static DRIVER_ATTR(WRregisters, S_IRUGO | S_IWUSR, show_WRregisters_value, store_WRregisters_value);
static DRIVER_ATTR(registers,   S_IRUGO | S_IWUSR, show_registers_value, store_registers_value);
static DRIVER_ATTR(temperature, S_IRUGO, show_temperature_value, NULL);
static DRIVER_ATTR(daemon,      S_IRUGO, show_daemon_name, NULL);
static DRIVER_ATTR(chipinfo,    S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata,  S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(posturedata, S_IRUGO, show_posturedata_value, NULL);
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value);
static DRIVER_ATTR(status,      S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(trace,       S_IRUGO | S_IWUSR, show_trace_value, store_trace_value);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *qmcX983_attr_list[] = {
	&driver_attr_shipmenttest,
	&driver_attr_dumpallreg,
    &driver_attr_WRregisters,
	&driver_attr_registers,
    &driver_attr_temperature,
    &driver_attr_daemon,
	&driver_attr_chipinfo,
	&driver_attr_sensordata,
	&driver_attr_posturedata,
	&driver_attr_layout,
	&driver_attr_status,
	&driver_attr_trace,
};
/*----------------------------------------------------------------------------*/
static int qmcX983_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)ARRAY_SIZE(qmcX983_attr_list);
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		err = driver_create_file(driver, qmcX983_attr_list[idx]);
		if(err < 0)
		{
			MSE_ERR("driver_create_file (%s) = %d\n", qmcX983_attr_list[idx]->attr.name, err);
			break;
		}
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int qmcX983_delete_attr(struct device_driver *driver)
{
	int idx;
	int num = (int)ARRAY_SIZE(qmcX983_attr_list);

	if(driver == NULL)
	{
		return -EINVAL;
	}


	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, qmcX983_attr_list[idx]);
	}

	return 0;
}


/*----------------------------------------------------------------------------*/
static int qmcX983_open(struct inode *inode, struct file *file)
{
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(this_client);
	int ret = -1;

	if(atomic_read(&obj->trace) & QMC_CTR_DEBUG)
	{
		MSE_LOG("Open device node:qmcX983\n");
	}
	ret = nonseekable_open(inode, file);

	return ret;
}
/*----------------------------------------------------------------------------*/
static int qmcX983_release(struct inode *inode, struct file *file)
{
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(this_client);
	atomic_dec(&dev_open_count);
	if(atomic_read(&obj->trace) & QMC_CTR_DEBUG)
	{
		MSE_LOG("Release device node:qmcX983\n");
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static long qmcX983_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	/* NOTE: In this function the size of "char" should be 1-byte. */
	char buff[QMCX983_BUFSIZE];				/* for chip information */
	char rwbuf[16]; 		/* for READ/WRITE */
	int value[CALIBRATION_DATA_SIZE];			/* for SET_YPR */
	int delay;			/* for GET_DELAY */
	int status; 			/* for OPEN/CLOSE_STATUS */
	int ret =-1;				
	short sensor_status;		/* for Orientation and Msensor status */
	unsigned char data[16] = {0};
	int vec[3] = {0};
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *clientdata = i2c_get_clientdata(client);
	struct hwm_sensor_data osensor_data;
	uint32_t enable;

	int err;

	if ((clientdata != NULL) && (atomic_read(&clientdata->trace) & QMC_FUN_DEBUG))
		MSE_LOG("qmcX983_unlocked_ioctl !cmd= 0x%x\n", cmd);
	
	switch (cmd){
	case QMC_IOCTL_WRITE:
		if(argp == NULL)
		{
			MSE_LOG("invalid argument.");
			return -EINVAL;
		}
		if(copy_from_user(rwbuf, argp, sizeof(rwbuf)))
		{
			MSE_LOG("copy_from_user failed.");
			return -EFAULT;
		}

		if((rwbuf[0] < 2) || (rwbuf[0] > (RWBUF_SIZE-1)))
		{
			MSE_LOG("invalid argument.");
			return -EINVAL;
		}
		ret = I2C_TxData(&rwbuf[1], rwbuf[0]);
		if(ret < 0)
		{
			return ret;
		}
		break;
			
	case QMC_IOCTL_READ:
		if(argp == NULL)
		{
			MSE_LOG("invalid argument.");
			return -EINVAL;
		}
		
		if(copy_from_user(rwbuf, argp, sizeof(rwbuf)))
		{
			MSE_LOG("copy_from_user failed.");
			return -EFAULT;
		}

		if((rwbuf[0] < 1) || (rwbuf[0] > (RWBUF_SIZE-1)))
		{
			MSE_LOG("invalid argument.");
			return -EINVAL;
		}
		ret = I2C_RxData(&rwbuf[1], rwbuf[0]);
		if (ret < 0)
		{
			return ret;
		}
		if(copy_to_user(argp, rwbuf, rwbuf[0]+1))
		{
			MSE_LOG("copy_to_user failed.");
			return -EFAULT;
		}
		break;

	case QMCX983_SET_RANGE:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			#if DEBUG
			MSE_ERR("copy_from_user error\n");
			#endif
			return -EFAULT;
		}
		err = qmcX983_set_range(*data);
		return err;

	case QMCX983_SET_MODE:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			#if DEBUG
			MSE_ERR("copy_from_user error\n");
			#endif
			return -EFAULT;
		}
		err = qmcX983_set_mode(*data);
		return err;


	case QMCX983_READ_MAGN_XYZ:
		if(argp == NULL){
			MSE_ERR("IO parameter pointer is NULL!\r\n");
			break;
		}

		err = qmcX983_read_mag_xyz(vec);

		MSE_LOG("mag_data[%d, %d, %d]\n",
				vec[0],vec[1],vec[2]);
			if(copy_to_user(argp, vec, sizeof(vec)))
			{
				return -EFAULT;
			}
			break;

/*------------------------------for daemon------------------------*/
	case QMC_IOCTL_SET_YPR:
		if(argp == NULL)
		{
			MSE_LOG("invalid argument.");
			return -EINVAL;
		}
		if(copy_from_user(value, argp, sizeof(value)))
		{
			MSE_LOG("copy_from_user failed.");
			return -EFAULT;
		}
		QMC_SaveData(value);
		break;

	case QMC_IOCTL_GET_OPEN_STATUS:
		status = QMC_GetOpenStatus();
		if(copy_to_user(argp, &status, sizeof(status)))
		{
			MSE_LOG("copy_to_user failed.");
			return -EFAULT;
		}
		break;

	case QMC_IOCTL_GET_CLOSE_STATUS:
		
		status = QMC_GetCloseStatus();
		if(copy_to_user(argp, &status, sizeof(status)))
		{
			MSE_LOG("copy_to_user failed.");
			return -EFAULT;
		}
		break;

	case QMC_IOC_GET_MFLAG:
		sensor_status = atomic_read(&m_flag);
		if(copy_to_user(argp, &sensor_status, sizeof(sensor_status)))
		{
			MSE_LOG("copy_to_user failed.");
			return -EFAULT;
		}
		break;

	case QMC_IOC_GET_OFLAG:
		sensor_status = atomic_read(&o_flag);
		if(copy_to_user(argp, &sensor_status, sizeof(sensor_status)))
		{
			MSE_LOG("copy_to_user failed.");
			return -EFAULT;
		}
		break;

	case QMC_IOCTL_GET_DELAY:
	    delay = qmcd_delay;
	    if (copy_to_user(argp, &delay, sizeof(delay))) {
	         MSE_LOG("copy_to_user failed.");
	         return -EFAULT;
	    }
	    break;
	/*-------------------------for ftm------------------------**/

	case MSENSOR_IOCTL_READ_CHIPINFO:       //reserved?
		if(argp == NULL)
		{
			MSE_ERR("IO parameter pointer is NULL!\r\n");
			break;
		}

		qmcX983_ReadChipInfo(buff, QMCX983_BUFSIZE);
		if(copy_to_user(argp, buff, strlen(buff)+1))
		{
			return -EFAULT;
		}
		break;

	case MSENSOR_IOCTL_READ_SENSORDATA:	//for daemon
		if(argp == NULL)
		{
			MSE_LOG("IO parameter pointer is NULL!\r\n");
			break;
		}

		qmcX983_read_mag_xyz(vec);

		if ((clientdata != NULL) && (atomic_read(&clientdata->trace) & QMC_DATA_DEBUG))
			MSE_LOG("mag_data[%d, %d, %d]\n",vec[0],vec[1],vec[2]);
		
		snprintf(buff, sizeof(buff), "%x %x %x", vec[0], vec[1], vec[2]);
		if(copy_to_user(argp, buff, strlen(buff)+1))
		{
			return -EFAULT;
		}

			break;

	case MSENSOR_IOCTL_SENSOR_ENABLE:

		if(argp == NULL)
		{
			MSE_ERR("IO parameter pointer is NULL!\r\n");
			break;
		}
		if(copy_from_user(&enable, argp, sizeof(enable)))
		{
			MSE_LOG("copy_from_user failed.");
			return -EFAULT;
		}
		else
		{
			if(enable == 1)
			{
				atomic_set(&m_flag, 1);
				v_open_flag |= 0x01;
				/// we start measurement at here
				//qmcX983_start_measure(this_client);
				qmcX983_enable(this_client);
			}
			else
			{
				atomic_set(&m_flag, 0);
				v_open_flag &= 0x3e;
			}
			// check we need stop sensor or not
			if(v_open_flag==0)
				qmcX983_disable(this_client);

			atomic_set(&open_flag, v_open_flag);

			wake_up(&open_wq);

			MSE_ERR("qmcX983 v_open_flag = 0x%x,open_flag= 0x%x\n",v_open_flag, atomic_read(&open_flag));
		}
		break;

	case MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:
		if(argp == NULL)
		{
			MSE_ERR("IO parameter pointer is NULL!\r\n");
			break;
		}


		mutex_lock(&sensor_data_mutex);

		osensor_data.values[0] = sensor_data[8];
		osensor_data.values[1] = sensor_data[9];
		osensor_data.values[2] = sensor_data[10];
		osensor_data.status = sensor_data[11];
		osensor_data.value_divide = CONVERT_O_DIV;

		mutex_unlock(&sensor_data_mutex);

		snprintf(buff, sizeof(buff), "%x %x %x %x %x", osensor_data.values[0], osensor_data.values[1],
			osensor_data.values[2],osensor_data.status,osensor_data.value_divide);
		if(copy_to_user(argp, buff, strlen(buff)+1))
		{
			return -EFAULT;
		}

		break;

	default:
		MSE_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
		return -ENOIOCTLCMD;
		break;
	}

	return 0;
}

#ifdef CONFIG_COMPAT
static long qmcX983_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;
	void __user *arg64 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl) {
		MSE_ERR("");
	}

	switch (cmd) {
	/* ================================================================ */
	case COMPAT_QMC_IOCTL_WRITE:
		err = file->f_op->unlocked_ioctl(file, QMC_IOCTL_WRITE, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOCTL_WRITE execute failed! err = %ld\n", err);
		break;

	/* ================================================================ */
	case COMPAT_QMC_IOCTL_READ:
		err = file->f_op->unlocked_ioctl(file, QMC_IOCTL_READ, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOCTL_READ execute failed! err = %ld\n", err);
		break;

	case COMPAT_QMCX983_SET_RANGE:
		err = file->f_op->unlocked_ioctl(file, QMCX983_SET_RANGE, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMCX983_SET_RANGE execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_QMCX983_SET_MODE:
		err = file->f_op->unlocked_ioctl(file, QMCX983_SET_MODE, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMCX983_SET_MODE execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_QMCX983_READ_MAGN_XYZ:
		err = file->f_op->unlocked_ioctl(file, QMCX983_READ_MAGN_XYZ, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMCX983_READ_MAGN_XYZ execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_QMC_IOCTL_SET_YPR:
		err = file->f_op->unlocked_ioctl(file, QMC_IOCTL_SET_YPR, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOCTL_SET_YPR execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_QMC_IOCTL_GET_OPEN_STATUS:
		err = file->f_op->unlocked_ioctl(file, QMC_IOCTL_GET_OPEN_STATUS, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOCTL_GET_OPEN_STATUS execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_QMC_IOCTL_GET_CLOSE_STATUS:
		err = file->f_op->unlocked_ioctl(file, QMC_IOCTL_GET_CLOSE_STATUS, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOCTL_GET_CLOSE_STATUS execute failed! err = %ld\n", err);

		break;

	case COMPAT_QMC_IOC_GET_MFLAG:
		err = file->f_op->unlocked_ioctl(file, QMC_IOC_GET_MFLAG, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOC_GET_MFLAG execute failed! err = %ld\n", err);

		break;

	case COMPAT_QMC_IOC_GET_OFLAG:
		err = file->f_op->unlocked_ioctl(file, QMC_IOC_GET_OFLAG, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOC_GET_OFLAG execute failed! err = %ld\n", err);

		break;

	case COMPAT_QMC_IOCTL_GET_DELAY:
		err = file->f_op->unlocked_ioctl(file, QMC_IOCTL_GET_DELAY, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("QMC_IOCTL_GET_DELAY execute failed! err = %ld\n", err);
		break;

	case COMPAT_MSENSOR_IOCTL_READ_CHIPINFO:
		err = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_READ_CHIPINFO, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("MSENSOR_IOCTL_READ_CHIPINFO execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_MSENSOR_IOCTL_READ_SENSORDATA:
		err = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_READ_SENSORDATA, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("MSENSOR_IOCTL_READ_SENSORDATA execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_MSENSOR_IOCTL_SENSOR_ENABLE:
		err = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_SENSOR_ENABLE, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("MSENSOR_IOCTL_SENSOR_ENABLE execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	case COMPAT_MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:
		err = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_READ_FACTORY_SENSORDATA, (unsigned long)arg64);
		if (err < 0)
			MSE_ERR("MSENSOR_IOCTL_READ_FACTORY_SENSORDATA execute failed! err = %ld\n", err);

		break;

	/* ================================================================ */
	default :
		MSE_ERR("ERR: 0x%4x CMD not supported!", cmd);
		return (-ENOIOCTLCMD);

		break;
	}

	return err;
}

#endif
/*----------------------------------------------------------------------------*/
static struct file_operations qmcX983_fops = {
//	.owner = THIS_MODULE,
	.open = qmcX983_open,
	.release = qmcX983_release,
	.unlocked_ioctl = qmcX983_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = qmcX983_compat_ioctl,
#endif
};
/*----------------------------------------------------------------------------*/
static struct miscdevice qmcX983_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "msensor",
    .fops = &qmcX983_fops,
};
/*----------------------------------------------------------------------------*/
#ifdef QMCX983_M_NEW_ARCH
static int qmcX983_m_open_report_data(int en)
{
	return 0;
}
static int qmcX983_m_set_delay(u64 delay)
{
	int value = (int)delay/1000/1000;
	struct qmcX983_i2c_data *data = NULL;

	if (unlikely(this_client == NULL)) {
		MSE_ERR("this_client is null!\n");
		return -EINVAL;
	}
		
	data = i2c_get_clientdata(this_client);
	if (unlikely(data == NULL)) {
		MSE_ERR("data is null!\n");
		return -EINVAL;
	}

	if(value <= 10)
		qmcd_delay = 10;

	qmcd_delay = value;

	return 0;
}
static int qmcX983_m_enable(int en)
{
	struct qmcX983_i2c_data *data = NULL;

	if (unlikely(this_client == NULL)) {
		MSE_ERR("this_client is null!\n");
		return -EINVAL;
	}
		
	data = i2c_get_clientdata(this_client);
	if (unlikely(data == NULL)) {
		MSE_ERR("data is null!\n");
		return -EINVAL;
	}

	if(en == 1)
	{
		atomic_set(&m_flag, 1);
		v_open_flag |= 0x01;
		/// we start measurement at here 
		//qmcX983_start_measure(this_client);
		qmcX983_enable(this_client);
	}
	else
	{
		atomic_set(&m_flag, 0);
		v_open_flag &= 0x3e;
	}
	// check we need stop sensor or not 
	if(v_open_flag==0)
		qmcX983_disable(this_client);
		
	atomic_set(&open_flag, v_open_flag);
	
	wake_up(&open_wq);

	MSE_ERR("qmcX983 v_open_flag = 0x%x,open_flag= 0x%x\n",v_open_flag, atomic_read(&open_flag));
	return 0;
}
static int qmcX983_o_open_report_data(int en)
{
	return 0;
}
static int qmcX983_o_set_delay(u64 delay)
{
	int value = (int)delay/1000/1000;
	struct qmcX983_i2c_data *data = NULL;

	if (unlikely(this_client == NULL)) {
		MSE_ERR("this_client is null!\n");
		return -EINVAL;
	}
		
	data = i2c_get_clientdata(this_client);
	if (unlikely(data == NULL)) {
		MSE_ERR("data is null!\n");
		return -EINVAL;
	}

	if(value <= 10)
		qmcd_delay = 10;

	qmcd_delay = value;


	return 0;
}
static int qmcX983_o_enable(int en)
{
	struct qmcX983_i2c_data *data = NULL;

	if (unlikely(this_client == NULL)) {
		MSE_ERR("this_client is null!\n");
		return -EINVAL;
	}
		
	data = i2c_get_clientdata(this_client);
	if (unlikely(data == NULL)) {
		MSE_ERR("data is null!\n");
		return -EINVAL;
	}

	if(en == 1)
	{
		atomic_set(&o_flag, 1);
		v_open_flag |= 0x02;
		
		/// we start measurement at here 
		//qmcX983_start_measure(this_client);
		qmcX983_enable(this_client);
	}
	else 
	{
			   
		atomic_set(&o_flag, 0);
		v_open_flag &= 0x3d;
	  
	}	
	// check we need stop sensor or not 
	if(v_open_flag==0)
		qmcX983_disable(this_client);
		
	atomic_set(&open_flag, v_open_flag);
	
	wake_up(&open_wq);
	MSE_ERR("qmcX983 v_open_flag = 0x%x,open_flag= 0x%x\n",v_open_flag, atomic_read(&open_flag));

	return 0;
}

static int qmcX983_o_get_data(int *x, int *y, int *z, int *status)
{
	struct qmcX983_i2c_data *data = NULL;

	if (unlikely(this_client == NULL)) {
		MSE_ERR("this_client is null!\n");
		return -EINVAL;
	}
		
	data = i2c_get_clientdata(this_client);
	if (unlikely(data == NULL)) {
		MSE_ERR("data is null!\n");
		return -EINVAL;
	}

	mutex_lock(&sensor_data_mutex);

	*x = sensor_data[8] * CONVERT_O;
	*y = sensor_data[9] * CONVERT_O;
	*z = sensor_data[10] * CONVERT_O;
	*status = sensor_data[11];

	mutex_unlock(&sensor_data_mutex);
#if DEBUG
	if(atomic_read(&data->trace) & QMC_HWM_DEBUG)
	{ 
		MSE_LOG("Hwm get o-sensor data: %d, %d, %d,status %d!\n", *x, *y, *z, *status);
	}
#endif

	return 0;
}
static int qmcX983_m_get_data(int *x, int *y, int *z, int *status)
{
	struct qmcX983_i2c_data *data = NULL;

	if (unlikely(this_client == NULL)) {
		MSE_ERR("this_client is null!\n");
		return -EINVAL;
	}
		
	data = i2c_get_clientdata(this_client);
	if (unlikely(data == NULL)) {
		MSE_ERR("data is null!\n");
		return -EINVAL;
	}

	mutex_lock(&sensor_data_mutex);

	*x = sensor_data[4] * CONVERT_M;
	*y = sensor_data[5] * CONVERT_M;
	*z = sensor_data[6] * CONVERT_M;
	*status = sensor_data[7];

	mutex_unlock(&sensor_data_mutex);
#if DEBUG
	if(atomic_read(&data->trace) & QMC_HWM_DEBUG)
	{				
		MSE_LOG("Hwm get m-sensor data: %d, %d, %d,status %d!\n", *x, *y, *z, *status);
	}		
#endif

	return 0;
}

#ifdef QST_VIRTUAL_SENSOR
static int qmcX983_gs_enable(int en)
{
	int value = 0;
	value = en;

	if(value == 1)
	{
		atomic_set(&g_flag, 1);
		v_open_flag |= 0x04;
		// we start measurement at here 
		qmcX983_enable(this_client);	
	}
	else 
	{
		 atomic_set(&g_flag, 0);
		 v_open_flag &= 0x3b;
	}			
	MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);
	// check we need stop sensor or not 
	if(v_open_flag==0)
	{
		qmcX983_disable(this_client);	
	}	
	atomic_set(&open_flag, v_open_flag);
	wake_up(&open_wq);
	return 0;
}
static int qmcX983_gs_set_delay(u64 ns)
{
	int value = 0;
	value = (int)ns/1000/1000;
	if(value <= 10)
	{
		qmcd_delay = 10;	
	}
	else
	{
		qmcd_delay = value;
	}
	return 0;
}

static int qmcX983_gs_open_report_data(int open)
{
	return 0;
}
static int qmcX983_gs_get_data(int* x ,int* y,int* z, int* status)
{
	mutex_lock(&sensor_data_mutex);
	
	*x = sensor_data[12] * CONVERT_GS;
	*y = sensor_data[13] * CONVERT_GS;
	*z = sensor_data[14] * CONVERT_GS;
	*status = sensor_data[7];
		
	mutex_unlock(&sensor_data_mutex);	
	return 0;
}
static int qmcX983_rv_enable(int en)
{
	int value = 0;
	
	value = en;
	if(value == 1)
	{
		atomic_set(&rv_flag, 1);
		v_open_flag |= 0x08; 
		// we start measurement at here 
		qmcX983_enable(this_client);						 
	}
	else 
	{
		atomic_set(&rv_flag, 0);
		v_open_flag &= 0x37;
	}	
	MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);
	// check we need stop sensor or not 
	if(v_open_flag==0)
	{
		qmcX983_disable(this_client);
	}
	atomic_set(&open_flag, v_open_flag);
	wake_up(&open_wq);
	return 0;
}
static int qmcX983_rv_set_delay(u64 ns)
{
	int value = 0;
	value = (int)ns/1000/1000;
	if(value <= 10)
	{
		qmcd_delay = 10;	
	}
	else
	{
		qmcd_delay = value;
	}
	return 0;
}
static int qmcX983_rv_open_report_data(int open)
{
	return 0;
}
static int qmcX983_rv_get_data(int* x ,int* y,int* z, int* status)
{
	mutex_lock(&sensor_data_mutex);
	
	*x = sensor_data[17] * CONVERT_RV;
	*y = sensor_data[18] * CONVERT_RV;
	*z = sensor_data[19] * CONVERT_RV;
	*status = sensor_data[7];
		
	mutex_unlock(&sensor_data_mutex);		
	return 0;
}
static int qmcX983_gr_enable(int en)
{
	int value = 0;
	
	value = en;
	if(value == 1)
	{
		atomic_set(&gr_flag, 1);
		v_open_flag |= 0x10;
		// we start measurement at here 
		qmcX983_enable(this_client);	
	}
	else 
	{
		atomic_set(&gr_flag, 0);
		v_open_flag &= 0x2f;
	}	
	MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);
	// check we need stop sensor or not 
	if(v_open_flag==0)
	{
		qmcX983_disable(this_client);
	}
	atomic_set(&open_flag, v_open_flag);
	wake_up(&open_wq);
	return 0;
}
static int qmcX983_gr_set_delay(u64 ns)
{
	int value = 0;
	value = (int)ns/1000/1000;
	if(value <= 10)
	{
		qmcd_delay = 10;	
	}
	else
	{
		qmcd_delay = value;
	}
	return 0;
}
static int qmcX983_gr_open_report_data(int open)
{
	return 0;
}
static int qmcX983_gr_get_data(int* x ,int* y,int* z, int* status)
{
	mutex_lock(&sensor_data_mutex);

	*x = sensor_data[20] * CONVERT_GR;
	*y = sensor_data[21] * CONVERT_GR;
	*z = sensor_data[22] * CONVERT_GR;
	*status = sensor_data[7];	
	
	mutex_unlock(&sensor_data_mutex);
	
	return 0;
}
static int qmcX983_la_enable(int en)
{
	int value = 0;
	
	value = en;
	if(value == 1)
	{
		atomic_set(&la_flag, 1);
		v_open_flag |= 0x20;
		/// we start measurement at here 
		qmcX983_enable(this_client);	
	}

	else 
	{
		atomic_set(&la_flag, 0);
		v_open_flag &= 0x1f;
	}	

	MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);

	// check we need stop sensor or not 
	if(v_open_flag==0)
	{
		qmcX983_disable(this_client);
	}
	atomic_set(&open_flag, v_open_flag);
	wake_up(&open_wq);
	return 0;
}
static int qmcX983_la_set_delay(u64 ns)
{
	int value = 0;
	value = (int)ns/1000/1000;
	if(value <= 10)
	{
		qmcd_delay = 10;	
	}
	else
	{
		qmcd_delay = value;
	}
	return 0;
}
static int qmcX983_la_open_report_data(int open)
{
	return 0;
}
static int qmcX983_la_get_data(int* x ,int* y,int* z, int* status)
{
	mutex_lock(&sensor_data_mutex);
				
	*x = sensor_data[24] * CONVERT_LA;
	*y = sensor_data[25] * CONVERT_LA;
	*z = sensor_data[26] * CONVERT_LA;
	*status = sensor_data[7];
					
	mutex_unlock(&sensor_data_mutex);
	return 0;
}

#endif //QST_VIRTUAL_SENSOR end
#else
int qmcX983_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* msensor_data;

#if DEBUG
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);

	if((data != NULL) && (atomic_read(&data->trace) & QMC_HWM_DEBUG))
		MSE_LOG("qmcX983_operate cmd = %d",command);
#endif

	switch (command)
	{
		case SENSOR_DELAY:
			MSE_ERR("qmcX983 Set delay !\n");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value <= 10)
				{
					qmcd_delay = 10;
				}
				qmcd_delay = value;
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;

				if(value == 1)
				{
					atomic_set(&m_flag, 1);
					v_open_flag |= 0x01;	
					/// we start measurement at here 		
					qmcX983_enable(this_client);	
				}
				else
				{
					atomic_set(&m_flag, 0);
					v_open_flag &= 0x3e;
				}

				// check we need stop sensor or not 
				if(v_open_flag==0)
					qmcX983_disable(this_client);
					
				atomic_set(&open_flag, v_open_flag);
				
				wake_up(&open_wq);
				
				MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);

				// TODO: turn device into standby or normal mode
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				MSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				msensor_data = (struct hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);

				msensor_data->values[0] = sensor_data[4] * CONVERT_M;
				msensor_data->values[1] = sensor_data[5] * CONVERT_M;
				msensor_data->values[2] = sensor_data[6] * CONVERT_M;
				msensor_data->status = sensor_data[7];
				msensor_data->value_divide = CONVERT_M_DIV;

				mutex_unlock(&sensor_data_mutex);
#if DEBUG
			if((data != NULL) && (atomic_read(&data->trace) & QMC_DATA_DEBUG))
				MSE_LOG("Hwm get m-sensor data: %d, %d, %d. divide %d, status %d!\n",
						msensor_data->values[0],msensor_data->values[1],msensor_data->values[2],
						msensor_data->value_divide,msensor_data->status);
				
#endif
			}
			break;
		default:
			MSE_ERR("msensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}

/*----------------------------------------------------------------------------*/
int qmcX983_orientation_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* osensor_data;
#if DEBUG
	struct i2c_client *client = this_client;
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
#endif

#if DEBUG
	if((data != NULL) && (atomic_read(&data->trace) & QMC_FUN_DEBUG))
		MSE_FUN();

#endif

	switch (command)
	{
		case SENSOR_DELAY:
			MSE_ERR("qmcX983 oir Set delay !\n");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value <= 10)
				{
					qmcd_delay = 10;
				}
				qmcd_delay = value;
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;

				if(value == 1)
				{
					atomic_set(&o_flag, 1);
					v_open_flag |= 0x02;		
				  // we start measurement at here 
					qmcX983_enable(this_client);	
				}
				else 
				{
                 		   
					atomic_set(&o_flag, 0);
					v_open_flag &= 0x3d;
                  
				}	
				MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);

				// check we need stop sensor or not 
				if(v_open_flag==0)
					qmcX983_disable(this_client);
					
					
				atomic_set(&open_flag, v_open_flag);
				
				wake_up(&open_wq);
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				MSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				osensor_data = (struct hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);

				osensor_data->values[0] = sensor_data[8] * CONVERT_O;
				osensor_data->values[1] = sensor_data[9] * CONVERT_O;
				osensor_data->values[2] = sensor_data[10] * CONVERT_O;
				osensor_data->status = sensor_data[11];
				osensor_data->value_divide = CONVERT_O_DIV;

				mutex_unlock(&sensor_data_mutex);
#if DEBUG
			if((data != NULL) && (atomic_read(&data->trace) & QMC_DATA_DEBUG))
				MSE_LOG("Hwm get o-sensor data: %d, %d, %d. divide %d, status %d!\n",
					osensor_data->values[0],osensor_data->values[1],osensor_data->values[2],
					osensor_data->value_divide,osensor_data->status);
#endif
			}
			break;
		default:
			MSE_ERR("gsensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
#endif

#ifdef QST_Dummygyro
/*----------------------------------------------------------------------------*/
int qmcX983_gyroscope_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* gyrosensor_data;	
	
#if DEBUG	
	struct i2c_client *client = this_client;  
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);

	if((data != NULL) && (atomic_read(&data->trace) & QMC_FUN_DEBUG))
		MSE_FUN();

#endif

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
	
				qmcd_delay = 10;  // fix to 100Hz
			}	
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
			//	MSE_ERR("qmcX983_gyroscope_operate SENSOR_ENABLE=%d  mEnabled=%d  gEnabled = %d\n",value,mEnabled,gEnabled);
				
				if(value == 1)
				{
					 atomic_set(&g_flag, 1);
					 v_open_flag |= 0x04;
					// we start measurement at here 
					qmcX983_enable(this_client);	
				}

				else 
				{

					 atomic_set(&g_flag, 0);
					 v_open_flag &= 0x3b;
                   
				}	
			
				MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);

				// check we need stop sensor or not 
				if(v_open_flag==0)
					qmcX983_disable(this_client);	
					
							
				atomic_set(&open_flag, v_open_flag);

				 
				wake_up(&open_wq);
			}
				
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				MSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				gyrosensor_data = (struct hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);
				
				gyrosensor_data->values[0] = sensor_data[12] * CONVERT_Q16;
				gyrosensor_data->values[1] = sensor_data[13] * CONVERT_Q16;
				gyrosensor_data->values[2] = sensor_data[14] * CONVERT_Q16;
				gyrosensor_data->status = sensor_data[15];
				gyrosensor_data->value_divide = CONVERT_Q16_DIV;
					
				mutex_unlock(&sensor_data_mutex);
#if DEBUG
				if((data != NULL) && (atomic_read(&data->trace) & QMC_DATA_DEBUG))
					MSE_LOG("Hwm get gyro-sensor data: %d, %d, %d. divide %d, status %d!\n",
						gyrosensor_data->values[0],gyrosensor_data->values[1],gyrosensor_data->values[2],
						gyrosensor_data->value_divide,gyrosensor_data->status);
				
#endif
			}
			break;
		default:
			MSE_ERR("gyrosensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

#ifdef QST_Dummygyro_VirtualSensors
/*----------------------------------------------------------------------------*/
int qmcX983_rotation_vector_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* RV_data;	
#if DEBUG	
	struct i2c_client *client = this_client;  
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
	
	if((data != NULL) && (atomic_read(&data->trace) & QMC_FUN_DEBUG))
		MSE_FUN();

#endif

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				qmcd_delay = 10; // fix to 100Hz
			}	
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				//MSE_ERR("qmcX983_rotation_vector_operate SENSOR_ENABLE=%d  mEnabled=%d\n",value,mEnabled);
				
				value = *(int *)buff_in;

				if(value == 1)
				{
					 atomic_set(&rv_flag, 1);
					 v_open_flag |= 0x08; 
					// we start measurement at here 
					//qmcX983_start_measure(this_client);
					qmcX983_enable(this_client);						 
			    }

				else 
				{
					 atomic_set(&rv_flag, 0);
					 v_open_flag &= 0x37;
				}	
			
				MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);

				// check we need stop sensor or not 
				if(v_open_flag==0)
						qmcX983_disable(this_client);
						
				atomic_set(&open_flag, v_open_flag);


				wake_up(&open_wq);
			}
				
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				MSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				RV_data = (struct hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);
				
				RV_data->values[0] = sensor_data[16] * CONVERT_Q16;
				RV_data->values[1] = sensor_data[17] * CONVERT_Q16;
				RV_data->values[2] = sensor_data[18] * CONVERT_Q16;
				RV_data->values[3] = sensor_data[19] * CONVERT_Q16;//old arch ,only updata 3 elemements		
				
				RV_data->status = sensor_data[7] ; //sensor_data[19];  fix w-> 0 w
				
				RV_data->value_divide = CONVERT_Q16_DIV;
					
				mutex_unlock(&sensor_data_mutex);
#if DEBUG
			if((data != NULL) && (atomic_read(&data->trace) & QMC_DATA_DEBUG))
			{
				MSE_LOG("Hwm get rv-sensor data: %d, %d, %d. divide %d, status %d!\n",
					RV_data->values[0],RV_data->values[1],RV_data->values[2],
					RV_data->value_divide,RV_data->status);
			}	
#endif
			}
			break;
		default:
			MSE_ERR("RV  operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}


/*----------------------------------------------------------------------------*/
int qmcX983_gravity_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* gravity_data;	
#if DEBUG	
	struct i2c_client *client = this_client;  
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
	
	if((data != NULL) && (atomic_read(&data->trace) & QMC_FUN_DEBUG))
		MSE_FUN();	
#endif

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value <= 10)
				{
					value = 10;
				}
				qmcd_delay = value;
			}	
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{

				//MSE_ERR("qmcX983_gravity_operate SENSOR_ENABLE=%d  mEnabled=%d\n",value,mEnabled);
				
				value = *(int *)buff_in;
				
				if(value == 1)
				{
					 atomic_set(&gr_flag, 1);
					 v_open_flag |= 0x10;
					// we start measurement at here 
					 qmcX983_enable(this_client);	
				}
			
				else 
				{

					  atomic_set(&gr_flag, 0);
					  v_open_flag &= 0x2f;

				}	
			
			MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);

			// check we need stop sensor or not 
			if(v_open_flag==0)
					qmcX983_disable(this_client);
					
			atomic_set(&open_flag, v_open_flag);

				wake_up(&open_wq);
			}
				
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				MSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				gravity_data = (struct hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);
				
				gravity_data->values[0] = sensor_data[20] * CONVERT_Q16;
				gravity_data->values[1] = sensor_data[21] * CONVERT_Q16;
				gravity_data->values[2] = sensor_data[22] * CONVERT_Q16;
				gravity_data->status = sensor_data[7];
				gravity_data->value_divide = CONVERT_Q16_DIV;
					
				mutex_unlock(&sensor_data_mutex);
#if DEBUG
			if((data != NULL) && (atomic_read(&data->trace) & QMC_DATA_DEBUG))
			{
				MSE_LOG("Hwm get gravity-sensor data: %d, %d, %d. divide %d, status %d!\n",
					gravity_data->values[0],gravity_data->values[1],gravity_data->values[2],
					gravity_data->value_divide,gravity_data->status);
			}	
#endif
			}
			break;
		default:
			MSE_ERR("gravity operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}


/*----------------------------------------------------------------------------*/
int qmcX983_linear_accelration_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* LA_data;	
#if DEBUG	
	struct i2c_client *client = this_client;  
	struct qmcX983_i2c_data *data = i2c_get_clientdata(client);
	
	if((data != NULL) && (atomic_read(&data->trace) & QMC_FUN_DEBUG))
		MSE_FUN();	

#endif

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value <= 10)
				{
					value = 10;
				}
				qmcd_delay = value;
			}	
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				MSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{

				//MSE_ERR("qmcX983_linear_accelration_operate SENSOR_ENABLE=%d  mEnabled=%d\n",value,mEnabled);
				
				value = *(int *)buff_in;
				
				if(value == 1)
				{
					 atomic_set(&la_flag, 1);
					 v_open_flag |= 0x20;
					/// we start measurement at here 
					 qmcX983_enable(this_client);	
				}
				
				else 
				{

					atomic_set(&la_flag, 0);
					v_open_flag &= 0x1f;
                 
				}	
			
				MSE_ERR("qmcX983 v_open_flag = %x\n",v_open_flag);

				// check we need stop sensor or not 
				if(v_open_flag==0)
						qmcX983_disable(this_client);
						
						
				atomic_set(&open_flag, v_open_flag);
				
				wake_up(&open_wq);
			}
				
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				MSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				LA_data = (struct hwm_sensor_data *)buff_out;
				mutex_lock(&sensor_data_mutex);
				
				LA_data->values[0] = sensor_data[24] * CONVERT_Q16;
				LA_data->values[1] = sensor_data[25] * CONVERT_Q16;
				LA_data->values[2] = sensor_data[26] * CONVERT_Q16;
				LA_data->status = sensor_data[7];
				LA_data->value_divide = CONVERT_Q16_DIV;
					
				mutex_unlock(&sensor_data_mutex);
#if DEBUG
			if((data != NULL) && (atomic_read(&data->trace) & QMC_DATA_DEBUG))
			{
				MSE_LOG("Hwm get LA-sensor data: %d, %d, %d. divide %d, status %d!\n",
					LA_data->values[0],LA_data->values[1],LA_data->values[2],
					LA_data->value_divide,LA_data->status);
			}	
#endif
			}
			break;
		default:
			MSE_ERR("linear_accelration operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

#endif
#endif

/*----------------------------------------------------------------------------*/
#ifndef	CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int qmcX983_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev); 
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(client);


//	if(msg.event == PM_EVENT_SUSPEND)
//	{
		qmcX983_power(obj->hw, 0);
//	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int qmcX983_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev); 
	struct qmcX983_i2c_data *obj = i2c_get_clientdata(client);
	
	qmcX983_power(obj->hw, 1);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void qmcX983_early_suspend(struct early_suspend *h)
{
	struct qmcX983_i2c_data *obj = container_of(h, struct qmcX983_i2c_data, early_drv);

	if(NULL == obj)
	{
		MSE_ERR("null pointer!!\n");
		return;
	}
	
	qmcX983_power(obj->hw, 0);
	
	if(qmcX983_SetPowerMode(obj->client, false))
	{
		MSE_LOG("qmcX983: write power control fail!!\n");
		return;
	}

}
/*----------------------------------------------------------------------------*/
static void qmcX983_late_resume(struct early_suspend *h)
{
	struct qmcX983_i2c_data *obj = container_of(h, struct qmcX983_i2c_data, early_drv);

	if(NULL == obj)
	{
		MSE_ERR("null pointer!!\n");
		return;
	}

	qmcX983_power(obj->hw, 1);
	
	/// we can not start measurement , because we have no single measurement mode 

}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
static int qmcX983_i2c_detect(struct i2c_client *client, struct i2c_board_info *info) 
{    
	strcpy(info->type, QMCX983_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int qmcX983_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct qmcX983_i2c_data *data = NULL;

	int err = 0;
	unsigned char databuf[6] = {0,0,0,0,0,0};
#ifdef QMCX983_M_NEW_ARCH
	struct mag_control_path ctl = {0};
	struct mag_data_path mag_data = {0};
#else
	struct hwmsen_object sobj_m, sobj_o;
#endif
	
	MSE_FUN();

//	printk("qmc7983, %s",__func__);

	data = kmalloc(sizeof(struct qmcX983_i2c_data), GFP_KERNEL);
	if(data == NULL)
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(data, 0, sizeof(struct qmcX983_i2c_data));


	data->hw = qst_hw;

	if (hwmsen_get_convert(data->hw->direction, &data->cvt)) {
        	MSE_ERR("QMCX983 invalid direction: %d\n", data->hw->direction);
        	goto exit_kfree;
	}

	atomic_set(&data->layout, data->hw->direction);
	atomic_set(&data->trace, 0);

	mutex_init(&sensor_data_mutex);
	mutex_init(&read_i2c_xyz);
	//mutex_init(&read_i2c_temperature);
	//mutex_init(&read_i2c_register);

	init_waitqueue_head(&data_ready_wq);
	init_waitqueue_head(&open_wq);
//	printk("qmc7983, i2c_addr=%d\n",client->addr);

	client->addr = 0x2d;

	data->client = client;
	new_client = data->client;
	i2c_set_clientdata(new_client, data);

	this_client = new_client;

	/* read chip id */
	databuf[0] = 0x0d;
	if(I2C_RxData(databuf, 1)<0)
	{
		MSE_ERR("QMCX983 I2C_RxData error!\n");
		goto exit_kfree;
	}

	if (databuf[0] == 0xff )
	{
		chip_id = QMC6983_A1_D1;
		MSE_LOG("QMCX983 I2C driver registered!\n");
	}
	else if(databuf[0] == 0x31) //qmc7983 asic 
	{
	    	databuf[0] = 0x3E;
		if(I2C_RxData(databuf, 1)<0)
		{
			MSE_ERR("QMCX983 I2C_RxData error!\n");
			goto exit_kfree;
		}
		if((databuf[0] & 0x20) == 1)
			chip_id = QMC6983_E1;
		else if((databuf[0] & 0x20) == 0)
			chip_id = QMC7983;
			
		MSE_LOG("QMCX983 I2C driver registered!\n");
	}
	else if(databuf[0] == 0x32) //qmc7983 asic low setreset
	{
			chip_id = QMC7983_LOW_SETRESET;

		MSE_LOG("QMCX983 I2C driver registered!\n");
	}
	else 
	{
		MSE_LOG("QMCX983 check ID faild!\n");
		goto exit_kfree;
	}


	/* Register sysfs attribute */
#ifdef QMCX983_M_NEW_ARCH
	err = qmcX983_create_attr(&(qmcX983_init_info.platform_diver_addr->driver));
#else
	err  = qmcX983_create_attr(&qmc_sensor_driver.driver);
#endif
	if (err < 0)
	{
		MSE_ERR("create attribute err = %d\n", err);
		goto exit_sysfs_create_group_failed;
	}

	err = misc_register(&qmcX983_device);

	if(err < 0)
	{
		MSE_ERR("qmcX983_device register failed\n");
		goto exit_misc_device_register_failed;	
	}
#ifdef QMCX983_M_NEW_ARCH
	ctl.m_enable = qmcX983_m_enable;
	ctl.m_set_delay = qmcX983_m_set_delay;
	ctl.m_open_report_data = qmcX983_m_open_report_data;
	ctl.o_enable = qmcX983_o_enable;
	ctl.o_set_delay = qmcX983_o_set_delay;
	ctl.o_open_report_data = qmcX983_o_open_report_data;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = data->hw->is_batch_supported;
#ifdef QST_VIRTUAL_SENSOR
	ctl.gs_enable = qmcX983_gs_enable;
	ctl.gs_set_delay  = qmcX983_gs_set_delay;
	ctl.gs_open_report_data = qmcX983_gs_open_report_data;
	ctl.rv_enable = qmcX983_rv_enable;
	ctl.rv_set_delay  = qmcX983_rv_set_delay;
	ctl.rv_open_report_data = qmcX983_rv_open_report_data;
	ctl.gr_enable = qmcX983_gr_enable;
	ctl.gr_set_delay  = qmcX983_gr_set_delay;
	ctl.gr_open_report_data = qmcX983_gr_open_report_data;
	ctl.la_enable = qmcX983_la_enable;
	ctl.la_set_delay  = qmcX983_la_set_delay;
	ctl.la_open_report_data = qmcX983_la_open_report_data;
#endif //end of QST_VIRTUAL_SENSOR

	err = mag_register_control_path(&ctl);
	if (err) {
		MAG_ERR("register mag control path err\n");
		goto exit_hwm_attach_failed;
	}

	mag_data.div_m = CONVERT_M_DIV;
	mag_data.div_o = CONVERT_O_DIV;
#ifdef QST_VIRTUAL_SENSOR
	mag_data.div_gs = CONVERT_GS_DIV;
	mag_data.div_rv = CONVERT_RV_DIV;
	mag_data.div_gr = CONVERT_GR_DIV;
	mag_data.div_la = CONVERT_LA_DIV;
#endif

	mag_data.get_data_o = qmcX983_o_get_data;
	mag_data.get_data_m = qmcX983_m_get_data;
#ifdef QST_VIRTUAL_SENSOR
	mag_data.get_data_gs = qmcX983_gs_get_data;
	mag_data.get_data_rv = qmcX983_rv_get_data;
	mag_data.get_data_gr = qmcX983_gr_get_data;
	mag_data.get_data_la = qmcX983_la_get_data;
#endif

	err = mag_register_data_path(&mag_data);
	if (err){
		MAG_ERR("register data control path err\n");
		goto exit_hwm_attach_failed;
	}

#else

	sobj_m.self = data;
	sobj_m.polling = 1;
	sobj_m.sensor_operate = qmcX983_operate;
	
	err = hwmsen_attach(ID_MAGNETIC, &sobj_m);

	if(err < 0)
	{
		MSE_ERR("attach fail = %d\n", err);
		goto exit_hwm_attach_failed;
	}

	sobj_o.self = data;
	sobj_o.polling = 1;
	sobj_o.sensor_operate = qmcX983_orientation_operate;

	err = hwmsen_attach(ID_ORIENTATION, &sobj_o);

	if(err < 0)
	{
		MSE_ERR("attach fail = %d\n", err);
		goto exit_hwm_attach_failed;
	}
#endif

//add by wangshuai
   #ifdef CONFIG_LCT_DEVINFO_SUPPORT
	devinfo_mag_qmcX_regchar("qmcX983","xirui","null",DEVINFO_USED);

   #endif
   //end of add

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	data->early_drv.suspend  = qmcX983_early_suspend,
	data->early_drv.resume   = qmcX983_late_resume,
	register_early_suspend(&data->early_drv);
#endif
	
#ifdef QMCX983_M_NEW_ARCH
    qmcX983_init_flag = 1;
#endif
	MSE_LOG("%s: OK\n", __func__);
	return 0;

exit_hwm_attach_failed:
	misc_deregister(&qmcX983_device);
exit_misc_device_register_failed:
#ifdef QMCX983_M_NEW_ARCH
	qmcX983_delete_attr(&(qmcX983_init_info.platform_diver_addr->driver));
#else
	qmcX983_delete_attr(&qmc_sensor_driver.driver);
#endif	
exit_sysfs_create_group_failed:
exit_kfree:
	kfree(data);
exit:
	MSE_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int qmcX983_i2c_remove(struct i2c_client *client)
{
	int err;

#ifdef QMCX983_M_NEW_ARCH
	err = qmcX983_delete_attr(&(qmcX983_init_info.platform_diver_addr->driver));
#else
	err  = qmcX983_delete_attr(&qmc_sensor_driver.driver);
#endif
	if (err < 0)
	{
		MSE_ERR("qmcX983_delete_attr fail: %d\n", err);
	}

	this_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	misc_deregister(&qmcX983_device);
	return 0;
}
#ifdef QMCX983_M_NEW_ARCH
static int qmcX983_local_init(void)
{
	qmcX983_power(qst_hw, 1);

	atomic_set(&dev_open_count, 0);

	if(i2c_add_driver(&qmcX983_i2c_driver))
	{
		MSE_ERR("add driver error\n");
		return -EINVAL;
	}

    if(-1 == qmcX983_init_flag)
    {
        MSE_ERR("%s failed!\n",__func__);
        return -EINVAL;
    }

	return 0;
}

static int qmcX983_remove(void)
{

	qmcX983_power(qst_hw, 0);
	atomic_set(&dev_open_count, 0);
	i2c_del_driver(&qmcX983_i2c_driver);
	return 0;
}
#else
/*----------------------------------------------------------------------------*/
static int qmc_probe(struct platform_device *pdev)
{


	qmcX983_power(qst_hw, 1);
	MSE_FUN();
	atomic_set(&dev_open_count, 0);

	if(i2c_add_driver(&qmcX983_i2c_driver))
	{
		MSE_ERR("add driver error\n");
		return -EINVAL;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int qmc_remove(struct platform_device *pdev)
{


	qmcX983_power(qst_hw, 0);
	atomic_set(&dev_open_count, 0);
	i2c_del_driver(&qmcX983_i2c_driver);
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/



static int __init qmcX983_init(void)
{
#ifdef Android_Marshmallow
	const char *name = "mediatek,qmcX983";
	qst_hw = get_mag_dts_func(name, qst_hw);
	printk("qmcX983_init addr0 = 0x%x,addr1 = 0x%x,i2c_num = %d \n",qst_hw->i2c_addr[0],qst_hw->i2c_addr[1],qst_hw->i2c_num);
	if (!qst_hw) {
		printk("get_mag_dts_func failed !\n");
		return -ENODEV;
	}
#else
	qst_hw = get_cust_mag_hw();
	i2c_register_board_info(hw->i2c_num, &i2c_qmcX983, 1);
#endif	
#ifdef QMCX983_M_NEW_ARCH
    mag_driver_add(&qmcX983_init_info);
#else
	if(platform_driver_register(&qmc_sensor_driver))
	{
		MSE_ERR("failed to register driver");
		return -ENODEV;
	}
#endif

	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit qmcX983_exit(void)
{
#ifndef QMCX983_M_NEW_ARCH
	platform_driver_unregister(&qmc_sensor_driver);
#endif
}
/*----------------------------------------------------------------------------*/
module_init(qmcX983_init);
module_exit(qmcX983_exit);

MODULE_AUTHOR("QST Corp");
MODULE_DESCRIPTION("qmcX983 compass driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

