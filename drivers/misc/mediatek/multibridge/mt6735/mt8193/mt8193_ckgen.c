#define pr_fmt(fmt) "mt8193-ckgen: " fmt
#define DEBUG 1

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_PM_AUTOSLEEP
#include <linux/fb.h>
#include <linux/notifier.h>
#endif
#include "mt8193.h"
#include "mt8193_ckgen.h"
#include "mt8193_gpio.h"


#define MT8193_MLC 0
#define MT8193_CKGEN_VFY 1
#define MT8193_CKGEN_DEVNAME "mt8193-ckgen"

static int mt8193_ckgen_probe(struct platform_device *pdev);
static int mt8193_ckgen_suspend(struct platform_device *pdev, pm_message_t state);
static int mt8193_ckgen_resume(struct platform_device *pdev);
static int mt8193_ckgen_remove(struct platform_device *pdev);
static void mt8193_ckgen_shutdown(struct platform_device *pdev);

/******************************************************************************
Device driver structure
******************************************************************************/
#ifdef CONFIG_OF
static const struct of_device_id mt8193ckgen_of_ids[] = {
	{.compatible = "mediatek,mt8193-ckgen", },
	{}
};
#endif

static struct platform_driver mt8193_ckgen_driver = {
	.probe		= mt8193_ckgen_probe,
	.remove		= mt8193_ckgen_remove,
	.shutdown	= mt8193_ckgen_shutdown,
	.suspend	= mt8193_ckgen_suspend,
	.resume		= mt8193_ckgen_resume,
	.driver		= {
		.name	= "mt8193-ckgen",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt8193ckgen_of_ids,
#endif
	},
};

static struct class *ckgen_class;
static struct cdev *ckgen_cdev;

static struct pinctrl *pinctrl;
static struct pinctrl_state *pins_gpio;
static struct pinctrl_state *pins_dpi;

int multibridge_exit = 0;

#if MT8193_CKGEN_VFY

static int mt8193_ckgen_release(struct inode *inode, struct file *file);
static int mt8193_ckgen_open(struct inode *inode, struct file *file);
static long mt8193_ckgen_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static const struct file_operations mt8193_ckgen_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= mt8193_ckgen_ioctl,
	.open			= mt8193_ckgen_open,
	.release		= mt8193_ckgen_release,
};
#endif

void mt8193_ckgen_early_suspend(void)
{
	pr_err("[CKGEN] mt8193_ckgen_early_suspend() enter\n");

#if !MT8193_MLC

	/* If we use 8193 NFI, we should turn off pllgp in suspend because early suspend state may still use NFI.
		Otherwise, we can turn off pllgp in early suspend. */

	mt8193_pllgp_ana_pwr_control(false);
	msleep(20);

	mt8193_nfi_ana_pwr_control(false);

	/* bus clk switch to 32K */
	mt8193_bus_clk_switch(true);
	msleep(50);

#if MT8193_DISABLE_DCXO
	mt8193_disable_dcxo_core();
#endif

#endif

	pr_debug("[CKGEN] mt8193_ckgen_early_suspend() exit\n");
}

void mt8193_ckgen_late_resume(void)
{
	pr_err("[CKGEN] mt8193_ckgen_late_resume() enter\n");

#if !MT8193_MLC

	/* If we use 8193 NFI, we should turn off pllgp in suspend because early suspend state may still use NFI.
		Otherwise, we can turn off pllgp in early suspend. */

#if MT8193_DISABLE_DCXO
	mt8193_enable_dcxo_core();
	msleep(20);
#endif

	mt8193_bus_clk_switch(false);
	msleep(20);

	mt8193_nfi_ana_pwr_control(true);

	/* turn on pllgp analog */
	mt8193_pllgp_ana_pwr_control(true);
	msleep(20);
#endif

	pr_debug("[CKGEN] mt8193_ckgen_late_resume() exit\n");
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static struct early_suspend mt8193_ckgen_early_suspend_desc = {
	.level		= 0xFF,
	.suspend	= mt8193_ckgen_early_suspend,
	.resume		= mt8193_ckgen_late_resume,
};
#endif
#if 0
#ifdef CONFIG_PM_AUTOSLEEP
static int mt8193_ckgen_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *fb_evdata)
{
	struct fb_event *evdata = fb_evdata;
	int blank;

	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
		mt8193_ckgen_late_resume();
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		break;
	case FB_BLANK_POWERDOWN:
		mt8193_ckgen_early_suspend();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct notifier_block mt8193ckgen_fb_notif = {
	.notifier_call = mt8193_ckgen_fb_notifier_callback,
};
#endif
#endif

#if MT8193_CKGEN_VFY
static int mt8193_ckgen_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int mt8193_ckgen_open(struct inode *inode, struct file *file)
{
	return 0;
}

static char *_mt8193_ckgen_ioctl_spy(unsigned int cmd)
{
	switch (cmd) {
	case MTK_MT8193_CKGEN_1:
		return "MTK_MT8193_CKGEN_1";
	case MTK_MT8193_CKGEN_2:
		return "MTK_MT8193_CKGEN_2";
	case MTK_MT8193_CKGEN_SPM_CTRL:
		return "MTK_MT8193_CKGEN_SPM_CTRL";
	case MTK_MT8193_CKGEN_LS_TEST:
		return "MTK_MT8193_CKGEN_LS_TEST";
	case MTK_MT8193_CKGEN_FREQ_METER:
		return "MTK_MT8193_CKGEN_FREQ_METER";
	default:
		return "unknown ioctl command";
	}
}

static long mt8193_ckgen_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int r = 0;

	pr_debug("[CKGEN] cmd=%s, arg=0x%08lx\n", _mt8193_ckgen_ioctl_spy(cmd), arg);

	switch (cmd) {
	case MTK_MT8193_CKGEN_1:
		break;
	case MTK_MT8193_CKGEN_2:
		break;
	case MTK_MT8193_CKGEN_LS_TEST:
	{
		struct mt8193_ckgen_ls_info_t tLsInfo;

		if (copy_from_user(&tLsInfo, (void __user *)arg, sizeof(tLsInfo))) {
			pr_err("[CKGEN] copy_from_user fails!!\n");
			return -1;
		}

		r = mt8193_ckgen_config_pad_level_shift(tLsInfo.i4GroupNum, tLsInfo.i4TurnLow);

		break;
	}
	case MTK_MT8193_CKGEN_SPM_CTRL:
	{
		int u4Func = 0;

		u4Func = arg;
		mt8193_spm_control_test(u4Func);
		break;
	}

	case MTK_MT8193_CKGEN_FREQ_METER:
	{
		struct mt8193_ckgen_freq_meter_t t_freq;
		u32 u4Clk = 0;

		if (copy_from_user(&t_freq, (void __user *)arg, sizeof(t_freq))) {
			pr_err("[CKGEN] copy_from_user fails!!\n");
			return -1;
		}

		u4Clk = mt8193_ckgen_measure_clk(t_freq.u4Func);
		break;
	}
	case MTK_MT8193_GPIO_CTRL:
	{
		struct mt8193_gpio_ctrl_t t_gpio;
		u32 u4Val = 0;

		if (copy_from_user(&t_gpio, (void __user *)arg, sizeof(t_gpio))) {
			pr_err("[CKGEN] copy_from_user fails!!\n");
			return -1;
		}

		if (t_gpio.u4Mode == MT8193_GPIO_OUTPUT) {
			GPIO_Output(t_gpio.u4GpioNum, t_gpio.u4Value);
		} else {
			GPIO_Config(t_gpio.u4GpioNum, MT8193_GPIO_INPUT, 0);
			u4Val = GPIO_Input(t_gpio.u4GpioNum);
			pr_debug("[CKGEN] GPIO INPUT VALUE IS %d\n", u4Val);
		}

		break;
	}
	case MTK_MT8193_EARLY_SUSPEND:
		break;
	case MTK_MT8193_LATE_RESUME:
		break;
	default:
		pr_err("[CKGEN] arguments error\n");
		break;
	}

	return r;
}

#endif

/******************************************************************************
 * mt8193_ckgen_init
 *
 * DESCRIPTION:
 *   Init the device driver !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static int __init mt8193_ckgen_init(void)
{
	int ret = 0;

	pr_debug("[CKGEN] mt8193_ckgen_init() enter\n");
	ret = platform_driver_register(&mt8193_ckgen_driver);
	if (ret) {
		pr_err("fail to register 8193ckgen driver, ret=%d\n", ret);
		return ret;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND)
	register_early_suspend(&mt8193_ckgen_early_suspend_desc);
#endif

#if 0
#ifdef CONFIG_PM_AUTOSLEEP
	ret = fb_register_client(&mt8193ckgen_fb_notif);
	if (ret) {
		pr_err("fail to register fb notifier, ret=%d\n", ret);
		return ret;
	}
#endif
#endif
	pr_debug("[CKGEN] mt8193_ckgen_init() exit\n");

	return 0;
}

/******************************************************************************
 * mt8193_ckgen_exit
 *
 * DESCRIPTION:
 *   Free the device driver !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void __exit mt8193_ckgen_exit(void)
{
	devm_pinctrl_put(pinctrl);
	platform_driver_unregister(&mt8193_ckgen_driver);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&mt8193_ckgen_early_suspend_desc);
#endif
}

static dev_t mt8193_ckgen_devno;
static struct cdev *mt8193_ckgen_cdev;

/******************************************************************************
 * mt8193_ckgen_probe
 *
 * DESCRIPTION:
 *   register the nand device file operations !
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static int mt8193_ckgen_probe(struct platform_device *pdev)
{
#if MT8193_CKGEN_VFY
	int ret = 0;

	pr_err("[CKGEN] %s\n", __func__);
	/* Allocate device number for hdmi driver */
	ret = alloc_chrdev_region(&mt8193_ckgen_devno, 0, 1, MT8193_CKGEN_DEVNAME);
	if (ret) {
		pr_err("[CKGEN] alloc_chrdev_region fail\n");
		return -1;
	}

	/* For character driver register to system, device number binded to file operations */
	mt8193_ckgen_cdev = cdev_alloc();
	mt8193_ckgen_cdev->owner = THIS_MODULE;
	mt8193_ckgen_cdev->ops = &mt8193_ckgen_fops;
	ret = cdev_add(mt8193_ckgen_cdev, mt8193_ckgen_devno, 1);

	/* For device number binded to device name(hdmitx), one class is corresponeded to one node */
	ckgen_class = class_create(THIS_MODULE, MT8193_CKGEN_DEVNAME);
	/* mknod /dev/hdmitx */
	ckgen_cdev = (struct cdev *)device_create(ckgen_class, NULL, mt8193_ckgen_devno, NULL, MT8193_CKGEN_DEVNAME);

	pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		pr_err("Cannot find pinctrl, ret=%d!\n", ret);
		return ret;
	}

	pins_gpio = pinctrl_lookup_state(pinctrl, "bus_switch_gpio");
	if (IS_ERR(pins_gpio)) {
		ret = PTR_ERR(pins_gpio);
		pr_err("Cannot find bus_switch_gpio, ret=%d!\n", ret);
		return ret;
	}

	pins_dpi = pinctrl_lookup_state(pinctrl, "bus_switch_dpi");
	if (IS_ERR(pins_dpi)) {
		ret = PTR_ERR(pins_dpi);
		pr_err("Cannot find bus_switch_dpi, ret=%d!\n", ret);
		return ret;
	}
#endif
	return 0;
}

/******************************************************************************
 * mt8193_ckgen_remove
 *
 * DESCRIPTION:
 *   unregister the nand device file operations !
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/

static int mt8193_ckgen_remove(struct platform_device *pdev)
{
	return 0;
}

static void mt8193_ckgen_shutdown(struct platform_device *pdev)
{
	if (!multibridge_exit) {
		multibridge_exit = 1;
		mt8193_bus_clk_switch(false);
	}
}

module_init(mt8193_ckgen_init);
module_exit(mt8193_ckgen_exit);

int mt8193_CKGEN_AgtOnClk(enum e_CLK_T eAgt)
{
	u32 u4Tmp;

	pr_debug("mt8193_CKGEN_AgtOnClk() %d\n", eAgt);

	switch (eAgt) {
	case e_CLK_NFI:
		u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
		CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp & (~CLK_PDN_NFI));
		break;
	case e_CLK_HDMIPLL:
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
		CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp & (~CLK_PDN_HDMI_PLL));
		break;
	case e_CLK_HDMIDISP:
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
		CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp & (~CLK_PDN_HDMI_DISP));
		break;
	case e_CLK_LVDSDISP:
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
		CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp & (~CLK_PDN_LVDS_DISP));
		break;
	case e_CLK_LVDSCTS:
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
		CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp & (~CLK_PDN_LVDS_CTS));
		break;
	default:
		return -1;
	}

	return 0;
}

int mt8193_CKGEN_AgtOffClk(enum e_CLK_T eAgt)
{
	u32 u4Tmp;

	pr_debug("mt8193_CKGEN_AgtOffClk() %d\n", eAgt);

	switch (eAgt) {
	case e_CLK_NFI:
		u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
		CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp | CLK_PDN_NFI);
		break;
	case e_CLK_HDMIPLL:
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
		CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp | CLK_PDN_HDMI_PLL);
		break;
	case e_CLK_HDMIDISP:
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
		CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp | CLK_PDN_HDMI_DISP);
		break;
	case e_CLK_LVDSDISP:
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
		CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | CLK_PDN_LVDS_DISP);
		break;
	case e_CLK_LVDSCTS:
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
		CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | CLK_PDN_LVDS_CTS);
		break;
	default:
		return -1;
	}

	return 0;
}

int mt8193_CKGEN_AgtSelClk(enum e_CLK_T eAgt, u32 u4Sel)
{
	u32 u4Tmp;

	pr_debug("mt8193_CKGEN_AgtSelClk() %d\n", eAgt);

	switch (eAgt) {
	case e_CLK_NFI:
		u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
		CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp | u4Sel);
		break;
	case e_CLK_HDMIPLL:
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
		CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp | u4Sel);
		break;
	case e_CLK_HDMIDISP:
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
		CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp | u4Sel);
		break;
	case e_CLK_LVDSDISP:
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
		CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | u4Sel);
		break;
	case e_CLK_LVDSCTS:
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
		CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | u4Sel);
		break;
	default:
		return -1;
	}

	return 0;
}

u32 mt8193_CKGEN_AgtGetClk(enum e_CLK_T eAgt)
{
	return 0;
}

int mt8193_ckgen_i2c_write(u16 addr, u32 data)
{
	u32 u4_ret = 0;

	pr_debug("mt8193_ckgen_i2c_write() 0x%x; 0x%x\n", addr, data);
	u4_ret = mt8193_i2c_write(addr, data);
	if (u4_ret != 0)
		pr_err("mt8193_i2c_read() fails!!!!!!\n");

	return 0;
}

u32 mt8193_ckgen_i2c_read(u16 addr)
{
	u32 u4_val = 0;
	u32 u4_ret = 0;

	u4_ret = mt8193_i2c_read(addr, &u4_val);
	if (u4_ret != 0)
		pr_err("mt8193_i2c_read() fails!!!!!!\n");

	pr_debug("mt8193_ckgen_i2c_read() 0x%x; value is 0x%x\n", addr, u4_val);
	return u4_val;
}

/* freq meter measure clock */
u32 mt8193_ckgen_measure_clk(u32 u4Func)
{
	u32 ui4_delay_cnt = 0x400;
	u32 ui4_result = 0;

	pr_debug("[CKGEN] mt8193_ckgen_measure_clk() %d\n", u4Func);

	/* select source */
	CKGEN_WRITE32(REG_RW_FMETER, (CKGEN_READ32(REG_RW_FMETER)&(~(0xFF<<3)))|(u4Func<<3));
	/* start fmeter */
	CKGEN_WRITE32(REG_RW_FMETER, (CKGEN_READ32(REG_RW_FMETER)|CKGEN_FMETER_RESET));
	/* wait until fmeter done */
	do {
		ui4_delay_cnt--;
	} while ((!(CKGEN_READ32(REG_RW_FMETER)&CKGEN_FMETER_DONE)) && ui4_delay_cnt);

	ui4_result = CKGEN_READ32(REG_RW_FMETER)>>16;

	pr_debug("[CKGEN] Measure Done CLK [0X%X] for func [%d] delay count [0x%X]\n",
			ui4_result, u4Func, ui4_delay_cnt);

	return ui4_result;
}

void mt8193_lvds_ana_pwr_control(bool power_on)
{
	u32 u4Tmp = 0;

	if (power_on) {
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_ANACFG4);
		u4Tmp &= (~LVDS_ANACFG4_VPlLL_PD);
		CKGEN_WRITE32(REG_RW_LVDS_ANACFG4, u4Tmp);
	} else {
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_ANACFG4);
		u4Tmp |= (LVDS_ANACFG4_VPlLL_PD);
		CKGEN_WRITE32(REG_RW_LVDS_ANACFG4, u4Tmp);
	}
}

void mt8193_hdmi_ana_pwr_control(bool power_on)
{
	u32 u4Tmp = 0;

	if (power_on) {
		u4Tmp = CKGEN_READ32(REG_RW_HDMITX_ANACFG3);
		u4Tmp |= (HDMITX_ANACFG3_BIT20);
		u4Tmp |= (HDMITX_ANACFG3_BIT21);
		CKGEN_WRITE32(REG_RW_HDMITX_ANACFG3, u4Tmp);
	} else {
		u4Tmp = CKGEN_READ32(REG_RW_HDMITX_ANACFG3);
		u4Tmp &= (~HDMITX_ANACFG3_BIT20);
		u4Tmp &= (~HDMITX_ANACFG3_BIT21);
		CKGEN_WRITE32(REG_RW_HDMITX_ANACFG3, u4Tmp);
	}
}

void mt8193_pllgp_ana_pwr_control(bool power_on)
{
	u32 u4Tmp = 0;

	if (power_on) {
		u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
		u4Tmp |= (PLLGP_ANACFG0_PLL1_EN);
		CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);
	} else {
		u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
		u4Tmp &= (~PLLGP_ANACFG0_PLL1_EN);
		CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);
	}
}

void mt8193_nfi_ana_pwr_control(bool power_on)
{
	u32 u4Tmp = 0;

	pr_debug("[CKGEN] mt8193_nfi_ana_pwr_control() %d\n", power_on);
	if (power_on) {
		u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG2);
		u4Tmp |= (PLLGP_ANACFG2_PLLGP_BIAS_EN);
		CKGEN_WRITE32(REG_RW_PLLGP_ANACFG2, u4Tmp);

		u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
		u4Tmp |= (PLLGP_ANACFG0_PLL1_NFIPLL_EN);
		CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);
	} else {
		u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
		u4Tmp &= (~PLLGP_ANACFG0_PLL1_NFIPLL_EN);
		CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);
		msleep(20);
		u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG2);
		u4Tmp &= (~PLLGP_ANACFG2_PLLGP_BIAS_EN);
		CKGEN_WRITE32(REG_RW_PLLGP_ANACFG2, u4Tmp);
		msleep(20);
	}
}

void mt8193_lvds_sys_spm_control(bool power_on)
{
	u32 u4Tmp = 0;
	u32 u4Tmp2 = 0;
	u32 ui4_delay_cnt = 0x40000;

	if (power_on) {
		/* turn on power */
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_PWR_CTRL);
		u4Tmp |= CKGEN_LVDS_PWR_PWR_ON;
		CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

		/* disable reset */
		u4Tmp2 = CKGEN_READ32(REG_RW_LVDS_PWR_RST_B);
		u4Tmp2 |= CKGEN_LVDS_PWR_RST_EN;
		CKGEN_WRITE32(REG_RW_LVDS_PWR_RST_B, u4Tmp2);

		/* disable iso */
		u4Tmp &= (~CKGEN_LVDS_PWR_ISO_EN);
		CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

		/* enable clock */
		u4Tmp &= (~CKGEN_LVDS_PWR_CLK_OFF);
		CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

		/* wait until pwr act */
		do {
			ui4_delay_cnt--;
		} while ((!(CKGEN_READ32(REG_RO_PWR_ACT)&CKGEN_LVDS_PWR_ON_ACT)) && ui4_delay_cnt);

		if (ui4_delay_cnt == 0)
			pr_err("[CKGEN] Did not get power act for LVDS!!!!\n");
	} else {
		/* disable clock */
		u4Tmp = CKGEN_READ32(REG_RW_LVDS_PWR_CTRL);
		u4Tmp |= CKGEN_LVDS_PWR_CLK_OFF;
		CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

		/* enable iso */
		u4Tmp |= CKGEN_LVDS_PWR_ISO_EN;
		CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

		/* enable reset */
		u4Tmp2 = CKGEN_READ32(REG_RW_LVDS_PWR_RST_B);
		u4Tmp2 &= (~CKGEN_LVDS_PWR_RST_EN);
		CKGEN_WRITE32(REG_RW_LVDS_PWR_RST_B, u4Tmp2);

		/* turn off power */
		u4Tmp &= (~CKGEN_LVDS_PWR_PWR_ON);
		CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);
	}
}

void mt8193_hdmi_sys_spm_control(bool power_on)
{
	u32 u4Tmp = 0;
	u32 u4Tmp2 = 0;
	u32 ui4_delay_cnt = 0x40000;

	if (power_on) {
		/* turn on power */
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_PWR_CTRL);
		u4Tmp |= CKGEN_HDMI_PWR_PWR_ON;
		CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

		/* disable reset */
		u4Tmp2 = CKGEN_READ32(REG_RW_HDMI_PWR_RST_B);
		u4Tmp2 |= CKGEN_HDMI_PWR_RST_EN;
		CKGEN_WRITE32(REG_RW_HDMI_PWR_RST_B, u4Tmp2);

		/* disable iso */
		u4Tmp &= (~CKGEN_HDMI_PWR_ISO_EN);
		CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

		/* enable clock */
		u4Tmp &= (~CKGEN_HDMI_PWR_CLK_OFF);
		CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

		/* wait until pwr act */
		do {
			ui4_delay_cnt--;
		} while ((!(CKGEN_READ32(REG_RO_PWR_ACT)&CKGEN_HDMI_PWR_ON_ACT)) && ui4_delay_cnt);

		if (ui4_delay_cnt == 0)
			pr_err("[CKGEN] Did not get power act for HDMI!!!!\n");
	} else {
		/* disable clock */
		u4Tmp = CKGEN_READ32(REG_RW_HDMI_PWR_CTRL);
		u4Tmp |= CKGEN_HDMI_PWR_CLK_OFF;
		CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

		/* enable iso */
		u4Tmp |= CKGEN_HDMI_PWR_ISO_EN;
		CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

		/* enable reset */
		u4Tmp2 = CKGEN_READ32(REG_RW_HDMI_PWR_RST_B);
		u4Tmp2 &= (~CKGEN_HDMI_PWR_RST_EN);
		CKGEN_WRITE32(REG_RW_HDMI_PWR_RST_B, u4Tmp2);

		/* turn off power */
		u4Tmp &= (~CKGEN_HDMI_PWR_PWR_ON);
		CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);
	}
}

void mt8193_nfi_sys_spm_control(bool power_on)
{
	u32 u4Tmp = 0;
	u32 u4Tmp2 = 0;
	u32 ui4_delay_cnt = 0x40000;

	if (power_on) {
		/* turn on power */
		u4Tmp = CKGEN_READ32(REG_RW_NFI_PWR_CTRL);
		u4Tmp |= CKGEN_NFI_PWR_PWR_ON;
		CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

		/* disable reset */
		u4Tmp2 = CKGEN_READ32(REG_RW_NFI_PWR_RST_B);
		u4Tmp2 |= CKGEN_NFI_PWR_RST_EN;
		CKGEN_WRITE32(REG_RW_NFI_PWR_RST_B, u4Tmp2);

		/* disable iso */
		u4Tmp &= (~CKGEN_NFI_PWR_ISO_EN);
		CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

		/* enable clock */
		u4Tmp &= (~CKGEN_NFI_PWR_CLK_OFF);
		CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

		/* wait until pwr act */
		do {
			ui4_delay_cnt--;
		} while ((!(CKGEN_READ32(REG_RO_PWR_ACT)&CKGEN_NFI_PWR_ON_ACT)) && ui4_delay_cnt);

		if (ui4_delay_cnt == 0)
			pr_err("[CKGEN] Did not get power act for NFI!!!!\n");
	} else {
		/* disable clock */
		u4Tmp = CKGEN_READ32(REG_RW_NFI_PWR_CTRL);
		u4Tmp |= CKGEN_NFI_PWR_CLK_OFF;
		CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

		/* enable iso */
		u4Tmp |= CKGEN_NFI_PWR_ISO_EN;
		CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

		/* enable reset */
		u4Tmp2 = CKGEN_READ32(REG_RW_NFI_PWR_RST_B);
		u4Tmp2 &= (~CKGEN_NFI_PWR_RST_EN);
		CKGEN_WRITE32(REG_RW_NFI_PWR_RST_B, u4Tmp2);

		/* turn off power */
		u4Tmp &= (~CKGEN_NFI_PWR_PWR_ON);
		CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);
	}
}

void mt8193_bus_clk_switch(bool bus_26m_to_32k)
{
	u32 u4Tmp = 0;
	struct device_node *dn;
	int bus_switch_pin;
	int ret;

	dn = of_find_compatible_node(NULL, NULL, "mediatek,mt8193-ckgen");
	bus_switch_pin = of_get_named_gpio(dn, "bus_switch_pin", 0);
	ret = gpio_request(bus_switch_pin, "8193 bus switch pin");
	if (ret) {
		pr_err("request gpio fail, ret=%d\n", ret);
		return;
	}

	if (bus_26m_to_32k) {
		/* bus clock switch from 26M to 32K */
		/* sequence: out -> dir -> select -> enable -> mode */
#if 0
		mt_set_gpio_out(GPIO_MT8193_BUS_SWITCH_PIN, GPIO_OUT_ONE);
		mt_set_gpio_dir(GPIO_MT8193_BUS_SWITCH_PIN, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO_MT8193_BUS_SWITCH_PIN, GPIO_PULL_UP);
		mt_set_gpio_pull_enable(GPIO_MT8193_BUS_SWITCH_PIN, GPIO_PULL_ENABLE);
		mt_set_gpio_mode(GPIO_MT8193_BUS_SWITCH_PIN, MT8193_BUS_SWITCH_PIN_GPIO_MODE);
#else
		pinctrl_select_state(pinctrl, pins_gpio);
#endif

		u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG9);
		u4Tmp &= (~(DCXO_ANACFG9_BUS_CK_SOURCE_SEL_MASK << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT));
#if defined(USING_MT8193_DPI1) && USING_MT8193_DPI1
		u4Tmp |= (6 << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT);
#else
		u4Tmp |= (3 << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT);
#endif
		/* using a GPIO as auto switch source */
		u4Tmp &= (~DCX0_ANACFG9_BUS_CK_CTRL_SEL);
		/* enable bus_ck auto switch function */
		u4Tmp |= (DCX0_ANACFG9_BUS_CK_AUTO_SWITCH_EN);
		pr_debug("[early_suspend] u4Tmp=0x%x\n", u4Tmp);
		CKGEN_WRITE32(REG_RW_DCXO_ANACFG9, u4Tmp);
#if 0
		mt_set_gpio_out(GPIO_MT8193_BUS_SWITCH_PIN, GPIO_OUT_ZERO);
#else
		gpio_set_value(bus_switch_pin, 0);
#endif
		msleep(20);
		/* verify: reading register must fail if switch clock success */
		u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG9);
	} else {
		/* bus clock switch from 32K to 26M */
#if 0
		mt_set_gpio_out(GPIO_MT8193_BUS_SWITCH_PIN, GPIO_OUT_ONE);
#else
		gpio_set_value(bus_switch_pin, 1);
#endif
		msleep(20);
		u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG9);
		u4Tmp &= (~(DCXO_ANACFG9_BUS_CK_SOURCE_SEL_MASK << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT));
		pr_debug("[late_resume] u4Tmp=0x%x\n", u4Tmp);
		CKGEN_WRITE32(REG_RW_DCXO_ANACFG9, u4Tmp);
#if 0
		mt_set_gpio_mode(GPIO_MT8193_BUS_SWITCH_PIN, MT8193_BUS_SWITCH_PIN_DPI_MODE);
#else
		pinctrl_select_state(pinctrl, pins_dpi);
#endif
	}

	gpio_free(bus_switch_pin);
}

#if 0
void mt8193_bus_clk_switch_to_26m(void)
{
	u32 u4Tmp = 0;

	pr_debug(" mt8193_bus_clk_switch_to_26m()\n");

	/* bus clock switch from 32K to 26M */

	mt_set_gpio_out(GPIO_MT8193_BUS_SWITCH_PIN, GPIO_OUT_ONE);

	mdelay(20);

	u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG9);
	u4Tmp &= (~(DCXO_ANACFG9_BUS_CK_SOURCE_SEL_MASK << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT));
	CKGEN_WRITE32(REG_RW_DCXO_ANACFG9, u4Tmp);

	mt_set_gpio_mode(GPIO_MT8193_BUS_SWITCH_PIN, MT8193_BUS_SWITCH_PIN_DPI_MODE);
}
#endif

#if MT8193_DISABLE_DCXO

/* disable dcxo ldo1, 8193 core clock buffer */
void mt8193_disable_dcxo_core(void)
{
	u32 u4Tmp = 0;

	pr_debug("mt8193_disable_dcxo_core()\n");

	/* set bt clock buffer manual mode */
	u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG2);
	u4Tmp &= (~DCXO_ANACFG2_PO_MAN);
	CKGEN_WRITE32(REG_RW_DCXO_ANACFG2, u4Tmp);

	/* disable dcxo ldo2 at manual mode */
	u4Tmp &= (~DCXO_ANACFG2_LDO1_MAN_EN);
	CKGEN_WRITE32(REG_RW_DCXO_ANACFG2, u4Tmp);

	/* disable dcxo ldo2*/
	u4Tmp &= (~DCXO_ANACFG2_LDO1_EN);
	CKGEN_WRITE32(REG_RW_DCXO_ANACFG2, u4Tmp);
}

/* enable dcxo ldo1, 8193 core clock buffer */
void mt8193_enable_dcxo_core(void)
{
	u32 u4Tmp = 0;

	pr_debug("mt8193_enable_dcxo_core()\n");

	/* disable dcxo ldo2*/
	u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG2);
	u4Tmp |= DCXO_ANACFG2_LDO1_EN;
	CKGEN_WRITE32(REG_RW_DCXO_ANACFG2, u4Tmp);

	/* disable dcxo ldo2 at manual mode */
	u4Tmp |= DCXO_ANACFG2_LDO1_MAN_EN;
	CKGEN_WRITE32(REG_RW_DCXO_ANACFG2, u4Tmp);

	/* set bt clock buffer manual mode */

	u4Tmp |= DCXO_ANACFG2_PO_MAN;
	CKGEN_WRITE32(REG_RW_DCXO_ANACFG2, u4Tmp);
}

#endif

#if 0
void mt8193_en_bb_ctrl(bool pd)
{
	/* GPIO60 is EN_BB */
	/* GPIO59 is CK_SEL */
	if (pd) {
		/* pull low */
		mt_set_gpio_out(GPIO60, 0);
		mt_set_gpio_out(GPIO59, 0);
	} else {
		/* pull high */
		mt_set_gpio_out(GPIO59, 1);
		mt_set_gpio_out(GPIO60, 1);
	}
}
#endif

/******************************************************************************
 * mt8193_ckgen_suspend
 *
 * DESCRIPTION:
 *   Suspend the nand device!
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static int mt8193_ckgen_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_debug("[CKGEN] mt8193_ckgen_suspend() enter\n");

#if MT8193_MLC

	/* If we use 8193 NFI, we should turn off pllgp in suspend because early suspend state may still use NFI.
		Otherwise, we can turn off pllgp in early suspend. */

	/* add 8193 suspend function here */

	mt8193_pllgp_ana_pwr_control(false);
	msleep(20);

	/* bus clk switch to 32K */
	mt8193_bus_clk_switch(true);
	msleep(50);
#endif
	pr_debug("[CKGEN] mt8193_ckgen_suspend() exit\n");

	return 0;
}
/******************************************************************************
 * mt8193_ckgen_resume
 *
 * DESCRIPTION:
 *   Resume the nand device!
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static int mt8193_ckgen_resume(struct platform_device *pdev)
{
	pr_debug("[CKGEN] mt8193_ckgen_resume() enter\n");

	/* If we use 8193 NFI, we should turn off pllgp in suspend because early suspend state may still use NFI.
		Otherwise, we can turn off pllgp in early suspend. */

#if MT8193_MLC
	/* add 8193 resume function here */
	/* bus clk switch to 26M */
	mt8193_bus_clk_switch(false);
	msleep(20);
	/* turn on pllgp analog */
	mt8193_pllgp_ana_pwr_control(true);
	msleep(20);
#endif
	pr_debug("[CKGEN] mt8193_ckgen_resume() exit\n");

	return 0;
}
