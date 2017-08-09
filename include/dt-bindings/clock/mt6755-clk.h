/*
* Copyright (c) 2014 MediaTek Inc.
* Author: Roy Chen <roy-cc.chen@mediatek.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef _DT_BINDINGS_CLK_MT6755_H
#define _DT_BINDINGS_CLK_MT6755_H

/* TOPCKGEN */
#define TOP_MUX_AXI	1
#define TOP_MUX_MEM	2
#define TOP_MUX_DDRPHY	3
#define TOP_MUX_MM	4
#define TOP_MUX_PWM	5
#define TOP_MUX_VDEC	6
#define TOP_MUX_MFG	7
#define TOP_MUX_CAMTG	8
#define TOP_MUX_UART	9
#define TOP_MUX_SPI	10
#define TOP_MUX_MSDC50_0_HCLK	11
#define TOP_MUX_MSDC50_0	12
/*#define TOP_MUX_MSDC30_0	13*/
#define TOP_MUX_MSDC30_1	14
#define TOP_MUX_MSDC30_2	15
#define TOP_MUX_MSDC30_3	16
#define TOP_MUX_AUDIO	17
#define TOP_MUX_AUDINTBUS	18
#define TOP_MUX_PMICSPI	19
#define TOP_MUX_ATB	20
#define TOP_MUX_DPI0	21
#define TOP_MUX_SCAM	22
/*#define TOP_MUX_MFG13M	23*/
#define TOP_MUX_AUD1	24
#define TOP_MUX_AUD2	25
#define TOP_MUX_DISPPWM	26
#define TOP_MUX_SSUSBTOPSYS	27
#define TOP_MUX_USBTOP	28
#define TOP_MUX_SPM	29
#define TOP_MUX_BSISPI	30
#define TOP_MUX_I2C	31
#define TOP_MUX_DVFSP	32
#define TOP_AD_APLL1_CK		33
#define TOP_AD_APLL2_CK		34
#define TOP_MMPLL_CK		35
#define TOP_DDRX1_CK		36
#define TOP_DMPLL_CK		37
#define TOP_MPLL_208M_CK 38
#define TOP_MSDCPLL_CK		39
#define TOP_MSDCPLL_D16		40
#define TOP_MSDCPLL_D2		41
#define TOP_MSDCPLL_D4		42
#define TOP_MSDCPLL_D8		43
#define TOP_OSC_D2        44
#define TOP_OSC_D4        45
#define TOP_OSC_D8       46
#define TOP_SYSPLL_D3		  47
#define TOP_SYSPLL_D3_D3	48
#define TOP_SYSPLL_D5		49
#define TOP_SYSPLL_D7		50
#define TOP_SYSPLL1_D2		51
#define TOP_SYSPLL1_D4		52
#define TOP_SYSPLL1_D8		53
#define TOP_SYSPLL1_D16		54
#define TOP_SYSPLL2_D2		55
#define TOP_SYSPLL2_D4		56
#define TOP_SYSPLL3_D2		57
#define TOP_SYSPLL3_D4		58
#define TOP_SYSPLL4_D2		59
#define TOP_SYSPLL4_D4		60
#define TOP_TVDPLL_D2		61
#define TOP_TVDPLL_D4		62
#define TOP_TVDPLL_D8		63
#define TOP_TVDPLL_D16		64
#define TOP_UNIVPLL_D2		65
#define TOP_UNIVPLL_D3		66
#define TOP_UNIVPLL_D5		67
#define TOP_UNIVPLL_D7		68
#define TOP_UNIVPLL_D26		69
#define TOP_UNIVPLL1_D2		70
#define TOP_UNIVPLL1_D4		71
#define TOP_UNIVPLL1_D8		72
#define TOP_UNIVPLL2_D2		73
#define TOP_UNIVPLL2_D4		74
#define TOP_UNIVPLL2_D8		75
#define TOP_UNIVPLL3_D2		76
#define TOP_UNIVPLL3_D4		77
#define TOP_VENCPLL_CK		78
#define TOP_WHPLL_AUDIO_CK		79
#define TOP_CLKPH_MCK_O		80
#define TOP_DPI_CK		81
#define TOP_MUX_USB20	82
#define TOP_MUX_SCP	  83
#define TOP_MUX_IRDA	84
#define TOP_MUX_IRTX	85
#define TOP_AD_SYS_26M_CK		86
#define TOP_AD_SYS_26M_D2		87
#define TOP_DMPLL_D2		88
#define TOP_DMPLL_D4		89
#define TOP_DMPLL_D8		90
#define TOP_SYSPLL_D2		91
#define TOP_SYSPLL4_D2_D8		92
#define TOP_TVDPLL_CK		93
#define TOP_VENCPLL_D3		94

#define TOP_SYSPLL_CK		95
#define TOP_SYSPLL1_CK		96
#define TOP_SYSPLL2_CK		97
#define TOP_SYSPLL3_CK		98
#define TOP_SYSPLL4_CK		99
#define TOP_UNIVPLL_CK		100
#define TOP_UNIVPLL1_CK		101
#define TOP_UNIVPLL2_CK		102
#define TOP_UNIVPLL3_CK		103
#define TOP_OSC_CK        104
#define TOP_NR_CLK		105

/* APMIXED_SYS */
#define APMIXED_ARMBPLL	1
#define APMIXED_ARMSPLL	2
#define APMIXED_MAINPLL	3
#define APMIXED_MSDCPLL	4
#define APMIXED_UNIVPLL	5
#define APMIXED_MMPLL		6
#define APMIXED_VENCPLL	7
#define APMIXED_TVDPLL	8
#define APMIXED_APLL1	9
#define APMIXED_APLL2	10
#define APMIXED_ARMPLL	11
#define SCP_OSCPLL	12
#define APMIXED_NR_CLK		13

/* INFRA_SYS0, infrasys0 */
#define	INFRA_PMIC_TMR	1
#define	INFRA_PMIC_AP	2
#define	INFRA_PMIC_MD	3
#define	INFRA_PMIC_CONN	4
/*#define	INFRA_SCPSYS*/
#define	INFRA_SEJ	5
#define	INFRA_APXGPT	6
#define	INFRA_ICUSB	7
#define	INFRA_GCE	8
#define	INFRA_THERM	9
#define	INFRA_I2C0	10
#define	INFRA_I2C1	11
#define	INFRA_I2C2	12
#define	INFRA_I2C3	13
#define	INFRA_PWM_HCLK	14
#define	INFRA_PWM1	15
#define	INFRA_PWM2	16
#define	INFRA_PWM3	17
#define	INFRA_PWM4	18
#define	INFRA_PWM	19
#define	INFRA_UART0	20
#define	INFRA_UART1	21
#define	INFRA_UART2	22
#define	INFRA_UART3	23
#define	INFRA_MD2MD_CCIF0	24
#define	INFRA_MD2MD_CCIF1	25
#define	INFRA_MD2MD_CCIF2	26
#define	INFRA_BTIF	27
/* INFRA_SYS1, infrasys1 */
#define	INFRA_MD2MD_CCIF3	28
#define	INFRA_SPI0	29
#define	INFRA_MSDC0	30
#define	INFRA_MD2MD_CCIF4	31
#define	INFRA_MSDC1	32
#define	INFRA_MSDC2	33
#define	INFRA_MSDC3	34
#define	INFRA_MD2MD_CCIF5	35
#define	INFRA_GCPU	36
#define	INFRA_TRNG	37
#define	INFRA_AUXADC	38
#define	INFRA_CPUM	39
#define	INFRA_CCIF1_AP	40
#define	INFRA_CCIF1_MD	41
/*#define	INFRA_NFI
#define	INFRA_NFI_ECC*/
#define	INFRA_AP_DMA	42
#define	INFRA_XIU	43
#define	INFRA_DEVICE_APC	44
#define	INFRA_XIU2AHB	45
/*#define	INFRA_L2C_SRAM*/
#define	INFRA_CCIF_AP	46
#define	INFRA_DEBUGSYS	47
#define	INFRA_AUDIO	48
#define	INFRA_CCIF_MD	49
#define	INFRA_DRAMC_F26M	50
/* INFRA_SYS2, infrasys2 */
#define	INFRA_IRTX	51
#define	INFRA_SSUSB_TOP	52
#define	INFRA_DISP_PWM	53
#define	INFRA_CLDMA_BCLK	54
#define	INFRA_AUDIO_26M_BCLK	55
#define	INFRA_MD_TEMP_26M_BCLK	56
#define	INFRA_SPI1	57
#define	INFRA_I2C4	58
#define	INFRA_MD_TEMP_SHARE	59
#define INFRA_CLK_13M	60
#define INFRA_NR_CLK	61






#if 0
#define INFRA_DBGCLK	1
#define INFRA_GCE	2
#define INFRA_TRBG	3
#define INFRA_CPUM	4
#define INFRA_DEVAPC	5
#define INFRA_AUDIO	6
#define INFRA_GCPU	7
#define INFRA_L2C_SRAM	8
#define INFRA_CLDMA	9
#define INFRA_APXGPT	10
#define INFRA_SEJ	11
#define INFRA_CCIF0_AP	12
#define INFRA_CCIF1_AP	13
#define INFRA_DRMAC_F26M	14
#define INFRA_M4U	15
#define INFRA_CONNMCU_BUS	16
#define INFRA_KP	17
#define INFRA_PMIC_SPI	18
#define INFRA_PMIC_WRAP	19
#define INFRA_NR_CLK	20
#endif

/* PERI_SYS, perisys */
#define PERI_NR_CLK	1
#if 0
#define PERI_DISP_PWM	1
#define PERI_THERM	2
#define PERI_PWM1	3
#define PERI_PWM2	4
#define PERI_PWM3	5
#define PERI_PWM4	6
#define PERI_PWM	7
#define PERI_PWM_HCLK	8
#define PERI_USB0	9
#define PERI_SSUSB	10
#define PERI_APDMA	11
#define PERI_MSDC30_0	12
#define PERI_MSDC30_1	13
#define PERI_MSDC30_2	14
#define PERI_MSDC30_3	15
#define PERI_UART0	16
#define PERI_UART1	17
#define PERI_UART2	18
#define PERI_UART3	19
#define PERI_BTIF	20
#define PERI_I2C0	21
#define PERI_I2C1	22
#define PERI_I2C2	23
#define PERI_I2C3	24
#define PERI_I2C4	25
#define PERI_AUXADC	26
#define PERI_AUDIO26M	27
#define PERI_SPI0	28
#define PERI_SPI1	29
#define PERI_IRTX	30
#define PERI_PWM5	31
#define PERI_PWM6	32
#define PERI_PWM7	33
#define PERI_IRDA	34
#define PERI_UART4	35
#define PERI_NR_CLK	36
#endif
/* MFG_SYS, mfgsys */
#define MFG_BG3D	1
#define MFG_NR_CLK	2

/* IMG_SYS, imgsys */
#define IMG_IMAGE_LARB2_SMI	1
#define IMG_IMAGE_LARB2_SMI_M4U	2
#define IMG_IMAGE_LARB2_SMI_SMI_COMMON	3
#define IMG_IMAGE_LARB2_SMI_MET_SMI_COMMON	4
#define IMG_IMAGE_LARB2_SMI_ISPSYS	5
#define IMG_IMAGE_CAM_SMI	6
#define IMG_IMAGE_CAM_CAM	7
#define IMG_IMAGE_SEN_TG	8
#define IMG_IMAGE_SEN_CAM	9
#define IMG_IMAGE_CAM_SV	10
#define IMG_IMAGE_SUFOD	11
#define IMG_IMAGE_FD	12
#define IMG_NR_CLK	13

/* MM_SYS, mmsys */
#define MM_DISP0_SMI_COMMON	1
#define MM_DISP0_SMI_COMMON_M4U	2
#define MM_DISP0_SMI_COMMON_MALI	3
#define MM_DISP0_SMI_COMMON_DISPSYS	4
#define MM_DISP0_SMI_COMMON_SMI_COMMON	5
#define MM_DISP0_SMI_COMMON_MET_SMI_COMMON	6
#define MM_DISP0_SMI_COMMON_ISPSYS	7
#define MM_DISP0_SMI_COMMON_FDVT	8
#define MM_DISP0_SMI_COMMON_VDEC_GCON	9
#define MM_DISP0_SMI_COMMON_JPGENC	10
#define MM_DISP0_SMI_COMMON_JPGDEC	11
#define MM_DISP0_SMI_LARB0	12
#define MM_DISP0_SMI_LARB0_M4U	13
#define MM_DISP0_SMI_LARB0_DISPSYS	14
#define MM_DISP0_SMI_LARB0_SMI_COMMON	15
#define MM_DISP0_SMI_LARB0_MET_SMI_COMMON	16
#define MM_DISP0_CAM_MDP	17
#define MM_DISP0_MDP_RDMA	18
#define MM_DISP0_MDP_RSZ0	19
#define MM_DISP0_MDP_RSZ1	20
#define MM_DISP0_MDP_TDSHP	21
#define MM_DISP0_MDP_WDMA	22
#define MM_DISP0_MDP_WROT	23
#define MM_DISP0_FAKE_ENG	24
#define MM_DISP0_DISP_OVL0	25
#define MM_DISP0_DISP_OVL1	26
#define MM_DISP0_DISP_RDMA0	27
#define MM_DISP0_DISP_RDMA1	28
#define MM_DISP0_DISP_WDMA0	29
#define MM_DISP0_DISP_COLOR	30
#define MM_DISP0_DISP_CCORR	31
#define MM_DISP0_DISP_AAL	32
#define MM_DISP0_DISP_GAMMA	33
#define MM_DISP0_DISP_DITHER	34
#define MM_DISP0_MDP_COLOR	35
#define MM_DISP0_DISP_UFOE_MOUT	36
#define MM_DISP0_DISP_WDMA1	37
#define MM_DISP0_DISP_2L_OVL0	38
#define MM_DISP0_DISP_2L_OVL1	39
#define MM_DISP0_DISP_OVL0_MOUT	40
#define MM_DISP1_DSI_ENGINE	41
#define MM_DISP1_DSI_DIGITAL	42
#define MM_DISP1_DPI_ENGINE	43
#define MM_DISP1_DPI_PIXEL	44
#define MM_NR_CLK		45

/* VDEC_SYS, vdecsys */
#define VDEC0_VDEC		1
#define VDEC1_LARB		2
#define VDEC1_LARB_M4U		3
#define VDEC1_LARB_SMI_COMMON		4
#define VDEC1_LARB_MET_SMI_COMMON		5
#define VDEC1_LARB_VDEC_GCON		6
#define VDEC_NR_CLK		7

/* VENC_SYS, vencsys */
#define VENC_LARB	1
#define VENC_LARB_M4U	2
#define VENC_LARB_SMI_COMMON	3
#define VENC_LARB_MET_SMI_COMMON	4
#define VENC_LARB_JPGENC	5
#define VENC_LARB_JPGDEC	6
#define VENC_VENC	7
#define VENC_JPGENC	8
#define VENC_JPGDEC	9
#define VENC_NR_CLK	10

/* AUDIO_SYS, audiosys */
#define AUDIO_AFE	1
#define AUDIO_I2S	2
#define AUDIO_22M	3
#define AUDIO_24M	4
#define AUDIO_APLL2_TUNER	5
#define AUDIO_APLL_TUNER	6
#define AUDIO_ADC	7
#define AUDIO_DAC	8
#define AUDIO_DAC_PREDIS	9
#define AUDIO_TML	10
#define AUDIO_APLL1_DIV0	11
#define AUDIO_APLL2_DIV0	12
#define AUDIO_NR_CLK	13

/* SCP_SYS */
#define SCP_SYS_MD1	1
#define SCP_SYS_MD2	2
#define SCP_SYS_CONN	3
#define SCP_SYS_DIS	4
#define SCP_SYS_MFG	5
#define SCP_SYS_ISP	6
#define SCP_SYS_VDE	7
#define SCP_SYS_VEN	8
#define SCP_SYS_AUD	9
#define SCP_NR_SYSS	10
#endif				/* _DT_BINDINGS_CLK_MT6755_H */
