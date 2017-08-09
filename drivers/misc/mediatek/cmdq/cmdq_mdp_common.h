#ifndef __CMDQ_MDP_COMMON_H__
#define __CMDQ_MDP_COMMON_H__

#include "cmdq_def.h"

#include <linux/types.h>

typedef int32_t(*CmdqMdpClockOn) (uint64_t engineFlag);

typedef int32_t(*CmdqMdpDumpInfo) (uint64_t engineFlag, int level);

typedef int32_t(*CmdqVEncDumpInfo) (uint64_t engineFlag, int level);

typedef int32_t(*CmdqMdpResetEng) (uint64_t engineFlag);

typedef int32_t(*CmdqMdpClockOff) (uint64_t engineFlag);

typedef const uint32_t(*CmdqMdpRdmaGetRegOffsetSrcAddr) (void);

typedef const uint32_t(*CmdqMdpWrotGetRegOffsetDstAddr) (void);

typedef const uint32_t(*CmdqMdpWdmaGetRegOffsetDstAddr) (void);

typedef void (*CmdqTestcaseClkmgrMdp) (void);

typedef struct cmdqMDPFuncStruct {
	CmdqMdpClockOn mdpClockOn;
	CmdqMdpDumpInfo mdpDumpInfo;
	CmdqVEncDumpInfo vEncDumpInfo;
	CmdqMdpResetEng mdpResetEng;
	CmdqMdpClockOff mdpClockOff;
	CmdqMdpRdmaGetRegOffsetSrcAddr rdmaGetRegOffsetSrcAddr;
	CmdqMdpWrotGetRegOffsetDstAddr wrotGetRegOffsetDstAddr;
	CmdqMdpWdmaGetRegOffsetDstAddr wdmaGetRegOffsetDstAddr;
	CmdqTestcaseClkmgrMdp testcaseClkmgrMdp;
} cmdqMDPFuncStruct;

#ifdef __cplusplus
extern "C" {
#endif
	void cmdq_mdp_virtual_function_setting(void);
	cmdqMDPFuncStruct *cmdq_mdp_get_func(void);

	void cmdq_mdp_enable(uint64_t engineFlag, CMDQ_ENG_ENUM engine);

	int cmdq_mdp_loop_reset(CMDQ_ENG_ENUM engine,
				const unsigned long resetReg,
				const unsigned long resetStateReg,
				const uint32_t resetMask,
				const uint32_t resetValue, const bool pollInitResult);

	void cmdq_mdp_loop_off(CMDQ_ENG_ENUM engine,
			       const unsigned long resetReg,
			       const unsigned long resetStateReg,
			       const uint32_t resetMask,
			       const uint32_t resetValue, const bool pollInitResult);

	void cmdq_mdp_dump_venc(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rdma(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rsz(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rot(const unsigned long base, const char *label);
	void cmdq_mdp_dump_tdshp(const unsigned long base, const char *label);
	void cmdq_mdp_dump_wdma(const unsigned long base, const char *label);

/**************************************************************************************/
/*******************                    Platform dependent function                    ********************/
/**************************************************************************************/

	int32_t cmdqMdpClockOn(uint64_t engineFlag);

	int32_t cmdqMdpDumpInfo(uint64_t engineFlag, int level);

	int32_t cmdqVEncDumpInfo(uint64_t engineFlag, int level);

	int32_t cmdqMdpResetEng(uint64_t engineFlag);

	int32_t cmdqMdpClockOff(uint64_t engineFlag);

	const uint32_t cmdq_mdp_rdma_get_reg_offset_src_addr(void);
	const uint32_t cmdq_mdp_wrot_get_reg_offset_dst_addr(void);
	const uint32_t cmdq_mdp_wdma_get_reg_offset_dst_addr(void);

	void testcase_clkmgr_mdp(void);

#ifdef __cplusplus
}
#endif
#endif				/* __CMDQ_MDP_COMMON_H__ */
