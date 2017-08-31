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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>

#define BSI_BASE          (xo_inst->base)
#define BSI_CON	          0x0000
#define BSI_WRDAT_CON     0x0004
#define BSI_WRDAT         0x0008
#define BSI_RDCON         0x0c40
#define BSI_RDADDR_CON    0x0c44
#define BSI_RDADDR        0x0c48
#define BSI_RDCS_CON      0x0c4c
#define BSI_RDDAT         0x0c50

#define BSI_WRITE_READY (1 << 31)
#define BSI_READ_READY (1 << 31)
#define BSI_READ_BIT (1 << 8)
#define BITS(m, n) (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))

#define READ_REGISTER_UINT32(reg)          readl((void __iomem *)reg)
#define WRITE_REGISTER_UINT32(reg, val)     writel((val), (void __iomem *)(reg))

struct xo_dev {
	struct device *dev;
	void __iomem *base;
	struct clk *bsi_clk;
	struct clk *rg_bsi_clk;
	uint32_t cur_xo_capid;
};

static struct xo_dev *xo_inst;
static const struct of_device_id apxo_of_ids[] = {
	{ .compatible = "mediatek,mt8167-xo", },
	{},
};

MODULE_DEVICE_TABLE(of, apxo_of_ids);

static uint32_t BSI_read(uint32_t rdaddr)
{
	uint32_t readaddr = BSI_READ_BIT | rdaddr;
	uint32_t ret;

	WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDCON, 0x9f8b);
	WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDADDR_CON, 0x0902);
	WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDADDR, readaddr);
	WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDCS_CON, 0x0);
	WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDCON, 0x89f8b);

	while (!(READ_REGISTER_UINT32(BSI_BASE + BSI_RDCON) & BSI_READ_READY))
		pr_debug("wait bsi read done!\n");

	ret = READ_REGISTER_UINT32(BSI_BASE + BSI_RDDAT) & 0x0000ffff;
	pr_debug("BSI Read Done: value = 0x%x\n", ret);
	return ret;
}

static void BSI_write(uint32_t wraddr, uint32_t wrdata)
{
	uint32_t wrdat;

	WRITE_REGISTER_UINT32(BSI_BASE + BSI_WRDAT_CON, 0x1d00);
	wrdat = (wraddr << 20) + wrdata;

	pr_debug("BSI_write: wrdat = 0x%x\n", wrdat);
	WRITE_REGISTER_UINT32(BSI_BASE + BSI_WRDAT, wrdat);
	WRITE_REGISTER_UINT32(BSI_BASE + BSI_CON, 0x80401);
	while (!(READ_REGISTER_UINT32(BSI_BASE + BSI_CON) & BSI_WRITE_READY))
		pr_debug("wait bsi write done!\n");

	pr_debug("BSI Write Done\n");
}

static void XO_trim_write(uint32_t cap_code)
{
	uint32_t wrdat = 0;
	/* 0x09 [14:12] = cap_code[6:4] */
	wrdat = BSI_read(0x09) & ~BITS(12, 14);
	wrdat |= (cap_code & BITS(4, 6)) << 8;
	BSI_write(0x09, wrdat);
	/* 0x09 [10:4] = cap_code[6:0] */
	wrdat = BSI_read(0x09) & ~BITS(4, 10);
	wrdat |= (cap_code & BITS(0, 6)) << 4;
	BSI_write(0x09, wrdat);
	/* 0x01 [11:10] = 2'b11 */
	BSI_write(0x01, 0xC00);
	mdelay(10);
	/* 0x01 [11:10] = 2'b01 */
	BSI_write(0x01, 0x400);
	/* 0x1f [5:3] =  cap_code[6:4] */
	wrdat = BSI_read(0x1f) & ~BITS(3, 5);
	wrdat |= (cap_code & BITS(4, 6)) >> 1;
	BSI_write(0x1f, wrdat);
	/* 0x1f [2:0] =  cap_code[6:4] */
	wrdat = BSI_read(0x1f) & ~BITS(0, 2);
	wrdat |= (cap_code & BITS(4, 6)) >> 4;
	BSI_write(0x1f, wrdat);
	/* 0x1e [15:12] =  cap_code[3:0] */
	wrdat = BSI_read(0x1e) & ~BITS(12, 15);
	wrdat |= (cap_code & BITS(0, 3)) << 12;
	BSI_write(0x1e, wrdat);
	/* 0x4b [5:3] =  cap_code[6:4] */
	wrdat = BSI_read(0x4b) & ~BITS(3, 5);
	wrdat |= (cap_code & BITS(4, 6)) >> 1;
	BSI_write(0x4b, wrdat);
	/* 0x4b [2:0] =  cap_code[6:4] */
	wrdat = BSI_read(0x4b) & ~BITS(0, 2);
	wrdat |= (cap_code & BITS(4, 6)) >> 4;
	BSI_write(0x4b, wrdat);
	/* 0x4a [15:12] =  cap_code[3:0] */
	wrdat = BSI_read(0x4a) & ~BITS(12, 15);
	wrdat |= (cap_code & BITS(0, 3)) << 12;
	BSI_write(0x4a, wrdat);
}

static uint32_t XO_trim_read(void)
{
	uint32_t cap_code = 0;
	/* cap_code[4:0] = 0x00 [15:11] */
	cap_code = (BSI_read(0x00) & BITS(11, 15)) >> 11;
	/* cap_code[6:5] = 0x01 [1:0] */
	cap_code |= (BSI_read(0x01) & BITS(0, 1)) << 5;
	return cap_code;
}

static void enable_xo_low_power_mode(void)
{
	uint32_t value = 0;

	/* RG_DA_EN_XO_BG_MANVALUE = 1 */
	value = BSI_read(0x03) | (1<<12);
	BSI_write(0x03, value);
	/* RG_DA_EN_XO_BG_MAN = 1 */
	value = BSI_read(0x03) | (1<<13);
	BSI_write(0x03, value);
	/* RG_DA_EN_XO_LDOH_MANVALUE = 1 */
	value = BSI_read(0x03) | (1<<8);
	BSI_write(0x03, value);
	/* RG_DA_EN_XO_LDOH_MAN = 1 */
	value = BSI_read(0x03) | (1<<9);
	BSI_write(0x03, value);
	/* RG_DA_EN_XO_LDOL_MANVALUE = 1 */
	value = BSI_read(0x03) | 0x1;
	BSI_write(0x03, value);
	/* RG_DA_EN_XO_LDOL_MAN = 1 */
	value = BSI_read(0x03) | (1<<1);
	BSI_write(0x03, value);
	/* RG_DA_EN_XO_PRENMBUF_VALUE = 1 */
	value = BSI_read(0x02) | (1<<6);
	BSI_write(0x02, value);
	/* RG_DA_EN_XO_PRENMBUF_MAN = 1 */
	value = BSI_read(0x02) | (1<<7);
	BSI_write(0x02, value);
	/* RG_DA_EN_XO_PLLGP_BUF_MANVALUE = 1 */
	value = BSI_read(0x34) | 0x1;
	BSI_write(0x34, value);
	/* RG_DA_EN_XO_PLLGP_BUF_MAN = 1 */
	value = BSI_read(0x34) | (1<<1);
	BSI_write(0x34, value);

	/* RG_DA_EN_XO_VGTIELOW_MANVALUE=0 */
	value = BSI_read(0x05) & 0xFEFF;
	BSI_write(0x05, value);

	/* RG_DA_EN_XO_VGTIELOW_MAN=1 */
	value = BSI_read(0x05) | (1<<9);
	BSI_write(0x05, value);

	/* RG_DA_XO_LPM_BIAS1/2/3_MAN=1 */
	value = BSI_read(0x06) | (1<<13);
	BSI_write(0x06, value);
	value = BSI_read(0x06) | (1<<11);
	BSI_write(0x06, value);
	value = BSI_read(0x06) | (1<<9);
	BSI_write(0x06, value);

	/* RG_DA_XO_LPM_BIAS1/2/3_MANVALUE=0 */
	value = BSI_read(0x06) & ~BIT(12);
	BSI_write(0x06, value);
	value = BSI_read(0x06) & ~BIT(10);
	BSI_write(0x06, value);
	value = BSI_read(0x06) & ~BIT(8);
	BSI_write(0x06, value);

	/* bit 10 set 0 */
	value = BSI_read(0x08) & 0xFBFF;
	BSI_write(0x08, value);

	/* DIG_CR_XO_04_L[9]:RG_XO_INT32K_NOR2LPM_TRIGGER = 1 */
	value = BSI_read(0x08) | (1<<9);
	BSI_write(0x08, value);
	pr_notice("[xo] enable xo low power mode!\n");
}

static void get_xo_status(void)
{
	uint32_t status = 0;

	status = (BSI_read(0x26) & BITS(4, 9))>>4;
	pr_notice("[xo] status: 0x%x\n", status);
}

static ssize_t show_xo_capid(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "xo capid: 0x%x\n", XO_trim_read());
}

static ssize_t store_xo_capid(struct device *dev, struct device_attribute *attr,
				     const char *buf, size_t size)
{
	uint32_t capid;
	int ret;

	if (buf != NULL && size != 0) {
		ret = kstrtouint(buf, 0, &capid);
		if (ret) {
			pr_err("wrong format!\n");
			return size;
		}
		if (capid > 0x7f) {
			pr_err("cap code should be 7bit!\n");
			return size;
		}

		pr_notice("original cap code: 0x%x\n", XO_trim_read());
		XO_trim_write(capid);
		mdelay(10);
		xo_inst->cur_xo_capid = XO_trim_read();
		pr_notice("write cap code 0x%x done. current cap code:0x%x\n", capid, xo_inst->cur_xo_capid);
	}

	return size;
}

static DEVICE_ATTR(xo_capid, 0664, show_xo_capid, store_xo_capid);

static ssize_t show_xo_board_offset(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "not support!\n");
}

static ssize_t store_xo_board_offset(struct device *dev, struct device_attribute *attr,
				     const char *buf, size_t size)
{
	uint32_t offset, capid;
	int ret;

	if (buf != NULL && size != 0) {
		ret = kstrtouint(buf, 0, &offset);
		if (ret) {
			pr_err("wrong format!\n");
			return size;
		}
		if (offset > 0x7f) {
			pr_err("offset should be within 7bit!\n");
			return size;
		}

		capid = XO_trim_read();
		pr_notice("original cap code: 0x%x\n", capid);

		/* check sign bit */
		if (offset & 0x40)
			capid -= (offset & 0x3F);
		else
			capid += (offset & 0x3F);

		XO_trim_write(capid);
		mdelay(10);
		xo_inst->cur_xo_capid = XO_trim_read();
		pr_notice("write cap code 0x%x done. current cap code:0x%x\n", capid, xo_inst->cur_xo_capid);
	}

	return size;
}

static DEVICE_ATTR(xo_board_offset, 0664, show_xo_board_offset, store_xo_board_offset);

static ssize_t show_xo_cmd(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "1: xo status 2: low power mode\n");
}

static ssize_t store_xo_cmd(struct device *dev, struct device_attribute *attr,
				     const char *buf, size_t size)
{
	uint32_t cmd;
	int ret;

	if (buf != NULL && size != 0) {
		ret = kstrtouint(buf, 0, &cmd);
		if (ret) {
			pr_err("wrong format!\n");
			return size;
		}

		switch (cmd) {
		case 1:
			get_xo_status();
			break;
		case 2:
			get_xo_status();
			enable_xo_low_power_mode();
			mdelay(10);
			get_xo_status();
			break;
		default:
			pr_notice("cmd not support!\n");
		}
	}

	return size;
}

static DEVICE_ATTR(xo_cmd, 0664, show_xo_cmd, store_xo_cmd);

/* for SPM driver to get cap code at suspend */
uint32_t mt_xo_get_current_capid(void)
{
	return xo_inst->cur_xo_capid;
}
EXPORT_SYMBOL(mt_xo_get_current_capid);

static int mt_xo_dts_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct resource *res;

	xo_inst = devm_kzalloc(&pdev->dev, sizeof(*xo_inst), GFP_KERNEL);
	if (!xo_inst)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xo_inst->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(xo_inst->base))
		return PTR_ERR(xo_inst->base);

	xo_inst->dev = &pdev->dev;
	platform_set_drvdata(pdev, xo_inst);

	retval = device_create_file(&pdev->dev, &dev_attr_xo_capid);
	if (retval != 0)
		dev_dbg(&pdev->dev, "fail to create file: %d\n", retval);

	retval = device_create_file(&pdev->dev, &dev_attr_xo_cmd);
	if (retval != 0)
		dev_dbg(&pdev->dev, "fail to create cmd file: %d\n", retval);

	retval = device_create_file(&pdev->dev, &dev_attr_xo_board_offset);
	if (retval != 0)
		dev_dbg(&pdev->dev, "fail to create offset file: %d\n", retval);

	xo_inst->bsi_clk = devm_clk_get(&pdev->dev, "bsi");
	if (IS_ERR(xo_inst->bsi_clk)) {
		dev_err(&pdev->dev, "fail to get bsi clock: %ld\n", PTR_ERR(xo_inst->bsi_clk));
		return PTR_ERR(xo_inst->bsi_clk);
	}

	xo_inst->rg_bsi_clk = devm_clk_get(&pdev->dev, "rgbsi");
	if (IS_ERR(xo_inst->rg_bsi_clk)) {
		dev_err(&pdev->dev, "fail to get rgbsi clock: %ld\n", PTR_ERR(xo_inst->rg_bsi_clk));
		return PTR_ERR(xo_inst->rg_bsi_clk);
	}

	retval = clk_prepare_enable(xo_inst->bsi_clk);
	if (retval) {
		dev_err(&pdev->dev, "fail to enable bsi clock: %d\n", retval);
		return retval;
	}

	retval = clk_prepare_enable(xo_inst->rg_bsi_clk);
	if (retval) {
		dev_err(&pdev->dev, "fail to enable rgbsi clock: %d\n", retval);
		return retval;
	}

	xo_inst->cur_xo_capid = XO_trim_read();
	pr_notice("[xo] current cap code: 0x%x\n", xo_inst->cur_xo_capid);

	return retval;
}

static int mt_xo_dts_remove(struct platform_device *pdev)
{
	clk_disable_unprepare(xo_inst->rg_bsi_clk);
	clk_disable_unprepare(xo_inst->bsi_clk);
	return 0;
}

static struct platform_driver mt_xo_driver = {
	.remove		= mt_xo_dts_remove,
	.probe		= mt_xo_dts_probe,
	.driver		= {
		.name	= "mt_dts_xo",
		.of_match_table = apxo_of_ids,
	},
};

static int __init mt_xo_init(void)
{
	int ret;

	ret = platform_driver_register(&mt_xo_driver);

	return ret;
}

module_init(mt_xo_init);

static void __exit mt_xo_exit(void)
{
	platform_driver_unregister(&mt_xo_driver);
}
module_exit(mt_xo_exit);

