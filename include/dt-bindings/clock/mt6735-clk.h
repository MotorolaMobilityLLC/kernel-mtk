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

#ifndef _DT_BINDINGS_CLK_MT6735_H
#define _DT_BINDINGS_CLK_MT6735_H

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
#define TOP_MUX_USB20	11
#define TOP_MUX_MSDC50_0	12
#define TOP_MUX_MSDC30_0	13
#define TOP_MUX_MSDC30_1	14
#define TOP_MUX_MSDC30_2	15
#define TOP_MUX_MSDC30_3	16
#define TOP_MUX_AUDIO	17
#define TOP_MUX_AUDINTBUS	18
#define TOP_MUX_PMICSPI	19
#define TOP_MUX_SCP	20
#define TOP_MUX_ATB	21
#define TOP_MUX_DPI0	22
#define TOP_MUX_SCAM	23
#define TOP_MUX_MFG13M	24
#define TOP_MUX_AUD1	25
#define TOP_MUX_AUD2	26
#define TOP_MUX_IRDA	27
#define TOP_MUX_IRTX	28
#define TOP_MUX_DISPPWM	29
#define TOP_UNIVPLL_D2		30
#define TOP_UNIVPLL_D3		31
#define TOP_UNIVPLL1_D2		32
#define TOP_AD_APLL1_CK		33
#define TOP_AD_SYS_26M_CK		34
#define TOP_AD_SYS_26M_D2		35
#define TOP_DMPLL_CK		36
#define TOP_DMPLL_D2		37
#define TOP_DMPLL_D4		38
#define TOP_DMPLL_D8		39
#define TOP_MMPLL_CK		40
#define TOP_MSDCPLL_CK		41
#define TOP_MSDCPLL_D16		42
#define TOP_MSDCPLL_D2		43
#define TOP_MSDCPLL_D4		44
#define TOP_MSDCPLL_D8		45
#define TOP_SYSPLL_D2		46
#define TOP_SYSPLL_D3		47
#define TOP_SYSPLL_D5		48
#define TOP_SYSPLL1_D16		49
#define TOP_SYSPLL1_D2		50
#define TOP_SYSPLL1_D4		51
#define TOP_SYSPLL1_D8		52
#define TOP_SYSPLL2_D2		53
#define TOP_SYSPLL2_D4		54
#define TOP_SYSPLL3_D2		55
#define TOP_SYSPLL3_D4		56
#define TOP_SYSPLL4_D2		57
#define TOP_SYSPLL4_D2_D8		58
#define TOP_SYSPLL4_D4		59
#define TOP_TVDPLL_CK		60
#define TOP_TVDPLL_D2		61
#define TOP_TVDPLL_D4		62
#define TOP_UNIVPLL_D26		63
#define TOP_UNIVPLL_D5		65
#define TOP_UNIVPLL1_D4		66
#define TOP_UNIVPLL1_D8		67
#define TOP_UNIVPLL2_D2		68
#define TOP_UNIVPLL2_D4		69
#define TOP_UNIVPLL2_D8		70
#define TOP_UNIVPLL3_D2		71
#define TOP_UNIVPLL3_D4		72
#define TOP_VENCPLL_CK		73
#define TOP_VENCPLL_D3		74
#define TOP_WHPLL_AUDIO_CK		75
#define TOP_CLKPH_MCK_O		76
#define TOP_DPI_CK		77
#define TOP_NR_CLK		78

/* APMIXED_SYS */
#define APMIXED_ARMPLL	1
#define APMIXED_MAINPLL	2
#define APMIXED_MSDCPLL	3
#define APMIXED_UNIVPLL	4
#define APMIXED_MMPLL		5
#define APMIXED_VENCPLL	6
#define APMIXED_TVDPLL	7
#define APMIXED_APLL1	8
#define APMIXED_APLL2	9
#define APMIXED_NR_CLK		10

/* INFRA_SYS, infrasys */
#define INFRA_DBGCLK	1
#define INFRA_GCE	2
#define INFRA_TRBG	3
#define INFRA_CPUM	4
#define INFRA_DEVAPC	5
#define INFRA_AUDIO	6
#define INFRA_GCPU	7
#define INFRA_L2C_SRAM	8
#define INFRA_M4U	9
#define INFRA_CLDMA	10
#define INFRA_CONNMCU_BUS	11
#define INFRA_KP	12
#define INFRA_APXGPT	13
#define INFRA_SEJ	14
#define INFRA_CCIF0_AP	15
#define INFRA_CCIF1_AP	16
#define INFRA_PMIC_SPI	17
#define INFRA_PMIC_WRAP	18
#define INFRA_NR_CLK	19

/* PERI_SYS, perisys */
#define PERI_DISP_PWM	1
#define PERI_THERM	2
#define PERI_PWM1	3
#define PERI_PWM2	4
#define PERI_PWM3	5
#define PERI_PWM4	6
#define PERI_PWM5	7
#define PERI_PWM6	8
#define PERI_PWM7	9
#define PERI_PWM	10
#define PERI_USB0	11
#define PERI_IRDA	12
#define PERI_APDMA	13
#define PERI_MSDC30_0	14
#define PERI_MSDC30_1	15
#define PERI_MSDC30_2	16
#define PERI_MSDC30_3	17
#define PERI_UART0	18
#define PERI_UART1	19
#define PERI_UART2	20
#define PERI_UART3	21
#define PERI_UART4	22
#define PERI_BTIF	23
#define PERI_I2C0	24
#define PERI_I2C1	25
#define PERI_I2C2	26
#define PERI_I2C3	27
#define PERI_AUXADC	28
#define PERI_SPI0	29
#define PERI_IRTX	30
#define PERI_NR_CLK	31

/* MFG_SYS, mfgsys */
#define MFG_BG3D	1
#define MFG_NR_CLK	2

/* IMG_SYS, imgsys */
#define IMG_IMAGE_LARB2_SMI	1
#define IMG_IMAGE_CAM_SMI	2
#define IMG_IMAGE_CAM_CAM	3
#define IMG_IMAGE_SEN_TG	4
#define IMG_IMAGE_SEN_CAM	5
#define IMG_IMAGE_CAM_SV	6
#define IMG_IMAGE_SUFOD	7
#define IMG_IMAGE_FD	8
#define IMG_NR_CLK	9

/* MM_SYS, mmsys */
#define MM_DISP0_SMI_COMMON	1
#define MM_DISP0_SMI_LARB0	2
#define MM_DISP0_CAM_MDP	3
#define MM_DISP0_MDP_RDMA	4
#define MM_DISP0_MDP_RSZ0	5
#define MM_DISP0_MDP_RSZ1	6
#define MM_DISP0_MDP_TDSHP	7
#define MM_DISP0_MDP_WDMA	8
#define MM_DISP0_MDP_WROT	9
#define MM_DISP0_FAKE_ENG	10
#define MM_DISP0_DISP_OVL0	11
#define MM_DISP0_DISP_RDMA0	12
#define MM_DISP0_DISP_RDMA1	13
#define MM_DISP0_DISP_WDMA0	14
#define MM_DISP0_DISP_COLOR	15
#define MM_DISP0_DISP_CCORR	16
#define MM_DISP0_DISP_AAL	17
#define MM_DISP0_DISP_GAMMA	18
#define MM_DISP0_DISP_DITHER	19
#define MM_DISP1_DSI_ENGINE	20
#define MM_DISP1_DSI_DIGITAL	21
#define MM_DISP1_DPI_ENGINE	22
#define MM_DISP1_DPI_PIXEL	23
#define MM_NR_CLK		24

/* VDEC_SYS, vdecsys */
#define VDEC0_VDEC		1
#define VDEC1_LARB		2
#define VDEC_NR_CLK		3

/* VENC_SYS, vencsys */
#define VENC_LARB	1
#define VENC_VENC	2
#define VENC_JPGENC	3
#define VENC_JPGDEC	4
#define VENC_NR_CLK	5

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
#define AUDIO_NR_CLK	11

/* SCP_SYS */
#define SCP_SYS_MD1	1
#define SCP_SYS_MD2	2
#define SCP_SYS_CONN	3
#define SCP_SYS_DIS	4
#define SCP_SYS_MFG	5
#define SCP_SYS_ISP	6
#define SCP_SYS_VDE	7
#define SCP_SYS_VEN	8
#define SCP_NR_SYSS	9
#endif /* _DT_BINDINGS_CLK_MT6735_H */
