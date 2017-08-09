#if defined(RDMA_DPI_PATH_SUPPORT) || defined(DPI_DVT_TEST_SUPPORT)
#ifdef BUILD_UBOOT
#include <asm/arch/disp_drv_platform.h>
#else
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include "cmdq_record.h"
#include <disp_drv_log.h>
#endif
#include <debug.h>
#include <mt-plat/sync_write.h>
#include <linux/types.h>
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#include <mach/irqs.h>
#include <linux/xlog.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "mtkfb.h"
#include "ddp_drv.h"
#include "ddp_hal.h"
#include "ddp_manager.h"
#include "ddp_dpi_reg.h"
#include "ddp_dpi.h"
#include "ddp_reg.h"
#include "ddp_log.h"

#include <linux/of.h>
#include <linux/of_irq.h>
#include <mach/eint.h>

#ifdef DPI_EXT_INREG32
#undef DPI_EXT_INREG32
#define DPI_EXT_INREG32(x)          (__raw_readl((unsigned long *)(x)))
#endif

#if 0
static int dpi_reg_op_debug;

#define DPI_EXT_OUTREG32(cmdq, addr, val) \
	{\
		if (dpi_reg_op_debug) \
			pr_warn("[dsi/reg]0x%08x=0x%08x, cmdq:0x%08x\n", addr, val, cmdq);\
		if (cmdq) \
			cmdqRecWrite(cmdq, (unsigned int)(addr)&0x1fffffff, val, ~0); \
		else \
			mt65xx_reg_sync_writel(val, addr); }
#else
#define DPI_EXT_OUTREG32(cmdq, addr, val) \
	{\
		mt_reg_sync_writel(val, addr); \
	}
#endif

#define DPI_EXT_LOG_PRINT(level, sub_module, fmt, arg...)  \
	{\
		pr_warn(fmt, ##arg); \
	}

/*****************************DPI DVT Case Start********************************/
int configInterlaceMode(unsigned int resolution)
{
	DPI_EXT_LOG_PRINT("enableInterlaceMode, resolution: %d\n", resolution);

	if (resolution == 0x0D || resolution == 0x0E) {	/*HDMI_VIDEO_720x480i_60Hz */
		/*Enable Interlace mode */
		DPI_REG_CNTL ctr = DPI_REG->CNTL;

		ctr.INTL_EN = 1;
		ctr.VS_LODD_EN = 1;
		ctr.VS_LEVEN_EN = 1;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctr));

		/*Set LODD,LEVEN Vsize */
		DPI_REG_SIZE size = DPI_REG->SIZE;

		size.HEIGHT = 240;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->SIZE, AS_UINT32(&size));

		/*Set LODD,VFP/VPW/VBP */
		DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd = DPI_REG->TGEN_VWIDTH_LODD;

		vwidth_lodd.VPW_LODD = 3;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
		DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LODD: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x28));


		DPI_REG_TGEN_VPORCH_LODD vporch_lodd = DPI_REG->TGEN_VPORCH_LODD;

		vporch_lodd.VFP_LODD = 4;
		vporch_lodd.VBP_LODD = 15;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));
		DPI_EXT_LOG_PRINT("TGEN_VPORCH_LODD: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x2C));

		/*Set LEVEN,VFP/VPW/VBP */
		DPI_REG_TGEN_VWIDTH_LEVEN vwidth_leven = DPI_REG->TGEN_VWIDTH_LEVEN;

		vwidth_leven.VPW_LEVEN = 3;
		vwidth_leven.VPW_HALF_LEVEN = 1;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_LEVEN, AS_UINT32(&vwidth_leven));
		DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LEVEN: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x68));

		DPI_REG_TGEN_VPORCH_LEVEN vporch_leven = DPI_REG->TGEN_VPORCH_LEVEN;

		vporch_leven.VFP_LEVEN = 4;
		vporch_leven.VBP_LEVEN = 16;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_LEVEN, AS_UINT32(&vporch_leven));
		DPI_EXT_LOG_PRINT("TGEN_VPORCH_LEVEN: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x6C));
	} else if (resolution == 0x0C) {	/*HDMI_VIDEO_1920x1080i_60Hz */
		/*Enable Interlace mode */
		DPI_REG_CNTL ctr = DPI_REG->CNTL;

		ctr.INTL_EN = 1;
		ctr.VS_LODD_EN = 1;
		ctr.VS_LEVEN_EN = 1;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctr));

		/*Set LODD,LEVEN Vsize */
		DPI_REG_SIZE size = DPI_REG->SIZE;

		size.HEIGHT = 540;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->SIZE, AS_UINT32(&size));

		/*Set LODD,VFP/VPW/VBP */
		DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd = DPI_REG->TGEN_VWIDTH_LODD;

		vwidth_lodd.VPW_LODD = 5;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
		DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LODD: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x28));


		DPI_REG_TGEN_VPORCH_LODD vporch_lodd = DPI_REG->TGEN_VPORCH_LODD;

		vporch_lodd.VFP_LODD = 2;
		vporch_lodd.VBP_LODD = 15;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));
		DPI_EXT_LOG_PRINT("TGEN_VPORCH_LODD: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x2C));

		/*Set LEVEN,VFP/VPW/VBP */
		DPI_REG_TGEN_VWIDTH_LEVEN vwidth_leven = DPI_REG->TGEN_VWIDTH_LEVEN;

		vwidth_leven.VPW_LEVEN = 5;
		vwidth_leven.VPW_HALF_LEVEN = 1;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_LEVEN, AS_UINT32(&vwidth_leven));
		DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LEVEN: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x68));

		DPI_REG_TGEN_VPORCH_LEVEN vporch_leven = DPI_REG->TGEN_VPORCH_LEVEN;

		vporch_leven.VFP_LEVEN = 2;
		vporch_leven.VBP_LEVEN = 16;
		DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_LEVEN, AS_UINT32(&vporch_leven));
		DPI_EXT_LOG_PRINT("TGEN_VPORCH_LEVEN: 0x%x\n",
				  DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x6C));
	}

	return 0;
}

int config3DMode(unsigned int resolution)
{
	DPI_EXT_LOG_PRINT("config3DMode\n");

	/*Enable Interlace mode */
	DPI_REG_CNTL ctr = DPI_REG->CNTL;

	ctr.TDFP_EN = 1;
	ctr.VS_RODD_EN = 0;
	ctr.FAKE_DE_RODD = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctr));

	/*Set LODD,LEVEN Vsize */
	DPI_REG_SIZE size = DPI_REG->SIZE;

	size.WIDTH = 1280;
	size.HEIGHT = 720;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->SIZE, AS_UINT32(&size));

	/*Set LODD,VFP/VPW/VBP */
	DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd = DPI_REG->TGEN_VWIDTH_LODD;

	vwidth_lodd.VPW_LODD = 5;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
	DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LODD: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x28));


	DPI_REG_TGEN_VPORCH_LODD vporch_lodd = DPI_REG->TGEN_VPORCH_LODD;

	vporch_lodd.VFP_LODD = 5;
	vporch_lodd.VBP_LODD = 20;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));
	DPI_EXT_LOG_PRINT("TGEN_VPORCH_LODD: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x2C));

	/*Set LEVEN,VFP/VPW/VBP */
	DPI_REG_TGEN_VWIDTH_RODD vwidth_rodd = DPI_REG->TGEN_VWIDTH_RODD;

	vwidth_rodd.VPW_RODD = 5;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_RODD, AS_UINT32(&vwidth_rodd));
	DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LEVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x70));

	DPI_REG_TGEN_VPORCH_RODD vporch_rodd = DPI_REG->TGEN_VPORCH_RODD;

	vporch_rodd.VFP_RODD = 5;
	vporch_rodd.VBP_RODD = 20;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_RODD, AS_UINT32(&vporch_rodd));
	DPI_EXT_LOG_PRINT("TGEN_VPORCH_LEVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x74));

	return 0;
}

int config3DInterlaceMode(unsigned int resolution)
{
	DPI_EXT_LOG_PRINT("config3DInterlaceMode\n");

	/*Enable Interlace mode */
	DPI_REG_CNTL ctr = DPI_REG->CNTL;

	ctr.INTL_EN = 1;
	ctr.TDFP_EN = 1;

	ctr.VS_LODD_EN = 1;
	ctr.VS_LEVEN_EN = 0;
	ctr.VS_RODD_EN = 0;
	ctr.VS_REVEN_EN = 0;

	ctr.FAKE_DE_LODD = 0;
	ctr.FAKE_DE_RODD = 1;
	ctr.FAKE_DE_LEVEN = 1;
	ctr.FAKE_DE_REVEN = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctr));

	/*Set LODD,LEVEN Vsize */
	DPI_REG_SIZE size = DPI_REG->SIZE;

	size.HEIGHT = 240;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->SIZE, AS_UINT32(&size));

	/*Set LODD,VFP/VPW/VBP */
	DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd = DPI_REG->TGEN_VWIDTH_LODD;

	vwidth_lodd.VPW_LODD = 3;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
	DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LODD: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x28));


	DPI_REG_TGEN_VPORCH_LODD vporch_lodd = DPI_REG->TGEN_VPORCH_LODD;

	vporch_lodd.VFP_LODD = 4;
	vporch_lodd.VBP_LODD = 15;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));
	DPI_EXT_LOG_PRINT("TGEN_VPORCH_LODD: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x2C));



	/*Set LEVEN,VFP/VPW/VBP */
	DPI_REG_TGEN_VWIDTH_RODD vwidth_rodd = DPI_REG->TGEN_VWIDTH_RODD;

	vwidth_rodd.VPW_RODD = 3;
	vwidth_rodd.VPW_HALF_RODD = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_RODD, AS_UINT32(&vwidth_rodd));
	DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LEVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x70));

	DPI_REG_TGEN_VPORCH_RODD vporch_rodd = DPI_REG->TGEN_VPORCH_RODD;

	vporch_rodd.VFP_RODD = 4;
	vporch_rodd.VBP_RODD = 16;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_RODD, AS_UINT32(&vporch_rodd));
	DPI_EXT_LOG_PRINT("TGEN_VPORCH_LEVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x74));



	/*Set LEVEN,VFP/VPW/VBP */
	DPI_REG_TGEN_VWIDTH_LEVEN vwidth_leven = DPI_REG->TGEN_VWIDTH_LEVEN;

	vwidth_leven.VPW_LEVEN = 3;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_LEVEN, AS_UINT32(&vwidth_leven));
	DPI_EXT_LOG_PRINT("TGEN_VWIDTH_LEVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x68));


	DPI_REG_TGEN_VPORCH_LEVEN vporch_leven = DPI_REG->TGEN_VPORCH_LEVEN;

	vporch_leven.VFP_LEVEN = 4;
	vporch_leven.VBP_LEVEN = 15;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_LEVEN, AS_UINT32(&vporch_leven));
	DPI_EXT_LOG_PRINT("TGEN_VPORCH_LEVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x6C));



	/*Set REVEN,VFP/VPW/VBP */
	DPI_REG_TGEN_VWIDTH_REVEN vwidth_reven = DPI_REG->TGEN_VWIDTH_REVEN;

	vwidth_reven.VPW_REVEN = 3;
	vwidth_reven.VPW_HALF_REVEN = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VWIDTH_REVEN, AS_UINT32(&vwidth_reven));
	DPI_EXT_LOG_PRINT("TGEN_VWIDTH_REVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x78));

	DPI_REG_TGEN_VPORCH_REVEN vporch_reven = DPI_REG->TGEN_VPORCH_REVEN;

	vporch_reven.VFP_REVEN = 4;
	vporch_reven.VBP_REVEN = 16;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->TGEN_VPORCH_REVEN, AS_UINT32(&vporch_reven));
	DPI_EXT_LOG_PRINT("TGEN_VPORCH_REVEN: 0x%x\n", DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x7C));
}

unsigned int readDPIStatus(void)
{
	unsigned int status = DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x40);
	unsigned int field = (status >> 20) & 0x01;
	unsigned int line_num = status & 0x1FFF;

	return field;
}

unsigned int readDPITDLRStatus(void)
{
	unsigned int status = DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x40);
	unsigned int tdlr = (status >> 21) & 0x01;

	return tdlr;
}

unsigned int clearDPIStatus(void)
{
	OUTREG32(&DPI_REG->STATUS, 0);

	return 0;
}

unsigned int clearDPIIntrStatus(void)
{
	OUTREG32(&DPI_REG->INT_STATUS, 0);

	return 0;
}


unsigned int readDPIIntrStatus(void)
{
	unsigned int status = DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x0C);
	return status;
}

unsigned int ClearDPIIntrStatus(void)
{
	OUTREG32(&DPI_REG->INT_STATUS, 0);

	return 0;
}

unsigned int enableRGB2YUV(AviColorSpace_e format)
{
	DPI_EXT_LOG_PRINT("enableRGB2YUV\n");

	DPI_REG_CNTL ctr = DPI_REG->CNTL;
	DPI_REG_OUTPUT_SETTING output_setting = DPI_REG->OUTPUT_SETTING;

	if (format == acsYCbCr444) {
		ctr.RGB2YUV_EN = 1;
		output_setting.YC_MAP = 7;
		output_setting.DUAL_EDGE_SEL = 0;
	} else if (format == acsYCbCr422) {
		ctr.RGB2YUV_EN = 1;
		ctr.YUV422_EN = 1;
		ctr.CLPF_EN = 1;

		output_setting.YC_MAP = 7;
	}

	/*
	   DPI_OUTPUT_SETTING  YUV422 bit mapping, output bit number
	   DPI_YUV422_SETTING
	 */
	DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctr));

	DPI_EXT_OUTREG32(NULL, &DPI_REG->OUTPUT_SETTING, AS_UINT32(&output_setting));
	return 0;
}

unsigned int enableSingleEdge(void)
{
	DPI_EXT_LOG_PRINT("enableSingleEdge\n");

	/*
	   Pixel clock = 2 * dpi clock
	   Disable dual edge
	 */
	DPI_REG_DDR_SETTING ddr_setting = DPI_REG->DDR_SETTING;

	ddr_setting.DDR_EN = 0;
	ddr_setting.DDR_4PHASE = 0;

	DPI_EXT_OUTREG32(NULL, &DPI_REG->OUTPUT_SETTING, AS_UINT32(&ddr_setting));
	return 0;
}

int enableAndGetChecksum(void)
{
	DPI_EXT_LOG_PRINT("enableAndGetChecksum\n");

	int checkSumNum = -1;
	DPI_REG_CHKSUM checkSum = DPI_REG->CHKSUM;

	checkSum.CHKSUM_EN = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->CHKSUM, AS_UINT32(&checkSum));

	while (((DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x48) >> 30) & 0x1) == 0) {
		checkSumNum = (DPI_EXT_INREG32(DISPSYS_DPI_BASE + 0x48) & 0x00FFFFFF);
		break;
	}

	return checkSumNum;
}

/*
int enableAndGetChecksumCmdq(cmdqRecHandle cmdq_handle)
{
	DPI_EXT_LOG_PRINT("enableAndGetChecksumCmdq\n");

	//loop
	{
		//poll vsync status
		cmdqRecPoll(cmdq_handle, DPI_PHY_ADDR+0xC, 0x1, 0x1);

		//stop DPI
		cmdqRecWrite(cmdq_handle, DPI_PHY_ADDR, 0x0, 0x1);

		//wait a token, wait vsync isr to read checksum and then set token
		cmdqRecWait(cmdq_handle, CMDQ_SYNC_TOKEN_STREAM_EOF);
		cmdqRecClearEventToken(cmdq_handle, CMDQ_SYNC_TOKEN_STREAM_EOF);

		//start DPI
		cmdqRecWrite(cmdq_handle, DPI_PHY_ADDR, 0x1, 0x1);

		//wait a token, wait vsync isr to read checksum and then set token
		cmdqRecWait(cmdq_handle, CMDQ_SYNC_TOKEN_STREAM_EOF);
		cmdqRecClearEventToken(cmdq_handle, CMDQ_SYNC_TOKEN_STREAM_EOF);

		//enable checksum
		cmdqRecWrite(cmdq_handle, DPI_PHY_ADDR+0x48, 0x80000000, 0x80000000);
		DPI_EXT_LOG_PRINT("DISPSYS_DPI_BASE+0x48: 0x%x\n", DISPSYS_DPI_BASE+0x48);

		//poll checksum status
		cmdqRecPoll(cmdq_handle, DPI_PHY_ADDR+0x48, 0x40000000, 0x40000000);

		// dump trigger loop instructions
	    cmdqRecDumpCommand(cmdq_handle);

		cmdqRecStartLoop(cmdq_handle);

		CMDQ_Handle = cmdq_handle;
		s_isCmdqInited = true;
	}

	return 0;
}
*/

unsigned int configDpiRepetition(void)
{
	DPI_EXT_LOG_PRINT("configDpiRepetition\n");

	DPI_REG_CNTL ctrl = DPI_REG->CNTL;

	ctrl.PIXREP = 1;

	DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctrl));

	return 0;
}

unsigned int configDpiEmbsync(void)
{
	DPI_EXT_LOG_PRINT("configDpiEmbsync\n");

	DPI_REG_CNTL ctrl = DPI_REG->CNTL;

	ctrl.EMBSYNC_EN = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctrl));

	DPI_REG_DDR_SETTING ddr_setting = DPI_REG->DDR_SETTING;

	ddr_setting.DATA_THROT = 0;	/*should sii8348 support */
	DPI_EXT_OUTREG32(NULL, &DPI_REG->DDR_SETTING, AS_UINT32(&ddr_setting));

	DPI_REG_EMBSYNC_SETTING emb_setting = DPI_REG->EMBSYNC_SETTING;

	emb_setting.EMBSYNC_OPT = 0;	/*should sii8348 support */
	DPI_EXT_OUTREG32(NULL, &DPI_REG->EMBSYNC_SETTING, AS_UINT32(&emb_setting));

	return 0;
}

unsigned int configDpiColorTransformToBT709(void)
{
	DPI_EXT_LOG_PRINT("configDpiColorTransform\n");

/*
	DPI_REG_CNTL ctrl = DPI_REG->CNTL;
	ctrl.RGB2YUV_EN = 1;
	DPI_EXT_OUTREG32(NULL,  &DPI_REG->CNTL, AS_UINT32(&ctrl));
*/

	DPI_REG_MATRIX_SET matrix_set = DPI_REG->MATRIX_SET;

	matrix_set.EXT_MATRIX_EN = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_SET, AS_UINT32(&matrix_set));

	DPI_REG_MATRIX_COEF matrix_coef_00 = DPI_REG->MATRIX_COEF_00;

	matrix_coef_00.MATRIX_COFEF_00 = 218;
	matrix_coef_00.MATRIX_COFEF_01 = 732;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_00, AS_UINT32(&matrix_coef_00));

	DPI_REG_MATRIX_COEF matrix_coef_02 = DPI_REG->MATRIX_COEF_02;

	matrix_coef_02.MATRIX_COFEF_00 = 74;
	matrix_coef_02.MATRIX_COFEF_01 = 8075;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_02, AS_UINT32(&matrix_coef_02));

	DPI_REG_MATRIX_COEF matrix_coef_11 = DPI_REG->MATRIX_COEF_11;

	matrix_coef_11.MATRIX_COFEF_00 = 7797;
	matrix_coef_11.MATRIX_COFEF_01 = 512;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_11, AS_UINT32(&matrix_coef_11));

	DPI_REG_MATRIX_COEF matrix_coef_20 = DPI_REG->MATRIX_COEF_20;

	matrix_coef_20.MATRIX_COFEF_00 = 512;
	matrix_coef_20.MATRIX_COFEF_01 = 7727;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_20, AS_UINT32(&matrix_coef_20));

	DPI_REG_MATRIX_COEF_ONE matrix_coef_22 = DPI_REG->MATRIX_COEF_22;

	matrix_coef_22.MATRIX_COFEF_00 = 8145;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_22, AS_UINT32(&matrix_coef_22));



	DPI_REG_MATRIX_OFFSET matrix_in_offset_0 = DPI_REG->MATRIX_IN_OFFSET_0;

	matrix_in_offset_0.MATRIX_OFFSET_0 = 0;
	matrix_in_offset_0.MATRIX_OFFSET_1 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_IN_OFFSET_0, AS_UINT32(&matrix_in_offset_0));

	DPI_REG_MATRIX_OFFSET_ONE matrix_in_offset_2 = DPI_REG->MATRIX_IN_OFFSET_2;

	matrix_in_offset_2.MATRIX_OFFSET_0 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_IN_OFFSET_2, AS_UINT32(&matrix_in_offset_2));

	DPI_REG_MATRIX_OFFSET matrix_out_offset_0 = DPI_REG->MATRIX_OUT_OFFSET_0;

	matrix_out_offset_0.MATRIX_OFFSET_0 = 0;
	matrix_out_offset_0.MATRIX_OFFSET_1 = 128;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_OUT_OFFSET_0, AS_UINT32(&matrix_out_offset_0));

	DPI_REG_MATRIX_OFFSET_ONE matrix_out_offset_2 = DPI_REG->MATRIX_OUT_OFFSET_2;

	matrix_out_offset_2.MATRIX_OFFSET_0 = 128;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_OUT_OFFSET_2, AS_UINT32(&matrix_out_offset_2));

	return 0;
}

unsigned int configDpiRGB888ToLimitRange(void)
{
	DPI_EXT_LOG_PRINT("configDpiRGB888ToLimitRange\n");

/*
	DPI_REG_CNTL ctrl = DPI_REG->CNTL;
	ctrl.RGB2YUV_EN = 1;
	DPI_EXT_OUTREG32(NULL,  &DPI_REG->CNTL, AS_UINT32(&ctrl));
*/
	DPI_REG_MATRIX_SET matrix_set = DPI_REG->MATRIX_SET;

	matrix_set.EXT_MATRIX_EN = 1;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_SET, AS_UINT32(&matrix_set));

	DPI_REG_MATRIX_COEF matrix_coef_00 = DPI_REG->MATRIX_COEF_00;

	matrix_coef_00.MATRIX_COFEF_00 = 879;
	matrix_coef_00.MATRIX_COFEF_01 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_00, AS_UINT32(&matrix_coef_00));

	DPI_REG_MATRIX_COEF matrix_coef_02 = DPI_REG->MATRIX_COEF_02;

	matrix_coef_02.MATRIX_COFEF_00 = 0;
	matrix_coef_02.MATRIX_COFEF_01 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_02, AS_UINT32(&matrix_coef_02));

	DPI_REG_MATRIX_COEF matrix_coef_11 = DPI_REG->MATRIX_COEF_11;

	matrix_coef_11.MATRIX_COFEF_00 = 879;
	matrix_coef_11.MATRIX_COFEF_01 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_11, AS_UINT32(&matrix_coef_11));

	DPI_REG_MATRIX_COEF matrix_coef_20 = DPI_REG->MATRIX_COEF_20;

	matrix_coef_20.MATRIX_COFEF_00 = 0;
	matrix_coef_20.MATRIX_COFEF_01 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_20, AS_UINT32(&matrix_coef_20));

	DPI_REG_MATRIX_COEF_ONE matrix_coef_22 = DPI_REG->MATRIX_COEF_22;

	matrix_coef_22.MATRIX_COFEF_00 = 879;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_COEF_22, AS_UINT32(&matrix_coef_22));



	DPI_REG_MATRIX_OFFSET matrix_in_offset_0 = DPI_REG->MATRIX_IN_OFFSET_0;

	matrix_in_offset_0.MATRIX_OFFSET_0 = 0;
	matrix_in_offset_0.MATRIX_OFFSET_1 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_IN_OFFSET_0, AS_UINT32(&matrix_in_offset_0));

	DPI_REG_MATRIX_OFFSET_ONE matrix_in_offset_2 = DPI_REG->MATRIX_IN_OFFSET_2;

	matrix_in_offset_2.MATRIX_OFFSET_0 = 0;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_IN_OFFSET_2, AS_UINT32(&matrix_in_offset_2));

	DPI_REG_MATRIX_OFFSET matrix_out_offset_0 = DPI_REG->MATRIX_OUT_OFFSET_0;

	matrix_out_offset_0.MATRIX_OFFSET_0 = 16;
	matrix_out_offset_0.MATRIX_OFFSET_1 = 16;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_OUT_OFFSET_0, AS_UINT32(&matrix_out_offset_0));

	DPI_REG_MATRIX_OFFSET_ONE matrix_out_offset_2 = DPI_REG->MATRIX_OUT_OFFSET_2;

	matrix_out_offset_2.MATRIX_OFFSET_0 = 16;
	DPI_EXT_OUTREG32(NULL, &DPI_REG->MATRIX_OUT_OFFSET_2, AS_UINT32(&matrix_out_offset_2));

	return 0;
}

void ddp_dpi_ConfigInputSwap(unsigned int order)
{
	DPI_REG_CNTL ctrl = DPI_REG->CNTL;

	ctrl.RGB_SWAP = order;
	DPI_EXT_LOG_PRINT("ddp_dpi_ConfigInputSwap: 0x%x\n", order);

	DPI_EXT_OUTREG32(NULL, &DPI_REG->CNTL, AS_UINT32(&ctrl));
}

int mux_test_read(void)
{
	DPI_EXT_LOG_PRINT("********** mux register dump *********\n");
	DPI_EXT_LOG_PRINT("[CLK_CFG_0]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_0));
	DPI_EXT_LOG_PRINT("[CLK_CFG_1]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_1));
	DPI_EXT_LOG_PRINT("[CLK_CFG_2]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_2));
	DPI_EXT_LOG_PRINT("[CLK_CFG_3]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_3));
	DPI_EXT_LOG_PRINT("[CLK_CFG_4]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_4));
	DPI_EXT_LOG_PRINT("[CLK_CFG_5]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_5));
	DPI_EXT_LOG_PRINT("[CLK_CFG_6]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_6));
	DPI_EXT_LOG_PRINT("[CLK_CFG_UPDATE]=0x%08x\n", DPI_EXT_INREG32(CLK_CFG_UPDATE));
	DPI_EXT_LOG_PRINT("[DISPSYS_IO_DRIVING]=0x%08x\n", DPI_EXT_INREG32(DISPSYS_IO_DRIVING));

	return 0;
}

/*****************************DPI DVT Case End*********************************/
#endif				/*#if defined(RDMA_DPI_PATH_SUPPORT) || defined(DPI_DVT_TEST_SUPPORT) */
