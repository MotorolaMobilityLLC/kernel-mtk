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

#ifndef SEC_SIGN_FORMAT_UTIL_H
#define SEC_SIGN_FORMAT_UTIL_H

#include "sec_sign_header.h"
#include "sec_sign_extension.h"
#include "sec_log.h"
#include "sec_osal_light.h"


/******************************************************************************
 *  EXPORT FUNCTION
 ******************************************************************************/
unsigned int get_hash_size(SEC_CRYPTO_HASH_TYPE hash);
unsigned int get_signature_size(SEC_CRYPTO_SIGNATURE_TYPE sig);
unsigned char is_signfmt_v1(SEC_IMG_HEADER *hdr);
unsigned char is_signfmt_v2(SEC_IMG_HEADER *hdr);
unsigned char is_signfmt_v3(SEC_IMG_HEADER *hdr);
unsigned char is_signfmt_v4(SEC_IMG_HEADER *hdr);

#endif
