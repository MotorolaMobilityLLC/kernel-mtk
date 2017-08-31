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
#include <linux/platform_data/spi-mt65xx.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/clk.h>

#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
#define SPI_TRUSTONIC_TEE_SUPPORT
#endif

#ifdef SPI_TRUSTONIC_TEE_SUPPORT
#include <mobicore_driver_api.h>
#include <tlspi_Api.h>
#endif

#define SPIDEV_LOG(fmt, args...) pr_err("[SPI-UT]: [%s]:[%d]" fmt, __func__, __LINE__, ##args)
#define SPIDEV_MSG(fmt, args...) pr_err(fmt, ##args)

static struct spi_device *spi_test;

struct mtk_spi {
	void __iomem *base;
	void __iomem *peri_regs;
	u32 state;
	int pad_num;
	u32 *pad_sel;
	struct clk *parent_clk, *sel_clk, *spi_clk;
	struct spi_transfer *cur_transfer;
	u32 xfer_len;
	struct scatterlist *tx_sgl, *rx_sgl;
	u32 tx_sgl_len, rx_sgl_len;
	const struct mtk_spi_compatible *dev_comp;
	u32 dram_8gb_offset;
};



#ifdef SPI_TRUSTONIC_TEE_SUPPORT
#define DEFAULT_HANDLES_NUM (64)
#define MAX_OPEN_SESSIONS (0xffffffff - 1)
/*
* Trustlet UUID.
*/
u8 spi_uuid[10][16] = {{0x09, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x09, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

static struct mc_session_handle secspi_session = { 0 };

static u32 secspi_session_ref;
static u32 secspi_devid = MC_DEVICE_ID_DEFAULT;
static tciSpiMessage_t *secspi_tci;

static DEFINE_MUTEX(secspi_lock);

int secspi_session_open(u32 spinum)
{
	enum mc_result mc_ret = MC_DRV_OK;

	mutex_lock(&secspi_lock);


	pr_warn("secspi_session_open start\n");
	do {
		/* sessions reach max numbers ? */

		if (secspi_session_ref > MAX_OPEN_SESSIONS) {
			pr_warn("secspi_session > 0x%x\n", MAX_OPEN_SESSIONS);
			break;
		}

		if (secspi_session_ref > 0) {
			secspi_session_ref++;
			break;
		}

		/* open device */
		mc_ret = mc_open_device(secspi_devid);
		if (mc_ret != MC_DRV_OK) {
			pr_warn("mc_open_device failed: %d\n", mc_ret);
			break;
		}

		/* allocating WSM for DCI */
		mc_ret =
		    mc_malloc_wsm(secspi_devid, 0, sizeof(tciSpiMessage_t),
				  (uint8_t **) &secspi_tci, 0);
		if (mc_ret != MC_DRV_OK) {
			pr_warn("mc_malloc_wsm failed: %d\n", mc_ret);
			mc_close_device(secspi_devid);
			break;
		}

		/* open session */
		secspi_session.device_id = secspi_devid;
		/* mc_ret =
		 *   mc_open_session(&secspi_session, &secspi_uuid, (uint8_t *) secspi_tci,
		 *		    sizeof(tciSpiMessage_t));
		 */
		mc_ret = mc_open_session(&secspi_session, (struct mc_uuid_t *)&spi_uuid[spinum][0],
				(uint8_t *)secspi_tci, sizeof(tciSpiMessage_t));

		if (mc_ret != MC_DRV_OK) {
			pr_warn("secspi_session_open fail: %d\n", mc_ret);
			mc_free_wsm(secspi_devid, (uint8_t *) secspi_tci);
			mc_close_device(secspi_devid);
			secspi_tci = NULL;
			break;
		}
		secspi_session_ref = 1;

	} while (0);

	pr_warn("secspi_session_open: ret=%d, ref=%d\n", mc_ret, secspi_session_ref);

	mutex_unlock(&secspi_lock);
	pr_err("secspi_session_open end\n");

	if (mc_ret != MC_DRV_OK)
		return -ENXIO;

	return 0;
}

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
		if (mc_ret != MC_DRV_OK) {
			SPIDEV_MSG("SPI mc_close_session failed: %d\n", mc_ret);
			break;
		}

		/* free WSM for DCI */
		mc_ret = mc_free_wsm(secspi_devid, (uint8_t *) secspi_tci);
		if (mc_ret != MC_DRV_OK) {
			SPIDEV_MSG("SPI mc_free_wsm failed: %d\n", mc_ret);
			break;
		}
		secspi_tci = NULL;
		secspi_session_ref = 0;

		/* close device */
		mc_ret = mc_close_device(secspi_devid);
		if (mc_ret != MC_DRV_OK)
			SPIDEV_MSG("SPI mc_close_device failed: %d\n", mc_ret);

	} while (0);

	SPIDEV_MSG("secspi_session_close: ret=%d, ref=%d\n", mc_ret, secspi_session_ref);

	mutex_unlock(&secspi_lock);

	if (mc_ret != MC_DRV_OK)
		return -ENXIO;

	return 0;

}

void secspi_enable_clk(struct spi_device *spidev)
{
	int ret;
	struct spi_master *master;
	struct mtk_spi *ms;

	master = spidev->master;
	ms = spi_master_get_devdata(master);
	/*
	 * prepare the clock source
	 */
	ret = clk_prepare_enable(ms->spi_clk);
}

int secspi_execute(u32 cmd, tciSpiMessage_t *param)
{
	enum mc_result mc_ret;

	pr_warn("secspi_execute\n");
	mutex_lock(&secspi_lock);

	if (secspi_tci == NULL) {
		mutex_unlock(&secspi_lock);
		pr_warn("secspi_tci not exist\n");
		return -ENODEV;
	}

	/*set transfer data para */
	if (param == NULL) {
		pr_warn("secspi_execute parameter is NULL !!\n");
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

	pr_warn("mc_notify\n");

	/* enable_clock(MT_CG_PERI_SPI0, "spi"); */
	/* enable_clk(ms); */

	mc_ret = mc_notify(&secspi_session);

	if (mc_ret != MC_DRV_OK) {
		pr_warn("mc_notify failed: %d", mc_ret);
		goto exit;
	}

	pr_warn("SPI mc_wait_notification\n");
	mc_ret = mc_wait_notification(&secspi_session, -1);

	if (mc_ret != MC_DRV_OK) {
		pr_warn("SPI mc_wait_notification failed: %d", mc_ret);
		goto exit;
	}

exit:

	mutex_unlock(&secspi_lock);

	if (mc_ret != MC_DRV_OK)
		return -ENOSPC;

	return 0;
}

/*used for REE to detach IRQ of TEE*/
void spi_detach_irq_tee(u32 spinum)
{
	secspi_session_open(spinum);
	secspi_execute(2, NULL);
	pr_warn("secspi_execute 2 finished!!!\n");
}
#endif
void mt_spi_disable_master_clk(struct spi_device *spidev)
{
	struct mtk_spi *ms;

	ms = spi_master_get_devdata(spidev->master);
	/*
	 * unprepare the clock source
	 */
	clk_disable_unprepare(ms->spi_clk);
}
EXPORT_SYMBOL(mt_spi_disable_master_clk);

void mt_spi_enable_master_clk(struct spi_device *spidev)
{
	int ret;
	struct mtk_spi *ms;

	ms = spi_master_get_devdata(spidev->master);
	/*
	 * prepare the clock source
	 */
	ret = clk_prepare_enable(ms->spi_clk);
}
EXPORT_SYMBOL(mt_spi_enable_master_clk);

static int spi_setup_xfer(struct device *dev, struct spi_transfer *xfer, u32 len, u32 flag)
{
	u32 tx_buffer = 0x12345678;
	u32 cnt, i;

	xfer->len = len;

	xfer->tx_buf = kzalloc(len, GFP_KERNEL);
	xfer->rx_buf = kzalloc(len, GFP_KERNEL);
	xfer->speed_hz = 500000;
	xfer->len = len;

	/* Instead of using kzalloc, if using below
	 * dma_alloc_coherent to allocate tx_dma/rx_dma, remember:
	 * 1. remove kfree in spi_recv_check_all, otherwise KE reboot
	 * 2. set msg.is_dma_mapped = 1 before calling spi_sync();
	 */

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	SPIDEV_LOG("Transfer addr:Tx:0x%llx, Rx:0x%llx\n", xfer->tx_dma, xfer->rx_dma);
#else
	SPIDEV_LOG("Transfer addr:Tx:0x%x, Rx:0x%x\n", xfer->tx_dma, xfer->rx_dma);
#endif

	if ((xfer->tx_buf == NULL) || (xfer->rx_buf == NULL))
		return -1;

	cnt = (len%4)?(len/4 + 1):(len/4);

	if (flag == 0) {
		for (i = 0; i < cnt; i++)
			*((u32 *)xfer->tx_buf + i) = tx_buffer;
	} else if (flag == 1) {
		for (i = 0; i < cnt; i++)
			*((u32 *)xfer->tx_buf + i) = tx_buffer + i;
	} else if (flag == 2) {

	} else {
		return -EINVAL;
	}
	return 0;

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
		cnt = (xfer->len%4)?(xfer->len/4 + 1):(xfer->len/4);
		for (i = 0; i < cnt; i++) {
			if (*((u32 *) xfer->rx_buf + i) != *((u32 *) xfer->tx_buf + i)) {
				SPIDEV_LOG("tx xfer %d is:%.8x\n", i, *((u32 *) xfer->tx_buf + i));
				SPIDEV_LOG("rx xfer %d is:%.8x\n", i, *((u32 *) xfer->rx_buf + i));
				SPIDEV_LOG("\n");
				err++;
			}
		}
		kfree(xfer->tx_buf);
		kfree(xfer->rx_buf);
	}

		SPIDEV_LOG("Message:0x%p,error %d,actual xfer length is:%d\n", msg, err, msg->actual_length);

	return err;
}

static ssize_t spi_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct spi_device *spi;

	struct mtk_chip_config *chip_config;

#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
	u32 spinum;
#endif

	spi = container_of(dev, struct spi_device, dev);

	SPIDEV_LOG("SPIDEV name is:%s\n", spi->modalias);

	chip_config = (struct mtk_chip_config *) spi->controller_data;

	if (!chip_config) {
		SPIDEV_LOG("chip_config is NULL.\n");
		chip_config = kzalloc(sizeof(struct mtk_chip_config), GFP_KERNEL);
		if (!chip_config)
			return -ENOMEM;
	}
	if (!buf) {
		SPIDEV_LOG("buf is NULL.\n");
		goto out;
	}
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
	if (!strncmp(buf, "send", 4) && (sscanf(buf + 4, "%d", &spinum) == 1)) { /*TRANSFER*/
		SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_enable_clk(spi);
		secspi_execute(1, NULL);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 1 finished!!!\n");
	} else if (!strncmp(buf, "config", 6) && (sscanf(buf + 6, "%d", &spinum) == 1)) { /*HW CONFIG*/
		SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_execute(2, NULL);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 2 finished!!!\n");
	} else if (!strncmp(buf, "debug", 5) && (sscanf(buf + 5, "%d", &spinum) == 1)) { /*DEBUG*/
		SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_execute(3, NULL);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 3 finished!!!\n");
	} else if (!strncmp(buf, "test", 4) && (sscanf(buf + 4, "%d", &spinum) == 1)) { /*TEST*/
		SPIDEV_MSG("start to access TL SPI driver.\n");
		secspi_session_open(spinum);
		secspi_execute(4, NULL);
		secspi_session_close();
		SPIDEV_MSG("secspi_execute 4 finished!!!\n");
#else
	if (!strncmp(buf, "-h", 2)) {
		SPIDEV_MSG("Please input the parameters for this device.\n");
#endif
	} else if (!strncmp(buf, "-w", 2)) {

	}
out:
	if (!spi->controller_data)
		kfree(chip_config);
	return count;
}

static ssize_t
spi_msg_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret = 0;
	struct spi_device *spi;

	struct spi_transfer transfer = {0,};
	struct spi_message msg;
	/*struct mtk_chip_config *chip_config;*/

	u32 len = 4;
	u32 tx_buffer = 0x12345678;
	u32 rx_buffer = 0xaaaaaaaa;

	transfer.tx_buf = &tx_buffer;
	transfer.rx_buf = &rx_buffer;
	transfer.len = 4;

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

	if (!strncmp(buf, "-h", 2)) {
		SPIDEV_MSG("Please input the message of this device to send and receive.\n");
	} else if (!strncmp(buf, "-func", 5)) {
		buf += 6;
		if (!strncmp(buf, "len=", 4) && 1 == sscanf(buf + 4, "%d", &len)) {
			spi_setup_xfer(&spi->dev, &transfer, len, 1);
			spi_message_add_tail(&transfer, &msg);
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
	} else{
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

	num = (int)ARRAY_SIZE(spi_attribute);
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

	num = (int)ARRAY_SIZE(spi_attribute);
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
	spi_test = spi;
	spi->mode = SPI_MODE_3;
	spi->bits_per_word = 8;
	return spi_create_attribute(&spi->dev);
	return 0;
}

struct spi_device_id spi_id_table[] = {
	{"spi-ut", 0},
	{},
};

static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "mediatek,spi-mt65xx-test" },
	{},
};
MODULE_DEVICE_TABLE(of, spidev_dt_ids);

static struct spi_driver spi_test_driver = {
	.driver = {
		.name = "test_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
		.of_match_table = spidev_dt_ids,
	},
	.probe = spi_test_probe,
	.remove = spi_test_remove,
	.id_table = spi_id_table,
};

static int __init spi_dev_init(void)
{
	SPIDEV_LOG("SPI_DEV_INIT.\n");
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
MODULE_AUTHOR("ZH Chen <zh.chen@mediatek.com>");
MODULE_LICENSE("GPL");
