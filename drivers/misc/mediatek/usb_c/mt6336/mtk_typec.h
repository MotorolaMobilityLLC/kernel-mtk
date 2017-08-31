/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _TYPEC_H
#define _TYPEC_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/pm_runtime.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <mtk_ppm_api.h>

#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/usb/class-dual-role.h>
#endif /* CONFIG_DUAL_ROLE_USB_INTF */

#include <linux/cdev.h>

#include "pd.h"
#include "usb_pd.h"

#ifdef CONFIG_RT7207_ADAPTER
#include "mtk_direct_charge_vdm.h"
#endif

#define MAX_SIZE 80
#define TYPEC "typec"


/* configurations */
/*platform dependent*/
#define USE_AUXADC 1 /*set to 1 if AUXADC is used*/
#define COMPLIANCE 0 /*set to 1 use CC only. No related to USB function*/
#define SUPPORT_PD 1
#define VDM_MODE
#define AUTO_SEND_HR
#define SUPPORT_SOP_P
#define BURST_READ

/*enable SNK<->ACC, SRC<->ACC transitions*/
#define ENABLE_ACC 1

#define RESET_STRESS_TEST 0

/* macros */
#define ZERO_INDEXED_VAL(val) ((val) - 1)
#define DIV_AND_RND_UP(dividend, divider) (((dividend) + (divider) - 1) / (divider))
#define ZERO_INDEXED_DIV_AND_RND_UP(dividdend, divider) (ZERO_INDEXED_VAL(DIV_AND_RND_UP((dividdend), (divider))))

#define SRC_CUR(x) ({ char *tmp; \
	if (x == 0) \
		tmp = "IDLE"; \
	else if (x == 1) \
		tmp = "DEFAULT"; \
	else if (x == 2) \
		tmp = "1.5A"; \
	else if (x == 3) \
		tmp = "3.0A"; \
	else \
		tmp = "UNKNOWN"; \
	tmp; })

/* TYPEC configurations */

/*when set, signals vbus off when VSAFE_0V is 1*/
/*when not set, signals vbus off when VSAFE_5V is 0*/
/*on FPGA, VSAFE5V is 1 when voltage is greater than 4.8v; VSAFE0V is 1 when voltage is smaller than 0.8v*/
#define DETECT_VSAFE_0V 1

/*reference clock*/
#define REF_CLK_DIVIDEND 750 /*75k*/
#define REF_CLK_DIVIDER 10

/*DAC reference voltage*/
#define SRC_VRD_DEFAULT_DAC_VAL 2
#define SRC_VOPEN_DEFAULT_DAC_VAL 37
#define SRC_VRD_15_DAC_VAL 7
#define SRC_VOPEN_15_DAC_VAL 37
#define SRC_VRD_30_DAC_VAL 17
#define SRC_VOPEN_30_DAC_VAL 62

#define SNK_VRPUSB_DAC_VAL 2
#define SNK_VRP15_DAC_VAL 14
#define SNK_VRP30_DAC_VAL 29

#if USE_AUXADC

enum sink_power_states {
	STATE_POWER_DEFAULT = 0,
	STATE_POWER_1500MA = 1,
	STATE_POWER_3000MA = 2,
};

#define FLAGS_AUXADC_NONE (0)
#define FLAGS_AUXADC_MIN (0x1<<0)
#define FLAGS_AUXADC_MAX (0x1<<1)

/*AUXADC reference voltage*/
#define AUXADC_INTERVAL_MS 5
#define AUXADC_DEBOUNCE_TIME 15
#define AUXADC_DEBOUNCE_COUNT (AUXADC_DEBOUNCE_TIME/AUXADC_INTERVAL_MS-1)

#define AUXADC_VOLTAGE_MV 1800
#define AUXADC_VOLTAGE_SCALE 4096

#define SNK_VRPUSB_VRP15_THRESHOLD_MV 680
#define SNK_VRP15_VRP30_THRESHOLD_MV 1280

/*
 * CC voltage send to AUXADC will divided by 2
 * vRd-connect <- 200mV -> vRd-USB <- 680mV -> vRd-1.5A <- 1280mV -> vRd-3.0A
*/
/*(200/2) / (1800/4096) = 100 / 0.43945  = 228*/
#define SNK_VRPUSB_AUXADC_MIN_VAL (228)

/*(680/2) / (1800/4096) = 340 / 0.43945  = 774*/
#define SNK_VRPUSB_AUXADC_MAX_VAL (774)
#define SNK_VRP15_AUXADC_MIN_VAL SNK_VRPUSB_AUXADC_MAX_VAL

/*(1280/2) / (1800/4096) = 640 / 0.43945  = 1456*/
#define SNK_VRP15_AUXADC_MAX_VAL (1456)
#define SNK_VRP30_AUXADC_MIN_VAL SNK_VRP15_AUXADC_MAX_VAL
#define SNK_VRP30_AUXADC_MAX_VAL (AUXADC_VOLTAGE_SCALE - 1)

#ifdef MT6336_E1
	/*INT_STATUS5 4th*/
#define TYPE_C_L_MIN (5*8+3)
	/*INT_STATUS5 7th*/
#define TYPE_C_H_MAX (5*8+6)
#else
	/*INT_STATUS5 1th*/
#define TYPE_C_L_MIN (5*8+0)
	/*INT_STATUS5 4th*/
#define TYPE_C_H_MAX (5*8+3)
#endif
#endif

/*timing*/
#define CC_VOL_PERIODIC_MEAS_VAL 5 /*5ms*/
#define CC_VOL_PD_DEBOUNCE_CNT_VAL 15 /*10-20ms*/
#define CC_VOL_CC_DEBOUNCE_CNT_VAL 150 /*100-200ms*/
#define DRP_SRC_CNT_VAL 37 /*50-100ms*/
#define DRP_SNK_CNT_VAL 37 /*50-100ms*/
#define DRP_TRY_CNT_VAL 112 /*75-150ms*/
#define DRP_TRY_WAIT_CNT_VAL 600 /*400-800ms*/

#define PD_VSAFE5V_LOW 4000 /*4.75 ~ 5.5*/
#define PD_VSAFE5V_HIGH 5500

#define PD_VSAFE0V_LOW 0 /*0 ~ 0.8v*/
#define PD_VSAFE0V_HIGH 800 /*0 ~ 0.8v*/

/*polling thread timing*/
#define POLLING_PERIOD_MS 10000
#define POLLING_MAX_TIME(x) (POLLING_PERIOD_MS / x)

#define CC_REG_BASE 0x100
#define PD_REG_BASE 0x200

#define WQ_FLAGS_VBUSON_ATTACHWAIT_SNK (1 << 0) /* vbus_on_attach_wait_snk */
#define WQ_FLAGS_VBUSON_TRYWAIT_SNK (1 << 1) /* vbus_on_try_wait_snk */
#define WQ_FLAGS_VBUSOFF_ATTACHED_SNK (1 << 2) /* vbus_off_attached_snk */
#define WQ_FLAGS_VBUSOFF_THEN_VBUSON (1 << 3) /* vbus_off_then_drive_attached_src */
#define WQ_FLAGS_VSAFE0V (1 << 4) /* vsafe0v */

enum enum_typec_dbg_lvl {
	TYPEC_DBG_LVL_0 = 0, /*nothing*/
	TYPEC_DBG_LVL_1 = 1, /*important interrupt dump, critical events*/
	TYPEC_DBG_LVL_2 = 2, /*nice to have dump*/
	TYPEC_DBG_LVL_3 = 3, /*full interrupt dump, verbose information dump*/
};

/* definitions */
#define TYPE_C_INTR_EN_0_ATTACH_MSK (REG_TYPE_C_CC_ENT_ATTACH_SRC_INTR_EN | \
		REG_TYPE_C_CC_ENT_ATTACH_SNK_INTR_EN)

#define TYPE_C_INTR_UNATTACH_TOGGLE (REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN | \
		REG_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_EN | \
		REG_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_EN)

#define TYPE_C_INTR_EN_0_MSK (TYPE_C_INTR_EN_0_ATTACH_MSK | \
		REG_TYPE_C_CC_ENT_AUDIO_ACC_INTR_EN | \
		REG_TYPE_C_CC_ENT_DBG_ACC_INTR_EN | \
		REG_TYPE_C_CC_ENT_DISABLE_INTR_EN | \
		REG_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_EN | \
		REG_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_EN | \
		REG_TYPE_C_CC_ENT_TRY_SRC_INTR_EN | \
		REG_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_EN | \
		REG_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_EN | \
		TYPE_C_INTR_UNATTACH_TOGGLE)

#define TYPE_C_INTR_EN_2_MSK (REG_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_EN | \
		REG_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_EN | \
		REG_TYPE_C_CC_ENT_SNK_PWR_15_INTR_EN | \
		REG_TYPE_C_CC_ENT_SNK_PWR_30_INTR_EN | \
		REG_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_EN)

#define TYPE_C_INTR_DRP_TOGGLE (REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN | \
		REG_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_EN)

#define TYPE_C_INTR_ACC_TOGGLE (REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN | \
		REG_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_EN)

#define TYPE_C_INTR_SRC_ADVERTISE (REG_TYPE_C_CC_ENT_SNK_PWR_30_INTR_EN | \
		REG_TYPE_C_CC_ENT_SNK_PWR_15_INTR_EN | \
		REG_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_EN)

#define TYPEC_TERM_CC (REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC1 | \
		REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC2)

#define TYPEC_TERM_CC1 (REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC1_EN | \
		REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC1_EN | \
		REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC1_EN)

#define TYPEC_TERM_CC2 (REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC2_EN | \
		REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC2_EN | \
		REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC2_EN)

#define TYPEC_ACC_EN (REG_TYPE_C_ACC_EN | \
		REG_TYPE_C_AUDIO_ACC_EN | \
		REG_TYPE_C_DEBUG_ACC_EN)

#define TYPEC_TRY (REG_TYPE_C_ATTACH_SRC_2_TRY_WAIT_SNK_ST_EN | \
		REG_TYPE_C_TRY_SRC_ST_EN)

/* enumerations */

enum enum_typec_role {
	TYPEC_ROLE_SINK = 0,
	TYPEC_ROLE_SOURCE = 1,
	TYPEC_ROLE_DRP = 2,
	TYPEC_ROLE_RESERVED = 3,
	TYPEC_ROLE_SINK_W_ACTIVE_CABLE = 4,
	TYPEC_ROLE_ACCESSORY_AUDIO = 5,
	TYPEC_ROLE_ACCESSORY_DEBUG = 6,
	TYPEC_ROLE_OPEN = 7,
	TYPEC_ROLE_NUM
};

enum enum_typec_rp {
	TYPEC_RP_DFT = 0, /*36K ohm*/
	TYPEC_RP_15A = 1, /*12K ohm*/
	TYPEC_RP_30A = 2, /*4.7K ohm*/
	TYPEC_RP_RESERVED = 3,
	TYPEC_RP_NUM = TYPEC_RP_RESERVED
};

enum enum_typec_term {
	TYPEC_TERM_RP = 0,
	TYPEC_TERM_RD = 1,
	TYPEC_TERM_RA = 2,
	TYPEC_TERM_NA = 3
};

enum enum_try_mode {
	TYPEC_TRY_DISABLE = 0,
	TYPEC_TRY_ENABLE = 1,
};

enum enum_typec_state {
	TYPEC_STATE_DISABLED = 0,
	TYPEC_STATE_UNATTACHED_SRC = 1,
	TYPEC_STATE_ATTACH_WAIT_SRC = 2,
	TYPEC_STATE_ATTACHED_SRC = 3,
	TYPEC_STATE_UNATTACHED_SNK = 4,
	TYPEC_STATE_ATTACH_WAIT_SNK = 5,
	TYPEC_STATE_ATTACHED_SNK = 6,
	TYPEC_STATE_TRY_SRC = 7,
	TYPEC_STATE_TRY_WAIT_SNK = 8,
	TYPEC_STATE_UNATTACHED_ACCESSORY = 9,
	TYPEC_STATE_ATTACH_WAIT_ACCESSORY = 10,
	TYPEC_STATE_AUDIO_ACCESSORY = 11,
	TYPEC_STATE_DEBUG_ACCESSORY = 12,
	TYPEC_STATE_NUM
};

enum enum_vbus_lvl {
	TYPEC_VSAFE_5V = 0,
	TYPEC_VSAFE_0V = 1,
};

/* structures */

struct cable_info {
	uint32_t id_header;
	uint32_t cer_stat_vdo;
	uint32_t product_vdo;
	uint32_t cable_vdo;
};

/**
 * struct typec_hba - per adapter private structure
 * @mmio_base: base register address
 * @dev: device handle
 * @irq: Irq number of the controller
 * @ee_ctrl_mask: Exception event control mask
 * @feh_workq: Work queue for fatal controller error handling
 * @eeh_work: Worker to handle exception events
 * @id: device id
 * @support_role: controller role
 * @power_role: power role
 */
struct typec_hba {
	void __iomem *mmio_base;
	struct device *dev;
	unsigned int mode; /*0:Disable Type-C & PD, 1:Support Type-C, 2: Support PD*/
	unsigned int irq;
	unsigned int cc_irq;
	unsigned int pd_irq;
	int id;
	bool is_kpoc;

#if !COMPLIANCE
	atomic_t lowq_cnt;
	struct mt6336_ctrl *core_ctrl;
	struct mutex lowq_lock;
#endif

	struct mutex ioctl_lock;
	struct mutex typec_lock;

	struct wake_lock typec_wakelock;

	struct workqueue_struct *pd_wq;
	struct work_struct wait_vbus_on_attach_wait_snk;
	struct work_struct wait_vbus_on_try_wait_snk;
	struct work_struct wait_vbus_off_attached_snk;
	struct work_struct wait_vbus_off_then_drive_attached_src;
	struct work_struct wait_vsafe0v;
	struct work_struct init_vbus_off;
	unsigned int wq_running;
	unsigned int wq_cnt;
#if USE_AUXADC
	struct work_struct auxadc_voltage_mon_attached_snk;
	struct completion auxadc_event;
	int auxadc_flags;
#endif
	struct work_struct irq_work;
	struct delayed_work usb_work;

	unsigned int prefer_role; /* 0: SNK Only, 1: SRC Only, 2: DRP, 3: Try.SRC, 4: Try.SNK */
	enum enum_typec_role support_role; /*0: SNK, 1: SRC, 2: DRP*/
	enum enum_typec_rp rp_val; /* 0: Default, 1: 1.5, 2: 3.0 */
	enum enum_typec_rp pd_rp_val;
	enum enum_typec_dbg_lvl dbg_lvl;

	uint8_t cc; /*0:no cc, 1;cc1 2;cc2*/
	uint8_t src_rp;
	bool ra;
	uint8_t vbus_en;
	uint8_t vbus_det_en;
	uint8_t vbus_present;
	uint8_t vconn_en;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	struct dual_role_phy_instance *dr_usb;
	uint8_t dual_role_supported_modes;
	uint8_t dual_role_mode;
	uint8_t dual_role_pr;
	uint8_t dual_role_dr;
	uint8_t dual_role_vconn;
#endif /* CONFIG_DUAL_ROLE_USB_INTF */


#if SUPPORT_PD
	uint16_t cc_is0;
	uint16_t pd_is0;
	uint16_t pd_is1;

	uint8_t pd_comm_enabled;
	uint32_t bist_mode;

	/* current port power role (SOURCE or SINK) */
	uint8_t power_role;
	/* current port data role (DFP or UFP) */
	uint8_t data_role;
	/* port flags, see PD_FLAGS_* */
	uint32_t flags;

	/* PD state for port */
	enum pd_states task_state;
	/* PD state when we run state handler the last time */
	enum pd_states last_state;
	/* The state to go to after timeout */
	enum pd_states timeout_state;
	/* Timeout for the current state. Set to 0 for no timeout. */

	unsigned long timeout_ms;
	struct hrtimer timeout_timer;
	unsigned long timeout_user;
	/* Time for source recovery after hard reset */
	unsigned long src_recover;

	wait_queue_head_t wq;
	bool tx_event;
	bool rx_event;
#if RESET_STRESS_TEST
	struct completion ready;
#endif
	/* status of last transmit */
	/*uint8_t tx_status;*/
	/* last requested voltage PDO index */
	int requested_idx;
#ifdef CONFIG_USB_PD_DUAL_ROLE
	/* Current limit / voltage based on the last request message */
	uint32_t curr_limit;
	uint32_t supply_voltage;
	/* Signal charging update that affects the port */
	int new_power_request;
	/* Store previously requested voltage request */
	int prev_request_mv;
#endif

	unsigned int discover_vmd;
	/* PD state for Vendor Defined Messages */
	enum vdm_states vdm_state;
	/* Timeout for the current vdm state.  Set to 0 for no timeout. */
	unsigned long vdm_timeout;
	/* next Vendor Defined Message to send */
	uint32_t vdo_data[VDO_MAX_SIZE];
	uint8_t vdo_count;
	/* VDO to retry if UFP responder replied busy. */
	uint32_t vdo_retry;
	uint16_t alt_mode_svid;

	unsigned int vbus_on_polling;
	unsigned int vbus_off_polling;
	unsigned int kpoc_delay;
	int hr_auto_sent;
	uint32_t cable_flags;
	struct cable_info sop_p;

#ifdef CONFIG_RT7207_ADAPTER
	struct pd_direct_chrg *dc;
#endif

#if PD_SW_WORKAROUND1_1
	uint16_t header;
	uint32_t payload[7];
#endif
#endif

	int vsafe_5v;
	int (*charger_det_notify)(int);
};

struct bit_mapping {
	uint16_t mask;
	char name[MAX_SIZE];
};

struct reg_mapping {
	uint16_t addr;
	uint16_t mask;
	uint16_t ofst;
	char name[MAX_SIZE];
};

/* SW probe */
#define DBG_TIMER0_VAL_OFST 20
#define DBG_VBUS_DET_EN_OFST 19
#define DBG_VCONN_EN_OFST 18
#define DBG_VBUS_EN_OFST 17
#define DBG_VBUS_PRESENT_OFST 16
#define DBG_INTR_TIMER_EVENT_OFST 15
#define DBG_INTR_CC_EVENT_OFST 14
#define DBG_INTR_RX_EVENT_OFST 13
#define DBG_INTR_TX_EVENT_OFST 12
#define DBG_LOOP_STATE_OFST 8
#define DBG_INTR_STATE_OFST 6
#define DBG_PD_STATE_OFST 0

#define DBG_TIMER0_VAL (0xfff<<DBG_TIMER0_VAL_OFST)
#define DBG_VBUS_DET_EN (0x1<<DBG_VBUS_DET_EN_OFST)
#define DBG_VCONN_EN (0x1<<DBG_VCONN_EN_OFST)
#define DBG_VBUS_EN (0x1<<DBG_VBUS_EN_OFST)
#define DBG_VBUS_PRESENT (0x1<<DBG_VBUS_PRESENT_OFST)
#define DBG_INTR_TIMER_EVENT (0x1<<DBG_INTR_TIMER_EVENT_OFST)
#define DBG_INTR_CC_EVENT (0x1<<DBG_INTR_CC_EVENT_OFST)
#define DBG_INTR_RX_EVENT (0x1<<DBG_INTR_RX_EVENT_OFST)
#define DBG_INTR_TX_EVENT (0x1<<DBG_INTR_TX_EVENT_OFST)
#define DBG_LOOP_STATE (0xf<<DBG_LOOP_STATE_OFST)
#define DBG_INTR_STATE (0x3<<DBG_INTR_STATE_OFST)
#define DBG_PD_STATE (0x3f<<DBG_PD_STATE_OFST)

enum enum_intr_state {
	DBG_INTR_NONE = 0,
	DBG_INTR_CC = 1,
	DBG_INTR_PD = 2,
};

enum enum_loop_state {
	DBG_LOOP_NONE = 0,
	DBG_LOOP_WAIT = 1,
	DBG_LOOP_CHK_EVENT = 2,
	DBG_LOOP_TIMER = 3,
	DBG_LOOP_RX = 4,
	DBG_LOOP_HARD_RESET = 5,
	DBG_LOOP_CHK_CC_EVENT = 6,
	DBG_LOOP_STATE_MACHINE = 7,
	DBG_LOOP_CHK_TIMEOUT = 8,
};

/* read/write function calls */
extern void typec_write8(struct typec_hba *hba, uint8_t val, unsigned int reg);
extern void typec_writew(struct typec_hba *hba, uint16_t val, unsigned int reg);
extern void typec_writedw(struct typec_hba *hba, uint32_t val, unsigned int reg);
extern uint8_t typec_read8(struct typec_hba *hba, unsigned int reg);
extern uint16_t typec_readw(struct typec_hba *hba, unsigned int reg);
#ifdef BURST_READ
extern uint32_t typec_readdw(struct typec_hba *hba, unsigned int reg);
#endif
extern void typec_write8_msk(struct typec_hba *hba, uint8_t msk, uint8_t val, unsigned int reg);
extern void typec_writew_msk(struct typec_hba *hba, uint16_t msk, uint16_t val, unsigned int reg);
extern void typec_set8(struct typec_hba *hba, uint8_t val, unsigned int reg);
extern void typec_set(struct typec_hba *hba, uint16_t val, unsigned int reg);
extern void typec_clear(struct typec_hba *hba, uint16_t val, unsigned int reg);

/* TYPEC function calls */

int typec_runtime_suspend(struct typec_hba *hba);
int typec_runtime_resume(struct typec_hba *hba);
int typec_runtime_idle(struct typec_hba *hba);

int typec_init(struct device *, struct typec_hba **, void __iomem *, unsigned int irq, int id);
void typec_remove(struct typec_hba *);

int typec_pltfrm_init(void);

#if SUPPORT_PD
/* PD function calls */
extern void pd_basic_settings(struct typec_hba *hba);
extern void pd_init(struct typec_hba *hba);
extern void pd_intr(struct typec_hba *hba, uint16_t pd_is0, uint16_t pd_is1, uint16_t cc_is0, uint16_t cc_is2);

extern void pd_rx_enable(struct typec_hba *hba, uint8_t enable);
extern void pd_ping_enable(struct typec_hba *hba, int enable);
extern void set_state(struct typec_hba *hba, enum pd_states next_state);
extern void pd_request_power_swap(struct typec_hba *hba);
extern void pd_request_vconn_swap(struct typec_hba *hba);
extern void pd_request_data_swap(struct typec_hba *hba);
extern int send_control(struct typec_hba *hba, enum pd_ctrl_msg_type type);

extern int typec_vbus(struct typec_hba *hba);
extern void typec_vbus_present(struct typec_hba *hba, uint8_t enable);
extern void typec_vbus_det_enable(struct typec_hba *hba, uint8_t enable);
extern unsigned int vbus_val(struct typec_hba *hba);
extern unsigned int vbus_val_self(struct typec_hba *hba);
extern void typec_drive_vbus(struct typec_hba *hba, uint8_t on);
extern void typec_drive_vconn(struct typec_hba *hba, uint8_t enable);
extern void typec_int_enable(struct typec_hba *hba, uint16_t msk0, uint16_t msk2);
extern void typec_int_disable(struct typec_hba *hba, uint16_t msk0, uint16_t msk2);
extern void typec_select_rp(struct typec_hba *hba, enum enum_typec_rp rp_val);
extern int typec_enable(struct typec_hba *hba, int enable);
extern int typec_set_mode(struct typec_hba *hba, enum enum_typec_role role, int param1, int param2);
extern void typec_set_vsafe5v(struct typec_hba *hba, int val);
extern void typec_enable_lowq(struct typec_hba *hba, char *str);
extern void typec_disable_lowq(struct typec_hba *hba, char *str);

extern struct typec_hba *get_hba(void);

extern int pd_task(void *data);
extern int pd_kpoc_task(void *data);
extern void pd_set_data_role(struct typec_hba *hba, int role);
extern void pd_get_message(struct typec_hba *hba, uint16_t *header, uint32_t *payload);
extern int pd_is_power_swapping(struct typec_hba *hba);

extern void pmic_enable_chrdet(unsigned char en);

#ifdef CONFIG_DUAL_ROLE_USB_INTF
extern int mt_dual_role_phy_init(struct typec_hba *hba);
#endif

#endif

#endif /* End of Header */
