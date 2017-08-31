#include <teei_client_api.h>
#include <teei_client.h>

TEEIC_Result TEEIC_InitializeContext(
    const char*   name,
    TEEIC_Context* context
)
{
    return TEEC_InitializeContext(name,context);
}


void TEEIC_FinalizeContext(
    TEEIC_Context* context)
{
    TEEC_FinalizeContext(context);
}

TEEIC_Result TEEIC_RegisterSharedMemory(
    TEEIC_Context*      context,
    TEEIC_SharedMemory* sharedMem)
{
    return TEEC_RegisterSharedMemory(context,sharedMem);
}


TEEIC_Result TEEIC_AllocateSharedMemory(
    TEEIC_Context*      context,
    TEEIC_SharedMemory* sharedMem)
{
    return TEEC_AllocateSharedMemory(context,sharedMem);
}


void TEEIC_ReleaseSharedMemory(
    TEEIC_SharedMemory* sharedMem)
{
    TEEC_ReleaseSharedMemory(sharedMem);
}

TEEIC_Result TEEIC_OpenSession (
    TEEIC_Context*    context,
    TEEIC_Session*    session,
    const TEEIC_UUID* destination,
    uint32_t         connectionMethod,
    const void*      connectionData,
    TEEIC_Operation* operation,
    uint32_t*        returnOrigin)
{
    return TEEC_OpenSession(context,session,destination,connectionMethod,
                            connectionData,operation,returnOrigin);
}


void TEEIC_CloseSession (
    TEEIC_Session*    session)
{
    TEEC_CloseSession(session);
}

TEEIC_Result TEEIC_InvokeCommand(
    TEEIC_Session*     session,
    uint32_t          commandID,
    TEEIC_Operation*   operation,
    uint32_t*         returnOrigin)
{
    return TEEC_InvokeCommand(session,commandID,operation,returnOrigin);
}


void TEEIC_RequestCancellation(
    TEEIC_Operation* operation)
{
    TEEC_RequestCancellation(operation);
}



