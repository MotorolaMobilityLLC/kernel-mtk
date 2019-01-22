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

#include <linux/delay.h>
#include <mt-plat/upmu_common.h>
#include "include/pmic.h"
#include "include/pmic_api.h"
#include "include/pmic_api_ldo.h"
#include "include/pmic_api_buck.h"
#include "include/regulator_codegen.h"

static const int vgp3_voltages[] = {
	1000000,
	1050000,
	1100000,
	1220000,
	1300000,
	1500000,
	1800000,
};

static const int vcamd1_voltages[] = {
	900000,
	950000,
	1000000,
	1050000,
	1100000,
	1200000,
	1210000,
};

static const int vsim1_voltages[] = {
	1200000,
	1300000,
	1700000,
	1800000,
	1860000,
	2760000,
	3000000,
	3100000,
};

static const int vibr_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2000000,
	2800000,
	3000000,
	3300000,
};

static const int vrf12_voltages[] = {
	1200000,
};

static const int vcamio_voltages[] = {
	1800000,
};

static const int va18_voltages[] = {
	1800000,
};

static const int vcn18_bt_voltages[] = {
	1800000,
};

static const int vcn18_wifi_voltages[] = {
	1800000,
};

static const int vfe28_voltages[] = {
	2800000,
};

static const int vrf18_2_voltages[] = {
	1810000,
};

static const int vmipi_voltages[] = {
	1800000,
};

static const int vcn28_voltages[] = {
	2800000,
};

static const int vefuse_voltages[] = {
	1200000,
	1200000,
	1200000,
	1200000,
	1200000,
	1200000,
	1200000,
	2200000,
	1200000,
	1300000,
	1710000,
	1800000,
	1890000,
	2000000,
	2100000,
	2200000,
};

static const int vrf18_1_voltages[] = {
	1810000,
};

static const int vcamd2_voltages[] = {
	900000,
	1000000,
	1050000,
	1100000,
	1210000,
	1300000,
	1500000,
	1800000,
};

static const int vmch_voltages[] = {
	2900000,
	2900000,
	3000000,
	3300000,
};

static const int vcama1_voltages[] = {
	1800000,
	2800000,
	2900000,
	2500000,
};

static const int vemc_voltages[] = {
	2900000,
	2900000,
	3000000,
	3300000,
};

static const int vio28_voltages[] = {
	2800000,
};

static const int va12_voltages[] = {
	1200000,
};

static const int va10_voltages[] = {
	800000,
	850000,
	875000,
	900000,
	950000,
	1000000,
};

static const int vcn33_bt_voltages[] = {
	3300000,
	3400000,
	3500000,
	3600000,
};

static const int vcn33_wifi_voltages[] = {
	3300000,
	3400000,
	3500000,
	3600000,
};

static const int vbif28_voltages[] = {
	2800000,
};

static const int vcama2_voltages[] = {
	1800000,
	2800000,
	2900000,
	3000000,
};

static const int vmc_voltages[] = {
	1860000,
	2900000,
	3000000,
	3300000,
};

static const int vxo22_voltages[] = {
	2200000,
};

static const int vsim2_voltages[] = {
	1200000,
	1300000,
	1700000,
	1800000,
	1860000,
	2760000,
	3000000,
	3100000,
};

static const int vio18_voltages[] = {
	1800000,
};

static const int vufs18_voltages[] = {
	1800000,
};

static const int vusb33_voltages[] = {
	3300000,
};

static const int vcamaf_voltages[] = {
	2800000,
};

static const int vtouch_voltages[] = {
	2800000,
};



/* Regulator vgp3 enable */
static int pmic_ldo_vgp3_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vgp3 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vgp3 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vgp3 disable */
static int pmic_ldo_vgp3_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vgp3 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vgp3 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vgp3 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vgp3 is_enabled */
static int pmic_ldo_vgp3_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vgp3 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vgp3 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vgp3 set_voltage_sel */
static int pmic_ldo_vgp3_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vgp3 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vgp3 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vgp3 get_voltage_sel */
static int pmic_ldo_vgp3_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vgp3 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vgp3 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vgp3 list_voltage */
static int pmic_ldo_vgp3_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vgp3_voltages[selector];
	PMICLOG("ldo vgp3 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vcamd1 enable */
static int pmic_ldo_vcamd1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd1 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcamd1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcamd1 disable */
static int pmic_ldo_vcamd1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcamd1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcamd1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcamd1 is_enabled */
static int pmic_ldo_vcamd1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcamd1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcamd1 set_voltage_sel */
static int pmic_ldo_vcamd1_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vcamd1 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vcamd1 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcamd1 get_voltage_sel */
static int pmic_ldo_vcamd1_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd1 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vcamd1 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcamd1 list_voltage */
static int pmic_ldo_vcamd1_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vcamd1_voltages[selector];
	PMICLOG("ldo vcamd1 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vsim1 enable */
static int pmic_ldo_vsim1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim1 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vsim1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsim1 disable */
static int pmic_ldo_vsim1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vsim1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vsim1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vsim1 is_enabled */
static int pmic_ldo_vsim1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vsim1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsim1 set_voltage_sel */
static int pmic_ldo_vsim1_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vsim1 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vsim1 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsim1 get_voltage_sel */
static int pmic_ldo_vsim1_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim1 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vsim1 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vsim1 list_voltage */
static int pmic_ldo_vsim1_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vsim1_voltages[selector];
	PMICLOG("ldo vsim1 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vs1 enable */
static int pmic_buck_vs1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs1 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("buck vs1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vs1 disable */
static int pmic_buck_vs1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vs1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vs1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vs1 is_enabled */
static int pmic_buck_vs1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vs1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vs1 set_voltage_sel */
static int pmic_buck_vs1_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vs1 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vs1 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vs1 get_voltage_sel */
static int pmic_buck_vs1_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs1 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vs1 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vibr enable */
static int pmic_ldo_vibr_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vibr enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vibr don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vibr disable */
static int pmic_ldo_vibr_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vibr disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vibr should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vibr don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vibr is_enabled */
static int pmic_ldo_vibr_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vibr is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vibr don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vibr set_voltage_sel */
static int pmic_ldo_vibr_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vibr set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vibr don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vibr get_voltage_sel */
static int pmic_ldo_vibr_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vibr get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vibr don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vibr list_voltage */
static int pmic_ldo_vibr_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vibr_voltages[selector];
	PMICLOG("ldo vibr list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vrf12 enable */
static int pmic_ldo_vrf12_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf12 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vrf12 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vrf12 disable */
static int pmic_ldo_vrf12_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf12 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vrf12 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vrf12 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vrf12 is_enabled */
static int pmic_ldo_vrf12_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf12 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vrf12 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vimvo enable */
static int pmic_buck_vimvo_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vimvo enable\n");
	if (mreg->en_cb != NULL) {
		pmic_config_interface(0x0FE4, 0x1, 0x1, 12);    /* 0x0FE4[12] = 1 */
		ret = (mreg->en_cb)(1);
		udelay(220);
		pmic_config_interface(0x0FE4, 0x0, 0x1, 12);    /* 0x0FE4[12] = 0 */
	} else {
		pr_err("buck vimvo don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vimvo disable */
static int pmic_buck_vimvo_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vimvo disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vimvo should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vimvo don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vimvo is_enabled */
static int pmic_buck_vimvo_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vimvo is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vimvo don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vimvo set_voltage_sel */
static int pmic_buck_vimvo_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vimvo set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vimvo don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vimvo get_voltage_sel */
static int pmic_buck_vimvo_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vimvo get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vimvo don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vsram_vcore enable */
static int pmic_ldo_vsram_vcore_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vcore enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vsram_vcore don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_vcore disable */
static int pmic_ldo_vsram_vcore_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vcore disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vsram_vcore should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vsram_vcore don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vsram_vcore is_enabled */
static int pmic_ldo_vsram_vcore_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vcore is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vsram_vcore don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_vcore set_voltage_sel */
static int pmic_ldo_vsram_vcore_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vsram_vcore set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vsram_vcore don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_vcore get_voltage_sel */
static int pmic_ldo_vsram_vcore_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vcore get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vsram_vcore don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcamio enable */
static int pmic_ldo_vcamio_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamio enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcamio don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcamio disable */
static int pmic_ldo_vcamio_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamio disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcamio should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcamio don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcamio is_enabled */
static int pmic_ldo_vcamio_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamio is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcamio don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator va18 enable */
static int pmic_ldo_va18_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va18 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo va18 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator va18 disable */
static int pmic_ldo_va18_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va18 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo va18 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo va18 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator va18 is_enabled */
static int pmic_ldo_va18_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va18 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo va18 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_vmd enable */
static int pmic_ldo_vsram_vmd_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vmd enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vsram_vmd don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_vmd disable */
static int pmic_ldo_vsram_vmd_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vmd disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vsram_vmd should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vsram_vmd don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vsram_vmd is_enabled */
static int pmic_ldo_vsram_vmd_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vmd is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vsram_vmd don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_vmd set_voltage_sel */
static int pmic_ldo_vsram_vmd_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vsram_vmd set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vsram_vmd don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_vmd get_voltage_sel */
static int pmic_ldo_vsram_vmd_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vmd get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vsram_vmd don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcn18_bt enable */
static int pmic_ldo_vcn18_bt_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn18_bt enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcn18_bt don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcn18_bt disable */
static int pmic_ldo_vcn18_bt_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn18_bt disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcn18_bt should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcn18_bt don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcn18_bt is_enabled */
static int pmic_ldo_vcn18_bt_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn18_bt is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcn18_bt don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcn18_wifi enable */
static int pmic_ldo_vcn18_wifi_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn18_wifi enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcn18_wifi don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcn18_wifi disable */
static int pmic_ldo_vcn18_wifi_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn18_wifi disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcn18_wifi should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcn18_wifi don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcn18_wifi is_enabled */
static int pmic_ldo_vcn18_wifi_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn18_wifi is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcn18_wifi don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vfe28 enable */
static int pmic_ldo_vfe28_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vfe28 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vfe28 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vfe28 disable */
static int pmic_ldo_vfe28_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vfe28 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vfe28 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vfe28 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vfe28 is_enabled */
static int pmic_ldo_vfe28_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vfe28 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vfe28 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vrf18_2 enable */
static int pmic_ldo_vrf18_2_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf18_2 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vrf18_2 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vrf18_2 disable */
static int pmic_ldo_vrf18_2_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf18_2 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vrf18_2 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vrf18_2 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vrf18_2 is_enabled */
static int pmic_ldo_vrf18_2_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf18_2 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vrf18_2 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vmodem enable */
static int pmic_buck_vmodem_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmodem enable\n");
	if (mreg->en_cb != NULL) {
		pmic_config_interface(0x0F88, 0x1, 0x1, 12);    /* 0x0F88[12] = 1 */
		ret = (mreg->en_cb)(1);
		udelay(220);
		pmic_config_interface(0x0F88, 0x0, 0x1, 12);    /* 0x0F88[12] = 0 */
	} else {
		pr_err("buck vmodem don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmodem disable */
static int pmic_buck_vmodem_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmodem disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vmodem should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vmodem don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vmodem is_enabled */
static int pmic_buck_vmodem_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmodem is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vmodem don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vmodem set_voltage_sel */
static int pmic_buck_vmodem_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vmodem set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vmodem don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmodem get_voltage_sel */
static int pmic_buck_vmodem_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmodem get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vmodem don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcore enable */
static int pmic_buck_vcore_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vcore enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("buck vcore don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcore disable */
static int pmic_buck_vcore_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vcore disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vcore should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vcore don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcore is_enabled */
static int pmic_buck_vcore_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vcore is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vcore don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcore set_voltage_sel */
static int pmic_buck_vcore_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vcore set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vcore don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcore get_voltage_sel */
static int pmic_buck_vcore_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vcore get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vcore don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vmipi enable */
static int pmic_ldo_vmipi_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmipi enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vmipi don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmipi disable */
static int pmic_ldo_vmipi_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmipi disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vmipi should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vmipi don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vmipi is_enabled */
static int pmic_ldo_vmipi_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmipi is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vmipi don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcn28 enable */
static int pmic_ldo_vcn28_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn28 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcn28 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcn28 disable */
static int pmic_ldo_vcn28_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn28 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcn28 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcn28 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcn28 is_enabled */
static int pmic_ldo_vcn28_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn28 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcn28 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_dvfs2 enable */
static int pmic_ldo_vsram_dvfs2_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs2 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vsram_dvfs2 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_dvfs2 disable */
static int pmic_ldo_vsram_dvfs2_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs2 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vsram_dvfs2 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vsram_dvfs2 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vsram_dvfs2 is_enabled */
static int pmic_ldo_vsram_dvfs2_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs2 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vsram_dvfs2 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_dvfs2 set_voltage_sel */
static int pmic_ldo_vsram_dvfs2_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vsram_dvfs2 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vsram_dvfs2 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_dvfs2 get_voltage_sel */
static int pmic_ldo_vsram_dvfs2_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs2 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vsram_dvfs2 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vs2 enable */
static int pmic_buck_vs2_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs2 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("buck vs2 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vs2 disable */
static int pmic_buck_vs2_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs2 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vs2 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vs2 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vs2 is_enabled */
static int pmic_buck_vs2_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs2 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vs2 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vs2 set_voltage_sel */
static int pmic_buck_vs2_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vs2 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vs2 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vs2 get_voltage_sel */
static int pmic_buck_vs2_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vs2 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vs2 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vefuse enable */
static int pmic_ldo_vefuse_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vefuse enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vefuse don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vefuse disable */
static int pmic_ldo_vefuse_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vefuse disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vefuse should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vefuse don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vefuse is_enabled */
static int pmic_ldo_vefuse_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vefuse is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vefuse don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vefuse set_voltage_sel */
static int pmic_ldo_vefuse_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	if (selector >= 0 && selector < 8)
		selector = 9;

	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vefuse set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vefuse don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vefuse get_voltage_sel */
static int pmic_ldo_vefuse_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vefuse get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vefuse don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vefuse list_voltage */
static int pmic_ldo_vefuse_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vefuse_voltages[selector];
	PMICLOG("ldo vefuse list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vpa1 enable */
static int pmic_buck_vpa1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vpa1 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("buck vpa1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vpa1 disable */
static int pmic_buck_vpa1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vpa1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vpa1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vpa1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vpa1 is_enabled */
static int pmic_buck_vpa1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vpa1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vpa1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vpa1 set_voltage_sel */
static int pmic_buck_vpa1_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vpa1 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vpa1 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vpa1 get_voltage_sel */
static int pmic_buck_vpa1_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vpa1 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vpa1 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vrf18_1 enable */
static int pmic_ldo_vrf18_1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf18_1 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vrf18_1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vrf18_1 disable */
static int pmic_ldo_vrf18_1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf18_1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vrf18_1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vrf18_1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vrf18_1 is_enabled */
static int pmic_ldo_vrf18_1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vrf18_1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vrf18_1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_vgpu enable */
static int pmic_ldo_vsram_vgpu_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vgpu enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vsram_vgpu don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_vgpu disable */
static int pmic_ldo_vsram_vgpu_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vgpu disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vsram_vgpu should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vsram_vgpu don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vsram_vgpu is_enabled */
static int pmic_ldo_vsram_vgpu_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vgpu is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vsram_vgpu don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_vgpu set_voltage_sel */
static int pmic_ldo_vsram_vgpu_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vsram_vgpu set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vsram_vgpu don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_vgpu get_voltage_sel */
static int pmic_ldo_vsram_vgpu_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_vgpu get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vsram_vgpu don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcamd2 enable */
static int pmic_ldo_vcamd2_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd2 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcamd2 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcamd2 disable */
static int pmic_ldo_vcamd2_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd2 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcamd2 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcamd2 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcamd2 is_enabled */
static int pmic_ldo_vcamd2_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd2 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcamd2 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcamd2 set_voltage_sel */
static int pmic_ldo_vcamd2_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vcamd2 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vcamd2 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcamd2 get_voltage_sel */
static int pmic_ldo_vcamd2_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamd2 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vcamd2 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcamd2 list_voltage */
static int pmic_ldo_vcamd2_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vcamd2_voltages[selector];
	PMICLOG("ldo vcamd2 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vmch enable */
static int pmic_ldo_vmch_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmch enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vmch don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmch disable */
static int pmic_ldo_vmch_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmch disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vmch should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vmch don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vmch is_enabled */
static int pmic_ldo_vmch_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmch is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vmch don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vmch set_voltage_sel */
static int pmic_ldo_vmch_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vmch set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vmch don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmch get_voltage_sel */
static int pmic_ldo_vmch_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmch get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vmch don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vmch list_voltage */
static int pmic_ldo_vmch_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vmch_voltages[selector];
	PMICLOG("ldo vmch list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vcama1 enable */
static int pmic_ldo_vcama1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama1 enable\n");
	if (mreg->en_cb != NULL) {
		ret = (mreg->en_cb)(1);
#if ENABLE_ALL_OC_IRQ
		mdelay(3);
		/* this OC interrupt needs to delay 1ms after enable power */
		pmic_enable_interrupt(INT_VCAMA1_OC, 1, "PMIC");
#endif
	} else {
		pr_err("ldo vcama1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcama1 disable */
static int pmic_ldo_vcama1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcama1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL) {
			ret = (mreg->en_cb)(0);
#if ENABLE_ALL_OC_IRQ
			/* after disable power, this OC interrupt should be disabled as well */
			pmic_enable_interrupt(INT_VCAMA1_OC, 0, "PMIC");
#endif
		} else {
			pr_err("ldo vcama1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcama1 is_enabled */
static int pmic_ldo_vcama1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcama1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcama1 set_voltage_sel */
static int pmic_ldo_vcama1_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vcama1 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vcama1 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcama1 get_voltage_sel */
static int pmic_ldo_vcama1_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama1 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vcama1 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcama1 list_voltage */
static int pmic_ldo_vcama1_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vcama1_voltages[selector];
	PMICLOG("ldo vcama1 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vemc enable */
static int pmic_ldo_vemc_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vemc enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vemc don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vemc disable */
static int pmic_ldo_vemc_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vemc disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vemc should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vemc don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vemc is_enabled */
static int pmic_ldo_vemc_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vemc is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vemc don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vemc set_voltage_sel */
static int pmic_ldo_vemc_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vemc set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vemc don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vemc get_voltage_sel */
static int pmic_ldo_vemc_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vemc get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vemc don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vemc list_voltage */
static int pmic_ldo_vemc_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vemc_voltages[selector];
	PMICLOG("ldo vemc list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vio28 enable */
static int pmic_ldo_vio28_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vio28 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vio28 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vio28 disable */
static int pmic_ldo_vio28_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vio28 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vio28 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vio28 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vio28 is_enabled */
static int pmic_ldo_vio28_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vio28 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vio28 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vmd1 enable */
static int pmic_buck_vmd1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmd1 enable\n");
	if (mreg->en_cb != NULL) {
		pmic_config_interface(0x0F9C, 0x1, 0x1, 12);    /* 0x0F9C[12] = 1 */
		ret = (mreg->en_cb)(1);
		udelay(220);
		pmic_config_interface(0x0F9C, 0x0, 0x1, 12);    /* 0x0F9C[12] = 0 */
	} else {
		pr_err("buck vmd1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmd1 disable */
static int pmic_buck_vmd1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmd1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vmd1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vmd1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vmd1 is_enabled */
static int pmic_buck_vmd1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmd1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vmd1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vmd1 set_voltage_sel */
static int pmic_buck_vmd1_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vmd1 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vmd1 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmd1 get_voltage_sel */
static int pmic_buck_vmd1_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vmd1 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vmd1 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator va12 enable */
static int pmic_ldo_va12_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va12 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo va12 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator va12 disable */
static int pmic_ldo_va12_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va12 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo va12 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo va12 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator va12 is_enabled */
static int pmic_ldo_va12_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va12 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo va12 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator va10 enable */
static int pmic_ldo_va10_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va10 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo va10 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator va10 disable */
static int pmic_ldo_va10_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va10 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo va10 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo va10 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator va10 is_enabled */
static int pmic_ldo_va10_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va10 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo va10 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator va10 set_voltage_sel */
static int pmic_ldo_va10_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo va10 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo va10 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator va10 get_voltage_sel */
static int pmic_ldo_va10_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo va10 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo va10 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator va10 list_voltage */
static int pmic_ldo_va10_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = va10_voltages[selector];
	PMICLOG("ldo va10 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vsram_dvfs1 enable */
static int pmic_ldo_vsram_dvfs1_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs1 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vsram_dvfs1 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_dvfs1 disable */
static int pmic_ldo_vsram_dvfs1_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs1 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vsram_dvfs1 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vsram_dvfs1 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vsram_dvfs1 is_enabled */
static int pmic_ldo_vsram_dvfs1_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs1 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vsram_dvfs1 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsram_dvfs1 set_voltage_sel */
static int pmic_ldo_vsram_dvfs1_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vsram_dvfs1 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vsram_dvfs1 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsram_dvfs1 get_voltage_sel */
static int pmic_ldo_vsram_dvfs1_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsram_dvfs1 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vsram_dvfs1 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcn33_bt enable */
static int pmic_ldo_vcn33_bt_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_bt enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcn33_bt don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcn33_bt disable */
static int pmic_ldo_vcn33_bt_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_bt disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcn33_bt should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcn33_bt don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcn33_bt is_enabled */
static int pmic_ldo_vcn33_bt_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_bt is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcn33_bt don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcn33_bt set_voltage_sel */
static int pmic_ldo_vcn33_bt_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vcn33_bt set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vcn33_bt don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcn33_bt get_voltage_sel */
static int pmic_ldo_vcn33_bt_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_bt get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vcn33_bt don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcn33_bt list_voltage */
static int pmic_ldo_vcn33_bt_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vcn33_bt_voltages[selector];
	PMICLOG("ldo vcn33_bt list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vcn33_wifi enable */
static int pmic_ldo_vcn33_wifi_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_wifi enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcn33_wifi don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcn33_wifi disable */
static int pmic_ldo_vcn33_wifi_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_wifi disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcn33_wifi should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcn33_wifi don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcn33_wifi is_enabled */
static int pmic_ldo_vcn33_wifi_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_wifi is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcn33_wifi don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcn33_wifi set_voltage_sel */
static int pmic_ldo_vcn33_wifi_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vcn33_wifi set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vcn33_wifi don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcn33_wifi get_voltage_sel */
static int pmic_ldo_vcn33_wifi_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcn33_wifi get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vcn33_wifi don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcn33_wifi list_voltage */
static int pmic_ldo_vcn33_wifi_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vcn33_wifi_voltages[selector];
	PMICLOG("ldo vcn33_wifi list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vbif28 enable */
static int pmic_ldo_vbif28_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vbif28 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vbif28 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vbif28 disable */
static int pmic_ldo_vbif28_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vbif28 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vbif28 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vbif28 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vbif28 is_enabled */
static int pmic_ldo_vbif28_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vbif28 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vbif28 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcama2 enable */
static int pmic_ldo_vcama2_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama2 enable\n");
	if (mreg->en_cb != NULL) {
		ret = (mreg->en_cb)(1);
#if ENABLE_ALL_OC_IRQ
		mdelay(3);
		/* this OC interrupt needs to delay 1ms after enable power */
		pmic_enable_interrupt(INT_VCAMA2_OC, 1, "PMIC");
#endif
	} else {
		pr_err("ldo vcama2 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcama2 disable */
static int pmic_ldo_vcama2_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama2 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcama2 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL) {
			ret = (mreg->en_cb)(0);
#if ENABLE_ALL_OC_IRQ
			/* after disable power, this OC interrupt should be disabled as well */
			pmic_enable_interrupt(INT_VCAMA2_OC, 0, "PMIC");
#endif
		} else {
			pr_err("ldo vcama2 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcama2 is_enabled */
static int pmic_ldo_vcama2_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama2 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcama2 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcama2 set_voltage_sel */
static int pmic_ldo_vcama2_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vcama2 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vcama2 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcama2 get_voltage_sel */
static int pmic_ldo_vcama2_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcama2 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vcama2 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vcama2 list_voltage */
static int pmic_ldo_vcama2_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vcama2_voltages[selector];
	PMICLOG("ldo vcama2 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vmc enable */
static int pmic_ldo_vmc_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmc enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vmc don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmc disable */
static int pmic_ldo_vmc_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmc disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vmc should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vmc don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vmc is_enabled */
static int pmic_ldo_vmc_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmc is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vmc don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vmc set_voltage_sel */
static int pmic_ldo_vmc_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vmc set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vmc don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vmc get_voltage_sel */
static int pmic_ldo_vmc_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vmc get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vmc don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vmc list_voltage */
static int pmic_ldo_vmc_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vmc_voltages[selector];
	PMICLOG("ldo vmc list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vdram enable */
static int pmic_buck_vdram_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vdram enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("buck vdram don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vdram disable */
static int pmic_buck_vdram_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vdram disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("buck vdram should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("buck vdram don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vdram is_enabled */
static int pmic_buck_vdram_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vdram is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("buck vdram don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vdram set_voltage_sel */
static int pmic_buck_vdram_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("buck vdram set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("buck vdram don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vdram get_voltage_sel */
static int pmic_buck_vdram_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("buck vdram get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("buck vdram don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vxo22 enable */
static int pmic_ldo_vxo22_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vxo22 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vxo22 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vxo22 disable */
static int pmic_ldo_vxo22_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vxo22 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vxo22 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vxo22 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vxo22 is_enabled */
static int pmic_ldo_vxo22_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vxo22 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vxo22 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsim2 enable */
static int pmic_ldo_vsim2_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim2 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vsim2 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsim2 disable */
static int pmic_ldo_vsim2_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim2 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vsim2 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vsim2 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vsim2 is_enabled */
static int pmic_ldo_vsim2_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim2 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vsim2 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vsim2 set_voltage_sel */
static int pmic_ldo_vsim2_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	mreg->vosel.cur_sel = selector;

	PMICLOG("ldo vsim2 set_voltage_sel: %d\n", selector);
	if (mreg->vol_cb != NULL)
		ret = (mreg->vol_cb)(selector);
	else {
		pr_err("ldo vsim2 don't have vol_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vsim2 get_voltage_sel */
static int pmic_ldo_vsim2_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vsim2 get_voltage_sel\n");
	if (mreg->da_vol_cb == NULL) {
		pr_err("ldo vsim2 don't have da_vol_cb\n");
		return -1;
	}

	return (mreg->da_vol_cb)();
}

/* Regulator vsim2 list_voltage */
static int pmic_ldo_vsim2_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int voltage;

	voltage = vsim2_voltages[selector];
	PMICLOG("ldo vsim2 list_voltage: %d\n", voltage);
	return voltage;
}

/* Regulator vio18 enable */
static int pmic_ldo_vio18_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vio18 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vio18 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vio18 disable */
static int pmic_ldo_vio18_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vio18 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vio18 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vio18 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vio18 is_enabled */
static int pmic_ldo_vio18_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vio18 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vio18 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vufs18 enable */
static int pmic_ldo_vufs18_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vufs18 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vufs18 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vufs18 disable */
static int pmic_ldo_vufs18_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vufs18 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vufs18 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vufs18 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vufs18 is_enabled */
static int pmic_ldo_vufs18_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vufs18 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vufs18 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vusb33 enable */
static int pmic_ldo_vusb33_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vusb33 enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vusb33 don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vusb33 disable */
static int pmic_ldo_vusb33_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vusb33 disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vusb33 should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vusb33 don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vusb33 is_enabled */
static int pmic_ldo_vusb33_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vusb33 is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vusb33 don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vcamaf enable */
static int pmic_ldo_vcamaf_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamaf enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vcamaf don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vcamaf disable */
static int pmic_ldo_vcamaf_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamaf disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vcamaf should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vcamaf don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vcamaf is_enabled */
static int pmic_ldo_vcamaf_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vcamaf is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vcamaf don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}

/* Regulator vtouch enable */
static int pmic_ldo_vtouch_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vtouch enable\n");
	if (mreg->en_cb != NULL)
		ret = (mreg->en_cb)(1);
	else {
		pr_err("ldo vtouch don't have en_cb\n");
		ret = -1;
	}

	return ret;
}

/* Regulator vtouch disable */
static int pmic_ldo_vtouch_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int ret = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vtouch disable\n");
	if (rdev->use_count == 0) {
		PMICLOG("ldo vtouch should not be disable (use_count=%d)\n", rdev->use_count);
		ret = -1;
	} else {
		if (mreg->en_cb != NULL)
			ret = (mreg->en_cb)(0);
		else {
			pr_err("ldo vtouch don't have enable callback\n");
			ret = -1;
		}
	}

	return ret;
}

/* Regulator vtouch is_enabled */
static int pmic_ldo_vtouch_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	PMICLOG("ldo vtouch is_enabled\n");

	if (mreg->da_en_cb == NULL) {
		pr_err("ldo vtouch don't have da_en_cb\n");
		return -1;
	}

	return (mreg->da_en_cb)();
}



/* Regulator vgp3 ops */
static struct regulator_ops pmic_ldo_vgp3_ops = {
	.enable = pmic_ldo_vgp3_enable,
	.disable = pmic_ldo_vgp3_disable,
	.is_enabled = pmic_ldo_vgp3_is_enabled,
	.get_voltage_sel = pmic_ldo_vgp3_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vgp3_set_voltage_sel,
	.list_voltage = pmic_ldo_vgp3_list_voltage,
	/* .enable_time = pmic_ldo_vgp3_enable_time, */
};

/* Regulator vcamd1 ops */
static struct regulator_ops pmic_ldo_vcamd1_ops = {
	.enable = pmic_ldo_vcamd1_enable,
	.disable = pmic_ldo_vcamd1_disable,
	.is_enabled = pmic_ldo_vcamd1_is_enabled,
	.get_voltage_sel = pmic_ldo_vcamd1_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vcamd1_set_voltage_sel,
	.list_voltage = pmic_ldo_vcamd1_list_voltage,
	/* .enable_time = pmic_ldo_vcamd1_enable_time, */
};

/* Regulator vsim1 ops */
static struct regulator_ops pmic_ldo_vsim1_ops = {
	.enable = pmic_ldo_vsim1_enable,
	.disable = pmic_ldo_vsim1_disable,
	.is_enabled = pmic_ldo_vsim1_is_enabled,
	.get_voltage_sel = pmic_ldo_vsim1_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vsim1_set_voltage_sel,
	.list_voltage = pmic_ldo_vsim1_list_voltage,
	/* .enable_time = pmic_ldo_vsim1_enable_time, */
};

/* Regulator vs1 ops */
static struct regulator_ops pmic_buck_vs1_ops = {
	.enable = pmic_buck_vs1_enable,
	.disable = pmic_buck_vs1_disable,
	.is_enabled = pmic_buck_vs1_is_enabled,
	.get_voltage_sel = pmic_buck_vs1_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vs1_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vs1_enable_time, */
};

/* Regulator vibr ops */
static struct regulator_ops pmic_ldo_vibr_ops = {
	.enable = pmic_ldo_vibr_enable,
	.disable = pmic_ldo_vibr_disable,
	.is_enabled = pmic_ldo_vibr_is_enabled,
	.get_voltage_sel = pmic_ldo_vibr_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vibr_set_voltage_sel,
	.list_voltage = pmic_ldo_vibr_list_voltage,
	/* .enable_time = pmic_ldo_vibr_enable_time, */
};

/* Regulator vrf12 ops */
static struct regulator_ops pmic_ldo_vrf12_ops = {
	.enable = pmic_ldo_vrf12_enable,
	.disable = pmic_ldo_vrf12_disable,
	.is_enabled = pmic_ldo_vrf12_is_enabled,
	/* .enable_time = pmic_ldo_vrf12_enable_time, */
};

/* Regulator vimvo ops */
static struct regulator_ops pmic_buck_vimvo_ops = {
	.enable = pmic_buck_vimvo_enable,
	.disable = pmic_buck_vimvo_disable,
	.is_enabled = pmic_buck_vimvo_is_enabled,
	.get_voltage_sel = pmic_buck_vimvo_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vimvo_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vimvo_enable_time, */
};

/* Regulator vsram_vcore ops */
static struct regulator_ops pmic_ldo_vsram_vcore_ops = {
	.enable = pmic_ldo_vsram_vcore_enable,
	.disable = pmic_ldo_vsram_vcore_disable,
	.is_enabled = pmic_ldo_vsram_vcore_is_enabled,
	.get_voltage_sel = pmic_ldo_vsram_vcore_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vsram_vcore_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_ldo_vsram_vcore_enable_time, */
};

/* Regulator vcamio ops */
static struct regulator_ops pmic_ldo_vcamio_ops = {
	.enable = pmic_ldo_vcamio_enable,
	.disable = pmic_ldo_vcamio_disable,
	.is_enabled = pmic_ldo_vcamio_is_enabled,
	/* .enable_time = pmic_ldo_vcamio_enable_time, */
};

/* Regulator va18 ops */
static struct regulator_ops pmic_ldo_va18_ops = {
	.enable = pmic_ldo_va18_enable,
	.disable = pmic_ldo_va18_disable,
	.is_enabled = pmic_ldo_va18_is_enabled,
	/* .enable_time = pmic_ldo_va18_enable_time, */
};

/* Regulator vsram_vmd ops */
static struct regulator_ops pmic_ldo_vsram_vmd_ops = {
	.enable = pmic_ldo_vsram_vmd_enable,
	.disable = pmic_ldo_vsram_vmd_disable,
	.is_enabled = pmic_ldo_vsram_vmd_is_enabled,
	.get_voltage_sel = pmic_ldo_vsram_vmd_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vsram_vmd_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_ldo_vsram_vmd_enable_time, */
};

/* Regulator vcn18_bt ops */
static struct regulator_ops pmic_ldo_vcn18_bt_ops = {
	.enable = pmic_ldo_vcn18_bt_enable,
	.disable = pmic_ldo_vcn18_bt_disable,
	.is_enabled = pmic_ldo_vcn18_bt_is_enabled,
	/* .enable_time = pmic_ldo_vcn18_bt_enable_time, */
};

/* Regulator vcn18_wifi ops */
static struct regulator_ops pmic_ldo_vcn18_wifi_ops = {
	.enable = pmic_ldo_vcn18_wifi_enable,
	.disable = pmic_ldo_vcn18_wifi_disable,
	.is_enabled = pmic_ldo_vcn18_wifi_is_enabled,
	/* .enable_time = pmic_ldo_vcn18_wifi_enable_time, */
};

/* Regulator vfe28 ops */
static struct regulator_ops pmic_ldo_vfe28_ops = {
	.enable = pmic_ldo_vfe28_enable,
	.disable = pmic_ldo_vfe28_disable,
	.is_enabled = pmic_ldo_vfe28_is_enabled,
	/* .enable_time = pmic_ldo_vfe28_enable_time, */
};

/* Regulator vrf18_2 ops */
static struct regulator_ops pmic_ldo_vrf18_2_ops = {
	.enable = pmic_ldo_vrf18_2_enable,
	.disable = pmic_ldo_vrf18_2_disable,
	.is_enabled = pmic_ldo_vrf18_2_is_enabled,
	/* .enable_time = pmic_ldo_vrf18_2_enable_time, */
};

/* Regulator vmodem ops */
static struct regulator_ops pmic_buck_vmodem_ops = {
	.enable = pmic_buck_vmodem_enable,
	.disable = pmic_buck_vmodem_disable,
	.is_enabled = pmic_buck_vmodem_is_enabled,
	.get_voltage_sel = pmic_buck_vmodem_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vmodem_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vmodem_enable_time, */
};

/* Regulator vcore ops */
static struct regulator_ops pmic_buck_vcore_ops = {
	.enable = pmic_buck_vcore_enable,
	.disable = pmic_buck_vcore_disable,
	.is_enabled = pmic_buck_vcore_is_enabled,
	.get_voltage_sel = pmic_buck_vcore_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vcore_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vcore_enable_time, */
};

/* Regulator vmipi ops */
static struct regulator_ops pmic_ldo_vmipi_ops = {
	.enable = pmic_ldo_vmipi_enable,
	.disable = pmic_ldo_vmipi_disable,
	.is_enabled = pmic_ldo_vmipi_is_enabled,
	/* .enable_time = pmic_ldo_vmipi_enable_time, */
};

/* Regulator vcn28 ops */
static struct regulator_ops pmic_ldo_vcn28_ops = {
	.enable = pmic_ldo_vcn28_enable,
	.disable = pmic_ldo_vcn28_disable,
	.is_enabled = pmic_ldo_vcn28_is_enabled,
	/* .enable_time = pmic_ldo_vcn28_enable_time, */
};

/* Regulator vsram_dvfs2 ops */
static struct regulator_ops pmic_ldo_vsram_dvfs2_ops = {
	.enable = pmic_ldo_vsram_dvfs2_enable,
	.disable = pmic_ldo_vsram_dvfs2_disable,
	.is_enabled = pmic_ldo_vsram_dvfs2_is_enabled,
	.get_voltage_sel = pmic_ldo_vsram_dvfs2_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vsram_dvfs2_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_ldo_vsram_dvfs2_enable_time, */
};

/* Regulator vs2 ops */
static struct regulator_ops pmic_buck_vs2_ops = {
	.enable = pmic_buck_vs2_enable,
	.disable = pmic_buck_vs2_disable,
	.is_enabled = pmic_buck_vs2_is_enabled,
	.get_voltage_sel = pmic_buck_vs2_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vs2_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vs2_enable_time, */
};

/* Regulator vefuse ops */
static struct regulator_ops pmic_ldo_vefuse_ops = {
	.enable = pmic_ldo_vefuse_enable,
	.disable = pmic_ldo_vefuse_disable,
	.is_enabled = pmic_ldo_vefuse_is_enabled,
	.get_voltage_sel = pmic_ldo_vefuse_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vefuse_set_voltage_sel,
	.list_voltage = pmic_ldo_vefuse_list_voltage,
	/* .enable_time = pmic_ldo_vefuse_enable_time, */
};

/* Regulator vpa1 ops */
static struct regulator_ops pmic_buck_vpa1_ops = {
	.enable = pmic_buck_vpa1_enable,
	.disable = pmic_buck_vpa1_disable,
	.is_enabled = pmic_buck_vpa1_is_enabled,
	.get_voltage_sel = pmic_buck_vpa1_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vpa1_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vpa1_enable_time, */
};

/* Regulator vrf18_1 ops */
static struct regulator_ops pmic_ldo_vrf18_1_ops = {
	.enable = pmic_ldo_vrf18_1_enable,
	.disable = pmic_ldo_vrf18_1_disable,
	.is_enabled = pmic_ldo_vrf18_1_is_enabled,
	/* .enable_time = pmic_ldo_vrf18_1_enable_time, */
};

/* Regulator vsram_vgpu ops */
static struct regulator_ops pmic_ldo_vsram_vgpu_ops = {
	.enable = pmic_ldo_vsram_vgpu_enable,
	.disable = pmic_ldo_vsram_vgpu_disable,
	.is_enabled = pmic_ldo_vsram_vgpu_is_enabled,
	.get_voltage_sel = pmic_ldo_vsram_vgpu_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vsram_vgpu_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_ldo_vsram_vgpu_enable_time, */
};

/* Regulator vcamd2 ops */
static struct regulator_ops pmic_ldo_vcamd2_ops = {
	.enable = pmic_ldo_vcamd2_enable,
	.disable = pmic_ldo_vcamd2_disable,
	.is_enabled = pmic_ldo_vcamd2_is_enabled,
	.get_voltage_sel = pmic_ldo_vcamd2_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vcamd2_set_voltage_sel,
	.list_voltage = pmic_ldo_vcamd2_list_voltage,
	/* .enable_time = pmic_ldo_vcamd2_enable_time, */
};

/* Regulator vmch ops */
static struct regulator_ops pmic_ldo_vmch_ops = {
	.enable = pmic_ldo_vmch_enable,
	.disable = pmic_ldo_vmch_disable,
	.is_enabled = pmic_ldo_vmch_is_enabled,
	.get_voltage_sel = pmic_ldo_vmch_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vmch_set_voltage_sel,
	.list_voltage = pmic_ldo_vmch_list_voltage,
	/* .enable_time = pmic_ldo_vmch_enable_time, */
};

/* Regulator vcama1 ops */
static struct regulator_ops pmic_ldo_vcama1_ops = {
	.enable = pmic_ldo_vcama1_enable,
	.disable = pmic_ldo_vcama1_disable,
	.is_enabled = pmic_ldo_vcama1_is_enabled,
	.get_voltage_sel = pmic_ldo_vcama1_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vcama1_set_voltage_sel,
	.list_voltage = pmic_ldo_vcama1_list_voltage,
	/* .enable_time = pmic_ldo_vcama1_enable_time, */
};

/* Regulator vemc ops */
static struct regulator_ops pmic_ldo_vemc_ops = {
	.enable = pmic_ldo_vemc_enable,
	.disable = pmic_ldo_vemc_disable,
	.is_enabled = pmic_ldo_vemc_is_enabled,
	.get_voltage_sel = pmic_ldo_vemc_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vemc_set_voltage_sel,
	.list_voltage = pmic_ldo_vemc_list_voltage,
	/* .enable_time = pmic_ldo_vemc_enable_time, */
};

/* Regulator vio28 ops */
static struct regulator_ops pmic_ldo_vio28_ops = {
	.enable = pmic_ldo_vio28_enable,
	.disable = pmic_ldo_vio28_disable,
	.is_enabled = pmic_ldo_vio28_is_enabled,
	/* .enable_time = pmic_ldo_vio28_enable_time, */
};

/* Regulator vmd1 ops */
static struct regulator_ops pmic_buck_vmd1_ops = {
	.enable = pmic_buck_vmd1_enable,
	.disable = pmic_buck_vmd1_disable,
	.is_enabled = pmic_buck_vmd1_is_enabled,
	.get_voltage_sel = pmic_buck_vmd1_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vmd1_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vmd1_enable_time, */
};

/* Regulator va12 ops */
static struct regulator_ops pmic_ldo_va12_ops = {
	.enable = pmic_ldo_va12_enable,
	.disable = pmic_ldo_va12_disable,
	.is_enabled = pmic_ldo_va12_is_enabled,
	/* .enable_time = pmic_ldo_va12_enable_time, */
};

/* Regulator va10 ops */
static struct regulator_ops pmic_ldo_va10_ops = {
	.enable = pmic_ldo_va10_enable,
	.disable = pmic_ldo_va10_disable,
	.is_enabled = pmic_ldo_va10_is_enabled,
	.get_voltage_sel = pmic_ldo_va10_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_va10_set_voltage_sel,
	.list_voltage = pmic_ldo_va10_list_voltage,
	/* .enable_time = pmic_ldo_va10_enable_time, */
};

/* Regulator vsram_dvfs1 ops */
static struct regulator_ops pmic_ldo_vsram_dvfs1_ops = {
	.enable = pmic_ldo_vsram_dvfs1_enable,
	.disable = pmic_ldo_vsram_dvfs1_disable,
	.is_enabled = pmic_ldo_vsram_dvfs1_is_enabled,
	.get_voltage_sel = pmic_ldo_vsram_dvfs1_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vsram_dvfs1_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_ldo_vsram_dvfs1_enable_time, */
};

/* Regulator vcn33_bt ops */
static struct regulator_ops pmic_ldo_vcn33_bt_ops = {
	.enable = pmic_ldo_vcn33_bt_enable,
	.disable = pmic_ldo_vcn33_bt_disable,
	.is_enabled = pmic_ldo_vcn33_bt_is_enabled,
	.get_voltage_sel = pmic_ldo_vcn33_bt_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vcn33_bt_set_voltage_sel,
	.list_voltage = pmic_ldo_vcn33_bt_list_voltage,
	/* .enable_time = pmic_ldo_vcn33_bt_enable_time, */
};

/* Regulator vcn33_wifi ops */
static struct regulator_ops pmic_ldo_vcn33_wifi_ops = {
	.enable = pmic_ldo_vcn33_wifi_enable,
	.disable = pmic_ldo_vcn33_wifi_disable,
	.is_enabled = pmic_ldo_vcn33_wifi_is_enabled,
	.get_voltage_sel = pmic_ldo_vcn33_wifi_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vcn33_wifi_set_voltage_sel,
	.list_voltage = pmic_ldo_vcn33_wifi_list_voltage,
	/* .enable_time = pmic_ldo_vcn33_wifi_enable_time, */
};

/* Regulator vbif28 ops */
static struct regulator_ops pmic_ldo_vbif28_ops = {
	.enable = pmic_ldo_vbif28_enable,
	.disable = pmic_ldo_vbif28_disable,
	.is_enabled = pmic_ldo_vbif28_is_enabled,
	/* .enable_time = pmic_ldo_vbif28_enable_time, */
};

/* Regulator vcama2 ops */
static struct regulator_ops pmic_ldo_vcama2_ops = {
	.enable = pmic_ldo_vcama2_enable,
	.disable = pmic_ldo_vcama2_disable,
	.is_enabled = pmic_ldo_vcama2_is_enabled,
	.get_voltage_sel = pmic_ldo_vcama2_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vcama2_set_voltage_sel,
	.list_voltage = pmic_ldo_vcama2_list_voltage,
	/* .enable_time = pmic_ldo_vcama2_enable_time, */
};

/* Regulator vmc ops */
static struct regulator_ops pmic_ldo_vmc_ops = {
	.enable = pmic_ldo_vmc_enable,
	.disable = pmic_ldo_vmc_disable,
	.is_enabled = pmic_ldo_vmc_is_enabled,
	.get_voltage_sel = pmic_ldo_vmc_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vmc_set_voltage_sel,
	.list_voltage = pmic_ldo_vmc_list_voltage,
	/* .enable_time = pmic_ldo_vmc_enable_time, */
};

/* Regulator vdram ops */
static struct regulator_ops pmic_buck_vdram_ops = {
	.enable = pmic_buck_vdram_enable,
	.disable = pmic_buck_vdram_disable,
	.is_enabled = pmic_buck_vdram_is_enabled,
	.get_voltage_sel = pmic_buck_vdram_get_voltage_sel,
	.set_voltage_sel = pmic_buck_vdram_set_voltage_sel,
	.list_voltage = regulator_list_voltage_linear,
	/* .enable_time = pmic_buck_vdram_enable_time, */
};

/* Regulator vxo22 ops */
static struct regulator_ops pmic_ldo_vxo22_ops = {
	.enable = pmic_ldo_vxo22_enable,
	.disable = pmic_ldo_vxo22_disable,
	.is_enabled = pmic_ldo_vxo22_is_enabled,
	/* .enable_time = pmic_ldo_vxo22_enable_time, */
};

/* Regulator vsim2 ops */
static struct regulator_ops pmic_ldo_vsim2_ops = {
	.enable = pmic_ldo_vsim2_enable,
	.disable = pmic_ldo_vsim2_disable,
	.is_enabled = pmic_ldo_vsim2_is_enabled,
	.get_voltage_sel = pmic_ldo_vsim2_get_voltage_sel,
	.set_voltage_sel = pmic_ldo_vsim2_set_voltage_sel,
	.list_voltage = pmic_ldo_vsim2_list_voltage,
	/* .enable_time = pmic_ldo_vsim2_enable_time, */
};

/* Regulator vio18 ops */
static struct regulator_ops pmic_ldo_vio18_ops = {
	.enable = pmic_ldo_vio18_enable,
	.disable = pmic_ldo_vio18_disable,
	.is_enabled = pmic_ldo_vio18_is_enabled,
	/* .enable_time = pmic_ldo_vio18_enable_time, */
};

/* Regulator vufs18 ops */
static struct regulator_ops pmic_ldo_vufs18_ops = {
	.enable = pmic_ldo_vufs18_enable,
	.disable = pmic_ldo_vufs18_disable,
	.is_enabled = pmic_ldo_vufs18_is_enabled,
	/* .enable_time = pmic_ldo_vufs18_enable_time, */
};

/* Regulator vusb33 ops */
static struct regulator_ops pmic_ldo_vusb33_ops = {
	.enable = pmic_ldo_vusb33_enable,
	.disable = pmic_ldo_vusb33_disable,
	.is_enabled = pmic_ldo_vusb33_is_enabled,
	/* .enable_time = pmic_ldo_vusb33_enable_time, */
};

/* Regulator vcamaf ops */
static struct regulator_ops pmic_ldo_vcamaf_ops = {
	.enable = pmic_ldo_vcamaf_enable,
	.disable = pmic_ldo_vcamaf_disable,
	.is_enabled = pmic_ldo_vcamaf_is_enabled,
	/* .enable_time = pmic_ldo_vcamaf_enable_time, */
};

/* Regulator vtouch ops */
static struct regulator_ops pmic_ldo_vtouch_ops = {
	.enable = pmic_ldo_vtouch_enable,
	.disable = pmic_ldo_vtouch_disable,
	.is_enabled = pmic_ldo_vtouch_is_enabled,
	/* .enable_time = pmic_ldo_vtouch_enable_time, */
};



/*------Regulator ATTR------*/
static ssize_t show_regulator_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_regulator *mreg;
	unsigned int ret_value = 0;

	mreg = container_of(attr, struct mtk_regulator, en_att);

	if (mreg->da_en_cb != NULL)
		ret_value = (mreg->da_en_cb)();
	else
		ret_value = 9999;

	PMICLOG("[EM] %s_STATUS : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_regulator_status(struct device *dev, struct device_attribute *attr, const char *buf,
				size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");
	return size;
}

static ssize_t show_regulator_voltage(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_regulator *mreg;
	const int *pVoltage;

	unsigned short regVal;
	unsigned int ret_value = 0;

	mreg = container_of(attr, struct mtk_regulator, voltage_att);

	if (mreg->desc.n_voltages != 1) {
		if (mreg->da_vol_cb != NULL) {
			regVal = (mreg->da_vol_cb)();
			if (mreg->pvoltages != NULL) {
				pVoltage = (const int *)mreg->pvoltages;
				ret_value = pVoltage[regVal];
			} else
				ret_value = mreg->desc.min_uV + mreg->desc.uV_step * regVal;
		} else
			pr_err("[EM] %s_VOLTAGE have no da_vol_cb\n", mreg->desc.name);
	} else {
		if (mreg->pvoltages != NULL) {
			pVoltage = (const int *)mreg->pvoltages;
			ret_value = pVoltage[0];
		} else if (mreg->desc.fixed_uV)
			ret_value = mreg->desc.fixed_uV;
		else
			pr_err("[EM] %s_VOLTAGE have no pVolatges\n", mreg->desc.name);
	}

	ret_value = ret_value / 1000;

	PMICLOG("[EM] %s_VOLTAGE : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_regulator_voltage(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");
	return size;
}


/* Regulator: BUCK */
#define BUCK_EN	REGULATOR_CHANGE_STATUS
#define BUCK_VOL REGULATOR_CHANGE_VOLTAGE
#define BUCK_VOL_EN (REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_VOLTAGE)
struct mtk_regulator mt_bucks[] = {
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vs1, buck, 1200000, 2787500, 12500, 0, BUCK_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vimvo, buck, 400000, 1193750, 6250, 0, BUCK_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vmodem, buck, 400000, 1193750, 6250, 0, BUCK_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vcore, buck, 400000, 1193750, 6250, 0, BUCK_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vs2, buck, 1200000, 2787500, 12500, 0, BUCK_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vpa1, buck, 500000, 3650000, 50000, 0, BUCK_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vmd1, buck, 400000, 1193750, 6250, 0, BUCK_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_BUCK_GEN(vdram, buck, 600000, 1393750, 6250, 0, BUCK_VOL_EN, 1),

};


/* Regulator: LDO */
#define LDO_EN	REGULATOR_CHANGE_STATUS
#define LDO_VOL REGULATOR_CHANGE_VOLTAGE
#define LDO_VOL_EN (REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_VOLTAGE)
struct mtk_regulator mt_ldos[] = {
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vgp3, ldo, vgp3_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vcamd1, ldo, vcamd1_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vsim1, ldo, vsim1_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vibr, ldo, vibr_voltages, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vrf12, ldo, 1200000, LDO_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_LDO_GEN(vsram_vcore, ldo, 400000, 1193750, 6250, 0, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vcamio, ldo, 1800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(va18, ldo, 1800000, LDO_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_LDO_GEN(vsram_vmd, ldo, 400000, 1193750, 6250, 0, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vcn18_bt, ldo, 1800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vcn18_wifi, ldo, 1800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vfe28, ldo, 2800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vrf18_2, ldo, 1810000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vmipi, ldo, 1800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vcn28, ldo, 2800000, LDO_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_LDO_GEN(vsram_dvfs2, ldo, 400000, 1193750, 6250, 0, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vefuse, ldo, vefuse_voltages, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vrf18_1, ldo, 1810000, LDO_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_LDO_GEN(vsram_vgpu, ldo, 400000, 1193750, 6250, 0, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vcamd2, ldo, vcamd2_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vmch, ldo, vmch_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vcama1, ldo, vcama1_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vemc, ldo, vemc_voltages, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vio28, ldo, 2800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(va12, ldo, 1200000, LDO_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(va10, ldo, va10_voltages, LDO_VOL_EN, 1),
	REGULAR_VOLTAGE_REGULATOR_LDO_GEN(vsram_dvfs1, ldo, 400000, 1193750, 6250, 0, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vcn33_bt, ldo, vcn33_bt_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vcn33_wifi, ldo, vcn33_wifi_voltages, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vbif28, ldo, 2800000, LDO_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vcama2, ldo, vcama2_voltages, LDO_VOL_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vmc, ldo, vmc_voltages, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vxo22, ldo, 2200000, LDO_EN, 1),
	NON_REGULAR_VOLTAGE_REGULATOR_GEN(vsim2, ldo, vsim2_voltages, LDO_VOL_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vio18, ldo, 1800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vufs18, ldo, 1800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vusb33, ldo, 3300000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vcamaf, ldo, 2800000, LDO_EN, 1),
	FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(vtouch, ldo, 2800000, LDO_EN, 1),

};



int mt_ldos_size = ARRAY_SIZE(mt_ldos);
int mt_bucks_size = ARRAY_SIZE(mt_bucks);
/* -------Code Gen End-------*/

#ifdef CONFIG_OF
#if !defined CONFIG_MTK_LEGACY

#define PMIC_REGULATOR_LDO_OF_MATCH(_name, _id)			\
	{						\
		.name = #_name,						\
		.driver_data = &mt_ldos[MT6335_POWER_LDO_##_id],	\
	}

#define PMIC_REGULATOR_BUCK_OF_MATCH(_name, _id)			\
	{						\
		.name = #_name,						\
		.driver_data = &mt_bucks[MT6335_POWER_BUCK_##_id],	\
	}

struct of_regulator_match pmic_regulator_buck_matches[] = {
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vs1, VS1),
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vimvo, VIMVO),
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vmodem, VMODEM),
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vcore, VCORE),
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vs2, VS2),
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vpa1, VPA1),
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vmd1, VMD1),
	PMIC_REGULATOR_BUCK_OF_MATCH(buck_vdram, VDRAM),

};

struct of_regulator_match pmic_regulator_ldo_matches[] = {
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vgp3, VGP3),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcamd1, VCAMD1),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vsim1, VSIM1),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vibr, VIBR),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vrf12, VRF12),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vsram_vcore, VSRAM_VCORE),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcamio, VCAMIO),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_va18, VA18),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vsram_vmd, VSRAM_VMD),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcn18_bt, VCN18_BT),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcn18_wifi, VCN18_WIFI),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vfe28, VFE28),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vrf18_2, VRF18_2),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vmipi, VMIPI),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcn28, VCN28),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vsram_dvfs2, VSRAM_DVFS2),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vefuse, VEFUSE),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vrf18_1, VRF18_1),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vsram_vgpu, VSRAM_VGPU),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcamd2, VCAMD2),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vmch, VMCH),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcama1, VCAMA1),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vemc, VEMC),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vio28, VIO28),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_va12, VA12),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_va10, VA10),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vsram_dvfs1, VSRAM_DVFS1),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcn33_bt, VCN33_BT),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcn33_wifi, VCN33_WIFI),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vbif28, VBIF28),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcama2, VCAMA2),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vmc, VMC),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vxo22, VXO22),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vsim2, VSIM2),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vio18, VIO18),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vufs18, VUFS18),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vusb33, VUSB33),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vcamaf, VCAMAF),
	PMIC_REGULATOR_LDO_OF_MATCH(ldo_vtouch, VTOUCH),

};

int pmic_regulator_ldo_matches_size = ARRAY_SIZE(pmic_regulator_ldo_matches);
int pmic_regulator_buck_matches_size = ARRAY_SIZE(pmic_regulator_buck_matches);

#endif				/* End of #ifdef CONFIG_OF */
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */
