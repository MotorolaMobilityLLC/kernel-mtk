/****************************************************************************
* This confidential and proprietary software may be used only as authorized *
* by a licensing agreement from ARM Israel.                                 *
* Copyright (C) 2015 ARM Limited or its affiliates. All rights reserved.    *
* The entire notice above must be reproduced on all authorized copies and   *
* copies may only be made to the extent permitted by a licensing agreement  *
* from ARM Israel.                                                          *
*****************************************************************************/

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
