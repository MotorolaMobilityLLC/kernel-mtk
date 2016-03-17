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
 *   AudDrv_devtree_parser.c
 *
 * Project:
 * --------
 *
 *
 * Description:
 * ------------
 *   AudDrv_devtree_parser
 *
 * Author:
 * -------
 *   Chipeng Chang (mtk02308)
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/
#include <mt-plat/aee.h>

void SetUnderFlowThreshold(unsigned int Threshold);
void Auddrv_Aee_Dump(void);
void Auddrv_Set_UnderFlow(void);
void Auddrv_Reset_Dump_State(void);
void Auddrv_Set_Interrupt_Changed(bool bChange);
void Auddrv_CheckInterruptTiming(void);
void RefineInterrruptInterval(void);
bool Auddrv_Set_DlSamplerate(unsigned int Samplerate);
bool Auddrv_Set_InterruptSample(unsigned int count);
bool Auddrv_Enable_dump(bool bEnable);
