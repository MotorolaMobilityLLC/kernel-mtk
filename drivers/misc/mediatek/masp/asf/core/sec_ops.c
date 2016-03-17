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

#include "sec_osal.h"
#include "sec_hal.h"
#include "sec_osal_light.h"
#include "sec_typedef.h"
#include "alg_sha1.h"
#include "sec_log.h"

static int sec_get_rid(unsigned int *rid)
{
	unsigned int obuf[5];
	unsigned int ibuf[4];

	masp_hal_get_uuid(ibuf);

	sha1((unsigned char *) ibuf, 16, (unsigned char *) obuf);

	memcpy(rid, obuf, 16);

#ifdef SEC_DEBUG
	{
		int i = 0;

		for (i = 0; i < 4; i++) {
			SMSG(TRUE, "IBUF[%d] = 0x%.8x\n", i, ibuf[i]);
			SMSG(TRUE, "OBUF[%d] = 0x%.8x\n", i, obuf[i]);
		}
	}
#endif

	return 0;
}


/**************************************************************************
 *  SEC RANDOM ID FUNCTION
 **************************************************************************/
int sec_get_random_id(unsigned int *rid)
{
	int ret;

	osal_rid_lock();
	ret = sec_get_rid(rid);
	osal_rid_unlock();
	return ret;
}
