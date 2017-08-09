#ifndef __MMDVFS_MGR_H__
#define __MMDVFS_MGR_H__

#include <aee.h>
#include <mt_smi.h>

#define MMDVFS_LOG_TAG	"MMDVFS"

#define MMDVFSMSG(string, args...) pr_debug("[pid=%d]"string, current->tgid, ##args)
#define MMDVFSMSG2(string, args...) pr_debug(string, ##args)
#define MMDVFSTMP(string, args...) pr_debug("[pid=%d]"string, current->tgid, ##args)
#define MMDVFSERR(string, args...) \
		do {\
			pr_debug("error: "string, ##args); \
			aee_kernel_warning(MMDVFS_LOG_TAG, "error: "string, ##args);  \
		} while (0)

/* MMDVFS extern APIs */
extern void mmdvfs_init(MTK_SMI_BWC_MM_INFO *info);
extern void mmdvfs_handle_cmd(MTK_MMDVFS_CMD *cmd);
extern void mmdvfs_notify_scenario_enter(MTK_SMI_BWC_SCEN scen);
extern void mmdvfs_notify_scenario_exit(MTK_SMI_BWC_SCEN scen);
extern void mmdvfs_notify_scenario_concurrency(unsigned int u4Concurrency);
extern void mmdvfs_mhl_enable(int enable);

/* screen size */
extern unsigned int DISP_GetScreenWidth(void);
extern unsigned int DISP_GetScreenHeight(void);


#define MMSYS_CLK_LOW (0)
#define MMSYS_CLK_HIGH (1)
#define MMSYS_CLK_MEDIUM (2)

#define MMDVFS_EVENT_OVL_SINGLE_LAYER_ENTER 0
#define MMDVFS_EVENT_OVL_SINGLE_LAYER_EXIT 1
#define MMDVFS_EVENT_UI_IDLE_ENTER 2
#define MMDVFS_EVENT_UI_IDLE_EXIT 3

#define MMDVFS_CLIENT_ID_ISP 0

typedef int (*clk_switch_cb)(int ori_mmsys_clk_mode, int update_mmsys_clk_mode);

/* MMDVFS V2 only APIs */
extern int mmdvfs_notify_mmclk_switch_request(int event);
extern int mmdvfs_raise_mmsys_by_mux(void);
extern int mmdvfs_lower_mmsys_by_mux(void);
extern int register_mmclk_switch_cb(clk_switch_cb notify_cb,
clk_switch_cb notify_cb_nolock);
extern int mmdvfs_register_mmclk_switch_cb(clk_switch_cb notify_cb, int mmdvfs_client_id);
extern void dump_mmdvfs_info(void);


/* Extern from other module */
extern MTK_SMI_BWC_SCEN smi_get_current_profile(void);
extern int is_mmdvfs_freq_hopping_disabled(void);
extern int is_mmdvfs_freq_mux_disabled(void);
extern int is_force_max_mmsys_clk(void);
extern int is_force_camera_hpm(void);


#ifdef MMDVFS_STANDALONE
#define vcorefs_request_dvfs_opp(scen, mode) do { \
	MMDVFSMSG("vcorefs_request_dvfs_opp"); \
	MMDVFSMSG("MMDVFS_STANDALONE mode enabled\n"); \
} while (0)

#define fliper_set_bw(BW_THRESHOLD_HIGH) do { \
	MMDVFSMSG("MMDVFS_STANDALONE mode enabled\n"); \
	MMDVFSMSG("fliper_set_bw");\
} while (0)

#define fliper_restore_bw() do {\
	MMDVFSMSG("MMDVFS_STANDALONE mode enabled\n"); \
	MMDVFSMSG("fliper_restore_bw(): fliper normal\n"); \
} while (0)

#endif /* MMDVFS_STANDALONE */

#ifdef MMDVFS_WQHD_1_0V
#include "disp_session.h"
extern int primary_display_switch_mode_for_mmdvfs(int sess_mode, unsigned int session, int blocking);
#endif

#endif				/* __MMDVFS_MGR_H__ */
