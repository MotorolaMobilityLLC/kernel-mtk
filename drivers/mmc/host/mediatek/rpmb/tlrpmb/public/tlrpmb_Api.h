/*
 * Copyright (c) 2013 TRUSTONIC LIMITED
 * All rights reserved
 *
 * The present software is the confidential and proprietary information of
 * TRUSTONIC LIMITED. You shall not disclose the present software and shall
 * use it only in accordance with the terms of the license agreement you
 * entered into with TRUSTONIC LIMITED. This software may be subject to
 * export or import laws in certain countries.
 */

#ifndef TLRPMB_H_
#define TLRPMB_H_

#include "tci.h"

/*
 * Command ID's for communication Trustlet Connector -> Trustlet.
 */
#define CMD_RPMB_WRAP           3
#define CMD_RPMB_UNWRAP         4
#define CMD_RPMB_WRITE_DATA     5
#define CMD_RPMB_READ_DATA      6

/*
 * Termination codes
 */
#define EXIT_ERROR                  ((uint32_t)(-1))

/*
 * command message.
 *
 * @param len Length of the data to process.
 * @param data Data to processed (cleartext or ciphertext).
 */
typedef struct {
	tciCommandHeader_t  header;     /**< Command header */
	uint32_t            len;        /**< Length of data to process or buffer */
	uint32_t            respLen;    /**< Length of response buffer */
} cmd_t;

/*
 * Response structure Trustlet -> Trustlet Connector.
 */
typedef struct {
	tciResponseHeader_t header;     /**< Response header */
	uint32_t            len;
} rsp_t;

/*
 * TCI message data.
 */
typedef struct {
	union {
	  cmd_t     cmdrpmb;
	  rsp_t     rsprpmb;
	};
	uint32_t    Num1;
	uint32_t    Num2;
	uint32_t    Buf[512];
	uint32_t    BufSize; /* byte unit. */
	uint32_t    ResultData;

} tciMessage_t;

/*
 * Trustlet UUID.
 */
#define TL_RPMB_UUID { { 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }

#endif
