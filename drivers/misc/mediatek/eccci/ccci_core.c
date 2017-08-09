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

	CCCI_INF_MSG(-1, CORE, "ccci core boot md%d\n", md_id + 1);
	list_for_each_entry(md, &modem_list, entry) {
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
				CCCI_ERR_MSG(md->index, CORE, "port->ops->recv_request is null\n");
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
		/* CCCI_ERR_MSG(md->index, CORE, "drop on channel %d\n", ccci_h->channel); */ /* Fix me, mask temp */
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
		CCCI_INF_MSG(md->index, CORE, "%08X %08X %08X %08X  %5lu.%06lu\n",
		       log->msg.data[0], log->msg.data[1], *(((u32 *)&log->msg) + 2),
		       log->msg.reserved, (unsigned long)ts_nsec, rem_nsec / 1000);
	} else {
		CCCI_INF_MSG(md->index, CORE, "%08X %08X %08X %08X  %5lu.%06lu -\n",
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
			CCCI_INF_MSG(md->index, CORE, "dump txq%d packet history, ptr=%d\n", i,
			       md->tx_history_ptr[i]);
			for (j = 0; j < PACKET_HISTORY_DEPTH; j++)
				ccci_dump_log_rec(md, &md->tx_history[i][j]);
		}
		for (i = 0; i < ((rx_queue_num <= MAX_RXQ_NUM) ? rx_queue_num : MAX_RXQ_NUM); i++) {
			CCCI_INF_MSG(md->index, CORE, "dump rxq%d packet history, ptr=%d\n", i,
			       md->rx_history_ptr[i]);
			for (j = 0; j < PACKET_HISTORY_DEPTH; j++)
				ccci_dump_log_rec(md, &md->rx_history[i][j]);
		}
	} else {
		CCCI_INF_MSG(md->index, CORE, "dump txq%d packet history, ptr=%d\n", tx_queue_num,
		       md->tx_history_ptr[tx_queue_num]);
		for (j = 0; j < PACKET_HISTORY_DEPTH; j++)
			ccci_dump_log_rec(md, &md->tx_history[tx_queue_num][j]);
		CCCI_INF_MSG(md->index, CORE, "dump rxq%d packet history, ptr=%d\n", rx_queue_num,
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
	memset_io(md->mem_layout.smem_region_vir, 0, md->mem_layout.smem_region_size);

	/* exception region */
	md->smem_layout.ccci_exp_smem_base_phy = md->mem_layout.smem_region_phy;
	md->smem_layout.ccci_exp_smem_base_vir = md->mem_layout.smem_region_vir;
	md->smem_layout.ccci_exp_smem_size = CCCI_SMEM_SIZE_EXCEPTION;
	md->smem_layout.ccci_exp_dump_size = CCCI_SMEM_DUMP_SIZE;
	/* dump region */
	md->smem_layout.ccci_exp_smem_ccci_debug_vir =
	    md->smem_layout.ccci_exp_smem_base_vir + CCCI_SMEM_OFFSET_CCCI_DEBUG;
	md->smem_layout.ccci_exp_smem_ccci_debug_size = CCCI_SMEM_CCCI_DEBUG_SIZE;
	md->smem_layout.ccci_exp_smem_mdss_debug_vir =
	    md->smem_layout.ccci_exp_smem_base_vir + CCCI_SMEM_OFFSET_MDSS_DEBUG;
#ifdef MD_UMOLY_EE_SUPPORT
	if (md->index == MD_SYS1)
		md->smem_layout.ccci_exp_smem_mdss_debug_size = CCCI_SMEM_MDSS_DEBUG_SIZE_UMOLY;
	else
#endif
		md->smem_layout.ccci_exp_smem_mdss_debug_size = CCCI_SMEM_MDSS_DEBUG_SIZE;
	md->smem_layout.ccci_exp_smem_sleep_debug_vir = md->smem_layout.ccci_exp_smem_base_vir +
		md->smem_layout.ccci_exp_smem_size - CCCI_SMEM_SLEEP_MODE_DBG_SIZE;
	md->smem_layout.ccci_exp_smem_sleep_debug_size = CCCI_SMEM_SLEEP_MODE_DBG_DUMP;

	/* exception record start address */
	md->smem_layout.ccci_exp_rec_base_vir = md->smem_layout.ccci_exp_smem_base_vir + CCCI_SMEM_OFFSET_EXREC;

	/*runtime region */
	md->smem_layout.ccci_rt_smem_base_phy = md->smem_layout.ccci_exp_smem_base_phy +
	    md->smem_layout.ccci_exp_smem_size;
	md->smem_layout.ccci_rt_smem_base_vir = md->smem_layout.ccci_exp_smem_base_vir +
	    md->smem_layout.ccci_exp_smem_size;
	md->smem_layout.ccci_rt_smem_size = CCCI_SMEM_SIZE_RUNTIME;

	/*md1 md3 shared memory region and remap*/
	get_md1_md3_resv_smem_info(md->index, &md->mem_layout.md1_md3_smem_phy,
		&md->mem_layout.md1_md3_smem_size);
	md->mem_layout.md1_md3_smem_vir =
	    ioremap_nocache(md->mem_layout.md1_md3_smem_phy, md->mem_layout.md1_md3_smem_size);
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

	CCCI_DBG_MSG(md->index, SYSFS, "md kobject release\n");
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

/* ------------------------------------------------------------------------- */
static int __init ccci_init(void)
{
	CCCI_INF_MSG(-1, CORE, "ccci core init\n");
	dev_class = class_create(THIS_MODULE, "ccci_node");
	/* init common sub-system */
	/* ccci_subsys_sysfs_init(); */
	ccci_subsys_bm_init();
	ccci_plat_common_init();
	ccci_subsys_kernel_init();

	return 0;
}

/* setup function is only for data structure initialization */
struct ccci_modem *ccci_allocate_modem(int private_size)
{
	struct ccci_modem *md = kzalloc(sizeof(struct ccci_modem), GFP_KERNEL);
	int i;

	if (!md) {
		CCCI_ERR_MSG(-1, CORE, "fail to allocate memory for modem structure\n");
		goto out;
	}

	md->private_data = kzalloc(private_size, GFP_KERNEL);
	md->sim_type = 0xEEEEEEEE;	/* sim_type(MCC/MNC) sent by MD wouldn't be 0xEEEEEEEE */
	md->config.setting |= MD_SETTING_FIRST_BOOT;
	md->md_state = INVALID;
	md->boot_stage = MD_BOOT_STAGE_0;
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

	spin_lock_init(&md->ctrl_lock);
	for (i = 0; i < ARRAY_SIZE(md->rx_ch_ports); i++)
		INIT_LIST_HEAD(&md->rx_ch_ports[i]);
 out:
	return md;
}
EXPORT_SYMBOL(ccci_allocate_modem);

int ccci_register_modem(struct ccci_modem *modem)
{
	int ret;
	/* init per-modem sub-system */
	ccci_subsys_char_init(modem);
	CCCI_INF_MSG(modem->index, CORE, "register modem %d\n", modem->major);
	md_port_cfg(modem);
	/* init modem */
	/* TODO: check modem->ops for all must-have functions */
	ret = modem->ops->init(modem);
	if (ret < 0)
		return ret;
	ccci_config_modem(modem);
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

	CCCI_DBG_MSG(md->index, CORE, "%ps execuste function %d\n", __builtin_return_address(0), id);
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
#if defined(CONFIG_MTK_SWITCH_TX_POWER)
	case ID_UPDATE_TX_POWER:
		{
			unsigned int msg_id = (md_id == 0) ? MD_SW_MD1_TX_POWER : MD_SW_MD2_TX_POWER;
			unsigned int mode = *((unsigned int *)buf);

			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, msg_id, mode, 0);
		}
		break;
#endif
	case ID_RESET_MD:
		CCCI_INF_MSG(md->index, CHAR, "MD reset API called by %ps\n", __builtin_return_address(0));
		ret = md->ops->reset(md);
		if (ret == 0)
			ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_RESET, 0);
		break;
	case ID_DUMP_MD_SLEEP_MODE:
		md->ops->dump_info(md, DUMP_FLAG_SMEM_MDSLP, NULL, 0);
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
int ccci_send_msg_to_md(struct ccci_modem *md, CCCI_CH ch, CCCI_MD_MSG msg, u32 resv, int blocking)
{
	struct ccci_port *port = NULL;
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h;
	int ret;

	if (md->md_state != BOOTING && md->md_state != READY && md->md_state != EXCEPTION)
		return -CCCI_ERR_MD_NOT_READY;
	if (ch == CCCI_SYSTEM_TX && md->md_state != READY)
		return -CCCI_ERR_MD_NOT_READY;

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

/*
 * kernel inject message to user space daemon, this function may sleep
 */
int ccci_send_virtual_md_msg(struct ccci_modem *md, CCCI_CH ch, CCCI_MD_MSG msg, u32 resv)
{
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h;
	int ret = 0, count = 0;

	if (unlikely(ch != CCCI_MONITOR_CH)) {
		CCCI_ERR_MSG(md->index, CORE, "invalid channel %x for sending virtual msg\n", ch);
		return -CCCI_ERR_INVALID_LOGIC_CHANNEL_ID;
	}
	if (unlikely(in_interrupt())) {
		CCCI_ERR_MSG(md->index, CORE, "sending virtual msg from IRQ context %ps\n",
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
		CCCI_ERR_MSG(md->index, CORE, "fail to send virtual msg %x for %ps\n", msg,
			     __builtin_return_address(0));
		list_del(&req->entry);
		req->policy = RECYCLE;
		ccci_free_req(req);
	}
	return ret;
}

#if defined(CONFIG_MTK_SWITCH_TX_POWER)
static int switch_Tx_Power(int md_id, unsigned int mode)
{
	int ret = 0;
	unsigned int resv = mode;
	unsigned int msg_id = (md_id == 0) ? MD_SW_MD1_TX_POWER : MD_SW_MD2_TX_POWER;

#if 1
	ret = exec_ccci_kern_func_by_md_id(md_id, ID_UPDATE_TX_POWER, (char *)&resv, sizeof(resv));
#else
	ret = ccci_send_msg_to_md(md_id, CCCI_SYSTEM_TX, msg_id, resv, 0);
#endif
	pr_debug("[swtp] switch_MD%d_Tx_Power(%d): ret[%d]\n", md_id + 1, resv, ret);

	CCCI_DBG_MSG(md_id, "ctl", "switch_MD%d_Tx_Power(%d): %d\n", md_id + 1, resv, ret);

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

int register_smem_sub_region_mem_func(int md_id, smem_sub_region_cb_t pfunc, int region_id)
{
	struct ccci_modem *md = NULL;
	int ret = 0;

	list_for_each_entry(md, &modem_list, entry) {
		if (md->index == md_id) {
			ret = 1;
			break;
		}
	}
	if (ret) {
		if (region_id < SMEM_SUB_REGION_MAX) {
			md->sub_region_cb_tbl[region_id] = pfunc;
			CCCI_INF_MSG(md_id, CORE, "region%d call back %p register success\n", region_id, pfunc);
			return 0;
		}

		CCCI_INF_MSG(md_id, CORE, "sub_region invalid %d\n", region_id);
		return -2;
	}

	CCCI_INF_MSG(md_id, CORE, "md id invalid %d\n", md_id);
	return -1;
}

void __iomem *get_smem_start_addr(int md_id, int region_id, int *size_o)
{
	struct ccci_modem *md = NULL;
	int ret = 0;

	list_for_each_entry(md, &modem_list, entry) {
		if (md->index == md_id) {
			ret = 1;
			break;
		}
	}

	if (!ret) {
		CCCI_INF_MSG(md_id, CORE, "md%d not support\n", md_id+1);
		return NULL;
	}

	if (region_id >= SMEM_SUB_REGION_MAX) {
		CCCI_INF_MSG(md_id, CORE, "region id%d not support\n", region_id);
		return NULL;
	}

	if (md->sub_region_cb_tbl[region_id] == NULL) {
		CCCI_INF_MSG(md_id, CORE, "region%d call back not register\n", region_id);
		return NULL;
	}

	return md->sub_region_cb_tbl[region_id](md, size_o);
}

subsys_initcall(ccci_init);

MODULE_AUTHOR("Xiao Wang <xiao.wang@mediatek.com>");
MODULE_DESCRIPTION("Unified CCCI driver v0.1");
MODULE_LICENSE("GPL");
