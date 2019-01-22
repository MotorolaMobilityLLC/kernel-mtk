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


#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/mutex.h>

#include <gz/kree/system.h>
#include <gz/kree/mem.h>

#include <linux/trusty/trusty_ipc.h>

#include <gz/tz_cross/trustzone.h>
#include <gz/tz_cross/ta_system.h>
#include <gz/kree/system.h>

#ifdef CONFIG_ARM64
#define ARM_SMC_CALLING_CONVENTION
#endif

#define KREE_DEBUG(fmt...) pr_debug("[KREE]"fmt)
#define KREE_ERR(fmt...) pr_debug("[KREE][ERR]"fmt)

#define DYNAMIC_TIPC_LEN

DEFINE_MUTEX(fd_mutex);

#define GZ_SYS_SERVICE_NAME "com.mediatek.geniezone.srv.sys"

#define KREE_SESSION_HANDLE_MAX_SIZE 512
#define KREE_SESSION_HANDLE_SIZE_MASK (KREE_SESSION_HANDLE_MAX_SIZE-1)

static int32_t _sys_service_Fd = -1; /* only need to open sys service once */
static tipc_k_handle _sys_service_h;

static tipc_k_handle _kree_session_handle_pool[KREE_SESSION_HANDLE_MAX_SIZE];
static int32_t _kree_session_handle_idx;

int32_t _setSessionHandle(tipc_k_handle h)
{
	int32_t session;
	int32_t i;

	mutex_lock(&fd_mutex);
	for (i = 0; i < KREE_SESSION_HANDLE_MAX_SIZE; i++) {
		if (_kree_session_handle_pool[_kree_session_handle_idx] == 0)
			break;
		_kree_session_handle_idx = (_kree_session_handle_idx + 1) % KREE_SESSION_HANDLE_MAX_SIZE;
	}
	if (i == KREE_SESSION_HANDLE_MAX_SIZE) {
		KREE_ERR(" %s: can not get empty slot for session!\n", __func__);
		return -1;
	}
	_kree_session_handle_pool[_kree_session_handle_idx] = h;
	session = _kree_session_handle_idx;
	_kree_session_handle_idx = (_kree_session_handle_idx + 1) % KREE_SESSION_HANDLE_MAX_SIZE;
	mutex_unlock(&fd_mutex);

	return session;
}

void _clearSessionHandle(int32_t session)
{
	mutex_lock(&fd_mutex);
	_kree_session_handle_pool[session] = 0;
	mutex_unlock(&fd_mutex);
}


int _getSessionHandle(int32_t session, tipc_k_handle *h)
{
	if (session < 0 || session > KREE_SESSION_HANDLE_MAX_SIZE)
		return -1;
	if (session == KREE_SESSION_HANDLE_MAX_SIZE)
		*h = _sys_service_h;
	else
		*h = _kree_session_handle_pool[session];
	return 0;
}


tipc_k_handle _FdToHandle(int32_t session)
{
	tipc_k_handle h = 0;
	int ret;

	ret = _getSessionHandle(session, &h);
	if (ret < 0)
		KREE_ERR(" %s: can not get seesion handle!\n", __func__);

	return h;
}

int32_t _HandleToFd(tipc_k_handle h)
{
	return _setSessionHandle(h);
}

#define _HandleToChanInfo(x) ((struct tipc_dn_chan *)(x))

static TZ_RESULT KREE_OpenSysFd(void)
{
	TZ_RESULT ret = TZ_RESULT_SUCCESS;

	ret = tipc_k_connect(&_sys_service_h, GZ_SYS_SERVICE_NAME);
	if (ret < 0) {
		KREE_DEBUG("%s: Failed to connect to service, ret = %d\n", __func__, ret);
		return TZ_RESULT_ERROR_COMMUNICATION;
	}
	_sys_service_Fd = KREE_SESSION_HANDLE_MAX_SIZE;

	KREE_DEBUG("===> %s: chan_p = 0x%llx\n", __func__, (uint64_t)_sys_service_h);

	return ret;
}

static TZ_RESULT KREE_OpenFd(const char *port, int32_t *Fd)
{

	tipc_k_handle h = 0;
	TZ_RESULT ret = TZ_RESULT_SUCCESS;
	int32_t tmp;

	*Fd = -1; /* invalid fd */

	if (!port)
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	KREE_DEBUG(" ===> %s: %s.\n", __func__, port);
	ret = tipc_k_connect(&h, port);
	if (ret < 0) {
		KREE_DEBUG("%s: Failed to connect to service, ret = %d\n", __func__, ret);
		return TZ_RESULT_ERROR_COMMUNICATION;
	}
	tmp = _HandleToFd(h);
	if (tmp < 0) {
		KREE_DEBUG("%s: Failed to get session\n", __func__);
		return TZ_RESULT_ERROR_OUT_OF_MEMORY;
	}

	*Fd = tmp;

	KREE_DEBUG("===> KREE_OpenFd: session = %d\n", *Fd);
	KREE_DEBUG("===> KREE_OpenFd: chan_p = 0x%llx\n", (uint64_t)h);

	return ret;
}

static TZ_RESULT KREE_CloseFd(int32_t Fd)
{

	int rc;
	tipc_k_handle h;
	int ret = TZ_RESULT_SUCCESS;

	KREE_DEBUG(" ===> %s: Close FD %u\n", __func__, Fd);

	h = _FdToHandle(Fd); /* verify Fd inside */
	if (!h)
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	rc = tipc_k_disconnect(h);
	if (rc) {
		KREE_ERR("%s: tipc_k_disconnect failed\n", __func__);
		ret = TZ_RESULT_ERROR_COMMUNICATION;
	}

	_clearSessionHandle(Fd);

	return ret;
}

/* GZ client issue command */
int _gz_client_cmd(int32_t Fd, int session, unsigned int cmd, void *param, int param_size)
{
	ssize_t rc;
	tipc_k_handle handle;

	handle = _FdToHandle(Fd);
	if (!handle) {
		KREE_ERR("%s: get tipc handle failed\n", __func__);
		return -1;
	}

	KREE_DEBUG(" ===> %s: command = %d.\n", __func__, cmd);
	KREE_DEBUG(" ===> %s: param_size = %d.\n", __func__, param_size);
	rc = tipc_k_write(handle, param, param_size, O_RDWR);
	KREE_DEBUG(" ===> %s: tipc_k_write rc = %d.\n", __func__, (int)rc);

	return rc;
}

/* GZ client wait command ack and return value */
int _gz_client_wait_ret(int32_t Fd, struct gz_syscall_cmd_param *data)
{
	ssize_t rc;
	tipc_k_handle handle;
	int size;

	handle = _FdToHandle(Fd);
	if (!handle) {
		KREE_ERR("%s: get tipc handle failed\n", __func__);
		return -1;
	}

	rc = tipc_k_read(handle, (void *)data, sizeof(struct gz_syscall_cmd_param), O_RDWR);
	size = data->payload_size;
	KREE_DEBUG(" ===> %s: tipc_k_read(1) rc = %d.\n", __func__, (int)rc);
	KREE_DEBUG(" ===> %s: data payload size = %d.\n", __func__, size);
#if 0
	if (size > GZ_MSG_DATA_MAX_LEN) {
		KREE_ERR("%s: ret payload size(%d) exceed max len(%d)\n", __func__, size, GZ_MSG_DATA_MAX_LEN);
		return -1;
	}
#endif

	/* FIXME!!!: need to check return... */

	return rc;
}

static void recover_64_params(union MTEEC_PARAM *dst, union MTEEC_PARAM *src, uint32_t types)
{
	int i;
	uint32_t type;

	for (i = 0; i < 4; i++) {
		type = TZ_GetParamTypes(types, i);
		if (type == TZPT_MEM_OUTPUT || type == TZPT_MEM_INOUT || type == TZPT_MEM_INPUT) {
			KREE_DEBUG("RP: recover %p to %p(%u)\n", dst[i].mem.buffer, src[i].mem.buffer, src[i].mem.size);
			dst[i].mem.buffer = src[i].mem.buffer;
			dst[i].mem.size = src[i].mem.size;
		}
	}
}

#ifdef GP_PARAM_DEBUG
static void report_param_byte(struct gz_syscall_cmd_param *param)
{
	int i, len;
	char *ptr = (char *)&(param->param);

	len = sizeof(union MTEEC_PARAM) * 4;
	KREE_DEBUG("==== report param byte ====\n");
	KREE_DEBUG("head of param: %p\n", param->param);
	KREE_DEBUG("len = %d\n", len);
	for (i = 0; i < len; i++) {
		KREE_DEBUG("byte[%d]: 0x%x\n", i, *ptr);
		ptr++;
	}
}

static void report_param(struct gz_syscall_cmd_param *param)
{
	int i;
	union MTEEC_PARAM *param_p = param->param;

	KREE_DEBUG("session: 0x%x, command: 0x%x\n", param->handle, param->command);
	KREE_DEBUG("paramTypes: 0x%x\n", param->paramTypes);
	for (i = 0; i < 4; i++)
		KREE_DEBUG("param[%d]: 0x%x, 0x%x\n", i, param_p[i].value.a, param_p[i].value.b);
}
#endif

static int GZ_CopyMemToBuffer(struct gz_syscall_cmd_param *param)
{
	uint32_t type, size, offset = 0;
	int i;
	void *addr;
	void *end_buf = &param->data[GZ_MSG_DATA_MAX_LEN];

	for (i = 0; i < 4; i++) {
		type = TZ_GetParamTypes(param->paramTypes, i);

		if (type == TZPT_MEM_INPUT || type == TZPT_MEM_INOUT) {
			addr = param->param[i].mem.buffer;
			size = param->param[i].mem.size;

			/* check mem addr & size */
			if (!addr) {
				KREE_DEBUG("CMTB: empty gp mem addr\n");
				continue;
			}
			if (!size) {
				KREE_DEBUG("CMTB: mem size=0\n");
				continue;
			}
			/* NOTE: currently a single mem param can use up to GZ_MSG_DATA_MAX_LEN */
			/* Users just need to make sure that total does not exceed the limit */
#if 0
			if (size > GZ_MEM_MAX_LEN) {
				KREE_DEBUG("CMTB: invalid gp mem size: %u\n", size);
				return -1;
			}
#endif

			/* if (offset >= GZ_MSG_DATA_MAX_LEN) { */
			if ((void *)&param->data[offset+size] > end_buf) {
				KREE_ERR("CMTB: ERR: buffer overflow\n");
				return -1;
			}

			KREE_DEBUG("CMTB: cp param %d from %p to %p, size %u\n",
					i, addr, (void *)&param->data[offset], size);
			memcpy((void *)&param->data[offset], addr, size);
#ifdef FIX_MEM_DATA_BUFFER
			offset += GZ_MEM_MAX_LEN;
#else
			offset += size;
#endif
		}
	}
	KREE_DEBUG("CMTB: total %u bytes copied\n", offset);
	return offset;
}

static int GZ_CopyMemFromBuffer(struct gz_syscall_cmd_param *param)
{
	uint32_t type, size, offset = 0;
	int i;
	void *addr;
	void *end_buf = &param->data[GZ_MSG_DATA_MAX_LEN];

	KREE_DEBUG("CMTB: end of buf: %p\n", end_buf);

	for (i = 0; i < 4; i++) {
		type = TZ_GetParamTypes(param->paramTypes, i);

		if (type == TZPT_MEM_OUTPUT || type == TZPT_MEM_INOUT) {
			addr = param->param[i].mem.buffer;
			size = param->param[i].mem.size;

			/* check mem addr & size */
			if (!addr) {
				KREE_DEBUG("CMTB: empty gp mem addr\n");
				continue;
			}
			if (!size) {
				KREE_DEBUG("CMTB: mem size=0\n");
				continue;
			}
			/* NOTE: currently a single mem param can use up to GZ_MSG_DATA_MAX_LEN */
			/* Users just need to make sure that total does not exceed the limit */
#if 0
			if (size > GZ_MEM_MAX_LEN) {
				KREE_DEBUG("CMTB: invalid gp mem size: %u\n", size);
				return -1;
			}
#endif

			/* if (offset >= GZ_MSG_DATA_MAX_LEN) { */
			if ((void *)&param->data[offset+size] > end_buf) {
				KREE_ERR("CMFB: ERR: buffer overflow\n");
				break;
			}

			KREE_DEBUG("CMFB: copy param %d from %p to %p, size %d\n",
					i, (void *)&param->data[offset], addr, size);
			memcpy(addr, (void *)&param->data[offset], size);
#ifdef FIX_MEM_DATA_BUFFER
			offset += GZ_MEM_MAX_LEN;
#else
			offset += size;
#endif
		}
	}
	return offset;
}


void GZ_RewriteParamMemAddr(struct gz_syscall_cmd_param *param)
{
	uint32_t type, size, offset = 0;
	int i;
	void *head = param->data;
	union MTEEC_PARAM *param_p = param->param;

	KREE_DEBUG("RPMA: head of param: %p\n", param_p);

	KREE_DEBUG("RPMA: data buffer head: %p\n", head);
	for (i = 0; i < 4; i++) {
		type = TZ_GetParamTypes(param->paramTypes, i);

		if (type == TZPT_MEM_OUTPUT || type == TZPT_MEM_INOUT) {
			size = param_p[i].mem32.size; /* GZ use mem rather than mem64 */
			KREE_DEBUG("RPMA: param %d mem size: %u\n", i, size);

			param_p[i].mem.buffer = &(param->data[offset]);
			param_p[i].mem.size = size;
			KREE_DEBUG("RPMA: head of param[%d]: %p\n", i, &param_p[i]);
			KREE_DEBUG("RPMA: rewrite param %d mem addr: %p\n", i, param_p[i].mem.buffer);
			KREE_DEBUG("RPMA: rewrite param %d mem size: %u\n", i, param_p[i].mem.size);

			if (offset >= GZ_MSG_DATA_MAX_LEN) {
				KREE_ERR("CMTB: ERR: buffer overflow\n");
				break;
			}
#ifdef FIX_MEM_DATA_BUFFER
			offset += GZ_MEM_MAX_LEN;
#else
			offset += size;
#endif
		}
	}
#ifdef GP_PARAM_DEBUG
	KREE_DEBUG("======== in RPMA =========\n");
	report_param_byte(param);
	KREE_DEBUG("======== leave RPMA =========\n");
#endif
}


void make_64_params_local(union MTEEC_PARAM *dst, uint32_t types)
{
	int i;
	long addr;
	uint32_t type, size;
	uint64_t tmp;
	uint64_t high = 0, low = 0;

	for (i = 0; i < 4; i++) {
		type = TZ_GetParamTypes(types, i);
		if (type == TZPT_MEM_INPUT || type == TZPT_MEM_INOUT || type == TZPT_MEM_OUTPUT) {
			KREE_DEBUG(" make_64_params_local, addr: %p/%u\n", dst[i].mem.buffer, dst[i].mem.size);
			size = dst[i].mem.size;
			addr = (long)dst[i].mem.buffer;
			high = addr;
			high >>= 32;
			high <<= 32;
			low = (addr & 0xffffffff);
			KREE_DEBUG(" make_64_params_local, high/low: 0x%llx/0x%llx\n", high, low);
			tmp = (high | low);

			dst[i].mem64.buffer = tmp;
			dst[i].mem64.size = size;
			KREE_DEBUG(" make_64_params_local, new addr: 0x%llx/%u\n",
					dst[i].mem64.buffer, dst[i].mem64.size);
		}
	}
}

TZ_RESULT _GzCreateSession_body(int32_t Fd, unsigned int cmd, const char *uuid)
{
	TZ_RESULT ret = TZ_RESULT_SUCCESS;
	int rc, len, copied = 0;
	KREE_SESSION_HANDLE session;
	struct gz_syscall_cmd_param *param;
	struct tipc_dn_chan *chan_p;

	if (!uuid)
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	chan_p = _HandleToChanInfo(_FdToHandle(Fd));
	if (!chan_p) {
		KREE_ERR("%s: NULL chan_p, invalid Fd\n", __func__);
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}
	KREE_DEBUG("===> _GzCreateSession_body: chan_p = 0x%llx\n", (uint64_t)chan_p);

	/* connect to sys service */
	if (_sys_service_Fd < 0) {
		KREE_DEBUG("%s: Open sys service fd first.\n", __func__);
		ret = KREE_OpenSysFd();
		if (ret) {
			KREE_ERR("%s: open sys service fd failed, ret = 0x%x\n", __func__, ret);
			return ret;
		}
	}

	/* make parameters */
	param = kmalloc(sizeof(*param), GFP_KERNEL);
	param->command = cmd;
	param->handle = -1;
	param->paramTypes = TZ_ParamTypes2(TZPT_MEM_INPUT, TZPT_VALUE_OUTPUT);

	param->param[0].mem.buffer = (void *)uuid;
	param->param[0].mem.size = (uint32_t)(strlen(uuid)+1);

	copied = GZ_CopyMemToBuffer(param);
	if (copied < 0) {
		KREE_ERR("%s: copy uuid str to param buffer failed\n", __func__);
		ret = TZ_RESULT_ERROR_BAD_PARAMETERS;
		goto create_fail;
	}
	param->payload_size = copied;

	/* Use 64bit struct unifiedly */
	make_64_params_local(param->param, param->paramTypes);

	/* send msg to GZ */
#ifdef DYNAMIC_TIPC_LEN
	len = GZ_MSG_HEADER_LEN + copied;
#else
	len = sizeof(*param);
#endif
	rc = _gz_client_cmd(_sys_service_Fd, 0, cmd, param, len);
	if (rc < 0) {
		KREE_ERR("%s: gz client cmd failed\n", __func__);
		ret = TZ_RESULT_ERROR_COMMUNICATION;
		goto create_fail;
	}
	rc = _gz_client_wait_ret(_sys_service_Fd, param);
	if (rc < 0) {
		KREE_ERR("%s: gz wait ret failed\n", __func__);
		ret = TZ_RESULT_ERROR_COMMUNICATION;
		goto create_fail;
	}

	session = param->param[1].value.a;
	ret = param->param[1].value.b;
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("%s: create session failed at GZ side\n", __func__);
		goto create_fail;
	}
	KREE_DEBUG(" ===> %s: Get session = %d.\n", __func__, (int)session);

	chan_p->session = session;

create_fail:
	kfree(param);
	return ret;
}

TZ_RESULT _GzCloseSession_body(int32_t Fd, unsigned int cmd)
{
	int rc = 0;
	TZ_RESULT ret = TZ_RESULT_SUCCESS;
	KREE_SESSION_HANDLE session;
	struct gz_syscall_cmd_param *param;
	struct tipc_dn_chan *chan_p;

	if (_sys_service_Fd < 0)
		return TZ_RESULT_ERROR_GENERIC;

	/* get session from Fd */
	chan_p = _HandleToChanInfo(_FdToHandle(Fd));
	if (!chan_p) {
		KREE_ERR("%s: NULL chan_p, invalid Fd\n", __func__);
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}

	session = chan_p->session;
	KREE_DEBUG(" ===> %s: session = %d.\n", __func__, (int)session);

	param = kmalloc(sizeof(*param), GFP_KERNEL);
	param->handle = session;
	param->command = cmd;
	param->paramTypes = TZ_ParamTypes1(TZPT_VALUE_INPUT);
	param->payload_size = 0;

	/* close session */
#ifdef DYNAMIC_TIPC_LEN
	rc = _gz_client_cmd(_sys_service_Fd, session, cmd, param, GZ_MSG_HEADER_LEN);
#else
	rc = _gz_client_cmd(_sys_service_Fd, session, cmd, param, sizeof(*param));
#endif
	if (rc < 0) {
		KREE_ERR("%s: gz client cmd failed\n", __func__);
		ret = TZ_RESULT_ERROR_COMMUNICATION;
		goto close_fail;
	}
	_gz_client_wait_ret(_sys_service_Fd, param);

	ret = param->param[0].value.a;
	KREE_DEBUG(" ===> %s: Get retVal = 0x%x.\n", __func__, ret);
	if (ret != TZ_RESULT_SUCCESS)
		KREE_ERR("%s: close session failed at GZ side\n", __func__);

close_fail:
	kfree(param);
	return ret;
}


/* GZ Ree service
*/

enum GZ_ReeServiceCommand {

	REE_SERVICE_CMD_BASE = 0x0,
	REE_SERVICE_CMD_ADD,
	REE_SERVICE_CMD_MUL,
	REE_SERVICE_CMD_END
};


TZ_RESULT _Gz_KreeServiceCall_body(KREE_SESSION_HANDLE handle, uint32_t command,
							  uint32_t paramTypes, union MTEEC_PARAM param[4])
{
/*
 * NOTE: Only support VALUE type gp parameters
 */
	KREE_DEBUG("=====> session %d, command %x\n", handle, command);
	KREE_DEBUG("=====> %s, param values %x %x %x %x\n", __func__
	, param[0].value.a
	, param[1].value.a
	, param[2].value.a
	, param[3].value.a);

	switch (command) {
	case REE_SERVICE_CMD_ADD:
		param[2].value.a = param[0].value.a + param[1].value.a;
		break;

	case REE_SERVICE_CMD_MUL:
		param[2].value.a = param[0].value.a * param[1].value.a;
		break;

	default:
		KREE_DEBUG("[REE service call] %s: invalid command = 0x%x\n", __func__, command);
		break;
	}

	return 0;
}


TZ_RESULT _GzServiceCall_body(int32_t Fd, unsigned int cmd,
		struct gz_syscall_cmd_param *param, union MTEEC_PARAM origin[4])
{
	int ret = TZ_RESULT_SUCCESS;
	int rc, copied = 0;
	KREE_SESSION_HANDLE session;
	struct tipc_dn_chan *chan_p;
	struct gz_syscall_cmd_param *ree_param;

	/* get session from Fd */
	chan_p = _HandleToChanInfo(_FdToHandle(Fd));
	if (!chan_p) {
		KREE_ERR("%s: NULL chan_p, invalid Fd\n", __func__);
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}
	session = chan_p->session;
	KREE_DEBUG("%s: session = %d.\n", __func__, (int)session);

	/* make input */
	param->handle = session;
	copied = GZ_CopyMemToBuffer(param);
	if (copied < 0) {
		KREE_ERR(" invalid input gp params in service call\n");
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}
	param->payload_size = copied;

	make_64_params_local(param->param, param->paramTypes);

	/* send command to establish system session in geniezone */
#ifdef DYNAMIC_TIPC_LEN
	rc = _gz_client_cmd(Fd, session, cmd, param, GZ_MSG_HEADER_LEN + param->payload_size);
#else
	rc = _gz_client_cmd(Fd, session, cmd, param, sizeof(*param));
#endif
	if (rc < 0) {
		KREE_ERR("%s: gz client cmd failed\n", __func__);
		return TZ_RESULT_ERROR_COMMUNICATION;
	}

	ree_param = kmalloc(sizeof(*ree_param), GFP_KERNEL);
	/* keeps serving REE call until ends */
	while (1) {
		rc = _gz_client_wait_ret(Fd, ree_param);
		if (rc < 0) {
			KREE_ERR("%s: wait ret failed(%d)\n", __func__, rc);
		ret = TZ_RESULT_ERROR_COMMUNICATION;
			break;
		}
		KREE_DEBUG("=====> %s, ree service %d\n", __func__, ree_param->ree_service);

	/* TODO: ret = ree_param.ret */

		/* check if REE service */
		if (ree_param->ree_service == 0) {
			KREE_DEBUG("=====> %s, general return!!!!\n", __func__);
			memcpy(param, ree_param, sizeof(*param));
			recover_64_params(param->param, origin, param->paramTypes);

			copied = GZ_CopyMemFromBuffer(param);
			if (copied < 0) {
				KREE_ERR(" invalid output gp params\n");
		ret = TZ_RESULT_ERROR_BAD_PARAMETERS;
			}
			break;
		} else if (ree_param->ree_service != 1) {
			KREE_ERR("invalid ree_service value\n");
			break;
		}

		/* REE service main function */
		GZ_RewriteParamMemAddr(ree_param);
		_Gz_KreeServiceCall_body(ree_param->handle, ree_param->command,
				ree_param->paramTypes, ree_param->param);

		/* return param to GZ */
		copied = GZ_CopyMemToBuffer(param);
		if (copied < 0) {
			KREE_ERR(" invalid gp params\n");
			break;
		}
		param->payload_size = copied;

		make_64_params_local(param->param, param->paramTypes);
#ifdef DYNAMIC_TIPC_LEN
		rc = _gz_client_cmd(Fd, session, 0, ree_param, GZ_MSG_HEADER_LEN + param->payload_size);
#else
		rc = _gz_client_cmd(Fd, session, 0, ree_param, sizeof(*ree_param));
#endif
		if (rc < 0) {
			KREE_ERR("%s: gz client cmd failed\n", __func__);
			ret = TZ_RESULT_ERROR_COMMUNICATION;
			break;
		}

		KREE_DEBUG("=========> %s _GzRounter done\n", __func__);
	}

	kfree(ree_param);
	return ret;
}

TZ_RESULT KREE_CreateSession(const char *ta_uuid, KREE_SESSION_HANDLE *pHandle)
{
	int iret;
	int32_t chan_fd;

	KREE_DEBUG(" ===> %s: Create session start\n", __func__);
	if (!pHandle || !ta_uuid)
		return TZ_RESULT_ERROR_BAD_PARAMETERS;

	/* connect to target service */
	iret = KREE_OpenFd(ta_uuid, &chan_fd);
	if (iret)
		return iret;

	/* send command */
	iret = _GzCreateSession_body(chan_fd, TZCMD_SYS_SESSION_CREATE, ta_uuid);

	*pHandle = chan_fd;

	return iret;
}
EXPORT_SYMBOL(KREE_CreateSession);

TZ_RESULT KREE_CloseSession(KREE_SESSION_HANDLE handle)
{
	int iret;
	int32_t Fd;

	KREE_DEBUG(" ===> %s: Close session ...\n", __func__);
	Fd = handle;
	iret = _GzCloseSession_body(Fd, TZCMD_SYS_SESSION_CLOSE);

	iret |= KREE_CloseFd(Fd);

	return iret;
}
EXPORT_SYMBOL(KREE_CloseSession);

TZ_RESULT KREE_TeeServiceCall(KREE_SESSION_HANDLE handle, uint32_t command,
							  uint32_t paramTypes, union MTEEC_PARAM param[4]) {
	int iret;
	struct gz_syscall_cmd_param *cparam;
	int32_t Fd;

	Fd = handle;

	cparam = kmalloc(sizeof(*cparam), GFP_KERNEL);
	cparam->command = command;
	cparam->paramTypes = paramTypes;
	memcpy(&(cparam->param[0]), param, sizeof(union MTEEC_PARAM)*4);

	KREE_DEBUG(" ===> KREE_TeeServiceCall cmd = %d / %d\n", command, cparam->command);
	iret = _GzServiceCall_body(Fd, command, cparam, param);
	memcpy(param, &(cparam->param[0]), sizeof(union MTEEC_PARAM)*4);
	kfree(cparam);

	return iret;
}
EXPORT_SYMBOL(KREE_TeeServiceCall);

#include "tz_cross/tz_error_strings.h"

const char *TZ_GetErrorString(TZ_RESULT res)
{
	return _TZ_GetErrorString(res);
}
EXPORT_SYMBOL(TZ_GetErrorString);

/***** System Hareware Counter *****/

u64 KREE_GetSystemCnt(void)
{
	u64 cnt = 0;

	cnt = arch_counter_get_cntvct();
	return cnt;
}

u32 KREE_GetSystemCntFrq(void)
{
	u32 freq = 0;

	freq = arch_timer_get_cntfrq();
	return freq;
}
