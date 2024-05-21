// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015 MediaTek Inc.
 */


#ifndef __SWTP_H__
#define __SWTP_H__

/* modify MAX_PIN_NUM/DTS to support more gpio,
 * need to follow SOP for customization.
 */
#define MAX_PIN_NUM 2     //HUAQIN,<ZHANGZHAO>,<2022-9-20>,<EKPENANG4G-3078>,[BUG],/Modify the SWTP function power boost method.
#define SWTP_COMPATIBLE_DEVICE_ID "mediatek, swtp-eint"
#define SWTP1_COMPATIBLE_DEVICE_ID "mediatek, swtp1-eint"
#define SWTP2_COMPATIBLE_DEVICE_ID "mediatek, swtp2-eint"
#define SWTP3_COMPATIBLE_DEVICE_ID "mediatek, swtp3-eint"
#define SWTP4_COMPATIBLE_DEVICE_ID "mediatek, swtp4-eint"

#define SWTP_EINT_PIN_PLUG_IN        (1)
#define SWTP_EINT_PIN_PLUG_OUT       (0)

#define SWTP_DO_TX_POWER	(0)
#define SWTP_NO_TX_POWER	(1)

//BEGIN HUAQIN,<ZHANGZHAO>,<2022-9-22>,<EKPENANG4G-3199>,[BUG],/The SWTP function is divided into NCA configuration and CA configuration.
//BEGIN HUAQIN,<WANGYIFAN>,<2022-11-16>,<EKPENANG4G-7381>,[BUG],/The SWTP function adds a Board ID(16、17、18).
typedef enum {
        BoardID_0 = 0,
        BoardID_1,
        BoardID_2,
        BoardID_3,
        BoardID_4,
        BoardID_5,
        BoardID_6,
        BoardID_7,
        BoardID_8,
        BoardID_9,
        BoardID_10,
        BoardID_11,
        BoardID_12,
        BoardID_13,
        BoardID_14,
        BoardID_15,
        BoardID_16,
        BoardID_17,
        BoardID_18
} BoardID;
//END <EKPENANG4G-7381>
//END <EKPENANG4G-3199>

struct swtp_t {
	unsigned int	md_id;
	unsigned int	curr_mode;
	unsigned int	irq[MAX_PIN_NUM];
	unsigned int	gpiopin[MAX_PIN_NUM];
	unsigned int	setdebounce[MAX_PIN_NUM];
	unsigned int	eint_type[MAX_PIN_NUM];
	unsigned int	gpio_state[MAX_PIN_NUM];
	int	tx_power_mode;
	spinlock_t		spinlock;
	struct delayed_work delayed_work;
	struct delayed_work init_delayed_work;
};
/*****************************************************************************/
/* External API Region called by ccci_swtp object */
/*****************************************************************************/
extern int swtp_init(int md_id);
extern void inject_pin_status_event(int pin_value, const char pin_name[]);
#endif				/* __SWTP_H__ */
