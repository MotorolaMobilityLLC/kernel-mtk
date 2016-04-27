/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*
 * CCCI common service and routine. Consider it as a "logical" layer.
 *
 * V0.1: Xiao Wang <xiao.wang@mediatek.com>
 */

#include <linux/list.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/kobject.h>

#include <mt-plat/mt_ccci_common.h>
#include "ccci_config.h"
#include "ccci_platform.h"

#include "ccci_core.h"
#include "ccci_bm.h"
#include "ccci_support.h"
#include "port_cfg.h"
static LIST_HEAD(modem_list);	/* don't use array, due to MD index may not be continuous */
static void *dev_class;

/* used for throttling feature - start */
unsigned long ccci_modem_boot_count[5];
unsigned long ccci_get_md_boot_count(int md_id)
{
	return ccci_modem_boot_count[md_id];
}

/* used for throttling feature - end */

int boot_md_show(int md_id, char *buf, int size)
{
	int curr = 0;
	struct ccci_modem *md;

	list_for_each_entry(md, &modem_list, entry) {
		if (md->index == md_id)
			curr += snprintf(&buf[curr], size, "md%d:%d/%d/%d", md->index + 1,
					 md->md_state, md->boot_stage, md->ex_stage);
	}
	return curr;
}

int boot_md_store(int md_id)
{
	struct ccci_modem *md;

	list_for_each_entry(md, &modem_list, entry) {
		CCCI_BOOTUP_LOG(md_id, CORE, "ccci core boot md%d, md_state=%d\n", md_id + 1, md->md_state);
		if (md->index == md_id && md->md_state == GATED) {
			md->ops->start(md);
			return 0;
		}
	}
	return -1;
}
char *ccci_get_ap_platform(void)
{
	return AP_PLATFORM_INFO;
}

/*
 * if recv_request returns 0 or -CCCI_ERR_DROP_PACKET, then it's port's duty to free the request, and caller should
 * NOT reference the request any more. but if it returns other error, caller should be responsible to free the request.
 */
int ccci_port_recv_request(struct ccci_modem *md, struct ccci_request *req, struct sk_buff *skb)
{
	struct ccci_header *ccci_h;
	struct ccci_port *port = NULL;
	struct list_head *port_list = NULL;
	int ret = -CCCI_ERR_CHANNEL_NUM_MIS_MATCH;
	char matched = 0;

	if (likely(skb)) {
		ccci_h = (struct ccci_header *)skb->data;
	} else if (req) {
		ccci_h = (struct ccci_header *)req->skb->data;
	} else {
		ret = -CCCI_ERR_INVALID_PARAM;
		goto err_exit;
	}
	if (unlikely(ccci_h->channel >= CCCI_MAX_CH_NUM)) {
		ret = -CCCI_ERR_CHANNEL_NUM_MIS_MATCH;
		goto err_exit;
	}
	if (unlikely((md->md_state == GATED || md->md_state == INVALID) &&
		     ccci_h->channel != CCCI_MONITOR_CH)) {
		ret = -CCCI_ERR_HIF_NOT_POWER_ON;
		goto err_exit;
	}

	port_list = &md->rx_ch_ports[ccci_h->channel];
	list_for_each_entry(port, port_list, entry) {
		/*
		 * multi-cast is not supported, because one port may freed or modified this request
		 * before another port can process it. but we still can use req->state to achive some
		 * kind of multi-cast if needed.
		 */
		matched =
		    (port->ops->req_match == NULL || req == NULL) ?
			(ccci_h->channel == port->rx_ch) : port->ops->req_match(port, req);
		if (matched) {
			if (likely(skb && port->ops->recv_skb)) {
				ret = port->ops->recv_skb(port, skb);
			} else if (req && port->ops->recv_request) {
				ret = port->ops->recv_request(port, req);
			} else {
				CCCI_ERROR_LOG(md->index, CORE, "port->ops->recv_request is null\n");
				ret = -CCCI_ERR_CHANNEL_NUM_MIS_MATCH;
				goto err_exit;
			}
			if (ret == -CCCI_ERR_PORT_RX_FULL)
				port->rx_busy_count++;
			break;
		}
	}

 err_exit:
	if (ret == -CCCI_ERR_CHANNEL_NUM_MIS_MATCH || ret == -CCCI_ERR_HIF_NOT_POWER_ON) {
		/* CCCI_ERROR_LOG(md->index, CORE, "drop on channel %d\n", ccci_h->channel); */ /* Fix me, mask temp */
		if (req) {
			list_del(&req->entry);
			req->policy = RECYCLE;
			ccci_free_req(req);
		} else if (skb) {
			dev_kfree_skb_any(skb);
		}
		ret = -CCCI_ERR_DROP_PACKET;
	}
	return ret;
}

static void ccci_dump_log_rec(struct ccci_modem *md, struct ccci_log *log)
{
	u64 ts_nsec = log->tv;
	unsigned long rem_nsec;

	if (ts_nsec == 0)
		return;
	rem_nsec = do_div(ts_nsec, 1000000000);
	if (!log->droped) {
		CCCI_MEM_LOG(md->index, CORE, "%08X %08X %08X %08X  %5lu.%06lu\n",
		       log->msg.data[0], log->msg.data[1], *(((u32 *)&log->msg) + 2),
		       log->msg.reserved, (unsigned long)ts_nsec, rem_nsec / 1000);
	} else {
		CCCI_MEM_LOG(md->index, CORE, "%08X %08X %08X %08X  %5lu.%06lu -\n",
		       log->msg.data[0], log->msg.data[1], *(((u32 *)&log->msg) + 2),
		       log->msg.reserved, (unsigned long)ts_nsec, rem_nsec / 1000);
	}
}

void ccci_dump_log_add(struct ccci_modem *md, DIRECTION dir, int queue_index, struct ccci_header *msg, int is_droped)
{
#ifdef PACKET_HISTORY_DEPTH
	if (dir == OUT) {
		memcpy(&md->tx_history[queue_index][md->tx_history_ptr[queue_index]].msg, msg,
		       sizeof(struct ccci_header));
		md->tx_history[queue_index][md->tx_history_ptr[queue_index]].tv = local_clock();
		md->tx_history[queue_index][md->tx_history_ptr[queue_index]].droped = is_droped;
		md->tx_history_ptr[queue_index]++;
		md->tx_history_ptr[queue_index] &= (PACKET_HISTORY_DEPTH - 1);
	}
	if (dir == IN) {
		memcpy(&md->rx_history[queue_index][md->rx_history_ptr[queue_index]].msg, msg,
		       sizeof(struct ccci_header));
		md->rx_history[queue_index][md->rx_history_ptr[queue_index]].tv = local_clock();
		md->rx_history[queue_index][md->rx_history_ptr[queue_index]].droped = is_droped;
		md->rx_history_ptr[queue_index]++;
		md->rx_history_ptr[queue_index] &= (PACKET_HISTORY_DEPTH - 1);
	}
#endif
}

void ccci_dump_log_history(struct ccci_modem *md, int dump_multi_rec, int tx_queue_num, int rx_queue_num)
{
#ifdef PACKET_HISTORY_DEPTH
	int i, j;

	if (dump_multi_rec) {
		for (i = 0; i < ((tx_queue_num <= MAX_TXQ_NUM) ? tx_queue_num : MAX_TXQ_NUM); i++) {
			CCCI_MEM_LOG_TAG(md->index, CORE, "dump txq%d packet history, ptr=%d\n", i,
			       md->tx_history_ptr[i]);
			for (j = 0; j < PACKET_HISTORY_DEPTH; j++)
				ccci_dump_log_rec(md, &md->tx_history[i][j]);
		}
		for (i = 0; i < ((rx_queue_num <= MAX_RXQ_NUM) ? rx_queue_num : MAX_RXQ_NUM); i++) {
			CCCI_MEM_LOG_TAG(md->index, CORE, "dump rxq%d packet history, ptr=%d\n", i,
			       md->rx_history_ptr[i]);
			for (j = 0; j < PACKET_HISTORY_DEPTH; j++)
				ccci_dump_log_rec(md, &md->rx_history[i][j]);
		}
	} else {
		CCCI_MEM_LOG_TAG(md->index, CORE, "dump txq%d packet history, ptr=%d\n", tx_queue_num,
		       md->tx_history_ptr[tx_queue_num]);
		for (j = 0; j < PACKET_HISTORY_DEPTH; j++)
			ccci_dump_log_rec(md, &md->tx_history[tx_queue_num][j]);
		CCCI_MEM_LOG_TAG(md->index, CORE, "dump rxq%d packet history, ptr=%d\n", rx_queue_num,
		       md->rx_history_ptr[rx_queue_num]);
		for (j = 0; j < PACKET_HISTORY_DEPTH; j++)
			ccci_dump_log_rec(md, &md->rx_history[rx_queue_num][j]);
	}
#endif
}

/*
 * most of this file is copied from mtk_ccci_helper.c, we use this function to
 * translate legacy data structure into current CCCI core.
 */
void ccci_config_modem(struct ccci_modem *md)
{
	phys_addr_t md_resv_mem_addr = 0, md_resv_smem_addr = 0;
	/* void __iomem *smem_base_vir; */
	unsigned int md_resv_mem_size = 0, md_resv_smem_size = 0;

	/* setup config */
	md->config.load_type = get_modem_support_cap(md->index);
	if (get_modem_is_enabled(md->index))
		md->config.setting |= MD_SETTING_ENABLE;
	else
		md->config.setting &= ~MD_SETTING_ENABLE;

	/* Get memory info */
	get_md_resv_mem_info(md->index, &md_resv_mem_addr, &md_resv_mem_size, &md_resv_smem_addr, &md_resv_smem_size);
	/* setup memory layout */
	/* MD image */
	md->mem_layout.md_region_phy = md_resv_mem_addr;
	md->mem_layout.md_region_size = md_resv_mem_size;
	md->mem_layout.md_region_vir = ioremap_nocache(md->mem_layout.md_region_phy, MD_IMG_DUMP_SIZE);
		/* do not remap whole region, consume too much vmalloc space */
	/* DSP image */
	md->mem_layout.dsp_region_phy = 0;
	md->mem_layout.dsp_region_size = 0;
	md->mem_layout.dsp_region_vir = 0;
	/* Share memory */
	md->mem_layout.smem_region_phy = md_resv_smem_addr;
	md->mem_layout.smem_region_size = md_resv_smem_size;
	md->mem_layout.smem_region_vir =
	    ioremap_nocache(md->mem_layout.smem_region_phy, md->mem_layout.smem_region_size);

	/* exception region */
	md->smem_layout.ccci_exp_smem_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_EXCEPTION;
	md->smem_layout.ccci_exp_smem_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_EXCEPTION;
	md->smem_layout.ccci_exp_smem_size = CCCI_SMEM_SIZE_EXCEPTION;
	md->smem_layout.ccci_exp_dump_size = CCCI_SMEM_DUMP_SIZE;

	/* dump region */
	md->smem_layout.ccci_exp_smem_ccci_debug_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_CCCI_DEBUG;
	md->smem_layout.ccci_exp_smem_ccci_debug_size = CCCI_SMEM_SIZE_CCCI_DEBUG;
	md->smem_layout.ccci_exp_smem_mdss_debug_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_MDSS_DEBUG;
#ifdef MD_UMOLY_EE_SUPPORT
	if (md->index == MD_SYS1)
		md->smem_layout.ccci_exp_smem_mdss_debug_size = CCCI_SMEM_SIZE_MDSS_DEBUG_UMOLY;
	else
#endif
		md->smem_layout.ccci_exp_smem_mdss_debug_size = CCCI_SMEM_SIZE_MDSS_DEBUG;
	md->smem_layout.ccci_exp_smem_sleep_debug_vir =
		md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_SLEEP_MODE_DBG;
	md->smem_layout.ccci_exp_smem_sleep_debug_size = CCCI_SMEM_SLEEP_MODE_DBG_DUMP;
#ifdef FEATURE_DBM_SUPPORT
	md->smem_layout.ccci_exp_smem_dbm_debug_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_DBM_DEBUG;
#endif

	/*runtime region */
	md->smem_layout.ccci_rt_smem_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_RUNTIME;
	md->smem_layout.ccci_rt_smem_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_RUNTIME;
	md->smem_layout.ccci_rt_smem_size = CCCI_SMEM_SIZE_RUNTIME;

	/* CCISM region */
#ifdef FEATURE_SCP_CCCI_SUPPORT
	md->smem_layout.ccci_ccism_smem_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_CCISM;
	md->smem_layout.ccci_ccism_smem_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_CCISM;
	md->smem_layout.ccci_ccism_smem_size = CCCI_SMEM_SIZE_CCISM;
	md->smem_layout.ccci_ccism_dump_size = CCCI_SMEM_CCISM_DUMP_SIZE;
#endif
	/* CCB DHL region */
#ifdef FEATURE_DHL_CCB_RAW_SUPPORT
	md->smem_layout.ccci_ccb_dhl_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_CCB_DHL;
	md->smem_layout.ccci_ccb_dhl_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_CCB_DHL;
	md->smem_layout.ccci_ccb_dhl_size = CCCI_SMEM_SIZE_CCB_DHL;
	md->smem_layout.ccci_raw_dhl_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_RAW_DHL;
	md->smem_layout.ccci_raw_dhl_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_RAW_DHL;
	md->smem_layout.ccci_raw_dhl_size = CCCI_SMEM_SIZE_RAW_DHL;
#endif
	/* direct tethering region */
#ifdef FEATURE_DIRECT_TETHERING_LOGGING
	md->smem_layout.ccci_dt_netd_smem_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_DT_NETD;
	md->smem_layout.ccci_dt_netd_smem_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_DT_NETD;
	md->smem_layout.ccci_dt_netd_smem_size = CCCI_SMEM_SIZE_DT_NETD;
	md->smem_layout.ccci_dt_usb_smem_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_DT_USB;
	md->smem_layout.ccci_dt_usb_smem_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_DT_USB;
	md->smem_layout.ccci_dt_usb_smem_size = CCCI_SMEM_SIZE_DT_USB;
#endif

	/* AP<->MD CCIF share memory region */
#ifdef CCCI_SMEM_OFFSET_CCIF_SMEM
	if (md->index == MD_SYS3) {
		md->smem_layout.ccci_ccif_smem_base_phy = md->mem_layout.smem_region_phy + CCCI_SMEM_OFFSET_CCIF_SMEM;
		md->smem_layout.ccci_ccif_smem_base_vir = md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_CCIF_SMEM;
		md->smem_layout.ccci_ccif_smem_size = CCCI_SMEM_SIZE_CCIF_SMEM;
	}
#endif
	/* smart logging region */
#ifdef FEATURE_SMART_LOGGING
	if (md->index == MD_SYS1) {
		md->smem_layout.ccci_smart_logging_base_phy = md->mem_layout.smem_region_phy +
								CCCI_SMEM_OFFSET_SMART_LOGGING;
		md->smem_layout.ccci_smart_logging_base_vir = md->mem_layout.smem_region_vir +
								CCCI_SMEM_OFFSET_SMART_LOGGING;
		md->smem_layout.ccci_smart_logging_size = md->mem_layout.smem_region_size -
								CCCI_SMEM_OFFSET_SMART_LOGGING;
	}
#endif

	/* md1 md3 shared memory region and remap */
	get_md1_md3_resv_smem_info(md->index, &md->mem_layout.md1_md3_smem_phy,
		&md->mem_layout.md1_md3_smem_size);
	md->mem_layout.md1_md3_smem_vir =
	    ioremap_nocache(md->mem_layout.md1_md3_smem_phy, md->mem_layout.md1_md3_smem_size);
	if (md->index == MD_SYS3)
		memset_io(md->mem_layout.md1_md3_smem_vir, 0, md->mem_layout.md1_md3_smem_size);

	/* updae image info */
	md->img_info[IMG_MD].type = IMG_MD;
	md->img_info[IMG_MD].address = md->mem_layout.md_region_phy;
	md->img_info[IMG_DSP].type = IMG_DSP;
	md->img_info[IMG_DSP].address = md->mem_layout.dsp_region_phy;
	md->img_info[IMG_ARMV7].type = IMG_ARMV7;

	if (md->config.setting & MD_SETTING_ENABLE)
		ccci_set_mem_remap(md, md_resv_smem_addr - md_resv_mem_addr,
				   ALIGN(md_resv_mem_addr + md_resv_mem_size + md_resv_smem_size, 0x2000000));
#if 0
	CCCI_INF_MSG(md->index, CORE, "dump memory layout\n");
	ccci_mem_dump(md->index, &md->mem_layout, sizeof(struct ccci_mem_layout));
	ccci_mem_dump(md->index, &md->smem_layout, sizeof(struct ccci_smem_layout));
#endif
}

/*
 * for debug log: 0 to disable; 1 for print to ram; 2 for print to uart
 * other value to desiable all log
 */
#ifndef CCCI_LOG_LEVEL
#define CCCI_LOG_LEVEL 0
#endif
unsigned int ccci_debug_enable = CCCI_LOG_LEVEL;

/* ================================================================== */
/* MD relate sys */
/* ================================================================== */
static void ccci_md_obj_release(struct kobject *kobj)
{
	struct ccci_modem *md = container_of(kobj, struct ccci_modem, kobj);

	CCCI_DEBUG_LOG(md->index, SYSFS, "md kobject release\n");
}

static ssize_t ccci_md_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	ssize_t len = 0;
	struct ccci_md_attribute *a = container_of(attr, struct ccci_md_attribute, attr);

	if (a->show)
		len = a->show(a->modem, buf);

	return len;
}

static ssize_t ccci_md_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t len = 0;
	struct ccci_md_attribute *a = container_of(attr, struct ccci_md_attribute, attr);

	if (a->store)
		len = a->store(a->modem, buf, count);

	return len;
}

static const struct sysfs_ops ccci_md_sysfs_ops = {
	.show = ccci_md_attr_show,
	.store = ccci_md_attr_store
};

static struct attribute *ccci_md_default_attrs[] = {
	NULL
};

static struct kobj_type ccci_md_ktype = {
	.release = ccci_md_obj_release,
	.sysfs_ops = &ccci_md_sysfs_ops,
	.default_attrs = ccci_md_default_attrs
};

#ifdef FEATURE_SCP_CCCI_SUPPORT
#include <scp_ipi.h>
static int scp_state = SCP_CCCI_STATE_INVALID;
static struct ccci_ipi_msg scp_ipi_tx_msg;
static struct mutex scp_ipi_tx_mutex;
static struct work_struct scp_ipi_rx_work;
static wait_queue_head_t scp_ipi_rx_wq;
static struct ccci_skb_queue scp_ipi_rx_skb_list;

static void scp_md_state_sync_work(struct work_struct *work)
{
	struct ccci_modem *md = container_of(work, struct ccci_modem, scp_md_state_sync_work);
	int data;

	switch (md->boot_stage) {
	case MD_BOOT_STAGE_2:
		switch (md->index) {
		case MD_SYS1:
			ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, CCISM_SHM_INIT, 0, 1);
			break;
		case MD_SYS3:
			ccci_send_msg_to_md(md, CCCI_CONTROL_TX, C2K_CCISM_SHM_INIT, 0, 1);
			break;
		};
		break;
	case MD_BOOT_STAGE_EXCEPTION:
		data = md->boot_stage;
		ccci_scp_ipi_send(md->index, CCCI_OP_MD_STATE, &data);
		break;
	default:
		break;
	};
}

static void ccci_scp_ipi_rx_work(struct work_struct *work)
{
	struct ccci_ipi_msg *ipi_msg_ptr;
	struct ccci_modem *md, *md3;
	struct sk_buff *skb = NULL;
	int data;

	while (!skb_queue_empty(&scp_ipi_rx_skb_list.skb_list)) {
		skb = ccci_skb_dequeue(&scp_ipi_rx_skb_list);
		ipi_msg_ptr = (struct ccci_ipi_msg *)skb->data;
		md = ccci_get_modem_by_id(ipi_msg_ptr->md_id);
		if (!md) {
			CCCI_ERROR_LOG(ipi_msg_ptr->md_id, CORE, "MD not exist\n");
			return;
		}
		switch (ipi_msg_ptr->op_id) {
		case CCCI_OP_SCP_STATE:
			switch (ipi_msg_ptr->data[0]) {
			case SCP_CCCI_STATE_BOOTING:
				if (scp_state == SCP_CCCI_STATE_RBREADY) {
					CCCI_NORMAL_LOG(md->index, CORE, "SCP reset detected\n");
					ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, CCISM_SHM_INIT, 0, 1);
					md3 = ccci_get_modem_by_id(MD_SYS3);
					if (md3)
						ccci_send_msg_to_md(md3, CCCI_CONTROL_TX,
									C2K_CCISM_SHM_INIT, 0, 1);
				} else {
					CCCI_NORMAL_LOG(md->index, CORE, "SCP boot up\n");
				}
				/* too early to init share memory here, EMI MPU may not be ready yet */
				break;
			case SCP_CCCI_STATE_RBREADY:
				switch (md->index) {
				case MD_SYS1:
					ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, CCISM_SHM_INIT_DONE, 0, 1);
					break;
				case MD_SYS3:
					ccci_send_msg_to_md(md, CCCI_CONTROL_TX, C2K_CCISM_SHM_INIT_DONE, 0, 1);
					break;
				};
				data = md->boot_stage;
				ccci_scp_ipi_send(md->index, CCCI_OP_MD_STATE, &data);
				break;
			default:
				break;
			};
			scp_state = ipi_msg_ptr->data[0];
			break;
		};
		ccci_free_skb(skb, FREE);
	}
}

static void ccci_scp_ipi_handler(int id, void *data, unsigned int len)
{
	struct ccci_ipi_msg *ipi_msg_ptr = (struct ccci_ipi_msg *)data;
	struct sk_buff *skb = NULL;

	if (len != sizeof(struct ccci_ipi_msg)) {
		CCCI_ERROR_LOG(-1, CORE, "IPI handler, data length wrong %d vs. %ld\n", len,
						sizeof(struct ccci_ipi_msg));
		return;
	}
	CCCI_NORMAL_LOG(ipi_msg_ptr->md_id, CORE, "IPI handler %d/0x%x, %d\n",
				ipi_msg_ptr->op_id, ipi_msg_ptr->data[0], len);

	skb = ccci_alloc_skb(len, 0, 0);
	if (!skb)
		return;
	memcpy(skb_put(skb, len), data, len);
	ccci_skb_enqueue(&scp_ipi_rx_skb_list, skb);
	schedule_work(&scp_ipi_rx_work); /* ipi_send use mutex, can not be called from ISR context */
}

static void ccci_scp_init(void)
{
	int ret;

	mutex_init(&scp_ipi_tx_mutex);
	ret = scp_ipi_registration(IPI_APCCCI, ccci_scp_ipi_handler, "AP CCCI");
	CCCI_INIT_LOG(-1, CORE, "register IPI %d %d\n", IPI_APCCCI, ret);
	INIT_WORK(&scp_ipi_rx_work, ccci_scp_ipi_rx_work);
	init_waitqueue_head(&scp_ipi_rx_wq);
	ccci_skb_queue_init(&scp_ipi_rx_skb_list, 16, 16, 0);
}

int ccci_scp_ipi_send(int md_id, int op_id, void *data)
{
	int ret = 0;

	if (scp_state == SCP_CCCI_STATE_INVALID) {
		CCCI_ERROR_LOG(md_id, CORE, "ignore IPI %d, SCP state %d!\n", op_id, scp_state);
		return -CCCI_ERR_MD_NOT_READY;
	}

	mutex_lock(&scp_ipi_tx_mutex);
	memset(&scp_ipi_tx_msg, 0, sizeof(scp_ipi_tx_msg));
	scp_ipi_tx_msg.md_id = md_id;
	scp_ipi_tx_msg.op_id = op_id;
	scp_ipi_tx_msg.data[0] = *((u32 *)data);
	CCCI_NORMAL_LOG(scp_ipi_tx_msg.md_id, CORE, "IPI send %d/0x%x, %ld\n",
				scp_ipi_tx_msg.op_id, scp_ipi_tx_msg.data[0], sizeof(struct ccci_ipi_msg));
	if (scp_ipi_send(IPI_APCCCI, &scp_ipi_tx_msg, sizeof(scp_ipi_tx_msg), 1) != DONE) {
		CCCI_ERROR_LOG(md_id, CORE, "IPI send fail!\n");
		ret = -CCCI_ERR_MD_NOT_READY;
	}
	mutex_unlock(&scp_ipi_tx_mutex);
	return ret;
}
#endif

void ccci_update_md_boot_stage(struct ccci_modem *md, MD_BOOT_STAGE stage)
{
#ifdef FEATURE_SCP_CCCI_SUPPORT
	int data;
#endif

	switch (stage) {
	case MD_BOOT_STAGE_0:
	case MD_BOOT_STAGE_1:
	case MD_BOOT_STAGE_2:
	case MD_BOOT_STAGE_EXCEPTION:
		md->boot_stage = stage;
#ifdef FEATURE_SCP_CCCI_SUPPORT
		schedule_work(&md->scp_md_state_sync_work);
#endif
		break;
#ifdef FEATURE_SCP_CCCI_SUPPORT
	case MD_ACK_SCP_INIT: /* in port kernel thread context, safe to send IPI */
		data = md->smem_layout.ccci_ccism_smem_base_phy;
		ccci_scp_ipi_send(md->index, CCCI_OP_SHM_INIT, &data);
		break;
#endif
	default:
		break;
	};
}

/* ------------------------------------------------------------------------- */
static int __init ccci_init(void)
{
	CCCI_INIT_LOG(-1, CORE, "ccci core init\n");
	dev_class = class_create(THIS_MODULE, "ccci_node");
	/* init common sub-system */
	/* ccci_subsys_sysfs_init(); */
	ccci_subsys_bm_init();
	ccci_plat_common_init();
	ccci_subsys_kernel_init();
#ifdef FEATURE_SCP_CCCI_SUPPORT
	ccci_scp_init();
#endif
	return 0;
}

/* setup function is only for data structure initialization */
struct ccci_modem *ccci_allocate_modem(int private_size)
{
	struct ccci_modem *md = kzalloc(sizeof(struct ccci_modem), GFP_KERNEL);
	int i;

	if (!md) {
		CCCI_ERROR_LOG(-1, CORE, "fail to allocate memory for modem structure\n");
		goto out;
	}

	md->private_data = kzalloc(private_size, GFP_KERNEL);
	md->sim_type = 0xEEEEEEEE;	/* sim_type(MCC/MNC) sent by MD wouldn't be 0xEEEEEEEE */
	md->config.setting |= MD_SETTING_FIRST_BOOT;
	md->md_state = INVALID;
	md->boot_stage = MD_BOOT_STAGE_0;
	md->flight_mode = MD_FIGHT_MODE_NONE; /* leave flight mode */
	md->ex_stage = EX_NONE;
	atomic_set(&md->wakeup_src, 0);
	INIT_LIST_HEAD(&md->entry);
	ccci_reset_seq_num(md);

	init_timer(&md->bootup_timer);
	md->bootup_timer.function = md_bootup_timeout_func;
	md->bootup_timer.data = (unsigned long)md;
	init_timer(&md->ex_monitor);
	md->ex_monitor.function = md_ex_monitor_func;
	md->ex_monitor.data = (unsigned long)md;
	init_timer(&md->ex_monitor2);
	md->ex_monitor2.function = md_ex_monitor2_func;
	md->ex_monitor2.data = (unsigned long)md;
	init_timer(&md->md_status_poller);
	md->md_status_poller.function = md_status_poller_func;
	md->md_status_poller.data = (unsigned long)md;
	init_timer(&md->md_status_timeout);
	md->md_status_timeout.function = md_status_timeout_func;
	md->md_status_timeout.data = (unsigned long)md;

	snprintf(md->md_wakelock_name, sizeof(md->md_wakelock_name), "md%d_wakelock", md->index + 1);
	wake_lock_init(&md->md_wake_lock, WAKE_LOCK_SUSPEND, md->md_wakelock_name);

	spin_lock_init(&md->ctrl_lock);
	for (i = 0; i < ARRAY_SIZE(md->rx_ch_ports); i++)
		INIT_LIST_HEAD(&md->rx_ch_ports[i]);
#ifdef FEATURE_SCP_CCCI_SUPPORT
	INIT_WORK(&md->scp_md_state_sync_work, scp_md_state_sync_work);
#endif
 out:
	return md;
}
EXPORT_SYMBOL(ccci_allocate_modem);

int ccci_register_modem(struct ccci_modem *modem)
{
	int ret;
	/* init per-modem sub-system */
	ccci_subsys_char_init(modem);
	CCCI_INIT_LOG(modem->index, CORE, "register modem %d\n", modem->major);
	md_port_cfg(modem);
	/* init modem */
	/* TODO: check modem->ops for all must-have functions */
	ret = modem->ops->init(modem);
	if (ret < 0)
		return ret;
	ccci_config_modem(modem);
	md_smem_port_cfg(modem); /* must be after modem config to get smem layout */
	list_add_tail(&modem->entry, &modem_list);
	ccci_sysfs_add_modem(modem->index, (void *)&modem->kobj, (void *)&ccci_md_ktype, boot_md_show, boot_md_store);
	ccci_platform_init(modem);
	return 0;
}
EXPORT_SYMBOL(ccci_register_modem);

struct ccci_modem *ccci_get_modem_by_id(int md_id)
{
	struct ccci_modem *md = NULL;

	list_for_each_entry(md, &modem_list, entry) {
		if (md->index == md_id)
			return md;
	}
	return NULL;
}

struct ccci_modem *ccci_get_another_modem(int md_id)
{
	struct ccci_modem *another_md = NULL;

	if (md_id == MD_SYS1 && get_modem_is_enabled(MD_SYS3))
		another_md = ccci_get_modem_by_id(MD_SYS3);

	if (md_id == MD_SYS3 && get_modem_is_enabled(MD_SYS1))
		another_md = ccci_get_modem_by_id(MD_SYS1);

	return another_md;
}

int ccci_get_modem_state(int md_id)
{
	struct ccci_modem *md = NULL;

	list_for_each_entry(md, &modem_list, entry) {
		if (md->index == md_id)
			return md->md_state;
	}
	return -CCCI_ERR_MD_INDEX_NOT_FOUND;
}

int exec_ccci_kern_func_by_md_id(int md_id, unsigned int id, char *buf, unsigned int len)
{
	struct ccci_modem *md = NULL;
	int ret = 0;

	list_for_each_entry(md, &modem_list, entry) {
		if (md->index == md_id) {
			ret = 1;
			break;
		}
	}
	if (!ret)
		return -CCCI_ERR_MD_INDEX_NOT_FOUND;

	CCCI_DEBUG_LOG(md->index, CORE, "%ps execuste function %d\n", __builtin_return_address(0), id);
	switch (id) {
	case ID_GET_MD_WAKEUP_SRC:
		atomic_set(&md->wakeup_src, 1);
		break;
	case ID_GET_TXPOWER:
		if (buf[0] == 0)
			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_TX_POWER, 0, 0);
		else if (buf[0] == 1)
			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_RF_TEMPERATURE, 0, 0);
		else if (buf[0] == 2)
			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_RF_TEMPERATURE_3G, 0, 0);
		break;
	case ID_FORCE_MD_ASSERT:
		CCCI_NORMAL_LOG(md->index, CHAR, "Force MD assert called by %s\n", current->comm);
		ret = md->ops->force_assert(md, CCIF_INTERRUPT);
		break;
#ifdef MD_UMOLY_EE_SUPPORT
	case ID_MD_MPU_ASSERT:
		if (md->index == MD_SYS1) {
			if (buf != NULL && strlen(buf)) {
				CCCI_NORMAL_LOG(md->index, CHAR, "Force MD assert(MPU) called by %s\n", current->comm);
				snprintf(md->ex_mpu_string, MD_EX_MPU_STR_LEN, "EMI MPU VIOLATION: %s", buf);
				ret = md->ops->force_assert(md, CCIF_MPU_INTR);
			} else {
				CCCI_NORMAL_LOG(md->index, CHAR, "ACK (MPU violation) called by %s\n", current->comm);
				ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_AP_MPU_ACK_MD, 0, 0);
			}
		} else
			CCCI_NORMAL_LOG(md->index, CHAR, "MD%d MPU API called by %s\n", md->index, current->comm);
		break;
#endif
	case ID_PAUSE_LTE:
		/*
		 * MD booting/flight mode/exception mode: return >0 to DVFS.
		 * MD ready: return 0 if message delivered, return <0 if get error.
		 * DVFS will call this API with IRQ disabled.
		 */
		if (md->md_state != READY)
			ret = 1;
		else {
			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_PAUSE_LTE, *((int *)buf), 1);
			if (ret == -CCCI_ERR_MD_NOT_READY || ret == -CCCI_ERR_HIF_NOT_POWER_ON)
				ret = 1;
		}
		break;
	case ID_STORE_SIM_SWITCH_MODE:
		{
			int simmode = *((int *)buf);

			ccci_store_sim_switch_mode(md, simmode);
		}
		break;
	case ID_GET_SIM_SWITCH_MODE:
		{
			int simmode = ccci_get_sim_switch_mode();

			memcpy(buf, &simmode, sizeof(int));
		}
		break;
	case ID_GET_MD_STATE:
		ret = md->boot_stage;
		break;
		/* used for throttling feature - start */
	case ID_THROTTLING_CFG:
		ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_THROTTLING, *((int *)buf), 1);
		break;
		/* used for throttling feature - end */
#if defined(FEATURE_MTK_SWITCH_TX_POWER)
	case ID_UPDATE_TX_POWER:
		{
			unsigned int msg_id = (md_id == 0) ? MD_SW_MD1_TX_POWER : MD_SW_MD2_TX_POWER;
			unsigned int mode = *((unsigned int *)buf);

			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, msg_id, mode, 0);
		}
		break;
#endif
	case ID_RESET_MD:
		CCCI_NOTICE_LOG(md->index, CHAR, "MD reset API called by %ps\n", __builtin_return_address(0));
		ret = md->ops->reset(md);
		if (ret == 0)
			ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_RESET, 0);
		break;
	case ID_DUMP_MD_SLEEP_MODE:
		md->ops->dump_info(md, DUMP_FLAG_SMEM_MDSLP, NULL, 0);
		break;
	case ID_PMIC_INTR:
		ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, PMIC_INTR_MODEM_BUCK_OC, *((int *)buf), 1);
		break;
	case ID_UPDATE_MD_BOOT_MODE:
		if (*((unsigned int *)buf) > MD_BOOT_MODE_INVALID && *((unsigned int *)buf) < MD_BOOT_MODE_MAX)
			md->md_boot_mode = *((unsigned int *)buf);
		break;
	default:
		ret = -CCCI_ERR_FUNC_ID_ERROR;
		break;
	};
	return ret;
}

int aee_dump_ccci_debug_info(int md_id, void **addr, int *size)
{
	struct ccci_modem *md = NULL;
	int ret = 0;

	md_id--;		/* EE string use 1 and 2, not 0 and 1 */

	list_for_each_entry(md, &modem_list, entry) {
		if (md->index == md_id) {
			ret = 1;
			break;
		}
	}
	if (!ret)
		return -CCCI_ERR_MD_INDEX_NOT_FOUND;

	*addr = md->smem_layout.ccci_exp_smem_ccci_debug_vir;
	*size = md->smem_layout.ccci_exp_smem_ccci_debug_size;
	return 0;
}

struct ccci_port *ccci_get_port_for_node(int major, int minor)
{
	struct ccci_modem *md = NULL;
	struct ccci_port *port = NULL;

	list_for_each_entry(md, &modem_list, entry) {
		if (md->major == major) {
			port = md->ops->get_port_by_minor(md, minor);
			break;
		}
	}
	return port;
}

int ccci_register_dev_node(const char *name, int major_id, int minor)
{
	int ret = 0;
	dev_t dev_n;
	struct device *dev;

	dev_n = MKDEV(major_id, minor);
	dev = device_create(dev_class, NULL, dev_n, NULL, "%s", name);

	if (IS_ERR(dev))
		ret = PTR_ERR(dev);

	return ret;
}

/*
 * kernel inject CCCI message to modem.
 */
int ccci_send_msg_to_md(struct ccci_modem *md, CCCI_CH ch, u32 msg, u32 resv, int blocking)
{
	struct ccci_port *port = NULL;
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h;
	int ret;

	if (md->md_state != BOOTING && md->md_state != READY && md->md_state != EXCEPTION)
		return -CCCI_ERR_MD_NOT_READY;
	if (ch == CCCI_SYSTEM_TX && md->md_state != READY)
		return -CCCI_ERR_MD_NOT_READY;
	if ((msg == CCISM_SHM_INIT || msg == CCISM_SHM_INIT_DONE ||
		msg == C2K_CCISM_SHM_INIT || msg == C2K_CCISM_SHM_INIT_DONE) &&
		md->md_state != READY) {
		return -CCCI_ERR_MD_NOT_READY;
	}

	port = md->ops->get_port_by_channel(md, ch);
	if (port) {
		if (!blocking)
			req = ccci_alloc_req(OUT, sizeof(struct ccci_header), 0, 0);
		else
			req = ccci_alloc_req(OUT, sizeof(struct ccci_header), 1, 1);
		if (req) {
			req->policy = RECYCLE;
			ccci_h = (struct ccci_header *)skb_put(req->skb, sizeof(struct ccci_header));
			ccci_h->data[0] = CCCI_MAGIC_NUM;
			ccci_h->data[1] = msg;
			ccci_h->channel = ch;
			ccci_h->reserved = resv;
			ret = ccci_port_send_request(port, req);
			if (ret)
				ccci_free_req(req);
			return ret;
		} else {
			return -CCCI_ERR_ALLOCATE_MEMORY_FAIL;
		}
	}
	return -CCCI_ERR_INVALID_LOGIC_CHANNEL_ID;
}

int ccci_set_md_boot_data(struct ccci_modem *md, unsigned int data[], int len)
{
	int ret = 0;

	if (len > 0 && data != NULL)
		md->mdlg_mode = data[0];
	else
		ret = -1;
	return ret;
}


/*
 * kernel inject message to user space daemon, this function may sleep
 */
int ccci_send_virtual_md_msg(struct ccci_modem *md, CCCI_CH ch, CCCI_MD_MSG msg, u32 resv)
{
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h;
	int ret = 0, count = 0;

	if (unlikely(ch != CCCI_MONITOR_CH)) {
		CCCI_ERROR_LOG(md->index, CORE, "invalid channel %x for sending virtual msg\n", ch);
		return -CCCI_ERR_INVALID_LOGIC_CHANNEL_ID;
	}
	if (unlikely(in_interrupt())) {
		CCCI_ERROR_LOG(md->index, CORE, "sending virtual msg from IRQ context %ps\n",
			     __builtin_return_address(0));
		return -CCCI_ERR_ASSERT_ERR;
	}

	req = ccci_alloc_req(IN, sizeof(struct ccci_header), 1, 0);
	/* request will be recycled in char device's read function */
	ccci_h = (struct ccci_header *)skb_put(req->skb, sizeof(struct ccci_header));
	ccci_h->data[0] = CCCI_MAGIC_NUM;
	ccci_h->data[1] = msg;
	ccci_h->channel = ch;
#ifdef FEATURE_SEQ_CHECK_EN
	ccci_h->assert_bit = 0;
#endif
	ccci_h->reserved = resv;
	INIT_LIST_HEAD(&req->entry);	/* as port will run list_del */
 retry:
	ret = ccci_port_recv_request(md, req, NULL);
	if (ret >= 0 || ret == -CCCI_ERR_DROP_PACKET)
		return ret;
	msleep(100);
	if (count++ < 20) {
		goto retry;
	} else {
		CCCI_ERROR_LOG(md->index, CORE, "fail to send virtual msg %x for %ps\n", msg,
			     __builtin_return_address(0));
		list_del(&req->entry);
		req->policy = RECYCLE;
		ccci_free_req(req);
	}
	return ret;
}

#if defined(FEATURE_MTK_SWITCH_TX_POWER)
static int switch_Tx_Power(int md_id, unsigned int mode)
{
	int ret = 0;
	unsigned int resv = mode;

	ret = exec_ccci_kern_func_by_md_id(md_id, ID_UPDATE_TX_POWER, (char *)&resv, sizeof(resv));

	pr_debug("[swtp] switch_MD%d_Tx_Power(%d): ret[%d]\n", md_id + 1, resv, ret);

	CCCI_DEBUG_LOG(md_id, "ctl", "switch_MD%d_Tx_Power(%d): %d\n", md_id + 1, resv, ret);

	return ret;
}

int switch_MD1_Tx_Power(unsigned int mode)
{
	return switch_Tx_Power(0, mode);
}
EXPORT_SYMBOL(switch_MD1_Tx_Power);

int switch_MD2_Tx_Power(unsigned int mode)
{
	return switch_Tx_Power(1, mode);
}
EXPORT_SYMBOL(switch_MD2_Tx_Power);
#endif

subsys_initcall(ccci_init);

MODULE_AUTHOR("Xiao Wang <xiao.wang@mediatek.com>");
MODULE_DESCRIPTION("Unified CCCI driver v0.1");
MODULE_LICENSE("GPL");
