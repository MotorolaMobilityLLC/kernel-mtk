#ifndef __NEU_COMMON_H_
#define __NEU_COMMON_H_

#define NEU_MAX_REQ_PARAMS  12
/* #define NEU_MAX_RES_PARAMS  4 */
#define NEU_MAX_RES_PARAMS  8
#define NEU_1K_SIZE 1024

/**
 * @brief Parameters type
 */
enum neu_param_type {
	NEU_PARAM_IN = 0,
	NEU_PARAM_OUT
};


/**
 * @brief Metadata used for encoding/decoding
 */
struct neu_encode_meta {
	int type;
	int len;
	unsigned int usr_addr;
	int ret_len;
};

#endif
