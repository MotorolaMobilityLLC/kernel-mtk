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

/* \file ssi_pm.h
    */

#ifndef __SSI_POWER_MGR_H__
#define __SSI_POWER_MGR_H__


#include "ssi_config.h"
#include "ssi_driver.h"


#define SSI_SUSPEND_TIMEOUT 3000

struct ssi_power_mgr_handle {
	struct completion sep_ready_compl; /* sep notify ready after power up */
};


int ssi_power_mgr_init(struct ssi_drvdata *drvdata);

void ssi_power_mgr_fini(struct ssi_drvdata *drvdata);

#if defined (CONFIG_PM_RUNTIME) || defined (CONFIG_PM_SLEEP)
int ssi_power_mgr_runtime_suspend(struct device *dev);

int ssi_power_mgr_runtime_resume(struct device *dev);

int ssi_power_mgr_runtime_get(struct device *dev);

int ssi_power_mgr_runtime_put_suspend(struct device *dev);
#endif

int ssi_power_mgr_alloc(struct ssi_drvdata *drvdata);

int ssi_power_mgr_free(struct ssi_drvdata *drvdata);

#endif /*__POWER_MGR_H__*/

