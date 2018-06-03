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

#ifndef TCI_H_
#define TCI_H_

typedef uint32_t tciCommandId_t;
typedef uint32_t tciResponseId_t;
typedef uint32_t tciReturnCode_t;

/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)

/**
 * Return codes of Trustlet commands.
 */
#define RET_OK              0            /**< Set, if processing is error free */
#define RET_ERR_UNKNOWN_CMD 1            /**< Unknown command */
#define INVALID_VIRTUAL_ADDR 2

/**
 * TCI command header.
 */
struct tciCommandHeader_t {
	tciCommandId_t commandId; /**< Command ID */
};

/**
 * TCI response header.
 */
struct tciResponseHeader_t {
	tciResponseId_t     responseId; /**< Response ID (must be command ID | RSP_ID_MASK )*/
	tciReturnCode_t     returnCode; /**< Return code of command */
};

#endif /* TCI_H_ */
