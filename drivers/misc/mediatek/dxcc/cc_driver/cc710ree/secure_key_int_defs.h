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


#ifndef __SECURE_KEY_INT_DEFS_H__
#define __SECURE_KEY_INT_DEFS_H__


#define DX_SECURE_KEY_RESTRICTED_REGIONS_NUM            8
#define DX_SECURE_KEY_RESTRICTED_ADDRESS_SIZE_IN_BYTES  8
#define DX_SECURE_KEY_NUM_OF_ADDRESSES_PER_RANGE        2
#define DX_SECURE_KEY_LOWER_AND_UPPER_SIZE_IN_BYTES     DX_SECURE_KEY_NUM_OF_ADDRESSES_PER_RANGE * DX_SECURE_KEY_RESTRICTED_ADDRESS_SIZE_IN_BYTES

/* secure key message format
	Word No.    Bits        Field Name
	0	    31:0        Token
	1	    31:0        Version
	2-4                     Nonce
	5           2:0         Secure key type (aes128 / aes256 / multi2)
		    3           Direction (enc / dec)
                    7:4         Cipher mode (cbc / ctr / ofb / cbc_cts)
                    15:8        Number of rounds (only for Multi2)
                    31:16       reserved
       6            31:0        Lower bound address
       7            31:0        Upper bound address
       8-17                     Restricted key  (encryption of the secured key padded with zeroes)
       18-21                    mac results
*/
#define DX_SECURE_KEY_TOKEN_OFFSET			0
#define DX_SECURE_KEY_TOKEN_VALUE			0x9DB2F60F

#define DX_SECURE_KEY_VERSION_OFFSET			1

#define DX_SECURE_KEY_NONCE_OFFSET			2

#define DX_SECURE_KEY_RESTRICT_CONFIG_OFFSET		5
#define DX_SECURE_KEY_RESTRICT_KEY_TYPE_BIT_SHIFT	0
#define DX_SECURE_KEY_RESTRICT_KEY_TYPE_BIT_SIZE	3
#define DX_SECURE_KEY_RESTRICT_DIR_BIT_SHIFT		3
#define DX_SECURE_KEY_RESTRICT_DIR_BIT_SIZE		1
#define DX_SECURE_KEY_RESTRICT_MODE_BIT_SHIFT		4
#define DX_SECURE_KEY_RESTRICT_MODE_BIT_SIZE		4
#define DX_SECURE_KEY_RESTRICT_NROUNDS_BIT_SHIFT	8
#define DX_SECURE_KEY_RESTRICT_NROUNDS_BIT_SIZE		8
#define DX_SECURE_KEY_RESTRICT_CTR_RANGE_BIT_SHIFT	16
#define DX_SECURE_KEY_RESTRICT_CTR_RANGE_BIT_SIZE	16

#define DX_SECURE_KEY_RESTRICT_LOWER_BOUND_0_OFFSET	6
#define DX_SECURE_KEY_RESTRICT_UPPER_BOUND_0_OFFSET 8

#define DX_SECURE_KEY_RESTRICT_START_TIME_STAMP_OFFSET 38
#define DX_SECURE_KEY_RESTRICT_END_TIME_STAMP_OFFSET 40


#define DX_SECURE_KEY_RESTRICT_IV_CTR_OFFSET		42

#define DX_SECURE_KEY_RESTRICT_KEY_OFFSET		    46
#define DX_SECURE_KEY_RESTRICT_KEY_SIZE_IN_BYTES	40

#define DX_SECURE_KEY_MAC_OFFSET			        56
#define DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_WORDS		60

#define HW_CIPHER_MULTI2_OFB				2

#define DX_SECURE_KEY_BYTE_MASK				0xFF

#define DX_SECURE_KEY_MAINTENANCE_MIN_PAIRS		1
#define DX_SECURE_KEY_MAINTENANCE_MAX_PAIRS		5

#define DX_SECURE_KEY_MAJOR_VERSION			2
#define DX_SECURE_KEY_MINOR_VERSION			1
#define DX_SECURE_KEY_VERSION_NUM			(DX_SECURE_KEY_MAJOR_VERSION<<16 |DX_SECURE_KEY_MINOR_VERSION )


/* CCM data structers: B0, A, text(key) - Note! each part should be a multiple of 16B */

/* B0[16B]: 0x7a,nonce[12],00,00,0x28 */
#define DX_SECURE_KEY_B0_SIZE_IN_BYTES		16
#define DX_SECURE_KEY_B0_FLAGS_OFFSET		0
#define DX_SECURE_KEY_B0_FLAGS_VALUE		0x7A
#define DX_SECURE_KEY_B0_NONCE_OFFSET		1
#define DX_SECURE_KEY_B0_DATA_LEN_OFFSET	15

/* A[32B]: 0x12,config,N,base[4],tail[4],token[4],sw_version[4],0[12]*/
#define DX_SECURE_KEY_ADATA_SIZE_IN_BYTES	176
#define DX_SECURE_KEY_ADATA_LEN_OFFSET		(DX_SECURE_KEY_B0_SIZE_IN_BYTES+1)
#define DX_SECURE_KEY_ADATA_LEN_IN_BYTES	0xAE
#define DX_SECURE_KEY_ADATA_CONFIG_OFFSET	18
#define DX_SECURE_KEY_ADATA_NROUNDS_OFFSET	19
#define DX_SECURE_KEY_ADATA_LOWER_BOUND_0_OFFSET	20 
#define DX_SECURE_KEY_ADATA_LOWER_BOUND_0_OFFSET_IN_WORDS	DX_SECURE_KEY_ADATA_LOWER_BOUND_0_OFFSET/sizeof(uint32_t) 
#define DX_SECURE_KEY_ADATA_UPPER_BOUND_0_OFFSET	28 
#define DX_SECURE_KEY_ADATA_UPPER_BOUND_0_OFFSET_IN_WORDS	DX_SECURE_KEY_ADATA_UPPER_BOUND_0_OFFSET/sizeof(uint32_t) 
#define DX_SECURE_KEY_ADATA_START_TIME_STAMP_OFFSET	148 
#define DX_SECURE_KEY_ADATA_START_TIME_STAMP_OFFSET_IN_WORDS	DX_SECURE_KEY_ADATA_START_TIME_STAMP_OFFSET/sizeof(uint32_t) 
#define DX_SECURE_KEY_ADATA_END_TIME_STAMP_OFFSET	156 
#define DX_SECURE_KEY_ADATA_END_TIME_STAMP_OFFSET_IN_WORDS	DX_SECURE_KEY_ADATA_END_TIME_STAMP_OFFSET/sizeof(uint32_t) 
#define DX_SECURE_KEY_ADATA_TOKEN_OFFSET	164 
#define DX_SECURE_KEY_ADATA_TOKEN_OFFSET_IN_WORDS		DX_SECURE_KEY_ADATA_TOKEN_OFFSET/sizeof(uint32_t) 
#define DX_SECURE_KEY_ADATA_VERSION_OFFSET	168 
#define DX_SECURE_KEY_ADATA_VERSION_OFFSET_IN_WORDS		DX_SECURE_KEY_ADATA_VERSION_OFFSET/sizeof(uint32_t) 
#define DX_SECURE_KEY_ADATA_IV_CTR_OFFSET	172 
#define DX_SECURE_KEY_ADATA_IV_CTR_OFFSET_IN_WORDS		DX_SECURE_KEY_ADATA_IV_CTR_OFFSET/sizeof(uint32_t)
#define DX_SECURE_KEY_ADATA_CTR_RANGE_OFFSET	188 
#define DX_SECURE_KEY_ADATA_CTR_RANGE_OFFSET_IN_WORDS		DX_SECURE_KEY_ADATA_CTR_RANGE_OFFSET/sizeof(uint32_t)



/* text[48B]: key[40],0[8] */
#define DX_SECURE_KEY_CCM_KEY_OFFSET		(DX_SECURE_KEY_B0_SIZE_IN_BYTES+DX_SECURE_KEY_ADATA_SIZE_IN_BYTES)
#define DX_SECURE_KEY_CCM_KEY_OFFSET_IN_WORDS	DX_SECURE_KEY_CCM_KEY_OFFSET/sizeof(uint32_t) 
#define DX_SECURE_KEY_CCM_KEY_SIZE_IN_BYTES	48 

#define DX_SECURE_KEY_CCM_BUF_IN_BYTES		(DX_SECURE_KEY_B0_SIZE_IN_BYTES+DX_SECURE_KEY_ADATA_SIZE_IN_BYTES+DX_SECURE_KEY_CCM_KEY_SIZE_IN_BYTES)
#define DX_SECURE_KEY_CCM_BUF_IN_WORDS		DX_SECURE_KEY_CCM_BUF_IN_BYTES/sizeof(uint32_t)



#define DX_SECURE_KEY_SET_NONCE_LEN(val, nonceLen)  val|=((nonceSize&0xF)<<4)
#define DX_SECURE_KEY_SET_CTR_LEN(val, ctrLen)  val|=(ctrLen&0xF)

#define DX_SECURE_KEY_GET_NONCE_LEN(val) ((val>>4)&0xF)
#define DX_SECURE_KEY_GET_CTR_LEN(val)   (val&0xF)

#endif /*__SECURE_KEY_INT_DEFS_H__*/


