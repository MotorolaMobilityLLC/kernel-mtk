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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/sysfs.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#include <linux/trusty/trusty_ipc.h>
#include <gz/tz_cross/trustzone.h>
#include <gz/tz_cross/ta_test.h>
#include <gz/tz_cross/ta_system.h>
#include <gz/tz_cross/ta_fbc.h> /* FPS GO cmd header */
#include <gz/kree/system.h>
#include <gz/kree/mem.h>
#include <gz/kree/tz_mod.h>
#include "unittest.h"

#include "gz_main.h"

/* GP memory parameter max len. Needs to be synced with GZ */
#define GP_MEM_MAX_LEN 128

#define KREE_DEBUG(fmt...) pr_debug("[KREE]"fmt)
#define KREE_ERR(fmt...) pr_debug("[KREE][ERR]"fmt)
#define GZTEST_DEBUG 1

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = gz_dev_open,
	.release = gz_dev_release,
	.unlocked_ioctl = gz_ioctl,
#if defined(CONFIG_COMPAT)
	.compat_ioctl = gz_compat_ioctl,
#endif
	.write = gz_dev_write,
	.read = gz_dev_read
};

static struct miscdevice gz_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gz_kree",
	.fops = &fops
};

static int gz_abort_test(void *args);
static int gz_fbc_test(void *argv);
static int gz_tipc_test(void *args);
static int gz_test(void *arg);
static int gz_test_shm(void *arg);
static int gz_abort_test(void *arg);
static int run_internal_test(void *args);
static const char *cases = "0: TIPC\n 1: General KREE API\n"
							"2: Shared memory API\n 3: GZ ramdump\n"
							"4: GZ internal API\n 5: FPS GO";

DEFINE_MUTEX(ut_mutex);

/************* sysfs operations ****************/
static ssize_t show_testcases(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", cases);
}

static ssize_t run_gz_case(struct device *dev, struct device_attribute *attr, const char *buf,
				size_t n)
{
	char tmp[50];
	char c;
	struct task_struct *th;

	strcpy(tmp, buf);
	tmp[n - 1] = '\0';
	c = tmp[0];

	KREE_DEBUG("GZ KREE test: %c\n", c);
	switch (c) {
	case '0':
		KREE_DEBUG("test tipc\n");
		th = kthread_run(gz_tipc_test, NULL, "GZ tipc test");

		break;

	case '1':
		KREE_DEBUG("test general functions\n");
		th = kthread_run(gz_test, NULL, "GZ KREE test");

		break;
	case '2':
		KREE_DEBUG("test shared memory functions\n");
		th = kthread_run(gz_test_shm, NULL, "GZ KREE test");

		break;
	case '3':
		KREE_DEBUG("test GenieZone abort\n");
		th = kthread_run(gz_abort_test, NULL, "GZ KREE test");

		break;
	case '4':
		KREE_DEBUG("test GZ internal API\n");
		th = kthread_run(run_internal_test, NULL, "GZ internal test");

		break;
	case '5':
		KREE_DEBUG("test FPS GO\n");
		th = kthread_run(gz_fbc_test, NULL, "FBC test");

		break;
	default:
		KREE_DEBUG("err: unknown test case\n");

		break;
	}
	return n;
}
DEVICE_ATTR(gz_test, S_IRUGO | S_IWUSR, show_testcases, run_gz_case);

static int create_files(void)
{
	int res;

	res = misc_register(&gz_device);

	if (res != 0) {
		KREE_DEBUG("ERR: misc register failed.");
		return res;
	}
	res = device_create_file(gz_device.this_device, &dev_attr_gz_test);
	if (res != 0) {
		KREE_DEBUG("ERR: create sysfs do_info failed.");
		return res;
	}
	return 0;
}

/*********** test case implementations *************/

static const char mem_srv_name[] = "com.mediatek.geniezone.srv.mem";
static const char echo_srv_name[] = "com.mediatek.geniezone.srv.echo";
#define APP_NAME2 "com.mediatek.gz.srv.sync-ut"

#define TEST_STR_SIZE 512
static char buf1[TEST_STR_SIZE];
static char buf2[TEST_STR_SIZE];

INIT_UNITTESTS;

static void test_hw_cnt(void)
{
	u32 f;
	u64 cnt;

	TEST_BEGIN("hardware counter");

	f = KREE_GetSystemCntFrq();
	KREE_DEBUG("KREE_GetSystemCntFreq: %u\n", f);
	CHECK_NEQ(0, f, "MTEE_GetSystemCntFreq");

	cnt = KREE_GetSystemCnt();
	KREE_DEBUG("KREE_GetSystemCnt: %llu\n", cnt);
	CHECK_NEQ(0, cnt, "MTEE_GetSystemCnt");

	TEST_END;
}

static int check_gp_inout_mem(char *buffer)
{
	int i;

	for (i = 0; i < TEST_STR_SIZE; i++) {
		if (i%3) {
			if (buffer[i] != 'c')
				return 1;
		} else {
			if (buffer[i] != 'd')
				return 1;
		}
	}
	return 0;
}

#define FBC_SERV_NAME "com.mediatek.fbc.main"
int gz_fbc_test(void *argv)
{
	TZ_RESULT ret;
	KREE_SESSION_HANDLE session;
	union MTEEC_PARAM param[4];
	uint32_t types;

	ret = KREE_CreateSession(FBC_SERV_NAME, &session);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_DEBUG("Create session Error: handle 0x%x, ret %d\n", (uint32_t)session, ret);
		return 0;
	}

	types = TZ_ParamTypes1(TZPT_VALUE_INPUT);
	param[0].value.a = 999;

	ret = KREE_TeeServiceCall(session, GZ_CMD_FBC_RUN1, types, param);
	if (ret != TZ_RESULT_SUCCESS)
		KREE_DEBUG("service call 1 Error: handle 0x%x, ret %d\n", (uint32_t)session, ret);

	ret = KREE_TeeServiceCall(session, GZ_CMD_FBC_RUN2, types, param);
	if (ret != TZ_RESULT_SUCCESS)
		KREE_DEBUG("service call 2 Error: handle 0x%x, ret %d\n", (uint32_t)session, ret);

	ret = KREE_CloseSession(session);
	if (ret != TZ_RESULT_SUCCESS)
		KREE_DEBUG("Close session Error: handle 0x%x, ret %d\n", (uint32_t)session, ret);
	else
		KREE_DEBUG("test FPS GO done\n");

	return 0;
}

static int tipc_test_send(tipc_k_handle handle, void *param, int param_size)
{
	ssize_t rc;

	KREE_DEBUG(" ===> %s: param_size = %d.\n", __func__, param_size);
	rc = tipc_k_write(handle, param, param_size, O_RDWR);
	KREE_DEBUG(" ===> %s: tipc_k_write rc = %d.\n", __func__, (int)rc);

	return rc;
}

static int tipc_test_rcv(tipc_k_handle handle, void *data, size_t len)
{
	ssize_t rc;

	rc = tipc_k_read(handle, (void *)data, len, O_RDWR);
	KREE_DEBUG(" ===> %s: tipc_k_read(1) rc = %d.\n", __func__, (int)rc);

	return rc;
}

#define TIPC_TEST_SRV "com.android.ipc-unittest.srv.echo"
int gz_tipc_test(void *args)
{
	int i, rc;
	tipc_k_handle h = 0;

	TEST_BEGIN("tipc basic test");
	RESET_UNITTESTS;

	mutex_lock(&ut_mutex);
	KREE_DEBUG(" ===> %s: test begin\n", __func__);
	/* init test data */
	buf1[0] = buf1[1] = 'Q';
	buf1[2] = '\0';
	for (i = 0; i < TEST_STR_SIZE; i++)
		buf2[i] = 'c';

	KREE_DEBUG(" ===> %s: %s\n", __func__, TIPC_TEST_SRV);
	rc = tipc_k_connect(&h, TIPC_TEST_SRV);
	CHECK_EQ(0, rc, "connect");

	rc = tipc_test_send(h, buf1, sizeof(buf1));
	CHECK_GT_ZERO(rc, "send 1");
	rc = tipc_test_rcv(h, buf1, sizeof(buf1));
	CHECK_GT_ZERO(rc, "rcv 1");

	rc = tipc_test_send(h, buf2, sizeof(buf2));
	CHECK_GT_ZERO(rc, "send 2");
	rc = tipc_test_rcv(h, buf1, sizeof(buf2));
	CHECK_GT_ZERO(rc, "rcv 2");

	rc = tipc_k_disconnect(h);
	CHECK_EQ(0, rc, "disconnect");

	mutex_unlock(&ut_mutex);
	TEST_END;
	REPORT_UNITTESTS;

	return 0;
}

static void test_gz_syscall(void)
{
	int i, tmp;
	TZ_RESULT ret;
	KREE_SESSION_HANDLE session, session2;
	union MTEEC_PARAM param[4];
	uint32_t types;

	TEST_BEGIN("basic session & syscall");
	mutex_lock(&ut_mutex);

	ret = KREE_CreateSession(echo_srv_name, &session);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "create echo srv session");

	/* connect to unknown server */
	ret = KREE_CreateSession("unknown.server", &session2);
	CHECK_EQ(TZ_RESULT_ERROR_COMMUNICATION, ret, "connect to unknown server");

	/* null checking */
	ret = KREE_CreateSession(echo_srv_name, NULL);
	CHECK_EQ(TZ_RESULT_ERROR_BAD_PARAMETERS, ret, "create session null checking");

	/* connect to the same server multiple times*/
	ret = KREE_CreateSession(echo_srv_name, &session2);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "create echo srv session 2");

	/**** Service call test ****/
	for (i = 0; i < TEST_STR_SIZE; i++)
		buf2[i] = 'c';

	param[0].value.a = 0x1230;
	param[1].mem.buffer = (void *)buf1;
	param[1].mem.size = TEST_STR_SIZE;
	param[2].mem.buffer = (void *)buf2;
	param[2].mem.size = TEST_STR_SIZE;

	/* memory boundary case parameters */
	types = TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_MEM_OUTPUT, TZPT_MEM_INOUT, TZPT_VALUE_OUTPUT);
	ret = KREE_TeeServiceCall(session, TZCMD_TEST_SYSCALL, types, param);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "test TA syscall");

	if (ret != TZ_RESULT_SUCCESS) {
		KREE_DEBUG("KREE_TeeServiceCall Error: handle 0x%x, ret %d\n", (uint32_t)session, ret);
	} else {
		tmp = strcmp((char *)param[1].mem.buffer, "sample data 1!!");
		CHECK_EQ(0, tmp, "check gp param: mem output");
		tmp = check_gp_inout_mem(buf2);
		CHECK_EQ(0, tmp, "check gp param: mem inout");
		CHECK_EQ(99, param[3].value.a, "check gp param: value output");
	}
	ret = KREE_CloseSession(session);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "close echo srv session");
	ret = KREE_CloseSession(session2);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "close echo srv session 2");

	mutex_unlock(&ut_mutex);
	TEST_END;
}

static void test_gz_mem_api(void)
{
	TZ_RESULT ret;
	KREE_SESSION_HANDLE mem_session;
	KREE_SECUREMEM_HANDLE mem_handle[4];

	TEST_BEGIN("mem service & mem APIs");

	ret = KREE_CreateSession(mem_srv_name, &mem_session);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "create mem srv session");

	/**** Memory ****/
	KREE_DEBUG("[GZTEST] memory APIs...\n");

	ret = KREE_AllocSecuremem(mem_session, &mem_handle[0], 0, 128);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "alloc secure mem 128");
	KREE_DEBUG("[GZTEST]KREE_AllocSecuremem handle = %d.\n", mem_handle[0]);

	ret = KREE_AllocSecuremem(mem_session, &mem_handle[1], 0, 512);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "alloc secure mem 512");
	KREE_DEBUG("[GZTEST]KREE_AllocSecuremem handle = %d.\n", mem_handle[1]);

	ret = KREE_AllocSecuremem(mem_session, &mem_handle[2], 0, 1024);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "alloc secure mem 1024");
	KREE_DEBUG("[GZTEST]KREE_AllocSecuremem handle = %d.\n", mem_handle[2]);

	ret = KREE_ZallocSecurememWithTag(mem_session, &mem_handle[3], 0, 2048);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "zero alloc secure mem 2048");
	KREE_DEBUG("[GZTEST]KREE_ZallocSecuremem handle = %d.\n", mem_handle[3]);


	ret = KREE_ReferenceSecuremem(mem_session, mem_handle[0]);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "reference secure mem 1");
	ret = KREE_ReferenceSecuremem(mem_session, mem_handle[0]);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "reference secure mem 2");

	ret = KREE_UnreferenceSecuremem(mem_session, mem_handle[0]);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "unreference secure mem 1");
	ret = KREE_UnreferenceSecuremem(mem_session, mem_handle[0]);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "unreference secure mem 2");
	ret = KREE_UnreferenceSecuremem(mem_session, mem_handle[0]);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "unreference secure mem 3");

	ret = KREE_CloseSession(mem_session);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "close mem srv session");

	TEST_END;
}

static int sync_test(void)
{
	TZ_RESULT ret;
	KREE_SESSION_HANDLE sessionHandle, sessionHandle2;

	union MTEEC_PARAM param[4];
	uint32_t types;

	TEST_BEGIN("TA sync test");

	/* Connect to echo service */
	ret = KREE_CreateSession(echo_srv_name, &sessionHandle);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "create echo srv session");
	CHECK_GT_ZERO(sessionHandle, "check echo srv session value");

	/* Connect to sync-ut service */
	ret = KREE_CreateSession(APP_NAME2, &sessionHandle2);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "create sync-ut srv session");
	CHECK_GT_ZERO(sessionHandle, "check echo sync-ut session value");

	/* Request mutex handle from TA1 */
	types = TZ_ParamTypes2(TZPT_VALUE_OUTPUT, TZPT_VALUE_OUTPUT);
	ret = KREE_TeeServiceCall(sessionHandle, TZCMD_GET_MUTEX, types, param);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "get mutex handle from TA1");

	CHECK_GT_ZERO(param[0].value.a, "check mutex value");

	/* Send mutex handle to TA2 */
	types = TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
	ret = KREE_TeeServiceCall(sessionHandle2, TZCMD_SEND_MUTEX, types, param);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "send mutex handle to TA2");

	/* start mutex test */
	ret = KREE_TeeServiceCall(sessionHandle, TZCMD_TEST_MUTEX, types, param);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "send start cmd to TA1");
	ret = KREE_TeeServiceCall(sessionHandle2, TZCMD_TEST_MUTEX, types, param);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "send start cmd to TA2");

	ret = KREE_CloseSession(sessionHandle);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "close echo srv session");
	ret = KREE_CloseSession(sessionHandle2);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "close echo sync-ut session");

	TEST_END;
	return 0;
}

static int gz_test(void *arg)
{
	KREE_DEBUG("[GZTEST]====> GenieZone Linux kernel test\n");

	RESET_UNITTESTS;

	test_gz_syscall();
	test_gz_mem_api();
	sync_test();
	test_hw_cnt();

	REPORT_UNITTESTS;

	return 0;
}

static int run_internal_test(void *args)
{
	KREE_SESSION_HANDLE ut_session_handle;
	TZ_RESULT ret;
	union MTEEC_PARAM param[4];
	uint32_t paramTypes;

	KREE_DEBUG("[%s] ====> Call KREE_CreateSession().\n", __func__);

	ret = KREE_CreateSession(echo_srv_name, &ut_session_handle);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_CreateSession() Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	paramTypes = TZ_ParamTypes3(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_OUTPUT);
	ret = KREE_TeeServiceCall(ut_session_handle, 0x9999, paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] ====> KREE_TeeServiceCall()[0x9999] Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	ret = KREE_CloseSession(ut_session_handle);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] ====> KREE_CloseSession() Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	return 0;
}

static int shm_mem_service_process(KREE_SHAREDMEM_PARAM *shm_param, int numOfPA)
{
	KREE_SESSION_HANDLE shm_session;
	KREE_SESSION_HANDLE echo_session;
	TZ_RESULT ret;
	union MTEEC_PARAM param[4];
	KREE_SHAREDMEM_HANDLE shm_handle;
	uint32_t paramTypes;


	if (numOfPA <= 0)  {
		KREE_DEBUG("[%s]====> numOfPA <= 0 ==>[stop!].\n", __func__);
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}

	KREE_DEBUG("[%s] ====> Call KREE_CreateSession().\n", __func__);
	ret = KREE_CreateSession(mem_srv_name, &shm_session);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_CreateSession(): shm_session returns Fail. [Stop!]. ret=0x%x\n", __func__, ret);
		return ret;
	}

	KREE_DEBUG("[%s] ====> Call KREE_RegisterSharedmem().\n", __func__);
	ret = KREE_RegisterSharedmem(shm_session, &shm_handle, shm_param);
	KREE_DEBUG("[%s] ====> shm_handle=%d\n", __func__, shm_handle);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_RegisterSharedmem() returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	/* Connect to echo service */
	KREE_DEBUG("[%s] ====> Create echo server.\n", __func__);
	ret = KREE_CreateSession(echo_srv_name, &echo_session);
	KREE_DEBUG("[%s] ===> get echo_session = %d.\n", __func__, echo_session);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_CreateSession():echo_session returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	KREE_DEBUG("[%s]======> Call echo service------1.\n", __func__);
	param[0].value.a = shm_handle;
	param[1].value.a = numOfPA;
	paramTypes = TZ_ParamTypes3(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_OUTPUT);
	ret = KREE_TeeServiceCall(echo_session, 0x5588, paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_TeeServiceCall():echo_session[0x5588] returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	KREE_DEBUG("[%s]======> Call echo service------2.\n", __func__);
	param[0].value.a = shm_handle;
	param[1].value.a = numOfPA;
	paramTypes = TZ_ParamTypes3(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_OUTPUT);
	ret = KREE_TeeServiceCall(echo_session, 0x5588, paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_TeeServiceCall():echo_session[0x5588] returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	ret = KREE_CloseSession(echo_session);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_CloseSession():echo_session returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}


	KREE_DEBUG("[%s] ====> KREE_UnregisterSharedmem() is calling.\n", __func__);
	ret = KREE_UnregisterSharedmem(shm_session, shm_handle);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_UnregisterSharedmem() returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	ret = KREE_CloseSession(shm_session);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_CloseSession():shm_session returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}

static int _registerSharedmem_body(KREE_SESSION_HANDLE shm_session,
		KREE_SHAREDMEM_PARAM *shm_param, int numOfPA, KREE_SHAREDMEM_HANDLE *shm_handle)
{
	TZ_RESULT ret;

	if (numOfPA <= 0)  {
		KREE_DEBUG("[%s]====> numOfPA <= 0 ==>[stop!].\n", __func__);
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}

	KREE_DEBUG("[%s] ====> Call KREE_RegisterSharedmem().\n", __func__);
	ret = KREE_RegisterSharedmem(shm_session, shm_handle, shm_param);
	KREE_DEBUG("[%s] ====> shm_handle=%d\n", __func__, *shm_handle);
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] KREE_RegisterSharedmem() returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	return TZ_RESULT_SUCCESS;
}

static uint64_t init_shm_test(char **buf_p, int size)
{
	int i;
	uint64_t pa = 0;
	char *buf;
	int stat[2]; /* 0: for a; 1: for b; for test stat */

	TEST_BEGIN("init shared memory test");

	*buf_p = kmalloc(size, GFP_KERNEL);
	buf = *buf_p;
	if (!buf)  {
		KREE_ERR("[%s] ====> kmalloc Fail.\n", __func__);
		return 0;
	}

	for (i = 0; i < (size-1); i++)
		buf[i] = 'a';			/* set string:a */
	buf[i] = '\0';

	/* init */
	for (i = 0; i < 2; i++)
		stat[i] = 0;

	/* calculate stat info: char a, b */
	for (i = 0; i < (size); i++) {
		if (buf[i] == 'a')
			stat[0]++;
		else if (buf[i] == 'b')
			stat[1]++;
	}

	pa = (uint64_t) virt_to_phys((void *)buf);

	/* test string at Linux kernel */
	KREE_DEBUG("[%s] ====> buf(%llx) kmalloc size=%d, PA=%llx\n",
			__func__, (uint64_t) buf, size, pa);
	CHECK_EQ((size-1), stat[0], "input string a");
	CHECK_EQ(0, stat[1], "input string b");
	TEST_END;
	return pa;
}

static void verify_shm_result(char *buf, int size)
{
	int i;
	int stat[2]; /* 0: for a; 1: for b; for test stat */

	TEST_BEGIN("verify shared memory");

	/* == verify results == */
	for (i = 0; i < 2; i++)
		stat[i] = 0;

	/* calculate stat info: char a, b */
	for (i = 0; i < (size); i++) {
		if (buf[i] == 'a')
			stat[0]++;
		else if (buf[i] == 'b')
			stat[1]++;
	}

	CHECK_EQ(0, stat[0], "input string a");
	CHECK_EQ((size-1), stat[1], "input string b");

	TEST_END;
}

static int gz_test_shm_case1(void *arg)
{
	uint64_t pa;
	char *buf = NULL;
	int numOfPA = 0;
	int size = 4272;
	TZ_RESULT ret;
	KREE_SHAREDMEM_PARAM shm_param;

	TEST_BEGIN("====> shared memory test [Test Case 1]. ");
	RESET_UNITTESTS;

	KREE_DEBUG("[%s] ====> GenieZone KREE test shm 1(continuous pages)\n", __func__);
	KREE_DEBUG("[%s] ====> countinuous pages is processing.\n", __func__);

	numOfPA = size / PAGE_SIZE;
	if ((size % PAGE_SIZE) != 0)
		numOfPA++;

	pa = init_shm_test(&buf, size);
	if (!buf) {
		KREE_ERR("[%s] ====> kmalloc Fail! [ buf, Return null. Stop!].\n", __func__);
		goto shm_out;
	}

	KREE_DEBUG("[%s] ====> kmalloc size=%d, &buf=%llx, PA=%llx, numOfPA=%d\n",
			__func__, size, (uint64_t) buf, pa, numOfPA);

	shm_param.buffer = (void *)pa;
	shm_param.size = size;

	/* case 1: continuous pages */
	shm_param.mapAry = NULL;

	ret = shm_mem_service_process(&shm_param, numOfPA);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "shared memory process sys call. ");
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] ====> shm_mem_service_process() returns Fail. [Stop!]. ret=0x%x\n",
				__func__, ret);
		goto shm_out;
	}

	verify_shm_result(buf, size);

shm_out:
	KREE_DEBUG("[%s] ====> test shared memory ends (test case:1[continuous pages]) ======.\n", __func__);

	kfree(buf);

	TEST_END;
	REPORT_UNITTESTS;
	return TZ_RESULT_SUCCESS;
}

static void fill_map_arr(uint64_t *mapAry, int *mapAry_idx, uint64_t pa, int testArySize)
{
	int idx;

	for (idx = 0; idx < testArySize; idx++) {
		mapAry[*mapAry_idx] = (uint64_t) ((uint64_t) pa + (uint64_t) (idx) * PAGE_SIZE);
		(*mapAry_idx)++;
	}
}

static int gz_test_shm_case2(void *arg)
{
	TZ_RESULT ret;
	KREE_SHAREDMEM_PARAM shm_param;
	int size = 2*4*1024;	/* test str1: 8KB */
	char *buf = NULL;
	uint64_t pa;

	int size2 = 1*4*1024;	/* test str2: 4KB */
	char *buf2 = NULL;
	uint64_t pa2;

	int size3 = 3*4*1024;	/* test str3: 12KB */
	char *buf3 = NULL;
	uint64_t pa3;

	int mapAry_idx = 0;

	int testArySize = size / PAGE_SIZE;
	int testArySize2 = size2 / PAGE_SIZE;
	int testArySize3 = size3 / PAGE_SIZE;

	int numOfPA = (testArySize + testArySize2 + testArySize3);
	uint64_t mapAry[testArySize + testArySize2 + testArySize3 + 1];

	TEST_BEGIN("====> shared memory test [Test Case 2]. ");
	RESET_UNITTESTS;

	KREE_DEBUG("[%s] ====> GenieZone Linux kernel test shm starts.\n", __func__);

	KREE_DEBUG("[%s]=====> discountinuous pages is processing.\n", __func__);

	/* init test string 1 */
	pa = init_shm_test(&buf, size);
	if (!buf)  {
		KREE_ERR("[%s] ====> kmalloc Fail! [ buf, Return null. Stop!].\n", __func__);
		goto shm_out;
	}

	/* init test string 2 */
	pa2 = init_shm_test(&buf2, size2);
	if (!buf2)  {
		KREE_ERR("[%s] ====> kmalloc Fail! [ buf2, Return null. Stop!].\n", __func__);
		goto shm_out;
	}

	/* init test string 3 */
	pa3 = init_shm_test(&buf3, size3);
	if (!buf3)  {
		KREE_ERR("[%s] ====> kmalloc Fail! [ buf3, Return null. Stop!].\n", __func__);
		goto shm_out;
	}

	/* put PA lists in mapAry: mapAry[0]: # of PAs, mapAry[1]=PA1, mapAry[2]= PA2, ... */
	mapAry_idx = 0;
	mapAry[mapAry_idx++] = (uint64_t) (numOfPA);		/* # of PAs */
	fill_map_arr(mapAry, &mapAry_idx, pa, testArySize);
	fill_map_arr(mapAry, &mapAry_idx, pa2, testArySize2);
	fill_map_arr(mapAry, &mapAry_idx, pa3, testArySize3);

	/* case 2: discountinuous pages */
	shm_param.buffer = NULL;
	shm_param.size = 0;
	shm_param.mapAry = (void *) mapAry;

	ret = shm_mem_service_process(&shm_param, numOfPA);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "shared memory process sys call. ");
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] ====> shm_mem_service_process() returns Fail. [Stop!]. ret=0x%x\n", __func__, ret);
		goto shm_out;
	}

	/* Input strings are updated in GZ echo server. show the updated string stat info */
	/* verify update data  (all 'a' are replaced with 'b') */
	verify_shm_result(buf, size);
	verify_shm_result(buf2, size2);
	verify_shm_result(buf3, size3);

shm_out:

	KREE_DEBUG("[%s] ====> test [case 2] shared memory ends ======.\n", __func__);

	kfree(buf);
	kfree(buf2);
	kfree(buf3);

	TEST_END;
	REPORT_UNITTESTS;
	return TZ_RESULT_SUCCESS;
}

static int gz_test_shm_case3(void *arg)
{
	int numOfCPGroup = 150;  /* total: 70,120,150 (all OK) PAs */
	int numOfPageInAGroup = 1;	/* 1, 2 (all OK) */

	uint64_t startPA = 0x0000000;

	int m = 0, n = 0;
	int i = 0;
	int str_cnt = 0;
	int stat[2]; /* 0: for a; 1: for b; */
	int total_PAs = numOfCPGroup * numOfPageInAGroup;  /* 70*2 = 140 */

	char *testAry[numOfCPGroup];
	uint64_t paAryOftestAry[numOfCPGroup];

	TZ_RESULT ret;
	KREE_SHAREDMEM_PARAM shm_param;

	uint64_t *mapAry = kmalloc((total_PAs + 1) * sizeof(uint64_t), GFP_KERNEL);
	int mapAry_idx = 0;
	int numOfChar = (int) ((numOfCPGroup * numOfPageInAGroup) * PAGE_SIZE - numOfCPGroup);

	TEST_BEGIN("====> shared memory test [Test Case 3].");
	RESET_UNITTESTS;

	for (m = 0; m < numOfCPGroup; m++) {
		testAry[m] = kmalloc((numOfPageInAGroup * PAGE_SIZE), GFP_KERNEL);
		if (!testAry[m])
			KREE_ERR("[%s] ====> kmalloc Fail! [ buf, Return null. Stop!].\n", __func__);
		paAryOftestAry[m] = (uint64_t) virt_to_phys((void *)testAry[m]);
	}

	mapAry[mapAry_idx++] = total_PAs;
	KREE_DEBUG("===> test total # of PAs: %d\n", total_PAs);

	for (m = 1; m < (numOfCPGroup + 1); m++) {		/* 1 to 70 groups */
		for (n = 0; n < numOfPageInAGroup; n++) {	/* 2 PAs in a group */
			startPA = paAryOftestAry[m-1];
			mapAry[mapAry_idx++] =	(uint64_t) ((uint64_t) startPA + (uint64_t) (n) * PAGE_SIZE);
		}
	}

	KREE_DEBUG("===> mapAry[0] = numOfPA= 0x%x\n",  (uint32_t) mapAry[0]);

	/* init string value */
	for (m = 0; m < numOfCPGroup; m++) {
		for (i = 0; i < ((numOfPageInAGroup * PAGE_SIZE)-1); i++) {
			testAry[m][i] = 'a';
			str_cnt++;
		}
		testAry[m][i] = '\0';
	}

	KREE_DEBUG("[%s]====> input string in Linux #of [a] (str_cnt) = %5d\n", __func__, str_cnt);

	/* verify input data */
	for (i = 0; i < 2; i++)
		stat[i] = 0;

	for (m = 0; m < numOfCPGroup; m++) {
		for (i = 0; i < (numOfPageInAGroup * PAGE_SIZE); i++) {
			if (testAry[m][i] == 'a')
				stat[0]++;
			else if (testAry[m][i] == 'b')
				stat[1]++;
		}
	}

	KREE_DEBUG("[%s]======> input string: [original]'s char stat:\n", __func__);
	KREE_DEBUG("[%s]====> input string in Linux #of [a] = %5d  (=%5d ?) -> [%s]\n",
			__func__, stat[0], numOfChar, (stat[0] == numOfChar)	? "Successful" : "Fail");
	KREE_DEBUG("[%s]====> input string in Linux #of [b] = %5d  (=%5d ?) -> [%s]\n",
			__func__, stat[1], 0, (stat[1] == 0)	? "Successful" : "Fail");

	CHECK_EQ(numOfChar, stat[0], "input string a");
	CHECK_EQ(0, stat[1], "input string b");

	/* case 3: discountinuous pages */
	shm_param.buffer = NULL;
	shm_param.size = 0;
	shm_param.mapAry = (void *) mapAry;

	ret = shm_mem_service_process(&shm_param, total_PAs);
	CHECK_EQ(TZ_RESULT_SUCCESS, ret, "shared memory process sys call. ");
	if (ret != TZ_RESULT_SUCCESS) {
		KREE_ERR("[%s] ====> shm_mem_service_process() returns Fail. [Stop!]. ret=0x%x\n", __func__, ret);
		goto shm_out;
	}


	/* Input strings are updated in GZ echo server. show the updated string stat info */
	/* verify update data  (all 'a' are replaced with 'b') */
	for (i = 0; i < 2; i++)
		stat[i] = 0;

	for (m = 0; m < numOfCPGroup; m++) {
		for (i = 0; i < (numOfPageInAGroup * PAGE_SIZE); i++) {
			if (testAry[m][i] == 'a')
				stat[0]++;
			else if (testAry[m][i] == 'b')
				stat[1]++;
		}
	}

	KREE_DEBUG("[%s]======>  updated string from GZ's echo server [upated: a-->b] 's char stat:\n", __func__);
	KREE_DEBUG("[%s]====> input string in Linux #of [a] = %5d  (=%5d ?) -> [%s]\n",
		__func__, stat[0], 0, (stat[0] == 0)	? "Successful" : "Fail");
	KREE_DEBUG("[%s]====> input string in Linux #of [b] = %5d  (=%5d ?) -> [%s]\n",
		__func__, stat[1], numOfChar,
		(stat[1] == ((numOfCPGroup * numOfPageInAGroup)*PAGE_SIZE - numOfCPGroup)) ? "Successful" : "Fail");

	CHECK_EQ(0, stat[0], "input string a");
	CHECK_EQ(numOfChar, stat[1], "input string b");

shm_out:
	KREE_DEBUG("[%s] ====> test [case 3] shared memory ends ======.\n", __func__);

	for (m = 0; m < numOfCPGroup; m++)
		kfree(testAry[m]);

	TEST_END;
	REPORT_UNITTESTS;
	return TZ_RESULT_SUCCESS;
}


static int gz_test_shm(void *arg)
{
	gz_test_shm_case1(arg);

	gz_test_shm_case2(arg);

	gz_test_shm_case3(arg);

	return TZ_RESULT_SUCCESS;
}

static int gz_abort_test(void *args)
{
	TZ_RESULT ret;
	KREE_SESSION_HANDLE session;
	union MTEEC_PARAM param[4];
	uint32_t types;

	ret = KREE_CreateSession(echo_srv_name, &session);
	/**** Service call test ****/
	param[0].value.a = 0x1230;
	/* memory boundary case parameters */
	types = TZ_ParamTypes1(TZPT_VALUE_INPUT);
	ret = KREE_TeeServiceCall(session, TZCMD_ABORT_TEST, types, param);

	if (ret != TZ_RESULT_SUCCESS)
		KREE_DEBUG("KREE_TeeServiceCall Error: handle 0x%x, ret %d\n", (uint32_t)session, ret);
	ret = KREE_CloseSession(session);

	return 0;
}

/************* kernel module file ops (dummy) ****************/
struct UREE_SHAREDMEM_PARAM {
	uint32_t buffer; /* FIXME: userspace void* is 32-bit */
	uint32_t size;
	uint32_t mapAry;
};

struct UREE_SHAREDMEM_PARAM_US {
	uint64_t buffer; /* FIXME: userspace void* is 32-bit */
	uint32_t size;
};

struct user_shm_param {
	uint32_t session;
	uint32_t shm_handle;
	struct UREE_SHAREDMEM_PARAM_US param;
};

static int gz_dev_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int gz_dev_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static ssize_t gz_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	*pos = 0;
	return 0;
}

static ssize_t gz_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	return count;
}

TZ_RESULT get_US_PAMapAry(struct user_shm_param *shm_data, KREE_SHAREDMEM_PARAM *shm_param,
		int *numOfPA, struct MTIOMMU_PIN_RANGE_T *pin, uint64_t *map_p)
{
	unsigned long cret;
	struct page **page;
	int i;
	unsigned long *pfns;

	KREE_DEBUG("====> [%s] get_US_PAMapAry is calling.\n", __func__);
	KREE_DEBUG("====> session: %u, shm_handle: %u, param.size: %u, param.buffer: 0x%llx\n",
			(*shm_data).session, (*shm_data).shm_handle, (*shm_data).param.size, (*shm_data).param.buffer);

	if (((*shm_data).param.size <= 0) || (!(*shm_data).param.buffer)) {
		KREE_DEBUG("[%s]====> param.size <= 0 OR !param.buffer ==>[stop!].\n", __func__);
		return TZ_RESULT_ERROR_BAD_PARAMETERS;
	}

/*
 * map pages
 */
	/*
	 * 1. get user pages
	 * note: 'pin' resource need to keep for unregister.
	 * It will be freed after unregisted.
	 */

	pin = kzalloc(sizeof(struct MTIOMMU_PIN_RANGE_T), GFP_KERNEL);
	if (pin == NULL) {
		KREE_DEBUG("[%s]====> pin is null.\n", __func__);
		goto us_map_fail;
	}
	cret = _map_user_pages(pin, (unsigned long)(*shm_data).param.buffer, (*shm_data).param.size, 0);

	if (cret != 0) {
		KREE_DEBUG("[%s]====> _map_user_pages fail. map user pages = 0x%x\n", __func__, (uint32_t) cret);
		goto us_map_fail;
	}
	/* 2. build PA table */
	map_p = kzalloc(sizeof(uint64_t) * (pin->nrPages + 1), GFP_KERNEL);
	if (map_p == NULL) {
		KREE_DEBUG("[%s]====> map_p is null.\n", __func__);
		goto us_map_fail;
	}
	map_p[0] = pin->nrPages;
	if (pin->isPage) {
		page = (struct page **)pin->pageArray;
		for (i = 0; i < pin->nrPages; i++)
			map_p[1 + i] = (uint64_t) PFN_PHYS(page_to_pfn(page[i])); /* PA */
	} else { /* pfn */
		pfns = (unsigned long *)pin->pageArray;
		for (i = 0; i < pin->nrPages; i++)
			map_p[1 + i] =  (uint64_t) PFN_PHYS(pfns[i]); /* get PA */
	}

	/* init register shared mem params */
	(*shm_param).buffer = NULL;
	(*shm_param).size = 0;
	(*shm_param).mapAry = (void *) map_p;

	*numOfPA  = pin->nrPages;


	return TZ_RESULT_SUCCESS;

us_map_fail:

	return TZ_RESULT_ERROR_INVALID_HANDLE;

}

/**************************************************************************
*  DEV DRIVER IOCTL
*  Ported from trustzone driver
**************************************************************************/
static long tz_client_open_session(struct file *file, unsigned long arg)
{
	struct kree_session_cmd_param param;
	unsigned long cret;
	char uuid[40];
	long len;
	TZ_RESULT ret;
	KREE_SESSION_HANDLE handle;

	cret = copy_from_user(&param, (void *)arg, sizeof(param));
	if (cret)
		return -EFAULT;

	/* Check if can we access UUID string. 10 for min uuid len. */
	if (!access_ok(VERIFY_READ, param.data, 10))
		return -EFAULT;

	KREE_DEBUG("%s: uuid addr = 0x%llx\n", __func__, param.data);
	len = strncpy_from_user(uuid,
				(void *)(unsigned long)param.data,
				sizeof(uuid));
	if (len <= 0)
		return -EFAULT;

	uuid[sizeof(uuid) - 1] = 0;
	ret = KREE_CreateSession(uuid, &handle);
	param.ret = ret;
	param.handle = handle;

	cret = copy_to_user((void *)arg, &param, sizeof(param));
	if (cret)
		return cret;

	return 0;

}

static long tz_client_close_session(struct file *file, unsigned long arg)
{
	struct kree_session_cmd_param param;
	unsigned long cret;
	TZ_RESULT ret;

	cret = copy_from_user(&param, (void *)arg, sizeof(param));
	if (cret)
		return -EFAULT;

	ret = KREE_CloseSession(param.handle);
	param.ret = ret;

	cret = copy_to_user((void *)arg, &param, sizeof(param));
	if (cret)
		return -EFAULT;

	return 0;
}


static long tz_client_tee_service(struct file *file, unsigned long arg,
					unsigned int compat)
{
	struct kree_tee_service_cmd_param cparam;
	unsigned long cret;
	uint32_t tmpTypes;
	union MTEEC_PARAM param[4], oparam[4];
	int i;
	TZ_RESULT ret;
	KREE_SESSION_HANDLE handle;
	void __user *ubuf;
	uint32_t ubuf_sz;

	cret = copy_from_user(&cparam, (void *)arg, sizeof(cparam));
	if (cret) {
		KREE_ERR("%s: copy_from_user(msg) failed\n", __func__);
		return -EFAULT;
	}

	if (cparam.paramTypes != TZPT_NONE || cparam.param) {
		if (!access_ok(VERIFY_READ, cparam.param, sizeof(oparam)))
			return -EFAULT;

		cret = copy_from_user(oparam,
					(void *)(unsigned long)cparam.param,
					sizeof(oparam));
		if (cret) {
			KREE_ERR("%s: copy_from_user(param) failed\n", __func__);
			return -EFAULT;
		}
	}

	handle = (KREE_SESSION_HANDLE)cparam.handle;
	KREE_DEBUG("%s: session handle = %u\n", __func__, handle);

	/* Parameter processing. */
	memset(param, 0, sizeof(param));
	tmpTypes = cparam.paramTypes;
	for (i = 0; tmpTypes; i++) {
		enum TZ_PARAM_TYPES type = tmpTypes & 0xff;

		tmpTypes >>= 8;
		switch (type) {
		case TZPT_VALUE_INPUT:
		case TZPT_VALUE_INOUT:
			param[i] = oparam[i];
			break;

		case TZPT_VALUE_OUTPUT:
		case TZPT_NONE:
			break;

		case TZPT_MEM_INPUT:
		case TZPT_MEM_OUTPUT:
		case TZPT_MEM_INOUT:
#ifdef CONFIG_COMPAT
			if (compat) {
				ubuf = compat_ptr(oparam[i].mem32.buffer);
				ubuf_sz = oparam[i].mem32.size;
			} else
#endif
			{
				ubuf = oparam[i].mem.buffer;
				ubuf_sz = oparam[i].mem.size;
			}

			KREE_DEBUG("%s: ubuf = %p, ubuf_sz = %u\n", __func__, ubuf, ubuf_sz);

			if (type != TZPT_MEM_OUTPUT) {
				if (!access_ok(VERIFY_READ, ubuf,
						   ubuf_sz)) {
					KREE_ERR("%s: cannnot read mem\n", __func__);
					cret = -EFAULT;
					goto error;
				}
			}
			if (type != TZPT_MEM_INPUT) {
				if (!access_ok(VERIFY_WRITE, ubuf,
						   ubuf_sz)) {
					KREE_ERR("%s: cannnot write mem\n", __func__);
					cret = -EFAULT;
					goto error;
				}
			}

			if (ubuf_sz > GP_MEM_MAX_LEN) {
				KREE_ERR("%s: ubuf_sz larger than max(%d)\n", __func__, GP_MEM_MAX_LEN);
				cret = -ENOMEM;
				goto error;
			}

			param[i].mem.size = ubuf_sz;
			param[i].mem.buffer = kmalloc(param[i].mem.size,
							GFP_KERNEL);
			if (!param[i].mem.buffer) {
				KREE_ERR("%s: kmalloc failed\n", __func__);
				cret = -ENOMEM;
				goto error;
			}

			if (type != TZPT_MEM_OUTPUT) {
				cret = copy_from_user(param[i].mem.buffer,
							  ubuf,
							  param[i].mem.size);
				if (cret) {
					KREE_ERR("%s: copy_from_user failed\n", __func__);
					cret = -EFAULT;
					goto error;
				}
			}
			break;

		case TZPT_MEMREF_INPUT:
		case TZPT_MEMREF_OUTPUT:
		case TZPT_MEMREF_INOUT:
			param[i] = oparam[i];
			break;

		default:
			ret = TZ_RESULT_ERROR_BAD_FORMAT;
			goto error;
		}
	}

	ret = KREE_TeeServiceCall(handle, cparam.command,
						cparam.paramTypes, param);

	cparam.ret = ret;
	tmpTypes = cparam.paramTypes;
	for (i = 0; tmpTypes; i++) {
		enum TZ_PARAM_TYPES type = tmpTypes & 0xff;

		tmpTypes >>= 8;
		switch (type) {
		case TZPT_VALUE_OUTPUT:
		case TZPT_VALUE_INOUT:
			oparam[i] = param[i];
			break;

		default:
		case TZPT_MEMREF_INPUT:
		case TZPT_MEMREF_OUTPUT:
		case TZPT_MEMREF_INOUT:
		case TZPT_VALUE_INPUT:
		case TZPT_NONE:
			break;

		case TZPT_MEM_INPUT:
		case TZPT_MEM_OUTPUT:
		case TZPT_MEM_INOUT:
#ifdef CONFIG_COMPAT
			if (compat)
				ubuf = compat_ptr(oparam[i].mem32.buffer);
			else
#endif
				ubuf = oparam[i].mem.buffer;

			if (type != TZPT_MEM_INPUT) {
				cret = copy_to_user(ubuf,
						param[i].mem.buffer,
						param[i].mem.size);
				if (cret) {
					cret = -EFAULT;
					goto error;
				}
			}

			kfree(param[i].mem.buffer);
			break;
		}
	}

	/* Copy data back. */
	if (cparam.paramTypes != TZPT_NONE) {
		cret = copy_to_user((void *)(unsigned long)cparam.param,
					oparam,
					sizeof(oparam));
		if (cret) {
			KREE_ERR("%s: copy_to_user(param) failed\n", __func__);
			return -EFAULT;
		}
	}


	cret = copy_to_user((void *)arg, &cparam, sizeof(cparam));
	if (cret) {
		KREE_ERR("%s: copy_to_user(msg) failed\n", __func__);
		return -EFAULT;
	}
	return 0;

 error:
	tmpTypes = cparam.paramTypes;
	for (i = 0; tmpTypes; i++) {
		enum TZ_PARAM_TYPES type = tmpTypes & 0xff;

		tmpTypes >>= 8;
		switch (type) {
		case TZPT_MEM_INPUT:
		case TZPT_MEM_OUTPUT:
		case TZPT_MEM_INOUT:
			kfree(param[i].mem.buffer);
			break;

		default:
			break;
		}
	}
	return cret;
}


static long _gz_ioctl(struct file *filep, unsigned int cmd, unsigned long arg, unsigned int compat)

{
	int err;
	TZ_RESULT ret = 0;
	char __user *user_req;
	struct user_shm_param shm_data;
	KREE_SHAREDMEM_PARAM shm_param;
	KREE_SHAREDMEM_HANDLE shm_handle;
	struct MTIOMMU_PIN_RANGE_T *pin = NULL;
	uint64_t *map_p = NULL;
	int numOfPA = 0;

	if (_IOC_TYPE(cmd) != MTEE_IOC_MAGIC)
		return -EINVAL;

	if (compat)
		user_req = (char __user *)compat_ptr(arg);
	else
		user_req = (char __user *)arg;

	switch (cmd) {
	case MTEE_CMD_SHM_REG:

		KREE_DEBUG("====> GZ_IOC_REG_SHAREDMEM ====\n");

		/* copy param from user */
		err = copy_from_user(&shm_data, user_req, sizeof(shm_data));
		if (err < 0) {
			KREE_ERR("%s: copy_from_user failed(%d)\n", __func__, err);
			return err;
		}

		if ((shm_data.param.size <= 0) || (!shm_data.param.buffer)) {
			KREE_DEBUG("[%s]====> param.size <= 0 OR !param.buffer ==>[stop!].\n", __func__);
			return TZ_RESULT_ERROR_BAD_PARAMETERS;
		}

		KREE_DEBUG("[%s]sizeof(shm_data):0x%x, session:%u, shm_handle:%u, size:%u, &buffer:0x%llx\n",
			__func__, (uint32_t) sizeof(shm_data), shm_data.session,
			shm_data.shm_handle, shm_data.param.size, shm_data.param.buffer);
		ret = get_US_PAMapAry(&shm_data, &shm_param, &numOfPA, pin, map_p);
		if (ret != TZ_RESULT_SUCCESS)
			KREE_ERR("[%s] ====> get_us_PAMapAry() returns Fail. [Stop!]. ret=0x%x\n", __func__, ret);

		if (ret == TZ_RESULT_SUCCESS) {
			ret = _registerSharedmem_body(shm_data.session, &shm_param, numOfPA, &shm_handle);
			if (ret != TZ_RESULT_SUCCESS)
				KREE_ERR("[%s] ====> shm_mem_service_process() returns Fail. [Stop!]. ret=0x%x\n",
					__func__, ret);
		}

		kfree(pin);
		kfree(map_p);

		if (ret != TZ_RESULT_SUCCESS) {
			KREE_ERR("%s: KREE API failed(0x%x)\n", __func__, ret);
			return ret;
		}

		shm_data.shm_handle = shm_handle;

		/* copy result back to user */
		shm_data.session = ret;
		err = copy_to_user(user_req, &shm_data, sizeof(shm_data));
		if (err < 0)
			KREE_ERR("%s: copy_to_user failed(%d)\n", __func__, err);

		break;

	case MTEE_CMD_SHM_UNREG:
		/* do nothing */
		break;

	case MTEE_CMD_OPEN_SESSION:
		KREE_DEBUG("====> MTEE_CMD_OPEN_SESSION ====\n");
		return tz_client_open_session(filep, arg);

	case MTEE_CMD_CLOSE_SESSION:
		KREE_DEBUG("====> MTEE_CMD_CLOSE_SESSION ====\n");
		return tz_client_close_session(filep, arg);

	case MTEE_CMD_TEE_SERVICE:
		KREE_DEBUG("====> MTEE_CMD_TEE_SERVICE ====\n");
		return tz_client_tee_service(filep, arg, compat);

	default:
		KREE_ERR("%s: undefined ioctl cmd 0x%x\n", __func__, cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static long gz_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	return _gz_ioctl(filep, cmd, arg, 0);
}

#if defined(CONFIG_COMPAT)
static long gz_compat_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	return _gz_ioctl(filep, cmd, arg, 1);
}
#endif

/************ kernel module init entry ***************/
static int __init gz_init(void)
{
	int res;

	res = create_files();
	if (res)
		KREE_DEBUG("create sysfs failed: %d\n", res);

	return 0;
}

/************ kernel module exit entry ***************/
static void __exit gz_exit(void)
{
	KREE_DEBUG("gz driver exit\n");
}

module_init(gz_init);
module_exit(gz_exit);
