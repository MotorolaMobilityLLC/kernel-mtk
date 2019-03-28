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
#include "config.h"
#include "firmware.h"
#include "protocol.h"
#include "i2c.h"
#include "flash.h"
#include "finger_report.h"
#include "gesture.h"
#include "mp_test.h"
#include "spi.h"

/* the list of support chip */
uint32_t ipio_chip_list[] = {
	CHIP_TYPE_ILI9881,
	CHIP_TYPE_ILI7807,
};

uint8_t g_read_buf[128] = {0};

extern char mode_chose;
struct core_config_data *core_config;

struct set_res_data set_res;

void core_config_read_flash_info(void)
{
	int i;
	uint16_t flash_id = 0, flash_mid = 0;
	uint8_t buf[4] = {0};
	uint8_t cmd = 0x9F;

	core_config_ice_mode_enable(STOP_MCU);

	core_config_ice_mode_write(0x41000, 0x0, 1);	/* CS low */
	core_config_ice_mode_write(0x41004, 0x66aa55, 3);	/* Key */

	core_config_ice_mode_write(0x41008, cmd, 1);
	for (i = 0; i < ARRAY_SIZE(buf); i++) {
		core_config_ice_mode_write(0x041008, 0xFF, 1);
		buf[i] = core_config_ice_mode_read(0x41010);
	}

	core_config_ice_mode_write(0x041000, 0x1, 1);	/* CS high */

	/* look up flash info and init its struct after obtained flash id. */
	flash_mid = buf[0];
	flash_id = buf[1] << 8 | buf[2];
	core_flash_init(flash_mid, flash_id);

	core_config_ice_mode_disable();
	return;
}

uint32_t core_config_read_pc_counter(void)
{
	uint32_t pc_cnt = 0x0;
	bool ic_mode_flag = 0;

	ic_mode_flag = core_config->icemodeenable;

	if (ic_mode_flag == false)
		core_config_ice_mode_enable(STOP_MCU);

	/* Read fw status if it was hanging on a unknown status */
	pc_cnt = core_config_ice_mode_read(ILI9881_PC_COUNTER_ADDR);
	ipio_err("pc counter = 0x%x\n", pc_cnt);

	if (ic_mode_flag == false) {
		if (core_config_ice_mode_disable() < 0)
			ipio_err("Failed to disable ice mode\n");
	}
	return pc_cnt;
}
EXPORT_SYMBOL(core_config_read_pc_counter);

uint32_t core_config_read_write_onebyte(uint32_t addr)
{
	int ret = 0;
	uint32_t data = 0;
	uint8_t szOutBuf[64] = { 0 };

	szOutBuf[0] = 0x25;
	szOutBuf[1] = (char)((addr & 0x000000FF) >> 0);
	szOutBuf[2] = (char)((addr & 0x0000FF00) >> 8);
	szOutBuf[3] = (char)((addr & 0x00FF0000) >> 16);

	ret = core_write(core_config->slave_i2c_addr, szOutBuf, 4);
	if (ret < 0)
		goto out;

	mdelay(1);

	ret = core_read(core_config->slave_i2c_addr, szOutBuf, 1);
	if (ret < 0)
		goto out;

	data = (szOutBuf[0]);

	return data;

out:
	ipio_err("Failed to read/write data in ICE mode, ret = %d\n", ret);
	return ret;
}
EXPORT_SYMBOL(core_config_read_write_onebyte);

uint32_t core_config_ice_mode_read(uint32_t addr)
{
	int ret = 0;
	uint8_t szOutBuf[64] = { 0 };
	uint32_t data = 0;

	szOutBuf[0] = 0x25;
	szOutBuf[1] = (char)((addr & 0x000000FF) >> 0);
	szOutBuf[2] = (char)((addr & 0x0000FF00) >> 8);
	szOutBuf[3] = (char)((addr & 0x00FF0000) >> 16);

	ret = core_write(core_config->slave_i2c_addr, szOutBuf, 4);
	if (ret < 0)
		goto out;

	mdelay(10);

	ret = core_read(core_config->slave_i2c_addr, szOutBuf, 4);
	if (ret < 0)
		goto out;

	data = (szOutBuf[0] | szOutBuf[1] << 8 | szOutBuf[2] << 16 | szOutBuf[3] << 24);

	return data;

out:
	ipio_err("Failed to read data in ICE mode, ret = %d\n", ret);
	return ret;
}
EXPORT_SYMBOL(core_config_ice_mode_read);

int core_config_switch_fw_mode(uint8_t *data)
{
	int ret = 0, i, mode, prev_mode;
	int checksum = 0;
	uint8_t mp_code[14] = { 0 };
	uint8_t cmd[4] = { 0 };

	if (data == NULL) {
		ipio_err("data is invaild\n");
		return -1;
	}

	if (protocol->major != 0x5) {
		ipio_err("Wrong the major version of protocol, 0x%x\n", protocol->major);
		return -1;
	}

	ilitek_platform_disable_irq();

	mode = data[0];
	prev_mode = core_fr->actual_fw_mode;
	core_fr->actual_fw_mode = mode;

	ipio_info("change fw mode from (%d) to (%d).\n", prev_mode, core_fr->actual_fw_mode);

	switch (core_fr->actual_fw_mode) {
	case P5_0_FIRMWARE_I2CUART_MODE:
		cmd[0] = protocol->cmd_i2cuart;
		cmd[1] = *(data + 1);
		cmd[2] = *(data + 2);

		ipio_info("Switch to I2CUART mode, cmd = %x, b1 = %x, b2 = %x\n", cmd[0], cmd[1], cmd[2]);

		ret = core_write(core_config->slave_i2c_addr, cmd, 3);
		if (ret < 0)
			ipio_err("Failed to switch I2CUART mode\n");

		break;
	case P5_0_FIRMWARE_DEMO_MODE:
		ipio_info("Switch to Demo mode by hw reset\n");

		ret = ilitek_platform_reset_ctrl(true, RST_METHODS);

		break;
	case P5_0_FIRMWARE_DEBUG_MODE:
		cmd[0] = protocol->cmd_mode_ctrl;
		cmd[1] = mode;

		ipio_info("Switch to Debug mode\n");

		ret = core_write(core_config->slave_i2c_addr, cmd, 2);
		if (ret < 0)
			ipio_err("Failed to switch Demo/Debug mode\n");

		break;
	case P5_0_FIRMWARE_GESTURE_MODE:
		cmd[0] = 0x1;
		cmd[1] = 0xA;
		cmd[2] = core_gesture->mode + 1;

		ipio_info("Switch to Gesture mode, b0[0] = 0x%x, b1 = 0x%x, b2 = 0x%x\n", cmd[0], cmd[1], cmd[2]);

		ret = core_write(core_config->slave_i2c_addr, cmd, 3);
		if (ret < 0)
			ipio_err("Failed to switch Gesture mode\n");
		break;
	case P5_0_FIRMWARE_TEST_MODE:
		ipio_info("Switch to Test mode\n");

		if (mode_chose == I2C_MODE) {
			cmd[0] = protocol->cmd_mode_ctrl;
			cmd[1] = mode;

			ret = core_write(core_config->slave_i2c_addr, cmd, 2);
			if (ret < 0) {
				ipio_err("Failed to switch Test mode\n");
				break;
			}

			ipio_info("b0[0] = 0x%x, b1 = 0x%x\n", cmd[0], cmd[1]);

			cmd[0] = protocol->cmd_get_mp_info;

			/* Read MP Test information to ensure if fw supports test mode. */
			core_write(core_config->slave_i2c_addr, cmd, 1);
			mdelay(10);
			core_read(core_config->slave_i2c_addr, mp_code, protocol->mp_info_len);

			for (i = 0; i < protocol->mp_info_len - 1; i++)
				checksum += mp_code[i];

			if ((-checksum & 0xFF) != mp_code[protocol->mp_info_len - 1]) {
				ipio_err("checksume error (0x%x), FW doesn't support test mode.\n",
						(-checksum & 0XFF));
				ret = -1;
				break;
			}

			/* After command to test mode, fw stays at demo mode until busy free. */
			core_fr->actual_fw_mode = protocol->demo_mode;

			/* Check ready to switch test mode from demo mode */
			if (core_config_check_cdc_busy(50, 50) < 0) {
				ipio_err("Mode(%d) Check busy is timout\n", core_fr->actual_fw_mode);
				ret = -1;
				break;
			}
		}

		/* Now set up fw as test mode */
		core_fr->actual_fw_mode = protocol->test_mode;

		if (core_mp_move_code() != 0) {
			ipio_err("Switch to test mode failed\n");
			ret = -1;
		}
		break;
	default:
		ipio_err("Unknown firmware mode: %x\n", mode);
		ret = -1;
		break;
	}

	if (ret < 0) {
		ipio_err("switch mode failed, return to previous mode\n");
		core_fr->actual_fw_mode = prev_mode;
	}

	ipio_info("Actual FW mode = %d\n", core_fr->actual_fw_mode);

	if (core_fr->actual_fw_mode != protocol->test_mode)
		ilitek_platform_enable_irq();

	return ret;
}
EXPORT_SYMBOL(core_config_switch_fw_mode);

int core_config_ice_mode_write(uint32_t addr, uint32_t data, uint32_t size)
{
	int ret = 0, i;
	uint8_t szOutBuf[64] = { 0 };

	szOutBuf[0] = 0x25;
	szOutBuf[1] = (char)((addr & 0x000000FF) >> 0);
	szOutBuf[2] = (char)((addr & 0x0000FF00) >> 8);
	szOutBuf[3] = (char)((addr & 0x00FF0000) >> 16);

	for (i = 0; i < size; i++) {
		szOutBuf[i + 4] = (char)(data >> (8 * i));
	}

	ret = core_write(core_config->slave_i2c_addr, szOutBuf, size + 4);

	if (ret < 0)
		ipio_err("Failed to write data in ICE mode, ret = %d\n", ret);

	return ret;
}
EXPORT_SYMBOL(core_config_ice_mode_write);

int core_config_ice_mode_bit_mask(uint32_t addr, uint32_t nMask, uint32_t value)
{
	int ret = 0;
	uint32_t data = 0;

	data = core_config_ice_mode_read(addr);

	data &= (~nMask);
	data |= (value & nMask);

	ipio_info("mask value data = %x\n", data);

	ret = core_config_ice_mode_write(addr, data, 4);
	if (ret < 0)
		ipio_err("Failed to re-write data in ICE mode, ret = %d\n", ret);

	return ret;
}
EXPORT_SYMBOL(core_config_ice_mode_bit_mask);

int core_config_ic_reset(void)
{
	int ret = 0;
	uint32_t key = 0;

	ipio_info("SW RESET\n");
	/*Whole chip reset*/
	if (!core_config->icemodeenable)
		core_config_ice_mode_enable(NO_STOP_MCU);

	if (core_config->chip_id == CHIP_TYPE_ILI9881)
		key = 0x00019881;
	else if (core_config->chip_id == CHIP_TYPE_ILI7807)
		key = 0x00019878;

	ipio_debug(DEBUG_CONFIG, "key = 0x%x\n", key);
	if (key != 0) {
		ret = core_config_ice_mode_write(core_config->ic_reset_addr, key, 4);
		if (ret < 0)
			ipio_err("ic reset failed, ret = %d\n", ret);
	}

	msleep(100);
	return ret;
}
EXPORT_SYMBOL(core_config_ic_reset);

void core_config_sense_ctrl(bool start)
{
	ipio_info("sense start = %d\n", start);

	return core_protocol_func_control(0, start);
}
EXPORT_SYMBOL(core_config_sense_ctrl);

void core_config_sleep_ctrl(bool out)
{
	uint8_t cmd[3] = {0x01, 0x02, 0x03};
	if (mode_chose == I2C_MODE) {
		cmd[2] = 0x00;
	}
	ipio_info("Sleep Out = %d\n", out);
	core_write(core_config->slave_i2c_addr, cmd, 3);
	return;
}
EXPORT_SYMBOL(core_config_sleep_ctrl);

void core_config_glove_ctrl(bool enable, bool seamless)
{
	int cmd = 0x2;		/* default as semaless */

	if (!seamless) {
		if (enable)
			cmd = 0x1;
		else
			cmd = 0x0;
	}

	ipio_info("Glove = %d, seamless = %d, cmd = %d\n", enable, seamless, cmd);

	return core_protocol_func_control(2, cmd);
}
EXPORT_SYMBOL(core_config_glove_ctrl);

void core_config_stylus_ctrl(bool enable, bool seamless)
{
	int cmd = 0x2;		/* default as semaless */

	if (!seamless) {
		if (enable)
			cmd = 0x1;
		else
			cmd = 0x0;
	}

	ipio_info("stylus = %d, seamless = %d, cmd = %x\n", enable, seamless, cmd);

	return core_protocol_func_control(3, cmd);
}
EXPORT_SYMBOL(core_config_stylus_ctrl);

void core_config_tp_scan_mode(bool mode)
{
	ipio_info("TP Scan mode = %d\n", mode);

	return core_protocol_func_control(4, mode);
}
EXPORT_SYMBOL(core_config_tp_scan_mode);

void core_config_lpwg_ctrl(bool enable)
{
	ipio_info("LPWG = %d\n", enable);

	return core_protocol_func_control(5, enable);
}
EXPORT_SYMBOL(core_config_lpwg_ctrl);

void core_config_gesture_ctrl(uint8_t func)
{
	uint8_t max_byte = 0x0, min_byte = 0x0;

	ipio_info("Gesture function = 0x%x\n", func);

	max_byte = 0x3F;
	min_byte = 0x20;

	if (func > max_byte || func < min_byte) {
		ipio_err("Gesture ctrl error, 0x%x\n", func);
		return;
	}

	return core_protocol_func_control(6, func);
}
EXPORT_SYMBOL(core_config_gesture_ctrl);

void core_config_phone_cover_ctrl(bool enable)
{
	ipio_info("Phone Cover = %d\n", enable);

	return core_protocol_func_control(7, enable);
}
EXPORT_SYMBOL(core_config_phone_cover_ctrl);

void core_config_finger_sense_ctrl(bool enable)
{
	ipio_info("Finger sense = %d\n", enable);

	return core_protocol_func_control(0, enable);
}
EXPORT_SYMBOL(core_config_finger_sense_ctrl);

void core_config_proximity_ctrl(bool enable)
{
	ipio_info("Proximity = %d\n", enable);

	return core_protocol_func_control(11, enable);
}
EXPORT_SYMBOL(core_config_proximity_ctrl);

void core_config_plug_ctrl(bool out)
{
	ipio_info("Plug Out = %d\n", out);

	return core_protocol_func_control(12, out);
}
EXPORT_SYMBOL(core_config_plug_ctrl);

void core_config_set_phone_cover(uint8_t *pattern)
{
	int i;

	if (pattern == NULL) {
		ipio_err("Invaild pattern\n");
		return;
	}

	for (i = 0; i < 8; i++)
		protocol->phone_cover_window[i+1] = pattern[i];

	ipio_info("window: cmd = 0x%x\n", protocol->phone_cover_window[0]);
	ipio_info("window: ul_x_l = 0x%x, ul_x_h = 0x%x\n", protocol->phone_cover_window[1],
		 protocol->phone_cover_window[2]);
	ipio_info("window: ul_y_l = 0x%x, ul_y_l = 0x%x\n", protocol->phone_cover_window[3],
		 protocol->phone_cover_window[4]);
	ipio_info("window: br_x_l = 0x%x, br_x_l = 0x%x\n", protocol->phone_cover_window[5],
		 protocol->phone_cover_window[6]);
	ipio_info("window: br_y_l = 0x%x, br_y_l = 0x%x\n", protocol->phone_cover_window[7],
		 protocol->phone_cover_window[8]);

	core_protocol_func_control(9, 0);
}
EXPORT_SYMBOL(core_config_set_phone_cover);

void core_config_ic_suspend(void)
{
	int i;

	ipio_info("Starting to suspend ...\n");
	if (ipd->suspended) {
		ipio_info("TP already in suspend status ...\n");
		return;
	}

	core_fr->isEnableFR = false;
	ilitek_platform_disable_irq();

	if (ipd->isEnablePollCheckPower)
		cancel_delayed_work_sync(&ipd->check_power_status_work);
	if (ipd->isEnablePollCheckEsd)
		cancel_delayed_work_sync(&ipd->check_esd_status_work);

	/* sense stop */
	core_config_sense_ctrl(false);

	/* check system busy */
	if (core_config_check_cdc_busy(50, 50) < 0)
		ipio_err("Check busy is timout !\n");

	ipio_info("Enabled Gesture = %d\n", core_config->isEnableGesture);

	if (core_config->isEnableGesture) {
		if (mode_chose == SPI_MODE) {
			if (core_gesture_load_code() < 0)
				ipio_err("load gesture code fail\n");
		} else {
			core_config_switch_fw_mode(&protocol->gesture_mode);
		}
		enable_irq_wake(ipd->isr_gpio);
		core_fr->isEnableFR = true;
		ilitek_platform_enable_irq();
		goto end;
	}

	/* sleep in if gesture is disabled. */
	core_config_sleep_ctrl(false);
	ipd->suspended = true;

end:
	/* release all touch points to get rid of locked points on screen. */
#ifdef MT_B_TYPE
	for (i = 0 ; i < MAX_TOUCH_NUM; i++)
		core_fr_touch_release(0, 0, i);

	input_report_key(core_fr->input_device, BTN_TOUCH, 0);
	input_report_key(core_fr->input_device, BTN_TOOL_FINGER, 0);
#else
	core_fr_touch_release(0, 0, 0);
#endif
	input_sync(core_fr->input_device);

	ipio_info("Suspend done\n");
}
EXPORT_SYMBOL(core_config_ic_suspend);

void core_config_ic_resume(void)
{
	ipio_info("Starting to resume ...\n");
	if (!ipd->suspended) {
		ipio_info("TP already in resume statues ...\n");
		return;
	}

	/* we hope there's no any reports during resume */
	core_fr->isEnableFR = false;
	ilitek_platform_disable_irq();

	if (core_config->isEnableGesture) {
		disable_irq_wake(ipd->isr_gpio);
	}

	if (mode_chose == I2C_MODE)
		ilitek_platform_reset_ctrl(true, HW_RST);
	else
		core_config_switch_fw_mode(&protocol->demo_mode);

	if (ipd->isEnablePollCheckPower)
		queue_delayed_work(ipd->check_power_status_queue,
			&ipd->check_power_status_work, ipd->work_delay);
	if (ipd->isEnablePollCheckEsd)
		queue_delayed_work(ipd->check_esd_status_queue,
			&ipd->check_esd_status_work, ipd->esd_check_time);

	core_fr->isEnableFR = true;
	ilitek_platform_enable_irq();
	ipd->suspended = false;

	ipio_info("Resume done\n");
}
EXPORT_SYMBOL(core_config_ic_resume);

int core_config_ice_mode_disable(void)
{
	int ret = 0;
	uint8_t cmd[4] = {0x1b, 0x62, 0x10, 0x18};

	ipio_info("ICE Mode disabled\n")

	ret = core_write(core_config->slave_i2c_addr, cmd, 4);
	if (ret < 0)
		ipio_err("Failed to write ice mode disable\n");

	core_config->icemodeenable = false;

	return ret;
}
EXPORT_SYMBOL(core_config_ice_mode_disable);

static int core_config_ice_mode_chipid_check(void)
{
	int ret = 0;
	uint32_t pid = 0;

	pid = core_config_ice_mode_read(core_config->pid_addr);

	if (((pid >> 16) != CHIP_TYPE_ILI9881) && ((pid >> 16) != CHIP_TYPE_ILI7807)) {
		ipio_info("read PID Fail  pid = 0x%x\n", (pid >> 16));
		ret = -EINVAL;
	} else {
		core_config->chip_pid = pid;
		core_config->chip_id = pid >> 16;
		core_config->chip_type = (pid & 0x0000FF00) >> 8;
		core_config->core_type = pid & 0xFF;
	}

	return ret;
}

int core_config_ice_mode_enable(bool stop_mcu)
{
	int ret = 0, retry = 3;
	uint8_t cmd[4] = {0x25, 0x62, 0x10, 0x18};

	ipio_info("ICE Mode enabled, stop mcu = %d\n", stop_mcu);

	core_config->icemodeenable = true;

	if (!stop_mcu)
		cmd[0] = 0x1F;

	do {
		ret = core_write(core_config->slave_i2c_addr, cmd, 4);
		if (ret < 0)
			ipio_err("Failed to write ice mode enable\n");

#ifdef ENABLE_SPI_SPEED_UP
		core_spi_speed_up(true);
#endif
		mdelay(25);

		if (core_config_ice_mode_chipid_check() >= 0) {
			core_config_ice_mode_write(0x047002, 0x00, 1);
			return ret;
		}
		ipio_info("ICE Mode enabled fail %d time\n", (4 - retry));
	} while (--retry > 0);

	ipio_info("ICE Mode enabled fail \n");
	return ret;
}
EXPORT_SYMBOL(core_config_ice_mode_enable);

int core_config_set_watch_dog(bool enable)
{
	int timeout = 100, ret = 0;
	uint8_t off_bit = 0x5A, on_bit = 0xA5;
	uint8_t value_low = 0x0, value_high = 0x0;
	uint32_t wdt_addr = core_config->wdt_addr;

	if (wdt_addr <= 0 || core_config->chip_id <= 0) {
		ipio_err("WDT/CHIP ID is invalid\n");
		return -EINVAL;
	}

	/* FW will automatiacally disable WDT in I2C */
	if (mode_chose == I2C_MODE) {
		ipio_info("Interface is I2C, do nothing\n");
		return 0;
	}

	/* Config register and values by IC */
	 if (core_config->chip_id == CHIP_TYPE_ILI9881) {
		value_low = 0x81;
		value_high = 0x98;
	} else if (core_config->chip_id == CHIP_TYPE_ILI7807) {
		value_low = 0x78;
		value_high = 0x98;
	} else {
		ipio_err("Unknown CHIP type (0x%x)\n", core_config->chip_id);
		return -ENODEV;
	}

	if (enable) {
		core_config_ice_mode_write(wdt_addr, 1, 1);
	} else {
		core_config_ice_mode_write(wdt_addr, value_low, 1);
		core_config_ice_mode_write(wdt_addr, value_high, 1);
		/* need to delay 300us after stop mcu to wait fw relaod */
		udelay(300);
	}

	while (timeout > 0) {
		ret = core_config_ice_mode_read(0x51018);
		ipio_debug(DEBUG_CONFIG, "bit = %x\n", ret);

		if (enable) {
			if (CHECK_EQUAL(ret, on_bit) == 0)
				break;
		} else {
			if (CHECK_EQUAL(ret, off_bit) == 0)
				break;

			/* If WDT can't be disabled, try to command and wait to see */
			core_config_ice_mode_write(wdt_addr, 0x00, 1);
			core_config_ice_mode_write(wdt_addr, 0x98, 1);
		}

		timeout--;
		mdelay(10);
	}

	if (timeout > 0) {
		if (enable) {
			ipio_info("WDT turn on succeed\n");
		} else {
			core_config_ice_mode_write(wdt_addr, 0, 1);
			ipio_info("WDT turn off succeed\n");
		}
	} else {
		ipio_err("WDT turn on/off timeout !, ret = %x\n", ret);
		core_config_read_pc_counter();
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(core_config_set_watch_dog);

int core_config_check_cdc_busy(int conut, int delay)
{
	int timer = conut, ret = -1;
	uint8_t cmd[2] = { 0 };
	uint8_t busy = 0, busy_byte = 0;

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_cdc_busy;

	if (core_fr->actual_fw_mode == protocol->demo_mode) {
		busy_byte = 0x41;
	} else if (core_fr->actual_fw_mode == protocol->test_mode) {
		busy_byte = 0x51;
	} else {
		ipio_err("Unknown fw mode (0x%x)\n", core_fr->actual_fw_mode);
		return -EINVAL;
	}

	ipio_debug(DEBUG_CONFIG, "busy byte = %x\n", busy_byte);

	while (timer > 0) {
		core_write(core_config->slave_i2c_addr, cmd, 2);
		core_write(core_config->slave_i2c_addr, &cmd[1], 1);
		core_read(core_config->slave_i2c_addr, &busy, 1);

		ipio_debug(DEBUG_CONFIG, "busy status = 0x%x\n", busy);

		if (busy == busy_byte) {
			ipio_info("Check busy is free\n");
			ret = 0;
			break;
		}
		timer--;
		mdelay(delay);
	}

	if (ret == -1) {
		ipio_err("Check busy (0x%x) timeout\n", busy);
		core_config_read_pc_counter();
	}

	return ret;
}
EXPORT_SYMBOL(core_config_check_cdc_busy);

int core_config_check_int_status(bool high)
{
	int timer = 1000, ret = -1;

	/* From FW request, timeout should at least be 5 sec */
	while (timer) {
		if (high) {
			if (gpio_get_value(ipd->int_gpio)) {
				ipio_info("Check busy is free\n");
				ret = 0;
				break;
			}
		} else {
			if (!gpio_get_value(ipd->int_gpio)) {
				ipio_info("Check busy is free\n");
				ret = 0;
				break;
			}
		}

		mdelay(5);
		timer--;
	}

	if (ret == -1) {
		ipio_err("Check INT timeout\n");
		core_config_read_pc_counter();
	}

	return ret;
}
EXPORT_SYMBOL(core_config_check_int_status);

int core_config_get_project_id(void)
{
	int ret = 0;
	int i = 0;
	uint8_t pid_data[5] = {0};
	if (mode_chose != SPI_MODE) {
		core_config_ice_mode_enable(STOP_MCU);

		/* Disable watch dog */
		core_config_set_watch_dog(false);

		core_config_ice_mode_write(0x041000, 0x0, 1);   /* CS low */
		core_config_ice_mode_write(0x041004, 0x66aa55, 3);  /* Key */

		core_config_ice_mode_write(0x041008, 0x03, 1);

		core_config_ice_mode_write(0x041008, (RESERVE_BLOCK_START_ADDR & 0xFF0000) >> 16, 1);
		core_config_ice_mode_write(0x041008, (RESERVE_BLOCK_START_ADDR & 0x00FF00) >> 8, 1);
		core_config_ice_mode_write(0x041008, (RESERVE_BLOCK_START_ADDR & 0x0000FF), 1);


		for (i = 0; i < ARRAY_SIZE(pid_data); i++) {
			core_config_ice_mode_write(0x041008, 0xFF, 1);
			pid_data[i] = core_config_read_write_onebyte(0x41010);
			ipio_info("project_id[%d] = 0x%x\n", i, pid_data[i]);
		}

		core_flash_dma_clear();
		core_config_ice_mode_write(0x041000, 0x1, 1);   /* CS high */
		core_config_ice_mode_disable();
	}
	return ret;
}
EXPORT_SYMBOL(core_config_get_project_id);

int core_config_get_panel_info(void)
{
	int ret = 0;
	uint8_t cmd[2] = { 0 };

	memset(g_read_buf, 0, sizeof(g_read_buf));

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_panel_info;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	ret = core_read(core_config->slave_i2c_addr, &g_read_buf[0], protocol->panel_info_len);
	if (ret < 0) {
		ipio_err("Failed to read data, %d\n", ret);
		goto out;
	}

	core_config->tp_info->res_width = (g_read_buf[1] << 8) | g_read_buf[2];
	core_config->tp_info->res_height = (g_read_buf[3] << 8) | g_read_buf[4];

out:
	set_res.width = TOUCH_SCREEN_X_MAX;
	set_res.height = TOUCH_SCREEN_Y_MAX;
	ipio_info("Panel info: width = %d, height = %d\n", set_res.width, set_res.height);
	return ret;
}
EXPORT_SYMBOL(core_config_get_panel_info);

int core_config_get_key_info(void)
{
	int ret = 0, i;
	uint8_t cmd[2] = { 0 };

	memset(g_read_buf, 0, sizeof(g_read_buf));

	if (mode_chose == SPI_MODE)
		return 0;

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_key_info;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_read(core_config->slave_i2c_addr, &g_read_buf[0], protocol->key_info_len);
	if (ret < 0) {
		ipio_err("Failed to read data, %d\n", ret);
		goto out;
	}

	if (core_config->tp_info->nKeyCount) {
		/* NOTE: Firmware not ready yet */
		core_config->tp_info->nKeyAreaXLength = (g_read_buf[0] << 8) + g_read_buf[1];
		core_config->tp_info->nKeyAreaYLength = (g_read_buf[2] << 8) + g_read_buf[3];

		ipio_info("key: length of X area = %x\n", core_config->tp_info->nKeyAreaXLength);
		ipio_info("key: length of Y area = %x\n", core_config->tp_info->nKeyAreaYLength);

		for (i = 0; i < core_config->tp_info->nKeyCount; i++) {
			core_config->tp_info->virtual_key[i].nId = g_read_buf[i * 5 + 4];
			core_config->tp_info->virtual_key[i].nX = (g_read_buf[i * 5 + 5] << 8) + g_read_buf[i * 5 + 6];
			core_config->tp_info->virtual_key[i].nY = (g_read_buf[i * 5 + 7] << 8) + g_read_buf[i * 5 + 8];
			core_config->tp_info->virtual_key[i].nStatus = 0;

			ipio_info("key: id = %d, X = %d, Y = %d\n", core_config->tp_info->virtual_key[i].nId,
				 core_config->tp_info->virtual_key[i].nX, core_config->tp_info->virtual_key[i].nY);
		}
	}

out:
	return ret;
}
EXPORT_SYMBOL(core_config_get_key_info);

int core_config_get_tp_info(void)
{
	int ret = 0;
	uint8_t cmd[2] = { 0 };

	memset(g_read_buf, 0, sizeof(g_read_buf));

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_tp_info;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_read(core_config->slave_i2c_addr, &g_read_buf[0], protocol->tp_info_len);
	if (ret < 0) {
		ipio_err("Failed to read data, %d\n", ret);
		goto out;
	}

	/* in protocol v5, ignore the first btye because of a header. */
	core_config->tp_info->nMinX = g_read_buf[1];
	core_config->tp_info->nMinY = g_read_buf[2];
	core_config->tp_info->nMaxX = (g_read_buf[4] << 8) + g_read_buf[3];
	core_config->tp_info->nMaxY = (g_read_buf[6] << 8) + g_read_buf[5];
	core_config->tp_info->nXChannelNum = g_read_buf[7];
	core_config->tp_info->nYChannelNum = g_read_buf[8];
	core_config->tp_info->self_tx_channel_num = g_read_buf[11];
	core_config->tp_info->self_rx_channel_num = g_read_buf[12];
	core_config->tp_info->side_touch_type = g_read_buf[13];
	core_config->tp_info->nMaxTouchNum = g_read_buf[9];
	core_config->tp_info->nKeyCount = g_read_buf[10];

	core_config->tp_info->nMaxKeyButtonNum = 5;

	ipio_info("minX = %d, minY = %d, maxX = %d, maxY = %d\n",
		 core_config->tp_info->nMinX, core_config->tp_info->nMinY,
		 core_config->tp_info->nMaxX, core_config->tp_info->nMaxY);
	ipio_info("xchannel = %d, ychannel = %d, self_tx = %d, self_rx = %d\n",
		 core_config->tp_info->nXChannelNum, core_config->tp_info->nYChannelNum,
		 core_config->tp_info->self_tx_channel_num, core_config->tp_info->self_rx_channel_num);
	ipio_info("side_touch_type = %d, max_touch_num= %d, touch_key_num = %d, max_key_num = %d\n",
		 core_config->tp_info->side_touch_type, core_config->tp_info->nMaxTouchNum,
		 core_config->tp_info->nKeyCount, core_config->tp_info->nMaxKeyButtonNum);

out:
	return ret;
}
EXPORT_SYMBOL(core_config_get_tp_info);

int core_config_get_protocol_ver(void)
{
	int ret = 0, i = 0;
	int major, mid, minor;
	uint8_t cmd[2] = { 0 };

	memset(g_read_buf, 0, sizeof(g_read_buf));
	memset(core_config->protocol_ver, 0x0, sizeof(core_config->protocol_ver));

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_pro_ver;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_read(core_config->slave_i2c_addr, &g_read_buf[0], protocol->pro_ver_len);
	if (ret < 0) {
		ipio_err("Failed to read data, %d\n", ret);
		goto out;
	}

	/* ignore the first btye because of a header. */
	for (; i < protocol->pro_ver_len - 1; i++) {
		core_config->protocol_ver[i] = g_read_buf[i + 1];
	}

	ipio_info("Procotol Version = %d.%d.%d\n",
		 core_config->protocol_ver[0], core_config->protocol_ver[1], core_config->protocol_ver[2]);

	major = core_config->protocol_ver[0];
	mid = core_config->protocol_ver[1];
	minor = core_config->protocol_ver[2];

	/* update protocol if they're different with the default ver set by driver */
	if (major != PROTOCOL_MAJOR || mid != PROTOCOL_MID || minor != PROTOCOL_MINOR) {
		ret = core_protocol_update_ver(major, mid, minor);
		if (ret < 0)
			ipio_err("Protocol version is invalid\n");
	}

out:
	return ret;
}
EXPORT_SYMBOL(core_config_get_protocol_ver);

int core_config_get_core_ver(void)
{
	int ret = 0, i = 0;
	uint8_t cmd[2] = { 0 };

	memset(g_read_buf, 0, sizeof(g_read_buf));

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_core_ver;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_read(core_config->slave_i2c_addr, &g_read_buf[0], protocol->core_ver_len);
	if (ret < 0) {
		ipio_err("Failed to read data, %d\n", ret);
		goto out;
	}

	for (; i < protocol->core_ver_len - 1; i++)
		core_config->core_ver[i] = g_read_buf[i + 1];

	/* in protocol v5, ignore the first btye because of a header. */
	ipio_info("Core Version = %d.%d.%d.%d\n",
		 core_config->core_ver[1], core_config->core_ver[2],
		 core_config->core_ver[3], core_config->core_ver[4]);

out:
	return ret;
}
EXPORT_SYMBOL(core_config_get_core_ver);

/*
 * Getting the version of firmware used on the current one.
 *
 */
int core_config_get_fw_ver(void)
{
	int ret = 0, i = 0;
	uint8_t cmd[2] = { 0 };

	memset(g_read_buf, 0, sizeof(g_read_buf));

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_fw_ver;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_read(core_config->slave_i2c_addr, &g_read_buf[0], protocol->fw_ver_len);
	if (ret < 0) {
		ipio_err("Failed to read fw version %d\n", ret);
		goto out;
	}

	for (; i < protocol->fw_ver_len; i++)
		core_config->firmware_ver[i] = g_read_buf[i];

	if (protocol->mid >= 0x3) {
		ipio_info("Firmware Version = %d.%d.%d.%d\n", core_config->firmware_ver[1], core_config->firmware_ver[2], core_config->firmware_ver[3], core_config->firmware_ver[4]);
	} else {
		ipio_info("Firmware Version = %d.%d.%d\n",
			core_config->firmware_ver[1], core_config->firmware_ver[2], core_config->firmware_ver[3]);
	}
out:
	return ret;
}
EXPORT_SYMBOL(core_config_get_fw_ver);

static uint32_t core_config_rd_pack(int packet)
{
	int retry = 100;
	uint32_t reg_data = 0;
	while (retry--) {
		reg_data = core_config_read_write_onebyte(0x73010);
		if ((reg_data & 0x02) == 0) {
			ipio_info("check  ok 0x73010 read 0x%X retry = %d\n", reg_data, retry);
			break;
		}
		mdelay(10);
	}
	if (retry <= 0) {
		ipio_info("check 0x73010 error read 0x%X\n", reg_data);
	}
	core_config_ice_mode_write(0x73000, packet, 4);

	retry = 100;
	while (retry--) {
		reg_data = core_config_read_write_onebyte(0x4800A);
		if ((reg_data & 0x02) == 0x02) {
			ipio_info("check  ok 0x4800A read 0x%X retry = %d\n", reg_data, retry);
			break;
		}
		mdelay(10);
	}
	if (retry <= 0) {
		ipio_info("check 0x4800A error read 0x%X\n", reg_data);
	}
	core_config_ice_mode_write(0x4800A, 0x02, 1);
	reg_data = core_config_ice_mode_read(0x73016);
	return reg_data;
}

static void core_config_wr_pack(int packet)
{
	int retry = 100;
	uint32_t reg_data = 0;
	while (retry--) {
		reg_data = core_config_read_write_onebyte(0x73010);
		if ((reg_data & 0x02) == 0) {
			ipio_info("check ok 0x73010 read 0x%X retry = %d\n", reg_data, retry);
			break;
		}
		mdelay(10);
	}
	if (retry <= 0) {
		ipio_info("check 0x73010 error read 0x%X\n", reg_data);
	}
	core_config_ice_mode_write(0x73000, packet, 4);
}

void core_config_protect_otp_prog_mode(int mode)
{
	int prog_mode, prog_done, retry = 5;

	if (core_config_ice_mode_enable(STOP_MCU) < 0) {
		ipio_err("enter ice mode failed in otp\n");
		return;
	}

	do {
		core_config_ice_mode_write(0x43008, 0x80, 1);
		core_config_ice_mode_write(0x43030, 0x0, 1);
		core_config_ice_mode_write(0x4300C, 0x4, 1);

		mdelay(1);

		core_config_ice_mode_write(0x4300C, 0x4, 1);

		prog_done = core_config_ice_mode_read(0x43030);
		prog_mode = core_config_ice_mode_read(0x43008);
		ipio_info("otp prog_mode = 0x%x, prog_done = 0x%x\n", prog_mode, prog_done);

		if (prog_done == 0x0 && prog_mode == 0x80)
			break;
	} while (--retry > 0);

	if (retry <= 0)
		ipio_err("OTP Program mode error!\n");
}

void core_set_ddi_register_onlyone(uint8_t page, uint8_t reg, uint8_t data)
{
	uint32_t setpage = 0x1FFFFF00 | page;
	uint32_t setreg = 0x1F000100 | (reg << 16) | data;
	ipio_info("setpage =  0x%X setreg = 0x%X\n", setpage, setreg);
	/*TDI_WR_KEY*/
	core_config_wr_pack(0x1FFF9527);
	/*Switch to Page*/
	core_config_wr_pack(setpage);
	/* Page*/
	core_config_wr_pack(setreg);
	/*TDI_WR_KEY OFF*/
	core_config_wr_pack(0x1FFF9500);
}


void core_get_ddi_register_onlyone(uint8_t page, uint8_t reg)
{
	uint32_t reg_data = 0;
	uint32_t setpage = 0x1FFFFF00 | page;
	uint32_t setreg = 0x2F000100 | (reg << 16);
	ipio_info("setpage =  0x%X setreg = 0x%X\n", setpage, setreg);

	/*TDI_WR_KEY*/
	core_config_wr_pack(0x1FFF9527);
	/*Set Read Page reg*/
	core_config_wr_pack(setpage);

	/*TDI_RD_KEY*/
	core_config_wr_pack(0x1FFF9487);
	/*( *( __IO uint8 *)	(0x4800A) ) =0x2*/
	core_config_ice_mode_write(0x4800A, 0x02, 1);

	reg_data = core_config_rd_pack(setreg);
	ipio_info("check page = 0x%X reg = 0x%X read 0x%X\n", page, reg, reg_data);

	/*TDI_RD_KEY OFF*/
	core_config_wr_pack(0x1FFF9400);
	/*TDI_WR_KEY OFF*/
	core_config_wr_pack(0x1FFF9500);
}

uint32_t core_config_get_reg_data(uint32_t addr)
{
	int res = 0;
	uint32_t reg_data = 0;
	bool ice_enable = core_config->icemodeenable;
	if (!ice_enable) {
		res = core_config_ice_mode_enable(NO_STOP_MCU);
		if (res < 0) {
			ipio_info("Failed to enter ICE mode, res = %d\n", res);
			goto out;
		}
		mdelay(1);
	}
	reg_data = core_config_ice_mode_read(addr);
	ipio_info("ice_enable = %d addr = 0x%X reg_data = 0x%X\n", ice_enable, addr, reg_data);

out:
	if (!ice_enable) {
		core_config_ice_mode_disable();
	}
	return reg_data;
}

int core_config_get_chip_id(void)
{
	int ret = 0;
	uint32_t pid = 0, OTPIDData = 0, ANAIDData = 0;

	ret = core_config_ice_mode_enable(STOP_MCU);
	if (ret < 0) {
		ipio_err("Failed to enter ICE mode, ret = %d\n", ret);
		return ret;
	}

	pid = core_config_ice_mode_read(core_config->pid_addr);
	OTPIDData = core_config_ice_mode_read(core_config->otp_id_addr);
	ANAIDData = core_config_ice_mode_read(core_config->ana_id_addr);

	core_config->chip_pid = pid;
	core_config->chip_id = pid >> 16;
	core_config->chip_type = (pid & 0x0000FF00) >> 8;
	core_config->core_type = pid & 0xFF;
	core_config->chip_otp_id = OTPIDData & 0xFF;
	core_config->chip_ana_id = ANAIDData & 0xFF;

	ipio_info("Chip PID = 0x%x\n", core_config->chip_pid);
	ipio_info("Chip ID = 0x%x\n", core_config->chip_id);
	ipio_info("Chip Type = 0x%x\n", core_config->chip_type);
	ipio_info("Chip Core id = 0x%x\n", core_config->core_type);
	ipio_info("OTP ID = 0x%x\n", core_config->chip_otp_id);
	ipio_info("ANA ID = 0x%x\n", core_config->chip_ana_id);

	core_config_ice_mode_disable();
	return ret;
}
EXPORT_SYMBOL(core_config_get_chip_id);

int core_edge_plam_ctrl(int type)
{
	int ret = 0;
	uint8_t cmd[4] = { 0 };

	ipio_info("edge plam ctrl ,Type = %d\n", type);
	memset(g_read_buf, 0, sizeof(g_read_buf));

	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->edge_plam_ctl[0];
	cmd[2] = protocol->edge_plam_ctl[1];
	cmd[3] = type;

	ret = core_write(core_config->slave_i2c_addr, cmd, 4);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 3);
	if (ret < 0) {
		ipio_err("Failed to write data, %d\n", ret);
		goto out;
	}
out:
	return ret;
}

int core_config_init(void)
{
	core_config = devm_kzalloc(ipd->dev, sizeof(struct core_config_data) * sizeof(uint8_t) * 6, GFP_KERNEL);
	if (ERR_ALLOC_MEM(core_config)) {
		ipio_err("Failed to allocate core_config mem, %ld\n", PTR_ERR(core_config));
		return -ENOMEM;
	}

	core_config->tp_info = devm_kzalloc(ipd->dev, sizeof(TP_INFO), GFP_KERNEL);
	if (ERR_ALLOC_MEM(core_config->tp_info)) {
		ipio_err("Failed to allocate core_config->tp_info mem, %ld\n", PTR_ERR(core_config->tp_info));
		return -ENOMEM;
	}

	core_config->slave_i2c_addr = ILITEK_I2C_ADDR;
	core_config->chip_type = 0x0000;
#ifdef GESTURE_ENABLE
	core_config->isEnableGesture = true;
#else
	core_config->isEnableGesture = false;
#endif
	core_config->ice_mode_addr = ICE_MODE_ADDR;
	core_config->pid_addr = PID_ADDR;
	core_config->otp_id_addr = OTP_ID_ADDR;
	core_config->ana_id_addr = ANA_ID_ADDR;
	core_config->wdt_addr = WDT_ADDR;
	core_config->ic_reset_addr = CHIP_RESET_ADDR;

	return 0;
}
EXPORT_SYMBOL(core_config_init);
