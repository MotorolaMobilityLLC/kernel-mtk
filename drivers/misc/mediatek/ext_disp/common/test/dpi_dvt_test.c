#if defined(RDMA_DPI_PATH_SUPPORT) || defined(DPI_DVT_TEST_SUPPORT)
#define VENDOR_CHIP_DRIVER

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/err.h>
#include <linux/switch.h>


#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <mach/mt_irq.h>
#include <linux/types.h>
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#include <mach/m4u.h>
#include <mach/m4u_port.h>
#include "ddp_log.h"
#include "ddp_dump.h"
#include "ddp_info.h"

#include "dpi_dvt_test.h"
#include "dpi_dvt_platform.h"

DPI_U64 data_va = 0, data_mva = 0;

/*
To-do list:
1.  Open RDMA_DPI_PATH_SUPPORT or DPI_DVT_TEST_SUPPORT;
2.  extern picture data structure and modify Makefile,
     e.g.  extern unsigned char kara_1280x720[2764800];
3.  Modify sii8348 driver;
*/

/***********************************************************************************/

/**********************************Resource File***************************************/
/*#define BMP_HEADER_SIZE 54*/
/*
extern unsigned char kara_1280x720[2764800];
extern unsigned char Gene_1280x720[2764854];
extern unsigned char Kara_1440x480[2073654];
extern unsigned char Gene_1280x720[2764854];
extern unsigned char venice_1920x1080[6220854];
extern unsigned char PDA0026_720x480[1036854];
extern unsigned char Kara_1440x480[2073654];
*/

/******************************DPI DVT Path Control*************************************/
DPI_DVT_CONTEXT DPI_DVT_Context;
DPI_BOOL disconnectFlag = true;

static HDMI_DRIVER *hdmi_drv;
static disp_ddp_path_config extd_dpi_params;
static disp_ddp_path_config extd_rdma_params;
static disp_ddp_path_config extd_ovl_params;
static disp_ddp_path_config extd_gamma_params;

/*TEST_CASE_TYPE test_type;*/
DPI_U64 data_va = 0, data_mva = 0
/*****************************Basic Function Start******************************************/
#ifdef DPI_DVT_TEST_SUPPORT
enum RDMA_MODE {
	RDMA_MODE_DIRECT_LINK = 0,
	RDMA_MODE_MEMORY = 1,
};

DPI_I32 dvt_init_OVL_param(DPI_U32 mode)
{
	if (mode == RDMA_MODE_DIRECT_LINK) {
		extd_ovl_params.dst_dirty = 1;
		extd_ovl_params.dst_w = extd_dpi_params.dispif_config.dpi.width;
		extd_ovl_params.dst_h = extd_dpi_params.dispif_config.dpi.height;
		extd_ovl_params.ovl_dirty = 1;

		extd_ovl_params.ovl_config[0].layer_en = 1;
		extd_ovl_params.ovl_config[1].layer_en = 0;
		extd_ovl_params.ovl_config[2].layer_en = 0;
		extd_ovl_params.ovl_config[3].layer_en = 0;

		extd_ovl_params.ovl_config[0].source = OVL_LAYER_SOURCE_MEM;
		extd_ovl_params.ovl_config[0].layer = 0;
		extd_ovl_params.ovl_config[0].fmt = eRGB888;
		extd_ovl_params.ovl_config[0].addr = data_mva;
		extd_ovl_params.ovl_config[0].vaddr = data_va;

		extd_ovl_params.ovl_config[0].src_x = 0;
		extd_ovl_params.ovl_config[0].src_y = 0;
		extd_ovl_params.ovl_config[0].src_w = extd_dpi_params.dispif_config.dpi.width;
		extd_ovl_params.ovl_config[0].src_h = extd_dpi_params.dispif_config.dpi.height;
		extd_ovl_params.ovl_config[0].src_pitch =
		    extd_dpi_params.dispif_config.dpi.width * 3;

		extd_ovl_params.ovl_config[0].dst_x = 0;
		extd_ovl_params.ovl_config[0].dst_y = 0;
		extd_ovl_params.ovl_config[0].dst_w = extd_dpi_params.dispif_config.dpi.width;
		extd_ovl_params.ovl_config[0].dst_h = extd_dpi_params.dispif_config.dpi.height;

		extd_ovl_params.ovl_config[0].isDirty = true;

	}

	return 0;
}

DPI_I32 dvt_init_GAMMA_param(DPI_U32 mode)
{
	if (mode == RDMA_MODE_DIRECT_LINK) {
		extd_gamma_params.dst_dirty = 1;
		extd_gamma_params.dst_w = extd_dpi_params.dispif_config.dpi.width;
		extd_gamma_params.dst_h = extd_dpi_params.dispif_config.dpi.height;
		extd_gamma_params.lcm_bpp = 24;
	}

	return 0;
}

/*
* OVL1 -> OVL1_MOUT -> DISP_RDMA1 -> RDMA1_SOUT -> DPI_SEL -> DPI
*/
DPI_I32 dvt_start_ovl1_to_dpi(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_ovl1_dpi(NULL);
	dvt_mutex_set_ovl1_dpi(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_LONG1, NULL);
	ddp_driver_ovl.init(DISP_MODULE_OVL1, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_LONG1, NULL);
	ddp_driver_ovl.power_on(DISP_MODULE_OVL1, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	ret = dvt_allocate_buffer(resolution, M4U_FOR_OVL1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_DIRECT_LINK);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_LONG1, &extd_rdma_params, NULL);

	dvt_init_OVL_param(RDMA_MODE_DIRECT_LINK);
	ddp_driver_ovl.config(DISP_MODULE_OVL1, &extd_ovl_params, NULL);
	configOVL0Layer0Swap(DISP_MODULE_OVL1, 0);	/* BGR:0,  RGB:1 */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_LONG1, NULL);
	ddp_driver_ovl.reset(DISP_MODULE_OVL1, NULL);


	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_LONG1, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);
	ddp_driver_ovl.start(DISP_MODULE_OVL1, NULL);

	msleep(3 * 1000);
	hdmi_drv->video_config(resolution, 0, 0);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_LONG1, 1);
	ddp_driver_ovl.dump_info(DISP_MODULE_OVL1, NULL);

	ddp_dump_reg(DISP_MODULE_CONFIG);
	ddp_dump_analysis(DISP_MODULE_CONFIG);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_LONG1, NULL);
	ddp_driver_ovl.stop(DISP_MODULE_OVL1, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());

	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_LONG1, NULL);
	ddp_driver_ovl.reset(DISP_MODULE_OVL1, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_LONG1, NULL);
	ddp_driver_ovl.power_off(DISP_MODULE_OVL1, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_ovl1_dpi(NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

/*
* OVL0 -> OVL0_MOUT -> COLOR0_SEL -> COLOR0 -> CCORRO -> AAL0 -> GAMMA0 -> DITHER0 -> DITHER0_MUT
* DISP_RDMA0 -> RDMA0_SOUT -> DPI_SEL -> DPI
*/
DPI_I32 dvt_start_ovl0_to_dpi(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_ovl0_dpi(NULL);
	dvt_mutex_set_ovl0_dpi(HW_MUTEX_FOR_UPLINK, NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_LONG0, NULL);
	ddp_driver_ovl.init(DISP_MODULE_OVL0, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_LONG0, NULL);
	ddp_driver_ovl.power_on(DISP_MODULE_OVL0, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	ret = dvt_allocate_buffer(resolution, M4U_FOR_OVL0);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_DIRECT_LINK);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_LONG0, &extd_rdma_params, NULL);

	dvt_init_OVL_param(RDMA_MODE_DIRECT_LINK);
	ddp_driver_ovl.config(DISP_MODULE_OVL0, &extd_ovl_params, NULL);
	configOVL0Layer0Swap(DISP_MODULE_OVL0, 0);	/* BGR:0,  RGB:1 */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, HW_MUTEX_FOR_UPLINK);
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_LONG0, NULL);
	ddp_driver_ovl.reset(DISP_MODULE_OVL0, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, HW_MUTEX_FOR_UPLINK);
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_LONG0, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);
	ddp_driver_ovl.start(DISP_MODULE_OVL0, NULL);

	msleep(3 * 1000);
	hdmi_drv->video_config(resolution, 0, 0);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_LONG0, 1);
	ddp_driver_ovl.dump_info(DISP_MODULE_OVL0, NULL);
	ddp_dump_reg(DISP_MODULE_CONFIG);
	ddp_dump_analysis(DISP_MODULE_CONFIG);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, HW_MUTEX_FOR_UPLINK);
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_LONG0, NULL);
	ddp_driver_ovl.stop(DISP_MODULE_OVL0, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, HW_MUTEX_FOR_UPLINK);
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_LONG0, NULL);
	ddp_driver_ovl.reset(DISP_MODULE_OVL0, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_LONG0, NULL);
	ddp_driver_ovl.power_off(DISP_MODULE_OVL0, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_ovl0_dpi(NULL);

	if (data_va)
		vfree(data_va);
	return 0;
}

/**********************************ROME LONG PATH End*****************************************/

/*
 * 1. register irq;
 * 2. aquire mutex and connect RDMA2 -> DPI
 * 3. enable Top clock and IRQ
 * 4. power on
 *
 * 5. configure -> start -> stop -> reset
 *
 * 6. power off
 * 7. close top clock and disable IRQ
 * 8. release mutex and disconnect RDMA2 -> DPI
 * 9. unregister irq
*/

DPI_I32 dpi_dvt_power_on(void)
{
	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);

	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
}

DPI_I32 dpi_dvt_show(DPI_U32 timeS)
{
	DPI_DVT_LOG_W("dpi_dvt_show\n");

	DPI_EnableColorBar(COLOR_BAR_PATTERN);
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	DPI_DisableColorBar();

	DPI_DVT_LOG_W("module power Off\n");

	return 0;
}

DPI_I32 dpi_dvt_show_pattern(DPI_U32 pattern, DPI_U32 timeS)
{
	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);

	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	DPI_EnableColorBar(pattern);

	if (pattern == 100)
		ddp_dpi_EnableColorBar_0();
	else if (pattern == 99)
		ddp_dpi_EnableColorBar_16();

	if (timeS == 100)
		enableRGB2YUV(acsYCbCr444);
	else if (timeS == 99) {
		configDpiRepetition();
		configInterlaceMode(HDMI_VIDEO_720x480i_60Hz);
	}

	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);
	msleep(50);

	hdmi_drv->video_config(DPI_DVT_Context.output_video_resolution, 0, 0);

	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);

	msleep(timeS * 1000);

	DPI_DisableColorBar();
	DPI_DVT_LOG_W("module stop\n");
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module power Off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);

	return 0;
}

DPI_I32 dpi_dvt_show_pattern_for_limit_range(DPI_U32 pattern, DPI_U32 timeS)
{
	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);

	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);

	if (pattern != 100)
		configDpiRGB888ToLimitRange();
	ddp_dpi_EnableColorBar_16();

	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(DPI_DVT_Context.output_video_resolution, 0, 0);

	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);

	msleep(timeS * 1000);

	DPI_DisableColorBar();
	DPI_DVT_LOG_W("module stop\n");
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module power Off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_checkSum(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;
	DPI_U32 loop_num = 1000;
	DPI_I32 checkSum = -1;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	/*_test_cmdq_build_trigger_loop();*/

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); *//*input_swap, format   11:RGB, 01 BGR */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);


	hdmi_drv->video_config(resolution, 0, 0);

	/*_test_cmdq_get_checkSum();*/

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_interlace(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;
	DPI_U32 loop_num = 10000;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	configInterlaceMode(resolution);
	if (timeS == 98)
		configDpiRepetition();
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	extd_rdma_params.rdma_config.pitch = extd_dpi_params.dispif_config.dpi.width * 3 * 2;
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); */

	/*
	   _test_cmdq_build_trigger_loop();
	   _test_cmdq_for_interlace(data_mva, (data_mva + (extd_dpi_params.dispif_config.dpi.width * 3)));
	 */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);


	hdmi_drv->video_config(resolution, 0, 0);
	msleep(50);

	for (;;) {
		wait_event_interruptible(hdmi_dpi_config_wq, atomic_read(&hdmi_dpi_config_event));
		atomic_set(&hdmi_dpi_config_event, 0);

		if (readDPIStatus() == 0) {
			set_rdma2_address(DISP_MODULE_RDMA_SHORT, data_mva);
		} else {
			set_rdma2_address(DISP_MODULE_RDMA_SHORT,
					  (data_mva +
					   (extd_dpi_params.dispif_config.dpi.width * 3)));
		}
		clearDPIStatus();

		if (disconnectFlag)
			break;
	}

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_RGB2YUV(DPI_U32 resolution, DPI_U32 timeS, AviColorSpace_e format)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	enableRGB2YUV(format);
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(resolution, 0, 0);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_3D(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;
	DPI_U32 loop_num = 10000;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	config3DMode(resolution);
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(resolution, 0, 0);
	msleep(50);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	for (;;) {
		wait_event_interruptible(hdmi_dpi_config_wq, atomic_read(&hdmi_dpi_config_event));
		atomic_set(&hdmi_dpi_config_event, 0);

		if (readDPIStatus() == 0)
			set_rdma2_address(DISP_MODULE_RDMA_SHORT, data_mva);
		else
			set_rdma2_address(DISP_MODULE_RDMA_SHORT, data_mva);

		if (disconnectFlag)
			break;
	}

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_3D_interlace(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;
	DPI_U32 loop_num = 10000;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	config3DInterlaceMode(resolution);
	if (resolution == HDMI_VIDEO_720x480i_60Hz)
		configDpiRepetition();

	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	extd_rdma_params.rdma_config.pitch = extd_dpi_params.dispif_config.dpi.width * 3 * 2;
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(resolution, 0, 0);
	msleep(50);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	for (;;) {
		wait_event_interruptible(hdmi_dpi_config_wq, atomic_read(&hdmi_dpi_config_event));
		atomic_set(&hdmi_dpi_config_event, 0);

		if (readDPIStatus() == 0) {
			set_rdma2_address(DISP_MODULE_RDMA_SHORT, data_mva);
		} else {
			set_rdma2_address(DISP_MODULE_RDMA_SHORT,
					  (data_mva +
					   (extd_dpi_params.dispif_config.dpi.width * 3)));
		}
		clearDPIStatus();

		if (disconnectFlag)
			break;
	}

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_Single_Edge(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	enableSingleEdge();
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(resolution, 0, 0);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_embsync(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	configDpiEmbsync();
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	extd_rdma_params.rdma_config.pitch = extd_dpi_params.dispif_config.dpi.width * 3 * 2;
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(resolution, 0, 0);
	msleep(50);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_color_space_transform(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	configDpiColorTransformToBT709();
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	/*configRDMASwap(0, 1); */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(resolution, 0, 0);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

DPI_I32 dvt_start_rdma_to_dpi_for_limit_range_transform(DPI_U32 resolution, DPI_U32 timeS)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	configDpiRGB888ToLimitRange();
	/*configRDMASwap(0, 1); */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

	hdmi_drv->video_config(resolution, 0, 0);

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}
#endif				/*#ifdef DPI_DVT_TEST_SUPPORT */

DPI_I32 dvt_init_RDMA_param(DPI_U32 mode)
{
	if (mode == RDMA_MODE_DIRECT_LINK) {
		DPI_DVT_LOG_W("RDMA_MODE_DIRECT_LINK configure\n");
		extd_rdma_params.dst_dirty = 1;
		extd_rdma_params.dst_w = extd_dpi_params.dispif_config.dpi.width;
		extd_rdma_params.dst_h = extd_dpi_params.dispif_config.dpi.height;
	} else if (mode == RDMA_MODE_MEMORY) {
		DPI_DVT_LOG_W("RDMA_MODE_MEMORY configure\n");
		extd_rdma_params.rdma_dirty = 1;
		extd_rdma_params.rdma_config.address = data_mva;
		extd_rdma_params.rdma_config.inputFormat = UFMT_RGB888;
		extd_rdma_params.rdma_config.pitch = extd_dpi_params.dispif_config.dpi.width * 3;
		extd_rdma_params.rdma_config.width = extd_dpi_params.dispif_config.dpi.width;
		extd_rdma_params.rdma_config.height = extd_dpi_params.dispif_config.dpi.height;
	}

	return 0;
}

void dpi_dvt_parameters(DPI_U8 arg)
{
	DPI_POLARITY clk_pol, de_pol, hsync_pol, vsync_pol;
	DPI_U32 dpi_clock = 0;
	DPI_U32 dpi_clk_div, dpi_clk_duty, hsync_pulse_width, hsync_back_porch, hsync_front_porch,
	    vsync_pulse_width, vsync_back_porch, vsync_front_porch, intermediat_buffer_num;

	switch (arg) {
	case HDMI_VIDEO_720x480p_60Hz:
		{
#if defined(MHL_SII8338) || defined(MHL_SII8348)
			clk_pol = HDMI_POLARITY_FALLING;
#else
			clk_pol = HDMI_POLARITY_RISING;
#endif
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;

			dpi_clk_div = 2;

			hsync_pulse_width = 62;
			hsync_back_porch = 60;
			hsync_front_porch = 16;

			vsync_pulse_width = 6;
			vsync_back_porch = 30;
			vsync_front_porch = 9;

			DPI_DVT_Context.bg_height =
			    ((480 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.bg_width =
			    ((720 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.hdmi_width = 720 - DPI_DVT_Context.bg_width;
			DPI_DVT_Context.hdmi_height = 480 - DPI_DVT_Context.bg_height;
			DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_720x480p_60Hz;
			dpi_clock = 27027;
			break;
		}
	case HDMI_VIDEO_720x480i_60Hz:
		{
#if defined(MHL_SII8338) || defined(MHL_SII8348)
			clk_pol = HDMI_POLARITY_FALLING;
#else
			clk_pol = HDMI_POLARITY_RISING;
#endif
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;

			dpi_clk_div = 2;

			hsync_pulse_width = 124 / 2;
			hsync_back_porch = 114 / 2;
			hsync_front_porch = 38 / 2;

			vsync_pulse_width = 6;
			vsync_back_porch = 30;
			vsync_front_porch = 9;

			DPI_DVT_Context.bg_height =
			    ((480 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.bg_width =
			    ((1440 / 2 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.hdmi_width = 1440 / 2 - DPI_DVT_Context.bg_width;
			DPI_DVT_Context.hdmi_height = 480 - DPI_DVT_Context.bg_height;
			DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_720x480i_60Hz;
			dpi_clock = 27027;
			break;
		}
	case HDMI_VIDEO_720x480i_60Hz:
		{
#if defined(MHL_SII8338) || defined(MHL_SII8348)
			clk_pol = HDMI_POLARITY_FALLING;
#else
			clk_pol = HDMI_POLARITY_RISING;
#endif
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;

			dpi_clk_div = 2;

			hsync_pulse_width = 124 / 2;
			hsync_back_porch = 114 / 2;
			hsync_front_porch = 38 / 2;

			vsync_pulse_width = 6;
			vsync_back_porch = 30;
			vsync_front_porch = 9;

			DPI_DVT_Context.bg_height =
			    ((480 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.bg_width =
			    ((1440 / 2 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.hdmi_width = 1440 / 2 - DPI_DVT_Context.bg_width;
			DPI_DVT_Context.hdmi_height = 480 - DPI_DVT_Context.bg_height;
			DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_720x480i_60Hz;
			dpi_clock = 27027;
			break;
		}
	case HDMI_VIDEO_1280x720p_60Hz:
		{
			clk_pol = HDMI_POLARITY_RISING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(HDMI_TDA19989)
			hsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_FALLING;
#endif
			vsync_pol = HDMI_POLARITY_FALLING;

#if defined(MHL_SII8338) || defined(MHL_SII8348)
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 40;
			hsync_back_porch = 220;
			hsync_front_porch = 110;

			vsync_pulse_width = 5;
			vsync_back_porch = 20;
			vsync_front_porch = 5;
			dpi_clock = 74250;

			DPI_DVT_Context.bg_height =
			    ((720 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.bg_width =
			    ((1280 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.hdmi_width = 1280 - DPI_DVT_Context.bg_width;
			DPI_DVT_Context.hdmi_height = 720 - DPI_DVT_Context.bg_height;

			DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_1280x720p_60Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080p_30Hz:
		{
#if defined(MHL_SII8338) || defined(MHL_SII8348)
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			clk_pol = HDMI_POLARITY_RISING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			DPI_DVT_Context.bg_height =
			    ((1080 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.bg_width =
			    ((1920 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.hdmi_width = 1920 - DPI_DVT_Context.bg_width;
			DPI_DVT_Context.hdmi_height = 1080 - DPI_DVT_Context.bg_height;

			DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_1920x1080p_30Hz;
			dpi_clock = 74250;
			break;
		}
	case HDMI_VIDEO_1920x1080p_60Hz:
		{
#if defined(MHL_SII8338) || defined(MHL_SII8348)
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			clk_pol = HDMI_POLARITY_RISING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			DPI_DVT_Context.bg_height =
			    ((1080 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.bg_width =
			    ((1920 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.hdmi_width = 1920 - DPI_DVT_Context.bg_width;
			DPI_DVT_Context.hdmi_height = 1080 - DPI_DVT_Context.bg_height;

			DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_1920x1080p_60Hz;
			dpi_clock = 148500;
			break;
		}
	case HDMI_VIDEO_1920x1080i_60Hz:
		{
#if defined(MHL_SII8338) || defined(MHL_SII8348)
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			clk_pol = HDMI_POLARITY_RISING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			DPI_DVT_Context.bg_height =
			    ((1080 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.bg_width =
			    ((1920 * DPI_DVT_Context.scaling_factor) / 100 >> 2) << 2;
			DPI_DVT_Context.hdmi_width = 1920 - DPI_DVT_Context.bg_width;
			DPI_DVT_Context.hdmi_height = 1080 - DPI_DVT_Context.bg_height;

			DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_1920x1080i_60Hz;
			dpi_clock = 74250;
			break;
		}

	default:
		break;
	}

	extd_dpi_params.dispif_config.dpi.width = DPI_DVT_Context.hdmi_width;
	extd_dpi_params.dispif_config.dpi.height = DPI_DVT_Context.hdmi_height;
	extd_dpi_params.dispif_config.dpi.bg_width = DPI_DVT_Context.bg_width;
	extd_dpi_params.dispif_config.dpi.bg_height = DPI_DVT_Context.bg_height;

	extd_dpi_params.dispif_config.dpi.clk_pol = clk_pol;
	extd_dpi_params.dispif_config.dpi.de_pol = de_pol;
	extd_dpi_params.dispif_config.dpi.vsync_pol = vsync_pol;
	extd_dpi_params.dispif_config.dpi.hsync_pol = hsync_pol;

	extd_dpi_params.dispif_config.dpi.hsync_pulse_width = hsync_pulse_width;
	extd_dpi_params.dispif_config.dpi.hsync_back_porch = hsync_back_porch;
	extd_dpi_params.dispif_config.dpi.hsync_front_porch = hsync_front_porch;
	extd_dpi_params.dispif_config.dpi.vsync_pulse_width = vsync_pulse_width;
	extd_dpi_params.dispif_config.dpi.vsync_back_porch = vsync_back_porch;
	extd_dpi_params.dispif_config.dpi.vsync_front_porch = vsync_front_porch;

	extd_dpi_params.dispif_config.dpi.format = 0;
	extd_dpi_params.dispif_config.dpi.rgb_order = 0;
	extd_dpi_params.dispif_config.dpi.i2x_en = true;
	extd_dpi_params.dispif_config.dpi.i2x_edge = 2;
	extd_dpi_params.dispif_config.dpi.embsync = false;
	extd_dpi_params.dispif_config.dpi.dpi_clock = dpi_clock;

	DPI_DVT_LOG_W("dpi_dvt_parameters:%d\n", arg);
}

/*****************************Basic Function End********************************************/

void dvt_dump_ext_dpi_parameters(void)
{
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.width:			%d\n",
		      extd_dpi_params.dispif_config.dpi.width);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.height:			%d\n",
		      extd_dpi_params.dispif_config.dpi.height);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.bg_width:			%d\n",
		      extd_dpi_params.dispif_config.dpi.bg_width);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.bg_height:		%d\n",
		      extd_dpi_params.dispif_config.dpi.bg_height);

	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.clk_pol:			%d\n",
		      extd_dpi_params.dispif_config.dpi.clk_pol);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.de_pol:			%d\n",
		      extd_dpi_params.dispif_config.dpi.de_pol);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.vsync_pol:		%d\n",
		      extd_dpi_params.dispif_config.dpi.vsync_pol);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.hsync_pol:		%d\n",
		      extd_dpi_params.dispif_config.dpi.hsync_pol);

	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.hsync_pulse_width: %d\n",
		      extd_dpi_params.dispif_config.dpi.hsync_pulse_width);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.hsync_back_porch:	%d\n",
		      extd_dpi_params.dispif_config.dpi.hsync_back_porch);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.hsync_front_porch: %d\n",
		      extd_dpi_params.dispif_config.dpi.hsync_front_porch);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.vsync_pulse_width: %d\n",
		      extd_dpi_params.dispif_config.dpi.vsync_pulse_width);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.vsync_back_porch:	%d\n",
		      extd_dpi_params.dispif_config.dpi.vsync_back_porch);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.vsync_front_porch: %d\n",
		      extd_dpi_params.dispif_config.dpi.vsync_front_porch);

	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.format:			%d\n",
		      extd_dpi_params.dispif_config.dpi.format);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.rgb_order:		%d\n",
		      extd_dpi_params.dispif_config.dpi.rgb_order);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.i2x_en:			%d\n",
		      extd_dpi_params.dispif_config.dpi.i2x_en);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.i2x_edge:			%d\n",
		      extd_dpi_params.dispif_config.dpi.i2x_edge);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.embsync:			%d\n",
		      extd_dpi_params.dispif_config.dpi.embsync);
	DPI_DVT_LOG_W("extd_dpi_params.dispif_config.dpi.dpi_clock:		%d\n",
		      extd_dpi_params.dispif_config.dpi.dpi_clock);
}

DPI_I32 dvt_copy_file_data(void *ptr, DPI_U32 resolution)
{
	DPI_U32 offset = 0;
	struct file *filp = NULL;
	struct inode *inode;
	mm_segment_t fs;
	off_t fsize;

	DPI_DVT_LOG_W("dvt_copy_file_data, resolution: %d\n", resolution);
	/*
	   if(resolution == HDMI_VIDEO_1280x720p_60Hz)
	   memcpy(ptr, kara_1280x720, 2764800);
	 */

	return 0;
}

DPI_I32 dvt_allocate_buffer(DPI_U32 resolution, HW_MODULE_Type hw_type)
{
	DPI_I32 ret = 0;
	M4U_PORT_STRUCT m4uport;
	m4u_client_t *client = NULL;
	DPI_I32 M4U_PORT = M4U_PORT_UNKNOWN;

	DPI_U32 hdmiPixelSize =
	    extd_dpi_params.dispif_config.dpi.width * extd_dpi_params.dispif_config.dpi.height;
	DPI_U32 hdmiDataSize = hdmiPixelSize * 3;
	DPI_U32 hdmiBufferSize = hdmiDataSize;

	DPI_DVT_LOG_W("dvt_allocate_buffer, width: %d, height: %d\n",
		      extd_dpi_params.dispif_config.dpi.width,
		      extd_dpi_params.dispif_config.dpi.height);
	data_va = (DPI_U64) vmalloc(hdmiBufferSize);
	if (((void *)data_va) == NULL) {
		DDPERR("vmalloc %d bytes fail\n", hdmiBufferSize);
		return -1;
	}
	memset((void *)data_va, 0, hdmiBufferSize);

	client = m4u_create_client();
	if (IS_ERR_OR_NULL(client)) {
		DDPERR("create client fail!\n");
		return -1;
	}

	if (hw_type == M4U_FOR_RDMA1)
		M4U_PORT = M4U_PORT_DISP_RDMA1;
#ifdef DPI_DVT_TEST_SUPPORT
	else if (hw_type == M4U_FOR_RDMA0)
		M4U_PORT = M4U_PORT_DISP_RDMA0;
	if (hw_type == M4U_FOR_OVL0)
		M4U_PORT = M4U_PORT_DISP_OVL0;
	else if (hw_type == M4U_FOR_OVL1)
		M4U_PORT = M4U_PORT_DISP_OVL1;
#endif

	m4u_alloc_mva(client, M4U_PORT, data_va, 0, hdmiBufferSize, M4U_PROT_READ | M4U_PROT_WRITE,
		      0, &data_mva);
	memset((void *)&m4uport, 0, sizeof(M4U_PORT_STRUCT));
	m4uport.ePortID = M4U_PORT;
	m4uport.Virtuality = 1;
	m4uport.domain = 0;
	m4uport.Security = 0;
	m4uport.Distance = 1;
	m4uport.Direction = 0;
	m4u_config_port(&m4uport);

	DPI_DVT_LOG_W("resolution %d\n", resolution);
	ret = dvt_copy_file_data(data_va, resolution);

	return ret;
}

DPI_I32 dvt_start_rdma_to_dpi(DPI_U32 resolution, DPI_U32 timeS, DPI_U32 caseNum)
{
	DPI_I32 ret = 0;

	DPI_DVT_LOG_W("set Mutex and connect path\n");
	dvt_connect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	DPI_DVT_LOG_W("top clock on\n");
	dvt_ddp_path_top_clock_on();

	DPI_DVT_LOG_W("module init\n");
	ddp_driver_dpi.init(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.init(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power on\n");
	ddp_driver_dpi.power_on(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_on(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module config\n");
	ddp_driver_dpi.config(DISP_MODULE_DPI, &extd_dpi_params, NULL);
	ret = dvt_allocate_buffer(resolution, M4U_FOR_RDMA1);
	if (ret < 0) {
		DDPERR("dvt_allocate_buffer error: ret: %d\n", ret);
		return ret;
	}

	dvt_init_RDMA_param(RDMA_MODE_MEMORY);
	ddp_driver_rdma.config(DISP_MODULE_RDMA_SHORT, &extd_rdma_params, NULL);
	DISP_REG_SET(NULL, 1 * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, 0x00000500);
	DISP_REG_SET(NULL, 1 * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_MEM_CON, 0x010);
	/*configRDMASwap(0, 1); *//*input_swap, format   11:RGB, 01 BGR */

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module trigger\n");
	dvt_mutex_enable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.trigger(DISP_MODULE_DPI, NULL);
	/*ddp_driver_rdma.trigger(DISP_MODULE_RDMA_SHORT, NULL); */

	DPI_DVT_LOG_W("module start\n");
	ddp_driver_rdma.start(DISP_MODULE_RDMA_SHORT, NULL);
	ddp_driver_dpi.start(DISP_MODULE_DPI, NULL);

#ifdef VENDOR_CHIP_DRIVER
	/*hdmi_drv->video_config(resolution, 0, 0); */
#endif

	DPI_DVT_LOG_W("module dump_info\n");
	ddp_driver_dpi.dump_info(DISP_MODULE_DPI, 1);
	ddp_driver_rdma.dump_info(DISP_MODULE_RDMA_SHORT, 1);
	dvt_mutex_dump_reg();

	msleep(timeS * 1000);

	DPI_DVT_LOG_W("module stop\n");
	dvt_mutex_disenable(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.stop(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.stop(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module reset\n");
	dvt_mutex_reset(NULL, dvt_acquire_mutex());
	ddp_driver_dpi.reset(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.reset(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("module power off\n");
	ddp_driver_dpi.power_off(DISP_MODULE_DPI, NULL);
	ddp_driver_rdma.power_off(DISP_MODULE_RDMA_SHORT, NULL);

	DPI_DVT_LOG_W("top clock off\n");
	dvt_ddp_path_top_clock_off();

	DPI_DVT_LOG_W("set Mutex and disconnect path\n");
	dvt_disconnect_path(NULL);
	dvt_mutex_set(dvt_acquire_mutex(), NULL);

	if (data_va)
		vfree(data_va);

	return 0;
}

/*******************************Test Case Start***********************************/
static DPI_I32 dpi_dvt_testcase_4_timing(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_4_timing, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();
	dvt_start_rdma_to_dpi(resolution, 20);

	return 0;
}

#ifdef DPI_DVT_TEST_SUPPORT
static DPI_I32 dpi_dvt_testcase_1_RB_Swap(DPI_U32 resolution, DPI_U32 swap)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_1_RB_Swap, swap: %d. (0: RGB,1: BGR)\n", swap);

	/* LCM_COLOR_ORDER_BGR  or  LCM_COLOR_ORDER_RGB */
	dpi_dvt_parameters(resolution);
	extd_dpi_params.dispif_config.dpi.rgb_order = swap;

	dvt_dump_ext_dpi_parameters();
	dpi_dvt_show_pattern(COLOR_BAR_PATTERN, 30 * 60 * 60)

	    return 0;
}

static DPI_I32 dpi_dvt_testcase_2_BG(DPI_U32 BG_factor)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_2_BG, BG_factor: %d\n", BG_factor);

	DPI_DVT_Context.scaling_factor = BG_factor;
	dpi_dvt_parameters(DPI_DVT_Context.output_video_resolution);

	dvt_dump_ext_dpi_parameters();
	dpi_dvt_show_pattern(COLOR_BAR_PATTERN, 10);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_3_pattern(DPI_U32 pattern)
{
	/* 0x01 -> 0x71 */
	DPI_DVT_LOG_W("dpi_dvt_testcase_3_pattern, pattern: %d\n", pattern);

	DPI_DVT_Context.output_video_resolution = HDMI_VIDEO_1920x1080p_30Hz;
	dpi_dvt_parameters(DPI_DVT_Context.output_video_resolution);
	dvt_dump_ext_dpi_parameters();

	dpi_dvt_show_pattern(pattern, 20);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_5_interlace(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_5_interlace, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_rdma_to_dpi_for_interlace(resolution, 30);


	return 0;
}

static DPI_I32 dpi_dvt_testcase_6_yuv(DPI_U32 resolution, AviColorSpace_e format)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_6_yuv444, resolution: %d, format: %d\n", resolution,
		      format);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_rdma_to_dpi_for_RGB2YUV(resolution, 30, format);
	return 0;
}

static DPI_I32 dpi_dvt_testcase_7_3D(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_7_3D, resolution: %d\n", resolution);

	/*
	   //for DPI DVT 3D Display
	   //hw_context->valid_audio_if = 0;
	   //hw_context->valid_vsif = 0;
	   //hw_context->valid_avif = 0;
	   //hw_context->valid_3d = 0;
	 */

	dpi_dvt_parameters(resolution);
	extd_dpi_params.dispif_config.dpi.dpi_clock *= 2;
	dvt_dump_ext_dpi_parameters();
	/*test_type = Test_3D; */

	dvt_start_rdma_to_dpi_for_3D(resolution, 20);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_8_3D_interlace(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_8_3D_interlace, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	extd_dpi_params.dispif_config.dpi.dpi_clock *= 2;
	dvt_dump_ext_dpi_parameters();
	/*test_type = Test_3D; */

	dvt_start_rdma_to_dpi_for_3D_interlace(resolution, 30);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_9_Single_Edge(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_9_Single_Edge, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_rdma_to_dpi_for_Single_Edge(resolution, 20);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_10_checkSum(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_10_checkSum, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_rdma_to_dpi_for_checkSum(resolution, 30);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_11_ovl1_to_dpi(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_11_ovl1_to_dpi, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_ovl1_to_dpi(resolution, 30);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_12_repetition(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_12_repetition, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();
	/*test_type = Test_Repeation; */

	dvt_start_rdma_to_dpi_for_interlace(resolution, 98);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_13_embsync(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_13_embsync, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_rdma_to_dpi_for_embsync(resolution, 10);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_14_color_space_transform(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_14_color_space_transform, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_rdma_to_dpi_for_color_space_transform(resolution, 10);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_15_limit_range_transform(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_15_limit_range_transform, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	/*test_type = Test_Limit_Range; */
	dpi_dvt_show_pattern_for_limit_range(0x41, 20);

	return 0;
}

static DPI_I32 dpi_dvt_testcase_16_ovl0_to_dpi(DPI_U32 resolution)
{
	DPI_DVT_LOG_W("dpi_dvt_testcase_16_ovl0_to_dpi, resolution: %d\n", resolution);

	dpi_dvt_parameters(resolution);
	dvt_dump_ext_dpi_parameters();

	dvt_start_ovl0_to_dpi(resolution, 20);

	return 0;
}
#endif

/*******************************Test Case End************************************/
DPI_I32 dpi_dvt_run_cases(DPI_U32 caseNum)
{
	switch (caseNum) {
	case 0x08:
		{
			dpi_dvt_testcase_4_timing(HDMI_VIDEO_1280x720p_60Hz);
			break;
		}
#ifdef DPI_DVT_TEST_SUPPORT
	case 0x1:
		{
			dpi_dvt_testcase_1_RB_Swap(HDMI_VIDEO_1280x720p_60Hz, DPI_COLOR_ORDER_BGR);
			msleep(500);
			dpi_dvt_testcase_1_RB_Swap(HDMI_VIDEO_1920x1080p_30Hz, DPI_COLOR_ORDER_RGB);
			break;
		}
	case 0x02:
		{
			dpi_dvt_testcase_2_BG(10);
			msleep(500);
			dpi_dvt_testcase_2_BG(50);
			msleep(500);
			dpi_dvt_testcase_2_BG(80);
			msleep(500);
			dpi_dvt_testcase_2_BG(100);
			msleep(500);
			dpi_dvt_testcase_2_BG(0);

			break;
		}
	case 0x04:
		{
			dpi_dvt_testcase_3_pattern(100);
			msleep(500);

			/*test_type = Test_Limit_Range; */
			dpi_dvt_testcase_3_pattern(99);
			msleep(500);

			dpi_dvt_testcase_3_pattern(0x41);
			msleep(500);

			dpi_dvt_testcase_3_pattern(0x11);
			msleep(500);
			dpi_dvt_testcase_3_pattern(0x21);
			msleep(500);
			dpi_dvt_testcase_3_pattern(0x31);
			msleep(500);

			dpi_dvt_testcase_3_pattern(0x41);
			msleep(500);

			dpi_dvt_testcase_3_pattern(0x51);
			msleep(500);
			dpi_dvt_testcase_3_pattern(0x61);
			msleep(500);
			dpi_dvt_testcase_3_pattern(0x71);

			break;
		}
	case 0x10:
		{

			dpi_dvt_testcase_5_interlace(HDMI_VIDEO_720x480i_60Hz);
			msleep(500);
			dpi_dvt_testcase_5_interlace(HDMI_VIDEO_1920x1080i_60Hz);
			dpi_dvt_testcase_5_interlace(HDMI_VIDEO_1440x480i_60Hz);

			break;

			dpi_dvt_testcase_5_interlace(HDMI_VIDEO_1920x1080i_60Hz);
			break;
		}
	case 0x20:
		{
			dpi_dvt_testcase_6_yuv(HDMI_VIDEO_1920x1080p_30Hz, acsYCbCr444);
			msleep(500);
			dpi_dvt_testcase_6_yuv(HDMI_VIDEO_1920x1080p_60Hz, acsYCbCr422);
			msleep(500);
			dpi_dvt_testcase_6_yuv(HDMI_VIDEO_1280x720p_60Hz, acsYCbCr444);
			msleep(500);

			break;
		}
	case 0x40:
		{
			dpi_dvt_testcase_7_3D(HDMI_VIDEO_1280x720p_60Hz);
			dpi_dvt_testcase_7_3D(HDMI_VIDEO_720x480p_60Hz);
			break;
		}
	case 0x80:
		{
			dpi_dvt_testcase_8_3D_interlace(HDMI_VIDEO_720x480i_60Hz);
			break;
		}
	case 0x100:
		{
			dpi_dvt_testcase_9_Single_Edge(HDMI_VIDEO_1920x1080p_60Hz);
			break;
		}
	case 0x200:
		{
			dpi_dvt_testcase_10_checkSum(HDMI_VIDEO_1920x1080p_30Hz);
			break;
		}
	case 0x400:
		{
			dpi_dvt_testcase_11_ovl1_to_dpi(HDMI_VIDEO_1920x1080p_60Hz);
			break;
		}
	case 0x800:
		{
			dpi_dvt_testcase_12_repetition(HDMI_VIDEO_720x480i_60Hz);
			break;
		}
	case 0x1000:
		{
			dpi_dvt_testcase_13_embsync(HDMI_VIDEO_1920x1080p_60Hz);
			break;
		}
	case 0x2000:
		{
			dpi_dvt_testcase_14_color_space_transform(HDMI_VIDEO_1920x1080p_60Hz);
			break;
			dpi_dvt_testcase_14_color_space_transform(HDMI_VIDEO_1920x1080p_30Hz);
			break;

		}
	case 0x4000:
		{
			dpi_dvt_testcase_15_limit_range_transform(HDMI_VIDEO_1920x1080p_30Hz);
			break;
		}
	case 0x8000:
		{
			dpi_dvt_testcase_16_ovl0_to_dpi(HDMI_VIDEO_1920x1080p_60Hz);
			break;
		}
#endif
	default:
		DDPERR("case number is invailed, case: %d\n", caseNum);
	}

	return 0;
}

unsigned int dpi_dvt_ioctl(unsigned int arg)
{
	int ret = 0;
	DPI_U32 flags, tempFlags, move;

	/*get HDMI Driver from vendor floder */
	hdmi_drv = (struct HDMI_DRIVER *)HDMI_GetDriver();
	if (NULL == hdmi_drv) {
		DPI_DVT_LOG_W("[hdmi]%s, hdmi_init fail, can not get hdmi driver handle\n",
			      __func__);
		return -1;
	}

	flags = arg;
	tempFlags = 0x1;
	move = 0;

	DPI_DVT_LOG_W("flags: 0x%x\n", flags);
	while (flags) {
		if (flags & 0x1) {
			tempFlags = (tempFlags << move);
			DPI_DVT_LOG_W("tempFlags: 0x%x\n", tempFlags);
			dpi_dvt_run_cases(tempFlags);

			tempFlags = 0x01;
		}
		flags = (flags >> 1);
		move += 1;
		DPI_DVT_LOG_W("flags: 0x%x, move: %d\n", flags, move);
	}

	return ret;
}
#else
unsigned int dpi_dvt_ioctl(unsigned int arg)
{
	return 0;
}
#endif
