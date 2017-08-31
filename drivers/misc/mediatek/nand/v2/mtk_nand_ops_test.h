#ifndef __MTK_NAND_OPS_TEST_H__
#define __MTK_NAND_OPS_TEST_H__

#include "mtk_nand_ops.h"

/* UNIT TEST RELATED */
/* #define MTK_NAND_CHIP_TEST */
/* #define MTK_NAND_CHIP_DUMP_DATA_TEST */
#define MTK_NAND_CHIP_MULTI_PLANE_TEST
/* #define MTK_NAND_READ_COMPARE */
#define MTK_NAND_PERFORMANCE_TEST
#define MTK_NAND_MARK_BAD_TEST 0
#define MTK_NAND_SIM_ERR 1

extern struct mtk_nand_data_info *data_info;

extern bool mtk_isbad_block(unsigned int block);

void mtk_chip_unit_test(void);

#if MTK_NAND_SIM_ERR
int sim_nand_err(enum operation_types op, int block, int page);
#endif

#endif
