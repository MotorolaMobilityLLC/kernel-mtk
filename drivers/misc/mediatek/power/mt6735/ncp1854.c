#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <mt-plat/charging.h>
#include "ncp1854.h"

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define NCP1854_SLAVE_ADDR_WRITE   0x6C
#define NCP1854_SLAVE_ADDR_READ	   0x6D

static struct i2c_client *new_client;
static const struct i2c_device_id ncp1854_i2c_id[] = { {"ncp1854", 0}, {} };

kal_bool chargin_hw_init_done = KAL_FALSE;
static int ncp1854_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

#ifdef CONFIG_OF
static const struct of_device_id ncp1854_of_match[] = {
	{.compatible = "ncp1854",},
	{},
};

MODULE_DEVICE_TABLE(of, ncp1854_of_match);
#endif

static struct i2c_driver ncp1854_driver = {
	.driver = {
		   .name = "ncp1854",
#ifdef CONFIG_OF
		   .of_match_table = ncp1854_of_match,
#endif
		   },
	.probe = ncp1854_driver_probe,
	.id_table = ncp1854_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
kal_uint8 ncp1854_reg[ncp1854_REG_NUM] = { 0 };

static DEFINE_MUTEX(ncp1854_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write ncp1854]
  *
  *********************************************************/
int ncp1854_read_byte(kal_uint8 cmd, kal_uint8 *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&ncp1854_i2c_access);

	/* new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG; */
	new_client->ext_flag =
	    ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

	cmd_buf[0] = cmd;
	ret = i2c_master_send(new_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
		new_client->ext_flag = 0;

		mutex_unlock(&ncp1854_i2c_access);
		return 0;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
	new_client->ext_flag = 0;

	mutex_unlock(&ncp1854_i2c_access);
	return 1;
}

int ncp1854_write_byte(kal_uint8 cmd, kal_uint8 writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&ncp1854_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

	new_client->ext_flag = ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG;

	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		new_client->ext_flag = 0;
		mutex_unlock(&ncp1854_i2c_access);
		return 0;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&ncp1854_i2c_access);
	return 1;
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 ncp1854_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK,
				  kal_uint8 SHIFT)
{
	kal_uint8 ncp1854_reg = 0;
	int ret = 0;

	battery_log(BAT_LOG_FULL, "--------------------------------------------------\n");

	ret = ncp1854_read_byte(RegNum, &ncp1854_reg);

	battery_log(BAT_LOG_FULL, "[ncp1854_read_interface] Reg[%x]=0x%x\n", RegNum, ncp1854_reg);

	ncp1854_reg &= (MASK << SHIFT);
	*val = (ncp1854_reg >> SHIFT);
	battery_log(BAT_LOG_FULL, "[ncp1854_read_interface] Val=0x%x\n", *val);

	return ret;
}

kal_uint32 ncp1854_config_interface(kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK,
				    kal_uint8 SHIFT)
{
	kal_uint8 ncp1854_reg = 0;
	int ret = 0;

	battery_log(BAT_LOG_FULL, "--------------------------------------------------\n");

	ret = ncp1854_read_byte(RegNum, &ncp1854_reg);
	battery_log(BAT_LOG_FULL, "[ncp1854_config_interface] Reg[%x]=0x%x\n", RegNum, ncp1854_reg);

	ncp1854_reg &= ~(MASK << SHIFT);
	ncp1854_reg |= (val << SHIFT);

	ret = ncp1854_write_byte(RegNum, ncp1854_reg);
	battery_log(BAT_LOG_FULL, "[ncp18546_config_interface] Write Reg[%x]=0x%x\n", RegNum, ncp1854_reg);

	/* Check */
	/* ncp1854_read_byte(RegNum, &ncp1854_reg); */
	/* battery_log(BAT_LOG_FULL, "[ncp1854_config_interface] Check Reg[%x]=0x%x\n", RegNum, ncp1854_reg); */

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
/* CON0 */
kal_uint32 ncp1854_get_chip_status(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) (NCP1854_CON0),
				     (kal_uint8 *) (&val),
				     (kal_uint8) (CON0_STATE_MASK), (kal_uint8) (CON0_STATE_SHIFT)
	    );
	return val;
}

kal_uint32 ncp1854_get_batfet(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) (NCP1854_CON0),
				     (kal_uint8 *) (&val),
				     (kal_uint8) (CON0_BATFET_MASK), (kal_uint8) (CON0_BATFET_SHIFT)
	    );
	return val;
}

kal_uint32 ncp1854_get_statint(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) (NCP1854_CON0),
				     (kal_uint8 *) (&val),
				     (kal_uint8) (CON0_STATINT_MASK),
				     (kal_uint8) (CON0_STATINT_SHIFT)
	    );
	return val;
}

kal_uint32 ncp1854_get_faultint(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) (NCP1854_CON0),
				     (kal_uint8 *) (&val),
				     (kal_uint8) (CON0_FAULTINT_MASK),
				     (kal_uint8) (CON0_FAULTINT_SHIFT)
	    );
	return val;
}

/* CON1 */
void ncp1854_set_reset(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_REG_RST_MASK),
				       (kal_uint8) (CON1_REG_RST_SHIFT)
	    );
}

void ncp1854_set_chg_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_CHG_EN_MASK),
				       (kal_uint8) (CON1_CHG_EN_SHIFT)
	    );
}

void ncp1854_set_otg_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_OTG_EN_MASK),
				       (kal_uint8) (CON1_OTG_EN_SHIFT)
	    );
}

kal_uint32 ncp1854_get_otg_en(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) (NCP1854_CON1),
				     (kal_uint8 *) (&val),
				     (kal_uint8) (CON1_OTG_EN_MASK), (kal_uint8) (CON1_OTG_EN_SHIFT)
	    );
	return val;
}

void ncp1854_set_fctry_mode(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_FCTRY_MOD_MASK),
				       (kal_uint8) (CON1_FCTRY_MOD_SHIFT)
	    );
}

void ncp1854_set_tj_warn_opt(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_TJ_WARN_OPT_MASK),
				       (kal_uint8) (CON1_TJ_WARN_OPT_SHIFT)
	    );
}

kal_uint32 ncp1854_get_usb_cfg(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) (NCP1854_CON1),
				     (kal_uint8 *) (&val),
				     (kal_uint8) (CON1_JEITA_OPT_MASK),
				     (kal_uint8) (CON1_JEITA_OPT_SHIFT)
	    );
	return val;
}

void ncp1854_set_tchg_rst(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_TCHG_RST_MASK),
				       (kal_uint8) (CON1_TCHG_RST_SHIFT)
	    );
}

void ncp1854_set_int_mask(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_INT_MASK_MASK),
				       (kal_uint8) (CON1_INT_MASK_SHIFT)
	    );
}

/* CON2 */
void ncp1854_set_wdto_dis(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_WDTO_DIS_MASK),
				       (kal_uint8) (CON2_WDTO_DIS_SHIFT)
	    );
}

void ncp1854_set_chgto_dis(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_CHGTO_DIS_MASK),
				       (kal_uint8) (CON2_CHGTO_DIS_SHIFT)
	    );
}

void ncp1854_set_pwr_path(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_PWR_PATH_MASK),
				       (kal_uint8) (CON2_PWR_PATH_SHIFT)
	    );
}

void ncp1854_set_trans_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_TRANS_EN_MASK),
				       (kal_uint8) (CON2_TRANS_EN_SHIFT)
	    );
}

void ncp1854_set_iinset_pin_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_IINSET_PIN_EN_MASK),
				       (kal_uint8) (CON2_IINSET_PIN_EN_SHIFT)
	    );
}

void ncp1854_set_iinlim_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_IINLIM_EN_MASK),
				       (kal_uint8) (CON2_IINLIM_EN_SHIFT)
	    );
}

void ncp1854_set_aicl_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_AICL_EN_MASK),
				       (kal_uint8) (CON2_AICL_EN_SHIFT)
	    );
}

/* CON8 */
kal_uint32 ncp1854_get_vfet_ok(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) (NCP1854_CON8),
				     (kal_uint8 *) (&val),
				     (kal_uint8) (CON8_VFET_OK_MASK),
				     (kal_uint8) (CON8_VFET_OK_SHIFT)
	    );
	return val;
}


/* CON14 */
void ncp1854_set_ctrl_vbat(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON14),
				       (kal_uint8) (val),
				       (kal_uint8) (CON14_CTRL_VBAT_MASK),
				       (kal_uint8) (CON14_CTRL_VBAT_SHIFT)
	    );
}

/* CON15 */
void ncp1854_set_ichg_high(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON15),
				       (kal_uint8) (val),
				       (kal_uint8) (CON15_ICHG_HIGH_MASK),
				       (kal_uint8) (CON15_ICHG_HIGH_SHIFT)
	    );
}

void ncp1854_set_ieoc(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON15),
				       (kal_uint8) (val),
				       (kal_uint8) (CON15_IEOC_MASK), (kal_uint8) (CON15_IEOC_SHIFT)
	    );
}

void ncp1854_set_ichg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON15),
				       (kal_uint8) (val),
				       (kal_uint8) (CON15_ICHG_MASK), (kal_uint8) (CON15_ICHG_SHIFT)
	    );
}

kal_uint32 ncp1854_get_ichg(void)
{
	kal_uint32 ret = 0;
	kal_uint32 val = 0;

	ret = ncp1854_read_interface((kal_uint8) NCP1854_CON15,
				     (kal_uint8 *) &val,
				     (kal_uint8) CON15_ICHG_MASK, (kal_uint8) CON15_ICHG_SHIFT);
	return val;
}

/* CON16 */
void ncp1854_set_iweak(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON16),
				       (kal_uint8) (val),
				       (kal_uint8) (CON16_IWEAK_MASK),
				       (kal_uint8) (CON16_IWEAK_SHIFT)
	    );
}

void ncp1854_set_ctrl_vfet(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON16),
				       (kal_uint8) (val),
				       (kal_uint8) (CON16_CTRL_VFET_MASK),
				       (kal_uint8) (CON16_CTRL_VFET_SHIFT)
	    );
}

void ncp1854_set_iinlim(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON16),
				       (kal_uint8) (val),
				       (kal_uint8) (CON16_IINLIM_MASK),
				       (kal_uint8) (CON16_IINLIM_SHIFT)
	    );
}

/* CON17 */
void ncp1854_set_iinlim_ta(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = ncp1854_config_interface((kal_uint8) (NCP1854_CON17),
				       (kal_uint8) (val),
				       (kal_uint8) (CON17_IINLIM_TA_MASK),
				       (kal_uint8) (CON17_IINLIM_TA_SHIFT)
	    );
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void ncp1854_dump_register(void)
{
	int i = 0;

	for (i = 0; i < ncp1854_REG_NUM; i++) {
		if ((i == 3) || (i == 4) || (i == 5) || (i == 6))	/* do not dump read clear status register */
			continue;
		if ((i == 10) || (i == 11) || (i == 12) || (i == 13))	/* do not dump interrupt mask bit register */
			continue;
		ncp1854_read_byte(i, &ncp1854_reg[i]);
		battery_log(BAT_LOG_FULL, "[ncp1854_dump_register] Reg[0x%X]=0x%X\n", i, ncp1854_reg[i]);
	}
}

void ncp1854_read_register(int i)
{
	ncp1854_read_byte(i, &ncp1854_reg[i]);
	battery_log(BAT_LOG_FULL, "[ncp1854_read_register] Reg[0x%X]=0x%X\n", i, ncp1854_reg[i]);
}

static int ncp1854_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;

	battery_log(BAT_LOG_CRTI, "[ncp1854_driver_probe]\n");

	new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);

	if (!new_client) {
		err = -ENOMEM;
		goto exit;
	}
	memset(new_client, 0, sizeof(struct i2c_client));

	new_client = client;

	chargin_hw_init_done = KAL_TRUE;

	return 0;

exit:
	return err;

}

/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/
kal_uint8 g_reg_value_ncp1854 = 0;
static ssize_t show_ncp1854_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	battery_log(BAT_LOG_CRTI, "[show_ncp1854_access] 0x%x\n", g_reg_value_ncp1854);
	return sprintf(buf, "%u\n", g_reg_value_ncp1854);
}

static ssize_t store_ncp1854_access(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	battery_log(BAT_LOG_CRTI, "[store_ncp1854_access]\n");

	if (buf != NULL && size != 0) {
		battery_log(BAT_LOG_CRTI, "[store_ncp1854_access] buf is %s and size is %d\n", buf, (int)size);
		/*reg_address = kstrtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		if (size > 3) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);

		if (size > 3) {
			val =  strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);

			battery_log(BAT_LOG_CRTI,
			    "[store_ncp1854_access] write ncp1854 reg 0x%x with value 0x%x !\n",
			     reg_address, reg_value);
			ret = ncp1854_config_interface(reg_address, reg_value, 0xFF, 0x0);
		} else {
			ret = ncp1854_read_interface(reg_address, &g_reg_value_ncp1854, 0xFF, 0x0);
			battery_log(BAT_LOG_CRTI,
			    "[store_ncp1854_access] read ncp1854 reg 0x%x with value 0x%x !\n",
			     reg_address, g_reg_value_ncp1854);
			battery_log(BAT_LOG_CRTI,
			    "[store_ncp1854_access] Please use \"cat ncp1854_access\" to get value\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(ncp1854_access, 0664, show_ncp1854_access, store_ncp1854_access);	/* 664 */

static int ncp1854_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	battery_log(BAT_LOG_CRTI, "******** ncp1854_user_space_probe!! ********\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_ncp1854_access);

	return 0;
}

struct platform_device ncp1854_user_space_device = {
	.name = "ncp1854-user",
	.id = -1,
};

static struct platform_driver ncp1854_user_space_driver = {
	.probe = ncp1854_user_space_probe,
	.driver = {
		   .name = "ncp1854-user",
		   },
};

static int __init ncp1854_init(void)
{
	int ret = 0;

	battery_log(BAT_LOG_CRTI, "[ncp1854_init] init start\n");

	if (i2c_add_driver(&ncp1854_driver) != 0)
		battery_log(BAT_LOG_CRTI, "[ncp1854_init] failed to register ncp1854 i2c driver.\n");
	else
		battery_log(BAT_LOG_CRTI, "[ncp1854_init] Success to register ncp1854 i2c driver.\n");

	/* ncp1854 user space access interface */
	ret = platform_device_register(&ncp1854_user_space_device);
	if (ret) {
		battery_log(BAT_LOG_CRTI, "****[ncp1854_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&ncp1854_user_space_driver);
	if (ret) {
		battery_log(BAT_LOG_CRTI, "****[ncp1854_init] Unable to register driver (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void __exit ncp1854_exit(void)
{
	i2c_del_driver(&ncp1854_driver);
}

module_init(ncp1854_init);
module_exit(ncp1854_exit);
/* subsys_initcall(ncp1854_subsys_init); */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C ncp1854 Driver");
MODULE_AUTHOR("YT Lee<yt.lee@mediatek.com>");
