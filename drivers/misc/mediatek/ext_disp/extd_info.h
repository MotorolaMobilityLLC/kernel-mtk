#ifndef __EXTD_INFO_H__
#define __EXTD_INFO_H__


enum EXTD_DEV_ID {
	DEV_MHL,
	DEV_EINK,
	DEV_WFD,
	DEV_MAX_NUM
};

enum EXTD_IOCTL_CMD {
	RECOMPUTE_BG_CMD,
	GET_DEV_TYPE_CMD,
	SET_LAYER_NUM_CMD
};

enum EXTD_GET_INFO_TYPE {
	AP_GET_INFO,
	SF_GET_INFO,
};

struct SWITCH_MODE_INFO_STRUCT {
	unsigned int old_session[DEV_MAX_NUM];
	unsigned int old_mode[DEV_MAX_NUM];
	unsigned int cur_mode;
	unsigned int switching;
	unsigned int ext_sid;
};

struct EXTD_DRIVER {
	int (*init)(void);
	int (*deinit)(void);
	int (*enable)(int enable);
	int (*power_enable)(int enable);
	int (*set_audio_enable)(int enable);
	int (*set_resolution)(int resolution);
	int (*get_dev_info)(int is_sf, void *info);
	int (*get_capability)(void *info);
	int (*get_edid)(void *info);
	int (*wait_vsync)(void);
	int (*fake_connect)(int connect);
	int (*factory_mode_test)(int step, void *info);
	int (*ioctl)(unsigned int ioctl_cmd, int param1, int param2, unsigned long *params);
};

/*get driver handle*/
const struct EXTD_DRIVER *EXTD_EPD_Driver(void);
const struct EXTD_DRIVER *EXTD_HDMI_Driver(void);

/*get driver handle for factory mode test*/
const struct EXTD_DRIVER *EXTD_Factory_HDMI_Driver(void);
const struct EXTD_DRIVER *EXTD_Factory_EPD_Driver(void);

#endif
