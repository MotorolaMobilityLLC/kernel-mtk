/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */





/**
 * @file
 * Run-time work-arounds helpers
 */

#include <mali_base_hwconfig.h>
#include <mali_midg_regmap.h>
#include "mali_kbase.h"
#include "mali_kbase_hw.h"

//MTK{{
#include <linux/of.h>
#include <linux/of_address.h>
//}}MTK

void kbase_hw_set_features_mask(struct kbase_device *kbdev)
{
	const enum base_hw_feature *features;
	u32 gpu_id;

	gpu_id = kbdev->gpu_props.props.raw_props.gpu_id;
	gpu_id &= GPU_ID_VERSION_PRODUCT_ID;
	gpu_id = gpu_id >> GPU_ID_VERSION_PRODUCT_ID_SHIFT;

	switch (gpu_id) {
	case GPU_ID_PI_T76X:
		features = base_hw_features_t76x;
		break;
#ifdef MALI_INCLUDE_TFRX
	case GPU_ID_PI_TFRX:
		/* Fall through */
#endif /* MALI_INCLUDE_TFRX */
	case GPU_ID_PI_T86X:
		features = base_hw_features_tFxx;
		break;
	case GPU_ID_PI_T72X:
		features = base_hw_features_t72x;
		break;
	case GPU_ID_MAKE(GPU_ID_PI_T62X, 0, 1, 0):
	case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 0, 0):
	case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 1, 0):
		features = base_hw_features_t62x;
		break;
	case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 0, GPU_ID_S_15DEV0):
	case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 0, GPU_ID_S_EAC):
	case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 1, 0):
		features = base_hw_features_t60x;
		break;
	default:
		features = base_hw_features_generic;
		break;
	}

	for (; *features != BASE_HW_FEATURE_END; features++)
		set_bit(*features, &kbdev->hw_features_mask[0]);
}

//MTK{{
void mtk_get_clk_MFG_HW_REST(void)
{
/*
b. MFG HW REST bit (0x10007018 )
*/
    struct device_node *node;
    void __iomem *clk_apmixed_base;
    
    node = of_find_compatible_node(NULL, NULL, "mediatek,TOPRGU");
    if (!node) {
        pr_alert("[Mali]MFG HW REST bit find node failed\n");
    }
    clk_apmixed_base = of_iomap(node, 0);
    if (!clk_apmixed_base){
        pr_alert("[Mali] MFG HW REST bit failed\n");
    }else{
        pr_alert("[Mali]MFG HW REST bit : 0x%08x \n",readl(clk_apmixed_base + 0x18) );        
    }

}

void mtk_get_clk_MFG_APB_PWR(void)
{
/*
c. MFG APB SW reset (0x10001150 )
*/
    struct device_node *node;
    void __iomem *clk_apmixed_base;
    
    node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
    if (!node) {
        pr_alert("[Mali]MFG APB SW reset find node failed\n");
    }
    clk_apmixed_base = of_iomap(node, 0);
    if (!clk_apmixed_base){
        pr_alert("[Mali] MFG APB SW reset failed\n");
    }else{
        pr_alert("[Mali]MFG APB SW reset : 0x%08x \n",readl(clk_apmixed_base + 0x150) );        
    }

}

void mtk_get_clk_MFG_CG_CON(void)
{
/*
f. MFG CG( 0x13000000 MFG_CG_CON ,correct : 0)
*/
    struct device_node *node;
    void __iomem *clk_apmixed_base;
    
    node = of_find_compatible_node(NULL, NULL, "mediatek,mt6755-mfgsys");
    if (!node) {
        pr_alert("[Mali]MFG_CG_CON find node failed\n");
    }
    clk_apmixed_base = of_iomap(node, 0);
    if (!clk_apmixed_base){
        pr_alert("[Mali] MFG_CG_CON failed\n");
    }else{
        pr_alert("[Mali]MFG_CG_CON : 0x%08x \n",readl(clk_apmixed_base + 0x18) );        
        pr_alert("[Mali]MFG 0x13000020  : 0x%08x \n",readl(clk_apmixed_base + 0x20) ); //Default Value 0x2A
    }

}

void mtk_get_clk_MFG_PWR(void)
{
#define MFG_ASYNC_PWR_CON_OFFSET 0x334
#define MFG_PWR_CON_OFFSET 0x338

    struct device_node *node;
    void __iomem *clk_apmixed_base;

    
    node = of_find_compatible_node(NULL, NULL, "mediatek,SLEEP");
    if (!node) {
        pr_alert("[Mali] find node failed\n");
    }
    clk_apmixed_base = of_iomap(node, 0);
    if (!clk_apmixed_base){
        pr_alert("[Mali] base failed\n");
    }else{
        pr_alert("[Mali]MFG_ASYNC_PWR_CON : 0x%08x \n",readl(clk_apmixed_base + MFG_ASYNC_PWR_CON_OFFSET) );
        pr_alert("[Mali]MFG_PWR : 0x%08x \n",readl(clk_apmixed_base + MFG_PWR_CON_OFFSET));
        pr_alert("[Mali]PWR_STATUS : 0x%08x \n",readl(clk_apmixed_base + 0x180) );
        pr_alert("[Mali]PWR_STATUS_2ND : 0x%08x \n",readl(clk_apmixed_base + 0x184));
        
    }
    mtk_get_clk_MFG_HW_REST();
    mtk_get_clk_MFG_APB_PWR();
    mtk_get_clk_MFG_CG_CON();
    /*
    b. MFG HW REST bit (0x10007018 )
c. MFG APB SW reset (0x10001150 )
d. PWR_STATUS (0x110006180) 
e. PWR_STATUS_2ND (0x110006184)
f. MFG CG( 0x13000000 MFG_CG_CON ,correct : 0)
*/


}
void get_more_Mali_Register(struct kbase_device *kbdev)
{
    pr_alert("MALI :[Register] get GPU ID : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(GPU_ID)) );
    pr_alert("MALI :[Register] get L2_FEATURES : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(L2_FEATURES)) );
    pr_alert("MALI :[Register] get L3_FEATURES : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(L3_FEATURES)) );
    pr_alert("MALI :[Register] get TILER_FEATURES : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(TILER_FEATURES)) );
    pr_alert("MALI :[Register] get MEM_FEATURES : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(MEM_FEATURES)) );
    pr_alert("MALI :[Register] get MMU_FEATURES : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(MMU_FEATURES)) );
    pr_alert("MALI :[Register] get AS_PRESENT : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(AS_PRESENT)) );
    pr_alert("MALI :[Register] get JS_PRESENT : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(JS_PRESENT)) );
    pr_alert("MALI :[Register] get GPU_IRQ_RAWSTAT : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)) );
    pr_alert("MALI :[Register] get GPU_IRQ_MASK : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK)) );
    pr_alert("MALI :[Register] get GPU_IRQ_STATUS : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_STATUS)) );
    pr_alert("MALI :[Register] get GPU_STATUS : 0x%x", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(GPU_STATUS)) );
}

//}}MTK

mali_error kbase_hw_set_issues_mask(struct kbase_device *kbdev)
{
	const enum base_hw_issue *issues;
	u32 gpu_id;
	u32 impl_tech;

	gpu_id = kbdev->gpu_props.props.raw_props.gpu_id;
	impl_tech = kbdev->gpu_props.props.thread_props.impl_tech;

    mtk_get_clk_MFG_PWR();
    pr_alert("[Mali] base gpu_id = 0x%x \n",gpu_id);

	if (impl_tech != IMPLEMENTATION_MODEL) {
		switch (gpu_id) {
		case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 0, GPU_ID_S_15DEV0):
			issues = base_hw_issues_t60x_r0p0_15dev0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 0, GPU_ID_S_EAC):
			issues = base_hw_issues_t60x_r0p0_eac;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 1, 0):
			issues = base_hw_issues_t60x_r0p1;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T62X, 0, 1, 0):
			issues = base_hw_issues_t62x_r0p1;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 0, 0):
		case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 0, 1):
			issues = base_hw_issues_t62x_r1p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 1, 0):
			issues = base_hw_issues_t62x_r1p1;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 0, 0):
			issues = base_hw_issues_t76x_r0p0_beta;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 0, 1):
			issues = base_hw_issues_t76x_r0p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 1, 1):
			issues = base_hw_issues_t76x_r0p1;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 1, 9):
			issues = base_hw_issues_t76x_r0p1_50rel0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 2, 1):
			issues = base_hw_issues_t76x_r0p2;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 3, 1):
			issues = base_hw_issues_t76x_r0p3;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T76X, 1, 0, 0):
			issues = base_hw_issues_t76x_r1p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T72X, 0, 0, 0):
		case GPU_ID_MAKE(GPU_ID_PI_T72X, 0, 0, 1):
		case GPU_ID_MAKE(GPU_ID_PI_T72X, 0, 0, 2):
			issues = base_hw_issues_t72x_r0p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T72X, 1, 0, 0):
			issues = base_hw_issues_t72x_r1p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T72X, 1, 1, 0):
			issues = base_hw_issues_t72x_r1p1;
			break;
#ifdef MALI_INCLUDE_TFRX
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 0, 0):
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 0, 1):
			issues = base_hw_issues_tFRx_r0p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 1, 2):
			issues = base_hw_issues_tFRx_r0p1;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 2, 0):
			issues = base_hw_issues_tFRx_r0p2;
			break;
#endif /* MALI_INCLUDE_TFRX */
		case GPU_ID_MAKE(GPU_ID_PI_T86X, 0, 0, 1):
			issues = base_hw_issues_t86x_r0p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T86X, 0, 2, 0):
			issues = base_hw_issues_t86x_r0p2;
			break;
		default:
			dev_err(kbdev->dev, "Unknown GPU ID %x", gpu_id);
			mtk_get_clk_MFG_PWR();
			get_more_Mali_Register(kbdev);
			return MALI_ERROR_FUNCTION_FAILED;
		}
	} else {
		/* Software model */
		switch (gpu_id >> GPU_ID_VERSION_PRODUCT_ID_SHIFT) {
		case GPU_ID_PI_T60X:
			issues = base_hw_issues_model_t60x;
			break;
		case GPU_ID_PI_T62X:
			issues = base_hw_issues_model_t62x;
			break;
		case GPU_ID_PI_T72X:
			issues = base_hw_issues_model_t72x;
			break;
		case GPU_ID_PI_T76X:
			issues = base_hw_issues_model_t76x;
			break;
#ifdef MALI_INCLUDE_TFRX
		case GPU_ID_PI_TFRX:
			issues = base_hw_issues_model_tFRx;
			break;
#endif /* MALI_INCLUDE_TFRX */
		case GPU_ID_PI_T86X:
			issues = base_hw_issues_model_t86x;
			break;
		default:
			dev_err(kbdev->dev, "Unknown GPU ID %x", gpu_id);
			mtk_get_clk_MFG_PWR();
			get_more_Mali_Register(kbdev);
			return MALI_ERROR_FUNCTION_FAILED;
		}
	}

	pr_alert("GPU identified as 0x%04x r%dp%d status %d", (gpu_id & GPU_ID_VERSION_PRODUCT_ID) >> GPU_ID_VERSION_PRODUCT_ID_SHIFT, (gpu_id & GPU_ID_VERSION_MAJOR) >> GPU_ID_VERSION_MAJOR_SHIFT, (gpu_id & GPU_ID_VERSION_MINOR) >> GPU_ID_VERSION_MINOR_SHIFT, (gpu_id & GPU_ID_VERSION_STATUS) >> GPU_ID_VERSION_STATUS_SHIFT);

	for (; *issues != BASE_HW_ISSUE_END; issues++)
		set_bit(*issues, &kbdev->hw_issues_mask[0]);

	return MALI_ERROR_NONE;
}
