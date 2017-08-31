#ifndef __NEU_CLIENT_H_
#define __NEU_CLIENT_H_

#define CLIENT_FULL_PATH_DEV_NAME "/dev/teei_client"
#define DEFAULT_TEE "tta"
#define CANCEL_MSG "cancel"

#define CLIENT_IOC_MAGIC 0x775B777F /* "TEEI Client" */

#define NEU_DEBUG

#undef TDEBUG
#ifdef NEU_DEBUG
#define TDEBUG(fmt, args...) pr_err(KERN_DEBUG "%s(%i, %s): " fmt "\n", \
        __func__, current->pid, current->comm, ## args)
#else
#define TDEBUG(fmt, args...)
#endif

#undef TERR
#define TERR(fmt, args...) pr_err(KERN_ERR "%s(%i, %s): " fmt "\n", \
        __func__, current->pid, current->comm, ## args)

#include <tee_client_api.h>

/** IOCTL request */

#define NEU_MAX_REQ_PARAMS  12
//#define NEU_MAX_RES_PARAMS  4
#define NEU_MAX_RES_PARAMS  8
#define NEU_1K_SIZE 1024

/**
 * @brief Parameters type
 */
enum param_type {
    PARAM_IN = 0,
    PARAM_OUT
};

enum param_value {
    PARAM_A = 0,
    PARAM_B
};

/**
 * @brief Encode command structure
 */
typedef struct client_encode_cmd {
    unsigned int len;
    unsigned long data;
    int   offset;
    int   flags;
    int   param_type;

    int encode_id;
    int session_id;
    unsigned int cmd_id;
    int value_flag;
    int param_pos;
    int param_pos_type;
    int return_value;
    int return_origin;
} EncodeCmd;

/**
 * @brief Session details structure
 */
struct ser_ses_id{
    int session_id;
    TEEC_UUID uuid;
    int return_value;
    int return_origin;
    int paramtype;
    int params[8];
};
struct user_ses_init{
    int session_id;
};
// add by lodovico
struct ctx_data{
    char name[255]; // context name
    int return_value; // context return
};
// add end

/**
 * @brief Share memory information for the session
 */
struct session_shared_mem_info{
    int session_id;
    unsigned int user_mem_addr;
};

/**
 * @brief Shared memory used for smc processing
 */
struct neu_smc_cdata {
    int cmd_addr;
    int ret_val;
};

/* For general service */
#define CLIENT_IOCTL_CANCEL_COMMAND			_IOW(CLIENT_IOC_MAGIC,	0x1002,         struct ctx_data)
#define CLIENT_IOCTL_SEND_CMD_REQ			_IOWR(CLIENT_IOC_MAGIC,	3,		struct client_encode_cmd)
#define CLIENT_IOCTL_SES_OPEN_REQ			_IOW(CLIENT_IOC_MAGIC,	4,		struct ser_ses_id)
#define CLIENT_IOCTL_SES_CLOSE_REQ			_IOWR(CLIENT_IOC_MAGIC,	5,		struct ser_ses_id)
#define CLIENT_IOCTL_SHR_MEM_FREE_REQ   		_IOWR(CLIENT_IOC_MAGIC,	6,		struct session_shared_mem_info )

#define CLIENT_IOCTL_ENC_UINT32				_IOWR(CLIENT_IOC_MAGIC,	7,		struct client_encode_cmd)
#define CLIENT_IOCTL_ENC_ARRAY				_IOWR(CLIENT_IOC_MAGIC,	8,		struct client_encode_cmd)
#define CLIENT_IOCTL_ENC_ARRAY_SPACE    		_IOWR(CLIENT_IOC_MAGIC,	9,		struct client_encode_cmd)
#define CLIENT_IOCTL_ENC_MEM_REF			_IOWR(CLIENT_IOC_MAGIC,	10,		struct client_encode_cmd)

#define CLIENT_IOCTL_DEC_UINT32				_IOWR(CLIENT_IOC_MAGIC,	11,		struct client_encode_cmd)
#define CLIENT_IOCTL_DEC_ARRAY_SPACE    		_IOWR(CLIENT_IOC_MAGIC,	12,		struct client_encode_cmd)

#define CLIENT_IOCTL_OPERATION_RELEASE  		_IOWR(CLIENT_IOC_MAGIC,	13,		struct client_encode_cmd)
#define CLIENT_IOCTL_GET_DECODE_TYPE    		_IOWR(CLIENT_IOC_MAGIC,	15,		struct client_encode_cmd)
#define CLIENT_IOCTL_INITCONTEXT_REQ    		_IOW(CLIENT_IOC_MAGIC,	16,		struct ctx_data)
#define CLIENT_IOCTL_CLOSECONTEXT_REQ   		_IOW(CLIENT_IOC_MAGIC,	17,		struct ctx_data)
#define CLIENT_IOCTL_SES_INIT_REQ			_IOW(CLIENT_IOC_MAGIC,	18,		struct user_ses_init)
#endif /* __NEU_CLIENT_H_ */
