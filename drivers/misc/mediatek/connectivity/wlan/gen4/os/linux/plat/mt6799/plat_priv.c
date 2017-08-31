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
#include <linux/sched.h>
#include "precomp.h"
#include "mach/mtk_ppm_api.h"

#define FREQUENCY_M(x)					(x * 1000)	/*parameter for mt_ppm_sysboost_freq in kHZ*/
#define MT_PPM_DEFAULT					(-1)

INT_32 kalBoostCpu(IN P_ADAPTER_T prAdapter, IN UINT_32 u4TarPerfLevel, IN UINT_32 u4BoostCpuTh)
{
	BOOLEAN fgBoostCpu = FALSE;
	cpumask_t rMainCpuMask, rRxCpuMask, rHifCpuMask;
	INT_32 i4MinCoreLL, i4MinCoreL, i4MinCoreB;
	INT_32 i4MinFreqLL, i4MinFreqL, i4MinFreqB;

	/* Reset to default */
	i4MinCoreLL = MT_PPM_DEFAULT;
	i4MinCoreL = MT_PPM_DEFAULT;
	i4MinCoreB = MT_PPM_DEFAULT;
	i4MinFreqLL = MT_PPM_DEFAULT;
	i4MinFreqL = MT_PPM_DEFAULT;
	i4MinFreqB = MT_PPM_DEFAULT;

	cpumask_clear(&rMainCpuMask);
	cpumask_clear(&rRxCpuMask);
	cpumask_clear(&rHifCpuMask);

	cpumask_setall(&rMainCpuMask);
	cpumask_setall(&rRxCpuMask);
	cpumask_setall(&rHifCpuMask);

	if (u4TarPerfLevel >= u4BoostCpuTh) {
		fgBoostCpu = TRUE;

		switch (u4TarPerfLevel) {
		case 1:
			i4MinCoreL = 1;
			i4MinFreqL = FREQUENCY_M(600);
			break;

		case 2:
		case 3:
			i4MinCoreB = 1;
			i4MinCoreL = 2;
			i4MinCoreLL = 3;

			i4MinFreqB = FREQUENCY_M(2300);
			i4MinFreqL = FREQUENCY_M(2000);
			i4MinFreqLL = FREQUENCY_M(1700);
			break;

		case 4:
		case 5:
		case 6:
			i4MinCoreB = 2;
			i4MinCoreL = 2;
			i4MinCoreLL = 4;

			i4MinFreqB = FREQUENCY_M(2400);
			i4MinFreqL = FREQUENCY_M(2110);
			i4MinFreqLL = FREQUENCY_M(1800);

			cpumask_clear(&rMainCpuMask);
			cpumask_clear(&rRxCpuMask);
			cpumask_clear(&rHifCpuMask);

			cpumask_set_cpu(4, &rMainCpuMask);
			cpumask_set_cpu(5, &rMainCpuMask);
			cpumask_set_cpu(6, &rMainCpuMask);
			cpumask_set_cpu(7, &rMainCpuMask);

			cpumask_set_cpu(4, &rRxCpuMask);
			cpumask_set_cpu(5, &rRxCpuMask);
			cpumask_set_cpu(6, &rRxCpuMask);
			cpumask_set_cpu(7, &rRxCpuMask);
			cpumask_set_cpu(8, &rRxCpuMask);
			cpumask_set_cpu(9, &rRxCpuMask);

			cpumask_set_cpu(8, &rHifCpuMask);
			cpumask_set_cpu(9, &rHifCpuMask);
			break;

		case 7:
		case 8:
		case 9:
		case 10:
			i4MinCoreB = 2;
			i4MinCoreL = 4;
			i4MinCoreLL = 4;

			i4MinFreqB = FREQUENCY_M(2700);
			i4MinFreqL = FREQUENCY_M(2300);
			i4MinFreqLL = FREQUENCY_M(2000);

			cpumask_clear(&rMainCpuMask);
			cpumask_clear(&rRxCpuMask);
			cpumask_clear(&rHifCpuMask);

			cpumask_set_cpu(4, &rMainCpuMask);
			cpumask_set_cpu(5, &rMainCpuMask);
			cpumask_set_cpu(6, &rMainCpuMask);
			cpumask_set_cpu(7, &rMainCpuMask);

			cpumask_set_cpu(4, &rRxCpuMask);
			cpumask_set_cpu(5, &rRxCpuMask);
			cpumask_set_cpu(6, &rRxCpuMask);
			cpumask_set_cpu(7, &rRxCpuMask);
			cpumask_set_cpu(8, &rRxCpuMask);
			cpumask_set_cpu(9, &rRxCpuMask);

			cpumask_set_cpu(8, &rHifCpuMask);
			cpumask_set_cpu(9, &rHifCpuMask);
			break;

		case 0:
		default:
			fgBoostCpu = FALSE;

			break;
		}
	}

	/* Set B core */
	mt_ppm_sysboost_set_core_limit(BOOST_BY_WIFI, CLUSTER_B_LKG, i4MinCoreB, MT_PPM_DEFAULT);
	mt_ppm_sysboost_set_freq_limit(BOOST_BY_WIFI, CLUSTER_B_LKG, i4MinFreqB, MT_PPM_DEFAULT);

	/* Set L core */
	mt_ppm_sysboost_set_core_limit(BOOST_BY_WIFI, CLUSTER_L_LKG, i4MinCoreL, MT_PPM_DEFAULT);
	mt_ppm_sysboost_set_freq_limit(BOOST_BY_WIFI, CLUSTER_L_LKG, i4MinFreqL, MT_PPM_DEFAULT);

	/* Set LL core */
	mt_ppm_sysboost_set_core_limit(BOOST_BY_WIFI, CLUSTER_LL_LKG, i4MinCoreLL, MT_PPM_DEFAULT);
	mt_ppm_sysboost_set_freq_limit(BOOST_BY_WIFI, CLUSTER_LL_LKG, i4MinFreqLL, MT_PPM_DEFAULT);

	/* Set affinity */
	set_cpus_allowed_ptr(prAdapter->prGlueInfo->main_thread, &rMainCpuMask);
	set_cpus_allowed_ptr(prAdapter->prGlueInfo->rx_thread, &rRxCpuMask);
	set_cpus_allowed_ptr(prAdapter->prGlueInfo->hif_thread, &rHifCpuMask);

	DBGLOG(SW4, TRACE, "BoostCpu:%d, TarPerfLevel:%d, u4BoostCpuTh:%d\n",
		fgBoostCpu, u4TarPerfLevel, u4BoostCpuTh);

	return 0;
}

