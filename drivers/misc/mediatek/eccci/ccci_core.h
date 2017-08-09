#ifndef __CCCI_CORE_H__
#define __CCCI_CORE_H__

#include <linux/wait.h>
#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <mt-plat/mt_ccci_common.h>
#include "ccci_debug.h"

#define CCCI_DEV_NAME "ccci"
#define CCCI_MAGIC_NUM 0xFFFFFFFF
#define MAX_TXQ_NUM 8
#define MAX_RXQ_NUM 8
#define PACKET_HISTORY_DEPTH 16	/* must be power of 2 */

struct ccci_log {
	struct ccci_header msg;
	u64 tv;
	int droped;
};

struct ccci_md_attribute {
	struct attribute attr;
	struct ccci_modem *modem;
	 ssize_t (*show)(struct ccci_modem *md, char *buf);
	 ssize_t (*store)(struct ccci_modem *md, const char *buf, size_t count);
};

#define CCCI_MD_ATTR(_modem, _name, _mode, _show, _store)	\
static struct ccci_md_attribute ccci_md_attr_##_name = {	\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.modem = _modem,					\
	.show = _show,						\
	.store = _store,					\
}

/* enumerations and marcos */

typedef enum {
	MD_BOOT_STAGE_0 = 0,
	MD_BOOT_STAGE_1 = 1,
	MD_BOOT_STAGE_2 = 2,
	MD_BOOT_STAGE_EXCEPTION = 3
} MD_BOOT_STAGE;		/* for other module */

typedef enum {
	EX_NONE = 0,
	EX_INIT,
	EX_DHL_DL_RDY,
	EX_INIT_DONE,
	/* internal use */
	MD_NO_RESPONSE,
	MD_WDT,
} MD_EX_STAGE;

/* MODEM MAUI Exception header (4 bytes)*/
typedef struct _exception_record_header_t {
	u8 ex_type;
	u8 ex_nvram;
	u16 ex_serial_num;
} __packed EX_HEADER_T;

/* MODEM MAUI Environment information (164 bytes) */
typedef struct _ex_environment_info_t {
	u8 boot_mode;		/* offset: +0x10 */
	u8 reserved1[8];
	u8 execution_unit[8];
	u8 status;		/* offset: +0x21, length: 1 */
	u8 ELM_status;		/* offset: +0x22, length: 1 */
	u8 reserved2[145];
} __packed EX_ENVINFO_T;

/* MODEM MAUI Special for fatal error (8 bytes)*/
typedef struct _ex_fatalerror_code_t {
	u32 code1;
	u32 code2;
} __packed EX_FATALERR_CODE_T;

/* MODEM MAUI fatal error (296 bytes)*/
typedef struct _ex_fatalerror_t {
	EX_FATALERR_CODE_T error_code;
	u8 reserved1[288];
} __packed EX_FATALERR_T;

/* MODEM MAUI Assert fail (296 bytes)*/
typedef struct _ex_assert_fail_t {
	u8 filename[24];
	u32 linenumber;
	u32 parameters[3];
	u8 reserved1[256];
} __packed EX_ASSERTFAIL_T;

/* MODEM MAUI Globally exported data structure (300 bytes) */
typedef union {
	EX_FATALERR_T fatalerr;
	EX_ASSERTFAIL_T assert;
} __packed EX_CONTENT_T;

/* MODEM MAUI Standard structure of an exception log ( */
typedef struct _ex_exception_log_t {
	EX_HEADER_T header;
	u8 reserved1[12];
	EX_ENVINFO_T envinfo;
	u8 reserved2[36];
	EX_CONTENT_T content;
} __packed EX_LOG_T;

typedef struct _ccci_msg {
	union {
		u32 magic;	/* For mail box magic number */
		u32 addr;	/* For stream start addr */
		u32 data0;	/* For ccci common data[0] */
	};
	union {
		u32 id;		/* For mail box message id */
		u32 len;	/* For stream len */
		u32 data1;	/* For ccci common data[1] */
	};
	u32 channel;
	u32 reserved;
} __packed ccci_msg_t;

typedef struct dump_debug_info {
	unsigned int type;
	char *name;
	unsigned int more_info;
	union {
		struct {
			char file_name[30];
			int line_num;
			unsigned int parameters[3];
		} assert;
		struct {
			int err_code1;
			int err_code2;
			char offender[9];
		} fatal_error;
		ccci_msg_t data;
		struct {
			unsigned char execution_unit[9];	/* 8+1 */
			char file_name[30];
			int line_num;
			unsigned int parameters[3];
		} dsp_assert;
		struct {
			unsigned char execution_unit[9];
			unsigned int code1;
		} dsp_exception;
		struct {
			unsigned char execution_unit[9];
			unsigned int err_code[2];
		} dsp_fatal_err;
	};
	void *ext_mem;
	size_t ext_size;
	void *md_image;
	size_t md_size;
	void *platform_data;
	void (*platform_call)(void *data);
} DEBUG_INFO_T;

typedef enum {
	IDLE = 0,		/* update by buffer manager */
	FLYING,			/* update by buffer manager */
	PARTIAL_READ,		/* update by port_char */
	ERROR,			/* not using */
} REQ_STATE;

typedef enum {
	IN = 0,
	OUT
} DIRECTION;

/*
 * This tells request free routine how it handles skb.
 * The CCCI request structure will always be recycled, but its skb can have different policy.
 * CCCI request can work as just a wrapper, due to netowork subsys will handler skb itself.
 * Tx: policy is determined by sender;
 * Rx: policy is determined by receiver;
 */
typedef enum {
	NOOP = 0,		/* don't handle the skb, just recycle the reqeuest wrapper */
	RECYCLE,		/* put the skb back into our pool */
	FREE,			/* simply free the skb */
} DATA_POLICY;

/* core classes */
struct ccci_request {
	struct sk_buff *skb;

	struct list_head entry;
	DATA_POLICY policy;
	REQ_STATE state;
	unsigned char blocking;	/* only for Tx */
	unsigned char ioc_override;	/* bit7: override or not; bit0: IOC setting */
};

struct ccci_modem;
struct ccci_port;

struct ccci_port_ops {
	/* must-have */
	int (*init)(struct ccci_port *port);
	int (*recv_request)(struct ccci_port *port, struct ccci_request *req);
	int (*recv_skb)(struct ccci_port *port, struct sk_buff *skb);
	/* optional */
	int (*req_match)(struct ccci_port *port, struct ccci_request *req);
	void (*md_state_notice)(struct ccci_port *port, MD_STATE state);
	void (*dump_info)(struct ccci_port *port, unsigned flag);
};

struct ccci_port {
	/* don't change the sequence unless you modified modem drivers as well */
	/* identity */
	CCCI_CH tx_ch;
	CCCI_CH rx_ch;
	/*
	 * 0xF? is used as invalid index number,  all virtual ports should use queue 0, but not 0xF?.
	 * always access queue index by using PORT_TXQ_INDEX and PORT_RXQ_INDEX macros.
	 * modem driver should always use >valid_queue_number to check invalid index, but not
	 * using ==0xF? style.
	 *
	 * here is a nasty trick, we assume no modem provide more than 0xF0 queues, so we use
	 * the lower 4 bit to smuggle info for network ports.
	 * Attention, in this trick we assume hardware queue index for net port will not exceed 0xF.
	 * check NET_ACK_TXQ_INDEX@port_net.c
	 */
	unsigned char txq_index;
	unsigned char rxq_index;
	unsigned char txq_exp_index;
	unsigned char rxq_exp_index;
	unsigned char flags;
	struct ccci_port_ops *ops;
	/* device node related */
	unsigned int minor;
	char *name;
	/* un-initiallized in defination, always put them at the end */
	struct ccci_modem *modem;
	void *private_data;
	atomic_t usage_cnt;
	struct list_head entry;
	/*
	 * the Tx and Rx flow are asymmetric due to ports are mutilplexed on queues.
	 * Tx: data block are sent directly to queue's list, so port won't maitain a Tx list. It only
	 provide a wait_queue_head for blocking write.
	 * Rx: due to modem needs to dispatch Rx packet as quickly as possible, so port needs a
	 *      Rx list to hold packets.
	 */
	struct list_head rx_req_list;
	spinlock_t rx_req_lock;
	wait_queue_head_t rx_wq;	/* for uplayer user */
	int rx_length;
	int rx_length_th;
	struct wake_lock rx_wakelock;
	unsigned int tx_busy_count;
	unsigned int rx_busy_count;
};
#define PORT_F_ALLOW_DROP	(1<<0)	/* packet will be dropped if port's Rx buffer full */
#define PORT_F_RX_FULLED	(1<<1)	/* rx buffer has been full once */
#define PORT_F_USER_HEADER	(1<<2)	/* CCCI header will be provided by user, but not by CCCI */
#define PORT_F_RX_EXCLUSIVE	(1<<3)	/* Rx queue only has this one port */

struct ccci_modem_cfg {
	unsigned int load_type;
	unsigned int load_type_saving;
	unsigned int setting;
};
#define MD_SETTING_ENABLE (1<<0)
#define MD_SETTING_RELOAD (1<<1)
#define MD_SETTING_FIRST_BOOT (1<<2)	/* this is the first time of boot up */
#define MD_SETTING_STOP_RETRY_BOOT (1<<3)
#define MD_SETTING_DUMMY  (1<<7)

struct ccci_mem_layout {	/* all from AP view, AP has no haredware remap after MT6592 */
	/* MD image */
	void __iomem *md_region_vir;
	phys_addr_t md_region_phy;
	unsigned int md_region_size;
	/* DSP image */
	void __iomem *dsp_region_vir;
	phys_addr_t dsp_region_phy;
	unsigned int dsp_region_size;
	/* Share memory */
	void __iomem *smem_region_vir;
	phys_addr_t smem_region_phy;
	unsigned int smem_region_size;
	unsigned int smem_offset_AP_to_MD;	/* offset between AP and MD view of share memory */
};

struct ccci_smem_layout {
	/* total exception region */
	void __iomem *ccci_exp_smem_base_vir;
	phys_addr_t ccci_exp_smem_base_phy;
	unsigned int ccci_exp_smem_size;
	unsigned int ccci_exp_dump_size;

	/* how we dump exception region */
	void __iomem *ccci_exp_smem_ccci_debug_vir;
	unsigned int ccci_exp_smem_ccci_debug_size;
	void __iomem *ccci_exp_smem_mdss_debug_vir;
	unsigned int ccci_exp_smem_mdss_debug_size;
	void __iomem *ccci_exp_smem_sleep_debug_vir;
	unsigned int ccci_exp_smem_sleep_debug_size;

	/* the address we parse MD exception record */
	void __iomem *ccci_exp_rec_base_vir;
};

typedef enum {
	DUMP_FLAG_CCIF = (1 << 0),
	DUMP_FLAG_CLDMA = (1 << 1),	/* tricky part, use argument length as queue index */
	DUMP_FLAG_REG = (1 << 2),
	DUMP_FLAG_SMEM = (1 << 3),
	DUMP_FLAG_IMAGE = (1 << 4),
	DUMP_FLAG_LAYOUT = (1 << 5),
	DUMP_FLAG_QUEUE_0 = (1 << 6),
	DUMP_FLAG_QUEUE_0_1 = (1 << 7),
	DUMP_FLAG_CCIF_REG = (1 << 8),
	DUMP_FLAG_SMEM_MDSLP = (1 << 9),
	DUMP_FLAG_MD_WDT = (1 << 10),
} MODEM_DUMP_FLAG;

typedef enum {
	EE_FLAG_ENABLE_WDT = (1 << 0),
	EE_FLAG_DISABLE_WDT = (1 << 1),
} MODEM_EE_FLAG;

#define MD_IMG_DUMP_SIZE (1<<8)
#define DSP_IMG_DUMP_SIZE (1<<9)

typedef enum {
	LOW_BATTERY,
	BATTERY_PERCENT,
} LOW_POEWR_NOTIFY_TYPE;

typedef enum {
	CCCI_MESSAGE,
	CCIF_INTERRUPT,
	CCIF_INTR_SEQ,
} MD_COMM_TYPE;

typedef enum {
	MD_STATUS_POLL_BUSY = (1 << 0),
	MD_STATUS_ASSERTED = (1 << 1),
} MD_STATUS_POLL_FLAG;

struct ccci_modem_ops {
	/* must-have */
	int (*init)(struct ccci_modem *md);
	int (*start)(struct ccci_modem *md);
	int (*reset)(struct ccci_modem *md);	/* as pre-stop */
	int (*stop)(struct ccci_modem *md, unsigned int timeout);
	int (*send_request)(struct ccci_modem *md, unsigned char txqno, struct ccci_request *req,
			     struct sk_buff *skb);
	int (*give_more)(struct ccci_modem *md, unsigned char rxqno);
	int (*write_room)(struct ccci_modem *md, unsigned char txqno);
	int (*start_queue)(struct ccci_modem *md, unsigned char qno, DIRECTION dir);
	int (*stop_queue)(struct ccci_modem *md, unsigned char qno, DIRECTION dir);
	int (*napi_poll)(struct ccci_modem *md, unsigned char rxqno, struct napi_struct *napi, int weight);
	int (*send_runtime_data)(struct ccci_modem *md, unsigned int sbp_code);
	int (*broadcast_state)(struct ccci_modem *md, MD_STATE state);
	int (*force_assert)(struct ccci_modem *md, MD_COMM_TYPE type);
	int (*dump_info)(struct ccci_modem *md, MODEM_DUMP_FLAG flag, void *buff, int length);
	struct ccci_port *(*get_port_by_minor)(struct ccci_modem *md, int minor);
	/*
	 * here we assume Rx and Tx channels are in the same address space,
	 * and Rx channel should be check first, so user can save one comparison if it always sends
	 * in Rx channel ID to identify a port.
	 */
	struct ccci_port *(*get_port_by_channel)(struct ccci_modem *md, CCCI_CH ch);
	int (*low_power_notify)(struct ccci_modem *md, LOW_POEWR_NOTIFY_TYPE type, int level);
	int (*ee_callback)(struct ccci_modem *md, MODEM_EE_FLAG flag);
};

struct ccci_modem {
	unsigned char index;
	unsigned char *private_data;
	struct list_head rx_ch_ports[CCCI_MAX_CH_NUM];	/* port list of each Rx channel, for Rx dispatching */
	short seq_nums[2][CCCI_MAX_CH_NUM];
	unsigned int capability;

	volatile MD_STATE md_state;	/* check comments below, put it here for cache benefit */
	struct ccci_modem_ops *ops;
	atomic_t wakeup_src;
	struct ccci_port *ports;

	struct list_head entry;
	unsigned char port_number;
	char post_fix[IMG_POSTFIX_LEN];
	unsigned int major;
	unsigned int minor_base;
	struct kobject kobj;
	struct ccci_mem_layout mem_layout;
	struct ccci_smem_layout smem_layout;
	struct ccci_image_info img_info[IMG_NUM];
	unsigned int sim_type;
	unsigned int sbp_code;
	unsigned int sbp_code_default;
	unsigned char critical_user_active[4];
	unsigned int md_img_exist[MAX_IMG_NUM];
	struct platform_device *plat_dev;
	/*
	 * the following members are readonly for CCCI core. they are maintained by modem and
	 * port_kernel.c.
	 * port_kernel.c should not be considered as part of CCCI core, we just move common part
	 * of modem message handling into this file. current modem all follows the same message
	 * protocol during bootup and exception. if future modem abandoned this protocl, we can
	 * simply replace function set of kernel port to support it.
	 */
	volatile MD_BOOT_STAGE boot_stage;
	MD_EX_STAGE ex_stage;	/* only for logging */
	phys_addr_t invalid_remap_base;
	struct ccci_modem_cfg config;
	struct timer_list bootup_timer;
	struct timer_list ex_monitor;
	struct timer_list ex_monitor2;
	struct timer_list md_status_poller;
	struct timer_list md_status_timeout;
	unsigned int md_status_poller_flag;
	spinlock_t ctrl_lock;
	volatile unsigned int ee_info_flag;
	DEBUG_INFO_T debug_info;
	unsigned char ex_type;
	EX_LOG_T ex_info;
#if PACKET_HISTORY_DEPTH
	struct ccci_log tx_history[MAX_TXQ_NUM][PACKET_HISTORY_DEPTH];
	struct ccci_log rx_history[MAX_RXQ_NUM][PACKET_HISTORY_DEPTH];
	int tx_history_ptr[MAX_TXQ_NUM];
	int rx_history_ptr[MAX_RXQ_NUM];
#endif
	unsigned long logic_ch_pkt_cnt[CCCI_MAX_CH_NUM];
	unsigned long logic_ch_pkt_pre_cnt[CCCI_MAX_CH_NUM];
#ifdef CCCI_SKB_TRACE
	unsigned long long netif_rx_profile[8];
#endif
	/* unsigned char private_data[0];
	do NOT use this manner, otherwise spinlock inside private_data will trigger alignment exception */
};

/* APIs */
extern void ccci_free_req(struct ccci_request *req);
extern void ccci_md_exception_notify(struct ccci_modem *md, MD_EX_STAGE stage);

static inline void ccci_setup_channel_mapping(struct ccci_modem *md)
{
	int i;
	struct ccci_port *port = NULL;
	/* setup mapping */
	for (i = 0; i < ARRAY_SIZE(md->rx_ch_ports); i++)
		INIT_LIST_HEAD(&md->rx_ch_ports[i]);	/* clear original list */
	for (i = 0; i < md->port_number; i++)
		list_add_tail(&md->ports[i].entry, &md->rx_ch_ports[md->ports[i].rx_ch]);
	for (i = 0; i < ARRAY_SIZE(md->rx_ch_ports); i++) {
		if (!list_empty(&md->rx_ch_ports[i])) {
			list_for_each_entry(port, &md->rx_ch_ports[i], entry) {
				CCCI_DBG_MSG(md->index, CORE, "CH%d ports:%s(%d/%d)\n", i,
					port->name, port->rx_ch, port->tx_ch);
			}
		}
	}
}

static inline void ccci_reset_seq_num(struct ccci_modem *md)
{
	/* it's redundant to use 2 arrays, but this makes sequence checking easy */
	memset(md->seq_nums[OUT], 0, sizeof(md->seq_nums[OUT]));
	memset(md->seq_nums[IN], -1, sizeof(md->seq_nums[IN]));
}

/* as one channel can only use one hardware queue,
	so it's safe we call this function in hardware queue's lock protection */
static inline void ccci_inc_tx_seq_num(struct ccci_modem *md, struct ccci_header *ccci_h)
{
#ifdef FEATURE_SEQ_CHECK_EN
	if (ccci_h->channel >= sizeof(md->seq_nums[OUT]) || ccci_h->channel < 0) {
		CCCI_INF_MSG(md->index, CORE, "ignore seq inc on channel %x\n", *(((u32 *) ccci_h) + 2));
		return;		/* for force assert channel, etc. */
	}
	ccci_h->seq_num = md->seq_nums[OUT][ccci_h->channel]++;
	ccci_h->assert_bit = 1;

	/* for rpx channel, can only set assert_bit when md is in single-task phase. */
	/* when md is in multi-task phase, assert bit should be 0, since ipc task are preemptible */
	if ((ccci_h->channel == CCCI_RPC_TX || ccci_h->channel == CCCI_FS_TX) && md->boot_stage != MD_BOOT_STAGE_1)
		ccci_h->assert_bit = 0;

#endif
}

static inline void ccci_channel_update_packet_counter(struct ccci_modem *md, struct ccci_header *ccci_h)
{
	if ((ccci_h->channel & 0xFF) < CCCI_MAX_CH_NUM)
		md->logic_ch_pkt_cnt[ccci_h->channel]++;
}

static inline void ccci_channel_dump_packet_counter(struct ccci_modem *md)
{
	CCCI_INF_MSG(md->index, CORE, "traffic(ch): tx:[%d]%ld, [%d]%ld, [%d]%ld rx:[%d]%ld, [%d]%ld, [%d]%ld\n",
		     CCCI_PCM_TX, md->logic_ch_pkt_cnt[CCCI_PCM_TX],
		     CCCI_UART2_TX, md->logic_ch_pkt_cnt[CCCI_UART2_TX],
		     CCCI_FS_TX, md->logic_ch_pkt_cnt[CCCI_FS_TX],
		     CCCI_PCM_RX, md->logic_ch_pkt_cnt[CCCI_PCM_RX],
		     CCCI_UART2_RX, md->logic_ch_pkt_cnt[CCCI_UART2_RX], CCCI_FS_RX, md->logic_ch_pkt_cnt[CCCI_FS_RX]);
	CCCI_INF_MSG(md->index, CORE,
		     "traffic(net): tx: [%d]%ld %ld, [%d]%ld %ld, [%d]%ld %ld, rx:[%d]%ld, [%d]%ld, [%d]%ld\n",
		     CCCI_CCMNI1_TX, md->logic_ch_pkt_pre_cnt[CCCI_CCMNI1_TX], md->logic_ch_pkt_cnt[CCCI_CCMNI1_TX],
		     CCCI_CCMNI2_TX, md->logic_ch_pkt_pre_cnt[CCCI_CCMNI2_TX], md->logic_ch_pkt_cnt[CCCI_CCMNI2_TX],
		     CCCI_CCMNI3_TX, md->logic_ch_pkt_pre_cnt[CCCI_CCMNI3_TX], md->logic_ch_pkt_cnt[CCCI_CCMNI3_TX],
		     CCCI_CCMNI1_RX, md->logic_ch_pkt_cnt[CCCI_CCMNI1_RX],
		     CCCI_CCMNI2_RX, md->logic_ch_pkt_cnt[CCCI_CCMNI2_RX],
		     CCCI_CCMNI3_RX, md->logic_ch_pkt_cnt[CCCI_CCMNI3_RX]);
}

#define PORT_TXQ_INDEX(p) ((p)->modem->md_state == EXCEPTION?(p)->txq_exp_index:(p)->txq_index)
#define PORT_RXQ_INDEX(p) ((p)->modem->md_state == EXCEPTION?(p)->rxq_exp_index:(p)->rxq_index)

/*
 * if send_request returns 0, then it's modem driver's duty to free the request, and caller should NOT reference the
 * request any more. but if it returns error, calller should be responsible to free the request.
 */
static inline int ccci_port_send_request(struct ccci_port *port, struct ccci_request *req)
{
	struct ccci_modem *md = port->modem;
	struct ccci_header *ccci_h = (struct ccci_header *)req->skb->data;

	md->logic_ch_pkt_pre_cnt[ccci_h->channel]++;
	return md->ops->send_request(md, PORT_TXQ_INDEX(port), req, NULL);
}

/*
 * caller should lock with port->rx_req_lock
 */
static inline int ccci_port_ask_more_request(struct ccci_port *port)
{
	struct ccci_modem *md = port->modem;
	int ret;

	if (port->flags & PORT_F_RX_FULLED)
		ret = md->ops->give_more(port->modem, PORT_RXQ_INDEX(port));
	else
		ret = -1;
	return ret;
}

/* structure initialize */
static inline void ccci_port_struct_init(struct ccci_port *port, struct ccci_modem *md)
{
	INIT_LIST_HEAD(&port->rx_req_list);
	spin_lock_init(&port->rx_req_lock);
	INIT_LIST_HEAD(&port->entry);
	init_waitqueue_head(&port->rx_wq);
	port->rx_length = 0;
	port->tx_busy_count = 0;
	port->rx_busy_count = 0;
	atomic_set(&port->usage_cnt, 0);
	port->modem = md;
	wake_lock_init(&port->rx_wakelock, WAKE_LOCK_SUSPEND, port->name);
}

/*
 * only used during allocate buffer pool, should NOT be used after allocated a request
 */
static inline void ccci_request_struct_init(struct ccci_request *req)
{
	memset(req, 0, sizeof(struct ccci_request));
	req->state = IDLE;
	req->policy = FREE;
	INIT_LIST_HEAD(&req->entry);
}

struct ccci_modem *ccci_allocate_modem(int private_size);
int ccci_register_modem(struct ccci_modem *modem);
int ccci_register_dev_node(const char *name, int major_id, int minor);
struct ccci_port *ccci_get_port_for_node(int major, int minor);
int ccci_send_msg_to_md(struct ccci_modem *md, CCCI_CH ch, CCCI_MD_MSG msg, u32 resv, int blocking);
int ccci_send_virtual_md_msg(struct ccci_modem *md, CCCI_CH ch, CCCI_MD_MSG msg, u32 resv);
struct ccci_modem *ccci_get_modem_by_id(int md_id);
int exec_ccci_kern_func_by_md_id(int md_id, unsigned int id, char *buf, unsigned int len);
void ccci_dump_log_history(struct ccci_modem *md, int dump_multi_rec, int tx_queue_num, int rx_queue_num);
void ccci_dump_log_add(struct ccci_modem *md, DIRECTION dir, int queue_index, struct ccci_header *msg, int is_dropped);

/* common sub-system */
extern int ccci_subsys_bm_init(void);
extern int ccci_subsys_sysfs_init(void);
extern int ccci_subsys_dfo_init(void);
/* per-modem sub-system */
extern int ccci_subsys_char_init(struct ccci_modem *md);
extern void md_ex_monitor_func(unsigned long data);
extern void md_ex_monitor2_func(unsigned long data);
extern void md_bootup_timeout_func(unsigned long data);
extern void md_status_poller_func(unsigned long data);
extern void md_status_timeout_func(unsigned long data);
extern void ccci_subsys_kernel_init(void);

/*
 * if recv_request returns 0 or -CCCI_ERR_DROP_PACKET, then it's port's duty to free the request, and caller should
 * NOT reference the request any more. but if it returns other error, caller should be responsible to free the request.
 */
extern int ccci_port_recv_request(struct ccci_modem *md, struct ccci_request *req, struct sk_buff *skb);

#endif	/* __CCCI_CORE_H__ */
