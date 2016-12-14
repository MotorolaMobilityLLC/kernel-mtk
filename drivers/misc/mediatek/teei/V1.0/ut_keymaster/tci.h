/*
 
 */
#ifndef __TCI_H__
#define __TCI_H__


typedef uint32_t tciCommandId_t;
typedef uint32_t tciResponseId_t;
typedef uint32_t tciReturnCode_t;


/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)


/**
 * Return codes
 */
#define RET_OK                    0
#define RET_ERR_UNKNOWN_CMD       1
#define RET_ERR_NOT_SUPPORTED     2
#define RET_ERR_INVALID_BUFFER    3
#define RET_ERR_INVALID_KEY_SIZE  4
#define RET_ERR_INVALID_KEY_TYPE  5
#define RET_ERR_INVALID_LENGTH    6
#define RET_ERR_INVALID_EXPONENT  7
#define RET_ERR_INVALID_CURVE     8
#define RET_ERR_KEY_GENERATION    9
#define RET_ERR_SIGN              10
#define RET_ERR_VERIFY            11
#define RET_ERR_DIGEST            12
#define RET_ERR_SECURE_OBJECT     13
#define RET_ERR_INTERNAL_ERROR    14
#define RET_ERR_OUT_OF_MEMORY     15
/* ... add more error codes when needed */


/**
 * TCI command header.
 */
typedef struct{
    tciCommandId_t commandId; /**< Command ID */
} tciCommandHeader_t;


/**
 * TCI response header.
 */
typedef struct{
    tciResponseId_t     responseId; /**< Response ID (must be command ID | RSP_ID_MASK )*/
    tciReturnCode_t     returnCode; /**< Return code of command */
} tciResponseHeader_t;

#endif // __TCI_H__

