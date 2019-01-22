/*
* Copyright (C) 2016 MediaTek Inc.
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef _UFS_MTK_H
#define _UFS_MTK_H

#define CONFIG_MTK_UFS_DEBUG
/* #define CONFIG_MTK_UFS_DEBUG_QUEUECMD */

#include <linux/of.h>
#include <linux/rpmb.h>

#include "ufshcd.h"

#define UPIU_COMMAND_CRYPTO_EN_OFFSET	23

#define UTP_TRANSFER_REQ_TIMEOUT (5 * HZ)   /* TODO: need fine-tune */

/* UFS device quirks */
/*
 * Some UFS memory device needs limited RPMB max rw size otherwise
 * device issue, for example, device hang, may happen in some scenarios.
 */
#define UFS_DEVICE_QUIRK_LIMITED_RPMB_MAX_RW_SIZE UFS_BIT(30)

#define UFS_RPMB_DEV_MAX_RW_SIZE_LIMITATION (8)

enum {
	UNIPRO_CG_CFG_NATURE        = 0,    /* not force */
	UNIPRO_CG_CFG_FORCE_ENABLE  = 1,
	UNIPRO_CG_CFG_FORCE_DISABLE = 2,
};

enum {
	UFS_CRYPTO_ALGO_AES_XTS             = 0,
	UFS_CRYPTO_ALGO_BITLOCKER_AES_CBC   = 1,
	UFS_CRYPTO_ALGO_AES_ECB             = 2,
	UFS_CRYPTO_ALGO_ESSIV_AES_CBC       = 3,
};

enum {
	UFS_MTK_RESREQ_DMA_OP,      /* request resource for DMA operations, e.g., DRAM */
	UFS_MTK_RESREQ_MPHY_NON_H8  /* request resource for mphy not in H8, e.g., main PLL, 26 mhz clock */
};

enum {
	UFS_H8                      = 0x0,
	UFS_H8_SUSPEND              = 0x1,
};

struct ufs_cmd_str_struct {
	char str[32];
	char cmd;
};

#ifdef MTK_UFS_HQA
#define UFS_CACHED_REGION_CNT (3)
#else
#define UFS_CACHED_REGION_CNT (2)
#endif

struct ufs_cached_region {
	char *name;
	sector_t start_sect;
	sector_t end_sect;
};

/* Hynix device need max 3 seconds to clear fDeviceInit, each fDeviceInit transaction takes */
/* around 1~2ms to get response from UFS. Max fDeviceInit clear time = 5000*(1~2)ms > 3seconds */
#define UFS_FDEVICEINIT_RETRIES    (5000)

#define ASCII_STD true

/* return true if s1 is a prefix of s2 */
#define STR_PRFX_EQUAL(s1, s2) !strncmp(s1, s2, strlen(s1))

#define UFS_ANY_VENDOR 0xFFFF
#define UFS_ANY_MODEL  "ANY_MODEL"

#define MAX_MODEL_LEN 16

#define UFS_VENDOR_TOSHIBA     0x198
#define UFS_VENDOR_SAMSUNG     0x1CE
#define UFS_VENDOR_SKHYNIX     0x1AD

/**
 * ufs_device_info - ufs device details
 * @wmanufacturerid: card details
 * @model: card model
 */
struct ufs_device_info {
	u16 wmanufacturerid;
	char model[MAX_MODEL_LEN + 1];
};

#define UFS_DESCRIPTOR_SIZE (255)

struct ufs_descriptor {
	u8 descriptor_idn;
	u8 index;
	u8 descriptor[UFS_DESCRIPTOR_SIZE];

	u8 *qresp_upiu;
	u32 qresp_upiu_size;
};

/**
 * ufs_dev_fix - ufs device quirk info
 * @card: ufs card details
 * @quirk: device quirk
 */
struct ufs_dev_fix {
	struct ufs_device_info card;
	unsigned int quirk;
};

union ufs_cpt_cap {
	u32 cap_raw;
	struct {
		u8 cap_cnt;
		u8 cfg_cnt;
		u8 resv;
		u8 cfg_ptr;
	} cap;
};
union ufs_cpt_capx {
	u32 capx_raw;
	struct {
		u8 alg_id;
		u8 du_size;
		u8 key_size;
		u8 resv;
	} capx;
};
union ufs_cap_cfg {
	u32 cfgx_raw[32];
	struct {
		u32 key[16];
		u8 du_size;
		u8 cap_id;
		u16 resv0  : 15;
		u16 cfg_en : 1;
		u8 mu1ti_host;
		u8 resv1;
		u16 vsb;
		u32 resv2[14];
	} cfgx;
};
struct ufs_crypto {
	u32 cfg_id;
	u32 cap_id;
	union ufs_cpt_cap cap;
	union ufs_cpt_capx capx;
	union ufs_cap_cfg cfg;
};

#define END_FIX { { 0 }, 0 }

/* add specific device quirk */
#define UFS_FIX(_vendor, _model, _quirk) \
	       {                                         \
		       .card.wmanufacturerid = (_vendor),\
		       .card.model = (_model),           \
		       .quirk = (_quirk),                \
	       }

/*
 * If UFS device is having issue in processing LCC (Line Control
 * Command) coming from UFS host controller then enable this quirk.
 * When this quirk is enabled, host controller driver should disable
 * the LCC transmission on UFS host controller (by clearing
 * TX_LCC_ENABLE attribute of host to 0).
 */
#define UFS_DEVICE_QUIRK_BROKEN_LCC (1 << 0)

/*
 * Some UFS devices don't need VCCQ rail for device operations. Enabling this
 * quirk for such devices will make sure that VCCQ rail is not voted.
 */
#define UFS_DEVICE_NO_VCCQ (1 << 1)

/*
 * Some vendor's UFS device sends back to back NACs for the DL data frames
 * causing the host controller to raise the DFES error status. Sometimes
 * such UFS devices send back to back NAC without waiting for new
 * retransmitted DL frame from the host and in such cases it might be possible
 * the Host UniPro goes into bad state without raising the DFES error
 * interrupt. If this happens then all the pending commands would timeout
 * only after respective SW command (which is generally too large).
 *
 * We can workaround such device behaviour like this:
 * - As soon as SW sees the DL NAC error, it should schedule the error handler
 * - Error handler would sleep for 50ms to see if there are any fatal errors
 *   raised by UFS controller.
 *    - If there are fatal errors then SW does normal error recovery.
 *    - If there are no fatal errors then SW sends the NOP command to device
 *      to check if link is alive.
 *        - If NOP command times out, SW does normal error recovery
 *        - If NOP command succeed, skip the error handling.
 *
 * If DL NAC error is seen multiple times with some vendor's UFS devices then
 * enable this quirk to initiate quick error recovery and also silence related
 * error logs to reduce spamming of kernel logs.
 */
#define UFS_DEVICE_QUIRK_RECOVERY_FROM_DL_NAC_ERRORS (1 << 2)

/*
 * Some UFS devices may not work properly after resume if the link was kept
 * in off state during suspend. Enabling this quirk will not allow the
 * link to be kept in off state during suspend.
 */
#define UFS_DEVICE_QUIRK_NO_LINK_OFF   (1 << 3)

/*
 * Few Toshiba UFS device models advertise RX_MIN_ACTIVATETIME_CAPABILITY as
 * 600us which may not be enough for reliable hibern8 exit hardware sequence
 * from UFS device.
 * To workaround this issue, host should set its PA_TACTIVATE time to 1ms even
 * if device advertises RX_MIN_ACTIVATETIME_CAPABILITY less than 1ms.
 */
#define UFS_DEVICE_QUIRK_PA_TACTIVATE  (1 << 4)

/*
 * Some UFS memory devices may have really low read/write throughput in
 * FAST AUTO mode, enable this quirk to make sure that FAST AUTO mode is
 * never enabled for such devices.
 */
#define UFS_DEVICE_NO_FASTAUTO         (1 << 5)

/* Mediatek specific quirks */

/*
 * Some UFS memory device will send linkup request after POR, error handling or
 * any other scenarios. For these devices, host may need special handling flow.
 */
#define UFS_DEVICE_QUIRK_AGGRESIVE_LINKUP    (1 << 31)

/*
 * Some UFS memory device report incorrect PWM BURST CLOSURE EXTENSION.
 * For these devices, host shall set correct value regardless of device's report.
 *
 */
#define UFS_DEVICE_QUIRK_INCORRECT_PWM_BURST_CLOSURE_EXTENSION    (1 << 30)

extern u32							ufs_mtk_auto_hibern8_timer_ms;
extern enum ufs_dbg_lvl_t			ufs_mtk_dbg_lvl;
extern struct ufs_hba              *ufs_mtk_hba;
extern bool							ufs_mtk_host_deep_stall_enable;
extern bool							ufs_mtk_host_scramble_enable;
extern bool							ufs_mtk_tr_cn_used;
extern const struct of_device_id			ufs_of_match[];

void             ufs_mtk_add_sysfs_nodes(struct ufs_hba *hba);
void             ufs_mtk_advertise_fixup_device(struct ufs_hba *hba);
void             ufs_mtk_cache_setup_cmd(struct scsi_cmnd *cmd);
void             ufs_mtk_crypto_cal_dun(u32 alg_id, u32 lba, u32 *dunl, u32 *dunu);
void             ufs_mtk_dbg_dump_scsi_cmd(struct ufs_hba *hba, struct scsi_cmnd *cmd);
int              ufs_mtk_deepidle_hibern8_check(void);
void             ufs_mtk_deepidle_leave(void);
int              ufs_mtk_generic_read_dme(u32 uic_cmd, u16 mib_attribute,
					u16 gen_select_index, u32 *value, unsigned long retry_ms);
void             ufs_mtk_parse_auto_hibern8_timer(struct ufs_hba *hba);
void             ufs_mtk_parse_pm_levels(struct ufs_hba *hba);
int              ufs_mtk_ioctl_ffu(struct scsi_device *dev, void __user *buf_user);
int              ufs_mtk_ioctl_get_fw_ver(struct scsi_device *dev, void __user *buf_user);
int              ufs_mtk_ioctl_query(struct ufs_hba *hba, u8 lun, void __user *buf_user);
void             ufs_mtk_rpmb_dump_frame(struct scsi_device *sdev, u8 *data_frame, u32 cnt);
struct rpmb_dev *ufs_mtk_rpmb_get_raw_dev(void);

#endif /* !_UFS_MTK_H */

