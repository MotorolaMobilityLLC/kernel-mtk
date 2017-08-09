/*
* Copyright (c) 2015 MediaTek Inc.
* Author: Andrew-CT Chen <andrew-ct.chen@mediatek.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef MTK_VPU_CORE_H
#define MTK_VPU_CORE_H

#include <linux/platform_device.h>

typedef void (*ipi_handler_t) (void *data,
			       unsigned int len,
			       void *priv);

/**
 * enum ipi_id - the id of inter-processor interrupt
 *
 * @IPI_VPU_INIT:	The interrupt is to notfiy kernel
			VPU initialization completed.
 * @IPI_VDEC_H264:	The interrupt is to notify kernel to handle H264
			vidoe decoder job.
 * @IPI_VDEC_VP8:	The interrupt is to notify kernel to handle VP8
			video decoder job.
 * @IPI_VDEC_VP9:	The interrupt is to notify kernel to handle VP9
			video decoder job.
 * @IPI_VENC_H264:	The interrupt is to notify kernel to handle H264
			video encoder job.
 * @IPI_MAX:		The maximum IPI number
 */
enum ipi_id {
	IPI_VPU_INIT = 0,
	IPI_VDEC_H264,
	IPI_VDEC_VP8,
	IPI_VDEC_VP9,
	IPI_VDEC_MPEG4,
	IPI_VENC_H264,
	IPI_VENC_VP8,
	IPI_MDP,
	IPI_CLR_CONV,
	IPI_MAX,
};

/**
 * vpu_wdt_reg_handler - register a WDT handler
 *
 * @pdev:		VPU platform device
 * @VPU_WDT_FUNC_PTR:	the callback reset handler
 * @private_data:	the private data for reset function
 * @module_name:	handler name
 *
 * Register a handler performing own tasks when vpu reset by watchdog
 *
 * Return: Return 0 or positive value if the handler is added successfully,
 * otherwise it is failed.
 *
 **/
int vpu_wdt_reg_handler(
			struct platform_device *pdev,
			void VPU_WDT_FUNC_PTR(void *),
			void *private_data,
			char *module_name);

/**
 * vpu_wdt_unreg_handler - unregister a WDT handler
 *
 * @pdev:		VPU platform device
 * @module_name:	handler name
 *
 * Unregister a handler performing own tasks when vpu reset by watchdog
 *
 * Return: Return 0 or positive value if the handler is unregistered
 * successfully, otherwise it is failed.
 *
 **/
int vpu_wdt_unreg_handler(struct platform_device *pdev, char *module_name);

/**
 * vpu_disable_clock - Disable VPU clock
 *
 * @pdev:		VPU platform device
 *
 *
 * Return: Return 0 if the clock is enabled successfully,
 * otherwise it is failed.
 *
 **/
void vpu_disable_clock(struct platform_device *pdev);

/**
 * vpu_enable_clock - Enable VPU clock
 *
 * @pdev:		VPU platform device
 *
 * Return: Return 0 if the clock is disabled successfully,
 * otherwise it is failed.
 *
 **/
int vpu_enable_clock(struct platform_device *pdev);

/**
 * vpu_ipi_registration - register a ipi function
 *
 * @pdev:	VPU platform device
 * @id:		IPI ID
 * @handler:	IPI handler
 * @name:	IPI name
 * @priv:	private data for IPI handler
 *
 * Register a ipi function to receive ipi interrupt from VPU.
 *
 * Return: Return 0 if ipi registers successfully, otherwise it is failed.
 */
int vpu_ipi_registration(struct platform_device *pdev,
			 enum ipi_id id,
			 ipi_handler_t handler,
			 const char *name,
			 void *priv);

/**
 * vpu_ipi_send - send data from AP to vpu.
 *
 * @pdev:	VPU platform device
 * @id:		IPI ID
 * @buf:	the data buffer
 * @len:	the data buffer length
 * @wait:	wait for the last ipi completed.
 *
 * Return: Return 0 if sending data successfully, otherwise it is failed.
 **/
int vpu_ipi_send(struct platform_device *pdev,
		 enum ipi_id id, void *buf,
		 unsigned int len,
		 unsigned int wait);

/**
 * get_vpu_semaphore - get a hardware semaphore.
 *
 * @pdev:	VPU platform device
 * @flag:	the semaphore number.
 *
 * A hardware semaphore to avoid concurrent access between AP and VPU
 *
 * Return: Return 0 if getting hardware semaphore successfully,
 * otherwise it is failed.
 **/
int get_vpu_semaphore(struct platform_device *pdev, int flag);

/**
 * release_vpu_semaphore - release a hardware semaphore.
 *
 * @pdev:	VPU platform device
 * @flag:	the semaphore number.
 *
 * Return: Return 0 if getting hardware semaphore successfully,
 * otherwise it is failed.
 **/
int release_vpu_semaphore(struct platform_device *pdev, int flag);

/**
 * vpu_get_plat_device - get VPU's platform device
 *
 * @pdev:	the platform device of the module requesting VPU platform
 *		device for using VPU API.
 *
 * Return: Return NULL if it is failed.
 * otherwise it is VPU's platform device
 **/
struct platform_device *vpu_get_plat_device(struct platform_device *pdev);

/**
 * vpu_load_firmware - download vpu firmware and boot it
 *
 * @pdev:	VPU platform device
 * @force_load:	force to download vpu firmware and boot it
 *
 * Return: Return 0 if downloading firmware successfully,
 * otherwise it is failed
 **/
int vpu_load_firmware(struct platform_device *pdev, int force_load);

/**
 * vpu_mapping_dm_addr - Mapping DTCM/DMEM to kernel virtual address
 *
 * @pdev:	VPU platform device
 * @dmem_addr:	VPU's data memory address
 *
 * Mapping the VPU's DTCM (Data Tightly-Coupled Memory) /
 * DMEM (Data Extended Memory) memory address to
 * kernel virtual address.
 *
 * Return: Return ERR_PTR(-EINVAL) if mapping failed,
 * otherwise the mapped kernel virtual address
 **/
void *vpu_mapping_dm_addr(struct platform_device *pdev,
			  void *dtcm_dmem_addr);

/**
 * vpu_mapping_ext_mem_addr - Mapping to physical address
 *
 * @pdev:		VPU platform device
 * @virt_ext_mem_addr:	the VPU's kernel virtual address of
 *			extended data/program memory address
 *
 * Mapping VPU's kernel virtual address of
 * extended data/program memory address to physical address.
 *
 * return ERR_PTR(-EINVAL) if mapping failed,
 * otherwise the mapped physical address.
 **/
void *vpu_mapping_ext_mem_addr(struct platform_device *pdev,
			       void *virt_ext_mem_addr);

/**
 * vpu_mapping_iommu_dm_addr - Mapping to iommu address
 *
 * @pdev:	VPU platform device
 * @dmem_addr:	VPU's extended data memory address
 *
 * Mapping the VPU's extended data address to iommu address
 *
 * Return: Return ERR_PTR(-EINVAL) if mapping failed,
 * otherwise the mapped iommu address
 **/
dma_addr_t *vpu_mapping_iommu_dm_addr(struct platform_device *pdev,
				      void *dmem_addr);
#endif /* MTK_VPU_CORE_H */
