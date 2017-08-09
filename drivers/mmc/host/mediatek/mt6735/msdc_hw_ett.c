/* weiping fix */
#if 0
#include <mach/board.h>
#include <mach/mt_chip.h>
#endif

#include "mt_sd.h"
#include "msdc_hw_ett.h"

#ifdef CONFIG_ARCH_MT6735M
#ifdef MSDC_SUPPORT_SANDISK_COMBO_ETT
struct msdc_ett_settings msdc0_ett_settings_for_sandisk[] = {
	{MSDC_HS200_MODE, 0xb0, (0x7 << 7), 0},
	{MSDC_HS200_MODE, 0xb0, (0x1f << 10), 0},
	{MSDC_HS400_MODE, 0xb0, (0x7 << 7), 0},
	{MSDC_HS400_MODE, 0xb0, (0x1f << 10), 0},
	{MSDC_HS400_MODE, 0x188, (0x1f << 2), 2 /*0x0 */ },
	{MSDC_HS400_MODE, 0x188, (0x1f << 12), 18 /*0x13 */ },

	/* command & resp ett settings */
	{MSDC_HS200_MODE, 0xb4, (0x7 << 3), 1},
	{MSDC_HS200_MODE, 0x4, (0x1 << 1), 1},
	{MSDC_HS200_MODE, 0xf0, (0x1f << 16), 0},
	{MSDC_HS200_MODE, 0xf0, (0x1f << 22), 6},

	{MSDC_HS400_MODE, 0xb4, (0x7 << 3), 1},
	{MSDC_HS400_MODE, 0x4, (0x1 << 1), 1},
	{MSDC_HS400_MODE, 0xf0, (0x1f << 16), 0},
	{MSDC_HS400_MODE, 0xf0, (0x1f << 22), 11 /*0x0 */ },

	/* write ett settings */
	{MSDC_HS200_MODE, 0xb4, (0x7 << 0), 1},
	{MSDC_HS200_MODE, 0xf0, (0x1f << 0), 15},
	{MSDC_HS200_MODE, 0x4, (0x1 << 10), 1},
	{MSDC_HS200_MODE, 0xf8, (0x1f << 24), 5},

	/* read ett settings */
	{MSDC_HS200_MODE, 0xf0, (0x1f << 8), 18},
	{MSDC_HS200_MODE, 0x4, (0x1 << 2), 3},
};
#endif				/* mt6735m MSDC_SUPPORT_SANDISK_COMBO_ETT */
#ifdef MSDC_SUPPORT_SAMSUNG_COMBO_ETT
struct msdc_ett_settings msdc0_ett_settings_for_samsung[] = {
	{MSDC_HS200_MODE, 0xb0, (0x7 << 7), 0},
	{MSDC_HS200_MODE, 0xb0, (0x1f << 10), 0},
	{MSDC_HS400_MODE, 0xb0, (0x7 << 7), 0},
	{MSDC_HS400_MODE, 0xb0, (0x1f << 10), 0},
	{MSDC_HS400_MODE, 0x188, (0x1f << 2), 2 /*0x0 */ },
	{MSDC_HS400_MODE, 0x188, (0x1f << 12), 18 /*0x13 */ },

	/* command & resp ett settings */
	{MSDC_HS200_MODE, 0xb4, (0x7 << 3), 1},
	{MSDC_HS200_MODE, 0x4, (0x1 << 1), 1},
	{MSDC_HS200_MODE, 0xf0, (0x1f << 16), 0},
	{MSDC_HS200_MODE, 0xf0, (0x1f << 22), 6},

	{MSDC_HS400_MODE, 0xb4, (0x7 << 3), 1},
	{MSDC_HS400_MODE, 0x4, (0x1 << 1), 1},
	{MSDC_HS400_MODE, 0xf0, (0x1f << 16), 0},
	{MSDC_HS400_MODE, 0xf0, (0x1f << 22), 11 /*0x0 */ },

	/* write ett settings */
	{MSDC_HS200_MODE, 0xb4, (0x7 << 0), 1},
	{MSDC_HS200_MODE, 0xf0, (0x1f << 0), 15},
	{MSDC_HS200_MODE, 0x4, (0x1 << 10), 1},
	{MSDC_HS200_MODE, 0xf8, (0x1f << 24), 5},

	/* read ett settings */
	{MSDC_HS200_MODE, 0xf0, (0x1f << 8), 18},
	{MSDC_HS200_MODE, 0x4, (0x1 << 2), 4},
};
#endif				/* mt6735m MSDC_SUPPORT_SAMSUNG_COMBO_ETT */

#endif

int msdc_setting_parameter(struct msdc_hw *hw, unsigned int *para)
{
	struct tag_msdc_hw_para *msdc_para_hw_datap = (struct tag_msdc_hw_para *)para;

	if (NULL == hw)
		return -1;

	if ((msdc_para_hw_datap != NULL) &&
	    ((msdc_para_hw_datap->host_function == MSDC_SD) || (msdc_para_hw_datap->host_function == MSDC_EMMC)) &&
	    (msdc_para_hw_datap->version == 0x5A01) && (msdc_para_hw_datap->end_flag == 0x5a5a5a5a)) {
		hw->clk_src = msdc_para_hw_datap->clk_src;
		hw->cmd_edge = msdc_para_hw_datap->cmd_edge;
		hw->rdata_edge = msdc_para_hw_datap->rdata_edge;
		hw->wdata_edge = msdc_para_hw_datap->wdata_edge;
		hw->clk_drv = msdc_para_hw_datap->clk_drv;
		hw->cmd_drv = msdc_para_hw_datap->cmd_drv;
		hw->dat_drv = msdc_para_hw_datap->dat_drv;
		hw->rst_drv = msdc_para_hw_datap->rst_drv;
		hw->ds_drv = msdc_para_hw_datap->ds_drv;
		hw->clk_drv_sd_18 = msdc_para_hw_datap->clk_drv_sd_18;
		hw->cmd_drv_sd_18 = msdc_para_hw_datap->cmd_drv_sd_18;
		hw->dat_drv_sd_18 = msdc_para_hw_datap->dat_drv_sd_18;
		hw->clk_drv_sd_18_sdr50 = msdc_para_hw_datap->clk_drv_sd_18_sdr50;
		hw->cmd_drv_sd_18_sdr50 = msdc_para_hw_datap->cmd_drv_sd_18_sdr50;
		hw->dat_drv_sd_18_sdr50 = msdc_para_hw_datap->dat_drv_sd_18_sdr50;
		hw->clk_drv_sd_18_ddr50 = msdc_para_hw_datap->clk_drv_sd_18_ddr50;
		hw->cmd_drv_sd_18_ddr50 = msdc_para_hw_datap->cmd_drv_sd_18_ddr50;
		hw->dat_drv_sd_18_ddr50 = msdc_para_hw_datap->dat_drv_sd_18_ddr50;
		hw->flags = msdc_para_hw_datap->flags;
		hw->data_pins = msdc_para_hw_datap->data_pins;
		hw->data_offset = msdc_para_hw_datap->data_offset;

		hw->ddlsel = msdc_para_hw_datap->ddlsel;
		hw->rdsplsel = msdc_para_hw_datap->rdsplsel;
		hw->wdsplsel = msdc_para_hw_datap->wdsplsel;

		hw->dat0rddly = msdc_para_hw_datap->dat0rddly;
		hw->dat1rddly = msdc_para_hw_datap->dat1rddly;
		hw->dat2rddly = msdc_para_hw_datap->dat2rddly;
		hw->dat3rddly = msdc_para_hw_datap->dat3rddly;
		hw->dat4rddly = msdc_para_hw_datap->dat4rddly;
		hw->dat5rddly = msdc_para_hw_datap->dat5rddly;
		hw->dat6rddly = msdc_para_hw_datap->dat6rddly;
		hw->dat7rddly = msdc_para_hw_datap->dat7rddly;
		hw->datwrddly = msdc_para_hw_datap->datwrddly;
		hw->cmdrrddly = msdc_para_hw_datap->cmdrrddly;
		hw->cmdrddly = msdc_para_hw_datap->cmdrddly;
		hw->host_function = msdc_para_hw_datap->host_function;
		hw->boot = msdc_para_hw_datap->boot;
		hw->cd_level = msdc_para_hw_datap->cd_level;
		return 0;
	}
	return -1;
}

#if defined(CFG_DEV_MSDC0)
#if defined(CONFIG_MTK_EMMC_SUPPORT)
struct msdc_ett_settings msdc0_ett_settings[] = {
	/* common ett settings */
	{MSDC_HS200_MODE, 0xb0, (0x7 << 7), 0x0},	/* PATCH_BIT0[MSDC_PB0_INT_DAT_LATCH_CK_SEL]*/
	{MSDC_HS200_MODE, 0xb0, (0x1f << 10), 0x0},	/*PATCH_BIT0[MSDC_PB0_CKGEN_MSDC_DLY_SEL]*/
	{MSDC_HS400_MODE, 0xb0, (0x7 << 7), 0x0},	/*PATCH_BIT0[MSDC_PB0_INT_DAT_LATCH_CK_SEL]*/
	{MSDC_HS400_MODE, 0xb0, (0x1f << 10), 0x0},	/*PATCH_BIT0[MSDC_PB0_CKGEN_MSDC_DLY_SEL]*/
	{MSDC_HS400_MODE, 0x188, (0x1f << 2), 0x2 /*0x0 */ },	/*EMMC50_PAD_DS_TUNE[MSDC_EMMC50_PAD_DS_TUNE_DLY1]*/
	{MSDC_HS400_MODE, 0x188, (0x1f << 12), 0x10 /*0x13 */ },
	/*EMMC50_PAD_DS_TUNE[MSDC_EMMC50_PAD_DS_TUNE_DLY3]*/

	/* command & resp ett settings */
	{MSDC_HS200_MODE, 0xb4, (0x7 << 3), 0x1},	/*PATCH_BIT1[MSDC_PB1_CMD_RSP_TA_CNTR]*/
	{MSDC_HS200_MODE, 0x4, (0x1 << 1), 0x0},	/*MSDC_IOCON[MSDC_IOCON_RSPL]*/
	{MSDC_HS200_MODE, 0xf0, (0x1f << 16), 0x7},	/*PAD_TUNE[MSDC_PAD_TUNE_CMDRDLY]*/
	{MSDC_HS200_MODE, 0xf0, (0x1f << 22), 0xb},	/*PAD_TUNE[MSDC_PAD_TUNE_CMDRRDLY]*/

	{MSDC_HS400_MODE, 0xb4, (0x7 << 3), 0x1},	/*PATCH_BIT1[MSDC_PB1_CMD_RSP_TA_CNTR]*/
	{MSDC_HS400_MODE, 0x4, (0x1 << 1), 0x0},	/*MSDC_IOCON[MSDC_IOCON_RSPL]*/
	{MSDC_HS400_MODE, 0xf0, (0x1f << 16), 0x6},	/*PAD_TUNE[MSDC_PAD_TUNE_CMDRDLY]*/
	{MSDC_HS400_MODE, 0xf0, (0x1f << 22), 0x6 /*0x0 */ },	/*PAD_TUNE[MSDC_PAD_TUNE_CMDRRDLY]*/

	/* write ett settings */
	{MSDC_HS200_MODE, 0xb4, (0x7 << 0), 0x1},	/*PATCH_BIT1[MSDC_PB1_WRDAT_CRCS_TA_CNTR]*/
	{MSDC_HS200_MODE, 0xf0, (0x1f << 0), 0xb},	/*PAD_TUNE[MSDC_PAD_TUNE_DATWRDLY]*/
	{MSDC_HS200_MODE, 0x4, (0x1 << 10), 0x0},	/*MSDC_IOCON[MSDC_IOCON_W_D0SPL]*/
	{MSDC_HS200_MODE, 0xf8, (0x1f << 24), 0x7},	/*DAT_RD_DLY0[MSDC_DAT_RDDLY0_D0]*/

	/* read ett settings */
	{MSDC_HS200_MODE, 0xf0, (0x1f << 8), 0x9},	/*PAD_TUNE[MSDC_PAD_TUNE_DATRRDLY]*/
	{MSDC_HS200_MODE, 0x4, (0x1 << 2), 0x0},	/*MSDC_IOCON[MSDC_IOCON_R_D_SMPL]*/
};

struct msdc_hw msdc0_hw = {
	.clk_src = MSDC50_CLKSRC_400MHZ,
	.cmd_edge = MSDC_SMPL_FALLING,
	.rdata_edge = MSDC_SMPL_FALLING,
	.wdata_edge = MSDC_SMPL_FALLING,
	.clk_drv = 2,
	.cmd_drv = 2,
	.dat_drv = 2,
	.rst_drv = 2,
	.ds_drv = 2,
	.data_pins = 8,
	.data_offset = 0,
#ifndef CONFIG_MTK_EMMC_CACHE
	.flags = MSDC_SYS_SUSPEND | MSDC_HIGHSPEED | MSDC_DDR ,/*| MSDC_UHS1 | MSDC_HS400,*/
#else
	.flags = MSDC_SYS_SUSPEND | MSDC_HIGHSPEED | MSDC_CACHE | MSDC_DDR ,/*| MSDC_UHS1 | MSDC_HS400,*/
#endif
	.dat0rddly = 0,
	.dat1rddly = 0,
	.dat2rddly = 0,
	.dat3rddly = 0,
	.dat4rddly = 0,
	.dat5rddly = 0,
	.dat6rddly = 0,
	.dat7rddly = 0,
	.datwrddly = 0,
	.cmdrrddly = 0,
	.cmdrddly = 0,
	.ett_count = 20,	/*should be same with ett_settings array size*/
	.ett_settings = (struct msdc_ett_settings *)msdc0_ett_settings,
	.host_function = MSDC_EMMC,
	.boot = MSDC_BOOT_EN,
};
#else
struct msdc_hw msdc0_hw = {
	.clk_src = MSDC50_CLKSRC_200MHZ,
	.cmd_edge = MSDC_SMPL_FALLING,
	.rdata_edge = MSDC_SMPL_FALLING,
	.wdata_edge = MSDC_SMPL_FALLING,
	.clk_drv = 1,
	.cmd_drv = 1,
	.dat_drv = 1,
	.clk_drv_sd_18 = 1,	/* sdr104 mode */
	.cmd_drv_sd_18 = 1,
	.dat_drv_sd_18 = 1,
	.clk_drv_sd_18_sdr50 = 1,	/* sdr50 mode */
	.cmd_drv_sd_18_sdr50 = 1,
	.dat_drv_sd_18_sdr50 = 1,
	.clk_drv_sd_18_ddr50 = 1,	/* ddr50 mode */
	.cmd_drv_sd_18_ddr50 = 1,
	.dat_drv_sd_18_ddr50 = 1,
	.data_pins = 4,
	.data_offset = 0,
	/* #ifdef CUST_EINT_MSDC1_INS_NUM*/
#if 0
	.flags = MSDC_SYS_SUSPEND | MSDC_CD_PIN_EN | MSDC_REMOVABLE | MSDC_HIGHSPEED | MSDC_UHS1 | MSDC_DDR,
#else
	.flags = MSDC_SYS_SUSPEND | MSDC_HIGHSPEED,	/* | MSDC_UHS1 |MSDC_DDR,*/
#endif
	.dat0rddly = 0,
	.dat1rddly = 0,
	.dat2rddly = 0,
	.dat3rddly = 0,
	.dat4rddly = 0,
	.dat5rddly = 0,
	.dat6rddly = 0,
	.dat7rddly = 0,
	.datwrddly = 0,
	.cmdrrddly = 0,
	.cmdrddly = 0,
	.ett_count = 0,		/*should be same with ett_settings array size*/
	.host_function = MSDC_SD,
	.boot = 0,
	.cd_level = MSDC_CD_LOW,
};
#endif
#endif

#if defined(CFG_DEV_MSDC1)
struct msdc_hw msdc1_hw = {
	.clk_src = MSDC30_CLKSRC_200MHZ,
	.cmd_edge = MSDC_SMPL_FALLING,
	.rdata_edge = MSDC_SMPL_FALLING,
	.wdata_edge = MSDC_SMPL_FALLING,
	.clk_drv = 3,
	.cmd_drv = 3,
	.dat_drv = 3,
	.clk_drv_sd_18 = 3,	/* sdr104 mode */
	.cmd_drv_sd_18 = 2,
	.dat_drv_sd_18 = 2,
	.clk_drv_sd_18_sdr50 = 3,	/* sdr50 mode */
	.cmd_drv_sd_18_sdr50 = 2,
	.dat_drv_sd_18_sdr50 = 2,
	.clk_drv_sd_18_ddr50 = 3,	/* ddr50 mode */
	.cmd_drv_sd_18_ddr50 = 2,
	.dat_drv_sd_18_ddr50 = 2,
	.data_pins = 4,
	.data_offset = 0,
#ifdef CUST_EINT_MSDC1_INS_NUM
	.flags = MSDC_SYS_SUSPEND | MSDC_CD_PIN_EN | MSDC_REMOVABLE | MSDC_HIGHSPEED | MSDC_UHS1 | MSDC_DDR,
#else
	.flags = MSDC_SYS_SUSPEND | MSDC_HIGHSPEED | MSDC_UHS1 | MSDC_DDR,
#endif
	.dat0rddly = 0,
	.dat1rddly = 0,
	.dat2rddly = 0,
	.dat3rddly = 0,
	.dat4rddly = 0,
	.dat5rddly = 0,
	.dat6rddly = 0,
	.dat7rddly = 0,
	.datwrddly = 0,
	.cmdrrddly = 0,
	.cmdrddly = 0,
	.ett_count = 0,		/* should be same with ett_settings array size*/
	.host_function = MSDC_SD,
	.boot = 0,
	.cd_level = MSDC_CD_LOW,
};
#endif
