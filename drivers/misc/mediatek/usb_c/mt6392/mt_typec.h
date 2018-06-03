/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
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
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/random.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>

#include <asm/irq.h>
#include <asm/byteorder.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_eh.h>

#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/mfd/mt6392/registers.h>
#include <linux/irqdomain.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "typec_reg.h"

#define MAX_SIZE 80
#define TYPEC "typec"

/* platform dependent */
#define FPGA_PLATFORM 0		/* set to 1 if V7 FPGA is used */
#define USE_AUXADC 0		/* set to 1 if AUXADC is used */

/* enable SNK<->ACC, SRC<->ACC transitions */
#define ENABLE_ACC 1

/* set to 1 if SW debug probe is used */
#define DBG_PROBE 0
#define DBG_STATIC 0

/* macros */
#define ZERO_INDEXED_VAL(val) ((val) - 1)
#define DIV_AND_RND_UP(dividend, divider) (((dividend) + (divider) - 1) / (divider))
#define ZERO_INDEXED_DIV_AND_RND_UP(dividdend, divider) (ZERO_INDEXED_VAL(DIV_AND_RND_UP((dividdend), (divider))))

/* TYPEC configurations
 * when set, signals vbus off when VSAFE_0V is 1
 * when not set, signals vbus off when VSAFE_5V is 0
 * on FPGA, VSAFE5V is 1 when voltage is greater than 4.8v
 * VSAFE0V is 1 when voltage is smaller than 0.8v
*/
#define DETECT_VSAFE_0V 1

/* reference clock */
#if FPGA_PLATFORM
#define REF_CLK_DIVIDEND 234	/* 23.4k */
#define REF_CLK_DIVIDER 10
#else
#define REF_CLK_DIVIDEND 750	/* 75k */
#define REF_CLK_DIVIDER 10
#endif

/* DAC reference voltage */
#if FPGA_PLATFORM
#define SRC_VRD_DEFAULT_DAC_VAL 3
#define SRC_VOPEN_DEFAULT_DAC_VAL 30
#define SRC_VRD_15_DAC_VAL 5
#define SRC_VOPEN_15_DAC_VAL 30
#define SRC_VRD_30_DAC_VAL 15
#define SRC_VOPEN_30_DAC_VAL 50

#define SNK_VRPUSB_DAC_VAL 3
#define SNK_VRP15_DAC_VAL 13
#define SNK_VRP30_DAC_VAL 24
#else
#define SRC_VRD_DEFAULT_DAC_VAL 2
#define SRC_VOPEN_DEFAULT_DAC_VAL 37
#define SRC_VRD_15_DAC_VAL 7
#define SRC_VOPEN_15_DAC_VAL 37
#define SRC_VRD_30_DAC_VAL 17
#define SRC_VOPEN_30_DAC_VAL 62

#define SNK_VRPUSB_DAC_VAL 2
#define SNK_VRP15_DAC_VAL 14
#define SNK_VRP30_DAC_VAL 29
#endif

#if USE_AUXADC
/* AUXADC reference voltage */
#define AUXADC_INTERVAL_MS 10
#define AUXADC_DEBOUNCE_TIME 2

#define AUXADC_EVENT_INTERVAL_MS 10

#define AUXADC_VOLTAGE_MV 1800
#define AUXADC_VOLTAGE_SCALE 4096

#define SNK_VRPUSB_VTH_MV 340
#define SNK_VRP15_VTH_MV 640
#define SNK_VRPUSB_AUXADC_MIN_VAL 0
#define SNK_VRPUSB_AUXADC_MAX_VAL DIV_AND_RND_UP((SNK_VRPUSB_VTH_MV * AUXADC_VOLTAGE_MV), AUXADC_VOLTAGE_SCALE)
#define SNK_VRP15_AUXADC_MIN_VAL SNK_VRPUSB_AUXADC_MAX_VAL
#define SNK_VRP15_AUXADC_MAX_VAL DIV_AND_RND_UP((SNK_VRP15_VTH_MV * AUXADC_VOLTAGE_MV), AUXADC_VOLTAGE_SCALE)
#define SNK_VRP30_AUXADC_MIN_VAL SNK_VRP15_AUXADC_MAX_VAL
#define SNK_VRP30_AUXADC_MAX_VAL (AUXADC_VOLTAGE_SCALE - 1)
#endif

/* timing */
#define CC_VOL_PERIODIC_MEAS_VAL 5	/* 5ms */
#define CC_VOL_PD_DEBOUNCE_CNT_VAL 15	/* 10-20ms */
#define CC_VOL_CC_DEBOUNCE_CNT_VAL 150	/* 100-200ms */
#define DRP_SRC_CNT_VAL 37	/* 50-100ms */
#define DRP_SNK_CNT_VAL 37	/* 50-100ms */
#define DRP_TRY_CNT_VAL 112	/* 75-150ms */
#define DRP_TRY_WAIT_CNT_VAL 600	/* 400-800ms */

/* polling thread timing */
#define POLLING_INTERVAL_MS 5
#define POLLING_PERIOD_MS 2000
#define POLLING_MAX_TIME (POLLING_PERIOD_MS / POLLING_INTERVAL_MS)

/* debug related */
enum enum_typec_dbg_lvl {
	TYPEC_DBG_LVL_0 = 0,	/* nothing */
	TYPEC_DBG_LVL_1 = 1,	/* important interrupt dump, critical events */
	TYPEC_DBG_LVL_2 = 2,	/* nice to have dump */
	TYPEC_DBG_LVL_3 = 3,	/* full interrupt dump, verbose information dump*/
};

/* definitions */
#define TYPEC_ATTACH_INTR (REG_TYPE_C_CC_ENT_ATTACH_SRC_INTR_EN | REG_TYPE_C_CC_ENT_ATTACH_SNK_INTR_EN)
#define TYPEC_UNATTACH_TOGGLE_INTR (REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN | REG_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_EN |\
				REG_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_EN)
#define TYPEC_TRY_SRC_INTR (REG_TYPE_C_CC_ENT_TRY_SRC_INTR_EN | REG_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_EN)
#if TYPEC_12
#define TYPEC_TRY_SNK_INTR (REG_TYPE_C_CC_ENT_TRY_SNK_INTR_EN | REG_TYPE_C_CC_ENT_TRY_WAIT_SRC_INTR_EN)
#define TYPEC_TRY_INTR (TYPEC_TRY_SRC_INTR | TYPEC_TRY_SNK_INTR)
#else
#define TYPEC_TRY_SNK_INTR (0)
#define TYPEC_TRY_INTR (TYPEC_TRY_SRC_INTR)
#endif
#define TYPEC_INTR_EN_0_MSK (TYPEC_ATTACH_INTR |\
	REG_TYPE_C_CC_ENT_AUDIO_ACC_INTR_EN | REG_TYPE_C_CC_ENT_DBG_ACC_INTR_EN | REG_TYPE_C_CC_ENT_DISABLE_INTR_EN |\
	REG_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_EN |\
	REG_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_EN | TYPEC_TRY_INTR |\
	REG_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_EN | TYPEC_UNATTACH_TOGGLE_INTR)
#define TYPEC_INTR_EN_2_MSK (REG_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_EN | REG_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_EN |\
	REG_TYPE_C_CC_ENT_SNK_PWR_15_INTR_EN | REG_TYPE_C_CC_ENT_SNK_PWR_30_INTR_EN |\
				REG_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_EN)

#define TYPEC_ACC_EN (REG_TYPE_C_ACC_EN | REG_TYPE_C_AUDIO_ACC_EN | REG_TYPE_C_DEBUG_ACC_EN)
#define TYPEC_TRY_SRC_EN (REG_TYPE_C_ATTACH_SRC_2_TRY_WAIT_SNK_ST_EN | REG_TYPE_C_TRY_SRC_ST_EN)
#if TYPEC_12
#define TYPEC_TRY_SNK_EN (REG_TYPE_C_TRY_SNK_ST_EN)
#define TYPEC_TRY_EN (TYPEC_TRY_SRC_EN | TYPEC_TRY_SNK_EN)
#else
#define TYPEC_TRY_SNK_EN (0)
#define TYPEC_TRY_EN (TYPEC_TRY_SRC_EN)
#endif
/* terminiation */
#define TYPEC_TERM_CC (REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC1 | REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC2)
#define TYPEC_TERM_CC1 (REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC1_EN | REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC1_EN |\
				REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC1_EN)
#define TYPEC_TERM_CC2 (REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC2_EN | REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC2_EN |\
				REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC2_EN)

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
	TYPEC_ROLE_ACCESSORY_DEBUG_SNK = 8
};

enum enum_typec_rp {
	TYPEC_RP_DFT = 0,	/* 36K ohm */
	TYPEC_RP_15A = 1,	/* 12K ohm */
	TYPEC_RP_30A = 2,	/* 4.7K ohm */
	TYPEC_RP_RESERVED = 3
};

enum enum_typec_term {
	TYPEC_TERM_RP = 0,
	TYPEC_TERM_RD = 1,
	TYPEC_TERM_RA = 2,
	TYPEC_TERM_NA = 3
};

/* PD 1.2 ECN */
/* 1. No try SRC or try SNK, if PD is supported */
/* 2. Either try SRC or try SNK can be enabled, if PD is not supported */
enum enum_try_mode {
	TYPEC_TRY_DISABLE = 0,
	TYPEC_TRY_SRC = 1,
	TYPEC_TRY_SNK = 2,
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
};

enum enum_vbus_lvl {
	TYPEC_VSAFE_5V = 0,
	TYPEC_VSAFE_0V = 1,
};

/**
 * struct typec_hba - per adapter private structure
 * @regmap: PMIC base register address
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
	struct regmap *regmap;
	struct device *dev;
	struct cdev cdev;
	unsigned int irq;
	u32 addr_base;
	int id;

	struct mutex ioctl_lock;
	spinlock_t typec_lock;

	struct work_struct wait_vbus_on_attach_wait_snk;
	struct work_struct wait_vbus_on_try_wait_snk;
	struct work_struct wait_vbus_off_attached_snk;
	struct work_struct wait_vbus_off_then_drive_attached_src;
	struct work_struct drive_vbus_work;
	struct work_struct platform_handler_work;
	struct pinctrl *pinctrl;
	struct pinctrl_state *typec_drvvbus;
	struct pinctrl_state *typec_drvvbus_low;
	struct pinctrl_state *typec_drvvbus_high;
#if USE_AUXADC
	struct work_struct auxadc_voltage_mon_attached_snk;
	uint8_t auxadc_event;
#endif

	enum enum_typec_role support_role;
	enum enum_typec_rp rp_val;
	enum enum_typec_dbg_lvl dbg_lvl;

	uint8_t vbus_en;
	uint8_t vbus_det_en;
	uint8_t vbus_present;
	uint8_t vconn_en;
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

/* read/write function calls */
static inline uint16_t typec_readw(struct typec_hba *hba, unsigned int reg)
{
	uint16_t ui2_data;
	unsigned int rdata = 0;

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
		dev_info(hba->dev, "typec_readw, reg:0x%x, hba->regmap:0x%p\n", reg, hba->regmap);

	regmap_read(hba->regmap, (hba->addr_base + reg), &rdata);
	ui2_data = (uint16_t) rdata;
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
		dev_info(hba->dev, "typec_readw,rdata:0x%x ui_data:0x%x\n", rdata, ui2_data);

	return ui2_data;
}

static inline void typec_writew(struct typec_hba *hba, uint16_t val, unsigned int reg)
{
	unsigned int rdata = 0;

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3) {
		dev_info(hba->dev, "typec_writew, reg:0x%x rdata:0x%x, val:0x%x\n", reg, rdata,
			 val);
	}
	regmap_write(hba->regmap, (hba->addr_base + reg), val);
	regmap_read(hba->regmap, (hba->addr_base + reg), &rdata);
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
		dev_info(hba->dev, "typec_writew, value:0x%x\n", rdata);
}
static inline void typec_writew_msk(struct typec_hba *hba, uint16_t msk, uint16_t val, unsigned int reg)
{
	regmap_update_bits(hba->regmap, (hba->addr_base + reg), msk, val);
}

static inline void typec_set(struct typec_hba *hba, uint16_t val, unsigned int reg)
{
	uint16_t msk = 0xffff;

	regmap_update_bits(hba->regmap, (hba->addr_base + reg), msk, val);
}

static inline void typec_clear(struct typec_hba *hba, uint16_t val, unsigned int reg)
{
	uint16_t msk = 0x0000;

	val = ~val;
	regmap_update_bits(hba->regmap, (hba->addr_base + reg), msk, val);
}

/* SW probe */
#define DBG_TIMER0_VAL_OFST 24
#define DBG_CC_ST_OFST 20
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

#define DBG_TIMER0_VAL (0xff<<DBG_TIMER0_VAL_OFST)
#define DBG_CC_ST (0xf<<DBG_CC_ST_OFST)
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

#if DBG_PROBE
static inline void typec_sw_probe(struct typec_hba *hba, uint32_t msk, uint32_t val)
{
	typec_writew_msk(hba, msk, val, TYPE_C_SW_DEBUG_PORT_0);
	typec_writew_msk(hba, (msk >> 16), (val >> 16), TYPE_C_SW_DEBUG_PORT_1);
}
#else
static inline void typec_sw_probe(struct typec_hba *hba, uint32_t msk, uint32_t val)
{
}
#endif

/* TYPEC function calls */
int typec_runtime_suspend(struct typec_hba *hba);
int typec_runtime_resume(struct typec_hba *hba);
int typec_runtime_idle(struct typec_hba *hba);

int typec_init(struct device *, struct typec_hba *, unsigned int, int);
void typec_remove(struct typec_hba *);

int typec_pltfrm_init(void);
void typec_pltfrm_exit(void);

/* PMIC get vbus value */
extern int PMIC_IMM_GetOneChannelValue(unsigned int dwChannel, int deCount, int trimd);

#endif				/* End of neader */
