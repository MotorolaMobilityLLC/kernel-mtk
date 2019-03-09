/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __SPI_H
#define __SPI_H

#define SPI_WRITE 		0x82
#define SPI_READ 		0x83
#define SPI_CLK_HZ		(10 * M)
#define SPI_RETRY		5
#define DMA_TRANSFER_MAX_TIMES 2
#define DMA_TRANSFER_MAX_SIZE 1024
#define SPI_WRITE_BUFF_MAXSIZE (1024 * DMA_TRANSFER_MAX_TIMES + 5)//plus 5 for IC Mode :(Head + Address) 0x82,0x25,Addr_L,Addr_M,Addr_H
#define SPI_READ_BUFF_MAXSIZE  (1024 * DMA_TRANSFER_MAX_TIMES)

struct core_spi_data {
	struct spi_device *spi;
	int (*spi_write_then_read)(struct spi_device *spi,
		const void *txbuf, unsigned n_tx,
		void *rxbuf, unsigned n_rx);
};

extern struct core_spi_data *core_spi;

extern void core_spi_speed_up(bool Enable);
extern int core_spi_write(uint8_t *pBuf, uint16_t nSize);
extern int core_spi_read(uint8_t *pBuf, uint16_t nSize);
extern int core_spi_init(struct spi_device *spi);
extern void core_spi_remove(void);

#endif
