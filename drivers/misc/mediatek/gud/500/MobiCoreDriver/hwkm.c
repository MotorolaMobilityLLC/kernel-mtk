// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2013-2021 TRUSTONIC LIMITED, All Rights Reserved.
 * Copyright (C) 2021 Motorola Mobility, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "main.h"
#include "public/mc_user.h"
#include "public/mc_linux_api.h"
#include "public/mobicore_driver_api.h"
#include "public/tlc_km.h"

static struct mc_session_handle g_km_session = { 0 };
struct mc_uuid_t tlkm_uuid = TEE_KEYMASTER_TA_UUID;
static u32 g_device_id = MC_DEVICE_ID_DEFAULT;
static kmtciMessage_t *km_tci = NULL;
static u32 km_session_counter = 0; /* Designed to be only one keymaster session.*/

static int hwkm_open_session()
{
	enum mc_result mc_ret = MC_DRV_ERR_UNKNOWN;

	if (km_session_counter == 1) {
		pr_notice("(%s):session already opened\n", __func__);
		return 0;
	}
	do {
		/* Open device */
		mc_ret = mc_open_device(g_device_id);
		if (mc_ret != MC_DRV_OK) {
			mc_dev_err(mc_ret, "mc_open_device failed\n");
			break;
		}

		/* Allocating WSM for TCI */
		mc_ret = mc_malloc_wsm(g_device_id, 0,
			sizeof(tci_t), (uint8_t **) &km_tci, 0);
		if (mc_ret != MC_DRV_OK) {
			mc_dev_err(mc_ret, "mc_malloc_wsm failed\n");
			mc_close_device(g_device_id);
			break;
		}

		/* Open session */
		g_km_session.device_id = g_device_id;
		mc_ret = mc_open_session(&g_km_session,
			(struct mc_uuid_t *)&tlkm_uuid,
			(uint8_t *)km_tci, sizeof(tci_t));
		if (mc_ret != MC_DRV_OK) {
			mc_dev_err(mc_ret, "mc_open_session fail\n");
			mc_free_wsm(g_device_id, (uint8_t *) km_tci);
			mc_close_device(g_device_id);
			km_tci = NULL;
			break;
		}

	} while (0);

	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "hwkm_open_session abort!\n");
		return -1;
	}

	/* Mark this as session opened */
	km_session_counter = 1;
	return 0;
}

static int hwkm_close_session(void)
{
	enum mc_result mc_ret = MC_DRV_ERR_UNKNOWN;

	if (km_session_counter == 0) {
		pr_notice("(%s):session already closed\n", __func__);
		return 0;
	}

	do {
		/* Close session */
		mc_ret = mc_close_session(&g_km_session);
		if (mc_ret != MC_DRV_OK) {
			mc_dev_err(mc_ret, "mc_close_session failed.\n");
			break;
		}

		/* Free WSM for TCI */
		mc_ret = mc_free_wsm(g_device_id, (uint8_t *) km_tci);
		if (mc_ret != MC_DRV_OK) {
			mc_dev_err(mc_ret, "mc_free_wsm failed\n");
			break;
		}
		km_tci = NULL;

		/* Close device */
		mc_ret = mc_close_device(g_device_id);
		if (mc_ret != MC_DRV_OK)
			mc_dev_err(mc_ret, "mc_close_device failed");

	} while (0);

	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "hwkm_close_session abort!");
		return -1;
	}

	/* Mark this with closed */
	km_session_counter = 0;

	return 0;

}

static unsigned int get_wrapped_key_size_in_bytes(u32 raw_secret_size)
{
	switch (raw_secret_size) {
		/* AES 128 wrapped key, 24 bytes */
		case HWKM_AES_128_BIT_KEY_SIZE:
		/* AES 256 wrapped key, 40 bytes */
		case HWKM_AES_256_BIT_KEY_SIZE:
			return raw_secret_size + WRAPPED_STORAGE_KEY_HEADER_SIZE;
		default:
			return 0;
	}
}

static bool check_wrapped_key_corrupted(const u8 *wrapped_key,
		u32 wrapped_key_size, u32 actual_size)
{
	u32 pos = actual_size;

	/* All the remaining fields are padded with 0x01, see the export_key()
	 * interface in keymaster HAL. This looks odd but for the time being
	 * Android has no specific format in terms of the storage key which is
	 * highly reliant upon the TEE implementation such that in our case we
	 * actually pad additional 24 bytes 0x01 to satify the fscrypt keyring
	 * process whereby the high level filesystem will require 64 bytes in total.
	 */
	if (wrapped_key_size > actual_size) {
		for (; pos < wrapped_key_size; pos++) {
			if (*(wrapped_key + pos) != 0x01)
				return true;
		}
	}
	return false;
}

int hwkm_program_key(const u8 *wrapped_key, u32 wrapped_key_size,
                     u32 gie_para, storage_key_type_t type)
{
	enum mc_result mc_ret = MC_DRV_ERR_UNKNOWN;
	int res = 0;
	struct mc_bulk_map wrapped_key_map = {0, 0};

	/* Check the input paremeters */
	if (wrapped_key == NULL ||
		wrapped_key_size < HWKM_AES_128_BIT_KEY_SIZE) {
		mc_dev_err(mc_ret, "invalid input, wrapped key_len=%d\n", wrapped_key_size);
		return -EINVAL;
	}

	/* Open the sesssion with mcDaemon */
	if (km_session_counter == 0) {
		pr_notice("(%s): open session for the first time\n", __func__);
		mc_ret = hwkm_open_session();
		if (mc_ret != MC_DRV_OK) {
			mc_dev_err(mc_ret, "open session failed");
			return -EINVAL;
		}
	}

	/* Map transfer data buffer */
	mc_ret = mc_map(&g_km_session, (void*)wrapped_key, wrapped_key_size, &wrapped_key_map);
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "mc_map() returned 0x%08x", mc_ret);
        hwkm_close_session();
		return -EINVAL;
	}

	/* Fill tci structure per command ID*/
	km_tci->command.header.commandId = CMD_ID_TEE_UNWRAP_AND_INSTALL_AES_STORAGE_KEY;
	km_tci->aes_storage_key_unwrap_install.wrapped_key_data.data = (uint32_t)wrapped_key_map.secure_virt_addr;
	km_tci->aes_storage_key_unwrap_install.wrapped_key_data.data_length = wrapped_key_map.secure_virt_len;
	km_tci->aes_storage_key_unwrap_install.cfg = gie_para;
	km_tci->aes_storage_key_unwrap_install.storage_key_type = type;

	/* Communicate with secure world */
	mc_ret = mc_notify(&g_km_session);
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "mc_notify failed.");
		res = -EINVAL;
		goto exit;
	}

	mc_ret = mc_wait_notification(&g_km_session, MC_INFINITE_TIMEOUT);
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "mc_wait_notification failed");
		res = -EINVAL;
		goto exit;
	}

	/* Session completed but secure world reports invalid error.
	 * We will not close the session but let the caller to catch
	 * the error returned from the secure world and make a decision
	 * for further operation e.g retry or not.
	 */
	res = mc_ret;
	if (km_tci->response.header.returnCode != 0) {
		mc_dev_err(mc_ret, "secure side error for keyslot programming: %d\n",
			km_tci->response.header.returnCode);
		res = -EAGAIN;
	}

exit:
	if (wrapped_key != NULL) {
		mc_unmap(&g_km_session, (void*)wrapped_key, &wrapped_key_map);
	}
	if (mc_ret != MC_DRV_OK) {
        hwkm_close_session();
		mc_dev_err(mc_ret, "hwkm_program_key fail!");
	}
	return res;
}

int hwkm_derive_raw_secret(const u8 *wrapped_key, u32 wrapped_key_size,
                           u8 *raw_secret, u32 raw_secret_size)
{
	enum mc_result mc_ret = MC_DRV_ERR_UNKNOWN;
	int res = 0;
	struct mc_bulk_map wrapped_key_map = {0, 0};
	struct mc_bulk_map raw_secret_map = {0, 0};
	u32 hwkm_wrapped_key_size = 0;

	/* Check the input paremeters */
	if (wrapped_key == NULL ||
		raw_secret == NULL ||
		wrapped_key_size <= raw_secret_size) {
		mc_dev_err(mc_ret, "invalid input, wrapped_key_size=%d, raw_secret_size=%d\n",
						wrapped_key_size, raw_secret_size);
		return -EINVAL;
	}

	/* Adjust the wrapped key size in case of the padding data */
	hwkm_wrapped_key_size = get_wrapped_key_size_in_bytes(raw_secret_size);
	if (hwkm_wrapped_key_size == 0 ||
		wrapped_key_size < hwkm_wrapped_key_size) {
		mc_dev_err(mc_ret, "invalid wrapped key size!");
		return -EINVAL;
	}

	/* If the wrapped key is padded with known fields, here in our case is 0x01
	 * we should validate the remaining data to see if it's corrupted or not.
	 */
	if (check_wrapped_key_corrupted(wrapped_key,
			wrapped_key_size, hwkm_wrapped_key_size)) {
		mc_dev_err(mc_ret, "wrapped key corruption detected!");
		return -EINVAL;
	}

	/* Open the sesssion with mcDaemon */
	if (km_session_counter == 0) {
		pr_notice("(%s): open session for the first time\n", __func__);
		mc_ret = hwkm_open_session();
		if (mc_ret != MC_DRV_OK) {
			mc_dev_err(mc_ret, "open session failed\n");
			return -EINVAL;
		}
	}

	/* Map wrapped key and raw secret buffer */
	mc_ret = mc_map(&g_km_session, (void*)wrapped_key, hwkm_wrapped_key_size, &wrapped_key_map);
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "mc_map failed, wrapped key buf returned: 0x%08x", mc_ret);
        hwkm_close_session();
		return -EINVAL;
	}
	mc_ret = mc_map(&g_km_session, (void*)raw_secret, raw_secret_size, &raw_secret_map);
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "mc_map failed, raw secret buf returned: 0x%08x", mc_ret);
		mc_unmap(&g_km_session, (void*)wrapped_key, &wrapped_key_map);
        hwkm_close_session();
		return -EINVAL;
	}

	/* Fill tci structure per command ID */
	km_tci->command.header.commandId = CMD_ID_TEE_UNWRAP_AND_DERIVE_AES_STORAGE_SECRET;
	km_tci->aes_storage_key_unwrap_derive.wrapped_key_data.data = (uint32_t)wrapped_key_map.secure_virt_addr;
	km_tci->aes_storage_key_unwrap_derive.wrapped_key_data.data_length = wrapped_key_map.secure_virt_len;
	km_tci->aes_storage_key_unwrap_derive.raw_secret.data = (uint32_t)raw_secret_map.secure_virt_addr;
	km_tci->aes_storage_key_unwrap_derive.raw_secret.data_length = raw_secret_map.secure_virt_len;

	/* Communicate with secure world */
	mc_ret = mc_notify(&g_km_session);
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "mc_notify failed");
		res = -EINVAL;
		goto exit;
	}

	mc_ret = mc_wait_notification(&g_km_session, MC_INFINITE_TIMEOUT);
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "mc_wait_notification failed");
		res = -EINVAL;
		goto exit;
	}

	/* Session completed but secure world reports invalid error.
	 * We will not close the session but let the caller to catch
	 * the error returned from the secure world and make a decision
	 * for further operation e.g retry or not.
	 */
	res = mc_ret;
	if (km_tci->response.header.returnCode != 0) {
		mc_dev_err(mc_ret, "secure side error for key derivation: %d\n",
			km_tci->response.header.returnCode);
		res = -EOPNOTSUPP;
	}

exit:
	if (wrapped_key != NULL) {
		mc_unmap(&g_km_session, (void*)wrapped_key, &wrapped_key_map);
	}
	if (raw_secret != NULL) {
		mc_unmap(&g_km_session, (void*)raw_secret, &raw_secret_map);
	}
	if (mc_ret != MC_DRV_OK) {
		mc_dev_err(mc_ret, "derive raw secret fail");
        hwkm_close_session();
	}
	return res;
}

static uint32_t g_slot_mask = 0;

void hwkm_set_slot_programmed_mask(unsigned int slot_num)
{
	g_slot_mask = (0x1 << slot_num) | g_slot_mask;
}

void hwkm_set_slot_evicted_mask(unsigned int slot_num)
{
	g_slot_mask = (~(0x1 << slot_num)) & g_slot_mask;
}

bool hwkm_is_slot_already_programmed(unsigned int slot_num)
{
	if ((g_slot_mask & (0x1 << slot_num)) != 0)
		return true;
	return false;
}
