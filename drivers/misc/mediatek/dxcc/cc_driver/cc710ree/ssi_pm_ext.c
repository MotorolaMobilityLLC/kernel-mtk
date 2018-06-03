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
#include "ssi_sram_mgr.h"

/*
This function should suspend the HW (if possiable), It should be implemented by
the driver user.
The reference code clears the internal SRAM to imitate lose of state.
*/
void ssi_pm_ext_hw_suspend(struct device *dev)
{
	struct ssi_drvdata *drvdata =
		(struct ssi_drvdata *)dev_get_drvdata(dev);
	unsigned int val;
	void __iomem *cc_base = drvdata->cc_base;
	unsigned int  sram_addr = 0;
#if SSI_CC_HAS_ROM
	sram_addr = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_SRAM_THRESHOLD));
#endif

	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, SRAM_ADDR), sram_addr);

	for (;sram_addr < SSI_CC_SRAM_SIZE ; sram_addr+=4) {
		WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, SRAM_DATA), 0x0);

		do {
			val = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, SRAM_DATA_READY));
		} while (!(val &0x1));
	}

	SSI_LOG_DEBUG("Disable dxcc pub clock...\n");
	clk_disable(dxcc_pub_clock);
}

/*
This function should resume the HW (if possiable).It should be implemented by
the driver user.
this function forces a Crypto Cell reset to simulate a SEP reset (kind of SEP Power Up).
Probably, in real implementation you will remove the reset function.
*/
void ssi_pm_ext_hw_resume(struct device *dev)
{
	struct ssi_drvdata *drvdata =
	(struct ssi_drvdata *)dev_get_drvdata(dev);
	void __iomem *cc_base = drvdata->cc_base;

	int ret;

	SSI_LOG_DEBUG("Enable dxcc pub clock...\n");
	ret = clk_enable(dxcc_pub_clock);
	if (ret != 0)
		SSI_LOG_ERR("Enable dxcc pub clock failed.\n");

	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_CC_SW_RST), 1);
	return;
}

