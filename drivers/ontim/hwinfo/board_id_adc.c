/*************************************************************************
 File Name: board_id_adc.c
 Author: jiangfuxiong
 fuxiong.jiang@ontim.cn
 Created Time: 2018年09月18日 星期二 15时33分33秒
 *************************************************************************/




#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/qpnp/qpnp-adc.h>
#include "board_id_adc.h"


struct qpnp_vadc_result adc_result;
struct adc_board_id *chip_adc;

static ssize_t board_id_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct adc_board_id *chip = dev_get_drvdata(dev);
	int value = 0;
	int rc;
	chip->vadc_mux = VADC_AMUX_THM4_PU2;//0x50

	rc = qpnp_vadc_read(chip->vadc_dev, chip->vadc_mux, &adc_result);
	if (rc) {
		pr_err("ontim: %s: qpnp_vadc_read failed(%d)\n",
				__func__, rc);
	} else {
		chip->voltage = adc_result.physical;
//		printk("ontim:jiangfuxiong adc read voltage=%d,adc_result=%lld\n",chip->voltage,adc_result.physical);
	}
	value = adc_result.physical;
//	printk("ontim:jiangfuxiong adc read voltage=%d,adc_result=%lld\n",value,adc_result.physical);
	return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}


static ssize_t board_id_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	//struct board_id *data = dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
#if 0
	if (0 == data->user_enabled) {
		dev_dbg(dev, "user space can NOT set the state at this moment.\n");
		return -1;
	}
#endif
	ret = kstrtoul(buf, 16, &value);
	if (ret < 0) {
		dev_err(dev, "%s:kstrtoul failed, ret=0x%x\n",
				__func__, ret);
		return ret;
	}

	dev_dbg(dev, "set value %ld\n", value);
	if (1 == value) {
	} else if (0 == value) {
	} else {
		dev_err(dev, "set a invalid value %ld\n", value);
	}
	return size;
}

static DEVICE_ATTR(enable, 0664, board_id_enable_show, board_id_enable_store);

static int board_id_probe(struct platform_device *pdev)
{
	struct adc_board_id *chip;
	int rc = 0;
	int err = 0;

	//printk("jiangfuxiong %s start\n",__func__);
	chip = devm_kzalloc(&pdev->dev,
		sizeof(struct adc_board_id), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, chip);
	chip->vadc_dev = qpnp_get_vadc(&pdev->dev, "board_id");
	if (IS_ERR(chip->vadc_dev)) {
		rc = PTR_ERR(chip->vadc_dev);
		pr_err("ontim: jiangfuxiong vadc property missing\n");
		if (rc != -EPROBE_DEFER)
			pr_err("ontim: vadc property missing\n");
		goto err_board_id;
	}

	chip_adc = chip;
	chip->vadc_mux = VADC_AMUX_THM4_PU2;//0x50
	printk("ontim: jiangfuxiong chip->vadc_mux= %d\n",chip->vadc_mux);

	rc = qpnp_vadc_read(chip->vadc_dev, chip->vadc_mux, &adc_result);
	if (rc) {
		pr_err("ontim: %s: qpnp_vadc_read failed(%d)\n",
				__func__, rc);
		goto err_board_id;
	} else {
		chip->voltage = adc_result.physical / 1000;
	//	printk("ontim:jiangfuxiong adc read voltage=%d,adc_result=%lld\n",chip->voltage,adc_result.physical);
	}

	err =  device_create_file(&pdev->dev, &dev_attr_enable);
	if (err) {
		dev_err(&pdev->dev, "create enable sys-file failed,\n");
		goto err_board_id;
	}

	//printk("jiangfuxiong %s end\n",__func__);

	return 0;
err_board_id:
	devm_kfree(&pdev->dev, chip);
	return rc;
}

static int board_id_remove(struct platform_device *pdev)
{
//	struct board_id *data = dev_get_drvdata(pdev);
//	devm_kfree(&pdev->dev, data);
//	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct of_device_id platform_match_table[] = {
	     { .compatible = "ontim,board-id-adc",},
	     { },
	 };

static struct platform_driver board_id_driver = {
	.driver = {
		.name = "board_id",
		.of_match_table = platform_match_table,
		.owner = THIS_MODULE,
	},
	.probe = board_id_probe,
	.remove = board_id_remove,
	//.id_table = board_id_id,
};

static int __init board_id_init(void)
{
	return platform_driver_register(&board_id_driver);
}

static void __exit board_id_exit(void)
{
	return platform_driver_unregister(&board_id_driver);
}


module_init(board_id_init);
module_exit(board_id_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("board_id driver");
