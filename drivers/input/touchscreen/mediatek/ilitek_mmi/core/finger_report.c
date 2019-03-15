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
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/i2c.h>
#include <linux/list.h>

#include "../common.h"
#include "../platform.h"
#include "config.h"
#include "i2c.h"
#include "finger_report.h"
#include "gesture.h"
#include "mp_test.h"
#include "protocol.h"

/* An id with position in each fingers */
struct mutual_touch_point {
	uint16_t id;
	uint16_t x;
	uint16_t y;
	uint16_t pressure;
};

/* Keys and code with each fingers */
struct mutual_touch_info {
	uint8_t touch_num;
	uint8_t key_code;
	struct mutual_touch_point mtp[10];
};

/* Store the packet of finger report */
struct fr_data_node {
	uint8_t *data;
	uint16_t len;
};

/* record the status of touch being pressed or released currently and previosuly */
uint8_t g_current_touch[MAX_TOUCH_NUM];
uint8_t g_previous_touch[MAX_TOUCH_NUM];

/* the total length of finger report packet */
uint16_t g_total_len;

struct mutual_touch_info g_mutual_data;
struct fr_data_node *g_fr_node = NULL, *g_fr_uart = NULL;
struct core_fr_data *core_fr;

/**
 * Calculate the check sum of each packet reported by firmware
 *
 * @pMsg: packet come from firmware
 * @nLength : the length of its packet
 */
uint8_t core_fr_calc_checksum(uint8_t *pMsg, uint32_t nLength)
{
	int i;
	int32_t nCheckSum = 0;

	for (i = 0; i < nLength; i++) {
		nCheckSum += pMsg[i];
	}

	return (uint8_t) ((-nCheckSum) & 0xFF);
}
EXPORT_SYMBOL(core_fr_calc_checksum);

/**
 *  Receive data when fw mode stays at i2cuart mode.
 *
 *  the first is to receive N bytes depending on the mode that firmware stays
 *  before going in this function, and it would check with i2c buffer if it
 *  remains the rest of data.
 */
static void i2cuart_recv_packet(void)
{
	int ret = 0, need_read_len = 0, one_data_bytes = 0;
	int type = g_fr_node->data[3] & 0x0F;
	int actual_len = g_fr_node->len - 5;

	ipio_debug(DEBUG_FINGER_REPORT, "pid = %x, data[3] = %x, type = %x, actual_len = %d\n",
	    g_fr_node->data[0], g_fr_node->data[3], type, actual_len);

	need_read_len = g_fr_node->data[1] * g_fr_node->data[2];

	if (type == 0 || type == 1 || type == 6) {
		one_data_bytes = 1;
	} else if (type == 2 || type == 3) {
		one_data_bytes = 2;
	} else if (type == 4 || type == 5) {
		one_data_bytes = 4;
	}

	ipio_debug(DEBUG_FINGER_REPORT, "need_read_len = %d  one_data_bytes = %d\n", need_read_len, one_data_bytes);

	need_read_len = need_read_len * one_data_bytes + 1;

	if (need_read_len > actual_len) {
		g_fr_uart = kmalloc(sizeof(struct fr_data_node), GFP_ATOMIC);
		if (ERR_ALLOC_MEM(g_fr_uart)) {
			ipio_err("Failed to allocate g_fr_uart memory %ld\n", PTR_ERR(g_fr_uart));
			return;
		}

		g_fr_uart->len = need_read_len - actual_len;
		g_fr_uart->data = kzalloc(g_fr_uart->len, GFP_ATOMIC);
		if (ERR_ALLOC_MEM(g_fr_uart->data)) {
			ipio_err("Failed to allocate g_fr_uart memory %ld\n", PTR_ERR(g_fr_uart->data));
			return;
		}

		g_total_len += g_fr_uart->len;
		ret = core_read(core_config->slave_i2c_addr, g_fr_uart->data, g_fr_uart->len);
		if (ret < 0)
			ipio_err("Failed to read finger report packet\n");
	}
}

/*
 * It'd be called when a finger's touching down a screen. It'll notify the event
 * to the uplayer from input device.
 *
 * @x: the axis of X
 * @y: the axis of Y
 * @pressure: the value of pressue on a screen
 * @id: an id represents a finger pressing on a screen
 */
void core_fr_touch_press(int32_t x, int32_t y, uint32_t pressure, int32_t id)
{
	ipio_debug(DEBUG_FINGER_REPORT, "DOWN: id = %d, x = %d, y = %d\n", id, x, y);

#ifdef MT_B_TYPE
	input_mt_slot(core_fr->input_device, id);
	input_mt_report_slot_state(core_fr->input_device, MT_TOOL_FINGER, true);
	input_report_abs(core_fr->input_device, ABS_MT_POSITION_X, x);
	input_report_abs(core_fr->input_device, ABS_MT_POSITION_Y, y);

	if (core_fr->isEnablePressure)
		input_report_abs(core_fr->input_device, ABS_MT_PRESSURE, pressure);
#else
	input_report_key(core_fr->input_device, BTN_TOUCH, 1);

	input_report_abs(core_fr->input_device, ABS_MT_TRACKING_ID, id);
	input_report_abs(core_fr->input_device, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(core_fr->input_device, ABS_MT_WIDTH_MAJOR, 1);
	input_report_abs(core_fr->input_device, ABS_MT_POSITION_X, x);
	input_report_abs(core_fr->input_device, ABS_MT_POSITION_Y, y);

	if (core_fr->isEnablePressure)
		input_report_abs(core_fr->input_device, ABS_MT_PRESSURE, pressure);

	input_mt_sync(core_fr->input_device);
#endif /* MT_B_TYPE */
}
EXPORT_SYMBOL(core_fr_touch_press);

/*
 * It'd be called when a finger's touched up from a screen. It'll notify
 * the event to the uplayer from input device.
 *
 * @x: the axis of X
 * @y: the axis of Y
 * @id: an id represents a finger leaving from a screen.
 */
void core_fr_touch_release(int32_t x, int32_t y, int32_t id)
{
	ipio_debug(DEBUG_FINGER_REPORT, "UP: id = %d, x = %d, y = %d\n", id, x, y);

#ifdef MT_B_TYPE
	input_mt_slot(core_fr->input_device, id);
	input_mt_report_slot_state(core_fr->input_device, MT_TOOL_FINGER, false);
#else
	input_report_key(core_fr->input_device, BTN_TOUCH, 0);
	input_mt_sync(core_fr->input_device);
#endif /* MT_B_TYPE */
}
EXPORT_SYMBOL(core_fr_touch_release);

static int parse_report_data(uint8_t pid)
{
	int i, ret = 0;
	uint8_t check_sum = 0;
	uint32_t nX = 0, nY = 0;

	dump_data(g_fr_node->data, 8, g_fr_node->len, 0, "touch report");

	check_sum = core_fr_calc_checksum(&g_fr_node->data[0], (g_fr_node->len - 1));
	ipio_debug(DEBUG_FINGER_REPORT, "data = %x;  check_sum : %x\n", g_fr_node->data[g_fr_node->len - 1], check_sum);

	if (g_fr_node->data[g_fr_node->len - 1] != check_sum) {
		ipio_err("Wrong checksum\n");
		ret = -1;
		goto out;
	}

	/* start to parsing the packet of finger report */
	if (pid == protocol->demo_pid) {
		ipio_debug(DEBUG_FINGER_REPORT, " **** Parsing DEMO packets : 0x%x ****\n", pid);

		for (i = 0; i < MAX_TOUCH_NUM; i++) {
			if ((g_fr_node->data[(4 * i) + 1] == 0xFF) && (g_fr_node->data[(4 * i) + 2] == 0xFF)
			    && (g_fr_node->data[(4 * i) + 3] == 0xFF)) {
#ifdef MT_B_TYPE
				g_current_touch[i] = 0;
#endif
				continue;
			}

			nX = (((g_fr_node->data[(4 * i) + 1] & 0xF0) << 4) | (g_fr_node->data[(4 * i) + 2]));
			nY = (((g_fr_node->data[(4 * i) + 1] & 0x0F) << 8) | (g_fr_node->data[(4 * i) + 3]));

			if (core_fr->isSetResolution) {
				g_mutual_data.mtp[g_mutual_data.touch_num].x = nX;
				g_mutual_data.mtp[g_mutual_data.touch_num].y = nY;
				g_mutual_data.mtp[g_mutual_data.touch_num].id = i;
			} else {
				g_mutual_data.mtp[g_mutual_data.touch_num].x = TOUCH_SCREEN_X_MAX - nX * set_res.width / TPD_WIDTH;
				g_mutual_data.mtp[g_mutual_data.touch_num].y = nY * set_res.height / TPD_HEIGHT;
				g_mutual_data.mtp[g_mutual_data.touch_num].id = i;
			}

			if (core_fr->isEnablePressure)
				g_mutual_data.mtp[g_mutual_data.touch_num].pressure = g_fr_node->data[(4 * i) + 4];
			else
				g_mutual_data.mtp[g_mutual_data.touch_num].pressure = 1;

			ipio_debug(DEBUG_FINGER_REPORT, "[x,y]=[%d,%d]\n", nX, nY);
			ipio_debug(DEBUG_FINGER_REPORT, "point[%d] : (%d,%d) = %d\n",
			    g_mutual_data.mtp[g_mutual_data.touch_num].id,
			    g_mutual_data.mtp[g_mutual_data.touch_num].x,
			    g_mutual_data.mtp[g_mutual_data.touch_num].y, g_mutual_data.mtp[g_mutual_data.touch_num].pressure);

			g_mutual_data.touch_num++;

#ifdef MT_B_TYPE
			g_current_touch[i] = 1;
#endif
		}
	} else if (pid == protocol->debug_pid) {
		ipio_debug(DEBUG_FINGER_REPORT, " **** Parsing DEBUG packets : 0x%x ****\n", pid);
		ipio_debug(DEBUG_FINGER_REPORT, "Length = %d\n", (g_fr_node->data[1] << 8 | g_fr_node->data[2]));

		for (i = 0; i < MAX_TOUCH_NUM; i++) {
			if ((g_fr_node->data[(3 * i) + 5] == 0xFF) && (g_fr_node->data[(3 * i) + 6] == 0xFF)
			    && (g_fr_node->data[(3 * i) + 7] == 0xFF)) {
#ifdef MT_B_TYPE
				g_current_touch[i] = 0;
#endif
				continue;
			}

			nX = (((g_fr_node->data[(3 * i) + 5] & 0xF0) << 4) | (g_fr_node->data[(3 * i) + 6]));
			nY = (((g_fr_node->data[(3 * i) + 5] & 0x0F) << 8) | (g_fr_node->data[(3 * i) + 7]));

			if (core_fr->isSetResolution) {
				g_mutual_data.mtp[g_mutual_data.touch_num].x = nX;
				g_mutual_data.mtp[g_mutual_data.touch_num].y = nY;
				g_mutual_data.mtp[g_mutual_data.touch_num].id = i;
			} else {
				g_mutual_data.mtp[g_mutual_data.touch_num].x = TOUCH_SCREEN_X_MAX - nX * set_res.width / TPD_WIDTH;
				g_mutual_data.mtp[g_mutual_data.touch_num].y = nY * set_res.height / TPD_HEIGHT;
				g_mutual_data.mtp[g_mutual_data.touch_num].id = i;
			}

			if (core_fr->isEnablePressure)
				g_mutual_data.mtp[g_mutual_data.touch_num].pressure = g_fr_node->data[(4 * i) + 4];
			else
				g_mutual_data.mtp[g_mutual_data.touch_num].pressure = 1;

			ipio_debug(DEBUG_FINGER_REPORT, "[x,y]=[%d,%d]\n", nX, nY);
			ipio_debug(DEBUG_FINGER_REPORT, "point[%d] : (%d,%d) = %d\n",
			    g_mutual_data.mtp[g_mutual_data.touch_num].id,
			    g_mutual_data.mtp[g_mutual_data.touch_num].x,
			    g_mutual_data.mtp[g_mutual_data.touch_num].y, g_mutual_data.mtp[g_mutual_data.touch_num].pressure);

			g_mutual_data.touch_num++;

#ifdef MT_B_TYPE
			g_current_touch[i] = 1;
#endif
		}
	} else {
		if (pid != 0) {
			/* ignore the pid with 0x0 after enable irq at once */
			ipio_err(" **** Unknown PID : 0x%x ****\n", pid);
			ret = -1;
		}
	}

out:
	return ret;
}

static int do_report_handle(void)
{
	int i, gesture, ret = 0;
	static int last_touch;
	uint8_t pid = 0x0;

	memset(&g_mutual_data, 0x0, sizeof(struct mutual_touch_info));

#ifdef I2C_SEGMENT
	ret = core_i2c_segmental_read(core_config->slave_i2c_addr, g_fr_node->data, g_fr_node->len);
#else
	ret = core_read(core_config->slave_i2c_addr, g_fr_node->data, g_fr_node->len);
#endif

	if (ret < 0) {
		ipio_err("Failed to read finger report packet\n");
#ifdef HOST_DOWNLOAD
		if (ret == CHECK_RECOVER) {
			ipio_err("Doing host download recovery !\n");
			ret = ilitek_platform_reset_ctrl(true, HW_RST_HOST_DOWNLOAD);
			if (ret < 0)
				ipio_info("host download failed!\n");
		}
#endif
		goto out;
	}

	pid = g_fr_node->data[0];
	ipio_debug(DEBUG_FINGER_REPORT, "PID = 0x%x\n", pid);

	if (pid == protocol->i2cuart_pid) {
		ipio_debug(DEBUG_FINGER_REPORT, "I2CUART(0x%x): prepare to receive rest of data\n", pid);
		i2cuart_recv_packet();
		goto out;
	}

	if (pid == protocol->ges_pid && core_config->isEnableGesture) {
		ipio_debug(DEBUG_FINGER_REPORT, "pid = 0x%x, code = %x\n", pid, g_fr_node->data[1]);
		gesture = core_gesture_match_key(g_fr_node->data[1]);
		if (gesture != -1) {
			input_report_key(core_fr->input_device, gesture, 1);
			input_sync(core_fr->input_device);
			input_report_key(core_fr->input_device, gesture, 0);
			input_sync(core_fr->input_device);
		}
		goto out;
	}

	ret = parse_report_data(pid);
	if (ret < 0) {
		ipio_err("Failed to parse packet of finger touch\n");
		goto out;
	}

	ipio_debug(DEBUG_FINGER_REPORT, "Touch Num = %d, LastTouch = %d\n", g_mutual_data.touch_num, last_touch);

	/* interpret parsed packat and send input events to system */
	if (g_mutual_data.touch_num > 0) {
#ifdef MT_B_TYPE
		for (i = 0; i < g_mutual_data.touch_num; i++) {
			input_report_key(core_fr->input_device, BTN_TOUCH, 1);
			core_fr_touch_press(g_mutual_data.mtp[i].x, g_mutual_data.mtp[i].y, g_mutual_data.mtp[i].pressure, g_mutual_data.mtp[i].id);

			input_report_key(core_fr->input_device, BTN_TOOL_FINGER, 1);
		}

		for (i = 0; i < MAX_TOUCH_NUM; i++) {
			ipio_debug(DEBUG_FINGER_REPORT, "g_previous_touch[%d]=%d, g_current_touch[%d]=%d\n", i,
			    g_previous_touch[i], i, g_current_touch[i]);

			if (g_current_touch[i] == 0 && g_previous_touch[i] == 1) {
				core_fr_touch_release(0, 0, i);
			}

			g_previous_touch[i] = g_current_touch[i];
		}
#else
		for (i = 0; i < g_mutual_data.touch_num; i++) {
			core_fr_touch_press(g_mutual_data.mtp[i].x, g_mutual_data.mtp[i].y, g_mutual_data.mtp[i].pressure, g_mutual_data.mtp[i].id);
		}
#endif
		input_sync(core_fr->input_device);

		last_touch = g_mutual_data.touch_num;
	} else {
		if (last_touch > 0) {
#ifdef MT_B_TYPE
			for (i = 0; i < MAX_TOUCH_NUM; i++) {
				ipio_debug(DEBUG_FINGER_REPORT, "g_previous_touch[%d]=%d, g_current_touch[%d]=%d\n", i,
				    g_previous_touch[i], i, g_current_touch[i]);

				if (g_current_touch[i] == 0 && g_previous_touch[i] == 1) {
					core_fr_touch_release(0, 0, i);
				}
				g_previous_touch[i] = g_current_touch[i];
			}
			input_report_key(core_fr->input_device, BTN_TOUCH, 0);
			input_report_key(core_fr->input_device, BTN_TOOL_FINGER, 0);
#else
			core_fr_touch_release(0, 0, 0);
#endif

			input_sync(core_fr->input_device);

			last_touch = 0;
		}
	}

out:
	return ret;
}

/**
 * Calculate the length with different modes according to the format of protocol 5.0
 *
 * We compute the length before receiving its packet. If the length is differnet between
 * firmware and the number we calculated, in this case I just print an error to inform users
 * and still send up to users.
 */
static uint16_t calc_packet_length(void)
{
	uint16_t xch = 0, ych = 0, stx = 0, srx = 0;
	/* FIXME: self_key not defined by firmware yet */
	uint16_t self_key = 2;
	uint16_t rlen = 0;

	if (protocol->major != 0x5) {
		ipio_err("doesn't support this version of protocol");
		return -1;
	}

	if (!ERR_ALLOC_MEM(core_config->tp_info)) {
		xch = core_config->tp_info->nXChannelNum;
		ych = core_config->tp_info->nYChannelNum;
		stx = core_config->tp_info->self_tx_channel_num;
		srx = core_config->tp_info->self_rx_channel_num;
	}

	ipio_debug(DEBUG_FINGER_REPORT, "firmware mode : 0x%x\n", core_fr->actual_fw_mode);

	switch (core_fr->actual_fw_mode) {
	case P5_0_FIRMWARE_DEMO_MODE:
		rlen = protocol->demo_len;
		break;
	case P5_0_FIRMWARE_DEBUG_MODE:
		rlen = (2 * xch * ych) + (stx * 2) + (srx * 2) + 2 * self_key + (8 * 2) + 1;
		rlen += 35;
		break;
	case P5_0_FIRMWARE_TEST_MODE:
		rlen = (2 * xch * ych) + (stx * 2) + (srx * 2) + 2 * self_key + 1;
		rlen += 1;
		break;
	case P5_0_FIRMWARE_GESTURE_MODE:
		if (core_gesture->mode == GESTURE_NORMAL_MODE)
			rlen = GESTURE_MORMAL_LENGTH;
		else
			rlen = GESTURE_INFO_LENGTH;
		break;
	default:
		ipio_err("Unknown firmware mode : %d\n", core_fr->actual_fw_mode);
		rlen = 0;
		break;
	}

	ipio_debug(DEBUG_FINGER_REPORT, "packet len = %d\n", rlen);
	return rlen;
}

void core_fr_handler(void)
{
	uint8_t *tdata = NULL;

	if (!core_fr->isEnableFR) {
		ipio_err("Figner report was disabled, do nothing\n");
		return;
	}

	if (atomic_read(&ipd->do_reset)) {
		ipio_err("IC is resetting, do nothing\n");
		return;
	}

	if (ipd->isEnablePollCheckPower) {
		mutex_lock(&ipd->plat_mutex);
		cancel_delayed_work_sync(&ipd->check_power_status_work);
		mutex_unlock(&ipd->plat_mutex);
	}
	if (ipd->isEnablePollCheckEsd) {
		mutex_lock(&ipd->plat_mutex);
		cancel_delayed_work_sync(&ipd->check_esd_status_work);
		mutex_unlock(&ipd->plat_mutex);
	}

	g_total_len = calc_packet_length();

	if (g_total_len <= 0) {
		ipio_err("Wrong the length of packet (%d)\n", g_total_len);
		goto out;;
	}

	g_fr_node = kmalloc(sizeof(struct fr_data_node), GFP_ATOMIC);
	if (ERR_ALLOC_MEM(g_fr_node)) {
		ipio_err("Failed to allocate g_fr_node memory %ld\n", PTR_ERR(g_fr_node));
		goto out;
	}

	g_fr_node->data = kcalloc(g_total_len, sizeof(uint8_t), GFP_ATOMIC);
	if (ERR_ALLOC_MEM(g_fr_node->data)) {
		ipio_err("Failed to allocate g_fr_node memory %ld\n", PTR_ERR(g_fr_node->data));
		goto out;
	}

	g_fr_node->len = g_total_len;
	memset(g_fr_node->data, 0xFF, (uint8_t) sizeof(uint8_t) * g_total_len);

	/* Handle report data */
	mutex_lock(&ipd->plat_mutex);
	do_report_handle();
	mutex_unlock(&ipd->plat_mutex);

	/* 2048 is referred to the defination by user */
	if (g_total_len > 2048) {
		ipio_err("total length (%d) is too long than user can handle\n",
			g_total_len);
		goto out;
	}

	tdata = kmalloc(g_total_len, GFP_ATOMIC);
	if (ERR_ALLOC_MEM(tdata)) {
		ipio_err("Failed to allocate g_fr_node memory %ld\n",
			PTR_ERR(tdata));
		goto out;
	}

	/* merge i2cuart data if it's at i2cuart mode */
	ipio_memcpy(tdata, g_fr_node->data, g_fr_node->len, g_total_len);
	if (g_fr_uart != NULL)
		ipio_memcpy(tdata + g_fr_node->len, g_fr_uart->data, g_fr_uart->len, g_total_len);

	/* Send data by netlink for apk */
	if (core_fr->isEnableNetlink)
		netlink_reply_msg(tdata, g_total_len);

	/* Send data to the device node of proc_debug_message_fops*/
	if (ipd->debug_node_open) {
		mutex_lock(&ipd->ilitek_debug_mutex);
		memset(ipd->debug_buf[ipd->debug_data_frame], 0x00,
				(uint8_t) sizeof(uint8_t) * 2048);
		ipio_memcpy(ipd->debug_buf[ipd->debug_data_frame], tdata, g_total_len, 2048);
		ipd->debug_data_frame++;
		if (ipd->debug_data_frame > 1) {
			ipio_info("ipd->debug_data_frame = %d\n", ipd->debug_data_frame);
		}
		if (ipd->debug_data_frame > 1023) {
			ipio_err("ipd->debug_data_frame = %d > 1024\n",
				ipd->debug_data_frame);
			ipd->debug_data_frame = 1023;
		}
		mutex_unlock(&ipd->ilitek_debug_mutex);
		wake_up(&(ipd->inq));
	}

	/* Send data to the device node of proc_get_debug_mode_data_fops */
	if (ipd->debug_data_start_flag && (ipd->debug_data_frame < 1024)) {
		mutex_lock(&ipd->ilitek_debug_mutex);
		memset(ipd->debug_buf[ipd->debug_data_frame], 0x00,
				(uint8_t) sizeof(uint8_t) * 2048);
		ipio_memcpy(ipd->debug_buf[ipd->debug_data_frame], tdata, g_total_len, 2048);

		ipd->debug_data_frame++;

		mutex_unlock(&ipd->ilitek_debug_mutex);
		wake_up(&(ipd->inq));
	}

out:
	ipio_kfree((void **)&tdata);

	if (g_fr_node != NULL) {
		ipio_kfree((void **)&g_fr_node->data);
		ipio_kfree((void **)&g_fr_node);
	}

	if (g_fr_uart != NULL) {
		ipio_kfree((void **)&g_fr_uart->data);
		ipio_kfree((void **)&g_fr_uart);
	}

	if (ipd->isEnablePollCheckPower) {
		mutex_lock(&ipd->plat_mutex);
		queue_delayed_work(ipd->check_power_status_queue,
				&ipd->check_power_status_work, ipd->work_delay);
		mutex_unlock(&ipd->plat_mutex);
	}

	if (ipd->isEnablePollCheckEsd) {
		mutex_lock(&ipd->plat_mutex);
		queue_delayed_work(ipd->check_esd_status_queue,
			&ipd->check_esd_status_work, ipd->esd_check_time);
		mutex_unlock(&ipd->plat_mutex);
	}

	ipio_debug(DEBUG_IRQ, "handle INT done\n\n");
}
EXPORT_SYMBOL(core_fr_handler);

void core_fr_input_set_param(struct input_dev *input_device)
{
	int max_x = 0, max_y = 0, min_x = 0, min_y = 0;
	int max_tp = 0;

	core_fr->input_device = input_device;

	/* set the supported event type for input device */
	set_bit(EV_ABS, core_fr->input_device->evbit);
	set_bit(EV_SYN, core_fr->input_device->evbit);
	set_bit(EV_KEY, core_fr->input_device->evbit);
	set_bit(BTN_TOUCH, core_fr->input_device->keybit);
	set_bit(BTN_TOOL_FINGER, core_fr->input_device->keybit);
	set_bit(INPUT_PROP_DIRECT, core_fr->input_device->propbit);

	if (core_fr->isSetResolution) {
		max_x = core_config->tp_info->nMaxX;
		max_y = core_config->tp_info->nMaxY;
		min_x = core_config->tp_info->nMinX;
		min_y = core_config->tp_info->nMinY;
		max_tp = core_config->tp_info->nMaxTouchNum;
	} else {
		max_x = set_res.width;
		max_y = set_res.height;
		min_x = TOUCH_SCREEN_X_MIN;
		min_y = TOUCH_SCREEN_Y_MIN;
		max_tp = MAX_TOUCH_NUM;
	}

	ipio_info("input resolution : max_x = %d, max_y = %d, min_x = %d, min_y = %d\n", max_x, max_y, min_x, min_y);
	ipio_info("input touch number: max_tp = %d\n", max_tp);

#if (TP_PLATFORM != PT_MTK)
	input_set_abs_params(core_fr->input_device, ABS_MT_POSITION_X, min_x, max_x - 1, 0, 0);
	input_set_abs_params(core_fr->input_device, ABS_MT_POSITION_Y, min_y, max_y - 1, 0, 0);

	input_set_abs_params(core_fr->input_device, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(core_fr->input_device, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#endif /* PT_MTK */

	if (core_fr->isEnablePressure)
		input_set_abs_params(core_fr->input_device, ABS_MT_PRESSURE, 0, 255, 0, 0);

#ifdef MT_B_TYPE
	#if KERNEL_VERSION(3, 7, 0) <= LINUX_VERSION_CODE
	input_mt_init_slots(core_fr->input_device, max_tp, INPUT_MT_DIRECT);
	#else
	input_mt_init_slots(core_fr->input_device, max_tp);
	#endif /* LINUX_VERSION_CODE */
#else
	input_set_abs_params(core_fr->input_device, ABS_MT_TRACKING_ID, 0, max_tp, 0, 0);
#endif /* MT_B_TYPE */

	/* Set up virtual key with gesture code */
	core_gesture_set_key(core_fr);
}
EXPORT_SYMBOL(core_fr_input_set_param);

int core_fr_init(void)
{
	core_fr = devm_kzalloc(ipd->dev, sizeof(struct core_fr_data), GFP_KERNEL);
	if (ERR_ALLOC_MEM(core_fr)) {
		ipio_err("Failed to allocate core_fr mem, %ld\n", PTR_ERR(core_fr));
		return -ENOMEM;
	}

	core_fr->isEnableFR = true;
	core_fr->isEnableNetlink = false;
	core_fr->isEnablePressure = false;
	core_fr->isSetResolution = false;
	core_fr->actual_fw_mode = protocol->demo_mode;

	return 0;
}
EXPORT_SYMBOL(core_fr_init);
