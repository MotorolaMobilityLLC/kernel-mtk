/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Process Event
 *
 * Author: TH <tsunghan_tsai@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "inc/pd_core.h"
#include "inc/tcpci_event.h"
#include "inc/pd_process_evt.h"
#include "inc/pd_dpm_core.h"

/*
 * [BLOCK] print event
 */

#if PE_EVENT_DBG_ENABLE
static const char * const pd_ctrl_msg_name[] = {
	"ctrl0",
	"good_crc",
	"goto_min",
	"accept",
	"reject",
	"ping",
	"ps_rdy",
	"get_src_cap",
	"get_snk_cap",
	"dr_swap",
	"pr_swap",
	"vs_swap",
	"wait",
	"soft_reset",
	"ctrlE",
	"ctrlF",
};

static inline void print_ctrl_msg_event(uint8_t msg)
{
	if (msg < PD_CTRL_MSG_NR)
		PE_EVT_INFO("%s\r\n", pd_ctrl_msg_name[msg]);
}

static const char * const pd_data_msg_name[] = {
	"data0",
	"src_cap",
	"request",
	"bist",
	"sink_cap",
	"data5",
	"data6",
	"data7",
	"data8",
	"data9",
	"dataA",
	"dataB",
	"dataC",
	"dataD",
	"dataE",
	"vdm",
};

static inline void print_data_msg_event(uint8_t msg)
{
	if (msg < PD_DATA_MSG_NR)
		PE_EVT_INFO("%s\r\n", pd_data_msg_name[msg]);
}

static const char *const pd_hw_msg_name[] = {
	"Detached",
	"Attached",
	"hard_reset",
	"vbus_high",
	"vbus_low",
	"vbus_0v",
	"vbus_stable",
	"tx_err",
	"retry_vdm",
};

static inline void print_hw_msg_event(uint8_t msg)
{
	if (msg < PD_HW_MSG_NR)
		PE_EVT_INFO("%s\r\n", pd_hw_msg_name[msg]);
}

static const char *const pd_pe_msg_name[] = {
	"reset_prl_done",
	"pr_at_dft",
	"hard_reset_done",
	"pe_idle",
};

static inline void print_pe_msg_event(uint8_t msg)
{
	if (msg < PD_PE_MSG_NR)
		PE_EVT_INFO("%s\r\n", pd_pe_msg_name[msg]);
}

static const char * const pd_dpm_msg_name[] = {
	"ack",
	"nak",
	"cap_change",
};

static inline void print_dpm_msg_event(uint8_t msg)
{
	if (msg < PD_DPM_MSG_NR)
		PE_EVT_INFO("dpm_%s\r\n", pd_dpm_msg_name[msg]);
}

static const char * const tcp_dpm_evt_name[] = {
	/* TCP_DPM_EVT_PD_COMMAND */
	"pr_swap_snk",
	"pr_swap_src",
	"dr_swap_ufp",
	"dr_swap_dfp",
	"vc_swap_off",
	"vc_swap_on",
	"goto_min",
	"soft_reset",
	"get_src_cap",
	"get_snk_cap",
	"request",
	"request_ex",
	"request_again",
	"bist_cm2",

	/* TCP_DPM_EVT_VDM_COMMAND */
	"disc_cable",
	"disc_id",
	"disc_svid",
	"disc_mode",
	"enter_mode",
	"exit_mode",
	"attention",

#ifdef CONFIG_USB_PD_ALT_MODE
	"dp_atten",
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	"dp_status",
	"dp_config",
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
	"uvdm",
#endif	/* CONFIG_USB_PD_UVDM */

	/* TCP_DPM_EVT_IMMEDIATELY */
	"hard_reset",
	"error_recovery",
};

static inline void print_tcp_event(uint8_t msg)
{
	if (msg < TCP_DPM_EVT_NR)
		PE_EVT_INFO("tcp_event(%s)\r\n", tcp_dpm_evt_name[msg]);
}
#endif

static inline void print_event(pd_port_t *pd_port, pd_event_t *pd_event)
{
#if PE_EVENT_DBG_ENABLE
	switch (pd_event->event_type) {
	case PD_EVT_CTRL_MSG:
		print_ctrl_msg_event(pd_event->msg);
		break;

	case PD_EVT_DATA_MSG:
		print_data_msg_event(pd_event->msg);
		break;

	case PD_EVT_DPM_MSG:
		print_dpm_msg_event(pd_event->msg);
		break;

	case PD_EVT_HW_MSG:
		print_hw_msg_event(pd_event->msg);
		break;

	case PD_EVT_PE_MSG:
		print_pe_msg_event(pd_event->msg);
		break;

	case PD_EVT_TIMER_MSG:
		PE_EVT_INFO("timer\r\n");
		break;

	case PD_EVT_TCP_MSG:
		print_tcp_event(pd_event->msg);
		break;
	}
#endif
}

/*-----------------------------------------------------------------------------*/

bool pd_make_pe_state_transit(pd_port_t *pd_port,
	uint8_t curr_state, const pe_state_reaction_t *state_reaction)
{
	int i;
	const pe_state_transition_t *state_transition =
		state_reaction->state_transition;

	for (i = 0; i < state_reaction->nr_transition; i++) {
		if (state_transition[i].curr_state == curr_state) {
			PE_TRANSIT_STATE(pd_port,
				state_transition[i].next_state);
			return true;
		}
	}

	return false;
}

bool pd_make_pe_state_transit_virt(pd_port_t *pd_port,
	uint8_t curr_state, const pe_state_reaction_t *state_reaction)
{
	bool ret = pd_make_pe_state_transit(
			pd_port, curr_state, state_reaction);

	if (ret) {
		switch (pd_port->pe_state_next) {
		case PE_VIRT_READY:
			PE_TRANSIT_READY_STATE(pd_port);
			break;

		case PE_VIRT_HARD_RESET:
			PE_TRANSIT_HARD_RESET_STATE(pd_port);
			break;
		}
	}

	return ret;
}


bool pd_make_pe_state_transit_force(pd_port_t *pd_port,
	uint8_t curr_state, uint8_t force_state,
	const pe_state_reaction_t *state_reaction)
{
	bool ret = pd_make_pe_state_transit(
			pd_port, curr_state, state_reaction);

	if (ret)
		return ret;

	PE_TRANSIT_STATE(pd_port, force_state);
	return true;
}

/*-----------------------------------------------------------------------------*/

bool pd_process_protocol_error(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool power_change = false;
	pd_msg_t *pd_msg = pd_event->pd_msg;

	uint8_t event_type = pd_event->event_type;
	uint8_t msg_id = PD_HEADER_ID(pd_msg->msg_hdr);
	uint8_t msg_type = PD_HEADER_TYPE(pd_msg->msg_hdr);

	switch (pd_port->pe_state_curr) {
	case PE_SNK_TRANSITION_SINK:
	case PE_SRC_TRANSITION_SUPPLY:
	case PE_SRC_TRANSITION_SUPPLY2:
		power_change = true;
	case PE_PRS_SRC_SNK_WAIT_SOURCE_ON:
		if (pd_event_msg_match(pd_event,
				PD_EVT_CTRL_MSG, PD_CTRL_PING)) {
			PE_DBG("Ignore Ping\r\n");
			return false;
		}
		break;

	case PE_SRC_SOFT_RESET:
	case PE_SRC_SEND_SOFT_RESET:
	case PE_SNK_SOFT_RESET:
	case PE_SNK_SEND_SOFT_RESET:
	case PE_SNK_READY:
	case PE_SRC_READY:
	case PE_BIST_TEST_DATA:
		PE_DBG("Ignore Unknown Event\r\n");
		return false;
	};

	if (pd_port->tcpc_dev->pd_wait_hard_reset_complete) {
		PE_DBG("Ignore Event during HReset\r\n");
		return false;
	}

	PE_INFO("PRL_ERR: %d-%d-%d\r\n", event_type, msg_type, msg_id);

	if (pd_port->during_swap) {
#ifdef CONFIG_USB_PD_PR_SWAP_ERROR_RECOVERY
		PE_TRANSIT_STATE(pd_port, PE_ERROR_RECOVERY);
#else
		PE_TRANSIT_HARD_RESET_STATE(pd_port);
#endif
	} else if (power_change)
		PE_TRANSIT_HARD_RESET_STATE(pd_port);
	else
		PE_TRANSIT_SEND_SOFT_RESET_STATE(pd_port);

	return true;
}

bool pd_process_data_msg_bist(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	if (!pd_check_pe_state_ready(pd_port))
		return false;

	if (pd_port->request_v > 5000) {
		PE_INFO("bist_not_vsafe5v\r\n");
		return false;
	}

	switch (BDO_MODE(pd_event->pd_msg->payload[0])) {
	case BDO_MODE_TEST_DATA:
		PE_DBG("bist_test\r\n");
		PE_TRANSIT_STATE(pd_port, PE_BIST_TEST_DATA);
		pd_noitfy_pe_bist_mode(pd_port, PD_BIST_MODE_TEST_DATA);
		return true;

	case BDO_MODE_CARRIER2:
		PE_DBG("bist_cm2\r\n");
		PE_TRANSIT_STATE(pd_port, PE_BIST_CARRIER_MODE_2);
		pd_noitfy_pe_bist_mode(pd_port, PD_BIST_MODE_DISABLE);
		return true;

	default:
	case BDO_MODE_RECV:
	case BDO_MODE_TRANSMIT:
	case BDO_MODE_COUNTERS:
	case BDO_MODE_CARRIER0:
	case BDO_MODE_CARRIER1:
	case BDO_MODE_CARRIER3:
	case BDO_MODE_EYE:
		PE_DBG("Unsupport BIST\r\n");
		pd_noitfy_pe_bist_mode(pd_port, PD_BIST_MODE_DISABLE);
		return false;
	}

	return false;
}

/*-----------------------------------------------------------------------------
 * DRP (dr_swap, pr_swap, vconn_swap)
 *---------------------------------------------------------------------------
 */

bool pd_process_ctrl_msg_dr_swap(
		pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool accept = false;

	if (pd_port->modal_operation) {
		PE_TRANSIT_HARD_RESET_STATE(pd_port);
		return true;
	}

	if (!pd_check_pe_state_ready(pd_port))
		return false;

	if (pd_port->dpm_caps & DPM_CAP_LOCAL_DR_DATA) {
		if (pd_port->data_role == PD_ROLE_DFP) {
			accept = (pd_port->dpm_caps &
					DPM_CAP_DR_SWAP_REJECT_AS_UFP) == 0;
		} else {
			accept = (pd_port->dpm_caps &
					DPM_CAP_DR_SWAP_REJECT_AS_DFP) == 0;
		}
	}

	if (accept) {
		pd_port->during_swap = false;
		pd_port->state_machine = PE_STATE_MACHINE_DR_SWAP;

		PE_TRANSIT_DATA_STATE(pd_port,
				PE_DRS_UFP_DFP_EVALUATE_DR_SWAP,
				PE_DRS_DFP_UFP_EVALUATE_DR_SWAP);
		return true;
	}

	pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_REJECT);
	return false;
}

bool pd_process_ctrl_msg_pr_swap(
		pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool reject;

	if (!pd_check_pe_state_ready(pd_port))
		return false;

	reject = !(pd_port->dpm_caps & DPM_CAP_LOCAL_DR_POWER);

	if (!reject) {
		if (pd_port->power_role == PD_ROLE_SOURCE)
			reject = pd_port->dpm_caps &
					DPM_CAP_PR_SWAP_REJECT_AS_SNK;
		else
			reject = pd_port->dpm_caps &
					DPM_CAP_PR_SWAP_REJECT_AS_SRC;
	}

	if (reject) {
		pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_REJECT);
		return false;
	}
	pd_port->during_swap = false;
	pd_port->state_machine = PE_STATE_MACHINE_PR_SWAP;

	PE_TRANSIT_POWER_STATE(pd_port,
			PE_PRS_SNK_SRC_EVALUATE_PR_SWAP,
			PE_PRS_SRC_SNK_EVALUATE_PR_SWAP);

	return true;
}

bool pd_process_ctrl_msg_vconn_swap(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	if (!pd_check_pe_state_ready(pd_port))
		return false;

	if (!(pd_port->dpm_caps & DPM_CAP_LOCAL_VCONN_SUPPLY)) {
		pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_REJECT);
		return false;
	}
	pd_port->state_machine = PE_STATE_MACHINE_VCONN_SWAP;
	PE_TRANSIT_STATE(pd_port, PE_VCS_EVALUATE_SWAP);
	return true;
}

/*---------------------------------------------------------------------------*/

bool pd_process_recv_hard_reset(
		pd_port_t *pd_port, pd_event_t *pd_event, uint8_t hreset_state)
{
#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
	if (pd_port->recv_hard_reset_count > PD_HARD_RESET_COUNT) {
		PE_TRANSIT_STATE(pd_port, PE_OVER_RECV_HRESET_LIMIT);
		return true;
	}

	pd_port->recv_hard_reset_count++;
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

	PE_TRANSIT_STATE(pd_port, hreset_state);
	return true;
}

/*-----------------------------------------------------------------------------
 * tcp_event
 *---------------------------------------------------------------------------
 */

static inline int pd_handle_tcp_event_pr_swap(
	pd_port_t *pd_port, uint8_t new_role)
{
#ifdef CONFIG_USB_PD_PR_SWAP
	if (pd_port->power_role == new_role)
		return TCP_DPM_RET_DENIED_SAME_ROLE;

	if (!(pd_port->dpm_caps & DPM_CAP_LOCAL_DR_POWER))
		return TCP_DPM_RET_DENIED_LOCAL_CAP;

	if (!pd_check_pe_state_ready(pd_port))
		return TCP_DPM_RET_DENIED_NOT_READY;

	pd_port->during_swap = false;
	pd_port->state_machine = PE_STATE_MACHINE_PR_SWAP;

	PE_TRANSIT_POWER_STATE(pd_port,
		PE_PRS_SNK_SRC_SEND_SWAP,
		PE_PRS_SRC_SNK_SEND_SWAP);
	return TCP_DPM_RET_SENT;
#else
	return TCP_DPM_RET_DENIED_NO_SUPPORT;
#endif	/* CONFIG_USB_PD_PR_SWAP */
}

static inline int pd_handle_tcp_event_dr_swap(
	pd_port_t *pd_port, uint8_t new_role)
{
#ifdef CONFIG_USB_PD_DR_SWAP
	if (pd_port->data_role == new_role)
		return TCP_DPM_RET_DENIED_SAME_ROLE;

	if (!(pd_port->dpm_caps & DPM_CAP_LOCAL_DR_DATA))
		return TCP_DPM_RET_DENIED_LOCAL_CAP;

	if (!pd_check_pe_state_ready(pd_port))
		return TCP_DPM_RET_DENIED_NOT_READY;

	pd_port->during_swap = false;
	pd_port->state_machine = PE_STATE_MACHINE_DR_SWAP;

	PE_TRANSIT_DATA_STATE(pd_port,
		PE_DRS_UFP_DFP_SEND_DR_SWAP,
		PE_DRS_DFP_UFP_SEND_DR_SWAP);

	return TCP_DPM_RET_SENT;
#else
	return TCP_DPM_RET_DENIED_NO_SUPPORT;
#endif	/* CONFIG_USB_PD_DR_SWAP */
}

static inline int pd_handle_tcp_event_vconn_swap(
	pd_port_t *pd_port, uint8_t new_role)
{
#ifdef CONFIG_USB_PD_VCONN_SWAP
	if (pd_port->vconn_source == new_role)
		return TCP_DPM_RET_DENIED_SAME_ROLE;

	if (!(pd_port->dpm_caps & DPM_CAP_LOCAL_VCONN_SUPPLY))
		return TCP_DPM_RET_DENIED_LOCAL_CAP;

	if (!pd_check_pe_state_ready(pd_port))
		return TCP_DPM_RET_DENIED_NOT_READY;

	pd_port->state_machine = PE_STATE_MACHINE_VCONN_SWAP;
	PE_TRANSIT_STATE(pd_port, PE_VCS_SEND_SWAP);
	return TCP_DPM_RET_SENT;
#else
	return TCP_DPM_RET_DENIED_NO_SUPPORT;
#endif	/* CONFIG_USB_PD_VCONN_SWAP */
}

static inline int pd_handle_tcp_event_gotomin(pd_port_t *pd_port)
{
	if (pd_port->pe_state_curr != PE_SRC_READY)
		return TCP_DPM_RET_DENIED_NOT_READY;

	if (!(pd_port->dpm_flags & DPM_FLAGS_PARTNER_GIVE_BACK))
		return TCP_DPM_RET_DENIED_PARTNER_CAP;

	PE_TRANSIT_STATE(pd_port, PE_SRC_TRANSITION_SUPPLY);
	return TCP_DPM_RET_SENT;
}

static inline int pd_handle_tcp_event_softreset(pd_port_t *pd_port)
{
	if (!pd_check_pe_state_ready(pd_port))
		return TCP_DPM_RET_DENIED_NOT_READY;

	PE_TRANSIT_SEND_SOFT_RESET_STATE(pd_port);
	return TCP_DPM_RET_SENT;
}

static inline int pd_handle_tcp_event_get_source_cap(pd_port_t *pd_port)
{
	switch (pd_port->pe_state_curr) {
	case PE_SNK_READY:
		PE_TRANSIT_STATE(pd_port, PE_SNK_GET_SOURCE_CAP);
		return TCP_DPM_RET_SENT;

#ifdef CONFIG_USB_PD_PR_SWAP
	case PE_SRC_READY:
		if (pd_port->dpm_caps & DPM_CAP_LOCAL_DR_POWER) {
			PE_TRANSIT_STATE(pd_port, PE_DR_SRC_GET_SOURCE_CAP);
			return TCP_DPM_RET_SENT;
		}

		return TCP_DPM_RET_DENIED_LOCAL_CAP;
#endif	/* CONFIG_USB_PD_PR_SWAP */
	}

	return TCP_DPM_RET_DENIED_NOT_READY;
}

static inline int pd_handle_tcp_event_get_sink_cap(pd_port_t *pd_port)
{
	switch (pd_port->pe_state_curr) {
	case PE_SRC_READY:
		PE_TRANSIT_STATE(pd_port, PE_SRC_GET_SINK_CAP);
		return TCP_DPM_RET_SENT;

#ifdef CONFIG_USB_PD_PR_SWAP
	case PE_SNK_READY:
		if (pd_port->dpm_caps & DPM_CAP_LOCAL_DR_POWER) {
			PE_TRANSIT_STATE(pd_port, PE_DR_SNK_GET_SINK_CAP);
			return TCP_DPM_RET_SENT;
		}

		return TCP_DPM_RET_DENIED_LOCAL_CAP;
#endif	/* CONFIG_USB_PD_PR_SWAP */
	}

	return TCP_DPM_RET_DENIED_NOT_READY;
}

static inline int pd_handle_tcp_event_request(pd_port_t *pd_port)
{
	bool ret = false;
	tcp_dpm_event_t *tcp_event = &pd_port->tcp_event;

	if (pd_port->pe_state_curr != PE_SNK_READY)
		return TCP_DPM_RET_DENIED_NOT_READY;

	switch (tcp_event->event_id) {
	case TCP_DPM_EVT_REQUEST:
		ret = pd_dpm_update_tcp_request(
			pd_port, &tcp_event->tcp_dpm_data.pd_req);
		break;
	case TCP_DPM_EVT_REQUEST_EX:
		ret = pd_dpm_update_tcp_request_ex(
			pd_port, &tcp_event->tcp_dpm_data.pd_req_ex);
		break;
	case TCP_DPM_EVT_REQUEST_AGAIN:
		ret = pd_dpm_update_tcp_request_again(pd_port);
		break;
	}

	if (!ret)
		return TCP_DPM_RET_DENIED_INVALID_REQUEST;

	PE_TRANSIT_STATE(pd_port, PE_SNK_SELECT_CAPABILITY);
	return TCP_DPM_RET_SENT;
}

static inline int pd_handle_tcp_event_bist_cm2(pd_port_t *pd_port)
{
	uint32_t bist = BDO_MODE_CARRIER2;

	if (!pd_check_pe_state_ready(pd_port))
		return TCP_DPM_RET_DENIED_NOT_READY;

	pd_send_data_msg(pd_port, TCPC_TX_SOP, PD_DATA_BIST, 1, &bist);
	return TCP_DPM_RET_SENT;
}

static inline int pd_handle_tcp_event_hardreset(pd_port_t *pd_port)
{
	PE_TRANSIT_HARD_RESET_STATE(pd_port);
	return TCP_DPM_RET_SENT;
}

static inline int pd_handle_tcp_event_error_recovery(pd_port_t *pd_port)
{
	PE_TRANSIT_STATE(pd_port, PE_ERROR_RECOVERY);
	return TCP_DPM_RET_SENT;
}

static inline int pd_process_tcp_dpm_event(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	int ret = TCP_DPM_RET_DENIED_UNKNOWM;

	switch (pd_event->msg) {
	case TCP_DPM_EVT_PR_SWAP_AS_SNK:
	case TCP_DPM_EVT_PR_SWAP_AS_SRC:
		ret = pd_handle_tcp_event_pr_swap(pd_port,
			pd_event->msg - TCP_DPM_EVT_PR_SWAP_AS_SNK);
		break;

	case TCP_DPM_EVT_DR_SWAP_AS_UFP:
	case TCP_DPM_EVT_DR_SWAP_AS_DFP:
		ret = pd_handle_tcp_event_dr_swap(pd_port,
			pd_event->msg - TCP_DPM_EVT_DR_SWAP_AS_UFP);
		break;

	case TCP_DPM_EVT_VCONN_SWAP_OFF:
	case TCP_DPM_EVT_VCONN_SWAP_ON:
		ret = pd_handle_tcp_event_vconn_swap(pd_port,
			pd_event->msg - TCP_DPM_EVT_VCONN_SWAP_OFF);
		break;

	case TCP_DPM_EVT_GOTOMIN:
		ret = pd_handle_tcp_event_gotomin(pd_port);
		break;

	case TCP_DPM_EVT_SOFTRESET:
		ret = pd_handle_tcp_event_softreset(pd_port);
		break;

	case TCP_DPM_EVT_GET_SOURCE_CAP:
		ret = pd_handle_tcp_event_get_source_cap(pd_port);
		break;

	case TCP_DPM_EVT_GET_SINK_CAP:
		ret = pd_handle_tcp_event_get_sink_cap(pd_port);
		break;

	case TCP_DPM_EVT_REQUEST:
	case TCP_DPM_EVT_REQUEST_EX:
	case TCP_DPM_EVT_REQUEST_AGAIN:
		ret = pd_handle_tcp_event_request(pd_port);
		break;

	case TCP_DPM_EVT_BIST_CM2:
		ret = pd_handle_tcp_event_bist_cm2(pd_port);
		break;

	case TCP_DPM_EVT_HARD_RESET:
		ret = pd_handle_tcp_event_hardreset(pd_port);
		break;

	case TCP_DPM_EVT_ERROR_RECOVERY:
		ret = pd_handle_tcp_event_error_recovery(pd_port);
		break;
	}

	pd_notify_current_tcp_event_result(pd_port, ret);
	return ret == TCP_DPM_RET_SENT;
}

/*
 *
 * @ true : valid message
 * @ false : invalid message, pe should drop the message
 */

static inline bool pe_is_valid_pd_msg(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	pd_msg_t *pd_msg = pd_event->pd_msg;

	uint8_t event_type = pd_event->event_type;
	uint8_t sop_type = pd_msg->frame_type;
	uint8_t msg_id = PD_HEADER_ID(pd_msg->msg_hdr);
	uint8_t msg_type = PD_HEADER_TYPE(pd_msg->msg_hdr);

	if (pd_port->pe_state_curr == PE_BIST_TEST_DATA)
		return false;

	if (event_type == PD_EVT_CTRL_MSG) {
		switch (msg_type) {
		/* SofReset always has a MessageID value of zero */
		case PD_CTRL_SOFT_RESET:
			if (msg_id != 0) {
				PE_INFO("Repeat soft_reset\r\n");
				return false;
			}
			return true;

		case PD_CTRL_GOOD_CRC:
			PE_DBG("Discard_CRC\r\n");
			return true;

#ifdef CONFIG_USB_PD_IGNORE_PS_RDY_AFTER_PR_SWAP
		case PD_CTRL_PS_RDY:
			if (pd_port->msg_id_pr_swap_last == msg_id) {
				PE_INFO("Repeat ps_rdy\r\n");
				return false;
			}
			break;
#endif	/* CONFIG_USB_PD_IGNORE_PS_RDY_AFTER_PR_SWAP */
		}
	}

	if ((pd_port->msg_id_rx_init[sop_type]) &&
		(pd_port->msg_id_rx[sop_type] == msg_id)) {
		PE_INFO("Repeat msg: %c:%d:%d\r\n",
			(pd_event->event_type == PD_EVT_CTRL_MSG) ? 'C' : 'D',
			pd_event->msg, msg_id);
		return false;
	}

	pd_port->msg_id_rx[sop_type] = msg_id;
	pd_port->msg_id_rx_init[sop_type] = true;

	return true;
}


static inline bool pe_is_valid_pd_msg_role(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool ret = true;
	uint8_t msg_pr, msg_dr;
	pd_msg_t *pd_msg = pd_event->pd_msg;

	if (pd_msg == NULL)	/* Good-CRC */
		return true;

	if (pd_msg->frame_type != TCPC_TX_SOP)
		return true;

	msg_pr = PD_HEADER_PR(pd_msg->msg_hdr);
	msg_dr = PD_HEADER_DR(pd_msg->msg_hdr);

	/*
	 * The Port Power Role field of a received Message shall not be verified
	 * by the receiver and no error recovery action shall be
	 * taken if it is incorrect.
	 */

	if (msg_pr == pd_port->power_role)
		PE_DBG("Wrong PR:%d\r\n", msg_pr);

	/*
	 * Should a Type-C Port receive a Message with the Port Data Role field
	 * set to the same Data Role as its current Data Role,
	 * except for the GoodCRC Message,
	 * Type-C error recovery actions as defined
	 * in [USBType-C 1.0] shall be performed.
	 */

	if (msg_dr == pd_port->data_role) {
#ifdef CONFIG_USB_PD_CHECK_DATA_ROLE
		ret = false;
#endif
		PE_INFO("Wrong DR:%d\r\n", msg_dr);
	}

	return ret;
}

static inline bool pe_translate_pd_msg_event(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	pd_msg_t *pd_msg;

	if (pd_event->event_type != PD_EVT_PD_MSG)
		return true;

	pd_msg = pd_event->pd_msg;

	if (PD_HEADER_CNT(pd_msg->msg_hdr))
		pd_event->event_type = PD_EVT_DATA_MSG;
	else
		pd_event->event_type = PD_EVT_CTRL_MSG;

	pd_event->msg = PD_HEADER_TYPE(pd_msg->msg_hdr);

	return pe_is_valid_pd_msg(pd_port, pd_event);
}

static inline bool pe_exit_idle_state(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool act_as_sink;

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	pd_port->custom_dbgacc = false;
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */

	switch (pd_event->msg_sec) {
	case TYPEC_ATTACHED_SNK:
		act_as_sink = true;
		break;

	case TYPEC_ATTACHED_SRC:
		act_as_sink = false;
		break;

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	case TYPEC_ATTACHED_DBGACC_SNK:
		act_as_sink = true;
		pd_port->custom_dbgacc = true;
		break;
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */

	default:
		return false;
	}

	if (act_as_sink) {
		pd_init_role(pd_port,
			PD_ROLE_SINK, PD_ROLE_UFP, PD_ROLE_VCONN_OFF);
	} else {
		pd_init_role(pd_port,
			PD_ROLE_SOURCE, PD_ROLE_DFP, PD_ROLE_VCONN_ON);
	}

	pd_port->cap_counter = 0;
	pd_port->discover_id_counter = 0;
	pd_port->hard_reset_counter = 0;
	pd_port->get_snk_cap_count = 0;
	pd_port->get_src_cap_count = 0;
	pd_port->send_pr_swap_count = 0;
	pd_port->send_dr_swap_count = 0;

#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
	pd_port->recv_hard_reset_count = 0;
#endif	/* CONFIG_USB_PD_RECV_HRESET_COUNTER */

	pd_port->pe_ready = 0;
	pd_port->pd_connected = 0;
	pd_port->pd_prev_connected = 0;
	pd_port->reset_vdm_state = 0;
	pd_port->power_cable_present = 0;

	pd_port->explicit_contract = false;
	pd_port->invalid_contract = false;

	pd_port->modal_operation = false;
	pd_port->during_swap = false;
	pd_port->dpm_ack_immediately = false;

#ifdef CONFIG_USB_PD_DFP_FLOW_DELAY
#ifdef CONFIG_USB_PD_DFP_FLOW_DELAY_STARTUP
	pd_port->dpm_dfp_flow_delay_done = false;
#else
	pd_port->dpm_dfp_flow_delay_done = true;
#endif	/* CONFIG_USB_PD_DFP_FLOW_DELAY_STARTUP */
#endif	/* CONFIG_USB_PD_DFP_FLOW_DELAY */

#ifdef CONFIG_USB_PD_UFP_FLOW_DELAY
	pd_port->dpm_ufp_flow_delay_done = false;
#endif	/* CONFIG_USB_PD_UFP_FLOW_DELAY */

#ifdef CONFIG_USB_PD_KEEP_PARTNER_ID
	pd_port->partner_id_present = false;
	memset(pd_port->partner_vdos, 0, sizeof(uint32_t) * VDO_MAX_NR);
#endif	/* CONFIG_USB_PD_KEEP_PARTNER_ID */

#ifdef CONFIG_USB_PD_KEEP_SVIDS
	pd_port->remote_svid_list.cnt = 0;
#endif	/* CONFIG_USB_PD_KEEP_SVIDS */

	pd_port->remote_src_cap.nr = 0;
	pd_port->remote_snk_cap.nr = 0;

	memset(pd_port->cable_vdos, 0, sizeof(uint32_t) * VDO_MAX_NR);

	pd_notify_pe_running(pd_port);
	pd_dpm_notify_pe_startup(pd_port);
	return true;
}

static inline bool pe_is_trap_in_idle_state(
	pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool trap = true;

	switch (pd_port->pe_state_curr) {
	case PE_IDLE1:
		if (pd_event_msg_match(pd_event, PD_EVT_PE_MSG, PD_PE_IDLE))
			return false;
		pd_try_put_pe_idle_event(pd_port);
	case PE_IDLE2:
		break;

	default:
		return false;
	}

	if (pd_event->event_type == PD_EVT_HW_MSG) {
		switch (pd_event->msg) {
		case PD_HW_CC_ATTACHED:
			trap = false;
			break;

		case PD_HW_CC_DETACHED:
			pd_notify_pe_idle(pd_port);
			break;

		default:
			break;
		}
	}

	if (!trap)
		trap = !pe_exit_idle_state(pd_port, pd_event);
	return trap;
}

bool pd_process_event(pd_port_t *pd_port, pd_event_t *pd_event, bool vdm_evt)
{
	bool ret = false;

	if (pe_is_trap_in_idle_state(pd_port, pd_event)) {
		PE_DBG("Trap in idle state, Ignore All MSG\r\n");
		return false;
	}

	if (!pe_translate_pd_msg_event(pd_port, pd_event))
		return false;

#if PE_EVT_INFO_VDM_DIS
	if (!vdm_evt)
#endif
		print_event(pd_port, pd_event);

	switch (pd_event->event_type) {
	case PD_EVT_CTRL_MSG:
	case PD_EVT_DATA_MSG:
		if (!pe_is_valid_pd_msg_role(pd_port, pd_event)) {
			PE_TRANSIT_STATE(pd_port, PE_ERROR_RECOVERY);
			return true;
		}
		break;
	}

	if (vdm_evt)
		return pd_process_event_vdm(pd_port, pd_event);

	if (pd_event->event_type == PD_EVT_TCP_MSG)
		return pd_process_tcp_dpm_event(pd_port, pd_event);

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	if (pd_port->custom_dbgacc)
		return pd_process_event_dbg(pd_port, pd_event);
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */

	if ((pd_event->event_type == PD_EVT_CTRL_MSG) &&
		(pd_event->msg != PD_CTRL_GOOD_CRC) &&
		(pd_event->pd_msg->frame_type != TCPC_TX_SOP)) {
		PE_DBG("Ignore not SOP Ctrl Msg\r\n");
		return false;
	}

	switch (pd_port->state_machine) {
	case PE_STATE_MACHINE_DR_SWAP:
		ret = pd_process_event_drs(pd_port, pd_event);
		break;
	case PE_STATE_MACHINE_PR_SWAP:
		ret = pd_process_event_prs(pd_port, pd_event);
		break;
	case PE_STATE_MACHINE_VCONN_SWAP:
		ret = pd_process_event_vcs(pd_port, pd_event);
		break;
	}

	if (ret)
		return true;

	if (pd_port->power_role == PD_ROLE_SINK)
		ret = pd_process_event_snk(pd_port, pd_event);
	else
		ret = pd_process_event_src(pd_port, pd_event);

	return ret;
}
