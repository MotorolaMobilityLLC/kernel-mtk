/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef	__MTK_COMMON_H__
#define	__MTK_COMMON_H__

/* MTK device	context	driver private data	*/
struct mtk_config	{
	/* mtk mfg mapped	register */
	void __iomem *mfg_register;

	/* main	clock	for	gpu	*/
	struct clk *clk_mfg;
	/* smi clock */
	struct clk *clk_smi_common;
	/* mfg MTCMOS	*/
	struct clk *clk_mfg_scp;
	/* display MTCMOS	*/
	struct clk *clk_display_scp;
};

#endif /*	__MTK_COMMON_H__ */
