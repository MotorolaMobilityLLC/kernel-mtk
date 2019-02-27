/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef TEE_TBASE_LEGACY_DEF_H_
#define TEE_TBASE_LEGACY_DEF_H_

#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)

struct tciCommandHeader_t {
	uint32_t commandId;
};

struct tciResponseHeader_t {
	uint32_t responseId;
	uint32_t returnCode;
};

struct tl_cmd_t {
	struct tciCommandHeader_t header;
	uint32_t len;
	uint32_t respLen;
};

struct tl_rsp_t {
	struct tciResponseHeader_t header;
	uint32_t len;
};

#define MAX_NAME_SZ (32)
struct tl_sender_info_t {
	uint8_t name[MAX_NAME_SZ];
	uint32_t id;
};

struct secmem_tci_msg_t {
	union {
		struct tl_cmd_t cmd_secmem;
		struct tl_rsp_t rsp_secmem;
	};

	uint64_t alignment;  /* IN */
	uint64_t size;       /* IN */
	uint32_t refcount;   /* INOUT */
	uint64_t sec_handle; /* OUT */

	uint32_t ResultData;
	struct tl_sender_info_t sender; /* debug purpose */
};

#endif /* TEE_TBASE_LEGACY_DEF_H_ */
