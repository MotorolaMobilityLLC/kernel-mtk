#ifndef _AW9610X_H_
#define _AW9610X_H_

#include <linux/regmap.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/leds.h>
#include <linux/regulator/consumer.h>

#define AW_POWER_ON

#define AW9610X_CHIP_ID				(0xa961)
#define AW_SAR_SUCCESS				(0)
#define AW_VCC_MIN_UV				(1700000)
#define AW_VCC_MAX_UV				(3600000)
#define AW_SPE_REG_NUM				(8)
#define AW_CLA1_SPE_REG_NUM			(6)
#define AW_SPE_REG_DWORD			(8)
#define AW_DATA_PROCESS_FACTOR				(1024)

#define INPUT_REPORT_TYPE_KEY

#ifdef INPUT_REPORT_TYPE_KEY
#define KEY_SAR_NEAR   0x2ec
#define KEY_SAR_CLOSE  0x2ed
#define KEY_SAR_FAR    0x2ef
#endif

#define AWLOGD(dev, format, arg...) \
	do {\
		 dev_printk(KERN_DEBUG, dev, \
			"[%s:%d] "format"\n", __func__, __LINE__, ##arg);\
	} while (0)

#define AWLOGI(dev, format, arg...) \
	do {\
		dev_printk(KERN_INFO, dev, \
			"[%s:%d] "format"\n", __func__, __LINE__, ##arg);\
	} while (0)

#define AWLOGE(dev, format, arg...) \
	do {\
		 dev_printk(KERN_ERR, dev, \
			"[%s:%d] "format"\n", __func__, __LINE__, ##arg);\
	} while (0)

/**********************************************
* cfg load situation
**********************************************/
enum aw9610x_cfg_situ {
	AW_CFG_UNLOAD,
	AW_CFG_LOADED,
};

/**********************************************
*spereg cali flag
**********************************************/
enum aw9610x_cali_flag {
	AW_NO_CALI,
	AW_CALI,
};

enum aw9610x_sar_vers {
	AW9610X = 2,
	AW9610XA = 6,
	AW9610XB = 0xa,
};

enum aw9610x_operation_mode {
	AW9610X_ACTIVE_MODE = 1,
	AW9610X_SLEEP_MODE,
	AW9610X_DEEPSLEEP_MODE,
};

/**********************************************
*spereg addr offset
**********************************************/
enum aw9610x_spereg_addr_offset {
	AW_CL1SPE_CALI_OS = 20,
	AW_CL1SPE_DEAL_OS = 60,
	AW_CL2SPE_CALI_OS = 4,
	AW_CL2SPE_DEAL_OS = 4,
};

/**********************************************
 *the flag of i2c read/write
 **********************************************/
enum aw9610x_function_flag {
	AW9610X_FUNC_OFF,
	AW9610X_FUNC_ON,
};

/**********************************************
 *the flag of i2c read/write
 **********************************************/
enum aw9610x_i2c_flags {
	AW9610X_I2C_WR,
	AW9610X_I2C_RD,
};

/*********************************************************
* aw9610x error flag:
* @AW_MALLOC_FAILED: malloc space failed.
* @AW_CHIPID_FAILED: the chipid is error.
* @AW_IRQIO_FAILED: irq gpio invalid.
* @AW_IRQ_REQUEST_FAILED: irq requst failed.
* @AW_INPUT_ALLOCATE_FILED : inpt malloc failed.
* @AW_INPUT_REGISTER_FAILED : inpt_dev register failed.
* @AW_CFG_LOAD_TIME_FAILED : cfg situation not confirmed.
* @AW_TRIM_ERROR : the chip no trim.
* @AW_IRQGPIO_FAILED : irq gpio is invalid.
* @AW_MULTIPLE_SAR_FAILED : sar-num is error in aw9610x.dtsi
**********************************************************/
enum aw9610x_err_flags {
	AW_MALLOC_FAILED = 200,
	AW_CHIPID_FAILED,
	AW_IRQIO_FAILED,
	AW_IRQ_REQUEST_FAILED,
	AW_INPUT_ALLOCATE_FILED,
	AW_INPUT_REGISTER_FAILED,
	AW_CFG_LOAD_TIME_FAILED,
	AW_TRIM_ERROR,
	AW_IRQGPIO_FAILED,
	AW_MULTIPLE_SAR_FAILED,
	AW_VERS_ERR,
};

/**********************************************
* mutiple sar define
**********************************************/
enum aw9610x_multiple_sar {
	AW_SAR0,
	AW_SAR1,
	AW_SAR_MAX,
};

/**********************************************
* mutiple channel define
**********************************************/
enum aw9610x_ichannel {
	AW_CHANNEL0,
	AW_CHANNEL1,
	AW_CHANNEL2,
	AW_CHANNEL3,
	AW_CHANNEL4,
	AW_CHANNEL5,
	AW_CHANNEL_MAX,
};

/************************************************************
* Interrupts near or far from the threshold will be triggered
*************************************************************/
enum aw9610x_irq_trigger_position {
	FAR,
	TRIGGER_TH0,
	TRIGGER_TH1 = 0x03,
	TRIGGER_TH2 = 0x07,
	TRIGGER_TH3 = 0x0f,
};


struct aw_i2c_package {
	uint8_t addr_bytes;
	uint8_t data_bytes;
	uint8_t reg_num;
	uint8_t init_addr[4];
	uint8_t *p_reg_data;
};

struct aw_pad {
	uint8_t curr_state;
	uint8_t last_state;
	struct input_dev *input;
	uint8_t *name;
};

struct aw9610x {
	uint8_t cali_flag;
	uint8_t power_prox;
	uint8_t vers;
	uint8_t mode_flag0;
	uint8_t mode_flag1;
	uint8_t pad;

	uint32_t sar_num;
	int32_t irq_gpio;
	int32_t to_irq;
	uint32_t irq_status;
	uint32_t hostirqen;
	uint32_t first_irq_flag;
	uint32_t mode;
	bool pwprox_dete;
	bool firmware_flag;
	bool satu_release;
	bool power_enable;

	struct delayed_work cfg_work;
	struct i2c_client *i2c;
	struct device *dev;
	struct delayed_work dworker; /* work struct for worker function */
	struct aw_bin *aw_bin;
	struct aw_i2c_package aw_i2c_package;
	struct aw_pad *aw_pad;
	struct regulator *vcc;
	uint8_t satu_flag[6];
	uint32_t spedata[8];
	uint32_t satu_data[6];
	uint32_t nvspe_data[8];
	uint8_t chip_name[9];
	uint8_t cfg_name[20];
	uint8_t cfg_vers[2];

	#ifdef CONFIG_CAPSENSE_USB_CAL
    struct work_struct ps_notify_work;
    struct notifier_block ps_notif;
    bool ps_is_present;
#ifdef CONFIG_CAPSENSE_ATTACH_CAL
    bool phone_is_present;
#endif
#ifdef CONFIG_CAPSENSE_FLIP_CAL
    struct notifier_block flip_notif;
    struct extcon_dev *ext_flip_det;
    bool phone_flip_state;
    bool phone_flip_update_regs;
    int phone_flip_open_val;
    int num_flip_closed_regs;
    int num_flip_open_regs;
    struct smtc_reg_data *flip_open_regs;
    struct smtc_reg_data *flip_closed_regs;
#endif
#endif
};

struct aw9610x_cfg {
	int len;
	unsigned int data[];
};

struct aw_pad pad_event[] =
{
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar0_pad0",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "CapSense_Bottom",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar0_pad2",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "CapSense_Top",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar0_pad4",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar0_pad5",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar1_pad0",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar1_pad1",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar1_pad2",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar1_pad3",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar1_pad4",
	},
	{
		.curr_state = 0,
		.last_state = 0,
		.name = "sar1_pad5",
	},
};

uint32_t attr_buf[] = {
	8, 10,
	9, 100,
	10, 1000,
};

/******************************************************
* Register Detail
******************************************************/
uint32_t aw9610x_reg_default[] = {
	0x0000, 0x00003f3f,
	0x0004, 0x00000064,
	0x0008, 0x0017c11e,
	0x000c, 0x05000000,
	0x0010, 0x00093ffd,
	0x0014, 0x19240009,
	0x0018, 0xd81c0207,
	0x001c, 0xff000000,
	0x0020, 0x00241900,
	0x0024, 0x00093ff7,
	0x0028, 0x58020009,
	0x002c, 0xd81c0207,
	0x0030, 0xff000000,
	0x0034, 0x00025800,
	0x0038, 0x00093fdf,
	0x003c, 0x7d3b0009,
	0x0040, 0xd81c0207,
	0x0044, 0xff000000,
	0x0048, 0x003b7d00,
	0x004c, 0x00093f7f,
	0x0050, 0xe9310009,
	0x0054, 0xd81c0207,
	0x0058, 0xff000000,
	0x005c, 0x0031e900,
	0x0060, 0x00093dff,
	0x0064, 0x1a0c0009,
	0x0068, 0xd81c0207,
	0x006c, 0xff000000,
	0x0070, 0x000c1a00,
	0x0074, 0x80093fff,
	0x0078, 0x043d0009,
	0x007c, 0xd81c0207,
	0x0080, 0xff000000,
	0x0084, 0x003d0400,
	0x00a0, 0xe6400000,
	0x00a4, 0x00000000,
	0x00a8, 0x010408d2,
	0x00ac, 0x00000000,
	0x00b0, 0x00000000,
	0x00b8, 0x00005fff,
	0x00bc, 0x00000000,
	0x00c0, 0x00000000,
	0x00c4, 0x00000000,
	0x00c8, 0x00000000,
	0x00cc, 0x00000000,
	0x00d0, 0x00000000,
	0x00d4, 0x00000000,
	0x00d8, 0x00000000,
	0x00dc, 0xe6447800,
	0x00e0, 0x78000000,
	0x00e4, 0x010408d2,
	0x00e8, 0x00000000,
	0x00ec, 0x00000000,
	0x00f4, 0x00005fff,
	0x00f8, 0x00000000,
	0x00fc, 0x00000000,
	0x0100, 0x00000000,
	0x0104, 0x00000000,
	0x0108, 0x00000000,
	0x010c, 0x02000000,
	0x0110, 0x00000000,
	0x0114, 0x00000000,
	0x0118, 0xe6447800,
	0x011c, 0x78000000,
	0x0120, 0x010408d2,
	0x0124, 0x00000000,
	0x0128, 0x00000000,
	0x0130, 0x00005fff,
	0x0134, 0x00000000,
	0x0138, 0x00000000,
	0x013c, 0x00000000,
	0x0140, 0x00000000,
	0x0144, 0x00000000,
	0x0148, 0x02000000,
	0x014c, 0x00000000,
	0x0150, 0x00000000,
	0x0154, 0xe6447800,
	0x0158, 0x78000000,
	0x015c, 0x010408d2,
	0x0160, 0x00000000,
	0x0164, 0x00000000,
	0x016c, 0x00005fff,
	0x0170, 0x00000000,
	0x0174, 0x00000000,
	0x0178, 0x00000000,
	0x017c, 0x00000000,
	0x0180, 0x00000000,
	0x0184, 0x02000000,
	0x0188, 0x00000000,
	0x018c, 0x00000000,
	0x0190, 0xe6447800,
	0x0194, 0x78000000,
	0x0198, 0x010408d2,
	0x019c, 0x00000000,
	0x01a0, 0x00000000,
	0x01a8, 0x00005fff,
	0x01ac, 0x00000000,
	0x01b0, 0x00000000,
	0x01b4, 0x00000000,
	0x01b8, 0x00000000,
	0x01bc, 0x00000000,
	0x01c0, 0x02000000,
	0x01c4, 0x00000000,
	0x01c8, 0x00000000,
	0x01cc, 0xe6407800,
	0x01d0, 0x78000000,
	0x01d4, 0x010408d2,
	0x01d8, 0x00000000,
	0x01dc, 0x00000000,
	0x01e4, 0x00005fff,
	0x01e8, 0x00000000,
	0x01ec, 0x00000000,
	0x01f0, 0x00000000,
	0x01f4, 0x00000000,
	0x01f8, 0x00000000,
	0x01fc, 0x02000000,
	0x0200, 0x00000000,
	0x0204, 0x00000000,
	0x0208, 0x00000008,
	0x020c, 0x0000000d,
	0x41fc, 0x00000000,
	0x4400, 0x00000000,
	0x4410, 0x00000000,
	0x4420, 0x00000000,
	0x4430, 0x00000000,
	0x4440, 0x00000000,
	0x4450, 0x00000000,
	0x4460, 0x00000000,
	0x4470, 0x00000000,
	0xf080, 0x00003018,
	0xf084, 0x00000fff,
	0xf800, 0x00000000,
	0xf804, 0x00002e00,
	0xf8d0, 0x00000001,
	0xf8d4, 0x00000000,
	0xff00, 0x00000301,
	0xff0c, 0x01000000,
	0xffe0, 0x00000000,
	0xfff4, 0x00004011,
	0x0090, 0x00000000,
	0x0094, 0x00000000,
	0x0098, 0x00000000,
	0x009c, 0x3f3f3f3f,
};
#endif
