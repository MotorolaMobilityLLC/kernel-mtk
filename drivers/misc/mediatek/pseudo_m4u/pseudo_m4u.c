/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/iommu.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/dma-iommu.h>
#include <soc/mediatek/smi.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/memblock.h>
#include <asm/cacheflush.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <mach/pseudo_m4u.h>
#include <linux/pagemap.h>
#include <linux/compat.h>
#ifdef CONFIG_MACH_MT8167
#include <dt-bindings/memory/mt8167-larb-port.h>
#endif
#include "mtk_iommu.h"

#define M4U_LOG_LEVEL_HIGH	3
#define M4U_LOG_LEVEL_MID	2
#define M4U_LOG_LEVEL_LOW	1

int m4u_log_level;
int m4u_log_to_uart = 1;

#define _M4ULOG(level, string, args...) \
do { \
	if (level > m4u_log_level) { \
		if (level > m4u_log_to_uart) \
			pr_warn("PSEUDO M4U"string, ##args); \
	}  \
} while (0)

#define M4ULOG_LOW(string, args...)	_M4ULOG(M4U_LOG_LEVEL_LOW, string, ##args)
#define M4ULOG_MID(string, args...)	_M4ULOG(M4U_LOG_LEVEL_MID, string, ##args)
#define M4ULOG_HIGH(string, args...)	_M4ULOG(M4U_LOG_LEVEL_HIGH, string, ##args)
#define M4UERR(string, args...)	M4ULOG_HIGH(string, ##args)
#define M4UMSG(string, args...)	M4ULOG_MID(string, ##args)
#define M4UDBG(string, args...)	M4ULOG_LOW(string, ##args)
#define M4UINFO(string, args...)	M4ULOG_LOW(string, ##args)
#define M4ULOG(string, args...)	M4ULOG_MID(string, ##args)
#define M4U_ASSERT(x) {	\
	if (!(x))	\
		pr_err("M4U assert fail, file:%s, line:%d", __FILE__, __LINE__); \
}

#define M4UTRACE() \
do { \
	if (!m4u_log_to_uart) \
		pr_err("PSEUDO M4U %s, %d\n", __func__, __LINE__); \
} while (0)

LIST_HEAD(pseudo_sglist);
/* this is the mutex lock to protect mva_sglist->list*/
static DEFINE_MUTEX(pseudo_list_mutex);

static const struct of_device_id mtk_pseudo_of_ids[] = {
	{ .compatible = "mediatek,mt8167-pseudo-m4u",},
	{}
};

static const struct of_device_id mtk_pseudo_port_of_ids[] = {
	{ .compatible = "mediatek,mt8167-pseudo-port-m4u",},
	{}
};

int gM4U_L2_enable = 1;

/* garbage collect related */
#define MVA_REGION_FLAG_NONE 0x0
#define MVA_REGION_HAS_TLB_RANGE 0x1
#define MVA_REGION_REGISTER    0x2

struct m4u_device *gM4uDev;

static char *m4u_get_port_name(M4U_PORT_ID_ENUM portID)
{
	switch (portID) {
	/* larb 0 port */

	case M4U_PORT_DISP_OVL0:
		return "M4U_PORT_DISP_OVL0";
	case M4U_PORT_DISP_RDMA0:
		return "M4U_PORT_DISP_RDMA0";
	case M4U_PORT_DISP_WDMA0:
		return "M4U_PORT_DISP_WDMA0";
	case M4U_PORT_DISP_RDMA1:
		return "M4U_PORT_DISP_RDMA1";
	case M4U_PORT_MDP_RDMA:
		return "M4U_PORT_MDP_RDMA";
	case M4U_PORT_MDP_WDMA:
		return "M4U_PORT_MDP_WDMA";
	case M4U_PORT_MDP_WROT:
		return "M4U_PORT_MDP_WROT";
	case M4U_PORT_DISP_FAKE:
		return "M4U_PORT_DISP_FAKE";

	/* larb 1 port */
	case M4U_PORT_CAM_IMGO:
		return "M4U_PORT_CAM_IMGO";
	case M4U_PORT_CAM_IMG2O:
		return "M4U_PORT_CAM_IMG2O";
	case M4U_PORT_CAM_LSCI:
		return "M4U_PORT_CAM_LSCI";
	case M4U_PORT_CAM_ESFKO:
		return "M4U_PORT_CAM_ESFKO";
	case M4U_PORT_CAM_AAO:
		return "M4U_PORT_CAM_AAO";
	case M4U_PORT_VENC_REC:
		return "M4U_PORT_VENC_REC";
	case M4U_PORT_VENC_BSDMA:
		return "M4U_PORT_VENC_BSDMA";
	case M4U_PORT_VENC_RD_COMV:
		return "M4U_PORT_VENC_RD_COMV";
	case M4U_PORT_CAM_IMGI:
		return "M4U_PORT_CAM_IMGI";
	case M4U_PORT_VENC_CUR_LUMA:
		return "M4U_PORT_VENC_CUR_LUMA";
	case M4U_PORT_VENC_CUR_CHROMA:
		return "M4U_PORT_VENC_CUR_CHROMA";
	case M4U_PORT_VENC_REF_LUMA:
		return "M4U_PORT_VENC_REF_LUMA";
	case M4U_PORT_VENC_REF_CHROMA:
		return "M4U_PORT_VENC_REF_CHROMA";

	/* larb 2 */
	case M4U_PORT_HW_VDEC_MC_EXT:
		return "M4U_PORT_HW_VDEC_MC_EXT";
	case M4U_PORT_HW_VDEC_PP_EXT:
		return "M4U_PORT_HW_VDEC_PP_EXT";
	case M4U_PORT_HW_VDEC_VLD_EXT:
		return "M4U_PORT_HW_VDEC_VLD_EXT";
	case M4U_PORT_HW_VDEC_AVC_MV_EXT:
		return "M4U_PORT_HW_VDEC_AVC_MV_EXT";
	case M4U_PORT_HW_VDEC_PRED_RD_EXT:
		return "M4U_PORT_HW_VDEC_PRED_RD_EXT";
	case M4U_PORT_HW_VDEC_PRED_WR_EXT:
		return "M4U_PORT_HW_VDEC_PRED_WR_EXT";
	case M4U_PORT_HW_VDEC_PPWRAP_EXT:
		return "M4U_PORT_HW_VDEC_PPWRAP_EXT";

	default:
		M4UMSG("invalid module id=%d.\n", portID);
		return "UNKNOWN";

	}
}

/*
 * userspace port defines are following the traditional defines with flat port enum
 * define, but kernel port have upstreamed defines different with userspace port define.
 */
static int m4u_port_inlarb[3] = {0, 8, 21};
#define MT8167_SMI_LARN_NR	3
static inline int m4u_user2kernel_port(int userport)
{
	return userport;
}

static inline int m4u_kernel2user_port(int kernelport)
{
	return kernelport;
}

static char *m4u_get_module_name(M4U_PORT_ID_ENUM portID)
{
	return m4u_get_port_name(portID);
}

static int MTK_M4U_flush(struct file *a_pstFile, fl_owner_t a_id)
{
	return 0;
}

m4u_client_t *m4u_create_client(void)
{
	m4u_client_t *client;

	client = kmalloc(sizeof(m4u_client_t), GFP_ATOMIC);
	if (!client)
		return NULL;

	mutex_init(&(client->dataMutex));
	mutex_lock(&(client->dataMutex));
	client->open_pid = current->pid;
	client->open_tgid = current->tgid;
	INIT_LIST_HEAD(&(client->mvaList));
	mutex_unlock(&(client->dataMutex));

	return client;
}
EXPORT_SYMBOL(m4u_create_client);

int m4u_destroy_client(m4u_client_t *client)
{
	m4u_buf_info_t *pMvaInfo;
	unsigned int mva, size;
	M4U_PORT_ID port;

	while (1) {
		mutex_lock(&(client->dataMutex));
		if (list_empty(&client->mvaList)) {
			mutex_unlock(&(client->dataMutex));
			break;
		}
		pMvaInfo = container_of(client->mvaList.next, m4u_buf_info_t, link);
		M4UINFO
		    ("warnning: clean garbage at m4u close: module=%s,va=0x%lx,mva=0x%x,size=%d\n",
		     m4u_get_port_name(pMvaInfo->port), pMvaInfo->va, pMvaInfo->mva,
		     pMvaInfo->size);

		port = pMvaInfo->port;
		mva = pMvaInfo->mva;
		size = pMvaInfo->size;

		mutex_unlock(&(client->dataMutex));

		/* m4u_dealloc_mva will lock client->dataMutex again */
		pseudo_dealloc_mva(client, port, mva);
	}

	kfree(client);

	return 0;
}
EXPORT_SYMBOL(m4u_destroy_client);

static int MTK_M4U_open(struct inode *inode, struct file *file)
{
	m4u_client_t *client;

	M4UDBG("enter MTK_M4U_open() process : %s\n", current->comm);
	client = m4u_create_client();
	if (IS_ERR_OR_NULL(client)) {
		M4UMSG("createclientfail\n");
		return -ENOMEM;
	}

	file->private_data = client;

	return 0;
}

static int MTK_M4U_release(struct inode *inode, struct file *file)
{
	m4u_client_t *client = file->private_data;

	M4UDBG("enter MTK_M4U_release() process : %s\n", current->comm);
	m4u_destroy_client(client);
	return 0;
}

/*
 * for mt8127 implement, the tee service call just implement the config port feature.
 * there's no caller of m4u_alloc_mva_sec, both in normal world nor in security world.
 * since the security world did not reserve the mva range in normal world.
 * That's to say, we just need to handle config port, and do not need to handle alloc
 * mva sec dealloc mva sec etc.
 */
#ifdef M4U_TEE_SERVICE_ENABLE

#include "tz_cross/trustzone.h"
#include "trustzone/kree/system.h"
#include "tz_cross/ta_m4u.h"

KREE_SESSION_HANDLE m4u_session;
bool m4u_tee_en;

static DEFINE_MUTEX(gM4u_port_tee);
static int pseudo_m4u_session_init(void)
{
	TZ_RESULT ret;

	ret = KREE_CreateSession(TZ_TA_M4U_UUID, &m4u_session);
	if (ret != TZ_RESULT_SUCCESS) {
		M4UMSG("m4u CreateSession error %d\n", ret);
		return -1;
	}
	M4UMSG("create session : 0x%x\n", (unsigned int)m4u_session);
	m4u_tee_en = true;
	return 0;
}

int m4u_larb_restore_sec(unsigned int larb_idx)
{
	MTEEC_PARAM param[4];
	uint32_t paramTypes;
	TZ_RESULT ret;

	if (!m4u_tee_en)  /*tee may not init*/
		return -2;

	if (larb_idx == 0 || larb_idx == 4) { /*only support disp*/
		param[0].value.a = larb_idx;
		paramTypes = TZ_ParamTypes1(TZPT_VALUE_INPUT);

		ret = KREE_TeeServiceCall(m4u_session,
			M4U_TZCMD_LARB_REG_RESTORE,
			paramTypes, param);
		if (ret != TZ_RESULT_SUCCESS) {
			M4UMSG("m4u reg backup ServiceCall error %d\n", ret);
			return -1;
		}
	}
	return 0;
}

int m4u_larb_backup_sec(unsigned int larb_idx)
{
	MTEEC_PARAM param[4];
	uint32_t paramTypes;
	TZ_RESULT ret;

	if (!m4u_tee_en)  /*tee may not init */
		return -2;

	if (larb_idx == 0 || larb_idx == 4) { /*only support disp*/
		param[0].value.a = larb_idx;
		paramTypes = TZ_ParamTypes1(TZPT_VALUE_INPUT);

		ret = KREE_TeeServiceCall(m4u_session,
					M4U_TZCMD_LARB_REG_BACKUP,
					paramTypes, param);
		if (ret != TZ_RESULT_SUCCESS) {
			M4UMSG("m4u reg backup ServiceCall error %d\n", ret);
			return -1;
		}
	}
	return 0;
}

int smi_reg_backup_sec(void)
{
	uint32_t paramTypes;
	TZ_RESULT ret;

	paramTypes = TZ_ParamTypes1(TZPT_NONE);
	ret = KREE_TeeServiceCall(m4u_session, M4U_TZCMD_REG_BACKUP,
				paramTypes, NULL);
	if (ret != TZ_RESULT_SUCCESS) {
		M4UMSG("m4u reg backup ServiceCall error %d\n", ret);
		return -1;
	}
	return 0;
}

int smi_reg_restore_sec(void)
{
	uint32_t paramTypes;
	TZ_RESULT ret;

	paramTypes = TZ_ParamTypes1(TZPT_NONE);
	ret = KREE_TeeServiceCall(m4u_session, M4U_TZCMD_REG_RESTORE,
				paramTypes, NULL);
	if (ret != TZ_RESULT_SUCCESS) {
		M4UMSG("m4u reg backup ServiceCall error %d\n", ret);
		return -1;
	}

	return 0;
}

int pseudo_m4u_do_config_port(M4U_PORT_STRUCT *pM4uPort)
{
	MTEEC_PARAM param[4];
	uint32_t paramTypes;
	TZ_RESULT ret;

	/* do not config port if session has not been inited. */
	if (!m4u_session)
		return 0;

	param[0].value.a = pM4uPort->ePortID;
	param[0].value.b = pM4uPort->Virtuality;
	param[1].value.a = pM4uPort->Distance;
	param[1].value.b = pM4uPort->Direction;

	paramTypes = TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);

	mutex_lock(&gM4u_port_tee);
	ret = KREE_TeeServiceCall(m4u_session, M4U_TZCMD_CONFIG_PORT, paramTypes, param);
	mutex_unlock(&gM4u_port_tee);

	if (ret != TZ_RESULT_SUCCESS)
		M4UMSG("m4u_config_port ServiceCall error 0x%x\n", ret);

	return 0;
}

static int pseudo_m4u_sec_init(unsigned int u4NonSecPa,
			unsigned int L2_enable, unsigned int *security_mem_size)
{
	MTEEC_PARAM param[4];
	uint32_t paramTypes;
	TZ_RESULT ret;

	param[0].value.a = u4NonSecPa;
	param[0].value.b = L2_enable;
	param[1].value.a = 1;
	paramTypes = TZ_ParamTypes2(TZPT_VALUE_INPUT,
					TZPT_VALUE_OUTPUT);
	ret = KREE_TeeServiceCall(m4u_session, M4U_TZCMD_SEC_INIT,
			paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS) {
		M4UMSG("m4u sec init error 0x%x\n", ret);
		return -1;
	}

	*security_mem_size = param[1].value.a;
	return 0;
}

/* the caller should enable smi clock, it should be only called by mtk_smi.c */
int pseudo_config_port_tee(int kernelport)
{
	M4U_PORT_STRUCT pM4uPort;

	pM4uPort.ePortID = m4u_kernel2user_port(kernelport);
	pM4uPort.Virtuality = 1;
	pM4uPort.Distance = 1;
	pM4uPort.Direction = 1;

	return pseudo_m4u_do_config_port(&pM4uPort);
}

#endif

int m4u_dma_cache_flush_all(void)
{
	/* L1 cache clean before hw read */
	smp_inner_dcache_flush_all();

	/* L2 cache maintenance by physical pages */
	outer_flush_all();

	return 0;
}

static struct pseudo_device larbdev[3];
static inline int m4u_get_larbid(int kernel_port)
{
	int i;

	for (i = MT8167_SMI_LARN_NR - 1; i >= 0; i--)
		if (kernel_port >= m4u_port_inlarb[i])
			return i;

	return 0;
}

struct device *m4u_get_larbdev(int portid)
{
	int larbid;
	struct pseudo_device *pseudo;

	larbid = m4u_get_larbid(portid);
	pseudo = &larbdev[larbid];

	if (pseudo && pseudo->dev)
		return pseudo->dev;

	M4UMSG("%s, %d, could not get larbdev\n", __func__, __LINE__);
	return NULL;
}

static inline int pseudo_config_port(M4U_PORT_STRUCT *pM4uPort)
{
	/* all the port will be attached by dma and configed by iommu driver */
	return 0;
}

int m4u_config_port(M4U_PORT_STRUCT *pM4uPort)
{
	int i, ret;

	for (i = 0; i < MT8167_SMI_LARN_NR; i++)
		mtk_smi_larb_clock_on(i, true);
#ifdef M4U_TEE_SERVICE_ENABLE
	ret = pseudo_m4u_do_config_port(pM4uPort);
#else
	ret = pseudo_config_port(pM4uPort);
#endif

	for (i = MT8167_SMI_LARN_NR - 1; i >= 0; i--)
		mtk_smi_larb_clock_off(i, true);

	return ret;
}

int m4u_config_port_array(struct m4u_port_array *port_array)
{
	int port, i, kernelport;
	int ret = 0;

	for (i = 0; i < MT8167_SMI_LARN_NR; i++)
		mtk_smi_larb_clock_on(i, true);

	for (port = 0; port < M4U_PORT_NR; port++) {
		if (port_array->ports[port] & M4U_PORT_ATTR_EN) {

			kernelport = m4u_user2kernel_port(port);
#ifdef M4U_TEE_SERVICE_ENABLE
			ret = pseudo_config_port_tee(kernelport);
#endif
/* In normal world, when get the smi larb, the port would be auto configured.*/
		}
	}

	for (i = MT8167_SMI_LARN_NR - 1; i >= 0; i--)
		mtk_smi_larb_clock_off(i, true);

	return ret;
}

mva_info_t *m4u_alloc_garbage_list(unsigned int mvaStart,
				   unsigned int bufSize,
				   M4U_MODULE_ID_ENUM eModuleID,
				   unsigned long va,
				   unsigned int flags, int security, int cache_coherent)
{
	mva_info_t *pList = NULL;

	pList = kmalloc(sizeof(mva_info_t), GFP_KERNEL);
	if (pList == NULL) {
		M4UERR("m4u_alloc_garbage_list(), pList = NULL\n");
		return NULL;
	}

	pList->mvaStart = mvaStart;
	pList->size = bufSize;
	pList->eModuleId = eModuleID;
	pList->bufAddr = va;
	pList->flags = flags;
	pList->security = security;
	pList->cache_coherent = cache_coherent;
	return pList;
}

/* static struct iova_domain *giovad; */

/* since the device have been attached, then we get from the dma_ops->map_sg is arm_iommu_map_sg */
static int __m4u_alloc_mva(M4U_PORT_ID port, unsigned long va, unsigned int size,
				struct sg_table *sg_table, unsigned int *retmva)
{
	struct mva_sglist *mva_sg;
	struct sg_table *table = NULL;
	int ret, kernelport = m4u_user2kernel_port(port);
	struct device *dev = m4u_get_larbdev(kernelport);

	dma_addr_t dma_addr;

	if (!va && !sg_table) {
		M4UMSG("va and sg_table are all NULL\n");
		return -EINVAL;
	}

	/* this is for ion mm heap and ion fb heap usage. */
	if (sg_table) {
		struct scatterlist *s = sg_table->sgl, *ng;
		phys_addr_t phys;
		int i;

		table = kzalloc(sizeof(*table), GFP_KERNEL);
		ret = sg_alloc_table(table, sg_table->nents, GFP_KERNEL);
		if (ret) {
			kfree(table);
			*retmva = 0;
			return ret;
		}

		ng = table->sgl;

		for (i = 0; i < sg_table->nents; i++) {
			phys = sg_phys(s);
			size += s->length;
			sg_set_page(ng, sg_page(s), s->length, 0);
			s = sg_next(s);
			ng = sg_next(ng);
		}
	}

	if (!table) {
		table = pseudo_get_sg(port, va, size);
		if (!table) {
			M4UMSG("pseudo_get_sg failed\n");
			goto err;
		}
	}

	if (sg_table) {
		iommu_dma_map_sg(dev, table->sgl, table->nents, IOMMU_READ | IOMMU_WRITE);
		dma_addr = sg_dma_address(table->sgl);
	} else {
		iommu_dma_map_sg(dev, table->sgl, table->orig_nents, IOMMU_READ | IOMMU_WRITE);
		dma_addr = sg_dma_address(table->sgl);
	}

	if (dma_addr == DMA_ERROR_CODE) {
		M4UERR("%s, %d alloc mva failed, port is %s, dma_address is 0x%lx, size is 0x%x\n",
			__func__, __LINE__, m4u_get_port_name(port), (unsigned long)dma_addr, size);
		M4UERR("SUSPECT that iova have been all exhaust, maybe there's someone hold too much mva\n");
		goto err;
	}

	*retmva = dma_addr;

	mva_sg = kzalloc(sizeof(*mva_sg), GFP_KERNEL);
	mva_sg->table = table;
	mva_sg->mva = *retmva;

	m4u_add_sgtable(mva_sg);

	M4UDBG("%s, %d mva is 0x%x, dma_address is 0x%lx, size is 0x%x\n",
		__func__, __LINE__, mva_sg->mva, (unsigned long)dma_addr, size);
	return 0;
#if 0
err_free_iova:
	if (iova)
		__free_iova(iovad, iova);
	M4UMSG("iommu_map_sg failed\n");
#endif
err:
	if (table) {
		sg_free_table(table);
		kfree(table);
	}
	*retmva = 0;
	return -EINVAL;
}

static m4u_buf_info_t *m4u_alloc_buf_info(void)
{
	m4u_buf_info_t *pList = NULL;

	pList = kzalloc(sizeof(m4u_buf_info_t), GFP_KERNEL);
	if (pList == NULL) {
		M4UMSG("m4u_client_add_buf(), pList=0x%p\n", pList);
		return NULL;
	}
	M4UDBG("pList size %d, ptr %p\n", (int)sizeof(m4u_buf_info_t), pList);
	INIT_LIST_HEAD(&(pList->link));
	return pList;
}

static int m4u_free_buf_info(m4u_buf_info_t *pList)
{
	kfree(pList);
	return 0;
}

static int m4u_client_add_buf(m4u_client_t *client, m4u_buf_info_t *pList)
{
	mutex_lock(&(client->dataMutex));
	list_add(&(pList->link), &(client->mvaList));
	mutex_unlock(&(client->dataMutex));

	return 0;
}

/***********************************************************/
/** find or delete a buffer from client list
* @param   client   -- client to be searched
* @param   mva      -- mva to be searched
* @param   del      -- should we del this buffer from client?
*
* @return buffer_info if found, NULL on fail
* @remark
* @see
* @to-do    we need to add multi domain support here.
* @author K Zhang      @date 2013/11/14
************************************************************/
static m4u_buf_info_t *m4u_client_find_buf(m4u_client_t *client, unsigned int mva, int del)
{
	struct list_head *pListHead;
	m4u_buf_info_t *pList = NULL;
	m4u_buf_info_t *ret = NULL;

	if (client == NULL) {
		M4UERR("m4u_delete_from_garbage_list(), client is NULL!\n");
		return NULL;
	}

	mutex_lock(&(client->dataMutex));
	list_for_each(pListHead, &(client->mvaList)) {
		pList = container_of(pListHead, m4u_buf_info_t, link);
		if (pList->mva == mva)
			break;
	}
	if (pListHead == &(client->mvaList)) {
		ret = NULL;
	} else {
		if (del)
			list_del(pListHead);
		ret = pList;
	}


	mutex_unlock(&(client->dataMutex));

	return ret;
}

/* interface for ion */
static m4u_client_t *ion_m4u_client;

int m4u_alloc_mva_sg(port_mva_info_t *port_info,
				struct sg_table *sg_table)
{
	if (!ion_m4u_client) {
		ion_m4u_client = m4u_create_client();
		if (IS_ERR_OR_NULL(ion_m4u_client)) {
			ion_m4u_client = NULL;
			return -1;
		}
	}

	return pseudo_alloc_mva(ion_m4u_client, port_info->eModuleID, 0, sg_table, port_info->BufSize, 0,
				port_info->flags, &port_info->mva);
}


int pseudo_alloc_mva(m4u_client_t *client, M4U_PORT_ID port,
			  unsigned long va, struct sg_table *sg_table,
			  unsigned int size, unsigned int prot, unsigned int flags, unsigned int *pMva)
{
	int ret, offset;
	m4u_buf_info_t *pbuf_info;
	unsigned int mva = 0;
	unsigned long va_align = va;
	unsigned int mva_align, size_align = size;

	/* align the va to allocate continues iova. */
	offset = m4u_va_align(&va_align, &size_align);
	/* pbuf_info for userspace compatible */
	pbuf_info = m4u_alloc_buf_info();

	pbuf_info->va = va;
	pbuf_info->port = port;
	pbuf_info->size = size;
	pbuf_info->prot = prot;
	pbuf_info->flags = flags;
	pbuf_info->sg_table = sg_table;

	ret = __m4u_alloc_mva(port, va_align, size_align, sg_table, &mva_align);
	if (ret) {
		M4UMSG("error alloc mva, %s, %d\n", __func__, __LINE__);
		mva = 0;
		goto err;
	}

	mva = mva_align + offset;
	pbuf_info->mva = mva;
	*pMva = mva;

	m4u_client_add_buf(client, pbuf_info);

	return 0;
err:
	m4u_free_buf_info(pbuf_info);
	return ret;
}

int pseudo_dealloc_mva(m4u_client_t *client, M4U_PORT_ID port, unsigned int mva)
{
	m4u_buf_info_t *pMvaInfo;
	int offset, ret;

	pMvaInfo = m4u_client_find_buf(client, mva, 1);

	offset = m4u_va_align(&pMvaInfo->va, &pMvaInfo->size);
	pMvaInfo->mva -= offset;

	ret = __m4u_dealloc_mva(port, pMvaInfo->va, pMvaInfo->size, mva, NULL);
	if (ret)
		return ret;

	m4u_free_buf_info(pMvaInfo);
	return ret;

}

#if 0
int m4u_alloc_mva_sg(M4U_MODULE_ID_ENUM eModuleID,
		     struct sg_table *sg_table,
		     const unsigned int BufSize,
		     int security, int cache_coherent, unsigned int *pRetMVABuf)
{
	mva_info_t *pMvaInfo = NULL;
	int ret;

	pMvaInfo = m4u_alloc_garbage_list(0, BufSize,
				eModuleID, 0, 0, security, cache_coherent);
	ret = __m4u_alloc_mva(pMvaInfo, sg_table);

	if (ret == 0)
		*pRetMVABuf = pMvaInfo->mvaStart;
	else
		*pRetMVABuf = 0;

	return ret;

}
#endif

int m4u_dealloc_mva_sg(M4U_MODULE_ID_ENUM eModuleID,
		       struct sg_table *sg_table,
		       const unsigned int BufSize, const unsigned int MVA)
{
	if (!sg_table) {
		M4UMSG("%s, %d, sg_table is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	return __m4u_dealloc_mva(eModuleID, 0, BufSize, MVA, sg_table);
}

struct sg_table *m4u_find_sgtable(unsigned int mva)
{
	struct mva_sglist *entry;

	mutex_lock(&pseudo_list_mutex);

	list_for_each_entry(entry, &pseudo_sglist, list) {
		if (entry->mva == mva) {
			mutex_unlock(&pseudo_list_mutex);
			return entry->table;
		}
	}

	mutex_unlock(&pseudo_list_mutex);
	return NULL;
}

struct sg_table *m4u_del_sgtable(unsigned int mva)
{
	struct mva_sglist *entry, *tmp;
	struct sg_table *table;

	M4UDBG("%s, %d, mva = 0x%x\n", __func__, __LINE__, mva);
	mutex_lock(&pseudo_list_mutex);
	list_for_each_entry_safe(entry, tmp, &pseudo_sglist, list) {
		M4UDBG("%s, %d, entry->mva = 0x%x\n", __func__, __LINE__, entry->mva);
		if (entry->mva == mva) {
			list_del(&entry->list);
			mutex_unlock(&pseudo_list_mutex);
			table = entry->table;
			M4UDBG("%s, %d, mva is 0x%x, entry->mva is 0x%x\n",
				__func__, __LINE__, mva, entry->mva);
			kfree(entry);
			return table;
		}
	}
	mutex_unlock(&pseudo_list_mutex);

	return NULL;
}

struct sg_table *m4u_add_sgtable(struct mva_sglist *mva_sg)
{
	struct sg_table *table;

	table = m4u_find_sgtable(mva_sg->mva);
	if (table)
		return table;

	table = mva_sg->table;
	mutex_lock(&pseudo_list_mutex);
	list_add(&mva_sg->list, &pseudo_sglist);
	mutex_unlock(&pseudo_list_mutex);

	M4UDBG("adding pseudo_sglist, mva = 0x%x\n", mva_sg->mva);
	return table;
}

/* make sure the va size is page aligned to get the continues iova. */
int m4u_va_align(unsigned long *addr, unsigned int *size)
{
	int offset, remain;

	/* we need to align the bufaddr to make sure the iova is continues */
	offset = *addr & (M4U_PAGE_SIZE - 1);
	if (offset) {
		*addr &= ~(M4U_PAGE_SIZE - 1);
		*size += offset;
	}

	/* make sure we alloc one page size iova at least */
	remain = *size % M4U_PAGE_SIZE;
	if (remain)
		*size += M4U_PAGE_SIZE - remain;
	/* dma32 would skip the last page, we added it here */
	/* *size += PAGE_SIZE; */
	return offset;
}

#if 0
int m4u_alloc_mva(M4U_MODULE_ID_ENUM eModuleID,
		  unsigned long BufAddr,
		  unsigned int BufSize,
		  int security, int cache_coherent, unsigned int *pRetMVABuf)
{
	mva_info_t *pMvaInfo = NULL;
	int offset, ret;

	/* we need to align the bufaddr to make sure the iova is continues */
	offset = m4u_va_align(&BufAddr, &BufSize);

	pMvaInfo = m4u_alloc_garbage_list(0, BufSize,
				eModuleID, BufAddr, 0,
				security, cache_coherent);
	ret = __m4u_alloc_mva(pMvaInfo, NULL);

	if (ret == 0)
		*pRetMVABuf = pMvaInfo->mvaStart + offset;
	else
		*pRetMVABuf = 0;

	return ret;

}
#endif

/* to-do: need modification to support 4G DRAM */
static phys_addr_t m4u_user_v2p(unsigned long va)
{
	unsigned long pageOffset = (va & (M4U_PAGE_SIZE - 1));
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	phys_addr_t pa;

	if (!current) {
		M4UMSG("warning: m4u_user_v2p, current is NULL!\n");
		return 0;
	}
	if (!current->mm) {
		M4UMSG("warning: m4u_user_v2p, current->mm is NULL! tgid=0x%x, name=%s\n",
		       current->tgid, current->comm);
		return 0;
	}

	pgd = pgd_offset(current->mm, va);	/* what is tsk->mm */
	if (pgd_none(*pgd) || pgd_bad(*pgd)) {
		M4UDBG("m4u_user_v2p(), va=0x%lx, pgd invalid!\n", va);
		return 0;
	}

	pud = pud_offset(pgd, va);
	if (pud_none(*pud) || pud_bad(*pud)) {
		M4UDBG("m4u_user_v2p(), va=0x%lx, pud invalid!\n", va);
		return 0;
	}

	pmd = pmd_offset(pud, va);
	if (pmd_none(*pmd) || pmd_bad(*pmd)) {
		M4UMSG("m4u_user_v2p(), va=0x%lx, pmd invalid!\n", va);
		return 0;
	}

	pte = pte_offset_map(pmd, va);
	if (pte_present(*pte)) {
#if 0				/* cloud, workaround */
		if ((long long)pte_val(pte[PTE_HWTABLE_PTRS]) == (long long)0) {
			M4UMSG("user_v2p, va=0x%x, *ppte=%08llx", va,
			       (long long)pte_val(pte[PTE_HWTABLE_PTRS]));
			pte_unmap(pte);
			return 0;
		}
#endif
		pa = (pte_val(*pte) & (PHYS_MASK) & (PAGE_MASK)) | pageOffset;
		pte_unmap(pte);
		return pa;
	}

	pte_unmap(pte);

	M4UDBG("m4u_user_v2p(), va=0x%lx, pte invalid!\n", va);
	return 0;
}

static int m4u_put_unlock_page(struct page *page)
{
	if (!page)
		return 0;

	if (!PageReserved(page))
		SetPageDirty(page);
	page_cache_release(page);

	return 0;

}

/* put ref count on all pages in sgtable */
int m4u_put_sgtable_pages(struct sg_table *table, int nents)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(table->sgl, sg, nents, i) {
		struct page *page = sg_page(sg);

		if (IS_ERR(page))
			return 0;
		if (page) {
			if (!PageReserved(page))
				SetPageDirty(page);
			page_cache_release(page);
		}
	}
	return 0;
}

int __m4u_get_user_pages(int eModuleID, struct task_struct *tsk, struct mm_struct *mm,
			 unsigned long start, int nr_pages, unsigned int gup_flags,
			 struct page **pages, struct vm_area_struct *vmas)
{
	int i, ret;
	unsigned long vm_flags;

	if (nr_pages <= 0)
		return 0;

	/* VM_BUG_ON(!!pages != !!(gup_flags & FOLL_GET)); */
	if (!!pages != !!(gup_flags & FOLL_GET)) {
		M4UMSG("error: __m4u_get_user_pages !!pages != !!(gup_flags & FOLL_GET),");
		M4UMSG("gup_flags & FOLL_GET=0x%x\n", gup_flags & FOLL_GET);
	}

	/*
	 * Require read or write permissions.
	 * If FOLL_FORCE is set, we only require the "MAY" flags.
	 */
	vm_flags = (gup_flags & FOLL_WRITE) ? (VM_WRITE | VM_MAYWRITE) : (VM_READ | VM_MAYREAD);
	vm_flags &= (gup_flags & FOLL_FORCE) ? (VM_MAYREAD | VM_MAYWRITE) : (VM_READ | VM_WRITE);
	i = 0;

	M4UDBG("Trying to get_user_pages from start vaddr 0x%lx with %d pages\n", start, nr_pages);

	do {
		struct vm_area_struct *vma;

		M4UDBG("For a new vma area from 0x%lx\n", start);
		if (vmas)
			vma = vmas;
		else
			vma = find_extend_vma(mm, start);

		if (!vma) {
			M4UMSG("error: the vma is not found, start=0x%x, module=%d\n",
			       (unsigned int)start, eModuleID);
			return i ? i : -EFAULT;
		}
		if (((~vma->vm_flags) & (VM_IO | VM_PFNMAP | VM_SHARED | VM_WRITE)) == 0) {
			M4UMSG("error: m4u_get_pages(): bypass pmem garbage pages!");
			M4UMSG("vma->vm_flags=0x%x, start=0x%lx, module=%d\n",
				(unsigned int)(vma->vm_flags), start, eModuleID);
			return i ? i : -EFAULT;
		}
		if (vma->vm_flags & VM_IO)
			M4UDBG("warning: vma is marked as VM_IO\n");

		if (vma->vm_flags & VM_PFNMAP) {
			M4UMSG
			    ("error: vma permission is not correct, vma->vm_flags=0x%x, start=0x%lx, module=%d\n",
			     (unsigned int)(vma->vm_flags), start, eModuleID);
			M4UMSG
			    ("hint: maybe the memory is remapped with un-permitted vma->vm_flags!\n");
			/* m4u_dump_maps(start); */
			return i ? i : -EFAULT;
		}
		if (!(vm_flags & vma->vm_flags)) {
			M4UMSG
			    ("error: vm_flags invalid, vm_flags=0x%x, vma->vm_flags=0x%x, start=0x%lx, module=%d\n",
			     (unsigned int)vm_flags, (unsigned int)(vma->vm_flags),
			     start, eModuleID);
			/* m4u_dump_maps(start); */
			return i ? : -EFAULT;
		}

		do {
			struct page *page;
			unsigned int foll_flags = gup_flags;
			/*
			 * If we have a pending SIGKILL, don't keep faulting
			 * pages and potentially allocating memory.
			 */
			if (unlikely(fatal_signal_pending(current)))
				return i ? i : -ERESTARTSYS;

			ret = get_user_pages_durable(current, current->mm, start, 1,
				(vma->vm_flags & VM_WRITE), 0, &page, NULL);
			if (ret == 1)
				pages[i] = page;

			while (!page) {
				int ret;

				ret =
				    handle_mm_fault(mm, vma, start,
						    (foll_flags & FOLL_WRITE) ? FAULT_FLAG_WRITE : 0);

				if (ret & VM_FAULT_ERROR) {
					if (ret & VM_FAULT_OOM) {
						M4UMSG("handle_mm_fault() error: no memory,");
						M4UMSG(" addr:0x%lx (%d pages are allocated), module=%d\n",
							start, i, eModuleID);
						/* m4u_dump_maps(start); */
						return i ? i : -ENOMEM;
					}
					if (ret & (VM_FAULT_HWPOISON | VM_FAULT_SIGBUS)) {
						M4UMSG("handle_mm_fault() error: invalid memory address,");
						M4UMSG(" vaddr:0x%lx (%d pages are allocated), module=%d\n",
							start, i, eModuleID);
						/* m4u_dump_maps(start); */
						return i ? i : -EFAULT;
					}
				}
				if (ret & VM_FAULT_MAJOR)
					tsk->maj_flt++;
				else
					tsk->min_flt++;

				/*
				 * The VM_FAULT_WRITE bit tells us that
				 * do_wp_page has broken COW when necessary,
				 * even if maybe_mkwrite decided not to set
				 * pte_write. We can thus safely do subsequent
				 * page lookups as if they were reads. But only
				 * do so when looping for pte_write is futile:
				 * in some cases userspace may also be wanting
				 * to write to the gotten user page, which a
				 * read fault here might prevent (a readonly
				 * page might get reCOWed by userspace write).
				 */
				if ((ret & VM_FAULT_WRITE) && !(vma->vm_flags & VM_WRITE))
					foll_flags &= ~FOLL_WRITE;

				ret = get_user_pages_durable(current, current->mm, start, 1,
					(vma->vm_flags & VM_WRITE), 0, &page, NULL);
				if (ret == 1)
					pages[i] = page;
			}
			if (IS_ERR(page)) {
				M4UMSG("handle_mm_fault() error: faulty page is returned,");
				M4UMSG("vaddr:0x%lx (%d pages are allocated), module=%d\n", start, i, eModuleID);
				/* m4u_dump_maps(start); */
				return i ? i : PTR_ERR(page);
			}

			i++;
			start += M4U_PAGE_SIZE;
			nr_pages--;
		} while (nr_pages && start < vma->vm_end);
	} while (nr_pages);

	return i;
}

/* refer to mm/memory.c:get_user_pages() */
int m4u_get_user_pages(int eModuleID, struct task_struct *tsk, struct mm_struct *mm,
		       unsigned long start, int nr_pages, int write, int force,
		       struct page **pages, struct vm_area_struct *vmas)
{
	int flags = FOLL_TOUCH;

	if (pages)
		flags |= FOLL_GET;
	if (write)
		flags |= FOLL_WRITE;
	if (force)
		flags |= FOLL_FORCE;

	return __m4u_get_user_pages(eModuleID, tsk, mm, start, nr_pages, flags, pages, vmas);
}


/* /> m4u driver internal use function */
/* /> should not be called outside m4u kernel driver */
static int m4u_get_pages(M4U_MODULE_ID_ENUM eModuleID, unsigned long BufAddr,
			 unsigned long BufSize, unsigned long *pPhys)
{
	int ret, i;
	int page_num;
	unsigned long start_pa;
	unsigned int write_mode = 0;
	struct vm_area_struct *vma = NULL;


	M4UDBG("^ m4u_get_pages: module=%s, BufAddr=0x%lx, BufSize=%ld\n",
	       m4u_get_module_name(eModuleID), BufAddr, BufSize);

	/* caculate page number */
	page_num = (BufSize + (BufAddr & 0xfff)) / M4U_PAGE_SIZE;
	if ((BufAddr + BufSize) & 0xfff)
		page_num++;

	if (BufSize > 200*1024*1024) {
		M4UMSG("m4u_get_pages(), single time alloc size=0x%lx, bigger than limit=0x%x\n",
		     BufSize, 200*1024*1024);
		return -EFAULT;
	}

	if (BufAddr < PAGE_OFFSET) {	/* from user space */
		start_pa = m4u_user_v2p(BufAddr);
		if (!start_pa)
			M4UDBG("m4u_user_v2p=0 in m4u_get_pages()\n");

		down_read(&current->mm->mmap_sem);

		vma = find_vma(current->mm, BufAddr);
		if (vma == NULL) {
			M4UMSG("cannot find vma: module=%s, va=0x%lx, size=0x%lx\n",
			       m4u_get_module_name(eModuleID), BufAddr, BufSize);
			up_read(&current->mm->mmap_sem);
			return -1;
		}
		write_mode = (vma->vm_flags & VM_WRITE) ? 1 : 0;
		if ((vma->vm_flags) & VM_PFNMAP) {
			unsigned long bufEnd = BufAddr + BufSize - 1;

			if (bufEnd > vma->vm_end + M4U_PAGE_SIZE) {
				M4UMSG
				    ("error: page_num=%d,module=%s, va=0x%lx, size=0x%lx, vm_flag=0x%x\n",
				     page_num, m4u_get_module_name(eModuleID),
				     BufAddr, BufSize, (unsigned int)vma->vm_flags);
				M4UMSG("but vma is: vm_start=0x%lx, vm_end=0x%lx\n",
				       (unsigned long)vma->vm_start, (unsigned long)vma->vm_end);
				up_read(&current->mm->mmap_sem);
				return -1;
			}
			up_read(&current->mm->mmap_sem);

			for (i = 0; i < page_num; i++) {
				unsigned long va_align = BufAddr & (~M4U_PAGE_MASK);
				int fault_cnt;

				for (fault_cnt = 0; fault_cnt < 30; fault_cnt++) {
					*(pPhys + i) = m4u_user_v2p(va_align + 0x1000 * i);
					if (!*(pPhys + i) && (va_align + 0x1000 * i >= vma->vm_start)
					    && (va_align + 0x1000 * i <= vma->vm_end)) {
						handle_mm_fault(current->mm, vma, va_align + 0x1000 * i,
								(vma->vm_flags & VM_WRITE) ? FAULT_FLAG_WRITE : 0);
						cond_resched();
					} else
						break;
				}

				if (fault_cnt > 20) {
					M4UMSG("%s, %d, fault_cnt %d, Bufaddr 0x%lx, i %d, page_num 0x%x\n",
					__func__, __LINE__, fault_cnt, BufAddr, i, page_num);
					M4UMSG("alloc_mva VM_PFNMAP module=%s, va=0x%lx, size=0x%lx, vm_flag=0x%x\n",
						m4u_get_module_name(eModuleID), BufAddr, BufSize,
						(unsigned int)vma->vm_flags);
					return -1;
				}
			}

			M4UDBG
			    ("alloc_mva VM_PFNMAP module=%s, va=0x%lx, size=0x%lx, vm_flag=0x%x\n",
			     m4u_get_module_name(eModuleID), BufAddr, BufSize,
			     (unsigned int)vma->vm_flags);
		} else {
			ret = m4u_get_user_pages(eModuleID, current, current->mm,
						BufAddr, page_num, write_mode,
						0, (struct page **)pPhys, vma);

			up_read(&current->mm->mmap_sem);

			if (ret < page_num) {
				/* release pages first */
				for (i = 0; i < ret; i++)
					m4u_put_unlock_page((struct page *)(*(pPhys + i)));

				if (unlikely(fatal_signal_pending(current))) {
					M4UMSG("error: receive sigkill during get_user_pages(), ");
					M4UMSG("page_num=%d, return=%d, module=%s, current_process:%s\n",
						page_num, ret, m4u_get_module_name(eModuleID),
						current->comm);
				}
				/*
				 * return value bigger than 0 but smaller than expected,
				 * trigger red screen
				 */
				if (ret > 0) {
					M4UMSG("error: page_num=%d, get_user_pages return=%d", page_num, ret);
					M4UMSG("module=%s, current_process:%s\n",
						m4u_get_module_name(eModuleID),
						current->comm);
					M4UMSG("error hint: maybe the allocated VA size is smaller than");
					M4UMSG("the size configured to m4u_alloc_mva()!");
					/*
					 * return value is smaller than 0, maybe the buffer is not
					 * exist, just return error to up-layer.
					 */
				} else {
					M4UMSG("error: page_num=%d, get_user_pages return=%d", page_num, ret);
					M4UMSG("module=%s, current_process:%s\n",
						m4u_get_module_name(eModuleID),
						current->comm);
					M4UMSG("error hint: maybe the VA is deallocated before call");
					M4UMSG("m4u_alloc_mva(), or no VA has be ever allocated!");
				}

				return -EFAULT;
			}

			for (i = 0; i < page_num; i++) {
				*(pPhys + i) =
				    page_to_phys((struct page *)(*(pPhys + i)));
			}
		}
	} else {		/* from kernel space */

		if (BufAddr >= VMALLOC_START && BufAddr <= VMALLOC_END) {	/* vmalloc */
			struct page *ppage;

			for (i = 0; i < page_num; i++) {
				ppage =
				    vmalloc_to_page((unsigned int *)(BufAddr +
								     i * M4U_PAGE_SIZE));
				*(pPhys + i) = page_to_phys(ppage) & 0xfffff000;
			}
		} else {	/* kmalloc */
			for (i = 0; i < page_num; i++)
				*(pPhys + i) = virt_to_phys((void *)((BufAddr & 0xfffff000) +
							i * M4U_PAGE_SIZE));
		}
	}
	return page_num;
}
/*
 * the upper caller should make sure the va is page aligned
 * get the pa from va, and calc the size of pa, fill the pa into the sgtable.
 * if the va does not have pages, fill the sg_dma_address with pa.
 * We need to modify the arm_iommu_map_sg inter face.
 */
struct sg_table *pseudo_get_sg(int portid, unsigned long va, int size)
{
	int i, page_num, ret, have_page, get_pages;
	struct sg_table *table;
	struct scatterlist *sg;
	struct page *page;
	struct vm_area_struct *vma = NULL;
	unsigned long *pPhys;

	page_num = M4U_GET_PAGE_NUM(va, size);
	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table) {
		M4UMSG("%s kzalloc failed table is null.\n", __func__);
		return NULL;
	}

	ret = sg_alloc_table(table, page_num, GFP_KERNEL);
	if (ret) {
		M4UMSG("%s sg alloc table failed %d. page_num is %d\n", __func__, ret, page_num);
		kfree(table);
		return NULL;
	}
	sg = table->sgl;

	pPhys = vmalloc(page_num * sizeof(unsigned long *));
	if (pPhys == NULL) {
		M4UMSG("m4u_fill_pagetable : error to vmalloc %d*4 size\n", page_num);
		goto err_free;
	}
	get_pages = m4u_get_pages(portid, va, size, pPhys);
	if (get_pages <= 0) {
		M4UDBG("Error : m4u_get_pages failed\n");
		goto err_free;
	}

	down_read(&current->mm->mmap_sem);
	vma = find_vma(current->mm, va);
	if (vma && vma->vm_flags & VM_PFNMAP)
		have_page = 0;
	else
		have_page = 1;
	up_read(&current->mm->mmap_sem);
	for (i = 0; i < page_num; i++) {
		va += i * M4U_PAGE_SIZE;

		if (((pPhys[i] & (M4U_PAGE_SIZE - 1)) != (va & (M4U_PAGE_SIZE - 1)) || !pPhys[i])
			&& (i != page_num - 1)) {
			M4UMSG("m4u user v2p failed, handle mm fault, pa is 0x%lx\n", pPhys[i]);
		}


		if (!pPhys[i] && i < page_num - 1) {
			M4UMSG("%s get pa failed, pa is 0. va is 0x%lx, page_num is %d, i %d, module is %s\n",
				__func__, va, page_num, i, m4u_get_port_name(portid));
			goto err_free;
		}

		if (have_page) {
			page = phys_to_page(pPhys[i]);
			sg_set_page(sg, page, M4U_PAGE_SIZE, 0);
		} else {
			/*
			 * the pa must not be set to zero or DMA would omit this page and then the mva
			 * allocation would be failed. So just make the last pages's pa as it's previous
			 * page plus page size. It's ok to do so since the hw would not access this very
			 * last page.
			 * DMA would like to ovmit the very last sg if the pa is 0
			 */
			if ((i == page_num - 1) && (pPhys[i] == 0)) {
				/* i == 0 should be take care of specially. */
				if (i)
					pPhys[i] = pPhys[i - 1] + M4U_PAGE_SIZE;
				else
					pPhys[i] = M4U_PAGE_SIZE;
			}

			sg_dma_address(sg) = pPhys[i];
			sg_dma_len(sg) = M4U_PAGE_SIZE;
		}
		sg = sg_next(sg);
	}

	vfree(pPhys);

	return table;

err_free:
	sg_free_table(table);
	kfree(table);
	if (pPhys)
		vfree(pPhys);

	return NULL;
}

/* the caller should make sure the mva offset have been eliminated. */
int __m4u_dealloc_mva(M4U_MODULE_ID_ENUM eModuleID,
		      const unsigned long BufAddr,
		      const unsigned int BufSize, const unsigned int MVA, struct sg_table *sg_table)
{
	struct sg_table *table = NULL;
	int kernelport = m4u_user2kernel_port(eModuleID);
	struct device *dev = m4u_get_larbdev(kernelport);
	struct iommu_domain *domain;
	unsigned long addr_align = MVA;
	unsigned int size_align = BufSize;
	int offset;

	if (!dev) {
		M4UMSG("%s, %d, dev is NULL\n", __func__, __LINE__);
		return -EINVAL;
	} else {
		domain = iommu_get_domain_for_dev(dev);
		if (!domain) {
			M4UMSG("%s, %d, domain is NULL\n", __func__, __LINE__);
			return -EINVAL;
		}
	}

	M4UDBG
	    ("m4u_dealloc_mva, module = %s, addr = 0x%lx, size = 0x%x, MVA = 0x%x, mva_end = 0x%x\n",
	     m4u_get_module_name(kernelport), BufAddr, BufSize, MVA, MVA + BufSize - 1);

	/* for ion sg alloc, we did not align the mva in allocation. */
	if (!sg_table)
		offset = m4u_va_align(&addr_align, &size_align);

	if (sg_table) {
		m4u_buf_info_t *m4u_buf_info;

		m4u_buf_info = m4u_client_find_buf(ion_m4u_client, addr_align, 1);
		if (m4u_buf_info && m4u_buf_info->mva != addr_align)
			M4UMSG("warning: %s, %d, mva address are not same\n", __func__, __LINE__);
		table = m4u_del_sgtable(addr_align);
		if (!table) {
			M4UERR("%s, %d, could not found the table from mva 0x%x\n",
				__func__, __LINE__, MVA);
			M4UERR("m4u_dealloc_mva, module = %s, addr = 0x%lx, size = 0x%x, MVA = 0x%x, mva_end = 0x%x\n",
				m4u_get_module_name(kernelport), BufAddr, BufSize, MVA, MVA + BufSize - 1);
			dump_stack();
			return -EINVAL;
		}

		if (sg_page(table->sgl) != sg_page(sg_table->sgl)) {
			M4UMSG("%s, %d, error, sg have not been added\n", __func__, __LINE__);
			return -EINVAL;
		}

		m4u_free_buf_info(m4u_buf_info);
	}

	if (!table)
		table = m4u_del_sgtable(addr_align);

	if (table)
		iommu_dma_unmap_sg(dev, table->sgl, table->orig_nents, 0, NULL);
	else {
		M4UERR("could not found the sgtable and would return error\n");
		return -EINVAL;
	}


	if (BufAddr) {
		/* from user space */
		if (BufAddr < PAGE_OFFSET) {
			struct vm_area_struct *vma = NULL;

			M4UTRACE();
			if (current->mm) {
				down_read(&current->mm->mmap_sem);
				vma = find_vma(current->mm, BufAddr);
			} else if (current->active_mm) {
				down_read(&current->active_mm->mmap_sem);
				vma = NULL;
			}
			M4UTRACE();
			if (vma == NULL) {
				M4UMSG("cannot find vma: module=%s, va=0x%lx, size=0x%x\n",
				       m4u_get_module_name(eModuleID), BufAddr, BufSize);
				if (current->mm)
					up_read(&current->mm->mmap_sem);
				else if (current->active_mm)
					up_read(&current->active_mm->mmap_sem);
				goto out;
			}
			if ((vma->vm_flags) & VM_PFNMAP) {
				if (current->mm)
					up_read(&current->mm->mmap_sem);
				else if (current->active_mm)
					up_read(&current->active_mm->mmap_sem);
				goto out;
			}
			if (current->mm)
				up_read(&current->mm->mmap_sem);
			else if (current->active_mm)
				up_read(&current->active_mm->mmap_sem);
			if (!((BufAddr >= VMALLOC_START) && (BufAddr <= VMALLOC_END)))
				if (!sg_table) {
					if (BufAddr +  BufSize < vma->vm_end)
						m4u_put_sgtable_pages(table, table->nents);
					else
						m4u_put_sgtable_pages(table, table->nents - 1);
				}
		}
	}

out:
	if (table) {
		sg_free_table(table);
		kfree(table);
	}

	M4UTRACE();
	return 0;

}

/*
 * when using this interface, the m4u alloc mva have make the va and mva with the page
 * offset, so the caller should make sure the offset and size is already eliminated.
 * or the iova we got from map_sg could not be unmapped all.
 */
int m4u_dealloc_mva(M4U_MODULE_ID_ENUM eModuleID,
		    const unsigned long BufAddr, const unsigned int BufSize,
		    const unsigned int MVA)
{
	return __m4u_dealloc_mva(eModuleID, BufAddr, BufSize, MVA, NULL);
}

/* the cache sync related ops are just copy from original implementation of mt7623. */
static struct vm_struct *cache_map_vm_struct;
int m4u_cache_sync_init(void)
{
	cache_map_vm_struct = get_vm_area(PAGE_SIZE, VM_ALLOC);
	if (!cache_map_vm_struct)
		return -ENOMEM;

	return 0;
}

struct page *m4u_cache_get_page(unsigned long va)
{
	unsigned long pa, start;
	struct page *page;

	start = va & (~M4U_PAGE_MASK);
	pa = m4u_user_v2p(start);
	if ((pa == 0)) {
		M4UMSG("error m4u_get_phys user_v2p return 0 on va=0x%lx\n", start);
		return NULL;
	}
	page = phys_to_page(pa);

	return page;
}

int __m4u_cache_sync_kernel(const void *start, size_t size, int direction)
{
	/* int i; */
	if (direction == DMA_TO_DEVICE) /* clean */
		dmac_map_area((void *)start, size, DMA_TO_DEVICE);
	else if (direction == DMA_FROM_DEVICE)/* invalid */
		dmac_unmap_area((void *)start, size, DMA_FROM_DEVICE);
	else if (direction == DMA_BIDIRECTIONAL)/* flush */
		dmac_flush_range((void *)start, (void *)(start + size - 1));

	return 0;
}

void *m4u_cache_map_page_va(struct page *page)
{
	int ret;
	struct page **ppPage = &page;

	ret = map_vm_area(cache_map_vm_struct, PAGE_KERNEL, ppPage);
	if (ret) {
		M4UMSG("error to map page\n");
		return NULL;
	}
	return cache_map_vm_struct->addr;
}

void m4u_cache_unmap_page_va(unsigned long va)
{
	unmap_kernel_range((unsigned long)cache_map_vm_struct->addr, PAGE_SIZE);
}

/* lock to protect cache_map_vm_struct */
static DEFINE_MUTEX(gM4u_cache_sync_user_lock);

int __m4u_cache_sync_user(unsigned long start, size_t size, int direction)
{
	unsigned int map_size;
	unsigned long map_start, map_end;
	unsigned long end = start + size;
	/* unsigned int fragment; */
	struct page *page;
	unsigned long map_va, map_va_align;
	int ret = 0;

	mutex_lock(&gM4u_cache_sync_user_lock);

	if (!cache_map_vm_struct) {
		M4UMSG("error: cache_map_vm_struct is NULL, retry\n");
		m4u_cache_sync_init();
	}
	if (!cache_map_vm_struct) {
		M4UMSG("error: cache_map_vm_struct is NULL, no vmalloc area\n");
		ret = -1;
		goto out;
	}

	M4UDBG("__m4u_sync_user: start=0x%lx, size=0x%x\n", start, (unsigned int)size);

	map_start = start;
	while (map_start < end) {
		map_end = min((map_start & (~M4U_PAGE_MASK)) + M4U_PAGE_SIZE, end);
		map_size = map_end - map_start;

		page = m4u_cache_get_page(map_start);
		if (!page) {
			ret = -1;
			goto out;
		}

		map_va = (unsigned long)m4u_cache_map_page_va(page);
		if (!map_va) {
			ret = -1;
			goto out;
		}

		map_va_align = map_va | (map_start & (M4U_PAGE_SIZE - 1));

		M4UDBG("__m4u_sync_user: map_start=0x%lx, map_size=0x%x, map_va=0x%lx\n",
		       map_start, map_size, map_va_align);
		__m4u_cache_sync_kernel((void *)map_va_align, map_size, direction);

		m4u_cache_unmap_page_va(map_va);
		map_start = map_end;
	}

out:
	mutex_unlock(&gM4u_cache_sync_user_lock);

	return ret;

}

int m4u_do_dma_cache_maint(M4U_MODULE_ID_ENUM eModuleID, const void *va, size_t size, int direction)
{

	unsigned int page_num;
	int ret = 0;

	if ((((unsigned long)va % L1_CACHE_BYTES != 0) || (size % L1_CACHE_BYTES) != 0)) {
		M4UMSG("Buffer align error: larb=%d,module=%s,addr=0x%lx,size=%zu,align=%d\n",
		       m4u_get_larbid(eModuleID), m4u_get_module_name(eModuleID),
		       (unsigned long)va, size, L1_CACHE_BYTES);
		M4UMSG
		    ("error: addr un-align, module=%s, addr=0x%lx, size=0x%x, process=%s, align=0x%x\n",
		     m4u_get_module_name(eModuleID), (unsigned long)va, (unsigned int)size, current->comm,
		     L1_CACHE_BYTES);
	}

	page_num = M4U_GET_PAGE_NUM(va, size);

	if ((unsigned long)va < PAGE_OFFSET) /* from user space */
		ret = __m4u_cache_sync_user((unsigned long)va, size, direction);
	else
		ret = __m4u_cache_sync_kernel(va, size, direction);

	M4UDBG("cache_sync: module=%s, addr=0x%lx, size=0x%lx\n",
	       m4u_get_module_name(eModuleID),  (unsigned long)va, size);

	return ret;
}

int m4u_dma_cache_maint(M4U_MODULE_ID_ENUM eModuleID,
		const void *start, size_t size, int direction)
{
	return m4u_do_dma_cache_maint(eModuleID, start, size, direction);
}

/*
 * inherent this from original m4u driver, we use this to make sure we could still support
 * userspace ioctl commands.
 */
static long MTK_M4U_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	M4U_MOUDLE_STRUCT m4u_module;
	M4U_PORT_STRUCT m4u_port;
	M4U_PORT_ID PortID;
	M4U_PORT_ID ModuleID;
	M4U_CACHE_STRUCT m4u_cache_data;
	m4u_client_t *client = filp->private_data;

	switch (cmd) {
	case MTK_M4U_T_POWER_ON:
		ret = copy_from_user(&ModuleID, (void *)arg, sizeof(unsigned int));
		if (ret) {
			M4UMSG("MTK_M4U_T_POWER_ON,copy_from_user failed,%d\n", ret);
			return -EFAULT;
		}
		ret = 0;
		break;

	case MTK_M4U_T_POWER_OFF:
		ret = copy_from_user(&ModuleID, (void *)arg, sizeof(unsigned int));
		if (ret) {
			M4UMSG("MTK_M4U_T_POWER_OFF,copy_from_user failed,%d\n", ret);
			return -EFAULT;
		}
		ret = 0;
		break;

	case MTK_M4U_T_ALLOC_MVA:
		ret = copy_from_user(&m4u_module, (void *)arg, sizeof(M4U_MOUDLE_STRUCT));
		if (ret) {
			M4UMSG("MTK_M4U_T_ALLOC_MVA,copy_from_user failed:%d\n", ret);
			return -EFAULT;
		}

		M4UDBG("MTK_M4U_T_ALLOC_MVA, %s, %d\n", __func__, __LINE__);
		ret = pseudo_alloc_mva(client, m4u_module.port, m4u_module.BufAddr,
				    NULL, m4u_module.BufSize, m4u_module.prot, m4u_module.flags,
				    &(m4u_module.MVAStart));

		if (ret)
			return ret;

		ret = copy_to_user(&(((M4U_MOUDLE_STRUCT *) arg)->MVAStart), &(m4u_module.MVAStart),
				sizeof(unsigned int));
		if (ret) {
			M4UMSG("MTK_M4U_T_ALLOC_MVA,copy_from_user failed:%d\n", ret);
			return -EFAULT;
		}
		break;

	case MTK_M4U_T_DEALLOC_MVA:
		{
			int offset;
			m4u_buf_info_t *pMvaInfo;
			unsigned long align_mva;

			ret = copy_from_user(&m4u_module, (void *)arg, sizeof(M4U_MOUDLE_STRUCT));
			if (ret) {
				M4UMSG("MTK_M4U_T_DEALLOC_MVA,copy_from_user failed:%d\n", ret);
				return -EFAULT;
			}

			M4UDBG
			    ("MTK_M4U_T_DEALLOC_MVA, eModuleID : %d, VABuf:0x%lx, Length : %d, MVAStart = 0x%x \r\n",
			     m4u_module.port, m4u_module.BufAddr, m4u_module.BufSize,
			     m4u_module.MVAStart);

			if (!m4u_module.BufAddr || !m4u_module.BufSize) {
				M4UDBG("MTK_M4U_T_DEALLOC_MVA va is 0x%lx, size is 0x%x",
					m4u_module.BufAddr, m4u_module.BufSize);
				/* return -EINVAL;*/
			}

			/*
			 * we store the not aligned value in m4u client, but
			 * aligned value in pseudo_sglist, so we need to delete
			 * from client with the not-aligned value and deallocate
			 * mva with the aligned value.
			 */
			pMvaInfo = m4u_client_find_buf(client, m4u_module.MVAStart, 1);

			/* to pass the code defect check */
			align_mva = m4u_module.MVAStart;
			offset = m4u_va_align(&align_mva, &m4u_module.BufSize);
			m4u_module.MVAStart = align_mva & 0xffffffff;
			/* m4u_module.MVAStart -= offset; */

			if (m4u_module.MVAStart != 0) {
				m4u_dealloc_mva(m4u_module.port,
						m4u_module.BufAddr,
						m4u_module.BufSize, m4u_module.MVAStart);
				m4u_free_buf_info(pMvaInfo);
			} else {
				M4UMSG
				    ("warning : deallocat a registered buffer, before any query !\n");
				M4UMSG
				    ("error to dealloc mva : id = %s, va = 0x%lx, size = %d, mva = 0x%x\n",
				     m4u_get_module_name(m4u_module.port),
				     m4u_module.BufAddr, m4u_module.BufSize,
				     m4u_module.MVAStart);
			}

		}

		break;

	case MTK_M4U_T_DUMP_INFO:
		ret = copy_from_user(&ModuleID, (void *)arg, sizeof(unsigned int));
		if (ret) {
			M4UMSG("MTK_M4U_Invalid_TLB_Range,copy_from_user failed,%d\n", ret);
			return -EFAULT;
		}

		break;

	case MTK_M4U_T_CACHE_SYNC:
		ret = copy_from_user(&m4u_cache_data, (void *)arg, sizeof(M4U_CACHE_STRUCT));
		if (ret) {
			M4UMSG("m4u_cache_sync,copy_from_user failed:%d\n", ret);
			return -EFAULT;
		}
		M4UDBG("MTK_M4U_T_CACHE_INVALID_AFTER_HW_WRITE_MEM(),");
		M4UDBG("moduleID = %d, eCacheSync = %d, buf_addr = 0x%lx, buf_length = 0x%x\n",
			m4u_cache_data.port, m4u_cache_data.eCacheSync, m4u_cache_data.va,
			m4u_cache_data.size);

		switch (m4u_cache_data.eCacheSync) {
		case M4U_CACHE_FLUSH_BEFORE_HW_READ_MEM:
			ret =
			    m4u_dma_cache_maint(m4u_cache_data.port,
						(unsigned long *)(m4u_cache_data.va),
						m4u_cache_data.size, M4U_DMA_READ_WRITE);
			break;

		case M4U_CACHE_FLUSH_BEFORE_HW_WRITE_MEM:
			ret =
			    m4u_dma_cache_maint(m4u_cache_data.port,
						(unsigned int *)(m4u_cache_data.va),
						m4u_cache_data.size, M4U_DMA_READ);
			break;

		case M4U_CACHE_CLEAN_BEFORE_HW_READ_MEM:
			ret =
			    m4u_dma_cache_maint(m4u_cache_data.port,
						(unsigned int *)(m4u_cache_data.va),
						m4u_cache_data.size, M4U_DMA_WRITE);
			break;

		case M4U_CACHE_CLEAN_ALL:
			smp_inner_dcache_flush_all();
			outer_clean_all();
			break;

		case M4U_CACHE_INVALID_ALL:
			M4UMSG("no one can use invalid all!\n");
			break;

		case M4U_CACHE_FLUSH_ALL:
			smp_inner_dcache_flush_all();
			outer_flush_all();
			break;

		default:
			M4UMSG
			    ("error : MTK_M4U_T_CACHE_SYNC, invalid eCacheSync = %d, module = %d\n",
			     m4u_cache_data.eCacheSync, m4u_cache_data.port);
		}
		break;

	case MTK_M4U_T_CONFIG_PORT:
		ret = copy_from_user(&m4u_port, (void *)arg, sizeof(M4U_PORT_STRUCT));
		if (ret) {
			M4UMSG("MTK_M4U_T_CONFIG_PORT,copy_from_user failed:%d\n", ret);
			return -EFAULT;
		}

		ret = m4u_config_port(&m4u_port);
		break;


	case MTK_M4U_T_MONITOR_START:
		ret = copy_from_user(&PortID, (void *)arg, sizeof(unsigned int));
		if (ret) {
			M4UMSG("MTK_M4U_T_MONITOR_START,copy_from_user failed,%d\n", ret);
			return -EFAULT;
		}
		ret = 0;

		break;

	case MTK_M4U_T_MONITOR_STOP:
		ret = copy_from_user(&PortID, (void *)arg, sizeof(unsigned int));
		if (ret) {
			M4UMSG("MTK_M4U_T_MONITOR_STOP,copy_from_user failed,%d\n", ret);
			return -EFAULT;
		}
		ret = 0;
		break;

	case MTK_M4U_T_CACHE_FLUSH_ALL:
		m4u_dma_cache_flush_all();
		break;

	case MTK_M4U_T_CONFIG_PORT_ARRAY:
		{
			struct m4u_port_array port_array;

			ret =
			    copy_from_user(&port_array, (void *)arg, sizeof(struct m4u_port_array));
			if (ret) {
				M4UMSG("MTK_M4U_T_CONFIG_PORT,copy_from_user failed:%d\n", ret);
				return -EFAULT;
			}

			ret = m4u_config_port_array(&port_array);
		}
		break;

	case MTK_M4U_T_CONFIG_MAU:
	case MTK_M4U_T_CONFIG_TF:
		break;

	default:
		M4UMSG("MTK M4U ioctl:No such command!!\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)

static int compat_get_m4u_module_struct(COMPAT_M4U_MOUDLE_STRUCT __user *data32,
					M4U_MOUDLE_STRUCT __user *data)
{
	compat_uint_t u;
	compat_ulong_t l;
	int err;

	err = get_user(u, &(data32->port));
	err |= put_user(u, &(data->port));
	err |= get_user(l, &(data32->BufAddr));
	err |= put_user(l, &(data->BufAddr));
	err |= get_user(u, &(data32->BufSize));
	err |= put_user(u, &(data->BufSize));
	err |= get_user(u, &(data32->prot));
	err |= put_user(u, &(data->prot));
	err |= get_user(u, &(data32->MVAStart));
	err |= put_user(u, &(data->MVAStart));
	err |= get_user(u, &(data32->MVAEnd));
	err |= put_user(u, &(data->MVAEnd));
	err |= get_user(u, &(data32->flags));
	err |= put_user(u, &(data->flags));

	return err;
}

static int compat_put_m4u_module_struct(COMPAT_M4U_MOUDLE_STRUCT __user *data32,
					M4U_MOUDLE_STRUCT __user *data)
{
	compat_uint_t u;
	compat_ulong_t l;
	int err;


	err = get_user(u, &(data->port));
	err |= put_user(u, &(data32->port));
	err |= get_user(l, &(data->BufAddr));
	err |= put_user(l, &(data32->BufAddr));
	err |= get_user(u, &(data->BufSize));
	err |= put_user(u, &(data32->BufSize));
	err |= get_user(u, &(data->prot));
	err |= put_user(u, &(data32->prot));
	err |= get_user(u, &(data->MVAStart));
	err |= put_user(u, &(data32->MVAStart));
	err |= get_user(u, &(data->MVAEnd));
	err |= put_user(u, &(data32->MVAEnd));
	err |= get_user(u, &(data->flags));
	err |= put_user(u, &(data32->flags));

	return err;
}

static int compat_get_m4u_cache_struct(COMPAT_M4U_CACHE_STRUCT __user *data32,
				       M4U_CACHE_STRUCT __user *data)
{
	compat_uint_t u;
	compat_ulong_t l;
	int err;

	err = get_user(u, &(data32->port));
	err |= put_user(u, &(data->port));
	err |= get_user(u, &(data32->eCacheSync));
	err |= put_user(u, &(data->eCacheSync));
	err |= get_user(l, &(data32->va));
	err |= put_user(l, &(data->va));
	err |= get_user(u, &(data32->size));
	err |= put_user(u, &(data->size));
	err |= get_user(u, &(data32->mva));
	err |= put_user(u, &(data->mva));

	return err;
}


long MTK_M4U_COMPAT_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_MTK_M4U_T_ALLOC_MVA:
		{
			COMPAT_M4U_MOUDLE_STRUCT __user *data32;
			M4U_MOUDLE_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(M4U_MOUDLE_STRUCT));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_m4u_module_struct(data32, data);
			if (err)
				return err;

			ret = filp->f_op->unlocked_ioctl(filp, MTK_M4U_T_ALLOC_MVA,
							 (unsigned long)data);

			err = compat_put_m4u_module_struct(data32, data);

			if (err)
				return err;

			return ret;
		}
	case COMPAT_MTK_M4U_T_DEALLOC_MVA:
		{
			COMPAT_M4U_MOUDLE_STRUCT __user *data32;
			M4U_MOUDLE_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(M4U_MOUDLE_STRUCT));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_m4u_module_struct(data32, data);
			if (err)
				return err;

			return filp->f_op->unlocked_ioctl(filp, MTK_M4U_T_DEALLOC_MVA,
							  (unsigned long)data);
		}
	case COMPAT_MTK_M4U_T_CACHE_SYNC:
		{
			COMPAT_M4U_CACHE_STRUCT __user *data32;
			M4U_CACHE_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(M4U_CACHE_STRUCT));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_m4u_cache_struct(data32, data);
			if (err)
				return err;

			return filp->f_op->unlocked_ioctl(filp, MTK_M4U_T_CACHE_SYNC,
							  (unsigned long)data);
		}
	case MTK_M4U_T_POWER_ON:
	case MTK_M4U_T_POWER_OFF:
	case MTK_M4U_T_DUMP_INFO:
	case MTK_M4U_T_CONFIG_PORT:
	case MTK_M4U_T_MONITOR_START:
	case MTK_M4U_T_MONITOR_STOP:
	case MTK_M4U_T_CACHE_FLUSH_ALL:
	case MTK_M4U_T_CONFIG_PORT_ARRAY:
		return filp->f_op->unlocked_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
	default:
		return -ENOIOCTLCMD;
	}

}

#else

#define MTK_M4U_COMPAT_ioctl  NULL

#endif

#ifdef M4U_TEE_SERVICE_ENABLE
/* reserve iova address range for security world for 1GB memory */
static int __reserve_iova_sec(struct device *device,
				dma_addr_t dma_addr, size_t size)
{
	struct device *dev = m4u_get_larbdev(0);
	struct iommu_domain *domain;
	struct iova_domain *iovad;
	struct iova *iova;
	unsigned long sec_mem_size = size >> PAGE_SHIFT;

retry:
	/* in case pseudo larb has not finished probe. */
	if (!dev && !device) {
		M4UERR("device is NULL and the larb have not been probed\n");
		return -EINVAL;
	} else if (!dev && device) {
		dev = device;
	}

	domain = iommu_get_domain_for_dev(dev);
	if (!domain) {
		M4UMSG("%s, %d, get iommu_domain failed\n", __func__, __LINE__);
		domain = iommu_get_domain_for_dev(dev);
		cond_resched();
		goto retry;
	}

	iovad = domain->iova_cookie;

	/* iovad->start_pfn is 0x1, so limit_pfn need to add 1, do not align the size */
	iova = alloc_iova(iovad, sec_mem_size,
		sec_mem_size + 1, false);

	if (!iova) {
		dev_err(dev, "pseudo alloc_iova failed %s, %d\n", __func__, __LINE__);
		return -1;
	}

	dev_err(dev, "reserve iova for security world success, we get the iova start 0x%lx, end 0x%lx\n",
			iova->pfn_lo << PAGE_SHIFT, iova->pfn_hi << PAGE_SHIFT);

	return 0;
}
#endif

/*
 * reserve the iova for direct mapping.
 * without this, the direct mapping iova maybe allocated to other users,
 * and the armv7s iopgtable may assert warning and return error.
 * We reserve those iova to avoid this iova been allocated by other users.
 */
static int pseudo_reserve_dm(void)
{
	struct iommu_dm_region *entry;
	struct list_head mappings;
	struct device *dev = m4u_get_larbdev(0);
	struct iommu_domain *domain;
	struct iova_domain *iovad;
	struct iova *iova;
	unsigned long pg_size = SZ_4K, limit, shift;

	INIT_LIST_HEAD(&mappings);

	iommu_get_dm_regions(dev, &mappings);

	/* We need to consider overlapping regions for different devices */
	list_for_each_entry(entry, &mappings, list) {
		dma_addr_t start;

		start = ALIGN(entry->start, pg_size);
retry:
		domain = iommu_get_domain_for_dev(dev);
		if (!domain) {
			M4UMSG("%s, %d, get iommu_domain failed\n", __func__, __LINE__);
			domain = iommu_get_domain_for_dev(dev);
			cond_resched();
			goto retry;
		}

		iovad = domain->iova_cookie;
		shift = iova_shift(iovad);
		limit = (start + entry->length) >> shift;
		/* add plus one page for the size of allocation or there maybe overlap */
		iova = alloc_iova(iovad, (entry->length >> shift) + 1, limit, false);
		if (!iova) {
			dev_err(dev, "pseudo alloc_iova failed %s, %d, dm->start 0x%lx, dm->length 0x%lx\n",
				__func__, __LINE__, (unsigned long)entry->start, entry->length);
			return -1;
		}

		M4UMSG("reserve iova for dm success, dm->start 0x%lx, dm->length 0x%lx, start 0x%lx, end 0x%lx\n",
			(unsigned long)entry->start, entry->length,
			iova->pfn_lo << shift, iova->pfn_hi << shift);

	}

	return 0;
}

static const struct file_operations g_stMTK_M4U_fops = {
	.owner = THIS_MODULE,
	.open = MTK_M4U_open,
	.release = MTK_M4U_release,
	.flush = MTK_M4U_flush,
	.unlocked_ioctl = MTK_M4U_ioctl,
	.compat_ioctl = MTK_M4U_COMPAT_ioctl,
};


/* this will reserve the securit mva address range for iommu. */
/* static int pseudo_reserve_sec(void)
 * {
 * return 0;
 * }
 */
/*
 * Here's something need to be done in probe, we should get the following information from dts
 * 1. get the reserved memory range for fb buffer and security trustzone mva range
 * 2. get the larb port device for attach device in order to config m4u port
 */

/*
 * iommus = <&iommu portid>
 */
static int pseudo_probe(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 3; i++) {
		/* wait for larb probe done. */
		if (mtk_smi_larb_ready(i) == 0)
			return -EPROBE_DEFER;

		/* wait for pseudo larb probe done. */
		if (!larbdev[i].dev)
			return -EPROBE_DEFER;
	}

#ifdef M4U_TEE_SERVICE_ENABLE
{
	/* init the sec_mem_size to 400M to avoid build error. */
	unsigned int sec_mem_size = 400 * 0x100000;
	/*reserve mva range for sec */
	struct device *dev = &pdev->dev;
	unsigned long iommu_pgt_base = mtk_get_pgt_base();

	pseudo_m4u_session_init();

	/* bit[0:11] is reserved */
	iommu_pgt_base &= 0xfffff000;
	M4UMSG("%s, %d, iommu_pgt_base 0x%lx\n", __func__, __LINE__, iommu_pgt_base);
	pseudo_m4u_sec_init(iommu_pgt_base, gM4U_L2_enable, &sec_mem_size);
		/* reserve mva range for security world */
	__reserve_iova_sec(dev, 0, sec_mem_size);
}
#endif
	pseudo_reserve_dm();
	gM4uDev = kzalloc(sizeof(struct m4u_device), GFP_KERNEL);
	if (!gM4uDev) {
		M4UMSG("kmalloc for m4u_device fail\n");
		return -ENOMEM;
	}
	gM4uDev->m4u_dev_proc_entry = proc_create("m4u", 0, NULL, &g_stMTK_M4U_fops);
	if (!gM4uDev->m4u_dev_proc_entry) {
		M4UERR("proc m4u create error\n");
		return -ENODEV;
	}

	m4u_cache_sync_init();

	return 0;
}

static int pseudo_port_probe(struct platform_device *pdev)
{
	int larbid;
	int ret;
	struct device *dev;
	struct device_dma_parameters *dma_param;
	struct device_node *np;

	/* dma will split the iova into max size to 65535 byte by default if we do not set this.*/
	dma_param = kzalloc(sizeof(*dma_param), GFP_KERNEL);
	if (!dma_param)
		return -ENOMEM;

	/* set the iova to 256MB for one time map, this should be suffice for ION */
	dma_param->max_segment_size = 0x10000000;
	dev = &pdev->dev;
	dev->dma_parms = dma_param;
	ret = of_property_read_u32(dev->of_node, "mediatek,larbid", &larbid);
	if (ret)
		return ret;

	larbdev[larbid].larbid = larbid;
	larbdev[larbid].dev = &pdev->dev;

	if (larbdev[larbid].mmuen)
		return 0;

	dev = larbdev[larbid].dev;

	np = of_parse_phandle(dev->of_node, "iommus", 0);
	if (!np)
		return 0;

	pdev = of_find_device_by_node(np);
	of_node_put(np);
	if (WARN_ON(!pdev))
		return -EINVAL;

	return 0;
}

static int pseudo_remove(struct platform_device *pdev)
{
	return 0;
}

static int pseudo_port_remove(struct platform_device *pdev)
{
	return 0;
}

static int pseudo_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int pseudo_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver pseudo_driver = {
	.probe = pseudo_probe,
	.remove = pseudo_remove,
	.suspend = pseudo_suspend,
	.resume = pseudo_resume,
	.driver = {
		.name = M4U_DEVNAME,
		.of_match_table = mtk_pseudo_of_ids,
		.owner = THIS_MODULE,
	}
};

static struct platform_driver pseudo_port_driver = {
	.probe = pseudo_port_probe,
	.remove = pseudo_port_remove,
	.suspend = pseudo_suspend,
	.resume = pseudo_resume,
	.driver = {
		.name = "pseudo_port_device",
		.of_match_table = mtk_pseudo_port_of_ids,
		.owner = THIS_MODULE,
	}
};


static int __init mtk_pseudo_init(void)
{
	if (platform_driver_register(&pseudo_driver)) {
		M4UMSG("failed to register pseudo driver");
		return -ENODEV;
	}

	if (platform_driver_register(&pseudo_port_driver)) {
		M4UMSG("failed to register pseudo port driver");
		platform_driver_unregister(&pseudo_driver);
		return -ENODEV;
	}

	return 0;
}

static void __exit mtk_pseudo_exit(void)
{
	platform_driver_unregister(&pseudo_driver);
	platform_driver_unregister(&pseudo_port_driver);
}

module_init(mtk_pseudo_init);
module_exit(mtk_pseudo_exit);

MODULE_DESCRIPTION("MTK pseudo m4u driver based on iommu");
MODULE_AUTHOR("Honghui Zhang <honghui.zhang@mediatek.com>");
MODULE_LICENSE("GPL");
