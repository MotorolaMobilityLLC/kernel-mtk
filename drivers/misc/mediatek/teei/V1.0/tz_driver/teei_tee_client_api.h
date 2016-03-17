
#ifndef __TEEI_TEE_CLIENT_API_H_
#define __TEEI_TEE_CLIENT_API_H_

#define MAX_SESSIONS_PER_DEVICE 16
#define MAX_OPERATIONS_PER_SESSION 16
#define MAX_MEMBLOCKS_PER_SESSION 16
#define MAX_MEMBLOCKS_PER_OPERATION 4


#define TEEC_PARAM_TYPES(param0Type, param1Type, param2Type, param3Type) \
	(param3Type << 12 | param2Type << 8 | param1Type << 4 | param0Type)

#define TEEC_VALUE_UNDEF 0xffffffff
#include <teei_list.h>
#include <teei_tee_api.h>

#define D(fmt, ...) \
fprintf(stderr, "[%s-%d]"fmt"\n",\
strrchr(__FILE__, '/')+1, __LINE__, ##__VA_ARGS__);

#define I(fmt, ...) \
fprintf(stderr, "[%s-%d]"fmt"\n",\
strrchr(__FILE__, '/')+1, __LINE__, ##__VA_ARGS__);

/**
* @brief Return code origin
*
*
*/
enum TEEC_return_code_origin {
	/*! The return code is an error that originated within the TEE Client API
	* implementation. */
	TEEC_ORIGIN_API = 0x1,
	/*! The return code is an error that originated within the underlying
	* communications stack linking the rich OS with the TEE. */
	TEEC_ORIGIN_COMMS = 0x2,
	/*! The return code is an error that originated within the common TEE code. */
	TEEC_ORIGIN_TEE = 0x3,
	/*! The return code is an error that originated within the Trusted application
	* code. This includes the case where the return code is a success. */
	TEEC_ORIGIN_TRUSTED_APP = 0x4,
};


/**
* @brief Login flag constants
*
*
*/
enum TEEC_login_flags {
	/*! No login is to be used.*/
	TEEC_LOGIN_PUBLIC = 0x0,
	/*! The user executing the application is provided.*/
	TEEC_LOGIN_USER ,
	/*! The user group executing the application is provided.*/
	TEEC_LOGIN_GROUP ,
	/*! Login data about the running Client Application itself is provided. */
	TEEC_LOGIN_APPLICATION = 0x4 ,
	/*! Login data about the user running the Client Application and about the
	* Client Application itself is provided. */
	TEEC_LOGIN_USER_APPLICATION = 0x5 ,
	/*! Login data about the group running the Client Application and about the
	* Client Application itself is provided. */
	TEEC_LOGIN_GROUP_APPLICATION = 0x6 ,
};

/**
* @brief Shared memory flag constants
*
*
*/
enum TEEC_shared_mem_flags {
	/*! The Shared Memory can carry data from the Client Application
	* to the Trusted Application. */
	TEEC_MEM_INPUT = 0x1,
	/*! The Shared Memory can carry data from the Trusted Application
	* to the Client Application. */
	TEEC_MEM_OUTPUT = 0x2,
};

/**
* @brief Param type constants
*
*/
enum TEEC_param_type {
	/*! The Parameter is not used. */
	TEEC_NONE = 0x1,
	/*! The Parameter is a TEEC_Value tagged as input. */
	TEEC_VALUE_INPUT,
	/*! The Parameter is a TEEC_Value tagged as output. */
	TEEC_VALUE_OUTPUT,
	/*! The Parameter is a TEEC_Value tagged as both as input and output,
	* i.e., for which both the behaviors of TEEC_VALUE_INPUT and
	* TEEC_VALUE_OUTPUT apply. */
	TEEC_VALUE_INOUT,
	/*! The Parameter is a TEEC_TempMemoryReference describing a region of memory
	* which needs to be temporarily registered for the duration of the Operation
	and is tagged as input. */
	TEEC_MEMREF_TEMP_INPUT,
	/*! Same as TEEC_MEMREF_TEMP_INPUT, but the Memory Reference is tagged as
	* output. The Implementation may update the size field to reflect the
	* required output size in some use cases. */
	TEEC_MEMREF_TEMP_OUTPUT,
	/*! A Temporary Memory Reference tagged as both input and output,
	* i.e., for which both the behaviors of TEEC_MEMREF_TEMP_INPUT and
	* TEEC_MEMREF_TEMP_OUTPUT apply. */
	TEEC_MEMREF_TEMP_INOUT,
	/*! The Parameter is a Registered Memory Reference that refers to the
	* entirety of its parent Shared Memory block. The parameter structure is a
	* TEEC_MemoryReference. In this structure, the Implementation MUST read
	* only the parent field and MAY update the size field when the
	* operation completes. */
	TEEC_MEMREF_WHOLE = 0xc,

	/*! A Registered Memory Reference structure that refers to a partial region
	* of its parent Shared Memory block and is tagged as input.
	*/
	TEEC_MEMREF_PARTIAL_INPUT = 0xd,

	/*! A Registered Memory Reference structure that refers to a partial region
	* of its parent Shared Memory block and is tagged as output.
	*/
	TEEC_MEMREF_PARTIAL_OUTPUT = 0xe,

	/*! A Registered Memory Reference structure that refers to a partial region
	* of its parent Shared Memory block and is tagged as both input and output.
	*/
	TEEC_MEMREF_PARTIAL_INOUT = 0xf
};


/**
* @brief Universally Unique IDentifier (UUID) type as defined in
* [RFC4122].A
*
* UUID is the mechanism by which a service is identified.
*/
struct TEEC_UUID {
	uint32_t timeLow;
	uint16_t timeMid;
	uint16_t timeHiAndVersion;
	uint8_t clockSeqAndNode[8];
};

/* typedef uint32_t TEEC_UUID; */

/**
* @brief The TEEC_Session structure is used to contain control information
* related to a session between a client and a service.
*
*/
struct TEEC_Session {
	/*! Implementation-defined variables */
	/*! Reference count of operations*/
	int operation_cnt;
	/*! Session id obtained for the  service*/
	int session_id;
	/*! Unique service id */
	int service_id;
	/*! Device context */
	struct TEEC_Context *device;
	/*! Service error number */
	int s_errno;
};


/**
* @brief The TEEC_Context structure is used to contain control information
* related to the TEE
*
*/
struct TEEC_Context {
	/*! Implementation-defined variables */
	/*! Device identifier */
	uint32_t fd;
	/*! Sessions count of the device */
	int session_count;
	/*! Shared memory counter which got created for this context */
	uint32_t shared_mem_cnt;
	/*! Shared memory list */
	struct list shared_mem_list;
	/*! Error number from the client driver */
	int s_errno;
	/* add by lodovico */
	char tee_name[255];
	uint32_t tee_id;
	/* add end */
};


/**
* @brief The TEEC_SharedMemory structure is used to contain control information
* related to a block of shared memory that is mapped between the client and the
* service.
*
*/

struct TEEC_SharedMemory {
	/*! The pointer to the block of shared memory. */
	void *buffer;

	/*! The length of the shared memory block in bytes. Should not be zero */
	size_t size;

	/*! flags is a bit-vector which can contain the following flags:\n
	*  TEEC_MEM_INPUT: the memory can be used to transfer data from the
	*  Client Application to the TEE. \n
	*  TEEC_MEM_OUTPUT: The memory can be used to transfer data from the
	*  TEE to the Client Application. \n
	*  All other bits in this field SHOULD be set to zero, and are reserved for
	*  future use.
	*/
	uint32_t flags;

	/*! Implementation defined fields. */

	/*! Device context */
	struct TEEC_Context *context;
	/*! Operation count */
	int operation_count;
	/*! Shared memory type */
	uint32_t allocated;
	/*! List head used by Context */
	struct list head_ref;
	/*! Service error number */
	int s_errno;
};


/**
* @brief Small raw data value type
*
* This type defines a parameter that is not referencing shared memory,
* but carries instead small raw data passed by value.
* It is used as a TEEC_Operation parameter when the corresponding
* parameter type is one of
* TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT, or TEEC_VALUE_INOUT.
*/
struct TEEC_Value {
	/*! The two fields of this structure do not have a particular meaning.
	* It is up to the protocol between the Client Application and
	* the Trusted Application to assign a semantic to those two integers.
	*/
	uint32_t a;
	uint32_t b;
};


/**
* @brief Temporary shared memory reference
*
*/
struct TEEC_TempMemoryReference {
	/*! "buffer" is a pointer to the first byte of a region of memory which needs\n
	* to be temporarily registered for the duration of the Operation.\n
	* This field can be NULL to specify a null Memory Reference. */
	void            *buffer;
	/*! Size of the referenced memory region. When the operation completes, and\n
	*   unless the parameter type is TEEC_MEMREF_TEMP_INPUT,\n
	*   the Implementation must update this field to reflect the actual or\n
	*   required size of the output:\n
	*   If the Trusted Application has actually written some data in the
	*   output buffer, then the Implementation MUST update the size field with
	*   the actual number of bytes written.\n\n
	*   If the output buffer was not large enough to contain the whole output,
	*   or if it is null, the Implementation MUST update the size field with
	*   the size of the output buffer requested by the Trusted Application.
	*   In this case, no data has been written into the output buffer
	*/
	size_t        size;
};

/**
* @brief Registered memory reference
*
* A pre-registered or pre-allocated Shared Memory block.
* It is used as a TEEC_Operation parameter when the corresponding
* parameter type is one of TEEC_MEMREF_WHOLE,
* TEEC_MEMREF_PARTIAL_INPUT, TEEC_MEMREF_PARTIAL_OUTPUT, or
* TEEC_MEMREF_PARTIAL_INOUT.
*/
struct TEEC_RegisteredMemoryReference {
	/*! Pointer to the shared memory structure. \n
	* The memory reference refers either to the whole Shared Memory or
	* to a partial region within the Shared Memory block, depending of the
	* parameter type. The data flow direction of the memory reference
	* must be consistent with the flags defined in the parent Shared Memory Block.
	* Note that the parent field MUST NOT be NULL. To encode a null
	* Memory Reference, the Client Application must use a Temporary Memory
	* Reference with the buffer field set to NULL. */
	struct TEEC_SharedMemory *parent;

	/*! Size of the referenced memory region, in bytes.\n
	* The Implementation MUST only interpret this field if the Memory Reference
	* type in the operation structure is not TEEC_MEMREF_WHOLE. Otherwise,
	* the size is read from the parent Shared Memory structure.\n
	* When an operation completes, and if the Memory Reference is
	* tagged as “output”, the Implementation must update this field to reflect
	* the actual or required size of the output. This applies even if the
	* parameter type is TEEC_MEMREF_WHOLE:\n
	* If the Trusted Application has actually written some data in the
	* output buffer, then the Implementation MUST update the size field with the
	* actual number of bytes written.\n
	* If the output buffer was not large enough to contain the
	* whole output, the Implementation MUST update the size field with the size of
	* the output buffer requested by the Trusted Application. In this case,
	* no data has been written into the output buffer.
	*/
	size_t        size;

	/*! Offset from the allocated Shared memory for reference\n
	* The Implementation MUST only interpret this field if the
	* Memory Reference type in the operation structure is not TEEC_MEMREF_WHOLE.
	* Otherwise, the Implementation MUST use the base address of the
	* Shared Memory block.
	*/
	size_t        offset;
};

/**
* @brief Parameter of a TEEC_Operation
*
* It can be a Temporary Memory Reference, a Registered Memory Reference,
* or a Value Parameter.
*/
union TEEC_Parameter {
	/*! For parameter type:\n
	*   TEEC_MEMREF_TEMP_INPUT\n
	*   TEEC_MEMREF_TEMP_OUTPUT\n
	*   TEEC_MEMREF_TEMP_INOUT\n
	*/
	struct TEEC_TempMemoryReference       tmpref;

	/*! For parameter type:\n
	*   TEEC_MEMREF_WHOLE\n
	*   TEEC_MEMREF_PARTIAL_INPUT\n
	*   TEEC_MEMREF_PARTIAL_OUTPUT\n
	*   TEEC_MEMREF_PARTIAL_INOUT\n
	*/
	struct TEEC_RegisteredMemoryReference memref;

	/*! For parameter type:\n
	*   TEEC_VALUE_INPUT\n
	*   TEEC_VALUE_OUTPUT\n
	*   TEEC_VALUE_INOUT\n
	*/
	struct TEEC_Value                     value;
};

/**
* @brief The TEEC_Operation structure is used to contain control information
* related to an operation that is to be invoked with the security environment.
*
* This type defines the payload of either an open Session operation or
* an invoke Command operation. It is also used for cancellation of operations,
* which may be desirable even if no payload is passed.
*/
struct TEEC_Operation {
	/*!
	* This field which MUST be initialized to zero by the Client Application
	* before each use in an operation if the Client Application may need to
	* cancel the operation about to be performed.
	*/
	uint32_t started;

	/*! paramTypes field encodes the type of each of the Parameters in the
	* operation. The layout of these types within a 32-bit integer is
	* implementation-defined and the Client Application MUST use the
	* macro TEEC_PARAMS_TYPE to construct a constant value for this field.
	* As a special case, if the Client Application sets paramTypes to 0,
	* then the Implementation MUST interpret it as meaning that the type for each
	* Parameter is set to TEEC_NONE.\n
	* The type of each Parameter can take one of the following values\n
	* TEEC_NONE\n
	* TEEC_VALUE_INPUT\n
	* TEEC_VALUE_OUTPUT\n
	* TEEC_VALUE_INOUT\n
	* TEEC_MEMREF_TEMP_INPUT\n
	* TEEC_MEMREF_TEMP_OUTPUT\n
	* TEEC_MEMREF_TEMP_INOUT\n
	* TEEC_MEMREF_WHOLE\n
	* TEEC_MEMREF_PARTIAL_INPUT\n
	* TEEC_MEMREF_PARTIAL_OUTPUT\n
	* TEEC_MEMREF_PARTIAL_INOUT\n
	*/
	uint32_t paramTypes;

	/*! params is an array of four Parameters. For each parameter, one of the
	* memref, tmpref, or value fields must be used depending on the corresponding
	* parameter type passed in paramTypes as described in the specification
	* of TEEC_Parameter
	*/
	union TEEC_Parameter params[4];
};


/**
* @brief Shared memory flag constants
*
*
*/
enum teei_shared_mem_flags {
	/*! Service can only read from the memory block.*/
	TEEI_MEM_SERVICE_RO = 0x0,
	/*! Service can only write from the memory block.*/
	TEEI_MEM_SERVICE_WO ,
	/*! Service can read and write from the memory block.*/
	TEEI_MEM_SERVICE_RW,
	/*! Invalid flag */
	TEEI_MEM_SERVICE_UNDEFINED
};


/**
* @brief Initialize Context
*
* This function initializes a new TEE Context, forming a connection between
* this Client Application and the TEE identified by the string identifier
* name.\n
* The Client Application MAY pass a NULL name, which means that the
* Implementation MUST select a default TEE to connect to.
* The supported name strings, the mapping of these names to a specific TEE,
* and the nature of the default TEE are implementation-defined.\n
* The caller MUST pass a pointer to a valid TEEC Context in context.
* The Implementation MUST assume that all fields of the TEEC_Context structure
* are in an undefined state.\n
*
* \b Programmer \b Error\n
* The following usage of the API is a programmer error:\n
* Attempting to initialize the same TEE Context structure concurrently
* from multiple threads. Multi-threaded Client Applications must use
* platform-provided locking mechanisms to ensure that this case
* does not occur.\n\n
* \b Implementers’ \b Notes\n
* It is valid Client Application behavior to concurrently initialize
* different TEE Contexts, so the Implementation MUST support this.
*
* @param name: A zero-terminated string that describes the TEE to connect to.
* If this parameter is set to NULL the Implementation MUST select a default TEE.
*
* @param context: A TEEC_Context structure that MUST be initialized by the
* Implementation.
*
* @return TEEC_Result:
* TEEC_SUCCESS: The initialization was successful.\n
* TEEC_ERROR_*: An implementation-defined error code for any other error.
*
*
*/
TEEC_Result TEEC_InitializeContext(
	const char *name,
	struct TEEC_Context *context);



/**
* @brief Finalizes an initialized TEE context.
*
* This function finalizes an initialized TEE Context,
* closing the connection between the Client Application and the TEE.
* The Client Application MUST only call this function when all Sessions
* inside this TEE Context have been closed and all
* Shared Memory blocks have been released.\n
* The implementation of this function MUST NOT be able to fail:
* after this function returns the Client Application must be able to
* consider that the Context has been closed.\n
* The function implementation MUST do nothing if context is NULL.
*
* \b Programmer \b Error \n
* The following usage of the API is a programmer error:\n
*       Calling with a context which still has sessions opened.\n
*       Calling with a context which contains unreleased Shared Memory blocks.\n
*       Attempting to finalize the same TEE Context structure concurrently
* from multiple threads.\n
*       Attempting to finalize the same TEE Context structure more than once,
* without an intervening call to TEEC_InitalizeContext.
*
* @param context: An initialized TEEC_Context structure which is to be
* finalized.
*
*/
void TEEC_FinalizeContext(
	struct TEEC_Context *context);

/**
* @brief Register a allocated shared memory block.
*
* This function registers a block of existing Client Application memory as a
* block of Shared Memory within the scope of the specified TEE Context,
* in accordance with the parameters which have been set by the
* Client Application inside the \a sharedMem structure.
*
* The parameter \a context MUST point to an initialized TEE Context.
*
* The parameter \a sharedMem MUST point to the Shared Memory structure
* defining the memory region to register.
* The Client Application MUST have populated the following fields of the
* Shared Memory structure before calling this function:\n
* The \a buffer field MUST point to the memory region to be shared,
* and MUST not be NULL.\n
* The \a size field MUST contain the size of the buffer, in bytes.
* Zero is a valid length for a buffer.\n
* The \a flags field indicates the intended directions of data flow
* between the Client Application and the TEE.\n
* The Implementation MUST assume that all other fields in the Shared Memory
* structure have undefined content.
*
* An Implementation MAY put a hard limit on the size of a single
* Shared Memory block, defined by the constant TEEC_CONFIG_SHAREDMEM_MAX_SIZE.
* However note that this function may fail to register a
* block smaller than this limit due to a low resource condition
* encountered at run-time.
*
* \b Programmer \b Error \n
* The following usage of the API is a programmer error:\n
*       Calling with a \a context which is not initialized.\n
*       Calling with a \a sharedMem which has not be correctly populated
*       in accordance with the specification.\n
*       Attempting to initialize the same Shared Memory structure concurrently
*       from multiple threads.Multi-threaded Client Applications must use
*       platform-provided locking mechanisms to ensure that
*       this case does not occur.
*
* \b Implementor's \b Notes \n
* This design allows a non-NULL buffer with a size of 0 bytes to allow
* trivial integration with any implementations of the C library malloc,
* in which is valid to allocate a zero byte buffer and receive a non-
* NULL pointer which may not be de-referenced in return.
* Once successfully registered, the Shared Memory block can be used for
* efficient data transfers between the Client Application and the
* Trusted Application. The TEE Client API implementation and the underlying
* communications infrastructure SHOULD attempt to transfer data in to the
* TEE without using copies, if this is possible on the underlying
* implementation, but MUST fall back on data copies if zero-copy cannot be
* achieved. Client Application developers should be aware that,
* if the Implementation requires data copies,
* then Shared Memory registration may allocate a block of memory of the
* same size as the block being registered.
*
* @param context: A pointer to an initialized TEE Context
* @param sharedMem:  A pointer to a Shared Memory structure to register:\n
* the \a buffer, \a size, and \a flags fields of the sharedMem structure
* MUST be set in accordance with the specification described above
*
* @return TEEC_Result:
* TEEC_SUCCESS: The device was successfully opened.\n
* TEEC_ERROR_*: An implementation-defined error code for any other error.
*
*/
TEEC_Result TEEC_RegisterSharedMemory(
	struct TEEC_Context *context,
	struct TEEC_SharedMemory *sharedMem);


/**
* @brief Allocate a shared memory block.
*
* This function allocates a new block of memory as a block of Shared Memory
* within the scope of the specified TEE Context, in accordance with the
* parameters which have been set by the Client Application inside the
* \a sharedMem structure.
*
* The parameter \a context MUST point to an initialized TEE Context.
*
* The \a sharedMem parameter MUST point to the Shared Memory structure
* defining the region to allocate.
* Client Application MUST have populated the following fields of the
* Shared Memory structure:\n
* The \a size field MUST contain the desired size of the buffer, in bytes.
* The size is allowed to be zero. In this case memory is allocated and
* the pointer written in to the buffer field on return MUST not be NULL
* but MUST never be de-referenced by the Client Application. In this case
* however, the Shared Memory block can be used in
* Registered Memory References.\n
* The \a flags field indicates the allowed directions of data flow
* between the Client Application and the TEE.\n
* The Implementation MUST assume that all other fields in the Shared Memory
* structure have undefined content.
*
* An Implementation MAY put a hard limit on the size of a single
* Shared Memory block, defined by the constant
* \a TEEC_CONFIG_SHAREDMEM_MAX_SIZE.
* However note that this function may fail to allocate a
* block smaller than this limit due to a low resource condition
* encountered at run-time.
*
* If this function returns any code other than \a TEEC_SUCCESS
* the Implementation MUST have set the \a buffer field of \a sharedMem to NULL.
*
*
* \b Programmer \b Error \n
* The following usage of the API is a programmer error:\n
*       Calling with a \a context which is not initialized.\n
*       Calling with a \a sharedMem which has not be correctly populated
*       in accordance with the specification.\n
*       Attempting to initialize the same Shared Memory structure concurrently
*       from multiple threads.Multi-threaded Client Applications must use
*       platform-provided locking mechanisms to ensure that
*       this case does not occur.
*
* \b Implementor's \b Notes \n
* Once successfully allocated the Shared Memory block can be used for
* efficient data transfers between the Client Application and the
* Trusted Application. The TEE Client API and the underlying communications
* infrastructure should attempt to transfer data in to the TEE
* without using copies, if this is possible on the underlying implementation,
* but may have to fall back on data copies if zero-copy cannot be achieved.
* The memory buffer allocated by this function must have sufficient
* alignment to store any fundamental C data type at a natural alignment.
* For most platforms this will require the memory buffer to have 8-byte
* alignment, but refer to the Application Binary Interface (ABI) of the
* target platform for details.
*
* @param context: A pointer to an initialized TEE Context
* @param sharedMem:  A pointer to a Shared Memory structure to allocate:\n
* Before calling this function, the Client Application MUST have set
* the \a size, and \a flags fields.\n
* On return, for a successful allocation the Implementation
* MUST have set the pointer buffer to the address of the allocated block,
* otherwise it MUST set buffer to NULL.
*
* @return TEEC_Result:
* TEEC_SUCCESS: The allocation was successful.\n
* TEEC_ERROR_*: An implementation-defined error code for any other error.
*
*/
TEEC_Result TEEC_AllocateSharedMemory(
	struct TEEC_Context *context,
	struct TEEC_SharedMemory *sharedMem);


/**
* @brief Release a shared memory block.
*
* This function deregisters or deallocates a previously initialized block of
* Shared Memory.
* For a memory buffer allocated using \a TEEC_AllocateSharedMemory the
* Implementation MUST free the underlying memory and the Client Application
* MUST NOT access this region after this function has been called.
* In this case the Implementation MUST set the \a buffer and \a size fields
* of the \a sharedMem structure to NULL and 0 respectively before returning.
*
* For memory registered using \a TEEC_RegisterSharedMemory
* the Implementation MUST deregister the underlying memory from the TEE,
* but the memory region will stay available to the Client Application for
* other purposes as the memory is owned by it.
*
* The Implementation MUST do nothing if the \a sharedMem parameter is \a NULL.
*
* \b Programmer \b Error\n
* The following usage of the API is a programmer error:\n
*       Attempting to release Shared Memory which is used by a
*       pending operation.\n
*       Attempting to release the same Shared Memory structure concurrently
*       from multiple threads. Multi-threaded Client Applications
*       must use platform-provided locking mechanisms to ensure that
*       this case does not occur.
*
* @param sharedMem:  A pointer to a valid Shared Memory structure\n
*
*/
void TEEC_ReleaseSharedMemory(
	struct TEEC_SharedMemory *sharedMem);


/**
* @brief Opens a new session between client and trusted application
*
*
* This function opens a new Session between the Client Application and
* the specified Trusted Application.
*
* The Implementation MUST assume that all fields of this \a session structure
* are in an \a undefined state. When this function returns \a TEEC_SUCCESS
* the Implementation MUST have populated this structure with any information
* necessary for subsequent operations within the Session.
*
* The target Trusted Application is identified by a UUID passed in the
* parameter destination.
*
* The Session MAY be opened using a specific connection method that can carry
* additional connection data, such as data about the user or user-group running
* the Client Application, or about the Client Application itself.
* This allows the Trusted Application to implement access control methods
* which separate functionality or data accesses for different actors
* in the rich environment outside of the TEE. The additional data associated
* with each connection method is passed in via the pointer \a connectionData.
* For the core login types the following connection data is required:
*
* \a TEEC_LOGIN_PUBLIC - \a connectionData SHOULD be \a NULL.\n
* \a TEEC_LOGIN_USER - \a connectionData SHOULD be \a NULL.\n
* \a TEEC_LOGIN_GROUP - \a connectionData MUST point to a uint32_t
* which contains the group which this Client Application wants to connect as.
* The Implementation is responsible for securely ensuring that the
* Client Application instance is actually a member of this group.\n
* \a TEEC_LOGIN_APPLICATION - \a connectionData SHOULD be \a NULL.\n
* \a TEEC_LOGIN_USER_APPLICATION - \a connectionData SHOULD be \a NULL.\n
* \a TEEC_LOGIN_GROUP_APPLICATION - \a connectionData MUST point to a uint32_t
* which contains the group which this Client Application wants to connect as.
* The Implementation is responsible for securely ensuring that the
* Client Application instance is actually a member of this group.\n
*
* An open-session operation MAY optionally carry an Operation Payload,
* and MAY also be cancellable. When the payload is present the parameter
* \a operation MUST point to a \a TEEC_Operation structure populated by the
* Client Application. If \a operation is NULL then no data buffers are
* exchanged with the Trusted Application, and the operation cannot be
* cancelled by the Client Application.
*
* The result of this function is returned both in the function
* \a TEEC_Result return code and the return origin, stored in the variable
* pointed to by \a returnOrigin: \n
* If the return origin is different from \a TEEC_ORIGIN_TRUSTED_APP,
* then the return code MUST be  one of the defined error codes .
* If the return code is \a TEEC_ERROR_CANCEL then it means that the
* operation was cancelled before it reached the Trusted Application.\n
* If the return origin is \a TEEC_ORIGIN_TRUSTED_APP, the meaning of the
* return code depends on the protocol between the Client Application
* and the Trusted Application. However, if \a TEEC_SUCCESS is returned,
* it always means that the session was successfully opened and if the
* function returns a code different from \a TEEC_SUCCESS,
* it means that the session opening failed.
*
* \b Programmer \b Error \n
* The following usage of the API is a programmer error:\n
*       Calling with a \a context which is not yet initialized. \n
*       Calling with a connectionData set to NULL if connection data is
*  required by the specified connection method. \n
*       Calling with an operation containing an invalid paramTypes field,
* i.e., containing a reserved parameter type or where a parameter type
* that conflicts with the parent Shared Memory. \n
*       Encoding Registered Memory References which refer to
* Shared Memory blocks allocated within the scope of a different TEE Context. \n
*       Attempting to open a Session using the same Session structure
* concurrently from multiple threads. Multi-threaded Client Applications
* must use platform-provided locking mechanisms, to ensure that this
* case does not occur.\n
*      Using the same Operation structure for multiple concurrent operations. \n
*
* @param context: A pointer to an initialized TEE Context.
* @param session: A pointer to a Session structure to open.
* @param destination: A pointer to a structure containing the UUID of the
* destination Trusted Application
* @param connectionMethod:  The method of connection to use
* @param connectionData: Any necessary data required to support the
* connection method chosen.
* @param operation: A pointer to an Operation containing a set of Parameters
* to exchange with the Trusted Application, or \a NULL if no Parameters
* are to be exchanged or if the operation cannot be cancelled
* @param returnOrigin: A pointer to a variable which will contain the
* return origin. This field may be \a NULL if the return origin is not needed.
*
* @return TEEC_Result:
* TEEC_SUCCESS: The session was successfully opened. \n
* TEEC_ERROR_*: An implementation-defined error code for any other error.
*/
TEEC_Result TEEC_OpenSession(
	struct TEEC_Context *context,
	struct TEEC_Session *session,
	const struct TEEC_UUID *destination,
	uint32_t         connectionMethod,
	const void *connectionData,
	struct TEEC_Operation *operation,
	uint32_t *returnOrigin);


/**
* @brief Close a opened session between client and trusted application
*
*
* This function closes a Session which has been opened with a
* Trusted Application.
*
* All Commands within the Session MUST have completed before
* calling this function.
*
* The Implementation MUST do nothing if the session parameter is NULL.
*
* The implementation of this function MUST NOT be able to fail:
* after this function returns the Client Application must be able to
* consider that the Session has been closed.
*
* \b Programmer \b Error \n
* The following usage of the API is a programmer error:\n
*       Calling with a session which still has commands running.\n
*       Attempting to close the same Session concurrently from multiple
* threads. \n
*       Attempting to close the same Session more than once.
*
* @param session: Session to close
*/
void TEEC_CloseSession(
	struct TEEC_Session *session);


/**
* @brief Invokes a command within the session
*
*
* This function invokes a Command within the specified Session.
*
* The parameter \a session MUST point to a valid open Session.
*
* The parameter \a commandID is an identifier that is used to indicate
* which of the exposed Trusted Application functions should be invoked.
* The supported command identifiers are defined by the Trusted Application‟s
* protocol.
*
* \b Operation \b Handling \n
* A Command MAY optionally carry an Operation Payload.
* When the payload is present the parameter \a operation MUST point to a
* \a TEEC_Operation structure populated by the Client Application.
* If \a operation is NULL then no parameters are exchanged with the
* Trusted Application, and only the Command ID is exchanged.
*
* The \a operation structure is also used to manage cancellation of the
* Command. If cancellation is required then \a the operation pointer MUST be
* \a non-NULL and the Client Application MUST have zeroed the \a started
* field of the \a operation structure before calling this function.
* The \a operation structure MAY contain no Parameters if no data payload
* is to be exchanged.
*
* The Operation Payload is handled as described by the following steps,
* which are executed sequentially: \n
* 1. Each Parameter in the Operation Payload is examined.
* If the parameter is a Temporary Memory Reference, then it is registered
* for the duration of the Operation in accordance with the fields set in
* the \a TEEC_TempMemoryReference structure and the data flow direction
* specified in the parameter type. Refer to the \a TEEC_RegisterSharedMemory
* function for error conditions which can be triggered during
* temporary registration of a memory region. \n
* 2. The contents of all the Memory Regions which are exchanged
* with the TEE are synchronized \n
* 3. The fields of all Value Parameters tagged as input are read by the
* Implementation. This applies to Parameters of type \a TEEC_VALUE_INPUT or
* \a TEEC_VALUE_INOUT. \n
* 4. The Operation is issued to the Trusted Application.
* During the execution of the Command, the Trusted Application may read
* the data held within the memory referred to by input Memory References.
* It may also write data in to the memory referred to by
* output Memory References, but these modifications are not guaranteed
* to be observable by the Client Application until the command completes. \n
* 5. After the Command has completed, the Implementation MUST update the
* \a size field of the Memory Reference structures flagged as output: \n
*
* a. For Memory References that are non-null and marked as output,
* the updated size field MAY be less than or equal to original size field.
* In this case this indicates the number of bytes actually written by the
* Trusted Application, and the Implementation MUST synchronize this region
* with the Client Application memory space. \n
* b. For all Memory References marked as output, the updated size
* field MAY be larger than the original size field.
* For null Memory References, a required buffer size MAY be specified by
* the Trusted Application. In these cases the passed output buffer was
* too small or absent, and the returned size indicates the size of the
* output buffer which is necessary for the operation to succeed.
* In these cases the Implementation SHOULD NOT synchronize any
* shared data with the Client Application.\n\n
*
* 6. When the Command completes, the Implementation MUST update the fields
* of all Value Parameters tagged as output,
* i.e., of type \a TEEC_VALUE_OUTPUT or \a TEEC_VALUE_INOUT. \n
* 7. All memory regions that were temporarily registered at the
* beginning of the function are deregistered as if the function
* \a TEEC_ReleaseSharedMemory was called on each of them.
* 8. Control is passed back to the calling Client Application code. \n
* \b Programmer \b Error \n.
*
* The result of this function is returned both in the function
* \a TEEC_Result return code and the return origin, stored in the
* variable pointed to by \a returnOrigin: \n
*       If the return origin is different from \a TEEC_ORIGIN_TRUSTED_APP,
* then the return code MUST be one of the error codes.
* If the return code is TEEC_ERROR_CANCEL then it means that the operation
* was cancelled before it reached the Trusted Application.\n
* If the return origin is \a TEEC_ORIGIN_TRUSTED_APP, then the
* meaning of the return code is determined by the protocol exposed by the
* Trusted Application. It is recommended that the Trusted Application
* developer chooses TEEC_SUCCESS (0) to indicate success in their protocol,
* as this means that it is possible for the Client Application developer
* to determine success or failure without looking at the return origin.
*
* \b Programmer \n Error \n
* The following usage of the API is a programmer error:\n
*       Calling with a \a session which is not an open session. \n
*       Calling with invalid content in the \a paramTypes field of the
* \a operation structure. This invalid behavior includes types which are
* \a Reserved for future use or which conflict with the \a flags
* of the parent Shared Memory block. \n
*       Encoding Registered Memory References which refer to
* Shared Memory blocks allocated or registered within the scope of a
* different TEE Context. \n
*       Using the same operation structure concurrently for
* multiple operations, whether open Session operations or Command invocations.
*
* @param session: The open Session in which the command will be invoked.
* @param commandID: The identifier of the Command within the
* Trusted Application to invoke. The meaning of each Command Identifier
* must be defined in the protocol exposed by the Trusted Application
* @param operation: A pointer to a Client Application initialized
* \a TEEC_Operation structure, or NULL if there is no payload to send or
* if the Command does not need to support cancellation.
* @param returnOrigin: A pointer to a variable which will contain the
* return origin. This field may be \a NULL if the return origin is not needed.
*
* @return TEEC_Result:
* TEEC_SUCCESS: The command was successfully invoked. \n
* TEEC_ERROR_*: An implementation-defined error code for any other error.
*/
TEEC_Result TEEC_InvokeCommand(
	struct TEEC_Session *session,
	uint32_t     commandID,
	struct TEEC_Operation *peration,
	uint32_t *returnOrigin);


/**
* @brief Request cancellation of pending open session or command invocation.
*
* This function requests the cancellation of a pending open Session operation
* or a Command invocation operation. As this is a synchronous API,
* this function must be called from a thread other than the one executing the
* \a TEEC_OpenSession or \a TEEC_InvokeCommand function.
*
* This function just sends a cancellation signal to the TEE and returns
* immediately; the operation is not guaranteed to have been cancelled
* when this function returns. In addition, the cancellation request is just
* a hint; the TEE or the Trusted Application MAY ignore the
* cancellation request.
*
* It is valid to call this function using a \a TEEC_Operation structure
* any time after the Client Application has set the \a started field of an
* Operation structure to zero. In particular, an operation can be
* cancelled before it is actually invoked, during invocation, and
* after invocation. Note that the Client Application MUST reset
* the started field to zero each time an Operation structure is used
* or re-used to open a Session or invoke a Command if the new operation
* is to be cancellable.
*
* Client Applications MUST NOT reuse the Operation structure for another
* Operation until the cancelled command has actually returned in the thread
* executing the \a TEEC_OpenSession or \a TEEC_InvokeCommand function.
*
* \b Detecting \b cancellation \n
* In many use cases it will be necessary for the Client Application
* to detect whether the operation was actually cancelled, or whether it
* completed normally. \n
* In some implementations it MAY be possible for part of the infrastructure
* to cancel the operation before it reaches the Trusted Application.
* In these cases the return origin returned by \a TEEC_OpenSession or
* \a TEEC_InvokeCommand MUST be either or \a TEEC_ORIGIN_API,
* \a TEEC_ORIGIN_COMMS, \a TEEC_ORIGIN_TEE, and the return code MUST be
* \a TEEC_ERROR_CANCEL. \n
* If the cancellation request is handled by the Trusted Application itself
* then the return origin returned by \a TEEC_OpenSession or
* \a TEEC_InvokeCommand MUST be \a TEE_ORIGIN_TRUSTED_APP,
* and the return code is defined by the Trusted Application‟s protocol.
* If possible, Trusted Applications SHOULD use \a TEEC_ERROR_CANCEL
* for their return code, but it is accepted that this is not always
* possible due to conflicts with existing return code definitions in
* other standards.
*
* @param operation: A pointer to a Client Application instantiated
* Operation structure.
*/
void TEEC_RequestCancellation(
	struct TEEC_Operation *operation);

/**
* @brief Returns error string.
*
* This function returns the error string value based on error number and
* return origin.
*
* @param error:  Error number.
* @param returnOrigin:  Origin of the return.
*
* @return char*: Error string value.
*
*/
char *TEEC_GetError(int error, int returnOrigin);
#endif

