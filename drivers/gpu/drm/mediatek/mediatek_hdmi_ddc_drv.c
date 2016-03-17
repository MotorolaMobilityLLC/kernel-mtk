/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>
#include <linux/regmap.h>
#include <linux/of_platform.h>
#include "mediatek_hdmi_display_core.h"

#define SIF1_CLOK			(288)
#define DDC_DDCMCTL0		(0x0)
#define DDCM_ODRAIN			BIT(31)
#define DDCM_CLK_DIV_OFFSET	(16)
#define DDCM_CLK_DIV_MASK	(0xfff << 16)
#define DDCM_CS_STATUS		BIT(4)
#define DDCM_SCL_STATE		BIT(3)
#define DDCM_SDA_STATE		BIT(2)
#define DDCM_SM0EN			BIT(1)
#define DDCM_SCL_STRECH		BIT(0)
#define DDC_DDCMCTL1		(0x4)
#define DDCM_ACK_OFFSET	(16)
#define DDCM_ACK_MASK		(0xff << 16)
#define DDCM_PGLEN_OFFSET	(8)
#define DDCM_PGLEN_MASK	(0x7 << 8)
#define DDCM_SIF_MODE_OFFSET	(4)
#define DDCM_SIF_MODE_MASK	(0x7 << 4)
#define DDCM_START		(0x1)
#define DDCM_WRITE_DATA	(0x2)
#define DDCM_STOP		(0x3)
#define DDCM_READ_DATA_NO_ACK	(0x4)
#define DDCM_READ_DATA_ACK	(0x5)
#define DDCM_TRI		BIT(0)
#define DDC_DDCMD0		(0x8)
#define DDCM_DATA3		(0xff << 24)
#define DDCM_DATA2		(0xff << 16)
#define DDCM_DATA1		(0xff << 8)
#define DDCM_DATA0		(0xff << 0)
#define DDC_DDCMD1		(0xc)
#define DDCM_DATA7		(0xff << 24)
#define DDCM_DATA6		(0xff << 16)
#define DDCM_DATA5		(0xff << 8)
#define DDCM_DATA4		(0xff << 0)

struct mediatek_hdmi_i2c {
	struct i2c_adapter adap;
	struct clk *clk;
	void __iomem *regs;
};

#define SIF_READ32(addr)        readl(addr)
#define SIF_WRITE32(addr, val)  writel(val, addr)

#define SIF_SET_BIT(addr, val) \
	SIF_WRITE32((addr), (SIF_READ32(addr) | (val)))
#define SIF_CLR_BIT(addr, val) \
	SIF_WRITE32((addr), (SIF_READ32(addr) & (~(val))))

#define IS_SIF_BIT(addr, val)   ((SIF_READ32(addr) & (val)) == (val))

#define SIF_WRITE_MASK(addr, mask, offset, val) \
	SIF_WRITE32(addr, ((SIF_READ32(addr) & (~(mask))) |\
	(((val) << (offset)) & (mask))))
#define SIF_READ_MASK(addr, mask, offset) \
	((SIF_READ32(addr) & (mask)) >> (offset))

static void ddcm_triger_mode(struct mediatek_hdmi_i2c *i2c, int mode)
{
	int timeout = 20 * 1000;

	SIF_WRITE_MASK(i2c->regs + DDC_DDCMCTL1,
		       DDCM_SIF_MODE_MASK,
		       DDCM_SIF_MODE_OFFSET, mode);
	SIF_SET_BIT(i2c->regs + DDC_DDCMCTL1, DDCM_TRI);
	while (IS_SIF_BIT(i2c->regs + DDC_DDCMCTL1, DDCM_TRI)) {
		timeout -= 2;
		udelay(2);
		if (timeout <= 0)
			break;
	}
}

static int mtk_hdmi_i2c_xfer(struct i2c_adapter *adapter,
			     struct i2c_msg *msgs, int num)
{
	int i = 0;

	u32 ack = 0;
	u32 processed_msg_count = 0;
	struct mediatek_hdmi_i2c *i2c = NULL;

	i2c = adapter->algo_data;
	if (!i2c) {
		mtk_hdmi_err("invalied arguments\n");
		return -EINVAL;
	}

	SIF_SET_BIT((i2c->regs + DDC_DDCMCTL0), DDCM_SCL_STRECH);
	SIF_SET_BIT((i2c->regs + DDC_DDCMCTL0), DDCM_SM0EN);
	SIF_CLR_BIT((i2c->regs + DDC_DDCMCTL0), DDCM_ODRAIN);

	if (IS_SIF_BIT((i2c->regs + DDC_DDCMCTL1), DDCM_TRI)) {
		mtk_hdmi_err("ddc line is busy!\n");
		return -EBUSY;
	}

	SIF_WRITE_MASK((i2c->regs + DDC_DDCMCTL0),
		       DDCM_CLK_DIV_MASK, DDCM_CLK_DIV_OFFSET, SIF1_CLOK);

	for (i = 0; i < num; i++) {
		struct i2c_msg *p = &msgs[i];

		mtk_hdmi_info("i2c msg, adr:0x%x, flags:%d, len :0x%x\n",
			      p->addr, p->flags, p->len);

		if (p->flags & I2C_M_RD) {
			u32 remain_count, ack_count, ack_final, read_count,
			    temp_count;
			u32 index = 0;

			ddcm_triger_mode(i2c, DDCM_START);
			SIF_WRITE_MASK((i2c->regs + DDC_DDCMD0),
				       0xff, 0, (p->addr << 1) | 0x01);
			SIF_WRITE_MASK((i2c->regs + DDC_DDCMCTL1),
				       DDCM_PGLEN_MASK,
				       DDCM_PGLEN_OFFSET, 0x00);
			ddcm_triger_mode(i2c, DDCM_WRITE_DATA);
			ack = SIF_READ_MASK((i2c->regs + DDC_DDCMCTL1),
					    DDCM_ACK_MASK, DDCM_ACK_OFFSET);
			mtk_hdmi_info("ack = 0x%x\n", ack);
			if (ack != 0x01) {
				mtk_hdmi_err("i2c ack err!\n");
				goto ddc_master_read_end;
			}

			remain_count = p->len;
			ack_count = (p->len - 1) / 8;
			ack_final = 0;

			while (remain_count > 0) {
				if (ack_count > 0) {
					read_count = 8;
					ack_final = 0;
					ack_count--;
				} else {
					read_count = remain_count;
					ack_final = 1;
				}

				SIF_WRITE_MASK((i2c->regs + DDC_DDCMCTL1),
					       DDCM_PGLEN_MASK,
					       DDCM_PGLEN_OFFSET,
					       (read_count - 1));
				ddcm_triger_mode(i2c, (ack_final ==
					       1) ? DDCM_READ_DATA_NO_ACK :
					      DDCM_READ_DATA_ACK);

				ack =
				SIF_READ_MASK((i2c->regs + DDC_DDCMCTL1),
					      DDCM_ACK_MASK, DDCM_ACK_OFFSET);
				temp_count = 0;
				while (((ack & (1 << temp_count)) != 0) &&
				       (temp_count < 8))
					temp_count++;
				if (((ack_final == 1) &&
				     (temp_count != (read_count - 1))) ||
				     ((ack_final == 0) &&
				     (temp_count != read_count))) {
					mtk_hdmi_err
					    (" Address NACK! ACK(0x%x)\n", ack);
					break;
				}

				switch (read_count) {
				case 8:
					p->buf[index + 7] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD1),
						      DDCM_DATA7, 24);
				case 7:
					p->buf[index + 6] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD1),
						      DDCM_DATA6, 16);
				case 6:
					p->buf[index + 5] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD1),
						      DDCM_DATA5, 8);
				case 5:
					p->buf[index + 4] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD1),
						      DDCM_DATA4, 0);
				case 4:
					p->buf[index + 3] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD0),
						      DDCM_DATA3, 24);
				case 3:
					p->buf[index + 2] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD0),
						      DDCM_DATA2, 16);
				case 2:
					p->buf[index + 1] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD0),
						      DDCM_DATA1, 8);
				case 1:
					p->buf[index + 0] =
					SIF_READ_MASK((i2c->regs + DDC_DDCMD0),
						      DDCM_DATA0, 0);
				default:
					break;
				}

				remain_count -= read_count;
				index += read_count;
			}

			processed_msg_count++;

		} else {
			ddcm_triger_mode(i2c, DDCM_START);
			SIF_WRITE_MASK((i2c->regs + DDC_DDCMD0),
				       DDCM_DATA0, 0,  p->addr << 1);
			SIF_WRITE_MASK((i2c->regs + DDC_DDCMD0),
				       DDCM_DATA1, 8,  p->buf[0]);
			SIF_WRITE_MASK((i2c->regs + DDC_DDCMCTL1),
				       DDCM_PGLEN_MASK, DDCM_PGLEN_OFFSET, 0x1);
			ddcm_triger_mode(i2c, DDCM_WRITE_DATA);

			ack = SIF_READ_MASK((i2c->regs + DDC_DDCMCTL1),
					    DDCM_ACK_MASK, DDCM_ACK_OFFSET);
			mtk_hdmi_info(" ack = %d\n", ack);

			if (ack != 0x03) {
				mtk_hdmi_err("i2c ack err!\n");
				goto ddc_master_read_end;
			}
			processed_msg_count++;
		}
	}

	ddcm_triger_mode(i2c, DDCM_STOP);

	return processed_msg_count;

ddc_master_read_end:
	ddcm_triger_mode(i2c, DDCM_STOP);
	mtk_hdmi_err(" ddc failed!\n");
	return -EFAULT;
}

static u32 mtk_hdmi_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm mediatek_hdmi_i2c_algorithm = {
	.master_xfer = mtk_hdmi_i2c_xfer,
	.functionality = mtk_hdmi_i2c_func,
};

static int mtk_hdmi_i2c_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mediatek_hdmi_i2c *i2c = NULL;
	struct resource *mem;
	int ret;

	i2c =
	    devm_kzalloc(&pdev->dev, sizeof(struct mediatek_hdmi_i2c),
			 GFP_KERNEL);
	if (!i2c) {
		mtk_hdmi_err("no enough memory\n");
		ret = -ENOMEM;
		goto errcode;
	}

	i2c->clk = of_clk_get_by_name(np, "ddc-i2c");
	if (IS_ERR(i2c->clk)) {
		mtk_hdmi_err(" get ddc_clk failed : %p ,\n",
			     i2c->clk);
		goto errcode;
	}

	if (clk_prepare_enable(i2c->clk)) {
		mtk_hdmi_err("enable ddc clk failed!\n");
		goto errcode;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		mtk_hdmi_err("get memory source fail!\n");
		ret = -EFAULT;
		goto errcode;
	}

	i2c->regs = devm_ioremap_resource(&pdev->dev, mem);

	strlcpy(i2c->adap.name, "mediatek-hdmi-i2c", sizeof(i2c->adap.name));
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &mediatek_hdmi_i2c_algorithm;
	i2c->adap.retries = 3;
	i2c->adap.dev.of_node = np;
	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;

	ret = i2c_add_adapter(&i2c->adap);
	if (ret < 0) {
		mtk_hdmi_err("failed to add bus to i2c core\n");
		goto errcode;
	}

	platform_set_drvdata(pdev, i2c);

	mtk_hdmi_info("i2c->adap: %p\n", &i2c->adap);
	mtk_hdmi_info("i2c->clk: %p\n", i2c->clk);
	mtk_hdmi_info("physical adr: 0x%llx, end: 0x%llx\n", mem->start,
		      mem->end);

	return 0;

errcode:
	clk_disable_unprepare(i2c->clk);
	i2c_del_adapter(&i2c->adap);
	devm_kfree(&pdev->dev, i2c);
	return ret;
}

static int mtk_hdmi_i2c_remove(struct platform_device *pdev)
{
	struct mediatek_hdmi_i2c *i2c = platform_get_drvdata(pdev);

	clk_disable_unprepare(i2c->clk);
	i2c_del_adapter(&i2c->adap);
	devm_kfree(&pdev->dev, i2c);

	return 0;
}

static const struct of_device_id mediatek_hdmi_i2c_match[] = {
	{
	 .compatible = "mediatek,8173-hdmi-ddc",
	 }, {},
};

struct platform_driver mediatek_hdmi_ddc_driver = {
	.probe = mtk_hdmi_i2c_probe,
	.remove = mtk_hdmi_i2c_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "mediatek-hdmi-ddc",
		   .of_match_table = mediatek_hdmi_i2c_match,
		   },
};

MODULE_AUTHOR("Jie Qiu <jie.qiu@mediatek.com>");
MODULE_DESCRIPTION("MediaTek HDMI i2c Driver");
MODULE_LICENSE("GPL");
