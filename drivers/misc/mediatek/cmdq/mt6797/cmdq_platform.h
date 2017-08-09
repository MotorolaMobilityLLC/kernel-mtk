#ifndef __CMDQ_PLATFORM_H__
#define __CMDQ_PLATFORM_H__

/* platform dependent utilities, format: cmdq_{util_type}_{name} */

#include "cmdq_def.h"

#define CMDQ_SPECIAL_SUBSYS_ADDR 99

/* use to generate [CMDQ_ENGINE_ENUM_id and name] mapping for status print */
#define CMDQ_FOREACH_MODULE_PRINT(ACTION)\
{		\
ACTION(CMDQ_ENG_ISP_IMGI,   ISP_IMGI)	\
ACTION(CMDQ_ENG_MDP_RDMA0,  MDP_RDMA)	\
ACTION(CMDQ_ENG_MDP_RSZ0,   MDP_RSZ0)	\
ACTION(CMDQ_ENG_MDP_RSZ1,   MDP_RSZ1)	\
ACTION(CMDQ_ENG_MDP_TDSHP0, MDP_TDSHP)	\
ACTION(CMDQ_ENG_MDP_WROT0,  MDP_WROT)	\
ACTION(CMDQ_ENG_MDP_WDMA,   MDP_WDMA)	\
}

#ifdef __cplusplus
extern "C" {
#endif
	void cmdq_platform_function_setting(void);

#ifdef __cplusplus
}
#endif
#endif				/* __CMDQ_PLATFORM_H__ */
