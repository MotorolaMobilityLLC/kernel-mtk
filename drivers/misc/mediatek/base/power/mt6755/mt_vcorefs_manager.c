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

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/notifier.h>

#include "mt_vcorefs_manager.h"
#include "mt_spm_vcore_dvfs.h"
#include "mt_spm.h"

static DEFINE_MUTEX(vcorefs_mutex);

#define DEFINE_ATTR_RO(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0444,			\
	},					\
	.show	= _name##_show,			\
}

#define DEFINE_ATTR_RW(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

#define __ATTR_OF(_name)	(&_name##_attr.attr)

#ifdef CONFIG_MTK_RAM_CONSOLE
#define SPM_AEE_RR_REC 1
#else
#define SPM_AEE_RR_REC 0
#endif

struct vcorefs_profile {
	int plat_init_opp;
	bool init_done;
	bool autok_lock;
	u32 kr_req_mask;
};

static struct vcorefs_profile vcorefs_ctrl = {
	.plat_init_opp = 0,
	.init_done = 0,
	.autok_lock = 0,
	.kr_req_mask = 0,
};

/*
 * __nosavedata will not be restored after IPO-H boot
 */
static bool feature_en __nosavedata;

static int vcorefs_curr_opp __nosavedata = OPPI_PERF;
static int vcorefs_prev_opp __nosavedata = OPPI_PERF;

static int vcorefs_autok_lock_dvfs(int kicker, bool lock);
static int vcorefs_autok_set_vcore(int kicker, enum dvfs_opp opp);

static vcorefs_req_handler_t vcorefs_req_handler;

void vcorefs_register_req_notify(vcorefs_req_handler_t handler)
{
	vcorefs_req_handler = handler;
}
EXPORT_SYMBOL(vcorefs_register_req_notify);

/*
 * Manager extern API
 */
int is_vcorefs_can_work(void)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	int r = 0;

	mutex_lock(&vcorefs_mutex);
	if (pwrctrl->init_done && feature_en)
		r = 1;		/* ready to use vcorefs */
	else if (!is_vcorefs_feature_enable())
		r = -1;		/* not support vcorefs */
	else
		r = 0;		/* init not finish, please wait */
	mutex_unlock(&vcorefs_mutex);

	return r;
}

int vcorefs_get_curr_opp(void)
{
	int opp;

	mutex_lock(&vcorefs_mutex);
	opp = vcorefs_curr_opp;
	mutex_unlock(&vcorefs_mutex);

	return opp;
}

/*
 * Sub-main function
 */
static int _get_dvfs_opp(int kicker, struct vcorefs_profile *pwrctrl, enum dvfs_opp orig_opp)
{
	unsigned int opp = UINT_MAX;
	int i;
	char table[NUM_KICKER * 4 + 1];
	char *p = table;
	char *buff_end = table + (NUM_KICKER * 4 + 1);

	for (i = 0; i < NUM_KICKER; i++)
		p += snprintf(p, buff_end - p, "%d, ", kicker_table[i]);

	for (i = 0; i < NUM_KICKER; i++) {
		if (kicker_table[i] < 0)
			continue;

		if (kicker_table[i] < opp)
			opp = kicker_table[i];
	}

	/* if have no request, set to init OPP */
	if (opp == UINT_MAX)
		opp = pwrctrl->plat_init_opp;

	vcorefs_debug_mask(kicker, "kicker: %d, opp: %d, dvfs_opp: %d, curr_opp: %d kr opp: %s\n",
				kicker, orig_opp, opp, vcorefs_curr_opp, table);
	return opp;
}

static int kicker_request_compare(enum dvfs_kicker kicker, enum dvfs_opp opp)
{
	/* compare kicker table opp with request opp (except SYSFS) */
	if (opp == kicker_table[kicker] && kicker != KIR_SYSFS) {
		/* try again since previous change is partial success */
		if (vcorefs_curr_opp == vcorefs_prev_opp) {
			vcorefs_debug_mask(LAST_KICKER, "opp no change, kr_tb: %d, kr: %d, opp: %d\n",
				    kicker_table[kicker], kicker, opp);
			return -1;
		}
	}

	kicker_table[kicker] = opp;

	return 0;
}

static int kicker_request_mask(struct vcorefs_profile *pwrctrl, enum dvfs_kicker kicker,
			       enum dvfs_opp opp)
{
	if (pwrctrl->kr_req_mask & (1U << kicker)) {
		if (opp < 0)
			kicker_table[kicker] = opp;

		vcorefs_debug_mask(LAST_KICKER, "mask request, mask: 0x%x, kr: %d, opp: %d\n",
			    pwrctrl->kr_req_mask, kicker, opp);
		return -1;
	}

	return 0;
}

static void record_kicker_opp_in_aee(int kicker, int opp)
{
#if SPM_AEE_RR_REC
	u32 val;
	/* record opp table */
	val = aee_rr_curr_vcore_dvfs_opp();
	val = (opp == OPP_0) ? (val & ~(1 << kicker)) : (val | (1 << kicker));
	aee_rr_rec_vcore_dvfs_opp(val);

	/* record last kicker and opp */
	val = aee_rr_curr_vcore_dvfs_status();
	val &= ~(0xFF000000);
	val |= ((opp << 30) | (kicker << 24));
	aee_rr_rec_vcore_dvfs_status(val);
#endif
}

/*
 * Main function API (Called by kicker request for DVFS)
 * PASS: return 0
 * FAIL: return less than 0
 */
int vcorefs_request_dvfs_opp(enum dvfs_kicker kicker, enum dvfs_opp opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_config krconf;
	int r;
	int autok_r, autok_lock;

	if (is_vcorefs_feature_enable() && !pwrctrl->init_done) {
		vcorefs_debug_mask(kicker, "request before init done(kr:%d opp:%d)\n", kicker, opp);
		if (vcorefs_request_init_opp(kicker, opp))
			return 0;
	}

	if (!feature_en || !pwrctrl->init_done) {
		vcorefs_err("feature_en: %d, init_done: %d\n", feature_en, pwrctrl->init_done);
		return -1;
	}

	autok_r = governor_autok_check(kicker, opp);
	if (autok_r == true) {
		autok_lock = governor_autok_lock_check(kicker, opp);

		if (autok_lock) {
			vcorefs_autok_lock_dvfs(kicker, true);
			vcorefs_autok_set_vcore(kicker, opp);
		} else {
			vcorefs_autok_set_vcore(kicker, opp);
			if (OPPI_PERF == get_kicker_group_opp(KIR_SYSFS, -1))
				vcorefs_autok_set_vcore(KIR_SYSFS, OPPI_PERF);
			vcorefs_autok_lock_dvfs(kicker, false);
		}
		return 0;
	}

	if (pwrctrl->autok_lock) {
		vcorefs_err("autoK lock: %d, Not allow kr: %d, opp: %d\n", pwrctrl->autok_lock,
			    kicker, opp);
		return -1;
	}

	if (kicker_request_mask(pwrctrl, kicker, opp))
		return -1;

	if (kicker_request_compare(kicker, opp))
		return 0; /* already request, return OK */

	mutex_lock(&vcorefs_mutex);

	krconf.kicker = kicker;
	krconf.opp = opp;
	krconf.dvfs_opp = _get_dvfs_opp(kicker, pwrctrl, opp);

	record_kicker_opp_in_aee(kicker, opp);
	if (vcorefs_req_handler)
		vcorefs_req_handler(kicker, opp);

	/*
	 * r = 0: DVFS completed
	 *
	 * LPM to HPM:
	 *      r = -1: step1 DVS fail
	 *      r = -2: step2 DFS fail
	 */
	r = kick_dvfs_by_opp_index(&krconf);

	if (r == 0) {
		vcorefs_prev_opp = krconf.dvfs_opp;
		vcorefs_curr_opp = krconf.dvfs_opp;
	} else if (r < 0) {
		vcorefs_err("kicker: %d, Vcore DVFS Fail, r: %d\n", kicker, r);

		/* if (r == -2) no change */
		if (r == -3) {
			vcorefs_prev_opp = vcorefs_curr_opp;
			vcorefs_curr_opp = krconf.dvfs_opp;
		}
	} else {
		vcorefs_err("kicker: %d, unknown error handling, r: %d\n", kicker, r);
		BUG();
	}
	mutex_unlock(&vcorefs_mutex);

	return r;
}

/*
 * SDIO AutoK related API
 */
int vcorefs_autok_lock_dvfs(int kicker, bool lock)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	int r = 0;

	mutex_lock(&vcorefs_mutex);
	vcorefs_crit("autok kicker %d set lock: %d\n", kicker, lock);
	pwrctrl->autok_lock = lock;
	mutex_unlock(&vcorefs_mutex);

	return r;
}

int vcorefs_autok_set_vcore(int kicker, enum dvfs_opp opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_config krconf;
	int r = 0;

	if (opp >= NUM_OPP || !pwrctrl->autok_lock) {
		vcorefs_err("set vcore fail, opp: %d, autok_lock: %d\n", opp, pwrctrl->autok_lock);
		return -1;
	}

	if (vcorefs_gpu_get_init_opp() == OPPI_PERF && opp == OPPI_LOW_PWR) {
		vcorefs_err("autok kicker:%d set vcore fail due to init_opp request OPPI_PERF\n", kicker);
		return -1;
	}

	mutex_lock(&vcorefs_mutex);
	krconf.kicker = kicker;
	krconf.opp = opp;
	krconf.dvfs_opp = opp;

	vcorefs_crit("autok kicker: %d, opp: %d, dvfs_opp: %d, curr_opp: %d\n",
		     krconf.kicker, krconf.opp, krconf.dvfs_opp, vcorefs_curr_opp);

	/*
	 * r = 0: DVFS completed
	 *
	 * LPM to HPM:
	 *      r = -1: step1 DVS fail
	 *      r = -2: step2 DFS fail
	 */
	r = kick_dvfs_by_opp_index(&krconf);
	mutex_unlock(&vcorefs_mutex);

	return r;
}

/*
 * Called by governor for init flow
 */
void vcorefs_drv_init(bool plat_init_done, bool plat_feature_en, int plat_init_opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	int i;

	mutex_lock(&vcorefs_mutex);
	if (!plat_feature_en)
		feature_en = 0;
	else
		feature_en = 1;

	for (i = 0; i < NUM_KICKER; i++)
		kicker_table[i] = -1;
	aee_rr_rec_vcore_dvfs_opp(0xffffffff);

	if (vcorefs_gpu_get_init_opp() == OPPI_PERF) {
		/* GPU kicker already request HPM in init */
		kicker_table[KIR_GPU] = OPPI_PERF;
		aee_rr_rec_vcore_dvfs_opp(0xfffffeff);
	}

	vcorefs_curr_opp = plat_init_opp;
	pwrctrl->plat_init_opp = plat_init_opp;
	pwrctrl->init_done = plat_init_done;
	mutex_unlock(&vcorefs_mutex);

	governor_autok_manager();
	vcorefs_gpu_set_init_opp(OPPI_UNREQ);
}

void vcorefs_set_feature_en(bool enable)
{
	feature_en = enable;
}

void vcorefs_set_kr_req_mask(unsigned int mask)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;

	pwrctrl->kr_req_mask = mask;
}
/*
 * Vcorefs debug sysfs
 */
static char *vcorefs_get_kicker_info(char *p)
{
	int i;
	char *buff_end = p + PAGE_SIZE;

	for (i = 0; i < NUM_KICKER; i++)
		p += snprintf(p, buff_end - p,
			"[%.32s] opp: %d\n", get_kicker_name(i), kicker_table[i]);

	return p;
}

static ssize_t vcore_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	char *p = buf;
	char *buff_end = buf + PAGE_SIZE;

	p += snprintf(p, buff_end - p, "\n");

	p += snprintf(p, buff_end - p,
		"[feature_en   ]: %d\n", feature_en);
	p += snprintf(p, buff_end - p,
		"[plat_init_opp]: %d\n", pwrctrl->plat_init_opp);
	p += snprintf(p, buff_end - p,
		"[init_done    ]: %d\n", pwrctrl->init_done);
	p += snprintf(p, buff_end - p,
		"[autok_lock   ]: %d\n", pwrctrl->autok_lock);
	p += snprintf(p, buff_end - p,
		"[kr_req_mask  ]: 0x%x\n", pwrctrl->kr_req_mask);
	p += snprintf(p, buff_end - p, "\n");

	p += snprintf(p, buff_end - p, "curr_opp: %d\n", vcorefs_curr_opp);
	p += snprintf(p, buff_end - p, "prev_opp: %d\n", vcorefs_prev_opp);
	p += snprintf(p, buff_end - p, "\n");

	p = vcorefs_get_dvfs_info(p);
	p += snprintf(p, buff_end - p, "\n");

	p = vcorefs_get_kicker_info(p);
	p += snprintf(p, buff_end - p, "\n");

	p += snprintf(p, buff_end - p,
		" [aee] vcore_dvfs_opp    = 0x%x\n", aee_rr_curr_vcore_dvfs_opp());
	p += snprintf(p, buff_end - p,
		" [aee] vcore_dvfs_status = 0x%x\n", aee_rr_curr_vcore_dvfs_status());
	p += snprintf(p, buff_end - p, "\n");

#ifndef CONFIG_MTK_FPGA
	p = spm_vcorefs_dump_dvfs_regs(p);
#endif

	return p - buf;
}

static ssize_t vcore_debug_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_config krconf;
	int kicker, val, r, opp;
	char cmd[32];
	int change = 0;

	/* governor debug sysfs */
	r = governor_debug_store(buf);
	if (!r)
		return count;

	if (!(sscanf(buf, "%31s 0x%x", cmd, &val) == 2 ||
		   sscanf(buf, "%31s %d", cmd, &val) == 2)) {
		return -EPERM;
	}

	if (!strcmp(cmd, "feature_en")) {
		mutex_lock(&vcorefs_mutex);

		if (val && (!feature_en)) {
			kicker_table[KIR_SYSFS] = OPP_OFF;
			opp = get_kicker_group_opp(KIR_SYSFS, -1);
			change = 1;
		} else if ((!val) && feature_en) {
			kicker_table[KIR_SYSFS] = OPP_0;
			opp = OPP_0;
			change = 1;
		} else {
			change = 0;
		}

		if (change == 1) {
			krconf.kicker = KIR_SYSFS;
			krconf.opp = opp;
			krconf.dvfs_opp = opp;

			vcorefs_crit("kicker: %d, opp: %d, dvfs_opp: %d, curr_opp: %d\n",
				     krconf.kicker, krconf.opp, krconf.dvfs_opp, vcorefs_curr_opp);

			/*
			 * r = 0: DVFS completed
			 *
			 * LPM to HPM:
			 *      r = -1: step1 DVS fail
			 *      r = -2: step2 DFS fail
			 */
			r = kick_dvfs_by_opp_index(&krconf);

			vcorefs_curr_opp = krconf.dvfs_opp;
			feature_en = !!(val);
		}
		mutex_unlock(&vcorefs_mutex);
	} else if (!strcmp(cmd, "kr_req_mask")) {
		mutex_lock(&vcorefs_mutex);
		pwrctrl->kr_req_mask = val;
		mutex_unlock(&vcorefs_mutex);
	} else if (!strcmp(cmd, "KIR_SYSFSX") && (val >= OPP_OFF && val < NUM_OPP)) {
		mutex_lock(&vcorefs_mutex);
		if (val != OPP_OFF)
			pwrctrl->kr_req_mask = (1U << NUM_KICKER) - 1;
		else
			pwrctrl->kr_req_mask = 0;

		krconf.kicker = KIR_SYSFSX;
		krconf.opp = val;
		krconf.dvfs_opp = val;
		vcorefs_debug_mask(krconf.kicker, "kicker: %d, opp: %d, dvfs_opp: %d, curr_opp: %d\n",
			     krconf.kicker, krconf.opp, krconf.dvfs_opp, vcorefs_curr_opp);

		/*
		 * r = 0: DVFS completed
		 *
		 * LPM to HPM:
		 *      r = -1: step1 DVS fail
		 *      r = -2: step2 DFS fail
		 */
		r = kick_dvfs_by_opp_index(&krconf);
		mutex_unlock(&vcorefs_mutex);
	} else if (!strcmp(cmd, "KIR_AUTOK_SDIO")) {
		kicker = KIR_AUTOK_SDIO;
		vcorefs_request_dvfs_opp(kicker, val);
	} else {
		/* set kicker opp and do DVFS */
		kicker = vcorefs_output_kicker_id(cmd);
		if (kicker != -1)
			vcorefs_request_dvfs_opp(kicker, val);
	}

	return count;
}

static ssize_t opp_table_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *p = buf;

	p = vcorefs_get_opp_table_info(p);

	return p - buf;
}

static ssize_t opp_table_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	int val;
	char cmd[32];

	if (sscanf(buf, "%31s %u", cmd, &val) != 2)
		return -EPERM;

	vcorefs_crit("opp_table: cmd: %s, val: %d (0x%x)\n", cmd, val, val);

	mutex_lock(&vcorefs_mutex);
	if (!feature_en)
		vcorefs_update_opp_table(cmd, val);
	mutex_unlock(&vcorefs_mutex);

	return count;
}

static ssize_t opp_num_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *p = buf;
	int num = vcorefs_get_num_opp();
	char *buff_end = p + PAGE_SIZE;

	p += snprintf(p, buff_end - p, "%d\n", num);

	return p - buf;
}

DEFINE_ATTR_RW(vcore_debug);
DEFINE_ATTR_RW(opp_table);
DEFINE_ATTR_RO(opp_num);

static struct attribute *vcorefs_attrs[] = {
	__ATTR_OF(opp_table),
	__ATTR_OF(vcore_debug),
	__ATTR_OF(opp_num),
	NULL,
};

static struct attribute_group vcorefs_attr_group = {
	.name = "vcorefs",
	.attrs = vcorefs_attrs,
};

int init_vcorefs_sysfs(void)
{
	int r;

	r = sysfs_create_group(power_kobj, &vcorefs_attr_group);

	return 0;
}
