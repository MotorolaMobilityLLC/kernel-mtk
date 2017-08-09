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
#include "ccci_config.h"
#include "ccci_debug.h"

#define CCCI_MAGIC_NUM 0xFFFFFFFF
#define MAX_TXQ_NUM 8
#define MAX_RXQ_NUM 8
#define PACKET_HISTORY_DEPTH 16	/* must be power of 2 */

#define C2K_MD_LOG_TX_Q		3
#define C2K_MD_LOG_RX_Q		3
#define C2K_PCM_TX_Q		1
#define C2K_PCM_RX_Q		1

#define EX_TIMER_SWINT 10
#define EX_TIMER_MD_EX 5
#define EX_TIMER_MD_EX_REC_OK 10
#define EX_EE_WHOLE_TIMEOUT  (EX_TIMER_SWINT + EX_TIMER_MD_EX + EX_TIMER_MD_EX_REC_OK + 2) /* 2s is buffer */

#define CCCI_BUF_MAGIC 0xFECDBA89

/*
  * this is a trick for port->minor, which is configured in-sequence by different type (char, net, ipc),
  * but when we use it in code, we need it's unique among all ports for addressing.
  */
#define CCCI_IPC_MINOR_BASE 100
#define CCCI_NET_MINOR_BASE 200

struct ccci_log {
	struct ccci_header msg;
	u64 tv;
	int droped;
};

enum ccci_ipi_op_id {
	CCCI_OP_SCP_STATE,
	CCCI_OP_MD_STATE,
	CCCI_OP_SHM_INIT,
	CCCI_OP_SHM_RESET,

	CCCI_OP_LOG_LEVEL,
	CCCI_OP_GPIO_TEST,
	CCCI_OP_EINT_TEST,
	CCCI_OP_MSGSND_TEST,
	CCCI_OP_ASSERT_TEST,
};

enum {
	SCP_CCCI_STATE_INVALID = 0,
	SCP_CCCI_STATE_BOOTING = 1,
	SCP_CCCI_STATE_RBREADY = 2,
};

struct ccci_ipi_msg {
	u16 md_id;
	u16 op_id;
	u32 data[1];
} __packed;

/* enumerations and marcos */
typedef enum {
	EX_NONE = 0,
	EX_INIT,
	EX_DHL_DL_RDY,
	EX_INIT_DONE,
	/* internal use */
	MD_NO_RESPONSE,
	MD_WDT,
	MD_BOOT_TIMEOUT,
} MD_EX_STAGE;


typedef enum {
	MD_FIGHT_MODE_NONE = 0,
	MD_FIGHT_MODE_ENTER = 1,
	MD_FIGHT_MODE_LEAVE = 2
} FLIGHT_STAGE;		/* for other module */

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


typedef enum {
	IDLE = 0,		/* update by buffer manager */
	FLYING,			/* update by buffer manager */
	PARTIAL_READ,		/* update by port_char */
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
	FREE = 0,		/* simply free the skb */
	RECYCLE,		/* put the skb back into our pool */
} DATA_POLICY;

/* ccci buffer control structure. Must be less than NET_SKB_PAD */
struct ccci_buffer_ctrl {
	unsigned int head_magic;
	DATA_POLICY policy;
	unsigned char ioc_override;	/* bit7: override or not; bit0: IOC setting */
	unsigned char blocking;
};

struct ccci_modem;
struct ccci_port;

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
	unsigned int smem_offset_AP_to_MD; /* offset between AP and MD view of share memory */
	/*MD1 MD3 shared memory*/
	void __iomem *md1_md3_smem_vir;
	phys_addr_t md1_md3_smem_phy;
	unsigned int md1_md3_smem_size;
};

struct ccci_smem_layout {
	/* total exception region */
	void __iomem *ccci_exp_smem_base_vir;
	phys_addr_t ccci_exp_smem_base_phy;
	unsigned int ccci_exp_smem_size;
	unsigned int ccci_exp_dump_size;

	/* runtime data and mpu region */
	void __iomem *ccci_rt_smem_base_vir;
	phys_addr_t ccci_rt_smem_base_phy;
	unsigned int ccci_rt_smem_size;

	/* CCISM region */
	void __iomem *ccci_ccism_smem_base_vir;
	phys_addr_t ccci_ccism_smem_base_phy;
	unsigned int ccci_ccism_smem_size;
	unsigned int ccci_ccism_dump_size;

	/* DHL share memory region */
	void __iomem *ccci_ccb_dhl_base_vir;
	phys_addr_t ccci_ccb_dhl_base_phy;
	unsigned int ccci_ccb_dhl_size;
	void __iomem *ccci_raw_dhl_base_vir;
	phys_addr_t ccci_raw_dhl_base_phy;
	unsigned int ccci_raw_dhl_size;

	/* direct tethering region */
	void __iomem *ccci_dt_netd_smem_base_vir;
	phys_addr_t ccci_dt_netd_smem_base_phy;
	unsigned int ccci_dt_netd_smem_size;
	void __iomem *ccci_dt_usb_smem_base_vir;
	phys_addr_t ccci_dt_usb_smem_base_phy;
	unsigned int ccci_dt_usb_smem_size;

	/* smart logging region */
	void __iomem *ccci_smart_logging_base_vir;
	phys_addr_t ccci_smart_logging_base_phy;
	unsigned int ccci_smart_logging_size;

	/* CCIF share memory region */
	void __iomem *ccci_ccif_smem_base_vir;
	phys_addr_t ccci_ccif_smem_base_phy;
	unsigned int ccci_ccif_smem_size;

	/* sub regions in exception region */
	void __iomem *ccci_exp_smem_ccci_debug_vir;
	unsigned int ccci_exp_smem_ccci_debug_size;
	void __iomem *ccci_exp_smem_mdss_debug_vir;
	unsigned int ccci_exp_smem_mdss_debug_size;
	void __iomem *ccci_exp_smem_sleep_debug_vir;
	unsigned int ccci_exp_smem_sleep_debug_size;
	void __iomem *ccci_exp_smem_dbm_debug_vir;
	void __iomem *ccci_exp_smem_force_assert_vir;
	unsigned int ccci_exp_smem_force_assert_size;
};

typedef enum {
	DUMP_FLAG_CCIF = (1 << 0),
	DUMP_FLAG_CLDMA = (1 << 1),	/* tricky part, use argument length as queue index */
	DUMP_FLAG_REG = (1 << 2),
	DUMP_FLAG_SMEM_EXP = (1 << 3),
	DUMP_FLAG_IMAGE = (1 << 4),
	DUMP_FLAG_LAYOUT = (1 << 5),
	DUMP_FLAG_QUEUE_0 = (1 << 6),
	DUMP_FLAG_QUEUE_0_1 = (1 << 7),
	DUMP_FLAG_CCIF_REG = (1 << 8),
	DUMP_FLAG_SMEM_MDSLP = (1 << 9),
	DUMP_FLAG_MD_WDT = (1 << 10),
	DUMP_FLAG_SMEM_CCISM = (1<<11),
	DUMP_MD_BOOTUP_STATUS = (1<<12),
	DUMP_FLAG_IRQ_STATUS = (1<<13),
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
	MD_FORCE_ASSERT_RESERVE = 0x000,
	MD_FORCE_ASSERT_BY_MD_NO_RESPONSE	= 0x100,
	MD_FORCE_ASSERT_BY_MD_SEQ_ERROR		= 0x200,
	MD_FORCE_ASSERT_BY_AP_Q0_BLOCKED	= 0x300,
	MD_FORCE_ASSERT_BY_USER_TRIGGER		= 0x400,
	MD_FORCE_ASSERT_BY_MD_WDT			= 0x500,
	MD_FORCE_ASSERT_BY_AP_MPU			= 0x600,
} MD_FORCE_ASSERT_TYPE;

typedef enum {
	CCCI_MESSAGE,
	CCIF_INTERRUPT,
	CCIF_MPU_INTR,
} MD_COMM_TYPE;

typedef enum {
	MD_STATUS_POLL_BUSY = (1 << 0),
	MD_STATUS_ASSERTED = (1 << 1),
} MD_STATUS_POLL_FLAG;

typedef enum {
	OTHER_MD_NONE,
	OTHER_MD_STOP,
	OTHER_MD_RESET,
} OTHER_MD_OPS;

struct ccci_force_assert_shm_fmt {
	unsigned int  error_code;
	unsigned int  param[3];
	unsigned char reserved[0];
};


typedef void __iomem *(*smem_sub_region_cb_t)(void *md_blk, int *size_o);

/****************handshake v2*************/
typedef enum{
	BOOT_INFO = 0,
	EXCEPTION_SHARE_MEMORY,
	CCIF_SHARE_MEMORY,
	SMART_LOGGING_SHARE_MEMORY,
	MD1MD3_SHARE_MEMORY,
	/*ccci misc info*/
	MISC_INFO_HIF_DMA_REMAP,
	MISC_INFO_RTC_32K_LESS,
	MISC_INFO_RANDOM_SEED_NUM,
	MISC_INFO_GPS_COCLOCK,
	MISC_INFO_SBP_ID,
	MISC_INFO_CCCI,	/*=10*/
	MISC_INFO_CLIB_TIME,
	MISC_INFO_C2K,
	MD_IMAGE_START_MEMORY,
	CCISM_SHARE_MEMORY,
	CCB_SHARE_MEMORY, /* total size of all CCB regions */
	DHL_RAW_SHARE_MEMORY,
	DT_NETD_SHARE_MEMORY,
	DT_USB_SHARE_MEMORY,
	EE_AFTER_EPOF,
	CCMNI_MTU, /* max Rx packet buffer size on AP side */
	MD_RUNTIME_FEATURE_ID_MAX,
} MD_CCCI_RUNTIME_FEATURE_ID;

typedef enum{
	AT_CHANNEL_NUM = 0,
	AP_RUNTIME_FEATURE_ID_MAX,
} AP_CCCI_RUNTIME_FEATURE_ID;

typedef enum{
	CCCI_FEATURE_NOT_EXIST = 0,
	CCCI_FEATURE_NOT_SUPPORT = 1,
	CCCI_FEATURE_MUST_SUPPORT = 2,
	CCCI_FEATURE_OPTIONAL_SUPPORT = 3,
	CCCI_FEATURE_SUPPORT_BACKWARD_COMPAT = 4,
} CCCI_RUNTIME_FEATURE_SUPPORT_TYPE;

struct ccci_feature_support {
	u8 support_mask:4;
	u8 version:4;
};

struct ccci_runtime_feature {
	u8 feature_id;	/*for debug only*/
	struct ccci_feature_support support_info;
	u8 reserved[2];
	u32 data_len;
	u8 data[0];
};

struct ccci_runtime_boot_info {
	u32 boot_channel;
	u32 booting_start_id;
	u32 boot_attributes;
	u32 boot_ready_id;
};

struct ccci_runtime_share_memory {
	u32 addr;
	u32 size;
};

struct ccci_misc_info_element {
	u32 feature[4];
};

#define FEATURE_COUNT 64
#define MD_FEATURE_QUERY_PATTERN 0x49434343
#define AP_FEATURE_QUERY_PATTERN 0x43434349

struct md_query_ap_feature {
	u32 head_pattern;
	struct ccci_feature_support feature_set[FEATURE_COUNT];
	u32 tail_pattern;
};

struct ap_query_md_feature {
	u32 head_pattern;
	struct ccci_feature_support feature_set[FEATURE_COUNT];
	u32 share_memory_support;
	u32 ap_runtime_data_addr;
	u32 ap_runtime_data_size;
	u32 md_runtime_data_addr;
	u32 md_runtime_data_size;
	u32 set_md_mpu_start_addr;
	u32 set_md_mpu_total_size;
	u32 tail_pattern;
};
/*********************************************/
typedef enum {
	MD_BOOT_MODE_INVALID = 0,
	MD_BOOT_MODE_NORMAL,
	MD_BOOT_MODE_META,
	MD_BOOT_MODE_MAX,
} MD_BOOT_MODE;

enum {
	CRIT_USR_FS,
	CRIT_USR_MUXD,
	CRIT_USR_MDLOG,
	CRIT_USR_META,
	CRIT_USR_MDLOG_CTRL = CRIT_USR_META,
	CRIT_USR_MAX,
};

enum {
	MD_DBG_DUMP_INVALID = -1,
	MD_DBG_DUMP_TOPSM = 0,
	MD_DBG_DUMP_PCMON,
	MD_DBG_DUMP_BUSREC,
	MD_DBG_DUMP_MDRGU,
	MD_DBG_DUMP_OST,
	MD_DBG_DUMP_BUS,
	MD_DBG_DUMP_PLL,
	MD_DBG_DUMP_ECT,

	MD_DBG_DUMP_SMEM,
	MD_DBG_DUMP_ALL = 0x7FFFFFFF,
};

#ifdef FEATURE_MD_GET_CLIB_TIME
extern volatile int current_time_zone;
#endif

int ccci_register_dev_node(const char *name, int major_id, int minor);
int exec_ccci_kern_func_by_md_id(int md_id, unsigned int id, char *buf, unsigned int len);
int ccci_scp_ipi_send(int md_id, int op_id, void *data);
void ccci_sysfs_add_md(int md_id, void *kobj);
void scp_md_state_sync_work(struct work_struct *work);

/* common sub-system */
extern int ccci_subsys_bm_init(void);
extern int ccci_subsys_sysfs_init(void);
extern int ccci_subsys_dfo_init(void);
/* per-modem sub-system */
extern void set_ccif_cg(unsigned int on);
extern int switch_MD1_Tx_Power(unsigned int mode);
extern int switch_MD2_Tx_Power(unsigned int mode);

#ifdef FEATURE_MTK_SWITCH_TX_POWER
int swtp_init(int md_id);
#endif
#endif	/* __CCCI_CORE_H__ */
