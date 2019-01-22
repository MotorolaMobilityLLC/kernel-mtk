#ifndef __SPM_V3__MTK_SPM_RESOURCE_REQ_H__
#define __SPM_V3__MTK_SPM_RESOURCE_REQ_H__

/* SPM resource request APIs: public */

enum {
	SPM_RESOURCE_MAINPLL = 1 << 0,
	SPM_RESOURCE_DRAM    = 1 << 1,
	SPM_RESOURCE_CK_26M  = 1 << 2,
	SPM_RESOURCE_AXI_BUS = 1 << 3,
	NF_SPM_RESOURCE = 4
};

enum {
	SPM_RESOURCE_USER_SPM = 0,
	SPM_RESOURCE_USER_UFS,
	SPM_RESOURCE_USER_SSUSB,
	SPM_RESOURCE_USER_AUDIO,
	NF_SPM_RESOURCE_USER
};

bool spm_resource_req(unsigned int user, unsigned int req_mask);

#endif /* __SPM_V3__MTK_SPM_RESOURCE_REQ_H__ */
