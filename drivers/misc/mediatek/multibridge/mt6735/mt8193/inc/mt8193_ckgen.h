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

#ifndef MT8193_CKGEN_H
#define MT8193_CKGEN_H

#define MT8193_CKGEN_VFY 1

#define MT8193_DISABLE_DCXO 0


#define CKGEN_IOW(num, dtype)     _IOW('H', num, dtype)
#define CKGEN_IOR(num, dtype)     _IOR('H', num, dtype)
#define CKGEN_IOWR(num, dtype)    _IOWR('H', num, dtype)
#define CKGEN_IO(num)             _IO('H', num)

#define MTK_MT8193_CKGEN_1      CKGEN_IO(1)
#define MTK_MT8193_CKGEN_2          CKGEN_IO(2)
#define MTK_MT8193_CKGEN_LS_TEST    CKGEN_IO(3)     /* level shift test */
#define MTK_MT8193_CKGEN_SPM_CTRL   CKGEN_IO(4)     /* spm ctrl test */
#define MTK_MT8193_CKGEN_FREQ_METER   CKGEN_IO(5)     /* FREQ METER TEST*/
#define MTK_MT8193_GPIO_CTRL   CKGEN_IO(6)     /* GPIO CTRL*/
#define MTK_MT8193_EARLY_SUSPEND   CKGEN_IO(7)     /* early suspend*/
#define MTK_MT8193_LATE_RESUME   CKGEN_IO(8)     /* late resume*/

#define REG_RW_FMETER                       0x04c    /* freq meter register */
#define CKGEN_FMETER_RESET                        1
#define CKGEN_FMETER_START                        (1U<<1)
#define CKGEN_FMETER_DONE                         (1U<<2)

/* HDMI POWER OFF/ON CONTROL*/

#define REG_RW_HDMI_PWR_RST_B                0x100
#define CKGEN_HDMI_PWR_RST_EN                1

#define REG_RW_HDMI_PWR_CTRL                 0x104
#define CKGEN_HDMI_PWR_ISO_EN                1
#define CKGEN_HDMI_PWR_PWR_ON                (1U<<1)
#define CKGEN_HDMI_PWR_CLK_OFF               (1U<<2)

/* LVDS POWER OFF/ON CONTROL*/

#define REG_RW_LVDS_PWR_RST_B                0x108
#define CKGEN_LVDS_PWR_RST_EN                1

#define REG_RW_LVDS_PWR_CTRL                0x10c
#define CKGEN_LVDS_PWR_ISO_EN                1
#define CKGEN_LVDS_PWR_PWR_ON                (1U<<1)
#define CKGEN_LVDS_PWR_CLK_OFF               (1U<<2)

/* NFI POWER OFF/ON CONTROL*/

#define REG_RW_NFI_PWR_RST_B                0x110
#define CKGEN_NFI_PWR_RST_EN                1

#define REG_RW_NFI_PWR_CTRL                 0x114
#define CKGEN_NFI_PWR_ISO_EN                1
#define CKGEN_NFI_PWR_PWR_ON                (1U<<1)
#define CKGEN_NFI_PWR_CLK_OFF               (1U<<2)

#define REG_RO_PWR_ACT                      0x118
#define CKGEN_NFI_PWR_ON_ACT                1
#define CKGEN_LVDS_PWR_ON_ACT               (1U<<1)
#define CKGEN_HDMI_PWR_ON_ACT               (1U<<2)

#define REG_RW_LS_CTRL                      0x012c   /* level shift control */
#define LS_CTRL_GROUP0_SHIFT_HIGH              1     /* 0: 3.3 -> 1.8; 1: 1.8->3.3 */
#define LS_CTRL_GROUP1_SHIFT_HIGH              (1U<<1)     /* 0: 3.3 -> 1.8; 1: 1.8->3.3 */
#define LS_CTRL_GROUP2_SHIFT_HIGH              (1U<<2)     /* 0: 3.3 -> 1.8; 1: 1.8->3.3 */
#define LS_CTRL_GROUP3_SHIFT_HIGH              (1U<<3)     /* 0: 3.3 -> 1.8; 1: 1.8->3.3 */
#define LS_CTRL_GROUP4_SHIFT_HIGH              (1U<<4)     /* 0: 3.3 -> 1.8; 1: 1.8->3.3 */
#define LS_CTRL_GROUP5_SHIFT_HIGH              (1U<<5)     /* 0: 3.3 -> 1.8; 1: 1.8->3.3 */
#define LS_CTRL_GROUP6_SHIFT_HIGH              (1U<<6)     /* 0: 3.3 -> 1.8; 1: 1.8->3.3 */
#define LS_CTRL_SHIFT_HIGH_EN                  (1U<<30)    /* 1.8>3.3 enable */
#define LS_CTRL_SHIFT_LOW_EN                  (1U<<31)    /* 3.3>1.8 enable */

#define REG_RW_LVDS_ANACFG4                0x320
#define LVDS_ANACFG4_VPlLL_PD             (1U<<10)

#define REG_RW_HDMITX_ANACFG3              0x334
#define HDMITX_ANACFG3_BIT20             (1U<<20)
#define HDMITX_ANACFG3_BIT21             (1U<<21)

#define REG_RW_PLLGP_ANACFG0              0x34c
#define PLLGP_ANACFG0_PLL1_RESERVED             1
#define PLLGP_ANACFG0_PLL1_NFIPLL_EN      (1U<<1)
#define PLLGP_ANACFG0_PLL1_EN             (1U<<31)

#define REG_RW_PLLGP_ANACFG2              0x354
#define PLLGP_ANACFG2_PLLGP_BIAS_EN       (1U<<20)

#define REG_RW_DCXO_ANACFG9              0x388
#define DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT   9
#define DCXO_ANACFG9_BUS_CK_SOURCE_SEL_MASK    0x7
#define DCX0_ANACFG9_BUS_CK_CTRL_SEL           (1U<<7)
#define DCX0_ANACFG9_BUS_CK_AUTO_SWITCH_EN     (1U<<5)

/* DCXO */

#define REG_RW_DCXO_ANACFG2              0x308
#define DCXO_ANACFG2_LDO4_EN             (1U<<2)
#define DCXO_ANACFG2_LDO4_MAN_EN         (1U<<3)
#define DCXO_ANACFG2_LDO3_EN             (1U<<4)
#define DCXO_ANACFG2_LDO3_MAN_EN         (1U<<5)
#define DCXO_ANACFG2_LDO2_EN             (1U<<6)
#define DCXO_ANACFG2_LDO2_MAN_EN         (1U<<7)
#define DCXO_ANACFG2_LDO1_EN             (1U<<8)
#define DCXO_ANACFG2_LDO1_MAN_EN         (1U<<9)
#define DCXO_ANACFG2_PO_MAN              (1U<<29)

#define REG_RW_DCXO_ANACFG4              0x370
#define DCXO_ANACFG4_BT_MAN             (1U<<18)
#define DCXO_ANACFG4_EXT2_MAN           (1U<<19)
#define DCXO_ANACFG4_EXT1_MAN           (1U<<20)

#if MT8193_CKGEN_VFY
/* level shift test parameter */
struct mt8193_ckgen_ls_info_t {
	int i4GroupNum;
	int i4TurnLow;
};

/* freq meter parameter */
struct mt8193_ckgen_freq_meter_t {
	int u4Func;
};

/* GPIO CTRL parameter */
struct mt8193_gpio_ctrl_t {
	int u4GpioNum;
	int u4Mode;		/* 0 is input. 1 is output */
	int u4Value;	/* 1 is high. 0 is low. only valid in output mode */
};

#if 0

/* config gpio */
int mt8193_ckgen_gpio_config(int i4GpioNum, int i4Output, int i4High);

/* print gpio input value */
int mt8193_ckgen_gpio_input(int i4GpioNum);


/* test pad level shift */
int mt8193_ckgen_config_pad_level_shift(int i4GroupNum, int i4TurnLow);

/* read and print chip id */
void mt8193_ckgen_chipid(void);


/* measure clock with freq meter */
u32 mt8193_ckgen_measure_clk(u32 u4Func);

u32 mt8193_ckgen_reg_rw_test(u16 addr);
#endif

void mt8193_lvds_sys_spm_control(bool power_on);
void mt8193_hdmi_sys_spm_control(bool power_on);
void mt8193_nfi_sys_spm_control(bool power_on);
void mt8193_lvds_ana_pwr_control(bool power_on);
void mt8193_pllgp_ana_pwr_control(bool power_on);
void mt8193_bus_clk_switch(bool bus_26m_to_32k);
void mt8193_hdmi_ana_pwr_control(bool power_on);
void mt8193_nfi_ana_pwr_control(bool power_on);
int mt8193_ckgen_config_pad_level_shift(int i4GroupNum, int i4TurnLow);
void mt8193_spm_control_test(int u4Func);
u32 mt8193_ckgen_measure_clk(u32 u4Func);
void mt8193_ckgen_early_suspend(void);
void mt8193_ckgen_late_resume(void);
void mt8193_bus_clk_switch_to_26m(void);

#endif

#endif /* MT8193_CKGEN_H */
