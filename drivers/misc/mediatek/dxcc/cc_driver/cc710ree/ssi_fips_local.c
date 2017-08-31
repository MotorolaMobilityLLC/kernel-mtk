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
This file defines the driver FIPS internal function, used by the driver itself.
***************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <crypto/des.h>

#include "ssi_config.h"
#include "ssi_driver.h"

#ifdef TRUSTONIC_FIPS_SUPPORT
#include <linux/kthread.h>
#endif

#define FIPS_POWER_UP_TEST_CIPHER	1
#define FIPS_POWER_UP_TEST_CMAC		1
#define FIPS_POWER_UP_TEST_HASH		1
#define FIPS_POWER_UP_TEST_HMAC		1
#define FIPS_POWER_UP_TEST_CCM		1
#define FIPS_POWER_UP_TEST_GCM		1

static bool ssi_fips_support = 1;
module_param(ssi_fips_support, bool, 0644);
MODULE_PARM_DESC(ssi_fips_support, "FIPS supported flag: 0 - off , 1 - on (default)");

#ifdef TRUSTONIC_FIPS_SUPPORT
static struct task_struct *tee_thread;
static ssi_fips_error_t test_error;
#endif


extern int ssi_fips_get_state(ssi_fips_state_t *p_state);
extern int ssi_fips_get_error(ssi_fips_error_t *p_err);
extern int ssi_fips_ext_set_state(ssi_fips_state_t state);
extern int ssi_fips_ext_set_error(ssi_fips_error_t err);
extern void ssi_fips_ext_update_tee_upon_ree_status(void);
extern enum ssi_fips_error ssi_fips_ext_get_tee_error(void);

/* FIPS power-up tests */
extern ssi_fips_error_t ssi_cipher_fips_power_up_tests(struct ssi_drvdata *drvdata, void *cpu_addr_buffer, dma_addr_t dma_coherent_buffer);
extern ssi_fips_error_t ssi_cmac_fips_power_up_tests(struct ssi_drvdata *drvdata, void *cpu_addr_buffer, dma_addr_t dma_coherent_buffer);
extern ssi_fips_error_t ssi_hash_fips_power_up_tests(struct ssi_drvdata *drvdata, void *cpu_addr_buffer, dma_addr_t dma_coherent_buffer);
extern ssi_fips_error_t ssi_hmac_fips_power_up_tests(struct ssi_drvdata *drvdata, void *cpu_addr_buffer, dma_addr_t dma_coherent_buffer);
extern ssi_fips_error_t ssi_ccm_fips_power_up_tests(struct ssi_drvdata *drvdata, void *cpu_addr_buffer, dma_addr_t dma_coherent_buffer);
extern ssi_fips_error_t ssi_gcm_fips_power_up_tests(struct ssi_drvdata *drvdata, void *cpu_addr_buffer, dma_addr_t dma_coherent_buffer);
extern size_t ssi_fips_max_mem_alloc_size(void);



ssi_fips_error_t cc_fips_run_power_up_tests(struct ssi_drvdata *drvdata)
{
	ssi_fips_error_t fips_error = CC_REE_FIPS_ERROR_OK;
	void * cpu_addr_buffer = NULL;
	dma_addr_t dma_handle;
	size_t alloc_buff_size = ssi_fips_max_mem_alloc_size();
	struct device *dev = &drvdata->plat_dev->dev;
	void __iomem *cc_base = drvdata->cc_base;

	
	if ( (READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR7))& CC_SEP_HOST_GPR7_CKSUM_ERROR_FLAG) != 0) {
		fips_error = CC_REE_FIPS_ERROR_ROM_CHECKSUM;
		FIPS_DBG("SEP ROM chesum error detected. (fips_error = %d) \n", fips_error);
		return CC_REE_FIPS_ERROR_ROM_CHECKSUM;
	}

	// allocate memory using dma_alloc_coherent - for phisical, consecutive and cache coherent buffer (memory map is not needed)
	// the return value is the virtual address - use it to copy data into the buffer
	// the dma_handle is the returned phy address - use it in the HW descriptor
	FIPS_DBG("dma_alloc_coherent \n");
	cpu_addr_buffer = dma_alloc_coherent(dev, alloc_buff_size, &dma_handle, GFP_KERNEL);
	if (cpu_addr_buffer == NULL) {
		return CC_REE_FIPS_ERROR_GENERAL;
	}
	FIPS_DBG("allocated coherent buffer - addr 0x%08X , size = %d \n", (size_t)cpu_addr_buffer, alloc_buff_size);

#if FIPS_POWER_UP_TEST_CIPHER
	FIPS_DBG("ssi_cipher_fips_power_up_tests ...\n");
	fips_error = ssi_cipher_fips_power_up_tests(drvdata, cpu_addr_buffer, dma_handle);
	FIPS_DBG("ssi_cipher_fips_power_up_tests - done. (fips_error = %d) \n", fips_error);
#endif
#if FIPS_POWER_UP_TEST_CMAC
	if (likely(fips_error == CC_REE_FIPS_ERROR_OK)) {
		FIPS_DBG("ssi_cmac_fips_power_up_tests ...\n");
		fips_error = ssi_cmac_fips_power_up_tests(drvdata, cpu_addr_buffer, dma_handle);
		FIPS_DBG("ssi_cmac_fips_power_up_tests - done. (fips_error = %d) \n", fips_error);
	}
#endif
#if FIPS_POWER_UP_TEST_HASH
	if (likely(fips_error == CC_REE_FIPS_ERROR_OK)) {
		FIPS_DBG("ssi_hash_fips_power_up_tests ...\n");
		fips_error = ssi_hash_fips_power_up_tests(drvdata, cpu_addr_buffer, dma_handle);
		FIPS_DBG("ssi_hash_fips_power_up_tests - done. (fips_error = %d) \n", fips_error);
	}
#endif
#if FIPS_POWER_UP_TEST_HMAC
	if (likely(fips_error == CC_REE_FIPS_ERROR_OK)) {
		FIPS_DBG("ssi_hmac_fips_power_up_tests ...\n");
		fips_error = ssi_hmac_fips_power_up_tests(drvdata, cpu_addr_buffer, dma_handle);
		FIPS_DBG("ssi_hmac_fips_power_up_tests - done. (fips_error = %d) \n", fips_error);
	}
#endif
#if FIPS_POWER_UP_TEST_CCM
	if (likely(fips_error == CC_REE_FIPS_ERROR_OK)) {
		FIPS_DBG("ssi_ccm_fips_power_up_tests ...\n");
		fips_error = ssi_ccm_fips_power_up_tests(drvdata, cpu_addr_buffer, dma_handle);
		FIPS_DBG("ssi_ccm_fips_power_up_tests - done. (fips_error = %d) \n", fips_error);
	}
#endif
#if FIPS_POWER_UP_TEST_GCM
	if (likely(fips_error == CC_REE_FIPS_ERROR_OK)) {
		FIPS_DBG("ssi_gcm_fips_power_up_tests ...\n");
		fips_error = ssi_gcm_fips_power_up_tests(drvdata, cpu_addr_buffer, dma_handle);
		FIPS_DBG("ssi_gcm_fips_power_up_tests - done. (fips_error = %d) \n", fips_error);
	}
#endif
	/* deallocate the buffer when all tests are done... */
	FIPS_DBG("dma_free_coherent \n");
	dma_free_coherent(dev, alloc_buff_size, cpu_addr_buffer, dma_handle);

	return fips_error;
}


int ssi_fips_check_fips_tee_error(void)
{
	ssi_fips_state_t fips_state;

	if (ssi_fips_get_state(&fips_state) != 0) {
		FIPS_LOG("ssi_fips_get_state FAILED, returning...\n");
		return -ENOEXEC;
	}
	if (fips_state == CC_FIPS_STATE_ERROR) {
		FIPS_LOG("ssi_fips_get_state: fips_state is %d, returning...\n", fips_state);
		return -ENOEXEC;
	}

	if (ssi_fips_ext_get_tee_error() != CC_REE_FIPS_ERROR_OK) {
		ssi_fips_set_error_upon_tee_error();
		return -ENOEXEC;
	}

	return 0;
}

/* The function checks if FIPS supported and FIPS error exists.*
*  It should be used in every driver API.*/
int ssi_fips_check_fips_error(void)
{
	ssi_fips_state_t fips_state;

	if (ssi_fips_get_state(&fips_state) != 0) {
		FIPS_LOG("ssi_fips_get_state FAILED, returning.. \n");
		return -ENOEXEC;
	}
	if (fips_state == CC_FIPS_STATE_ERROR) {
		FIPS_LOG("ssi_fips_get_state: fips_state is %d, returning.. \n", fips_state);
		return -ENOEXEC;
	}

	return 0;
}


/* The function sets the REE FIPS state.*
*  It should be used while driver is being loaded .*/
int ssi_fips_set_state(ssi_fips_state_t state)
{
	return ssi_fips_ext_set_state(state);
}

/* The function sets the REE FIPS error, and pushes it to TEE library. *
*  It should be used when any of the KAT tests fails.*/
int ssi_fips_set_error(ssi_fips_error_t err)
{
	int rc = 0;
        ssi_fips_error_t current_err;

        FIPS_LOG("ssi_fips_set_error - fips_error = %d \n", err);

	/* setting no error is not allowed */
	if (err == CC_REE_FIPS_ERROR_OK) {
                return -ENOEXEC;
	}
	/* If error exists, do not set new error */
        if (ssi_fips_get_error(&current_err) != 0) {
                return -ENOEXEC;
        }
        if (current_err != CC_REE_FIPS_ERROR_OK) {
                return -ENOEXEC;
        }
	/* set REE internal error and state */
	rc = ssi_fips_ext_set_error(err);
	if (rc != 0) {
                return -ENOEXEC;
	}
	rc = ssi_fips_ext_set_state(CC_FIPS_STATE_ERROR);
	if (rc != 0) {
                return -ENOEXEC;
	}

	/* push status towards TEE libraray, if it's not TEE error */
	if (err != CC_REE_FIPS_ERROR_FROM_TEE) {
		ssi_fips_ext_update_tee_upon_ree_status();
	}
	return rc;
}

#ifdef TRUSTONIC_FIPS_SUPPORT
static int fips_tee_thread(void *data)
{
	ssi_fips_state_t state;
	ssi_fips_error_t error;

	ssi_fips_get_state(&state);
	ssi_fips_get_error(&error);
	FIPS_LOG("Begin fips_tee_thread (%u, %u)\n", state, error);

	error = ssi_fips_ext_get_tee_error();
	if (unlikely(error != CC_REE_FIPS_ERROR_OK))
		goto exit;

	ssi_fips_ext_set_error(test_error);
	ssi_fips_ext_update_tee_upon_ree_status();
	if (test_error == CC_REE_FIPS_ERROR_OK) {
		state = (ssi_fips_support == 0 ? CC_FIPS_STATE_NOT_SUPPORTED : CC_FIPS_STATE_SUPPORTED);
		ssi_fips_ext_set_state(state);
	}
exit:
	ssi_fips_get_state(&state);
	ssi_fips_get_error(&error);
	FIPS_LOG("End fips_tee_thread (%u, %u)\n", state, error);

	return 0;
}
#endif

/* The function called once at driver entry point .*/
int ssi_fips_init(struct ssi_drvdata *p_drvdata)
{
	ssi_fips_error_t rc = CC_REE_FIPS_ERROR_OK;

	FIPS_DBG("CC FIPS code ..  (fips=%d) \n", ssi_fips_support);

	/* init fips driver data */
	rc = ssi_fips_set_state((ssi_fips_support == 0)? CC_FIPS_STATE_NOT_SUPPORTED : CC_FIPS_STATE_SUPPORTED);
	if (unlikely(rc != 0)) {
		return -EAGAIN;
	}

	/* Run power up tests (before registration and operating the HW engines) */
#ifdef TRUSTONIC_FIPS_SUPPORT
	FIPS_DBG("cc_fips_run_power_up_tests\n");
	rc = cc_fips_run_power_up_tests(p_drvdata);
	FIPS_LOG("cc_fips_run_power_up_tests - done  ...  fips_error = %u\n", rc);

	test_error = rc;
	ssi_fips_set_error_upon_tee_error();
	tee_thread = kthread_run(fips_tee_thread, NULL, "fips_tee_thread");
	if (IS_ERR(tee_thread))
		SSI_LOG_ERR("%s, init kthread_run failed!\n", __func__);
#else
	FIPS_DBG("ssi_fips_ext_get_tee_error \n");
	rc = ssi_fips_ext_get_tee_error();
	if (unlikely(rc != CC_REE_FIPS_ERROR_OK)) {
		ssi_fips_set_error(CC_REE_FIPS_ERROR_FROM_TEE);
		return -EAGAIN;
	}

	FIPS_DBG("cc_fips_run_power_up_tests \n");
	rc = cc_fips_run_power_up_tests(p_drvdata);
	if (unlikely(rc != CC_REE_FIPS_ERROR_OK)) {
		ssi_fips_set_error(rc);
		return -EAGAIN;
	}
	FIPS_LOG("cc_fips_run_power_up_tests - done  ...  fips_error = %u\n", rc);

	/* when all tests passed, update TEE with fips OK status after power up tests */
	ssi_fips_ext_update_tee_upon_ree_status();
#endif
	if (unlikely(rc != CC_REE_FIPS_ERROR_OK))
		return -EAGAIN;

	return 0;
}

