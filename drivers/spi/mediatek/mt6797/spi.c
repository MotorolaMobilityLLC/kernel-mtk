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

#if !defined(CONFIG_MTK_CLKMGR)
#include <linux/clk.h>
#endif				/* !defined(CONFIG_MTK_CLKMGR) */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
/*#include <mach/irqs.h>*/
#include "mt_spi.h"
#include "mt_spi_hal.h"
/*#include <mach/mt_gpio.h>*/
#include <mt-plat/mt_lpae.h>/* DMA */


#if defined(CONFIG_MTK_CLKMGR)
 /* mt_clkmgr.h will be removed after CCF porting is finished. */
#include <mach/mt_clkmgr.h>
#endif				/* defined(CONFIG_MTK_CLKMGR) */

#if (defined(CONFIG_MTK_FPGA))
#define  CONFIG_MT_SPI_FPGA_ENABLE
#endif
/*auto select transfer mode*/
/*#define SPI_AUTO_SELECT_MODE*/
#ifdef SPI_AUTO_SELECT_MODE
#define SPI_DATA_SIZE 16
#endif

/*open base log out*/
/*#define SPI_DEBUG*/

/*open verbose log out*/
/*#define SPI_VERBOSE*/


#define IDLE 0
#define INPROGRESS 1
#define PAUSED 2

#define PACKET_SIZE 0x400
#define SPI_FIFO_SIZE 32
#define INVALID_DMA_ADDRESS 0xffffffff
#define SPI1_DMA_LIMIT_L 0x40000000
#define SPI1_DMA_LIMIT_H 0xbfffffff
#if 0
struct mt_spi_t {
	struct platform_device *pdev;
	void __iomem *regs;
	int irq;
	int running;
	u32 pad_macro;
	struct wake_lock wk_lock;
	struct mt_chip_conf *config;
	struct spi_master *master;

	struct spi_transfer *cur_transfer;
	struct spi_transfer *next_transfer;

	spinlock_t lock;
	struct list_head queue;
#if !defined(CONFIG_MTK_CLKMGR)
	struct clk *clk_main;	/* main clock for spi bus */
#endif				/* !defined(CONFIG_MTK_CLKMGR) */
};
#endif
/*open time record debug, log can't affect transfer*/
/*	#define SPI_REC_DEBUG */

struct platform_device *spi_pdev[6];
static void enable_clk(struct mt_spi_t *ms)
{
#if (!defined(CONFIG_MT_SPI_FPGA_ENABLE))
#if defined(CONFIG_MTK_CLKMGR)
	enable_clock(MT_CG_PERI_SPI0, "spi");
#else
	int ret;
/*	clk_prepare_enable(ms->clk_main); */
	ret = clk_enable(ms->clk_main);
#endif
#endif
}

void mt_spi_enable_clk(struct mt_spi_t *ms)
{
	enable_clk(ms);
}

void mt_spi_enable_master_clk(struct spi_device *ms)
{
	enable_clk(spi_master_get_devdata(ms->master));
}

static void disable_clk(struct mt_spi_t *ms)
{
#if (!defined(CONFIG_MT_SPI_FPGA_ENABLE))
#if defined(CONFIG_MTK_CLKMGR)
	disable_clock(MT_CG_PERI_SPI0, "spi");
#else
/*	clk_disable_unprepare(ms->clk_main); */
	clk_disable(ms->clk_main);
#endif
#endif

}

void mt_spi_disable_clk(struct mt_spi_t *ms)
{
	disable_clk(ms);
}

void mt_spi_disable_master_clk(struct spi_device *ms)
{
	disable_clk(spi_master_get_devdata(ms->master));
}

#ifdef SPI_DEBUG
	/*#define SPI_DBG(fmt, args...)  printk(KERN_ALERT "mt-spi.c:%5d: <%s>" fmt, __LINE__, __func__, ##args )*/
	#define SPI_DBG(fmt, args...)  pr_debug("mt-spi.c:%5d: <%s>" fmt,  __LINE__, __func__, ##args)

#ifdef SPI_VERBOSE
#define SPI_INFO(dev, fmt, args...)  dev_info(dev, "spi.c:%5d: <%s>" fmt, __LINE__, __func__, ##args)
static void spi_dump_reg(struct mt_spi_t *ms)
{

	SPI_DBG("||*****************************************||\n");
	SPI_DBG("cfg0:0x%.8x\n", spi_readl(ms, SPI_CFG0_REG));
	SPI_DBG("cfg1:0x%.8x\n", spi_readl(ms, SPI_CFG1_REG));
	SPI_DBG("cfg2:0x%.8x\n", spi_readl(ms, SPI_CFG2_REG));
	SPI_DBG("cmd :0x%.8x\n", spi_readl(ms, SPI_CMD_REG));
/*	SPI_DBG("spi_tx_data_reg:0x%x\n", spi_readl(ms, SPI_TX_DATA_REG));
	SPI_DBG("spi_rx_data_reg:0x%x\n", spi_readl(ms, SPI_RX_DATA_REG));*/
	SPI_DBG("tx_s:0x%.8x\n", spi_readl(ms, SPI_TX_SRC_REG));
	SPI_DBG("rx_d:0x%.8x\n", spi_readl(ms, SPI_RX_DST_REG));
	SPI_DBG("sta1:0x%.8x\n", spi_readl(ms, SPI_STATUS1_REG));
	SPI_DBG("pad_sel:0x%.8x\n", spi_readl(ms, SPI_PAD_SEL_REG));
	SPI_DBG("||*****************************************||\n");

}
#else
#define SPI_INFO(dev, fmt, args...)
static void spi_dump_reg(struct mt_spi_t *ms)
{
}
#endif
#else
#define SPI_DBG(fmt, args...)
#define SPI_INFO(dev, fmt, args...)
#endif

#ifdef SPI_REC_DEBUG
/*EVB the clock frequency is 130MHz, may be is reason that ldvt env.
  but it is 98500KHz on phone.
  */
#define SPI_CLOCK_PERIED 100000	/*kHz */

#include <linux/syscalls.h>	/*getpid() */
#define SPI_REC_MSG_MAX 500
#define SPI_REC_NUM 20
#define SPI_REC_STR_LEN 256
static u32 spi_speed;
static char msg_rec[SPI_REC_MSG_MAX][SPI_REC_STR_LEN];
static int rec_count;
static int rec_count_tmp = 1;
static int rec_msg_count;

static atomic_t rec_log_count = ATOMIC_INIT(0);

static unsigned long long rec_msg_time[SPI_REC_MSG_MAX];
static unsigned long long rec_time;

/*should coding in file kernel/arch/arm/kernel/irq.c.
	extern unsigned long long spi_rec_t0;
	spi_rec_t0 = sched_clock();
*/
unsigned long long spi_rec_t0;	/* record interrupt act */

DEFINE_SPINLOCK(msg_rec_lock);
/*      static unsigned long long t_rec[4]; */

/*
	the function invoke time averrage 2us.
*/

static inline void spi_rec_time(const char *str)
{
	unsigned long flags;
	char tmp[64];

	spin_lock_irqsave(&msg_rec_lock, flags);

	if (strncmp(str, "msgs", 4) == 0) {
		rec_msg_count++;
		if (rec_msg_count >= SPI_REC_MSG_MAX)
			rec_msg_count = 0;
		rec_msg_time[rec_msg_count] = sched_clock();
		msg_rec[rec_msg_count][0] = '\0';
		sprintf(tmp, "%s,pid:%4d;", str, sys_getpid());
		strcat(msg_rec[rec_msg_count], tmp);
	} else if (strncmp(str, "msgn", 4) == 0) {
		rec_count++;
		if (rec_count >= SPI_REC_MSG_MAX)
			rec_count = 0;
		rec_time = sched_clock();

		sprintf(tmp, "%s:%8lld;", str, rec_time - rec_msg_time[rec_count]);
		strcat(msg_rec[rec_count], tmp);

/*if want to spi interrupt action, cancle the comment*/
	}			/*else if(strncmp(str, "irqs", 4) == 0){
				   sprintf(tmp,"%s,%5lld:%8lld;",str,
				   sched_clock() - spi_rec_t0,sched_clock() - rec_time);
				   if((strlen(tmp) + strlen(msg_rec[rec_count])) < (SPI_REC_STR_LEN - 64)){
				   strcat(msg_rec[rec_count],tmp);
				   }else{
				   strcat(msg_rec[rec_count],"#");
				   }
				   } */
	else {
		sprintf(tmp, "%s:%8lld;", str, sched_clock() - rec_time);
		if ((strlen(tmp) + strlen(msg_rec[rec_count])) < (SPI_REC_STR_LEN - 64))
			strcat(msg_rec[rec_count], tmp);
		else
			strcat(msg_rec[rec_count], "@");
	}
	spin_unlock_irqrestore(&msg_rec_lock, flags);

}

void mt_spi_workqueue_handler(void *data)
{
	int i = 0;

	for (i = 0; i < SPI_REC_NUM; i++) {
		SPI_DBG("spi-rec%3d-%3d:%s\n", rec_count, rec_count_tmp, msg_rec[rec_count_tmp]);
		msg_rec[rec_count_tmp][0] = '\0';
		rec_count_tmp++;
		if (rec_count_tmp >= SPI_REC_MSG_MAX)
			rec_count_tmp = 0;
	}
	atomic_dec(&rec_log_count);

}

DECLARE_WORK(mt_spi_workqueue, mt_spi_workqueue_handler);

#else
static inline void spi_rec_time(const char *str)
{

}

#endif

#if 1
struct pinctrl *pinctrl_spi;
struct pinctrl_state *pins_spi_cs_set, *pins_spi_cs_clear, *pins_spi_clk_set, *pins_spi_clk_clear;
struct pinctrl_state *pins_spi_miso_set, *pins_spi_miso_clear, *pins_spi_mosi_set, *pins_spi_mosi_clear;

static int spi_get_pinctrl_info(struct platform_device *pdev)
{
	int ret;

	SPI_DBG("spi_get_gpio_info pin ctrl===================\n");
	pinctrl_spi = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl_spi)) {
		ret = PTR_ERR(pinctrl_spi);
		dev_err(&pdev->dev, "Cannot find spi pinctrl1!\n");
		return ret;
	}
	pins_spi_cs_set = pinctrl_lookup_state(pinctrl_spi, "spi1_cs_set");
	if (IS_ERR(pins_spi_cs_set)) {
		ret = PTR_ERR(pins_spi_cs_set);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_cs_set!\n");
		return ret;
	}
	pins_spi_cs_clear = pinctrl_lookup_state(pinctrl_spi, "spi1_cs_clr");
	if (IS_ERR(pins_spi_cs_clear)) {
		ret = PTR_ERR(pins_spi_cs_clear);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_cs_clear!\n");
		return ret;
	}
	pins_spi_clk_set = pinctrl_lookup_state(pinctrl_spi, "spi1_clk_set");
	if (IS_ERR(pins_spi_clk_set)) {
		ret = PTR_ERR(pins_spi_clk_set);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_clk_set!\n");
		return ret;
	}
	pins_spi_clk_clear = pinctrl_lookup_state(pinctrl_spi, "spi1_clk_clr");
	if (IS_ERR(pins_spi_clk_clear)) {
		ret = PTR_ERR(pins_spi_clk_clear);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_clk_clear!\n");
		return ret;
	}
	pins_spi_miso_set = pinctrl_lookup_state(pinctrl_spi, "spi1_miso_set");
	if (IS_ERR(pins_spi_miso_set)) {
		ret = PTR_ERR(pins_spi_miso_set);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_miso_set!\n");
		return ret;
	}
	pins_spi_miso_clear = pinctrl_lookup_state(pinctrl_spi, "spi1_miso_clr");
	if (IS_ERR(pins_spi_miso_clear)) {
		ret = PTR_ERR(pins_spi_miso_clear);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_miso_clear!\n");
		return ret;
	}
	pins_spi_mosi_set = pinctrl_lookup_state(pinctrl_spi, "spi1_mosi_set");
	if (IS_ERR(pins_spi_mosi_set)) {
		ret = PTR_ERR(pins_spi_mosi_set);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_mosi_set!\n");
		return ret;
	}
	pins_spi_mosi_clear = pinctrl_lookup_state(pinctrl_spi, "spi1_mosi_clr");
	if (IS_ERR(pins_spi_mosi_clear)) {
		ret = PTR_ERR(pins_spi_mosi_clear);
		dev_err(&pdev->dev, "Cannot find spi pinctrl pins_spi1_mosi_clear!\n");
		return ret;
	}
	return 0;
}

void spi_set_pinctrl(int spi_id)
{
	struct platform_device *pdev = spi_pdev[spi_id];

	spi_get_pinctrl_info(pdev);

	pinctrl_select_state(pinctrl_spi, pins_spi_cs_set);
	pinctrl_select_state(pinctrl_spi, pins_spi_clk_set);
	pinctrl_select_state(pinctrl_spi, pins_spi_miso_set);
	pinctrl_select_state(pinctrl_spi, pins_spi_mosi_set);

}

void spi_clr_pinctrl(int spi_id)
{
	struct platform_device *pdev = spi_pdev[spi_id];

	spi_get_pinctrl_info(pdev);

	pinctrl_select_state(pinctrl_spi, pins_spi_cs_clear);
	pinctrl_select_state(pinctrl_spi, pins_spi_clk_clear);
	pinctrl_select_state(pinctrl_spi, pins_spi_miso_clear);
	pinctrl_select_state(pinctrl_spi, pins_spi_mosi_clear);
}

#endif

static void spi_gpio_set(struct mt_spi_t *ms)
{
/*  mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_SPI_CS_N);
	mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_SPI_SCK);
	mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_SPI_MISO);
	mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_SPI_MOSI);*/

}

static void spi_gpio_reset(struct mt_spi_t *ms)
{
	/*set dir pull to save power */
/*
	mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_GPIO);
	mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_GPIO);
	mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_GPIO);
	mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_GPIO); */

}

static int is_pause_mode(struct spi_message *msg)
{
	struct mt_chip_conf *conf;

	conf = (struct mt_chip_conf *)msg->state;
	return conf->pause;
}

static int is_fifo_read(struct spi_message *msg)
{
	struct mt_chip_conf *conf;
	u32 value = 0;

	conf = (struct mt_chip_conf *)msg->state;
	value = (conf->com_mod == FIFO_TRANSFER) || (conf->com_mod == OTHER1);
	return value;
}

static int is_interrupt_enable(struct mt_spi_t *ms)
{
	u32 cmd;

	cmd = spi_readl(ms, SPI_CMD_REG);
	return (cmd >> SPI_CMD_FINISH_IE_OFFSET) & 1;
}

static inline void set_pause_bit(struct mt_spi_t *ms)
{
	u32 reg_val;

	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val |= 1 << SPI_CMD_PAUSE_EN_OFFSET;
	spi_writel(ms, SPI_CMD_REG, reg_val);

}

static inline void clear_pause_bit(struct mt_spi_t *ms)
{
	u32 reg_val;

	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_PAUSE_EN_MASK;
	spi_writel(ms, SPI_CMD_REG, reg_val);

}

static inline void clear_resume_bit(struct mt_spi_t *ms)
{
	u32 reg_val;

	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RESUME_MASK;
	spi_writel(ms, SPI_CMD_REG, reg_val);

}

static inline void spi_disable_dma(struct mt_spi_t *ms)
{
	u32 cmd;

	cmd = spi_readl(ms, SPI_CMD_REG);
	cmd &= ~SPI_CMD_TX_DMA_MASK;
	cmd &= ~SPI_CMD_RX_DMA_MASK;
	spi_writel(ms, SPI_CMD_REG, cmd);

}

static inline void spi_enable_dma(struct mt_spi_t *ms, u8 mode)
{
	u32 cmd;
	u32 id;
	dma_addr_t	temp_tx_dma;
	dma_addr_t	temp_rx_dma;

	id = ms->pdev->id;
	cmd = spi_readl(ms, SPI_CMD_REG);
#define SPI_4B_ALIGN 0x4
	/*set up the DMA bus address */
	if ((mode == DMA_TRANSFER) || (mode == OTHER1)) {
		temp_tx_dma = ms->cur_transfer->tx_dma;
		if ((ms->cur_transfer->tx_buf != NULL)
		    || ((ms->cur_transfer->tx_dma != INVALID_DMA_ADDRESS) && (ms->cur_transfer->tx_dma != 0))) {
			if (ms->cur_transfer->tx_dma & (SPI_4B_ALIGN - 1)) {
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
				dev_err(&ms->pdev->dev,
					"Warning!Tx_DMA address should be 4Byte alignment,buf:%p,dma:%llx\n",
					ms->cur_transfer->tx_buf, ms->cur_transfer->tx_dma);
#else
				dev_err(&ms->pdev->dev,
					"Warning!Tx_DMA address should be 4Byte alignment,buf:%p,dma:%x\n",
					ms->cur_transfer->tx_buf, ms->cur_transfer->tx_dma);
#endif
			}
			if (enable_4G()) {/*4G mode*/
				if (1 == id) {/*SPI1*/
					if ((ms->cur_transfer->tx_dma < SPI1_DMA_LIMIT_L)
						|| (ms->cur_transfer->tx_dma
							+ ms->cur_transfer->len > SPI1_DMA_LIMIT_H))
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
						dev_err(&ms->pdev->dev,
							"Warning!Tx_DMA address range error in 4G mode,dma:%llx\n",
							ms->cur_transfer->tx_dma);
#else
						dev_err(&ms->pdev->dev,
							"Warning!Tx_DMA address range error in 4G mode,dma:%x\n",
							ms->cur_transfer->tx_dma);
#endif
					MAPPING_DRAM_ACCESS_ADDR(temp_tx_dma);
				}
			}
			spi_writel(ms, SPI_TX_SRC_REG, cpu_to_le32(temp_tx_dma));
			cmd |= 1 << SPI_CMD_TX_DMA_OFFSET;
		}
	}
	if ((mode == DMA_TRANSFER) || (mode == OTHER2)) {
		temp_rx_dma = ms->cur_transfer->rx_dma;
		if ((ms->cur_transfer->rx_buf != NULL)
		|| ((ms->cur_transfer->rx_dma != INVALID_DMA_ADDRESS) && (ms->cur_transfer->rx_dma != 0))) {
			if (ms->cur_transfer->rx_dma & (SPI_4B_ALIGN - 1)) {
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
				dev_err(&ms->pdev->dev,
					"Warning!Rx_DMA address should be 4Byte alignment,buf:%p,dma:%llx\n",
					ms->cur_transfer->rx_buf, ms->cur_transfer->rx_dma);
#else
				dev_err(&ms->pdev->dev,
					"Warning!Rx_DMA address should be 4Byte alignment,buf:%p,dma:%x\n",
					ms->cur_transfer->rx_buf, ms->cur_transfer->rx_dma);
#endif
			}
			if (enable_4G()) {/*4G mode*/
				if (1 == id) {/*SPI1*/
					if ((ms->cur_transfer->rx_dma < SPI1_DMA_LIMIT_L)
						|| (ms->cur_transfer->rx_dma
							+ ms->cur_transfer->len > SPI1_DMA_LIMIT_H))
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
						dev_err(&ms->pdev->dev,
							"Warning!Rx_DMA address range error in 4G mode,dma:%llx\n",
							ms->cur_transfer->rx_dma);
#else
						dev_err(&ms->pdev->dev,
							"Warning!Rx_DMA address range error in 4G mode,dma:%x\n",
							ms->cur_transfer->rx_dma);
#endif
					MAPPING_DRAM_ACCESS_ADDR(temp_rx_dma);
				}
			}
			spi_writel(ms, SPI_RX_DST_REG, cpu_to_le32(temp_rx_dma));
			cmd |= 1 << SPI_CMD_RX_DMA_OFFSET;
		}
	}
	mb();
	spi_writel(ms, SPI_CMD_REG, cmd);

}

static inline int spi_setup_packet(struct mt_spi_t *ms)
{
	u32 packet_size, packet_loop, cfg1;

	/*set transfer packet and loop */
	if (ms->cur_transfer->len < PACKET_SIZE)
		packet_size = ms->cur_transfer->len;
	else
		packet_size = PACKET_SIZE;

	if (ms->cur_transfer->len % packet_size) {
		packet_loop = ms->cur_transfer->len / packet_size + 1;
		/*parameter not correct, there will be more data transfer,notice user to change */
		dev_err(&ms->pdev->dev, "ERROR!!The lens must be a multiple of %d, your len %u\n\n", PACKET_SIZE,
			ms->cur_transfer->len);
		return -EINVAL;
	}
	packet_loop = (ms->cur_transfer->len) / packet_size;

	SPI_DBG("The packet_size:0x%x packet_loop:0x%x\n", packet_size, packet_loop);

	cfg1 = spi_readl(ms, SPI_CFG1_REG);
	cfg1 &= ~(SPI_CFG1_PACKET_LENGTH_MASK + SPI_CFG1_PACKET_LOOP_MASK);
	cfg1 |= (packet_size - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET;
	cfg1 |= (packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET;
	spi_writel(ms, SPI_CFG1_REG, cfg1);

	return 0;
}

/*
static int spi_is_busy(struct mt_spi_t *ms)
{
	u32 reg_val;
	unsigned long flags;

	spin_lock_irqsave(&ms->spin_lock, flags);
	reg_val = spi_readl(ms, SPI_STATUS1_REG);
	spin_unlock_irqrestore(&ms->spin_lock, flags);
	if ( reg_val & 0x1) {
		SPI_DBG("is not busy.\n");
		return 0;
	}else {
		SPI_DBG("is busy.\n");
		return 1;
	}
}
*/
static inline void spi_start_transfer(struct mt_spi_t *ms)
{
	u32 reg_val;

	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val |= 1 << SPI_CMD_ACT_OFFSET;

	/*All register must be prepared before setting the start bit [SMP] */
	mb();
	spi_writel(ms, SPI_CMD_REG, reg_val);

}

static inline void spi_resume_transfer(struct mt_spi_t *ms)
{
	u32 reg_val;

	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RESUME_MASK;
	reg_val |= 1 << SPI_CMD_RESUME_OFFSET;
	/*All register must be prepared before setting the start bit [SMP] */
	mb();
	spi_writel(ms, SPI_CMD_REG, reg_val);

}

static void reset_spi(struct mt_spi_t *ms)
{
	u32 reg_val;

	/*set the software reset bit in SPI_CMD_REG. */
	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RST_MASK;
	reg_val |= 1 << SPI_CMD_RST_OFFSET;
	spi_writel(ms, SPI_CMD_REG, reg_val);

	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RST_MASK;
	spi_writel(ms, SPI_CMD_REG, reg_val);

}

static inline int is_last_xfer(struct spi_message *msg, struct spi_transfer *xfer)
{
	return msg->transfers.prev == &xfer->transfer_list;
}

static int transfer_dma_mapping(struct mt_spi_t *ms, u8 mode, struct spi_transfer *xfer)
{
	struct device *dev = &ms->pdev->dev;

	xfer->tx_dma = xfer->rx_dma = INVALID_DMA_ADDRESS;

	if ((mode == DMA_TRANSFER) || (mode == OTHER1)) {
		if (xfer->tx_buf) {
			xfer->tx_dma = dma_map_single(dev, (void *)xfer->tx_buf, xfer->len, DMA_TO_DEVICE);

			if (dma_mapping_error(dev, xfer->tx_dma)) {
				dev_err(&ms->pdev->dev, "dma mapping tx_buf error.\n");
				return -ENOMEM;
			}
		}
	}
	if ((mode == DMA_TRANSFER) || (mode == OTHER2)) {
		if (xfer->rx_buf) {
			xfer->rx_dma = dma_map_single(dev, xfer->rx_buf, xfer->len, DMA_FROM_DEVICE);

			if (dma_mapping_error(dev, xfer->rx_dma)) {
				if (xfer->tx_buf)
					dma_unmap_single(dev, xfer->tx_dma, xfer->len, DMA_TO_DEVICE);

				dev_err(&ms->pdev->dev, "dma mapping rx_buf error.\n");
				return -ENOMEM;
			}
		}
	}
	if (mode != FIFO_TRANSFER)
		SPI_DBG("Transfer_dma_mapping success.\n");
#if 0
	/*cross 1K test, non 4byte alignment */
	{
#define SPI_CROSS_ALIGN_OFFSET 1009

		u8 *p;

		SPI_DBG("Transfer dma addr:Tx:0x%x, Rx:0x%x, before\n", xfer->tx_dma, xfer->rx_dma);
		SPI_DBG("Transfer buf addr:Tx:0x%x, Rx:0x%x, before\n", xfer->tx_buf, xfer->rx_buf);
		xfer->len = 32;
		p = (u8 *) xfer->tx_dma;
		xfer->tx_dma = (u32 *) (p + SPI_CROSS_ALIGN_OFFSET);
		p = (u8 *) xfer->rx_dma;
		xfer->rx_dma = (u32 *) (p + SPI_CROSS_ALIGN_OFFSET);

		p = (u8 *) xfer->tx_buf;
		xfer->tx_buf = (u32 *) (p + SPI_CROSS_ALIGN_OFFSET);
		p = (u8 *) xfer->rx_buf;
		xfer->rx_buf = (u32 *) (p + SPI_CROSS_ALIGN_OFFSET);
		SPI_DBG("Transfer dma addr:Tx:0x%x, Rx:0x%x\n", xfer->tx_dma, xfer->rx_dma);
		SPI_DBG("Transfer buf addr:Tx:0x%x, Rx:0x%x\n", xfer->tx_buf, xfer->rx_buf);
	}
#endif
	return 0;
}

static void transfer_dma_unmapping(struct mt_spi_t *ms, struct spi_transfer *xfer)
{
	struct device *dev = &ms->pdev->dev;

	if ((xfer->tx_dma != INVALID_DMA_ADDRESS) && (xfer->tx_dma != 0)) {
		dma_unmap_single(dev, xfer->tx_dma, xfer->len, DMA_TO_DEVICE);
		xfer->tx_dma = INVALID_DMA_ADDRESS;
	}
	if ((xfer->rx_dma != INVALID_DMA_ADDRESS) && (xfer->rx_dma != 0)) {
		dma_unmap_single(dev, xfer->rx_dma, xfer->len, DMA_FROM_DEVICE);
		xfer->rx_dma = INVALID_DMA_ADDRESS;
	}

}

static void mt_spi_msg_done(struct mt_spi_t *ms, struct spi_message *msg, int status);
static void mt_spi_next_message(struct mt_spi_t *ms);
static int mt_do_spi_setup(struct mt_spi_t *ms, struct mt_chip_conf *chip_config);

static int mt_spi_next_xfer(struct mt_spi_t *ms, struct spi_message *msg)
{
	struct spi_transfer *xfer;
	struct mt_chip_conf *chip_config = (struct mt_chip_conf *)msg->state;
	u8 mode, cnt, i;
	int ret = 0;
	char xfer_rec[16];
#ifdef SPI_AUTO_SELECT_MODE
	u32 reg_val = 0;
#endif
	bool spi1_flag = 0;

	if (unlikely(!ms)) {
		dev_err(&msg->spi->dev, "master wrapper is invalid\n");
		ret = -EINVAL;
		goto fail;
	}
	if (unlikely(!msg)) {
		dev_err(&msg->spi->dev, "msg is invalid\n");
		ret = -EINVAL;
		goto fail;
	}
	if (unlikely(!msg->state)) {
		dev_err(&msg->spi->dev, "msg config is invalid\n");
		ret = -EINVAL;
		goto fail;
	}

	if (unlikely(!is_interrupt_enable(ms))) {
		dev_err(&msg->spi->dev, "interrupt is disable\n");
		ret = -EINVAL;
		goto fail;
	}
#ifdef SPI_AUTO_SELECT_MODE
	if (ms->cur_transfer.len > SPI_DATA_SIZE) {
		chip_config->com_mod = DMA_TRANSFER;
		SPI_DBG("SPI auto select DMA mode\n");
		reg_val = spi_readl(ms, SPI_CMD_REG);
		reg_val |= ((1 << SPI_CMD_TX_DMA_OFFSET) | (1 << SPI_CMD_RX_DMA_OFFSET));
		spi_writel(ms, SPI_CMD_REG, reg_val);
		SPI_DBG("SPI auto select CMD = 0x%x\n", spi_readl(ms, SPI_CMD_REG));
	} else {
		SPI_DBG("SPI auto select do nothing\n");
	}
#endif

	mode = chip_config->com_mod;

	xfer = ms->cur_transfer;

	SPI_DBG("start xfer 0x%p, mode %d, len %u\n", xfer, mode, xfer->len);

	if ((mode == FIFO_TRANSFER) || (mode == OTHER1) || (mode == OTHER2)) {
		if (xfer->len > SPI_FIFO_SIZE) {
			ret = -EINVAL;
			dev_err(&msg->spi->dev, "xfer len is invalid over fifo size\n");
			goto fail;
		}
	}

	if (is_last_xfer(msg, xfer)) {
		SPI_DBG("The last xfer.\n");
		ms->next_transfer = NULL;
		clear_pause_bit(ms);
	} else {
		SPI_DBG("Not the last xfer.\n");
		ms->next_transfer = list_entry(xfer->transfer_list.next, struct spi_transfer, transfer_list);
	}
	/*disable DMA */
	spi_disable_dma(ms);

	ret = spi_setup_packet(ms);
	if (ret < 0)
		goto fail;

	/*Using FIFO to send data */
	if ((mode == FIFO_TRANSFER) || (mode == OTHER2)) {
		cnt = (xfer->len % 4) ? (xfer->len / 4 + 1) : (xfer->len / 4);
		for (i = 0; i < cnt; i++) {
			if (xfer->tx_buf == NULL)
				spi_writel(ms, SPI_TX_DATA_REG, 0);
			else {
				spi_writel(ms, SPI_TX_DATA_REG, *((u32 *) xfer->tx_buf + i));
				SPI_INFO(&msg->spi->dev, "tx_buf data is:%x\n", *((u32 *) xfer->tx_buf + i));
				SPI_INFO(&msg->spi->dev, "tx_buf addr is:%p\n", (u32 *) xfer->tx_buf + i);
			}
		}
	}
	/*Using DMA to send data */
	if ((mode == DMA_TRANSFER) || (mode == OTHER1) || (mode == OTHER2))
		spi_enable_dma(ms, mode);

#ifdef SPI_VERBOSE
	spi_dump_reg(ms);	/*Dump register before transfer */
#endif

	if (ms->running == PAUSED) {
		SPI_DBG("pause status to resume.\n");

		spi_resume_transfer(ms);
	} else if (ms->running == IDLE) {
		SPI_DBG("The xfer start\n");
		/*if there is only one transfer, pause bit should not be set. */
		if (is_pause_mode(msg) && !is_last_xfer(msg, xfer)) {
			SPI_DBG("set pause mode.\n");
			set_pause_bit(ms);
		}
		/*All register must be prepared before setting the start bit [SMP] */

		spi_start_transfer(ms);

	} else {
		dev_err(&msg->spi->dev, "Wrong status\n");
		ret = -1;
		goto fail;
	}
	sprintf(xfer_rec, "xfer,%3d", xfer->len);
	spi_rec_time(xfer_rec);

	ms->running = INPROGRESS;

	/*exit pause mode */
	if (is_pause_mode(msg) && is_last_xfer(msg, xfer))
		clear_resume_bit(ms);

	return 0;

 fail:
	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
		if ((!msg->is_dma_mapped)) {
			spi1_flag = (enable_4G() && (1 == ms->pdev->id));
			if (!spi1_flag)
				transfer_dma_unmapping(ms, xfer);
		}
	}
	ms->running = IDLE;
	mt_spi_msg_done(ms, msg, ret);

	return ret;
}

static void mt_spi_msg_done(struct mt_spi_t *ms, struct spi_message *msg, int status)
{

	list_del(&msg->queue);
	msg->status = status;

	SPI_DBG("msg:%p complete(%d): %u bytes transferred\n", msg, status, msg->actual_length);

	spi_disable_dma(ms);

	msg->complete(msg->context);

	ms->running = IDLE;
	ms->cur_transfer = NULL;
	ms->next_transfer = NULL;

	/*disable_clk(); */
	spi_rec_time("msge");
#ifdef SPI_REC_DEBUG

	if (!(rec_count % SPI_REC_NUM)) {
		atomic_inc(&rec_log_count);
		schedule_work(&mt_spi_workqueue);
	}
	/*      if(atomic_read(&rec_log_count) > 0){
	   schedule_work(&mt_spi_workqueue);
	   tasklet_schedule(&spi_tasklet);
	   SPI_OUT_REC_TIME;
	   rec_count = 0;
	   } */
#endif

	/* continue if needed */
	if (list_empty(&ms->queue)) {
		SPI_DBG("All msg is completion.\n\n");
		/*clock and gpio reset */
		spi_gpio_reset(ms);
		/*disable_clk(); */
		disable_clk(ms);
		SPI_DBG("=======disable_clk=======.\n");
		/*schedule_work(&mt_spi_msgdone_workqueue);//disable clock */

		wake_unlock(&ms->wk_lock);
	} else
		mt_spi_next_message(ms);
}

static void mt_spi_next_message(struct mt_spi_t *ms)
{
	struct spi_message *msg;
	struct mt_chip_conf *chip_config;
	char msg_addr[32];

	msg = list_entry(ms->queue.next, struct spi_message, queue);
	chip_config = (struct mt_chip_conf *)msg->state;

#ifdef SPI_REC_DEBUG
	spi_speed = SPI_CLOCK_PERIED / (chip_config->low_time + chip_config->high_time);
	sprintf(msg_addr, "msgn,%4dKHz", spi_speed);
#else
	sprintf(msg_addr, "msgn");
#endif

	spi_rec_time(msg_addr);

	SPI_DBG("start transfer message:0x%p\n", msg);
	ms->cur_transfer = list_entry(msg->transfers.next, struct spi_transfer, transfer_list);

	/*clock and gpio set */
/*	spi_gpio_set(ms);
	enable_clk();*/

	reset_spi(ms);
	mt_do_spi_setup(ms, chip_config);
	mt_spi_next_xfer(ms, msg);

}

static int mt_spi_transfer(struct spi_device *spidev, struct spi_message *msg)
{
	struct spi_master *master;
	struct mt_spi_t *ms;
	struct spi_transfer *xfer;
	struct mt_chip_conf *chip_config;
	unsigned long flags;
	char msg_addr[32];
	bool spi1_flag = 0;

	master = spidev->master;
	ms = spi_master_get_devdata(master);

	/*wake_lock ( &ms->wk_lock ); */
	SPI_DBG("enter,start add msg:0x%p\n", msg);

	if (unlikely(!msg)) {
		dev_err(&spidev->dev, "msg is NULL pointer.\n");
		msg->status = -EINVAL;
		goto out;
	}
	if (unlikely(list_empty(&msg->transfers))) {
		dev_err(&spidev->dev, "the message is NULL.\n");
		msg->status = -EINVAL;
		msg->actual_length = 0;
		goto out;
	}

	/*if device don't config chip, set default */
	if (master->setup(spidev)) {
		dev_err(&spidev->dev, "set up error.\n");
		msg->status = -EINVAL;
		msg->actual_length = 0;
		goto out;
	}
	sprintf(msg_addr, "msgs:%p", msg);
	spi_rec_time(msg_addr);

	chip_config = (struct mt_chip_conf *)spidev->controller_data;
	msg->state = chip_config;

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
		if (!((xfer->tx_buf || xfer->rx_buf) && xfer->len)) {
			dev_err(&spidev->dev, "missing tx %p or rx %p buf, len%d\n", xfer->tx_buf, xfer->rx_buf,
				xfer->len);
			msg->status = -EINVAL;
			goto out;
		}

		/*
		 * DMA map early, for performance (empties dcache ASAP) and
		 * better fault reporting.
		 *
		 * NOTE that if dma_unmap_single() ever starts to do work on
		 * platforms supported by this driver, we would need to clean
		 * up mappings for previously-mapped transfers.
		 */
		spi1_flag = (enable_4G() && (1 == ms->pdev->id));
		if ((!msg->is_dma_mapped) && (!spi1_flag)) {
			if (transfer_dma_mapping(ms, chip_config->com_mod, xfer) < 0)
				return -ENOMEM;
		}
	}
#ifdef SPI_VERBOSE
/*	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
	SPI_INFO(&spidev->dev,"xfer %p: len %04u tx %p/%08x rx %p/%08x\n",
		xfer, xfer->len,xfer->tx_buf, xfer->tx_dma, xfer->rx_buf, xfer->rx_dma);
	}*/
#endif
	msg->status = -EINPROGRESS;
	msg->actual_length = 0;

	spin_lock_irqsave(&ms->lock, flags);
	list_add_tail(&msg->queue, &ms->queue);
	SPI_DBG("add msg %p to queue\n", msg);
	if (!ms->cur_transfer) {
		wake_lock(&ms->wk_lock);
		spi_gpio_set(ms);
		/*enable_clk(); */

		enable_clk(ms);
		SPI_DBG("=======enable_clk=======.\n");
		mt_spi_next_message(ms);
	}
	spin_unlock_irqrestore(&ms->lock, flags);

	return 0;

 out:
	return -1;

}

static irqreturn_t mt_spi_interrupt(int irq, void *dev_id)
{
	struct mt_spi_t *ms = (struct mt_spi_t *)dev_id;
	struct spi_message *msg;
	struct spi_transfer *xfer;
	struct mt_chip_conf *chip_config;
	bool spi1_flag = 0;

	unsigned long flags;
	u32 reg_val, cnt;
	u8 mode, i;

	spi_rec_time("irqs");

	spin_lock_irqsave(&ms->lock, flags);
	xfer = ms->cur_transfer;
	msg = list_entry(ms->queue.next, struct spi_message, queue);

	/*Clear interrupt status first by reading the register */
	reg_val = spi_readl(ms, SPI_STATUS0_REG);
	SPI_DBG("xfer:0x%p interrupt status:%x\n", xfer, reg_val & 0x3);

	chip_config = (struct mt_chip_conf *)msg->state;
	mode = chip_config->com_mod;
	/*clear the interrupt status bits by reading the register */
	/*
	   reg_val = spi_readl(ms,SPI_STATUS0_REG);
	   SPI_DBG("xfer:0x%p interrupt status:%x\n",xfer,reg_val&0x3); */
	if ((reg_val & 0x03) == 0)
		goto out;

	spi1_flag = (enable_4G() && (1 == ms->pdev->id));
	if ((!msg->is_dma_mapped) && (!spi1_flag))
		transfer_dma_unmapping(ms, ms->cur_transfer);

	if (is_pause_mode(msg)) {
		if (ms->running == INPROGRESS)
			ms->running = PAUSED;
		else
			dev_err(&msg->spi->dev, "Wrong spi status.\n");
	} else
		ms->running = IDLE;

	if (is_fifo_read(msg) && xfer->rx_buf) {
		cnt = (xfer->len % 4) ? (xfer->len / 4 + 1) : (xfer->len / 4);
		for (i = 0; i < cnt; i++) {
			reg_val = spi_readl(ms, SPI_RX_DATA_REG);	/*get the data from rx */
			SPI_INFO(&msg->spi->dev, "SPI_RX_DATA_REG:0x%x", reg_val);
			*((u32 *) xfer->rx_buf + i) = reg_val;
		}
	}

	msg->actual_length += xfer->len;

	if (is_last_xfer(msg, xfer)) {
		mt_spi_msg_done(ms, msg, 0);
	} else {
		ms->cur_transfer = ms->next_transfer;
		mt_spi_next_xfer(ms, msg);

	}

	spin_unlock_irqrestore(&ms->lock, flags);
	return IRQ_HANDLED;
 out:
	spin_unlock_irqrestore(&ms->lock, flags);
	SPI_DBG("return IRQ_NONE.\n");
	return IRQ_NONE;
}

/* Write chip configuration to HW register */
static int mt_do_spi_setup(struct mt_spi_t *ms, struct mt_chip_conf *chip_config)
{
	u32 reg_val;

#ifdef SPI_VERBOSE
	u32 speed;
#define SPI_MODULE_CLOCK 134300
	speed = SPI_MODULE_CLOCK / (chip_config->low_time + chip_config->high_time);
	SPI_DBG("mode:%d, speed:%d KHz,CPOL%d,CPHA%d\n", chip_config->com_mod, speed, chip_config->cpol,
		chip_config->cpha);
#endif
	/*clear RST bits */
/*	reg_val = spi_readl ( ms, SPI_CMD_REG );
	reg_val &= ~ SPI_CMD_RST_MASK;
	spi_writel ( ms, SPI_CMD_REG, reg_val ); */

	/*set the timing */
	reg_val = spi_readl(ms, SPI_CFG0_REG);
	reg_val &= ~(SPI_CFG0_CS_HOLD_MASK | SPI_CFG0_CS_SETUP_MASK);
	reg_val |= ((chip_config->holdtime - 1) << SPI_CFG0_CS_HOLD_OFFSET);
	reg_val |= ((chip_config->setuptime - 1) << SPI_CFG0_CS_SETUP_OFFSET);
	spi_writel(ms, SPI_CFG0_REG, reg_val);

	reg_val = spi_readl(ms, SPI_CFG1_REG);
	reg_val &= ~(SPI_CFG1_CS_IDLE_MASK);
	reg_val |= ((chip_config->cs_idletime - 1) << SPI_CFG1_CS_IDLE_OFFSET);

	reg_val &= ~(SPI_CFG1_GET_TICK_DLY_MASK);
	reg_val |= ((chip_config->tckdly) << SPI_CFG1_GET_TICK_DLY_OFFSET);
	spi_writel(ms, SPI_CFG1_REG, reg_val);

	/*set the mlsbx and mlsbtx */
	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~(SPI_CMD_TX_ENDIAN_MASK | SPI_CMD_RX_ENDIAN_MASK);
	reg_val &= ~(SPI_CMD_TXMSBF_MASK | SPI_CMD_RXMSBF_MASK);
	reg_val &= ~(SPI_CMD_CPHA_MASK | SPI_CMD_CPOL_MASK);
	reg_val &= ~(SPI_CMD_SAMPLE_SEL_MASK | SPI_CMD_CS_POL_MASK);
	reg_val |= (chip_config->tx_mlsb << SPI_CMD_TXMSBF_OFFSET);
	reg_val |= (chip_config->rx_mlsb << SPI_CMD_RXMSBF_OFFSET);
	reg_val |= (chip_config->tx_endian << SPI_CMD_TX_ENDIAN_OFFSET);
	reg_val |= (chip_config->rx_endian << SPI_CMD_RX_ENDIAN_OFFSET);
	reg_val |= (chip_config->sample_sel << SPI_CMD_SAMPLE_SEL_OFFSET);
	reg_val |= (chip_config->cs_pol << SPI_CMD_CS_POL_OFFSET);
	reg_val |= (chip_config->cpha << SPI_CMD_CPHA_OFFSET);
	reg_val |= (chip_config->cpol << SPI_CMD_CPOL_OFFSET);
	spi_writel(ms, SPI_CMD_REG, reg_val);

	/*set pause mode */
	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_PAUSE_EN_MASK;
	reg_val &= ~SPI_CMD_PAUSE_IE_MASK;
	/*if  ( chip_config->com_mod == DMA_TRANSFER ) */
	reg_val |= (chip_config->pause << SPI_CMD_PAUSE_IE_OFFSET);
	spi_writel(ms, SPI_CMD_REG, reg_val);

	/*set finish interrupt always enable */
	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_FINISH_IE_MASK;
	/*reg_val |= ( chip_config->finish_intr << SPI_CMD_FINISH_IE_OFFSET ); */
	reg_val |= (1 << SPI_CMD_FINISH_IE_OFFSET);

	spi_writel(ms, SPI_CMD_REG, reg_val);

	/*set the communication of mode */
	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_TX_DMA_MASK;
	reg_val &= ~SPI_CMD_RX_DMA_MASK;
	spi_writel(ms, SPI_CMD_REG, reg_val);

	/*set deassert mode */
	reg_val = spi_readl(ms, SPI_CMD_REG);
	reg_val &= ~SPI_CMD_DEASSERT_MASK;
	reg_val |= (chip_config->deassert << SPI_CMD_DEASSERT_OFFSET);
	spi_writel(ms, SPI_CMD_REG, reg_val);

	spi_writel(ms, SPI_PAD_SEL_REG, ms->pad_macro);
/*
#if defined(GPIO_SPI_CS_PIN) && defined(GPIO_SPI_SCK_PIN)
	&&defined(GPIO_SPI_MISO_PIN) && defined(GPIO_SPI_MOSI_PIN)
		spi_writel(ms, SPI_PAD_SEL_REG, 0);
#elif defined(GPIO_SPI2_CS_PIN) && defined(GPIO_SPI2_SCK_PIN)
	&&defined(GPIO_SPI2_MISO_PIN) && defined(GPIO_SPI2_MOSI_PIN)
		spi_writel(ms, SPI_PAD_SEL_REG, 1);
#endif
*/
	/*set the timing */
	reg_val = spi_readl(ms, SPI_CFG2_REG);
	reg_val &= ~(SPI_CFG0_SCK_HIGH_MASK | SPI_CFG0_SCK_LOW_MASK);
	reg_val |= ((chip_config->high_time - 1) << SPI_CFG0_SCK_HIGH_OFFSET);
	reg_val |= ((chip_config->low_time - 1) << SPI_CFG0_SCK_LOW_OFFSET);
	spi_writel(ms, SPI_CFG2_REG, reg_val);

#if 0
	/*set ultra high priority */
	reg_val &= ~SPI_ULTRA_HIGH_EN_MASK;
	reg_val |= chip_config->ulthigh << SPI_ULTRA_HIGH_EN_OFFSET;

	reg_val &= ~SPI_ULTRA_HIGH_THRESH_MASK;
	reg_val |= (chip_config->ulthgh_thrsh << SPI_ULTRA_HIGH_THRESH_OFFSET);

	spi_writel(ms, SPI_ULTRA_HIGH_REG, reg_val);
#endif
	return 0;
}

static int mt_spi_setup(struct spi_device *spidev)
{
	struct spi_master *master;
	struct mt_spi_t *ms;

	struct mt_chip_conf *chip_config = NULL;

	if (!spidev) {
		pr_err("spidev is null. error\n");
		/* dev_err(&spidev->dev, "spi device %s: error.\n", dev_name(&spidev->dev)); */
		return -EINVAL;
	}
	master = spidev->master;
	ms = spi_master_get_devdata(master);

	if (spidev->chip_select >= master->num_chipselect) {
		dev_err(&spidev->dev, "spi device chip select excesses the number of master's chipselect number.\n");
		return -EINVAL;
	}

	chip_config = (struct mt_chip_conf *)spidev->controller_data;
	if (!chip_config) {
		chip_config = kzalloc(sizeof(struct mt_chip_conf), GFP_KERNEL);
		if (!chip_config) {
			dev_err(&spidev->dev, " spidev %s: can not get enough memory.\n", dev_name(&spidev->dev));
			return -ENOMEM;
		}
		SPI_DBG("device %s: set default at chip's runtime state\n", dev_name(&spidev->dev));

		chip_config->setuptime = 3;
		chip_config->holdtime = 3;
		chip_config->high_time = 10;
		chip_config->low_time = 10;
		chip_config->cs_idletime = 2;
		chip_config->ulthgh_thrsh = 0;

		chip_config->cs_pol = 0;
		chip_config->sample_sel = 0;
		chip_config->cpol = 0;
		chip_config->cpha = 1;
		chip_config->rx_mlsb = 1;
		chip_config->tx_mlsb = 1;
		chip_config->tx_endian = 0;
		chip_config->rx_endian = 0;

		chip_config->com_mod = DMA_TRANSFER;
		chip_config->pause = 0;
		chip_config->finish_intr = 1;
		chip_config->deassert = 0;
		chip_config->ulthigh = 0;
		chip_config->tckdly = 0;

		spidev->controller_data = chip_config;
	}

	SPI_INFO(&spidev->dev, "set up chip config,mode:%d\n", chip_config->com_mod);

	/*check chip configuration valid */

	ms->config = chip_config;
	if (!((chip_config->pause == PAUSE_MODE_ENABLE) || (chip_config->pause == PAUSE_MODE_DISABLE)) ||
	    !((chip_config->cpol == SPI_CPOL_0) || (chip_config->cpol == SPI_CPOL_1)) ||
	    !((chip_config->cpha == SPI_CPHA_0) || (chip_config->cpha == SPI_CPHA_1)) ||
	    !((chip_config->tx_mlsb == SPI_LSB) || (chip_config->tx_mlsb == SPI_MSB)) ||
	    !((chip_config->com_mod == FIFO_TRANSFER) || (chip_config->com_mod == DMA_TRANSFER) ||
	      (chip_config->com_mod == OTHER1) || (chip_config->com_mod == OTHER2))) {
		return -EINVAL;
	}
	return 0;
}

static void mt_spi_cleanup(struct spi_device *spidev)
{
	struct spi_master *master;
	struct mt_spi_t *ms;

	master = spidev->master;
	ms = spi_master_get_devdata(master);

	SPI_DBG("Calling mt_spi_cleanup.\n");

	spidev->controller_data = NULL;
	spidev->master = NULL;

}

static int mt_spi_probe(struct platform_device *pdev)
{
	int ret = 0;
	int irq;
	struct resource *regs;
	struct spi_master *master;
	struct mt_spi_t *ms;
#ifdef CONFIG_OF
	void __iomem *spi_base;
	/*unsigned int pin[4];
	unsigned int pin_mode[4];
	unsigned int if_config = 1;
	unsigned int i;*/
#endif

	master = spi_alloc_master(&pdev->dev, sizeof(struct mt_spi_t));
	if (!master) {
		dev_err(&pdev->dev, " device %s: alloc spi master fail.\n", dev_name(&pdev->dev));
		goto out;
	}
	ms = spi_master_get_devdata(master);

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "get resource regs NULL.\n");
		return -ENXIO;
	}
	if (!request_mem_region(regs->start, resource_size(regs), pdev->name)) {
		dev_err(&pdev->dev, "SPI register memory region failed");
		return -ENOMEM;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "platform_get_irq error. get invalid irq\n");
		return irq;
	}
#ifdef CONFIG_OF
	spi_base = of_iomap(pdev->dev.of_node, 0);
	if (!spi_base) {
		dev_err(&pdev->dev, "SPI iomap failed\n");
		return -ENODEV;
	}
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node) {
		if (of_property_read_u32(pdev->dev.of_node, "cell-index", &pdev->id)) {
			dev_err(&pdev->dev, "SPI get cell-index failed\n");
			return -ENODEV;
		}
		if (of_property_read_u32(pdev->dev.of_node, "spi-padmacro",
							 &ms->pad_macro)) {
			dev_err(&pdev->dev, "SPI get pad macro fail failed\n");
			return -ENODEV;
		}
		ms->clk_main = devm_clk_get(&pdev->dev, "spi-main");
		if (IS_ERR(ms->clk_main)) {
			dev_err(&pdev->dev, "cannot get spi1 main clock or dma clock. main clk err : %ld .\n",
				PTR_ERR(ms->clk_main));
			return PTR_ERR(ms->clk_main);
		}
	}
	SPI_DBG("SPI reg: 0x%p,  irq: %d, id: %d, pad_macro: %u\n", spi_base, irq, pdev->id, ms->pad_macro);
#endif

	master->dev.of_node = pdev->dev.of_node;
	/*hardware can only connect 1 slave.if you want to multiple, using gpio CS */
	master->num_chipselect = 2;

	master->mode_bits = (SPI_CPOL | SPI_CPHA);
	master->bus_num = pdev->id;
	master->setup = mt_spi_setup;
	master->transfer = mt_spi_transfer;
	master->cleanup = mt_spi_cleanup;
	platform_set_drvdata(pdev, master);


	/*ms = spi_master_get_devdata(master); */
#ifdef CONFIG_OF
	ms->regs = spi_base;
#else
	ms->regs = ioremap(regs->start, resource_size(regs));
#endif
	ms->pdev = pdev;
	ms->irq = irq;
	ms->running = IDLE;
	ms->cur_transfer = NULL;
	ms->next_transfer = NULL;
	wake_lock_init(&ms->wk_lock, WAKE_LOCK_SUSPEND, "spi_wakelock");

	spin_lock_init(&ms->lock);
	INIT_LIST_HEAD(&ms->queue);

	SPI_INFO(&pdev->dev, "Controller at 0x%p (irq %d)\n", ms->regs, irq);
#ifdef CONFIG_OF
	ret = request_irq(irq, mt_spi_interrupt, IRQF_TRIGGER_NONE, dev_name(&pdev->dev), ms);
#else
	ret = request_irq(irq, mt_spi_interrupt, IRQF_TRIGGER_LOW, dev_name(&pdev->dev), ms);
#endif
	if (ret) {
		dev_err(&pdev->dev, "registering interrupt handler fails.\n");
		goto out;
	}

	spi_master_set_devdata(master, ms);
	spi_pdev[pdev->id] = pdev;
#if !defined(CONFIG_MTK_CLKMGR)
	/*
	 * prepare the clock source
	 */
	ret = clk_prepare(ms->clk_main);
#endif
	/*
	 * enable clk before access spi register
	 */
	enable_clk(ms);
	reset_spi(ms);
	/*
	 * disable clk when finishing access spi register
	 */
	disable_clk(ms);

	ret = spi_register_master(master);
	if (ret) {
		dev_err(&pdev->dev, "spi_register_master fails.\n");
		goto out_free;
	} else {
		SPI_DBG("spi register master success.\n");
		return 0;
	}
	pr_debug("SPI probe end\n");
 out_free:
	free_irq(irq, ms);
 out:
	spi_master_put(master);
	return ret;
}

static int __exit mt_spi_remove(struct platform_device *pdev)
{
	struct mt_spi_t *ms;
	struct spi_message *msg;
	struct spi_master *master = platform_get_drvdata(pdev);

	if (!master) {
		dev_err(&pdev->dev, "master %s: is invalid.\n", dev_name(&pdev->dev));
		return -EINVAL;
	}
	ms = spi_master_get_devdata(master);

	list_for_each_entry(msg, &ms->queue, queue) {
		msg->status = -ESHUTDOWN;
		msg->complete(msg->context);
	}
	ms->cur_transfer = NULL;
	ms->running = IDLE;

	reset_spi(ms);

	free_irq(ms->irq, master);
	spi_unregister_master(master);
	return 0;
}

#ifdef CONFIG_PM
static int mt_spi_suspend(struct platform_device *pdev, pm_message_t message)
{
	/* if interrupt is enabled,
	 * then wait for interrupt complete. */
	struct mt_spi_t *ms;
	struct spi_master *master = platform_get_drvdata(pdev);

	ms = spi_master_get_devdata(master);

#if !defined(CONFIG_MTK_CLKMGR)
	/*
	 * unprepare the clock source
	 */
	clk_unprepare(ms->clk_main);
	SPI_DBG("spi mt_spi_suspend clk_unpreparer success.\n");
#endif
	return 0;
}

static int mt_spi_resume(struct platform_device *pdev)
{
#if !defined(CONFIG_MTK_CLKMGR)
	int ret;
#endif
	struct mt_spi_t *ms;
	struct spi_master *master = platform_get_drvdata(pdev);

	ms = spi_master_get_devdata(master);

#if !defined(CONFIG_MTK_CLKMGR)
	/*
	 * prepare the clock source
	 */
	ret = clk_prepare(ms->clk_main);
	SPI_DBG("spi mt_spi_resume clk_prepare success.\n");
#endif
	return 0;
}
#else
#define mt_spi_suspend NULL
#define mt_spi_resume NULL
#endif

static const struct of_device_id mt_spi_of_match[] = {
	{.compatible = "mediatek,mt6735-spi",},
	{.compatible = "mediatek,mt6735m-spi",},
	{.compatible = "mediatek,mt6753-spi",},
	{.compatible = "mediatek,mt6797-spi",},
	{},
};

MODULE_DEVICE_TABLE(of, mt_spi_of_match);

struct platform_driver mt_spi_driver = {
	.driver = {
		   .name = "mt-spi",
		   .owner = THIS_MODULE,
		   .of_match_table = mt_spi_of_match,
		   },
	.probe = mt_spi_probe,
	.suspend = mt_spi_suspend,
	.resume = mt_spi_resume,
	.remove = __exit_p(mt_spi_remove),
};

static int __init mt_spi_init(void)
{
	int ret;

	pr_debug("SPI init!\n");
	ret = platform_driver_register(&mt_spi_driver);
	return ret;
}

static void __exit mt_spi_exit(void)
{
	platform_driver_unregister(&mt_spi_driver);
}

module_init(mt_spi_init);
module_exit(mt_spi_exit);

MODULE_DESCRIPTION("mt SPI Controller driver");
MODULE_AUTHOR("Ranran Lu <ranran.lu@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: mt_spi");
