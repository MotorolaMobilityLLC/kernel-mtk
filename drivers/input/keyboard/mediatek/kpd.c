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
static int32_t kpd_show_hw_keycode = 1;

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
	u16 i, j;
	int32_t pressed;
	u16 new_state[KPD_NUM_MEMS], change, mask;
	u16 hw_keycode, linux_keycode;
	void *dest;

	kpd_get_keymap_state(new_state);

	for (i = 0; i < KPD_NUM_MEMS; i++) {
		change = new_state[i] ^ kpd_keymap_state[i];
		if (change == 0U)
			continue;

		for (j = 0; j < 16U; j++) {
			mask = (u16)1 << j;
			if ((change & mask) == 0U)
				continue;

			hw_keycode = (i << 4) + j;

			if (hw_keycode >= KPD_NUM_KEYS)
				continue;

			/* bit is 1: not pressed, 0: pressed */
			pressed = ((new_state[i] & mask) == 0U) ? 1:0;
			if (kpd_show_hw_keycode != 0)
				kpd_print("(%s) HW keycode = %d\n",
					(pressed == 1) ? "pressed" : "released", hw_keycode);

			linux_keycode = kpd_keymap[hw_keycode];
			if (linux_keycode == 0U)
				continue;
			input_report_key(kpd_input_dev, linux_keycode, pressed);
			input_sync(kpd_input_dev);
			kpd_print("report Linux keycode = %d\n", linux_keycode);
		}
	}

	dest = memcpy(kpd_keymap_state, new_state, sizeof(new_state));
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

static long kpd_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	/* void __user *uarg = (void __user *)arg; */
	return 0;
}

static int kpd_dev_open(struct inode *inode, struct file *file)
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
static void kpd_get_dts_info(struct device_node *node)
{
	int32_t ret, i;
	u32 temp;
	u32 map[KPD_NUM_KEYS] = {0};
	void *dest;

	dest = memset(&kpd_dts_data, 0, sizeof(struct keypad_dts_data));
	ret = of_property_read_u32(node, "mediatek,kpd-key-debounce", &temp);
	if (ret == 0)
		kpd_dts_data.kpd_key_debounce = (u16)temp;
	ret += of_property_read_u32(node, "mediatek,kpd-use-extend-type", &temp);
	if (ret == 0)
		kpd_dts_data.kpd_use_extend_type = (u16)temp;
	ret += of_property_read_u32(node, "mediatek,kpd-hw-map-num", &temp);
	if (ret == 0)
		kpd_dts_data.kpd_hw_map_num = (u16)temp;

	if (kpd_dts_data.kpd_hw_map_num > KPD_NUM_KEYS)
		kpd_dts_data.kpd_hw_map_num = KPD_NUM_KEYS;
	ret += of_property_read_u32_array(node, "mediatek,kpd-hw-init-map",
			map, kpd_dts_data.kpd_hw_map_num);
	if (ret == 0) {
		for (i = 0; i < kpd_dts_data.kpd_hw_map_num; i++)
			kpd_dts_data.kpd_hw_init_map[i] = (u16)map[i];
	}

	kpd_print("dts info %d, %d\n", kpd_dts_data.kpd_hw_map_num, kpd_dts_data.kpd_hw_init_map[0]);

	if (ret != 0) {
		kpd_print("kpd-hw-init-map was not defined in dts.\n");
		dest = memset(kpd_dts_data.kpd_hw_init_map, 0, sizeof(kpd_dts_data.kpd_hw_init_map));
	}
}

static int32_t kpd_gpio_init(struct device *dev)
{
	struct pinctrl *keypad_pinctrl;
	struct pinctrl_state *kpd_default;
	int32_t ret;

	if (dev == NULL) {
		kpd_print("kpd device is NULL!\n");
		ret = -1;
	} else {
		keypad_pinctrl = devm_pinctrl_get(dev);
		if (IS_ERR(keypad_pinctrl)) {
			ret = -1;
			kpd_print("Cannot find keypad_pinctrl!\n");
		} else {
			kpd_default = pinctrl_lookup_state(keypad_pinctrl, "default");
			if (IS_ERR(kpd_default)) {
				ret = -1;
				kpd_print("Cannot find ecall_state!\n");
			} else
				ret = pinctrl_select_state(keypad_pinctrl, kpd_default);
		}
	}
	return ret;
}

static int kpd_pdrv_probe(struct platform_device *pdev)
{
	u16 i;
	int32_t err = 0;
	struct clk *kpd_clk = NULL;

	kpd_clk = devm_clk_get(&pdev->dev, "kpd-clk");
	if (!IS_ERR(kpd_clk) && (kpd_clk != NULL)) {
		err = clk_prepare_enable(kpd_clk);
		if (err != 0) {
			clk_disable_unprepare(kpd_clk);
			kpd_print("clock enable fail.\n");
		}
	} else {
		kpd_print("get kpd-clk fail.\n");
		err = -ENODEV;
	}

	if (err == 0) {
		kp_base = of_iomap(pdev->dev.of_node, 0);
		kp_irqnr = irq_of_parse_and_map(pdev->dev.of_node, 0);

		if (kp_base == NULL) {
			kpd_print("keypad iomap failed\n");
			err = -ENODEV;
		}
		if (kp_irqnr == 0U) {
			kpd_print("KP get irq number failed\n");
			err = -ENODEV;
		}
	}

	if (err == 0) {
		kpd_info("kp base: 0x%p, kp irq: %d\n", kp_base, kp_irqnr);
		err = kpd_gpio_init(&pdev->dev);
		if (err != 0)
			kpd_print("gpio init failed\n");
	}

	/* initialize and register input device (/dev/input/eventX) */
	if (err == 0) {
		kpd_input_dev = input_allocate_device();
		if (kpd_input_dev != NULL) {
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
				if (kpd_keymap[i] != 0U)
					__set_bit((int32_t)kpd_keymap[i], kpd_input_dev->keybit);
			}
			kpd_input_dev->dev.parent = &pdev->dev;
		} else {
			kpd_print("input allocate device fail.\n");
			err = -ENOMEM;
		}
	}

	if (err == 0) {
		err = input_register_device(kpd_input_dev);
		if (err != 0) {
			kpd_print("register input device failed (%d)\n", err);
			input_free_device(kpd_input_dev);
		}
	}

	/* register device (/dev/mt6575-kpd) */
	if (err == 0) {
		kpd_dev.parent = &pdev->dev;
		err = misc_register(&kpd_dev);
		if (err != 0) {
			kpd_print("register device failed (%d)\n", err);
			input_unregister_device(kpd_input_dev);
			input_free_device(kpd_input_dev);
		}
	}

	if (err == 0) {
		if (kpd_dts_data.kpd_use_extend_type != 0U)
			kpd_double_key_enable(1);
		else
			kpd_double_key_enable(0);

		/* register IRQ and EINT */
		kpd_set_debounce(kpd_dts_data.kpd_key_debounce);
		err = request_irq(kp_irqnr, kpd_irq_handler, IRQF_TRIGGER_NONE, KPD_NAME, NULL);
		if (err != 0) {
			kpd_print("register IRQ failed (%d)\n", err);
			misc_deregister(&kpd_dev);
			input_unregister_device(kpd_input_dev);
			input_free_device(kpd_input_dev);
		}
	}
	if (err != 0) {
		kpd_info("kpd_probe ERROR(%d)!\n", err);
		err = -ENODEV;
	} else
		kpd_info("kpd_probe OK.\n");

	return err;
}

/* should never be called */
static int kpd_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static int kpd_pdrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	kpd_suspend = (bool)true;
	kpd_enable(0);
	return 0;
}

static int kpd_pdrv_resume(struct platform_device *pdev)
{
	kpd_suspend = (bool)false;
	kpd_enable(1);

	return 0;
}

static int __init kpd_mod_init(void)
{
	int32_t ret;

	ret = platform_driver_register(&kpd_pdrv);
	if (ret != 0)
		kpd_print("register driver failed (%d)\n", ret);

	return ret;
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
