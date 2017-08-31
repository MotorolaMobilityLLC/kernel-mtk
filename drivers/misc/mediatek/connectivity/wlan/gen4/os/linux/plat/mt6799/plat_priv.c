/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/
#include <linux/threads.h>
#include "precomp.h"
#include "mach/mtk_ppm_api.h"

#define FREQUENCY_M(x)					(x*1000)	/*parameter for mt_ppm_sysboost_freq in kHZ*/
#define CLUSTER_B_CORE_MAX_FREQ			1638		/*E1 is 1638000 KHz, E2 is TBD */
#define CLUSTER_L_CORE_MAX_FREQ			1378		/*E1 is 1378000 KHz  E2 is TBD */

INT_32 kalBoostCpu(IN P_ADAPTER_T prAdapter, IN UINT_32 u4TarPerfLevel, IN UINT_32 u4BoostCpuTh)
{
	BOOLEAN fgBoostCpu = FALSE;

	if (u4TarPerfLevel >= u4BoostCpuTh) {
		fgBoostCpu = TRUE;
		mt_ppm_sysboost_set_core_limit(BOOST_BY_WIFI, CLUSTER_B_LKG, 2, 2);
		mt_ppm_sysboost_set_freq_limit(BOOST_BY_WIFI, CLUSTER_B_LKG,
			FREQUENCY_M(CLUSTER_B_CORE_MAX_FREQ), FREQUENCY_M(CLUSTER_B_CORE_MAX_FREQ));
		mt_ppm_sysboost_set_core_limit(BOOST_BY_WIFI, CLUSTER_L_LKG, 4, 4);
		mt_ppm_sysboost_set_freq_limit(BOOST_BY_WIFI, CLUSTER_L_LKG,
			FREQUENCY_M(CLUSTER_L_CORE_MAX_FREQ), FREQUENCY_M(CLUSTER_L_CORE_MAX_FREQ));
	} else {
		fgBoostCpu = FALSE;
		mt_ppm_sysboost_core(BOOST_BY_WIFI, 0);
		mt_ppm_sysboost_freq(BOOST_BY_WIFI, FREQUENCY_M(0));
	}
	DBGLOG(SW4, WARN, "BoostCpu:%d,TarPerfLevel:%d,u4BoostCpuTh:%d\n",
		fgBoostCpu, u4TarPerfLevel, u4BoostCpuTh);
	return 0;
}
