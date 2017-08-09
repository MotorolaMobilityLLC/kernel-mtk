#include "cmdq_core.h"
#include "cmdq_virtual.h"
#include "cmdq_reg.h"
#include "cmdq_struct.h"
#include "cmdq_device.h"
#include "cmdq_record.h"
#include "cmdq_sec.h"
#ifdef CMDQ_PROFILE_MMP
#include "cmdq_mmp.h"
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/memory.h>
#include <linux/ftrace.h>
#ifdef CMDQ_MET_READY
#include <linux/met_drv.h>
#endif
#include <linux/seq_file.h>
#include <linux/kthread.h>

#ifndef CMDQ_OF_SUPPORT
#include <mach/mt_irq.h>
#include "ddp_reg.h"
#endif

#ifdef CMDQ_OF_SUPPORT
#define MMSYS_CONFIG_BASE cmdq_dev_get_module_base_VA_MMSYS_CONFIG()
#else
#include <mach/mt_reg_base.h>
#endif

/* #define CMDQ_PROFILE_COMMAND_TRIGGER_LOOP */
/* #define CMDQ_APPEND_WITHOUT_SUSPEND */
/* #define CMDQ_ENABLE_BUS_ULTRA */

#define CMDQ_GET_COOKIE_CNT(thread) (CMDQ_REG_GET32(CMDQ_THR_EXEC_CNT(thread)) & CMDQ_MAX_COOKIE_VALUE)
#define CMDQ_SYNC_TOKEN_APPEND_THR(id)     (CMDQ_SYNC_TOKEN_APPEND_THR0 + id)

/* use mutex because we don't access task list in IRQ */
/* and we may allocate memory when create list items */
static DEFINE_MUTEX(gCmdqTaskMutex);
static DEFINE_MUTEX(gCmdqSaveBufferMutex);
/* static DEFINE_MUTEX(gCmdqWriteAddrMutex); */
static DEFINE_SPINLOCK(gCmdqWriteAddrLock);

#if defined(CMDQ_SECURE_PATH_SUPPORT) && !defined(CMDQ_SECURE_PATH_NORMAL_IRQ)
/* ensure atomic start/stop notify loop*/
static DEFINE_MUTEX(gCmdqNotifyLoopMutex);
#endif

/* t-base(secure OS) doesn't allow entry secure world in ISR context, */
/* but M4U has to restore lab0 register in secure world when enable lab 0 first time. */
/* HACK: use m4u_larb0_enable() to lab0 on/off to ensure larb0 restore and clock on/off sequence */
/* HACK: use gCmdqClockMutex to ensure acquire/release thread and enable/disable clock sequence */
static DEFINE_MUTEX(gCmdqClockMutex);

/* These may access in IRQ so use spin lock. */
static DEFINE_SPINLOCK(gCmdqThreadLock);
static atomic_t gCmdqThreadUsage;
static atomic_t gSMIThreadUsage;
static bool gCmdqSuspended;
static DEFINE_SPINLOCK(gCmdqExecLock);
static DEFINE_SPINLOCK(gCmdqRecordLock);
static DEFINE_MUTEX(gCmdqResourceMutex);

/* Emergency buffer when fail to allocate memory. */
static DEFINE_SPINLOCK(gCmdqAllocLock);
static EmergencyBufferStruct gCmdqEmergencyBuffer[CMDQ_EMERGENCY_BLOCK_COUNT];

/* The main context structure */
static wait_queue_head_t gCmdWaitQueue[CMDQ_MAX_THREAD_COUNT];	/* task done notification */
static wait_queue_head_t gCmdqThreadDispatchQueue;	/* thread acquire notification */

static ContextStruct gCmdqContext;
static CmdqCBkStruct gCmdqGroupCallback[CMDQ_MAX_GROUP_COUNT];
static CmdqDebugCBkStruct gCmdqDebugCallback;

#ifdef CMDQ_DUMP_FIRSTERROR
DumpFirstErrorStruct gCmdqFirstError;
#endif

static DumpCommandBufferStruct gCmdqBufferDump;

/* Debug or test usage */
/* enable this option only when test emergency buffer*/
/* #define CMDQ_TEST_EMERGENCY_BUFFER */
#ifdef CMDQ_TEST_EMERGENCY_BUFFER
static atomic_t gCmdqDebugForceUseEmergencyBuffer = ATOMIC_INIT(0);
#endif

/* use to generate [CMDQ_ENGINE_ENUM_id and name] mapping for status print */
#define CMDQ_FOREACH_MODULE_PRINT(ACTION)\
{		\
ACTION(CMDQ_ENG_ISP_IMGI,   ISP_IMGI)	\
ACTION(CMDQ_ENG_MDP_RDMA0,  MDP_RDMA0)	\
ACTION(CMDQ_ENG_MDP_RDMA1,  MDP_RDMA1)	\
ACTION(CMDQ_ENG_MDP_RSZ0,   MDP_RSZ0)	\
ACTION(CMDQ_ENG_MDP_RSZ1,   MDP_RSZ1)	\
ACTION(CMDQ_ENG_MDP_RSZ2,   MDP_RSZ2)	\
ACTION(CMDQ_ENG_MDP_TDSHP0, MDP_TDSHP0)	\
ACTION(CMDQ_ENG_MDP_TDSHP1, MDP_TDSHP1)	\
ACTION(CMDQ_ENG_MDP_COLOR0, MDP_COLOR0) \
ACTION(CMDQ_ENG_MDP_WROT0,  MDP_WROT0)	\
ACTION(CMDQ_ENG_MDP_WROT1,  MDP_WROT1)	\
ACTION(CMDQ_ENG_MDP_WDMA,   MDP_WDMA)	\
}

static const uint64_t gCmdqEngineGroupBits[CMDQ_MAX_GROUP_COUNT] = {
	CMDQ_ENG_ISP_GROUP_BITS,
	CMDQ_ENG_MDP_GROUP_BITS,
	CMDQ_ENG_DISP_GROUP_BITS,
	CMDQ_ENG_JPEG_GROUP_BITS,
	CMDQ_ENG_VENC_GROUP_BITS,
	CMDQ_ENG_DPE_GROUP_BITS
};

static cmdqDTSDataStruct gCmdqDtsData;

const uint32_t cmdq_core_max_task_in_thread(int32_t thread)
{
	int32_t maxTaskNUM = CMDQ_MAX_TASK_IN_THREAD;

#ifdef CMDQ_SECURE_PATH_SUPPORT
	if (true == cmdq_get_func()->isSecureThread(thread))
		maxTaskNUM = CMDQ_MAX_TASK_IN_SECURE_THREAD;
#endif
	return maxTaskNUM;
}

/* Use CMDQ as Resource Manager */
void cmdq_core_unlock_resource(struct work_struct *workItem)
{
	struct ResourceUnitStruct *pResource = NULL;
	struct delayed_work *delayedWorkItem = NULL;
	int32_t status = 0;

	delayedWorkItem = container_of(workItem, struct delayed_work, work);
	pResource = container_of(delayedWorkItem, struct ResourceUnitStruct, delayCheckWork);

	mutex_lock(&gCmdqResourceMutex);

	CMDQ_MSG("[Res] unlock resource with engine: 0x%016llx\n", pResource->engine);
	if (pResource->used) {
		pResource->used = false;
		pResource->delaying = false;
		/* delay time is reached and unlock resource */
		if (NULL == pResource->availableCB) {
			/* print error message */
			CMDQ_ERR("[Res]: available CB func is NULL, event:%d\n", pResource->lockEvent);
		} else {
			/* before call callback, release lock at first */
			mutex_unlock(&gCmdqResourceMutex);
			status =
				pResource->availableCB(pResource->lockEvent);
			mutex_lock(&gCmdqResourceMutex);

			if (status < 0) {
				/* Error status print */
				CMDQ_ERR("[Res]: available CB (%d) return fail:%d\n",
							pResource->lockEvent, status);
			}
		}
	}
	mutex_unlock(&gCmdqResourceMutex);
}

void cmdq_core_init_resource(uint64_t engineFlag, CMDQ_EVENT_ENUM resourceEvent)
{
	struct ResourceUnitStruct *pResource;

	pResource = kzalloc(sizeof(ResourceUnitStruct), GFP_KERNEL);
	if (pResource) {
		pResource->engine = engineFlag;
		pResource->lockEvent = resourceEvent;
		INIT_DELAYED_WORK(&pResource->delayCheckWork, cmdq_core_unlock_resource);
		INIT_LIST_HEAD(&(pResource->listEntry));
		list_add_tail(&(pResource->listEntry), &gCmdqContext.resourceList);
	}
}

/* engineFlag: task original engineFlag */
/* enginesNotUsed: flag which indicate Not Used engine after release task */
void cmdq_core_delay_check_unlock(uint64_t engineFlag, const uint64_t enginesNotUsed)
{
	/* Check engine in enginesNotUsed */
	struct ResourceUnitStruct *pResource = NULL;
	struct list_head *p = NULL;

	CMDQ_MSG("[Res] delay check with engine: 0x%016llx\n", enginesNotUsed);
	list_for_each(p, &gCmdqContext.resourceList) {
		pResource = list_entry(p, struct ResourceUnitStruct, listEntry);
		if (enginesNotUsed & pResource->engine) {
			mutex_lock(&gCmdqResourceMutex);
			CMDQ_MSG("[Res] start delay check resource with engine: 0x%016llx\n", pResource->engine);
			/* find matched engine become not used*/
			pResource->delay = sched_clock();

			if (!pResource->used) {
				/* resource is not used but we got engine is released! */
				/* log as error and still continue */
				CMDQ_ERR("[Res]: resource will delay but not used, engine: 0x%016llx\n",
					pResource->engine);
			}

			/* Cancel previous delay task if existed */
			if (pResource->delaying) {
				cancel_delayed_work(&pResource->delayCheckWork);
				CMDQ_MSG("[Res] after cancel delay check resource with engine: 0x%016llx\n",
					pResource->engine);
			}

			/* Start a new delay task */
			queue_delayed_work(gCmdqContext.resourceCheckWQ,
				&pResource->delayCheckWork, CMDQ_DELAY_RELEASE_RESOURCE_MS);
			pResource->delaying = true;
			mutex_unlock(&gCmdqResourceMutex);
		}
	}
}

cmdqDTSDataStruct *cmdq_core_get_whole_DTS_Data(void)
{
	return &gCmdqDtsData;
}

void cmdq_core_init_DTS_data(void)
{
	uint32_t i;

	memset(&(gCmdqDtsData), 0x0, sizeof(gCmdqDtsData));

	for (i = 0; i < CMDQ_SYNC_TOKEN_MAX; i++) {
		if (i <= CMDQ_MAX_HW_EVENT_COUNT) {
			/* GCE HW evevt */
			gCmdqDtsData.eventTable[i] = CMDQ_SYNC_TOKEN_INVALID - 1 - i;
		} else {
			/* GCE SW evevt */
			gCmdqDtsData.eventTable[i] = i;
		}
	}
}

void cmdq_core_set_event_table(CMDQ_EVENT_ENUM event, const int32_t value)
{
	if (event >= 0 && event < CMDQ_SYNC_TOKEN_MAX)
		gCmdqDtsData.eventTable[event] = value;
}

const int32_t cmdq_core_get_event_value(CMDQ_EVENT_ENUM event)
{
	if (event < 0 || event >= CMDQ_SYNC_TOKEN_MAX)
		return -EINVAL;

	return gCmdqDtsData.eventTable[event];
}

const int32_t cmdq_core_reverse_event_ENUM(const uint32_t value)
{
	uint32_t eventENUM = CMDQ_SYNC_TOKEN_INVALID;
	uint32_t i;

	for (i = 0; i < CMDQ_SYNC_TOKEN_MAX; i++) {
		if (value == gCmdqDtsData.eventTable[i]) {
			eventENUM = i;
			break;
		}
	}

	return eventENUM;
}

void *cmdq_core_alloc_hw_buffer(struct device *dev, size_t size, dma_addr_t *dma_handle,
				const gfp_t flag)
{
	void *pVA;
	dma_addr_t PA;

	do {
		PA = 0;
		pVA = NULL;

#ifdef CMDQ_TEST_EMERGENCY_BUFFER
		const uint32_t minForceEmergencyBufferSize = 64 * 1024;

		if ((1 == atomic_read(&gCmdqDebugForceUseEmergencyBuffer)) &&
		    (minForceEmergencyBufferSize < size)) {
			CMDQ_LOG("[INFO]%s failed because...force use emergency buffer\n",
				 __func__);
			break;
		}
#endif

		CMDQ_PROF_START(current->pid, __func__);

		pVA = dma_alloc_coherent(dev, size, &PA, flag);

		CMDQ_PROF_END(current->pid, __func__);
	} while (0);

	*dma_handle = PA;

	CMDQ_VERBOSE("%s, pVA:0x%p, PA:0x%pa, PAout:0x%pa\n", __func__, pVA, &PA, &(*dma_handle));

	return pVA;
}

void cmdq_core_free_hw_buffer(struct device *dev, size_t size, void *cpu_addr,
			      dma_addr_t dma_handle)
{
	dma_free_coherent(dev, size, cpu_addr, dma_handle);
}

static bool cmdq_core_init_emergency_buffer(void)
{
	int i;

	memset(&gCmdqEmergencyBuffer[0], 0, sizeof(gCmdqEmergencyBuffer));

	for (i = 0; i < CMDQ_EMERGENCY_BLOCK_COUNT; ++i) {
		EmergencyBufferStruct *buf = &gCmdqEmergencyBuffer[i];

		buf->va = cmdq_core_alloc_hw_buffer(cmdq_dev_get(), CMDQ_EMERGENCY_BLOCK_SIZE,
							&buf->pa, GFP_KERNEL);
		buf->size = CMDQ_EMERGENCY_BLOCK_SIZE;
		buf->used = false;
	}

	return true;
}

static bool cmdq_core_uninit_emergency_buffer(void)
{
	int i;

	for (i = 0; i < CMDQ_EMERGENCY_BLOCK_COUNT; ++i) {
		EmergencyBufferStruct *buf = &gCmdqEmergencyBuffer[i];

		cmdq_core_free_hw_buffer(cmdq_dev_get(), CMDQ_EMERGENCY_BLOCK_SIZE,
					 buf->va, buf->pa);
		if (buf->used) {
			CMDQ_ERR("Emergency buffer %d, 0x%p, 0x%pa still using\n",
				 i, buf->va, &buf->pa);
		}
	}

	memset(&gCmdqEmergencyBuffer[0], 0, sizeof(gCmdqEmergencyBuffer));

	return true;
}

static bool cmdq_core_alloc_emergency_buffer(void **va, dma_addr_t *pa)
{
	int i;
	bool ret = false;

	spin_lock(&gCmdqAllocLock);

	for (i = 0; i < CMDQ_EMERGENCY_BLOCK_COUNT; ++i) {

		/* find a free emergency buffer */
		CMDQ_VERBOSE("EmergencyBuffer[%d], use:%d, 0x%p, 0x%pa\n",
			     i,
			     gCmdqEmergencyBuffer[i].used,
			     gCmdqEmergencyBuffer[i].va, &(gCmdqEmergencyBuffer[i].pa));

		if (!gCmdqEmergencyBuffer[i].used && gCmdqEmergencyBuffer[i].va) {

			gCmdqEmergencyBuffer[i].used = true;
			*va = gCmdqEmergencyBuffer[i].va;
			*pa = gCmdqEmergencyBuffer[i].pa;
			ret = true;

			break;
		}
	}

	spin_unlock(&gCmdqAllocLock);
	return ret;
}

static void cmdq_core_free_emergency_buffer(void *va, dma_addr_t pa)
{
	int i;

	spin_lock(&gCmdqAllocLock);
	for (i = 0; i < CMDQ_EMERGENCY_BLOCK_COUNT; ++i) {
		if (gCmdqEmergencyBuffer[i].used && va == gCmdqEmergencyBuffer[i].va) {

			gCmdqEmergencyBuffer[i].used = false;
			break;
		}
	}
	spin_unlock(&gCmdqAllocLock);
}

bool cmdq_core_is_emergency_buffer(void *va)
{
	int i;
	bool ret = false;

	spin_lock(&gCmdqAllocLock);
	for (i = 0; i < CMDQ_EMERGENCY_BLOCK_COUNT; ++i) {
		if (gCmdqEmergencyBuffer[i].used && va == gCmdqEmergencyBuffer[i].va) {
			ret = true;
			break;
		}
	}
	spin_unlock(&gCmdqAllocLock);
	return ret;
}

int32_t cmdq_core_set_secure_IRQ_status(uint32_t value)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	const uint32_t offset = CMDQ_SEC_SHARED_IRQ_RAISED_OFFSET;
	uint32_t *pVA;

	value = 0x0;
	if (NULL == gCmdqContext.hSecSharedMem) {
		CMDQ_ERR("%s, shared memory is not created\n", __func__);
		return -EFAULT;
	}

	pVA = (uint32_t *) (gCmdqContext.hSecSharedMem->pVABase + offset);
	(*pVA) = value;

	CMDQ_VERBOSE("[shared_IRQ]set raisedIRQ:0x%08x\n", value);

	return 0;
#else
	CMDQ_ERR("func:%s failed since CMDQ secure path not support in this proj\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_core_get_secure_IRQ_status(void)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	const uint32_t offset = CMDQ_SEC_SHARED_IRQ_RAISED_OFFSET;
	uint32_t *pVA;
	int32_t value;

	value = 0x0;
	if (NULL == gCmdqContext.hSecSharedMem) {
		CMDQ_ERR("%s, shared memory is not created\n", __func__);
		return -EFAULT;
	}

	pVA = (uint32_t *) (gCmdqContext.hSecSharedMem->pVABase + offset);
	value = *pVA;

	CMDQ_VERBOSE("[shared_IRQ]IRQ raised:0x%08x\n", value);

	return value;
#else
	CMDQ_ERR("func:%s failed since CMDQ secure path not support in this proj\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_core_set_secure_thread_exec_counter(const int32_t thread, const uint32_t cookie)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	const uint32_t offset = CMDQ_SEC_SHARED_THR_CNT_OFFSET + thread * sizeof(uint32_t);
	uint32_t *pVA = NULL;

	if (0 > cmdq_get_func()->isSecureThread(thread)) {
		CMDQ_ERR("%s, invalid param, thread: %d\n", __func__, thread);
		return -EFAULT;
	}

	if (NULL == gCmdqContext.hSecSharedMem) {
		CMDQ_ERR("%s, shared memory is not created\n", __func__);
		return -EFAULT;
	}

	CMDQ_MSG("[shared_cookie] set thread %d CNT(%p) to %d\n", thread, pVA, cookie);
	pVA = (uint32_t *) (gCmdqContext.hSecSharedMem->pVABase + offset);
	(*pVA) = cookie;

	return 0;
#else
	CMDQ_ERR("func:%s failed since CMDQ secure path not support in this proj\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_core_get_secure_thread_exec_counter(const int32_t thread)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	const uint32_t offset = CMDQ_SEC_SHARED_THR_CNT_OFFSET + thread * sizeof(uint32_t);
	uint32_t *pVA;
	uint32_t value;

	if (0 > cmdq_get_func()->isSecureThread(thread)) {
		CMDQ_ERR("%s, invalid param, thread: %d\n", __func__, thread);
		return -EFAULT;
	}

	if (NULL == gCmdqContext.hSecSharedMem) {
		CMDQ_ERR("%s, shared memory is not created\n", __func__);
		return -EFAULT;
	}

	pVA = (uint32_t *) (gCmdqContext.hSecSharedMem->pVABase + offset);
	value = *pVA;
#if defined(CMDQ_SECURE_PATH_NORMAL_IRQ) || defined(CMDQ_SECURE_PATH_HW_LOCK)
	value = value + 1;
#endif
	CMDQ_VERBOSE("[shared_cookie] get thread %d CNT(%p) value is %d\n", thread, pVA, value);

	return value;
#else
	CMDQ_ERR("func:%s failed since CMDQ secure path not support in this proj\n", __func__);
	return -EFAULT;
#endif

}

int32_t cmdq_core_thread_exec_counter(const int32_t thread)
{
	return (false == cmdq_get_func()->isSecureThread(thread)) ?
	    (CMDQ_GET_COOKIE_CNT(thread)) : (cmdq_core_get_secure_thread_exec_counter(thread));
}

cmdqSecSharedMemoryHandle cmdq_core_get_secure_shared_memory(void)
{
	return gCmdqContext.hSecSharedMem;
}

int32_t cmdq_core_stop_secure_path_notify_thread(void)
{
#if defined(CMDQ_SECURE_PATH_SUPPORT) && !defined(CMDQ_SECURE_PATH_NORMAL_IRQ)
	int status = 0;
	unsigned long flags;

	mutex_lock(&gCmdqNotifyLoopMutex);
	do {
		if (NULL == gCmdqContext.hNotifyLoop) {
			/* no notify thread */
			CMDQ_MSG("[WARNING]NULL notify loop\n");
			break;
		}

		status = cmdqRecStopLoop(gCmdqContext.hNotifyLoop);
		if (0 > status) {
			CMDQ_ERR("stop notify loop failed, status:%d\n", status);
			break;
		}

		/* destroy handle */
		spin_lock_irqsave(&gCmdqExecLock, flags);
		cmdqRecDestroy(gCmdqContext.hNotifyLoop);
		gCmdqContext.hNotifyLoop = NULL;
		spin_unlock_irqrestore(&gCmdqExecLock, flags);

		/* CPU clear event */
		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_UPD, CMDQ_SYNC_SECURE_THR_EOF);
	} while (0);
	mutex_unlock(&gCmdqNotifyLoopMutex);

	return status;
#else
	return 0;
#endif
}

int32_t cmdq_core_start_secure_path_notify_thread(void)
{
#if defined(CMDQ_SECURE_PATH_SUPPORT) && !defined(CMDQ_SECURE_PATH_NORMAL_IRQ)
	int status = 0;
	cmdqRecHandle handle;
	unsigned long flags;

	mutex_lock(&gCmdqNotifyLoopMutex);
	do {
		if (NULL != gCmdqContext.hNotifyLoop) {
			/* already created it */
			break;
		}

		/* CPU clear event */
		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_UPD, CMDQ_SYNC_SECURE_THR_EOF);

		/* record command */
		cmdqRecCreate(CMDQ_SCENARIO_SECURE_NOTIFY_LOOP, &handle);
		cmdqRecReset(handle);
		cmdqRecWait(handle, CMDQ_SYNC_SECURE_THR_EOF);
#ifdef CMDQ_SECURE_PATH_HW_LOCK
		cmdqRecWait(handle, CMDQ_SYNC_SECURE_WSM_LOCK);
#endif
		status = cmdqRecStartLoop(handle);

		if (0 > status) {
			CMDQ_ERR("start notify loop failed, status:%d\n", status);
			break;
		}

		/* update notify handle */
		spin_lock_irqsave(&gCmdqExecLock, flags);
		gCmdqContext.hNotifyLoop = (CmdqRecLoopHandle *) handle;
		spin_unlock_irqrestore(&gCmdqExecLock, flags);
	} while (0);
	mutex_unlock(&gCmdqNotifyLoopMutex);

	return status;
#else
	return 0;
#endif
}

const char *cmdq_core_get_event_name_ENUM(CMDQ_EVENT_ENUM event)
{
	const char *eventName = "CMDQ_EVENT_UNKNOWN";

#undef DECLARE_CMDQ_EVENT
#define DECLARE_CMDQ_EVENT(name, val, dts_name)	{ if (val == event) { eventName = #name; break; }  }
	do {
#include "cmdq_event_common.h"
	} while (0);
#undef DECLARE_CMDQ_EVENT

	return eventName;
}

const char *cmdq_core_get_event_name(CMDQ_EVENT_ENUM event)
{
	const int32_t eventENUM = cmdq_core_reverse_event_ENUM(event);

	return cmdq_core_get_event_name_ENUM(eventENUM);
}

void cmdqCoreClearEvent(CMDQ_EVENT_ENUM event)
{
	int32_t eventValue = cmdq_core_get_event_value(event);

	CMDQ_MSG("clear event %d\n", eventValue);
	CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_UPD, eventValue);
}

void cmdqCoreSetEvent(CMDQ_EVENT_ENUM event)
{
	int32_t eventValue = cmdq_core_get_event_value(event);

	CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_UPD, (1L << 16) | eventValue);
}

uint32_t cmdqCoreGetEvent(CMDQ_EVENT_ENUM event)
{
	uint32_t regValue = 0;
	int32_t eventValue = cmdq_core_get_event_value(event);

	CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_ID, (0x3FF && eventValue));
	regValue = CMDQ_REG_GET32(CMDQ_SYNC_TOKEN_VAL);
	return regValue;
}

const bool cmdq_core_support_sync_non_suspendable(void)
{
#ifdef CMDQ_USE_LEGACY
	return false;
#else
	return true;
#endif
}

ssize_t cmdqCorePrintLogLevel(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	if (buf)
		len = sprintf(buf, "%d\n", gCmdqContext.logLevel);

	return len;
}

ssize_t cmdqCoreWriteLogLevel(struct device *dev,
			      struct device_attribute *attr, const char *buf, size_t size)
{
	int len = 0;
	int value = 0;
	int status = 0;

	char textBuf[10] = { 0 };

	do {
		if (size >= 10) {
			status = -EFAULT;
			break;
		}

		len = size;
		memcpy(textBuf, buf, len);

		textBuf[len] = '\0';
		if (0 > kstrtoint(textBuf, 10, &value)) {
			status = -EFAULT;
			break;
		}

		status = len;
		if (value < 0 || value > 3)
			value = 0;

		cmdq_core_set_log_level(value);
	} while (0);

	return status;
}

ssize_t cmdqCorePrintProfileEnable(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	if (buf)
		len = sprintf(buf, "%d\n", gCmdqContext.enableProfile);

	return len;

}

ssize_t cmdqCoreWriteProfileEnable(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t size)
{
	int len = 0;
	int value = 0;
	int status = 0;

	char textBuf[10] = { 0 };

	do {
		if (size >= 10) {
			status = -EFAULT;
			break;
		}

		len = size;
		memcpy(textBuf, buf, len);

		textBuf[len] = '\0';
		if (0 > kstrtoint(textBuf, 10, &value)) {
			status = -EFAULT;
			break;
		}

		status = len;
		if (value < 0 || value > 3)
			value = 0;

		gCmdqContext.enableProfile = value;
		if (0 < value)
			cmdqSecEnableProfile(true);
		else
			cmdqSecEnableProfile(false);
	} while (0);

	return status;
}

static uint32_t *cmdq_core_get_pc(const TaskStruct *pTask, uint32_t thread, uint32_t insts[4])
{
	long currPC = 0L;
	uint8_t *pInst = NULL;

	insts[0] = 0;
	insts[1] = 0;
	insts[2] = 0;
	insts[3] = 0;

	if ((NULL == pTask) || (NULL == pTask->pVABase) || (CMDQ_INVALID_THREAD == thread)) {
		CMDQ_ERR
		    ("get pc failed since invalid param, pTask %p, pTask->pVABase:%p, thread:%d\n",
		     pTask, pTask->pVABase, thread);
		return NULL;
	}

	if (true == pTask->secData.isSecure) {
		/* be carefull it dose not allow normal access to secure threads' register */
		return NULL;
	}

	currPC = CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread)));
	pInst = (uint8_t *) pTask->pVABase + (currPC - pTask->MVABase);

	if (((uint8_t *) pTask->pVABase <= pInst) && (pInst <= (uint8_t *) pTask->pCMDEnd)) {
		if (pInst != (uint8_t *) pTask->pCMDEnd) {
			/* If PC points to start of pCMD, */
			/* - 8 causes access violation */
			/* insts[0] = CMDQ_REG_GET32(pInst - 8); */
			/* insts[1] = CMDQ_REG_GET32(pInst - 4); */
			insts[2] = CMDQ_REG_GET32(pInst + 0);
			insts[3] = CMDQ_REG_GET32(pInst + 4);
		} else {
			/* insts[0] = CMDQ_REG_GET32(pInst - 16); */
			/* insts[1] = CMDQ_REG_GET32(pInst - 12); */
			insts[2] = CMDQ_REG_GET32(pInst - 8);
			insts[3] = CMDQ_REG_GET32(pInst - 4);
		}
	} else {
		/* invalid PC address */
		return NULL;
	}

	return (uint32_t *) pInst;
}

static int cmdq_core_print_record(const RecordStruct *pRecord, int index, char *_buf, int bufLen)
{
	int length = 0;
	char *unit[5] = { "ms", "ms", "ms", "ms", "ms" };
	int32_t IRQTime;
	int32_t execTime;
	int32_t beginWaitTime;
	int32_t totalTime;
	int32_t acquireThreadTime;
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	int32_t profileMarkerCount;
	int32_t i;
#endif
	unsigned long rem_nsec;
	CMDQ_TIME submitTimeSec;
	char *buf;

	rem_nsec = 0;
	submitTimeSec = pRecord->submit;
	rem_nsec = do_div(submitTimeSec, 1000000000);
	buf = _buf;

	unit[0] = "ms";
	unit[1] = "ms";
	unit[2] = "ms";
	unit[3] = "ms";
	unit[4] = "ms";
	CMDQ_GET_TIME_IN_MS(pRecord->submit, pRecord->done, totalTime);
	CMDQ_GET_TIME_IN_MS(pRecord->submit, pRecord->trigger, acquireThreadTime);
	CMDQ_GET_TIME_IN_MS(pRecord->submit, pRecord->beginWait, beginWaitTime);
	CMDQ_GET_TIME_IN_MS(pRecord->trigger, pRecord->gotIRQ, IRQTime);
	CMDQ_GET_TIME_IN_MS(pRecord->trigger, pRecord->wakedUp, execTime);

	/* detect us interval */
	if (0 == acquireThreadTime) {
		CMDQ_GET_TIME_IN_US_PART(pRecord->submit, pRecord->trigger, acquireThreadTime);
		unit[0] = "us";
	}
	if (0 == IRQTime) {
		CMDQ_GET_TIME_IN_US_PART(pRecord->trigger, pRecord->gotIRQ, IRQTime);
		unit[1] = "us";
	}
	if (0 == beginWaitTime) {
		CMDQ_GET_TIME_IN_US_PART(pRecord->submit, pRecord->beginWait, beginWaitTime);
		unit[2] = "us";
	}
	if (0 == execTime) {
		CMDQ_GET_TIME_IN_US_PART(pRecord->trigger, pRecord->wakedUp, execTime);
		unit[3] = "us";
	}
	if (0 == totalTime) {
		CMDQ_GET_TIME_IN_US_PART(pRecord->submit, pRecord->done, totalTime);
		unit[4] = "us";
	}

	/* pRecord->priority for task priority */
	/* when pRecord->isSecure is 0 for secure task */
	length = snprintf(buf, bufLen,
			  "%4d:(%5d, %2d, 0x%012llx, %2d, %d, %d)(%02d, %02d)(%5dns : %lld, %lld)",
			  index, pRecord->user, pRecord->scenario, pRecord->engineFlag,
			  pRecord->priority, pRecord->isSecure, pRecord->size,
			  pRecord->thread,
			  cmdq_get_func()->priority(pRecord->scenario),
			  pRecord->writeTimeNS, pRecord->writeTimeNSBegin, pRecord->writeTimeNSEnd);
	bufLen -= length;
	buf += length;

	length = snprintf(buf, bufLen,
			  "(%5llu.%06lu, %4d%s, %4d%s, %4d%s, %4d%s)%4d%s",
			  submitTimeSec, rem_nsec / 1000,
			  acquireThreadTime, unit[0],
			  IRQTime, unit[1], beginWaitTime, unit[2],
			  execTime, unit[3], totalTime, unit[4]);
	bufLen -= length;
	buf += length;

#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	profileMarkerCount = pRecord->profileMarkerCount;
	if (profileMarkerCount > CMDQ_MAX_PROFILE_MARKER_IN_TASK)
		profileMarkerCount = CMDQ_MAX_PROFILE_MARKER_IN_TASK;

	for (i = 0; i < profileMarkerCount; i++) {
		length = snprintf(buf, bufLen, "(P%d,%s,%lld,)",
				  i, pRecord->profileMarkerTag[i], pRecord->profileMarkerTimeNS[i]);
		bufLen -= length;
		buf += length;
	}
#endif
	length = snprintf(buf, bufLen, "\n");
	bufLen -= length;
	buf += length;

	length = (buf - _buf);
	return length;
}

int cmdqCorePrintRecordSeq(struct seq_file *m, void *v)
{
	unsigned long flags;
	int32_t index;
	int32_t numRec;
	RecordStruct record;
	const uint32_t msg_bufLen = 512;
	char *msg;

	msg = vmalloc(msg_bufLen);

	/* we try to minimize time spent in spin lock */
	/* since record is an array so it is okay to */
	/* allow displaying an out-of-date entry. */
	spin_lock_irqsave(&gCmdqRecordLock, flags);
	numRec = gCmdqContext.recNum;
	index = gCmdqContext.lastID - 1;
	spin_unlock_irqrestore(&gCmdqRecordLock, flags);

	/* we print record in reverse order. */
	for (; numRec > 0; --numRec, --index) {
		if (index >= CMDQ_MAX_RECORD_COUNT)
			index = 0;
		else if (index < 0)
			index = CMDQ_MAX_RECORD_COUNT - 1;

		/* Make sure we don't print a record that is during updating. */
		/* However, this record may already be different */
		/* from the time of entering cmdqCorePrintRecordSeq(). */
		spin_lock_irqsave(&gCmdqRecordLock, flags);
		record = gCmdqContext.record[index];
		spin_unlock_irqrestore(&gCmdqRecordLock, flags);

		cmdq_core_print_record(&record, index, msg, msg_bufLen);

		seq_printf(m, "%s", msg);
	}

	vfree(msg);

	return 0;
}

int cmdqCorePrintErrorSeq(struct seq_file *m, void *v)
{
	/* error is not used by now */
	return 0;
}

int cmdqCorePrintStatusSeq(struct seq_file *m, void *v)
{
	unsigned long flags = 0;
	EngineStruct *pEngine = NULL;
	TaskStruct *pTask = NULL;
	struct list_head *p = NULL;
	ThreadStruct *pThread = NULL;
	int32_t index = 0;
	int32_t inner = 0;
	int listIdx = 0;
	const struct list_head *lists[] = {
		&gCmdqContext.taskFreeList,
		&gCmdqContext.taskActiveList,
		&gCmdqContext.taskWaitList
	};
	uint32_t *pcVA = NULL;
	uint32_t insts[4] = { 0 };
	char parsedInstruction[128] = { 0 };

	static const char *const listNames[] = { "Free", "Active", "Wait" };

	const CMDQ_ENG_ENUM engines[] = CMDQ_FOREACH_MODULE_PRINT(GENERATE_ENUM);
	static const char *const engineNames[] = CMDQ_FOREACH_MODULE_PRINT(GENERATE_STRING);

#ifdef CMDQ_DUMP_FIRSTERROR
	if (gCmdqFirstError.cmdqCount > 0) {
		unsigned long long saveTimeSec = gCmdqFirstError.savetime;
		unsigned long rem_nsec = do_div(saveTimeSec, 1000000000);
		struct tm nowTM;

		time_to_tm(gCmdqFirstError.savetv.tv_sec, sys_tz.tz_minuteswest * 60, &nowTM);
		seq_puts(m, "================= [CMDQ] Dump first error ================\n");
		seq_printf(m, "kernel time:[%5llu.%06lu],", saveTimeSec, rem_nsec / 1000);
		seq_printf(m, " UTC time:[%04ld-%02d-%02d %02d:%02d:%02d.%06ld],",
			   (nowTM.tm_year + 1900), (nowTM.tm_mon + 1), nowTM.tm_mday,
			   nowTM.tm_hour, nowTM.tm_min, nowTM.tm_sec,
			   gCmdqFirstError.savetv.tv_usec);
		seq_printf(m, " Pid: %d, Name: %s\n", gCmdqFirstError.callerPid,
			   gCmdqFirstError.callerName);
		seq_printf(m, "%s", gCmdqFirstError.cmdqString);
		if (gCmdqFirstError.cmdqMaxSize <= 0)
			seq_printf(m, "\nWARNING: MAX size: %d is full\n", CMDQ_MAX_FIRSTERROR);
		seq_puts(m, "\n\n");
	}
#endif

	/* Save command buffer dump */
	if (gCmdqBufferDump.count > 0) {
		int32_t buffer_id;

		seq_puts(m, "================= [CMDQ] Dump Command Buffer =================\n");
		mutex_lock(&gCmdqTaskMutex);
		for (buffer_id = 0; buffer_id < gCmdqBufferDump.bufferSize; buffer_id++)
			seq_printf(m, "%c", gCmdqBufferDump.cmdqString[buffer_id]);
		mutex_unlock(&gCmdqTaskMutex);
		seq_puts(m, "\n=============== [CMDQ] Dump Command Buffer END ===============\n\n\n");
	}

#ifdef CMDQ_PWR_AWARE
	/* note for constatnt format (without a % substitution), use seq_puts to speed up outputs */
	seq_puts(m, "====== Clock Status =======\n");
	cmdq_get_func()->printStatusSeqClock(m);
#endif

	seq_puts(m, "====== Engine Usage =======\n");

	for (listIdx = 0; listIdx < (sizeof(engines) / sizeof(engines[0])); ++listIdx) {
		pEngine = &gCmdqContext.engine[engines[listIdx]];
		seq_printf(m, "%s: count %d, owner %d, fail: %d, reset: %d\n",
			   engineNames[listIdx],
			   pEngine->userCount,
			   pEngine->currOwner, pEngine->failCount, pEngine->resetCount);
	}


	mutex_lock(&gCmdqTaskMutex);

	/* print all tasks in both list */
	for (listIdx = 0; listIdx < (sizeof(lists) / sizeof(lists[0])); listIdx++) {
		/* skip FreeTasks by default */
		if (!cmdq_core_should_print_msg() && 0 == listIdx)
			continue;

		index = 0;
		list_for_each(p, lists[listIdx]) {
			pTask = list_entry(p, struct TaskStruct, listEntry);
			seq_printf(m, "====== %s Task(%d) 0x%p Usage =======\n", listNames[listIdx],
				   index, pTask);

			seq_printf(m, "State %d, VABase: 0x%p, MVABase: %pa, Size: %d\n",
				   pTask->taskState, pTask->pVABase, &pTask->MVABase,
				   pTask->commandSize);
			seq_printf(m, "Scenario %d, Priority: %d, Flag: 0x%08llx, VAEnd: 0x%p\n",
				   pTask->scenario, pTask->priority, pTask->engineFlag,
				   pTask->pCMDEnd);
			seq_printf(m,
				   "Reorder:%d, Trigger %lld, IRQ: %lld, Wait: %lld, Wake Up: %lld\n",
				   pTask->reorder,
				   pTask->trigger, pTask->gotIRQ, pTask->beginWait, pTask->wakedUp);
			++index;
		}
		seq_printf(m, "====== Total %d %s Task =======\n", index, listNames[listIdx]);
	}

	for (index = 0; index < CMDQ_MAX_THREAD_COUNT; index++) {
		pThread = &(gCmdqContext.thread[index]);

		if (pThread->taskCount > 0) {
			seq_printf(m, "====== Thread %d Usage =======\n", index);
			seq_printf(m, "Wait Cookie %d, Next Cookie %d\n", pThread->waitCookie,
				   pThread->nextCookie);

			spin_lock_irqsave(&gCmdqThreadLock, flags);

			for (inner = 0; inner < cmdq_core_max_task_in_thread(index); inner++) {
				pTask = pThread->pCurTask[inner];
				if (NULL != pTask) {
					/* dump task basic info */
					seq_printf(m,
						   "Slot: %d, Task: 0x%p, Pid: %d, Name: %s, Scn: %d,",
						   index, pTask, pTask->callerPid,
						   pTask->callerName, pTask->scenario);
					seq_printf(m,
						   " VABase: 0x%p, MVABase: %pa, Size: %d",
						   pTask->pVABase, &pTask->MVABase,
						   pTask->commandSize);

					if (pTask->pCMDEnd) {
						seq_printf(m,
							   ", Last Command: 0x%08x:0x%08x",
							   pTask->pCMDEnd[-1], pTask->pCMDEnd[0]);
					}

					seq_puts(m, "\n");

					/* dump PC info */
					pcVA = cmdq_core_get_pc(pTask, index, insts);
					if (pcVA) {
						cmdq_core_parse_instruction(pcVA, parsedInstruction,
									    sizeof(parsedInstruction));
						seq_printf(m,
							   "PC(VA): 0x%p, 0x%08x:0x%08x => %s",
							   pcVA, insts[2], insts[3],
							   parsedInstruction);
					} else {
						seq_puts(m, "PC(VA): Not available\n");
					}
				}
			}

			spin_unlock_irqrestore(&gCmdqThreadLock, flags);
		}
	}

	mutex_unlock(&gCmdqTaskMutex);

	return 0;
}

ssize_t cmdqCorePrintRecord(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long flags;
	int32_t begin;
	int32_t curPos;
	ssize_t bufLen = PAGE_SIZE;
	ssize_t length;
	int32_t index;
	int32_t numRec;
	RecordStruct record;

	begin = 0;
	curPos = 0;
	length = 0;
	bufLen = PAGE_SIZE;

	/* we try to minimize time spent in spin lock */
	/* since record is an array so it is okay to */
	/* allow displaying an out-of-date entry. */
	spin_lock_irqsave(&gCmdqRecordLock, flags);
	numRec = gCmdqContext.recNum;
	index = gCmdqContext.lastID - 1;
	spin_unlock_irqrestore(&gCmdqRecordLock, flags);

	/* we print record in reverse order. */
	for (; numRec > 0; --numRec, --index) {
		/* CMDQ_ERR("[rec] index=%d numRec =%d\n", index, numRec); */

		if (index >= CMDQ_MAX_RECORD_COUNT)
			index = 0;
		else if (index < 0)
			index = CMDQ_MAX_RECORD_COUNT - 1;

		/* Make sure we don't print a record that is during updating. */
		/* However, this record may already be different */
		/* from the time of entering cmdqCorePrintRecordSeq(). */
		spin_lock_irqsave(&gCmdqRecordLock, flags);
		record = (gCmdqContext.record[index]);
		spin_unlock_irqrestore(&gCmdqRecordLock, flags);

		length = cmdq_core_print_record(&record, index, &buf[curPos], bufLen);

		bufLen -= length;
		curPos += length;

		if (bufLen <= 0 || curPos >= PAGE_SIZE)
			break;
	}

	if (curPos >= PAGE_SIZE)
		curPos = PAGE_SIZE;

	return curPos;
}

ssize_t cmdqCorePrintError(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;
	int length = 0;

	for (i = 0; i < gCmdqContext.errNum && i < CMDQ_MAX_ERROR_COUNT; ++i) {
		ErrorStruct *pError = &gCmdqContext.error[i];
		u64 ts = pError->ts_nsec;
		unsigned long rem_nsec = do_div(ts, 1000000000);

		length += snprintf(buf + length,
				   PAGE_SIZE - length,
				   "[%5lu.%06lu] ", (unsigned long)ts, rem_nsec / 1000);
		length += cmdq_core_print_record(&pError->errorRec,
						 i, buf + length, PAGE_SIZE - length);
		if (length >= PAGE_SIZE)
			break;
	}

	return length;
}

ssize_t cmdqCorePrintStatus(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long flags = 0L;
	EngineStruct *pEngine = NULL;
	TaskStruct *pTask = NULL;
	struct list_head *p = NULL;
	ThreadStruct *pThread = NULL;
	int32_t index = 0;
	int32_t inner = 0;
	int32_t length = 0;
	int listIdx = 0;
	char *pBuffer = buf;
	const struct list_head *lists[] = {
		&gCmdqContext.taskFreeList,
		&gCmdqContext.taskActiveList,
		&gCmdqContext.taskWaitList
	};
	uint32_t *pcVA = NULL;
	uint32_t insts[4] = { 0 };
	char parsedInstruction[128] = { 0 };

	static const char *const listNames[] = { "Free", "Active", "Wait" };

	const CMDQ_ENG_ENUM engines[] = CMDQ_FOREACH_MODULE_PRINT(GENERATE_ENUM);
	static const char *const engineNames[] = CMDQ_FOREACH_MODULE_PRINT(GENERATE_STRING);

#ifdef CMDQ_PWR_AWARE
	pBuffer += sprintf(pBuffer, "====== Clock Status =======\n");
	pBuffer += cmdq_get_func()->printStatusClock(pBuffer);
#endif

	pBuffer += sprintf(pBuffer, "====== Engine Usage =======\n");

	for (listIdx = 0; listIdx < (sizeof(engines) / sizeof(engines[0])); ++listIdx) {
		pEngine = &gCmdqContext.engine[engines[listIdx]];
		pBuffer += sprintf(pBuffer, "%s: count %d, owner %d, fail: %d, reset: %d\n",
				   engineNames[listIdx],
				   pEngine->userCount,
				   pEngine->currOwner, pEngine->failCount, pEngine->resetCount);
	}


	mutex_lock(&gCmdqTaskMutex);

	/* print all tasks in both list */
	for (listIdx = 0; listIdx < (sizeof(lists) / sizeof(lists[0])); listIdx++) {
		/* skip FreeTasks by default */
		if (!cmdq_core_should_print_msg() && 0 == listIdx)
			continue;

		index = 0;
		list_for_each(p, lists[listIdx]) {
			pTask = list_entry(p, struct TaskStruct, listEntry);
			pBuffer += sprintf(pBuffer,
					   "====== %s Task(%d) 0x%p Usage =======\n",
					   listNames[listIdx], index, pTask);

			pBuffer += sprintf(pBuffer,
					   "State %d, VABase: 0x%p, MVABase: %pa, Size: %d\n",
					   pTask->taskState, pTask->pVABase, &pTask->MVABase,
					   pTask->commandSize);

			pBuffer += sprintf(pBuffer,
					   "Scenario %d, Priority: %d, Flag: 0x%08llx, VAEnd: 0x%p\n",
					   pTask->scenario, pTask->priority, pTask->engineFlag,
					   pTask->pCMDEnd);

			pBuffer += sprintf(pBuffer,
					   "Reoder:%d, Trigger %lld, IRQ: %lld, Wait: %lld, Wake Up: %lld\n",
					   pTask->reorder,
					   pTask->trigger, pTask->gotIRQ, pTask->beginWait,
					   pTask->wakedUp);
			++index;
		}
		pBuffer +=
		    sprintf(pBuffer, "====== Total %d %s Task =======\n", index,
			    listNames[listIdx]);
	}

	for (index = 0; index < CMDQ_MAX_THREAD_COUNT; index++) {
		pThread = &(gCmdqContext.thread[index]);

		if (pThread->taskCount > 0) {
			pBuffer += sprintf(pBuffer, "====== Thread %d Usage =======\n", index);
			pBuffer += sprintf(pBuffer, "Wait Cookie %d, Next Cookie %d\n",
					   pThread->waitCookie, pThread->nextCookie);

			spin_lock_irqsave(&gCmdqThreadLock, flags);

			for (inner = 0; inner < cmdq_core_max_task_in_thread(index); inner++) {
				pTask = pThread->pCurTask[inner];
				if (NULL != pTask) {
					/* dump task basic info */
					pBuffer += sprintf(pBuffer,
							   "Slot: %d, Task: 0x%p, Pid: %d, Name: %s, Scn: %d,",
							   index, pTask, pTask->callerPid,
							   pTask->callerName, pTask->scenario);
					pBuffer += sprintf(pBuffer,
							   " VABase: 0x%p, MVABase: %pa, Size: %d",
							   pTask->pVABase, &pTask->MVABase,
							   pTask->commandSize);

					if (pTask->pCMDEnd) {
						pBuffer += sprintf(pBuffer,
								   ", Last Command: 0x%08x:0x%08x",
								   pTask->pCMDEnd[-1],
								   pTask->pCMDEnd[0]);
					}

					pBuffer += sprintf(pBuffer, "\n");

					/* dump PC info */
					pcVA = cmdq_core_get_pc(pTask, index, insts);
					if (pcVA) {
						cmdq_core_parse_instruction(pcVA, parsedInstruction,
									    sizeof(parsedInstruction));
						pBuffer += sprintf(pBuffer,
								   "PC(VA): 0x%p, 0x%08x:0x%08x => %s",
								   pcVA,
								   insts[2],
								   insts[3],
								   parsedInstruction);
					} else {
						pBuffer += sprintf(pBuffer, "PC(VA): Not available\n");
					}
				}
			}

			spin_unlock_irqrestore(&gCmdqThreadLock, flags);
		}
	}

	mutex_unlock(&gCmdqTaskMutex);

	length = pBuffer - buf;

	BUG_ON(length > PAGE_SIZE);

	return length;

}

static void cmdq_task_init_profile_marker_data(cmdqCommandStruct *pCommandDesc, TaskStruct *pTask)
{
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	uint32_t i;

	pTask->profileMarker.count = pCommandDesc->profileMarker.count;
	pTask->profileMarker.hSlot = pCommandDesc->profileMarker.hSlot;
	for (i = 0; i < CMDQ_MAX_PROFILE_MARKER_IN_TASK; i++)
		pTask->profileMarker.tag[i] = pCommandDesc->profileMarker.tag[i];
#endif
}

static void cmdq_task_deinit_profile_marker_data(TaskStruct *pTask)
{
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	if (NULL == pTask)
		return;

	if ((0 >= pTask->profileMarker.count) || (0 == pTask->profileMarker.hSlot))
		return;

	cmdqBackupFreeSlot((cmdqBackupSlotHandle) (pTask->profileMarker.hSlot));
	pTask->profileMarker.hSlot = 0LL;
	pTask->profileMarker.count = 0;
#endif
}

/*  */
/* For kmemcache, initialize variables of TaskStruct (but not buffers) */
static void cmdq_core_task_ctor(void *param)
{
	struct TaskStruct *pTask = (TaskStruct *) param;

	CMDQ_VERBOSE("cmdq_core_task_ctor: 0x%p\n", param);
	memset(pTask, 0, sizeof(TaskStruct));
	INIT_LIST_HEAD(&(pTask->listEntry));
	pTask->taskState = TASK_STATE_IDLE;
	pTask->thread = CMDQ_INVALID_THREAD;
}

void cmdq_task_free_task_command_buffer(TaskStruct *pTask)
{
	if (pTask->pVABase) {
		if (pTask->useEmergencyBuf) {
			cmdq_core_free_emergency_buffer(pTask->pVABase, pTask->MVABase);
		} else {
			cmdq_core_free_hw_buffer(cmdq_dev_get(), pTask->bufferSize,
						 pTask->pVABase, pTask->MVABase);
		}

		pTask->pVABase = NULL;
		pTask->MVABase = 0;
		pTask->bufferSize = 0;
		pTask->commandSize = 0;
		pTask->pCMDEnd = NULL;
	}
}

/*  */
/* Ensures size of command buffer of the given task. */
/* Existing buffer will be copied to new buffer. */
/*  */
/* This buffer is guranteed to be physically continuous. */
/*  */
/* returns -ENOMEM if cannot allocate new buffer */
static int32_t cmdq_core_task_realloc_buffer_size(TaskStruct *pTask, uint32_t size)
{
	void *pNewBuffer = NULL;
	dma_addr_t newMVABase = 0;
	int32_t commandSize = 0;
	uint32_t *pCMDEnd = NULL;

	if (pTask->pVABase && pTask->bufferSize >= size) {
		/* buffer size is already good, do nothing. */
		return 0;
	}

	do {
		/* allocate new buffer, try if we can alloc without reclaim */
		pNewBuffer = cmdq_core_alloc_hw_buffer(cmdq_dev_get(), size,
						&newMVABase, GFP_KERNEL | __GFP_NO_KSWAPD);

		if (pNewBuffer) {
			pTask->useEmergencyBuf = false;
			break;
		}

		/* failed. Try emergency buffer */
		if (size <= CMDQ_EMERGENCY_BLOCK_SIZE)
			cmdq_core_alloc_emergency_buffer(&pNewBuffer, &newMVABase);

		if (pNewBuffer) {
			CMDQ_MSG("emergency buffer %p allocated\n", pNewBuffer);
			pTask->useEmergencyBuf = true;
			break;
		}

		/* finally try reclaim */
		pNewBuffer =
		    cmdq_core_alloc_hw_buffer(cmdq_dev_get(), size, &newMVABase,
					      GFP_KERNEL);
		if (pNewBuffer) {
			pTask->useEmergencyBuf = false;
			break;
		}
	} while (0);

	if (NULL == pNewBuffer) {
		CMDQ_ERR("realloc cmd buffer of size %d failed\n", size);
		return -ENOMEM;
	}

	memset(pNewBuffer, 0, size);

	/* copy and release old buffer */
	if (pTask->pVABase)
		memcpy(pNewBuffer, pTask->pVABase, pTask->bufferSize);

	/* we should keep track of pCMDEnd and cmdSize since they are cleared in free command buffer */
	pCMDEnd = pTask->pCMDEnd;
	commandSize = pTask->commandSize;

	cmdq_task_free_task_command_buffer(pTask);

	/* attach the new buffer */
	pTask->pVABase = (uint32_t *) pNewBuffer;
	pTask->MVABase = newMVABase;
	pTask->bufferSize = size;
	pTask->pCMDEnd = pCMDEnd;
	pTask->commandSize = commandSize;

	CMDQ_MSG("Task Buffer:0x%p, VA:%p PA:%pa\n", pTask, pTask->pVABase, &pTask->MVABase);
	return 0;
}

/*  */
/* Allocate and initialize TaskStruct and its command buffer */
static TaskStruct *cmdq_core_task_create(void)
{
	struct TaskStruct *pTask = NULL;
	int32_t status = 0;

	pTask = (TaskStruct *) kmem_cache_alloc(gCmdqContext.taskCache, GFP_KERNEL);
	status = cmdq_core_task_realloc_buffer_size(pTask, CMDQ_INITIAL_CMD_BLOCK_SIZE);
	if (status < 0) {
		CMDQ_AEE("CMDQ", "Allocate command buffer failed\n");
		kmem_cache_free(gCmdqContext.taskCache, pTask);
		pTask = NULL;
		return NULL;
	}
	return pTask;
}

void cmdq_core_reset_hw_events_impl(CMDQ_EVENT_ENUM event)
{
	int32_t value = cmdq_core_get_event_value(event);

	if (value > 0) {
		/* Reset GCE event */
		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_UPD, (CMDQ_SYNC_TOKEN_MAX & value));
	}
}

void cmdq_core_reset_hw_events(void)
{
	int index;
	/* set all defined events to 0 */
	CMDQ_MSG("cmdq_core_reset_hw_events\n");


#undef DECLARE_CMDQ_EVENT
#define DECLARE_CMDQ_EVENT(name, val, dts_name) \
{	\
	cmdq_core_reset_hw_events_impl(name);	\
}
#include "cmdq_event_common.h"
#undef DECLARE_CMDQ_EVENT

	/* However, GRP_SET are resource flags, */
	/* by default they should be 1. */
	cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_GPR_SET_0);
	cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_GPR_SET_1);
	cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_GPR_SET_2);
	cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_GPR_SET_3);
	cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_GPR_SET_4);

	/* However, CMDQ_SYNC_RESOURCE are resource flags, */
	/* by default they should be 1. */
	cmdqCoreSetEvent(CMDQ_SYNC_RESOURCE_WROT0);
	/* However, CMDQ_SYNC_RESOURCE are WSM lock flags, */
	/* by default they should be 1. */
	cmdqCoreSetEvent(CMDQ_SYNC_SECURE_WSM_LOCK);

	/* However, APPEND_THR are resource flags, */
	/* by default they should be 1. */
	for (index = 0; index < CMDQ_MAX_THREAD_COUNT; index++)
		cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_APPEND_THR(index));
}

#if 0
uint32_t *addressToDump[3] = { IO_VIRT_TO_PHYS(MMSYS_CONFIG_BASE + 0x0890),
	IO_VIRT_TO_PHYS(MMSYS_CONFIG_BASE + 0x0890),
	IO_VIRT_TO_PHYS(MMSYS_CONFIG_BASE + 0x0890)
};

static int32_t testcase_regdump_begin(uint32_t taskID, uint32_t *regCount, uint32_t **regAddress)
{
	CMDQ_MSG("@@@@@@@@@@@@@@@@@@ testcase_regdump_begin, tid = %d\n", taskID);
	*regCount = 3;
	*regAddress = addressToDump;
	return 0;
}

static int32_t testcase_regdump_end(uint32_t taskID, uint32_t regCount, uint32_t *regValues)
{
	int i;

	CMDQ_MSG("@@@@@@@@@@@@@@@@@@ testcase_regdump_end, tid = %d\n", taskID);
	CMDQ_MSG("@@@@@@@@@@@@@@@@@@ regCount = %d\n", regCount);

	for (i = 0; i < regCount; ++i)
		CMDQ_MSG("@@@@@@@@@@@@@@@@@@ regValue[%d] = 0x%08x\n", i, regValues[i]);

	return 0;
}
#endif

void cmdq_core_reset_engine_struct(void)
{
	struct EngineStruct *pEngine;
	int index;

	/* Reset engine status */
	pEngine = gCmdqContext.engine;
	for (index = 0; index < CMDQ_MAX_ENGINE_COUNT; index++)
		pEngine[index].currOwner = CMDQ_INVALID_THREAD;
}

void cmdq_core_reset_thread_struct(void)
{
	struct ThreadStruct *pThread;
	int index;

	/* Reset thread status */
	pThread = &(gCmdqContext.thread[0]);
	for (index = 0; index < CMDQ_MAX_THREAD_COUNT; index++)
		pThread[index].allowDispatching = 1;

	/* Initialize work queue per thread */
	for (index = 0; index < CMDQ_MAX_THREAD_COUNT; index++) {
		gCmdqContext.thread[index].taskThreadAutoReleaseWQ =
		    create_singlethread_workqueue("cmdq_auto_release_thread");
	}
}

bool cmdq_core_is_valid_group(CMDQ_GROUP_ENUM engGroup)
{
	/* check range */
	if (engGroup < 0 || engGroup >= CMDQ_MAX_GROUP_COUNT)
		return false;

	return true;
}

int32_t cmdq_core_is_group_flag(CMDQ_GROUP_ENUM engGroup, uint64_t engineFlag)
{
	if (!cmdq_core_is_valid_group(engGroup))
		return false;

	if (gCmdqEngineGroupBits[engGroup] & engineFlag)
		return true;

	return false;
}

static inline uint32_t cmdq_core_get_task_timeout_cycle(struct ThreadStruct *pThread)
{
	/* if there is loop callback, this thread is in loop mode, */
	/* and should not have a timeout. */
	/* So pass 0 as "no timeout" */

	/* return pThread->loopCallback ? 0 : CMDQ_MAX_INST_CYCLE; */

	/* HACK: disable HW timeout */
	return 0;
}

void cmdqCoreInitGroupCB(void)
{
	memset(&(gCmdqGroupCallback), 0x0, sizeof(gCmdqGroupCallback));
	memset(&(gCmdqDebugCallback), 0x0, sizeof(gCmdqDebugCallback));
}

void cmdqCoreDeinitGroupCB(void)
{
	memset(&(gCmdqGroupCallback), 0x0, sizeof(gCmdqGroupCallback));
	memset(&(gCmdqDebugCallback), 0x0, sizeof(gCmdqDebugCallback));
}

int32_t cmdqCoreRegisterCB(CMDQ_GROUP_ENUM engGroup,
			   CmdqClockOnCB clockOn,
			   CmdqDumpInfoCB dumpInfo,
			   CmdqResetEngCB resetEng, CmdqClockOffCB clockOff)
{
	CmdqCBkStruct *pCallback;

	if (!cmdq_core_is_valid_group(engGroup))
		return -EFAULT;

	CMDQ_MSG("Register %d group engines' callback\n", engGroup);
	CMDQ_MSG("clockOn:  0x%pf, dumpInfo: 0x%pf\n", clockOn, dumpInfo);
	CMDQ_MSG("resetEng: 0x%pf, clockOff: 0x%pf\n", resetEng, clockOff);

	pCallback = &(gCmdqGroupCallback[engGroup]);

	pCallback->clockOn = clockOn;
	pCallback->dumpInfo = dumpInfo;
	pCallback->resetEng = resetEng;
	pCallback->clockOff = clockOff;

	return 0;
}

int32_t cmdqCoreRegisterDebugRegDumpCB(CmdqDebugRegDumpBeginCB beginCB, CmdqDebugRegDumpEndCB endCB)
{
	CMDQ_VERBOSE("Register reg dump: begin=%p, end=%p\n", beginCB, endCB);
	gCmdqDebugCallback.beginDebugRegDump = beginCB;
	gCmdqDebugCallback.endDebugRegDump = endCB;
	return 0;
}

bool cmdqIsValidTaskPtr(void *pTask)
{
	struct TaskStruct *ptr = NULL;
	struct list_head *p = NULL;
	bool ret = false;

	mutex_lock(&gCmdqTaskMutex);

	list_for_each(p, &gCmdqContext.taskActiveList) {
		ptr = list_entry(p, struct TaskStruct, listEntry);
		if (ptr == pTask && TASK_STATE_IDLE != ptr->taskState) {
			ret = true;
			break;
		}
	}

	list_for_each(p, &gCmdqContext.taskWaitList) {
		ptr = list_entry(p, struct TaskStruct, listEntry);
		if (ptr == pTask && TASK_STATE_WAITING == ptr->taskState) {
			ret = true;
			break;
		}
	}

	mutex_unlock(&gCmdqTaskMutex);
	return ret;
}

static void cmdq_core_release_buffer(TaskStruct *pTask)
{
	CMDQ_MSG("cmdq_core_release_buffer start\n");
	if (pTask->profileData) {
		cmdq_core_free_hw_buffer(cmdq_dev_get(),
					 2 * sizeof(uint32_t), pTask->profileData,
					 pTask->profileDataPA);
		pTask->profileData = NULL;
		pTask->profileDataPA = 0;
	}

	if (pTask->regResults) {
		CMDQ_MSG("COMMAND: Free result buf VA:0x%p, PA:%pa\n", pTask->regResults,
			 &pTask->regResultsMVA);
		cmdq_core_free_hw_buffer(cmdq_dev_get(),
					 pTask->regCount * sizeof(pTask->regResults[0]),
					 pTask->regResults, pTask->regResultsMVA);
	}
	pTask->regResults = NULL;
	pTask->regResultsMVA = 0;
	pTask->regCount = 0;

	cmdq_task_free_task_command_buffer(pTask);

	cmdq_task_deinit_profile_marker_data(pTask);
	CMDQ_MSG("cmdq_core_release_buffer end\n");
}

static void cmdq_core_release_task_unlocked(TaskStruct *pTask)
{
	CMDQ_MSG("cmdq_core_release_task_unlocked start\n");
	pTask->taskState = TASK_STATE_IDLE;
	pTask->thread = CMDQ_INVALID_THREAD;

	cmdq_core_release_buffer(pTask);

	/* remove from active/waiting list */
	list_del_init(&(pTask->listEntry));
	/* insert into free list. Currently we don't shrink free list. */
	list_add_tail(&(pTask->listEntry), &gCmdqContext.taskFreeList);
	CMDQ_MSG("cmdq_core_release_task_unlocked end\n");
}

static void cmdq_core_release_task(TaskStruct *pTask)
{
	CMDQ_MSG("-->TASK: Release task structure 0x%p begin\n", pTask);

	pTask->taskState = TASK_STATE_IDLE;
	pTask->thread = CMDQ_INVALID_THREAD;

	cmdq_core_release_buffer(pTask);

	mutex_lock(&gCmdqTaskMutex);

	/* remove from active/waiting list */
	list_del_init(&(pTask->listEntry));
	/* insert into free list. Currently we don't shrink free list. */
	list_add_tail(&(pTask->listEntry), &gCmdqContext.taskFreeList);

	mutex_unlock(&gCmdqTaskMutex);

	CMDQ_MSG("<--TASK: Release task structure end\n");
}

static void cmdq_core_release_task_in_queue(struct work_struct *workItem)
{
	TaskStruct *pTask = NULL;

	pTask = container_of(workItem, struct TaskStruct, autoReleaseWork);

	CMDQ_MSG("-->Work QUEUE: TASK: Release task structure 0x%p begin\n", pTask);
	if (pTask) {
		pTask->taskState = TASK_STATE_IDLE;
		pTask->thread = CMDQ_INVALID_THREAD;

		cmdq_core_release_buffer(pTask);

		mutex_lock(&gCmdqTaskMutex);

		/* remove from active/waiting list */
		list_del_init(&(pTask->listEntry));
		/* insert into free list. Currently we don't shrink free list. */
		list_add_tail(&(pTask->listEntry), &gCmdqContext.taskFreeList);

		mutex_unlock(&gCmdqTaskMutex);
	}

	CMDQ_MSG("<--Work QUEUE: TASK: Release task structure end\n");
}

static void cmdq_core_auto_release_task(TaskStruct *pTask)
{
	CMDQ_MSG("-->TASK: Auto release task structure 0x%p begin\n", pTask);

	if (pTask->useWorkQueue) {
		/* this is called via auto release work, no need to put in work queue again */
		cmdq_core_release_task(pTask);
	} else {
		/* Not auto release work, use for auto release task ! */
		/* the work item is embeded in pTask already */
		/* but we need to initialized it */
		INIT_WORK(&pTask->autoReleaseWork, cmdq_core_release_task_in_queue);
		pTask->useWorkQueue = true;
		queue_work(gCmdqContext.taskAutoReleaseWQ, &pTask->autoReleaseWork);
	}

	CMDQ_MSG("<--TASK: Auto release task structure end\n");
}

/**
 * Re-fetch thread's command buffer
 * Usage:
 *     If SW motifies command buffer content after SW configed command to GCE,
 *     SW should notify GCE to re-fetch command in order to ensure inconsistent command buffer content
 *     between DRAM and GCE's SRAM. */
void cmdq_core_invalidate_hw_fetched_buffer(int32_t thread)
{
	/* Setting HW thread PC will invoke that */
	/* GCE (CMDQ HW) gives up fetched command buffer, and fetch command from DRAM to GCE's SRAM again. */
	const int32_t pc = CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread));

	CMDQ_REG_SET32(CMDQ_THR_CURR_ADDR(thread), pc);
}

void cmdq_core_fix_command_scenario_for_user_space(cmdqCommandStruct *pCommand)
{
	if ((CMDQ_SCENARIO_USER_DISP_COLOR == pCommand->scenario)
	    || (CMDQ_SCENARIO_USER_MDP == pCommand->scenario)) {
		CMDQ_VERBOSE("user space request, scenario:%d\n", pCommand->scenario);
	} else {
		CMDQ_VERBOSE("[WARNING]fix user space request to CMDQ_SCENARIO_USER_SPACE\n");
		pCommand->scenario = CMDQ_SCENARIO_USER_SPACE;
	}
}

bool cmdq_core_is_request_from_user_space(const CMDQ_SCENARIO_ENUM scenario)
{
	switch (scenario) {
	case CMDQ_SCENARIO_USER_DISP_COLOR:
	case CMDQ_SCENARIO_USER_MDP:
	case CMDQ_SCENARIO_USER_SPACE:	/* phased out */
		return true;
	default:
		return false;
	}
	return false;
}

static void cmdq_core_append_command(TaskStruct *pTask, uint32_t argA, uint32_t argB)
{
	pTask->pCMDEnd[1] = argB;
	pTask->pCMDEnd[2] = argA;
	pTask->commandSize += 1 * CMDQ_INST_SIZE;
	pTask->pCMDEnd += 2;
}

static void cmdq_core_dump_task(const TaskStruct *pTask)
{
	CMDQ_ERR
	    ("Task: 0x%p, Scenario: %d, State: %d, Priority: %d, Flag: 0x%016llx, VABase: 0x%p\n",
	     pTask, pTask->scenario, pTask->taskState, pTask->priority, pTask->engineFlag,
	     pTask->pVABase);

	/* dump last Inst only when VALID command buffer */
	/* otherwise data abort is happened */
	if (pTask->pVABase) {
		CMDQ_ERR
		    ("CMDEnd: 0x%p, MVABase: %pa, Size: %d, Last Inst: 0x%08x:0x%08x, 0x%08x:0x%08x\n",
		     pTask->pCMDEnd, &pTask->MVABase, pTask->commandSize, pTask->pCMDEnd[-3],
		     pTask->pCMDEnd[-2], pTask->pCMDEnd[-1], pTask->pCMDEnd[0]);
	} else {
		CMDQ_ERR("CMDEnd: 0x%p, MVABase: %pa, Size: %d\n",
			 pTask->pCMDEnd, &pTask->MVABase, pTask->commandSize);
	}

	CMDQ_ERR("Reorder:%d, Trigger: %lld, Got IRQ: %lld, Wait: %lld, Finish: %lld\n",
		 pTask->reorder, pTask->trigger, pTask->gotIRQ, pTask->beginWait, pTask->wakedUp);
	CMDQ_ERR("Caller pid:%d name:%s\n", pTask->callerPid, pTask->callerName);
}

static void cmdq_core_dump_all_task(void)
{
	struct TaskStruct *ptr = NULL;
	struct list_head *p = NULL;

	mutex_lock(&gCmdqTaskMutex);

	CMDQ_ERR("=============== [CMDQ] All active tasks ===============\n");
	list_for_each(p, &gCmdqContext.taskActiveList) {
		ptr = list_entry(p, struct TaskStruct, listEntry);
		if (TASK_STATE_IDLE != ptr->taskState)
			cmdq_core_dump_task(ptr);
	}

	CMDQ_ERR("=============== [CMDQ] All wait tasks ===============\n");
	list_for_each(p, &gCmdqContext.taskWaitList) {
		ptr = list_entry(p, struct TaskStruct, listEntry);
		if (TASK_STATE_WAITING == ptr->taskState)
			cmdq_core_dump_task(ptr);
	}

	mutex_unlock(&gCmdqTaskMutex);
}

static void cmdq_core_insert_backup_instr(TaskStruct *pTask,
					  const uint32_t regAddr,
					  const dma_addr_t writeAddress,
					  const CMDQ_DATA_REGISTER_ENUM valueRegId,
					  const CMDQ_DATA_REGISTER_ENUM destRegId)
{
	uint32_t argA;
	int32_t subsysCode;

	/* register to read from */
	/* note that we force convert to physical reg address. */
	/* if it is already physical address, it won't be affected (at least on this platform) */
	argA = regAddr;
	subsysCode = cmdq_core_subsys_from_phys_addr(argA);

	/* CMDQ_ERR("test %d\n", __LINE__); */
	/*  */

	if (CMDQ_SPECIAL_SUBSYS_ADDR == subsysCode) {
		CMDQ_LOG("Backup: Special handle memory base address 0x%08x\n", argA);
		/* Move extra handle APB address to destRegId */
		cmdq_core_append_command(pTask,
					 (CMDQ_CODE_MOVE << 24) | ((destRegId & 0x1f) << 16) | (4 << 21),
					 argA);
		/* Use arg-A GPR enable instruction to read destRegId value to valueRegId */
		cmdq_core_append_command(pTask,
					 (CMDQ_CODE_READ << 24) | ((destRegId & 0x1f) << 16) | (6 << 21),
					 valueRegId);
	} else if (-1 == subsysCode) {
		CMDQ_ERR("Backup: Unsupported memory base address 0x%08x\n", argA);
	} else {
		/* Load into 32-bit GPR (R0-R15) */
		cmdq_core_append_command(pTask, (CMDQ_CODE_READ << 24) | (argA & 0xffff) |
					 ((subsysCode & 0x1f) << 16) | (2 << 21), valueRegId);
	}

	/* CMDQ_ERR("test %d\n", __LINE__); */

	/* Note that <MOVE> argB is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	cmdq_core_append_command(pTask, (CMDQ_CODE_MOVE << 24) |
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
				 ((writeAddress >> 32) & 0xffff) |
#endif
				 ((destRegId & 0x1f) << 16) | (4 << 21), (uint32_t) writeAddress);

	/* CMDQ_ERR("test %d\n", __LINE__); */

	/* write to memory */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_WRITE << 24) | (0 & 0xffff) |
				 ((destRegId & 0x1f) << 16) | (6 << 21), valueRegId);

	CMDQ_VERBOSE("COMMAND: copy reg:0x%08x to phys:%pa, GPR(%d, %d)\n", argA, &writeAddress,
		     valueRegId, destRegId);

	/* CMDQ_ERR("test %d\n", __LINE__); */
}

/**
 * Insert instruction to back secure threads' cookie count to normal world
 * Return:
 *     < 0, return the error code
 *     >=0, okay case, return number of bytes for inserting instruction
 */
#ifdef CMDQ_SECURE_PATH_NORMAL_IRQ
static int32_t cmdq_core_insert_backup_cookie_instr(TaskStruct *pTask, int32_t thread)
{
	const uint32_t originalSize = pTask->commandSize;
	const CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const CMDQ_DATA_REGISTER_ENUM destRegId = CMDQ_DATA_REG_DEBUG_DST;
	const uint32_t regAddr = CMDQ_THR_EXEC_CNT_PA(thread);
	uint64_t addrCookieOffset = CMDQ_SEC_SHARED_THR_CNT_OFFSET + thread * sizeof(uint32_t);
	uint64_t WSMCookieAddr = gCmdqContext.hSecSharedMem->MVABase + addrCookieOffset;
	const uint32_t subsysBit = cmdq_get_func()->getSubsysLSBArgA();
	int32_t subsysCode = cmdq_core_subsys_from_phys_addr(regAddr);
	int32_t offset;

	if (0 > cmdq_get_func()->isSecureThread(thread)) {
		CMDQ_ERR("%s, invalid param, thread: %d\n", __func__, thread);
		return -EFAULT;
	}

	if (NULL == gCmdqContext.hSecSharedMem) {
		CMDQ_ERR("%s, shared memory is not created\n", __func__);
		return -EFAULT;
	}

	/* Shift JUMP and EOC */
	pTask->pCMDEnd[10] = pTask->pCMDEnd[0];
	pTask->pCMDEnd[9] = pTask->pCMDEnd[-1];
	pTask->pCMDEnd[8] = pTask->pCMDEnd[-2];
	pTask->pCMDEnd[7] = pTask->pCMDEnd[-3];

	pTask->pCMDEnd -= 4;

	/* use SYNC TOKEN to make sure only 1 thread access at a time */
	/* bit 0-11: wait_value */
	/* bit 15: to_wait, true */
	/* bit 31: to_update, true */
	/* bit 16-27: update_value */
	/* wait and clear */
	cmdq_core_append_command(pTask, (CMDQ_CODE_WFE << 24) | regAccessToken,
				 ((1 << 31) | (1 << 15) | 1));

	/* Load into 32-bit GPR (R0-R15) */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_READ << 24) | (regAddr & 0xffff) |
				 ((subsysCode & 0x1f) << subsysBit) | (2 << 21), valueRegId);

	/* Note that <MOVE> argB is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_MOVE << 24) | ((WSMCookieAddr >> 32) & 0xffff) |
				 ((destRegId & 0x1f) << 16) | (4 << 21), (uint32_t) WSMCookieAddr);

	/* write to memory */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_WRITE << 24) |
				 ((destRegId & 0x1f) << 16) | (6 << 21), valueRegId);

	/* set directly */
	cmdq_core_append_command(pTask, (CMDQ_CODE_WFE << 24) | regAccessToken,
				 ((1 << 31) | (1 << 16)));

	pTask->pCMDEnd += 4;

	/* calculate added command length */
	offset = pTask->commandSize - originalSize;

	CMDQ_VERBOSE("insert_backup_cookie, offset:%d\n", offset);

	return offset;
}
#endif

/**
 * Insert instruction to backup secure threads' cookie and IRQ to normal world
 * Return:
 *     < 0, return the error code
 *     >=0, okay case, return number of bytes for inserting instruction
 */
#ifdef CMDQ_SECURE_PATH_HW_LOCK
static int32_t cmdq_core_insert_secure_IRQ_instr(TaskStruct *pTask, int32_t thread)
{
	const uint32_t originalSize = pTask->commandSize;
	const CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const CMDQ_DATA_REGISTER_ENUM destRegId = CMDQ_DATA_REG_DEBUG_DST;
	const uint32_t regAddr = CMDQ_THR_EXEC_CNT_PA(thread);
	uint64_t addrCookieOffset = CMDQ_SEC_SHARED_THR_CNT_OFFSET + thread * sizeof(uint32_t);
	uint64_t WSMCookieAddr = gCmdqContext.hSecSharedMem->MVABase + addrCookieOffset;
	uint64_t WSMIRQAddr = gCmdqContext.hSecSharedMem->MVABase + CMDQ_SEC_SHARED_IRQ_RAISED_OFFSET;
	const uint32_t subsysBit = cmdq_get_func()->getSubsysLSBArgA();
	int32_t subsysCode = cmdq_core_subsys_from_phys_addr(regAddr);
	int32_t offset;

	if (0 > cmdq_get_func()->isSecureThread(thread)) {
		CMDQ_ERR("%s, invalid param, thread: %d\n", __func__, thread);
		return -EFAULT;
	}

	if (NULL == gCmdqContext.hSecSharedMem) {
		CMDQ_ERR("%s, shared memory is not created\n", __func__);
		return -EFAULT;
	}

	/* Shift JUMP and EOC */
	pTask->pCMDEnd[22] = pTask->pCMDEnd[0];
	pTask->pCMDEnd[21] = pTask->pCMDEnd[-1];
	pTask->pCMDEnd[20] = pTask->pCMDEnd[-2];
	pTask->pCMDEnd[19] = pTask->pCMDEnd[-3];

	pTask->pCMDEnd -= 4;

	/* use SYNC TOKEN to make sure only 1 thread access at a time */
	/* bit 0-11: wait_value */
	/* bit 15: to_wait, true */
	/* bit 31: to_update, true */
	/* bit 16-27: update_value */
	/* wait and clear */
	/* set unlock WSM resource directly */
	cmdq_core_append_command(pTask, (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_SECURE_WSM_LOCK,
				 ((1 << 31) | (1 << 15) | 1));

	cmdq_core_append_command(pTask, (CMDQ_CODE_WFE << 24) | regAccessToken,
				 ((1 << 31) | (1 << 15) | 1));

	/* Load into 32-bit GPR (R0-R15) */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_READ << 24) | (regAddr & 0xffff) |
				 ((subsysCode & 0x1f) << subsysBit) | (2 << 21), valueRegId);

	/* Note that <MOVE> argB is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_MOVE << 24) | ((WSMCookieAddr >> 32) & 0xffff) |
				 ((destRegId & 0x1f) << 16) | (4 << 21), (uint32_t) WSMCookieAddr);

	/* write to memory */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_WRITE << 24) |
				 ((destRegId & 0x1f) << 16) | (6 << 21), valueRegId);

	/* Write GCE secure thread's IRQ to WSM */
	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_MOVE << 24), ~(1 << thread));

	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_MOVE << 24) |
				 ((destRegId & 0x1f) << 16) | (4 << 21), WSMIRQAddr);

	cmdq_core_append_command(pTask,
				 (CMDQ_CODE_WRITE << 24) |
				 ((destRegId & 0x1f) << 16) | (4 << 21) | 1, (1 << thread));

	/* set directly */
	cmdq_core_append_command(pTask, (CMDQ_CODE_WFE << 24) | regAccessToken,
				 ((1 << 31) | (1 << 16)));

	/* set unlock WSM resource directly */
	cmdq_core_append_command(pTask, (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_SECURE_WSM_LOCK,
				 ((1 << 31) | (1 << 16)));

	/* set notify thread token directly */
	cmdq_core_append_command(pTask, (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_SECURE_THR_EOF,
				 ((1 << 31) | (1 << 16)));

	pTask->pCMDEnd += 4;

	/* secure thread doesn't raise IRQ */
	pTask->pCMDEnd[-3] = pTask->pCMDEnd[-3] & 0xFFFFFFFE;

	/* calculate added command length */
	offset = pTask->commandSize - originalSize;

	CMDQ_VERBOSE("insert_backup_cookie, offset:%d\n", offset);

	return offset;
}
#endif

static int32_t cmdq_core_insert_secure_handle_instr(TaskStruct *pTask, int32_t thread)
{
#ifdef CMDQ_SECURE_PATH_HW_LOCK
	return cmdq_core_insert_secure_IRQ_instr(pTask, thread);
#else
#ifdef CMDQ_SECURE_PATH_NORMAL_IRQ
	return cmdq_core_insert_backup_cookie_instr(pTask, thread);
#else
	return 0;
#endif
#endif
}

static void cmdq_core_reorder_task_array(ThreadStruct *pThread, int32_t thread, int32_t prevID)
{
	int loop, nextID, searchLoop, searchID;
	int reorderCount = 0;

	nextID = prevID + 1;
	for (loop = 1; loop < (cmdq_core_max_task_in_thread(thread) - 1); loop++, nextID++) {
		if (nextID >= cmdq_core_max_task_in_thread(thread))
			nextID = 0;

		if (NULL != pThread->pCurTask[nextID])
			break;

		searchID = nextID + 1;
		for (searchLoop = (loop + 1); searchLoop < cmdq_core_max_task_in_thread(thread);
		     searchLoop++, searchID++) {
			if (searchID >= cmdq_core_max_task_in_thread(thread))
				searchID = 0;

			if (NULL != pThread->pCurTask[searchID]) {
				pThread->pCurTask[nextID] = pThread->pCurTask[searchID];
				pThread->pCurTask[searchID] = NULL;
				CMDQ_VERBOSE("WAIT: reorder slot %d to slot 0%d.\n",
					     searchID, nextID);
				if ((searchLoop - loop) > reorderCount)
					reorderCount = searchLoop - loop;

				break;
			}
		}

		if ((0x10000000 == pThread->pCurTask[nextID]->pCMDEnd[0]) &&
		    (0x00000008 == pThread->pCurTask[nextID]->pCMDEnd[-1])) {
			/* We reached the last task */
			break;
		}
	}

	pThread->nextCookie -= reorderCount;
	CMDQ_VERBOSE("WAIT: nextcookie minus %d.\n", reorderCount);
}

static int32_t cmdq_core_copy_buffer_impl(void *dst, void *src, const uint32_t size,
					  const bool copyFromUser)
{
	int32_t status = 0;

	if (false == copyFromUser) {
		CMDQ_VERBOSE("COMMAND: Copy kernel to 0x%p\n", dst);
		memcpy(dst, src, size);
	} else {
		CMDQ_VERBOSE("COMMAND: Copy user to 0x%p\n", dst);
		if (copy_from_user(dst, src, size)) {
			CMDQ_AEE("CMDQ",
				 "CRDISPATCH_KEY:CMDQ Fail to copy from user 0x%p, size:%d\n",
				 src, size);
			status = -ENOMEM;
		}
	}

	return status;
}

bool cmdq_core_verfiy_command_desc_end(cmdqCommandStruct *pCommandDesc)
{
	uint32_t *pCMDEnd = NULL;
	bool valid = true;
	/* make sure we have sufficient command to parse */
	if (!CMDQ_U32_PTR(pCommandDesc->pVABase) || pCommandDesc->blockSize < (2 * CMDQ_INST_SIZE))
		return false;

	if (true == cmdq_core_is_request_from_user_space(pCommandDesc->scenario)) {
		/* command buffer has not copied from user space yet, skip verify. */
		return true;
	}

	pCMDEnd =
	    CMDQ_U32_PTR(pCommandDesc->pVABase) + (pCommandDesc->blockSize / sizeof(uint32_t)) - 1;

	/* make sure the command is ended by EOC + JUMP */
	if ((pCMDEnd[-3] & 0x1) != 1) {
		CMDQ_ERR
		    ("[CMD] command desc 0x%p does not throw IRQ (%08x:%08x), pEnd:%p(%p, %d)\n",
		     pCommandDesc, pCMDEnd[-3], pCMDEnd[-2], pCMDEnd,
		     CMDQ_U32_PTR(pCommandDesc->pVABase), pCommandDesc->blockSize);
		valid = false;
	}

	if (((pCMDEnd[-2] & 0xFF000000) >> 24) != CMDQ_CODE_EOC ||
	    ((pCMDEnd[0] & 0xFF000000) >> 24) != CMDQ_CODE_JUMP) {
		CMDQ_ERR
		    ("[CMD] command desc 0x%p does not end in EOC+JUMP (%08x:%08x, %08x:%08x), pEnd:%p(%p, %d)\n",
		     pCommandDesc, pCMDEnd[-3], pCMDEnd[-2], pCMDEnd[-1], pCMDEnd[0], pCMDEnd,
		     CMDQ_U32_PTR(pCommandDesc->pVABase), pCommandDesc->blockSize);
		valid = false;
	}
#if 0
	BUG_ON(!valid);
#else
	if (false == valid) {
		/* invalid command, raise AEE */
		CMDQ_AEE("CMDQ", "INVALID command desc 0x%p\n", pCommandDesc);
	}
#endif

	return valid;
}

bool cmdq_core_verfiy_command_end(const TaskStruct *pTask)
{
	bool valid = true;
	bool noIRQ = false;
	/* make sure we have sufficient command to parse */
	if (!pTask->pVABase || pTask->commandSize < (2 * CMDQ_INST_SIZE))
		return false;

#ifdef CMDQ_SECURE_PATH_HW_LOCK
	if ((pTask->pCMDEnd[-3] & 0x1) != 1 && false == pTask->secData.isSecure)
		noIRQ = true;
#else
	if ((pTask->pCMDEnd[-3] & 0x1) != 1)
		noIRQ = true;
#endif

	/* make sure the command is ended by EOC + JUMP */
	if (true == noIRQ) {
		if (cmdq_get_func()->isLoopScenario(pTask->scenario, true)) {
			/* Allow display only loop not  throw IRQ */
			CMDQ_MSG("[CMD] DISP Loop pTask 0x%p does not throw IRQ (%08x:%08x)\n",
				 pTask, pTask->pCMDEnd[-3], pTask->pCMDEnd[-2]);
		} else {
			CMDQ_ERR("[CMD] pTask 0x%p does not throw IRQ (%08x:%08x)\n",
				 pTask, pTask->pCMDEnd[-3], pTask->pCMDEnd[-2]);
			valid = false;
		}
	}
	if (((pTask->pCMDEnd[-2] & 0xFF000000) >> 24) != CMDQ_CODE_EOC ||
	    ((pTask->pCMDEnd[0] & 0xFF000000) >> 24) != CMDQ_CODE_JUMP) {

		CMDQ_ERR("[CMD] pTask 0x%p does not end in EOC+JUMP (%08x:%08x, %08x:%08x)\n",
			 pTask,
			 pTask->pCMDEnd[-3], pTask->pCMDEnd[-2], pTask->pCMDEnd[-1],
			 pTask->pCMDEnd[0]);
		valid = false;
	}
#if 0
	BUG_ON(!valid);
#else
	if (false == valid) {
		/* Raise AEE */
		CMDQ_AEE("CMDQ", "INVALID pTask 0x%p\n", pTask);
	}
#endif

	return valid;
}

static TaskStruct *cmdq_core_find_free_task(void)
{
	TaskStruct *pTask = NULL;

	mutex_lock(&gCmdqTaskMutex);

	/* Pick from free list first; */
	/* create one if there is no free entry. */
	if (list_empty(&gCmdqContext.taskFreeList)) {
		pTask = cmdq_core_task_create();
	} else {
		pTask = list_first_entry(&(gCmdqContext.taskFreeList), TaskStruct, listEntry);
		/* remove from free list */
		list_del_init(&(pTask->listEntry));
	}

	mutex_unlock(&gCmdqTaskMutex);

	return pTask;
}

static int32_t cmdq_core_insert_read_reg_command(TaskStruct *pTask,
						 cmdqCommandStruct *pCommandDesc)
{
	/* #define CMDQ_PROFILE_COMMAND */

	int32_t status = 0;
	uint32_t extraBufferSize = 0;
	int i = 0;
	CMDQ_DATA_REGISTER_ENUM valueRegId;
	CMDQ_DATA_REGISTER_ENUM destRegId;
	CMDQ_EVENT_ENUM regAccessToken;
	uint32_t prependBufferSize = 0;
	const bool userSpaceRequest = cmdq_core_is_request_from_user_space(pTask->scenario);
	bool isValidCommandEnd = true;

	/* HACK: let blockSize >= commandSize + 1 * CMDQ_INST_SIZE to prevnet GCE stop wrongly */
	/* ALPS01676155, queue 2 tasks in same HW thread */
	/* .task_2: MVABase 0x0x435b3000, Size: 2944 */
	/* .task_3: MVABase 0x0x435b2000, Size: 4096 */
	/* THR_END = end of task_3
	 *         = 0x0x435b3000 = 0x0x435b2000 + 4096
	 *         = begin of task_2, so...GCE stop at begin of task_2's command and never exec task_2 & task_3 */
	const uint32_t reservedAppendBufferSize = 1 * CMDQ_INST_SIZE;

#ifdef CMDQ_PROFILE_COMMAND
	bool profileCommand = false;
#endif
	uint32_t copiedBeforeProfile = 0;
	int32_t subsysCode;
	uint32_t physAddr;

	uint32_t reservedAppendBlockInstrSize = 0;
	uint32_t reservedBackCookieInstrSize = 0;

#ifdef CMDQ_APPEND_WITHOUT_SUSPEND
	/* HACK: Insert WAIT CMDQ APPEND event here */
	reservedAppendBlockInstrSize = 1 * CMDQ_INST_SIZE;
#endif

	/* HACK: Insert BACKUP secure threads' COOKIE and IRQ here */
	if (true == pTask->secData.isSecure) {
		/* we need to consider {READ, MOVE, WRITE} for each register */
		/* and the SYNC in the begin and end */
#ifdef CMDQ_SECURE_PATH_HW_LOCK
		reservedBackCookieInstrSize = (3 + 3 + 2 + 2 + 1) * CMDQ_INST_SIZE;
#else
#ifdef CMDQ_SECURE_PATH_NORMAL_IRQ
		reservedBackCookieInstrSize = (3 + 2) * CMDQ_INST_SIZE;
#endif
#endif
	}

	/* calculate required buffer size */
	/* we need to consider {READ, MOVE, WRITE} for each register */
	/* and the SYNC in the begin and end */
	if (pTask->regCount) {
		extraBufferSize = (3 * CMDQ_INST_SIZE * pTask->regCount) + (2 * CMDQ_INST_SIZE);
		/* Add move instruction count for handle Extra APB address (add move instructions) */
		for (i = 0; i < pTask->regCount; ++i) {
			physAddr = CMDQ_U32_PTR(pCommandDesc->regRequest.regAddresses)[i];
			subsysCode = cmdq_core_subsys_from_phys_addr(physAddr);
			if (CMDQ_SPECIAL_SUBSYS_ADDR == subsysCode)
				extraBufferSize += CMDQ_INST_SIZE;
		}
	} else {
		extraBufferSize = 0;
	}

	CMDQ_VERBOSE("test %d, original command size = %d\n", __LINE__, pTask->commandSize);

#ifdef CMDQ_PROFILE_COMMAND
	/* do not insert profile command for trigger loop */
#ifdef CMDQ_PROFILE_COMMAND_TRIGGER_LOOP
	profileCommand = pTask &&
	    cmdq_get_func()->shouldProfile(pTask->scenario) && (pTask->profileData != NULL);
#else
	profileCommand = pTask &&
	    cmdq_get_func()->shouldProfile(pTask->scenario) &&
	    (pTask->loopCallback == NULL) && (pTask->profileData != NULL);
#endif

	if (profileCommand) {
		/* backup GPT at begin and end */
		extraBufferSize += (CMDQ_INST_SIZE * 3);

		/* insert after the first MARKER instruction */
		/* and first SYNC instruction (if any) */
		prependBufferSize += 2 * (CMDQ_INST_SIZE * 3);
		if (pTask->commandSize < prependBufferSize)
			prependBufferSize = 0;
	}
#endif

	status = cmdq_core_task_realloc_buffer_size(pTask,
						    pTask->commandSize + prependBufferSize +
						    extraBufferSize + reservedAppendBufferSize +
						    reservedAppendBlockInstrSize +
						    reservedBackCookieInstrSize);
	if (status < 0) {
		CMDQ_ERR("finalize command buffer failed to realloc, pTask=0x%p, requireSize=%d\n",
			 pTask, pTask->commandSize + prependBufferSize + extraBufferSize);
		return status;
	}

	/* init pCMDEnd */
	/* pCMDEnd start from beginning. Note it is out-of-sync with pTask->commandSize. */
	pTask->pCMDEnd = pTask->pVABase - 1;


#ifdef CMDQ_PROFILE_COMMAND

	if (profileCommand) {
		if (cmdq_get_func()->shouldEnablePrefetch(pTask->scenario)) {
			/* HACK: */
			/* MARKER + WAIT_FOR_EOF */
			copiedBeforeProfile = 2 * CMDQ_INST_SIZE;
		} else if (pTask->loopCallback != NULL) {
#ifdef CMDQ_PROFILE_COMMAND_TRIGGER_LOOP
			/* HACK: insert profile instr after WAIT TE_EOF (command mode only) */
			/* note content of trigger loop varies according to platform */
			copiedBeforeProfile = 8 * CMDQ_INST_SIZE;
#endif
		} else if (true == userSpaceRequest) {
			copiedBeforeProfile = 0;
		} else {
			/* HACK: we copy the 1st "WAIT FOR EOF" instruction, */
			/* because this is the point where we start writing registers! */
			copiedBeforeProfile = 1 * CMDQ_INST_SIZE;
		}


		if (0 < copiedBeforeProfile) {
			cmdq_core_copy_buffer_impl(pTask->pCMDEnd + 1,
						   CMDQ_U32_PTR(pCommandDesc->pVABase),
						   copiedBeforeProfile, userSpaceRequest);
			pTask->pCMDEnd =
			    pTask->pVABase + (copiedBeforeProfile / sizeof(pTask->pVABase[0])) - 1;
		}

		/* now we start insert backup instructions */
		CMDQ_VERBOSE("test %d\n", __LINE__);
		do {
			CMDQ_VERBOSE("[BACKUP]va=%p, pa=%pa, task=%p\n", pTask->profileData,
				     &pTask->profileDataPA, pTask);

			cmdq_core_insert_backup_instr(pTask,
						      CMDQ_APXGPT2_COUNT,
						      pTask->profileDataPA,
						      CMDQ_DATA_REG_JPEG, CMDQ_DATA_REG_JPEG_DST);
		} while (0);
		/* this increases pTask->commandSize */
	}
#endif

	/* Copy the commands to our DMA buffer */
	status = cmdq_core_copy_buffer_impl(pTask->pCMDEnd + 1,
					    CMDQ_U32_PTR(pCommandDesc->pVABase) +
					    (copiedBeforeProfile /
					     sizeof(CMDQ_U32_PTR(pCommandDesc->pVABase)[0])),
					    pCommandDesc->blockSize - copiedBeforeProfile,
					    userSpaceRequest);
	if (0 > status)
		return status;

	/* re-adjust pCMDEnd according to commandSize */
	pTask->pCMDEnd = pTask->pVABase + (pTask->commandSize / sizeof(pTask->pVABase[0])) - 1;

	/* make sure instructions are really in DRAM */
	smp_mb();

	CMDQ_VERBOSE("test %d, CMDEnd=%p, base=%p, cmdSize=%d\n", __LINE__, pTask->pCMDEnd,
		     pTask->pVABase, pTask->commandSize);

	isValidCommandEnd = cmdq_core_verfiy_command_end(pTask);
	if (!isValidCommandEnd) {
		CMDQ_ERR("[CMD] with smp_mb() cmdSize=%d, blockSize:%d\n", pTask->commandSize,
			 pCommandDesc->blockSize);
		cmdq_core_dump_task(pTask);
		cmdq_core_dump_all_task();
	}

	/* if no read request, no post-process needed. */
	if (0 == pTask->regCount && extraBufferSize == 0)
		return 0;

	/* move EOC+JUMP to the new end */
	memcpy(pTask->pCMDEnd + 1 - 4 + (extraBufferSize / sizeof(pTask->pCMDEnd[0])),
	       &pTask->pCMDEnd[-3], 2 * CMDQ_INST_SIZE);

	/* start from old EOC (replace it) */
	pTask->pCMDEnd -= 4;

	if (pTask->regCount) {
		CMDQ_VERBOSE("COMMAND:allocate register output section\n");
		/* allocate register output section */
		BUG_ON(pTask->regResults);
		pTask->regResults = cmdq_core_alloc_hw_buffer(cmdq_dev_get(),
							      pTask->regCount * sizeof(pTask->regResults[0]),
							      &pTask->regResultsMVA,
							      GFP_KERNEL);
		CMDQ_MSG("COMMAND: result buf VA:0x%p, PA:%pa\n", pTask->regResults,
			 &pTask->regResultsMVA);



		/* allocate GPR resource */
		cmdq_get_func()->getRegID(pTask->engineFlag, &valueRegId, &destRegId,
					  &regAccessToken);

		/* use SYNC TOKEN to make sure only 1 thread access at a time */
		/* bit 0-11: wait_value */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value */

		/* wait and clear */
		cmdq_core_append_command(pTask,
					 (CMDQ_CODE_WFE << 24) | regAccessToken,
					 ((1 << 31) | (1 << 15) | 1));

		for (i = 0; i < pTask->regCount; ++i) {
			cmdq_core_insert_backup_instr(pTask,
						      CMDQ_U32_PTR(pCommandDesc->regRequest.regAddresses)[i],
						      pTask->regResultsMVA + (i * sizeof(pTask->regResults[0])),
						      valueRegId, destRegId);
		}

		/* set directly */
		cmdq_core_append_command(pTask,
					 (CMDQ_CODE_WFE << 24) | regAccessToken,
					 ((1 << 31) | (1 << 16)));
	}
#ifdef CMDQ_PROFILE_COMMAND
	if (profileCommand) {
		cmdq_core_insert_backup_instr(pTask,
					      CMDQ_APXGPT2_COUNT,
					      pTask->profileDataPA + sizeof(uint32_t),
					      CMDQ_DATA_REG_JPEG, CMDQ_DATA_REG_JPEG_DST);
	}
	/* this increases pTask->commandSize */
#endif

	/* move END to copied EOC+JUMP */
	pTask->pCMDEnd += 4;

#ifdef CMDQ_PROFILE_COMMAND_TRIGGER_LOOP
	/* revise jump */
	if (pTask && pTask->loopCallback) {
		if (((pTask->pCMDEnd[0] & 0xFF000000) >> 24) == CMDQ_CODE_JUMP) {
			/* The Last Instruction is JUMP */
			pTask->pCMDEnd[-1] = -pTask->commandSize + CMDQ_INST_SIZE;
		} else {
			CMDQ_ERR("trigger loop error since profile\n");
		}
	}
#endif

	/* make sure instructions are really in DRAM */
	smp_mb();

#ifdef CMDQ_PROFILE_COMMAND_TRIGGER_LOOP
	if (pTask && pTask->loopCallback && cmdq_get_func()->shouldProfile(pTask->scenario))
		cmdqCoreDebugDumpCommand(pTask);
#endif

	CMDQ_MSG("COMMAND: size = %d, end = 0x%p\n", pTask->commandSize, pTask->pCMDEnd);

	isValidCommandEnd = cmdq_core_verfiy_command_end(pTask);
	if (!isValidCommandEnd) {
		CMDQ_ERR("[CMD] cmdSize=%d, blockSize:%d\n", pTask->commandSize,
			 pCommandDesc->blockSize);
		cmdq_core_dump_task(pTask);
	}

	return status;
}

static TaskStruct *cmdq_core_acquire_task(cmdqCommandStruct *pCommandDesc,
					  CmdqInterruptCB loopCB, unsigned long loopData)
{
	TaskStruct *pTask = NULL;
	int32_t status;

	CMDQ_MSG("-->TASK: acquire task begin CMD: 0x%p, size: %d, Eng: 0x%016llx\n",
		 CMDQ_U32_PTR(pCommandDesc->pVABase), pCommandDesc->blockSize,
		 pCommandDesc->engineFlag);
	CMDQ_PROF_START(current->pid, __func__);

	pTask = cmdq_core_find_free_task();
	do {
		if (NULL == pTask) {
			CMDQ_AEE("CMDQ", "Can't acquire task info\n");
			break;
		}

		/* initialize field values */
		pTask->scenario = pCommandDesc->scenario;
		pTask->priority = pCommandDesc->priority;
		pTask->engineFlag = pCommandDesc->engineFlag;
		pTask->privateData = (void *)(unsigned long long *)&pCommandDesc->privateData;
		pTask->loopCallback = loopCB;
		pTask->loopData = loopData;
		pTask->taskState = TASK_STATE_WAITING;
		pTask->reorder = 0;
		pTask->thread = CMDQ_INVALID_THREAD;
		pTask->irqFlag = 0x0;
		pTask->useWorkQueue = false;

		/* secure exec data */
		pTask->secData.isSecure = pCommandDesc->secData.isSecure;
		pTask->secData.enginesNeedDAPC = pCommandDesc->secData.enginesNeedDAPC;
		pTask->secData.enginesNeedPortSecurity =
		    pCommandDesc->secData.enginesNeedPortSecurity;
		pTask->secData.addrMetadataCount = pCommandDesc->secData.addrMetadataCount;
		pTask->secData.addrMetadatas = pCommandDesc->secData.addrMetadatas;

		/* profile data for command profiling */
		if (cmdq_get_func()->shouldProfile(pTask->scenario)) {
			pTask->profileData =
			    cmdq_core_alloc_hw_buffer(cmdq_dev_get(),
						      2 * sizeof(uint32_t), &pTask->profileDataPA,
						      GFP_KERNEL);
		} else {
			pTask->profileData = NULL;
		}

		/* profile timers */
		memset(&(pTask->trigger), 0x0, sizeof(pTask->trigger));
		memset(&(pTask->gotIRQ), 0x0, sizeof(pTask->gotIRQ));
		memset(&(pTask->beginWait), 0x0, sizeof(pTask->beginWait));
		memset(&(pTask->wakedUp), 0x0, sizeof(pTask->wakedUp));

		/* profile marker */
		cmdq_task_init_profile_marker_data(pCommandDesc, pTask);

		pTask->commandSize = pCommandDesc->blockSize;
		pTask->regCount = pCommandDesc->regRequest.count;

		/* store caller info for debug */
		if (current) {
			pTask->callerPid = current->pid;
			memcpy(pTask->callerName, current->comm, sizeof(current->comm));
		}

		status = cmdq_core_insert_read_reg_command(pTask, pCommandDesc);
		if (0 > status) {
			/* raise AEE first */
			CMDQ_AEE("CMDQ", "Can't alloc command buffer\n");

			/* then release task */
			cmdq_core_release_task(pTask);
			pTask = NULL;
		}
	} while (0);

	/*  */
	/* insert into waiting list to process */
	/*  */
	mutex_lock(&gCmdqTaskMutex);
	if (pTask) {
		struct list_head *insertAfter = &gCmdqContext.taskWaitList;

		struct TaskStruct *taskEntry = NULL;
		struct list_head *p = NULL;

		pTask->submit = sched_clock();

		/* add to waiting list, keep it sorted by priority */
		/* so that we add high-priority tasks first. */
		list_for_each(p, &gCmdqContext.taskWaitList) {
			taskEntry = list_entry(p, struct TaskStruct, listEntry);
			/* keep the list sorted. */
			/* higher priority tasks are inserted in front of the queue */
			if (taskEntry->priority < pTask->priority)
				break;

			insertAfter = p;
		}

		list_add(&(pTask->listEntry), insertAfter);
	}
	mutex_unlock(&gCmdqTaskMutex);

	CMDQ_MSG("<--TASK: acquire task 0x%p end\n", pTask);
	CMDQ_PROF_END(current->pid, __func__);
	return pTask;
}

static void cmdq_core_enable_common_clock_locked(const bool enable,
						 const uint64_t engineFlag,
						 CMDQ_SCENARIO_ENUM scenario)
{
	/* CMDQ(GCE) clock */
	if (enable) {
		CMDQ_VERBOSE("[CLOCK] Enable CMDQ(GCE) Clock test=%d SMI %d\n",
			     atomic_read(&gCmdqThreadUsage), atomic_read(&gSMIThreadUsage));

		if (0 == atomic_read(&gCmdqThreadUsage)) {
			/* CMDQ init flow: */
			/* 1. clock-on */
			/* 2. reset all events */
			cmdq_get_func()->enableGCEClockLocked(enable);
			cmdq_core_reset_hw_events();
#ifdef CMDQ_ENABLE_BUS_ULTRA
			CMDQ_LOG("Enable GCE Ultra ability");
			CMDQ_REG_SET32(CMDQ_BUS_CONTROL_TYPE, 0x3);
#endif
#ifdef CMDQ_EVENT_NEED_BACKUP
			/* Restore event */
			cmdq_get_func()->eventRestore();
#endif

		}
		atomic_inc(&gCmdqThreadUsage);

		/* SMI related threads common clock enable, excluding display scenario on his own */
		if (!cmdq_get_func()->isDispScenario(scenario)) {
			if (0 == atomic_read(&gSMIThreadUsage)) {
				CMDQ_VERBOSE("[CLOCK] SMI clock enable %d\n", scenario);
				cmdq_get_func()->enableCommonClockLocked(enable);
			}
			atomic_inc(&gSMIThreadUsage);
		}

	} else {
		atomic_dec(&gCmdqThreadUsage);

		CMDQ_VERBOSE("[CLOCK] Disable CMDQ(GCE) Clock test=%d SMI %d\n",
			     atomic_read(&gCmdqThreadUsage), atomic_read(&gSMIThreadUsage));
		if (0 >= atomic_read(&gCmdqThreadUsage)) {
#ifdef CMDQ_EVENT_NEED_BACKUP
			/* Backup event */
			cmdq_get_func()->eventBackup();
#endif
			/* clock-off */
			cmdq_get_func()->enableGCEClockLocked(enable);
		}

		/* SMI related threads common clock enable, excluding display scenario on his own */
		if (!cmdq_get_func()->isDispScenario(scenario)) {
			atomic_dec(&gSMIThreadUsage);

			if (0 >= atomic_read(&gSMIThreadUsage)) {
				CMDQ_VERBOSE("[CLOCK] SMI clock disable %d\n", scenario);
				cmdq_get_func()->enableCommonClockLocked(enable);
			}
		}
	}
}

static uint64_t cmdq_core_get_actual_engine_flag_for_enable_clock(uint64_t engineFlag,
								  int32_t thread)
{
	EngineStruct *pEngine;
	ThreadStruct *pThread;
	uint64_t engines;
	int32_t index;

	pEngine = gCmdqContext.engine;
	pThread = gCmdqContext.thread;
	engines = 0;
	for (index = 0; index < CMDQ_MAX_ENGINE_COUNT; index++) {
		if (engineFlag & (1LL << index)) {
			if (pEngine[index].userCount <= 0) {
				pEngine[index].currOwner = thread;
				engines |= (1LL << index);
				/* also assign engine flag into ThreadStruct */
				pThread[thread].engineFlag |= (1LL << index);
			}

			pEngine[index].userCount++;
		}
	}
	return engines;
}

static int32_t gCmdqISPClockCounter;

static void cmdq_core_enable_clock(uint64_t engineFlag,
				   int32_t thread,
				   uint64_t engineMustEnableClock, CMDQ_SCENARIO_ENUM scenario)
{
	const uint64_t engines = engineMustEnableClock;
	int32_t index;
	CmdqCBkStruct *pCallback;
	int32_t status;

	CMDQ_VERBOSE("-->CLOCK: Enable flag 0x%llx thread %d begin, mustEnable: 0x%llx(0x%llx)\n",
		     engineFlag, thread, engineMustEnableClock, engines);

	/* enable fundamental clocks if needed */
	cmdq_core_enable_common_clock_locked(true, engineFlag, scenario);

	pCallback = gCmdqGroupCallback;

	/* ISP special check: Always call ISP on/off if this task */
	/* involves ISP. Ignore the ISP HW flags. */
	if (cmdq_core_is_group_flag(CMDQ_GROUP_ISP, engineFlag)) {
		CMDQ_VERBOSE("CLOCK: enable group %d clockOn\n", CMDQ_GROUP_ISP);

		if (NULL == pCallback[CMDQ_GROUP_ISP].clockOn) {
			CMDQ_ERR("CLOCK: enable group %d clockOn func NULL\n", CMDQ_GROUP_ISP);
		} else {
			status =
			    pCallback[CMDQ_GROUP_ISP].clockOn(gCmdqEngineGroupBits[CMDQ_GROUP_ISP] &
							      engineFlag);

#if 1
			++gCmdqISPClockCounter;
#endif

			if (status < 0) {
				/* Error status print */
				CMDQ_ERR("CLOCK: enable group %d clockOn failed\n", CMDQ_GROUP_ISP);
			}
		}
	}

	for (index = CMDQ_MAX_GROUP_COUNT - 1; index >= 0; --index) {
		/* note that DISPSYS controls their own clock on/off */
		if (CMDQ_GROUP_DISP == index)
			continue;

		/* note that ISP is per-task on/off, not per HW flag */
		if (CMDQ_GROUP_ISP == index)
			continue;

		if (cmdq_core_is_group_flag((CMDQ_GROUP_ENUM) index, engines)) {
			CMDQ_MSG("CLOCK: enable group %d clockOn\n", index);
			if (NULL == pCallback[index].clockOn) {
				CMDQ_LOG("[WARNING]CLOCK: enable group %d clockOn func NULL\n",
					 index);
				continue;
			}
			status = pCallback[index].clockOn(gCmdqEngineGroupBits[index] & engines);
			if (status < 0) {
				/* Error status print */
				CMDQ_ERR("CLOCK: enable group %d clockOn failed\n", index);
			}
		}
	}

	CMDQ_MSG("<--CLOCK: Enable hardware clock end\n");
}

static const int32_t cmdq_core_can_start_to_acquire_HW_thread_unlocked(const uint64_t
								       engineFlag,
								       const bool isSecure)
{
	struct TaskStruct *pFirstWaitingTask = NULL;
	struct TaskStruct *pTempTask = NULL;
	struct list_head *p = NULL;
	bool preferSecurePath;
	int32_t status = 0;
	char longMsg[CMDQ_LONGSTRING_MAX];
	uint32_t msgOffset;
	int32_t msgMAXSize;

	/* find the first waiting task with OVERLAPPED engine flag with pTask */
	list_for_each(p, &gCmdqContext.taskWaitList) {
		pTempTask = list_entry(p, struct TaskStruct, listEntry);
		if (NULL != pTempTask && (engineFlag & (pTempTask->engineFlag))) {
			pFirstWaitingTask = pTempTask;
			break;
		}
	}

	do {
		if (NULL == pFirstWaitingTask) {
			/* no waiting task with overlape engine, go to dispath thread */
			break;
		}

		preferSecurePath = pFirstWaitingTask->secData.isSecure;

		if (preferSecurePath == isSecure) {
			/* same security path as first waiting task, go to start to thread dispatch */
			cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
			cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
					   "THREAD: is sec(%d, eng:0x%llx) as first waiting task",
					   isSecure, engineFlag);
			cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
					   "(0x%p, eng:0x%llx), start thread dispatch.\n",
					   pFirstWaitingTask, pFirstWaitingTask->engineFlag);
			if (msgOffset > 0) {
				/* print message */
				CMDQ_MSG("%s", longMsg);
			}
			break;
		}

		CMDQ_VERBOSE("THREAD: is not the first waiting task(0x%p), yield.\n",
			     pFirstWaitingTask);
		status = -EFAULT;
	} while (0);

	return status;
}

/**
 * check if engine conflict when thread dispatch
 * Parameter:
 *     engineFlag: [IN] engine flag
 *	   forceLog: [IN] print debug log
 *     isSecure: [IN] secure path
 *     *pThreadOut:
 *         [IN] prefer thread. please pass CMDQ_INVALID_THREAD if no prefere
 *         [OUT] dispatch thread result
 * Return:
 *     0 for success; else the error code is returned
 */

static bool cmdq_core_check_engine_conflict_unlocked(const uint64_t engineFlag,
						     bool forceLog,
						     const bool isSecure, int32_t *pThreadOut)
{
	EngineStruct *pEngine;
	ThreadStruct *pThread;
	uint32_t free;
	int32_t index;
	int32_t thread;
	uint64_t engine;
	bool isEngineConflict;
	char longMsg[CMDQ_LONGSTRING_MAX];
	uint32_t msgOffset;
	int32_t msgMAXSize;

	pEngine = gCmdqContext.engine;
	pThread = gCmdqContext.thread;
	isEngineConflict = false;

	engine = engineFlag;
	thread = (*pThreadOut);
	free = (CMDQ_INVALID_THREAD == thread) ? 0xFFFFFFFF : 0xFFFFFFFF & (~(0x1 << thread));

	/* check if engine conflict */
	for (index = 0; ((index < CMDQ_MAX_ENGINE_COUNT) && (engine != 0)); index++) {
		if (engine & (0x1LL << index)) {
			if (CMDQ_INVALID_THREAD == pEngine[index].currOwner) {
				continue;
			} else if (CMDQ_INVALID_THREAD == thread) {
				thread = pEngine[index].currOwner;
				free &= ~(0x1 << thread);
			} else if (thread != pEngine[index].currOwner) {
				/* Partial HW occupied by different threads, */
				/* we need to wait. */
				if (forceLog) {
					cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
					cmdqCoreLongString(true, longMsg, &msgOffset, &msgMAXSize,
							   "THREAD: try locate on thread %d but engine %d",
							   thread, index);
					cmdqCoreLongString(true, longMsg, &msgOffset, &msgMAXSize,
							   " also occupied by thread %d, secure:%d\n",
							   pEngine[index].currOwner, isSecure);
					if (msgOffset > 0) {
						/* print message */
						CMDQ_LOG("%s", longMsg);
					}
				} else {
					cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
					cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
							   "THREAD: try locate on thread %d but engine %d",
							   thread, index);
					cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
							   " also occupied by thread %d, secure:%d\n",
							   pEngine[index].currOwner, isSecure);
					if (msgOffset > 0) {
						/* print message */
						CMDQ_VERBOSE("%s", longMsg);
					}
				}

				isEngineConflict = true;	/* engine conflict! */
				thread = CMDQ_INVALID_THREAD;
				break;
			}

			engine &= ~(0x1LL << index);
		}
	}

	(*pThreadOut) = thread;
	return isEngineConflict;
}

static int32_t cmdq_core_find_a_free_HW_thread(uint64_t engineFlag,
					       CMDQ_HW_THREAD_PRIORITY_ENUM thread_prio,
					       CMDQ_SCENARIO_ENUM scenario, bool forceLog,
					       const bool isSecure)
{
	ThreadStruct *pThread;
	unsigned long flagsExecLock;
	int32_t index;
	int32_t thread;
	bool isEngineConflict;

	int32_t insertCookie;

	pThread = gCmdqContext.thread;

	do {
		CMDQ_VERBOSE
		    ("THREAD: find a free thread, engine: 0x%llx, scenario: %d, secure:%d\n",
		     engineFlag, scenario, isSecure);

		/* start to dispatch? */
		/* note we should not favor secure or normal path, */
		/* traverse waiting list to decide that we should dispatch thread to secure or normal path */
		if (0 > cmdq_core_can_start_to_acquire_HW_thread_unlocked(engineFlag, isSecure)) {
			thread = CMDQ_INVALID_THREAD;
			break;
		}

		/* it's okey to dispatch thread, */
		/* use scenario and pTask->secure to get default thread */
		thread = cmdq_get_func()->getThreadID(scenario, isSecure);

		/* check if engine conflict happened except DISP scenario */
		isEngineConflict = false;
		if (false == cmdq_get_func()->isDispScenario(scenario)) {
			isEngineConflict = cmdq_core_check_engine_conflict_unlocked(engineFlag,
										    forceLog,
										    isSecure,
										    &thread);
		}
		CMDQ_VERBOSE("THREAD: isEngineConflict:%d, thread:%d\n", isEngineConflict, thread);

#if 1				/* TODO: secure path proting */
		/* because all thread are pre-dispatched, there 2 outcome of engine conflict check:
		 * 1. pre-dispatched secure thread, and no conflict with normal path
		 * 2. pre-dispatched secure thread, but conflict with normal/anothor secure path
		 *
		 * no need to check get normal thread in secure path
		 */

		/* ensure not dispatch secure thread to normal task */
		if ((false == isSecure) && (true == cmdq_get_func()->isSecureThread(thread))) {
			thread = CMDQ_INVALID_THREAD;
			isEngineConflict = true;
			break;
		}
#endif
		/* no enfine conflict with running thread, AND used engines have no owner */
		/* try to find a free thread */
		if ((false == isEngineConflict) && (CMDQ_INVALID_THREAD == thread)) {

			/* thread 0 - CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT are preserved for DISPSYS */
			const bool isDisplayThread = CMDQ_THR_PRIO_DISPLAY_TRIGGER < thread_prio;
			int startIndex = isDisplayThread ? 0 : CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT;
			int endIndex = isDisplayThread ?
			    CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT : CMDQ_MAX_THREAD_COUNT;

			for (index = startIndex; index < endIndex; ++index) {

				spin_lock_irqsave(&gCmdqExecLock, flagsExecLock);

				if ((0 == pThread[index].engineFlag) &&
				    (0 == pThread[index].taskCount) &&
				    (1 == pThread[index].allowDispatching)) {

					CMDQ_VERBOSE
					    ("THREAD: dispatch to thread %d, taskCount:%d, allowDispatching:%d\n",
					     index, pThread[index].taskCount,
					     pThread[index].allowDispatching);

					thread = index;
					pThread[index].allowDispatching = 0;
					spin_unlock_irqrestore(&gCmdqExecLock, flagsExecLock);
					break;
				}

				spin_unlock_irqrestore(&gCmdqExecLock, flagsExecLock);
			}
		}

		/* no thread available now, wait for it */
		if (CMDQ_INVALID_THREAD == thread)
			break;

		/* Make sure the found thread has enough space for the task; */
		/* ThreadStruct->pCurTask has size limitation. */
		if (cmdq_core_max_task_in_thread(thread) <= pThread[thread].taskCount) {
			if (forceLog) {
				CMDQ_LOG("THREAD: thread %d task count = %d full\n",
					 thread, pThread[thread].taskCount);
			} else {
				CMDQ_VERBOSE("THREAD: thread %d task count = %d full\n",
					     thread, pThread[thread].taskCount);
			}

			thread = CMDQ_INVALID_THREAD;
		}

		insertCookie = pThread[thread].nextCookie % cmdq_core_max_task_in_thread(thread);
		if (NULL != pThread[thread].pCurTask[insertCookie]) {
			if (forceLog) {
				CMDQ_LOG("THREAD: thread %d nextCookie = %d already has task\n",
					 thread, pThread[thread].nextCookie);
			} else {
				CMDQ_VERBOSE("THREAD: thread %d nextCookie = %d already has task\n",
					     thread, pThread[thread].nextCookie);
			}

			thread = CMDQ_INVALID_THREAD;
		}
	} while (0);

	return thread;
}

static int32_t cmdq_core_acquire_thread(uint64_t engineFlag,
					CMDQ_HW_THREAD_PRIORITY_ENUM thread_prio,
					CMDQ_SCENARIO_ENUM scenario, bool forceLog,
					const bool isSecure)
{
	unsigned long flags;
	int32_t thread;
	uint64_t engineMustEnableClock;

	CMDQ_PROF_START(current->pid, __func__);

	do {
		mutex_lock(&gCmdqClockMutex);
		spin_lock_irqsave(&gCmdqThreadLock, flags);

		thread =
		    cmdq_core_find_a_free_HW_thread(engineFlag, thread_prio, scenario, forceLog,
						    isSecure);

		if (CMDQ_INVALID_THREAD != thread) {
			/* get actual engine flag. Each bit represents a engine must enable clock. */
			engineMustEnableClock =
			    cmdq_core_get_actual_engine_flag_for_enable_clock(engineFlag, thread);
		}

		spin_unlock_irqrestore(&gCmdqThreadLock, flags);

		if (CMDQ_INVALID_THREAD != thread) {
			/* enable clock */
			cmdq_core_enable_clock(engineFlag, thread, engineMustEnableClock, scenario);
		}

		mutex_unlock(&gCmdqClockMutex);
	} while (0);

	CMDQ_PROF_END(current->pid, __func__);

	return thread;
}

static uint64_t cmdq_core_get_not_used_engine_flag_for_disable_clock(const uint64_t engineFlag)
{
	EngineStruct *pEngine;
	ThreadStruct *pThread;
	uint64_t enginesNotUsed;
	int32_t index;
	int32_t currOwnerThread = CMDQ_INVALID_THREAD;

	enginesNotUsed = 0LL;
	pEngine = gCmdqContext.engine;
	pThread = gCmdqContext.thread;

	for (index = 0; index < CMDQ_MAX_ENGINE_COUNT; index++) {
		if (engineFlag & (1LL << index)) {
			pEngine[index].userCount--;
			if (pEngine[index].userCount <= 0) {
				enginesNotUsed |= (1LL << index);
				currOwnerThread = pEngine[index].currOwner;
				/* remove engine flag in assigned pThread */
				pThread[currOwnerThread].engineFlag &= ~(1LL << index);
				pEngine[index].currOwner = CMDQ_INVALID_THREAD;
			}
		}
	}
	CMDQ_VERBOSE("%s, enginesNotUsed:0x%llx\n", __func__, enginesNotUsed);
	return enginesNotUsed;
}

static void cmdq_core_disable_clock(uint64_t engineFlag,
				    const uint64_t enginesNotUsed, CMDQ_SCENARIO_ENUM scenario)
{
	int32_t index;
	int32_t status;
	CmdqCBkStruct *pCallback;

	CMDQ_VERBOSE("-->CLOCK: Disable hardware clock 0x%llx begin, enginesNotUsed 0x%llx\n",
		     engineFlag, enginesNotUsed);

	pCallback = gCmdqGroupCallback;

	/* ISP special check: Always call ISP on/off if this task */
	/* involves ISP. Ignore the ISP HW flags ref count. */
	if (cmdq_core_is_group_flag(CMDQ_GROUP_ISP, engineFlag)) {
		CMDQ_VERBOSE("CLOCK: disable group %d clockOff\n", CMDQ_GROUP_ISP);
		if (NULL == pCallback[CMDQ_GROUP_ISP].clockOff) {
			CMDQ_ERR("CLOCK: disable group %d clockOff func NULL\n", CMDQ_GROUP_ISP);
		} else {
			status =
			    pCallback[CMDQ_GROUP_ISP].clockOff(gCmdqEngineGroupBits[CMDQ_GROUP_ISP]
							       & engineFlag);

#if 1
			--gCmdqISPClockCounter;
			if (gCmdqISPClockCounter != 0) {
				/* ISP clock off */
				CMDQ_VERBOSE("CLOCK: ISP clockOff cnt=%d\n", gCmdqISPClockCounter);
			}
#endif

			if (status < 0) {
				CMDQ_ERR("CLOCK: disable group %d clockOff failed\n",
					 CMDQ_GROUP_ISP);
			}
		}
	}

	/* Turn off unused engines */
	for (index = 0; index < CMDQ_MAX_GROUP_COUNT; ++index) {
		/* note that DISPSYS controls their own clock on/off */
		if (CMDQ_GROUP_DISP == index)
			continue;

		/* note that ISP is per-task on/off, not per HW flag */
		if (CMDQ_GROUP_ISP == index)
			continue;

		if (cmdq_core_is_group_flag((CMDQ_GROUP_ENUM) index, enginesNotUsed)) {
			CMDQ_MSG("CLOCK: Disable engine group %d flag=0x%llx clockOff\n", index,
				 enginesNotUsed);
			if (NULL == pCallback[index].clockOff) {
				CMDQ_LOG
				    ("[WARNING]CLOCK: Disable engine group %d clockOff func NULL\n",
				     index);
				continue;
			}
			status =
			    pCallback[index].clockOff(gCmdqEngineGroupBits[index] & enginesNotUsed);
			if (status < 0) {
				/* Error status print */
				CMDQ_ERR("CLOCK: Disable engine group %d clock failed\n", index);
			}
		}
	}

	/* disable fundamental clocks if needed */
	cmdq_core_enable_common_clock_locked(false, engineFlag, scenario);

	CMDQ_MSG("<--CLOCK: Disable hardware clock 0x%llx end\n", engineFlag);
}

void cmdq_core_add_consume_task(void)
{
	if (!work_pending(&gCmdqContext.taskConsumeWaitQueueItem)) {
		CMDQ_PROF_MMP(cmdq_mmp_get_event()->consume_add, MMProfileFlagPulse, 0, 0);
		queue_work(gCmdqContext.taskConsumeWQ, &gCmdqContext.taskConsumeWaitQueueItem);
	}
}

static void cmdq_core_release_thread(TaskStruct *pTask)
{
	unsigned long flags;
	const int32_t thread = pTask->thread;
	const uint64_t engineFlag = pTask->engineFlag;
	uint64_t engineNotUsed = 0LL;

	if (thread == CMDQ_INVALID_THREAD)
		return;

	mutex_lock(&gCmdqClockMutex);
	spin_lock_irqsave(&gCmdqThreadLock, flags);

	/* get not used engines for disable clock */
	engineNotUsed = cmdq_core_get_not_used_engine_flag_for_disable_clock(engineFlag);
	pTask->thread = CMDQ_INVALID_THREAD;

	spin_unlock_irqrestore(&gCmdqThreadLock, flags);

	/* clock off */
	cmdq_core_disable_clock(engineFlag, engineNotUsed, pTask->scenario);
	/* Delay release resource  */
	cmdq_core_delay_check_unlock(engineFlag, engineNotUsed);

	mutex_unlock(&gCmdqClockMutex);
}

static void cmdq_core_reset_hw_engine(int32_t engineFlag)
{
	EngineStruct *pEngine;
	uint32_t engines;
	int32_t index;
	int32_t status;
	CmdqCBkStruct *pCallback;

	CMDQ_MSG("Reset hardware engine begin\n");

	pEngine = gCmdqContext.engine;

	engines = 0;
	for (index = 0; index < CMDQ_MAX_ENGINE_COUNT; index++) {
		if (engineFlag & (1LL << index))
			engines |= (1LL << index);
	}

	pCallback = gCmdqGroupCallback;

	for (index = 0; index < CMDQ_MAX_GROUP_COUNT; ++index) {
		if (cmdq_core_is_group_flag((CMDQ_GROUP_ENUM) index, engines)) {
			CMDQ_MSG("Reset engine group %d clock\n", index);
			if (NULL == pCallback[index].resetEng) {
				CMDQ_ERR("Reset engine group %d clock func NULL\n", index);
				continue;
			}
			status =
			    pCallback[index].resetEng(gCmdqEngineGroupBits[index] & engineFlag);
			if (status < 0) {
				/* Error status print */
				CMDQ_ERR("Reset engine group %d clock failed\n", index);
			}
		}
	}

	CMDQ_MSG("Reset hardware engine end\n");
}

int32_t cmdq_core_suspend_HW_thread(int32_t thread, uint32_t lineNum)
{
	int32_t loop = 0;
	uint32_t enabled = 0;

	if (CMDQ_INVALID_THREAD == thread) {
		CMDQ_ERR("suspend invalid thread\n");
		return -EFAULT;
	}

	CMDQ_PROF_MMP(cmdq_mmp_get_event()->thread_suspend, MMProfileFlagPulse, thread, lineNum);
	/* write suspend bit */
	CMDQ_REG_SET32(CMDQ_THR_SUSPEND_TASK(thread), 0x01);

	/* check if the thread is already disabled. */
	/* if already disabled, treat as suspend successful but print error log */
	enabled = CMDQ_REG_GET32(CMDQ_THR_ENABLE_TASK(thread));
	if (0 == (0x01 & enabled)) {
		CMDQ_LOG("[WARNING] thread %d suspend not effective, enable=%d\n", thread, enabled);
		return 0;
	}

	loop = 0;
	while (0x0 == (CMDQ_REG_GET32(CMDQ_THR_CURR_STATUS(thread)) & 0x2)) {
		if (loop > CMDQ_MAX_LOOP_COUNT) {
			CMDQ_AEE("CMDQ", "Suspend HW thread %d failed\n", thread);
			return -EFAULT;
		}
		loop++;
	}

#ifdef CONFIG_MTK_FPGA
	CMDQ_MSG("EXEC: Suspend HW thread(%d)\n", thread);
#endif

	return 0;
}

static inline void cmdq_core_resume_HW_thread(int32_t thread)
{
#ifdef CONFIG_MTK_FPGA
	CMDQ_MSG("EXEC: Resume HW thread(%d)\n", thread);
#endif
	/* make sure instructions are really in DRAM */
	smp_mb();
	CMDQ_PROF_MMP(cmdq_mmp_get_event()->thread_resume, MMProfileFlagPulse, thread, __LINE__);
	CMDQ_REG_SET32(CMDQ_THR_SUSPEND_TASK(thread), 0x00);
}

static inline int32_t cmdq_core_reset_HW_thread(int32_t thread)
{
	int32_t loop = 0;

	CMDQ_MSG("Reset HW thread(%d)\n", thread);

	CMDQ_REG_SET32(CMDQ_THR_WARM_RESET(thread), 0x01);
	while (0x1 == (CMDQ_REG_GET32(CMDQ_THR_WARM_RESET(thread)))) {
		if (loop > CMDQ_MAX_LOOP_COUNT) {
			CMDQ_AEE("CMDQ", "Reset HW thread %d failed\n", thread);
			return -EFAULT;
		}
		loop++;
	}

	CMDQ_REG_SET32(CMDQ_THR_SLOT_CYCLES, 0x3200);
	return 0;
}

static inline int32_t cmdq_core_disable_HW_thread(int32_t thread)
{
	cmdq_core_reset_HW_thread(thread);

	/* Disable thread */
	CMDQ_MSG("Disable HW thread(%d)\n", thread);
	CMDQ_REG_SET32(CMDQ_THR_ENABLE_TASK(thread), 0x00);
	return 0;
}

uint32_t cmdq_core_subsys_to_reg_addr(uint32_t argA)
{
	const uint32_t subsysBit = cmdq_get_func()->getSubsysLSBArgA();
	const int32_t subsys_id = (argA & CMDQ_ARG_A_SUBSYS_MASK) >> subsysBit;
	uint32_t offset = 0;
	uint32_t base_addr = 0;
	uint32_t i;

	for (i = 0; i < CMDQ_SUBSYS_MAX_COUNT; i++) {
		if (gCmdqDtsData.subsys[i].subsysID == subsys_id) {
			base_addr = gCmdqDtsData.subsys[i].msb;
			offset = (argA & ~gCmdqDtsData.subsys[i].mask);
			break;
		}
	}

	return base_addr | offset;
}

const char *cmdq_core_parse_subsys_from_reg_addr(uint32_t reg_addr)
{
	uint32_t addr_base_shifted;
	const char *module = "CMDQ";
	uint32_t i;

	for (i = 0; i < CMDQ_SUBSYS_MAX_COUNT; i++) {
		if (-1 == gCmdqDtsData.subsys[i].subsysID)
			continue;

		addr_base_shifted = (reg_addr & gCmdqDtsData.subsys[i].mask);
		if (gCmdqDtsData.subsys[i].msb == addr_base_shifted) {
			module = gCmdqDtsData.subsys[i].grpName;
			break;
		}
	}

	return module;
}

int32_t cmdq_core_subsys_from_phys_addr(uint32_t physAddr)
{
	int32_t msb;
	int32_t subsysID = -1;
	uint32_t i;

	for (i = 0; i < CMDQ_SUBSYS_MAX_COUNT; i++) {
		if (-1 == gCmdqDtsData.subsys[i].subsysID)
			continue;

		msb = (physAddr & gCmdqDtsData.subsys[i].mask);
		if (msb == gCmdqDtsData.subsys[i].msb) {
			subsysID = gCmdqDtsData.subsys[i].subsysID;
			break;
		}
	}

	if (-1 == subsysID) {
		/* printf error message */
		CMDQ_ERR("unrecognized subsys, physAddr:0x%08x\n", physAddr);
	}
	return subsysID;
}

static const char *cmdq_core_parse_op(uint32_t opCode)
{
	switch (opCode) {
	case CMDQ_CODE_POLL:
		return "POLL";
	case CMDQ_CODE_WRITE:
		return "WRIT";
	case CMDQ_CODE_WFE:
		return "SYNC";
	case CMDQ_CODE_READ:
		return "READ";
	case CMDQ_CODE_MOVE:
		return "MASK";
	case CMDQ_CODE_JUMP:
		return "JUMP";
	case CMDQ_CODE_EOC:
		return "MARK";
	}
	return NULL;
}

static void cmdq_core_parse_error(const TaskStruct *pTask, uint32_t thread,
				  const char **moduleName, int32_t *flag, uint32_t *instA,
				  uint32_t *instB)
{
	uint32_t op, argA, argB;
	int32_t eventENUM;
	uint32_t insts[4] = { 0 };
	uint32_t addr = 0;
	const char *module = NULL;
	int32_t irqFlag = pTask->irqFlag;
	int isSMIHang = 0;

	do {
		/* confirm if SMI is hang */
		isSMIHang = cmdq_get_func()->dumpSMI(0);
		if (isSMIHang) {
			module = "SMI";
			break;
		}

		/* other cases, use instruction to judge */
		/* because scenario / HW flag are not sufficient */
		/* e.g. ISP pass 2 involves both MDP and ISP */
		/* so we need to check which instruction timeout-ed. */
		if (cmdq_core_get_pc(pTask, thread, insts)) {
			op = (insts[3] & 0xFF000000) >> 24;
			argA = insts[3] & (~0xFF000000);
			argB = insts[2];

			/* quick exam by hwflag first */
			module = cmdq_get_func()->parseErrorModule(pTask);
			if (NULL != module)
				break;

			switch (op) {
			case CMDQ_CODE_POLL:
			case CMDQ_CODE_WRITE:
				addr = cmdq_core_subsys_to_reg_addr(argA);
				module = cmdq_get_func()->parseModule(addr);
				break;
			case CMDQ_CODE_WFE:
				/* argA is the event ID */
				eventENUM = cmdq_core_reverse_event_ENUM(argA);
				module = cmdq_get_func()->moduleFromEvent(eventENUM);
				break;
			case CMDQ_CODE_READ:
			case CMDQ_CODE_MOVE:
			case CMDQ_CODE_JUMP:
			case CMDQ_CODE_EOC:
			default:
				module = "CMDQ";
				break;
			}
			break;
		}

		module = "CMDQ";
		break;
	} while (0);


	/* fill output parameter */
	*moduleName = module;
	*flag = irqFlag;
	*instA = insts[3];
	*instB = insts[2];

}

static uint32_t *cmdq_core_dump_pc(const TaskStruct *pTask, int thread, const char *tag)
{
	uint32_t *pcVA = NULL;
	uint32_t insts[4] = { 0 };
	char parsedInstruction[128] = { 0 };

	pcVA = cmdq_core_get_pc(pTask, thread, insts);
	if (pcVA) {
		const uint32_t op = (insts[3] & 0xFF000000) >> 24;

		cmdq_core_parse_instruction(pcVA, parsedInstruction, sizeof(parsedInstruction));
		CMDQ_LOG("[%s]Thread %d PC(VA): 0x%p, 0x%08x:0x%08x => %s",
			 tag, thread, pcVA, insts[2], insts[3], parsedInstruction);

		/* for WFE, we specifically dump the event value */
		if (op == CMDQ_CODE_WFE) {
			uint32_t regValue = 0;
			const uint32_t eventID = 0x3FF & insts[3];

			CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_ID, eventID);
			regValue = CMDQ_REG_GET32(CMDQ_SYNC_TOKEN_VAL);
			CMDQ_LOG("[%s]CMDQ_SYNC_TOKEN_VAL of %s is %d\n", tag,
				 cmdq_core_get_event_name(eventID), regValue);
		}
	} else {
		if (true == pTask->secData.isSecure) {
			CMDQ_LOG("[%s]Thread %d PC(VA): HIDDEN INFO since is it's secure thread\n",
				 tag, thread);
		} else {
			CMDQ_LOG("[%s]Thread %d PC(VA): Not available\n", tag, thread);
		}
	}

	return pcVA;
}

static void cmdq_core_dump_status(const char *tag)
{
	int32_t coreExecThread = CMDQ_INVALID_THREAD;
	uint32_t value[6] = { 0 };

	value[0] = CMDQ_REG_GET32(CMDQ_CURR_LOADED_THR);
	value[1] = CMDQ_REG_GET32(CMDQ_THR_EXEC_CYCLES);
	value[2] = CMDQ_REG_GET32(CMDQ_THR_TIMEOUT_TIMER);
	value[3] = CMDQ_REG_GET32(CMDQ_BUS_CONTROL_TYPE);

	/* this returns (1 + index of least bit set) or 0 if input is 0. */
	coreExecThread = __builtin_ffs(value[0]) - 1;
	CMDQ_LOG("[%s]IRQ flag:0x%08x, Execing:%d, Exec Thread:%d, CMDQ_CURR_LOADED_THR: 0x%08x\n",
		 tag,
		 CMDQ_REG_GET32(CMDQ_CURR_IRQ_STATUS),
		 (0x80000000 & value[0]) ? 1 : 0, coreExecThread, value[0]);
	CMDQ_LOG("[%s]CMDQ_THR_EXEC_CYCLES:0x%08x, CMDQ_THR_TIMER:0x%08x, CMDQ_BUS_CTRL:0x%08x\n",
		 tag, value[1], value[2], value[3]);
	CMDQ_LOG("[%s]CMDQ_DEBUG_1: 0x%08x\n", tag, CMDQ_REG_GET32((GCE_BASE_VA + 0xF0)));
	CMDQ_LOG("[%s]CMDQ_DEBUG_2: 0x%08x\n", tag, CMDQ_REG_GET32((GCE_BASE_VA + 0xF4)));
	CMDQ_LOG("[%s]CMDQ_DEBUG_3: 0x%08x\n", tag, CMDQ_REG_GET32((GCE_BASE_VA + 0xF8)));
	CMDQ_LOG("[%s]CMDQ_DEBUG_4: 0x%08x\n", tag, CMDQ_REG_GET32((GCE_BASE_VA + 0xFC)));
}

void cmdq_core_dump_disp_trigger_loop(const char *tag)
{
	/* we assume the first non-high-priority thread is trigger loop thread. */
	/* since it will start very early */
	if (gCmdqContext.thread[CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT].taskCount
	    && gCmdqContext.thread[CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT].pCurTask[1]
	    && gCmdqContext.thread[CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT].loopCallback) {

		uint32_t regValue = 0;
		TaskStruct *pTask =
		    gCmdqContext.thread[CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT].pCurTask[1];

		cmdq_core_dump_pc(pTask, CMDQ_MAX_HIGH_PRIORITY_THREAD_COUNT, tag);

		regValue = cmdqCoreGetEvent(CMDQ_EVENT_DISP_RDMA0_EOF);
		CMDQ_LOG("[%s]CMDQ_SYNC_TOKEN_VAL of %s is %d\n",
			 tag, cmdq_core_get_event_name_ENUM(CMDQ_EVENT_DISP_RDMA0_EOF), regValue);
	}
}

static void cmdq_core_dump_thread_pc(const int32_t thread)
{
	int32_t i;
	ThreadStruct *pThread;
	TaskStruct *pTask;
	uint32_t *pcVA;
	uint32_t insts[4] = { 0 };
	char parsedInstruction[128] = { 0 };

	if (CMDQ_INVALID_THREAD == thread)
		return;

	pThread = &(gCmdqContext.thread[thread]);
	pcVA = NULL;

	for (i = 0; i < cmdq_core_max_task_in_thread(thread); i++) {
		pTask = pThread->pCurTask[i];

		if (NULL != pTask) {
			pcVA = cmdq_core_get_pc(pTask, thread, insts);
			if (pcVA) {
				const uint32_t op = (insts[3] & 0xFF000000) >> 24;

				cmdq_core_parse_instruction(pcVA, parsedInstruction,
							    sizeof(parsedInstruction));
				CMDQ_LOG
				    ("[INFO]task:%p(index:%d), Thread %d PC(VA): 0x%p, 0x%08x:0x%08x => %s",
				     pTask, i, thread, pcVA, insts[2], insts[3], parsedInstruction);

				/* for wait event case, dump token value */
				/* for WFE, we specifically dump the event value */
				if (op == CMDQ_CODE_WFE) {
					uint32_t regValue = 0;
					const uint32_t eventID = 0x3FF & insts[3];

					CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_ID, eventID);
					regValue = CMDQ_REG_GET32(CMDQ_SYNC_TOKEN_VAL);
					CMDQ_LOG("[INFO]CMDQ_SYNC_TOKEN_VAL of %s is %d\n",
						 cmdq_core_get_event_name(eventID), regValue);
				}
				break;
			}
		}
	}
}

static void cmdq_core_dump_task_with_engine_flag(uint64_t engineFlag)
{
	struct TaskStruct *pDumpTask = NULL;
	struct list_head *p = NULL;

	CMDQ_ERR
	    ("=============== [CMDQ] All active tasks sharing same engine flag 0x%08llx===============\n",
	     engineFlag);

	list_for_each(p, &gCmdqContext.taskActiveList) {
		pDumpTask = list_entry(p, struct TaskStruct, listEntry);
		if (NULL != pDumpTask && (engineFlag & pDumpTask->engineFlag)) {
			CMDQ_ERR("Thr %d, Task: 0x%p, VABase: 0x%p, MVABase 0x%pa, Size: %d\n",
					   (pDumpTask->thread), (pDumpTask),
					   (pDumpTask->pVABase), &(pDumpTask->MVABase),
					   pDumpTask->commandSize);
			CMDQ_ERR("  cont'd: Flag: 0x%08llx, Last Inst 0x%08x:0x%08x, 0x%08x:0x%08x\n",
					   pDumpTask->engineFlag, pDumpTask->pCMDEnd[-3],
					   pDumpTask->pCMDEnd[-2], pDumpTask->pCMDEnd[-1],
					   pDumpTask->pCMDEnd[0]);
		}
	}
}

static void cmdq_core_dump_task_in_thread(const int32_t thread,
					  const bool fullTatskDump, const bool dumpCookie,
					  const bool dumpCmd)
{
	ThreadStruct *pThread;
	TaskStruct *pDumpTask;
	int32_t index;
	uint32_t value[4] = { 0 };
	uint32_t cookie;

	if (CMDQ_INVALID_THREAD == thread)
		return;

	pThread = &(gCmdqContext.thread[thread]);
	pDumpTask = NULL;

	CMDQ_ERR("=============== [CMDQ] All Task in Error Thread %d ===============\n", thread);
	cookie = cmdq_core_thread_exec_counter(thread);
	if (dumpCookie) {
		CMDQ_ERR
		    ("Curr Cookie: %d, Wait Cookie: %d, Next Cookie: %d, Task Count %d, engineFlag: 0x%llx\n",
		     cookie, pThread->waitCookie, pThread->nextCookie, pThread->taskCount,
		     pThread->engineFlag);
	}

	for (index = 0; index < cmdq_core_max_task_in_thread(thread); index++) {
		pDumpTask = pThread->pCurTask[index];
		if (NULL == pDumpTask)
			continue;

		/* full task dump */
		if (fullTatskDump) {
			CMDQ_ERR("Slot %d, Task: 0x%p\n", index, pDumpTask);
			cmdq_core_dump_task(pDumpTask);

			if (true == dumpCmd) {
				print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 16, 4,
					       pDumpTask->pVABase, (pDumpTask->commandSize), true);
			}
			continue;
		}

		/* otherwise, simple dump task info */
		if (NULL == pDumpTask->pVABase) {
			value[0] = 0xBCBCBCBC;
			value[1] = 0xBCBCBCBC;
			value[2] = 0xBCBCBCBC;
			value[3] = 0xBCBCBCBC;
		} else {
			value[0] = pDumpTask->pCMDEnd[-3];
			value[1] = pDumpTask->pCMDEnd[-2];
			value[2] = pDumpTask->pCMDEnd[-1];
			value[3] = pDumpTask->pCMDEnd[0];
		}
		CMDQ_ERR("Slot %d, Task: 0x%p, VABase: 0x%p, MVABase 0x%pa, Size: %d\n",
				   index, (pDumpTask), (pDumpTask->pVABase),
				   &(pDumpTask->MVABase), pDumpTask->commandSize);
		CMDQ_ERR("  cont'd: Last Inst 0x%08x:0x%08x, 0x%08x:0x%08x, priority:%d\n",
				   value[0], value[1], value[2], value[3], pDumpTask->priority);

		if (true == dumpCmd) {
			print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 16, 4,
				       pDumpTask->pVABase, (pDumpTask->commandSize), true);
		}

	}
}

void cmdq_core_dump_secure_metadata(cmdqSecDataStruct *pSecData)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	uint32_t i = 0;
	cmdqSecAddrMetadataStruct *pAddr = NULL;

	if (NULL == pSecData)
		return;

	pAddr = (cmdqSecAddrMetadataStruct *) (CMDQ_U32_PTR(pSecData->addrMetadatas));

	CMDQ_LOG("========= pSecData: %p dump =========\n", pSecData);
	CMDQ_LOG("count:%d(%d), enginesNeedDAPC:0x%llx, enginesPortSecurity:0x%llx\n",
		 pSecData->addrMetadataCount, pSecData->addrMetadataMaxCount,
		 pSecData->enginesNeedDAPC, pSecData->enginesNeedPortSecurity);

	if (NULL == pAddr)
		return;

	for (i = 0; i < pSecData->addrMetadataCount; i++) {
		CMDQ_LOG("idx:%d, type:%d, baseHandle:%x, offset:%d, size:%d, port:%d\n",
			 i, pAddr[i].type, pAddr[i].baseHandle, pAddr[i].offset, pAddr[i].size,
			 pAddr[i].port);
	}
#endif
}


int32_t cmdq_core_interpret_instruction(char *textBuf, int bufLen,
					const uint32_t op, const uint32_t argA, const uint32_t argB)
{
	int reqLen = 0;

	switch (op) {
	case CMDQ_CODE_MOVE:
		if (1 & (argA >> 23)) {
			reqLen =
			    snprintf(textBuf, bufLen, "MOVE: 0x%08x to Reg%d\n", argB,
				     (argA >> 16) & 0x1f);
		} else {
			reqLen = snprintf(textBuf, bufLen, "Set MASK: 0x%08x\n", argB);
		}
		break;
	case CMDQ_CODE_READ:
	case CMDQ_CODE_WRITE:
	case CMDQ_CODE_POLL:
		reqLen = snprintf(textBuf, bufLen, "%s: ", cmdq_core_parse_op(op));
		bufLen -= reqLen;
		textBuf += reqLen;

		/* data (value) */
		if (argA & (1 << 22)) {
			reqLen = snprintf(textBuf, bufLen, "Reg%d, ", argB);
			bufLen -= reqLen;
			textBuf += reqLen;
		} else {
			reqLen = snprintf(textBuf, bufLen, "0x%08x, ", argB);
			bufLen -= reqLen;
			textBuf += reqLen;
		}

		/* address */
		if (argA & (1 << 23)) {
			reqLen = snprintf(textBuf, bufLen, "Reg%d\n", (argA >> 16) & 0x1F);
			bufLen -= reqLen;
			textBuf += reqLen;
		} else {
			const uint32_t addr = cmdq_core_subsys_to_reg_addr(argA);
			const uint32_t addrMask = 0xFFFFFFFE;

			reqLen = snprintf(textBuf, bufLen, "addr=0x%08x [%s], use_mask=%d\n",
					  (addr & addrMask),
					  cmdq_get_func()->parseModule(addr), (addr & 0x1));
			bufLen -= reqLen;
			textBuf += reqLen;
		}
		break;
	case CMDQ_CODE_JUMP:
		if (argA) {
			if (argA & (1 << 22)) {
				/* jump by register */
				reqLen = snprintf(textBuf, bufLen, "JUMP(register): Reg%d\n", argB);
			} else {
				/* absolute */
				reqLen =
				    snprintf(textBuf, bufLen, "JUMP(absolute): 0x%08x\n", argB);
			}
		} else {
			/* relative */
			if ((int32_t) argB >= 0) {
				reqLen = snprintf(textBuf, bufLen,
						  "JUMP(relative): +%d\n", (int32_t) argB);
			} else {
				reqLen = snprintf(textBuf, bufLen,
						  "JUMP(relative): %d\n", (int32_t) argB);
			}
		}
		break;
	case CMDQ_CODE_WFE:
		if (0x80008001 == argB) {
			reqLen =
			    snprintf(textBuf, bufLen, "Wait And Clear Event: %s\n",
				     cmdq_core_get_event_name(argA));
		} else if (0x80000000 == argB) {
			reqLen =
			    snprintf(textBuf, bufLen, "Clear Event: %s\n",
				     cmdq_core_get_event_name(argA));
		} else if (0x80010000 == argB) {
			reqLen =
			    snprintf(textBuf, bufLen, "Set Event: %s\n",
				     cmdq_core_get_event_name(argA));
		} else if (0x00008001 == argB) {
			reqLen =
			    snprintf(textBuf, bufLen, "Wait No Clear Event: %s\n",
				     cmdq_core_get_event_name(argA));
		} else {
			reqLen = snprintf(textBuf, bufLen,
					  "SYNC: %s, upd=%d, op=%d, val=%d, wait=%d, wop=%d, val=%d\n",
					  cmdq_core_get_event_name(argA),
					  (argB >> 31) & 0x1,
					  (argB >> 28) & 0x7,
					  (argB >> 16) & 0xFFF,
					  (argB >> 15) & 0x1,
					  (argB >> 12) & 0x7, (argB >> 0) & 0xFFF);
		}
		break;
	case CMDQ_CODE_EOC:
		if (argA == 0 && argB == 0x00000001) {
			reqLen = snprintf(textBuf, bufLen, "EOC\n");
		} else {
			if (cmdq_core_support_sync_non_suspendable()) {
				reqLen = snprintf(textBuf, bufLen,
						  "MARKER: sync_no_suspnd=%d",
						  (argA & (1 << 20)) > 0);
			} else {
				reqLen = snprintf(textBuf, bufLen, "MARKER:");
			}
			bufLen -= reqLen;
			textBuf += reqLen;
			if (argB == 0x00100000) {
				reqLen = snprintf(textBuf, bufLen, " Disable");
			} else if (argB == 0x00130000) {
				reqLen = snprintf(textBuf, bufLen, " Enable");
			} else {
				reqLen = snprintf(textBuf, bufLen,
						  "no_suspnd=%d, no_inc=%d, m=%d, m_en=%d, prefetch=%d, irq=%d\n",
						  (argA & (1 << 21)) > 0,
						  (argA & (1 << 16)) > 0,
						  (argB & (1 << 20)) > 0,
						  (argB & (1 << 17)) > 0,
						  (argB & (1 << 16)) > 0, (argB & (1 << 0)) > 0);
			}
		}
		break;
	default:
		reqLen = snprintf(textBuf, bufLen, "UNDEFINED\n");
		break;
	}

	return reqLen;
}

int32_t cmdq_core_parse_instruction(const uint32_t *pCmd, char *textBuf, int bufLen)
{
	int reqLen = 0;

	const uint32_t op = (pCmd[1] & 0xFF000000) >> 24;
	const uint32_t argA = pCmd[1] & (~0xFF000000);
	const uint32_t argB = pCmd[0];

	reqLen = cmdq_core_interpret_instruction(textBuf, bufLen, op, argA, argB);

	return reqLen;
}

void cmdq_core_dump_error_instruction(const uint32_t *pcVA, uint32_t *insts, int thread,
				      uint32_t lineNum)
{
	char parsedInstruction[128] = { 0 };
	const uint32_t op = (insts[3] & 0xFF000000) >> 24;

	if (pcVA == NULL) {
		CMDQ_ERR("Dump error instruction with null va, line: %u\n", lineNum);
		return;
	}
	cmdq_core_parse_instruction(pcVA, parsedInstruction, sizeof(parsedInstruction));
	CMDQ_ERR("Thread %d error instruction: 0x%p, 0x%08x:0x%08x => %s",
		 thread, pcVA, insts[2], insts[3], parsedInstruction);

	/* for WFE, we specifically dump the event value */
	if (op == CMDQ_CODE_WFE) {
		uint32_t regValue = 0;
		const uint32_t eventID = 0x3FF & insts[3];

		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_ID, eventID);
		regValue = CMDQ_REG_GET32(CMDQ_SYNC_TOKEN_VAL);
		CMDQ_ERR("CMDQ_SYNC_TOKEN_VAL of %s is %d\n",
			 cmdq_core_get_event_name(eventID), regValue);
	}
}

static void cmdq_core_dump_summary(const TaskStruct *pTask, int thread,
				   const TaskStruct **pOutNGTask)
{
	uint32_t *pcVA = NULL;
	uint32_t insts[4] = { 0 };
	ThreadStruct *pThread;
	const TaskStruct *pNGTask = NULL;
	const char *module = NULL;
	int32_t index;
	uint32_t instA = 0, instB = 0;
	int32_t irqFlag = 0;

	if ((NULL == pTask) || (NULL == pTask->pVABase) || (CMDQ_INVALID_THREAD == thread)) {
		CMDQ_ERR
		    ("dump summary failed since invalid param, pTask %p, pTask->pVABase:%p, thread:%d\n",
		     pTask, pTask->pVABase, thread);
		return;
	}

	if (true == pTask->secData.isSecure) {
		CMDQ_ERR("Summary dump does not support secure now.\n");
		pNGTask = pTask;
		return;
	}

	/* Find correct task */
	pThread = &(gCmdqContext.thread[thread]);
	pcVA = cmdq_core_get_pc(pTask, thread, insts);
	if (NULL == pcVA) {
		/* Find all task to get correct PC */
		for (index = 0; index < cmdq_core_max_task_in_thread(thread); index++) {
			pNGTask = pThread->pCurTask[index];
			if (NULL == pNGTask)
				continue;

			pcVA = cmdq_core_get_pc(pNGTask, thread, insts);
			if (pcVA) {
				/* we got NG task ! */
				break;
			}
		}
	}
	if (NULL == pNGTask)
		pNGTask = pTask;

	/* Do summary ! */
	CMDQ_ERR("***************************************\n");
	cmdq_core_parse_error(pNGTask, thread, &module, &irqFlag, &instA, &instB);
	CMDQ_ERR("** [Module] %s **\n", module);
	if (pTask != pNGTask) {
		CMDQ_ERR
		    ("** [Note] PC is not in first error task (0x%p) but in previous task (0x%p) **\n",
		     pTask, pNGTask);
	}
	CMDQ_ERR("** [Error Info] Refer to instruction and check engine dump for debug**\n");
	cmdq_core_dump_error_instruction(pcVA, insts, thread, __LINE__);
	cmdq_core_dump_disp_trigger_loop("ERR");
	CMDQ_ERR("***************************************\n");

	*pOutNGTask = pNGTask;
}

static void cmdqCoreDumpCommandMem(const uint32_t *pCmd, int32_t commandSize)
{
	static char textBuf[128] = { 0 };
	int i;

	mutex_lock(&gCmdqTaskMutex);

	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 16, 4, pCmd, commandSize, false);
	CMDQ_LOG("======TASK command buffer END\n");

	for (i = 0; i < commandSize; i += CMDQ_INST_SIZE, pCmd += 2) {
		cmdq_core_parse_instruction(pCmd, textBuf, 128);
		CMDQ_LOG("%s", textBuf);
	}
	CMDQ_LOG("TASK command buffer TRANSLATED END\n");

	mutex_unlock(&gCmdqTaskMutex);
}

int32_t cmdqCoreDebugDumpCommand(TaskStruct *pTask)
{
	if (NULL == pTask)
		return -EFAULT;

	CMDQ_LOG("======TASK 0x%p , size (%d) command START\n", pTask, pTask->commandSize);
	cmdqCoreDumpCommandMem(pTask->pVABase, pTask->commandSize);
	CMDQ_LOG("======TASK 0x%p command END\n", pTask);

	return 0;
}

void cmdq_core_set_command_buffer_dump(int32_t scenario, int32_t bufferSize)
{
	mutex_lock(&gCmdqSaveBufferMutex);

	if (bufferSize != gCmdqBufferDump.bufferSize && bufferSize != -1) {
		if (gCmdqBufferDump.bufferSize != 0) {
			vfree(gCmdqBufferDump.cmdqString);
			gCmdqBufferDump.bufferSize = 0;
			gCmdqBufferDump.count = 0;
		}

		if (bufferSize > 0) {
			gCmdqBufferDump.bufferSize = bufferSize;
			gCmdqBufferDump.cmdqString = vmalloc(gCmdqBufferDump.bufferSize);
		}
	}

	if (-1 == scenario) {
		/* clear all scenario */
		gCmdqBufferDump.scenario = 0LL;
	} else if (-2 == scenario) {
		/* set all scenario */
		gCmdqBufferDump.scenario = ~0LL;
	} else if (scenario >= 0 && scenario < CMDQ_MAX_SCENARIO_COUNT) {
		/* set scenario to save command buffer */
		gCmdqBufferDump.scenario |= (1LL << scenario);
	}

	CMDQ_LOG("[SET DUMP]CONFIG: bufferSize: %d, scenario: 0x%08llx\n",
		gCmdqBufferDump.bufferSize, gCmdqBufferDump.scenario);

	mutex_unlock(&gCmdqSaveBufferMutex);
}

static void cmdq_core_save_buffer(const char *string, ...)
{
	int logLen, redundantLen, i;
	va_list argptr;
	char *pBuffer;

	va_start(argptr, string);
	do {
		logLen = vsnprintf(NULL, 0, string, argptr) + 1;
		if (logLen <= 1)
			break;

		redundantLen = gCmdqBufferDump.bufferSize - gCmdqBufferDump.count;
		if (logLen >= redundantLen) {
			for (i = 0; i < redundantLen; i++)
				*(gCmdqBufferDump.cmdqString + gCmdqBufferDump.count + i) = 0;
			gCmdqBufferDump.count = 0;
		}

		pBuffer = gCmdqBufferDump.cmdqString + gCmdqBufferDump.count;
		gCmdqBufferDump.count += vsnprintf(pBuffer, logLen, string, argptr);
	} while (0);

	va_end(argptr);
}

static void cmdq_core_save_command_buffer_dump(const TaskStruct *pTask)
{
	static char textBuf[128] = { 0 };
	struct timeval savetv;
	struct tm nowTM;
	unsigned long long saveTimeSec;
	unsigned long rem_nsec;
	const uint32_t *pCmd;
	int i;

	if (gCmdqContext.errNum > 0)
		return;

	if (gCmdqBufferDump.bufferSize <= 0 || NULL == pTask->pVABase)
		return;

	mutex_lock(&gCmdqSaveBufferMutex);

	if (gCmdqBufferDump.scenario & (1LL << pTask->scenario)) {
		pCmd = pTask->pVABase;

		cmdq_core_save_buffer("************TASK command buffer TRANSLATED************\n");
		/* get kernel time */
		saveTimeSec = sched_clock();
		rem_nsec = do_div(saveTimeSec, 1000000000);
		/* get UTC time */
		do_gettimeofday(&savetv);
		time_to_tm(savetv.tv_sec, sys_tz.tz_minuteswest * 60, &nowTM);
		/* print current task information */
		cmdq_core_save_buffer("kernel time:[%5llu.%06lu],", saveTimeSec, rem_nsec / 1000);
		cmdq_core_save_buffer(" UTC time:[%04ld-%02d-%02d %02d:%02d:%02d.%06ld]",
				   (nowTM.tm_year + 1900), (nowTM.tm_mon + 1), nowTM.tm_mday,
				   nowTM.tm_hour, nowTM.tm_min, nowTM.tm_sec,
				   savetv.tv_usec);
		cmdq_core_save_buffer(" Pid: %d, Name: %s\n", pTask->callerPid, pTask->callerName);
		cmdq_core_save_buffer("Task: 0x%p, Scenario: %d, Size: %d, Flag: 0x%016llx\n",
			pTask, pTask->scenario, pTask->commandSize, pTask->engineFlag);
		/* print command buffer */
		for (i = 0; i < pTask->commandSize; i += CMDQ_INST_SIZE, pCmd += 2) {
			cmdq_core_parse_instruction(pCmd, textBuf, 128);
			cmdq_core_save_buffer("[%5llu.%06lu] %s", saveTimeSec, rem_nsec / 1000, textBuf);
		}
		cmdq_core_save_buffer("****************TASK command buffer END***************\n\n");
	}

	mutex_unlock(&gCmdqSaveBufferMutex);
}

#ifdef CMDQ_INSTRUCTION_COUNT
CmdqModulePAStatStruct gCmdqModulePAStat;

CmdqModulePAStatStruct *cmdq_core_Initial_and_get_module_stat(void)
{
	memset(&gCmdqModulePAStat, 0, sizeof(gCmdqModulePAStat));

	return &gCmdqModulePAStat;
}

ssize_t cmdqCorePrintInstructionCountLevel(struct device *dev, struct device_attribute *attr,
					   char *buf)
{
	int len = 0;

	if (buf)
		len = sprintf(buf, "%d\n", gCmdqContext.instructionCountLevel);

	return len;
}

ssize_t cmdqCoreWriteInstructionCountLevel(struct device *dev,
					   struct device_attribute *attr, const char *buf,
					   size_t size)
{
	int len = 0;
	int value = 0;
	int status = 0;

	char textBuf[10] = { 0 };

	do {
		if (size >= 10) {
			status = -EFAULT;
			break;
		}

		len = size;
		memcpy(textBuf, buf, len);

		textBuf[len] = '\0';
		if (0 > kstrtoint(textBuf, 10, &value)) {
			status = -EFAULT;
			break;
		}

		status = len;
		if (value < 0)
			value = 0;

		cmdq_core_set_instruction_count_level(value);
	} while (0);

	return status;
}

void cmdq_core_set_instruction_count_level(const int32_t value)
{
	gCmdqContext.instructionCountLevel = value;
}

static void cmdq_core_fill_module_stat(const uint32_t *pCommand,
				       unsigned short *pModuleCount,
				       uint32_t *pOtherInstruction,
				       uint32_t *pOtherInstructionCount)
{

	const uint32_t argA = pCommand[1] & (~0xFF000000);
	const uint32_t addr = cmdq_core_subsys_to_reg_addr(argA);
	int32_t i;

	for (i = 0; i < CMDQ_MODULE_STAT_GPR; i++) {
		if ((gCmdqModulePAStat.start[i] > 0) && (addr >= gCmdqModulePAStat.start[i])
		    && (addr <= gCmdqModulePAStat.end[i])) {
			pModuleCount[i]++;
			break;
		}
	}

	if (i >= CMDQ_MODULE_STAT_GPR) {
		if (3 & (pCommand[1] >> 22)) {
			pModuleCount[CMDQ_MODULE_STAT_GPR]++;
		} else {
			pOtherInstruction[(*pOtherInstructionCount)++] = addr;
			pModuleCount[CMDQ_MODULE_STAT_OTHERS]++;
		}
	}
}

static void cmdq_core_fill_module_event_count(const uint32_t *pCommand,
					      unsigned short *pEventCount)
{

	const uint32_t argA = pCommand[1] & (~0xFF000000);

	if (argA >= CMDQ_MAX_HW_EVENT_COUNT)
		pEventCount[CMDQ_EVENT_STAT_SW]++;
	else
		pEventCount[CMDQ_EVENT_STAT_HW]++;
}

static void cmdq_core_fill_task_instruction_stat(RecordStruct *pRecord, const TaskStruct *pTask)
{
	bool invalidinstruction = false;
	int32_t commandIndex = 0;
	uint32_t argA_prefetch_en, argB_prefetch_en, argA_prefetch_dis, argB_prefetch_dis;
	uint32_t *pCommand;
	uint32_t op;

	if (gCmdqContext.instructionCountLevel < 1)
		return;

	if ((NULL == pRecord) || (NULL == pTask))
		return;

	memset(&(pRecord->instructionStat[0]), 0x0, sizeof(pRecord->instructionStat));
	memset(&(pRecord->writeModule[0]), 0x0, sizeof(pRecord->writeModule));
	memset(&(pRecord->writewmaskModule[0]), 0x0, sizeof(pRecord->writewmaskModule));
	memset(&(pRecord->readModlule[0]), 0x0, sizeof(pRecord->readModlule));
	memset(&(pRecord->pollModule[0]), 0x0, sizeof(pRecord->pollModule));
	memset(&(pRecord->eventCount[0]), 0x0, sizeof(pRecord->eventCount));
	memset(&(pRecord->otherInstr[0]), 0x0, sizeof(pRecord->otherInstr));
	pRecord->otherInstrNUM = 0;

	do {
		pCommand = (uint32_t *) ((uint8_t *) pTask->pVABase + commandIndex);
		op = (pCommand[1] & 0xFF000000) >> 24;

		switch (op) {
		case CMDQ_CODE_MOVE:
			if (1 & (pCommand[1] >> 23)) {
				if (CMDQ_SPECIAL_SUBSYS_ADDR ==
				    cmdq_core_subsys_from_phys_addr(pCommand[0])) {
					commandIndex += CMDQ_INST_SIZE;
					pRecord->instructionStat[CMDQ_STAT_WRITE]++;
					pRecord->writeModule[CMDQ_MODULE_STAT_DISP_PWM]++;
				} else {
					pRecord->instructionStat[CMDQ_STAT_MOVE]++;
				}
			} else if ((commandIndex + CMDQ_INST_SIZE) < pTask->commandSize) {
				commandIndex += CMDQ_INST_SIZE;
				pCommand = (uint32_t *) ((uint8_t *) pTask->pVABase + commandIndex);
				op = (pCommand[1] & 0xFF000000) >> 24;

				if (CMDQ_CODE_WRITE == op) {
					pRecord->instructionStat[CMDQ_STAT_WRITE_W_MASK]++;
					cmdq_core_fill_module_stat(pCommand,
								   pRecord->writewmaskModule,
								   pRecord->otherInstr,
								   &(pRecord->otherInstrNUM));
				} else if (CMDQ_CODE_POLL == op) {
					pRecord->instructionStat[CMDQ_STAT_POLLING]++;
					cmdq_core_fill_module_stat(pCommand,
								   pRecord->pollModule,
								   pRecord->otherInstr,
								   &(pRecord->otherInstrNUM));
				} else {
					invalidinstruction = true;
				}
			} else {
				invalidinstruction = true;
			}
			break;
		case CMDQ_CODE_READ:
			pRecord->instructionStat[CMDQ_STAT_READ]++;
			cmdq_core_fill_module_stat(pCommand, pRecord->readModlule,
						   pRecord->otherInstr, &(pRecord->otherInstrNUM));
			break;
		case CMDQ_CODE_WRITE:
			pRecord->instructionStat[CMDQ_STAT_WRITE]++;
			cmdq_core_fill_module_stat(pCommand, pRecord->writeModule,
						   pRecord->otherInstr, &(pRecord->otherInstrNUM));
			break;
		case CMDQ_CODE_WFE:
			pRecord->instructionStat[CMDQ_STAT_SYNC]++;
			cmdq_core_fill_module_event_count(pCommand, pRecord->eventCount);
			break;
		case CMDQ_CODE_JUMP:
			pRecord->instructionStat[CMDQ_STAT_JUMP]++;
			break;
		case CMDQ_CODE_EOC:
			argB_prefetch_en = ((1 << 20) | (1 << 17) | (1 << 16));
			argA_prefetch_en =
			    (CMDQ_CODE_EOC << 24) | (0x1 << (53 - 32)) | (0x1 << (48 - 32));
			argB_prefetch_dis = (1 << 20);
			argA_prefetch_dis = (CMDQ_CODE_EOC << 24) | (0x1 << (48 - 32));

			if ((argB_prefetch_en == pCommand[0]) && (argA_prefetch_en == pCommand[1])) {
				pRecord->instructionStat[CMDQ_STAT_PREFETCH_EN]++;
			} else if ((argB_prefetch_dis == pCommand[0])
				   && (argA_prefetch_dis == pCommand[1])) {
				pRecord->instructionStat[CMDQ_STAT_PREFETCH_DIS]++;
			} else {
				pRecord->instructionStat[CMDQ_STAT_EOC]++;
			}
			break;
		default:
			invalidinstruction = true;
			break;
		}
		commandIndex += CMDQ_INST_SIZE;
		if (true == invalidinstruction) {
			memset(&(pRecord->instructionStat[0]), 0x0,
			       sizeof(pRecord->instructionStat));
			break;
		}
	} while (commandIndex < pTask->commandSize);
}

static const char *gCmdqModuleInstructionLabel[CMDQ_MODULE_STAT_MAX] = {
	"MMSYS_CONFIG",
	"MDP_RDMA",
	"MDP_RSZ0",
	"MDP_RSZ1",
	"MDP_WDMA",
	"MDP_WROT",
	"MDP_TDSHP",
	"MM_MUTEX",
	"VENC",
	"DISP_OVL0",
	"DISP_OVL1",
	"DISP_RDMA0",
	"DISP_RDMA1",
	"DISP_WDMA0",
	"DISP_COLOR",
	"DISP_CCORR",
	"DISP_AAL",
	"DISP_GAMMA",
	"DISP_DITHER",
	"DISP_UFOE",
	"DISP_PWM",
	"DISP_WDMA1",
	"DISP_MUTEX",
	"DISP_DSI0",
	"DISP_DPI0",
	"DISP_OD",
	"CAM0",
	"CAM1",
	"CAM2",
	"CAM3",
	"SODI",
	"GPR",
	"Others",
};

int cmdqCorePrintInstructionCountSeq(struct seq_file *m, void *v)
{
	unsigned long flags;
	int32_t i;
	int32_t index;
	int32_t numRec;
	RecordStruct record;

	if (gCmdqContext.instructionCountLevel < 1)
		return 0;

	seq_puts(m, "Record ID, PID, scenario, total, write, write_w_mask, read,");
	seq_puts(m, " polling, move, sync, prefetch_en, prefetch_dis, EOC, jump");
	for (i = 0; i < CMDQ_MODULE_STAT_MAX; i++) {
		seq_printf(m, ", (%s)=>, write, write_w_mask, read, polling",
			   gCmdqModuleInstructionLabel[i]);
	}
	seq_puts(m, ", (SYNC)=>, HW Event, SW Event\n");
	/* we try to minimize time spent in spin lock */
	/* since record is an array so it is okay to */
	/* allow displaying an out-of-date entry. */
	spin_lock_irqsave(&gCmdqRecordLock, flags);
	numRec = gCmdqContext.recNum;
	index = gCmdqContext.lastID - 1;
	spin_unlock_irqrestore(&gCmdqRecordLock, flags);

	/* we print record in reverse order. */
	for (; numRec > 0; --numRec, --index) {
		if (index >= CMDQ_MAX_RECORD_COUNT)
			index = 0;
		else if (index < 0)
			index = CMDQ_MAX_RECORD_COUNT - 1;

		/* Make sure we don't print a record that is during updating. */
		/* However, this record may already be different */
		/* from the time of entering cmdqCorePrintRecordSeq(). */
		spin_lock_irqsave(&gCmdqRecordLock, flags);
		record = gCmdqContext.record[index];
		spin_unlock_irqrestore(&gCmdqRecordLock, flags);

		if ((0 == record.instructionStat[CMDQ_STAT_EOC]) &&
		    (0 == record.instructionStat[CMDQ_STAT_JUMP])) {
			seq_printf(m, "%4d, %5c, %2c, %4d", index, 'X', 'X', 0);
			for (i = 0; i < CMDQ_STAT_MAX; i++)
				seq_printf(m, ", %4d", 0);

			for (i = 0; i < CMDQ_MODULE_STAT_MAX; i++)
				seq_printf(m, ", , %4d, %4d, %4d, %4d", 0, 0, 0, 0);

			seq_printf(m, ", , %4d, %4d", 0, 0);
		} else {
			uint32_t totalCount = (uint32_t) (record.size / CMDQ_INST_SIZE);

			seq_printf(m, " %4d, %5d, %02d, %4d", index, record.user, record.scenario,
				   totalCount);
			for (i = 0; i < CMDQ_STAT_MAX; i++)
				seq_printf(m, ", %4d", record.instructionStat[i]);

			for (i = 0; i < CMDQ_MODULE_STAT_MAX; i++) {
				seq_printf(m, ", , %4d, %4d, %4d, %4d", record.writeModule[i],
					   record.writewmaskModule[i], record.readModlule[i],
					   record.pollModule[i]);
			}
			seq_printf(m, ", , %4d, %4d",
				   record.eventCount[CMDQ_EVENT_STAT_HW],
				   record.eventCount[CMDQ_EVENT_STAT_SW]);
		}
		seq_puts(m, "\n");

	}

	seq_puts(m, "\n\n==============Other Instruction==============\n");
	/* we try to minimize time spent in spin lock */
	/* since record is an array so it is okay to */
	/* allow displaying an out-of-date entry. */
	spin_lock_irqsave(&gCmdqRecordLock, flags);
	numRec = gCmdqContext.recNum;
	index = gCmdqContext.lastID - 1;
	spin_unlock_irqrestore(&gCmdqRecordLock, flags);

	/* we print record in reverse order. */
	for (; numRec > 0; --numRec, --index) {
		if (index >= CMDQ_MAX_RECORD_COUNT)
			index = 0;
		else if (index < 0)
			index = CMDQ_MAX_RECORD_COUNT - 1;

		/* Make sure we don't print a record that is during updating. */
		/* However, this record may already be different */
		/* from the time of entering cmdqCorePrintRecordSeq(). */
		spin_lock_irqsave(&gCmdqRecordLock, flags);
		record = gCmdqContext.record[index];
		spin_unlock_irqrestore(&gCmdqRecordLock, flags);

		for (i = 0; i < record.otherInstrNUM; i++)
			seq_printf(m, "0x%08x\n", record.otherInstr[i]);
	}

	return 0;
}
#endif

static void cmdq_core_fill_task_profile_marker_record(RecordStruct *pRecord,
						      const TaskStruct *pTask)
{
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	uint32_t i;
	uint32_t profileMarkerCount;
	uint32_t value;
	cmdqBackupSlotHandle hSlot;

	if ((NULL == pRecord) || (NULL == pTask))
		return;

	if (0 == pTask->profileMarker.hSlot)
		return;

	profileMarkerCount = pTask->profileMarker.count;
	hSlot = (cmdqBackupSlotHandle) (pTask->profileMarker.hSlot);

	pRecord->profileMarkerCount = profileMarkerCount;
	for (i = 0; i < profileMarkerCount; i++) {
		/* timestamp, each count is 76ns */
		cmdqBackupReadSlot(hSlot, i, &value);
		pRecord->profileMarkerTimeNS[i] = value * 76;
		pRecord->profileMarkerTag[i] = (char *)(const unsigned long long *)&pTask->profileMarker.tag[i];
	}
#endif
}

static void cmdq_core_fill_task_record(RecordStruct *pRecord, const TaskStruct *pTask,
				       uint32_t thread)
{
	uint32_t begin, end;

	if (pRecord && pTask) {
		/* Record scenario */
		pRecord->user = pTask->callerPid;
		pRecord->scenario = pTask->scenario;
		pRecord->priority = pTask->priority;
		pRecord->thread = thread;
		pRecord->reorder = pTask->reorder;
		pRecord->engineFlag = pTask->engineFlag;
		pRecord->size = pTask->commandSize;

		pRecord->isSecure = pTask->secData.isSecure;

		if (NULL == pTask->profileData) {
			pRecord->writeTimeNS = 0;
			pRecord->writeTimeNSBegin = 0;
			pRecord->writeTimeNSEnd = 0;
		} else {
			/* Command exec time, each count is 76ns */
			begin = *((volatile uint32_t *)pTask->profileData);
			end = *((volatile uint32_t *)(pTask->profileData + 1));
			pRecord->writeTimeNS = (end - begin) * 76;

			pRecord->writeTimeNSBegin = (begin) * 76;
			pRecord->writeTimeNSEnd = (end) * 76;
		}

		/* Record time */
		pRecord->submit = pTask->submit;
		pRecord->trigger = pTask->trigger;
		pRecord->gotIRQ = pTask->gotIRQ;
		pRecord->beginWait = pTask->beginWait;
		pRecord->wakedUp = pTask->wakedUp;

		cmdq_core_fill_task_profile_marker_record(pRecord, pTask);
#ifdef CMDQ_INSTRUCTION_COUNT
		/* Instruction count statistics */
		cmdq_core_fill_task_instruction_stat(pRecord, pTask);
#endif
	}
}

static void cmdq_core_track_task_record(TaskStruct *pTask, uint32_t thread)
{
	RecordStruct *pRecord;
	unsigned long flags;
	CMDQ_TIME done;

#if 0
	if (cmdq_get_func()->shouldProfile(pTask->scenario))
		return;
#endif

	done = sched_clock();

	spin_lock_irqsave(&gCmdqRecordLock, flags);

	pRecord = &(gCmdqContext.record[gCmdqContext.lastID]);

	cmdq_core_fill_task_record(pRecord, pTask, thread);

	pRecord->done = done;

	gCmdqContext.lastID++;
	if (gCmdqContext.lastID >= CMDQ_MAX_RECORD_COUNT)
		gCmdqContext.lastID = 0;

	gCmdqContext.recNum++;
	if (gCmdqContext.recNum >= CMDQ_MAX_RECORD_COUNT)
		gCmdqContext.recNum = CMDQ_MAX_RECORD_COUNT;

	spin_unlock_irqrestore(&gCmdqRecordLock, flags);
}

void cmdq_core_dump_GIC(void)
{
#ifndef CMDQ_OF_SUPPORT		/* OF Support removes mt_irq.h, mt_irq_dump_status support will be added later. */
#if CMDQ_DUMP_GIC
	mt_irq_dump_status(cmdq_dev_get_irq_id());
	mt_irq_dump_status(cmdq_dev_get_irq_secure_id());
#endif
#endif
}

static void cmdq_core_dump_error_buffer(const TaskStruct *pTask, uint32_t *hwPC)
{
	if (hwPC && pTask && hwPC >= pTask->pVABase) {
		/* because hwPC points to "start" of the instruction */
		/* add offset 1 */
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 16, 4,
			       pTask->pVABase, (2 + hwPC - pTask->pVABase) * sizeof(uint32_t),
			       true);
		cmdq_core_save_hex_first_dump("", 16, 4,
					      pTask->pVABase,
					      (2 + hwPC - pTask->pVABase) * sizeof(uint32_t));
	} else {
		CMDQ_ERR("hwPC is not in region, dump all\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 16, 4,
			       pTask->pVABase, (pTask->commandSize), true);
		cmdq_core_save_hex_first_dump("", 16, 4, pTask->pVABase, (pTask->commandSize));
	}
}

static void cmdq_core_dump_error_task(const TaskStruct *pTask, const TaskStruct *pNGTask, uint32_t thread)
{
	CmdqCBkStruct *pCallback = NULL;
	ThreadStruct *pThread;
	int32_t index = 0;
	uint32_t *hwPC = NULL;
	uint32_t *hwNGPC = NULL;
	uint64_t printEngineFlag = 0;
	uint32_t value[10] = { 0 };

	static const char *const engineGroupName[] = {
		CMDQ_FOREACH_GROUP(GENERATE_STRING)
	};

	CMDQ_ERR("=============== [CMDQ] Error Thread Status ===============\n");
	pThread = &(gCmdqContext.thread[thread]);
	if (false == cmdq_get_func()->isSecureThread(thread)) {
		/* normal thread */
		value[0] = CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread));
		value[1] = CMDQ_REG_GET32(CMDQ_THR_END_ADDR(thread));
		value[2] = CMDQ_REG_GET32(CMDQ_THR_WAIT_TOKEN(thread));
		value[3] = cmdq_core_thread_exec_counter(thread);
		value[4] = CMDQ_REG_GET32(CMDQ_THR_IRQ_STATUS(thread));
		value[5] = CMDQ_REG_GET32(CMDQ_THR_INST_CYCLES(thread));
		value[6] = CMDQ_REG_GET32(CMDQ_THR_CURR_STATUS(thread));
		value[7] = CMDQ_REG_GET32(CMDQ_THR_IRQ_ENABLE(thread));
		value[8] = CMDQ_REG_GET32(CMDQ_THR_ENABLE_TASK(thread));

		CMDQ_ERR
		    ("Index: %d, Enabled: %d, IRQ: 0x%08x, Thread PC: 0x%08x, End: 0x%08x, Wait Token: 0x%08x\n",
		     thread, value[8], value[4], value[0], value[1], value[2]);
		CMDQ_ERR
		    ("Curr Cookie: %d, Wait Cookie: %d, Next Cookie: %d, Task Count %d, engineFlag: 0x%llx\n",
		     value[3], pThread->waitCookie, pThread->nextCookie, pThread->taskCount,
		     pThread->engineFlag);
		CMDQ_ERR("Timeout Cycle:%d, Status:0x%08x, IRQ_EN: 0x%08x\n", value[5], value[6],
			 value[7]);
	} else {
		/* do nothing since it's a secure thread */
		CMDQ_ERR("Wait Cookie: %d, Next Cookie: %d, Task Count %d,\n",
			 pThread->waitCookie, pThread->nextCookie, pThread->taskCount);
	}

	/* Begin is not first, save NG task but print pTask as well */
	if (pNGTask != pTask && NULL != pNGTask) {
		CMDQ_ERR("== [CMDQ] We have NG task, so engine dumps may more than you think ==\n");
		CMDQ_ERR("========== [CMDQ] Error Thread PC (NG Task) ==========\n");
		hwNGPC = cmdq_core_dump_pc(pNGTask, thread, "ERR");

		CMDQ_ERR("========= [CMDQ] Error Task Status (NG Task) =========\n");
		cmdq_core_dump_task(pNGTask);

		printEngineFlag |= pNGTask->engineFlag;
	}

	if (NULL != pTask) {
		CMDQ_ERR("=============== [CMDQ] Error Thread PC ===============\n");
		hwPC = cmdq_core_dump_pc(pTask, thread, "ERR");

		CMDQ_ERR("=============== [CMDQ] Error Task Status ===============\n");
		cmdq_core_dump_task(pTask);

		printEngineFlag |= pTask->engineFlag;
	}

	/* dump tasks in error thread */
	cmdq_core_dump_task_in_thread(thread, false, false, false);
	cmdq_core_dump_task_with_engine_flag(printEngineFlag);

	CMDQ_ERR("=============== [CMDQ] CMDQ Status ===============\n");
	cmdq_core_dump_status("ERR");

#ifndef CONFIG_MTK_FPGA
	CMDQ_ERR("=============== [CMDQ] SMI Status ===============\n");
	cmdq_get_func()->dumpSMI(1);
#endif

	CMDQ_ERR("=============== [CMDQ] Clock Gating Status ===============\n");
	CMDQ_ERR("[CLOCK] common clock ref=%d\n", atomic_read(&gCmdqThreadUsage));
	cmdq_get_func()->dumpClockGating();

	/*      */
	/* Dump MMSYS configuration */
	/*      */
	CMDQ_ERR("=============== [CMDQ] MMSYS_CONFIG ===============\n");
	cmdq_mdp_get_func()->dumpMMSYSConfig();

	/*      */
	/* ask each module to print their status */
	/*      */
	CMDQ_ERR("=============== [CMDQ] Engine Status ===============\n");
	pCallback = gCmdqGroupCallback;
	for (index = 0; index < CMDQ_MAX_GROUP_COUNT; ++index) {
		if (!cmdq_core_is_group_flag((CMDQ_GROUP_ENUM) index, printEngineFlag))
			continue;

		CMDQ_ERR("====== engine group %s status =======\n", engineGroupName[index]);

		if (NULL == pCallback[index].dumpInfo) {
			CMDQ_ERR("(no dump function)\n");
			continue;
		}

		pCallback[index].dumpInfo((gCmdqEngineGroupBits[index] & printEngineFlag),
					  gCmdqContext.logLevel);
	}

	/* force dump DISP for DISP scenario with 0x0 engine flag */
	if (cmdq_get_func()->isDispScenario(pTask->scenario)
		|| cmdq_get_func()->isDispScenario(pNGTask->scenario)) {
		index = CMDQ_GROUP_DISP;
		if (pCallback[index].dumpInfo) {
			pCallback[index].dumpInfo((gCmdqEngineGroupBits[index] & printEngineFlag),
						  gCmdqContext.logLevel);
		}
	}

	CMDQ_ERR("=============== [CMDQ] GIC dump ===============\n");
	cmdq_core_dump_GIC();

	/* Begin is not first, save NG task but print pTask as well */
	if (pNGTask != pTask && NULL != pNGTask) {
		CMDQ_ERR("========== [CMDQ] Error Command Buffer (NG Task) ==========\n");
		cmdq_core_dump_error_buffer(pNGTask, hwNGPC);
	}

	if (NULL != pTask) {
		CMDQ_ERR("=============== [CMDQ] Error Command Buffer ===============\n");
		cmdq_core_dump_error_buffer(pTask, hwPC);
	}
}

static void cmdq_core_attach_error_task(const TaskStruct *pTask, int32_t thread,
					const TaskStruct **pOutNGTask)
{
	EngineStruct *pEngine = NULL;
	ThreadStruct *pThread = NULL;
	const TaskStruct *pNGTask = NULL;
	uint64_t engFlag = 0;
	int32_t index = 0;

	pThread = &(gCmdqContext.thread[thread]);
	pEngine = gCmdqContext.engine;

	CMDQ_PROF_MMP(cmdq_mmp_get_event()->warning, MMProfileFlagPulse, ((unsigned long)pTask),
		      thread);

	/*  */
	/* Update engine fail count */
	/*  */
	if (pTask) {
		engFlag = pTask->engineFlag;
		for (index = 0; index < CMDQ_MAX_ENGINE_COUNT; index++) {
			if (engFlag & (1LL << index))
				pEngine[index].failCount++;
		}
	}

	/*  */
	/* register error record */
	/*  */
	if (gCmdqContext.errNum < CMDQ_MAX_ERROR_COUNT) {
		ErrorStruct *pError = &gCmdqContext.error[gCmdqContext.errNum];

		cmdq_core_fill_task_record(&pError->errorRec, pTask, thread);
		pError->ts_nsec = local_clock();
	}
#ifdef CMDQ_DUMP_FIRSTERROR
	if (0 == gCmdqFirstError.cmdqCount) {
		gCmdqFirstError.flag = true;
		/* save kernel time, pid, and caller name */
		gCmdqFirstError.callerPid = pTask->callerPid;
		snprintf(gCmdqFirstError.callerName, TASK_COMM_LEN, "%s", pTask->callerName);
		gCmdqFirstError.savetime = sched_clock();
		do_gettimeofday(&gCmdqFirstError.savetv);
	}
#endif

	/*  */
	/* Then we just print out info */
	/*  */
	CMDQ_ERR("================= [CMDQ] Begin of Error %d================\n",
		 gCmdqContext.errNum);

	cmdq_core_dump_summary(pTask, thread, &pNGTask);
	if (gCmdqContext.errNum <= 2 || gCmdqContext.errNum % 16 == 0 || cmdq_core_should_full_error()) {
		/* Dump error task */
		cmdq_core_dump_error_task(pTask, pNGTask, thread);
	}

	CMDQ_ERR("================= [CMDQ] End of Error %d ================\n",
		 gCmdqContext.errNum);
	gCmdqContext.errNum++;

	if (pOutNGTask != NULL) {
		if (NULL != pNGTask)
			*pOutNGTask = pNGTask;
		else
			*pOutNGTask = pTask;
	}
}

static int32_t cmdq_core_insert_task_from_thread_array_by_cookie(TaskStruct *pTask,
								 ThreadStruct *pThread,
								 const int32_t cookie,
								 const bool resetHWThread)
{

	if (NULL == pTask || NULL == pThread) {
		CMDQ_ERR("invalid param, pTask[0x%p], pThread[0x%p], cookie[%d], needReset[%d]\n",
			 pTask, pThread, cookie, resetHWThread);
		return -EFAULT;
	}

	if (true == resetHWThread) {
		pThread->waitCookie = cookie;
		pThread->nextCookie = cookie + 1;
		if (pThread->nextCookie > CMDQ_MAX_COOKIE_VALUE) {
			/* Reach the maximum cookie */
			pThread->nextCookie = 0;
		}

		/* taskCount must start from 0. */
		/* and we are the first task, so set to 1. */
		pThread->taskCount = 1;

	} else {
		pThread->nextCookie += 1;
		if (pThread->nextCookie > CMDQ_MAX_COOKIE_VALUE) {
			/* Reach the maximum cookie */
			pThread->nextCookie = 0;
		}

		pThread->taskCount++;
	}

	/* genernal part */
	pThread->pCurTask[cookie % cmdq_core_max_task_in_thread(pTask->thread)] = pTask;
	pThread->allowDispatching = 1;

	/* secure path */
	if (pTask->secData.isSecure) {
		pTask->secData.waitCookie = cookie;
		pTask->secData.resetExecCnt = resetHWThread;
	}

	return 0;
}

static int32_t cmdq_core_remove_task_from_thread_array_by_cookie(ThreadStruct *pThread,
								 int32_t index,
								 TASK_STATE_ENUM newTaskState)
{
	TaskStruct *pTask = NULL;

	if ((NULL == pThread) || (0 > index)) {
		CMDQ_ERR
		    ("remove task from thread array, invalid param. THR[0x%p], task_slot[%d], newTaskState[%d]\n",
		     pThread, index, newTaskState);
		return -EINVAL;
	}

	pTask = pThread->pCurTask[index];
	if (cmdq_core_max_task_in_thread(pTask->thread) <= index) {
		CMDQ_ERR
		    ("remove task from thread array, invalid index. THR[0x%p], task_slot[%d], newTaskState[%d]\n",
		     pThread, index, newTaskState);
		return -EINVAL;
	}

	if (NULL == pTask) {
		CMDQ_ERR("remove fail, task_slot[%d] on thread[%p] is NULL\n", index, pThread);
		return -EINVAL;
	}

	/* to switch a task to done_status(_ERROR, _KILLED, _DONE) is aligned with thread's taskcount change */
	/* check task status to prevent double clean-up thread's taskcount */
	if (TASK_STATE_BUSY != pTask->taskState) {
		CMDQ_ERR
		    ("remove task, taskStatus err[%d]. THR[0x%p], task_slot[%d], targetTaskStaus[%d]\n",
		     pTask->taskState, pThread, index, newTaskState);
		return -EINVAL;
	}

	CMDQ_VERBOSE("remove task, slot[%d], targetStatus: %d\n", index, newTaskState);
	pTask->taskState = newTaskState;
	pTask = NULL;
	pThread->pCurTask[index] = NULL;
	pThread->taskCount--;

	if (0 > pThread->taskCount) {
		/* Error status print */
		CMDQ_ERR("taskCount < 0 after cmdq_core_remove_task_from_thread_array_by_cookie\n");
	}

	return 0;
}

static int32_t cmdq_core_force_remove_task_from_thread(TaskStruct *pTask, uint32_t thread)
{
	int32_t status = 0;
	int32_t cookie = 0;
	int index = 0;
	int loop = 0;
	struct TaskStruct *pExecTask = NULL;
	struct ThreadStruct *pThread = &(gCmdqContext.thread[thread]);

	status = cmdq_core_suspend_HW_thread(thread, __LINE__);

	CMDQ_REG_SET32(CMDQ_THR_INST_CYCLES(thread), cmdq_core_get_task_timeout_cycle(pThread));

	/* The cookie of the task currently being processed */
	cookie = CMDQ_GET_COOKIE_CNT(thread) + 1;

	pExecTask = pThread->pCurTask[cookie % cmdq_core_max_task_in_thread(thread)];
	if (NULL != pExecTask && (pExecTask == pTask)) {
		/* The task is executed now, set the PC to EOC for bypass */
		CMDQ_REG_SET32(CMDQ_THR_CURR_ADDR(thread),
			       CMDQ_PHYS_TO_AREG(pTask->MVABase + pTask->commandSize - 16));

		cmdq_core_reset_hw_engine(pTask->engineFlag);

		pThread->pCurTask[cookie % cmdq_core_max_task_in_thread(thread)] = NULL;
		pTask->taskState = TASK_STATE_KILLED;
	} else {
		loop = pThread->taskCount;
		for (index = (cookie % cmdq_core_max_task_in_thread(thread)); loop > 0; loop--, index++) {
			if (index >= cmdq_core_max_task_in_thread(thread))
				index = 0;

			pExecTask = pThread->pCurTask[index];
			if (NULL == pExecTask)
				continue;

			if ((0x10000000 == pExecTask->pCMDEnd[0]) &&
			    (0x00000008 == pExecTask->pCMDEnd[-1])) {
				/* We reached the last task */
				break;
			} else if (pExecTask->pCMDEnd[-1] == pTask->MVABase) {
				/* Fake EOC command */
				pExecTask->pCMDEnd[-1] = 0x00000001;
				pExecTask->pCMDEnd[0] = 0x40000000;

				/* Bypass the task */
				pExecTask->pCMDEnd[1] = pTask->pCMDEnd[-1];
				pExecTask->pCMDEnd[2] = pTask->pCMDEnd[0];

				index += 1;
				if (index >= cmdq_core_max_task_in_thread(thread))
					index = 0;

				pThread->pCurTask[index] = NULL;
				pTask->taskState = TASK_STATE_KILLED;
				status = 0;
				break;
			}
		}
	}

	return status;
}

static void cmdq_core_handle_done_with_cookie_impl(int32_t thread,
						   int32_t value, CMDQ_TIME *pGotIRQ,
						   const uint32_t cookie)
{
#ifdef CMDQ_MDP_MET_STATUS
	struct TaskStruct *pTask;
#endif
	ThreadStruct *pThread;
	int32_t count;
	int32_t inner;
	int32_t maxTaskNUM = cmdq_core_max_task_in_thread(thread);

	pThread = &(gCmdqContext.thread[thread]);

	/* do not print excessive message for looping thread */
	if (NULL == pThread->loopCallback) {
#ifdef CONFIG_MTK_FPGA
		/* ASYNC: debug log, use printk_sched to prevent block IRQ handler */
		CMDQ_MSG("IRQ: Done, thread: %d, cookie:%d\n", thread, cookie);
#endif
	}

	if (pThread->waitCookie <= cookie) {
		count = cookie - pThread->waitCookie + 1;
	} else if ((cookie+1) % CMDQ_MAX_COOKIE_VALUE == pThread->waitCookie) {
		count = 0;
		CMDQ_MSG("IRQ: duplicated cookie: waitCookie:%d, hwCookie:%d",
			pThread->waitCookie, cookie);
	} else {
		/* Counter wrapped */
		count = (CMDQ_MAX_COOKIE_VALUE - pThread->waitCookie + 1) + (cookie + 1);
		CMDQ_ERR("IRQ: counter wrapped: waitCookie:%d, hwCookie:%d, count=%d",
			pThread->waitCookie, cookie, count);
	}

	for (inner = (pThread->waitCookie % maxTaskNUM); count > 0; count--, inner++) {
		if (inner >= maxTaskNUM)
			inner = 0;

		if (NULL != pThread->pCurTask[inner]) {
			struct TaskStruct *pTask = pThread->pCurTask[inner];

			pTask->gotIRQ = *pGotIRQ;
			pTask->irqFlag = value;
			cmdq_core_remove_task_from_thread_array_by_cookie(pThread,
									  inner, TASK_STATE_DONE);
#ifdef CMDQ_MDP_MET_STATUS
			/* MET MMSYS: Thread done */
			if (met_mmsys_event_gce_thread_end)
				met_mmsys_event_gce_thread_end(thread, (uintptr_t) pTask, pTask->engineFlag);
#endif
		}
	}

	CMDQ_PROF_MMP(cmdq_mmp_get_event()->CMDQ_IRQ, MMProfileFlagPulse, thread, cookie);

	pThread->waitCookie = cookie + 1;
	if (pThread->waitCookie > CMDQ_MAX_COOKIE_VALUE)
		pThread->waitCookie -= (CMDQ_MAX_COOKIE_VALUE + 1);	/* min cookie value is 0 */
#ifdef CMDQ_MDP_MET_STATUS
	/* MET MMSYS: GCE should trigger next waiting task */
	if ((0 < pThread->taskCount) && met_mmsys_event_gce_thread_begin) {
		count = pThread->nextCookie - pThread->waitCookie;
		for (inner = (pThread->waitCookie % maxTaskNUM); count > 0; count--, inner++) {
			if (inner >= maxTaskNUM)
				inner = 0;

			if (NULL != pThread->pCurTask[inner]) {
				pTask = pThread->pCurTask[inner];
				met_mmsys_event_gce_thread_begin(thread, (uintptr_t) pTask, pTask->engineFlag,
					(void *)pTask->pVABase, pTask->commandSize);
				break;
			}
		}
	}
#endif
	wake_up(&gCmdWaitQueue[thread]);
}

static void cmdq_core_handle_secure_thread_done_impl(const int32_t thread,
						     const int32_t value, CMDQ_TIME *pGotIRQ)
{
	const int32_t cookie = cmdq_core_get_secure_thread_exec_counter(thread);

	/* get cookie value from shared memory */
	if (0 > cookie)
		return;

	cmdq_core_handle_done_with_cookie_impl(thread, value, pGotIRQ, cookie);
}

static void cmdq_core_handle_secure_paths_exec_done_notify(const int32_t notifyThread,
							   const int32_t value, CMDQ_TIME *pGotIRQ)
{
	uint32_t i;
	int32_t thread;
	const uint32_t startThread = CMDQ_MIN_SECURE_THREAD_ID;
	const uint32_t endThread = CMDQ_MIN_SECURE_THREAD_ID + CMDQ_MAX_SECURE_THREAD_COUNT;
	int32_t raisedIRQ;

	raisedIRQ = 0x0;

	/* HACK:
	 * IRQ of the notify thread,
	 * implies threre are some secure tasks execute done.
	 *
	 * when receive it, we should
	 * .suspend notify thread
	 * .scan shared memory to update secure path task status
	 *  (and notify waiting process context to check result)
	 * .resume notify thread
	 */

	/* it's okey that SWd update and NWd read shared memory, which used to
	 * store copy value of secure thread cookie, at the same time.
	 *
	 * The reason is NWd will receive a notify thread IRQ again after resume notify thread.
	 * The later IRQ let driver scan shared memory again.
	 * (note it's possible that same content in shared memory in such case)
	 */

	/* confirm if it is notify thread */
	if (false == cmdq_get_func()->isValidNotifyThread(notifyThread))
		return;

	raisedIRQ = cmdq_core_get_secure_IRQ_status();
	CMDQ_LOG("%s, raisedIRQ:0x%08x, shared_cookie(%d, %d, %d)\n",
		 __func__,
		 raisedIRQ,
		 cmdq_core_get_secure_thread_exec_counter(12),
		 cmdq_core_get_secure_thread_exec_counter(13),
		 cmdq_core_get_secure_thread_exec_counter(14));

	/* update tasks' status according cookie in shared memory */
	for (i = startThread; i < endThread; i++) {
		/* bit X = 1 means thread X raised IRQ */
		if (0 == (raisedIRQ & (0x1 << i)))
			continue;

		thread = i;
		cmdq_core_handle_secure_thread_done_impl(thread, value, pGotIRQ);
	}

	cmdq_core_set_secure_IRQ_status(0x0);
#ifdef CMDQ_SECURE_PATH_HW_LOCK
	cmdqCoreSetEvent(CMDQ_SYNC_SECURE_WSM_LOCK);
#endif

}


static void cmdqCoreHandleError(int32_t thread, int32_t value, CMDQ_TIME *pGotIRQ)
{
	ThreadStruct *pThread = NULL;
	TaskStruct *pTask = NULL;
	int32_t cookie;
	int32_t count;
	int32_t inner;
	int32_t status = 0;

	cookie = cmdq_core_thread_exec_counter(thread);

	CMDQ_ERR("IRQ: error thread=%d, irq_flag=0x%x, cookie:%d\n", thread, value, cookie);
	CMDQ_ERR("IRQ: Thread PC: 0x%08x, End PC:0x%08x\n",
		 CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread)),
		 CMDQ_REG_GET32(CMDQ_THR_END_ADDR(thread)));

	pThread = &(gCmdqContext.thread[thread]);

	/* we assume error happens BEFORE EOC */
	/* because it wouldn't be error if this interrupt is issue by EOC. */
	/* So we should inc by 1 to locate "current" task */
	cookie += 1;

	/* Set the issued task to error state */
#define CMDQ_TEST_PREFETCH_FOR_MULTIPLE_COMMAND
#ifdef CMDQ_TEST_PREFETCH_FOR_MULTIPLE_COMMAND
	cmdq_core_dump_task_in_thread(thread, true, true, true);
#endif
	/* suspend HW thread first, so that we work in a consistent state */
	/* outer function should acquire spinlock - gCmdqExecLock */
	status = cmdq_core_suspend_HW_thread(thread, __LINE__);
	if (0 > status) {
		/* suspend HW thread failed */
		CMDQ_ERR("IRQ: suspend HW thread failed!");
	}
	CMDQ_ERR("Error IRQ: always suspend thread (%d) to prevent contiuous error IRQ\n", thread);
	if (NULL != pThread->pCurTask[cookie % cmdq_core_max_task_in_thread(thread)]) {
		pTask = pThread->pCurTask[cookie % cmdq_core_max_task_in_thread(thread)];
		pTask->gotIRQ = *pGotIRQ;
		pTask->irqFlag = value;
		cmdq_core_attach_error_task(pTask, thread, NULL);
		cmdq_core_remove_task_from_thread_array_by_cookie(pThread,
								  cookie % cmdq_core_max_task_in_thread(thread),
								  TASK_STATE_ERR_IRQ);
	} else {
		CMDQ_ERR
		    ("IRQ: can not find task in cmdqCoreHandleError, pc:0x%08x, end_pc:0x%08x\n",
		     CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread)),
		     CMDQ_REG_GET32(CMDQ_THR_END_ADDR(thread)));
		if (0 >= pThread->taskCount) {
			cmdq_core_disable_HW_thread(thread);
			CMDQ_ERR("IRQ: there is no task for thread (%d) cmdqCoreHandleError\n",
				 thread);
		}
	}

	/* Set the remain tasks to done state */
	if (pThread->waitCookie <= cookie) {
		count = cookie - pThread->waitCookie + 1;
	} else if ((cookie+1) % CMDQ_MAX_COOKIE_VALUE == pThread->waitCookie) {
		count = 0;
		CMDQ_MSG("IRQ: duplicated cookie: waitCookie:%d, hwCookie:%d",
			pThread->waitCookie, cookie);
	} else {
		/* Counter wrapped */
		count = (CMDQ_MAX_COOKIE_VALUE - pThread->waitCookie + 1) + (cookie + 1);
		CMDQ_ERR("IRQ: counter wrapped: waitCookie:%d, hwCookie:%d, count=%d",
			pThread->waitCookie, cookie, count);
	}

	for (inner = (pThread->waitCookie % cmdq_core_max_task_in_thread(thread)); count > 0; count--, inner++) {
		if (inner >= cmdq_core_max_task_in_thread(thread))
			inner = 0;

		if (NULL != pThread->pCurTask[inner]) {
			pTask = pThread->pCurTask[inner];
			pTask->gotIRQ = (*pGotIRQ);
			pTask->irqFlag = 0;	/* we don't know the exact irq flag. */
			cmdq_core_remove_task_from_thread_array_by_cookie(pThread,
									  inner, TASK_STATE_DONE);
		}
	}

	/* Error cookie will be handled in cmdq_core_handle_wait_task_result_impl API */
	/*pThread->waitCookie = cookie + 1;
	   if (pThread->waitCookie > CMDQ_MAX_COOKIE_VALUE) {
	   pThread->waitCookie -= (CMDQ_MAX_COOKIE_VALUE + 1);
	   } */

	wake_up(&gCmdWaitQueue[thread]);
}

static void cmdqCoreHandleDone(int32_t thread, int32_t value, CMDQ_TIME *pGotIRQ)
{
	ThreadStruct *pThread;
	int32_t cookie;
	int32_t loopResult = 0;

	pThread = &(gCmdqContext.thread[thread]);

	/*  */
	/* Loop execution never gets done; unless */
	/* user loop function returns error */
	/*  */
	if (NULL != pThread->loopCallback) {
		loopResult = pThread->loopCallback(pThread->loopData);

		CMDQ_PROF_MMP(cmdq_mmp_get_event()->loopBeat,
			      MMProfileFlagPulse, thread, loopResult);

		/* HACK: there are some seucre task execue done */
		cmdq_core_handle_secure_paths_exec_done_notify(thread, value, pGotIRQ);

		if (loopResult >= 0) {
#ifdef CMDQ_PROFILE_COMMAND_TRIGGER_LOOP
			/* HACK */
			if (pThread->pCurTask[1])
				cmdq_core_track_task_record(pThread->pCurTask[1], thread);
#endif
			/* Success, contiue execution as if nothing happens */
			CMDQ_REG_SET32(CMDQ_THR_IRQ_STATUS(thread), ~value);
			return;
		}
	}

	if (loopResult < 0) {
		/* The loop CB failed, so stop HW thread now. */
		cmdq_core_disable_HW_thread(thread);

		/* loop CB failed. the EXECUTION count should not be used as cookie, */
		/* since it will increase by each loop iteration. */
		cookie = pThread->waitCookie;
	} else {
		/* task cookie */
		cookie = cmdq_core_thread_exec_counter(thread);
		CMDQ_MSG("Done: thread %d got cookie: %d\n", thread, cookie);
	}

	cmdq_core_handle_done_with_cookie_impl(thread, value, pGotIRQ, cookie);
}

void cmdqCoreHandleIRQ(int32_t thread)
{
	unsigned long flags = 0L;
	CMDQ_TIME gotIRQ;
	int value;
	int enabled;
	int32_t cookie;

	/* note that do_gettimeofday may cause HWT in spin_lock_irqsave (ALPS01496779) */
	gotIRQ = sched_clock();

	/*  */
	/* Normal execution, marks tasks done and remove from thread */
	/* Also, handle "loop CB fail" case */
	/*  */
	spin_lock_irqsave(&gCmdqExecLock, flags);

	/* it is possible for another CPU core */
	/* to run "releaseTask" right before we acquire the spin lock */
	/* and thus reset / disable this HW thread */
	/* so we check both the IRQ flag and the enable bit of this thread */
	value = CMDQ_REG_GET32(CMDQ_THR_IRQ_STATUS(thread));

	if (0 == (value & 0x13)) {
		CMDQ_ERR("IRQ: thread %d got interrupt but IRQ flag is 0x%08x in NWd\n", thread,
			 value);
		spin_unlock_irqrestore(&gCmdqExecLock, flags);
		return;
	}

	if (false == cmdq_get_func()->isSecureThread(thread)) {
		enabled = CMDQ_REG_GET32(CMDQ_THR_ENABLE_TASK(thread));

		if (0 == (enabled & 0x01)) {
			CMDQ_ERR("IRQ: thread %d got interrupt already disabled 0x%08x\n", thread,
				 enabled);
			spin_unlock_irqrestore(&gCmdqExecLock, flags);
			return;
		}
	}

	CMDQ_PROF_START(0, gCmdqThreadLabel[thread]);

	/* Read HW cookie here to print message only */
	cookie = cmdq_core_thread_exec_counter(thread);
	/* Move the reset IRQ before read HW cookie to prevent race condition and save the cost of suspend */
	CMDQ_REG_SET32(CMDQ_THR_IRQ_STATUS(thread), ~value);
	CMDQ_MSG("IRQ: thread %d got interrupt, after reset, and IRQ flag is 0x%08x, cookie: %d\n",
		 thread, value, cookie);

	if (value & 0x12)
		cmdqCoreHandleError(thread, value, &gotIRQ);
	else if (value & 0x01)
		cmdqCoreHandleDone(thread, value, &gotIRQ);

	CMDQ_PROF_END(0, gCmdqThreadLabel[thread]);

	spin_unlock_irqrestore(&gCmdqExecLock, flags);
}

static TaskStruct *cmdq_core_search_task_by_pc(uint32_t threadPC, const ThreadStruct *pThread, int32_t thread)
{
	TaskStruct *pTask = NULL;
	int i = 0;

	for (i = 0; i < cmdq_core_max_task_in_thread(thread); ++i) {
		pTask = pThread->pCurTask[i];
		if (pTask &&
		    threadPC >= pTask->MVABase &&
		    threadPC <= (pTask->MVABase + pTask->commandSize)) {
			break;
		}
	}
	return pTask;
}

/* Implementation of wait task done
 * Return:
 *     wait time of wait_event_timeout() kernel API
 *     . =0, for timeout elapsed,
 *     . >0, remain jiffies if condition passed
 *
 * Note process will go to sleep with state TASK_UNINTERRUPTIBLE until
 * the condition[task done] passed or timeout happened.
 */
static int32_t cmdq_core_wait_task_done_with_timeout_impl(TaskStruct *pTask, int32_t thread)
{
	int32_t waitQ;
	unsigned long flags;
	ThreadStruct *pThread = NULL;
	int32_t retryCount = 0;

	pThread = &(gCmdqContext.thread[thread]);


	/* timeout wait & make sure this task is finished. */
	/* pTask->taskState flag is updated in IRQ handlers like cmdqCoreHandleDone. */
	retryCount = 0;
	waitQ = wait_event_timeout(gCmdWaitQueue[thread],
				   (TASK_STATE_BUSY != pTask->taskState
				    && TASK_STATE_WAITING != pTask->taskState),
				   /* timeout_jiffies); */
				   msecs_to_jiffies(CMDQ_PREDUMP_TIMEOUT_MS));

	/* if SW-timeout, pre-dump hang instructions */
	while (0 == waitQ && retryCount < CMDQ_PREDUMP_RETRY_COUNT) {
		CMDQ_LOG("=============== [CMDQ] SW timeout Pre-dump(%d)===============\n",
			 retryCount);

		++retryCount;

		spin_lock_irqsave(&gCmdqExecLock, flags);
		cmdq_core_dump_status("INFO");
		cmdq_core_dump_pc(pTask, thread, "INFO");

		/* HACK: check trigger thread status */
		cmdq_core_dump_disp_trigger_loop("INFO");
		/* end of HACK */

		spin_unlock_irqrestore(&gCmdqExecLock, flags);

		/* then we wait again */
		waitQ = wait_event_timeout(gCmdWaitQueue[thread],
					   (TASK_STATE_BUSY != pTask->taskState
					    && TASK_STATE_WAITING != pTask->taskState),
					   msecs_to_jiffies(CMDQ_PREDUMP_TIMEOUT_MS));
	}

	return waitQ;
}

static int32_t cmdq_core_handle_wait_task_result_secure_impl(TaskStruct *pTask,
							     int32_t thread, const int32_t waitQ)
{
	int32_t i;
	int32_t status;
	ThreadStruct *pThread = NULL;
	/* error report */
	bool throwAEE = false;
	const char *module = NULL;
	int32_t irqFlag = 0;
	cmdqSecCancelTaskResultStruct result;
	char parsedInstruction[128] = { 0 };

	/* Init default status */
	status = 0;
	pThread = &(gCmdqContext.thread[thread]);
	memset(&result, 0, sizeof(cmdqSecCancelTaskResultStruct));

	/* lock cmdqSecLock */
	cmdq_sec_lock_secure_path();

	do {
		/* check if this task has finished */
#if defined(CMDQ_SECURE_PATH_NORMAL_IRQ) || defined(CMDQ_SECURE_PATH_HW_LOCK)
		if (TASK_STATE_DONE == pTask->taskState)
			break;
#else
		if (TASK_STATE_BUSY != pTask->taskState)
			break;
#endif
		/* Oops, tha tasks is not done. */
		/* We have several possible error scenario: */
		/* 1. task still running (hang / timeout) */
		/* 2. IRQ pending (done or error/timeout IRQ) */
		/* 3. task's SW thread has been signaled (e.g. SIGKILL) */

		/* dump shared cookie */
		CMDQ_VERBOSE
		    ("WAIT: [1]secure path failed, pTask:%p, thread:%d, shared_cookie(%d, %d, %d)\n",
		     pTask, thread,
		     cmdq_core_get_secure_thread_exec_counter(12),
		     cmdq_core_get_secure_thread_exec_counter(13),
		     cmdq_core_get_secure_thread_exec_counter(14));


		/* suppose that task failed, entry secure world to confirm it */
		/* we entry secure world: */
		/* .check if pending IRQ and update cookie value to shared memory */
		/* .confirm if task execute done  */
		/* if not, do error handle */
		/*     .recover M4U & DAPC setting */
		/*     .dump secure HW thread */
		/*     .reset CMDQ secure HW thread */
		cmdq_sec_cancel_error_task_unlocked(pTask, thread, &result);

		/* dump shared cookie */
		CMDQ_VERBOSE
		    ("WAIT: [2]secure path failed, pTask:%p, thread:%d, shared_cookie(%d, %d, %d)\n",
		     pTask, thread,
		     cmdq_core_get_secure_thread_exec_counter(12),
		     cmdq_core_get_secure_thread_exec_counter(13),
		     cmdq_core_get_secure_thread_exec_counter(14));

		/* confirm pending IRQ first */
		cmdq_core_handle_secure_thread_done_impl(thread, 0x01, &pTask->wakedUp);

		/* check if this task has finished after handling pending IRQ */
		if (TASK_STATE_DONE == pTask->taskState)
			break;

		status = -ETIMEDOUT;
		throwAEE = true;
		/* shall we pass the error instru back from secure path?? */
		/* cmdq_core_parse_error(pTask, thread, &module, &irqFlag, &instA, &instB); */
		module = cmdq_get_func()->parseErrorModule(pTask);

		/* module dump */
		cmdq_core_attach_error_task(pTask, thread, NULL);

		/* module reset */
		/* TODO: get needReset infor by secure thread PC */
		cmdq_core_reset_hw_engine(pTask->engineFlag);

		/* remove all tasks in tread since we have reset HW thread in SWd */
		for (i = 0; i < cmdq_core_max_task_in_thread(thread); i++) {
			pTask = pThread->pCurTask[i];
			if (pTask) {
				cmdq_core_remove_task_from_thread_array_by_cookie(pThread, i,
										  TASK_STATE_ERROR);
			}
		}
		pThread->taskCount = 0;
		pThread->waitCookie = pThread->nextCookie;
	} while (0);

	/* unlock cmdqSecLock */
	cmdq_sec_unlock_secure_path();

	/* throw AEE if nessary */
	if (throwAEE) {
		const uint32_t instA = result.errInstr[1];
		const uint32_t instB = result.errInstr[0];
		const uint32_t op = (instA & 0xFF000000) >> 24;

		cmdq_core_interpret_instruction(parsedInstruction, sizeof(parsedInstruction), op,
						instA & (~0xFF000000), instB);

		CMDQ_AEE(module, "%s in CMDQ IRQ:0x%02x, INST:(0x%08x, 0x%08x), OP:%s => %s\n",
			 module, irqFlag, instA, instB, cmdq_core_parse_op(op), parsedInstruction);
	}

	return status;
}

static int32_t cmdq_core_handle_wait_task_result_impl(TaskStruct *pTask, int32_t thread,
						      const int32_t waitQ)
{
	int32_t status;
	int32_t index;
	unsigned long flags;
	ThreadStruct *pThread = NULL;
	const TaskStruct *pNGTask = NULL;
	bool markAsErrorTask = false;
	/* error report */
	bool throwAEE = false;
	const char *module = NULL;
	uint32_t instA = 0, instB = 0;
	int32_t irqFlag = 0;


	/* Init default status */
	status = 0;
	pThread = &(gCmdqContext.thread[thread]);

	/* Note that although we disable IRQ, HW continues to execute */
	/* so it's possible to have pending IRQ */
	spin_lock_irqsave(&gCmdqExecLock, flags);

	do {
		TaskStruct *pNextTask = NULL;
		TaskStruct *pPrevTask = NULL;
		int32_t cookie = 0;
		long threadPC = 0L;

		status = 0;
		throwAEE = false;
		markAsErrorTask = false;

		if (TASK_STATE_DONE == pTask->taskState)
			break;

		CMDQ_ERR("Task state of %p is not TASK_STATE_DONE, %d\n", pTask, pTask->taskState);

		/* Oops, tha tasks is not done. */
		/* We have several possible error scenario: */
		/* 1. task still running (hang / timeout) */
		/* 2. IRQ pending (done or error/timeout IRQ) */
		/* 3. task's SW thread has been signaled (e.g. SIGKILL) */

		/* suspend HW thread first, so that we work in a consistent state */
		status = cmdq_core_suspend_HW_thread(thread, __LINE__);
		if (0 > status)
			throwAEE = true;

		/* The cookie of the task currently being processed */
		cookie = CMDQ_GET_COOKIE_CNT(thread) + 1;
		threadPC = CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread)));

		/* process any pending IRQ */
		/* TODO: provide no spin lock version because we already locked. */
		irqFlag = CMDQ_REG_GET32(CMDQ_THR_IRQ_STATUS(thread));
		if (irqFlag & 0x12)
			cmdqCoreHandleError(thread, irqFlag, &pTask->wakedUp);
		else if (irqFlag & 0x01)
			cmdqCoreHandleDone(thread, irqFlag, &pTask->wakedUp);

		CMDQ_REG_SET32(CMDQ_THR_IRQ_STATUS(thread), ~irqFlag);

		/* check if this task has finished after handling pending IRQ */
		if (TASK_STATE_DONE == pTask->taskState)
			break;

		/* Then decide we are SW timeout or SIGNALed (not an error) */
		if (0 == waitQ) {
			/* SW timeout and no IRQ received */
			markAsErrorTask = true;

			/* if we reach here, we're in errornous state. */
			/* print error log immediately. */
			cmdq_core_attach_error_task(pTask, thread, &pNGTask);
			CMDQ_ERR("SW timeout of task 0x%p on thread %d\n", pTask, thread);
			if (pTask != pNGTask) {
				CMDQ_ERR("   But pc stays in task 0x%p on thread %d\n", pNGTask,
					 thread);
			}
			throwAEE = true;
			cmdq_core_parse_error(pNGTask, thread, &module, &irqFlag, &instA, &instB);
			status = -ETIMEDOUT;

		} else if (0 > waitQ) {
			/* Task be killed. Not an error, but still need removal. */

			markAsErrorTask = false;

			if (-ERESTARTSYS == waitQ) {
				/* Error status print */
				CMDQ_ERR("Task %p KILLED by waitQ = -ERESTARTSYS\n", pTask);
			} else if (-EINTR == waitQ) {
				/* Error status print */
				CMDQ_ERR("Task %p KILLED by waitQ = -EINTR\n", pTask);
			} else {
				/* Error status print */
				CMDQ_ERR("Task %p KILLED by waitQ = %d\n", pTask, waitQ);
			}

			status = waitQ;
		}

		/* reset HW engine immediately if we already got error IRQ. */
		if ((TASK_STATE_ERROR == pTask->taskState) ||
		    (TASK_STATE_ERR_IRQ == pTask->taskState)) {
			cmdq_core_reset_hw_engine(pTask->engineFlag);
			CMDQ_MSG("WAIT: task state is error, reset engine\n");
		} else if (TASK_STATE_BUSY == pTask->taskState) {
			/*  */
			/* if taskState is BUSY, this means we did not reach EOC, did not have error IRQ. */
			/* - remove the task from thread.pCurTask[] */
			/* - and decrease thread.taskCount */
			/* NOTE: after this, the pCurTask will not contain link to pTask anymore. */
			/* and pTask should become TASK_STATE_ERROR */

			/* we find our place in pThread->pCurTask[]. */
			for (index = 0; index < cmdq_core_max_task_in_thread(thread); ++index) {
				if (pThread->pCurTask[index] == pTask) {
					/* update taskCount and pCurTask[] */
					cmdq_core_remove_task_from_thread_array_by_cookie(pThread,
											  index,
											  markAsErrorTask
											  ?
											  TASK_STATE_ERROR
											  :
											  TASK_STATE_KILLED);
					break;
				}
			}
		}

		pNextTask = NULL;
		/* find pTask's jump destination */
		if (0x10000001 == pTask->pCMDEnd[0]) {
			pNextTask = cmdq_core_search_task_by_pc(pTask->pCMDEnd[-1], pThread, thread);
		} else {
			CMDQ_MSG("No next task: LAST instruction : (0x%08x, 0x%08x)\n",
				 pTask->pCMDEnd[0], pTask->pCMDEnd[-1]);
		}

		/* Then, we try remove pTask from the chain of pThread->pCurTask. */
		/* . if HW PC falls in pTask range */
		/* . HW EXEC_CNT += 1 */
		/* . thread.waitCookie += 1 */
		/* . set HW PC to next task head */
		/* . if not, find previous task (whose jump address is pTask->MVABase) */
		/* . check if HW PC points is not at the EOC/JUMP end */
		/* . change jump to fake EOC(no IRQ) */
		/* . insert jump to next task head and increase cmd buffer size */
		/* . if there is no next task, set HW End Address */
		if (pTask->pCMDEnd && threadPC >= pTask->MVABase
		    && threadPC <= (pTask->MVABase + pTask->commandSize)) {
			if (pNextTask) {
				/* cookie already +1 */
				CMDQ_REG_SET32(CMDQ_THR_EXEC_CNT(thread), cookie);
				pThread->waitCookie = cookie + 1;
				CMDQ_REG_SET32(CMDQ_THR_CURR_ADDR(thread),
					       CMDQ_PHYS_TO_AREG(pNextTask->MVABase));
				CMDQ_MSG("WAIT: resume task 0x%p from err\n", pNextTask);
			}
		} else if (TASK_STATE_ERR_IRQ == pTask->taskState) {
			/* Error IRQ might not stay in normal Task range (jump to a strange part) */
			/* We always execute next due to error IRQ must correct task */
			if (pNextTask) {
				/* cookie already +1 */
				CMDQ_REG_SET32(CMDQ_THR_EXEC_CNT(thread), cookie);
				pThread->waitCookie = cookie + 1;
				CMDQ_REG_SET32(CMDQ_THR_CURR_ADDR(thread),
					       CMDQ_PHYS_TO_AREG(pNextTask->MVABase));
				CMDQ_MSG("WAIT: resume task 0x%p from err IRQ\n", pNextTask);
			}
		} else {
			pPrevTask = NULL;
			for (index = 0; index < cmdq_core_max_task_in_thread(thread); ++index) {
				pPrevTask = pThread->pCurTask[index];

				/* find which task JUMP into pTask */
				if (pPrevTask && pPrevTask->pCMDEnd
				    && pPrevTask->pCMDEnd[-1] == pTask->MVABase
				    && pPrevTask->pCMDEnd[0] == 0x10000001) {
					/* Copy Jump instruction */
					pPrevTask->pCMDEnd[-1] = pTask->pCMDEnd[-1];
					pPrevTask->pCMDEnd[0] = pTask->pCMDEnd[0];

					if (pNextTask)
						cmdq_core_reorder_task_array(pThread, thread, index);

					CMDQ_VERBOSE
					    ("WAIT: modify jump to 0x%08x (pPrev:0x%p, pTask:0x%p)\n",
					     pTask->pCMDEnd[-1], pPrevTask, pTask);

					/* Give up fetched command, invoke CMDQ HW to re-fetch command buffer again. */
					cmdq_core_invalidate_hw_fetched_buffer(thread);
					break;
				}
			}
		}
	} while (0);

	if (pThread->taskCount <= 0) {
		cmdq_core_disable_HW_thread(thread);
	} else {
		do {
			/* Reset GCE thread when task state is ERROR or KILL */
			uint32_t backupCurrPC, backupEnd, backupCookieCnt;
			int threadPrio;

			if (TASK_STATE_DONE == pTask->taskState)
				break;

			/* Backup PC, End address, and GCE cookie count before reset GCE thread */
			backupCurrPC =
			    CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread)));
			backupEnd = CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_END_ADDR(thread)));
			backupCookieCnt = CMDQ_GET_COOKIE_CNT(thread);
			CMDQ_LOG
			    ("Reset Backup Thread PC: 0x%08x, End: 0x%08x, CookieCnt: 0x%08x\n",
			     backupCurrPC, backupEnd, backupCookieCnt);
			/* Reset GCE thread */
			if (cmdq_core_reset_HW_thread(thread) < 0) {
				status = -EFAULT;
				break;
			}

			CMDQ_REG_SET32(CMDQ_THR_INST_CYCLES(thread),
				       cmdq_core_get_task_timeout_cycle(pThread));
			/* Set PC & End address */
			CMDQ_REG_SET32(CMDQ_THR_CURR_ADDR(thread), CMDQ_PHYS_TO_AREG(backupCurrPC));
			CMDQ_REG_SET32(CMDQ_THR_END_ADDR(thread), CMDQ_PHYS_TO_AREG(backupEnd));
			/* bit 0-2 for priority level; */
			threadPrio = cmdq_get_func()->priority(pTask->scenario);
			CMDQ_MSG("RESET HW THREAD: set HW thread(%d), qos:%d\n", thread,
				 threadPrio);
			CMDQ_REG_SET32(CMDQ_THR_CFG(thread), threadPrio & 0x7);
			/* For loop thread, do not enable timeout */
			CMDQ_REG_SET32(CMDQ_THR_IRQ_ENABLE(thread),
				       pThread->loopCallback ? 0x011 : 0x013);
			if (pThread->loopCallback) {
				CMDQ_MSG("RESET HW THREAD: HW thread(%d) in loop func 0x%p\n",
					 thread, pThread->loopCallback);
			}
			/* Set GCE cookie count */
			CMDQ_REG_SET32(CMDQ_THR_EXEC_CNT(thread), backupCookieCnt);
			/* Enable HW thread */
			CMDQ_REG_SET32(CMDQ_THR_ENABLE_TASK(thread), 0x01);
		} while (0);
		cmdq_core_resume_HW_thread(thread);
	}

	spin_unlock_irqrestore(&gCmdqExecLock, flags);

	if (throwAEE) {
		const uint32_t op = (instA & 0xFF000000) >> 24;

		switch (op) {
		case CMDQ_CODE_WFE:
			CMDQ_AEE(module,
				 "%s in CMDQ IRQ:0x%02x, INST:(0x%08x, 0x%08x), OP:WAIT EVENT:%s\n",
				 module, irqFlag, instA, instB,
				 cmdq_core_get_event_name(instA & (~0xFF000000)));
			break;
		default:
			CMDQ_AEE(module, "%s in CMDQ IRQ:0x%02x, INST:(0x%08x, 0x%08x), OP:%s\n",
				 module, irqFlag, instA, instB, cmdq_core_parse_op(op));
			break;
		}
	}

	return status;
}

static int32_t cmdq_core_wait_task_done(TaskStruct *pTask, long timeout_jiffies)
{
	int32_t waitQ;
	int32_t status;
	uint32_t thread;
	ThreadStruct *pThread = NULL;

	status = 0;		/* Default status */

	thread = pTask->thread;
	if (CMDQ_INVALID_THREAD == thread) {
		CMDQ_PROF_MMP(cmdq_mmp_get_event()->wait_thread,
			      MMProfileFlagPulse, ((unsigned long)pTask), -1);

		CMDQ_PROF_START(current->pid, "wait_for_thread");

		CMDQ_LOG("pid:%d task:0x%p wait for valid thread first\n", current->pid, pTask);

		/* wait for acquire thread (this is done by cmdq_core_consume_waiting_list); */
		waitQ = wait_event_timeout(gCmdqThreadDispatchQueue,
					   (CMDQ_INVALID_THREAD != pTask->thread),
					   msecs_to_jiffies(CMDQ_ACQUIRE_THREAD_TIMEOUT_MS));

		CMDQ_PROF_END(current->pid, "wait_for_thread");
		if (0 == waitQ || CMDQ_INVALID_THREAD == pTask->thread) {
			mutex_lock(&gCmdqTaskMutex);
			/* it's possible that the task was just consumed now. */
			/* so check again. */
			if (CMDQ_INVALID_THREAD == pTask->thread) {
				/* task may already released, or starved to death */
				CMDQ_ERR("task 0x%p timeout with invalid thread\n", pTask);
				cmdq_core_dump_task(pTask);
				cmdq_core_dump_task_with_engine_flag(pTask->engineFlag);
				/* remove from waiting list, */
				/* so that it won't be consumed in the future */
				list_del_init(&(pTask->listEntry));

				mutex_unlock(&gCmdqTaskMutex);
				return -EINVAL;
			}

			/* valid thread, so we keep going */
			mutex_unlock(&gCmdqTaskMutex);
		}
	}

	/* double confim if it get a valid thread */
	thread = pTask->thread;
	if ((0 > thread) || (CMDQ_MAX_THREAD_COUNT <= thread)) {
		CMDQ_ERR("invalid thread %d in %s\n", thread, __func__);
		return -EINVAL;
	}

	pThread = &(gCmdqContext.thread[thread]);

	CMDQ_PROF_MMP(cmdq_mmp_get_event()->wait_task,
		      MMProfileFlagPulse, ((unsigned long)pTask), thread);

	CMDQ_PROF_START(current->pid, "wait_for_task_done");

	/* start to wait */
	pTask->beginWait = sched_clock();
	CMDQ_MSG("-->WAIT: task 0x%p on thread %d timeout: %d(ms) begin\n", pTask, thread,
		 jiffies_to_msecs(timeout_jiffies));
	waitQ = cmdq_core_wait_task_done_with_timeout_impl(pTask, thread);

	/* wake up! */
	/* so the maximum total waiting time would be */
	/* CMDQ_PREDUMP_TIMEOUT_MS * CMDQ_PREDUMP_RETRY_COUNT */
	pTask->wakedUp = sched_clock();
	CMDQ_MSG("WAIT: task 0x%p waitq=%d state=%d\n", pTask, waitQ, pTask->taskState);
	CMDQ_PROF_END(current->pid, "wait_for_task_done");

	status = (false == pTask->secData.isSecure) ?
	    cmdq_core_handle_wait_task_result_impl(pTask, thread, waitQ) :
	    cmdq_core_handle_wait_task_result_secure_impl(pTask, thread, waitQ);

	CMDQ_MSG("<--WAIT: task 0x%p on thread %d end\n", pTask, thread);

	return status;
}

static int32_t cmdq_core_exec_task_async_secure_impl(TaskStruct *pTask, int32_t thread)
{
	int32_t status;
	ThreadStruct *pThread;
	int32_t cookie;
	char longMsg[CMDQ_LONGSTRING_MAX];
	uint32_t msgOffset;
	int32_t msgMAXSize;

	cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
	cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
			   "-->EXEC: task 0x%p on thread %d begin, VABase: 0x%p,",
			   pTask, thread, pTask->pVABase);
	cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
			   " MVABase: %pa, Size: %d, bufferSize: %d, scenario:%d, flag:0x%llx\n",
			   &(pTask->MVABase), pTask->commandSize, pTask->bufferSize,
			   pTask->scenario, pTask->engineFlag);
	if (msgOffset > 0) {
		/* print message */
		CMDQ_MSG("%s", longMsg);
	}

	status = 0;
	pThread = &(gCmdqContext.thread[thread]);

	cmdq_sec_lock_secure_path();

	do {
		/* setup whole patah */
		status = cmdq_sec_allocate_path_resource_unlocked(true);
		if (0 > status)
			break;

		/* update task's thread info */
		pTask->thread = thread;
		pTask->irqFlag = 0;
		pTask->taskState = TASK_STATE_BUSY;

		/* insert task to pThread's task lsit, and */
		/* delay HW config when entry SWd */
		if (pThread->taskCount <= 0) {
			cookie = 1;
			cmdq_core_insert_task_from_thread_array_by_cookie(pTask, pThread, cookie,
									  true);
		} else {
			/* append directly */
			cookie = pThread->nextCookie;
			cmdq_core_insert_task_from_thread_array_by_cookie(pTask, pThread, cookie,
									  false);
		}

		pTask->trigger = sched_clock();

		/* execute */
		status = cmdq_sec_exec_task_async_unlocked(pTask, thread);

		if (0 > status) {
			/* config failed case, dump for more detail */
			cmdq_core_attach_error_task(pTask, thread, NULL);
		}
	} while (0);

	cmdq_sec_unlock_secure_path();

	return status;
}

static inline int32_t cmdq_core_exec_find_task_slot(TaskStruct **pLast, TaskStruct *pTask,
						    int32_t thread, int32_t loop)
{
	int32_t status = 0;
	ThreadStruct *pThread;
	TaskStruct *pPrev;
	int32_t index;
	int32_t prev;
	int32_t cookie;

	pThread = &(gCmdqContext.thread[thread]);
	cookie = pThread->nextCookie;

	/* Traverse forward to adjust tasks' order according to their priorities */
	for (prev = (cookie % cmdq_core_max_task_in_thread(thread)); loop > 0; loop--) {
		index = prev;
		if (index < 0)
			index = cmdq_core_max_task_in_thread(thread) - 1;

		prev = index - 1;
		if (prev < 0)
			prev = cmdq_core_max_task_in_thread(thread) - 1;

		pPrev = pThread->pCurTask[prev];

		/* Maybe the job is killed, search a new one */
		while ((NULL == pPrev) && (loop > 1)) {
			CMDQ_LOG("pPrev is NULL, prev:%d, loop:%d, index:%d\n", prev, loop, index);
			prev = prev - 1;
			if (prev < 0)
				prev = cmdq_core_max_task_in_thread(thread) - 1;

			pPrev = pThread->pCurTask[prev];
			loop--;
		}

		if (NULL == pPrev) {
			cmdq_core_attach_error_task(pTask, thread, NULL);
			CMDQ_ERR("Invalid task state for reorder %d %d\n", index, loop);
			status = -EFAULT;
			break;
		}

		if (loop <= 1) {
			CMDQ_MSG("Set current(%d) order for the new task, line:%d\n", index, __LINE__);

			CMDQ_MSG("Original PC %pa, end %pa\n", &pPrev->MVABase,
				 &pPrev->MVABase + pPrev->commandSize);
			CMDQ_MSG("Original instruction 0x%08x, 0x%08x\n", pPrev->pCMDEnd[0],
				 pPrev->pCMDEnd[-1]);

			pThread->pCurTask[index] = pTask;
			/* Jump: Absolute */
			pPrev->pCMDEnd[0] = 0x10000001;
			/* Jump to here */
			pPrev->pCMDEnd[-1] = pTask->MVABase;
			CMDQ_VERBOSE("EXEC: modify jump to %pa, line:%d\n", &(pTask->MVABase), __LINE__);
#ifndef CMDQ_APPEND_WITHOUT_SUSPEND
			/* re-fetch command buffer again. */
			cmdq_core_invalidate_hw_fetched_buffer(thread);
#endif
			break;
		}

		if (pPrev->priority < pTask->priority) {
			CMDQ_LOG("Switch prev(%d, 0x%p) and curr(%d, 0x%p) order\n",
				 prev, pPrev, index, pTask);

			pThread->pCurTask[index] = pPrev;
			pPrev->pCMDEnd[0] = pTask->pCMDEnd[0];
			pPrev->pCMDEnd[-1] = pTask->pCMDEnd[-1];

			/* Boot priority for the task */
			pPrev->priority += CMDQ_MIN_AGE_VALUE;
			pPrev->reorder++;

			pThread->pCurTask[prev] = pTask;
			/* Jump: Absolute */
			pTask->pCMDEnd[0] = 0x10000001;
			/* Jump to here */
			pTask->pCMDEnd[-1] = pPrev->MVABase;

			CMDQ_VERBOSE("EXEC: modify jump to %pa, line:%d\n", &(pPrev->MVABase), __LINE__);
#ifndef CMDQ_APPEND_WITHOUT_SUSPEND
			/* re-fetch command buffer again. */
			cmdq_core_invalidate_hw_fetched_buffer(thread);
#endif
			if (*pLast == pTask) {
				CMDQ_LOG("update pLast from 0x%p to 0x%p\n", pTask, pPrev);
				*pLast = pPrev;
			}
		} else {
			CMDQ_MSG("Set current(%d) order for the new task, line:%d\n", index, __LINE__);

			CMDQ_MSG("Original PC %pa, end %pa\n", &pPrev->MVABase,
				 &pPrev->MVABase + pPrev->commandSize);
			CMDQ_MSG("Original instruction 0x%08x, 0x%08x\n", pPrev->pCMDEnd[0], pPrev->pCMDEnd[-1]);

			pThread->pCurTask[index] = pTask;
			/* Jump: Absolute */
			pPrev->pCMDEnd[0] = 0x10000001;
			/* Jump to here */
			pPrev->pCMDEnd[-1] = pTask->MVABase;

			CMDQ_VERBOSE("EXEC: modify jump to %pa, line:%d\n", &(pTask->MVABase), __LINE__);
#ifndef CMDQ_APPEND_WITHOUT_SUSPEND
			/* re-fetch command buffer again. */
			cmdq_core_invalidate_hw_fetched_buffer(thread);
#endif
			break;
		}
	}

	CMDQ_MSG("Reorder %d tasks for performance end, pLast:0x%p\n", loop, *pLast);

	return status;
}

static int32_t cmdq_core_exec_task_async_impl(TaskStruct *pTask, int32_t thread)
{
	int32_t status;
	ThreadStruct *pThread;
	TaskStruct *pLast;
	unsigned long flags;
	int32_t loop;
	int32_t minimum;
	int32_t cookie;
	int threadPrio = 0;
	uint32_t EndAddr;
	char longMsg[CMDQ_LONGSTRING_MAX];
	uint32_t msgOffset;
	int32_t msgMAXSize;

	/* for no suspend thread, we shift END before JUMP */
	int32_t shiftEnd = 0;

	cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
	cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
			   "-->EXEC: task 0x%p on thread %d begin, VABase: 0x%p, MVABase: %pa,",
			   pTask, thread, pTask->pVABase, &(pTask->MVABase));
	cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
			   " Size: %d, bufferSize: %d, scenario:%d, flag:0x%llx\n",
			   pTask->commandSize, pTask->bufferSize, pTask->scenario,
			   pTask->engineFlag);
	if (msgOffset > 0) {
		/* print message */
		CMDQ_MSG("%s", longMsg);
	}

	status = 0;

	pThread = &(gCmdqContext.thread[thread]);

	pTask->trigger = sched_clock();

	spin_lock_irqsave(&gCmdqExecLock, flags);

	/* update task's thread info */
	pTask->thread = thread;
	pTask->irqFlag = 0;
	pTask->taskState = TASK_STATE_BUSY;

#ifdef CMDQ_APPEND_WITHOUT_SUSPEND
	/* for loop command, we do not shift END before JUMP */
	if (pThread->loopCallback)
		shiftEnd = 0;
	else
		shiftEnd = CMDQ_INST_SIZE;
#endif

	if (pThread->taskCount <= 0) {
		CMDQ_MSG("EXEC: new HW thread(%d)\n", thread);

		if (cmdq_core_reset_HW_thread(thread) < 0) {
			spin_unlock_irqrestore(&gCmdqExecLock, flags);
			return -EFAULT;
		}

		CMDQ_REG_SET32(CMDQ_THR_INST_CYCLES(thread),
			       cmdq_core_get_task_timeout_cycle(pThread));
		threadPrio = cmdq_get_func()->priority(pTask->scenario);
		CMDQ_MSG("EXEC: set HW thread(%d) pc:%pa, qos:%d\n",
			 thread, &pTask->MVABase, threadPrio);
		CMDQ_REG_SET32(CMDQ_THR_CURR_ADDR(thread), CMDQ_PHYS_TO_AREG(pTask->MVABase));
		EndAddr = CMDQ_PHYS_TO_AREG(pTask->MVABase + pTask->commandSize - shiftEnd);
		CMDQ_REG_SET32(CMDQ_THR_END_ADDR(thread), EndAddr);
		CMDQ_REG_SET32(CMDQ_THR_CFG(thread), threadPrio & 0x7);	/* bit 0-2 for priority level; */

		/* For loop thread, do not enable timeout */
		CMDQ_REG_SET32(CMDQ_THR_IRQ_ENABLE(thread), pThread->loopCallback ? 0x011 : 0x013);

		if (pThread->loopCallback) {
			CMDQ_MSG("EXEC: HW thread(%d) in loop func 0x%p\n", thread,
				 pThread->loopCallback);
		}

		/* attach task to thread */
		minimum = CMDQ_GET_COOKIE_CNT(thread);
		cmdq_core_insert_task_from_thread_array_by_cookie(pTask, pThread, (minimum + 1),
								  true);

		/* verify that we don't corrupt EOC + JUMP pattern */
		cmdq_core_verfiy_command_end(pTask);

		/* enable HW thread */
		CMDQ_MSG("enable HW thread(%d)\n", thread);

		CMDQ_PROF_MMP(cmdq_mmp_get_event()->thread_en,
			      MMProfileFlagPulse, thread, pThread->nextCookie - 1);

		CMDQ_REG_SET32(CMDQ_THR_ENABLE_TASK(thread), 0x01);
#ifdef CMDQ_MDP_MET_STATUS
		/* MET MMSYS : Primary Trigger start */
		if (met_mmsys_event_gce_thread_begin)
			met_mmsys_event_gce_thread_begin(thread, (uintptr_t) pTask, pTask->engineFlag,
				(void *)pTask->pVABase, pTask->commandSize);
#endif
	} else {
		CMDQ_MSG("EXEC: reuse HW thread(%d), taskCount:%d\n", thread, pThread->taskCount);

#ifdef CMDQ_APPEND_WITHOUT_SUSPEND
		cmdqCoreClearEvent(CMDQ_SYNC_TOKEN_APPEND_THR(thread));
#else
		status = cmdq_core_suspend_HW_thread(thread, __LINE__);
		if (status < 0) {
			spin_unlock_irqrestore(&gCmdqExecLock, flags);
			return status;
		}

		CMDQ_REG_SET32(CMDQ_THR_INST_CYCLES(thread),
			       cmdq_core_get_task_timeout_cycle(pThread));
#endif

		cookie = pThread->nextCookie;

		/* Boundary case tested: EOC have been executed, but JUMP is not executed */
		/* Thread PC: 0x9edc0dd8, End: 0x9edc0de0, Curr Cookie: 1, Next Cookie: 2 */

		/* PC = END - 8, EOC is executed */
		/* PC = END - 0, All CMDs are executed */
		if ((CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread))) ==
		     (CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_END_ADDR(thread))) - 8)) ||
		    (CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread))) ==
		     (CMDQ_AREG_TO_PHYS(CMDQ_REG_GET32(CMDQ_THR_END_ADDR(thread))) - 0))) {
			cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
			cmdqCoreLongString(true, longMsg, &msgOffset, &msgMAXSize,
					   "EXEC: Set HW thread(%d) pc from 0x%08x(end:0x%08x) to %pa,",
					   thread,
					   CMDQ_REG_GET32(CMDQ_THR_CURR_ADDR(thread)),
					   CMDQ_REG_GET32(CMDQ_THR_END_ADDR(thread)),
					   &pTask->MVABase);
			cmdqCoreLongString(true, longMsg, &msgOffset, &msgMAXSize,
					   " oriNextCookie:%d, oriTaskCount:%d\n",
					   cookie, pThread->taskCount);
			if (msgOffset > 0) {
				/* print message */
				CMDQ_LOG("%s", longMsg);
			}

			/* set to pTask directly */
			CMDQ_REG_SET32(CMDQ_THR_CURR_ADDR(thread),
				       CMDQ_PHYS_TO_AREG(pTask->MVABase));
			EndAddr = CMDQ_PHYS_TO_AREG(pTask->MVABase + pTask->commandSize - shiftEnd);
			CMDQ_REG_SET32(CMDQ_THR_END_ADDR(thread), EndAddr);

			pThread->pCurTask[cookie % cmdq_core_max_task_in_thread(thread)] = pTask;
			pThread->taskCount++;
			pThread->allowDispatching = 1;
		} else {
			CMDQ_MSG("Connect new task's MVA to previous one\n");

			/* Current task that shuld be processed */
			minimum = CMDQ_GET_COOKIE_CNT(thread) + 1;
			if (minimum > CMDQ_MAX_COOKIE_VALUE)
				minimum = 0;

			/* Calculate loop count to adjust the tasks' order */
			if (minimum <= cookie) {
				loop = cookie - minimum;
			} else {
				/* Counter wrapped */
				loop = (CMDQ_MAX_COOKIE_VALUE - minimum + 1) + cookie;
			}

			CMDQ_MSG("Reorder task in range [%d, %d] with count %d\n", minimum, cookie, loop);

			/* ALPS01672377 */
			/* .note pThread->taskCount-- when remove task from pThread in ISR */
			/* .In mutlple SW clients or async case, */
			/*  clients may continue submit tasks with overlap engines */
			/*  it's okey 0 = abs(pThread->nextCookie, THR_CNT+1) when... */
			/*      .submit task_1, trigger GCE */
			/*      .submit task_2: */
			/*          .GCE exec task1 done */
			/*          .task_2 lock execLock when insert task to thread */
			/*          .task 1's IRQ */

			if (loop < 0) {
				cmdq_core_dump_task_in_thread(thread, true, true, false);

				cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
				cmdqCoreLongString(true, longMsg, &msgOffset, &msgMAXSize,
						   "Invalid task count(%d) in thread %d for reorder,",
						   loop, thread);
				cmdqCoreLongString(true, longMsg, &msgOffset, &msgMAXSize,
						   " nextCookie:%d, nextCookieHW:%d, pTask:%p\n",
						   pThread->nextCookie, minimum, pTask);
				if (msgOffset > 0) {
					/* print message */
					CMDQ_AEE("CMDQ", "%s", longMsg);
				}
#ifdef CMDQ_APPEND_WITHOUT_SUSPEND
				cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_APPEND_THR(thread));
#endif
				spin_unlock_irqrestore(&gCmdqExecLock, flags);
				return -EFAULT;
			}

			if (loop > cmdq_core_max_task_in_thread(thread)) {
				CMDQ_LOG("loop = %d, execeed max task in thread", loop);
				loop = loop % cmdq_core_max_task_in_thread(thread);
			}
			CMDQ_MSG("Reorder %d tasks for performance begin\n", loop);
			/* By default, pTask is the last task, and insert [cookie % CMDQ_MAX_TASK_IN_THREAD] */
			pLast = pTask;

			status = cmdq_core_exec_find_task_slot(&pLast, pTask, thread, loop);
			if (status < 0) {
#ifdef CMDQ_APPEND_WITHOUT_SUSPEND
				cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_APPEND_THR(thread));
#endif
				spin_unlock_irqrestore(&gCmdqExecLock, flags);
				CMDQ_AEE("CMDQ", "Invalid task state for reorder.\n");
				return status;
			}

			/* We must set memory barrier here to make sure we modify jump before enable thread */
			smp_mb();

			EndAddr = CMDQ_PHYS_TO_AREG(pLast->MVABase + pLast->commandSize - shiftEnd);
			CMDQ_REG_SET32(CMDQ_THR_END_ADDR(thread), EndAddr);
			pThread->taskCount++;
			pThread->allowDispatching = 1;
		}

		pThread->nextCookie += 1;
		if (pThread->nextCookie > CMDQ_MAX_COOKIE_VALUE) {
			/* Reach the maximum cookie */
			pThread->nextCookie = 0;
		}

		/* verify that we don't corrupt EOC + JUMP pattern */
		cmdq_core_verfiy_command_end(pTask);

		/* resume HW thread */
		CMDQ_PROF_MMP(cmdq_mmp_get_event()->thread_en,
			      MMProfileFlagPulse, thread, pThread->nextCookie - 1);
#ifdef CMDQ_APPEND_WITHOUT_SUSPEND
		cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_APPEND_THR(thread));
#else
		cmdq_core_resume_HW_thread(thread);
#endif
	}

	spin_unlock_irqrestore(&gCmdqExecLock, flags);

	CMDQ_MSG("<--EXEC: status: %d\n", status);

	return status;
}

#ifdef CMDQ_PROFILE
static const char *gCmdqThreadLabel[CMDQ_MAX_THREAD_COUNT] = {
	"CMDQ_IRQ_THR_0",
	"CMDQ_IRQ_THR_1",
	"CMDQ_IRQ_THR_2",
	"CMDQ_IRQ_THR_3",
	"CMDQ_IRQ_THR_4",
	"CMDQ_IRQ_THR_5",
	"CMDQ_IRQ_THR_6",
	"CMDQ_IRQ_THR_7",
	"CMDQ_IRQ_THR_8",
	"CMDQ_IRQ_THR_9",
	"CMDQ_IRQ_THR_10",
	"CMDQ_IRQ_THR_11",
	"CMDQ_IRQ_THR_12",
	"CMDQ_IRQ_THR_13",
	"CMDQ_IRQ_THR_14",
	"CMDQ_IRQ_THR_15",
};
#endif

int32_t cmdqCoreSuspend(void)
{
	unsigned long flags = 0L;
	EngineStruct *pEngine = NULL;
	uint32_t execThreads = 0x0;
	int refCount = 0;
	bool killTasks = false;
	struct TaskStruct *pTask = NULL;
	struct list_head *p = NULL;
	int i = 0;

	/* destroy secure path notify thread */
	cmdq_core_stop_secure_path_notify_thread();

	pEngine = gCmdqContext.engine;

	execThreads = CMDQ_REG_GET32(CMDQ_CURR_LOADED_THR);
	refCount = atomic_read(&gCmdqThreadUsage);

	if (0 > cmdq_get_func()->moduleEntrySuspend(pEngine)) {
		CMDQ_ERR("[SUSPEND] MDP running, kill tasks. threads:0x%08x, ref:%d\n", execThreads,
			 refCount);
		killTasks = true;
	} else if ((refCount > 0) || (0x80000000 & execThreads)) {
		CMDQ_ERR("[SUSPEND] other running, kill tasks. threads:0x%08x, ref:%d\n",
			 execThreads, refCount);
		killTasks = true;
	}

	/*  */
	/* We need to ensure the system is ready to suspend, */
	/* so kill all running CMDQ tasks */
	/* and release HW engines. */
	/*  */
	if (killTasks) {
		/* print active tasks */
		CMDQ_ERR("[SUSPEND] active tasks during suspend:\n");
		list_for_each(p, &gCmdqContext.taskActiveList) {
			pTask = list_entry(p, struct TaskStruct, listEntry);
			cmdq_core_dump_task(pTask);
		}

		/* remove all active task from thread */
		CMDQ_ERR("[SUSPEND] remove all active tasks\n");
		list_for_each(p, &gCmdqContext.taskActiveList) {
			pTask = list_entry(p, struct TaskStruct, listEntry);

			if (pTask->thread != CMDQ_INVALID_THREAD) {
				spin_lock_irqsave(&gCmdqExecLock, flags);

				cmdq_core_force_remove_task_from_thread(pTask, pTask->thread);
				pTask->taskState = TASK_STATE_KILLED;

				spin_unlock_irqrestore(&gCmdqExecLock, flags);

				/* release all thread and mark all active tasks as "KILLED" */
				/* (so that thread won't release again) */
				CMDQ_ERR("[SUSPEND] release all threads and HW clocks\n");
				cmdq_core_release_thread(pTask);
			}
		}

		/* TODO: skip secure path thread... */
		/* disable all HW thread */
		CMDQ_ERR("[SUSPEND] disable all HW threads\n");
		for (i = 0; i < CMDQ_MAX_THREAD_COUNT; ++i)
			cmdq_core_disable_HW_thread(i);

		/* reset all threadStruct */
		memset(&gCmdqContext.thread[0], 0, sizeof(gCmdqContext.thread));
		cmdq_core_reset_thread_struct();

		/* reset all engineStruct */
		memset(&gCmdqContext.engine[0], 0, sizeof(gCmdqContext.engine));
		cmdq_core_reset_engine_struct();
	}

	spin_lock_irqsave(&gCmdqThreadLock, flags);
	gCmdqSuspended = true;
	spin_unlock_irqrestore(&gCmdqThreadLock, flags);

	/* ALWAYS allow suspend */
	return 0;
}


int32_t cmdq_core_reume_impl(const char *tag)
{
	unsigned long flags = 0L;
	int refCount = 0;

	spin_lock_irqsave(&gCmdqThreadLock, flags);

	refCount = atomic_read(&gCmdqThreadUsage);
	CMDQ_MSG("[%s] resume, refCount:%d\n", tag, refCount);

	gCmdqSuspended = false;

	/* during suspending, there may be queued tasks. */
	/* we should process them if any. */
	if (!work_pending(&gCmdqContext.taskConsumeWaitQueueItem)) {
		CMDQ_MSG("[%s] there are undone task, process them\n", tag);
		/* we use system global work queue (kernel thread kworker/n) */
		CMDQ_PROF_MMP(cmdq_mmp_get_event()->consume_add, MMProfileFlagPulse, 0, 0);
		queue_work(gCmdqContext.taskConsumeWQ, &gCmdqContext.taskConsumeWaitQueueItem);
	}

	spin_unlock_irqrestore(&gCmdqThreadLock, flags);
	return 0;
}

int32_t cmdqCoreResume(void)
{
	CMDQ_VERBOSE("[RESUME] do nothing\n");
	/* do nothing */
	return 0;
}

int32_t cmdqCoreResumedNotifier(void)
{
	/* TEE project limitation:
	 * .t-base daemon process is available after process-unfreeze
	 * .need t-base daemon for communication to secure world
	 * .M4U port security setting backup/resore needs to entry secure world
	 * .M4U port security setting is access normal PA
	 *
	 * Delay resume timing until process-unfreeze done in order to
	 * ensure M4U driver had restore M4U port security setting
	 */
	CMDQ_VERBOSE("[RESUME] cmdqCoreResumedNotifier\n");
	return cmdq_core_reume_impl("RESUME_NOTIFIER");
}

static int32_t cmdq_core_exec_task_async_with_retry(TaskStruct *pTask, int32_t thread)
{
	int32_t retry = 0;
	int32_t status = 0;
	ThreadStruct *pThread;

	pThread = &(gCmdqContext.thread[thread]);
	if (pThread->loopCallback) {
		/* Do not insert Wait for loop due to loop no need append */
		CMDQ_MSG("Ignore insert wait for loop task\n");
	} else {
		if (true == pTask->secData.isSecure)
			status = cmdq_core_insert_secure_handle_instr(pTask, thread);
#ifdef CMDQ_APPEND_WITHOUT_SUSPEND
		/* Shift JUMP and EOC */
		pTask->pCMDEnd += 2;
		pTask->pCMDEnd[0] = pTask->pCMDEnd[-2];
		pTask->pCMDEnd[-1] = pTask->pCMDEnd[-3];
		pTask->pCMDEnd[-2] = pTask->pCMDEnd[-4];
		pTask->pCMDEnd[-3] = pTask->pCMDEnd[-5];
		/* Update original JUMP to wait event */
		/* Sync: Op and sync event */
		pTask->pCMDEnd[-4] = (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_TOKEN_APPEND_THR(thread);
		/* Sync: Wait and no clear */
		pTask->pCMDEnd[-5] = ((0 << 31) | (1 << 15) | 1);
		pTask->commandSize += CMDQ_INST_SIZE;
		/* make sure instructions are synced in DRAM */
		smp_mb();
		CMDQ_MSG
		    ("After insert wait: pTask 0x%p last 3 instr (%08x:%08x, %08x:%08x, %08x:%08x)\n",
		     pTask,
		     pTask->pCMDEnd[-5], pTask->pCMDEnd[-4], pTask->pCMDEnd[-3],
		     pTask->pCMDEnd[-2], pTask->pCMDEnd[-1], pTask->pCMDEnd[0]);
#endif
	}

	if (status < 0)
		return status;

	do {
		if (false == cmdq_core_verfiy_command_end(pTask)) {
			status = -EFAULT;
			break;
		}

		/* Save command buffer dump */
		if (0 == retry)
			cmdq_core_save_command_buffer_dump(pTask);

		status = (false == pTask->secData.isSecure) ?
		    (cmdq_core_exec_task_async_impl(pTask, thread)) :
		    (cmdq_core_exec_task_async_secure_impl(pTask, thread));

		if (status >= 0)
			break;

		if ((TASK_STATE_KILLED == pTask->taskState) ||
		    (TASK_STATE_ERROR == pTask->taskState) ||
		    (TASK_STATE_ERR_IRQ == pTask->taskState)) {
			CMDQ_ERR("cmdq_core_exec_task_async_impl fail\n");
			status = -EFAULT;
			break;
		}

		++retry;
	} while (retry < CMDQ_MAX_RETRY_COUNT);

	return status;
}

static int32_t cmdq_core_consume_waiting_list(struct work_struct *_ignore)
{
	struct list_head *p, *n = NULL;
	struct TaskStruct *pTask = NULL;
	struct ThreadStruct *pThread = NULL;
	int32_t thread = CMDQ_INVALID_THREAD;
	int32_t status = 0;
	bool threadAcquired = false;
	CMDQ_HW_THREAD_PRIORITY_ENUM thread_prio = CMDQ_THR_PRIO_NORMAL;
	CMDQ_TIME consumeTime;
	int32_t waitingTimeMS;
	bool needLog = false;
	bool dumpTriggerLoop = false;

	/* when we're suspending, do not execute any tasks. delay & hold them. */
	if (gCmdqSuspended)
		return status;

	CMDQ_PROF_START(current->pid, __func__);
	CMDQ_PROF_MMP(cmdq_mmp_get_event()->consume_done, MMProfileFlagStart, current->pid, 0);
	consumeTime = sched_clock();

	mutex_lock(&gCmdqTaskMutex);

	threadAcquired = false;

	/* scan and remove (if executed) waiting tasks */
	list_for_each_safe(p, n, &gCmdqContext.taskWaitList) {
		pTask = list_entry(p, struct TaskStruct, listEntry);

		thread_prio = cmdq_get_func()->priority(pTask->scenario);

		CMDQ_MSG("-->THREAD: try acquire thread for task: 0x%p, thread_prio: %d\n",
				   pTask, thread_prio);
		CMDQ_MSG("-->THREAD: task_prio: %d, flag: 0x%llx, scenario:%d begin\n",
				   pTask->priority, pTask->engineFlag, pTask->scenario);

		CMDQ_GET_TIME_IN_MS(pTask->submit, consumeTime, waitingTimeMS);
		needLog = waitingTimeMS >= CMDQ_PREDUMP_TIMEOUT_MS;

		/* Allocate hw thread */
		thread = cmdq_core_acquire_thread(pTask->engineFlag,
						  thread_prio, pTask->scenario, needLog,
						  pTask->secData.isSecure);

		if (CMDQ_INVALID_THREAD == thread) {
			/* have to wait, remain in wait list */
			CMDQ_MSG("<--THREAD: acquire thread fail, need to wait\n");
			if (needLog) {
				/* task wait too long */
				CMDQ_ERR("acquire thread pre-dump for task: 0x%p\n",
						   pTask);
				CMDQ_ERR(" thread_prio: %d, task_prio: %d, flag: 0x%llx\n",
						   thread_prio, pTask->priority, pTask->engineFlag);

				dumpTriggerLoop =
				    (CMDQ_SCENARIO_PRIMARY_DISP == pTask->scenario) ?
				    (true) : (dumpTriggerLoop);
			}
			continue;
		}

		pThread = &gCmdqContext.thread[thread];

		/* some task is ready to run */
		threadAcquired = true;

		/* Assign loop function if the thread should be a loop thread */
		pThread->loopCallback = pTask->loopCallback;
		pThread->loopData = pTask->loopData;

		/* Start execution, */
		/* remove from wait list and put into active list */
		list_del_init(&(pTask->listEntry));
		list_add_tail(&(pTask->listEntry), &gCmdqContext.taskActiveList);

		CMDQ_MSG("<--THREAD: acquire thread w/flag: 0x%llx on thread(%d): 0x%p end\n",
			 pTask->engineFlag, thread, pThread);

		/* Run task on thread */
		status = cmdq_core_exec_task_async_with_retry(pTask, thread);
		if (status < 0) {
			CMDQ_ERR
			    ("<--THREAD: cmdq_core_exec_task_async_with_retry fail, release task 0x%p\n",
			     pTask);
			cmdq_core_track_task_record(pTask, thread);
			cmdq_core_release_thread(pTask);
			cmdq_core_release_task_unlocked(pTask);
			pTask = NULL;
		}
	}

	if (dumpTriggerLoop) {
		/* HACK: observe trigger loop status when acquire config thread failed. */
		int32_t dumpThread = cmdq_get_func()->dispThread(CMDQ_SCENARIO_PRIMARY_DISP);

		cmdq_core_dump_disp_trigger_loop("ACQUIRE");
		cmdq_core_dump_thread_pc(dumpThread);
	}

	if (threadAcquired) {
		/* notify some task's SW thread to change their waiting state. */
		/* (if they already called cmdqCoreWaitResultAndReleaseTask()) */
		wake_up_all(&gCmdqThreadDispatchQueue);
	}

	mutex_unlock(&gCmdqTaskMutex);

	CMDQ_PROF_END(current->pid, __func__);

	CMDQ_PROF_MMP(cmdq_mmp_get_event()->consume_done, MMProfileFlagEnd, current->pid, 0);

	return status;
}

static void cmdqCoreConsumeWaitQueueItem(struct work_struct *_ignore)
{
	int32_t status;

	status = cmdq_core_consume_waiting_list(_ignore);
}

int32_t cmdqCoreSubmitTaskAsyncImpl(cmdqCommandStruct *pCommandDesc,
				    CmdqInterruptCB loopCB,
				    unsigned long loopData, TaskStruct **ppTaskOut)
{
	struct TaskStruct *pTask = NULL;
	int32_t status = 0;

	if (CMDQ_SCENARIO_TRIGGER_LOOP != pCommandDesc->scenario)
		cmdq_core_verfiy_command_desc_end(pCommandDesc);

	CMDQ_MSG("-->SUBMIT_ASYNC: cmd 0x%p begin\n", CMDQ_U32_PTR(pCommandDesc->pVABase));
	CMDQ_PROF_START(current->pid, __func__);

	CMDQ_PROF_MMP(cmdq_mmp_get_event()->alloc_task, MMProfileFlagStart, current->pid, 0);

	/* Allocate Task. This creates a new task */
	/* and put into tail of waiting list */
	pTask = cmdq_core_acquire_task(pCommandDesc, loopCB, loopData);

	CMDQ_PROF_MMP(cmdq_mmp_get_event()->alloc_task, MMProfileFlagEnd, current->pid, 0);

	if (NULL == pTask) {
		CMDQ_PROF_END(current->pid, __func__);
		return -EFAULT;
	}

	if (NULL != ppTaskOut)
		*ppTaskOut = pTask;

	/* Try to lock resource base on engine flag */
	cmdqCoreLockResource(pTask->engineFlag, false);

	/* consume the waiting list. */
	/* this may or may not execute the task, */
	/* depending on available threads. */
	status = cmdq_core_consume_waiting_list(NULL);

	CMDQ_MSG("<--SUBMIT_ASYNC: task: 0x%p end\n", CMDQ_U32_PTR(pCommandDesc->pVABase));
	CMDQ_PROF_END(current->pid, __func__);
	return status;
}

int32_t cmdqCoreSubmitTaskAsync(cmdqCommandStruct *pCommandDesc,
				CmdqInterruptCB loopCB,
				unsigned long loopData, TaskStruct **ppTaskOut)
{
	int32_t status = 0;
	TaskStruct *pTask = NULL;

	if (true == pCommandDesc->secData.isSecure) {
		status = cmdq_core_start_secure_path_notify_thread();
		if (0 > status)
			return status;
	}

	status = cmdqCoreSubmitTaskAsyncImpl(pCommandDesc, loopCB, loopData, &pTask);
	if (NULL != ppTaskOut)
		*ppTaskOut = pTask;

	return status;
}

int32_t cmdqCoreReleaseTask(TaskStruct *pTask)
{
	unsigned long flags;
	int32_t status = 0;
	int32_t thread = pTask->thread;
	struct ThreadStruct *pThread = &(gCmdqContext.thread[thread]);

	CMDQ_MSG("<--TASK: cmdqCoreReleaseTask 0x%p\n", pTask);

	if (thread != CMDQ_INVALID_THREAD && NULL != pThread) {
		/* this task is being executed (or queueed) on a HW thread */

		/* get SW lock first to ensure atomic access HW */
		spin_lock_irqsave(&gCmdqExecLock, flags);
		/* make sure instructions are really in DRAM */
		smp_mb();

		if (pThread->loopCallback) {
			/* a loop thread has only 1 task involved */
			/* so we can release thread directly */
			/* otherwise we need to connect remaining tasks */
			BUG_ON(pThread->taskCount > 1);

			/* suspend and reset the thread */
			status = cmdq_core_suspend_HW_thread(thread, __LINE__);
			BUG_ON(status < 0);

			pThread->taskCount = 0;
			cmdq_core_disable_HW_thread(thread);
		} else {
			/* TODO: we should check thread enabled or not before resume it. */
			status = cmdq_core_force_remove_task_from_thread(pTask, thread);
			if (pThread->taskCount > 0)
				cmdq_core_resume_HW_thread(thread);
		}

		spin_unlock_irqrestore(&gCmdqExecLock, flags);
		wake_up(&gCmdWaitQueue[thread]);
	}

	cmdq_core_track_task_record(pTask, thread);
	cmdq_core_release_thread(pTask);
	cmdq_core_auto_release_task(pTask);
	CMDQ_MSG("-->TASK: cmdqCoreReleaseTask 0x%p end\n", pTask);
	return 0;
}

int32_t cmdqCoreWaitAndReleaseTask(TaskStruct *pTask, long timeout_jiffies)
{
	return cmdqCoreWaitResultAndReleaseTask(pTask, NULL, timeout_jiffies);
}

int32_t cmdqCoreWaitResultAndReleaseTask(TaskStruct *pTask, cmdqRegValueStruct *pResult,
					 long timeout_jiffies)
{
	int32_t status;
	int32_t thread;
	int i;

	if (NULL == pTask) {
		CMDQ_ERR("cmdqCoreWaitAndReleaseTask err ptr=0x%p\n", pTask);
		return -EFAULT;
	}

	if (pTask->taskState == TASK_STATE_IDLE) {
		CMDQ_ERR("cmdqCoreWaitAndReleaseTask task=0x%p is IDLE\n", pTask);
		return -EFAULT;
	}

	CMDQ_PROF_START(current->pid, __func__);

	/*  */
	/* wait for task finish */
	thread = pTask->thread;
	status = cmdq_core_wait_task_done(pTask, timeout_jiffies);

	/*  */
	/* retrieve result */
	if (pResult && pResult->count) {
		/* clear results */
		memset(CMDQ_U32_PTR(pResult->regValues), 0,
		       pResult->count * sizeof(CMDQ_U32_PTR(pResult->regValues)[0]));

		mutex_lock(&gCmdqTaskMutex);
		for (i = 0; i < pResult->count && i < pTask->regCount; ++i) {
			/* fill results */
			CMDQ_U32_PTR(pResult->regValues)[i] = pTask->regResults[i];
		}
		mutex_unlock(&gCmdqTaskMutex);
	}

	cmdq_core_track_task_record(pTask, thread);
	cmdq_core_release_thread(pTask);
	cmdq_core_auto_release_task(pTask);

	CMDQ_PROF_END(current->pid, __func__);

	return status;
}

static void cmdq_core_auto_release_work(struct work_struct *workItem)
{
	int32_t status = 0;
	TaskStruct *pTask = NULL;
	CmdqAsyncFlushCB finishCallback = NULL;
	uint32_t userData = 0;
	uint32_t *pCmd = NULL;
	int32_t commandSize = 0;

	pTask = container_of(workItem, struct TaskStruct, autoReleaseWork);

	if (pTask) {
		finishCallback = pTask->flushCallback;
		userData = pTask->flushData;
		commandSize = pTask->commandSize;
		pCmd = kzalloc(commandSize, GFP_KERNEL);
		memcpy(pCmd, pTask->pVABase, commandSize);

		status = cmdqCoreWaitResultAndReleaseTask(pTask,
							  NULL,
							  msecs_to_jiffies
							  (CMDQ_DEFAULT_TIMEOUT_MS));

		CMDQ_VERBOSE("[Auto Release] released pTask=%p, status=%d\n", pTask, status);
		CMDQ_PROF_MMP(cmdq_mmp_get_event()->autoRelease_done,
			      MMProfileFlagPulse, ((unsigned long)pTask), current->pid);

		/* Notify user */
		if (finishCallback) {
			CMDQ_VERBOSE("[Auto Release] call user callback %p with data 0x%08x\n",
				     finishCallback, userData);
			if (0 > finishCallback(userData)) {
				CMDQ_LOG
				    ("[DEBUG]user complains execution abnormal, dump command...\n");
				CMDQ_LOG("======TASK 0x%p command (%d) START\n", pTask,
					 commandSize);
				cmdqCoreDumpCommandMem(pCmd, commandSize);
				CMDQ_LOG("======TASK 0x%p command END\n", pTask);
			}
		}

		kfree(pCmd);
		pCmd = NULL;

		pTask = NULL;
	}
}

int32_t cmdqCoreAutoReleaseTask(TaskStruct *pTask)
{
	int32_t threadNo = CMDQ_INVALID_THREAD;
	bool isSecure;
	/* the work item is embeded in pTask already */
	/* but we need to initialized it */
	INIT_WORK(&pTask->autoReleaseWork, cmdq_core_auto_release_work);
	pTask->useWorkQueue = true;
	CMDQ_PROF_MMP(cmdq_mmp_get_event()->autoRelease_add,
		      MMProfileFlagPulse, ((unsigned long)pTask), pTask->thread);

	/* Put auto release task to corresponded thread */
	if (CMDQ_INVALID_THREAD != pTask->thread) {
		queue_work(gCmdqContext.thread[pTask->thread].taskThreadAutoReleaseWQ,
			   &pTask->autoReleaseWork);
	} else {
		/* if task does not belong thread, use static dispatch thread at first, */
		/* otherwise, use global context workqueue */
		isSecure = pTask->secData.isSecure;
		threadNo = cmdq_get_func()->getThreadID(pTask->scenario, isSecure);
		if (CMDQ_INVALID_THREAD != threadNo) {
			queue_work(gCmdqContext.thread[threadNo].taskThreadAutoReleaseWQ,
				   &pTask->autoReleaseWork);
		} else {
			queue_work(gCmdqContext.taskAutoReleaseWQ, &pTask->autoReleaseWork);
		}
	}
	return 0;
}

int32_t cmdqCoreSubmitTask(cmdqCommandStruct *pCommandDesc)
{
	int32_t status;
	TaskStruct *pTask = NULL;

	CMDQ_MSG("-->SUBMIT: SYNC cmd 0x%p begin\n", CMDQ_U32_PTR(pCommandDesc->pVABase));
	status = cmdqCoreSubmitTaskAsync(pCommandDesc, NULL, 0, &pTask);

	if (status >= 0) {
		status = cmdqCoreWaitResultAndReleaseTask(pTask,
							  &pCommandDesc->regValue,
							  msecs_to_jiffies
							  (CMDQ_DEFAULT_TIMEOUT_MS));
		if (status < 0) {
			/* error status print */
			CMDQ_ERR("Task 0x%p wait fails\n", pTask);
		}
	} else {
		CMDQ_ERR("cmdqCoreSubmitTaskAsync failed=%d", status);
	}

	CMDQ_MSG("<--SUBMIT: SYNC cmd 0x%p end\n", CMDQ_U32_PTR(pCommandDesc->pVABase));
	return status;
}

int32_t cmdqCoreQueryUsage(int32_t *pCount)
{
	unsigned long flags;
	EngineStruct *pEngine;
	int32_t index;

	pEngine = gCmdqContext.engine;

	spin_lock_irqsave(&gCmdqThreadLock, flags);

	for (index = 0; index < CMDQ_MAX_ENGINE_COUNT; index++)
		pCount[index] = pEngine[index].userCount;

	spin_unlock_irqrestore(&gCmdqThreadLock, flags);

	return 0;
}

void cmdq_core_release_task_by_file_node(void *file_node)
{
	struct TaskStruct *pTask = NULL;
	struct list_head *p = NULL;

	/* Since the file node is closed, there is no way */
	/* user space can issue further "wait_and_close" request, */
	/* so we must auto-release running/waiting tasks */
	/* to prevent resource leakage */

	/* walk through active and waiting lists and release them */
	mutex_lock(&gCmdqTaskMutex);

	list_for_each(p, &gCmdqContext.taskActiveList) {
		pTask = list_entry(p, struct TaskStruct, listEntry);
		if (TASK_STATE_IDLE != pTask->taskState &&
		    pTask->privateData == file_node &&
		    (cmdq_core_is_request_from_user_space(pTask->scenario))) {

			CMDQ_LOG
			    ("[WARNING] ACTIVE task 0x%p release because file node 0x%p closed\n",
			     pTask, file_node);
			cmdq_core_dump_task(pTask);

			/* since we already inside mutex, */
			/* do not cmdqReleaseTask directly, */
			/* instead we change state to "KILLED" */
			/* and arrange a auto-release. */
			/* Note that these tasks may already issued to HW */
			/* so there is a chance that following MPU/M4U violation */
			/* may occur, if the user space process has destroyed. */
			/* The ideal solution is to stop / cancel HW operation */
			/* immediately, but we cannot do so due to SMI hang risk. */
			cmdqCoreAutoReleaseTask(pTask);
		}
	}
	list_for_each(p, &gCmdqContext.taskWaitList) {
		pTask = list_entry(p, struct TaskStruct, listEntry);
		if (TASK_STATE_WAITING == pTask->taskState &&
		    pTask->privateData == file_node &&
		    (cmdq_core_is_request_from_user_space(pTask->scenario))) {

			CMDQ_LOG
			    ("[WARNING] WAITING task 0x%p release because file node 0x%p closed\n",
			     pTask, file_node);
			cmdq_core_dump_task(pTask);

			/* since we already inside mutex, */
			/* and these WAITING tasks will not be consumed (acquire thread / exec) */
			/* we can release them directly. */
			/* note that we use unlocked version since we already hold gCmdqTaskMutex. */
			cmdq_core_release_task_unlocked(pTask);
		}
	}

	mutex_unlock(&gCmdqTaskMutex);
}

unsigned long long cmdq_core_get_GPR64(const CMDQ_DATA_REGISTER_ENUM regID)
{
#ifdef CMDQ_GPR_SUPPORT
	unsigned long long value;
	unsigned long long value1;
	unsigned long long value2;
	const uint32_t x = regID & 0x0F;

	if (0 < (regID & 0x10)) {
		/* query address GPR(64bit), Px */
		value1 = 0LL | CMDQ_REG_GET32(CMDQ_GPR_R32((2 * x)));
		value2 = 0LL | CMDQ_REG_GET32(CMDQ_GPR_R32((2 * x + 1)));
	} else {
		/* query data GPR(32bit), Rx */
		value1 = 0LL | CMDQ_REG_GET32(CMDQ_GPR_R32(x));
		value2 = 0LL;
	}

	value = (0LL) | (value2 << 32) | (value1);
	CMDQ_VERBOSE("get_GPR64(%x): 0x%llx(0x%llx, 0x%llx)\n", regID, value, value2, value1);

	return value;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return 0LL;
#endif
}

void cmdq_core_set_GPR64(const CMDQ_DATA_REGISTER_ENUM regID, const unsigned long long value)
{
#ifdef CMDQ_GPR_SUPPORT

	const unsigned long long value1 = 0x00000000FFFFFFFF & value;
	const unsigned long long value2 = 0LL | value >> 32;
	const uint32_t x = regID & 0x0F;
	unsigned long long result;

	if (0 < (regID & 0x10)) {
		/* set address GPR(64bit), Px */
		CMDQ_REG_SET32(CMDQ_GPR_R32((2 * x)), value1);
		CMDQ_REG_SET32(CMDQ_GPR_R32((2 * x + 1)), value2);
	} else {
		/* set data GPR(32bit), Rx */
		CMDQ_REG_SET32(CMDQ_GPR_R32((2 * x)), value1);
	}

	result = 0LL | cmdq_core_get_GPR64(regID);
	if (value != result) {
		CMDQ_ERR("set_GPR64(%x) failed, value is 0x%llx, not value 0x%llx\n", regID, result,
			 value);
	}
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
#endif
}

uint32_t cmdqCoreReadDataRegister(CMDQ_DATA_REGISTER_ENUM regID)
{
#ifdef CMDQ_GPR_SUPPORT
	return CMDQ_REG_GET32(CMDQ_GPR_R32(regID));
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return 0;
#endif
}

int32_t cmdqCoreInitialize(void)
{
	struct TaskStruct *pTask;
	int32_t index;

	atomic_set(&gCmdqThreadUsage, 0);
	atomic_set(&gSMIThreadUsage, 0);

	BUG_ON(0 != atomic_read(&gCmdqThreadUsage));
	BUG_ON(0 != atomic_read(&gSMIThreadUsage));

	for (index = 0; index < CMDQ_MAX_THREAD_COUNT; index++)
		init_waitqueue_head(&gCmdWaitQueue[index]);

	init_waitqueue_head(&gCmdqThreadDispatchQueue);

	/* Reset overall context */
	memset(&gCmdqContext, 0x0, sizeof(ContextStruct));
	/* some fields has non-zero initial value */
	cmdq_core_reset_engine_struct();
	cmdq_core_reset_thread_struct();

	/* Create task pool */
	gCmdqContext.taskCache = kmem_cache_create(CMDQ_DRIVER_DEVICE_NAME "_task",
						   sizeof(struct TaskStruct),
						   __alignof__(struct TaskStruct),
						   SLAB_POISON | SLAB_HWCACHE_ALIGN | SLAB_RED_ZONE,
						   &cmdq_core_task_ctor);
	/* Initialize task lists */
	INIT_LIST_HEAD(&gCmdqContext.taskFreeList);
	INIT_LIST_HEAD(&gCmdqContext.taskActiveList);
	INIT_LIST_HEAD(&gCmdqContext.taskWaitList);
	INIT_LIST_HEAD(&gCmdqContext.resourceList);
	INIT_WORK(&gCmdqContext.taskConsumeWaitQueueItem, cmdqCoreConsumeWaitQueueItem);

	/* Initialize writable address */
	INIT_LIST_HEAD(&gCmdqContext.writeAddrList);

	/* Initialize emergency buffer */
	cmdq_core_init_emergency_buffer();

	gCmdqContext.taskAutoReleaseWQ = create_singlethread_workqueue("cmdq_auto_release");
	gCmdqContext.taskConsumeWQ = create_singlethread_workqueue("cmdq_task");
	gCmdqContext.resourceCheckWQ = create_singlethread_workqueue("cmdq_resource");

	/* Initialize command buffer dump */
	memset(&gCmdqBufferDump, 0x0, sizeof(DumpCommandBufferStruct));

#ifdef CMDQ_DUMP_FIRSTERROR
	/* Reset overall first error dump */
	memset(&gCmdqFirstError, 0x0, sizeof(DumpFirstErrorStruct));
	gCmdqFirstError.cmdqMaxSize = CMDQ_MAX_FIRSTERROR;
#endif

#ifdef CMDQ_EVENT_NEED_BACKUP
	/* Initialize backup event */
	cmdq_get_func()->initialBackupEvent();
#endif

	/* pre-allocate free tasks */
	for (index = 0; index < CMDQ_INIT_FREE_TASK_COUNT; index++) {
		pTask = cmdq_core_task_create();
		if (pTask) {
			mutex_lock(&gCmdqTaskMutex);
			list_add_tail(&(pTask->listEntry), &gCmdqContext.taskFreeList);
			mutex_unlock(&gCmdqTaskMutex);
		}
	}

	/* allocate shared memory */
	gCmdqContext.hSecSharedMem = NULL;
#ifdef CMDQ_SECURE_PATH_SUPPORT
	cmdq_sec_create_shared_memory(&(gCmdqContext.hSecSharedMem), PAGE_SIZE);
#endif

#if 0
	/* cmdqCoreRegisterDebugRegDumpCB(testcase_regdump_begin, testcase_regdump_end); */
#endif

	/* Initialize MET for statistics */
	/* note that we don't need to uninit it. */
	CMDQ_PROF_INIT();
#ifdef CMDQ_PROFILE_MMP
	cmdq_mmp_init();
#endif
	/* Initialize secure path context */
	cmdqSecInitialize();
	/* Initialize test case structure */
	cmdq_test_init_setting();
	/* Initialize Resource TODO: via device tree */
	cmdq_core_init_resource((1LL << CMDQ_ENG_MDP_WROT0), CMDQ_SYNC_RESOURCE_WROT0);

	return 0;
}

#ifdef CMDQ_SECURE_PATH_SUPPORT
int32_t cmdqCoreLateInitialize(void)
{
	int32_t status = 0;
	struct task_struct *open_th =
	    kthread_run(cmdq_sec_init_allocate_resource_thread, NULL, "cmdq_WSM_init");
	if (IS_ERR(open_th)) {
		CMDQ_LOG("%s, init kthread_run failed!\n", __func__);
		status = -EFAULT;
	}
	return status;
}
#endif

void cmdqCoreDeInitialize(void)
{
	struct TaskStruct *pTask = NULL;
	struct list_head *p;
	int index;
	struct list_head *lists[] = {
		&gCmdqContext.taskFreeList,
		&gCmdqContext.taskActiveList,
		&gCmdqContext.taskWaitList
	};

	/* directly destroy the auto release WQ since we're going to release tasks anyway. */
	destroy_workqueue(gCmdqContext.taskAutoReleaseWQ);
	gCmdqContext.taskAutoReleaseWQ = NULL;

	destroy_workqueue(gCmdqContext.taskConsumeWQ);
	gCmdqContext.taskConsumeWQ = NULL;

	destroy_workqueue(gCmdqContext.resourceCheckWQ);
	gCmdqContext.resourceCheckWQ = NULL;

	/* release all tasks in both list */
	for (index = 0; index < (sizeof(lists) / sizeof(lists[0])); ++index) {
		list_for_each(p, lists[index]) {
			mutex_lock(&gCmdqTaskMutex);

			pTask = list_entry(p, struct TaskStruct, listEntry);

			/* free allocated DMA buffer */
			cmdq_task_free_task_command_buffer(pTask);
			kmem_cache_free(gCmdqContext.taskCache, pTask);
			list_del(p);

			mutex_unlock(&gCmdqTaskMutex);
		}
	}

	/* check if there are dangling write addresses. */
	if (!list_empty(&gCmdqContext.writeAddrList)) {
		/* there are unreleased write buffer, raise AEE */
		CMDQ_AEE("CMDQ", "there are unreleased write buffer");
	}

	kmem_cache_destroy(gCmdqContext.taskCache);
	gCmdqContext.taskCache = NULL;

	/* release emergency buffer */
	cmdq_core_uninit_emergency_buffer();

	/* Deinitialize secure path context */
	cmdqSecDeInitialize();
}

int cmdqCoreAllocWriteAddress(uint32_t count, dma_addr_t *paStart)
{
	unsigned long flagsWriteAddr = 0L;
	WriteAddrStruct *pWriteAddr = NULL;
	int status = 0;

	CMDQ_VERBOSE("ALLOC: line %d\n", __LINE__);

	do {
		if (NULL == paStart) {
			CMDQ_ERR("invalid output argument\n");
			status = -EINVAL;
			break;
		}
		*paStart = 0;

		CMDQ_VERBOSE("ALLOC: line %d\n", __LINE__);

		pWriteAddr = kzalloc(sizeof(WriteAddrStruct), GFP_KERNEL);
		if (NULL == pWriteAddr) {
			CMDQ_ERR("failed to alloc WriteAddrStruct\n");
			status = -ENOMEM;
			break;
		}
		memset(pWriteAddr, 0, sizeof(WriteAddrStruct));

		CMDQ_VERBOSE("ALLOC: line %d\n", __LINE__);

		pWriteAddr->count = count;
		pWriteAddr->va =
		    cmdq_core_alloc_hw_buffer(cmdq_dev_get(),
					      count * sizeof(uint32_t), &(pWriteAddr->pa),
					      GFP_KERNEL);
		if (current)
			pWriteAddr->user = current->pid;

		CMDQ_VERBOSE("ALLOC: line %d\n", __LINE__);

		if (NULL == pWriteAddr->va) {
			CMDQ_ERR("failed to alloc write buffer\n");
			status = -ENOMEM;
			break;
		}

		CMDQ_VERBOSE("ALLOC: line %d\n", __LINE__);

		/* clear buffer content */
		do {
			volatile uint32_t *pInt = (uint32_t *) pWriteAddr->va;
			int i = 0;

			for (i = 0; i < count; ++i) {
				*(pInt + i) = 0xcdcdabab;
				mb();
				/* make sure instructions are really in DRAM */
				smp_mb();
			}
		} while (0);

		/* assign output pa */
		*paStart = pWriteAddr->pa;

		spin_lock_irqsave(&gCmdqWriteAddrLock, flagsWriteAddr);
		list_add_tail(&(pWriteAddr->list_node), &gCmdqContext.writeAddrList);
		spin_unlock_irqrestore(&gCmdqWriteAddrLock, flagsWriteAddr);

		status = 0;

	} while (0);

	if (0 != status) {
		/* release resources */
		if (pWriteAddr && pWriteAddr->va) {
			cmdq_core_free_hw_buffer(cmdq_dev_get(),
						 sizeof(uint32_t) * pWriteAddr->count,
						 pWriteAddr->va, pWriteAddr->pa);
			memset(pWriteAddr, 0, sizeof(WriteAddrStruct));
		}

		kfree(pWriteAddr);
		pWriteAddr = NULL;
	}

	CMDQ_VERBOSE("ALLOC: line %d\n", __LINE__);

	return status;
}

uint32_t cmdqCoreReadWriteAddress(dma_addr_t pa)
{
	struct list_head *p = NULL;
	WriteAddrStruct *pWriteAddr = NULL;
	int32_t offset = 0;
	uint32_t value = 0;
	unsigned long flagsWriteAddr = 0L;

	/* search for the entry */
	spin_lock_irqsave(&gCmdqWriteAddrLock, flagsWriteAddr);
	list_for_each(p, &gCmdqContext.writeAddrList) {
		pWriteAddr = list_entry(p, struct WriteAddrStruct, list_node);
		if (NULL == pWriteAddr)
			continue;

		offset = pa - pWriteAddr->pa;

		if (offset >= 0 && (offset / sizeof(uint32_t)) < pWriteAddr->count) {
			CMDQ_VERBOSE
			    ("cmdqCoreReadWriteAddress() input:%pa, got offset=%d va=%p pa_start=%pa\n",
			     &pa, offset, (pWriteAddr->va + offset), &(pWriteAddr->pa));
			value = *((volatile uint32_t *)(pWriteAddr->va + offset));
			CMDQ_VERBOSE
			    ("cmdqCoreReadWriteAddress() found offset=%d va=%p value=0x%08x\n",
			     offset, (pWriteAddr->va + offset), value);
			break;
		}
	}
	spin_unlock_irqrestore(&gCmdqWriteAddrLock, flagsWriteAddr);

	return value;
}

uint32_t cmdqCoreWriteWriteAddress(dma_addr_t pa, uint32_t value)
{
	struct list_head *p = NULL;
	WriteAddrStruct *pWriteAddr = NULL;
	int32_t offset = 0;
	unsigned long flagsWriteAddr = 0L;
	char longMsg[CMDQ_LONGSTRING_MAX];
	uint32_t msgOffset;
	int32_t msgMAXSize;

	/* search for the entry */
	spin_lock_irqsave(&gCmdqWriteAddrLock, flagsWriteAddr);
	list_for_each(p, &gCmdqContext.writeAddrList) {
		pWriteAddr = list_entry(p, struct WriteAddrStruct, list_node);
		if (NULL == pWriteAddr)
			continue;

		offset = pa - pWriteAddr->pa;

		/* note it is 64 bit length for uint32_t variable in 64 bit kernel */
		/* use sizeof(u_log) to check valid offset range */
		if (offset >= 0 && (offset / sizeof(unsigned long)) < pWriteAddr->count) {
			cmdq_core_longstring_init(longMsg, &msgOffset, &msgMAXSize);
			cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
					   "cmdqCoreWriteWriteAddress() input:0x%pa,", &pa);
			cmdqCoreLongString(false, longMsg, &msgOffset, &msgMAXSize,
					   " got offset=%d va=%p pa_start=0x%pa, value=0x%08x\n",
					   offset, (pWriteAddr->va + offset),
					   &pWriteAddr->pa, value);
			if (msgOffset > 0) {
				/* print message */
				CMDQ_VERBOSE("%s", longMsg);
			}

			*((volatile uint32_t *)(pWriteAddr->va + offset)) = value;
			break;
		}
	}
	spin_unlock_irqrestore(&gCmdqWriteAddrLock, flagsWriteAddr);

	return value;
}


int cmdqCoreFreeWriteAddress(dma_addr_t paStart)
{
	struct list_head *p, *n = NULL;
	WriteAddrStruct *pWriteAddr = NULL;
	bool foundEntry;
	unsigned long flagsWriteAddr = 0L;

	foundEntry = false;

	/* search for the entry */
	spin_lock_irqsave(&gCmdqWriteAddrLock, flagsWriteAddr);
	list_for_each_safe(p, n, &gCmdqContext.writeAddrList) {
		pWriteAddr = list_entry(p, struct WriteAddrStruct, list_node);
		if (pWriteAddr && pWriteAddr->pa == paStart) {
			list_del(&(pWriteAddr->list_node));
			foundEntry = true;
			break;
		}
	}
	spin_unlock_irqrestore(&gCmdqWriteAddrLock, flagsWriteAddr);

	/* when list is not empty, we always get a entry even we don't found a valid entry */
	/* use foundEntry to confirm search result */
	if (false == foundEntry) {
		CMDQ_ERR("cmdqCoreFreeWriteAddress() no matching entry, paStart:%pa\n", &paStart);
		return -EINVAL;
	}

	/* release resources */
	if (pWriteAddr->va) {
		cmdq_core_free_hw_buffer(cmdq_dev_get(),
					 sizeof(uint32_t) * pWriteAddr->count,
					 pWriteAddr->va, pWriteAddr->pa);
		memset(pWriteAddr, 0xdeaddead, sizeof(WriteAddrStruct));
	}

	kfree(pWriteAddr);
	pWriteAddr = NULL;

	return 0;
}

int32_t cmdqCoreDebugRegDumpBegin(uint32_t taskID, uint32_t *regCount, uint32_t **regAddress)
{
	if (NULL == gCmdqDebugCallback.beginDebugRegDump) {
		CMDQ_ERR("beginDebugRegDump not registered\n");
		return -EFAULT;
	}

	return gCmdqDebugCallback.beginDebugRegDump(taskID, regCount, regAddress);
}

int32_t cmdqCoreDebugRegDumpEnd(uint32_t taskID, uint32_t regCount, uint32_t *regValues)
{
	if (NULL == gCmdqDebugCallback.endDebugRegDump) {
		CMDQ_ERR("endDebugRegDump not registered\n");
		return -EFAULT;
	}

	return gCmdqDebugCallback.endDebugRegDump(taskID, regCount, regValues);
}

void cmdq_core_set_log_level(const int32_t value)
{
	if (value == CMDQ_LOG_LEVEL_NORMAL) {
		/* Only print CMDQ ERR and CMDQ LOG */
		gCmdqContext.logLevel = CMDQ_LOG_LEVEL_NORMAL;
	} else if (value < CMDQ_LOG_LEVEL_MAX) {
		/* Modify log level */
		gCmdqContext.logLevel = (1 << value);
	}
}

bool cmdq_core_should_print_msg(void)
{
	bool logLevel = (gCmdqContext.logLevel & (1 << CMDQ_LOG_LEVEL_MSG)) ? (1) : (0);
	return logLevel;
}

bool cmdq_core_should_full_error(void)
{
	bool logLevel = (gCmdqContext.logLevel & (1 << CMDQ_LOG_LEVEL_FULL_ERROR)) ? (1) : (0);
	return logLevel;
}

int32_t cmdq_core_profile_enabled(void)
{
	return gCmdqContext.enableProfile;
}

int32_t cmdq_core_enable_emergency_buffer_test(const bool enable)
{
#ifdef CMDQ_TEST_EMERGENCY_BUFFER
	const uint32_t value = (enable) ? (1) : (0);

	atomic_set(&gCmdqDebugForceUseEmergencyBuffer, value);
	return 0;
#else
	CMDQ_ERR("CMDQ_TEST_EMERGENCY_BUFFER not support\n");
	return -EFAULT;
#endif
}

void cmdq_core_longstring_init(char *buf, uint32_t *offset, int32_t *maxSize)
{
	buf[0] = '\0';
	*offset = 0;
	*maxSize = CMDQ_LONGSTRING_MAX - 1;
}

void cmdqCoreLongString(bool forceLog, char *buf, uint32_t *offset, int32_t *maxSize,
			const char *string, ...)
{
	int msgLen;
	va_list argptr;
	char *pBuffer;

	if ((false == forceLog) && (false == cmdq_core_should_print_msg()) && (*maxSize <= 0))
		return;

	va_start(argptr, string);
	pBuffer = buf + (*offset);
	msgLen = vsnprintf(pBuffer, *maxSize, string, argptr);
	*maxSize -= msgLen;
	if (*maxSize < 0)
		*maxSize = 0;
	*offset += msgLen;
	va_end(argptr);
}

void cmdq_core_turnoff_first_dump(void)
{
#ifdef CMDQ_DUMP_FIRSTERROR
	gCmdqFirstError.flag = false;
#endif
}

int32_t cmdq_core_save_first_dump(const char *string, ...)
{
#ifdef CMDQ_DUMP_FIRSTERROR
	int logLen;
	va_list argptr;
	char *pBuffer;

	if (false == gCmdqFirstError.flag)
		return -EFAULT;

	va_start(argptr, string);
	pBuffer = gCmdqFirstError.cmdqString + gCmdqFirstError.cmdqCount;
	logLen = vsnprintf(pBuffer, gCmdqFirstError.cmdqMaxSize, string, argptr);
	gCmdqFirstError.cmdqMaxSize -= logLen;
	gCmdqFirstError.cmdqCount += logLen;

	if (gCmdqFirstError.cmdqMaxSize <= 0) {
		gCmdqFirstError.flag = false;
		pr_err("[CMDQ][ERR] Error0 dump saving buffer is full\n");
	}
	va_end(argptr);
	return 0;
#else
	return -EFAULT;
#endif
}

#ifdef CMDQ_DUMP_FIRSTERROR
void cmdq_core_hex_dump_to_buffer(const void *buf, size_t len, int rowsize,
				  int groupsize, char *linebuf, size_t linebuflen)
{
	const u8 *ptr = buf;
	u8 ch;
	int j, lx = 0;

	if (rowsize != 16 && rowsize != 32)
		rowsize = 16;

	if (!len)
		goto nil;
	if (len > rowsize)	/* limit to one line at a time */
		len = rowsize;
	if ((len % groupsize) != 0)	/* no mixed size output */
		groupsize = 1;

	switch (groupsize) {
	case 8:{
			const u64 *ptr8 = buf;
			int ngroups = len / groupsize;

			for (j = 0; j < ngroups; j++)
				lx += scnprintf(linebuf + lx, linebuflen - lx,
						"%s%16.16llx", j ? " " : "",
						(unsigned long long)*(ptr8 + j));
			break;
		}

	case 4:{
			const u32 *ptr4 = buf;
			int ngroups = len / groupsize;

			for (j = 0; j < ngroups; j++)
				lx += scnprintf(linebuf + lx, linebuflen - lx,
						"%s%8.8x", j ? " " : "", *(ptr4 + j));
			break;
		}

	case 2:{
			const u16 *ptr2 = buf;
			int ngroups = len / groupsize;

			for (j = 0; j < ngroups; j++)
				lx += scnprintf(linebuf + lx, linebuflen - lx,
						"%s%4.4x", j ? " " : "", *(ptr2 + j));
			break;
		}

	default:
		for (j = 0; (j < len) && (lx + 3) <= linebuflen; j++) {
			ch = ptr[j];
			linebuf[lx++] = hex_asc_hi(ch);
			linebuf[lx++] = hex_asc_lo(ch);
			linebuf[lx++] = ' ';
		}
		if (j)
			lx--;
		break;
	}
nil:
	linebuf[lx++] = '\0';
}
#endif

void cmdq_core_save_hex_first_dump(const char *prefix_str,
				   int rowsize, int groupsize, const void *buf, size_t len)
{
#ifdef CMDQ_DUMP_FIRSTERROR
	const u8 *ptr = buf;
	int i, linelen, remaining = len;
	unsigned char linebuf[32 * 3 + 2 + 32 + 1];
	int logLen;
	char *pBuffer;

	if (false == gCmdqFirstError.flag)
		return;

	if (rowsize != 16 && rowsize != 32)
		rowsize = 16;

	for (i = 0; i < len; i += rowsize) {
		linelen = min(remaining, rowsize);
		remaining -= rowsize;

		cmdq_core_hex_dump_to_buffer(ptr + i, linelen, rowsize, groupsize,
					     linebuf, sizeof(linebuf));

		pBuffer = gCmdqFirstError.cmdqString + gCmdqFirstError.cmdqCount;
		logLen = snprintf(pBuffer, gCmdqFirstError.cmdqMaxSize, "%s%p: %s\n", prefix_str, ptr + i, linebuf);
		gCmdqFirstError.cmdqMaxSize -= logLen;
		gCmdqFirstError.cmdqCount += logLen;

		if (gCmdqFirstError.cmdqMaxSize <= 0) {
			gCmdqFirstError.flag = false;
			pr_err("[CMDQ][ERR] Error0 dump saving buffer is full\n");
		}
	}
#endif
}

/* Use CMDQ as Resource Manager */
void cmdqCoreLockResource(uint64_t engineFlag, bool fromNotify)
{
	struct ResourceUnitStruct *pResource = NULL;
	struct list_head *p = NULL;

	CMDQ_MSG("[Res] Lock resource with engine: 0x%016llx, fromNotify:%d\n",
		engineFlag, fromNotify);

	list_for_each(p, &gCmdqContext.resourceList) {
		pResource = list_entry(p, struct ResourceUnitStruct, listEntry);
		if (engineFlag & pResource->engine) {
			mutex_lock(&gCmdqResourceMutex);
			/* find matched engine */
			if (fromNotify)
				pResource->notify = sched_clock();
			else
				pResource->lock = sched_clock();

			if (!pResource->used) {
				/* First time used */
				int32_t status;

				CMDQ_MSG("[Res] Callback to release\n");
				if (NULL == pResource->releaseCB) {
					CMDQ_ERR("[Res]: release CB func is NULL, event:%d\n",
						pResource->lockEvent);
				} else {
					/* release mutex before callback */
					mutex_unlock(&gCmdqResourceMutex);
					status =
						pResource->releaseCB(pResource->lockEvent);
					mutex_lock(&gCmdqResourceMutex);

					if (status < 0) {
						/* Error status print */
						CMDQ_ERR("[Res]: release CB (%d) return fail:%d\n",
									pResource->lockEvent, status);
					}
				}
			}
			pResource->used = true;
			mutex_unlock(&gCmdqResourceMutex);
		}
	}
}

bool cmdqCoreAcquireResource(CMDQ_EVENT_ENUM resourceEvent)
{
	struct ResourceUnitStruct *pResource = NULL;
	struct list_head *p = NULL;
	bool result = false;

	CMDQ_MSG("[Res] Acquire resource with event: %d\n", resourceEvent);
	list_for_each(p, &gCmdqContext.resourceList) {
		pResource = list_entry(p, struct ResourceUnitStruct, listEntry);
		if (resourceEvent == pResource->lockEvent) {
			mutex_lock(&gCmdqResourceMutex);
			/* find matched resource */
			result = !pResource->used;
			if (result) {
				CMDQ_MSG("[Res] Acquire successfully, event: %d\n", resourceEvent);
				cmdqCoreClearEvent(resourceEvent);
				pResource->acquire = sched_clock();
			}
			mutex_unlock(&gCmdqResourceMutex);
			break;
		}
	}
	return result;
}

void cmdqCoreReleaseResource(CMDQ_EVENT_ENUM resourceEvent)
{
	struct ResourceUnitStruct *pResource = NULL;
	struct list_head *p = NULL;

	CMDQ_MSG("[Res] Release resource with event: %d\n", resourceEvent);
	list_for_each(p, &gCmdqContext.resourceList) {
		pResource = list_entry(p, struct ResourceUnitStruct, listEntry);
		if (resourceEvent == pResource->lockEvent) {
			mutex_lock(&gCmdqResourceMutex);
			/* find matched resource */
			pResource->release = sched_clock();
			mutex_unlock(&gCmdqResourceMutex);
			break;
		}
	}
}

void cmdqCoreSetResourceCallback(CMDQ_EVENT_ENUM resourceEvent,
							CmdqResourceAvailableCB resourceAvailable,
							CmdqResourceReleaseCB resourceRelease)
{
	struct ResourceUnitStruct *pResource = NULL;
	struct list_head *p = NULL;

	CMDQ_MSG("[Res] Set resource callback with event: %d\n", resourceEvent);
	list_for_each(p, &gCmdqContext.resourceList) {
		pResource = list_entry(p, struct ResourceUnitStruct, listEntry);
		if (resourceEvent == pResource->lockEvent) {
			CMDQ_MSG("[Res] Set resource callback ok!\n");
			mutex_lock(&gCmdqResourceMutex);
			/* find matched resource */
			pResource->availableCB = resourceAvailable;
			pResource->releaseCB = resourceRelease;
			mutex_unlock(&gCmdqResourceMutex);
			break;
		}
	}
}

