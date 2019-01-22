/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __PKT_TRACK_STRUCT_H__
#define __PKT_TRACK_STRUCT_H__

#if defined(CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT)

#include <linux/usb/composite.h>
#include "mtk_gadget.h"

#define IPC_IP_TYPE_IPV4            1
#define IPC_IP_TYPE_IPV6            2

enum pkt_track_mdt_action_e {
	PKT_TRACK_MDT_ACTION_ADD,
	PKT_TRACK_MDT_ACTION_DELETE,
};

enum pkt_track_data_usage_cmd_e {
	MSG_ID_MDT_SET_GLOBAL_ALERT_REQ,
	MSG_ID_MDT_ALERT_GLOBAL_ALERT_IND,
	MSG_ID_MDT_SET_IQUOTA_REQ,
	MSG_ID_MDT_ALERT_IQUOTA_IND,
	MSG_ID_MDT_DEL_IQUOTA_REQ,
};

struct pkt_track_iquota_t {
	struct delayed_work dwork;
	bool is_add;
	struct mdt_data_limitation_t *user_data;
	spinlock_t lock;
};

struct pkt_track_alert_t {
	struct delayed_work dwork;
	struct mdt_data_measurement_t *user_data;
	spinlock_t lock;
};

struct pkt_track_data_t {
	struct work_struct pkt_work;
	struct list_head msg_queue;
	spinlock_t lock;
};

struct pkt_track_msg_t {
	struct list_head list;
	void *data;
	bool is_add_rule;
};

/* MDT rule related commands structs */
struct mdt_ipv4_rule_t {
	u32 src_ip;
	u32 dst_ip;
	u16 src_port;
	u16 dst_port;
	u8 protocol;
	u8 in_netif_id;
	u8 out_netif_id;
	u8 reserved;
	u32 src_nat_ip;
	u32 dst_nat_ip;
	u16 src_nat_port;
	u16 dst_nat_port;
} __packed;

struct mdt_ipv6_rule_t {
	u32 src_ip[4];
	u32 dst_ip[4];
	u16 src_port;
	u16 dst_port;
	u8 protocol;
	u8 in_netif_id;
	u8 out_netif_id;
	u8 reserved;
} __packed;

struct mdt_common_des_t {
	u8 ip_type;
	u8 action;
	u8 rule_id;
	u8 reserved;
} __packed;

struct pkt_track_ilm_common_des_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	struct mdt_common_des_t des;
} __packed;

struct pkt_track_ilm_del_rule_ind_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	u32   ipv4_rule_cnt;
	u32   ipv4_rule_id[128];
	u32   ipv6_rule_cnt;
	u32   ipv6_rule_id[128];
} __packed;

/* Direct Tethering related command structs */
struct pkt_track_ilm_enable_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	ufpm_enable_md_func_req_t req;
} __packed;

struct pkt_track_ilm_disable_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	ufpm_md_fast_path_common_req_t req;
} __packed;

struct pkt_track_ilm_activate_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	ufpm_activate_md_func_req_t req;
} __packed;

struct pkt_track_ilm_deactivate_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	ufpm_md_fast_path_common_req_t req;
} __packed;

struct pkt_track_ilm_common_rsp_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	ufpm_md_fast_path_common_rsp_t rsp;
} __packed;

/* Data Usage related CCCI IPC structs */
#ifdef NETLINK_ENABLE
struct mdt_data_measurement_t {
	u32 trans_id;
	u32 status;		/*unused */
	u64 measure_buffer_size;
	u32 dev_name_index;	/*unused */
};

struct mdt_global_alert_t {
	u32 trans_id;
	u32 status;		/*unused */
	u64 measure_buffer_size;
	u8 mdt_id;
} __packed;

struct pkt_track_ilm_global_alert_req_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	u32 cmd;
	struct mdt_global_alert_t ga;
} __packed;

struct pkt_track_ilm_global_alert_ind_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	u32 cmd;
	struct mdt_global_alert_t ga;
} __packed;

struct mdt_data_limitation_t {
	u32 trans_id;
	u32 status;		/*unused */
	u64 limit_buffer_size;
	u8 dev_name[IFNAMSIZ];
};

struct mdt_iquota_t {
	u32 trans_id;
	u32 status;		/*unused */
	u64 limit_buffer_size;
	u8 mdt_id;
	u8 reserved[3];
} __packed;

struct pkt_track_ilm_iquota_req_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	u32 cmd;
	struct mdt_iquota_t iq;
} __packed;

struct pkt_track_ilm_iquota_ind_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	u32 cmd;
	struct mdt_iquota_t iq;
} __packed;

struct pkt_track_ilm_data_usage_cmd_t {
	u8    ref_count;
	u8    lp_reserved;
	u16   msg_len;
	u32 cmd;
} __packed;
#endif				/* NETLINK_ENABLE */

#endif				/* CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT */

#endif				/* __PKT_TRACK_STRUCT_H__ */
