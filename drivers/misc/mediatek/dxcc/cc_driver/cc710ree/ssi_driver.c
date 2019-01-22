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

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/sha.h>
#include <crypto/aead.h>
#include <crypto/authenc.h>
#include <crypto/scatterwalk.h>
#include <crypto/internal/skcipher.h>

#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/random.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/sysctl.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/pm.h>

/* cache.h required for L1_CACHE_ALIGN() and cache_line_size() */
#include <linux/cache.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <linux/of.h>
#include <linux/clk.h>

#include "ssi_config.h"
#include "ssi_driver.h"
#include "ssi_request_mgr.h"
#include "ssi_buffer_mgr.h"
#include "ssi_sysfs.h"
#include "ssi_cipher.h"
#include "ssi_aead.h"
#include "ssi_hash.h"
#include "ssi_ivgen.h"
#include "ssi_sram_mgr.h"
#include "ssi_pm.h"
#include "ssi_fips_local.h"

#define DX_CACHE_PARAMS_SET_MASK 0x80000000
#define DX_CACHE_PARAMS_SET_TIMEOUT_MS 100

static struct clk *dxcc_pub_clock;

#ifdef DX_DUMP_BYTES
void dump_byte_array(const char *name, const uint8_t *the_array, unsigned long size)
{
	int i , line_offset = 0, ret = 0;
	const uint8_t *cur_byte;
	char line_buf[80];

	if (the_array == NULL) {
		SSI_LOG_ERR("cannot dump_byte_array - NULL pointer\n");
		return;
	}

	ret = snprintf(line_buf, sizeof(line_buf), "%s[%lu]: ",
		name, size);
	if (ret < 0) {
		SSI_LOG_ERR("snprintf returned %d . aborting buffer array dump\n",ret);
		return;
	}
	line_offset = ret;
	for (i = 0 , cur_byte = the_array;
	     (i < size) && (line_offset < sizeof(line_buf)); i++, cur_byte++) {
			ret = snprintf(line_buf + line_offset,
					sizeof(line_buf) - line_offset,
					"0x%02X ", *cur_byte);
		if (ret < 0)
		{
			SSI_LOG_ERR("snprintf returned %d . aborting buffer array dump\n",ret);
			return;
		}
		line_offset += ret;
		if (line_offset > 75) { /* Cut before line end */
			SSI_LOG_DEBUG("%s\n", line_buf);
			line_offset = 0;
		}
	}

	if (line_offset > 0) /* Dump remainding line */
		SSI_LOG_DEBUG("%s\n", line_buf);

}
#endif

static irqreturn_t cc_isr(int irq, void *dev_id)
{
	struct ssi_drvdata *drvdata = (struct ssi_drvdata *)dev_id;
	void __iomem *cc_base = drvdata->cc_base;
	uint32_t irr;
	struct ssi_power_mgr_handle *power_mgr_h = (struct ssi_power_mgr_handle *)drvdata->power_mgr_handle;
	DECL_CYCLE_COUNT_RESOURCES;

	/* STAT_OP_TYPE_GENERIC STAT_PHASE_0: Interrupt */
	START_CYCLE_COUNT();

	/* read the interrupt status */
	irr = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IRR));
	SSI_LOG_DEBUG("Got IRR=0x%08X\n", irr);
	if (unlikely(irr == 0)) { /* Probably shared interrupt line */
		SSI_LOG_ERR("Got interrupt with empty IRR\n");
		return IRQ_NONE;
	}

	/* clear interrupt - must be before processing events */
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_ICR), irr);

	/* Completion interrupt - most probable */
	if (likely((irr & SSI_COMP_IRQ_MASK) != 0)) {
		/* Mask AXI completion interrupt */
		WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR), 
			READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR)) | SSI_COMP_IRQ_MASK);
		complete_request(drvdata);
		irr &= ~SSI_COMP_IRQ_MASK;
	}

	/* GPR7 interrupt */
	if (unlikely(((irr & SSI_GPR7_IRQ_MASK) != 0) && ((~READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR)) & SSI_GPR7_IRQ_MASK)!=0))) {
		SSI_LOG_DEBUG("Got GPR7 IRQ in cc_isr. masking gpr7 until next resume. set sep_ready_compl\n");
		/* Mask GPR7 interrupt */
		WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR), 
			READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR)) | SSI_GPR7_IRQ_MASK);
		complete(&power_mgr_h->sep_ready_compl);
		irr &= ~SSI_GPR7_IRQ_MASK;
	}

	/* AXI error interrupt */
	if (unlikely((irr & SSI_AXI_ERR_IRQ_MASK) != 0)) {
		uint32_t axi_err;
		
		/* Read the AXI error ID */
		axi_err = READ_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_MON_ERR));
		SSI_LOG_DEBUG("AXI completion error: axim_mon_err=0x%08X\n", axi_err);
		
		irr &= ~SSI_AXI_ERR_IRQ_MASK;
	}

	if (unlikely(irr != 0)) {
		SSI_LOG_DEBUG("IRR includes unknown cause bits (0x%08X)\n", irr);
		/* Just warning */
	}

	END_CYCLE_COUNT(STAT_OP_TYPE_GENERIC, STAT_PHASE_0);
	START_CYCLE_COUNT_AT(drvdata->isr_exit_cycles);

	return IRQ_HANDLED;
}

void init_cc_gpr7_interrupt(struct ssi_drvdata *drvdata)
{
	unsigned int val;
	void __iomem *cc_base = drvdata->cc_base;

	/* Clear all pending interrupts except GPR7 interrupts */
	val = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IRR));
	SSI_LOG_DEBUG("IRR=0x%08X\n", val);
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_ICR), val & (~SSI_GPR7_IRQ_MASK));

	/* Unmask relevant interrupt cause */
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR),~(SSI_GPR7_IRQ_MASK));
}

int init_cc_regs(struct ssi_drvdata *drvdata, bool is_probe)
{
	unsigned int val;
	void __iomem *cc_base = drvdata->cc_base;
#if SSI_CC_HAS_ROM
	uint32_t rom_ver;

	rom_ver = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR0));
	if (rom_ver != DX_ROM_VERSION) {
		SSI_LOG_ERR("wrong ROM version: ROM version=0x%08X != expected=0x%08X\n",
			rom_ver, DX_ROM_VERSION);
		return -EINVAL;
	}
	drvdata->rom_ver = rom_ver;
#endif

	/* Unmask all AXI interrupt sources AXI_CFG1 register */
	val = READ_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CFG1));
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CFG1), val & ~SSI_AXI_IRQ_MASK);
	SSI_LOG_DEBUG("AXIM_CFG1=0x%08X\n", READ_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CFG1)));

	/* Clear all pending interrupts */
	val = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IRR));
	SSI_LOG_DEBUG("IRR=0x%08X\n", val);
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_ICR), val);

	/* Unmask relevant interrupt cause */
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR),
			~(SSI_COMP_IRQ_MASK | SSI_AXI_ERR_IRQ_MASK));
		
#ifdef DX_HOST_IRQ_TIMER_INIT_VAL_REG_OFFSET
#ifdef DX_IRQ_DELAY
	/* Set CC IRQ delay */
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IRQ_TIMER_INIT_VAL),
		DX_IRQ_DELAY);
#endif
	if (READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IRQ_TIMER_INIT_VAL)) > 0) {
		SSI_LOG_DEBUG("irq_delay=%d CC cycles\n",
			READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IRQ_TIMER_INIT_VAL)));
	}
#endif
	
	val = READ_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CACHE_PARAMS));
	if (is_probe == true) {
		SSI_LOG_INFO("Cache params previous: 0x%08X\n", val);
	}
	WRITE_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CACHE_PARAMS), SSI_CACHE_PARAMS);
	val = READ_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CACHE_PARAMS));
	if (is_probe == true) {
		SSI_LOG_INFO("Cache params current: 0x%08X\n", val);
	}
	return 0;
}

static int init_cc_resources(struct platform_device *plat_dev)
{
	struct resource *req_mem_cc_regs = NULL;
	void __iomem *cc_base = NULL;
	bool irq_registered = false;
	struct ssi_drvdata *new_drvdata = kzalloc(sizeof(struct ssi_drvdata), GFP_KERNEL);
	uint32_t signature_val;
#if SSI_CC_HAS_ROM
	uint32_t rom_ver;
#endif
	int rc = 0;

	if (unlikely(new_drvdata == NULL)) {
		SSI_LOG_ERR("Failed to allocate drvdata");
		rc = -ENOMEM;
		goto init_cc_res_err;
	}

	/*Initialize inflight counter used in dx_ablkcipher_secure_complete used for count of BYSPASS blocks operations*/
	new_drvdata->inflight_counter = 0;

	dev_set_drvdata(&plat_dev->dev, new_drvdata);
	/* Get device resources */
	/* First CC registers space */
	new_drvdata->res_mem = platform_get_resource(plat_dev, IORESOURCE_MEM, 0);
	if (unlikely(new_drvdata->res_mem == NULL)) {
		SSI_LOG_ERR("Failed getting IO memory resource\n");
		rc = -ENODEV;
		goto init_cc_res_err;
	}
	SSI_LOG_DEBUG("Got MEM resource (%s): start=0x%llX end=0x%llX\n",
		new_drvdata->res_mem->name,
		(unsigned long long)new_drvdata->res_mem->start,
		(unsigned long long)new_drvdata->res_mem->end);
	/* Map registers space */
	req_mem_cc_regs = request_mem_region(new_drvdata->res_mem->start, resource_size(new_drvdata->res_mem), "dx_cc63p_regs");
	if (unlikely(req_mem_cc_regs == NULL)) {
		SSI_LOG_ERR("Couldn't allocate registers memory region at "
			     "0x%08X\n", (unsigned int)new_drvdata->res_mem->start);
		rc = -EBUSY;
		goto init_cc_res_err;
	}
	cc_base = ioremap(new_drvdata->res_mem->start, resource_size(new_drvdata->res_mem));
	if (unlikely(cc_base == NULL)) {
		SSI_LOG_ERR("ioremap[CC](0x%08X,0x%08X) failed\n",
			(unsigned int)new_drvdata->res_mem->start, (unsigned int)resource_size(new_drvdata->res_mem));
		rc = -ENOMEM;
		goto init_cc_res_err;
	}
	SSI_LOG_DEBUG("CC registers mapped from %pa to 0x%p\n", &new_drvdata->res_mem->start, cc_base);
	new_drvdata->cc_base = cc_base;
	

	/* Then IRQ */
	new_drvdata->res_irq = platform_get_resource(plat_dev, IORESOURCE_IRQ, 0);
	if (unlikely(new_drvdata->res_irq == NULL)) {
		SSI_LOG_ERR("Failed getting IRQ resource\n");
		rc = -ENODEV;
		goto init_cc_res_err;
	}
	rc = request_irq(new_drvdata->res_irq->start, cc_isr,
			 IRQF_SHARED , "cc63_perf_test", new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("Could not register to interrupt %llu\n",
			(unsigned long long)new_drvdata->res_irq->start);
		goto init_cc_res_err;
	}
	init_completion(&new_drvdata->icache_setup_completion);

	irq_registered = true;
	SSI_LOG_DEBUG("Registered to IRQ (%s) %llu\n",
		new_drvdata->res_irq->name,
		(unsigned long long)new_drvdata->res_irq->start);

	new_drvdata->plat_dev = plat_dev;

#ifdef CONFIG_ARM
#ifdef DISABLE_COHERENT_DMA_OPS
#warning Using default (incoherent) DMA operations
#else
	/* ARM-specific DMA coherency operations option */
	set_dma_ops(&plat_dev->dev, &arm_coherent_dma_ops);
#endif
#else
/* #warning Not an ARM(32) platform - Using default DMA operations */ //Bypass build error
#endif

	if(new_drvdata->plat_dev->dev.dma_mask == NULL)
	{
		new_drvdata->plat_dev->dev.dma_mask = & new_drvdata->plat_dev->dev.coherent_dma_mask;
	}
	if (!new_drvdata->plat_dev->dev.coherent_dma_mask)
	{
		new_drvdata->plat_dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	}

	/* Verify correct mapping */
	signature_val = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_CC_SIGNATURE));
	if (signature_val != DX_DEV_SIGNATURE) {
		SSI_LOG_ERR("Invalid CC signature: SIGNATURE=0x%08X != expected=0x%08X\n",
			signature_val, (uint32_t)DX_DEV_SIGNATURE);
		rc = -EINVAL;
		goto init_cc_res_err;
	}
	SSI_LOG_DEBUG("CC SIGNATURE=0x%08X\n", signature_val);

#if SSI_CC_HAS_ROM
	rom_ver = READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR0));
	if (rom_ver != DX_ROM_VERSION) {
		SSI_LOG_ERR("wrong ROM version: ROM version=0x%08X != expected=0x%08X\n",
			rom_ver, DX_ROM_VERSION);
		rc = -EINVAL;
		goto init_cc_res_err;
	}
	/* Display ROM versions */
	SSI_LOG(KERN_INFO, "CryptoCell cc710ree Driver: ROM version 0x%08X\n", rom_ver);
#endif

	/* Display HW versions */
	SSI_LOG(KERN_INFO, "ARM CryptoCell %s Driver: HW version 0x%08X, Driver version %s\n", SSI_DEV_NAME_STR,
		READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_VERSION)), DRV_MODULE_VERSION);

	rc = ssi_power_mgr_alloc(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("ssi_power_mgr_alloc failed\n");
		goto init_cc_res_err;
	}

	rc = init_cc_regs(new_drvdata, true);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("init_cc_regs failed\n");
		goto init_cc_res_err;
	}

#ifdef ENABLE_CYCLE_COUNT
	rc = ssi_sysfs_init(&(plat_dev->dev.kobj), new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("init_stat_db failed\n");
		goto init_cc_res_err;
	}
#endif

	rc = ssi_sram_mgr_init(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("ssi_sram_mgr_init failed\n");
		goto init_cc_res_err;
	}

	new_drvdata->mlli_sram_addr =
		ssi_sram_mgr_alloc(new_drvdata, MAX_MLLI_BUFF_SIZE);
	if (unlikely(new_drvdata->mlli_sram_addr == NULL_SRAM_ADDR)) {
		SSI_LOG_ERR("Failed to alloc MLLI Sram buffer\n");
		rc = -ENOMEM;
		goto init_cc_res_err;
	}

	rc = request_mgr_init(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("request_mgr_init failed\n");
		goto init_cc_res_err;
	}

	rc = ssi_buffer_mgr_init(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("buffer_mgr_init failed\n");
		goto init_cc_res_err;
	}

	rc = ssi_power_mgr_init(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("ssi_power_mgr_init failed\n");
		goto init_cc_res_err;
	}

	rc = SSI_FIPS_INIT(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("SSI_FIPS_INIT failed 0x%x\n", rc);
		goto init_cc_res_err;
	}

	rc = ssi_ivgen_init(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("ssi_ivgen_init failed\n");
		goto init_cc_res_err;
	}

	/* Allocate crypto algs */
	rc = ssi_ablkcipher_alloc(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("ssi_ablkcipher_alloc failed\n");
		goto init_cc_res_err;
	}

	/* hash must be allocated before aead since hash exports APIs */
	rc = ssi_hash_alloc(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("ssi_hash_alloc failed\n");
		goto init_cc_res_err;
	}

	rc = ssi_aead_alloc(new_drvdata);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("ssi_aead_alloc failed\n");
		goto init_cc_res_err;
	}

	return 0;

init_cc_res_err:
	SSI_LOG_ERR("Freeing CC HW resources!\n");
	
	if (new_drvdata != NULL) {
		ssi_aead_free(new_drvdata);
		ssi_hash_free(new_drvdata);
		ssi_ablkcipher_free(new_drvdata);
		ssi_ivgen_fini(new_drvdata);
		ssi_power_mgr_fini(new_drvdata);
		ssi_buffer_mgr_fini(new_drvdata);
		request_mgr_fini(new_drvdata);
		ssi_sram_mgr_fini(new_drvdata);
#ifdef ENABLE_CYCLE_COUNT
		ssi_sysfs_fini();
#endif
		ssi_power_mgr_free(new_drvdata);
		if (req_mem_cc_regs != NULL) {
			if (irq_registered) {
				free_irq(new_drvdata->res_irq->start, new_drvdata);
				new_drvdata->res_irq = NULL;
				iounmap(cc_base);
				new_drvdata->cc_base = NULL;
			}
			release_mem_region(new_drvdata->res_mem->start,
				resource_size(new_drvdata->res_mem));
			new_drvdata->res_mem = NULL;
		}
		kfree(new_drvdata);
		dev_set_drvdata(&plat_dev->dev, NULL);
	}

	return rc;
}

void fini_cc_regs(struct ssi_drvdata *drvdata)
{
	/* Mask all interrupts */
	WRITE_REGISTER(drvdata->cc_base + 
		       SASI_REG_OFFSET(HOST_RGF, HOST_IMR), 0xFFFFFFFF);

}

static void cleanup_cc_resources(struct platform_device *plat_dev)
{
	struct ssi_drvdata *drvdata =
		(struct ssi_drvdata *)dev_get_drvdata(&plat_dev->dev);

	ssi_aead_free(drvdata);
	ssi_hash_free(drvdata);
	ssi_ablkcipher_free(drvdata);
	ssi_ivgen_fini(drvdata);
	ssi_power_mgr_fini(drvdata);
	ssi_buffer_mgr_fini(drvdata);
	request_mgr_fini(drvdata);
	ssi_sram_mgr_fini(drvdata);
#ifdef ENABLE_CYCLE_COUNT
	ssi_sysfs_fini();
#endif

	/* Mask all interrupts */
	fini_cc_regs(drvdata);
	free_irq(drvdata->res_irq->start, drvdata);
	drvdata->res_irq = NULL;

	if (drvdata->cc_base != NULL) {
		iounmap(drvdata->cc_base);
		release_mem_region(drvdata->res_mem->start,
			resource_size(drvdata->res_mem));
		drvdata->cc_base = NULL;
		drvdata->res_mem = NULL;
	}

	kfree(drvdata);
	dev_set_drvdata(&plat_dev->dev, NULL);
}

static int cc64_probe(struct platform_device *plat_dev)
{
	int rc;

#if defined(CONFIG_ARM) && defined(DX_DEBUG)
	uint32_t ctr, cacheline_size;

	asm volatile("mrc p15, 0, %0, c0, c0, 1" : "=r" (ctr));
	cacheline_size =  4 << ((ctr >> 16) & 0xf);
	SSI_LOG_DEBUG("CP15(L1_CACHE_BYTES) = %u , Kconfig(L1_CACHE_BYTES) = %u\n",
		cacheline_size, L1_CACHE_BYTES);

	asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (ctr));
	SSI_LOG_DEBUG("Main ID register (MIDR): Implementer 0x%02X, Arch 0x%01X,"
		     " Part 0x%03X, Rev r%dp%d\n",
		(ctr>>24), (ctr>>16)&0xF, (ctr>>4)&0xFFF, (ctr>>20)&0xF, ctr&0xF);
#endif

	dxcc_pub_clock = devm_clk_get(&plat_dev->dev, "dxcc-pubcore-clock");
	if (IS_ERR(dxcc_pub_clock)) {
		SSI_LOG_ERR("Cannot get dxcc public core clock from common clock framework.\n");
		return PTR_ERR(dxcc_pub_clock);
	}

	rc = clk_prepare_enable(dxcc_pub_clock);
	if (rc != 0) {
		SSI_LOG_ERR("Cannot prepare dxcc public core clock from common clock framework.\n");
		return rc;
	}

	/* Map registers space */
	rc = init_cc_resources(plat_dev);
	if (rc != 0)
		return rc;

	SSI_LOG(KERN_INFO, "ARM cc710ree device initialized\n");

	return 0;
}

static int cc64_remove(struct platform_device *plat_dev)
{
	SSI_LOG_DEBUG("Releasing CC64 resources...\n");

	cleanup_cc_resources(plat_dev);

	SSI_LOG(KERN_INFO, "ARM cc710ree device terminated\n");
#ifdef ENABLE_CYCLE_COUNT
	display_all_stat_db();
#endif
	clk_disable_unprepare(dxcc_pub_clock);
	return 0;
}
#if defined (CONFIG_PM_RUNTIME) || defined (CONFIG_PM_SLEEP)
static struct dev_pm_ops dx_driver_pm = {
	SET_RUNTIME_PM_OPS(ssi_power_mgr_runtime_suspend, ssi_power_mgr_runtime_resume, NULL)
};
#endif
#if defined (CONFIG_PM_RUNTIME) || defined (CONFIG_PM_SLEEP)

#define	DX_DRIVER_RUNTIME_PM	(&dx_driver_pm)

#else

#define	DX_DRIVER_RUNTIME_PM	NULL

#endif


#ifdef CONFIG_OF
static const struct of_device_id dx_dev_of_match[] = {
	{.compatible = "mediatek,dxcc_pub"},	// device id is taken from system.dts.dx in FPGA image
	{}
};
MODULE_DEVICE_TABLE(of, dx_dev_of_match);
#endif

static struct platform_driver cc64_driver = {
	.driver = {
		   .name = "cc710ree",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = dx_dev_of_match,
#endif
		   .pm = DX_DRIVER_RUNTIME_PM,
	},
	.probe = cc64_probe,
	.remove = cc64_remove,
};
module_platform_driver(cc64_driver);

/* Module description */
MODULE_DESCRIPTION("ARM cc710ree Driver");
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_AUTHOR("ARM");
MODULE_LICENSE("GPL v2");
