#ifndef __PORT_SMEM_H__
#define __PORT_SMEM_H__

#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <mt-plat/mt_ccci_common.h>
#include "ccci_config.h"
#include "ccci_core.h"

#define CCCI_SMEM_MINOR_BASE 150

enum {
	TYPE_RAW = 0,
	TYPE_CCB,
};

enum {
	CCB_USER_INVALID = 0,
	CCB_USER_OK,
	CCB_USER_ERR,
};

struct tx_notify_task {
	struct hrtimer notify_timer;
	unsigned short core_id;
	struct ccci_port *port;
};

struct ccci_smem_port {
	SMEM_USER_ID user_id;
	unsigned char type;

	unsigned char state;
	unsigned char wakeup;
	phys_addr_t addr_phy;
	unsigned char *addr_vir;
	unsigned int length;
	struct ccci_port *port;
	wait_queue_head_t rx_wq;
};

int port_smem_init(struct ccci_port *port);
int port_smem_tx_nofity(struct ccci_port *port, unsigned int user_data);
int port_smem_rx_poll(struct ccci_port *port);
int port_smem_rx_wakeup(struct ccci_port *port);

#endif
