/*
 
 */

#ifndef __TLTEEKEYMASTERAPI_H__
#define __TLTEEKEYMASTERAPI_H__

#include "tci.h"



/**
 * Command ID's
 */
#define CMD_ID_TEE_RSA_GEN_KEY_PAIR    1
#define CMD_ID_TEE_RSA_SIGN            2
#define CMD_ID_TEE_RSA_VERIFY          3
#define CMD_ID_TEE_KEY_IMPORT          4
#define CMD_ID_TEE_GET_PUB_KEY         5
#define CMD_ID_TEE_DSA_GEN_KEY_PAIR    6
#define CMD_ID_TEE_DSA_SIGN            7
#define CMD_ID_TEE_DSA_VERIFY          8
#define CMD_ID_TEE_ECDSA_GEN_KEY_PAIR  9
#define CMD_ID_TEE_ECDSA_SIGN          10
#define CMD_ID_TEE_ECDSA_VERIFY        11
#define CMD_ID_TEE_GET_KEY_INFO        12

#define CMD_ID_TEE_GENERATE_KEY        13
#define CMD_ID_TEE_GET_SUPPORTED_ALGO  14
#define CMD_ID_TEE_GET_SUPPORTED_BLOCK 15
#define CMD_ID_TEE_GET_SUPPORTED_PADDING 16
#define CMD_ID_TEE_GET_SUPPORTED_DIGESTS 17
#define CMD_ID_TEE_GET_SUPPORTED_IMPORT_FORMATS 18
#define CMD_ID_TEE_GET_SUPPORTED_EXPORT_FORMATS 19
#define CMD_ID_TEE_GET_KEY_CHARACTERISTICS 20
#define CMD_ID_TEE_IMPORT_KEY 21
#define CMD_ID_TEE_EXPORT_KEY 22
#define CMD_ID_TEE_DELETE_KEY 23
#define CMD_ID_TEE_DELETE_ALL_KEYS 24
#define CMD_ID_TEE_BEGIN 25
#define CMD_ID_TEE_UPDATE 26
#define CMD_ID_TEE_FINISH 27
#define CMD_ID_TEE_ABORT  28


/*... add more command ids when needed */


/**
 * Curve types
 */
#define ECC_CURVE_NIST_P192 1
#define ECC_CURVE_NIST_P224 2
#define ECC_CURVE_NIST_P256 3
#define ECC_CURVE_NIST_P384 4
#define ECC_CURVE_NIST_P521 5
/*... add more curves when needed */

/**
 * Command message.
 *
 * @param len Length of the data to process.
 * @param data Data to be processed
 */
typedef struct {
    uint32_t commandId;
} command_t;


/**
 * Response structure
 */
typedef struct {
    int32_t            error;
} response_t;


/**
 * RSA private key metadata (Private modulus and exponent lengths)
 */
typedef struct {
    uint32_t  priv_exp_len; /**< Private key exponent length */
} rsa_priv_key_meta_t;


/**
 * RSA CRT private key metadata
 */
typedef struct {
    uint32_t  p_len;          /**< Prime p length */
    uint32_t  q_len;          /**< Prime q length */
    uint32_t  dp_len;         /**< DP length */
    uint32_t  dq_len;         /**< DQ length */
    uint32_t  qinv_len;       /**< QP length */
} rsa_crt_priv_key_meta_t;


/**
 * RSA key metadata (key size, modulus/exponent lengths, etc..)
 */
typedef struct {
    uint32_t  type;           /**< RSA key pair type. RSA or RSA CRT */
    uint32_t  key_size;      /**<  RSA key size */
    uint32_t  pub_mod_len;    /**< Public key modulus length */
    uint32_t  pub_exp_len;    /**< Public key exponent length */
	
    rsa_priv_key_meta_t     rsa_priv;     /**< RSA private key */
    rsa_crt_priv_key_meta_t rsa_crt_priv; /**< RSA CRT private key */

} rsa_key_meta_t;


/**
 * DSA key metadata (p, q, g, x and y lengths)
 */
typedef struct {
    uint32_t  p_len;      /**< q length */
    uint32_t  q_len;      /**< q length */
    uint32_t  g_len;      /**< Generator length */
    uint32_t  x_len;      /**< x length */
    uint32_t  y_len;      /**< y length */
} dsa_key_meta_t;


/**
 * ECDSA key metadata
 */
typedef struct {
    uint32_t  curve;        /**< Curve type */
    uint32_t  curve_length; /**< curve length */
} ecdsa_key_meta_t;


/**
 * Key metadata
 */
typedef struct {
    uint32_t  key_type;      /**< Key type (RSA, DSA, ECDSA,..) */
	
    rsa_key_meta_t   rsa_key;   /**< RSA key */
    dsa_key_meta_t   dsa_key;   /**< DSA key */
    ecdsa_key_meta_t ecdsa_key; /**< ECDSA key */

} key_meta_t;


/**
 * RSA public key metadata (modulus and exponent lengths)
 */
typedef struct {
    uint32_t  pub_mod_len;     /**< Public key modulus length */
    uint32_t  pub_exp_len;     /**< Public key exponent length */
} rsa_pub_key_meta_t;


/**
 * DSA public key metadata (p, q, g, and y lengths)
 */
typedef struct {
    uint32_t  p_len;        /**< q length */
    uint32_t  q_len;        /**< q length */
    uint32_t  g_len;        /**< Generator length */
    uint32_t  y_len;        /**< y length */
} dsa_pub_key_meta_t;


/**
 * ECDSA public key metadata
 */
typedef struct {
    uint32_t  curve;        /**< Curve e.g. P192, P224,..,P521*/
} ecdsa_pub_key_meta_t;


/**
 * Generate RSA key data
 * Response data contains generated RSA key pair data is
 * wrapped as below:
 *
 * |-- Key metadata --|-- Public key (plaintext) --|-- Private key (encrypted) --|
 */
 typedef struct {
	uint32_t  type; 		  /**< Key pair type. RSA or RSACRT */
	uint32_t  key_size; 	  /**< Key size in bits, e.g. 1024, 2048,.. */
	uint32_t  exponent; 	  /**< Exponent number */
	uint32_t  key_data; 	  /**< Key data buffer passed by TLC  */
	uint32_t  key_data_len;   /**< Length of key data buffer */
	uint32_t  so_len;		 /**< Secure object length	(of key data) (provided by the trustlet)  */
} rsa_gen_key_t;



/**
 *  RSA sign data structure
 */
typedef struct {
    uint32_t  key_data;           /**< Key data buffer */
    uint32_t  key_data_len;       /**< Length of key data buffer */
    uint32_t  plain_data;         /**< Plaintext data buffer */
    uint32_t  plain_data_len;     /**< Length of plaintext data buffer */
    uint32_t  signature_data;     /**< Signature data buffer */
    uint32_t  signature_data_len; /**< Length of signature data buffer */
    uint32_t  algorithm;          /**< Signing algorithm */
} rsa_sign_t;


/**
 *  RSA signature verify data structure
 */
typedef struct {
    uint32_t  key_data;           /**< Key data buffer */
    uint32_t  key_data_len;       /**< Length of key data buffer */
    uint32_t  plain_data;         /**< Plaintext data buffer */
    uint32_t  plain_data_len;     /**< Length of plaintext data buffer */
    uint32_t  signature_data;     /**< Signature data buffer */
    uint32_t  signature_data_len; /**< Length of signature data buffer */
    uint32_t  algorithm;          /**< Signing algorithm */
    uint32_t  validity;           /**< Signature validity */
} rsa_verify_t;


/**
 *  Key import data structure
 */
typedef struct {
    uint32_t  key_data;        /**< Key data buffer */
    uint32_t  key_data_len;    /**< Length of key data buffer */
    uint32_t  so_data;         /**< Wrapped buffer */
    uint32_t  so_data_len;     /**< Length of wrapped data buffer */
} key_import_t;


/**
 *  Get public key data structure
 */
typedef struct {
    uint32_t  type;              /**< Key type */
    uint32_t  key_data;          /**< Key data buffer */
    uint32_t  key_data_len;      /**< Length of key data buffer */
    uint32_t  pub_key_data;      /**< Public key data */
    uint32_t  pub_key_data_len;  /**< Public key length */
} get_pub_key_t;


/**
 * Generate DSA key data
 * Response data contains generated DSA key pair data is
 * wrapped as below:
 *
 * |-- Key metadata --|-- Public key (plaintext) --|-- Private key (encrypted) --|
 *
 * Public key data includes p, q, g and y
 */
typedef struct {
    uint32_t  p;            /**< p */
    uint32_t  q;            /**< q */
    uint32_t  g;            /**< g */
    uint32_t  p_len;        /**< p length */
    uint32_t  q_len;        /**< q length */
    uint32_t  g_len;        /**< g length */
    uint32_t  x_len;        /**< x length */
    uint32_t  y_len;        /**< y length */
    uint32_t  key_data;     /**< Key data buffer passed by TLC  */
    uint32_t  key_data_len; /**< Length of key data buffer */
    uint32_t  so_len;       /**< Secure object length (of key data) */
} dsa_gen_key_t;


/**
 *  DSA sign data structure
 */
typedef struct {
    uint32_t  key_data;            /**< Key data buffer */
    uint32_t  key_data_len;        /**< Length of key data buffer */
    uint32_t  digest_data;         /**< Digest data buffer */
    uint32_t  digest_data_len;     /**< Length of digest data buffer */
    uint32_t  signature_data;      /**< Signature data buffer */
    uint32_t  signature_data_len;  /**< Length of signature data buffer */
} dsa_sign_t;


/**
 *  DSA signature verify data structure
 */
typedef struct {
    uint32_t  key_data;           /**< Key data buffer */
    uint32_t  key_data_len;       /**< Length of key data buffer */
    uint32_t  digest_data;        /**< Plaintext data buffer */
    uint32_t  digest_data_len;    /**< Length of plaintext data buffer */
    uint32_t  signature_data;     /**< Signature data buffer */
    uint32_t  signature_data_len; /**< Length of signature data buffer */
    uint32_t  validity;           /**< Signature validity */
} dsa_verify_t;


/**
 * Generate ECDSA key data
 * Response data contains generated ECDSA key pair data is
 * wrapped as below:
 *
 * |-- Key metadata --|-- Public key (plaintext) --|-- Private key (encrypted) --|
 *
 * Public key data includes x and y
 */
typedef struct {
    uint32_t  curve;        /**< Curve type P192=1, P224=2, P256=3, P384=4, P521=5  */
    uint32_t  key_data;     /**< Key data buffer passed by TLC  */
    uint32_t  key_data_len; /**< Length of key data buffer */
    uint32_t  so_len;       /**< Secure object length (of key data) */
} ecdsa_gen_key_t;


/**
 *  ECDSA sign data structure
 */
typedef struct {
    uint32_t  key_data;           /**< Key data buffer */
    uint32_t  key_data_len;       /**< Length of key data buffer */
    uint32_t  digest_data;        /**< Digest data buffer */
    uint32_t  digest_data_len;    /**< Length of digest data buffer */
    uint32_t  signature_data;     /**< Signature data buffer */
    uint32_t  signature_data_len; /**< Length of signature data buffer */
} ecdsa_sign_t;


/**
 *  ECDSA signature verify data structure
 */
typedef struct {
    uint32_t  key_data;           /**< Key data buffer */
    uint32_t  key_data_len;       /**< Length of key data buffer */
    uint32_t  digest_data;        /**< Digest data buffer */
    uint32_t  digest_data_len;    /**< Length of digest data buffer */
    uint32_t  signature_data;     /**< Signature data buffer */
    uint32_t  signature_data_len; /**< Length of signature data buffer */
    uint32_t  validity;           /**< Signature validity */
} ecdsa_verify_t;


/**
 *  Key info data structure
 */
typedef struct {
    uint32_t  key_blob;           /**< Key blob buffer */
    uint32_t  key_blob_len;       /**< Length of key blob buffer */
    uint32_t  key_metadata;       /**< Key metadata */
} get_key_info_t;

typedef struct{
	uint32_t  in_params_length;
	uint32_t  hw_params_length;
	uint32_t  sw_params_length;
	uint32_t  key_material_size;
}generate_key_t;

typedef struct{
	uint32_t algorithms_length;
}get_supported_algo_t;

typedef struct{
	uint32_t algo;
	uint32_t   purpose;
	uint32_t             modes_length;
}get_supported_block_t;


typedef struct{
	uint32_t algo;
	uint32_t   purpose;
	uint32_t             modes_length;
}get_supported_padding_t;

typedef struct{
	uint32_t algo;
	uint32_t   purpose;
	uint32_t             digests_length;
}get_supported_digests_t;

typedef struct{
	uint32_t algo;
	uint32_t             formats_length;
}get_import_formats_t;

typedef struct{
	uint32_t algo;
	uint32_t             formats_length;
}get_export_formats_t;

typedef struct{
	uint32_t key_material_size;
	uint32_t client_id_data_length;
	uint32_t app_data_length;
	uint32_t hw_length;
	uint32_t sw_length;
}get_key_character_t;

typedef struct{
	uint32_t params_length;
	uint32_t key_format;
	uint32_t key_data_length;
	uint32_t key_material_size;
	uint32_t hw_length;
	uint32_t sw_length;
}import_key_t;

typedef struct{
	uint32_t export_format;
	uint32_t export_data_length;
	uint32_t app_data_length;
	uint32_t client_id_data_length;
	uint32_t key_material_size;
}export_key_t;





/**
 * TCI message data.
 */
typedef struct {
	
	command_t     command;
    response_t    response;
	//if we get this param is 1, then we need to parse keymaster_key_param_t.block
	//also if we send a data, we need to check, if the keymaster_key_param_t data has a block type param
	//if yes, we set this param 1, then add the buffer according to the rule you make,
	unsigned keymaster_key_param_t_has_block;
	union
	{
        rsa_gen_key_t         rsa_gen_key;
        rsa_sign_t            rsa_sign;
        rsa_verify_t          rsa_verify;
        key_import_t          key_import;
        get_pub_key_t         get_pub_key;
        dsa_gen_key_t         dsa_gen_key;
        dsa_sign_t            dsa_sign;
        dsa_verify_t           dsa_verify;
        ecdsa_gen_key_t        ecdsa_gen_key;
        ecdsa_sign_t            ecdsa_sign;
        ecdsa_verify_t          ecdsa_verify;
        get_key_info_t          get_key_info;
		generate_key_t           generate_key;
		get_supported_algo_t     get_supported_algo;
		get_supported_block_t    get_supported_block;
		get_supported_padding_t  get_supported_padding;
		get_supported_digests_t  get_supported_digests;
		get_import_formats_t     get_import_format;
		get_export_formats_t     get_export_format;
		get_key_character_t		get_key_character;
	    import_key_t            import_key;
		export_key_t            export_key;
	};


} tciMessage_t, *tciMessage_ptr;


/**
 * Overall TCI structure.
 */
typedef struct {
    tciMessage_t message;   /**< TCI message */
} tci_t;


/**
 * Trustlet UUID
 */
#define TEE_KEYMASTER_TL_UUID { { 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }


#endif // __TLTEEKEYMASTERAPI_H__
