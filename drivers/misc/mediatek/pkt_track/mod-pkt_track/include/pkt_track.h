#ifndef __PKT_TRACK_H__
#define __PKT_TRACK_H__

#if defined(CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT)

#include "port_ipc.h"
#include "ccci_ipc_task_ID.h"
#include "ccci_ipc_msg_id.h"
#include <linux/usb/composite.h>
#include "mtk_gadget.h"

#define PKT_TRACK_MDT_MAGIC_CODE 0xA8
#define PKT_TRACK_POLLING_MD_TIMER 1000
#define NETLINK_ENABLE

/* MD to AP CCCI_IPC callback function*/
int pkt_track_md_msg_hdlr(ipc_ilm_t *ilm);

/* AP to MD CCCI_IPC control msg API for USB gadget driver */
bool pkt_track_enable_md_fast_path(ufpm_enable_md_func_req_t *req);
bool pkt_track_disable_md_fast_path(ufpm_md_fast_path_common_req_t *req);
bool pkt_track_activate_md_fast_path(ufpm_activate_md_func_req_t *req);
bool pkt_track_deactivate_md_fast_path(ufpm_md_fast_path_common_req_t *req);

#endif /* CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT */

#endif /* __PKT_TRACK_H__ */
