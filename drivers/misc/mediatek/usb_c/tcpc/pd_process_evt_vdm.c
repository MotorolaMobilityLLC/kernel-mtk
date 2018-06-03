/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Process Event For VDM
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

#define VDM_CMD_STATE(cmd, cmd_type)	\
	((cmd & 0x1f) | ((cmd_type & 0x03) << 6))

#define VDM_CMD_INIT_STATE(cmd, next_state)	\
	{ VDM_CMD_STATE(cmd, CMDT_INIT), next_state }

#define VDM_CMD_ACK_STATE(cmd, next_state)	\
	{ VDM_CMD_STATE(cmd, CMDT_RSP_ACK), next_state }

#define VDM_CMD_NACK_STATE(cmd, next_state)	\
	{ VDM_CMD_STATE(cmd, CMDT_RSP_NAK), next_state }

#define VDM_CMD_BUSY_STATE(cmd, next_state)	\
	{ VDM_CMD_STATE(cmd, CMDT_RSP_BUSY), next_state }

/* UFP PD VDM Command's reactions */

DECL_PE_STATE_TRANSITION(PD_UFP_VDM_CMD) = {

	VDM_CMD_INIT_STATE(CMD_DISCOVER_IDENT, PE_UFP_VDM_GET_IDENTITY),
	VDM_CMD_INIT_STATE(CMD_DISCOVER_SVID, PE_UFP_VDM_GET_SVIDS),
	VDM_CMD_INIT_STATE(CMD_DISCOVER_MODES, PE_UFP_VDM_GET_MODES),
	VDM_CMD_INIT_STATE(CMD_ENTER_MODE, PE_UFP_VDM_EVALUATE_MODE_ENTRY),
	VDM_CMD_INIT_STATE(CMD_EXIT_MODE, PE_UFP_VDM_MODE_EXIT),

#ifdef CONFIG_USB_PD_ALT_MODE
	VDM_CMD_INIT_STATE(CMD_DP_STATUS, PE_UFP_VDM_DP_STATUS_UPDATE),
	VDM_CMD_INIT_STATE(CMD_DP_CONFIG, PE_UFP_VDM_DP_CONFIGURE),
#endif
};
DECL_PE_STATE_REACTION(PD_UFP_VDM_CMD);

/* DFP PD VDM Command's reactions */

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_DISCOVER_ID) = {
	VDM_CMD_ACK_STATE(CMD_DISCOVER_IDENT,
		PE_DFP_UFP_VDM_IDENTITY_ACKED),
	VDM_CMD_NACK_STATE(CMD_DISCOVER_IDENT, PE_DFP_UFP_VDM_IDENTITY_NAKED),
	VDM_CMD_BUSY_STATE(CMD_DISCOVER_IDENT, PE_DFP_UFP_VDM_IDENTITY_NAKED),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_DISCOVER_ID);

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_DISCOVER_SVID) = {
	VDM_CMD_ACK_STATE(CMD_DISCOVER_SVID,
		PE_DFP_VDM_SVIDS_ACKED),
	VDM_CMD_NACK_STATE(CMD_DISCOVER_SVID, PE_DFP_VDM_SVIDS_NAKED),
	VDM_CMD_BUSY_STATE(CMD_DISCOVER_SVID, PE_DFP_VDM_SVIDS_NAKED),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_DISCOVER_SVID);

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_DISCOVER_MODES) = {
	VDM_CMD_ACK_STATE(CMD_DISCOVER_MODES,
			PE_DFP_VDM_MODES_ACKED),
	VDM_CMD_NACK_STATE(CMD_DISCOVER_MODES, PE_DFP_VDM_MODES_NAKED),
	VDM_CMD_BUSY_STATE(CMD_DISCOVER_MODES, PE_DFP_VDM_MODES_NAKED),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_DISCOVER_MODES);

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_ENTER_MODE) = {
	VDM_CMD_ACK_STATE(CMD_ENTER_MODE,
			PE_DFP_VDM_MODE_ENTRY_ACKED),
	VDM_CMD_NACK_STATE(CMD_ENTER_MODE, PE_DFP_VDM_MODE_ENTRY_NAKED),
	VDM_CMD_BUSY_STATE(CMD_ENTER_MODE, PE_DFP_VDM_MODE_ENTRY_NAKED),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_ENTER_MODE);

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_EXIT_MODE) = {
	VDM_CMD_ACK_STATE(CMD_EXIT_MODE,
		PE_DFP_VDM_MODE_ENTRY_ACKED),
	VDM_CMD_NACK_STATE(CMD_EXIT_MODE,
		PE_DFP_VDM_MODE_ENTRY_NAKED),
	VDM_CMD_BUSY_STATE(CMD_EXIT_MODE,
		PE_VIRT_HARD_RESET),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_EXIT_MODE);

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_ATTENTION) = {
	VDM_CMD_INIT_STATE(CMD_ATTENTION,
		PE_DFP_VDM_ATTENTION_REQUEST),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_ATTENTION);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_DP_STATUS) = {
	VDM_CMD_ACK_STATE(CMD_DP_STATUS,
			PE_DFP_VDM_DP_STATUS_UPDATE_ACKED),
	VDM_CMD_NACK_STATE(CMD_DP_STATUS, PE_DFP_VDM_DP_STATUS_UPDATE_NAKED),
	VDM_CMD_BUSY_STATE(CMD_DP_STATUS, PE_DFP_VDM_DP_STATUS_UPDATE_NAKED),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_DP_STATUS);

DECL_PE_STATE_TRANSITION(PD_DFP_VDM_DP_CONFIG) = {
	VDM_CMD_ACK_STATE(CMD_DP_CONFIG,
			PE_DFP_VDM_DP_CONFIGURATION_ACKED),
	VDM_CMD_NACK_STATE(CMD_DP_CONFIG, PE_DFP_VDM_DP_CONFIGURATION_NAKED),
	VDM_CMD_BUSY_STATE(CMD_DP_CONFIG, PE_DFP_VDM_DP_CONFIGURATION_NAKED),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_DP_CONFIG);

#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */


/* HW Event reactions */

DECL_PE_STATE_TRANSITION(PD_HW_MSG_TX_FAILED) = {
#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	{PE_SRC_VDM_IDENTITY_REQUEST, PE_SRC_VDM_IDENTITY_NAKED},
#endif	/* PD_CAP_SRC_STARTUP_DISCOVERY_ID */

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
	{PE_DFP_CBL_VDM_IDENTITY_REQUEST, PE_DFP_CBL_VDM_IDENTITY_NAKED},
#endif	/* CONFIG_USB_PD_DFP_READY_DISCOVER_ID */

#ifdef CONFIG_USB_PD_UVDM
	{PE_DFP_UVDM_SEND, PE_DFP_UVDM_NAKED},
#endif	/* CONFIG_USB_PD_UVDM */
};
DECL_PE_STATE_REACTION(PD_HW_MSG_TX_FAILED);

/* DPM Event reactions */

DECL_PE_STATE_TRANSITION(PD_DPM_MSG_ACK) = {
	{ PE_UFP_VDM_GET_IDENTITY, PE_UFP_VDM_SEND_IDENTITY },
	{ PE_UFP_VDM_GET_SVIDS, PE_UFP_VDM_SEND_SVIDS },
	{ PE_UFP_VDM_GET_MODES, PE_UFP_VDM_SEND_MODES },
	{ PE_UFP_VDM_MODE_EXIT, PE_UFP_VDM_MODE_EXIT_ACK},
	{ PE_UFP_VDM_EVALUATE_MODE_ENTRY, PE_UFP_VDM_MODE_ENTRY_ACK },
};
DECL_PE_STATE_REACTION(PD_DPM_MSG_ACK);

DECL_PE_STATE_TRANSITION(PD_DPM_MSG_NAK) = {
	{PE_UFP_VDM_GET_IDENTITY, PE_UFP_VDM_GET_IDENTITY_NAK},
	{PE_UFP_VDM_GET_SVIDS, PE_UFP_VDM_GET_SVIDS_NAK},
	{PE_UFP_VDM_GET_MODES, PE_UFP_VDM_GET_MODES_NAK},
	{PE_UFP_VDM_MODE_EXIT, PE_UFP_VDM_MODE_EXIT_NAK},
	{PE_UFP_VDM_EVALUATE_MODE_ENTRY, PE_UFP_VDM_MODE_ENTRY_NAK},
};
DECL_PE_STATE_REACTION(PD_DPM_MSG_NAK);

/* Discover Cable ID */

#ifdef CONFIG_PD_DISCOVER_CABLE_ID
DECL_PE_STATE_TRANSITION(PD_DPM_MSG_DISCOVER_CABLE) = {
#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	{ PE_SRC_STARTUP, PE_SRC_VDM_IDENTITY_REQUEST},
	{ PE_SRC_DISCOVERY, PE_SRC_VDM_IDENTITY_REQUEST},
#endif

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
	{ PE_SRC_READY, PE_DFP_CBL_VDM_IDENTITY_REQUEST},
	{ PE_SNK_READY, PE_DFP_CBL_VDM_IDENTITY_REQUEST},
#endif
};
DECL_PE_STATE_REACTION(PD_DPM_MSG_DISCOVER_CABLE);
#endif

/* Source Startup Discover Cable ID */
#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
DECL_PE_STATE_TRANSITION(PD_SRC_VDM_DISCOVER_CABLE) = {
	VDM_CMD_ACK_STATE(CMD_DISCOVER_IDENT, PE_SRC_VDM_IDENTITY_ACKED),
	VDM_CMD_NACK_STATE(CMD_DISCOVER_IDENT, PE_SRC_VDM_IDENTITY_NAKED),
	VDM_CMD_BUSY_STATE(CMD_DISCOVER_IDENT, PE_SRC_VDM_IDENTITY_NAKED),
};
DECL_PE_STATE_REACTION(PD_SRC_VDM_DISCOVER_CABLE);
#endif /* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
DECL_PE_STATE_TRANSITION(PD_DFP_VDM_DISCOVER_CABLE) = {
	VDM_CMD_ACK_STATE(CMD_DISCOVER_IDENT, PE_DFP_CBL_VDM_IDENTITY_ACKED),
	VDM_CMD_NACK_STATE(CMD_DISCOVER_IDENT, PE_DFP_CBL_VDM_IDENTITY_NAKED),
	VDM_CMD_BUSY_STATE(CMD_DISCOVER_IDENT, PE_DFP_CBL_VDM_IDENTITY_NAKED),
};
DECL_PE_STATE_REACTION(PD_DFP_VDM_DISCOVER_CABLE);

#endif /* CONFIG_USB_PD_DFP_READY_DISCOVER_ID */

/* Timer Event reactions */

DECL_PE_STATE_TRANSITION(PD_TIMER_VDM_MODE_ENTRY) = {
	{ PE_DFP_VDM_MODE_ENTRY_REQUEST, PE_DFP_VDM_MODE_ENTRY_NAKED },
};
DECL_PE_STATE_REACTION(PD_TIMER_VDM_MODE_ENTRY);

DECL_PE_STATE_TRANSITION(PD_TIMER_VDM_MODE_EXIT) = {
	{ PE_DFP_VDM_MODE_EXIT_REQUEST, PE_VIRT_HARD_RESET },
};
DECL_PE_STATE_REACTION(PD_TIMER_VDM_MODE_EXIT);

DECL_PE_STATE_TRANSITION(PD_TIMER_VDM_RESPONSE) = {
	{ PE_DFP_UFP_VDM_IDENTITY_REQUEST, PE_DFP_UFP_VDM_IDENTITY_NAKED },
	{ PE_DFP_VDM_SVIDS_REQUEST, PE_DFP_VDM_SVIDS_NAKED },
	{ PE_DFP_VDM_MODES_REQUEST, PE_DFP_VDM_MODES_NAKED },

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	{ PE_SRC_VDM_IDENTITY_REQUEST, PE_SRC_VDM_IDENTITY_NAKED },
#endif

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
	{ PE_DFP_CBL_VDM_IDENTITY_REQUEST, PE_DFP_CBL_VDM_IDENTITY_NAKED },
#endif

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	{ PE_DFP_VDM_DP_STATUS_UPDATE_REQUEST,
			PE_DFP_VDM_DP_STATUS_UPDATE_NAKED },
	{ PE_DFP_VDM_DP_CONFIGURATION_REQUEST,
			PE_DFP_VDM_DP_CONFIGURATION_NAKED },
#endif
};
DECL_PE_STATE_REACTION(PD_TIMER_VDM_RESPONSE);

#ifdef CONFIG_USB_PD_UVDM
DECL_PE_STATE_TRANSITION(PD_TIMER_UVDM_RESPONSE) = {
	{ PE_DFP_UVDM_SEND, PE_DFP_UVDM_NAKED },
};
DECL_PE_STATE_REACTION(PD_TIMER_UVDM_RESPONSE);
#endif	/* CONFIG_USB_PD_UVDM */

/*
 * [BLOCK] Porcess Ctrl MSG
 */

static inline bool pd_process_ctrl_msg_good_crc(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
#ifdef CONFIG_USB_PD_ALT_MODE
#ifdef CONFIG_USB_PD_DBG_DP_UFP_U_AUTO_ATTENTION
	if (pd_port->pe_state_curr == PE_UFP_VDM_DP_CONFIGURE) {
		PE_TRANSIT_STATE(pd_port,
			PE_UFP_VDM_ATTENTION_REQUEST);
		return true;
	}
#endif	/* CONFIG_USB_PD_DBG_DP_UFP_U_AUTO_ATTENTION */
#endif	/* CONFIG_USB_PD_ALT_MODE */

	switch (pd_port->pe_state_curr) {
	case PE_UFP_VDM_SEND_IDENTITY:
	case PE_UFP_VDM_GET_IDENTITY_NAK:
	case PE_UFP_VDM_SEND_SVIDS:
	case PE_UFP_VDM_GET_SVIDS_NAK:

	case PE_UFP_VDM_SEND_MODES:
	case PE_UFP_VDM_GET_MODES_NAK:
	case PE_UFP_VDM_MODE_ENTRY_ACK:
	case PE_UFP_VDM_MODE_ENTRY_NAK:
	case PE_UFP_VDM_MODE_EXIT_ACK:
	case PE_UFP_VDM_MODE_EXIT_NAK:
		PE_TRANSIT_READY_STATE(pd_port);
		return true;
#ifdef CONFIG_USB_PD_ALT_MODE
	case PE_UFP_VDM_DP_CONFIGURE:
		PE_TRANSIT_READY_STATE(pd_port);
		return true;
	case PE_UFP_VDM_DP_STATUS_UPDATE:
		PE_TRANSIT_READY_STATE(pd_port);
		return true;
#endif

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	case PE_SRC_VDM_IDENTITY_REQUEST:
		pd_port->power_cable_present = true;
		return false;
#endif	/* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
	case PE_DFP_CBL_VDM_IDENTITY_REQUEST:
		pd_port->power_cable_present = true;
		return false;
#endif	/* CONFIG_USB_PD_DFP_READY_DISCOVER_ID */

#ifdef CONFIG_USB_PD_UVDM
	case PE_DFP_UVDM_SEND:
		if (!pd_port->uvdm_wait_resp) {
			PE_TRANSIT_STATE(pd_port, PE_DFP_UVDM_ACKED);
			return true;
		}
		break;
#endif	/* CONFIG_USB_PD_UVDM */
	default:
		break;
	}

	return false;
}

static inline bool pd_process_ctrl_msg(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool ret = false;

	switch (pd_event->msg) {
	case PD_CTRL_GOOD_CRC:
		return pd_process_ctrl_msg_good_crc(pd_port, pd_event);

	default:
		break;
	}

	return ret;
}

/*
 * [BLOCK] Porcess Data MSG (UVDM)
 */

#ifdef CONFIG_USB_PD_UVDM

static inline bool pd_process_uvdm(struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	/* support sop only */
	if (pd_event->pd_msg->frame_type != TCPC_TX_SOP)
		return false;

	if (pd_port->data_role == PD_ROLE_UFP) {

		pd_port->pe_vdm_state = pd_port->pe_pd_state;
		pd_port->pe_state_curr = pd_port->pe_pd_state;

#if PE_DBG_RESET_VDM_DIS == 0
		PE_DBG("reset vdm_state\r\n");
#endif

		if (pd_check_pe_state_ready(pd_port)) {
			PE_TRANSIT_STATE(pd_port, PE_UFP_UVDM_RECV);
			return true;
		}
	} else { /* DFP */
		if (pd_port->pe_state_curr == PE_DFP_UVDM_SEND) {
			PE_TRANSIT_STATE(pd_port, PE_DFP_UVDM_ACKED);
			return true;
		}
	}

	PE_DBG("659 : invalid, current status\r\n");
	return false;
}
#else
static inline bool pd_process_uvdm(struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	return false;
}
#endif	/* CONFIG_USB_PD_UVDM */

/*
 * [BLOCK] Porcess Data MSG (VDM)
 */

#if (PE_EVT_INFO_VDM_DIS == 0)
static const char * const pe_vdm_cmd_name[] = {
	"DiscoverID",
	"DiscoverSVID",
	"DiscoverMode",
	"EnterMode",
	"ExitMode",
	"Attention",
};

static const char *const pe_vdm_dp_cmd_name[] = {
	"DPStatus",
	"DPConfig",
};

static const char * const pe_vdm_cmd_type_name[] = {
	"INIT",
	"ACK",
	"NACK",
	"BUSY",
};

static inline const char *assign_vdm_cmd_name(uint8_t cmd)
{
	if (cmd <= ARRAY_SIZE(pe_vdm_cmd_name) && cmd != 0) {
		cmd -= 1;
		return pe_vdm_cmd_name[cmd];
	}

	return NULL;
}

#ifdef CONFIG_USB_PD_ALT_MODE
static inline const char *assign_vdm_dp_cmd_name(uint8_t cmd)
{
	if (cmd >= CMD_DP_STATUS) {
		cmd -= CMD_DP_STATUS;
		if (cmd < ARRAY_SIZE(pe_vdm_dp_cmd_name))
			return pe_vdm_dp_cmd_name[cmd];
	}

	return NULL;
}
#endif     /* CONFIG_USB_PD_ALT_MODE */

#endif /* if (PE_EVT_INFO_VDM_DIS == 0) */

static inline void print_vdm_msg(struct __pd_port *pd_port, struct __pd_event *pd_event)
{
#if (PE_EVT_INFO_VDM_DIS == 0)
	uint8_t cmd;
	uint8_t cmd_type;
	uint16_t svid;
	const char *name = NULL;
	uint32_t vdm_hdr = pd_event->pd_msg->payload[0];

	cmd = PD_VDO_CMD(vdm_hdr);
	cmd_type = PD_VDO_CMDT(vdm_hdr);
	svid = PD_VDO_VID(vdm_hdr);

	name = assign_vdm_cmd_name(cmd);

#ifdef CONFIG_USB_PD_ALT_MODE
	if (name == NULL && svid == USB_SID_DISPLAYPORT)
		name = assign_vdm_dp_cmd_name(cmd);
#endif     /* CONFIG_USB_PD_ALT_MODE */

	if (name == NULL)
		return;

	if (cmd_type >= ARRAY_SIZE(pe_vdm_cmd_type_name))
		return;

	PE_INFO("%s:%s\r\n", name, pe_vdm_cmd_type_name[cmd_type]);

#endif	/* PE_EVT_INFO_VDM_DIS */
}

static inline bool pd_process_ufp_vdm(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	if (!pd_check_pe_state_ready(pd_port)) {
		PE_DBG("659 : invalid, current status\r\n");
		return false;
	}

	if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_UFP_VDM_CMD))
		return true;

	return false;
}

static inline bool pd_process_dfp_vdm(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	uint32_t vdm_hdr = pd_event->pd_msg->payload[0];

	if ((PD_VDO_CMDT(vdm_hdr) == CMDT_INIT) &&
			PD_VDO_CMD(vdm_hdr) == CMD_ATTENTION) {

		if (!pd_check_pe_state_ready(pd_port)) {
			PE_DBG("670 : invalid, current status\r\n");
			return false;
		}

		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_ATTENTION))
			return true;
	}

	switch (pd_port->pe_state_curr) {

	case PE_DFP_UFP_VDM_IDENTITY_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_DISCOVER_ID))
			return true;

	case PE_DFP_VDM_SVIDS_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_DISCOVER_SVID))
			return true;

	case PE_DFP_VDM_MODES_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_DISCOVER_MODES))
			return true;

	case PE_DFP_VDM_MODE_ENTRY_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_ENTER_MODE))
			return true;

	case PE_DFP_VDM_MODE_EXIT_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT_VIRT(PD_DFP_VDM_EXIT_MODE))
			return true;

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	case PE_DFP_VDM_DP_STATUS_UPDATE_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_DP_STATUS))
			return true;

	case PE_DFP_VDM_DP_CONFIGURATION_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_DP_CONFIG))
			return true;
#endif
	}

	return false;
}

static inline bool pd_process_sop_vdm(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool ret = false;

	if (pd_port->data_role == PD_ROLE_UFP)
		ret = pd_process_ufp_vdm(pd_port, pd_event);
	else
		ret = pd_process_dfp_vdm(pd_port, pd_event);

	if (!ret)
		PE_DBG("Unknown VDM\r\n");

	return ret;
}

static inline bool pd_process_sop_prime_vdm(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool ret = false;

	switch (pd_port->pe_state_curr) {

#ifdef CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID
	case PE_SRC_VDM_IDENTITY_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_SRC_VDM_DISCOVER_CABLE))
			ret = true;
		break;
#endif /* CONFIG_USB_PD_SRC_STARTUP_DISCOVER_ID */

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
	case PE_DFP_CBL_VDM_IDENTITY_REQUEST:
		if (PE_MAKE_VDM_CMD_STATE_TRANSIT(PD_DFP_VDM_DISCOVER_CABLE))
			ret = true;
		break;
#endif /* CONFIG_USB_PD_DFP_READY_DISCOVER_ID */
	default:
		break;
	}

	return ret;
}

static inline bool pd_process_data_msg(
		struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool ret = false;
	uint32_t vdm_hdr;
	struct __pd_msg *pd_msg = pd_event->pd_msg;

	if (pd_event->msg != PD_DATA_VENDOR_DEF)
		return ret;

	vdm_hdr = pd_msg->payload[0];

	if (!PD_VDO_SVDM(vdm_hdr))
		return pd_process_uvdm(pd_port, pd_event);

	/* From Port Partner, copy curr_state from pd_state */
	if (PD_VDO_CMDT(vdm_hdr) == CMDT_INIT) {
		pd_port->pe_vdm_state = pd_port->pe_pd_state;
		pd_port->pe_state_curr = pd_port->pe_pd_state;

#if PE_DBG_RESET_VDM_DIS == 0
		PE_DBG("reset vdm_state\r\n");
#endif /* if PE_DBG_RESET_VDM_DIS == 0 */
	}

	print_vdm_msg(pd_port, pd_event);

	if (pd_msg->frame_type == TCPC_TX_SOP_PRIME)
		ret = pd_process_sop_prime_vdm(pd_port, pd_event);
	else
		ret = pd_process_sop_vdm(pd_port, pd_event);

	return ret;
}

/*
 * [BLOCK] Porcess PDM MSG
 */

static inline bool pd_process_dpm_msg_ack(
		struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	if (pd_port->data_role == PD_ROLE_DFP) {
		switch (pd_port->pe_state_curr) {
		case PE_DFP_UFP_VDM_IDENTITY_ACKED:
		case PE_DFP_UFP_VDM_IDENTITY_NAKED:
		case PE_DFP_CBL_VDM_IDENTITY_ACKED:
		case PE_DFP_CBL_VDM_IDENTITY_NAKED:
		case PE_DFP_VDM_SVIDS_ACKED:
		case PE_DFP_VDM_SVIDS_NAKED:
		case PE_DFP_VDM_MODES_ACKED:
		case PE_DFP_VDM_MODES_NAKED:
		case PE_DFP_VDM_MODE_ENTRY_ACKED:
		case PE_DFP_VDM_MODE_EXIT_REQUEST:
		case PE_DFP_VDM_MODE_EXIT_ACKED:
		case PE_DFP_VDM_ATTENTION_REQUEST:
			PE_TRANSIT_READY_STATE(pd_port);
			return true;
#ifdef CONFIG_USB_PD_UVDM
		case PE_DFP_UVDM_ACKED:
		case PE_DFP_UVDM_NAKED:
			PE_TRANSIT_READY_STATE(pd_port);
			return true;
#endif	/* CONFIG_USB_PD_UVDM */
		default:
			return false;
		}
	} else
		return PE_MAKE_STATE_TRANSIT(PD_DPM_MSG_ACK);
}

static inline bool pd_process_dpm_msg(
		struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool ret = false;

	switch (pd_event->msg) {
	case PD_DPM_ACK:
		ret = pd_process_dpm_msg_ack(pd_port, pd_event);
		break;

	case PD_DPM_NAK:
		ret = PE_MAKE_STATE_TRANSIT(PD_DPM_MSG_NAK);
		break;
	}

	return ret;
}

/*
 * [BLOCK] Porcess HW MSG
 */

static inline bool pd_process_hw_msg_retry_vdm(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	PE_DBG("RetryVDM\r\n");
	return pd_process_sop_vdm(pd_port, pd_event);
}

static inline bool pd_process_hw_msg(
		struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool ret = false;

	switch (pd_event->msg) {
	case PD_HW_TX_FAILED:
		ret = PE_MAKE_STATE_TRANSIT(PD_HW_MSG_TX_FAILED);
		break;

	case PD_HW_RETRY_VDM:
		ret = pd_process_hw_msg_retry_vdm(pd_port, pd_event);
		break;
	}

	return ret;
}

/*
 * [BLOCK] Porcess Timer MSG
 */

static inline bool pd_process_timer_msg(
		struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	switch (pd_event->msg) {
	case PD_TIMER_VDM_MODE_ENTRY:
		return PE_MAKE_STATE_TRANSIT(PD_TIMER_VDM_MODE_ENTRY);

	case PD_TIMER_VDM_MODE_EXIT:
		return PE_MAKE_STATE_TRANSIT(PD_TIMER_VDM_MODE_EXIT);

	case PD_TIMER_VDM_RESPONSE:
		return PE_MAKE_STATE_TRANSIT_VIRT(PD_TIMER_VDM_RESPONSE);

#ifdef CONFIG_USB_PD_UVDM
	case PD_TIMER_UVDM_RESPONSE:
		return PE_MAKE_STATE_TRANSIT(PD_TIMER_UVDM_RESPONSE);
#endif	/* CONFIG_USB_PD_UVDM */

	default:
		return false;
	}
}

/*
 * [BLOCK] Porcess TCP MSG
 */

const uint8_t tcp_vdm_evt_init_state[] = {
	PE_DFP_UFP_VDM_IDENTITY_REQUEST, /* TCP_DPM_EVT_DISCOVER_ID */
	PE_DFP_VDM_SVIDS_REQUEST, /* TCP_DPM_EVT_DISCOVER_SVIDS */
	PE_DFP_VDM_MODES_REQUEST, /* TCP_DPM_EVT_DISCOVER_MODES */
	PE_DFP_VDM_MODE_ENTRY_REQUEST, /* TCP_DPM_EVT_ENTER_MODE */
	PE_DFP_VDM_MODE_EXIT_REQUEST, /* TCP_DPM_EVT_EXIT_MODE */
	PE_UFP_VDM_ATTENTION_REQUEST, /* TCP_DPM_EVT_ATTENTION */

#ifdef CONFIG_USB_PD_ALT_MODE
	PE_UFP_VDM_ATTENTION_REQUEST, /* TCP_DPM_EVT_DP_ATTENTION */
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	PE_DFP_VDM_DP_STATUS_UPDATE_REQUEST, /* TCP_DPM_EVT_DP_STATUS_UPDATE */
	PE_DFP_VDM_DP_CONFIGURATION_REQUEST, /* TCP_DPM_EVT_DP_CONFIG */
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
	PE_DFP_UVDM_SEND, /* TCP_DPM_EVT_UVDM */
#endif	/* CONFIG_USB_PD_UVDM */
};

static inline bool pd_process_tcp_cable_event(
		struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool ret;
	int tcp_ret;

#ifdef CONFIG_PD_DISCOVER_CABLE_ID
	ret = PE_MAKE_STATE_TRANSIT(PD_DPM_MSG_DISCOVER_CABLE);
	tcp_ret = ret ? TCP_DPM_RET_SENT : TCP_DPM_RET_DENIED_NOT_READY;
#else
	ret = false;
	tcp_ret = TCP_DPM_RET_DENIED_NO_SUPPORT;
#endif	/* CONFIG_PD_DISCOVER_CABLE_ID */

	pd_notify_current_tcp_event_result(pd_port, tcp_ret);
	return ret;
}

#ifdef CONFIG_USB_PD_ALT_MODE

static inline uint32_t tcpc_update_bits(
	uint32_t var, uint32_t update, uint32_t mask)
{
	return (var & (~mask)) | (update & mask);
}

static inline void pd_parse_tcp_dpm_evt_dp_status(struct __pd_port *pd_port)
{
	struct tcp_dpm_dp_data *dp_data =
		&pd_port->tcp_event.tcp_dpm_data.dp_data;

	pd_port->mode_svid = USB_SID_DISPLAYPORT;
	pd_port->dp_status = tcpc_update_bits(
		pd_port->dp_status, dp_data->val, dp_data->mask);
}

#ifdef CONFIG_USB_PD_ALT_MODE_DFP

static inline void pd_parse_tcp_dpm_evt_dp_config(struct __pd_port *pd_port)
{
	struct tcp_dpm_dp_data *dp_data =
		&pd_port->tcp_event.tcp_dpm_data.dp_data;

	pd_port->mode_svid = USB_SID_DISPLAYPORT;
	pd_port->local_dp_config = tcpc_update_bits(
		pd_port->local_dp_config, dp_data->val, dp_data->mask);
}

#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
static inline void pd_parse_tcp_dpm_evt_uvdm(struct __pd_port *pd_port)
{
	struct tcp_dpm_uvdm_data *uvdm_data =
		&pd_port->tcp_event.tcp_dpm_data.uvdm_data;

	pd_port->uvdm_cnt = uvdm_data->cnt;
	pd_port->uvdm_wait_resp = uvdm_data->wait_resp;
	memcpy(pd_port->uvdm_data,
		uvdm_data->vdos, sizeof(uint32_t) * uvdm_data->cnt);
}
#endif	/* CONFIG_USB_PD_UVDM */

static inline void pd_parse_tcp_dpm_evt_from_tcpm(
	struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	switch (pd_event->msg) {
	case TCP_DPM_EVT_DP_ATTENTION:
		pd_parse_tcp_dpm_evt_dp_status(pd_port);
		break;

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	case TCP_DPM_EVT_DP_STATUS_UPDATE:
		pd_parse_tcp_dpm_evt_dp_status(pd_port);
		break;
	case TCP_DPM_EVT_DP_CONFIG:
		pd_parse_tcp_dpm_evt_dp_config(pd_port);
		break;
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */

#ifdef CONFIG_USB_PD_UVDM
	case TCP_DPM_EVT_UVDM:
		pd_parse_tcp_dpm_evt_uvdm(pd_port);
		break;
#endif	/* CONFIG_USB_PD_UVDM */
	}
}

static inline bool pd_process_tcp_msg(
		struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	bool is_dfp;
	bool is_attention;
	uint8_t new_state;

	if (pd_event->msg == TCP_DPM_EVT_DISCOVER_CABLE)
		return pd_process_tcp_cable_event(pd_port, pd_event);

	if (!pd_check_pe_state_ready(pd_port)) {
		pd_notify_current_tcp_event_result(
			pd_port, TCP_DPM_RET_DENIED_NOT_READY);
		PE_DBG("skip vdm_request, not ready_state (%d)\r\n",
					pd_port->pe_state_curr);
		return false;
	}

	is_dfp = pd_port->data_role == PD_ROLE_DFP;
	is_attention = pd_event->msg == TCP_DPM_EVT_ATTENTION;

	if ((is_dfp && is_attention) || (!is_dfp && !is_attention)) {
		pd_notify_current_tcp_event_result(
			pd_port, TCP_DPM_RET_DENIED_WRONG_DATA_ROLE);
		PE_DBG("skip vdm_request, WRONG DATA ROLE\r\n");
		return false;
	}

	new_state = tcp_vdm_evt_init_state[
		pd_event->msg - TCP_DPM_EVT_DISCOVER_ID];

	if (pd_event->msg_sec == PD_TCP_FROM_TCPM)
		pd_parse_tcp_dpm_evt_from_tcpm(pd_port, pd_event);

	PE_TRANSIT_STATE(pd_port, new_state);
	pd_notify_current_tcp_event_result(pd_port, TCP_DPM_RET_SENT);
	return true;
}

/*
 * [BLOCK] Process Policy Engine's VDM Message
 */

bool pd_process_event_vdm(struct __pd_port *pd_port, struct __pd_event *pd_event)
{
	switch (pd_event->event_type) {
	case PD_EVT_CTRL_MSG:
		return pd_process_ctrl_msg(pd_port, pd_event);

	case PD_EVT_DATA_MSG:
		return pd_process_data_msg(pd_port, pd_event);

	case PD_EVT_DPM_MSG:
		return pd_process_dpm_msg(pd_port, pd_event);

	case PD_EVT_HW_MSG:
		return pd_process_hw_msg(pd_port, pd_event);

	case PD_EVT_TIMER_MSG:
		return pd_process_timer_msg(pd_port, pd_event);

	case PD_EVT_TCP_MSG:
		return pd_process_tcp_msg(pd_port, pd_event);
	}

	return false;
}
