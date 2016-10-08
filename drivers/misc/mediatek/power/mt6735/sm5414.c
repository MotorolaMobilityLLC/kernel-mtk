#include <linux/types.h>
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
#include <linux/gpio.h>
#include "sm5414.h"

/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define sm5414_SLAVE_ADDR_WRITE   0x92
#define sm5414_SLAVE_ADDR_READ    0x93

#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define sm5414_BUSNUM I2C_SWITHING_CHARGER_CHANNEL
#else
#define sm5414_BUSNUM 1
#endif

static struct i2c_client *new_client = NULL;
//static struct i2c_board_info __initdata i2c_sm5414 = { I2C_BOARD_INFO("sm5414", (sm5414_SLAVE_ADDR_WRITE>>1))};
static const struct i2c_device_id sm5414_i2c_id[] = {{"sm5414",0},{}};   
kal_bool chargin_hw_init_done = KAL_FALSE;
static int sm5414_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

//static unsigned int sm5414_irq = 0;

#ifdef CONFIG_OF
static const struct of_device_id sm5414_of_match[] = {
	{ .compatible = "mediatek,SWITHING_CHARGER", },
	{},
};

MODULE_DEVICE_TABLE(of, sm5414_of_match);
#endif

static struct i2c_driver sm5414_driver = {
    .driver = {
        .name    = "sm5414",
#ifdef CONFIG_OF 
        .of_match_table = sm5414_of_match,
#endif
    },
    .probe       = sm5414_driver_probe,
    .id_table    = sm5414_i2c_id,
};

//add by caozhg
static int sm5414_gpio_probe(struct platform_device *pdev);
void sm5414_gpio_output(int pin, int level);
struct of_device_id sm5414_gpio_of_match[] = {
	{ .compatible = "mediatek,charger_sm5414", },
};
struct platform_device sm5414_gpio_device = {
	.name		= "sm5414_gpio",
	.id			= -1,
};
static struct platform_driver sm5414_gpio_driver = {
	.probe = sm5414_gpio_probe,
	.driver = {
			.name = "sm5414_gpio",
			.owner = THIS_MODULE,
			.of_match_table = sm5414_gpio_of_match,
	},
};
struct pinctrl *sm5414_pinctrl;
struct pinctrl_state *sm5414_nshdn_high, *sm5414_nshdn_low, *sm5414_chren_high, *sm5414_chren_low;

//end by caozhg
/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
unsigned char sm5414_reg[SM5414_REG_NUM] = {0};

static DEFINE_MUTEX(sm5414_i2c_access);

int g_sm5414_hw_exist=0;

/**********************************************************
  *
  *   [I2C Function For Read/Write sm5414] 
  *
  *********************************************************/
#ifdef CONFIG_MTK_I2C_EXTENSION
unsigned int sm5414_read_byte(unsigned char cmd, unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&sm5414_i2c_access);

	/* new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG; */
	new_client->ext_flag =
	    ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

	cmd_buf[0] = cmd;
	ret = i2c_master_send(new_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
		new_client->ext_flag = 0;
		mutex_unlock(&sm5414_i2c_access);

		return 0;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
	new_client->ext_flag = 0;
	mutex_unlock(&sm5414_i2c_access);

	return 1;
}

unsigned int sm5414_write_byte(unsigned char cmd, unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&sm5414_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

	new_client->ext_flag = ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG;

	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		new_client->ext_flag = 0;
		mutex_unlock(&sm5414_i2c_access);
		return 0;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&sm5414_i2c_access);
	return 1;
}
#else
unsigned int sm5414_read_byte(unsigned char cmd, unsigned char *returnData)
{
	unsigned char xfers = 2;
	int ret, retries = 1;

	mutex_lock(&sm5414_i2c_access);

	do {
		struct i2c_msg msgs[2] = {
			{
				.addr = new_client->addr,
				.flags = 0,
				.len = 1,
				.buf = &cmd,
			},
			{

				.addr = new_client->addr,
				.flags = I2C_M_RD,
				.len = 1,
				.buf = returnData,
			}
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			battery_log(BAT_LOG_CRTI, "skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&sm5414_i2c_access);

	return ret == xfers ? 1 : -1;
}

unsigned int sm5414_write_byte(unsigned char cmd, unsigned char writeData)
{
	unsigned char xfers = 1;
	int ret, retries = 1;
	unsigned char buf[8];

	mutex_lock(&sm5414_i2c_access);

	buf[0] = cmd;
	memcpy(&buf[1], &writeData, 1);

	do {
		struct i2c_msg msgs[1] = {
			{
				.addr = new_client->addr,
				.flags = 0,
				.len = 1 + 1,
				.buf = buf,
			},
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			battery_log(BAT_LOG_CRTI, "skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&sm5414_i2c_access);

	return ret == xfers ? 1 : -1;
}
#endif
/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
unsigned int sm5414_read_interface (unsigned char RegNum, unsigned char *val, unsigned char MASK, unsigned char SHIFT)
{
    unsigned char sm5414_reg = 0;
    int ret = 0;

    battery_log(BAT_LOG_FULL, "--------------------------------------------------\n");

    ret = sm5414_read_byte(RegNum, &sm5414_reg);

    battery_log(BAT_LOG_FULL, "[sm5414_read_interface] Reg[%x]=0x%x\n", RegNum, sm5414_reg);
    
    sm5414_reg &= (MASK << SHIFT);
    *val = (sm5414_reg >> SHIFT);    
	
    battery_log(BAT_LOG_FULL, "[sm5414_read_interface] val=0x%x\n", *val);
	
    return ret;
}

unsigned int sm5414_config_interface (unsigned char RegNum, unsigned char val, unsigned char MASK, unsigned char SHIFT)
{
    unsigned char sm5414_reg = 0;
    int ret = 0;

    battery_log(BAT_LOG_FULL, "--------------------------------------------------\n");

    ret = sm5414_read_byte(RegNum, &sm5414_reg);
    battery_log(BAT_LOG_FULL, "[sm5414_config_interface] Reg[%x]=0x%x\n", RegNum, sm5414_reg);
    
    sm5414_reg &= ~(MASK << SHIFT);
    sm5414_reg |= (val << SHIFT);

    ret = sm5414_write_byte(RegNum, sm5414_reg);
    battery_log(BAT_LOG_FULL, "[sm5414_config_interface] write Reg[%x]=0x%x\n", RegNum, sm5414_reg);

    // Check
    //sm5414_read_byte(RegNum, &sm5414_reg);
    //battery_log(BAT_LOG_FULL, "[sm5414_config_interface] Check Reg[%x]=0x%x\n", RegNum, sm5414_reg);

    return ret;
}

//write one register directly
unsigned int sm5414_reg_config_interface (unsigned char RegNum, unsigned char val)
{   
    unsigned int ret = 0;
    
    ret = sm5414_write_byte(RegNum, val);

    return ret;
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
//STATUS2----------------------------------------------------
unsigned int sm5414_get_topoff_status(void)
{
    unsigned char val=0;
    unsigned int ret=0; 
 
    ret=sm5414_read_interface(     (unsigned char)(SM5414_STATUS), 
                                    (&val),
                                    (unsigned char)(SM5414_STATUS_TOPOFF_MASK),
                                    (unsigned char)(SM5414_STATUS_TOPOFF_SHIFT)
                                    );
    return val;
   // return 0;
}

//CTRL----------------------------------------------------
void sm5414_set_enboost(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CTRL), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CTRL_ENBOOST_MASK),
                                    (unsigned char)(SM5414_CTRL_ENBOOST_SHIFT)
                                    );
}

void sm5414_set_chgen(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CTRL), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CTRL_CHGEN_MASK),
                                    (unsigned char)(SM5414_CTRL_CHGEN_SHIFT)
                                    );
}

void sm5414_set_suspen(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CTRL), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CTRL_SUSPEN_MASK),
                                    (unsigned char)(SM5414_CTRL_SUSPEN_SHIFT)
                                    );
}

void sm5414_set_reset(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CTRL), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CTRL_RESET_MASK),
                                    (unsigned char)(SM5414_CTRL_RESET_SHIFT)
                                    );
}

void sm5414_set_encomparator(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CTRL), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CTRL_ENCOMPARATOR_MASK),
                                    (unsigned char)(SM5414_CTRL_ENCOMPARATOR_SHIFT)
                                    );
}

//vbusctrl----------------------------------------------------
void sm5414_set_vbuslimit(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_VBUSCTRL), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_VBUSCTRL_VBUSLIMIT_MASK),
                                    (unsigned char)(SM5414_VBUSCTRL_VBUSLIMIT_SHIFT)
                                    );
}

//chgctrl1----------------------------------------------------
void sm5414_set_prechg(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL1), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL1_PRECHG_MASK),
                                    (unsigned char)(SM5414_CHGCTRL1_PRECHG_SHIFT)
                                    );
}
void sm5414_set_aiclen(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL1), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL1_AICLEN_MASK),
                                    (unsigned char)(SM5414_CHGCTRL1_AICLEN_SHIFT)
                                    );
}
void sm5414_set_autostop(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL1), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL1_AUTOSTOP_MASK),
                                    (unsigned char)(SM5414_CHGCTRL1_AUTOSTOP_SHIFT)
                                    );
}
void sm5414_set_aiclth(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL1), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL1_AICLTH_MASK),
                                    (unsigned char)(SM5414_CHGCTRL1_AICLTH_SHIFT)
                                    );
}

//chgctrl2----------------------------------------------------
void sm5414_set_fastchg(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL2), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL2_FASTCHG_MASK),
                                    (unsigned char)(SM5414_CHGCTRL2_FASTCHG_SHIFT)
                                    );
}

//chgctrl3----------------------------------------------------
void sm5414_set_weakbat(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL3), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL3_WEAKBAT_MASK),
                                    (unsigned char)(SM5414_CHGCTRL3_WEAKBAT_SHIFT)
                                    );
}
void sm5414_set_batreg(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL3), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL3_BATREG_MASK),
                                    (unsigned char)(SM5414_CHGCTRL3_BATREG_SHIFT)
                                    );
}

//chgctrl4----------------------------------------------------
void sm5414_set_dislimit(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL4), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL4_DISLIMIT_MASK),
                                    (unsigned char)(SM5414_CHGCTRL4_DISLIMIT_SHIFT)
                                    );
}
void sm5414_set_topoff(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL4), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL4_TOPOFF_MASK),
                                    (unsigned char)(SM5414_CHGCTRL4_TOPOFF_SHIFT)
                                    );
}

//chgctrl5----------------------------------------------------
void sm5414_set_topofftimer(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL5), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL5_TOPOFFTIMER_MASK),
                                    (unsigned char)(SM5414_CHGCTRL5_TOPOFFTIMER_SHIFT)
                                    );
}
void sm5414_set_fasttimer(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL5), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL5_FASTTIMER_MASK),
                                    (unsigned char)(SM5414_CHGCTRL5_FASTTIMER_SHIFT)
                                    );
}
void sm5414_set_votg(unsigned int val)
{
    unsigned int ret=0;    

    ret=sm5414_config_interface(   (unsigned char)(SM5414_CHGCTRL5), 
                                    (unsigned char)(val),
                                    (unsigned char)(SM5414_CHGCTRL5_VOTG_MASK),
                                    (unsigned char)(SM5414_CHGCTRL5_VOTG_SHIFT)
                                    );
}  

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
void sm5414_cnt_nshdn_pin(unsigned int enable)
{
#if 0 //modify by caozhg
    //SM : Below is nSHDN pin
    if(KAL_TRUE == enable)
    {    
    //SM : Below is nSHDN pin
        mt_set_gpio_mode(GPIO_SM5414_SHDN_PIN,GPIO_MODE_GPIO);  
        mt_set_gpio_dir(GPIO_SM5414_SHDN_PIN,GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_SM5414_SHDN_PIN,GPIO_OUT_ONE);
    }
    else
    {    
    //SM : Below is nSHDN pin
        mt_set_gpio_mode(GPIO_SM5414_SHDN_PIN,GPIO_MODE_GPIO);  
        mt_set_gpio_dir(GPIO_SM5414_SHDN_PIN,GPIO_DIR_OUT);        
        mt_set_gpio_out(GPIO_SM5414_SHDN_PIN,GPIO_OUT_ZERO);
    }
#endif
}
void sm5414_otg_enable(unsigned int enable)
{
    if(KAL_TRUE == enable)
    {
	//modify by caozhg
        //Before turning on OTG, system must turn off charing function.        
        //SM : Below is nCHGEN pin
        //mt_set_gpio_mode(GPIO_SM5414_CHGEN_PIN,GPIO_MODE_GPIO);  
        //mt_set_gpio_dir(GPIO_SM5414_CHGEN_PIN,GPIO_DIR_OUT);
        //mt_set_gpio_out(GPIO_SM5414_CHGEN_PIN,GPIO_OUT_ONE);

	//add by caozhg no use
	//gpio_set_value (GPIO_SM5414_CHREN_PIN, 1);
	//gpio_set_value (GPIO_SM5414_NSHDN_PIN, 1);
	sm5414_gpio_output(0,1);
	sm5414_gpio_output(1,1);
	//sm5414_set_chgen (0);
        sm5414_set_enboost(ENBOOST_EN);
    }
    else
    {   
        sm5414_set_enboost(ENBOOST_DIS);
	sm5414_gpio_output(0,1);
        sm5414_gpio_output(1,0);
    }
}
void sm5414_hw_component_detect(void)
{
    unsigned int ret=0;
    unsigned char val=0;

    ret=sm5414_read_interface(0x0E, &val, 0xFF, 0x0);
    
    if(val == 0)
        g_sm5414_hw_exist=0;
    else
        g_sm5414_hw_exist=1;

    battery_log(BAT_LOG_FULL, "[sm5414_hw_component_detect] exist=%d, Reg[0x0E]=0x%x\n", 
        g_sm5414_hw_exist, val);
}

int is_sm5414_exist(void)
{
    battery_log(BAT_LOG_FULL, "[is_sm5414_exist] g_sm5414_hw_exist=%d\n", g_sm5414_hw_exist);
    
    return g_sm5414_hw_exist;
}

void sm5414_dump_register(void)
{
    int i=0;
    battery_log(BAT_LOG_FULL, "[sm5414]\n ");
    for (i=SM5414_INTMASK1;i<SM5414_REG_NUM;i++)
    {
        sm5414_read_byte(i, &sm5414_reg[i]);
        printk ("[0x%x]=0x%x ", i, sm5414_reg[i]);        
    }
    battery_log(BAT_LOG_FULL, "\n");
}

void sm5414_reg_init(void)
{
   //INT MASK 1/2/3
    sm5414_write_byte(SM5414_INTMASK1, 0xFF);
    sm5414_write_byte(SM5414_INTMASK2, 0xFF);
    sm5414_write_byte(SM5414_INTMASK3, 0xFF);

	sm5414_set_encomparator(ENCOMPARATOR_EN);
    sm5414_set_topoff(TOPOFF_150mA);
#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
	sm5414_set_batreg(BATREG_4_3_5_0_V); //VREG 4.352V
#else
	sm5414_set_batreg(BATREG_4_2_0_0_V); //VREG 4.208V
#endif 
    //sm5414_set_autostop(AUTOSTOP_DIS);

#if defined(SM5414_TOPOFF_TIMER_SUPPORT)
    sm5414_set_autostop(AUTOSTOP_EN);
    sm5414_set_topofftimer(TOPOFFTIMER_10MIN);
#else
    sm5414_set_autostop(AUTOSTOP_DIS);
#endif
}

static int sm5414_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int err=0; 

    printk ("[sm5414_driver_probe] \n");

    #if 0     //modify by caozhg
    //SM : Below is nSHDN pin
    mt_set_gpio_mode(GPIO_SM5414_SHDN_PIN,GPIO_MODE_GPIO);  
    mt_set_gpio_dir(GPIO_SM5414_SHDN_PIN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_SM5414_SHDN_PIN,GPIO_OUT_ONE);
    #endif
    sm5414_gpio_output (0, 1);

    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }  
    printk ("[sm5414_driver_probe] xx\n");  
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client = client;    
    //---------------------
    sm5414_hw_component_detect();

	sm5414_reg_init();
	
	chargin_hw_init_done = KAL_TRUE;

	sm5414_dump_register();

	printk ("[sm5414_driver_probe] DONE\n");

    return 0;
    
exit:
    return err;

}

/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/
unsigned char g_reg_value_sm5414=0;
static ssize_t show_sm5414_access(struct device *dev,struct device_attribute *attr, char *buf)
{
    battery_log(BAT_LOG_CRTI, "[show_sm5414_access] 0x%x\n", g_reg_value_sm5414);
    return sprintf(buf, "%u\n", g_reg_value_sm5414);
}
static ssize_t store_sm5414_access(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int ret=0;
    char *pvalue = NULL;
    unsigned int reg_value = 0;
    unsigned int reg_address = 0;
    
    battery_log(BAT_LOG_CRTI, "[store_sm5414_access] \n");
    
    if(buf != NULL && size != 0)
    {
        battery_log(BAT_LOG_CRTI, "[store_sm5414_access] buf is %s and size is %zu \n",buf,size);
        reg_address = simple_strtoul(buf,&pvalue,16);
        
        if(size > 3)
        {        
            reg_value = simple_strtoul((pvalue+1),NULL,16);        
            battery_log(BAT_LOG_CRTI, "[store_sm5414_access] write sm5414 reg 0x%x with value 0x%x !\n",reg_address,reg_value);
            ret=sm5414_config_interface(reg_address, reg_value, 0xFF, 0x0);
        }
        else
        {    
            ret=sm5414_read_interface(reg_address, &g_reg_value_sm5414, 0xFF, 0x0);
            battery_log(BAT_LOG_CRTI, "[store_sm5414_access] read sm5414 reg 0x%x with value 0x%x !\n",reg_address,g_reg_value_sm5414);
            battery_log(BAT_LOG_CRTI, "[store_sm5414_access] Please use \"cat sm5414_access\" to get value\r\n");
        }        
    }    
    return size;
}
static DEVICE_ATTR(sm5414_access, 0664, show_sm5414_access, store_sm5414_access); //664

static int sm5414_user_space_probe(struct platform_device *dev)    
{    
    int ret_device_file = 0;

    battery_log(BAT_LOG_CRTI, "******** sm5414_user_space_probe!! ********\n" );
    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_sm5414_access);
    
    return 0;
}

struct platform_device sm5414_user_space_device = {
    .name   = "sm5414-user",
    .id     = -1,
};

static struct platform_driver sm5414_user_space_driver = {
    .probe      = sm5414_user_space_probe,
    .driver     = {
        .name = "sm5414-user",
    },
};
//add by caozhg 

static int sm5414_gpio_probe(struct platform_device *pdev)
{
	int ret;
	printk ("[sm5414 %d] mt_sm5414_pinctrl+++++++++++++++++\n", pdev->id);
	sm5414_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(sm5414_pinctrl)) {
		ret = PTR_ERR(sm5414_pinctrl);
		dev_err(&pdev->dev, "fwq Cannot find sm5414 sm5414_pinctrl!\n");
		return ret;
	}
	sm5414_nshdn_high = pinctrl_lookup_state(sm5414_pinctrl, "nshdn-pullhigh");
	if (IS_ERR(sm5414_nshdn_high)) {
		ret = PTR_ERR(sm5414_nshdn_high);
		dev_err(&pdev->dev, "fwq Cannot find sm5414 pinctrl nshdn-pullhigh!\n");
		return ret;
	}
	sm5414_nshdn_low = pinctrl_lookup_state(sm5414_pinctrl, "nshdn-pulllow");
	if (IS_ERR(sm5414_nshdn_low)) {
		ret = PTR_ERR(sm5414_nshdn_low);
		dev_err(&pdev->dev, "fwq Cannot find sm5414 pinctrl nshdn-pulllow!\n");
		return ret;
	}
	sm5414_chren_high = pinctrl_lookup_state(sm5414_pinctrl, "chren-pullhigh");
	if (IS_ERR(sm5414_chren_high)) {
		ret = PTR_ERR(sm5414_chren_high);
		dev_err(&pdev->dev, "fwq Cannot find sm5414 pinctrl chren-pullhigh!\n");
		return ret;
	}
	sm5414_chren_low = pinctrl_lookup_state(sm5414_pinctrl, "chren-pulllow");
	if (IS_ERR(sm5414_chren_low)) {
		ret = PTR_ERR(sm5414_chren_low);
		dev_err(&pdev->dev, "fwq Cannot find sm5414 pinctrl chren-pulllow!\n");
		return ret;
	}
	printk ("[sm5414 %d] mt_sm5414_pinctrl----------\n", pdev->id);
	return 0;
}
void sm5414_gpio_output(int pin, int level) //pin 0->nshdn 1>chren   0->pull_down   1->pull_up  
{
	printk ("[sm5414] sm5414_output pin = %d, level = %d\n", pin, level);
	if (pin == 0) {
		if (level)
			pinctrl_select_state(sm5414_pinctrl, sm5414_nshdn_high);
		else
			pinctrl_select_state(sm5414_pinctrl, sm5414_nshdn_low);
	} else {
		if (level)
			pinctrl_select_state(sm5414_pinctrl, sm5414_chren_high);
		else
			pinctrl_select_state(sm5414_pinctrl, sm5414_chren_low);
	}
}


//end by caozhg
static int __init sm5414_subsys_init(void)
{    
    int ret=0;
#ifdef CONFIG_OF
	//battery_log(BAT_LOG_CRTI, "[sm5414_init] init start with i2c DTS");
	printk ("[sm5414_init] init start with i2c DTS");
#else
    printk ("[sm5414_init] init start. ch=%d\n", sm5414_BUSNUM);
    i2c_register_board_info(sm5414_BUSNUM, &i2c_sm5414, 1);
#endif
    if(i2c_add_driver(&sm5414_driver)!=0)
    {
        printk ("[sm5414_init] failed to register sm5414 i2c driver.\n");
    }
    else
    {
       printk ("[sm5414_init] Success to register sm5414 i2c driver.\n");
    }
    //add by caozhg
    if (platform_driver_register(&sm5414_gpio_driver) != 0) {
		printk ( "unable to register sm5414 gpio driver.\n");
		return -1;
    }
    //end

    // sm5414 user space access interface
    ret = platform_device_register(&sm5414_user_space_device);
    if (ret) {
        printk ( "****[sm5414_init] Unable to device register(%d)\n", ret);
        return ret;
    }
    ret = platform_driver_register(&sm5414_user_space_driver);
    if (ret) {
       printk ("****[sm5414_init] Unable to register driver (%d)\n", ret);
        return ret;
    }
    
    return 0;        
}

static void __exit sm5414_exit(void)
{
    i2c_del_driver(&sm5414_driver);
}

//module_init(sm5414_init);
//module_exit(sm5414_exit);
subsys_initcall(sm5414_subsys_init);
   
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C sm5414 Driver");
