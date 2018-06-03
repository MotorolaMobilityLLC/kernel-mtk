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


#include "ssi_config.h"
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <crypto/ctr.h>
#include <linux/pm_runtime.h>
#include "ssi_driver.h"
#include "ssi_buffer_mgr.h"
#include "ssi_request_mgr.h"
#include "hw_queue_defs.h"
#include "ssi_sram_mgr.h"
#include "ssi_sysfs.h"
#include "ssi_ivgen.h"
#include "ssi_hash.h"
#include "ssi_pm.h"
#include "ssi_pm_ext.h"
#include <linux/clk.h>

static struct clk *dxcc_pub_clk;

#if defined (CONFIG_PM_RUNTIME) || defined (CONFIG_PM_SLEEP)

int ssi_power_mgr_runtime_suspend(struct device *dev)
{
	struct ssi_drvdata *drvdata =
		(struct ssi_drvdata *)dev_get_drvdata(dev);
	int rc;

	rc = ssi_request_mgr_runtime_suspend_queue(drvdata);
	if (rc != 0) {
		SSI_LOG_ERR("ssi_request_mgr_runtime_suspend_queue (%x)\n", rc);
		return rc;
	}
	fini_cc_regs(drvdata);

	/* Specific HW suspend code */
	ssi_pm_ext_hw_suspend(dev, dxcc_pub_clk);
	return 0;
}

int ssi_power_mgr_runtime_resume(struct device *dev)
{
	int rc;
	struct ssi_drvdata *drvdata =
		(struct ssi_drvdata *)dev_get_drvdata(dev);
	void __iomem *cc_base = drvdata->cc_base;
	uint32_t regVal;
	long timeout;

	/* Specific HW resume code */
	ssi_pm_ext_hw_resume(dev, dxcc_pub_clk);

	init_cc_gpr7_interrupt(drvdata);

	//check sep initialization is done (when gpr7 is different from 0)
	timeout = wait_for_completion_timeout(&((struct ssi_power_mgr_handle*)drvdata->power_mgr_handle)->sep_ready_compl, msecs_to_jiffies(5000));
	SSI_LOG_DEBUG("after wait_for_completion_interruptible_timeout .\n");

	if (timeout ==0) {
		SSI_LOG_ERR("wait_for_completion_interruptible_timeout reached timeout.\n");
		rc = -EFAULT;
		return rc;
	}

	rc = init_cc_regs(drvdata, false);
	if (rc !=0) {
		SSI_LOG_ERR("init_cc_regs (%x)\n",rc);
		return rc;
	}

	regVal = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR7));

	/* check if sep rom checksum error was reported on gpr7 */
	if ( (regVal& CC_SEP_HOST_GPR7_CKSUM_ERROR_FLAG) != 0) {
		SSI_FIPS_SET_CKSUM_ERROR();
		SSI_LOG_ERR("SEP ROM chesum error detected when PM runtime resume. \n");
	}

	rc = ssi_request_mgr_runtime_resume_queue(drvdata);
	if (rc !=0) {
		SSI_LOG_ERR("ssi_request_mgr_runtime_resume_queue (%x)\n",rc);
		return rc;
	}

	/* must be after the queue resuming as it uses the HW queue*/
	ssi_hash_init_sram_digest_consts(drvdata);
	
	ssi_ivgen_init_sram_pool(drvdata);
	return 0;
}

int ssi_power_mgr_runtime_get(struct device *dev)
{
	int rc = 0;

	if (ssi_request_mgr_is_queue_runtime_suspend(
				(struct ssi_drvdata *)dev_get_drvdata(dev))) {
		rc = pm_runtime_get_sync(dev);
	} else {
		pm_runtime_get_noresume(dev);
	}
	return rc;
}

int ssi_power_mgr_runtime_put_suspend(struct device *dev)
{
	int rc = 0;

	if (!ssi_request_mgr_is_queue_runtime_suspend(
				(struct ssi_drvdata *)dev_get_drvdata(dev))) {
		pm_runtime_mark_last_busy(dev);
		rc = pm_runtime_put_autosuspend(dev);
	}
	else {
		/* Something wrong happens*/
		BUG();
	}
	return rc;

}

#endif



int ssi_power_mgr_init(struct ssi_drvdata *drvdata)
{
	int rc = 0;
#if defined (CONFIG_PM_RUNTIME) || defined (CONFIG_PM_SLEEP)
	struct platform_device *plat_dev = drvdata->plat_dev;

	dxcc_pub_clk = devm_clk_get(&plat_dev->dev, "dxcc-pubcore-clock");
	if (IS_ERR(dxcc_pub_clk)) {
		SSI_LOG_ERR("Cannot get dxcc public core clock from common clock framework in power manager.\n");
		return PTR_ERR(dxcc_pub_clk);
	}

	/* must be before the enabling to avoid resdundent suspending */
	pm_runtime_set_autosuspend_delay(&plat_dev->dev,SSI_SUSPEND_TIMEOUT);
	pm_runtime_use_autosuspend(&plat_dev->dev);
	/* activate the PM module */
	rc = pm_runtime_set_active(&plat_dev->dev);
	if (rc != 0)
		return rc;
	/* enable the PM module*/
	pm_runtime_enable(&plat_dev->dev);
#endif
	return rc;
}

void ssi_power_mgr_fini(struct ssi_drvdata *drvdata)
{
#if defined (CONFIG_PM_RUNTIME) || defined (CONFIG_PM_SLEEP)
	struct platform_device *plat_dev = drvdata->plat_dev;

	pm_runtime_disable(&plat_dev->dev);
#endif
}

int ssi_power_mgr_alloc(struct ssi_drvdata *drvdata)
{
	struct ssi_power_mgr_handle *power_mgr_handle;

	power_mgr_handle = kmalloc(sizeof(struct ssi_power_mgr_handle),
		GFP_KERNEL);
	if (power_mgr_handle == NULL)
		return -ENOMEM;

	init_completion(&power_mgr_handle->sep_ready_compl);
	drvdata->power_mgr_handle = power_mgr_handle;
	
	return 0;
}

int ssi_power_mgr_free(struct ssi_drvdata *drvdata)
{
	struct ssi_power_mgr_handle *power_mgr_handle = 
						drvdata->power_mgr_handle;
	struct device *dev;
	dev = &drvdata->plat_dev->dev;

	if (power_mgr_handle != NULL) {
		kfree(power_mgr_handle);
		drvdata->power_mgr_handle = NULL;

	}
	return 0;
}


