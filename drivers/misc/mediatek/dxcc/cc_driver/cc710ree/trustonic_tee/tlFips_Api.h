/*
 * Copyright (c) 2013 TRUSTONIC LIMITED
 * All rights reserved
 *
 * The present software is the confidential and proprietary information of
 * TRUSTONIC LIMITED. You shall not disclose the present software and shall
 * use it only in accordance with the terms of the license agreement you
 * entered into with TRUSTONIC LIMITED. This software may be subject to
 * export or import laws in certain countries.
 */

#ifndef TL_FIPS_API_H_
#define TL_FIPS_API_H_

#include "tci.h"

/**
 * Command ID's for communication Trusted Application Connector (TAC)
 * -> Trusted Application (TA).
 */

#define CMD_SASI_LIB_INIT                 (0)
#define CMD_SASI_LIB_FINI                 (1)
#define CMD_SASI_AES_INIT                 (2)
#define CMD_SASI_AES_SETKEY               (3)
#define CMD_SASI_AES_SETIV                (4)
#define CMD_SASI_AES_BLOCK                (5)
#define CMD_SASI_AES_FINISH               (6)
#define CMD_CRYS_AESCCM                   (7)
#define CMD_CRYS_DES                      (8)
#define CMD_CRYS_HASH                     (9)
#define CMD_CRYS_HMAC                     (10)
#define CMD_CRYS_KDF                      (11)
#define CMD_CRYS_RSA_KEY_GEN              (12)
#define CMD_CRYS_RSA_BUILD_PUBK           (13)
#define CMD_SASI_RSA_VERIFY               (14)
#define CMD_SASI_RSA_SIGN                 (15)
#define CMD_SASI_RSA_GET_PUBK             (16)
#define CMD_CRYS_ECPKI_KEY_GEN            (17)
#define CMD_CRYS_ECPKI_EXPORT_PUBK        (18)
#define CMD_CRYS_CONVERT_ENDIAN           (19)
#define CMD_CRYS_ECPKI_BUILD_PUBK         (20)
#define CMD_CRYS_ECDSA_VERIFY             (21)
#define CMD_CRYS_ECDSA_GET_DOMAIN         (22)
#define CMD_CRYS_ECDSA_SIGN               (23)
#define CMD_CRYS_RND_ADD_ADDITIONAL_INPUT (24)
#define CMD_CRYS_RND_INSTANTIATION        (25)
#define CMD_CRYS_RND_RESEEDING            (26)
#define CMD_CRYS_RND_GENERATE_VECTOR      (27)
#define CMD_CRYS_RND_ENTER_KAT_MODE       (28)
#define CMD_CRYS_RND_UNINSTANTIATION      (29)
#define CMD_SASI_UTIL_KDF                 (30)
#define CMD_ECDH_SVDP_DH                  (31)
#define CMD_CRYS_FIPS_GET_TEE_ERROR       (32)
#define CMD_CRYS_FIPS_SET_REE_STATUS      (33)

/* sample cmd */
#define CMD_SAMPLE_ROT13_CIPHER           (34)
#define CMD_SAMPLE_ROT13_UNWRAP           (35)

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    rnd_work_buf_off;
	uint32_t                    rnd_work_buf_sz;
	uint32_t                    fips_ctx_off;
	uint32_t                    fips_ctx_sz;
	bool                        is_fips_support;
} TL_PARAM_SASI_LIB_INIT;

typedef struct {
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
} TL_PARAM_SASI_LIB_FINISH;

typedef struct {
	uint32_t                    ret;
	uint32_t                    ctx_off;
	uint32_t                    ctx_sz;
	uint32_t                    enc_mode;
	uint32_t                    op_mode;
	uint32_t                    pad_type;
} TL_PARAM_SASI_AES_INIT;

typedef struct {
	uint32_t                    ret;
	uint32_t                    ctx_off;
	uint32_t                    ctx_sz;
	uint32_t                    key_type;
	uint32_t                    key_data_off;
	uint32_t                    key_data_sz;
	uint32_t                    key_off;
	uint32_t                    key_sz;
} TL_PARAM_SASI_AES_SETKEY;

typedef struct {
	uint32_t                    ret;
	uint32_t                    ctx_off;
	uint32_t                    ctx_sz;
	uint32_t                    iv_off;
	uint32_t                    iv_sz;
} TL_PARAM_SASI_AES_SETIV;

typedef struct {
	uint32_t                    ret;
	uint32_t                    ctx_off;
	uint32_t                    ctx_sz;
	uint32_t                    data_in_off;
	uint32_t                    data_in_sz;
	uint32_t                    data_out_off;
} TL_PARAM_SASI_AES_BLOCK;

typedef struct {
	uint32_t                    ret;
	uint32_t                    ctx_off;
	uint32_t                    ctx_sz;
	uint32_t                    data_sz;
	uint32_t                    data_in_off;
	uint32_t                    data_in_buf_sz;
	uint32_t                    data_out_off;
	uint32_t                    data_out_buf_sz;
} TL_PARAM_SASI_AES_FINISH;

typedef struct {
	uint32_t                    ret;
	uint32_t                    enc_mode;
	uint32_t                    key_data_off;
	uint32_t                    key_data_sz;
	uint32_t                    key_sz_id;
	uint32_t                    nonce_off;
	uint32_t                    nonce_sz;
	uint32_t                    add_data_off;
	uint32_t                    add_data_sz;
	uint32_t                    text_data_in_off;
	uint32_t                    text_data_in_sz;
	uint32_t                    text_data_out_off;
	uint32_t                    tag_off;
	uint32_t                    tag_sz;
} TL_PARAM_CRYS_AESCCM;

typedef struct {
	uint32_t                    ret;
	uint32_t                    iv_off;
	uint32_t                    iv_sz;
	uint32_t                    key_data_off;
	uint32_t                    key_data_sz;
	uint32_t                    key_data_num;
	uint32_t                    enc_mode;
	uint32_t                    op_mode;
	uint32_t                    data_in_off;
	uint32_t                    data_in_sz;
	uint32_t                    data_out_off;
} TL_PARAM_CRYS_DES;

typedef struct {
	uint32_t                    ret;
	uint32_t                    op_mode;
	uint32_t                    data_in_off;
	uint32_t                    data_in_sz;
	uint32_t                    data_out_off;
	uint32_t                    data_out_sz;
} TL_PARAM_CRYS_HASH;

typedef struct {
	uint32_t                    ret;
	uint32_t                    op_mode;
	uint32_t                    key_data_off;
	uint32_t                    key_data_sz;
	uint32_t                    data_in_off;
	uint32_t                    data_in_sz;
	uint32_t                    data_out_off;
	uint32_t                    data_out_sz;
} TL_PARAM_CRYS_HMAC;

typedef struct {
	uint32_t                    ret;
	uint32_t                    zz_secret_off;
	uint32_t                    zz_secret_sz;
	uint32_t                    other_info_off;
	uint32_t                    other_info_sz;
	uint32_t                    kdf_hash_mode;
	uint32_t                    deriv_mode;
	uint32_t                    key_data_off;
	uint32_t                    key_data_sz;
} TL_PARAM_CRYS_KDF;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    pub_exp_off;
	uint32_t                    pub_exp_sz;
	uint32_t                    key_data_sz;
	uint32_t                    cc_user_prvk_off;
	uint32_t                    cc_user_prvk_sz;
	uint32_t                    cc_user_pubk_off;
	uint32_t                    cc_user_pubk_sz;
	uint32_t                    key_gen_data_off;
	uint32_t                    key_gen_data_sz;
	uint32_t                    fips_ctx_off;
	uint32_t                    fips_ctx_sz;
} TL_PARAM_CRYS_RSA_KEY_GEN;

typedef struct {
	uint32_t                    ret;
	uint32_t                    user_pubk_off;
	uint32_t                    user_pubk_sz;
	uint32_t                    exp_off;
	uint32_t                    exp_sz;
	uint32_t                    mod_off;
	uint32_t                    mod_sz;
} TL_PARAM_CRYS_RSA_BUILD_PUBK;

typedef struct {
	uint32_t                    ret;
	uint32_t                    user_ctx_off;
	uint32_t                    user_ctx_sz;
	uint32_t                    user_pubk_off;
	uint32_t                    user_pubk_sz;
	uint32_t                    rsa_hash_mode;
	uint32_t                    mgf;
	uint32_t                    salt_len;
	uint32_t                    data_in_off;
	uint32_t                    data_in_sz;
	uint32_t                    sig_off;
	uint32_t                    sig_sz;
	uint32_t                    pkcs1_ver;
} TL_PARAM_SASI_RSA_VERIFY;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    user_ctx_off;
	uint32_t                    user_ctx_sz;
	uint32_t                    user_prvk_off;
	uint32_t                    user_prvk_sz;
	uint32_t                    rsa_hash_mode;
	uint32_t                    mgf;
	uint32_t                    salt_len;
	uint32_t                    data_in_off;
	uint32_t                    data_in_sz;
	uint32_t                    output_off;
	uint32_t                    output_sz;
	uint32_t                    pkcs1_ver;
} TL_PARAM_SASI_RSA_SIGN;

typedef struct {
	uint32_t                    ret;
	uint32_t                    user_pubk_off;
	uint32_t                    user_pubk_sz;
	uint32_t                    exp_off;
	uint32_t                    exp_sz;
	uint32_t                    mod_off;
	uint32_t                    mod_sz;
} TL_PARAM_SASI_RSA_GET_PUBK;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    domain_off;
	uint32_t                    domain_sz;
	uint32_t                    user_prvk_off;
	uint32_t                    user_prvk_sz;
	uint32_t                    user_pubk_off;
	uint32_t                    user_pubk_sz;
	uint32_t                    tmp_buf_off;
	uint32_t                    tmp_buf_sz;
	uint32_t                    fips_ctx_off;
	uint32_t                    fips_ctx_sz;
} TL_PARAM_CRYS_ECPKI_KEY_GEN;

typedef struct {
	uint32_t                    ret;
	uint32_t                    user_pubk_off;
	uint32_t                    user_pubk_sz;
	uint32_t                    compression;
	uint32_t                    export_pubk_off;
	uint32_t                    export_pubk_sz;
} TL_PARAM_CRYS_ECPKI_EXPORT_PUBK;

typedef struct {
	uint32_t                    ret;
	uint32_t                    out_off;
	uint32_t                    out_sz;
	uint32_t                    in_off;
	uint32_t                    in_sz;
} TL_PARAM_CRYS_CONVERT_ENDIAN;

typedef struct {
	uint32_t                    ret;
	uint32_t                    domain_off;
	uint32_t                    domain_sz;
	uint32_t                    pubk_in_off;
	uint32_t                    pubk_in_sz;
	uint32_t                    check_mode;
	uint32_t                    user_pubk_off;
	uint32_t                    user_pubk_sz;
	uint32_t                    tmp_buf_off;
	uint32_t                    tmp_buf_sz;
} TL_PARAM_CRYS_ECPKI_BUILD_PUBK;

typedef struct {
	uint32_t                    ret;
	uint32_t                    verify_user_ctx_off;
	uint32_t                    verify_user_ctx_sz;
	uint32_t                    user_pubk_off;
	uint32_t                    user_pubk_sz;
	uint32_t                    hash_mode;
	uint32_t                    sig_in_off;
	uint32_t                    sig_in_sz;
	uint32_t                    msg_data_in_off;
	uint32_t                    msg_data_in_sz;
} TL_PARAM_CRYS_ECDSA_VERIFY;

typedef struct {
	uint32_t                    domain_id;
	uint32_t                    domain_off;
	uint32_t                    domain_sz;
} TL_PARAM_CRYS_ECDSA_GET_DOMAIN;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    sign_user_ctx_off;
	uint32_t                    sign_user_ctx_sz;
	uint32_t                    prvk_off;
	uint32_t                    prvk_sz;
	uint32_t                    hash_mode;
	uint32_t                    msg_data_in_off;
	uint32_t                    msg_data_in_sz;
	uint32_t                    sig_out_off;
	uint32_t                    sig_out_sz;
} TL_PARAM_CRYS_ECDSA_SIGN;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    additional_input_off;
	uint32_t                    additional_input_sz;
} TL_PARAM_CRYS_RND_ADD_ADDITIONAL_INPUT;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    work_buf_off;
	uint32_t                    work_buf_sz;
} TL_PARAM_CRYS_RND_INSTANTIATION;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    work_buf_off;
	uint32_t                    work_buf_sz;
} TL_PARAM_CRYS_RND_RESEEDING;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_state_off;
	uint32_t                    rnd_state_sz;
	uint32_t                    out_off;
	uint32_t                    out_sz;
} TL_PARAM_CRYS_RND_GENERATE_VECTOR;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
	uint32_t                    entr_data_off;
	uint32_t                    entr_data_sz;
	uint32_t                    nonce_off;
	uint32_t                    nonce_sz;
	uint32_t                    work_buf_off;
	uint32_t                    work_buf_sz;
} TL_PARAM_CRYS_RND_ENTER_KAT_MODE;

typedef struct {
	uint32_t                    ret;
	uint32_t                    rnd_ctx_off;
	uint32_t                    rnd_ctx_sz;
} TL_PARAM_CRYS_RND_UNINSTANTIATION;

typedef struct {
	uint32_t                    ret;
	uint32_t                    key_type;
	uint32_t                    user_key_off;
	uint32_t                    user_key_sz;
	uint32_t                    label_off;
	uint32_t                    label_sz;
	uint32_t                    ctx_off;
	uint32_t                    ctx_sz;
	uint32_t                    deriv_key_off;
	uint32_t                    deriv_key_sz;
} TL_PARAM_SASI_UTIL_KDF;

typedef struct {
	uint32_t                    ret;
	uint32_t                    pubk_off;
	uint32_t                    pubk_sz;
	uint32_t                    prvk_off;
	uint32_t                    prvk_sz;
	uint32_t                    shared_secret_val_off;
	uint32_t                    shared_secret_val_sz;
	uint32_t                    tmp_buf_off;
	uint32_t                    tmp_buf_sz;
} TL_PARAM_ECDH_SVDP_DH;

typedef struct {
	uint32_t                    ret;
	uint32_t                    tee_error;
} TL_PARAM_CRYS_FIPS_GET_TEE_ERROR;

typedef struct {
	uint32_t                    ret;
	uint32_t                    ree_status;
} TL_PARAM_CRYS_FIPS_SET_REE_STATUS;

/**
 * Return codes
 */
#define RET_ERR_ROT13_INVALID_LENGTH   (RET_CUSTOM_START + 1)
#define RET_ERR_ROT13_UNWRAP           (RET_CUSTOM_START + 3)

/**
 * Termination codes
 */
#define EXIT_ERROR ((uint32_t)(-1))

#define MAX_PARAM_LEN (0x1000)
#define MAX_DATA_LEN  (0xC0000)

/**
 * ROT13 CIPHER and ROT13 UNWRAP command messages.
 *
 * The ROT13 CIPHER command will apply disguise data provided in the payload
 * buffer of the TCI. Only characters (a-z, A-Z) will be processed. Other
 * data is ignored and stays untouched.

 * The ROT13 UNWRAP command will unwrap in the TA the Secure Object Data
 * container provided in the buffer of the TCI.
 *
 * @param len Length of the data to process.
 * @param data Data to process (text to cipher or data container to unwrap).
 */
typedef struct {
	tciCommandHeader_t header; /**< Command header */
	uint32_t           param_len;
	uint32_t           data_len;
} tciCommand_t;

/**
 * Response structure TA -> TAC.
 */
typedef struct {
	tciResponseHeader_t header; /**< Response header */
	uint32_t            len; /**< Length of data processed */
	long long           time_cipher;
} tciResponse_t;

/**
 * TCI message data.
 */
typedef struct {
	union {
		tciCommandHeader_t  cmd_hdr;    /* Command header  */
		tciResponseHeader_t rsp_hdr;    /* Response header */
		tciCommand_t        cmd;
		tciResponse_t       rsp;
	};
	/**< Clear/Cipher text  or Wrap/Unwrap Data container */
	union {
		TL_PARAM_SASI_LIB_INIT                 param_sasi_lib_init;
		TL_PARAM_SASI_LIB_FINISH               param_sasi_lib_finish;
		TL_PARAM_SASI_AES_INIT                 param_sasi_aes_init;
		TL_PARAM_SASI_AES_SETKEY               param_sasi_aes_setkey;
		TL_PARAM_SASI_AES_SETIV                param_sasi_aes_setiv;
		TL_PARAM_SASI_AES_BLOCK                param_sasi_aes_block;
		TL_PARAM_SASI_AES_FINISH               param_sasi_aes_finish;
		TL_PARAM_CRYS_AESCCM                   param_aesccm;
		TL_PARAM_CRYS_DES                      param_des;
		TL_PARAM_CRYS_HASH                     param_hash;
		TL_PARAM_CRYS_HMAC                     param_hmac;
		TL_PARAM_CRYS_KDF                      param_kdf;
		TL_PARAM_CRYS_RSA_KEY_GEN              param_rsa_key_gen;
		TL_PARAM_CRYS_RSA_BUILD_PUBK           param_rsa_build_pubk;
		TL_PARAM_SASI_RSA_VERIFY               param_rsa_verify;
		TL_PARAM_SASI_RSA_SIGN                 param_rsa_sign;
		TL_PARAM_SASI_RSA_GET_PUBK             param_rsa_get_pubk;
		TL_PARAM_CRYS_ECPKI_KEY_GEN            param_ecpki_key_gen;
		TL_PARAM_CRYS_ECPKI_EXPORT_PUBK        param_ecpki_export_pubk;
		TL_PARAM_CRYS_CONVERT_ENDIAN           param_convert_endian;
		TL_PARAM_CRYS_ECPKI_BUILD_PUBK         param_ecpki_build_pubk;
		TL_PARAM_CRYS_ECDSA_VERIFY             param_ecdsa_verify;
		TL_PARAM_CRYS_ECDSA_GET_DOMAIN         param_ecdsa_get_domain;
		TL_PARAM_CRYS_ECDSA_SIGN               param_ecdsa_sign;
		TL_PARAM_CRYS_RND_ADD_ADDITIONAL_INPUT param_rnd_add_additional_input;
		TL_PARAM_CRYS_RND_INSTANTIATION        param_rnd_instantiation;
		TL_PARAM_CRYS_RND_RESEEDING            param_rnd_reseeding;
		TL_PARAM_CRYS_RND_GENERATE_VECTOR      param_rnd_generate_vector;
		TL_PARAM_CRYS_RND_ENTER_KAT_MODE       param_rnd_enter_kat_mode;
		TL_PARAM_CRYS_RND_UNINSTANTIATION      param_rnd_uninstantiation;
		TL_PARAM_SASI_UTIL_KDF                 param_sasi_util_kdf;
		TL_PARAM_ECDH_SVDP_DH                  param_ecdh_svdp_dh;
		TL_PARAM_CRYS_FIPS_GET_TEE_ERROR       param_fips_get_tee_error;
		TL_PARAM_CRYS_FIPS_SET_REE_STATUS      param_fips_set_ree_status;
		uint8_t                                param[MAX_PARAM_LEN];
	};
	uint8_t data[MAX_DATA_LEN];
} tciMessage_t;

/**
 * TA UUID.
 */
#define TL_FIPS_UUID { {0x5, 0x21, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} }

#endif /* TL_FIPS_API_H_*/

