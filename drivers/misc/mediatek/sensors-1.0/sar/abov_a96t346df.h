/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
#ifndef ABOV_SAR_H
#define ABOV_SAR_H

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>


/*
 *  I2C Registers
 */
#define ABOV_VERSION_REG		    0x01
#define ABOV_MODELNO_REG		    0x02
#define ABOV_ABOV_WHOAMI_REG		0x03
#define ABOV_IRQSTAT_REG			0x04
#define ABOV_SOFTRESET_REG  		0x06
#define ABOV_CTRL_MODE_REG			0x07
#define ABOV_CTRL_CHANNEL_REG		0x08
#define ABOV_CH0_CAP_MSB_REG		0x18
#define ABOV_CH0_CAP_LSB_REG		0x19
#define ABOV_CH1_CAP_MSB_REG		0x1A
#define ABOV_CH1_CAP_LSB_REG		0x1B
#define ABOV_CH2_CAP_MSB_REG		0x1C
#define ABOV_CH2_CAP_LSB_REG		0x1D
#define ABOV_REF0_CAP_MSB_REG		0x1E
#define ABOV_REF0_CAP_LSB_REG		0x1F
#define ABOV_REF1_CAP_MSB_REG		0x20
#define ABOV_REF1_CAP_LSB_REG		0x21
#define ABOV_CH0_DIFF_MSB_REG		0x34
#define ABOV_CH0_DIFF_LSB_REG		0x35
#define ABOV_CH1_DIFF_MSB_REG		0x36
#define ABOV_CH1_DIFF_LSB_REG		0x37
#define ABOV_CH2_DIFF_MSB_REG		0x38
#define ABOV_CH2_DIFF_LSB_REG		0x39
#define ABOV_TH_LEVEL_EN_REG		0x3F
#define ABOV_RECALI_REG				0xFB

/* enable body stat */
#define ABOV_TCHCMPSTAT_TCHSTAT0_FLAG   0x03
/* enable body stat */
#define ABOV_TCHCMPSTAT_TCHSTAT1_FLAG   0x0C
/* enable body stat */
#define ABOV_TCHCMPSTAT_TCHSTAT2_FLAG   0x30


#define KEY_CAP_CS0  0x00
#define KEY_CAP_CS1  0x01
#define KEY_CAP_CS2  0x02



/**************************************
* define platform data
*
**************************************/
struct smtc_reg_data {
	unsigned char reg;
	unsigned char val;
};

typedef struct smtc_reg_data smtc_reg_data_t;
typedef struct smtc_reg_data *psmtc_reg_data_t;


struct _buttonInfo {
	/* The Key to send to the input */
	//int keycode;
	/* Mask to look for on Touch Status */
	int mask;
	/* Current state of button. */
	int state;
};

struct _totalButtonInformation {
	struct _buttonInfo *buttons;
	int buttonSize;
	//struct input_dev *input_top;
	//struct input_dev *input_bottom_lmb;
	//struct input_dev *input_bottom_hb;
};

typedef struct _totalButtonInformation buttonInformation_t;
typedef struct _totalButtonInformation *pbuttonInformation_t;

/* Define Registers that need to be initialized to values different than
 * default
 */
static struct smtc_reg_data abov_i2c_reg_setup[] = {
	{
		.reg = ABOV_CTRL_MODE_REG,
		.val = 0x00,
	},
	{
		.reg = ABOV_CTRL_CHANNEL_REG,
		.val = 0x1F,
	},
	{
		.reg = ABOV_RECALI_REG,
		.val = 0x01,
	},
};



static struct _buttonInfo psmtcButtons[] = {
	{
		//.keycode = KEY_CAP_CS0,
		.mask = ABOV_TCHCMPSTAT_TCHSTAT0_FLAG,
	},
	{
		//.keycode = KEY_CAP_CS1,
		.mask = ABOV_TCHCMPSTAT_TCHSTAT1_FLAG,
	},
	{
		//.keycode = KEY_CAP_CS2,
		.mask = ABOV_TCHCMPSTAT_TCHSTAT2_FLAG,
	},
};

struct abov_platform_data {
	int i2c_reg_num;
	struct smtc_reg_data *pi2c_reg;
	unsigned irq_gpio;
	/* used for custom setting for channel and scan period */
	int cap_channel_top;
	int cap_channel_bottom_lmb;
	int cap_channel_bottom_hb;
	pbuttonInformation_t pbuttonInformation;
	const char *fw_name;

	int (*get_is_nirq_low)(unsigned irq_gpio);
	int (*init_platform_hw)(void);
	void (*exit_platform_hw)(void);
};
typedef struct abov_platform_data abov_platform_data_t;
typedef struct abov_platform_data *pabov_platform_data_t;



/***************************************
* define data struct/interrupt
* @pdev: pdev common device struction for linux
* @dworker: work struct for worker function
* @board: constant pointer to platform data
* @mutex: mutex for interrupt process
* @lock: Spin Lock used for nirq worker function
* @bus: either i2c_client or spi_client
* @pDevice: device specific struct pointer
*@read_flag : used for dump specified register
* @irq: irq number used
* @irqTimeout: msecs only set if useIrqTimer is true
* @irq_disabled: whether irq should be ignored
* @irq_gpio: irq gpio number
* @useIrqTimer: older models need irq timer for pen up cases
* @read_reg: record reg address which want to read
*@cust_prox_ctrl0 : used for custom setting for channel and scan period
* @init: (re)initialize device
* @refreshStatus: read register status
* @get_nirq_low: get whether nirq is low (platform data)
* @statusFunc: array of functions to call for corresponding status bit
***************************************/
#define USE_THREADED_IRQ

#define MAX_NUM_STATUS_BITS (8)

enum channel_index
{
	CHANNEL_TOP = 0x00,
	CHANNEL_BOTTOM_LMB = 0x01,
//	CHANNEL_BOTTOM = 0x02,
	CHANNEL_BOTTOM_NB = 0x02,
};

enum channel_state
{
	IDLE = 0x0,
	S_PROX,
	S_BODY,
	DISABLE,
};

typedef struct abovXX abovXX_t, *pabovXX_t;
struct abovXX {
	struct device *pdev;
	struct delayed_work dworker;
	struct abov_platform_data *board;
#if defined(USE_THREADED_IRQ)
	struct mutex mutex;
#else
	spinlock_t	lock;
#endif
	void *bus;
	void *pDevice;
	int read_flag;
	int get_ch0_flag;
	int irq;
	int irqTimeout;
	char irq_disabled;
	/* whether irq should be ignored.. cases if enable/disable irq is not used
	 * or does not work properly */
	u8 useIrqTimer;
	u8 read_reg;
	u8 read_len;//add to read serial multi-reg
	int32_t report_data;
	bool loading_fw;
	struct work_struct ps_notify_work;
	struct notifier_block ps_notif;
	bool ps_is_present;
	struct work_struct fw_update_work;

	/* Function Pointers */
	int (*init)(pabovXX_t this);
	/* since we are trying to avoid knowing registers, create a pointer to a
	 * common read register which would be to read what the interrupt source
	 * is from
	 */
	int (*refreshStatus)(pabovXX_t this);
	int (*get_nirq_low)(unsigned irq_gpio);
	void (*statusFunc[MAX_NUM_STATUS_BITS])(pabovXX_t this);
};

void abovXX_suspend(pabovXX_t this);
void abovXX_resume(pabovXX_t this);
int abovXX_sar_init(pabovXX_t this);
int abovXX_sar_remove(pabovXX_t this);
static int abov_tk_fw_mode_enter(struct i2c_client *client);
//static int sar_misc_init(pabovXX_t this);
int abovXX_sar_data_report(pabovXX_t this,int32_t channel);
#endif
