/******************************************************************************
*
 *
 * Filename:
 * ---------
 *   AudDrv_Common_func.h
 *
 * Project:
 * --------
 *   MT6583 FPGA LDVT Audio Driver
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   George
 *
 *---------------------------------------------------------------------------
---
 *
 *

*******************************************************************************/

#ifndef AUDIO_COMMON_FUNC_H
#define AUDIO_COMMON_FUNC_H

bool get_voice_bt_status(void);
bool get_voice_status(void);
bool get_voice_md2_bt_status(void);
bool get_voice_md2_status(void);
void Auddrv_Read_Efuse_HPOffset(void);
bool get_internalmd_status(void);

#endif
