/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


#include <gz/kree/mem.h>
#include <gz/kree/system.h>
#include <gz/tz_cross/ta_mem.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define EACH_MAP_ENTRY_SIZE sizeof(struct KREE_SHM_RUNLENGTH_ENTRY)

#define MAX_PA_ENTRY (256/EACH_MAP_ENTRY_SIZE)
#define MAX_MARY_SIZE MAX_PA_ENTRY
#define MAX_NUM_OF_PARAM 3

#define DBG_KREE_MEM
#ifdef DBG_KREE_MEM
#define KREE_DEBUG(fmt...) pr_debug("[KREE_MEM]"fmt)
#define KREE_ERR(fmt...) pr_debug("[KREE_MEM][ERR]"fmt)
#else
#define KREE_DEBUG(fmt...)
#define KREE_ERR(fmt...) pr_debug("[KREE_MEM][ERR]"fmt)
#endif

DEFINE_MUTEX(shared_mem_mutex);

/* notiec: handle type is the same */
static inline TZ_RESULT _allocFunc(uint32_t cmd, KREE_SESSION_HANDLE session,
				KREE_SECUREMEM_HANDLE *mem_handle, uint32_t alignment,
				uint32_t size, char *dbg)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	if ((mem_handle == NULL) || (size == 0)) {
		KREE_ERR("_allocFunc: invalid parameters\n");
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}

	p[0].value.a = alignment;
	p[1].value.a = size;
	ret = KREE_TeeServiceCall(session, cmd,
					TZ_ParamTypes3(TZPT_VALUE_INPUT,
							TZPT_VALUE_INPUT,
							TZPT_VALUE_OUTPUT),
					p);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("%s Error: %d\n", dbg, ret);
		return ret;
	}

	*mem_handle = (KREE_SECUREMEM_HANDLE)p[2].value.a;

	return TZ_RESULT_SUCCESS;
}

static inline TZ_RESULT _handleOpFunc(uint32_t cmd, KREE_SESSION_HANDLE session,
					  KREE_SECUREMEM_HANDLE mem_handle, char *dbg)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	if ((mem_handle == 0))
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	p[0].value.a = (uint32_t) mem_handle;
	ret = KREE_TeeServiceCall(session, cmd,
					TZ_ParamTypes1(TZPT_VALUE_INPUT), p);
	if (ret < 0) {
		KREE_ERR("%s Error: %d\n", dbg, ret);
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}

static inline TZ_RESULT _handleOpFunc_1(uint32_t cmd,
					KREE_SESSION_HANDLE session,
					KREE_SECUREMEM_HANDLE mem_handle, uint32_t *count,
					char *dbg)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	if ((mem_handle == 0) || (count == NULL))
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	p[0].value.a = (uint32_t) mem_handle;
	ret = KREE_TeeServiceCall(session, cmd,
					TZ_ParamTypes2(TZPT_VALUE_INPUT,
							TZPT_VALUE_OUTPUT),
					p);
	if (ret < 0) {
		KREE_ERR("%s Error: %d\n", dbg, ret);
		*count = 0;
		return ret;
	}

	*count = p[1].value.a;

	return TZ_RESULT_SUCCESS;
}

static void add_shm_list_node(struct KREE_SHM_RUNLENGTH_LIST **tail, uint64_t start, uint32_t size)
{
	struct KREE_SHM_RUNLENGTH_LIST *mylist;

	if (!tail)
		return;

	/* add a record */
	mylist = kmalloc(sizeof(*mylist), GFP_KERNEL);
	mylist->entry.low = (uint32_t) (start & (0x00000000ffffffff));
	mylist->entry.high = (uint32_t) (start >> 32);
	mylist->entry.size = size;
	mylist->next = NULL;
	(*tail)->next = mylist;
	(*tail) = mylist;
}

struct KREE_SHM_RUNLENGTH_ENTRY *shmem_param_run_length_encoding(
		int numOfPA, int *runLeng_arySize, uint64_t *ary)
{
	int arySize = numOfPA + 1;
	int i = 0;
	uint64_t start;
	uint64_t now, next;
	uint32_t size = 1;
	int xx = 0;
	int idx = 0;

	struct KREE_SHM_RUNLENGTH_LIST *head, *tail;
	struct KREE_SHM_RUNLENGTH_LIST *curr;
	struct KREE_SHM_RUNLENGTH_ENTRY *runLengAry = NULL;

	/* compress by run length coding */
	KREE_DEBUG("[%s] ====> shmem_param_run_length_encoding. numOfPA=%d, runLeng_arySize=%d\n",
			__func__, numOfPA, *runLeng_arySize);

	head = kmalloc(sizeof(*head), GFP_KERNEL);
	head->next = NULL;
	tail = head;

	for (i = 1; i < arySize; i++) {
		now  = ary[i];

		if (size == 1)
			start = now;
		next = ary[i+1];

		if ((i+1) < arySize) {

			next = ary[i+1];
			if ((next-now) == (1*PAGE_SIZE)) {
				size++;
			} else {
				add_shm_list_node(&tail, start, size);
				size = 1; /* reset */
				xx++;
			}

		} else if (i == (arySize-1)) {	/* process the last entry */

			if ((ary[i] - start + (1*PAGE_SIZE)) == (size * PAGE_SIZE)) {

				add_shm_list_node(&tail, start, size);
				size = 1; /* reset */
				xx++;
			}
		}
	}

	*runLeng_arySize = xx;
	KREE_DEBUG("=====> runLeng_arySize = %d\n", *runLeng_arySize);

	runLengAry = kmalloc((*runLeng_arySize+1) * sizeof(struct KREE_SHM_RUNLENGTH_ENTRY), GFP_KERNEL);

	runLengAry[0].high = numOfPA;
	runLengAry[0].low  = *runLeng_arySize;
	runLengAry[0].size = 0x0;

	idx = 1;

	if (head->next != NULL) {
		curr = head->next;
		while (curr != NULL) {
			runLengAry[idx].high = curr->entry.high;
			runLengAry[idx].low = curr->entry.low;
			runLengAry[idx].size = curr->entry.size;

			idx++;
			tail = curr;
			curr = curr->next;

			kfree(tail);
		}
	}
	kfree(head);

#ifdef DBG_KREE_SHM
	for (idx = 0; idx <= xx; idx++)
		KREE_DEBUG("=====> runLengAry[%d]. high = 0x%x, low=0x%x, size = 0x%x\n",
				idx, runLengAry[idx].high, runLengAry[idx].low, runLengAry[idx].size);
#endif
	KREE_DEBUG("========> end of run length encoding =================================\n");

	return runLengAry;
}

static TZ_RESULT send_shm_cmd(int round, union MTEEC_PARAM *p, KREE_SESSION_HANDLE session, uint32_t numOfPA)
{
	uint32_t paramTypes;
	TZ_RESULT ret = 0;

	p[3].value.a = round;
	p[3].value.b = numOfPA;
	paramTypes = TZ_ParamTypes4(TZPT_MEM_INPUT, TZPT_MEM_INPUT, TZPT_MEM_INPUT, TZPT_VALUE_INOUT);
	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SHAREDMEM_REG, paramTypes, p);

	return ret;
}

static TZ_RESULT send_shm_ending_cmd(union MTEEC_PARAM *p, KREE_SESSION_HANDLE session, uint32_t numOfPA)
{
	int i;
	uint32_t paramTypes;
	TZ_RESULT ret = 0;

	/* send ending command */
	for (i = 0; i < MAX_NUM_OF_PARAM; i++) {
		p[i].mem.buffer = NULL;
		p[i].mem.size = 0;
	}
	p[3].value.a = -99;
	p[3].value.b = numOfPA;
	paramTypes = TZ_ParamTypes4(TZPT_MEM_INPUT, TZPT_MEM_INPUT, TZPT_MEM_INPUT, TZPT_VALUE_INOUT);
	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SHAREDMEM_REG, paramTypes, p);

	return ret;
}

#ifdef DBG_KREE_SHM
static void print_runlength_arr(union MTEEC_PARAM *p, int *prt_id)
{
	int i, j;
	int entry_num;
	struct KREE_SHM_RUNLENGTH_ENTRY *pp2;

	if (!p)
		return;

	for (i = 0; i < MAX_NUM_OF_PARAM; i++) {
		if (!p[i].mem.size)
			continue;

		pp2 = (struct KREE_SHM_RUNLENGTH_ENTRY *) p[i].mem.buffer;
		entry_num = (int)(p[i].mem.size/EACH_MAP_ENTRY_SIZE);

		KREE_DEBUG("[%s] p[%d].mem.buffer done --> size =0x%x [%d entries]\n",
			__func__, i, p[i].mem.size, entry_num);

		for (j = 0; j < entry_num; j++) {
			if ((j == 0) || j == (entry_num - 1)) {
				KREE_DEBUG("[%s][loop][%d].pp2[%d] high=0x%x,low=0x%x,size=0x%x\n",
					__func__, (*prt_id)++, j,
					(uint32_t) pp2[j].high, (uint32_t) pp2[j].low,
					(uint32_t) pp2[j].size);
			}
		}

	}
}
#endif

static void init_shm_params(union MTEEC_PARAM *p, int *arr)
{
	int i;

	for (i = 0; i < MAX_NUM_OF_PARAM; i++) {
		p[i].mem.buffer = NULL;
		p[i].mem.size = 0;
		if (arr)
			arr[i] = 0;
	}
}

static TZ_RESULT kree_register_cont_shm(union MTEEC_PARAM *p,
										KREE_SESSION_HANDLE session,
										void *start, uint32_t size)
{
	TZ_RESULT ret = 0;
	int numOfPA;
#ifdef DBG_KREE_SHM
	int prt_id;
#endif
	struct KREE_SHM_RUNLENGTH_ENTRY *tmpAry = NULL;

	/* input data: ex: start = 0xaa000000; size = 8192(8KB) */
	KREE_DEBUG("[%s] Map Continuous Pages\n", __func__);

	/* init mtee param & other buffer */
	init_shm_params(p, NULL);

	numOfPA =  size / PAGE_SIZE;
	if ((size % PAGE_SIZE) != 0)
		numOfPA++;
	KREE_DEBUG("[%s] numOfPA= 0x%x\n", __func__, numOfPA);

	tmpAry = kmalloc((MAX_MARY_SIZE) * sizeof(struct KREE_SHM_RUNLENGTH_ENTRY), GFP_KERNEL);
	tmpAry[0].high = (uint32_t) ((uint64_t) start >> 32);
	tmpAry[0].low = (uint32_t) ((uint64_t) start & (0x00000000ffffffff));
	tmpAry[0].size = numOfPA;

	p[0].mem.buffer = tmpAry;
	p[0].mem.size = EACH_MAP_ENTRY_SIZE;

	KREE_DEBUG("[%s] [Case1]====> tmpAry[0] high= 0x%x, low= 0x%x, size= 0x%x\n",
			__func__,
			(uint32_t)tmpAry[0].high,
			(uint32_t)tmpAry[0].low,
			(uint32_t)tmpAry[0].size);

#ifdef DBG_KREE_SHM
	print_runlength_arr(p, &prt_id);
#endif
	ret = send_shm_cmd(0, p, session, numOfPA);
	ret = send_shm_ending_cmd(p, session, numOfPA);
	/* ------------------------------------------------------------------ */
	kfree(tmpAry);

	return ret;
}

static TZ_RESULT kree_register_desc_shm(union MTEEC_PARAM *p, KREE_SESSION_HANDLE session,
									void *start, uint32_t size, void *mapAry)
{
	TZ_RESULT ret = 0;
	int numOfPA;
	int i, idx = 0, p_idx = 0, offset = 0;
	uint64_t *ary;
	int round = 0;
	int runLeng_arySize = 0;
#ifdef DBG_KREE_SHM
	int prt_id;
#endif
	int tmp_ary_entryNum[MAX_NUM_OF_PARAM];
	struct KREE_SHM_RUNLENGTH_ENTRY tmp_ary[MAX_NUM_OF_PARAM][MAX_MARY_SIZE];
	struct KREE_SHM_RUNLENGTH_ENTRY *runLengAry = NULL;

	KREE_DEBUG("[%s] Map Discontinuous Pages\n", __func__);

	/* init mtee param & other buffer */
	init_shm_params(p, tmp_ary_entryNum);

	ary = (uint64_t *) mapAry;
	numOfPA = ary[0];
	KREE_DEBUG("[%s] numOfPA = %d, MAX_MARY_SIZE = %lu\n", __func__, numOfPA, MAX_MARY_SIZE);

	/* encode page tables */
	runLengAry = shmem_param_run_length_encoding(numOfPA, &runLeng_arySize, ary);

#ifdef DBG_KREE_SHM
	for (i = 0; i <= numOfPA; i++)
		KREE_DEBUG("[%s] ====> mapAry[%d]= 0x%llx\n", __func__, i, (uint64_t) ary[i]);
	for (idx = 0; idx <= runLeng_arySize; idx++)
		KREE_DEBUG("=====> runLengAry[%d]. high = 0x%x, low=0x%x, size = 0x%x\n",
				idx, runLengAry[idx].high, runLengAry[idx].low, runLengAry[idx].size);
#endif

	/* start sending page tables... */
	idx = 1;
	do {
#ifdef DBG_KREE_SHM
		KREE_DEBUG("[%s]=====> idx [%d] runs.....\n", __func__, idx);
#endif
		if (idx % (MAX_NUM_OF_PARAM * MAX_MARY_SIZE) == 1) { /* each round restarts */

			if (tmp_ary_entryNum[0] > 0) {
				for (i = 0; i < MAX_NUM_OF_PARAM; i++) {
					p[i].mem.buffer = tmp_ary[i];
					/* #ofEntry * 12 Bytes */
					p[i].mem.size = tmp_ary_entryNum[i] * EACH_MAP_ENTRY_SIZE;
#ifdef DBG_KREE_SHM
					KREE_DEBUG("[%s] [loop] p[i].mem.size = %d [entryNum =%d]\n",
						__func__, p[i].mem.size, tmp_ary_entryNum[i]);
#endif
				}

#ifdef DBG_KREE_SHM
				prt_id = 1;
				print_runlength_arr(p, &prt_id);
#endif
				/* send a command */
				ret = send_shm_cmd(round, p, session, numOfPA);
				KREE_DEBUG("[%s]====> round %d done, restart\n", __func__, round);
				round++;
			}

			init_shm_params(p, tmp_ary_entryNum);
		}

		round = (int) ((idx-1) / (MAX_MARY_SIZE * MAX_NUM_OF_PARAM));
		p_idx = (int) ((idx-1) / MAX_MARY_SIZE) % MAX_NUM_OF_PARAM;
		offset = (int) (((idx-1) - (round * MAX_MARY_SIZE * MAX_NUM_OF_PARAM)) % MAX_MARY_SIZE);
		tmp_ary[p_idx][offset] = runLengAry[idx];
#ifdef DBG_KREE_SHM
		KREE_DEBUG("[%s] tmp_ary[%d][%d] high= 0x%x, low= 0x%x, size= 0x%x\n",
			__func__, p_idx, offset,
			tmp_ary[p_idx][offset].high, tmp_ary[p_idx][offset].low,
			tmp_ary[p_idx][offset].size);
#endif
		tmp_ary_entryNum[p_idx]++;
		idx++;

	} while (idx <= runLeng_arySize);

	KREE_DEBUG("[%s] do the last print (send command).\n", __func__);
	if (tmp_ary_entryNum[0] > 0) {
		for (i = 0; i < MAX_NUM_OF_PARAM; i++) {
			p[i].mem.buffer = tmp_ary[i];
			p[i].mem.size = tmp_ary_entryNum[i] * EACH_MAP_ENTRY_SIZE;
#ifdef DBG_KREE_SHM
			KREE_DEBUG("[%s] [last time] p[i].mem.size = %d [tmp_ary_entryNum =%d]\n",
				__func__, p[i].mem.size, tmp_ary_entryNum[i]);
#endif
		}

#ifdef DBG_KREE_SHM
		print_runlength_arr(p, &prt_id);
#endif
		/* send commnad. */
		KREE_DEBUG("===>  round = %d\n", round);
		ret = send_shm_cmd(round, p, session, numOfPA);
	}

	ret = send_shm_ending_cmd(p, session, numOfPA);
	kfree(runLengAry);

	return ret;
}

TZ_RESULT kree_register_sharedmem(KREE_SESSION_HANDLE session,
					KREE_SHAREDMEM_HANDLE *mem_handle,
					void *start, uint32_t size, void *mapAry)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret = 0;
	int locktry;

	KREE_DEBUG("[%s] kree_register_sharedmem is calling.\n", __func__);

	/** FIXME: mutex should be removed after re-implement sending procedure **/
	do {
		locktry = mutex_lock_interruptible(&shared_mem_mutex);
		if (locktry && locktry != -EINTR) {
			KREE_DEBUG("mutex lock error: %d", locktry);
			return TZ_RESULT_ERROR_GENERIC;
		}
	} while (locktry);

	if (mapAry == NULL)
		ret = kree_register_cont_shm(p, session, start, size);
	else
		ret = kree_register_desc_shm(p, session, start, size, mapAry);

	mutex_unlock(&shared_mem_mutex); /* FIXME: should be removed */

	if (ret != TZ_RESULT_SUCCESS) {
		*mem_handle = 0;
		return ret;
	}
	*mem_handle = p[3].value.a;

	return TZ_RESULT_SUCCESS;
}

TZ_RESULT kree_unregister_sharedmem(KREE_SESSION_HANDLE session,
					KREE_SHAREDMEM_HANDLE mem_handle)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	p[0].value.a = (uint32_t) mem_handle;
	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SHAREDMEM_UNREG,
				  TZ_ParamTypes1(TZPT_VALUE_INPUT), p);
	return ret;
}

/* APIs
*/
TZ_RESULT KREE_RegisterSharedmem(KREE_SESSION_HANDLE session,
					KREE_SHAREDMEM_HANDLE *shm_handle,
					KREE_SHAREDMEM_PARAM *param)
{
	TZ_RESULT ret;

	if (shm_handle == NULL)
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	ret = kree_register_sharedmem(session, shm_handle,
						param->buffer, param->size,
						param->mapAry); /* set 0 for no remap... */
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("KREE_RegisterSharedmem Error: %d\n", ret);
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}

TZ_RESULT KREE_UnregisterSharedmem(KREE_SESSION_HANDLE session,
					KREE_SHAREDMEM_HANDLE shm_handle)
{
	TZ_RESULT ret;

	if ((shm_handle == 0))
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	ret = kree_unregister_sharedmem(session, shm_handle);
	if (ret < 0) {
		KREE_ERR("KREE_UnregisterSharedmem Error: %d\n", ret);
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}

TZ_RESULT KREE_AllocSecuremem(KREE_SESSION_HANDLE session,
				KREE_SECUREMEM_HANDLE *mem_handle,
				uint32_t alignment, uint32_t size)
{
	TZ_RESULT ret;

	ret =
		_allocFunc(TZCMD_MEM_SECUREMEM_ALLOC, session, mem_handle,
			alignment, size, "KREE_AllocSecuremem");

	return ret;
}

TZ_RESULT KREE_ZallocSecuremem(KREE_SESSION_HANDLE session,
				KREE_SECUREMEM_HANDLE *mem_handle,
				uint32_t alignment, uint32_t size)
{
	TZ_RESULT ret;

	ret =
		_allocFunc(TZCMD_MEM_SECUREMEM_ZALLOC, session, mem_handle,
			alignment, size, "KREE_ZallocSecuremem");

	return ret;
}

TZ_RESULT KREE_ZallocSecurememWithTag(KREE_SESSION_HANDLE session,
				KREE_SECUREMEM_HANDLE *mem_handle,
				uint32_t alignment, uint32_t size)
{
	TZ_RESULT ret;

	ret =
		_allocFunc(TZCMD_MEM_SECUREMEM_ZALLOC, session, mem_handle,
			alignment, size, "KREE_ZallocSecurememWithTag");

	return ret;
}

TZ_RESULT KREE_ReferenceSecuremem(KREE_SESSION_HANDLE session,
					KREE_SECUREMEM_HANDLE mem_handle)
{
	TZ_RESULT ret;

	ret =
		_handleOpFunc(TZCMD_MEM_SECUREMEM_REF, session, mem_handle,
				"KREE_ReferenceSecuremem");

	return ret;
}

TZ_RESULT KREE_UnreferenceSecuremem(KREE_SESSION_HANDLE session,
					KREE_SECUREMEM_HANDLE mem_handle)
{
	TZ_RESULT ret;
	uint32_t count = 0;

	ret =
		_handleOpFunc_1(TZCMD_MEM_SECUREMEM_UNREF, session, mem_handle,
				&count, "KREE_UnreferenceSecuremem");
#ifdef DBG_KREE_MEM
	KREE_DEBUG("KREE_UnreferenceSecuremem: count = 0x%x\n", count);
#endif

	return ret;
}

TZ_RESULT KREE_AllocSecurechunkmem(KREE_SESSION_HANDLE session,
					KREE_SECUREMEM_HANDLE *cm_handle,
					uint32_t alignment,
					uint32_t size)
{
	TZ_RESULT ret;

	ret =
		_allocFunc(TZCMD_MEM_SECURECM_ALLOC, session, cm_handle,
			alignment, size, "KREE_AllocSecurechunkmem");

	return ret;
}

TZ_RESULT KREE_ReferenceSecurechunkmem(KREE_SESSION_HANDLE session,
					KREE_SECURECM_HANDLE cm_handle)
{
	TZ_RESULT ret;

	ret =
		_handleOpFunc(TZCMD_MEM_SECURECM_REF, session, cm_handle,
			  "KREE_ReferenceSecurechunkmem");

	return ret;
}

TZ_RESULT KREE_UnreferenceSecurechunkmem(KREE_SESSION_HANDLE session,
					 KREE_SECURECM_HANDLE cm_handle)
{
	TZ_RESULT ret;
	uint32_t count = 0;

	ret =
		_handleOpFunc_1(TZCMD_MEM_SECURECM_UNREF, session, cm_handle,
				&count, "KREE_UnreferenceSecurechunkmem");
#ifdef DBG_KREE_MEM
	KREE_DEBUG("KREE_UnreferenceSecurechunkmem: count = 0x%x\n", count);
#endif

	return ret;
}

TZ_RESULT KREE_ReadSecurechunkmem(KREE_SESSION_HANDLE session, uint32_t offset,
					uint32_t size, void *buffer)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	if ((session == 0) || (size == 0))
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	p[0].value.a = offset;
	p[1].value.a = size;
	p[2].mem.buffer = buffer;
	p[2].mem.size = size;	/* fix me!!!! */
	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SECURECM_READ,
					TZ_ParamTypes3(TZPT_VALUE_INPUT,
							TZPT_VALUE_INPUT,
							TZPT_MEM_OUTPUT),
					p);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("KREE_ReadSecurechunkmem Error: %d\n", ret);
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}

TZ_RESULT KREE_WriteSecurechunkmem(KREE_SESSION_HANDLE session, uint32_t offset,
					uint32_t size, void *buffer)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	if ((session == 0) || (size == 0))
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	p[0].value.a = offset;
	p[1].value.a = size;
	p[2].mem.buffer = buffer;
	p[2].mem.size = size;	/* fix me!!!! */
	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SECURECM_WRITE,
					TZ_ParamTypes3(TZPT_VALUE_INPUT,
							TZPT_VALUE_INPUT,
							TZPT_MEM_INPUT),
					p);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("KREE_WriteSecurechunkmem Error: %d\n", ret);
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}


TZ_RESULT KREE_GetSecurechunkReleaseSize(KREE_SESSION_HANDLE session,
						uint32_t *size)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	ret =
		KREE_TeeServiceCall(session, TZCMD_MEM_SECURECM_RSIZE,
				TZ_ParamTypes1(TZPT_VALUE_OUTPUT), p);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("KREE_GetSecurechunkReleaseSize Error: %d\n",
			ret);
		return ret;
	}

	*size = p[0].value.a;

	return TZ_RESULT_SUCCESS;
}

#if (GZ_API_MAIN_VERSION > 2)
TZ_RESULT KREE_StartSecurechunkmemSvc(KREE_SESSION_HANDLE session,
					unsigned long start_pa, uint32_t size)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	p[0].value.a = start_pa;
	p[1].value.a = size;
	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SECURECM_START,
					TZ_ParamTypes2(TZPT_VALUE_INPUT,
							TZPT_VALUE_INPUT),
					p);
	if (ret != TZ_RESULT_SUCCESS) {
#ifdef DBG_KREE_MEM
		pr_debug("[kree] KREE_StartSecurechunkmemSvc Error: %d\n", ret);
#endif
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}

TZ_RESULT KREE_StopSecurechunkmemSvc(KREE_SESSION_HANDLE session,
					unsigned long *cm_pa, uint32_t *size)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SECURECM_STOP,
					TZ_ParamTypes2(TZPT_VALUE_OUTPUT,
							TZPT_VALUE_OUTPUT),
					p);
	if (ret != TZ_RESULT_SUCCESS) {
#ifdef DBG_KREE_MEM
		pr_debug("[kree] KREE_StopSecurechunkmemSvc Error: %d\n", ret);
#endif
		return ret;
	}

	if (cm_pa != NULL)
		*cm_pa = (unsigned long)p[0].value.a;
	if (size != NULL)
		*size = p[1].value.a;

	return TZ_RESULT_SUCCESS;
}

TZ_RESULT KREE_QuerySecurechunkmem(KREE_SESSION_HANDLE session,
					unsigned long *cm_pa, uint32_t *size)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	ret = KREE_TeeServiceCall(session, TZCMD_MEM_SECURECM_QUERY,
					TZ_ParamTypes2(TZPT_VALUE_OUTPUT,
							TZPT_VALUE_OUTPUT),
					p);
	if (ret != TZ_RESULT_SUCCESS) {
#ifdef DBG_KREE_MEM
		pr_debug("[kree] KREE_QuerySecurechunkmem Error: %d\n", ret);
#endif
		return ret;
	}

	if (cm_pa != NULL)
		*cm_pa = (unsigned long)p[0].value.a;
	if (size != NULL)
		*size = p[1].value.a;

	return TZ_RESULT_SUCCESS;
}
#endif /* GZ_API_MAIN_VERSION */

TZ_RESULT KREE_GetTEETotalSize(KREE_SESSION_HANDLE session, uint32_t *size)
{
	union MTEEC_PARAM p[4];
	TZ_RESULT ret;

	ret = KREE_TeeServiceCall(session, TZCMD_MEM_TOTAL_SIZE,
					TZ_ParamTypes1(TZPT_VALUE_OUTPUT), p);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("KREE_GetTEETotalSize Error: %d\n", ret);
		return ret;
	}

	*size = p[0].value.a;

	return TZ_RESULT_SUCCESS;
}


