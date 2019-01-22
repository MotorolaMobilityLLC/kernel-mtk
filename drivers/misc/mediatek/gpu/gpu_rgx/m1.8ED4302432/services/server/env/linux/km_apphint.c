/*************************************************************************/ /*!
@File           km_apphint.c
@Title          Apphint routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device specific functions
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if defined(SUPPORT_KERNEL_SRVINIT)

#include "pvr_debugfs.h"
#include "pvr_uaccess.h"
#include <linux/moduleparam.h>
#include <linux/workqueue.h>
#include <linux/string.h>

/* for action device access */
#include "pvrsrv.h"
#include "device.h"
#include "rgxdevice.h"
#include "rgxfwutils.h"
#include "debugmisc_server.h"
#include "htbserver.h"
#include "rgxutils.h"
#include "rgxapi_km.h"

#include "img_defs.h"

/* defines for default values */
#include "rgx_fwif.h"
#include "htbuffer_types.h"

#include "pvr_notifier.h"

#include "km_apphint_defs.h"
#include "km_apphint.h"

#if defined(PDUMP)
#include <stdarg.h>
#include "pdump_km.h"
#endif

/* Size of temporary buffers used to read and write AppHint data.
 * Must be large enough to contain any strings read/written
 * but no larger than 4096 with is the buffer size for the
 * kernel_param_ops .get function.
 * And less than 1024 to keep the stack frame size within bounds.
 */
#define APPHINT_BUFFER_SIZE 512

#define APPHINT_DEVICES_MAX 16

/*
*******************************************************************************
 * AppHint mnemonic data type helper tables
******************************************************************************/
struct apphint_lookup {
	char *name;
	int value;
};

struct apphint_map {
	int id;
	unsigned flag;
};

static const struct apphint_map apphint_flag_map[] = {
	{ APPHINT_ID_AssertOnHWRTrigger, RGXFWIF_INICFG_ASSERT_ON_HWR_TRIGGER },
	{ APPHINT_ID_AssertOutOfMemory,  RGXFWIF_INICFG_ASSERT_ON_OUTOFMEMORY },
	{ APPHINT_ID_CheckMList,         RGXFWIF_INICFG_CHECK_MLIST_EN },
	{ APPHINT_ID_EnableHWR,          RGXFWIF_INICFG_HWR_EN },
	{ APPHINT_ID_DisableFEDLogging,  RGXKMIF_DEVICE_STATE_DISABLE_DW_LOGGING_EN },
	{ APPHINT_ID_ZeroFreelist,       RGXKMIF_DEVICE_STATE_ZERO_FREELIST },
	{ APPHINT_ID_DustRequestInject,  RGXKMIF_DEVICE_STATE_DUST_REQUEST_INJECT_EN },
	{ APPHINT_ID_DisableClockGating, RGXFWIF_INICFG_DISABLE_CLKGATING_EN },
	{ APPHINT_ID_DisableDMOverlap,   RGXFWIF_INICFG_DISABLE_DM_OVERLAP },
};

static const struct apphint_lookup fwt_logtype_tbl[] = {
	{ "trace", 2},
	{ "tbi", 1},
	{ "none", 0}
};

static const struct apphint_lookup fwt_loggroup_tbl[] = {
	RGXFWIF_LOG_GROUP_NAME_VALUE_MAP
};

static const struct apphint_lookup htb_loggroup_tbl[] = {
#define X(a, b) { #b, HTB_LOG_GROUP_FLAG(a) },
	HTB_LOG_SFGROUPLIST
#undef X
};

static const struct apphint_lookup htb_opmode_tbl[] = {
	{ "droplatest", HTB_OPMODE_DROPLATEST},
	{ "dropoldest", HTB_OPMODE_DROPOLDEST},
	{ "block", HTB_OPMODE_BLOCK}
};

__maybe_unused
static const struct apphint_lookup htb_logmode_tbl[] = {
	{ "all", HTB_LOGMODE_ALLPID},
	{ "restricted", HTB_LOGMODE_RESTRICTEDPID}
};

/*
*******************************************************************************
 Data types
******************************************************************************/
union apphint_value {
	IMG_UINT64 UINT64;
	IMG_UINT32 UINT32;
	IMG_BOOL BOOL;
};

struct apphint_action {
	union {
		PVRSRV_ERROR (*UINT64)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT64 *value);
		PVRSRV_ERROR (*UINT32)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT32 *value);
		PVRSRV_ERROR (*BOOL)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_BOOL *value);
	} query;
	union {
		PVRSRV_ERROR (*UINT64)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT64 value);
		PVRSRV_ERROR (*UINT32)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT32 value);
		PVRSRV_ERROR (*BOOL)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_BOOL value);
	} set;
	const PVRSRV_DEVICE_NODE *device;
	const void *private_data;
	union apphint_value stored;
};

struct apphint_param {
	IMG_UINT32 id;
	APPHINT_DATA_TYPE data_type;
	APPHINT_ACTION  action_type;
	const void *data_type_helper;
	IMG_UINT32 helper_size;
};

struct apphint_init_data {
	IMG_UINT32 id;			/* index into AppHint Table */
	APPHINT_CLASS class;
	u16 perm;
	IMG_CHAR *name;
	union apphint_value default_value;
};

struct apphint_class_state {
	APPHINT_CLASS class;
	IMG_BOOL enabled;
};

struct apphint_work {
	struct work_struct work;
	union apphint_value new_value;
	struct apphint_action *action;
};

/*
*******************************************************************************
 Initialization / configuration table data
******************************************************************************/
static const struct apphint_init_data init_data_buildvar[] = {
#define X(a, b, c, d, e, f, g) \
	{APPHINT_ID_ ## a, APPHINT_CLASS_ ## e, c, #a, {f} },
	APPHINT_LIST_BUILDVAR
#undef X
};

static const struct apphint_init_data init_data_modparam[] = {
#define X(a, b, c, d, e, f, g) \
	{APPHINT_ID_ ## a, APPHINT_CLASS_ ## e, c, #a, {f} },
	APPHINT_LIST_MODPARAM
#undef X
};

static const struct apphint_init_data init_data_debugfs[] = {
#define X(a, b, c, d, e, f, g) \
	{APPHINT_ID_ ## a, APPHINT_CLASS_ ## e, c, #a, {f} },
	APPHINT_LIST_DEBUGFS
#undef X
};

static const struct apphint_init_data init_data_debugfs_device[] = {
#define X(a, b, c, d, e, f, g) \
	{APPHINT_ID_ ## a, APPHINT_CLASS_ ## e, c, #a, {f} },
	APPHINT_LIST_DEBUGFS_DEVICE
#undef X
};

/* Don't use the kernel ARRAY_SIZE macro here because it checks
 * __must_be_array() and we need to be able to use this safely on a NULL ptr.
 * This will return an undefined size for a NULL ptr - so should only be
 * used here.
 */
#define APPHINT_HELP_ARRAY_SIZE(a) (sizeof((a))/(sizeof((a[0]))))

static const struct apphint_param param_lookup[] = {
#define X(a, b, c, d, e, f, g) \
	{APPHINT_ID_ ## a, APPHINT_DATA_TYPE_ ## b, APPHINT_ACTION_ ## d, g, APPHINT_HELP_ARRAY_SIZE(g) },
	APPHINT_LIST_ALL
#undef X
};

#undef APPHINT_HELP_ARRAY_SIZE

static const struct apphint_class_state class_state[] = {
#define X(a) {APPHINT_CLASS_ ## a, APPHINT_ENABLED_CLASS_ ## a},
	APPHINT_CLASS_LIST
#undef X
};

/*
*******************************************************************************
 Global state
******************************************************************************/
/* If the union apphint_value becomes such that it is not possible to read
 * and write atomically, a mutex may be desirable to prevent a read returning
 * a partially written state.
 * This would require a statically initialized mutex outside of the
 * struct apphint_state to prevent use of an uninitialized mutex when
 * module_params are provided on the command line.
 *     static DEFINE_MUTEX(apphint_mutex);
 */
static struct apphint_state
{
	struct workqueue_struct *workqueue;
	PVR_DEBUGFS_DIR_DATA *debugfs_device_rootdir[APPHINT_DEVICES_MAX];
	PVR_DEBUGFS_ENTRY_DATA *debugfs_device_entry[APPHINT_DEVICES_MAX][APPHINT_DEBUGFS_DEVICE_ID_MAX];
	PVR_DEBUGFS_DIR_DATA *debugfs_rootdir;
	PVR_DEBUGFS_ENTRY_DATA *debugfs_entry[APPHINT_DEBUGFS_ID_MAX];
	PVR_DEBUGFS_DIR_DATA *buildvar_rootdir;
	PVR_DEBUGFS_ENTRY_DATA *buildvar_entry[APPHINT_BUILDVAR_ID_MAX];

	int num_devices;
	PVRSRV_DEVICE_NODE *devices[APPHINT_DEVICES_MAX];
	int initialized;

	struct apphint_action val[APPHINT_ID_MAX + ((APPHINT_DEVICES_MAX-1)*APPHINT_DEBUGFS_DEVICE_ID_MAX)];

} apphint = {
/* statically initialise default values to ensure that any module_params
 * provided on the command line are not overwritten by defaults.
 */
	.val = {
#define X(a, b, c, d, e, f, g) \
	{ {NULL}, {NULL}, NULL, NULL, {f} },
	APPHINT_LIST_ALL
#undef X
	},
	.initialized = 0,
	.num_devices = 0
};

#define APPHINT_DEBUGFS_DEVICE_ID_OFFSET (APPHINT_ID_MAX-APPHINT_DEBUGFS_DEVICE_ID_MAX)

static inline void
get_apphint_id_from_action_addr(const struct apphint_action * const addr,
                                APPHINT_ID * const id)
{
	*id = (APPHINT_ID)(addr - apphint.val);
	if (*id >= APPHINT_ID_MAX) {
		*id -= APPHINT_DEBUGFS_DEVICE_ID_OFFSET;
		*id %= APPHINT_DEBUGFS_DEVICE_ID_MAX;
		*id += APPHINT_DEBUGFS_DEVICE_ID_OFFSET;
	}
}

static inline void
get_value_offset_from_device(const PVRSRV_DEVICE_NODE * const device,
                             int * const offset)
{
	int i;
	for (i = 0; device && i < APPHINT_DEVICES_MAX; i++) {
		if (apphint.devices[i] == device)
			break;
	}
	if (APPHINT_DEVICES_MAX == i) {
		PVR_DPF((PVR_DBG_WARNING, "%s: Unregistered device", __func__));
		i = 0;
	}
	*offset = i * APPHINT_DEBUGFS_DEVICE_ID_MAX;
}


/*
*******************************************************************************
 Generic AppHint bool/flag handling
******************************************************************************/
static PVRSRV_ERROR RGXSetPdumpPanicAppHint(const PVRSRV_DEVICE_NODE *psDevice, const void *psPrivate, IMG_BOOL bValue)
{
	const struct apphint_param *param = (struct apphint_param *)psPrivate;
	PVRSRV_ERROR eResult;
	int device_value_offset;

	get_value_offset_from_device(psDevice, &device_value_offset);

	eResult = RGXSetPdumpPanicEnable((PVRSRV_RGXDEV_INFO *)psDevice->pvDevice, bValue);
	if (PVRSRV_OK == eResult)
		apphint.val[param->id + device_value_offset].stored.BOOL = bValue;

	return eResult;
}

static PVRSRV_ERROR RGXSetAppHintFlag(
		PVRSRV_ERROR (*set)(PVRSRV_RGXDEV_INFO *device, IMG_UINT32 flag, IMG_BOOL value),
		const PVRSRV_DEVICE_NODE *psDevice, const void *psPrivate, IMG_BOOL bValue)
{
	const struct apphint_param *param = (struct apphint_param *)psPrivate;
	struct apphint_map *map;
	IMG_UINT32 ui32Flag = 0;
	PVRSRV_ERROR eResult;
	int i, device_value_offset;

	get_value_offset_from_device(psDevice, &device_value_offset);

	if (!param || !param->data_type_helper || !param->helper_size || param->id >= APPHINT_ID_MAX)
		return PVRSRV_ERROR_INVALID_PARAMS;

	map = (struct apphint_map *)param->data_type_helper;

	if (APPHINT_DATA_TYPE_FLAG != param->data_type)
		return PVRSRV_ERROR_INVALID_PARAMS;

	/* EnableHWR is a special case, can only disable */
	if (APPHINT_ID_EnableHWR == param->id && IMG_FALSE != bValue)
			return PVRSRV_ERROR_NOT_SUPPORTED;

	for (i = 0; i < param->helper_size; i++) {
		if (map[i].id == param->id) {
			ui32Flag = map[i].flag;
			break;
		}
	}

	if (i == param->helper_size)
		return PVRSRV_ERROR_INVALID_PARAMS;


	eResult = set((PVRSRV_RGXDEV_INFO *)psDevice->pvDevice, ui32Flag, bValue);
	if (PVRSRV_OK == eResult)
		apphint.val[param->id + device_value_offset].stored.BOOL = bValue;

	return eResult;
}

static PVRSRV_ERROR RGXSetStateFlag(
		const PVRSRV_DEVICE_NODE *psDevice, const void *psPrivate, IMG_BOOL bValue)
{
	return RGXSetAppHintFlag(RGXSetStateFlags, psDevice, psPrivate, bValue);
}

static PVRSRV_ERROR RGXSetDeviceFlag(
		const PVRSRV_DEVICE_NODE *psDevice, const void *psPrivate, IMG_BOOL bValue)
{
	return RGXSetAppHintFlag(RGXSetDeviceFlags, psDevice, psPrivate, bValue);
}

static PVRSRV_ERROR RGXReadAppHintBool(const PVRSRV_DEVICE_NODE *psDevice, const void *psPrivate, IMG_BOOL *bValue)
{
	const struct apphint_param *param = (struct apphint_param *)psPrivate;
	int device_value_offset;

	if (!param || param->id >= APPHINT_ID_MAX)
		return PVRSRV_ERROR_INVALID_PARAMS;

	if (APPHINT_DATA_TYPE_FLAG != param->data_type
	    && APPHINT_DATA_TYPE_BOOL != param->data_type) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	get_value_offset_from_device(psDevice, &device_value_offset);


	/* TODO: This function should query the real value from the device
	 */

	*bValue = apphint.val[param->id + device_value_offset].stored.BOOL;


	return PVRSRV_OK;
}

static void register_rgx_flag_handlers(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	pvr_apphint_register_handlers_bool(APPHINT_ID_DisableClockGating,
	                                   RGXReadAppHintBool, RGXSetStateFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_DisableClockGating]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_DisableDMOverlap,
	                                   RGXReadAppHintBool, RGXSetStateFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_DisableDMOverlap]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_AssertOnHWRTrigger,
	                                   RGXReadAppHintBool, RGXSetStateFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_AssertOnHWRTrigger]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_AssertOutOfMemory,
	                                   RGXReadAppHintBool, RGXSetStateFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_AssertOutOfMemory]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_CheckMList,
	                                   RGXReadAppHintBool, RGXSetStateFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_CheckMList]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_EnableHWR,
	                                   RGXReadAppHintBool, RGXSetStateFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_EnableHWR]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_DisableFEDLogging,
	                                   RGXReadAppHintBool, RGXSetDeviceFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_DisableFEDLogging]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_ZeroFreelist,
	                                   RGXReadAppHintBool, RGXSetDeviceFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_ZeroFreelist]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_DustRequestInject,
	                                   RGXReadAppHintBool, RGXSetDeviceFlag,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_DustRequestInject]);
	pvr_apphint_register_handlers_bool(APPHINT_ID_DisablePDumpPanic,
	                                   RGXReadAppHintBool, RGXSetPdumpPanicAppHint,
	                                   psDeviceNode,
	                                   &param_lookup[APPHINT_ID_DisablePDumpPanic]);
}

/**
 * apphint_action_worker - perform an action after an AppHint update has been
 *                    requested by a UM process
 *                    And update the record of the current active value
 */
static void apphint_action_worker(struct work_struct *work)
{
	struct apphint_work *work_pkt = container_of(work,
	                                             struct apphint_work,
	                                             work);
	struct apphint_action *a = work_pkt->action;
	union apphint_value value = work_pkt->new_value;
	APPHINT_ID id;
	PVRSRV_ERROR result = PVRSRV_OK;

	get_apphint_id_from_action_addr(a, &id);

	if (a->set.UINT64) {
		switch (param_lookup[id].data_type) {
		case APPHINT_DATA_TYPE_UINT64:
			result = a->set.UINT64(a->device,
			                       a->private_data,
			                       value.UINT64);
			break;

		case APPHINT_DATA_TYPE_UINT32:
		case APPHINT_DATA_TYPE_UINT32Bitfield:
		case APPHINT_DATA_TYPE_UINT32List:
		case APPHINT_DATA_TYPE_UINT32Array:
			result = a->set.UINT32(a->device,
			                       a->private_data,
			                       value.UINT32);
			break;

		case APPHINT_DATA_TYPE_BOOL:
		case APPHINT_DATA_TYPE_FLAG:
			result = a->set.BOOL(a->device,
			                     a->private_data,
			                     value.BOOL);
			break;

		default:
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: unrecognised data type (%d), index (%d)",
			         __func__, param_lookup[id].data_type, id));
		}

		if (PVRSRV_OK != result) {
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: failed (%s)",
			         __func__, PVRSRVGetErrorStringKM(result)));
		}
	} else {
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: value update requested for AppHint with no registered handler, ID(%d)",
		         __func__, id));
	}

	kfree((void *)work_pkt);
}

static void apphint_action(union apphint_value new_value,
                           struct apphint_action *action)
{
	struct apphint_work *work_pkt = kmalloc(sizeof(*work_pkt), GFP_KERNEL);

	/* queue apphint update on a serialized workqueue to avoid races */
	if (work_pkt) {
		work_pkt->new_value = new_value;
		work_pkt->action = action;
		INIT_WORK(&work_pkt->work, apphint_action_worker);
		if (0 == queue_work(apphint.workqueue, &work_pkt->work)) {
			PVR_DPF((PVR_DBG_ERROR,
				"%s: failed to queue apphint change request",
				__func__));
		}
	} else {
		PVR_DPF((PVR_DBG_ERROR,
			"%s: failed to alloc memory for apphint change request",
			__func__));
	}
}

/**
 * apphint_read - read the different AppHint data types
 * return -errno or the buffer size
 */
static int apphint_read(char *buffer, size_t count, APPHINT_ID ue,
			 union apphint_value *value)
{
	APPHINT_DATA_TYPE data_type = param_lookup[ue].data_type;
	int result = 0;

	switch (data_type) {
	case APPHINT_DATA_TYPE_UINT64:
		if (kstrtou64(buffer, 0, &value->UINT64) < 0) {
			PVR_DPF((PVR_DBG_ERROR,
				"%s: Invalid UINT64 input data for id %d: %s",
				__func__, ue, buffer));
			result = -EINVAL;
			goto err_exit;
		}
		break;
	case APPHINT_DATA_TYPE_UINT32:
		if (kstrtou32(buffer, 0, &value->UINT32) < 0) {
			PVR_DPF((PVR_DBG_ERROR,
				"%s: Invalid UINT32 input data for id %d: %s",
				__func__, ue, buffer));
			result = -EINVAL;
			goto err_exit;
		}
		break;
	case APPHINT_DATA_TYPE_BOOL:
	case APPHINT_DATA_TYPE_FLAG:
		switch (buffer[0]) {
		case '0':
		case 'n':
		case 'N':
		case 'f':
		case 'F':
			value->BOOL = IMG_FALSE;
			break;
		case '1':
		case 'y':
		case 'Y':
		case 't':
		case 'T':
			value->BOOL = IMG_TRUE;
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR,
				"%s: Invalid BOOL input data for id %d: %s",
				__func__, ue, buffer));
			result = -EINVAL;
			goto err_exit;
		}
		break;
	case APPHINT_DATA_TYPE_UINT32List:
	{
		int i;
		struct apphint_lookup *lookup =
			(struct apphint_lookup *)
			param_lookup[ue].data_type_helper;
		int size = param_lookup[ue].helper_size;
		/* buffer may include '\n', remove it */
		char *arg = strsep(&buffer, "\n");

		if (!lookup) {
			result = -EINVAL;
			goto err_exit;
		}

		for (i = 0; i < size; i++) {
			if (strcasecmp(lookup[i].name, arg) == 0) {
				value->UINT32 = lookup[i].value;
				break;
			}
		}
		if (i == size) {
			if (strlen(arg) == 0) {
				PVR_DPF((PVR_DBG_ERROR,
					"%s: No value set for AppHint",
					__func__));
			} else {
				PVR_DPF((PVR_DBG_ERROR,
					"%s: Unrecognised AppHint value (%s)",
					__func__, arg));
			}
			result = -EINVAL;
		}
		break;
	}
	case APPHINT_DATA_TYPE_UINT32Bitfield:
	{
		int i;
		struct apphint_lookup *lookup =
			(struct apphint_lookup *)
			param_lookup[ue].data_type_helper;
		int size = param_lookup[ue].helper_size;
		/* buffer may include '\n', remove it */
		char *string = strsep(&buffer, "\n");
		char *token = strsep(&string, ",");

		if (!lookup) {
			result = -EINVAL;
			goto err_exit;
		}

		value->UINT32 = 0;
		/* empty string is valid to clear the bitfield */
		while (token && *token) {
			for (i = 0; i < size; i++) {
				if (strcasecmp(lookup[i].name, token) == 0) {
					value->UINT32 |= lookup[i].value;
					break;
				}
			}
			if (i == size) {
				PVR_DPF((PVR_DBG_ERROR,
					"%s: Unrecognised AppHint value (%s)",
					__func__, token));
				result = -EINVAL;
				goto err_exit;
			}
			token = strsep(&string, ",");
		}
		break;
	}
	case APPHINT_DATA_TYPE_UINT32Array:
		value->UINT32 = 0;
		break;
	default:
		result = -EINVAL;
		goto err_exit;
	}

err_exit:
	return (result < 0) ? result : count;
}

/**
 * apphint_write - write the current AppHint data to a buffer
 *
 * Returns length written or -errno
 */
static int apphint_write(char *buffer, const size_t size,
                         const struct apphint_action *a)
{
	const struct apphint_param *hint;
	int result = 0;
	APPHINT_ID id;
	union apphint_value value;

	get_apphint_id_from_action_addr(a, &id);
	hint = &param_lookup[id];

	if (a->query.UINT64) {
		switch (hint->data_type) {
		case APPHINT_DATA_TYPE_UINT64:
			result = a->query.UINT64(a->device,
			                         a->private_data,
			                         &value.UINT64);
			break;

		case APPHINT_DATA_TYPE_UINT32:
		case APPHINT_DATA_TYPE_UINT32Bitfield:
		case APPHINT_DATA_TYPE_UINT32List:
		case APPHINT_DATA_TYPE_UINT32Array:
			result = a->query.UINT32(a->device,
			                         a->private_data,
			                         &value.UINT32);
			break;

		case APPHINT_DATA_TYPE_BOOL:
		case APPHINT_DATA_TYPE_FLAG:
			result = a->query.BOOL(a->device,
			                       a->private_data,
			                       &value.BOOL);
			break;

		default:
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: unrecognised data type (%d), index (%d)",
			         __func__, hint->data_type, id));
		}

		if (PVRSRV_OK != result) {
			PVR_DPF((PVR_DBG_ERROR, "%s: failed (%d), index (%d)",
			         __func__, result, id));
		}
	} else {
		value = a->stored;
	}

	switch (hint->data_type) {
	case APPHINT_DATA_TYPE_UINT64:
		result += snprintf(buffer + result, size - result,
				"0x%016llx",
				value.UINT64);
		break;
	case APPHINT_DATA_TYPE_UINT32:
		result += snprintf(buffer + result, size - result,
				"0x%08x",
				value.UINT32);
		break;
	case APPHINT_DATA_TYPE_BOOL:
	case APPHINT_DATA_TYPE_FLAG:
		result += snprintf(buffer + result, size - result,
			"%s",
			value.BOOL ? "Y" : "N");
		break;
	case APPHINT_DATA_TYPE_UINT32List:
	{
		struct apphint_lookup *lookup =
			(struct apphint_lookup *) hint->data_type_helper;
		IMG_UINT32 i;

		if (!lookup) {
			result = -EINVAL;
			goto err_exit;
		}

		for (i = 0; i < hint->helper_size; i++) {
			if (lookup[i].value == value.UINT32) {
				result += snprintf(buffer + result,
						size - result,
						"%s",
						lookup[i].name);
				break;
			}
		}
		break;
	}
	case APPHINT_DATA_TYPE_UINT32Bitfield:
	{
		struct apphint_lookup *lookup =
			(struct apphint_lookup *) hint->data_type_helper;
		IMG_UINT32 i;

		if (!lookup) {
			result = -EINVAL;
			goto err_exit;
		}

		for (i = 0; i < hint->helper_size; i++) {
			if (lookup[i].value & value.UINT32) {
				result += snprintf(buffer + result,
						size - result,
						"%s,",
						lookup[i].name);
			}
		}
		if (result) {
			/* remove any trailing ',' */
			--result;
			*(buffer + result) = '\0';
		} else {
			result += snprintf(buffer + result,
					size - result, "none");
		}
		break;
	}
	case APPHINT_DATA_TYPE_UINT32Array:
		result += snprintf(buffer + result, size - result,
				"Not yet implemented");
		break;
	default:
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: unrecognised data type (%d), index (%d)",
			 __func__, hint->data_type, id));
		result = -EINVAL;
	}

err_exit:
	return result;
}

/*
*******************************************************************************
 Module parameters initialization - different from debugfs
******************************************************************************/
/**
 * apphint_kparam_set - Handle an update of a module parameter
 *
 * Returns 0, or -errno.  arg is in kp->arg.
 */
static int apphint_kparam_set(const char *val, const struct kernel_param *kp)
{
	char val_copy[APPHINT_BUFFER_SIZE];
	APPHINT_ID id;
	union apphint_value value;
	int result;

	/* need to discard const in case of string comparison */
	result = strlcpy(val_copy, val, APPHINT_BUFFER_SIZE);

	get_apphint_id_from_action_addr(kp->arg, &id);
	if (result < APPHINT_BUFFER_SIZE) {
		result = apphint_read(val_copy, result, id, &value);
		if (result >= 0)
			((struct apphint_action *)kp->arg)->stored = value;
	} else {
		PVR_DPF((PVR_DBG_ERROR, "%s: String too long", __func__));
	}
	return (result > 0) ? 0 : result;
}

/**
 * apphint_kparam_get - handle a read of a module parameter
 *
 * Returns length written or -errno.  Buffer is 4k (ie. be short!)
 */
static int apphint_kparam_get(char *buffer, const struct kernel_param *kp)
{
	return apphint_write(buffer, PAGE_SIZE, kp->arg);
}

__maybe_unused
static const struct kernel_param_ops apphint_kparam_fops = {
	.set = apphint_kparam_set,
	.get = apphint_kparam_get,
};

/*
 * call module_param_cb() for all AppHints listed in APPHINT_LIST_MODPARAM
 * apphint_modparam_class_ ## resolves to apphint_modparam_enable() except for
 * AppHint classes that have been disabled.
 */

#define apphint_modparam_enable(name, number, perm) \
	module_param_cb(name, &apphint_kparam_fops, &apphint.val[number], perm);

#define X(a, b, c, d, e, f, g) \
	apphint_modparam_class_ ##e(a, APPHINT_ID_ ## a, (S_IRUSR|S_IRGRP|S_IROTH))
	APPHINT_LIST_MODPARAM
#undef X

/*
*******************************************************************************
 Debugfs get (seq file) operations - supporting functions
******************************************************************************/
static void *apphint_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos == 0) {
		/* We want only one entry in the sequence, one call to show() */
		return (void *) 1;
	}

	PVR_UNREFERENCED_PARAMETER(s);

	return NULL;
}

static void apphint_seq_stop(struct seq_file *s, void *v)
{
	PVR_UNREFERENCED_PARAMETER(s);
	PVR_UNREFERENCED_PARAMETER(v);
}

static void *apphint_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	PVR_UNREFERENCED_PARAMETER(s);
	PVR_UNREFERENCED_PARAMETER(v);
	PVR_UNREFERENCED_PARAMETER(pos);
	return NULL;
}

static int apphint_seq_show(struct seq_file *s, void *v)
{
	IMG_CHAR km_buffer[APPHINT_BUFFER_SIZE];
	int result;

	PVR_UNREFERENCED_PARAMETER(v);

	result = apphint_write(km_buffer, APPHINT_BUFFER_SIZE, s->private);
	if (result < 0) {
		PVR_DPF((PVR_DBG_ERROR, "%s: failure", __func__));
	} else {
		/* debugfs requires a trailing \n, module_params don't */
		result += snprintf(km_buffer + result,
				APPHINT_BUFFER_SIZE - result,
				"\n");
		seq_puts(s, km_buffer);
	}

	/* have to return 0 to see output */
	return (result < 0) ? result : 0;
}

static const struct seq_operations apphint_seq_fops = {
	.start = apphint_seq_start,
	.stop  = apphint_seq_stop,
	.next  = apphint_seq_next,
	.show  = apphint_seq_show,
};

/*
*******************************************************************************
 Debugfs supporting functions
******************************************************************************/
/**
 * apphint_set - Handle a debugfs value update
 */
static ssize_t apphint_set(const char __user *buffer,
			    size_t count,
			    loff_t position,
			    void *data)
{
	APPHINT_ID id;
	union apphint_value value;
	struct apphint_action *action = data;
	char km_buffer[APPHINT_BUFFER_SIZE];
	int result = 0;

	PVR_UNREFERENCED_PARAMETER(position);

	if (action->device->eDevState != PVRSRV_DEVICE_STATE_ACTIVE) {
		PVR_DPF((PVR_DBG_ERROR, "%s: Device not initialised yet", __func__));
		result = -ENODEV;
		goto err_exit;
	}

	if (count >= APPHINT_BUFFER_SIZE) {
		PVR_DPF((PVR_DBG_ERROR, "%s: String too long (%zd)",
			__func__, count));
		result = -EINVAL;
		goto err_exit;
	}

	if (pvr_copy_from_user(km_buffer, buffer, count)) {
		PVR_DPF((PVR_DBG_ERROR, "%s: Copy of user data failed",
			__func__));
		result = -EFAULT;
		goto err_exit;
	}
	km_buffer[count] = '\0';

	get_apphint_id_from_action_addr(action, &id);
	result = apphint_read(km_buffer, count, id, &value);
	if (result >= 0)
		apphint_action(value, action);

err_exit:
	return result;
}

/**
 * apphint_debugfs_init - Create the specified debugfs entries
 */
static int apphint_debugfs_init(char *sub_dir,
		int device_num,
		unsigned init_data_size,
		const struct apphint_init_data *init_data,
		PVR_DEBUGFS_DIR_DATA *parentdir,
		PVR_DEBUGFS_DIR_DATA **rootdir, PVR_DEBUGFS_ENTRY_DATA **entry)
{
	int result = 0;
	unsigned i;
	int device_value_offset = device_num * APPHINT_DEBUGFS_DEVICE_ID_MAX;

	if (*rootdir) {
		PVR_DPF((PVR_DBG_WARNING,
			"AppHint DebugFS already created, skipping"));
		result = -EEXIST;
		goto err_exit;
	}

	result = PVRDebugFSCreateEntryDir(sub_dir, parentdir,
					  rootdir);
	if (result < 0) {
		PVR_DPF((PVR_DBG_WARNING,
			"Failed to create \"%s\" DebugFS directory.", sub_dir));
		goto err_exit;
	}

	for (i = 0; i < init_data_size; i++) {
		if (!class_state[init_data[i].class].enabled)
			continue;

		result = PVRDebugFSCreateEntry(init_data[i].name,
				*rootdir,
				&apphint_seq_fops,
				(init_data[i].perm ? apphint_set : NULL),
				NULL,
				NULL,
				(void *) &apphint.val[init_data[i].id + device_value_offset],
				&entry[i]);
		if (result < 0) {
			PVR_DPF((PVR_DBG_WARNING,
				"Failed to create \"%s/%s\" DebugFS entry.",
				sub_dir, init_data[i].name));
		}
	}

err_exit:
	return result;
}

/**
 * apphint_debugfs_deinit- destroy the debugfs entries
 */
static void apphint_debugfs_deinit(unsigned num_entries,
		PVR_DEBUGFS_DIR_DATA **rootdir, PVR_DEBUGFS_ENTRY_DATA **entry)
{
	unsigned i;

	for (i = 0; i < num_entries; i++) {
		if (entry[i]) {
			PVRDebugFSRemoveEntry(&entry[i]);
			entry[i] = NULL;
		}
	}

	if (*rootdir) {
		PVRDebugFSRemoveEntryDir(rootdir);
		*rootdir = NULL;
	}
}

/*
*******************************************************************************
 AppHint status dump implementation
******************************************************************************/
#if defined(PDUMP)
static void apphint_pdump_values(void *flags, const IMG_CHAR *format, ...)
{
	char km_buffer[APPHINT_BUFFER_SIZE];
	IMG_UINT32 ui32Flags = *(IMG_UINT32 *)flags;
	va_list ap;

	va_start(ap, format);
	(void)vsnprintf(km_buffer, APPHINT_BUFFER_SIZE, format, ap);
	va_end(ap);

	PDumpCommentKM(km_buffer, ui32Flags);
}
#endif

static void apphint_dump_values(char *group_name,
			int device_num,
			const struct apphint_init_data *group_data,
			int group_size,
			DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
			void *pvDumpDebugFile)
{
	int i, result;
	int device_value_offset = device_num * APPHINT_DEBUGFS_DEVICE_ID_MAX;
	char km_buffer[APPHINT_BUFFER_SIZE];

	PVR_DUMPDEBUG_LOG("  %s", group_name);
	for (i = 0; i < group_size; i++) {
		result = apphint_write(km_buffer, APPHINT_BUFFER_SIZE,
				&apphint.val[group_data[i].id + device_value_offset]);

		if (result <= 0) {
			PVR_DUMPDEBUG_LOG("    %s: <Error>",
				group_data[i].name);
		} else {
			PVR_DUMPDEBUG_LOG("    %s: %s",
				group_data[i].name, km_buffer);
		}
	}
}

/**
 * Callback for debug dump
 */
static void apphint_dump_state(PVRSRV_DBGREQ_HANDLE hDebugRequestHandle,
			IMG_UINT32 ui32VerbLevel,
			DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
			void *pvDumpDebugFile)
{
	int i, result;
	char km_buffer[APPHINT_BUFFER_SIZE];
	PVRSRV_DEVICE_NODE *device = (PVRSRV_DEVICE_NODE *)hDebugRequestHandle;

	if (DEBUG_REQUEST_VERBOSITY_HIGH == ui32VerbLevel) {
		PVR_DUMPDEBUG_LOG("------[ AppHint Settings ]------");

		apphint_dump_values("Build Vars", 0,
			init_data_buildvar, ARRAY_SIZE(init_data_buildvar),
			pfnDumpDebugPrintf, pvDumpDebugFile);

		apphint_dump_values("Module Params", 0,
			init_data_modparam, ARRAY_SIZE(init_data_modparam),
			pfnDumpDebugPrintf, pvDumpDebugFile);

		apphint_dump_values("Debugfs Params", 0,
			init_data_debugfs, ARRAY_SIZE(init_data_debugfs),
			pfnDumpDebugPrintf, pvDumpDebugFile);

		for (i = 0; i < APPHINT_DEVICES_MAX; i++) {
			if (apphint.devices[i] != device)
				continue;

			result = snprintf(km_buffer,
					  APPHINT_BUFFER_SIZE,
					  "Debugfs Params Device ID: %d",
					  i);
			if (0 > result)
				continue;

			apphint_dump_values(km_buffer, i,
					    init_data_debugfs_device,
					    ARRAY_SIZE(init_data_debugfs_device),
					    pfnDumpDebugPrintf,
					    pvDumpDebugFile);
		}
	}
}

/*
*******************************************************************************
 Public interface
******************************************************************************/
int pvr_apphint_init(void)
{
	int result, i;

	if (apphint.initialized) {
		result = -EEXIST;
		goto err_out;
	}

	for (i = 0; i < APPHINT_DEVICES_MAX; i++)
		apphint.devices[i] = NULL;

	/* create workqueue with strict execution ordering to ensure no
	 * race conditions when setting/updating apphints from different
	 * contexts
	 */
	apphint.workqueue = alloc_workqueue("apphint_workqueue", WQ_UNBOUND, 1);
	if (!apphint.workqueue) {
		result = -ENOMEM;
		goto err_out;
	}

	result = apphint_debugfs_init("apphint", 0,
		ARRAY_SIZE(init_data_debugfs), init_data_debugfs,
		NULL,
		&apphint.debugfs_rootdir, apphint.debugfs_entry);
	if (0 != result)
		goto err_out;

	result = apphint_debugfs_init("buildvar", 0,
		ARRAY_SIZE(init_data_buildvar), init_data_buildvar,
		NULL,
		&apphint.buildvar_rootdir, apphint.buildvar_entry);

	apphint.initialized = 1;

err_out:
	return result;
}

int pvr_apphint_device_register(PVRSRV_DEVICE_NODE *device)
{
	int result, i;
	char device_num[APPHINT_BUFFER_SIZE];
	int device_value_offset;

	if (!apphint.initialized) {
		result = -EAGAIN;
		goto err_out;
	}

	if (apphint.num_devices+1 >= APPHINT_DEVICES_MAX) {
		result = -EMFILE;
		goto err_out;
	}

	result = snprintf(device_num, APPHINT_BUFFER_SIZE, "%d", apphint.num_devices);
	if (result < 0) {
		PVR_DPF((PVR_DBG_WARNING,
			"snprintf failed (%d)", result));
		result = -EINVAL;
		goto err_out;
	}

	/* Set the default values for the new device */
	device_value_offset = apphint.num_devices * APPHINT_DEBUGFS_DEVICE_ID_MAX;
	for (i = 0; i < APPHINT_DEBUGFS_DEVICE_ID_MAX; i++) {
		apphint.val[init_data_debugfs_device[i].id + device_value_offset].stored
			= init_data_debugfs_device[i].default_value;
	}


	result = apphint_debugfs_init(device_num, apphint.num_devices,
	                              ARRAY_SIZE(init_data_debugfs_device),
	                              init_data_debugfs_device,
	                              apphint.debugfs_rootdir,
	                              &apphint.debugfs_device_rootdir[apphint.num_devices],
	                              apphint.debugfs_device_entry[apphint.num_devices]);
	if (0 != result)
		goto err_out;

	apphint.devices[apphint.num_devices] = device;
	apphint.num_devices++;

	(void)PVRSRVRegisterDbgRequestNotify(
			&device->hAppHintDbgReqNotify,
			device,
			apphint_dump_state,
			DEBUG_REQUEST_APPHINT,
			device);

	/* Register flag handlers */
	register_rgx_flag_handlers(device);

err_out:
	return result;
}

void pvr_apphint_device_unregister(PVRSRV_DEVICE_NODE *device)
{
	int i;

	if (!apphint.initialized)
		return;

	/* find the device */
	for (i = 0; i < APPHINT_DEVICES_MAX; i++) {
		if (apphint.devices[i] == device)
			break;
	}

	if (APPHINT_DEVICES_MAX == i)
		return;

	if (device->hAppHintDbgReqNotify) {
		(void)PVRSRVUnregisterDbgRequestNotify(
			device->hAppHintDbgReqNotify);
		device->hAppHintDbgReqNotify = NULL;
	}

	apphint_debugfs_deinit(APPHINT_DEBUGFS_DEVICE_ID_MAX,
	                       &apphint.debugfs_device_rootdir[i],
	                       apphint.debugfs_device_entry[i]);

	apphint.devices[i] = NULL;
	apphint.num_devices--;
}

void pvr_apphint_deinit(void)
{
	int i;

	if (!apphint.initialized)
		return;

	/* remove any remaining device data */
	for (i = 0; apphint.num_devices && i < APPHINT_DEVICES_MAX; i++) {
		if (apphint.devices[i])
			pvr_apphint_device_unregister(apphint.devices[i]);
	}

	apphint_debugfs_deinit(APPHINT_DEBUGFS_ID_MAX,
			&apphint.debugfs_rootdir, apphint.debugfs_entry);
	apphint_debugfs_deinit(APPHINT_BUILDVAR_ID_MAX,
			&apphint.buildvar_rootdir, apphint.buildvar_entry);

	destroy_workqueue(apphint.workqueue);

	apphint.initialized = 0;
}

void pvr_apphint_dump_state(void)
{
#if defined(PDUMP)
	IMG_UINT32 ui32Flags = PDUMP_FLAGS_CONTINUOUS;

	apphint_dump_state(NULL, DEBUG_REQUEST_VERBOSITY_HIGH,
	                   apphint_pdump_values, (void *)&ui32Flags);
#endif
	apphint_dump_state(NULL, DEBUG_REQUEST_VERBOSITY_HIGH,
	                   NULL, NULL);
}

int pvr_apphint_get_uint64(APPHINT_ID ue, IMG_UINT64 *pVal)
{
	int error = -ERANGE;

	if (ue < APPHINT_ID_MAX) {
		*pVal = apphint.val[ue].stored.UINT64;
		error = 0;
	}
	return error;
}

int pvr_apphint_get_uint32(APPHINT_ID ue, IMG_UINT32 *pVal)
{
	int error = -ERANGE;

	if (ue < APPHINT_ID_MAX) {
		*pVal = apphint.val[ue].stored.UINT32;
		error = 0;
	}
	return error;
}

int pvr_apphint_get_bool(APPHINT_ID ue, IMG_BOOL *pVal)
{
	int error = -ERANGE;

	if (ue < APPHINT_ID_MAX) {
		error = 0;
		*pVal = apphint.val[ue].stored.BOOL;
	}


	return error;
}

void pvr_apphint_register_handlers_uint64(APPHINT_ID id,
	PVRSRV_ERROR (*query)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT64 *value),
	PVRSRV_ERROR (*set)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT64 value),
	const PVRSRV_DEVICE_NODE *device,
	const void *private_data)
{
	int device_value_offset;

	get_value_offset_from_device(device, &device_value_offset);

	switch (param_lookup[id].data_type) {
	case APPHINT_DATA_TYPE_UINT64:
		break;
	default:
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Does not match AppHint data type for ID (%d)",
		         __func__, id));
		return;
	}

	apphint.val[id + device_value_offset] = (struct apphint_action){
		.query.UINT64 = query,
		.set.UINT64 = set,
		.device = device,
		.private_data = private_data
	};
}

void pvr_apphint_register_handlers_uint32(APPHINT_ID id,
	PVRSRV_ERROR (*query)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT32 *value),
	PVRSRV_ERROR (*set)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_UINT32 value),
	const PVRSRV_DEVICE_NODE *device,
	const void *private_data)
{
	int device_value_offset;

	get_value_offset_from_device(device, &device_value_offset);

	switch (param_lookup[id].data_type) {
	case APPHINT_DATA_TYPE_UINT32:
	case APPHINT_DATA_TYPE_UINT32Bitfield:
	case APPHINT_DATA_TYPE_UINT32List:
	case APPHINT_DATA_TYPE_UINT32Array:
		break;

	default:
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Does not match AppHint data type for ID (%d)",
		         __func__, id));
		return;
	}

	apphint.val[id + device_value_offset] = (struct apphint_action){
		.query.UINT32 = query,
		.set.UINT32 = set,
		.device = device,
		.private_data = private_data
	};
}

void pvr_apphint_register_handlers_bool(APPHINT_ID id,
	PVRSRV_ERROR (*query)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_BOOL *value),
	PVRSRV_ERROR (*set)(const PVRSRV_DEVICE_NODE *device, const void *private_data, IMG_BOOL value),
	const PVRSRV_DEVICE_NODE *device,
	const void *private_data)
{
	int device_value_offset;

	get_value_offset_from_device(device, &device_value_offset);

	switch (param_lookup[id].data_type) {
	case APPHINT_DATA_TYPE_FLAG:
	case APPHINT_DATA_TYPE_BOOL:
		break;

	default:
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Does not match AppHint data type for ID (%d)",
		         __func__, id));
		return;
	}

	apphint.val[id + device_value_offset] = (struct apphint_action){
		.query.BOOL = query,
		.set.BOOL = set,
		.device = device,
		.private_data = private_data
	};
}

#endif /* #if defined(SUPPORT_KERNEL_SRVINIT) */
/* EOF */

