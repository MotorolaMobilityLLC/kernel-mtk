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

 #include "mtk_typec.h"

#if SUPPORT_PD
#include "usb_pd_func.h"
#include "usb_pd.h"
#include "typec_reg.h"
#include <typec.h>
#include <mt6336/mt6336.h>
#include <mtk_clkbuf_ctl.h>

#ifdef CONFIG_USB_PD_DUAL_ROLE
#define DUAL_ROLE_IF_ELSE(hba, sink_clause, src_clause) \
	(hba->power_role == PD_ROLE_SINK ? (sink_clause) : (src_clause))
#else
#define DUAL_ROLE_IF_ELSE(hba, sink_clause, src_clause) (src_clause)
#endif

#define READY_RETURN_STATE(hba) DUAL_ROLE_IF_ELSE(hba, PD_STATE_SNK_READY, \
							 PD_STATE_SRC_READY)

/* Type C supply voltage (mV) */
#define TYPE_C_VOLTAGE	5000 /*mV*/

/* PD counter definitions */
#define PD_MESSAGE_ID_COUNT 7
#define PD_HARD_RESET_COUNT 2
#define PD_CAPS_COUNT 50
#define PD_SNK_CAP_RETRIES 3

#ifdef CONFIG_USB_PD_DUAL_ROLE
/* Last received source cap */
static uint32_t pd_src_caps[PDO_MAX_OBJECTS];
static int pd_src_cap_cnt;
#endif

static struct os_mapping {
	uint16_t type;
	char name[MAX_SIZE];
} pd_os_type_mapping[] = {
	{PD_TX_SOP, "SOP"},
	{PD_TX_SOP_PRIME, "SOP'"},
	{PD_TX_SOP_PRIME_PRIME, "SOP''"},
	{PD_TX_HARD_RESET, "HARD_RESET"},
	{PD_TX_CABLE_RESET, "CABLE_RESET"},
	{PD_TX_SOP_DEBUG_PRIME, "DBG_SOP'"},
	{PD_TX_SOP_DEBUG_PRIME_PRIME, "DBG_SPO''"},
};

static struct state_mapping {
	uint16_t state;
	char name[MAX_SIZE];
} pd_state_mapping[] = {
	{PD_STATE_DISABLED, "DISABLED"},
	{PD_STATE_SUSPENDED, "SUSPENDED"},
#ifdef CONFIG_USB_PD_DUAL_ROLE
	{PD_STATE_SNK_UNATTACH, "SNK_UNATTACH"},
	{PD_STATE_SNK_ATTACH, "SNK_ATTACH"},
	{PD_STATE_SNK_HARD_RESET_RECOVER, "SNK_HARD_RESET_RECOVER"},
	{PD_STATE_SNK_DISCOVERY, "SNK_DISCOVERY"},
	{PD_STATE_SNK_REQUESTED, "SNK_REQUESTED"},
	{PD_STATE_SNK_TRANSITION, "SNK_TRANSITION"},
	{PD_STATE_SNK_READY, "SNK_READY"},
	{PD_STATE_SNK_SWAP_INIT, "SNK_SWAP_INIT"},
	{PD_STATE_SNK_SWAP_SNK_DISABLE, "SNK_SWAP_SNK_DISABLE"},
	{PD_STATE_SNK_SWAP_SRC_DISABLE, "SNK_SWAP_SRC_DISABLE"},
	{PD_STATE_SNK_SWAP_STANDBY, "SNK_SWAP_STANDBY"},
	{PD_STATE_SNK_SWAP_COMPLETE, "SNK_SWAP_COMPLETE"},
#endif /* CONFIG_USB_PD_DUAL_ROLE */
	{PD_STATE_SRC_UNATTACH, "SRC_UNATTACH"},
	{PD_STATE_SRC_ATTACH, "SRC_ATTACH"},
	{PD_STATE_SRC_HARD_RESET_RECOVER, "SRC_HARD_RESET_RECOVER"},
	{PD_STATE_SRC_STARTUP, "SRC_STARTUP"},
	{PD_STATE_SRC_DISCOVERY, "SRC_DISCOVERY"},
	{PD_STATE_SRC_NEGOTIATE, "SRC_NEGOCIATE"},
	{PD_STATE_SRC_ACCEPTED, "SRC_ACCEPTED"},
	{PD_STATE_SRC_POWERED, "SRC_POWERED"},
	{PD_STATE_SRC_TRANSITION, "SRC_TRANSITION"},
	{PD_STATE_SRC_READY, "SRC_READY"},
	{PD_STATE_SRC_GET_SINK_CAP, "SRC_GET_SINK_CAP"},
	{PD_STATE_DR_SWAP, "DR_SWAP"},
#ifdef CONFIG_USB_PD_DUAL_ROLE
	{PD_STATE_SRC_SWAP_INIT, "SRC_SWAP_INIT"},
	{PD_STATE_SRC_SWAP_SNK_DISABLE, "SRC_SWAP_SNK_DISABLE"},
	{PD_STATE_SRC_SWAP_SRC_DISABLE, "SRC_SWAP_SRC_DISABLE"},
	{PD_STATE_SRC_SWAP_STANDBY, "SRC_SWAP_STANDBY"},
#ifdef CONFIG_USBC_VCONN_SWAP
	{PD_STATE_VCONN_SWAP_SEND, "VCONN_SWAP_SEND"},
	{PD_STATE_VCONN_SWAP_INIT, "VCONN_SWAP_INIT"},
	{PD_STATE_VCONN_SWAP_READY, "VCONN_SWAP_READY"},
#endif /* CONFIG_USBC_VCONN_SWAP */
#endif /* CONFIG_USB_PD_DUAL_ROLE */
	{PD_STATE_SOFT_RESET, "SOFT_RESET"},
	{PD_STATE_HARD_RESET_SEND, "HARD_RESET_SEND"},
	{PD_STATE_HARD_RESET_EXECUTE, "HARD_RESET_EXECUTE"},
	{PD_STATE_BIST_CMD, "BIST_CMD"},
	{PD_STATE_BIST_CARRIER_MODE_2, "BIST_CARRIER_MODE_2"},
	{PD_STATE_BIST_TEST_DATA, "BIST_TEST_DATA"},
	{PD_STATE_HARD_RESET_RECEIVED, "HARD_RESET_RECEIVED"},
	{PD_STATE_DISCOVERY_SOP_P, "DISCOVERY_SOP_P"},
	{PD_STATE_SRC_DISABLED, "SRC_DISABLED"},
	{PD_STATE_SNK_HARD_RESET_NOVSAFE5V, "SNK_HARD_RESET_NOVSAFE5V"},
	{PD_STATE_SNK_KPOC_PWR_OFF, "SNK_KPOC_PWR_OFF"},
	{PD_STATE_NO_TIMEOUT, "NO_TIMEOUT"},
};

struct bit_mapping pd_is0_mapping[] = {
	/*RX events*/
	{PD_RX_TRANS_GCRC_FAIL_INTR, "RX_TRANS_GCRC_FAIL"},
	{PD_RX_DUPLICATE_INTR, "RX_DUPLICATE"},
	{PD_RX_LENGTH_MIS_INTR, "RX_LENGTH_MIS"},
	{PD_RX_RCV_MSG_INTR, "RX_RCV_MSG"},
	/*TX events*/
	{PD_TX_AUTO_SR_PHY_LAYER_RST_DISCARD_MSG_INTR, "TX_AUTO_SR_PHY_LAYER_RST_DISCARD_MSG"},
	{PD_TX_AUTO_SR_RCV_NEW_MSG_DISCARD_MSG_INTR, "TX_AUTO_SR_RCV_NEW_MSG_DISCARD_MSG"},
	{PD_TX_AUTO_SR_RETRY_ERR_INTR, "TX_AUTO_SR_RETRY_ERR"},
	{PD_TX_AUTO_SR_DONE_INTR, "TX_AUTO_SR_DONE"},
	{PD_TX_CRC_RCV_TIMEOUT_INTR, "TX_CRC_RCV_TIMEOUT"},
	{PD_TX_DIS_BUS_REIDLE_INTR, "TX_DIS_BUS_REIDLE"},
	{PD_TX_PHY_LAYER_RST_DISCARD_MSG_INTR, "TX_PHY_LAYER_RST_DISCARD_MSG"},
	{PD_TX_RCV_NEW_MSG_DISCARD_MSG_INTR, "TX_RCV_NEW_MSG_DISCARD_MSG"},
	{PD_TX_RETRY_ERR_INTR, "TX_RETRY_ERR"},
	{PD_TX_DONE_INTR, "TX_DONE"},
};

struct bit_mapping pd_is1_mapping[] = {
	/*{PD_TIMER1_TIMEOUT_INTR, "TIMER1_TIMEOUT"},*/
	/*{PD_TIMER0_TIMEOUT_INTR, "TIMER0_TIMEOUT"},*/
	{PD_AD_PD_CC2_OVP_INTR, "AD_PD_CC2_OVP"},
	{PD_AD_PD_CC1_OVP_INTR, "AD_PD_CC1_OVP"},
	{PD_AD_PD_VCONN_UVP_INTR, "AD_PD_VCONN_UVP"},
	{PD_HR_TRANS_FAIL_INTR, "HR_TRANS_FAIL"},
	/*{PD_HR_RCV_DONE_INTR, "HR_RCV_DONE"},*/
	/*{PD_HR_TRANS_DONE_INTR, "HR_TRANS_DONE"},*/
	{PD_HR_TRANS_CPL_TIMEOUT_INTR, "HR_TRANS_CPL_TIMEOUT"},
};

struct bit_mapping pd_is0_mapping_dbg[] = {
	/*{PD_RX_RCV_MSG_INTR, "RX_RCV_MSG"},*/
	/*{PD_TX_DONE_INTR, "TX_DONE"},*/
};

struct bit_mapping pd_is1_mapping_dbg[] = {
	/*{PD_TIMER1_TIMEOUT_INTR, "TIMER1_TIMEOUT"},*/
	{PD_TIMER0_TIMEOUT_INTR, "TIMER0_TIMEOUT"},
	{PD_HR_RCV_DONE_INTR, "HR_RCV_DONE"},
	{PD_HR_TRANS_DONE_INTR, "HR_TRANS_DONE"},
};

static enum hrtimer_restart timeout_hrtimer_callback(struct hrtimer *timer)
{
	struct typec_hba *hba = container_of(timer, struct typec_hba, timeout_timer);

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
		dev_err(hba->dev, "%s Timeout=%lu State=%d\n", __func__,
				hba->timeout_ms, hba->timeout_state);

	hba->flags |= PD_FLAGS_TIMEOUT;

	hba->rx_event = true;
	wake_up(&hba->wq);

	return HRTIMER_NORESTART;
}

/* From Simpson Huang
 *Set RG_PD_RX_MODE[1:0] to change Rx Vth
 * 00: Power Neutral
 * 01: SNK power
 * 10: SRC power
 * 11: Reserved
 *
 * When detected to be SRC, set RX_MODE=10b and Rx Vth change
 * from 0.55v to 0.6v. The duty cycle of PHYA RX output is
 * 18.55%~18.45% for both Power Neutral and SRC test signal
 *
 * When detected to be SNK, set RX_MODE=01b and Rx Vth change
 * from 0.55v to 0.5v. The duty cycle of PHYA RX output is
 * 18.55%~18.45% for both Power Neutral and SNK test signal
 */
void pd_rx_phya_setting(struct typec_hba *hba)
{
#ifdef MT6336_E1
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
		dev_err(hba->dev, "E1 chip skip phya setting\n");
#else
	uint8_t tmp = typec_read8(hba, PD_PHY_RG_PD_0);

	tmp = tmp & (~REG_TYPE_C_PHY_RG_PD_RX_MODE);

	if (hba->task_state == PD_STATE_SRC_ATTACH)
		tmp = tmp | (0x2 << REG_TYPE_C_PHY_RG_PD_RX_MODE_OFST);
	else if (hba->task_state == PD_STATE_SNK_ATTACH)
		tmp = tmp | (0x1 << REG_TYPE_C_PHY_RG_PD_RX_MODE_OFST);
	else
		tmp = tmp | (0x0 << REG_TYPE_C_PHY_RG_PD_RX_MODE_OFST);

	typec_write8(hba, tmp, PD_PHY_RG_PD_0);
#endif
}

void pd_rx_enable(struct typec_hba *hba, uint8_t enable)
{
	if ((typec_readw(hba, PD_RX_PARAMETER) & REG_PD_RX_EN) ^ enable) {
		typec_writew_msk(hba, REG_PD_RX_EN, (enable ? REG_PD_RX_EN : 0), PD_RX_PARAMETER);

		if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
			dev_err(hba->dev, "RX %s\n", (enable ? "ON" : "OFF"));
	}
}

void pd_bist_enable(struct typec_hba *hba, uint8_t enable, uint32_t mode)
{
	if (enable) {
		if (mode == BDO_MODE_CARRIER2)
			typec_set(hba, PD_TX_BIST_CARRIER_MODE2_START, PD_TX_CTRL);
		hba->bist_mode = mode;
	} else {
		typec_clear(hba, PD_TX_BIST_CARRIER_MODE2_START, PD_TX_CTRL);
		hba->bist_mode = 0;
	}
}

void pd_int_enable(struct typec_hba *hba, uint8_t enable)
{
	if (enable) {
		typec_writew(hba, PD_INTR_EN_0_MSK, PD_INTR_EN_0);
		typec_writew(hba, PD_INTR_EN_1_MSK, PD_INTR_EN_1);
	} else {
		typec_writew(hba, 0, PD_INTR_EN_0);
		typec_writew(hba, 0, PD_INTR_EN_1);
	}
}

void pd_set_default_param(struct typec_hba *hba)
{
	/*set TX options*/
#ifdef AUTO_SEND_HR
	typec_clear(hba, (REG_PD_TX_AUTO_SEND_SR_EN | REG_PD_TX_AUTO_SEND_CR_EN),
		PD_TX_PARAMETER);
#else
	typec_clear(hba, (REG_PD_TX_AUTO_SEND_SR_EN | REG_PD_TX_AUTO_SEND_HR_EN | REG_PD_TX_AUTO_SEND_CR_EN),
		PD_TX_PARAMETER);
#endif
}

#define MAIN_CON4 (0x404)
#define RG_EN_CLKSQ_PDTYPEC_MSK (0x1<<1)
#define RG_EN_CLKSQ_PDTYPEC_OFST (1)

#define CLK_CKPDN_CON3 (0x034)
#define CLK_CKPDN_CON3_CLR (0x036)
#define CLK_TYPE_C_PD_CK_PDN_MSK (0x1<<4)
#define CLK_TYPE_C_PD_CK_PDN_OFST (4)

#define CLK_LOWQ_PDN_DIS_CON0 (0x041)
#define CLK_LOWQ_PDN_DIS_CON0_SET (0x042)
#define CLK_TYPE_C_PD_LOWQ_PDN_DIS_MSK (0x1<<7)
#define CLK_TYPE_C_PD_LOWQ_PDN_DIS_OFST (7)

void pd_basic_settings(struct typec_hba *hba)
{
/*
 * 5.1.1 Analog basic setting @ MT6336 PD programming guide v0.1.docx
 * 1.   0x0566[3]: RG_A_ANABASE_RSV[3] set 1'b1.  ' Enable the AVDD33 detection
 *      (should be set when using type-c. For PD no need to set again.)
 * 2.   0x0404[1]: RG_EN_CLKSQ_PDTYPEC set 1'b1. ' Enable clock 26MHz clock square function.
 * 3.   0x0036[4]: set 1'b1 ? Enable PD clock. (If want to turn off PD clock, 0x0035[4] set 1'b1)
 * 4.   If want PD clock work in lowQ mode, 0x0042[7] set 1'b1 (0x0043[7] set 1'b1 to turn off this clock in lowQ)
 */
	const int is_print = 0;

	typec_write8_msk(hba, RG_EN_CLKSQ_PDTYPEC_MSK, (1<<RG_EN_CLKSQ_PDTYPEC_OFST), MAIN_CON4);
	if (is_print)
		dev_err(hba->dev, "MAIN_CON4(0x404)=0x%x [0x2]\n", typec_read8(hba, MAIN_CON4));

	typec_write8(hba, (1<<CLK_TYPE_C_PD_CK_PDN_OFST), CLK_CKPDN_CON3_CLR);
	if (is_print)
		dev_err(hba->dev, "CLK_CKPDN_CON3(0x34)=0x%x [Clear 0x10(0x36)]\n", typec_read8(hba, CLK_CKPDN_CON3));

#ifndef MT6336_E1
	typec_write8(hba, 0x5, 0x2F2);
	if (is_print)
		dev_err(hba->dev, "0x2F2=0x%x [0x5]\n", typec_read8(hba, 0x2F2));

	typec_write8(hba, 0x2, 0x2F3);
	if (is_print)
		dev_err(hba->dev, "0x2F3=0x%x [0x2]\n", typec_read8(hba, 0x2F3));
#endif

	typec_write8(hba, (1<<CLK_TYPE_C_PD_LOWQ_PDN_DIS_OFST), CLK_LOWQ_PDN_DIS_CON0_SET);
	if (is_print)
		dev_err(hba->dev, "CLK_LOWQ_PDN_DIS_CON0=0x%x\n", typec_read8(hba, CLK_LOWQ_PDN_DIS_CON0));
}

void pd_init(struct typec_hba *hba)
{
	init_waitqueue_head(&hba->wq);
	hba->tx_event = false;
	hba->rx_event = false;

	wake_lock_init(&hba->typec_wakelock, WAKE_LOCK_SUSPEND, "TYPEC.lock");

	pd_set_default_param(hba);

	/*turn on PD module but leave PD-RX disabled*/
	typec_set(hba, TYPE_C_SW_PD_EN, TYPE_C_CC_SW_CTRL);
	pd_rx_enable(hba, 0);
	pd_int_enable(hba, 1);

	hrtimer_init(&hba->timeout_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hba->timeout_timer.function = timeout_hrtimer_callback;

	/*start PD task*/
	if (hba->is_kpoc == true)
		kthread_run(pd_kpoc_task, (void *)hba, "pd_kpoc_task");
	else
		kthread_run(pd_task, (void *)hba, "pd_task");
}

void pd_set_event(struct typec_hba *hba, uint16_t pd_is0, uint16_t pd_is1, uint16_t cc_is0)
{
	hba->pd_is0 |= pd_is0;
	hba->pd_is1 |= pd_is1;
	hba->cc_is0 |= cc_is0;
}

void pd_clear_event(struct typec_hba *hba, uint16_t pd_is0, uint16_t pd_is1, uint16_t cc_is0)
{
	hba->pd_is0 &= ~pd_is0;
	hba->pd_is1 &= ~pd_is1;
	hba->cc_is0 &= ~cc_is0;
}

/**
 * ufshcd_pd_intr - Interrupt service routine
 * @hba: per adapter instance
 * @pd_is0: PD interrupt status 0
 * @pd_is1: PD interrupt status 1
 * @cc_is0: CC interrupt status 0
 * @cc_is2: CC interrupt status 2
 */
void pd_intr(struct typec_hba *hba, uint16_t pd_is0, uint16_t pd_is1, uint16_t cc_is0, uint16_t cc_is2)
{
	uint16_t cc_event;
	uint16_t tx_event;
	uint16_t rx_event;
	uint16_t timer_event;

	/*leave handling to main loop*/
	pd_set_event(hba, pd_is0, pd_is1, cc_is0);

#if PD_SW_WORKAROUND1_1
	if (pd_is0 & PD_RX_RCV_MSG_INTR) {
		hba->header = 0;
		pd_get_message(hba, &hba->header, hba->payload);
		typec_writew(hba, PD_RX_RCV_MSG_INTR, PD_INTR_0);
	}
#endif

	/* event type */
	tx_event = ((pd_is0 & PD_TX_EVENTS0_LISTEN) || (pd_is1 & PD_TX_EVENTS1_LISTEN));
	rx_event = ((pd_is0 & PD_RX_EVENTS0_LISTEN) || (pd_is1 & PD_RX_EVENTS1_LISTEN));
	cc_event = (cc_is0 & PD_INTR_IS0_LISTEN);
	timer_event = (pd_is1 & PD_TIMER0_TIMEOUT_INTR);

	/* TX events */
	if (tx_event) {
		hba->tx_event = true;
		wake_up(&hba->wq);
	}

	/* CC & RX & timer events */
	if (cc_event || rx_event || timer_event) {
		if (pd_is1 & PD_HR_RCV_DONE_INTR) {
			dev_err(hba->dev, "%s Receive HardReset\n", __func__);
			hba->tx_event = true;
			wake_up(&hba->wq);
			if (hba->is_kpoc)
				typec_vbus_det_enable(hba, 0);
		}

		hba->rx_event = true;
		wake_up(&hba->wq);
	}

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
		dev_err(hba->dev, "%s pd0=0x%X, pd1=0x%X cc0=0x%X cc2=0x%X %s %s %s %s\n", __func__,
			pd_is0, pd_is1, cc_is0, cc_is2,
			tx_event?"TX":" ",
			cc_event?"CC":" ",
			rx_event?"RX":" ",
			timer_event?"TMR":" ");
}

inline void pd_complete_hr(struct typec_hba *hba)
{
	typec_set(hba, W1_PD_PE_HR_CPL, PD_HR_CTRL);
}

void pd_set_msg_id(struct typec_hba *hba, uint8_t msg_id)
{
	int i;


	for (i = PD_TX_SOP; i <= PD_TX_SOP_PRIME_PRIME; i++) {
		typec_writew_msk(hba, REG_PD_TX_OS, (i<<REG_PD_TX_OS_OFST), PD_TX_CTRL);
		typec_writew_msk(hba, REG_PD_SW_MSG_ID, msg_id, PD_MSG_ID_SW_MODE);
		typec_set(hba, PD_SW_MSG_ID_SYNC, PD_MSG_ID_SW_MODE);
	}
}

inline void set_state_timeout(struct typec_hba *hba,
	unsigned long ms_timeout, enum pd_states timeout_state)
{
	if (hrtimer_try_to_cancel(&hba->timeout_timer) == -1)
		dev_err(hba->dev, "Can not cancel timeout_timer\n");

	if (timeout_state == PD_STATE_NO_TIMEOUT) {
		/*Cancel the timeout timer*/
		hba->timeout_ms = ULONG_MAX;
		/*TODO: When cancel the timeout state, what state should be set?*/
		/*hba->timeout_state = PD_STATE_NO_TIMEOUT;*/
	} else {
		hba->timeout_state = timeout_state;
		hba->timeout_ms = ms_timeout;

		if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
			dev_err(hba->dev, "Set timeout=%lu state=%s\n", ms_timeout,
					pd_state_mapping[hba->timeout_state].name);

		hrtimer_start(&hba->timeout_timer, ms_to_ktime(ms_timeout), HRTIMER_MODE_REL);
	}
}

/* Return flag for pd state is connected */
int pd_is_connected(struct typec_hba *hba)
{
	if ((hba->task_state == PD_STATE_DISABLED) ||
		(hba->task_state == PD_STATE_SNK_UNATTACH) ||
		(hba->task_state == PD_STATE_SRC_UNATTACH))
		return 0;

	return DUAL_ROLE_IF_ELSE(hba,
		/* sink */
		hba->task_state != PD_STATE_DISABLED &&
		hba->task_state != PD_STATE_SNK_UNATTACH,
		/* source */
		hba->task_state != PD_STATE_DISABLED &&
		hba->task_state != PD_STATE_SRC_UNATTACH);
}

inline int state_changed(struct typec_hba *hba)
{
	return (hba->last_state != hba->task_state);
}

void set_state(struct typec_hba *hba, enum pd_states next_state)
{
	set_state_timeout(hba, 0, PD_STATE_NO_TIMEOUT);
	hba->task_state = next_state;
}

int pd_transmit(struct typec_hba *hba, enum pd_transmit_type type,
		       uint16_t header, const uint32_t *data)
{
	int ret = -1;
	uint8_t cnt;
	uint16_t pd_is0, pd_is1;
	int timeout = PD_TX_TIMEOUT;
	#ifdef PROFILING
	ktime_t start, end;
	#endif

	#ifdef PROFILING
	start = ktime_get();
	#endif

	if (hba->is_shutdown)
		return 0;

	/*reception ordered set*/
	typec_write8_msk(hba, REG_PD_TX_OS, (type<<REG_PD_TX_OS_OFST), PD_TX_CTRL);

	/*prepare header*/
	cnt = PD_HEADER_CNT(header);
	typec_writew(hba, header, PD_TX_HEADER);

	/*prepare data*/
	if (cnt == 1)
		typec_writedw(hba, *data, PD_TX_DATA_OBJECT0_0);
#ifdef NEVER
	else if (cnt == 3) {
		uint8_t v[4*3];

		v[0] = data[0] & 0xFF;
		v[1] = (data[0] >> 8) & 0xFF;
		v[2] = (data[0] >> 16) & 0xFF;
		v[3] = (data[0] >> 24) & 0xFF;

		v[4] = data[1] & 0xFF;
		v[5] = (data[1] >> 8) & 0xFF;
		v[6] = (data[1] >> 16) & 0xFF;
		v[7] = (data[1] >> 24) & 0xFF;

		v[8] = data[2] & 0xFF;
		v[9] = (data[2] >> 8) & 0xFF;
		v[10] = (data[2] >> 16) & 0xFF;
		v[11] = (data[2] >> 24) & 0xFF;

		mt6336_write_bytes(PD_TX_DATA_OBJECT0_0, v, 4*3);
	} else if (cnt == 4) {
		uint8_t v[4*4];

		v[0] = data[0] & 0xFF;
		v[1] = (data[0] >> 8) & 0xFF;
		v[2] = (data[0] >> 16) & 0xFF;
		v[3] = (data[0] >> 24) & 0xFF;

		v[4] = data[1] & 0xFF;
		v[5] = (data[1] >> 8) & 0xFF;
		v[6] = (data[1] >> 16) & 0xFF;
		v[7] = (data[1] >> 24) & 0xFF;

		v[8] = data[2] & 0xFF;
		v[9] = (data[2] >> 8) & 0xFF;
		v[10] = (data[2] >> 16) & 0xFF;
		v[11] = (data[2] >> 24) & 0xFF;

		v[12] = data[3] & 0xFF;
		v[13] = (data[3] >> 8) & 0xFF;
		v[14] = (data[3] >> 16) & 0xFF;
		v[15] = (data[3] >> 24) & 0xFF;

		mt6336_write_bytes(PD_TX_DATA_OBJECT0_0, v, 4*4);
	} else {
#endif /* NEVER */
	else {
		int i;

		for (i = 0; i < cnt; i++)
			typec_writedw(hba, data[i], (PD_TX_DATA_OBJECT0_0+i*4));
	}

	hba->tx_event = false;

	/*send message*/
	typec_set8(hba, PD_TX_START, PD_TX_CTRL);

	#ifdef PROFILING
	end = ktime_get();
	dev_err(hba->dev, "TX took %lld us", ktime_to_ns(ktime_sub(end, start))/1000);
	#endif

	/*mask off message id because it's controlled by HW*/
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
		dev_err(hba->dev, "TX (%s) %04x/%d/%X",
			pd_os_type_mapping[type].name, (header & ~PD_HEADER_ID_MSK), cnt, PD_HEADER_TYPE(header));

	/*report unhandled TX events before sending*/
	if (hba->pd_is0 & PD_TX_EVENTS0 || hba->pd_is1 & PD_TX_EVENTS1)
		dev_err(hba->dev, "%s unhandled events pd_is0: %x, pd_is1: %x", __func__,
			hba->pd_is0 & PD_TX_EVENTS0, hba->pd_is1 & PD_TX_EVENTS1);

	/*give VDM more time to timeout*/
	if (hba->is_kpoc || (PD_HEADER_TYPE(header) == PD_DATA_VENDOR_DEF))
		timeout = PD_VDM_TX_TIMEOUT;

	if (wait_event_timeout(hba->wq, hba->tx_event, msecs_to_jiffies(timeout))) {
		/*clean up TX events*/
		mutex_lock(&hba->typec_lock);

		pd_is0 = hba->pd_is0;
		pd_is1 = hba->pd_is1;

		pd_clear_event(hba, (pd_is0 & PD_TX_EVENTS0), (pd_is1 & PD_TX_EVENTS1), 0);
		mutex_unlock(&hba->typec_lock);

		if (pd_is1 & PD_TX_HR_SUCCESS) {
			pd_complete_hr(hba);
#ifdef AUTO_SEND_HR
			if (hba->task_state == PD_STATE_SOFT_RESET)
				hba->hr_auto_sent = 1;
#endif
		}

		if ((pd_is0 & PD_TX_SUCCESS0) | (pd_is1 & PD_TX_SUCCESS1))
			ret = 0;
		else if ((pd_is0 & PD_TX_FAIL0) | (pd_is1 & PD_TX_FAIL1))
			ret = 1;
		else if (pd_is1 & PD_HR_RCV_DONE_INTR) {
			ret = 0;
			dev_err(hba->dev, "Receive HardReset");
		}

		if (pd_is0 & PD_TX_RCV_NEW_MSG_DISCARD_MSG_INTR)
			dev_err(hba->dev, "Receive New Msg Discard Our Msg");
	} else {
		ret = 2;
		dev_err(hba->dev, "%s TX timeout", __func__);
	}

	if (unlikely(ret != 0))
		dev_err(hba->dev, "%s ret=%d", __func__, ret);

	return ret;
}

void pd_update_roles(struct typec_hba *hba)
{
	typec_writew_msk(hba,
		(REG_PD_TX_HDR_PORT_POWER_ROLE | REG_PD_TX_HDR_PORT_DATA_ROLE),
		(hba->power_role << REG_PD_TX_HDR_PORT_POWER_ROLE_OFST |
		 hba->data_role << REG_PD_TX_HDR_PORT_DATA_ROLE_OFST),
		PD_TX_HEADER);
}

int send_control(struct typec_hba *hba, enum pd_ctrl_msg_type type)
{
	int ret;
	uint16_t header;

	struct type_mapping {
		uint16_t type;
		char name[32];
	} pd_ctrl_type_mapping[] = {
		{PD_CTRL_RESERVED, "CTRL_RESERVED"},
		{PD_CTRL_GOOD_CRC, "CTRL_GOOD_CRC"},
		{PD_CTRL_GOTO_MIN, "CTRL_GOTO_MIN"},
		{PD_CTRL_ACCEPT, "CTRL_ACCEPT"},
		{PD_CTRL_REJECT, "CTRL_REJECT"},
		{PD_CTRL_PING, "CTRL_PING"},
		{PD_CTRL_PS_RDY, "CTRL_PS_RDY"},
		{PD_CTRL_GET_SOURCE_CAP, "CTRL_GET_SOURCE_CAP"},
		{PD_CTRL_GET_SINK_CAP, "CTRL_GET_SINK_CAP"},
		{PD_CTRL_DR_SWAP, "CTRL_DR_SWAP"},
		{PD_CTRL_PR_SWAP, "CTRL_PR_SWAP"},
		{PD_CTRL_VCONN_SWAP, "CTRL_VCONN_SWAP"},
		{PD_CTRL_WAIT, "CTRL_WAIT"},
		{PD_CTRL_SOFT_RESET, "CTRL_SOFT_RESET"},
	};

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
		dev_err(hba->dev, "%s %s", __func__, pd_ctrl_type_mapping[type].name);

	header = PD_HEADER(type, hba->power_role, hba->data_role, 0);
	ret = pd_transmit(hba, PD_TX_SOP, header, NULL);

	return ret;
}

#ifdef SUPPORT_SOP_P
int send_cable_discovery_identity(struct typec_hba *hba, int type)
{
	int ret;
	uint16_t header;
	uint32_t vdm_header;

	/* set VDM header with VID & CMD */
	vdm_header = VDO(USB_SID_PD, 1, VDO_SVDM_VERS(0) |
						(VDO_OPOS(0) & VDO_OPOS_MASK) |
						(VDO_CMDT(CMDT_INIT) & VDO_CMDT_MASK) |
						CMD_DISCOVER_IDENT);

	/* Prepare and send VDM */
	header = PD_HEADER(PD_DATA_VENDOR_DEF, 0, 0, 1);
	ret = pd_transmit(hba, type, header, &vdm_header);

	return ret;
}
#endif

int send_source_cap(struct typec_hba *hba)
{
	int i;
	int ret;
	uint16_t header;

	/*flag for DRP*/
	for (i = 0; i < pd_src_pdo_cnt; i++) {
		if ((pd_src_pdo[i] & PDO_TYPE_MASK) == PDO_TYPE_FIXED) {
			if (hba->support_role == TYPEC_ROLE_DRP)
				pd_src_pdo[i] |= (PDO_FIXED_DUAL_ROLE | PDO_FIXED_DATA_SWAP);
			else
				pd_src_pdo[i] &= ~(PDO_FIXED_DUAL_ROLE | PDO_FIXED_DATA_SWAP);
		}
	}

	if (pd_src_pdo_cnt == 0)
		/* No source capabilities defined, sink only */
		header = PD_HEADER(PD_CTRL_REJECT, hba->power_role, hba->data_role, 0);
	else
		header = PD_HEADER(PD_DATA_SOURCE_CAP, hba->power_role, hba->data_role, pd_src_pdo_cnt);
	ret = pd_transmit(hba, PD_TX_SOP, header, pd_src_pdo);

	return ret;
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
int send_sink_cap(struct typec_hba *hba)
{
	int i;
	int ret;
	uint16_t header;
	int is_1st_fixed = 1;

	/*flag for DRP*/
	for (i = 0; i < pd_snk_pdo_cnt; i++) {
		if (((pd_snk_pdo[i] & PDO_TYPE_MASK) == PDO_TYPE_FIXED) && (is_1st_fixed == 1)) {
			if (hba->support_role == TYPEC_ROLE_DRP)
				pd_snk_pdo[i] |= PDO_FIXED_DUAL_ROLE;
			else
				pd_snk_pdo[i] &= ~PDO_FIXED_DUAL_ROLE;
			is_1st_fixed = 0;
		}
	}

	header = PD_HEADER(PD_DATA_SINK_CAP, hba->power_role, hba->data_role, pd_snk_pdo_cnt);
	ret = pd_transmit(hba, PD_TX_SOP, header, pd_snk_pdo);

	return ret;
}

int send_request(struct typec_hba *hba, uint32_t rdo)
{
	int ret;
	uint16_t header;

	header = PD_HEADER(PD_DATA_REQUEST, hba->power_role, hba->data_role, 1);
	ret = pd_transmit(hba, PD_TX_SOP, header, &rdo);

	return ret;
}
#endif /* CONFIG_USB_PD_DUAL_ROLE */

int send_bist_cmd(struct typec_hba *hba, uint32_t mode)
{
	uint32_t bdo;
	int ret;
	uint16_t header;


	if (mode == BDO_MODE_CARRIER2 || mode == BDO_MODE_TEST_DATA)
		bdo = BDO(mode, 0);
	else
		return 1;

	header = PD_HEADER(PD_DATA_BIST, hba->power_role, hba->data_role, 1);
	ret = pd_transmit(hba, PD_TX_SOP, header, &bdo);

	return ret;
}

void queue_vdm(struct typec_hba *hba, uint32_t *header, const uint32_t *data,
			     int data_cnt)
{
	/*The additional one is for VDM header*/
	hba->vdo_count = data_cnt + 1;
	hba->vdo_data[0] = header[0];
	memcpy(&hba->vdo_data[1], data, sizeof(uint32_t) * data_cnt);
	/* Set ready, pd task will actually send */
	hba->vdm_state = VDM_STATE_READY;
}

static void handle_vdm_request(struct typec_hba *hba, int cnt, uint32_t *payload)
{
	int rlen = 0;
	uint32_t *rdata;

	if (hba->vdm_state == VDM_STATE_BUSY) {
		/* If UFP responded busy retry after timeout */
		if (PD_VDO_CMDT(payload[0]) == CMDT_RSP_BUSY) {
			hba->vdm_timeout = jiffies +
				msecs_to_jiffies(PD_T_VDM_BUSY);
			hba->vdm_state = VDM_STATE_WAIT_RSP_BUSY;
			hba->vdo_retry = (payload[0] & ~VDO_CMDT_MASK) |
				CMDT_INIT;
			goto end;
		} else {
			hba->vdm_state = VDM_STATE_DONE;
		}
	}

	/*Check VDM type. Structured/Unstructured VDM*/
	if (PD_VDO_SVDM(payload[0]))
		rlen = pd_svdm(hba, cnt, payload, &rdata);
	else {
		/*Handle Unstructured VDM*/
		rlen = pd_custom_vdm(hba, cnt, payload, &rdata);
	}

	if (rlen > 0) {
		queue_vdm(hba, rdata, &rdata[1], rlen - 1);
		goto end;
	}

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
		dev_err(hba->dev, "Unhandled VDM VID %04x CMD %04x\n",
			PD_VDO_VID(payload[0]), payload[0] & 0xFFFF);

end:
	return;
}

#ifdef SUPPORT_SOP_P
void handle_cbl_info(struct typec_hba *hba, int cnt, uint32_t *payload)
{
	struct cable_info *cbl_inf = NULL;

	dev_err(hba->dev, "%s\n", __func__);

	if (hba->cable_flags & PD_FLAGS_CBL_DISCOVERYING_SOP_P) {
		hba->cable_flags &= ~PD_FLAGS_CBL_DISCOVERYING_SOP_P;
		hba->cable_flags |= PD_FLAGS_CBL_DISCOVERIED_SOP_P;

		cbl_inf = &hba->sop_p;
	}

	if ((cbl_inf != NULL) && (PD_VDO_SVDM(payload[0]) == 1) &&
			(PD_VDO_VID(payload[0]) == USB_SID_PD) && cnt == 5) {

		memcpy(cbl_inf, &payload[1], sizeof(struct cable_info));

		dev_err(hba->dev, "0x%08x 0x%08x 0x%08x 0x%08x\n", cbl_inf->id_header, cbl_inf->cer_stat_vdo,
			cbl_inf->product_vdo, cbl_inf->cable_vdo);
	} else {
		dev_err(hba->dev, "What is this?\n");
	}
}
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE
void pd_set_power_role(struct typec_hba *hba, uint8_t role, uint8_t update_hdr)
{
	uint8_t cur_role;
	uint16_t tmp;

	/* header */
	if (1) {
		hba->power_role = role;
		pd_update_roles(hba);

		/*Ignores PING for SNK*/
		/*related compliance test items: TD.PD.LL.E6 Ping & TD.PD.SRC.E14 Atomic Message Sequence*/
		if (role == PD_ROLE_SINK)
			typec_clear(hba, REG_PD_RX_PING_MSG_RCV_EN, PD_RX_PARAMETER);
		else
			typec_set(hba, REG_PD_RX_PING_MSG_RCV_EN, PD_RX_PARAMETER);
	}

	/* termination */
	/*this register ONLY updates when entering SRC_ATTACH or SNK_ATTACH state; 0 SNK, 1 SRC*/
	cur_role = ((typec_readw(hba, TYPE_C_PWR_STATUS) & RO_TYPE_C_CC_PWR_ROLE) >> 4);

	/*toggle PR when necessary*/
	if (cur_role != role) {
		/*setting this changes termination and generates Attached.SNK->Attached.SRC
		 * or Attached.SRC->Attached.SNK event we don't have to serve generated event
		 * in the middle of power role swap
		 */
		typec_set(hba, W1_TYPE_C_SW_PR_SWAP_INDICATE_CMD, TYPE_C_CC_SW_CTRL);

		/* TYPEC controller syncs register with Khz frequency that is much smaller
		 * than PD controller (MHz). Wait a while for this to take effect, before
		 * proceeding to PD actions
		 */
		tmp = (role == TYPEC_ROLE_SOURCE) ? TYPEC_STATE_ATTACHED_SRC : TYPEC_STATE_ATTACHED_SNK;

		while ((typec_readw(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST) != tmp)
			;

		usleep_range(1000, 2000);

		if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
			dev_err(hba->dev, "Switch to %s", (role == TYPEC_ROLE_SOURCE) ? "Rp" : "Rd");
	}
}
#endif

void pd_execute_hard_reset(struct typec_hba *hba)
{
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
		dev_err(hba->dev, "%s", __func__);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	pd_dfp_exit_mode(hba, 0, 0);
#endif
	/*
	 * No need to reset Message ID by SW. HW would reset Message ID auto.
	 * Transmit or Receive Hard Reset will clear all SOP/SOP'/SOP" Message ID.
	 * Transmit or Receive Soft Reset will clear one of SOP* Message ID which
	 * is the same as SOP* type of Soft Reset is received or transmitted.
	*/

	/*TX header PD spec revision should be 01b=2.0*/
	typec_writew_msk(hba, 0x3<<6, PD_REV20<<6, PD_TX_HEADER);

	/* 6.7.3.2 Type-C and Hard Reset
	 * If there has been a Data Role Swap the Hard Reset shall cause the Port
	 * Data Role to be changed back to DFP for a Port with the Rp resistor
	 * asserted and UFP for a Port with the Rd resistor asserted.
	 */
	if (hba->power_role == PD_ROLE_SINK && hba->data_role == PD_ROLE_DFP)
		pd_set_data_role(hba, PD_ROLE_UFP);
	else if (hba->power_role == PD_ROLE_SOURCE && hba->data_role == PD_ROLE_UFP)
		pd_set_data_role(hba, PD_ROLE_DFP);

	/*
	 * Fake set last state to hard reset to make sure that the next
	 * state to run knows that we just did a hard reset.
	 */
	hba->last_state = PD_STATE_HARD_RESET_EXECUTE;

	hba->vdm_state = VDM_STATE_DONE;
	hba->flags &= ~PD_FLAGS_EXPLICIT_CONTRACT;
	hba->flags &= ~PD_FLAGS_SNK_CAP_RECVD;

#ifdef CONFIG_USB_PD_DUAL_ROLE
	/*
	 * If we are swapping to a source and have changed to Rp, restore back
	 * to Rd and turn off vbus to match our power_role.
	 */
	if (hba->task_state == PD_STATE_SNK_SWAP_STANDBY ||
	    hba->task_state == PD_STATE_SNK_SWAP_COMPLETE) {
		pd_set_power_role(hba, PD_ROLE_SINK, 1);
		pd_power_supply_reset(hba);
	}

	if (hba->power_role == PD_ROLE_SINK) {
		/* Clear the input current limit */
		pd_set_input_current_limit(hba, 0, 0);
#ifdef CONFIG_CHARGE_MANAGER
		charge_manager_set_ceil(hba,
					CEIL_REQUESTOR_PD,
					CHARGE_CEIL_NONE);
#endif /* CONFIG_CHARGE_MANAGER */

		if (hba->flags & PD_FLAGS_VCONN_ON) {
			typec_drive_vconn(hba, 0);
			hba->flags &= ~PD_FLAGS_VCONN_ON;
		}

		typec_vbus_det_enable(hba, 0);
		set_state(hba, PD_STATE_SNK_HARD_RESET_RECOVER);

		return;
	}
#endif /* CONFIG_USB_PD_DUAL_ROLE */

	/* We are a source, cut power */
	pd_power_supply_reset(hba);

	/* Use this way,
	 * 1. We want to stay at this state PD_STATE_SRC_HARD_RESET_RECOVER,
	 *  not PD_STATE_HARD_RESET_EXECUTE. If go to PD_STATE_HARD_RESET_EXECUTE,
	 *  it will call pd_execute_hard_reset() over and over again.
	 * 2. We want to execute this state PD_STATE_SRC_HARD_RESET_RECOVER
	 *  after tSrcRecover strongly. So use src_recover to block all actions
	 *  before tSrcRecover.
	 */
	hba->src_recover = jiffies + msecs_to_jiffies(PD_T_SRC_RECOVER);
	set_state(hba, PD_STATE_SRC_HARD_RESET_RECOVER);
}

void execute_soft_reset(struct typec_hba *hba)
{
	/*go back to DISCOVERY state for explicit contract negotiation*/
	hba->flags &= ~PD_FLAGS_EXPLICIT_CONTRACT;
	set_state(hba, DUAL_ROLE_IF_ELSE(hba, PD_STATE_SNK_DISCOVERY,
						PD_STATE_SRC_DISCOVERY));

	hba->vdm_state = VDM_STATE_DONE;

	/*
	 * No need to reset Message ID by SW. HW would reset Message ID auto.
	 * Transmit or Receive Hard Reset will clear all SOP/SOP'/SOP" Message ID.
	 * Transmit or Receive Soft Reset will clear one of SOP* Message ID which
	 * is the same as SOP* type of Soft Reset is received or transmitted.
	*/

#if 0
	/* if flag to disable PD comms after soft reset, then disable comms */
	if (hba->flags & PD_FLAGS_SFT_RST_DIS_COMM)
		pd_rx_enable(hba, 0);
#endif
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
#ifdef NEVER
void dump_pdo(struct typec_hba *hba, enum pd_data_msg_type type, int cnt, uint32_t *caps)
{
	int i = 0;

	dev_err(hba->dev, "===%s PDO===", ((type == PD_DATA_SOURCE_CAP)?"SRC":"SNK"));

	for (i = 0; i < cnt; i++) {
		int pdo_type = caps[i] & PDO_TYPE_MASK;

		switch (pdo_type) {
		case PDO_TYPE_FIXED:
			dev_err(hba->dev, "FIXED %s %s %s %s %s Peak=%d %dmV %dmA",
				((caps[i] & PDO_FIXED_DUAL_ROLE)?"DRP":""),
				((type == PD_DATA_SOURCE_CAP) ?
					((caps[i] & PDO_FIXED_USB_SUSPEND)?"USB Suspend":"") :
					((caps[i] & PDO_FIXED_HIGHER_CAP)?"High Cap":"")),
				((caps[i] & PDO_FIXED_EXTERNAL)?"EXT PWR":""),
				((caps[i] & PDO_FIXED_COMM_CAP)?"USB COMM":""),
				((caps[i] & PDO_FIXED_DATA_SWAP)?"DR SWAP":""),
				((caps[i] & PDO_FIXED_PEAK_CURR_MSK) >> PDO_FIXED_PEAK_CURR_OFST),
				(((caps[i] & PDO_FIXED_VOLT_MSK) >> PDO_FIXED_VOLT_OFST)*50),
				(((caps[i] & PDO_FIXED_CURR_MSK) >> PDO_FIXED_CURR_OFST)*10)
				);
		break;
		case PDO_TYPE_BATTERY:
			dev_err(hba->dev, "BAT Min=%dmV Max=%dmV %dmW",
				(((caps[i] & PDO_BATT_MAX_VOLT_MSK) >> PDO_BATT_MAX_VOLT_OFST)*50),
				(((caps[i] & PDO_BATT_MIN_VOLT_MSK) >> PDO_BATT_MIN_VOLT_OFST)*50),
				(((caps[i] & PDO_BATT_PWR_MSK) >> PDO_BATT_PWR_OFST)*250)
				);
		break;
		case PDO_TYPE_VARIABLE:
			dev_err(hba->dev, "VAR Min=%dmV Max=%dmV %dmA",
				(((caps[i] & PDO_VAR_MAX_VOLT_MSK) >> PDO_VAR_MAX_VOLT_OFST)*50),
				(((caps[i] & PDO_VAR_MIN_VOLT_MSK) >> PDO_VAR_MIN_VOLT_OFST)*50),
				(((caps[i] & PDO_VAR_CURR_MSK) >> PDO_VAR_CURR_OFST)*10)
				);
		break;
		default:
			dev_err(hba->dev, "Reserved");
		break;
		}
	}

	dev_err(hba->dev, "===PDO===");
}
#endif /* NEVER */

void pd_store_src_cap(struct typec_hba *hba, int cnt, uint32_t *src_caps)
{
	int i;

	/*cap*/
	pd_src_cap_cnt = cnt;
	for (i = 0; i < cnt; i++) {
		pd_src_caps[i] = *src_caps;
		src_caps++;
	}
}

void pd_send_request_msg(struct typec_hba *hba, int always_send_request)
{
	uint32_t rdo, curr_limit, supply_voltage;
	int ret;

#ifdef CONFIG_CHARGE_MANAGER
	int charging = (charge_manager_get_active_charge_port() == port);
#else
	const int charging = 1;
#endif

#ifdef CONFIG_USB_PD_CHECK_MAX_REQUEST_ALLOWED
	int max_request_allowed = pd_is_max_request_allowed();
#else
	const int max_request_allowed = 1;
#endif

	/* Clear new power request */
	hba->new_power_request = 0;

	/* Build and send request RDO */
	/*
	 * If this port is not actively charging or we are not allowed to
	 * request the max voltage, then select vSafe5V
	 */
	ret = pd_build_request(pd_src_cap_cnt, pd_src_caps,
			       &rdo, &curr_limit, &supply_voltage,
			       charging && max_request_allowed ?
					PD_REQUEST_MAX : PD_REQUEST_VSAFE5V);

	if (ret)
		/*
		 * If fail to choose voltage, do nothing, let source re-send
		 * source cap
		 */
		return;

	/* Don't re-request the same voltage */
	if (!always_send_request && hba->prev_request_mv == supply_voltage)
		return;

	hba->curr_limit = curr_limit;
	hba->supply_voltage = supply_voltage;
	hba->prev_request_mv = supply_voltage;
	ret = send_request(hba, rdo);
	if (!ret)
		set_state(hba, PD_STATE_SNK_REQUESTED);
	else
		set_state(hba, PD_STATE_SOFT_RESET);
	/* If fail send request, do nothing, let source re-send source cap */

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2 && !ret) {
		dev_err(hba->dev, "Req [%d] %dmV %dmA", RDO_POS(rdo), supply_voltage, curr_limit);
		if (rdo & RDO_CAP_MISMATCH)
			dev_err(hba->dev, " Mismatch");
	}
}
#endif

void pd_update_pdo_flags(struct typec_hba *hba, uint32_t pdo)
{
#ifdef CONFIG_CHARGE_MANAGER
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	int charge_whitelisted =
		(hba->power_role == PD_ROLE_SINK &&
		 pd_charge_from_device(pd_get_identity_vid(hba),
				       pd_get_identity_pid(hba)));
#else
	const int charge_whitelisted = 0;
#endif
#endif

	/* can only parse PDO flags if type is fixed */
	if ((pdo & PDO_TYPE_MASK) != PDO_TYPE_FIXED)
		return;

#ifdef CONFIG_USB_PD_DUAL_ROLE
	if (pdo & PDO_FIXED_DUAL_ROLE)
		hba->flags |= PD_FLAGS_PARTNER_DR_POWER;
	else
		hba->flags &= ~PD_FLAGS_PARTNER_DR_POWER;

	if (pdo & PDO_FIXED_EXTERNAL)
		hba->flags |= PD_FLAGS_PARTNER_EXTPOWER;
	else
		hba->flags &= ~PD_FLAGS_PARTNER_EXTPOWER;
#endif

	if (pdo & PDO_FIXED_DATA_SWAP)
		hba->flags |= PD_FLAGS_PARTNER_DR_DATA;
	else
		hba->flags &= ~PD_FLAGS_PARTNER_DR_DATA;

#ifdef CONFIG_CHARGE_MANAGER
	/*
	 * Treat device as a dedicated charger (meaning we should charge
	 * from it) if it does not support power swap, or if it is externally
	 * powered, or if we are a sink and the device identity matches a
	 * charging white-list.
	 */
	if (!(hba->flags & PD_FLAGS_PARTNER_DR_POWER) ||
	    (hba->flags & PD_FLAGS_PARTNER_EXTPOWER) ||
	    charge_whitelisted)
		charge_manager_update_dualrole(hba, CAP_DEDICATED);
	else
		charge_manager_update_dualrole(hba, CAP_DUALROLE);
#endif
}

void handle_data_request(struct typec_hba *hba, uint16_t head, uint32_t *payload)
{
	int type = PD_HEADER_TYPE(head);
	int cnt = PD_HEADER_CNT(head);

	struct type_mapping {
		uint16_t type;
		char name[32];
	} pd_data_type_mapping[] = {
		{0, "DATA_RESERVED"},
		{PD_DATA_SOURCE_CAP, "DATA_SRC_CAP"},
		{PD_DATA_REQUEST, "DATA_REQ"},
		{PD_DATA_BIST, "DATA_BIST"},
		{PD_DATA_SINK_CAP, "DATA_SNK_CAP"},
		{ 5, " "}, { 6, " "}, { 7, " "}, { 8, " "}, { 9, " "},
		{10, " "}, {11, " "}, {12, " "}, {13, " "}, {14, " "},
		{PD_DATA_VENDOR_DEF, "DATA_VDM"},
	};

	if ((hba->dbg_lvl >= TYPEC_DBG_LVL_2) && (type != PD_DATA_VENDOR_DEF))
		dev_err(hba->dev, "%s %s cnt=%d", __func__, pd_data_type_mapping[type].name, cnt);

	switch (type) {
#ifdef CONFIG_USB_PD_DUAL_ROLE
	case PD_DATA_SOURCE_CAP:
		if ((hba->task_state == PD_STATE_SNK_DISCOVERY)
			|| (hba->task_state == PD_STATE_SNK_TRANSITION)
#ifdef CONFIG_USB_PD_NO_VBUS_DETECT
			|| (hba->task_state ==
			    PD_STATE_SNK_HARD_RESET_RECOVER)
#endif
			|| (hba->task_state == PD_STATE_SNK_READY)) {
			/* Port partner is now known to be PD capable */
			hba->flags |= PD_FLAGS_PREVIOUS_PD_CONN;

			pd_store_src_cap(hba, cnt, payload);
			/* src cap 0 should be fixed PDO */
			pd_update_pdo_flags(hba, payload[0]);

			pd_process_source_cap(hba, pd_src_cap_cnt, pd_src_caps);

			pd_send_request_msg(hba, 1);

			/*dump_pdo(hba, PD_DATA_SOURCE_CAP, cnt, payload);*/
		}
		break;
#endif /* CONFIG_USB_PD_DUAL_ROLE */

	case PD_DATA_REQUEST:
		if ((hba->power_role == PD_ROLE_SOURCE) && (cnt == 1)) {
			if (!pd_check_requested_voltage(hba, payload[0])) {
				if (send_control(hba, PD_CTRL_ACCEPT)) {
					/*
					 * if we fail to send accept, do
					 * nothing and let sink timeout and
					 * send hard reset
					 */
					dev_err(hba->dev, "%s Go to SOFT RESET!", __func__);
					set_state(hba, PD_STATE_SOFT_RESET);
					return;
				}

				/* explicit contract is now in place */
				hba->flags |= PD_FLAGS_EXPLICIT_CONTRACT;
				hba->requested_idx = RDO_POS(payload[0]);
				set_state(hba, PD_STATE_SRC_ACCEPTED);
				return;
			}
		}
		/* the message was incorrect or cannot be satisfied */
		send_control(hba, PD_CTRL_REJECT);
		/* keep last contract in place (whether implicit or explicit) */
		set_state(hba, PD_STATE_SRC_READY);
		break;

	case PD_DATA_BIST:
		/* If not in READY state, then don't start BIST */
		if (DUAL_ROLE_IF_ELSE(hba,
				hba->task_state == PD_STATE_SNK_READY, hba->task_state == PD_STATE_SRC_READY)) {
			/* currently only support sending bist carrier mode 2 */
			if ((payload[0] >> 28) == 5)
				set_state(hba, PD_STATE_BIST_CARRIER_MODE_2);
			else if (payload[0] >> 28 == 8)
				set_state(hba, PD_STATE_BIST_TEST_DATA);
		}
		break;

	case PD_DATA_SINK_CAP:
		hba->flags |= PD_FLAGS_SNK_CAP_RECVD;
		/* snk cap 0 should be fixed PDO */
		pd_update_pdo_flags(hba, payload[0]);

		/*dump_pdo(hba, PD_DATA_SINK_CAP, cnt, payload);*/

		if (hba->task_state == PD_STATE_SRC_GET_SINK_CAP)
			set_state(hba, PD_STATE_SRC_READY);
		break;

	case PD_DATA_VENDOR_DEF:
#ifdef VDM_MODE
#ifdef SUPPORT_SOP_P
		if ((hba->cable_flags & PD_FLAGS_CBL_DISCOVERYING_SOP_P))
			handle_cbl_info(hba, cnt, payload);
		else
#endif
			handle_vdm_request(hba, cnt, payload);
#else
		dev_err(hba->dev, "No support VDM\n");
#endif
		break;

	default:
		dev_err(hba->dev, "Unhandled data message type %d\n", type);
	}
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
void pd_request_power_swap(struct typec_hba *hba)
{
	if (hba->task_state == PD_STATE_SRC_READY)
		set_state(hba, PD_STATE_SRC_SWAP_INIT);
	else if (hba->task_state == PD_STATE_SNK_READY)
		set_state(hba, PD_STATE_SNK_SWAP_INIT);
}

#ifdef CONFIG_USBC_VCONN_SWAP
void pd_request_vconn_swap(struct typec_hba *hba)
{
	if (DUAL_ROLE_IF_ELSE(hba,
		(hba->task_state == PD_STATE_SNK_READY), (hba->task_state == PD_STATE_SRC_READY)))
		set_state(hba, PD_STATE_VCONN_SWAP_SEND);
}
#endif
#endif /* CONFIG_USB_PD_DUAL_ROLE */

void pd_request_data_swap(struct typec_hba *hba)
{
	if (DUAL_ROLE_IF_ELSE(hba, (hba->task_state == PD_STATE_SNK_READY), (hba->task_state == PD_STATE_SRC_READY)))
		set_state(hba, PD_STATE_DR_SWAP);
}

void pd_set_data_role(struct typec_hba *hba, int role)
{
	hba->data_role = role;

	pd_execute_data_swap(hba, role);

	/*Only DFP receives SOP' and SOP''*/
	if (hba->data_role == PD_ROLE_DFP)
		typec_set(hba, (REG_PD_RX_SOP_PRIME_RCV_EN | REG_PD_RX_SOP_DPRIME_RCV_EN), PD_RX_PARAMETER);
	else
		typec_clear(hba, (REG_PD_RX_SOP_PRIME_RCV_EN | REG_PD_RX_SOP_DPRIME_RCV_EN), PD_RX_PARAMETER);

	pd_update_roles(hba);
}

void pd_dr_swap(struct typec_hba *hba)
{
	pd_set_data_role(hba, (hba->data_role == PD_ROLE_DFP ? PD_ROLE_UFP : PD_ROLE_DFP));
	hba->flags |= PD_FLAGS_DATA_SWAPPED;
}

void handle_ctrl_request(struct typec_hba *hba, uint16_t head,
		uint32_t *payload)
{
	int type = PD_HEADER_TYPE(head);
	int ret;

	struct type_mapping {
		uint16_t type;
		char name[32];
	} pd_ctrl_type_mapping[] = {
		{PD_CTRL_RESERVED, "CTRL_RESERVED"},
		{PD_CTRL_GOOD_CRC, "CTRL_GOOD_CRC"},
		{PD_CTRL_GOTO_MIN, "CTRL_GOTO_MIN"},
		{PD_CTRL_ACCEPT, "CTRL_ACCEPT"},
		{PD_CTRL_REJECT, "CTRL_REJECT"},
		{PD_CTRL_PING, "CTRL_PING"},
		{PD_CTRL_PS_RDY, "CTRL_PS_RDY"},
		{PD_CTRL_GET_SOURCE_CAP, "CTRL_GET_SOURCE_CAP"},
		{PD_CTRL_GET_SINK_CAP, "CTRL_GET_SINK_CAP"},
		{PD_CTRL_DR_SWAP, "CTRL_DR_SWAP"},
		{PD_CTRL_PR_SWAP, "CTRL_PR_SWAP"},
		{PD_CTRL_VCONN_SWAP, "CTRL_VCONN_SWAP"},
		{PD_CTRL_WAIT, "CTRL_WAIT"},
		{PD_CTRL_SOFT_RESET, "CTRL_SOFT_RESET"},
	};

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2)
		dev_err(hba->dev, "RX %s\n", pd_ctrl_type_mapping[type].name);

	switch (type) {
	case PD_CTRL_GOOD_CRC:
		/*GCRC is consumed by our controller; should not get it*/
		dev_err(hba->dev, "GOOD_CRC\n");
		break;

	case PD_CTRL_PING:
		/*PING is ONLY from source; should not get it*/
		if (hba->power_role == PD_ROLE_SOURCE) {
			set_state(hba, PD_STATE_SOFT_RESET);
			dev_err(hba->dev, "PING\n");
		}
		break;

	case PD_CTRL_GET_SOURCE_CAP:
		if (hba->task_state == PD_STATE_SNK_REQUESTED)
			set_state(hba, PD_STATE_SOFT_RESET);
		else {
			ret = send_source_cap(hba);
			if (!ret && ((hba->task_state == PD_STATE_SRC_DISCOVERY) ||
				(hba->task_state == PD_STATE_SRC_READY)))
				set_state(hba, PD_STATE_SRC_NEGOTIATE);
		}
		break;

	case PD_CTRL_GET_SINK_CAP:
		if ((hba->task_state == PD_STATE_SNK_READY) ||
			(hba->task_state == PD_STATE_SRC_READY) ||
			(hba->task_state == PD_STATE_SRC_GET_SINK_CAP)) {
			if (send_sink_cap(hba))
				set_state(hba, PD_STATE_SOFT_RESET);
		} else if (hba->task_state == PD_STATE_SNK_TRANSITION)
			set_state(hba, PD_STATE_HARD_RESET_SEND);
		else
			set_state(hba, PD_STATE_SOFT_RESET);
		break;

#ifdef CONFIG_USB_PD_DUAL_ROLE
	case PD_CTRL_GOTO_MIN:
		break;

	case PD_CTRL_PS_RDY:
		if (hba->task_state == PD_STATE_SNK_SWAP_SRC_DISABLE) {
			set_state(hba, PD_STATE_SNK_SWAP_STANDBY);
		} else if (hba->task_state == PD_STATE_SRC_SWAP_STANDBY) {
			/* reset message ID and swap roles */
			pd_set_msg_id(hba, 0);
			pd_set_power_role(hba, PD_ROLE_SINK, 1);
			typec_vbus_det_enable(hba, 1);

			set_state(hba, PD_STATE_SNK_DISCOVERY);
		}
#ifdef CONFIG_USBC_VCONN_SWAP
		else if (hba->task_state == PD_STATE_VCONN_SWAP_INIT) {
			/*
			 * If VCONN is on, then this PS_RDY tells us it's
			 * ok to turn VCONN off
			 */
			if (hba->flags & PD_FLAGS_VCONN_ON)
				set_state(hba, PD_STATE_VCONN_SWAP_READY);
		}
#endif
		else if (hba->task_state == PD_STATE_SNK_DISCOVERY) {
			/* Don't know what power source is ready. Reset. */
			set_state(hba, PD_STATE_HARD_RESET_SEND);
		} else if (hba->task_state == PD_STATE_SNK_SWAP_STANDBY) {
			/* Do nothing, assume this is a redundant PD_RDY */
		} else if (hba->power_role == PD_ROLE_SINK) {
			set_state(hba, PD_STATE_SNK_READY);
#ifdef CONFIG_CHARGE_MANAGER
			/* Set ceiling based on what's negotiated */
			charge_manager_set_ceil(hba,
						CEIL_REQUESTOR_PD,
						hba->curr_limit);
#else
			#if 0
			pd_set_input_current_limit(hba, hba->curr_limit,
						   hba->supply_voltage);
			#endif
#endif
		}
		break;
#endif

	case PD_CTRL_REJECT:
	case PD_CTRL_WAIT:
		if (hba->task_state == PD_STATE_DR_SWAP)
			set_state(hba, READY_RETURN_STATE(hba));
#ifdef CONFIG_USB_PD_DUAL_ROLE
#ifdef CONFIG_USBC_VCONN_SWAP
		else if (hba->task_state == PD_STATE_VCONN_SWAP_SEND)
			set_state(hba, READY_RETURN_STATE(hba));
#endif
		else if (hba->task_state == PD_STATE_SRC_SWAP_INIT)
			set_state(hba, PD_STATE_SRC_READY);
		else if (hba->task_state == PD_STATE_SNK_SWAP_INIT)
			set_state(hba, PD_STATE_SNK_READY);
		else if (hba->task_state == PD_STATE_SNK_REQUESTED)
			/* no explicit contract */
			set_state(hba, PD_STATE_SNK_READY);
#endif
		break;

	case PD_CTRL_ACCEPT:
		if (hba->task_state == PD_STATE_SOFT_RESET) {
			execute_soft_reset(hba);
		} else if (hba->task_state == PD_STATE_DR_SWAP) {
			/* switch data role */
			pd_dr_swap(hba);
			set_state(hba, READY_RETURN_STATE(hba));
		}
#ifdef CONFIG_USB_PD_DUAL_ROLE
#ifdef CONFIG_USBC_VCONN_SWAP
		else if (hba->task_state == PD_STATE_VCONN_SWAP_SEND) {
			/* switch vconn */
			set_state(hba, PD_STATE_VCONN_SWAP_INIT);
		}
#endif
		else if (hba->task_state == PD_STATE_SRC_SWAP_INIT) {
			/* explicit contract goes away for power swap */
			hba->flags &= ~PD_FLAGS_EXPLICIT_CONTRACT;
			set_state(hba, PD_STATE_SRC_SWAP_SNK_DISABLE);
		} else if (hba->task_state == PD_STATE_SNK_SWAP_INIT) {
			/* explicit contract goes away for power swap */
			hba->flags &= ~PD_FLAGS_EXPLICIT_CONTRACT;
			set_state(hba, PD_STATE_SNK_SWAP_SNK_DISABLE);
		}
#endif
		else if (hba->task_state == PD_STATE_SNK_REQUESTED) {
			/* explicit contract is now in place */
			hba->flags |= PD_FLAGS_EXPLICIT_CONTRACT;
			set_state(hba, PD_STATE_SNK_TRANSITION);
		}
		break;

	case PD_CTRL_SOFT_RESET:
		execute_soft_reset(hba);
		/* We are done, acknowledge with an Accept packet */
		send_control(hba, PD_CTRL_ACCEPT);
		break;

	case PD_CTRL_PR_SWAP:
#ifdef CONFIG_USB_PD_DUAL_ROLE
		if (pd_check_power_swap(hba)) {
			dev_err(hba->dev, "power_role=%d ext=%d partner_ext=%d\n",
				hba->power_role,
				pd_src_pdo[hba->requested_idx-1] & PDO_FIXED_EXTERNAL,
				hba->flags & PD_FLAGS_PARTNER_EXTPOWER);

			send_control(hba, PD_CTRL_ACCEPT);

			/*
			 * Clear flag for checking power role to avoid
			 * immediately requesting another swap.
			 */
			hba->flags &= ~PD_FLAGS_CHECK_PR_ROLE;
			set_state(hba,
					DUAL_ROLE_IF_ELSE(hba,
					PD_STATE_SNK_SWAP_SNK_DISABLE,
					PD_STATE_SRC_SWAP_SNK_DISABLE));
		} else {
			send_control(hba, PD_CTRL_REJECT);
		}
#else
		send_control(hba, PD_CTRL_REJECT);
#endif
		break;

	case PD_CTRL_DR_SWAP:
		if (pd_check_data_swap(hba)) {
			/*
			 * Accept switch and perform data swap. Clear
			 * flag for checking data role to avoid
			 * immediately requesting another swap.
			 */
			hba->flags &= ~PD_FLAGS_CHECK_DR_ROLE;
			if (!send_control(hba, PD_CTRL_ACCEPT))
				pd_dr_swap(hba);
		} else {
			send_control(hba, PD_CTRL_REJECT);
		}
		break;

	case PD_CTRL_VCONN_SWAP:
#ifdef CONFIG_USBC_VCONN_SWAP
		if ((hba->task_state == PD_STATE_SRC_READY) ||
			(hba->task_state == PD_STATE_SNK_READY) ||
			(hba->task_state == PD_STATE_SRC_GET_SINK_CAP)) {
			if (pd_check_vconn_swap(hba)) {
				if (!send_control(hba, PD_CTRL_ACCEPT))
					set_state(hba, PD_STATE_VCONN_SWAP_INIT);
			} else {
				send_control(hba, PD_CTRL_REJECT);
			}
		}
#else
		send_control(hba, PD_CTRL_REJECT);
#endif
		break;

	default:
		dev_err(hba->dev, "WARNING: unknown control message\n");
		break;
	}
}

void handle_request(struct typec_hba *hba, uint16_t header,
		uint32_t *payload)
{
	#if 0
	/*
	* If we are in disconnected state, we shouldn't get a request. Do
	* a hard reset if we get one.
	*/
	if (!pd_is_connected(hba))
		set_state(hba, PD_STATE_HARD_RESET_SEND);
	#endif

	if (PD_HEADER_CNT(header))
		handle_data_request(hba, header, payload); /*more than 1 objects*/
	else
		handle_ctrl_request(hba, header, payload); /*no object*/
}

int pd_send_vdm(struct typec_hba *hba, uint32_t vid, int cmd, const uint32_t *data,
		 int count)
{
	if (count > VDO_MAX_SIZE - 1) {
		dev_err(hba->dev, "VDM over max size\n");
		return -1;
	}

	if (hba->vdm_state != VDM_STATE_DONE) {
		dev_err(hba->dev, "PD is busy!\n");
		return -1;
	}

	/* set VDM header with VID & CMD */
	hba->vdo_data[0] = VDO(vid,
		((vid == USB_SID_PD) || (vid == USB_SID_DISPLAYPORT) ? 1 : 0), cmd);
	queue_vdm(hba, hba->vdo_data, data, count);
	return 0;
}

inline int pdo_busy(struct typec_hba *hba)
{
	/*
	 * Note, main PDO state machine (pd_task) uses READY state exclusively
	 * to denote port partners have successfully negociated a contract.  All
	 * other protocol actions force state transitions.
	 */
	int rv = (hba->task_state != PD_STATE_SRC_READY);
#ifdef CONFIG_USB_PD_DUAL_ROLE
	rv &= (hba->task_state != PD_STATE_SNK_READY);
#endif
	return rv;
}

uint64_t vdm_get_ready_timeout(uint32_t vdm_hdr)
{
	uint64_t timeout;
	int cmd = PD_VDO_CMD(vdm_hdr);

	/* its not a structured VDM command */
	if (!PD_VDO_SVDM(vdm_hdr))
		return 500;

	switch (PD_VDO_CMDT(vdm_hdr)) {
	case CMDT_INIT:
		if ((cmd == CMD_ENTER_MODE) || (cmd == CMD_EXIT_MODE))
			timeout = PD_T_VDM_WAIT_MODE_E;
		else
			timeout = PD_T_VDM_SNDR_RSP;
		break;
	default:
		if ((cmd == CMD_ENTER_MODE) || (cmd == CMD_EXIT_MODE))
			timeout = PD_T_VDM_E_MODE;
		else
			timeout = PD_T_VDM_RCVR_RSP;
		break;
	}
	return timeout;
}

static void pd_vdm_send_state_machine(struct typec_hba *hba)
{
	int res;
	uint16_t header;

	if ((hba->dbg_lvl >= TYPEC_DBG_LVL_3) && (hba->vdm_state != VDM_STATE_DONE))
		dev_err(hba->dev, "%s vdm_state=%x", __func__, hba->vdm_state);

	switch (hba->vdm_state) {
	case VDM_STATE_READY:
		/* Only transmit VDM if connected. */
		if (!pd_is_connected(hba)) {
			hba->vdm_state = VDM_STATE_DONE;
			break;
		}

		/*
		 * if there's traffic or we're not in PDO ready state don't send
		 * a VDM.
		 */
		if (pdo_busy(hba))
			break;

		if (hba->vdm_state == VDM_STATE_READY)
			if ((hba->pd_is0 & PD_RX_SUCCESS0) || (typec_read8(hba, PD_INTR_0+1) & 0x10)) {
				dev_err(hba->dev, "New MSG is coming in. Skip this MSG.\n");
				hba->vdm_state = VDM_STATE_DONE;
				return;
			}

		/* Prepare and send VDM */
		header = PD_HEADER(PD_DATA_VENDOR_DEF, hba->power_role,
				   hba->data_role, (int)hba->vdo_count);
		res = pd_transmit(hba, PD_TX_SOP, header,
				  hba->vdo_data);
		if (res != 0) {
			hba->vdm_state = VDM_STATE_ERR_SEND;
			set_state(hba, PD_STATE_SOFT_RESET);
		} else {
			if (PD_VDO_CMDT(hba->vdo_data[0]) == CMDT_INIT) {
				hba->vdm_state = VDM_STATE_BUSY;
				hba->vdm_timeout = jiffies +
					msecs_to_jiffies(vdm_get_ready_timeout(hba->vdo_data[0]));
			} else
				hba->vdm_state = VDM_STATE_DONE;
		}
		break;
	case VDM_STATE_WAIT_RSP_BUSY:
		/* wait and then initiate request again */
		if (time_after(jiffies, hba->vdm_timeout)) {
			hba->vdo_data[0] = hba->vdo_retry;
			hba->vdo_count = 1;
			hba->vdm_state = VDM_STATE_READY;
		}
		break;
	case VDM_STATE_BUSY:
		/* Wait for VDM response or timeout */
		if (hba->vdm_timeout &&
			(time_after(jiffies, hba->vdm_timeout))) {
			hba->vdm_state = VDM_STATE_ERR_TMOUT;
		}
		break;
	default:
		break;
	}
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
int pd_is_power_swapping(struct typec_hba *hba)
{
	/* return true if in the act of swapping power roles */
	return (hba->task_state == PD_STATE_SNK_SWAP_SNK_DISABLE ||
		hba->task_state == PD_STATE_SNK_SWAP_SRC_DISABLE ||
		hba->task_state == PD_STATE_SNK_SWAP_STANDBY ||
		hba->task_state == PD_STATE_SNK_SWAP_COMPLETE ||
		hba->task_state == PD_STATE_SRC_SWAP_SNK_DISABLE ||
		hba->task_state == PD_STATE_SRC_SWAP_SRC_DISABLE ||
		hba->task_state == PD_STATE_SRC_SWAP_STANDBY);
}
#endif /* CONFIG_USB_PD_DUAL_ROLE */

void pd_ping_enable(struct typec_hba *hba, int enable)
{
	if (enable)
		hba->flags |= PD_FLAGS_PING_ENABLED;
	else
		hba->flags &= ~PD_FLAGS_PING_ENABLED;
}

void pd_get_message(struct typec_hba *hba, uint16_t *header, uint32_t *payload)
{
	int i;
	uint8_t cnt;

	/*header*/
	*header = typec_readw(hba, PD_RX_HEADER);

	/*data*/
	cnt = PD_HEADER_CNT(*header);
	for (i = 0; i < cnt; i++) {
#ifdef BURST_READ
		payload[i] = typec_readdw(hba, (PD_RX_DATA_OBJECT0_0 + i*4));
#else
		payload[i] = (typec_readw(hba, (PD_RX_DATA_OBJECT0_1+i*4)) << 16);
		payload[i] |= typec_readw(hba, (PD_RX_DATA_OBJECT0_0+i*4));
#endif
	}

	#if PD_SW_WORKAROUND1_2
	mutex_lock(&hba->typec_lock);
	typec_writew(hba, PD_RX_RCV_MSG_INTR, PD_INTR_0);
	typec_set(hba, REG_PD_RX_RCV_MSG_INTR_EN, PD_INTR_EN_0);
	mutex_unlock(&hba->typec_lock);
	#endif
}

#ifdef CONFIG_DUAL_ROLE_USB_INTF
int need_update(struct typec_hba *hba)
{
	/*Is Power Role Different?*/
	if (((hba->power_role == PD_ROLE_SOURCE) && (hba->dual_role_pr != DUAL_ROLE_PROP_PR_SRC)) ||
		((hba->power_role == PD_ROLE_SINK) && (hba->dual_role_pr != DUAL_ROLE_PROP_PR_SNK)) ||
		((hba->power_role == PD_NO_ROLE) && (hba->dual_role_pr != DUAL_ROLE_PROP_PR_NONE)))
		return 1;

	/*Is Data Role Different?*/
	if (((hba->data_role == PD_ROLE_DFP) && (hba->dual_role_dr != DUAL_ROLE_PROP_DR_HOST)) ||
		((hba->data_role == PD_ROLE_UFP) && (hba->dual_role_dr != DUAL_ROLE_PROP_DR_DEVICE)) ||
		((hba->data_role == PD_NO_ROLE) && (hba->dual_role_dr != DUAL_ROLE_PROP_MODE_NONE)))
		return 1;

	return 0;
}

void update_dual_role_usb(struct typec_hba *hba, int is_on)
{
	if (!is_on) {
		hba->dual_role_pr = DUAL_ROLE_PROP_PR_NONE;
		hba->dual_role_dr = DUAL_ROLE_PROP_DR_NONE;
		hba->dual_role_mode = DUAL_ROLE_PROP_MODE_NONE;
		hba->dual_role_vconn = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
		goto end;
	}

	if (hba->power_role == PD_ROLE_SOURCE)
		hba->dual_role_pr = DUAL_ROLE_PROP_PR_SRC;
	else
		hba->dual_role_pr = DUAL_ROLE_PROP_PR_SNK;

	if (hba->data_role == PD_ROLE_DFP) {
		hba->dual_role_mode = DUAL_ROLE_PROP_MODE_DFP;
		hba->dual_role_dr = DUAL_ROLE_PROP_DR_HOST;
	} else {
		hba->dual_role_mode = DUAL_ROLE_PROP_MODE_UFP;
		hba->dual_role_dr = DUAL_ROLE_PROP_DR_DEVICE;
	}

	if (hba->flags & PD_FLAGS_VCONN_ON)
		hba->dual_role_vconn = DUAL_ROLE_PROP_VCONN_SUPPLY_YES;
	else
		hba->dual_role_vconn = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;

end:
	dual_role_instance_changed(hba->dr_usb);
}
#endif

int pd_kpoc_task(void *data)
{
#if PD_SW_WORKAROUND1_2
	uint16_t header;
	uint32_t payload[7];
#endif
	unsigned long pre_timeout = ULONG_MAX;
	unsigned long timeout = 10;

	int ret = 0;
	int incoming_packet = 0;
	int hard_reset_count = 0;
#ifdef CONFIG_USB_PD_DUAL_ROLE
	/*uint64_t next_role_swap = PD_T_DRP_SNK;*/
#ifndef CONFIG_USB_PD_NO_VBUS_DETECT
	int snk_hard_reset_vbus_off = 0;
#endif
#endif /* CONFIG_USB_PD_DUAL_ROLE */
	enum pd_states curr_state;
	int hard_reset_sent = 0;
	struct typec_hba *hba = (struct typec_hba *)data;
	uint16_t pd_is0, pd_is1, cc_is0;
	uint8_t missing_event;

	static const struct sched_param param = {
			.sched_priority = 98,
	};
	sched_setscheduler(current, SCHED_FIFO, &param);

	/* Initialize PD protocol state variables for each port. */
	hba->power_role = PD_NO_ROLE;
	hba->data_role = PD_NO_ROLE;
	hba->flags = 0;
	hba->vdm_state = VDM_STATE_DONE;
	hba->alt_mode_svid = 0;
	hba->timeout_user = 0;
	set_state(hba, PD_STATE_DISABLED);

	msleep(hba->kpoc_delay);

	typec_enable(hba, 1);

	while (1) {
		/* process VDM messages last */
		pd_vdm_send_state_machine(hba);
		if ((hba->vdm_state == VDM_STATE_WAIT_RSP_BUSY) || (hba->vdm_state == VDM_STATE_BUSY)) {
			/*VDM timer is running. So kick the FSM quicklier to check timeout*/
			pre_timeout = timeout;
			timeout = 5;
		} else {
			if (pre_timeout != ULONG_MAX) {
				timeout = pre_timeout;
				pre_timeout = ULONG_MAX;
			}
		}

		/*
		 * RX & CC events may happen
		 *  1) in the middle of init_completion
		 *  2) after their event handlers finished last time
		 * if this is case, jump directly into event handlers; otherwise,
		 * wait for timeout, or CC/RX/timer events
		 */
		mutex_lock(&hba->typec_lock);
		missing_event = (hba->pd_is0 & PD_RX_SUCCESS0) || (hba->pd_is1 & PD_RX_HR_SUCCESS) || (hba->cc_is0);
		mutex_unlock(&hba->typec_lock);

		if (!missing_event && timeout) {
			hba->rx_event = false;
			ret = wait_event_timeout(hba->wq, hba->rx_event, msecs_to_jiffies(timeout));
		}

		/* latch events */
		mutex_lock(&hba->typec_lock);
		pd_is0 = hba->pd_is0;
		pd_is1 = hba->pd_is1;
		cc_is0 = hba->cc_is0;

		pd_clear_event(hba, (pd_is0 & PD_EVENTS0), (pd_is1 & PD_EVENTS1), cc_is0);
		mutex_unlock(&hba->typec_lock);

		if ((hba->dbg_lvl >= TYPEC_DBG_LVL_3) && !ret)
			dev_err(hba->dev, "[TIMEOUT %lums]\n", timeout);

		/* Directly go to timeout_state when time is up set by set_state_timeout() */
		if (hba->flags & PD_FLAGS_TIMEOUT) {
			if (hba->timeout_state != PD_STATE_NO_TIMEOUT)
				set_state(hba, hba->timeout_state);
			hba->flags = hba->flags & (~PD_FLAGS_TIMEOUT);
			dev_err(hba->dev, "timeout go to %s\n", pd_state_mapping[hba->task_state].name);
		}

		/* process RX events */
		if (pd_is0 & PD_RX_SUCCESS0) {
		#ifdef PROFILING
			ktime_t start, end;
		#endif

			incoming_packet = 1;

		#if PD_SW_WORKAROUND1_1

			if (hba->header && (hba->bist_mode != BDO_MODE_TEST_DATA))
				handle_request(hba, hba->header, hba->payload);

		#else

		#ifdef PROFILING
			start = ktime_get();
		#endif

			pd_get_message(hba, &header, payload);

		#ifdef PROFILING
			end = ktime_get();
			dev_err(hba->dev, "RX took %lld us", ktime_to_ns(ktime_sub(end, start))/1000);
		#endif

			if (header && (hba->bist_mode != BDO_MODE_TEST_DATA))
				handle_request(hba, header, payload);
		#endif
		} else {
			incoming_packet = 0;
		}

		/* process hard reset */
		if (pd_is1 & PD_RX_HR_SUCCESS) {
			dev_err(hba->dev, "PD_RX_HR_SUCCESS\n");

			pd_complete_hr(hba);

			if (hba->task_state != PD_STATE_SRC_HARD_RESET_RECOVER) {
				hba->last_state = hba->task_state;
				set_state(hba, PD_STATE_HARD_RESET_RECEIVED);
			}
		}

		/* process CC events */
		/* change state according to Type-C controller status */
		if ((cc_is0 & TYPE_C_CC_ENT_ATTACH_SNK_INTR) && !pd_is_power_swapping(hba))
			set_state(hba, PD_STATE_SNK_ATTACH);

		if (cc_is0 & TYPE_C_CC_ENT_DISABLE_INTR)  {
			dev_err(hba->dev, "TYPE_C_CC_ENT_DISABLE_INTR\n");
			set_state(hba, PD_STATE_DISABLED);
		}

		if (cc_is0 & TYPE_C_CC_ENT_UNATTACH_SNK_INTR)
			set_state(hba, PD_STATE_SNK_UNATTACH);

		/*There is a VDM have to be sent.*/
		if (hba->vdm_state == VDM_STATE_READY)
			continue;

		curr_state = hba->task_state;

		/* if nothing to do, verify the state of the world in 500ms
		 *  also reset the timeout value set by the last state.
		 */
		timeout = (hba->timeout_user ? hba->timeout_user : PD_T_NO_ACTIVITY);

		/* skip DRP toggle cases*/
		if ((hba->dbg_lvl >= TYPEC_DBG_LVL_1)
			&& state_changed(hba)
			&& !(hba->last_state == PD_STATE_SRC_UNATTACH && hba->task_state == PD_STATE_SNK_UNATTACH)
			&& !(hba->last_state == PD_STATE_SNK_UNATTACH && hba->task_state == PD_STATE_SRC_UNATTACH)) {
			dev_err(hba->dev, "%s->%s\n",
				pd_state_mapping[hba->last_state].name, pd_state_mapping[hba->task_state].name);
		}

		switch (curr_state) {
		case PD_STATE_DISABLED:
#ifdef CONFIG_USB_PD_DUAL_ROLE
		case PD_STATE_SNK_UNATTACH:
			/* Clear the input current limit */
			/* pd_set_input_current_limit(hba, 0, 0);*/
#ifdef CONFIG_USBC_VCONN
			typec_drive_vconn(hba, 0);
#endif
#endif

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
			pd_dfp_exit_mode(hba, 0, 0);
#endif
			/* be aware of Vbus */
			typec_vbus_det_enable(hba, 1);

			/* disable RX */
			pd_rx_enable(hba, 0);

			/* disable BIST */
			pd_bist_enable(hba, 0, 0);

			/*TX header PD spec revision should be 01b=2.0*/
			typec_writew_msk(hba, 0x3<<6, PD_REV20<<6, PD_TX_HEADER);

			/*Restore back to real Rp*/
			typec_select_rp(hba, hba->rp_val);

			typec_set(hba, REG_PD_RX_DUPLICATE_INTR_EN, PD_INTR_EN_0);

			/*Improve RX signal quality from analog to digital part.*/
			if (state_changed(hba))
				pd_rx_phya_setting(hba);

			if (wake_lock_active(&hba->typec_wakelock))
				wake_unlock(&hba->typec_wakelock);

			typec_enable_lowq(hba, "PD-UNATTACHED");

			/*Disable 26MHz clock XO_PD*/
			clk_buf_ctrl(CLK_BUF_CHG, false);

			hba->power_role = PD_NO_ROLE;
			hba->data_role = PD_NO_ROLE;
			hba->vdm_state = VDM_STATE_DONE;
			hba->cable_flags = PD_FLAGS_CBL_NO_INFO;
			hba->vsafe_5v = PD_VSAFE5V_LOW;

			cancel_delayed_work_sync(&hba->usb_work);
			schedule_delayed_work(&hba->usb_work, 0);

			if ((hba->charger_det_notify) && (hba->last_state != PD_STATE_DISABLED)) {
				if ((hba->flags & PD_FLAGS_SRC_NO_PD) ||
					(hba->flags & PD_FLAGS_EXPLICIT_CONTRACT) ||
					((hba->task_state == PD_STATE_SNK_ATTACH) &&
					 (hba->timeout_state == PD_STATE_SNK_KPOC_PWR_OFF))) {
					hba->charger_det_notify(0);
				} else {
					set_state_timeout(hba, 2*1000, PD_STATE_SNK_KPOC_PWR_OFF);
				}
			}

			hba->flags &= ~PD_FLAGS_RESET_ON_DISCONNECT_MASK;

			/* When enter DISABLE/UNATTACHED.SRC/UNATTACHED.SNK, do _NOT_ check regularly of
			 * PD status. If there is any status change. CC interrupt will kick PD FSM and
			 * set a new timeout value to start polling check.
			 */
			timeout = ULONG_MAX;
			break;

		case PD_STATE_SNK_HARD_RESET_RECOVER:
			if (state_changed(hba))
				hba->flags |= PD_FLAGS_DATA_SWAPPED;

			/* Wait for VBUS to go low and then high*/
			if (state_changed(hba)) {
				snk_hard_reset_vbus_off = 0;
				set_state_timeout(hba, PD_T_SAFE_0V,
					(hard_reset_count < PD_HARD_RESET_COUNT) ?
					PD_STATE_HARD_RESET_SEND : PD_STATE_SNK_DISCOVERY);
			}

			timeout = 20;
			if (!snk_hard_reset_vbus_off && (typec_vbus(hba) < PD_VSAFE0V_HIGH)) {
				/* VBUS has gone low, reset timeout */
				snk_hard_reset_vbus_off = 1;
				set_state_timeout(hba, (PD_T_SRC_RECOVER_MAX + PD_T_SRC_TURN_ON),
						  PD_STATE_SNK_HARD_RESET_NOVSAFE5V);
			}

			if (snk_hard_reset_vbus_off && (typec_vbus(hba) > PD_VSAFE5V_LOW)) {
				/* VBUS went high again */
				set_state(hba, PD_STATE_SNK_DISCOVERY);
				timeout = 10;

				pd_complete_hr(hba);
				typec_vbus_det_enable(hba, 1);

				pd_bist_enable(hba, 0, 0);
			}
			break;

		case PD_STATE_SNK_HARD_RESET_NOVSAFE5V:
			/*
			 * Currently as SNK and sent the hard reset
			 * The partner as SRC turn off VBUS, but does not turn on VBUS.
			 * After tSrcRecover + tSrcTurnOn (1275ms), should just jump to unattached state
			 */
			if (state_changed(hba)) {
				if ((typec_read8(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST)
									== TYPEC_STATE_ATTACHED_SNK) {
					/* disable RX */
					pd_rx_enable(hba, 0);

					/* notify IP VBUS is gone.*/
					typec_vbus_present(hba, 0);
				}
			}
			break;

		case PD_STATE_SNK_ATTACH:
			if (!state_changed(hba))
				break;

			/*Enable 26MHz clock XO_PD*/
			clk_buf_ctrl(CLK_BUF_CHG, true);

			if (hba->charger_det_notify)
				hba->charger_det_notify(1);

			if (!wake_lock_active(&hba->typec_wakelock))
				wake_lock(&hba->typec_wakelock);

			/* reset message ID  on connection */
			pd_set_msg_id(hba, 0);
			/* initial data role for sink is UFP */
			pd_set_power_role(hba, PD_ROLE_SINK, 1);
			if (!(hba->flags & PD_FLAGS_POWER_SWAPPED)) /*don't update DR after SRC->SNK power role swap*/
				pd_set_data_role(hba, PD_ROLE_UFP);
			else
				hba->flags &= ~PD_FLAGS_POWER_SWAPPED; /*PR SWAP path 1*/

			pd_rx_enable(hba, 1);

			/*Improve RX signal quality from analog to digital part.*/
			pd_rx_phya_setting(hba);

			/*
			 * fake set data role swapped flag so we send
			 * discover identity when we enter SRC_READY
			 */
			hba->flags |= (PD_FLAGS_CHECK_PR_ROLE | PD_FLAGS_DATA_SWAPPED);
			hba->cable_flags = PD_FLAGS_CBL_NO_INFO;
			hard_reset_count = 0;

			timeout = 0;
			set_state(hba, PD_STATE_SNK_DISCOVERY);

			break;

		case PD_STATE_SNK_DISCOVERY:
			/* Wait for source cap expired only if we are enabled */
			if (state_changed(hba)) {
				hba->flags &= ~PD_FLAGS_POWER_SWAPPED; /*PR SWAP path 2*/

				/*
				 * If we haven't passed hard reset counter,
				 * start SinkWaitCapTimer, otherwise start
				 * NoResponseTimer.
				 */
				if (hard_reset_count < PD_HARD_RESET_COUNT)
					set_state_timeout(hba, 2*1000, PD_STATE_HARD_RESET_SEND);
				else if (hba->flags & PD_FLAGS_PREVIOUS_PD_CONN)
					/* ErrorRecovery */
					set_state_timeout(hba, PD_T_NO_RESPONSE, PD_STATE_SNK_UNATTACH);
				else if (hba->last_state == PD_STATE_SNK_HARD_RESET_RECOVER &&
						hard_reset_count == PD_HARD_RESET_COUNT) {

					hba->flags |= PD_FLAGS_SRC_NO_PD;
					/*
					 * Once the Hard Reset has been retried nHardResetCount times
					 * then it shall be assumed that the remote device is
					 * non-responsive.
					 */
					dev_err(hba->dev, "SRC NO SUPPORT PD");
					typec_vbus_det_enable(hba, 1);
					timeout = PD_T_NO_ACTIVITY;

					if (wake_lock_active(&hba->typec_wakelock))
						wake_unlock(&hba->typec_wakelock);
				}
			}

			if (hba->flags & PD_FLAGS_SRC_NO_PD) {
				int val = typec_vbus(hba);

				if (val < hba->vsafe_5v) {
					/* notify IP VBUS is gone.*/
					typec_vbus_present(hba, 0);
					dev_err(hba->dev, "Vbus OFF in Attached.SNK\n");
				}

			}
			break;

		case PD_STATE_SNK_REQUESTED:
			/* Wait for ACCEPT or REJECT */
			if (state_changed(hba)) {
				hard_reset_count = 0;
				/*set_state_timeout(hba, PD_T_SENDER_RESPONSE-3, PD_STATE_HARD_RESET_SEND);*/
				set_state_timeout(hba, 1*1000, PD_STATE_HARD_RESET_SEND);
			}
			break;

		case PD_STATE_SNK_TRANSITION:
			/* Wait for PS_RDY */
			if (state_changed(hba))
				/*set_state_timeout(hba, PD_T_PS_TRANSITION, PD_STATE_HARD_RESET_SEND);*/
				set_state_timeout(hba, 1*1000, PD_STATE_HARD_RESET_SEND);
			break;

		case PD_STATE_SNK_READY:
			timeout = 20;

			/*
			 * Don't send any PD traffic if we woke up due to
			 * incoming packet or if VDO response pending to avoid
			 * collisions.
			 */
			if (incoming_packet || (hba->vdm_state == VDM_STATE_BUSY))
				break;

			/* Check for new power to request */
			if (hba->new_power_request) {
				pd_send_request_msg(hba, 0);
				break;
			}

#ifdef SUPPORT_SOP_P
			if (hba->data_role == PD_ROLE_DFP &&
				hba->cable_flags == PD_FLAGS_CBL_NO_INFO) {
				set_state(hba, PD_STATE_DISCOVERY_SOP_P);

				/*Kick FSM to enter the next stat earlier.*/
				timeout = 10;
				break;
			}
#endif

			/* If DFP, send discovery SVDMs */
			if ((hba->discover_vmd == 1) &&
			     (hba->data_role == PD_ROLE_DFP) &&
			     (hba->flags & PD_FLAGS_DATA_SWAPPED)) {

				pd_send_vdm(hba, USB_SID_PD,
					    CMD_DISCOVER_IDENT, NULL, 0);

				hba->flags &= ~PD_FLAGS_DATA_SWAPPED;
				break;
			}

			/* Sent all messages, don't need to wake very often */
			timeout = PD_T_NO_ACTIVITY;
			break;

#ifdef CONFIG_USBC_VCONN_SWAP
		case PD_STATE_VCONN_SWAP_SEND:
			if (state_changed(hba)) {
				if (send_control(hba, PD_CTRL_VCONN_SWAP)) {
					timeout = 10;
					set_state(hba, PD_STATE_SOFT_RESET);
					break;
				}

				/* Wait for accept or reject */
				set_state_timeout(hba, PD_T_SENDER_RESPONSE, READY_RETURN_STATE(hba));
			}
			break;

		case PD_STATE_VCONN_SWAP_INIT:
			if (state_changed(hba)) {
				if (!(hba->flags & PD_FLAGS_VCONN_ON)) {
					/* Turn VCONN on and wait for it */
					typec_drive_vconn(hba, 1);
					set_state_timeout(hba, PD_VCONN_SWAP_DELAY, PD_STATE_VCONN_SWAP_READY);
				} else {
					set_state_timeout(hba,
						PD_T_VCONN_SOURCE_ON, READY_RETURN_STATE(hba));
				}
			}
			break;

		case PD_STATE_VCONN_SWAP_READY:
			if (state_changed(hba)) {
				if (!(hba->flags & PD_FLAGS_VCONN_ON)) {
					/* VCONN is now on, send PS_RDY */
					hba->flags |= PD_FLAGS_VCONN_ON;

					if (send_control(hba, PD_CTRL_PS_RDY)) {
						timeout = 10;

						/*
						 * If failed to get goodCRC,
						 * send soft reset
						 */
						set_state(hba, PD_STATE_SOFT_RESET);

						break;
					}

					set_state(hba, READY_RETURN_STATE(hba));
				} else {
					/* Turn VCONN off and wait for it */
					typec_drive_vconn(hba, 0);
					hba->flags &= ~PD_FLAGS_VCONN_ON;

					set_state_timeout(hba, PD_VCONN_SWAP_DELAY, READY_RETURN_STATE(hba));
				}
			}
			break;
#endif /* CONFIG_USBC_VCONN_SWAP */

		case PD_STATE_SOFT_RESET:
			if (state_changed(hba)) {
				dev_err(hba->dev, "No SOFT RESET @ KPOC");
				set_state(hba, PD_STATE_SNK_READY);
			}
			break;

		case PD_STATE_HARD_RESET_SEND:
			if (state_changed(hba))
				hard_reset_sent = 0;

			/*
			 * If Hard Reset happened in the middle of doing PR_SWAP
			 * from SRC to SNK. The driver should check VBUS. If there
			 * is no VBUS, jump to UNATTACHED mode.
			 */
			if (hba->last_state == PD_STATE_SRC_SWAP_STANDBY)
				typec_vbus_det_enable(hba, 1);

			/* try sending hard reset until it succeeds */
			if (!hard_reset_sent) {
				hard_reset_count++;

				if (pd_transmit(hba, PD_TX_HARD_RESET, 0, NULL)) {
					timeout = 10;

					break;
				}

				/* successfully sent hard reset */
				hard_reset_sent = 1;

				/*
				 * If we are source, delay before cutting power
				 * to allow sink time to get hard reset.
				 */
				if (hba->power_role == PD_ROLE_SOURCE) {
					/*
					 * TD.PD.SRC.E6 PSHardResetTimer Timeout - Testing Downstream Port
					 * PSHardResetTimer is too long (actual 35.1 ms)
					 * Our original tPSHardReset is 25ms. But the exact time is over 35ms.
					 * So just cut the value a little bit.
					 */
					set_state_timeout(hba,
						PD_T_PS_HARD_RESET-2, PD_STATE_HARD_RESET_EXECUTE);
				} else {
					timeout = 0;
					/*Do not detect VBUS when receiving HARD RESET ASAP*/
					typec_vbus_det_enable(hba, 0);
					set_state(hba,
						PD_STATE_HARD_RESET_EXECUTE);
				}
			}
			break;
		case PD_STATE_HARD_RESET_RECEIVED:
			if (state_changed(hba)) {
				if (hba->power_role == PD_ROLE_SOURCE) {
					set_state_timeout(hba,
						PD_T_PS_HARD_RESET-2, PD_STATE_HARD_RESET_EXECUTE);
				} else {
					timeout = 0;
					/*Do not detect VBUS when receiving HARD RESET ASAP*/
					typec_vbus_det_enable(hba, 0);
					set_state(hba,
						PD_STATE_HARD_RESET_EXECUTE);
				}
			}
			break;
		case PD_STATE_HARD_RESET_EXECUTE:
			/* reset our own state machine */
			pd_execute_hard_reset(hba);
			timeout = 0;
			break;

#ifdef SUPPORT_SOP_P
		case PD_STATE_DISCOVERY_SOP_P:
			if (state_changed(hba)) {
				hba->cable_flags &= ~PD_FLAGS_CBL_NO_INFO;

				if (!send_cable_discovery_identity(hba, PD_TX_SOP_PRIME)) {
					hba->cable_flags |= PD_FLAGS_CBL_DISCOVERYING_SOP_P;

					dev_err(hba->dev, "%s Send SOP_P discovery identity success", __func__);

					timeout = PD_T_VDM_SNDR_RSP;

				} else {
					hba->cable_flags &= ~PD_FLAGS_CBL_DISCOVERYING_SOP_P;
					hba->cable_flags |= PD_FLAGS_CBL_NO_RESP_SOP_P;
					dev_err(hba->dev, "%s Send SOP_P discovery identity fail", __func__);
					timeout = 10;
				}

				if (hba->flags & PD_FLAGS_EXPLICIT_CONTRACT)
					set_state(hba, DUAL_ROLE_IF_ELSE(hba, PD_STATE_SNK_READY, PD_STATE_SRC_READY));
				else
					set_state(hba, PD_STATE_SRC_DISCOVERY);
			}
			break;
#endif

		case PD_STATE_SNK_KPOC_PWR_OFF:
			if (state_changed(hba)) {
				if (hba->charger_det_notify)
					hba->charger_det_notify(0);
			}
			break;
		default:
			break;
		}

		hba->last_state = curr_state;
	}
}

int pd_task(void *data)
{
#if PD_SW_WORKAROUND1_2
	uint16_t header;
	uint32_t payload[7];
#endif
	unsigned long pre_timeout = ULONG_MAX;
	unsigned long timeout = 10;

	int ret = 0;
	int incoming_packet = 0;
	int hard_reset_count = 0;
#ifdef CONFIG_USB_PD_DUAL_ROLE
	/*uint64_t next_role_swap = PD_T_DRP_SNK;*/
#ifndef CONFIG_USB_PD_NO_VBUS_DETECT
	int snk_hard_reset_vbus_off = 0;
#endif
#ifdef CONFIG_CHARGE_MANAGER
	int typec_curr = 0, typec_curr_change = 0;
#endif /* CONFIG_CHARGE_MANAGER */
#endif /* CONFIG_USB_PD_DUAL_ROLE */
	enum pd_states curr_state;
	int caps_count = 0, hard_reset_sent = 0;
	int snk_cap_count = 0;
	struct typec_hba *hba = (struct typec_hba *)data;
	uint16_t pd_is0, pd_is1, cc_is0;
	uint8_t missing_event;

	static const struct sched_param param = {
			.sched_priority = 1,
	};
	sched_setscheduler(current, SCHED_FIFO, &param);

	/* Disable RX until connection is established */
	pd_rx_enable(hba, 0);

	/* Ensure the power supply is in the default state */
	/*pd_power_supply_reset(hba);*/

	/* Initialize PD protocol state variables for each port. */
	hba->power_role = PD_NO_ROLE;
	hba->data_role = PD_NO_ROLE;
	hba->flags = 0;
	hba->vdm_state = VDM_STATE_DONE;
	hba->alt_mode_svid = 0;
	hba->timeout_user = 0;
	set_state(hba, PD_STATE_DISABLED);

	/* Initialize completion for stress test */
#if RESET_STRESS_TEST
	init_completion(&hba->ready);
#endif

	#if 0
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	/* Initialize PD Policy engine */
	pd_dfp_pe_init(hba);
#endif

#ifdef CONFIG_CHARGE_MANAGER
	/* Initialize PD and type-C supplier current limits to 0 */
	pd_set_input_current_limit(hba, 0, 0);
	typec_set_input_current_limit(hba, 0, 0);
	charge_manager_update_dualrole(hba, CAP_UNKNOWN);
#endif
	#endif

	while (1) {
		/* process VDM messages last */
		pd_vdm_send_state_machine(hba);
		if ((hba->vdm_state == VDM_STATE_WAIT_RSP_BUSY) || (hba->vdm_state == VDM_STATE_BUSY)) {
			/*VDM timer is running. So kick the FSM quicklier to check timeout*/
			pre_timeout = timeout;
			timeout = 5;
		} else {
			if (pre_timeout != ULONG_MAX) {
				timeout = pre_timeout;
				pre_timeout = ULONG_MAX;
			}
		}

		/*
		 * RX & CC events may happen
		 *  1) in the middle of init_completion
		 *  2) after their event handlers finished last time
		 * if this is case, jump directly into event handlers; otherwise,
		 * wait for timeout, or CC/RX/timer events
		 */
		mutex_lock(&hba->typec_lock);
		missing_event = (hba->pd_is0 & PD_RX_SUCCESS0) || (hba->pd_is1 & PD_RX_HR_SUCCESS) || (hba->cc_is0);
		mutex_unlock(&hba->typec_lock);

		if (!missing_event && timeout) {
			hba->rx_event = false;
			ret = wait_event_timeout(hba->wq, hba->rx_event, msecs_to_jiffies(timeout));
		}

		if (hba->is_shutdown)
			break;

		/* latch events */
		mutex_lock(&hba->typec_lock);
		pd_is0 = hba->pd_is0;
		pd_is1 = hba->pd_is1;
		cc_is0 = hba->cc_is0;

		pd_clear_event(hba, (pd_is0 & PD_EVENTS0), (pd_is1 & PD_EVENTS1), cc_is0);
		mutex_unlock(&hba->typec_lock);

		if ((hba->dbg_lvl >= TYPEC_DBG_LVL_3) && !ret)
			dev_err(hba->dev, "[TIMEOUT %lums]\n", timeout);

		/* Directly go to timeout_state when time is up set by set_state_timeout() */
		if (hba->flags & PD_FLAGS_TIMEOUT) {
			if (hba->timeout_state != PD_STATE_NO_TIMEOUT)
				set_state(hba, hba->timeout_state);
			hba->flags = hba->flags & (~PD_FLAGS_TIMEOUT);
			dev_err(hba->dev, "timeout go to %s\n", pd_state_mapping[hba->task_state].name);
		}

		/* process RX events */
		if (pd_is0 & PD_RX_SUCCESS0) {
			#ifdef PROFILING
			ktime_t start, end;
			#endif

			incoming_packet = 1;

			#if PD_SW_WORKAROUND1_1

			if (hba->header && (hba->bist_mode != BDO_MODE_TEST_DATA))
				handle_request(hba, hba->header, hba->payload);

			#else

			#ifdef PROFILING
			start = ktime_get();
			#endif

			pd_get_message(hba, &header, payload);

			#ifdef PROFILING
			end = ktime_get();
			dev_err(hba->dev, "RX took %lld us", ktime_to_ns(ktime_sub(end, start))/1000);
			#endif

			if (header && (hba->bist_mode != BDO_MODE_TEST_DATA))
				handle_request(hba, header, payload);
			#endif
		} else {
			incoming_packet = 0;
		}

		/* process hard reset */
		if (pd_is1 & PD_RX_HR_SUCCESS) {
			dev_err(hba->dev, "PD_RX_HR_SUCCESS\n");

			pd_complete_hr(hba);

			if (hba->task_state != PD_STATE_SRC_HARD_RESET_RECOVER) {
				hba->last_state = hba->task_state;
				set_state(hba, PD_STATE_HARD_RESET_RECEIVED);
			}
		}

		/* process CC events */
		/* change state according to Type-C controller status */
		if ((cc_is0 & TYPE_C_CC_ENT_ATTACH_SRC_INTR) && !pd_is_power_swapping(hba))
			set_state(hba, PD_STATE_SRC_ATTACH);
		if ((cc_is0 & TYPE_C_CC_ENT_ATTACH_SNK_INTR) && !pd_is_power_swapping(hba))
			set_state(hba, PD_STATE_SNK_ATTACH);
		if (cc_is0 & TYPE_C_CC_ENT_DISABLE_INTR)  {
			dev_err(hba->dev, "TYPE_C_CC_ENT_DISABLE_INTR\n");
			set_state(hba, PD_STATE_DISABLED);
		}
		if (cc_is0 & TYPE_C_CC_ENT_UNATTACH_SRC_INTR)
			set_state(hba, PD_STATE_SRC_UNATTACH);

		if (cc_is0 & TYPE_C_CC_ENT_UNATTACH_SNK_INTR)
			set_state(hba, PD_STATE_SNK_UNATTACH);

		/*There is a VDM have to be sent.*/
		if (hba->vdm_state == VDM_STATE_READY)
			continue;

		curr_state = hba->task_state;

		/* if nothing to do, verify the state of the world in 500ms
		 *  also reset the timeout value set by the last state.
		 */
		timeout = (hba->timeout_user ? hba->timeout_user : PD_T_NO_ACTIVITY);

		/* skip DRP toggle cases*/
		if ((hba->dbg_lvl >= TYPEC_DBG_LVL_1)
			&& state_changed(hba)
			&& !(hba->last_state == PD_STATE_SRC_UNATTACH && hba->task_state == PD_STATE_SNK_UNATTACH)
			&& !(hba->last_state == PD_STATE_SNK_UNATTACH && hba->task_state == PD_STATE_SRC_UNATTACH)) {
			dev_err(hba->dev, "%s->%s\n",
				pd_state_mapping[hba->last_state].name, pd_state_mapping[hba->task_state].name);
		}

		switch (curr_state) {
		case PD_STATE_DISABLED:
		case PD_STATE_SRC_UNATTACH:
#ifdef CONFIG_USB_PD_DUAL_ROLE
		case PD_STATE_SNK_UNATTACH:
			/* Clear the input current limit */
			/* pd_set_input_current_limit(hba, 0, 0);*/
#ifdef CONFIG_CHARGE_MANAGER
			typec_set_input_current_limit(hba, 0, 0);
			charge_manager_set_ceil(hba,
						CEIL_REQUESTOR_PD,
						CHARGE_CEIL_NONE);
#endif
#ifdef CONFIG_USBC_VCONN
			typec_drive_vconn(hba, 0);
#endif
#endif

#ifdef CONFIG_CHARGE_MANAGER
			charge_manager_update_dualrole(hba, CAP_UNKNOWN);
#endif
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
			pd_dfp_exit_mode(hba, 0, 0);
#endif
			/* be aware of Vbus */
			typec_vbus_det_enable(hba, 1);

			/* disable RX */
			pd_rx_enable(hba, 0);

			/* disable BIST */
			pd_bist_enable(hba, 0, 0);

			/*TX header PD spec revision should be 01b=2.0*/
			typec_writew_msk(hba, 0x3<<6, PD_REV20<<6, PD_TX_HEADER);

			/*Restore back to real Rp*/
			typec_select_rp(hba, hba->rp_val);

			typec_set(hba, REG_PD_RX_DUPLICATE_INTR_EN, PD_INTR_EN_0);

			/*Improve RX signal quality from analog to digital part.*/
			if (state_changed(hba))
				pd_rx_phya_setting(hba);

			if (wake_lock_active(&hba->typec_wakelock))
				wake_unlock(&hba->typec_wakelock);

			typec_enable_lowq(hba, "PD-UNATTACHED");

			/*Disable 26MHz clock XO_PD*/
			clk_buf_ctrl(CLK_BUF_CHG, false);

			typec_write8_msk(hba, RG_EN_CLKSQ_PDTYPEC_MSK, 0, MAIN_CON4);

			if (hba->is_boost) {
				mt_ppm_sysboost_set_core_limit(BOOST_BY_USB_PD, 1, -1, -1);
				hba->is_boost = false;
			}

			hba->power_role = PD_NO_ROLE;
			hba->data_role = PD_NO_ROLE;
			hba->vdm_state = VDM_STATE_DONE;
			hba->cable_flags = PD_FLAGS_CBL_NO_INFO;
			hba->vsafe_5v = PD_VSAFE5V_LOW;

			cancel_delayed_work_sync(&hba->usb_work);
			schedule_delayed_work(&hba->usb_work, 0);

			if ((hba->charger_det_notify) && (hba->last_state != PD_STATE_DISABLED))
				hba->charger_det_notify(0);

			hba->flags &= ~PD_FLAGS_RESET_ON_DISCONNECT_MASK;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
			if (need_update(hba))
				update_dual_role_usb(hba, 0);
#endif
			/* When enter DISABLE/UNATTACHED.SRC/UNATTACHED.SNK, do _NOT_ check regularly of
			 * PD status. If there is any status change. CC interrupt will kick PD FSM and
			 * set a new timeout value to start polling check.
			 */
			timeout = ULONG_MAX;
			break;

		case PD_STATE_SRC_HARD_RESET_RECOVER:
			/* Do not continue until hard reset recovery time */
			if (time_after(hba->src_recover, jiffies)) {
				timeout = 50;
				break;
			}

			/* Enable VBUS */
			timeout = 10;
			if (!pd_set_power_supply_ready(hba)) {
				set_state(hba, PD_STATE_SRC_UNATTACH);
			} else {
				set_state(hba, PD_STATE_SRC_STARTUP);
				/* Need set again???
				 *  SNK : receive PD_RX_HR_SUCCESS
				 *  SRC : receive PD_TX_HR_SUCCESS
				 */
				pd_complete_hr(hba);

				pd_bist_enable(hba, 0, 0);
			}
			break;

		case PD_STATE_SRC_ATTACH:
			if (hba->vbus_en == 1) {
				/*Enable 26MHz clock XO_PD*/
				clk_buf_ctrl(CLK_BUF_CHG, true);

				typec_write8_msk(hba, RG_EN_CLKSQ_PDTYPEC_MSK,
							(1<<RG_EN_CLKSQ_PDTYPEC_OFST), MAIN_CON4);

				if (!hba->is_boost) {
					mt_ppm_sysboost_set_core_limit(BOOST_BY_USB_PD, 1, 4, 4);
					hba->is_boost = true;
				}

				if (!wake_lock_active(&hba->typec_wakelock))
					wake_lock(&hba->typec_wakelock);

				pd_rx_enable(hba, 1);

				pd_set_power_role(hba, PD_ROLE_SOURCE, 1);
				pd_set_data_role(hba, PD_ROLE_DFP);

				/*Improve RX signal quality from analog to digital part.*/
				if (state_changed(hba))
					pd_rx_phya_setting(hba);

#ifdef CONFIG_USBC_VCONN
				/* drive Vconn ONLY when there is Ra */
				if (hba->ra) {
					hba->flags |= PD_FLAGS_VCONN_ON;
				} else {
					typec_drive_vconn(hba, 0);
					hba->flags &= ~PD_FLAGS_VCONN_ON;
				}
#endif
				hba->flags |= (PD_FLAGS_CHECK_PR_ROLE);
				hba->cable_flags = PD_FLAGS_CBL_NO_INFO;
				hard_reset_count = 0;
				timeout = 5;
				set_state(hba, PD_STATE_SRC_STARTUP);
			} else {
				timeout = 5;
			}
			break;

		case PD_STATE_SRC_STARTUP:
			/* Wait for power source to enable */
			if (state_changed(hba)) {
				/*
				 * fake set data role swapped flag so we send
				 * discover identity when we enter SRC_READY
				 */
				hba->flags |= PD_FLAGS_DATA_SWAPPED;
				/* reset various counters */
				caps_count = 0;
				snk_cap_count = 0;
				pd_set_msg_id(hba, 0);
#ifdef SUPPORT_SOP_P
				set_state_timeout(hba, PD_POWER_SUPPLY_TURN_ON_DELAY, PD_STATE_DISCOVERY_SOP_P);
#else
				set_state_timeout(hba, PD_POWER_SUPPLY_TURN_ON_DELAY, PD_STATE_SRC_DISCOVERY);
#endif
			}
			break;

		case PD_STATE_SRC_DISCOVERY:
			if (state_changed(hba)) {
				/*
				 * If we have had PD connection with this port
				 * partner, then start NoResponseTimer.
				 */
				if (hba->flags & PD_FLAGS_PREVIOUS_PD_CONN)
					set_state_timeout(hba, PD_T_NO_RESPONSE,
						((hard_reset_count < PD_HARD_RESET_COUNT) ?
							PD_STATE_HARD_RESET_SEND : PD_STATE_SRC_UNATTACH));
			}

			/* Send source cap some minimum number of times */
			if (caps_count < PD_CAPS_COUNT) {
				/* Query capabilities of the other side */
				ret = send_source_cap(hba);
				/* packet was acked => PD capable device) */
				if (ret) {
					timeout = PD_T_SEND_SOURCE_CAP;
					caps_count++;
					dev_err(hba->dev, "SRC CAP[%d]", caps_count);
				} else {
					set_state(hba, PD_STATE_SRC_NEGOTIATE);

					/*
					 * Set SenderResponseTimer here to catch the time.
					 * Because the timeout value of SenderResponseTimer
					 * - tSenderResponse only has 24~30ms. If set the timer
					 * at PD_STATE_SRC_NEGOTIATE, the system overhead costs
					 * 5 more ms.
					 */
					set_state_timeout(hba, PD_T_SENDER_RESPONSE-3, PD_STATE_HARD_RESET_SEND);
					timeout = 0;
					hard_reset_count = 0;
					caps_count = 0;
					/* Port partner is PD capable */
					hba->flags |=
						PD_FLAGS_PREVIOUS_PD_CONN;
				}
			} else if (caps_count == PD_CAPS_COUNT) {
				set_state(hba, PD_STATE_SRC_DISABLED);
				timeout = 0;
			}
			break;

		case PD_STATE_SRC_DISABLED:
			if (state_changed(hba)) {
				dev_err(hba->dev, "SNK NO SUPPORT PD");
				pd_rx_enable(hba, 0);
				typec_select_rp(hba, hba->rp_val);
				timeout = PD_T_NO_ACTIVITY;

				if (wake_lock_active(&hba->typec_wakelock))
					wake_unlock(&hba->typec_wakelock);

				if (hba->is_boost) {
					mt_ppm_sysboost_set_core_limit(BOOST_BY_USB_PD, 1, -1, -1);
					hba->is_boost = false;
				}
			}
			break;

		case PD_STATE_SRC_NEGOTIATE:
			/* wait for a "Request" message */
			if (state_changed(hba)) {
				if (hba->last_state != PD_STATE_SRC_DISCOVERY)
					set_state_timeout(hba, PD_T_SENDER_RESPONSE-3, PD_STATE_HARD_RESET_SEND);
			}
			break;

		case PD_STATE_SRC_ACCEPTED:
			/* Accept sent, wait for enabling the new voltage */
			if (state_changed(hba))
				set_state_timeout(hba, PD_T_SRC_TRANSITION, PD_STATE_SRC_POWERED);
			break;

		case PD_STATE_SRC_POWERED:
			/* Switch to the new requested voltage */
			if (state_changed(hba)) {
				pd_transition_voltage(hba->requested_idx);
				set_state_timeout(hba,
					PD_POWER_SUPPLY_TURN_ON_DELAY__T_SRC_READY,
					PD_STATE_SRC_TRANSITION);
			}
			break;

		case PD_STATE_SRC_TRANSITION:
			/* the voltage output is good, notify the source */
			ret = send_control(hba, PD_CTRL_PS_RDY);
			if (ret) {
				/* The sink did not ack, cut the power... */
				/* pd_power_supply_reset(hba); */
				dev_err(hba->dev, "%s Go to SRC UNATTACH! %d", __func__, ret);
				set_state(hba, PD_STATE_SRC_DISABLED);
			} else {
				typec_select_rp(hba, hba->pd_rp_val);

				timeout = 50;
				/* it'a time to ping regularly the sink */
				set_state(hba, PD_STATE_SRC_READY);
			}
			break;

		case PD_STATE_SRC_READY:
#if RESET_STRESS_TEST
			complete(&hba->ready);
#endif
			timeout = PD_T_SOURCE_ACTIVITY;

			/*
			 * Don't send any PD traffic if we woke up due to
			 * incoming packet or if VDO response pending to avoid
			 * collisions.
			 */
			if (incoming_packet) {
				dev_err(hba->dev, "Incoming packet trigger, skip\n");
				break;
			}

			if (hba->vdm_state == VDM_STATE_BUSY)
				break;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
			if (need_update(hba))
				update_dual_role_usb(hba, 1);
#endif

			/* Send get sink cap if haven't received it yet */
			if (!(hba->flags & PD_FLAGS_SNK_CAP_RECVD)) {
				if (++snk_cap_count <= PD_SNK_CAP_RETRIES) {
					/* Get sink cap to know if dual-role device */
					send_control(hba, PD_CTRL_GET_SINK_CAP);
					set_state(hba, PD_STATE_SRC_GET_SINK_CAP);
					break;
				} else if (snk_cap_count == PD_SNK_CAP_RETRIES+1) {
					dev_err(hba->dev, "ERR SNK_CAP\n");
				}
			}

			/* Check power role policy, which may trigger a swap */
			if (hba->flags & PD_FLAGS_CHECK_PR_ROLE) {
				pd_check_pr_role(hba, PD_ROLE_SOURCE, hba->flags);
				hba->flags &= ~PD_FLAGS_CHECK_PR_ROLE;

				/*Kick FSM to enter the next stat earlier.*/
				timeout = 10;
				break;
			}

			/* Check data role policy, which may trigger a swap */
			if (hba->flags & PD_FLAGS_CHECK_DR_ROLE) {
				pd_check_dr_role(hba, hba->data_role, hba->flags);
				hba->flags &= ~PD_FLAGS_CHECK_DR_ROLE;

				/*Kick FSM to enter the next stat earlier.*/
				timeout = 10;
				break;
			}

#ifdef SUPPORT_SOP_P
			if (hba->data_role == PD_ROLE_DFP &&
				hba->cable_flags == PD_FLAGS_CBL_NO_INFO) {
				set_state(hba, PD_STATE_DISCOVERY_SOP_P);

				/*Kick FSM to enter the next stat earlier.*/
				timeout = 10;
				break;
			}
#endif

			/* Send discovery SVDMs last */
			if ((hba->discover_vmd == 1) &&
			     (hba->data_role == PD_ROLE_DFP) &&
			     (hba->flags & PD_FLAGS_DATA_SWAPPED)) {
				pd_send_vdm(hba, USB_SID_PD,
					    CMD_DISCOVER_IDENT, NULL, 0);

				hba->flags &= ~PD_FLAGS_DATA_SWAPPED;

				/*Kick FSM to enter the next stat earlier.*/
				timeout = 10;
				break;
			}

			if (!(hba->flags & PD_FLAGS_PING_ENABLED)) {
				timeout = PD_T_NO_ACTIVITY;
				break;
			}

			ret = send_control(hba, PD_CTRL_PING);
			#if 0
			/* Verify that the sink is alive */
			if (ret) {
				/* Ping dropped. Try soft reset. */
				set_state(hba, PD_STATE_SOFT_RESET);
				timeout = 10;
			}
			#endif

			break;

		case PD_STATE_SRC_GET_SINK_CAP:
			if (state_changed(hba))
				set_state_timeout(hba, PD_T_SENDER_RESPONSE, PD_STATE_SRC_READY);
			break;

		case PD_STATE_DR_SWAP:
			if (state_changed(hba)) {
				if (send_control(hba, PD_CTRL_DR_SWAP)) {
					timeout = 10;

					#if 0
					/*
					 * If failed to get goodCRC, send
					 * soft reset, otherwise ignore
					 * failure.
					 */
					set_state(hba, res == -1 ?
						   PD_STATE_SOFT_RESET :
						   READY_RETURN_STATE(port));
					#else
					set_state(hba, PD_STATE_SOFT_RESET);
					#endif

					break;
				}

				/* Wait for accept or reject */
				set_state_timeout(hba, PD_T_SENDER_RESPONSE, READY_RETURN_STATE(hba));
			}
			break;

#ifdef CONFIG_USB_PD_DUAL_ROLE
		case PD_STATE_SRC_SWAP_INIT:
			if (state_changed(hba)) {
				if (send_control(hba, PD_CTRL_PR_SWAP)) {
					timeout = 10;

					#if 0
					/*
					 * If failed to get goodCRC, send
					 * soft reset, otherwise ignore
					 * failure.
					 */
					set_state(hba, res == -1 ?
						   PD_STATE_SOFT_RESET :
						   PD_STATE_SRC_READY);
					#else
					set_state(hba, PD_STATE_SOFT_RESET);
					#endif

					break;
				}

				/* Wait for accept or reject */
				set_state_timeout(hba, PD_T_SENDER_RESPONSE, PD_STATE_SRC_READY);
			}
			break;

		case PD_STATE_SRC_SWAP_SNK_DISABLE:
			/* Give time for sink to stop drawing current */
			if (state_changed(hba)) {
				/* tSrcTransition: 25ms-35ms
				 *  The time the Source waits before transitioning the power supply to ensure
				 *  that the Sink has sufficient time to prepare.
				 *  NOTES:
				 *    However it needs time to really turn off VBUS,
				 *    so shorten this delay from tSrcTransition to 5ms.
				 *    Let the driver has enough time to turn off VBUS.
				 *    On the Ellysis Analyzer, VBUS starts being turned off
				 *    after 30ms from Accept-GOOD.CRC.
				 */
				set_state_timeout(hba, PD_T_SRC_TRANSITION-10, PD_STATE_SRC_SWAP_SRC_DISABLE);
			}
			break;

		case PD_STATE_SRC_SWAP_SRC_DISABLE:
			/* Turn power off */
			if (state_changed(hba)) {
				/*cut off power and do not detect vbus during the swap*/
				typec_vbus_det_enable(hba, 0);
				pd_power_supply_reset(hba);

				set_state_timeout(hba, PD_POWER_SUPPLY_TURN_OFF_DELAY__T_SRC_SWAP_STDBY,
					PD_STATE_SRC_SWAP_STANDBY);

				timeout = 25;
			}

			if (typec_vbus(hba) < PD_VSAFE0V_HIGH)
				set_state(hba, PD_STATE_SRC_SWAP_STANDBY);
			break;

		case PD_STATE_SRC_SWAP_STANDBY:
			/* Send PS_RDY to let sink know our power is off */
			if (state_changed(hba)) {
				/*Turn VBUS_PRESENT on because PR is changed to SNK later*/
				typec_vbus_present(hba, 1);

				/*Switch to Rd but update message header later*/
				pd_set_power_role(hba, PD_ROLE_SINK, 0);

				/*Send PD_RDY*/
				if (send_control(hba, PD_CTRL_PS_RDY)) {
					timeout = 10;
					set_state(hba, PD_STATE_SRC_UNATTACH);

					break;
				}

				/*instruct driver not to update PD on entry to SNK attach*/
				hba->flags |= PD_FLAGS_POWER_SWAPPED;

				/* Wait for PS_RDY from new source */
				set_state_timeout(hba, PD_T_PS_SOURCE_ON, PD_STATE_HARD_RESET_SEND);
			}
			break;

		case PD_STATE_SNK_HARD_RESET_RECOVER:
			if (state_changed(hba))
				hba->flags |= PD_FLAGS_DATA_SWAPPED;

#ifdef CONFIG_USB_PD_NO_VBUS_DETECT
			/*
			 * Can't measure vbus state so this is the maximum
			 * recovery time for the source.
			 */
			if (state_changed(hba))
				set_state_timeout(hba, get_time().val +
						  PD_T_SAFE_0V +
						  PD_T_SRC_RECOVER_MAX +
						  PD_T_SRC_TURN_ON,
						  PD_STATE_SNK_DISCONNECTED);
#else
			/* Wait for VBUS to go low and then high*/
			if (state_changed(hba)) {
				snk_hard_reset_vbus_off = 0;
				set_state_timeout(hba, PD_T_SAFE_0V,
					(hard_reset_count < PD_HARD_RESET_COUNT) ?
					PD_STATE_HARD_RESET_SEND : PD_STATE_SNK_DISCOVERY);
			}

			timeout = 20;
			if (!snk_hard_reset_vbus_off && (typec_vbus(hba) < PD_VSAFE0V_HIGH)) {
				/* VBUS has gone low, reset timeout */
				snk_hard_reset_vbus_off = 1;
				set_state_timeout(hba, (PD_T_SRC_RECOVER_MAX + PD_T_SRC_TURN_ON),
						  PD_STATE_SNK_HARD_RESET_NOVSAFE5V);
			}

			if (snk_hard_reset_vbus_off && (typec_vbus(hba) > PD_VSAFE5V_LOW)) {
				/* VBUS went high again */
				set_state(hba, PD_STATE_SNK_DISCOVERY);
				timeout = 10;

				pd_complete_hr(hba);
				typec_vbus_det_enable(hba, 1);

				pd_bist_enable(hba, 0, 0);
			}

			/*
			 * Don't need to set timeout because VBUS changing
			 * will trigger an interrupt and wake us up.
			 */
#endif
			break;

		case PD_STATE_SNK_HARD_RESET_NOVSAFE5V:
			/*
			 * Currently as SNK and sent the hard reset
			 * The partner as SRC turn off VBUS, but does not turn on VBUS.
			 * After tSrcRecover + tSrcTurnOn (1275ms), should just jump to unattached state
			 */
			if (state_changed(hba)) {
				if ((typec_read8(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST)
									== TYPEC_STATE_ATTACHED_SNK) {
					/* disable RX */
					pd_rx_enable(hba, 0);

					/* notify IP VBUS is gone.*/
					typec_vbus_present(hba, 0);
				}
			}
			break;

		case PD_STATE_SNK_ATTACH:
			if (!state_changed(hba))
				break;

			/*Enable 26MHz clock XO_PD*/
			clk_buf_ctrl(CLK_BUF_CHG, true);

			typec_write8_msk(hba, RG_EN_CLKSQ_PDTYPEC_MSK, (1<<RG_EN_CLKSQ_PDTYPEC_OFST), MAIN_CON4);

			if (!hba->is_boost) {
				mt_ppm_sysboost_set_core_limit(BOOST_BY_USB_PD, 1, 4, 4);
				hba->is_boost = true;
			}

			if (hba->charger_det_notify)
				hba->charger_det_notify(1);

			if (!wake_lock_active(&hba->typec_wakelock))
				wake_lock(&hba->typec_wakelock);

			/* reset message ID  on connection */
			pd_set_msg_id(hba, 0);
			/* initial data role for sink is UFP */
			pd_set_power_role(hba, PD_ROLE_SINK, 1);
			if (!(hba->flags & PD_FLAGS_POWER_SWAPPED)) /*don't update DR after SRC->SNK power role swap*/
				pd_set_data_role(hba, PD_ROLE_UFP);
			else
				hba->flags &= ~PD_FLAGS_POWER_SWAPPED; /*PR SWAP path 1*/
#ifdef CONFIG_CHARGE_MANAGER
			typec_curr = get_typec_current_limit(
				pd[port].polarity ? cc2 : cc1);
			typec_set_input_current_limit(
				port, typec_curr, TYPE_C_VOLTAGE);
#endif

			pd_rx_enable(hba, 1);

			/*Improve RX signal quality from analog to digital part.*/
			pd_rx_phya_setting(hba);

			/*
			 * fake set data role swapped flag so we send
			 * discover identity when we enter SRC_READY
			 */
			hba->flags |= (PD_FLAGS_CHECK_PR_ROLE | PD_FLAGS_DATA_SWAPPED);
			hba->cable_flags = PD_FLAGS_CBL_NO_INFO;
			hard_reset_count = 0;

			timeout = 10;
			set_state(hba, PD_STATE_SNK_DISCOVERY);
			break;

		case PD_STATE_SNK_DISCOVERY:
			/* Wait for source cap expired only if we are enabled */
			if (state_changed(hba)) {
				hba->flags &= ~PD_FLAGS_POWER_SWAPPED; /*PR SWAP path 2*/

				/*
				 * If we haven't passed hard reset counter,
				 * start SinkWaitCapTimer, otherwise start
				 * NoResponseTimer.
				 */
				if (hard_reset_count < PD_HARD_RESET_COUNT)
					set_state_timeout(hba, PD_T_SINK_WAIT_CAP, PD_STATE_HARD_RESET_SEND);
				else if (hba->flags & PD_FLAGS_PREVIOUS_PD_CONN)
					/* ErrorRecovery */
					set_state_timeout(hba, PD_T_NO_RESPONSE, PD_STATE_SNK_UNATTACH);
				else if (hba->last_state == PD_STATE_SNK_HARD_RESET_RECOVER &&
						hard_reset_count == PD_HARD_RESET_COUNT) {

					hba->flags |= PD_FLAGS_SRC_NO_PD;
					/*
					 * Once the Hard Reset has been retried nHardResetCount times
					 * then it shall be assumed that the remote device is
					 * non-responsive.
					 */
					dev_err(hba->dev, "SRC NO SUPPORT PD");
					typec_vbus_det_enable(hba, 1);
					timeout = PD_T_NO_ACTIVITY;

					if (hba->is_boost) {
						mt_ppm_sysboost_set_core_limit(BOOST_BY_USB_PD, 1, -1, -1);
						hba->is_boost = false;
					}

					if (wake_lock_active(&hba->typec_wakelock))
						wake_unlock(&hba->typec_wakelock);
				}
#ifdef CONFIG_CHARGE_MANAGER
				/*
				 * If we didn't come from disconnected, must
				 * have come from some path that did not set
				 * typec current limit. So, set to 0 so that
				 * we guarantee this is revised below.
				 */
				if (hba->last_state !=
				    PD_STATE_SNK_UNATTACH)
					typec_curr = 0;
#endif
			}

			if (hba->flags & PD_FLAGS_SRC_NO_PD) {
				int val = typec_vbus(hba);

				if (val < hba->vsafe_5v) {
					/* notify IP VBUS is gone.*/
					typec_vbus_present(hba, 0);
					dev_err(hba->dev, "Vbus OFF in Attached.SNK\n");
				}

			}
#ifdef CONFIG_CHARGE_MANAGER
			timeout = PD_T_SINK_ADJ - PD_T_DEBOUNCE;

			/* Check if CC pull-up has changed */
			tcpm_get_cc(hba, &cc1, &cc2);
			if (hba->polarity)
				cc1 = cc2;
			if (typec_curr != get_typec_current_limit(cc1)) {
				/* debounce signal by requiring two reads */
				if (typec_curr_change) {
					/* set new input current limit */
					typec_curr = get_typec_current_limit(
							cc1);
					typec_set_input_current_limit(
					  port, typec_curr, TYPE_C_VOLTAGE);
				} else {
					/* delay for debounce */
					timeout = PD_T_DEBOUNCE;
				}
				typec_curr_change = !typec_curr_change;
			} else {
				typec_curr_change = 0;
			}
#endif
			break;

		case PD_STATE_SNK_REQUESTED:
			/* Wait for ACCEPT or REJECT */
			if (state_changed(hba)) {
				hard_reset_count = 0;
				/*set_state_timeout(hba, PD_T_SENDER_RESPONSE, PD_STATE_HARD_RESET_SEND);*/

				/* TD.PD.SNK.E6 SenderResponseTimer Timeout - Accept - Testing Upstream Port
				 * FAILED
				 * SenderResponseTimer too long, HardReset detected after 30 ms (actual 39.4 ms)
				 * The driver set a 27ms timer to check the source does response or not.
				 * But the system has delay. The hard reset would be issued after 30ms to
				 * violate the spec. So shorten timeout value.
				 */
				set_state_timeout(hba, PD_T_SENDER_RESPONSE-3, PD_STATE_HARD_RESET_SEND);
			}
			break;

		case PD_STATE_SNK_TRANSITION:
			/* Wait for PS_RDY */
			if (state_changed(hba))
				set_state_timeout(hba, PD_T_PS_TRANSITION, PD_STATE_HARD_RESET_SEND);
			break;

		case PD_STATE_SNK_READY:
#if RESET_STRESS_TEST
			complete(&hba->ready);
#endif
			timeout = 20;

			/*
			 * Don't send any PD traffic if we woke up due to
			 * incoming packet or if VDO response pending to avoid
			 * collisions.
			 */
			if (incoming_packet || (hba->vdm_state == VDM_STATE_BUSY))
				break;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
			if (need_update(hba))
				update_dual_role_usb(hba, 1);
#endif
			/* Check for new power to request */
			if (hba->new_power_request) {
				pd_send_request_msg(hba, 0);
				break;
			}

			/* Check power role policy, which may trigger a swap */
			if (hba->flags & PD_FLAGS_CHECK_PR_ROLE) {
				pd_check_pr_role(hba, PD_ROLE_SINK, hba->flags);
				hba->flags &= ~PD_FLAGS_CHECK_PR_ROLE;
				break;
			}

			/* Check data role policy, which may trigger a swap */
			if (hba->flags & PD_FLAGS_CHECK_DR_ROLE) {
				pd_check_dr_role(hba, hba->data_role, hba->flags);
				hba->flags &= ~PD_FLAGS_CHECK_DR_ROLE;
				break;
			}

#ifdef SUPPORT_SOP_P
			if (hba->data_role == PD_ROLE_DFP &&
				hba->cable_flags == PD_FLAGS_CBL_NO_INFO) {
				set_state(hba, PD_STATE_DISCOVERY_SOP_P);

				/*Kick FSM to enter the next stat earlier.*/
				timeout = 10;
				break;
			}
#endif

			/* If DFP, send discovery SVDMs */
			if ((hba->discover_vmd == 1) &&
			     (hba->data_role == PD_ROLE_DFP) &&
			     (hba->flags & PD_FLAGS_DATA_SWAPPED)) {

				pd_send_vdm(hba, USB_SID_PD,
					    CMD_DISCOVER_IDENT, NULL, 0);

				hba->flags &= ~PD_FLAGS_DATA_SWAPPED;
				break;
			}

			/* Sent all messages, don't need to wake very often */
			timeout = PD_T_NO_ACTIVITY;
			break;

		case PD_STATE_SNK_SWAP_INIT:
			if (state_changed(hba)) {
				if (send_control(hba, PD_CTRL_PR_SWAP)) {
					timeout = 10;

					#if 0
					/*
					 * If failed to get goodCRC, send
					 * soft reset, otherwise ignore
					 * failure.
					 */
					set_state(hba, res == -1 ?
						   PD_STATE_SOFT_RESET :
						   PD_STATE_SNK_READY);
					#else
					set_state(hba, PD_STATE_SOFT_RESET);
					#endif

					break;
				}

				/* Wait for accept or reject */
				set_state_timeout(hba, PD_T_SENDER_RESPONSE, PD_STATE_SNK_READY);
			}
			break;

		case PD_STATE_SNK_SWAP_SNK_DISABLE:
			#if 0
			/* Stop drawing power */
			pd_set_input_current_limit(hba, 0, 0);
#ifdef CONFIG_CHARGE_MANAGER
			typec_set_input_current_limit(hba, 0, 0);
			charge_manager_set_ceil(hba,
						CEIL_REQUESTOR_PD,
						CHARGE_CEIL_NONE);
#endif
			#endif

			typec_vbus_det_enable(hba, 0);

			timeout = 10;
			set_state(hba, PD_STATE_SNK_SWAP_SRC_DISABLE);
			break;

		case PD_STATE_SNK_SWAP_SRC_DISABLE:
			/* Wait for PS_RDY */
			if (state_changed(hba)) {
				set_state_timeout(hba,
					PD_T_PS_SOURCE_OFF, PD_STATE_HARD_RESET_SEND);
			}
			break;

		case PD_STATE_SNK_SWAP_STANDBY:
			if (state_changed(hba)) {
				/*Switch to Rp*/
				pd_set_power_role(hba, PD_ROLE_SOURCE, 0);
				typec_vbus_det_enable(hba, 1);

				if (!pd_set_power_supply_ready(hba)) {
					/*Switch back to Rd*/
					pd_set_power_role(hba, PD_ROLE_SINK, 0);

					timeout = 10;
					set_state(hba, PD_STATE_SNK_UNATTACH);

					break;
				}

				/* Wait for power supply to turn on */
				set_state_timeout(hba,
					PD_POWER_SUPPLY_TURN_ON_DELAY__T_NEW_SRC, PD_STATE_SNK_SWAP_COMPLETE);
			}
			break;

		case PD_STATE_SNK_SWAP_COMPLETE:
			if (state_changed(hba)) {
				/* Send PS_RDY and change to source role */
				ret = send_control(hba, PD_CTRL_PS_RDY);
				if (ret) {
					/*Switch back to Rd*/
					pd_set_power_role(hba, PD_ROLE_SINK, 0);
					pd_power_supply_reset(hba);

					timeout = 10;
					set_state(hba, PD_STATE_SNK_UNATTACH);

					break;
				}

				/* Don't send GET_SINK_CAP on swap */
				snk_cap_count = PD_SNK_CAP_RETRIES+1;
				caps_count = 0;
				pd_set_msg_id(hba, 0);

				/*successful switch to SRC. vbus_present is no longer needed*/
				typec_vbus_present(hba, 0);

				/*update message header*/
				pd_set_power_role(hba, PD_ROLE_SOURCE, 1);

				set_state_timeout(hba, PD_T_SWAP_SOURCE_START, PD_STATE_SRC_DISCOVERY);
			}
			break;

#ifdef CONFIG_USBC_VCONN_SWAP
		case PD_STATE_VCONN_SWAP_SEND:
			if (state_changed(hba)) {
				if (send_control(hba, PD_CTRL_VCONN_SWAP)) {
					timeout = 10;

					#if 0
					/*
					 * If failed to get goodCRC, send
					 * soft reset, otherwise ignore
					 * failure.
					 */
					set_state(hba, res == -1 ?
						   PD_STATE_SOFT_RESET :
						   READY_RETURN_STATE(hba));
					#else
					set_state(hba, PD_STATE_SOFT_RESET);
					#endif

					break;
				}

				/* Wait for accept or reject */
				set_state_timeout(hba, PD_T_SENDER_RESPONSE, READY_RETURN_STATE(hba));
			}
			break;

		case PD_STATE_VCONN_SWAP_INIT:
			if (state_changed(hba)) {
				if (!(hba->flags & PD_FLAGS_VCONN_ON)) {
					/* Turn VCONN on and wait for it */
					typec_drive_vconn(hba, 1);
					set_state_timeout(hba, PD_VCONN_SWAP_DELAY, PD_STATE_VCONN_SWAP_READY);
				} else {
					set_state_timeout(hba,
						PD_T_VCONN_SOURCE_ON, READY_RETURN_STATE(hba));
				}
			}
			break;

		case PD_STATE_VCONN_SWAP_READY:
			if (state_changed(hba)) {
				if (!(hba->flags & PD_FLAGS_VCONN_ON)) {
					/* VCONN is now on, send PS_RDY */
					hba->flags |= PD_FLAGS_VCONN_ON;

					if (send_control(hba, PD_CTRL_PS_RDY)) {
						timeout = 10;

						/*
						 * If failed to get goodCRC,
						 * send soft reset
						 */
						set_state(hba, PD_STATE_SOFT_RESET);

						break;
					}

					set_state(hba, READY_RETURN_STATE(hba));
				} else {
					/* Turn VCONN off and wait for it */
					typec_drive_vconn(hba, 0);
					hba->flags &= ~PD_FLAGS_VCONN_ON;

					set_state_timeout(hba, PD_VCONN_SWAP_DELAY, READY_RETURN_STATE(hba));
				}
			}
			break;
#endif /* CONFIG_USBC_VCONN_SWAP */
#endif /* CONFIG_USB_PD_DUAL_ROLE */

		case PD_STATE_SOFT_RESET:
			if (state_changed(hba)) {
				/*
				 * No need to reset Message ID by SW. HW would reset Message ID auto.
				 * Transmit or Receive Hard Reset will clear all SOP/SOP'/SOP" Message ID.
				 * Transmit or Receive Soft Reset will clear one of SOP* Message ID which
				 * is the same as SOP* type of Soft Reset is received or transmitted.
				*/
				/* if soft reset failed, try hard reset. */
				if (send_control(hba, PD_CTRL_SOFT_RESET)) {
					/*
					 * 6.3.13 Soft Reset Message
					 * If the Soft_Reset Message fails a Hard Reset shall be initiated
					 * within tHardReset of the last CRCReceiveTimer expiring after
					 * nRetryCount retries have been completed.
					 * tHardReset = Max 5ms
					 */
					timeout = 0;
					set_state(hba, PD_STATE_HARD_RESET_SEND);

					break;
				}
#ifdef AUTO_SEND_HR
				if (hba->hr_auto_sent) {
					if (hba->power_role == PD_ROLE_SOURCE)
						set_state_timeout(hba,
							PD_T_PS_HARD_RESET-2, PD_STATE_HARD_RESET_EXECUTE);
					else {
						timeout = 0;
						set_state(hba,
							PD_STATE_HARD_RESET_EXECUTE);
					}
					hba->hr_auto_sent = 0;
				} else
#endif
					set_state_timeout(hba,
						PD_T_SENDER_RESPONSE, PD_STATE_HARD_RESET_SEND);
			}
			break;

		case PD_STATE_HARD_RESET_SEND:
			if (state_changed(hba))
				hard_reset_sent = 0;

#ifdef CONFIG_CHARGE_MANAGER
			if (hba->last_state == PD_STATE_SNK_DISCOVERY) {
				/*
				 * If discovery timed out, assume that we
				 * have a dedicated charger attached. This
				 * may not be a correct assumption according
				 * to the specification, but it generally
				 * works in practice and the harmful
				 * effects of a wrong assumption here
				 * are minimal.
				 */
				charge_manager_update_dualrole(hba,
							       CAP_DEDICATED);
			}
#endif
			/*
			 * If Hard Reset happened in the middle of doing PR_SWAP
			 * from SRC to SNK. The driver should check VBUS. If there
			 * is no VBUS, jump to UNATTACHED mode.
			 */
			if (hba->last_state == PD_STATE_SRC_SWAP_STANDBY)
				typec_vbus_det_enable(hba, 1);

			/* try sending hard reset until it succeeds */
			if (!hard_reset_sent) {
				hard_reset_count++;

				if (pd_transmit(hba, PD_TX_HARD_RESET, 0, NULL)) {
					timeout = 10;

					break;
				}

				/* successfully sent hard reset */
				hard_reset_sent = 1;

				/*
				 * If we are source, delay before cutting power
				 * to allow sink time to get hard reset.
				 */
				if (hba->power_role == PD_ROLE_SOURCE) {
					/*
					 * TD.PD.SRC.E6 PSHardResetTimer Timeout - Testing Downstream Port
					 * PSHardResetTimer is too long (actual 35.1 ms)
					 * Our original tPSHardReset is 25ms. But the exact time is over 35ms.
					 * So just cut the value a little bit.
					 */
					set_state_timeout(hba,
						PD_T_PS_HARD_RESET-2, PD_STATE_HARD_RESET_EXECUTE);
				} else {
					timeout = 0;
					/*Do not detect VBUS when receiving HARD RESET ASAP*/
					typec_vbus_det_enable(hba, 0);
					set_state(hba,
						PD_STATE_HARD_RESET_EXECUTE);
				}
			}
			break;
		case PD_STATE_HARD_RESET_RECEIVED:
			if (state_changed(hba)) {
				if (hba->power_role == PD_ROLE_SOURCE) {
					set_state_timeout(hba,
						PD_T_PS_HARD_RESET-2, PD_STATE_HARD_RESET_EXECUTE);
				} else {
					timeout = 0;
					/*Do not detect VBUS when receiving HARD RESET ASAP*/
					typec_vbus_det_enable(hba, 0);
					set_state(hba,
						PD_STATE_HARD_RESET_EXECUTE);
				}
			}
			break;
		case PD_STATE_HARD_RESET_EXECUTE:
			#if 0 /*covered by pd_execute_hard_reset*/
#ifdef CONFIG_USB_PD_DUAL_ROLE
			/*
			 * If hard reset while in the last stages of power
			 * swap, then we need to restore our CC resistor.
			 */
			if (hba->last_state == PD_STATE_SNK_SWAP_STANDBY)
				pd_set_power_role(hba, PD_ROLE_SINK);
#endif
			#endif

			/* reset our own state machine */
			pd_execute_hard_reset(hba);
			timeout = 0;
			break;

		case PD_STATE_BIST_CMD:
			if (state_changed(hba)) {
				send_bist_cmd(hba, hba->bist_mode);
				/* Delay at least enough for partner to finish BIST */
				timeout = PD_T_BIST_RECEIVE + 20;

				#if 0
				/* Set to appropriate port disconnected state */
				set_state(hba, DUAL_ROLE_IF_ELSE(hba, PD_STATE_SNK_UNATTACH, PD_STATE_SRC_UNATTACH));
				#else
				set_state(hba, DUAL_ROLE_IF_ELSE(hba, PD_STATE_SNK_READY, PD_STATE_SRC_READY));
				#endif
			}
			break;

		case PD_STATE_BIST_CARRIER_MODE_2:
			if (state_changed(hba)) {
				/* Start BIST_CARRIER_MODE2 */
				pd_bist_enable(hba, 1, BDO_MODE_CARRIER2);

				/* Delay at least enough to finish sending BIST */
				timeout = PD_T_BIST_TRANSMIT;

				/* Set to appropriate port disconnected state */
				set_state(hba, DUAL_ROLE_IF_ELSE(hba, PD_STATE_SNK_UNATTACH, PD_STATE_SRC_UNATTACH));
			}
			break;

		case PD_STATE_BIST_TEST_DATA:
			/*leave this state by hard reset*/
			if (state_changed(hba))
				pd_bist_enable(hba, 1, BDO_MODE_TEST_DATA);

			/*
			 * The driver receives tons of PD_RX_DUPLICATE_INTR during testing
			 * in BIST TEST DATA mode. When ISR handles the interrupt, the
			 * driver accesses registers by I2C. It causes kernel panic because
			 * it may schedule out when preparing I2C clock in transferring.
			 * But it forbids in ISR.
			 * schedule+0x24/0x74
			 * schedule_preempt_disabled+0x10/0x24
			 * mutex_lock_nested+0x1c4/0x538
			 * clk_prepare_lock+0x48/0xf8
			 * clk_unprepare+0x18/0x30
			 * mt_i2c_clock_disable+0x3c/0x4c
			 * mtk_i2c_transfer+0x324/0x1204
			 * At the other hand, this interrupt is meaningless at this moment.
			 * So mask this interrupt here and restore at UNATTACHED/DISABLE state.
			 */
			typec_clear(hba, REG_PD_RX_DUPLICATE_INTR_EN, PD_INTR_EN_0);

			timeout = ULONG_MAX;
			break;
#ifdef SUPPORT_SOP_P
		case PD_STATE_DISCOVERY_SOP_P:
			if (state_changed(hba)) {
				hba->cable_flags &= ~PD_FLAGS_CBL_NO_INFO;

				if (!send_cable_discovery_identity(hba, PD_TX_SOP_PRIME)) {
					hba->cable_flags |= PD_FLAGS_CBL_DISCOVERYING_SOP_P;

					dev_err(hba->dev, "%s Send SOP_P discovery identity success", __func__);

					timeout = PD_T_VDM_SNDR_RSP;

				} else {
					hba->cable_flags &= ~PD_FLAGS_CBL_DISCOVERYING_SOP_P;
					hba->cable_flags |= PD_FLAGS_CBL_NO_RESP_SOP_P;
					dev_err(hba->dev, "%s Send SOP_P discovery identity fail", __func__);
					timeout = 10;
				}

				if (hba->flags & PD_FLAGS_EXPLICIT_CONTRACT)
					set_state(hba, DUAL_ROLE_IF_ELSE(hba, PD_STATE_SNK_READY, PD_STATE_SRC_READY));
				else
					set_state(hba, PD_STATE_SRC_DISCOVERY);
			}
			break;
#endif

		case PD_STATE_SNK_KPOC_PWR_OFF:
			if (state_changed(hba)) {
				if (hba->charger_det_notify)
					hba->charger_det_notify(0);
			}
			break;
		default:
			break;
		}

		hba->last_state = curr_state;
	}

	dev_err(hba->dev, "%s EXIT", __func__);
	return 0;
}
#endif

