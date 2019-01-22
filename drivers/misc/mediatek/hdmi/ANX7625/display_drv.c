/******************************************************************************
*
Copyright (c) 2016, Analogix Semiconductor, Inc.

PKG Ver  : V1.0

Filename :

Project  : ANX7625

Created  : 02 Oct. 2016

Devices  : ANX7625

Toolchain: Android

Description:

Revision History:

******************************************************************************/


#include <linux/kernel.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include "hdmi_drv.h"

#include "display.h"
#include "anx7625_driver.h"

/*SLIMPORT_FEATURE*/
#include "display_platform.h"


/**
* LOG For SLIMPORT TX Chip HAL
*/

static int txInitFlag;

bool SlimPort_3D_Support;
int  SlimPort_3D_format;

#ifdef ANX7625_MTK_PLATFORM
#else
unsigned char dst_is_dsi;
#endif

#define SLIMPORT_LOG(fmt, arg...)  \
	do { \
		if (slimport_log_on) {\
			pr_err("[SLIMPORT_Chip_HAL]%s,%d,",\
				__func__, __LINE__);\
			pr_info(fmt, ##arg); } \
	} while (0)

#define SLIMPORT_FUNC()    \
	do { \
		if (slimport_log_on) \
			pr_err("[SLIMPORT_Chip_HAL] %s\n", \
				__func__);\
	} while (0)

void slimport_drv_log_enable(bool enable)
{
	slimport_log_on = enable;
}

static int audio_enable;
void slimport_drv_force_on(int from_uart_drv)
{
	SLIMPORT_LOG("slimport_drv_force_on %d\n", from_uart_drv);
}

/****************** Upper Layer To HAL****************************/
static struct HDMI_UTIL_FUNCS slimport_util = {0};
static void slimport_drv_set_util_funcs(
	const struct HDMI_UTIL_FUNCS *util)
{
	memcpy(&slimport_util, util, sizeof(struct HDMI_UTIL_FUNCS));
}

static char *cable_type_print(unsigned short type)
{
	switch (type) {
	case HDMI_CABLE:
		return "HDMI_CABLE";
	case MHL_CABLE:
		return "MHL_CABLE";
	case MHL_SMB_CABLE:
		return "MHL_SMB_CABLE";
	case MHL_2_CABLE:
		return "MHL_2_CABLE";
	case SLIMPORT_CABLE:
		return "SLIMPORT_CABLE";
	default:
		SLIMPORT_LOG("Unknown MHL Cable Type\n");
		return "Unknown MHL Cable Type\n";
	}
}

enum HDMI_CABLE_TYPE Connect_cable_type = SLIMPORT_CABLE;
static unsigned int HDCP_Supported_Info = 140;

static void slimport_drv_get_params(struct HDMI_PARAMS *params)
{
	char *cable_str = "";
	enum HDMI_VIDEO_RESOLUTION input_resolution =
		params->init_config.vformat;
	memset(params, 0, sizeof(struct HDMI_PARAMS));

	if (dst_is_dsi) {
#ifdef ANX7625_MTK_PLATFORM
		params->dsi_params.mode = SYNC_PULSE_VDO_MODE;
		params->dsi_params.switch_mode = 0;
		params->dsi_params.switch_mode_enable = 0;
		params->dsi_params.LANE_NUM = 4;
		params->dsi_params.data_format.color_order =
			LCM_COLOR_ORDER_RGB;
		params->dsi_params.data_format.trans_seq =
			LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi_params.data_format.padding =
			LCM_DSI_PADDING_ON_LSB;
		params->dsi_params.data_format.format =
			LCM_DSI_FORMAT_RGB888;
		params->dsi_params.packet_size = 256;
		/*	params->dsi_params.PS = PACKED_PS_24BIT_RGB888;*/
		params->dsi_params.PS = 2;
		/*	params->dsi_params.cont_clock = TRUE;*/
		params->dsi_params.cont_clock = 1;
		switch (input_resolution) {
		case HDMI_VIDEO_720x480p_60Hz:
			params->dsi_params.horizontal_active_pixel = 720;
			params->dsi_params.horizontal_backporch = 60;
			params->dsi_params.horizontal_frontporch = 16;
			params->dsi_params.horizontal_sync_active = 62;
			params->dsi_params.vertical_active_line = 480;
			params->dsi_params.vertical_backporch = 30;
			params->dsi_params.vertical_frontporch = 9;
			params->dsi_params.vertical_sync_active = 6;
			params->dsi_params.PLL_CLOCK = 83;/*need to tune*/
			params->dsi_params.PLL_CK_VDO = 83;
			params->dsi_params.word_count = 720;

			params->hsync_pulse_width = 62;
			params->hsync_back_porch  = 60;
			params->hsync_front_porch = 16;

			params->vsync_pulse_width = 6;
			params->vsync_back_porch  = 30;
			params->vsync_front_porch = 9;

			params->width       = 720;
			params->height      = 480;

			params->init_config.vformat = HDMI_VIDEO_720x480p_60Hz;
			break;
		case HDMI_VIDEO_1280x720p_60Hz:
			params->dsi_params.horizontal_active_pixel = 1280;
			params->dsi_params.horizontal_backporch = 220;
			params->dsi_params.horizontal_frontporch = 110;
			params->dsi_params.horizontal_sync_active = 40;
			params->dsi_params.vertical_active_line = 720;
			params->dsi_params.vertical_backporch = 20;
			params->dsi_params.vertical_frontporch = 5;
			params->dsi_params.vertical_sync_active = 5;
			params->dsi_params.PLL_CLOCK = 228;/*need to tune*/
			params->dsi_params.PLL_CK_VDO = 228;
			params->dsi_params.word_count = 1280;

			params->hsync_pulse_width = 40;
			params->hsync_back_porch  = 220;
			params->hsync_front_porch = 110;

			params->vsync_pulse_width = 5;
			params->vsync_back_porch  = 20;
			params->vsync_front_porch = 5;

			params->width       = 1280;
			params->height      = 720;
#ifdef CONFIG_MTK_SMARTBOOK_SUPPORT
			if (MHL_Connect_type == MHL_SMB_CABLE) {
				params->width  = 1366;
				params->height = 768;
			}
#endif
			params->init_config.vformat = HDMI_VIDEO_1280x720p_60Hz;
			break;
		case HDMI_VIDEO_1920x1080p_30Hz:
			params->dsi_params.horizontal_active_pixel = 1920;
			params->dsi_params.horizontal_backporch = 148;
			params->dsi_params.horizontal_frontporch = 88;
			params->dsi_params.horizontal_sync_active = 44;
			params->dsi_params.vertical_active_line = 1080;
			params->dsi_params.vertical_backporch = 36;
			params->dsi_params.vertical_frontporch = 4;
			params->dsi_params.vertical_sync_active = 5;
			params->dsi_params.PLL_CLOCK = 228;/*need to tune*/
			params->dsi_params.PLL_CK_VDO = 228;
			params->dsi_params.word_count = 1920;

			params->hsync_pulse_width = 44;
			params->hsync_back_porch  = 148;
			params->hsync_front_porch = 88;

			params->vsync_pulse_width = 5;
			params->vsync_back_porch  = 36;
			params->vsync_front_porch = 4;

			params->width       = 1920;
			params->height      = 1080;

			params->init_config.vformat =
				HDMI_VIDEO_1920x1080p_30Hz;
			break;
		case HDMI_VIDEO_1920x1080p_60Hz:
			params->dsi_params.horizontal_active_pixel = 1920;
			params->dsi_params.horizontal_backporch = 148;
			params->dsi_params.horizontal_frontporch = 88;
			params->dsi_params.horizontal_sync_active = 44;
			params->dsi_params.vertical_active_line = 1080;
			params->dsi_params.vertical_backporch = 36;
			params->dsi_params.vertical_frontporch = 4;
			params->dsi_params.vertical_sync_active = 5;
			params->dsi_params.PLL_CLOCK = 459;/*need to tune*/
			params->dsi_params.PLL_CK_VDO = 459;
			params->dsi_params.word_count = 1920;

			params->hsync_pulse_width = 44;
			params->hsync_back_porch  = 148;
			params->hsync_front_porch = 88;

			params->vsync_pulse_width = 5;
			params->vsync_back_porch  = 36;
			params->vsync_front_porch = 4;

			params->width       = 1920;
			params->height      = 1080;

			params->init_config.vformat =
				HDMI_VIDEO_1920x1080p_60Hz;
			break;
		case HDMI_VIDEO_2160p_DSC_30Hz:
			params->dsi_params.horizontal_active_pixel = 1280;
			params->dsi_params.horizontal_backporch = 98;
			params->dsi_params.horizontal_frontporch = 66;
			params->dsi_params.horizontal_sync_active = 30;
			params->dsi_params.vertical_active_line = 2160;
			params->dsi_params.vertical_backporch = 72;
			params->dsi_params.vertical_frontporch = 8;
			params->dsi_params.vertical_sync_active = 10;
			params->dsi_params.PLL_CLOCK = 308;/*need to tune*/
			params->dsi_params.PLL_CK_VDO = 308;
			params->dsi_params.word_count = 1280 * 3;

			params->hsync_pulse_width = 30;
			params->hsync_back_porch  = 98;
			params->hsync_front_porch = 66;

			params->vsync_pulse_width = 10;
			params->vsync_back_porch  = 72;
			params->vsync_front_porch = 8;

			params->width		= 3840;
			params->height		= 2160;

			params->init_config.vformat =
				HDMI_VIDEO_2160p_DSC_30Hz;
			break;
		case HDMI_VIDEO_2160p_DSC_24Hz:
			params->dsi_params.horizontal_active_pixel = 1280;
			params->dsi_params.horizontal_backporch = 98;
			params->dsi_params.horizontal_frontporch = 242;
			params->dsi_params.horizontal_sync_active = 30;
			params->dsi_params.vertical_active_line = 2160;
			params->dsi_params.vertical_backporch = 72;
			params->dsi_params.vertical_frontporch = 8;
			params->dsi_params.vertical_sync_active = 10;
			params->dsi_params.PLL_CLOCK = 275;/*need to tune*/
			params->dsi_params.PLL_CK_VDO = 275;
			params->dsi_params.word_count = 1280 * 3;

			params->hsync_pulse_width = 30;
			params->hsync_back_porch  = 98;
			params->hsync_front_porch = 242;

			params->vsync_pulse_width = 10;
			params->vsync_back_porch  = 72;
			params->vsync_front_porch = 8;

			params->width		= 3840;
			params->height		= 2160;

			params->init_config.vformat =
				HDMI_VIDEO_2160p_DSC_24Hz;
			break;
		default:
			SLIMPORT_LOG("Unknown support resolution\n");
			break;
		}
#endif
	} else {
		switch (input_resolution) {
		case HDMI_VIDEO_720x480p_60Hz:
			params->clk_pol   = HDMI_POLARITY_RISING;
			params->de_pol    = HDMI_POLARITY_RISING;
			params->hsync_pol = HDMI_POLARITY_RISING;
			params->vsync_pol = HDMI_POLARITY_RISING;

			params->hsync_pulse_width = 62;
			params->hsync_back_porch  = 60;
			params->hsync_front_porch = 16;

			params->vsync_pulse_width = 6;
			params->vsync_back_porch  = 30;
			params->vsync_front_porch = 9;

			params->width       = 720;
			params->height      = 480;
			params->input_clock = HDMI_VIDEO_720x480p_60Hz;

			params->init_config.vformat =
				HDMI_VIDEO_720x480p_60Hz;
			break;
		case HDMI_VIDEO_1280x720p_60Hz:
			params->clk_pol   = HDMI_POLARITY_RISING;
			params->de_pol    = HDMI_POLARITY_RISING;
			params->hsync_pol = HDMI_POLARITY_FALLING;
			params->vsync_pol = HDMI_POLARITY_FALLING;

			params->hsync_pulse_width = 40;
			params->hsync_back_porch  = 220;
			params->hsync_front_porch = 110;

			params->vsync_pulse_width = 5;
			params->vsync_back_porch  = 20;
			params->vsync_front_porch = 5;

			params->width       = 1280;
			params->height      = 720;
#ifdef CONFIG_MTK_SMARTBOOK_SUPPORT
			if (MHL_Connect_type == MHL_SMB_CABLE) {
				params->width  = 1366;
				params->height = 768;
			}
#endif
			params->input_clock = HDMI_VIDEO_1280x720p_60Hz;

			params->init_config.vformat =
				HDMI_VIDEO_1280x720p_60Hz;
			break;
		case HDMI_VIDEO_1920x1080p_30Hz:
			params->clk_pol   = HDMI_POLARITY_RISING;
			params->de_pol    = HDMI_POLARITY_RISING;
			params->hsync_pol = HDMI_POLARITY_FALLING;
			params->vsync_pol = HDMI_POLARITY_FALLING;

			params->hsync_pulse_width = 44;
			params->hsync_back_porch  = 148;
			params->hsync_front_porch = 88;

			params->vsync_pulse_width = 5;
			params->vsync_back_porch  = 36;
			params->vsync_front_porch = 4;

			params->width       = 1920;
			params->height      = 1080;
			params->input_clock = HDMI_VIDEO_1920x1080p_30Hz;

			params->init_config.vformat =
				HDMI_VIDEO_1920x1080p_30Hz;
			break;
		case HDMI_VIDEO_1920x1080p_60Hz:
			params->clk_pol   = HDMI_POLARITY_RISING;
			params->de_pol    = HDMI_POLARITY_RISING;
			params->hsync_pol = HDMI_POLARITY_FALLING;
			params->vsync_pol = HDMI_POLARITY_FALLING;

			params->hsync_pulse_width = 44;
			params->hsync_back_porch  = 148;
			params->hsync_front_porch = 88;

			params->vsync_pulse_width = 5;
			params->vsync_back_porch  = 36;
			params->vsync_front_porch = 4;

			params->width       = 1920;
			params->height      = 1080;
			params->input_clock = HDMI_VIDEO_1920x1080p_60Hz;

			params->init_config.vformat =
				HDMI_VIDEO_1920x1080p_60Hz;
			break;
		case HDMI_VIDEO_2160p_DSC_30Hz:
			params->clk_pol   = HDMI_POLARITY_RISING;
			params->de_pol	  = HDMI_POLARITY_RISING;
			params->hsync_pol = HDMI_POLARITY_FALLING;
			params->vsync_pol = HDMI_POLARITY_FALLING;

			params->hsync_pulse_width = 30;
			params->hsync_back_porch  = 98;
			params->hsync_front_porch = 66;

			params->vsync_pulse_width = 10;
			params->vsync_back_porch  = 72;
			params->vsync_front_porch = 8;

			params->width		= 3840;
			params->height		= 2160;
			params->input_clock = HDMI_VIDEO_2160p_DSC_30Hz;

			params->init_config.vformat =
				HDMI_VIDEO_2160p_DSC_30Hz;
			break;
		case HDMI_VIDEO_2160p_DSC_24Hz:
			params->clk_pol   = HDMI_POLARITY_RISING;
			params->de_pol	  = HDMI_POLARITY_RISING;
			params->hsync_pol = HDMI_POLARITY_FALLING;
			params->vsync_pol = HDMI_POLARITY_FALLING;

			params->hsync_pulse_width = 30;
			params->hsync_back_porch  = 98;
			params->hsync_front_porch = 242;

			params->vsync_pulse_width = 10;
			params->vsync_back_porch  = 72;
			params->vsync_front_porch = 8;

			params->width		= 3840;
			params->height		= 2160;
			params->input_clock = HDMI_VIDEO_2160p_DSC_24Hz;

			params->init_config.vformat =
				HDMI_VIDEO_2160p_DSC_24Hz;
			break;
#if 0 /* VR demo resolution */
		case HDMI_VIDEO_2K_DSC_70Hz:
			params->clk_pol   = HDMI_POLARITY_RISING;
			params->de_pol	  = HDMI_POLARITY_RISING;
			params->hsync_pol = HDMI_POLARITY_FALLING;
			params->vsync_pol = HDMI_POLARITY_FALLING;

			/*params->hsync_pulse_width = 8;*/
			params->hsync_pulse_width = 10;
			params->hsync_back_porch  = 12;
			params->hsync_front_porch = 24;

			params->vsync_pulse_width = 8;
			params->vsync_back_porch  = 1;
			params->vsync_front_porch = 7;

			params->width		= 1440;
			params->height		= 2560;
			params->input_clock = 94414;

			params->init_config.vformat =
				HDMI_VIDEO_2K_DSC_70Hz;
			break;
#endif
		default:
			SLIMPORT_LOG("Unknown support resolution\n");
			break;
		}
	}
	params->init_config.aformat		= HDMI_AUDIO_44K_2CH;
	params->rgb_order				= HDMI_COLOR_ORDER_RGB;
	params->io_driving_current		= IO_DRIVING_CURRENT_2MA;
	params->intermediat_buffer_num	= 4;
	params->scaling_factor			= 0;
	params->cabletype				= Connect_cable_type;
	params->HDCPSupported			= HDCP_Supported_Info;
	/*#ifdef CONFIG_MTK_HDMI_3D_SUPPORT*/
	params->is_3d_support			= SlimPort_3D_Support;
	/*#endif*/
	cable_str = cable_type_print(params->cabletype);
	SLIMPORT_LOG("type %s-%d hdcp %d-%d, 3d-%d\n",
		cable_str, Connect_cable_type, params->HDCPSupported,
		HDCP_Supported_Info, SlimPort_3D_Support);
}

void slimport_drv_suspend(void)
{
	SLIMPORT_LOG("[slimport_drv_suspend]\n");

	command_Mute_Video(1);

}
void slimport_drv_resume(void)
{
	SLIMPORT_LOG("[slimport_drv_resume]\n");

	/*	command_Mute_Video(0);*/

}
static int slimport_drv_audio_config(
	enum HDMI_AUDIO_FORMAT aformat, int bitWidth)
{
	struct AudioFormat Audio_format;

	memset(&Audio_format, 0, sizeof(Audio_format));

	SLIMPORT_LOG("slimport_drv_audio_config format%d,bitwidth%d\n",
		aformat, bitWidth);
	Audio_format.bAudio_word_len = AUDIO_W_LEN_16_20MAX;

	if (bitWidth == 24)
		Audio_format.bAudio_word_len = AUDIO_W_LEN_24_24MAX;

	switch (aformat) {
	case HDMI_AUDIO_32K_2CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_32K;
		Audio_format.bAudioChNum = I2S_CH_2;
		command_Configure_Audio_Input(AUDIO_I2S_2CH_32K);
		break;
	}
	case HDMI_AUDIO_44K_2CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_441K;
		Audio_format.bAudioChNum = I2S_CH_2;
		command_Configure_Audio_Input(AUDIO_I2S_2CH_441K);
		break;
	}
	case HDMI_AUDIO_48K_2CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_48K;
		Audio_format.bAudioChNum = I2S_CH_2;
		command_Configure_Audio_Input(AUDIO_I2S_2CH_48K);

		break;
	}
	case HDMI_AUDIO_96K_2CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_96K;
		Audio_format.bAudioChNum = I2S_CH_2;
		command_Configure_Audio_Input(AUDIO_I2S_2CH_96K);

		break;
	}
	case HDMI_AUDIO_192K_2CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_192K;
		Audio_format.bAudioChNum = I2S_CH_2;

		command_Configure_Audio_Input(AUDIO_I2S_2CH_192K);
		break;
	}
	case HDMI_AUDIO_32K_8CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_32K;
		Audio_format.bAudioChNum = TDM_CH_8;

		command_Configure_Audio_Input(AUDIO_TDM_8CH_32K);
		break;
	}
	case HDMI_AUDIO_44K_8CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_441K;
		Audio_format.bAudioChNum = TDM_CH_8;
		command_Configure_Audio_Input(AUDIO_TDM_8CH_441K);
		break;
	}
	case HDMI_AUDIO_48K_8CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_48K;
		Audio_format.bAudioChNum = TDM_CH_8;
		command_Configure_Audio_Input(AUDIO_TDM_8CH_48K);

		break;
	}
	case HDMI_AUDIO_96K_8CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_96K;
		Audio_format.bAudioChNum = TDM_CH_8;

		command_Configure_Audio_Input(AUDIO_TDM_8CH_96K);
		break;
	}
	case HDMI_AUDIO_192K_8CH: {
		Audio_format.bAudio_Fs = AUDIO_FS_192K;
		Audio_format.bAudioChNum = TDM_CH_8;

		command_Configure_Audio_Input(AUDIO_TDM_8CH_192K);
		break;
	}
	default: {
		SLIMPORT_LOG("slimport_drv_audio_config do not support");
		break;
	}
	}

	if (Audio_format.bAudioChNum == I2S_CH_2) {
		Audio_format.AUDIO_LAYOUT = I2S_LAYOUT_0;
		Audio_format.bAudioType = AUDIO_I2S;
	} else {
		Audio_format.AUDIO_LAYOUT = I2S_LAYOUT_1;
		Audio_format.bAudioType = AUDIO_TDM;
	}


	return 0;
}

static int slimport_drv_video_enable(bool enable)
{
	return 0;
}

static int slimport_drv_audio_enable(bool enable)
{
	SLIMPORT_LOG("[EXTD]Set_I2S_Pin, enable = %d\n", enable);
	if (enable)
		i2s_gpio_ctrl(1);
	else
		i2s_gpio_ctrl(0);
	audio_enable = enable;
	return 0;
}

static int slimport_drv_enter(void)
{
	return 0;
}
static int slimport_drv_exit(void)
{
	return 0;
}


static int slimport_drv_video_config(
	enum HDMI_VIDEO_RESOLUTION vformat,
	enum HDMI_VIDEO_INPUT_FORMAT vin, int vout)
{


	if (vout & HDMI_VOUT_FORMAT_3D_SBS)
		SlimPort_3D_format = VIDEO_3D_SIDE_BY_SIDE;
	else if (vout & HDMI_VOUT_FORMAT_3D_TAB)
		SlimPort_3D_format = VIDEO_3D_TOP_AND_BOTTOM;
	else
		SlimPort_3D_format = VIDEO_3D_NONE;

	sp_tx_set_3d_format(SlimPort_3D_format);

	SLIMPORT_LOG("SlimPort_3D_format: %d\n", SlimPort_3D_format);
	SLIMPORT_LOG("slimport_drv_video_config: %d\n", vformat);

	if (vformat == HDMI_VIDEO_720x480p_60Hz) {
		SLIMPORT_LOG("[slimport_drv]480p\n");
		/*siHdmiTx_VideoSel(HDMI_480P60_4X3);*/
		if (dst_is_dsi)
			command_DSI_Configuration(RESOLUTION_480P_DSI);
		else
			command_DPI_Configuration(RESOLUTION_480P);

	} else if (vformat == HDMI_VIDEO_1280x720p_60Hz) {
		SLIMPORT_LOG("[slimport_drv]720p\n");
		/*siHdmiTx_VideoSel(HDMI_720P60);*/
		if (dst_is_dsi)
			command_DSI_Configuration(RESOLUTION_720P_DSI);
		else
			command_DPI_Configuration(RESOLUTION_720P);
	} else if (vformat == HDMI_VIDEO_1920x1080p_30Hz) {
		SLIMPORT_LOG("[slimport_drv]1080p_30\n");
		if (dst_is_dsi)
			command_DSI_Configuration(RESOLUTION_1080P30_DSI);
		else
			command_DPI_Configuration(RESOLUTION_1080P30);

		/*siHdmiTx_VideoSel(HDMI_1080P30);*/
	} else if (vformat == HDMI_VIDEO_1920x1080p_60Hz) {
		SLIMPORT_LOG("[slimport_drv]1080p_60\n");
		/*siHdmiTx_VideoSel(HDMI_1080P60);*/
		if (dst_is_dsi)
			command_DSI_Configuration(RESOLUTION_1080P60_DSI);
		else
			command_DPI_Configuration(RESOLUTION_1080P60);
	} else if (vformat == HDMI_VIDEO_2160p_DSC_30Hz) {
		SLIMPORT_LOG("[slimport_drv]2160p_30\n");

		if (dst_is_dsi)
			command_DSI_DSC_Configuration(RESOLUTION_DSI_4K30);
		else
			command_DPI_DSC_Configuration(RESOLUTION_DPI_4K30);

	} else if (vformat == HDMI_VIDEO_2160p_DSC_24Hz) {
		SLIMPORT_LOG("[slimport_drv]2160p_24\n");

		if (dst_is_dsi)
			command_DSI_DSC_Configuration(RESOLUTION_DSI_4K24);
		else
			command_DPI_DSC_Configuration(RESOLUTION_DPI_4K24);
#if 0 /* VR demo resolution */
	} else if (vformat == HDMI_VIDEO_2K_DSC_70Hz) {
		SLIMPORT_LOG("[slimport_drv]2K_DSC_70\n");

		if (dst_is_dsi)
			command_DSI_DSC_Configuration(RESOLUTION_DSI_4K24);
		else
			command_DPI_DSC_Configuration(RESOLUTION_DPI_DSC_2K70);
#endif
	} else
		SLIMPORT_LOG("%s, video format not support now\n", __func__);

	return 0;
}

static unsigned int sii_mhl_connected;
static uint8_t ReadConnectionStatus(void)
{
	return (sii_mhl_connected == SLIMPORT_TX_EVENT_CALLBACK) ? 1 : 0;
}
enum HDMI_STATE slimport_drv_get_state(void)
{
	int ret = ReadConnectionStatus();

	SLIMPORT_LOG("ret: %d\n", ret);

	if (ret == 1)
		ret = HDMI_STATE_ACTIVE;
	else
		ret = HDMI_STATE_NO_DEVICE;

	return ret;
}

bool chip_inited;
/*extern int __init anx7625_init(void);*/
static int slimport_drv_init(void)
{
	SLIMPORT_LOG("slimport_drv_init--\n");

	slimport_anx7625_init();

	txInitFlag = 0;
	chip_inited = false;

	SLIMPORT_LOG("slimport_drv_init -\n");
	return 0;
}

/*Should be enhanced*/
int	chip_device_id;
bool need_reset_usb_switch = true;
int slimport_drv_power_on(void)
{
	int ret = 1;

	SLIMPORT_FUNC();
	/*command_Mute_Video(0);*/

	dpi_gpio_ctrl(1);
	i2s_gpio_ctrl(1);

	if (txInitFlag == 0)
		txInitFlag = 1;

	if (chip_device_id > 0)
		ret = 0;

	/*/Unmask_MHL_Intr();*/
	SLIMPORT_LOG("status %d, chipid: %x, ret: %d--%d\n",
		ReadConnectionStatus(), chip_device_id, ret,
		need_reset_usb_switch);

	return ret;
}

void slimport_drv_power_off(void)
{
	SLIMPORT_FUNC();
	command_Mute_Video(1);

	dpi_gpio_ctrl(0);
	i2s_gpio_ctrl(0);

}


static unsigned int pal_resulution;
#define HDMI_4k30_DSC		95 /*MHL doesn't supported*/
#define HDMI_4k24_DSC		93 /*MHL doesn't supported*/
#define HDMI_SPECAIL_RES	190 /*the value is Forbidden resolution*/
#define HDMI_2k70_DSC		191 /*the value is Forbidden resolution*/

void update_av_info_edid(
	bool audio_video, unsigned int param1, unsigned int param2)
{
	if (audio_video) { /*video infor*/
		/*SLIMPORT_LOG("update_av_info_edid: %d\n", param1);*/

		switch (param1) {
		case 0x22:
		case 0x14:
			pal_resulution |= SINK_1080P30;
			break;
		case 0x10:
			pal_resulution |= SINK_1080P60;
			break;
		case 0x4:
			pal_resulution |= SINK_720P60;
			break;
		case 0x3:
		case 0x2:
			pal_resulution |= SINK_480P;
			break;

		case HDMI_4k30_DSC:
			pal_resulution |= SINK_2160p30;
			break;
		case HDMI_4k24_DSC:
			pal_resulution |= SINK_2160p24;
			break;
#if 0 /* VR demo resolution */
		case HDMI_2k70_DSC:
			pal_resulution |= SINK_2K70;
			break;

		case HDMI_SPECAIL_RES:
			pal_resulution |= SINK_SPECIAL_RESOLUTION;
			break;
#endif
		default:
			SLIMPORT_LOG("param1: %d\n", param1);
		}
	}

}
unsigned int si_mhl_get_av_info(void)
{
	unsigned int temp = SINK_1080P30;

	if (pal_resulution & SINK_1080P60)
		pal_resulution &= (~temp);

#ifdef MHL_RESOLUTION_LIMIT_720P_60
	pal_resulution &= (~SINK_1080P60);
	pal_resulution &= (~SINK_1080P30);
#endif

	return pal_resulution;
}
void reset_av_info(void)
{
	pal_resulution = 0;
}
void slimport_GetEdidInfo(void *pv_get_info)
{
	struct HDMI_EDID_INFO_T *ptr = (struct HDMI_EDID_INFO_T *)pv_get_info;

	if (ptr) {
		ptr->ui4_ntsc_resolution = 0;
		ptr->ui4_pal_resolution = si_mhl_get_av_info();
		if (ptr->ui4_pal_resolution == 0) {
			SLIMPORT_LOG("MHL edid parse error\n");
			ptr->ui4_pal_resolution =
				SINK_720P60 | SINK_1080P30 | SINK_480P;
		}
		SLIMPORT_LOG("EDID:0x%x\n", ptr->ui4_pal_resolution);

		if (((sp_tx_bw == BW_54G) && (sp_tx_lane_count == 1)) ||
			((sp_tx_bw == BW_27G) && (sp_tx_lane_count >= 2))) {
			ptr->ui4_pal_resolution  &= (~SINK_2160p30);
			ptr->ui4_pal_resolution  &= (~SINK_2160p24);
		} else if (((sp_tx_bw == BW_27G) && (sp_tx_lane_count == 1)) ||
			((sp_tx_bw == BW_162G) && (sp_tx_lane_count >= 2))) {
			ptr->ui4_pal_resolution  &= (~SINK_2160p30);
			ptr->ui4_pal_resolution  &= (~SINK_2160p24);
			ptr->ui4_pal_resolution  &= (~SINK_1080P60);
		} else if (((sp_tx_bw == BW_162G) && (sp_tx_lane_count == 1))) {
			ptr->ui4_pal_resolution  &= (~SINK_2160p30);
			ptr->ui4_pal_resolution  &= (~SINK_2160p24);
			ptr->ui4_pal_resolution  &= (~SINK_1080P60);
			ptr->ui4_pal_resolution  &= (~SINK_1080P30);
			ptr->ui4_pal_resolution  &= (~SINK_720P60);
		}

		SLIMPORT_LOG("Filter EDID:0x%x\n", ptr->ui4_pal_resolution);

	}
}



enum {
	HDMI_CHANNEL_2 = 0x2,
	HDMI_CHANNEL_3 = 0x3,
	HDMI_CHANNEL_4 = 0x4,
	HDMI_CHANNEL_5 = 0x5,
	HDMI_CHANNEL_6 = 0x6,
	HDMI_CHANNEL_7 = 0x7,
	HDMI_CHANNEL_8 = 0x8,
};

enum {
	HDMI_SAMPLERATE_32  = 0x1,
	HDMI_SAMPLERATE_44  = 0x2,
	HDMI_SAMPLERATE_48  = 0x3,
	HDMI_SAMPLERATE_96  = 0x4,
	HDMI_SAMPLERATE_192 = 0x5,
};

enum {
	HDMI_BITWIDTH_16  = 0x1,
	HDMI_BITWIDTH_24  = 0x2,
	HDMI_BITWIDTH_20  = 0x3,
};

enum {
	AUDIO_32K_2CH		= 0x01,
	AUDIO_44K_2CH		= 0x02,
	AUDIO_48K_2CH		= 0x03,
	AUDIO_96K_2CH		= 0x05,
	AUDIO_192K_2CH	    = 0x07,
	AUDIO_32K_8CH		= 0x81,
	AUDIO_44K_8CH		= 0x82,
	AUDIO_48K_8CH		= 0x83,
	AUDIO_96K_8CH		= 0x85,
	AUDIO_192K_8CH	    = 0x87,
	AUDIO_INITIAL		= 0xFF
};

int slimport_drv_get_external_device_capablity(void)
{
	int capablity = 0;
	/*capablity = HDMI_CHANNEL_8 << 3 |
	*HDMI_SAMPLERATE_192 << 7 | HDMI_BITWIDTH_24 << 10;
	*return capablity;
	*/
	/*
	* bit3-bit6: channal count;
	*bit7-bit9: sample rate;
	*bit10-bit11: bitwidth
	*/
	SLIMPORT_LOG("Cap_MAX_channel%d,Cap_Samplebit%d,Cap_SampleRate%d\n",
		Cap_MAX_channel, Cap_Samplebit, Cap_SampleRate);
	capablity = Cap_MAX_channel << 3 | Cap_SampleRate << 7 |
		Cap_Samplebit << 10;
	if (capablity == 0) {
		capablity = HDMI_CHANNEL_2 << 3 |
			HDMI_SAMPLERATE_44 << 7 | HDMI_BITWIDTH_16 << 10;
	}
	return capablity;
}

#define SLIMPORT_MAX_INSERT_CALLBACK   10
static CABLE_INSERT_CALLBACK
slimport_callback_table[SLIMPORT_MAX_INSERT_CALLBACK];
void slimport_register_cable_insert_callback(CABLE_INSERT_CALLBACK cb)
{
	int i = 0;

	for (i = 0; i < SLIMPORT_MAX_INSERT_CALLBACK; i++) {
		if (slimport_callback_table[i] == cb)
			break;
	}
	if (i < SLIMPORT_MAX_INSERT_CALLBACK)
		return;

	for (i = 0; i < SLIMPORT_MAX_INSERT_CALLBACK; i++) {
		if (slimport_callback_table[i] == NULL)
			break;
	}
	if (i == SLIMPORT_MAX_INSERT_CALLBACK) {
		SLIMPORT_LOG("not enough mhl callback entries for module\n");
		return;
	}

	slimport_callback_table[i] = cb;
	SLIMPORT_LOG("callback: %p,i: %d\n", slimport_callback_table[i], i);

}

void slimport_unregister_cable_insert_callback(CABLE_INSERT_CALLBACK cb)
{
	int i;

	for (i = 0; i < SLIMPORT_MAX_INSERT_CALLBACK; i++) {
		if (slimport_callback_table[i] == cb) {
			SLIMPORT_LOG("Unreg cable insert callback:%p,i:%d\n",
				slimport_callback_table[i], i);
			slimport_callback_table[i] = NULL;
			break;
		}
	}
	if (i == SLIMPORT_MAX_INSERT_CALLBACK) {
		SLIMPORT_LOG("Try to unregister callback function 0x%lx\n",
			(unsigned long int)cb);
		return;
	}

}

void slimport_invoke_cable_callbacks(enum HDMI_STATE state)
{
	int i = 0, j = 0;
	/* 0 is for external display*/
	for (i = 0; i < SLIMPORT_MAX_INSERT_CALLBACK; i++) {
		if (slimport_callback_table[i])
			j = i;
	}

	if (slimport_callback_table[j]) {
		SLIMPORT_LOG("callback: %p, state: %d, j: %d\n",
			slimport_callback_table[j], state, j);
		slimport_callback_table[j](state);
	}
}

static void slimport_drv_dump_register(void)
{
	SP_CTRL_Dump_Reg();
}

/************************** ************************************************/
const struct HDMI_DRIVER *HDMI_GetDriver(void)
{
	static const struct HDMI_DRIVER HDMI_DRV = {
		.set_util_funcs = slimport_drv_set_util_funcs,
		.get_params     = slimport_drv_get_params,
		.init           = slimport_drv_init,
		.enter          = slimport_drv_enter,
		.exit           = slimport_drv_exit,
		.suspend        = slimport_drv_suspend,
		.resume         = slimport_drv_resume,
		.video_config   = slimport_drv_video_config,
		.audio_config   = slimport_drv_audio_config,
		.video_enable   = slimport_drv_video_enable,
		.audio_enable   = slimport_drv_audio_enable,
		.power_on       = slimport_drv_power_on,
		.power_off      = slimport_drv_power_off,
		.get_state      = slimport_drv_get_state,
		.log_enable     = slimport_drv_log_enable,
		.getedid        = slimport_GetEdidInfo,
		.get_external_device_capablity =
			slimport_drv_get_external_device_capablity,
		.register_callback   =
			slimport_register_cable_insert_callback,
		.unregister_callback =
			slimport_unregister_cable_insert_callback,
		.force_on = slimport_drv_force_on,
		.dump = slimport_drv_dump_register,
	};

	SLIMPORT_FUNC();
	return &HDMI_DRV;
}

/************************** *************************************************/

/************************** MHL TX Chip User Layer To HAL*******************/
static char *MHL_TX_Event_Print(unsigned int event)
{
	switch (event) {
	case SLIMPORT_TX_EVENT_CONNECTION:
		return "SLIMPORT_TX_EVENT_CONNECTION";
	case SLIMPORT_TX_EVENT_DISCONNECTION:
		return "SLIMPORT_TX_EVENT_DISCONNECTION";
	case SLIMPORT_TX_EVENT_HPD_CLEAR:
		return "SLIMPORT_TX_EVENT_HPD_CLEAR";
	case SLIMPORT_TX_EVENT_HPD_GOT:
		return "SLIMPORT_TX_EVENT_HPD_GOT";
	case SLIMPORT_TX_EVENT_DEV_CAP_UPDATE:
		return "SLIMPORT_TX_EVENT_DEV_CAP_UPDATE";
	case SLIMPORT_TX_EVENT_EDID_UPDATE:
		return "SLIMPORT_TX_EVENT_EDID_UPDATE";
	case SLIMPORT_TX_EVENT_EDID_DONE:
		return "SLIMPORT_TX_EVENT_EDID_DONE";
	default:
		SLIMPORT_LOG("Unknown SLIMPORT TX Event Type\n");
		return "Unknown SLIMPORT TX Event Type\n";
	}
}

void Notify_AP_MHL_TX_Event(
	unsigned int event, unsigned int event_param, void *param)
{
	if (event != SLIMPORT_TX_EVENT_SMB_DATA)
		SLIMPORT_LOG("%s, event_param: %d\n",
			MHL_TX_Event_Print(event), event_param);
	switch (event) {
	case SLIMPORT_TX_EVENT_CONNECTION:
		break;
	case SLIMPORT_TX_EVENT_DISCONNECTION: {
		sii_mhl_connected = SLIMPORT_TX_EVENT_DISCONNECTION;
		slimport_invoke_cable_callbacks(HDMI_STATE_NO_DEVICE);
		reset_av_info();
		Connect_cable_type = MHL_CABLE;
	}
	break;
	case SLIMPORT_TX_EVENT_HPD_CLEAR: {
		sii_mhl_connected = SLIMPORT_TX_EVENT_DISCONNECTION;
		slimport_invoke_cable_callbacks(HDMI_STATE_NO_DEVICE);
		reset_av_info();
	}
	break;
	case SLIMPORT_TX_EVENT_HPD_GOT:
		break;
	case SLIMPORT_TX_EVENT_DEV_CAP_UPDATE: {
		Connect_cable_type = MHL_SMB_CABLE;
	}
	break;
	case SLIMPORT_TX_EVENT_EDID_UPDATE: {
		update_av_info_edid(true, event_param, 0);
	}
	break;
	case SLIMPORT_TX_EVENT_EDID_DONE: {

		if (hdcp_cap == HDCP14_SUPPORT)
			HDCP_Supported_Info = 140; /*/HDCP 1.4*/
		else if (hdcp_cap == HDCP22_SUPPORT)
			HDCP_Supported_Info = 220; /*/HDCP 2.2*/
		else
			HDCP_Supported_Info = 0; /*/HDCP none*/

		sii_mhl_connected = SLIMPORT_TX_EVENT_CALLBACK;
		slimport_invoke_cable_callbacks(HDMI_STATE_ACTIVE);
	}
	break;
	default:
		return;
	}

}
/*********************************************************/

