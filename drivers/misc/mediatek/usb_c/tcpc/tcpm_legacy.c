/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Legacy Power Delivery Managert Driver
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

#include "inc/tcpm.h"
#include "inc/tcpci.h"
#include "inc/tcpci_typec.h"

#ifdef CONFIG_USB_POWER_DELIVERY
#include "inc/pd_core.h"
#include "inc/pd_dpm_core.h"
#include "inc/pd_policy_engine.h"
#endif	/* CONFIG_USB_POWER_DELIVERY */

/* Request TCPC to send PD Request */

#ifdef CONFIG_USB_POWER_DELIVERY
#ifdef CONFIG_USB_PD_LEGACY_TCPM

int tcpm_power_role_swap(struct tcpc_device *tcpc_dev)
{
	uint8_t role = tcpm_inquire_pd_power_role(tcpc_dev);

	if (role == PD_ROLE_SINK)
		role = PD_ROLE_SOURCE;
	else
		role = PD_ROLE_SINK;

	return tcpm_dpm_pd_power_swap(tcpc_dev, role, NULL);
}
EXPORT_SYMBOL(tcpm_power_role_swap);

int tcpm_data_role_swap(struct tcpc_device *tcpc_dev)
{
	uint8_t role = tcpm_inquire_pd_data_role(tcpc_dev);

	if (role == PD_ROLE_UFP)
		role = PD_ROLE_DFP;
	else
		role = PD_ROLE_UFP;

	return tcpm_dpm_pd_data_swap(tcpc_dev, role, NULL);
}
EXPORT_SYMBOL(tcpm_data_role_swap);

int tcpm_vconn_swap(struct tcpc_device *tcpc_dev)
{
	uint8_t role = tcpm_inquire_pd_vconn_role(tcpc_dev);

	if (role == PD_ROLE_VCONN_OFF)
		role = PD_ROLE_VCONN_ON;
	else
		role = PD_ROLE_VCONN_OFF;

	return tcpm_dpm_pd_vconn_swap(tcpc_dev, role, NULL);
}
EXPORT_SYMBOL(tcpm_vconn_swap);

int tcpm_goto_min(struct tcpc_device *tcpc_dev)
{
	return tcpm_dpm_pd_goto_min(tcpc_dev, NULL);
}
EXPORT_SYMBOL(tcpm_goto_min);

int tcpm_soft_reset(struct tcpc_device *tcpc_dev)
{
	return tcpm_dpm_pd_soft_reset(tcpc_dev, NULL);
}
EXPORT_SYMBOL(tcpm_soft_reset);

int tcpm_hard_reset(struct tcpc_device *tcpc_dev)
{
	return tcpm_dpm_pd_hard_reset(tcpc_dev);
}
EXPORT_SYMBOL(tcpm_hard_reset);

int tcpm_get_source_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap)
{
	if (cap == NULL)
		return tcpm_dpm_pd_get_source_cap(tcpc_dev, NULL);

	return tcpm_inquire_pd_source_cap(tcpc_dev, cap);
}
EXPORT_SYMBOL(tcpm_get_source_cap);

int tcpm_get_sink_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap)
{
	if (cap == NULL)
		return tcpm_dpm_pd_get_sink_cap(tcpc_dev, NULL);

	return tcpm_inquire_pd_sink_cap(tcpc_dev, cap);
}
EXPORT_SYMBOL(tcpm_get_sink_cap);

int tcpm_bist_cm2(struct tcpc_device *tcpc_dev)
{
	return tcpm_dpm_pd_bist_cm2(tcpc_dev, NULL);
}
EXPORT_SYMBOL(tcpm_bist_cm2);

int tcpm_request(struct tcpc_device *tcpc_dev, int mv, int ma)
{
	return tcpm_dpm_pd_request(tcpc_dev, mv, ma, NULL);
}
EXPORT_SYMBOL(tcpm_request);

int tcpm_error_recovery(struct tcpc_device *tcpc_dev)
{
	return tcpm_dpm_pd_error_recovery(tcpc_dev);
}
EXPORT_SYMBOL(tcpm_error_recovery);

int tcpm_discover_cable(struct tcpc_device *tcpc_dev, uint32_t *vdos)
{
	if (vdos == NULL)
		return tcpm_dpm_vdm_discover_cable(tcpc_dev, NULL);

	return tcpm_inquire_cable_inform(tcpc_dev, vdos);
}
EXPORT_SYMBOL(tcpm_discover_cable);

int tcpm_vdm_request_id(
	struct tcpc_device *tcpc_dev, uint32_t *vdos)
{
	if (vdos == NULL)
		return tcpm_dpm_vdm_discover_id(tcpc_dev, NULL);

	return tcpm_inquire_pd_partner_inform(tcpc_dev, vdos);
}
EXPORT_SYMBOL(tcpm_vdm_request_id);

#ifdef CONFIG_USB_PD_ALT_MODE

int tcpm_dp_attention(
	struct tcpc_device *tcpc_dev, uint32_t dp_status)
{
	return tcpm_dpm_dp_attention(tcpc_dev, dp_status, 0xffffffff, NULL);
}
EXPORT_SYMBOL(tcpm_dp_attention);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP

int tcpm_dp_status_update(
	struct tcpc_device *tcpc_dev, uint32_t dp_status)
{
	return tcpm_dpm_dp_status_update(tcpc_dev, dp_status, 0xffffffff, NULL);
}
EXPORT_SYMBOL(tcpm_dp_status_update);

int tcpm_dp_configuration(
	struct tcpc_device *tcpc_dev, uint32_t dp_config)
{
	return tcpm_dpm_dp_config(tcpc_dev, dp_config, 0xffffffff, NULL);
}
EXPORT_SYMBOL(tcpm_dp_configuration);

#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
int tcpm_send_uvdm(struct tcpc_device *tcpc_dev,
	uint8_t cnt, uint32_t *data, bool wait_resp)
{
	return tcpm_dpm_send_uvdm(tcpc_dev, cnt, data, wait_resp, NULL);
}
#endif	/* CONFIG_USB_PD_UVDM */

#endif	/* CONFIG_USB_PD_LEGACY_TCPM */
#endif	/* CONFIG_USB_POWER_DELIVERY */
