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

#ifndef TCPM_H_
#define TCPM_H_

#include <linux/kernel.h>
#include <linux/notifier.h>

#include "tcpci_config.h"

struct tcpc_device;

/*
 * Type-C Port Notify Chain
 */

enum typec_attach_type {
	TYPEC_UNATTACHED = 0,
	TYPEC_ATTACHED_SNK,
	TYPEC_ATTACHED_SRC,
	TYPEC_ATTACHED_AUDIO,
	TYPEC_ATTACHED_DEBUG,		/* Rd, Rd */

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	TYPEC_ATTACHED_DBGACC_SNK,		/* Rp, Rp */
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	TYPEC_ATTACHED_CUSTOM_SRC,		/* Same Rp */
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */
};

enum pd_connect_result {
	PD_CONNECT_NONE = 0,
	PD_CONNECT_TYPEC_ONLY,	/* Internal Only */
	PD_CONNECT_TYPEC_ONLY_SNK_DFT,
	PD_CONNECT_TYPEC_ONLY_SNK,
	PD_CONNECT_TYPEC_ONLY_SRC,
	PD_CONNECT_PE_READY,	/* Internal Only */
	PD_CONNECT_PE_READY_SNK,
	PD_CONNECT_PE_READY_SRC,

#ifdef CONFIG_USB_PD_CUSTOM_DBGACC
	PD_CONNECT_PE_READY_DBGACC_UFP,
	PD_CONNECT_PE_READY_DBGACC_DFP,
#endif	/* CONFIG_USB_PD_CUSTOM_DBGACC */
};

#ifdef CONFIG_TCPC_VCONN_SUPPLY_MODE

enum tcpc_vconn_supply_mode {
	/* Never provide vconn even in TYPE-C state (reject swap to On) */
	TCPC_VCONN_SUPPLY_NEVER = 0,

	/* Always provide vconn */
	TCPC_VCONN_SUPPLY_ALWAYS,

	/* Always provide vconn only if we detect Ra, otherwise startup only */
	TCPC_VCONN_SUPPLY_EMARK_ONLY,

	/* Only provide vconn during DPM initial */
	TCPC_VCONN_SUPPLY_STARTUP,

	TCPC_VCONN_SUPPLY_NR,
};

#endif	/* CONFIG_TCPC_VCONN_SUPPLY_MODE */

/* Power role */
#define PD_ROLE_SINK   0
#define PD_ROLE_SOURCE 1

/* Data role */
#define PD_ROLE_UFP    0
#define PD_ROLE_DFP    1

/* Vconn role */
#define PD_ROLE_VCONN_OFF 0
#define PD_ROLE_VCONN_ON  1

enum {
	TCP_NOTIFY_DIS_VBUS_CTRL,
	TCP_NOTIFY_SOURCE_VCONN,
	TCP_NOTIFY_SOURCE_VBUS,
	TCP_NOTIFY_SINK_VBUS,
	TCP_NOTIFY_PR_SWAP,
	TCP_NOTIFY_DR_SWAP,
	TCP_NOTIFY_VCONN_SWAP,
	TCP_NOTIFY_ENTER_MODE,
	TCP_NOTIFY_EXIT_MODE,
	TCP_NOTIFY_AMA_DP_STATE,
	TCP_NOTIFY_AMA_DP_ATTENTION,
	TCP_NOTIFY_AMA_DP_HPD_STATE,

	TCP_NOTIFY_TYPEC_STATE,
	TCP_NOTIFY_PD_STATE,

#ifdef CONFIG_USB_PD_UVDM
	TCP_NOTIFY_UVDM,
#endif /* CONFIG_USB_PD_UVDM */

#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
	TCP_NOTIFY_DC_EN_UNLOCK,
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */

#ifdef CONFIG_TYPEC_NOTIFY_ATTACHWAIT_SNK
	TCP_NOTIFY_ATTACHWAIT_SNK,
#endif	/* CONFIG_TYPEC_NOTIFY_ATTACHWAIT_SNK */

#ifdef CONFIG_TYPEC_NOTIFY_ATTACHWAIT_SRC
	TCP_NOTIFY_ATTACHWAIT_SRC,
#endif	/* CONFIG_TYPEC_NOTIFY_ATTACHWAIT_SRC */

	TCP_NOTIFY_EXT_DISCHARGE,

	TCP_NOTIFY_HARD_RESET_STATE,

#ifdef CONFIG_USB_PD_REV30
	TCP_NOTIFY_ALERT,
	TCP_NOTIFY_STATUS,
	TCP_NOTIFY_PPS_STATUS,
	TCP_NOTIFY_PPS_READY,
#endif	/* CONFIG_USB_PD_REV30 */
};

struct tcp_ny_pd_state {
	uint8_t connected;
};

struct tcp_ny_swap_state {
	uint8_t new_role;
};

struct tcp_ny_enable_state {
	bool en;
};

struct tcp_ny_typec_state {
	uint8_t rp_level;
	uint8_t polarity;
	uint8_t old_state;
	uint8_t new_state;
};

enum {
	TCP_VBUS_CTRL_REMOVE = 0,
	TCP_VBUS_CTRL_TYPEC = 1,
	TCP_VBUS_CTRL_PD = 2,

	TCP_VBUS_CTRL_HRESET = TCP_VBUS_CTRL_PD,
	TCP_VBUS_CTRL_PR_SWAP = 3,
	TCP_VBUS_CTRL_REQUEST = 4,
	TCP_VBUS_CTRL_STANDBY = 5,
	TCP_VBUS_CTRL_STANDBY_UP = 6,
	TCP_VBUS_CTRL_STANDBY_DOWN = 7,

	TCP_VBUS_CTRL_PD_DETECT = (1 << 7),

	TCP_VBUS_CTRL_PD_HRESET =
		TCP_VBUS_CTRL_HRESET | TCP_VBUS_CTRL_PD_DETECT,

	TCP_VBUS_CTRL_PD_PR_SWAP =
		TCP_VBUS_CTRL_PR_SWAP | TCP_VBUS_CTRL_PD_DETECT,

	TCP_VBUS_CTRL_PD_REQUEST =
		TCP_VBUS_CTRL_REQUEST | TCP_VBUS_CTRL_PD_DETECT,

	TCP_VBUS_CTRL_PD_STANDBY =
		TCP_VBUS_CTRL_STANDBY | TCP_VBUS_CTRL_PD_DETECT,

	TCP_VBUS_CTRL_PD_STANDBY_UP =
		TCP_VBUS_CTRL_STANDBY_UP | TCP_VBUS_CTRL_PD_DETECT,

	TCP_VBUS_CTRL_PD_STANDBY_DOWN =
		TCP_VBUS_CTRL_STANDBY_DOWN | TCP_VBUS_CTRL_PD_DETECT,
};

struct tcp_ny_vbus_state {
	int mv;
	int ma;
	uint8_t type;
};

struct tcp_ny_mode_ctrl {
	uint16_t svid;
	uint8_t ops;
	uint32_t mode;
};

enum {
	SW_USB = 0,
	SW_DFP_D,
	SW_UFP_D,
};

struct tcp_ny_ama_dp_state {
	uint8_t sel_config;
	uint8_t signal;
	uint8_t pin_assignment;
	uint8_t polarity;
	uint8_t active;
};

enum {
	TCP_DP_UFP_U_MASK = 0x7C,
	TCP_DP_UFP_U_POWER_LOW = 1 << 2,
	TCP_DP_UFP_U_ENABLED = 1 << 3,
	TCP_DP_UFP_U_MF_PREFER = 1 << 4,
	TCP_DP_UFP_U_USB_CONFIG = 1 << 5,
	TCP_DP_UFP_U_EXIT_MODE = 1 << 6,
};

struct tcp_ny_ama_dp_attention {
	uint8_t state;
};

struct tcp_ny_ama_dp_hpd_state {
	bool irq:1;
	bool state:1;
};

struct tcp_ny_uvdm {
	bool ack;
	uint8_t uvdm_cnt;
	uint16_t uvdm_svid;
	uint32_t *uvdm_data;
};

/*
 * TCP_NOTIFY_HARD_RESET_STATE
 *
 * Please don't expect that every signal will have a corresponding result.
 * The signal can be generated multiple times before receiving a result.
 */

enum {
	/* HardReset finished because recv GoodCRC or TYPE-C only */
	TCP_HRESET_RESULT_DONE = 0,

	/* HardReset failed because detach or error recovery */
	TCP_HRESET_RESULT_FAIL,

	/* HardReset signal from Local Policy Engine */
	TCP_HRESET_SIGNAL_SEND,

	/* HardReset signal from Port Partner */
	TCP_HRESET_SIGNAL_RECV,
};

struct tcp_ny_hard_reset_state {
	uint8_t state;
};

struct tcp_notify {
	union {
		struct tcp_ny_enable_state en_state;
		struct tcp_ny_vbus_state vbus_state;
		struct tcp_ny_typec_state typec_state;
		struct tcp_ny_swap_state swap_state;
		struct tcp_ny_pd_state pd_state;
		struct tcp_ny_mode_ctrl mode_ctrl;
		struct tcp_ny_ama_dp_state ama_dp_state;
		struct tcp_ny_ama_dp_attention ama_dp_attention;
		struct tcp_ny_ama_dp_hpd_state ama_dp_hpd_state;
		struct tcp_ny_uvdm uvdm_msg;
		struct tcp_ny_hard_reset_state hreset_state;
	};
};

#ifdef CONFIG_TCPC_CLASS
extern struct tcpc_device
		*tcpc_dev_get_by_name(const char *name);
#else
static inline struct tcpc_device
		*tcpc_dev_get_by_name(const char *name)
{
	return NULL;
}
#endif

extern int register_tcp_dev_notifier(struct tcpc_device *tcp_dev,
				     struct notifier_block *nb);
extern int unregister_tcp_dev_notifier(struct tcpc_device *tcp_dev,
				       struct notifier_block *nb);

/*
 * Type-C Port Control I/F
 */

enum tcpm_error_list {
	TCPM_SUCCESS = 0,
	TCPM_ERROR_UNKNOWN = -1,
	TCPM_ERROR_UNATTACHED = -2,
	TCPM_ERROR_PARAMETER = -3,
	TCPM_ERROR_PUT_EVENT = -4,
	TCPM_ERROR_NO_SUPPORT = -5,
	TCPM_ERROR_NO_PD_CONNECTED = -6,
	TCPM_ERROR_NO_POWER_CABLE = -7,
	TCPM_ERROR_NO_PARTNER_INFORM = -8,
	TCPM_ERROR_NO_SOURCE_CAP = -9,
	TCPM_ERROR_NO_SINK_CAP = -10,
	TCPM_ERROR_NOT_DRP_ROLE = -11,
	TCPM_ERROR_DURING_ROLE_SWAP = -12,
	TCPM_ERROR_NO_EXPLICIT_CONTRACT = -13,
	TCPM_ERROR_ERROR_RECOVERY = -14,
	TCPM_ERROR_NOT_FOUND = -15,
	TCPM_ERROR_CHARGING_POLICY = -16,
};

#define TCPM_PDO_MAX_SIZE	7

struct tcpm_remote_power_cap {
	uint8_t selected_cap_idx;
	uint8_t nr;
	int max_mv[TCPM_PDO_MAX_SIZE];
	int min_mv[TCPM_PDO_MAX_SIZE];
	int ma[TCPM_PDO_MAX_SIZE];
};

/* Inquire TCPM status */

enum tcpc_cc_voltage_status {
	TYPEC_CC_VOLT_OPEN = 0,
	TYPEC_CC_VOLT_RA = 1,
	TYPEC_CC_VOLT_RD = 2,

	TYPEC_CC_VOLT_SNK_DFT = 5,
	TYPEC_CC_VOLT_SNK_1_5 = 6,
	TYPEC_CC_VOLT_SNK_3_0 = 7,

	TYPEC_CC_DRP_TOGGLING = 15,
};

enum tcpm_vbus_level {
#ifdef CONFIG_TCPC_VSAFE0V_DETECT
	TCPC_VBUS_SAFE0V = 0,		/* < 0.8V */
	TCPC_VBUS_INVALID,		/* > 0.8V */
	TCPC_VBUS_VALID,		/* > 4.5V */
#else
	TCPC_VBUS_INVALID = 0,
	TCPC_VBUS_VALID,
#endif
};

enum typec_role_defination {
	TYPEC_ROLE_UNKNOWN = 0,
	TYPEC_ROLE_SNK,
	TYPEC_ROLE_SRC,
	TYPEC_ROLE_DRP,
	TYPEC_ROLE_TRY_SRC,
	TYPEC_ROLE_TRY_SNK,
	TYPEC_ROLE_NR,
};

enum pd_cable_current_limit {
	PD_CABLE_CURR_UNKNOWN = 0,
	PD_CABLE_CURR_1A5 = 1,
	PD_CABLE_CURR_3A = 2,
	PD_CABLE_CURR_5A = 3,
};

/* DPM Flags */

#define DPM_FLAGS_PARTNER_DR_POWER		(1<<0)
#define DPM_FLAGS_PARTNER_DR_DATA		(1<<1)
#define DPM_FLAGS_PARTNER_EXTPOWER		(1<<2)
#define DPM_FLAGS_PARTNER_USB_COMM		(1<<3)
#define DPM_FLAGS_PARTNER_USB_SUSPEND	(1<<4)
#define DPM_FLAGS_PARTNER_HIGH_CAP		(1<<5)

#define DPM_FLAGS_PARTNER_MISMATCH		(1<<7)
#define DPM_FLAGS_PARTNER_GIVE_BACK		(1<<8)
#define DPM_FLAGS_PARTNER_NO_SUSPEND	(1<<9)

#define DPM_FLAGS_RESET_PARTNER_MASK	\
	(DPM_FLAGS_PARTNER_DR_POWER | DPM_FLAGS_PARTNER_DR_DATA|\
	DPM_FLAGS_PARTNER_EXTPOWER | DPM_FLAGS_PARTNER_USB_COMM)

#define DPM_FLAGS_CHECK_DC_MODE			(1<<20)
#define DPM_FLAGS_CHECK_UFP_SVID		(1<<21)
#define DPM_FLAGS_CHECK_EXT_POWER		(1<<22)
#define DPM_FLAGS_CHECK_DP_MODE			(1<<23)
#define DPM_FLAGS_CHECK_SINK_CAP		(1<<24)
#define DPM_FLAGS_CHECK_SOURCE_CAP		(1<<25)
#define DPM_FLAGS_CHECK_UFP_ID			(1<<26)
#define DPM_FLAGS_CHECK_CABLE_ID		(1<<27)
#define DPM_FLAGS_CHECK_CABLE_ID_DFP		(1<<28)
#define DPM_FLAGS_CHECK_PR_ROLE			(1<<29)
#define DPM_FLAGS_CHECK_DR_ROLE			(1<<30)

/* DPM_CAPS */

#define DPM_CAP_LOCAL_DR_POWER			(1<<0)
#define DPM_CAP_LOCAL_DR_DATA			(1<<1)
#define DPM_CAP_LOCAL_EXT_POWER			(1<<2)
#define DPM_CAP_LOCAL_USB_COMM			(1<<3)
#define DPM_CAP_LOCAL_USB_SUSPEND		(1<<4)
#define DPM_CAP_LOCAL_HIGH_CAP			(1<<5)
#define DPM_CAP_LOCAL_GIVE_BACK			(1<<6)
#define DPM_CAP_LOCAL_NO_SUSPEND		(1<<7)
#define DPM_CAP_LOCAL_VCONN_SUPPLY		(1<<8)

#define DPM_CAP_ATTEMP_ENTER_DC_MODE		(1<<11)
#define DPM_CAP_ATTEMP_DISCOVER_CABLE_DFP	(1<<12)
#define DPM_CAP_ATTEMP_ENTER_DP_MODE		(1<<13)
#define DPM_CAP_ATTEMP_DISCOVER_CABLE		(1<<14)
#define DPM_CAP_ATTEMP_DISCOVER_ID		(1<<15)

enum dpm_cap_pr_check_prefer {
	DPM_CAP_PR_CHECK_DISABLE = 0,
	DPM_CAP_PR_CHECK_PREFER_SNK = 1,
	DPM_CAP_PR_CHECK_PREFER_SRC = 2,
};

#define DPM_CAP_PR_CHECK_PROP(cap)			((cap & 0x03) << 16)
#define DPM_CAP_EXTRACT_PR_CHECK(raw)		((raw >> 16) & 0x03)
#define DPM_CAP_PR_SWAP_REJECT_AS_SRC		(1<<18)
#define DPM_CAP_PR_SWAP_REJECT_AS_SNK		(1<<19)
#define DPM_CAP_PR_SWAP_CHECK_GP_SRC		(1<<20)
#define DPM_CAP_PR_SWAP_CHECK_GP_SNK		(1<<21)
#define DPM_CAP_PR_SWAP_CHECK_GOOD_POWER	\
	(DPM_CAP_PR_SWAP_CHECK_GP_SRC | DPM_CAP_PR_SWAP_CHECK_GP_SNK)

enum dpm_cap_dr_check_prefer {
	DPM_CAP_DR_CHECK_DISABLE = 0,
	DPM_CAP_DR_CHECK_PREFER_UFP = 1,
	DPM_CAP_DR_CHECK_PREFER_DFP = 2,
};

#define DPM_CAP_DR_CHECK_PROP(cap)		((cap & 0x03) << 22)
#define DPM_CAP_EXTRACT_DR_CHECK(raw)		((raw >> 22) & 0x03)
#define DPM_CAP_DR_SWAP_REJECT_AS_DFP		(1<<24)
#define DPM_CAP_DR_SWAP_REJECT_AS_UFP		(1<<25)

#define DPM_CAP_DP_PREFER_MF				(1<<29)

extern int tcpm_shutdown(struct tcpc_device *tcpc_dev);

extern int tcpm_inquire_remote_cc(struct tcpc_device *tcpc_dev,
	uint8_t *cc1, uint8_t *cc2, bool from_ic);
extern int tcpm_inquire_vbus_level(struct tcpc_device *tcpc_dev, bool from_ic);
extern bool tcpm_inquire_cc_polarity(struct tcpc_device *tcpc_dev);
extern uint8_t tcpm_inquire_typec_attach_state(struct tcpc_device *tcpc_dev);
extern uint8_t tcpm_inquire_typec_role(struct tcpc_device *tcpc_dev);
extern uint8_t tcpm_inquire_typec_local_rp(struct tcpc_device *tcpc_dev);

extern int tcpm_typec_set_wake_lock(
	struct tcpc_device *tcpc, bool user_lock);

extern int tcpm_typec_set_usb_sink_curr(
	struct tcpc_device *tcpc_dev, int curr);

extern int tcpm_typec_set_rp_level(
	struct tcpc_device *tcpc_dev, uint8_t level);

extern int tcpm_typec_set_custom_hv(
	struct tcpc_device *tcpc_dev, bool en);

extern int tcpm_typec_role_swap(
	struct tcpc_device *tcpc_dev);

extern int tcpm_typec_change_role(
	struct tcpc_device *tcpc_dev, uint8_t typec_role);

#ifdef CONFIG_USB_POWER_DELIVERY

/* --- PD data message helpers --- */

#define PD_DATA_OBJ_SIZE		(7)
#define PDO_MAX_NR				(PD_DATA_OBJ_SIZE)
#define VDO_MAX_NR				(PD_DATA_OBJ_SIZE-1)
#define VDO_MAX_SVID_NR			(VDO_MAX_NR*2)

#define VDO_DISCOVER_ID_IDH			0
#define VDO_DISCOVER_ID_CSTAT		1
#define VDO_DISCOVER_ID_PRODUCT		2
#define VDO_DISCOVER_ID_CABLE		3
#define VDO_DISCOVER_ID_AMA			3

struct tcpm_power_cap {
	uint8_t cnt;
	uint32_t pdos[PDO_MAX_NR];
};

extern bool tcpm_inquire_pd_connected(
	struct tcpc_device *tcpc_dev);

extern bool tcpm_inquire_pd_prev_connected(
	struct tcpc_device *tcpc_dev);

extern uint8_t tcpm_inquire_pd_data_role(
	struct tcpc_device *tcpc_dev);

extern uint8_t tcpm_inquire_pd_power_role(
	struct tcpc_device *tcpc_dev);

extern uint8_t tcpm_inquire_pd_vconn_role(
	struct tcpc_device *tcpc_dev);

extern uint8_t tcpm_inquire_pd_pe_ready(
	struct tcpc_device *tcpc_dev);

extern uint8_t tcpm_inquire_cable_current(
	struct tcpc_device *tcpc_dev);

extern uint32_t tcpm_inquire_dpm_flags(
	struct tcpc_device *tcpc_dev);

extern uint32_t tcpm_inquire_dpm_caps(
	struct tcpc_device *tcpc_dev);

extern void tcpm_set_dpm_flags(
	struct tcpc_device *tcpc_dev, uint32_t flags);

extern void tcpm_set_dpm_caps(
	struct tcpc_device *tcpc_dev, uint32_t caps);

#endif	/* CONFIG_USB_POWER_DELIVERY */


#ifdef CONFIG_USB_POWER_DELIVERY

/* Request TCPM to send PD Request */

struct tcp_dpm_event;

enum {
	TCP_DPM_RET_SUCCESS = 0,
	TCP_DPM_RET_SENT = 0,
	TCP_DPM_RET_VDM_ACK = 0,

	TCP_DPM_RET_DENIED_UNKNOWM,
	TCP_DPM_RET_DENIED_NOT_READY,
	TCP_DPM_RET_DENIED_NO_SUPPORT,
	TCP_DPM_RET_DENIED_LOCAL_CAP,
	TCP_DPM_RET_DENIED_PARTNER_CAP,
	TCP_DPM_RET_DENIED_SAME_ROLE,
	TCP_DPM_RET_DENIED_INVALID_REQUEST,
	TCP_DPM_RET_DENIED_WRONG_DATA_ROLE,

	TCP_DPM_RET_DROP_CC_DETACH,
	TCP_DPM_RET_DROP_SENT_SRESET,
	TCP_DPM_RET_DROP_RECV_SRESET,
	TCP_DPM_RET_DROP_SENT_HRESET,
	TCP_DPM_RET_DROP_RECV_HRESET,
	TCP_DPM_RET_DROP_ERROR_REOCVERY,
	TCP_DPM_RET_DROP_SEND_BIST,

	TCP_DPM_RET_WAIT,
	TCP_DPM_RET_REJECT,
	TCP_DPM_RET_TIMEOUT,

	TCP_DPM_RET_VDM_NAK,

	TCP_DPM_RET_BK_TIMEOUT,
};

enum {
	TCP_DPM_EVT_UNKONW = 0,

	TCP_DPM_EVT_PD_COMMAND,

	TCP_DPM_EVT_PR_SWAP_AS_SNK = TCP_DPM_EVT_PD_COMMAND,
	TCP_DPM_EVT_PR_SWAP_AS_SRC,
	TCP_DPM_EVT_DR_SWAP_AS_UFP,
	TCP_DPM_EVT_DR_SWAP_AS_DFP,
	TCP_DPM_EVT_VCONN_SWAP_OFF,
	TCP_DPM_EVT_VCONN_SWAP_ON,
	TCP_DPM_EVT_GOTOMIN,

	TCP_DPM_EVT_SOFTRESET,
	TCP_DPM_EVT_CABLE_SOFTRESET,

	TCP_DPM_EVT_GET_SOURCE_CAP,
	TCP_DPM_EVT_GET_SINK_CAP,

	TCP_DPM_EVT_REQUEST,
	TCP_DPM_EVT_REQUEST_EX,
	TCP_DPM_EVT_REQUEST_AGAIN,
	TCP_DPM_EVT_BIST_CM2,

#ifdef CONFIG_USB_PD_REV30
	TCP_DPM_EVT_GET_SOURCE_CAP_EXT,
	TCP_DPM_EVT_GET_STATUS,
	TCP_DPM_EVT_FR_SWAP_AS_SINK,
	TCP_DPM_EVT_FR_SWAP_AS_SOURCE,
	TCP_DPM_EVT_GET_PPS_STATUS,
#endif	/* CONFIG_USB_PD_REV30 */

	TCP_DPM_EVT_VDM_COMMAND,
	TCP_DPM_EVT_DISCOVER_CABLE = TCP_DPM_EVT_VDM_COMMAND,
	TCP_DPM_EVT_DISCOVER_ID,
	TCP_DPM_EVT_DISCOVER_SVIDS,
	TCP_DPM_EVT_DISCOVER_MODES,
	TCP_DPM_EVT_ENTER_MODE,
	TCP_DPM_EVT_EXIT_MODE,
	TCP_DPM_EVT_ATTENTION,

#ifdef CONFIG_USB_PD_ALT_MODE
	TCP_DPM_EVT_DP_ATTENTION,
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
	TCP_DPM_EVT_DP_STATUS_UPDATE,
	TCP_DPM_EVT_DP_CONFIG,
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
	TCP_DPM_EVT_UVDM,
#endif	/* CONFIG_USB_PD_UVDM */

	TCP_DPM_EVT_IMMEDIATELY,
	TCP_DPM_EVT_HARD_RESET = TCP_DPM_EVT_IMMEDIATELY,
	TCP_DPM_EVT_ERROR_RECOVERY,

	TCP_DPM_EVT_NR,
};

typedef int (*tcp_dpm_event_cb)(
	struct tcpc_device *tcpc, int ret, struct tcp_dpm_event *event);

struct tcp_dpm_new_role {
	uint8_t new_role;
};

struct tcp_dpm_pd_request {
	int mv;
	int ma;
};

struct tcp_dpm_pd_request_ex {
	uint8_t pos;

	union {
		uint32_t max;
		uint32_t max_uw;
		uint32_t max_ma;
	};

	union {
		uint32_t oper;
		uint32_t oper_uw;
		uint32_t oper_ma;
	};
};

struct tcp_dpm_svdm_data {
	uint16_t svid;
	uint8_t ops;
};

#ifdef CONFIG_USB_PD_ALT_MODE
struct tcp_dpm_dp_data {
	uint32_t val;
	uint32_t mask;
};
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
struct tcp_dpm_uvdm_data {
	bool wait_resp;
	uint8_t cnt;
	uint32_t vdos[PD_DATA_OBJ_SIZE];
};
#endif	/* CONFIG_USB_PD_UVDM */

struct tcp_dpm_event {
	uint8_t event_id;
	tcp_dpm_event_cb event_cb;

	union {
		struct tcp_dpm_pd_request pd_req;
		struct tcp_dpm_pd_request_ex pd_req_ex;

#ifdef CONFIG_USB_PD_ALT_MODE
		struct tcp_dpm_dp_data dp_data;
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
		struct tcp_dpm_uvdm_data uvdm_data;
#endif	/* CONFIG_USB_PD_UVDM */

		struct tcp_dpm_svdm_data svdm_data;
	} tcp_dpm_data;
};

int tcpm_put_tcp_dpm_event(
	struct tcpc_device *tcpc, struct tcp_dpm_event *event);

/* TCPM DPM PD I/F */

extern int tcpm_inquire_pd_contract(
	struct tcpc_device *tcpc, int *mv, int *ma);
extern int tcpm_inquire_cable_inform(
	struct tcpc_device *tcpc, uint32_t *vdos);
extern int tcpm_inquire_pd_partner_inform(
	struct tcpc_device *tcpc, uint32_t *vdos);
extern int tcpm_inquire_pd_source_cap(
	struct tcpc_device *tcpc, struct tcpm_power_cap *cap);
extern int tcpm_inquire_pd_sink_cap(
	struct tcpc_device *tcpc, struct tcpm_power_cap *cap);

enum tcpm_power_cap_val_type {
	TCPM_POWER_CAP_VAL_TYPE_FIXED = 0,
	TCPM_POWER_CAP_VAL_TYPE_BATTERY = 1,
	TCPM_POWER_CAP_VAL_TYPE_VARIABLE = 2,
	TCPM_POWER_CAP_VAL_TYPE_AUGMENT = 3,

	TCPM_POWER_CAP_VAL_TYPE_UNKNOWN = 0xff,
};

#define TCPM_APDO_TYPE_MASK	(0x0f)

enum tcpm_power_cap_apdo_type {
	TCPM_POWER_CAP_APDO_TYPE_PPS = 1 << 0,

	TCPM_POWER_CAP_APDO_TYPE_PPS_CF = (1 << 7),
};

struct tcpm_power_cap_val {
	uint8_t type;
#ifdef CONFIG_USB_PD_REV30
	uint8_t apdo_type;
#endif /* CONFIG_USB_PD_REV30 */
	int max_mv;
	int min_mv;
	union {
		int uw;
		int ma;
	};
};

extern bool tcpm_extract_power_cap_val(
	uint32_t pdo, struct tcpm_power_cap_val *cap);

/* Request TCPM to send PD Request */

extern int tcpm_dpm_pd_power_swap(
	struct tcpc_device *tcpc, uint8_t role, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_data_swap(
	struct tcpc_device *tcpc, uint8_t role, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_vconn_swap(
	struct tcpc_device *tcpc, uint8_t role, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_goto_min(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_soft_reset(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_get_source_cap(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_get_sink_cap(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_request(struct tcpc_device *tcpc,
	int mv, int ma, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_request_ex(struct tcpc_device *tcpc,
	uint8_t pos, uint32_t max, uint32_t oper, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_bist_cm2(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);

#ifdef CONFIG_USB_PD_REV30

#define TCPM_PPS_GET_OUTPUT_MV(raw)		(raw*20)
#define TCPM_PPS_GET_OUTPUT_MA(raw)		(raw*50)

#define TCPM_PPS_SET_OUTPUT_MV(mv)		(((mv) / 20) & 0xFFFF)
#define TCPM_PPS_SET_OUTPUT_MA(ma)		(((ma) / 50) & 0xFF)

#define TCPM_PPS_FLAGS_CFF	(1 << 3)
#define TCPM_PPS_FLGAS_PTF(raw)	((raw & 0x06) >> 1)

#define TCPM_PPS_FLAGS_SET_PTF(flags)	((flags & 0x03) << 1)

enum tcpm_pps_present_temp_flag {
	TCPM_PPS_PTF_NO_SUPPORT = 0,
	TCPM_PPS_PTF_NORMAL,
	TCPM_PPS_PTF_WARNING,
	TCPM_PPS_PTF_OVER_TEMP,
};

struct tcpm_pps_status {
	uint16_t output_mv;	/* 0xffff means no support */
	uint8_t output_ma;	/* 0xff means no support */
	uint8_t real_time_flags;
};

#define TCPM_STASUS_EVENT_OCP	(1<<1)
#define TCPM_STATUS_EVENT_OTP	(1<<2)
#define TCPM_STATUS_EVENT_OVP	(1<<3)

#define TCPM_STATUS_TEMP_PTF(raw)	((raw & 0x03) >> 1)
#define TCPM_STATUS_TEMP_SET_PTF(val)		((val & 0x03) << 1)

struct tcpm_status_data {
	uint8_t internal_temp;	/* 0 means no support */
	uint8_t present_input;	/* bit filed */
	uint8_t present_battey_input; /* bit filed */
	uint8_t event_flags;	/* bit filed */
	uint8_t temperature;	/* bit filed */
};

extern int tcpm_dpm_pd_get_source_cap_ext(
		struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_fast_swap(
	struct tcpc_device *tcpc, uint8_t role, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_get_status(
		struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_pd_get_pps_status(
		struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
#endif	/* CONFIG_USB_PD_REV30 */

extern int tcpm_dpm_pd_hard_reset(struct tcpc_device *tcpc);
extern int tcpm_dpm_pd_error_recovery(struct tcpc_device *tcpc);

/* Request TCPM to send VDM */

extern int tcpm_dpm_vdm_discover_cable(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_vdm_discover_id(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_vdm_discover_svid(
	struct tcpc_device *tcpc, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_vdm_discover_mode(
	struct tcpc_device *tcpc, uint16_t svid, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_vdm_enter_mode(struct tcpc_device *tcpc,
	uint16_t svid, uint8_t ops, tcp_dpm_event_cb event_cb);
extern int tcpm_dpm_vdm_exit_mode(struct tcpc_device *tcpc,
	uint16_t svid, uint8_t ops, tcp_dpm_event_cb event_cb);

/* Request TCPM to send PD-DP Request */

#ifdef CONFIG_USB_PD_ALT_MODE

enum pd_dp_ufp_u_state {
	DP_UFP_U_NONE = 0,
	DP_UFP_U_STARTUP,
	DP_UFP_U_WAIT,
	DP_UFP_U_OPERATION,
	DP_UFP_U_STATE_NR,

	DP_UFP_U_ERR = 0X10,
};

extern int tcpm_inquire_dp_ufp_u_state(
	struct tcpc_device *tcpc, uint8_t *state);

extern int tcpm_dpm_dp_attention(struct tcpc_device *tcpc,
	uint32_t dp_status, uint32_t mask, tcp_dpm_event_cb event_cb);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP

enum pd_dp_dfp_u_state {
	DP_DFP_U_NONE = 0,
	DP_DFP_U_DISCOVER_ID,
	DP_DFP_U_DISCOVER_SVIDS,
	DP_DFP_U_DISCOVER_MODES,
	DP_DFP_U_ENTER_MODE,
	DP_DFP_U_STATUS_UPDATE,
	DP_DFP_U_WAIT_ATTENTION,
	DP_DFP_U_CONFIGURE,
	DP_DFP_U_OPERATION,
	DP_DFP_U_STATE_NR,

	DP_DFP_U_ERR = 0X10,

	DP_DFP_U_ERR_DISCOVER_ID_TYPE,
	DP_DFP_U_ERR_DISCOVER_ID_NAK_TIMEOUT,

	DP_DFP_U_ERR_DISCOVER_SVID_DP_SID,
	DP_DFP_U_ERR_DISCOVER_SVID_NAK_TIMEOUT,

	DP_DFP_U_ERR_DISCOVER_MODE_DP_SID,
	DP_DFP_U_ERR_DISCOVER_MODE_CAP,	/* NO SUPPORT UFP-D */
	DP_DFP_U_ERR_DISCOVER_MODE_NAK_TIMEROUT,

	DP_DFP_U_ERR_ENTER_MODE_DP_SID,
	DP_DFP_U_ERR_ENTER_MODE_NAK_TIMEOUT,

	DP_DFP_U_ERR_EXIT_MODE_DP_SID,
	DP_DFP_U_ERR_EXIT_MODE_NAK_TIMEOUT,

	DP_DFP_U_ERR_STATUS_UPDATE_DP_SID,
	DP_DFP_U_ERR_STATUS_UPDATE_NAK_TIMEOUT,
	DP_DFP_U_ERR_STATUS_UPDATE_ROLE,

	DP_DFP_U_ERR_CONFIGURE_SELECT_MODE,
};

extern int tcpm_inquire_dp_dfp_u_state(
	struct tcpc_device *tcpc, uint8_t *state);

extern int tcpm_dpm_dp_status_update(struct tcpc_device *tcpc,
	uint32_t dp_status, uint32_t mask, tcp_dpm_event_cb event_cb);

extern int tcpm_dpm_dp_config(struct tcpc_device *tcpc,
	uint32_t dp_config, uint32_t mask, tcp_dpm_event_cb event_cb);
#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

/* Request TCPM to send PD-UVDM Request */

#ifdef CONFIG_USB_PD_UVDM

#define PD_UVDM_HDR(vid, custom)	\
	(((vid) << 16) | ((custom) & 0x7FFF))

#define PD_UVDM_HDR_CMD(hdr)	\
	(hdr & 0x7FFF)

extern int tcpm_dpm_send_uvdm(
	struct tcpc_device *tcpc, uint8_t cnt,
	uint32_t *data, bool wait_resp, tcp_dpm_event_cb event_cb);

#endif	/* CONFIG_USB_PD_UVDM */

/* Notify TCPM */

extern int tcpm_notify_vbus_stable(struct tcpc_device *tcpc_dev);

/* mtk only start */
extern void tcpm_set_uvdm_handle_flag(struct tcpc_device *tcpc_dev, unsigned char en);
extern bool tcpm_get_uvdm_handle_flag(struct tcpc_device *tcpc_dev);
/* mtk only end */


/* capability
 * CABLE_CURR_UNKNOWN = 0
 * CABLE_CURR_1_5A = 1
 * CABLE_CURR_3A = 2
 * CABLE_CURR_5A = 3
 */
#if defined(CONFIG_MTK_GAUGE_VERSION) && CONFIG_MTK_GAUGE_VERSION == 30
extern int tcpm_get_cable_capability(void *tcpc_dev, unsigned char *capability);
#else
extern int tcpm_get_cable_capability(struct tcpc_device *tcpc_dev,
					unsigned char *capability);
#endif

#ifdef CONFIG_RT7207_ADAPTER
#if defined(CONFIG_MTK_GAUGE_VERSION) && CONFIG_MTK_GAUGE_VERSION == 30
extern int tcpm_set_direct_charge_en(void *tcpc_dev, bool en);
#else
extern int tcpm_set_direct_charge_en(struct tcpc_device *tcpc_dev, bool en);
#endif

enum {
	PE30_TA_UNKNOWN,
	PE30_TA_NOT_SUPPORT,
	PE30_TA_SUPPORT,
};
extern int tcpm_get_pe30_ta_sup(struct tcpc_device *tcpc);
extern void tcpm_reset_pe30_ta(struct tcpc_device *tcpc);
#else
#if defined(CONFIG_MTK_GAUGE_VERSION) && CONFIG_MTK_GAUGE_VERSION == 20
static inline int tcpm_set_direct_charge_en(void *tcpc, bool en)
{
	return -1;
}
#endif
#endif /* CONFIG_RT7207_ADAPTER */

#ifdef CONFIG_MTK_SMART_BATTERY
extern void wake_up_bat3(void);
#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT
extern void chrdet_int_handler(void);
#endif /* CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT */
#endif /* CONFIG_MTK_SMART_BATTERY */

/* Charging Policy: Select PDO */

#define DPM_CHARGING_POLICY_MASK	(0x0f)

enum dpm_charging_policy {
	/* VSafe5V only */
	DPM_CHARGING_POLICY_VSAFE5V = 0,

	/* Max Power */
	DPM_CHARGING_POLICY_MAX_POWER = 1,

	/* Custom defined Policy */
	DPM_CHARGING_POLICY_CUSTOM = 2,

	/*  Runtime Policy, restore to default after plug-out or hard-reset */
	DPM_CHARGING_POLICY_RUNTIME = 3,

	/* Direct charge <Variable PDO only> */
	DPM_CHARGING_POLICY_DIRECT_CHARGE = 3,

	/* PPS <Augmented PDO only> */
	DPM_CHARGING_POLICY_PPS = 4,

	/* Default Charging Policy <from DTS>*/
	DPM_CHARGING_POLICY_DEFAULT = 0xff,

	DPM_CHARGING_POLICY_IGNORE_MISMATCH_CURR = 1 << 4,
	DPM_CHARGING_POLICY_PREFER_LOW_VOLTAGE = 1 << 5,
	DPM_CHARGING_POLICY_PREFER_HIGH_VOLTAGE = 1 << 6,

	DPM_CHARGING_POLICY_MAX_POWER_LV =
		DPM_CHARGING_POLICY_MAX_POWER |
		DPM_CHARGING_POLICY_PREFER_LOW_VOLTAGE,
	DPM_CHARGING_POLICY_MAX_POWER_LVIC =
		DPM_CHARGING_POLICY_MAX_POWER_LV |
		DPM_CHARGING_POLICY_IGNORE_MISMATCH_CURR,

	DPM_CHARGING_POLICY_MAX_POWER_HV =
		DPM_CHARGING_POLICY_MAX_POWER |
		DPM_CHARGING_POLICY_PREFER_HIGH_VOLTAGE,
	DPM_CHARGING_POLICY_MAX_POWER_HVIC =
		DPM_CHARGING_POLICY_MAX_POWER_HV |
		DPM_CHARGING_POLICY_IGNORE_MISMATCH_CURR,
};

extern int tcpm_set_pd_charging_policy(
	struct tcpc_device *tcpc, uint8_t policy);

extern uint8_t tcpm_inquire_pd_charging_policy(struct tcpc_device *tcpc);

#ifdef CONFIG_USB_PD_DIRECT_CHARGE
extern bool tcpm_inquire_during_direct_charge(struct tcpc_device *tcpc);
#endif	/* CONFIG_USB_PD_DIRECT_CHARGE */


#ifdef CONFIG_TCPC_VCONN_SUPPLY_MODE
extern int tcpm_dpm_set_vconn_supply_mode(
	struct tcpc_device *tcpc, uint8_t mode);
#endif	/* CONFIG_TCPC_VCONN_SUPPLY_MODE */


#ifdef CONFIG_USB_PD_REV30_PPS_SINK

#define TCPM_PD_SDB_SIZE	5
#define TCPM_PD_PPSDB_SIZE	4

extern int tcpm_set_apdo_charging_policy(
	struct tcpc_device *tcpc, uint8_t policy, int mv, int ma);

extern int tcpm_inquire_pd_apdo_boundary(
	struct tcpc_device *tcpc, struct tcpm_power_cap_val *cap_val);

extern int tcpm_inquire_pd_source_apdo(struct tcpc_device *tcpc,
	uint8_t apdo_type, uint8_t *cap_i, struct tcpm_power_cap_val *cap);

#endif	/* CONFIG_USB_PD_REV30_PPS_SINK */

extern int tcpm_get_remote_power_cap(struct tcpc_device *tcpc_dev,
					struct tcpm_remote_power_cap *cap);
extern int tcpm_set_remote_power_cap(struct tcpc_device *tcpc,
					int mv, int ma);

#else	/* CONFIG_USB_POWER_DELIVERY */

static inline bool tcpm_inquire_pd_prev_connected(
	struct tcpc_device *tcpc_dev)
{
	return false;
}

static inline int tcpm_get_remote_power_cap(struct tcpc_device *tcpc_dev,
					struct tcpm_remote_power_cap *cap)
{
	return 0;
}

static inline int tcpm_set_remote_power_cap(struct tcpc_device *tcpc,
					int mv, int ma)
{
	return 0;
}

#endif	/* CONFIG_USB_POWER_DELIVERY */

extern bool tcpm_get_boot_check_flag(struct tcpc_device *tcpc);
extern void tcpm_set_boot_check_flag(struct tcpc_device *tcpc, unsigned char en);
extern bool tcpm_get_ta_hw_exist(struct tcpc_device *tcpc);

#endif /* TCPM_H_ */
