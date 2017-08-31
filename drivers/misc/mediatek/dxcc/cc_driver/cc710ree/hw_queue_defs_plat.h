/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                     *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  *
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *
* You should have received a copy of the GNU General Public License along    *
* with this program; if not, write to the Free Software Foundation, 	     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.        *
******************************************************************************/

#ifndef __HW_QUEUE_DEFS_PLAT_H__
#define __HW_QUEUE_DEFS_PLAT_H__


/*****************************/
/* Descriptor packing macros */
/*****************************/

#define HW_DESC_PUSH_TO_QUEUE(qid, pDesc) do {        				  \
	LOG_HW_DESC(pDesc);							  \
	HW_DESC_DUMP(pDesc);							  \
	SASI_HAL_WRITE_REGISTER(GET_HW_Q_DESC_WORD_IDX(qid, 0), (pDesc)->word[0]); \
	SASI_HAL_WRITE_REGISTER(GET_HW_Q_DESC_WORD_IDX(qid, 1), (pDesc)->word[1]); \
	SASI_HAL_WRITE_REGISTER(GET_HW_Q_DESC_WORD_IDX(qid, 2), (pDesc)->word[2]); \
	SASI_HAL_WRITE_REGISTER(GET_HW_Q_DESC_WORD_IDX(qid, 3), (pDesc)->word[3]); \
	SASI_HAL_WRITE_REGISTER(GET_HW_Q_DESC_WORD_IDX(qid, 4), (pDesc)->word[4]); \
	wmb();									   \
	SASI_HAL_WRITE_REGISTER(GET_HW_Q_DESC_WORD_IDX(qid, 5), (pDesc)->word[5]); \
} while (0)

#endif /*__HW_QUEUE_DEFS_PLAT_H__*/
