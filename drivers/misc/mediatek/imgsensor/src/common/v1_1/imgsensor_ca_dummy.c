/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include "imgsensor_ca.h"
#include "tee_client_api.h"
#include "tee_client_api_cust.h"

unsigned int imgsensor_ca_open(void)
{
	return TEEC_SUCCESS;
}

void imgsensor_ca_close(void)
{
}

void imgsensor_ca_release(void)
{
}

unsigned int  imgsensor_ca_invoke_command(
	enum IMGSENSOR_TEE_CMD cmd, struct command_params parms, MUINT32 *ret)
{
	return TEEC_SUCCESS;
}

