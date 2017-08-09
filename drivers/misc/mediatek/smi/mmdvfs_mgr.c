#if ((defined(D1) || defined(D2) || defined(D3)) && !IS_ENABLED(CONFIG_FPGA_EARLY_PORTING))
#define MMDVFS_ENABLE 1
#endif

#include <linux/uaccess.h>
#include <aee.h>

#include <mt_smi.h>

#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <mt_vcore_dvfs.h>

#include "mmdvfs_mgr.h"

#undef pr_fmt
#define pr_fmt(fmt) "[" MMDVFS_LOG_TAG "]" fmt

#define MMDVFS_ENABLE_FLIPER_CONTROL    0

#if MMDVFS_ENABLE_FLIPER_CONTROL
#include <mach/fliper.h>
#endif

/* mmdvfs MM sizes */
#define MMDVFS_PIXEL_NUM_720P   (1280 * 720)
#define MMDVFS_PIXEL_NUM_2160P  (3840 * 2160)
#define MMDVFS_PIXEL_NUM_1080P  (2100 * 1300)
#define MMDVFS_PIXEL_NUM_2M     (2100 * 1300)
/* 13M sensor */
#define MMDVFS_PIXEL_NUM_SENSOR_FULL    (13000000)
#define MMDVFS_PIXEL_NUM_SENSOR_6M  (5800000)
#define MMDVFS_PIXEL_NUM_SENSOR_8M  (7800000)

/* mmdvfs display sizes */
#define MMDVFS_DISPLAY_SIZE_HD  (1280 * 832)
#define MMDVFS_DISPLAY_SIZE_FHD (1920 * 1216)

#define MMDVFS_CAM_MON_SCEN SMI_BWC_SCEN_CNT
#define MMDVFS_SCEN_COUNT   (SMI_BWC_SCEN_CNT + 1)

/* + 1 for MMDVFS_CAM_MON_SCEN */
static mmdvfs_voltage_enum g_mmdvfs_scenario_voltage[MMDVFS_SCEN_COUNT] = {
MMDVFS_VOLTAGE_DEFAULT};
static mmdvfs_voltage_enum g_mmdvfs_current_step;
static MTK_SMI_BWC_MM_INFO *g_mmdvfs_info;
static MTK_MMDVFS_CMD g_mmdvfs_cmd;

struct mmdvfs_context_struct {
	spinlock_t scen_lock;
	int is_mhl_enable;
};

/* mmdvfs_query() return value, remember to sync with user space */
enum mmdvfs_step_enum {
	MMDVFS_STEP_LOW = 0, MMDVFS_STEP_HIGH,

	MMDVFS_STEP_LOW2LOW, /* LOW */
	MMDVFS_STEP_HIGH2LOW, /* LOW */
	MMDVFS_STEP_LOW2HIGH, /* HIGH */
	MMDVFS_STEP_HIGH2HIGH,
/* HIGH */
};

/* lcd size */
enum mmdvfs_lcd_size_enum {
	MMDVFS_LCD_SIZE_HD, MMDVFS_LCD_SIZE_FHD, MMDVFS_LCD_SIZE_WQHD, MMDVFS_LCD_SIZE_END_OF_ENUM
};

static struct mmdvfs_context_struct g_mmdvfs_mgr_cntx;
static struct mmdvfs_context_struct * const g_mmdvfs_mgr = &g_mmdvfs_mgr_cntx;

static enum mmdvfs_lcd_size_enum mmdvfs_get_lcd_resolution(void)
{
	if (DISP_GetScreenWidth() * DISP_GetScreenHeight()
	<= MMDVFS_DISPLAY_SIZE_HD)
		return MMDVFS_LCD_SIZE_HD;

	return MMDVFS_LCD_SIZE_FHD;
}

static mmdvfs_voltage_enum mmdvfs_get_default_step(void)
{
#if defined(D3)
	return MMDVFS_VOLTAGE_LOW;
#else               /* defined(D3) */
	if (mmdvfs_get_lcd_resolution() == MMDVFS_LCD_SIZE_HD)
		return MMDVFS_VOLTAGE_LOW;

	return MMDVFS_VOLTAGE_HIGH;
#endif              /* defined(D3) */
}

static mmdvfs_voltage_enum mmdvfs_get_current_step(void)
{
	return g_mmdvfs_current_step;
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
#if defined(D2)         /* D2 ISP >= 6M HIGH */
	case SMI_BWC_SCEN_VR_SLOW:
	case SMI_BWC_SCEN_VR:
	if (cmd->sensor_size >= MMDVFS_PIXEL_NUM_SENSOR_6M)
		step = MMDVFS_VOLTAGE_HIGH;

	break;
#endif
	/* force HPM for engineering mode */
	case SMI_BWC_SCEN_FORCE_MMDVFS:
		step = MMDVFS_VOLTAGE_HIGH;
		break;
	default:
		break;
	}

	return step;
}

static void mmdvfs_update_cmd(MTK_MMDVFS_CMD *cmd)
{
	if (cmd == NULL)
		return;

	if (cmd->sensor_size)
		g_mmdvfs_cmd.sensor_size = cmd->sensor_size;

	if (cmd->sensor_fps)
		g_mmdvfs_cmd.sensor_fps = cmd->sensor_fps;

	MMDVFSMSG("update cm %d %d\n", cmd->camera_mode, cmd->sensor_size);
	g_mmdvfs_cmd.camera_mode = cmd->camera_mode;
}

static void mmdvfs_dump_info(void)
{
	MMDVFSMSG("CMD %d %d %d\n", g_mmdvfs_cmd.sensor_size,
	g_mmdvfs_cmd.sensor_fps, g_mmdvfs_cmd.camera_mode);
	MMDVFSMSG("INFO VR %d %d\n", g_mmdvfs_info->video_record_size[0],
	g_mmdvfs_info->video_record_size[1]);
}

/* delay 4 seconds to go LPM to workaround camera ZSD + PIP issue */
#if !defined(D3)
static void mmdvfs_cam_work_handler(struct work_struct *work)
{
	MMDVFSMSG("CAM handler %d\n", jiffies_to_msecs(jiffies));
	mmdvfs_set_step(MMDVFS_CAM_MON_SCEN, mmdvfs_get_default_step());
}

static DECLARE_DELAYED_WORK(g_mmdvfs_cam_work, mmdvfs_cam_work_handler);

static void mmdvfs_stop_cam_monitor(void)
{
	cancel_delayed_work_sync(&g_mmdvfs_cam_work);
}

#define MMDVFS_CAM_MON_DELAY (4 * HZ)
static void mmdvfs_start_cam_monitor(void)
{
	mmdvfs_stop_cam_monitor();
	MMDVFSMSG("CAM start %d\n", jiffies_to_msecs(jiffies));
	mmdvfs_set_step(MMDVFS_CAM_MON_SCEN, MMDVFS_VOLTAGE_HIGH);
	/* 4 seconds for PIP switch preview aspect delays... */
	schedule_delayed_work(&g_mmdvfs_cam_work, MMDVFS_CAM_MON_DELAY);
}

#endif              /* !defined(D3) */

int mmdvfs_set_step(MTK_SMI_BWC_SCEN scenario, mmdvfs_voltage_enum step)
{
	int i, scen_index;
	mmdvfs_voltage_enum final_step = mmdvfs_get_default_step();

#if !MMDVFS_ENABLE
	return 0;
#endif

#if defined(D1)
	/* D1 FHD always HPM. do not have to trigger vcore dvfs. */
	if (mmdvfs_get_lcd_resolution() == MMDVFS_LCD_SIZE_FHD)
		return 0;

#endif

	MMDVFSMSG("MMDVFS set voltage scen %d step %d\n", scenario, step);

	if ((scenario >= MMDVFS_SCEN_COUNT) || (scenario < SMI_BWC_SCEN_NORMAL)) {
		MMDVFSERR("invalid scenario\n");
		return -1;
	}

	/* dump information */
	mmdvfs_dump_info();

	/* go through all scenarios to decide the final step */
	scen_index = (int)scenario;

	spin_lock(&g_mmdvfs_mgr->scen_lock);

	g_mmdvfs_scenario_voltage[scen_index] = step;

	/* one high = final high */
	for (i = 0; i < MMDVFS_SCEN_COUNT; i++) {
		if (g_mmdvfs_scenario_voltage[i] == MMDVFS_VOLTAGE_HIGH) {
			final_step = MMDVFS_VOLTAGE_HIGH;
			break;
		}
	}

	g_mmdvfs_current_step = final_step;

	spin_unlock(&g_mmdvfs_mgr->scen_lock);

	MMDVFSMSG("MMDVFS set voltage scen %d step %d final %d\n", scenario,
	step, final_step);

#if MMDVFS_ENABLE
	/* call vcore dvfs API */
	if (final_step == MMDVFS_VOLTAGE_HIGH)
		vcorefs_request_dvfs_opp(KIR_MM, OPPI_PERF);
	else
		vcorefs_request_dvfs_opp(KIR_MM, OPPI_UNREQ);

#endif

	return 0;
}

void mmdvfs_handle_cmd(MTK_MMDVFS_CMD *cmd)
{
#if !MMDVFS_ENABLE
	return;
#endif

	MMDVFSMSG("MMDVFS cmd %u %d\n", cmd->type, cmd->scen);

	switch (cmd->type) {
	case MTK_MMDVFS_CMD_TYPE_SET:
		/* save cmd */
		mmdvfs_update_cmd(cmd);
		cmd->ret = mmdvfs_set_step(cmd->scen,
		mmdvfs_query(cmd->scen, cmd));
		break;

	case MTK_MMDVFS_CMD_TYPE_QUERY: { /* query with some parameters */
		if (mmdvfs_get_lcd_resolution() == MMDVFS_LCD_SIZE_FHD) {
			/* QUERY ALWAYS HIGH for FHD */
			cmd->ret = (unsigned int)MMDVFS_STEP_HIGH2HIGH;

		} else { /* FHD */
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

		MMDVFSMSG("query %d\n", cmd->ret);
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

	MMDVFSMSG("leave %d\n", scen);

#if !defined(D3)        /* d3 does not need this workaround because the MMCLK is always the highest */
	/*
	 * keep HPM for 4 seconds after exiting camera scenarios to get rid of
	 * cam framework will let us go to normal scenario for a short time
	 * (ex: STOP PREVIEW --> NORMAL --> START PREVIEW)
	 * where the LPM mode (low MMCLK) may cause ISP failures
	 */
	if ((scen == SMI_BWC_SCEN_VR) || (scen == SMI_BWC_SCEN_VR_SLOW)
	|| (scen == SMI_BWC_SCEN_ICFP)) {
		mmdvfs_start_cam_monitor();
	}
#endif              /* !defined(D3) */

	/* reset scenario voltage to default when it exits */
	mmdvfs_set_step(scen, mmdvfs_get_default_step());
}

void mmdvfs_notify_scenario_enter(MTK_SMI_BWC_SCEN scen)
{
#if !MMDVFS_ENABLE
	return;
#endif

	MMDVFSMSG("enter %d\n", scen);

	/* ISP ON = high */
	switch (scen) {
#if defined(D2)         /* d2 sensor > 6M */
	case SMI_BWC_SCEN_VR:
	mmdvfs_set_step(scen, mmdvfs_query(scen, NULL));
	break;
	case SMI_BWC_SCEN_VR_SLOW:
#elif defined(D1)       /* default VR high */
	case SMI_BWC_SCEN_VR:
	case SMI_BWC_SCEN_VR_SLOW:
#else               /* D3 */
	case SMI_BWC_SCEN_WFD:
	case SMI_BWC_SCEN_VSS:
#endif
		/* Fall through */
	case SMI_BWC_SCEN_ICFP:
		/* Fall through */
	case SMI_BWC_SCEN_FORCE_MMDVFS:
		mmdvfs_set_step(scen, MMDVFS_VOLTAGE_HIGH);
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
}

void mmdvfs_mhl_enable(int enable)
{
	g_mmdvfs_mgr->is_mhl_enable = enable;
}

void mmdvfs_notify_scenario_concurrency(unsigned int u4Concurrency)
{
	/* raise EMI monitor BW threshold in VP, VR, VR SLOW motion cases */
	/* to make sure vcore stay MMDVFS level as long as possible */
	if (u4Concurrency & ((1 << SMI_BWC_SCEN_VP) | (1 << SMI_BWC_SCEN_VR)
	| (1 << SMI_BWC_SCEN_VR_SLOW))) {
#if MMDVFS_ENABLE_FLIPER_CONTROL
		MMDVFSMSG("fliper high\n");
		fliper_set_bw(BW_THRESHOLD_HIGH);
#endif
	} else {
#if MMDVFS_ENABLE_FLIPER_CONTROL
		MMDVFSMSG("fliper normal\n");
		fliper_restore_bw();
#endif
	}
}
