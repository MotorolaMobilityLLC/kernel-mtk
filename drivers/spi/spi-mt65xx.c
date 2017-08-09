/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Leilk Liu <leilk.liu@mediatek.com>
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/spi/spi.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>

#define SPI_CFG0_REG                      0x0000
#define SPI_CFG1_REG                      0x0004
#define SPI_TX_SRC_REG                    0x0008
#define SPI_RX_DST_REG                    0x000c
#define SPI_CMD_REG                       0x0018
#define SPI_STATUS0_REG                   0x001c
#define SPI_PAD_SEL_REG                   0x0024

#define SPI_CFG0_SCK_HIGH_OFFSET          0
#define SPI_CFG0_SCK_LOW_OFFSET           8
#define SPI_CFG0_CS_HOLD_OFFSET           16
#define SPI_CFG0_CS_SETUP_OFFSET          24

#define SPI_CFG0_SCK_HIGH_MASK            0xff
#define SPI_CFG0_SCK_LOW_MASK             0xff00
#define SPI_CFG0_CS_HOLD_MASK             0xff0000
#define SPI_CFG0_CS_SETUP_MASK            0xff000000

#define SPI_CFG1_CS_IDLE_OFFSET           0
#define SPI_CFG1_PACKET_LOOP_OFFSET       8
#define SPI_CFG1_PACKET_LENGTH_OFFSET     16
#define SPI_CFG1_GET_TICK_DLY_OFFSET      30

#define SPI_CFG1_CS_IDLE_MASK             0xff
#define SPI_CFG1_PACKET_LOOP_MASK         0xff00
#define SPI_CFG1_PACKET_LENGTH_MASK       0x3ff0000
#define SPI_CFG1_GET_TICK_DLY_MASK        0xc0000000

#define SPI_CMD_ACT_OFFSET                0
#define SPI_CMD_RESUME_OFFSET             1
#define SPI_CMD_RST_OFFSET                2
#define SPI_CMD_PAUSE_EN_OFFSET           4
#define SPI_CMD_DEASSERT_OFFSET           5
#define SPI_CMD_CPHA_OFFSET               8
#define SPI_CMD_CPOL_OFFSET               9
#define SPI_CMD_RX_DMA_OFFSET             10
#define SPI_CMD_TX_DMA_OFFSET             11
#define SPI_CMD_TXMSBF_OFFSET             12
#define SPI_CMD_RXMSBF_OFFSET             13
#define SPI_CMD_RX_ENDIAN_OFFSET          14
#define SPI_CMD_TX_ENDIAN_OFFSET          15
#define SPI_CMD_FINISH_IE_OFFSET          16
#define SPI_CMD_PAUSE_IE_OFFSET           17

#define SPI_CMD_RST_MASK                  0x4
#define SPI_CMD_PAUSE_EN_MASK             0x10
#define SPI_CMD_DEASSERT_MASK             0x20
#define SPI_CMD_CPHA_MASK                 0x100
#define SPI_CMD_CPOL_MASK                 0x200
#define SPI_CMD_RX_DMA_MASK               0x400
#define SPI_CMD_TX_DMA_MASK               0x800
#define SPI_CMD_TXMSBF_MASK               0x1000
#define SPI_CMD_RXMSBF_MASK               0x2000
#define SPI_CMD_RX_ENDIAN_MASK            0x4000
#define SPI_CMD_TX_ENDIAN_MASK            0x8000
#define SPI_CMD_FINISH_IE_MASK            0x10000

#define COMPAT_MT6589			(0x1 << 0)
#define COMPAT_MT8135			(0x1 << 1)
#define COMPAT_MT8173			(0x1 << 2)
#define COMPAT_MT2701			(0x1 << 3)

#define MT8173_MAX_PAD_SEL 3

#define MTK_SPI_IDLE 0
#define MTK_SPI_INPROGRESS 1
#define MTK_SPI_PAUSED 2

#define MTK_SPI_PACKET_SIZE 1024
#define MTK_SPI_MAX_PACKET_LOOP 256

struct mtk_chip_config {
	u32 setuptime;
	u32 holdtime;
	u32 high_time;
	u32 low_time;
	u32 cs_idletime;
	u32 tx_mlsb;
	u32 rx_mlsb;
	u32 tx_endian;
	u32 rx_endian;
	u32 pause;
	u32 finish_intr;
	u32 deassert;
	u32 tckdly;
};

struct mtk_spi_ddata {
	struct device *dev;
	struct spi_master *master;
	void __iomem *base;
	u32 irq;
	u32 state;
	u32 platform_compat;
	u32 pad_sel;
	struct clk *clk;

	const u8 *tx_buf;
	u8 *rx_buf;
	u32 tx_len, rx_len;
	struct completion done;
};

/*
 * A piece of default chip info unless the platform
 * supplies it.
 */
static const struct mtk_chip_config mtk_default_chip_info = {
	.setuptime = 50,
	.holdtime = 50,
	.high_time = 25,
	.low_time = 25,
	.cs_idletime = 50,
	.rx_mlsb = 1,
	.tx_mlsb = 1,
	.tx_endian = 0,
	.rx_endian = 0,
	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.tckdly = 0,
};

static const struct of_device_id mtk_spi_of_match[] = {
	{ .compatible = "mediatek,mt6589-spi", .data = (void *)COMPAT_MT6589},
	{ .compatible = "mediatek,mt8135-spi", .data = (void *)COMPAT_MT8135},
	{ .compatible = "mediatek,mt8173-spi", .data = (void *)COMPAT_MT8173},
	{ .compatible = "mediatek,mt2701-spi", .data = (void *)COMPAT_MT2701},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_spi_of_match);

static void mtk_spi_reset(struct mtk_spi_ddata *mdata)
{
	u32 reg_val;

	/* set the software reset bit in SPI_CMD_REG. */
	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RST_MASK;
	reg_val |= 1 << SPI_CMD_RST_OFFSET;
	writel(reg_val, mdata->base + SPI_CMD_REG);
	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RST_MASK;
	writel(reg_val, mdata->base + SPI_CMD_REG);
}

static int mtk_spi_config(struct mtk_spi_ddata *mdata,
			  struct mtk_chip_config *chip_config)
{
	u32 reg_val;

	/* set the timing */
	reg_val = readl(mdata->base + SPI_CFG0_REG);
	reg_val &= ~(SPI_CFG0_SCK_HIGH_MASK | SPI_CFG0_SCK_LOW_MASK);
	reg_val &= ~(SPI_CFG0_CS_HOLD_MASK | SPI_CFG0_CS_SETUP_MASK);
	reg_val |= ((chip_config->high_time - 1) << SPI_CFG0_SCK_HIGH_OFFSET);
	reg_val |= ((chip_config->low_time - 1) << SPI_CFG0_SCK_LOW_OFFSET);
	reg_val |= ((chip_config->holdtime - 1) << SPI_CFG0_CS_HOLD_OFFSET);
	reg_val |= ((chip_config->setuptime - 1) << SPI_CFG0_CS_SETUP_OFFSET);
	writel(reg_val, mdata->base + SPI_CFG0_REG);

	reg_val = readl(mdata->base + SPI_CFG1_REG);
	reg_val &= ~SPI_CFG1_CS_IDLE_MASK;
	reg_val |= ((chip_config->cs_idletime - 1) << SPI_CFG1_CS_IDLE_OFFSET);
	reg_val &= ~SPI_CFG1_GET_TICK_DLY_MASK;
	reg_val |= ((chip_config->tckdly) << SPI_CFG1_GET_TICK_DLY_OFFSET);
	writel(reg_val, mdata->base + SPI_CFG1_REG);

	/* set the mlsbx and mlsbtx */
	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~(SPI_CMD_TX_ENDIAN_MASK | SPI_CMD_RX_ENDIAN_MASK);
	reg_val &= ~(SPI_CMD_TXMSBF_MASK | SPI_CMD_RXMSBF_MASK);
	reg_val |= (chip_config->tx_mlsb << SPI_CMD_TXMSBF_OFFSET);
	reg_val |= (chip_config->rx_mlsb << SPI_CMD_RXMSBF_OFFSET);
	reg_val |= (chip_config->tx_endian << SPI_CMD_TX_ENDIAN_OFFSET);
	reg_val |= (chip_config->rx_endian << SPI_CMD_RX_ENDIAN_OFFSET);
	writel(reg_val, mdata->base + SPI_CMD_REG);

	/* set finish and pause interrupt always enable */
	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~SPI_CMD_FINISH_IE_MASK;
	reg_val |= (chip_config->finish_intr << SPI_CMD_FINISH_IE_OFFSET);
	reg_val |= (1 << SPI_CMD_PAUSE_IE_OFFSET);
	writel(reg_val, mdata->base + SPI_CMD_REG);

	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val |= 1 << SPI_CMD_TX_DMA_OFFSET;
	reg_val |= 1 << SPI_CMD_RX_DMA_OFFSET;
	writel(reg_val, mdata->base + SPI_CMD_REG);

	/* set deassert mode */
	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~SPI_CMD_DEASSERT_MASK;
	reg_val |= (chip_config->deassert << SPI_CMD_DEASSERT_OFFSET);
	writel(reg_val, mdata->base + SPI_CMD_REG);

	/* pad select */
	if (mdata->platform_compat & COMPAT_MT8173)
		writel(mdata->pad_sel, mdata->base + SPI_PAD_SEL_REG);

	return 0;
}

static int mtk_spi_prepare_message(struct spi_master *master,
				   struct spi_message *msg)
{
	u32 reg_val;
	u8 cpha, cpol;
	struct spi_transfer *trans;
	struct mtk_chip_config *chip_config;
	struct spi_device *spi = msg->spi;
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(master);

	if (spi->mode & SPI_CPHA)
		cpha = 1;
	else
		cpha = 0;

	if (spi->mode & SPI_CPOL)
		cpol = 1;
	else
		cpol = 0;

	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~(SPI_CMD_CPHA_MASK | SPI_CMD_CPOL_MASK);
	reg_val |= (cpha << SPI_CMD_CPHA_OFFSET);
	reg_val |= (cpol << SPI_CMD_CPOL_OFFSET);
	writel(reg_val, mdata->base + SPI_CMD_REG);

	chip_config = (struct mtk_chip_config *)spi->controller_data;
	if (!chip_config) {
		chip_config = (void *)&mtk_default_chip_info;
		spi->controller_data = chip_config;
	}

	trans = list_first_entry(&msg->transfers, struct spi_transfer,
				 transfer_list);
	if (trans->cs_change == 0) {
		mdata->state = MTK_SPI_IDLE;
		mtk_spi_reset(mdata);
	}

	mtk_spi_config(mdata, chip_config);

	return 0;
}

static void mtk_spi_set_cs(struct spi_device *spi, bool enable)
{
	u32 reg_val;
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(spi->master);

	reg_val = readl(mdata->base + SPI_CMD_REG);
	if (!enable)
		reg_val |= 1 << SPI_CMD_PAUSE_EN_OFFSET;
	else
		reg_val &= ~SPI_CMD_PAUSE_EN_MASK;
	writel(reg_val, mdata->base + SPI_CMD_REG);
}

static int mtk_spi_setup_packet(struct mtk_spi_ddata *mdata,
				struct spi_transfer *xfer)
{
	u32 packet_size, packet_loop, reg_val;

	packet_size = min_t(unsigned, xfer->len, MTK_SPI_PACKET_SIZE);

	/* mtk hw has the restriction that xfer len must be a multiple of 1024,
	 * when it is greater than 1024bytes.
	 */
	if (xfer->len % packet_size) {
		dev_err(mdata->dev, "ERROR!The lens must be a multiple of %d, your len %d\n",
			MTK_SPI_PACKET_SIZE, xfer->len);
		return -EINVAL;
	}

	packet_loop = xfer->len / packet_size;
	if (packet_loop > MTK_SPI_MAX_PACKET_LOOP) {
		dev_err(mdata->dev, "packet_loop(%d) error\n", packet_loop);
		return -EINVAL;
	}

	reg_val = readl(mdata->base + SPI_CFG1_REG);
	reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK + SPI_CFG1_PACKET_LOOP_MASK);
	reg_val |= (packet_size - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET;
	reg_val |= (packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET;
	writel(reg_val, mdata->base + SPI_CFG1_REG);

	return 0;
}

static int mtk_spi_transfer_one(struct spi_master *master,
				struct spi_device *spi,
				struct spi_transfer *xfer)
{
	int cmd = 0, ret = 0;
	unsigned int tx_sgl_len = 0, rx_sgl_len = 0;
	struct scatterlist *tx_sgl = NULL, *rx_sgl = NULL;
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(master);

	/* Here is mt8173 HW issue: RX must enable TX, then TX transfer
	 * dummy data; TX don't need to enable RX. so enable TX dma for
	 * RX to workaround.
	 */
	cmd = readl(mdata->base + SPI_CMD_REG);
	if (xfer->tx_buf || (mdata->platform_compat & COMPAT_MT8173))
		cmd |= 1 << SPI_CMD_TX_DMA_OFFSET;
	if (xfer->rx_buf)
		cmd |= 1 << SPI_CMD_RX_DMA_OFFSET;
	writel(cmd, mdata->base + SPI_CMD_REG);

	if (xfer->tx_buf)
		tx_sgl = xfer->tx_sg.sgl;
	if (xfer->rx_buf)
		rx_sgl = xfer->rx_sg.sgl;

	while (rx_sgl || tx_sgl) {
		if (tx_sgl && (tx_sgl_len == 0)) {
			xfer->tx_dma = sg_dma_address(tx_sgl);
			tx_sgl_len = sg_dma_len(tx_sgl);
		}
		if (rx_sgl && (rx_sgl_len == 0)) {
			xfer->rx_dma = sg_dma_address(rx_sgl);
			rx_sgl_len = sg_dma_len(rx_sgl);
		}

		if (tx_sgl && rx_sgl)
			xfer->len = min_t(unsigned int, tx_sgl_len, rx_sgl_len);
		else
			xfer->len = max_t(unsigned int, tx_sgl_len, rx_sgl_len);

		ret = mtk_spi_setup_packet(mdata, xfer);
		if (ret != 0)
			return ret;

		/* set up the DMA bus address */
		writel(cpu_to_le32(xfer->tx_dma), mdata->base + SPI_TX_SRC_REG);
		writel(cpu_to_le32(xfer->rx_dma), mdata->base + SPI_RX_DST_REG);

		/* trigger to transfer */
		if (mdata->state == MTK_SPI_IDLE) {
			cmd = readl(mdata->base + SPI_CMD_REG);
			cmd |= 1 << SPI_CMD_ACT_OFFSET;
			writel(cmd, mdata->base + SPI_CMD_REG);
		} else if (mdata->state == MTK_SPI_PAUSED) {
			cmd = readl(mdata->base + SPI_CMD_REG);
			cmd |= 1 << SPI_CMD_RESUME_OFFSET;
			writel(cmd, mdata->base + SPI_CMD_REG);
		} else {
			mdata->state = MTK_SPI_INPROGRESS;
		}

		wait_for_completion(&mdata->done);

		if (tx_sgl && rx_sgl) {
			if (tx_sgl_len > rx_sgl_len) {
				xfer->tx_dma += rx_sgl_len;
				tx_sgl_len -= rx_sgl_len;
				rx_sgl_len = 0;
				rx_sgl = sg_next(rx_sgl);
				continue;
			} else if (tx_sgl_len < rx_sgl_len) {
				xfer->rx_dma += tx_sgl_len;
				rx_sgl_len -= tx_sgl_len;
				tx_sgl_len = 0;
				tx_sgl = sg_next(tx_sgl);
				continue;
			}
		}

		rx_sgl_len = 0;
		tx_sgl_len = 0;

		if (rx_sgl)
			rx_sgl = sg_next(rx_sgl);
		if (tx_sgl)
			tx_sgl = sg_next(tx_sgl);
	}

	/* spi disable dma */
	cmd = readl(mdata->base + SPI_CMD_REG);
	cmd &= ~SPI_CMD_TX_DMA_MASK;
	cmd &= ~SPI_CMD_RX_DMA_MASK;
	writel(cmd, mdata->base + SPI_CMD_REG);

	return ret;
}

static bool mtk_spi_can_dma(struct spi_master *master,
			    struct spi_device *spi,
			    struct spi_transfer *xfer)
{
	return true;
}

static irqreturn_t mtk_spi_interrupt(int irq, void *dev_id)
{
	struct mtk_spi_ddata *mdata = dev_id;
	u32 reg_val;

	reg_val = readl(mdata->base + SPI_STATUS0_REG);
	if (reg_val & 0x2)
		mdata->state = MTK_SPI_PAUSED;
	else
		mdata->state = MTK_SPI_IDLE;

	complete(&mdata->done);

	return IRQ_HANDLED;
}

static int mtk_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct mtk_spi_ddata *mdata;
	const struct of_device_id *of_id;
	struct resource *res;
	int	ret;

	master = spi_alloc_master(&pdev->dev, sizeof(struct mtk_spi_ddata));
	if (!master) {
		dev_err(&pdev->dev, "failed to alloc spi master\n");
		return -ENOMEM;
	}

	if (of_property_read_u32(pdev->dev.of_node, "cell-index", &pdev->id)) {
		dev_err(&pdev->dev, "get cell-index failed\n");
		return -ENODEV;
	}
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	master->auto_runtime_pm = true;
	master->dev.of_node = pdev->dev.of_node;
	master->bus_num = pdev->id;
	master->num_chipselect = 2;
	master->mode_bits = SPI_CPOL | SPI_CPHA;

	mdata = spi_master_get_devdata(master);
	memset(mdata, 0, sizeof(struct mtk_spi_ddata));

	mdata->master = master;
	mdata->dev = &pdev->dev;

	master->set_cs = mtk_spi_set_cs;
	master->prepare_message = mtk_spi_prepare_message;
	master->transfer_one = mtk_spi_transfer_one;
	master->can_dma = mtk_spi_can_dma;

	of_id = of_match_node(mtk_spi_of_match, pdev->dev.of_node);
	if (!of_id) {
		dev_err(&pdev->dev, "failed to probe of_node\n");
		ret = -EINVAL;
		goto err;
	}

	mdata->platform_compat = (unsigned long)of_id->data;

	if (mdata->platform_compat & COMPAT_MT8173) {
		ret = of_property_read_u32(pdev->dev.of_node, "pad-select",
					   &mdata->pad_sel);
		if (ret) {
			dev_err(&pdev->dev, "failed to read pad select: %d\n",
				ret);
			goto err;
		}

		if (mdata->pad_sel > MT8173_MAX_PAD_SEL) {
			dev_err(&pdev->dev, "wrong pad-select: %u\n",
				mdata->pad_sel);
			ret = -EINVAL;
			goto err;
		}
	}

	platform_set_drvdata(pdev, master);
	init_completion(&mdata->done);

	mdata->clk = devm_clk_get(&pdev->dev, "main");
	if (IS_ERR(mdata->clk)) {
		ret = PTR_ERR(mdata->clk);
		dev_err(&pdev->dev, "failed to get clock: %d\n", ret);
		goto err;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "failed to determine base address\n");
		goto err;
	}

	mdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mdata->base)) {
		ret = PTR_ERR(mdata->base);
		goto err;
	}

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get irq (%d)\n", ret);
		goto err;
	}

	mdata->irq = ret;

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	ret = clk_prepare_enable(mdata->clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable clock (%d)\n", ret);
		goto err;
	}

	ret = devm_request_irq(&pdev->dev, mdata->irq, mtk_spi_interrupt,
			       IRQF_TRIGGER_NONE, dev_name(&pdev->dev), mdata);
	if (ret) {
		dev_err(&pdev->dev, "failed to register irq (%d)\n", ret);
		goto err_disable_clk;
	}

	ret = devm_spi_register_master(&pdev->dev, master);
	if (ret) {
		dev_err(&pdev->dev, "failed to register master (%d)\n", ret);
err_disable_clk:
		clk_disable_unprepare(mdata->clk);
err:
		spi_master_put(master);
	}

	return ret;
}

static int mtk_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(master);

	pm_runtime_disable(&pdev->dev);

	mtk_spi_reset(mdata);
	clk_disable_unprepare(mdata->clk);
	spi_master_put(master);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mtk_spi_suspend(struct device *dev)
{
	int ret = 0;
	struct spi_master *master = dev_get_drvdata(dev);
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(master);

	ret = spi_master_suspend(master);
	if (ret)
		return ret;

	if (!pm_runtime_suspended(dev))
		clk_disable_unprepare(mdata->clk);

	return ret;
}

static int mtk_spi_resume(struct device *dev)
{
	int ret = 0;
	struct spi_master *master = dev_get_drvdata(dev);
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(master);

	if (!pm_runtime_suspended(dev)) {
		ret = clk_prepare_enable(mdata->clk);
		if (ret < 0)
			return ret;
	}

	return spi_master_resume(master);
}
#endif /* CONFIG_PM_SLEEP */

#ifdef CONFIG_PM
static int mtk_spi_runtime_suspend(struct device *dev)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(master);

	clk_disable_unprepare(mdata->clk);

	return 0;
}

static int mtk_spi_runtime_resume(struct device *dev)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct mtk_spi_ddata *mdata = spi_master_get_devdata(master);

	return clk_prepare_enable(mdata->clk);
}
#endif /* CONFIG_PM */

static const struct dev_pm_ops mtk_spi_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_spi_suspend, mtk_spi_resume)
	SET_RUNTIME_PM_OPS(mtk_spi_runtime_suspend,
			   mtk_spi_runtime_resume, NULL)
};

struct platform_driver mtk_spi_driver = {
	.driver = {
		.name = "mtk-spi",
		.pm	= &mtk_spi_pm,
		.of_match_table = mtk_spi_of_match,
	},
	.probe = mtk_spi_probe,
	.remove = mtk_spi_remove,
};

module_platform_driver(mtk_spi_driver);

MODULE_DESCRIPTION("MTK SPI Controller driver");
MODULE_AUTHOR("Leilk Liu <leilk.liu@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform: mtk_spi");
