 /*
  * Copyright (C) 2016 MediaTek Inc.
  *
  * Author: Sakya <jeff_chang@richtek.com>
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


#ifndef __LINUX_TA_VDM_H
#define __LINUX_TA_VDM_H

#define RT7207_SVID 0x29CF

#define DC_VDO_VID(vdo)  ((vdo) >> 16)
#define DC_VDO_SVDM(vdo) (((vdo) >> 15) & 1)
#define DC_VDO_ACTION(vdo) ((vdo) & 0x7000)
#define DC_VDO_CMD(vdo) ((vdo) & 0xFF)

#define OP_READ (0x1<<12)
#define OP_WRITE (0x1<<13)
#define OP_ACK (0x1<<14)

#define CMD_READ_DEVICE_TYPE 0x10
#define CMD_READ_SPEC_VERSION 0x11
#define CMD_READ_MANUFACTURE_INFO 0x12
#define CMD_READ_PRODUCT_TYPE 0x13
#define CMD_READ_DATA_CODE 0x14
#define CMD_READ_SERIAL_NUM 0x15
#define CMD_READ_VER_INFO 0x16
#define CMD_READ_MAX_PWR 0x17
#define CMD_READ_MAX_VOL_CUR 0x18
#define CMD_READ_MIN_VOL_CUR 0x19
#define CMD_READ_CHARGER_STAT 0x1A
#define CMD_READ_VOL_CUR_STAT 0x1B
#define CMD_READ_SETTING_VOL_CUR 0x1C
#define CMD_READ_TEMP 0x1D
#define CMD_READ_AC_INPUT_VOL 0x1E
#define CMD_READ_TA_INFO 0x1F

#define CMD_SET_OUTPUT_CONTROL 0x20
#define CMD_SET_BNDRY_VOL_CUR 0x21
#define CMD_VOL_CUR_SETTING 0x22
#define CMD_SET_UVLO_VOL_CUR 0x23
#define CMD_AUTH 0x24

struct pd_direct_chrg {
	struct typec_hba *hba;
	struct mutex vdm_event_lock;
	struct mutex vdm_payload_lock;
	bool dc_vdm_inited;
	uint32_t vdm_payload[7];
	wait_queue_head_t wq;
	bool data_in;
	uint32_t auth_code;
	int32_t auth_pass;
#ifdef CONFIG_DEBUG_FS
	struct dentry *root;
#endif
};

struct mtk_vdm_ta_cap {
	int cur;
	int vol;
	int temp;
};

enum {
	PD_USB_NOT_SUPPORT = -1,
	PD_USB_DISCONNECT = 0,
	PD_USB_CONNECT = 1,
};

struct pd_ta_stat {
	unsigned char chg_mode:1;
	unsigned char dc_en:1;
	unsigned char dpc_en:1;
	unsigned char pc_en:1;
	unsigned char ovp:1;
	unsigned char otp:1;
	unsigned char uvp:1;
	unsigned char rvs_cur:1;
	unsigned char ping_chk_fail:1;
};

#ifdef CONFIG_RT7207_ADAPTER
enum { /* charge status */
	RT7207_CC_MODE,
	RT7207_CV_MODE,
};

#define MTK_VDM_FAIL  (-1)
#define MTK_VDM_SUCCESS  (0)
#define MTK_VDM_SW_BUSY	(1)

/* mtk_direct_charge_vdm_init
 *	1. get tcpc_device handler
 *	2. init mutex & wakelock
 *	3. register tcp notifier
 *	4. add debugfs node
 */
extern bool mtk_is_pd_chg_ready(void);
extern int tcpm_hard_reset(void *ptr);
extern int tcpm_set_direct_charge_en(void *ptr, bool en);
extern int tcpm_get_cable_capability(void *ptr, unsigned char *capability);
extern bool mtk_is_pep30_en_unlock(void);

extern int mtk_direct_charge_vdm_init(void);
extern int mtk_get_ta_id(void *ptr);
extern int mtk_clr_ta_pingcheck_fault(void *ptr);
extern int mtk_get_ta_charger_status(void *ptr, struct pd_ta_stat *ta);
extern int mtk_get_ta_temperature(void *ptr, int *temp);
extern int mtk_set_ta_boundary_cap(void *ptr, struct mtk_vdm_ta_cap *cap);
extern int mtk_set_ta_uvlo(void *ptr,  int mv);
extern int mtk_get_ta_current_cap(void *ptr, struct mtk_vdm_ta_cap *cap);
extern int mtk_get_ta_setting_dac(void *ptr, struct mtk_vdm_ta_cap *cap);
extern int mtk_get_ta_boundary_cap(void *ptr, struct mtk_vdm_ta_cap *cap);
extern int mtk_set_ta_cap(void *ptr, struct mtk_vdm_ta_cap *cap);
extern int mtk_get_ta_cap(void *ptr, struct mtk_vdm_ta_cap *cap);
extern int mtk_monitor_ta_inform(void *ptr, struct mtk_vdm_ta_cap *cap);
extern int mtk_enable_direct_charge(void *ptr, bool en);
extern int mtk_enable_ta_dplus_dect(void *ptr, bool en, int time);

extern int start_dc_auth(struct pd_direct_chrg *dc);
extern int handle_dc_auth(struct pd_direct_chrg *dc, int cnt, uint32_t *payload, uint32_t **rpayload);
extern void mtk_direct_charging_payload(struct pd_direct_chrg *dc, int cnt, uint32_t *payload);

#else /* not config RT7027 PD adapter */

static inline int mtk_direct_charge_vdm_init(void)
{
	return -1;
}

static inline int mtk_get_ta_id(void *tcpc)
{
	return -1;
}

static inline int mtk_set_ta_cap(
		void *tcpc, struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}

static inline int mtk_get_ta_cap(
		void *tcpc, struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}

static inline int mtk_get_ta_charger_status(
			void *tcpc, struct pd_ta_stat *ta)
{
	return -1;
}

static inline int mtk_get_ta_current_cap(
		void *tcpc, struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}

static inline int mtk_get_ta_temperature(void *tcpc, int *temp)
{
	return -1;
}

static inline int mtk_update_ta_info(void *tcpc)
{
	return -1;
}

static inline int mtk_set_ta_boundary_cap(
		void *tcpc, struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}

static inline int mtk_rqst_ta_cap(
		void *tcpc, struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}

static inline int mtk_set_ta_uvlo(void *tcpc, int mv)
{
	return -1;
}

static inline int mtk_show_ta_info(void *tcpc)
{
	return -1;
}

static inline int mtk_get_ta_setting_dac(
			void *tcpc, struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}

static inline int mtk_get_ta_boundary_cap(
			void *tcpc, struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}

static inline int mtk_enable_direct_charge(void *tcpc, bool en)
{
	return -1;
}

static inline int mtk_enable_ta_dplus_dect(
			void *tcpc, bool en, int time)
{
	return -1;
}

static inline int mtk_monitor_ta_inform(void *tcpc,
					struct mtk_vdm_ta_cap *cap)
{
	cap->cur = cap->vol = cap->temp = 0;
	return -1;
}
#endif /* CONFIG_RT7207_ADAPTER */

#endif /* __LINUX_TA_VDM_H */
