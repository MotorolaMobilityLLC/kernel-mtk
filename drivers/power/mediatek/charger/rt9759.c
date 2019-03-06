/*
 * Copyright (C) 2019 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>

#include "mtk_charger_intf.h"
#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif /* CONFIG_RT_REGMAP */

/* Information */
#define RT9759_DRV_VERSION	"1.0.0_MTK"
#define RT9759_DEVID		0x08

/* Registers */
#define RT9759_REG_VBATOVP	0x00
#define RT9759_REG_VBATOVP_ALM	0x01
#define RT9759_REG_IBATOCP	0x02
#define RT9759_REG_IBATOCP_ALM	0x03
#define RT9759_REG_IBATUCP_ALM	0x04
#define RT9759_REG_ACPROTECT	0x05
#define RT9759_REG_VBUSOVP	0x06
#define RT9759_REG_VBUSOVP_ALM	0x07
#define RT9759_REG_IBUSOCUCP	0x08
#define RT9759_REG_IBUSOCP_ALM	0x09
#define RT9759_REG_CONVSTAT	0x0A
#define RT9759_REG_CHGCTRL0	0x0B
#define RT9759_REG_CHGCTRL1	0x0C
#define RT9759_REG_INTSTAT	0x0D
#define RT9759_REG_INTFLAG	0x0E
#define RT9759_REG_INTMASK	0x0F
#define RT9759_REG_FLTSTAT	0x10
#define RT9759_REG_FLTFLAG	0x11
#define RT9759_REG_FLTMASK	0x12
#define RT9759_REG_DEVINFO	0x13
#define RT9759_REG_ADCCTRL	0x14
#define RT9759_REG_ADCEN	0x15
#define RT9759_REG_IBUSADC1	0x16
#define RT9759_REG_IBUSADC0	0x17
#define RT9759_REG_VBUSADC1	0x18
#define RT9759_REG_VBUSADC0	0x19
#define RT9759_REG_VACADC1	0x1A
#define RT9759_REG_VACADC0	0x1B
#define RT9759_REG_VOUTADC1	0x1C
#define RT9759_REG_VOUTADC0	0x1D
#define RT9759_REG_VBATADC1	0x1E
#define RT9759_REG_VBATADC0	0x1F
#define RT9759_REG_IBATADC1	0x20
#define RT9759_REG_IBATADC0	0x21
#define RT9759_REG_TSBUSADC1	0x22
#define RT9759_REG_TSBUSADC0	0x23
#define RT9759_REG_TSBATADC1	0x24
#define RT9759_REG_TSBATADC0	0x25
#define RT9759_REG_TDIEADC1	0x26
#define RT9759_REG_TDIEADC0	0x27
#define RT9759_REG_TSBUSOTP	0x28
#define RT9759_REG_TSBATOTP	0x29
#define RT9759_REG_TDIEALM	0x2A
#define RT9759_REG_REGCTRL	0x2B
#define RT9759_REG_REGTHRES	0x2C
#define RT9759_REG_REGFLAGMASK	0x2D
#define RT9759_REG_BUSDEGLH	0x2E
#define RT9759_REG_OTHER1	0x30
#define RT9759_REG_SYSCTRL1	0x42
#define RT9759_REG_PASSWORD0	0x90
#define RT9759_REG_PASSWORD1	0x91

/* Control bits */
#define RT9759_CHGEN_MASK	BIT(7)
#define RT9759_CHGEN_SHFT	7
#define RT9759_ADCEN_MASK	BIT(7)
#define RT9759_VBUSOVP_MASK	0x7F
#define RT9759_IBUSOCP_MASK	0x0F
#define RT9759_VBATOVP_MASK	0x3F
#define RT9759_IBATOCP_MASK	0x7F

enum rt9759_irqidx {
	RT9759_IRQIDX_VACOVP = 0,
	RT9759_IRQIDX_IBUSUCPF,
	RT9759_IRQIDX_IBUSUCPR,
	RT9759_IRQIDX_CFLYDIAG,
	RT9759_IRQIDX_CONOCP,
	RT9759_IRQIDX_SWITCHING,
	RT9759_IRQIDX_IBUSUCPTOUT,
	RT9759_IRQIDX_VBUSHERR,
	RT9759_IRQIDX_VBUSLERR,
	RT9759_IRQIDX_TDIEOTP,
	RT9759_IRQIDX_WDT,
	RT9759_IRQIDX_ADCDONE,
	RT9759_IRQIDX_VOUTINSERT,
	RT9759_IRQIDX_VACINSERT,
	RT9759_IRQIDX_IBATUCPALM,
	RT9759_IRQIDX_IBUSOCPALM,
	RT9759_IRQIDX_VBUSOVPALM,
	RT9759_IRQIDX_IBATOCPALM,
	RT9759_IRQIDX_VBATOVPALM,
	RT9759_IRQIDX_TDIEOTPALM,
	RT9759_IRQIDX_TSBUSOTP,
	RT9759_IRQIDX_TSBATOTP,
	RT9759_IRQIDX_TSBUSBATOTPALM,
	RT9759_IRQIDX_IBUSOCP,
	RT9759_IRQIDX_VBUSOVP,
	RT9759_IRQIDX_IBATOCP,
	RT9759_IRQIDX_VBATOVP,
	RT9759_IRQIDX_VOUTOVP,
	RT9759_IRQIDX_VDROVP,
	RT9759_IRQIDX_IBATREG,
	RT9759_IRQIDX_VBATREG,
	RT9759_IRQIDX_MAX,
};

enum rt9759_statflag_idx {
	RT9759_SF_ACPROTECT = 0,
	RT9759_SF_IBUSOCUCP,
	RT9759_SF_CONVSTAT,
	RT9759_SF_CHGCTRL0,
	RT9759_SF_INTFLAG,
	RT9759_SF_INTSTAT,
	RT9759_SF_FLTFLAG,
	RT9759_SF_FLTSTAT,
	RT9759_SF_REGFLAGMASK,
	RT9759_SF_REGTHRES,
	RT9759_SF_MAX,
};

static const u8 rt9759_reg_sf[RT9759_SF_MAX] = {
	RT9759_REG_ACPROTECT,
	RT9759_REG_IBUSOCUCP,
	RT9759_REG_CONVSTAT,
	RT9759_REG_CHGCTRL0,
	RT9759_REG_INTFLAG,
	RT9759_REG_INTSTAT,
	RT9759_REG_FLTFLAG,
	RT9759_REG_FLTSTAT,
	RT9759_REG_REGFLAGMASK,
	RT9759_REG_REGTHRES,
};

struct rt9759_desc {
	const char *chg_name;
	const char *rm_name;
	u8 rm_slave_addr;
	u32 vbatovp;
	u32 vbatovp_alm;
	u32 ibatocp;
	u32 ibatocp_alm;
	u32 ibatucp_alm;
	u32 vbusovp;
	u32 vbusovp_alm;
	u32 ibusocp;
	u32 ibusocp_alm;
	u32 vacovp;
	u32 wdt;
	bool vbatovp_dis;
	bool vbatovp_alm_dis;
	bool ibatocp_dis;
	bool ibatocp_alm_dis;
	bool ibatucp_alm_dis;
	bool vbusovp_alm_dis;
	bool ibusocp_dis;
	bool ibusocp_alm_dis;
	bool wdt_dis;
	bool tsbusotp_dis;
	bool tsbatotp_dis;
	bool tdieotp_dis;
	bool reg_en;
	bool voutovp_dis;
};

static const struct rt9759_desc rt9759_desc_defval = {
	.chg_name = "divider_charger",
	.rm_name = "rt9759",
	.rm_slave_addr = 0x66,
	.vbatovp = 4350000,
	.vbatovp_alm = 4200000,
	.ibatocp = 8100000,
	.ibatocp_alm = 8000000,
	.ibatucp_alm = 2000000,
	.vbusovp = 8900000,
	.vbusovp_alm = 8800000,
	.ibusocp = 4250000,
	.ibusocp_alm = 4000000,
	.vacovp = 11000000,
	.wdt = 500000,
	.vbatovp_dis = false,
	.vbatovp_alm_dis = false,
	.ibatocp_dis = false,
	.ibatocp_alm_dis = false,
	.ibatucp_alm_dis = false,
	.vbusovp_alm_dis = false,
	.ibusocp_dis = false,
	.ibusocp_alm_dis = false,
	.wdt_dis = false,
	.tsbusotp_dis = false,
	.tsbatotp_dis = false,
	.tdieotp_dis = false,
	.reg_en = false,
	.voutovp_dis = false,
};

struct rt9759_chip {
	struct device *dev;
	struct i2c_client *client;
	struct mutex io_lock;
	struct mutex adc_lock;
	struct mutex stat_lock;
	struct mutex hm_lock;
	struct mutex suspend_lock;
	struct charger_device *chg_dev;
	struct charger_properties chg_prop;
	struct rt9759_desc *desc;
	struct gpio_desc *irq_gpio;
	int irq;
	u8 revision;
	u32 flag;
	u32 stat;
	u32 hm_cnt;

#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *rm_dev;
	struct rt_regmap_properties *rm_prop;
#endif /* CONFIG_RT_REGMAP */
};

enum rt9759_adc_channel {
	RT9759_ADC_IBUS = 0,
	RT9759_ADC_VBUS,
	RT9759_ADC_VAC,
	RT9759_ADC_VOUT,
	RT9759_ADC_VBAT,
	RT9759_ADC_IBAT,
	RT9759_ADC_TSBUS,
	RT9759_ADC_TSBAT,
	RT9759_ADC_TDIE,
	RT9759_ADC_MAX,
	RT9759_ADC_NOTSUPP = RT9759_ADC_MAX,
};

static const u8 rt9759_adc_reg[RT9759_ADC_MAX] = {
	RT9759_REG_IBUSADC1,
	RT9759_REG_VBUSADC1,
	RT9759_REG_VACADC1,
	RT9759_REG_VOUTADC1,
	RT9759_REG_VBATADC1,
	RT9759_REG_IBATADC1,
	RT9759_REG_TSBUSADC1,
	RT9759_REG_TSBATADC1,
	RT9759_REG_TDIEADC1,
};

static const char *rt9759_adc_name[RT9759_ADC_MAX] = {
	"Ibus", "Vbus", "VAC", "Vout", "Vbat", "Ibat", "TSBus", "TSBat", "TDie",
};

static int rt9759_read_device(void *client, u32 addr, int len, void *dst)
{
	int ret;
	struct i2c_client *i2c = (struct i2c_client *)client;
	struct rt9759_chip *chip = i2c_get_clientdata(i2c);

	pm_stay_awake(chip->dev);
	mutex_lock(&chip->suspend_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, addr, len, dst);
	mutex_unlock(&chip->suspend_lock);
	pm_relax(chip->dev);
	return ret;
}

static int rt9759_write_device(void *client, u32 addr, int len, const void *src)
{
	int ret;
	struct i2c_client *i2c = (struct i2c_client *)client;
	struct rt9759_chip *chip = i2c_get_clientdata(i2c);

	pm_stay_awake(chip->dev);
	mutex_lock(&chip->suspend_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, addr, len, src);
	mutex_unlock(&chip->suspend_lock);
	pm_relax(chip->dev);
	return ret;
}

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT9759_REG_VBATOVP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VBATOVP_ALM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBATOCP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBATOCP_ALM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBATUCP_ALM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_ACPROTECT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VBUSOVP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VBUSOVP_ALM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBUSOCUCP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBUSOCP_ALM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_CONVSTAT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_CHGCTRL0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_CHGCTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_INTSTAT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_INTFLAG, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_INTMASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_FLTSTAT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_FLTFLAG, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_FLTMASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_DEVINFO, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_ADCCTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_ADCEN, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBUSADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBUSADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VBUSADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VBUSADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VACADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VACADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VOUTADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VOUTADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VBATADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_VBATADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBATADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_IBATADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TSBUSADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TSBUSADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TSBATADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TSBATADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TDIEADC1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TDIEADC0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TSBUSOTP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TSBATOTP, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_TDIEALM, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_REGCTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_REGTHRES, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_REGFLAGMASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_BUSDEGLH, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_OTHER1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_SYSCTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_PASSWORD0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9759_REG_PASSWORD1, 1, RT_VOLATILE, {});

static const rt_register_map_t rt9759_regmap[] = {
	RT_REG(RT9759_REG_VBATOVP),
	RT_REG(RT9759_REG_VBATOVP_ALM),
	RT_REG(RT9759_REG_IBATOCP),
	RT_REG(RT9759_REG_IBATOCP_ALM),
	RT_REG(RT9759_REG_IBATUCP_ALM),
	RT_REG(RT9759_REG_ACPROTECT),
	RT_REG(RT9759_REG_VBUSOVP),
	RT_REG(RT9759_REG_VBUSOVP_ALM),
	RT_REG(RT9759_REG_IBUSOCUCP),
	RT_REG(RT9759_REG_IBUSOCP_ALM),
	RT_REG(RT9759_REG_CONVSTAT),
	RT_REG(RT9759_REG_CHGCTRL0),
	RT_REG(RT9759_REG_CHGCTRL1),
	RT_REG(RT9759_REG_INTSTAT),
	RT_REG(RT9759_REG_INTFLAG),
	RT_REG(RT9759_REG_INTMASK),
	RT_REG(RT9759_REG_FLTSTAT),
	RT_REG(RT9759_REG_FLTFLAG),
	RT_REG(RT9759_REG_FLTMASK),
	RT_REG(RT9759_REG_DEVINFO),
	RT_REG(RT9759_REG_ADCCTRL),
	RT_REG(RT9759_REG_ADCEN),
	RT_REG(RT9759_REG_IBUSADC1),
	RT_REG(RT9759_REG_IBUSADC0),
	RT_REG(RT9759_REG_VBUSADC1),
	RT_REG(RT9759_REG_VBUSADC0),
	RT_REG(RT9759_REG_VACADC1),
	RT_REG(RT9759_REG_VACADC0),
	RT_REG(RT9759_REG_VOUTADC1),
	RT_REG(RT9759_REG_VOUTADC0),
	RT_REG(RT9759_REG_VBATADC1),
	RT_REG(RT9759_REG_VBATADC0),
	RT_REG(RT9759_REG_IBATADC1),
	RT_REG(RT9759_REG_IBATADC0),
	RT_REG(RT9759_REG_TSBUSADC1),
	RT_REG(RT9759_REG_TSBUSADC0),
	RT_REG(RT9759_REG_TSBATADC1),
	RT_REG(RT9759_REG_TSBATADC0),
	RT_REG(RT9759_REG_TDIEADC1),
	RT_REG(RT9759_REG_TDIEADC0),
	RT_REG(RT9759_REG_TSBUSOTP),
	RT_REG(RT9759_REG_TSBATOTP),
	RT_REG(RT9759_REG_TDIEALM),
	RT_REG(RT9759_REG_REGCTRL),
	RT_REG(RT9759_REG_REGTHRES),
	RT_REG(RT9759_REG_REGFLAGMASK),
	RT_REG(RT9759_REG_BUSDEGLH),
	RT_REG(RT9759_REG_OTHER1),
	RT_REG(RT9759_REG_SYSCTRL1),
	RT_REG(RT9759_REG_PASSWORD0),
	RT_REG(RT9759_REG_PASSWORD1),
};

static struct rt_regmap_fops rt9759_rm_fops = {
	.read_device = rt9759_read_device,
	.write_device = rt9759_write_device,
};

static int rt9759_register_regmap(struct rt9759_chip *chip)
{
	struct i2c_client *client = chip->client;
	struct rt_regmap_properties *prop = NULL;

	dev_info(chip->dev, "%s\n", __func__);

	prop = devm_kzalloc(&client->dev, sizeof(*prop), GFP_KERNEL);
	if (!prop)
		return -ENOMEM;

	prop->name = chip->desc->rm_name;
	prop->aliases = chip->desc->rm_name;
	prop->register_num = ARRAY_SIZE(rt9759_regmap);
	prop->rm = rt9759_regmap;
	prop->rt_regmap_mode = RT_SINGLE_BYTE | RT_CACHE_DISABLE |
			       RT_IO_PASS_THROUGH;
	prop->io_log_en = 0;

	chip->rm_prop = prop;
	chip->rm_dev = rt_regmap_device_register_ex(chip->rm_prop,
						    &rt9759_rm_fops, chip->dev,
						    client,
						    chip->desc->rm_slave_addr,
						    chip);
	if (!chip->rm_dev) {
		dev_info(chip->dev, "%s register regmap dev fail\n", __func__);
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_RT_REGMAP */

#define I2C_ACCESS_MAX_RETRY	5
static inline int __rt9759_i2c_write8(struct rt9759_chip *chip, u8 reg, u8 data)
{
	int ret, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_write(chip->rm_dev, reg, 1, &data);
#else
		ret = rt9759_write_device(chip->client, reg, 1, &data);
#endif /* CONFIG_RT_REGMAP */
		retry++;
		if (ret < 0)
			usleep_range(10, 15);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0)
		dev_info(chip->dev, "%s I2CW[0x%02X] = 0x%02X fail\n", __func__,
			 reg, data);
	else
		dev_info(chip->dev, "%s I2CW[0x%02X] = 0x%02X\n", __func__,
			 reg, data);

	return ret;
}

static int rt9759_i2c_write8(struct rt9759_chip *chip, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&chip->io_lock);
	ret = __rt9759_i2c_write8(chip, reg, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}

static inline int __rt9759_i2c_read8(struct rt9759_chip *chip, u8 reg, u8 *data)
{
	int ret, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_read(chip->rm_dev, reg, 1, data);
#else
		ret = rt9759_read_device(chip->client, reg, 1, data);
#endif /* CONFIG_RT_REGMAP */
		retry++;
		if (ret < 0)
			usleep_range(10, 15);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0) {
		dev_info(chip->dev, "%s I2CR[0x%02X] fail\n", __func__, reg);
		return ret;
	}

	dev_info(chip->dev, "%s I2CR[0x%02X] = 0x%02X\n", __func__, reg, *data);
	return 0;
}

static int rt9759_i2c_read8(struct rt9759_chip *chip, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&chip->io_lock);
	ret = __rt9759_i2c_read8(chip, reg, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}

static inline int __rt9759_i2c_write_block(struct rt9759_chip *chip, u8 reg,
					   u32 len, const u8 *data)
{
	int ret;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(chip->rm_dev, reg, len, data);
#else
	ret = rt9759_write_device(chip->client, reg, len, data);
#endif /* CONFIG_RT_REGMAP */

	return ret;
}

static int rt9759_i2c_write_block(struct rt9759_chip *chip, u8 reg, u32 len,
				  const u8 *data)
{
	int ret;

	mutex_lock(&chip->io_lock);
	ret = __rt9759_i2c_write_block(chip, reg, len, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}

static inline int __rt9759_i2c_read_block(struct rt9759_chip *chip, u8 reg,
					  u32 len, u8 *data)
{
	int ret;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(chip->rm_dev, reg, len, data);
#else
	ret = rt9759_read_device(chip->client, reg, len, data);
#endif /* CONFIG_RT_REGMAP */

	return ret;
}

static int rt9759_i2c_read_block(struct rt9759_chip *chip, u8 reg, u32 len,
				 u8 *data)
{
	int ret;

	mutex_lock(&chip->io_lock);
	ret = __rt9759_i2c_read_block(chip, reg, len, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}

static int rt9759_i2c_test_bit(struct rt9759_chip *chip, u8 reg, u8 shft,
			       bool *one)
{
	int ret;
	u8 data;

	ret = rt9759_i2c_read8(chip, reg, &data);
	if (ret < 0) {
		*one = false;
		return ret;
	}

	*one = (data & (1 << shft)) ? true : false;
	return 0;
}

static int rt9759_i2c_update_bits(struct rt9759_chip *chip, u8 reg, u8 data,
				  u8 mask)
{
	int ret;
	u8 _data;

	mutex_lock(&chip->io_lock);
	ret = __rt9759_i2c_read8(chip, reg, &_data);
	if (ret < 0)
		goto out;
	_data &= ~mask;
	_data |= (data & mask);
	ret = __rt9759_i2c_write8(chip, reg, _data);
out:
	mutex_unlock(&chip->io_lock);
	return ret;
}

static inline int rt9759_set_bits(struct rt9759_chip *chip, u8 reg, u8 mask)
{
	return rt9759_i2c_update_bits(chip, reg, mask, mask);
}

static inline int rt9759_clr_bits(struct rt9759_chip *chip, u8 reg, u8 mask)
{
	return rt9759_i2c_update_bits(chip, reg, 0x00, mask);
}

static inline u8 rt9759_val_toreg(u32 min, u32 max, u32 step, u32 target)
{
	if (target <= min)
		return 0;

	if (target >= max)
		return (max - min) / step;

	return (target - min) / step;
}

static inline u8 rt9759_val_toreg_via_tbl(const u32 *tbl, int tbl_size,
					  u32 target)
{
	int i;

	if (target < tbl[0])
		return 0;

	for (i = 0; i < tbl_size - 1; i++) {
		if (target >= tbl[i] && target < tbl[i + 1])
			return i;
	}

	return tbl_size - 1;
}

static u8 rt9759_vbatovp_toreg(u32 uV)
{
	return rt9759_val_toreg(3500000, 5075000, 25000, uV);
}

static u8 rt9759_ibatocp_toreg(u32 uA)
{
	return rt9759_val_toreg(2000000, 10000000, 100000, uA);
}

static u8 rt9759_ibatucp_toreg(u32 uA)
{
	return rt9759_val_toreg(0, 6350000, 50000, uA);
}

static u8 rt9759_vbusovp_toreg(u32 uV)
{
	return rt9759_val_toreg(6000000, 12350000, 50000, uV);
}

static u8 rt9759_ibusocp_toreg(u32 uA)
{
	return rt9759_val_toreg(1000000, 3750000, 250000, uA);
}

static u8 rt9759_ibusocp_alm_toreg(u32 uA)
{
	return rt9759_val_toreg(0, 6350000, 50000, uA);
}

static const u32 rt9759_wdt[] = {
	500000, 1000000, 5000000, 3000000,
};

static u8 rt9759_wdt_toreg(u32 uS)
{
	return rt9759_val_toreg_via_tbl(rt9759_wdt, ARRAY_SIZE(rt9759_wdt), uS);
}

static u8 rt9759_vacovp_toreg(u32 uV)
{
	if (uV < 11000000)
		return 0x07;
	return rt9759_val_toreg(11000000, 17000000, 1000000, uV);
}

static int __rt9759_update_status(struct rt9759_chip *chip);

static const u8 rt9759_hm_password[2] = {0x69, 0x96};
static int __rt9759_enter_hidden_mode(struct rt9759_chip *chip, bool en)
{
	int ret = 0;

	mutex_lock(&chip->hm_lock);

	if (en) {
		if (chip->hm_cnt == 0) {
			ret = rt9759_i2c_write_block(chip, RT9759_REG_PASSWORD0,
						     2, rt9759_hm_password);
			if (ret < 0)
				goto err;
		}
		chip->hm_cnt++;
	} else {
		if (chip->hm_cnt == 1) /* last one */
			ret = rt9759_i2c_write8(chip, RT9759_REG_PASSWORD0,
						0x00);
		if (chip->hm_cnt > 0)
			chip->hm_cnt--;
		if (ret < 0)
			goto err;
	}
	dev_info(chip->dev, "%s en = %d\n", __func__, en);
	goto out;

err:
	dev_info(chip->dev, "%s en = %d fail(%d)\n", __func__, en, ret);
out:
	mutex_unlock(&chip->hm_lock);
	return ret;
}

static int __rt9759_get_adc(struct rt9759_chip *chip,
			    enum rt9759_adc_channel chan, int *val)
{
	int ret;
	u8 data[2];

	ret = __rt9759_enter_hidden_mode(chip, true);
	if (ret < 0)
		return ret;

	mutex_lock(&chip->adc_lock);
	/* force enable sEN_BASE pin */
	ret = rt9759_set_bits(chip, RT9759_REG_SYSCTRL1, 0x80);
	if (ret < 0)
		goto out_unlock_adc;
	ret = rt9759_set_bits(chip, RT9759_REG_ADCCTRL, RT9759_ADCEN_MASK);
	if (ret < 0)
		goto out_dis_base;
	usleep_range(12000, 15000);
	ret = rt9759_i2c_read_block(chip, rt9759_adc_reg[chan], 2, data);
	if (ret < 0)
		goto out_dis;
	switch (chan) {
	case RT9759_ADC_IBUS:
	case RT9759_ADC_VBUS:
	case RT9759_ADC_VAC:
	case RT9759_ADC_VOUT:
	case RT9759_ADC_VBAT:
	case RT9759_ADC_IBAT:
		*val = (data[0] << 8) + data[1];
		break;
	case RT9759_ADC_TDIE:
		*val = (data[0] << 7) + (data[1] >> 1);
		break;
	case RT9759_ADC_TSBAT:
	case RT9759_ADC_TSBUS:
	default:
		ret = -ENOTSUPP;
		break;
	}
	if (ret < 0)
		dev_info(chip->dev, "%s %s fail(%d)\n", __func__,
			 rt9759_adc_name[chan], ret);
	else
		dev_info(chip->dev, "%s %s %d\n", __func__,
			 rt9759_adc_name[chan], *val);
out_dis:
	ret = rt9759_clr_bits(chip, RT9759_REG_ADCCTRL, RT9759_ADCEN_MASK);
out_dis_base:
	rt9759_clr_bits(chip, RT9759_REG_SYSCTRL1, 0x80);
out_unlock_adc:
	mutex_unlock(&chip->adc_lock);
	__rt9759_enter_hidden_mode(chip, false);
	return ret;
}

static int rt9759_enable_chg(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct rt9759_chip *chip = charger_get_data(chg_dev);
	u32 err_check = BIT(RT9759_IRQIDX_VBUSOVP) |
			BIT(RT9759_IRQIDX_VACOVP) |
			BIT(RT9759_IRQIDX_VDROVP) |
			BIT(RT9759_IRQIDX_VBUSOVP) |
			BIT(RT9759_IRQIDX_TDIEOTP) |
			BIT(RT9759_IRQIDX_VBUSLERR) |
			BIT(RT9759_IRQIDX_VBUSHERR) |
			BIT(RT9759_IRQIDX_VOUTOVP) |
			BIT(RT9759_IRQIDX_TSBUSOTP) |
			BIT(RT9759_IRQIDX_TSBATOTP);
	u32 stat_check = BIT(RT9759_IRQIDX_VACINSERT) |
			 BIT(RT9759_IRQIDX_VOUTINSERT);

	dev_info(chip->dev, "%s %d\n", __func__, en);

	ret = __rt9759_enter_hidden_mode(chip, true);
	if (ret < 0)
		return ret;

	mutex_lock(&chip->adc_lock);
	if (!en) {
		ret = rt9759_clr_bits(chip, RT9759_REG_CHGCTRL1,
				      RT9759_CHGEN_MASK);
		if (ret < 0)
			goto out_unlock_adc;
		ret = rt9759_clr_bits(chip, RT9759_REG_ADCCTRL,
				      RT9759_ADCEN_MASK);
		if (ret < 0)
			goto out_unlock_adc;
		/* force disable sEN_BASE pin */
		ret = rt9759_clr_bits(chip, RT9759_REG_SYSCTRL1, 0x80);
		goto out_unlock_adc;
	}

	/* force enable sEN_BASE pin */
	ret = rt9759_set_bits(chip, RT9759_REG_SYSCTRL1, 0x80);
	if (ret < 0)
		goto out_unlock_adc;
	/* Enable ADC to check status before enable charging */
	ret = rt9759_set_bits(chip, RT9759_REG_ADCCTRL, RT9759_ADCEN_MASK);
	if (ret < 0)
		goto out_dis_base;
	usleep_range(12000, 15000);
	mutex_lock(&chip->stat_lock);
	__rt9759_update_status(chip);
	if ((chip->stat & err_check) ||
	    ((chip->stat & stat_check) != stat_check)) {
		dev_info(chip->dev, "%s error(0x%08X,0x%08X,0x%08X)\n",
			 __func__, chip->stat, err_check, stat_check);
		ret = -EINVAL;
		mutex_unlock(&chip->stat_lock);
		goto out_dis_base;
	}
	mutex_unlock(&chip->stat_lock);
	ret = rt9759_set_bits(chip, RT9759_REG_CHGCTRL1, RT9759_CHGEN_MASK);
	goto out_unlock_adc;
out_dis_base:
	rt9759_clr_bits(chip, RT9759_REG_SYSCTRL1, 0x80);
out_unlock_adc:
	mutex_unlock(&chip->adc_lock);
	__rt9759_enter_hidden_mode(chip, false);
	return ret;
}

static int rt9759_is_chg_enabled(struct charger_device *chg_dev, bool *en)
{
	int ret;
	struct rt9759_chip *chip = charger_get_data(chg_dev);

	ret = rt9759_i2c_test_bit(chip, RT9759_REG_CHGCTRL1, RT9759_CHGEN_SHFT,
				   en);
	dev_info(chip->dev, "%s %d, ret(%d)\n", __func__, *en, ret);
	return ret;
}

static inline enum rt9759_adc_channel to_rt9759_adc(enum adc_channel chan)
{
	switch (chan) {
	case ADC_CHANNEL_VBUS:
		return RT9759_ADC_VBUS;
	case ADC_CHANNEL_VBAT:
		return RT9759_ADC_VBAT;
	case ADC_CHANNEL_IBUS:
		return RT9759_ADC_IBUS;
	case ADC_CHANNEL_IBAT:
		return RT9759_ADC_IBAT;
	case ADC_CHANNEL_TEMP_JC:
		return RT9759_ADC_TDIE;
	case ADC_CHANNEL_VOUT:
		return RT9759_ADC_VOUT;
	default:
		break;
	}
	return RT9759_ADC_NOTSUPP;
}

static int rt9759_get_adc(struct charger_device *chg_dev, enum adc_channel chan,
			  int *min, int *max)
{
	int ret;
	struct rt9759_chip *chip = charger_get_data(chg_dev);
	enum rt9759_adc_channel _chan = to_rt9759_adc(chan);

	if (_chan == RT9759_ADC_NOTSUPP)
		return -EINVAL;
	ret = __rt9759_get_adc(chip, _chan, max);
	if (ret < 0)
		return ret;
	if (min != max)
		*min = *max;
	return 0;
}

static int rt9759_set_vbusovp(struct charger_device *chg_dev, u32 uV)
{
	struct rt9759_chip *chip = charger_get_data(chg_dev);
	u8 reg = rt9759_vbusovp_toreg(uV);

	return rt9759_i2c_update_bits(chip, RT9759_REG_VBUSOVP, reg,
				      RT9759_VBUSOVP_MASK);
}

static int rt9759_set_ibusocp(struct charger_device *chg_dev, u32 uA)
{
	struct rt9759_chip *chip = charger_get_data(chg_dev);
	u8 reg = rt9759_ibusocp_toreg(uA);

	return rt9759_i2c_update_bits(chip, RT9759_REG_IBUSOCUCP, reg,
				      RT9759_IBUSOCP_MASK);
}

static int rt9759_set_vbatovp(struct charger_device *chg_dev, u32 uV)
{
	struct rt9759_chip *chip = charger_get_data(chg_dev);
	u8 reg = rt9759_vbatovp_toreg(uV);

	return rt9759_i2c_update_bits(chip, RT9759_REG_VBATOVP, reg,
				      RT9759_VBATOVP_MASK);
}

static int rt9759_set_ibatocp(struct charger_device *chg_dev, u32 uA)
{
	struct rt9759_chip *chip = charger_get_data(chg_dev);
	u8 reg = rt9759_ibatocp_toreg(uA);

	return rt9759_i2c_update_bits(chip, RT9759_REG_IBATOCP, reg,
				      RT9759_IBATOCP_MASK);
}

static int rt9759_vacovp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibusucpf_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibusucpr_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_cflydiag_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_conocp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_switching_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibusucptout_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vbushigherr_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vbuslowerr_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_tdieotp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_wdt_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_adcdone_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_voutinsert_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vacinsert_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibatucpalm_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibusocpalm_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vbusovpalm_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibatocpalm_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vbatovpalm_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_tdieotpalm_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_tsbusotp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_tsbatotp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_tsbusbatotpalm_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibusocp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vbusovp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibatocp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vbatovp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_voutovp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vdrovp_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_ibatreg_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

static int rt9759_vbatreg_irq_handler(struct rt9759_chip *chip)
{
	dev_info(chip->dev, "%s\n", __func__);
	return 0;
}

struct irq_map_desc {
	const char *name;
	int (*hdlr)(struct rt9759_chip *chip);
	u8 flag_idx;
	u8 stat_idx;
	u8 flag_mask;
	u8 stat_mask;
	u32 irq_idx;
};

#define RT9759_IRQ_DESC(_name, _flag_i, _stat_i, _flag_s, _stat_s, _irq_idx) \
	{.name = #_name, .hdlr = rt9759_##_name##_irq_handler, \
	 .flag_idx = _flag_i, .stat_idx = _stat_i, \
	 .flag_mask = (1 << _flag_s), .stat_mask = (1 << _stat_s), \
	 .irq_idx = _irq_idx}

/*
 * RSS: Reister index of flag, Shift of flag, Shift of state
 * RRS: Register index of flag, Register index of state, shift of flag
 * RS: Register index of flag, shift of flag
 */
#define RT9759_IRQ_DESC_RSS(_name, _flag_i, _flag_s, _stat_s, _irq_idx) \
	RT9759_IRQ_DESC(_name, _flag_i, _flag_i, _flag_s, _stat_s, _irq_idx)

#define RT9759_IRQ_DESC_RRS(_name, _flag_i, _stat_i, _flag_s, _irq_idx) \
	RT9759_IRQ_DESC(_name, _flag_i, _stat_i, _flag_s, _flag_s, _irq_idx)

#define RT9759_IRQ_DESC_RS(_name, _flag_i, _flag_s, _irq_idx) \
	RT9759_IRQ_DESC(_name, _flag_i, _flag_i, _flag_s, _flag_s, _irq_idx)

static const struct irq_map_desc rt9759_irq_map_tbl[RT9759_IRQIDX_MAX] = {
	RT9759_IRQ_DESC_RSS(vacovp, RT9759_SF_ACPROTECT, 6, 7,
			    RT9759_IRQIDX_VACOVP),
	RT9759_IRQ_DESC_RS(ibusucpf, RT9759_SF_IBUSOCUCP, 4,
			   RT9759_IRQIDX_IBUSUCPF),
	RT9759_IRQ_DESC_RS(ibusucpr, RT9759_SF_IBUSOCUCP, 6,
			   RT9759_IRQIDX_IBUSUCPR),
	RT9759_IRQ_DESC_RS(cflydiag, RT9759_SF_CONVSTAT, 0,
			   RT9759_IRQIDX_CFLYDIAG),
	RT9759_IRQ_DESC_RS(conocp, RT9759_SF_CONVSTAT, 1, RT9759_IRQIDX_CONOCP),
	RT9759_IRQ_DESC_RS(switching, RT9759_SF_CONVSTAT, 2,
			   RT9759_IRQIDX_SWITCHING),
	RT9759_IRQ_DESC_RS(ibusucptout, RT9759_SF_CONVSTAT, 3,
			   RT9759_IRQIDX_IBUSUCPTOUT),
	RT9759_IRQ_DESC_RS(vbushigherr, RT9759_SF_CONVSTAT, 4,
			   RT9759_IRQIDX_VBUSHERR),
	RT9759_IRQ_DESC_RS(vbuslowerr, RT9759_SF_CONVSTAT, 5,
			   RT9759_IRQIDX_VBUSLERR),
	RT9759_IRQ_DESC_RSS(tdieotp, RT9759_SF_CONVSTAT, 7, 6,
			    RT9759_IRQIDX_TDIEOTP),
	RT9759_IRQ_DESC_RS(wdt, RT9759_SF_CHGCTRL0, 3, RT9759_IRQIDX_WDT),
	RT9759_IRQ_DESC_RRS(adcdone, RT9759_SF_INTFLAG, RT9759_SF_INTSTAT, 0,
			    RT9759_IRQIDX_ADCDONE),
	RT9759_IRQ_DESC_RRS(voutinsert, RT9759_SF_INTFLAG,
			    RT9759_SF_INTSTAT, 1, RT9759_IRQIDX_VOUTINSERT),
	RT9759_IRQ_DESC_RRS(vacinsert, RT9759_SF_INTFLAG, RT9759_SF_INTSTAT, 2,
			    RT9759_IRQIDX_VACINSERT),
	RT9759_IRQ_DESC_RRS(ibatucpalm, RT9759_SF_INTFLAG,
			    RT9759_SF_INTSTAT, 3, RT9759_IRQIDX_IBATUCPALM),
	RT9759_IRQ_DESC_RRS(ibusocpalm, RT9759_SF_INTFLAG,
			    RT9759_SF_INTSTAT, 4, RT9759_IRQIDX_IBUSOCPALM),
	RT9759_IRQ_DESC_RRS(vbusovpalm, RT9759_SF_INTFLAG,
			    RT9759_SF_INTSTAT, 5, RT9759_IRQIDX_VBUSOVPALM),
	RT9759_IRQ_DESC_RRS(ibatocpalm, RT9759_SF_INTFLAG,
			    RT9759_SF_INTSTAT, 6, RT9759_IRQIDX_IBATOCPALM),
	RT9759_IRQ_DESC_RRS(vbatovpalm, RT9759_SF_INTFLAG,
			    RT9759_SF_INTSTAT, 7, RT9759_IRQIDX_VBATOVPALM),
	RT9759_IRQ_DESC_RRS(tdieotpalm, RT9759_SF_FLTFLAG,
			    RT9759_SF_FLTSTAT, 0, RT9759_IRQIDX_TDIEOTPALM),
	RT9759_IRQ_DESC_RRS(tsbusotp, RT9759_SF_FLTFLAG, RT9759_SF_FLTSTAT, 1,
			    RT9759_IRQIDX_TSBUSOTP),
	RT9759_IRQ_DESC_RRS(tsbatotp, RT9759_SF_FLTFLAG, RT9759_SF_FLTSTAT, 2,
			    RT9759_IRQIDX_TSBATOTP),
	RT9759_IRQ_DESC_RRS(tsbusbatotpalm, RT9759_SF_FLTFLAG,
			    RT9759_SF_FLTSTAT, 3, RT9759_IRQIDX_TSBUSBATOTPALM),
	RT9759_IRQ_DESC_RRS(ibusocp, RT9759_SF_FLTFLAG, RT9759_SF_FLTSTAT, 4,
			    RT9759_IRQIDX_IBUSOCP),
	RT9759_IRQ_DESC_RRS(vbusovp, RT9759_SF_FLTFLAG, RT9759_SF_FLTSTAT, 5,
			    RT9759_IRQIDX_VBUSOVP),
	RT9759_IRQ_DESC_RRS(ibatocp, RT9759_SF_FLTFLAG, RT9759_SF_FLTSTAT, 6,
			    RT9759_IRQIDX_IBATOCP),
	RT9759_IRQ_DESC_RRS(vbatovp, RT9759_SF_FLTFLAG, RT9759_SF_FLTSTAT, 7,
			    RT9759_IRQIDX_VBATOVP),
	RT9759_IRQ_DESC(voutovp, RT9759_SF_REGFLAGMASK, RT9759_SF_REGTHRES,
			4, 0, RT9759_IRQIDX_VOUTOVP),
	RT9759_IRQ_DESC(vdrovp, RT9759_SF_REGFLAGMASK, RT9759_SF_REGTHRES,
			5, 1, RT9759_IRQIDX_VDROVP),
	RT9759_IRQ_DESC(ibatreg, RT9759_SF_REGFLAGMASK, RT9759_SF_REGTHRES,
			6, 2, RT9759_IRQIDX_IBATREG),
	RT9759_IRQ_DESC(vbatreg, RT9759_SF_REGFLAGMASK, RT9759_SF_REGTHRES,
			7, 3, RT9759_IRQIDX_VBATREG),
};

static int __rt9759_update_status(struct rt9759_chip *chip)
{
	int i;
	u8 sf[RT9759_SF_MAX] = {0};
	const struct irq_map_desc *desc;

	for (i = 0; i < RT9759_SF_MAX; i++)
		rt9759_i2c_read8(chip, rt9759_reg_sf[i], &sf[i]);

	for (i = 0; i < ARRAY_SIZE(rt9759_irq_map_tbl); i++) {
		desc = &rt9759_irq_map_tbl[i];
		if (sf[desc->flag_idx] & desc->flag_mask)
			chip->flag |= (1 << desc->irq_idx);
		if (sf[desc->stat_idx] & desc->stat_mask)
			chip->stat |= (1 << desc->irq_idx);
		else
			chip->stat &= ~(1 << desc->irq_idx);
	}
	return 0;
}

static int __maybe_unused rt9759_update_status(struct rt9759_chip *chip)
{
	int ret;

	mutex_lock(&chip->stat_lock);
	ret = __rt9759_update_status(chip);
	mutex_unlock(&chip->stat_lock);
	return ret;
}

static irqreturn_t rt9759_irq_handler(int irq, void *data)
{
	int i;
	struct rt9759_chip *chip = data;
	const struct irq_map_desc *desc;

	pm_stay_awake(chip->dev);
	mutex_lock(&chip->stat_lock);
	__rt9759_update_status(chip);
	for (i = 0; i < ARRAY_SIZE(rt9759_irq_map_tbl); i++) {
		desc = &rt9759_irq_map_tbl[i];
		if ((chip->flag & (1 << desc->irq_idx)) && desc->hdlr)
			desc->hdlr(chip);
	}
	chip->flag = 0;
	mutex_unlock(&chip->stat_lock);
	pm_relax(chip->dev);
	return IRQ_HANDLED;
}

static const struct charger_ops rt9759_chg_ops = {
	.enable_direct_charging = rt9759_enable_chg,
	.is_enabled = rt9759_is_chg_enabled,
	.get_adc = rt9759_get_adc,
	.set_direct_charging_vbusov = rt9759_set_vbusovp,
	.set_direct_charging_ibusoc = rt9759_set_ibusocp,
	.set_direct_charging_vbatov = rt9759_set_vbatovp,
	.set_direct_charging_ibatoc = rt9759_set_ibatocp,
};

static int rt9759_register_chgdev(struct rt9759_chip *chip)
{
	chip->chg_prop.alias_name = chip->desc->chg_name;
	chip->chg_dev = charger_device_register(chip->desc->chg_name, chip->dev,
						chip, &rt9759_chg_ops,
						&chip->chg_prop);
	if (!chip->chg_dev)
		return -EINVAL;
	return 0;
}

static int rt9759_clearall_irq(struct rt9759_chip *chip)
{
	int i, ret;
	u8 data;

	for (i = 0; i < RT9759_SF_MAX; i++) {
		ret = rt9759_i2c_read8(chip, rt9759_reg_sf[i], &data);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int rt9759_init_irq(struct rt9759_chip *chip)
{
	int ret = 0, len = 0;
	char *name = NULL;

	dev_info(chip->dev, "%s\n", __func__);
	ret = rt9759_clearall_irq(chip);
	if (ret < 0) {
		dev_info(chip->dev, "%s clr all irq fail(%d)\n", __func__, ret);
		return ret;
	}

	len = strlen(chip->desc->chg_name);
	chip->irq = gpiod_to_irq(chip->irq_gpio);
	if (chip->irq < 0) {
		dev_info(chip->dev, "%s irq mapping fail(%d)\n", __func__,
			 chip->irq);
		return ret;
	}
	dev_info(chip->dev, "%s irq = %d\n", __func__, chip->irq);

	/* Request threaded IRQ */
	name = devm_kzalloc(chip->dev, len + 5, GFP_KERNEL);
	snprintf(name, len + 5, "%s_irq", chip->desc->chg_name);
	ret = devm_request_threaded_irq(chip->dev, chip->irq, NULL,
		rt9759_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, name,
		chip);
	if (ret < 0) {
		dev_info(chip->dev, "%s request thread irq fail(%d)\n",
			 __func__, ret);
		return ret;
	}
	device_init_wakeup(chip->dev, true);
	return 0;
}

#define RT9759_DT_VALPROP(name, reg, shft, mask, func, base) \
	{#name, offsetof(struct rt9759_desc, name), reg, shft, mask, func, base}

struct rt9759_dtprop {
	const char *name;
	size_t offset;
	u8 reg;
	u8 shft;
	u8 mask;
	u8 (*toreg)(u32 val);
	u8 base;
};

static inline void rt9759_parse_dt_u32(struct device_node *np, void *desc,
				       const struct rt9759_dtprop *props,
				       int prop_cnt)
{
	int i;

	for (i = 0; i < prop_cnt; i++) {
		if (unlikely(!props[i].name))
			continue;
		of_property_read_u32(np, props[i].name, desc + props[i].offset);
	}
}

static inline void rt9759_parse_dt_bool(struct device_node *np, void *desc,
					const struct rt9759_dtprop *props,
					int prop_cnt)
{
	int i;

	for (i = 0; i < prop_cnt; i++) {
		if (unlikely(!props[i].name))
			continue;
		*((bool *)(desc + props[i].offset)) =
			of_property_read_bool(np, props[i].name);
	}
}

static inline int rt9759_apply_dt(struct rt9759_chip *chip, void *desc,
				  const struct rt9759_dtprop *props,
				  int prop_cnt)
{
	int i, ret;
	u32 val;

	for (i = 0; i < prop_cnt; i++) {
		val = *(u32 *)(desc + props[i].offset);
		if (props[i].toreg)
			val = props[i].toreg(val);
		val += props[i].base;
		ret = rt9759_i2c_update_bits(chip, props[i].reg,
					     val << props[i].shft,
					     props[i].mask);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static const struct rt9759_dtprop rt9759_dtprops_u32[] = {
	RT9759_DT_VALPROP(vbatovp, RT9759_REG_VBATOVP, 0, 0x3f,
			  rt9759_vbatovp_toreg, 0),
	RT9759_DT_VALPROP(vbatovp_alm, RT9759_REG_VBATOVP_ALM, 0, 0x3f,
			  rt9759_vbatovp_toreg, 0),
	RT9759_DT_VALPROP(ibatocp, RT9759_REG_IBATOCP, 0, 0x7f,
			  rt9759_ibatocp_toreg, 0),
	RT9759_DT_VALPROP(ibatocp_alm, RT9759_REG_IBATOCP_ALM, 0, 0x7f,
			  rt9759_ibatocp_toreg, 0),
	RT9759_DT_VALPROP(ibatucp_alm, RT9759_REG_IBATUCP_ALM, 0, 0x7f,
			  rt9759_ibatucp_toreg, 0),
	RT9759_DT_VALPROP(vbusovp, RT9759_REG_VBUSOVP, 0, 0x7f,
			  rt9759_vbusovp_toreg, 0),
	RT9759_DT_VALPROP(vbusovp_alm, RT9759_REG_VBUSOVP_ALM, 0, 0x7f,
			  rt9759_vbusovp_toreg, 0),
	RT9759_DT_VALPROP(ibusocp, RT9759_REG_IBUSOCUCP, 0, 0x0f,
			  rt9759_ibusocp_toreg, 0),
	RT9759_DT_VALPROP(ibusocp_alm, RT9759_REG_IBUSOCP_ALM, 0, 0x7f,
			  rt9759_ibusocp_alm_toreg, 0),
	RT9759_DT_VALPROP(wdt, RT9759_REG_CHGCTRL0, 0, 0x03,
			  rt9759_wdt_toreg, 0),
	RT9759_DT_VALPROP(vacovp, RT9759_REG_ACPROTECT, 0, 0x07,
			  rt9759_vacovp_toreg, 0),
};

static const struct rt9759_dtprop rt9759_dtprops_bool[] = {
	RT9759_DT_VALPROP(vbatovp_dis, RT9759_REG_VBATOVP, 7, 0x80, NULL, 0),
	RT9759_DT_VALPROP(vbatovp_alm_dis, RT9759_REG_VBATOVP_ALM, 7, 0x80,
			  NULL, 0),
	RT9759_DT_VALPROP(ibatocp_dis, RT9759_REG_IBATOCP, 7, 0x80, NULL, 0),
	RT9759_DT_VALPROP(ibatocp_alm_dis, RT9759_REG_IBATOCP_ALM, 7, 0x80,
			  NULL, 0),
	RT9759_DT_VALPROP(ibatucp_alm_dis, RT9759_REG_IBATUCP_ALM, 7, 0x80,
			  NULL, 0),
	RT9759_DT_VALPROP(vbusovp_alm_dis, RT9759_REG_VBUSOVP_ALM, 7, 0x80,
			  NULL, 0),
	RT9759_DT_VALPROP(ibusocp_dis, RT9759_REG_IBUSOCUCP, 7, 0x80, NULL, 0),
	RT9759_DT_VALPROP(ibusocp_alm_dis, RT9759_REG_IBUSOCP_ALM, 7, 0x80,
			  NULL, 0),
	RT9759_DT_VALPROP(wdt_dis, RT9759_REG_CHGCTRL0, 2, 0x04, NULL, 0),
	RT9759_DT_VALPROP(tsbusotp_dis, RT9759_REG_CHGCTRL1, 2, 0x04, NULL, 0),
	RT9759_DT_VALPROP(tsbatotp_dis, RT9759_REG_CHGCTRL1, 1, 0x02, NULL, 0),
	RT9759_DT_VALPROP(tdieotp_dis, RT9759_REG_CHGCTRL1, 0, 0x01, NULL, 0),
	RT9759_DT_VALPROP(reg_en, RT9759_REG_REGCTRL, 4, 0x10, NULL, 0),
	RT9759_DT_VALPROP(voutovp_dis, RT9759_REG_REGCTRL, 3, 0x08, NULL, 0),
};

static int rt9759_parse_dt(struct rt9759_chip *chip)
{
	struct rt9759_desc *desc;
	struct device_node *np = chip->dev->of_node;

	if (!np)
		return -ENODEV;

	chip->irq_gpio = devm_gpiod_get(chip->dev, "rt9759,intr", GPIOD_IN);
	if (IS_ERR(chip->irq_gpio))
		return PTR_ERR(chip->irq_gpio);

	desc = devm_kzalloc(chip->dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	memcpy(desc, &rt9759_desc_defval, sizeof(*desc));
	if (of_property_read_string(np, "chg_name", &desc->chg_name) < 0)
		dev_info(chip->dev, "%s no chg name\n", __func__);
	if (of_property_read_string(np, "rm_name", &desc->rm_name) < 0)
		dev_info(chip->dev, "%s no rm name\n", __func__);
	if (of_property_read_u8(np, "rm_slave_addr", &desc->rm_slave_addr) < 0)
		dev_info(chip->dev, "%s no regmap slave addr\n", __func__);
	rt9759_parse_dt_u32(np, (void *)desc, rt9759_dtprops_u32,
			    ARRAY_SIZE(rt9759_dtprops_u32));
	rt9759_parse_dt_bool(np, (void *)desc, rt9759_dtprops_bool,
			     ARRAY_SIZE(rt9759_dtprops_bool));
	chip->desc = desc;
	return 0;
}

static int rt9759_init_chip(struct rt9759_chip *chip)
{
	int ret;

	dev_info(chip->dev, "%s\n", __func__);
	ret = rt9759_apply_dt(chip, (void *)chip->desc, rt9759_dtprops_u32,
			      ARRAY_SIZE(rt9759_dtprops_u32));
	if (ret < 0)
		return ret;
	ret = rt9759_apply_dt(chip, (void *)chip->desc, rt9759_dtprops_bool,
			      ARRAY_SIZE(rt9759_dtprops_bool));
	if (ret < 0)
		return ret;
	return 0;
}

static int rt9759_check_devinfo(struct i2c_client *client, u8 *chip_rev)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, RT9759_REG_DEVINFO);
	if (ret < 0)
		return ret;
	if ((ret & 0x0f) != RT9759_DEVID)
		return -ENODEV;
	*chip_rev = (ret & 0xf0) >> 4;
	dev_info(&client->dev, "%s rev = 0x%02X\n", __func__, *chip_rev);
	return 0;
}

static int rt9759_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	int ret;
	struct rt9759_chip *chip;
	u8 chip_rev;

	dev_info(&client->dev, "%s\n", __func__);

	ret = rt9759_check_devinfo(client, &chip_rev);
	if (ret < 0)
		return ret;

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
	chip->dev = &client->dev;
	chip->client = client;
	chip->revision = chip_rev;
	mutex_init(&chip->io_lock);
	mutex_init(&chip->adc_lock);
	mutex_init(&chip->stat_lock);
	mutex_init(&chip->hm_lock);
	mutex_init(&chip->suspend_lock);
	i2c_set_clientdata(client, chip);

	ret = rt9759_parse_dt(chip);
	if (ret < 0) {
		dev_info(chip->dev, "%s parse dt fail(%d)\n", __func__, ret);
		goto err;
	}
#ifdef CONFIG_RT_REGMAP
	ret = rt9759_register_regmap(chip);
	if (ret < 0) {
		dev_info(chip->dev, "%s reg regmap fail(%d)\n", __func__, ret);
		goto err;
	}
#endif /* CONFIG_RT_REGMAP */
	ret = rt9759_init_chip(chip);
	if (ret < 0) {
		dev_info(chip->dev, "%s init chip fail(%d)\n", __func__, ret);
		goto err_initchip;
	}
	ret = rt9759_register_chgdev(chip);
	if (ret < 0) {
		dev_info(chip->dev, "%s reg chgdev fail(%d)\n", __func__, ret);
		goto err_initchip;
	}
	ret = rt9759_init_irq(chip);
	if (ret < 0) {
		dev_info(chip->dev, "%s init irq fail(%d)\n", __func__, ret);
		goto err_initirq;
	}

	dev_info(chip->dev, "%s successfully\n", __func__);
	return 0;
err_initirq:
	charger_device_unregister(chip->chg_dev);
err_initchip:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(chip->rm_dev);
#endif /* CONFIG_RT_REGMAP */
err:
	mutex_destroy(&chip->suspend_lock);
	mutex_destroy(&chip->hm_lock);
	mutex_destroy(&chip->stat_lock);
	mutex_destroy(&chip->adc_lock);
	mutex_destroy(&chip->io_lock);
	return ret;
}

static void rt9759_i2c_shutdown(struct i2c_client *client)
{
	dev_info(&client->dev, "%s\n", __func__);
}

static int rt9759_i2c_remove(struct i2c_client *client)
{
	struct rt9759_chip *chip = i2c_get_clientdata(client);

	dev_info(&client->dev, "%s\n", __func__);
	if (!chip)
		return 0;
	charger_device_unregister(chip->chg_dev);
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(chip->rm_dev);
#endif /* CONFIG_RT_REGMAP */
	mutex_destroy(&chip->hm_lock);
	mutex_destroy(&chip->stat_lock);
	mutex_destroy(&chip->adc_lock);
	mutex_destroy(&chip->io_lock);
	return 0;
}

static int __maybe_unused rt9759_i2c_suspend(struct device *dev)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	struct rt9759_chip *chip = i2c_get_clientdata(i2c);

	dev_info(dev, "%s\n", __func__);
	mutex_lock(&chip->suspend_lock);
	if (device_may_wakeup(dev))
		enable_irq_wake(chip->irq);
	return 0;
}

static int __maybe_unused rt9759_i2c_resume(struct device *dev)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	struct rt9759_chip *chip = i2c_get_clientdata(i2c);

	dev_info(dev, "%s\n", __func__);
	mutex_unlock(&chip->suspend_lock);
	if (device_may_wakeup(dev))
		disable_irq_wake(chip->irq);
	return 0;
}

static SIMPLE_DEV_PM_OPS(rt9759_pm_ops, rt9759_i2c_suspend, rt9759_i2c_resume);

static const struct of_device_id rt9759_of_id[] = {
	{ .compatible = "richtek,rt9759" },
	{},
};
MODULE_DEVICE_TABLE(of, rt9759_of_id);

static const struct i2c_device_id rt9759_i2c_id[] = {
	{ "rt9759", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, rt9759_i2c_id);

static struct i2c_driver rt9759_i2c_driver = {
	.driver = {
		.name = "rt9759",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt9759_of_id),
		.pm = &rt9759_pm_ops,
	},
	.probe = rt9759_i2c_probe,
	.shutdown = rt9759_i2c_shutdown,
	.remove = rt9759_i2c_remove,
	.id_table = rt9759_i2c_id,
};
module_i2c_driver(rt9759_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Richtek RT9759 Charger Driver");
MODULE_AUTHOR("ShuFan Lee<shufan_lee@richtek.com>");
MODULE_VERSION(RT9759_DRV_VERSION);
