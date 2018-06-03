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

#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <mt-plat/aee.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/syscalls.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#include <linux/suspend.h>
#include <linux/uaccess.h>

#include <mt-plat/upmu_common.h>
#include <mach/mtk_pmic_wrap.h>
#include "include/pmic_regulator.h"
#include "include/regulator_codegen.h"

/*****************************************************************************
 * Global variable
 ******************************************************************************/
unsigned int g_pmic_pad_vbif28_vol = 1;

unsigned int pmic_read_vbif28_volt(unsigned int *val)
{
	if (g_pmic_pad_vbif28_vol != 0x1) {
		*val = g_pmic_pad_vbif28_vol;
		return 1;
	} else
		return 0;
}

unsigned int pmic_get_vbif28_volt(void)
{
	return g_pmic_pad_vbif28_vol;
}
/*****************************************************************************
 * PMIC6355 linux reguplator driver
 ******************************************************************************/
#ifdef REGULATOR_TEST

unsigned int g_buck_uV;
static ssize_t show_buck_api(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_buck_api] 0x%x\n", g_buck_uV);
	return sprintf(buf, "%u\n", g_buck_uV);
}

static ssize_t store_buck_api(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t size)
{
	/*PMICLOG("[EM] Not Support Write Function\n");*/
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int buck_uV = 0;
	unsigned int buck_type = 0;

	pr_err("[store_buck_api]\n");
	if (buf != NULL && size != 0) {
		pr_err("[store_buck_api] buf is %s\n", buf);
		/*buck_type = simple_strtoul(buf, &pvalue, 16);*/

		pvalue = (char *)buf;
		if (size > 5) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 10, (unsigned int *)&buck_type);
		} else
			ret = kstrtou32(pvalue, 10, (unsigned int *)&buck_type);

		if (buck_type == 9999) {
			pr_err("[store_buck_api] regulator_test!\n");
				pmic_regulator_en_test();
				pmic_regulator_vol_test();
		} else {
			if (size > 5) {
				val =  strsep(&pvalue, " ");
				ret = kstrtou32(val, 10, (unsigned int *)&buck_uV);

				pr_err("[store_buck_api] write buck_type[%d] with voltgae %d !\n",
					buck_type, buck_uV);

				/* only for regulator test*/
				/* ret = buck_set_voltage(buck_type, buck_uV); */
			} else {
				pr_err("[store_buck_api] use \"cat pmic_access\" to get value(decimal)\r\n");
			}
		}
	}
	return size;
}

static DEVICE_ATTR(buck_api, 0664, show_buck_api, store_buck_api);     /*664*/

#endif /*--REGULATOR_TEST--*/

/*****************************************************************************
 * PMIC extern variable
 ******************************************************************************/

struct platform_device mt_pmic_device = {
	.name = "pmic_regulator",
	.id = -1,
};

static const struct platform_device_id pmic_regulator_id[] = {
	{"pmic_regulator", 0},
	{},
};

static const struct of_device_id pmic_cust_of_ids[] = {
	{.compatible = "mediatek,mt6355",},
	{},
};

MODULE_DEVICE_TABLE(of, pmic_cust_of_ids);

static int pmic_regulator_cust_dts_parser(struct platform_device *pdev, struct device_node *regulators)
{
	struct device_node *child;
	int ret = 0;
	unsigned int i = 0, default_on;

	if (!regulators) {
		PMICLOG("[PMIC]pmic_regulator_cust_dts_parser regulators node not found\n");
		return -ENODEV;
	}

	for_each_child_of_node(regulators, child) {
		for (i = 0; i < pmic_regulator_ldo_matches_size; i++) {
			/* compare dt name & ldos name */
			if (!of_node_cmp(child->name, pmic_regulator_ldo_matches[i].name)) {
				PMICLOG("[PMIC]%s regulator_matches %s\n", child->name,
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			}
		}
		if (i == pmic_regulator_ldo_matches_size)
			continue;
		/* check regualtors and set it */
		if (!of_property_read_u32(child, "regulator-default-on", &default_on)) {
			switch (default_on) {
			case 0:
				/* skip */
				PMICLOG("[PMIC]%s regulator_skip %s\n", child->name,
					pmic_regulator_ldo_matches[i].name);
				break;
			case 1:
				/* turn ldo off */
				(mt_ldos[i].en_cb)(0);
				PMICLOG("[PMIC]%s default is off\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			case 2:
				/* turn ldo on */
				(mt_ldos[i].en_cb)(1);
				PMICLOG("[PMIC]%s default is on\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			default:
				break;
			}
		}
	}
	PMICLOG("[PMIC]pmic_regulator_cust_dts_parser done\n");
	return ret;
}

static int pmic_regulator_buck_dts_parser(struct platform_device *pdev, struct device_node *np)
{
	struct device_node *buck_regulators;
	int matched, i = 0, ret = 0;
#ifdef REGULATOR_TEST
	int isEn;
#endif /*--REGULATOR_TEST--*/

	buck_regulators = of_get_child_by_name(np, "buck_regulators");
	if (!buck_regulators) {
		pr_err("[PMIC]regulators node not found\n");
		return -EINVAL;
	}

	matched = of_regulator_match(&pdev->dev, buck_regulators,
				     pmic_regulator_buck_matches,
				     pmic_regulator_buck_matches_size);
	if ((matched < 0) || (matched != mt_bucks_size)) {
		pr_err("[PMIC]Error parsing regulator init data: %d %d\n", matched, mt_bucks_size);
		ret = -matched;
		goto out;
	}

	for (i = 0; i < pmic_regulator_buck_matches_size; i++) {
		if (mt_bucks[i].isUsedable == 1) {
			mt_bucks[i].config.dev = &(pdev->dev);
			mt_bucks[i].config.init_data = pmic_regulator_buck_matches[i].init_data;
			mt_bucks[i].config.of_node = pmic_regulator_buck_matches[i].of_node;
			mt_bucks[i].config.driver_data = pmic_regulator_buck_matches[i].driver_data;
			mt_bucks[i].desc.owner = THIS_MODULE;

			mt_bucks[i].rdev =
			    regulator_register(&mt_bucks[i].desc, &mt_bucks[i].config);

			if (IS_ERR(mt_bucks[i].rdev)) {
				ret = PTR_ERR(mt_bucks[i].rdev);
				pr_warn("[regulator_register] failed to register %s (%d)\n",
								mt_bucks[i].desc.name, ret);
				continue;
			} else
				PMICLOG("[regulator_register] pass to register %s\n", mt_bucks[i].desc.name);

#ifdef REGULATOR_TEST
			mt_bucks[i].reg = regulator_get(&(pdev->dev), mt_bucks[i].desc.name);
			isEn = regulator_is_enabled(mt_bucks[i].reg);
			if (isEn != 0)
				PMICLOG("[regulator] %s is default on\n", mt_bucks[i].desc.name);
#endif /*--REGULATOR_TEST--*/

			PMICLOG("[PMIC]mt_bucks[%d].config.init_data min_uv:%d max_uv:%d\n",
				i, mt_bucks[i].config.init_data->constraints.min_uV,
				mt_bucks[i].config.init_data->constraints.max_uV);
		}
	}

out:
	of_node_put(buck_regulators);
	return ret;
}

static int pmic_regulator_ldo_dts_parser(struct platform_device *pdev, struct device_node *np)
{
	struct device_node *ldo_regulators;
	int matched, i = 0, ret;
#ifdef REGULATOR_TEST
	int isEn;
#endif /*--REGULATOR_TEST--*/

	ldo_regulators = of_get_child_by_name(np, "ldo_regulators");
	if (!ldo_regulators) {
		pr_err("[PMIC]regulators node not found\n");
		return -EINVAL;
	}

	matched = of_regulator_match(&pdev->dev, ldo_regulators,
				     pmic_regulator_ldo_matches,
				     pmic_regulator_ldo_matches_size);
	if ((matched < 0) || (matched != mt_ldos_size)) {
		pr_err("[PMIC]Error parsing regulator init data: %d %d\n", matched,
			mt_ldos_size);
		ret = -matched;
		goto out;
	}

	for (i = 0; i < pmic_regulator_ldo_matches_size; i++) {
		if (mt_ldos[i].isUsedable == 1) {
			mt_ldos[i].config.dev = &(pdev->dev);
			mt_ldos[i].config.init_data = pmic_regulator_ldo_matches[i].init_data;
			mt_ldos[i].config.of_node = pmic_regulator_ldo_matches[i].of_node;
			mt_ldos[i].config.driver_data = pmic_regulator_ldo_matches[i].driver_data;
			mt_ldos[i].desc.owner = THIS_MODULE;

			mt_ldos[i].rdev =
			    regulator_register(&mt_ldos[i].desc, &mt_ldos[i].config);

			if (IS_ERR(mt_ldos[i].rdev)) {
				ret = PTR_ERR(mt_ldos[i].rdev);
				pr_warn("[regulator_register] failed to register %s (%d)\n",
					mt_ldos[i].desc.name, ret);
				continue;
			} else {
				PMICLOG("[regulator_register] pass to register %s\n",
					mt_ldos[i].desc.name);
			}

#ifdef REGULATOR_TEST
			mt_ldos[i].reg = regulator_get(&(pdev->dev), mt_ldos[i].desc.name);
			isEn = regulator_is_enabled(mt_ldos[i].reg);
			if (isEn != 0)
				PMICLOG("[regulator] %s is default on\n", mt_ldos[i].desc.name);
#endif /*--REGULATOR_TEST--*/
			/* To initialize varriables which were used to record status, */
			/* if ldo regulator have been modified by user.               */
			/* mt_ldos[i].vosel.ldo_user = mt_ldos[i].rdev->use_count;  */
			if (mt_ldos[i].da_vol_cb != NULL)
				mt_ldos[i].vosel.def_sel = (mt_ldos[i].da_vol_cb)();

			mt_ldos[i].vosel.cur_sel = mt_ldos[i].vosel.def_sel;

			PMICLOG("[PMIC]mt_ldos[%d].config.init_data min_uv:%d max_uv:%d\n",
				i, mt_ldos[i].config.init_data->constraints.min_uV,
				mt_ldos[i].config.init_data->constraints.max_uV);
		}
	}
	/*--for ldo customization--*/
	ret = pmic_regulator_cust_dts_parser(pdev, ldo_regulators);
	if (ret) {
		pr_err("pmic_regulator_cust_dts_parser fail\n");
		ret = -EINVAL;
		goto out;
	}

out:
	of_node_put(ldo_regulators);
	return ret;
}



static int pmic_regulator_init(struct platform_device *pdev)
{
	struct device_node *np;
	int ret = 0;

	pdev->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,mt_pmic");
	np = of_node_get(pdev->dev.of_node);
	if (!np) {
		pr_err("pmic_regulator_init np = NULL\n");
		return -EINVAL;
	}

	ret = pmic_regulator_buck_dts_parser(pdev, np);
	if (ret) {
		pr_err("pmic_regulator_buck_dts_parser fail\n");
		ret = -EINVAL;
		goto out;
	}

	ret = pmic_regulator_ldo_dts_parser(pdev, np);
	if (ret) {
		pr_err("pmic_regulator_ldo_dts_parser fail\n");
		ret = -EINVAL;
		goto out;
	}

out:
	of_node_put(np);
	return ret;
}

void pmic_regulator_suspend(void)
{
	int i;

	for (i = 0; i < mt_ldos_size; i++) {
		if (mt_ldos[i].isUsedable == 1) {
			if (mt_ldos[i].da_vol_cb != NULL) {
				mt_ldos[i].vosel.cur_sel = (mt_ldos[i].da_vol_cb)();

				if (mt_ldos[i].vosel.cur_sel != mt_ldos[i].vosel.def_sel) {
					mt_ldos[i].vosel.restore = true;
					pr_err("pmic_regulator_suspend(name=%s id=%d default_sel=%d current_sel=%d)\n",
						mt_ldos[i].rdev->desc->name, mt_ldos[i].rdev->desc->id,
						mt_ldos[i].vosel.def_sel, mt_ldos[i].vosel.cur_sel);
				} else
					mt_ldos[i].vosel.restore = false;
			}
		}
	}

}

void pmic_regulator_resume(void)
{
	int i, selector;

	for (i = 0; i < mt_ldos_size; i++) {
		if (mt_ldos[i].isUsedable == 1) {
			if (mt_ldos[i].vol_cb != NULL) {
				if (mt_ldos[i].vosel.restore == true) {
					/*-- regulator voltage changed? --*/
					selector = mt_ldos[i].vosel.cur_sel;
					(mt_ldos[i].vol_cb)(selector);

					pr_err("pmic_regulator_resume(name=%s id=%d default_sel=%d current_sel=%d)\n",
						mt_ldos[i].rdev->desc->name, mt_ldos[i].rdev->desc->id,
						mt_ldos[i].vosel.def_sel, mt_ldos[i].vosel.cur_sel);
				}
			}
		}
	}
}

static int pmic_regulator_pm_event(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:	/* Going to hibernate */
		pr_warn("[%s] pm_event %lu (IPOH)\n", __func__, pm_event);
		return NOTIFY_DONE;

	case PM_POST_HIBERNATION:	/* Hibernation finished */
		pr_warn("[%s] pm_event %lu\n", __func__, pm_event);
		pmic_regulator_resume();
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static struct notifier_block pmic_regulator_pm_notifier_block = {
	.notifier_call = pmic_regulator_pm_event,
	.priority = 0,
};

int mtk_regulator_init(struct platform_device *dev)
{
	int ret = 0;

#ifdef CONFIG_OF
	pmic_regulator_init(dev);
#endif
	ret = register_pm_notifier(&pmic_regulator_pm_notifier_block);
	if (ret) {
		PMICLOG("****failed to register PM notifier %d\n", ret);
		return ret;
	}

	return ret;
}


#ifdef REGULATOR_TEST
void pmic_regulator_en_test(void)
{
	int i = 0;
	int ret1, ret2;
	struct regulator *reg;

	/*for (i = 0; i < ARRAY_SIZE(mt_ldos); i++) {*/
	for (i = 0; i < mt_ldos_size; i++) {
		/*---VIO18 should not be off---*/
		if (i != 1) {
			if (mt_ldos[i].isUsedable == 1) {
				reg = mt_ldos[i].reg;
				PMICLOG("[regulator enable test] %s\n", mt_ldos[i].desc.name);

				ret1 = regulator_enable(reg);
				ret2 = regulator_is_enabled(reg);

				if (ret2 == (mt_ldos[i].da_en_cb()))
					PMICLOG("[enable test pass]\n");
				else
					PMICLOG("[enable test fail]\n");

				ret1 = regulator_disable(reg);
				ret2 = regulator_is_enabled(reg);

				if (ret2 == (mt_ldos[i].da_en_cb()))
					PMICLOG("[disable test pass]\n");
				else
					PMICLOG("[disable test fail]\n");
			}
		} /*---VIO18 should not be off---*/
	}
}

void pmic_regulator_vol_test(void)
{
	int i = 0, j, sj = 0;
	/*int ret1, ret2;*/
	struct regulator *reg;

	for (i = 0; i < mt_ldos_size; i++) {
		const int *pVoltage;

		reg = mt_ldos[i].reg;
		/*---VIO18 should not be off---*/
		if (i != 1 && (mt_ldos[i].isUsedable == 1)) {
			PMICLOG("[regulator voltage test] %s voltage:%d\n",
				mt_ldos[i].desc.name, mt_ldos[i].desc.n_voltages);

			if (mt_ldos[i].pvoltages != NULL) {
				pVoltage = (const int *)mt_ldos[i].pvoltages;

				if (i == 8)
					sj = 8;
				else
					sj = 0;

				for (j = sj; j < mt_ldos[i].desc.n_voltages; j++) {
					int rvoltage;

					regulator_set_voltage(reg, pVoltage[j], pVoltage[j]);
					rvoltage = regulator_get_voltage(reg);

					if (j == (mt_ldos[i].da_vol_cb())
						   && (pVoltage[j] == rvoltage)) {
						PMICLOG("[%d:%d]:pass  set_voltage:%d  rvoltage:%d\n",
							j, j, pVoltage[j], rvoltage);
					} else {
						PMICLOG("[%d:%d]:fail  set_voltage:%d  rvoltage:%d\n",
							j, (mt_ldos[i].da_vol_cb()), pVoltage[j], rvoltage);
					}
				}
			}
		}
	} /*---VIO18 should not be off---*/
}
#endif /*--REGULATOR_TEST--*/

/*****************************************************************************
 * Dump all LDO status
 ******************************************************************************/
void dump_ldo_status_read_debug(void)
{
	int i;
	int en = 0;
	int voltage_reg = 0;
	int voltage = 0;
	const int *pVoltage;

	pr_debug("********** BUCK/LDO status dump [1:ON,0:OFF]**********\n");

	/*for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {*/
	for (i = 0; i < mt_bucks_size; i++) {
		if (mt_bucks[i].da_en_cb != NULL)
			en = (mt_bucks[i].da_en_cb)();
		else
			en = -1;

		if (mt_bucks[i].da_vol_cb != NULL) {
			voltage_reg = (mt_bucks[i].da_vol_cb)();
			voltage =
			    mt_bucks[i].desc.min_uV + mt_bucks[i].desc.uV_step * voltage_reg;
		} else {
			voltage_reg = -1;
			voltage = -1;
		}
		pr_err("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			mt_bucks[i].desc.name, en, voltage, voltage_reg);
	}

	/*for (i = 0; i < ARRAY_SIZE(mt_ldos); i++) {*/
	for (i = 0; i < mt_ldos_size; i++) {
		if (mt_ldos[i].da_en_cb != NULL)
			en = (mt_ldos[i].da_en_cb)();
		else
			en = -1;

		if (mt_ldos[i].desc.n_voltages != 1) {
			if (mt_ldos[i].da_vol_cb != NULL) {
				voltage_reg = (mt_ldos[i].da_vol_cb)();
				if (mt_ldos[i].pvoltages != NULL) {
					pVoltage = (const int *)mt_ldos[i].pvoltages;
					/*HW LDO sequence issue, we need to change it */
					voltage = pVoltage[voltage_reg];
				} else {
					voltage =
					    mt_ldos[i].desc.min_uV +
					    mt_ldos[i].desc.uV_step * voltage_reg;
				}
			} else {
				voltage_reg = -1;
				voltage = -1;
			}
		} else {
			pVoltage = (const int *)mt_ldos[i].pvoltages;
			voltage = pVoltage[0];
		}

		pr_err("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			mt_ldos[i].desc.name, en, voltage, voltage_reg);
	}
}

static int proc_utilization_show(struct seq_file *m, void *v)
{
	int i;
	int en = 0;
	int voltage_reg = 0;
	int voltage = 0;
	const int *pVoltage;

	seq_puts(m, "********** BUCK/LDO status dump [1:ON,0:OFF]**********\n");

	/*for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {*/
	for (i = 0; i < mt_bucks_size; i++) {
		if (mt_bucks[i].da_en_cb != NULL)
			en = (mt_bucks[i].da_en_cb)();
		else
			en = -1;

		if (mt_bucks[i].da_vol_cb != NULL) {
			voltage_reg = (mt_bucks[i].da_vol_cb)();
			voltage =
			    mt_bucks[i].desc.min_uV + mt_bucks[i].desc.uV_step * voltage_reg;
		} else {
			voltage_reg = -1;
			voltage = -1;
		}
		seq_printf(m, "%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			   mt_bucks[i].desc.name, en, voltage, voltage_reg);
	}

	/*for (i = 0; i < ARRAY_SIZE(mt_ldos); i++) {*/
	for (i = 0; i < mt_ldos_size; i++) {
		if (mt_ldos[i].da_en_cb != NULL)
			en = (mt_ldos[i].da_en_cb)();
		else
			en = -1;

		if (mt_ldos[i].desc.n_voltages != 1) {
			if (mt_ldos[i].da_vol_cb != NULL) {
				voltage_reg = (mt_ldos[i].da_vol_cb)();
				if (mt_ldos[i].pvoltages != NULL) {
					pVoltage = (const int *)mt_ldos[i].pvoltages;
					/*HW LDO sequence issue, we need to change it */
					voltage = pVoltage[voltage_reg];
				} else {
					voltage =
					    mt_ldos[i].desc.min_uV +
					    mt_ldos[i].desc.uV_step * voltage_reg;
				}
			} else {
				voltage_reg = -1;
				voltage = -1;
			}
		} else {
			pVoltage = (const int *)mt_ldos[i].pvoltages;
			voltage = pVoltage[0];
		}
		seq_printf(m, "%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			   mt_ldos[i].desc.name, en, voltage, voltage_reg);
	}
	return 0;
}

static int proc_utilization_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_utilization_show, NULL);
}

static const struct file_operations pmic_debug_proc_fops = {
	.open = proc_utilization_open,
	.read = seq_read,
};

void pmic_regulator_debug_init(struct platform_device *dev, struct dentry *debug_dir)
{
	/* /sys/class/regulator/.../ */
	int ret_device_file = 0, i;
	struct dentry *mt_pmic_dir = debug_dir;

#ifdef REGULATOR_TEST
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_buck_api);
#endif /*--REGULATOR_TEST--*/

	/* /sys/class/regulator/.../ */
	/*EM BUCK voltage & Status*/
	for (i = 0; i < mt_bucks_size; i++) {
		/*PMICLOG("[PMIC] register buck id=%d\n",i);*/
		pr_err("[PMIC] register buck id=%d\n", i);
		if (*(int *)(&mt_bucks[i].en_att)) {
			PMICLOG("[PMIC] register ldo en_att\n");
			ret_device_file = device_create_file(&(dev->dev), &(mt_bucks[i].en_att));
		}

		if (*(int *)(&mt_bucks[i].voltage_att)) {
			PMICLOG("[PMIC] register ldo voltage_att\n");
			ret_device_file = device_create_file(&(dev->dev), &(mt_bucks[i].voltage_att));
		}
	}
	/*ret_device_file = device_create_file(&(dev->dev), &mtk_bucks_class[i].voltage_att);*/
	/*EM ldo voltage & Status*/
	for (i = 0; i < mt_ldos_size; i++) {
		/*PMICLOG("[PMIC] register ldo id=%d\n",i);*/
		if (*(int *)(&mt_ldos[i].en_att)) {
			PMICLOG("[PMIC] register ldo en_att\n");
			ret_device_file = device_create_file(&(dev->dev), &mt_ldos[i].en_att);
		}

		if (*(int *)(&mt_ldos[i].voltage_att)) {
			PMICLOG("[PMIC] register ldo voltage_att\n");
			ret_device_file = device_create_file(&(dev->dev), &mt_ldos[i].voltage_att);
		}
	}

	debugfs_create_file("dump_ldo_status", S_IRUGO | S_IWUSR, mt_pmic_dir, NULL, &pmic_debug_proc_fops);
	PMICLOG("proc_create pmic_debug_proc_fops\n");

}

MODULE_AUTHOR("Jimmy-YJ Huang");
MODULE_DESCRIPTION("MT PMIC REGULATOR Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0_M");
