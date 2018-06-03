#ifndef __TEEI_CLIENT_API_H_
#define __TEEI_CLIENT_API_H_
#include "tee_client_api.h"
#define TEEIC_PARAM_TYPES(param0Type, param1Type, param2Type, param3Type) \
	(param3Type << 12 | param2Type << 8 | param1Type << 4 | param0Type)
/*
*typedef uint32_t TEEIC_Result;
*typedef TEEC_UUID TEEIC_UUID;
*typedef TEEC_Context TEEIC_Context;
*typedef TEEC_Session TEEIC_Session;
*typedef TEEC_SharedMemory TEEIC_SharedMemory;
*typedef TEEC_TempMemoryReference TEEIC_TempMemoryReference;
*typedef TEEC_Value TEEIC_Value;
*typedef TEEC_Parameter TEEIC_Parameter;
*typedef TEEC_Operation TEEIC_Operation;
*/

TEEIC_Result TEEIC_InitializeContext(
	const char *name,
	TEEIC_Context *context);



void TEEIC_FinalizeContext(
	TEEIC_Context *context);

TEEIC_Result TEEIC_RegisterSharedMemory(
	TEEIC_Context *context,
	TEEIC_SharedMemory *sharedMem);


TEEIC_Result TEEIC_AllocateSharedMemory(
	TEEIC_Context *context,
	TEEIC_SharedMemory *sharedMem);


void TEEIC_ReleaseSharedMemory(
	TEEIC_SharedMemory *sharedMem);

TEEIC_Result TEEIC_OpenSession(
	TEEIC_Context	*context,
	TEEIC_Session	*session,
	const TEEIC_UUID *destination,
	uint32_t         connectionMethod,
	const void		*connectionData,
	TEEIC_Operation *operation,
	uint32_t		*returnOrigin);

void TEEIC_CloseSession(
	TEEIC_Session *session);

TEEIC_Result TEEIC_InvokeCommand(
	TEEIC_Session *session,
	uint32_t          commandID,
	TEEIC_Operation *operation,
	uint32_t		*returnOrigin);


void TEEIC_RequestCancellation(
	TEEIC_Operation *operation);

void allow(TEEC_Operation *operation);
void block(TEEC_Operation *operation);

#endif

