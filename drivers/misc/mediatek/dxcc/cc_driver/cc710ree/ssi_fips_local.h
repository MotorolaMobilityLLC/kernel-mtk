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

#ifndef __SSI_FIPS_LOCAL_H__
#define __SSI_FIPS_LOCAL_H__

#ifdef SSI_SUPPORT_FIPS

#include "ssi_fips.h"
struct ssi_drvdata;


#define CHECK_AND_RETURN_UPON_FIPS_TEE_ERROR() {\
	if (ssi_fips_check_fips_tee_error() != 0) {\
		return -ENOEXEC;\
	} \
}
#define CHECK_AND_RETURN_UPON_FIPS_ERROR() {\
	if (ssi_fips_check_fips_error() != 0) {\
		return -ENOEXEC;\
	} \
}
#define CHECK_AND_RETURN_VOID_UPON_FIPS_ERROR() {\
	if (ssi_fips_check_fips_error() != 0) {\
		return;\
	} \
}
#define SSI_FIPS_INIT(p_drvData)  (ssi_fips_init(p_drvData))
#define SSI_FIPS_SET_CKSUM_ERROR() (ssi_fips_set_error(CC_REE_FIPS_ERROR_ROM_CHECKSUM))

#define FIPS_LOG(...)	SSI_LOG(KERN_INFO, __VA_ARGS__)
#define FIPS_DBG(...)	//SSI_LOG(KERN_INFO, __VA_ARGS__)

/* FIPS functions */
int ssi_fips_init(struct ssi_drvdata *p_drvdata);
int ssi_fips_check_fips_tee_error(void);
int ssi_fips_check_fips_error(void);
int ssi_fips_set_error(ssi_fips_error_t err);

#else  /* SSI_SUPPORT_FIPS */

#define CHECK_AND_RETURN_UPON_FIPS_TEE_ERROR()
#define CHECK_AND_RETURN_UPON_FIPS_ERROR()
#define CHECK_AND_RETURN_VOID_UPON_FIPS_ERROR()
#define SSI_FIPS_INIT(p_drvData)  (0)
#define SSI_FIPS_SET_CKSUM_ERROR()

#endif  /* SSI_SUPPORT_FIPS */


#endif  /*__SSI_FIPS_LOCAL_H__*/

