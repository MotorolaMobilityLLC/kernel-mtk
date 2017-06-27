/*
 * Copyright (C) 2010 MediaTek, Inc.
 *
 * Author: Terry Chang <terry.chang@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "kpd.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>

#define KPD_NAME	"mtk-kpd"

void __iomem *kp_base;
static unsigned int kp_irqnr;
struct input_dev *kpd_input_dev;
static bool kpd_suspend;
static int kpd_show_hw_keycode = 1;

/*for kpd_memory_setting() function*/
static u16 kpd_keymap[KPD_NUM_KEYS];
static u16 kpd_keymap_state[KPD_NUM_MEMS];
struct keypad_dts_data kpd_dts_data;

/* for keymap handling */
static void kpd_keymap_handler(unsigned long data);
static DECLARE_TASKLET(kpd_keymap_tasklet, kpd_keymap_handler, 0);

/*********************************************************************/
static void kpd_memory_setting(void);

/*********************************************************************/
static int kpd_pdrv_probe(struct platform_device *pdev);
static int kpd_pdrv_remove(struct platform_device *pdev);
static int kpd_pdrv_suspend(struct platform_device *pdev, pm_message_t state);
static int kpd_pdrv_resume(struct platform_device *pdev);

static const struct of_device_id kpd_of_match[] = {
	{.compatible = "mediatek,mt2701-keypad"},
	{.compatible = "mediatek,mt7623-keypad"},
	{.compatible = "mediatek,mt2712-keypad"},
	{},
};

static struct platform_driver kpd_pdrv = {
	.probe = kpd_pdrv_probe,
	.remove = kpd_pdrv_remove,
	.suspend = kpd_pdrv_suspend,
	.resume = kpd_pdrv_resume,
	.driver = {
		.name = KPD_NAME,
		.owner = THIS_MODULE,
		.of_match_table = kpd_of_match,
	},
};

/********************************************************************/
static void kpd_memory_setting(void)
{
	kpd_init_keymap(kpd_keymap);
	kpd_init_keymap_state(kpd_keymap_state);
}

/*********************************************************************/
static void kpd_keymap_handler(unsigned long data)
{
	int i, j;
	bool pressed;
	u16 new_state[KPD_NUM_MEMS], change, mask;
	u16 hw_keycode, linux_keycode;

	kpd_get_keymap_state(new_state);

	for (i = 0; i < KPD_NUM_MEMS; i++) {
		change = new_state[i] ^ kpd_keymap_state[i];
		if (!change)
			continue;

		for (j = 0; j < 16; j++) {
			mask = 1U << j;
			if (!(change & mask))
				continue;

			hw_keycode = (i << 4) + j;

			if (hw_keycode >= KPD_NUM_KEYS)
				continue;

			/* bit is 1: not pressed, 0: pressed */
			pressed = !(new_state[i] & mask);
			if (kpd_show_hw_keycode)
				kpd_print("(%s) HW keycode = %u\n", pressed ? "pressed" : "released", hw_keycode);

			linux_keycode = kpd_keymap[hw_keycode];
			if (unlikely(linux_keycode == 0))
				continue;
			input_report_key(kpd_input_dev, linux_keycode, pressed);
			input_sync(kpd_input_dev);
			kpd_print("report Linux keycode = %u\n", linux_keycode);
		}
	}

	memcpy(kpd_keymap_state, new_state, sizeof(new_state));
	enable_irq(kp_irqnr);
}

static irqreturn_t kpd_irq_handler(int irq, void *dev_id)
{
	/* use _nosync to avoid deadlock */
	disable_irq_nosync(kp_irqnr);
	tasklet_schedule(&kpd_keymap_tasklet);
	return IRQ_HANDLED;
}

/*********************************************************************/

long kpd_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	/* void __user *uarg = (void __user *)arg; */

	switch (cmd) {

	case SET_KPD_KCOL:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int kpd_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations kpd_dev_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = kpd_dev_ioctl,
	.open = kpd_dev_open,
};

/*********************************************************************/
static struct miscdevice kpd_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = KPD_NAME,
	.fops = &kpd_dev_fops,
};

static int kpd_open(struct input_dev *dev)
{
	return 0;
}
void kpd_get_dts_info(struct device_node *node)
{
	int ret;

	memset(&kpd_dts_data, 0, sizeof(struct keypad_dts_data));
	of_property_read_u32(node, "mediatek,kpd-key-debounce", &kpd_dts_data.kpd_key_debounce);
	of_property_read_u32(node, "mediatek,kpd-use-extend-type", &kpd_dts_data.kpd_use_extend_type);
	of_property_read_u32(node, "mediatek,kpd-hw-map-num", &kpd_dts_data.kpd_hw_map_num);

	if (kpd_dts_data.kpd_hw_map_num > KPD_NUM_KEYS)
		kpd_dts_data.kpd_hw_map_num = KPD_NUM_KEYS;
	ret = of_property_read_u32_array(node, "mediatek,kpd-hw-init-map",
			kpd_dts_data.kpd_hw_init_map, kpd_dts_data.kpd_hw_map_num);

	if (ret) {
		kpd_print("kpd-hw-init-map was not defined in dts.\n");
		memset(kpd_dts_data.kpd_hw_init_map, 0, sizeof(kpd_dts_data.kpd_hw_init_map));
	}
}

static int kpd_gpio_init(struct device *dev)
{
	struct pinctrl *keypad_pinctrl;
	struct pinctrl_state *kpd_default;
	int ret;

	if (!dev) {
		kpd_print("kpd device is NULL!\n");
		return -1;
	}
	keypad_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(keypad_pinctrl)) {
		ret = PTR_ERR(keypad_pinctrl);
		kpd_print("Cannot find keypad_pinctrl!\n");
		return ret;
	}

	kpd_default = pinctrl_lookup_state(keypad_pinctrl, "default");
	if (IS_ERR(kpd_default)) {
		ret = PTR_ERR(kpd_default);
		kpd_print("Cannot find ecall_state!\n");
		return ret;
	}
	pinctrl_select_state(keypad_pinctrl, kpd_default);

	return 0;
}

static int kpd_pdrv_probe(struct platform_device *pdev)
{
	int i;
	int err = 0;
	struct clk *kpd_clk = NULL;

	kpd_clk = devm_clk_get(&pdev->dev, "kpd-clk");
	if (!IS_ERR(kpd_clk) && (kpd_clk != NULL)) {
		err = clk_prepare_enable(kpd_clk);
		if (err) {
			clk_disable_unprepare(kpd_clk);
			kpd_print("clock enable fail.\n");
			return -ENODEV;
		}
	} else {
		kpd_print("get kpd-clk fail.\n");
		return -ENODEV;
	}

	kp_base = of_iomap(pdev->dev.of_node, 0);
	if (!kp_base) {
		kpd_print("keypad iomap failed\n");
		return -ENODEV;
	};

	kp_irqnr = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!kp_irqnr) {
		kpd_print("KP get irq number failed\n");
		return -ENODEV;
	}
	kpd_info("kp base: 0x%p, kp irq: %d\n", kp_base, kp_irqnr);

	err = kpd_gpio_init(&pdev->dev);
	if (err) {
		kpd_print("gpio init failed\n");
		return -ENODEV;
	}

	/* initialize and register input device (/dev/input/eventX) */
	kpd_input_dev = input_allocate_device();
	if (!kpd_input_dev) {
		kpd_print("input allocate device fail.\n");
		return -ENOMEM;
	}

	kpd_input_dev->name = KPD_NAME;
	kpd_input_dev->id.bustype = BUS_HOST;
	kpd_input_dev->id.vendor = 0x2454;
	kpd_input_dev->id.product = 0x6500;
	kpd_input_dev->id.version = 0x0010;
	kpd_input_dev->open = kpd_open;

	kpd_get_dts_info(pdev->dev.of_node);
	kpd_memory_setting();

	__set_bit(EV_KEY, kpd_input_dev->evbit);

	for (i = 0; i < KPD_NUM_KEYS; i++) {
		if (kpd_keymap[i] != 0)
			__set_bit(kpd_keymap[i], kpd_input_dev->keybit);
	}

	kpd_input_dev->dev.parent = &pdev->dev;
	err = input_register_device(kpd_input_dev);
	if (err) {
		kpd_print("register input device failed (%d)\n", err);
		goto exit_input_reg_fail;
	}

	/* register device (/dev/mt6575-kpd) */
	kpd_dev.parent = &pdev->dev;
	err = misc_register(&kpd_dev);
	if (err) {
		kpd_print("register device failed (%d)\n", err);
		goto exit_misc_reg_fail;
	}

	if (kpd_dts_data.kpd_use_extend_type)
		kpd_double_key_enable(1);
	else
		kpd_double_key_enable(0);

	/* register IRQ and EINT */
	kpd_set_debounce(kpd_dts_data.kpd_key_debounce);
	err = request_irq(kp_irqnr, kpd_irq_handler, IRQF_TRIGGER_NONE, KPD_NAME, NULL);
	if (err) {
		kpd_print("register IRQ failed (%d)\n", err);
		goto exit_request_irq_fail;
	}

	kpd_info("%s Done\n", __func__);
	return 0;

exit_request_irq_fail:
	misc_deregister(&kpd_dev);
exit_misc_reg_fail:
	input_unregister_device(kpd_input_dev);
exit_input_reg_fail:
	input_free_device(kpd_input_dev);

	kpd_print("create attr file fail\n");

	return err;
}

/* should never be called */
static int kpd_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static int kpd_pdrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	kpd_suspend = true;
	kpd_enable(0);
	return 0;
}

static int kpd_pdrv_resume(struct platform_device *pdev)
{
	kpd_suspend = false;
	kpd_enable(1);

	return 0;
}

static int __init kpd_mod_init(void)
{
	int ret;

	ret = platform_driver_register(&kpd_pdrv);
	if (ret) {
		kpd_print("register driver failed (%d)\n", ret);
		return ret;
	}

	return 0;
}

/* should never be called */
static void __exit kpd_mod_exit(void)
{
}

module_init(kpd_mod_init);
module_exit(kpd_mod_exit);

module_param(kpd_show_hw_keycode, int, 0644);

MODULE_AUTHOR("yucong.xiong <yucong.xiong@mediatek.com>");
MODULE_DESCRIPTION("MTK Keypad (KPD) Driver v0.4");
MODULE_LICENSE("GPL");
