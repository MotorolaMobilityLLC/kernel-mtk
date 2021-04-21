#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#define BIAS_VPOS_ADDR	0x00
#define BIAS_VNEG_ADDR	0x01
#define BIAS_CTR_ADDR	0x03
#define BIAS_MTP_ADDR	0xFF

enum LCM_BIAS {
	POS_REG = 0,
	NEG_REG,
};

static struct i2c_client *lcm_bias_i2c_client;
static unsigned char bias_value[2] = {0x00,0x00};

static int lcm_bias_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lcm_bias_i2c_remove(struct i2c_client *client);

static int lcm_bias_write_byte(unsigned char addr, unsigned char value)
{
    int ret = 0;
    unsigned char write_data[2] = {0};

    write_data[0] = addr;
    write_data[1] = value;

    if (NULL == lcm_bias_i2c_client) {
        pr_info("%s: [BIAS] client is null\n",__func__);
        return -1;
    }
    ret = i2c_master_send(lcm_bias_i2c_client, write_data, 2);

    if (ret < 0)
        pr_info("%s: [BIAS] i2c write data fail addr=%x, value=%x, ret=%x !!\n",__func__, addr, value, ret);

    return ret;
}

static int lcm_bias_read_byte(unsigned char addr, unsigned char *value)
{
	int ret = 0;
	struct i2c_msg msgs[2];
	struct i2c_client *client = lcm_bias_i2c_client;
	if (client == NULL) {
		 pr_err("%s: [BIAS] client is null\n",__func__);
		 return 0;
	 }

	msgs[0].addr = client->addr;
	msgs[0].len = 1;
	msgs[0].buf = &addr;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = 1;
	msgs[1].buf = value;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret < 0)
		pr_info("%s: [BIAS] i2c transfer error addr=%x, ret=%x\n",__func__, addr, ret);

	return ret;
}

int lcm_bias_config(void)
{
	unsigned char reg0 = 0, reg3 = 0;

	lcm_bias_read_byte(0x00, &reg0);
	pr_info("%s: [BIAS] reg0 = %x !!\n",__func__, reg0);

	lcm_bias_read_byte(0x03,&reg3);
	pr_info("%s: [BIAS] reg3 = %x !!\n",__func__, reg3);

	reg3 = reg3 & 0x30;
	if (reg3 == 0x30){ //ocp2130
		reg0 = reg0 & 0x1F;
		if (reg0 != bias_value[POS_REG]){
			lcm_bias_write_byte(BIAS_VPOS_ADDR, bias_value[POS_REG]);
			lcm_bias_write_byte(BIAS_VNEG_ADDR, bias_value[NEG_REG]);
			lcm_bias_write_byte(BIAS_MTP_ADDR, 0x80);

			pr_info("%s: [BIAS][OCP2130] POS = 0x%x, NEG = 0x%x\n",__func__, bias_value[POS_REG], bias_value[NEG_REG]);
		}
		lcm_bias_write_byte(BIAS_CTR_ADDR, 0x33);
	} else {//sm5109
		lcm_bias_write_byte(BIAS_VPOS_ADDR, bias_value[POS_REG]);
		lcm_bias_write_byte(BIAS_VNEG_ADDR, bias_value[NEG_REG]);
		lcm_bias_write_byte(BIAS_CTR_ADDR, 0x03);

		pr_info("%s: [BIAS][SM5109] POS = 0x%x, NEG = 0x%x\n",__func__, bias_value[POS_REG], bias_value[NEG_REG]);
	}

    return 0;
}
EXPORT_SYMBOL(lcm_bias_config);

static const struct of_device_id i2c_of_match[] = {
	{ .compatible = "mediatek,i2c_lcd_bias", },
    {},
};

static const struct i2c_device_id lcm_bias_i2c_id[] = {
    {"I2C_LCM_BIAS", 0},
    {},
};

static struct i2c_driver lcm_bias_i2c_driver = {
    .id_table = lcm_bias_i2c_id,
    .probe = lcm_bias_i2c_probe,
    .remove = lcm_bias_i2c_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "I2C_LCM_BIAS",
#ifdef CONFIG_OF
        .of_match_table = i2c_of_match,
#endif
    },
};

static int lcm_bias_parse_dt(void)
{
    struct device_node *np = NULL;
    int ret = 0;
    u32 value = 0;

    np = of_find_compatible_node(NULL, NULL, "mediatek,i2c_lcd_bias");

    if (!np) {
		pr_info("%s: [BIAS] dt node: not found\n", __func__);
		ret = -1;
		goto exit;
    } else {
		if (of_property_read_u32(np, "vpos", &value) < 0)
			bias_value[POS_REG] |= 15; //default 5.5v
		else
			bias_value[POS_REG] |= value;

		if (of_property_read_u32(np, "vneg", &value) < 0)
			bias_value[NEG_REG] |= 15; //default -5.5v
		else
			bias_value[NEG_REG] |= value;

		pr_info("%s: [BIAS] POS = 0x%x, NEG = 0x%x\n",__func__, bias_value[POS_REG], bias_value[NEG_REG]);
    }
    return 0;
exit:
    return ret;
}

static int lcm_bias_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;

    if (NULL == client) {
        pr_info("%s: [BIAS] client is NULL\n",__func__);
        return -1;
    }

	lcm_bias_i2c_client = client;
	ret = lcm_bias_parse_dt();
	pr_info("%s: [BIAS] addr = 0x%x\n",__func__, client->addr);

    return ret;
}

static int lcm_bias_i2c_remove(struct i2c_client *client)
{
    lcm_bias_i2c_client = NULL;
    i2c_unregister_device(client);

    return 0;
}

static int __init lcm_bias_init(void)
{
    if (i2c_add_driver(&lcm_bias_i2c_driver)) {
        pr_info("%s: [BIAS] failed to register!\n",__func__);
        return -1;
    }

    pr_info("%s: [BIAS] success\n",__func__);

    return 0;
}

static void __exit lcm_bias_exit(void)
{
	i2c_del_driver(&lcm_bias_i2c_driver);
	pr_info("%s: [BIAS] unregister\n",__func__);
}

module_init(lcm_bias_init);
module_exit(lcm_bias_exit);

MODULE_AUTHOR("lilianxu@wingtech.com>");
MODULE_DESCRIPTION("LCM BIAS Driver");
MODULE_LICENSE("GPL");
