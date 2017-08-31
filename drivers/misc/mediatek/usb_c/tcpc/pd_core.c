/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Core Driver
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

#include <linux/of.h>
#include <linux/slab.h>

#include "inc/tcpci.h"
#include "inc/pd_core.h"
#include "inc/pd_dpm_core.h"
#include "inc/tcpci_typec.h"
#include "inc/tcpci_event.h"
#include "inc/pd_policy_engine.h"

#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/usb/class-dual-role.h>
#endif /* CONFIG_DUAL_ROLE_USB_INTF */

/* From DTS */

static int pd_parse_pdata(struct pd_port *pd_port)
{
	u32 val;
	struct device_node *np;
	int ret = 0, i;

	pr_info("%s\n", __func__);
	np = of_find_node_by_name(pd_port->tcpc_dev->dev.of_node, "pd-data");

	if (np) {
		ret = of_property_read_u32(np, "pd,source-pdo-size",
				(u32 *)&pd_port->local_src_cap_default.nr);
		if (ret < 0)
			pr_err("%s get source pdo size fail\n", __func__);

		ret = of_property_read_u32_array(np, "pd,source-pdo-data",
			(u32 *)pd_port->local_src_cap_default.pdos,
			pd_port->local_src_cap_default.nr);
		if (ret < 0)
			pr_err("%s get source pdo data fail\n", __func__);

		pr_info("%s src pdo data =\n", __func__);
		for (i = 0; i < pd_port->local_src_cap_default.nr; i++) {
			pr_info("%s %d: 0x%08x\n", __func__, i,
				pd_port->local_src_cap_default.pdos[i]);
		}

		ret = of_property_read_u32(np, "pd,sink-pdo-size",
					(u32 *)&pd_port->local_snk_cap.nr);
		if (ret < 0)
			pr_err("%s get sink pdo size fail\n", __func__);

		ret = of_property_read_u32_array(np, "pd,sink-pdo-data",
			(u32 *)pd_port->local_snk_cap.pdos,
				pd_port->local_snk_cap.nr);
		if (ret < 0)
			pr_err("%s get sink pdo data fail\n", __func__);

		pr_info("%s snk pdo data =\n", __func__);
		for (i = 0; i < pd_port->local_snk_cap.nr; i++) {
			pr_info("%s %d: 0x%08x\n", __func__, i,
				pd_port->local_snk_cap.pdos[i]);
		}

		ret = of_property_read_u32(np, "pd,id-vdo-size",
					(u32 *)&pd_port->id_vdo_nr);
		if (ret < 0)
			pr_err("%s get id vdo size fail\n", __func__);
		ret = of_property_read_u32_array(np, "pd,id-vdo-data",
			(u32 *)pd_port->id_vdos, pd_port->id_vdo_nr);
		if (ret < 0)
			pr_err("%s get id vdo data fail\n", __func__);

		pr_info("%s id vdos data =\n", __func__);
		for (i = 0; i < pd_port->id_vdo_nr; i++)
			pr_info("%s %d: 0x%08x\n", __func__, i,
			pd_port->id_vdos[i]);

		val = DPM_CHARGING_POLICY_MAX_POWER_LVIC;
		if (of_property_read_u32(np, "pd,charging_policy", &val) < 0)
			pr_info("%s get charging policy fail\n", __func__);

		pd_port->dpm_charging_policy = val;
		pd_port->dpm_charging_policy_default = val;
		pr_info("%s charging_policy = %d\n", __func__, val);
	}

	return 0;
}

static const struct {
	const char *prop_name;
	uint32_t val;
} supported_dpm_caps[] = {
	{"local_dr_power", DPM_CAP_LOCAL_DR_POWER},
	{"local_dr_data", DPM_CAP_LOCAL_DR_DATA},
	{"local_ext_power", DPM_CAP_LOCAL_EXT_POWER},
	{"local_usb_comm", DPM_CAP_LOCAL_USB_COMM},
	{"local_usb_suspend", DPM_CAP_LOCAL_USB_SUSPEND},
	{"local_high_cap", DPM_CAP_LOCAL_HIGH_CAP},
	{"local_give_back", DPM_CAP_LOCAL_GIVE_BACK},
	{"local_no_suspend", DPM_CAP_LOCAL_NO_SUSPEND},
	{"local_vconn_supply", DPM_CAP_LOCAL_VCONN_SUPPLY},

	{"attemp_discover_cable_dfp", DPM_CAP_ATTEMP_DISCOVER_CABLE_DFP},
	{"attemp_enter_dp_mode", DPM_CAP_ATTEMP_ENTER_DP_MODE},
	{"attemp_discover_cable", DPM_CAP_ATTEMP_DISCOVER_CABLE},
	{"attemp_discover_id", DPM_CAP_ATTEMP_DISCOVER_ID},

	{"pr_reject_as_source", DPM_CAP_PR_SWAP_REJECT_AS_SRC},
	{"pr_reject_as_sink", DPM_CAP_PR_SWAP_REJECT_AS_SNK},
	{"pr_check_gp_source", DPM_CAP_PR_SWAP_CHECK_GP_SRC},
	{"pr_check_gp_sink", DPM_CAP_PR_SWAP_CHECK_GP_SNK},

	{"dr_reject_as_dfp", DPM_CAP_DR_SWAP_REJECT_AS_DFP},
	{"dr_reject_as_ufp", DPM_CAP_DR_SWAP_REJECT_AS_UFP},
};

static void pd_core_power_flags_init(struct pd_port *pd_port)
{
	uint32_t src_flag, snk_flag, val;
	struct device_node *np;
	int i;
	struct pd_port_power_capabilities *snk_cap = &pd_port->local_snk_cap;
	struct pd_port_power_capabilities *src_cap =
				&pd_port->local_src_cap_default;

	np = of_find_node_by_name(pd_port->tcpc_dev->dev.of_node, "dpm_caps");

	for (i = 0; i < ARRAY_SIZE(supported_dpm_caps); i++) {
		if (of_property_read_bool(np,
			supported_dpm_caps[i].prop_name))
			pd_port->dpm_caps |=
				supported_dpm_caps[i].val;
			pr_info("dpm_caps: %s\n",
				supported_dpm_caps[i].prop_name);
	}

	if (of_property_read_u32(np, "pr_check", &val) == 0)
		pd_port->dpm_caps |= DPM_CAP_PR_CHECK_PROP(val);
	else
		pr_err("%s get pr_check data fail\n", __func__);

	if (of_property_read_u32(np, "dr_check", &val) == 0)
		pd_port->dpm_caps |= DPM_CAP_DR_CHECK_PROP(val);
	else
		pr_err("%s get dr_check data fail\n", __func__);

	pr_info("dpm_caps = 0x%08x\n", pd_port->dpm_caps);

	src_flag = 0;
	if (pd_port->dpm_caps & DPM_CAP_LOCAL_DR_POWER)
		src_flag |= PDO_FIXED_DUAL_ROLE;

	if (pd_port->dpm_caps & DPM_CAP_LOCAL_DR_DATA)
		src_flag |= PDO_FIXED_DATA_SWAP;

	if (pd_port->dpm_caps & DPM_CAP_LOCAL_EXT_POWER)
		src_flag |= PDO_FIXED_EXTERNAL;

	if (pd_port->dpm_caps & DPM_CAP_LOCAL_USB_COMM)
		src_flag |= PDO_FIXED_COMM_CAP;

	if (pd_port->dpm_caps & DPM_CAP_LOCAL_USB_SUSPEND)
		src_flag |= PDO_FIXED_SUSPEND;

	snk_flag = src_flag;
	if (pd_port->dpm_caps & DPM_CAP_LOCAL_HIGH_CAP)
		snk_flag |= PDO_FIXED_HIGH_CAP;

	snk_cap->pdos[0] |= snk_flag;
	src_cap->pdos[0] |= src_flag;
}

int pd_core_init(struct tcpc_device *tcpc_dev)
{
	int ret;
	struct pd_port *pd_port = &tcpc_dev->pd_port;

	mutex_init(&pd_port->pd_lock);

#ifdef CONFIG_USB_PD_BLOCK_TCPM
	mutex_init(&pd_port->tcpm_bk_lock);
	init_waitqueue_head(&pd_port->tcpm_bk_wait_que);
#endif	/* CONFIG_USB_PD_BLOCK_TCPM */

	pd_port->tcpc_dev = tcpc_dev;

	pd_port->pe_pd_state = PE_IDLE2;
	pd_port->pe_vdm_state = PE_IDLE2;

	pd_port->pd_connect_state = PD_CONNECT_NONE;

	ret = pd_parse_pdata(pd_port);
	if (ret)
		return ret;

	pd_core_power_flags_init(pd_port);

	pd_dpm_core_init(pd_port);

	PE_INFO("pd_core_init\r\n");
	return 0;
}

void pd_extract_rdo_power(uint32_t rdo, uint32_t pdo,
			uint32_t *op_curr, uint32_t *max_curr)
{
	uint32_t op_power, max_power, vmin;

	switch (pdo & PDO_TYPE_MASK) {
	case PDO_TYPE_FIXED:
	case PDO_TYPE_VARIABLE:
		*op_curr = RDO_FIXED_VAR_EXTRACT_OP_CURR(rdo);
		*max_curr = RDO_FIXED_VAR_EXTRACT_MAX_CURR(rdo);
		break;

	case PDO_TYPE_BATTERY: /* TODO: check it later !! */
		vmin = PDO_BATT_EXTRACT_MIN_VOLT(pdo);
		op_power = RDO_BATT_EXTRACT_OP_POWER(rdo);
		max_power = RDO_BATT_EXTRACT_MAX_POWER(rdo);

		*op_curr = op_power / vmin;
		*max_curr = max_power / vmin;
		break;

	default:
		*op_curr = *max_curr = 0;
		break;
	}
}

uint32_t pd_reset_pdo_power(uint32_t pdo, uint32_t imax)
{
	uint32_t ioper;

	switch (pdo & PDO_TYPE_MASK) {
	case PDO_TYPE_FIXED:
		ioper = PDO_FIXED_EXTRACT_CURR(pdo);
		if (ioper > imax)
			return PDO_FIXED_RESET_CURR(pdo, imax);
		break;

	case PDO_TYPE_VARIABLE:
		ioper = PDO_VAR_EXTRACT_CURR(pdo);
		if (ioper > imax)
			return PDO_VAR_RESET_CURR(pdo, imax);
		break;

	case PDO_TYPE_BATTERY:
		/* TODO: check it later !! */
		PD_ERR("No Support\r\n");
		break;
	}
	return pdo;
}

uint32_t pd_get_cable_curr_lvl(struct pd_port *pd_port)
{
	return PD_VDO_CABLE_CURR(
		pd_port->cable_vdos[VDO_DISCOVER_ID_CABLE]);
}

uint32_t pd_get_cable_current_limit(struct pd_port *pd_port)
{
	switch (pd_get_cable_curr_lvl(pd_port)) {
	case CABLE_CURR_1A5:
		return 1500;
	case CABLE_CURR_5A:
		return 5000;
	default:
	case CABLE_CURR_3A:
		return 3000;
	}
}

void pd_reset_svid_data(struct pd_port *pd_port)
{
	uint8_t i;
	struct svdm_svid_data *svid_data;

	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		svid_data->exist = false;
		svid_data->remote_mode.mode_cnt = 0;
		svid_data->active_mode = 0;
	}
}

int pd_reset_protocol_layer(struct pd_port *pd_port, bool sop_only)
{
	int i = 0;
	int max = sop_only ? 1 : PD_SOP_NR;

	pd_notify_pe_reset_protocol(pd_port);

	pd_port->explicit_contract = 0;
	pd_port->local_selected_cap = 0;
	pd_port->remote_selected_cap = 0;
	pd_port->during_swap = 0;
	pd_port->dpm_ack_immediately = 0;

#ifdef CONFIG_USB_PD_DFP_FLOW_DELAY
#ifdef CONFIG_USB_PD_DFP_FLOW_DELAY_RESET
	if (pd_port->pd_prev_connected)
		pd_port->dpm_dfp_flow_delay_done = 0;
#endif	/* CONFIG_USB_PD_DFP_FLOW_DELAY_RESET */
#endif	/* CONFIG_USB_PD_DFP_FLOW_DELAY */

#ifdef CONFIG_USB_PD_DFP_READY_DISCOVER_ID
	pd_port->vconn_return = false;
#endif	/* CONFIG_USB_PD_DFP_READY_DISCOVER_ID */

	for (i = 0; i < max; i++) {
		pd_port->msg_id_tx[i] = 0;
		pd_port->msg_id_rx[i] = 0;
		pd_port->msg_id_rx_init[i] = false;
	}

#ifdef CONFIG_USB_PD_IGNORE_PS_RDY_AFTER_PR_SWAP
	pd_port->msg_id_pr_swap_last = 0xff;
#endif	/* CONFIG_USB_PD_IGNORE_PS_RDY_AFTER_PR_SWAP */

	return 0;
}

int pd_set_rx_enable(struct pd_port *pd_port, uint8_t enable)
{
	return tcpci_set_rx_enable(pd_port->tcpc_dev, enable);
}

int pd_enable_vbus_valid_detection(struct pd_port *pd_port, bool wait_valid)
{
	PE_DBG("WaitVBUS=%d\r\n", wait_valid);
	pd_notify_pe_wait_vbus_once(pd_port,
		wait_valid ? PD_WAIT_VBUS_VALID_ONCE :
					PD_WAIT_VBUS_INVALID_ONCE);
	return 0;
}

int pd_enable_vbus_safe0v_detection(struct pd_port *pd_port)
{
	PE_DBG("WaitVSafe0V\r\n");
	pd_notify_pe_wait_vbus_once(pd_port, PD_WAIT_VBUS_SAFE0V_ONCE);
	return 0;
}

int pd_enable_vbus_stable_detection(struct pd_port *pd_port)
{
	PE_DBG("WaitVStable\r\n");
	pd_notify_pe_wait_vbus_once(pd_port, PD_WAIT_VBUS_STABLE_ONCE);
	return 0;
}

static inline int pd_update_msg_header(struct pd_port *pd_port)
{
#ifdef CONFIG_USB_PD_REV30
	uint8_t pd_rev = pd_port->pd_revision[0];
#else
	uint8_t pd_rev = PD_REV20;
#endif	/* CONFIG_USB_PD_REV30 */

	return tcpci_set_msg_header(pd_port->tcpc_dev,
			pd_port->power_role, pd_port->data_role, pd_rev);
}

int pd_set_data_role(struct pd_port *pd_port, uint8_t dr)
{
	pd_port->data_role = dr;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	/* dual role usb--> 0:ufp, 1:dfp */
	pd_port->tcpc_dev->dual_role_mode = pd_port->data_role;
	/* dual role usb --> 0: Device, 1: Host */
	pd_port->tcpc_dev->dual_role_dr = !(pd_port->data_role);
	dual_role_instance_changed(pd_port->tcpc_dev->dr_usb);
#endif /* CONFIG_DUAL_ROLE_USB_INTF */

	tcpci_notify_role_swap(pd_port->tcpc_dev, TCP_NOTIFY_DR_SWAP, dr);
	return pd_update_msg_header(pd_port);
}

int pd_set_power_role(struct pd_port *pd_port, uint8_t pr)
{
	int ret;

	pd_port->power_role = pr;
	ret = pd_update_msg_header(pd_port);
	if (ret)
		return ret;

	pd_notify_pe_pr_changed(pd_port);

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	/* 0:sink, 1: source */
	pd_port->tcpc_dev->dual_role_pr = !(pd_port->power_role);
	dual_role_instance_changed(pd_port->tcpc_dev->dr_usb);
#endif /* CONFIG_DUAL_ROLE_USB_INTF */

	tcpci_notify_role_swap(pd_port->tcpc_dev, TCP_NOTIFY_PR_SWAP, pr);
	return ret;
}

int pd_init_message_hdr(struct pd_port *pd_port, bool act_as_sink)
{
	if (act_as_sink) {
		pd_port->power_role = PD_ROLE_SINK;
		pd_port->data_role = PD_ROLE_UFP;
		pd_port->vconn_source = PD_ROLE_VCONN_OFF;
	} else {
		pd_port->power_role = PD_ROLE_SOURCE;
		pd_port->data_role = PD_ROLE_DFP;
		pd_port->vconn_source = PD_ROLE_VCONN_ON;
	}

#ifdef CONFIG_USB_PD_REV30
	if (pd_port->tcpc_dev->tcpc_flags & TCPC_FLAGS_PD_REV30) {
		pd_port->pd_revision[0] = PD_REV30;
		pd_port->pd_revision[1] = PD_REV30;
	} else {
		pd_port->pd_revision[0] = PD_REV20;
		pd_port->pd_revision[1] = PD_REV20;
	}
#endif	/* CONFIG_USB_PD_REV30 */

	return pd_update_msg_header(pd_port);
}

int pd_set_vconn(struct pd_port *pd_port, int enable)
{
	pd_port->vconn_source = enable;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	pd_port->tcpc_dev->dual_role_vconn = pd_port->vconn_source;
	dual_role_instance_changed(pd_port->tcpc_dev->dr_usb);
#endif /* CONFIG_DUAL_ROLE_USB_INTF */

	tcpci_notify_role_swap(pd_port->tcpc_dev,
				TCP_NOTIFY_VCONN_SWAP, enable);
	return tcpci_set_vconn(pd_port->tcpc_dev, enable);
}

static inline int pd_reset_modal_operation(struct pd_port *pd_port)
{
	uint8_t i;
	struct svdm_svid_data *svid_data;

	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];

		if (svid_data->active_mode) {
			svid_data->active_mode = 0;
			tcpci_exit_mode(pd_port->tcpc_dev, svid_data->svid);
		}
	}

	pd_port->svdm_ready = false;
	pd_port->modal_operation = false;
	return 0;
}

int pd_reset_local_hw(struct pd_port *pd_port)
{
	pd_notify_pe_transit_to_default(pd_port);
	pd_unlock_msg_output(pd_port);

	pd_reset_pe_timer(pd_port);
	pd_set_rx_enable(pd_port, PD_RX_CAP_PE_HARDRESET);

	pd_port->explicit_contract = false;
	pd_port->pd_connected  = false;
	pd_port->pe_ready = false;
	pd_port->dpm_ack_immediately = false;

#ifdef CONFIG_USB_PD_RESET_CABLE
	pd_port->reset_cable = false;
#endif	/* CONFIG_USB_PD_RESET_CABLE */

	pd_reset_modal_operation(pd_port);

	pd_set_vconn(pd_port, false);

	if (pd_port->power_role == PD_ROLE_SINK) {
		pd_port->state_machine = PE_STATE_MACHINE_SINK;
		pd_set_data_role(pd_port, PD_ROLE_UFP);
	} else {
		pd_port->state_machine = PE_STATE_MACHINE_SOURCE;
		pd_set_data_role(pd_port, PD_ROLE_DFP);
	}

	pd_dpm_notify_pe_hardreset(pd_port);
	PE_DBG("reset_local_hw\r\n");

	return 0;
}

int pd_enable_bist_test_mode(struct pd_port *pd_port, bool en)
{
	PE_DBG("bist_test_mode=%d\r\n", en);
	return tcpci_set_bist_test_mode(pd_port->tcpc_dev, en);
}

/* ---- Handle PD Message ----*/

int pd_handle_soft_reset(struct pd_port *pd_port, uint8_t state_machine)
{
	pd_port->state_machine = state_machine;

	pd_reset_protocol_layer(pd_port, true);
	pd_notify_tcp_event_buf_reset(pd_port, TCP_DPM_RET_DROP_RECV_SRESET);
	return pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_ACCEPT);
}

/* ---- Send PD Message ----*/

static int pd_send_message(struct pd_port *pd_port, uint8_t sop_type,
		uint8_t msg, bool ext, uint16_t count, const uint32_t *data)
{
	int ret;
	uint16_t msg_hdr;
	uint8_t pd_rev = PD_REV20;
	uint8_t type = PD_TX_STATE_WAIT_CRC_PD;
	struct tcpc_device *tcpc_dev = pd_port->tcpc_dev;

	if (tcpc_dev->typec_attach_old == 0) {
		PE_DBG("[SendMsg] Unattached\r\n");
		return 0;
	}

	if (tcpc_dev->pd_hard_reset_event_pending) {
		PE_DBG("[SendMsg] HardReset Pending");
		return 0;
	}

	if (sop_type == TCPC_TX_SOP) {
#ifdef CONFIG_USB_PD_REV30
		pd_rev = pd_port->pd_revision[0];
#endif	/* CONFIG_USB_PD_REV30 */
		msg_hdr = PD_HEADER_SOP(msg, pd_rev,
				pd_port->power_role, pd_port->data_role,
				pd_port->msg_id_tx[sop_type], count, ext);
	} else {
#ifdef CONFIG_USB_PD_REV30
		pd_rev = pd_port->pd_revision[1];
#endif	/* CONFIG_USB_PD_REV30 */
		msg_hdr = PD_HEADER_SOP_PRIME(msg, pd_rev,
				0, pd_port->msg_id_tx[sop_type], count, ext);
	}

	if ((count > 0) && (msg == PD_DATA_VENDOR_DEF))
		type = PD_TX_STATE_WAIT_CRC_VDM;

	pd_port->msg_id_tx[sop_type] = (pd_port->msg_id_tx[sop_type] + 1) % 8;

	pd_notify_pe_transmit_msg(pd_port, type);
	ret = tcpci_transmit(pd_port->tcpc_dev, sop_type, msg_hdr, data);
	if (ret < 0)
		PD_ERR("[SendMsg] Failed, %d\r\n", ret);

	return ret;
}

int pd_send_ctrl_msg(struct pd_port *pd_port, uint8_t sop_type, uint8_t msg)
{
	return pd_send_message(pd_port, sop_type, msg, false, 0, NULL);
}

int pd_send_data_msg(struct pd_port *pd_port,
		uint8_t sop_type, uint8_t msg, uint8_t cnt, uint32_t *payload)
{
	return pd_send_message(pd_port, sop_type, msg, false, cnt, payload);
}

#ifdef CONFIG_USB_PD_REV30
int pd_send_ext_msg(struct pd_port *pd_port,
		uint8_t sop_type, uint8_t msg, bool request,
		uint8_t chunk_nr, uint8_t size, uint8_t *data)
{
	uint8_t cnt;
	uint32_t payload[PD_DATA_OBJ_SIZE];

	uint16_t *ext_hdr = (uint16_t *)payload;

	*ext_hdr = PD_EXT_HEADER_CK(size, request, chunk_nr, true);
	memcpy(&payload[1], data, size);

	cnt = ((size + PD_EXT_HEADER_PAYLOAD_INDEX - 1) / 4) + 1;
	return pd_send_message(pd_port, sop_type, msg, true, cnt, payload);
}

int pd_send_status(struct pd_port *pd_port)
{
	return pd_send_ext_msg(pd_port, TCPC_TX_SOP, PD_EXT_STATUS,
			false, 0, PD_SDB_SIZE, pd_port->local_status);
}

#endif	/* CONFIG_USB_PD_REV30 */

#ifdef CONFIG_USB_PD_RESET_CABLE
int pd_send_cable_soft_reset(struct pd_port *pd_port)
{
	/* reset_protocol_layer */

	pd_port->msg_id_tx[TCPC_TX_SOP_PRIME] = 0;
	pd_port->msg_id_rx[TCPC_TX_SOP_PRIME] = 0;
	pd_port->msg_id_rx_init[TCPC_TX_SOP_PRIME] = false;

	pd_port->reset_cable = false;

	return pd_send_ctrl_msg(pd_port, TCPC_TX_SOP_PRIME, PD_CTRL_SOFT_RESET);
}
#endif	/* CONFIG_USB_PD_RESET_CABLE */

int pd_send_soft_reset(struct pd_port *pd_port, uint8_t state_machine)
{
	pd_port->state_machine = state_machine;

	pd_reset_protocol_layer(pd_port, true);
	pd_notify_tcp_event_buf_reset(pd_port, TCP_DPM_RET_DROP_SENT_SRESET);
	return pd_send_ctrl_msg(pd_port, TCPC_TX_SOP, PD_CTRL_SOFT_RESET);
}

int pd_send_hard_reset(struct pd_port *pd_port)
{
	int ret;
	struct tcpc_device *tcpc_dev = pd_port->tcpc_dev;

	PE_DBG("Send HARD Reset\r\n");

	pd_port->hard_reset_counter++;
	pd_notify_pe_send_hard_reset(pd_port);
	ret = tcpci_transmit(tcpc_dev, TCPC_TX_HARD_RESET, 0, NULL);
	if (ret)
		return ret;

#ifdef CONFIG_USB_PD_IGNORE_HRESET_COMPLETE_TIMER
	if (!(tcpc_dev->tcpc_flags & TCPC_FLAGS_WAIT_HRESET_COMPLETE)) {
		pd_put_sent_hard_reset_event(tcpc_dev);
		return 0;
	}
#endif	/* CONFIG_USB_PD_IGNORE_HRESET_COMPLETE_TIMER */

	return 0;
}

int pd_send_bist_mode2(struct pd_port *pd_port)
{
	int ret = 0;

	pd_notify_tcp_event_buf_reset(pd_port, TCP_DPM_RET_DROP_SEND_BIST);

#ifdef CONFIG_USB_PD_TRANSMIT_BIST2
	TCPC_DBG("BIST_MODE_2\r\n");
	ret = tcpci_transmit(
		pd_port->tcpc_dev, TCPC_TX_BIST_MODE_2, 0, NULL);
#else
	ret = tcpci_set_bist_carrier_mode(
		pd_port->tcpc_dev, 1 << 2);
#endif

	return ret;
}

int pd_disable_bist_mode2(struct pd_port *pd_port)
{
#ifndef CONFIG_USB_PD_TRANSMIT_BIST2
	return tcpci_set_bist_carrier_mode(
		pd_port->tcpc_dev, 0);
#else
	return 0;
#endif
}

/* ---- Send / Reply VDM Command ----*/

int pd_send_svdm_request(struct pd_port *pd_port,
		uint8_t sop_type, uint16_t svid, uint8_t vdm_cmd,
		uint8_t obj_pos, uint8_t cnt, uint32_t *data_obj,
		uint32_t timer_id)
{
#ifdef CONFIG_USB_PD_STOP_SEND_VDM_IF_RX_BUSY
	int rv;
	uint32_t alert_status;
#endif	/* CONFIG_USB_PD_STOP_SEND_VDM_IF_RX_BUSY */

	int ret;
	uint32_t payload[PD_DATA_OBJ_SIZE];

	PD_BUG_ON(cnt > VDO_MAX_NR);

	payload[0] = VDO_S(svid, CMDT_INIT, vdm_cmd, obj_pos);
	memcpy(&payload[1], data_obj, sizeof(uint32_t) * cnt);

#ifdef CONFIG_USB_PD_STOP_SEND_VDM_IF_RX_BUSY
	rv = tcpci_get_alert_status(pd_port->tcpc_dev, &alert_status);
	if (rv)
		return rv;

	if (alert_status & TCPC_REG_ALERT_RX_STATUS) {
		PE_DBG("RX Busy, stop send VDM\r\n");
		return 0;
	}
#endif	/* CONFIG_USB_PD_STOP_SEND_VDM_IF_RX_BUSY */

	ret = pd_send_data_msg(
			pd_port, sop_type, PD_DATA_VENDOR_DEF, 1+cnt, payload);

	if (ret == 0 && timer_id != 0)
		pd_enable_timer(pd_port, timer_id);

	return ret;
}

int pd_reply_svdm_request(struct pd_port *pd_port, struct pd_event *pd_event,
				uint8_t reply, uint8_t cnt, uint32_t *data_obj)
{
#ifdef CONFIG_USB_PD_STOP_REPLY_VDM_IF_RX_BUSY
	int rv;
	uint32_t alert_status;
#endif	/* CONFIG_USB_PD_STOP_REPLY_VDM_IF_RX_BUSY */

	uint32_t vdo;
	uint32_t payload[PD_DATA_OBJ_SIZE];

	PD_BUG_ON(cnt > VDO_MAX_NR);
	PD_BUG_ON(pd_event->pd_msg == NULL);

	vdo = pd_event->pd_msg->payload[0];
	payload[0] = VDO_S(
		PD_VDO_VID(vdo), reply, PD_VDO_CMD(vdo), PD_VDO_OPOS(vdo));

	if (cnt > 0) {
		PD_BUG_ON(data_obj == NULL);
		memcpy(&payload[1], data_obj, sizeof(uint32_t) * cnt);
	}

#ifdef CONFIG_USB_PD_STOP_REPLY_VDM_IF_RX_BUSY
	rv = tcpci_get_alert_status(pd_port->tcpc_dev, &alert_status);
	if (rv)
		return rv;

	if (alert_status & TCPC_REG_ALERT_RX_STATUS) {
		PE_DBG("RX Busy, stop reply VDM\r\n");
		return 0;
	}
#endif	/* CONFIG_USB_PD_STOP_REPLY_VDM_IF_RX_BUSY */

	return pd_send_data_msg(pd_port,
			TCPC_TX_SOP, PD_DATA_VENDOR_DEF, 1+cnt, payload);
}

void pd_lock_msg_output(struct pd_port *pd_port)
{
	if (pd_port->msg_output_lock)
		return;
	pd_port->msg_output_lock = true;

	pd_dbg_info_lock();
}

void pd_unlock_msg_output(struct pd_port *pd_port)
{
	if (!pd_port->msg_output_lock)
		return;
	pd_port->msg_output_lock = false;

	pd_dbg_info_unlock();
}

int pd_update_connect_state(struct pd_port *pd_port, uint8_t state)
{
	if (pd_port->pd_connect_state == state)
		return 0;

	switch (state) {
	case PD_CONNECT_TYPEC_ONLY:
		if (pd_port->power_role == PD_ROLE_SOURCE)
			state = PD_CONNECT_TYPEC_ONLY_SRC;
		else {
			switch (pd_port->tcpc_dev->typec_remote_rp_level) {
			case TYPEC_CC_VOLT_SNK_DFT:
				state = PD_CONNECT_TYPEC_ONLY_SNK_DFT;
				break;

			case TYPEC_CC_VOLT_SNK_1_5:
			case TYPEC_CC_VOLT_SNK_3_0:
				state = PD_CONNECT_TYPEC_ONLY_SNK;
				break;
			}
		}
		break;

	case PD_CONNECT_PE_READY:
		state = pd_port->power_role == PD_ROLE_SOURCE ?
			PD_CONNECT_PE_READY_SRC : PD_CONNECT_PE_READY_SNK;
		break;

	case PD_CONNECT_NONE:
		break;
	}

	pd_port->pd_connect_state = state;
	return tcpci_notify_pd_state(pd_port->tcpc_dev, state);
}

#ifdef CONFIG_USB_PD_REV30
#ifndef MIN
#define MIN(a, b)       ((a < b) ? (a) : (b))
#endif
void pd_sync_sop_spec_revision(struct pd_port *pd_port, uint8_t rev)
{
	if (pd_port->tcpc_dev->tcpc_flags & TCPC_FLAGS_PD_REV30) {
		pd_port->pd_revision[0] = MIN(PD_REV30, rev);
		pd_port->pd_revision[1] = MIN(pd_port->pd_revision[1], rev);
		pd_update_msg_header(pd_port);
	}
}

void pd_sync_sop_prime_spec_revision(struct pd_port *pd_port, uint8_t rev)
{
	if (pd_port->tcpc_dev->tcpc_flags & TCPC_FLAGS_PD_REV30)
		pd_port->pd_revision[1] = MIN(pd_port->pd_revision[1], rev);
}
#endif	/* CONFIG_USB_PD_REV30 */
