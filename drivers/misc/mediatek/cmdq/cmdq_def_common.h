#ifndef __CMDQ_DEF_COMMON_H__
#define __CMDQ_DEF_COMMON_H__

#include <linux/kernel.h>

typedef enum CMDQ_SCENARIO_ENUM {
	CMDQ_SCENARIO_JPEG_DEC = 0,
	CMDQ_SCENARIO_PRIMARY_DISP = 1,
	CMDQ_SCENARIO_PRIMARY_MEMOUT = 2,
	CMDQ_SCENARIO_PRIMARY_ALL = 3,
	CMDQ_SCENARIO_SUB_DISP = 4,
	CMDQ_SCENARIO_SUB_MEMOUT = 5,
	CMDQ_SCENARIO_SUB_ALL = 6,
	CMDQ_SCENARIO_MHL_DISP = 7,
	CMDQ_SCENARIO_RDMA0_DISP = 8,
	CMDQ_SCENARIO_RDMA0_COLOR0_DISP = 9,
	CMDQ_SCENARIO_RDMA1_DISP = 10,

	/* Trigger loop scenario does not enable HWs */
	CMDQ_SCENARIO_TRIGGER_LOOP = 11,

	/* client from user space, so the cmd buffer is in user space. */
	CMDQ_SCENARIO_USER_MDP = 12,

	CMDQ_SCENARIO_DEBUG = 13,
	CMDQ_SCENARIO_DEBUG_PREFETCH = 14,

	/* ESD check */
	CMDQ_SCENARIO_DISP_ESD_CHECK = 15,
	/* for screen capture to wait for RDMA-done without blocking config thread */
	CMDQ_SCENARIO_DISP_SCREEN_CAPTURE = 16,

	/* notifiy there are some tasks exec done in secure path */
	CMDQ_SCENARIO_SECURE_NOTIFY_LOOP = 17,

	CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH = 18,
	CMDQ_SCENARIO_DISP_SUB_DISABLE_SECURE_PATH = 19,

	/* color path request from kernel */
	CMDQ_SCENARIO_DISP_COLOR = 20,
	/* color path request from user sapce */
	CMDQ_SCENARIO_USER_DISP_COLOR = 21,

	/* [phased out]client from user space, so the cmd buffer is in user space. */
	CMDQ_SCENARIO_USER_SPACE = 22,

	CMDQ_SCENARIO_DISP_MIRROR_MODE = 23,

	CMDQ_SCENARIO_DISP_CONFIG_AAL = 24,
	CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_GAMMA = 25,
	CMDQ_SCENARIO_DISP_CONFIG_SUB_GAMMA = 26,
	CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_DITHER = 27,
	CMDQ_SCENARIO_DISP_CONFIG_SUB_DITHER = 28,
	CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PWM = 29,
	CMDQ_SCENARIO_DISP_CONFIG_SUB_PWM = 30,
	CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PQ = 31,
	CMDQ_SCENARIO_DISP_CONFIG_SUB_PQ = 32,
	CMDQ_SCENARIO_DISP_CONFIG_OD = 33,

	CMDQ_SCENARIO_RDMA2_DISP = 34,

	CMDQ_MAX_SCENARIO_COUNT	/* ALWAYS keep at the end */
} CMDQ_SCENARIO_ENUM;

/* CMDQ Events */
#undef DECLARE_CMDQ_EVENT
#ifdef CMDQ_DVENT_FROM_DTS
#define DECLARE_CMDQ_EVENT(name_struct, val, dts_name) name_struct = val,
typedef enum CMDQ_EVENT_ENUM {
#include "cmdq_event_common.h"
} CMDQ_EVENT_ENUM;
#else
#define DECLARE_CMDQ_EVENT(name_struct, val) name_struct = val,
typedef enum CMDQ_EVENT_ENUM {
#include "cmdq_event.h"
} CMDQ_EVENT_ENUM;
#endif
#undef DECLARE_CMDQ_EVENT

/* Custom "wide" pointer type for 64-bit job handle (pointer to VA) */
typedef unsigned long long cmdqJobHandle_t;
/* Custom "wide" pointer type for 64-bit compatibility. Always cast from uint32_t*. */
typedef unsigned long long cmdqU32Ptr_t;

#define CMDQ_U32_PTR(x) ((uint32_t *)(unsigned long)x)

typedef struct cmdqReadRegStruct {
	uint32_t count;		/* number of entries in regAddresses */
	cmdqU32Ptr_t regAddresses;	/* an array of 32-bit register addresses (uint32_t) */
} cmdqReadRegStruct;

typedef struct cmdqRegValueStruct {
	/* number of entries in result */
	uint32_t count;

	/* array of 32-bit register values (uint32_t). */
	/* in the same order as cmdqReadRegStruct */
	cmdqU32Ptr_t regValues;
} cmdqRegValueStruct;

typedef struct cmdqReadAddressStruct {
	uint32_t count;		/* [IN] number of entries in result. */

	/* [IN] array of physical addresses to read. */
	/* these value must allocated by CMDQ_IOCTL_ALLOC_WRITE_ADDRESS ioctl */
	/*  */
	/* indeed param dmaAddresses should be UNSIGNED LONG type for 64 bit kernel. */
	/* Considering our plartform supports max 4GB RAM(upper-32bit don't care for SW) */
	/* and consistent common code interface, remain uint32_t type. */
	cmdqU32Ptr_t dmaAddresses;

	cmdqU32Ptr_t values;	/* [OUT] uint32_t values that dmaAddresses point into */
} cmdqReadAddressStruct;

/*
 * Secure address metadata:
 * According to handle type, translate handle and replace (_d)th instruciton to
 *     1. sec_addr = hadnle_sec_base_addr(baseHandle) + offset(_b)
 *     2. sec_mva = mva( hadnle_sec_base_addr(baseHandle) + offset(_b) )
 *     3. secure world normal mva = map(baseHandle)
 *        . pass normal mva to parameter baseHandle
 *        . use case: OVL reads from secure and normal buffers at the same time)
 */
typedef enum CMDQ_SEC_ADDR_METADATA_TYPE {
	CMDQ_SAM_H_2_PA = 0,	/* sec handle to sec PA */
	CMDQ_SAM_H_2_MVA = 1,	/* sec handle to sec MVA */
	CMDQ_SAM_NMVA_2_MVA = 2,	/* map normal MVA to secure world */
} CMDQ_SEC_ADDR_METADATA_TYPE;

typedef struct cmdqSecAddrMetadataStruct {
	/* [IN]_d, index of instruction. Update its argB value to real PA/MVA in secure world */
	uint32_t instrIndex;

	CMDQ_SEC_ADDR_METADATA_TYPE type;	/* [IN] addr handle type */
	uint32_t baseHandle;	/* [IN]_h, secure address handle */
	uint32_t offset;	/* [IN]_b, buffser offset to secure handle */
	uint32_t size;		/* buffer size */
	uint32_t port;		/* hw port id (i.e. M4U port id) */
} cmdqSecAddrMetadataStruct;

typedef struct cmdqSecDataStruct {
	bool isSecure;		/* [IN]true for secure command */

	/* address metadata, used to translate secure buffer PA related instruction in secure world */
	uint32_t addrMetadataCount;	/* [IN] count of element in addrList */
	cmdqU32Ptr_t addrMetadatas;	/* [IN] array of cmdqSecAddrMetadataStruct */
	uint32_t addrMetadataMaxCount;	/*[Reserved] */

	uint64_t enginesNeedDAPC;
	uint64_t enginesNeedPortSecurity;

	/* [Reserved] This is for CMDQ driver usage itself. Not for client. */
	int32_t waitCookie;	/* task index in thread's tasklist. -1 for not in tasklist. */
	bool resetExecCnt;	/* reset HW thread in SWd */
} cmdqSecDataStruct;

#ifdef CMDQ_PROFILE_MARKER_SUPPORT
typedef struct cmdqProfileMarkerStruct {
	uint32_t count;
	long long hSlot;	/* i.e. cmdqBackupSlotHandle, physical start address of backup slot */
	cmdqU32Ptr_t tag[CMDQ_MAX_PROFILE_MARKER_IN_TASK];
} cmdqProfileMarkerStruct;
#endif

typedef struct cmdqCommandStruct {
	/* [IN] deprecated. will remove in the future. */
	uint32_t scenario;
	/* [IN] task schedule priority. this is NOT HW thread priority. */
	uint32_t priority;
	/* [IN] bit flag of engines used. */
	uint64_t engineFlag;
	/* [IN] pointer to instruction buffer. Use 64-bit for compatibility. */
	/* This must point to an 64-bit aligned uint32_t array */
	cmdqU32Ptr_t pVABase;
	/* [IN] size of instruction buffer, in bytes. */
	uint32_t blockSize;
	/* [IN] request to read register values at the end of command */
	cmdqReadRegStruct regRequest;
	/* [OUT] register values of regRequest */
	cmdqRegValueStruct regValue;
	/* [IN/OUT] physical addresses to read value */
	cmdqReadAddressStruct readAddress;
	/*[IN] secure execution data */
	cmdqSecDataStruct secData;
	/* [IN] set to non-zero to enable register debug dump. */
	uint32_t debugRegDump;
	/* [Reserved] This is for CMDQ driver usage itself. Not for client. Do not access this field from User Space */
	cmdqU32Ptr_t privateData;
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	cmdqProfileMarkerStruct profileMarker;
#endif
} cmdqCommandStruct;

typedef enum CMDQ_CAP_BITS {
	/* bit 0: TRUE if WFE instruction support is ready. FALSE if we need to POLL instead. */
	CMDQ_CAP_WFE = 0,
} CMDQ_CAP_BITS;


/**
 * reply struct for cmdq_sec_cancel_error_task
 */

typedef struct {
	/* [OUT] */
	bool throwAEE;
	bool hasReset;
	int32_t irqFlag;
	uint32_t errInstr[2];
	uint32_t regValue;
	uint32_t pc;
} cmdqSecCancelTaskResultStruct;

#endif				/* __CMDQ_DEF_COMMON_H__ */
