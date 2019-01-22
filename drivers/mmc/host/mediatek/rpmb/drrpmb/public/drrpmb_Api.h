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

/**
 * @file   drrpmb_Api.h
 * @brief  Contains DCI command definitions and data structures
 *
 */

#ifndef __DRRPMBAPI_H__
#define __DRRPMBAPI_H__

#include "dci.h"


/*
 * Command ID's
 */
#define DCI_RPMB_CMD_READ_DATA      1
#define DCI_RPMB_CMD_GET_WCNT       2
#define DCI_RPMB_CMD_WRITE_DATA     3


/*... add more command ids when needed */

/**
 * command message.
 *
 * @param len Length of the data to process.
 * @param data Data to be processed
 */
typedef struct {
	dciCommandHeader_t  header;     /**< Command header */
	uint32_t            len;        /**< Length of data to process */
} cmd_t;

/**
 * Response structure
 */
typedef struct {
	dciResponseHeader_t header;     /**< Response header */
	uint32_t            len;
} rsp_t;


/*
 * Alternative access flow to improve performance. (this is customization)
 */
#define RPMB_MULTI_BLOCK_ACCESS 1

#if RPMB_MULTI_BLOCK_ACCESS
#define MAX_RPMB_TRANSFER_BLK (16)
#define MAX_RPMB_REQUEST_SIZE (512*MAX_RPMB_TRANSFER_BLK) /* 8KB(16blks) per requests. */
#else
#define MAX_RPMB_TRANSFER_BLK (1)
#define MAX_RPMB_REQUEST_SIZE (512*MAX_RPMB_TRANSFER_BLK) /* 512B(1blks) per requests. */
#endif

typedef struct {
	uint8_t frame[MAX_RPMB_REQUEST_SIZE];
	uint32_t frameLen;
	uint16_t type;
	uint16_t addr;
	uint16_t blks;
	uint16_t result;
} rpmb_req_t;

/*
 * DCI message data.
 */
typedef struct {
	union {
		cmd_t     command;
		rsp_t     response;
	};

	rpmb_req_t    request;

} dciMessage_t;

/*
 * Driver UUID. Update accordingly after reserving UUID
 */
#define RPMB_UUID { { 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }

#endif
