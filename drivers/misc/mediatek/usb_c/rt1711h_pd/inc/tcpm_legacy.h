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

#ifndef TCPM_LEGACY_H_
#define TCPM_LEGACY_H_

#include "tcpci_config.h"


#ifdef CONFIG_USB_POWER_DELIVERY
#ifdef CONFIG_USB_PD_LEGACY_TCPM

#include "tcpm.h"

/* Request TCPM to send PD Request */

extern int tcpm_power_role_swap(struct tcpc_device *tcpc_dev);
extern int tcpm_data_role_swap(struct tcpc_device *tcpc_dev);
extern int tcpm_vconn_swap(struct tcpc_device *tcpc_dev);
extern int tcpm_goto_min(struct tcpc_device *tcpc_dev);
extern int tcpm_soft_reset(struct tcpc_device *tcpc_dev);
extern int tcpm_hard_reset(struct tcpc_device *tcpc_dev);
extern int tcpm_get_source_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap);
extern int tcpm_get_sink_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap);
extern int tcpm_bist_cm2(struct tcpc_device *tcpc_dev);
extern int tcpm_request(
	struct tcpc_device *tcpc_dev, int mv, int ma);
extern int tcpm_error_recovery(struct tcpc_device *tcpc_dev);

/* Request TCPM to send VDM */

extern int tcpm_discover_cable(
	struct tcpc_device *tcpc_dev, uint32_t *vdos);

extern int tcpm_vdm_request_id(
	struct tcpc_device *tcpc_dev, uint32_t *vdos);

/* Request TCPM to send PD-DP Request */

#ifdef CONFIG_USB_PD_ALT_MODE
extern int tcpm_dp_attention(
	struct tcpc_device *tcpc_dev, uint32_t dp_status);
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
extern int tcpm_dp_status_update(
	struct tcpc_device *tcpc_dev, uint32_t dp_status);
extern int tcpm_dp_configuration(
	struct tcpc_device *tcpc_dev, uint32_t dp_config);
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

/* Request TCPM to send PD-UVDM Request */

#ifdef CONFIG_USB_PD_UVDM
extern int tcpm_send_uvdm(struct tcpc_device *tcpc_dev,
	uint8_t cnt, uint32_t *data, bool wait_resp);
#endif	/* CONFIG_USB_PD_UVDM */

#endif	/* CONFIG_USB_PD_LEGACY_TCPM */
#endif	/* CONFIG_USB_POWER_DELIVERY */
#endif	/* TCPM_LEGACY_H_ */
