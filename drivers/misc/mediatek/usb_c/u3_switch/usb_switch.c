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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <typec.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#include <linux/gpio.h>


#define K_EMERG	(1<<7)
#define K_QMU	(1<<7)
#define K_ALET		(1<<6)
#define K_CRIT		(1<<5)
#define K_ERR		(1<<4)
#define K_WARNIN	(1<<3)
#define K_NOTICE	(1<<2)
#define K_INFO		(1<<1)
#define K_DEBUG	(1<<0)

#define usbc_pinctrl_printk(level, fmt, args...) do { \
		if (debug_level & level) { \
			pr_err("[USB_SWITCH]" fmt, ## args); \
		} \
	} while (0)

static u32 debug_level = (255 - K_DEBUG);
static struct usbtypc *g_exttypec;

int usb3_switch_en(struct usbtypc *typec, int on)
{
	int retval = 0;

	if (!typec->pinctrl || !typec->pin_cfg || !typec->u3_sw) {
		usbc_pinctrl_printk(K_ERR, "%s not init\n", __func__);
		goto end;
	}

	if (!typec->pin_cfg->u3_switch_enable || !typec->pin_cfg->u3_switch_disable) {
		usbc_pinctrl_printk(K_ERR, "%s not ENABLE pin\n", __func__);
		goto end;
	}

	usbc_pinctrl_printk(K_DEBUG, "%s on=%d\n", __func__, on);

	if (on == ENABLE) {	/*enable usb switch */
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->u3_switch_enable);
		typec->u3_sw->en = ENABLE;
	} else { /*disable usb switch */
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->u3_switch_disable);
		typec->u3_sw->en = DISABLE;
	}

	usbc_pinctrl_printk(K_DEBUG, "%s gpio=%d\n", __func__, gpio_get_value(typec->u3_sw->en_gpio));
end:
	return retval;
}

int usb3_switch_sel(struct usbtypc *typec, int sel)
{
	int retval = 0;

	if (!typec->pinctrl || !typec->pin_cfg || !typec->u3_sw) {
		usbc_pinctrl_printk(K_ERR, "%s not init\n", __func__);
		goto end;
	}

	if (!typec->pin_cfg->u3_switch_sel1 || !typec->pin_cfg->u3_switch_sel2) {
		usbc_pinctrl_printk(K_ERR, "%s not SELECT PIN\n", __func__);
		goto end;
	}

	usbc_pinctrl_printk(K_DEBUG, "%s on=%d\n", __func__, sel);

	if (sel == CC1_SIDE) {
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->u3_switch_sel1);
		typec->u3_sw->sel = sel;
	} else if (sel == CC2_SIDE) {
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->u3_switch_sel2);
		typec->u3_sw->sel = sel;
	}

	usbc_pinctrl_printk(K_DEBUG, "%s gpio=%d\n", __func__, gpio_get_value(typec->u3_sw->sel_gpio));
end:
	return retval;
}

int usb3_switch_init(struct usbtypc *typec)
{
	int retval = 0;

	/*chip enable pin*/
	if (typec->u3_sw->en_gpio) {
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->u3_switch_disable);

		usbc_pinctrl_printk(K_DEBUG, "en_gpio=0x%X, out=%d\n", typec->u3_sw->en_gpio,
	    gpio_get_value(typec->u3_sw->en_gpio));
	}

	/*dir selection */
	if (typec->u3_sw->sel_gpio) {
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->u3_switch_sel1);

		usbc_pinctrl_printk(K_DEBUG, "sel_gpio=0x%X, out=%d\n", typec->u3_sw->sel_gpio,
		    gpio_get_value(typec->u3_sw->sel_gpio));
	}

	return retval;
}

int usb_redriver_init(struct usbtypc *typec)
{
	int retval = 0;

	if (typec->u_rd->c1_gpio) {
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->re_c1_init);

		usbc_pinctrl_printk(K_DEBUG, "c1_gpio=0x%X, out=%d\n", typec->u_rd->c1_gpio,
		    gpio_get_value(typec->u_rd->c1_gpio));
	}

	if (typec->u_rd->c2_gpio) {
		pinctrl_select_state(typec->pinctrl, typec->pin_cfg->re_c2_init);

		usbc_pinctrl_printk(K_DEBUG, "c2_gpio=0x%X, out=%d\n", typec->u_rd->c2_gpio,
		    gpio_get_value(typec->u_rd->c2_gpio));
	}

	return retval;
}

/*
 * ctrl_pin=1=C1 control pin
 * ctrl_pin=2=C2 control pin
 * (stat=0) = State=L
 * (stat=1) = State=High-Z
 * (stat=2) = State=H
 */
int usb_redriver_config(struct usbtypc *typec, int ctrl_pin, int stat)
{
	int retval = 0;
	int pin_num = 0;

	if (!typec->pinctrl || !typec->pin_cfg || !typec->u_rd) {
		usbc_pinctrl_printk(K_ERR, "%s not init\n", __func__);
		goto end;
	}

	usbc_pinctrl_printk(K_DEBUG, "%s pin=%d, stat=%d\n", __func__, ctrl_pin, stat);

	if (ctrl_pin == U3_EQ_C1)
		pin_num = typec->u_rd->c1_gpio;
	else if (ctrl_pin == U3_EQ_C2)
		pin_num = typec->u_rd->c2_gpio;

	if (!pin_num) {
		usbc_pinctrl_printk(K_ERR, "%s not support this PIN\n", __func__);
		goto end;
	}

	switch (stat) {
	case U3_EQ_LOW:
		if (ctrl_pin == U3_EQ_C1)
			pinctrl_select_state(typec->pinctrl,
					typec->pin_cfg->re_c1_low);
		else
			pinctrl_select_state(typec->pinctrl,
					typec->pin_cfg->re_c2_low);
		break;
	case U3_EQ_HZ:
		if (ctrl_pin == U3_EQ_C1)
			pinctrl_select_state(typec->pinctrl,
					typec->pin_cfg->re_c1_hiz);
		else
			pinctrl_select_state(typec->pinctrl,
					typec->pin_cfg->re_c2_hiz);
		break;
	case U3_EQ_HIGH:
		if (ctrl_pin == U3_EQ_C1)
			pinctrl_select_state(typec->pinctrl,
					typec->pin_cfg->re_c1_high);
		else
			pinctrl_select_state(typec->pinctrl,
					typec->pin_cfg->re_c2_high);
		break;
	default:
		retval = -EINVAL;
		break;
	}

	/* usbc_pinctrl_printk(K_DEBUG, "%s gpio=%d, out=%d\n", __func__, pin_num,
	 * gpio_get_value(pin_num));
	 */
end:
	return retval;
}

/*Print U3 switch & Redriver*/
static int usb_gpio_debugfs_show(struct seq_file *s, void *unused)
{
	struct usbtypc *typec = s->private;
	int pin = 0;

	seq_puts(s, "---U3 SWITCH---\n");
	pin = typec->u3_sw->en_gpio;
	if (pin) {
		usbc_pinctrl_printk(K_INFO, "en=%d, out=%d\n", pin, gpio_get_value(pin));

		seq_printf(s, "EN[%x] out=%d\n", pin, gpio_get_value(pin));
	}

	pin = typec->u3_sw->sel_gpio;
	if (pin) {
		usbc_pinctrl_printk(K_INFO, "sel=%d, out=%d\n", pin, gpio_get_value(pin));

		seq_printf(s, "SEL[%x] out=%d\n", pin, gpio_get_value(pin));
	}

	seq_puts(s, "---REDRIVER---\n");
	pin = typec->u_rd->c1_gpio;
	if (pin) {
		usbc_pinctrl_printk(K_INFO, "C1=%d, out=%d\n", pin, gpio_get_value(pin));

		seq_printf(s, "C1[%x] out=%d\n", pin, gpio_get_value(pin));
	}

	pin = typec->u_rd->c2_gpio;
	if (pin) {
		usbc_pinctrl_printk(K_INFO, "C2=%d, out=%d\n", pin, gpio_get_value(pin));

		seq_printf(s, "C2[%x] out=%d\n", pin, gpio_get_value(pin));
	}

	seq_puts(s, "---------\n");
	seq_puts(s, "sw [en|sel] [0|1]\n");
	seq_puts(s, "sw e 0 --> Disable\n");
	seq_puts(s, "sw e 1 --> Enable\n");
	seq_puts(s, "sw s 1 --> sel CC1\n");
	seq_puts(s, "sw s 2 --> sel CC2\n");
	seq_puts(s, "rd [1|2] [H|Z|L]\n");
	seq_puts(s, "---------\n");

	return 0;
}

static int usb_gpio_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, usb_gpio_debugfs_show, inode->i_private);
}

static ssize_t usb_gpio_debugfs_write(struct file *file,
				      const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct usbtypc *typec = s->private;

	char buf[18] = { 0 };
	char type = '\0';
	char gpio = '\0';
	int val = 0;

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	usbc_pinctrl_printk(K_INFO, "%s %s\n", __func__, buf);

	if (sscanf(buf, "sw %c %d", &gpio, &val) == 2) {
		usbc_pinctrl_printk(K_INFO, "%c %d\n", gpio, val);
		if (gpio == 'e')
			usb3_switch_en(typec, ((val == 0) ? DISABLE : ENABLE));
		else if (gpio == 's')
			usb3_switch_sel(typec, ((val == 1) ? CC1_SIDE : CC2_SIDE));
	} else if (sscanf(buf, "rd %c %c", &gpio, &type) == 2) {
		int stat = 0;

		usbc_pinctrl_printk(K_INFO, "%c %c\n", gpio, type);

		if (type == 'H')
			stat = U3_EQ_HIGH;
		else if (type == 'L')
			stat = U3_EQ_LOW;
		else if (type == 'Z')
			stat = U3_EQ_HZ;

		if (gpio == '1') {
			usb_redriver_config(typec, U3_EQ_C1, stat);
			typec->u_rd->eq_c1 = stat;
		} else if (gpio == '2') {
			usb_redriver_config(typec, U3_EQ_C2, stat);
			typec->u_rd->eq_c2 = stat;
		}
	}

	return count;
}

static const struct file_operations usb_gpio_debugfs_fops = {
	.open = usb_gpio_debugfs_open,
	.write = usb_gpio_debugfs_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int usb_switch_set(void *data, u64 val)
{
	struct usbtypc *typec = data;
	int sel = val;

	usbc_pinctrl_printk(K_INFO, "%s %d\n", __func__, sel);

	if (sel == 0) {
		usb3_switch_en(typec, DISABLE);
	} else {
		usb3_switch_en(typec, ENABLE);

		if (sel == 1)
			usb3_switch_sel(typec, CC1_SIDE);
		else
			usb3_switch_sel(typec, CC2_SIDE);
	}

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(usb_debugfs_fops, NULL, usb_switch_set, "%llu\n");

int usb_typec_init_debugfs(struct usbtypc *typec)
{
	struct dentry *root;
	struct dentry *file;
	int ret;

	root = debugfs_create_dir("usb_c", NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	file = debugfs_create_file("gpio", S_IRUGO|S_IWUSR, root, typec, &usb_gpio_debugfs_fops);

	file = debugfs_create_file("smt", S_IWUSR, root, typec,
						&usb_debugfs_fops);

	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

struct usbtypc *get_usbtypec(void)
{
	if (!g_exttypec)
		g_exttypec = kzalloc(sizeof(struct usbtypc), GFP_KERNEL);

	return g_exttypec;
}

static int usbc_pinctrl_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct usbtypc *typec;
	struct device_node *np;

	typec = get_usbtypec();

	typec->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(typec->pinctrl)) {
		usbc_pinctrl_printk(K_ERR, "Cannot find usb pinctrl!\n");
	} else {
		typec->pin_cfg = kzalloc(sizeof(struct usbc_pin_ctrl), GFP_KERNEL);

		usbc_pinctrl_printk(K_DEBUG, "pinctrl=%p\n", typec->pinctrl);

		/********************************************************/
		typec->pin_cfg->re_c1_init = pinctrl_lookup_state(typec->pinctrl, "redriver_c1_init");
		if (IS_ERR(typec->pin_cfg->re_c1_init)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c1_init\n");
			typec->pin_cfg->re_c1_init = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c1_init\n");

		typec->pin_cfg->re_c1_low = pinctrl_lookup_state(typec->pinctrl, "redriver_c1_low");
		if (IS_ERR(typec->pin_cfg->re_c1_low)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c1_low\n");
			typec->pin_cfg->re_c1_low = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c1_low\n");

		typec->pin_cfg->re_c1_hiz = pinctrl_lookup_state(typec->pinctrl, "redriver_c1_hiz");
		if (IS_ERR(typec->pin_cfg->re_c1_hiz)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c1_hiz\n");
			typec->pin_cfg->re_c1_hiz = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c1_hiz\n");

		typec->pin_cfg->re_c1_high = pinctrl_lookup_state(typec->pinctrl, "redriver_c1_high");
		if (IS_ERR(typec->pin_cfg->re_c1_high)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c1_high\n");
			typec->pin_cfg->re_c1_high = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c1_high\n");

		/********************************************************/

		typec->pin_cfg->re_c2_init = pinctrl_lookup_state(typec->pinctrl, "redriver_c2_init");
		if (IS_ERR(typec->pin_cfg->re_c2_init)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c2_init\n");
			typec->pin_cfg->re_c2_init = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c2_init\n");

		typec->pin_cfg->re_c2_low = pinctrl_lookup_state(typec->pinctrl, "redriver_c2_low");
		if (IS_ERR(typec->pin_cfg->re_c2_low)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c2_low\n");
			typec->pin_cfg->re_c2_low = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c2_low\n");

		typec->pin_cfg->re_c2_hiz = pinctrl_lookup_state(typec->pinctrl, "redriver_c2_hiz");
		if (IS_ERR(typec->pin_cfg->re_c2_hiz)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c2_hiz\n");
			typec->pin_cfg->re_c2_hiz = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c2_hiz\n");

		typec->pin_cfg->re_c2_high = pinctrl_lookup_state(typec->pinctrl, "redriver_c2_high");
		if (IS_ERR(typec->pin_cfg->re_c2_high)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find redriver_c2_high\n");
			typec->pin_cfg->re_c2_high = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find redriver_c2_high\n");

		/********************************************************/

		typec->pin_cfg->u3_switch_enable = pinctrl_lookup_state(typec->pinctrl, "u3_switch_enable");
		if (IS_ERR(typec->pin_cfg->u3_switch_enable)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find u3_switch_enable\n");
			typec->pin_cfg->u3_switch_enable = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find u3_switch_enable\n");

		typec->pin_cfg->u3_switch_disable = pinctrl_lookup_state(typec->pinctrl, "u3_switch_disable");
		if (IS_ERR(typec->pin_cfg->u3_switch_disable)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find u3_switch_disable\n");
			typec->pin_cfg->u3_switch_disable = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find u3_switch_disable\n");

		/********************************************************/

		typec->pin_cfg->u3_switch_sel1 = pinctrl_lookup_state(typec->pinctrl, "u3_switch_sel1");
		if (IS_ERR(typec->pin_cfg->u3_switch_sel1)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find u3_switch_sel1\n");
			typec->pin_cfg->u3_switch_sel1 = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find u3_switch_sel1\n");

		typec->pin_cfg->u3_switch_sel2 = pinctrl_lookup_state(typec->pinctrl, "u3_switch_sel2");
		if (IS_ERR(typec->pin_cfg->u3_switch_sel2)) {
			usbc_pinctrl_printk(K_ERR, "Can *NOT* find u3_switch_sel2\n");
			typec->pin_cfg->u3_switch_sel2 = NULL;
		} else
			usbc_pinctrl_printk(K_DEBUG, "Find u3_switch_sel2\n");

		usbc_pinctrl_printk(K_INFO, "Finish parsing pinctrl\n");
	}

	np = of_find_node_by_name(pdev->dev.of_node, "usb_switch-data");
	if (np) {
		typec->u3_sw = kzalloc(sizeof(struct usb3_switch), GFP_KERNEL);

		ret = of_property_read_u32(np, "en_pin_num", &typec->u3_sw->en_gpio);
		if (ret < 0) {
			usbc_pinctrl_printk(K_ERR, "%s get en_pin_num fail\n", __func__);
			typec->u3_sw->en = 0;
		}

		ret = of_property_read_u32(np, "en_pin_val", &typec->u3_sw->en);
		if (ret < 0)
			usbc_pinctrl_printk(K_ERR, "%s get en_pin_val fail\n", __func__);

		if (typec->u3_sw->en_gpio != 0)
			usbc_pinctrl_printk(K_ERR, "%s EN[%d]=%d\n", __func__,
							typec->u3_sw->en_gpio, typec->u3_sw->en);

		ret = of_property_read_u32(np, "sel_pin_num", &typec->u3_sw->sel_gpio);
		if (ret < 0) {
			usbc_pinctrl_printk(K_ERR, "%s get sel_pin_num fail\n", __func__);
			typec->u3_sw->sel_gpio = 0;
		}

		ret = of_property_read_u32(np, "sel_pin_val", &typec->u3_sw->sel);
		if (ret < 0)
			usbc_pinctrl_printk(K_ERR, "%s get sel_pin_val fail\n", __func__);

		if (typec->u3_sw->sel_gpio != 0)
			usbc_pinctrl_printk(K_ERR, "%s SEL[%d]=%d\n", __func__,
							typec->u3_sw->sel_gpio, typec->u3_sw->sel);

		typec->u_rd = kzalloc(sizeof(struct usb_redriver), GFP_KERNEL);

		ret = of_property_read_u32(np, "c1_pin_num", &typec->u_rd->c1_gpio);
		if (ret < 0) {
			usbc_pinctrl_printk(K_ERR, "%s get c1_pin_num fail\n", __func__);
			typec->u_rd->c1_gpio = 0;
		}

		ret = of_property_read_u32(np, "c1_pin_val", &typec->u_rd->eq_c1);
		if (ret < 0)
			usbc_pinctrl_printk(K_ERR, "%s get c1_pin_val fail\n", __func__);

		if (typec->u_rd->c1_gpio != 0)
			usbc_pinctrl_printk(K_ERR, "%s C1[%d]=%d\n", __func__,
						typec->u_rd->c1_gpio, typec->u_rd->eq_c1);

		ret = of_property_read_u32(np, "c2_pin_num", &typec->u_rd->c2_gpio);
		if (ret < 0) {
			usbc_pinctrl_printk(K_ERR, "%s get c2_pin_num fail\n", __func__);
			typec->u_rd->c2_gpio = 0;
		}

		ret = of_property_read_u32(np, "c2_pin_val", &typec->u_rd->eq_c2);
		if (ret < 0)
			usbc_pinctrl_printk(K_ERR, "%s get c2_pin_val fail\n", __func__);

		if (typec->u_rd->c2_gpio != 0)
			usbc_pinctrl_printk(K_ERR, "%s C1[%d]=%d\n", __func__,
						typec->u_rd->c2_gpio, typec->u_rd->eq_c2);
	}

	usb_redriver_init(typec);
	usb3_switch_init(typec);

	usb_typec_init_debugfs(typec);

	return ret;
}

static const struct of_device_id usb_pinctrl_ids[] = {
	{.compatible = "mediatek,usb_c_pinctrl",},
	{.compatible = "mediatek,tusb542",},
	{},
};

static struct platform_driver usb_switch_pinctrl_driver = {
	.probe = usbc_pinctrl_probe,
	.driver = {
		.name = "usbc_pinctrl",
#ifdef CONFIG_OF
		.of_match_table = usb_pinctrl_ids,
#endif
	},
};

int __init usbc_pinctrl_init(void)
{
	int ret = 0;

	if (!platform_driver_register(&usb_switch_pinctrl_driver))
		usbc_pinctrl_printk(K_DEBUG, "register usbc pinctrl succeed!!\n");
	else {
		usbc_pinctrl_printk(K_ERR, "register usbc pinctrl fail!!\n");
		ret = -1;
	}

	return ret;
}

late_initcall(usbc_pinctrl_init);

