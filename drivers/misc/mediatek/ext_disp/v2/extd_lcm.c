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


/*****************************************************************************/
/* Record the modify of dts, dtsi, configs & dws
 * If we support dual lcm, please modify the dts, dtsi, configs and dws files as the following content.
 *
 * dts: Add LCM1 GPIO (RESET & DSI_TE).
 *
 * dtsi: Add dsi_te1 node:
 *		dsi_te_1: dsi_te_1 {
 *			compatible = "mediatek, dsi_te_1-eint";
 *			status = "disabled";
 *		};
 *
 * configs:
 *	CONFIG_CUSTOM_KERNEL_LCM="nt35595_fhd_dsi_cmd_truly_nt50358_extern nt35595_fhd_dsi_cmd_truly_nt50358_2th"
 *	CONFIG_MTK_DUAL_DISPLAY_SUPPORT=2
 *
 * dws: Add dsi_te1 EINT.
 *
*/

/*****************************************************************************/
/*****************************************************************************/
#include "extd_info.h"

#if (CONFIG_MTK_DUAL_DISPLAY_SUPPORT == 2)
#include <linux/mm.h>


#include <linux/delay.h>

#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/io.h>

#include "m4u.h"

#include "mtkfb_info.h"
#include "mtkfb.h"

#include "mtkfb_fence.h"
#include "display_recorder.h"

#include "disp_session.h"
#include "ddp_mmp.h"
#include "ddp_irq.h"

#include "extd_platform.h"
#include "extd_factory.h"
#include "extd_log.h"
#include "extd_utils.h"
#include "external_display.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~the static variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static unsigned int ovl_layer_num;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~the gloable variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
LCM_PARAMS  extd_interface_params;
/*
* struct task_struct *fence_release_task = NULL;
* wait_queue_head_t fence_release_wq;
* atomic_t fence_release_event = ATOMIC_INIT(0);
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~the definition~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define LCM_SESSION_ID          (0x20003)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

unsigned int lcm_get_width(void)
{
	return extd_interface_params.width;

}

unsigned int lcm_get_height(void)
{
	return extd_interface_params.height;
}

#if 0
static void rdma_irq_handler(DISP_MODULE_ENUM module, unsigned int param)
{
	if (param & 0x2) {	/* start */
		atomic_set(&fence_release_event, 1);
		wake_up_interruptible(&fence_release_wq);
	}
}

static int fence_release_kthread(void *data)
{
	int layid = 0;
	int ovl_release = 0;
	unsigned int session_id = 0;
	int fence_idx = 0;
	bool ovl_reg_updated = false;
	unsigned long input_curr_addr[EXTD_OVERLAY_CNT];
	struct sched_param param = {.sched_priority = 94 }; /*RTPM_PRIO_SCRN_UPDATE*/

	sched_setscheduler(current, SCHED_RR, &param);

	for (;;) {
		wait_event_interruptible(fence_release_wq, atomic_read(&fence_release_event));
		atomic_set(&fence_release_event, 0);

		ovl_reg_updated = false;
		session_id = ext_disp_get_sess_id();
		fence_idx = -1;

		/*LCM_LOG("fence_release_kthread wake up\n");*/
		if (session_id == 0 || !ext_disp_is_alive())
			continue;

		if (ext_disp_path_source_is_RDMA(LCM_SESSION_ID)) {
			if (ext_disp_get_ovl_req_status(LCM_SESSION_ID) == EXTD_OVL_REMOVED) {
				ext_disp_path_change(EXTD_OVL_NO_REQ, LCM_SESSION_ID);

				if ((ovl_release == 1) && (EXTD_OVERLAY_CNT > 1)) {
					for (layid = 1; layid < EXTD_OVERLAY_CNT; layid++) {
						fence_idx = disp_sync_find_fence_idx_by_addr(session_id,
										layid, ovl_config_address[layid]);
						fence_idx = ((fence_idx >= 0) ? (fence_idx + 1) : fence_idx);
						mtkfb_release_fence(session_id, layid, fence_idx);
					}

					ovl_release = 0;
				}
			}

			ext_disp_get_curr_addr(input_curr_addr, 0);
			fence_idx = disp_sync_find_fence_idx_by_addr(session_id, 0, input_curr_addr[0]);
			mtkfb_release_fence(session_id, 0, fence_idx);

		} else {
			if (ext_disp_get_ovl_req_status(LCM_SESSION_ID) == EXTD_OVL_INSERTED)
				ext_disp_path_change(EXTD_OVL_NO_REQ, LCM_SESSION_ID);

			ext_disp_get_curr_addr(input_curr_addr, 1);
			for (layid = 0; layid < EXTD_OVERLAY_CNT; layid++) {
				if (ovl_config_address[layid] != input_curr_addr[layid]) {
					ovl_config_address[layid] = input_curr_addr[layid];
					ovl_reg_updated = true;
					ovl_release = 1;
				}

				if (ext_disp_is_dim_layer(input_curr_addr[layid]) == 1)
					fence_idx = disp_sync_find_fence_idx_by_addr(session_id, layid, 0);
				else
					fence_idx = disp_sync_find_fence_idx_by_addr(session_id,
										layid, input_curr_addr[layid]);
				LCM_LOG("Donglei - ovl release, mva:0x%lx, fence_idx:%d\n",
						input_curr_addr[layid], fence_idx);
				mtkfb_release_fence(session_id, layid, fence_idx);
			}

			if (ovl_reg_updated == false) {
				MMProfileLogEx(ddp_mmp_get_events()->Extd_trigger,
						MMProfileFlagPulse, input_curr_addr[0], input_curr_addr[1]);
			}

			MMProfileLogEx(ddp_mmp_get_events()->Extd_UsedBuff,
					MMProfileFlagPulse, input_curr_addr[0], input_curr_addr[1]);
		}

		if (kthread_should_stop())
			break;
	}

	return 0;
}
#endif

int lcm_get_dev_info(int is_sf, void *info)
{
	int ret = 0;

	 if (is_sf == SF_GET_INFO) {
		struct disp_session_info *dispif_info = (struct disp_session_info *) info;

		memset((void *)dispif_info, 0, sizeof(struct disp_session_info));

		dispif_info->isOVLDisabled = (ovl_layer_num == 1) ? 1 : 0;
		dispif_info->maxLayerNum = ovl_layer_num;
		if (extd_interface_params.type == LCM_TYPE_DSI) {
			dispif_info->displayFormat = extd_interface_params.dsi.data_format.format;
			dispif_info->displayHeight = extd_interface_params.height;
			dispif_info->displayWidth = extd_interface_params.width;
			dispif_info->displayType = DISP_IF_TYPE_DSI1;
			if (extd_interface_params.dsi.mode == CMD_MODE)
				dispif_info->displayMode = DISP_IF_MODE_COMMAND;
			else
				dispif_info->displayMode = DISP_IF_MODE_VIDEO;
		}

		dispif_info->isHwVsyncAvailable = 1;
		dispif_info->vsyncFPS = 6000;
		dispif_info->physicalHeight = dispif_info->physicalWidth = 0;

		dispif_info->isConnected = 1;
/*
*		LCM_LOG("DEV_INFO configuration displayHeight:%u, displayWidth:%u, mode:%u\n",
*			dispif_info->displayHeight, dispif_info->displayWidth, dispif_info->displayMode);
*/
	}

	return ret;
}
/*
 *
	int lcm_power_enable(int enable)
	{
		if (enable) {
			ext_disp_resume(0x20003);
		} else {
			ext_disp_suspend(0x20003);
		}

		return 0;
	}
*/
void lcm_set_layer_num(int layer_num)
{
	if (layer_num >= 0)
		ovl_layer_num = layer_num;
}

int lcm_ioctl(unsigned int ioctl_cmd, int param1, int param2, unsigned long *params)
{
	/* HDMI_LOG("hdmi_ioctl ioctl_cmd:%d\n", ioctl_cmd); */
	int ret = 0;

	switch (ioctl_cmd) {
	case SET_LAYER_NUM_CMD:
		lcm_set_layer_num(param1);
		break;
	default:
		LCM_LOG("lcm_ioctl unknown command\n");
		break;
	}

	return ret;
}

int lcm_post_init(void)
{
	struct disp_lcm_handle *plcm;
	LCM_PARAMS *lcm_param;

	memset((void *)&extd_interface_params, 0, sizeof(LCM_PARAMS));

	extd_disp_get_interface((struct disp_lcm_handle **)&plcm);
	if (plcm && plcm->params && plcm->drv) {
		lcm_param = disp_lcm_get_params(plcm);
		if (lcm_param)
			memcpy(&extd_interface_params, lcm_param, sizeof(LCM_PARAMS));
	}

/*
*	init_waitqueue_head(&fence_release_wq);
*
*	if (!fence_release_task) {
*		disp_register_module_irq_callback(DISP_MODULE_RDMA, rdma_irq_handler);
*		fence_release_task = kthread_create(fence_release_kthread,
*							NULL, "fence_release_kthread");
*		wake_up_process(fence_release_task);
*	}
*/
	Extd_DBG_Init();
	return 0;
}
#endif

const struct EXTD_DRIVER *EXTD_LCM_Driver(void)
{
	static const struct EXTD_DRIVER extd_driver_lcm = {
#if (CONFIG_MTK_DUAL_DISPLAY_SUPPORT == 2)
		.post_init =	lcm_post_init,
		.get_dev_info =	lcm_get_dev_info,
		.ioctl =		lcm_ioctl,
		.power_enable =    NULL,
#else
		.init = 0,
#endif
	};

	return &extd_driver_lcm;
}
