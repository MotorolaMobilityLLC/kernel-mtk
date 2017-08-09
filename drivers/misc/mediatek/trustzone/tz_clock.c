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


#include <linux/module.h>
#include <linux/types.h>

#include "tz_cross/trustzone.h"
#include "trustzone/kree/system.h"
#include "kree_int.h"

TZ_RESULT KREE_ServEnableClock(u32 op, u8 uparam[REE_SERVICE_BUFFER_SIZE])
{
/*	struct ree_service_clock *param = (struct ree_service_clock *)uparam; */
	TZ_RESULT ret = TZ_RESULT_ERROR_GENERIC;
/*	int rret; */

/*	rret = enable_clock(param->clk_id, param->clk_name); */
/*	if (rret < 0) */
/*		ret = TZ_RESULT_ERROR_GENERIC; */

	return ret;
}

TZ_RESULT KREE_ServDisableClock(u32 op, u8 uparam[REE_SERVICE_BUFFER_SIZE])
{
/*	struct ree_service_clock *param = (struct ree_service_clock *)uparam; */
	TZ_RESULT ret = TZ_RESULT_ERROR_GENERIC;
/*	int rret; */

/*	rret = disable_clock(param->clk_id, param->clk_name); */
/*	if (rret < 0) */
/*		ret = TZ_RESULT_ERROR_GENERIC; */

	return ret;
}
