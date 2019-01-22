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

#ifndef _TYPEC_PD_H
#define _TYPEC_PD_H

#define SUPPORT_PD 1

#if SUPPORT_PD

/*W1C RCV_MSG_INTR clears RX buffer, choose method 1 or 2*/
/*method 1: read RX buffer and W1C RCV_MSG_INTR in ISR*/
/*mtthod 2: leave RCV_MSG_INTR untouched, read RX buffer and W1C it in main loop*/
#define PD_SW_WORKAROUND1_1 1
#define PD_SW_WORKAROUND1_2 0

#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USBC_VCONN
#define CONFIG_USBC_VCONN_SWAP
#define CONFIG_USB_PD_ALT_MODE
#define CONFIG_USB_PD_ALT_MODE_DFP

/*#define PROFILING*/

#include "pd_reg.h"

/*interrupt classification - by type*/

/*TX/TX(Auto SR)/RX interrupts -- SUCCESS*/
#define PD_TX_MSG_SUCCESS (PD_TX_DONE_INTR)
#define PD_TX_AUTO_SR_SUCCESS (PD_TX_AUTO_SR_DONE_INTR)
#define PD_TX_HR_SUCCESS (PD_HR_TRANS_DONE_INTR)
#define PD_RX_MSG_SUCCESS (PD_RX_RCV_MSG_INTR)
#define PD_RX_HR_SUCCESS (PD_HR_RCV_DONE_INTR)

/*TX/TX(Auto SR)/RX interrupts -- FAIL*/
#define PD_TX_MSG_FAIL (PD_TX_RETRY_ERR_INTR | \
	PD_TX_RCV_NEW_MSG_DISCARD_MSG_INTR | PD_TX_PHY_LAYER_RST_DISCARD_MSG_INTR)
#define PD_TX_AUTO_SR_FAIL (PD_TX_AUTO_SR_RETRY_ERR_INTR | \
	PD_TX_AUTO_SR_RCV_NEW_MSG_DISCARD_MSG_INTR | PD_TX_AUTO_SR_PHY_LAYER_RST_DISCARD_MSG_INTR)
#define PD_TX_HR_FAIL (PD_HR_TRANS_CPL_TIMEOUT_INTR | PD_HR_TRANS_FAIL_INTR)

/*TX/TX(Auto SR)/RX interrupts -- INFO ONLY*/
#define PD_TX_MSG_INFO (PD_TX_DIS_BUS_REIDLE_INTR | PD_TX_CRC_RCV_TIMEOUT_INTR)
#define PD_TX_AUTO_SR_FAIL_INFO (PD_TX_MSG_INFO)
#define PD_RX_MSG_FAIL_INFO (PD_RX_LENGTH_MIS_INTR | PD_RX_DUPLICATE_INTR | PD_RX_TRANS_GCRC_FAIL_INTR)

/*interrupt classification - by register*/

#define PD_TX_SUCCESS0 (PD_TX_MSG_SUCCESS | PD_TX_AUTO_SR_SUCCESS)
#define PD_TX_FAIL0 (PD_TX_MSG_FAIL | PD_TX_AUTO_SR_FAIL)
#define PD_TX_INFO0 (PD_TX_MSG_INFO)
#define PD_RX_SUCCESS0 (PD_RX_MSG_SUCCESS)
#define PD_RX_INFO0 (PD_RX_MSG_FAIL_INFO)

#define PD_TX_SUCCESS1 (PD_TX_HR_SUCCESS)
#define PD_TX_FAIL1 (PD_TX_HR_FAIL)
#define PD_RX_SUCCESS1 (PD_RX_HR_SUCCESS)

#define PD_TX_EVENTS0_LISTEN (PD_TX_SUCCESS0 | PD_TX_FAIL0)
#define PD_TX_EVENTS0 (PD_TX_EVENTS0_LISTEN | PD_TX_INFO0)
#define PD_RX_EVENTS0_LISTEN (PD_RX_SUCCESS0)
#define PD_EVENTS0 (PD_RX_EVENTS0_LISTEN | PD_RX_INFO0)

#define PD_TX_EVENTS1_LISTEN (PD_TX_SUCCESS1 | PD_TX_FAIL1)
#define PD_TX_EVENTS1 (PD_TX_EVENTS1_LISTEN)
#define PD_RX_EVENTS1_LISTEN (PD_RX_SUCCESS1)
#define PD_EVENTS1 (PD_RX_EVENTS1_LISTEN | PD_TIMER0_TIMEOUT_INTR)

#define PD_INTR_EN_0_MSK (PD_TX_SUCCESS0 | PD_TX_FAIL0 | PD_TX_INFO0 | \
	PD_RX_SUCCESS0 | PD_RX_INFO0)

#define PD_INTR_EN_1_MSK (PD_TX_SUCCESS1 | PD_TX_FAIL1 | PD_RX_SUCCESS1 | \
	PD_TIMER0_TIMEOUT_INTR)

/* Time to wait to complete transmit */
#define PD_TX_TIMEOUT (100)/*(30)*/
#define PD_VDM_TX_TIMEOUT (500)

#define PD_STRESS_DELAY 2500

/* start as a sink in case we have no other power supply/battery */
#define PD_DEFAULT_STATE PD_STATE_SNK_DISCONNECTED

/* TODO: determine the following board specific type-C power constants */
/*
 * delay to turn on the power supply max is ~16ms.
 * delay to turn off the power supply max is about ~180ms.
 */

/*
 * tNewSrc -275ms
 * Maximum time allowed for an initial Sink in Swap
 * Standby to transition to new Source operation.
 */
#define PD_POWER_SUPPLY_TURN_ON_DELAY__T_NEW_SRC  (100*1)

/*
 * tSrcReady -285ms
 * Time from positive/negative transition start (t0)
 * to when VBUS is within the range defined by vSrcNew.
 */
#define PD_POWER_SUPPLY_TURN_ON_DELAY__T_SRC_READY  (20*1)

#define PD_POWER_SUPPLY_TURN_ON_DELAY  (20*1)

/*
 * tSrcSwapStdby -650ms
 * The maximum time for the Source to transition to
 * Swap Standby.
 */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY__T_SRC_SWAP_STDBY (550*1)
#define PD_VCONN_SWAP_DELAY (10*1)

/* Define typical operating power and max power */
#define PD_OPERATING_POWER_MW 7500 /* 5V*1.5A = 7.5W = 7500mW */
#define PD_MAX_POWER_MW       13500 /* 9V*1.5A = 13.5W = 13500mW */
#define PD_MAX_CURRENT_MA     1500
#define PD_MAX_VOLTAGE_MV     9000

#define PD_REF_CK (12 * 1000000)
#define PD_REF_CK_MS (PD_REF_CK / 1000)

enum vdm_states {
	VDM_STATE_ERR_BUSY = -3,
	VDM_STATE_ERR_SEND = -2,
	VDM_STATE_ERR_TMOUT = -1,
	VDM_STATE_DONE = 0,
	/* Anything >0 represents an active state */
	VDM_STATE_READY = 1,
	VDM_STATE_BUSY = 2,
	VDM_STATE_WAIT_RSP_BUSY = 3,
};

#endif
#endif
