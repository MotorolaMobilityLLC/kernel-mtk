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

/*Register address define*/
#include <mach/upmu_hw.h>

#ifdef CONFIG_MTK_PMIC_NEW_ARCH

#ifdef CONFIG_MTK_PMIC_CHIP_MT6353
#include "reg_accdet_mt6353.h"
#endif

#else

#define ACCDET_BASE                 0x00000000
#define TOP_RST_ACCDET		     MT6351_TOP_RST_CON0
#define TOP_RST_ACCDET_SET		MT6351_TOP_RST_CON0_SET
#define TOP_RST_ACCDET_CLR		MT6351_TOP_RST_CON0_CLR

#define INT_CON_ACCDET           MT6351_INT_CON0
#define INT_CON_ACCDET_SET		 MT6351_INT_CON0_SET
#define INT_CON_ACCDET_CLR       MT6351_INT_CON0_CLR

#define INT_STATUS_ACCDET        MT6351_INT_STATUS0
#define INT_STATUS_1             MT6351_INT_STATUS1

/*clock register*/
#define TOP_CKPDN                MT6351_TOP_CKPDN_CON2
#define TOP_CKPDN_SET            MT6351_TOP_CKPDN_CON2_SET
#define TOP_CKPDN_CLR            MT6351_TOP_CKPDN_CON2_CLR

#define ACCDET_RSV               MT6351_ACCDET_CON0
#define ACCDET_CTRL              MT6351_ACCDET_CON1
#define ACCDET_STATE_SWCTRL      MT6351_ACCDET_CON2
#define ACCDET_PWM_WIDTH         MT6351_ACCDET_CON3
#define ACCDET_PWM_THRESH        MT6351_ACCDET_CON4
#define ACCDET_EN_DELAY_NUM      MT6351_ACCDET_CON5
#define ACCDET_DEBOUNCE0         MT6351_ACCDET_CON6
#define ACCDET_DEBOUNCE1         MT6351_ACCDET_CON7
#define ACCDET_DEBOUNCE2         MT6351_ACCDET_CON8
#define ACCDET_DEBOUNCE3         MT6351_ACCDET_CON9
#define ACCDET_DEBOUNCE4         MT6351_ACCDET_CON10
#define ACCDET_DEFAULT_STATE_RG  MT6351_ACCDET_CON11
#define ACCDET_IRQ_STS           MT6351_ACCDET_CON12
#define ACCDET_CONTROL_RG        MT6351_ACCDET_CON13
#define ACCDET_STATE_RG          MT6351_ACCDET_CON14
#define ACCDET_EINT_CTL			 MT6351_ACCDET_CON15
#define ACCDET_EINT_PWM_DELAY	 MT6351_ACCDET_CON16
#define ACCDET_TEST_DEBUG	     MT6351_ACCDET_CON17
#define ACCDET_NEGV              MT6351_ACCDET_CON20
#define ACCDET_CUR_DEB			 MT6351_ACCDET_CON21
#define ACCDET_EINT_CUR_DEB		 MT6351_ACCDET_CON22
#define ACCDET_RSV_CON0			 MT6351_ACCDET_CON23
#define ACCDET_RSV_CON1			 MT6351_ACCDET_CON24

#define ACCDET_AUXADC_CTL	     MT6351_AUXADC_RQST0
#define ACCDET_AUXADC_CTL_SET	 MT6351_AUXADC_RQST0_SET
#define ACCDET_AUXADC_REG	     MT6351_AUXADC_ADC5
#define ACCDET_AUXADC_AUTO_SPL   MT6351_AUXADC_ACCDET

#define ACCDET_ADC_REG           MT6351_AUDENC_ANA_CON11
#define ACCDET_MICBIAS_REG       MT6351_AUDENC_ANA_CON10

/*Register value define*/

#define ACCDET_AUXADC_AUTO_SET   (1<<0)
#define ACCDET_DATA_READY        (1<<15)
#define ACCDET_CH_REQ_EN         (1<<5)
#define ACCDET_DATA_MASK         (0x0FFF)

#define ACCDET_POWER_MOD         (1<<13)
#define ACCDET_MIC1_ON           (1<<7)
#define ACCDET_BF_ON             (1<<9)
#define ACCDET_BF_OFF            (0<<9)
#define ACCDET_BF_MOD            (1<<11)
#define ACCDET_INPUT_MICP        (1<<3)
#define ACCDET_EINT_CON_EN       (1<<11)
#define ACCDET_NEGV_DT_EN        (1<<13)

#define ACCDET_CTRL_EN           (1<<0)
#define ACCDET_EINT_EN           (1<<2)
#define ACCDET_NEGV_EN           (1<<4)
#define ACCDET_EINT_PWM_IDLE     (1<<7)
#define ACCDET_MIC_PWM_IDLE      (1<<6)
#define ACCDET_VTH_PWM_IDLE      (1<<5)
#define ACCDET_CMP_PWM_IDLE      (1<<4)
#define ACCDET_EINT_PWM_EN       (1<<3)
#define ACCDET_CMP_EN            (1<<0)
#define ACCDET_VTH_EN            (1<<1)
#define ACCDET_MICBIA_EN         (1<<2)


#define ACCDET_ENABLE			 (1<<0)
#define ACCDET_DISABLE			 (0<<0)

#define ACCDET_RESET_SET          (1<<4)
#define ACCDET_RESET_CLR          (1<<4)

#define IRQ_CLR_BIT              0x100
#define IRQ_EINT_CLR_BIT         0x400
#define IRQ_NEGV_CLR_BIT         0x200
#define IRQ_STATUS_BIT           (1<<0)
#define EINT_IRQ_STATUS_BIT      (1<<2)
#define NEGV_IRQ_STATUS_BIT      (1<<1)
#define EINT_IRQ_DE_OUT          0x00 /* 1 msec */
#define EINT_IRQ_DE_IN           0x60 /* 256 msec */
#define EINT_PWM_THRESH          0x400
#define EINT_IRQ_POL_HIGH        (1<<15)
#define EINT_IRQ_POL_LOW         (1<<15)


#define RG_ACCDET_IRQ_SET        (1<<12)
#define RG_ACCDET_IRQ_CLR        (1<<12)
#define RG_ACCDET_IRQ_STATUS_CLR (1<<12)

#define RG_ACCDET_EINT_IRQ_SET        (1<<13)
#define RG_ACCDET_EINT_IRQ_CLR        (1<<13)
#define RG_ACCDET_EINT_IRQ_STATUS_CLR (1<<10)
#define RG_ACCDET_EINT_HIGH				(1<<15)
#define RG_ACCDET_EINT_LOW            (0<<15)

#define RG_ACCDET_NEGV_IRQ_SET        (1<<14)
#define RG_ACCDET_NEGV_IRQ_CLR        (1<<14)
#define RG_ACCDET_NEGV_IRQ_STATUS_CLR (1<<14)

#define RG_OTP_PA_ADDR_WORD_INDEX (0x03)
#define RG_OTP_PA_ACCDET_BIT_SHIFT (0x05)

/*CLOCK*/
#define RG_ACCDET_CLK_SET        (1<<9)
#define RG_ACCDET_CLK_CLR        (1<<9)


#define ACCDET_PWM_EN_SW         (1<<15)
#define ACCDET_MIC_EN_SW         (1<<14)
#define ACCDET_VTH_EN_SW         (1<<13)
#define ACCDET_CMP_EN_SW         (1<<12)

#define ACCDET_SWCTRL_EN         0x07

#define ACCDET_IN_SW             0x10

#define ACCDET_DE4               0x42 /*2ms*/
/*
#define ACCDET_PWM_SEL_CMP       0x00
#define ACCDET_PWM_SEL_VTH       0x01
#define ACCDET_PWM_SEL_MIC       0x10
#define ACCDET_PWM_SEL_SW        0x11



#define ACCDET_TEST_MODE5_ACCDET_IN_GPI        (1<<5)
#define ACCDET_TEST_MODE4_ACCDET_IN_SW        (1<<4)
#define ACCDET_TEST_MODE3_MIC_SW        (1<<3)
#define ACCDET_TEST_MODE2_VTH_SW        (1<<2)
#define ACCDET_TEST_MODE1_CMP_SW        (1<<1)
#define ACCDET_TEST_MODE0_GPI        (1<<0)
//#define ACCDET_DEFVAL_SEL        (1<<15)
*/
/*power mode and auxadc switch on/off*/
#define ACCDET_1V9_MODE_OFF   0x1A10
#define ACCDET_2V8_MODE_OFF   0x5A10
#define ACCDET_1V9_MODE_ON   0x1E10
#define ACCDET_2V8_MODE_ON   0x5A20
#define ACCDET_SWCTRL_IDLE_EN    (0x07<<4)

#endif
