#ifndef _RSEE_MSG_H
#define _RSEE_MSG_H

#include <linux/bitops.h>
#include <linux/types.h>

/*
 * This file defines the RSEE message protocol used to communicate
 * with an instance of RSEE running in secure world.
 *
 * This file is divided into three sections.
 * 1. Formatting of messages.
 * 2. Requests from normal world
 * 3. Requests from secure world, Remote Procedure Call (RPC), handled by
 *    rsee_daemon.
 */

/*****************************************************************************
 * Part 1 - formatting of messages
 *****************************************************************************/

#define RSEE_MSG_ATTR_TYPE_NONE		0x0
#define RSEE_MSG_ATTR_TYPE_VALUE_INPUT		0x1
#define RSEE_MSG_ATTR_TYPE_VALUE_OUTPUT	0x2
#define RSEE_MSG_ATTR_TYPE_VALUE_INOUT		0x3
#define RSEE_MSG_ATTR_TYPE_RMEM_INPUT		0x5
#define RSEE_MSG_ATTR_TYPE_RMEM_OUTPUT		0x6
#define RSEE_MSG_ATTR_TYPE_RMEM_INOUT		0x7
#define RSEE_MSG_ATTR_TYPE_TMEM_INPUT		0x9
#define RSEE_MSG_ATTR_TYPE_TMEM_OUTPUT		0xa
#define RSEE_MSG_ATTR_TYPE_TMEM_INOUT		0xb

#define RSEE_MSG_ATTR_TYPE_MASK		GENMASK(7, 0)

/*
 * Meta parameter to be absorbed by the Secure OS and not passed
 * to the Trusted Application.
 *
 * Currently only used with RSEE_MSG_CMD_OPEN_SESSION.
 */
#define RSEE_MSG_ATTR_META			BIT(8)

/*
 * The temporary shared memory object is not physically contigous and this
 * temp memref is followed by another fragment until the last temp memref
 * that doesn't have this bit set.
 */
#define RSEE_MSG_ATTR_FRAGMENT			BIT(9)

/*
 * Memory attributes for caching passed with temp memrefs. The actual value
 * used is defined outside the message protocol with the exception of
 * RSEE_MSG_ATTR_CACHE_PREDEFINED which means the attributes already
 * defined for the memory range should be used. If rsee_smc.h is used as
 * bearer of this protocol RSEE_SMC_SHM_* is used for values.
 */
#define RSEE_MSG_ATTR_CACHE_SHIFT		16
#define RSEE_MSG_ATTR_CACHE_MASK		GENMASK(2, 0)
#define RSEE_MSG_ATTR_CACHE_PREDEFINED		0

/*
 * Same values as TEE_LOGIN_* from TEE Internal API
 */
#define RSEE_MSG_LOGIN_PUBLIC			0x00000000
#define RSEE_MSG_LOGIN_USER			0x00000001
#define RSEE_MSG_LOGIN_GROUP			0x00000002
#define RSEE_MSG_LOGIN_APPLICATION		0x00000004
#define RSEE_MSG_LOGIN_APPLICATION_USER	0x00000005
#define RSEE_MSG_LOGIN_APPLICATION_GROUP	0x00000006

/**
 * struct rsee_msg_param_tmem - temporary memory reference parameter
 * @buf_ptr:	Address of the buffer
 * @size:	Size of the buffer
 * @shm_ref:	Temporary shared memory reference, pointer to a struct tee_shm
 *
 * Secure and normal world communicates pointers as physical address
 * instead of the virtual address. This is because secure and normal world
 * have completely independent memory mapping. Normal world can even have a
 * hypervisor which need to translate the guest physical address (AKA IPA
 * in ARM documentation) to a real physical address before passing the
 * structure to secure world.
 */
struct rsee_msg_param_tmem {
	u64 buf_ptr;
	u64 size;
	u64 shm_ref;
};

/**
 * struct rsee_msg_param_rmem - registered memory reference parameter
 * @offs:	Offset into shared memory reference
 * @size:	Size of the buffer
 * @shm_ref:	Shared memory reference, pointer to a struct tee_shm
 */
struct rsee_msg_param_rmem {
	u64 offs;
	u64 size;
	u64 shm_ref;
};

/**
 * struct rsee_msg_param_value - opaque value parameter
 *
 * Value parameters are passed unchecked between normal and secure world.
 */
struct rsee_msg_param_value {
	u64 a;
	u64 b;
	u64 c;
};

/**
 * struct rsee_msg_param - parameter used together with struct rsee_msg_arg
 * @attr:	attributes
 * @tmem:	parameter by temporary memory reference
 * @rmem:	parameter by registered memory reference
 * @value:	parameter by opaque value
 *
 * @attr & RSEE_MSG_ATTR_TYPE_MASK indicates if tmem, rmem or value is used in
 * the union. RSEE_MSG_ATTR_TYPE_VALUE_* indicates value,
 * RSEE_MSG_ATTR_TYPE_TMEM_* indicates tmem and
 * RSEE_MSG_ATTR_TYPE_RMEM_* indicates rmem.
 * RSEE_MSG_ATTR_TYPE_NONE indicates that none of the members are used.
 */
struct rsee_msg_param {
	u64 attr;
	union {
		struct rsee_msg_param_tmem tmem;
		struct rsee_msg_param_rmem rmem;
		struct rsee_msg_param_value value;
	} u;
};

/**
 * struct rsee_msg_arg - call argument
 * @cmd: Command, one of RSEE_MSG_CMD_* or RSEE_MSG_RPC_CMD_*
 * @func: Trusted Application function, specific to the Trusted Application,
 *	     used if cmd == RSEE_MSG_CMD_INVOKE_COMMAND
 * @session: In parameter for all RSEE_MSG_CMD_* except
 *	     RSEE_MSG_CMD_OPEN_SESSION where it's an output parameter instead
 * @cancel_id: Cancellation id, a unique value to identify this request
 * @ret: return value
 * @ret_origin: origin of the return value
 * @num_params: number of parameters supplied to the OS Command
 * @params: the parameters supplied to the OS Command
 *
 * All normal calls to Trusted OS uses this struct. If cmd requires further
 * information than what these field holds it can be passed as a parameter
 * tagged as meta (setting the RSEE_MSG_ATTR_META bit in corresponding
 * attrs field). All parameters tagged as meta has to come first.
 *
 * Temp memref parameters can be fragmented if supported by the Trusted OS
 * (when rsee_smc.h is bearer of this protocol this is indicated with
 * RSEE_SMC_SEC_CAP_UNREGISTERED_SHM). If a logical memref parameter is
 * fragmented then has all but the last fragment the
 * RSEE_MSG_ATTR_FRAGMENT bit set in attrs. Even if a memref is fragmented
 * it will still be presented as a single logical memref to the Trusted
 * Application.
 */
struct rsee_msg_arg {
	u32 cmd;
	u32 func;
	u32 session;
	u32 cancel_id;
	u32 pad;
	u32 ret;
	u32 ret_origin;
	u32 num_params;

	/*
	 * this struct is 8 byte aligned since the 'struct rsee_msg_param'
	 * which follows requires 8 byte alignment.
	 *
	 * Commented out element used to visualize the layout dynamic part
	 * of the struct. This field is not available at all if
	 * num_params == 0.
	 *
	 * params is accessed through the macro RSEE_MSG_GET_PARAMS
	 *
	 * struct rsee_msg_param params[num_params];
	 */
} __aligned(8);

/**
 * RSEE_MSG_GET_PARAMS - return pointer to struct rsee_msg_param *
 *
 * @x: Pointer to a struct rsee_msg_arg
 *
 * Returns a pointer to the params[] inside a struct rsee_msg_arg.
 */
#define RSEE_MSG_GET_PARAMS(x) \
	(struct rsee_msg_param *)(((struct rsee_msg_arg *)(x)) + 1)

/**
 * RSEE_MSG_GET_ARG_SIZE - return size of struct rsee_msg_arg
 *
 * @num_params: Number of parameters embedded in the struct rsee_msg_arg
 *
 * Returns the size of the struct rsee_msg_arg together with the number
 * of embedded parameters.
 */
#define RSEE_MSG_GET_ARG_SIZE(num_params) \
	(sizeof(struct rsee_msg_arg) + \
	 sizeof(struct rsee_msg_param) * (num_params))

/*****************************************************************************
 * Part 2 - requests from normal world
 *****************************************************************************/

/*
 * Return the following UID if using API specified in this file without
 * further extensions:
 * 384fb3e0-e7f8-11e3-af63-0002a5d5c51b.
 * Represented in 4 32-bit words in RSEE_MSG_UID_0, RSEE_MSG_UID_1,
 * RSEE_MSG_UID_2, RSEE_MSG_UID_3.
 */
#define RSEE_MSG_UID_0			0x384fb3e0
#define RSEE_MSG_UID_1			0xe7f811e3
#define RSEE_MSG_UID_2			0xaf630002
#define RSEE_MSG_UID_3			0xa5d5c51b
#define RSEE_MSG_FUNCID_CALLS_UID	0xFF01

/*
 * Returns 2.0 if using API specified in this file without further
 * extensions. Represented in 2 32-bit words in RSEE_MSG_REVISION_MAJOR
 * and RSEE_MSG_REVISION_MINOR
 */
#define RSEE_MSG_REVISION_MAJOR	2
#define RSEE_MSG_REVISION_MINOR	0
#define RSEE_MSG_FUNCID_CALLS_REVISION	0xFF03

/*
 * Get UUID of Trusted OS.
 *
 * Used by non-secure world to figure out which Trusted OS is installed.
 * Note that returned UUID is the UUID of the Trusted OS, not of the API.
 *
 * Returns UUID in 4 32-bit words in the same way as
 * RSEE_MSG_FUNCID_CALLS_UID described above.
 */
#define RSEE_MSG_OS_RSEE_UUID_0	0x486178e0
#define RSEE_MSG_OS_RSEE_UUID_1	0xe7f811e3
#define RSEE_MSG_OS_RSEE_UUID_2	0xbc5e0002
#define RSEE_MSG_OS_RSEE_UUID_3	0xa5d5c51b
#define RSEE_MSG_FUNCID_GET_OS_UUID	0x0000

/*
 * Get revision of Trusted OS.
 *
 * Used by non-secure world to figure out which version of the Trusted OS
 * is installed. Note that the returned revision is the revision of the
 * Trusted OS, not of the API.
 *
 * Returns revision in 2 32-bit words in the same way as
 * RSEE_MSG_CALLS_REVISION described above.
 */
#define RSEE_MSG_OS_RSEE_REVISION_MAJOR	1
#define RSEE_MSG_OS_RSEE_REVISION_MINOR	0
#define RSEE_MSG_FUNCID_GET_OS_REVISION	0x0001

/*
 * Do a secure call with struct rsee_msg_arg as argument
 * The RSEE_MSG_CMD_* below defines what goes in struct rsee_msg_arg::cmd
 *
 * RSEE_MSG_CMD_OPEN_SESSION opens a session to a Trusted Application.
 * The first two parameters are tagged as meta, holding two value
 * parameters to pass the following information:
 * param[0].u.value.a-b uuid of Trusted Application
 * param[1].u.value.a-b uuid of Client
 * param[1].u.value.c Login class of client RSEE_MSG_LOGIN_*
 *
 * RSEE_MSG_CMD_INVOKE_COMMAND invokes a command a previously opened
 * session to a Trusted Application.  struct rsee_msg_arg::func is Trusted
 * Application function, specific to the Trusted Application.
 *
 * RSEE_MSG_CMD_CLOSE_SESSION closes a previously opened session to
 * Trusted Application.
 *
 * RSEE_MSG_CMD_CANCEL cancels a currently invoked command.
 *
 * RSEE_MSG_CMD_REGISTER_SHM registers a shared memory reference. The
 * information is passed as:
 * [in] param[0].attr			RSEE_MSG_ATTR_TYPE_TMEM_INPUT
 *					[| RSEE_MSG_ATTR_FRAGMENT]
 * [in] param[0].u.tmem.buf_ptr		physical address (of first fragment)
 * [in] param[0].u.tmem.size		size (of first fragment)
 * [in] param[0].u.tmem.shm_ref		holds shared memory reference
 * ...
 * The shared memory can optionally be fragmented, temp memrefs can follow
 * each other with all but the last with the RSEE_MSG_ATTR_FRAGMENT bit set.
 *
 * RSEE_MSG_CMD_UNREGISTER_SHM unregisteres a previously registered shared
 * memory reference. The information is passed as:
 * [in] param[0].attr			RSEE_MSG_ATTR_TYPE_RMEM_INPUT
 * [in] param[0].u.rmem.shm_ref		holds shared memory reference
 * [in] param[0].u.rmem.offs		0
 * [in] param[0].u.rmem.size		0
 */
#define RSEE_MSG_CMD_OPEN_SESSION	0
#define RSEE_MSG_CMD_INVOKE_COMMAND	1
#define RSEE_MSG_CMD_CLOSE_SESSION	2
#define RSEE_MSG_CMD_CANCEL		3
#define RSEE_MSG_CMD_REGISTER_SHM	4
#define RSEE_MSG_CMD_UNREGISTER_SHM	5
#define RSEE_MSG_FUNCID_CALL_WITH_ARG	0x0004

/*****************************************************************************
 * Part 3 - Requests from secure world, RPC
 *****************************************************************************/

/*
 * All RPC is done with a struct rsee_msg_arg as bearer of information,
 * struct rsee_msg_arg::arg holds values defined by RSEE_MSG_RPC_CMD_* below
 *
 * RPC communication with rsee_daemon is reversed compared to normal
 * client communication desribed above. The supplicant receives requests
 * and sends responses.
 */

/*
 * Load a TA into memory, defined in rsee_daemon
 */
#define RSEE_MSG_RPC_CMD_LOAD_TA	0

/*
 * Reserved
 */
#define RSEE_MSG_RPC_CMD_RPMB		1

/*
 * File system access, defined in rsee_daemon
 */
#define RSEE_MSG_RPC_CMD_FS		2

/*
 * Get time
 *
 * Returns number of seconds and nano seconds since the Epoch,
 * 1970-01-01 00:00:00 +0000 (UTC).
 *
 * [out] param[0].u.value.a	Number of seconds
 * [out] param[0].u.value.b	Number of nano seconds.
 */
#define RSEE_MSG_RPC_CMD_GET_TIME	3

/*
 * Wait queue primitive, helper for secure world to implement a wait queue
 *
 * Waiting on a key
 * [in] param[0].u.value.a RSEE_MSG_RPC_WAIT_QUEUE_SLEEP
 * [in] param[0].u.value.b wait key
 *
 * Waking up a key
 * [in] param[0].u.value.a RSEE_MSG_RPC_WAIT_QUEUE_WAKEUP
 * [in] param[0].u.value.b wakeup key
 */
#define RSEE_MSG_RPC_CMD_WAIT_QUEUE	4
#define RSEE_MSG_RPC_WAIT_QUEUE_SLEEP	0
#define RSEE_MSG_RPC_WAIT_QUEUE_WAKEUP	1

/*
 * Suspend execution
 *
 * [in] param[0].value	.a number of milliseconds to suspend
 */
#define RSEE_MSG_RPC_CMD_SUSPEND	5

/*
 * Allocate a piece of shared memory
 *
 * Shared memory can optionally be fragmented, to support that additional
 * spare param entries are allocated to make room for eventual fragments.
 * The spare param entries has .attr = RSEE_MSG_ATTR_TYPE_NONE when
 * unused. All returned temp memrefs except the last should have the
 * RSEE_MSG_ATTR_FRAGMENT bit set in the attr field.
 *
 * [in]  param[0].u.value.a		type of memory one of
 *					RSEE_MSG_RPC_SHM_TYPE_* below
 * [in]  param[0].u.value.b		requested size
 * [in]  param[0].u.value.c		required alignment
 *
 * [out] param[0].u.tmem.buf_ptr	physical address (of first fragment)
 * [out] param[0].u.tmem.size		size (of first fragment)
 * [out] param[0].u.tmem.shm_ref	shared memory reference
 * ...
 * [out] param[n].u.tmem.buf_ptr	physical address
 * [out] param[n].u.tmem.size		size
 * [out] param[n].u.tmem.shm_ref	shared memory reference (same value
 *					as in param[n-1].u.tmem.shm_ref)
 */
#define RSEE_MSG_RPC_CMD_SHM_ALLOC	6
/* Memory that can be shared with a non-secure user space application */
#define RSEE_MSG_RPC_SHM_TYPE_APPL	0
/* Memory only shared with non-secure kernel */
#define RSEE_MSG_RPC_SHM_TYPE_KERNEL	1

/*
 * Free shared memory previously allocated with RSEE_MSG_RPC_CMD_SHM_ALLOC
 *
 * [in]  param[0].u.value.a		type of memory one of
 *					RSEE_MSG_RPC_SHM_TYPE_* above
 * [in]  param[0].u.value.b		value of shared memory reference
 *					returned in param[0].u.tmem.shm_ref
 *					above
 */
#define RSEE_MSG_RPC_CMD_SHM_FREE	7

#define RSEE_MSG_RPC_CMD_DRV_BASE	0x1000
#define RSEE_DRV_RPC_CMD(x)	(RSEE_MSG_RPC_CMD_DRV_BASE + x)

#define RSEE_MSG_RPC_CMD_SEC_SPI_CLK_ENABLE		RSEE_DRV_RPC_CMD(0)
#define RSEE_MSG_RPC_CMD_SEC_SPI_CLK_DISABLE	RSEE_DRV_RPC_CMD(1)
#define RSEE_MSG_RPC_CMD_SEND_DEBUG_INFO		RSEE_DRV_RPC_CMD(2)
#endif /* _RSEE_MSG_H */
