/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Policy Engine for SRC
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

#include <linux/delay.h>
#include "inc/pd_core.h"
#include "inc/pd_dpm_core.h"
#include "inc/tcpci.h"
#include "inc/pd_policy_engine.h"

/*
 * [PD2.0] Figure 8-38 Source Port Policy Engine state diagram
 */

void pe_src_startup_entry(struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_port->state_machine = PE_STATE_MACHINE_SOURCE;

	pd_port->cap_counter = 0;
	pd_port->request_i = -1;
	pd_port->request_v = TCPC_VBUS_SOURCE_5V;

	pd_reset_protocol_layer(pd_port, false);
	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_STARTUP);

	switch (pd_event->event_type) {
	case PD_EVT_HW_MSG:	/* CC attached */
		pd_enable_vbus_valid_detection(pd_port, true);
		break;

	case PD_EVT_PE_MSG: /* From Hard-Reset */
		pd_enable_timer(pd_port, PD_TIMER_SOURCE_START);
		break;

	case PD_EVT_CTRL_MSG: /* From PR-SWAP (Sent PS_RDY) */
#ifdef CONFIG_USB_PD_RESET_CABLE
		pd_port->reset_cable = true;
#endif	/* CONFIG_USB_PD_RESET_CABLE */

		pd_dpm_dynamic_enable_vconn(pd_port);
		pd_enable_timer(pd_port, PD_TIMER_SOURCE_START);
		break;
	}
}

void pe_src_discovery_entry(struct pd_port *pd_port, struct pd_event *pd_event)
{
	/* MessageID Should be 0 for First SourceCap (Ellisys)... */

	/* The SourceCapabilitiesTimer continues to run during the states
	 * defined in Source Startup Structured VDM Discover Identity State
	 * Diagram
	 */

	pd_port->msg_id_tx[TCPC_TX_SOP] = 0;
	pd_port->pd_connected = false;

	pd_enable_timer(pd_port, PD_TIMER_SOURCE_CAPABILITY);

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	if (pd_is_auto_discover_cable_id(pd_port)) {
		pd_port->msg_id_tx[TCPC_TX_SOP_PRIME] = 0;
		pd_enable_timer(pd_port, PD_TIMER_DISCOVER_ID);
	}
#endif	/* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */
}

void pe_src_send_capabilities_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_port->pd_wait_sender_response = true;

	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_SEND_WAIT_CAP);

	pd_dpm_send_source_caps(pd_port);
	pd_port->cap_counter++;

	pd_free_pd_event(pd_port, pd_event);	/* soft-reset */
}

void pe_src_negotiate_capabilities_entry(
		struct pd_port *pd_port, struct pd_event *pd_event)
{
#ifdef CONFIG_USB_PD_REV30
	if (!pd_port->pd_prev_connected) {
		pd_sync_sop_spec_revision(pd_port,
			PD_HEADER_REV(pd_event->pd_msg->msg_hdr));
	}
#endif	/* CONFIG_USB_PD_REV30 */

#ifdef CONFIG_PD_TA_WAKELOCK
	/* wakelock, no suspend waiting PD event */
	if (!pd_port->pd_prev_connected)
		wake_lock(&pd_port->tcpc_dev->attach_wake_lock);
#endif /* CONFIG_PD_TA_WAKELOCK */

	pd_port->pd_connected = true;
	pd_port->pd_prev_connected = true;

	pd_dpm_src_evaluate_request(pd_port, pd_event);
	pd_free_pd_event(pd_port, pd_event);
}

void pe_src_transition_supply_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	if (pd_event->event_type == PD_EVT_TCP_MSG)	/* goto-min */ {
		pd_port->request_i_new = pd_port->request_i_op;
		pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_GOTO_MIN);
	} else
		pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_ACCEPT);

	pd_enable_timer(pd_port, PD_TIMER_SOURCE_TRANSITION);
}

void pe_src_transition_supply_exit(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_disable_timer(pd_port, PD_TIMER_SOURCE_TRANSITION);
}

void pe_src_transition_supply2_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_PS_RDY);
}

void pe_src_ready_entry(struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_port->state_machine = PE_STATE_MACHINE_SOURCE;
	pd_notify_pe_src_explicit_contract(pd_port);
	pe_power_ready_entry(pd_port, pd_event);
}

void pe_src_disabled_entry(struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_DISABLE);
	pd_update_connect_state(pd_port, PD_CONNECT_TYPEC_ONLY);
	pd_dpm_dynamic_disable_vconn(pd_port);
}

void pe_src_capability_response_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	switch (pd_event->msg_sec) {
	case PD_DPM_NAK_REJECT_INVALID:
		pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_REJECT);
		pd_port->invalid_contract = true;
		break;
	case PD_DPM_NAK_REJECT:
		pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_REJECT);
		break;

	case PD_DPM_NAK_WAIT:
		pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_WAIT);
		break;
	default:
		break;
	}
}

void pe_src_hard_reset_entry(struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_send_hard_reset(pd_port);

	pd_free_pd_event(pd_port, pd_event);
	pd_enable_timer(pd_port, PD_TIMER_PS_HARD_RESET);
}

void pe_src_hard_reset_received_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_enable_timer(pd_port, PD_TIMER_PS_HARD_RESET);
}

void pe_src_transition_to_default_entry(
		struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_reset_local_hw(pd_port);
	pd_dpm_src_hard_reset(pd_port);
}

void pe_src_transition_to_default_exit(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_dpm_enable_vconn(pd_port, true);
	pd_enable_timer(pd_port, PD_TIMER_NO_RESPONSE);
}

void pe_src_get_sink_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_port->pd_wait_sender_response = true;

	pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_GET_SINK_CAP);
}

void pe_src_get_sink_cap_exit(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_dpm_dr_inform_sink_cap(pd_port, pd_event);
}

void pe_src_wait_new_capabilities_entry(
			struct pd_port *pd_port, struct pd_event *pd_event)
{
	/* Wait for new Source Capabilities */
}

void pe_src_send_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_port->pd_wait_sender_response = true;

	pd_send_soft_reset(pd_port, PE_STATE_MACHINE_SOURCE);
	pd_free_pd_event(pd_port, pd_event);
}

void pe_src_soft_reset_entry(struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_handle_soft_reset(pd_port, PE_STATE_MACHINE_SOURCE);
	pd_free_pd_event(pd_port, pd_event);
}

void pe_src_ping_entry(struct pd_port *pd_port, struct pd_event *pd_event)
{
	/* TODO: Send Ping Message */
}

/*
 * [PD2.0] Figure 8-81
 Source Startup Structured VDM Discover Identity State Diagram (TODO)
 */

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID

#ifdef CONFIG_PD_SRC_RESET_CABLE
void pe_src_cbl_send_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_DISCOVER_CABLE);

	pd_port->pd_wait_sender_response = true;

	pd_send_cable_soft_reset(pd_port);
	pd_free_pd_event(pd_port, pd_event);
}
#endif	/* CONFIG_PD_SRC_RESET_CABLE */

void pe_src_vdm_identity_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_DISCOVER_CABLE);

	pd_send_vdm_discover_id(pd_port, TCPC_TX_SOP_PRIME);

	pd_port->discover_id_counter++;
	pd_enable_timer(pd_port, PD_TIMER_VDM_RESPONSE);

	pd_free_pd_event(pd_port, pd_event);
}

void pe_src_vdm_identity_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_port->dpm_flags &= ~DPM_FLAGS_CHECK_CABLE_ID;

	pd_disable_timer(pd_port, PD_TIMER_VDM_RESPONSE);
	pd_dpm_inform_cable_id(pd_port, pd_event);

	pd_put_dpm_ack_event(pd_port);
	pd_free_pd_event(pd_port, pd_event);
}

void pe_src_vdm_identity_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_disable_timer(pd_port, PD_TIMER_VDM_RESPONSE);

#ifdef CONFIG_USB_PD_REV30
	pd_sync_sop_prime_spec_revision(pd_port, PD_REV20);
#endif	/* CONFIG_USB_PD_REV30 */

	pd_put_dpm_ack_event(pd_port);
	pd_free_pd_event(pd_port, pd_event);
}

#endif	/* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */

#ifdef CONFIG_USB_PD_REV30

void pe_src_send_source_alert_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	/* @@ */
}

void pe_src_give_source_status_entry(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	pd_port->local_status[0] = 0x44;
	pd_send_status(pd_port);

	pd_free_pd_event(pd_port, pd_event);
}

#endif	/* CONFIG_USB_PD_REV30 */
