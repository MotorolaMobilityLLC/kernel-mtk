#include "mach/mt_ppm_api.h"
#include "gl_typedef.h"
INT32 kalBoostCpu(UINT_32 core_num)
{
	pr_warn("enter kalBoostCpu, core_num:%d\n", core_num);
	mt_ppm_sysboost_core(BOOST_BY_WIFI, core_num);
	return 0;
}
