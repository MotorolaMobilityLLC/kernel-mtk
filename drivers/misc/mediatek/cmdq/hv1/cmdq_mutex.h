#ifndef __CMDQ_MUTEX_H__
#define __CMDQ_MUTEX_H__

#include <linux/mutex.h>
#include <ddp_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

	int32_t cmdqMutexInitialize(void);

	int32_t cmdqMutexAcquire(void);

	int32_t cmdqMutexRelease(int32_t mutex);

	void cmdqMutexDeInitialize(void);

	bool cmdqMDPMutexInUse(int index);

	pid_t cmdqMDPMutexOwnerPid(int index);


#ifdef __cplusplus
}
#endif
#endif				/* __CMDQ_MUTEX_H__ */
