/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *  mt_sco_digital_type.h
 *
 * Project:
 * --------
 *   MT6583  Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/

#ifndef _AUDIO_HDMI_TYPE_H
#define _AUDIO_HDMI_TYPE_H

#include <linux/list.h>

/*****************************************************************************
 *                ENUM DEFINITION
 ******************************************************************************/
enum HDMI_DISPLAY_TYPE {
	HDMI_DISPLAY_MHL = 0,
	HDMI_DISPLAY_SILMPORT = 1,
};

typedef struct {
	uint8 mHDMI_DisplayType;
	uint32 mI2Snum;
	uint32 mI2S_MCKDIV;
	uint32 mI2S_BCKDIV;

	uint32 mHDMI_Samplerate;
	uint32 mHDMI_Channels; /* channel number HDMI transmitted */
	uint32 mHDMI_Data_Lens;
	uint32 mTDM_Data_Lens;
	uint32 mClock_Data_Lens;
	uint32 mTDM_LRCK;
	uint32 msDATA_Channels; /* channel number per sdata */
	uint32 mMemIfFetchFormatPerSample;
	bool mSdata0;
	bool mSdata1;
	bool mSdata2;
	bool mSdata3;
} AudioHDMIFormat;


#endif
