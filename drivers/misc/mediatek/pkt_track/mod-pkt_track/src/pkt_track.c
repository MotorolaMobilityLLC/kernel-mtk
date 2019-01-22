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

#if defined(CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT)

#include <linux/types.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/sysctl.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/dst.h>

#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/netfilter_bridge.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <mtk_ccci_common.h>

#include "pkt_track.h"
#include "pkt_track_struct.h"
#include "pkt_track_debug.h"
#include "fastpath.h"
#include "dev.h"

#ifdef NETLINK_ENABLE
#include <net/netlink.h>
#include <linux/netfilter_ipv4/ipt_ULOG.h>

static struct sock *netlink_sock_md;	/* used to receive data statistic command from user-space netd. */
static int pt_nl_event = 112;
static u32 pkt_track_ga_trans_id_s;
static u32 pkt_track_iq_trans_id_s;
#define PT_GA_TRANS_ID_GENERATE()       (pkt_track_ga_trans_id_s++)
#define PT_IQ_TRANS_ID_GENERATE()       (pkt_track_iq_trans_id_s++)

enum drt_tethering_cmd_e {
	DRT_TETHERING_CMD_ADD_ALERT = 0,
	DRT_TETHERING_CMD_ADD_QUOTA,
	DRT_TETHERING_CMD_DEL_QUOTA,
};

static struct pkt_track_alert_t pkt_measure;
static struct pkt_track_iquota_t pkt_limit;

#endif

/* set pkt_track log level */
u32 pkt_log_level = K_ALET | K_CRIT | K_ERR | K_WARNIN | K_NOTICE;

module_param(pkt_log_level, int, 0644);
MODULE_PARM_DESC(pkt_log_level, "Debug Print Log Lvl");

enum pkt_track_dt_state_e {
	PKT_TRACK_DT_STATE_UNINIT,
	PKT_TRACK_DT_STATE_INIT,
	PKT_TRACK_DT_STATE_DEACTIVATE,
	PKT_TRACK_DT_STATE_ACTIVATE,
};

enum pkt_track_dt_state_e pkt_track_dt_state_s;

static unsigned int nf_fastpath_in(void *priv,
				   struct sk_buff *skb,
				   const struct nf_hook_state *state)
{
	if (pkt_track_dt_state_s != PKT_TRACK_DT_STATE_ACTIVATE) {
		pkt_printk(K_DEBUG, "%s: pre-routing, pkt_track is in wrong state[%d].\n",
					__func__, pkt_track_dt_state_s);
		return NF_ACCEPT;
	}

	if (unlikely(!state->in || !skb->dev || !skb_mac_header_was_set(skb))) {
		pkt_printk(K_DEBUG, "%s: pre-routing, parameters are invalid, in[%p], skb->dev[%p], skb_mac_header[%d]\n",
					__func__, state->in, skb->dev, skb_mac_header_was_set(skb));
		return NF_ACCEPT;
	}

	if ((state->in->priv_flags & IFF_EBRIDGE) || (state->in->flags & IFF_LOOPBACK)) {
		pkt_printk(K_DEBUG, "%s: pre-routing, flags are invalid, priv_flags[%x], flags[%x].\n",
					__func__, state->in->priv_flags, state->in->flags);
		return NF_ACCEPT;
	}

	if (!fastpath_is_support_dev(state->in->name)) {
		pkt_printk(K_DEBUG, "%s: pre-routing, cannot support device[%s].\n", __func__, state->in->name);
		return NF_ACCEPT;
	}

	fastpath_in_nf(0, skb);
	return NF_ACCEPT;
}

static unsigned int nf_fastpath_out(void *priv,
				   struct sk_buff *skb,
				   const struct nf_hook_state *state)
{
	if (pkt_track_dt_state_s != PKT_TRACK_DT_STATE_ACTIVATE) {
		pkt_printk(K_DEBUG, "%s: post-routing, pkt_track is in wrong state[%d].\n",
					__func__, pkt_track_dt_state_s);
		return NF_ACCEPT;
	}

	if (unlikely(!state->out || !skb->dev || (skb_headroom(skb) < ETH_HLEN))) {
		pkt_printk(K_DEBUG, "%s: post-routing, parameters are invalid, out[%p], skb->dev[%p], skb_headroom[%d]\n",
					__func__, state->out, skb->dev, skb_headroom(skb));
		goto out;
	}

	if ((state->out->priv_flags & IFF_EBRIDGE) || (state->out->flags & IFF_LOOPBACK)) {
		pkt_printk(K_DEBUG, "%s: post-routing, flags are invalid, priv_flags[%x], flags[%x].\n",
					__func__, state->out->priv_flags, state->out->flags);
		return NF_ACCEPT;
	}

	if (!fastpath_is_support_dev(state->out->name)) {
		pkt_printk(K_DEBUG, "%s: post-routing, cannot support device[%s].\n", __func__, state->out->name);
		return NF_ACCEPT;
	}

	fastpath_out_nf(0, skb, state->out);
out:
	return NF_ACCEPT;
}

static struct nf_hook_ops pkt_track_ops[] __read_mostly = {
	{
	 .hook = nf_fastpath_in,
	 .pf = NFPROTO_IPV4,
	 .hooknum = NF_INET_PRE_ROUTING,
	 .priority = NF_IP_PRI_FIRST + 1,
	 },
	{
	 .hook = nf_fastpath_out,
	 .pf = NFPROTO_IPV4,
	 .hooknum = NF_INET_POST_ROUTING,
	 .priority = NF_IP_PRI_LAST,
	 },
	{
	 .hook = nf_fastpath_in,
	 .pf = NFPROTO_IPV6,
	 .hooknum = NF_INET_PRE_ROUTING,
	 .priority = NF_IP6_PRI_FIRST + 1,
	 },
	{
	 .hook = nf_fastpath_out,
	 .pf = NFPROTO_IPV6,
	 .hooknum = NF_INET_POST_ROUTING,
	 .priority = NF_IP6_PRI_LAST,
	 },
	{
	 .hook = nf_fastpath_in,
	 .pf = PF_BRIDGE,
	 .hooknum = NF_BR_PRE_ROUTING,
	 .priority = NF_BR_PRI_FIRST + 1,
	 },
	{
	 .hook = nf_fastpath_out,
	 .pf = PF_BRIDGE,
	 .hooknum = NF_BR_POST_ROUTING,
	 .priority = NF_IP_PRI_LAST,
	 },
};

#ifdef NETLINK_ENABLE
static void pkt_track_nl_set_cmd(struct sk_buff *skb);
#endif

static void pkt_track_mdt_set_global_alert_work(struct work_struct *work)
{
	int md_status;
	unsigned long flags;

	md_status = exec_ccci_kern_func_by_md_id(0, ID_GET_MD_STATE, NULL, 0);

	if (unlikely(md_status == MD_STATE_EXCEPTION)) {
		pkt_printk(K_ERR, "%s: MD exception.\n", __func__);
		return;
	}

	if (md_status == MD_STATE_READY) {
		int ret;
		ipc_ilm_t ilm;
		struct pkt_track_ilm_global_alert_req_t local_para;
		u64 measure_buffer_size;

		spin_lock_irqsave(&pkt_measure.lock, flags);
		measure_buffer_size = pkt_measure.user_data->measure_buffer_size;
		spin_unlock_irqrestore(&pkt_measure.lock, flags);

		pkt_printk(K_NOTICE, "%s: Set global alert value[%lx].\n", __func__,
					(unsigned long int)measure_buffer_size);

		/* Fill local_para_struct */
		memset(&local_para, 0, sizeof(local_para));
		local_para.msg_len = sizeof(local_para);
		local_para.cmd = MSG_ID_MDT_SET_GLOBAL_ALERT_REQ;
		local_para.ga.trans_id = PT_GA_TRANS_ID_GENERATE();
		local_para.ga.measure_buffer_size = measure_buffer_size;

		/* Fill ipc_ilm_t */
		memset(&ilm, 0, sizeof(ilm));

		ilm.src_mod_id = AP_MOD_PKTTRC;
		ilm.dest_mod_id = MD_MOD_MDT;
		ilm.msg_id = IPC_MSG_ID_MDT_DATA_USAGE_CMD;
		ilm.local_para_ptr = (struct local_para *)&local_para;

		ret = ccci_ipc_send_ilm(0, &ilm);
		if (unlikely(ret < 0)) {
			if (unlikely(ret == -EINVAL)) {
				pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return invalid[%d], ilm[%p]!\n",
							__func__, ret, &ilm);
				return;
			}
			pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return error[%d]!\n", __func__, ret);

			/* If MD is not ready or ccci_ipc_send_ilm() failed, then try it again later. */
			schedule_delayed_work(&pkt_measure.dwork, msecs_to_jiffies(PKT_TRACK_POLLING_MD_TIMER));
		}

	} else {
		pkt_printk(K_ERR, "%s: MD is not ready!\n", __func__);
	}
}

static void pkt_track_mdt_iquota_handler_work(struct work_struct *work)
{
	int md_status;
	unsigned long flags;

	md_status = exec_ccci_kern_func_by_md_id(0, ID_GET_MD_STATE, NULL, 0);

	if (unlikely(md_status == MD_STATE_EXCEPTION)) {
		pkt_printk(K_ERR, "%s: MD exception.\n", __func__);
		return;
	}

	if (md_status == MD_STATE_READY) {
		int ret;
		ipc_ilm_t ilm;
		struct pkt_track_ilm_iquota_req_t local_para;
		int mdt_id;
		bool is_add;
		u64 limit_buffer_size;
		u8 dev_name[IFNAMSIZ];

		spin_lock_irqsave(&pkt_limit.lock, flags);

		is_add = pkt_limit.is_add;
		limit_buffer_size = pkt_limit.user_data->limit_buffer_size;
		memcpy(dev_name, pkt_limit.user_data->dev_name, sizeof(dev_name));

		spin_unlock_irqrestore(&pkt_limit.lock, flags);

		mdt_id = fastpath_dev_name_to_id(dev_name);

		if (unlikely(mdt_id < 0)) {
			pkt_printk(K_ERR, "%s: Unknown dev name:%s.\n", __func__, dev_name);
			return;
		}

		if (is_add) {
			pkt_printk(K_NOTICE, "%s: Set iquota of dev[%s], iquota[%lx].\n", __func__,
						dev_name, (unsigned long int)limit_buffer_size);

			/* Fill local_para_struct */
			memset(&local_para, 0, sizeof(local_para));
			local_para.msg_len = sizeof(local_para);
			local_para.cmd = MSG_ID_MDT_SET_IQUOTA_REQ;
			local_para.iq.trans_id = PT_IQ_TRANS_ID_GENERATE();
			local_para.iq.limit_buffer_size = limit_buffer_size;
			local_para.iq.mdt_id = mdt_id;
		} else {
			pkt_printk(K_NOTICE, "%s: Delete iquota of dev[%s], iquota[%lx].\n", __func__,
						dev_name, (unsigned long int)limit_buffer_size);

			/* Fill local_para_struct */
			memset(&local_para, 0, sizeof(local_para));
			local_para.msg_len = sizeof(local_para);
			local_para.cmd = MSG_ID_MDT_DEL_IQUOTA_REQ;
			local_para.iq.trans_id = PT_IQ_TRANS_ID_GENERATE();
			local_para.iq.limit_buffer_size = limit_buffer_size;
			local_para.iq.mdt_id = mdt_id;
		}

		/* Fill ipc_ilm_t */
		memset(&ilm, 0, sizeof(ilm));

		ilm.src_mod_id = AP_MOD_PKTTRC;
		ilm.dest_mod_id = MD_MOD_MDT;
		ilm.msg_id = IPC_MSG_ID_MDT_DATA_USAGE_CMD;
		ilm.local_para_ptr = (struct local_para *)&local_para;

		ret = ccci_ipc_send_ilm(0, &ilm);
		if (unlikely(ret < 0)) {
			if (unlikely(ret == -EINVAL)) {
				pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return invalid[%d], ilm[%p]!\n",
						__func__, ret, &ilm);
				return;
			}
			pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return error[%d]!\n", __func__, ret);

			/* If MD is not ready or ccci_ipc_send_ilm() failed, then try it again later. */
			schedule_delayed_work(&pkt_limit.dwork, msecs_to_jiffies(PKT_TRACK_POLLING_MD_TIMER));
		}
	} else {
		pkt_printk(K_ERR, "%s: MD is not ready!\n", __func__);
	}
}

static int __init pkt_track_init(void)
{
	int ret = 0;
#ifdef NETLINK_ENABLE
	struct netlink_kernel_cfg cfg = {
		.input = pkt_track_nl_set_cmd,
	};
#endif

	pkt_track_dt_state_s = PKT_TRACK_DT_STATE_UNINIT;

	ret = nf_register_hooks(pkt_track_ops, ARRAY_SIZE(pkt_track_ops));
	if (ret < 0) {
		pkt_printk(K_ERR, "%s: Cannot register hooks.\n", __func__);
		goto out;
	}

#ifdef NETLINK_ENABLE
	pkt_printk(K_NOTICE, "%s: Create md netlink socket.\n", __func__);
	netlink_sock_md = netlink_kernel_create(&init_net, NETLINK_NFLOG_MD, &cfg);

	if (netlink_sock_md == NULL) {

		pkt_printk(K_ERR, "%s: Cannot create md netlink socket.\n", __func__);
		ret = -1;
		goto out;
	}

	/* delayed works for Data Usage commands */
	INIT_DELAYED_WORK(&pkt_measure.dwork, pkt_track_mdt_set_global_alert_work);
	INIT_DELAYED_WORK(&pkt_limit.dwork, pkt_track_mdt_iquota_handler_work);
	spin_lock_init(&pkt_measure.lock);
	spin_lock_init(&pkt_limit.lock);

	pkt_limit.user_data = kmalloc(sizeof(struct mdt_data_limitation_t), GFP_KERNEL);
	if (!pkt_limit.user_data) {

		pkt_printk(K_ERR, "%s: limit iquota data kmalloc failed!\n", __func__);
		ret = -1;
		goto out;
	}

	pkt_measure.user_data = kmalloc(sizeof(struct mdt_data_measurement_t), GFP_KERNEL);
	if (!pkt_measure.user_data) {

		pkt_printk(K_ERR, "%s: global alert data kmalloc failed!\n", __func__);
		ret = -1;
	}
#endif

out:
	return ret;
}

static void __exit pkt_track_fini(void)
{
	synchronize_net();

#ifdef NETLINK_ENABLE
	netlink_kernel_release(netlink_sock_md);

	/* delayed works for Data Usage commands */
	cancel_delayed_work(&pkt_measure.dwork);
	cancel_delayed_work(&pkt_limit.dwork);

	kfree(pkt_measure.user_data);
	kfree(pkt_limit.user_data);
#endif

	nf_unregister_hooks(pkt_track_ops, ARRAY_SIZE(pkt_track_ops));

	pkt_track_dt_state_s = PKT_TRACK_DT_STATE_UNINIT;
}
module_init(pkt_track_init);
module_exit(pkt_track_fini);

static int pkt_track_ufpm_msg_hdlr(ipc_ilm_t *ilm)
{
	struct pkt_track_ilm_common_rsp_t *rsp = (struct pkt_track_ilm_common_rsp_t *) ilm->local_para_ptr;

	pkt_printk(K_INFO, "%s: Handle ilm from UFPM.\n", __func__);

	if (unlikely(rsp->rsp.mode != UFPM_FUNC_MODE_TETHER)) {
		pkt_printk(K_ERR, "%s: Wrong mode[%d]!\n", __func__, rsp->rsp.mode);
		return -EINVAL;
	}

	switch (ilm->msg_id) {
	case IPC_MSG_ID_UFPM_ENABLE_MD_FAST_PATH_RSP:
		if (rsp->rsp.result) {
			/* Update state machine */
			pkt_printk(K_INFO, "%s: Enable rsp reports success, result[%d]\n", __func__, rsp->rsp.result);
			pkt_track_dt_state_s = PKT_TRACK_DT_STATE_DEACTIVATE;
		} else {
			/* Update state machine */
			pkt_printk(K_INFO, "%s: Enable rsp reports failed, result[%d]\n", __func__, rsp->rsp.result);
			pkt_track_dt_state_s = PKT_TRACK_DT_STATE_UNINIT;
		}
		break;

	case IPC_MSG_ID_UFPM_DISABLE_MD_FAST_PATH_RSP:
		if (unlikely(!rsp->rsp.result)) {
			pkt_printk(K_ALET, "%s: Disable rsp reports failed, result[%d]\n", __func__, rsp->rsp.result);
			WARN_ON(1);
		}

		/* Update state machine */
		pkt_printk(K_INFO, "%s: Disable rsp reports success, result[%d]\n", __func__, rsp->rsp.result);
		pkt_track_dt_state_s = PKT_TRACK_DT_STATE_UNINIT;
		break;

	case IPC_MSG_ID_UFPM_ACTIVATE_MD_FAST_PATH_RSP:
		if (rsp->rsp.result) {
			/* Update state machine */
			pkt_printk(K_INFO, "%s: Activate rsp reports success, result[%d]\n", __func__, rsp->rsp.result);
			pkt_track_dt_state_s = PKT_TRACK_DT_STATE_ACTIVATE;
		} else {
			/* Update state machine */
			pkt_printk(K_INFO, "%s: Activate rsp reports failed, result[%d]\n", __func__, rsp->rsp.result);
			pkt_track_dt_state_s = PKT_TRACK_DT_STATE_DEACTIVATE;
		}
		break;

	case IPC_MSG_ID_UFPM_DEACTIVATE_MD_FAST_PATH_RSP:
		if (unlikely(!rsp->rsp.result)) {
			pkt_printk(K_ERR, "%s: Deactivate rsp reports failed, result[%d]\n", __func__, rsp->rsp.result);
			WARN_ON(1);
		}

		/* Update state machine */
		pkt_printk(K_INFO, "%s: Deactivate rsp reports success, result[%d]\n", __func__, rsp->rsp.result);
		pkt_track_dt_state_s = PKT_TRACK_DT_STATE_DEACTIVATE;
		break;

	default:
		pkt_printk(K_ERR, "%s: Cannot handle rsp MSG_ID[%d] from UFPM.\n", __func__, ilm->msg_id);
		break;
	}

	/* Transfer msg to usb gadget driver */
	pkt_printk(K_INFO, "%s: Transfer ilm to usb gadget driver.\n", __func__);
	musb_md_msg_hdlr(ilm);

	return 0;
}

#ifdef NETLINK_ENABLE
static int pkt_track_data_usage_msg_hdlr(ipc_ilm_t *ilm)
{
	struct pkt_track_ilm_data_usage_cmd_t *cmd_ptr =
	    (struct pkt_track_ilm_data_usage_cmd_t *) ilm->local_para_ptr;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	size_t size;
	ulog_packet_msg_t *user_data;
	struct mdt_global_alert_t *ga;
	struct mdt_iquota_t *iq;

	pkt_printk(K_INFO, "%s: It's about to send netlink message, netlink socket[%p].\n",
				__func__, netlink_sock_md);

	switch (cmd_ptr->cmd) {
	case MSG_ID_MDT_ALERT_GLOBAL_ALERT_IND:
		ga = (struct mdt_global_alert_t
		      *) (&((struct pkt_track_ilm_global_alert_ind_t *) (ilm->local_para_ptr))->ga);

		size = NLMSG_SPACE(sizeof(*user_data));
		size = max_t(size_t, size, NLMSG_GOODSIZE);
		skb = alloc_skb(size, GFP_ATOMIC);
		if (!skb) {
			pkt_printk(K_ERR, "%s: Failed to alloc_skb() for ALERT_GLOBAL_ALERT_IND!\n", __func__);
			return -ENOMEM;
		}

		nlh = nlmsg_put(skb,	/* skb */
				0,	/* portid */
				0,	/* seq */
				pt_nl_event,	/* type */
				sizeof(*user_data),	/* payload */
				0);	/* flags */
		if (!nlh) {
			pkt_printk(K_ERR, "%s: Failed to nlmsg_put() for ALERT_GLOBAL_ALERT_IND!\n", __func__);
			nlmsg_free(skb);
			return -EFAULT;
		}

		user_data = nlmsg_data(nlh);
		user_data->data_len = 0;

		strlcpy(user_data->prefix, "globalAlert", sizeof(user_data->prefix));
		strlcpy(user_data->indev_name, fastpath_data_usage_id_to_dev_name(ga->mdt_id),
			sizeof(user_data->indev_name));
		strlcpy(user_data->outdev_name, fastpath_data_usage_id_to_dev_name(ga->mdt_id),
			sizeof(user_data->outdev_name));

		NETLINK_CB(skb).dst_group = 1;

		pkt_printk(K_NOTICE, "%s: Send global alert to netlink group 1, indev_name[%s], outdev_name[%s].\n",
					__func__, user_data->indev_name, user_data->outdev_name);

		netlink_broadcast(netlink_sock_md,	/* ssk */
				  skb,	/* skb */
				  0,	/* portid */
				  1,	/* group */
				  GFP_ATOMIC);	/* allocation */

		break;

	case MSG_ID_MDT_ALERT_IQUOTA_IND:
		iq = (struct mdt_iquota_t *) (&((struct pkt_track_ilm_iquota_ind_t *) (ilm->local_para_ptr))->iq);

		size = NLMSG_SPACE(sizeof(*user_data));
		size = max_t(size_t, size, NLMSG_GOODSIZE);
		skb = alloc_skb(size, GFP_ATOMIC);
		if (!skb) {
			pkt_printk(K_ERR, "%s: Failed to alloc_skb() for ALERT_IQUOTA_IND!\n", __func__);
			return -ENOMEM;
		}

		nlh = nlmsg_put(skb,	/* skb */
				0,	/* portid */
				0,	/* seq */
				pt_nl_event,	/* type */
				sizeof(*user_data),	/* payload */
				0);	/* flags */

		if (!nlh) {
			pkt_printk(K_ERR, "%s: Failed to nlmsg_put() for ALERT_IQUOTA_IND!\n", __func__);
			nlmsg_free(skb);
			return -EFAULT;
		}

		user_data = nlmsg_data(nlh);
		user_data->data_len = 0;
		strlcpy(user_data->prefix, fastpath_data_usage_id_to_dev_name(iq->mdt_id),
			sizeof(user_data->prefix));
		strlcpy(user_data->indev_name, fastpath_data_usage_id_to_dev_name(iq->mdt_id),
			sizeof(user_data->indev_name));
		strlcpy(user_data->outdev_name, fastpath_data_usage_id_to_dev_name(iq->mdt_id),
			sizeof(user_data->outdev_name));
		NETLINK_CB(skb).dst_group = 1;

		pkt_printk(K_NOTICE, "%s: Send alert iquota to netlink group 1, indev_name[%s], outdev_name[%s].\n",
					__func__, user_data->indev_name, user_data->outdev_name);

		netlink_broadcast(netlink_sock_md,	/* ssk */
				  skb,	/* skb */
				  0,	/* portid */
				  1,	/* group */
				  GFP_ATOMIC);	/* allocation */

		break;
	default:
		pkt_printk(K_ERR, "%s: Cannot handle MSG_ID[%d] from MDT about data usage.\n", __func__, ilm->msg_id);
		WARN_ON(1);
	}

	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/* Public fucntions.                                                          */
/*----------------------------------------------------------------------------*/
int pkt_track_md_msg_hdlr(ipc_ilm_t *ilm)
{
	int ret = 0;

	pkt_printk(K_NOTICE, "%s: ilm with msg_id[0x%x] is received.\n", __func__, ilm->msg_id);

	switch (ilm->msg_id) {
	case IPC_MSG_ID_UFPM_ENABLE_MD_FAST_PATH_RSP:
	case IPC_MSG_ID_UFPM_DISABLE_MD_FAST_PATH_RSP:
	case IPC_MSG_ID_UFPM_ACTIVATE_MD_FAST_PATH_RSP:
	case IPC_MSG_ID_UFPM_DEACTIVATE_MD_FAST_PATH_RSP:
		ret = pkt_track_ufpm_msg_hdlr(ilm);
		break;

#ifdef NETLINK_ENABLE
	case IPC_MSG_ID_MDT_DATA_USAGE_CMD:
		ret = pkt_track_data_usage_msg_hdlr(ilm);
		break;
#endif

	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(pkt_track_md_msg_hdlr);

bool pkt_track_enable_md_fast_path(ufpm_enable_md_func_req_t *req)
{
	int ret;
	ipc_ilm_t ilm;
	struct pkt_track_ilm_enable_t ilm_local_para;

	pkt_printk(K_NOTICE, "%s: Send enable command to MD.\n", __func__);

	memset(&ilm, 0, sizeof(ilm));
	memset(&ilm_local_para, 0, sizeof(ilm_local_para));

	/* Fill ilm_local_para */
	memcpy(&(ilm_local_para.req), req, sizeof(ufpm_enable_md_func_req_t));
	ilm_local_para.msg_len = sizeof(ilm_local_para);

	ilm.src_mod_id = AP_MOD_PKTTRC;
	ilm.dest_mod_id = MD_MOD_UFPM;
	ilm.msg_id = IPC_MSG_ID_UFPM_ENABLE_MD_FAST_PATH_REQ;
	ilm.local_para_ptr = (struct local_para *)(&ilm_local_para);

	ret = ccci_ipc_send_ilm(0, &ilm);
	if (unlikely(ret < 0)) {
		pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm send return error[%d]!\n", __func__, ret);
		return false;
	}

	/* Update state machine */
	pkt_track_dt_state_s = PKT_TRACK_DT_STATE_INIT;

	return true;
}

bool pkt_track_disable_md_fast_path(ufpm_md_fast_path_common_req_t *req)
{
	int ret;
	ipc_ilm_t ilm;
	struct pkt_track_ilm_disable_t ilm_local_para;

	pkt_printk(K_NOTICE, "%s: Send disable command to MD.\n", __func__);

	memset(&ilm, 0, sizeof(ilm));
	memset(&ilm_local_para, 0, sizeof(ilm_local_para));

	/* Fill ilm_local_para */
	memcpy(&(ilm_local_para.req), req, sizeof(ufpm_md_fast_path_common_req_t));
	ilm_local_para.msg_len = sizeof(ilm_local_para);

	ilm.src_mod_id = AP_MOD_PKTTRC;
	ilm.dest_mod_id = MD_MOD_UFPM;
	ilm.msg_id = IPC_MSG_ID_UFPM_DISABLE_MD_FAST_PATH_REQ;
	ilm.local_para_ptr = (struct local_para *)(&ilm_local_para);

	ret = ccci_ipc_send_ilm(0, &ilm);
	if (unlikely(ret < 0)) {
		pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm send return error[%d]!\n", __func__, ret);
		return false;
	}

	return true;
}

bool pkt_track_activate_md_fast_path(ufpm_activate_md_func_req_t *req)
{
	int ret;
	ipc_ilm_t ilm;
	struct pkt_track_ilm_activate_t ilm_local_para;

	pkt_printk(K_NOTICE, "%s: Send activate command to MD.\n", __func__);

	memset(&ilm, 0, sizeof(ilm));
	memset(&ilm_local_para, 0, sizeof(ilm_local_para));

	/* Fill ilm_local_para */
	memcpy(&(ilm_local_para.req), req, sizeof(ufpm_activate_md_func_req_t));
	ilm_local_para.msg_len = sizeof(ilm_local_para);

	ilm.src_mod_id = AP_MOD_PKTTRC;
	ilm.dest_mod_id = MD_MOD_UFPM;
	ilm.msg_id = IPC_MSG_ID_UFPM_ACTIVATE_MD_FAST_PATH_REQ;
	ilm.local_para_ptr = (struct local_para *)&ilm_local_para;

	ret = ccci_ipc_send_ilm(0, &ilm);
	if (unlikely(ret < 0)) {
		pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm send return error[%d]!\n", __func__, ret);
		return false;
	}

	return true;
}

bool pkt_track_deactivate_md_fast_path(ufpm_md_fast_path_common_req_t *req)
{
	int ret;
	ipc_ilm_t ilm;
	struct pkt_track_ilm_deactivate_t ilm_local_para;

	pkt_printk(K_NOTICE, "%s: Send deactivate command to MD.\n", __func__);

	memset(&ilm, 0, sizeof(ilm));
	memset(&ilm_local_para, 0, sizeof(ilm_local_para));

	/* Fill ilm_local_para */
	memcpy(&(ilm_local_para.req), req, sizeof(ufpm_md_fast_path_common_req_t));
	ilm_local_para.msg_len = sizeof(ilm_local_para);

	ilm.src_mod_id = AP_MOD_PKTTRC;
	ilm.dest_mod_id = MD_MOD_UFPM;
	ilm.msg_id = IPC_MSG_ID_UFPM_DEACTIVATE_MD_FAST_PATH_REQ;
	ilm.local_para_ptr = (struct local_para *)(&ilm_local_para);

	ret = ccci_ipc_send_ilm(0, &ilm);
	if (unlikely(ret < 0)) {
		pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm send return error[%d]!\n", __func__, ret);
		return false;
	}

	return true;
}

#ifdef NETLINK_ENABLE
/*
 * MD Direct Tethering Data Usage BEGIN
 */
int pkt_track_mdt_set_global_alert(void)
{
	int md_status;
	int err = 0;
	unsigned long flags;

	md_status = exec_ccci_kern_func_by_md_id(0, ID_GET_MD_STATE, NULL, 0);

	if (unlikely(md_status == MD_STATE_EXCEPTION)) {
		pkt_printk(K_ERR, "%s: MD exception.\n", __func__);

		err = -ENODEV;
		return err;
	}

	if (md_status == MD_STATE_READY) {
		int ret;
		ipc_ilm_t ilm;
		struct pkt_track_ilm_global_alert_req_t local_para;
		u64 measure_buffer_size;

		spin_lock_irqsave(&pkt_measure.lock, flags);
		measure_buffer_size = pkt_measure.user_data->measure_buffer_size;
		spin_unlock_irqrestore(&pkt_measure.lock, flags);

		pkt_printk(K_NOTICE, "%s: Set global alert value[%lx].\n", __func__,
					(unsigned long int)measure_buffer_size);

		/* Fill local_para_struct */
		memset(&local_para, 0, sizeof(local_para));
		local_para.msg_len = sizeof(local_para);
		local_para.cmd = MSG_ID_MDT_SET_GLOBAL_ALERT_REQ;
		local_para.ga.trans_id = PT_GA_TRANS_ID_GENERATE();
		local_para.ga.measure_buffer_size = measure_buffer_size;

		/* Fill ipc_ilm_t */
		memset(&ilm, 0, sizeof(ilm));

		ilm.src_mod_id = AP_MOD_PKTTRC;
		ilm.dest_mod_id = MD_MOD_MDT;
		ilm.msg_id = IPC_MSG_ID_MDT_DATA_USAGE_CMD;
		ilm.local_para_ptr = (struct local_para *)&local_para;

		ret = ccci_ipc_send_ilm(0, &ilm);
		if (unlikely(ret < 0)) {
			if (unlikely(ret == -EINVAL)) {
				pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return invalid[%d], ilm[%p]!\n",
							__func__, ret, &ilm);
				err = -EINVAL;
				return err;
			}
			pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return error[%d]!\n", __func__, ret);

			/* If MD is not ready or ccci_ipc_send_ilm() failed, then try it again later. */
			schedule_delayed_work(&pkt_measure.dwork, msecs_to_jiffies(PKT_TRACK_POLLING_MD_TIMER));
			err = -EAGAIN;
		}

	} else {
		pkt_printk(K_ERR, "%s: MD is not ready!\n", __func__);
		err = -ENODEV;
	}

	return err;
}

int pkt_track_mdt_iquota_handler(void)
{
	int md_status;
	int err = 0;
	unsigned long flags;

	md_status = exec_ccci_kern_func_by_md_id(0, ID_GET_MD_STATE, NULL, 0);

	if (unlikely(md_status == MD_STATE_EXCEPTION)) {
		pkt_printk(K_ERR, "%s: MD exception.\n", __func__);
		err = -ENODEV;

		return err;
	}

	if (md_status == MD_STATE_READY) {
		int ret;
		ipc_ilm_t ilm;
		struct pkt_track_ilm_iquota_req_t local_para;
		int mdt_id;
		bool is_add;
		u64 limit_buffer_size;
		u8 dev_name[IFNAMSIZ];

		spin_lock_irqsave(&pkt_limit.lock, flags);

		is_add = pkt_limit.is_add;
		limit_buffer_size = pkt_limit.user_data->limit_buffer_size;
		memcpy(dev_name, pkt_limit.user_data->dev_name, sizeof(dev_name));

		spin_unlock_irqrestore(&pkt_limit.lock, flags);

		mdt_id = fastpath_dev_name_to_id(dev_name);

		if (unlikely(mdt_id < 0)) {
			pkt_printk(K_ERR, "%s: Unknown dev name:%s.\n", __func__, dev_name);

			err = -EINVAL;
			return err;
		}

		if (is_add) {
			pkt_printk(K_NOTICE, "%s: Set iquota of dev[%s], iquota[%lx].\n", __func__,
						dev_name, (unsigned long int)limit_buffer_size);

			/* Fill local_para_struct */
			memset(&local_para, 0, sizeof(local_para));
			local_para.msg_len = sizeof(local_para);
			local_para.cmd = MSG_ID_MDT_SET_IQUOTA_REQ;
			local_para.iq.trans_id = PT_IQ_TRANS_ID_GENERATE();
			local_para.iq.limit_buffer_size = limit_buffer_size;
			local_para.iq.mdt_id = mdt_id;
		} else {
			pkt_printk(K_NOTICE, "%s: Delete iquota of dev[%s], iquota[%lx].\n", __func__,
						dev_name, (unsigned long int)limit_buffer_size);

			/* Fill local_para_struct */
			memset(&local_para, 0, sizeof(local_para));
			local_para.msg_len = sizeof(local_para);
			local_para.cmd = MSG_ID_MDT_DEL_IQUOTA_REQ;
			local_para.iq.trans_id = PT_IQ_TRANS_ID_GENERATE();
			local_para.iq.limit_buffer_size = limit_buffer_size;
			local_para.iq.mdt_id = mdt_id;
		}


		/* Fill ipc_ilm_t */
		memset(&ilm, 0, sizeof(ilm));

		ilm.src_mod_id = AP_MOD_PKTTRC;
		ilm.dest_mod_id = MD_MOD_MDT;
		ilm.msg_id = IPC_MSG_ID_MDT_DATA_USAGE_CMD;
		ilm.local_para_ptr = (struct local_para *)&local_para;

		ret = ccci_ipc_send_ilm(0, &ilm);
		if (unlikely(ret < 0)) {
			if (unlikely(ret == -EINVAL)) {
				pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return invalid[%d], ilm[%p]!\n",
						__func__, ret, &ilm);
				err = -EINVAL;
				return err;
			}
			pkt_printk(K_ERR, "%s: ccci_ipc_send_ilm return error[%d]!\n", __func__, ret);

			/* If MD is not ready or ccci_ipc_send_ilm() failed, then try it again later. */
			schedule_delayed_work(&pkt_limit.dwork, msecs_to_jiffies(PKT_TRACK_POLLING_MD_TIMER));

			err = -EAGAIN;
		}
	} else {
		pkt_printk(K_ERR, "%s: MD is not ready!\n", __func__);
		err = -ENODEV;
	}

	return err;
}

/*
 * MD Direct Tethering Data Usage END
 */
static void pkt_track_nl_set_cmd(struct sk_buff *skb)
{
	int err = 0;
	struct nlmsghdr *nlh;
	void *user_data;
	unsigned long flags;

	nlh = nlmsg_hdr(skb);
	user_data = NLMSG_DATA(nlh);

	if (nlh->nlmsg_len > sizeof(*nlh)) {
		switch (nlh->nlmsg_type) {
		case DRT_TETHERING_CMD_ADD_ALERT:
			if (sizeof(struct mdt_data_measurement_t) != (nlh->nlmsg_len - NLMSG_HDRLEN)) {
				pkt_printk(K_ERR, "%s: Invalid nlmsg_len[%d] with nlmsg_type[%d]!\n", __func__,
							nlh->nlmsg_len, nlh->nlmsg_type);

				err = -EINVAL;
				goto out;
			}

			spin_lock_irqsave(&pkt_measure.lock, flags);
			memcpy(pkt_measure.user_data, user_data, sizeof(struct mdt_data_limitation_t));
			spin_unlock_irqrestore(&pkt_measure.lock, flags);

			err = pkt_track_mdt_set_global_alert();

			break;

		case DRT_TETHERING_CMD_ADD_QUOTA:
			if (sizeof(struct mdt_data_limitation_t) != (nlh->nlmsg_len - NLMSG_HDRLEN)) {
				pkt_printk(K_ERR, "%s: Invalid nlmsg_len[%d] with nlmsg_type[%d]!\n", __func__,
							nlh->nlmsg_len, nlh->nlmsg_type);

				err = -EINVAL;
				goto out;
			}

			spin_lock_irqsave(&pkt_limit.lock, flags);
			pkt_limit.is_add = true;
			memcpy(pkt_limit.user_data, user_data, sizeof(struct mdt_data_limitation_t));
			spin_unlock_irqrestore(&pkt_limit.lock, flags);

			err = pkt_track_mdt_iquota_handler();

			break;

		case DRT_TETHERING_CMD_DEL_QUOTA:
			if (sizeof(struct mdt_data_limitation_t) != (nlh->nlmsg_len - NLMSG_HDRLEN)) {
				pkt_printk(K_ERR, "%s: Invalid nlmsg_len[%d] with nlmsg_type[%d]!\n", __func__,
							nlh->nlmsg_len, nlh->nlmsg_type);

				err = -EINVAL;
				goto out;
			}

			spin_lock_irqsave(&pkt_limit.lock, flags);
			pkt_limit.is_add = false;
			memcpy(pkt_limit.user_data, user_data, sizeof(struct mdt_data_limitation_t));
			spin_unlock_irqrestore(&pkt_limit.lock, flags);

			err = pkt_track_mdt_iquota_handler();

			break;

		default:
			pkt_printk(K_ERR, "%s: Invalid nlmsg_type[%d]!\n", __func__, nlh->nlmsg_type);
			err = -EINVAL;
			break;
		}
	} else {
		pkt_printk(K_ERR, "%s: Invalid nlmsg_len[%d]!\n", __func__, nlh->nlmsg_len);
		err = -EINVAL;
	}

out:
	if (err >= 0)
		err = skb->len;

	netlink_ack(skb, nlh, err);
}

#endif				/* NETLINK_ENABLE */

#endif				/* CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT */
