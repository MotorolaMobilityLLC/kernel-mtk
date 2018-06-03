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

#include <linux/version.h>
#include "mtk_typec.h"
#include "typec-ioctl.h"
#include "typec_reg.h"

static ssize_t enable_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%s VBus detection\n", hba->vbus_det_en?"Enable":"Disable");
}

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int enabled = 0;

	if (kstrtoint(buff, 0, &enabled) == 0)
		typec_enable(hba, !!enabled);

	return size;
}

static const char * const string_typec_role[] = {
	"SINK",
	"SOURCE",
	"DRP",
	"RESERVED",
	"SINK_W_ACTIVE_CABLE",
	"ACCESSORY_AUDIO",
	"ACCESSORY_DEBUG",
	"OPEN",
};

static const char * const string_rp[] = {
	"DFT", /*36K ohm*/
	"1A5", /*12K ohm*/
	"3A0", /*4.7K ohm*/
	"RESERVED",
};

static ssize_t mode_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "Role=%s, Rp=%s\n",
			string_typec_role[hba->support_role], string_rp[hba->rp_val]);
}

static ssize_t mode_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int mode = 0;
	int param1 = 0;
	int param2 = 0;

	if (sscanf(buff, "%d %d %d", &mode, &param1, &param2) == 3) {

		dev_err(pdev, "mode=%s p1=%d p2=%d\n", string_typec_role[mode], param1, param2);

		typec_enable(hba, 0);
		typec_set_mode(hba, mode, param1, param2);
		typec_enable(hba, 1);
	}

	return size;
}

static ssize_t rp_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "Rp=%s\n", string_rp[hba->rp_val]);
}

static ssize_t rp_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int rp_val = 0;

	if (kstrtoint(buff, 0, &rp_val) == 0) {

		dev_err(pdev, "rp_val=%s\n", string_rp[rp_val]);

		typec_select_rp(hba, rp_val);
		hba->rp_val = rp_val;
	}

	return size;
}

static ssize_t dump_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	uint32_t addr;
	uint32_t val1, val2, val3, val4;

#if SUPPORT_PD
#define MAX_REG 0x200
#else
#define MAX_REG 0x100
#endif
	const char *title = "        03    00 07    04 0B    08 0F    0C\n";
	const char *divider = "===========================================\n";

	dev_err(pdev, "%s%s", title, divider);
	snprintf(buf + strlen(buf), PAGE_SIZE, "%s%s", title, divider);

	for (addr = 0; addr <= MAX_REG; addr += 0x10) {
		uint32_t real_addr = addr + CC_REG_BASE;

		val1 = typec_readw(hba, real_addr+2)<<16 | typec_readw(hba, real_addr);
		val2 = typec_readw(hba, real_addr+6)<<16 | typec_readw(hba, real_addr+4);
		val3 = typec_readw(hba, real_addr+10)<<16 | typec_readw(hba, real_addr+8);
		val4 = typec_readw(hba, real_addr+14)<<16 | typec_readw(hba, real_addr+12);

		dev_err(pdev, "0x%03x %08X %08X %08X %08X\n", addr, val1, val2, val3, val4);
		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "0x%03x  %08X %08X %08X %08X\n",
				addr, val1, val2, val3, val4);
	}

	return strlen(buf);
}

static int g_read_val;

static ssize_t read_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	/*struct typec_hba *hba = dev_get_drvdata(pdev);*/

	return snprintf(buf, PAGE_SIZE, "val=0x%x\n", g_read_val);
}

static ssize_t read_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int len = 1;
	int addr = 0;

	if (sscanf(buff, "%d 0x%x", &len, &addr) == 2) {
		if (len == 1)
			g_read_val = typec_read8(hba, addr);
		else if (len == 2)
			g_read_val = typec_readw(hba, addr);
		dev_err(pdev, "Read addr=0x%x val=0x%x\n", addr, g_read_val);
	} else if (kstrtoint(buff, 0, &addr) == 0) {
		g_read_val = typec_read8(hba, addr);
		dev_err(pdev, "Read8 addr=0x%x val=0x%x\n", addr, g_read_val);
	}

	return size;
}

static ssize_t write_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int addr = 0;
	int val = 0;
	int len = 1;

	if (sscanf(buff, "0x%x 0x%x", &addr, &val) == 2) {
		dev_err(pdev, "Write8 addr=0x%x val=0x%x\n", addr, val);
		typec_writew(hba, val, addr);
	} else if (sscanf(buff, "%d 0x%x 0x%x", &len, &addr, &val) == 3) {
		if (len == 1) {
			dev_err(pdev, "Write8 addr=0x%x val=0x%x\n", addr, val);
			typec_write8(hba, val, addr);
		} else if (len == 2) {
			dev_err(pdev, "Write addr=0x%x val=0x%x\n", addr, val);
			typec_writew(hba, val, addr);
		}
	}
	return size;
}

static ssize_t dbg_lvl_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "debug level=%d\n", hba->dbg_lvl);
}

static ssize_t dbg_lvl_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int lvl = 0;

	if (kstrtoint(buff, 0, &lvl) == 0) {
		hba->dbg_lvl = lvl;
		dev_err(pdev, "debug level=%d\n", hba->dbg_lvl);
	}

	return size;
}

static ssize_t vbus_det_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int en = 0;

	if (kstrtoint(buff, 0, &en) == 0)
		typec_vbus_det_enable(hba, en);

	return size;
}

static const char * const string_typec_state[] = {
	"DISABLED",
	"UNATTACHED_SRC",
	"ATTACH_WAIT_SRC",
	"ATTACHED_SRC",
	"UNATTACHED_SNK",
	"ATTACH_WAIT_SNK",
	"ATTACHED_SNK",
	"TRY_SRC",
	"TRY_WAIT_SNK",
	"UNATTACHED_ACCESSORY",
	"ATTACH_WAIT_ACCESSORY",
	"AUDIO_ACCESSORY",
	"DEBUG_ACCESSORY"
};

static const char * const string_cc_routed[] = {
	"CC1",
	"CC2"
};

static const char * const string_sink_power[] = {
	"IDLE",
	"DEFAULT",
	"1A5",
	"3A0",
};

static const char * const string_power_role[] = {
	"SNK",
	"SRC",
	"NR",
};

static const char * const string_data_role[] = {
	"UFP",
	"DFP",
	"NR",
};

static ssize_t stat_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	const struct reg_mapping dump[] = {
		{TYPE_C_CC_STATUS, RO_TYPE_C_CC_ST, RO_TYPE_C_CC_ST_OFST, "cc_st"},
		{TYPE_C_CC_STATUS, RO_TYPE_C_ROUTED_CC, RO_TYPE_C_ROUTED_CC_OFST, "routed_cc"},
		{TYPE_C_PWR_STATUS, RO_TYPE_C_CC_SNK_PWR_ST, RO_TYPE_C_CC_SNK_PWR_ST_OFST, "cc_snk_pwr_st"},
		{TYPE_C_PWR_STATUS, RO_TYPE_C_CC_PWR_ROLE, RO_TYPE_C_CC_PWR_ROLE_OFST, "cc_pwr_role"},
	};

	int i = 0;
	int val[sizeof(dump) / sizeof(struct reg_mapping)] = {0};

	for (i = 0; i < sizeof(dump) / sizeof(struct reg_mapping); i++)
		val[i] = ((typec_readw(hba, dump[i].addr) & dump[i].mask) >> dump[i].ofst);

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "%s=%s,", dump[0].name, string_typec_state[val[0]]);
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "%s=%s,", dump[1].name, string_cc_routed[val[1]]);
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "%s=%s,", dump[2].name, string_sink_power[val[2]]);

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "support_role=%s\n",
							string_typec_role[hba->support_role]);

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "rp_val=%s\n", string_rp[hba->rp_val]);
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "dbg_lvl=%d\n", hba->dbg_lvl);

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "vbus_en=%d\n", hba->vbus_en);
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "vbus_det_en=%d\n", hba->vbus_det_en);
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "vbus_present=%d\n", hba->vbus_present);
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "vconn_en=%d\n", hba->vconn_en);

	if (hba->mode == 2) {
		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "power_role=%s\n",
							string_power_role[hba->power_role]);
	}

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "data_role=%s\n", string_data_role[hba->data_role]);

	if (hba->power_role == PD_ROLE_SINK)
		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Rp=%s\n", SRC_CUR(hba->src_rp));

	if (hba->mode == 2) {
		if (hba->cable_flags & PD_FLAGS_CBL_DISCOVERIED_SOP_P)
			snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "e-mark cable\n");

		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "flags=0x%x\n", hba->flags);

		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "task_state=%d\n", hba->task_state);
		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "last_state=%d\n", hba->last_state);
		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "timeout_state=%d\n", hba->timeout_state);
		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "timeout_ms=%lu\n", hba->timeout_ms);

		if (hba->power_role == PD_ROLE_SOURCE)
			snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "request pdo=%d\n", hba->requested_idx);

		if (hba->data_role == PD_ROLE_DFP)
			snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)),
						"alt_mode_svid=0x%04X\n", hba->alt_mode_svid);

		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "wq_running=0x%X\n", hba->wq_running);
		snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "wq_cnt=0x%X\n", hba->wq_cnt);
	}
#if !COMPLIANCE
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "LowQ=%d\n", (int)atomic_read(&hba->lowq_cnt));
#endif
	return strlen(buf);
}

#if SUPPORT_PD
static ssize_t pd_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	char arg1[10] = {0};
	char arg2[10] = {0};
	char arg3[10] = {0};
	int val = 0;

	val = sscanf(buff, "%s %s %s", arg1, arg2, arg3);

	dev_err(pdev, "Arg %d [1]%s [2]%s [3]%s\n", val, arg1, arg2, arg3);

	switch (val) {
	case 1:
		if (!strncasecmp(arg1, "getsrccap", 9))
			send_control(hba, PD_CTRL_GET_SOURCE_CAP);
		else if (!strncasecmp(arg1, "getsnkcap", 9))
			send_control(hba, PD_CTRL_GET_SINK_CAP);
		break;
	case 2:
		if (!strncasecmp(arg1, "comm", 4)) {
			if (!strncasecmp(arg2, "on", 2))
				pd_rx_enable(hba, 1);
			else if (!strncasecmp(arg2, "off", 3))
				pd_rx_enable(hba, 0);
			else
				dev_err(pdev, "comm [on|off]\n");

		} else if (!strncasecmp(arg1, "ping", 4)) {
			if (!strncasecmp(arg2, "on", 2))
				pd_ping_enable(hba, 1);
			else if (!strncasecmp(arg2, "off", 3))
				pd_ping_enable(hba, 0);
			else
				dev_err(pdev, "ping [on|off]\n");

		} else if (!strncasecmp(arg1, "bist", 4)) {
			/*int cnt = simple_strtoul(arg2, NULL, 10);*/
			int cnt = -1;

			if (kstrtoint(arg2, 0, &cnt) != 0)
				cnt = -1;

			if (cnt == 0) {
				hba->bist_mode = BDO_MODE_CARRIER2;
				set_state(hba, PD_STATE_BIST_CMD);
			} else if (cnt == 1) {
				hba->bist_mode = BDO_MODE_TEST_DATA;
				set_state(hba, PD_STATE_BIST_CMD);
			} else {
				dev_err(pdev, "bist [0|1]\n");
			}

		} else if (!strncasecmp(arg1, "timeout", 7)) {
			int tmp = 0;

			if (kstrtoint(arg2, 0, &tmp) == 0) {
				hba->timeout_user = tmp;
				dev_err(hba->dev, "set timeout to %lu\n", hba->timeout_user);
			}
		} else if (!strncasecmp(arg1, "soft", 4)) {
#if RESET_STRESS_TEST
			/*int cnt = simple_strtol(arg2, NULL, 10);*/
			int cnt = 0;
			int i = 0;

			if (kstrtoint(arg2, 0, &cnt) != 0)
				cnt = 0;

			for (; i < cnt; i++) {
				dev_err(hba->dev, "%d/%d\n", (i+1), cnt);
#endif
				set_state(hba, PD_STATE_SOFT_RESET);
#if RESET_STRESS_TEST
				if (!wait_for_completion_timeout(&hba->ready, msecs_to_jiffies(PD_STRESS_DELAY)))
					dev_err(hba->dev, "SOFT RESET timeout\n");
			}
#endif
		} else if (!strncasecmp(arg1, "hard", 4)) {
#if RESET_STRESS_TEST
			/*int cnt = simple_strtol(arg2, NULL, 10);*/
			int cnt = 0;
			int i = 0;

			if (kstrtoint(arg2, 0, &cnt) != 0)
				cnt = 0;

			for (; i < cnt; i++) {
				dev_err(hba->dev, "%d/%d\n", (i+1), cnt);
#endif
				set_state(hba, PD_STATE_HARD_RESET_SEND);
#if RESET_STRESS_TEST

				if (!wait_for_completion_timeout(&hba->ready, msecs_to_jiffies(PD_STRESS_DELAY)))
					dev_err(hba->dev, "HARD RESET timeout\n");
			}
#endif
		}
		break;
	case 3:
		if (!strncasecmp(arg1, "swap", 4)) {
#if RESET_STRESS_TEST
			/*int cnt = simple_strtol(arg3, NULL, 10);*/
			int cnt = 0;
			int i = 0;

			if (kstrtoint(arg3, 0, &cnt) != 0)
				cnt = 0;

			for (; i < cnt; i++) {
				dev_err(hba->dev, "%d/%d\n", (i+1), cnt);
#endif
				if (!strncasecmp(arg2, "power", 5)) {
					pd_request_power_swap(hba);

					/*Tigger pd_task to do PR_SWAP*/
					hba->rx_event = true;
					wake_up(&hba->wq);
				} else if (!strncasecmp(arg2, "data", 4)) {
					pd_request_data_swap(hba);

					/*Tigger pd_task to do DR_SWAP*/
					hba->rx_event = true;
					wake_up(&hba->wq);
				} else if (!strncasecmp(arg2, "vconn", 5)) {
					pd_request_vconn_swap(hba);

					/*Tigger pd_task to do VCONN_SWAP*/
					hba->rx_event = true;
					wake_up(&hba->wq);
				} else
					dev_err(pdev, "swap [power|data|vconn] [0-9]+\n");
#if RESET_STRESS_TEST
				if (!wait_for_completion_timeout(&hba->ready, msecs_to_jiffies(PD_STRESS_DELAY)))
					dev_err(hba->dev, "SWAP ACTION timeout\n");
			}
#endif
		}
		break;
	default:
		dev_err(pdev, "Nothing can do!\n");
		break;
	}
	return size;
}
#endif

static ssize_t vbus_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);

#ifdef NEVER
	return snprintf(buf, PAGE_SIZE, "Vbus is %dmV %dmV, vbus_en=%d\n",
				vbus_val(hba), vbus_val_self(hba), hba->vbus_en);
#endif /* NEVER */
	return snprintf(buf, PAGE_SIZE, "Vbus is %dmV, vbus_en=%d\n",
			vbus_val(hba), hba->vbus_en);

}

static ssize_t vbus_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int on = 0;

	if (kstrtoint(buff, 0, &on) == 0) {
		typec_drive_vbus(hba, !!on);
		dev_err(pdev, "Turn %s Vbus\n", ((!!on)?"_ON_":"_OFF_"));
	}

	return size;
}

#if SUPPORT_PD
static ssize_t vconn_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "vconn_en=%d\n", hba->vconn_en);
}

static ssize_t vconn_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	int on = 0;

	if (kstrtoint(buff, 0, &on) == 0) {
		typec_drive_vconn(hba, !!on);
		dev_err(pdev, "Turn %s Vconn\n", ((!!on)?"_ON_":"_OFF_"));
	}

	return size;
}

#ifdef SUPPORT_SOP_P
static const char * const string_product_type[] = {
	"Undefined",
	"Hub",
	"Peripheral",
	"Passive Cable",
	"Active Cable",
	"Alternate Mode Adapter",
	"Reserved",
};

static const char * const string_cable_type[] = {
	"Type-A",
	"Type-B",
	"Type-C",
	"Captive",
};

static const char * const string_cable_term[] = {
	"Both ends Passive, VCONN not required",
	"Both ends Passive, VCONN required",
	"One end Active, one end passive, VCONN required",
	"Both ends Active, VCONN required",
};

static const char * const string_vbus_cap[] = {
	"VBUS not through cable",
	"3A",
	"5A",
	"Reserved",
};

static const char * const string_usb_speed[] = {
	"USB 2.0 only",
	"USB 3.1 Gen1",
	"USB 3.1 Gen1 & Gen2",
	"Reserved",
};

static ssize_t cable_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	struct cable_info *cbl_inf = NULL;

	dev_err(hba->dev, "cable_flags=0x%x\n", hba->cable_flags);

	if (hba->cable_flags & PD_FLAGS_CBL_DISCOVERIED_SOP_P)
		cbl_inf = &hba->sop_p;
	else {
		snprintf(buf, PAGE_SIZE, "No cable info\n");
		goto end;
	}

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "---ID Header---\n");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Data Capable as USB Host:%s\n",
		PD_IDH_USB_HOST(cbl_inf->id_header)?"YES":"NO");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Data Capable as USB Device:%s\n",
		PD_IDH_USB_DEVICE(cbl_inf->id_header)?"YES":"NO");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Product Type=%s\n",
		string_product_type[PD_IDH_PTYPE(cbl_inf->id_header)]);

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Modal Operation Supported:%s\n",
		PD_IDH_MODAL(cbl_inf->id_header)?"YES":"NO");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "USB Vendor ID = 0x%04X\n",
		PD_IDH_VID(cbl_inf->id_header));

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "---Cert Stat---\n");
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "TID = 0x%05X\n",
		PD_CSTAT_TID(cbl_inf->cer_stat_vdo));

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "---Product---\n");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "USB Product ID = 0x%04X\n",
		PD_PRODUCT_PID(cbl_inf->product_vdo));

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "bcdDevice = 0x%04X\n",
		PD_PRODUCT_BCD(cbl_inf->product_vdo));

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "---Cable---\n");
	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "HW Version = 0x%01X\n",
		PD_CABLE_HW_VER(cbl_inf->cable_vdo));

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "FW Version = 0x%01X\n",
		PD_CABLE_FW_VER(cbl_inf->cable_vdo));

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Type-C plug to %s %s\n",
		string_cable_type[PD_CABLE_CABLE_TYPE(cbl_inf->cable_vdo)],
		PD_CABLE_PLUG_OR_REC(cbl_inf->cable_vdo)?"Receptacle":"Plug");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Cable Latency = 0x%01X\n",
		PD_CABLE_LATENCY(cbl_inf->cable_vdo));

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "Cable Termination Type:%s\n",
		string_cable_term[PD_CABLE_TERM_TYPE(cbl_inf->cable_vdo)]);

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "SSTX1 Directionality Support : %s\n",
		PD_CABLE_SSTX1(cbl_inf->cable_vdo)?"Configurable":"Fixed");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "SSTX2 Directionality Support : %s\n",
		PD_CABLE_SSTX2(cbl_inf->cable_vdo)?"Configurable":"Fixed");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "SSRX1 Directionality Support : %s\n",
		PD_CABLE_SSRX1(cbl_inf->cable_vdo)?"Configurable":"Fixed");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "SSRX2 Directionality Support : %s\n",
		PD_CABLE_SSRX2(cbl_inf->cable_vdo)?"Configurable":"Fixed");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "VBUS Current Handling Capability : %s\n",
		string_vbus_cap[PD_CABLE_VBUS_CAP(cbl_inf->cable_vdo)]);

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "VBUS through cable : %s\n",
		PD_CABLE_VBUS_THRO(cbl_inf->cable_vdo)?"YES":"NO");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "SOP'' controller present? : %s\n",
		PD_CABLE_SOP_PP(cbl_inf->cable_vdo)?"YES":"NO");

	snprintf(buf + strlen(buf), (PAGE_SIZE - strlen(buf)), "USB Superspeed Signaling Support : %s\n",
		string_usb_speed[PD_CABLE_USB_SPEED(cbl_inf->cable_vdo)]);
end:
	return strlen(buf);
}

static ssize_t cable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct typec_hba *hba = dev_get_drvdata(pdev);
	char arg[10] = {0};
	int val = 0;

	val = sscanf(buff, "%s", arg);

	if (val == 1) {
		if (!strncasecmp(arg, "sop_p", 5))
			set_state(hba, PD_STATE_DISCOVERY_SOP_P);
		else
			dev_err(pdev, "invalid argument\n");
	} else
		dev_err(pdev, "invalid argument\n");

	return size;
}
#endif
#endif

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR, mode_show, mode_store);
static DEVICE_ATTR(rp, S_IRUGO | S_IWUSR, rp_show, rp_store);
static DEVICE_ATTR(dump, S_IRUGO, dump_show, NULL);
static DEVICE_ATTR(read, S_IRUGO | S_IWUSR, read_show, read_store);
static DEVICE_ATTR(write, S_IWUSR, NULL, write_store);
static DEVICE_ATTR(dbg_lvl, S_IRUGO | S_IWUSR, dbg_lvl_show, dbg_lvl_store);
static DEVICE_ATTR(vbus_det, S_IWUSR, NULL, vbus_det_store);
static DEVICE_ATTR(stat, S_IRUGO, stat_show, NULL);
static DEVICE_ATTR(vbus, S_IRUGO | S_IWUSR, vbus_show, vbus_store);

#if SUPPORT_PD
static DEVICE_ATTR(pd, S_IWUSR, NULL, pd_store);
static DEVICE_ATTR(vconn, S_IRUGO | S_IWUSR, vconn_show, vconn_store);
#ifdef SUPPORT_SOP_P
static DEVICE_ATTR(cable, S_IRUGO | S_IWUSR, cable_show, cable_store);
#endif
#endif

static struct device_attribute *mt_typec_attributes[] = {
	&dev_attr_enable,
	&dev_attr_mode,
	&dev_attr_rp,
	&dev_attr_dump,
	&dev_attr_read,
	&dev_attr_write,
	&dev_attr_dbg_lvl,
	&dev_attr_vbus_det,
	&dev_attr_stat,
#if SUPPORT_PD
	&dev_attr_pd,
	&dev_attr_vconn,
#ifdef SUPPORT_SOP_P
	&dev_attr_cable,
#endif
#endif
	&dev_attr_vbus,
	NULL
};

static struct class *typec_cdev_class;
static unsigned int typec_cdev_major;

int typec_cdev_init(struct device *parent, struct typec_hba *hba, int id)
{
	int err;
	struct device *device;
	int minor;
	struct device_attribute **attrs = mt_typec_attributes;
	struct device_attribute *attr;

	/*minor = typec_cdev_get_minor();*/
	minor = id;

	device = device_create(typec_cdev_class, parent,
			MKDEV(typec_cdev_major, minor), hba, "typec%u", minor);
	if (IS_ERR(device)) {
		dev_err(parent, "device_create filed\n");
		err = PTR_ERR(device);
		goto out_cdev_del;
	}

	dev_set_drvdata(device, hba);

	while ((attr = *attrs++)) {
		err = device_create_file(device, attr);
		if (err) {
			device_destroy(typec_cdev_class, device->devt);
			return err;
		}
	}

	return 0;

out_cdev_del:

	return err;
}

void typec_cdev_remove(struct typec_hba *hba)
{
	/*FIXME: How to do this?*/
	/*device_destroy(typec_cdev_class, MKDEV(typec_cdev_major, minor));*/
}

int typec_cdev_module_init(void)
{
	int error;

	typec_cdev_major = 0;

	typec_cdev_class = class_create(THIS_MODULE, "typec");
	if (IS_ERR(typec_cdev_class)) {
		error = PTR_ERR(typec_cdev_class);
		pr_err("failed to register with sysfs\n");
		goto out_unreg_chrdev;
	}

#ifdef INMEM_LOG
	/*in mem logger buffer*/
	inmem_logger_buf = vzalloc(INMEM_LOGGER_BUF_SIZE);
#endif

	return 0;

out_unreg_chrdev:

	return error;
}

void typec_cdev_module_exit(void)
{
	class_destroy(typec_cdev_class);

#ifdef INMEM_LOG
	vfree(inmem_logger_buf);
#endif
}

