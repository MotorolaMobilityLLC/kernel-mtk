#ifndef _EXTERNAL_FACTORY_H_
#define _EXTERNAL_FACTORY_H_

#include "hdmi_drv.h"

enum HDMI_FACTORY_TEST {
	STEP1_CHIP_INIT,
	STEP2_JUDGE_CALLBACK,
	STEP3_START_DPI_AND_CONFIG,
	STEP4_DPI_STOP_AND_POWER_OFF,
	STEP_FACTORY_MAX_NUM
};

struct DPI_PARAM_CONTEXT {
	int hdmi_width;	/* DPI read buffer width */
	int hdmi_height;	/* DPI read buffer height */
	int bg_width;	/* DPI read buffer width */
	int bg_height;	/* DPI read buffer height */
	enum HDMI_VIDEO_RESOLUTION output_video_resolution;
	int scaling_factor;
};

int hdmi_factory_mode_test(enum HDMI_FACTORY_TEST test_step, void *info);
#endif
