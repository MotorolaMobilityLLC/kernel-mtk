/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Xudong.chen <xudong.chen@mediatek.com>
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
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>

#define I2C_HS_NACKERR			(1 << 2)
#define I2C_ACKERR			(1 << 1)
#define I2C_TRANSAC_COMP		(1 << 0)
#define I2C_TRANSAC_START		(1 << 0)
#define I2C_TIMING_STEP_DIV_MASK	(0x3f << 0)
#define I2C_TIMING_SAMPLE_COUNT_MASK	(0x7 << 0)
#define I2C_TIMING_SAMPLE_DIV_MASK	(0x7 << 8)
#define I2C_TIMING_DATA_READ_MASK	(0x7 << 12)
#define I2C_DCM_DISABLE			0x0000
#define I2C_IO_CONFIG_OPEN_DRAIN	0x0003
#define I2C_IO_CONFIG_PUSH_PULL		0x0000
#define I2C_SOFT_RST			0x0001
#define I2C_FIFO_ADDR_CLR		0x0001
#define I2C_DELAY_LEN			0x0002
#define I2C_ST_START_CON		0x8001
#define I2C_FS_START_CON		0x1800
#define I2C_TIME_CLR_VALUE		0x0000
#define I2C_TIME_DEFAULT_VALUE		0x0003

#define I2C_DMA_CON_TX			0x0000
#define I2C_DMA_CON_RX			0x0001
#define I2C_DMA_START_EN		0x0001
#define I2C_DMA_INT_FLAG_NONE		0x0000

#define I2C_DEFAUT_SPEED		100000	/* hz */
#define MAX_FS_MODE_SPEED		400000
#define MAX_HS_MODE_SPEED		3400000
#define MAX_DMA_TRANS_SIZE		4096	/* 255 */
#define MAX_SAMPLE_CNT_DIV		8
#define MAX_STEP_CNT_DIV		64
#define MAX_HS_STEP_CNT_DIV		8

#define I2C_CONTROL_RS                  (0x1 << 1)
#define I2C_CONTROL_DMA_EN              (0x1 << 2)
#define I2C_CONTROL_CLK_EXT_EN          (0x1 << 3)
#define I2C_CONTROL_DIR_CHANGE          (0x1 << 4)
#define I2C_CONTROL_ACKERR_DET_EN       (0x1 << 5)
#define I2C_CONTROL_TRANSFER_LEN_CHANGE (0x1 << 6)
#define I2C_CONTROL_WRAPPER             (0x1 << 0)

#define I2C_DRV_NAME		"mt-i2c"

enum DMA_REGS_OFFSET {
	OFFSET_INT_FLAG = 0x0,
	OFFSET_INT_EN = 0x04,
	OFFSET_EN = 0x08,
	OFFSET_CON = 0x18,
	OFFSET_TX_MEM_ADDR = 0x1c,
	OFFSET_RX_MEM_ADDR = 0x20,
	OFFSET_TX_LEN = 0x24,
	OFFSET_RX_LEN = 0x28,
};

enum i2c_trans_st_rs {
	I2C_TRANS_STOP = 0,
	I2C_TRANS_REPEATED_START,
};

enum {
	FS_MODE,
	HS_MODE,
};

enum mt_trans_op {
	I2C_MASTER_WR = 1,
	I2C_MASTER_RD,
	I2C_MASTER_WRRD,
};

enum I2C_REGS_OFFSET {
	OFFSET_DATA_PORT = 0x0,
	OFFSET_SLAVE_ADDR = 0x04,
	OFFSET_INTR_MASK = 0x08,
	OFFSET_INTR_STAT = 0x0c,
	OFFSET_CONTROL = 0x10,
	OFFSET_TRANSFER_LEN = 0x14,
	OFFSET_TRANSAC_LEN = 0x18,
	OFFSET_DELAY_LEN = 0x1c,
	OFFSET_TIMING = 0x20,
	OFFSET_START = 0x24,
	OFFSET_EXT_CONF = 0x28,
	OFFSET_FIFO_STAT = 0x30,
	OFFSET_FIFO_THRESH = 0x34,
	OFFSET_FIFO_ADDR_CLR = 0x38,
	OFFSET_IO_CONFIG = 0x40,
	OFFSET_RSV_DEBUG = 0x44,
	OFFSET_HS = 0x48,
	OFFSET_SOFTRESET = 0x50,
	OFFSET_DCM_EN = 0x54,
	OFFSET_PATH_DIR = 0x60,
	OFFSET_DEBUGSTAT = 0x64,
	OFFSET_DEBUGCTRL = 0x68,
	OFFSET_TRANSFER_LEN_AUX = 0x6c,
};

struct mt_i2c_data {
	unsigned int clk_frequency;	/* bus speed in Hz */
	unsigned int flags;
	unsigned int clk_src_div;
};

struct i2c_dma_buf {
	u8 *vaddr;
	dma_addr_t paddr;
};

struct mt_i2c_ext {
#define I2C_HWTRIG_FLAG		0x00000001
	bool isEnable;
	bool is_hw_trig;
	u32 timing;
};

struct mt_i2c {
	struct i2c_adapter adap;	/* i2c host adapter */
	struct device *dev;
	wait_queue_head_t wait;		/* i2c transfer wait queue */
	/* set in i2c probe */
	void __iomem *base;		/* i2c base addr */
	void __iomem *pdmabase;		/* dma base address*/
	int irqnr;			/* i2c interrupt number */
	int id;
	struct i2c_dma_buf dma_buf;	/* memory alloc for DMA mode */
	struct clk *clk_main;		/* main clock for i2c bus */
	struct clk *clk_dma;		/* DMA clock for i2c via DMA */
	struct clk *clk_pmic;		/* PMIC clock for i2c from PMIC */
	bool have_pmic;			/* can use i2c pins form PMIC */
	bool have_dcm;			/* HW DCM function */
	bool use_push_pull;		/* IO config push-pull mode */
	/* set when doing the transfer */
	u16 irq_stat;			/* interrupt status */
	unsigned int speed_hz;		/* The speed in transfer */
	unsigned int clk_src_div;
	bool trans_stop;		/* i2c transfer stop */
	enum mt_trans_op op;
	u16 msg_len;
	u8 *msg_buf;			/* pointer to msg data */
	u16 msg_aux_len;		/* WRRD mode to set AUX_LEN register*/
	u16 addr;	/* 7bit slave address, without read/write bit */
	u16 timing_reg;
	u16 high_speed_reg;
	struct mutex i2c_mutex;
	struct mt_i2c_ext ext_data;
};

static inline void i2c_writew(u16 value, struct mt_i2c *i2c, u8 offset)
{
	writew(value, i2c->base + offset);
}

static inline u16 i2c_readw(struct mt_i2c *i2c, u8 offset)
{
	return readw(i2c->base + offset);
}

static inline void i2c_writel_dma(u32 value, struct mt_i2c *i2c, u8 offset)
{
	writel(value, i2c->pdmabase + offset);
}

static int mt_i2c_clock_enable(struct mt_i2c *i2c)
{
	int ret;

	ret = clk_prepare_enable(i2c->clk_dma);
	if (ret)
		return ret;
	ret = clk_prepare_enable(i2c->clk_main);
	if (ret)
		goto err_main;
	if (i2c->have_pmic) {
		ret = clk_prepare_enable(i2c->clk_pmic);
		if (ret)
			goto err_pmic;
	}
	return 0;

err_pmic:
	clk_disable_unprepare(i2c->clk_main);
err_main:
	clk_disable_unprepare(i2c->clk_dma);
	return ret;
}

static void mt_i2c_clock_disable(struct mt_i2c *i2c)
{
	if (i2c->have_pmic)
		clk_disable_unprepare(i2c->clk_pmic);
	clk_disable_unprepare(i2c->clk_main);
	clk_disable_unprepare(i2c->clk_dma);
}

static void free_i2c_dma_bufs(struct mt_i2c *i2c)
{
	dma_free_coherent(i2c->adap.dev.parent, PAGE_SIZE,
		i2c->dma_buf.vaddr, i2c->dma_buf.paddr);
}

static inline void mt_i2c_init_hw(struct mt_i2c *i2c)
{
	i2c_writew(I2C_SOFT_RST, i2c, OFFSET_SOFTRESET);
	/* Set ioconfig */
	if (i2c->use_push_pull)
		i2c_writew(I2C_IO_CONFIG_PUSH_PULL, i2c, OFFSET_IO_CONFIG);
	else
		i2c_writew(I2C_IO_CONFIG_OPEN_DRAIN, i2c, OFFSET_IO_CONFIG);
	if (i2c->have_dcm)
		i2c_writew(I2C_DCM_DISABLE, i2c, OFFSET_DCM_EN);
	i2c_writew(i2c->timing_reg, i2c, OFFSET_TIMING);
	i2c_writew(i2c->high_speed_reg, i2c, OFFSET_HS);
}

/* calculate i2c port speed */
static int i2c_set_speed(struct mt_i2c *i2c, unsigned int clk_src_in_hz)
{
	int mode;
	unsigned int khz;
	unsigned int step_cnt;
	unsigned int sample_cnt;
	unsigned int sclk;
	unsigned int hclk;
	unsigned int max_step_cnt;
	unsigned int sample_div = MAX_SAMPLE_CNT_DIV;
	unsigned int step_div;
	unsigned int min_div;
	unsigned int best_mul;
	unsigned int cnt_mul;
	unsigned int speed_hz;

	if (i2c->ext_data.isEnable && i2c->ext_data.timing)
		speed_hz = i2c->ext_data.timing;
	else
		speed_hz = i2c->speed_hz;

	if (speed_hz > MAX_HS_MODE_SPEED) {
		return -EINVAL;
	} else if (speed_hz > MAX_FS_MODE_SPEED) {
		mode = HS_MODE;
		max_step_cnt = MAX_HS_STEP_CNT_DIV;
	} else {
		mode = FS_MODE;
		max_step_cnt = MAX_STEP_CNT_DIV;
	}
	step_div = max_step_cnt;

	/* Find the best combination */
	khz = speed_hz / 1000;
	hclk = clk_src_in_hz / 1000;
	min_div = ((hclk >> 1) + khz - 1) / khz;
	best_mul = MAX_SAMPLE_CNT_DIV * max_step_cnt;
	for (sample_cnt = 1; sample_cnt <= MAX_SAMPLE_CNT_DIV; sample_cnt++) {
		step_cnt = (min_div + sample_cnt - 1) / sample_cnt;
		cnt_mul = step_cnt * sample_cnt;
		if (step_cnt > max_step_cnt)
			continue;
		if (cnt_mul < best_mul) {
			best_mul = cnt_mul;
			sample_div = sample_cnt;
			step_div = step_cnt;
			if (best_mul == min_div)
				break;
		}
	}
	sample_cnt = sample_div;
	step_cnt = step_div;
	sclk = hclk / (2 * sample_cnt * step_cnt);
	if (sclk > khz) {
		dev_dbg(i2c->dev, "%s mode: unsupported speed (%ldkhz)\n",
			(mode == HS_MODE) ? "HS" : "ST/FT", (long int)khz);
		return -ENOTSUPP;
	}

	step_cnt--;
	sample_cnt--;

	if (mode == HS_MODE) {
		/* Set the hign speed mode register */
		i2c->timing_reg = 0x1303;
		i2c->high_speed_reg = I2C_TIME_DEFAULT_VALUE |
			(sample_cnt & I2C_TIMING_SAMPLE_COUNT_MASK) << 12 |
			(step_cnt & I2C_TIMING_SAMPLE_COUNT_MASK) << 8;
	} else {
		i2c->timing_reg =
			(sample_cnt & I2C_TIMING_SAMPLE_COUNT_MASK) << 8 |
			(step_cnt & I2C_TIMING_STEP_DIV_MASK) << 0;
		/* Disable the high speed transaction */
		i2c->high_speed_reg = I2C_TIME_CLR_VALUE;
	}

	return 0;
}

#ifdef I2C_DEBUG_FS
static void i2c_dump_info(struct mt_i2c *i2c)
{
	if (i2c->ext_data.isEnable && timing)
		dev_err(i2c->dev, "I2C structure:\nspeed %d\n",
			i2c->ext_data.timing);
	else
		dev_err(i2c->dev, "I2C structure:\nspeed %d\n",
			i2c->speed_hz);
	dev_err(i2c->dev, "I2C structure:\nOp %x\n", i2c->op);
	dev_err(i2c->dev,
		"I2C structure:\nData_size %x\nIrq_stat %x\nTrans_stop %d\n",
		i2c->msg_len, i2c->irq_stat, i2c->trans_stop);
	dev_err(i2c->dev, "base address %x\n", i2c->base);
	dev_err(i2c->dev,
		"I2C register:\nSLAVE_ADDR %x\nINTR_MASK %x\n",
		(i2c_readw(i2c, OFFSET_SLAVE_ADDR)),
		(i2c_readw(i2c, OFFSET_INTR_MASK)));
	dev_err(i2c->dev,
		"I2C register:\nINTR_STAT %x\nCONTROL %x\n",
		(i2c_readw(i2c, OFFSET_INTR_STAT)),
		(i2c_readw(i2c, OFFSET_CONTROL)));
	dev_err(i2c->dev,
		"I2C register:\nTRANSFER_LEN %x\nTRANSAC_LEN %x\n",
		(i2c_readw(i2c, OFFSET_TRANSFER_LEN)),
		(i2c_readw(i2c, OFFSET_TRANSAC_LEN)));
	dev_err(i2c->dev,
		"I2C register:\nDELAY_LEN %x\nTIMING %x\n",
		(i2c_readw(i2c, OFFSET_DELAY_LEN)),
		(i2c_readw(i2c, OFFSET_TIMING)));
	dev_err(i2c->dev,
		"I2C register:\nSTART %x\nFIFO_STAT %x\n",
		(i2c_readw(i2c, OFFSET_START)),
		(i2c_readw(i2c, OFFSET_FIFO_STAT)));
	dev_err(i2c->dev,
		"I2C register:\nIO_CONFIG %x\nHS %x\n",
		(i2c_readw(i2c, OFFSET_IO_CONFIG)),
		(i2c_readw(i2c, OFFSET_HS)));
	dev_err(i2c->dev,
		"I2C register:\nDEBUGSTAT %x\nEXT_CONF %x\nPATH_DIR %x\n",
		(i2c_readw(i2c, OFFSET_DEBUGSTAT)),
		(i2c_readw(i2c, OFFSET_EXT_CONF)),
		(i2c_readw(i2c, OFFSET_PATH_DIR)));
}
#else
static void i2c_dump_info(struct mt_i2c *i2c)
{
}
#endif
static int mt_i2c_do_transfer(struct mt_i2c *i2c)
{
	u16 addr_reg;
	u16 control_reg;
	int tmo = i2c->adap.timeout;
	unsigned int speed_hz;
	int ret;

	i2c->trans_stop = false;
	i2c->irq_stat = 0;
	if (i2c->ext_data.isEnable && i2c->ext_data.timing)
		speed_hz = i2c->ext_data.timing;
	else
		speed_hz = i2c->speed_hz;
	ret = i2c_set_speed(i2c, clk_get_rate(i2c->clk_pmic) / i2c->clk_src_div);
	if (ret) {
		dev_err(i2c->dev, "Failed to set the speed\n");
		return -EINVAL;
	}
	/* If use i2c pin from PMIC mt6397 side, need set PATH_DIR first */
	if (i2c->have_pmic)
		i2c_writew(I2C_CONTROL_WRAPPER, i2c, OFFSET_PATH_DIR);
	control_reg = I2C_CONTROL_ACKERR_DET_EN |
		I2C_CONTROL_CLK_EXT_EN | I2C_CONTROL_DMA_EN;
	if (speed_hz > 400000)
		control_reg |= I2C_CONTROL_RS;
	if (i2c->op == I2C_MASTER_WRRD)
		control_reg |= I2C_CONTROL_DIR_CHANGE | I2C_CONTROL_RS;
	i2c_writew(control_reg, i2c, OFFSET_CONTROL);

	/* set start condition */
	if (speed_hz <= 100000)
		i2c_writew(I2C_ST_START_CON, i2c, OFFSET_EXT_CONF);
	else
		i2c_writew(I2C_FS_START_CON, i2c, OFFSET_EXT_CONF);

	if (~control_reg & I2C_CONTROL_RS)
		i2c_writew(I2C_DELAY_LEN, i2c, OFFSET_DELAY_LEN);

	addr_reg = i2c->addr << 1;
	if (i2c->op == I2C_MASTER_RD)
		addr_reg |= 0x1;
	i2c_writew(addr_reg, i2c, OFFSET_SLAVE_ADDR);
	/* Clear interrupt status */
	i2c_writew(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP,
		i2c, OFFSET_INTR_STAT);
	i2c_writew(I2C_FIFO_ADDR_CLR, i2c, OFFSET_FIFO_ADDR_CLR);
	/* Enable interrupt */
	i2c_writew(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP,
		i2c, OFFSET_INTR_MASK);
	/* Set transfer and transaction len */
	if (i2c->op == I2C_MASTER_WRRD) {
		i2c_writew(i2c->msg_len, i2c, OFFSET_TRANSFER_LEN);
		i2c_writew(i2c->msg_aux_len, i2c, OFFSET_TRANSFER_LEN_AUX);
		i2c_writew(0x02, i2c, OFFSET_TRANSAC_LEN);
	} else {
		i2c_writew(i2c->msg_len, i2c, OFFSET_TRANSFER_LEN);
		i2c_writew(0x01, i2c, OFFSET_TRANSAC_LEN);
	}

	/* Prepare buffer data to start transfer */
	if (i2c->op == I2C_MASTER_RD) {
		i2c_writel_dma(I2C_DMA_INT_FLAG_NONE, i2c, OFFSET_INT_FLAG);
		i2c_writel_dma(I2C_DMA_CON_RX, i2c, OFFSET_CON);
		i2c_writel_dma((u32)i2c->dma_buf.paddr, i2c, OFFSET_RX_MEM_ADDR);
		i2c_writel_dma(i2c->msg_len, i2c, OFFSET_RX_LEN);
	} else if (i2c->op == I2C_MASTER_WR) {
		i2c_writel_dma(I2C_DMA_INT_FLAG_NONE, i2c, OFFSET_INT_FLAG);
		i2c_writel_dma(I2C_DMA_CON_TX, i2c, OFFSET_CON);
		i2c_writel_dma((u32)i2c->dma_buf.paddr, i2c, OFFSET_TX_MEM_ADDR);
		i2c_writel_dma(i2c->msg_len, i2c, OFFSET_TX_LEN);
	} else {
		i2c_writel_dma(0x0000, i2c, OFFSET_INT_FLAG);
		i2c_writel_dma(0x0000, i2c, OFFSET_CON);
		i2c_writel_dma((u32)i2c->dma_buf.paddr, i2c, OFFSET_TX_MEM_ADDR);
		i2c_writel_dma((u32)i2c->dma_buf.paddr, i2c,
			OFFSET_RX_MEM_ADDR);
		i2c_writel_dma(i2c->msg_len, i2c, OFFSET_TX_LEN);
		i2c_writel_dma(i2c->msg_aux_len, i2c, OFFSET_RX_LEN);
	}
	/* flush before sending start */
	mb();
	i2c_writel_dma(I2C_DMA_START_EN, i2c, OFFSET_EN);
	if (!i2c->ext_data.isEnable || !i2c->ext_data.is_hw_trig)
		i2c_writew(I2C_TRANSAC_START, i2c, OFFSET_START);
	else
		dev_err(i2c->dev, "I2C hw trig.\n");

	tmo = wait_event_timeout(i2c->wait, i2c->trans_stop, tmo);
	if (tmo == 0) {
		dev_err(i2c->dev, "addr: %x, transfer timeout\n", i2c->addr);
		i2c_dump_info(i2c);
		mt_i2c_init_hw(i2c);
		return -ETIMEDOUT;
	}
	if (i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR)) {
		dev_err(i2c->dev, "addr: %x, transfer ACK error\n", i2c->addr);
		mt_i2c_init_hw(i2c);
		i2c_dump_info(i2c);
		return -EREMOTEIO;
	}
	dev_dbg(i2c->dev, "i2c transfer done.\n");
	return 0;
}

static inline void mt_i2c_copy_to_dma(struct mt_i2c *i2c, struct i2c_msg *msg)
{
	/* if the operate is write, need to copy the data to DMA memory */
	if (!(msg->flags & I2C_M_RD))
		memcpy(i2c->dma_buf.vaddr, msg->buf, msg->len);
}

static inline void mt_i2c_copy_from_dma(struct mt_i2c *i2c,
	struct i2c_msg *msg)
{
	/* if the operate is read, need to copy the data from DMA memory */
	if (msg->flags & I2C_M_RD)
		memcpy(msg->buf, i2c->dma_buf.vaddr, msg->len);
}

/*
 * In MTK platform the STOP will be issued after each
 * message was transferred which is not flow the clarify
 * for i2c_transfer(), several I2C devices tolerate the STOP,
 * but some device need Repeat-Start and do not compatible with STOP
 * MTK platform has WRRD mode which can write then read with
 * Repeat-Start between two message, so we combined two
 * messages into one transaction.
 * The max read length is 4096
 */
static bool mt_i2c_should_combine(struct i2c_msg *msg)
{
	struct i2c_msg *next_msg = msg + 1;

	if ((next_msg->len < 4096) &&
			msg->addr == next_msg->addr &&
			!(msg->flags & I2C_M_RD) &&
			(next_msg->flags & I2C_M_RD) == I2C_M_RD) {
		return true;
	}
	return false;
}


static int __mt_i2c_transfer(struct mt_i2c *i2c,
	struct i2c_msg msgs[], int num)
{
	int ret;
	int left_num = num;

	ret = mt_i2c_clock_enable(i2c);
	if (ret)
		return ret;
	while (left_num--) {
		/* In MTK platform the max transfer number is 4096 */
		if (msgs->len > MAX_DMA_TRANS_SIZE) {
			dev_dbg(i2c->dev,
				" message data length is more than 255\n");
			ret = -EINVAL;
			goto err_exit;
		}
		if (msgs->addr == 0) {
			dev_dbg(i2c->dev, " addr is invalid.\n");
			ret = -EINVAL;
			goto err_exit;
		}
		if (msgs->buf == NULL) {
			dev_dbg(i2c->dev, " data buffer is NULL.\n");
			ret = -EINVAL;
			goto err_exit;
		}

		i2c->addr = msgs->addr;
		i2c->msg_len = msgs->len;
		i2c->msg_buf = msgs->buf;
		if (msgs->flags & I2C_M_RD)
			i2c->op = I2C_MASTER_RD;
		else
			i2c->op = I2C_MASTER_WR;
		/* combined two messages into one transaction */
		if (left_num >= 1 && mt_i2c_should_combine(msgs)) {
			i2c->msg_aux_len = (msgs + 1)->len;
			i2c->op = I2C_MASTER_WRRD;
			left_num--;
		}
		/*
		 * always use DMA mode.
		 * 1st when write need copy the data of message to dma memory
		 * 2nd when read need copy the DMA data to the message buffer.
		 * The length should be less than 255.
		 */
		mt_i2c_copy_to_dma(i2c, msgs);
		i2c->msg_buf = (u8 *)i2c->dma_buf.paddr;

		ret = mt_i2c_do_transfer(i2c);
		if (ret < 0)
			goto err_exit;
		if (i2c->op == I2C_MASTER_WRRD)
			mt_i2c_copy_from_dma(i2c, msgs + 1);
		else
			mt_i2c_copy_from_dma(i2c, msgs);
		msgs++;
		/* after combined two messages so we need ignore one */
		if (left_num > 0 && i2c->op == I2C_MASTER_WRRD)
			msgs++;
	}
	/* the return value is number of executed messages */
	ret = num;
err_exit:
	mt_i2c_clock_disable(i2c);
	return ret;
}

static int mt_i2c_transfer(struct i2c_adapter *adap,
	struct i2c_msg msgs[], int num)
{
	int ret;
	struct mt_i2c *i2c = i2c_get_adapdata(adap);

	mutex_lock(&i2c->i2c_mutex);
	ret = __mt_i2c_transfer(i2c, msgs, num);
	mutex_unlock(&i2c->i2c_mutex);

	return ret;
}


static void mt_i2c_parse_extension(struct mt_i2c_ext *pext, u32 ext_flag, u32 timing)
{
	if (ext_flag & I2C_HWTRIG_FLAG)
		pext->is_hw_trig = true;
	if (timing)
		pext->timing = timing;
}

int mtk_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num,
					u32 ext_flag, u32 timing)
{
	int ret;
	struct mt_i2c *i2c = i2c_get_adapdata(adap);

	mutex_lock(&i2c->i2c_mutex);
	i2c->ext_data.isEnable = true;

	mt_i2c_parse_extension(&i2c->ext_data, ext_flag, timing);
	ret = __mt_i2c_transfer(i2c, msgs, num);

	i2c->ext_data.isEnable = false;
	mutex_unlock(&i2c->i2c_mutex);
	return ret;
}
EXPORT_SYMBOL(mtk_i2c_transfer);

static irqreturn_t mt_i2c_irq(int irqno, void *dev_id)
{
	struct mt_i2c *i2c = dev_id;

	/* Clear interrupt mask */
	i2c_writew(~(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP),
		i2c, OFFSET_INTR_MASK);
	i2c->irq_stat = i2c_readw(i2c, OFFSET_INTR_STAT);
	i2c_writew(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP,
		i2c, OFFSET_INTR_STAT);
	i2c->trans_stop = true;
	wake_up(&i2c->wait);
	return IRQ_HANDLED;
}

static u32 mt_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm mt_i2c_algorithm = {
	.master_xfer = mt_i2c_transfer,
	.functionality = mt_i2c_functionality,
};

static int mt_i2c_parse_dt(struct device_node *np, struct mt_i2c *i2c)
{
	i2c->speed_hz = I2C_DEFAUT_SPEED;
	of_property_read_u32(np, "clock-frequency", &i2c->speed_hz);
	of_property_read_u32(np, "clock-div", &i2c->clk_src_div);
	of_property_read_u32(np, "id", (u32 *)&i2c->id);
	i2c->have_pmic = of_property_read_bool(np, "mediatek,have-pmic");
	i2c->have_dcm = of_property_read_bool(np, "mediatek,have-dcm");
	i2c->use_push_pull = of_property_read_bool(np, "mediatek,use-push-pull");
	if (i2c->clk_src_div == 0)
		return -EINVAL;
	return 0;
}

static int mt_i2c_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mt_i2c *i2c;
	unsigned int clk_src_in_hz;
	struct resource *res;

	i2c = devm_kzalloc(&pdev->dev, sizeof(struct mt_i2c), GFP_KERNEL);
	if (i2c == NULL)
		return -ENOMEM;

	ret = mt_i2c_parse_dt(pdev->dev.of_node, i2c);
	if (ret)
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	i2c->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c->base))
		return PTR_ERR(i2c->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	i2c->pdmabase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c->pdmabase))
		return PTR_ERR(i2c->pdmabase);

	i2c->irqnr = platform_get_irq(pdev, 0);
	if (i2c->irqnr <= 0)
		return -EINVAL;

	ret = devm_request_irq(&pdev->dev, i2c->irqnr, mt_i2c_irq,
		IRQF_TRIGGER_NONE, I2C_DRV_NAME, i2c);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"Request I2C IRQ %d fail\n", i2c->irqnr);
		return ret;
	}

	i2c->adap.dev.of_node = pdev->dev.of_node;
	i2c->dev = &i2c->adap.dev;
	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &mt_i2c_algorithm;
	i2c->adap.algo_data = NULL;
	i2c->adap.timeout = 2 * HZ;
	i2c->adap.retries = 1;
	i2c->adap.nr = i2c->id;

	i2c->clk_main = devm_clk_get(&pdev->dev, "main");
	if (IS_ERR(i2c->clk_main)) {
		dev_err(&pdev->dev, "cannot get main clock\n");
		return PTR_ERR(i2c->clk_main);
	}
	i2c->clk_dma = devm_clk_get(&pdev->dev, "dma");
	if (IS_ERR(i2c->clk_dma)) {
		dev_err(&pdev->dev, "cannot get dma clock\n");
		return PTR_ERR(i2c->clk_dma);
	}
	if (i2c->have_pmic) {
		i2c->clk_pmic = devm_clk_get(&pdev->dev, "pmic");
		if (IS_ERR(i2c->clk_pmic)) {
			dev_err(&pdev->dev, "cannot get pmic clock\n");
			return PTR_ERR(i2c->clk_pmic);
		}
		clk_src_in_hz = clk_get_rate(i2c->clk_pmic) / i2c->clk_src_div;
	} else {
		clk_src_in_hz = clk_get_rate(i2c->clk_main) / i2c->clk_src_div;
	}
	dev_dbg(&pdev->dev, "clock source %p,clock src frequency %d\n",
		i2c->clk_main, clk_src_in_hz);

	strlcpy(i2c->adap.name, I2C_DRV_NAME, sizeof(i2c->adap.name));
	init_waitqueue_head(&i2c->wait);
	mutex_init(&i2c->i2c_mutex);
	ret = i2c_set_speed(i2c, clk_src_in_hz);
	if (ret) {
		dev_err(&pdev->dev, "Failed to set the speed\n");
		return -EINVAL;
	}
	ret = mt_i2c_clock_enable(i2c);
	if (ret) {
		dev_err(&pdev->dev, "clock enable failed!\n");
		return ret;
	}
	mt_i2c_init_hw(i2c);
	mt_i2c_clock_disable(i2c);
	i2c->dma_buf.vaddr = dma_alloc_coherent(&pdev->dev,
		PAGE_SIZE, &i2c->dma_buf.paddr, GFP_KERNEL);
	if (i2c->dma_buf.vaddr == NULL) {
		dev_err(&pdev->dev, "dma_alloc_coherent fail\n");
		return -ENOMEM;
	}
	i2c_set_adapdata(&i2c->adap, i2c);
	/* ret = i2c_add_adapter(&i2c->adap); */
	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add i2c bus to i2c core\n");
		free_i2c_dma_bufs(i2c);
		return ret;
	}
	platform_set_drvdata(pdev, i2c);
	return 0;
}

static int mt_i2c_remove(struct platform_device *pdev)
{
	struct mt_i2c *i2c = platform_get_drvdata(pdev);

	i2c_del_adapter(&i2c->adap);
	free_i2c_dma_bufs(i2c);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id mtk_i2c_of_match[] = {
	{ .compatible = "mediatek,mt6735-i2c", },
	{},
};
MODULE_DEVICE_TABLE(of, mt_i2c_match);

static struct platform_driver mt_i2c_driver = {
	.probe = mt_i2c_probe,
	.remove = mt_i2c_remove,
	.driver = {
		.name = I2C_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mtk_i2c_of_match),
	},
};

module_platform_driver(mt_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek I2C Bus Driver");
MODULE_AUTHOR("Xudong Chen <xudong.chen@mediatek.com>");
