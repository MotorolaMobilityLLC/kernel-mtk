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

/* --------------------------------------------------------------------------- */
#ifndef HDMITX_H
#define     HDMITX_H

/* /#include "mtkfb.h" */

#define MHL_UART_SHARE_PIN

enum HDMI_FACTORY_MODE_TEST {
	STEP1_ENABLE,
	STEP2_GET_STATUS,
	STEP3_START,
	STEP3_SUSPEND,
	STEP_MAX_NUM
};

enum AUDIO_MAX_CHANNEL {
	HDMI_MAX_CHANNEL_2 = 0x2,
	HDMI_MAX_CHANNEL_3 = 0x3,
	HDMI_MAX_CHANNEL_4 = 0x4,
	HDMI_MAX_CHANNEL_5 = 0x5,
	HDMI_MAX_CHANNEL_6 = 0x6,
	HDMI_MAX_CHANNEL_7 = 0x7,
	HDMI_MAX_CHANNEL_8 = 0x8,
};

enum AUDIO_MAX_SAMPLERATE {
	HDMI_MAX_SAMPLERATE_32 = 0x1,
	HDMI_MAX_SAMPLERATE_44 = 0x2,
	HDMI_MAX_SAMPLERATE_48 = 0x3,
	HDMI_MAX_SAMPLERATE_96 = 0x4,
	HDMI_MAX_SAMPLERATE_192 = 0x5,
};

enum AUDIO_MAX_BITWIDTH {
	HDMI_MAX_BITWIDTH_16 = 0x1,
	HDMI_MAX_BITWIDTH_24 = 0x2,
};

enum HDMI_CAPABILITY {
	HDMI_SCALE_ADJUSTMENT_SUPPORT = 0x01,
	HDMI_ONE_RDMA_LIMITATION = 0x02,
	HDMI_PHONE_GPIO_REUSAGE  = 0x04,
	/*bit3-bit6: channal count; bit7-bit9: sample rate; bit10-bit11: bitwidth*/
	HDMI_FACTORY_MODE_NEW    = 0x1000,
};

struct hdmi_device_status {
	bool is_audio_enabled;
	bool is_video_enabled;
};

struct hdmi_device_write {
	unsigned long u4Addr;
	unsigned long u4Data;
};

struct hdmi_para_setting {
	unsigned int u4Data1;
	unsigned int u4Data2;
};

struct hdmi_hdcp_key {
	unsigned char u1Hdcpkey[287];
};

struct hdmi_hdcp_drmkey {
	unsigned char u1Hdcpkey[384];
};

struct send_slt_data {
	unsigned char u1sendsltdata[15];
};

struct MHL_3D_SUPP_T {
	unsigned int ui4_sink_FP_SUP_3D_resolution;
	unsigned int ui4_sink_TOB_SUP_3D_resolution;
	unsigned int ui4_sink_SBS_SUP_3D_resolution;
};

struct CEC_DRV_ADDR_CFG {
	unsigned char ui1_la_num;
	unsigned char e_la[3];
	unsigned short ui2_pa;
	unsigned short h_cecm_svc;
};

struct CEC_HEADER_BLOCK_IO {
	unsigned char destination:4;
	unsigned char initiator:4;
};

struct CEC_FRAME_BLOCK_IO {
	struct CEC_HEADER_BLOCK_IO header;
	unsigned char opcode;
	unsigned char operand[15];
};

struct CEC_FRAME_DESCRIPTION_IO {
	unsigned char size;
	unsigned char sendidx;
	unsigned char reTXcnt;
	void *txtag;
	struct CEC_FRAME_BLOCK_IO blocks;
};

struct CEC_FRAME_INFO {
	unsigned char ui1_init_addr;
	unsigned char ui1_dest_addr;
	unsigned short ui2_opcode;
	unsigned char aui1_operand[14];
	unsigned int z_operand_size;
};

struct CEC_SEND_MSG {
	struct CEC_FRAME_INFO t_frame_info;
	unsigned char b_enqueue_ok;
};

struct CEC_ADDRESS_IO {
	unsigned char ui1_la;
	unsigned short ui2_pa;
};

struct CEC_GETSLT_DATA {
	unsigned char u1Size;
	unsigned char au1Data[14];
};

struct READ_REG_VALUE {
	unsigned int u1address;
	unsigned int pu1Data;
};

struct HDMITX_AUDIO_PARA {
	unsigned char e_hdmi_aud_in;
	unsigned char e_iec_frame;
	unsigned char e_hdmi_fs;
	unsigned char e_aud_code;
	unsigned char u1Aud_Input_Chan_Cnt;
	unsigned char e_I2sFmt;
	unsigned char u1HdmiI2sMclk;
	unsigned char bhdmi_LCh_status[5];
	unsigned char bhdmi_RCh_status[5];
};

int hdmi_post_init(void);
void hdmi_force_on(int from_uart_drv);
void hdmi_cable_fake_plug_in(void);
void hdmi_cable_fake_plug_out(void);
void hdmi_force_resolution(int params);

void hdmi_suspend(void);
void hdmi_resume(void);
void hdmi_power_on(void);
void hdmi_power_off(void);

int hdmi_wait_vsync_debug(int enable);
int hdmi_dump_vendor_chip_register(void);

extern void Extd_DBG_Init(void);
#endif
