#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/mtk_gpu_utility.h>

#include <aee.h>
#include <mt_smi.h>




#ifndef MMDVFS_STANDALONE
#include <mt_vcorefs_manager.h>
#endif
#include <mach/mt_freqhopping.h>


#include "mmdvfs_mgr.h"

#undef pr_fmt
#define pr_fmt(fmt) "[" MMDVFS_LOG_TAG "]" fmt

/* MMDVFS SWITCH. NO MMDVFS for 6595 */
#if IS_ENABLED(CONFIG_ARM64)
/* 6795 */
#define MMDVFS_ENABLE	1
#else
/* 6595 */
#define MMDVFS_ENABLE	0
#endif

#if MMDVFS_ENABLE
#ifndef MMDVFS_STANDALONE
#include <mach/fliper.h>
#endif
#endif

/* WQHD MMDVFS SWITCH */
#define MMDVFS_ENABLE_WQHD	0

#define MMDVFS_GPU_LOADING_NUM	30
#define MMDVFS_GPU_LOADING_START_INDEX	10
#define MMDVFS_GPU_LOADING_SAMPLE_DURATION_IN_MS	100
#define MMDVFS_GPU_LOADING_THRESHOLD	18

/* enable WQHD defalt 1.0v */
/* #define MMDVFS_WQHD_1_0V */

#if (MMDVFS_GPU_LOADING_START_INDEX >= MMDVFS_GPU_LOADING_NUM)
#error "start index too large"
#endif

/* mmdvfs MM sizes */
#define MMDVFS_PIXEL_NUM_720P	(1280 * 720)
#define MMDVFS_PIXEL_NUM_2160P	(3840 * 2160)
#define MMDVFS_PIXEL_NUM_1080P	(2100 * 1300)
#define MMDVFS_PIXEL_NUM_2M		(2100 * 1300)
#define MMDVFS_PIXEL_NUM_13M		(13000000)

/* 13M sensor */
#define MMDVFS_PIXEL_NUM_SENSOR_FULL (13000000)

/* mmdvfs display sizes */
#define MMDVFS_DISPLAY_SIZE_FHD	(1920 * 1216)

#define MMDVFS_CLK_SWITCH_CB_MAX 16
#define MMDVFS_CLK_SWITCH_CLIENT_MSG_MAX 20

static int notify_cb_func_checked(clk_switch_cb func, int ori_mmsys_clk_mode,
int update_mmsys_clk_mode, char *msg);
static int mmdfvs_adjust_mmsys_clk_by_hopping(int clk_mode);
static int mmdvfs_set_step_with_mmsys_clk(MTK_SMI_BWC_SCEN scenario, mmdvfs_voltage_enum step,
int mmsys_clk_mode);
static void notify_mmsys_clk_change(int ori_mmsys_clk_mode, int update_mmsys_clk_mode);
static int mmsys_clk_change_notify_checked(clk_switch_cb func, int ori_mmsys_clk_mode,
int update_mmsys_clk_mode, char *msg);
static mmdvfs_voltage_enum determine_current_mmsys_clk(void);
static int is_cam_monior_work;


enum {
	MMDVFS_CAM_MON_SCEN = SMI_BWC_SCEN_CNT, MMDVFS_SCEN_MHL, MMDVFS_SCEN_COUNT
};

static clk_switch_cb quick_mmclk_cbs[MMDVFS_CLK_SWITCH_CB_MAX];
static clk_switch_cb notify_cb_func;
static clk_switch_cb notify_cb_func_nolock;
static int current_mmsys_clk = MMSYS_CLK_MEDIUM;

/* + 1 for MMDVFS_CAM_MON_SCEN */
static mmdvfs_voltage_enum g_mmdvfs_scenario_voltage[MMDVFS_SCEN_COUNT] = {
MMDVFS_VOLTAGE_DEFAULT};
static mmdvfs_voltage_enum g_mmdvfs_current_step;
static unsigned int g_mmdvfs_concurrency;
static MTK_SMI_BWC_MM_INFO *g_mmdvfs_info;
static MTK_MMDVFS_CMD g_mmdvfs_cmd;

/* mmdvfs timer for monitor gpu loading */
typedef struct {
	/* linux timer */
	struct timer_list timer;

	/* work q */
	struct workqueue_struct *work_queue;
	struct work_struct work;

	/* data payload */
	unsigned int gpu_loadings[MMDVFS_GPU_LOADING_NUM];
	int gpu_loading_index;
} mmdvfs_gpu_monitor_struct;

typedef struct {
	spinlock_t scen_lock;
	int is_mhl_enable;
	mmdvfs_gpu_monitor_struct gpu_monitor;

} mmdvfs_context_struct;

/* mmdvfs_query() return value, remember to sync with user space */
typedef enum {
	MMDVFS_STEP_LOW = 0, MMDVFS_STEP_HIGH,

	MMDVFS_STEP_LOW2LOW, /* LOW */
	MMDVFS_STEP_HIGH2LOW, /* LOW */
	MMDVFS_STEP_LOW2HIGH, /* HIGH */
	MMDVFS_STEP_HIGH2HIGH,
/* HIGH */
} mmdvfs_step_enum;

/* lcd size */
typedef enum {
	MMDVFS_LCD_SIZE_FHD, MMDVFS_LCD_SIZE_WQHD, MMDVFS_LCD_SIZE_END_OF_ENUM
} mmdvfs_lcd_size_enum;

static mmdvfs_context_struct g_mmdvfs_mgr_cntx;
static mmdvfs_context_struct * const g_mmdvfs_mgr = &g_mmdvfs_mgr_cntx;

static mmdvfs_lcd_size_enum mmdvfs_get_lcd_resolution(void)
{
	if (DISP_GetScreenWidth() * DISP_GetScreenHeight()
	<= MMDVFS_DISPLAY_SIZE_FHD) {
		return MMDVFS_LCD_SIZE_FHD;
	}

	return MMDVFS_LCD_SIZE_WQHD;
}

static mmdvfs_voltage_enum mmdvfs_get_default_step(void)
{
#ifdef MMDVFS_WQHD_1_0V
	return MMDVFS_VOLTAGE_LOW;
#else
	if (mmdvfs_get_lcd_resolution() == MMDVFS_LCD_SIZE_FHD)
		return MMDVFS_VOLTAGE_LOW;
	else
		return MMDVFS_VOLTAGE_HIGH;
#endif
}

static mmdvfs_voltage_enum mmdvfs_get_current_step(void)
{
	return g_mmdvfs_current_step;
}

static int mmsys_clk_query(MTK_SMI_BWC_SCEN scenario,
MTK_MMDVFS_CMD *cmd)
{
	int step = MMSYS_CLK_MEDIUM;

	unsigned int venc_size;
	MTK_MMDVFS_CMD cmd_default;

	venc_size = g_mmdvfs_info->video_record_size[0]
	* g_mmdvfs_info->video_record_size[1];

	/* use default info */
	if (cmd == NULL) {
		memset(&cmd_default, 0, sizeof(MTK_MMDVFS_CMD));
		cmd_default.camera_mode = MMDVFS_CAMERA_MODE_FLAG_DEFAULT;
		cmd = &cmd_default;
	}

	/* collect the final information */
	if (cmd->sensor_size == 0)
		cmd->sensor_size = g_mmdvfs_cmd.sensor_size;

	if (cmd->sensor_fps == 0)
		cmd->sensor_fps = g_mmdvfs_cmd.sensor_fps;

	if (cmd->camera_mode == MMDVFS_CAMERA_MODE_FLAG_DEFAULT)
		cmd->camera_mode = g_mmdvfs_cmd.camera_mode;

	/* HIGH level scenarios */
	switch (scenario) {
	case SMI_BWC_SCEN_VR:
		if (is_force_max_mmsys_clk())
			step = MMSYS_CLK_HIGH;

		if (cmd->sensor_size >= MMDVFS_PIXEL_NUM_13M)
			/* 13M high */
			step = MMSYS_CLK_HIGH;
		else if (cmd->camera_mode & (MMDVFS_CAMERA_MODE_FLAG_PIP | MMDVFS_CAMERA_MODE_FLAG_STEREO))
			/* PIP for ISP clock */
			step = MMSYS_CLK_HIGH;
		break;

	case SMI_BWC_SCEN_VR_SLOW:
	case SMI_BWC_SCEN_ICFP:
		step = MMSYS_CLK_HIGH;
		break;

	default:
		break;
	}

	return step;
}

static mmdvfs_voltage_enum mmdvfs_query(MTK_SMI_BWC_SCEN scenario,
MTK_MMDVFS_CMD *cmd)
{
	mmdvfs_voltage_enum step = mmdvfs_get_default_step();
	unsigned int venc_size;
	MTK_MMDVFS_CMD cmd_default;

	venc_size = g_mmdvfs_info->video_record_size[0]
	* g_mmdvfs_info->video_record_size[1];

	/* use default info */
	if (cmd == NULL) {
		memset(&cmd_default, 0, sizeof(MTK_MMDVFS_CMD));
		cmd_default.camera_mode = MMDVFS_CAMERA_MODE_FLAG_DEFAULT;
		cmd = &cmd_default;
	}

	/* collect the final information */
	if (cmd->sensor_size == 0)
		cmd->sensor_size = g_mmdvfs_cmd.sensor_size;

	if (cmd->sensor_fps == 0)
		cmd->sensor_fps = g_mmdvfs_cmd.sensor_fps;

	if (cmd->camera_mode == MMDVFS_CAMERA_MODE_FLAG_DEFAULT)
		cmd->camera_mode = g_mmdvfs_cmd.camera_mode;

	/* HIGH level scenarios */
	switch (scenario) {

	case SMI_BWC_SCEN_VR:
		if (is_force_camera_hpm())
			step = MMDVFS_VOLTAGE_HIGH;

		if (cmd->sensor_size >= MMDVFS_PIXEL_NUM_13M)
			/* 13M high */
			step = MMDVFS_VOLTAGE_HIGH;
		else if (cmd->camera_mode & (MMDVFS_CAMERA_MODE_FLAG_PIP | MMDVFS_CAMERA_MODE_FLAG_STEREO |
		MMDVFS_CAMERA_MODE_FLAG_VFB | MMDVFS_CAMERA_MODE_FLAG_EIS_2_0))
			/* PIP for ISP clock */
			step = MMDVFS_VOLTAGE_HIGH;

		break;

	case SMI_BWC_SCEN_VR_SLOW:
	case SMI_BWC_SCEN_ICFP:
		step = MMDVFS_VOLTAGE_HIGH;
		break;

	default:
		break;
	}

	return step;
}

static mmdvfs_voltage_enum determine_current_mmsys_clk(void)
{
	int i = 0;
	int final_clk = MMSYS_CLK_MEDIUM;

	for (i = 0; i < MMDVFS_SCEN_COUNT; i++) {
		if (g_mmdvfs_scenario_voltage[i] == MMDVFS_VOLTAGE_HIGH) {
			/* Check the mmsys clk */
			switch (i) {
			case SMI_BWC_SCEN_VR:
			case MMDVFS_CAM_MON_SCEN:
				if (is_force_max_mmsys_clk())
					final_clk = MMSYS_CLK_HIGH;
				else if (g_mmdvfs_cmd.sensor_size >= MMDVFS_PIXEL_NUM_13M)
					/* 13M high */
					final_clk = MMSYS_CLK_HIGH;
				else if (g_mmdvfs_cmd.camera_mode & (MMDVFS_CAMERA_MODE_FLAG_PIP |
				MMDVFS_CAMERA_MODE_FLAG_STEREO))
					/* PIP for ISP clock */
					final_clk = MMSYS_CLK_HIGH;
				break;
			case SMI_BWC_SCEN_VR_SLOW:
			case SMI_BWC_SCEN_ICFP:
				final_clk = MMSYS_CLK_HIGH;
				break;
			default:
				break;
			}
		}
	}

	return final_clk;
}


static void mmdvfs_update_cmd(MTK_MMDVFS_CMD *cmd)
{
	if (cmd == NULL)
		return;

	if (cmd->sensor_size)
		g_mmdvfs_cmd.sensor_size = cmd->sensor_size;

	if (cmd->sensor_fps)
		g_mmdvfs_cmd.sensor_fps = cmd->sensor_fps;

	/* MMDVFSMSG("update cm %d\n", cmd->camera_mode); */

	/* if (cmd->camera_mode != MMDVFS_CAMERA_MODE_FLAG_DEFAULT) { */
	g_mmdvfs_cmd.camera_mode = cmd->camera_mode;
	/* } */
}

/* static void mmdvfs_dump_info(void)
{
	MMDVFSMSG("CMD %d %d %d\n", g_mmdvfs_cmd.sensor_size,
	g_mmdvfs_cmd.sensor_fps, g_mmdvfs_cmd.camera_mode);
	MMDVFSMSG("INFO VR %d %d\n", g_mmdvfs_info->video_record_size[0],
	g_mmdvfs_info->video_record_size[1]);
}
*/

#ifdef MMDVFS_GPU_MONITOR_ENABLE
static void mmdvfs_timer_callback(unsigned long data)
{
	mmdvfs_gpu_monitor_struct *gpu_monitor =
	(mmdvfs_gpu_monitor_struct *)data;

	unsigned int gpu_loading = 0;

	/* if (mtk_get_gpu_loading(&gpu_loading)) {
		MMDVFSMSG("gpuload %d %ld\n", gpu_loading, jiffies_to_msecs(jiffies));
	*/

	/* store gpu loading into the array */
	gpu_monitor->gpu_loadings[gpu_monitor->gpu_loading_index++]
	= gpu_loading;

	/* fire another timer until the end */
	if (gpu_monitor->gpu_loading_index < MMDVFS_GPU_LOADING_NUM - 1) {
		mod_timer(
		&gpu_monitor->timer,
		jiffies + msecs_to_jiffies(
		MMDVFS_GPU_LOADING_SAMPLE_DURATION_IN_MS));
	} else {
		/* the final timer */
		int i;
		int avg_loading;
		unsigned int sum = 0;

		for (i = MMDVFS_GPU_LOADING_START_INDEX; i
		< MMDVFS_GPU_LOADING_NUM; i++) {
			sum += gpu_monitor->gpu_loadings[i];
		}

		avg_loading = sum / MMDVFS_GPU_LOADING_NUM;

		MMDVFSMSG("gpuload %d AVG %d\n", jiffies_to_msecs(jiffies),
		avg_loading);

		/* drops to low step if the gpu loading is low */
		if (avg_loading <= MMDVFS_GPU_LOADING_THRESHOLD)
			queue_work(gpu_monitor->work_queue, &gpu_monitor->work);
	}

}

static void mmdvfs_gpu_monitor_work(struct work_struct *work)
{
	MMDVFSMSG("WQ %d\n", jiffies_to_msecs(jiffies));
}

static void mmdvfs_init_gpu_monitor(mmdvfs_gpu_monitor_struct *gm)
{
	struct timer_list *gpu_timer = &gm->timer;

	/* setup gpu monitor timer */
	setup_timer(gpu_timer, mmdvfs_timer_callback, (unsigned long)gm);

	gm->work_queue = create_singlethread_workqueue("mmdvfs_gpumon");
	INIT_WORK(&gm->work, mmdvfs_gpu_monitor_work);
}
#endif /* MMDVFS_GPU_MONITOR_ENABLE */

/* delay 4 seconds to go LPM to workaround camera ZSD + PIP issue */
static void mmdvfs_cam_work_handler(struct work_struct *work)
{
	/* MMDVFSMSG("CAM handler %d\n", jiffies_to_msecs(jiffies)); */
	mmdvfs_set_step(MMDVFS_CAM_MON_SCEN, mmdvfs_get_default_step());

	spin_lock(&g_mmdvfs_mgr->scen_lock);
	is_cam_monior_work = 0;
	spin_unlock(&g_mmdvfs_mgr->scen_lock);

}

static DECLARE_DELAYED_WORK(g_mmdvfs_cam_work, mmdvfs_cam_work_handler);

static void mmdvfs_stop_cam_monitor(void)
{
	cancel_delayed_work_sync(&g_mmdvfs_cam_work);
}

#define MMDVFS_CAM_MON_DELAY (6 * HZ)
static void mmdvfs_start_cam_monitor(int scen)
{
	int delayed_mmsys_state = MMSYS_CLK_MEDIUM;

	mmdvfs_stop_cam_monitor();

	spin_lock(&g_mmdvfs_mgr->scen_lock);
	is_cam_monior_work = 1;
	spin_unlock(&g_mmdvfs_mgr->scen_lock);


	if (current_mmsys_clk == MMSYS_CLK_LOW) {
		MMDVFSMSG("Can't switch clk by hopping when CLK is low\n");
		delayed_mmsys_state = MMSYS_CLK_MEDIUM;
	} else {
		delayed_mmsys_state = current_mmsys_clk;
	}

	/* MMDVFSMSG("CAM start %d\n", jiffies_to_msecs(jiffies)); */

	if (is_force_max_mmsys_clk()) {

		mmdvfs_set_step_with_mmsys_clk(MMDVFS_CAM_MON_SCEN, MMDVFS_VOLTAGE_HIGH, MMSYS_CLK_HIGH);

	}	else if (scen == SMI_BWC_SCEN_ICFP || scen == SMI_BWC_SCEN_VR_SLOW || scen == SMI_BWC_SCEN_VR) {

		if (g_mmdvfs_cmd.camera_mode & (MMDVFS_CAMERA_MODE_FLAG_PIP | MMDVFS_CAMERA_MODE_FLAG_STEREO))
			mmdvfs_set_step_with_mmsys_clk(MMDVFS_CAM_MON_SCEN, MMDVFS_VOLTAGE_HIGH, MMSYS_CLK_HIGH);
			/* MMDVFSMSG("CAM monitor keep MMSYS_CLK_HIGH\n"); */
		else if (g_mmdvfs_cmd.camera_mode & (MMDVFS_CAMERA_MODE_FLAG_VFB | MMDVFS_CAMERA_MODE_FLAG_EIS_2_0))
			mmdvfs_set_step_with_mmsys_clk(MMDVFS_CAM_MON_SCEN, MMDVFS_VOLTAGE_HIGH, delayed_mmsys_state);
		/*
		else {
			MMDVFSMSG("Keep cam monitor going so that DISP can't disable the vencpll\n");
		}
		*/

	}
	/* 4 seconds for PIP switch preview aspect delays... */
	schedule_delayed_work(&g_mmdvfs_cam_work, MMDVFS_CAM_MON_DELAY);
}

#if MMDVFS_ENABLE_WQHD

static void mmdvfs_start_gpu_monitor(mmdvfs_gpu_monitor_struct *gm)
{
	struct timer_list *gpu_timer = &gm->timer;

	gm->gpu_loading_index = 0;
	memset(gm->gpu_loadings, 0, sizeof(unsigned int) * MMDVFS_GPU_LOADING_NUM);

	mod_timer(gpu_timer, jiffies + msecs_to_jiffies(MMDVFS_GPU_LOADING_SAMPLE_DURATION_IN_MS));
}

static void mmdvfs_stop_gpu_monitor(mmdvfs_gpu_monitor_struct *gm)
{
	struct timer_list *gpu_timer = &gm->timer;

	/* flush workqueue */
	flush_workqueue(gm->work_queue);
	/* delete timer */
	del_timer(gpu_timer);
}

#endif /* MMDVFS_ENABLE_WQHD */

static void mmdvfs_vcorefs_request_dvfs_opp(int mm_kicker, int mm_dvfs_opp)
{
	int vcore_enable = 0;

	vcore_enable = is_vcorefs_can_work();

	if (vcore_enable != 1) {
		MMDVFSMSG("Vcore disable: is_vcorefs_can_work = %d, (%d, %d)\n", vcore_enable, mm_kicker, mm_dvfs_opp);
	} else {
		/* MMDVFSMSG("Vcore trigger: is_vcorefs_can_work = %d, (%d, %d)\n", vcore_enable,
		mm_kicker, mm_dvfs_opp); */
		vcorefs_request_dvfs_opp(mm_kicker, mm_dvfs_opp);
	}
}

int mmdvfs_set_step(MTK_SMI_BWC_SCEN scenario, mmdvfs_voltage_enum step)
{
	return mmdvfs_set_step_with_mmsys_clk(scenario, step, MMSYS_CLK_MEDIUM);
}

int mmdvfs_set_step_with_mmsys_clk(MTK_SMI_BWC_SCEN smi_scenario, mmdvfs_voltage_enum step, int mmsys_clk_mode_request)
{
	int i, scen_index;
	unsigned int concurrency;
	unsigned int scenario = smi_scenario;
	mmdvfs_voltage_enum final_step = mmdvfs_get_default_step();
	int mmsys_clk_step = MMSYS_CLK_MEDIUM;
	int mmsys_clk_mode = mmsys_clk_mode_request;

	/* workaround for WFD VENC scenario*/
	if (scenario == SMI_BWC_SCEN_VENC || scenario == SMI_BWC_SCEN_VP)
		return 0;

	if (step == MMDVFS_VOLTAGE_DEFAULT_STEP)
		step = final_step;

#if !MMDVFS_ENABLE
	return 0;
#endif

	/* MMDVFSMSG("MMDVFS set voltage scen %d step %d\n", scenario, step); */

	if ((scenario >= (MTK_SMI_BWC_SCEN)MMDVFS_SCEN_COUNT) || (scenario
	< SMI_BWC_SCEN_NORMAL)) {
		MMDVFSERR("invalid scenario\n");
		return -1;
	}

	/* dump information */
	/* mmdvfs_dump_info(); */

	/* go through all scenarios to decide the final step */
	scen_index = (int)scenario;

	spin_lock(&g_mmdvfs_mgr->scen_lock);

	g_mmdvfs_scenario_voltage[scen_index] = step;

	concurrency = 0;
	for (i = 0; i < MMDVFS_SCEN_COUNT; i++) {
		if (g_mmdvfs_scenario_voltage[i] == MMDVFS_VOLTAGE_HIGH)
			concurrency |= 1 << i;
	}

	/* one high = final high */
	for (i = 0; i < MMDVFS_SCEN_COUNT; i++) {
		if (g_mmdvfs_scenario_voltage[i] == MMDVFS_VOLTAGE_HIGH) {
			final_step = MMDVFS_VOLTAGE_HIGH;
			break;
		}
	}

	mmsys_clk_step = determine_current_mmsys_clk();
	if (mmsys_clk_mode_request == MMSYS_CLK_MEDIUM && mmsys_clk_step == MMSYS_CLK_HIGH)
		mmsys_clk_mode = MMSYS_CLK_HIGH;
	else
		mmsys_clk_mode = mmsys_clk_mode_request;

	g_mmdvfs_current_step = final_step;

	spin_unlock(&g_mmdvfs_mgr->scen_lock);

#if	MMDVFS_ENABLE

	/* call vcore dvfs API */
	/* MMDVFSMSG("FHD %d\n", final_step); */



	if (final_step == MMDVFS_VOLTAGE_HIGH) {
		if (scenario == MMDVFS_SCEN_MHL)
			mmdvfs_vcorefs_request_dvfs_opp(KIR_MM_MHL, OPPI_PERF);
		else if (scenario == SMI_BWC_SCEN_WFD)
			mmdvfs_vcorefs_request_dvfs_opp(KIR_MM_WFD, OPPI_PERF);
		else {
			mmdvfs_vcorefs_request_dvfs_opp(KIR_MM_16MCAM, OPPI_PERF);
			if (mmsys_clk_mode == MMSYS_CLK_HIGH)
				mmdfvs_adjust_mmsys_clk_by_hopping(MMSYS_CLK_HIGH);
			else
				mmdfvs_adjust_mmsys_clk_by_hopping(MMSYS_CLK_MEDIUM);
		}
	}	else{
		if (scenario == MMDVFS_SCEN_MHL)
			mmdvfs_vcorefs_request_dvfs_opp(KIR_MM_MHL, OPPI_UNREQ);
		else if (scenario == SMI_BWC_SCEN_WFD)
			mmdvfs_vcorefs_request_dvfs_opp(KIR_MM_WFD, OPPI_UNREQ);
		else {
		  /* must lower the mmsys clk before enter LPM mode */
			mmdfvs_adjust_mmsys_clk_by_hopping(MMSYS_CLK_MEDIUM);
			mmdvfs_vcorefs_request_dvfs_opp(KIR_MM_16MCAM, OPPI_UNREQ);
		}
	}
#endif /* MMDVFS_ENABLE */

	MMDVFSMSG("Set vol scen:%d,step:%d,final:%d(0x%x),CMD(%d,%d,0x%x),INFO(%d,%d),CLK:%d\n",
	scenario, step, final_step, concurrency,
	g_mmdvfs_cmd.sensor_size, g_mmdvfs_cmd.sensor_fps, g_mmdvfs_cmd.camera_mode,
	g_mmdvfs_info->video_record_size[0], g_mmdvfs_info->video_record_size[1],
	current_mmsys_clk);


	return 0;
}

void mmdvfs_handle_cmd(MTK_MMDVFS_CMD *cmd)
{
#if !MMDVFS_ENABLE
	return;
#endif

	/* MMDVFSMSG("MMDVFS handle cmd %u s %d\n", cmd->type, cmd->scen); */

	switch (cmd->type) {
	case MTK_MMDVFS_CMD_TYPE_SET:
		/* save cmd */
		mmdvfs_update_cmd(cmd);

		if (!(g_mmdvfs_concurrency & (1 << cmd->scen))) {
			MMDVFSMSG("invalid set scen %d\n", cmd->scen);
			cmd->ret = -1;
		} else {
			cmd->ret = mmdvfs_set_step_with_mmsys_clk(cmd->scen,
			mmdvfs_query(cmd->scen, cmd), mmsys_clk_query(cmd->scen, cmd));
		}
		break;

	case MTK_MMDVFS_CMD_TYPE_QUERY: { /* query with some parameters */
#ifndef MMDVFS_WQHD_1_0V
		if (mmdvfs_get_lcd_resolution() == MMDVFS_LCD_SIZE_WQHD) {
			/* QUERY ALWAYS HIGH for WQHD */
			cmd->ret = (unsigned int)MMDVFS_STEP_HIGH2HIGH;
		} else
#endif
		{
			mmdvfs_voltage_enum query_voltage = mmdvfs_query(cmd->scen, cmd);

			mmdvfs_voltage_enum current_voltage =	mmdvfs_get_current_step();

			if (current_voltage < query_voltage) {
				cmd->ret = (unsigned int)MMDVFS_STEP_LOW2HIGH;
			} else if (current_voltage > query_voltage) {
				cmd->ret = (unsigned int)MMDVFS_STEP_HIGH2LOW;
			} else {
				cmd->ret
				= (unsigned int)(query_voltage
				== MMDVFS_VOLTAGE_HIGH
							 ? MMDVFS_STEP_HIGH2HIGH
							 : MMDVFS_STEP_LOW2LOW);
			}
		}

		/* MMDVFSMSG("query %d\n", cmd->ret); */
		/* cmd->ret = (unsigned int)query_voltage; */
		break;
	}

	default:
		MMDVFSMSG("invalid mmdvfs cmd\n");
		BUG();
		break;
	}
}

void mmdvfs_notify_scenario_exit(MTK_SMI_BWC_SCEN scen)
{
#if !MMDVFS_ENABLE
	return;
#endif

	/* MMDVFSMSG("leave %d\n", scen); */

	if ((scen == SMI_BWC_SCEN_VR) || (scen == SMI_BWC_SCEN_VR_SLOW) || (scen == SMI_BWC_SCEN_ICFP))
		mmdvfs_start_cam_monitor(scen);

	/* reset scenario voltage to default when it exits */
	mmdvfs_set_step(scen, mmdvfs_get_default_step());
}

void mmdvfs_notify_scenario_enter(MTK_SMI_BWC_SCEN scen)
{
#if !MMDVFS_ENABLE
	return;
#endif

	/* MMDVFSMSG("enter %d\n", scen); */

	switch (scen) {
	case SMI_BWC_SCEN_WFD:
		mmdvfs_set_step(scen, MMDVFS_VOLTAGE_HIGH);
		if (current_mmsys_clk == MMSYS_CLK_LOW)
			mmdvfs_raise_mmsys_by_mux();
		break;
	case SMI_BWC_SCEN_VR:
		if (current_mmsys_clk == MMSYS_CLK_LOW)
			mmdvfs_raise_mmsys_by_mux();

		if (is_force_camera_hpm()) {
			if (is_force_max_mmsys_clk())
				mmdvfs_set_step_with_mmsys_clk(scen, MMDVFS_VOLTAGE_HIGH, MMSYS_CLK_HIGH);
			else
				mmdvfs_set_step(scen, MMDVFS_VOLTAGE_HIGH);
		} else {
			if (g_mmdvfs_cmd.camera_mode & (MMDVFS_CAMERA_MODE_FLAG_PIP | MMDVFS_CAMERA_MODE_FLAG_STEREO)) {
				mmdvfs_set_step_with_mmsys_clk(scen, MMDVFS_VOLTAGE_HIGH, MMSYS_CLK_HIGH);
			} else if (g_mmdvfs_cmd.camera_mode & (MMDVFS_CAMERA_MODE_FLAG_VFB |
			MMDVFS_CAMERA_MODE_FLAG_EIS_2_0)){
				mmdvfs_set_step(scen, MMDVFS_VOLTAGE_HIGH);
			}
		}
		break;
	case SMI_BWC_SCEN_VR_SLOW:
	case SMI_BWC_SCEN_ICFP:
		if (current_mmsys_clk == MMSYS_CLK_LOW)
			mmdvfs_raise_mmsys_by_mux();
		mmdvfs_set_step_with_mmsys_clk(scen, MMDVFS_VOLTAGE_HIGH, MMSYS_CLK_HIGH);
		break;

	default:
		break;
	}
}

void mmdvfs_init(MTK_SMI_BWC_MM_INFO *info)
{
#if !MMDVFS_ENABLE
	return;
#endif

	spin_lock_init(&g_mmdvfs_mgr->scen_lock);
	/* set current step as the default step */
	g_mmdvfs_current_step = mmdvfs_get_default_step();

	g_mmdvfs_info = info;

#ifdef MMDVFS_GPU_MONITOR_ENABLE
	mmdvfs_init_gpu_monitor(&g_mmdvfs_mgr->gpu_monitor);
#endif /* MMDVFS_GPU_MONITOR_ENABLE */
}

void mmdvfs_mhl_enable(int enable)
{
	g_mmdvfs_mgr->is_mhl_enable = enable;

	if (enable)
		mmdvfs_set_step(MMDVFS_SCEN_MHL, MMDVFS_VOLTAGE_HIGH);
	else
		mmdvfs_set_step(MMDVFS_SCEN_MHL, MMDVFS_VOLTAGE_DEFAULT_STEP);
}

void mmdvfs_notify_scenario_concurrency(unsigned int u4Concurrency)
{
	/*
	 * DO NOT CALL VCORE DVFS API HERE. THIS FUNCTION IS IN SMI SPIN LOCK.
	 */

	/* raise EMI monitor BW threshold in VP, VR, VR SLOW motion cases to
	make sure vcore stay MMDVFS level as long as possible */
	if (u4Concurrency & ((1 << SMI_BWC_SCEN_VP) | (1 << SMI_BWC_SCEN_VR)
	| (1 << SMI_BWC_SCEN_VR_SLOW))) {
#if MMDVFS_ENABLE
		/* MMDVFSMSG("fliper high\n"); */
		/* fliper_set_bw(BW_THRESHOLD_HIGH); */
#endif
	} else {
#if MMDVFS_ENABLE
		/* MMDVFSMSG("fliper normal\n"); */
		/* fliper_restore_bw(); */
#endif
	}

	g_mmdvfs_concurrency = u4Concurrency;
}

int mmdvfs_is_default_step_need_perf(void)
{
	if (mmdvfs_get_default_step() == MMDVFS_VOLTAGE_LOW)
		return 0;
	else
		return 1;
}

/* switch MM CLK callback from VCORE DVFS driver */
void mmdvfs_mm_clock_switch_notify(int is_before, int is_to_high)
{
	/* for WQHD 1.0v, we have to dynamically switch DL/DC */
#ifdef MMDVFS_WQHD_1_0V
	int session_id;

	if (mmdvfs_get_lcd_resolution() != MMDVFS_LCD_SIZE_WQHD)
		return;

	session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);

	if (!is_before && is_to_high) {
		MMDVFSMSG("DL\n");
		/* nonblocking switch to direct link after HPM */
		primary_display_switch_mode_for_mmdvfs(DISP_SESSION_DIRECT_LINK_MODE, session_id, 0);
	} else if (is_before && !is_to_high) {
		/* BLOCKING switch to decouple before switching to LPM */
		MMDVFSMSG("DC\n");
		primary_display_switch_mode_for_mmdvfs(DISP_SESSION_DECOUPLE_MODE, session_id, 1);
	}
#endif /* MMDVFS_WQHD_1_0V */
}

static int mmdfvs_adjust_mmsys_clk_by_hopping(int clk_mode)
{
	int freq_hopping_disable = is_mmdvfs_freq_hopping_disabled();

	int result = 0;

	if (clk_mode == MMSYS_CLK_HIGH) {
		if (current_mmsys_clk == MMSYS_CLK_LOW) {
			MMDVFSMSG("Doesn't allow mmsys clk adjust from low to high!\n");
	  } else if (!freq_hopping_disable && current_mmsys_clk != MMSYS_CLK_HIGH) {
			/* MMDVFSMSG("Freq hopping: DSS: %d\n", 0xE0000);*/
			mt_dfs_vencpll(0xE0000);
			notify_cb_func_checked(notify_cb_func, current_mmsys_clk, MMSYS_CLK_HIGH,
			"notify_cb_func");
			/* For common clients */
			notify_mmsys_clk_change(current_mmsys_clk, MMSYS_CLK_HIGH);
			current_mmsys_clk = MMSYS_CLK_HIGH;
		} else {
			if (freq_hopping_disable)
				MMDVFSMSG("Freq hopping disable, not trigger: DSS: %d\n", 0xE0000);
		}
		result = 1;
	} else if (clk_mode == MMSYS_CLK_MEDIUM) {
		if (!freq_hopping_disable && current_mmsys_clk != MMSYS_CLK_MEDIUM) {
			/* MMDVFSMSG("Freq hopping: DSS: %d\n", 0xB0000); */
			mt_dfs_vencpll(0xB0000);
			notify_cb_func_checked(notify_cb_func, current_mmsys_clk, MMSYS_CLK_MEDIUM,
			"notify_cb_func");
			/* For common clients */
			notify_mmsys_clk_change(current_mmsys_clk, MMSYS_CLK_MEDIUM);
			current_mmsys_clk = MMSYS_CLK_MEDIUM;
		} else {
			if (freq_hopping_disable)
				MMDVFSMSG("Freq hopping disable, not trigger: DSS: %d\n", 0xB0000);
		}
		result = 1;
	} else if (clk_mode == MMSYS_CLK_LOW) {
		MMDVFSMSG("Doesn't support MMSYS_CLK_LOW with hopping in this platform\n");
		result = 1;
	} else {
		MMDVFSMSG("Don't change CLK: mode=%d\n", clk_mode);
		result = 0;
	}

	return result;
}

int mmdvfs_raise_mmsys_by_mux(void)
{
	if (is_mmdvfs_freq_mux_disabled())
		return 0;

	notify_cb_func_checked(notify_cb_func, current_mmsys_clk, MMSYS_CLK_MEDIUM,
	"notify_cb_func");
	current_mmsys_clk = MMSYS_CLK_MEDIUM;
	return 1;

}

int mmdvfs_lower_mmsys_by_mux(void)
{
	if (is_mmdvfs_freq_mux_disabled())
		return 0;

	if (notify_cb_func != NULL && current_mmsys_clk != MMSYS_CLK_HIGH) {
		notify_cb_func(current_mmsys_clk, MMSYS_CLK_LOW);
		current_mmsys_clk = MMSYS_CLK_LOW;
	}	else{
		MMDVFSMSG("lower_cb_func has not been registered");
		return 0;
	}
	return 1;

}

int register_mmclk_switch_cb(clk_switch_cb notify_cb,
clk_switch_cb notify_cb_nolock)
{
	notify_cb_func = notify_cb;
	notify_cb_func_nolock = notify_cb_nolock;

	return 1;
}

static int notify_cb_func_checked(clk_switch_cb func, int ori_mmsys_clk_mode, int update_mmsys_clk_mode, char *msg)
{

	if (is_mmdvfs_freq_mux_disabled()) {
		MMDVFSMSG("notify_cb_func is disabled, not invoked: %s, (%d,%d)\n", msg, ori_mmsys_clk_mode,
			update_mmsys_clk_mode);
		return 0;
	}
	if (func == NULL) {
		MMDVFSMSG("notify_cb_func is NULL, not invoked: %s, (%d,%d)\n", msg, ori_mmsys_clk_mode,
		update_mmsys_clk_mode);
	} else {
		if (ori_mmsys_clk_mode != update_mmsys_clk_mode)
			MMDVFSMSG("notify_cb_func: %s, (%d,%d)\n", msg, ori_mmsys_clk_mode, update_mmsys_clk_mode);

		func(ori_mmsys_clk_mode, update_mmsys_clk_mode);
		return 1;
	}
	return 0;
}

int mmdvfs_notify_mmclk_switch_request(int event)
{
	int i = 0;
	MTK_SMI_BWC_SCEN current_smi_scenario = smi_get_current_profile();

	/* Don't get the lock since there is no need to synchronize the is_cam_monior_work here*/
	if (is_cam_monior_work != 0) {
		/* MMDVFSMSG("Doesn't handle disp request when cam monitor is active\n"); */
		return 0;
	}
	/* MMDVFSMSG("mmclk_switch_request: event=%d, current=%d", event, current_smi_scenario); */

	/* Only in UI idle modw or VP 1 layer scenario */
	/* we can lower the mmsys clock */
	if (event == MMDVFS_EVENT_UI_IDLE_ENTER && current_smi_scenario == SMI_BWC_SCEN_NORMAL) {
		for (i = 0; i < MMDVFS_SCEN_COUNT; i++) {
			if (g_mmdvfs_scenario_voltage[i] == MMDVFS_VOLTAGE_HIGH) {
				MMDVFSMSG("Doesn't switch to low mmsys clk; vore is still in HPM mode");
				return 0;
			}
		}

		/* call back from DISP so we don't need use DISP lock here */
		if (current_mmsys_clk != MMSYS_CLK_HIGH) {
			/* Only disable VENC pll when clock is in 286MHz */
			notify_cb_func_checked(notify_cb_func_nolock, current_mmsys_clk, MMSYS_CLK_LOW,
			"notify_cb_func_nolock");
			current_mmsys_clk = MMSYS_CLK_LOW;
			return 1;
		}
	} else if (event == MMDVFS_EVENT_OVL_SINGLE_LAYER_EXIT || event == MMDVFS_EVENT_UI_IDLE_EXIT) {
		if (current_mmsys_clk != MMSYS_CLK_HIGH) {
			/* call back from DISP so we don't need use DISP lock here */
			notify_cb_func_checked(notify_cb_func_nolock, current_mmsys_clk, MMSYS_CLK_MEDIUM,
			"notify_cb_func_nolock");
			current_mmsys_clk = MMSYS_CLK_MEDIUM;
			return 1;
		}
	} else if (event == MMDVFS_EVENT_OVL_SINGLE_LAYER_ENTER && SMI_BWC_SCEN_VP) {
		/* call back from DISP so we don't need use DISP lock here */
		if (current_mmsys_clk != MMSYS_CLK_HIGH) {
			notify_cb_func_checked(notify_cb_func_nolock, current_mmsys_clk, MMSYS_CLK_LOW,
			"notify_cb_func_nolock");
			current_mmsys_clk = MMSYS_CLK_LOW;
			return 1;
		}
	}
	return 0;
}


int mmdvfs_register_mmclk_switch_cb(clk_switch_cb notify_cb, int mmdvfs_client_id)
{
	if (mmdvfs_client_id >= 0 && mmdvfs_client_id < MMDVFS_CLK_SWITCH_CB_MAX) {
		quick_mmclk_cbs[mmdvfs_client_id] = notify_cb;
	} else{
		MMDVFSMSG("clk_switch_cb register failed: id=%d\n", mmdvfs_client_id);
		return 1;
	}
	return 0;
}

static int mmsys_clk_change_notify_checked(clk_switch_cb func, int ori_mmsys_clk_mode,
int update_mmsys_clk_mode, char *msg)
{
	if (func == NULL) {
		MMDVFSMSG("notify_cb_func is NULL, not invoked: %s, (%d,%d)\n", msg, ori_mmsys_clk_mode,
		update_mmsys_clk_mode);
	} else {
		MMDVFSMSG("notify_cb_func: %s, (%d,%d)\n", msg, ori_mmsys_clk_mode, update_mmsys_clk_mode);
		func(ori_mmsys_clk_mode, update_mmsys_clk_mode);
		return 1;
	}
	return 0;
}

static void notify_mmsys_clk_change(int ori_mmsys_clk_mode, int update_mmsys_clk_mode)
{
	int i = 0;

	char msg[MMDVFS_CLK_SWITCH_CLIENT_MSG_MAX] = "";

	for (i = 0; i < MMDVFS_CLK_SWITCH_CB_MAX; i++) {
		snprintf(msg, MMDVFS_CLK_SWITCH_CLIENT_MSG_MAX, "id=%d", i);
		if (quick_mmclk_cbs[i] != NULL)
			mmsys_clk_change_notify_checked(quick_mmclk_cbs[i], ori_mmsys_clk_mode,
			update_mmsys_clk_mode, msg);
	}
}


void dump_mmdvfs_info(void)
{
	int i = 0;

	MMDVFSMSG("MMDVFS dump: CMD(%d,%d,0x%x),INFO VR(%d,%d),CLK: %d\n",
	g_mmdvfs_cmd.sensor_size, g_mmdvfs_cmd.sensor_fps, g_mmdvfs_cmd.camera_mode,
	g_mmdvfs_info->video_record_size[0], g_mmdvfs_info->video_record_size[1],
	current_mmsys_clk);

	for (i = 0; i < MMDVFS_SCEN_COUNT; i++)
		MMDVFSMSG("Secn:%d,vol-step:%d\n", i, g_mmdvfs_scenario_voltage[i]);

}
