/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                     *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  *
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *
* You should have received a copy of the GNU General Public License along    *
* with this program; if not, write to the Free Software Foundation, 	     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.        *
******************************************************************************/

/*! @file
This file contains secure key related enums and definitions.
*/

#ifndef __SECURE_KEY_DEFS_H__
#define __SECURE_KEY_DEFS_H__


/******************************************************************************
*				DEFINITIONS
******************************************************************************/

/*! The number of address boundry slots */
#define DX_NUM_OF_ADDRESS_BOUNDARY_SLOTS 8

/*! The secure key package buffer size in bytes */
#define DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES 	240

/*! The secure key nonce size in bytes */
#define DX_SECURE_KEY_NONCE_SIZE_IN_BYTES		12

/*! Multi2 minimum number of rounds  */
#define DX_SECURE_KEY_MULTI2_MIN_ROUNDS			8
/*! Multi2 maximum number of rounds  */
#define DX_SECURE_KEY_MULTI2_MAX_ROUNDS			128
/*! Maximum CTR range value */
#define DX_SECURE_KEY_MAX_CTR_RANGE_VALUE		0x10000

//bit 7 in GPR7 is used by SEP to indicate ROM checksum error 
#define CC_SEP_HOST_GPR7_CKSUM_ERROR_FLAG 0x80

/******************************************************************************
*				TYPE DEFINITIONS
******************************************************************************/

/*!
Secure key type
*/ 
enum secure_key_type {
	DX_SECURE_KEY_AES_KEY128  = 0,		/*!< AES key 128 */
	DX_SECURE_KEY_AES_KEY256  = 1,		/*!< AES key 256 */
	DX_SECURE_KEY_MULTI2 	  = 2,		/*!< multi2 */
	DX_SECURE_KEY_BYPASS 	  = 3,		/*!< bypass */
	DX_SECURE_KEY_MAINTENANCE  = 7,		/*!< maintenance */
	DX_SECURE_KEY_KSIZE_RESERVE32B = INT32_MAX
};

/*!
Cipher operation mode
*/ 
enum secure_key_cipher_mode {
	DX_SECURE_KEY_CIPHER_CBC = 1,			/*!< CBC mode */
	DX_SECURE_KEY_CIPHER_CTR = 2,			/*!< CTR mode */
	DX_SECURE_KEY_CIPHER_OFB = 6,			/*!< OFB mode */
	DX_SECURE_KEY_CIPHER_CTR_NONCE_CTR_PROT_NSP = 9,/*!< CTR nonce CTR prot NSP mode */
	DX_SECURE_KEY_CIPHER_CTR_NONCE_PROT = 10,	/*!< CTR nonce prot mode */
	DX_SECURE_KEY_CIPHER_CBC_CTS = 11,		/*!< CBC-CTS mode */
	DX_SECURE_KEY_CIPHER_RESERVE32B = INT32_MAX
};

/*!
Direction mode (Encrypt or Decrypt)
*/ 
enum secure_key_direction {
	DX_SECURE_KEY_DIRECTION_ENCRYPT = 0,	/*!< Encrypt mode */
	DX_SECURE_KEY_DIRECTION_DECRYPT = 1,	/*!< Decrypt mode */
	DX_SECURE_KEY_DIRECTION_RESERVE32B = INT32_MAX
};


/*! Defines the secure key package buffer. */
typedef uint8_t skeyPackageBuf_t[DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES];

/*! Defines the secure key nonce buffer. */
typedef uint8_t skeyNonceBuf_t[DX_SECURE_KEY_NONCE_SIZE_IN_BYTES];


#endif /*__SECURE_KEY_DEFS_H__*/


