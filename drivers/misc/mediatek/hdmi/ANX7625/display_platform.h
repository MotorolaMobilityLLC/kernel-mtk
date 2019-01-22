/******************************************************************************
*
Copyright (c) 2016, Analogix Semiconductor, Inc.

PKG Ver  : V1.0

Filename :

Project  : ANX7625

Created  : 02 Oct. 2016

Devices  : ANX7625

Toolchain: Android

Description:

Revision History:

******************************************************************************/

#include <linux/init.h>

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>

#include <linux/semaphore.h>
#include <linux/mutex.h>

#include "anx7625_driver.h"


#ifndef SLIMPORT_FEATURE
#define SLIMPORT_FEATURE
#endif




/****************************************************************/
#define TAG	"[EXTD/HDMI/ANX7625]"

/*
*#define SLIMPORT_TX_EDID_READ(context,fmt,arg...) \
	pr_debug(fmt, ## arg);
*/
#define SLIMPORT_TX_EDID_READ(context, fmt, arg...)

#define SLIMPORT_TX_EDID_INFO(context, fmt, arg...)	\
	pr_info(TAG fmt, ## arg)

#define SLIMPORT_TX_DBG_INFO(driver_context, fmt, arg...)	\
	pr_info(TAG fmt, ## arg)
/*
*
#define SLIMPORT_TX_DBG_WARN(driver_context, fmt, arg...)	\
	pr_info(fmt, ## arg)
*/
#define SLIMPORT_TX_DBG_ERR(driver_context, fmt, arg...)	\
	pr_info(TAG fmt, ## arg)

enum PIN_TYPE {
	RESET_PIN,
	PD_PIN,
	MAX_TYPE_PIN
};
extern unsigned int mhl_eint_number;

#ifdef ANX7625_MTK_PLATFORM
extern unsigned int dst_is_dsi;
extern uint8_t  Cap_MAX_channel;
extern uint16_t Cap_SampleRate;
extern uint8_t  Cap_Samplebit;

extern irqreturn_t anx7625_cbl_det_isr(int irq, void *data);
extern irqreturn_t anx7625_intr_comm_isr(int irq, void *data);

extern struct device *ext_dev_context;

#endif

extern wait_queue_head_t mhl_irq_wq;
extern atomic_t mhl_irq_event;

int HalOpenI2cDevice(char const *DeviceName, char const *DriverName);
void slimport_platform_init(void);
void register_slimport_eint(void);
void register_slimport_comm_eint(void);

void mhl_power_ctrl(int enable);
void reset_mhl_board(int hwResetPeriod, int hwResetDelay, int is_power_on);
void i2s_gpio_ctrl(int enable);
void dpi_gpio_ctrl(int enable);
void set_pin_high_low(enum PIN_TYPE pin, bool is_high);

void Unmask_Slimport_Intr(void);
void Mask_Slimport_Intr(bool irq_context);
void Unmask_Slimport_Comm_Intr(void);
int confirm_mhl_irq_level(void);
int confirm_mhl_comm_irq_level(void);

void cust_power_on(int enable);
void cust_reset(int hwResetPeriod, int hwResetDelay, int is_power_on);

#ifdef CONFIG_MACH_MT6799
extern struct mt6336_ctrl *mt6336_ctrl;
#endif

