/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */


/*! \file
 * \brief  Declaration of library functions
 * Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
 */

#ifndef _OSAL_H_
#define _OSAL_H_

#include "osal_typedef.h"
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#define OS_BIT_OPS_SUPPORT 1

#define _osal_inline_ inline

#define MAX_THREAD_NAME_LEN 16
#define MAX_WAKE_LOCK_NAME_LEN 16
#define OSAL_OP_BUF_SIZE    64

#if (defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) && !defined(CONFIG_MTK_ENG_BUILD))
#define OSAL_OP_DATA_SIZE   8
#else
#define OSAL_OP_DATA_SIZE   32
#endif

#define DBG_LOG_STR_SIZE    256

#define osal_sizeof(x) sizeof(x)

#define osal_array_size(x) ARRAY_SIZE(x)

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

#define WMT_OP_BIT(x) (0x1UL << x)
#define WMT_OP_HIF_BIT WMT_OP_BIT(0)

#define RB_LATEST(prb) ((prb)->write - 1)
#define RB_SIZE(prb) ((prb)->size)
#define RB_MASK(prb) (RB_SIZE(prb) - 1)
#define RB_COUNT(prb) ((prb)->write - (prb)->read)
#define RB_FULL(prb) (RB_COUNT(prb) >= RB_SIZE(prb))
#define RB_EMPTY(prb) ((prb)->write == (prb)->read)

#define RB_INIT(prb, qsize) \
do { \
	(prb)->read = (prb)->write = 0; \
	(prb)->size = (qsize); \
} while (0)

#define RB_PUT(prb, value) \
do { \
	if (!RB_FULL(prb)) { \
		(prb)->queue[(prb)->write & RB_MASK(prb)] = value; \
		++((prb)->write); \
	} \
	else { \
		osal_assert(!RB_FULL(prb)); \
	} \
} while (0)

#define RB_GET(prb, value) \
do { \
	if (!RB_EMPTY(prb)) { \
		value = (prb)->queue[(prb)->read & RB_MASK(prb)]; \
		++((prb)->read); \
		if (RB_EMPTY(prb)) { \
			(prb)->read = (prb)->write = 0; \
		} \
	} \
	else { \
		value = NULL; \
		osal_assert(!RB_EMPTY(prb)); \
	} \
} while (0)

#define RB_GET_LATEST(prb, value) \
do { \
	if (!RB_EMPTY(prb)) { \
		value = (prb)->queue[RB_LATEST(prb) & RB_MASK(prb)]; \
		if (RB_EMPTY(prb)) { \
			(prb)->read = (prb)->write = 0; \
		} \
	} \
	else { \
		value = NULL; \
	} \
} while (0)

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

typedef VOID(*P_TIMEOUT_HANDLER) (ULONG);
typedef INT32(*P_COND) (PVOID);

struct OSAL_TIMER {
	struct timer_list timer;
	P_TIMEOUT_HANDLER timeoutHandler;
	ULONG timeroutHandlerData;
};

struct OSAL_UNSLEEPABLE_LOCK {
	spinlock_t lock;
	ULONG flag;
};

struct OSAL_SLEEPABLE_LOCK {
	struct mutex lock;
};

struct OSAL_SIGNAL {
	struct completion comp;
	UINT32 timeoutValue;
};

struct OSAL_EVENT {
	wait_queue_head_t waitQueue;
/* VOID *pWaitQueueData; */
	UINT32 timeoutValue;
	INT32 waitFlag;

};

struct OSAL_THREAD {
	struct task_struct *pThread;
	PVOID pThreadFunc;
	PVOID pThreadData;
	INT8 threadName[MAX_THREAD_NAME_LEN];
};


struct OSAL_FIFO {
	/*fifo definition */
	PVOID pFifoBody;
	spinlock_t fifoSpinlock;
	/*fifo operations */
	INT32 (*FifoInit)(struct OSAL_FIFO *pFifo, PUINT8 buf, UINT32);
	INT32 (*FifoDeInit)(struct OSAL_FIFO *pFifo);
	INT32 (*FifoReset)(struct OSAL_FIFO *pFifo);
	INT32 (*FifoSz)(struct OSAL_FIFO *pFifo);
	INT32 (*FifoAvailSz)(struct OSAL_FIFO *pFifo);
	INT32 (*FifoLen)(struct OSAL_FIFO *pFifo);
	INT32 (*FifoIsEmpty)(struct OSAL_FIFO *pFifo);
	INT32 (*FifoIsFull)(struct OSAL_FIFO *pFifo);
	INT32 (*FifoDataIn)(struct OSAL_FIFO *pFifo, const PVOID buf, UINT32 len);
	INT32 (*FifoDataOut)(struct OSAL_FIFO *pFifo, PVOID buf, UINT32 len);
};



struct OSAL_OP_DAT {
	UINT32 opId;		/* Event ID */
	UINT32 u4InfoBit;	/* Reserved */
	SIZE_T au4OpData[OSAL_OP_DATA_SIZE];	/* OP Data */
};

struct OSAL_OP {
	struct OSAL_OP_DAT op;
	struct OSAL_SIGNAL signal;
	INT32 result;
};

struct OSAL_OP_Q {
	struct OSAL_SLEEPABLE_LOCK sLock;
	UINT32 write;
	UINT32 read;
	UINT32 size;
	struct OSAL_OP *queue[OSAL_OP_BUF_SIZE];
};

struct OSAL_WAKE_LOCK {
#if 1
    struct wakeup_source *wake_lock;
#else
    struct wake_lock wake_lock;
#endif
	UINT8 name[MAX_WAKE_LOCK_NAME_LEN];
};
#if 1
struct OSAL_BIT_OP_VAR {
	ULONG data;
	struct OSAL_UNSLEEPABLE_LOCK opLock;
};
#else
#define OSAL_BIT_OP_VAR unsigned long
#define P_OSAL_BIT_OP_VAR unsigned long *

#endif
typedef UINT32(*P_OSAL_EVENT_CHECKER) (struct OSAL_THREAD *pThread);
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

UINT32 osal_strlen(const PINT8 str);
INT32 osal_strcmp(const PINT8 dst, const PINT8 src);
INT32 osal_strncmp(const PINT8 dst, const PINT8 src, UINT32 len);
PINT8 osal_strcpy(PINT8 dst, const PINT8 src);
PINT8 osal_strncpy(PINT8 dst, const PINT8 src, UINT32 len);
PINT8 osal_strcat(PINT8 dst, const PINT8 src);
PINT8 osal_strncat(PINT8 dst, const PINT8 src, UINT32 len);
PINT8 osal_strchr(const PINT8 str, UINT8 c);
PINT8 osal_strsep(PPINT8 str, const PINT8 c);
INT32 osal_strtol(const PINT8 str, UINT32 adecimal, PLONG res);
PINT8 osal_strstr(PINT8 str1, const PINT8 str2);
PINT8 osal_strnstr(PINT8 str1, const PINT8 str2, INT32 n);

VOID osal_bug_on(UINT32 val);

INT32 osal_snprintf(PINT8 buf, UINT32 len, const PINT8 fmt, ...);
INT32 osal_err_print(const PINT8 str, ...);
INT32 osal_dbg_print(const PINT8 str, ...);
INT32 osal_warn_print(const PINT8 str, ...);

INT32 osal_dbg_assert(INT32 expr, const PINT8 file, INT32 line);
INT32 osal_dbg_assert_aee(const PINT8 module, const PINT8 detail_description);
INT32 osal_sprintf(PINT8 str, const PINT8 format, ...);
PVOID osal_malloc(UINT32 size);
VOID osal_free(const PVOID dst);
PVOID osal_memset(PVOID buf, INT32 i, UINT32 len);
PVOID osal_memcpy(PVOID dst, const PVOID src, UINT32 len);
VOID osal_memcpy_fromio(PVOID dst, const PVOID src, UINT32 len);
INT32 osal_memcmp(const PVOID buf1, const PVOID buf2, UINT32 len);

UINT16 osal_crc16(const PUINT8 buffer, const UINT32 length);
VOID osal_thread_show_stack(struct OSAL_THREAD *pThread);

INT32 osal_sleep_ms(UINT32 ms);
INT32 osal_udelay(UINT32 us);
INT32 osal_usleep_range(ULONG min, ULONG max);
INT32 osal_timer_create(struct OSAL_TIMER *pTimer);
INT32 osal_timer_start(struct OSAL_TIMER *pTimer, UINT32);
INT32 osal_timer_stop(struct OSAL_TIMER *pTimer);
INT32 osal_timer_stop_sync(struct OSAL_TIMER *pTimer);
INT32 osal_timer_modify(struct OSAL_TIMER *pTimer, UINT32);
INT32 osal_timer_delete(struct OSAL_TIMER *pTimer);

INT32 osal_fifo_init(struct OSAL_FIFO *pFifo, PUINT8 buffer, UINT32 size);
VOID osal_fifo_deinit(struct OSAL_FIFO *pFifo);
INT32 osal_fifo_reset(struct OSAL_FIFO *pFifo);
UINT32 osal_fifo_in(struct OSAL_FIFO *pFifo, PUINT8 buffer, UINT32 size);
UINT32 osal_fifo_out(struct OSAL_FIFO *pFifo, PUINT8 buffer, UINT32 size);
UINT32 osal_fifo_len(struct OSAL_FIFO *pFifo);
UINT32 osal_fifo_sz(struct OSAL_FIFO *pFifo);
UINT32 osal_fifo_avail(struct OSAL_FIFO *pFifo);
UINT32 osal_fifo_is_empty(struct OSAL_FIFO *pFifo);
UINT32 osal_fifo_is_full(struct OSAL_FIFO *pFifo);

INT32 osal_wake_lock_init(struct OSAL_WAKE_LOCK *plock);
INT32 osal_wake_lock(struct OSAL_WAKE_LOCK *plock);
INT32 osal_wake_unlock(struct OSAL_WAKE_LOCK *plock);
INT32 osal_wake_lock_count(struct OSAL_WAKE_LOCK *plock);
INT32 osal_wake_lock_deinit(struct OSAL_WAKE_LOCK *plock);

#if defined(CONFIG_PROVE_LOCKING)
#define osal_unsleepable_lock_init(l) { spin_lock_init(&((l)->lock)); }
#else
INT32 osal_unsleepable_lock_init(struct OSAL_UNSLEEPABLE_LOCK *pUSL);
#endif
INT32 osal_lock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL);
INT32 osal_unlock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL);
INT32 osal_unsleepable_lock_deinit(struct OSAL_UNSLEEPABLE_LOCK *pUSL);

#if defined(CONFIG_PROVE_LOCKING)
#define osal_sleepable_lock_init(l) { mutex_init(&((l)->lock)); }
#else
INT32 osal_sleepable_lock_init(struct OSAL_SLEEPABLE_LOCK *pSL);
#endif
INT32 osal_lock_sleepable_lock(struct OSAL_SLEEPABLE_LOCK *pSL);
INT32 osal_unlock_sleepable_lock(struct OSAL_SLEEPABLE_LOCK *pSL);
INT32 osal_sleepable_lock_deinit(struct OSAL_SLEEPABLE_LOCK *pSL);

INT32 osal_signal_init(struct OSAL_SIGNAL *pSignal);
INT32 osal_wait_for_signal(struct OSAL_SIGNAL *pSignal);
INT32 osal_wait_for_signal_timeout(struct OSAL_SIGNAL *pSignal);
INT32 osal_raise_signal(struct OSAL_SIGNAL *pSignal);
INT32 osal_signal_active_state(struct OSAL_SIGNAL *pSignal);
INT32 osal_signal_deinit(struct OSAL_SIGNAL *pSignal);

INT32 osal_event_init(struct OSAL_EVENT *pEvent);
INT32 osal_wait_for_event(struct OSAL_EVENT *pEvent, P_COND, PVOID);
INT32 osal_wait_for_event_timeout(struct OSAL_EVENT *pEvent, P_COND, PVOID);
extern INT32 osal_trigger_event(struct OSAL_EVENT *pEvent);

INT32 osal_event_deinit(struct OSAL_EVENT *pEvent);
LONG osal_wait_for_event_bit_set(struct OSAL_EVENT *pEvent, PULONG pState, UINT32 bitOffset);
LONG osal_wait_for_event_bit_clr(struct OSAL_EVENT *pEvent, PULONG pState, UINT32 bitOffset);

INT32 osal_thread_create(struct OSAL_THREAD *pThread);
INT32 osal_thread_run(struct OSAL_THREAD *pThread);
INT32 osal_thread_should_stop(struct OSAL_THREAD *pThread);
INT32 osal_thread_stop(struct OSAL_THREAD *pThread);
/*INT32 osal_thread_wait_for_event(P_OSAL_THREAD, P_OSAL_EVENT);*/
INT32 osal_thread_wait_for_event(struct OSAL_THREAD *pThread, struct OSAL_EVENT *pEvent, P_OSAL_EVENT_CHECKER);
/*check pOsalLxOp and OSAL_THREAD_SHOULD_STOP*/
INT32 osal_thread_destroy(struct OSAL_THREAD *pThread);

INT32 osal_clear_bit(UINT32 bitOffset, struct OSAL_BIT_OP_VAR *pData);
INT32 osal_set_bit(UINT32 bitOffset, struct OSAL_BIT_OP_VAR *pData);
INT32 osal_test_bit(UINT32 bitOffset, struct OSAL_BIT_OP_VAR *pData);
INT32 osal_test_and_clear_bit(UINT32 bitOffset, struct OSAL_BIT_OP_VAR *pData);
INT32 osal_test_and_set_bit(UINT32 bitOffset, struct OSAL_BIT_OP_VAR *pData);

INT32 osal_gettimeofday(PINT32 sec, PINT32 usec);
INT32 osal_printtimeofday(const PUINT8 prefix);
VOID osal_get_local_time(PUINT64 sec, PULONG nsec);

VOID osal_buffer_dump(const PUINT8 buf, const PUINT8 title, UINT32 len, UINT32 limit);

UINT32 osal_op_get_id(struct OSAL_OP *pOp);
MTK_WCN_BOOL osal_op_is_wait_for_signal(struct OSAL_OP *pOp);
VOID osal_op_raise_signal(struct OSAL_OP *pOp, INT32 result);
VOID osal_set_op_result(struct OSAL_OP *pOp, INT32 result);

INT32 osal_ftrace_print(const PINT8 str, ...);
INT32 osal_ftrace_print_ctrl(INT32 flag);
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#define osal_assert(condition) \
do { \
	if (!(condition)) \
		osal_err_print("%s, %d, (%s)\n", __FILE__, __LINE__, #condition); \
} while (0)

#endif /* _OSAL_H_ */
