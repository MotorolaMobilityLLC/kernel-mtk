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
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <hwmsensor.h>
#include <linux/poll.h>
#include "sensor_event.h"
#include "sensor_attr.h"
#include <linux/of_irq.h>

/*
 *  I2C Registers
 */
#define ABOV_IRQSTAT_REG_MACRO		0x00
#define ABOV_VERSION_REG		    0x01
#define ABOV_MODELNO_REG		    0x02
#define ABOV_VENDOR_ID_REG		    0x03
#define ABOV_IRQSTAT_REG_DETAIL		0x04
#define ABOV_SOFTRESET_REG  		0x06
#define ABOV_CTRL_MODE_REG			0x07
#define ABOV_CTRL_CHANNEL_REG		0x08
#define ABOV_CH0_DIFF_MSB_REG		0x1C
#define ABOV_CH0_DIFF_LSB_REG		0x1D
#define ABOV_CH1_DIFF_MSB_REG		0x1E
#define ABOV_CH1_DIFF_LSB_REG		0x1F
#define ABOV_CH0_CAP_MSB_REG		0x20
#define ABOV_CH0_CAP_LSB_REG		0x21
#define ABOV_CH1_CAP_MSB_REG		0x22
#define ABOV_CH1_CAP_LSB_REG		0x23
#define ABOV_REF_CAP_MSB_REG		0x24
#define ABOV_REF_CAP_LSB_REG		0x25
#define ABOV_CTRL_TH_LEVEL_REG		0x2B
#define ABOV_RECALI_REG				0xFB

/* define key value for report */
#define KEY_CAPSENSE_TOUCHCS0       253
#define KEY_CAPSENSE_TOUCHCS1       254
#define KEY_CAPSENSE_RELEASECS0	    198
#define KEY_CAPSENSE_RELEASECS1	    199

/* enable body stat */
#define ABOV_TCHCMPSTAT_TCHSTAT0_FLAG_MACRO   0x01
#define ABOV_TCHCMPSTAT_TCHSTAT1_FLAG_MACRO   0x02

#define ABOV_TCHCMPSTAT_TCHSTAT0_FLAG_DETAIL   0x0C
#define ABOV_TCHCMPSTAT_TCHSTAT1_FLAG_DETAIL   0x03


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
		.val = 0x07,
	},
	{
		.reg = ABOV_RECALI_REG,
		.val = 0x01,
	},
};


struct abov_platform_data {
	int i2c_reg_num;
	struct smtc_reg_data *pi2c_reg;
	unsigned irq_gpio;
	/* used for custom setting for channel and scan period */
	int cap_channel_top;
	int cap_channel_bottom;
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
* @init: (re)initialize device
* @refreshStatus: read register status
* @get_nirq_low: get whether nirq is low (platform data)
* @statusFunc: array of functions to call for corresponding status bit
***************************************/
#define USE_THREADED_IRQ

#define MAX_NUM_STATUS_BITS (2)

enum channel_index
{
	CHANNEL_STATE = 0x00,
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
	struct sensor_attr_t mdev;
	struct device *pdev;
	struct delayed_work dworker;
	struct abov_platform_data *board;
#if defined(USE_THREADED_IRQ)
	struct mutex mutex;
#else
	spinlock_t	lock;
#endif
	void *bus;
	// void *pDevice;
	int32_t report_data[3];
	int read_flag;
	int irq;
	int irqTimeout;
	char irq_disabled;
	/* whether irq should be ignored.. cases if enable/disable irq is not used
	 * or does not work properly */
	u8 useIrqTimer;
	u8 read_reg;

	struct work_struct ps_notify_work;
	struct notifier_block ps_notif;
	bool ps_is_present;
	bool loading_fw;

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
static int sar_misc_init(pabovXX_t this);
int abovXX_sar_data_report(pabovXX_t this);
#endif
