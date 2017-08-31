/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>

#include <linux/slab.h>
#include <linux/spinlock.h>

#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/printk.h>

#include <mt-plat/sync_write.h>

#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include "i2c-mtk.h"
#include <m4u.h>

#include "ccu_drv.h"
#include "ccu_cmn.h"
#include "ccu_reg.h"
#include "ccu_n3d_a.h"

/*******************************************************************************
*
********************************************************************************/

#define CCU_DEV_NAME            "ccu"


ccu_device_t *g_ccu_device;
static ccu_power_t power;

static wait_queue_head_t wait_queue_deque;
static wait_queue_head_t wait_queue_enque;

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source ccu_wake_lock;
#else
struct wake_lock ccu_wake_lock;
#endif
static volatile int g_bWaitLock;

static irqreturn_t ccu_isr_callback_xxx(int rrq, void *device_id);

typedef irqreturn_t(*ccu_isr_fp_t) (int, void *);

typedef struct {
	ccu_isr_fp_t irq_fp;
	unsigned int int_number;
	char device_name[16];
} ccu_isr_callback_t;

/* int number is got from kernel api */
const ccu_isr_callback_t ccu_isr_callbacks[CCU_IRQ_NUM_TYPES] = {
	/* The last used be mapping to device node. Must be the same name with that in device node. */
	{ccu_isr_callback_xxx, 0, "ccu2"}
};

static irqreturn_t ccu_isr_callback_xxx(int irq, void *device_id)
{
	LOG_DBG("ccu_isr_callback_xxx:0x%x\n", irq);
	return IRQ_HANDLED;
}


static int ccu_probe(struct platform_device *dev);

static int ccu_remove(struct platform_device *dev);

static int ccu_suspend(struct platform_device *dev, pm_message_t mesg);

static int ccu_resume(struct platform_device *dev);

/*---------------------------------------------------------------------------*/
/* CCU Driver: pm operations                                                 */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
int ccu_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	return ccu_suspend(pdev, PMSG_SUSPEND);
}

int ccu_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	return ccu_resume(pdev);
}

/* extern void mt_irq_set_sens(unsigned int irq, unsigned int sens); */
/* extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity); */
int ccu_pm_restore_noirq(struct device *device)
{
#ifndef CONFIG_OF
	mt_irq_set_sens(CAM0_IRQ_BIT_ID, MT_LEVEL_SENSITIVE);
	mt_irq_set_polarity(CAM0_IRQ_BIT_ID, MT_POLARITY_LOW);
#endif
	return 0;
}
#else
#define ccu_pm_suspend NULL
#define ccu_pm_resume  NULL
#define ccu_pm_restore_noirq NULL
#endif

const struct dev_pm_ops ccu_pm_ops = {
	.suspend = ccu_pm_suspend,
	.resume = ccu_pm_resume,
	.freeze = ccu_pm_suspend,
	.thaw = ccu_pm_resume,
	.poweroff = ccu_pm_suspend,
	.restore = ccu_pm_resume,
	.restore_noirq = ccu_pm_restore_noirq,
};


/*---------------------------------------------------------------------------*/
/* CCU Driver: Prototype                                                     */
/*---------------------------------------------------------------------------*/

static const struct of_device_id ccu_of_ids[] = {
	{.compatible = "mediatek,ccu",},
	{.compatible = "mediatek,ccu_camsys",},
	{.compatible = "mediatek,n3d_ctl_a",},
	{}
};

static struct platform_driver ccu_driver = {
	.probe = ccu_probe,
	.remove = ccu_remove,
	.suspend = ccu_suspend,
	.resume = ccu_resume,
	.driver = {
		   .name = CCU_DEV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = ccu_of_ids,
#endif
#ifdef CONFIG_PM
		   .pm = &ccu_pm_ops,
#endif
		   }
};

/*--todo: check if need id-table & name, id of_match_table is given*/
#if 1
static int ccu_i2c_probe_main(struct i2c_client *client, const struct i2c_device_id *id);
static int ccu_i2c_probe_sub(struct i2c_client *client, const struct i2c_device_id *id);
static int ccu_i2c_remove(struct i2c_client *client);

static enum CCU_I2C_CHANNEL g_ccuI2cChannel = CCU_I2C_CHANNEL_UNDEF;
static struct i2c_client *g_ccuI2cClientMain;
static struct i2c_client *g_ccuI2cClientSub;

#define MAX_I2C_CMD_LEN 255
#define CCU_I2C_APDMA_TXLEN 128
#define CCU_I2C_MAIN_HW_DRVNAME  "ccu_i2c_main_hwtrg"
#define CCU_I2C_SUB_HW_DRVNAME  "ccu_i2c_sub_hwtrg"


static const struct i2c_device_id ccu_i2c_main_ids[] = { {CCU_I2C_MAIN_HW_DRVNAME, 0}, {} };
static const struct i2c_device_id ccu_i2c_sub_ids[] = { {CCU_I2C_SUB_HW_DRVNAME, 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id ccu_i2c_main_driver_of_ids[] = {
	{.compatible = "mediatek,ccu_sensor_i2c_main_hw",},
	{}
};

static const struct of_device_id ccu_i2c_sub_driver_of_ids[] = {
	{.compatible = "mediatek,ccu_sensor_i2c_sub_hw",},
	{}
};
#endif

struct i2c_driver ccu_i2c_main_driver = {
	.probe = ccu_i2c_probe_main,
	.remove = ccu_i2c_remove,
	.driver = {
		   .name = CCU_I2C_MAIN_HW_DRVNAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = ccu_i2c_main_driver_of_ids,
#endif
		   },
	.id_table = ccu_i2c_main_ids,
};

struct i2c_driver ccu_i2c_sub_driver = {
	.probe = ccu_i2c_probe_sub,
	.remove = ccu_i2c_remove,
	.driver = {
		   .name = CCU_I2C_SUB_HW_DRVNAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = ccu_i2c_sub_driver_of_ids,
#endif
		   },
	.id_table = ccu_i2c_sub_ids,
};


/*---------------------------------------------------------------------------*/
/* CCU Driver: i2c driver funcs                                              */
/*---------------------------------------------------------------------------*/
static int ccu_i2c_probe_main(struct i2c_client *client, const struct i2c_device_id *id)
{
	/*int i4RetValue = 0;*/
	LOG_DBG("[ccu_i2c_probe] Attach I2C for HW trriger g_ccuI2cClientMain %p\n", client);

	/* get sensor i2c client */
	/*--todo: add subcam implementation*/
	g_ccuI2cClientMain = client;

	/* set I2C clock rate */
	/*#ifdef CONFIG_MTK_I2C_EXTENSION*/
	/*g_pstI2Cclient3->timing = 100;*/ /* 100k */
	/*g_pstI2Cclient3->ext_flag &= ~I2C_POLLING_FLAG;*/ /* No I2C polling busy waiting */
	/*#endif*/

	LOG_DBG("[ccu_i2c_probe] Attached!!\n");
	return 0;
}

static int ccu_i2c_probe_sub(struct i2c_client *client, const struct i2c_device_id *id)
{
	/*int i4RetValue = 0;*/
	LOG_DBG("[ccu_i2c_probe] Attach I2C for HW trriger g_ccuI2cClientSub %p\n", client);

	g_ccuI2cClientSub = client;

	LOG_DBG("[ccu_i2c_probe] Attached!!\n");
	return 0;
}


static struct i2c_client *getCcuI2cClient(void)
{
	switch (g_ccuI2cChannel) {
	case CCU_I2C_CHANNEL_MAINCAM:
		{
			return g_ccuI2cClientMain;
		}
	case CCU_I2C_CHANNEL_SUBCAM:
		{
			return g_ccuI2cClientSub;
		}
	default:
		{
			return MNULL;
		}
	}
}

static int ccu_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static inline u32 i2c_readl_dma(struct mt_i2c *i2c, u8 offset)
{
	return readl(i2c->pdmabase + offset);
}

static inline void i2c_writel_dma(u32 value, struct mt_i2c *i2c, u8 offset)
{
	writel(value, i2c->pdmabase + offset);
}

static inline u16 i2c_readw(struct mt_i2c *i2c, u8 offset)
{
	return readw(i2c->base + offset);
}

static inline void i2c_writew(u16 value, struct mt_i2c *i2c, u8 offset)
{
	writew(value, i2c->base + offset);
}

static struct i2c_dma_info g_dma_reg;
static void ccu_record_i2c_dma_info(struct mt_i2c *i2c)
{
	g_dma_reg.base = (unsigned long)i2c->pdmabase;
	g_dma_reg.int_flag = i2c_readl_dma(i2c, OFFSET_INT_FLAG);
	g_dma_reg.int_en = i2c_readl_dma(i2c, OFFSET_INT_EN);
	g_dma_reg.en = i2c_readl_dma(i2c, OFFSET_EN);
	g_dma_reg.rst = i2c_readl_dma(i2c, OFFSET_RST);
	g_dma_reg.stop = i2c_readl_dma(i2c, OFFSET_STOP);
	g_dma_reg.flush = i2c_readl_dma(i2c, OFFSET_FLUSH);
	g_dma_reg.con = i2c_readl_dma(i2c, OFFSET_CON);
	g_dma_reg.tx_mem_addr = i2c_readl_dma(i2c, OFFSET_TX_MEM_ADDR);
	g_dma_reg.rx_mem_addr = i2c_readl_dma(i2c, OFFSET_RX_MEM_ADDR);
	g_dma_reg.tx_len = i2c_readl_dma(i2c, OFFSET_TX_LEN);
	g_dma_reg.rx_len = i2c_readl_dma(i2c, OFFSET_RX_LEN);
	g_dma_reg.int_buf_size = i2c_readl_dma(i2c, OFFSET_INT_BUF_SIZE);
	g_dma_reg.debug_sta = i2c_readl_dma(i2c, OFFSET_DEBUG_STA);
	g_dma_reg.tx_mem_addr2 = i2c_readl_dma(i2c, OFFSET_TX_MEM_ADDR2);
	g_dma_reg.rx_mem_addr2 = i2c_readl_dma(i2c, OFFSET_RX_MEM_ADDR2);
}

void ccu_i2c_dump_info(struct mt_i2c *i2c)
{
	/* I2CFUC(); */
	/* int val=0; */
	pr_err("i2c_dump_info ++++++++++++++++++++++++++++++++++++++++++\n");
	pr_err("I2C structure:\n"
	       I2CTAG "Clk=%d,Id=%d,Op=%x,Irq_stat=%x,Total_len=%x\n"
	       I2CTAG "Trans_len=%x,Trans_num=%x,Trans_auxlen=%x,speed=%d\n"
	       I2CTAG "Trans_stop=%u\n",
	       15600, i2c->id, i2c->op, i2c->irq_stat, i2c->total_len,
	       i2c->msg_len, 1, i2c->msg_aux_len, i2c->speed_hz, i2c->trans_stop);

	pr_err("base address 0x%p\n", i2c->base);
	pr_err("I2C register:\n"
	       I2CTAG "SLAVE_ADDR=%x,INTR_MASK=%x,INTR_STAT=%x,CONTROL=%x,TRANSFER_LEN=%x\n"
	       I2CTAG "TRANSAC_LEN=%x,DELAY_LEN=%x,TIMING=%x,START=%x,FIFO_STAT=%x\n"
	       I2CTAG "IO_CONFIG=%x,HS=%x,DCM_EN=%x,DEBUGSTAT=%x,EXT_CONF=%x,TRANSFER_LEN_AUX=%x\n",
	       (i2c_readw(i2c, OFFSET_SLAVE_ADDR)),
	       (i2c_readw(i2c, OFFSET_INTR_MASK)),
	       (i2c_readw(i2c, OFFSET_INTR_STAT)),
	       (i2c_readw(i2c, OFFSET_CONTROL)),
	       (i2c_readw(i2c, OFFSET_TRANSFER_LEN)),
	       (i2c_readw(i2c, OFFSET_TRANSAC_LEN)),
	       (i2c_readw(i2c, OFFSET_DELAY_LEN)),
	       (i2c_readw(i2c, OFFSET_TIMING)),
	       (i2c_readw(i2c, OFFSET_START)),
	       (i2c_readw(i2c, OFFSET_FIFO_STAT)),
	       (i2c_readw(i2c, OFFSET_IO_CONFIG)),
	       (i2c_readw(i2c, OFFSET_HS)),
	       (i2c_readw(i2c, OFFSET_DCM_EN)),
	       (i2c_readw(i2c, OFFSET_DEBUGSTAT)),
	       (i2c_readw(i2c, OFFSET_EXT_CONF)), (i2c_readw(i2c, OFFSET_TRANSFER_LEN_AUX)));

	pr_err("before enable DMA register(0x%lx):\n"
	       I2CTAG "INT_FLAG=%x,INT_EN=%x,EN=%x,RST=%x,\n"
	       I2CTAG "STOP=%x,FLUSH=%x,CON=%x,TX_MEM_ADDR=%x, RX_MEM_ADDR=%x\n"
	       I2CTAG "TX_LEN=%x,RX_LEN=%x,INT_BUF_SIZE=%x,DEBUG_STATUS=%x\n"
	       I2CTAG "TX_MEM_ADDR2=%x, RX_MEM_ADDR2=%x\n",
	       g_dma_reg.base,
	       g_dma_reg.int_flag,
	       g_dma_reg.int_en,
	       g_dma_reg.en,
	       g_dma_reg.rst,
	       g_dma_reg.stop,
	       g_dma_reg.flush,
	       g_dma_reg.con,
	       g_dma_reg.tx_mem_addr,
	       g_dma_reg.tx_mem_addr,
	       g_dma_reg.tx_len,
	       g_dma_reg.rx_len,
	       g_dma_reg.int_buf_size, g_dma_reg.debug_sta,
	       g_dma_reg.tx_mem_addr2, g_dma_reg.tx_mem_addr2);
	pr_err("DMA register(0x%p):\n"
	       I2CTAG "INT_FLAG=%x,INT_EN=%x,EN=%x,RST=%x,\n"
	       I2CTAG "STOP=%x,FLUSH=%x,CON=%x,TX_MEM_ADDR=%x, RX_MEM_ADDR=%x\n"
	       I2CTAG "TX_LEN=%x,RX_LEN=%x,INT_BUF_SIZE=%x,DEBUG_STATUS=%x\n"
	       I2CTAG "TX_MEM_ADDR2=%x, RX_MEM_ADDR2=%x\n",
	       i2c->pdmabase,
	       (i2c_readl_dma(i2c, OFFSET_INT_FLAG)),
	       (i2c_readl_dma(i2c, OFFSET_INT_EN)),
	       (i2c_readl_dma(i2c, OFFSET_EN)),
	       (i2c_readl_dma(i2c, OFFSET_RST)),
	       (i2c_readl_dma(i2c, OFFSET_STOP)),
	       (i2c_readl_dma(i2c, OFFSET_FLUSH)),
	       (i2c_readl_dma(i2c, OFFSET_CON)),
	       (i2c_readl_dma(i2c, OFFSET_TX_MEM_ADDR)),
	       (i2c_readl_dma(i2c, OFFSET_RX_MEM_ADDR)),
	       (i2c_readl_dma(i2c, OFFSET_TX_LEN)),
	       (i2c_readl_dma(i2c, OFFSET_RX_LEN)),
	       (i2c_readl_dma(i2c, OFFSET_INT_BUF_SIZE)),
	       (i2c_readl_dma(i2c, OFFSET_DEBUG_STA)),
	       (i2c_readl_dma(i2c, OFFSET_TX_MEM_ADDR2)),
	       (i2c_readl_dma(i2c, OFFSET_RX_MEM_ADDR2)));
	pr_err("i2c_dump_info ------------------------------------------\n");

}

/*do i2c apdma warm reset & re-write dma buf addr, txlen*/
static int ccu_reset_i2c_apdma(struct mt_i2c *i2c)
{
	i2c_writel_dma(I2C_DMA_WARM_RST, i2c, OFFSET_RST);

#ifdef CONFIG_MTK_LM_MODE
	if ((i2c->dev_comp->dma_support == 1) && (enable_4G())) {
		i2c_writel_dma(0x1, i2c, OFFSET_TX_MEM_ADDR2);
		i2c_writel_dma(0x1, i2c, OFFSET_RX_MEM_ADDR2);
	}
#endif

	i2c_writel_dma(I2C_DMA_INT_FLAG_NONE, i2c, OFFSET_INT_FLAG);
	i2c_writel_dma(I2C_DMA_CON_TX, i2c, OFFSET_CON);
	i2c_writel_dma((u32) i2c->dma_buf.paddr, i2c, OFFSET_TX_MEM_ADDR);
	if ((i2c->dev_comp->dma_support >= 2))
		i2c_writel_dma(i2c->dma_buf.paddr >> 32, i2c, OFFSET_TX_MEM_ADDR2);

	/*write ap mda tx len = 128(must > totoal tx len within a frame)*/
	i2c_writel_dma(CCU_I2C_APDMA_TXLEN, i2c, OFFSET_TX_LEN);

	return 0;
}

int ccu_i2c_frame_reset(void)
{
	struct i2c_client *pClient = NULL;
	struct mt_i2c *i2c;

	pClient = getCcuI2cClient();
	i2c = i2c_get_adapdata(pClient->adapter);

	ccu_reset_i2c_apdma(i2c);

	/*--todo:remove dump log on production*/
	/*ccu_record_i2c_dma_info(i2c);*/

	i2c_writew(I2C_FIFO_ADDR_CLR, i2c, OFFSET_FIFO_ADDR_CLR);
	i2c_writew(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP, i2c, OFFSET_INTR_MASK);
	mb();
	/*--todo:remove dump log on production*/
	/*ccu_i2c_dump_info(i2c);*/

	return 0;
}


int ccu_trigger_i2c(int transac_len, MBOOL do_dma_en)
{
	struct i2c_client *pClient = NULL;
	struct mt_i2c *i2c;

	u8 *dmaBufVa;

	pClient = getCcuI2cClient();
	i2c = i2c_get_adapdata(pClient->adapter);

	dmaBufVa = i2c->dma_buf.vaddr;

	/*LOG_DBG("i2c_dma_buf_content: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",*/
	/*  dmaBufVa[0],dmaBufVa[1],dmaBufVa[2],dmaBufVa[3],dmaBufVa[4],dmaBufVa[5],*/
	/*dmaBufVa[18],dmaBufVa[19],dmaBufVa[20],dmaBufVa[21],dmaBufVa[22],dmaBufVa[23]);*/

	/*set i2c transaction length & enable apdma*/
	i2c_writew(transac_len, i2c, OFFSET_TRANSAC_LEN);

	/*--todo:remove dump log on production*/
	/*ccu_record_i2c_dma_info(i2c);*/

	i2c_writel_dma(I2C_DMA_START_EN, i2c, OFFSET_EN);

	/*--todo:remove dump log on production*/
	/*ccu_i2c_dump_info(i2c);*/

	/*--todo: add subcam implementation (using n3d_b);*/
	/*trigger i2c start from n3d_a*/
	n3d_a_writew(0x0000017C, g_ccu_device->n3d_a_base, OFFSET_CTL);

	switch (g_ccuI2cChannel) {
	case CCU_I2C_CHANNEL_MAINCAM:
		{
			n3d_a_writew(0x00000001, g_ccu_device->n3d_a_base, OFFSET_TRIG);
			break;
		}
	case CCU_I2C_CHANNEL_SUBCAM:
		{
			n3d_a_writew(0x00000002, g_ccu_device->n3d_a_base, OFFSET_TRIG);
			break;
		}
	default:
		{
			LOG_ERR("ccu_trigger_i2c fail, unknown channel: %d\n", g_ccuI2cChannel);
			return MNULL;
		}
	}

	/*--todo:remove dump log on production*/
	/*ccu_i2c_dump_info(i2c);*/

	return 0;
}


int ccu_config_i2c_buf_mode(int transfer_len)
{
	struct i2c_client *pClient = NULL;
	struct mt_i2c *i2c;

	pClient = getCcuI2cClient();
	i2c = i2c_get_adapdata(pClient->adapter);

	/*write i2c controller tx len*/
	i2c->total_len = transfer_len;
	i2c->msg_len = transfer_len;
	i2c_writew(transfer_len, i2c, OFFSET_TRANSFER_LEN);

	/*ccu_reset_i2c_apdma(i2c);*/

	ccu_record_i2c_dma_info(i2c);

	/*flush before sending DMA start*/
	/*mb();*/
	/*i2c_writel_dma(I2C_DMA_START_EN, i2c, OFFSET_EN);*/

	ccu_i2c_frame_reset();

	ccu_i2c_dump_info(i2c);

	return 0;
}


/*--todo: add subcam implementation*/
static struct i2c_msg ccu_i2c_msg[MAX_I2C_CMD_LEN];
int ccu_init_i2c_buf_mode(u16 i2cId)
{
	int ret = 0;
	unsigned char dummy_data[] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18
	};

	struct i2c_client *pClient = NULL;
	int trans_num = 1;

	memset(ccu_i2c_msg, 0, MAX_I2C_CMD_LEN * sizeof(struct i2c_msg));

	pClient = getCcuI2cClient();

	/*for(i = 0 ; i<trans_num ; i++){*/
	ccu_i2c_msg[0].addr = i2cId >> 1;
	ccu_i2c_msg[0].flags = 0;
	ccu_i2c_msg[0].len = 16;
	ccu_i2c_msg[0].buf = dummy_data;
	/*}*/

	ret = hw_trig_i2c_transfer(pClient->adapter, ccu_i2c_msg, trans_num);
	return ret;
}

int ccu_i2c_buf_mode_en(int enable)
{
	int ret = 0;
	struct i2c_client *pClient = NULL;

	LOG_DBG("i2c_buf_mode_en %d\n", enable);

	pClient = getCcuI2cClient();

	LOG_DBG("i2c_buf_mode_en, pClient: %p\n", pClient);
	if (pClient == NULL)
		return -1;

	if (enable)
		ret = hw_trig_i2c_enable(pClient->adapter);
	else
		ret = hw_trig_i2c_disable(pClient->adapter);
	return ret;
}

int i2c_get_dma_buffer_addr(void **va)
{
	struct i2c_client *pClient = NULL;
	struct mt_i2c *i2c;

	pClient = getCcuI2cClient();

	if (pClient == MNULL) {
		LOG_ERR("ccu client is NULL");
		return -EFAULT;
	}

	i2c = i2c_get_adapdata(pClient->adapter);

	/*i2c_get_dma_buffer_addr_imp(pClient->adapter ,va);*/
	*va = i2c->dma_buf.vaddr;
	LOG_DBG("got i2c buf va: %p\n", *va);

	return 0;
}
#endif

/*---------------------------------------------------------------------------*/
/* CCU Driver: file operations                                               */
/*---------------------------------------------------------------------------*/
static int ccu_open(struct inode *inode, struct file *flip);

static int ccu_release(struct inode *inode, struct file *flip);

static int ccu_mmap(struct file *flip, struct vm_area_struct *vma);

static long ccu_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

static long ccu_compat_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

static const struct file_operations ccu_fops = {
	.owner = THIS_MODULE,
	.open = ccu_open,
	.release = ccu_release,
	.mmap = ccu_mmap,
	.unlocked_ioctl = ccu_ioctl,
	/*for 32bit usersapce program doing ioctl, compat_ioctl will be called*/
	.compat_ioctl = ccu_compat_ioctl
};

/*---------------------------------------------------------------------------*/
/* M4U: fault callback                                                       */
/*---------------------------------------------------------------------------*/
m4u_callback_ret_t ccu_m4u_fault_callback(int port, unsigned int mva, void *data)
{
	LOG_DBG("[m4u] fault callback: port=%d, mva=0x%x", port, mva);
	return M4U_CALLBACK_HANDLED;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static int ccu_num_users;

int ccu_create_user(ccu_user_t **user)
{
	ccu_user_t *u;

	u = kmalloc(sizeof(vlist_type(ccu_user_t)), GFP_ATOMIC);
	if (!u)
		return -1;

	mutex_init(&u->data_mutex);
	/*mutex_lock(&u->data_mutex);*/
	u->id = ++ccu_num_users;
	u->open_pid = current->pid;
	u->open_tgid = current->tgid;
	u->running = false;
	u->flush = false;
	INIT_LIST_HEAD(&u->enque_ccu_cmd_list);
	INIT_LIST_HEAD(&u->deque_ccu_cmd_list);
	init_waitqueue_head(&u->deque_wait);
	/*mutex_unlock(&u->data_mutex);*/

	mutex_lock(&g_ccu_device->user_mutex);
	list_add_tail(vlist_link(u, ccu_user_t), &g_ccu_device->user_list);
	mutex_unlock(&g_ccu_device->user_mutex);

	*user = u;
	return 0;
}


int ccu_push_command_to_queue(ccu_user_t *user, ccu_cmd_st *cmd)
{
	if (!user) {
		LOG_ERR("empty user");
		return -1;
	}

	LOG_DBG("+:%s\n", __func__);

	spin_lock(&g_ccu_device->cmd_wait.lock);

	/*mutex_lock(&user->data_mutex);*/
	list_add_tail(vlist_link(cmd, ccu_cmd_st), &user->enque_ccu_cmd_list);
	/*mutex_unlock(&user->data_mutex);*/

	wake_up_locked(&g_ccu_device->cmd_wait);
	spin_unlock(&g_ccu_device->cmd_wait.lock);

	return 0;
}

int ccu_flush_commands_from_queue(ccu_user_t *user)
{

	struct list_head *head, *temp;
	ccu_cmd_st *cmd;

	/*mutex_lock(&user->data_mutex);*/

	if (!user->running && list_empty(&user->enque_ccu_cmd_list)
	    && list_empty(&user->deque_ccu_cmd_list)) {
		/*mutex_unlock(&user->data_mutex);*/
		return 0;
	}

	user->flush = true;
	/*mutex_unlock(&user->data_mutex);*/

	/* the running command will add to the deque before interrupt */
	wait_event_interruptible(user->deque_wait, !user->running);

	/*mutex_lock(&user->data_mutex);*/
	/* push the remaining enque to the deque */
	list_for_each_safe(head, temp, &user->enque_ccu_cmd_list) {
		cmd = vlist_node_of(head, ccu_cmd_st);
		cmd->status = CCU_ENG_STATUS_FLUSH;
		list_del_init(vlist_link(cmd, ccu_cmd_st));
		list_add_tail(vlist_link(cmd, ccu_cmd_st), &user->deque_ccu_cmd_list);
	}

	user->flush = false;
	/*mutex_unlock(&user->data_mutex);*/
	return 0;
}

int ccu_pop_command_from_queue(ccu_user_t *user, ccu_cmd_st **rcmd)
{
	int ret;
	ccu_cmd_st *cmd;

	/* wait until condition is true */
	ret = wait_event_interruptible_timeout(user->deque_wait,
					       !list_empty(&user->deque_ccu_cmd_list),
					       msecs_to_jiffies(3 * 1000));

	/* ret == 0, if timeout; ret == -ERESTARTSYS, if signal interrupt */
	if (ret < 1) {
		LOG_ERR("timeout: pop a command! ret=%d\n", ret);
		*rcmd = NULL;
		return -1;
	}
	/*mutex_lock(&user->data_mutex);*/
	/* This part should not be happened */
	if (list_empty(&user->deque_ccu_cmd_list)) {
		/*mutex_unlock(&user->data_mutex);*/
		LOG_ERR("pop a command from empty queue! ret=%d\n", ret);
		*rcmd = NULL;
		return -1;
	};

	/* get first node from deque list */
	cmd = vlist_node_of(user->deque_ccu_cmd_list.next, ccu_cmd_st);

	list_del_init(vlist_link(cmd, ccu_cmd_st));
	/*mutex_unlock(&user->data_mutex);*/

	*rcmd = cmd;
	return 0;
}


int ccu_delete_user(ccu_user_t *user)
{

	if (!user) {
		LOG_ERR("delete empty user!\n");
		return -1;
	}
	/* TODO: notify dropeed command to user?*/
	/* ccu_dropped_command_notify(user, command);*/
	ccu_flush_commands_from_queue(user);

	mutex_lock(&g_ccu_device->user_mutex);
	list_del(vlist_link(user, ccu_user_t));
	mutex_unlock(&g_ccu_device->user_mutex);

	kfree(user);

	return 0;
}

int ccu_lock_user_mutex(void)
{
	mutex_lock(&g_ccu_device->user_mutex);
	return 0;
}

int ccu_unlock_user_mutex(void)
{
	mutex_unlock(&g_ccu_device->user_mutex);
	return 0;
}

/*---------------------------------------------------------------------------*/
/* IOCTL: implementation                                                     */
/*---------------------------------------------------------------------------*/
int ccu_set_power(ccu_power_t *power)
{
	return ccu_power(power);
}

static int ccu_open(struct inode *inode, struct file *flip)
{
	int ret = 0;

	ccu_user_t *user;

	ccu_create_user(&user);
	if (IS_ERR_OR_NULL(user)) {
		LOG_ERR("fail to create user\n");
		return -ENOMEM;
	}

	flip->private_data = user;

	return ret;
}

static long ccu_compat_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	/*<<<<<<<<<< debug 32/64 compat check*/
	compat_ccu_power_t __user *ptr_power32;
	ccu_power_t __user *ptr_power64;

	compat_uptr_t uptr_Addr32;
	compat_uint_t uint_Data32;

	int err;
	int i;
	/*>>>>>>>>>> debug 32/64 compat check*/

	int ret = 0;
	ccu_user_t *user = flip->private_data;

#define CCU_ASSERT(condition, fmt, args...) {\
			if (!(condition)) { \
				LOG_ERR(fmt, ##args);\
				return -EFAULT; \
			} \
		}

	LOG_DBG("+, cmd: %d\n", cmd);

	switch (cmd) {
	case CCU_IOCTL_SET_POWER:
		{
			LOG_DBG("CCU_IOCTL_SET_POWER+\n");

			/*<<<<<<<<<< debug 32/64 compat check*/
			LOG_DBG("[IOCTL_DBG] ccu_power_t size: %zu\n", sizeof(ccu_power_t));
			LOG_DBG("[IOCTL_DBG] ccu_working_buffer_t size: %zu\n",
				sizeof(ccu_working_buffer_t));
			LOG_DBG("[IOCTL_DBG] arg: %p\n", (void *)arg);
			LOG_DBG("[IOCTL_DBG] long size: %zu\n", sizeof(long));
			LOG_DBG("[IOCTL_DBG] long long: %zu\n", sizeof(long long));
			LOG_DBG("[IOCTL_DBG] char *size: %zu\n", sizeof(char *));
			LOG_DBG("[IOCTL_DBG] power.workBuf.va_log[0]: %p\n",
				power.workBuf.va_log[0]);

			ptr_power32 = compat_ptr(arg);
			ptr_power64 = compat_alloc_user_space(sizeof(*ptr_power64));
			if (ptr_power64 == NULL)
				return -EFAULT;

			LOG_DBG("[IOCTL_DBG] (void *)arg: %p\n", (void *)arg);
			LOG_DBG("[IOCTL_DBG] ptr_power32: %p\n", ptr_power32);
			LOG_DBG("[IOCTL_DBG] ptr_power64: %p\n", ptr_power64);
			LOG_DBG("[IOCTL_DBG] *ptr_power32 size: %zu\n", sizeof(*ptr_power32));
			LOG_DBG("[IOCTL_DBG] *ptr_power64 size: %zu\n", sizeof(*ptr_power64));

			err = 0;
			err |= get_user(uint_Data32, &(ptr_power32->bON));
			err |= put_user(uint_Data32, &(ptr_power64->bON));

			for (i = 0; i < MAX_LOG_BUF_NUM; i++) {
				err |= get_user(uptr_Addr32, (&ptr_power32->workBuf.va_log[i]));
				err |=
				    put_user(compat_ptr(uptr_Addr32),
					     (&ptr_power64->workBuf.va_log[i]));
				err |= get_user(uint_Data32, (&ptr_power32->workBuf.mva_log[i]));

				err |=
				    copy_to_user(&(ptr_power64->workBuf.mva_log[i]), &uint_Data32,
						 sizeof(uint_Data32));
			}

			LOG_DBG("[IOCTL_DBG] err: %d\n", err);
			LOG_DBG("[IOCTL_DBG] ptr_power32->workBuf.va_pool: %x\n",
				ptr_power32->workBuf.va_pool);
			LOG_DBG("[IOCTL_DBG] ptr_power64->workBuf.va_pool: %p\n",
				ptr_power64->workBuf.va_pool);
			LOG_DBG("[IOCTL_DBG] ptr_power32->workBuf.va_log: %x\n",
				ptr_power32->workBuf.va_log[0]);
			LOG_DBG("[IOCTL_DBG] ptr_power64->workBuf.va_log: %p\n",
				ptr_power64->workBuf.va_log[0]);
			LOG_DBG("[IOCTL_DBG] ptr_power32->workBuf.mva_log: %x\n",
				ptr_power32->workBuf.mva_log[0]);
			LOG_DBG("[IOCTL_DBG] ptr_power64->workBuf.mva_log: %x\n",
				ptr_power64->workBuf.mva_log[0]);

			ret = flip->f_op->unlocked_ioctl(flip, cmd, (unsigned long)ptr_power64);
			/*>>>>>>>>>> debug 32/64 compat check*/

			LOG_DBG("CCU_IOCTL_SET_POWER-");
			break;
		}
	default:
		ret = flip->f_op->unlocked_ioctl(flip, cmd, arg);
		break;
	}

	if (ret != 0) {
		LOG_ERR("fail, cmd(%d), pid(%d), (process, pid, tgid)=(%s, %d, %d)\n",
			cmd, user->open_pid, current->comm, current->pid, current->tgid);
	}
#undef CCU_ASSERT
	return ret;
}

static int ccu_alloc_command(ccu_cmd_st **rcmd)
{
	ccu_cmd_st *cmd;

	cmd = kzalloc(sizeof(vlist_type(ccu_cmd_st)), GFP_KERNEL);
	if (cmd == NULL) {
		LOG_ERR("ccu_alloc_command(), node=0x%p\n", cmd);
		return -ENOMEM;
	}

	*rcmd = cmd;

	return 0;
}


static int ccu_free_command(ccu_cmd_st *cmd)
{
	kfree(cmd);
	return 0;
}


static long ccu_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	CCU_WAIT_IRQ_STRUCT IrqInfo;
	ccu_user_t *user = flip->private_data;

#define CCU_ASSERT(condition, fmt, args...) {\
			if (!(condition)) { \
				LOG_ERR(fmt, ##args);\
				return -EFAULT; \
			} \
		}

	LOG_DBG("ccu_ioctl+, cmd:%d\n", cmd);

	switch (cmd) {
	case CCU_IOCTL_SET_POWER:
		{
			LOG_DBG("ccuk: ioctl set powerk+\n");
			ret = copy_from_user(&power, (void *)arg, sizeof(ccu_power_t));
			CCU_ASSERT(ret == 0, "[SET_POWER] copy_from_user failed, ret=%d\n", ret);
			ret = ccu_set_power(&power);
			LOG_DBG("ccuk: ioctl set powerk-\n");
			break;
		}
	case CCU_IOCTL_SET_RUN:
		{
			ret = ccu_run();
			break;
		}
	case CCU_IOCTL_ENQUE_COMMAND:
		{
			ccu_cmd_st *cmd = 0;

			/*allocate ccu_cmd_st_list instead of ccu_cmd_st*/
			ccu_alloc_command(&cmd);
			ret = copy_from_user(cmd, (void *)arg, sizeof(ccu_cmd_st));
			CCU_ASSERT(ret == 0, "[ENQUE_COMMAND] copy_from_user failed, ret=%d\n",
				   ret);
			ret = ccu_push_command_to_queue(user, cmd);
			break;
		}
	case CCU_IOCTL_DEQUE_COMMAND:
		{
			ccu_cmd_st *cmd = 0;

			ret = ccu_pop_command_from_queue(user, &cmd);
			CCU_ASSERT(ret == 0, "[DEQUE_COMMAND] pop command failed, ret=%d\n", ret);

			ret = copy_to_user((void *)arg, cmd, sizeof(ccu_cmd_st));
			CCU_ASSERT(ret == 0, "[DEQUE_COMMAND] copy_to_user failed, ret=%d\n", ret);

			ret = ccu_free_command(cmd);
			CCU_ASSERT(ret == 0, "[DEQUE_COMMAND] free command, ret=%d\n", ret);

			break;
		}
	case CCU_IOCTL_FLUSH_COMMAND:
		{
			ret = ccu_flush_commands_from_queue(user);
			CCU_ASSERT(ret == 0, "[FLUSH_COMMAND] flush command failed, ret=%d\n", ret);

			break;
		}
	case CCU_IOCTL_WAIT_IRQ:
		{
			if (copy_from_user(&IrqInfo, (void *)arg, sizeof(CCU_WAIT_IRQ_STRUCT)) == 0) {
				if ((IrqInfo.Type >= CCU_IRQ_TYPE_AMOUNT) || (IrqInfo.Type < 0)) {
					ret = -EFAULT;
					LOG_ERR("invalid type(%d)\n", IrqInfo.Type);
					goto EXIT;
				}

				LOG_DBG("IRQ type(%d), userKey(%d), timeout(%d), sttype(%d), st(%d)\n",
					IrqInfo.Type,
					IrqInfo.EventInfo.UserKey,
					IrqInfo.EventInfo.Timeout,
					IrqInfo.EventInfo.St_type,
					IrqInfo.EventInfo.Status);

				ret = ccu_waitirq(&IrqInfo);

				if (copy_to_user((void *)arg, &IrqInfo, sizeof(CCU_WAIT_IRQ_STRUCT))
				    != 0) {
					LOG_ERR("copy_to_user failed\n");
					ret = -EFAULT;
				}
			} else {
				LOG_ERR("copy_from_user failed\n");
				ret = -EFAULT;
			}

			break;
		}
	case CCU_IOCTL_SEND_CMD:	/*--todo: not used for now, remove it*/
		{
			ccu_cmd_st cmd;

			ret = copy_from_user(&cmd, (void *)arg, sizeof(ccu_cmd_st));

			CCU_ASSERT(ret == 0, "[CCU_IOCTL_SEND_CMD] copy_from_user failed, ret=%d\n",
				   ret);
			ccu_send_command(&cmd);
			break;
		}
	case CCU_IOCTL_FLUSH_LOG:
		{
			ccu_flushLog(0, NULL);
			break;
		}
	case CCU_IOCTL_GET_I2C_DMA_BUF_ADDR:
		{
			uint32_t mva;

			ret = ccu_get_i2c_dma_buf_addr(&mva);

			if (ret != 0) {
				LOG_DBG("ccu_get_i2c_dma_buf_addr fail: %d\n", ret);
				break;
			}

			ret = copy_to_user((void *)arg, &mva, sizeof(uint32_t));

			break;
		}
	case CCU_IOCTL_SET_I2C_MODE:
		{
			struct ccu_i2c_arg i2c_arg;

			ret = copy_from_user(&i2c_arg, (void *)arg, sizeof(struct ccu_i2c_arg));

			ret = ccu_i2c_ctrl(i2c_arg.i2c_write_id, i2c_arg.transfer_len);

			break;
		}
	case CCU_IOCTL_SET_I2C_CHANNEL:
		{
#if 1
			ret =
			    copy_from_user(&g_ccuI2cChannel, (void *)arg,
					   sizeof(enum CCU_I2C_CHANNEL));

			if ((g_ccuI2cChannel == CCU_I2C_CHANNEL_MAINCAM)
			    || (g_ccuI2cChannel == CCU_I2C_CHANNEL_SUBCAM)) {
				ret = 0;
			} else {
				LOG_ERR("invalid i2c channel: %d\n", g_ccuI2cChannel);
				ret = -EFAULT;
			}
#else
			enum CCU_I2C_CHANNEL channel;

			ret = copy_from_user(&channel, (void *)arg, sizeof(enum CCU_I2C_CHANNEL));

			if ((channel == CCU_I2C_CHANNEL_MAINCAM)
			    || (channel == CCU_I2C_CHANNEL_SUBCAM)) {
				ccu_i2c_set_channel(channel);
				ret = 0;
			} else {
				LOG_ERR("invalid i2c channel: %d\n", channel);
				ret = -EFAULT;
			}
#endif
			break;
		}
	case CCU_IOCTL_GET_CURRENT_FPS:
		{
			int32_t current_fps = ccu_get_current_fps();

			ret = copy_to_user((void *)arg, &current_fps, sizeof(int32_t));

			break;
		}
	case CCU_READ_REGISTER:
		{
			int regToRead = (int)arg;

			return ccu_read_info_reg(regToRead);
		}
	default:
		LOG_WRN("ioctl:No such command!\n");
		ret = -EINVAL;
		break;
	}

EXIT:
	if (ret != 0) {
		LOG_ERR("fail, cmd(%d), pid(%d), (process, pid, tgid)=(%s, %d, %d)\n",
			cmd, user->open_pid, current->comm, current->pid, current->tgid);
	}
#undef CCU_ASSERT
	return ret;
}

static int ccu_release(struct inode *inode, struct file *flip)
{
	ccu_user_t *user = flip->private_data;

	ccu_delete_user(user);

	return 0;
}


/*******************************************************************************
*
********************************************************************************/
static int ccu_mmap(struct file *flip, struct vm_area_struct *vma)
{
	unsigned long length = 0;
	unsigned int pfn = 0x0;

	length = (vma->vm_end - vma->vm_start);
	/*  */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	pfn = vma->vm_pgoff << PAGE_SHIFT;

	LOG_DBG
	    ("CCU_mmap: vm_pgoff(0x%lx),pfn(0x%x),phy(0x%lx),vm_start(0x%lx),vm_end(0x%lx),length(0x%lx)\n",
	     vma->vm_pgoff, pfn, vma->vm_pgoff << PAGE_SHIFT, vma->vm_start, vma->vm_end, length);

	if (pfn >= CCU_REG_BASE_HW) {

		switch (pfn) {
		case CCU_A_BASE_HW:
			if (length > CCU_REG_RANGE) {
				LOG_ERR
				    ("mmap range error :module(0x%x),length(0x%lx),CCU_A_BASE_HW(0x%x)!\n",
				     pfn, length, 0x4000);
				return -EAGAIN;
			}
			break;
		case CCU_CAMSYS_BASE_HW:
			if (length > CCU_REG_RANGE) {
				LOG_ERR
				    ("mmap range error :module(0x%x),length(0x%lx),CCU_CAMSYS_BASE_HW(0x%x)!\n",
				     pfn, length, 0x4000);
				return -EAGAIN;
			}
			break;
		case CCU_PMEM_BASE_HW:
			if (length > CCU_PMEM_RANGE) {
				LOG_ERR
				    ("mmap range error :module(0x%x),length(0x%lx),CCU_PMEM_BASE_HW(0x%x)!\n",
				     pfn, length, 0x4000);
				return -EAGAIN;
			}
			break;
		case CCU_DMEM_BASE_HW:
			if (length > CCU_DMEM_RANGE) {
				LOG_ERR
				    ("mmap range error :module(0x%x),length(0x%lx),CCU_PMEM_BASE_HW(0x%x)!\n",
				     pfn, length, 0x4000);
				return -EAGAIN;
			}
			break;
		default:
			LOG_ERR("Illegal starting HW addr for mmap!\n");
			return -EAGAIN;
		}
		if (remap_pfn_range
		    (vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start,
		     vma->vm_page_prot)) {
			LOG_ERR("remap_pfn_range\n");
			return -EAGAIN;
		}

	} else {

		return ccu_mmap_hw(flip, vma);

	}

	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static dev_t ccu_devt;
static struct cdev *ccu_chardev;
static struct class *ccu_class;
static int ccu_num_devs;

static inline void ccu_unreg_chardev(void)
{
	/* Release char driver */
	if (ccu_chardev != NULL) {
		cdev_del(ccu_chardev);
		ccu_chardev = NULL;
	}
	unregister_chrdev_region(ccu_devt, 1);
}

static inline int ccu_reg_chardev(void)
{
	int ret = 0;

	ret = alloc_chrdev_region(&ccu_devt, 0, 1, CCU_DEV_NAME);
	if ((ret) < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d\n", ret);
		return ret;
	}
	/* Allocate driver */
	ccu_chardev = cdev_alloc();
	if (ccu_chardev == NULL) {
		LOG_ERR("cdev_alloc failed\n");
		ret = -ENOMEM;
		goto EXIT;
	}

	/* Attatch file operation. */
	cdev_init(ccu_chardev, &ccu_fops);

	ccu_chardev->owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(ccu_chardev, ccu_devt, 1);
	if ((ret) < 0) {
		LOG_ERR("Attatch file operation failed, %d\n", ret);
		goto EXIT;
	}

EXIT:
	if (ret < 0)
		ccu_unreg_chardev();

	return ret;
}

/*******************************************************************************
* platform_driver
********************************************************************************/

static int ccu_probe(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	struct device *dev;
	struct device_node *node;
	int ret = 0;
	int i2c_ret = 0;

	node = pdev->dev.of_node;
	g_ccu_device->dev = &pdev->dev;
	LOG_DBG("probe 0, pdev id = %d name = %s\n", pdev->id, pdev->name);

#ifdef MTK_CCU_EMULATOR
	/* emulator will fill ccu_base and bin_base */
	/*ccu_init_emulator(g_ccu_device);*/
#else
	/* get register address */
	if ((strcmp("ccu", g_ccu_device->dev->of_node->name) == 0)) {
		g_ccu_device->ccu_base = (unsigned long)of_iomap(node, 1);
		LOG_DBG("ccu_base=0x%lx\n", g_ccu_device->ccu_base);

		/* get physical address of pmem  */
		{
			uint32_t phy_addr;
			uint32_t phy_size;

			if (of_property_read_u32(node, "ccu_dmem_base", &phy_addr) ||
				of_property_read_u32(node, "ccu_dmem_size", &phy_size)) {
				LOG_ERR("fail to get physical address of ccu dmem!\n");
				return -ENODEV;
			}

			LOG_INF("probe 1, phy_addr: 0x%x, phy_size: 0x%x\n", phy_addr, phy_size);

			/* ioremap_wc() has no access 4 bytes alignment limitation as of_iomap() does?
			* https://forums.xilinx.com/xlnx/attachments/
			* xlnx/ELINUX/11158/1/Linux%20CPU%20to%20PL%20Access.pdf
			*/
			g_ccu_device->dmem_base = (unsigned long)ioremap_wc(phy_addr, phy_size);

			g_ccu_device->camsys_base = (unsigned long)of_iomap(node, 0);
			LOG_DBG("ccu_base_camsys=0x%lx\n", g_ccu_device->camsys_base);
		}
		/**/
		g_ccu_device->irq_num = irq_of_parse_and_map(node, 0);
		LOG_DBG("probe 1, ccu_base: 0x%lx, bin_base: 0x%lx, irq_num: %d, pdev: %p\n",
			g_ccu_device->ccu_base, g_ccu_device->bin_base,
			g_ccu_device->irq_num, g_ccu_device->dev);

		if (g_ccu_device->irq_num > 0) {
			/* get IRQ flag from device node */
			unsigned int irq_info[3];

			if (of_property_read_u32_array
			    (node, "interrupts", irq_info, ARRAY_SIZE(irq_info))) {
				dev_err(g_ccu_device->dev, "get irq flags from DTS fail!\n");
				return -ENODEV;
			}
		} else {
			LOG_DBG("No IRQ!!: ccu_num_devs=%d, devnode(%s), irq=%d\n", ccu_num_devs,
				g_ccu_device->dev->of_node->name, g_ccu_device->irq_num);
		}

		/* Only register char driver in the 1st time */
		if (++ccu_num_devs == 1) {

			/* Register char driver */
			ret = ccu_reg_chardev();
			if (ret) {
				dev_err(g_ccu_device->dev, "register char failed");
				return ret;
			}

			/* Create class register */
			ccu_class = class_create(THIS_MODULE, "ccudrv");
			if (IS_ERR(ccu_class)) {
				ret = PTR_ERR(ccu_class);
				LOG_ERR("Unable to create class, err = %d\n", ret);
				goto EXIT;
			}

			dev = device_create(ccu_class, NULL, ccu_devt, NULL, CCU_DEV_NAME);
			if (IS_ERR(dev)) {
				ret = PTR_ERR(dev);
				dev_err(g_ccu_device->dev,
					"Failed to create device: /dev/%s, err = %d", CCU_DEV_NAME,
					ret);
				goto EXIT;
			}
#ifdef CONFIG_PM_WAKELOCKS
			wakeup_source_init(&ccu_wake_lock, "ccu_lock_wakelock");
#else
			wake_lock_init(&ccu_wake_lock, WAKE_LOCK_SUSPEND, "ccu_lock_wakelock");
#endif

			/* enqueue/dequeue control in ihalpipe wrapper */
			init_waitqueue_head(&wait_queue_deque);
			init_waitqueue_head(&wait_queue_enque);

			/*for (i = 0; i < CCU_IRQ_NUM_TYPES; i++) {*/
			/*      tasklet_init(ccu_tasklet[i].pCCU_tkt, ccu_tasklet[i].tkt_cb, 0);*/
			/*}*/

			/*register i2c driver callback*/
			/*ccu_i2c_add_drivers();*/
			LOG_DBG("i2c_add_driver(&ccu_i2c_main_driver)++\n");
			i2c_ret = i2c_add_driver(&ccu_i2c_main_driver);
			LOG_DBG("i2c_add_driver(&ccu_i2c_main_driver), ret: %d--\n", i2c_ret);
			LOG_DBG("i2c_add_driver(&ccu_i2c_sub_driver)++\n");
			i2c_ret = i2c_add_driver(&ccu_i2c_sub_driver);
			LOG_DBG("i2c_add_driver(&ccu_i2c_sub_driver), ret: %d--\n", i2c_ret);

EXIT:
			if (ret < 0)
				ccu_unreg_chardev();
		}

		ccu_init_hw(g_ccu_device);

		LOG_ERR("ccu probe cuccess...\n");

	} else if ((strcmp("n3d_ctl_a", g_ccu_device->dev->of_node->name) == 0)) {
		g_ccu_device->n3d_a_base = (unsigned long)of_iomap(node, 0);
		LOG_DBG("n3d_a_base=0x%lx\n", g_ccu_device->n3d_a_base);
		LOG_ERR("ccu n3da probe success...\n");
	}
#endif
#endif

	LOG_DBG("- X. CCU driver probe.\n");

	return ret;
}


static int ccu_remove(struct platform_device *pDev)
{
	/*    struct resource *pRes; */
	int irq_num;

	/*  */
	LOG_DBG("- E.");

	/*uninit hw*/
	ccu_uninit_hw(g_ccu_device);

	/* unregister char driver. */
	ccu_unreg_chardev();

	/*ccu_i2c_del_drivers();*/
	i2c_del_driver(&ccu_i2c_main_driver);
	i2c_del_driver(&ccu_i2c_sub_driver);

	/* Release IRQ */
	disable_irq(g_ccu_device->irq_num);
	irq_num = platform_get_irq(pDev, 0);
	free_irq(irq_num, (void *)ccu_chardev);

	/* kill tasklet */
	/*for (i = 0; i < CCU_IRQ_NUM_TYPES; i++) {*/
	/*      tasklet_kill(ccu_tasklet[i].p_ccu_tkt);*/
	/*}*/

	/*  */
	device_destroy(ccu_class, ccu_devt);
	/*  */
	class_destroy(ccu_class);
	ccu_class = NULL;
	/*  */
	return 0;
}

static int ccu_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int ccu_resume(struct platform_device *pdev)
{
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static int __init CCU_INIT(void)
{
	int ret = 0;
	/*struct device_node *node = NULL;*/

	g_ccu_device = kzalloc(sizeof(ccu_device_t), GFP_KERNEL);
	/*g_ccu_device = dma_cache_coherent();*/

	INIT_LIST_HEAD(&g_ccu_device->user_list);
	mutex_init(&g_ccu_device->user_mutex);
	init_waitqueue_head(&g_ccu_device->cmd_wait);

	/* Register M4U callback */
	LOG_DBG("register m4u callback");
	m4u_register_fault_callback(CCUI_OF_M4U_PORT, ccu_m4u_fault_callback, NULL);
	m4u_register_fault_callback(CCUO_OF_M4U_PORT, ccu_m4u_fault_callback, NULL);
	m4u_register_fault_callback(CCUG_OF_M4U_PORT, ccu_m4u_fault_callback, NULL);

	LOG_DBG("platform_driver_register start\n");
	if (platform_driver_register(&ccu_driver)) {
		LOG_ERR("failed to register CCU driver");
		return -ENODEV;
	}

	LOG_DBG("platform_driver_register finsish\n");

	return ret;
}


static void __exit CCU_EXIT(void)
{
	platform_driver_unregister(&ccu_driver);

	kfree(g_ccu_device);

	/* Un-Register M4U callback */
	LOG_DBG("un-register m4u callback");
	m4u_unregister_fault_callback(CCUI_OF_M4U_PORT);
	m4u_unregister_fault_callback(CCUO_OF_M4U_PORT);
	m4u_unregister_fault_callback(CCUG_OF_M4U_PORT);
}


/*******************************************************************************
*
********************************************************************************/
module_init(CCU_INIT);
module_exit(CCU_EXIT);
MODULE_DESCRIPTION("MTK CCU Driver");
MODULE_AUTHOR("SW1");
MODULE_LICENSE("GPL");
