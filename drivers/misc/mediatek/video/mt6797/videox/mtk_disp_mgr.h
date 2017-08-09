#ifndef __H_MTK_DISP_MGR__
#define __H_MTK_DISP_MGR__
#include "disp_session.h"

typedef enum {
	PREPARE_INPUT_FENCE,
	PREPARE_OUTPUT_FENCE,
	PREPARE_PRESENT_FENCE
} ePREPARE_FENCE_TYPE;

long mtk_disp_mgr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int disp_create_session(disp_session_config *config);
int disp_destroy_session(disp_session_config *config);
int set_session_mode(disp_session_config *config_info, int force);
char *disp_session_mode_spy(unsigned int session_id);

#endif
