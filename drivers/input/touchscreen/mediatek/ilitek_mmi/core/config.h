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

#ifndef __CONFIG_H
#define __CONFIG_H

typedef struct {
	int nId;
	int nX;
	int nY;
	int nStatus;
	int nFlag;
} VIRTUAL_KEYS;

typedef struct {
	uint16_t nMaxX;
	uint16_t nMaxY;
	uint16_t nMinX;
	uint16_t nMinY;

	uint8_t nMaxTouchNum;
	uint8_t nMaxKeyButtonNum;

	uint8_t nXChannelNum;
	uint8_t nYChannelNum;
	uint8_t nHandleKeyFlag;
	uint8_t nKeyCount;

	uint16_t nKeyAreaXLength;
	uint16_t nKeyAreaYLength;

	VIRTUAL_KEYS virtual_key[10];

	uint16_t res_width;
	uint16_t res_height;

	/* added for protocol v5 */
	uint8_t self_tx_channel_num;
	uint8_t self_rx_channel_num;
	uint8_t side_touch_type;

} TP_INFO;

struct core_config_data {
	uint32_t chip_id;
	uint32_t chip_type;
	uint32_t chip_pid;
	uint32_t chip_otp_id;
	uint32_t chip_ana_id;
	uint8_t core_type;

	uint32_t slave_i2c_addr;
	uint32_t ice_mode_addr;
	uint32_t pid_addr;
	uint32_t otp_id_addr;
	uint32_t ana_id_addr;
	uint32_t wdt_addr;
	uint32_t ic_reset_addr;

	uint8_t protocol_ver[4];
	uint8_t firmware_ver[9];
	uint8_t core_ver[5];

	bool isEnableGesture;
	bool icemodeenable;
	bool spi_pro_9881h11;
	TP_INFO *tp_info;
};

struct set_res_data {
	uint16_t width;
	uint16_t height;
};

extern struct core_config_data *core_config;
extern struct set_res_data set_res;

extern int fw_cmd_len;
extern int protocol_cmd_len;
extern int tp_info_len;
extern int key_info_len;
extern int panel_info_len;
extern int core_cmd_len;

/* R/W with Touch ICs */
extern uint32_t core_config_ice_mode_read(uint32_t addr);
extern int core_config_ice_mode_write(uint32_t addr, uint32_t data, uint32_t size);
extern int core_config_ice_mode_bit_mask(uint32_t addr, uint32_t nMask, uint32_t value);
extern uint32_t core_config_read_write_onebyte(uint32_t addr);
extern int core_config_ice_mode_disable(void);
extern int core_config_ice_mode_enable(bool stop_mcu);

/* Touch IC status */
extern void core_config_read_flash_info(void);
extern uint32_t core_config_read_pc_counter(void);
extern int core_config_switch_fw_mode(uint8_t *data);
extern int core_config_set_watch_dog(bool enable);
extern int core_config_check_cdc_busy(int count, int delay);
extern int core_config_check_int_status(bool high);
extern void core_config_ic_suspend(void);
extern void core_config_ic_resume(void);
extern int core_config_ic_reset(void);

/* control features of Touch IC */
extern void core_config_sense_ctrl(bool start);
extern void core_config_sleep_ctrl(bool out);
extern void core_config_glove_ctrl(bool enable, bool seamless);
extern void core_config_stylus_ctrl(bool enable, bool seamless);
extern void core_config_tp_scan_mode(bool mode);
extern void core_config_lpwg_ctrl(bool enable);
extern void core_config_gesture_ctrl(uint8_t func);
extern void core_config_phone_cover_ctrl(bool enable);
extern void core_config_finger_sense_ctrl(bool enable);
extern void core_config_proximity_ctrl(bool enable);
extern void core_config_plug_ctrl(bool out);
extern void core_config_set_phone_cover(uint8_t *pattern);
extern void core_get_ddi_register_onlyone(uint8_t page, uint8_t reg);
extern uint32_t core_config_get_reg_data(uint32_t addr);
extern void core_set_ddi_register_onlyone(uint8_t page, uint8_t reg, uint8_t data);
extern int core_edge_plam_ctrl(int type);

/* Touch IC information */
extern int core_config_get_project_id(void);
extern int core_config_get_core_ver(void);
extern int core_config_get_key_info(void);
extern int core_config_get_tp_info(void);
extern int core_config_get_panel_info(void);
extern int core_config_get_protocol_ver(void);
extern int core_config_get_fw_ver(void);
extern int core_config_get_chip_id(void);

extern int core_config_init(void);
#endif /* __CONFIG_H */
