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

#include <typec.h>
#include "usb_pd_func.h"
#include <mt6336/mt6336.h>

#ifdef CONFIG_RT7207_ADAPTER
#include "mtk_direct_charge_vdm.h"
#endif

#define PDO_FIXED_FLAGS (PDO_FIXED_DATA_SWAP)

/* TODO: fill in correct source and sink capabilities */
uint32_t pd_src_pdo[] = {
		PDO_FIXED(5000, 500, PDO_FIXED_FLAGS),
};
int pd_src_pdo_cnt = ARRAY_SIZE(pd_src_pdo);

uint32_t pd_snk_pdo[] = {
		PDO_FIXED(5000, 1500, PDO_FIXED_FLAGS | PDO_FIXED_HIGHER_CAP),
		PDO_FIXED(9000, 1500, 0), /*All other Fixed Supply PDOs shall set bits 29...20 to 0.*/
		#if 0
		PDO_BATT(5000, 20000, 15000),
		PDO_VAR(5000, 20000, 3000),
		#endif
};
int pd_snk_pdo_cnt = ARRAY_SIZE(pd_snk_pdo);

int pd_is_valid_input_voltage(int mv)
{
	return 1;
}

int pd_check_requested_voltage(struct typec_hba *hba, uint32_t rdo)
{
	int max_ma = rdo & 0x3FF;
	int op_ma = (rdo >> 10) & 0x3FF;
	int idx = rdo >> 28;
	uint32_t pdo;
	uint32_t pdo_ma;

	if (!idx || idx > pd_src_pdo_cnt)
		return 1; /* Invalid index */

	/* check current ... */
	pdo = pd_src_pdo[idx - 1];
	pdo_ma = (pdo & 0x3ff);
	if (op_ma > pdo_ma)
		return 1; /* too much op current */
	if (max_ma > pdo_ma)
		return 1; /* too much max current */

	dev_err(hba->dev, "Requested %d V %d mA (for %d/%d mA)\n",
		 ((pdo >> 10) & 0x3ff) * 50, (pdo & 0x3ff) * 10,
		 ((rdo >> 10) & 0x3ff) * 10, (rdo & 0x3ff) * 10);

	return 0;
}

void pd_transition_voltage(int idx)
{
	/* No-operation: we are always 5V */
}

int pd_set_power_supply_ready(struct typec_hba *hba)
{
	hba->drive_vbus(hba, 1);

	return 1;
}

void pd_power_supply_reset(struct typec_hba *hba)
{
	/* Disable VBUS */
	hba->drive_vbus(hba, 0);
}

void pd_set_input_current_limit(struct typec_hba *hba, uint32_t max_ma,
				uint32_t supply_voltage)
{
#ifdef CONFIG_CHARGE_MANAGER
	struct charge_port_info charge;

	charge.current = max_ma;
	charge.voltage = supply_voltage;
	charge_manager_update_charge(CHARGE_SUPPLIER_PD, port, &charge);
#endif
	/* notify host of power info change */
}

void typec_set_input_current_limit(struct typec_hba *hba, uint32_t max_ma,
				   uint32_t supply_voltage)
{
#ifdef CONFIG_CHARGE_MANAGER
	struct charge_port_info charge;

	charge.current = max_ma;
	charge.voltage = supply_voltage;
	charge_manager_update_charge(CHARGE_SUPPLIER_TYPEC, port, &charge);
#endif

	/* notify host of power info change */
}

#if 0
int pd_board_checks(void)
{
#if BOARD_REV <= OAK_REV3
	/* wake up VBUS task to check vbus change */
	task_wake(TASK_ID_VBUS);
#endif
	return EC_SUCCESS;
}
#endif

int pd_check_power_swap(struct typec_hba *hba)
{
	/*
	 * Allow power swap as long as we are acting as a dual role device,
	 * otherwise assume our role is fixed (not in S0 or console command
	 * to fix our role).
	 */
	return ((hba->support_role == TYPEC_ROLE_DRP) ? 1 : 0);
}

int pd_check_vconn_swap(struct typec_hba *hba)
{
	return 1;
}

int pd_check_data_swap(struct typec_hba *hba)
{
	#if 0
	/* Allow data swap if we are a UFP, otherwise don't allow */
	return ((hba->data_role == PD_ROLE_UFP) ? 1 : 0);
	#else
	return 1;
	#endif
}

void pd_execute_data_swap(struct typec_hba *hba, int data_role)
{
	int debounce = 0;
	/*
	 * Under these 5 situations, the driver will call DR_SWAP.
	 * 1. Hard Reset. Reset to the default state
	 *       SNK + DFP --> SNK + UFP
	 *       SRC + UFP --> SRC + DFP
	 * 2. Receiving DR_SWAP & Resp ACCEPT
	 *       DFP --> UFP & UFP --> DFP
	 * 3. Sending DR_SWAP & Resp ACCEPT
	 *       DFP --> UFP & UFP --> DFP
	 * 4. Attached.SRC
	 *       DFP
	 * 5. Attached.SNK
	 *       UFP
	 */
	if (hba->task_state == PD_STATE_SRC_ATTACH)
		debounce = 100;

	dev_err(hba->dev, "Queue usb_work d=%d\n", debounce);

	schedule_delayed_work(&hba->usb_work, msecs_to_jiffies(debounce));
}

void pd_check_pr_role(struct typec_hba *hba, int pr_role, int flags)
{
	/*
	 * If partner is dual-role power and dualrole toggling is on, consider
	 * if a power swap is necessary.
	 */
	if ((flags & PD_FLAGS_PARTNER_DR_POWER) &&
		(hba->support_role == TYPEC_ROLE_DRP)) {
		/*
		 * If we are a sink and partner is not externally powered, then
		 * swap to become a source. If we are source and partner is
		 * externally powered, swap to become a sink.
		 */
		int partner_extpower = flags & PD_FLAGS_PARTNER_EXTPOWER;
#ifdef NEVER
		if ((!partner_extpower && pr_role == PD_ROLE_SINK) ||
		     (partner_extpower && pr_role == PD_ROLE_SOURCE))
#else
		if (partner_extpower && pr_role == PD_ROLE_SOURCE)
#endif /* NEVER */
			pd_request_power_swap(hba);
	}
}

void pd_check_dr_role(struct typec_hba *hba, int dr_role, int flags)
{
	/* If UFP, try to switch to DFP */
	if ((flags & PD_FLAGS_PARTNER_DR_DATA) && dr_role == PD_ROLE_UFP)
		pd_request_data_swap(hba);
}

/* ----------------- Vendor Defined Messages ------------------ */
#ifdef CONFIG_RT7207_ADAPTER
const uint32_t vdo_idh = VDO_IDH(1, /* data caps as USB host */
				 1, /* data caps as USB device */
				 IDH_PTYPE_PERIPH, /* Peripheral*/
				 0, /* supports alt modes */
				 RT7207_SVID);
#else
const uint32_t vdo_idh = VDO_IDH(0, /* data caps as USB host */
				 0, /* data caps as USB device */
				 IDH_PTYPE_PERIPH, /* Peripheral*/
				 0, /* supports alt modes */
				 USB_VID_MEDIATEK);
#endif

const uint32_t vdo_product = VDO_PRODUCT(CONFIG_USB_PID, CONFIG_USB_BCD_DEV);

static int svdm_response_identity(struct typec_hba *hba, uint32_t *payload)
{
	payload[VDO_I(IDH)] = vdo_idh;
	payload[VDO_I(CSTAT)] = VDO_CSTAT(MTK_XID);
	payload[VDO_I(PRODUCT)] = vdo_product;
	return VDO_I(PRODUCT) + 1;
}

#ifdef CONFIG_RT7207_ADAPTER
static int svdm_response_svids(struct typec_hba *hba, uint32_t *payload)
{
#ifdef CONFIG_RT7207_ADAPTER
	payload[1] = VDO_SVID(USB_VID_MEDIATEK, RT7207_SVID);
	payload[2] = VDO_SVID(0, 0);
	return 3;
#else
	payload[1] = VDO_SVID(USB_VID_MEDIATEK, 0);
	return 2;
#endif
}
#endif

#ifdef NEVER
/* Will only ever be a single mode for this device */
#define MODE_CNT 1
#define OPOS 1

const uint32_t vdo_dp_mode[MODE_CNT] =  {
	VDO_MODE_GOOGLE(MODE_GOOGLE_FU)
};

static int svdm_response_modes(struct typec_hba *hba, uint32_t *payload)
{
	if (PD_VDO_VID(payload[0]) != USB_VID_GOOGLE)
		return 0; /* nak */

	memcpy(payload + 1, vdo_dp_mode, sizeof(vdo_dp_mode));
	return MODE_CNT + 1;
}

static int svdm_enter_mode(struct typec_hba *hba, uint32_t *payload)
{
	/* SID & mode request is valid */
	if ((PD_VDO_VID(payload[0]) != USB_VID_GOOGLE) ||
	    (PD_VDO_OPOS(payload[0]) != OPOS))
		return 0; /* will generate NAK */

	gfu_mode = 1;
	debug_printf("GFU\n");
	return 1;
}

static int svdm_exit_mode(struct typec_hba *hba, uint32_t *payload)
{
	gfu_mode = 0;
	return 1; /* Must return ACK */
}

static struct amode_fx dp_fx = {
	.status = NULL,
	.config = NULL,
};
#endif /* NEVER */

const struct svdm_response svdm_rsp = {
	.identity = &svdm_response_identity,
#ifdef CONFIG_RT7207_ADAPTER
	.svids = &svdm_response_svids,
#else
	.svids = NULL,
#endif
	.modes = NULL,
#ifdef NEVER
	.modes = &svdm_response_modes,
	.enter_mode = &svdm_enter_mode,
	.amode = &dp_fx,
	.exit_mode = &svdm_exit_mode,
#endif /* NEVER */
};

int pd_custom_vdm(struct typec_hba *hba, int cnt, uint32_t *payload,
		  uint32_t **rpayload)
{
	int svid = PD_VDO_VID(payload[0]);
	int len = 0;

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
		dev_err(hba->dev, "Unstructured VDM 0x%04X\n", svid);

	/* make sure we have some payload */
	if (cnt == 0)
		return 0;

#ifdef CONFIG_RT7207_ADAPTER
	if (svid == RT7207_SVID) {
		if (DC_VDO_ACTION(payload[0]) == OP_ACK &&
			DC_VDO_CMD(payload[0]) == CMD_AUTH) {
			len = handle_dc_auth(hba->dc, cnt, payload, rpayload);
		} else if (hba->dc->auth_pass > 0) {
			mtk_direct_charging_payload(hba->dc, cnt, payload);
			hba->dc->data_in = true;
			wake_up(&hba->dc->wq);
		} else {
			dev_err(hba->dev, "%s can not handle RT7207 CMD\n", __func__);
		}
	}
#endif

#ifdef NEVER
	int cmd = PD_VDO_CMD(payload[0]);
	uint16_t dev_id = 0;
	int is_rw;

	/* make sure we have some payload */
	if (cnt == 0)
		return 0;

	switch (cmd) {
	case VDO_CMD_VERSION:
		/* guarantee last byte of payload is null character */
		*(payload + cnt - 1) = 0;
		CPRINTF("version: %s\n", (char *)(payload+1));
		break;
	case VDO_CMD_READ_INFO:
	case VDO_CMD_SEND_INFO:
		/* copy hash */
		if (cnt == 7) {
			dev_id = VDO_INFO_HW_DEV_ID(payload[6]);
			is_rw = VDO_INFO_IS_RW(payload[6]);

			CPRINTF("DevId:%d.%d SW:%d RW:%d\n",
				HW_DEV_ID_MAJ(dev_id),
				HW_DEV_ID_MIN(dev_id),
				VDO_INFO_SW_DBG_VER(payload[6]),
				is_rw);
		} else if (cnt == 6) {
			/* really old devices don't have last byte */
			pd_dev_store_rw_hash(port, dev_id, payload + 1,
					     SYSTEM_IMAGE_UNKNOWN);
		}
		break;
	case VDO_CMD_CURRENT:
		CPRINTF("Current: %dmA\n", payload[1]);
		break;
	case VDO_CMD_FLIP:
		/* board_flip_usb_mux(port); */
		break;
#ifdef CONFIG_USB_PD_LOGGING
	case VDO_CMD_GET_LOG:
		pd_log_recv_vdm(port, cnt, payload);
		break;
#endif /* CONFIG_USB_PD_LOGGING */
	}
#endif /* NEVER */
	return len;
}

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
#ifdef DISPLAYPORT
static int dp_flags;
/* DP Status VDM as returned by UFP */
static uint32_t dp_status;

static void svdm_safe_dp_mode(struct typec_hba *hba)
{
#ifdef NEVER
	const char *dp_str, *usb_str;

	enum typec_mux typec_mux_setting;

	/* make DP interface safe until configure */
	dp_flags = 0;
	dp_status = 0;

	/*
	 * Check current status, due to the mux may be switched to SS
	 * and SS device was attached before (for example: Type-C dock).
	 * To avoid broken the SS connection,
	 * keep the current setting if SS connection is enabled already.
	 */

	typec_mux_setting = (usb_mux_get(port, &dp_str, &usb_str) && usb_str) ?
			    TYPEC_MUX_USB : TYPEC_MUX_NONE;
	usb_mux_set(port, typec_mux_setting,
		    USB_SWITCH_CONNECT, pd_get_polarity(port));
#endif /* NEVER */
}

static int svdm_enter_dp_mode(struct typec_hba *hba, uint32_t mode_caps)
{
	/* Only enter mode if device is DFP_D capable */
	if (mode_caps & MODE_DP_SNK) {
		svdm_safe_dp_mode(hba);
		return 0;
	}

	return -1;
}

static int svdm_dp_status(struct typec_hba *hba, uint32_t *payload)
{
	int opos = pd_alt_mode(hba, USB_SID_DISPLAYPORT);

	payload[0] = VDO(USB_SID_DISPLAYPORT, 1,
			 CMD_DP_STATUS | VDO_OPOS(opos));
	payload[1] = VDO_DP_STATUS(0, /* HPD IRQ  ... not applicable */
				   0, /* HPD level ... not applicable */
				   0, /* exit DP? ... no */
				   0, /* usb mode? ... no */
				   0, /* multi-function ... no */
				   (!!(dp_flags & DP_FLAGS_DP_ON)),
				   0, /* power low? ... no */
				   (!!(dp_flags & DP_FLAGS_DP_ON)));
	return 2;
};

static int svdm_dp_config(struct typec_hba *hba, uint32_t *payload)
{
	int opos = pd_alt_mode(hba, USB_SID_DISPLAYPORT);
	/*int mf_pref = PD_VDO_DPSTS_MF_PREF(dp_status);*/
	int pin_mode = pd_dfp_dp_get_pin_mode(hba, dp_status);

	dev_err(hba->dev, "pin_mode=%d, dp_status=%x\n", pin_mode, dp_status);

	if (!pin_mode)
		return 0;

	if (!(PD_VDO_DPSTS_CONNECT(dp_status) > 0x0))
		return 0;

	payload[0] = VDO(USB_SID_DISPLAYPORT, 1,
			 CMD_DP_CONFIG | VDO_OPOS(opos));

	payload[1] = VDO_DP_CFG(pin_mode,      /* pin mode */
				1,             /* DPv1.3 signaling */
				2);            /* UFP_U connected as UFP_D */
	return 2;
};

static void svdm_dp_post_config(struct typec_hba *hba)
{
	dp_flags |= DP_FLAGS_DP_ON;
	if (!(dp_flags & DP_FLAGS_HPD_HI_PENDING))
		return;
}

static int svdm_dp_attention(struct typec_hba *hba, uint32_t *payload)
{
	int lvl = PD_VDO_DPSTS_HPD_LVL(payload[1]);
	int irq = PD_VDO_DPSTS_HPD_IRQ(payload[1]);

	dp_status = payload[1];

	/* Its initial DP status message prior to config */
	if (!(dp_flags & DP_FLAGS_DP_ON)) {
		if (lvl)
			dp_flags |= DP_FLAGS_HPD_HI_PENDING;
		return 1;
	}

	if (irq) {
		dev_err(hba->dev, "---\n");
	} else {
		dev_err(hba->dev, "ERR:HPD:IRQ&LOW\n");
		return 0; /* nak */
	}

	/* ack */
	return 1;
}

static void svdm_exit_dp_mode(struct typec_hba *hba)
{
	svdm_safe_dp_mode(hba);
}
#endif

#ifdef CONFIG_RT7207_ADAPTER
static int svdm_enter_dc_mode(struct typec_hba *hba, uint32_t mode_caps)
{
	if (mode_caps == 1) {
		hba->dc->auth_pass = -1;
		dev_err(hba->dev, "Enter PE3.0 mode\n");

		typec_auxadc_low_register(hba);
		typec_auxadc_set_thresholds(hba, SNK_VRPUSB_AUXADC_MIN_VAL, 0);
		mt6336_enable_interrupt(TYPE_C_L_MIN, "TYPE_C_L_MIN");
		return 0;
	}

	return -1;
}

static void svdm_exit_dc_mode(struct typec_hba *hba)
{
	hba->dc->auth_pass = -1;

	typec_disable_auxadc_irq(hba);
	mt6336_disable_interrupt(TYPE_C_L_MIN, "TYPE_C_L_MIN");

	dev_err(hba->dev, "Exit PE3.0 mode\n");
}
#endif

const struct svdm_amode_fx supported_modes[] = {
#ifdef DISPLAYPORT
	{
		.svid = USB_SID_DISPLAYPORT,
		.enter = &svdm_enter_dp_mode,
		.status = &svdm_dp_status,
		.config = &svdm_dp_config,
		.post_config = &svdm_dp_post_config,
		.attention = &svdm_dp_attention,
		.exit = &svdm_exit_dp_mode,
	},
#endif /* NEVER */
#ifdef CONFIG_RT7207_ADAPTER
	{
		.svid = RT7207_SVID,
		.enter = &svdm_enter_dc_mode,
		.status = NULL,
		.config = NULL,
		.post_config = NULL,
		.attention = NULL,
		.exit = &svdm_exit_dc_mode,
	},
#endif
};
const int supported_modes_cnt = ARRAY_SIZE(supported_modes);
#endif /* CONFIG_USB_PD_ALT_MODE_DFP */

#endif
