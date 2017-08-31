/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Blocking Power Delivery Managert Driver
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
#include <linux/sched.h>
#include "inc/tcpm.h"

#ifdef CONFIG_USB_POWER_DELIVERY
#include "inc/pd_core.h"
#include "inc/pd_dpm_core.h"
#include "inc/pd_policy_engine.h"
#endif	/* CONFIG_USB_POWER_DELIVERY */

#ifdef CONFIG_USB_POWER_DELIVERY
#ifdef CONFIG_USB_PD_BLOCK_TCPM

static const char * const bk_event_ret_name[] = {
	"OK",
	"Unknown",
	"NotReady",
	"NoSupport",
	"LocalCap",
	"PartnerCap",
	"SameRole",
	"InvalidReq",
	"WrongDR",
	"Detach",
	"SReset0",
	"SReset1",
	"HReset0",
	"HReset1",
	"Recovery",
	"BIST",
	"Wait",
	"Reject",
	"TOUT",
	"NAK"
};

int tcpm_dpm_bk_event_cb(
	struct tcpc_device *tcpc, int ret, struct tcp_dpm_event *event)
{
	struct pd_port *pd_port = &tcpc->pd_port;

	pd_port->tcpm_bk_ret = ret;
	pd_port->tcpm_bk_done = true;
	wake_up_interruptible(&pd_port->tcpm_bk_wait_que);

	if (ret < TCP_DPM_RET_BK_TIMEOUT)
		TCPM_DBG("bk_event_cb -> %s\r\n", bk_event_ret_name[ret]);

	return 0;
}

int tcpm_dpm_wait_bk_event(struct pd_port *pd_port, uint32_t tout_ms)
{
	wait_event_interruptible_timeout(pd_port->tcpm_bk_wait_que,
				pd_port->tcpm_bk_done,
				msecs_to_jiffies(tout_ms));

	if (pd_port->tcpm_bk_done)
		return pd_port->tcpm_bk_ret;

	TCPM_DBG("tcpm_dpm_wait_bk_event: timeout\r\n");
	return TCP_DPM_RET_BK_TIMEOUT;
}

static int tcpm_put_tcp_dpm_event_bk(
	struct tcpc_device *tcpc, uint32_t tout_ms, struct tcp_dpm_event *event)
{
	int ret;
	struct pd_port *pd_port = &tcpc->pd_port;

	mutex_lock(&pd_port->tcpm_bk_lock);

	pd_port->tcpm_bk_done = false;

	ret =  tcpm_put_tcp_dpm_event(tcpc, event);
	if (ret != TCPM_SUCCESS) {
		mutex_unlock(&pd_port->tcpm_bk_lock);
		return ret;
	}

	ret = tcpm_dpm_wait_bk_event(pd_port, tout_ms);
	mutex_unlock(&pd_port->tcpm_bk_lock);
	return ret;
}

int tcpm_dpm_pd_power_swap_bk(struct tcpc_device *tcpc, uint8_t role)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_PR_SWAP_AS_SNK + role,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 2000, &tcp_event);
}

int tcpm_dpm_pd_data_swap_bk(struct tcpc_device *tcpc, uint8_t role)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_DR_SWAP_AS_UFP + role,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_vconn_swap_bk(struct tcpc_device *tcpc, uint8_t role)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_VCONN_SWAP_OFF + role,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_goto_min_bk(struct tcpc_device *tcpc)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_GOTOMIN,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_soft_reset_bk(struct tcpc_device *tcpc)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_SOFTRESET,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_get_source_cap_bk(struct tcpc_device *tcpc)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_GET_SOURCE_CAP,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_get_sink_cap_bk(struct tcpc_device *tcpc)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_GET_SINK_CAP,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_request_bk(struct tcpc_device *tcpc, int mv, int ma)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_REQUEST,
		.event_cb = tcpm_dpm_bk_event_cb,

		.tcp_dpm_data.pd_req.mv = mv,
		.tcp_dpm_data.pd_req.ma = ma,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_request_ex_bk(struct tcpc_device *tcpc,
	uint8_t pos, uint32_t max, uint32_t oper)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_REQUEST_EX,
		.event_cb = tcpm_dpm_bk_event_cb,

		.tcp_dpm_data.pd_req_ex.pos = pos,
	};

	if (oper > max)
		return TCPM_ERROR_PARAMETER;

	tcp_event.tcp_dpm_data.pd_req_ex.max = max;
	tcp_event.tcp_dpm_data.pd_req_ex.oper = oper;
	return tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
}

int tcpm_dpm_pd_hard_reset_bk(struct tcpc_device *tcpc)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_HARD_RESET,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	return tcpm_put_tcp_dpm_event_bk(tcpc, 3500, &tcp_event);
}

#ifdef CONFIG_USB_PD_UVDM
int tcpm_dpm_send_uvdm_bk(struct tcpc_device *tcpc,
	uint8_t cnt, uint32_t *data, bool wait_resp)
{
	int ret;
	struct tcp_dpm_uvdm_data *uvdm_data;

	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_UVDM,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	if (cnt > PD_DATA_OBJ_SIZE)
		return TCPM_ERROR_PARAMETER;

	uvdm_data = &tcp_event.tcp_dpm_data.uvdm_data;

	uvdm_data->cnt = cnt;
	uvdm_data->wait_resp = wait_resp;
	memcpy(uvdm_data->vdos, data, sizeof(uint32_t) * cnt);

	ret = tcpm_put_tcp_dpm_event_bk(tcpc, 1000, &tcp_event);
	if (ret != TCP_DPM_RET_SUCCESS)
		return ret;

	if (wait_resp) {
		memcpy(data, tcpc->pd_port.uvdm_data,
			sizeof(uint32_t) * tcpc->pd_port.uvdm_cnt);
	}

	return TCP_DPM_RET_SUCCESS;
}
#endif	/* CONFIG_USB_PD_UVDM */

int tcpm_set_pd_charging_policy_bk(struct tcpc_device *tcpc, uint8_t policy)
{
	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_REQUEST_AGAIN,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	struct pd_port *pd_port = &tcpc->pd_port;

	if (pd_port->dpm_charging_policy == policy)
		return TCPM_SUCCESS;

	mutex_lock(&pd_port->pd_lock);
	pd_port->dpm_charging_policy = policy;
	mutex_unlock(&pd_port->pd_lock);

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1500, &tcp_event);
}


#ifdef CONFIG_USB_PD_REV30_PPS_SINK

int tcpm_set_apdo_boundary(struct tcpc_device *tcpc, uint8_t index)
{
	struct pd_port *pd_port = &tcpc->pd_port;

	if (pd_port->dpm_charging_policy ==  DPM_CHARGING_POLICY_PPS)
		return TCPM_ERROR_CHARGING_POLICY;

	if (pd_port->request_apdo_pos == index)
		return TCPM_SUCCESS;

	mutex_lock(&pd_port->pd_lock);
	pd_port->request_apdo_pos = index;
	mutex_unlock(&pd_port->pd_lock);

	return TCPM_SUCCESS;
}

int tcpm_set_apdo_charging_policy_bk(
	struct tcpc_device *tcpc, uint8_t policy, int mv, int ma)
{
	int ret;
	struct tcpm_power_cap_val cap_val;

	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_REQUEST_AGAIN,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	struct pd_port *pd_port = &tcpc->pd_port;

	/* Not PPS should call another function ... */
	if ((policy & DPM_CHARGING_POLICY_MASK) < DPM_CHARGING_POLICY_PPS)
		return TCPM_ERROR_PARAMETER;

	if (pd_port->dpm_charging_policy == policy)
		return TCPM_SUCCESS;

	if (pd_port->request_apdo_pos == 0) {
		ret = tcpm_inquire_pd_source_apdo(tcpc,
			TCPM_POWER_CAP_APDO_TYPE_PPS,
			&pd_port->request_apdo_pos, &cap_val);

		if (ret != TCPM_SUCCESS)
			return ret;
	}

	mutex_lock(&pd_port->pd_lock);
	pd_port->dpm_charging_policy = policy;
	pd_port->request_v_apdo = mv;
	pd_port->request_i_apdo = ma;
	mutex_unlock(&pd_port->pd_lock);

	return tcpm_put_tcp_dpm_event_bk(tcpc, 1500, &tcp_event);
}

int tcpm_dpm_pd_get_status_bk(
	struct tcpc_device *tcpc, uint8_t *status)
{
	int ret;

	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_GET_STATUS,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	ret = tcpm_put_tcp_dpm_event_bk(tcpc, 500, &tcp_event);
	if (ret != TCPM_SUCCESS)
		return ret;

	memcpy(status,
		tcpc->pd_port.remote_status, TCPM_PD_SDB_SIZE);
	return TCPM_SUCCESS;
}

int tcpm_dpm_pd_get_pps_status_bk(
	struct tcpc_device *tcpc, uint8_t *pps_status)
{
	int ret;

	struct tcp_dpm_event tcp_event = {
		.event_id = TCP_DPM_EVT_GET_PPS_STATUS,
		.event_cb = tcpm_dpm_bk_event_cb,
	};

	ret = tcpm_put_tcp_dpm_event_bk(tcpc, 500, &tcp_event);
	if (ret != TCPM_SUCCESS)
		return ret;

	memcpy(pps_status,
		tcpc->pd_port.remote_pps_status, TCPM_PD_PPSDB_SIZE);
	return TCPM_SUCCESS;
}

#endif	/* CONFIG_USB_PD_REV30_PPS_SINK */

#endif	/* CONFIG_USB_PD_BLOCK_TCPM */
#endif	/* CONFIG_USB_POWER_DELIVERY */
