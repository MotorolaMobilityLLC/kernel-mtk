 /*
  * Copyright (C) 2016 MediaTek Inc.
  *
  * MTK Direct Charge Vdm Driver
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

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/random.h>

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif /* CONFIG_DEBUG_FS */

#include "mtk_typec.h"

#include "usb_pd.h"
#include "usb_pd_func.h"
#include "mtk_direct_charge_vdm.h"

static struct pd_direct_chrg *g_dc;

struct pd_direct_chrg *get_dc(void)
{
	return g_dc;
}

bool mtk_is_pd_chg_ready(void)
{
	struct pd_direct_chrg *dc = get_dc();

	if (dc)
		return (dc->hba->task_state == PD_STATE_SNK_READY);

	return false;
}

int tcpm_hard_reset(void *ptr)
{
	struct pd_direct_chrg *dc = get_dc();

	pr_info("%s request HARD RESET\n", __func__);

	if (!dc || (dc->hba->task_state != PD_STATE_SNK_READY))
		return -1;

	set_state(dc->hba, PD_STATE_HARD_RESET_SEND);

	return 0;
}

#define MTK_PE30_VSAFE5V 3400

int tcpm_set_direct_charge_en(void *ptr, bool en)
{
	struct pd_direct_chrg *dc = get_dc();

	if (!dc)
		return -1;

	if (mtk_is_pep30_en_unlock()) {
		pr_info("%s vSafe5v @ Dircet Charging\n", (en?"Decrease":"Restore"));

		typec_set_vsafe5v(dc->hba, (en ? MTK_PE30_VSAFE5V : PD_VSAFE5V_LOW));
	}

	return 0;
}

#define PD_VDO_CABLE_CURR(x)	(((x) >> 5) & 0x03)

int tcpm_get_cable_capability(void *ptr, unsigned char *capability)
{
	struct typec_hba *hba = get_dc()->hba;

	unsigned char limit = 0;

	if (hba->sop_p.cable_vdo) {
		limit =  PD_VDO_CABLE_CURR(hba->sop_p.cable_vdo);

		pr_info("%s limit = %d\n", __func__, limit);

		limit = (limit > 3) ? 0 : limit;

		*capability = limit;
	} else
		pr_info("%s it's not power cable\n", __func__);

	return 0;
}

bool mtk_is_pep30_en_unlock(void)
{
	struct pd_direct_chrg *dc = get_dc();

	if (!mtk_is_pd_chg_ready())
		return false;

	pr_info("%s PE3.0 is %s\n", __func__, (dc->auth_pass == 1)?"UNLOCK":"LOCK");

	return (dc->auth_pass == 1);
}

void mtk_direct_charging_payload(struct pd_direct_chrg *dc, int cnt, uint32_t *payload)
{
	mutex_lock(&dc->vdm_payload_lock);
	memcpy(dc->vdm_payload, payload, sizeof(uint32_t)*cnt);
	mutex_unlock(&dc->vdm_payload_lock);
}

int queue_dc_command(struct pd_direct_chrg *dc, uint32_t vid, int op, int cmd, const uint32_t *data, int cnt)
{
	int status = MTK_VDM_FAIL;
	int ret = 0;

	if (!dc->dc_vdm_inited) {
		pr_err("%s vdm not inited\n", __func__);
		goto end;
	}

	if (dc->auth_pass != 1) {
		pr_err("%s pep30 is not ready\n", __func__);
		goto end;
	}

	pr_err("%s cmd=%x\n", __func__, (op|cmd));

	mutex_lock(&dc->vdm_event_lock);

	dc->data_in = false;

	ret = pd_send_vdm(dc->hba, vid, op|cmd, data, cnt);
	if (!ret) {
		dc->hba->rx_event = true;
		wake_up(&dc->hba->wq);
		if (wait_event_timeout(dc->wq, dc->data_in, msecs_to_jiffies(PD_VDM_TX_TIMEOUT+10)) > 0) {
			if (DC_VDO_ACTION(dc->vdm_payload[0]) != OP_ACK)
				pr_err("%s return action error\n", __func__);
			else if (DC_VDO_CMD(dc->vdm_payload[0]) != cmd)
				pr_err("%s return cmd error\n", __func__);
			else
				status = MTK_VDM_SUCCESS;
		} else {
			pr_err("%s Time out\n", __func__);
			dc->hba->vdm_state = VDM_STATE_DONE;
		}
	} else
		status = MTK_VDM_SW_BUSY;

	mutex_unlock(&dc->vdm_event_lock);
end:
	return status;
}

int mtk_get_ta_id(void *ptr)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int id = 0;
	int ret = 0;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_READ_MANUFACTURE_INFO, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		id = dc->vdm_payload[1];
		pr_err("%s id=%d\n", __func__, id);
	} else {
		id = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return id;
}

int mtk_clr_ta_pingcheck_fault(void *ptr)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;
	uint32_t data = 0;

	data = 0x10;

	ret = queue_dc_command(dc, RT7207_SVID, OP_WRITE, CMD_READ_CHARGER_STAT, &data, 1);
	if (ret == MTK_VDM_SUCCESS) {
		pr_info("%s Success\n", __func__);
		status = MTK_VDM_SUCCESS;
	} else {
		pr_err("%s CMD FAIL\n", __func__);
		status = ret;
	}

	return status;
}


#define BIT_CHG_MODE (0x1<<31)
#define BIT_DC_EN (0x1<<30)
#define BIT_DPC_EN (0x1<<29)
#define BIT_PC_EN (0x1<<28)
#define BIT_OVP (0x1<<0)
#define BIT_OTP (0x1<<1)
#define BIT_UVP (0x1<<2)
#define BIT_RVS_CUR (0x1<<3)
#define BIT_PING_CHK_FAIL (0x1<<4)

int mtk_get_ta_charger_status(void *ptr, struct pd_ta_stat *ta)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_READ_CHARGER_STAT, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		ta->chg_mode = (dc->vdm_payload[1] & BIT_CHG_MODE) ?
			RT7207_CC_MODE : RT7207_CV_MODE;
		ta->dc_en = !!(dc->vdm_payload[1] & BIT_DC_EN);
		ta->dpc_en = !!(dc->vdm_payload[1] & BIT_DPC_EN);
		ta->pc_en = !!(dc->vdm_payload[1] & BIT_PC_EN);
		ta->ovp = !!(dc->vdm_payload[1] & BIT_OVP);
		ta->otp = !!(dc->vdm_payload[1] & BIT_OTP);
		ta->uvp = !!(dc->vdm_payload[1] & BIT_UVP);
		ta->rvs_cur = !!(dc->vdm_payload[1] & BIT_RVS_CUR);
		ta->ping_chk_fail = !!(dc->vdm_payload[1] & BIT_PING_CHK_FAIL);
		pr_debug("%s %s, dc(%d), dpc(%d), pc(%d) ovp(%d), otp(%d) uvp(%d) rc(%d), pf(%d)\n",
			__func__, ta->chg_mode > 0 ? "CC Mode" : "CV Mode",
			ta->dc_en, ta->dpc_en, ta->pc_en, ta->ovp, ta->otp,
			ta->uvp, ta->rvs_cur, ta->ping_chk_fail);
		status = MTK_VDM_SUCCESS;
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_get_ta_current_cap(void *ptr, struct mtk_vdm_ta_cap *cap)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_READ_VOL_CUR_STAT, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		cap->vol = dc->vdm_payload[1] & 0xFFFF;
		cap->cur = (dc->vdm_payload[1] >> 16) & 0xFFFF;
		status = MTK_VDM_SUCCESS;
		pr_debug("%s mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_monitor_ta_inform(void *ptr, struct mtk_vdm_ta_cap *cap)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_READ_TA_INFO, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		cap->vol = (dc->vdm_payload[1] & 0xFFFF) * 5 * 2700 / 1023;
		cap->cur = (dc->vdm_payload[1] >> 16) & 0xFFFF;
		cap->temp = dc->vdm_payload[2];
		pr_debug("%s VOL(%d),CUR(%d),TEMP(%d)\n",
			__func__, cap->vol, cap->cur, cap->temp);
		status = MTK_VDM_SUCCESS;
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_enable_direct_charge(void *ptr, bool en)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	struct pd_ta_stat stat;
	int ret = 0;
	int status = MTK_VDM_FAIL;
	uint32_t data = 0;

	ret = mtk_get_ta_charger_status(dc, &stat);
	if (ret < 0) {
		pr_err("%s get ta charger status fail\n", __func__);
		goto end;
	}

	if (en) {
		if (stat.dc_en) {
			pr_err("%s DC already enable\n", __func__);
			status = MTK_VDM_SUCCESS;
			goto end;
		}
	} else {
		if (!stat.dc_en) {
			pr_err("%s DC already disable\n", __func__);
			status = MTK_VDM_SUCCESS;
			goto end;
		}
	}

	data = en ? RT7207_SVID : 0;

	ret = queue_dc_command(dc, RT7207_SVID, OP_WRITE, CMD_SET_OUTPUT_CONTROL, &data, 1);
	if (ret == MTK_VDM_SUCCESS) {
		pr_debug("%s (%d)Success\n", __func__, en);
		status = MTK_VDM_SUCCESS;
	} else {
		pr_err("%s (%d)Failed\n", __func__, en);
		status = ret;
	}
end:
	return status;
}

int mtk_enable_ta_dplus_dect(void *ptr, bool en, int time)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	struct pd_ta_stat stat;
	int ret = 0;
	int status = MTK_VDM_FAIL;
	uint32_t data[2] = {0};

	ret = mtk_get_ta_charger_status(dc, &stat);
	if (ret < 0) {
		pr_err("%s get ta charger status fail\n", __func__);
		goto end;
	}

	if (en) {
		if (stat.dc_en) {
			data[0] = RT7207_SVID;
			data[1] = time;
		} else {
			pr_err("%s DC should enabled\n", __func__);
			goto end;
		}
	} else {
		if (stat.dc_en)
			data[0] = RT7207_SVID;
		else {
			pr_err("%s DC already disabled\n", __func__);
			status = MTK_VDM_SUCCESS;
			goto end;
		}
	}

	ret = queue_dc_command(dc, RT7207_SVID, OP_WRITE, CMD_SET_OUTPUT_CONTROL, data, en?2:1);
	if (ret == MTK_VDM_SUCCESS) {
		pr_debug("%s (%d)Success\n", __func__, en);
		status = MTK_VDM_SUCCESS;
	} else {
		pr_err("%s (%d)Failed\n", __func__, en);
		status = ret;
	}
end:
	return status;
}

int mtk_get_ta_setting_dac(void *ptr, struct mtk_vdm_ta_cap *cap)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_READ_SETTING_VOL_CUR, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		cap->vol = (dc->vdm_payload[1] & 0xFFFF) * 26;
		cap->cur = ((dc->vdm_payload[1] >> 16) & 0xFFFF) * 26;
		status = MTK_VDM_SUCCESS;
		pr_debug("%s mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_get_ta_temperature(void *ptr, int *temp)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_READ_TEMP, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		*temp = dc->vdm_payload[1] & 0xFFFF;
		status = MTK_VDM_SUCCESS;
		pr_debug("%s temperatue = %d\n", __func__, *temp);
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_set_ta_boundary_cap(void *ptr, struct mtk_vdm_ta_cap *cap)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;
	uint32_t data = 0;

	data = (cap->cur<<16) | cap->vol;
	pr_err("%s set mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);

	ret = queue_dc_command(dc, RT7207_SVID, OP_WRITE, CMD_SET_BNDRY_VOL_CUR, &data, 1);
	if (ret == MTK_VDM_SUCCESS) {
		cap->vol = dc->vdm_payload[1] & 0xFFFF;
		cap->cur = (dc->vdm_payload[1] >> 16) & 0xFFFF;
		status = MTK_VDM_SUCCESS;
		pr_debug("%s mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_get_ta_boundary_cap(void *ptr, struct mtk_vdm_ta_cap *cap)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_SET_BNDRY_VOL_CUR, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		cap->vol = dc->vdm_payload[1] & 0xFFFF;
		cap->cur = (dc->vdm_payload[1] >> 16) & 0xFFFF;
		status = MTK_VDM_SUCCESS;
		pr_debug("%s mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_set_ta_cap(void *ptr, struct mtk_vdm_ta_cap *cap)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;
	uint32_t data = 0;

	data = (cap->cur<<16) | cap->vol;
	pr_debug("%s set mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);

	ret = queue_dc_command(dc, RT7207_SVID, OP_WRITE, CMD_VOL_CUR_SETTING, &data, 1);
	if (ret == MTK_VDM_SUCCESS) {
		cap->vol = dc->vdm_payload[1] & 0xFFFF;
		cap->cur = (dc->vdm_payload[1] >> 16) & 0xFFFF;
		status = MTK_VDM_SUCCESS;
		pr_debug("%s mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_get_ta_cap(void *ptr, struct mtk_vdm_ta_cap *cap)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;

	ret = queue_dc_command(dc, RT7207_SVID, OP_READ, CMD_VOL_CUR_SETTING, NULL, 0);
	if (ret == MTK_VDM_SUCCESS) {
		cap->vol = dc->vdm_payload[1] & 0xFFFF;
		cap->cur = (dc->vdm_payload[1] >> 16) & 0xFFFF;
		status = MTK_VDM_SUCCESS;
		pr_debug("%s mv = %dmv,ma = %dma\n", __func__, cap->vol, cap->cur);
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

int mtk_set_ta_uvlo(void *ptr, int mv)
{
	struct pd_direct_chrg *dc = ((!ptr) ? get_dc() : ptr);
	int ret = 0;
	int status = MTK_VDM_FAIL;
	uint32_t data = 0;

	data = (mv & 0xFFFF);

	ret = queue_dc_command(dc, RT7207_SVID, OP_WRITE, CMD_SET_UVLO_VOL_CUR, &data, 1);
	if (ret == MTK_VDM_SUCCESS) {
		pr_debug("%s uvlo = %dmv\n", __func__, (dc->vdm_payload[1] & 0xFFFF));
		status = MTK_VDM_SUCCESS;
	} else {
		status = ret;
		pr_err("%s CMD FAIL\n", __func__);
	}

	return status;
}

void crcBits(uint32_t Data, uint32_t *crc, uint32_t *pPolynomial)
{
	uint32_t i, newbit, newword, rl_crc;

	for (i = 0; i < 32; i++) {
		newbit = ((*crc >> 31) ^ ((Data >> i) & 1)) & 1;
		if (newbit)
			newword = *pPolynomial;
		else
			newword = 0;
		rl_crc = (*crc << 1) | newbit;
		*crc = rl_crc ^ newword;
	}
}

uint32_t crcWrap(uint32_t V)
{
	uint32_t ret = 0, i, j, bit;

	V = ~V;
	for (i = 0; i < 32; i++) {
		j = 31 - i;
		bit = (V >> i) & 1;
		ret |= bit << j;
	}

	return ret;
}

static uint32_t dc_get_random_code(void)
{
	uint32_t num;

	get_random_bytes(&num, sizeof(num));
	return num;
}

static uint32_t dc_get_authorization_code(uint32_t data)
{
	uint32_t dwPolynomial = 0x04C11DB6, dwCrc = 0xFFFFFFFF;

	crcBits(data, &dwCrc, &dwPolynomial);
	dwCrc = crcWrap(dwCrc);
	return dwCrc;
}

int start_dc_auth(struct pd_direct_chrg *dc)
{
	uint32_t data[2];
	int ret = -1;

	dc->auth_pass = -1;

	data[0] = dc_get_random_code();
	data[1] = dc_get_random_code();
	dc->auth_code = dc_get_authorization_code(
			(data[0] & 0xffff) | (data[1] & 0xffff0000));

	ret = pd_send_vdm(dc->hba, RT7207_SVID, (OP_WRITE|CMD_AUTH), data, 2);

	/*pr_err("%s auth code auth_code=0x%04x 0x%04x 0x%04X\n", __func__, dc->auth_code, data[0], data[1]);*/

	return ret;
}

int handle_dc_auth(struct pd_direct_chrg *dc, int cnt, uint32_t *payload, uint32_t **rpayload)
{
	uint32_t data[2];
	int len = 0;

	if (cnt != 3) {
		pr_err("%s incorrect autho vdo number %d\n", __func__, cnt);
		dc->auth_pass = -1;
		goto end;
	}

	data[0] = payload[1];
	data[1] = payload[2];

	if (dc->auth_pass < 0) {
		if (dc->auth_code == data[0]) {
			*rpayload = payload;

			payload[0] = VDO(RT7207_SVID, 0, (OP_WRITE|CMD_AUTH));
			payload[1] = dc->auth_code = dc_get_authorization_code(data[1]);
			payload[2] = dc_get_random_code();

			len = 3;

			/*
			 * pr_err("%s auth code auth_code=0x%04x 0x%04x 0x%04X\n",
			 *				__func__, dc->auth_code, payload[1], payload[2]);
			*/

			dc->auth_pass = 0;
		} else {
			pr_err("%s auth fail @ step1 0x%04x 0x%04x\n", __func__, dc->auth_code, data[0]);

			set_state(dc->hba, PD_STATE_HARD_RESET_SEND);
		}
	} else if (dc->auth_pass == 0) {
		if (data[0] == 0x1) {
			dc->auth_pass = 1;
			pr_err("%s Pass RT7207 Auth\n", __func__);
		} else {
			dc->auth_pass = -1;
			pr_err("%s auth fail @ step2 0x%04x 0x%04x\n", __func__, dc->auth_code, data[0]);

			set_state(dc->hba, PD_STATE_HARD_RESET_SEND);
		}
	}
end:
	return len;
}

#ifdef CONFIG_DEBUG_FS
static int de_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t de_read(struct file *file,
		char __user *user_buffer, size_t count, loff_t *position)
{
	char tmp[200] = {0};

	sprintf(tmp, "RT7207 VDM API Test\n");

	return simple_read_from_buffer(user_buffer, count,
					position, tmp, strlen(tmp));
}

static ssize_t de_write(struct file *file,
		const char __user *user_buffer, size_t count, loff_t *position)
{
	struct pd_direct_chrg *dc = file->private_data;

	char buf[200] = {0};
	unsigned long yo;
	struct mtk_vdm_ta_cap cap;
	struct pd_ta_stat stat;
	int ret;

	simple_write_to_buffer(buf, sizeof(buf), position, user_buffer, count);
	ret = kstrtoul(buf, 16, &yo);

	switch (yo) {
	case 1:
		mtk_get_ta_id(NULL);
		break;
	case 2:
		mtk_get_ta_charger_status(dc, &stat);
		break;
	case 3:
		mtk_get_ta_current_cap(dc, &cap);
		break;
	case 4:
		mtk_get_ta_temperature(dc, &ret);
		break;
	case 5:
		tcpm_set_direct_charge_en(dc, true);
		break;
	case 6:
		tcpm_set_direct_charge_en(dc, false);
		break;
	case 7:
		cap.cur = 3000;
		cap.vol = 5000;
		mtk_set_ta_boundary_cap(dc, &cap);
		break;
	case 8:
		cap.cur = 1200;
		cap.vol = 3800;
		mtk_set_ta_cap(dc, &cap);
		break;
	case 9:
		mtk_set_ta_uvlo(dc, 3000);
		break;
	case 10:
		mtk_clr_ta_pingcheck_fault(dc);
		break;
	case 11:
		tcpm_hard_reset(dc);
		break;
	case 12:
		{
			int i = 0;
			struct mtk_vdm_ta_cap cap;

			tcpm_set_direct_charge_en(NULL, 1);

			for (i = 0; i < 100; i++) {
				pr_info("[%d]+\n", i);

				memset(&cap, 0, sizeof(struct mtk_vdm_ta_cap));

				mtk_get_ta_current_cap(NULL, &cap);
				pr_info("[%d]-\n", i);
			}

			tcpm_set_direct_charge_en(NULL, 0);
		}
		break;
	default:
		pr_info("%s unsupport cmd\n", __func__);
		break;
	}
	return count;
}

static const struct file_operations debugfs_fops = {
	.open = de_open,
	.read = de_read,
	.write = de_write,
};
#endif /* CONFIG_DEBUG_FS */

int mtk_direct_charge_vdm_init(void)
{
	struct pd_direct_chrg *dc = kzalloc(sizeof(struct pd_direct_chrg), GFP_KERNEL);
#ifdef CONFIG_DEBUG_FS
	struct dentry *file;
	int ret = 0;
#endif

	dc->hba = get_hba();
	dc->hba->dc = dc;
	g_dc = dc;

	if (!dc->dc_vdm_inited) {
		init_waitqueue_head(&dc->wq);

		mutex_init(&dc->vdm_event_lock);
		mutex_init(&dc->vdm_payload_lock);

		dc->data_in = false;
		dc->auth_pass = -1;
		dc->dc_vdm_inited = true;

#ifdef CONFIG_DEBUG_FS
		dc->root = debugfs_create_dir("rt7207", 0);
		if (!dc->root) {
			ret = -ENOMEM;
			goto err0;
		}

		file = debugfs_create_file("test", S_IRUGO|S_IWUSR, dc->root, dc, &debugfs_fops);
		if (!file) {
			ret = -ENOMEM;
			goto err1;
		}
#endif
		pr_info("%s init OK!\n", __func__);
	}
	return 0;

#ifdef CONFIG_DEBUG_FS
err1:
	debugfs_remove_recursive(dc->root);
err0:
	return ret;
#endif
}
EXPORT_SYMBOL(mtk_direct_charge_vdm_init);

int mtk_direct_charge_vdm_deinit(struct typec_hba *hba)
{
	struct pd_direct_chrg *dc = hba->dc;

	if (dc->dc_vdm_inited) {
		mutex_destroy(&dc->vdm_event_lock);
		mutex_destroy(&dc->vdm_payload_lock);
		dc->dc_vdm_inited = false;
#ifdef CONFIG_DEBUG_FS
		if (!IS_ERR(dc->root))
			debugfs_remove_recursive(dc->root);
#endif
	}
	return 0;
}
EXPORT_SYMBOL(mtk_direct_charge_vdm_deinit);
