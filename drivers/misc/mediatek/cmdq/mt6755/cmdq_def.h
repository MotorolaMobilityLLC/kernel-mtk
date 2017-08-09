#ifndef __CMDQ_DEF_H__
#define __CMDQ_DEF_H__

#ifdef CONFIG_OF
#define CMDQ_OF_SUPPORT		/* enable device tree support */

/* Load HW event from device tree */
/* #define CMDQ_DVENT_FROM_DTS */
#else
#undef  CMDQ_OF_SUPPORT
#endif

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
#define CMDQ_SECURE_PATH_SUPPORT
/* #define CMDQ_SECURE_PATH_NORMAL_IRQ		(1) */
#endif

/* CMDQ capability */
#define CMDQ_GPR_SUPPORT

#define CMDQ_PROFILE_MARKER_SUPPORT
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
#define CMDQ_MAX_PROFILE_MARKER_IN_TASK (5)
#endif

#define CMDQ_DUMP_GIC (1)	/* TODO: enable when GIC porting done */
/* #define CMDQ_PROFILE_MMP (0) */

#define CMDQ_DUMP_FIRSTERROR
#define CMDQ_INSTRUCTION_COUNT

#define CMDQ_INIT_FREE_TASK_COUNT       (8)
#define CMDQ_MAX_THREAD_COUNT           (16)
#define CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT (6)	/* Thread that are high-priority (display threads) */
#define CMDQ_MIN_SECURE_THREAD_ID		(12)
#define CMDQ_MAX_SECURE_THREAD_COUNT	(3)
#define CMDQ_MAX_RECORD_COUNT           (1024)
#define CMDQ_MAX_ERROR_COUNT            (2)
#define CMDQ_MAX_RETRY_COUNT            (1)
#define CMDQ_MAX_TASK_IN_THREAD         (16)
#define CMDQ_MAX_READ_SLOT_COUNT        (4)

#define CMDQ_MAX_PREFETCH_INSTUCTION    (240)	/* Maximum prefetch buffer size, in instructions. */
#define CMDQ_INITIAL_CMD_BLOCK_SIZE     (PAGE_SIZE)
#define CMDQ_EMERGENCY_BLOCK_SIZE       (256 * 1024)	/* 256 KB command buffer */
#define CMDQ_EMERGENCY_BLOCK_COUNT      (4)
#define CMDQ_INST_SIZE                  (2 * sizeof(uint32_t))	/* instruction is 64-bit */

#define CMDQ_MAX_LOOP_COUNT             (1000000)
#define CMDQ_MAX_INST_CYCLE             (27)
#define CMDQ_MIN_AGE_VALUE              (5)
#define CMDQ_MAX_ERROR_SIZE             (8 * 1024)

#define CMDQ_MAX_COOKIE_VALUE           (0xFFFFFFFF)	/* max value of CMDQ_THR_EXEC_CMD_CNT (value starts from 0) */
#define CMDQ_ARG_A_SUBSYS_MASK          (0x001F0000)

#ifdef CONFIG_MTK_FPGA
#define CMDQ_DEFAULT_TIMEOUT_MS         (10000)
#else
#define CMDQ_DEFAULT_TIMEOUT_MS         (1000)
#endif

#define CMDQ_ACQUIRE_THREAD_TIMEOUT_MS  (2000)
#define CMDQ_PREDUMP_TIMEOUT_MS         (200)
#define CMDQ_PREDUMP_RETRY_COUNT        (5)

#define CMDQ_INVALID_THREAD             (-1)

#define CMDQ_DRIVER_DEVICE_NAME         "mtk_cmdq"

#ifndef CONFIG_MTK_FPGA
#define CMDQ_PWR_AWARE		/* FPGA does not have ClkMgr */
#else
#undef CMDQ_PWR_AWARE
#endif

typedef enum CMDQ_HW_THREAD_PRIORITY_ENUM {
	CMDQ_THR_PRIO_NORMAL = 0,	/* nomral (lowest) priority */
	CMDQ_THR_PRIO_DISPLAY_TRIGGER = 1,	/* trigger loop (enables display mutex) */

	CMDQ_THR_PRIO_DISPLAY_ESD = 2,	/* display ESD check (every 2 secs) */
	CMDQ_THR_PRIO_DISPLAY_CONFIG = 3,	/* display config (every frame) */

	CMDQ_THR_PRIO_MAX = 7,	/* maximum possible priority */
} CMDQ_HW_THREAD_PRIORITY_ENUM;

typedef enum CMDQ_DATA_REGISTER_ENUM {
	/* Value Reg, we use 32-bit */
	/* Address Reg, we use 64-bit */
	/* Note that R0-R15 and P0-P7 actullay share same memory */
	/* and R1 cannot be used. */

	CMDQ_DATA_REG_JPEG = 0x00,	/* R0 */
	CMDQ_DATA_REG_JPEG_DST = 0x11,	/* P1 */

	CMDQ_DATA_REG_PQ_COLOR = 0x04,	/* R4 */
	CMDQ_DATA_REG_PQ_COLOR_DST = 0x13,	/* P3 */

	CMDQ_DATA_REG_2D_SHARPNESS_0 = 0x05,	/* R5 */
	CMDQ_DATA_REG_2D_SHARPNESS_0_DST = 0x14,	/* P4 */

	CMDQ_DATA_REG_2D_SHARPNESS_1 = 0x0a,	/* R10 */
	CMDQ_DATA_REG_2D_SHARPNESS_1_DST = 0x16,	/* P6 */

	CMDQ_DATA_REG_DEBUG = 0x0b,	/* R11 */
	CMDQ_DATA_REG_DEBUG_DST = 0x17,	/* P7 */

	/* sentinel value for invalid register ID */
	CMDQ_DATA_REG_INVALID = -1,
} CMDQ_DATA_REGISTER_ENUM;

typedef enum CMDQ_ENG_ENUM {
	/* ISP */
	CMDQ_ENG_ISP_IMGI = 0,
	CMDQ_ENG_ISP_IMGO,	/* 1 */
	CMDQ_ENG_ISP_IMG2O,	/* 2 */

	/* MDP */
	CMDQ_ENG_MDP_CAMIN,	/* 3 */
	CMDQ_ENG_MDP_RDMA0,	/* 4 */
	CMDQ_ENG_MDP_RSZ0,	/* 5 */
	CMDQ_ENG_MDP_RSZ1,	/* 6 */
	CMDQ_ENG_MDP_TDSHP0,	/* 7 */
	CMDQ_ENG_MDP_WROT0,	/* 8 */
	CMDQ_ENG_MDP_WDMA,	/* 9 */
	CMDQ_ENG_MDP_COLOR,	/* 10 */

	/* JPEG & VENC */
	CMDQ_ENG_JPEG_ENC,	/* 11 */
	CMDQ_ENG_VIDEO_ENC,	/* 12 */
	CMDQ_ENG_JPEG_DEC,	/* 13 */
	CMDQ_ENG_JPEG_REMDC,	/* 14 */

	/* DISP */
	CMDQ_ENG_DISP_AAL,	/* 15 */
	CMDQ_ENG_DISP_COLOR0,	/* 16 */
	CMDQ_ENG_DISP_RDMA0,	/* 17 */
	CMDQ_ENG_DISP_RDMA1,	/* 18 */
	CMDQ_ENG_DISP_WDMA0,	/* 19 */
	CMDQ_ENG_DISP_WDMA1,	/* 20 */
	CMDQ_ENG_DISP_OVL0,	/* 21 */
	CMDQ_ENG_DISP_OVL1,	/* 22 */
	CMDQ_ENG_DISP_GAMMA,	/* 23 */
	CMDQ_ENG_DISP_DSI0_VDO,	/* 24 */
	CMDQ_ENG_DISP_DSI0_CMD,	/* 25 */
	CMDQ_ENG_DISP_DSI0,	/* 26 */
	CMDQ_ENG_DISP_DPI,	/* 27 */

	/* temp: CMDQ internal usage */
	CMDQ_ENG_CMDQ,
	CMDQ_ENG_DISP_MUTEX,
	CMDQ_ENG_MMSYS_CONFIG,

	CMDQ_MAX_ENGINE_COUNT	/* ALWAYS keep at the end */
} CMDQ_ENG_ENUM;

#include "../cmdq_def_common.h"

#endif				/* __CMDQ_DEF_H__ */
