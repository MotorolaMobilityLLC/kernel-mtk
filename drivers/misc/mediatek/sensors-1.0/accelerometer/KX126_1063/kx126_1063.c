/* KX126_1063 motion sensor driver
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

#include <cust_acc.h>
#include "kx126_1063.h"
#include <accel.h>
#include <hwmsensor.h>
#if defined(CONFIG_MTK_I2C_EXTENSION) && defined(KXTJ_ENABLE_I2C_DMA)
#include <linux/dma-mapping.h>
#endif

//+add by hzb for ontim debug
#include <ontim/ontim_dev_dgb.h>
static char version[]="kx126-1063-1.0";
static char vendor_name[20]="kx126-1063-rohm";
    DEV_ATTR_DECLARE(gsensor)
    DEV_ATTR_DEFINE("version",version)
    DEV_ATTR_DEFINE("vendor",vendor_name)
    DEV_ATTR_DECLARE_END;
    ONTIM_DEBUG_DECLARE_AND_INIT(gsensor,gsensor,8);
//-add by hzb for ontim debug

/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
					/*#define CONFIG_KX126_1063_LOWPASS  *//*apply low pass filter on output */
#define SW_CALIBRATION
/*#define USE_EARLY_SUSPEND*/

static struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;
static DEFINE_MUTEX(KX126_1063_mutex);
#if 0
/* For  driver get cust info */
struct acc_hw *get_cust_acc(void)
{
	return &accel_cust;
}
#endif
/*----------------------------------------------------------------------------*/
#define KX126_1063_AXIS_X          0
#define KX126_1063_AXIS_Y          1
#define KX126_1063_AXIS_Z          2
#define KX126_1063_DATA_LEN        6
#define KX126_1063_DEV_NAME        "KX126_1063"

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id KX126_1063_i2c_id[] = { {KX126_1063_DEV_NAME, 0}, {} };
//static struct i2c_board_info __initdata i2c_KX126_1063={ I2C_BOARD_INFO(KX126_1063_DEV_NAME, 0x1e)};
/* static struct i2c_board_info __initdata i2c_KX126_1063={ I2C_BOARD_INFO(KX126_1063_DEV_NAME, (KX126_1063_I2C_SLAVE_ADDR>>1))};*/
/*the adapter id will be available in customization*/
/*static unsigned short KX126_1063_force[] = {0x00, KX126_1063_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};*/
/*static const unsigned short *const KX126_1063_forces[] = { KX126_1063_force, NULL };*/
/*static struct i2c_client_address_data KX126_1063_addr_data = { .forces = KX126_1063_forces,};*/

/*----------------------------------------------------------------------------*/
static int KX126_1063_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int KX126_1063_i2c_remove(struct i2c_client *client);
/*static int KX126_1063_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);*/
static int KX126_1063_suspend(struct device *dev);
static int KX126_1063_resume(struct device *dev);
static int KX126_1063_local_init(void);
static int KX126_1063_remove(void);

#ifdef CUSTOM_KERNEL_SENSORHUB
static int KX126_1063_setup_irq(void);
#endif

/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               pr_warn(GSE_TAG"%s\n", __func__)
#define GSE_ERR(fmt, args...)    pr_err(GSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    pr_debug(fmt, ##args)
/*----------------------------------------------------------------------------*/
static struct i2c_client *KX126_1063_i2c_client;

#ifdef CONFIG_MTK_I2C_EXTENSION
#ifdef KXTJ_ENABLE_I2C_DMA
static u8 *g_dma_buff_va;
static u8 *g_dma_buff_pa;
#endif
#else
static u8 *g_i2c_buff;
static u8 *g_i2c_addr;
#endif

#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(KXTJ_ENABLE_I2C_DMA)
static int msg_dma_alloc(void);
static void msg_dma_release(void);
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION
#ifdef KXTJ_ENABLE_I2C_DMA
static int msg_dma_alloc(void)
{
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	KX126_1063_i2c_client->dev.coherent_dma_mask = DMA_BIT_MASK(64);
#else
	KX126_1063_i2c_client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
#endif
	KX126_1063_i2c_client->dev.dma_mask = &KX126_1063_i2c_client->dev.coherent_dma_mask;
	g_dma_buff_va = (u8 *) dma_alloc_coherent(&KX126_1063_i2c_client->dev,
						  128, (dma_addr_t *) (&g_dma_buff_pa),
						  GFP_KERNEL | GFP_DMA);

	/*g_dma_buff_va = (u8 *) dma_alloc_coherent(NULL, KXTJ_DMA_MAX_TRANSACTION_LEN,
	   (dma_addr_t *) (&g_dma_buff_pa),
	   GFP_KERNEL | GFP_DMA); */
	if (!g_dma_buff_va) {
		GSE_ERR("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
		return -1;
	}
	return 0;
}

static void msg_dma_release(void)
{
	if (g_dma_buff_va) {
		dma_free_coherent(NULL, KXTJ_DMA_MAX_TRANSACTION_LEN,
				  g_dma_buff_va, (dma_addr_t) g_dma_buff_pa);
		g_dma_buff_va = NULL;
		g_dma_buff_pa = NULL;
		GSE_LOG("[DMA][release]I2C Buffer release!\n");
	}
}

#ifdef KXTJ_ENABLE_WRRD_MODE
/*WRRD(write and read) mode, no stop condition after write reg addr*/
/*max DMA read len  31  bytes */
static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if (rxbuf == NULL)
		return -1;
	memset(&msg, 0, sizeof(struct i2c_msg));

	*g_dma_buff_va = addr;
	msg.addr = client->addr & I2C_MASK_FLAG;
	msg.flags = 0;
	msg.len = (len << 8) | KXTJ_REG_ADDR_LEN;
	msg.buf = g_dma_buff_pa;
	msg.ext_flag = client->ext_flag | I2C_ENEXT_FLAG | I2C_WR_FLAG | I2C_RS_FLAG | I2C_DMA_FLAG;
	msg.timing = KXTJ_I2C_MASTER_CLOCK;

	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_dma_buff_va, len);
		return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d", addr, len, ret);
	return ret;
}
#else
/*read only mode, max read length is 65532bytes*/
static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg[2];

	if (rxbuf == NULL)
		return -1;
	memset(&msg, 0, sizeof(struct i2c_msg));

	*g_dma_buff_va = addr;
	msg[0].addr = client->addr & I2C_MASK_FLAG;
	msg[0].flags = 0;
	msg[0].len = KXTJ_REG_ADDR_LEN;
	msg[0].buf = g_dma_buff_pa;
	msg[0].ext_flag = I2C_DMA_FLAG;
	msg[0].timing = KXTJ_I2C_MASTER_CLOCK;

	msg[1].addr = client->addr & I2C_MASK_FLAG;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = g_dma_buff_pa;
	msg[1].ext_flag = client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG;
	msg[1].timing = KXTJ_I2C_MASTER_CLOCK;

	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_dma_buff_va, len);
		return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d", addr, len, ret);
	return ret;
}
#endif

static s32 i2c_dma_write(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if ((txbuf == NULL) && (len > 0))
		return -1;

	memset(&msg, 0, sizeof(struct i2c_msg));
	*g_dma_buff_va = addr;

	msg.addr = (client->addr & I2C_MASK_FLAG);
	msg.flags = 0;
	msg.buf = g_dma_buff_pa;
	msg.len = KXTJ_REG_ADDR_LEN + len;
	msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
	msg.timing = KXTJ_I2C_MASTER_CLOCK;

	if (txbuf)
		memcpy(g_dma_buff_va + KXTJ_REG_ADDR_LEN, txbuf, len);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSE_ERR("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

#else				/*KXTJ_ENABLE_I2C_DMA */
#ifdef KXTJ_ENABLE_WRRD_MODE
static s32 i2c_read_nondma(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if (rxbuf == NULL)
		return -1;
	memset(&msg, 0, sizeof(struct i2c_msg));

	rxbuf[0] = addr;
	msg.addr = client->addr & I2C_MASK_FLAG;
	msg.flags = 0;
	msg.len = (len << 8) | KXTJ_REG_ADDR_LEN;
	msg.buf = rxbuf;
	msg.ext_flag = client->ext_flag | I2C_ENEXT_FLAG | I2C_WR_FLAG | I2C_RS_FLAG;
	msg.timing = KXTJ_I2C_MASTER_CLOCK;

	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%4X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}
#else
static s32 i2c_read_nondma(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg[2];

	if (rxbuf == NULL)
		return -1;
	memset(&msg, 0, sizeof(struct i2c_msg));

	msg[0].addr = client->addr & I2C_MASK_FLAG;
	msg[0].flags = 0;
	msg[0].len = KXTJ_REG_ADDR_LEN;
	msg[0].buf = &addr;
	msg[0].ext_flag = client->ext_flag | I2C_ENEXT_FLAG | I2C_RS_FLAG;
	msg[0].timing = KXTJ_I2C_MASTER_CLOCK;

	msg[1].addr = client->addr & I2C_MASK_FLAG;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = rxbuf;
	msg[1].ext_flag = client->ext_flag | I2C_ENEXT_FLAG;
	msg[1].timing = KXTJ_I2C_MASTER_CLOCK;


	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;
		return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%4X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}
#endif

static s32 i2c_write_nondma(struct i2c_client *client, u8 addr, u8 *txbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;
	u8 wrBuf[C_I2C_FIFO_SIZE];

	if ((txbuf == NULL) && (len > 0))
		return -1;

	memset(&msg, 0, sizeof(struct i2c_msg));
	memset(wrBuf, 0, C_I2C_FIFO_SIZE);
	wrBuf[0] = addr;
	if (txbuf)
		memcpy(wrBuf + KXTJ_REG_ADDR_LEN, txbuf, len);

	msg.flags = 0;
	msg.buf = wrBuf;
	msg.len = KXTJ_REG_ADDR_LEN + len;
	msg.addr = (client->addr & I2C_MASK_FLAG);
	msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG);
	msg.timing = KXTJ_I2C_MASTER_CLOCK;

	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSE_ERR("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}
#endif
#else				/*CONFIG_MTK_I2C_EXTENSION */
static int msg_dma_alloc(void)
{
	g_i2c_buff = kzalloc(KXTJ_DMA_MAX_TRANSACTION_LEN, GFP_KERNEL);
	if (!g_i2c_buff) {
		GSE_ERR("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
		return -1;
	}

	g_i2c_addr = kzalloc(KXTJ_REG_ADDR_LEN, GFP_KERNEL);
	if (!g_i2c_addr) {
		GSE_ERR("[DMA]Allocate DMA I2C addr buf failed!\n");
		kfree(g_i2c_buff);
		g_i2c_buff = NULL;
		return -1;
	}

	return 0;
}

static void msg_dma_release(void)
{
	if (g_i2c_buff) {
		kfree(g_i2c_buff);
		g_i2c_buff = NULL;
	}

	if (g_i2c_addr) {
		kfree(g_i2c_addr);
		g_i2c_addr = NULL;
	}
}

static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg[2];

	if (rxbuf == NULL)
		return -1;

	memset(&msg, 0, 2 * sizeof(struct i2c_msg));
	memcpy(g_i2c_addr, &addr, KXTJ_REG_ADDR_LEN);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = g_i2c_addr;
	msg[0].len = KXTJ_REG_ADDR_LEN;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = g_i2c_buff;
	msg[1].len = len;

	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_i2c_buff, len);
		return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%4X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

static s32 i2c_dma_write(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if ((txbuf == NULL) && (len > 0))
		return -1;

	memset(&msg, 0, sizeof(struct i2c_msg));
	*g_i2c_buff = addr;

	msg.addr = (client->addr);
	msg.flags = 0;
	msg.buf = g_i2c_buff;
	msg.len = KXTJ_REG_ADDR_LEN + len;

	if (txbuf)
		memcpy(g_i2c_buff + KXTJ_REG_ADDR_LEN, txbuf, len);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSE_ERR("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}
#endif

static int kx126_i2c_read(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int left = len;
	int readLen = 0;
	u8 *rd_buf = rxbuf;
	int ret = 0;
    mutex_lock(&KX126_1063_mutex);
	while (left > 0) {
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(KXTJ_ENABLE_I2C_DMA)
		readLen = left > KXTJ_DMA_MAX_RD_SIZE ? KXTJ_DMA_MAX_RD_SIZE : left;
		ret = i2c_dma_read(client, addr, rd_buf, readLen);
#else
		readLen = left > KXTJ_FIFO_MAX_RD_SIZE ? KXTJ_FIFO_MAX_RD_SIZE : left;
		ret = i2c_read_nondma(client, addr, rd_buf, readLen);
#endif
		if (ret < 0) {
			GSE_ERR("dma read failed!\n");
		    mutex_unlock(&KX126_1063_mutex);	
            return -1;
		}

		left -= readLen;
		if (left > 0) {
			addr += readLen;
			rd_buf += readLen;
		}
	}
    mutex_unlock(&KX126_1063_mutex);	
    return 0;
}

static s32 kx126_i2c_write(struct i2c_client *client, u8 addr, u8 *txbuf, int len)
{

	int ret = 0;
	int write_len = 0;
	int left = len;
	u8 *wr_buf = txbuf;
	u8 offset = 0;
	u8 wrAddr = addr;
    mutex_lock(&KX126_1063_mutex);
	while (left > 0) {
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(KXTJ_ENABLE_I2C_DMA)
		write_len = left > KXTJ_DMA_MAX_WR_SIZE ? KXTJ_DMA_MAX_WR_SIZE : left;
		ret = i2c_dma_write(client, wrAddr, wr_buf, write_len);
#else
		write_len = left > KXTJ_FIFO_MAX_WR_SIZE ? KXTJ_FIFO_MAX_WR_SIZE : left;
		ret = i2c_write_nondma(client, wrAddr, wr_buf, write_len);
#endif

		if (ret < 0) {
			GSE_ERR("dma i2c write failed!\n");
		    mutex_unlock(&KX126_1063_mutex);	
            return -1;
		}
		offset += write_len;
		left -= write_len;
		if (left > 0) {
			wrAddr = addr + offset;
			wr_buf = txbuf + offset;
		}
	}
	mutex_unlock(&KX126_1063_mutex);
    return 0;
}



s32 kx126_i2c_write_step( u8 addr, u8 *txbuf, int len)
{
    s32 ret =0;
    struct i2c_client *client = KX126_1063_i2c_client;
    ret = kx126_i2c_write(client,addr,txbuf,len);
    return ret;
}

int kx126_i2c_read_step(u8 addr, u8 *rxbuf, int len)
{
    int ret = 0;
    struct i2c_client *client = KX126_1063_i2c_client;
    ret = kx126_i2c_read(client,addr,rxbuf,len);
    return ret;
}


/*----------------------------------------------------------------------------*/
typedef enum {
	ADX_TRC_FILTER = 0x01,
	ADX_TRC_RAWDATA = 0x02,
	ADX_TRC_IOCTL = 0x04,
	ADX_TRC_CALI = 0X08,
	ADX_TRC_INFO = 0X10,
} ADX_TRC;
/*----------------------------------------------------------------------------*/
struct scale_factor {
	u8 whole;
	u8 fraction;
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
	struct scale_factor scalefactor;
	int sensitivity;
};
/*----------------------------------------------------------------------------*/
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][KX126_1063_AXES_NUM];
	int sum[KX126_1063_AXES_NUM];
	int num;
	int idx;
};
/*----------------------------------------------------------------------------*/
struct KX126_1063_i2c_data {
	struct i2c_client *client;
	struct acc_hw *hw;
	struct hwmsen_convert cvt;
#ifdef CUSTOM_KERNEL_SENSORHUB
	struct work_struct irq_work;
#endif

	/*misc */
	struct data_resolution *reso;
	atomic_t trace;
	atomic_t suspend;
	atomic_t selftest;
	atomic_t filter;
	s16 cali_sw[KX126_1063_AXES_NUM + 1];

	/*data */
	s8 offset[KX126_1063_AXES_NUM + 1];	/*+1: for 4-byte alignment */
	s16 data[KX126_1063_AXES_NUM + 1];

#ifdef CUSTOM_KERNEL_SENSORHUB
	int SCP_init_done;
#endif

#if defined(CONFIG_KX126_1063_LOWPASS)
	atomic_t firlen;
	atomic_t fir_en;
	struct data_filter fir;
#endif
	/*early suspend */
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(USE_EARLY_SUSPEND)
	struct early_suspend early_drv;
#endif
};

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor_kx126_1063"},
	{},
};
#endif

/*----------------------------------------------------------------------------*/
#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops KX126_1063_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(KX126_1063_suspend, KX126_1063_resume)
};
#endif
static struct i2c_driver KX126_1063_i2c_driver = {
	.driver = {
		   .name = KX126_1063_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm = &KX126_1063_pm_ops,
#endif
#ifdef CONFIG_OF
		   .of_match_table = accel_of_match,
#endif
		   },
	.probe = KX126_1063_i2c_probe,
	.remove = KX126_1063_i2c_remove,
	/*      .detect  = KX126_1063_i2c_detect, */
	.id_table = KX126_1063_i2c_id,
	/*.address_data = &KX126_1063_addr_data, */
};

/*----------------------------------------------------------------------------*/
static struct KX126_1063_i2c_data *obj_i2c_data;
//struct KX126_1063_i2c_data *obj_i2c_data_kx126;
int KX126_1063_suspend_flag =0;

static bool sensor_power = true;
static struct GSENSOR_VECTOR3D gsensor_gain;
static char selftestRes[8] = { 0 };

//static DEFINE_MUTEX(KX126_1063_mutex);
static bool enable_status;

int KX126_1063_init_flag = -1;	/*0<==>OK -1 <==> fail */

static struct acc_init_info KX126_1063_init_info = {
	.name = "KX126_1063",
	.init = KX126_1063_local_init,
	.uninit = KX126_1063_remove,
};



static struct data_resolution KX126_1063_data_resolution[1] = {
	/* combination by {FULL_RES,RANGE} */
	{{0, 9}, 1024},
	/* dataformat +/-2g  in 12-bit resolution;  { 3, 9} = 3.9 = (2*2*1000)/(2^12);  256 = (2^12)/(2*2)   */
};

/*----------------------------------------------------------------------------*/
static struct data_resolution KX126_1063_offset_resolution = { {15, 6}, 64 };

/*----------------------------------------------------------------------------*/
static int KX126_1063_SetPowerMode(struct i2c_client *client, bool enable);
/*--------------------KX126_1063 power control function----------------------------------*/
static void KX126_1063_power(struct acc_hw *hw, unsigned int on)
{

}

/*----------------------------------------------------------------------------*/
static int KX126_1063_SetDataResolution(struct KX126_1063_i2c_data *obj)
{
	int err;
	u8 databuf[2];
	bool cur_sensor_power = sensor_power;

	KX126_1063_SetPowerMode(obj->client, false);

	err = kx126_i2c_read(obj->client, KX126_1063_REG_DATA_RESOLUTION, databuf, 0x01);
	if (err < 0) {
		GSE_ERR("kxj3_1057 read Dataformat failt\n");
		return KX126_1063_ERR_I2C;
	}

	databuf[0] &= ~KX126_1063_RANGE_DATA_RESOLUTION_MASK;
	//databuf[0] |= KX126_1063_RANGE_DATA_RESOLUTION_MASK;

	err = kx126_i2c_write(obj->client, KX126_1063_REG_DATA_RESOLUTION, databuf, 0x01);
	/*err = i2c_master_send(obj->client, databuf, 0x2); */

	if (err < 0)
		return KX126_1063_ERR_I2C;

	KX126_1063_SetPowerMode(obj->client, cur_sensor_power /*true */);

	/*KX126_1063_data_resolution[0] has been set when initialize: +/-2g  in 8-bit resolution:  15.6 mg/LSB */
	obj->reso = &KX126_1063_data_resolution[0];

	return 0;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_ReadData(struct i2c_client *client, s16 data[KX126_1063_AXES_NUM])
{
	struct KX126_1063_i2c_data *priv = i2c_get_clientdata(client);
	int err = 0;
	u8 addr = KX126_1063_REG_DATAX0;
	u8 buf[KX126_1063_DATA_LEN] = { 0 };
	int i;
#ifdef CONFIG_KX126_1063_LOWPASS
	int idx, firlen;
#endif
    

	if (NULL == client) {
		err = -EINVAL;
		goto exit;
	}
	err = kx126_i2c_read(client, addr, buf, 0x06);
	if (err) {
		GSE_ERR("KX126_1063_ReadData error: %d\n", err);
		goto exit;
	}

	data[KX126_1063_AXIS_X] = (s16)((buf[KX126_1063_AXIS_X*2]) |
			 (buf[KX126_1063_AXIS_X*2+1] << 8));
	data[KX126_1063_AXIS_Y] = (s16)((buf[KX126_1063_AXIS_Y*2]) |
			 (buf[KX126_1063_AXIS_Y*2+1] << 8));
	data[KX126_1063_AXIS_Z] = (s16)((buf[KX126_1063_AXIS_Z*2]) |
			 (buf[KX126_1063_AXIS_Z*2+1] << 8));
	//GSE_LOG("read sensor data==============%d %d %d\n",data[KX126_1063_AXIS_X],data[KX126_1063_AXIS_Y],data[KX126_1063_AXIS_Z]);
	for(i=0;i<3;i++)				
	{								//because the data is store in binary complement number formation in computer system
		//if ( data[i] == 0x1000 )	//so we want to calculate actual number here
		if ( data[i] == 0x8000 )	//so we want to calculate actual number here
			data[i]= -4096;			//16bit resolution, 512= 2^(12-1)
			//data[i]= -16384;			//16bit resolution, 512= 2^(12-1)
		//else if ( data[i] & 0x1000 )//transfor format
		else if ( data[i] & 0x8000 )//transfor format
		{							//printk("data 0 step %x \n",data[i]);
			data[i] -= 0x1; 		//printk("data 1 step %x \n",data[i]);
			data[i] = ~data[i]; 	//printk("data 2 step %x \n",data[i]);
			data[i] &= 0x1fff;		//printk("data 3 step %x \n\n",data[i]);
			//data[i] &= 0x7fff;		//printk("data 3 step %x \n\n",data[i]);
			data[i] = -data[i]; 	
		}
	}
    
    data[KX126_1063_AXIS_X] = (data[KX126_1063_AXIS_X] >> 2); 
    data[KX126_1063_AXIS_Y] = (data[KX126_1063_AXIS_Y] >> 2);
    data[KX126_1063_AXIS_Z] = (data[KX126_1063_AXIS_Z] >> 2);
   

	if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n", data[KX126_1063_AXIS_X],
			data[KX126_1063_AXIS_Y], data[KX126_1063_AXIS_Z],
			data[KX126_1063_AXIS_X], data[KX126_1063_AXIS_Y],
			data[KX126_1063_AXIS_Z]);
#ifdef CONFIG_KX126_1063_LOWPASS
	if (atomic_read(&priv->filter)) {
		if (atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend)) {
			firlen = atomic_read(&priv->firlen);

			if (priv->fir.num < firlen) {
				priv->fir.raw[priv->fir.num][KX126_1063_AXIS_X] =
				    data[KX126_1063_AXIS_X];
				priv->fir.raw[priv->fir.num][KX126_1063_AXIS_Y] =
				    data[KX126_1063_AXIS_Y];
				priv->fir.raw[priv->fir.num][KX126_1063_AXIS_Z] =
				    data[KX126_1063_AXIS_Z];
				priv->fir.sum[KX126_1063_AXIS_X] += data[KX126_1063_AXIS_X];
				priv->fir.sum[KX126_1063_AXIS_Y] += data[KX126_1063_AXIS_Y];
				priv->fir.sum[KX126_1063_AXIS_Z] += data[KX126_1063_AXIS_Z];
				if (atomic_read(&priv->trace) & ADX_TRC_FILTER)
					GSE_LOG
					    ("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n",
					     priv->fir.num,
					     priv->fir.raw[priv->fir.
							   num][KX126_1063_AXIS_X],
					     priv->fir.raw[priv->fir.
							   num][KX126_1063_AXIS_Y],
					     priv->fir.raw[priv->fir.
							   num][KX126_1063_AXIS_Z],
					     priv->fir.sum[KX126_1063_AXIS_X],
					     priv->fir.sum[KX126_1063_AXIS_Y],
					     priv->fir.sum[KX126_1063_AXIS_Z]);
				priv->fir.num++;
				priv->fir.idx++;
			} else {
				idx = priv->fir.idx % firlen;
				priv->fir.sum[KX126_1063_AXIS_X] -=
				    priv->fir.raw[idx][KX126_1063_AXIS_X];
				priv->fir.sum[KX126_1063_AXIS_Y] -=
				    priv->fir.raw[idx][KX126_1063_AXIS_Y];
				priv->fir.sum[KX126_1063_AXIS_Z] -=
				    priv->fir.raw[idx][KX126_1063_AXIS_Z];
				priv->fir.raw[idx][KX126_1063_AXIS_X] =
				    data[KX126_1063_AXIS_X];
				priv->fir.raw[idx][KX126_1063_AXIS_Y] =
				    data[KX126_1063_AXIS_Y];
				priv->fir.raw[idx][KX126_1063_AXIS_Z] =
				    data[KX126_1063_AXIS_Z];
				priv->fir.sum[KX126_1063_AXIS_X] += data[KX126_1063_AXIS_X];
				priv->fir.sum[KX126_1063_AXIS_Y] += data[KX126_1063_AXIS_Y];
				priv->fir.sum[KX126_1063_AXIS_Z] += data[KX126_1063_AXIS_Z];
				priv->fir.idx++;
				data[KX126_1063_AXIS_X] =
				    priv->fir.sum[KX126_1063_AXIS_X] / firlen;
				data[KX126_1063_AXIS_Y] =
				    priv->fir.sum[KX126_1063_AXIS_Y] / firlen;
				data[KX126_1063_AXIS_Z] =
				    priv->fir.sum[KX126_1063_AXIS_Z] / firlen;
				if (atomic_read(&priv->trace) & ADX_TRC_FILTER) {
					GSE_LOG
					    ("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n",
					     idx, priv->fir.raw[idx][KX126_1063_AXIS_X],
					     priv->fir.raw[idx][KX126_1063_AXIS_Y],
					     priv->fir.raw[idx][KX126_1063_AXIS_Z],
					     priv->fir.sum[KX126_1063_AXIS_X],
					     priv->fir.sum[KX126_1063_AXIS_Y],
					     priv->fir.sum[KX126_1063_AXIS_Z],
					     data[KX126_1063_AXIS_X],
					     data[KX126_1063_AXIS_Y],
					     data[KX126_1063_AXIS_Z]);
				}
			}
		}
	}
#endif

exit:
	return err;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_ReadOffset(struct i2c_client *client, s8 ofs[KX126_1063_AXES_NUM])
{
	int err = 0;

	ofs[1] = ofs[2] = ofs[0] = 0x00;

	GSE_LOG("offesx=%x, y=%x, z=%x", ofs[0], ofs[1], ofs[2]);

	return err;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_ResetCalibration(struct i2c_client *client)
{
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA data;
	KX126_1063_CUST_DATA *pCustData;
	unsigned int len;

	if (0 != obj->SCP_init_done) {
		pCustData = (KX126_1063_CUST_DATA *) &data.set_cust_req.custData;

		data.set_cust_req.sensorType = ID_ACCELEROMETER;
		data.set_cust_req.action = SENSOR_HUB_SET_CUST;
		pCustData->resetCali.action = KX126_1063_CUST_ACTION_RESET_CALI;
		len =
		    offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(pCustData->resetCali);
		SCP_sensorHub_req_send(&data, &len, 1);
	}
#endif

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_ReadCalibration(struct i2c_client *client, int dat[KX126_1063_AXES_NUM])
{
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int mul;

#ifdef SW_CALIBRATION
	mul = 0;		/*only SW Calibration, disable HW Calibration */
#else
	err = KX126_1063_ReadOffset(client, obj->offset);
	if (err != 0) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}
	mul = obj->reso->sensitivity / KX126_1063_offset_resolution.sensitivity;
#endif

	dat[obj->cvt.map[KX126_1063_AXIS_X]] =
	    obj->cvt.sign[KX126_1063_AXIS_X] * (obj->offset[KX126_1063_AXIS_X] * mul +
						obj->cali_sw[KX126_1063_AXIS_X]);
	dat[obj->cvt.map[KX126_1063_AXIS_Y]] =
	    obj->cvt.sign[KX126_1063_AXIS_Y] * (obj->offset[KX126_1063_AXIS_Y] * mul +
						obj->cali_sw[KX126_1063_AXIS_Y]);
	dat[obj->cvt.map[KX126_1063_AXIS_Z]] =
	    obj->cvt.sign[KX126_1063_AXIS_Z] * (obj->offset[KX126_1063_AXIS_Z] * mul +
						obj->cali_sw[KX126_1063_AXIS_Z]);

	return 0;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_ReadCalibrationEx(struct i2c_client *client, int act[KX126_1063_AXES_NUM],
					int raw[KX126_1063_AXES_NUM])
{
	/*raw: the raw calibration data; act: the actual calibration data */
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
#ifdef SW_CALIBRATION
#else
	int err;
#endif
	int mul;

#ifdef SW_CALIBRATION
	mul = 0;		/*only SW Calibration, disable HW Calibration */
#else
	err = KX126_1063_ReadOffset(client, obj->offset);
	if (err != 0) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}
	mul = obj->reso->sensitivity / KX126_1063_offset_resolution.sensitivity;
#endif

	raw[KX126_1063_AXIS_X] =
	    obj->offset[KX126_1063_AXIS_X] * mul + obj->cali_sw[KX126_1063_AXIS_X];
	raw[KX126_1063_AXIS_Y] =
	    obj->offset[KX126_1063_AXIS_Y] * mul + obj->cali_sw[KX126_1063_AXIS_Y];
	raw[KX126_1063_AXIS_Z] =
	    obj->offset[KX126_1063_AXIS_Z] * mul + obj->cali_sw[KX126_1063_AXIS_Z];

	act[obj->cvt.map[KX126_1063_AXIS_X]] =
	    obj->cvt.sign[KX126_1063_AXIS_X] * raw[KX126_1063_AXIS_X];
	act[obj->cvt.map[KX126_1063_AXIS_Y]] =
	    obj->cvt.sign[KX126_1063_AXIS_Y] * raw[KX126_1063_AXIS_Y];
	act[obj->cvt.map[KX126_1063_AXIS_Z]] =
	    obj->cvt.sign[KX126_1063_AXIS_Z] * raw[KX126_1063_AXIS_Z];

	return 0;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_WriteCalibration(struct i2c_client *client, int dat[KX126_1063_AXES_NUM])
{
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int cali[KX126_1063_AXES_NUM], raw[KX126_1063_AXES_NUM];
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA data;
	KX126_1063_CUST_DATA *pCustData;
	unsigned int len;
#endif
#ifdef SW_CALIBRATION
#else
	int lsb = KX126_1063_offset_resolution.sensitivity;
	int divisor = obj->reso->sensitivity / lsb;
#endif

	err = KX126_1063_ReadCalibrationEx(client, cali, raw);
	if (err != 0) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		raw[KX126_1063_AXIS_X], raw[KX126_1063_AXIS_Y], raw[KX126_1063_AXIS_Z],
		obj->offset[KX126_1063_AXIS_X], obj->offset[KX126_1063_AXIS_Y],
		obj->offset[KX126_1063_AXIS_Z], obj->cali_sw[KX126_1063_AXIS_X],
		obj->cali_sw[KX126_1063_AXIS_Y], obj->cali_sw[KX126_1063_AXIS_Z]);

#ifdef CUSTOM_KERNEL_SENSORHUB
	pCustData = (KX126_1063_CUST_DATA *) data.set_cust_req.custData;
	data.set_cust_req.sensorType = ID_ACCELEROMETER;
	data.set_cust_req.action = SENSOR_HUB_SET_CUST;
	pCustData->setCali.action = KX126_1063_CUST_ACTION_SET_CALI;
	pCustData->setCali.data[KX126_1063_AXIS_X] = dat[KX126_1063_AXIS_X];
	pCustData->setCali.data[KX126_1063_AXIS_Y] = dat[KX126_1063_AXIS_Y];
	pCustData->setCali.data[KX126_1063_AXIS_Z] = dat[KX126_1063_AXIS_Z];
	len = offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(pCustData->setCali);
	SCP_sensorHub_req_send(&data, &len, 1);
#endif

	/*calculate the real offset expected by caller */
	cali[KX126_1063_AXIS_X] += dat[KX126_1063_AXIS_X];
	cali[KX126_1063_AXIS_Y] += dat[KX126_1063_AXIS_Y];
	cali[KX126_1063_AXIS_Z] += dat[KX126_1063_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n",
		dat[KX126_1063_AXIS_X], dat[KX126_1063_AXIS_Y], dat[KX126_1063_AXIS_Z]);

#ifdef SW_CALIBRATION
	obj->cali_sw[KX126_1063_AXIS_X] =
	    obj->cvt.sign[KX126_1063_AXIS_X] * (cali[obj->cvt.map[KX126_1063_AXIS_X]]);
	obj->cali_sw[KX126_1063_AXIS_Y] =
	    obj->cvt.sign[KX126_1063_AXIS_Y] * (cali[obj->cvt.map[KX126_1063_AXIS_Y]]);
	obj->cali_sw[KX126_1063_AXIS_Z] =
	    obj->cvt.sign[KX126_1063_AXIS_Z] * (cali[obj->cvt.map[KX126_1063_AXIS_Z]]);
#else
	obj->offset[KX126_1063_AXIS_X] =
	    (s8) (obj->cvt.sign[KX126_1063_AXIS_X] * (cali[obj->cvt.map[KX126_1063_AXIS_X]]) /
		  (divisor));
	obj->offset[KX126_1063_AXIS_Y] =
	    (s8) (obj->cvt.sign[KX126_1063_AXIS_Y] * (cali[obj->cvt.map[KX126_1063_AXIS_Y]]) /
		  (divisor));
	obj->offset[KX126_1063_AXIS_Z] =
	    (s8) (obj->cvt.sign[KX126_1063_AXIS_Z] * (cali[obj->cvt.map[KX126_1063_AXIS_Z]]) /
		  (divisor));

	/*convert software calibration using standard calibration */
	obj->cali_sw[KX126_1063_AXIS_X] =
	    obj->cvt.sign[KX126_1063_AXIS_X] * (cali[obj->cvt.map[KX126_1063_AXIS_X]]) % (divisor);
	obj->cali_sw[KX126_1063_AXIS_Y] =
	    obj->cvt.sign[KX126_1063_AXIS_Y] * (cali[obj->cvt.map[KX126_1063_AXIS_Y]]) % (divisor);
	obj->cali_sw[KX126_1063_AXIS_Z] =
	    obj->cvt.sign[KX126_1063_AXIS_Z] * (cali[obj->cvt.map[KX126_1063_AXIS_Z]]) % (divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		obj->offset[KX126_1063_AXIS_X] * divisor + obj->cali_sw[KX126_1063_AXIS_X],
		obj->offset[KX126_1063_AXIS_Y] * divisor + obj->cali_sw[KX126_1063_AXIS_Y],
		obj->offset[KX126_1063_AXIS_Z] * divisor + obj->cali_sw[KX126_1063_AXIS_Z],
		obj->offset[KX126_1063_AXIS_X], obj->offset[KX126_1063_AXIS_Y],
		obj->offset[KX126_1063_AXIS_Z], obj->cali_sw[KX126_1063_AXIS_X],
		obj->cali_sw[KX126_1063_AXIS_Y], obj->cali_sw[KX126_1063_AXIS_Z]);

	err = kx126_i2c_write(obj->client, KX126_1063_REG_OFSX, obj->offset, KX126_1063_AXES_NUM);
	if (err != 0) {
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
#endif

	return err;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];
	int res = 0;

	memset(databuf, 0, sizeof(u8) * 10);

	res = kx126_i2c_read(client, KX126_1063_REG_DEVID, databuf, 0x1);

	GSE_LOG("KX126_1063_CheckDeviceID 0x%x pass!\n ", databuf[0]);

	if (res < 0) {
		GSE_ERR("KX126_1063_CheckDeviceID send fail\n");
		goto exit_KX126_1063_CheckDeviceID;
	}

exit_KX126_1063_CheckDeviceID:
	if (res < 0)
		return KX126_1063_ERR_I2C;

	return KX126_1063_SUCCESS;
}

/*----------------------------------------------------------------------------*/
#ifdef CUSTOM_KERNEL_SENSORHUB
static int KX126_1063_SCP_SetPowerMode(bool enable)
{
	int res = 0;
	SCP_SENSOR_HUB_DATA req;
	int len;

	if (enable == sensor_power) {
		GSE_LOG("Sensor power status is newest!\n");
		return KX126_1063_SUCCESS;
	}

	req.activate_req.sensorType = ID_ACCELEROMETER;
	req.activate_req.action = SENSOR_HUB_ACTIVATE;
	req.activate_req.enable = enable;
	len = sizeof(req.activate_req);
	res = SCP_sensorHub_req_send(&req, &len, 1);
	if (res) {
		GSE_ERR("SCP_sensorHub_req_send!\n");
		return res;
	}

	GSE_LOG("KX126_1063_SetPowerMode %d!\n ", enable);

	sensor_power = enable;

	mdelay(5);

	return KX126_1063_SUCCESS;
}
#endif
/*----------------------------------------------------------------------------*/
static int KX126_1063_SetPowerMode(struct i2c_client *client, bool enable)
{
	int res = 0, ret = 0;
	u8 databuf[2];
	u8 addr = KX126_1063_REG_CTL_REG1;

	if (enable == sensor_power) {
		GSE_LOG("Sensor power status is newest!\n");
		return KX126_1063_SUCCESS;
	}

	ret = kx126_i2c_read(client, addr, databuf, 0x01);
	if (ret < 0) {
		GSE_ERR("read power ctl register err!\n");
		return KX126_1063_ERR_I2C;
	}


	if (enable == true)
		databuf[0] |= (KX126_1063_MEASURE_MODE);
		//databuf[0] |= (KX126_1063_MEASURE_MODE | 0x02);
	else
		databuf[0] &= ~ (KX126_1063_MEASURE_MODE);
		//databuf[0] &= ~ (KX126_1063_MEASURE_MODE | 0x02);

	res = kx126_i2c_write(client, KX126_1063_REG_CTL_REG1, databuf, 0x1);

	if (res < 0)
		return KX126_1063_ERR_I2C;

	GSE_LOG("KX126_1063_SetPowerMode %d!\n ", enable);

	sensor_power = enable;

	mdelay(10);
	//mdelay(5);

	return KX126_1063_SUCCESS;
}

int KX126_1063_SetPowerMode_step(bool enable)
{
    int res =0;
    struct i2c_client *client = KX126_1063_i2c_client;
    res = KX126_1063_SetPowerMode(client,enable);
    return res;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];
	int res = 0;
	bool cur_sensor_power = sensor_power;

	memset(databuf, 0, sizeof(u8) * 10);

	KX126_1063_SetPowerMode(client, false);

	res = kx126_i2c_read(client, KX126_1063_REG_DATA_FORMAT, databuf, 0x01);
	if (res < 0) {
		GSE_ERR("kxj3_1057 read Dataformat failt\n");
		return KX126_1063_ERR_I2C;
	}

	databuf[0] &= ~KX126_1063_RANGE_MASK;
	databuf[0] |= dataformat;

	res = kx126_i2c_write(client, KX126_1063_REG_DATA_FORMAT, databuf, 0x01);

	if (res < 0)
		return KX126_1063_ERR_I2C;

	KX126_1063_SetPowerMode(client, cur_sensor_power /*true */);

	GSE_LOG("KX126_1063_SetDataFormat OK!\n");

	return KX126_1063_SetDataResolution(obj);
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];
	int res = 0;
	bool cur_sensor_power = sensor_power;

	memset(databuf, 0, sizeof(u8) * 10);

	KX126_1063_SetPowerMode(client, false);

	res = kx126_i2c_read(client, KX126_1063_REG_BW_RATE, databuf, 0x01);
	if (res < 0) {
		GSE_ERR("kxj3_1057 read rate failt\n");
		return KX126_1063_ERR_I2C;
	}

	databuf[0] &= 0xf0;
	databuf[0] |= bwrate;

	res = kx126_i2c_write(client, KX126_1063_REG_BW_RATE, databuf, 0x01);

	if (res < 0)
		return KX126_1063_ERR_I2C;

	KX126_1063_SetPowerMode(client, cur_sensor_power /*true */);
	GSE_LOG("KX126_1063_SetBWRate OK!\n");

	return KX126_1063_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];
	int res = 0;
    //bool cur_sensor_power = sensor_power;

	memset(databuf, 0, sizeof(u8) * 10);
	databuf[0] = 0x00;

	res = kx126_i2c_write(client, KX126_1063_REG_INT_CTL_REG1, databuf, 0x01);

	if (res < 0)
		return KX126_1063_ERR_I2C;

	return KX126_1063_SUCCESS;
}

/*----------------------------------------------------------------------------*/
int KX126_1063_init_pedometer(int enable)
{
	struct i2c_client *client = KX126_1063_i2c_client;
    struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	u8 databuf[2];
    //bool cur_sensor_power = sensor_power;
	
    //mutex_lock(&KX126_1063_mutex);
	
    res = KX126_1063_SetPowerMode(client, false);
	if (res != KX126_1063_SUCCESS) {
		GSE_ERR("pedometer KX126_1063_SetPowerMode fail\n");
		return res;
	}
    msleep(30);	
	
	databuf[0] = 0x32;
	res = kx126_i2c_write(obj->client, KX126_1063_REG_CTL_REG1, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
    	return res;
	
	//watermark step counter, 65535 steps=0xFFFF
	databuf[0] = 0xFF;
	res = kx126_i2c_write(obj->client, PED_STPWM_L, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	databuf[0] = 0xFF;
	res = kx126_i2c_write(obj->client, PED_STPWM_H, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;

	databuf[0] = 0xC3;
	res = kx126_i2c_write(client, KX126_1063_REG_BW_RATE, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x7B;//LP_CNTL 128 samples averaged
	res = kx126_i2c_write(obj->client, LP_CNTL, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x61;//overflow and watermark interrupt reported to INT2, step counter interrupt reported to INT1
	res = kx126_i2c_write(obj->client, INC7, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x20;//set INT1 interrupt,latched
	res = kx126_i2c_write(obj->client, INC1, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x20;//set INT2 interrupt,latched
	res = kx126_i2c_write(obj->client, INC5, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	///pedometer control register start******************************
	databuf[0] = 0x66;//4 steps default, input signal value 6
	res = kx126_i2c_write(obj->client, PED_CNTL1, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;

	databuf[0] = 0x2C;//high-pass filter default 8, ODR 100HZ
	res = kx126_i2c_write(obj->client, PED_CNTL2, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x17;
	res = kx126_i2c_write(obj->client, PED_CNTL3, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x1F;
	res = kx126_i2c_write(obj->client, PED_CNTL4, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x24;//peak threshold
	res = kx126_i2c_write(obj->client, PED_CNTL5, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x13;
	res = kx126_i2c_write(obj->client, PED_CNTL6, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x0b;
	res = kx126_i2c_write(obj->client, PED_CNTL7, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x08;
	res = kx126_i2c_write(obj->client, PED_CNTL8, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x19;
	res = kx126_i2c_write(obj->client, PED_CNTL9, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	
	databuf[0] = 0x1c;//0x13=0.18sencond
	res = kx126_i2c_write(obj->client, PED_CNTL10, databuf, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	///pedometer control register end******************************
	
	res = KX126_1063_SetPowerMode(client, true /*true */);
	//res = KX126_1063_SetPowerMode(client, cur_sensor_power /*true */);
	if (res != KX126_1063_SUCCESS) {
		GSE_ERR("pedometer KX126_1063_SetPowerMode fail\n");
		return res;
	}
	
    //mutex_unlock(&KX126_1063_mutex);
	
	GSE_LOG("KX126_1063_init_pedometer OK!\n");
	return KX126_1063_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_init_client(struct i2c_client *client, int reset_cali)
{
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	res = KX126_1063_CheckDeviceID(client);
	if (res != KX126_1063_SUCCESS) {
		GSE_ERR("KX126_1063_CheckDeviceID fail\n");
		return res;
	}

	res = KX126_1063_SetPowerMode(client, false/*enable_status *//*false */);
	if (res != KX126_1063_SUCCESS) {
		GSE_ERR("KX126_1063_SetPowerMode fail\n");
		return res;
	}


	res = KX126_1063_SetBWRate(client, KX126_1063_BW_100HZ);
	if (res != KX126_1063_SUCCESS) {	/*0x2C->BW=100Hz */
		GSE_ERR("KX126_1063_SetBWRate fail\n");
		return res;
	}

	res = KX126_1063_SetDataFormat(client, KX126_1063_RANGE_8G);
	if (res != KX126_1063_SUCCESS) {	/*0x2C->BW=100Hz */
		GSE_ERR("KX126_1063_SetDataFormat fail\n");
		return res;
	}

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

#ifdef CUSTOM_KERNEL_SENSORHUB
	res = KX126_1063_setup_irq();
	if (res != KX126_1063_SUCCESS)
		return res;
#endif

	res = KX126_1063_SetIntEnable(client, 0x00);
	if (res != KX126_1063_SUCCESS)
		return res;

	if (0 != reset_cali) {
		/*reset calibration only in power on */
		res = KX126_1063_ResetCalibration(client);
		if (res != KX126_1063_SUCCESS)
			return res;
	}
	GSE_LOG("KX126_1063_init_client OK!\n");
#ifdef CONFIG_KX126_1063_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	res = KX126_1063_SetPowerMode(client, true/*enable_status *//*false */);
	if (res != KX126_1063_SUCCESS) {
		GSE_ERR("KX126_1063_SetPowerMode fail\n");
		return res;
	}

	return KX126_1063_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8) * 10);

	if ((NULL == buf) || (bufsize <= 30))
		return -1;

	if (NULL == client) {
		*buf = 0;
		return -2;
	}

	sprintf(buf, "KX126_1063 Chip");
	return 0;
}

/*Kionix Auto-Cali Start*/
#define KIONIX_AUTO_CAL		/*Setup AUTO-Cali parameter */
#ifdef KIONIX_AUTO_CAL
/*#define DEBUG_MSG_CAL*/
#define Sensitivity_def      1024
#define Detection_range   200	/* Follow KXTJ2 SPEC Offset Range define */
#define Stable_range        50
#define BUF_RANGE_Limit 10
static int BUF_RANGE = BUF_RANGE_Limit;
static int temp_zbuf[50] = { 0 };

static int temp_zsum;	/* 1024 * BUF_RANGE ; */
static int Z_AVG[2] = { Sensitivity_def, Sensitivity_def };

static int Wave_Max, Wave_Min;
#endif
/*Kionix Auto-Cali End*/

/*----------------------------------------------------------------------------*/
static int KX126_1063_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct KX126_1063_i2c_data *obj = (struct KX126_1063_i2c_data *)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[KX126_1063_AXES_NUM];
	int res = 0;
	/*Kionix Auto-Cali Start */
#ifdef KIONIX_AUTO_CAL
	s16 raw[3];
	int k;
#endif
	/*Kionix Auto-Cali End */

	memset(databuf, 0, sizeof(u8) * 10);

	if (NULL == buf)
		return -1;
	if (NULL == client) {
		*buf = 0;
		return -2;
	}

	if (atomic_read(&obj->suspend))
		return 0;
	res = KX126_1063_ReadData(client, obj->data);
	if (res != 0) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	} else {
#if 0				/*ifdef CUSTOM_KERNEL_SENSORHUB */
		acc[KX126_1063_AXIS_X] = obj->data[KX126_1063_AXIS_X];
		acc[KX126_1063_AXIS_Y] = obj->data[KX126_1063_AXIS_Y];
		acc[KX126_1063_AXIS_Z] = obj->data[KX126_1063_AXIS_Z];
#else

		/*Kionix Auto-Cali Start */
#ifdef KIONIX_AUTO_CAL
		raw[0] = obj->data[KX126_1063_AXIS_X];
		raw[1] = obj->data[KX126_1063_AXIS_Y];
		raw[2] = obj->data[KX126_1063_AXIS_Z];

		if ((abs(raw[0]) < Detection_range)
		    && (abs(raw[1]) < Detection_range)
		    && (abs((abs(raw[2]) - Sensitivity_def)) < ((Detection_range) + 308))) {
#ifdef DEBUG_MSG_CAL
			GSE_LOG("+++kx126 Calibration Raw Data,%d,%d,%d\n", raw[0], raw[1], raw[2]);
#endif
			temp_zsum = 0;
			Wave_Max = -4095;
			Wave_Min = 4095;

			/* BUF_RANGE = 1000 / acc_data.delay;
			   BUF_RANGE = 1000 / acceld->poll_interval ; */

			if (BUF_RANGE > BUF_RANGE_Limit)
				BUF_RANGE = BUF_RANGE_Limit;

			for (k = 0; k < BUF_RANGE - 1; k++) {
				temp_zbuf[k] = temp_zbuf[k + 1];
				if (temp_zbuf[k] == 0)
					temp_zbuf[k] = Sensitivity_def;
				temp_zsum += temp_zbuf[k];
				if (temp_zbuf[k] > Wave_Max)
					Wave_Max = temp_zbuf[k];
				if (temp_zbuf[k] < Wave_Min)
					Wave_Min = temp_zbuf[k];
			}

			temp_zbuf[k] = raw[2];	/* k=BUF_RANGE-1, update Z raw to bubber */
			temp_zsum += temp_zbuf[k];
			if (temp_zbuf[k] > Wave_Max)
				Wave_Max = temp_zbuf[k];
			if (temp_zbuf[k] < Wave_Min)
				Wave_Min = temp_zbuf[k];
			if (Wave_Max - Wave_Min < Stable_range) {

				if (temp_zsum > 0) {
					Z_AVG[0] = temp_zsum / BUF_RANGE;
#ifdef DEBUG_MSG_CAL
					GSE_LOG("+++ Z_AVG=%d\n ", Z_AVG[0]);
#endif
				} else {
					Z_AVG[1] = temp_zsum / BUF_RANGE;
#ifdef DEBUG_MSG_CAL
					GSE_LOG("--- Z_AVG=%d\n ", Z_AVG[1]);
#endif
				}
			}
		} else if (abs((abs(raw[2]) - Sensitivity_def)) > ((Detection_range) + 154)) {
#ifdef DEBUG_MSG_CAL
			GSE_LOG("kx126 out of SPEC Raw Data,%d,%d,%d\n", raw[0], raw[1], raw[2]);
#endif
		}

		if (raw[2] >= 0)
			raw[2] = raw[2] * 1024 / abs(Z_AVG[0]);	/* Gain Compensation */
		else
			raw[2] = raw[2] * 1024 / abs(Z_AVG[1]);	/* Gain Compensation */

#ifdef DEBUG_MSG_CAL
		/*GSE_FUN("---kx126 Calibration Raw Data,%d,%d,%d==> Z+=%d  Z-=%d\n",raw[0],raw[1],raw[2],Z_AVG[0],Z_AVG[1]); */
		GSE_LOG("---After Cali,X=%d,Y=%d,Z=%d\n", raw[0], raw[1], raw[2]);
#endif
		obj->data[KX126_1063_AXIS_X] = raw[0];
		obj->data[KX126_1063_AXIS_Y] = raw[1];
		obj->data[KX126_1063_AXIS_Z] = raw[2];
#endif
		/*Kionix Auto-Cali End */

		//GSE_LOG("kx126 raw data x=%d, y=%d, z=%d\n", obj->data[KX126_1063_AXIS_X],
		//	obj->data[KX126_1063_AXIS_Y], obj->data[KX126_1063_AXIS_Z]);
		obj->data[KX126_1063_AXIS_X] += obj->cali_sw[KX126_1063_AXIS_X];
		obj->data[KX126_1063_AXIS_Y] += obj->cali_sw[KX126_1063_AXIS_Y];
		obj->data[KX126_1063_AXIS_Z] += obj->cali_sw[KX126_1063_AXIS_Z];
        
		
        //GSE_LOG("kx126 cali_sw x=%d, y=%d, z=%d\n", obj->cali_sw[KX126_1063_AXIS_X],
	//		obj->cali_sw[KX126_1063_AXIS_Y], obj->cali_sw[KX126_1063_AXIS_Z]);
        
		/*remap coordinate */
		acc[obj->cvt.map[KX126_1063_AXIS_X]] =
		    obj->cvt.sign[KX126_1063_AXIS_X] * obj->data[KX126_1063_AXIS_X];
		acc[obj->cvt.map[KX126_1063_AXIS_Y]] =
		    obj->cvt.sign[KX126_1063_AXIS_Y] * obj->data[KX126_1063_AXIS_Y];
		acc[obj->cvt.map[KX126_1063_AXIS_Z]] =
		    obj->cvt.sign[KX126_1063_AXIS_Z] * obj->data[KX126_1063_AXIS_Z];
		/*printk("cvt x=%d, y=%d, z=%d\n",obj->cvt.sign[KX126_1063_AXIS_X],obj->cvt.sign[KX126_1063_AXIS_Y],obj->cvt.sign[KX126_1063_AXIS_Z]); */


		/*GSE_LOG("kx126 Mapped gsensor data: %d, %d, %d!\n", acc[KX126_1063_AXIS_X],
			acc[KX126_1063_AXIS_Y], acc[KX126_1063_AXIS_Z]);
        */
		/*Out put the mg */

		acc[KX126_1063_AXIS_X] =
		    acc[KX126_1063_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[KX126_1063_AXIS_Y] =
		    acc[KX126_1063_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[KX126_1063_AXIS_Z] =
		    acc[KX126_1063_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		/*GSE_LOG("kx126 mg x=%d, y=%d, z=%d\n", acc[KX126_1063_AXIS_X],
			acc[KX126_1063_AXIS_Y], acc[KX126_1063_AXIS_Z]);
        */
#endif

		sprintf(buf, "%04x %04x %04x", acc[KX126_1063_AXIS_X], acc[KX126_1063_AXIS_Y],
			acc[KX126_1063_AXIS_Z]);
		if (atomic_read(&obj->trace) & ADX_TRC_IOCTL)
			GSE_LOG("gsensor data: %s!\n", buf);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_ReadRawData(struct i2c_client *client, char *buf)
{
	struct KX126_1063_i2c_data *obj = (struct KX126_1063_i2c_data *)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
		return -EINVAL;

	res = KX126_1063_ReadData(client, obj->data);
	if (res != 0) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	} else {
		sprintf(buf, "KX126_1063_ReadRawData %04x %04x %04x", obj->data[KX126_1063_AXIS_X],
			obj->data[KX126_1063_AXIS_Y], obj->data[KX126_1063_AXIS_Z]);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_InitSelfTest(struct i2c_client *client)
{
	int res = 0;
	u8 data, result, data0, data1;

	res = kx126_i2c_read(client, KX126_1063_REG_CTL_REG2, &data, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	/*enable selftest bit */
	data0 = KX126_1063_SELF_TEST | data;
	res = kx126_i2c_write(client, KX126_1063_REG_CTL_REG2, &data0, 0x01);
	if (res != KX126_1063_SUCCESS)	/*0x2C->BW=100Hz */
		return res;

	/*step 1 */
	res = kx126_i2c_read(client, KX126_1063_DCST_RESP, &result, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;
	GSE_LOG("step1: result = %x", result);
	if (result != 0xaa)
		return -EINVAL;

	/*step 2 */
	data1 = KX126_1063_SELF_TEST | data;
	res = kx126_i2c_write(client, KX126_1063_REG_CTL_REG2, &data1, 0x01);
	if (res != KX126_1063_SUCCESS)	/*0x2C->BW=100Hz */
		return res;

	/*step 3 */
	res = kx126_i2c_read(client, KX126_1063_DCST_RESP, &result, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;

	GSE_LOG("step3: result = %x", result);
	if (result != 0xAA)
		return -EINVAL;

	/*step 4 */
	res = kx126_i2c_read(client, KX126_1063_DCST_RESP, &result, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;

	GSE_LOG("step4: result = %x", result);
	if (result != 0x55)
		return -EINVAL;
	else
		return KX126_1063_SUCCESS;
}

/*----------------------------------------------------------------------------*/
#if 0
static int KX126_1063_JudgeTestResult(struct i2c_client *client, s32 prv[KX126_1063_AXES_NUM],
				      s32 nxt[KX126_1063_AXES_NUM])
{

	int re s = 0;
	u8 test_result = 0;

	res = hwmsen_read_byte(client, 0x0c, &test_result);
	if (res != 0)
		return res;

	GSE_LOG("test_result = %x\n", test_result);
	if (test_result != 0xaa) {
		GSE_ERR("KX126_1063_JudgeTestResult failt\n");
		res = -EINVAL;
	}
	return res;
}
#endif
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = KX126_1063_i2c_client;
	char strbuf[KX126_1063_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	KX126_1063_ReadChipInfo(client, strbuf, KX126_1063_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

#if 0
static ssize_t gsensor_init(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = KX126_1063_i2c_client;
	char strbuf[KX126_1063_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	KX126_1063_init_client(client, 1);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
#endif
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = KX126_1063_i2c_client;
	char strbuf[KX126_1063_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	KX126_1063_ReadSensorData(client, strbuf, KX126_1063_BUFSIZE);
	/*KX126_1063_ReadRawData(client, strbuf); */
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

#if 0
static ssize_t show_sensorrawdata_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = KX126_1063_i2c_client;
	char strbuf[KX126_1063_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	/*KX126_1063_ReadSensorData(client, strbuf, KX126_1063_BUFSIZE); */
	KX126_1063_ReadRawData(client, strbuf);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
#endif
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = KX126_1063_i2c_client;
	struct KX126_1063_i2c_data *obj;
	int err, len = 0, mul;
	int tmp[KX126_1063_AXES_NUM];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return -EINVAL;
	}

	obj = i2c_get_clientdata(client);
	err = KX126_1063_ReadOffset(client, obj->offset);
	if (err)
		return -EINVAL;

	err = KX126_1063_ReadCalibration(client, tmp);
	if (err)
		return -EINVAL;

	mul = obj->reso->sensitivity / KX126_1063_offset_resolution.sensitivity;
	len +=
	    snprintf(buf + len, PAGE_SIZE - len,
		     "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
		     obj->offset[KX126_1063_AXIS_X], obj->offset[KX126_1063_AXIS_Y],
		     obj->offset[KX126_1063_AXIS_Z], obj->offset[KX126_1063_AXIS_X],
		     obj->offset[KX126_1063_AXIS_Y], obj->offset[KX126_1063_AXIS_Z]);
	len +=
	    snprintf(buf + len, PAGE_SIZE - len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		     obj->cali_sw[KX126_1063_AXIS_X], obj->cali_sw[KX126_1063_AXIS_Y],
		     obj->cali_sw[KX126_1063_AXIS_Z]);

	len +=
	    snprintf(buf + len, PAGE_SIZE - len,
		     "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
		     obj->offset[KX126_1063_AXIS_X] * mul + obj->cali_sw[KX126_1063_AXIS_X],
		     obj->offset[KX126_1063_AXIS_Y] * mul + obj->cali_sw[KX126_1063_AXIS_Y],
		     obj->offset[KX126_1063_AXIS_Z] * mul + obj->cali_sw[KX126_1063_AXIS_Z],
		     tmp[KX126_1063_AXIS_X], tmp[KX126_1063_AXIS_Y],
		     tmp[KX126_1063_AXIS_Z]);

	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = KX126_1063_i2c_client;
	int err, x, y, z;
	int dat[KX126_1063_AXES_NUM];
	if (!strncmp(buf, "rst", 3)) {
		err = KX126_1063_ResetCalibration(client);
		if (err)
			GSE_ERR("reset offset err = %d\n", err);
	} else if (3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z)) {
		dat[KX126_1063_AXIS_X] = x;
		dat[KX126_1063_AXIS_Y] = y;
		dat[KX126_1063_AXIS_Z] = z;
		err = KX126_1063_WriteCalibration(client, dat);
		if (err)
			GSE_ERR("write calibration err = %d\n", err);
	} else
		GSE_ERR("invalid format\n");

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_self_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = KX126_1063_i2c_client;

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	return snprintf(buf, 8, "%s\n", selftestRes);
}

/*----------------------------------------------------------------------------*/
static ssize_t store_self_value(struct device_driver *ddri, const char *buf, size_t count)
{				/*write anything to this register will trigger the process */
	struct item {
		s16 raw[KX126_1063_AXES_NUM];
	};
	struct i2c_client *client = KX126_1063_i2c_client;
	int res, num;
	struct item *prv = NULL, *nxt = NULL;
	u8 data, data0;

	if (1 != sscanf(buf, "%d", &num)) {
		GSE_ERR("parse number fail\n");
		return count;
	} else if (num == 0) {
		GSE_ERR("invalid data count\n");
		return count;
	}

	prv = kcalloc(num, sizeof(*prv), GFP_KERNEL);
	nxt = kcalloc(num, sizeof(*nxt), GFP_KERNEL);
	if (!prv || !nxt)
		goto exit;

	GSE_LOG("NORMAL:\n");
	KX126_1063_SetPowerMode(client, true);

	/*initial setting for self test */
	if (!KX126_1063_InitSelfTest(client)) {
		GSE_LOG("SELFTEST : PASS\n");
		strcpy(selftestRes, "y");
	} else {
		GSE_LOG("SELFTEST : FAIL\n");
		strcpy(selftestRes, "n");
	}

	res = kx126_i2c_read(client, KX126_1063_REG_CTL_REG2, &data, 0x01);
	if (res != KX126_1063_SUCCESS)
		return res;

	data0 = ~KX126_1063_SELF_TEST & data;
	res = kx126_i2c_write(client, KX126_1063_REG_CTL_REG2, &data0, 0x01);
	if (res != KX126_1063_SUCCESS)	/*0x2C->BW=100Hz */
		return res;

exit:
	/*restore the setting */
	KX126_1063_init_client(client, 0);
	kfree(prv);
	kfree(nxt);
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = KX126_1063_i2c_client;
	struct KX126_1063_i2c_data *obj;

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->selftest));
}

/*----------------------------------------------------------------------------*/
static ssize_t store_selftest_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct KX126_1063_i2c_data *obj = obj_i2c_data;
	int tmp;

	if (NULL == obj) {
		GSE_ERR("i2c data obj is null!!\n");
		return 0;
	}


	if (1 == sscanf(buf, "%d", &tmp)) {
		if (atomic_read(&obj->selftest) && !tmp)
			/*enable -> disable */
			KX126_1063_init_client(obj->client, 0);
		else if (!atomic_read(&obj->selftest) && tmp)
			/*disable -> enable */
			KX126_1063_InitSelfTest(obj->client);

		GSE_LOG("selftest: %d => %d\n", atomic_read(&obj->selftest), tmp);
		atomic_set(&obj->selftest, tmp);
	} else
		GSE_ERR("invalid content: '%s', length = %d\n", buf, (int)count);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_KX126_1063_LOWPASS
	struct i2c_client *client = KX126_1063_i2c_client;
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);

	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);

		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++)
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][KX126_1063_AXIS_X],
				obj->fir.raw[idx][KX126_1063_AXIS_Y],
				obj->fir.raw[idx][KX126_1063_AXIS_Z]);

		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[KX126_1063_AXIS_X],
			obj->fir.sum[KX126_1063_AXIS_Y], obj->fir.sum[KX126_1063_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[KX126_1063_AXIS_X] / len,
			obj->fir.sum[KX126_1063_AXIS_Y] / len,
			obj->fir.sum[KX126_1063_AXIS_Z] / len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}

/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_KX126_1063_LOWPASS
	struct i2c_client *client = KX126_1063_i2c_client;
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if (1 != sscanf(buf, "%d", &firlen))
		GSE_ERR("invallid format\n");
	else if (firlen > C_MAX_FIR_LENGTH)
		GSE_ERR("exceeds maximum filter length\n");
	else {
		atomic_set(&obj->firlen, firlen);
		if (NULL == firlen)
			atomic_set(&obj->fir_en, 0);
		else {
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
	struct KX126_1063_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct KX126_1063_i2c_data *obj = obj_i2c_data;
	int trace;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace))
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s', length = %d\n", buf, (int)count);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct KX126_1063_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d %d (%d %d)\n",
				obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id,
				obj->hw->power_vol);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	}
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{
	u8 databuf[2];
	u8 addr = KX126_1063_REG_CTL_REG1;
	int ret = 0;

	ret = kx126_i2c_read(KX126_1063_i2c_client, addr, databuf, 0x01);
	if (ret < 0) {
		GSE_ERR("read power ctl register err!\n");
		return KX126_1063_ERR_I2C;
	}

	if (sensor_power)
		GSE_LOG("G sensor is in work mode, sensor_power = %d\n", sensor_power);
	else
		GSE_LOG("G sensor is in standby mode, sensor_power = %d\n", sensor_power);

	return snprintf(buf, PAGE_SIZE, "%x\n", databuf[0]);
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(selftest, S_IWUSR | S_IRUGO, show_self_value, store_self_value);
static DRIVER_ATTR(self, S_IWUSR | S_IRUGO, show_selftest_value, store_selftest_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO, show_firlen_value, store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(powerstatus, S_IRUGO, show_power_status_value, NULL);

/*----------------------------------------------------------------------------*/
static u8 i2c_dev_reg;

static ssize_t show_register(struct device_driver *pdri, char *buf)
{
	GSE_LOG("i2c_dev_reg is 0x%2x\n", i2c_dev_reg);

	return 0;
}

static ssize_t store_register(struct device_driver *ddri, const char *buf, size_t count)
{
	i2c_dev_reg = simple_strtoul(buf, NULL, 16);
	GSE_LOG("set i2c_dev_reg = 0x%2x\n", i2c_dev_reg);

	return 0;
}

static ssize_t store_register_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct KX126_1063_i2c_data *obj = obj_i2c_data;
	u8 databuf[2];
	unsigned long input_value;
	int res;

	memset(databuf, 0, sizeof(u8) * 2);

	input_value = simple_strtoul(buf, NULL, 16);
	GSE_LOG("input_value = 0x%2x\n", (unsigned int)input_value);

	if (NULL == obj) {
		GSE_ERR("i2c data obj is null!!\n");
		return 0;
	}

	databuf[0] = input_value;
	GSE_LOG("databuf[0]=0x%2x  databuf[1]=0x%2x\n", databuf[0], databuf[1]);

	res = kx126_i2c_write(obj->client, i2c_dev_reg, databuf, 0x01);
	/*res = i2c_master_send(obj->client, databuf, 0x2); */
	if (res < 0)
		return KX126_1063_ERR_I2C;

	return 0;
}

static ssize_t show_register_value(struct device_driver *ddri, char *buf)
{
	struct KX126_1063_i2c_data *obj = obj_i2c_data;
	u8 databuf[1];
	int ret = 0;

	memset(databuf, 0, sizeof(u8) * 1);

	if (NULL == obj) {
		GSE_ERR("i2c data obj is null!!\n");
		return 0;
	}

	ret = kx126_i2c_read(obj->client, i2c_dev_reg, databuf, 0x01);
	if (ret < 0) {
		GSE_ERR("read power ctl register err!\n");
		return KX126_1063_ERR_I2C;
	}

	GSE_LOG("i2c_dev_reg=0x%2x  data=0x%2x\n", i2c_dev_reg, databuf[0]);

	return 0;
}


static DRIVER_ATTR(i2c, S_IWUSR | S_IRUGO, show_register_value, store_register_value);
static DRIVER_ATTR(register, S_IWUSR | S_IRUGO, show_register, store_register);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *KX126_1063_attr_list[] = {
	&driver_attr_chipinfo,	/*chip information */
	&driver_attr_sensordata,	/*dump sensor data */
	&driver_attr_cali,	/*show calibration data */
	&driver_attr_self,	/*self test demo */
	&driver_attr_selftest,	/*self control: 0: disable, 1: enable */
	&driver_attr_firlen,	/*filter length: 0: disable, others: enable */
	&driver_attr_trace,	/*trace log */
	&driver_attr_status,
	&driver_attr_powerstatus,
	&driver_attr_register,
	&driver_attr_i2c,
};

/*----------------------------------------------------------------------------*/
static int KX126_1063_create_attr(struct device_driver *driver)
{
	int idx = 0, err = 0;
	int num = (int)(sizeof(KX126_1063_attr_list) / sizeof(KX126_1063_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, KX126_1063_attr_list[idx]);
		if (err != 0) {
			GSE_ERR("driver_create_file (%s) = %d\n",
				KX126_1063_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(KX126_1063_attr_list) / sizeof(KX126_1063_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, KX126_1063_attr_list[idx]);

	return err;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
/*----------------------------------------------------------------------------*/
#ifdef CUSTOM_KERNEL_SENSORHUB
static void KX126_1063_irq_work(struct work_struct *work)
{
	struct KX126_1063_i2c_data *obj = obj_i2c_data;
	struct scp_acc_hw scp_hw;
	KX126_1063_CUST_DATA *p_cust_data;
	SCP_SENSOR_HUB_DATA data;
	int max_cust_data_size_per_packet;
	int i;
	uint sizeOfCustData;
	uint len;
	char *p = (char *)&scp_hw;

	GSE_FUN();

	scp_hw.i2c_num = obj->hw->i2c_num;
	scp_hw.direction = obj->hw->direction;
	scp_hw.power_id = obj->hw->power_id;
	scp_hw.power_vol = obj->hw->power_vol;
	scp_hw.firlen = obj->hw->firlen;
	memcpy(scp_hw.i2c_addr, obj->hw->i2c_addr, sizeof(obj->hw->i2c_addr));
	scp_hw.power_vio_id = obj->hw->power_vio_id;
	scp_hw.power_vio_vol = obj->hw->power_vio_vol;
	scp_hw.is_batch_supported = obj->hw->is_batch_supported;

	p_cust_data = (KX126_1063_CUST_DATA *) data.set_cust_req.custData;
	sizeOfCustData = sizeof(scp_hw);
	max_cust_data_size_per_packet =
	    sizeof(data.set_cust_req.custData) - offsetof(KX126_1063_SET_CUST, data);

	for (i = 0; sizeOfCustData > 0; i++) {
		data.set_cust_req.sensorType = ID_ACCELEROMETER;
		data.set_cust_req.action = SENSOR_HUB_SET_CUST;
		p_cust_data->setCust.action = KX126_1063_CUST_ACTION_SET_CUST;
		p_cust_data->setCust.part = i;
		if (sizeOfCustData > max_cust_data_size_per_packet)
			len = max_cust_data_size_per_packet;
		else
			len = sizeOfCustData;

		memcpy(p_cust_data->setCust.data, p, len);
		sizeOfCustData -= len;
		p += len;

		len +=
		    offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + offsetof(KX126_1063_SET_CUST,
									       data);
		SCP_sensorHub_req_send(&data, &len, 1);
	}

	/*KX126_1063_ResetCalibration */
	p_cust_data = (KX126_1063_CUST_DATA *) &data.set_cust_req.custData;

	data.set_cust_req.sensorType = ID_ACCELEROMETER;
	data.set_cust_req.action = SENSOR_HUB_SET_CUST;
	p_cust_data->resetCali.action = KX126_1063_CUST_ACTION_RESET_CALI;
	len = offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(p_cust_data->resetCali);
	SCP_sensorHub_req_send(&data, &len, 1);

	obj->SCP_init_done = 1;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_irq_handler(void *data, uint len)
{
	struct KX126_1063_i2c_data *obj = obj_i2c_data;
	SCP_SENSOR_HUB_DATA_P rsp = (SCP_SENSOR_HUB_DATA_P) data;

	if (!obj)
		return -1;

	switch (rsp->rsp.action) {
	case SENSOR_HUB_NOTIFY:
		switch (rsp->notify_rsp.event) {
		case SCP_INIT_DONE:
			schedule_work(&obj->irq_work);
			break;
		default:
			GSE_ERR("Error sensor hub notify");
			break;
		}
		break;
	default:
		GSE_ERR("Error sensor hub action");
		break;
	}

	return 0;
}

static int KX126_1063_setup_irq(void)
{
	int err = 0;

	err = SCP_sensorHub_rsp_registration(ID_ACCELEROMETER, KX126_1063_irq_handler);

	return err;
}
#endif
/*----------------------------------------------------------------------------*/
static int KX126_1063_open(struct inode *inode, struct file *file)
{
	file->private_data = KX126_1063_i2c_client;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

#ifdef CONFIG_COMPAT
static long KX126_1063_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
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
                err = file->f_op->unlocked_ioctl(file,
                        GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
                if (err) {
                        GSE_ERR("GSENSOR_IOCTL_SET_CALI failed.");
                        return err;
                }
                break;
        case COMPAT_GSENSOR_IOCTL_GET_CALI:
                if (arg32 == NULL) {
                        err = -EINVAL;
                        break;
                }
                err = file->f_op->unlocked_ioctl(file,
                        GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
                if (err) {
                        GSE_ERR("GSENSOR_IOCTL_GET_CALI failed.");
                        return err;
                }
                break;
        case COMPAT_GSENSOR_IOCTL_CLR_CALI:
                if (arg32 == NULL) {
                        err = -EINVAL;
                        break;
                }
                err = file->f_op->unlocked_ioctl(file,
                        GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
                if (err) {
                        GSE_ERR("GSENSOR_IOCTL_CLR_CALI failed.");
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
/*static int KX126_1063_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
       unsigned long arg)*/
static long KX126_1063_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct KX126_1063_i2c_data *obj = (struct KX126_1063_i2c_data *)i2c_get_clientdata(client);
	char strbuf[KX126_1063_BUFSIZE];
	void __user *data;
	struct SENSOR_DATA sensor_data;
	long err = 0;
	int cali[3];

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		KX126_1063_init_client(client, 0);
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		KX126_1063_ReadChipInfo(client, strbuf, KX126_1063_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_SENSORDATA:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		KX126_1063_SetPowerMode(obj->client, true);
		KX126_1063_ReadSensorData(client, strbuf, KX126_1063_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_GAIN:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_RAW_DATA:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		KX126_1063_ReadRawData(client, strbuf);
		if (copy_to_user(data, &strbuf, strlen(strbuf) + 1)) {
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
			cali[KX126_1063_AXIS_X] =
			    sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[KX126_1063_AXIS_Y] =
			    sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[KX126_1063_AXIS_Z] =
			    sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			err = KX126_1063_WriteCalibration(client, cali);
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		err = KX126_1063_ResetCalibration(client);
		break;

	case GSENSOR_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		err = KX126_1063_ReadCalibration(client, cali);
		if (err != 0)
			break;

		sensor_data.x =
		    cali[KX126_1063_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.y =
		    cali[KX126_1063_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.z =
		    cali[KX126_1063_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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

/*----------------------------------------------------------------------------*/
static struct file_operations KX126_1063_fops = {
	.owner = THIS_MODULE,
	.open = KX126_1063_open,
	.release = KX126_1063_release,
	.unlocked_ioctl = KX126_1063_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = KX126_1063_compat_ioctl,
#endif
};

/*----------------------------------------------------------------------------*/
static struct miscdevice KX126_1063_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &KX126_1063_fops,
};


/*----------------------------------------------------------------------------*/
#if !defined(CONFIG_HAS_EARLYSUSPEND) || !defined(USE_EARLY_SUSPEND)
/*----------------------------------------------------------------------------*/
static int KX126_1063_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
    u8 flag = 0;
	GSE_FUN();

		if (obj == NULL) {
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		//mutex_lock(&KX126_1063_mutex);
		atomic_set(&obj->suspend, 1);
        KX126_1063_suspend_flag = 1; 
        err = kx126_i2c_read(obj->client,KX126_1063_REG_CTL_REG1,&flag,1);
        if(err < 0){
            GSE_ERR("KX126_1063 i2c err \n");
            return -EINVAL;
        }
        if(flag & 0x02){
            GSE_LOG("KX126_1063_suspend step_COUNTER OPEN flag=0x%x\n",flag);
            return 0; 
        } 
#ifdef CUSTOM_KERNEL_SENSORHUB
		err = KX126_1063_SCP_SetPowerMode(false);
		if (err != 0)
#else
		err = KX126_1063_SetPowerMode(obj->client, false);
		if (err != 0)
#endif
		{
			GSE_ERR("write power control fail!!\n");
			//mutex_unlock(&KX126_1063_mutex);
			return -1;
		}
		//mutex_unlock(&KX126_1063_mutex);
#ifndef CUSTOM_KERNEL_SENSORHUB
		KX126_1063_power(obj->hw, 0);
#endif
	return err;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
	int err;
    u8 flag =0;

	GSE_FUN();
	
	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

    err = kx126_i2c_read(obj->client,KX126_1063_REG_CTL_REG1,&flag,1);
    if(err < 0){
        GSE_ERR("KX126_1063 i2c err \n");
        return -EINVAL;
    }
    if(flag & 0x02){
        GSE_LOG("KX126_1063_resume step_COUNTER OPEN flag=0x%x\n",flag);
        atomic_set(&obj->suspend, 0);
        KX126_1063_suspend_flag = 0; 
        return 0; 
    }
    
#ifndef CUSTOM_KERNEL_SENSORHUB
	KX126_1063_power(obj->hw, 1);
#endif
	//mutex_lock(&KX126_1063_mutex);
#ifdef CUSTOM_KERNEL_SENSORHUB
	err = KX126_1063_SCP_SetPowerMode(enable_status);
	if (err != 0)
#else
	err = KX126_1063_init_client(client, 0);
	if (err != 0)
#endif
	{
		GSE_ERR("initialize client fail!!\n");
		//mutex_unlock(&KX126_1063_mutex);
		return err;
	}
	atomic_set(&obj->suspend, 0);
    KX126_1063_suspend_flag=0;	
    //mutex_unlock(&KX126_1063_mutex);

	return 0;
}

/*----------------------------------------------------------------------------*/
#else
/*----------------------------------------------------------------------------*/
static void KX126_1063_early_suspend(struct early_suspend *h)
{
	struct KX126_1063_i2c_data *obj = container_of(h, struct KX126_1063_i2c_data, early_drv);
	int err;

	GSE_FUN();
	
	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}
	//mutex_lock(&KX126_1063_mutex);
	atomic_set(&obj->suspend, 1);
#ifdef CUSTOM_KERNEL_SENSORHUB
	err = KX126_1063_SCP_SetPowerMode(false);
#else
	err = KX126_1063_SetPowerMode(obj->client, false);
#endif
	if (err != 0) {
		GSE_ERR("write power control fail!!\n");
		//mutex_unlock(&KX126_1063_mutex);
		return;
	}
	//mutex_unlock(&KX126_1063_mutex);

#ifndef CUSTOM_KERNEL_SENSORHUB
	KX126_1063_power(obj->hw, 0);
#endif
}

/*----------------------------------------------------------------------------*/
static void KX126_1063_late_resume(struct early_suspend *h)
{
	struct KX126_1063_i2c_data *obj = container_of(h, struct KX126_1063_i2c_data, early_drv);
	int err;

	GSE_FUN();
	
	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}
#ifndef CUSTOM_KERNEL_SENSORHUB
	KX126_1063_power(obj->hw, 1);
#endif
	//mutex_lock(&KX126_1063_mutex);
#ifdef CUSTOM_KERNEL_SENSORHUB
	err = KX126_1063_SCP_SetPowerMode(enable_status);
#else
	err = KX126_1063_init_client(obj->client, 0);
#endif
	if (err) {
		GSE_ERR("initialize client fail!!\n");
		//mutex_unlock(&KX126_1063_mutex);
		return;
	}
	atomic_set(&obj->suspend, 0);
	//mutex_unlock(&KX126_1063_mutex);
}

/*----------------------------------------------------------------------------*/
#endif				/*!defined(CONFIG_HAS_EARLYSUSPEND) || !defined(USE_EARLY_SUSPEND) */
/*----------------------------------------------------------------------------*/

/*/ if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL*/
static int KX126_1063_open_report_data(int open)
{
	/*should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL*/

static int KX126_1063_enable_nodata(int en)
{
    struct i2c_client *client = KX126_1063_i2c_client;
    struct KX126_1063_i2c_data *obj = i2c_get_clientdata(client);
    int err = 0;
    u8 flag = 0;
    err = kx126_i2c_read(obj->client,KX126_1063_REG_CTL_REG1,&flag,1);
    if(err < 0){          
        GSE_ERR("KX126_1063 i2c err \n");
        return -EINVAL;   
    }                     
	//mutex_lock(&KX126_1063_mutex); 
	if (((en == 0) && (sensor_power == false)) || ((en == 1) && (sensor_power == true))) {
		enable_status = sensor_power;
		GSE_LOG("Gsensor device have updated!\n");
	} else {
		enable_status = !sensor_power;
		if (atomic_read(&obj_i2c_data->suspend) == 0) {
            if(flag & 0x02){      
                GSE_LOG("KX126_1063_enable nodata step_COUNTER OPEN flag=0x%x\n",flag);
                return 0;         
            } 
            #ifdef CUSTOM_KERNEL_SENSORHUB
			err = KX126_1063_SCP_SetPowerMode(enable_status);
#else				/*#ifdef CUSTOM_KERNEL_SENSORHUB */
			err = KX126_1063_SetPowerMode(obj_i2c_data->client, enable_status);
#endif
			GSE_LOG
			    ("Gsensor not in suspend KX126_1063_SetPowerMode!, enable_status = %d\n",
			     enable_status);
		} else
			GSE_LOG
			    ("Gsensor in suspend and can not enable or disable!enable_status = %d\n",
			     enable_status);
	}

	//mutex_unlock(&KX126_1063_mutex);
	if (err != KX126_1063_SUCCESS) {
		GSE_LOG("KX126_1063_enable_nodata fail!\n");
		return -1;
	}

	GSE_LOG("KX126_1063_enable_nodata OK!\n");
	return 0;
}

static int KX126_1063_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	ACC_LOG("%s\n", __func__);
	return 0;
}

static int KX126_1063_flush(void)
{
	return acc_flush_report();
}

static int KX126_1063_set_delay(u64 ns)
{
	int err = 0;
	int value;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#else
	int sample_delay;
#endif

	value = (int)ns / 1000 / 1000;

#ifdef CUSTOM_KERNEL_SENSORHUB
	req.set_delay_req.sensorType = ID_ACCELEROMETER;
	req.set_delay_req.action = SENSOR_HUB_SET_DELAY;
	req.set_delay_req.delay = value;
	len = sizeof(req.activate_req);
	err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err) {
		GSE_ERR("SCP_sensorHub_req_send!\n");
		return err;
	}
#else
	if (value <= 5)
		sample_delay = KX126_1063_BW_200HZ;
	else if (value <= 10)
		sample_delay = KX126_1063_BW_100HZ;
	else
		//sample_delay = KX126_1063_BW_50HZ;
        sample_delay = KX126_1063_BW_100HZ;

	//mutex_lock(&KX126_1063_mutex); 
	err = KX126_1063_SetBWRate(obj_i2c_data->client, sample_delay);
	//mutex_unlock(&KX126_1063_mutex);
	if (err != KX126_1063_SUCCESS) {
		GSE_ERR("Set delay parameter error!\n");
		return -1;
	}

	if (value >= 50)
		atomic_set(&obj_i2c_data->filter, 0);
	else {
#if defined(CONFIG_KX126_1063_LOWPASS)
		priv->fir.num = 0;
		priv->fir.idx = 0;
		priv->fir.sum[KX126_1063_AXIS_X] = 0;
		priv->fir.sum[KX126_1063_AXIS_Y] = 0;
		priv->fir.sum[KX126_1063_AXIS_Z] = 0;
		atomic_set(&priv->filter, 1);
#endif
	}
#endif

    GSE_LOG("KX126_1063_set_delay (%d)\n", value);

	return 0;
}

static int KX126_1063_get_data(int *x, int *y, int *z, int *status)
{
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
	int err = 0;
#else
	char buff[KX126_1063_BUFSIZE];
#endif

#ifdef CUSTOM_KERNEL_SENSORHUB
	req.get_data_req.sensorType = ID_ACCELEROMETER;
	req.get_data_req.action = SENSOR_HUB_GET_DATA;
	len = sizeof(req.get_data_req);
	err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err) {
		GSE_ERR("SCP_sensorHub_req_send!\n");
		return err;
	}

	if (ID_ACCELEROMETER != req.get_data_rsp.sensorType ||
	    SENSOR_HUB_GET_DATA != req.get_data_rsp.action || 0 != req.get_data_rsp.errCode) {
		GSE_ERR("error : %d\n", req.get_data_rsp.errCode);
		return req.get_data_rsp.errCode;
	}

	*x = (int)req.get_data_rsp.int16_Data[0] * GRAVITY_EARTH_1000 / 1000;
	*y = (int)req.get_data_rsp.int16_Data[1] * GRAVITY_EARTH_1000 / 1000;
	*z = (int)req.get_data_rsp.int16_Data[2] * GRAVITY_EARTH_1000 / 1000;
	GSE_ERR("x = %d, y = %d, z = %d\n", *x, *y, *z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
#else
	//mutex_lock(&KX126_1063_mutex); 
	KX126_1063_ReadSensorData(obj_i2c_data->client, buff, KX126_1063_BUFSIZE);
	//mutex_unlock(&KX126_1063_mutex); 
	sscanf(buff, "%x %x %x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
#endif
	return 0;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct KX126_1063_i2c_data *obj;
	struct acc_control_path ctl = { 0 };
	struct acc_data_path data = { 0 };
	int err = 0;
//+add by hzb for ontim debug
        if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
        {
           return -EIO;
        }
//-add by hzb for ontim debug
	GSE_FUN();

	err = get_accel_dts_func(client->dev.of_node, hw);
	if (err < 0) {
		GSE_ERR("get cust_baro dts info fail\n");
		goto exit;
	}
	GSE_ERR("i2c_num=0x%x,I2C_ADDRS=0x%x;0x%x;\n",hw->i2c_num,hw->i2c_addr[0],client->addr);

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!(obj)) {
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct KX126_1063_i2c_data));

	obj->hw = hw;
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (0 != err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}
#ifdef CUSTOM_KERNEL_SENSORHUB
	INIT_WORK(&obj->irq_work, KX126_1063_irq_work);
#endif

	obj_i2c_data = obj;
	//obj_i2c_data_kx126 = obj;
	
    obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
    KX126_1063_suspend_flag = 0;    

#ifdef CUSTOM_KERNEL_SENSORHUB
	obj->SCP_init_done = 0;
#endif

#ifdef CONFIG_KX126_1063_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);

	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);

#endif

	KX126_1063_i2c_client = new_client;
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(KXTJ_ENABLE_I2C_DMA)
	if (msg_dma_alloc())
		goto exit;
#endif

	err = KX126_1063_init_client(new_client, 1);
	if (0 != err) {
		GSE_ERR("KX126_1063_init_client fail\n");
		goto exit_init_failed;
	}

	err = misc_register(&KX126_1063_device);
	if (0 != err) {
		GSE_ERR("KX126_1063_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	err = KX126_1063_create_attr(&KX126_1063_init_info.platform_diver_addr->driver);
	if (0 != err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data = KX126_1063_open_report_data;
	ctl.enable_nodata = KX126_1063_enable_nodata;
	ctl.set_delay = KX126_1063_set_delay;
	ctl.batch = KX126_1063_batch;
	ctl.flush = KX126_1063_flush;
	/*ctl.batch = KX126_1063_set_batch; */
	ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	ctl.is_support_batch = obj->hw->is_batch_supported;
#else
	ctl.is_support_batch = false;
#endif

	err = acc_register_control_path(&ctl);
	if (err) {
		GSE_ERR("register acc control path err\n");
		goto exit_kfree;
	}
	

	data.get_data = KX126_1063_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if (err) {
		GSE_ERR("register acc data path err\n");
		goto exit_kfree;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(USE_EARLY_SUSPEND)
	obj->early_drv.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	    obj->early_drv.suspend = KX126_1063_early_suspend,
	    obj->early_drv.resume = KX126_1063_late_resume, register_early_suspend(&obj->early_drv);
#endif

	KX126_1063_init_flag = 0;
//+add by hzb for ontim debug
        REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
//-add by hzb for ontim debug
	GSE_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&KX126_1063_device);
exit_misc_device_register_failed:
exit_init_failed:
exit_kfree:
	kfree(obj);
exit:
	GSE_ERR("%s: err = %d\n", __func__, err);
	KX126_1063_init_flag = -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = KX126_1063_delete_attr(&(KX126_1063_init_info.platform_diver_addr->driver));
	if (0 != err)
		GSE_ERR("KX126_1063_delete_attr fail: %d\n", err);

	 misc_deregister(&KX126_1063_device);

	KX126_1063_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}

/*----------------------------------------------------------------------------*/
static int KX126_1063_remove(void)
{
	GSE_FUN();
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(KXTJ_ENABLE_I2C_DMA)
	msg_dma_release();
#endif
	KX126_1063_power(hw, 0);
	i2c_del_driver(&KX126_1063_i2c_driver);
	return 0;
}

static int KX126_1063_local_init(void)
{
	GSE_ERR("KX126_1063_local_init entry\n");

	KX126_1063_power(hw, 1);
	if (i2c_add_driver(&KX126_1063_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return -1;
	}
	if (-1 == KX126_1063_init_flag)
		return -1;

	return 0;
}

/*----------------------------------------------------------------------------*/
static int __init KX126_1063_init(void)
{

	GSE_FUN();
	acc_driver_add(&KX126_1063_init_info);
	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit KX126_1063_exit(void)
{
	GSE_FUN();
}

/*----------------------------------------------------------------------------*/
module_init(KX126_1063_init);
module_exit(KX126_1063_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KX126_1063 I2C driver");
MODULE_AUTHOR("www.kionix.com");
