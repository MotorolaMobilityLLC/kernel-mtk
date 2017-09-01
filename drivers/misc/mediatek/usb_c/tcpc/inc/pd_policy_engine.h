/*
 * Copyright (C) 2016 Richtek Technology Corp.
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

#ifndef PD_POLICY_ENGINE_H_
#define PD_POLICY_ENGINE_H_

#include "pd_core.h"
#include "tcpci_event.h"

/* ---- Policy Engine State ---- */

enum pd_pe_state_machine {
	PE_STATE_MACHINE_IDLE = 0,
	PE_STATE_MACHINE_SINK,
	PE_STATE_MACHINE_SOURCE,
	PE_STATE_MACHINE_DR_SWAP,
	PE_STATE_MACHINE_PR_SWAP,
	PE_STATE_MACHINE_VCONN_SWAP,
	PE_STATE_MACHINE_DBGACC,
};

enum pd_pe_state {
	PE_STATE_START_ID = -1,

/******************* Source *******************/
#ifdef CONFIG_USB_PD_PE_SOURCE
	PE_SRC_STARTUP,
	PE_SRC_DISCOVERY,
	PE_SRC_SEND_CAPABILITIES,
	PE_SRC_NEGOTIATE_CAPABILITIES,
	PE_SRC_TRANSITION_SUPPLY,
	PE_SRC_TRANSITION_SUPPLY2,
	PE_SRC_READY,
	PE_SRC_DISABLED,
	PE_SRC_CAPABILITY_RESPONSE,
	PE_SRC_HARD_RESET,
	PE_SRC_HARD_RESET_RECEIVED,
	PE_SRC_TRANSITION_TO_DEFAULT,
	PE_SRC_GET_SINK_CAP,
	PE_SRC_WAIT_NEW_CAPABILITIES,
	PE_SRC_SEND_SOFT_RESET,
	PE_SRC_SOFT_RESET,
	PE_SRC_PING,
#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
#ifdef CONFIG_PD_SRC_RESET_CABLE
	PE_SRC_CBL_SEND_SOFT_RESET,
#endif	/* CONFIG_PD_SRC_RESET_CABLE */
	PE_SRC_VDM_IDENTITY_REQUEST,
	PE_SRC_VDM_IDENTITY_ACKED,
	PE_SRC_VDM_IDENTITY_NAKED,
#endif	/* PD_CAP_PE_SRC_STARTUP_DISCOVER_ID */
#endif	/* CONFIG_USB_PD_PE_SOURCE */
/******************* Sink *******************/
#ifdef CONFIG_USB_PD_PE_SINK
	PE_SNK_STARTUP,
	PE_SNK_DISCOVERY,
	PE_SNK_WAIT_FOR_CAPABILITIES,
	PE_SNK_EVALUATE_CAPABILITY,
	PE_SNK_SELECT_CAPABILITY,
	PE_SNK_TRANSITION_SINK,
	PE_SNK_READY,
	PE_SNK_HARD_RESET,
	PE_SNK_TRANSITION_TO_DEFAULT,
	PE_SNK_GIVE_SINK_CAP,
	PE_SNK_GET_SOURCE_CAP,
	PE_SNK_SEND_SOFT_RESET,
	PE_SNK_SOFT_RESET,
#endif	/* CONFIG_USB_PD_PE_SINK */
/******************* DR_SWAP *******************/
#ifdef CONFIG_USB_PD_DR_SWAP
	PE_DRS_DFP_UFP_EVALUATE_DR_SWAP,
	PE_DRS_DFP_UFP_ACCEPT_DR_SWAP,
	PE_DRS_DFP_UFP_CHANGE_TO_UFP,
	PE_DRS_DFP_UFP_SEND_DR_SWAP,
	PE_DRS_DFP_UFP_REJECT_DR_SWAP,
	PE_DRS_UFP_DFP_EVALUATE_DR_SWAP,
	PE_DRS_UFP_DFP_ACCEPT_DR_SWAP,
	PE_DRS_UFP_DFP_CHANGE_TO_DFP,
	PE_DRS_UFP_DFP_SEND_DR_SWAP,
	PE_DRS_UFP_DFP_REJECT_DR_SWAP,
#endif	/* CONFIG_USB_PD_DR_SWAP */
/******************* PR_SWAP *******************/
#ifdef CONFIG_USB_PD_PR_SWAP
	PE_PRS_SRC_SNK_EVALUATE_PR_SWAP,
	PE_PRS_SRC_SNK_ACCEPT_PR_SWAP,
	PE_PRS_SRC_SNK_TRANSITION_TO_OFF,
	PE_PRS_SRC_SNK_ASSERT_RD,
	PE_PRS_SRC_SNK_WAIT_SOURCE_ON,
	PE_PRS_SRC_SNK_SEND_SWAP,
	PE_PRS_SRC_SNK_REJECT_PR_SWAP,
	PE_PRS_SNK_SRC_EVALUATE_PR_SWAP,
	PE_PRS_SNK_SRC_ACCEPT_PR_SWAP,
	PE_PRS_SNK_SRC_TRANSITION_TO_OFF,
	PE_PRS_SNK_SRC_ASSERT_RP,
	PE_PRS_SNK_SRC_SOURCE_ON,
	PE_PRS_SNK_SRC_SEND_SWAP,
	PE_PRS_SNK_SRC_REJECT_SWAP,
/* get same role cap */
	PE_DR_SRC_GET_SOURCE_CAP,
	PE_DR_SRC_GIVE_SINK_CAP,
	PE_DR_SNK_GET_SINK_CAP,
	PE_DR_SNK_GIVE_SOURCE_CAP,
#endif	/* CONFIG_USB_PD_PR_SWAP */
/******************* VCONN_SWAP *******************/
#ifdef CONFIG_USB_PD_VCONN_SWAP
	PE_VCS_SEND_SWAP,
	PE_VCS_EVALUATE_SWAP,
	PE_VCS_ACCEPT_SWAP,
	PE_VCS_REJECT_VCONN_SWAP,
	PE_VCS_WAIT_FOR_VCONN,
	PE_VCS_TURN_OFF_VCONN,
	PE_VCS_TURN_ON_VCONN,
	PE_VCS_SEND_PS_RDY,
#endif	/* CONFIG_USB_PD_VCONN_SWAP */
/******************* UFP_VDM *******************/
	PE_UFP_VDM_GET_IDENTITY,
	PE_UFP_VDM_SEND_IDENTITY,
	PE_UFP_VDM_GET_IDENTITY_NAK,
	PE_UFP_VDM_GET_SVIDS,
	PE_UFP_VDM_SEND_SVIDS,
	PE_UFP_VDM_GET_SVIDS_NAK,
	PE_UFP_VDM_GET_MODES,
	PE_UFP_VDM_SEND_MODES,
	PE_UFP_VDM_GET_MODES_NAK,
	PE_UFP_VDM_EVALUATE_MODE_ENTRY,
	PE_UFP_VDM_MODE_ENTRY_ACK,
	PE_UFP_VDM_MODE_ENTRY_NAK,
	PE_UFP_VDM_MODE_EXIT,
	PE_UFP_VDM_MODE_EXIT_ACK,
	PE_UFP_VDM_MODE_EXIT_NAK,
	PE_UFP_VDM_ATTENTION_REQUEST,
#ifdef CONFIG_USB_PD_ALT_MODE
	PE_UFP_VDM_DP_STATUS_UPDATE,
	PE_UFP_VDM_DP_CONFIGURE,
#endif/* CONFIG_USB_PD_ALT_MODE */
/******************* DFP_VDM *******************/
	PE_DFP_UFP_VDM_IDENTITY_REQUEST,
	PE_DFP_UFP_VDM_IDENTITY_ACKED,
	PE_DFP_UFP_VDM_IDENTITY_NAKED,
	PE_DFP_CBL_VDM_IDENTITY_REQUEST,
	PE_DFP_CBL_VDM_IDENTITY_ACKED,
	PE_DFP_CBL_VDM_IDENTITY_NAKED,
	PE_DFP_VDM_SVIDS_REQUEST,
	PE_DFP_VDM_SVIDS_ACKED,
	PE_DFP_VDM_SVIDS_NAKED,
	PE_DFP_VDM_MODES_REQUEST,
	PE_DFP_VDM_MODES_ACKED,
	PE_DFP_VDM_MODES_NAKED,
	PE_DFP_VDM_MODE_ENTRY_REQUEST,
	PE_DFP_VDM_MODE_ENTRY_ACKED,
	PE_DFP_VDM_MODE_ENTRY_NAKED,
	PE_DFP_VDM_MODE_EXIT_REQUEST,
	PE_DFP_VDM_MODE_EXIT_ACKED,
	PE_DFP_VDM_ATTENTION_REQUEST,
#ifdef CONFIG_PD_DFP_RESET_CABLE
	PE_DFP_CBL_SEND_SOFT_RESET,
	PE_DFP_CBL_SEND_CABLE_RESET,
#endif	/* CONFIG_PD_DFP_RESET_CABLE */
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	PE_DFP_VDM_DP_STATUS_UPDATE_REQUEST,
	PE_DFP_VDM_DP_STATUS_UPDATE_ACKED,
	PE_DFP_VDM_DP_STATUS_UPDATE_NAKED,
	PE_DFP_VDM_DP_CONFIGURATION_REQUEST,
	PE_DFP_VDM_DP_CONFIGURATION_ACKED,
	PE_DFP_VDM_DP_CONFIGURATION_NAKED,
#endif/* CONFIG_USB_PD_ALT_MODE_DFP */
/******************* UVDM & SVDM *******************/
#ifdef CONFIG_USB_PD_UVDM
	PE_UFP_UVDM_RECV,
	PE_DFP_UVDM_SEND,
	PE_DFP_UVDM_ACKED,
	PE_DFP_UVDM_NAKED,
#endif/* CONFIG_USB_PD_UVDM */
/******************* PD30 for PPS *******************/
#ifdef CONFIG_USB_PD_REV30
	PE_SRC_SEND_SOURCE_ALERT,
	PE_SNK_SOURCE_ALERT_RECEIVED,
	PE_SNK_GET_SOURCE_STATUS,
	PE_SRC_GIVE_SOURCE_STATUS,
#ifdef CONFIG_USB_PD_REV30_PPS_SINK
	PE_SNK_GET_PPS_STATUS,
#endif	/* CONFIG_USB_PD_REV30_PPS_SINK */
#ifdef CONFIG_USB_PD_REV30_PPS_SOURCE
	PE_SRC_GIVE_PPS_STATUS,
#endif	/* CONFIG_USB_PD_REV30_PPS_SOURCE */
#endif /* CONFIG_USB_PD_REV30 */
/******************* Others *******************/
#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	PE_DBG_READY,
#endif/* CONFIG_USB_PD_CUSTOM_DBGACC */
#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
	PE_OVER_RECV_HRESET_LIMIT,
#endif/* CONFIG_USB_PD_RECV_HRESET_COUNTER */
	PE_ERROR_RECOVERY,
	PE_BIST_TEST_DATA,
	PE_BIST_CARRIER_MODE_2,
/* Wait tx finished */
	PE_IDLE1,
	PE_IDLE2,
/******************* Virtual *******************/
	PD_NR_PE_STATES,
	PE_VIRT_HARD_RESET,
	PE_VIRT_READY,
};

/*
 * > 0 : Process Event
 * = 0 : No Event
 * < 0 : Error
*/

int pd_policy_engine_run(struct tcpc_device *tcpc_dev);


/* ---- Policy Engine (General) ---- */

void pe_power_ready_entry(struct pd_port *pd_port, struct pd_event *pd_event);




/******************* Source *******************/
#ifdef CONFIG_USB_PD_PE_SOURCE
void pe_src_startup_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_discovery_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_send_capabilities_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_negotiate_capabilities_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_transition_supply_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_transition_supply_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_transition_supply2_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_ready_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_disabled_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_capability_response_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_hard_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_hard_reset_received_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_transition_to_default_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_transition_to_default_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_get_sink_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_get_sink_cap_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_wait_new_capabilities_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_send_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_ping_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
#ifdef CONFIG_PD_SRC_RESET_CABLE
void pe_src_cbl_send_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_PD_SRC_RESET_CABLE */
void pe_src_vdm_identity_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_vdm_identity_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_vdm_identity_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* PD_CAP_PE_SRC_STARTUP_DISCOVER_ID */
#endif	/* CONFIG_USB_PD_PE_SOURCE */
/******************* Sink *******************/
#ifdef CONFIG_USB_PD_PE_SINK
void pe_snk_startup_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_discovery_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_wait_for_capabilities_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_wait_for_capabilities_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_evaluate_capability_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_select_capability_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_select_capability_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_transition_sink_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_transition_sink_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_ready_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_hard_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_transition_to_default_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_transition_to_default_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_give_sink_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_get_source_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_send_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_USB_PD_PE_SINK */
/******************* DR_SWAP *******************/
#ifdef CONFIG_USB_PD_DR_SWAP
void pe_drs_dfp_ufp_evaluate_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_dfp_ufp_accept_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_dfp_ufp_change_to_ufp_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_dfp_ufp_send_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_dfp_ufp_reject_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_ufp_dfp_evaluate_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_ufp_dfp_accept_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_ufp_dfp_change_to_dfp_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_ufp_dfp_send_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_drs_ufp_dfp_reject_dr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_USB_PD_DR_SWAP */
/******************* PR_SWAP *******************/
#ifdef CONFIG_USB_PD_PR_SWAP
void pe_prs_src_snk_evaluate_pr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_src_snk_accept_pr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_src_snk_transition_to_off_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_src_snk_assert_rd_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_src_snk_wait_source_on_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_src_snk_wait_source_on_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_src_snk_send_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_src_snk_reject_pr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_evaluate_pr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_accept_pr_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_transition_to_off_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_transition_to_off_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_assert_rp_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_source_on_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_source_on_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_send_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_prs_snk_src_reject_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
/* get same role cap */
void pe_dr_src_get_source_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dr_src_get_source_cap_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dr_src_give_sink_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dr_snk_get_sink_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dr_snk_get_sink_cap_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dr_snk_give_source_cap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_USB_PD_PR_SWAP */
/******************* VCONN_SWAP *******************/
#ifdef CONFIG_USB_PD_VCONN_SWAP
void pe_vcs_send_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_evaluate_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_accept_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_reject_vconn_swap_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_wait_for_vconn_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_wait_for_vconn_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_turn_off_vconn_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_turn_on_vconn_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_vcs_send_ps_rdy_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_USB_PD_VCONN_SWAP */
/******************* UFP_VDM *******************/
void pe_ufp_vdm_get_identity_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_send_identity_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_get_identity_nak_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_get_svids_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_send_svids_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_get_svids_nak_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_get_modes_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_send_modes_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_get_modes_nak_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_evaluate_mode_entry_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_mode_entry_ack_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_mode_entry_nak_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_mode_exit_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_mode_exit_ack_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_mode_exit_nak_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_attention_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#ifdef CONFIG_USB_PD_ALT_MODE
void pe_ufp_vdm_dp_status_update_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_ufp_vdm_dp_configure_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif/* CONFIG_USB_PD_ALT_MODE */
/******************* DFP_VDM *******************/
void pe_dfp_ufp_vdm_identity_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_ufp_vdm_identity_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_ufp_vdm_identity_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_cbl_vdm_identity_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_cbl_vdm_identity_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_cbl_vdm_identity_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_svids_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_svids_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_svids_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_modes_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_modes_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_modes_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_mode_entry_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_mode_entry_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_mode_entry_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_mode_exit_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_mode_exit_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_attention_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#ifdef CONFIG_PD_DFP_RESET_CABLE
void pe_dfp_cbl_send_soft_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_cbl_send_cable_reset_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_PD_DFP_RESET_CABLE */
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
void pe_dfp_vdm_dp_status_update_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_dp_status_update_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_dp_status_update_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_dp_configuration_request_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_dp_configuration_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_vdm_dp_configuration_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif/* CONFIG_USB_PD_ALT_MODE_DFP */
/******************* UVDM & SVDM *******************/
#ifdef CONFIG_USB_PD_UVDM
void pe_ufp_uvdm_recv_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_uvdm_send_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_uvdm_acked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_dfp_uvdm_naked_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif/* CONFIG_USB_PD_UVDM */
/******************* PD30 for PPS *******************/
#ifdef CONFIG_USB_PD_REV30
void pe_src_send_source_alert_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_source_alert_received_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_get_source_status_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_get_source_status_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_src_give_source_status_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#ifdef CONFIG_USB_PD_REV30_PPS_SINK
void pe_snk_get_pps_status_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_snk_get_pps_status_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_USB_PD_REV30_PPS_SINK */
#ifdef CONFIG_USB_PD_REV30_PPS_SOURCE
void pe_src_give_pps_status_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif	/* CONFIG_USB_PD_REV30_PPS_SOURCE */
#endif /* CONFIG_USB_PD_REV30 */
/******************* Others *******************/
#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
void pe_dbg_ready_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif/* CONFIG_USB_PD_CUSTOM_DBGACC */
#ifdef CONFIG_USB_PD_RECV_HRESET_COUNTER
void pe_over_recv_hreset_limit_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
#endif/* CONFIG_USB_PD_RECV_HRESET_COUNTER */
void pe_error_recovery_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_bist_test_data_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_bist_test_data_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_bist_carrier_mode_2_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_bist_carrier_mode_2_exit(
	struct pd_port *pd_port, struct pd_event *pd_event);
/* Wait tx finished */
void pe_idle1_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
void pe_idle2_entry(
	struct pd_port *pd_port, struct pd_event *pd_event);
/******************* Virtual *******************/

#endif /* PD_POLICY_ENGINE_H_ */
