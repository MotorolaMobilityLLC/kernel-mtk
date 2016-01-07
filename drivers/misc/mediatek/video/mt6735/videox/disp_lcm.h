#ifndef _DISP_LCM_H_
#define _DISP_LCM_H_
#include "lcm_drv.h"

#define MAX_LCM_NUMBER	2

#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
extern unsigned char lcm_name_list[][128];
#endif


typedef struct {
	LCM_PARAMS *params;
	LCM_DRIVER *drv;
	LCM_INTERFACE_ID lcm_if_id;
	int module;
	int is_inited;
	unsigned int lcm_original_width;
	unsigned int lcm_original_height;
	int index;
#if !defined(CONFIG_MTK_LEGACY)
	unsigned int driver_id;
	unsigned int module_id;
#endif
} disp_lcm_handle, *pdisp_lcm_handle;

disp_lcm_handle *disp_lcm_probe(char *plcm_name, LCM_INTERFACE_ID lcm_id);
int disp_lcm_init(disp_lcm_handle *plcm, int force);
LCM_PARAMS *disp_lcm_get_params(disp_lcm_handle *plcm);
LCM_INTERFACE_ID disp_lcm_get_interface_id(disp_lcm_handle *plcm);
int disp_lcm_update(disp_lcm_handle *plcm, int x, int y, int w, int h, int force);
int disp_lcm_esd_check(disp_lcm_handle *plcm);
int disp_lcm_esd_recover(disp_lcm_handle *plcm);
int disp_lcm_suspend(disp_lcm_handle *plcm);
int disp_lcm_resume(disp_lcm_handle *plcm);
int disp_lcm_set_backlight(disp_lcm_handle *plcm, int level);
int disp_lcm_read_fb(disp_lcm_handle *plcm);
int disp_lcm_ioctl(disp_lcm_handle *plcm, LCM_IOCTL ioctl, unsigned int arg);
int disp_lcm_is_video_mode(disp_lcm_handle *plcm);
int disp_lcm_is_inited(disp_lcm_handle *plcm);
unsigned int disp_lcm_ATA(disp_lcm_handle *plcm);
void *disp_lcm_switch_mode(disp_lcm_handle *plcm, int mode);
int disp_lcm_set_cmd(disp_lcm_handle *plcm, void *handle, int *lcm_cmd, unsigned int cmd_num);

#endif
