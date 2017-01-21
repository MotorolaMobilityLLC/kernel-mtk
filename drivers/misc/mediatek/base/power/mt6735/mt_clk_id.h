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

#ifndef __MT_CLK_ID_H__
#define __MT_CLK_ID_H__

#include <mach/mt_clkmgr.h>

#if !defined(CONFIG_MTK_CLKMGR)
#if !defined(_MT_CLKMGR1_LEGACY_H) && !defined(_MT_CLKMGR2_H) \
		&& !defined(_MT_CLKMGR3_H)
enum cg_clk_id {
	MT_CG_INFRA_DBGCLK              = 0,
	MT_CG_INFRA_GCE                 = 1,
	MT_CG_INFRA_TRBG                = 2,
	MT_CG_INFRA_CPUM                = 3,
	MT_CG_INFRA_DEVAPC              = 4,
	MT_CG_INFRA_AUDIO               = 5,
	MT_CG_INFRA_GCPU                = 6,
	MT_CG_INFRA_L2C_SRAM            = 7,
	MT_CG_INFRA_M4U                 = 8,
	MT_CG_INFRA_CLDMA               = 12,
	MT_CG_INFRA_CONNMCU_BUS         = 15,
	MT_CG_INFRA_KP                  = 16,
	MT_CG_INFRA_APXGPT              = 18,
	MT_CG_INFRA_SEJ                 = 19,
	MT_CG_INFRA_CCIF0_AP            = 20,
	MT_CG_INFRA_CCIF1_AP            = 21,
	MT_CG_INFRA_PMIC_SPI            = 22,
	MT_CG_INFRA_PMIC_WRAP           = 23,

	MT_CG_PERI_DISP_PWM             = 0  + 32,
	MT_CG_PERI_THERM                = 1  + 32,
	MT_CG_PERI_PWM1                 = 2  + 32,
	MT_CG_PERI_PWM2                 = 3  + 32,
	MT_CG_PERI_PWM3                 = 4  + 32,
	MT_CG_PERI_PWM4                 = 5  + 32,
	MT_CG_PERI_PWM5                 = 6  + 32,
	MT_CG_PERI_PWM6                 = 7  + 32,
	MT_CG_PERI_PWM7                 = 8  + 32,
	MT_CG_PERI_PWM                  = 9  + 32,
	MT_CG_PERI_USB0                 = 10 + 32,
	MT_CG_PERI_IRDA                 = 11 + 32,
	MT_CG_PERI_APDMA                = 12 + 32,
	MT_CG_PERI_MSDC30_0             = 13 + 32,
	MT_CG_PERI_MSDC30_1             = 14 + 32,
	MT_CG_PERI_MSDC30_2             = 15 + 32,
	MT_CG_PERI_MSDC30_3             = 16 + 32,
	MT_CG_PERI_UART0                = 17 + 32,
	MT_CG_PERI_UART1                = 18 + 32,
	MT_CG_PERI_UART2                = 19 + 32,
	MT_CG_PERI_UART3                = 20 + 32,
	MT_CG_PERI_UART4                = 21 + 32,
	MT_CG_PERI_BTIF                 = 22 + 32,
	MT_CG_PERI_I2C0                 = 23 + 32,
	MT_CG_PERI_I2C1                 = 24 + 32,
	MT_CG_PERI_I2C2                 = 25 + 32,
	MT_CG_PERI_I2C3                 = 26 + 32,
	MT_CG_PERI_AUXADC               = 27 + 32,
	MT_CG_PERI_SPI0                 = 28 + 32,
	MT_CG_PERI_IRTX                 = 29 + 32,

	MT_CG_DISP0_SMI_COMMON          = 0  + 64,       /* SMI_COMMON */
	MT_CG_DISP0_SMI_LARB0           = 1  + 64,       /* SMI_LARB0  */
	MT_CG_DISP0_CAM_MDP             = 2  + 64,       /* CAM_MDP    */
	MT_CG_DISP0_MDP_RDMA            = 3  + 64,       /* MDP_RDMA   */
	MT_CG_DISP0_MDP_RSZ0            = 4  + 64,       /* MDP_RSZ0   */
	MT_CG_DISP0_MDP_RSZ1            = 5  + 64,
	MT_CG_DISP0_MDP_TDSHP           = 6  + 64,
	MT_CG_DISP0_MDP_WDMA            = 7  + 64,
	MT_CG_DISP0_MDP_WROT            = 8  + 64,
	MT_CG_DISP0_FAKE_ENG            = 9  + 64,
	MT_CG_DISP0_DISP_OVL0           = 10 + 64,
	MT_CG_DISP0_DISP_RDMA0          = 11 + 64,
	MT_CG_DISP0_DISP_RDMA1          = 12 + 64,
	MT_CG_DISP0_DISP_WDMA0          = 13 + 64,
	MT_CG_DISP0_DISP_COLOR          = 14 + 64,
	MT_CG_DISP0_DISP_CCORR          = 15 + 64,
	MT_CG_DISP0_DISP_AAL            = 16 + 64,
	MT_CG_DISP0_DISP_GAMMA          = 17 + 64,
	MT_CG_DISP0_DISP_DITHER         = 18 + 64,

	MT_CG_DISP1_DSI_ENGINE          = 2  + 96,
	MT_CG_DISP1_DSI_DIGITAL         = 3  + 96,
	MT_CG_DISP1_DPI_ENGINE          = 4  + 96,
	MT_CG_DISP1_DPI_PIXEL           = 5  + 96,

	MT_CG_IMAGE_LARB2_SMI           = 0  + 128,      /* LARB2_SMI_CKPDN */
	MT_CG_IMAGE_CAM_SMI             = 5  + 128,      /* CAM_SMI_CKPDN   */
	MT_CG_IMAGE_CAM_CAM             = 6  + 128,      /* CAM_CAM_CKPDN   */
	MT_CG_IMAGE_SEN_TG              = 7  + 128,      /* SEN_TG_CKPDN    */
	MT_CG_IMAGE_SEN_CAM             = 8  + 128,      /* SEN_CAM_CKPDN   */
	MT_CG_IMAGE_CAM_SV              = 9  + 128,      /* CAM_SV_CKPDN    */
	MT_CG_IMAGE_SUFOD               = 10 + 128,      /* SUFOD_CKPDN     */
	MT_CG_IMAGE_FD                  = 11 + 128,      /* FD_CKPDN        */

	MT_CG_MFG_BG3D                  = 0  + 160,      /* BG3D_PDN        */

	MT_CG_AUDIO_AFE                 = 2  + 192,      /* PDN_AFE         */
	MT_CG_AUDIO_I2S                 = 6  + 192,      /* PDN_I2S         */
	MT_CG_AUDIO_22M                 = 8  + 192,      /* PDN_22M         */
	MT_CG_AUDIO_24M                 = 9  + 192,      /* PDN_24M         */
	MT_CG_AUDIO_APLL2_TUNER         = 18 + 192,      /* PDN_APLL2_TUNER */
	MT_CG_AUDIO_APLL_TUNER          = 19 + 192,      /* PDN_APLL_TUNER  */
	MT_CG_AUDIO_ADC                 = 24 + 192,
	MT_CG_AUDIO_DAC                 = 25 + 192,
	MT_CG_AUDIO_DAC_PREDIS          = 26 + 192,
	MT_CG_AUDIO_TML                 = 27 + 192,

	MT_CG_VDEC0_VDEC				= 0  + 224,      /* VDEC_CKEN       */

	MT_CG_VDEC1_LARB				= 0  + 256,      /* LARB_CKEN       */

	MT_CG_VENC_LARB                 = 0  + 288,      /* LARB_CKE        */
	MT_CG_VENC_VENC                 = 4  + 288,      /* VENC_CKE        */
	MT_CG_VENC_JPGENC               = 8  + 288,      /* JPGENC_CKE      */
	MT_CG_VENC_JPGDEC               = 12 + 288,      /* JPGDEC_CKE      */

	CG_INFRA_FROM                   = MT_CG_INFRA_DBGCLK,
	CG_INFRA_TO                     = MT_CG_INFRA_PMIC_WRAP,
	NR_INFRA_CLKS                   = 23,

	CG_PERI_FROM                    = MT_CG_PERI_DISP_PWM,
	CG_PERI_TO                      = MT_CG_PERI_IRTX,
	NR_PERI_CLKS                    = 29,

	CG_DISP0_FROM                   = MT_CG_DISP0_SMI_COMMON,
	CG_DISP0_TO                     = MT_CG_DISP0_DISP_DITHER,
	NR_DISP0_CLKS                   = 19,

	CG_DISP1_FROM                   = MT_CG_DISP1_DSI_ENGINE,
	CG_DISP1_TO                     = MT_CG_DISP1_DPI_PIXEL,
	NR_DISP1_CLKS                   = 5,

	CG_IMAGE_FROM                   = MT_CG_IMAGE_LARB2_SMI,
	CG_IMAGE_TO                     = MT_CG_IMAGE_FD,
	NR_IMAGE_CLKS                   = 11,

	CG_MFG_FROM                     = MT_CG_MFG_BG3D,
	CG_MFG_TO                       = MT_CG_MFG_BG3D,
	NR_MFG_CLKS                     = 1,

	CG_AUDIO_FROM                   = MT_CG_AUDIO_AFE,
	CG_AUDIO_TO                     = MT_CG_AUDIO_TML,
	NR_AUDIO_CLKS                   = 27,

	CG_VDEC0_FROM                   = MT_CG_VDEC0_VDEC,
	CG_VDEC0_TO                     = MT_CG_VDEC0_VDEC,
	NR_VDEC0_CLKS                   = 1,

	CG_VDEC1_FROM                   = MT_CG_VDEC1_LARB,
	CG_VDEC1_TO                     = MT_CG_VDEC1_LARB,
	NR_VDEC1_CLKS                   = 1,

	CG_VENC_FROM                    = MT_CG_VENC_LARB,
	CG_VENC_TO                      = MT_CG_VENC_JPGDEC,
	NR_VENC_CLKS                    = 12,

	NR_CLKS                         = 301,
};
#endif /* _MT_CLKMGR1_LEGACY_H, _MT_CLKMGR2_H, _MT_CLKMGR3_H */
#endif

#endif
