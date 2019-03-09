/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "../common.h"
#include "../platform.h"
#include "finger_report.h"
#include "firmware.h"
#include "gesture.h"
#include "protocol.h"
#include "config.h"

#define ESD_GESTURE_PWD			0xF38A94EF
#define ESD_GESTURE_RUN			0x5B92E7F4
#define ESD_GESTURE_PWD_ADDR	0x25FF8

struct core_gesture_data *core_gesture;
extern uint8_t ap_fw[MAX_AP_FIRMWARE_SIZE];

int core_esd_gesture(void)
{
	int ret = 0, retry = 100;
	u32 answer = 0;

	/* start to download AP code with HW reset or host download */
	ret = ilitek_platform_reset_ctrl(true, RST_METHODS);

	ret = core_config_ice_mode_enable(STOP_MCU);
	if (ret < 0) {
		ipio_err("Failed to enable ICE mode\n");
		goto out;
	}

	/* write a special password to inform FW go back into gesture mode */
	ret = core_config_ice_mode_write(ESD_GESTURE_PWD_ADDR, ESD_GESTURE_PWD, 4);
	if (ret < 0) {
		ipio_err("esd gesture : write password failed, ret = %d\n", ret);
		goto out;
	}

	/* HW reset or host download again gives effect to FW receives password successed */

	ret = ilitek_platform_reset_ctrl(true, RST_METHODS);
	if (ret < 0)
		goto out;

	/* waiting for FW reloading code */
    msleep(100);

	ret = core_config_ice_mode_enable(NO_STOP_MCU);
	if (ret < 0) {
		ipio_err("Failed to enable ICE mode\n");
		goto out;
	}

	/* polling another specific register to see if gesutre is enabled properly */
	do {
		answer = core_config_ice_mode_read(ESD_GESTURE_PWD_ADDR);
		ipio_info("answer = 0x%x\n", answer);

		msleep(10);

		retry--;
	} while (answer != ESD_GESTURE_RUN && retry > 0);

	if (retry <= 0) {
		ipio_err("reenter gesture failed\n");
		goto out;
	}

	core_config_ice_mode_disable();

	/* reloading gesture code if host download is enabled */
#ifdef HOST_DOWNLOAD
	ret = core_gesture_load_code();
	if (ret < 0) {
		ipio_err("load gesture code failed\n");
		goto out;
	}
#endif

	return 0;

out:
	if (core_config->icemodeenable)
		core_config_ice_mode_disable();

	ipio_err("esd failure handling failed on gesture mode\n");
	return ret;
}
EXPORT_SYMBOL(core_esd_gesture);

#ifdef HOST_DOWNLOAD
int core_gesture_load_code(void)
{
	int i = 0, ret = 0, retry = 0;
	uint8_t temp[64] = {0};

	core_gesture->entry = true;
	retry = core_firmware->retry_times;

	/* Already read during parsing hex file */
	ipio_info("gesture_start_addr = 0x%x, length = 0x%x\n", core_gesture->start_addr, core_gesture->length);
	ipio_info("area = %d, ap_start_addr = 0x%x, ap_length = 0x%x\n",
				core_gesture->area_section, core_gesture->ap_start_addr, core_gesture->ap_length);

	/* write load gesture flag */
	temp[0] = 0x01;
	temp[1] = 0x0A;
	temp[2] = 0x03;
	if ((core_write(core_config->slave_i2c_addr, temp, 3)) < 0) {
		ipio_err("write gesture flag error\n");
	}

	/* enter gesture cmd lpwg start */
	ret = core_config_switch_fw_mode(&protocol->gesture_mode);
	if (ret < 0)
		ipio_err("Failed to switch gesture mode\n");

	for (i = 0; i < 20; i++) {
		/* Prepare Check Ready */
		temp[0] = 0xF6;
		temp[1] = 0x0A;
		temp[2] = 0x05;
		if ((core_write(core_config->slave_i2c_addr, temp, 2)) < 0) {
			ipio_err("write 0xF6,0xA,0x05 command error\n");
		}

		mdelay(i * 50);

		/* Check ready for load code*/
		temp[0] = 0x01;
		temp[1] = 0x0A;
		temp[2] = 0x05;
		if ((core_write(core_config->slave_i2c_addr, temp, 3)) < 0) {
			ipio_err("write command error\n");
		}
		if ((core_read(core_config->slave_i2c_addr, temp, 1)) < 0) {
			ipio_err("Read command error\n");
		}
		if (temp[0] == 0x91) {
			ipio_info("check fw ready\n");
			break;
		}
	}

	if (temp[0] != 0x91)
		ipio_err("FW is busy, error\n");

	ret = core_firmware_upgrade(UPGRADE_IRAM, HEX_FILE, OPEN_FW_METHOD);
	if (ret < 0)
		ipio_err("Gesture load code failed \n");

	/* FW star run gestrue code cmd*/
	temp[0] = 0x01;
	temp[1] = 0x0A;
	temp[2] = 0x06;
	if ((core_write(core_config->slave_i2c_addr, temp, 3)) < 0) {
		ipio_err("write command error\n");
	}

	core_gesture->entry = false;
	return ret;
}
EXPORT_SYMBOL(core_gesture_load_code);
#endif

int core_gesture_match_key(uint8_t gdata)
{
	int gcode;

	switch (gdata) {
	case GESTURE_LEFT:
		gcode = KEY_GESTURE_LEFT;
		break;
	case GESTURE_RIGHT:
		gcode = KEY_GESTURE_RIGHT;
		break;
	case GESTURE_UP:
		gcode = KEY_GESTURE_UP;
		break;
	case GESTURE_DOWN:
		gcode = KEY_GESTURE_DOWN;
		break;
	case GESTURE_DOUBLECLICK:
		gcode = KEY_GESTURE_D;
		break;
	case GESTURE_O:
		gcode = KEY_GESTURE_O;
		break;
	case GESTURE_W:
		gcode = KEY_GESTURE_W;
		break;
	case GESTURE_M:
		gcode = KEY_GESTURE_M;
		break;
	case GESTURE_E:
		gcode = KEY_GESTURE_E;
		break;
	case GESTURE_S:
		gcode = KEY_GESTURE_S;
		break;
	case GESTURE_V:
		gcode = KEY_GESTURE_V;
		break;
	case GESTURE_Z:
		gcode = KEY_GESTURE_Z;
		break;
	case GESTURE_C:
		gcode = KEY_GESTURE_C;
		break;
	case GESTURE_F:
		gcode = KEY_GESTURE_F;
		break;
	default:
		gcode = -1;
		break;
	}

	ipio_debug(DEBUG_GESTURE, "gcode = %d\n", gcode);
	return gcode;
}
EXPORT_SYMBOL(core_gesture_match_key);

void core_gesture_set_key(struct core_fr_data *fr_data)
{
	struct input_dev *input_dev = fr_data->input_device;
	if (input_dev != NULL) {
		input_set_capability(input_dev, EV_KEY, KEY_POWER);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_UP);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_DOWN);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_LEFT);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_RIGHT);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_O);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C);
		input_set_capability(input_dev, EV_KEY, KEY_GESTURE_F);

		__set_bit(KEY_POWER, input_dev->keybit);
		__set_bit(KEY_GESTURE_UP, input_dev->keybit);
		__set_bit(KEY_GESTURE_DOWN, input_dev->keybit);
		__set_bit(KEY_GESTURE_LEFT, input_dev->keybit);
		__set_bit(KEY_GESTURE_RIGHT, input_dev->keybit);
		__set_bit(KEY_GESTURE_O, input_dev->keybit);
		__set_bit(KEY_GESTURE_E, input_dev->keybit);
		__set_bit(KEY_GESTURE_M, input_dev->keybit);
		__set_bit(KEY_GESTURE_W, input_dev->keybit);
		__set_bit(KEY_GESTURE_S, input_dev->keybit);
		__set_bit(KEY_GESTURE_V, input_dev->keybit);
		__set_bit(KEY_GESTURE_Z, input_dev->keybit);
		__set_bit(KEY_GESTURE_C, input_dev->keybit);
		__set_bit(KEY_GESTURE_F, input_dev->keybit);
		return;
	}

	ipio_err("GESTURE: input dev is NULL\n");
}
EXPORT_SYMBOL(core_gesture_set_key);

int core_gesture_init(void)
{
	core_gesture = devm_kzalloc(ipd->dev, sizeof(struct core_gesture_data), GFP_KERNEL);
	if (ERR_ALLOC_MEM(core_gesture)) {
		ipio_err("Failed to allocate core_gesture mem, %ld\n", PTR_ERR(core_gesture));
		return -ENOMEM;
	}

	core_gesture->entry = false;
	core_gesture->mode = GESTURE_MODE;

	return 0;
}
EXPORT_SYMBOL(core_gesture_init);

void core_gesture_remove(void)
{
	ipio_info("Remove core-gesture members\n");
	ipio_kfree((void **)&core_gesture);
}
EXPORT_SYMBOL(core_gesture_remove);
