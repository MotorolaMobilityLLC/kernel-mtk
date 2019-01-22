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

#include <mt-plat/upmu_common.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/dcache.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include "include/pmic_lbat_service.h"

#define VOLT_TO_THD(volt) ((volt) * 4096 / 5400)
#define USER_SIZE 16

#define LBAT_SERVICE_DBG 0

static DEFINE_MUTEX(lbat_mutex);
static struct list_head lbat_hv_list = LIST_HEAD_INIT(lbat_hv_list);
static struct list_head lbat_lv_list = LIST_HEAD_INIT(lbat_lv_list);
static unsigned int user_count;

struct lbat_thd_t {
	bool active;
	unsigned int thd;
	struct lbat_user *user;
	struct list_head list;
};

static struct lbat_thd_t *cur_hv_ptr;
static struct lbat_thd_t *cur_lv_ptr;
static struct lbat_user *lbat_user_table[USER_SIZE];

static struct lbat_thd_t *lbat_thd_t_init(unsigned int thd, struct lbat_user *user)
{
	struct lbat_thd_t *thd_t;

	thd_t = kzalloc(sizeof(*thd_t), GFP_KERNEL);
	if (thd_t == NULL)
		return NULL;
	thd_t->active = false;
	thd_t->thd = thd;
	thd_t->user = user;
	INIT_LIST_HEAD(&thd_t->list);
	return thd_t;
}

static int cmp(void *priv, struct list_head *a, struct list_head *b)
{
	bool *is_hv_list = priv;
	struct lbat_thd_t *thd_t_a, *thd_t_b;

	thd_t_a = list_entry(a, struct lbat_thd_t, list);
	thd_t_b = list_entry(b, struct lbat_thd_t, list);

	if (*is_hv_list)
		return thd_t_a->thd - thd_t_b->thd;
	return thd_t_b->thd - thd_t_a->thd;
}

static int lbat_user_update(struct lbat_user *new_user)
{
	bool is_hv_list = true;
	struct lbat_thd_t *thd_t = NULL;

	if (user_count < 0)
		return -1;

	lbat_user_table[user_count++] = new_user;
	/* init hv_thd and add to lbat_hv_list */
	thd_t = lbat_thd_t_init(new_user->hv_thd, new_user);
	if (thd_t == NULL)
		return -2;
	list_add(&thd_t->list, &lbat_hv_list);
	list_sort(&is_hv_list, &lbat_hv_list, cmp);

	/* init lv_thd and add to lbat_lv_list */
	thd_t = lbat_thd_t_init(new_user->lv1_thd, new_user);
	if (thd_t == NULL)
		return -3;
	thd_t->active = true;
	list_add(&thd_t->list, &lbat_lv_list);

	if (new_user->lv2_thd) {
		/* init lv2_thd and add to lbat_lv_list */
		thd_t = lbat_thd_t_init(new_user->lv2_thd, new_user);
		if (thd_t == NULL)
			return -4;
		list_add_tail(&thd_t->list, &lbat_lv_list);
	}
	is_hv_list = false;
	list_sort(&is_hv_list, &lbat_lv_list, cmp);

	/* set cur_lv_ptr first entry of lv_list */
	if (cur_lv_ptr == NULL)
		lbat_min_en_setting(1);
	cur_lv_ptr = list_first_entry(&lbat_lv_list, struct lbat_thd_t, list);
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN,
		VOLT_TO_THD(cur_lv_ptr->thd));

	return 0;
}

int lbat_user_register(struct lbat_user *user, const char *name,
	unsigned int hv_thd, unsigned int lv1_thd, unsigned int lv2_thd,
	void (*callback)(unsigned int))
{
	int ret = 0;

	mutex_lock(&lbat_mutex);
	strncpy(user->name, name, strlen(name));
	if (hv_thd >= 5400 || lv1_thd <= 2650) {
		ret = -11;
		goto out;
	} else if (hv_thd < lv1_thd || lv1_thd < lv2_thd) {
		ret = -12;
		goto out;
	} else if (callback == NULL) {
		ret = -13;
		goto out;
	}
	user->hv_thd = hv_thd;
	user->lv1_thd = lv1_thd;
	user->lv2_thd = lv2_thd;
	user->callback = callback;
	pr_info("[%s] hv=%d, lv1=%d, lv2=%d\n", __func__, hv_thd, lv1_thd, lv2_thd);
	ret = lbat_user_update(user);
	if (ret)
		pr_notice("[%s] error ret=%d\n", __func__, ret);
out:
	mutex_unlock(&lbat_mutex);
	return ret;
}
EXPORT_SYMBOL(lbat_user_register);

void lbat_min_en_setting(int en_val)
{
	pmic_set_register_value(PMIC_AUXADC_LBAT_EN_MIN, en_val);
	pmic_set_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MIN, en_val);
	pmic_enable_interrupt(INT_BAT_L, en_val, "lbat_service");
}

void lbat_max_en_setting(int en_val)
{
	pmic_set_register_value(PMIC_AUXADC_LBAT_EN_MAX, en_val);
	pmic_set_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MAX, en_val);
	pmic_enable_interrupt(INT_BAT_H, en_val, "lbat_service");
}

static void bat_h_int_handler(void)
{
	bool is_set;
	struct lbat_user *user;
	struct lbat_thd_t *thd_t = NULL;

	if (cur_hv_ptr == NULL) {
		lbat_min_en_setting(0);
		lbat_max_en_setting(0);
		return;
	}
	mutex_lock(&lbat_mutex);
	pr_info("[%s] cur_thd=%d\n", __func__, cur_hv_ptr->thd);

	user = cur_hv_ptr->user;
	user->callback(cur_hv_ptr->thd);
	cur_hv_ptr->active = false;

	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);

	is_set = false;
	list_for_each_entry(thd_t, &lbat_lv_list, list) {
		if (thd_t->user == user)
			thd_t->active = true;
		if (thd_t->active == true && is_set == false) {
			cur_lv_ptr = thd_t;
			is_set = true;
		}
#if LBAT_SERVICE_DBG
		pr_info("[%s] lv_thd=%d, active=%d\n",
			__func__, thd_t->thd, thd_t->active);
#endif
	}
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN,
		VOLT_TO_THD(cur_lv_ptr->thd));

	if (cur_hv_ptr == list_last_entry(
				&lbat_hv_list, struct lbat_thd_t, list)) {
		cur_hv_ptr = NULL;
		goto out;
	}
	is_set = false;
	list_for_each_entry(thd_t, &lbat_hv_list, list) {
		if (thd_t->thd > cur_hv_ptr->thd && thd_t->user == user)
			thd_t->active = true;
		if (thd_t->active == true && is_set == false) {
			cur_hv_ptr = thd_t;
			is_set = true;
		}
#if LBAT_SERVICE_DBG
		pr_info("[%s] hv_thd=%d, active=%d\n",
			__func__, thd_t->thd, thd_t->active);
#endif
	}
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MAX,
		VOLT_TO_THD(cur_hv_ptr->thd));
	lbat_max_en_setting(1);
out:
	lbat_min_en_setting(1);
	mutex_unlock(&lbat_mutex);
}

static void bat_l_int_handler(void)
{
	bool is_set;
	struct lbat_user *user;
	struct lbat_thd_t *thd_t = NULL;

	if (cur_lv_ptr == NULL) {
		lbat_min_en_setting(0);
		lbat_max_en_setting(0);
		return;
	}
	mutex_lock(&lbat_mutex);
	pr_info("[%s] cur_thd=%d\n", __func__, cur_lv_ptr->thd);

	user = cur_lv_ptr->user;
	user->callback(cur_lv_ptr->thd);
	cur_lv_ptr->active = false;

	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);

	is_set = false;
	list_for_each_entry(thd_t, &lbat_hv_list, list) {
		if (thd_t->user == user)
			thd_t->active = true;
		if (thd_t->active == true && is_set == false) {
			cur_hv_ptr = thd_t;
			is_set = true;
		}
#if LBAT_SERVICE_DBG
		pr_info("[%s] hv_thd=%d, active=%d\n",
			__func__, thd_t->thd, thd_t->active);
#endif
	}
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MAX,
		VOLT_TO_THD(cur_hv_ptr->thd));

	if (cur_lv_ptr == list_last_entry(
				&lbat_lv_list, struct lbat_thd_t, list)) {
		cur_lv_ptr = NULL;
		goto out;
	}
	is_set = false;
	list_for_each_entry(thd_t, &lbat_lv_list, list) {
		if (thd_t->thd < cur_lv_ptr->thd && thd_t->user == user)
			thd_t->active = true;
		if (thd_t->active == true && is_set == false) {
			cur_lv_ptr = thd_t;
			is_set = true;
		}
#if LBAT_SERVICE_DBG
		pr_info("[%s] lv_thd=%d, active=%d\n",
			__func__, thd_t->thd, thd_t->active);
#endif
	}
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN,
		VOLT_TO_THD(cur_lv_ptr->thd));
	lbat_min_en_setting(1);
out:
	lbat_max_en_setting(1);
	mutex_unlock(&lbat_mutex);
}

void lbat_suspend(void)
{
	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
}

void lbat_resume(void)
{
	if (cur_hv_ptr || cur_lv_ptr) {
		lbat_min_en_setting(0);
		lbat_max_en_setting(0);
		mdelay(1);
		if (cur_hv_ptr != NULL)
			lbat_max_en_setting(1);

		if (cur_lv_ptr != NULL)
			lbat_min_en_setting(1);
	}
}

int lbat_service_init(void)
{
	int ret = 0;

	pr_info("[%s]", __func__);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DEBT_MIN, 0);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DEBT_MAX, 0);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DET_PRD_15_0, 15);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DET_PRD_19_16, 0);

	pmic_register_interrupt_callback(INT_BAT_L, bat_l_int_handler);
	pmic_register_interrupt_callback(INT_BAT_H, bat_h_int_handler);

	return ret;
}

/*****************************************************************************
 * Lbat service debug
 ******************************************************************************/
void lbat_dump_reg(void)
{
	pr_notice("AUXADC_LBAT_VOLT_MAX = 0x%x, AUXADC_LBAT_VOLT_MIN = 0x%x, RG_INT_EN_BAT_H = %d, RG_INT_EN_BAT_L = %d\n",
		pmic_get_register_value(PMIC_AUXADC_LBAT_VOLT_MAX),
		pmic_get_register_value(PMIC_AUXADC_LBAT_VOLT_MIN),
		pmic_get_register_value(PMIC_RG_INT_EN_BAT_H),
		pmic_get_register_value(PMIC_RG_INT_EN_BAT_L));
	pr_notice("AUXADC_LBAT_EN_MAX = %d, AUXADC_LBAT_IRQ_EN_MAX = %d, AUXADC_LBAT_EN_MIN = %d, AUXADC_LBAT_IRQ_EN_MIN = %d\n",
		pmic_get_register_value(PMIC_AUXADC_LBAT_EN_MAX),
		pmic_get_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MAX),
		pmic_get_register_value(PMIC_AUXADC_LBAT_EN_MIN),
		pmic_get_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MIN));
}

static void lbat_dump_thd_list(struct seq_file *s)
{
	unsigned int len = 0;
	char str[128] = "";
	struct lbat_thd_t *thd_t = NULL;

	if (list_empty(&lbat_hv_list) || list_empty(&lbat_lv_list)) {
		pr_notice("[%s] no entry in lbat list\n", __func__);
		seq_puts(s, "no entry in lbat list\n");
		return;
	}
	mutex_lock(&lbat_mutex);
	list_for_each_entry(thd_t, &lbat_hv_list, list) {
		len += snprintf(str + len, sizeof(str) - len,
			"%shv_list, thd:%d, active:%d, user:%s\n",
			(thd_t == cur_hv_ptr || thd_t == cur_lv_ptr) ? "->" : "  ",
			thd_t->thd, thd_t->active, thd_t->user->name);
		pr_notice("%s", str);
		seq_printf(s, "%s", str);
		strncpy(str, "", strlen(str));
		len = 0;
	}
	pr_notice("\n");
	seq_puts(s, "\n");

	list_for_each_entry(thd_t, &lbat_lv_list, list) {
		len += snprintf(str + len, sizeof(str) - len,
			"%slv_list, thd:%d, active:%d, user:%s\n",
			(thd_t == cur_hv_ptr || thd_t == cur_lv_ptr) ? "->" : "  ",
			thd_t->thd, thd_t->active, thd_t->user->name);
		pr_notice("%s", str);
		seq_printf(s, "%s", str);
		strncpy(str, "", strlen(str));
		len = 0;
	}
	pr_notice("\n");
	mutex_unlock(&lbat_mutex);
}

static void lbat_dbg_dump_reg(struct seq_file *s)
{
	lbat_dump_reg();
	seq_printf(s, "AUXADC_LBAT_VOLT_MAX = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_LBAT_VOLT_MAX));
	seq_printf(s, "AUXADC_LBAT_VOLT_MIN = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_LBAT_VOLT_MIN));
	seq_printf(s, "RG_INT_EN_BAT_H = 0x%x\n", pmic_get_register_value(PMIC_RG_INT_EN_BAT_H));
	seq_printf(s, "RG_INT_EN_BAT_L = 0x%x\n", pmic_get_register_value(PMIC_RG_INT_EN_BAT_L));
	seq_printf(s, "AUXADC_LBAT_EN_MAX = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_LBAT_EN_MAX));
	seq_printf(s, "AUXADC_LBAT_IRQ_EN_MAX = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MAX));
	seq_printf(s, "AUXADC_LBAT_EN_MIN = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_LBAT_EN_MIN));
	seq_printf(s, "AUXADC_LBAT_IRQ_EN_MIN = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MIN));
	seq_printf(s, "AUXADC_ADC_RDY_LBAT = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_ADC_RDY_LBAT));
	seq_printf(s, "AUXADC_ADC_OUT_LBAT = 0x%x\n", pmic_get_register_value(PMIC_AUXADC_ADC_OUT_LBAT));
}

static void lbat_dump_user_table(struct seq_file *s)
{
	unsigned int i;
	struct lbat_user *user;

	mutex_lock(&lbat_mutex);
	for (i = 0; i < user_count; i++) {
		user = lbat_user_table[i];
		seq_printf(s, "%2d:%20s, %d, %d, %d, %pf\n",
			i, user->name, user->hv_thd,
			user->lv1_thd, user->lv2_thd, user->callback);
	}
	mutex_unlock(&lbat_mutex);
}

enum {
	LBAT_DBG_DUMP_LIST,
	LBAT_DBG_DUMP_REG,
	LBAT_DBG_DUMP_TABLE,
	LBAT_DBG_MAX,
};

static struct lbat_dbg_st dbg_data[LBAT_DBG_MAX];

static int lbat_dbg_show(struct seq_file *s, void *unused)
{
	struct lbat_dbg_st *dbg_st = s->private;

	switch (dbg_st->dbg_id) {
	case LBAT_DBG_DUMP_LIST:
		lbat_dump_thd_list(s);
		break;
	case LBAT_DBG_DUMP_REG:
		lbat_dbg_dump_reg(s);
		break;
	case LBAT_DBG_DUMP_TABLE:
		lbat_dump_user_table(s);
		break;
	default:
		break;
	}
	return 0;
}

static int lbat_dbg_open(struct inode *inode, struct file *file)
{
	if (file->f_mode & FMODE_READ)
		return single_open(file, lbat_dbg_show, inode->i_private);
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t lbat_dbg_write(struct file *file,
	const char __user *user_buffer, size_t count, loff_t *position)
{
	return count;
}

static int lbat_dbg_release(struct inode *inode, struct file *file)
{
	if (file->f_mode & FMODE_READ)
		return single_release(inode, file);
	return 0;
}

static const struct file_operations lbat_dbg_fops = {
	.open = lbat_dbg_open,
	.read = seq_read,
	.write = lbat_dbg_write,
	.llseek  = seq_lseek,
	.release = lbat_dbg_release,
};

int lbat_debug_init(struct dentry *debug_dir)
{
	struct dentry *lbat_dbg_dir;

	if (IS_ERR(debug_dir) || !debug_dir) {
		pr_notice("dir mtk_pmic does not exist\n");
		return -1;
	}
	lbat_dbg_dir = debugfs_create_dir("lbat_dbg", debug_dir);
	if (IS_ERR(lbat_dbg_dir) || !lbat_dbg_dir) {
		pr_notice("fail to mkdir /sys/kernel/debug/mtk_pmic/lbat_dbg\n");
		return -1;
	}
	/* lbat service debug init */
	dbg_data[0].dbg_id = LBAT_DBG_DUMP_LIST;
	debugfs_create_file("lbat_dump_list", (S_IFREG | S_IRUGO),
		lbat_dbg_dir, (void *)&dbg_data[0], &lbat_dbg_fops);

	dbg_data[1].dbg_id = LBAT_DBG_DUMP_REG;
	debugfs_create_file("lbat_dump_reg", (S_IFREG | S_IRUGO),
		lbat_dbg_dir, (void *)&dbg_data[1], &lbat_dbg_fops);

	dbg_data[2].dbg_id = LBAT_DBG_DUMP_TABLE;
	debugfs_create_file("lbat_dump_table", (S_IFREG | S_IRUGO),
		lbat_dbg_dir, (void *)&dbg_data[2], &lbat_dbg_fops);

	return 0;
}

