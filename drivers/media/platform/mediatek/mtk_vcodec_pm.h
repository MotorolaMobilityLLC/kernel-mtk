/*
* Copyright (c) 2015 MediaTek Inc.
* Author: Tiffany Lin <tiffany.lin@mediatek.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef _MEDIA_PLATFORM_MEDIATEK_MTK_VCODEC_PM_H_
#define _MEDIA_PLATFORM_MEDIATEK_MTK_VCODEC_PM_H_

#include "mtk_vcodec_drv.h"

int mtk_vcodec_init_dec_pm(struct mtk_vcodec_dev *dev);
void mtk_vcodec_release_dec_pm(struct mtk_vcodec_dev *dev);
int mtk_vcodec_init_enc_pm(struct mtk_vcodec_dev *dev);
void mtk_vcodec_release_enc_pm(struct mtk_vcodec_dev *dev);

void mtk_vcodec_dec_pw_on(void);
void mtk_vcodec_dec_pw_off(void);
void mtk_vcodec_dec_clock_on(void);
void mtk_vcodec_dec_clock_off(void);
void mtk_vcodec_enc_pw_on(void);
void mtk_vcodec_enc_pw_off(void);
void mtk_vcodec_enc_clock_on(void);
void mtk_vcodec_enc_clock_off(void);

#endif /* _MEDIA_PLATFORM_MEDIATEK_MTK_VCODEC_PM_H_ */
