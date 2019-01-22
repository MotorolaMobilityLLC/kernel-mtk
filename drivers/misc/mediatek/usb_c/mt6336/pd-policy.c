/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "mtk_typec.h"

#if SUPPORT_PD

#include "usb_pd_func.h"

#ifdef CONFIG_RT7207_ADAPTER
#include "mtk_direct_charge_vdm.h"
#endif

#define MIN(x, y)               (((x) < (y)) ? (x) : (y))

#ifdef CONFIG_USB_PD_DUAL_ROLE
/* Cap on the max voltage requested as a sink (in millivolts) */
static unsigned max_request_mv = PD_MAX_VOLTAGE_MV; /* no cap */

/**
 * Find PDO index that offers the most amount of power and stays within
 * max_mv voltage.
 *
 * @param cnt  the number of Power Data Objects.
 * @param src_caps Power Data Objects representing the source capabilities.
 * @param max_mv maximum voltage (or -1 if no limit)
 * @return index of PDO within source cap packet
 */
static int pd_find_pdo_index(int cnt, uint32_t *src_caps, int max_mv)
{
	int i, uw, max_uw = 0, mv, ma = 0;
	int ret = -1;
#ifdef PD_PREFER_LOW_VOLTAGE
	int cur_mv;
#endif

	/* max voltage is always limited by this boards max request */
	max_mv = MIN(max_mv, PD_MAX_VOLTAGE_MV);

	/* Get max power that is under our max voltage input */
	for (i = 0; i < cnt; i++) {
		mv = ((src_caps[i] >> 10) & 0x3FF) * 50;
		/* Skip any voltage not supported by this board */
		if (!pd_is_valid_input_voltage(mv))
			continue;

		if ((src_caps[i] & PDO_TYPE_MASK) == PDO_TYPE_BATTERY) {
			uw = 250000 * (src_caps[i] & 0x3FF);
		} else {
			ma = (src_caps[i] & 0x3FF) * 10;
			ma = MIN(ma, PD_MAX_CURRENT_MA);
			uw = ma * mv;
		}
#ifdef PD_PREFER_LOW_VOLTAGE
		if (mv > max_mv)
			continue;
		uw = MIN(uw, PD_MAX_POWER_MW * 1000);
		if ((uw > max_uw) || ((uw == max_uw) && mv < cur_mv)) {
			ret = i;
			max_uw = uw;
			cur_mv = mv;
		}
#else
		if ((uw > max_uw) && (mv <= max_mv)) {
			ret = i;
			max_uw = uw;
		}
#endif
	}
	return ret;
}

/**
 * Extract power information out of a Power Data Object (PDO)
 *
 * @pdo Power data object
 * @ma Current we can request from that PDO
 * @mv Voltage of the PDO
 */
static void pd_extract_pdo_power(uint32_t pdo, uint32_t *ma, uint32_t *mv)
{
	int max_ma, uw;
	*mv = ((pdo >> 10) & 0x3FF) * 50;

	if ((pdo & PDO_TYPE_MASK) == PDO_TYPE_BATTERY) {
		uw = 250000 * (pdo & 0x3FF);
		max_ma = 1000 * MIN(1000 * uw, PD_MAX_POWER_MW) / *mv;
	} else {
		max_ma = 10 * (pdo & 0x3FF);
		max_ma = MIN(max_ma, PD_MAX_POWER_MW * 1000 / *mv);
	}

	*ma = MIN(max_ma, PD_MAX_CURRENT_MA);
}

int pd_build_request(int cnt, uint32_t *src_caps, uint32_t *rdo,
		     uint32_t *ma, uint32_t *mv, enum pd_request_type req_type)
{
	int pdo_index, flags = 0;
	int uw;

	if (req_type == PD_REQUEST_VSAFE5V)
		/* src cap 0 should be vSafe5V */
		pdo_index = 0;
	else
		/* find pdo index for max voltage we can request */
		pdo_index = pd_find_pdo_index(cnt, src_caps, max_request_mv);

	/* If could not find desired pdo_index, then use index vSafe5V */
	if (pdo_index == -1)
		pdo_index = 0;

	pd_extract_pdo_power(src_caps[pdo_index], ma, mv);
	uw = *ma * *mv;
	/* Mismatch bit set if less power offered than the operating power */
	if (uw < (1000 * PD_OPERATING_POWER_MW))
		flags |= RDO_CAP_MISMATCH;

	flags |= RDO_NO_SUSPEND;

	if ((src_caps[pdo_index] & PDO_TYPE_MASK) == PDO_TYPE_BATTERY) {
		int mw = uw / 1000;
		*rdo = RDO_BATT(pdo_index + 1, mw, mw, flags);
	} else {
		*rdo = RDO_FIXED(pdo_index + 1, *ma, *ma, flags);
	}

	return 0;
}

void pd_process_source_cap(struct typec_hba *hba, int cnt, uint32_t *src_caps)
{
#ifdef CONFIG_CHARGE_MANAGER
	uint32_t ma, mv;
	int pdo_index;

	/* Get max power info that we could request */
	pdo_index = pd_find_pdo_index(cnt, src_caps, PD_MAX_VOLTAGE_MV);
	if (pdo_index < 0)
		pdo_index = 0;
	pd_extract_pdo_power(src_caps[pdo_index], &ma, &mv);

	/* Set max. limit, but apply 500mA ceiling */
	charge_manager_set_ceil(port, CEIL_REQUESTOR_PD, PD_MIN_MA);
	pd_set_input_current_limit(port, ma, mv);
#endif
}

void pd_set_max_voltage(unsigned mv)
{
	max_request_mv = mv;
}

unsigned pd_get_max_voltage(void)
{
	return max_request_mv;
}

#if 0
int pd_charge_from_device(uint16_t vid, uint16_t pid)
{
	/* TODO: rewrite into table if we get more of these */
	/*
	 * White-list Apple charge-through accessory since it doesn't set
	 * externally powered bit, but we still need to charge from it when
	 * we are a sink.
	 */
	return (vid == USB_VID_APPLE && (pid == 0x1012 || pid == 0x1013));
}
#endif
#endif /* CONFIG_USB_PD_DUAL_ROLE */

#ifdef CONFIG_USB_PD_ALT_MODE

#ifdef CONFIG_USB_PD_ALT_MODE_DFP

static struct pd_policy pe;

void pd_dfp_pe_init(struct typec_hba *hba)
{
	memset(&pe, 0, sizeof(struct pd_policy));
}

static void dfp_consume_identity(struct typec_hba *hba, int cnt, uint32_t *payload)
{
	int ptype = PD_IDH_PTYPE(payload[VDO_I(IDH)]);
	size_t identity_size = MIN(sizeof(pe.identity),
				   (cnt - 1) * sizeof(uint32_t));
	pd_dfp_pe_init(hba);
	memcpy(&pe.identity, payload + 1, identity_size);
	switch (ptype) {
	case IDH_PTYPE_AMA:
		/* TODO(tbroch) do I disable VBUS here if power contract
		 * requested it
		 */
		if (!PD_VDO_AMA_VBUS_REQ(payload[VDO_I(AMA)]))
			dev_err(hba->dev, "AMA VBUS no required\n");
			/* If turn off VBUS, Ellisys can not continue test DP alt mode. */
			/*pd_power_supply_reset(hba);*/
		break;
		/* TODO(crosbug.com/p/30645) provide vconn support here */
	default:
		break;
	}
}

static int dfp_discover_svids(struct typec_hba *hba, uint32_t *payload)
{
	payload[0] = VDO(USB_SID_PD, 1, CMD_DISCOVER_SVID);
	return 1;
}

static int dfp_consume_svids(struct typec_hba *hba, uint32_t *payload)
{
	int i;
	uint32_t *ptr = payload + 1;
	uint16_t svid0 = U16_MAX, svid1 = U16_MAX;
	int svid_base = pe.svid_cnt;

	for (i = pe.svid_cnt; i < pe.svid_cnt + 12; i += 2) {
		if (i == 12 + svid_base) {
			dev_err(hba->dev, "Not finish");
			break;
		}

		svid0 = PD_VDO_SVID_SVID0(*ptr);
		if (!svid0)
			break;
		pe.svids[i].svid = svid0;
		pe.svid_cnt++;

		svid1 = PD_VDO_SVID_SVID1(*ptr);
		if (!svid1)
			break;
		pe.svids[i + 1].svid = svid1;
		pe.svid_cnt++;
		ptr++;
	}

	if ((svid0 == 0) || (svid1 == 0))
		return 1;

	dev_err(hba->dev, "Try next SVIDs\n");
	return 0;
}

static int dfp_discover_modes(struct typec_hba *hba, uint32_t *payload)
{
	uint16_t svid = pe.svids[pe.svid_idx].svid;

	if (pe.svid_idx >= pe.svid_cnt)
		return 0;
	payload[0] = VDO(svid, 1, CMD_DISCOVER_MODES);
	return 1;
}

static void dfp_consume_modes(struct typec_hba *hba, int cnt, uint32_t *payload)
{
	int idx = pe.svid_idx;

	pe.svids[idx].mode_cnt = cnt - 1;
	if (pe.svids[idx].mode_cnt < 0) {
		dev_err(hba->dev, "ERR:NOMODE\n");
	} else {
		memcpy(pe.svids[pe.svid_idx].mode_vdo, &payload[1],
		       sizeof(uint32_t) * pe.svids[idx].mode_cnt);
	}

	pe.svid_idx++;
}

static int get_mode_idx(struct typec_hba *hba, uint16_t svid)
{
	int i;

	for (i = 0; i < PD_AMODE_COUNT; i++) {
		if (pe.amodes[i].fx == NULL)
			return -1;
		if (pe.amodes[i].fx->svid == svid)
			return i;
	}
	return -1;
}

static struct svdm_amode_data *get_modep(struct typec_hba *hba, uint16_t svid)
{
	int idx = get_mode_idx(hba, svid);

	return (idx == -1) ? NULL : &pe.amodes[idx];
}

int pd_alt_mode(struct typec_hba *hba, uint16_t svid)
{
	struct svdm_amode_data *modep = get_modep(hba, svid);

	return (modep) ? modep->opos : -1;
}

int allocate_mode(struct typec_hba *hba, uint16_t svid)
{
	int i, j;
	struct svdm_amode_data *modep;
	int mode_idx = get_mode_idx(hba, svid);

	if (mode_idx != -1)
		return mode_idx;

	/* There's no space to enter another mode */
	if (pe.amode_idx == PD_AMODE_COUNT) {
		dev_err(hba->dev, "ERR:NO AMODE SPACE\n");
		return -1;
	}

	/* Allocate ...  if SVID == 0 enter default supported policy */
	for (i = 0; i < supported_modes_cnt; i++) {
		for (j = 0; j < pe.svid_cnt; j++) {
			struct svdm_svid_data *svidp = &pe.svids[j];

			if ((svidp->svid != supported_modes[i].svid) ||
			    (svid && (svidp->svid != svid)))
				continue;

			modep = &pe.amodes[pe.amode_idx];
			modep->fx = &supported_modes[i];
			modep->data = &pe.svids[j];
			pe.amode_idx++;
			return pe.amode_idx - 1;
		}
	}
	return -1;
}

/*
 * Enter default mode ( payload[0] == 0 ) or attempt to enter mode via svid &
 * opos
*/
uint32_t pd_dfp_enter_mode(struct typec_hba *hba, uint16_t svid, int opos)
{
	int mode_idx = allocate_mode(hba, svid);
	struct svdm_amode_data *modep;
	uint32_t mode_caps;

	if (mode_idx == -1)
		return 0;
	modep = &pe.amodes[mode_idx];

	if (!opos) {
		/* choose the lowest as default */
		modep->opos = 1;
	} else if (opos <= modep->data->mode_cnt) {
		modep->opos = opos;
	} else {
		dev_err(hba->dev, "opos error\n");
		return 0;
	}

	mode_caps = modep->data->mode_vdo[modep->opos - 1];
	if (modep->fx->enter(hba, mode_caps) == -1)
		return 0;

	/* SVDM to send to UFP for mode entry */
	return VDO(modep->fx->svid, 1, CMD_ENTER_MODE | VDO_OPOS(modep->opos));
}

static int validate_mode_request(struct typec_hba *hba, struct svdm_amode_data *modep,
				 uint16_t svid, int opos)
{
	if (!modep->fx)
		return 0;

	if (svid != modep->fx->svid) {
		dev_err(hba->dev, "ERR:svid r:0x%04x != c:0x%04x\n",
			svid, modep->fx->svid);
		return 0;
	}

	if (opos != modep->opos) {
		dev_err(hba->dev, "ERR:opos r:%d != c:%d\n",
			opos, modep->opos);
		return 0;
	}

	return 1;
}

static void dfp_consume_attention(struct typec_hba *hba, uint32_t *payload)
{
	uint16_t svid = PD_VDO_VID(payload[0]);
	int opos = PD_VDO_OPOS(payload[0]);
	struct svdm_amode_data *modep = get_modep(hba, svid);

	if (!modep || !validate_mode_request(hba, modep, svid, opos))
		return;

	if (modep->fx->attention)
		modep->fx->attention(hba, payload);
}

/*
 * This algorithm defaults to choosing higher pin config over lower ones.  Pin
 * configs are organized in pairs with the following breakdown.
 *
 *  NAME | SIGNALING | OUTPUT TYPE | MULTI-FUNCTION | PIN CONFIG
 * -------------------------------------------------------------
 *  A    |  USB G2   |  ?          | no             | 00_0001
 *  B    |  USB G2   |  ?          | yes            | 00_0010
 *  C    |  DP       |  CONVERTED  | no             | 00_0100
 *  D    |  PD       |  CONVERTED  | yes            | 00_1000
 *  E    |  DP       |  DP         | no             | 01_0000
 *  F    |  PD       |  DP         | yes            | 10_0000
 *
 * if UFP has NOT asserted multi-function preferred code masks away B/D/F
 * leaving only A/C/E.  For single-output dongles that should leave only one
 * possible pin config depending on whether its a converter DP->(VGA|HDMI) or DP
 * output.  If someone creates a multi-output dongle presumably they would need
 * to either offer different mode capabilities depending upon connection type or
 * the DFP would need additional system policy to expose those options.
 */
int pd_dfp_dp_get_pin_mode(struct typec_hba *hba, uint32_t status)
{
	struct svdm_amode_data *modep = get_modep(hba, USB_SID_DISPLAYPORT);
	uint32_t mode_caps;
	uint32_t pin_caps;

	if (!modep)
		return 0;

	mode_caps = modep->data->mode_vdo[modep->opos - 1];

	/* TODO(crosbug.com/p/39656) revisit with DFP that can be a sink */
	pin_caps = PD_DP_PIN_CAPS(mode_caps);

	/* if don't want multi-function then ignore those pin configs */
	if (!PD_VDO_DPSTS_MF_PREF(status))
		pin_caps &= ~MODE_DP_PIN_MF_MASK;

	/* TODO(crosbug.com/p/39656) revisit if DFP drives USB Gen 2 signals */
	pin_caps &= ~MODE_DP_PIN_BR2_MASK;

	/* get_next_bit returns undefined for zero */
	if (!pin_caps)
		return 0;

	/*FIXME: no declaration of function get_next_bit. Fix later*/
	return 1;/* << get_next_bit(&pin_caps); */
}

int pd_dfp_exit_mode(struct typec_hba *hba, uint16_t svid, int opos)
{
	struct svdm_amode_data *modep;
	int idx;

	hba->alt_mode_svid = 0;

	/*
	 * Empty svid signals we should reset DFP VDM state by exiting all
	 * entered modes then clearing state.  This occurs when we've
	 * disconnected or for hard reset.
	 */
	if (!svid) {
		for (idx = 0; idx < PD_AMODE_COUNT; idx++)
			if (pe.amodes[idx].fx)
				pe.amodes[idx].fx->exit(hba);

		pd_dfp_pe_init(hba);
		return 0;
	}

	modep = get_modep(hba, svid);
	if (!modep || !validate_mode_request(hba, modep, svid, opos))
		return 0;

	/* call DFPs exit function */
	modep->fx->exit(hba);
	/* exit the mode */
	modep->opos = 0;
	return 1;
}

uint16_t pd_get_identity_vid(struct typec_hba *hba)
{
	return PD_IDH_VID(pe.identity[0]);
}

uint16_t pd_get_identity_pid(struct typec_hba *hba)
{
	return PD_PRODUCT_PID(pe.identity[2]);
}

#ifdef CONFIG_CMD_USB_PD_PE
static void dump_pe(struct typec_hba *hba)
{
	const char * const idh_ptype_names[]  = {
		"UNDEF", "Hub", "Periph", "PCable", "ACable", "AMA",
		"RSV6", "RSV7"};

	int i, j, idh_ptype;
	struct svdm_amode_data *modep;
	uint32_t mode_caps;

	if (pe.identity[0] == 0) {
		ccprintf("No identity discovered yet.\n");
		return;
	}
	idh_ptype = PD_IDH_PTYPE(pe.identity[0]);
	ccprintf("IDENT:\n");
	ccprintf("\t[ID Header] %08x :: %s, VID:%04x\n", pe.identity[0],
		 idh_ptype_names[idh_ptype], pd_get_identity_vid(hba));
	ccprintf("\t[Cert Stat] %08x\n", pe.identity[1]);
	for (i = 2; i < ARRAY_SIZE(pe.identity); i++) {
		ccprintf("\t");
		if (pe.identity[i])
			ccprintf("[%d] %08x ", i, pe.identity[i]);
	}
	ccprintf("\n");

	if (pe.svid_cnt < 1) {
		ccprintf("No SVIDS discovered yet.\n");
		return;
	}

	for (i = 0; i < pe.svid_cnt; i++) {
		ccprintf("SVID[%d]: %04x MODES:", i, pe.svids[i].svid);
		for (j = 0; j < pe.svids[j].mode_cnt; j++)
			ccprintf(" [%d] %08x", j + 1,
				 pe.svids[i].mode_vdo[j]);
		ccprintf("\n");
		modep = get_modep(hba, pe.svids[i].svid);
		if (modep) {
			mode_caps = modep->data->mode_vdo[modep->opos - 1];
			ccprintf("MODE[%d]: svid:%04x caps:%08x\n", modep->opos,
				 modep->fx->svid, mode_caps);
		}
	}
}

static int command_pe(int argc, char **argv)
{
	int port;
	char *e;

	if (argc < 3)
		return EC_ERROR_PARAM_COUNT;
	/* command: pe <port> <subcmd> <args> */
	port = strtoi(argv[1], &e, 10);
	if (*e || port >= CONFIG_USB_PD_PORT_COUNT)
		return EC_ERROR_PARAM2;
	if (!strncasecmp(argv[2], "dump", 4))
		dump_pe(hba);

	return EC_SUCCESS;
}

DECLARE_CONSOLE_COMMAND(pe, command_pe,
			"<port> dump",
			"USB PE",
			NULL);
#endif /* CONFIG_CMD_USB_PD_PE */

#endif /* CONFIG_USB_PD_ALT_MODE_DFP */

int pd_svdm(struct typec_hba *hba, int cnt, uint32_t *payload, uint32_t **rpayload)
{
	int cmd = PD_VDO_CMD(payload[0]);
	int cmd_type = PD_VDO_CMDT(payload[0]);
	int (*func)(struct typec_hba *hba, uint32_t *payload) = NULL;
	int is_dp_attention = 0;

	int rsize = 1; /* VDM header at a minimum */
	const char *str_cmd_type[4] = {
			"INIT",
			"ACK",
			"NAK",
			"BUSY",
	};

	dev_err(hba->dev, "%s %s cmd=%d cnt=%d\n", __func__, str_cmd_type[cmd_type], cmd, cnt);

	payload[0] &= ~VDO_CMDT_MASK;
	*rpayload = payload;

	if (cmd_type == CMDT_INIT) {
		switch (cmd) {
		case CMD_DISCOVER_IDENT:
			if (PD_VDO_VID(payload[0]) == USB_SID_PD)
				func = svdm_rsp.identity;
			else
				dev_err(hba->dev, "Discover Identity Wrong SVID 0x%04X\n", PD_VDO_VID(payload[0]));
			break;
		case CMD_DISCOVER_SVID:
			func = svdm_rsp.svids;
			break;
		case CMD_DISCOVER_MODES:
			func = svdm_rsp.modes;
			break;
		case CMD_ENTER_MODE:
			func = svdm_rsp.enter_mode;
			break;
		case CMD_DP_STATUS:
			func = svdm_rsp.amode->status;
			break;
		case CMD_DP_CONFIG:
			func = svdm_rsp.amode->config;
			break;
		case CMD_EXIT_MODE:
			func = svdm_rsp.exit_mode;
			break;
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
		case CMD_ATTENTION:

			dfp_consume_attention(hba, payload);
			/*At DP, when receive attention, should send config cmd */
			if (PD_VDO_VID(payload[0]) == USB_SID_DISPLAYPORT)
				is_dp_attention = 1;
			else
				return 0;
			break;
#endif
		default:
			dev_err(hba->dev, "ERR:CMD:%d\n", cmd);
			rsize = 0;
		}
		if (func)
			rsize = func(hba, payload);
		else /* not supported : NACK it */
			rsize = 0;
		if (rsize >= 1)
			payload[0] |= VDO_CMDT(CMDT_RSP_ACK);
		else if (!rsize) {
			payload[0] |= VDO_CMDT(CMDT_RSP_NAK);
			rsize = 1;
		} else {
			payload[0] |= VDO_CMDT(CMDT_RSP_BUSY);
			rsize = 1;
		}
	} else if (cmd_type == CMDT_RSP_ACK) {
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
		struct svdm_amode_data *modep = NULL;

		if (cmd > CMD_DISCOVER_MODES)
			modep = get_modep(hba, PD_VDO_VID(payload[0]));
#endif
		switch (cmd) {
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
		case CMD_DISCOVER_IDENT:
			dfp_consume_identity(hba, cnt, payload);
			rsize = dfp_discover_svids(hba, payload);
			break;
		case CMD_DISCOVER_SVID:
			rsize = dfp_consume_svids(hba, payload);

			if (rsize == 1)
				rsize = dfp_discover_modes(hba, payload);
			else
				rsize = dfp_discover_svids(hba, payload);
			break;
		case CMD_DISCOVER_MODES:
			dfp_consume_modes(hba, cnt, payload);
			rsize = dfp_discover_modes(hba, payload);
			/* enter the default mode for DFP */
			if (!rsize) {
				payload[0] = pd_dfp_enter_mode(hba, 0, 0);
				if (payload[0])
					rsize = 1;
			}
			break;
		case CMD_ENTER_MODE:
			if (!modep) {
				rsize = 0;
			} else {
				if (!modep->opos)
					pd_dfp_enter_mode(hba, 0, 0);

				if (modep->opos) {
					hba->alt_mode_svid = modep->fx->svid;
#ifdef CONFIG_RT7207_ADAPTER
					if (modep->fx->svid == RT7207_SVID) {
						start_dc_auth(hba->dc);
						rsize = 0;
					} else
#endif
					{
						rsize = modep->fx->status(hba, payload);
						payload[0] |= PD_VDO_OPOS(modep->opos);
					}
				}
			}
			break;
		case CMD_DP_STATUS:
			/* DP status response & UFP's DP attention have same
			 * payload
			 */
			dfp_consume_attention(hba, payload);
			if (modep && modep->opos)
				rsize = modep->fx->config(hba, payload);
			else
				rsize = 0;
			break;
		case CMD_DP_CONFIG:
			if (modep && modep->opos && modep->fx->post_config)
				modep->fx->post_config(hba);
			/* no response after DFPs ack */
			rsize = 0;
			break;
		case CMD_EXIT_MODE:
			/* no response after DFPs ack */
			rsize = 0;
			break;
#endif
		case CMD_ATTENTION:
			/* no response after DFPs ack */
			rsize = 0;
			break;
		default:
			dev_err(hba->dev, "ERR:CMD:%d\n", cmd);
			rsize = 0;
		}

		payload[0] |= VDO_CMDT(CMDT_INIT);
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	} else if (cmd_type == CMDT_RSP_BUSY) {
		switch (cmd) {
		case CMD_DISCOVER_IDENT:
		case CMD_DISCOVER_SVID:
		case CMD_DISCOVER_MODES:
			/* resend if its discovery */
			rsize = 1;
			break;
		case CMD_ENTER_MODE:
			/* Error */
			dev_err(hba->dev, "ERR:ENTBUSY\n");
			rsize = 0;
			break;
		case CMD_EXIT_MODE:
			rsize = 0;
			break;
		default:
			rsize = 0;
		}
	} else if (cmd_type == CMDT_RSP_NAK) {
		switch (cmd) {
		case CMD_DISCOVER_MODES:
			/*If can not discover this mode, just discover the next one*/
			pe.svid_idx++;
			rsize = dfp_discover_modes(hba, payload);
			break;
		default:
			rsize = 0;
		break;
		}
		payload[0] |= VDO_CMDT(CMDT_INIT);
#endif /* CONFIG_USB_PD_ALT_MODE_DFP */
	}

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	if (is_dp_attention == 1) {
		struct svdm_amode_data *modep = get_modep(hba, USB_SID_DISPLAYPORT);

		dev_err(hba->dev, "Send DP Config CMD\n");
		if (modep && modep->opos) {
			rsize = modep->fx->config(hba, payload);
			payload[0] |= VDO_CMDT(CMDT_INIT);
		}
		dev_err(hba->dev, "rsize=%d, modep=%p\n", rsize, modep);
		is_dp_attention = 0;
	}
#endif
	return rsize;
}

#else

int pd_svdm(struct typec_hba *hba, int cnt, uint32_t *payload, uint32_t **rpayload)
{
	return 0;
}

#endif /* CONFIG_USB_PD_ALT_MODE */

#ifndef CONFIG_USB_PD_CUSTOM_VDM
int pd_vdm(struct typec_hba *hba, int cnt, uint32_t *payload, uint32_t **rpayload)
{
	return 0;
}
#endif /* !CONFIG_USB_PD_CUSTOM_VDM */

#ifdef NEVER
void pd_usb_billboard_deferred(void)
{
#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP) \
	&& !defined(CONFIG_USB_PD_SIMPLE_DFP) && defined(CONFIG_USB_BOS)

	/*
	 * TODO(tbroch)
	 * 1. Will we have multiple type-C port UFPs
	 * 2. Will there be other modes applicable to DFPs besides DP
	 */
	if (!pd_alt_mode(0, USB_SID_DISPLAYPORT))
		usb_connect();

#endif
}
DECLARE_DEFERRED(pd_usb_billboard_deferred);
#endif /* NEVER */

#endif
