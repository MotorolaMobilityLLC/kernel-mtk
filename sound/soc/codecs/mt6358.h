/*
 * mt6358.h  --  mt6358 ALSA SoC audio codec driver
 *
 * Copyright (c) 2017 MediaTek Inc.
 * Author: KaiChieh Chuang <kaichieh.chuang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MT6358_H__
#define __MT6358_H__

/* Reg bit define */
/* TOP_CLKSQ */
#define RG_CLKSQ_EN_AUD_SFT 0

/* DCXO_CW14 */
#define RG_XO_AUDIO_EN_M_SFT 13

/* AUDENC_ANA_CON0 */
#define RG_AUDPREAMPLON 0
#define RG_AUDPREAMPLDCCEN 1
#define RG_AUDPREAMPLDCPRECHARGE 2

#define RG_AUDADCLPWRUP 12

#define RG_AUDADCLINPUTSEL_SFT 13
#define RG_AUDADCLINPUTSEL_MASK 0x3

/* AUDENC_ANA_CON1 */
#define RG_AUDPREAMPRON 0
#define RG_AUDPREAMPRDCCEN 1
#define RG_AUDPREAMPRDCPRECHARGE 2

#define RG_AUDADCRPWRUP 12

#define RG_AUDADCRINPUTSEL_SFT 13
#define RG_AUDADCRINPUTSEL_MASK 0x3

/* AUDENC_ANA_CON10 */
#define RG_AUDPWDBMICBIAS0 0
#define RG_AUDMICBIAS0VREF 4
#define RG_AUDMICBIAS0LOWPEN 7

#define RG_AUDPWDBMICBIAS2 8
#define RG_AUDMICBIAS2LOWPEN 15

/* AUDENC_ANA_CON11 */
#define RG_AUDPWDBMICBIAS1 0
#define RG_AUDMICBIAS1DCSW1PEN 1
#define RG_AUDMICBIAS1DCSW1NEN 2
#define RG_AUDMICBIAS1VREF 4
#define RG_AUDMICBIAS1LOWPEN 7




/* AUD_TOP_CKPDN_CON0 */
#define RG_AUDNCP_CK_PDN_SFT                    6
#define RG_AUDNCP_CK_PDN_MASK                   0x1
#define RG_AUDNCP_CK_PDN_MASK_SFT               (0x1 << 6)
#define RG_ZCD13M_CK_PDN_SFT                    5
#define RG_ZCD13M_CK_PDN_MASK                   0x1
#define RG_ZCD13M_CK_PDN_MASK_SFT               (0x1 << 5)
#define RG_AUDIF_CK_PDN_SFT                     2
#define RG_AUDIF_CK_PDN_MASK                    0x1
#define RG_AUDIF_CK_PDN_MASK_SFT                (0x1 << 2)
#define RG_AUD_CK_PDN_SFT                       1
#define RG_AUD_CK_PDN_MASK                      0x1
#define RG_AUD_CK_PDN_MASK_SFT                  (0x1 << 1)
#define RG_ACCDET_CK_PDN_SFT                    0
#define RG_ACCDET_CK_PDN_MASK                   0x1
#define RG_ACCDET_CK_PDN_MASK_SFT               (0x1 << 0)

/* AFE_UL_DL_CON0 */
#define AFE_UL_LR_SWAP_SFT                               15
#define AFE_UL_LR_SWAP_MASK                              0x1
#define AFE_UL_LR_SWAP_MASK_SFT                          (0x1 << 15)
#define AFE_DL_LR_SWAP_SFT                               14
#define AFE_DL_LR_SWAP_MASK                              0x1
#define AFE_DL_LR_SWAP_MASK_SFT                          (0x1 << 14)
#define AFE_ON_SFT                                       0
#define AFE_ON_MASK                                      0x1
#define AFE_ON_MASK_SFT                                  (0x1 << 0)

/* AFE_DL_SRC2_CON0_L */
#define DL_2_SRC_ON_TMP_CTL_PRE_SFT                      0
#define DL_2_SRC_ON_TMP_CTL_PRE_MASK                     0x1
#define DL_2_SRC_ON_TMP_CTL_PRE_MASK_SFT                 (0x1 << 0)

/* AFE_UL_SRC_CON0_H */
#define C_DIGMIC_PHASE_SEL_CH1_CTL_SFT                   11
#define C_DIGMIC_PHASE_SEL_CH1_CTL_MASK                  0x7
#define C_DIGMIC_PHASE_SEL_CH1_CTL_MASK_SFT              (0x7 << 11)
#define C_DIGMIC_PHASE_SEL_CH2_CTL_SFT                   8
#define C_DIGMIC_PHASE_SEL_CH2_CTL_MASK                  0x7
#define C_DIGMIC_PHASE_SEL_CH2_CTL_MASK_SFT              (0x7 << 8)
#define C_TWO_DIGITAL_MIC_CTL_SFT                        7
#define C_TWO_DIGITAL_MIC_CTL_MASK                       0x1
#define C_TWO_DIGITAL_MIC_CTL_MASK_SFT                   (0x1 << 7)

/* AFE_UL_SRC_CON0_L */
#define DMIC_LOW_POWER_MODE_CTL_SFT                      14
#define DMIC_LOW_POWER_MODE_CTL_MASK                     0x3
#define DMIC_LOW_POWER_MODE_CTL_MASK_SFT                 (0x3 << 14)
#define DIGMIC_3P25M_1P625M_SEL_CTL_SFT                  5
#define DIGMIC_3P25M_1P625M_SEL_CTL_MASK                 0x1
#define DIGMIC_3P25M_1P625M_SEL_CTL_MASK_SFT             (0x1 << 5)
#define UL_LOOP_BACK_MODE_CTL_SFT                        2
#define UL_LOOP_BACK_MODE_CTL_MASK                       0x1
#define UL_LOOP_BACK_MODE_CTL_MASK_SFT                   (0x1 << 2)
#define UL_SDM_3_LEVEL_CTL_SFT                           1
#define UL_SDM_3_LEVEL_CTL_MASK                          0x1
#define UL_SDM_3_LEVEL_CTL_MASK_SFT                      (0x1 << 1)
#define UL_SRC_ON_TMP_CTL_SFT                            0
#define UL_SRC_ON_TMP_CTL_MASK                           0x1
#define UL_SRC_ON_TMP_CTL_MASK_SFT                       (0x1 << 0)

/* AFE_TOP_CON0 */
#define UL_SINE_ON_SFT                                   1
#define UL_SINE_ON_MASK                                  0x1
#define UL_SINE_ON_MASK_SFT                              (0x1 << 1)
#define DL_SINE_ON_SFT                                   0
#define DL_SINE_ON_MASK                                  0x1
#define DL_SINE_ON_MASK_SFT                              (0x1 << 0)

/* AUDIO_TOP_CON0 */
#define FIFO_AUTO_RST_CTL_SFT                            15
#define FIFO_AUTO_RST_CTL_MASK                           0x1
#define FIFO_AUTO_RST_CTL_MASK_SFT                       (0x1 << 15)
#define AFE_CK_DIV_RST_CTL_SFT                           14
#define AFE_CK_DIV_RST_CTL_MASK                          0x1
#define AFE_CK_DIV_RST_CTL_MASK_SFT                      (0x1 << 14)
#define PDN_ADC_HIRES_CTL_SFT                            13
#define PDN_ADC_HIRES_CTL_MASK                           0x1
#define PDN_ADC_HIRES_CTL_MASK_SFT                       (0x1 << 13)
#define AFE_13M_PHASE_SEL_CTL_SFT                        12
#define AFE_13M_PHASE_SEL_CTL_MASK                       0x1
#define AFE_13M_PHASE_SEL_CTL_MASK_SFT                   (0x1 << 12)
#define AFE_6P5M_PHASE_SEL_CTL_SFT                       10
#define AFE_6P5M_PHASE_SEL_CTL_MASK                      0x3
#define AFE_6P5M_PHASE_SEL_CTL_MASK_SFT                  (0x3 << 10)
#define AFE_3P25M_PHASE_SEL_CTL_SFT                      8
#define AFE_3P25M_PHASE_SEL_CTL_MASK                     0x3
#define AFE_3P25M_PHASE_SEL_CTL_MASK_SFT                 (0x3 << 8)
#define PDN_AFE_CTL_SFT                                  7
#define PDN_AFE_CTL_MASK                                 0x1
#define PDN_AFE_CTL_MASK_SFT                             (0x1 << 7)
#define PDN_DAC_CTL_SFT                                  6
#define PDN_DAC_CTL_MASK                                 0x1
#define PDN_DAC_CTL_MASK_SFT                             (0x1 << 6)
#define PDN_ADC_CTL_SFT                                  5
#define PDN_ADC_CTL_MASK                                 0x1
#define PDN_ADC_CTL_MASK_SFT                             (0x1 << 5)
#define PDN_BUCKOSC_CTL_SFT                              4
#define PDN_BUCKOSC_CTL_MASK                             0x1
#define PDN_BUCKOSC_CTL_MASK_SFT                         (0x1 << 4)
#define PDN_I2S_DL_CTL_SFT                               3
#define PDN_I2S_DL_CTL_MASK                              0x1
#define PDN_I2S_DL_CTL_MASK_SFT                          (0x1 << 3)
#define PWR_CLK_DIS_CTL_SFT                              2
#define PWR_CLK_DIS_CTL_MASK                             0x1
#define PWR_CLK_DIS_CTL_MASK_SFT                         (0x1 << 2)
#define PDN_AFE_TESTMODEL_CTL_SFT                        1
#define PDN_AFE_TESTMODEL_CTL_MASK                       0x1
#define PDN_AFE_TESTMODEL_CTL_MASK_SFT                   (0x1 << 1)
#define PDN_RESERVED_SFT                                 0
#define PDN_RESERVED_MASK                                0x1
#define PDN_RESERVED_MASK_SFT                            (0x1 << 0)

/* AFE_MON_DEBUG0 */
#define AUDIO_SYS_TOP_MON_SWAP_SFT                       14
#define AUDIO_SYS_TOP_MON_SWAP_MASK                      0x3
#define AUDIO_SYS_TOP_MON_SWAP_MASK_SFT                  (0x3 << 14)
#define AUDIO_SYS_TOP_MON_SEL_SFT                        8
#define AUDIO_SYS_TOP_MON_SEL_MASK                       0x1f
#define AUDIO_SYS_TOP_MON_SEL_MASK_SFT                   (0x1f << 8)
#define AFE_MON_SEL_SFT                                  0
#define AFE_MON_SEL_MASK                                 0xff
#define AFE_MON_SEL_MASK_SFT                             (0xff << 0)

/* AFUNC_AUD_CON0 */
#define CCI_AUD_ANACK_SEL_SFT                            15
#define CCI_AUD_ANACK_SEL_MASK                           0x1
#define CCI_AUD_ANACK_SEL_MASK_SFT                       (0x1 << 15)
#define CCI_AUDIO_FIFO_WPTR_SFT                          12
#define CCI_AUDIO_FIFO_WPTR_MASK                         0x7
#define CCI_AUDIO_FIFO_WPTR_MASK_SFT                     (0x7 << 12)
#define CCI_SCRAMBLER_CG_EN_SFT                          11
#define CCI_SCRAMBLER_CG_EN_MASK                         0x1
#define CCI_SCRAMBLER_CG_EN_MASK_SFT                     (0x1 << 11)
#define CCI_LCH_INV_SFT                                  10
#define CCI_LCH_INV_MASK                                 0x1
#define CCI_LCH_INV_MASK_SFT                             (0x1 << 10)
#define CCI_RAND_EN_SFT                                  9
#define CCI_RAND_EN_MASK                                 0x1
#define CCI_RAND_EN_MASK_SFT                             (0x1 << 9)
#define CCI_SPLT_SCRMB_CLK_ON_SFT                        8
#define CCI_SPLT_SCRMB_CLK_ON_MASK                       0x1
#define CCI_SPLT_SCRMB_CLK_ON_MASK_SFT                   (0x1 << 8)
#define CCI_SPLT_SCRMB_ON_SFT                            7
#define CCI_SPLT_SCRMB_ON_MASK                           0x1
#define CCI_SPLT_SCRMB_ON_MASK_SFT                       (0x1 << 7)
#define CCI_AUD_IDAC_TEST_EN_SFT                         6
#define CCI_AUD_IDAC_TEST_EN_MASK                        0x1
#define CCI_AUD_IDAC_TEST_EN_MASK_SFT                    (0x1 << 6)
#define CCI_ZERO_PAD_DISABLE_SFT                         5
#define CCI_ZERO_PAD_DISABLE_MASK                        0x1
#define CCI_ZERO_PAD_DISABLE_MASK_SFT                    (0x1 << 5)
#define CCI_AUD_SPLIT_TEST_EN_SFT                        4
#define CCI_AUD_SPLIT_TEST_EN_MASK                       0x1
#define CCI_AUD_SPLIT_TEST_EN_MASK_SFT                   (0x1 << 4)
#define CCI_AUD_SDM_MUTEL_SFT                            3
#define CCI_AUD_SDM_MUTEL_MASK                           0x1
#define CCI_AUD_SDM_MUTEL_MASK_SFT                       (0x1 << 3)
#define CCI_AUD_SDM_MUTER_SFT                            2
#define CCI_AUD_SDM_MUTER_MASK                           0x1
#define CCI_AUD_SDM_MUTER_MASK_SFT                       (0x1 << 2)
#define CCI_AUD_SDM_7BIT_SEL_SFT                         1
#define CCI_AUD_SDM_7BIT_SEL_MASK                        0x1
#define CCI_AUD_SDM_7BIT_SEL_MASK_SFT                    (0x1 << 1)
#define CCI_SCRAMBLER_EN_SFT                             0
#define CCI_SCRAMBLER_EN_MASK                            0x1
#define CCI_SCRAMBLER_EN_MASK_SFT                        (0x1 << 0)

/* AFUNC_AUD_CON1 */
#define AUD_SDM_TEST_L_SFT                               8
#define AUD_SDM_TEST_L_MASK                              0xff
#define AUD_SDM_TEST_L_MASK_SFT                          (0xff << 8)
#define AUD_SDM_TEST_R_SFT                               0
#define AUD_SDM_TEST_R_MASK                              0xff
#define AUD_SDM_TEST_R_MASK_SFT                          (0xff << 0)

/* AFUNC_AUD_CON2 */
#define CCI_AUD_DAC_ANA_MUTE_SFT                         7
#define CCI_AUD_DAC_ANA_MUTE_MASK                        0x1
#define CCI_AUD_DAC_ANA_MUTE_MASK_SFT                    (0x1 << 7)
#define CCI_AUD_DAC_ANA_RSTB_SEL_SFT                     6
#define CCI_AUD_DAC_ANA_RSTB_SEL_MASK                    0x1
#define CCI_AUD_DAC_ANA_RSTB_SEL_MASK_SFT                (0x1 << 6)
#define CCI_AUDIO_FIFO_CLKIN_INV_SFT                     4
#define CCI_AUDIO_FIFO_CLKIN_INV_MASK                    0x1
#define CCI_AUDIO_FIFO_CLKIN_INV_MASK_SFT                (0x1 << 4)
#define CCI_AUDIO_FIFO_ENABLE_SFT                        3
#define CCI_AUDIO_FIFO_ENABLE_MASK                       0x1
#define CCI_AUDIO_FIFO_ENABLE_MASK_SFT                   (0x1 << 3)
#define CCI_ACD_MODE_SFT                                 2
#define CCI_ACD_MODE_MASK                                0x1
#define CCI_ACD_MODE_MASK_SFT                            (0x1 << 2)
#define CCI_AFIFO_CLK_PWDB_SFT                           1
#define CCI_AFIFO_CLK_PWDB_MASK                          0x1
#define CCI_AFIFO_CLK_PWDB_MASK_SFT                      (0x1 << 1)
#define CCI_ACD_FUNC_RSTB_SFT                            0
#define CCI_ACD_FUNC_RSTB_MASK                           0x1
#define CCI_ACD_FUNC_RSTB_MASK_SFT                       (0x1 << 0)

/* AFUNC_AUD_CON3 */
#define SDM_ANA13M_TESTCK_SEL_SFT                        15
#define SDM_ANA13M_TESTCK_SEL_MASK                       0x1
#define SDM_ANA13M_TESTCK_SEL_MASK_SFT                   (0x1 << 15)
#define SDM_ANA13M_TESTCK_SRC_SEL_SFT                    12
#define SDM_ANA13M_TESTCK_SRC_SEL_MASK                   0x7
#define SDM_ANA13M_TESTCK_SRC_SEL_MASK_SFT               (0x7 << 12)
#define SDM_TESTCK_SRC_SEL_SFT                           8
#define SDM_TESTCK_SRC_SEL_MASK                          0x7
#define SDM_TESTCK_SRC_SEL_MASK_SFT                      (0x7 << 8)
#define DIGMIC_TESTCK_SRC_SEL_SFT                        4
#define DIGMIC_TESTCK_SRC_SEL_MASK                       0x7
#define DIGMIC_TESTCK_SRC_SEL_MASK_SFT                   (0x7 << 4)
#define DIGMIC_TESTCK_SEL_SFT                            0
#define DIGMIC_TESTCK_SEL_MASK                           0x1
#define DIGMIC_TESTCK_SEL_MASK_SFT                       (0x1 << 0)

/* AFUNC_AUD_CON4 */
#define UL_FIFO_WCLK_INV_SFT                             8
#define UL_FIFO_WCLK_INV_MASK                            0x1
#define UL_FIFO_WCLK_INV_MASK_SFT                        (0x1 << 8)
#define UL_FIFO_DIGMIC_WDATA_TESTSRC_SEL_SFT             6
#define UL_FIFO_DIGMIC_WDATA_TESTSRC_SEL_MASK            0x1
#define UL_FIFO_DIGMIC_WDATA_TESTSRC_SEL_MASK_SFT        (0x1 << 6)
#define UL_FIFO_WDATA_TESTEN_SFT                         5
#define UL_FIFO_WDATA_TESTEN_MASK                        0x1
#define UL_FIFO_WDATA_TESTEN_MASK_SFT                    (0x1 << 5)
#define UL_FIFO_WDATA_TESTSRC_SEL_SFT                    4
#define UL_FIFO_WDATA_TESTSRC_SEL_MASK                   0x1
#define UL_FIFO_WDATA_TESTSRC_SEL_MASK_SFT               (0x1 << 4)
#define UL_FIFO_WCLK_6P5M_TESTCK_SEL_SFT                 3
#define UL_FIFO_WCLK_6P5M_TESTCK_SEL_MASK                0x1
#define UL_FIFO_WCLK_6P5M_TESTCK_SEL_MASK_SFT            (0x1 << 3)
#define UL_FIFO_WCLK_6P5M_TESTCK_SRC_SEL_SFT             0
#define UL_FIFO_WCLK_6P5M_TESTCK_SRC_SEL_MASK            0x7
#define UL_FIFO_WCLK_6P5M_TESTCK_SRC_SEL_MASK_SFT        (0x7 << 0)

/* AFE_SGEN_CFG0 */
#define C_AMP_DIV_CH1_CTL_SFT                            12
#define C_AMP_DIV_CH1_CTL_MASK                           0xf
#define C_AMP_DIV_CH1_CTL_MASK_SFT                       (0xf << 12)
#define C_DAC_EN_CTL_SFT                                 7
#define C_DAC_EN_CTL_MASK                                0x1
#define C_DAC_EN_CTL_MASK_SFT                            (0x1 << 7)
#define C_MUTE_SW_CTL_SFT                                6
#define C_MUTE_SW_CTL_MASK                               0x1
#define C_MUTE_SW_CTL_MASK_SFT                           (0x1 << 6)

/* AUDDEC_ANA_CON0 */
#define RG_AUDDACLPWRUP_VAUDP15_SFT                  0
#define RG_AUDDACLPWRUP_VAUDP15_MASK                 0x1
#define RG_AUDDACLPWRUP_VAUDP15_MASK_SFT             (0x1 << 0)
#define RG_AUDDACRPWRUP_VAUDP15_SFT                  1
#define RG_AUDDACRPWRUP_VAUDP15_MASK                 0x1
#define RG_AUDDACRPWRUP_VAUDP15_MASK_SFT             (0x1 << 1)
#define RG_AUD_DAC_PWR_UP_VA28_SFT                   2
#define RG_AUD_DAC_PWR_UP_VA28_MASK                  0x1
#define RG_AUD_DAC_PWR_UP_VA28_MASK_SFT              (0x1 << 2)
#define RG_AUD_DAC_PWL_UP_VA28_SFT                   3
#define RG_AUD_DAC_PWL_UP_VA28_MASK                  0x1
#define RG_AUD_DAC_PWL_UP_VA28_MASK_SFT              (0x1 << 3)
#define RG_AUDHPLPWRUP_VAUDP15_SFT                   4
#define RG_AUDHPLPWRUP_VAUDP15_MASK                  0x1
#define RG_AUDHPLPWRUP_VAUDP15_MASK_SFT              (0x1 << 4)
#define RG_AUDHPRPWRUP_VAUDP15_SFT                   5
#define RG_AUDHPRPWRUP_VAUDP15_MASK                  0x1
#define RG_AUDHPRPWRUP_VAUDP15_MASK_SFT              (0x1 << 5)
#define RG_AUDHPLPWRUP_IBIAS_VAUDP15_SFT             6
#define RG_AUDHPLPWRUP_IBIAS_VAUDP15_MASK            0x1
#define RG_AUDHPLPWRUP_IBIAS_VAUDP15_MASK_SFT        (0x1 << 6)
#define RG_AUDHPRPWRUP_IBIAS_VAUDP15_SFT             7
#define RG_AUDHPRPWRUP_IBIAS_VAUDP15_MASK            0x1
#define RG_AUDHPRPWRUP_IBIAS_VAUDP15_MASK_SFT        (0x1 << 7)
#define RG_AUDHPLMUXINPUTSEL_VAUDP15_SFT             8
#define RG_AUDHPLMUXINPUTSEL_VAUDP15_MASK            0x3
#define RG_AUDHPLMUXINPUTSEL_VAUDP15_MASK_SFT        (0x3 << 8)
#define RG_AUDHPRMUXINPUTSEL_VAUDP15_SFT             10
#define RG_AUDHPRMUXINPUTSEL_VAUDP15_MASK            0x3
#define RG_AUDHPRMUXINPUTSEL_VAUDP15_MASK_SFT        (0x3 << 10)
#define RG_AUDHPLSCDISABLE_VAUDP15_SFT               12
#define RG_AUDHPLSCDISABLE_VAUDP15_MASK              0x1
#define RG_AUDHPLSCDISABLE_VAUDP15_MASK_SFT          (0x1 << 12)
#define RG_AUDHPRSCDISABLE_VAUDP15_SFT               13
#define RG_AUDHPRSCDISABLE_VAUDP15_MASK              0x1
#define RG_AUDHPRSCDISABLE_VAUDP15_MASK_SFT          (0x1 << 13)
#define RG_AUDHPLBSCCURRENT_VAUDP15_SFT              14
#define RG_AUDHPLBSCCURRENT_VAUDP15_MASK             0x1
#define RG_AUDHPLBSCCURRENT_VAUDP15_MASK_SFT         (0x1 << 14)
#define RG_AUDHPRBSCCURRENT_VAUDP15_SFT              15
#define RG_AUDHPRBSCCURRENT_VAUDP15_MASK             0x1
#define RG_AUDHPRBSCCURRENT_VAUDP15_MASK_SFT         (0x1 << 15)

/* AUDDEC_ANA_CON1 */
#define RG_AUDHPLOUTPWRUP_VAUDP15_SFT                0
#define RG_AUDHPLOUTPWRUP_VAUDP15_MASK               0x1
#define RG_AUDHPLOUTPWRUP_VAUDP15_MASK_SFT           (0x1 << 0)
#define RG_AUDHPROUTPWRUP_VAUDP15_SFT                1
#define RG_AUDHPROUTPWRUP_VAUDP15_MASK               0x1
#define RG_AUDHPROUTPWRUP_VAUDP15_MASK_SFT           (0x1 << 1)
#define RG_AUDHPLOUTAUXPWRUP_VAUDP15_SFT             2
#define RG_AUDHPLOUTAUXPWRUP_VAUDP15_MASK            0x1
#define RG_AUDHPLOUTAUXPWRUP_VAUDP15_MASK_SFT        (0x1 << 2)
#define RG_AUDHPROUTAUXPWRUP_VAUDP15_SFT             3
#define RG_AUDHPROUTAUXPWRUP_VAUDP15_MASK            0x1
#define RG_AUDHPROUTAUXPWRUP_VAUDP15_MASK_SFT        (0x1 << 3)
#define RG_HPLAUXFBRSW_EN_VAUDP15_SFT                4
#define RG_HPLAUXFBRSW_EN_VAUDP15_MASK               0x1
#define RG_HPLAUXFBRSW_EN_VAUDP15_MASK_SFT           (0x1 << 4)
#define RG_HPRAUXFBRSW_EN_VAUDP15_SFT                5
#define RG_HPRAUXFBRSW_EN_VAUDP15_MASK               0x1
#define RG_HPRAUXFBRSW_EN_VAUDP15_MASK_SFT           (0x1 << 5)
#define RG_HPLSHORT2HPLAUX_EN_VAUDP15_SFT            6
#define RG_HPLSHORT2HPLAUX_EN_VAUDP15_MASK           0x1
#define RG_HPLSHORT2HPLAUX_EN_VAUDP15_MASK_SFT       (0x1 << 6)
#define RG_HPRSHORT2HPRAUX_EN_VAUDP15_SFT            7
#define RG_HPRSHORT2HPRAUX_EN_VAUDP15_MASK           0x1
#define RG_HPRSHORT2HPRAUX_EN_VAUDP15_MASK_SFT       (0x1 << 7)
#define RG_HPLOUTSTGCTRL_VAUDP15_SFT                 8
#define RG_HPLOUTSTGCTRL_VAUDP15_MASK                0x7
#define RG_HPLOUTSTGCTRL_VAUDP15_MASK_SFT            (0x7 << 8)
#define RG_HPROUTSTGCTRL_VAUDP15_SFT                 11
#define RG_HPROUTSTGCTRL_VAUDP15_MASK                0x7
#define RG_HPROUTSTGCTRL_VAUDP15_MASK_SFT            (0x7 << 11)

/* AUDDEC_ANA_CON2 */
#define RG_HPLOUTPUTSTBENH_VAUDP15_SFT               0
#define RG_HPLOUTPUTSTBENH_VAUDP15_MASK              0x7
#define RG_HPLOUTPUTSTBENH_VAUDP15_MASK_SFT          (0x7 << 0)
#define RG_HPROUTPUTSTBENH_VAUDP15_SFT               4
#define RG_HPROUTPUTSTBENH_VAUDP15_MASK              0x7
#define RG_HPROUTPUTSTBENH_VAUDP15_MASK_SFT          (0x7 << 4)
#define RG_AUDHPSTARTUP_VAUDP15_SFT                  13
#define RG_AUDHPSTARTUP_VAUDP15_MASK                 0x1
#define RG_AUDHPSTARTUP_VAUDP15_MASK_SFT             (0x1 << 13)
#define RG_AUDREFN_DERES_EN_VAUDP15_SFT              14
#define RG_AUDREFN_DERES_EN_VAUDP15_MASK             0x1
#define RG_AUDREFN_DERES_EN_VAUDP15_MASK_SFT         (0x1 << 14)
#define RG_HPPSHORT2VCM_VAUDP15_SFT                  15
#define RG_HPPSHORT2VCM_VAUDP15_MASK                 0x1
#define RG_HPPSHORT2VCM_VAUDP15_MASK_SFT             (0x1 << 15)

/* AUDDEC_ANA_CON3 */
#define RG_HPINPUTSTBENH_VAUDP15_SFT                 13
#define RG_HPINPUTSTBENH_VAUDP15_MASK                0x1
#define RG_HPINPUTSTBENH_VAUDP15_MASK_SFT            (0x1 << 13)
#define RG_HPINPUTRESET0_VAUDP15_SFT                 14
#define RG_HPINPUTRESET0_VAUDP15_MASK                0x1
#define RG_HPINPUTRESET0_VAUDP15_MASK_SFT            (0x1 << 14)
#define RG_HPOUTPUTRESET0_VAUDP15_SFT                15
#define RG_HPOUTPUTRESET0_VAUDP15_MASK               0x1
#define RG_HPOUTPUTRESET0_VAUDP15_MASK_SFT           (0x1 << 15)

/* AUDDEC_ANA_CON4 */
#define RG_AUDHPDIFFINPBIASADJ_VAUDP15_SFT           0
#define RG_AUDHPDIFFINPBIASADJ_VAUDP15_MASK          0x7
#define RG_AUDHPDIFFINPBIASADJ_VAUDP15_MASK_SFT      (0x7 << 0)
#define RG_AUDHPLFCOMPRESSEL_VAUDP15_SFT             4
#define RG_AUDHPLFCOMPRESSEL_VAUDP15_MASK            0x7
#define RG_AUDHPLFCOMPRESSEL_VAUDP15_MASK_SFT        (0x7 << 4)
#define RG_AUDHPHFCOMPRESSEL_VAUDP15_SFT             8
#define RG_AUDHPHFCOMPRESSEL_VAUDP15_MASK            0x7
#define RG_AUDHPHFCOMPRESSEL_VAUDP15_MASK_SFT        (0x7 << 8)
#define RG_AUDHPHFCOMPBUFGAINSEL_VAUDP15_SFT         12
#define RG_AUDHPHFCOMPBUFGAINSEL_VAUDP15_MASK        0x3
#define RG_AUDHPHFCOMPBUFGAINSEL_VAUDP15_MASK_SFT    (0x3 << 12)
#define RG_AUDHPCOMP_EN_VAUDP15_SFT                  15
#define RG_AUDHPCOMP_EN_VAUDP15_MASK                 0x1
#define RG_AUDHPCOMP_EN_VAUDP15_MASK_SFT             (0x1 << 15)

/* AUDDEC_ANA_CON5 */
#define RG_AUDHPDECMGAINADJ_VAUDP15_SFT              0
#define RG_AUDHPDECMGAINADJ_VAUDP15_MASK             0x7
#define RG_AUDHPDECMGAINADJ_VAUDP15_MASK_SFT         (0x7 << 0)
#define RG_AUDHPDEDMGAINADJ_VAUDP15_SFT              4
#define RG_AUDHPDEDMGAINADJ_VAUDP15_MASK             0x7
#define RG_AUDHPDEDMGAINADJ_VAUDP15_MASK_SFT         (0x7 << 4)

/* AUDDEC_ANA_CON6 */
#define RG_AUDHSPWRUP_VAUDP15_SFT                    0
#define RG_AUDHSPWRUP_VAUDP15_MASK                   0x1
#define RG_AUDHSPWRUP_VAUDP15_MASK_SFT               (0x1 << 0)
#define RG_AUDHSPWRUP_IBIAS_VAUDP15_SFT              1
#define RG_AUDHSPWRUP_IBIAS_VAUDP15_MASK             0x1
#define RG_AUDHSPWRUP_IBIAS_VAUDP15_MASK_SFT         (0x1 << 1)
#define RG_AUDHSMUXINPUTSEL_VAUDP15_SFT              2
#define RG_AUDHSMUXINPUTSEL_VAUDP15_MASK             0x3
#define RG_AUDHSMUXINPUTSEL_VAUDP15_MASK_SFT         (0x3 << 2)
#define RG_AUDHSSCDISABLE_VAUDP15_SFT                4
#define RG_AUDHSSCDISABLE_VAUDP15_MASK               0x1
#define RG_AUDHSSCDISABLE_VAUDP15_MASK_SFT           (0x1 << 4)
#define RG_AUDHSBSCCURRENT_VAUDP15_SFT               5
#define RG_AUDHSBSCCURRENT_VAUDP15_MASK              0x1
#define RG_AUDHSBSCCURRENT_VAUDP15_MASK_SFT          (0x1 << 5)
#define RG_AUDHSSTARTUP_VAUDP15_SFT                  6
#define RG_AUDHSSTARTUP_VAUDP15_MASK                 0x1
#define RG_AUDHSSTARTUP_VAUDP15_MASK_SFT             (0x1 << 6)
#define RG_HSOUTPUTSTBENH_VAUDP15_SFT                7
#define RG_HSOUTPUTSTBENH_VAUDP15_MASK               0x1
#define RG_HSOUTPUTSTBENH_VAUDP15_MASK_SFT           (0x1 << 7)
#define RG_HSINPUTSTBENH_VAUDP15_SFT                 8
#define RG_HSINPUTSTBENH_VAUDP15_MASK                0x1
#define RG_HSINPUTSTBENH_VAUDP15_MASK_SFT            (0x1 << 8)
#define RG_HSINPUTRESET0_VAUDP15_SFT                 9
#define RG_HSINPUTRESET0_VAUDP15_MASK                0x1
#define RG_HSINPUTRESET0_VAUDP15_MASK_SFT            (0x1 << 9)
#define RG_HSOUTPUTRESET0_VAUDP15_SFT                10
#define RG_HSOUTPUTRESET0_VAUDP15_MASK               0x1
#define RG_HSOUTPUTRESET0_VAUDP15_MASK_SFT           (0x1 << 10)
#define RG_HSOUT_SHORTVCM_VAUDP15_SFT                11
#define RG_HSOUT_SHORTVCM_VAUDP15_MASK               0x1
#define RG_HSOUT_SHORTVCM_VAUDP15_MASK_SFT           (0x1 << 11)

/* AUDDEC_ANA_CON7 */
#define RG_AUDLOLPWRUP_VAUDP15_SFT                   0
#define RG_AUDLOLPWRUP_VAUDP15_MASK                  0x1
#define RG_AUDLOLPWRUP_VAUDP15_MASK_SFT              (0x1 << 0)
#define RG_AUDLOLPWRUP_IBIAS_VAUDP15_SFT             1
#define RG_AUDLOLPWRUP_IBIAS_VAUDP15_MASK            0x1
#define RG_AUDLOLPWRUP_IBIAS_VAUDP15_MASK_SFT        (0x1 << 1)
#define RG_AUDLOLMUXINPUTSEL_VAUDP15_SFT             2
#define RG_AUDLOLMUXINPUTSEL_VAUDP15_MASK            0x3
#define RG_AUDLOLMUXINPUTSEL_VAUDP15_MASK_SFT        (0x3 << 2)
#define RG_AUDLOLSCDISABLE_VAUDP15_SFT               4
#define RG_AUDLOLSCDISABLE_VAUDP15_MASK              0x1
#define RG_AUDLOLSCDISABLE_VAUDP15_MASK_SFT          (0x1 << 4)
#define RG_AUDLOLBSCCURRENT_VAUDP15_SFT              5
#define RG_AUDLOLBSCCURRENT_VAUDP15_MASK             0x1
#define RG_AUDLOLBSCCURRENT_VAUDP15_MASK_SFT         (0x1 << 5)
#define RG_AUDLOSTARTUP_VAUDP15_SFT                  6
#define RG_AUDLOSTARTUP_VAUDP15_MASK                 0x1
#define RG_AUDLOSTARTUP_VAUDP15_MASK_SFT             (0x1 << 6)
#define RG_LOINPUTSTBENH_VAUDP15_SFT                 7
#define RG_LOINPUTSTBENH_VAUDP15_MASK                0x1
#define RG_LOINPUTSTBENH_VAUDP15_MASK_SFT            (0x1 << 7)
#define RG_LOOUTPUTSTBENH_VAUDP15_SFT                8
#define RG_LOOUTPUTSTBENH_VAUDP15_MASK               0x1
#define RG_LOOUTPUTSTBENH_VAUDP15_MASK_SFT           (0x1 << 8)
#define RG_LOINPUTRESET0_VAUDP15_SFT                 9
#define RG_LOINPUTRESET0_VAUDP15_MASK                0x1
#define RG_LOINPUTRESET0_VAUDP15_MASK_SFT            (0x1 << 9)
#define RG_LOOUTPUTRESET0_VAUDP15_SFT                10
#define RG_LOOUTPUTRESET0_VAUDP15_MASK               0x1
#define RG_LOOUTPUTRESET0_VAUDP15_MASK_SFT           (0x1 << 10)
#define RG_LOOUT_SHORTVCM_VAUDP15_SFT                11
#define RG_LOOUT_SHORTVCM_VAUDP15_MASK               0x1
#define RG_LOOUT_SHORTVCM_VAUDP15_MASK_SFT           (0x1 << 11)

/* AUDDEC_ANA_CON8 */
#define RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP15_SFT        0
#define RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP15_MASK       0xf
#define RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP15_MASK_SFT   (0xf << 0)
#define RG_AUDTRIMBUF_GAINSEL_VAUDP15_SFT            4
#define RG_AUDTRIMBUF_GAINSEL_VAUDP15_MASK           0x3
#define RG_AUDTRIMBUF_GAINSEL_VAUDP15_MASK_SFT       (0x3 << 4)
#define RG_AUDTRIMBUF_EN_VAUDP15_SFT                 6
#define RG_AUDTRIMBUF_EN_VAUDP15_MASK                0x1
#define RG_AUDTRIMBUF_EN_VAUDP15_MASK_SFT            (0x1 << 6)
#define RG_AUDHPSPKDET_INPUTMUXSEL_VAUDP15_SFT       8
#define RG_AUDHPSPKDET_INPUTMUXSEL_VAUDP15_MASK      0x3
#define RG_AUDHPSPKDET_INPUTMUXSEL_VAUDP15_MASK_SFT  (0x3 << 8)
#define RG_AUDHPSPKDET_OUTPUTMUXSEL_VAUDP15_SFT      10
#define RG_AUDHPSPKDET_OUTPUTMUXSEL_VAUDP15_MASK     0x3
#define RG_AUDHPSPKDET_OUTPUTMUXSEL_VAUDP15_MASK_SFT (0x3 << 10)
#define RG_AUDHPSPKDET_EN_VAUDP15_SFT                12
#define RG_AUDHPSPKDET_EN_VAUDP15_MASK               0x1
#define RG_AUDHPSPKDET_EN_VAUDP15_MASK_SFT           (0x1 << 12)

/* AUDDEC_ANA_CON9 */
#define RG_ABIDEC_RSVD0_VA28_SFT                     0
#define RG_ABIDEC_RSVD0_VA28_MASK                    0xff
#define RG_ABIDEC_RSVD0_VA28_MASK_SFT                (0xff << 0)
#define RG_ABIDEC_RSVD0_VAUDP15_SFT                  8
#define RG_ABIDEC_RSVD0_VAUDP15_MASK                 0xff
#define RG_ABIDEC_RSVD0_VAUDP15_MASK_SFT             (0xff << 8)

/* AUDDEC_ANA_CON10 */
#define RG_ABIDEC_RSVD1_VAUDP15_SFT                  0
#define RG_ABIDEC_RSVD1_VAUDP15_MASK                 0xff
#define RG_ABIDEC_RSVD1_VAUDP15_MASK_SFT             (0xff << 0)
#define RG_ABIDEC_RSVD2_VAUDP15_SFT                  8
#define RG_ABIDEC_RSVD2_VAUDP15_MASK                 0xff
#define RG_ABIDEC_RSVD2_VAUDP15_MASK_SFT             (0xff << 8)

/* AUDDEC_ANA_CON11 */
#define RG_AUDZCDMUXSEL_VAUDP15_SFT                  0
#define RG_AUDZCDMUXSEL_VAUDP15_MASK                 0x7
#define RG_AUDZCDMUXSEL_VAUDP15_MASK_SFT             (0x7 << 0)
#define RG_AUDZCDCLKSEL_VAUDP15_SFT                  3
#define RG_AUDZCDCLKSEL_VAUDP15_MASK                 0x1
#define RG_AUDZCDCLKSEL_VAUDP15_MASK_SFT             (0x1 << 3)
#define RG_AUDBIASADJ_0_VAUDP15_SFT                  7
#define RG_AUDBIASADJ_0_VAUDP15_MASK                 0x1ff
#define RG_AUDBIASADJ_0_VAUDP15_MASK_SFT             (0x1ff << 7)

/* AUDDEC_ANA_CON12 */
#define RG_AUDBIASADJ_1_VAUDP15_SFT                  0
#define RG_AUDBIASADJ_1_VAUDP15_MASK                 0xff
#define RG_AUDBIASADJ_1_VAUDP15_MASK_SFT             (0xff << 0)
#define RG_AUDIBIASPWRDN_VAUDP15_SFT                 8
#define RG_AUDIBIASPWRDN_VAUDP15_MASK                0x1
#define RG_AUDIBIASPWRDN_VAUDP15_MASK_SFT            (0x1 << 8)

/* AUDDEC_ANA_CON13 */
#define RG_RSTB_DECODER_VA28_SFT                     0
#define RG_RSTB_DECODER_VA28_MASK                    0x1
#define RG_RSTB_DECODER_VA28_MASK_SFT                (0x1 << 0)
#define RG_SEL_DECODER_96K_VA28_SFT                  1
#define RG_SEL_DECODER_96K_VA28_MASK                 0x1
#define RG_SEL_DECODER_96K_VA28_MASK_SFT             (0x1 << 1)
#define RG_SEL_DELAY_VCORE_SFT                       2
#define RG_SEL_DELAY_VCORE_MASK                      0x1
#define RG_SEL_DELAY_VCORE_MASK_SFT                  (0x1 << 2)
#define RG_AUDGLB_PWRDN_VA28_SFT                     4
#define RG_AUDGLB_PWRDN_VA28_MASK                    0x1
#define RG_AUDGLB_PWRDN_VA28_MASK_SFT                (0x1 << 4)
#define RG_RSTB_ENCODER_VA28_SFT                     5
#define RG_RSTB_ENCODER_VA28_MASK                    0x1
#define RG_RSTB_ENCODER_VA28_MASK_SFT                (0x1 << 5)
#define RG_SEL_ENCODER_96K_VA28_SFT                  6
#define RG_SEL_ENCODER_96K_VA28_MASK                 0x1
#define RG_SEL_ENCODER_96K_VA28_MASK_SFT             (0x1 << 6)

/* AUDDEC_ANA_CON14 */
#define RG_HCLDO_EN_VA18_SFT                         0
#define RG_HCLDO_EN_VA18_MASK                        0x1
#define RG_HCLDO_EN_VA18_MASK_SFT                    (0x1 << 0)
#define RG_HCLDO_PDDIS_EN_VA18_SFT                   1
#define RG_HCLDO_PDDIS_EN_VA18_MASK                  0x1
#define RG_HCLDO_PDDIS_EN_VA18_MASK_SFT              (0x1 << 1)
#define RG_HCLDO_REMOTE_SENSE_VA18_SFT               2
#define RG_HCLDO_REMOTE_SENSE_VA18_MASK              0x1
#define RG_HCLDO_REMOTE_SENSE_VA18_MASK_SFT          (0x1 << 2)
#define RG_LCLDO_EN_VA18_SFT                         4
#define RG_LCLDO_EN_VA18_MASK                        0x1
#define RG_LCLDO_EN_VA18_MASK_SFT                    (0x1 << 4)
#define RG_LCLDO_PDDIS_EN_VA18_SFT                   5
#define RG_LCLDO_PDDIS_EN_VA18_MASK                  0x1
#define RG_LCLDO_PDDIS_EN_VA18_MASK_SFT              (0x1 << 5)
#define RG_LCLDO_REMOTE_SENSE_VA18_SFT               6
#define RG_LCLDO_REMOTE_SENSE_VA18_MASK              0x1
#define RG_LCLDO_REMOTE_SENSE_VA18_MASK_SFT          (0x1 << 6)
#define RG_LCLDO_ENC_EN_VA28_SFT                     8
#define RG_LCLDO_ENC_EN_VA28_MASK                    0x1
#define RG_LCLDO_ENC_EN_VA28_MASK_SFT                (0x1 << 8)
#define RG_LCLDO_ENC_PDDIS_EN_VA28_SFT               9
#define RG_LCLDO_ENC_PDDIS_EN_VA28_MASK              0x1
#define RG_LCLDO_ENC_PDDIS_EN_VA28_MASK_SFT          (0x1 << 9)
#define RG_LCLDO_ENC_REMOTE_SENSE_VA28_SFT           10
#define RG_LCLDO_ENC_REMOTE_SENSE_VA28_MASK          0x1
#define RG_LCLDO_ENC_REMOTE_SENSE_VA28_MASK_SFT      (0x1 << 10)
#define RG_VA33REFGEN_EN_VA18_SFT                    12
#define RG_VA33REFGEN_EN_VA18_MASK                   0x1
#define RG_VA33REFGEN_EN_VA18_MASK_SFT               (0x1 << 12)
#define RG_VA28REFGEN_EN_VA28_SFT                    13
#define RG_VA28REFGEN_EN_VA28_MASK                   0x1
#define RG_VA28REFGEN_EN_VA28_MASK_SFT               (0x1 << 13)
#define RG_HCLDO_VOSEL_VA18_SFT                      14
#define RG_HCLDO_VOSEL_VA18_MASK                     0x1
#define RG_HCLDO_VOSEL_VA18_MASK_SFT                 (0x1 << 14)
#define RG_LCLDO_VOSEL_VA18_SFT                      15
#define RG_LCLDO_VOSEL_VA18_MASK                     0x1
#define RG_LCLDO_VOSEL_VA18_MASK_SFT                 (0x1 << 15)

/* AUDDEC_ANA_CON15 */
#define RG_NVREG_EN_VAUDP15_SFT                      0
#define RG_NVREG_EN_VAUDP15_MASK                     0x1
#define RG_NVREG_EN_VAUDP15_MASK_SFT                 (0x1 << 0)
#define RG_NVREG_PULL0V_VAUDP15_SFT                  1
#define RG_NVREG_PULL0V_VAUDP15_MASK                 0x1
#define RG_NVREG_PULL0V_VAUDP15_MASK_SFT             (0x1 << 1)
#define RG_AUDPMU_RSD0_VAUDP15_SFT                   4
#define RG_AUDPMU_RSD0_VAUDP15_MASK                  0xf
#define RG_AUDPMU_RSD0_VAUDP15_MASK_SFT              (0xf << 4)
#define RG_AUDPMU_RSD0_VA18_SFT                      8
#define RG_AUDPMU_RSD0_VA18_MASK                     0xf
#define RG_AUDPMU_RSD0_VA18_MASK_SFT                 (0xf << 8)
#define RG_AUDPMU_RSD0_VA28_SFT                      12
#define RG_AUDPMU_RSD0_VA28_MASK                     0xf
#define RG_AUDPMU_RSD0_VA28_MASK_SFT                 (0xf << 12)

/* AUDENC_ANA_CON0 */
#define RG_AUDPREAMPLON_SFT                  0
#define RG_AUDPREAMPLON_MASK                 0x1
#define RG_AUDPREAMPLON_MASK_SFT             (0x1 << 0)
#define RG_AUDPREAMPLDCCEN_SFT               1
#define RG_AUDPREAMPLDCCEN_MASK              0x1
#define RG_AUDPREAMPLDCCEN_MASK_SFT          (0x1 << 1)
#define RG_AUDPREAMPLDCRPECHARGE_SFT         2
#define RG_AUDPREAMPLDCRPECHARGE_MASK        0x1
#define RG_AUDPREAMPLDCRPECHARGE_MASK_SFT    (0x1 << 2)
#define RG_AUDPREAMPLPGATEST_SFT             3
#define RG_AUDPREAMPLPGATEST_MASK            0x1
#define RG_AUDPREAMPLPGATEST_MASK_SFT        (0x1 << 3)
#define RG_AUDPREAMPLVSCALE_SFT              4
#define RG_AUDPREAMPLVSCALE_MASK             0x3
#define RG_AUDPREAMPLVSCALE_MASK_SFT         (0x3 << 4)
#define RG_AUDPREAMPLINPUTSEL_SFT            6
#define RG_AUDPREAMPLINPUTSEL_MASK           0x3
#define RG_AUDPREAMPLINPUTSEL_MASK_SFT       (0x3 << 6)
#define RG_AUDPREAMPLGAIN_SFT                8
#define RG_AUDPREAMPLGAIN_MASK               0x7
#define RG_AUDPREAMPLGAIN_MASK_SFT           (0x7 << 8)
#define RG_AUDADCLPWRUP_SFT                  12
#define RG_AUDADCLPWRUP_MASK                 0x1
#define RG_AUDADCLPWRUP_MASK_SFT             (0x1 << 12)
#define RG_AUDADCLINPUTSEL_SFT               13
#define RG_AUDADCLINPUTSEL_MASK              0x3
#define RG_AUDADCLINPUTSEL_MASK_SFT          (0x3 << 13)

/* AUDENC_ANA_CON1 */
#define RG_AUDPREAMPRON_SFT                  0
#define RG_AUDPREAMPRON_MASK                 0x1
#define RG_AUDPREAMPRON_MASK_SFT             (0x1 << 0)
#define RG_AUDPREAMPRDCCEN_SFT               1
#define RG_AUDPREAMPRDCCEN_MASK              0x1
#define RG_AUDPREAMPRDCCEN_MASK_SFT          (0x1 << 1)
#define RG_AUDPREAMPRDCRPECHARGE_SFT         2
#define RG_AUDPREAMPRDCRPECHARGE_MASK        0x1
#define RG_AUDPREAMPRDCRPECHARGE_MASK_SFT    (0x1 << 2)
#define RG_AUDPREAMPRPGATEST_SFT             3
#define RG_AUDPREAMPRPGATEST_MASK            0x1
#define RG_AUDPREAMPRPGATEST_MASK_SFT        (0x1 << 3)
#define RG_AUDPREAMPRVSCALE_SFT              4
#define RG_AUDPREAMPRVSCALE_MASK             0x3
#define RG_AUDPREAMPRVSCALE_MASK_SFT         (0x3 << 4)
#define RG_AUDPREAMPRINPUTSEL_SFT            6
#define RG_AUDPREAMPRINPUTSEL_MASK           0x3
#define RG_AUDPREAMPRINPUTSEL_MASK_SFT       (0x3 << 6)
#define RG_AUDPREAMPRGAIN_SFT                8
#define RG_AUDPREAMPRGAIN_MASK               0x7
#define RG_AUDPREAMPRGAIN_MASK_SFT           (0x7 << 8)
#define RG_AUDADCRPWRUP_SFT                  12
#define RG_AUDADCRPWRUP_MASK                 0x1
#define RG_AUDADCRPWRUP_MASK_SFT             (0x1 << 12)
#define RG_AUDADCRINPUTSEL_SFT               13
#define RG_AUDADCRINPUTSEL_MASK              0x3
#define RG_AUDADCRINPUTSEL_MASK_SFT          (0x3 << 13)

/* AUDENC_ANA_CON2 */
#define RG_AUDULHALFBIAS_SFT                 0
#define RG_AUDULHALFBIAS_MASK                0x1
#define RG_AUDULHALFBIAS_MASK_SFT            (0x1 << 0)
#define RG_AUDGLBMADLPWEN_SFT                1
#define RG_AUDGLBMADLPWEN_MASK               0x1
#define RG_AUDGLBMADLPWEN_MASK_SFT           (0x1 << 1)
#define RG_AUDPREAMPLPEN_SFT                 2
#define RG_AUDPREAMPLPEN_MASK                0x1
#define RG_AUDPREAMPLPEN_MASK_SFT            (0x1 << 2)
#define RG_AUDADC1STSTAGELPEN_SFT            3
#define RG_AUDADC1STSTAGELPEN_MASK           0x1
#define RG_AUDADC1STSTAGELPEN_MASK_SFT       (0x1 << 3)
#define RG_AUDADC2NDSTAGELPEN_SFT            4
#define RG_AUDADC2NDSTAGELPEN_MASK           0x1
#define RG_AUDADC2NDSTAGELPEN_MASK_SFT       (0x1 << 4)
#define RG_AUDADCFLASHLPEN_SFT               5
#define RG_AUDADCFLASHLPEN_MASK              0x1
#define RG_AUDADCFLASHLPEN_MASK_SFT          (0x1 << 5)
#define RG_AUDPREAMPIDDTEST_SFT              6
#define RG_AUDPREAMPIDDTEST_MASK             0x3
#define RG_AUDPREAMPIDDTEST_MASK_SFT         (0x3 << 6)
#define RG_AUDADC1STSTAGEIDDTEST_SFT         8
#define RG_AUDADC1STSTAGEIDDTEST_MASK        0x3
#define RG_AUDADC1STSTAGEIDDTEST_MASK_SFT    (0x3 << 8)
#define RG_AUDADC2NDSTAGEIDDTEST_SFT         10
#define RG_AUDADC2NDSTAGEIDDTEST_MASK        0x3
#define RG_AUDADC2NDSTAGEIDDTEST_MASK_SFT    (0x3 << 10)
#define RG_AUDADCREFBUFIDDTEST_SFT           12
#define RG_AUDADCREFBUFIDDTEST_MASK          0x3
#define RG_AUDADCREFBUFIDDTEST_MASK_SFT      (0x3 << 12)
#define RG_AUDADCFLASHIDDTEST_SFT            14
#define RG_AUDADCFLASHIDDTEST_MASK           0x3
#define RG_AUDADCFLASHIDDTEST_MASK_SFT       (0x3 << 14)

/* AUDENC_ANA_CON3 */
#define RG_AUDADCDAC0P25FS_SFT               0
#define RG_AUDADCDAC0P25FS_MASK              0x1
#define RG_AUDADCDAC0P25FS_MASK_SFT          (0x1 << 0)
#define RG_AUDADCCLKSEL_SFT                  1
#define RG_AUDADCCLKSEL_MASK                 0x1
#define RG_AUDADCCLKSEL_MASK_SFT             (0x1 << 1)
#define RG_AUDADCCLKSOURCE_SFT               2
#define RG_AUDADCCLKSOURCE_MASK              0x3
#define RG_AUDADCCLKSOURCE_MASK_SFT          (0x3 << 2)
#define RG_AUDPREAMPAAFEN_SFT                8
#define RG_AUDPREAMPAAFEN_MASK               0x1
#define RG_AUDPREAMPAAFEN_MASK_SFT           (0x1 << 8)
#define RG_DCCVCMBUFLPMODSEL_SFT             9
#define RG_DCCVCMBUFLPMODSEL_MASK            0x1
#define RG_DCCVCMBUFLPMODSEL_MASK_SFT        (0x1 << 9)
#define RG_DCCVCMBUFLPSWEN_SFT               10
#define RG_DCCVCMBUFLPSWEN_MASK              0x1
#define RG_DCCVCMBUFLPSWEN_MASK_SFT          (0x1 << 10)
#define RG_AUDSPAREPGA_SFT                   11
#define RG_AUDSPAREPGA_MASK                  0x1
#define RG_AUDSPAREPGA_MASK_SFT              (0x1 << 11)

/* AUDENC_ANA_CON4 */
#define RG_AUDADC1STSTAGESDENB_SFT           0
#define RG_AUDADC1STSTAGESDENB_MASK          0x1
#define RG_AUDADC1STSTAGESDENB_MASK_SFT      (0x1 << 0)
#define RG_AUDADC2NDSTAGERESET_SFT           1
#define RG_AUDADC2NDSTAGERESET_MASK          0x1
#define RG_AUDADC2NDSTAGERESET_MASK_SFT      (0x1 << 1)
#define RG_AUDADC3RDSTAGERESET_SFT           2
#define RG_AUDADC3RDSTAGERESET_MASK          0x1
#define RG_AUDADC3RDSTAGERESET_MASK_SFT      (0x1 << 2)
#define RG_AUDADCFSRESET_SFT                 3
#define RG_AUDADCFSRESET_MASK                0x1
#define RG_AUDADCFSRESET_MASK_SFT            (0x1 << 3)
#define RG_AUDADCWIDECM_SFT                  4
#define RG_AUDADCWIDECM_MASK                 0x1
#define RG_AUDADCWIDECM_MASK_SFT             (0x1 << 4)
#define RG_AUDADCNOPATEST_SFT                5
#define RG_AUDADCNOPATEST_MASK               0x1
#define RG_AUDADCNOPATEST_MASK_SFT           (0x1 << 5)
#define RG_AUDADCBYPASS_SFT                  6
#define RG_AUDADCBYPASS_MASK                 0x1
#define RG_AUDADCBYPASS_MASK_SFT             (0x1 << 6)
#define RG_AUDADCFFBYPASS_SFT                7
#define RG_AUDADCFFBYPASS_MASK               0x1
#define RG_AUDADCFFBYPASS_MASK_SFT           (0x1 << 7)
#define RG_AUDADCDACFBCURRENT_SFT            8
#define RG_AUDADCDACFBCURRENT_MASK           0x1
#define RG_AUDADCDACFBCURRENT_MASK_SFT       (0x1 << 8)
#define RG_AUDADCDACIDDTEST_SFT              9
#define RG_AUDADCDACIDDTEST_MASK             0x3
#define RG_AUDADCDACIDDTEST_MASK_SFT         (0x3 << 9)
#define RG_AUDADCDACNRZ_SFT                  11
#define RG_AUDADCDACNRZ_MASK                 0x1
#define RG_AUDADCDACNRZ_MASK_SFT             (0x1 << 11)
#define RG_AUDADCNODEM_SFT                   12
#define RG_AUDADCNODEM_MASK                  0x1
#define RG_AUDADCNODEM_MASK_SFT              (0x1 << 12)
#define RG_AUDADCDACTEST_SFT                 13
#define RG_AUDADCDACTEST_MASK                0x1
#define RG_AUDADCDACTEST_MASK_SFT            (0x1 << 13)

/* AUDENC_ANA_CON5 */
#define RG_AUDADCTESTDATA_SFT                0
#define RG_AUDADCTESTDATA_MASK               0xffff
#define RG_AUDADCTESTDATA_MASK_SFT           (0xffff << 0)

/* AUDENC_ANA_CON6 */
#define RG_AUDRCTUNEL_SFT                    0
#define RG_AUDRCTUNEL_MASK                   0x1f
#define RG_AUDRCTUNEL_MASK_SFT               (0x1f << 0)
#define RG_AUDRCTUNELSEL_SFT                 5
#define RG_AUDRCTUNELSEL_MASK                0x1
#define RG_AUDRCTUNELSEL_MASK_SFT            (0x1 << 5)
#define RG_AUDRCTUNER_SFT                    8
#define RG_AUDRCTUNER_MASK                   0x1f
#define RG_AUDRCTUNER_MASK_SFT               (0x1f << 8)
#define RG_AUDRCTUNERSEL_SFT                 13
#define RG_AUDRCTUNERSEL_MASK                0x1
#define RG_AUDRCTUNERSEL_MASK_SFT            (0x1 << 13)

/* AUDENC_ANA_CON7 */
#define RG_AUDSPAREVA28_SFT                  0
#define RG_AUDSPAREVA28_MASK                 0xf
#define RG_AUDSPAREVA28_MASK_SFT             (0xf << 0)
#define RG_AUDSPAREVA18_SFT                  4
#define RG_AUDSPAREVA18_MASK                 0xf
#define RG_AUDSPAREVA18_MASK_SFT             (0xf << 4)
#define RG_AUDENCSPAREVA28_SFT               8
#define RG_AUDENCSPAREVA28_MASK              0xf
#define RG_AUDENCSPAREVA28_MASK_SFT          (0xf << 8)
#define RG_AUDENCSPAREVA18_SFT               12
#define RG_AUDENCSPAREVA18_MASK              0xf
#define RG_AUDENCSPAREVA18_MASK_SFT          (0xf << 12)

/* AUDENC_ANA_CON8 */
#define RG_AUDDIGMICEN_SFT                   0
#define RG_AUDDIGMICEN_MASK                  0x1
#define RG_AUDDIGMICEN_MASK_SFT              (0x1 << 0)
#define RG_AUDDIGMICBIAS_SFT                 1
#define RG_AUDDIGMICBIAS_MASK                0x3
#define RG_AUDDIGMICBIAS_MASK_SFT            (0x3 << 1)
#define RG_DMICHPCLKEN_SFT                   3
#define RG_DMICHPCLKEN_MASK                  0x1
#define RG_DMICHPCLKEN_MASK_SFT              (0x1 << 3)
#define RG_AUDDIGMICPDUTY_SFT                4
#define RG_AUDDIGMICPDUTY_MASK               0x3
#define RG_AUDDIGMICPDUTY_MASK_SFT           (0x3 << 4)
#define RG_AUDDIGMICNDUTY_SFT                6
#define RG_AUDDIGMICNDUTY_MASK               0x3
#define RG_AUDDIGMICNDUTY_MASK_SFT           (0x3 << 6)
#define RG_DMICMONEN_SFT                     8
#define RG_DMICMONEN_MASK                    0x1
#define RG_DMICMONEN_MASK_SFT                (0x1 << 8)
#define RG_DMICMONSEL_SFT                    9
#define RG_DMICMONSEL_MASK                   0x7
#define RG_DMICMONSEL_MASK_SFT               (0x7 << 9)
#define RG_AUDSPAREVMIC_SFT                  12
#define RG_AUDSPAREVMIC_MASK                 0xf
#define RG_AUDSPAREVMIC_MASK_SFT             (0xf << 12)

/* AUDENC_ANA_CON9 */
#define RG_AUDPWDBMICBIAS0_SFT               0
#define RG_AUDPWDBMICBIAS0_MASK              0x1
#define RG_AUDPWDBMICBIAS0_MASK_SFT          (0x1 << 0)
#define RG_AUDMICBIAS0BYPASSEN_SFT           1
#define RG_AUDMICBIAS0BYPASSEN_MASK          0x1
#define RG_AUDMICBIAS0BYPASSEN_MASK_SFT      (0x1 << 1)
#define RG_AUDMICBIAS0LOWPEN_SFT             2
#define RG_AUDMICBIAS0LOWPEN_MASK            0x1
#define RG_AUDMICBIAS0LOWPEN_MASK_SFT        (0x1 << 2)
#define RG_AUDMICBIAS0VREF_SFT               4
#define RG_AUDMICBIAS0VREF_MASK              0x7
#define RG_AUDMICBIAS0VREF_MASK_SFT          (0x7 << 4)
#define RG_AUDMICBIAS0DCSW0P1EN_SFT          8
#define RG_AUDMICBIAS0DCSW0P1EN_MASK         0x1
#define RG_AUDMICBIAS0DCSW0P1EN_MASK_SFT     (0x1 << 8)
#define RG_AUDMICBIAS0DCSW0P2EN_SFT          9
#define RG_AUDMICBIAS0DCSW0P2EN_MASK         0x1
#define RG_AUDMICBIAS0DCSW0P2EN_MASK_SFT     (0x1 << 9)
#define RG_AUDMICBIAS0DCSW0NEN_SFT           10
#define RG_AUDMICBIAS0DCSW0NEN_MASK          0x1
#define RG_AUDMICBIAS0DCSW0NEN_MASK_SFT      (0x1 << 10)
#define RG_AUDMICBIAS0DCSW2P1EN_SFT          12
#define RG_AUDMICBIAS0DCSW2P1EN_MASK         0x1
#define RG_AUDMICBIAS0DCSW2P1EN_MASK_SFT     (0x1 << 12)
#define RG_AUDMICBIAS0DCSW2P2EN_SFT          13
#define RG_AUDMICBIAS0DCSW2P2EN_MASK         0x1
#define RG_AUDMICBIAS0DCSW2P2EN_MASK_SFT     (0x1 << 13)
#define RG_AUDMICBIAS0DCSW2NEN_SFT           14
#define RG_AUDMICBIAS0DCSW2NEN_MASK          0x1
#define RG_AUDMICBIAS0DCSW2NEN_MASK_SFT      (0x1 << 14)

/* AUDENC_ANA_CON10 */
#define RG_AUDPWDBMICBIAS1_SFT               0
#define RG_AUDPWDBMICBIAS1_MASK              0x1
#define RG_AUDPWDBMICBIAS1_MASK_SFT          (0x1 << 0)
#define RG_AUDMICBIAS1BYPASSEN_SFT           1
#define RG_AUDMICBIAS1BYPASSEN_MASK          0x1
#define RG_AUDMICBIAS1BYPASSEN_MASK_SFT      (0x1 << 1)
#define RG_AUDMICBIAS1LOWPEN_SFT             2
#define RG_AUDMICBIAS1LOWPEN_MASK            0x1
#define RG_AUDMICBIAS1LOWPEN_MASK_SFT        (0x1 << 2)
#define RG_AUDMICBIAS1VREF_SFT               4
#define RG_AUDMICBIAS1VREF_MASK              0x7
#define RG_AUDMICBIAS1VREF_MASK_SFT          (0x7 << 4)
#define RG_AUDMICBIAS1DCSW1PEN_SFT           8
#define RG_AUDMICBIAS1DCSW1PEN_MASK          0x1
#define RG_AUDMICBIAS1DCSW1PEN_MASK_SFT      (0x1 << 8)
#define RG_AUDMICBIAS1DCSW1NEN_SFT           9
#define RG_AUDMICBIAS1DCSW1NEN_MASK          0x1
#define RG_AUDMICBIAS1DCSW1NEN_MASK_SFT      (0x1 << 9)
#define RG_BANDGAPGEN_SFT                    12
#define RG_BANDGAPGEN_MASK                   0x1
#define RG_BANDGAPGEN_MASK_SFT               (0x1 << 12)

/* AUDENC_ANA_CON11 */
#define RG_AUDACCDETMICBIAS0PULLLOW_SFT      0
#define RG_AUDACCDETMICBIAS0PULLLOW_MASK     0x1
#define RG_AUDACCDETMICBIAS0PULLLOW_MASK_SFT (0x1 << 0)
#define RG_AUDACCDETMICBIAS1PULLLOW_SFT      1
#define RG_AUDACCDETMICBIAS1PULLLOW_MASK     0x1
#define RG_AUDACCDETMICBIAS1PULLLOW_MASK_SFT (0x1 << 1)
#define RG_AUDACCDETVIN1PULLLOW_SFT          2
#define RG_AUDACCDETVIN1PULLLOW_MASK         0x1
#define RG_AUDACCDETVIN1PULLLOW_MASK_SFT     (0x1 << 2)
#define RG_AUDACCDETVTHACAL_SFT              4
#define RG_AUDACCDETVTHACAL_MASK             0x1
#define RG_AUDACCDETVTHACAL_MASK_SFT         (0x1 << 4)
#define RG_AUDACCDETVTHBCAL_SFT              5
#define RG_AUDACCDETVTHBCAL_MASK             0x1
#define RG_AUDACCDETVTHBCAL_MASK_SFT         (0x1 << 5)
#define RG_AUDACCDETTVDET_SFT                6
#define RG_AUDACCDETTVDET_MASK               0x1
#define RG_AUDACCDETTVDET_MASK_SFT           (0x1 << 6)
#define RG_ACCDETSEL_SFT                     7
#define RG_ACCDETSEL_MASK                    0x1
#define RG_ACCDETSEL_MASK_SFT                (0x1 << 7)
#define RG_SWBUFMODSEL_SFT                   8
#define RG_SWBUFMODSEL_MASK                  0x1
#define RG_SWBUFMODSEL_MASK_SFT              (0x1 << 8)
#define RG_SWBUFSWEN_SFT                     9
#define RG_SWBUFSWEN_MASK                    0x1
#define RG_SWBUFSWEN_MASK_SFT                (0x1 << 9)
#define RG_EINTCOMPVTH_SFT                   10
#define RG_EINTCOMPVTH_MASK                  0x1
#define RG_EINTCOMPVTH_MASK_SFT              (0x1 << 10)
#define RG_EINTCONFIGACCDET_SFT              11
#define RG_EINTCONFIGACCDET_MASK             0x1
#define RG_EINTCONFIGACCDET_MASK_SFT         (0x1 << 11)
#define RG_EINTHIRENB_SFT                    12
#define RG_EINTHIRENB_MASK                   0x1
#define RG_EINTHIRENB_MASK_SFT               (0x1 << 12)
#define RG_ACCDETSPAREVA28_SFT               13
#define RG_ACCDETSPAREVA28_MASK              0x7
#define RG_ACCDETSPAREVA28_MASK_SFT          (0x7 << 13)

/* AUDENC_ANA_CON12 */
#define RGS_AUDRCTUNELREAD_SFT               0
#define RGS_AUDRCTUNELREAD_MASK              0x1f
#define RGS_AUDRCTUNELREAD_MASK_SFT          (0x1f << 0)
#define RGS_AUDRCTUNERREAD_SFT               8
#define RGS_AUDRCTUNERREAD_MASK              0x1f
#define RGS_AUDRCTUNERREAD_MASK_SFT          (0x1f << 8)

/* ZCD_CON0 */
#define RG_AUDZCDENABLE_SFT              0
#define RG_AUDZCDENABLE_MASK             0x1
#define RG_AUDZCDENABLE_MASK_SFT         (0x1 << 0)
#define RG_AUDZCDGAINSTEPTIME_SFT        1
#define RG_AUDZCDGAINSTEPTIME_MASK       0x7
#define RG_AUDZCDGAINSTEPTIME_MASK_SFT   (0x7 << 1)
#define RG_AUDZCDGAINSTEPSIZE_SFT        4
#define RG_AUDZCDGAINSTEPSIZE_MASK       0x3
#define RG_AUDZCDGAINSTEPSIZE_MASK_SFT   (0x3 << 4)
#define RG_AUDZCDTIMEOUTMODESEL_SFT      6
#define RG_AUDZCDTIMEOUTMODESEL_MASK     0x1
#define RG_AUDZCDTIMEOUTMODESEL_MASK_SFT (0x1 << 6)

/* ZCD_CON1 */
#define RG_AUDLOLGAIN_SFT                0
#define RG_AUDLOLGAIN_MASK               0x1f
#define RG_AUDLOLGAIN_MASK_SFT           (0x1f << 0)
#define RG_AUDLORGAIN_SFT                7
#define RG_AUDLORGAIN_MASK               0x1f
#define RG_AUDLORGAIN_MASK_SFT           (0x1f << 7)

/* ZCD_CON2 */
#define RG_AUDHPLGAIN_SFT                0
#define RG_AUDHPLGAIN_MASK               0x1f
#define RG_AUDHPLGAIN_MASK_SFT           (0x1f << 0)
#define RG_AUDHPRGAIN_SFT                7
#define RG_AUDHPRGAIN_MASK               0x1f
#define RG_AUDHPRGAIN_MASK_SFT           (0x1f << 7)

/* ZCD_CON3 */
#define RG_AUDHSGAIN_SFT                 0
#define RG_AUDHSGAIN_MASK                0x1f
#define RG_AUDHSGAIN_MASK_SFT            (0x1f << 0)

/* ZCD_CON4 */
#define RG_AUDIVLGAIN_SFT                0
#define RG_AUDIVLGAIN_MASK               0x7
#define RG_AUDIVLGAIN_MASK_SFT           (0x7 << 0)
#define RG_AUDIVRGAIN_SFT                8
#define RG_AUDIVRGAIN_MASK               0x7
#define RG_AUDIVRGAIN_MASK_SFT           (0x7 << 8)

/* ZCD_CON5 */
#define RG_AUDINTGAIN1_SFT               0
#define RG_AUDINTGAIN1_MASK              0x3f
#define RG_AUDINTGAIN1_MASK_SFT          (0x3f << 0)
#define RG_AUDINTGAIN2_SFT               8
#define RG_AUDINTGAIN2_MASK              0x3f
#define RG_AUDINTGAIN2_MASK_SFT          (0x3f << 8)

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
#define DRV_CON3            0x3a
#define GPIO_DIR0           0x4c

#define GPIO_MODE2          0x7a /* mosi */
#define GPIO_MODE2_SET      0x7c
#define GPIO_MODE2_CLR      0x7e

#define GPIO_MODE3          0x80 /* miso */
#define GPIO_MODE3_SET      0x82
#define GPIO_MODE3_CLR      0x84

#define TOP_CKPDN_CON0      0x10a
#define TOP_CKPDN_CON0_SET  0x10c
#define TOP_CKPDN_CON0_CLR  0x10e

#define TOP_CKHWEN_CON0     0x128
#define TOP_CKHWEN_CON0_SET 0x12a
#define TOP_CKHWEN_CON0_CLR 0x12c

#define TOP_CLKSQ           0x132
#define TOP_CLKSQ_SET       0x134
#define TOP_CLKSQ_CLR       0x136

#define OTP_CON0            0x1a2
#define OTP_CON8            0x1b2
#define OTP_CON11           0x1b8
#define OTP_CON12           0x1ba
#define OTP_CON13           0x1bc

#define DCXO_CW14           0x9ea

#define AUXADC_CON10        0x816

/* audio register */
#define AUD_TOP_ID                    0x1540
#define AUD_TOP_REV0                  0x1542
#define AUD_TOP_REV1                  0x1544
#define AUD_TOP_CKPDN_PM0             0x1546
#define AUD_TOP_CKPDN_PM1             0x1548
#define AUD_TOP_CKPDN_CON0            0x154a
#define AUD_TOP_CKSEL_CON0            0x154c
#define AUD_TOP_CKTST_CON0            0x154e
#define AUD_TOP_RST_CON0              0x1550
#define AUD_TOP_RST_BANK_CON0         0x1552
#define AUD_TOP_INT_CON0              0x1554
#define AUD_TOP_INT_CON0_SET          0x1556
#define AUD_TOP_INT_CON0_CLR          0x1558
#define AUD_TOP_INT_MASK_CON0         0x155a
#define AUD_TOP_INT_MASK_CON0_SET     0x155c
#define AUD_TOP_INT_MASK_CON0_CLR     0x155e
#define AUD_TOP_INT_STATUS0           0x1560
#define AUD_TOP_INT_RAW_STATUS0       0x1562
#define AUD_TOP_INT_MISC_CON0         0x1564
#define AUDNCP_CLKDIV_CON0            0x1566
#define AUDNCP_CLKDIV_CON1            0x1568
#define AUDNCP_CLKDIV_CON2            0x156a
#define AUDNCP_CLKDIV_CON3            0x156c
#define AUDNCP_CLKDIV_CON4            0x156e
#define AUD_TOP_MON_CON0              0x1570
#define AUDIO_DIG_ID                  0x1580
#define AUDIO_DIG_REV0                0x1582
#define AUDIO_DIG_REV1                0x1584
#define AFE_UL_DL_CON0                0x1586
#define AFE_DL_SRC2_CON0_L            0x1588
#define AFE_UL_SRC_CON0_H             0x158a
#define AFE_UL_SRC_CON0_L             0x158c
#define PMIC_AFE_TOP_CON0             0x158e
#define PMIC_AUDIO_TOP_CON0           0x1590
#define AFE_MON_DEBUG0                0x1592
#define AFUNC_AUD_CON0                0x1594
#define AFUNC_AUD_CON1                0x1596
#define AFUNC_AUD_CON2                0x1598
#define AFUNC_AUD_CON3                0x159a
#define AFUNC_AUD_CON4                0x159c
#define AFUNC_AUD_MON0                0x159e
#define AUDRC_TUNE_MON0               0x15a0
#define AFE_ADDA_MTKAIF_FIFO_CFG0     0x15a2
#define AFE_ADDA_MTKAIF_FIFO_LOG_MON1 0x15a4
#define PMIC_AFE_ADDA_MTKAIF_MON0     0x15a6
#define PMIC_AFE_ADDA_MTKAIF_MON1     0x15a8
#define PMIC_AFE_ADDA_MTKAIF_MON2     0x15aa
#define PMIC_AFE_ADDA_MTKAIF_MON3     0x15ac
#define PMIC_AFE_ADDA_MTKAIF_CFG0     0x15ae
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG0  0x15b0
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG1  0x15b2
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG2  0x15b4
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG3  0x15b6
#define PMIC_AFE_ADDA_MTKAIF_TX_CFG1  0x15b8
#define AFE_SGEN_CFG0                 0x15ba
#define AFE_SGEN_CFG1                 0x15bc
#define AFE_ADC_ASYNC_FIFO_CFG        0x15be
#define AFE_DCCLK_CFG0                0x15c0
#define AFE_DCCLK_CFG1                0x15c2
#define AUDIO_DIG_CFG                 0x15c4
#define AFE_AUD_PAD_TOP               0x15c6
#define AFE_AUD_PAD_TOP_MON           0x15c8
#define AFE_AUD_PAD_TOP_MON1          0x15ca
#define AUDENC_DSN_ID                 0x1600
#define AUDENC_DSN_REV0               0x1602
#define AUDENC_DSN_REV1               0x1604
#define AUDENC_ANA_CON0               0x1606
#define AUDENC_ANA_CON1               0x1608
#define AUDENC_ANA_CON2               0x160a
#define AUDENC_ANA_CON3               0x160c
#define AUDENC_ANA_CON4               0x160e
#define AUDENC_ANA_CON5               0x1610
#define AUDENC_ANA_CON6               0x1612
#define AUDENC_ANA_CON7               0x1614
#define AUDENC_ANA_CON8               0x1616
#define AUDENC_ANA_CON9               0x1618
#define AUDENC_ANA_CON10              0x161a
#define AUDENC_ANA_CON11              0x161c
#define AUDENC_ANA_CON12              0x161e
#define AUDDEC_DSN_ID                 0x1640
#define AUDDEC_DSN_REV0               0x1642
#define AUDDEC_DSN_REV1               0x1644
#define AUDDEC_ANA_CON0               0x1646
#define AUDDEC_ANA_CON1               0x1648
#define AUDDEC_ANA_CON2               0x164a
#define AUDDEC_ANA_CON3               0x164c
#define AUDDEC_ANA_CON4               0x164e
#define AUDDEC_ANA_CON5               0x1650
#define AUDDEC_ANA_CON6               0x1652
#define AUDDEC_ANA_CON7               0x1654
#define AUDDEC_ANA_CON8               0x1656
#define AUDDEC_ANA_CON9               0x1658
#define AUDDEC_ANA_CON10              0x165a
#define AUDDEC_ANA_CON11              0x165c
#define AUDDEC_ANA_CON12              0x165e
#define AUDDEC_ANA_CON13              0x1660
#define AUDDEC_ANA_CON14              0x1662
#define AUDDEC_ANA_CON15              0x1664
#define AUDDEC_ELR_NUM                0x1666
#define AUDDEC_ELR_0                  0x1668
#define AUDZCDID                      0x1680
#define AUDZCDREV0                    0x1682
#define AUDZCDREV1                    0x1684
#define ZCD_CON0                      0x1686
#define ZCD_CON1                      0x1688
#define ZCD_CON2                      0x168a
#define ZCD_CON3                      0x168c
#define ZCD_CON4                      0x168e
#define ZCD_CON5                      0x1690
#define ACCDET_DIG_ID                 0x16c0
#define ACCDET_DIG_REV0               0x16c2
#define ACCDET_DIG_REV1               0x16c4
#define ACCDET_CON0                   0x16c6
#define ACCDET_CON1                   0x16c8
#define ACCDET_CON2                   0x16ca
#define ACCDET_CON3                   0x16cc
#define ACCDET_CON4                   0x16ce
#define ACCDET_CON5                   0x16d0
#define ACCDET_CON6                   0x16d2
#define ACCDET_CON7                   0x16d4
#define ACCDET_CON8                   0x16d6
#define ACCDET_CON9                   0x16d8
#define ACCDET_CON10                  0x16da
#define ACCDET_CON11                  0x16dc
#define ACCDET_CON12                  0x16de
#define ACCDET_CON13                  0x16e0
#define ACCDET_CON14                  0x16e2
#define ACCDET_CON15                  0x16e4
#define ACCDET_CON16                  0x16e6
#define ACCDET_CON17                  0x16e8
#define ACCDET_CON18                  0x16ea
#define ACCDET_CON19                  0x16ec
#define ACCDET_CON20                  0x16ee
#define ACCDET_CON21                  0x16f0
#define ACCDET_CON22                  0x16f2
#define ACCDET_CON23                  0x16f4
#define ACCDET_CON24                  0x16f6
#define ACCDET_CON25                  0x16f8
#define ACCDET_CON26                  0x16fa
#define ACCDET_CON27                  0x16fc
#define ACCDET_CON28                  0x16fe
#define ACCDET_ELR_NUM                0x1700
#define ACCDET_ELR0                   0x1702
#define ACCDET_ELR1                   0x1704
#define ACCDET_ELR2                   0x1706

#define MT6358_MAX_REGISTER ACCDET_ELR2

#endif /* __MT6358_H__ */

