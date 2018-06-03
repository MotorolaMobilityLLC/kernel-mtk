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

#ifndef TCPM_BLOCK_H_
#define TCPM_BLOCK_H_

#include "inc/tcpci_config.h"

#ifdef CONFIG_USB_POWER_DELIVERY
#ifdef CONFIG_USB_PD_BLOCK_TCPM

#include "inc/tcpm.h"

extern int tcpm_dpm_pd_power_swap_bk(struct tcpc_device *tcpc, uint8_t role);
extern int tcpm_dpm_pd_data_swap_bk(struct tcpc_device *tcpc, uint8_t role);
extern int tcpm_dpm_pd_vconn_swap_bk(struct tcpc_device *tcpc, uint8_t role);
extern int tcpm_dpm_pd_goto_min_bk(struct tcpc_device *tcpc);
extern int tcpm_dpm_pd_soft_reset_bk(struct tcpc_device *tcpc);
extern int tcpm_dpm_pd_get_source_cap_bk(struct tcpc_device *tcpc);
extern int tcpm_dpm_pd_get_sink_cap_bk(struct tcpc_device *tcpc);
extern int tcpm_dpm_pd_request_bk(struct tcpc_device *tcpc, int mv, int ma);
extern int tcpm_dpm_pd_request_ex_bk(struct tcpc_device *tcpc,
	uint8_t pos, uint32_t max, uint32_t oper);

extern int tcpm_dpm_pd_hard_reset_bk(struct tcpc_device *tcpc);

#ifdef CONFIG_USB_PD_UVDM
extern int tcpm_dpm_send_uvdm_bk(struct tcpc_device *tcpc,
	uint8_t cnt, uint32_t *data, bool wait_resp);
#endif	/* CONFIG_USB_PD_UVDM */

extern int tcpm_set_pd_charging_policy_bk(
			struct tcpc_device *tcpc, uint8_t policy);

#ifdef CONFIG_USB_PD_REV30_PPS_SINK
extern int tcpm_set_apdo_boundary(struct tcpc_device *tcpc, uint8_t index);

extern int tcpm_set_apdo_charging_policy_bk(
	struct tcpc_device *tcpc, uint8_t policy, int mv, int ma);

extern int tcpm_dpm_pd_get_status_bk(
	struct tcpc_device *tcpc, uint8_t *status);
extern int tcpm_dpm_pd_get_pps_status_bk(
	struct tcpc_device *tcpc, uint8_t *pps_status);

#endif	/* CONFIG_USB_PD_REV30_PPS_SINK */

#endif	/* CONFIG_USB_PD_BLOCK_TCPM */
#endif	/* CONFIG_USB_POWER_DELIVERY */
#endif	/* TCPM_BLOCK_H_ */
