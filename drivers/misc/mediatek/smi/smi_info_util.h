#ifndef __SMI_INFO_UTIL_H__
#define __SMI_INFO_UTIL_H__

#include <asm/io.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <mt_smi.h>
int smi_set_mm_info_ioctl_wrapper(struct file *pFile, unsigned int cmd, unsigned long param);
int smi_get_mm_info_ioctl_wrapper(struct file *pFile, unsigned int cmd, unsigned long param);
void smi_bwc_mm_info_set(int property_id, long val1, long val2);
extern MTK_SMI_BWC_MM_INFO g_smi_bwc_mm_info;

#endif				/* __SMI_INFO_UTIL_H__ */
