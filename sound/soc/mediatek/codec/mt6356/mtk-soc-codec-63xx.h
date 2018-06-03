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
*
* You should have received a copy of the GNU General Public License
* along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/


/*******************************************************************************
 *
 * Filename:
 * ---------
 *  mtk-soc-codec-63xx.h
 *
 * Project:
 * --------
 *    Audio codec header file
 *
 * Description:
 * ------------
 *   Audio codec function
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *******************************************************************************/

#ifndef _AUDIO_CODEC_63xx_H
#define _AUDIO_CODEC_63xx_H

/* Headphone Impedance Detection */
struct mtk_hpdet_param {
	int auxadc_upper_bound;
	int dc_Step;
	int dc_Phase0;
	int dc_Phase1;
	int dc_Phase2;
	int resistance_first_threshold;
	int resistance_second_threshold;
};

struct mtk_codec_ops {
	int (*enable_dc_compensation)(bool enable);
	int (*set_lch_dc_compensation)(int value);
	int (*set_rch_dc_compensation)(int value);

	int (*set_ap_dmic)(bool enable);
};

void audckbufEnable(bool enable);
void OpenAnalogTrimHardware(bool bEnable);
void setOffsetTrimMux(unsigned int Mux);
void setOffsetTrimBufferGain(unsigned int gain);
void EnableTrimbuffer(bool benable);
void SetHplTrimOffset(int Offset);
void SetHprTrimOffset(int Offset);
void setHpGainZero(void);
void CalculateDCCompenForEachdB_L(void);
void CalculateDCCompenForEachdB_R(void);
void set_hp_impedance(int impedance);

/* headphone impedance detection function*/
int read_efuse_hp_impedance_current_calibration(void);
bool OpenHeadPhoneImpedanceSetting(bool bEnable);
void mtk_read_hp_detection_parameter(struct mtk_hpdet_param *hpdet_param);
int mtk_calculate_impedance_formula(int pcm_offset, int aux_diff);

void SetAnalogSuspend(bool bEnable);

bool hasHpDepopHw(void);
bool hasHp33Ohm(void);

int set_codec_ops(struct mtk_codec_ops *ops);

#endif
