/*! \file   mt7668.c
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

#include "mt7668.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

ECO_INFO_T mt7668_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0xA},	/* E1 */

	{0x00, 0x00, 0x0}	/* End of table */
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

VOID
mt7668ConstructFirmwarePrio(P_GLUE_INFO_T prGlueInfo, PPUINT_8 apucNameTable,
PPUINT_8 apucName, PUINT_8 pucNameIdx, UINT_8 ucMaxNameIdx)
{
	struct mt66xx_chip_info *prChipInfo = prGlueInfo->prAdapter->chip_info;
	UINT_32 chip_id = prChipInfo->chip_id;
	UINT_8 sub_idx = 0;

	for (sub_idx = 0; apucNameTable[sub_idx]; sub_idx++) {
		if (((*pucNameIdx) + 3) < ucMaxNameIdx) {
			/* Type 1. WIFI_RAM_CODE_MTxxxx.bin */
			snprintf(*(apucName + (*pucNameIdx)), CFG_FW_NAME_MAX_LEN, "%s%x.bin",
					apucNameTable[sub_idx], chip_id);
			(*pucNameIdx) += 1;

			/* Type 2. WIFI_RAM_CODE_MTxxxx */
			snprintf(*(apucName + (*pucNameIdx)), CFG_FW_NAME_MAX_LEN, "%s%x",
					apucNameTable[sub_idx], chip_id);
			(*pucNameIdx) += 1;

			/* Type 3. WIFI_RAM_CODE_MTxxxx_Ex.bin */
			snprintf(*(apucName + (*pucNameIdx)), CFG_FW_NAME_MAX_LEN, "%s%x_E%u.bin",
					apucNameTable[sub_idx], chip_id,
					wlanGetEcoVersion(prGlueInfo->prAdapter));
			(*pucNameIdx) += 1;

			/* Type 4. WIFI_RAM_CODE_MTxxxx_Ex */
			snprintf(*(apucName + (*pucNameIdx)), CFG_FW_NAME_MAX_LEN, "%s%x_E%u",
					apucNameTable[sub_idx], chip_id,
					wlanGetEcoVersion(prGlueInfo->prAdapter));
			(*pucNameIdx) += 1;
		} else {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR, "kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
		}
	}
}

/* Litien code refine to support multi chip */
struct mt66xx_chip_info mt66xx_chip_info_mt7668 = {
	.chip_id = MT7668_CHIP_ID,
	.sw_sync0 = MT7668_SW_SYNC0,
	.sw_ready_bit_offset = MT7668_SW_SYNC0_RDY_OFFSET,
	.patch_addr = MT7668_PATCH_START_ADDR,
	.is_pcie_32dw_read = MT7668_IS_PCIE_32DW_READ, /* Litien */
	.eco_info = mt7668_eco_table,
	.constructFirmwarePrio = mt7668ConstructFirmwarePrio,
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt7668 = {
	.chip_info = &mt66xx_chip_info_mt7668,
};

