/*
 * Copyright (C) 2017 MediaTek Inc.
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


#ifndef __DRSPIAPI_H__
#define __DRSPIAPI_H__

#include "dci.h"

/*
 * Command ID's
 */
#define	CMD_SPI_DRV_SEND		1
#define	CMD_SPI_DRV_CONFIG		2
#define	CMD_SPI_DRV_DEBUG		3
#define	CMD_SPI_DRV_TEST		4
#define	DCI_SPI_CMD_SET_DVFS	5
#define	DCI_SPI_CMD_RELEASE_DVFS	6

/*... add more command ids when needed */

/**
 * command message.
 *
 * @param len Length of the data to process.
 * @param data Data to be processed
 */
#define EXIT_ERROR			((uint32_t)(-1))

typedef struct {
	dciCommandHeader_t	header;		/**< Command header */
	uint32_t		len;		/**< Length of data to process */
} dr_cmd_t;

/**
 * Response structure
 */
typedef struct {
	dciResponseHeader_t header;	/**< Response header */
	uint32_t	len;
} dr_rsp_t;

/*
 * DCI message data.
 */
typedef struct {
	union {
	dr_cmd_t	cmd_spi;
	dr_rsp_t	rsp_spi;
	};
} dciMessage_t;

/*
 * Driver UUID. Update accordingly after reserving UUID
 */
/*#define DRV_DBG_UUID { { 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }*/

#define	DRV_DBG_UUID	{ 0x03, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

#endif /*__DRFOOAPI_H__*/
