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
/*#include <asm/uaccess.h>*/
#include <linux/uaccess.h>

#include <mt-plat/upmu_common.h>
#include <mach/mt_pmic_wrap.h>
#include "include/pmic_regulator.h"

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
 * PMIC6335 linux reguplator driver
 ******************************************************************************/

int mtk_regulator_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned int add = 0, val = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	if (mreg->en_reg != 0) {
		pmic_set_register_value(mreg->en_reg, 1);
		add = pmu_flags_table[mreg->en_reg].offset;
		val = upmu_get_reg_value(add);
	}

	PMICLOG("regulator_enable(name=%s id=%d en_reg=%x vol_reg=%x) [%x]=0x%x\n", rdesc->name,
		rdesc->id, mreg->en_reg, mreg->vol_reg, add, val);

	return 0;
}

int mtk_regulator_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned int add = 0, val = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	if (rdev->use_count == 0) {
		PMICLOG("regulator name=%s should not disable( use_count=%d)\n", rdesc->name,
			rdev->use_count);
		return -1;
	}

	if (mreg->en_reg != 0) {
		pmic_set_register_value(mreg->en_reg, 0);
		add = pmu_flags_table[mreg->en_reg].offset;
		val = upmu_get_reg_value(add);
	}

	PMICLOG("regulator_disable(name=%s id=%d en_reg=%x vol_reg=%x use_count=%d) [%x]=0x%x\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, rdev->use_count, add, val);

	return 0;
}

int mtk_regulator_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int en;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	en = pmic_get_register_value(mreg->en_reg);

	PMICLOG("[PMIC]regulator_is_enabled(name=%s id=%d en_reg=%x vol_reg=%x en=%d)\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, en);

	return en;
}

int mtk_regulator_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	const int *pVoltage;
	int voltage = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

		if (mreg->desc.n_voltages != 1) {
			if (mreg->vol_reg != 0) {
				if (mreg->pvoltages != NULL) {
					pVoltage = (const int *)mreg->pvoltages;
					voltage = pVoltage[selector];
				} else {
					voltage = mreg->desc.min_uV + mreg->desc.uV_step * selector;
				}
			} else {
				PMICLOG
				("mtk_regulator_list_voltage bugl(name=%s id=%d en_reg=%x vol_reg=%x)\n",
				rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg);
			}
		} else {
			pVoltage = (const int *)mreg->pvoltages;
			voltage = pVoltage[0];
		}

	return voltage;
}

int mtk_regulator_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned char regVal = 0;
	const int *pVoltage;
	int voltage = 0;
	unsigned int add = 0, val = 9;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	/*mreg = container_of(rdev, struct mtk_regulator, rdev);*/

	if (mreg->desc.n_voltages != 1) {
		if (mreg->vol_reg != 0) {
			regVal = pmic_get_register_value(mreg->vol_reg);
			if (mreg->pvoltages != NULL) {
				pVoltage = (const int *)mreg->pvoltages;
				voltage = pVoltage[regVal];
				add = pmu_flags_table[mreg->en_reg].offset;
				val = upmu_get_reg_value(add);
			} else {
				voltage = mreg->desc.min_uV + mreg->desc.uV_step * regVal;
			}
		} else {
			PMICLOG
			    ("regulator_get_voltage_sel bugl(name=%s id=%d en_reg=%x vol_reg=%x)\n",
			     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg);
		}
	} else {
		if (mreg->vol_reg != 0) {
			regVal = 0;
			pVoltage = (const int *)mreg->pvoltages;
			voltage = pVoltage[regVal];
		} else {
			PMICLOG
			    ("regulator_get_voltage_sel bugl(name=%s id=%d en_reg=%x vol_reg=%x)\n",
			     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg);
		}
	}
	PMICLOG
	    ("regulator_get_voltage_sel(name=%s id=%d en_reg=%x vol_reg=%x reg/sel:%d voltage:%d [0x%x]=0x%x)\n",
	     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, regVal, voltage, add, val);

	return regVal;
}

int mtk_regulator_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("regulator_set_voltage_sel(name=%s id=%d en_reg=%x vol_reg=%x selector=%d)\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, selector);

	/* record status that ldo regulator have been modified */
	mreg->vosel.cur_sel = selector;

	/*pr_err("regulator_set_voltage_sel(name=%s selector=%d)\n",
		rdesc->name, selector);*/
	if (mreg->vol_reg != 0)
		pmic_set_register_value(mreg->vol_reg, selector);

	return 0;
}

#if 0
static struct regulator_ops mtk_regulator_ops = {
	.enable = mtk_regulator_enable,
	.disable = mtk_regulator_disable,
	.is_enabled = mtk_regulator_is_enabled,
	.get_voltage_sel = mtk_regulator_get_voltage_sel,
	.set_voltage_sel = mtk_regulator_set_voltage_sel,
	.list_voltage = mtk_regulator_list_voltage,
};
#endif

static ssize_t show_BUCK_STATUS(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_regulator *mreg;
	unsigned int ret_value = 0;

	mreg = container_of(attr, struct mtk_regulator, en_att);

	ret_value = pmic_get_register_value(mreg->qi_en_reg);

	pr_debug("[EM] BUCK_%s_STATUS : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_BUCK_STATUS(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");

	return size;
}

unsigned int g_buck_uV = 0;
static ssize_t show_buck_voltage(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_buck_voltage] 0x%x\n", g_buck_uV);
	return sprintf(buf, "%u\n", g_buck_uV);
}

static ssize_t store_buck_voltage(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t size)
{
	/*PMICLOG("[EM] Not Support Write Function\n");*/
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int buck_uV = 0;
	unsigned int buck_type = 0;

	pr_err("[store_buck_voltage]\n");
	if (buf != NULL && size != 0) {
		pr_err("[store_buck_voltage] buf is %s\n", buf);
		/*buck_type = simple_strtoul(buf, &pvalue, 16);*/

		pvalue = (char *)buf;
		if (size > 5) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 10, (unsigned int *)&buck_type);
		} else
			ret = kstrtou32(pvalue, 10, (unsigned int *)&buck_type);

#ifdef LDO_REGULATOR_TEST
		if (buck_type == 9999) {
			pr_err("[store_buck_voltage] regulator_test!\n");
				PMIC6335_regulator_test();
		} else {
#endif /*--LDO_REGULATOR_TEST--*/
			if (size > 5) {
				/*buck_uV = simple_strtoul((pvalue + 1), NULL, 16);*/
				/*pvalue = (char *)buf + 1;*/
				val =  strsep(&pvalue, " ");
				ret = kstrtou32(val, 10, (unsigned int *)&buck_uV);

				pr_err("[store_buck_voltage] write buck_type[%d] with voltgae %d !\n",
					buck_type, buck_uV);
#ifdef BUCK_REGULATOR_TEST
				/*ret = buck_set_voltage(buck_type, buck_uV);*/
				ret = buck_set_voltage(buck_type, buck_uV);
#endif /*--BUCK_REGULATOR_TEST--*/
			} else {
				g_buck_uV = buck_get_voltage(buck_type);

				pr_err("[store_buck_voltage] read buck_type[%d] with voltgae %d !\n",
					buck_type, g_buck_uV);
				pr_err("[store_buck_voltage] use \"cat pmic_access\" to get value(decimal)\r\n");
			}
#ifdef LDO_REGULATOR_TEST
		}
#endif /*--LDO_REGULATOR_TEST--*/
	}
	return size;
}

static DEVICE_ATTR(pmic_buck_voltage, 0664, show_buck_voltage, store_buck_voltage);     /*664*/

/*****************************************************************************
 * PMIC extern variable
 ******************************************************************************/
struct mtk_bucks_t mtk_bucks_class[] = {
	PMIC_BUCK_GEN1(VCORE, PMIC_RG_BUCK_VCORE_EN, PMIC_RG_VCORE_MODESET,
			PMIC_RG_BUCK_VCORE_VOSEL, PMIC_DA_QI_VCORE_EN, PMIC_DA_NI_VCORE_VOSEL,
			400000, 1200000, 6250, 30),
	PMIC_BUCK_GEN1(VDRAM, PMIC_RG_BUCK_VDRAM_EN, PMIC_RG_VDRAM_MODESET,
			PMIC_RG_BUCK_VDRAM_VOSEL, PMIC_DA_QI_VDRAM_EN, PMIC_DA_NI_VDRAM_VOSEL,
			600000, 1170000, 6250, 30),
	PMIC_BUCK_GEN1(VMODEM, PMIC_RG_BUCK_VMODEM_EN, PMIC_RG_VMODEM_MODESET,
			PMIC_RG_BUCK_VMODEM_VOSEL, PMIC_DA_QI_VMODEM_EN, PMIC_DA_NI_VMODEM_VOSEL,
			400000, 1200000, 6250, 30),
	PMIC_BUCK_GEN1(VMD1, PMIC_RG_BUCK_VMD1_EN, PMIC_RG_VMD1_MODESET,
			PMIC_RG_BUCK_VMD1_VOSEL, PMIC_DA_QI_VMD1_EN, PMIC_DA_NI_VMD1_VOSEL,
			400000, 1200000, 6250, 30),
	PMIC_BUCK_GEN1(VS1, PMIC_RG_BUCK_VS1_EN, PMIC_RG_VS1_MODESET,
			PMIC_RG_BUCK_VS1_VOSEL, PMIC_DA_QI_VS1_EN, PMIC_DA_NI_VS1_VOSEL,
			1200000, 2050000, 6250, 30),
	PMIC_BUCK_GEN1(VS2, PMIC_RG_BUCK_VS2_EN, PMIC_RG_VS2_MODESET,
			PMIC_RG_BUCK_VS2_VOSEL, PMIC_DA_QI_VS2_EN, PMIC_DA_NI_VS2_VOSEL,
			1200000, 1600000, 6250, 30),
	PMIC_BUCK_GEN1(VPA1, PMIC_RG_BUCK_VPA1_EN, PMIC_RG_VPA1_MODESET,
			PMIC_RG_BUCK_VPA1_VOSEL, PMIC_DA_QI_VPA1_EN, PMIC_DA_NI_VPA1_VOSEL,
			500000, 1393750, 50000, 100),
	PMIC_BUCK_GEN1(VPA2, PMIC_RG_BUCK_VPA2_EN, PMIC_RG_VPA2_MODESET,
			PMIC_RG_BUCK_VPA2_VOSEL, PMIC_DA_QI_VPA2_EN, PMIC_DA_NI_VPA2_VOSEL,
			500000, 1393750, 50000, 100),
	PMIC_BUCK_GEN1(VSRAM_DVFS1, PMIC_RG_VSRAM_DVFS1_SW_EN, PMU_COMMAND_MAX,
			PMIC_RG_VSRAM_DVFS1_VOSEL, PMIC_DA_QI_VSRAM_DVFS1_EN, PMIC_DA_NI_VSRAM_DVFS1_VOSEL,
			400000, 1200000, 6250, 30),
	PMIC_BUCK_GEN1(VSRAM_DVFS2, PMIC_RG_VSRAM_DVFS2_SW_EN, PMU_COMMAND_MAX,
			PMIC_RG_VSRAM_DVFS2_VOSEL, PMIC_DA_QI_VSRAM_DVFS2_EN, PMIC_DA_NI_VSRAM_DVFS2_VOSEL,
			400000, 1200000, 6250, 30),
	PMIC_BUCK_GEN1(VSRAM_VGPU, PMIC_RG_VSRAM_VGPU_SW_EN, PMU_COMMAND_MAX,
			PMIC_RG_VSRAM_VGPU_VOSEL, PMIC_DA_QI_VSRAM_VGPU_EN, PMIC_DA_NI_VSRAM_VGPU_VOSEL,
			400000, 1200000, 6250, 30),
	PMIC_BUCK_GEN1(VSRAM_VCORE, PMIC_RG_VSRAM_VCORE_SW_EN, PMU_COMMAND_MAX,
			PMIC_RG_VSRAM_VCORE_VOSEL, PMIC_DA_QI_VSRAM_VCORE_EN, PMIC_DA_NI_VSRAM_VCORE_VOSEL,
			400000, 1200000, 6250, 30),
	PMIC_BUCK_GEN1(VSRAM_VMD, PMIC_RG_VSRAM_VMD_SW_EN, PMU_COMMAND_MAX,
			PMIC_RG_VSRAM_VMD_VOSEL, PMIC_DA_QI_VSRAM_VMD_EN, PMIC_DA_NI_VSRAM_VMD_VOSEL,
			400000, 1200000, 6250, 30),
};

static unsigned int mtk_bucks_size = ARRAY_SIZE(mtk_bucks_class);

/*en = 1 enable*/
/*en = 0 disable*/
int buck_enable(BUCK_TYPE type, unsigned char en)
{
	if (type > mtk_bucks_size) {
		pr_debug("[PMIC]Wrong buck type for setting on-off\n");
		return -1;
	}

	if (type == VS1 || type == VS2) {
		if (!en) {
			pr_debug("[PMIC] VS1/VS2 can't off\n");
			return -1;
		}
	}

	pmic_set_register_value(mtk_bucks_class[type].en, en);

	/*---Make sure BUCK <NAME> ON before setting---*/
	if (pmic_get_register_value(mtk_bucks_class[type].da_qi_en) == en)
		pr_debug("[PMIC] Set %s Votage on-off:%d success\n", mtk_bucks_class[type].name, en);
	else
		pr_debug("[PMIC] Set %s Votage on-off:%d fail\n", mtk_bucks_class[type].name, en);

	return pmic_get_register_value(mtk_bucks_class[type].da_qi_en);
}

/*pmode = 1 force PWM mode*/
/*pmode = 0 auto mode*/
int buck_set_mode(BUCK_TYPE type, unsigned char pmode)
{
	if (type > mtk_bucks_size) {
		pr_err("[PMIC]Wrong buck type for setting mode\n");
		return -1;
	}

	if (mtk_bucks_class[type].mode == PMU_COMMAND_MAX) {
		pr_err("[PMIC]Can't set mode for %s\n",
			mtk_bucks_class[type].name);
		return -1;
	}

	/*---Make sure BUCK <NAME> ON before setting---*/
	pmic_set_register_value(mtk_bucks_class[type].mode, pmode);
	udelay(220);

	if (pmic_get_register_value(mtk_bucks_class[type].mode) == pmode)
		pr_debug("[PMIC] Set %s Mode to %d pass\n", mtk_bucks_class[type].name, pmode);
	else
		pr_debug("[PMIC] Set %s Mode to %d fail\n", mtk_bucks_class[type].name, pmode);

	return pmic_get_register_value(mtk_bucks_class[type].mode);
}


int buck_set_voltage(BUCK_TYPE type, unsigned int voltage)
{
	unsigned short value = 0;

	if (type > mtk_bucks_size) {
		pr_err("[PMIC]Wrong buck type for setting voltage\n");
		return -1;
	}
	if (voltage > mtk_bucks_class[type].max_uV || voltage < mtk_bucks_class[type].min_uV) {
		pr_err("[PMIC]Set Wrong buck voltage for %s, range (%duV - %duV)\n",
			mtk_bucks_class[type].name, mtk_bucks_class[type].min_uV, mtk_bucks_class[type].max_uV);
		return -1;
	}

	if (type == VS1 || type == VS2) {
		voltage >>= 1;
		value = (voltage - mtk_bucks_class[type].min_uV)/(mtk_bucks_class[type].uV_step);
	} else
		value = (voltage - mtk_bucks_class[type].min_uV)/(mtk_bucks_class[type].uV_step);

	pr_debug("[PMIC]%s Expected volt step: 0x%x\n", mtk_bucks_class[type].name, value);

		/*---Make sure BUCK <NAME> ON before setting---*/
		if (pmic_get_register_value(mtk_bucks_class[type].da_qi_en)) {
			pmic_set_register_value(mtk_bucks_class[type].vosel, value);
			udelay(220);
			if (pmic_get_register_value(mtk_bucks_class[type].da_ni_vosel) == value)
				pr_debug("[PMIC] Set %s Voltage to %d pass\n",
						mtk_bucks_class[type].name, value);
			else
				pr_debug("[PMIC] Set %s Voltage to %d fail\n",
						mtk_bucks_class[type].name, value);
		} else
			pr_debug("[PMIC] Set %s Votage to %d fail, due to buck non-enable\n",
				mtk_bucks_class[type].name, value);

	return pmic_get_register_value(mtk_bucks_class[type].da_ni_vosel);
}


unsigned int buck_get_voltage(BUCK_TYPE type)
{
	unsigned short value = 0;
	unsigned int voltage = 0;

	if (type > mtk_bucks_size) {
		pr_err("[PMIC]Wrong buck type for getting voltage\n");
		return 1;
	}

	value = pmic_get_register_value(mtk_bucks_class[type].da_ni_vosel);

	if (type == VS1 || type == VS2) {
		voltage = ((value * (mtk_bucks_class[type].uV_step)) + mtk_bucks_class[type].min_uV);
		voltage <<= 1;
	} else
		voltage = ((value * (mtk_bucks_class[type].uV_step)) + mtk_bucks_class[type].min_uV);

	if (voltage > mtk_bucks_class[type].max_uV || voltage < mtk_bucks_class[type].min_uV) {
		pr_err("[PMIC]Get Wrong buck voltage for %s, range (%duV - %duV)\n",
			mtk_bucks_class[type].name, mtk_bucks_class[type].min_uV, mtk_bucks_class[type].max_uV);
		return 1;
	}

	return voltage;
}

/*#if !defined CONFIG_MTK_LEGACY*//*Jimmy*/
#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
struct platform_device mt_pmic_device = {
	.name = "pmic_regulator",
	.id = -1,
};

static const struct platform_device_id pmic_regulator_id[] = {
	{"pmic_regulator", 0},
	{},
};

static const struct of_device_id pmic_cust_of_ids[] = {
	{.compatible = "mediatek,mt6335",},
	{},
};

MODULE_DEVICE_TABLE(of, pmic_cust_of_ids);

static int pmic_regulator_init(struct platform_device *pdev)
{
	struct device_node *np, *regulators;
	int matched, i = 0, ret;
#ifdef LDO_REGULATOR_TEST
	int isEn = 0;
#endif /*--LDO_REGULATOR_TEST--*/

	pdev->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,mt_pmic");
	np = of_node_get(pdev->dev.of_node);
	if (!np)
		return -EINVAL;

	regulators = of_get_child_by_name(np, "ldo_regulators");
	if (!regulators) {
		PMICLOG("[PMIC]regulators node not found\n");
		ret = -EINVAL;
		goto out;
	}
	/*matched = of_regulator_match(&pdev->dev, regulators,
				     pmic_regulator_matches,
				     ARRAY_SIZE(pmic_regulator_matches));*/
	matched = of_regulator_match(&pdev->dev, regulators,
				     pmic_regulator_matches,
				     pmic_regulator_matches_size);
	if ((matched < 0) || (matched != MT65XX_POWER_COUNT_END)) {
		pr_err("[PMIC]Error parsing regulator init data: %d %d\n", matched,
			MT65XX_POWER_COUNT_END);
		return matched;
	}

	/*for (i = 0; i < ARRAY_SIZE(pmic_regulator_matches); i++) {*/
	for (i = 0; i < pmic_regulator_matches_size; i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			mtk_ldos[i].config.dev = &(pdev->dev);
			mtk_ldos[i].config.init_data = pmic_regulator_matches[i].init_data;
			mtk_ldos[i].config.of_node = pmic_regulator_matches[i].of_node;
			mtk_ldos[i].config.driver_data = pmic_regulator_matches[i].driver_data;
			mtk_ldos[i].desc.owner = THIS_MODULE;

			mtk_ldos[i].rdev =
			    regulator_register(&mtk_ldos[i].desc, &mtk_ldos[i].config);

			if (IS_ERR(mtk_ldos[i].rdev)) {
				ret = PTR_ERR(mtk_ldos[i].rdev);
				pr_warn("[regulator_register] failed to register %s (%d)\n",
					mtk_ldos[i].desc.name, ret);
				continue;
			} else {
				PMICLOG("[regulator_register] pass to register %s\n",
					mtk_ldos[i].desc.name);
			}

#ifdef LDO_REGULATOR_TEST
			mtk_ldos[i].reg = regulator_get(&(pdev->dev), mtk_ldos[i].desc.name);
			isEn = regulator_is_enabled(mtk_ldos[i].reg);
			if (isEn != 0) {
				PMICLOG("[regulator] %s is default on\n", mtk_ldos[i].desc.name);
				/*ret=regulator_enable(mtk_ldos[i].reg);*/
			}
#endif /*--LDO_REGULATOR_TEST--*/
			/* To initialize varriables which were used to record status, */
			/* if ldo regulator have been modified by user.               */
			/* mtk_ldos[i].vosel.ldo_user = mtk_ldos[i].rdev->use_count;  */
			mtk_ldos[i].vosel.def_sel = mtk_regulator_get_voltage_sel(mtk_ldos[i].rdev);
			mtk_ldos[i].vosel.cur_sel = mtk_ldos[i].vosel.def_sel;

			PMICLOG("[PMIC]mtk_ldos[%d].config.init_data min_uv:%d max_uv:%d\n",
				i, mtk_ldos[i].config.init_data->constraints.min_uV,
				mtk_ldos[i].config.init_data->constraints.max_uV);
		}
	}
	of_node_put(regulators);
	return 0;

out:
	of_node_put(np);
	return ret;
}

static int pmic_mt_cust_probe(struct platform_device *pdev)
{
	struct device_node *np, *nproot, *regulators, *child;
#ifdef non_ks
	const struct of_device_id *match;
#endif
	int ret;
	unsigned int i = 0, default_on;

	PMICLOG("[PMIC]pmic_mt_cust_probe %s %s\n", pdev->name, pdev->id_entry->name);
#ifdef non_ks
	/* check if device_id is matched */
	match = of_match_device(pmic_cust_of_ids, &pdev->dev);
	if (!match) {
		pr_warn("[PMIC]pmic_cust_of_ids do not matched\n");
		return -EINVAL;
	}

	/* check customer setting */
	nproot = of_find_compatible_node(NULL, NULL, "mediatek,mt6328");
	if (nproot == NULL) {
		pr_info("[PMIC]pmic_mt_cust_probe get node failed\n");
		return -ENOMEM;
	}

	np = of_node_get(nproot);
	if (!np) {
		pr_info("[PMIC]pmic_mt_cust_probe of_node_get fail\n");
		return -ENODEV;
	}

	regulators = of_find_node_by_name(np, "regulators");
	if (!regulators) {
		pr_info("[PMIC]failed to find regulators node\n");
		ret = -ENODEV;
		goto out;
	}
	for_each_child_of_node(regulators, child) {
		/* check ldo regualtors and set it */
		/*for (i = 0; i < ARRAY_SIZE(pmic_regulator_matches); i++) {*/
		for (i = 0; i < pmic_regulator_matches_size; i++) {
			/* compare dt name & ldos name */
			if (!of_node_cmp(child->name, pmic_regulator_matches[i].name)) {
				PMICLOG("[PMIC]%s regulator_matches %s\n", child->name,
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			}
		}
		/*if (i == ARRAY_SIZE(pmic_regulator_matches))*/
		if (i == pmic_regulator_matches_size)
			continue;
		if (!of_property_read_u32(child, "regulator-default-on", &default_on)) {
			switch (default_on) {
			case 0:
				/* skip */
				PMICLOG("[PMIC]%s regulator_skip %s\n", child->name,
					pmic_regulator_matches[i].name);
				break;
			case 1:
				/* turn ldo off */
				pmic_set_register_value(mtk_ldos[i].en_reg, false);
				PMICLOG("[PMIC]%s default is off\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			case 2:
				/* turn ldo on */
				pmic_set_register_value(mtk_ldos[i].en_reg, true);
				PMICLOG("[PMIC]%s default is on\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			default:
				break;
			}
		}
	}
	of_node_put(regulators);
	PMICLOG("[PMIC]pmic_mt_cust_probe done\n");
	return 0;
#else
	nproot = of_find_compatible_node(NULL, NULL, "mediatek,mt_pmic");
	if (nproot == NULL) {
		pr_info("[PMIC]pmic_mt_cust_probe get node failed\n");
		return -ENOMEM;
	}

	np = of_node_get(nproot);
	if (!np) {
		pr_info("[PMIC]pmic_mt_cust_probe of_node_get fail\n");
		return -ENODEV;
	}

	regulators = of_get_child_by_name(np, "ldo_regulators");
	if (!regulators) {
		PMICLOG("[PMIC]pmic_mt_cust_probe ldo regulators node not found\n");
		ret = -ENODEV;
		goto out;
	}

	for_each_child_of_node(regulators, child) {
		/* check ldo regualtors and set it */
		if (!of_property_read_u32(child, "regulator-default-on", &default_on)) {
			switch (default_on) {
			case 0:
				/* skip */
				PMICLOG("[PMIC]%s regulator_skip %s\n", child->name,
					pmic_regulator_matches[i].name);
				break;
			case 1:
				/* turn ldo off */
				pmic_set_register_value(mtk_ldos[i].en_reg, false);
				PMICLOG("[PMIC]%s default is off\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			case 2:
				/* turn ldo on */
				pmic_set_register_value(mtk_ldos[i].en_reg, true);
				PMICLOG("[PMIC]%s default is on\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			default:
				break;
			}
		}
	}
	of_node_put(regulators);
	pr_err("[PMIC]pmic_mt_cust_probe done\n");
	return 0;
#endif
out:
	of_node_put(np);
	return ret;
}

static int pmic_mt_cust_remove(struct platform_device *pdev)
{
       /*platform_driver_unregister(&mt_pmic_driver);*/
	return 0;
}

static struct platform_driver mt_pmic_driver = {
	.driver = {
		   .name = "pmic_regulator",
		   .owner = THIS_MODULE,
		   .of_match_table = pmic_cust_of_ids,
		   },
	.probe = pmic_mt_cust_probe,
	.remove = pmic_mt_cust_remove,
/*      .id_table = pmic_regulator_id,*/
};
#endif				/* End of #ifdef CONFIG_OF */
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */

void pmic_regulator_suspend(void)
{
	int i;

	for (i = 0; i < mtk_ldos_size; i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			if (mtk_ldos[i].vol_reg != 0) {
				mtk_ldos[i].vosel.cur_sel = mtk_regulator_get_voltage_sel(mtk_ldos[i].rdev);
				if (mtk_ldos[i].vosel.cur_sel != mtk_ldos[i].vosel.def_sel) {
					mtk_ldos[i].vosel.restore = true;
					pr_err("pmic_regulator_suspend(name=%s id=%d default_sel=%d current_sel=%d)\n",
							mtk_ldos[i].rdev->desc->name, mtk_ldos[i].rdev->desc->id,
							mtk_ldos[i].vosel.def_sel, mtk_ldos[i].vosel.cur_sel);
				} else
					mtk_ldos[i].vosel.restore = false;
			}
		}
	}

}

void pmic_regulator_resume(void)
{
	int i, selector;

	/*for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {*/
	for (i = 0; i < mtk_ldos_size; i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			if (mtk_ldos[i].vol_reg != 0) {
				if (mtk_ldos[i].vosel.restore == true) {
					/*-- regulator voltage changed? --*/
						selector = mtk_ldos[i].vosel.cur_sel;
						pmic_set_register_value(mtk_ldos[i].vol_reg, selector);
						pr_err("pmic_regulator_resume(name=%s id=%d default_sel=%d current_sel=%d)\n",
							mtk_ldos[i].rdev->desc->name, mtk_ldos[i].rdev->desc->id,
							mtk_ldos[i].vosel.def_sel, mtk_ldos[i].vosel.cur_sel);
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
#if defined CONFIG_MTK_LEGACY
	int i = 0;
	/*int ret = 0;*/
	int isEn = 0;
#endif
	int ret = 0;

/*#if !defined CONFIG_MTK_LEGACY*//*Jimmy*/
#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	pmic_regulator_init(dev);
#endif
#else
	/*for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {*/
	for (i = 0; i < mtk_ldos_size; i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			mtk_ldos[i].config.dev = &(dev->dev);
			mtk_ldos[i].config.init_data = &mtk_ldos[i].init_data;
			if (mtk_ldos[i].desc.n_voltages != 1) {
				const int *pVoltage;

				if (mtk_ldos[i].vol_reg != 0) {
					if (mtk_ldos[i].pvoltages != NULL) {
						pVoltage = (const int *)mtk_ldos[i].pvoltages;
						mtk_ldos[i].init_data.constraints.max_uV =
						    pVoltage[mtk_ldos[i].desc.n_voltages - 1];
						mtk_ldos[i].init_data.constraints.min_uV =
						    pVoltage[0];
					} else {
						mtk_ldos[i].init_data.constraints.max_uV =
						    (mtk_ldos[i].desc.n_voltages -
						     1) * mtk_ldos[i].desc.uV_step +
						    mtk_ldos[i].desc.min_uV;
						mtk_ldos[i].init_data.constraints.min_uV =
						    mtk_ldos[i].desc.min_uV;
						PMICLOG("test man_uv:%d min_uv:%d\n",
							(mtk_ldos[i].desc.n_voltages -
							 1) * mtk_ldos[i].desc.uV_step +
							mtk_ldos[i].desc.min_uV,
							mtk_ldos[i].desc.min_uV);
					}
				}
				PMICLOG("min_uv:%d max_uv:%d\n",
					mtk_ldos[i].init_data.constraints.min_uV,
					mtk_ldos[i].init_data.constraints.max_uV);
			}

			mtk_ldos[i].desc.owner = THIS_MODULE;

			mtk_ldos[i].rdev =
			    regulator_register(&mtk_ldos[i].desc, &mtk_ldos[i].config);
			if (IS_ERR(mtk_ldos[i].rdev)) {
				ret = PTR_ERR(mtk_ldos[i].rdev);
				PMICLOG("[regulator_register] failed to register %s (%d)\n",
					mtk_ldos[i].desc.name, ret);
			} else {
				PMICLOG("[regulator_register] pass to register %s\n",
					mtk_ldos[i].desc.name);
			}

#ifdef LDO_REGULATOR_TEST
			mtk_ldos[i].reg = regulator_get(&(dev->dev), mtk_ldos[i].desc.name);
			isEn = regulator_is_enabled(mtk_ldos[i].reg);
			if (isEn != 0) {
				PMICLOG("[regulator] %s is default on\n", mtk_ldos[i].desc.name);
				/*ret=regulator_enable(mtk_ldos[i].reg);*/
			}
#endif /*--LDO_REGULATOR_TEST--*/
		}
	}
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */
/*#if !defined CONFIG_MTK_LEGACY*//*Jimmy*/
#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	ret = platform_driver_register(&mt_pmic_driver);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to register driver by DT(%d)\n", ret);
		return ret;
	}
#endif				/* End of #ifdef CONFIG_OF */
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */

	ret = register_pm_notifier(&pmic_regulator_pm_notifier_block);
	if (ret) {
		PMICLOG("****failed to register PM notifier %d\n", ret);
		return ret;
	}

	return ret;
}


#ifdef LDO_REGULATOR_TEST
void PMIC6335_regulator_test(void)
{
	int i = 0, j, sj = 0;
	int ret1, ret2;
	struct regulator *reg;

	/*for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {*/
	for (i = 0; i < mtk_ldos_size; i++) {
		/*---VIO18 should not be off---*/
		if (i != 1) {
			if (mtk_ldos[i].isUsedable == 1) {
				reg = mtk_ldos[i].reg;
				PMICLOG("[regulator enable test] %s\n", mtk_ldos[i].desc.name);

				ret1 = regulator_enable(reg);
				ret2 = regulator_is_enabled(reg);

				if (ret2 == pmic_get_register_value(mtk_ldos[i].en_reg)) {
					PMICLOG("[enable test pass]\n");
				} else {
					PMICLOG("[enable test fail] ret = %d enable = %d addr:0x%x reg:0x%x\n",
					ret1, ret2, pmu_flags_table[mtk_ldos[i].en_reg].offset,
					pmic_get_register_value(mtk_ldos[i].en_reg));
				}

				ret1 = regulator_disable(reg);
				ret2 = regulator_is_enabled(reg);

				if (ret2 == pmic_get_register_value(mtk_ldos[i].en_reg)) {
					PMICLOG("[disable test pass]\n");
				} else {
					PMICLOG("[disable test fail] ret = %d enable = %d addr:0x%x reg:0x%x\n",
					ret1, ret2, pmu_flags_table[mtk_ldos[i].en_reg].offset,
					pmic_get_register_value(mtk_ldos[i].en_reg));
				}
			}
		} /*---VIO18 should not be off---*/
	}

	for (i = 0; i < mtk_ldos_size; i++) {
		const int *pVoltage;

		reg = mtk_ldos[i].reg;
		/*---VIO18 should not be off---*/
		if (i != 1 && (mtk_ldos[i].isUsedable == 1)) {
			PMICLOG("[regulator voltage test] %s voltage:%d\n",
				mtk_ldos[i].desc.name, mtk_ldos[i].desc.n_voltages);

			if (mtk_ldos[i].pvoltages != NULL) {
				pVoltage = (const int *)mtk_ldos[i].pvoltages;

				if (i == 8)
					sj = 8;
				else
					sj = 0;

				for (j = sj; j < mtk_ldos[i].desc.n_voltages; j++) {
					int rvoltage;

					regulator_set_voltage(reg, pVoltage[j], pVoltage[j]);
					rvoltage = regulator_get_voltage(reg);

					if ((j == pmic_get_register_value(mtk_ldos[i].vol_reg))
						   && (pVoltage[j] == rvoltage)) {
						PMICLOG("[%d:%d]:pass  set_voltage:%d  rvoltage:%d\n",
							j, pmic_get_register_value(mtk_ldos[i].vol_reg),
							pVoltage[j], rvoltage);
					} else {
						PMICLOG("[%d:%d]:fail  set_voltage:%d  rvoltage:%d\n",
							j, pmic_get_register_value(mtk_ldos[i].vol_reg),
							pVoltage[j], rvoltage);
					}
				}
			}
		}
	} /*---VIO18 should not be off---*/
}
#endif /*--LDO_REGULATOR_TEST--*/

/*#if defined CONFIG_MTK_LEGACY*/
int getHwVoltage(MT65XX_POWER powerId)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;

	if (powerId == MT65XX_POWER_NONE) {
		pr_err("[PMIC]getHwVoltage failed: plz check the powerId, since it's none\n");
		return -1;
	}

	/*if (powerId >= ARRAY_SIZE(mtk_ldos))*/
	if (powerId >= mtk_ldos_size)
		return -100;

	if (mtk_ldos[powerId].isUsedable != true)
		return -101;

	reg = mtk_ldos[powerId].reg;

	return regulator_get_voltage(reg);
#else
	return -1;
#endif
}
EXPORT_SYMBOL(getHwVoltage);

int isHwPowerOn(MT65XX_POWER powerId)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;

	if (powerId == MT65XX_POWER_NONE) {
		pr_err("[PMIC]isHwPowerOn failed: plz check the powerId, since it's none\n");
		return -1;
	}

	/*if (powerId >= ARRAY_SIZE(mtk_ldos))*/
	if (powerId >= mtk_ldos_size)
		return -100;

	if (mtk_ldos[powerId].isUsedable != true)
		return -101;

	reg = mtk_ldos[powerId].reg;

	return regulator_is_enabled(reg);
#else
	return -1;
#endif

}
EXPORT_SYMBOL(isHwPowerOn);

bool hwPowerOn(MT65XX_POWER powerId, int powerVolt, char *mode_name)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;
	int ret1, ret2;

	if (powerId == MT65XX_POWER_NONE) {
		pr_err("[PMIC]hwPowerOn failed:plz check the powerId, since it's none\n");
		return -1;
	}

	/*if (powerId >= ARRAY_SIZE(mtk_ldos))*/
	if (powerId >= mtk_ldos_size)
		return false;

	if (mtk_ldos[powerId].isUsedable != true)
		return false;

	reg = mtk_ldos[powerId].reg;

	ret2 = regulator_set_voltage(reg, powerVolt, powerVolt);

	if (ret2 != 0) {
		PMICLOG("hwPowerOn:%s can't set the same volt %d again.", mtk_ldos[powerId].desc.name, powerVolt);
		PMICLOG("Or please check:%s volt %d is correct.", mtk_ldos[powerId].desc.name, powerVolt);
	}

	ret1 = regulator_enable(reg);

	if (ret1 != 0)
		PMICLOG("hwPowerOn:%s enable fail", mtk_ldos[powerId].desc.name);

	PMICLOG("hwPowerOn:%d:%s volt:%d name:%s cnt:%d", powerId, mtk_ldos[powerId].desc.name,
		powerVolt, mode_name, mtk_ldos[powerId].rdev->use_count);
	return true;
#else
	return false;
#endif
}
EXPORT_SYMBOL(hwPowerOn);

bool hwPowerDown(MT65XX_POWER powerId, char *mode_name)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;
	int ret1;

	if (powerId == MT65XX_POWER_NONE) {
		pr_err("[PMIC]hwPowerDown failed:plz check the powerId, since it's none\n");
		return -1;
	}

	/*if (powerId >= ARRAY_SIZE(mtk_ldos))*/
	if (powerId >= mtk_ldos_size)
		return false;

	if (mtk_ldos[powerId].isUsedable != true)
		return false;
	reg = mtk_ldos[powerId].reg;
	ret1 = regulator_disable(reg);

	if (ret1 != 0)
		PMICLOG("hwPowerOn err:ret1:%d ", ret1);

	PMICLOG("hwPowerDown:%d:%s name:%s cnt:%d", powerId, mtk_ldos[powerId].desc.name, mode_name,
		mtk_ldos[powerId].rdev->use_count);
	return true;
#else
	return false;
#endif
}
EXPORT_SYMBOL(hwPowerDown);

bool hwPowerSetVoltage(MT65XX_POWER powerId, int powerVolt, char *mode_name)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;
	int ret1;

	if (powerId == MT65XX_POWER_NONE) {
		pr_err("[PMIC]hwPowerSetVoltage failed:plz check the powerId, since it's none\n");
		return -1;
	}

	/*if (powerId >= ARRAY_SIZE(mtk_ldos))*/
	if (powerId >= mtk_ldos_size)
		return false;

	reg = mtk_ldos[powerId].reg;

	ret1 = regulator_set_voltage(reg, powerVolt, powerVolt);

	if (ret1 != 0) {
		PMICLOG("hwPowerSetVoltage:%s can't set the same voltage %d", mtk_ldos[powerId].desc.name,
			powerVolt);
	}


	PMICLOG("hwPowerSetVoltage:%d:%s name:%s cnt:%d", powerId, mtk_ldos[powerId].desc.name,
		mode_name, mtk_ldos[powerId].rdev->use_count);
	return true;
#else
	return false;
#endif
}
EXPORT_SYMBOL(hwPowerSetVoltage);

/*#endif*/ /* End of #if defined CONFIG_MTK_LEGACY */

/*****************************************************************************
 * Dump all LDO status
 ******************************************************************************/
void dump_ldo_status_read_debug(void)
{
	int i, j;
	int en = 0;
	int voltage_reg = 0;
	int voltage = 0;
	const int *pVoltage;

	pr_debug("********** BUCK/LDO status dump [1:ON,0:OFF]**********\n");

	/*for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {*/
	for (i = 0; i < mtk_bucks_size; i++) {
		if (mtk_bucks_class[i].da_qi_en != 0)
			en = pmic_get_register_value(mtk_bucks_class[i].da_qi_en);
		else
			en = -1;

		if (mtk_bucks_class[i].da_ni_vosel != 0) {
			voltage_reg = pmic_get_register_value(mtk_bucks_class[i].da_ni_vosel);
			voltage =
			    mtk_bucks_class[i].min_uV + mtk_bucks_class[i].uV_step * voltage_reg;
		} else {
			voltage_reg = -1;
			voltage = -1;
		}
		pr_err("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			mtk_bucks_class[i].name, en, voltage, voltage_reg);
	}

	/*for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {*/
	for (i = 0; i < mtk_ldos_size; i++) {
		if (mtk_ldos[i].en_reg != 0)
			en = pmic_get_register_value(mtk_ldos[i].en_reg);
		else
			en = -1;

		if (mtk_ldos[i].desc.n_voltages != 1) {
			if (mtk_ldos[i].vol_reg != 0) {
				voltage_reg = pmic_get_register_value(mtk_ldos[i].vol_reg);
				if (mtk_ldos[i].pvoltages != NULL) {
					pVoltage = (const int *)mtk_ldos[i].pvoltages;
					/*HW LDO sequence issue, we need to change it */
					voltage = pVoltage[voltage_reg];
				} else {
					voltage =
					    mtk_ldos[i].desc.min_uV +
					    mtk_ldos[i].desc.uV_step * voltage_reg;
				}
			} else {
				voltage_reg = -1;
				voltage = -1;
			}
		} else {
			pVoltage = (const int *)mtk_ldos[i].pvoltages;
			voltage = pVoltage[0];
		}

		pr_err("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			mtk_ldos[i].desc.name, en, voltage, voltage_reg);
	}

	for (i = 0; i < interrupts_size; i++) {
		for (j = 0; j < PMIC_INT_WIDTH; j++) {

			PMICLOG("[PMIC_INT][%s] interrupt issue times: %d\n",
				interrupts[i].interrupts[j].name,
				interrupts[i].interrupts[j].times);
		}
	}

	PMICLOG("Power Good Status 0=0x%x. 1=0x%x\n", upmu_get_reg_value(MT6335_PGSTATUS0),
			upmu_get_reg_value(MT6335_PGSTATUS1));
	PMICLOG("Power Source OC Status =0x%x.\n", upmu_get_reg_value(MT6335_PSOCSTATUS));
	PMICLOG("Thermal Status=0x%x.\n", upmu_get_reg_value(MT6335_THERMALSTATUS));
}

static int proc_utilization_show(struct seq_file *m, void *v)
{
	int i, j;
	int en = 0;
	int voltage_reg = 0;
	int voltage = 0;
	const int *pVoltage;

	seq_puts(m, "********** BUCK/LDO status dump [1:ON,0:OFF]**********\n");

	/*for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {*/
	for (i = 0; i < mtk_bucks_size; i++) {
		if (mtk_bucks_class[i].da_qi_en != 0)
			en = pmic_get_register_value(mtk_bucks_class[i].da_qi_en);
		else
			en = -1;

		if (mtk_bucks_class[i].da_ni_vosel != 0) {
			voltage_reg = pmic_get_register_value(mtk_bucks_class[i].da_ni_vosel);
			voltage =
			    mtk_bucks_class[i].min_uV + mtk_bucks_class[i].uV_step * voltage_reg;
		} else {
			voltage_reg = -1;
			voltage = -1;
		}
		seq_printf(m, "%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			   mtk_bucks_class[i].name, en, voltage, voltage_reg);
	}

	/*for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {*/
	for (i = 0; i < mtk_ldos_size; i++) {
		if (mtk_ldos[i].en_reg != 0)
			en = pmic_get_register_value(mtk_ldos[i].en_reg);
		else
			en = -1;

		if (mtk_ldos[i].desc.n_voltages != 1) {
			if (mtk_ldos[i].vol_reg != 0) {
				voltage_reg = pmic_get_register_value(mtk_ldos[i].vol_reg);
				if (mtk_ldos[i].pvoltages != NULL) {
					pVoltage = (const int *)mtk_ldos[i].pvoltages;
					/*HW LDO sequence issue, we need to change it */
					voltage = pVoltage[voltage_reg];
				} else {
					voltage =
					    mtk_ldos[i].desc.min_uV +
					    mtk_ldos[i].desc.uV_step * voltage_reg;
				}
			} else {
				voltage_reg = -1;
				voltage = -1;
			}
		} else {
			pVoltage = (const int *)mtk_ldos[i].pvoltages;
			voltage = pVoltage[0];
		}
		seq_printf(m, "%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			   mtk_ldos[i].desc.name, en, voltage, voltage_reg);
	}

	for (i = 0; i < interrupts_size; i++) {
		for (j = 0; j < PMIC_INT_WIDTH; j++) {

			seq_printf(m, "[PMIC_INT][%s] interrupt issue times: %d\n",
				   interrupts[i].interrupts[j].name,
				   interrupts[i].interrupts[j].times);
		}
	}
	seq_printf(m, "Power Good Status 0=0x%x. 1=0x%x\n", upmu_get_reg_value(MT6335_PGSTATUS0),
			upmu_get_reg_value(MT6335_PGSTATUS1));
	seq_printf(m, "Power Source OC Status=0x%x.\n", upmu_get_reg_value(MT6335_PSOCSTATUS));
	seq_printf(m, "Thermal Status=0x%x.\n", upmu_get_reg_value(MT6335_THERMALSTATUS));

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

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_buck_voltage);

	/* /sys/class/regulator/.../ */
	/*EM BUCK voltage & Status*/
	/*for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {*/
	for (i = 0; i < mtk_bucks_size; i++) {
		/*PMICLOG("[PMIC] register buck id=%d\n",i);*/
		ret_device_file = device_create_file(&(dev->dev), &mtk_bucks_class[i].en_att);
		/*ret_device_file = device_create_file(&(dev->dev), &mtk_bucks_class[i].voltage_att);*/
	}
	/*ret_device_file = device_create_file(&(dev->dev), &mtk_bucks_class[i].voltage_att);*/
	/*EM ldo voltage & Status*/
	/*for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {*/
	for (i = 0; i < mtk_ldos_size; i++) {
		/*PMICLOG("[PMIC] register ldo id=%d\n",i);*/
		ret_device_file = device_create_file(&(dev->dev), &mtk_ldos[i].en_att);
		ret_device_file = device_create_file(&(dev->dev), &mtk_ldos[i].voltage_att);
	}

	debugfs_create_file("dump_ldo_status", S_IRUGO | S_IWUSR, mt_pmic_dir, NULL, &pmic_debug_proc_fops);
	PMICLOG("proc_create pmic_debug_proc_fops\n");

}

MODULE_AUTHOR("Jimmy-YJ Huang");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");
