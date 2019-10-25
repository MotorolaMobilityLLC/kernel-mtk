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
#ifndef _REG_ACCDET_H_
#define _REG_ACCDET_H_

#include <mach/upmu_hw.h>
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355) /* support mt6355 */
/* Register address define */
#define ACCDET_BASE (0x00000000)
#define TOP_RST_ACCDET ((unsigned int)(ACCDET_BASE + 0x0600))
#define TOP_RST_ACCDET_SET ((unsigned int)(ACCDET_BASE + 0x0602))
#define TOP_RST_ACCDET_CLR ((unsigned int)(ACCDET_BASE + 0x0604))
#define INT_CON_ACCDET ((unsigned int)(ACCDET_BASE + 0x0824))
#define INT_CON_ACCDET_SET ((unsigned int)(ACCDET_BASE + 0x0826))
#define INT_CON_ACCDET_CLR ((unsigned int)(ACCDET_BASE + 0x0828))
#define INT_STATUS_ACCDET ((unsigned int)(ACCDET_BASE + 0x0860))
#define INT_STATUS_1 ((unsigned int)(ACCDET_BASE + 0x0856)) /* no use */
/* clock register */
#define TOP_CKPDN ((unsigned int)(ACCDET_BASE + 0x0412))
#define TOP_CKPDN_SET ((unsigned int)(ACCDET_BASE + 0x0414))
#define TOP_CKPDN_CLR ((unsigned int)(ACCDET_BASE + 0x0416))
/*  accdet register */
#define ACCDET_RSV ((unsigned int)(ACCDET_BASE + 0x1C00))
#define ACCDET_CTRL ((unsigned int)(ACCDET_BASE + 0x1C02))
#define ACCDET_STATE_SWCTRL ((unsigned int)(ACCDET_BASE + 0x1C04))
#define ACCDET_PWM_WIDTH ((unsigned int)(ACCDET_BASE + 0x1C06))
#define ACCDET_PWM_THRESH ((unsigned int)(ACCDET_BASE + 0x1C08))
#define ACCDET_EN_DELAY_NUM ((unsigned int)(ACCDET_BASE + 0x1C0A))
#define ACCDET_DEBOUNCE0 ((unsigned int)(ACCDET_BASE + 0x1C0C))
#define ACCDET_DEBOUNCE1 ((unsigned int)(ACCDET_BASE + 0x1C0E))
#define ACCDET_DEBOUNCE2 ((unsigned int)(ACCDET_BASE + 0x1C10))
#define ACCDET_DEBOUNCE3 ((unsigned int)(ACCDET_BASE + 0x1C12))
#define ACCDET_DEBOUNCE4 ((unsigned int)(ACCDET_BASE + 0x1C14))
#define ACCDET_DEFAULT_STATE_RG ((unsigned int)(ACCDET_BASE + 0x1C16))
#define ACCDET_IRQ_STS ((unsigned int)(ACCDET_BASE + 0x1C18))
#define ACCDET_CONTROL_RG ((unsigned int)(ACCDET_BASE + 0x1C1A))
#define ACCDET_STATE_RG ((unsigned int)(ACCDET_BASE + 0x1C1C)) /* RO */
#define ACCDET_EINT_CTL ((unsigned int)(ACCDET_BASE + 0x1C1E))
#define ACCDET_EINT_PWM_DELAY ((unsigned int)(ACCDET_BASE + 0x1C20))
#define ACCDET_TEST_DEBUG ((unsigned int)(ACCDET_BASE + 0x1C22))
#define ACCDET_EINT_STATE ((unsigned int)(ACCDET_BASE + 0x1C24))
#define ACCDET_CUR_DEB ((unsigned int)(ACCDET_BASE + 0x1C26))
#define ACCDET_EINT_CUR_DEB ((unsigned int)(ACCDET_BASE + 0x1C28))
#define ACCDET_RSV_CON0 ((unsigned int)(ACCDET_BASE + 0x1C2A)) /* Reserve */
#define ACCDET_RSV_CON1 ((unsigned int)(ACCDET_BASE + 0x1C2C)) /* Reserve */
#define ACCDET_AUXADC_CON_TIME ((unsigned int)(ACCDET_BASE + 0x1C2E))
#define ACCDET_HW_MODE_DFF                                                     \
	((unsigned int)(ACCDET_BASE + 0x1C30)) /* accdet eint HW mode */
/* auxadc need check */
#define ACCDET_AUXADC_CTL ((unsigned int)(ACCDET_BASE + 0x32A0))
#define ACCDET_AUXADC_CTL_SET ((unsigned int)(ACCDET_BASE + 0x32A2))
#define ACCDET_AUXADC_REG ((unsigned int)(ACCDET_BASE + 0x320A))
#define ACCDET_AUXADC_AUTO_SPL ((unsigned int)(ACCDET_BASE + 0x32F0))
#define ACCDET_ADC_REG ((unsigned int)(ACCDET_BASE + 0x3636))
#define ACCDET_MICBIAS_REG ((unsigned int)(ACCDET_BASE + 0x3634))
#define REG_ACCDET_AD_CALI_0 ((unsigned int)(ACCDET_BASE + 0x1E84))
#define REG_ACCDET_AD_CALI_1 ((unsigned int)(ACCDET_BASE + 0x1E86))
#define REG_ACCDET_AD_CALI_2 ((unsigned int)(ACCDET_BASE + 0x1E88))
#define REG_ACCDET_AD_CALI_3 ((unsigned int)(ACCDET_BASE + 0x1E8A))
#define PMIC_REG_BASE_START (0x0000) /* MT6355 register base address start */
#define PMIC_REG_BASE_END (0x380A)   /* MT6355 register base address end */
/* Bit Define: analog bias, ACCDET_MICBIAS_REG */
#define RG_AUDMICBIAS1LOWPEN                                                   \
	(0x01 << 7) /* Mic bias1: 0,Normal mode; 1, Lowpower mode */
#define RG_AUDMICBIAS1VREF (0x07 << 4)
	/* Micbias1 voltage output: 000,1.7V;010,1.9V;101,2.5V;111,2.7V */

#define RG_AUDMICBIAS1DCSW1PEN (0x01 << 1)
	/* 1,Mic bias1 DC couple switch 1P, for mode6 */

#define RG_AUDMICBIAS1DCSW1NEN (0x01 << 2)
	/* 1,Mic bias1 DC couple switch 1N */

#define RG_MICBIAS1DCSWPEN (RG_AUDMICBIAS1DCSW1PEN)
/* Bit Define: analog bias, ACCDET_ADC_REG */

#define RG_AUDACCDETMICBIAS0PULLLOW (0x01 << 0)
#define RG_AUDACCDETMICBIAS1PULLLOW (0x01 << 1)
	/* Mic bias1: 0,Normal mode; 1, Lowpower mode */

#define RG_AUDACCDETMICBIAS2PULLLOW (0x01 << 2)
	/* Micbias1 voltage output:010,1.9V;101,2.5V;111,2.7V */

#define RG_AUDACCDETVIN1PULLLOW (0x01 << 3)
#define RG_AUDACCDETPULLLOW (0x0F)
	/* Pull low Mic bias pads when MICBIAS is off */
#define RG_ACCDETSEL (0x01 << 8)
#define RG_EINTTHIRENB (0x00 << 4)
#define RG_EINTCONFIGACCDET (0x01 << 12)
	/* 1,enable Internal connection between ACCDET and EINT */

#define RG_EINT_ANA_CONFIG (RG_ACCDETSEL | RG_EINTCONFIGACCDET)
/* Bit Define: clock, TOP_CKPDN_SET */
#define RG_ACCDET_CLK_SET (1 << 2)
#define RG_ACCDET_CLK_CLR (1 << 2)
/* Bit Define: INT_CON_ACCDET_SET */
#define RG_ACCDET_IRQ_SET (1 << 7)
#define RG_ACCDET_IRQ_CLR (1 << 7)
#define RG_ACCDET_EINT_IRQ_SET (1 << 8)
#define RG_ACCDET_EINT_IRQ_CLR (1 << 8)
#define RG_ACCDET_IRQ_STATUS_CLR (1 << 12)
#define RG_ACCDET_EINT_IRQ_STATUS_CLR (1 << 10)
/* Bit Define: INT_STATUS_ACCDET(0x0860) */
#define RG_INT_STATUS_ACCDET (0x01 << 7)
#define RG_INT_STATUS_ACCDET_EINT (0x01 << 8)
/* Bit Define: ACCDET_EINT_CTL */
#define EINT_IRQ_DE_OUT (0x00 << 3) /* 0.12 msec */
#define EINT_IRQ_DE_IN (0x0E << 3)  /* 256 msec */
#define EINT_PLUG_DEB_CLR (~(0x0F << 3))
#define EINT_PWM_THRESH 0x400
/* Bit Define:  ACCDET_HW_MODE_DFF */
#define ACCDET_HWMODE_SEL (0x01 << 0) /* Enable HW mode */
#define ACCDET_EINT_DEB_OUT_DFF (0x01 << 1)
	/* 1, modified for path mt6355;0,path to mt6337 */
	/* [0x1E84]--[0x1E8A] */
#define RG_ACCDET_BIT_SHIFT (0x09)
#define RG_ACCDET_HIGH_BIT_SHIFT (0x07)
#define ACCDET_CALI_MASK0 (0x7F) /* offset mask */
#define ACCDET_CALI_MASK1 (0xFF) /* reserve */
#define ACCDET_CALI_MASK2 (0x7F) /* AD efuse mask */
#define ACCDET_CALI_MASK3 (0xFF) /* DB efuse mask */
#define ACCDET_CALI_MASK4 (0x7F) /* BC efuse mask */
#else /* support mt6351 */
/* Register address define */
#define ACCDET_BASE 0x00000000
#define TOP_RST_ACCDET MT6351_TOP_RST_CON0
#define TOP_RST_ACCDET_SET MT6351_TOP_RST_CON0_SET
#define TOP_RST_ACCDET_CLR MT6351_TOP_RST_CON0_CLR
#define INT_CON_ACCDET MT6351_INT_CON0
#define INT_CON_ACCDET_SET MT6351_INT_CON0_SET
#define INT_CON_ACCDET_CLR MT6351_INT_CON0_CLR
#define INT_STATUS_ACCDET MT6351_INT_STATUS0
#define INT_STATUS_1 MT6351_INT_STATUS1
/* clock register */
#define TOP_CKPDN MT6351_TOP_CKPDN_CON2
#define TOP_CKPDN_SET MT6351_TOP_CKPDN_CON2_SET
#define TOP_CKPDN_CLR MT6351_TOP_CKPDN_CON2_CLR
#define ACCDET_RSV MT6351_ACCDET_CON0
#define ACCDET_CTRL MT6351_ACCDET_CON1
#define ACCDET_STATE_SWCTRL MT6351_ACCDET_CON2
#define ACCDET_PWM_WIDTH MT6351_ACCDET_CON3
#define ACCDET_PWM_THRESH MT6351_ACCDET_CON4
#define ACCDET_EN_DELAY_NUM MT6351_ACCDET_CON5
#define ACCDET_DEBOUNCE0 MT6351_ACCDET_CON6
#define ACCDET_DEBOUNCE1 MT6351_ACCDET_CON7
#define ACCDET_DEBOUNCE2 MT6351_ACCDET_CON8
#define ACCDET_DEBOUNCE3 MT6351_ACCDET_CON9
#define ACCDET_DEBOUNCE4 MT6351_ACCDET_CON10
#define ACCDET_DEFAULT_STATE_RG MT6351_ACCDET_CON11
#define ACCDET_IRQ_STS MT6351_ACCDET_CON12
#define ACCDET_CONTROL_RG MT6351_ACCDET_CON13
#define ACCDET_STATE_RG MT6351_ACCDET_CON14
#define ACCDET_EINT_CTL MT6351_ACCDET_CON15
#define ACCDET_EINT_PWM_DELAY MT6351_ACCDET_CON16
#define ACCDET_TEST_DEBUG MT6351_ACCDET_CON17
#define ACCDET_NEGV MT6351_ACCDET_CON20
#define ACCDET_CUR_DEB MT6351_ACCDET_CON21
#define ACCDET_EINT_CUR_DEB MT6351_ACCDET_CON22
#define ACCDET_RSV_CON0 MT6351_ACCDET_CON23
#define ACCDET_RSV_CON1 MT6351_ACCDET_CON24
#define ACCDET_AUXADC_CTL MT6351_AUXADC_RQST0
#define ACCDET_AUXADC_CTL_SET MT6351_AUXADC_RQST0_SET
#define ACCDET_AUXADC_REG MT6351_AUXADC_ADC5
#define ACCDET_AUXADC_AUTO_SPL MT6351_AUXADC_ACCDET
#define ACCDET_ADC_REG MT6351_AUDENC_ANA_CON11
#define ACCDET_MICBIAS_REG MT6351_AUDENC_ANA_CON10
#define PMIC_REG_BASE_START (0x0000) /* MT6351 register base address start */
#define PMIC_REG_BASE_END (0x0FE0)   /* MT6351 register base address end */
/* Bit Define: analog bias, ACCDET_MICBIAS_REG */
#define RG_AUDMICBIAS1LOWPEN (0x01 << 7)
	/* Mic bias1: 0,Normal mode; 1, Lowpower mode */
#define RG_AUDMICBIAS1VREF (0x07 << 4)
	/* Micbias1 voltage output:010,1.9V;101,2.5V;111,2.7V */
#define RG_AUDMICBIAS1DCSW3PEN(0x01 << 8)
	/* 1,Mic bias1 DC couple switch 3P, for 5-pole detect */
#define RG_AUDMICBIAS1DCSW3NEN (0x01 << 9) /* 1,Mic bias1 DC couple switch 3N */
#define RG_AUDMICBIAS1DCSW1PEN (0x01 << 2)
	/* 1,Mic bias1 DC couple switch 1P, for mode6 */
#define RG_AUDMICBIAS1DCSW1NEN (0x01 << 1) /* 1,Mic bias1 DC couple switch 1N */
#define RG_MICBIAS1DCSWPEN (RG_AUDMICBIAS1DCSW3PEN | RG_AUDMICBIAS1DCSW1PEN)
/* Bit Define: analog bias, ACCDET_ADC_REG */
#define RG_AUDACCDETMICBIAS0PULLLOW (0x01 << 0)
#define RG_AUDACCDETMICBIAS1PULLLOW (0x01 << 1)
	/* Mic bias1: 0,Normal mode; 1, Lowpower mode */
#define RG_AUDACCDETMICBIAS2PULLLOW (0x01 << 2)
	/* Micbias1 voltage output:010,1.9V;101,2.5V;111,2.7V */
#define RG_AUDACCDETVIN1PULLLOW (0x01 << 3)
#define RG_AUDACCDETPULLLOW (0x0F)
	/* Pull low Mic bias pads when MICBIAS is off */
#define RG_ACCDET1SEL (0x01 << 6) /* accdet input select */
#define RG_ACCDET2SEL (0x01 << 7) /* accdet input select */
/* #define RG_EINTTHIRENB		(0x00<<4)  ???*/
#define RG_EINTCONFIGACCDET (0x01 << 11)
/* 1,enable Internal connection between ACCDET and EINT */
#define RG_EINT_ANA_CONFIG (RG_ACCDET1SEL | RG_ACCDET2SEL | RG_EINTCONFIGACCDET)
/* Bit Define: clock, TOP_CKPDN_SET */
#define RG_ACCDET_CLK_SET (1 << 9)
#define RG_ACCDET_CLK_CLR (1 << 9)
/* Bit Define: INT_CON_ACCDET_SET */
#define RG_ACCDET_IRQ_SET (1 << 12)
#define RG_ACCDET_IRQ_CLR (1 << 12)
#define RG_ACCDET_IRQ_STATUS_CLR (1 << 12)
#define RG_ACCDET_EINT_IRQ_SET (1 << 13)
#define RG_ACCDET_EINT_IRQ_CLR (1 << 13)
#define RG_ACCDET_EINT_IRQ_STATUS_CLR (1 << 10)
/* Bit Define: INT_STATUS_ACCDET(0x02E0) */
#define RG_INT_STATUS_ACCDET (0x01 << 12)
#define RG_INT_STATUS_ACCDET_EINT (0x01 << 13)
/* Bit Define: ACCDET_EINT_CTL */
#define EINT_IRQ_DE_OUT (0x00 << 4) /* 1 msec */
#define EINT_IRQ_DE_IN (0x06 << 4)  /* 256 msec */
#define EINT_PLUG_DEB_CLR (~(0x07 << 4))
#define EINT_PWM_THRESH 0x400
#endif /* end CONFIG */

/*Register value define*/
/* Bit Define: TOP_RST_ACCDET_SET */
#define ACCDET_RESET_SET (1 << 4)
#define ACCDET_RESET_CLR (1 << 4)
/* Bit Define: ACCDET_AUXADC_AUTO_SPL */
#define ACCDET_AUXADC_AUTO_SET (1 << 0)
/* Bit Define: ACCDET_AUXADC_REG */
#define ACCDET_DATA_READY (1 << 15)
#define ACCDET_DATA_MASK (0x0FFF)
/* Bit Define: ACCDET_AUXADC_CTL_SET */
#define ACCDET_CH_REQ_EN (1 << 5)
/* Bit Define: ACCDET_RSV */
#define ACCDET_INPUT_MICP (1 << 3)
/* Bit Define: ACCDET_CTRL */
#define ACCDET_ENABLE (1 << 0)
#define ACCDET_DISABLE (0 << 0)
#define ACCDET_EINT_EN (1 << 2)
#define ACCDET_CTRL_EN (1 << 0) /* Discard */
/* Bit Define: ACCDET_STATE_SWCTRL */
#define ACCDET_EINT_PWM_IDLE (1 << 7)
#define ACCDET_MIC_PWM_IDLE (1 << 6)
#define ACCDET_VTH_PWM_IDLE (1 << 5)
#define ACCDET_CMP_PWM_IDLE (1 << 4)
#define ACCDET_EINT_PWM_EN (1 << 3)
#define ACCDET_CMP_EN (1 << 0)
#define ACCDET_VTH_EN (1 << 1)
#define ACCDET_MICBIA_EN (1 << 2)
#define ACCDET_SWCTRL_EN 0x07
/* Bit Define: ACCDET_IRQ_STS */
#define IRQ_CLR_BIT 0x100
#define IRQ_EINT_CLR_BIT 0x400
#define IRQ_STATUS_BIT (1 << 0)
#define EINT_IRQ_STATUS_BIT (1 << 2)
#define EINT_IRQ_POL_HIGH (1 << 15)
#define EINT_IRQ_POL_LOW (1 << 15)
/* Bit Define: ACCDET_DEBOUNCE4 */
#define ACCDET_DE4 0x42 /* 2ms */
/* efuse offset */
#define RG_OTP_PA_ADDR_WORD_INDEX (0x03)
#define RG_OTP_PA_ACCDET_BIT_SHIFT (0x05)
#define RG_ACCDET_EINT_HIGH (1 << 15)		/* Discard */
#define RG_ACCDET_EINT_LOW (0 << 15)		/* Discard */
#define ACCDET_POWER_MOD (1 << 13)		/* Discard */
#define ACCDET_MIC1_ON (1 << 7)			/* Discard */
#define ACCDET_BF_ON (1 << 9)			/* Discard */
#define ACCDET_BF_OFF (0 << 9)			/* Discard */
#define ACCDET_BF_MOD (1 << 11)			/* Discard */
#define ACCDET_PWM_EN_SW (1 << 15)		/* Discard */
#define ACCDET_MIC_EN_SW (1 << 14)		/* Discard */
#define ACCDET_VTH_EN_SW (1 << 13)		/* Discard */
#define ACCDET_CMP_EN_SW (1 << 12)		/* Discard */
#define ACCDET_IN_SW 0x10			/* Discard */
#define NEGV_IRQ_STATUS_BIT (1 << 1)		/* Discard */
#define IRQ_NEGV_CLR_BIT 0x200			/* Discard */
#define ACCDET_NEGV_EN (1 << 4)			/* Discard */
#define RG_ACCDET_NEGV_IRQ_SET (1 << 14)	/* Discard */
#define RG_ACCDET_NEGV_IRQ_CLR (1 << 14)	/* Discard */
#define RG_ACCDET_NEGV_IRQ_STATUS_CLR (1 << 14) /* Discard */
/*
 * #define ACCDET_PWM_SEL_CMP       0x00
 * #define ACCDET_PWM_SEL_VTH       0x01
 * #define ACCDET_PWM_SEL_MIC       0x10
 * #define ACCDET_PWM_SEL_SW        0x11
 *
 * #define ACCDET_TEST_MODE5_ACCDET_IN_GPI        (1<<5)
 * #define ACCDET_TEST_MODE4_ACCDET_IN_SW        (1<<4)
 * #define ACCDET_TEST_MODE3_MIC_SW        (1<<3)
 * #define ACCDET_TEST_MODE2_VTH_SW        (1<<2)
 * #define ACCDET_TEST_MODE1_CMP_SW        (1<<1)
 * #define ACCDET_TEST_MODE0_GPI        (1<<0)
 * //#define ACCDET_DEFVAL_SEL        (1<<15)
 */
/* power mode and auxadc switch on/off */
#define ACCDET_1V9_MODE_OFF 0x1A10
#define ACCDET_2V8_MODE_OFF 0x5A10
#define ACCDET_1V9_MODE_ON 0x1E10
#define ACCDET_2V8_MODE_ON 0x5A20
#define ACCDET_SWCTRL_IDLE_EN (0x07 << 4)
#endif /* end _REG_ACCDET_H_ */
