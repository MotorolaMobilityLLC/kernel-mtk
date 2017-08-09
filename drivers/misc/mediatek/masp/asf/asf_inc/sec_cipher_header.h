/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef _CIPHERHEADER_H
#define _CIPHERHEADER_H

/**************************************************************************
 *  CIPHER HEADER FORMAT
 **************************************************************************/

#define CUSTOM_NAME                  "CUSTOM_NAME"
#define IMAGE_VERSION                "IMAGE_VERSION"
#define CIPHER_IMG_MAGIC             (0x63636363)
#define CIPHER_IMG_HEADER_SIZE       (128)

typedef struct _SEC_CIPHER_IMG_HEADER {
	unsigned int magic_number;

	unsigned char cust_name[32];
	unsigned int image_version;
	unsigned int image_length;
	unsigned int image_offset;

	unsigned int cipher_offset;
	unsigned int cipher_length;

	/*
	 * v0 : legacy
	 * v1 : standard operations
	 */
	unsigned int aes_version;
	unsigned char dummy[68];

} CIPHER_HEADER;

#endif	 /*_CIPHERHEADER_H*/
