#include <linux/types.h>
#include <linux/init.h>     /* For init/exit macros */
#include <linux/module.h>   /* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include "bq24157.h"
#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
#include <mt-plat/charger_class.h>
#include "mtk_charger_intf.h"
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */

#ifdef CONFIG_TINNO_JEITA_CURRENT
#ifdef CONFIG_TINNO_CUSTOM_CHARGER_PARA
#include "cust_charging.h"
#else
#include "cust_charger_init.h"
#endif
#endif

//#define CHG_DEBUG
#ifdef CHG_DEBUG
#define CHARGER_DEBUG(fmt, args...) printk(KERN_ERR "[BQ24157][%s]"fmt"\n", __func__, ##args)
#else
#define CHARGER_DEBUG(fmt, args...)
#endif
#define CHARGER_ERR(fmt, args...) printk(KERN_ERR "[BQ24157][%s]"fmt"\n", __func__, ##args)

struct bq24157_info
{
    struct device *dev;
    struct i2c_client *i2c;
#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
    struct charger_device *chg_dev;
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */
    u8 chip_rev;
};

static const struct bq24157_platform_data bq24157_def_platform_data =
{
    .chg_name = "primary_chg",
    .ichg = 1550000, /* unit: uA */
    .aicr = 500000, /* unit: uA */
    .mivr = 4500000, /* unit: uV */
    .ieoc = 150000, /* unit: uA */
    .voreg = 4350000, /* unit : uV */
    .vmreg = 4350000, /* unit : uV */
    .intr_gpio = 76,
};


static struct i2c_client *new_client;

#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
unsigned char bq24157_reg[bq24157_REG_NUM] = { 0 };

static DEFINE_MUTEX(bq24157_i2c_access);

int g_bq24157_hw_exist = 0;

u32 charging_parameter_to_value(const u32 *parameter, const u32 array_size, const u32 val)
{
    u32 i;

    for (i = 0; i < array_size; i++)
        if (val == *(parameter + i))
            return i;

    //chr_info("NO register value match \r\n");

    return 0;
}


static u32 bmt_find_closest_level(const u32 *pList, u32 number, u32 level)
{
    u32 i;
    u32 max_value_in_last_element;

    if (pList[0] < pList[1])
        max_value_in_last_element = true;
    else
        max_value_in_last_element = false;

    if (max_value_in_last_element == true)
    {
        for (i = (number - 1); i != 0; i--) /* max value in the last element */
            if (pList[i] <= level)
                return pList[i];

        //chr_info("Can't find closest level, small value first \r\n");
        return pList[0];
        /* return CHARGE_CURRENT_0_00_MA; */
    }
    else
    {
        for (i = 0; i < number; i++)    /* max value in the first element */
            if (pList[i] <= level)
                return pList[i];

        //chr_info("Can't find closest level, large value first \r\n");
        return pList[number - 1];
        /* return CHARGE_CURRENT_0_00_MA; */
    }
}

/**********************************************************
  *
  *   [I2C Function For Read/Write bq24157]
  *
  *********************************************************/
/*int bq24157_read_byte(unsigned char cmd, unsigned char *returnData)
{
    char buf[1] = { 0x00 };
    char readData = 0;
    int ret = 0;
    mutex_lock(&bq24157_i2c_access);
    ret = i2c_smbus_read_i2c_block_data(new_client, cmd, 1, buf);
    if (ret < 0) {
        printk("bq24157 read byte error\n");
        mutex_unlock(&bq24157_i2c_access);
        return 0;
    }
    mutex_unlock(&bq24157_i2c_access);
    readData = buf[0];
    *returnData = readData;
    return 1;
}*/
int bq24157_read_byte(unsigned char cmd, unsigned char *returnData)
{
    int ret = 0;
    struct i2c_msg msgs[] =
    {
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
        },
    };
    mutex_lock(&bq24157_i2c_access);
    ret = i2c_transfer(new_client->adapter, msgs, 2);

    mutex_unlock(&bq24157_i2c_access);
    return 1;
}


/*int bq24157_write_byte(unsigned char cmd, unsigned char writeData)
{
    char write_data[2] = { 0 };
    int ret = 0;

    mutex_lock(&bq24157_i2c_access);

    write_data[0] = cmd;
    write_data[1] = writeData;

    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) {
        printk("bq24157 write byte error\n");
        mutex_unlock(&bq24157_i2c_access);
        return 0;
    }

    mutex_unlock(&bq24157_i2c_access);
    return 1;
}*/

int bq24157_write_byte(unsigned char cmd, unsigned char writeData)
{
    char write_data[2] = { 0 };
    int ret = 0;

    struct i2c_msg msgs[] =
    {
        {
            .addr = new_client->addr,
            .flags = 0,
            .len = 2,
            .buf = write_data,
        },
    };

    write_data[0] = cmd;
    write_data[1] = writeData;
    mutex_lock(&bq24157_i2c_access);

    ret = i2c_transfer(new_client->adapter, msgs, 1);
    if (ret < 0)
    {
        pr_err("bq24157_write_byte failed\n");
		mutex_unlock(&bq24157_i2c_access);
        return ret;
    }
    mutex_unlock(&bq24157_i2c_access);
    return 1;
}


/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
unsigned int bq24157_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK,
                                    unsigned char SHIFT)
{
    unsigned char bq24157_reg = 0;
    int ret = 0;
    ret = bq24157_read_byte(RegNum, &bq24157_reg);
    //chr_info("[bq24157_read_interface] Reg[%x]=0x%x\n", RegNum, bq24157_reg);

    bq24157_reg &= (MASK << SHIFT);
    *val = (bq24157_reg >> SHIFT);

    //chr_info("[bq24157_read_interface] val=0x%x\n", *val);

    return ret;
}

unsigned int bq24157_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK,
                                      unsigned char SHIFT)
{
    unsigned char bq24157_reg = 0;
    int ret = 0;

    ret = bq24157_read_byte(RegNum, &bq24157_reg);
    //chr_info("[bq24157_config_interface] Reg[%x]=0x%x\n", RegNum, bq24157_reg);


    bq24157_reg &= ~(MASK << SHIFT);
    bq24157_reg |= (val << SHIFT);

    if (RegNum == bq24157_CON4 && val == 1 && MASK == CON4_RESET_MASK
        && SHIFT == CON4_RESET_SHIFT)
    {
        /* RESET bit */
    }
    else if (RegNum == bq24157_CON4)
    {
        bq24157_reg &= ~0x80;   /* RESET bit read returs 1, so clear it */
    }


    ret = bq24157_write_byte(RegNum, bq24157_reg);

    //chr_info("[bq24157_config_interface] write Reg[%x]=0x%x\n", RegNum,
    //    bq24157_reg);

    return ret;
}

/* write one register directly */
unsigned int bq24157_reg_config_interface(unsigned char RegNum, unsigned char val)
{
    int ret = 0;

    ret = bq24157_write_byte(RegNum, val);

    return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
/* CON0 */

void bq24157_set_tmr_rst(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON0),
                                   (unsigned char) (val),
                                   (unsigned char) (CON0_TMR_RST_MASK),
                                   (unsigned char) (CON0_TMR_RST_SHIFT)
                                  );
}

unsigned int bq24157_get_otg_status(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON0),
                                 (&val), (unsigned char) (CON0_OTG_MASK),
                                 (unsigned char) (CON0_OTG_SHIFT)
                                );
    return val;
}

void bq24157_set_en_stat(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON0),
                                   (unsigned char) (val),
                                   (unsigned char) (CON0_EN_STAT_MASK),
                                   (unsigned char) (CON0_EN_STAT_SHIFT)
                                  );
}

unsigned int bq24157_get_chip_status(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON0),
                                 (&val), (unsigned char) (CON0_STAT_MASK),
                                 (unsigned char) (CON0_STAT_SHIFT)
                                );
    return val;
}

unsigned int bq24157_get_boost_status(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON0),
                                 (&val), (unsigned char) (CON0_BOOST_MASK),
                                 (unsigned char) (CON0_BOOST_SHIFT)
                                );
    return val;
}

unsigned int bq24157_get_fault_status(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON0),
                                 (&val), (unsigned char) (CON0_FAULT_MASK),
                                 (unsigned char) (CON0_FAULT_SHIFT)
                                );
    return val;
}

/* CON1 */

void bq24157_set_input_charging_current(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON1),
                                   (unsigned char) (val),
                                   (unsigned char) (CON1_LIN_LIMIT_MASK),
                                   (unsigned char) (CON1_LIN_LIMIT_SHIFT)
                                  );
}

void bq24157_set_v_low(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON1),
                                   (unsigned char) (val),
                                   (unsigned char) (CON1_LOW_V_MASK),
                                   (unsigned char) (CON1_LOW_V_SHIFT)
                                  );
}

void bq24157_set_te(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON1),
                                   (unsigned char) (val),
                                   (unsigned char) (CON1_TE_MASK),
                                   (unsigned char) (CON1_TE_SHIFT)
                                  );
}

void bq24157_set_ce(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON1),
                                   (unsigned char) (val),
                                   (unsigned char) (CON1_CE_MASK),
                                   (unsigned char) (CON1_CE_SHIFT)
                                  );
}

void bq24157_set_hz_mode(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON1),
                                   (unsigned char) (val),
                                   (unsigned char) (CON1_HZ_MODE_MASK),
                                   (unsigned char) (CON1_HZ_MODE_SHIFT)
                                  );
}

void bq24157_set_opa_mode(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON1),
                                   (unsigned char) (val),
                                   (unsigned char) (CON1_OPA_MODE_MASK),
                                   (unsigned char) (CON1_OPA_MODE_SHIFT)
                                  );
}

/* CON2 */

void bq24157_set_oreg(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON2),
                                   (unsigned char) (val),
                                   (unsigned char) (CON2_OREG_MASK),
                                   (unsigned char) (CON2_OREG_SHIFT)
                                  );
}

void bq24157_set_otg_pl(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON2),
                                   (unsigned char) (val),
                                   (unsigned char) (CON2_OTG_PL_MASK),
                                   (unsigned char) (CON2_OTG_PL_SHIFT)
                                  );
}

void bq24157_set_otg_en(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON2),
                                   (unsigned char) (val),
                                   (unsigned char) (CON2_OTG_EN_MASK),
                                   (unsigned char) (CON2_OTG_EN_SHIFT)
                                  );
}

/* CON3 */

unsigned int bq24157_get_vender_code(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON3),
                                 (&val), (unsigned char) (CON3_VENDER_CODE_MASK),
                                 (unsigned char) (CON3_VENDER_CODE_SHIFT)
                                );
    return val;
}

unsigned int bq24157_get_pn(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON3),
                                 (&val), (unsigned char) (CON3_PIN_MASK),
                                 (unsigned char) (CON3_PIN_SHIFT)
                                );
    return val;
}

unsigned int bq24157_get_revision(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON3),
                                 (&val), (unsigned char) (CON3_REVISION_MASK),
                                 (unsigned char) (CON3_REVISION_SHIFT)
                                );
    return val;
}

/* CON4 */

void bq24157_set_reset(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON4),
                                   (unsigned char) (val),
                                   (unsigned char) (CON4_RESET_MASK),
                                   (unsigned char) (CON4_RESET_SHIFT)
                                  );
}

void bq24157_set_iocharge(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON4),
                                   (unsigned char) (val),
                                   (unsigned char) (CON4_I_CHR_MASK),
                                   (unsigned char) (CON4_I_CHR_SHIFT)
                                  );
}

void bq24157_set_iterm(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON4),
                                   (unsigned char) (val),
                                   (unsigned char) (CON4_I_TERM_MASK),
                                   (unsigned char) (CON4_I_TERM_SHIFT)
                                  );
}

/* CON5 */

void bq24157_set_dis_vreg(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON5),
                                   (unsigned char) (val),
                                   (unsigned char) (CON5_DIS_VREG_MASK),
                                   (unsigned char) (CON5_DIS_VREG_SHIFT)
                                  );
}

void bq24157_set_io_level(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON5),
                                   (unsigned char) (val),
                                   (unsigned char) (CON5_IO_LEVEL_MASK),
                                   (unsigned char) (CON5_IO_LEVEL_SHIFT)
                                  );
}

unsigned int bq24157_get_sp_status(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON5),
                                 (&val), (unsigned char) (CON5_SP_STATUS_MASK),
                                 (unsigned char) (CON5_SP_STATUS_SHIFT)
                                );
    return val;
}

unsigned int bq24157_get_en_level(void)
{
    unsigned int ret = 0;
    unsigned char val = 0;

    ret = bq24157_read_interface((unsigned char) (bq24157_CON5),
                                 (&val), (unsigned char) (CON5_EN_LEVEL_MASK),
                                 (unsigned char) (CON5_EN_LEVEL_SHIFT)
                                );
    return val;
}

void bq24157_set_vsp(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON5),
                                   (unsigned char) (val),
                                   (unsigned char) (CON5_VSP_MASK),
                                   (unsigned char) (CON5_VSP_SHIFT)
                                  );
}

/* CON6 */

void bq24157_set_i_safe(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON6),
                                   (unsigned char) (val),
                                   (unsigned char) (CON6_ISAFE_MASK),
                                   (unsigned char) (CON6_ISAFE_SHIFT)
                                  );
}

void bq24157_set_v_safe(unsigned int val)
{
    unsigned int ret = 0;

    ret = bq24157_config_interface((unsigned char) (bq24157_CON6),
                                   (unsigned char) (val),
                                   (unsigned char) (CON6_VSAFE_MASK),
                                   (unsigned char) (CON6_VSAFE_SHIFT)
                                  );
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void bq24157_dump_register(void)
{
    int i = 0;

    //chr_info("[bq24157_dump_register] ");
    for (i = 0; i < bq24157_REG_NUM; i++)
    {
        bq24157_read_byte(i, &bq24157_reg[i]);
        CHARGER_ERR("[0x%x]=0x%x", i, bq24157_reg[i]);
        //chr_info("[0x%x]=0x%x ", i, bq24157_reg[i]);
    }
    //chr_info("\n");
}

static int bq24157_chip_pdata_init(struct bq24157_info *ri)
{
	  bq24157_reg_config_interface(0x06, 0x5a);
    bq24157_reg_config_interface(0x00, 0xC0);   /* kick chip watch dog */
    bq24157_reg_config_interface(0x01, 0x78);   /* TE=1, CE=0, HZ_MODE=0, OPA_MODE=0 */
    bq24157_reg_config_interface(0x02, 0xaa);
    bq24157_reg_config_interface(0x05, 0x03);
	
    bq24157_reg_config_interface(0x06, 0x78);// ISAFE = (6.8mV * 0x07+ 37.4 mV ) /68  mohm = 1250 mA, VSAFE = 4.36V
    bq24157_set_iterm((TERMINATION_CURRENT-50000)/50000);//(3.4mV * 0x02+ 3.4 mV ) /68  mohm = 150 mA

    return 0;
}

static const unsigned long bq24157_support_iaicr[] = { 100, 500, 800, 1000};
static  int bq24157_set_aicr(unsigned long uA)
{
    u8 iaicr =0;
    unsigned long mA = uA / 1000;
    int i = 0;
    /* change by sw control */

    for (i = 0; i < ARRAY_SIZE(bq24157_support_iaicr); i++)
    {
        if (mA < bq24157_support_iaicr[i])
            break;
    }
    if (i == ARRAY_SIZE(bq24157_support_iaicr))
        pr_err("will config to no limit\n");
    if (i > 0)
        i--;
    switch (i)
    {
        case 0:
            iaicr = 0;//100 ma input current limit
            break;
        case 1:
            iaicr = 1;//500 ma input current limit
            break;
        case 2:
            iaicr = 2;//800 ma input current limit
            break;
        case 3:
            iaicr = 3;//No input current limit
            break;
        default:
            pr_err("illegal selection\n");
            return -EINVAL;
    }
	CHARGER_DEBUG("value=%d",iaicr);
    bq24157_set_input_charging_current(iaicr);
    return 0;
}

static  int bq24157_set_cvreg(unsigned long uV)
{
    u8 data = 0;

    if (uV >= 4400000)
    {
        data = 0x2d;//cv 4.4v
    }
    else if (uV >= 4330000)
    {
        data = 0x2a;//cv 4.34v
    }
    else if (uV >= 4200000)
    {
        data = 0x23;//cv 4.2v
    }
    else if (uV >= 3500000)
    {
        data = 0x1f;//cv 4.1v
    }
    else
    {
        data = 0x23;//cv 4.2v
    }
	CHARGER_ERR("bq24157_set_cvreg:CV = %d,data = 0x%x",(u32)uV,data);
    bq24157_set_oreg(data);

    return 0;
}

const u32 CS_VTH[] =//only for Rsns = 68 mohm
{
    550000, 650000, 750000,850000,
    950000, 1050000, 1150000,1250000
};

static  int bq24157_set_ichg(u32 uA)
{
    u32 status = 0;
    u32 set_chr_current;
    u32 array_size;
    u32 register_value;
    u32 current_value = uA;

    if (current_value <= 350000)
    {
        bq24157_set_io_level(1);
    }
    else
    {
        bq24157_set_io_level(0);
        array_size = GETARRAYNUM(CS_VTH);
        set_chr_current = bmt_find_closest_level(CS_VTH, array_size, current_value);
        register_value = charging_parameter_to_value(CS_VTH, array_size, set_chr_current);
		CHARGER_DEBUG("set_chr_current = %d,register_value=%d",set_chr_current,register_value);
        bq24157_set_iocharge(register_value);
    }
    return status;
}


#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
static int bq24157_charger_is_charging_done(struct charger_device *chg_dev,
        bool *done)
{
    int ret = 0;
    CHARGER_DEBUG();
    ret = bq24157_get_chip_status();
    if (ret < 0)
        return ret;

    if (ret == 0x02)
        *done = true;
    else
        *done = false;
    return 0;
}

static int bq24157_charger_enable_otg(struct charger_device *chg_dev, bool en)
{
    CHARGER_DEBUG();

    if(en)
    {
        bq24157_set_opa_mode(1);
        bq24157_set_otg_pl(1);
        bq24157_set_otg_en(1);
    }
    else
    {
        bq24157_set_opa_mode(0);
        bq24157_set_otg_pl(0);
        bq24157_set_otg_en(0);
    }

    return 0;
}

static int bq24157_charger_enable_te(struct charger_device *chg_dev, bool en)
{
    //do nothing
    CHARGER_DEBUG();
    return 0;
}

static int bq24157_charger_enable_timer(struct charger_device *chg_dev, bool en)
{
    CHARGER_DEBUG();
    bq24157_set_tmr_rst(1);
    return 0;
}

static int bq24157_charger_is_timer_enabled(struct charger_device *chg_dev,
        bool *en)
{
    CHARGER_DEBUG();
    bq24157_set_tmr_rst(0);
    return 0;
}

static int bq24157_charger_set_mivr(struct charger_device *chg_dev, u32 uV)
{
    //do nothing
    CHARGER_DEBUG();
    return 0;
}

static int bq24157_charger_get_min_aicr(struct charger_device *chg_dev, u32 *uA)
{
    //do nothing
    CHARGER_DEBUG();
    *uA = 100000;
    return 0;
}

static int bq24157_charger_set_aicr(struct charger_device *chg_dev, u32 uA)
{
    pr_err("set input current ua=%d\n",uA);
    return bq24157_set_aicr(uA);
}

static int bq24157_charger_get_aicr(struct charger_device *chg_dev, u32 *uA)
{
    //do nothing
    CHARGER_DEBUG();
    return 0;
}

static int bq24157_charger_get_cv(struct charger_device *chg_dev, u32 *uV)
{
    //do nothing
    CHARGER_DEBUG();
    return 0;
}

static int bq24157_charger_set_cv(struct charger_device *chg_dev, u32 uV)
{
    CHARGER_DEBUG("uV=%d",uV);
    return bq24157_set_cvreg(uV);
}

static int bq24157_charger_get_min_ichg(struct charger_device *chg_dev, u32 *uA)
{
    //do nothing
    CHARGER_DEBUG();
    *uA = 500000;
    return 0;
}

static int bq24157_charger_set_ichg(struct charger_device *chg_dev, u32 uA)
{
    pr_err("set charge current=%d\n",uA);
    return bq24157_set_ichg(uA);
}

static int bq24157_charger_get_ichg(struct charger_device *chg_dev, u32 *uA)
{
    //do nothing
    CHARGER_DEBUG();
    return 0;
}

static int bq24157_charger_is_enabled(struct charger_device *chg_dev, bool *en)
{
    CHARGER_DEBUG();
    bq24157_set_ce(1);
    return 0;
}

static int bq24157_charger_enable(struct charger_device *chg_dev, bool en)
{
    CHARGER_DEBUG();
	if (en) {
		bq24157_set_ce(0);
		bq24157_set_hz_mode(0);
		bq24157_set_opa_mode(0);
	} else {
#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if (mt_usb_is_device())
#endif
 
			bq24157_set_ce(1);
	}
    return 0;
}

static int bq24157_charger_plug_out(struct charger_device *chg_dev)
{
    //do nothing
    return 0;
}

static int bq24157_charger_plug_in(struct charger_device *chg_dev)
{
    //do nothing
    CHARGER_DEBUG();
    return 0;
}

static int bq24157_charger_dump_registers(struct charger_device *chg_dev)
{
    CHARGER_DEBUG();
    bq24157_dump_register();
    return 0;
}

static int bq24157_charger_do_event(struct charger_device *chg_dev, u32 event,
                                    u32 args)
{
    struct bq24157_info *ri = charger_get_data(chg_dev);
    CHARGER_DEBUG("event=%d",event);
    switch (event)
    {
        case EVENT_EOC:
            charger_dev_notify(ri->chg_dev, CHARGER_DEV_NOTIFY_EOC);
            break;
        case EVENT_RECHARGE:
            charger_dev_notify(ri->chg_dev, CHARGER_DEV_NOTIFY_RECHG);
            break;
        default:
            break;
    }
    return 0;
}

static const struct charger_ops bq24157_chg_ops =
{
    /* cable plug in/out */
    .plug_in = bq24157_charger_plug_in,
    .plug_out = bq24157_charger_plug_out,
    /* enable */
    .enable = bq24157_charger_enable,
    .is_enabled = bq24157_charger_is_enabled,
    /* charging current */
    .get_charging_current = bq24157_charger_get_ichg,
    .set_charging_current = bq24157_charger_set_ichg,
    .get_min_charging_current = bq24157_charger_get_min_ichg,
    /* charging voltage */
    .set_constant_voltage = bq24157_charger_set_cv,
    .get_constant_voltage = bq24157_charger_get_cv,
    /* charging input current */
    .get_input_current = bq24157_charger_get_aicr,
    .set_input_current = bq24157_charger_set_aicr,
    .get_min_input_current = bq24157_charger_get_min_aicr,
    /* charging mivr */
    .set_mivr = bq24157_charger_set_mivr,
    /* safety timer */
    .is_safety_timer_enabled = bq24157_charger_is_timer_enabled,
    .enable_safety_timer = bq24157_charger_enable_timer,
    /* charing termination */
    .enable_termination = bq24157_charger_enable_te,
    /* OTG */
    .enable_otg = bq24157_charger_enable_otg,
    /* misc */
    .is_charging_done = bq24157_charger_is_charging_done,
    .dump_registers = bq24157_charger_dump_registers,
    /* event */
    .event = bq24157_charger_do_event,
};

static const struct charger_properties bq24157_chg_props =
{
    .alias_name = "bq24157",
};
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */

unsigned char g_reg_value_bq24157 = 0;
static ssize_t show_bq24157_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i=0;
	int offset = 0;

	bq24157_dump_register();
	for(i = 0; i<7; i++)
	{
		offset += sprintf(buf+offset, "bq24157:reg[0x%x] = 0x%x\n", i, bq24157_reg[i]);
	}

	return offset;
}

static ssize_t store_bq24157_access(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_err("[store_bq24157_access]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_bq24157_access] buf is %s and size is %zu\n", buf, size);
		/*reg_address = kstrtoul(buf, 16, &pvalue);*/

		pvalue = (char *)buf;
		if (size > 3) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);

		if (size > 3) {
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);

			pr_err("[store_bq24157_access] write bq24157 reg 0x%x with value 0x%x !\n",reg_address, reg_value);
			ret = bq24157_config_interface(reg_address, reg_value, 0xFF, 0x0);
		} else {
			ret = bq24157_read_interface(reg_address, &g_reg_value_bq24157, 0xFF, 0x0);
			pr_err("[store_bq24157_access] read bq24157 reg 0x%x with value 0x%x !\n",reg_address, g_reg_value_bq24157);
			pr_err("[store_bq24157_access] Please use \"cat bq24157_access\" to get value\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(bq24157_access, 0664, show_bq24157_access, store_bq24157_access);	/* 664 */

static int bq24157_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	pr_err("******** bq24157_user_space_probe!! ********\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_bq24157_access);

	return 0;
}

struct platform_device bq24157_user_space_device = {
	.name = "bq24157-user",
	.id = -1,
};

static struct platform_driver bq24157_user_space_driver = {
	.probe = bq24157_user_space_probe,
	.driver = {
		.name = "bq24157-user",
	},
};

static void bq_parse_dt(struct device *dev, struct bq24157_platform_data *pdata)
{
    /* just used to prevent the null parameter */
    if (!dev || !pdata)
        return;
    if (of_property_read_string(dev->of_node, "charger_name",
                                &pdata->chg_name) < 0)
        dev_warn(dev, "not specified chg_name\n");
    if (of_property_read_u32(dev->of_node, "ichg", &pdata->ichg) < 0)
        dev_warn(dev, "not specified ichg value\n");
    if (of_property_read_u32(dev->of_node, "aicr", &pdata->aicr) < 0)
        dev_warn(dev, "not specified aicr value\n");
    if (of_property_read_u32(dev->of_node, "mivr", &pdata->mivr) < 0)
        dev_warn(dev, "not specified mivr value\n");
    if (of_property_read_u32(dev->of_node, "ieoc", &pdata->ieoc) < 0)
        dev_warn(dev, "not specified ieoc_value\n");
    if (of_property_read_u32(dev->of_node, "cv", &pdata->voreg) < 0)
        dev_warn(dev, "not specified cv value\n");
    if (of_property_read_u32(dev->of_node, "vmreg", &pdata->vmreg) < 0)
        dev_warn(dev, "not specified vmreg value\n");
    //pdata->enable_te = of_property_read_bool(dev->of_node, "enable_te");
    pdata->enable_eoc_shdn = of_property_read_bool(dev->of_node,
                             "enable_eoc_shdn");
}

static int bq24157_i2c_probe(struct i2c_client *i2c,
                             const struct i2c_device_id *id)
{
    struct bq24157_platform_data *pdata = dev_get_platdata(&i2c->dev);
    struct bq24157_info *ri = NULL;
    u8 rev_id = 0;
    bool use_dt = i2c->dev.of_node;
    int ret = 0;
    //dev_info(&i2c->dev, "%s start\n", __func__);
	ret = platform_device_register(&bq24157_user_space_device);
	if (ret) {
		pr_err("****[bq24157_init] Unable to device register(%d)\n",ret);
		return ret;
	}
	ret = platform_driver_register(&bq24157_user_space_driver);
	if (ret) {
		pr_err("****[bq24157_init] Unable to register driver (%d)\n",ret);
		return ret;
	}
    /* if success, return value is revision id */
    rev_id = (u8)ret;
    /* driver data */
    ri = devm_kzalloc(&i2c->dev, sizeof(*ri), GFP_KERNEL);
    if (!ri)
    {
        return -ENOMEM;
    }
    /* platform data */
    if (use_dt)
    {
        pdata = devm_kzalloc(&i2c->dev, sizeof(*pdata), GFP_KERNEL);
        if (!pdata)
            return -ENOMEM;
        memcpy(pdata, &bq24157_def_platform_data, sizeof(*pdata));
        i2c->dev.platform_data = pdata;
        bq_parse_dt(&i2c->dev, pdata);
    }
    else
    {
        if (!pdata)
        {
            pr_err("no pdata specify\n");
            return -EINVAL;
        }
    }

    new_client = i2c;
    //ret = bq24157_read_interface(0x03, &val, 0xFF, 0x0);;
    ri->dev = &i2c->dev;
    ri->i2c = i2c;
    ri->chip_rev = rev_id;
    i2c_set_clientdata(i2c, ri);

    ret = bq24157_chip_pdata_init(ri);
    if (ret < 0)
    {
        pr_err("bq24157 bq24157_chip_pdata_init failed\n\n");
        return ret;
    }
    else
    {
        pr_err("bq24157_chip_pdata_init success\n");
    }
    bq24157_dump_register();
#if defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
    /* charger class register */
    ri->chg_dev = charger_device_register(pdata->chg_name, ri->dev, ri,
                                          &bq24157_chg_ops,
                                          &bq24157_chg_props);
    if (IS_ERR(ri->chg_dev))
    {
        pr_err("charger device register fail\n");
        return PTR_ERR(ri->chg_dev);
    }
#endif /* #if (CONFIG_MTK_GAUGE_VERSION == 30) */
    return 0;
}

static int bq24157_i2c_remove(struct i2c_client *i2c)
{
    //do nothing
    return 0;
}

static void bq24157_i2c_shutdown(struct i2c_client *i2c)
{
    //do nothing
}

static int bq24157_i2c_suspend(struct device *dev)
{
    //do nothing
    return 0;
}

static int bq24157_i2c_resume(struct device *dev)
{
    //do nothing
    return 0;
}

static SIMPLE_DEV_PM_OPS(bq24157_pm_ops, bq24157_i2c_suspend, bq24157_i2c_resume);

static const struct of_device_id of_id_table[] =
{
    { .compatible = "mediatek,bq24157"},
    {},
};
MODULE_DEVICE_TABLE(of, of_id_table);

static const struct i2c_device_id i2c_id_table[] =
{
    { "bq24157", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, i2c_id_table);

static struct i2c_driver bq24157_i2c_driver =
{
    .driver = {
        .name = "bq24157",
        .owner = THIS_MODULE,
        .pm = &bq24157_pm_ops,
        .of_match_table = of_match_ptr(of_id_table),
    },
    .probe = bq24157_i2c_probe,
    .remove = bq24157_i2c_remove,
    .shutdown = bq24157_i2c_shutdown,
    .id_table = i2c_id_table,
};
module_i2c_driver(bq24157_i2c_driver);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C bq24157 Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
