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
#include <linux/spi/spi.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/cma.h>
#include <linux/debugfs.h>
#include <linux/stat.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/memblock.h>
#include <asm/page.h>
#include <asm-generic/memory_model.h>
#include <mt-plat/mt_lpae.h> /* DMA */
#include <mt-plat/aee.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include "mt_spi_hal.h"
#include "mt_spi.h"
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
#define SPI_TRUSTONIC_TEE_SUPPORT
#endif

#ifdef SPI_TRUSTONIC_TEE_SUPPORT
#include <mobicore_driver_api.h>
#include <tlspi_Api.h>
#endif

/*#define SPIDEV_LOG(fmt, args...) printk("[SPI-UT]: [%s]:[%d]" fmt, __func__, __LINE__, ##args)*/
/*#define SPIDEV_MSG(fmt, args...) printk(KERN_ERR  fmt, ##args )*/
#define SPIDEV_LOG(fmt, args...) pr_debug("[SPI-UT]: [%s]:[%d]" fmt, __func__, __LINE__, ##args)
#define SPIDEV_MSG(fmt, args...) pr_debug(fmt, ##args)

#define SPI_STRESS_MAX 1
DECLARE_COMPLETION(mt_spi_done);
static u32 stress_err;
static struct task_struct *spi_concur1;
static struct task_struct *spi_concur2;
static struct task_struct *spi_concur3;
static struct task_struct *spi_concur4;

static struct spi_transfer stress_xfer[SPI_STRESS_MAX];
static struct spi_transfer stress_xfer_con[SPI_STRESS_MAX];

static struct spi_message stress_msg[SPI_STRESS_MAX];
/*static struct spi_device *spi_test;*/

#define USE_SPI1_4GB_TEST (0)

#if USE_SPI1_4GB_TEST
static dma_addr_t SpiDmaBufTx_pa;
static dma_addr_t SpiDmaBufRx_pa;
static char *spi_tx_local_buf;
static char *spi_rx_local_buf;

static int reserve_memory_spi_fn(struct reserved_mem *rmem)
{
	SPIDEV_LOG(" name: %s, base: 0x%llx, size: 0x%llx\n", rmem->name,
			(unsigned long long)rmem->base, (unsigned long long)rmem->size);
	BUG_ON(rmem->size < 0x8000);
	SpiDmaBufTx_pa = rmem->base;
	SpiDmaBufRx_pa = rmem->base+0x4000;
	return 0;
}
RESERVEDMEM_OF_DECLARE(reserve_memory_test, "mediatek,spi-reserve-memory", reserve_memory_spi_fn);

#endif

bool spi1_4G_flag = 0;

static int spi_setup_xfer(struct device *dev, struct spi_transfer *xfer, u32 len, u32 flag)
{
	u8 tx_buffer = 0x01;
	u32 i, spi_id;
	struct spi_device *spi;

	#define SPI_CROSS_ALIGN_OFFSET 1008
	xfer->len = len;

	spi = container_of(dev, struct spi_device, dev);
	spi_id = spi->master->bus_num;

	spi1_4G_flag = (enable_4G() && (1 == spi_id));

	if (spi1_4G_flag) {
	#if USE_SPI1_4GB_TEST
		SPIDEV_LOG("spi1 4G Memory UT\n");
		/* map physical addr to virtual addr */
		if (NULL == spi_tx_local_buf) {
			spi_tx_local_buf = (char *)ioremap_nocache(SpiDmaBufTx_pa, 0x4000);
			if (!spi_tx_local_buf) {
				SPIDEV_LOG("SPI Failed to dma_alloc_coherent()\n");
				return -ENOMEM;
			}
		}
		if (NULL == spi_rx_local_buf) {
			spi_rx_local_buf = (char *)ioremap_nocache(SpiDmaBufRx_pa, 0x4000);
			if (!spi_rx_local_buf) {
				SPIDEV_LOG("SPI Failed to dma_alloc_coherent()\n");
				return -ENOMEM;
			}
		}

		xfer->tx_buf = spi_tx_local_buf;
		xfer->rx_buf = spi_rx_local_buf;
		xfer->tx_dma = SpiDmaBufTx_pa;
		xfer->rx_dma = SpiDmaBufRx_pa;

	#else
		SPIDEV_LOG("Wanning: If using SPI1, you should define <USE_SPI1_4GB_TEST = 1>\n");
	#endif
	} else {
		SPIDEV_LOG("SPI UT debug\n");
		xfer->tx_buf = kzalloc(len, GFP_KERNEL);
		xfer->rx_buf = kzalloc(len, GFP_KERNEL);

		if ((xfer->tx_buf == NULL) || (xfer->rx_buf == NULL))
			return -1;
	}
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
		SPIDEV_LOG("Transfer addr:Tx:0x%llx, Rx:0x%llx, before\n", xfer->tx_dma, xfer->rx_dma);
#else
		SPIDEV_LOG("Transfer addr:Tx:0x%x, Rx:0x%x, before\n", xfer->tx_dma, xfer->rx_dma);
#endif

	if (flag == 0) {
		for (i = 0; i < len; i++)
			*((u8 *) xfer->tx_buf + i) = tx_buffer + i;
	} else if (flag == 1) {
		for (i = 0; i < len; i++)
			*((u8 *) xfer->tx_buf + i) = tx_buffer + i;
	} else if (flag == 2) {	/*cross 1 K boundary*/
		if (len < 2048)
			return -EINVAL;
		for (i = 0; i < len; i++)
			*((u8 *) xfer->tx_buf + i) = tx_buffer + i;

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
		SPIDEV_LOG("Transfer addr:Tx:0x%llx, Rx:0x%llx, before\n", xfer->tx_dma, xfer->rx_dma);
#else
		SPIDEV_LOG("Transfer addr:Tx:0x%x, Rx:0x%x, before\n", xfer->tx_dma, xfer->rx_dma);
#endif
		xfer->len = 32;

		xfer->tx_dma += SPI_CROSS_ALIGN_OFFSET;
		xfer->rx_dma += SPI_CROSS_ALIGN_OFFSET;

		xfer->tx_buf += SPI_CROSS_ALIGN_OFFSET;
		xfer->rx_buf += SPI_CROSS_ALIGN_OFFSET;

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
		SPIDEV_LOG("Transfer addr:Tx:0x%llx, Rx:0x%llx\n", xfer->tx_dma, xfer->rx_dma);
#else
		SPIDEV_LOG("Transfer addr:Tx:0x%x, Rx:0x%x\n", xfer->tx_dma, xfer->rx_dma);
#endif
		SPIDEV_LOG("Transfer addr:Tx:0x%p, Rx:0x%p\n", xfer->tx_buf, xfer->rx_buf);
	} else {
		return -EINVAL;
	}
	return 0;

}

static inline int is_last_xfer(struct spi_message *msg, struct spi_transfer *xfer)
{
	return msg->transfers.prev == &xfer->transfer_list;
}

static int spi_recv_check(struct spi_message *msg)
{

	struct spi_transfer *xfer;
	u32 cnt, i, err = 0;

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {

		if (!xfer) {
			SPIDEV_MSG("rv msg is NULL.\n");
			return -1;
		}
		cnt = (xfer->len % 4) ? (xfer->len / 4 + 1) : (xfer->len / 4);
		for (i = 0; i < cnt; i++) {
			if (*((u32 *) xfer->rx_buf + i) != *((u32 *) xfer->tx_buf + i)) {
				SPIDEV_LOG("tx xfer %d is:%.8x\n", i, *((u32 *) xfer->tx_buf + i));
				SPIDEV_LOG("rx xfer %d is:%.8x\n", i, *((u32 *) xfer->rx_buf + i));
				SPIDEV_LOG("\n");
				err++;
			}
		}
		if (spi1_4G_flag) {
		#if USE_SPI1_4GB_TEST
			if (spi_tx_local_buf) {
				iounmap(spi_tx_local_buf);
				spi_tx_local_buf = NULL;
			}
			if (spi_rx_local_buf) {
				iounmap(spi_rx_local_buf);
				spi_rx_local_buf = NULL;
			}
		#endif
		} else {
			kfree(xfer->tx_buf);
			kfree(xfer->rx_buf);
		}
	}

	SPIDEV_LOG("Message:0x%p,error %d,actual xfer length is:%d\n", msg, err, msg->actual_length);

	return err;

}

static int spi_recv_check_all(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_transfer *xfer;
	u32 i, err = 0;
	int j;
	u8 rec_cac = 0;
	struct mt_chip_conf *chip_config;

	chip_config = (struct mt_chip_conf *)spi->controller_data;

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {

		if (!xfer) {
			SPIDEV_MSG("rv msg is NULL.\n");
			return -1;
		}
		for (i = 0; i < xfer->len; i++) {
			if (chip_config->tx_mlsb ^ chip_config->rx_mlsb) {
				rec_cac = 0;
				for (j = 7; j >= 0; j--)
					rec_cac |= ((*((u8 *) xfer->tx_buf + i) & (1 << j)) >> j) << (7 - j);
			} else
				rec_cac = *((u8 *) xfer->tx_buf + i);
			if (*((u8 *) xfer->rx_buf + i) != rec_cac) {
				SPIDEV_LOG("tx xfer %d is:%x\n", i, *((u8 *) xfer->tx_buf + i));
				SPIDEV_LOG("rx xfer %d is:%x\n", i, *((u8 *) xfer->rx_buf + i));
				err++;
			}
		}
		if (spi1_4G_flag) {
		#if USE_SPI1_4GB_TEST
			if (spi_tx_local_buf) {
				iounmap(spi_tx_local_buf);
				spi_tx_local_buf = NULL;
			}
			if (spi_rx_local_buf) {
				iounmap(spi_rx_local_buf);
				spi_rx_local_buf = NULL;
			}
		#endif
		} else {
			kfree(xfer->tx_buf);
			kfree(xfer->rx_buf);
		}
	}

	SPIDEV_LOG("Message:0x%p,error %d,actual xfer length is:%d\n", msg, err, msg->actual_length);

	return err;

}

static void spi_complete(void *arg)
{
	static u32 i;

	stress_err += spi_recv_check((struct spi_message *)arg);
	if (stress_err > 0)
		SPIDEV_LOG("Message transfer err:%d\n", stress_err);
	if (++i == SPI_STRESS_MAX) {
		i = 0;
		complete(&mt_spi_done);
	}
}

static int threadfunc1(void *data)
{
	struct spi_transfer transfer = {0,};
	struct spi_message msg;
	struct spi_device *spi = (struct spi_device *)data;
	u32 len = 8;
	int ret;

	while (1) {
		spi_message_init(&msg);

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;

		spi_setup_xfer(&spi->dev, &transfer, len, 0);
		spi_message_add_tail(&transfer, &msg);
		ret = spi_sync(spi, &msg);
		if (ret < 0) {
			stress_err++;
			SPIDEV_LOG("Message transfer err:%d\n", ret);
		} else {
			ret = spi_recv_check(&msg);
			if (ret != 0) {
				stress_err += ret;
				SPIDEV_LOG("thread Message transfer err:%d\n", ret);
			}
		}
		schedule_timeout(HZ);
	}
	return 0;
}

static int threadfunc2(void *data)
{
	struct spi_transfer transfer = {0,};
	struct spi_message msg;
	struct spi_device *spi = (struct spi_device *)data;

	u32 len = 128;
	int ret;

	while (1) {
		spi_message_init(&msg);

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;

		spi_setup_xfer(&spi->dev, &transfer, len, 0);
		spi_message_add_tail(&transfer, &msg);
		ret = spi_sync(spi, &msg);
		if (ret < 0) {
			stress_err++;
			SPIDEV_LOG("Message transfer err:%d\n", ret);
		} else {
			ret = spi_recv_check(&msg);
			if (ret != 0) {
				stress_err += ret;
				SPIDEV_LOG("thread Message transfer err:%d\n", ret);
			}
		}
		schedule_timeout(HZ);
	}
	return 0;
}

static int threadfunc3(void *data)
{
	struct spi_message msg;
	struct spi_device *spi = (struct spi_device *)data;
	static struct spi_message *p;
	u32 len = 64;
	int ret;
	u16 i;

	while (1) {
		spi_message_init(&msg);

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;

		p = stress_msg;
		for (i = 0; i < SPI_STRESS_MAX; i++) {
			spi_message_init(p);
			ret = spi_setup_xfer(&spi->dev, &stress_xfer[i], len, 0);
			if (ret != 0)
				SPIDEV_LOG("xfer set up err:%d\n", ret);
			spi_message_add_tail(&stress_xfer[i], p);

			p->complete = spi_complete;
			p->context = p;
			ret = spi_async(spi, p);
			if (ret < 0)
				SPIDEV_LOG("Message %d transfer err:%d\n", i, ret);
			p++;
		}

		wait_for_completion(&mt_spi_done);

		schedule_timeout(5 * HZ);
	}
	return 0;
}
static int threadfunc4(void *data)
{
	struct spi_message msg;
	struct spi_device *spi = (struct spi_device *)data;

	u32 len = 32;
	int ret;
	u16 i;

	while (1) {
		spi_message_init(&msg);

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;

		for (i = 0; i < SPI_STRESS_MAX; i++) {
			ret = spi_setup_xfer(&spi->dev, &stress_xfer_con[i], len, 1);
			if (ret != 0)
				SPIDEV_LOG("Message set up err:%d\n", ret);
			spi_message_add_tail(&stress_xfer_con[i], &msg);

		}
		ret = spi_sync(spi, &msg);
		if (ret < 0) {
			SPIDEV_LOG("Message transfer err:%d\n", ret);
		} else {
			ret = spi_recv_check(&msg);
			if (ret != 0) {
				stress_err += ret;
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			}
		/*SPIDEV_LOG("Message multi xfer stress pass\n");*/
		}

		schedule_timeout(2 * HZ);
	}
	return 0;
}

#ifdef SPI_TRUSTONIC_TEE_SUPPORT
#define DEFAULT_HANDLES_NUM (64)
#define MAX_OPEN_SESSIONS (0xffffffff - 1)
/*
* Trustlet UUID.
*/
u8 spi_uuid[6][16] = {{0x09, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

static struct mc_session_handle secspi_session = { 0 };

static u32 secspi_session_ref;
static u32 secspi_devid = MC_DEVICE_ID_DEFAULT;
static tciSpiMessage_t *secspi_tci;

static DEFINE_MUTEX(secspi_lock);

int secspi_session_open(u32 spinum)
{
	enum mc_result mc_ret = MC_DRV_OK;

	mutex_lock(&secspi_lock);

	SPIDEV_MSG("secspi_session_open start\n");
	do {
		/* sessions reach max numbers ? */

		if (secspi_session_ref > MAX_OPEN_SESSIONS) {
			SPIDEV_MSG("secspi_session > 0x%x\n", MAX_OPEN_SESSIONS);
			break;
		}

		if (secspi_session_ref > 0) {
			secspi_session_ref++;
			break;
		}

		/* open device */
		mc_ret = mc_open_device(secspi_devid);
		if (MC_DRV_OK != mc_ret) {
			SPIDEV_MSG("mc_open_device failed: %d\n", mc_ret);
			break;
		}

		/* allocating WSM for DCI */
		mc_ret = mc_malloc_wsm(secspi_devid, 0, sizeof(tciSpiMessage_t), (uint8_t **)&secspi_tci, 0);
		if (MC_DRV_OK != mc_ret) {
			SPIDEV_MSG("mc_malloc_wsm failed: %d\n", mc_ret);
			mc_close_device(secspi_devid);
			break;
		}

		/* open session */
		secspi_session.device_id = secspi_devid;
		mc_ret = mc_open_session(&secspi_session, (struct mc_uuid_t *)&spi_uuid[spinum][0],
				(uint8_t *)secspi_tci, sizeof(tciSpiMessage_t));

		if (MC_DRV_OK != mc_ret) {
			SPIDEV_MSG("secspi_session_open fail: %d\n", mc_ret);
			mc_free_wsm(secspi_devid, (uint8_t *) secspi_tci);
			mc_close_device(secspi_devid);
			secspi_tci = NULL;
			break;
		}
		secspi_session_ref = 1;

	} while (0);

	SPIDEV_MSG("secspi_session_open: ret=%d, ref=%d\n", mc_ret, secspi_session_ref);

	mutex_unlock(&secspi_lock);
	SPIDEV_MSG("secspi_session_open end\n");

	if (MC_DRV_OK != mc_ret)
		return -ENXIO;

	return 0;
}

int secspi_execute(u32 cmd, tciSpiMessage_t *param, struct mt_spi_t *ms)
{
	enum mc_result mc_ret;

	SPIDEV_MSG("secspi_execute\n");
	mutex_lock(&secspi_lock);

	if (NULL == secspi_tci) {
		mutex_unlock(&secspi_lock);
		SPIDEV_MSG("secspi_tci not exist\n");
		return -ENODEV;
	}

	/*set transfer data para */
	if (NULL == param) {
		SPIDEV_MSG("secspi_execute parameter is NULL !!\n");
	} else {
		secspi_tci->tx_buf = param->tx_buf;
		secspi_tci->rx_buf = param->rx_buf;
		secspi_tci->len = param->len;
		secspi_tci->is_dma_used = param->is_dma_used;
		secspi_tci->tx_dma = param->tx_dma;
		secspi_tci->rx_dma = param->rx_dma;
		secspi_tci->tl_chip_config = param->tl_chip_config;
	}

	secspi_tci->cmd_spi.header.commandId = (tciCommandId_t) cmd;
	secspi_tci->cmd_spi.len = 0;

	SPIDEV_MSG("mc_notify\n");

	/*enable_clock(MT_CG_PERI_SPI0, "spi"); */
	mt_spi_enable_clk(ms);

	mc_ret = mc_notify(&secspi_session);

	if (MC_DRV_OK != mc_ret) {
		SPIDEV_MSG("mc_notify failed: %d", mc_ret);
		goto exit;
	}

	SPIDEV_MSG("SPI mc_wait_notification\n");
	mc_ret = mc_wait_notification(&secspi_session, -1);

	if (MC_DRV_OK != mc_ret) {
		SPIDEV_MSG("SPI mc_wait_notification failed: %d", mc_ret);
		goto exit;
	}

 exit:

	mutex_unlock(&secspi_lock);

	if (MC_DRV_OK != mc_ret)
		return -ENOSPC;

	return 0;
}
#if 1
static int secspi_session_close(void)
{
	enum mc_result mc_ret = MC_DRV_OK;

	mutex_lock(&secspi_lock);

	do {
		/* session is already closed ? */
		if (secspi_session_ref == 0) {
			SPIDEV_MSG("spi_session already closed\n");
			break;
		}

		if (secspi_session_ref > 1) {
			secspi_session_ref--;
			break;
		}

		/* close session */
		mc_ret = mc_close_session(&secspi_session);
		if (MC_DRV_OK != mc_ret) {
			SPIDEV_MSG("SPI mc_close_session failed: %d\n", mc_ret);
			break;
		}

		/* free WSM for DCI */
		mc_ret = mc_free_wsm(secspi_devid, (uint8_t *) secspi_tci);
		if (MC_DRV_OK != mc_ret) {
			SPIDEV_MSG("SPI mc_free_wsm failed: %d\n", mc_ret);
			break;
		}
		secspi_tci = NULL;
		secspi_session_ref = 0;

		/* close device */
		mc_ret = mc_close_device(secspi_devid);
		if (MC_DRV_OK != mc_ret)
			SPIDEV_MSG("SPI mc_close_device failed: %d\n", mc_ret);

	} while (0);

	SPIDEV_MSG("secspi_session_close: ret=%d, ref=%d\n", mc_ret, secspi_session_ref);

	mutex_unlock(&secspi_lock);

	if (MC_DRV_OK != mc_ret)
		return -ENXIO;

	return 0;

}
#endif
#endif

static ssize_t spi_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct spi_device *spi;

	struct mt_chip_conf *chip_config;

	u32 setuptime, holdtime, high_time, low_time;
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
	u32 spinum;
#endif
	u32 cs_idletime, ulthgh_thrsh;
	int cpol, cpha, tx_mlsb, rx_mlsb, tx_endian, sample_sel, cs_pol;
	int rx_endian, com_mod, pause, finish_intr;
	int deassert, tckdly, ulthigh;

	struct spi_master *master;
	struct mt_spi_t *ms;
	spi = container_of(dev, struct spi_device, dev);
	master = spi->master;
	ms = spi_master_get_devdata(master);

	SPIDEV_LOG("SPIDEV name is:%s\n", spi->modalias);

	chip_config = (struct mt_chip_conf *)spi->controller_data;

	if (!chip_config) {
		SPIDEV_LOG("chip_config is NULL.\n");
		chip_config = kzalloc(sizeof(struct mt_chip_conf), GFP_KERNEL);
		if (!chip_config)
			return -ENOMEM;
	}
	if (!buf) {
		SPIDEV_LOG("buf is NULL.\n");
		goto out;
	}
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
	if (!strncmp(buf, "send", 4) && (1 == sscanf(buf + 4, "%d", &spinum))) {
		/*TRANSFER*/ SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_execute(1, NULL, ms);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 1 finished!!!\n");
	} else if (!strncmp(buf, "config", 6) && (1 == sscanf(buf + 6, "%d", &spinum))) {	/*HW CONFIG */
		SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_execute(2, NULL, ms);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 2 finished!!!\n");
	} else if (!strncmp(buf, "debug", 5) && (1 == sscanf(buf + 5, "%d", &spinum))) {
		/*DEBUG*/ SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_execute(3, NULL, ms);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 3 finished!!!\n");
	} else if (!strncmp(buf, "test", 4) && (1 == sscanf(buf + 4, "%d", &spinum))) {
		/*TEST*/ SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_execute(4, NULL, ms);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 4 finished!!!\n");
#else
	if (!strncmp(buf, "-h", 2)) {
		SPIDEV_MSG("Please input the parameters for this device.\n");
#endif
	} else if (!strncmp(buf, "-w", 2)) {
		buf += 3;
		if (!strncmp(buf, "setuptime=", 10) && (1 == sscanf(buf + 10, "%d", &setuptime))) {
			SPIDEV_MSG("setuptime is:%d\n", setuptime);
			chip_config->setuptime = setuptime;
		} else if (!strncmp(buf, "holdtime=", 9) && (1 == sscanf(buf + 9, "%d", &holdtime))) {
			SPIDEV_MSG("Set holdtime is:%d\n", holdtime);
			chip_config->holdtime = holdtime;
		} else if (!strncmp(buf, "high_time=", 10) && (1 == sscanf(buf + 10, "%d", &high_time))) {
			SPIDEV_MSG("Set high_time is:%d\n", high_time);
			chip_config->high_time = high_time;
		} else if (!strncmp(buf, "low_time=", 9) && (1 == sscanf(buf + 9, "%d", &low_time))) {
			SPIDEV_MSG("Set low_time is:%d\n", low_time);
			chip_config->low_time = low_time;
		} else if (!strncmp(buf, "cs_idletime=", 12) && (1 == sscanf(buf + 12, "%d", &cs_idletime))) {
			SPIDEV_MSG("Set cs_idletime is:%d\n", cs_idletime);
			chip_config->cs_idletime = cs_idletime;
		} else if (!strncmp(buf, "ulthgh_thrsh=", 13) && (1 == sscanf(buf + 13, "%d", &ulthgh_thrsh))) {
			SPIDEV_MSG("Set slwdown_thrsh is:%d\n", ulthgh_thrsh);
			chip_config->ulthgh_thrsh = ulthgh_thrsh;
		} else if (!strncmp(buf, "cpol=", 5) && (1 == sscanf(buf + 5, "%d", &cpol))) {
			SPIDEV_MSG("Set cpol is:%d\n", cpol);
			chip_config->cpol = cpol;
		} else if (!strncmp(buf, "cpha=", 5) && (1 == sscanf(buf + 5, "%d", &cpha))) {
			SPIDEV_MSG("Set cpha is:%d\n", cpha);
			chip_config->cpha = cpha;
		} else if (!strncmp(buf, "tx_mlsb=", 8) && (1 == sscanf(buf + 8, "%d", &tx_mlsb))) {
			SPIDEV_MSG("Set tx_mlsb is:%d\n", tx_mlsb);
			chip_config->tx_mlsb = tx_mlsb;
		} else if (!strncmp(buf, "rx_mlsb=", 8) && (1 == sscanf(buf + 8, "%d", &rx_mlsb))) {
			SPIDEV_MSG("Set rx_mlsb is:%d\n", rx_mlsb);
			chip_config->rx_mlsb = rx_mlsb;
		} else if (!strncmp(buf, "tx_endian=", 10) && (1 == sscanf(buf + 10, "%d", &tx_endian))) {
			SPIDEV_MSG("Set tx_endian is:%d\n", tx_endian);
			chip_config->tx_endian = tx_endian;
		} else if (!strncmp(buf, "rx_endian=", 10) && (1 == sscanf(buf + 10, "%d", &rx_endian))) {
			SPIDEV_MSG("Set rx_endian is:%d\n", rx_endian);
			chip_config->rx_endian = rx_endian;
		} else if (!strncmp(buf, "com_mod=", 8) && (1 == sscanf(buf + 8, "%d", &com_mod))) {
			chip_config->com_mod = com_mod;
			SPIDEV_MSG("Set com_mod is:%d\n", com_mod);
		} else if (!strncmp(buf, "pause=", 6) && (1 == sscanf(buf + 6, "%d", &pause))) {
			SPIDEV_MSG("Set pause is:%d\n", pause);
			chip_config->pause = pause;
		} else if (!strncmp(buf, "finish_intr=", 12) && (1 == sscanf(buf + 12, "%d", &finish_intr))) {
			SPIDEV_MSG("Set finish_intr is:%d\n", finish_intr);
			chip_config->finish_intr = finish_intr;
		} else if (!strncmp(buf, "deassert=", 9) && (1 == sscanf(buf + 9, "%d", &deassert))) {
			SPIDEV_MSG("Set deassert is:%d\n", deassert);
			chip_config->deassert = deassert;
		} else if (!strncmp(buf, "ulthigh=", 8) && (1 == sscanf(buf + 8, "%d", &ulthigh))) {
			SPIDEV_MSG("Set ulthigh is:%d\n", ulthigh);
			chip_config->ulthigh = ulthigh;
		} else if (!strncmp(buf, "tckdly=", 7) && (1 == sscanf(buf + 7, "%d", &tckdly))) {
			SPIDEV_MSG("Set tckdly is:%d\n", tckdly);
			chip_config->tckdly = tckdly;
		} else if (!strncmp(buf, "sample_sel=", 11) && (1 == sscanf(buf + 11, "%d", &sample_sel))) {
			SPIDEV_MSG("Set sample_sel is:%d\n", sample_sel);
			chip_config->sample_sel = sample_sel;
		} else if (!strncmp(buf, "cs_pol=", 7) && (1 == sscanf(buf + 7, "%d", &cs_pol))) {
			SPIDEV_MSG("Set cs_pol is:%d\n", cs_pol);
			chip_config->cs_pol = cs_pol;
		} else {
			SPIDEV_LOG("Wrong parameters.\n");
			goto out;
		}
		spi->controller_data = chip_config;
		/*spi_setup(spi);*/

	}
 out:
	if (!spi->controller_data)
		kfree(chip_config);
	return count;
}

static int tc_spi_cross_1k(struct spi_device *spi)
{
	int ret = 0;
	/*struct spi_transfer transfer;
	struct spi_message msg;
	spi_setup_xfer(&transfer,2048,2);
	spi_message_add_tail ( &transfer, &msg );
	msg.is_dma_mapped=1;
	ret = spi_sync ( spi_test, &msg );
	if(ret < 0){
		SPIDEV_LOG("Message transfer err:%d\n", ret);
	}else{
		ret = spi_recv_check(&msg);
	}*/
	return ret;
}

static struct spi_transfer test_xfer;
static struct spi_transfer test_xfer2;
static struct spi_transfer test_xfer3;
static ssize_t spi_msg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	struct spi_message *p;
	int ret = 0;
	struct spi_device *spi;
	struct spi_message msg;
	struct spi_message msg2;
	struct mt_chip_conf *chip_config;

	u32 i, len = 4;
	u32 tx_buffer = 0x12345678;
	u32 rx_buffer = 0xaaaaaaaa;

	test_xfer.tx_buf = &tx_buffer;
	test_xfer.rx_buf = &rx_buffer;
	test_xfer.len = 4;

	spi = container_of(dev, struct spi_device, dev);

	if (unlikely(!spi)) {
		SPIDEV_LOG("spi device is invalid\n");
		goto out;
	}
	if (unlikely(!buf)) {
		SPIDEV_LOG("buf is invalid\n");
		goto out;
	}
	spi_message_init(&msg);
	spi_message_init(&msg2);

	if (!strncmp(buf, "-h", 2)) {
		SPIDEV_MSG("Please input the message of this device to send and receive.\n");
	} else if (!strncmp(buf, "-w", 2)) {
		buf += 3;
		if (!buf) {
			SPIDEV_LOG("Parameter is not enough.\n");
			goto out;
		}
		if (!strncmp(buf, "len=", 4) && 1 == sscanf(buf + 4, "%d", &len)) {
			spi_setup_xfer(&spi->dev, &test_xfer, len, 0);
			spi_message_add_tail(&test_xfer, &msg);
			ret = spi_sync(spi, &msg);
			if (ret < 0) {
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			} else {
				ret = spi_recv_check_all(spi, &msg);
				if (ret != 0) {
					ret = -ret;
					SPIDEV_LOG("Message transfer err:%d\n", ret);
					goto out;
				}
			}
		}
	} else if (!strncmp(buf, "-func", 5)) {
		buf += 6;
		if (!buf) {
			SPIDEV_LOG("Parameter is not enough.\n");
			goto out;
		}
		if (!strncmp(buf, "len=", 4) && 1 == sscanf(buf + 4, "%d", &len)) {
			spi_setup_xfer(&spi->dev, &test_xfer, len, 1);
			spi_message_add_tail(&test_xfer, &msg);
			ret = spi_sync(spi, &msg);
			if (ret < 0) {
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			} else {
				ret = spi_recv_check(&msg);
				if (ret != 0) {
					ret = -ret;
					SPIDEV_LOG("Message transfer err:%d\n", ret);
					goto out;
				}
			}
		}
		if (!strncmp(buf, "cross", 5)) {
			ret = tc_spi_cross_1k(spi);
			if (ret < 0) {
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			} else if (ret != 0) {
				ret = -ret;
				SPIDEV_LOG("Message transfer err:%d\n", ret);
				goto out;
			}
		}
	} else if (!strncmp(buf, "-err", 4)) {
		buf += 5;
		chip_config = (struct mt_chip_conf *)spi->controller_data;

		if (!strncmp(buf, "buf", 3)) {	/*case: tx_buf = NULL,rx_buf = NULL*/
			test_xfer.len = 8;
			test_xfer.tx_buf = NULL;
			test_xfer.rx_buf = NULL;
		} else if (!strncmp(buf, "len", 3)) {	/*case: tx_buf != NULL,rx_buf != NULL, len = 0*/
			test_xfer.len = 0;
			test_xfer.tx_buf = kzalloc(8, GFP_KERNEL);
			test_xfer.rx_buf = kzalloc(8, GFP_KERNEL);
		} else if (!strncmp(buf, "nomem", 5)) {	/*case: DMA mapping error*/
			spi_setup_xfer(&spi->dev, &test_xfer, 8, 0);
		} else if (!strncmp(buf, "fifo_len", 8)) {	/*case: len exceed FIFO size*/
			chip_config->com_mod = 0;
			spi_setup_xfer(&spi->dev, &test_xfer, 33, 0);
		} else if (!strncmp(buf, "dma_len", 7)) {	/*case: len exceed DMA size*/
			chip_config->com_mod = 1;
			spi_setup_xfer(&spi->dev, &test_xfer, 1025, 0);
		} else if (!strncmp(buf, "xfer", 4)) {	/*case: len exceed DMA size*/
			;
		} else if (!strncmp(buf, "msg", 3)) {	/*case: len exceed DMA size*/
			;
		} else {
			SPIDEV_LOG("Error test Wrong parameters.\n");
			ret = -1;
			goto out;
		}

		if (strncmp(buf, "xfer", 4)) {	/*case: MSG empty*/
			spi_message_add_tail(&test_xfer, &msg);
		}
		if (!strncmp(buf, "msg", 3)) {	/*case: message = NULL*/
			ret = spi_sync(spi, NULL);

		} else {
			ret = spi_sync(spi, &msg);
		}
		if (ret != -EINVAL)
			return -100;
		SPIDEV_LOG("Message transfer error test passed ret:%d\n", ret);
	} else if (!strncmp(buf, "-pause", 6)) {
		spi_setup_xfer(&spi->dev, &test_xfer, 32, 0);
		spi_message_add_tail(&test_xfer, &msg);
		spi_setup_xfer(&spi->dev, &test_xfer2, 1024, 0);
		spi_message_add_tail(&test_xfer2, &msg);
		spi_setup_xfer(&spi->dev, &test_xfer3, 32, 0);
		spi_message_add_tail(&test_xfer3, &msg);

		ret = spi_sync(spi, &msg);
		if (ret < 0)
			SPIDEV_LOG("Message transfer err:%d\n", ret);
		else {
			ret = spi_recv_check(&msg);
			if (ret != 0) {
				ret = -ret;
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			}
		}
	} else if (!strncmp(buf, "-test", 5)) {
		spi_setup_xfer(&spi->dev, &test_xfer, 8, 0);
		spi_message_add_tail(&test_xfer, &msg);
		ret = spi_sync(spi, &msg);
		if (ret < 0)
			SPIDEV_LOG("Message transfer err:%d\n", ret);
		else {
			ret = spi_recv_check(&msg);
			if (ret != 0) {
				ret = -ret;
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			}
		}
		spi_setup_xfer(&spi->dev, &test_xfer2, 8, 0);
		spi_message_add_tail(&test_xfer2, &msg2);

		ret = spi_sync(spi, &msg2);
		if (ret < 0)
			SPIDEV_LOG("Message transfer err:%d\n", ret);
		else {
			ret = spi_recv_check(&msg2);
			if (ret != 0) {
				ret = -ret;
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			}
		}
	} else if (!strncmp(buf, "-stress", 7) && 1 == sscanf(buf + 8, "%d", &len)) {
		stress_err = 0;
		if (len == 0) {
			/*xfer = kzalloc(SPI_STRESS_MAX*sizeof( struct spi_transfer) , GFP_KERNEL);*/
			/*will release in spi_recv_check() function*/
			SPIDEV_LOG("Message multi xfer stress start\n");
			for (i = 0; i < SPI_STRESS_MAX; i++) {
				ret = spi_setup_xfer(&spi->dev, &stress_xfer[i], 32, 0);
				if (ret != 0)
					SPIDEV_LOG("Message set up err:%d\n", ret);
				spi_message_add_tail(&stress_xfer[i], &msg);

			}
			ret = spi_sync(spi, &msg);
			if (ret < 0) {
				SPIDEV_LOG("Message transfer err:%d\n", ret);
			} else {
				ret = spi_recv_check(&msg);
				if (ret != 0) {
					ret = -ret;
					SPIDEV_LOG("Message transfer err:%d\n", ret);
				} else {
					SPIDEV_LOG("Message multi xfer stress pass\n");
				}
			}
		} else if (len == 1) {
			int loop_count = 0;
			/*loop_count  =250, take aboat half a hour.*/
			for (loop_count = 0; loop_count <= 5000; loop_count++) {
				/*msg_str = kzalloc(SPI_STRESS_MAX * sizeof(struct spi_message), GFP_KERNEL);*/
				/*xfer = kzalloc(SPI_STRESS_MAX *	sizeof(struct spi_transfer) , GFP_KERNEL);*/
				SPIDEV_LOG("Message multi msg stress start\n");
				p = stress_msg;
				for (i = 0; i < SPI_STRESS_MAX; i++) {
					spi_message_init(p);
					ret = spi_setup_xfer(&spi->dev, &stress_xfer[i], 32, 0);
					if (ret != 0)
						SPIDEV_LOG("xfer set up err:%d\n", ret);
					spi_message_add_tail(&stress_xfer[i], p);

					p->complete = spi_complete;
					p->context = p;
					p++;
				}
				p = stress_msg;
				for (i = 0; i < SPI_STRESS_MAX; i++) {
					ret = spi_async(spi, p);
					if (ret < 0)
						SPIDEV_LOG("Message %d transfer err:%d\n", i, ret);
					p++;
				}
				wait_for_completion(&mt_spi_done);
			}
			/*kfree(msg_str);*/
			if (stress_err != 0) {
				ret = -stress_err;
				stress_err = 0;
				SPIDEV_LOG("Message stress err:%d\n", ret);
			} else {
				SPIDEV_LOG("Message stress Pass\n");
				ret = 0;
			}
		} else {
		}
	} else if (!strncmp(buf, "-concurrent", 11)) {

		stress_err = 0;
		spi_concur1 = kthread_run(threadfunc1, (void *)spi, "spi_concrrent1");
		if (IS_ERR(spi_concur1)) {
			SPIDEV_LOG("Unable to start kernelthread 1\n");
			ret = -5;
			goto out;
		}
		spi_concur2 = kthread_run(threadfunc2, (void *)spi, "spi_concrrent2");
		if (IS_ERR(spi_concur2)) {
			SPIDEV_LOG("Unable to start kernelthread 2\n");
			ret = -5;
			goto out;
		}
		spi_concur3 = kthread_run(threadfunc3, (void *)spi, "spi_concrrent3");
		if (IS_ERR(spi_concur3)) {
			SPIDEV_LOG("Unable to start kernelthread 3\n");
			ret = -5;
			goto out;
		}
		spi_concur4 = kthread_run(threadfunc4, (void *)spi, "spi_concrrent4");
		if (IS_ERR(spi_concur4)) {
			SPIDEV_LOG("Unable to start kernelthread 4\n");
			ret = -5;
			goto out;
		}

		msleep(10000);
		SPIDEV_LOG("stop kernelthread\n");

		kthread_stop(spi_concur1);
		kthread_stop(spi_concur2);
		kthread_stop(spi_concur3);
		kthread_stop(spi_concur4);
		ret = -stress_err;
		stress_err = 0;
		SPIDEV_LOG("concurrent test done %d\n\n\n", ret);

	} else {
		SPIDEV_LOG("Wrong parameters.\n");
		ret = -1;
		goto out;
	}
	ret = count;

out:
	return ret;

}

static DEVICE_ATTR(spi, 0200, NULL, spi_store);
static DEVICE_ATTR(spi_msg, 0200, NULL, spi_msg_store);

static struct device_attribute *spi_attribute[] = {
	&dev_attr_spi,
	&dev_attr_spi_msg,
};

static int spi_create_attribute(struct device *dev)
{
	int num, idx;
	int err = 0;

	num = (int)(sizeof(spi_attribute) / sizeof(spi_attribute[0]));
	for (idx = 0; idx < num; idx++) {
		err = device_create_file(dev, spi_attribute[idx]);
		if (err)
			break;
	}
	return err;
}

static void spi_remove_attribute(struct device *dev)
{
	int num, idx;

	num = (int)(sizeof(spi_attribute) / sizeof(spi_attribute[0]));
	for (idx = 0; idx < num; idx++)
		device_remove_file(dev, spi_attribute[idx]);
}

static int spi_test_remove(struct spi_device *spi)
{
	SPIDEV_LOG("spi_test_remove.\n");
	spi_remove_attribute(&spi->dev);
	return 0;
}

static int spi_test_probe(struct spi_device *spi)
{
	SPIDEV_LOG("spi test probe  enter\n");
	/*spi_test = spi;*/
	spi->mode = SPI_MODE_3;
	spi->bits_per_word = 32;
	return spi_create_attribute(&spi->dev);
	return 0;
}

struct spi_device_id spi_id_table[] = {
	{"spi-ut0", 0},
	{"spi-ut1", 1},
	{"spi-ut2", 2},
	{"spi-ut3", 3},
	{"spi-ut4", 4},
	{"spi-ut5", 5}
	};

static struct spi_driver spi_test_driver = {
	.driver = {
		.name = "test_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
		},
	.probe = spi_test_probe,
	.remove = spi_test_remove,
	.id_table = spi_id_table,
};

static struct spi_board_info spi_board_devs[] __initdata = {
	[0] = {
	.modalias = "spi-ut0",
	.bus_num = 0,
	.chip_select = 1,
	.mode = SPI_MODE_3,
	},
	[1] = {
	.modalias = "spi-ut1",
	.bus_num = 1,
	.chip_select = 1,
	.mode = SPI_MODE_3,
	},
	[2] = {
	.modalias = "spi-ut2",
	.bus_num = 2,
	.chip_select = 1,
	.mode = SPI_MODE_3,
	},
	[3] = {
	.modalias = "spi-ut3",
	.bus_num = 3,
	.chip_select = 1,
	.mode = SPI_MODE_3,
	},
	[4] = {
	.modalias = "spi-ut4",
	.bus_num = 4,
	.chip_select = 1,
	.mode = SPI_MODE_3,
	},
	[5] = {
	.modalias = "spi-ut5",
	.bus_num = 5,
	.chip_select = 1,
	.mode = SPI_MODE_3,
	}
};

static int __init spi_dev_init(void)
{
	SPIDEV_LOG("SPI_DEV_INIT.\n");
	spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
	return spi_register_driver(&spi_test_driver);
}

static void __exit spi_test_exit(void)
{
	SPIDEV_LOG("SPI_DEV_EXIT.\n");
	spi_unregister_driver(&spi_test_driver);
}

module_init(spi_dev_init);
module_exit(spi_test_exit);

MODULE_DESCRIPTION("mt SPI test device driver");
MODULE_AUTHOR("Ranran Lu <ranran.lu@mediatek.com>");
MODULE_LICENSE("GPL");
