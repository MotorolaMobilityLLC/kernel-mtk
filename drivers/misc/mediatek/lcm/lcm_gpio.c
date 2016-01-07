#include <linux/string.h>
#include <linux/wait.h>

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/upmu_hw.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>	/* hwPowerOn */
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#endif
#endif
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif

#include "lcm_define.h"
#include "lcm_drv.h"
#include "lcm_gpio.h"


#if 0
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN
#endif


static LCM_STATUS _lcm_gpio_check_data(char type, const LCM_DATA_T1 *t1)
{
	switch (type) {
	case LCM_GPIO_MODE:
		switch (t1->data) {
		case LCM_GPIO_MODE_00:
		case LCM_GPIO_MODE_01:
		case LCM_GPIO_MODE_02:
		case LCM_GPIO_MODE_03:
		case LCM_GPIO_MODE_04:
		case LCM_GPIO_MODE_05:
		case LCM_GPIO_MODE_06:
		case LCM_GPIO_MODE_07:
			break;

		default:
			pr_debug("[LCM][ERROR] %s/%d: %d, %d\n", __func__, __LINE__, type, t1->data);
			return LCM_STATUS_ERROR;
		}
		break;

	case LCM_GPIO_DIR:
		switch (t1->data) {
		case LCM_GPIO_DIR_IN:
		case LCM_GPIO_DIR_OUT:
			break;

		default:
			pr_debug("[LCM][ERROR] %s/%d: %d, %d\n", __func__, __LINE__, type, t1->data);
			return LCM_STATUS_ERROR;
		}
		break;

	case LCM_GPIO_OUT:
		switch (t1->data) {
		case LCM_GPIO_OUT_ZERO:
		case LCM_GPIO_OUT_ONE:
			break;

		default:
			pr_debug("[LCM][ERROR] %s/%d: %d, %d\n", __func__, __LINE__, type, t1->data);
			return LCM_STATUS_ERROR;
		}
		break;

	default:
		pr_debug("[LCM][ERROR] %s/%d: %d\n", __func__, __LINE__, type);
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


LCM_STATUS lcm_gpio_set_data(char type, const LCM_DATA_T1 *t1)
{
	/* check parameter is valid */
	if (LCM_STATUS_OK == _lcm_gpio_check_data(type, t1)) {
		switch (type) {
#ifdef CONFIG_MTK_LEGACY
		case LCM_GPIO_MODE:
			mt_set_gpio_mode(GPIO_65132_EN, (unsigned int)t1->data);
			break;

		case LCM_GPIO_DIR:
			mt_set_gpio_dir(GPIO_65132_EN, (unsigned int)t1->data);
			break;

		case LCM_GPIO_OUT:
			mt_set_gpio_out(GPIO_65132_EN, (unsigned int)t1->data);
			break;
#endif
		default:
			pr_debug("[LCM][ERROR] %s/%d: %d\n", __func__, __LINE__, type);
			return LCM_STATUS_ERROR;
		}
	} else {
		pr_debug("[LCM][ERROR] %s: 0x%x, 0x%x\n", __func__, type, t1->data);
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}
