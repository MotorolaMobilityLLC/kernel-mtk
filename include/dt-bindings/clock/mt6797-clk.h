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

#ifndef _DT_BINDINGS_CLK_MT6797_H
#define _DT_BINDINGS_CLK_MT6797_H

/* TOPCKGEN */
#define	TOP_MUX_ULPOSC_AXI_CK_MUX_PRE	1
#define	TOP_MUX_ULPOSC_AXI_CK_MUX	2
#define	TOP_MUX_AXI	3
#define	TOP_MUX_MEM	4
#define	TOP_MUX_DDRPHYCFG	5
#define	TOP_MUX_MM	6
#define	TOP_MUX_PWM	7
#define	TOP_MUX_VDEC	8
#define	TOP_MUX_VENC	9
#define	TOP_MUX_MFG	10
#define	TOP_MUX_CAMTG	11
#define	TOP_MUX_UART	12
#define	TOP_MUX_SPI	13
#define	TOP_MUX_ULPOSC_SPI_CK_MUX	14
#define	TOP_MUX_USB20	15
#define	TOP_MUX_MSDC50_0_HCLK	16
#define	TOP_MUX_MSDC50_0	17
#define	TOP_MUX_MSDC30_1	18
#define	TOP_MUX_MSDC30_2	19
#define	TOP_MUX_AUDIO	20
#define	TOP_MUX_AUD_INTBUS	21
#define	TOP_MUX_PMICSPI	22
#define	TOP_MUX_SCP	23
#define	TOP_MUX_ATB	24
#define	TOP_MUX_MJC	25
#define	TOP_MUX_DPI0	26
#define	TOP_MUX_AUD_1	27
#define	TOP_MUX_AUD_2	28
#define	TOP_MUX_SSUSB_TOP_SYS	29
#define	TOP_MUX_SPM	30
#define	TOP_MUX_BSI_SPI	31
#define	TOP_MUX_AUDIO_H	32
#define	TOP_MUX_ANC_MD32	33
#define	TOP_MUX_MFG_52M	34
#define	TOP_SYSPLL_CK	35
#define	TOP_SYSPLL_D2	36
#define	TOP_SYSPLL1_D2	37
#define	TOP_SYSPLL1_D4	38
#define	TOP_SYSPLL1_D8	39
#define	TOP_SYSPLL1_D16	40
#define	TOP_SYSPLL_D3	41
#define	TOP_SYSPLL_D3_D3	42
#define	TOP_SYSPLL2_D2	43
#define	TOP_SYSPLL2_D4	44
#define	TOP_SYSPLL2_D8	45
#define	TOP_SYSPLL_D5	46
#define	TOP_SYSPLL3_D2	47
#define	TOP_SYSPLL3_D4	48
#define	TOP_SYSPLL_D7	49
#define	TOP_SYSPLL4_D2	50
#define	TOP_SYSPLL4_D4	51
#define	TOP_UNIVPLL_CK	52
#define	TOP_UNIVPLL_D7	53
#define	TOP_UNIVPLL_D26	54
#define	TOP_SSUSB_PHY_48M_CK	55
#define	TOP_USB_PHY48M_CK	56
#define	TOP_UNIVPLL_D2	57
#define	TOP_UNIVPLL1_D2	58
#define	TOP_UNIVPLL1_D4	59
#define	TOP_UNIVPLL1_D8	60
#define	TOP_UNIVPLL_D3	61
#define	TOP_UNIVPLL2_D2	62
#define	TOP_UNIVPLL2_D4	63
#define	TOP_UNIVPLL2_D8	64
#define	TOP_UNIVPLL_D5	65
#define	TOP_UNIVPLL3_D2	66
#define	TOP_UNIVPLL3_D4	67
#define	TOP_UNIVPLL3_D8	68
#define	TOP_ULPOSC_CK_ORG	69
#define	TOP_ULPOSC_CK	70
#define	TOP_ULPOSC_D2	71
#define	TOP_ULPOSC_D3	72
#define	TOP_ULPOSC_D4	73
#define	TOP_ULPOSC_D8	74
#define	TOP_ULPOSC_D10	75
#define	TOP_APLL1_CK	76
#define	TOP_APLL2_CK	77
#define	TOP_MFGPLL_CK	78
#define	TOP_MFGPLL_D2	79
#define	TOP_IMGPLL_CK	80
#define	TOP_IMGPLL_D2	81
#define	TOP_IMGPLL_D4	82
#define	TOP_CODECPLL_CK	83
#define	TOP_CODECPLL_D2	84
#define	TOP_VDECPLL_CK	85
#define	TOP_TVDPLL_CK	86
#define	TOP_TVDPLL_D2	87
#define	TOP_TVDPLL_D4	88
#define	TOP_TVDPLL_D8	89
#define	TOP_TVDPLL_D16	90
#define	TOP_MSDCPLL_CK	91
#define	TOP_MSDCPLL_D2	92
#define	TOP_MSDCPLL_D4	93
#define	TOP_MSDCPLL_D8	94
#define TOP_NR_CLK		95


/* APMIXED_SYS */
/* #define APMIXED_ARMPLL 1 */
#define APMIXED_MAINPLL	1
#define APMIXED_UNIVPLL 2
#define APMIXED_MFGPLL	3
#define APMIXED_MSDCPLL	4
#define APMIXED_IMGPLL	5
#define APMIXED_TVDPLL	6
#define APMIXED_MPLL	7
#define APMIXED_CODECPLL	8
#define APMIXED_VDECPLL	9
#define APMIXED_APLL1	10
#define APMIXED_APLL2	11
#define APMIXED_NR_CLK	12

/* INFRA_SYS, infrasys */
#define	INFRA_PMIC_TMR	1
#define	INFRA_PMIC_AP	2
#define	INFRA_PMIC_MD	3
#define	INFRA_PMIC_CONN	4
#define	INFRA_SCP	5
#define	INFRA_SEJ	6
#define	INFRA_APXGPT	7
#define	INFRA_SEJ_13M	8
#define	INFRA_ICUSB	9
#define	INFRA_GCE	10
#define	INFRA_THERM	11
#define	INFRA_I2C0	12
#define	INFRA_I2C1	13
#define	INFRA_I2C2	14
#define	INFRA_I2C3	15
#define	INFRA_PWM_HCLK	16
#define	INFRA_PWM1	17
#define	INFRA_PWM2	18
#define	INFRA_PWM3	19
#define	INFRA_PWM4	20
#define	INFRA_PWM	21
#define	INFRA_UART0	22
#define	INFRA_UART1	23
#define	INFRA_UART2	24
#define	INFRA_UART3	25
#define	INFRA_MD2MD_CCIF_0	26
#define	INFRA_MD2MD_CCIF_1	27
#define	INFRA_MD2MD_CCIF_2	28
#define	INFRA_FHCTL	29
#define	INFRA_BTIF	30
#define	INFRA_MD2MD_CCIF_3	31
#define	INFRA_SPI	32
#define	INFRA_MSDC0	33
#define	INFRA_MD2MD_CCIF_4	34
#define	INFRA_MSDC1	35
#define	INFRA_MSDC2	36
#define	INFRA_MD2MD_CCIF_5	37
#define	INFRA_GCPU	38
#define	INFRA_TRNG	39
#define	INFRA_AUXADC	40
#define	INFRA_CPUM	41
#define	INFRA_AP_C2K_CCIF_0	42
#define	INFRA_AP_C2K_CCIF_1	43
#define	INFRA_CLDMA	44
#define	INFRA_DISP_PWM	45
#define	INFRA_AP_DMA	46
#define	INFRA_DEVICE_APC	47
#define	INFRA_L2C_SRAM	48
#define	INFRA_CCIF_AP	49
/*#define	INFRA_DEBUGSYS	50*/
#define	INFRA_AUDIO	50
#define	INFRA_CCIF_MD	51
#define	INFRA_DRAMC_F26M	52
#define	INFRA_I2C4	53
#define	INFRA_I2C_APPM	54
#define	INFRA_I2C_GPUPM	55
#define	INFRA_I2C2_IMM	56
#define	INFRA_I2C2_ARB	57
#define	INFRA_I2C3_IMM	58
#define	INFRA_I2C3_ARB	59
#define	INFRA_I2C5	60
#define	INFRA_SYS_CIRQ	61
#define	INFRA_SPI1	62
#define	INFRA_DRAMC_B_F26M	63
#define	INFRA_ANC_MD32	64
#define	INFRA_ANC_MD32_32K	65
#define	INFRA_DVFS_SPM1	66
#define	INFRA_AES_TOP0	67
#define	INFRA_AES_TOP1	68
#define	INFRA_SSUSB_BUS	69
#define	INFRA_SPI2	70
#define	INFRA_SPI3	71
#define	INFRA_SPI4	72
#define	INFRA_SPI5	73
#define	INFRA_IRTX	74
#define	INFRA_SSUSB_SYS	75
#define	INFRA_SSUSB_REF	76
#define	INFRA_AUDIO_26M	77
#define	INFRA_AUDIO_26M_PAD_TOP	78
#define	INFRA_MODEM_TEMP_SHARE	79
#define	INFRA_VAD_WRAP_SOC	80
#define	INFRA_DRAMC_CONF	81
#define	INFRA_DRAMC_B_CONF	82
#define INFRA_MFG_VCG 83
#define INFRA_NR_CLK	84

/* MFG_SYS, mfgsys */
#define MFG_BG3D	1
#define MFG_NR_CLK	2

/* IMG_SYS, imgsys */
#define	IMG_FDVT	1
#define	IMG_DPE	2
#define	IMG_DIP	3
#define	IMG_LARB6	4
#define IMG_NR_CLK	5

/* CAM_SYS, camsys*/
#define	CAM_CAMSV2	1
#define	CAM_CAMSV1	2
#define	CAM_CAMSV0	3
#define	CAM_SENINF	4
#define	CAM_CAMTG	5
#define	CAM_CAMSYS	6
#define	CAM_LARB2	7
#define CAM_NR_CLK	8

/* MM_SYS, mmsys */
#define	MM_SMI_COMMON	1
#define	MM_SMI_LARB0	2
#define	MM_SMI_LARB5	3
#define	MM_CAM_MDP	4
#define	MM_MDP_RDMA0	5
#define	MM_MDP_RDMA1	6
#define	MM_MDP_RSZ0	7
#define	MM_MDP_RSZ1	8
#define	MM_MDP_RSZ2	9
#define	MM_MDP_TDSHP	10
#define	MM_MDP_COLOR	11
#define	MM_MDP_WDMA	12
#define	MM_MDP_WROT0	13
#define	MM_MDP_WROT1	14
#define	MM_FAKE_ENG	15
#define	MM_DISP_OVL0	16
#define	MM_DISP_OVL1	17
#define	MM_DISP_OVL0_2L	18
#define	MM_DISP_OVL1_2L	19
#define	MM_DISP_RDMA0	20
#define	MM_DISP_RDMA1	21
#define	MM_DISP_WDMA0	22
#define	MM_DISP_WDMA1	23
#define	MM_DISP_COLOR	24
#define	MM_DISP_CCORR	25
#define	MM_DISP_AAL	26
#define	MM_DISP_GAMMA	27
#define	MM_DISP_OD	28
#define	MM_DISP_DITHER	29
#define	MM_DISP_UFOE	30
#define	MM_DISP_DSC	31
#define	MM_DISP_SPLIT	32
#define	MM_DSI0_MM_CLOCK	33
#define	MM_DSI1_MM_CLOCK	34
#define	MM_DPI_MM_CLOCK	35
#define	MM_DPI_INTERFACE_CLOCK	36
#define	MM_LARB4_AXI_ASIF_MM_CLOCK	37
#define	MM_LARB4_AXI_ASIF_MJC_CLOCK	38
#define	MM_DISP_OVL0_MOUT_CLOCK	39
#define	MM_FAKE_ENG2	40
#define	MM_DSI0_INTERFACE_CLOCK	41
#define	MM_DSI1_INTERFACE_CLOCK	42
#define MM_NR_CLK		43

/* MJC_SYS, mjcsys */
#define	MJC_SMI_LARB	1
#define	MJC_TOP_CLK_0	2
#define	MJC_TOP_CLK_1	3
#define	MJC_TOP_CLK_2	4
#define	MJC_FAKE_ENGINE	5
#define	MJC_LARB4_ASIF	6
#define MJC_NR_CLK		7


/* VDEC_SYS, vdecsys */
#define	VDEC_CKEN_ENG	1
#define	VDEC_ACTIVE	2
#define	VDEC_CKEN	3
#define	VDEC_LARB1_CKEN	4
#define VDEC_NR_CLK		5

/* VENC_SYS, vencsys */
#define	VENC_0	1
#define	VENC_1	2
#define	VENC_2	3
#define	VENC_3	4
#define VENC_NR_CLK	5

/* AUDIO_SYS, audiosys */
#define	AUDIO_PDN_TML	1
#define	AUDIO_PDN_DAC_PREDIS	2
#define	AUDIO_PDN_DAC	3
#define	AUDIO_PDN_ADC	4
#define	AUDIO_PDN_APLL_TUNER	5
#define	AUDIO_PDN_APLL2_TUNER	6
#define	AUDIO_PDN_24M	7
#define	AUDIO_PDN_22M	8
#define	AUDIO_PDN_AFE	9
#define	AUDIO_PDN_ADC_HIRES_TML	10
#define	AUDIO_PDN_ADC_HIRES	11
#define AUDIO_NR_CLK 12

/* SCP_SYS */
#define SCP_SYS_MD1  1
#define SCP_SYS_CONN  2
#define SCP_SYS_DIS  3
#define SCP_SYS_ISP  4
#define SCP_SYS_VDE  5
#define SCP_SYS_MFG_ASYNC  6
#define SCP_SYS_MFG  7
#define SCP_SYS_MFG_CORE3  8
#define SCP_SYS_MFG_CORE2  9
#define SCP_SYS_MFG_CORE1  10
#define SCP_SYS_MFG_CORE0  11
#define SCP_SYS_MJC  12
#define SCP_SYS_VEN  13
#define SCP_SYS_AUDIO  14
#define SCP_SYS_C2K  15
#define SCP_NR_SYSS  16

#endif				/* _DT_BINDINGS_CLK_MT6797_H */
