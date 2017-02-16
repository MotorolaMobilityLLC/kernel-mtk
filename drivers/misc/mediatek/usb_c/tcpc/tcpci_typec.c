/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * TCPC Type-C Driver for Richtek
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
#include <linux/cpu.h>

#include "inc/tcpci.h"
#include "inc/tcpci_typec.h"
#include "inc/tcpci_timer.h"

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
#define CONFIG_TYPEC_CAP_TRY_STATE
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
#undef CONFIG_TYPEC_CAP_TRY_STATE
#define CONFIG_TYPEC_CAP_TRY_STATE
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

enum TYPEC_WAIT_PS_STATE {
	TYPEC_WAIT_PS_DISABLE = 0,
	TYPEC_WAIT_PS_SNK_VSAFE5V,
	TYPEC_WAIT_PS_SRC_VSAFE0V,
	TYPEC_WAIT_PS_SRC_VSAFE5V,
};

enum TYPEC_ROLE_SWAP_STATE {
	TYPEC_ROLE_SWAP_NONE = 0,
	TYPEC_ROLE_SWAP_TO_SNK,
	TYPEC_ROLE_SWAP_TO_SRC,
};

#if TYPEC_DBG_ENABLE
static const char *const typec_wait_ps_name[] = {
	"Disable",
	"SNK_VSafe5V",
	"SRC_VSafe0V",
	"SRC_VSafe5V",
};
#endif	/* TYPEC_DBG_ENABLE */

static inline void typec_wait_ps_change(struct tcpc_device *tcpc_dev,
					enum TYPEC_WAIT_PS_STATE state)
{
#if TYPEC_DBG_ENABLE
	uint8_t old_state = tcpc_dev->typec_wait_ps_change;
	uint8_t new_state = (uint8_t) state;

	if (new_state != old_state)
		TYPEC_DBG("wait_ps=%s\r\n", typec_wait_ps_name[new_state]);
#endif	/* TYPEC_DBG_ENABLE */

#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
	if (state == TYPEC_WAIT_PS_SRC_VSAFE0V)
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_TOUT);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */

	if (tcpc_dev->typec_wait_ps_change == TYPEC_WAIT_PS_SRC_VSAFE0V
		&& state != TYPEC_WAIT_PS_SRC_VSAFE0V) {
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_DELAY);

#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_TOUT);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */
	}

	tcpc_dev->typec_wait_ps_change = (uint8_t) state;
}

/* #define TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE */
#define TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS

static inline int typec_enable_low_power_mode(
	struct tcpc_device *tcpc_dev, int pull);

#define typec_get_cc1()		\
	tcpc_dev->typec_remote_cc[0]
#define typec_get_cc2()		\
	tcpc_dev->typec_remote_cc[1]
#define typec_get_cc_res()	\
	(tcpc_dev->typec_polarity ? typec_get_cc2() : typec_get_cc1())

#define typec_check_cc1(cc)	\
	(typec_get_cc1() == cc)

#define typec_check_cc2(cc)	\
	(typec_get_cc2() == cc)

#define typec_check_cc(cc1, cc2)	\
	(typec_check_cc1(cc1) && typec_check_cc2(cc2))

#define typec_check_cc_both(res)	\
	(typec_check_cc(res, res))

#define typec_check_cc_any(res)		\
	(typec_check_cc1(res) || typec_check_cc2(res))

#define typec_is_drp_toggling() \
	(typec_get_cc1() == TYPEC_CC_DRP_TOGGLING)

#define typec_is_cc_open()	\
	typec_check_cc_both(TYPEC_CC_VOLT_OPEN)


/* TYPEC_GET_CC_STATUS */

/*
 * [BLOCK] TYPEC Connection State Definition
 */

enum TYPEC_CONNECTION_STATE {
	typec_disabled = 0,
	typec_errorrecovery,

	typec_unattached_snk,
	typec_unattached_src,

	typec_attachwait_snk,
	typec_attachwait_src,

	typec_attached_snk,
	typec_attached_src,

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	/* Require : Assert Rp
	 * Exit(-> Attached.SRC) : Detect Rd (tPDDebounce).
	 * Exit(-> TryWait.SNK) : Not detect Rd after tDRPTry
	 */
	typec_try_src,

	/* Require : Assert Rd
	 * Exit(-> Attached.SNK) : Detect Rp (tCCDebounce) and Vbus present.
	 * Exit(-> Unattached.SNK) : Not detect Rp (tPDDebounce)
	 */

	typec_trywait_snk,
	typec_trywait_snk_pe,
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK

	/* Require : Assert Rd
	 * Wait for tDRPTry and only then begin monitoring CC.
	 * Exit (-> Attached.SNK) : Detect Rp (tPDDebounce) and Vbus present.
	 * Exit (-> TryWait.SRC) : Not detect Rp for tPDDebounce.
	 */
	typec_try_snk,

	/*
	 * Require : Assert Rp
	 * Exit (-> Attached.SRC) : Detect Rd (tCCDebounce)
	 * Exit (-> Unattached.SNK) : Not detect Rd after tDRPTry
	 */

	typec_trywait_src,
	typec_trywait_src_pe,
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_audioaccessory,
	typec_debugaccessory,

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	typec_attached_dbgacc_snk,
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	typec_attached_custom_src,
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	typec_role_swap,
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	typec_unattachwait_pe,	/* Wait Policy Engine go to Idle */
};

static const char *const typec_state_name[] = {
	"Disabled",
	"ErrorRecovery",

	"Unattached.SNK",
	"Unattached.SRC",

	"AttachWait.SNK",
	"AttachWait.SRC",

	"Attached.SNK",
	"Attached.SRC",

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	"Try.SRC",
	"TryWait.SNK",
	"TryWait.SNK.PE",
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	"Try.SNK",
	"TryWait.SRC",
	"TryWait.SRC.PE",
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	"AudioAccessory",
	"DebugAccessory",

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	"DBGACC.SNK",
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	"Custom.SRC",
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	"RoleSwap",
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	"UnattachWait.PE",
};

static inline void typec_transfer_state(struct tcpc_device *tcpc_dev,
					enum TYPEC_CONNECTION_STATE state)
{
	TYPEC_INFO("** %s\r\n", typec_state_name[state]);
	tcpc_dev->typec_state = (uint8_t) state;
}

#define TYPEC_NEW_STATE(state)  \
	(typec_transfer_state(tcpc_dev, state))

/*
 * [BLOCK] TypeC Alert Attach Status Changed
 */

static const char *const typec_attach_name[] = {
	"NULL",
	"SINK",
	"SOURCE",
	"AUDIO",
	"DEBUG",

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	"DBGACC_SNK",
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	"CUSTOM_SRC",
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */
};

static int typec_alert_attach_state_change(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable)
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_NOT_LEGACY);
	else
		tcpc_restart_timer(tcpc_dev, TYPEC_RT_TIMER_NOT_LEGACY);
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (tcpc_dev->typec_attach_old == tcpc_dev->typec_attach_new) {
		TYPEC_DBG("Attached-> %s(repeat)\r\n",
			typec_attach_name[tcpc_dev->typec_attach_new]);
		return 0;
	}

	TYPEC_INFO("Attached-> %s\r\n",
		   typec_attach_name[tcpc_dev->typec_attach_new]);

	/*Report function */
	ret = tcpci_report_usb_port_changed(tcpc_dev);

	tcpc_dev->typec_attach_old = tcpc_dev->typec_attach_new;
	return ret;
}

static inline int typec_set_drp_toggling(struct tcpc_device *tcpc_dev)
{
	int ret;

	ret = tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
	if (ret < 0)
		return ret;

	return typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
}

/*
 * [BLOCK] Unattached Entry
 */

static inline int typec_try_low_power_mode(struct tcpc_device *tcpc_dev)
{
	int ret = tcpci_set_low_power_mode(
		tcpc_dev, true, tcpc_dev->typec_lpm_pull);
	if (ret < 0)
		return ret;

#ifdef CONFIG_TCPC_LPM_CONFIRM
	ret = tcpci_is_low_power_mode(tcpc_dev);
	if (ret < 0)
		return ret;

	if (ret == 1)
		return 0;

	if (tcpc_dev->typec_lpm_retry == 0) {
		TYPEC_INFO("TryLPM Failed\r\n");
		return 0;
	}

	tcpc_dev->typec_lpm_retry--;
	TYPEC_DBG("RetryLPM : %d\r\n", tcpc_dev->typec_lpm_retry);
	tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LOW_POWER_MODE);
#endif	/* CONFIG_TCPC_LPM_CONFIRM */

	return 0;
}

static inline int typec_enter_low_power_mode(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

#ifdef CONFIG_TCPC_LPM_POSTPONE
	tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LOW_POWER_MODE);
#else
	ret = typec_try_low_power_mode(tcpc_dev);
#endif	/* CONFIG_TCPC_POSTPONE_LPM */

	return ret;
}

static inline int typec_enable_low_power_mode(
	struct tcpc_device *tcpc_dev, int pull)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable) {
		TYPEC_DBG("LPM_LCOnly\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (tcpc_dev->typec_cable_only) {
		TYPEC_DBG("LPM_RaOnly\r\n");

#ifdef CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG
		if (tcpc_dev->tcpc_flags & TCPC_FLAGS_LPM_WAKEUP_WATCHDOG)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_WAKEUP);
#endif	/* CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG */

		return 0;
	}

	if (tcpc_dev->typec_lpm != true) {
		tcpc_dev->typec_lpm = true;
		tcpc_dev->typec_lpm_retry = TCPC_LOW_POWER_MODE_RETRY;
		tcpc_dev->typec_lpm_pull = (uint8_t) pull;
		ret = typec_enter_low_power_mode(tcpc_dev);
	}

	return ret;
}

static inline int typec_disable_low_power_mode(
	struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	if (tcpc_dev->typec_lpm != false) {
		tcpc_dev->typec_lpm = false;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_LOW_POWER_MODE);
		ret = tcpci_set_low_power_mode(tcpc_dev, false, TYPEC_CC_DRP);
	}

	return ret;
}

static void typec_unattached_power_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (tcpc_dev->typec_power_ctrl) {
		tcpci_set_vconn(tcpc_dev, false);
		tcpci_disable_vbus_control(tcpc_dev);
		tcpci_report_power_control(tcpc_dev, false);
	}
}

static inline void typec_unattached_cc_entry(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap) {
		TYPEC_NEW_STATE(typec_role_swap);
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	switch (tcpc_dev->typec_role) {
	case TYPEC_ROLE_SNK:
		TYPEC_NEW_STATE(typec_unattached_snk);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_RD);
		break;
	case TYPEC_ROLE_SRC:
#ifdef CONFIG_TYPEC_CHECK_SRC_UNATTACH_OPEN
		if (typec_check_cc_any(TYPEC_CC_VOLT_RD)) {
			TYPEC_DBG("typec_src_unattach not open\r\n");
			tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
			mdelay(5);
		}
#endif	/* CONFIG_TYPEC_CHECK_SRC_UNATTACH_OPEN */
		TYPEC_NEW_STATE(typec_unattached_src);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_RP);
		break;
	default:
		switch (tcpc_dev->typec_state) {
		case typec_attachwait_snk:
		case typec_audioaccessory:
			TYPEC_NEW_STATE(typec_unattached_src);
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_DRP_SRC_TOGGLE);
			break;
		default:
			TYPEC_NEW_STATE(typec_unattached_snk);
			tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
			typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
			break;
		}
		break;
	}
}

static void typec_unattached_entry(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_CUSTOM_HV
	tcpc_dev->typec_during_custom_hv = false;
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_HV */

	tcpc_dev->typec_usb_sink_curr = CONFIG_TYPEC_SNK_CURR_DFT;

	typec_unattached_cc_entry(tcpc_dev);
	typec_unattached_power_entry(tcpc_dev);
}

static void typec_unattach_wait_pe_idle_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->typec_attach_old) {
		TYPEC_NEW_STATE(typec_unattachwait_pe);
		return;
	}
#endif

	typec_unattached_entry(tcpc_dev);
}

/*
 * [BLOCK] Attached Entry
 */

static inline int typec_set_polarity(struct tcpc_device *tcpc_dev,
					bool polarity)
{
	tcpc_dev->typec_polarity = polarity;
	return tcpci_set_polarity(tcpc_dev, polarity);
}

static inline int typec_set_plug_orient(struct tcpc_device *tcpc_dev,
				uint8_t res, bool polarity)
{
	int rv = typec_set_polarity(tcpc_dev, polarity);

	if (rv)
		return rv;

	return tcpci_set_cc(tcpc_dev, res);
}

static void typec_source_attached_with_vbus_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SRC;
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);
}

static inline void typec_source_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_attached_src);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SRC_VSAFE5V);

	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap) {
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	typec_set_plug_orient(tcpc_dev,
		tcpc_dev->typec_local_rp_level,
		typec_check_cc2(TYPEC_CC_VOLT_RD));

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_set_vconn(tcpc_dev, true);
	tcpci_source_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, -1);
}

static inline void typec_sink_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_attached_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SNK;

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
	if (tcpc_dev->typec_role >= TYPEC_ROLE_DRP)
		tcpc_reset_typec_try_timer(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap) {
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	typec_set_plug_orient(tcpc_dev, TYPEC_CC_RD,
		!typec_check_cc2(TYPEC_CC_VOLT_OPEN));
	tcpc_dev->typec_remote_rp_level = typec_get_cc_res();

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}

static inline void typec_custom_src_attached_entry(
	struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();

	if (cc1 == TYPEC_CC_VOLT_SNK_DFT && cc2 == TYPEC_CC_VOLT_SNK_DFT) {
		TYPEC_NEW_STATE(typec_attached_custom_src);
		tcpc_dev->typec_attach_new = TYPEC_ATTACHED_CUSTOM_SRC;
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	TYPEC_DBG("[Warning] Same Rp (%d)\r\n", typec_get_cc1());
#else
	TYPEC_DBG("[Warning] CC Both Rp\r\n");
#endif
}

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK

static inline uint8_t typec_get_sink_dbg_acc_rp_level(
	int cc1, int cc2)
{
	if (cc2 == TYPEC_CC_VOLT_SNK_DFT)
		return cc1;

	return TYPEC_CC_VOLT_SNK_DFT;
}

static inline void typec_sink_dbg_acc_attached_entry(
	struct tcpc_device *tcpc_dev)
{
	bool polarity;
	uint8_t rp_level;

	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();

	if (cc1 == cc2) {
		typec_custom_src_attached_entry(tcpc_dev);
		return;
	}

	TYPEC_NEW_STATE(typec_attached_dbgacc_snk);

	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_DBGACC_SNK;

	polarity = cc2 > cc1;

	if (polarity)
		rp_level = typec_get_sink_dbg_acc_rp_level(cc2, cc1);
	else
		rp_level = typec_get_sink_dbg_acc_rp_level(cc1, cc2);

	typec_set_plug_orient(tcpc_dev, TYPEC_CC_RD, polarity);
	tcpc_dev->typec_remote_rp_level = rp_level;

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}
#else
static inline void typec_sink_dbg_acc_attached_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_custom_src_attached_entry(tcpc_dev);
}
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */


/*
 * [BLOCK] Try.SRC / TryWait.SNK
 */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE

static inline bool typec_role_is_try_src(
	struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_role != TYPEC_ROLE_TRY_SRC)
		return false;

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap)
		return false;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	return true;
}

static inline void typec_try_src_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_try_src);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

static inline void typec_trywait_snk_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_trywait_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	tcpci_set_vconn(tcpc_dev, false);
	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
	tcpci_source_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_0V, 0);
	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

	tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
}

static inline void typec_trywait_snk_pe_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->typec_attach_old) {
		TYPEC_NEW_STATE(typec_trywait_snk_pe);
		return;
	}
#endif

	typec_trywait_snk_entry(tcpc_dev);
}

#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

/*
 * [BLOCK] Try.SNK / TryWait.SRC
 */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK

static inline bool typec_role_is_try_sink(
	struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_role != TYPEC_ROLE_TRY_SNK)
		return false;

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap)
		return false;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	return true;
}

static inline void typec_try_snk_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_try_snk);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

static inline void typec_trywait_src_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_trywait_src);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_0V, 0);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

/*
 * [BLOCK] Attach / Detach
 */

static inline void typec_cc_snk_detect_vsafe5v_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (!typec_check_cc_any(TYPEC_CC_VOLT_OPEN)) {	/* Both Rp */
		typec_sink_dbg_acc_attached_entry(tcpc_dev);
		return;
	}

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (typec_role_is_try_src(tcpc_dev)) {
		if (tcpc_dev->typec_state == typec_attachwait_snk) {
			typec_try_src_entry(tcpc_dev);
			return;
		}
	}
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_sink_attached_entry(tcpc_dev);
}

static inline void typec_cc_snk_detect_entry(struct tcpc_device *tcpc_dev)
{
	/* If Port Partner act as Source without VBUS, wait vSafe5V */
	if (tcpci_check_vbus_valid(tcpc_dev))
		typec_cc_snk_detect_vsafe5v_entry(tcpc_dev);
	else
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SNK_VSAFE5V);
}

static inline void typec_cc_src_detect_vsafe0v_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if (typec_role_is_try_sink(tcpc_dev)) {
		if (tcpc_dev->typec_state == typec_attachwait_src) {
			typec_try_snk_entry(tcpc_dev);
			return;
		}
	}
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_source_attached_entry(tcpc_dev);
}

static inline void typec_cc_src_detect_entry(
	struct tcpc_device *tcpc_dev)
{
	/* If Port Partner act as Sink with low VBUS, wait vSafe0v */
	bool vbus_absent = tcpci_check_vsafe0v(tcpc_dev, true);

	if (vbus_absent)
		typec_cc_src_detect_vsafe0v_entry(tcpc_dev);
	else
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SRC_VSAFE0V);
}

static inline void typec_cc_src_remove_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (typec_role_is_try_src(tcpc_dev)) {
		switch (tcpc_dev->typec_state) {
		case typec_attached_src:
			typec_trywait_snk_pe_entry(tcpc_dev);
			return;
		case typec_try_src:
			typec_trywait_snk_entry(tcpc_dev);
			return;
		}
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
}

static inline void typec_cc_snk_remove_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if (tcpc_dev->typec_state == typec_try_snk) {
		typec_trywait_src_entry(tcpc_dev);
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
}

/*
 * [BLOCK] Check CC Stable
 */

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE

static inline bool typec_check_cc_stable_source(
	struct tcpc_device *tcpc_dev)
{
	int ret, cc1a, cc2a, cc1b, cc2b;
	bool check_stable = false;

	if (!(tcpc_dev->tcpc_flags & TCPC_FLAGS_CHECK_CC_STABLE))
		return true;

	cc1a = typec_get_cc1();
	cc2a = typec_get_cc2();

	if ((cc1a == TYPEC_CC_VOLT_RD) && (cc2a == TYPEC_CC_VOLT_RD))
		check_stable = true;

	if ((cc1a == TYPEC_CC_VOLT_RA) || (cc2a == TYPEC_CC_VOLT_RA))
		check_stable = true;

	if (check_stable) {
		TYPEC_INFO("CC Stable Check...\r\n");
		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a)) {
			TYPEC_INFO("CC Unstable... %d/%d\r\n", cc1b, cc2b);

			if ((cc1b == TYPEC_CC_VOLT_RD) &&
				(cc2b != TYPEC_CC_VOLT_RD))
				return true;

			if ((cc1b != TYPEC_CC_VOLT_RD) &&
				(cc2b == TYPEC_CC_VOLT_RD))
				return true;

			typec_cc_src_remove_entry(tcpc_dev);
			return false;
		}

		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a)) {
			TYPEC_INFO("CC Unstable1... %d/%d\r\n", cc1b, cc2b);

			if ((cc1b == TYPEC_CC_VOLT_RD) &&
						(cc2b != TYPEC_CC_VOLT_RD))
				return true;

			if ((cc1b != TYPEC_CC_VOLT_RD) &&
						(cc2b == TYPEC_CC_VOLT_RD))
				return true;

			typec_cc_src_remove_entry(tcpc_dev);
			return false;
		}
	}

	return true;
}

static inline bool typec_check_cc_stable_sink(
	struct tcpc_device *tcpc_dev)
{
	int ret, cc1a, cc2a, cc1b, cc2b;

	if (!(tcpc_dev->tcpc_flags & TCPC_FLAGS_CHECK_CC_STABLE))
		return true;

	cc1a = typec_get_cc1();
	cc2a = typec_get_cc2();

	if ((cc1a != TYPEC_CC_VOLT_OPEN) && (cc2a != TYPEC_CC_VOLT_OPEN)) {
		TYPEC_INFO("CC Stable Check...\r\n");
		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a))
			TYPEC_INFO("CC Unstable... %d/%d\r\n", cc1b, cc2b);
	}

	return true;
}

#endif

/*
 * [BLOCK] Check Legacy Cable
 */

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE

static inline void typec_legacy_reset_cable_once(
	struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	uint8_t data = 0;

	if (tcpc_dev->tcpc_flags & TCPC_FLAGS_PREFER_LEGACY2)
		data = TCPC_LEGACY_CABLE2_CONFIRM;

	tcpc_dev->typec_legacy_cable_once = data;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */
}

static inline void typec_legacy_enable_discharge(
	struct tcpc_device *tcpc_dev, bool en)
{
#ifdef CONFIG_TYPEC_CAP_FORCE_DISCHARGE
	if (tcpc_dev->tcpc_flags & TCPC_FLAGS_PREFER_LEGACY2) {
		if (en)
			tcpci_enable_force_discharge(tcpc_dev, 0);
		else
			tcpci_disable_force_discharge(tcpc_dev);
	}
#endif	/* CONFIG_TYPEC_CAP_FORCE_DISCHARGE */
}

static inline void typec_legacy_keep_default_rp(
	struct tcpc_device *tcpc_dev, bool en)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	typec_legacy_enable_discharge(tcpc_dev, en);

	if (en) {
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		mdelay(1);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
		mdelay(1);
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */
}

static inline bool typec_legacy_charge(
	struct tcpc_device *tcpc_dev)
{
	int i, vbus_level = 0;

	TYPEC_INFO("LC->Charge\r\n");
	tcpci_source_vbus(tcpc_dev,
		TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, 100);

	for (i = 0; i < 6; i++) { /* 275 ms */
		vbus_level = tcpm_inquire_vbus_level(tcpc_dev, true);
		if (vbus_level >= TCPC_VBUS_VALID)
			return true;
		msleep(50);
	}

	TYPEC_INFO("LC->Charge Failed\r\n");
	return false;
}

static inline bool typec_legacy_discharge(
	struct tcpc_device *tcpc_dev)
{
	int i, vbus_level = 0;

	TYPEC_INFO("LC->Discharge\r\n");
	tcpci_source_vbus(tcpc_dev,
		TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_0V, 0);

	for (i = 0; i < 13; i++) { /* 650 ms */
		vbus_level = tcpm_inquire_vbus_level(tcpc_dev, true);
		if (vbus_level < TCPC_VBUS_VALID)
			return true;
		msleep(50);
	}

	TYPEC_INFO("LC->Discharge Failed\r\n");
	return false;
}

static inline bool typec_legacy_suspect(struct tcpc_device *tcpc_dev)
{
	TYPEC_INFO("LC->Suspect\r\n");
	tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_1_5);
	tcpc_dev->typec_legacy_cable_suspect = 0;
	mdelay(1);

	return tcpci_get_cc(tcpc_dev) != 0;
}

static inline bool typec_legacy_stable1(struct tcpc_device *tcpc_dev)
{
	typec_legacy_charge(tcpc_dev);
	typec_legacy_discharge(tcpc_dev);
	TYPEC_INFO("LC->Stable\r\n");
	tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY_STABLE);

	return true;
}

static inline bool typec_legacy_stable2(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2

	if (tcpc_dev->typec_legacy_cable_once
		>= TCPC_LEGACY_CABLE2_CONFIRM) {
		tcpc_dev->typec_legacy_cable = 2;
		TYPEC_INFO("LC->Stable2\r\n");
		typec_legacy_keep_default_rp(tcpc_dev, true);

#ifdef CONFIG_TYPEC_LEGACY2_AUTO_RECYCLE
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY_RECYCLE);
#endif	/* CONFIG_TYPEC_LEGACY2_AUTO_RECYCLE */

		return true;
	}

	tcpc_dev->typec_legacy_cable_once++;
	TYPEC_DBG("LC->StableOnce%d\r\n", tcpc_dev->typec_legacy_cable_once);

#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */
	return false;
}

static inline bool typec_legacy_confirm(struct tcpc_device *tcpc_dev)
{
	TYPEC_INFO("LC->Confirm\r\n");
	tcpc_dev->typec_legacy_cable = 1;
	tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_NOT_LEGACY);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	if (typec_legacy_stable2(tcpc_dev))
		return true;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */

	return typec_legacy_stable1(tcpc_dev);
}

static inline bool typec_legacy_check_cable(struct tcpc_device *tcpc_dev)
{
	bool check_legacy = false;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	if (tcpc_dev->typec_legacy_cable == 2) {
		TYPEC_NEW_STATE(typec_unattached_src);
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_DRP_SRC_TOGGLE);
		return true;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */

	if (typec_check_cc(TYPEC_CC_VOLT_RD, TYPEC_CC_VOLT_OPEN) ||
		typec_check_cc(TYPEC_CC_VOLT_OPEN, TYPEC_CC_VOLT_RD))
		check_legacy = true;

	if (check_legacy &&
		tcpc_dev->typec_legacy_cable_suspect >=
					TCPC_LEGACY_CABLE_CONFIRM) {

		if (typec_legacy_suspect(tcpc_dev)) {
			typec_legacy_confirm(tcpc_dev);
			return true;
		}

		tcpc_dev->typec_legacy_cable = false;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	}

	return false;
}

static inline void typec_legacy_reset_timer(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	uint32_t timer_id;

	if (tcpc_dev->typec_legacy_cable == 1)
		timer_id = TYPEC_RT_TIMER_LEGACY_STABLE;
	else
		timer_id = TYPEC_RT_TIMER_LEGACY_RECYCLE;

	tcpc_disable_timer(tcpc_dev, timer_id);
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */
}

static inline void typec_legacy_reach_vsafe5v(struct tcpc_device *tcpc_dev)
{
	TYPEC_INFO("LC->Attached\r\n");
	tcpc_dev->typec_legacy_cable = false;
	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
	typec_legacy_reset_timer(tcpc_dev);
}

static inline void typec_legacy_reach_vsafe0v(struct tcpc_device *tcpc_dev)
{
	TYPEC_INFO("LC->Detached (PS)\r\n");
	tcpc_dev->typec_legacy_cable = false;
	typec_set_drp_toggling(tcpc_dev);
	tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY_STABLE);
}

static inline void typec_legacy_handle_ps_change(
	struct tcpc_device *tcpc_dev, int vbus_level)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	if (tcpc_dev->typec_legacy_cable != 1)
		return;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */

	if (vbus_level >= TCPC_VBUS_VALID)
		typec_legacy_reach_vsafe5v(tcpc_dev);
	else if (vbus_level == TCPC_VBUS_SAFE0V)
		typec_legacy_reach_vsafe0v(tcpc_dev);
}

static inline void typec_legacy_handle_detach(struct tcpc_device *tcpc_dev)
{
	bool suspect_legacy = false;

	if (tcpc_dev->typec_state == typec_attachwait_src)
		suspect_legacy = true;
	else if (tcpc_dev->typec_state == typec_attached_src) {
		if (tcpc_dev->typec_attach_old != TYPEC_ATTACHED_SRC)
			suspect_legacy = true;
	}

	if (suspect_legacy) {
		tcpc_dev->typec_legacy_cable_suspect++;
		TYPEC_DBG("LC->Suspect: %d\r\n",
			tcpc_dev->typec_legacy_cable_suspect);
	}
}

static inline int typec_legacy_handle_cc_open(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	if (tcpc_dev->typec_legacy_cable == 2) {
		tcpc_dev->typec_legacy_cable_once = 0;
		typec_legacy_keep_default_rp(tcpc_dev, false);
		return 1;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */

	return 0;
}

static inline int typec_legacy_handle_cc_present(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	return (tcpc_dev->typec_legacy_cable == 1);
#else
	return 1;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */
}

static inline int typec_legacy_handle_cc_change(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	if (typec_is_cc_open())
		ret = typec_legacy_handle_cc_open(tcpc_dev);
	else
		ret = typec_legacy_handle_cc_present(tcpc_dev);

	if (ret == 0)
		return 0;

	TYPEC_INFO("LC->Detached (CC)\r\n");

	tcpc_dev->typec_legacy_cable = false;
	typec_set_drp_toggling(tcpc_dev);
	typec_legacy_reset_timer(tcpc_dev);
	return 1;
}

#endif /* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

/*
 * [BLOCK] CC Change (after debounce)
 */

static inline bool typec_debug_acc_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_debugaccessory);
	TYPEC_DBG("[Debug] CC1&2 Both Rd\r\n");
	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_DEBUG;
	return true;
}

#ifdef CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS
static inline bool typec_audio_acc_sink_vbus(
	struct tcpc_device *tcpc_dev, bool vbus_valid)
{
	if (vbus_valid) {
		tcpci_report_power_control(tcpc_dev, true);
		tcpci_sink_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, 500);
	} else {
		tcpci_sink_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_0V, 0);
		tcpci_report_power_control(tcpc_dev, false);
	}

	return true;
}
#endif	/* CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS */

static inline bool typec_audio_acc_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_audioaccessory);
	TYPEC_DBG("[Audio] CC1&2 Both Ra\r\n");
	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_AUDIO;

#ifdef CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS
	if (tcpci_check_vbus_valid(tcpc_dev))
		typec_audio_acc_sink_vbus(tcpc_dev, true);
#endif	/* CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS */

	return true;
}

static inline bool typec_cc_change_source_entry(struct tcpc_device *tcpc_dev)
{
	bool src_remove = false;

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE
	if (!typec_check_cc_stable_source(tcpc_dev))
		return true;
#endif

	switch (tcpc_dev->typec_state) {
	case typec_attached_src:
		if (typec_get_cc_res() != TYPEC_CC_VOLT_RD)
			src_remove = true;
		break;
	case typec_audioaccessory:
		if (!typec_check_cc_both(TYPEC_CC_VOLT_RA))
			src_remove = true;
		break;
	case typec_debugaccessory:
		if (!typec_check_cc_both(TYPEC_CC_VOLT_RD))
			src_remove = true;
		break;
	default:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RD))
			typec_debug_acc_attached_entry(tcpc_dev);
		else if (typec_check_cc_both(TYPEC_CC_VOLT_RA))
			typec_audio_acc_attached_entry(tcpc_dev);
		else if (typec_check_cc_any(TYPEC_CC_VOLT_RD))
			typec_cc_src_detect_entry(tcpc_dev);
		else
			src_remove = true;
		break;
	}

	if (src_remove)
		typec_cc_src_remove_entry(tcpc_dev);

	return true;
}

static inline bool typec_attached_snk_cc_change(struct tcpc_device *tcpc_dev)
{
	uint8_t cc_res = typec_get_cc_res();

	if (cc_res != tcpc_dev->typec_remote_rp_level) {
#ifdef CONFIG_TYPEC_CAP_CUSTOM_HV
		if (tcpc_dev->typec_during_custom_hv)
			return true;
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_HV */

		TYPEC_INFO("RpLvl Change\r\n");
		tcpc_dev->typec_remote_rp_level = cc_res;

#ifdef CONFIG_USB_POWER_DELIVERY
		if (tcpc_dev->pd_port.pd_prev_connected)
			return true;
#endif	/* CONFIG_USB_POWER_DELIVERY */

		tcpci_sink_vbus(tcpc_dev,
				TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
	}

	return true;
}

static inline bool typec_cc_change_sink_entry(struct tcpc_device *tcpc_dev)
{
	bool snk_remove = false;

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE
	typec_check_cc_stable_sink(tcpc_dev);
#endif

	switch (tcpc_dev->typec_state) {
	case typec_attached_snk:
		if (typec_get_cc_res() == TYPEC_CC_VOLT_OPEN)
			snk_remove = true;
		else
			typec_attached_snk_cc_change(tcpc_dev);
		break;

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	case typec_attached_dbgacc_snk:
		if (typec_get_cc_res() == TYPEC_CC_VOLT_OPEN)
			snk_remove = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	case typec_attached_custom_src:
		if (typec_check_cc_any(TYPEC_CC_VOLT_OPEN))
			snk_remove = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

	default:
		if (!typec_is_cc_open())
			typec_cc_snk_detect_entry(tcpc_dev);
		else
			snk_remove = true;
	}

	if (snk_remove)
		typec_cc_snk_remove_entry(tcpc_dev);

	return true;
}

static inline bool typec_is_act_as_sink_role(
	struct tcpc_device *tcpc_dev)
{
	bool as_sink = true;
	uint8_t cc_sum;

	switch (tcpc_dev->typec_local_cc & 0x07) {
	case TYPEC_CC_RP:
		as_sink = false;
		break;
	case TYPEC_CC_RD:
		as_sink = true;
		break;
	case TYPEC_CC_DRP:
		cc_sum = typec_get_cc1() + typec_get_cc2();
		as_sink = (cc_sum >= TYPEC_CC_VOLT_SNK_DFT);
		break;
	}

	return as_sink;
}

static inline bool typec_handle_cc_changed_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_INFO("[CC_Change] %d/%d\r\n", typec_get_cc1(), typec_get_cc2());

	tcpc_dev->typec_attach_new = tcpc_dev->typec_attach_old;

	if (typec_is_act_as_sink_role(tcpc_dev))
		typec_cc_change_sink_entry(tcpc_dev);
	else
		typec_cc_change_source_entry(tcpc_dev);

	typec_alert_attach_state_change(tcpc_dev);
	return true;
}

/*
 * [BLOCK] Handle cc-change event (from HW)
 */

static inline void typec_attach_wait_entry(struct tcpc_device *tcpc_dev)
{
	bool as_sink;

#ifdef CONFIG_USB_POWER_DELIVERY
	bool pd_en = tcpc_dev->pd_port.pd_prev_connected;
#else
	bool pd_en = false;
#endif	/* CONFIG_USB_POWER_DELIVERY */

	if (tcpc_dev->typec_attach_old == TYPEC_ATTACHED_SNK && !pd_en) {
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		TYPEC_DBG("RpLvl Alert\r\n");
		return;
	}

	if (tcpc_dev->typec_attach_old ||
		tcpc_dev->typec_state == typec_attached_src) {
		tcpc_reset_typec_debounce_timer(tcpc_dev);
		TYPEC_DBG("Attached, Ignore cc_attach\r\n");
		return;
	}

	switch (tcpc_dev->typec_state) {

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return;

	case typec_trywait_snk:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		return;
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_try_snk:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return;

	case typec_trywait_src:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		return;
#endif

#ifdef CONFIG_USB_POWER_DELIVERY
	case typec_unattachwait_pe:
		TYPEC_INFO("Force PE Idle\r\n");
		tcpc_dev->pd_wait_pe_idle = false;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_PE_IDLE);
		typec_unattached_power_entry(tcpc_dev);
		break;
#endif
	}

	as_sink = typec_is_act_as_sink_role(tcpc_dev);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (!as_sink && typec_legacy_check_cable(tcpc_dev))
		return;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_TYPEC_NOTIFY_ATTACHWAIT
	tcpci_notify_attachwait_state(tcpc_dev, as_sink);
#endif	/* CONFIG_TYPEC_NOTIFY_ATTACHWAIT */

	if (as_sink)
		TYPEC_NEW_STATE(typec_attachwait_snk);
	else {
		if (!tcpci_check_vsafe0v(tcpc_dev, true)) {
			TYPEC_NEW_STATE(typec_unattached_src);
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_DRP_SRC_TOGGLE);
			typec_wait_ps_change(
				tcpc_dev, TYPEC_WAIT_PS_SRC_VSAFE0V);
			return;
		}

		TYPEC_NEW_STATE(typec_attachwait_src);
	}

	tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
}

#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
static inline int typec_attached_snk_cc_detach(struct tcpc_device *tcpc_dev)
{
	int vbus_valid = tcpci_check_vbus_valid(tcpc_dev);
	bool detach_by_cc = false;

	/* For Ellisys Test, Applying Low VBUS (3.67v) as Sink */
	if (vbus_valid) {
		detach_by_cc = true;
		TYPEC_DBG("Detach_CC (LowVBUS)\r\n");
	}

#ifdef CONFIG_USB_POWER_DELIVERY
	/* For Source detach during HardReset */
	if ((!vbus_valid) &&
		tcpc_dev->pd_wait_hard_reset_complete) {
		detach_by_cc = true;
		TYPEC_DBG("Detach_CC (HardReset)\r\n");
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

	if (detach_by_cc)
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);

	return 0;
}
#endif	/* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

static inline void typec_detach_wait_entry(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	typec_legacy_handle_detach(tcpc_dev);
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	switch (tcpc_dev->typec_state) {
#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
	case typec_attached_snk:
		typec_attached_snk_cc_detach(tcpc_dev);
		break;
#endif /* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

	case typec_audioaccessory:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		break;

#ifdef TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE
	case typec_attached_src:
		TYPEC_INFO("Exit Attached.SRC immediately\r\n");
		tcpc_reset_typec_debounce_timer(tcpc_dev);

		/* force to terminate TX */
		tcpci_init(tcpc_dev, true);

		typec_cc_src_remove_entry(tcpc_dev);
		typec_alert_attach_state_change(tcpc_dev);
		break;
#endif /* TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		if (tcpc_dev->typec_drp_try_timeout)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		else {
			tcpc_reset_typec_debounce_timer(tcpc_dev);
			TYPEC_DBG("[Try] Ignore cc_detach\r\n");
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_trywait_src:
		if (tcpc_dev->typec_drp_try_timeout)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		else {
			tcpc_reset_typec_debounce_timer(tcpc_dev);
			TYPEC_DBG("[Try] Ignore cc_detach\r\n");
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */
	default:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		break;
	}
}

static inline bool typec_is_cc_attach(struct tcpc_device *tcpc_dev)
{
	bool cc_attach = false;
	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();
	int cc_res = typec_get_cc_res();

	tcpc_dev->typec_cable_only = false;

	switch (tcpc_dev->typec_attach_old) {
	case TYPEC_ATTACHED_SNK:
	case TYPEC_ATTACHED_SRC:
		if ((cc_res != TYPEC_CC_VOLT_OPEN) &&
				(cc_res != TYPEC_CC_VOLT_RA))
			cc_attach = true;
		break;

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	case TYPEC_ATTACHED_CUSTOM_SRC:
		if ((cc_res != TYPEC_CC_VOLT_OPEN) &&
				(cc_res != TYPEC_CC_VOLT_RA))
			cc_attach = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	case TYPEC_ATTACHED_DBGACC_SNK:
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */
		if ((cc_res != TYPEC_CC_VOLT_OPEN) &&
				(cc_res != TYPEC_CC_VOLT_RA))
			cc_attach = true;
		break;

	case TYPEC_ATTACHED_AUDIO:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RA))
			cc_attach = true;
		break;

	case TYPEC_ATTACHED_DEBUG:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RD))
			cc_attach = true;
		break;

	default:	/* TYPEC_UNATTACHED */
		if (cc1 != TYPEC_CC_VOLT_OPEN)
			cc_attach = true;

		if (cc2 != TYPEC_CC_VOLT_OPEN)
			cc_attach = true;

		/* Cable Only, no device */
		if ((cc1+cc2) == TYPEC_CC_VOLT_RA) {
			cc_attach = false;
			tcpc_dev->typec_cable_only = true;
			TYPEC_DBG("[Cable] Ra Only\r\n");
		}
		break;
	}

	return cc_attach;
}

int tcpc_typec_handle_ra_detach(struct tcpc_device *tcpc_dev)
{
	bool retry_lpm = false;
	bool drp = tcpc_dev->typec_role >= TYPEC_ROLE_DRP;

	if (tcpc_dev->typec_lpm) {
		if (drp) {
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
			mdelay(1);
		}

		tcpci_get_cc(tcpc_dev);
		tcpc_dev->typec_cable_only = !typec_is_cc_open();

		if (drp) {
			tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
			tcpci_alert_status_clear(tcpc_dev,
				TCPC_REG_ALERT_EXT_RA_DETACH);
		}

		if (tcpc_dev->typec_cable_only) {
			TYPEC_DBG("False_RaDetach\r\n");
			return 0;
		}

		if (!drp)
			retry_lpm = true;
	} else if (tcpc_dev->typec_cable_only) {
		retry_lpm = true;
	}

	if (retry_lpm) {
		tcpc_dev->typec_lpm = true;
		TYPEC_DBG("EnterLPM_RaDetach\r\n");
		tcpci_set_low_power_mode(tcpc_dev, true,
			(drp) ? TYPEC_CC_DRP : TYPEC_CC_RP);
	}

	return 0;
}

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
static inline int typec_handle_try_sink_cc_change(
	struct tcpc_device *tcpc_dev)
{
	if (!tcpc_dev->typec_drp_try_timeout) {
		TYPEC_DBG("[Try.SNK] Ignore CC_Alert\r\n");
		return 1;
	}

	if (!typec_is_cc_open()) {
		tcpci_notify_attachwait_state(tcpc_dev, true);
		return 0;
	}

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

int tcpc_typec_handle_cc_change(struct tcpc_device *tcpc_dev)
{
	int ret = tcpci_get_cc(tcpc_dev);

	if (ret < 0)
		return ret;

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		if (tcpc_dev->typec_lpm && !tcpc_dev->typec_cable_only)
			typec_enter_low_power_mode(tcpc_dev);
		return 0;
	}

	TYPEC_INFO("[CC_Alert] %d/%d\r\n", typec_get_cc1(), typec_get_cc2());

	typec_disable_low_power_mode(tcpc_dev);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable &&
		typec_legacy_handle_cc_change(tcpc_dev)) {
		return 0;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore CC_Alert\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery) {
		TYPEC_DBG("[Recovery] Ignore CC_Alert\r\n");
		return 0;
	}
#endif /* CONFIG_USB_POWER_DELIVERY */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if (tcpc_dev->typec_state == typec_try_snk) {
		if (typec_handle_try_sink_cc_change(tcpc_dev) > 0)
			return 0;
	}

	if (tcpc_dev->typec_state == typec_trywait_src_pe) {
		TYPEC_DBG("[Try.PE] Ignore CC_Alert\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc_dev->typec_state == typec_trywait_snk_pe) {
		TYPEC_DBG("[Try.PE] Ignore CC_Alert\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	if (tcpc_dev->typec_state == typec_attachwait_snk
		|| tcpc_dev->typec_state == typec_attachwait_src)
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (typec_is_cc_attach(tcpc_dev))
		typec_attach_wait_entry(tcpc_dev);
	else
		typec_detach_wait_entry(tcpc_dev);

	return 0;
}

/*
 * [BLOCK] Handle timeout event
 */

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
static inline int typec_handle_drp_try_timeout(struct tcpc_device *tcpc_dev)
{
	bool src_detect = false, en_timer;

	tcpc_dev->typec_drp_try_timeout = true;
	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	if (typec_check_cc1(TYPEC_CC_VOLT_RD) ||
		typec_check_cc2(TYPEC_CC_VOLT_RD)) {
		src_detect = true;
	}

	switch (tcpc_dev->typec_state) {
#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		en_timer = !src_detect;
		break;
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_trywait_src:
		en_timer = !src_detect;
		break;

	case typec_try_snk:
		en_timer = true;
		if (!typec_is_cc_open())
			tcpci_notify_attachwait_state(tcpc_dev, true);
		break;
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

	default:
		en_timer = false;
		break;
	}

	if (en_timer)
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

static inline int typec_handle_debounce_timeout(struct tcpc_device *tcpc_dev)
{
	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY);
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	typec_handle_cc_changed_entry(tcpc_dev);
	return 0;
}

#ifdef CONFIG_USB_POWER_DELIVERY

static inline int typec_handle_error_recovery_timeout(
						struct tcpc_device *tcpc_dev)
{
	/* TODO: Check it later */
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

	mutex_lock(&tcpc_dev->access_lock);
	tcpc_dev->pd_wait_error_recovery = false;
	if (tcpc_dev->pd_transmit_state >= PD_TX_STATE_WAIT) {
		TYPEC_INFO("Still wait tx_done\r\n");
		tcpc_dev->pd_transmit_state = PD_TX_STATE_GOOD_CRC;
	}
	mutex_unlock(&tcpc_dev->access_lock);

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
	typec_alert_attach_state_change(tcpc_dev);

	return 0;
}

static inline int typec_handle_pe_idle(struct tcpc_device *tcpc_dev)
{
	switch (tcpc_dev->typec_state) {

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_trywait_snk_pe:
		typec_trywait_snk_entry(tcpc_dev);
		break;
#endif

	case typec_unattachwait_pe:
		typec_unattached_entry(tcpc_dev);
		break;

	default:
		TYPEC_DBG("Dummy pe_idle\r\n");
		break;
	}

	return 0;
}
#endif /* CONFIG_USB_POWER_DELIVERY */

static inline int typec_handle_src_reach_vsafe0v(struct tcpc_device *tcpc_dev)
{
	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	if (tcpc_dev->typec_state == typec_unattached_src) {
		TYPEC_NEW_STATE(typec_attachwait_src);
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		tcpc_disable_timer(tcpc_dev, TYPEC_TIMER_DRP_SRC_TOGGLE);
		return 0;
	}

	typec_cc_src_detect_vsafe0v_entry(tcpc_dev);
	return 0;
}

static inline int typec_handle_src_toggle_timeout(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap)
		return 0;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	if (tcpc_dev->typec_state == typec_unattached_src) {
		TYPEC_NEW_STATE(typec_unattached_snk);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
	}

	return 0;
}

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
static inline int typec_handle_role_swap_start(struct tcpc_device *tcpc_dev)
{
	uint8_t role_swap = tcpc_dev->typec_during_role_swap;

	if (role_swap == TYPEC_ROLE_SWAP_TO_SNK) {
		TYPEC_INFO("Role Swap to Sink\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	} else if (role_swap == TYPEC_ROLE_SWAP_TO_SRC) {
		TYPEC_INFO("Role Swap to Source\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	}

	return 0;
}

static inline int typec_handle_role_swap_stop(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_during_role_swap) {
		TYPEC_INFO("TypeC Role Swap Failed\r\n");
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
	}

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

int tcpc_typec_handle_timeout(struct tcpc_device *tcpc_dev, uint32_t timer_id)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
	if (timer_id == TYPEC_TRY_TIMER_DRP_TRY)
		return typec_handle_drp_try_timeout(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

	if (timer_id >= TYPEC_TIMER_START_ID)
		tcpc_reset_typec_debounce_timer(tcpc_dev);
	else if (timer_id >= TYPEC_RT_TIMER_START_ID)
		tcpc_disable_timer(tcpc_dev, timer_id);

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore timer_evt\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery &&
		(timer_id != TYPEC_TIMER_ERROR_RECOVERY)) {
		TYPEC_DBG("[Recovery] Ignore timer_evt\r\n");
		return 0;
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

	switch (timer_id) {
	case TYPEC_TIMER_CCDEBOUNCE:
	case TYPEC_TIMER_PDDEBOUNCE:
		ret = typec_handle_debounce_timeout(tcpc_dev);
		break;

#ifdef CONFIG_USB_POWER_DELIVERY
	case TYPEC_TIMER_ERROR_RECOVERY:
		ret = typec_handle_error_recovery_timeout(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_PE_IDLE:
		ret = typec_handle_pe_idle(tcpc_dev);
		break;
#endif /* CONFIG_USB_POWER_DELIVERY */

	case TYPEC_RT_TIMER_SAFE0V_DELAY:
		ret = typec_handle_src_reach_vsafe0v(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_LOW_POWER_MODE:
		if (tcpc_dev->typec_lpm)
			typec_try_low_power_mode(tcpc_dev);
		break;

	case TYPEC_TIMER_WAKEUP:
		tcpc_typec_handle_ra_detach(tcpc_dev);
		break;

#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
	case TYPEC_RT_TIMER_SAFE0V_TOUT:
		if (!tcpci_check_vbus_valid(tcpc_dev))
			ret = tcpc_typec_handle_vsafe0v(tcpc_dev);
		else
			TYPEC_INFO("VBUS still Valid!!\r\n");
		break;
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */

	case TYPEC_TIMER_DRP_SRC_TOGGLE:
		ret = typec_handle_src_toggle_timeout(tcpc_dev);
		break;

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	case TYPEC_RT_TIMER_ROLE_SWAP_START:
		typec_handle_role_swap_start(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_ROLE_SWAP_STOP:
		typec_handle_role_swap_stop(tcpc_dev);
		break;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

#ifdef CONFIG_TYPEC_CAP_AUTO_DISCHARGE
	case TYPEC_RT_TIMER_AUTO_DISCHARGE:
		if (!tcpc_dev->typec_power_ctrl) {
			tcpci_enable_ext_discharge(tcpc_dev, false);
			tcpci_enable_auto_discharge(tcpc_dev, false);
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_AUTO_DISCHARGE */

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	case TYPEC_RT_TIMER_LEGACY:
		typec_alert_attach_state_change(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_NOT_LEGACY:
		tcpc_dev->typec_legacy_cable = false;
		tcpc_dev->typec_legacy_cable_suspect = 0;
		typec_legacy_reset_cable_once(tcpc_dev);
		break;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE2
	case TYPEC_RT_TIMER_LEGACY_STABLE:
		if (tcpc_dev->typec_legacy_cable)
			typec_legacy_reset_cable_once(tcpc_dev);
		break;

#ifdef CONFIG_TYPEC_LEGACY2_AUTO_RECYCLE
	case TYPEC_RT_TIMER_LEGACY_RECYCLE:
		if (tcpc_dev->typec_legacy_cable == 2) {
			TYPEC_INFO("LC->Recycle\r\n");
			tcpc_dev->typec_legacy_cable = false;
			typec_legacy_keep_default_rp(tcpc_dev, false);
			typec_set_drp_toggling(tcpc_dev);
		}
		break;
#endif	/* CONFIG_TYPEC_LEGACY2_AUTO_RECYCLE */
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE2 */
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */
	}

	return ret;
}

/*
 * [BLOCK] Handle ps-change event
 */

static inline int typec_handle_vbus_present(struct tcpc_device *tcpc_dev)
{
	switch (tcpc_dev->typec_wait_ps_change) {
	case TYPEC_WAIT_PS_SNK_VSAFE5V:
		typec_cc_snk_detect_vsafe5v_entry(tcpc_dev);
		typec_alert_attach_state_change(tcpc_dev);
		break;
	case TYPEC_WAIT_PS_SRC_VSAFE5V:
		typec_source_attached_with_vbus_entry(tcpc_dev);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
		if (typec_get_cc_res() != TYPEC_CC_VOLT_RD) {
			TYPEC_DBG("Postpone AlertChange\r\n");
			tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY);
			break;
		}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

		typec_alert_attach_state_change(tcpc_dev);
		break;
	}

	return 0;
}

static inline int typec_attached_snk_vbus_absent(struct tcpc_device *tcpc_dev)
{
#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
#ifdef CONFIG_USB_POWER_DELIVERY
#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
	if (tcpc_dev->pd_during_direct_charge &&
		!tcpci_check_vsafe0v(tcpc_dev, true)) {
		TYPEC_DBG("Ignore vbus_absent(snk), DirectCharge\r\n");
		return 0;
	}
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */

	if (tcpc_dev->pd_wait_hard_reset_complete ||
				tcpc_dev->pd_hard_reset_event_pending) {
		if (typec_get_cc_res() != TYPEC_CC_VOLT_OPEN) {
			TYPEC_DBG
			    ("Ignore vbus_absent(snk), HReset & CC!=0\r\n");
			return 0;
		}
	}
#endif /* CONFIG_USB_POWER_DELIVERY */

#ifdef CONFIG_RT7207_ADAPTER
	if (tcpc_dev->rt7207_direct_charge_flag) {
		if (typec_get_cc_res() != TYPEC_CC_VOLT_OPEN &&
				!tcpci_check_vsafe0v(tcpc_dev, true)) {
			TYPEC_DBG("Ignore vbus_absent(snk), Dircet Charging\n");
			return 0;
		}
	}
#endif /* CONFIG_RT7207_ADAPTER */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
	typec_alert_attach_state_change(tcpc_dev);
#endif /* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

	return 0;
}


static inline int typec_handle_vbus_absent(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore vbus_absent\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery) {
		TYPEC_DBG("[Recovery] Ignore vbus_absent\r\n");
		return 0;
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

	if (tcpc_dev->typec_state == typec_attached_snk)
		typec_attached_snk_vbus_absent(tcpc_dev);

#ifndef CONFIG_TCPC_VSAFE0V_DETECT
	tcpc_typec_handle_vsafe0v(tcpc_dev);
#endif /* #ifdef CONFIG_TCPC_VSAFE0V_DETECT */

	return 0;
}

int tcpc_typec_handle_ps_change(struct tcpc_device *tcpc_dev, int vbus_level)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable) {
		typec_legacy_handle_ps_change(tcpc_dev, vbus_level);
		return 0;
	}
#endif /* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

#ifdef CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS
	if (tcpc_dev->typec_state == typec_audioaccessory) {
		return typec_audio_acc_sink_vbus(
			tcpc_dev, vbus_level >= TCPC_VBUS_VALID);
	}
#endif	/* CONFIG_TYPEC_CAP_AUDIO_ACC_SINK_VBUS */

	if (vbus_level >= TCPC_VBUS_VALID)
		return typec_handle_vbus_present(tcpc_dev);

	return typec_handle_vbus_absent(tcpc_dev);
}

/*
 * [BLOCK] Handle PE event
 */

#ifdef CONFIG_USB_POWER_DELIVERY

int tcpc_typec_advertise_explicit_contract(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_local_rp_level == TYPEC_CC_RP_DFT)
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_1_5);

	return 0;
}

int tcpc_typec_handle_pe_pr_swap(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	mutex_lock(&tcpc_dev->typec_lock);
	switch (tcpc_dev->typec_state) {
	case typec_attached_snk:
		TYPEC_NEW_STATE(typec_attached_src);
		tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SRC;
		tcpci_set_cc(tcpc_dev, tcpc_dev->typec_local_rp_level);
		break;
	case typec_attached_src:
		TYPEC_NEW_STATE(typec_attached_snk);
		tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SNK;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		break;
	default:
		break;
	}

	typec_alert_attach_state_change(tcpc_dev);
	mutex_unlock(&tcpc_dev->typec_lock);
	return ret;
}

#endif /* CONFIG_USB_POWER_DELIVERY */

/*
 * [BLOCK] Handle reach vSafe0V event
 */

int tcpc_typec_handle_vsafe0v(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_wait_ps_change == TYPEC_WAIT_PS_SRC_VSAFE0V) {
#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_DELAY);
#else
		typec_handle_src_reach_vsafe0v(tcpc_dev);
#endif
	}

	return 0;
}

/*
 * [BLOCK] TCPCI TypeC I/F
 */

static const char *const typec_role_name[] = {
	"UNKNOWN",
	"SNK",
	"SRC",
	"DRP",
	"TrySRC",
	"TrySNK",
};

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
int tcpc_typec_swap_role(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_role < TYPEC_ROLE_DRP)
		return TCPM_ERROR_NOT_DRP_ROLE;

	if (tcpc_dev->typec_during_role_swap)
		return TCPM_ERROR_DURING_ROLE_SWAP;

	switch (tcpc_dev->typec_attach_old) {
	case TYPEC_ATTACHED_SNK:
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_TO_SRC;
		break;
	case TYPEC_ATTACHED_SRC:
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_TO_SNK;
		break;
	}

	if (tcpc_dev->typec_during_role_swap) {
		TYPEC_INFO("TypeC Role Swap Start\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_START);
		return TCPM_SUCCESS;
	}

	return TCPM_ERROR_UNATTACHED;
}
#endif /* CONFIG_TYPEC_CAP_ROLE_SWAP */

int tcpc_typec_set_rp_level(struct tcpc_device *tcpc_dev, uint8_t res)
{
	switch (res) {
	case TYPEC_CC_RP_DFT:
	case TYPEC_CC_RP_1_5:
	case TYPEC_CC_RP_3_0:
		TYPEC_INFO("TypeC-Rp: %d\r\n", res);
		tcpc_dev->typec_local_rp_level = res;
		break;

	default:
		TYPEC_INFO("TypeC-Unknown-Rp (%d)\r\n", res);
		return -1;
	}

#ifdef CONFIG_USB_PD_DBG_ALWAYS_LOCAL_RP
	tcpci_set_cc(tcpc_dev, tcpc_dev->typec_local_rp_level);
#else
	if ((tcpc_dev->typec_attach_old != TYPEC_UNATTACHED) &&
		(tcpc_dev->typec_attach_new != TYPEC_UNATTACHED)) {
		return tcpci_set_cc(tcpc_dev, res);
	}
#endif

	return 0;
}

int tcpc_typec_change_role(
	struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	uint8_t local_cc;
	bool force_unattach = false;

	if (typec_role == TYPEC_ROLE_UNKNOWN ||
		typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\r\n", typec_role);
		return -1;
	}

	mutex_lock(&tcpc_dev->access_lock);

	tcpc_dev->typec_role = typec_role;
	TYPEC_INFO("typec_new_role: %s\r\n", typec_role_name[typec_role]);

	local_cc = tcpc_dev->typec_local_cc & 0x07;

	if (typec_role == TYPEC_ROLE_SNK && local_cc == TYPEC_CC_RP)
		force_unattach = true;

	if (typec_role == TYPEC_ROLE_SRC && local_cc == TYPEC_CC_RD)
		force_unattach = true;

	if (tcpc_dev->typec_attach_new == TYPEC_UNATTACHED) {
		force_unattach = true;
		typec_disable_low_power_mode(tcpc_dev);
	}

	if (force_unattach) {
		TYPEC_DBG("force_unattach\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
		mutex_unlock(&tcpc_dev->access_lock);
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return 0;
	}

	mutex_unlock(&tcpc_dev->access_lock);
	return 0;
}

#ifdef CONFIG_TYPEC_CAP_POWER_OFF_CHARGE
static int typec_init_power_off_charge(struct tcpc_device *tcpc_dev)
{
	int ret = tcpci_get_cc(tcpc_dev);

	if (ret < 0)
		return ret;

	if (tcpc_dev->typec_role == TYPEC_ROLE_SRC)
		return 0;

	if (typec_is_cc_open())
		return 0;

	if (!tcpci_check_vbus_valid(tcpc_dev))
		return 0;

	TYPEC_DBG("PowerOffCharge\r\n");

	TYPEC_NEW_STATE(typec_unattached_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);

	return 1;
}
#endif	/* CONFIG_TYPEC_CAP_POWER_OFF_CHARGE */

int tcpc_typec_init(struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	int ret = 0;

	if (typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\r\n", typec_role);
		return -2;
	}

	TYPEC_INFO("typec_init: %s\r\n", typec_role_name[typec_role]);

	tcpc_dev->typec_role = typec_role;
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;
	tcpc_dev->typec_attach_old = TYPEC_UNATTACHED;

	tcpc_dev->typec_remote_cc[0] = TYPEC_CC_VOLT_OPEN;
	tcpc_dev->typec_remote_cc[1] = TYPEC_CC_VOLT_OPEN;

	tcpc_dev->wake_lock_pd = 0;
	tcpc_dev->wake_lock_user = true;
	tcpc_dev->typec_usb_sink_curr = CONFIG_TYPEC_SNK_CURR_DFT;

#ifdef CONFIG_TYPEC_CAP_CUSTOM_HV
	tcpc_dev->typec_during_custom_hv = false;
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_HV */

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	tcpc_dev->typec_legacy_cable = false;
	tcpc_dev->typec_legacy_cable_suspect = 0;
	typec_legacy_reset_cable_once(tcpc_dev);
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_TYPEC_CAP_POWER_OFF_CHARGE
	ret = typec_init_power_off_charge(tcpc_dev);
	if (ret != 0)
		return ret;
#endif	/* CONFIG_TYPEC_CAP_POWER_OFF_CHARGE */

#ifdef CONFIG_TYPEC_POWER_CTRL_INIT
	tcpc_dev->typec_power_ctrl = true;
#endif	/* CONFIG_TYPEC_POWER_CTRL_INIT */

	typec_unattached_entry(tcpc_dev);
	return ret;
}

void  tcpc_typec_deinit(struct tcpc_device *tcpc_dev)
{
}
