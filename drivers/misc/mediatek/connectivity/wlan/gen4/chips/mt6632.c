/*! \file   mt6632.c
*    \brief  Internal driver stack will export the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported from MediaTek 802.11 Wireless
*    LAN driver stack to GLUE Layer.
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

#include "mt6632.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

ECO_INFO_T mt6632_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0xA},	/* E1 */
	{0x00, 0x00, 0xA},	/* E2 */
	{0x10, 0x10, 0xA},	/* E3 */

	{0x00, 0x00, 0x0}	/* End of table */
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/* Litien code refine to support multi chip */
struct mt66xx_chip_info mt66xx_chip_info_mt6632 = {
	.chip_id = MT6632_CHIP_ID,
	.sw_sync0 = MT6632_SW_SYNC0,
	.sw_ready_bit_offset = MT6632_SW_SYNC0_RDY_OFFSET,
	.patch_addr = MT6632_PATCH_START_ADDR,
	.is_pcie_32dw_read = MT6632_IS_PCIE_32DW_READ, /* Litien */
	.eco_info = mt6632_eco_table,
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt6632 = {
	.chip_info = &mt66xx_chip_info_mt6632,
};
