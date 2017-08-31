#ifndef NEU_TEE_API_H
#define NEU_TEE_API_H
/*define TYPE_UINT_DEFINED*/
#ifndef TYPE_UINT_DEFINED
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
#ifndef _STDINT_H
typedef char uint8_t;
#endif
#endif


/**
* @brief Error codes
*
*/
enum TEEIC_Result {
/*!The operation succeeded. \n*/
	TEEC_SUCCESS = 0x0,
	TEEIC_SUCCESS = 0x0,
	TEEC_ERROR_INTERNAL	= 0x00000001,
	TEEC_ERROR_RESOURCE_LIMIT = 0x00000002,
	TEEC_ERROR_DEVICE_BUSY = 0x00000003,
	TEEC_ERROR_DEVICE_HANDLE_INVALID = 0x00000004,
	TEEC_ERROR_DEVICE_OVERFLOW = 0x00000005,
	TEEC_ERROR_DEVICE_EVENT_INVALID	= 0x00000006,
	TEEC_ERROR_TA_NOT_EXIST = 0x00000009,
	TEEC_ERROR_HOST_NAME_EXIST = 0x0000000a,
	TEEC_ERROR_HOST_NOT_EXIST = 0x0000000b,
	TEEC_ERROR_GATE_NOT_EXIST = 0x0000000c,
	TEEC_ERROR_PIPE_CREATE_DENIED = 0x0000000d,
	TEEC_ERROR_PIPE_BUSY = 0x0000000e,
	TEEC_ERROR_NETWORK_ERROR = 0x00000010,
	TEEC_ERROR_CIPHERTEXT_ERROR = 0x00000011,
	TEEC_ERROR_UNIQUE_CONFLICT = 0x00000012,
/*!Non-specific cause.*/
	TEEC_ERROR_GENERIC = 0xFFFF0000,
	TEEIC_ERROR_GENERIC = 0xFFFF0000,
/*!Access privileges are not sufficient.*/
	TEEC_ERROR_ACCESS_DENIED = 0xFFFF0001,
	TEEIC_ERROR_ACCESS_DENIED = 0xFFFF0001,
/*!The operation was cancelled.*/
	TEEC_ERROR_CANCEL = 0xFFFF0002,
	TEEIC_ERROR_CANCEL = 0xFFFF0002,
/*!Concurrent accesses caused conflict.*/
	TEEC_ERROR_ACCESS_CONFLICT = 0xFFFF0003,
	TEEIC_ERROR_ACCESS_CONFLICT = 0xFFFF0003,
/*!Too much data for the requested operation was passed.*/
	TEEC_ERROR_EXCESS_DATA = 0xFFFF0004,
	TEEIC_ERROR_EXCESS_DATA = 0xFFFF0004,
/*!Input data was of invalid format.*/
	TEEC_ERROR_BAD_FORMAT = 0xFFFF0005,
	TEEIC_ERROR_BAD_FORMAT = 0xFFFF0005,
/*!Input parameters were invalid.*/
	TEEC_ERROR_BAD_PARAMETERS = 0xFFFF0006,
	TEEIC_ERROR_BAD_PARAMETERS = 0xFFFF0006,
/*!Operation is not valid in the current state.*/
	TEEC_ERROR_BAD_STATE = 0xFFFF0007,
	TEEIC_ERROR_BAD_STATE = 0xFFFF0007,
/*!The requested data item is not found.*/
	TEEC_ERROR_ITEM_NOT_FOUND = 0xFFFF0008,
	TEEIC_ERROR_ITEM_NOT_FOUND = 0xFFFF0008,
/*!The requested operation should exist but is not yet implemented.*/
	TEEC_ERROR_NOT_IMPLEMENTED = 0xFFFF0009,
	TEEIC_ERROR_NOT_IMPLEMENTED = 0xFFFF0009,
/*!The requested operation is valid but is not supported in this
* Implementation.
*/
	TEEC_ERROR_NOT_SUPPORTED = 0xFFFF000A,
	TEEIC_ERROR_NOT_SUPPORTED = 0xFFFF000A,
/*!Expected data was missing.*/
	TEEC_ERROR_NO_DATA = 0xFFFF000B,
	TEEIC_ERROR_NO_DATA = 0xFFFF000B,
/*!System ran out of resources.*/
	TEEC_ERROR_OUT_OF_MEMORY = 0xFFFF000C,
	TEEIC_ERROR_OUT_OF_MEMORY = 0xFFFF000C,
/*!The system is busy working on something else. */
	TEEC_ERROR_BUSY = 0xFFFF000D,
	TEEIC_ERROR_BUSY = 0xFFFF000D,
/*!Communication with a remote party failed.*/
	TEEC_ERROR_COMMUNICATION = 0xFFFF000E,
	TEEIC_ERROR_COMMUNICATION = 0xFFFF000E,
/*!A security fault was detected.*/
	TEEC_ERROR_SECURITY = 0xFFFF000F,
	TEEIC_ERROR_SECURITY = 0xFFFF000F,
/*!The supplied buffer is too short for the generated output.*/
	TEEC_ERROR_SHORT_BUFFER = 0xFFFF0010,
	TEEIC_ERROR_SHORT_BUFFER = 0xFFFF0010,
/*! The MAC value supplied is different from the one calculated */
	TEEC_ERROR_MAC_INVALID = 0xFFFF3071,
	TEEIC_ERROR_MAC_INVALID = 0xFFFF3071,

    /* add by luocl start*/
	TEEC_ERROR_VNET_INVALID = 0xFFFF6000,
	TEEIC_ERROR_VNET_INVALID = 0xFFFF6000,
	TEEC_ERROR_REEAGENT_INVALID = 0xFFFF6001,
	TEEIC_ERROR_REEAGENT_INVALID = 0xFFFF6001,
    /* add by luocl end*/

};

/* mod by luocl 20150112*/
#ifndef TEE_RESULT
#define TEE_RESULT
/*typedef uint32_t TEE_Result;*/
#endif
/*typedef TEE_Result TEEC_Result;*/



#endif
