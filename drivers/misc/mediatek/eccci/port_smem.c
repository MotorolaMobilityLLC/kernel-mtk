#include "port_smem.h"

/* FIXME, global structures are indexed by SMEM_USER_ID */
static struct ccci_smem_port md1_ccci_smem_ports[] = {
	{SMEM_USER_RAW_DBM, TYPE_RAW, }, /* mt_pbm.c */
	{SMEM_USER_CCB_DHL, TYPE_CCB, }, /* CCB DHL */
	{SMEM_USER_RAW_DHL, TYPE_RAW, }, /* raw region for DHL settings */
	{SMEM_USER_RAW_NETD, TYPE_RAW, }, /* for direct tethering */
	{SMEM_USER_RAW_USB, TYPE_RAW, }, /* for derect tethering */
};
static struct ccci_smem_port md3_ccci_smem_ports[] = {
	{SMEM_USER_RAW_DBM, TYPE_RAW, }, /* mt_pbm.c */
};
struct tx_notify_task md1_tx_notify_tasks[SMEM_USER_MAX];

static enum hrtimer_restart smem_tx_timer_func(struct hrtimer *timer)
{
	struct tx_notify_task *notify_task = container_of(timer, struct tx_notify_task, notify_timer);

	notify_task->port->modem->ops->send_ccb_tx_notify(notify_task->port->modem, notify_task->core_id);
	return HRTIMER_NORESTART;
}

int port_smem_init(struct ccci_port *port)
{
	/* FIXME, port->minor is indexed by SMEM_USER_ID */
	switch (port->modem->index) {
	case MD_SYS1:
		port->private_data = &(md1_ccci_smem_ports[port->minor]);
		md1_ccci_smem_ports[port->minor].port = port;
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
		port->modem->ops->send_ccb_tx_notify(port->modem, user_data);
		md1_tx_notify_tasks[smem_port->user_id].core_id = user_data;
		md1_tx_notify_tasks[smem_port->user_id].port = port;
		hrtimer_start(&(md1_tx_notify_tasks[smem_port->user_id].notify_timer),
				ktime_set(0, 1000000), HRTIMER_MODE_REL);
	}
	return 0;
}

int port_smem_rx_poll(struct ccci_port *port)
{
	struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;
	int ret;

	if (smem_port->type != TYPE_CCB)
		return -EFAULT;
	ret = wait_event_interruptible_exclusive(smem_port->rx_wq, smem_port->wakeup != 0);
	smem_port->wakeup = 0;
	if (ret == -ERESTARTSYS)
		ret = -EINTR;
	return ret;
}

int port_smem_rx_wakeup(struct ccci_port *port)
{
	struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;

	if (smem_port->type != TYPE_CCB)
		return -EFAULT;
	smem_port->wakeup = 1;
	wake_up_all(&smem_port->rx_wq);
	return 0;
}

int md_smem_port_cfg(struct ccci_modem *md)
{
	int i;

	switch (md->index) {
	case MD_SYS1:
#ifdef FEATURE_DBM_SUPPORT
		md1_ccci_smem_ports[SMEM_USER_RAW_DBM].addr_vir = md->smem_layout.ccci_exp_smem_dbm_debug_vir +
								CCCI_SMEM_DBM_GUARD_SIZE;
		md1_ccci_smem_ports[SMEM_USER_RAW_DBM].length = CCCI_SMEM_DBM_SIZE;
#endif

		md1_ccci_smem_ports[SMEM_USER_CCB_DHL].addr_vir = md->smem_layout.ccci_ccb_dhl_base_vir;
		md1_ccci_smem_ports[SMEM_USER_CCB_DHL].addr_phy = md->smem_layout.ccci_ccb_dhl_base_phy;
		md1_ccci_smem_ports[SMEM_USER_CCB_DHL].length = md->smem_layout.ccci_ccb_dhl_size;

		md1_ccci_smem_ports[SMEM_USER_RAW_DHL].addr_vir = md->smem_layout.ccci_raw_dhl_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_DHL].addr_phy = md->smem_layout.ccci_raw_dhl_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_DHL].length = md->smem_layout.ccci_raw_dhl_size;

		md1_ccci_smem_ports[SMEM_USER_RAW_NETD].addr_vir = md->smem_layout.ccci_dt_netd_smem_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_NETD].addr_phy = md->smem_layout.ccci_dt_netd_smem_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_NETD].length = md->smem_layout.ccci_dt_netd_smem_size;

		md1_ccci_smem_ports[SMEM_USER_RAW_USB].addr_vir = md->smem_layout.ccci_dt_usb_smem_base_vir;
		md1_ccci_smem_ports[SMEM_USER_RAW_USB].addr_phy = md->smem_layout.ccci_dt_usb_smem_base_phy;
		md1_ccci_smem_ports[SMEM_USER_RAW_USB].length = md->smem_layout.ccci_dt_usb_smem_size;

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
