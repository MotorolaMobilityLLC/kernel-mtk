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

#include "port_smem.h"
#include "ccci_modem.h"
#include "port_proxy.h"
#define TAG "smem"
/* FIXME, global structures are indexed by SMEM_USER_ID, this array must aligns with md1_ccci_ports[] */
static struct ccci_smem_port md1_ccci_smem_ports[] = {
	{SMEM_USER_RAW_DBM, TYPE_RAW, }, /* mt_pbm.c, this must be 1st element, user need this condition. */
	{SMEM_USER_CCB_DHL, TYPE_CCB, }, /* CCB DHL */
	{SMEM_USER_CCB_MD_MONITOR, TYPE_CCB, }, /* CCB DHL */
	{SMEM_USER_CCB_META, TYPE_CCB, }, /* CCB DHL */
	{SMEM_USER_RAW_DHL, TYPE_RAW, }, /* raw region for DHL settings */
	{SMEM_USER_RAW_NETD, TYPE_RAW, }, /* for direct tethering */
	{SMEM_USER_RAW_USB, TYPE_RAW, }, /* for derect tethering */
	{SMEM_USER_RAW_AUDIO, TYPE_RAW, }, /* for speech */
	{SMEM_USER_RAW_LWA, TYPE_RAW, }, /* for LWA Wi-FI driver */
};
static struct ccci_smem_port md3_ccci_smem_ports[] = {
	{SMEM_USER_RAW_DBM, TYPE_RAW, }, /* mt_pbm.c */
	{SMEM_USER_CCB_DHL, TYPE_CCB, }, /* dummy */
	{SMEM_USER_CCB_MD_MONITOR, TYPE_CCB, }, /* dummy */
	{SMEM_USER_CCB_META, TYPE_CCB, }, /*dummy */
	{SMEM_USER_RAW_DHL, TYPE_RAW, }, /* dummy */
	{SMEM_USER_RAW_NETD, TYPE_RAW, }, /* dummy */
	{SMEM_USER_RAW_USB, TYPE_RAW, }, /* dummy */
	{SMEM_USER_RAW_AUDIO, TYPE_RAW, }, /* for speech */
	{SMEM_USER_RAW_LWA, TYPE_RAW, }, /* dummy */
};
struct tx_notify_task md1_tx_notify_tasks[SMEM_USER_MAX];

static enum hrtimer_restart smem_tx_timer_func(struct hrtimer *timer)
{
	struct tx_notify_task *notify_task = container_of(timer, struct tx_notify_task, notify_timer);

	port_proxy_send_ccb_tx_notify_to_md(notify_task->port->port_proxy, notify_task->core_id);
	return HRTIMER_NORESTART;
}

void collect_ccb_info(struct ccci_modem *md)
{
	int i, j;
	int data_buf_counter = 0;
	static int collected;

	if (!collected)
		collected = 1;
	else
		return;

	for (i = SMEM_USER_CCB_START; i <= SMEM_USER_CCB_END; i++) {
		md1_ccci_smem_ports[i].length = 0;
		md1_ccci_smem_ports[i].ccb_ctrl_offset = data_buf_counter;

		for (j = 0; j < ccb_configs_len; j++) {
			if (i == ccb_configs[j].user_id) {
				md1_ccci_smem_ports[i].length +=
					ccb_configs[j].dl_buff_size + ccb_configs[j].ul_buff_size;
				data_buf_counter++;
			}
		}

		/* align to 4k */
		md1_ccci_smem_ports[i].length = (md1_ccci_smem_ports[i].length + 0xFFF) & (~0xFFF);

		if (i == SMEM_USER_CCB_START) {
			md1_ccci_smem_ports[i].addr_vir = md->mem_layout.ccci_ccb_data_base_vir;
			md1_ccci_smem_ports[i].addr_phy = md->mem_layout.ccci_ccb_data_base_phy;
		} else {
			md1_ccci_smem_ports[i].addr_vir = md1_ccci_smem_ports[i-1].addr_vir +
				md1_ccci_smem_ports[i-1].length;
			md1_ccci_smem_ports[i].addr_phy = md1_ccci_smem_ports[i-1].addr_phy +
				md1_ccci_smem_ports[i-1].length;
		}

		CCCI_BOOTUP_LOG(-1, "smem", "smem_user=%d, ccb_ctrl_offset=%d, buf_len=%d\n",
				i, md1_ccci_smem_ports[i].ccb_ctrl_offset,  md1_ccci_smem_ports[i].length);
	}
}

int port_smem_init(struct ccci_port *port)
{
	struct ccci_modem *md = port->modem;
	/* FIXME, port->minor is indexed by SMEM_USER_ID */
	switch (port->md_id) {
	case MD_SYS1:
		port->private_data = &(md1_ccci_smem_ports[port->minor]);
		spin_lock_init(&(md1_ccci_smem_ports[port->minor].write_lock));
		md1_ccci_smem_ports[port->minor].port = port;
		collect_ccb_info(md);
		break;
	case MD_SYS3:
		port->private_data = &(md3_ccci_smem_ports[port->minor]);
		md3_ccci_smem_ports[port->minor].port = port;
		break;
	default:
		return -1;
	}
	port->minor += CCCI_SMEM_MINOR_BASE;

	return 0;
}

int port_smem_tx_nofity(struct ccci_port *port, unsigned int user_data)
{
	struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;

	if (smem_port->type != TYPE_CCB)
		return -EFAULT;
	if (!hrtimer_active(&(md1_tx_notify_tasks[smem_port->user_id].notify_timer))) {
		port_proxy_send_ccb_tx_notify_to_md(port->port_proxy, user_data);
		md1_tx_notify_tasks[smem_port->user_id].core_id = user_data;
		md1_tx_notify_tasks[smem_port->user_id].port = port;
		hrtimer_start(&(md1_tx_notify_tasks[smem_port->user_id].notify_timer),
				ktime_set(0, 1000000), HRTIMER_MODE_REL);
	}
	return 0;
}

int port_smem_rx_poll(struct ccci_port *port, unsigned int user_data)
{
	struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;
	int ret;
	unsigned long flags;

	if (smem_port->type != TYPE_CCB)
		return -EFAULT;
	CCCI_REPEAT_LOG(-1, "smem", "before wait event, bitmask=%x\n", user_data);
	ret = wait_event_interruptible(smem_port->rx_wq, smem_port->wakeup & user_data);
	spin_lock_irqsave(&smem_port->write_lock, flags);
	smem_port->wakeup &= ~user_data;
	CCCI_REPEAT_LOG(-1, "smem", "after wait event, wakeup=%x\n", smem_port->wakeup);
	spin_unlock_irqrestore(&smem_port->write_lock, flags);

	if (ret == -ERESTARTSYS)
		ret = -EINTR;
	return ret;
}

int port_smem_rx_wakeup(struct ccci_port *port)
{
	struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;
	unsigned long flags;

	if (smem_port->type != TYPE_CCB)
		return -EFAULT;
	spin_lock_irqsave(&smem_port->write_lock, flags);
	smem_port->wakeup = 0xFFFFFFFF;
	spin_unlock_irqrestore(&smem_port->write_lock, flags);

	CCCI_DEBUG_LOG(-1, "smem", "wakeup port.\n");
	wake_up_all(&smem_port->rx_wq);
	return 0;
}

int port_smem_cfg(struct ccci_modem *md)
{
	int i, md_id;

	md_id = md->index;

	switch (md_id) {
	case MD_SYS1:
#ifdef FEATURE_DHL_CCB_RAW_SUPPORT
		md1_ccci_smem_ports[SMEM_USER_RAW_DHL].addr_vir = md->mem_layout.ccci_raw_dhl_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_DHL].addr_phy = md->mem_layout.ccci_raw_dhl_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_DHL].length = md->mem_layout.ccci_raw_dhl_size;
		CCCI_NORMAL_LOG(-1, "smem", "CCB addr_phy=%llx, size=%d\n", md->mem_layout.ccci_ccb_data_base_phy,
				md->mem_layout.ccci_ccb_data_size);
#ifdef FEATURE_LWA
		md1_ccci_smem_ports[SMEM_USER_RAW_LWA].addr_vir = md->mem_layout.ccci_lwa_smem_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_LWA].addr_phy = md->mem_layout.ccci_lwa_smem_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_LWA].length = md->mem_layout.ccci_lwa_smem_size;
#endif
#endif

#ifdef FEATURE_DBM_SUPPORT
		md1_ccci_smem_ports[SMEM_USER_RAW_DBM].addr_vir = md->smem_layout.ccci_exp_smem_dbm_debug_vir +
								CCCI_SMEM_DBM_GUARD_SIZE;
		md1_ccci_smem_ports[SMEM_USER_RAW_DBM].length = CCCI_SMEM_DBM_SIZE;
#endif

		md1_ccci_smem_ports[SMEM_USER_RAW_NETD].addr_vir = md->smem_layout.ccci_dt_netd_smem_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_NETD].addr_phy = md->smem_layout.ccci_dt_netd_smem_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_NETD].length = md->smem_layout.ccci_dt_netd_smem_size;

		md1_ccci_smem_ports[SMEM_USER_RAW_USB].addr_vir = md->smem_layout.ccci_dt_usb_smem_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_USB].addr_phy = md->smem_layout.ccci_dt_usb_smem_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_USB].length = md->smem_layout.ccci_dt_usb_smem_size;

		md1_ccci_smem_ports[SMEM_USER_RAW_AUDIO].addr_vir = md->smem_layout.ccci_raw_audio_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_AUDIO].addr_phy = md->smem_layout.ccci_raw_audio_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_AUDIO].length = md->smem_layout.ccci_raw_audio_size;

		for (i = 0; i < ARRAY_SIZE(md1_ccci_smem_ports); i++) {
			md1_ccci_smem_ports[i].state = CCB_USER_INVALID;
			md1_ccci_smem_ports[i].wakeup = 0;
			init_waitqueue_head(&(md1_ccci_smem_ports[i].rx_wq));
		}

		for (i = 0; i < ARRAY_SIZE(md1_tx_notify_tasks); i++) {
			hrtimer_init(&(md1_tx_notify_tasks[i].notify_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			md1_tx_notify_tasks[i].notify_timer.function = smem_tx_timer_func;
		}
		break;
	case MD_SYS3:
#ifdef FEATURE_DBM_SUPPORT
		md3_ccci_smem_ports[SMEM_USER_RAW_DBM].addr_vir = md->smem_layout.ccci_exp_smem_dbm_debug_vir +
								CCCI_SMEM_DBM_GUARD_SIZE;
		md3_ccci_smem_ports[SMEM_USER_RAW_DBM].length = CCCI_SMEM_DBM_SIZE;
#endif
		md3_ccci_smem_ports[SMEM_USER_RAW_AUDIO].addr_vir = md->smem_layout.ccci_raw_audio_base_vir;
		md3_ccci_smem_ports[SMEM_USER_RAW_AUDIO].addr_phy = md->smem_layout.ccci_raw_audio_base_phy;
		md3_ccci_smem_ports[SMEM_USER_RAW_AUDIO].length = md->smem_layout.ccci_raw_audio_size;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

void __iomem *get_smem_start_addr(int md_id, SMEM_USER_ID user_id, int *size_o)
{
	void __iomem *addr = NULL;

	switch (md_id) {
	case MD_SYS1:
		if (user_id < 0 || user_id >= ARRAY_SIZE(md1_ccci_smem_ports))
			return NULL;
		addr = (void __iomem *)md1_ccci_smem_ports[user_id].addr_vir;
		if (size_o)
			*size_o = md1_ccci_smem_ports[user_id].length;
		break;
	case MD_SYS3:
		if (user_id < 0 || user_id >= ARRAY_SIZE(md3_ccci_smem_ports))
			return NULL;
		addr = (void __iomem *)md3_ccci_smem_ports[user_id].addr_vir;
		if (size_o)
			*size_o = md3_ccci_smem_ports[user_id].length;
		break;
	default:
		return NULL;
	}
	return addr;
}

long port_ccb_ioctl(struct ccci_port *port, unsigned int cmd, unsigned long arg)
{
	struct ccci_modem *md = port->modem;
	int md_id = port->md_id;
	long ret = 0;
	struct ccci_ccb_config in_ccb, out_ccb;

	/*
	 * all users share this ccb ctrl region, and port CCCI_SMEM_CH's initailization is special,
	 * so, these ioctl cannot use CCCI_SMEM_CH channel.
	 */
	switch (cmd) {
	case CCCI_IOC_MB:
		CCCI_DEBUG_LOG(-1, "smem", "smp_mb() from userspace.\n");
		smp_mb();
		break;
	case CCCI_IOC_CCB_CTRL_BASE:
		ret = put_user((unsigned int)md->smem_layout.ccci_ccb_ctrl_base_phy,
				(unsigned int __user *)arg);
		break;
	case CCCI_IOC_CCB_CTRL_LEN:
		ret = put_user((unsigned int)md->smem_layout.ccci_ccb_ctrl_size,
				(unsigned int __user *)arg);

		break;
	case CCCI_IOC_GET_CCB_CONFIG_LENGTH:
		CCCI_NORMAL_LOG(md_id, "smem", "ccb_configs_len: %d\n", ccb_configs_len);

		ret = put_user(ccb_configs_len, (unsigned int __user *)arg);
		break;
	case CCCI_IOC_GET_CCB_CONFIG:
		if (copy_from_user(&in_ccb, (void __user *)arg, sizeof(struct ccci_ccb_config)))
			CCCI_ERR_MSG(md_id, TAG, "set user_id fail: copy_from_user fail!\n");
		else
		/* use user_id as input param, which is the array index, and it will override user space's ID value */
		if (in_ccb.user_id > ccb_configs_len) {
			ret = -EINVAL;
			break;
		}
		memcpy(&out_ccb, &ccb_configs[in_ccb.user_id], sizeof(struct ccci_ccb_config));
		/* user space's CCB array index is count from zero, as it only deal with CCB user, no raw user */
		out_ccb.user_id -= SMEM_USER_CCB_START;
		if (copy_to_user((void __user *)arg, &out_ccb, sizeof(struct ccci_ccb_config)))
			CCCI_ERROR_LOG(md_id, TAG, "copy_to_user ccb failed !!\n");
		break;
	default:
		ret = -ENOTTY;
		break;
	}
	return ret;
}

long port_smem_ioctl(struct ccci_port *port, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	int md_id = port->md_id;
	unsigned int data;
	struct ccci_smem_port *smem_port;
	struct ccci_modem *md = port->modem;
	unsigned char *ptr;
	struct ccci_ccb_debug debug_in, debug_out;

	ptr = NULL;

	switch (cmd) {
	case CCCI_IOC_MB:
		smp_mb();
		break;
	case CCCI_IOC_GET_CCB_DEBUG_VAL:
		if (copy_from_user(&debug_in, (void __user *)arg, sizeof(struct ccci_ccb_debug))) {
			CCCI_ERR_MSG(md_id, TAG, "set user_id fail: copy_from_user fail!\n");
		} else {
			CCCI_DEBUG_LOG(md_id, TAG, "get buf_num=%d, page_num=%d\n",
					debug_in.buffer_id, debug_in.page_id);
		}
		memset(&debug_out, 0, sizeof(debug_out));
		if (debug_in.buffer_id == 0) {
			ptr = (char *)md->mem_layout.ccci_ccb_data_base_vir + ccb_configs[0].dl_buff_size +
				debug_in.page_id*ccb_configs[0].ul_page_size + 8;
			debug_out.value = *ptr;
		} else if (debug_in.buffer_id == 1) {
			ptr  = (char *)md->mem_layout.ccci_ccb_data_base_vir + ccb_configs[0].dl_buff_size +
				ccb_configs[0].ul_buff_size + ccb_configs[1].dl_buff_size +
				debug_in.page_id*ccb_configs[1].ul_page_size + 8;
			debug_out.value = *ptr;
		} else
			CCCI_ERR_MSG(md_id, TAG, "wrong buffer num\n");

		if (copy_to_user((void __user *)arg, &debug_out, sizeof(struct ccci_ccb_debug)))
			CCCI_ERROR_LOG(md_id, TAG, "copy_to_user ccb failed !!\n");

		break;
	case CCCI_IOC_SMEM_BASE:
		smem_port = (struct ccci_smem_port *)port->private_data;
		CCCI_NORMAL_LOG(-1, "smem", "smem_port->addr_phy=%llx\n", smem_port->addr_phy);
		ret = put_user((unsigned int)smem_port->addr_phy,
				(unsigned int __user *)arg);
		break;
	case CCCI_IOC_SMEM_LEN:
		smem_port = (struct ccci_smem_port *)port->private_data;
		ret = put_user((unsigned int)smem_port->length,
				(unsigned int __user *)arg);
		break;
	case CCCI_IOC_CCB_CTRL_OFFSET:
		CCCI_REPEAT_LOG(md_id, TAG, "rx_ch who invoke CCCI_IOC_CCB_CTRL_OFFSET:%d\n", port->rx_ch);

		smem_port = (struct ccci_smem_port *)port->private_data;
		ret = put_user((unsigned int)smem_port->ccb_ctrl_offset,
				(unsigned int __user *)arg);
		CCCI_REPEAT_LOG(md_id, TAG, "get ctrl_offset=%d\n", smem_port->ccb_ctrl_offset);

		break;

	case CCCI_IOC_SMEM_TX_NOTIFY:
		if (copy_from_user(&data, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md_id, TAG, "smem tx notify fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else
			ret = port_smem_tx_nofity(port, data);
		break;
	case CCCI_IOC_SMEM_RX_POLL:

		if (copy_from_user(&data, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md_id, TAG, "smem rx poll fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			ret = port_smem_rx_poll(port, data);
		}
		break;
	case CCCI_IOC_SMEM_SET_STATE:
		smem_port = (struct ccci_smem_port *)port->private_data;
		if (copy_from_user(&data, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md_id, TAG, "smem set state fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else
			smem_port->state = data;
		break;
	case CCCI_IOC_SMEM_GET_STATE:
		smem_port = (struct ccci_smem_port *)port->private_data;
		ret = put_user((unsigned int)smem_port->state,
				(unsigned int __user *)arg);
		break;
	default:
		ret = port_proxy_user_ioctl(port->port_proxy, port->rx_ch, cmd, arg);
	}

	return ret;
}

