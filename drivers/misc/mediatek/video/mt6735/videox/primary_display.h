#ifndef _PRIMARY_DISPLAY_H_
#define _PRIMARY_DISPLAY_H_

#include "ddp_hal.h"
#include "ddp_manager.h"
#include <linux/types.h>
#include "disp_lcm.h"
#include "disp_session.h"

typedef enum {
	DIRECT_LINK_MODE,
	DECOUPLE_MODE,
	SINGLE_LAYER_MODE,
	DEBUG_RDMA1_DSI0_MODE
} DISP_PRIMARY_PATH_MODE;


/* --------------------------------------------------------------------------- */

#define DISP_CHECK_RET(expr)										\
	do {												\
		DISP_STATUS ret = (expr);								\
		if (DISP_STATUS_OK != ret) {								\
			pr_err("DISP/COMMON " "[ERROR][mtkfb] DISP API return error code: 0x%x\n"	\
				"  file : %s, line : %d\n"						\
				"  expr : %s\n", ret, __FILE__, __LINE__, #expr);			\
		}											\
	} while (0)

/* --------------------------------------------------------------------------- */

#define ASSERT_LAYER    (DDP_OVL_LAYER_MUN-1)
extern unsigned int FB_LAYER;	/* default LCD layer */
#define DISP_DEFAULT_UI_LAYER_ID (DDP_OVL_LAYER_MUN-1)
#define DISP_CHANGED_UI_LAYER_ID (DDP_OVL_LAYER_MUN-2)

extern unsigned int gEnableDSIStateCheck;
extern unsigned int ext_session_id;
extern unsigned int is_hwc_enabled;
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
extern  unsigned int g_suspend_flag;
#endif

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
	DISP_SLEPT
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
	unsigned int buff_source;
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
	DISP_BUFFER_TYPE security;
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
	DISP_BUFFER_TYPE security;
	unsigned int dirty;
	int mode;
} disp_mem_output_config;

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


typedef struct {
	DISP_POWER_STATE state;
	unsigned int lcm_fps;
	int max_layer;
	int need_trigger_overlay;
	int need_trigger_ovl1to2;
	int need_trigger_dcMirror_out;
	DISP_PRIMARY_PATH_MODE mode;
	int session_mode;
	unsigned int session_id;
	unsigned int last_vsync_tick;
	unsigned long framebuffer_mva;
	unsigned long framebuffer_va;
	struct mutex lock;
	struct mutex capture_lock;
	struct mutex vsync_lock;
#ifdef DISP_SWITCH_DST_MODE
	struct mutex switch_dst_lock;
#endif
	struct mutex cmd_lock;
	disp_lcm_handle *plcm;
	cmdqRecHandle cmdq_handle_config_esd;

	cmdqRecHandle cmdq_handle_trigger;

	cmdqRecHandle cmdq_handle_config;
	disp_path_handle dpmgr_handle;

	cmdqRecHandle cmdq_handle_ovl1to2_config;
	disp_path_handle ovl2mem_path_handle;

	char *mutex_locker;
	int vsync_drop;
	struct mutex dc_lock;
	struct list_head dc_free_list;
	struct list_head dc_reading_list;
	struct list_head dc_writing_list;
	unsigned int dc_buf_id;
	unsigned int dc_buf[3];
	unsigned int session_buf_id;
	unsigned int session_buf[3];
	cmdqBackupSlotHandle cur_config_fence;
	cmdqBackupSlotHandle subtractor_when_free;
	cmdqBackupSlotHandle cur_mem_config_fence;
	cmdqBackupSlotHandle mem_subtractor_when_free;

	cmdqBackupSlotHandle rdma_buff_info;
	cmdqBackupSlotHandle ovl_status_info;
	cmdqBackupSlotHandle dsi_state_info;
	cmdqBackupSlotHandle rdma_state_info;

#ifdef DISP_DUMP_EVENT_STATUS
	cmdqBackupSlotHandle event_status;
#endif

	DISP_DC_TYPE dc_type;
	unsigned int force_on_wdma_path;
} display_primary_path_context;

struct sec_session_node {
	struct list_head link;
	unsigned int tgid;
};

typedef int (*PRIMARY_DISPLAY_CALLBACK) (unsigned int user_data);

int primary_display_init(char *lcm_name, unsigned int lcm_fps);
int primary_display_config(unsigned long pa, unsigned long mva);

int primary_display_set_frame_buffer_address(unsigned long va, unsigned long mva);
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
int primary_display_get_dc_bpp(void);
int primary_display_get_pages(void);

int primary_display_set_overlay_layer(primary_disp_input_config *input);
int primary_display_is_alive(void);
int primary_display_is_sleepd(void);
int primary_display_is_sleepd_nolock(void);
int primary_display_wait_for_vsync(void *config);
unsigned int primary_display_get_ticket(void);
int primary_display_config_input(primary_disp_input_config *input);
int primary_display_user_cmd(unsigned int cmd, unsigned long arg);
int primary_display_trigger(int blocking, void *callback, unsigned int userdata);
int primary_display_ext_trigger(int blocking, void *callback, unsigned int userdata);
int primary_display_memory_trigger(int blocking, void *callback, unsigned int userdata);
int primary_display_merge_session_cmd(disp_session_config *config);
int primary_display_config_output(disp_mem_output_config *output, unsigned int session_id);
int primary_display_mem_out_trigger(int blocking, void *callback, unsigned int userdata);
int primary_display_switch_mode(int sess_mode, unsigned int session, int force);
int primary_display_diagnose(void);

int primary_display_get_info(void *info);
int primary_display_capture_framebuffer(unsigned long pbuf);
int primary_display_capture_framebuffer_ovl(unsigned long pbuf, unsigned int format);
uint32_t DISP_GetVRamSizeBoot(char *cmdline);
uint32_t DISP_GetVRamSize(void);
uint32_t DISP_GetFBRamSize(void);
uint32_t DISP_GetPages(void);
uint32_t DISP_GetScreenBpp(void);
uint32_t DISP_GetScreenWidth(void);
uint32_t DISP_GetScreenHeight(void);
uint32_t DISP_GetActiveHeight(void);
uint32_t DISP_GetActiveWidth(void);
int disp_hal_allocate_framebuffer(phys_addr_t pa_start, phys_addr_t pa_end, unsigned long *va,
				  unsigned long *mva);
int primary_display_is_video_mode(void);
int primary_display_is_decouple_mode(void);
int primary_display_is_mirror_mode(void);
int primary_display_is_ovl1to2_handle(cmdqRecHandle *handle);
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
int primary_display_config_input_multiple(disp_session_input_config *session_input);
int primary_display_config_interface_input(primary_disp_input_config *input);
int primary_display_force_set_vsync_fps(unsigned int fps);
unsigned int primary_display_get_fps(void);
int primary_display_get_original_width(void);
int primary_display_get_original_height(void);
int primary_display_insert_session_buf(disp_session_buf_info *session_buf_info);
int primary_display_enable_path_cg(int enable);
int primary_display_lcm_ATA(void);
int primary_display_setbacklight(unsigned int level);
int fbconfig_get_esd_check_test(uint32_t dsi_id, uint32_t cmd, uint8_t *buffer, uint32_t num);
int primary_display_pause(PRIMARY_DISPLAY_CALLBACK callback, unsigned int user_data);
int primary_display_switch_dst_mode(int mode);
int primary_display_get_lcm_index(void);
int primary_display_set_cmd(int *lcm_cmd, unsigned int cmd_num);
int disp_fmt_to_hw_fmt(DISP_FORMAT src_fmt, unsigned int *hw_fmt,
		       unsigned int *Bpp, unsigned int *bpp);
void disp_update_trigger_time(void);

display_primary_path_context *primary_display_path_lock(const char *caller);
void primary_display_path_unlock(const char *caller);
int primary_display_switch_wdma_dump(int on);
void _cmdq_insert_wait_frame_done_token_mira(void *handle);
int primary_display_switch_mode_nolock(int sess_mode, unsigned int session, int force);
int primary_display_release_fence_fake(void);

int primary_display_check_test(void);

int primary_display_cmdq_set_reg(unsigned int addr, unsigned int val);

void primary_display_reset_ovl_by_cmdq(unsigned int force);

uint32_t DISP_GetScreenWidth(void);
uint32_t DISP_GetScreenHeight(void);

#endif

void primary_display_update_present_fence(unsigned int fence_idx);
void *primary_get_dpmgr_handle(void);

extern unsigned int WDMA0_FRAME_START_FLAG;
extern unsigned int ALL_LAYER_DISABLE_STEP;
extern unsigned int gTriggerDispMode;	/* 0: normal, 1: lcd only, 2: none of lcd and lcm */
extern unsigned int _need_wait_esd_eof(void);
extern unsigned int _need_register_eint(void);
extern unsigned int _need_do_esd_check(void);
extern void disp_exit_idle_ex(const char *caller);


int primary_display_set_secondary_display(int add, DISP_SESSION_TYPE type);
int init_ext_decouple_buffers(void);
int deinit_ext_decouple_buffers(void);
int primary_display_get_session_mode(void);
