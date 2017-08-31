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

#ifndef _HAL_API_H_
#define _HAL_API_H_
#include "val_types_public.h"
#include "hal_types_private.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @par Function
 *   eHalInit
 * @par Description
 *   The init hal driver function
 * @param
 *   a_phHalHandle      [IN/OUT] The hal handle
 * @param
 *   a_eHalCodecType    [IN] VDEC or VENC
 * @par Returns
 *   VAL_RESULT_T,
 *   return VAL_RESULT_NO_ERROR if success,
 *   return VAL_RESULT_INVALID_DRIVER or VAL_RESULT_INVALID_MEMORY if failed
 */
VAL_RESULT_T eHalInit(VAL_HANDLE_T *a_phHalHandle, HAL_CODEC_TYPE_T a_eHalCodecType);


/**
 * @par Function
 *   eHalDeInit
 * @par Description
 *   The deinit hal driver function
 * @param
 *   a_phHalHandle      [IN/OUT] The hal handle
 * @par Returns
 *   VAL_RESULT_T, return VAL_RESULT_NO_ERROR if success, return else if failed
 */
VAL_RESULT_T eHalDeInit(VAL_HANDLE_T *a_phHalHandle);


/**
 * @par Function
 *   eHalGetMMAP
 * @par Description
 *   The get hw register memory map to vitural address function
 * @param
 *   a_hHalHandle       [IN/OUT] The hal handle
 * @param
 *   RegAddr            [IN] hw register address
 * @par Returns
 *   VAL_UINT32_T, vitural address of hw register memory mapping
 */
VAL_ULONG_T eHalGetMMAP(VAL_HANDLE_T *a_hHalHandle, VAL_UINT32_T RegAddr);


/**
 * @par Function
 *   eHalCmdProc
 * @par Description
 *   The hal command processing function
 * @param
 *   a_hHalHandle       [IN/OUT] The hal handle
 * @param
 *   a_eHalCmd          [IN] The hal command structure
 * @param
 *   a_pvInParam        [IN] The hal input parameter
 * @param
 *   a_pvOutParam       [OUT] The hal output parameter
 * @par Returns
 *   VAL_RESULT_T, return VAL_RESULT_NO_ERROR if success, return else if failed
 */
VAL_RESULT_T eHalCmdProc(
	VAL_HANDLE_T *a_hHalHandle,
	HAL_CMD_T a_eHalCmd,
	VAL_VOID_T *a_pvInParam,
	VAL_VOID_T *a_pvOutParam
);



#ifdef __cplusplus
}
#endif

#endif /* #ifndef _HAL_API_H_ */
