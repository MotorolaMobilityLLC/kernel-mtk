#ifndef __NEU_COMMON_H_
#define __NEU_COMMON_H_

#define NEU_MAX_REQ_PARAMS  12
//mod by luocl start
//#define NEU_MAX_RES_PARAMS  4
#define NEU_MAX_RES_PARAMS  8
//mod by luocl end
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

/**
 * @brief SMC command structure
 */
struct neu_smc_cmd {
    unsigned int    teei_cmd_type;
    unsigned int    id;
    unsigned int    context;
    unsigned int    enc_id;

    unsigned int    src_id;
    unsigned int    src_context;

    unsigned int    req_buf_len;
    unsigned int    resp_buf_len;
    unsigned int    ret_resp_buf_len;
    unsigned int    cmd_status;
    unsigned int    req_buf_phys;
    unsigned int    resp_buf_phys;
    unsigned int    meta_data_phys;
    unsigned int    dev_file_id;
};

#endif
