//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_spi.c

	Description : source of SPI interface

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>


#include "fci_types.h"
#include "fc8180_regs.h"
#include "fci_oal.h"

#ifdef FEATURE_MTK
#include <mt_spi.h>
#endif

#define SPI_BMODE           0x00
#define SPI_WMODE           0x10
#define SPI_LMODE           0x20
#define SPI_READ            0x40
#define SPI_WRITE           0x00
#define SPI_AINC            0x80
#define CHIPID              (0 << 3)

#define DRIVER_NAME "isdbt_spi"

struct spi_device *fc8180_spi;

static u8 tx_data[10];

#define FEATURE_QSD_BAM
#ifdef FEATURE_QSD_BAM
static u8 *wdata_buf;
static u8 *rdata_buf;
static u8 *rdata_buf_org;
#else
static u8 wdata_buf[32] __cacheline_aligned;
static u8 rdata_buf[8192] __cacheline_aligned;
#endif
static DEFINE_MUTEX(fci_spi_lock);
extern  int isdbt_power_init(struct spi_device *spi);
static int fc8180_spi_probe(struct spi_device *spi)
{
	s32 ret;
	struct mt_chip_conf *chip_config;
	print_log(0, "[FC8180] fc8180_spi_probe\n");
#ifdef FEATURE_MTK

	print_log(0, "fc8180_spi_probe\n");
	chip_config = (struct mt_chip_conf *) spi->controller_data;

	chip_config->setuptime = 3;
	chip_config->holdtime = 3;
	chip_config->high_time = 4;
	chip_config->low_time = 4;
	chip_config->cs_idletime = 2;
	chip_config->ulthgh_thrsh = 0;

	chip_config->cpol = 0;
	chip_config->cpha = 0;

	chip_config->rx_mlsb = 1;
	chip_config->tx_mlsb = 1;

	chip_config->tx_endian = 0;
	chip_config->rx_endian = 0;

	chip_config->com_mod = DMA_TRANSFER;
	chip_config->pause = 0;
	chip_config->finish_intr = 1;
	chip_config->deassert = 0;
	chip_config->ulthigh = 0;
	chip_config->tckdly = 2;

	spi->max_speed_hz = 12000000;
#else

	spi->max_speed_hz = 19200000;
#endif
	spi->bits_per_word = 8;
	spi->mode =  SPI_MODE_0;

	ret = spi_setup(spi);

	if (ret < 0)
		return ret;

	fc8180_spi = spi;
	isdbt_power_init(spi);
	return ret;
}


static int fc8180_spi_remove(struct spi_device *spi)
{

	return 0;
}

struct of_device_id fc8180_match_table[] = {
	{
		.compatible = "fci,isdbt",
	},
	{}
};

static struct spi_driver fc8180_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = fc8180_match_table,
	},
	.probe		= fc8180_spi_probe,
	.remove		= fc8180_spi_remove,
};
#ifdef FEATURE_MTK

static struct spi_board_info fc8180_spi_devs[] __initdata = {
	[0] = {
		.modalias="isdbt_spi",
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.max_speed_hz	 = 12000000,
	},
};
#endif
static int fc8180_spi_write_then_read(struct spi_device *spi
	, u8 *txbuf, u16 tx_length, u8 *rxbuf, u16 rx_length)
{
	int res = 0;

	struct spi_message	message;
	struct spi_transfer	x;
#ifdef FEATURE_MTK
	struct spi_transfer	t[2];
#endif

	if (spi == NULL) {
		print_log(0, "[FC8180] FC8180_SPI Handle Fail...........\n");
		return BBM_NOK;
	}

#ifdef FEATURE_MTK
	if ((tx_length + rx_length) >= 1024) {
		spi_message_init(&message);
		memset(&t, 0, sizeof t);
		spi_message_add_tail(&t[0], &message);
		spi_message_add_tail(&t[1], &message);
		memcpy(wdata_buf, txbuf, tx_length);

		t[0].tx_buf = wdata_buf;
		t[0].rx_buf = rdata_buf;
		t[0].len = tx_length + (rx_length % 1024);
		t[0].cs_change = 0;
		t[0].bits_per_word = 8;

		t[1].tx_buf = wdata_buf;
		t[1].rx_buf = &rdata_buf[t[0].len + (4-(t[0].len % 4))];
		t[1].len = rx_length - (rx_length % 1024);
		t[1].cs_change = 0;
		t[1].bits_per_word = 8;
		print_log(0, "[FC8180] DataRead F[0x%x], S[0x%x]\n", t[0].rx_buf, t[1].rx_buf);

		res = spi_sync(spi, &message);
		if ((rxbuf != NULL) && (rx_length > 0)) {
			memcpy(rxbuf, t[0].rx_buf + tx_length, (rx_length % 1024));
			memcpy(&rxbuf[rx_length % 1024], t[1].rx_buf, rx_length - (rx_length % 1024));
		}
	}else
#endif
{
	spi_message_init(&message);
	memset(&x, 0, sizeof x);

	spi_message_add_tail(&x, &message);

#ifdef FEATURE_QSD_BAM
	memcpy(wdata_buf, txbuf, tx_length);

	x.tx_buf = wdata_buf;
	x.rx_buf = rdata_buf;
#else
	memcpy(&wdata_buf[0], txbuf, tx_length);

	x.tx_buf = &wdata_buf[0];
	x.rx_buf = &rdata_buf[0];
#endif

	x.len = tx_length + rx_length;
	x.cs_change = 0;
	x.bits_per_word = 8;
	res = spi_sync(spi, &message);

	memcpy(rxbuf, x.rx_buf + tx_length, rx_length);
}
	return res;
}

static s32 spi_bulkread(HANDLE handle, u16 addr, u8 command, u8 *data,
							u32 length)
{
	int res;

	tx_data[0] = addr & 0xff;
	tx_data[1] = (addr >> 8) & 0xff;
	tx_data[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	tx_data[3] = (length >> 8) & 0xff;
	tx_data[4] = length & 0xff;

	res = fc8180_spi_write_then_read(fc8180_spi
		, &tx_data[0], 5, data, length);

	if (res) {
		print_log(0, "[FC8180] fc8180_spi_bulkread fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}

static s32 spi_bulkwrite(HANDLE handle, u16 addr, u8 command, u8 *data,
							u32 length)
{
	int i;
	int res;

	tx_data[0] = addr & 0xff;
	tx_data[1] = (addr >> 8) & 0xff;
	tx_data[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	tx_data[3] = (length >> 8) & 0xff;
	tx_data[4] = length & 0xff;

	for (i = 0; i < length; i++)
		tx_data[5 + i] = data[i];

	res = fc8180_spi_write_then_read(fc8180_spi
		, &tx_data[0], length + 5, data, 0);

	if (res) {
		print_log(0, "[FC8180] fc8180_spi_bulkwrite fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}


static s32 spi_dataread(HANDLE handle, u16 addr, u8 command, u8 *data,
							u32 length)
{
	int res;

	tx_data[0] = addr & 0xff;
	tx_data[1] = (addr >> 8) & 0xff;
	tx_data[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	tx_data[3] = (length >> 8) & 0xff;
	tx_data[4] = length & 0xff;

	res = fc8180_spi_write_then_read(fc8180_spi
		, &tx_data[0], 5, data, length);

	if (res) {
		print_log(0, "[FC8180] fc8180_spi_dataread fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}

s32 fc8180_spi_init(HANDLE handle, u16 param1, u16 param2)
{
	int res = 0;
	print_log(0, "fc8180_spi_init : %d\n", res);
#ifdef FEATURE_MTK
	spi_register_board_info(fc8180_spi_devs, ARRAY_SIZE(fc8180_spi_devs));
#endif
#ifdef FEATURE_QSD_BAM
	if (wdata_buf == NULL) {
		wdata_buf = kmalloc(128, GFP_DMA | GFP_KERNEL);

		if (!wdata_buf) {
			print_log(0, "spi wdata_buf kmalloc fail \n");
			return BBM_NOK;
		}
	}

	if (rdata_buf == NULL) {
		rdata_buf = kmalloc(8 * 1024, GFP_DMA | GFP_KERNEL);
		if ((ulong)rdata_buf & 0x03) {
			rdata_buf_org = rdata_buf;
			rdata_buf = (u8 *)(((ulong)rdata_buf_org + 0x04) & 0x04);
		}
		if (!rdata_buf) {
			print_log(0, "spi rdata_buf kmalloc fail \n");
			return BBM_NOK;
		}
	}
#endif
	res = spi_register_driver(&fc8180_spi_driver);

	if (res) {
		print_log(0, "fc8180_spi register fail : %d\n", res);
		return BBM_NOK;
	}

	return res;
}

s32 fc8180_spi_byteread(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;
	u8 command = SPI_READ;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, addr, command, data, 1);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_wordread(HANDLE handle, u16 addr, u16 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, addr, command, (u8 *) data, 2);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_longread(HANDLE handle, u16 addr, u32 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, addr, command, (u8 *) data, 4);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, addr, command, data, length);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_bytewrite(HANDLE handle, u16 addr, u8 data)
{
	s32 res;
	u8 command = SPI_WRITE;

	mutex_lock(&fci_spi_lock);

#ifdef BBM_SPI_IF
	if (addr == BBM_DM_DATA) {
#ifdef BBM_SPI_PHA_1
		u8 ifcommand = 0xff;
		u16 ifaddr = 0xffff;
		u8 ifdata = 0xff;
#else
		u8 ifcommand = 0x00;
		u16 ifaddr = 0x0000;
		u8 ifdata = 0x00;
#endif
		res = spi_bulkwrite(handle, ifaddr, ifcommand, &ifdata, 1);
	} else
#endif
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 1);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_wordwrite(HANDLE handle, u16 addr, u16 data)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 2);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_longwrite(HANDLE handle, u16 addr, u32 data)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 4);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkwrite(handle, addr, command, data, length);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8180_spi_dataread(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	s32 res;
	u8 command = SPI_READ;

	mutex_lock(&fci_spi_lock);
	res = spi_dataread(handle, addr, command, data, length);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8180_spi_deinit(HANDLE handle)
{
	spi_unregister_driver(&fc8180_spi_driver);
	return BBM_OK;
}
//add dtv ---shenyong@wind-mobi.com ---20161119 end 