/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                     *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  *
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *
******************************************************************************/

/**************************************************************
This file defines the driver FIPS functions that should be
implemented by the driver user. Current implementation is sample code only.
***************************************************************/

#include <linux/module.h>
#include "ssi_fips_local.h"
#include "ssi_driver.h"

#ifdef TRUSTONIC_FIPS_SUPPORT
#include <mobicore_driver_api.h>
#include <linux/delay.h>
#include "ssi_fips_ext.h"
#include "trustonic_tee/tlFips_Api.h"

static CC_FipsError_t tee_error = CC_TEE_FIPS_ERROR_GENERAL;
static const uint32_t MC_DEVICE_ID = MC_DEVICE_ID_DEFAULT;
static const struct mc_uuid_t uuid = TL_FIPS_UUID;
static struct mc_session_handle session_handle;
static tciMessage_t *tciMessage;
#else
static bool tee_error;
module_param(tee_error, bool, 0644);
MODULE_PARM_DESC(tee_error, "Simulate TEE library failure flag: 0 - no error (default), 1 - TEE error occured ");
#endif

static ssi_fips_state_t fips_state = CC_FIPS_STATE_NOT_SUPPORTED;
static ssi_fips_error_t fips_error = CC_REE_FIPS_ERROR_OK;

extern int ssi_fips_set_error(ssi_fips_error_t err);

/*
This function returns the FIPS REE state.
The function should be implemented by the driver user, depends on where
the state value is stored.
The reference code uses global variable.
*/
int ssi_fips_ext_get_state(ssi_fips_state_t *p_state)
{
        int rc = 0;

	if (p_state == NULL) {
		return -EINVAL;
	}

	*p_state = fips_state;

	return rc;
}

/*
This function returns the FIPS REE error.
The function should be implemented by the driver user, depends on where
the error value is stored.
The reference code uses global variable.
*/
int ssi_fips_ext_get_error(ssi_fips_error_t *p_err)
{
        int rc = 0;

	if (p_err == NULL) {
		return -EINVAL;
	}

	*p_err = fips_error;

	return rc;
}

/*
This function sets the FIPS REE state.
The function should be implemented by the driver user, depends on where
the state value is stored.
The reference code uses global variable.
*/
int ssi_fips_ext_set_state(ssi_fips_state_t state)
{
	fips_state = state;
	return 0;
}

/*
This function sets the FIPS REE status (specific error or success).
The function should be implemented by the driver user, depends on where
the error value is stored.
The reference code uses global variable.
*/
int ssi_fips_ext_set_error(ssi_fips_error_t err)
{
	fips_error = err;
	return 0;
}

#ifdef TRUSTONIC_FIPS_SUPPORT
static uint32_t tlc_get_cmd_param_len(tciCommandId_t cmd_id)
{
	uint32_t len = 0;

	switch (cmd_id) {
	case CMD_CRYS_FIPS_GET_TEE_STATUS:
		len = sizeof(TL_PARAM_CRYS_FIPS_GET_TEE_STATUS);
		break;
	case CMD_CRYS_FIPS_SET_REE_STATUS:
		len = sizeof(TL_PARAM_CRYS_FIPS_SET_REE_STATUS);
		break;
	default:
		len = 0;
		break;
	}

	return len;
}

static uint32_t tlcExecuteCmd(tciCommandId_t cmd_id)
{
	uint32_t ret = 0;
	enum mc_result result = MC_DRV_OK;

	do {
		/* Prepare command message in TCI. */
		tciMessage->cmd.header.commandId = cmd_id;
		tciMessage->cmd.param_len = tlc_get_cmd_param_len(cmd_id);
		tciMessage->cmd.data_len = 0;

		/* Notify the TA. */
		result = mc_notify(&session_handle);
		if (result != MC_DRV_OK) {
			SSI_LOG_ERR("Notify failed: %u\n", result);
			ret = TLC_FIPS_NOTIFY_TA_FAIL;
			break;
		}

		/* Wait for the TA response, set timeout as INFINITE. */
		result = mc_wait_notification(&session_handle, MC_INFINITE_TIMEOUT);
		if (result != MC_DRV_OK) {
			SSI_LOG_ERR("Wait for response notification failed: %u\n", result);
			ret = TLC_FIPS_WAIT_TA_FAIL;
			break;
		}

		/* Verify that the TA sent a response. */
		if (tciMessage->rsp_hdr.responseId != RSP_ID(cmd_id)) {
			SSI_LOG_ERR("TA did not send a response: %u\n", tciMessage->rsp_hdr.responseId);
			ret = TLC_FIPS_UNEXPECTED_REP;
			break;
		}

		/* Check the TA return code. */
		if (tciMessage->rsp_hdr.returnCode != RET_OK) {
			SSI_LOG_ERR("TA did not send a valid return code: %u\n", tciMessage->rsp_hdr.returnCode);
			ret = TLC_FIPS_RETURN_ERR;
			break;
		}
	} while (false);

	return ret;
}
#endif

/*
This function should get the FIPS TEE library error.
It is used before REE library starts running it own KAT tests.
The function should be implemented by the driver user.
The reference code checks the tee_error value provided in module prob.
*/
enum ssi_fips_error ssi_fips_ext_get_tee_error(void)
{
#ifdef TRUSTONIC_FIPS_SUPPORT
	uint32_t ret = 0, retry = 0;
	enum mc_result result = MC_DRV_OK;

	result = mc_open_device(MC_DEVICE_ID);
	if (result != MC_DRV_OK) {
		SSI_LOG_ERR("Error opening device: %u\n", result);
		goto exit0;
	}

	tciMessage = kmalloc(sizeof(tciMessage_t), GFP_KERNEL);
	if (tciMessage == NULL) {
		SSI_LOG_ERR("Allocation of TCI buffer failed\n");
		goto exit1;
	}

	for (retry = 0; retry < FIPS_RETRY_LIMIT; ++retry) {
		if (tee_error == CC_TEE_FIPS_ERROR_GENERAL) {
			SSI_LOG_DEBUG("before sleep (%u)\n", retry);
			msleep(FIPS_RETRY_DELAY);
			SSI_LOG_DEBUG("after sleep (%u)\n", retry);
		}

		memset(tciMessage, 0, sizeof(tciMessage_t));
		memset(&session_handle, 0, sizeof(struct mc_session_handle));
		session_handle.device_id = MC_DEVICE_ID;

		result = mc_open_session(&session_handle, &uuid, (u8 *)tciMessage, sizeof(tciMessage_t));
		SSI_LOG_DEBUG("after mc_open_session (%u)\n", retry);
		if (result == MC_DRV_OK)
			break;

		SSI_LOG_DEBUG("Open session failed(%u): %u\n", retry, result);
		if (tee_error != CC_TEE_FIPS_ERROR_GENERAL) {
			SSI_LOG_DEBUG("before sleep (%u)\n", retry);
			msleep(FIPS_RETRY_RUNTIME_DELAY);
			SSI_LOG_DEBUG("after sleep (%u)\n", retry);
		}
	}
	if (retry == FIPS_RETRY_LIMIT) {
		SSI_LOG_ERR("Open session failed: %u\n", result);
		goto exit2;
	}

	ret = tlcExecuteCmd(CMD_CRYS_FIPS_GET_TEE_STATUS);
	if (ret != TLC_FIPS_OK) {
		SSI_LOG_ERR("ExecuteCMD(%d) failed: %u\n", CMD_CRYS_FIPS_GET_TEE_STATUS, ret);
		goto exit3;
	}

	ret = tciMessage->param_fips_get_tee_status.ret;
	if (ret != SASI_OK) {
		SSI_LOG_ERR("Get TEE FIPS status failed: %u\n", ret);
		goto exit3;
	}

	tee_error = tciMessage->param_fips_get_tee_status.tee_status;
exit3:
	result = mc_close_session(&session_handle);
	if (result != MC_DRV_OK)
		SSI_LOG_ERR("Close session failed: %u\n", result);
exit2:
	kfree(tciMessage);
exit1:
	result = mc_close_device(MC_DEVICE_ID);
	if (result != MC_DRV_OK)
		SSI_LOG_ERR("Error closing device: %u\n", result);
exit0:
	return (tee_error == CC_TEE_FIPS_ERROR_OK ? CC_REE_FIPS_ERROR_OK : CC_REE_FIPS_ERROR_FROM_TEE);
#else
	return (tee_error ? CC_REE_FIPS_ERROR_FROM_TEE : CC_REE_FIPS_ERROR_OK);
#endif
}

/*
 This function should push the FIPS REE library status towards the TEE library.
 FIPS error can occur while running KAT tests at library init,
 or be OK in case all KAT tests had passed successfully.
 The function should be implemented by the driver user.
 The reference code prints as an indication it was called.
*/
void ssi_fips_ext_update_tee_upon_ree_status(void)
{
#ifdef TRUSTONIC_FIPS_SUPPORT
	uint32_t ret = 0, retry = 0;
	enum mc_result result = MC_DRV_OK;

	result = mc_open_device(MC_DEVICE_ID);
	if (result != MC_DRV_OK) {
		SSI_LOG_ERR("Error opening device: %u\n", result);
		goto exit0;
	}

	tciMessage = kmalloc(sizeof(tciMessage_t), GFP_KERNEL);
	if (tciMessage == NULL) {
		SSI_LOG_ERR("Allocation of TCI buffer failed\n");
		goto exit1;
	}

	for (retry = 0; retry < FIPS_RETRY_LIMIT; ++retry) {
		if (tee_error == CC_TEE_FIPS_ERROR_GENERAL) {
			SSI_LOG_DEBUG("before sleep (%u)\n", retry);
			msleep(FIPS_RETRY_DELAY);
			SSI_LOG_DEBUG("after sleep (%u)\n", retry);
		}

		memset(tciMessage, 0, sizeof(tciMessage_t));
		memset(&session_handle, 0, sizeof(struct mc_session_handle));
		session_handle.device_id = MC_DEVICE_ID;

		result = mc_open_session(&session_handle, &uuid, (u8 *)tciMessage, sizeof(tciMessage_t));
		SSI_LOG_DEBUG("after mc_open_session (%u)\n", retry);
		if (result == MC_DRV_OK)
			break;

		SSI_LOG_DEBUG("Open session failed(%u): %u\n", retry, result);
		if (tee_error != CC_TEE_FIPS_ERROR_GENERAL) {
			SSI_LOG_DEBUG("before sleep (%u)\n", retry);
			msleep(FIPS_RETRY_RUNTIME_DELAY);
			SSI_LOG_DEBUG("after sleep (%u)\n", retry);
		}
	}
	if (retry == FIPS_RETRY_LIMIT) {
		SSI_LOG_ERR("Open session failed: %u\n", result);
		goto exit2;
	}

	tciMessage->param_fips_set_ree_status.ree_status = fips_error;
	ret = tlcExecuteCmd(CMD_CRYS_FIPS_SET_REE_STATUS);
	if (ret != TLC_FIPS_OK) {
		SSI_LOG_ERR("ExecuteCMD(%d) failed: %u\n", CMD_CRYS_FIPS_SET_REE_STATUS, ret);
		goto exit3;
	}

	ret = tciMessage->param_fips_set_ree_status.ret;
	if (ret != SASI_OK) {
		SSI_LOG_ERR("Set TEE FIPS status failed: %u\n", ret);
		goto exit3;
	}
exit3:
	result = mc_close_session(&session_handle);
	if (result != MC_DRV_OK)
		SSI_LOG_ERR("Close session failed: %u\n", result);
exit2:
	kfree(tciMessage);
exit1:
	result = mc_close_device(MC_DEVICE_ID);
	if (result != MC_DRV_OK)
		SSI_LOG_ERR("Error closing device: %u\n", result);
exit0:
	return;
#else
	ssi_fips_error_t status;
	ssi_fips_ext_get_error(&status);

	SSI_LOG_ERR("Exporting the REE FIPS status %d.\n", status);
#endif
}

