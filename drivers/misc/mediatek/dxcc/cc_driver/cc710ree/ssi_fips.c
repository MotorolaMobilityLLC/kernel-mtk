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


/**************************************************************
This file defines the driver FIPS APIs                                                             *
***************************************************************/

#include <linux/module.h>
#include "ssi_fips.h"


extern int ssi_fips_ext_get_state(ssi_fips_state_t *p_state);
extern int ssi_fips_ext_get_error(ssi_fips_error_t *p_err);
extern int ssi_fips_set_error(ssi_fips_error_t err);

/*
This function returns the REE FIPS state.  
It should be called by kernel module. 
*/
int ssi_fips_get_state(ssi_fips_state_t *p_state)
{
        int rc = 0;

	if (p_state == NULL) {
		return -EINVAL;
	}

	rc = ssi_fips_ext_get_state(p_state);

	return rc;
}

EXPORT_SYMBOL(ssi_fips_get_state);

/*
This function returns the REE FIPS error.  
It should be called by kernel module. 
*/
int ssi_fips_get_error(ssi_fips_error_t *p_err)
{
        int rc = 0;

	if (p_err == NULL) {
		return -EINVAL;
	}

	rc = ssi_fips_ext_get_error(p_err);

	return rc;
}

EXPORT_SYMBOL(ssi_fips_get_error);

/*
This function updates the REE FIPS error state with the TEE library FIPS failure 
and as a result blocks all driver operations. It should be called when FIPS TEE 
error occured while REE library is running.  
It should be called by kernel module. 
*/
void ssi_fips_set_error_upon_tee_error(void)
{
        ssi_fips_set_error(CC_REE_FIPS_ERROR_FROM_TEE);
}

EXPORT_SYMBOL(ssi_fips_set_error_upon_tee_error);

