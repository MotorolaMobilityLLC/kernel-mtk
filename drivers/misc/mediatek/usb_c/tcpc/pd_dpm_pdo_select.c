/*
 * drives/usb/pd/pd_dpm_cor.c
 * Power Delivery Core Driver
 *
 * Copyright (C) 2015 Richtek Technology Corp.
 * Author: TH <tsunghan_tsai@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "inc/pd_dpm_pdo_select.h"

#ifdef CONFIG_USB_POWER_DELIVERY

struct dpm_select_info_t {
	uint8_t pos;
	int max_uw;
	int cur_mv;
	uint8_t policy;
};

void dpm_extract_pdo_info(
			uint32_t pdo, struct dpm_pdo_info_t *info)
{
	memset(info, 0, sizeof(struct dpm_pdo_info_t));

	info->type = DPM_PDO_TYPE(pdo);

	switch (info->type) {
	case DPM_PDO_TYPE_FIXED:
		info->ma = PDO_FIXED_EXTRACT_CURR(pdo);
		info->vmax = info->vmin = PDO_FIXED_EXTRACT_VOLT(pdo);
		info->uw = info->ma * info->vmax;
		break;

	case DPM_PDO_TYPE_VAR:
		info->ma = PDO_VAR_EXTRACT_CURR(pdo);
		info->vmin = PDO_VAR_EXTRACT_MIN_VOLT(pdo);
		info->vmax = PDO_VAR_EXTRACT_MAX_VOLT(pdo);
		info->uw = info->ma * info->vmax;
		break;

	case DPM_PDO_TYPE_BAT:
		info->uw = PDO_BATT_EXTRACT_OP_POWER(pdo) * 1000;
		info->vmin = PDO_BATT_EXTRACT_MIN_VOLT(pdo);
		info->vmax = PDO_BATT_EXTRACT_MAX_VOLT(pdo);
		info->ma = info->uw / info->vmin;
		break;
	}
}

#ifndef MIN
#define MIN(a, b)	((a < b) ? (a) : (b))
#endif

static inline int dpm_calc_src_cap_power_uw(
	struct dpm_pdo_info_t *source, struct dpm_pdo_info_t *sink)
{
	int uw, ma;

	if (source->type == DPM_PDO_TYPE_BAT) {
		uw = source->uw;

		if (sink->type == DPM_PDO_TYPE_BAT)
			uw = MIN(uw, sink->uw);
	} else {
		ma = source->ma;

		if (sink->type != DPM_PDO_TYPE_BAT)
			ma = MIN(ma, sink->ma);

		uw = ma * source->vmax;
	}

	return uw;
}

/*
  * Select PDO from VSafe5V
  */

static bool dpm_select_pdo_from_vsafe5v(
	struct dpm_select_info_t *select_info,
	struct dpm_pdo_info_t *sink, struct dpm_pdo_info_t *source)
{
	int uw;

	if ((sink->vmax != TCPC_VBUS_SINK_5V) ||
		(sink->vmin != TCPC_VBUS_SINK_5V) ||
		(source->vmax != TCPC_VBUS_SINK_5V) ||
		(source->vmin != TCPC_VBUS_SINK_5V))
		return false;

	uw = dpm_calc_src_cap_power_uw(source, sink);
	if (uw > select_info->max_uw) {
		select_info->max_uw = uw;
		select_info->cur_mv = source->vmax;
		return true;
	}

	return false;
}

/*
  * Select PDO from Direct Charge
  */

#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
static bool dpm_select_pdo_from_direct_charge(
	struct dpm_select_info_t *select_info,
	struct dpm_pdo_info_t *sink, struct dpm_pdo_info_t *source)
{
	int uw;

	if (sink->type != DPM_PDO_TYPE_VAR
		|| source->type != DPM_PDO_TYPE_VAR)
		return false;

	if (source->vmin >= TCPC_VBUS_SINK_5V)
		return false;

	if (sink->vmax < source->vmax)
		return false;

	if (sink->vmin > source->vmin)
		return false;

	uw = dpm_calc_src_cap_power_uw(source, sink);
	if (uw > select_info->max_uw) {
		select_info->max_uw = uw;
		select_info->cur_mv = source->vmax;
		return true;
	}

	return false;
}
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */

/*
  * Select PDO from Custom
  */

static bool dpm_select_pdo_from_custom(
	struct dpm_select_info_t *select_info,
	struct dpm_pdo_info_t *sink, struct dpm_pdo_info_t *source)
{
	/* TODO */
	return dpm_select_pdo_from_vsafe5v(select_info, sink, source);
}

/*
  * Select PDO from Max Power
  */

static inline bool dpm_is_valid_pdo_pair(struct dpm_pdo_info_t *sink,
	struct dpm_pdo_info_t *source, uint32_t policy)
{
	if (sink->vmax < source->vmax)
		return false;

	if (sink->vmin > source->vmin)
		return false;

	if (policy & DPM_CHARGING_POLICY_IGNORE_MISMATCH_CURR)
		return (sink->ma <= source->ma);

	return true;
}

static bool dpm_select_pdo_from_max_power(
	struct dpm_select_info_t *select_info,
	struct dpm_pdo_info_t *sink, struct dpm_pdo_info_t *source)
{
	bool overload;
	int uw;

#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
	/* Variable for direct charge only */
	if (sink->type == DPM_PDO_TYPE_VAR)
		return false;
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */

	if (!dpm_is_valid_pdo_pair(sink, source, select_info->policy))
		return false;

	uw = dpm_calc_src_cap_power_uw(source, sink);

	overload = uw > select_info->max_uw;

	if ((!overload) && (uw == select_info->max_uw)) {
		if (select_info->policy &
			DPM_CHARGING_POLICY_PREFER_LOW_VOLTAGE)
			overload |= (source->vmax < select_info->cur_mv);
		else if (select_info->policy &
			DPM_CHARGING_POLICY_PREFER_HIGH_VOLTAGE)
			overload |= (source->vmax > select_info->cur_mv);
	}

	if (overload) {
		select_info->max_uw = uw;
		select_info->cur_mv = source->vmax;
		return true;
	}

	return false;
}

/*
  * Select PDO from defined rule ...
  */

typedef bool (*dpm_select_pdo_fun)(
	struct dpm_select_info_t *select_info,
	struct dpm_pdo_info_t *sink, struct dpm_pdo_info_t *source);

bool dpm_find_match_req_info(struct dpm_rdo_info_t *req_info,
	uint32_t snk_pdo, int cnt, uint32_t *src_pdos,
	int min_uw, uint32_t policy)
{
	int i;
	struct dpm_select_info_t select;
	struct dpm_pdo_info_t sink, source;
	dpm_select_pdo_fun select_pdo_fun;

	dpm_extract_pdo_info(snk_pdo, &sink);

	select.pos = 0;
	select.cur_mv = 0;
	select.max_uw = min_uw;
	select.policy = policy;

	switch (policy & 0x0f) {
#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
	case DPM_CHARGING_POLICY_DIRECT_CHARGE:
		select_pdo_fun = dpm_select_pdo_from_direct_charge;
		break;
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */
	case DPM_CHARGING_POLICY_CUSTOM:
		select_pdo_fun = dpm_select_pdo_from_custom;
		break;
	case DPM_CHARGING_POLICY_MAX_POWER:
		select_pdo_fun = dpm_select_pdo_from_max_power;
		break;
	default: /* DPM_CHARGING_POLICY_VSAFE5V */
		select_pdo_fun = dpm_select_pdo_from_vsafe5v;
		break;
	}

	for (i = 0; i < cnt; i++) {
		dpm_extract_pdo_info(src_pdos[i], &source);

		if (select_pdo_fun(&select, &sink, &source))
			select.pos = i+1;
	}

	if (select.pos > 0) {
		dpm_extract_pdo_info(src_pdos[select.pos-1], &source);

		req_info->pos = select.pos;
		req_info->type = source.type;
		req_info->vmax = source.vmax;
		req_info->vmin = source.vmin;

		if (sink.type == DPM_PDO_TYPE_BAT)
			req_info->mismatch = select.max_uw < sink.uw;
		else
			req_info->mismatch = source.ma < sink.ma;

		if (source.type == DPM_PDO_TYPE_BAT) {
			req_info->max_uw = sink.uw;
			req_info->oper_uw = select.max_uw;
		} else {
			req_info->max_ma = sink.ma;
			req_info->oper_ma = MIN(sink.ma, source.ma);
		}

		return true;
	}

	return false;
}

#endif	/* CONFIG_USB_POWER_DELIVERY */
