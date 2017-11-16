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

#ifndef _PRIMARY_DISPLAY_H_
#define _PRIMARY_DISPLAY_H_

#include "ddp_hal.h"
#include "ddp_manager.h"
#include <linux/types.h>
#include "disp_session.h"

typedef enum {
	DIRECT_LINK_MODE,
	DECOUPLE_MODE,
	SINGLE_LAYER_MODE,
	DEBUG_RDMA1_DSI0_MODE
} DISP_PRIMARY_PATH_MODE;

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

/* --------------------------------------------------------------------------- */

#define DISP_CHECK_RET(expr)                                                \
	do {                                                                    \
		DISP_STATUS ret = (expr);                                           \
		if (DISP_STATUS_OK != ret) {                                        \
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "COMMON", "[ERROR][mtkfb] DISP API return error code: 0x%x\n"\
		   "  file : %s, line : %d\n"                               \
		   "  expr : %s\n", ret, __FILE__, __LINE__, #expr);        \
	}                                                                   \
	} while (0)

/* --------------------------------------------------------------------------- */

#define ASSERT_LAYER    (DDP_OVL_LAYER_MUN-1)
extern unsigned int gEnable_DSI_ClkTrail;
extern bool is_ipoh_bootup;
extern unsigned int FB_LAYER;	/* default LCD layer */
extern unsigned int islcmconnected;
extern unsigned int vramsize;
#define DISP_DEFAULT_UI_LAYER_ID (DDP_OVL_LAYER_MUN-1)
#define DISP_CHANGED_UI_LAYER_ID (DDP_OVL_LAYER_MUN-2)

typedef struct {
	unsigned int id;
	unsigned int curr_en;
	unsigned int next_en;
	unsigned int hw_en;
	int curr_idx;
	int next_idx;
	int hw_idx;
	int curr_identity;
	int next_identity;
	int hw_identity;
	int curr_conn_type;
	int next_conn_type;
	int hw_conn_type;
} DISP_LAYER_INFO;

typedef enum {
	DISP_STATUS_OK = 0,

	DISP_STATUS_NOT_IMPLEMENTED,
	DISP_STATUS_ALREADY_SET,
	DISP_STATUS_ERROR,
} DISP_STATUS;

typedef enum {
	DISP_STATE_IDLE = 0,
	DISP_STATE_BUSY,
} DISP_STATE;

typedef enum {
	DISP_OP_PRE = 0,
	DISP_OP_NORMAL,
	DISP_OP_POST,
} DISP_OP_STATE;

typedef enum {
	DISP_ALIVE = 0xf0,
	DISP_SLEPT,
	DISP_FREEZE,
} DISP_POWER_STATE;

typedef enum {
	FRM_CONFIG = 0,
	FRM_TRIGGER,
	FRM_START,
	FRM_END
} DISP_FRM_SEQ_STATE;

typedef enum {
	DISPLAY_HAL_IOCTL_SET_CMDQ = 0xff00,
	DISPLAY_HAL_IOCTL_ENABLE_CMDQ,
	DISPLAY_HAL_IOCTL_DUMP,
	DISPLAY_HAL_IOCTL_PATTERN,
} DISPLAY_HAL_IOCTL;

typedef struct {
	unsigned int layer;
	unsigned int layer_en;
	unsigned int buffer_source;
	unsigned int fmt;
	unsigned long addr;
	unsigned long addr_sub_u;
	unsigned long addr_sub_v;
	unsigned long vaddr;
	unsigned int src_x;
	unsigned int src_y;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int src_pitch;
	unsigned int dst_x;
	unsigned int dst_y;
	unsigned int dst_w;
	unsigned int dst_h;	/* clip region */
	unsigned int keyEn;
	unsigned int key;
	unsigned int aen;
	unsigned char alpha;

	unsigned int sur_aen;
	unsigned int src_alpha;
	unsigned int dst_alpha;

	unsigned int isTdshp;
	unsigned int isDirty;

	unsigned int buff_idx;
	unsigned int identity;
	unsigned int connected_type;
	unsigned int security;
	unsigned int dirty;
	unsigned int yuv_range;
} primary_disp_input_config;

typedef struct {
	unsigned int fmt;
	unsigned long addr;
	unsigned long addr_sub_u;
	unsigned long addr_sub_v;
	unsigned long vaddr;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
	unsigned int pitch;
	unsigned int pitchUV;

	unsigned int buff_idx;
	unsigned int interface_idx;
	unsigned int security;
	unsigned int dirty;
	int mode;
} disp_mem_output_config;

#define DISP_INTERNAL_BUFFER_COUNT 3

typedef struct {
	struct list_head list;
	struct ion_handle *handle;
	struct sync_fence *pfence;
	void *va;
	uint32_t fence_id;
	uint32_t mva;
	uint32_t size;
	uint32_t output_fence_id;
	uint32_t interface_fence_id;
	unsigned long long timestamp;
	struct ion_client *client;
} disp_internal_buffer_info;

typedef struct {
	unsigned int mva;
	unsigned int max_offset;
	unsigned int seq;
	DISP_FRM_SEQ_STATE state;
} disp_frm_seq_info;

typedef int (*PRIMARY_DISPLAY_CALLBACK) (unsigned int user_data);

int primary_display_init(char *lcm_name, unsigned int lcm_fps, int is_lcm_inited);
int primary_display_config(unsigned long pa, unsigned long mva);

int primary_display_set_frame_buffer_address(unsigned long va,
					     unsigned long mva);
unsigned long primary_display_get_frame_buffer_mva_address(void);
unsigned long primary_display_get_frame_buffer_va_address(void);
unsigned long get_dim_layer_mva_addr(void);
int is_dim_layer(unsigned int long mva);

int primary_display_suspend(void);
int primary_display_resume(void);
int primary_display_ipoh_restore(void);
int primary_display_ipoh_recover(void);

int primary_display_get_width(void);
int primary_display_get_height(void);
int primary_display_get_bpp(void);
int primary_display_get_pages(void);

void primary_display_esd_cust_bycmdq(int enable);
int primary_display_esd_cust_get(void);

int primary_display_set_overlay_layer(primary_disp_input_config *input);
int primary_display_is_alive(void);
int primary_display_is_sleepd(void);
int primary_display_wait_for_vsync(void *config);
unsigned int primary_display_get_ticket(void);
int primary_display_config_input(primary_disp_input_config *input);
int primary_display_user_cmd(unsigned int cmd, unsigned long arg);
int primary_display_trigger(int blocking, void *callback, int need_merge);
int primary_display_config_output(disp_mem_output_config *output);
int primary_display_mem_out_trigger(int blocking, void *callback,
				    unsigned int userdata);
int primary_display_switch_mode(int sess_mode, unsigned int session, int force);
int primary_display_diagnose(void);
int primary_display_signal_recovery(void);
int primary_display_get_info(void *info);
int primary_display_capture_framebuffer(unsigned long pbuf);
int primary_display_capture_framebuffer_ovl(unsigned long pbuf,
					    unsigned int format);
uint32_t DISP_GetVRamSizeBoot(char *cmdline);
uint32_t DISP_GetVRamSize(void);
uint32_t DISP_GetFBRamSize(void);
uint32_t DISP_GetPages(void);
uint32_t DISP_GetScreenBpp(void);
uint32_t DISP_GetScreenWidth(void);
uint32_t DISP_GetScreenHeight(void);
uint32_t DISP_GetActiveHeight(void);
uint32_t DISP_GetActiveWidth(void);
int disp_hal_allocate_framebuffer(phys_addr_t pa_start, phys_addr_t pa_end,
				  unsigned long *va, unsigned long *mva);
int primary_display_is_video_mode(void);
int primary_display_is_decouple_mode(void);
int primary_display_is_mirror_mode(void);
unsigned int primary_display_get_option(const char *option);
CMDQ_SWITCH primary_display_cmdq_enabled(void);
int primary_display_switch_cmdq_cpu(CMDQ_SWITCH use_cmdq);
int primary_display_check_path(char *stringbuf, int buf_len);
int primary_display_manual_lock(void);
int primary_display_manual_unlock(void);
int primary_display_start(void);
int primary_display_stop(void);
int primary_display_esd_recovery(void);
int primary_display_get_debug_state(char *stringbuf, int buf_len);
void primary_display_set_max_layer(int maxlayer);
void primary_display_reset(void);
void primary_display_esd_check_enable(int enable);
LCM_PARAMS *DISP_GetLcmPara(void);
LCM_DRIVER *DISP_GetLcmDrv(void);
int Panel_Master_dsi_config_entry(const char *name, void *config_value);
int primary_display_config_input_multiple(disp_session_input_config *
					  session_input);
int primary_display_force_set_vsync_fps(unsigned int fps);
unsigned int primary_display_get_fps(void);
int primary_display_get_original_width(void);
int primary_display_get_original_height(void);
int primary_display_enable_path_cg(int enable);
int primary_display_lcm_ATA(void);
int primary_display_setbacklight(unsigned int level);
int fbconfig_get_esd_check_test(uint32_t dsi_id, uint32_t cmd, uint8_t *buffer,
				uint32_t num);
int primary_display_pause(PRIMARY_DISPLAY_CALLBACK callback,
			  unsigned int user_data);
int primary_display_switch_dst_mode(int mode);
int primary_display_get_lcm_index(void);
int primary_display_force_set_fps(unsigned int keep, unsigned int skip);
int primary_display_set_fps(int fps);

void primary_display_idlemgr_kick(char *source, int need_lock);
void primary_display_idlemgr_enter_idle(int need_lock);
void primary_display_idlemgr_leave_idle(int need_lock);
void primary_display_update_present_fence(unsigned int fence_idx);
void _cmdq_insert_wait_frame_done_token_mira(void *handle);
int disp_fmt_to_hw_fmt(DISP_FORMAT src_fmt, unsigned int *hw_fmt,
		       unsigned int *Bpp, unsigned int *bpp);
int primary_display_switch_esd_mode(int mode);
int primary_display_cmdq_set_reg(unsigned int addr, unsigned int val);
int primary_display_check_test(void);
int primary_display_vsync_switch(int method);
unsigned int _need_wait_esd_eof(void);
unsigned int _need_register_eint(void);
unsigned int _need_do_esd_check(void);

unsigned long get_Assert_Layer_PA(void);
int is_DAL_Enabled(void);
int DAL_Clean(void);
int DAL_Printf(const char *fmt, ...);
void _cmdq_start_trigger_loop(void);
void _cmdq_stop_trigger_loop(void);
void mtkfb_clear_lcm(void);
int mtkfb_get_debug_state(char *stringbuf, int buf_len);
unsigned int mtkfb_fm_auto_test(void);
int _parse_tag_videolfb(void);

extern unsigned int recovery_start;
extern int decouple_shorter_path;
extern unsigned int isAEEEnabled;
extern bool is_early_suspended;
extern int dfo_query(const char *s, unsigned long *v);

int primary_display_get_session_mode(void);
int display_freeze_mode(int enable, int need_lock);
#endif
