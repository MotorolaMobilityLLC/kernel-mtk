#ifndef __PORT_CFG_H__
#define __PORT_CFG_H__
#include "ccci_core.h"

/* Port mapping */
extern struct ccci_port_ops char_port_ops;
extern struct ccci_port_ops net_port_ops;
extern struct ccci_port_ops kernel_port_ops;
extern struct ccci_port_ops ipc_port_ack_ops;
extern struct ccci_port_ops ipc_kern_port_ops;

int md_port_cfg(struct ccci_modem *md);
#endif /* __PORT_CFG_H__ */
