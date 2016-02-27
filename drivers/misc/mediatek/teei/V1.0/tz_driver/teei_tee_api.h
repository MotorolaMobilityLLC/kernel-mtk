
#ifndef TEEI_TEE_API_H
#define TEEI_TEE_API_H
#include <teei_common_types.h>
#define TYPE_UINT_DEFINED
#ifndef TYPE_UINT_DEFINED
#define uint32_t (unsigned int)
#define uint16_t (unsigned short)
#ifndef _STDINT_H
#define uint8_t (char)
#endif
#endif


/**
* @brief Error codes
*
*/
enum TEEC_Result {
	/*!The operation succeeded. \n*/
	TEEC_SUCCESS = 0x0,
	/*!Non-specific cause.*/
	TEEC_ERROR_GENERIC = 0xFFFF0000,
	/*!Access privileges are not sufficient.*/
	TEEC_ERROR_ACCESS_DENIED = 0xFFFF0001 ,
	/*!The operation was cancelled.*/
	TEEC_ERROR_CANCEL = 0xFFFF0002 ,
	/*!Concurrent accesses caused conflict.*/
	TEEC_ERROR_ACCESS_CONFLICT = 0xFFFF0003 ,
	/*!Too much data for the requested operation was passed.*/
	TEEC_ERROR_EXCESS_DATA = 0xFFFF0004 ,
	/*!Input data was of invalid format.*/
	TEEC_ERROR_BAD_FORMAT = 0xFFFF0005 ,
	/*!Input parameters were invalid.*/
	TEEC_ERROR_BAD_PARAMETERS = 0xFFFF0006 ,
	/*!Operation is not valid in the current state.*/
	TEEC_ERROR_BAD_STATE = 0xFFFF0007,
	/*!The requested data item is not found.*/
	TEEC_ERROR_ITEM_NOT_FOUND = 0xFFFF0008,
	/*!The requested operation should exist but is not yet implemented.*/
	TEEC_ERROR_NOT_IMPLEMENTED = 0xFFFF0009,
	/*!The requested operation is valid but is not supported in this
	* Implementation.*/
	TEEC_ERROR_NOT_SUPPORTED = 0xFFFF000A,
	/*!Expected data was missing.*/
	TEEC_ERROR_NO_DATA = 0xFFFF000B,
	/*!System ran out of resources.*/
	TEEC_ERROR_OUT_OF_MEMORY = 0xFFFF000C,
	/*!The system is busy working on something else. */
	TEEC_ERROR_BUSY = 0xFFFF000D,
	/*!Communication with a remote party failed.*/
	TEEC_ERROR_COMMUNICATION = 0xFFFF000E,
	/*!A security fault was detected.*/
	TEEC_ERROR_SECURITY = 0xFFFF000F,
	/*!The supplied buffer is too short for the generated output.*/
	TEEC_ERROR_SHORT_BUFFER = 0xFFFF0010,
	/*! The MAC value supplied is different from the one calculated */
	TEEC_ERROR_MAC_INVALID = 0xFFFF3071,
};

#define TEE_Result (uint32_t)
#define TEEC_Result TEE_Result

#endif
