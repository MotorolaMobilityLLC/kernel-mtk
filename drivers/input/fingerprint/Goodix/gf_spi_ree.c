/* Goodix's GF516/GF318/GF516M/GF318M/GF518M/GF3118M/GF5118M
 *  fingerprint sensor linux driver for REE and factory mode
 *
 * 2010 - 2015 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include "gf_spi_tee.h"
#include "gf_spi_ree.h"

/* -------------------------------------------------------------------- */
/* normal world SPI read/write function                 */
/* -------------------------------------------------------------------- */

/* gf_spi_setup_conf_ree, configure spi speed and transfer mode in REE mode
  *
  * speed: 1, 4, 6, 8 unit:MHz
  * mode: DMA mode or FIFO mode
  */
void gf_spi_setup_conf_ree(u32 speed, enum spi_transfer_mode mode)
{
	struct mt_chip_conf *mcc = &g_gf_dev->spi_mcc;

	switch (speed) {
	case 1:
		/* set to 1MHz clock */
		mcc->high_time = 50;
		mcc->low_time = 50;
		break;
	case 4:
		/* set to 4MHz clock */
		mcc->high_time = 15;
		mcc->low_time = 15;
		break;
	case 6:
		/* set to 6MHz clock */
		mcc->high_time = 10;
		mcc->low_time = 10;
		break;
	case 8:
		/* set to 8MHz clock */
		mcc->high_time = 8;
		mcc->low_time = 8;
		break;
	default:
		/* default set to 1MHz clock */
		mcc->high_time = 50;
		mcc->low_time = 50;
	}

	if ((mode == DMA_TRANSFER) || (mode == FIFO_TRANSFER)) {
		mcc->com_mod = mode;
	} else {
		/* default set to FIFO mode */
		mcc->com_mod = FIFO_TRANSFER;
	}

	if (spi_setup(g_gf_dev->spi))
		gf_debug(ERR_LOG, "%s, failed to setup spi conf\n", __func__);

}

int gf_spi_read_bytes_ree(u16 addr, u32 data_len, u8 *rx_buf)
{
	struct spi_message msg;
	struct spi_transfer *xfer;
	u8 *tmp_buf;
	u32 package, reminder, retry;

	if (g_gf_dev->spi_ree_enable) {
		package = (data_len + 2) / 1024;
		reminder = (data_len + 2) % 1024;

		if ((package > 0) && (reminder != 0)) {
			xfer = kzalloc(sizeof(*xfer)*4, GFP_KERNEL);
			retry = 1;
		} else {
			xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
			retry = 0;
		}
		if (xfer == NULL) {
			gf_debug(ERR_LOG, "%s, no memory for SPI transfer\n", __func__);
			return -ENOMEM;
		}

		tmp_buf = g_gf_dev->spi_buffer;

		/* switch to DMA mode if transfer length larger than 32 bytes */
		if ((data_len + 1) > 32)
			g_gf_dev->spi_mcc.com_mod = DMA_TRANSFER;

		spi_setup(g_gf_dev->spi);

		spi_message_init(&msg);
		*tmp_buf = 0xF0;
		*(tmp_buf + 1) = (u8)((addr>>8)&0xFF);
		*(tmp_buf + 2) = (u8)(addr&0xFF);
		xfer[0].tx_buf = tmp_buf;
		xfer[0].len = 3;
		xfer[0].delay_usecs = 5;
		spi_message_add_tail(&xfer[0], &msg);
		spi_sync(g_gf_dev->spi, &msg);

		spi_message_init(&msg);
		/* memset((tmp_buf + 4), 0x00, data_len + 1); */
		/* 4 bytes align */
		*(tmp_buf + 4) = 0xF1;
		xfer[1].tx_buf = tmp_buf + 4;
		xfer[1].rx_buf = tmp_buf + 4;
		if (retry)
			xfer[1].len = package * 1024;
		else
			xfer[1].len = data_len + 1;

		xfer[1].delay_usecs = 5;
		spi_message_add_tail(&xfer[1], &msg);
		spi_sync(g_gf_dev->spi, &msg);

		/* copy received data */
		if (retry)
			memcpy(rx_buf, (tmp_buf + 5), (package * 1024-1));
		else
			memcpy(rx_buf, (tmp_buf + 5), data_len);

		/* send reminder SPI data */
		if (retry) {
			addr = addr + package * 1024 - 2;
			spi_message_init(&msg);
			*tmp_buf = 0xF0;
			*(tmp_buf + 1) = (u8)((addr>>8)&0xFF);
			*(tmp_buf + 2) = (u8)(addr&0xFF);
			xfer[2].tx_buf = tmp_buf;
			xfer[2].len = 3;
			xfer[2].delay_usecs = 5;
			spi_message_add_tail(&xfer[2], &msg);
			spi_sync(g_gf_dev->spi, &msg);

			spi_message_init(&msg);
			*(tmp_buf + 4) = 0xF1;
			xfer[3].tx_buf = tmp_buf + 4;
			xfer[3].rx_buf = tmp_buf + 4;
			xfer[3].len = reminder + 1;
			xfer[3].delay_usecs = 5;
			spi_message_add_tail(&xfer[3], &msg);
			spi_sync(g_gf_dev->spi, &msg);

			memcpy((rx_buf + package * 1024 - 1), (tmp_buf + 6), (reminder - 1));
		}

		/* restore to FIFO mode if has used DMA */
		if ((data_len + 1) > 32)
			g_gf_dev->spi_mcc.com_mod = FIFO_TRANSFER;

		spi_setup(g_gf_dev->spi);

		kfree(xfer);
		if (xfer != NULL)
			xfer = NULL;

	} else {
		gf_debug(ERR_LOG, "%s, ree spi has been disabled\n", __func__);
	}
	return 0;
}

int gf_spi_write_bytes_ree(u16 addr, u32 data_len, u8 *tx_buf)
{
	struct spi_message msg;
	struct spi_transfer *xfer;
	u8 *tmp_buf;
	u32 package, reminder, retry;

	if (g_gf_dev->spi_ree_enable) {
		package = (data_len + 3) / 1024;
		reminder = (data_len + 3) % 1024;

		if ((package > 0) && (reminder != 0)) {
			xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
			retry = 1;
		} else {
			xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
			retry = 0;
		}
		if (xfer == NULL) {
			gf_debug(ERR_LOG, "%s, no memory for SPI transfer\n", __func__);
			return -ENOMEM;
		}
		tmp_buf = g_gf_dev->spi_buffer;

		/* switch to DMA mode if transfer length larger than 32 bytes */
		if ((data_len + 3) > 32)
			g_gf_dev->spi_mcc.com_mod = DMA_TRANSFER;

		spi_setup(g_gf_dev->spi);

		spi_message_init(&msg);
		*tmp_buf = 0xF0;
		*(tmp_buf + 1) = (u8)((addr>>8)&0xFF);
		*(tmp_buf + 2) = (u8)(addr&0xFF);
		if (retry) {
			memcpy(tmp_buf + 3, tx_buf, (package * 1024 - 3));
			xfer[0].len = package*1024;
		} else {
			memcpy(tmp_buf + 3, tx_buf, data_len);
			xfer[0].len = data_len + 3;
		}
		xfer[0].tx_buf = tmp_buf;
		xfer[0].delay_usecs = 5;
		spi_message_add_tail(&xfer[0], &msg);
		spi_sync(g_gf_dev->spi, &msg);

		if (retry) {
			addr = addr + package*1024 - 3;
			spi_message_init(&msg);
			*tmp_buf = 0xF0;
			*(tmp_buf + 1) = (u8)((addr>>8)&0xFF);
			*(tmp_buf + 2) = (u8)(addr&0xFF);
			memcpy(tmp_buf + 3, (tx_buf + package * 1024 - 3), reminder);
			xfer[1].tx_buf = tmp_buf;
			xfer[1].len = reminder + 3;
			xfer[1].delay_usecs = 5;
			spi_message_add_tail(&xfer[1], &msg);
			spi_sync(g_gf_dev->spi, &msg);
		}

		/* restore to FIFO mode if has used DMA */
		if ((data_len + 3) > 32)
			g_gf_dev->spi_mcc.com_mod = FIFO_TRANSFER;

		spi_setup(g_gf_dev->spi);

		kfree(xfer);
		if (xfer != NULL)
			xfer = NULL;

	} else {
		gf_debug(ERR_LOG, "%s, ree spi has been disabled\n", __func__);
	}

	return 0;
}

int gf_spi_read_byte_ree(u16 addr, u8 *value)
{
	struct spi_message msg;
	struct spi_transfer *xfer;

	if (g_gf_dev->spi_ree_enable) {
		xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
		if (xfer == NULL) {
			gf_debug(ERR_LOG, "%s, no memory for SPI transfer\n", __func__);
			return -ENOMEM;
		}

		spi_message_init(&msg);
		*g_gf_dev->spi_buffer = 0xF0;
		*(g_gf_dev->spi_buffer + 1) = (u8)((addr>>8)&0xFF);
		*(g_gf_dev->spi_buffer + 2) = (u8)(addr&0xFF);

		xfer[0].tx_buf = g_gf_dev->spi_buffer;
		xfer[0].len = 3;
		xfer[0].delay_usecs = 5;
		spi_message_add_tail(&xfer[0], &msg);
		spi_sync(g_gf_dev->spi, &msg);

		spi_message_init(&msg);
		/* 4 bytes align */
		*(g_gf_dev->spi_buffer + 4) = 0xF1;
		xfer[1].tx_buf = g_gf_dev->spi_buffer + 4;
		xfer[1].rx_buf = g_gf_dev->spi_buffer + 4;
		xfer[1].len = 2;
		xfer[1].delay_usecs = 5;
		spi_message_add_tail(&xfer[1], &msg);
		spi_sync(g_gf_dev->spi, &msg);

		*value = *(g_gf_dev->spi_buffer + 5);

		kfree(xfer);
		if (xfer != NULL)
			xfer = NULL;

	} else {
		gf_debug(ERR_LOG, "%s, ree spi has been disabled\n", __func__);
	}
	return 0;
}


int gf_spi_write_byte_ree(u16 addr, u8 value)
{
	struct spi_message msg;
	struct spi_transfer *xfer;

	if (g_gf_dev->spi_ree_enable) {
		xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
		if (xfer == NULL) {
			gf_debug(ERR_LOG, "%s, no memory for SPI transfer\n", __func__);
			return -ENOMEM;
		}

		spi_message_init(&msg);
		*g_gf_dev->spi_buffer = 0xF0;
		*(g_gf_dev->spi_buffer + 1) = (u8)((addr>>8)&0xFF);
		*(g_gf_dev->spi_buffer + 2) = (u8)(addr&0xFF);
		*(g_gf_dev->spi_buffer + 3) = value;

		xfer[0].tx_buf = g_gf_dev->spi_buffer;
		xfer[0].len = 3 + 1;
		xfer[0].delay_usecs = 5;
		spi_message_add_tail(&xfer[0], &msg);
		spi_sync(g_gf_dev->spi, &msg);

		kfree(xfer);
		if (xfer != NULL)
			xfer = NULL;

	} else {
		gf_debug(ERR_LOG, "%s, ree spi has been disabled\n", __func__);
	}
	return 0;
}
