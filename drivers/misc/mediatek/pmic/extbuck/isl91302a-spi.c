/*
 * Copyright (C) 2016 MediaTek Inc.
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
#include <linux/types.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <mtk_spi.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#ifdef CONFIG_RT_REGMAP
#include "../../rt-regmap/rt-regmap.h"
#endif /* CONFIG_RT_REGMAP */
#include "isl91302a-spi.h"

#define ISL91302A_DRV_VERSION	"1.0.0_MTK"
#define ISL91302A_IRQ_ENABLE	0

static int isl91302a_read_device(void *client, u32 addr, int len, void *dst)
{
	int ret;
	struct spi_device *spi = (struct spi_device *)client;
	struct spi_transfer xfer = {0,}; /* must init spi_transfer here */
	struct spi_message msg;
	u32 tx_buf;
	u32 rx_buf;

	if (len != 1) {
		pr_err("%s not support multi read now\n", __func__);
		return -EINVAL;
	}

	/* LSM first, TX: buf, size, reg, 89 */
	tx_buf = 0x00010080;
	rx_buf = 0xffffffff;

	tx_buf |= addr << 8;

	xfer.tx_buf = &tx_buf;
	xfer.rx_buf = &rx_buf;
	xfer.len = 4;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	ret = spi_sync(spi, &msg);
	if (ret < 0 || rx_buf == 0xffffffff)
		return ret;

	*(unsigned char *)dst = (rx_buf & 0xff000000) >> 24;
#if 0
	pr_info("%s addr 0x%02x = 0x%02x\n", __func__, addr, *val);
	pr_info("%s tx_buf = 0x%08x\n", __func__, tx_buf);
	pr_info("%s rx_buf = 0x%08x\n", __func__, rx_buf);
#endif
	return ret;
}

int isl91302a_write_device(void *client,
		uint32_t addr, int len, const void *src)
{
	int ret;
	struct spi_device *spi = (struct spi_device *)client;
	struct spi_transfer xfer = {0,}; /* must init spi_transfer here */
	struct spi_message msg;
	unsigned char regval;
	u32 tx_buf;
	u32 rx_buf;

	if (len != 1) {
		pr_err("%s not support multi write now\n", __func__);
		return -EINVAL;
	}

	tx_buf = 0x00010000;
	rx_buf = 0xffffffff;

	regval = *(unsigned char *)src;
	tx_buf |= ((regval << 24) | (addr << 8));

	xfer.tx_buf = &tx_buf;
	xfer.rx_buf = &rx_buf;
	xfer.len = 4;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	ret = spi_sync(spi, &msg);
	if (ret < 0 || rx_buf == 0xffffffff)
		return ret;

#if 0
	pr_info("%s addr 0x%02x = 0x%02x\n", __func__, addr, value);
	pr_info("%s tx_buf = 0x%08x\n", __func__, tx_buf);
	pr_info("%s rx_buf = 0x%08x\n", __func__, rx_buf);
#endif
	return ret;
}

static int isl91302a_write_page2(struct spi_device *spi,
			unsigned char addr, unsigned char value)
{
	int ret;
	struct spi_transfer xfer = {0,}; /* must init spi_transfer here */
	struct spi_message msg;
	u32 tx_buf;
	u32 rx_buf;

	tx_buf = 0x00010002;
	rx_buf = 0xffffffff;

	tx_buf |= ((value << 24) | (addr << 8));

	xfer.tx_buf = &tx_buf;
	xfer.rx_buf = &rx_buf;
	xfer.len = 4;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	ret = spi_sync(spi, &msg);
	if (ret < 0 || rx_buf == 0xffffffff)
		return ret;

#if 0
	pr_info("%s addr 0x%02x = 0x%02x\n", __func__, addr, value);
	pr_info("%s tx_buf = 0x%08x\n", __func__, tx_buf);
	pr_info("%s rx_buf = 0x%08x\n", __func__, rx_buf);
#endif
	return ret;
}



#ifdef CONFIG_RT_REGMAP

RT_REG_DECL(ISL91302A_CHIPNAME_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_FLT_RECORDTEMP_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_MODECTRL_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_IRQ_MASK_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK1_DCM_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK1_UP_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK1_LO_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK1_RSPCFG1_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK2_DCM_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK2_UP_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK2_LO_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK2_RSPCFG1_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK3_DCM_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK3_UP_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK3_LO_R, 1, RT_VOLATILE, {});
RT_REG_DECL(ISL91302A_BUCK3_RSPCFG1_R, 1, RT_VOLATILE, {});

static const rt_register_map_t isl91302a_regmap[] = {
	RT_REG(ISL91302A_CHIPNAME_R),
	RT_REG(ISL91302A_FLT_RECORDTEMP_R),
	RT_REG(ISL91302A_MODECTRL_R),
	RT_REG(ISL91302A_IRQ_MASK_R),
	RT_REG(ISL91302A_BUCK1_DCM_R),
	RT_REG(ISL91302A_BUCK1_UP_R),
	RT_REG(ISL91302A_BUCK1_LO_R),
	RT_REG(ISL91302A_BUCK1_RSPCFG1_R),
	RT_REG(ISL91302A_BUCK2_DCM_R),
	RT_REG(ISL91302A_BUCK2_UP_R),
	RT_REG(ISL91302A_BUCK2_LO_R),
	RT_REG(ISL91302A_BUCK2_RSPCFG1_R),
	RT_REG(ISL91302A_BUCK3_DCM_R),
	RT_REG(ISL91302A_BUCK3_UP_R),
	RT_REG(ISL91302A_BUCK3_LO_R),
	RT_REG(ISL91302A_BUCK3_RSPCFG1_R),
};

static struct rt_regmap_properties isl91302a_regmap_props = {
	.name = "isl91302a",
	.aliases = "isl91302a",
	.register_num = ARRAY_SIZE(isl91302a_regmap),
	.rm = isl91302a_regmap,
	.rt_regmap_mode = RT_CACHE_DISABLE,
};

static struct rt_regmap_fops isl91302a_regmap_fops = {
	.read_device = isl91302a_read_device,
	.write_device = isl91302a_write_device,
};
#endif /* CONFIG_RT_REGMAP */

int isl91302a_read_byte(void *client, uint32_t addr, uint32_t *value)
{
	struct spi_device *spi = (struct spi_device *)client;
	struct isl91302a_chip *chip = spi_get_drvdata(spi);
	int ret;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(chip->regmap_dev, addr, 1, value);
#else
	ret = isl91302a_read_device(chip->spi, addr, 1, value);
#endif /* CONFIG_RT_REGMAP */
	if (ret < 0)
		pr_err("%s read addr0x%02x fail\n", __func__, addr);
	return ret;
}
EXPORT_SYMBOL(isl91302a_read_byte);

int isl91302a_write_byte(void *client, uint32_t addr, uint32_t data)
{
	int ret;
	struct spi_device *spi = (struct spi_device *)client;
	struct isl91302a_chip *chip = spi_get_drvdata(spi);

#ifdef CONFIG_RT_REGMAP
	ret =  rt_regmap_block_write(chip->regmap_dev, addr, 1, &data);
#else
	ret =  isl91302a_write_device(chip->spi, addr, 1, &data);
#endif /* CONFIG_RT_REGMAP */
	if (ret < 0)
		pr_err("%s write addr0x%02x fail\n", __func__, addr);
	return ret;
}

int isl91302a_assign_bit(void *client,
	uint32_t reg, uint32_t mask, uint32_t data)
{
	struct spi_device *spi = (struct spi_device *)client;
	struct isl91302a_chip *ri = spi_get_drvdata(spi);
	unsigned char tmp = 0;
	uint32_t regval = 0;
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = isl91302a_read_byte(spi, reg, &regval);
	if (ret < 0) {
		pr_err("%s fail reg0x%02x data0x%02x\n",
				__func__, reg, data);
		goto OUT_ASSIGN;
	}
	tmp = ((regval & 0xff) & ~mask);
	tmp |= (data & mask);
	ret = isl91302a_write_byte(spi, reg, tmp);
	if (ret < 0)
		pr_err("%s fail reg0x%02x data0x%02x\n",
				__func__, reg, tmp);
OUT_ASSIGN:
	mutex_unlock(&ri->io_lock);
	return  ret;
}
EXPORT_SYMBOL(isl91302a_assign_bit);

static struct mt_chip_conf isl91302a_spi_config = {
	/* setuptime, holdtime --> timing for waveform of SPI */
	.setuptime = 3,
	.holdtime = 3,
	/* high_time, low_time --> set SCK */
	.high_time = 10,
	.low_time = 10,
	/* CS pin idle time */
	.cs_idletime = 2,
	/* cpol, cpha -->set type */
	.cpol = 0,
	.cpha = 0,
	/* rx_mlsb, tx_mlsb --> MSB first or not */
	.rx_mlsb = 1,
	.tx_mlsb = 1,
	/* tx_endian, rx_endian -->
	 * Defines whether to reverse the endian order
	 */
	.tx_endian = 0,
	.rx_endian = 0,
	/* com_mod --> FIFO/DMA mode */
	.com_mod = FIFO_TRANSFER,
	/* pause --> if want to always let CS active, set this flag to 1*/
	.pause = 1,
	/* tckdly --> tune timing */
	.tckdly = 0,

	/* ?? */
	.ulthgh_thrsh = 0,
	.cs_pol = 0,
	.sample_sel = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
};

static void isl91302a_spi_init(struct spi_device *spi)
{
	pr_info("%s inited\n", __func__);
	spi->bits_per_word = 32;
	spi->controller_data = &isl91302a_spi_config;
	mdelay(100);
}

static void isl91302a_reg_init(struct spi_device *spi)
{
	/* Intersil provided */
	isl91302a_write_byte(spi, 0x37, 0xba);
	isl91302a_write_byte(spi, 0x36, 0x5e);
	isl91302a_write_byte(spi, 0x35, 0xba);
	isl91302a_write_byte(spi, 0x34, 0x11);
	isl91302a_write_byte(spi, 0x5d, 0xe0);
	isl91302a_write_byte(spi, 0x68, 0x17);
	isl91302a_write_byte(spi, 0x69, 0x02);
	isl91302a_write_byte(spi, 0x85, 0x02);
	isl91302a_write_byte(spi, 0x5d, 0xb0);
	isl91302a_write_byte(spi, 0x6f, 0x16);
	isl91302a_write_page2(spi, 0x44, 0x65);
	isl91302a_write_page2(spi, 0x46, 0x65);
	isl91302a_write_page2(spi, 0x48, 0x67);
	isl91302a_write_page2(spi, 0x4a, 0x67);

}

static int isl91302a_check_id(struct spi_device *spi)
{
	int ret;
	unsigned char data;

	ret = isl91302a_read_device(spi, ISL91302A_CHIPNAME_R, 1, &data);
	if (ret < 0) {
		pr_err("%s IO fail\n", __func__);
		return -EIO;
	}

	if (data != ISL91302A_CHIPNAME) {
		pr_err("%s ID(0x%02x) not match\n", __func__, data);
		return -EINVAL;
	}
	return 0;
}

#if ISL91302A_IRQ_ENABLE
enum {
	ISL91302A_EVT_OT_SHUTDOWN_FALLING = 1,
	ISL91302A_EVT_OT_SHUTDOWN_RISING,
	ISL91302A_EVT_MAX,
};

static void isl91302a_irq_evt_handler(void *info, int eventno)
{
	struct isl91302a_chip *chip = info;

	dev_info(chip->dev, "%s eventno = %d\n", __func__, eventno);
	switch (eventno) {
	case ISL91302A_EVT_OT_SHUTDOWN_RISING:
		pr_err("%s Enter OT Shutdown\n", __func__);
		break;
	case ISL91302A_EVT_OT_SHUTDOWN_FALLING:
		pr_err("%s Exit OT Shutdown\n", __func__);
		break;
	}
}

typedef void (*rt_irq_handler)(void *info, int eventno);

static rt_irq_handler isl91302a_handler[ISL91302A_EVT_MAX] = {
	[ISL91302A_EVT_OT_SHUTDOWN_RISING] = isl91302a_irq_evt_handler,
	[ISL91302A_EVT_OT_SHUTDOWN_FALLING] = isl91302a_irq_evt_handler,
};

static irqreturn_t isl91302a_irq_handler(int irqno, void *param)
{
	struct isl91302a_chip *chip = param;
	uint32_t regval = 0;
	int ret, i;

	ret = isl91302a_read_byte(chip->spi,
			ISL91302A_FLT_RECORDTEMP_R, &regval);
	if (ret < 0) {
		pr_err("%s get irq regval fail\n", __func__);
		return IRQ_HANDLED;
	}
	if (regval) {
		pr_info("%s thermal event 0x%02x\n", __func__, regval);
		for (i = 0; i < ISL91302A_EVT_MAX; i++) {
			if ((regval & (1 << i)) && isl91302a_handler[i])
				isl91302a_handler[i](chip, i);
		}
	}

	return IRQ_HANDLED;
}

static int isl91302a_init_irq(struct isl91302a_chip *chip, struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret;
	uint32_t regval = 0;

	if (np)
		chip->irq = irq_of_parse_and_map(np, 0);
	else {
		pr_err("%s no dts node\n", __func__);
		return -ENODEV;
	}

	/* mask IRQ first */
	ret = isl91302a_write_byte(chip->spi, ISL91302A_IRQ_MASK_R, 0x83);
	if (ret < 0) {
		pr_err("%s mask IRQ fail\n", __func__);
		return -EINVAL;
	}
	/* clear IRQ */
	isl91302a_read_byte(chip->spi, ISL91302A_FLT_RECORDTEMP_R, &regval);

	ret = devm_request_threaded_irq(chip->dev, chip->irq,
		isl91302a_irq_handler, NULL, IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
		"isl91302a-irq", chip);
	if (ret < 0) {
		pr_err("%s request irq fail\n", __func__);
		return -EINVAL;
	}
	/* unmask IRQ */
	ret = isl91302a_write_byte(chip->spi, ISL91302A_IRQ_MASK_R, 0x00);
	if (ret < 0) {
		pr_err("%s unmask IRQ fail\n", __func__);
		return -EINVAL;
	}

	enable_irq_wake(chip->irq);
	return 0;
}
#endif /* if ISL91302A_IRQ_ENABLE */

static int isl91302a_spi_probe(struct spi_device *spi)
{
	struct isl91302a_chip *chip;
	int ret;

	pr_info("%s\n", __func__);

	chip = devm_kzalloc(&spi->dev,
		sizeof(struct isl91302a_chip), GFP_KERNEL);

	chip->spi = spi;
	chip->dev = &spi->dev;
	mutex_init(&chip->io_lock);
	spi_set_drvdata(spi, chip);

	isl91302a_spi_init(spi);

	ret = isl91302a_check_id(spi);
	if (ret < 0)
		return ret;
#ifdef CONFIG_RT_REGMAP
	chip->regmap_dev = rt_regmap_device_register(&isl91302a_regmap_props,
			&isl91302a_regmap_fops, &spi->dev, spi, chip);
	if (!chip->regmap_dev) {
		pr_err("%s register regmap fail\n", __func__);
		return -EINVAL;
	}
#endif /* #ifdef CONFIG_RT_REGMAP */

	isl91302a_reg_init(spi);

#if ISL91302A_IRQ_ENABLE
	ret = isl91302a_init_irq(chip, &spi->dev);
	if (ret < 0) {
		pr_err("%s IRQ init fail\n", __func__);
		goto fail_irq;
	}
#endif /* if ISL91302A_ENABLE_IRQ */


	ret = isl91302a_regulator_init(chip);
	if (ret < 0) {
		ISL91302A_ERR("%s regulator init fail\n", __func__);
		return -EINVAL;
	}

	pr_info("%s --OK!!--\n", __func__);
	return 0;

#if ISL91302A_IRQ_ENABLE
fail_irq:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(chip->regmap_dev);
#endif /* CONFIG_RT_REGMAP */
#endif /* CONFIG_IRQ_ENABLE */
	return ret;
}

static int isl91302a_spi_remove(struct spi_device *spi)
{
	struct isl91302a_chip *chip = spi_get_drvdata(spi);

	if (chip) {
		isl91302a_regulator_deinit(chip);
		mutex_destroy(&chip->io_lock);
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(chip->regmap_dev);
#endif /* #ifdef CONFIG_RT_REGMAP */
	}
	return 0;
}

static const struct of_device_id isl91302a_id_table[] = {
	{.compatible = "mediatek,isl91302a",},
};

static struct spi_driver isl91302a_spi_driver = {
	.driver = {
		.name = "isl91302a",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
		.of_match_table = isl91302a_id_table,
	},
	.probe = isl91302a_spi_probe,
	.remove = isl91302a_spi_remove,
};

static int __init isl91302a_init(void)
{
	pr_err("%s ver(%s)\n", __func__, ISL91302A_DRV_VERSION);
	return spi_register_driver(&isl91302a_spi_driver);
}

static void __exit isl91302a_exit(void)
{
	spi_unregister_driver(&isl91302a_spi_driver);
}
/* late_initcall(isl91302a_init); */
module_init(isl91302a_init);
module_exit(isl91302a_exit);

MODULE_DESCRIPTION("ISL91302A Regulator Driver");
MODULE_VERSION(ISL91302A_DRV_VERSION);
MODULE_AUTHOR("Sakya <jeff_chang@richtek.com>");
MODULE_LICENSE("GPL");
