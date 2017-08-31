/*
 * Copyright (C) 2016 MediaTek Inc.
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


#ifndef _ACCDET_REG_H_
#define _ACCDET_REG_H_

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
typedef unsigned int	UINT32;

/* Register Address Define */
#define MT6337_REG_BASE					(0x8000)/* MT6337 register base address offset */

/* clock register */
#define TOP_CKPDN						((UINT32)(MT6337_REG_BASE+0x0206))
#define TOP_CKPDN_SET					((UINT32)(MT6337_REG_BASE+0x0208))
#define TOP_CKPDN_CLR					((UINT32)(MT6337_REG_BASE+0x020A))

#define TOP_RST_ACCDET					((UINT32)(MT6337_REG_BASE+0x0400))
#define TOP_RST_ACCDET_SET				((UINT32)(MT6337_REG_BASE+0x0402))
#define TOP_RST_ACCDET_CLR				((UINT32)(MT6337_REG_BASE+0x0404))

#define INT_CON_ACCDET					((UINT32)(MT6337_REG_BASE+0x0600))
#define INT_CON_ACCDET_SET				((UINT32)(MT6337_REG_BASE+0x0602))
#define INT_CON_ACCDET_CLR				((UINT32)(MT6337_REG_BASE+0x0604))

#define INT_CON1_ACCDET					((UINT32)(MT6337_REG_BASE+0x0606))
#define INT_CON1_ACCDET_SET				((UINT32)(MT6337_REG_BASE+0x0608))
#define INT_CON1_ACCDET_CLR				((UINT32)(MT6337_REG_BASE+0x060A))

#define INT_STATUS_ACCDET				((UINT32)(MT6337_REG_BASE+0x0618))

#define AUXADC_ADC5_CHN_REG				((UINT32)(MT6337_REG_BASE+0x140A))
#define AUXADC_CHN_RQST0				((UINT32)(MT6337_REG_BASE+0x1428))
#define AUXADC_RQST0_CH_SET				((UINT32)(MT6337_REG_BASE+0x142A))
#define AUXADC_RQST0_CH_CLR				((UINT32)(MT6337_REG_BASE+0x142C))
#define AUXADC_AVG_NUM_SEL				((UINT32)(MT6337_REG_BASE+0x143C))/* set average num */


#define AUXADC_AUTO_SPL					((UINT32)(MT6337_REG_BASE+0x145A))

/* accdet register */
#define ACCDET_RSV						((UINT32)(MT6337_REG_BASE+0x1600))
#define ACCDET_CTRL						((UINT32)(MT6337_REG_BASE+0x1602))
#define ACCDET_STATE_SWCTRL				((UINT32)(MT6337_REG_BASE+0x1604))
#define ACCDET_PWM_WIDTH				((UINT32)(MT6337_REG_BASE+0x1606))
#define ACCDET_PWM_THRESH				((UINT32)(MT6337_REG_BASE+0x1608))
#define ACCDET_EN_DELAY_NUM				((UINT32)(MT6337_REG_BASE+0x160A))
#define ACCDET_DEBOUNCE0				((UINT32)(MT6337_REG_BASE+0x160C))
#define ACCDET_DEBOUNCE1				((UINT32)(MT6337_REG_BASE+0x160E))
#define ACCDET_DEBOUNCE2				((UINT32)(MT6337_REG_BASE+0x1610))
#define ACCDET_DEBOUNCE3				((UINT32)(MT6337_REG_BASE+0x1612))
#define ACCDET_DEBOUNCE4				((UINT32)(MT6337_REG_BASE+0x1614))
#define ACCDET_DEBOUNCE5				((UINT32)(MT6337_REG_BASE+0x1616))
#define ACCDET_DEBOUNCE6				((UINT32)(MT6337_REG_BASE+0x1618))
#define ACCDET_DEBOUNCE7				((UINT32)(MT6337_REG_BASE+0x161A))
#define ACCDET_DEBOUNCE8				((UINT32)(MT6337_REG_BASE+0x161C))
#define ACCDET_DEFAULT_STS				((UINT32)(MT6337_REG_BASE+0x161E))
#define ACCDET_DEFAULT_EINT_STS			((UINT32)(MT6337_REG_BASE+0x1620))
#define ACCDET_IRQ_STATUS				((UINT32)(MT6337_REG_BASE+0x1622))
#define ACCDET_SW_CONTROL				((UINT32)(MT6337_REG_BASE+0x1624))
#define ACCDET_CLK_STATUS				((UINT32)(MT6337_REG_BASE+0x1626))/* RO, clk status */
#define ACCDET_FSM_STATE_RG				((UINT32)(MT6337_REG_BASE+0x1628))/* RO */
#define ACCDET_EINT0_CONTROL			((UINT32)(MT6337_REG_BASE+0x162A))
#define ACCDET_EINT1_CONTROL			((UINT32)(MT6337_REG_BASE+0x162C))
#define ACCDET_EINT_PWM_DELAY			((UINT32)(MT6337_REG_BASE+0x162E))
#define ACCDET_EINT1_PWM_DELAY			((UINT32)(MT6337_REG_BASE+0x1630))
#define ACCDET_INT_CMP_SW				((UINT32)(MT6337_REG_BASE+0x1632))
#define ACCDET_TEST_DEBUG				((UINT32)(MT6337_REG_BASE+0x1634))
#define ACCDET_EINT0_FSM_STATE			((UINT32)(MT6337_REG_BASE+0x1636))/* RO */
#define ACCDET_EINT1_FSM_STATE			((UINT32)(MT6337_REG_BASE+0x1638))/* RO */
#define ACCDET_NEGV						((UINT32)(MT6337_REG_BASE+0x163A))/* RO */
#define ACCDET_CUR_DEB					((UINT32)(MT6337_REG_BASE+0x163C))/* RO */
#define ACCDET_EINT_CUR_DEB				((UINT32)(MT6337_REG_BASE+0x163E))/* RO */
#define ACCDET_EINT1_CUR_DEB			((UINT32)(MT6337_REG_BASE+0x1640))/* RO */
#define ACCDET_RSV_CON0					((UINT32)(MT6337_REG_BASE+0x1642))/* Reserve */
#define ACCDET_RSV_CON1					((UINT32)(MT6337_REG_BASE+0x1644))/* Reserve */
#define ACCDET_AUXADC_CONNECT_TIME		((UINT32)(MT6337_REG_BASE+0x1646))
#define ACCDET_HW_SET					((UINT32)(MT6337_REG_BASE+0x1648))

#define AUDENC_BIAS_POWER_REG			((UINT32)(MT6337_REG_BASE+0x1816))
#define AUDENC_MICBIAS_REG				((UINT32)(MT6337_REG_BASE+0x1840))
#define AUDENC_ADC_REG					((UINT32)(MT6337_REG_BASE+0x1844))

#define REG_ACCDET_AD_CALI				((UINT32)(MT6337_REG_BASE+0x1230))
#define REG_ACCDET_AD_0					((UINT32)(MT6337_REG_BASE+0x1232))
#define REG_ACCDET_AD_1					((UINT32)(MT6337_REG_BASE+0x1234))
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#define PMIC_REG_BASE_START				(0x8000)/* MT6337 register base address start */
#define PMIC_REG_BASE_END				(0x9E00)/* MT6337 register base address end */


/*************Register Bit Define*************/
/* 0x8208: set accdet clock, relevant 0x8606 */
#define RG_ACCDET_CLK_SET					(1<<11)
/* 0x820A: set accdet clock, relevant 0x8606 */
#define RG_ACCDET_CLK_CLR					(1<<11)

/* 0x8402: set accdet, Relevant: 0x8400 */
#define ACCDET_RESET_SET					(1<<4)
/* 0x8404: clear accdet, Relevant: 0x8400 */
#define ACCDET_RESET_CLR					(1<<4)

/* 0x8602: relevant 0x8600 */
#define RG_INT_EN_ACCDET					(1<<4)
#define RG_INT_EN_ACCDET_EINT0				(1<<5)
#define RG_INT_EN_ACCDET_EINT1				(1<<6)
#define RG_INT_EN_ACCDET_EINT				(0x03<<5)
#define RG_INT_EN_ACCDET_NEGV				(1<<7)
/* 0x8604: relevant 0x8600 */
#define RG_INT_CLR_ACCDET					(1<<4)
#define RG_INT_CLR_ACCDET_EINT0				(1<<5)
#define RG_INT_CLR_ACCDET_EINT1				(1<<6)
#define RG_INT_CLR_ACCDET_EINT				(0x03<<5)
#define RG_INT_CLR_ACCDET_NEGV				(1<<7)

/* 0x8608: relevant 0x8606 */
#define RG_INT_MASK_B_ACCDET_EN				(1<<4)
#define RG_INT_MASK_B_ACCDET_EINT0_EN		(1<<5)
#define RG_INT_MASK_B_ACCDET_EINT1_EN		(1<<6)
#define RG_INT_MASK_B_ACCDET_EINT_EN		(0x03<<5)
#define RG_INT_MASK_B_ACCDET_NEGV_EN		(1<<7)
/* 0x860A: relevant 0x8606 */
#define RG_INT_MASK_B_ACCDET_CLR			(1<<4)
#define RG_INT_MASK_B_ACCDET_EINT0_CLR		(1<<5)
#define RG_INT_MASK_B_ACCDET_EINT1_CLR		(1<<6)
#define RG_INT_MASK_B_ACCDET_EINT_CLR		(0x03<<5)
#define RG_INT_MASK_B_ACCDET_NEGV_CLR		(1<<7)

/* 0x8618:  accdet & accdet_eint status issued */
#define RG_INT_STATUS_ACCDET_EN				(1<<4)
#define RG_INT_STATUS_ACCDET_EINT0_EN		(1<<5)
#define RG_INT_STATUS_ACCDET_EINT1_EN		(1<<6)
#define RG_INT_STATUS_ACCDET_NEGV_EN		(1<<7)
#define RG_INT_STATUS_ACCDET_EINT_EN		(0x03<<5)
#define RG_INT_STATUS_ACCDET_DISEN			(0<<4)
#define RG_INT_STATUS_ACCDET_EINT0_DISEN	(0<<5)
#define RG_INT_STATUS_ACCDET_EINT1_DISEN	(0<<6)
#define RG_INT_STATUS_ACCDET_NEGV_DISEN		(0<<7)

/* 0x940A:  Auxadc CH5 read data */
#define AUXADC_DATA_RDY_CH5					(1<<15)
#define AUXADC_DATA_PROCEED_CH5				(0<<15)
#define AUXADC_DATA_MASK					(0x0FFF)


/* 0x942A:  Auxadc CH5 request, relevant 0x9428 */
#define AUXADC_RQST_CH5_SET					(1<<5)

/* 0x942C:  Auxadc CH5 request, relevant 0x9428 */
#define AUXADC_RQST_CH5_CLR					(1<<5)

/* 0x943C: AUXADC_AVG_NUM_SEL */
#define AUXADC_AVG_CH5_NUM_SEL				(0x02A0)


/* 0x945A:  ACCDET auto request enable/disable*/
#define AUXADC_ACCDET_AUTO_SPL_EN			(1<<0)/* ACCDET auto request enable */
#define AUXADC_ACCDET_AUTO_SPL_DISEN		(0<<0)/* ACCDET auto request disable */
#define AUXADC_ACCDET_AUTO_RQST_CLR			(1<<1)/* ACCDET auto request clear */
#define AUXADC_ACCDET_AUTO_RQST_NONE		(0<<1)/* ACCDET auto request none */


/* CON0,0x9600: Input pin, auxadc,accdet,audio analog control. */
#define RG_AUD_ACCDET_ANA_SW_EN_B			(1<<2)
#define RG_ACCDET_INPUT_MICP				(1<<3)
#define RG_ACCDET_INPUT_ACCDET				(0<<3)
#define RG_ACCDET2AUXADC_SW_NORMAL			(0x01<<4)
#define RG_ACCDET2AUXADC_SW_200K			(0x02<<4)
#define RG_ACCDET2AUXADC_SW_100K			(0x04<<4)
#define AUD_ACCDET_AUXADC_SW_CTRL			(1<<10)
#define AUD_ACCDET_AUXADC_MODE_SW			(1<<11)/* SW mode */
#define AUD_ACCDET_AUXADC_MODE_HW			(1<<11)/* HW mode */

/* CON1,0x9602:  Detection, accdet, eint, mode, etc. enable  */
#define ACCDET_ENABLE						(1<<0)
#define ACCDET_DISABLE						(0<<0)
#define ACCDET_SEQ_MODE_INIT				(1<<1)
#define ACCDET_SEQ_MODE_NORMAL				(0<<1)
#define ACCDET_EINT0_EN						(1<<2)
#define ACCDET_EINT0_DISEN					(0<<2)
#define ACCDET_EINT0_SEQ_INIT_EN			(1<<3)
#define ACCDET_EINT0_SEQ_INIT_DISEN			(0<<3)
#define ACCDET_EINT1_EN						(1<<4)
#define ACCDET_EINT1_DISEN					(1<<4)
#define ACCDET_EINT1_SEQ_INIT_EN			(1<<5)
#define ACCDET_EINT1_SEQ_INIT_DISEN			(0<<5)
#define ACCDET_NEGVDET_EN					(1<<6)
#define ACCDET_NEGVDET_CTRL_HW_EN			(0<<7)
#define ACCDET_NEGVDET_CTRL_SW_EN			(1<<7)
#define ANA_SW_AUXADC						(0<<8)
#define ANA_SW_ACCDET						(1<<8)
#define ACCDET_EINT_SEQ_INIT_EN				(0x05<<3)


/* CON2,0x9604:PWM accdet unit, eint uint,cmp uint,mbias uint  enable/disable  */
#define ACCDET_EINT1_PWM_IDLE				(1<<11)
#define ACCDET_EINT0_PWM_IDLE				(1<<10)
#define ACCDET_CMP1_PWM_IDLE				(1<<9)
#define ACCDET_MBIAS_PWM_IDLE				(1<<8)
#define ACCDET_VTH_PWM_IDLE					(1<<7)
#define ACCDET_CMP0_PWM_IDLE				(1<<6)
#define ACCDET_PWM_IDLE_0					(0x07<<6)
#define ACCDET_PWM_IDLE						(0x0F<<6)
#define ACCDET_EINT1_PWM_EN					(1<<5)
#define ACCDET_EINT0_PWM_EN					(1<<4)
#define ACCDET_CMP1_PWM_EN					(1<<3)
#define ACCDET_MBIAS_PWM_EN					(1<<2)
#define ACCDET_VTH_PWM_EN					(1<<1)
#define ACCDET_CMP0_PWM_EN					(1<<0)
#define ACCDET_EINT1_PWM_DISEN				(0<<5)
#define ACCDET_EINT0_PWM_DISEN				(0<<4)
#define ACCDET_CMP1_PWM_DISEN				(0<<3)
#define ACCDET_MBIAS_PWM_DISEN				(0<<2)
#define ACCDET_VTH_PWM_DISEN				(0<<1)
#define ACCDET_CMP0_PWM_DISEN				(0<<0)
#define ACCDET_SWCTRL_ACCDET_EN	(ACCDET_VTH_PWM_EN|ACCDET_MBIAS_PWM_EN)
#define ACCDET_SWCTRL_CMP_EN	(ACCDET_CMP0_PWM_EN|ACCDET_CMP1_PWM_EN)
#define ACCDET_EINT_PWM_EN		(ACCDET_EINT0_PWM_EN|ACCDET_EINT1_PWM_EN)
#define ACCDET_SWCTRL_EN		(ACCDET_SWCTRL_ACCDET_EN|ACCDET_SWCTRL_CMP_EN|ACCDET_EINT_PWM_EN)


/* CON3-CON5,0x9606 ~ 0x960A set ACCDET PWM width, thresh, rise/falling */
/* CON6-CON14,0x960C ~ 0x961C  set debounce[0-8]. */
/* CON15,0x961E set default value of MEM,CUR,SAM, etc. */

/* CON16,0x9620: set default value of accdet eint if use high_level trigger */
#define ACCDET_EINT0_IVAL					(0x7000)
#define ACCDET_EINT0_IVAL_SEL				(0x8000)
#define ACCDET_EINT1_IVAL					(0x07)
#define ACCDET_EINT1_IVAL_SEL				(0x08)
#define ACCDET_EINT_IVAL					(0x7007)
#define ACCDET_EINT_IVAL_SEL				(0x8008)

/* CON17,0x9622:  ACCDET(ABC) interrupt status */
#define ACCDET_EINT1_IRQ_POL_HIGH			(1<<15)
#define ACCDET_EINT1_IRQ_POL_LOW			(1<<15)
#define ACCDET_EINT0_IRQ_POL_HIGH			(1<<14)
#define ACCDET_EINT0_IRQ_POL_LOW			(1<<14)
#define ACCDET_EINT_IRQ_POL_HIGH			(0x03<<14)
#define ACCDET_EINT_IRQ_POL_LOW				(0x03<<14)
#define ACCDET_IRQ_EINT1_CLR_BIT			(1<<11)
#define ACCDET_IRQ_EINT0_CLR_BIT			(1<<10)
#define ACCDET_IRQ_EINT_CLR					(0x03<<10)
#define ACCDET_IRQ_NEGV_CLR_BIT				(1<<9)
#define ACCDET_IRQ_CLR_BIT					(1<<8)
#define ACCDET_EINT1_IRQ_STATUS_BIT			(1<<3)
#define ACCDET_EINT0_IRQ_STATUS_BIT			(1<<2)
#define ACCDET_EINT_IRQ_STATUS				(0x03<<2)/* RO */
#define ACCDET_NEGV_IRQ_STATUS_BIT			(1<<1)/* RO */
#define ACCDET_IRQ_STATUS_BIT				(1<<0)/* RO */
#define ACCDET_IRQ_STS_BIT_ALL				(0x0F)/* RO */

/* CON18,0x9624: accdet pwm,cmp,mbias,SW selection, etc. */
#define ACCDET_CMP0_SWSEL					(1<<1)
#define ACCDET_VTH_SWSEL					(1<<2)
#define ACCDET_MBIAS_SWSEL					(1<<3)
#define ACCDET_CMP1_SWSEL					(1<<4)
#define ACCDET_CMP0_HWSEL					(0<<1)
#define ACCDET_VTH_HWSEL					(0<<2)
#define ACCDET_MBIAS_HWSEL					(0<<3)
#define ACCDET_CMP1_HWSEL					(0<<4)
#define ACCDET_PWM_SEL_CMP					(0x00<<9)
#define ACCDET_PWM_SEL_VTH					(0x01<<9)
#define ACCDET_PWM_SEL_MBIAS				(0x02<<9)
#define ACCDET_PWM_SEL_SW					(0x03<<9)
#define ACCDET_CMP1_EN_SW					(1<<11)
#define ACCDET_CMP0_EN_SW					(1<<12)
#define ACCDET_VTH_EN_SW					(1<<12)
#define ACCDET_MBIAS_EN_SW					(1<<14)
#define ACCDET_PWM_EN_SW					(1<<15)
#define ACCDET_CMP1_DISEN_SW				(0<<11)
#define ACCDET_CMP_DISEN_SW					(0<<12)
#define ACCDET_VTH_DISEN_SW					(0<<12)
#define ACCDET_MBIAS_DISEN_SW				(0<<14)
#define ACCDET_PWM_DISEN_SW					(0<<15)

/* CON19,0x9626: RO, accdet, mbias,cmp,etc.  clk */
/* CON20,0x9628: RO, accdet FSM state */
#define ACCDET_STATE_MEM_BIT_OFFSET			9
#define ACCDET_STATE_ABC_VALUE				0x07
#define ACCDET_STATE_ABC_000				(0x00)/* debounce0 */
#define ACCDET_STATE_ABC_001				(0x01)/* debounce4, no use */
#define ACCDET_STATE_ABC_010				(0x02)/* debounce1*/
#define ACCDET_STATE_ABC_011				(0x03)/* debounce5 */
#define ACCDET_STATE_ABC_100				(0x04)/* debounce2, no use */
#define ACCDET_STATE_ABC_101				(0x05)/* debounce6, no use */
#define ACCDET_STATE_ABC_110				(0x06)/* debounce3 */
#define ACCDET_STATE_ABC_111				(0x07)/* debounce7, no use */

/* CON21,0x962A: accdet eint0 debounce, PWM width&thresh, etc. set */
#define ACCDET_EINT0_DEB_BYPASS				(0x00<<3)/* 0ms */
#define ACCDET_EINT0_DEB_IN_256				(0x0E<<3)/* 256ms */
#define ACCDET_EINT0_DEB_OUT_012			(0x01<<3)/* 0.12ms */
#define ACCDET_EINT0_DEB_512				(0x0F<<3)/* 512ms */
#define ACCDET_EINT0_PWM_THRSH				(0x06<<8)/* 16ms */
#define ACCDET_EINT0_PWM_WIDTH				(0x02<<12)/* 16ms */

/* CON22,0x962C: accdet eint1 debounce, PWM width&thresh, etc. set */
#define ACCDET_EINT1_DEB_BYPASS				(0x00<<3)/* 0ms */
#define ACCDET_EINT1_DEB_IN_256				(0x0E<<3)/* 256ms */
#define ACCDET_EINT1_DEB_OUT_012			(0x01<<3)/* 0.12ms */
#define ACCDET_EINT1_PWM_THRSH				(0x06<<8)/* 16ms */
#define ACCDET_EINT1_PWM_WIDTH				(0x02<<12)/* 16ms */

/* CON23,0x962E: accdet eint0 PWM rise/falling etc. set */
/* CON24,0x9630: accdet eint1 PWM rise/falling etc. set */

/* CON25,0x9632: accdet eint CMP,AUXADC,etc. switch */
#define ACCDET_EINT0_CMP_OUT_SW				(1<<1)
#define ACCDET_EINT1_CMP_OUT_SW				(1<<2)
#define ACCDET_AUXADC_CTRL_SW				(1<<3)
#define ACCDET_EINT0_CMP_EN_SW				(1<<14)
#define ACCDET_EINT1_CMP_EN_SW				(1<<15)

/* CON26,0x9634: accdet test mode */
/* CON27,0x9636: RO. accdet_eint FSM state */
/* CON28,0x9638: RO. accdet_eint1 FSM state */
/* CON29,0x963A: RO. accdet_NEGV state */
/* CON30,0x963C: RO. accdet current used debounce time */
/* CON31,0x963E: RO. accdet_eint current used debounce time */
/* CON32,0x9640: RO. accdet_eint1 current used debounce time */
/* CON33-CON34,0x9642~0x9644: reserve */
/* CON35,0x9646: auxadc to accdet connect time (time/32K) */

/* 0x9648: SW/HW path & CMPC enable/disable.
 * 0x00--> HW mode is triggered by eint0
 * 0x01--> HW mode is triggered by eint1
 * 0x02--> HW mode is triggered by eint0 or eint1
 * 0x03--> HW mode is triggered by eint0 and eint1
 */
#define ACCDET_HWEN_SEL_0					(0x00)
#define ACCDET_HWEN_SEL_1					(0x01)
#define ACCDET_HWEN_SEL_0_OR_1				(0x02)
#define ACCDET_HWEN_SEL_0_AND_1				(0x03)

#define ACCDET_HWMODE_EN					(1<<4)
#define ACCDET_HWMODE_DISABLE				(0<<4)
#define ACCDET_EN_CMPC						(0<<15)/* default: for 5-pole set */
#define ACCDET_DISABLE_CMPC					(1<<15)/* Mask CMP C */

/* 0x9816: Ibias Distib circuit enable/disable */
#define RG_IBIAS_PWRDN_EN					(1<<15)
#define RG_IBIAS_PWRDN_DISEN				(0<<15)

/* 0x9840: Mic-bias1 output voltage & lowpower Enable */
#define RG_AUD_MBIAS1_DC_SW_1P				(0x01<<1)/* MIC bias 1 DC couple switch 1P */
#define RG_AUD_MBIAS1_DC_SW_1N				(0x01<<2)/* MIC bias 1 DC couple switch 1N */
#define RG_AUD_MBIAS1_DC_SW_3P				(0x01<<8)/* MIC bias 1 DC couple switch 3P */
#define RG_AUD_MBIAS1_DC_SW_3N				(0x01<<9)/* MIC bias 1 DC couple switch 3N */
#define RG_MBIAS_MODE_6	(RG_AUD_MBIAS1_DC_SW_1P | RG_AUD_MBIAS1_DC_SW_3P)/* mic_bias_mode= 6 */

#define RG_MBIAS_LOWPOWER_EN				(1<<7)
#define RG_MBIAS_NORMAL_EN					(0<<7)
#define RG_MBIAS_OUTPUT_1V9					(0x02<<4)/* 1.9V */
#define RG_MBIAS_OUTPUT_2V5					(0x05<<4)/* 2.5V */
#define RG_MBIAS_OUTPUT_2V7					(0x07<<4)/* 2.7V */
#define RG_MICBIAS1_LOWPEN_OUTVOL	(RG_MBIAS_LOWPOWER_EN | RG_MBIAS_OUTPUT_2V7)/* add */

/* 0x9844: Mic-bias1 pull low enable/disable */
#define RG_AUD_ACCDET_MBIAS1_EN				(1<<1)
#define RG_AUD_ACCDET_MBIAS1_DISABLE        (0<<1)
#define RG_ACCDET1_SEL_MIC1P				(1<<7)
#define RG_ACCDET1_SEL_ACCDET1				(0<<7)
#define RG_ACCDET2_SEL_MIC3P				(1<<8)
#define RG_ACCDET2_SEL_ACCDET2				(0<<8)
#define RG_SWBUF_SW_EN						(1<<10)/* power on */
#define RG_SWBUF_SW_DISEN					(0<<10)/* power off */
#define ACCDET_EINT0_CON_EN					(1<<12)
#define ACCDET_EINT1_CON_EN					(1<<13)

#define ACCDET_SEL_EINT_EN					(0x3182)
#define ACCDET_SEL_EINT0_EN		(RG_ACCDET1_SEL_MIC1P | ACCDET_EINT0_CON_EN)
#define ACCDET_SEL_EINT1_EN		(RG_ACCDET2_SEL_MIC3P | ACCDET_EINT1_CON_EN)
#define ACCDET_SEL_EINT_CON_EN	(ACCDET_EINT0_EN | ACCDET_EINT1_EN)


#define RG_OTP_PA_ADDR_WORD_INDEX			(0x03)
#define RG_OTP_PA_ACCDET_BIT_SHIFT			(0x06)

/*
 * [0x9230]--[0x9232]--[0x92334]
 * accdet adc_cali bit6~bit13;
 * accdet0:(bit14~bit15+bit0~bit5);
 * accdet1:(bit6~bit13);
 * accdet2:(bit14~bit15+bit0~bit5);
 * accdet3:(bit6~bit13);
 */
#define ACCDET_CALI_MASK					(0xFF<<6)/* 0x9230 */
#define ACCDET_0_MASK0						(0x03<<14)/* 0x9230 */
#define ACCDET_0_MASK1						(0x3F<<0)/* 0x9232 */
#define ACCDET_1_MASK						(0xFF<<6)/* 0x9232 */
#define ACCDET_2_MASK0						(0x03<<14)/* 0x9232 */
#define ACCDET_2_MASK1						(0x3F<<0)/* 0x9234 */
#define ACCDET_3_MASK						(0xFF<<6)/* 0x9234 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#endif

