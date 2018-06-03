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
This file defines the driver FIPS functions that should be 
implemented by the driver user. Current implementation is sample code only.
***************************************************************/

#include <linux/module.h>
#include "ssi_fips_local.h"
#include "ssi_driver.h"


static bool tee_error;
module_param(tee_error, bool, 0644);
MODULE_PARM_DESC(tee_error, "Simulate TEE library failure flag: 0 - no error (default), 1 - TEE error occured ");

static ssi_fips_state_t fips_state = CC_FIPS_STATE_NOT_SUPPORTED;
static ssi_fips_error_t fips_error = CC_REE_FIPS_ERROR_OK;

extern int ssi_fips_set_error(ssi_fips_error_t err);

/*
This function returns the FIPS REE state. 
The function should be implemented by the driver user, depends on where                          .
the state value is stored. 
The reference code uses global variable. 
*/
int ssi_fips_ext_get_state(ssi_fips_state_t *p_state)
{
        int rc = 0;

	if (p_state == NULL) {
		return -EINVAL;
	}

	*p_state = fips_state;

	return rc;
}

/*
This function returns the FIPS REE error. 
The function should be implemented by the driver user, depends on where                          .
the error value is stored. 
The reference code uses global variable. 
*/
int ssi_fips_ext_get_error(ssi_fips_error_t *p_err)
{
        int rc = 0;

	if (p_err == NULL) {
		return -EINVAL;
	}

	*p_err = fips_error;

	return rc;
}

/*
This function sets the FIPS REE state. 
The function should be implemented by the driver user, depends on where                          .
the state value is stored. 
The reference code uses global variable. 
*/
int ssi_fips_ext_set_state(ssi_fips_state_t state)
{
	fips_state = state;
	return 0;
}

/*
This function sets the FIPS REE status (specific error or success). 
The function should be implemented by the driver user, depends on where                          .
the error value is stored. 
The reference code uses global variable. 
*/
int ssi_fips_ext_set_error(ssi_fips_error_t err)
{
	fips_error = err;
	return 0;
}


/*
This function should get the FIPS TEE library error. 
It is used before REE library starts running it own KAT tests. 
The function should be implemented by the driver user. 
The reference code checks the tee_error value provided in module prob. 
*/
enum ssi_fips_error ssi_fips_ext_get_tee_error(void)
{
	return (tee_error ? CC_REE_FIPS_ERROR_FROM_TEE : CC_REE_FIPS_ERROR_OK);
}

/*
 This function should push the FIPS REE library status towards the TEE library.
 FIPS error can occur while running KAT tests at library init,
 or be OK in case all KAT tests had passed successfully.
 The function should be implemented by the driver user.
 The reference code prints as an indication it was called.
*/
void ssi_fips_ext_update_tee_upon_ree_status(void)
{
	ssi_fips_error_t status;
	ssi_fips_ext_get_error(&status);

	SSI_LOG_ERR("Exporting the REE FIPS status %d.\n", status);
}


