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

#ifndef __SSI_IVGEN_H__
#define __SSI_IVGEN_H__

#include "hw_queue_defs.h"


#define SSI_IVPOOL_SEQ_LEN 8

/*!
 * Allocates iv-pool and maps resources. 
 * This function generates the first IV pool.  
 * 
 * \param drvdata Driver's private context
 * 
 * \return int Zero for success, negative value otherwise.
 */
int ssi_ivgen_init(struct ssi_drvdata *drvdata);

/*!
 * Free iv-pool and ivgen context.
 *  
 * \param drvdata 
 */
void ssi_ivgen_fini(struct ssi_drvdata *drvdata);

/*!
 * Generates the initial pool in SRAM. 
 * This function should be invoked when resuming DX driver. 
 * 
 * \param drvdata 
 *  
 * \return int Zero for success, negative value otherwise.
 */
int ssi_ivgen_init_sram_pool(struct ssi_drvdata *drvdata);

/*!
 * Acquires 16 Bytes IV from the iv-pool
 * 
 * \param drvdata Driver private context
 * \param iv_out_dma Array of physical IV out addresses
 * \param iv_out_dma_len Length of iv_out_dma array (additional elements of iv_out_dma array are ignore)
 * \param iv_out_size May be 8 or 16 bytes long 
 * \param iv_seq IN/OUT array to the descriptors sequence
 * \param iv_seq_len IN/OUT pointer to the sequence length 
 *  
 * \return int Zero for success, negative value otherwise. 
 */
int ssi_ivgen_getiv(
	struct ssi_drvdata *drvdata,
	dma_addr_t iv_out_dma[],
	unsigned int iv_out_dma_len,
	unsigned int iv_out_size,
	HwDesc_s iv_seq[],
	unsigned int *iv_seq_len);

#endif /*__SSI_IVGEN_H__*/
