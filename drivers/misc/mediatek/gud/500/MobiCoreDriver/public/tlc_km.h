/*
 * Copyright (c) 2013-2020 TRUSTONIC LIMITED
 * Copyright (C) 2021 Motorola Mobility, All Rights Reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the TRUSTONIC LIMITED nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TLC_KM_H
#define TLC_KM_H

/**
 * Command IDs
 */
#define CMD_MASK_ID                  0x0000ffffu
#define CMD_MASK_VERSION             0x001f0000u
#define CMD_SHIFT_VERSION                    16
#define CMD_MASK_RESERVED            0xffe00000u

#define KEYMASTER_WANTED_VERSION 4

// #define CMD_VERSION_TEE_KEYMASTER2   (2 << CMD_SHIFT_VERSION)
#define CMD_VERSION_TEE_KEYMASTER3   (3 << CMD_SHIFT_VERSION)
#define CMD_VERSION_TEE_KEYMASTER4   (4 << CMD_SHIFT_VERSION)

#if defined(KEYMASTER_WANTED_VERSION) && KEYMASTER_WANTED_VERSION <= 1
#  define KEYMASTER_IMPLICIT_CMD_BITS        0
#elif KEYMASTER_WANTED_VERSION == 3
#  define KEYMASTER_IMPLICIT_CMD_BITS CMD_VERSION_TEE_KEYMASTER3
#elif KEYMASTER_WANTED_VERSION == 4
#  define KEYMASTER_IMPLICIT_CMD_BITS CMD_VERSION_TEE_KEYMASTER4
#elif defined(TRUSTLET) || defined(TT_BUILD)
#  error "KEYMASTER_WANTED_VERSION unknown or unset"
#else
/* HIDL build: latest version. TODO Find a better way to control this. */
#  define KEYMASTER_IMPLICIT_CMD_BITS CMD_VERSION_TEE_KEYMASTER4
#endif

#define CMD_ID_TEE_ADD_RNG_ENTROPY              (0x01 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_GENERATE_KEY                 (0x02 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_GET_KEY_CHARACTERISTICS      (0x03 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_IMPORT_KEY                   (0x04 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_EXPORT_KEY                   (0x05 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_BEGIN                        (0x06 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_UPDATE                       (0x07 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_FINISH                       (0x08 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_ABORT                        (0x09 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_CONFIGURE                    (0x0a | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_UPGRADE_KEY                  (0x0b | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_ATTEST_KEY                   (0x0c | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_GET_HARDWARE_FEATURES        (0x0d | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_DESTROY_ATTESTATION_IDS      (0x0e | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_IMPORT_WRAPPED_KEY           (0x0f | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_GET_HMAC_SHARING_PARAMETERS  (0x10 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_COMPUTE_SHARED_HMAC          (0x11 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_VERIFY_AUTHORIZATION         (0x12 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_EARLY_BOOT_ENDED             (0x13 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_DEVICE_LOCKED                (0x14 | KEYMASTER_IMPLICIT_CMD_BITS)

// for internal use by the Keymaster TLC:
#define CMD_ID_TEE_GET_KEY_INFO           (0x0101 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_GET_OPERATION_INFO     (0x0102 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_GET_VERSION            (0x0103 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_SET_DEBUG_LIES         (0x0104 | KEYMASTER_IMPLICIT_CMD_BITS)

// for use by the key provisioning agent:
#define CMD_ID_TEE_SET_ATTESTATION_DATA  (0x0201 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_LOAD_ATTESTATION_DATA (0x0202 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_SET_ATTESTATION_IDS   (0x0203 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_LOAD_ATTESTATION_IDS  (0x0204 | KEYMASTER_IMPLICIT_CMD_BITS)

// for use by Android Linux kernel / UFS driver
#define CMD_ID_TEE_UNWRAP_AES_STORAGE_KEY (0x301 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_UNWRAP_AND_INSTALL_AES_STORAGE_KEY (0x302 | KEYMASTER_IMPLICIT_CMD_BITS)
#define CMD_ID_TEE_UNWRAP_AND_DERIVE_AES_STORAGE_SECRET (0x303 | KEYMASTER_IMPLICIT_CMD_BITS)

/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)

/**
 * TCI command header.
 */
typedef struct {
    uint32_t       commandId; /**< Command ID */
    uint32_t       padding;
} tciCommandHeader_t; // 8 bytes

/**
 * TCI response header.
 */
typedef struct {
    uint32_t       responseId; /**< Response ID (must be command ID | RSP_ID_MASK )*/
    uint32_t       returnCode; /**< Return code of command (keymaster_error_t) */
} tciResponseHeader_t; // 8 bytes

typedef struct {
    uint32_t data; /**< secure address */
    uint32_t data_length; /**< data length */
} data_blob_t;

/**
 * Command message.
 *
 * @param len Length of the data to process.
 * @param data Data to be processed
 */
typedef struct __attribute__((packed)) {
    tciCommandHeader_t header; /**< Command header */
    uint32_t len; /**< Length of data to process */
} command_t;


/**
 * Response structure
 */
typedef struct __attribute__((packed)) {
    tciResponseHeader_t header; /**< Response header */
    uint32_t len;
} response_t;


#define MAX_SESSION_NUM 16

typedef enum {
    STORAGE_KEY_EMMC_SWCQHCI = 0x01,
    STORAGE_KEY_EMMC_HWCQHCI = 0x02,
    STORAGE_KEY_UFS = 0x03
} storage_key_type_t;

/**
 * AES storage key unwrap data structure
 */
typedef struct __attribute__((packed)) {
    data_blob_t wrapped_key_data; /**< [in] AES storage key to unwrap */
} aes_storage_key_unwrap_v41_t;

/**
 * AES storage key unwrap and install data structure
 */
typedef struct __attribute__((packed)) {
    data_blob_t wrapped_key_data; /**< [in] AES storage key to unwrap */
    uint32_t cfg; /**< [in] ICE configuration */
    storage_key_type_t storage_key_type; /**< [in] ICE storage type */
} aes_storage_key_unwrap_install_v41_t;


/**
 * AES storage key unwrap and derive data structure
 */
typedef struct __attribute__((packed)) {
    data_blob_t wrapped_key_data; /**< [in] AES wrapped storage key */
    data_blob_t raw_secret; /**< [out] Raw secret unwrapped and derived from AES storage key */
} aes_storage_key_unwrap_derive_v41_t;


typedef struct __attribute__((packed)) {
    union {
        command_t command;
        response_t response;
    };

    union {
        aes_storage_key_unwrap_v41_t aes_storage_key_unwrap;
        aes_storage_key_unwrap_install_v41_t aes_storage_key_unwrap_install;
        aes_storage_key_unwrap_derive_v41_t aes_storage_key_unwrap_derive;
    };
} kmtciMessage_t;

typedef kmtciMessage_t tciMessage_t, *tciMessage_ptr;

/**
 * Overall TCI structure.
 */
typedef struct __attribute__((packed)) {
    tciMessage_t message; /**< TCI message */
} tci_t;


/**
 * Trustlet UUID
 */
#define TEE_KEYMASTER_TA_UUID { { 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x4D } }
#define TEE_KEYMASTER_M_TA_UUID TEE_KEYMASTER_TA_UUID
#define TEE_KEYMASTER_N_TA_UUID TEE_KEYMASTER_TA_UUID

#define HWKM_AES_DERIVED_RAW_SECRET_SIZE 32  /* 256 bit AES key used for the kernel fname encryption*/

#define HWKM_AES_128_BIT_KEY_SIZE 16 /* 128 bits AES key size */
#define HWKM_AES_192_BIT_KEY_SIZE 24 /* 192 bits AES Key size */
#define HWKM_AES_256_BIT_KEY_SIZE 32 /* 256 bits AES Key size */
#define HWKM_AES_512_BIT_KEY_SIZE 64 /* 512 bits AES key size */
#define HWKM_AES_STORAGE_KEY_MAX_SIZE 64 /* 512 bits AES key size max */

#define WRAPPED_STORAGE_KEY_HEADER_SIZE 8 /* 8 bytes storage wrapped key header */

enum bc_flags_bits {
    __HWKM_BC_CRYPT,        /* marks the request needs crypt */
    __HWKM_BC_IV_PAGE_IDX,  /* use page index as iv. */
    __HWKM_BC_IV_CTX,       /* use the iv saved in crypt context */
    __HWKM_BC_AES_128_XTS,  /* crypt algorithms */
    __HWKM_BC_AES_192_XTS,
    __HWKM_BC_AES_256_XTS,
    __HWKM_BC_AES_128_CBC,
    __HWKM_BC_AES_256_CBC,
    __HWKM_BC_AES_128_ECB,
    __HWKM_BC_AES_256_ECB,
};

#define HWKM_BC_CRYPT        (1UL << __HWKM_BC_CRYPT)
#define HWKM_BC_IV_PAGE_IDX  (1UL << __HWKM_BC_IV_PAGE_IDX)
#define HWKM_BC_IV_CTX       (1UL << __HWKM_BC_IV_CTX)
#define HWKM_BC_AES_128_XTS  (1UL << __HWKM_BC_AES_128_XTS)
#define HWKM_BC_AES_192_XTS  (1UL << __HWKM_BC_AES_192_XTS)
#define HWKM_BC_AES_256_XTS  (1UL << __HWKM_BC_AES_256_XTS)
#define HWKM_BC_AES_128_CBC  (1UL << __HWKM_BC_AES_128_CBC)
#define HWKM_BC_AES_256_CBC  (1UL << __HWKM_BC_AES_256_CBC)
#define HWKM_BC_AES_128_ECB  (1UL << __HWKM_BC_AES_128_ECB)
#define HWKM_BC_AES_256_ECB  (1UL << __HWKM_BC_AES_256_ECB)

enum hwkm_crypto_mode_num {
    ENCRYPTION_MODE_AES_128_XTS,
    ENCRYPTION_MODE_AES_128_CBC,
    ENCRYPTION_MODE_AES_128_CBC_ESSIV,
    ENCRYPTION_MODE_AES_128_CBC_ECB,
    ENCRYPTION_MODE_AES_256_XTS,
    ENCRYPTION_MODE_AES_256_CBC,
    ENCRYPTION_MODE_AES_256_CTS,
    ENCRYPTION_MODE_AES_256_ECB,
    ENCRYPTION_MODE_AES_MAX,
};

int hwkm_program_key(const u8 *wrapped_key,
                     u32 wrapped_key_size,
                     u32 gie_para,
                     storage_key_type_t type);

int hwkm_derive_raw_secret(const u8 *wrapped_key,
                           u32 wrapped_key_size,
                           u8 *raw_secret,
                           u32 raw_secret_size);

bool hwkm_is_slot_already_programmed(unsigned int slot_num);

void hwkm_set_slot_programmed_mask(unsigned int slot_num);

void hwkm_set_slot_evicted_mask(unsigned int slot_num);

#endif // TLC_KM_H
