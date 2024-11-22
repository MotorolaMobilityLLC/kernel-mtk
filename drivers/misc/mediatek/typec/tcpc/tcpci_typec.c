// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/power_supply.h>

#include "inc/tcpci.h"
#include "inc/tcpci_typec.h"
#include "inc/tcpci_timer.h"
#include "inc/std_tcpci_v10.h"

#ifdef CONFIG_MOTO_CHANNEL_SWITCH
#include "mtk_charger.h"
#endif

#ifdef CONFIG_MOTO_CHANNEL_SWITCH
#define ON 1
#define CC_SUM_MAX 30
#define CC_SUM_MIN 0
static bool g_flag = false;
#endif

enum TYPEC_WAIT_PS_STATE {
	TYPEC_WAIT_PS_DISABLE = 0,
	TYPEC_WAIT_PS_SRC_VSAFE0V,
	TYPEC_WAIT_PS_SNK_VSAFE5V,
	TYPEC_WAIT_PS_SRC_VSAFE5V,
#if CONFIG_TYPEC_CAP_DBGACC
	TYPEC_WAIT_PS_DBG_VSAFE5V,
#endif	/* CONFIG_TYPEC_CAP_DBGACC */
};

enum TYPEC_ROLE_SWAP_STATE {
	TYPEC_ROLE_SWAP_NONE = 0,
	TYPEC_ROLE_SWAP_TO_SNK,
	TYPEC_ROLE_SWAP_TO_SRC,
};

#if TYPEC_INFO_ENABLE
static const char *const typec_wait_ps_name[] = {
	"Disable",
	"SRC_VSafe0V",
	"SNK_VSafe5V",
	"SRC_VSafe5V",
#if CONFIG_TYPEC_CAP_DBGACC
	"DBG_VSafe5V",
#endif	/* CONFIG_TYPEC_CAP_DBGACC */
};
#endif	/* TYPEC_INFO_ENABLE */

static inline void typec_wait_ps_change(struct tcpc_device *tcpc,
					enum TYPEC_WAIT_PS_STATE state)
{
#if TYPEC_INFO_ENABLE
	uint8_t old_state = tcpc->typec_wait_ps_change;
	uint8_t new_state = state;

	if (new_state != old_state)
		TYPEC_INFO("wait_ps=%s\n", typec_wait_ps_name[new_state]);
#endif	/* TYPEC_INFO_ENABLE */

#if CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
	if (state == TYPEC_WAIT_PS_SRC_VSAFE0V) {
		mutex_lock(&tcpc->access_lock);
		tcpci_enable_discharge(tcpc, true, 0);
		tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_SAFE0V_TOUT);
		mutex_unlock(&tcpc->access_lock);
	}
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */

	if (tcpc->typec_wait_ps_change == TYPEC_WAIT_PS_SRC_VSAFE0V
		&& state != TYPEC_WAIT_PS_SRC_VSAFE0V) {
#if CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY
		tcpc_disable_timer(tcpc, TYPEC_RT_TIMER_SAFE0V_DELAY);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY */

#if CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
		mutex_lock(&tcpc->access_lock);
		tcpc_disable_timer(tcpc, TYPEC_RT_TIMER_SAFE0V_TOUT);
		tcpci_enable_discharge(tcpc, false, 0);
		mutex_unlock(&tcpc->access_lock);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */
	}

	tcpc->typec_wait_ps_change = state;
}

#define typec_check_cc1(cc)	\
	(typec_get_cc1() == cc)

#define typec_check_cc2(cc)	\
	(typec_get_cc2() == cc)

#define typec_check_cc(cc1, cc2)	\
	(typec_check_cc1(cc1) && typec_check_cc2(cc2))

#define typec_check_cc_both(res)	\
	(typec_check_cc(res, res))

#define typec_check_cc_reversible(cc1, cc2)	\
	(typec_check_cc(cc1, cc2) || typec_check_cc(cc2, cc1))

#define typec_check_cc_any(res)		\
	(typec_check_cc1(res) || typec_check_cc2(res))

#define typec_is_drp_toggling() \
	(typec_get_cc1() == TYPEC_CC_DRP_TOGGLING)

#define typec_is_cc_open()	\
	typec_check_cc_both(TYPEC_CC_VOLT_OPEN)

#define typec_is_sink_with_emark()	\
	(typec_get_cc1() + typec_get_cc2() == \
	TYPEC_CC_VOLT_RA+TYPEC_CC_VOLT_RD)

#define typec_is_cable_only()	\
	(typec_get_cc1() + typec_get_cc2() == TYPEC_CC_VOLT_RA)

#define typec_is_cc_no_res()	\
	(typec_is_drp_toggling() || typec_is_cc_open())

#define typec_is_src_detected()	\
	(typec_get_cc_sum() >= TYPEC_CC_VOLT_SNK_DFT &&	\
	 typec_get_cc_sum() <= TYPEC_CC_VOLT_SNK_3_0)

#define typec_is_snk_detected()	\
	(typec_check_cc_reversible(TYPEC_CC_VOLT_RD, TYPEC_CC_VOLT_OPEN) || \
	 typec_check_cc_reversible(TYPEC_CC_VOLT_RD, TYPEC_CC_VOLT_RA))

static inline int typec_enable_vconn(struct tcpc_device *tcpc)
{
	if (!typec_is_sink_with_emark())
		return 0;

	if (tcpc->tcpc_vconn_supply == TCPC_VCONN_SUPPLY_NEVER)
		return 0;

	return tcpci_set_vconn(tcpc, true);
}

/*
 * [BLOCK] TYPEC Connection State Definition
 */

#if TYPEC_INFO_ENABLE || TCPC_INFO_ENABLE || TYPEC_DBG_ENABLE
static const char *const typec_state_names[] = {
	"Disabled",
	"ErrorRecovery",

	"Unattached.SNK",
	"Unattached.SRC",

	"AttachWait.SNK",
	"AttachWait.SRC",

	"Attached.SNK",
	"Attached.SRC",

#if CONFIG_TYPEC_CAP_TRY_SOURCE
	"Try.SRC",
	"TryWait.SNK",
	"TryWait.SNK.PE",
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	"Try.SNK",
	"TryWait.SRC",

	"AudioAccessory",
#if CONFIG_TYPEC_CAP_DBGACC
	"DebugAccessory",
#endif	/* CONFIG_TYPEC_CAP_DBGACC */

#if CONFIG_TYPEC_CAP_DBGACC_SNK
	"DBGACC.SNK",
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

	"Custom.SRC",

#if CONFIG_TYPEC_CAP_NORP_SRC
	"NoRp.SRC",
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

#if CONFIG_TYPEC_CAP_ROLE_SWAP
	"RoleSwap",
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

#if CONFIG_WATER_DETECTION
	"WaterProtection",
#endif /* CONFIG_WATER_DETECTION */

	"ForeignObjectProtection",

	"TypeC.OTP",

	"UnattachWait.PE",
};
#endif /* TYPEC_INFO_ENABLE || TCPC_INFO_ENABLE || TYPEC_DBG_ENABLE */

static inline void typec_transfer_state(struct tcpc_device *tcpc,
					enum TYPEC_CONNECTION_STATE state)
{
	TYPEC_INFO("** %s\n",
		   state < ARRAY_SIZE(typec_state_names) ?
		   typec_state_names[state] : "Unknown");
	tcpc->typec_state = state;
}

#define TYPEC_NEW_STATE(state)  \
	(typec_transfer_state(tcpc, state))

/*
 * [BLOCK] TypeC Alert Attach Status Changed
 */

#if TYPEC_INFO_ENABLE || TYPEC_DBG_ENABLE
static const char *const typec_attach_names[] = {
	"NULL",
	"SINK",
	"SOURCE",
	"AUDIO",
	"DEBUG",

	"DBGACC_SNK",
	"CUSTOM_SRC",
	"NORP_SRC",
	"PROTECTION",
};
#endif /* TYPEC_INFO_ENABLE || TYPEC_DBG_ENABLE */

#ifdef CONFIG_SUPPORT_MMI_ADAPTER
static bool g_tcpc_pd_adapter_flag = false;
void mmi_tcpc_set_pd_flag(bool flag){
	pr_notice("%s  pd flag:%d\n", __func__, flag);
	g_tcpc_pd_adapter_flag = flag;
}

bool mmi_tcpc_get_pd_flag(void){
	pr_notice("%s  pd flag:%d\n", __func__, g_tcpc_pd_adapter_flag);
	return g_tcpc_pd_adapter_flag;
}
#endif

static int typec_alert_attach_state_change(struct tcpc_device *tcpc)
{
	int ret = 0;

	if (tcpc->typec_attach_old == tcpc->typec_attach_new) {
		TYPEC_INFO("Attached-> %s(repeat)\n",
			typec_attach_names[tcpc->typec_attach_new]);
		return 0;
	}

	TYPEC_INFO("Attached-> %s\n",
		   typec_attach_names[tcpc->typec_attach_new]);

#ifdef CONFIG_SUPPORT_MMI_ADAPTER
	if (tcpc->typec_remote_rp_level == TYPEC_CC_VOLT_SNK_3_0 &&
	    tcpc->typec_local_rp_level == TYPEC_CC_RP_DFT &&
	    tcpc->typec_attach_new != TYPEC_UNATTACHED ) {
		mmi_tcpc_set_pd_flag(true);
	} else if (tcpc->typec_attach_new == TYPEC_UNATTACHED) {
		mmi_tcpc_set_pd_flag(false);
	}
#endif

	/* Report function */
	ret = tcpci_report_usb_port_changed(tcpc);

	tcpc->typec_attach_old = tcpc->typec_attach_new;
	return ret;
}

static inline void typec_enable_low_power_mode(struct tcpc_device *tcpc)
{
	tcpc->typec_lpm = true;
	tcpc->typec_lpm_tout = 5000;
	tcpc_enable_lpm_timer(tcpc, true);
}

static inline int typec_disable_low_power_mode(struct tcpc_device *tcpc)
{
	tcpc_enable_lpm_timer(tcpc, false);
	return tcpci_set_low_power_mode(tcpc, false);
}

/*
 * [BLOCK] Unattached Entry
 */

#if CONFIG_TYPEC_CAP_ROLE_SWAP
static inline int typec_handle_role_swap_start(struct tcpc_device *tcpc)
{
	uint8_t role_swap = tcpc->typec_during_role_swap;

	if (role_swap == TYPEC_ROLE_SWAP_TO_SNK) {
		TYPEC_INFO("Role Swap to Sink\n");
		tcpci_set_cc(tcpc, TYPEC_CC_RD);
	} else if (role_swap == TYPEC_ROLE_SWAP_TO_SRC) {
		TYPEC_INFO("Role Swap to Source\n");
		tcpci_set_cc(tcpc,
			TYPEC_CC_PULL(tcpc->typec_local_rp_level, TYPEC_CC_RP));
	}

	return 0;
}

static inline int typec_handle_role_swap_stop(struct tcpc_device *tcpc)
{
	if (tcpc->typec_during_role_swap) {
		tcpc->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		if (tcpc->typec_attach_old == TYPEC_UNATTACHED) {
			TYPEC_INFO("TypeC Role Swap Failed\n");
			tcpc_enable_timer(tcpc, TYPEC_TIMER_PDDEBOUNCE);
		}
	}

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

static inline void typec_unattached_src_and_drp_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_unattached_src);
	tcpci_set_cc(tcpc, TYPEC_CC_RP);
	tcpc_enable_timer(tcpc, TYPEC_TIMER_DRP_SRC_TOGGLE);
}

static inline void typec_unattached_snk_and_drp_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_unattached_snk);
	tcpci_set_cc(tcpc, TYPEC_CC_DRP);
	typec_enable_low_power_mode(tcpc);
}

static inline void typec_unattached_cc_entry(struct tcpc_device *tcpc)
{
#if CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc->typec_during_role_swap) {
		TYPEC_NEW_STATE(typec_role_swap);
		if (typec_is_cc_open())
			typec_handle_role_swap_start(tcpc);
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	if (tcpc->typec_role >= TYPEC_ROLE_TRY_SRC)
		tcpc_reset_typec_try_timer(tcpc);

#if CONFIG_CABLE_TYPE_DETECTION
	if (tcpc->tcpc_flags & TCPC_FLAGS_CABLE_TYPE_DETECTION)
		tcpc_typec_handle_ctd(tcpc, TCPC_CABLE_TYPE_NONE);
#endif /* CONFIG_CABLE_TYPE_DETECTION */
	tcpci_set_cc_hidet(tcpc, false);
	tcpci_set_auto_dischg_discnt(tcpc, false);

	tcpc->typec_role = tcpc->typec_role_new;
	switch (tcpc->typec_role) {
	case TYPEC_ROLE_SNK:
		TYPEC_NEW_STATE(typec_unattached_snk);
		tcpci_set_cc(tcpc, TYPEC_CC_RD);
		typec_enable_low_power_mode(tcpc);
		break;
	case TYPEC_ROLE_SRC:
		TYPEC_NEW_STATE(typec_unattached_src);
		tcpci_set_cc(tcpc, TYPEC_CC_RP);
		typec_enable_low_power_mode(tcpc);
		break;
	case TYPEC_ROLE_TRY_SRC:
		if (tcpc->typec_state == typec_errorrecovery) {
			typec_unattached_src_and_drp_entry(tcpc);
			break;
		}
		fallthrough;
	default:
		switch (tcpc->typec_state) {
		case typec_attachwait_snk:
		case typec_audioaccessory:
			typec_unattached_src_and_drp_entry(tcpc);
			break;
		default:
			typec_unattached_snk_and_drp_entry(tcpc);
			break;
		}
		break;
	}
}

static void typec_unattached_power_entry(struct tcpc_device *tcpc)
{
	tcpc->typec_usb_sink_curr = CONFIG_TYPEC_SNK_CURR_DFT;
	typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DISABLE);

	if (tcpc->typec_power_ctrl) {
		tcpci_set_vconn(tcpc, false);
		tcpci_disable_vbus_control(tcpc);
		tcpci_report_power_control(tcpc, false);
	}
}

static void typec_unattached_entry(struct tcpc_device *tcpc)
{
	if (tcpc->typec_power_ctrl)
		tcpci_set_vconn(tcpc, false);
	typec_unattached_cc_entry(tcpc);
	typec_unattached_power_entry(tcpc);
}

static bool typec_is_in_protection_states(struct tcpc_device *tcpc)
{
#if CONFIG_WATER_DETECTION
	if (tcpc->typec_state == typec_water_protection)
		return true;
#endif /* CONFIG_WATER_DETECTION */

	if ((tcpc->tcpc_flags & TCPC_FLAGS_FOREIGN_OBJECT_DETECTION) &&
	    tcpc->typec_state == typec_foreign_object_protection)
		return true;

	if ((tcpc->tcpc_flags & TCPC_FLAGS_TYPEC_OTP) &&
	    tcpc->typec_state == typec_otp)
		return true;

	return false;
}

static void typec_attach_new_unattached(struct tcpc_device *tcpc)
{
	tcpc->typec_attach_new = TYPEC_UNATTACHED;
	tcpc->typec_remote_rp_level = TYPEC_CC_VOLT_SNK_DFT;
	tcpc->typec_polarity = false;
}

static void typec_unattach_wait_pe_idle_entry(struct tcpc_device *tcpc)
{
	typec_attach_new_unattached(tcpc);

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	if (tcpc->pd_pe_running) {
		TYPEC_NEW_STATE(typec_unattachwait_pe);
		return;
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

	typec_unattached_entry(tcpc);
}

static void typec_postpone_state_change(struct tcpc_device *tcpc)
{
	TYPEC_DBG("Postpone AlertChange\n");
	tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_STATE_CHANGE);
}

static void typec_cc_open_entry(struct tcpc_device *tcpc, uint8_t state)
{
	typec_attach_new_unattached(tcpc);
	TYPEC_NEW_STATE(state);
	tcpci_set_cc(tcpc, TYPEC_CC_OPEN);
	if (tcpc->typec_state == typec_disabled)
		typec_enable_low_power_mode(tcpc);
	else
		typec_disable_low_power_mode(tcpc);
	typec_unattached_power_entry(tcpc);
	typec_alert_attach_state_change(tcpc);
	if (typec_is_in_protection_states(tcpc)) {
		tcpc->typec_attach_new = TYPEC_PROTECTION;
		typec_postpone_state_change(tcpc);
	}
}

static inline void typec_error_recovery_entry(struct tcpc_device *tcpc)
{
	typec_cc_open_entry(tcpc, typec_errorrecovery);
	tcpc_reset_typec_debounce_timer(tcpc);
	tcpc_enable_timer(tcpc, TYPEC_TIMER_ERROR_RECOVERY);
}

static inline void typec_disable_entry(struct tcpc_device *tcpc)
{
	typec_cc_open_entry(tcpc, typec_disabled);
}

/*
 * [BLOCK] Attached Entry
 */

static inline int typec_set_polarity(struct tcpc_device *tcpc,
					bool polarity)
{
	tcpc->typec_polarity = polarity;
	return tcpci_set_polarity(tcpc, polarity);
}

static inline int typec_set_plug_orient(struct tcpc_device *tcpc,
				uint8_t pull, bool polarity)
{
	int ret = typec_set_polarity(tcpc, polarity);

	if (ret)
		return ret;

	return tcpci_set_cc(tcpc, pull);
}

static inline void typec_source_attached_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_attached_src);
	typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_SRC_VSAFE5V);

	typec_set_plug_orient(tcpc,
		TYPEC_CC_PULL(tcpc->typec_local_rp_level, TYPEC_CC_RP),
		typec_check_cc2(TYPEC_CC_VOLT_RD));

	tcpci_report_power_control(tcpc, true);
	typec_enable_vconn(tcpc);
	tcpci_source_vbus(tcpc,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, -1);
}

static inline void typec_sink_attached_entry(struct tcpc_device *tcpc)
{
#if CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc->typec_during_role_swap) {
		tcpc->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		tcpc_disable_timer(tcpc, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	TYPEC_NEW_STATE(typec_attached_snk);
	tcpc->typec_attach_new = TYPEC_ATTACHED_SNK;

	tcpci_set_auto_dischg_discnt(tcpc, true);
	typec_set_plug_orient(tcpc, TYPEC_CC_RD,
		!typec_check_cc2(TYPEC_CC_VOLT_OPEN));
	tcpc->typec_remote_rp_level = typec_get_cc_res();

	tcpci_report_power_control(tcpc, true);
	tcpci_sink_vbus(tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}

static inline void typec_custom_src_attached_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_attached_custom_src);
	tcpc->typec_attach_new = TYPEC_ATTACHED_CUSTOM_SRC;

	tcpci_set_auto_dischg_discnt(tcpc, true);
	tcpci_set_cc(tcpc, TYPEC_CC_RD);
	tcpc->typec_remote_rp_level = typec_get_cc1();

	tcpci_report_power_control(tcpc, true);
	tcpci_sink_vbus(tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}

#if CONFIG_TYPEC_CAP_DBGACC_SNK

static inline uint8_t typec_get_sink_dbg_acc_rp_level(
	int cc1, int cc2)
{
	if (cc2 == TYPEC_CC_VOLT_SNK_DFT)
		return cc1;

	return TYPEC_CC_VOLT_SNK_DFT;
}

static inline void typec_sink_dbg_acc_attached_entry(struct tcpc_device *tcpc)
{
	uint8_t cc1 = typec_get_cc1();
	uint8_t cc2 = typec_get_cc2();
	bool polarity = cc2 > cc1;
	uint8_t rp_level = TYPEC_CC_VOLT_SNK_DFT;

	if (cc1 == cc2) {
		typec_custom_src_attached_entry(tcpc);
		return;
	}

	TYPEC_NEW_STATE(typec_attached_dbgacc_snk);
	tcpc->typec_attach_new = TYPEC_ATTACHED_DBGACC_SNK;

	if (polarity)
		rp_level = typec_get_sink_dbg_acc_rp_level(cc2, cc1);
	else
		rp_level = typec_get_sink_dbg_acc_rp_level(cc1, cc2);

	tcpci_set_auto_dischg_discnt(tcpc, true);
	typec_set_plug_orient(tcpc, TYPEC_CC_RD, polarity);
	tcpc->typec_remote_rp_level = rp_level;

	tcpci_report_power_control(tcpc, true);
	tcpci_sink_vbus(tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}
#else
static inline void typec_sink_dbg_acc_attached_entry(struct tcpc_device *tcpc)
{
	typec_custom_src_attached_entry(tcpc);
}
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

/*
 * [BLOCK] NoRpSRC Entry
 */

#if CONFIG_TYPEC_CAP_NORP_SRC
static bool typec_try_norp_src(struct tcpc_device *tcpc)
{
	if (tcpc->typec_state == typec_unattached_snk) {
		if (tcpci_check_vbus_valid(tcpc) &&
		    typec_is_cc_no_res()) {
			TYPEC_DBG("norp_src=1\n");
			tcpc_enable_timer(tcpc, TYPEC_TIMER_NORP_SRC);
			return true;
		}
		TYPEC_DBG("disable norp_src timer\n");
		tcpc_disable_timer(tcpc, TYPEC_TIMER_NORP_SRC);
	}

	if (tcpc->typec_state == typec_attached_norp_src) {
		if (typec_is_cc_no_res()) {
			if (tcpci_check_vbus_valid(tcpc)) {
				TYPEC_DBG("keep norp_src\n");
			} else {
				TYPEC_DBG("norp_src=0\n");
				typec_unattach_wait_pe_idle_entry(tcpc);
				typec_alert_attach_state_change(tcpc);
			}
		} else if (!typec_is_cable_only()) {
			TYPEC_DBG("enter attachwait from norp_src\n");
			typec_attach_new_unattached(tcpc);
			if (tcpc_typec_is_act_as_sink_role(tcpc))
				TYPEC_NEW_STATE(typec_unattached_snk);
			else
				TYPEC_NEW_STATE(typec_unattached_src);
			typec_unattached_power_entry(tcpc);
			typec_alert_attach_state_change(tcpc);
			return false;
		}
		return true;
	}

	return false;
}

static inline void typec_norp_src_attached_entry(struct tcpc_device *tcpc)
{
	typec_disable_low_power_mode(tcpc);

	TYPEC_NEW_STATE(typec_attached_norp_src);
	tcpc->typec_attach_new = TYPEC_ATTACHED_NORP_SRC;

	if (tcpc->typec_role >= TYPEC_ROLE_DRP)
		tcpci_set_cc(tcpc, TYPEC_CC_DRP);
	else
		tcpci_set_cc(tcpc, TYPEC_CC_RD);
	tcpc->typec_remote_rp_level = TYPEC_CC_VOLT_SNK_DFT;
	tcpci_report_power_control(tcpc, true);
	tcpci_sink_vbus(tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, 500);
}
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

/*
 * [BLOCK] Try.SRC / TryWait.SNK
 */

#if CONFIG_TYPEC_CAP_TRY_SOURCE
static inline void typec_try_src_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_try_src);

	tcpci_set_cc(tcpc, TYPEC_CC_RP);
	tcpc->typec_drp_try_timeout = false;
	tcpc_enable_timer(tcpc, TYPEC_TRY_TIMER_DRP_TRY);
	tcpc_enable_timer(tcpc, TYPEC_TRY_TIMER_TRY_TOUT);
}

static inline void typec_trywait_snk_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_trywait_snk);

	if (tcpc->typec_power_ctrl)
		tcpci_set_vconn(tcpc, false);
	tcpci_set_cc(tcpc, TYPEC_CC_RD);
	typec_unattached_power_entry(tcpc);

	tcpc_enable_timer(tcpc, TYPEC_TIMER_PDDEBOUNCE);
}

static inline void typec_trywait_snk_pe_entry(struct tcpc_device *tcpc)
{
	typec_attach_new_unattached(tcpc);

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	if (tcpc->pd_pe_running) {
		TYPEC_NEW_STATE(typec_trywait_snk_pe);
		return;
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

	typec_trywait_snk_entry(tcpc);
}

#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

/*
 * [BLOCK] Try.SNK / TryWait.SRC
 */

static inline void typec_try_snk_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_try_snk);

	tcpci_set_cc(tcpc, TYPEC_CC_RD);
	tcpc->typec_drp_try_timeout = false;
	tcpc_enable_timer(tcpc, TYPEC_TRY_TIMER_DRP_TRY);
}

static inline void typec_trywait_src_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_trywait_src);

	tcpci_set_cc(tcpc, TYPEC_CC_RP);
	tcpc->typec_drp_try_timeout = false;
	tcpc_enable_timer(tcpc, TYPEC_TRY_TIMER_DRP_TRY);
}

/*
 * [BLOCK] Attach / Detach
 */

static inline void typec_cc_snk_detect_vsafe5v_entry(struct tcpc_device *tcpc)
{
	if (!typec_check_cc_any(TYPEC_CC_VOLT_OPEN)) {	/* Both Rp */
		typec_sink_dbg_acc_attached_entry(tcpc);
		return;
	}

#if CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc->typec_role == TYPEC_ROLE_TRY_SRC &&
	    tcpc->typec_state == typec_attachwait_snk) {
		typec_try_src_entry(tcpc);
		return;
	}
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_sink_attached_entry(tcpc);
}

static inline void typec_cc_snk_detect_entry(struct tcpc_device *tcpc)
{
	/* If Port Partner act as Source without VBUS, wait vSafe5V */
	if (tcpci_check_vbus_valid(tcpc))
		typec_cc_snk_detect_vsafe5v_entry(tcpc);
	else
		typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_SNK_VSAFE5V);
}

static inline void typec_cc_src_detect_vsafe0v_entry(struct tcpc_device *tcpc)
{
	if (tcpc->typec_role == TYPEC_ROLE_TRY_SNK &&
	    tcpc->typec_state == typec_attachwait_src) {
		typec_try_snk_entry(tcpc);
		return;
	}

	typec_source_attached_entry(tcpc);
}

static inline void typec_cc_src_detect_entry(struct tcpc_device *tcpc)
{
	/* If Port Partner act as Sink with low VBUS, wait vSafe0v */
	if (tcpci_check_vsafe0v(tcpc))
		typec_cc_src_detect_vsafe0v_entry(tcpc);
	else
		typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_SRC_VSAFE0V);
}

static inline void typec_cc_src_remove_entry(struct tcpc_device *tcpc)
{
	typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DISABLE);

#if CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc->typec_role == TYPEC_ROLE_TRY_SRC &&
	    !tcpc->typec_during_role_swap) {
		switch (tcpc->typec_state) {
		case typec_attached_src:
			typec_trywait_snk_pe_entry(tcpc);
			return;
		case typec_try_src:
			if (tcpci_check_vsafe0v(tcpc))
				typec_trywait_snk_entry(tcpc);
			else
				typec_wait_ps_change(tcpc,
					TYPEC_WAIT_PS_SRC_VSAFE0V);
			return;
		}
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_unattach_wait_pe_idle_entry(tcpc);
}

static inline void typec_cc_snk_remove_entry(struct tcpc_device *tcpc)
{
	typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DISABLE);

	if (tcpc->typec_state == typec_try_snk) {
		typec_trywait_src_entry(tcpc);
		return;
	}

	typec_unattach_wait_pe_idle_entry(tcpc);
}

/*
 * [BLOCK] CC Change (after debounce)
 */

static inline void typec_debug_acc_attached_entry(struct tcpc_device *tcpc)
{
#if CONFIG_TYPEC_CAP_DBGACC
	TYPEC_NEW_STATE(typec_debugaccessory);
	typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DBG_VSAFE5V);

	tcpci_set_cc(tcpc,
		TYPEC_CC_PULL(tcpc->typec_local_rp_level, TYPEC_CC_RP));

	tcpci_report_power_control(tcpc, true);
	tcpci_source_vbus(tcpc,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, -1);
#endif	/* CONFIG_TYPEC_CAP_DBGACC */
}

#if CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS
static int typec_audio_acc_sink_vbus(
	struct tcpc_device *tcpc, bool vbus_valid)
{
	if (vbus_valid) {
		tcpci_report_power_control(tcpc, true);
		tcpci_sink_vbus(tcpc,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, 500);
	} else {
		tcpci_sink_vbus(tcpc,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_0V, 0);
		tcpci_report_power_control(tcpc, false);
	}

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS */

static inline void typec_audio_acc_attached_entry(struct tcpc_device *tcpc)
{
	TYPEC_NEW_STATE(typec_audioaccessory);
	tcpc->typec_attach_new = TYPEC_ATTACHED_AUDIO;

	tcpci_set_cc(tcpc,
		TYPEC_CC_PULL(tcpc->typec_local_rp_level, TYPEC_CC_RP));

#if CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS
	if (tcpci_check_vbus_valid(tcpc))
		typec_audio_acc_sink_vbus(tcpc, true);
#endif	/* CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS */
}

static inline bool typec_cc_change_source_entry(struct tcpc_device *tcpc)
{
	bool src_remove = false;

	switch (tcpc->typec_state) {
	case typec_attached_src:
		if (typec_get_cc_res() != TYPEC_CC_VOLT_RD)
			src_remove = true;
		break;
	case typec_audioaccessory:
		if (!typec_check_cc_both(TYPEC_CC_VOLT_RA))
			src_remove = true;
		break;
#if CONFIG_TYPEC_CAP_DBGACC
	case typec_debugaccessory:
		if (!typec_check_cc_both(TYPEC_CC_VOLT_RD))
			src_remove = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_DBGACC */
	case typec_trywait_src:
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */
#if CONFIG_TYPEC_CAP_ROLE_SWAP
	case typec_role_swap:
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */
		if (typec_is_snk_detected())
			typec_cc_src_detect_entry(tcpc);
		else
			src_remove = true;
		break;
	default:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RD))
			typec_debug_acc_attached_entry(tcpc);
		else if (typec_check_cc_both(TYPEC_CC_VOLT_RA))
			typec_audio_acc_attached_entry(tcpc);
		else if (typec_check_cc_any(TYPEC_CC_VOLT_RD))
			typec_cc_src_detect_entry(tcpc);
		else
			src_remove = true;
		break;
	}

	if (src_remove)
		typec_cc_src_remove_entry(tcpc);

	return true;
}

static inline bool typec_attached_snk_cc_change(struct tcpc_device *tcpc)
{
	uint8_t cc_res = typec_get_cc_res();
	bool changed = false;
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	struct pd_port *pd_port = &tcpc->pd_port;
#endif	/* CONFIG_USB_POWER_DELIVERY */

	if (cc_res != tcpc->typec_remote_rp_level) {
		TYPEC_DBG("RpLvl Change\n");
		tcpc->typec_remote_rp_level = cc_res;
		changed = true;
	}

#if CONFIG_USB_PD_REV30
	if (pd_port->pe_data.pd_connected && pd_check_rev30(pd_port) &&
	    cc_res == TYPEC_CC_VOLT_SNK_3_0)
		pd_put_sink_tx_event(tcpc, cc_res);
#endif	/* CONFIG_USB_PD_REV30 */

	if (changed)
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
		if (!pd_port->pe_data.pd_connected)
#endif	/* CONFIG_USB_POWER_DELIVERY */
			tcpci_sink_vbus(tcpc,
				TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);

	return true;
}

static inline bool typec_cc_change_sink_entry(struct tcpc_device *tcpc)
{
	bool snk_remove = false;

	switch (tcpc->typec_state) {
	case typec_attached_snk:
#if CONFIG_TYPEC_CAP_DBGACC_SNK
	case typec_attached_dbgacc_snk:
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */
		if (typec_get_cc_res() == TYPEC_CC_VOLT_OPEN)
			snk_remove = true;
		else
			typec_attached_snk_cc_change(tcpc);
		break;
	case typec_attached_custom_src:
		if (typec_check_cc_any(TYPEC_CC_VOLT_OPEN))
			snk_remove = true;
		else
			typec_attached_snk_cc_change(tcpc);
		break;
	case typec_try_snk:
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_trywait_snk:
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */
#if CONFIG_TYPEC_CAP_ROLE_SWAP
	case typec_role_swap:
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */
		if (typec_is_src_detected())
			typec_cc_snk_detect_entry(tcpc);
		else
			snk_remove = true;
		break;
	default:
		if (!typec_is_cc_open())
			typec_cc_snk_detect_entry(tcpc);
		else
			snk_remove = true;
	}

	if (snk_remove)
		typec_cc_snk_remove_entry(tcpc);

	return true;
}

bool tcpc_typec_is_act_as_sink_role(struct tcpc_device *tcpc)
{
	bool as_sink = true;

	switch (TYPEC_CC_PULL_GET_RES(tcpc->typec_local_cc)) {
	case TYPEC_CC_RP:
		as_sink = false;
		break;
	case TYPEC_CC_RD:
		as_sink = true;
		break;
	case TYPEC_CC_DRP:
		as_sink = typec_get_cc_sum() >= TYPEC_CC_VOLT_SNK_DFT;
		break;
	}

	return as_sink;
}
EXPORT_SYMBOL(tcpc_typec_is_act_as_sink_role);

static inline bool typec_handle_cc_changed_entry(struct tcpc_device *tcpc)
{
	/* refresh vbus_level too */
	TYPEC_INFO("[CC_Change] %d/%d, vbus_valid = %d\n",
		   typec_get_cc1(), typec_get_cc2(),
		   tcpci_check_vbus_valid_from_ic(tcpc));

	tcpc->typec_attach_new = tcpc->typec_attach_old;

	if (tcpc_typec_is_act_as_sink_role(tcpc))
		typec_cc_change_sink_entry(tcpc);
	else
		typec_cc_change_source_entry(tcpc);

	typec_alert_attach_state_change(tcpc);
	return true;
}

/*
 * [BLOCK] Handle cc-change event (from HW)
 */

static inline void typec_attach_wait_entry(struct tcpc_device *tcpc)
{
	bool as_sink = tcpc_typec_is_act_as_sink_role(tcpc);
	uint8_t cc_res = typec_get_cc_res();
#if CONFIG_USB_PD_REV30
	struct pd_port *pd_port = &tcpc->pd_port;
#endif	/* CONFIG_USB_PD_REV30 */

	switch (tcpc->typec_state) {
	case typec_attached_src:
#if CONFIG_TYPEC_CAP_DBGACC
	case typec_debugaccessory:
#endif	/* CONFIG_TYPEC_CAP_DBGACC */
		break;
	default:
		typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DISABLE);
		break;
	}

	switch (tcpc->typec_state) {
	case typec_attached_snk:
#if CONFIG_TYPEC_CAP_DBGACC_SNK
	case typec_attached_dbgacc_snk:
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */
	case typec_attached_custom_src:
		TYPEC_DBG("RpLvl Alert\n");
#if CONFIG_USB_PD_REV30
		if (pd_port->pe_data.pd_connected && pd_check_rev30(pd_port) &&
		    cc_res != TYPEC_CC_VOLT_SNK_3_0)
			pd_put_sink_tx_event(tcpc, cc_res);
#endif	/* CONFIG_USB_PD_REV30 */
		tcpc_enable_timer(tcpc, TYPEC_TIMER_PDDEBOUNCE);
		return;

	case typec_attached_src:
		typec_enable_vconn(tcpc);
		fallthrough;
	case typec_audioaccessory:
#if CONFIG_TYPEC_CAP_DBGACC
	case typec_debugaccessory:
#endif	/* CONFIG_TYPEC_CAP_DBGACC */
		tcpc_reset_typec_debounce_timer(tcpc);
		TYPEC_DBG("Attached, Ignore cc_attach\n");
		return;

#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_trywait_snk:
		tcpci_notify_attachwait_state(tcpc, true);
		tcpc_enable_timer(tcpc, TYPEC_TIMER_CCDEBOUNCE);
		return;
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

	/* typec_drp_try_timeout = don't care */
	case typec_try_snk:
	case typec_trywait_src:
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */
#if CONFIG_TYPEC_CAP_ROLE_SWAP
	case typec_role_swap:
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */
		tcpci_notify_attachwait_state(tcpc, as_sink);
		tcpc_enable_timer(tcpc, TYPEC_TIMER_TRYCCDEBOUNCE);
		return;

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	case typec_unattachwait_pe:
		TYPEC_INFO("Force PE Idle\n");
		tcpc->pd_wait_pe_idle = false;
		tcpc_disable_timer(tcpc, TYPEC_RT_TIMER_PE_IDLE);
		typec_unattached_power_entry(tcpc);
		break;
#endif
	default:
		break;
	}

	tcpci_notify_attachwait_state(tcpc, as_sink);
	TYPEC_NEW_STATE(as_sink ? typec_attachwait_snk : typec_attachwait_src);
	tcpc_enable_timer(tcpc, TYPEC_TIMER_CCDEBOUNCE);
}

static inline int typec_attached_snk_cc_detach(struct tcpc_device *tcpc)
{
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	tcpc_enable_timer(tcpc, TYPEC_TIMER_PDDEBOUNCE);
#if CONFIG_USB_PD_DIRECT_CHARGE
	if (typec_is_cc_open()) {
		mutex_lock(&tcpc->access_lock);
		tcpc->pd_during_direct_charge = false;
		mutex_unlock(&tcpc->access_lock);
	}
#endif	/* CONFIG_USB_PD_DIRECT_CHARGE */
#else
	tcpc_reset_typec_debounce_timer(tcpc);
#endif	/* CONFIG_USB_POWER_DELIVERY */
	return 0;
}

static inline void typec_detach_wait_entry(struct tcpc_device *tcpc)
{
	typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DISABLE);

	switch (tcpc->typec_state) {
	case typec_attached_snk:
#if CONFIG_TYPEC_CAP_DBGACC_SNK
	case typec_attached_dbgacc_snk:
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */
	case typec_attached_custom_src:
		typec_attached_snk_cc_detach(tcpc);
		break;

	case typec_attached_src:
		tcpc_enable_timer(tcpc, TYPEC_TIMER_SRCDISCONNECT);
		break;

	case typec_audioaccessory:
		tcpc_enable_timer(tcpc, TYPEC_TIMER_CCDEBOUNCE);
		break;

	case typec_try_snk:
	case typec_trywait_src:
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */
		if (tcpc->typec_drp_try_timeout)
			tcpc_enable_timer(tcpc, TYPEC_TIMER_TRYCCDEBOUNCE);
		else {
			tcpc_reset_typec_debounce_timer(tcpc);
			TYPEC_DBG("[Try] Ignore cc_detach\n");
		}
		break;

	default:
		tcpc_enable_timer(tcpc, TYPEC_TIMER_PDDEBOUNCE);
		break;
	}
}

static inline bool typec_is_cc_attach(struct tcpc_device *tcpc)
{
	bool cc_attach = false;

	switch (tcpc->typec_state) {
	case typec_attached_snk:
	case typec_attached_src:
#if CONFIG_TYPEC_CAP_DBGACC_SNK
		fallthrough;
	case typec_attached_dbgacc_snk:
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */
		if (typec_get_cc_res() >= TYPEC_CC_VOLT_RD)
			cc_attach = true;
		break;

	case typec_audioaccessory:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RA))
			cc_attach = true;
		break;

#if CONFIG_TYPEC_CAP_DBGACC
	case typec_debugaccessory:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RD))
			cc_attach = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_DBGACC */

	case typec_attached_custom_src:
		if (!typec_check_cc_any(TYPEC_CC_VOLT_OPEN))
			cc_attach = true;
		break;

	case typec_try_snk:
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_trywait_snk:
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */
		if (typec_is_src_detected())
			cc_attach = true;
		break;

	case typec_trywait_src:
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */
		if (typec_is_snk_detected())
			cc_attach = true;
		break;

#if CONFIG_TYPEC_CAP_ROLE_SWAP
	case typec_role_swap:
		if (typec_is_src_detected() || typec_is_snk_detected())
			cc_attach = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	default:
		if (typec_is_cable_only())
			TYPEC_INFO("Cable Only\n");
		else if (!typec_is_cc_open())
			cc_attach = true;
		break;
	}

	return cc_attach;
}

static inline int typec_enter_low_power_mode(struct tcpc_device *tcpc)
{
	int ret = 0;

	TYPEC_INFO("%s typec_lpm = %d\n", __func__, tcpc->typec_lpm);

	if (!tcpc->typec_lpm)
		return 0;

	ret = tcpci_set_low_power_mode(tcpc, true);
	if (ret < 0)
		tcpc_enable_lpm_timer(tcpc, true);

	return ret;
}

int tcpc_typec_get_rp_present_flag(struct tcpc_device *tcpc)
{
	int rp_flag = 0;

	if (tcpc->typec_remote_cc[0] >= TYPEC_CC_VOLT_SNK_DFT
		&& tcpc->typec_remote_cc[0] != TYPEC_CC_DRP_TOGGLING)
		rp_flag |= 1;

	if (tcpc->typec_remote_cc[1] >= TYPEC_CC_VOLT_SNK_DFT
		&& tcpc->typec_remote_cc[1] != TYPEC_CC_DRP_TOGGLING)
		rp_flag |= 2;

	return rp_flag;
}

bool tcpc_typec_is_cc_open_state(struct tcpc_device *tcpc)
{
	TYPEC_DBG("%s %s\n", __func__,
		  tcpc->typec_state < ARRAY_SIZE(typec_state_names) ?
		  typec_state_names[tcpc->typec_state] : "Unknown");

	if (tcpc->typec_state == typec_disabled)
		return true;

	if (tcpc->typec_state == typec_errorrecovery)
		return true;

	if (typec_is_in_protection_states(tcpc))
		return true;

	return false;
}

static inline bool typec_is_ignore_cc_change(struct tcpc_device *tcpc)
{
	if (typec_is_drp_toggling())
		return true;

	if (tcpc_typec_is_cc_open_state(tcpc))
		return true;

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	if (tcpc->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore CC_Alert\n");
		return true;
	}
#endif /* CONFIG_USB_POWER_DELIVERY */

#if CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc->typec_state == typec_trywait_snk_pe) {
		TYPEC_DBG("[Try.PE] Ignore CC_Alert\n");
		return true;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	return false;
}

#ifdef CONFIG_MOTO_CHANNEL_SWITCH
static int mmi_mux_typec_chg_chan(enum mmi_mux_channel channel, bool on)
{
	struct mtk_charger *info = NULL;
	struct power_supply *chg_psy = NULL;

	chg_psy = power_supply_get_by_name("mtk-master-charger");
	if (chg_psy == NULL || IS_ERR(chg_psy)) {
		pr_err("%s Couldn't get chg_psy\n", __func__);
		return 0;
	} else {
		info = (struct mtk_charger *)power_supply_get_drvdata(chg_psy);
	}

	pr_info("%s open typec WLC chan =%d, on = %d\n", __func__, channel, on);
	if (info->algo.do_mux)
		info->algo.do_mux(info, channel, on);
	else
		pr_err("%s get info->algo.do_mux fail", __func__);

	return 0;
}
#endif

int tcpc_typec_handle_cc_change(struct tcpc_device *tcpc)
{
#ifdef CONFIG_MOTO_CHANNEL_SWITCH
	int cc_sum = 0;
#endif
	int ret = 0;

	ret = tcpci_get_cc(tcpc);
	if (ret < 0)
		return ret;

	TYPEC_INFO("[CC_Alert] %d/%d\n", typec_get_cc1(), typec_get_cc2());

#ifdef CONFIG_MOTO_CHANNEL_SWITCH
	cc_sum = typec_get_cc1() + typec_get_cc2();
	if ((cc_sum < CC_SUM_MAX && cc_sum > CC_SUM_MIN) && !g_flag) {
		 mmi_mux_typec_chg_chan(MMI_MUX_CHANNEL_TYPEC_CHG, ON);
		g_flag = true;
	} else if((cc_sum == CC_SUM_MAX) && g_flag) {
		g_flag = false;
	}
#endif

#if CONFIG_TYPEC_CAP_NORP_SRC
	if (typec_try_norp_src(tcpc))
		return 0;
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

	if (typec_is_ignore_cc_change(tcpc))
		return 0;

	if (typec_is_cc_attach(tcpc)) {
		typec_disable_low_power_mode(tcpc);
		typec_attach_wait_entry(tcpc);
	} else {
		typec_detach_wait_entry(tcpc);
	}

	return 0;
}

/*
 * [BLOCK] Handle timeout event
 */

static inline int typec_handle_drp_try_timeout(struct tcpc_device *tcpc)
{
	bool en_timer = false;

	tcpc->typec_drp_try_timeout = true;

	switch (tcpc->typec_state) {
	case typec_try_snk:
		en_timer = !typec_is_src_detected();
		break;

	case typec_trywait_src:
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */
		en_timer = !typec_is_snk_detected();
		break;

	default:
		break;
	}

	if (en_timer)
		tcpc_enable_timer(tcpc, TYPEC_TIMER_TRYCCDEBOUNCE);

	return 0;
}

static inline int typec_handle_debounce_timeout(struct tcpc_device *tcpc)
{
#if CONFIG_TYPEC_CAP_NORP_SRC
	if (tcpc->typec_state == typec_unattached_snk &&
	    tcpci_check_vbus_valid(tcpc) && typec_is_cc_no_res()) {
		typec_norp_src_attached_entry(tcpc);
		return typec_alert_attach_state_change(tcpc);
	}
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Warning] DRP Toggling\n");
		return 0;
	}

	typec_handle_cc_changed_entry(tcpc);
	return 0;
}

static inline int typec_handle_error_recovery_timeout(struct tcpc_device *tcpc)
{
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	tcpc->pd_wait_pe_idle = false;
#endif	/* CONFIG_USB_POWER_DELIVERY */

	typec_unattach_wait_pe_idle_entry(tcpc);
	typec_alert_attach_state_change(tcpc);
	return 0;
}

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
static inline int typec_handle_pe_idle(struct tcpc_device *tcpc)
{
	switch (tcpc->typec_state) {
#if CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_trywait_snk_pe:
		typec_trywait_snk_entry(tcpc);
		break;
#endif

	case typec_unattachwait_pe:
		typec_unattached_entry(tcpc);
		break;

	default:
		TYPEC_DBG("Dummy pe_idle\n");
		break;
	}

	return 0;
}

#if CONFIG_USB_PD_WAIT_BC12
static inline void typec_handle_pd_wait_bc12(struct tcpc_device *tcpc)
{
	int ret = 0;
	uint8_t type = TYPEC_UNATTACHED;
	union power_supply_propval val = {.intval = 0};

	mutex_lock(&tcpc->access_lock);

	type = tcpc->typec_attach_new;
	ret = power_supply_get_property(tcpc->chg_psy,
		POWER_SUPPLY_PROP_USB_TYPE, &val);
	TYPEC_INFO("type=%d, ret,chg_type=%d,%d, count=%d\n", type,
		ret, val.intval, tcpc->pd_wait_bc12_count);

	if (type != TYPEC_ATTACHED_SNK && type != TYPEC_ATTACHED_DBGACC_SNK)
		goto out;

	if ((ret >= 0 && val.intval != POWER_SUPPLY_USB_TYPE_UNKNOWN) ||
		tcpc->pd_wait_bc12_count >= 20) {
		__pd_put_cc_attached_event(tcpc, type);
	} else {
		tcpc->pd_wait_bc12_count++;
		tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_PD_WAIT_BC12);
	}
out:
	mutex_unlock(&tcpc->access_lock);
}
#endif /* CONFIG_USB_PD_WAIT_BC12 */
#endif /* CONFIG_USB_POWER_DELIVERY */

static inline int typec_handle_src_reach_vsafe0v(struct tcpc_device *tcpc)
{
	if (tcpc->typec_state == typec_try_src && !typec_is_snk_detected()) {
		typec_trywait_snk_entry(tcpc);
		return 0;
	}
	typec_cc_src_detect_vsafe0v_entry(tcpc);
	typec_alert_attach_state_change(tcpc);
	return 0;
}

int tcpc_typec_handle_timeout(struct tcpc_device *tcpc, uint32_t timer_id)
{
	int ret = 0;

	if (timer_id >= TYPEC_TIMER_START_ID &&
	    tcpc_is_timer_active(tcpc, TYPEC_TIMER_START_ID, PD_TIMER_NR)) {
		TYPEC_DBG("[Type-C] Ignore timer_evt\n");
		return 0;
	}

	if (timer_id == TYPEC_TIMER_ERROR_RECOVERY)
		return typec_handle_error_recovery_timeout(tcpc);
	else if (timer_id == TYPEC_RT_TIMER_STATE_CHANGE)
		return typec_alert_attach_state_change(tcpc);
	else if (tcpc_typec_is_cc_open_state(tcpc)) {
		TYPEC_DBG("[Open] Ignore timer_evt\n");
		return 0;
	}

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	if (tcpc->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore timer_evt\n");
		return 0;
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

	switch (timer_id) {
	case TYPEC_TIMER_CCDEBOUNCE:
	case TYPEC_TIMER_PDDEBOUNCE:
	case TYPEC_TIMER_TRYCCDEBOUNCE:
	case TYPEC_TIMER_SRCDISCONNECT:
#if CONFIG_TYPEC_CAP_NORP_SRC
	case TYPEC_TIMER_NORP_SRC:
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */
		ret = typec_handle_debounce_timeout(tcpc);
		break;

	case TYPEC_TIMER_DRP_SRC_TOGGLE:
		if (tcpc->typec_state == typec_unattached_src)
			typec_unattached_snk_and_drp_entry(tcpc);
		break;

	case TYPEC_TRY_TIMER_DRP_TRY:
		ret = typec_handle_drp_try_timeout(tcpc);
		break;

	case TYPEC_TRY_TIMER_TRY_TOUT:
		if (tcpc->typec_state == typec_try_src &&
		    !typec_is_snk_detected())
			typec_trywait_snk_entry(tcpc);
		break;

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	case TYPEC_RT_TIMER_PE_IDLE:
		ret = typec_handle_pe_idle(tcpc);
		break;
#if CONFIG_USB_PD_WAIT_BC12
	case TYPEC_RT_TIMER_PD_WAIT_BC12:
		typec_handle_pd_wait_bc12(tcpc);
		break;
#endif /* CONFIG_USB_PD_WAIT_BC12 */
#endif /* CONFIG_USB_POWER_DELIVERY */

#if CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY
	case TYPEC_RT_TIMER_SAFE0V_DELAY:
		ret = typec_handle_src_reach_vsafe0v(tcpc);
		break;
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY */

	case TYPEC_RT_TIMER_LOW_POWER_MODE:
		typec_enter_low_power_mode(tcpc);
		break;

#if CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
	case TYPEC_RT_TIMER_SAFE0V_TOUT:
		TCPC_INFO("VSafe0V TOUT (%d)\n", tcpc->vbus_level);

		if (!tcpci_check_vbus_valid_from_ic(tcpc))
			ret = tcpc_typec_handle_vsafe0v(tcpc);
		break;
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */

#if CONFIG_TYPEC_CAP_ROLE_SWAP
	case TYPEC_RT_TIMER_ROLE_SWAP_STOP:
		typec_handle_role_swap_stop(tcpc);
		break;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	case TYPEC_RT_TIMER_DISCHARGE:
		if (!tcpc->typec_power_ctrl) {
			mutex_lock(&tcpc->access_lock);
			tcpci_enable_discharge(tcpc, false, 0);
			mutex_unlock(&tcpc->access_lock);
		}
		break;

#if CONFIG_WATER_DETECTION
	case TYPEC_RT_TIMER_WD_IN_KPOC:
		if (tcpc->wd_in_kpoc) {
			tcpci_report_usb_port_detached(tcpc);
			ret = tcpci_set_water_protection(tcpc, true);
		}
		break;
#endif /* CONFIG_WATER_DETECTION */
	}

	return ret;
}

/*
 * [BLOCK] Handle ps-change event
 */

static inline int typec_handle_vbus_present(struct tcpc_device *tcpc)
{
	switch (tcpc->typec_wait_ps_change) {
	case TYPEC_WAIT_PS_SNK_VSAFE5V:
		typec_cc_snk_detect_vsafe5v_entry(tcpc);
		typec_alert_attach_state_change(tcpc);
		break;
	case TYPEC_WAIT_PS_SRC_VSAFE5V:
		tcpc->typec_attach_new = TYPEC_ATTACHED_SRC;
		typec_alert_attach_state_change(tcpc);
		break;
#if CONFIG_TYPEC_CAP_DBGACC
	case TYPEC_WAIT_PS_DBG_VSAFE5V:
		tcpc->typec_attach_new = TYPEC_ATTACHED_DEBUG;
		typec_alert_attach_state_change(tcpc);
		break;
#endif	/* CONFIG_TYPEC_CAP_DBGACC */
	}
	if (tcpc->typec_wait_ps_change >= TYPEC_WAIT_PS_SNK_VSAFE5V)
		typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DISABLE);

	return 0;
}

static inline int typec_attached_snk_vbus_absent(struct tcpc_device *tcpc)
{
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
#if CONFIG_USB_PD_DIRECT_CHARGE
	if (tcpc->pd_during_direct_charge && !tcpci_check_vsafe0v(tcpc)) {
		TYPEC_DBG("Ignore vbus_absent(snk), DirectCharge\n");
		return 0;
	}
#endif	/* CONFIG_USB_PD_DIRECT_CHARGE */

	if (tcpc->pd_wait_hard_reset_complete &&
	    typec_get_cc_res() != TYPEC_CC_VOLT_OPEN) {
		TYPEC_DBG("Ignore vbus_absent(snk), HReset & CC!=0\n");
		return 0;
	}

	if (tcpc->pd_exit_attached_snk_via_cc &&
	    tcpc->pd_port.pe_data.pd_prev_connected && !typec_is_cc_no_res()) {
		TYPEC_DBG("Ignore vbus_absent(snk), PD & CC!=0\n");
		return 0;
	}
#endif /* CONFIG_USB_POWER_DELIVERY */
	typec_unattach_wait_pe_idle_entry(tcpc);
	typec_alert_attach_state_change(tcpc);

	return 0;
}


static inline int typec_handle_vbus_absent(struct tcpc_device *tcpc)
{
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
	if (tcpc->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore vbus_absent\n");
		return 0;
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

	switch (tcpc->typec_state) {
	case typec_attached_snk:
#if CONFIG_TYPEC_CAP_DBGACC_SNK
	case typec_attached_dbgacc_snk:
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */
	case typec_attached_custom_src:
		typec_attached_snk_vbus_absent(tcpc);
		break;
	default:
		break;
	}

	return 0;
}

int tcpc_typec_handle_ps_change(struct tcpc_device *tcpc, int vbus_level)
{
	tcpci_notify_ps_change(tcpc, vbus_level);

#if CONFIG_TYPEC_CAP_NORP_SRC
	if (typec_try_norp_src(tcpc))
		return 0;
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

#if CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS
	if (tcpc->typec_state == typec_audioaccessory) {
		return typec_audio_acc_sink_vbus(
			tcpc, vbus_level >= TCPC_VBUS_VALID);
	}
#endif	/* CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS */

	if (vbus_level >= TCPC_VBUS_VALID)
		return typec_handle_vbus_present(tcpc);

	return typec_handle_vbus_absent(tcpc);
}

/*
 * [BLOCK] Handle PE event
 */

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)

int tcpc_typec_handle_pe_pr_swap(struct tcpc_device *tcpc)
{
	int ret = 0;

	tcpci_lock_typec(tcpc);
	switch (tcpc->typec_state) {
	case typec_attached_snk:
		TYPEC_NEW_STATE(typec_attached_src);
		tcpc->typec_attach_new = TYPEC_ATTACHED_SRC;
		tcpci_set_auto_dischg_discnt(tcpc, false);
		tcpci_set_cc(tcpc,
			TYPEC_CC_PULL(tcpc->typec_local_rp_level, TYPEC_CC_RP));
		break;
	case typec_attached_src:
		TYPEC_NEW_STATE(typec_attached_snk);
		tcpc->typec_attach_new = TYPEC_ATTACHED_SNK;
		tcpci_set_auto_dischg_discnt(tcpc, true);
		tcpci_set_cc(tcpc, TYPEC_CC_RD);
		break;
	default:
		break;
	}

	typec_alert_attach_state_change(tcpc);
	tcpci_unlock_typec(tcpc);
	return ret;
}

#endif /* CONFIG_USB_POWER_DELIVERY */

/*
 * [BLOCK] Handle reach vSafe0V event
 */

int tcpc_typec_handle_vsafe0v(struct tcpc_device *tcpc)
{
	if (tcpc->typec_wait_ps_change != TYPEC_WAIT_PS_SRC_VSAFE0V)
		return 0;

	typec_wait_ps_change(tcpc, TYPEC_WAIT_PS_DISABLE);

	if (tcpc_typec_is_cc_open_state(tcpc))
		return 0;

#if CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY
	tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_SAFE0V_DELAY);
#else
	typec_handle_src_reach_vsafe0v(tcpc);
#endif

	return 0;
}

/*
 * [BLOCK] TCPCI TypeC I/F
 */

const char *const typec_role_name[] = {
	"UNKNOWN",
	"SNK",
	"SRC",
	"DRP",
	"TrySRC",
	"TrySNK",
};

#if CONFIG_TYPEC_CAP_ROLE_SWAP
int tcpc_typec_swap_role(struct tcpc_device *tcpc)
{
	if (tcpc->typec_role < TYPEC_ROLE_DRP)
		return TCPM_ERROR_NOT_DRP_ROLE;

	if (tcpc->typec_during_role_swap)
		return TCPM_ERROR_DURING_ROLE_SWAP;

	switch (tcpc->typec_attach_old) {
	case TYPEC_ATTACHED_SNK:
		tcpc->typec_during_role_swap = TYPEC_ROLE_SWAP_TO_SRC;
		break;
	case TYPEC_ATTACHED_SRC:
		tcpc->typec_during_role_swap = TYPEC_ROLE_SWAP_TO_SNK;
		break;
	}

	if (tcpc->typec_during_role_swap) {
		TYPEC_INFO("TypeC Role Swap Start\n");
		tcpci_set_cc(tcpc, TYPEC_CC_OPEN);
		tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
		return TCPM_SUCCESS;
	}

	return TCPM_ERROR_UNATTACHED;
}
#endif /* CONFIG_TYPEC_CAP_ROLE_SWAP */

int tcpc_typec_set_rp_level(struct tcpc_device *tcpc, uint8_t rp_lvl)
{
	switch (rp_lvl) {
	case TYPEC_RP_DFT:
	case TYPEC_RP_1_5:
	case TYPEC_RP_3_0:
		TYPEC_INFO("TypeC-Rp: %d\n", rp_lvl);
		tcpc->typec_local_rp_level = rp_lvl;
		break;
	default:
		TYPEC_INFO("TypeC-Unknown-Rp (%d)\n", rp_lvl);
		return -EINVAL;
	}

	return 0;
}

int tcpc_typec_error_recovery(struct tcpc_device *tcpc)
{
	if (tcpc->typec_state != typec_errorrecovery)
		typec_error_recovery_entry(tcpc);

	return 0;
}
EXPORT_SYMBOL(tcpc_typec_error_recovery);

int tcpc_typec_disable(struct tcpc_device *tcpc)
{
	if (tcpc->typec_state != typec_disabled)
		typec_disable_entry(tcpc);

	return 0;
}

int tcpc_typec_enable(struct tcpc_device *tcpc)
{
	if (tcpc->typec_state == typec_disabled)
		typec_unattached_entry(tcpc);

	return 0;
}

int tcpc_typec_change_role(
	struct tcpc_device *tcpc, uint8_t typec_role, bool postpone)
{
	if (typec_role == TYPEC_ROLE_UNKNOWN ||
		typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\n", typec_role);
		return -EINVAL;
	}

	if (tcpc->typec_role_new == typec_role) {
		TYPEC_INFO("typec_new_role: %s is the same\n",
			typec_role_name[typec_role]);
		return 0;
	}
	tcpc->typec_role_new = typec_role;

	TYPEC_INFO("typec_new_role: %s\n", typec_role_name[typec_role]);

	if (tcpc_typec_is_cc_open_state(tcpc))
		return 0;

	if (!postpone || tcpc->typec_attach_old == TYPEC_UNATTACHED ||
			 tcpc->typec_attach_old == TYPEC_ATTACHED_NORP_SRC)
		return tcpc_typec_error_recovery(tcpc);
	else
		return 0;
}

#if CONFIG_TYPEC_SNK_ONLY_WHEN_SUSPEND
int tcpc_typec_suspend(struct tcpc_device *tcpc)
{
	int ret = 0;
	uint32_t alert_mask = 0;

	if (tcpc->typec_role < TYPEC_ROLE_DRP)
		return ret;

	if (tcpc->typec_state != typec_unattached_snk &&
	    tcpc->typec_state != typec_attached_norp_src)
		return ret;

	ret = tcpci_get_alert_mask(tcpc, &alert_mask);
	if (ret < 0)
		return ret;
	ret = tcpci_set_alert_mask(tcpc, alert_mask &
				   ~TCPC_V10_REG_ALERT_CC_STATUS);
	if (ret < 0)
		return ret;
	tcpci_set_cc(tcpc, TYPEC_CC_RD);
	usleep_range(500, 1000);
	tcpci_alert_status_clear(tcpc, TCPC_V10_REG_ALERT_CC_STATUS);
	tcpci_set_alert_mask(tcpc, alert_mask);
	tcpci_get_cc(tcpc);
	if (!typec_is_cc_open()) {
		tcpci_set_cc(tcpc, TYPEC_CC_DRP);
		return -EBUSY;
	}
	return 0;
}

void tcpc_typec_resume(struct tcpc_device *tcpc)
{
	if (tcpc->typec_role < TYPEC_ROLE_DRP)
		return;

	if (tcpc->typec_state != typec_unattached_snk &&
	    tcpc->typec_state != typec_attached_norp_src)
		return;

	tcpci_set_cc(tcpc, TYPEC_CC_DRP);
}
#endif	/* CONFIG_TYPEC_SNK_ONLY_WHEN_SUSPEND */

int tcpc_typec_init(struct tcpc_device *tcpc, uint8_t typec_role)
{
	int ret = 0;
	char *reason = NULL;

	if (typec_role == TYPEC_ROLE_UNKNOWN ||
		typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\n", typec_role);
		return -EINVAL;
	}

	TYPEC_INFO("typec_init: %s\n", typec_role_name[typec_role]);

	if ((tcpc->bootmode == 8 || tcpc->bootmode == 9) &&
	    typec_role != TYPEC_ROLE_SRC) {
		reason = "KPOC";
		typec_role = TYPEC_ROLE_SNK;
	} else if (tcpc->tcpc_flags & TCPC_FLAGS_FLOATING_GROUND) {
		reason = "WD0";
		typec_role = TYPEC_ROLE_SNK;
	}
	if (reason) {
		TYPEC_INFO("%s, typec_init: %s\n",
			   reason, typec_role_name[typec_role]);
	}

	tcpc->typec_role = typec_role;
	tcpc->typec_role_new = typec_role;
	typec_attach_new_unattached(tcpc);
	tcpc->typec_attach_old = TYPEC_UNATTACHED;

	ret = tcpci_get_cc(tcpc);

#if CONFIG_TYPEC_CAP_NORP_SRC
	if (!tcpci_check_vbus_valid(tcpc) || ret < 0)
		tcpc->typec_power_ctrl = true;
#else
	if (!tcpci_check_vbus_valid(tcpc) || ret < 0 || typec_is_cc_no_res())
		tcpc->typec_power_ctrl = true;
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

	if (!typec_is_cc_no_res()) {
		tcpci_set_cc(tcpc, TYPEC_CC_OPEN);
		usleep_range(20000, 30000);
	}
	typec_unattached_entry(tcpc);
#if CONFIG_TYPEC_CAP_NORP_SRC
	if (typec_try_norp_src(tcpc))
		return 0;
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */
	return ret;
}

void tcpc_typec_deinit(struct tcpc_device *tcpc)
{
}

#if CONFIG_WATER_DETECTION
int tcpc_typec_handle_wd(struct tcpc_device **tcpcs, size_t nr, bool wd)
{
	int ret = 0, i = 0;
	struct tcpc_device *tcpc = NULL;
	bool cc_open = false;
	uint8_t typec_state = typec_disabled;
	bool modal_operation = false;
	bool hreset = false;

	if (nr < 1)
		return ret;
#if CONFIG_TCPC_LOG_WITH_PORT_NAME
	tcpc = tcpcs[0];
#endif /* CONFIG_TCPC_LOG_WITH_PORT_NAME */
	TYPEC_INFO("%s %d, nr = %lu\n", __func__, wd, nr);

	for (i = 0; i < nr; i++) {
		tcpc = tcpcs[i];
		if (i > 0)
			tcpci_lock_typec(tcpc);
		cc_open = tcpc_typec_is_cc_open_state(tcpc);
		typec_state = tcpc->typec_state;
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
		modal_operation = tcpc->pd_port.pe_data.modal_operation;
		hreset = tcpc->pd_wait_hard_reset_complete;
#endif	/* CONFIG_USB_POWER_DELIVERY */
		if (i > 0)
			tcpci_unlock_typec(tcpc);
		if (wd && (cc_open ||
			   typec_state == typec_audioaccessory ||
			   modal_operation))
			return -EPERM;
#if !CONFIG_WD_DURING_PLUGGED_IN
		if (typec_state != typec_unattached_snk &&
		    typec_state != typec_unattached_src &&
		    typec_state != typec_attachwait_snk &&
		    typec_state != typec_attachwait_src &&
		    typec_state != typec_water_protection)
			return -EPERM;
#endif	/* CONFIG_WD_DURING_PLUGGED_IN */
		if (wd && hreset)
			return -EAGAIN;
	}

	i = 0;
repeat:
	tcpc = tcpcs[i];
	if (i > 0)
		tcpci_lock_typec(tcpc);
	if (tcpc->bootmode == 8 || tcpc->bootmode == 9) {
		tcpc->wd_in_kpoc = wd;
		if (wd) {
			tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_WD_IN_KPOC);
		} else {
			tcpc_disable_timer(tcpc, TYPEC_RT_TIMER_WD_IN_KPOC);
			tcpci_set_water_protection(tcpc, false);
			ret = tcpci_report_usb_port_attached(tcpc);
		}
		goto out;
	}
	if (!wd) {
		tcpci_set_water_protection(tcpc, false);
		tcpc_typec_error_recovery(tcpc);
		goto out;
	}

	typec_cc_open_entry(tcpc, typec_water_protection);
	tcpci_set_cc_hidet(tcpc, true);
	ret = tcpci_set_water_protection(tcpc, true);
out:
	tcpci_notify_wd_status(tcpc, wd);
	if (i > 0)
		tcpci_unlock_typec(tcpc);
	if (++i < nr)
		goto repeat;
	return ret;
}
EXPORT_SYMBOL(tcpc_typec_handle_wd);
#endif /* CONFIG_WATER_DETECTION */

int tcpc_typec_handle_fod(struct tcpc_device *tcpc, enum tcpc_fod_status fod)
{
	int ret = 0;
	enum tcpc_fod_status fod_old = tcpc->typec_fod;
	uint8_t typec_state = tcpc->typec_state;

	if (!(tcpc->tcpc_flags & TCPC_FLAGS_FOREIGN_OBJECT_DETECTION))
		return 0;

	TCPC_INFO("%s fod (%d, %d)\n", __func__, fod_old, fod);

	if (fod_old == fod)
		return 0;
	if (typec_state != typec_unattached_snk &&
	    typec_state != typec_unattached_src &&
	    typec_state != typec_attachwait_snk &&
	    typec_state != typec_attachwait_src &&
	    typec_state != typec_foreign_object_protection)
		return -EPERM;
	tcpc->typec_fod = fod;

	if (tcpc->bootmode == 8 || tcpc->bootmode == 9) {
		TYPEC_INFO("Not to do foreign object protection in KPOC\n");
		goto out;
	}

	if (fod_old == TCPC_FOD_LR) {
		tcpc_typec_error_recovery(tcpc);
		goto out;
	}
	if (fod != TCPC_FOD_LR)
		goto out;

	typec_cc_open_entry(tcpc, typec_foreign_object_protection);
	ret = tcpci_set_cc_hidet(tcpc, true);
out:
	tcpci_notify_fod_status(tcpc);
	return ret;
}
EXPORT_SYMBOL(tcpc_typec_handle_fod);

#if CONFIG_CABLE_TYPE_DETECTION
int tcpc_typec_handle_ctd(struct tcpc_device *tcpc,
			  enum tcpc_cable_type cable_type)
{
	if (!(tcpc->tcpc_flags & TCPC_FLAGS_CABLE_TYPE_DETECTION))
		return 0;

	TCPC_DBG("%s: cable_type = %d\n", __func__, cable_type);

	if (tcpc->tcpc_flags & TCPC_FLAGS_FOREIGN_OBJECT_DETECTION) {
		if ((cable_type == TCPC_CABLE_TYPE_C2C) &&
		    (tcpc->typec_fod == TCPC_FOD_DISCHG_FAIL ||
		     tcpc->typec_fod == TCPC_FOD_OV))
			cable_type = TCPC_CABLE_TYPE_A2C;
	}

	TCPC_INFO("%s cable (%d, %d)\n", __func__, tcpc->typec_cable_type,
		  cable_type);

	if (tcpc->typec_cable_type == cable_type)
		return 0;

	if (tcpc->typec_cable_type != TCPC_CABLE_TYPE_NONE &&
	    cable_type != TCPC_CABLE_TYPE_NONE) {
		TCPC_INFO("%s ctd done once %d\n", __func__,
			  tcpc->typec_cable_type);
		return 0;
	}

	tcpc->typec_cable_type = cable_type;
	tcpci_notify_cable_type(tcpc);
	return 0;
}
EXPORT_SYMBOL(tcpc_typec_handle_ctd);
#endif /* CONFIG_CABLE_TYPE_DETECTION */

int tcpc_typec_handle_otp(struct tcpc_device *tcpc, bool otp)
{
	int ret = 0;

	if (!(tcpc->tcpc_flags & TCPC_FLAGS_TYPEC_OTP))
		return 0;

	TCPC_INFO("%s otp (%d, %d)\n", __func__, tcpc->typec_otp, otp);

	if (tcpc->typec_otp == otp)
		return 0;
	tcpc->typec_otp = otp;
	if (!otp) {
		tcpc_typec_error_recovery(tcpc);
		goto out;
	}

	typec_cc_open_entry(tcpc, typec_otp);
	ret = tcpci_set_cc_hidet(tcpc, true);
out:
	tcpci_notify_typec_otp(tcpc);
	return ret;
}
EXPORT_SYMBOL(tcpc_typec_handle_otp);
