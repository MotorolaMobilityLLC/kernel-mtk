
#include <linux/types.h>
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
//#include <mt-plat/mtk_boot.h>
//#include <mt-plat/upmu_common.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include "wt6670f.h"
#include "wt6670f_firmware.h"
#include <asm/neon.h>

static DEFINE_MUTEX(wt6670f_i2c_access);
//static DEFINE_MUTEX(wt6670f_access_lock);
//static struct i2c_client *new_client;
//static int wt6670f_reset_pin = -1;
static int wt6670f_int_pin = -1;

struct pinctrl *i2c6_pinctrl;
struct pinctrl_state *i2c6_i2c;
struct pinctrl_state *i2c6_scl_low;
struct pinctrl_state *i2c6_scl_high;
struct pinctrl_state *i2c6_sda_low;
struct pinctrl_state *i2c6_sda_high;

struct wt6670f *_wt = NULL;
int is_already_probe_ok = 0;
int g_qc3p_id = 0;
int m_chg_type = 0;
bool qc3p_z350_init_ok = false;
EXPORT_SYMBOL_GPL(is_already_probe_ok);
EXPORT_SYMBOL_GPL(qc3p_z350_init_ok);
EXPORT_SYMBOL_GPL(g_qc3p_id);
EXPORT_SYMBOL_GPL(m_chg_type);


// firme update
/**
 * Enter ISP Mode Sequence
 */
void wt_enter_isp_mode_sequence(struct wt6670f *chip)
{
#if 1
	mutex_lock(&chip->i2c_rw_lock);

	gpio_direction_output(chip->reset_pin, 0);
	msleep(5);
	gpio_direction_output(chip->reset_pin, 1);
	msleep(1);
	gpio_direction_output(chip->reset_pin, 0);
	mdelay(3);

	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(2);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(2);

	/* sequence */
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//1
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//2
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//3
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//4
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//5
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//6
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//7
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//8
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//9
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(8);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//10
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//11
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//12
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//13
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//14
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(4);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//15
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//16
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	udelay(5);
	/* sequence end */

	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//17
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);	//18
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_low);
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_low);
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_scl_high);
	udelay(5);
	pinctrl_select_state(i2c6_pinctrl, i2c6_sda_high);

	msleep(10);

	pinctrl_select_state(i2c6_pinctrl, i2c6_i2c);

	mutex_unlock(&chip->i2c_rw_lock);
#else
	int ret;
	u8 data[1]={0x00};
	struct i2c_msg msgs[] = {
        {
			.addr = 0x2B,
			.flags = 0,
			.len = 1,
			.buf = data,
        },
        {
			.addr = 0x48,
			.flags = 0,
			.len = 1,
			.buf = data,
        },
    };

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, msgs, 2);
	if (ret != 2)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	 mutex_unlock(&chip->i2c_rw_lock);
#endif
}

/**
 * ENABLE ISP Command
 * (1) This command is used to enable ISP through I2C interface.
 * (2) WT6670F will ACK all bytes, HOST must use READ CHIP ID command to
 * check that ISP mode is enabled
 */
int wt_enable_isp(struct wt6670f *chip)
{
	u8 cmd_buf[7] = {0x57, 0x54, 0x36, 0x36, 0x37, 0x30, 0x46};
	struct i2c_msg msg;
	int ret = 0;

	memset(&msg, 0, sizeof(struct i2c_msg));

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0;
	msg.buf = cmd_buf;
	msg.len = sizeof(cmd_buf);

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

/**
 * HOLD 8051 Command
 * (1) This command will hold 8051 CPU and stop executing instruction.
 */
int wt_hold_8051(struct wt6670f *chip)
{
	u8 cmd_buf[3] = {0x10, 0x00, 0x01};
	struct i2c_msg msg;
	int ret = 0;

	memset(&msg, 0, sizeof(struct i2c_msg));

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0 | I2C_M_STOP;
	msg.buf = cmd_buf;
	msg.len = sizeof(cmd_buf);

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

/**
 * READ CHIP ID Command
 * (1) The Chip ID of WT6670F is 0x70H
 */
int wt_read_chip_id(struct wt6670f *chip, u8 *r_buf, u8 len)		//id should be 0x70
{
	int ret = 0;
	struct i2c_adapter *adap = chip->client->adapter;
	struct i2c_msg msg[2];
	u8 w_buf[2] = {0x80, 0x00};

	memset(msg, 0, 2 * sizeof(struct i2c_msg));

	msg[0].addr = WT6670_ISP_I2C_ADDR;
	msg[0].flags = 0 | I2C_M_STOP;
	msg[0].len = sizeof(w_buf);
	msg[0].buf = w_buf;

	msg[1].addr = WT6670_ISP_I2C_ADDR;
	msg[1].flags = (I2C_M_STOP & I2C_M_NOSTART) | I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = r_buf;

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(adap, msg, 2);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}


/**
 * ENABLE ISP FLASH MODE Command
 * (1) This command will enable ISP access control to flash memory
 */
int wt_enable_isp_flash_mode(struct wt6670f *chip)
{
	u8 cmd_buf[3] = {0x10, 0x02, 0x08};
	struct i2c_msg msg;
	int ret = 0;

	memset(&msg, 0, sizeof(struct i2c_msg));

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0 | I2C_M_STOP;
	msg.buf = cmd_buf;
	msg.len = sizeof(cmd_buf);

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

/**
 * CHIP ERASE Command
 * (1) This command will erase whole 4K bytes flash memory.
 * (2) After send this command, host must wait 20ms ~ 40ms
 * and then send FINISH command.
 */
int wt_chip_erase(struct wt6670f *chip)
{
	u8 cmd_buf[3] = {0x20, 0x00, 0x00};
	struct i2c_msg msg;
	int ret = 0;

	memset(&msg, 0, sizeof(struct i2c_msg));

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0 | I2C_M_STOP;
	msg.buf = cmd_buf;
	msg.len = sizeof(cmd_buf);

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

/**
 * SET ADDRESS HIGH-BYTE Command
 * (1) This command will set flash address bit11 ~ bit8.
 */
int wt_set_address_high_byte(struct wt6670f *chip, u8 addr_bit_15_8)
{
	u8 cmd_buf[3] = {0x10, 0x01, addr_bit_15_8};
	struct i2c_msg msg;
	int ret = 0;

	memset(&msg, 0, sizeof(struct i2c_msg));

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0 | I2C_M_STOP;
	msg.buf = cmd_buf;
	msg.len = sizeof(cmd_buf);

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

/**
 * PROGRAM Command
 * (1) ADDR[11:6] select a 64-byte row of flash memory.
 * ADDR[5:0] access one byte of one 64-byte row.
 * (2) When Host program 64-byte data from 0x0300 to 0x033F,
 * first set address ADDR[11:8] = 0x03,command is S+68+10+01+03+P.
 * And then, command S+68+41+00+64-byte data +P is to program 64-byte data.
 * (3) If I2C speed is 400khz and a series of data more than
 * 64 bytes will be programmed, Host needs to send a “FINISH COMMAND”
 * after the end of the 64nd data.
 * Ex. Program 64 bytes data from address 0x0300 to 0x033F
 * S+68+10+01+03+P
 * S+68+41+00+Data1+Data2+…+Data64+P
 * S+68+00+00+00+P
 * (4) If ISP I2C speed is 400Khz, a byte to byte delay is needed
 * for program flash spec.
 */
int wt_program_flash_64byte(struct wt6670f *chip, u8 addr_bit_7_0, u8 *buf, u8 len)
{
	int ret = 0;
	struct i2c_adapter *adap = chip->client->adapter;
	struct i2c_msg msg;
	u8 *w_buf = NULL;

	memset(&msg, 0, sizeof(struct i2c_msg));

	w_buf = kzalloc(len + 2, GFP_KERNEL);
	if (w_buf == NULL)
		return -1;

	w_buf[0] = 0x41;
	w_buf[1] = addr_bit_7_0;
	memcpy(w_buf + 2, buf, len);

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0 | I2C_M_STOP;
	msg.len = len + 2;
	//msg.len = 1 + 2;
	msg.buf = w_buf;

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(adap, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	kfree(w_buf);
	return ret;
}

/**
 * FINISH Command
 * (1) FINISH COMMAND stops erase/program function
 */
int wt_finish_erase_or_program(struct wt6670f *chip)
{
	u8 cmd_buf[3] = {0x00, 0x00, 0x00};
	struct i2c_msg msg;
	int ret = 0;

	memset(&msg, 0, sizeof(struct i2c_msg));

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0 | I2C_M_STOP;
	msg.buf = cmd_buf;
	msg.len = sizeof(cmd_buf);

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

/**
 * READ Command
 * (1) READ COMMAND can read maximum 64 bytes flash data.
 * (2) If Host read 64-byte data from 0x0700 ~ 0x073F,
 * set address ADDR[13:8] = 0x07 first.
 * READ COMMAND is S+68+61+00+rS+69+64-byte data+P
 */
int wt_read_flash_64byte(struct wt6670f *chip, u8 addr_bit_7_0, u8 *buf, u8 len)
{
	int ret = 0;
	struct i2c_msg msg[2];
	u8 w_buf[2] = {0x61, addr_bit_7_0};
	u8 *r_buf = NULL;

	memset(msg, 0, 2 * sizeof(struct i2c_msg));

	r_buf = kzalloc(len, GFP_KERNEL);
	if (r_buf == NULL)
		return -1;

	msg[0].addr = WT6670_ISP_I2C_ADDR;
	msg[0].flags = 0 | I2C_M_STOP;
	msg[0].len = sizeof(w_buf);
	msg[0].buf = w_buf;

	msg[1].addr = WT6670_ISP_I2C_ADDR;
	msg[1].flags = 1;
	msg[1].len = len;
	msg[1].buf = r_buf;

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, msg, 2);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	memcpy(buf, r_buf, len);

	kfree(r_buf);
	return ret;
}

/**
 * END Command
 * (1) END COMMAND will reset WT6670F.
 * Then WT6670F restarts to execute user’s code
 */
int wt_restart_chip(struct wt6670f *chip)
{
	u8 cmd_buf[3] = {0x10, 0x00, 0x02};
	struct i2c_msg msg;
	int ret = 0;

	memset(&msg, 0, sizeof(struct i2c_msg));

	msg.addr = WT6670_ISP_I2C_ADDR;
	msg.flags = 0 | I2C_M_STOP;
	msg.buf = cmd_buf;
	msg.len = sizeof(cmd_buf);

	mutex_lock(&chip->i2c_rw_lock);
	ret = i2c_transfer(chip->client->adapter, &msg, 1);
	if (ret < 0)
	{
		pr_info("[%s] i2c transfer failed, ret:%d\n", __func__, ret);
	}
	mutex_unlock(&chip->i2c_rw_lock);

	return ret;
}

int wt6670f_isp_flow(struct wt6670f *chip)
{
	u8 chip_id = 0;
	int programed_cnt = 0x0;
	int addr_bit_15_8 = 0x0;
	int addr_bit_7_0 = 0x0;
	u8 *code = NULL;
	int i = 0;
	int len = 0;
	int read_flash_cnt = 0x0;

	code = kzalloc(0x40, GFP_KERNEL);
	if (code == NULL)
		return -1;
#if 0
	pr_info("%s: i2c speed before set:%d\n", __func__, get_i2c_speed(6));
	set_i2c_speed(6, 100 * 1000);
	pr_info("%s: i2c speed after set:%d\n", __func__, get_i2c_speed(6));
#endif
	wt6670f_do_reset();
	wt_enter_isp_mode_sequence(chip);

	//while(check_i2c_bus_free);

	wt_enable_isp(chip);

	wt_read_chip_id(chip, &chip_id, 1);
	if (chip_id != 0x70)
	{
		pr_info("[%s] err, chip_id:%d, skip isp\n", __func__, chip_id);
		goto isp_err;
	}
	pr_info("[%s] , chip_id:%d, skip isp\n", __func__, chip_id);
	wt_enable_isp_flash_mode(chip);
	wt_chip_erase(chip);
	msleep(30);
	wt_finish_erase_or_program(chip);

	//wt_enable_isp_flash_mode(chip->client);

	while (programed_cnt < WT70F_QC3p_V02_210326_FBA1_bin_len)
	{
		addr_bit_15_8 = (programed_cnt >> 8) & 0x0f;
		addr_bit_7_0 = programed_cnt % 0x100;

		memset(code, 0, 0x40);

		wt_set_address_high_byte(chip, addr_bit_15_8);

		if ((programed_cnt + 0x40) > WT70F_QC3p_V02_210326_FBA1_bin_len)
		{
			len =  WT70F_QC3p_V02_210326_FBA1_bin_len % 0x40;
		}
		else
		{
			len = 0x40;
		}

		for (i = 0; i < len; i++)
		{
			code[i] = WT70F_QC3p_V02_210326_FBA1_bin[programed_cnt + i];
		}

		if (len != 0x40)
		{
			for (i = len; i < 0x40; i++)
			{
				code[i] = 0xff;
			}
		}

		wt_program_flash_64byte(chip, addr_bit_7_0, code, 0x40);

		wt_finish_erase_or_program(chip);
		programed_cnt += 0x40;
	}

	pr_info("[%s] finish program flash, start verify...\n", __func__);

	//wt_enable_isp_flash_mode(chip->client);

	while (read_flash_cnt < 0x1000)
	{
		addr_bit_15_8 = (read_flash_cnt >> 8) & 0x0f;
		addr_bit_7_0 = read_flash_cnt % 0x100;

		memset(code, 0, 0x40);

		wt_set_address_high_byte(chip, addr_bit_15_8);
		wt_read_flash_64byte(chip, addr_bit_7_0, code, 0x40);

		if (read_flash_cnt > WT70F_QC3p_V02_210326_FBA1_bin_len)
		{

		}
		else if (read_flash_cnt + 0x40 > WT70F_QC3p_V02_210326_FBA1_bin_len)
		{
			for (i = 0; i < WT70F_QC3p_V02_210326_FBA1_bin_len % 0x40; i++)
			{
				if (code[i] != WT70F_QC3p_V02_210326_FBA1_bin[read_flash_cnt + i])
				{
					pr_info("[%s] verify flash data failed, fail_addr:0x%x, fail_data:0x%x, correct_data:0x%x\n", __func__, read_flash_cnt + i,
						code[i], WT70F_QC3p_V02_210326_FBA1_bin[read_flash_cnt + i]);
				}
			}

			//need to verify 0xff
		}
		else
		{
			for (i = 0; i < 0x40; i++)
			{
				if (code[i] != WT70F_QC3p_V02_210326_FBA1_bin[read_flash_cnt + i])
				{
					pr_info("[%s] verify flash data failed, fail_addr:0x%x, fail_data:0x%x, correct_data:0x%x\n", __func__, read_flash_cnt + i,
						code[i], WT70F_QC3p_V02_210326_FBA1_bin[read_flash_cnt + i]);
				}
			}
		}

		read_flash_cnt += 0x40;
	}

	wt_restart_chip(chip);

	kfree(code);
#if 0
	set_i2c_speed(6, 400 * 1000);
	pr_info("%s: i2c speed resume:%dk\n", __func__, get_i2c_speed(6));
#endif
	return 0;

isp_err:
	wt_restart_chip(chip);
	kfree(code);
#if 0
	set_i2c_speed(6, 400 * 1000);
	pr_info("%s: i2c speed resume:%dk\n", __func__, get_i2c_speed(6));
#endif
	return -1;
}

static int __wt6670f_write_word(struct wt6670f *wt, u8 reg, u16 data)
{
	s32 ret;

	ret = i2c_smbus_write_word_data(wt->client, reg, data);
	if (ret < 0) {
		pr_err("i2c write fail: can't write from reg 0x%02X\n", reg);
		return ret;
	}

	return 0;
}


static int __wt6670f_read_word(struct wt6670f *wt, u8 reg, u16 *data)
{
	s32 ret;

	ret = i2c_smbus_read_word_data(wt->client, reg);
	if (ret < 0) {
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}

	*data = (u16) ret;

	return 0;
}

static int wt6670f_read_word(struct wt6670f *wt, u8 reg, u16 *data)
{
	int ret;

	mutex_lock(&wt->i2c_rw_lock);
	ret = __wt6670f_read_word(wt, reg, data);
	mutex_unlock(&wt->i2c_rw_lock);

	return ret;
}

static int wt6670f_write_word(struct wt6670f *wt, u8 reg, u16 data)
{
	int ret;

	mutex_lock(&wt->i2c_rw_lock);
	ret = __wt6670f_write_word(wt, reg, data);
	mutex_unlock(&wt->i2c_rw_lock);

	return ret;
}

#define WAIT_I2C_COUNT 50
#define WAIT_I2C_TIME 10
int mmi_wt6670f_write_word(struct wt6670f *wt, u8 reg, u16 data)
{
	int retry_count = 0;

	while (wt->wt6670f_suspend_flag && retry_count < WAIT_I2C_COUNT) {
		retry_count ++;
		dev_err(wt->dev, "wait system resume when I2C write, count %d\n", retry_count);
		msleep(WAIT_I2C_TIME);
	}

	if (retry_count >= WAIT_I2C_COUNT)
		return -EBUSY;

	return wt6670f_write_word(wt, reg, data);
}

int mmi_wt6670f_read_word(struct wt6670f *wt, u8 reg, u16 *data)
{
	int retry_count = 0;

	while (wt->wt6670f_suspend_flag && retry_count < WAIT_I2C_COUNT) {
		retry_count ++;
		dev_err(wt->dev, "wait system resume when I2C read, count %d\n", retry_count);
		msleep(WAIT_I2C_TIME);
	}

	if (retry_count >= WAIT_I2C_COUNT)
		return -EBUSY;

	return wt6670f_read_word(wt, reg, data);
}

u16 wt6670f_get_vbus_voltage(void)
{
	int ret;
	u8 data[2];
	u16 tmp;
	if(QC3P_WT6670F == g_qc3p_id)
		ret = mmi_wt6670f_read_word(_wt, 0xBE, (u16 *)data);
	else if(QC3P_Z350 == g_qc3p_id)
		ret = mmi_wt6670f_read_word(_wt, 0x12, (u16 *)data);
	else
		ret = -1;
	if(ret < 0)
	{
		pr_err("wt6670f get vbus voltage fail\n");
		return ret;
	}

	tmp = ((data[0] << 8) | data[1]) & 0x3ff;

	pr_err(">>>>>>wt6670f get vbus voltage = %04x  %02x  %02x\n", tmp,
			data[0], data[1]);
	kernel_neon_begin();
	tmp = (u16)(tmp * 1898 / 100);
	kernel_neon_end();
	return tmp;
}
EXPORT_SYMBOL_GPL(wt6670f_get_vbus_voltage);

static u16 wt6670f_get_id(u8 reg)
{
	int ret;
	u16 data;

	ret = mmi_wt6670f_read_word(_wt, reg, &data);//wt6670f-0xBC,Z350-0x13

	if(ret < 0)
	{
		pr_err("wt6670f get id fail\n");
		return ret;
	}

	pr_err(">>>>>>wt6670f get id = %x\n", data);
	return data;
}

int wt6670f_start_detection(void)
{
	int ret;
	u16 data = 0x01;

	ret = mmi_wt6670f_write_word(_wt, 0xB6, data);
	if (ret < 0)
	{
		pr_info("wt6670f start detection fail\n");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_start_detection);

int wt6670f_re_run_apsd(void)
{
	int ret;
	u16 data = 0x01;

	ret = mmi_wt6670f_write_word(_wt, 0xB6, data);

	if (ret < 0)
	{
		pr_info("wt6670f re run apsd fail\n");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_re_run_apsd);


int wt6670f_en_hvdcp(void)
{
	int ret;
	u16 data = 0x01;

	ret = mmi_wt6670f_write_word(_wt, 0x05, data);

	if (ret < 0)
	{
		pr_info("z350 en hvdcp fail\n");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_en_hvdcp);

int wt6670f_force_qc2_5V(void)
{
	int ret;
	u16 data = 0x01;

	if(1 == g_qc3p_id)
		ret = mmi_wt6670f_write_word(_wt, 0x02, data);
	else
		ret = mmi_wt6670f_write_word(_wt, 0xB1, data);
	if (ret < 0)
	{
		pr_info("%s force qc2 5V fail\n",g_qc3p_id?"z350":"wt6670f");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_force_qc2_5V);

int wt6670f_force_qc3_5V(void)
{
	int ret;
	u16 data = 0x04;

	if(1 == g_qc3p_id)
		ret = mmi_wt6670f_write_word(_wt, 0x02, data);
	else
		ret = mmi_wt6670f_write_word(_wt, 0xB1, data);

	if (ret < 0)
	{
		pr_info("%s force qc3 5V fail\n",g_qc3p_id?"z350":"wt6670f");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_force_qc3_5V);

int wt6670f_get_protocol(void)
{
	int ret;
	u16 data;
	u8 data1, data2;

	if(QC3P_WT6670F == g_qc3p_id){
		ret = mmi_wt6670f_read_word(_wt, 0xBD, &data);
		pr_err("wt6670f get protocol %x\n",data);
	}
	else if(QC3P_Z350 == g_qc3p_id){
		ret = mmi_wt6670f_read_word(_wt, 0x11, &data);
		pr_err("z350 get protocol %x\n",data);
	}
	else
		ret = -1;

	if (ret < 0)
	{
		pr_err("wt6670f get protocol fail\n");
		return ret;
	}

        // Get data2 part
        data1 = data & 0xFF;
        data2 = data >> 8;
	if((QC3P_Z350 == g_qc3p_id)&&(data1 <= 7))
		data1--;
        pr_err("Get charger type, rowdata = 0X%04x, data1= 0X%02x, data2=0X%02x \n", data, data1, data2);
        if((data2 == 0x03) && ((data1 > 0x9) || (data1 == 0x7)))
        {
                pr_err("fail to get charger type, error happens!\n");
                return -EINVAL;
        }

        if(data2 == 0x04)
        {
                pr_err("detected QC3+ charger:0X%02x!\n", data1);
        }

	if((data1 > 0x00 && data1 < 0x07) ||
           (data1 > 0x07 && data1 < 0x0a) ||(QC3P_Z350 == g_qc3p_id && data1 == 0x10)){
		ret = data1;
	}
	else {
		ret = 0x00;
	}

	_wt->chg_type = ret;
//	return data & 0xff;
	return ret;
}
EXPORT_SYMBOL_GPL(wt6670f_get_protocol);

int wt6670f_get_charger_type(void)
{
        return _wt->chg_type;
}
EXPORT_SYMBOL_GPL(wt6670f_get_charger_type);

bool wt6670f_is_charger_ready(void)
{
	return _wt->chg_ready;
}
EXPORT_SYMBOL_GPL(wt6670f_is_charger_ready);

int wt6670f_get_firmware_version(void)
{
	int ret;
	u16 data;

	ret = mmi_wt6670f_read_word(_wt, 0xBF, &data);
	if (ret < 0)
	{
		pr_err("wt6670f get firmware fail\n");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_get_firmware_version);

int z350_get_firmware_version(void)
{
	int ret;
	u16 data;

	ret = mmi_wt6670f_read_word(_wt, 0x14, &data);
	if (ret < 0)
	{
		pr_err("z350 get firmware fail\n");
		return ret;
	}
	return data;
}

int wt6670f_set_voltage(u16 voltage)
{
	int ret;
	u16 step;
	u16 voltage_now;

	voltage_now = wt6670f_get_vbus_voltage();
        pr_err("wt6670f current voltage = %d, set voltage = %d\n", voltage_now, voltage);

	if(voltage - voltage_now < 0)
	{
		step = (u16)((voltage_now - voltage) / 20);
    _wt->count -= step;
		step &= 0x7FFF;
		step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);

    if(_wt->count < 0)
        _wt->count = 0;
	}
	else
	{
		step = (u16)((voltage - voltage_now) / 20);
    _wt->count += step;
    step |= 0x8000;
    step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
	}
	pr_err("---->southchip count = %d   %04x,QC3P_WT6670F=%d,g_qc3p_id=%d,voltage=%d,voltage_now=%d\n", _wt->count, step,QC3P_WT6670F,g_qc3p_id,voltage,voltage_now);

	if(QC3P_WT6670F == g_qc3p_id)
		ret = mmi_wt6670f_write_word(_wt, 0xBB, step);
	else if(QC3P_Z350 == g_qc3p_id)
		ret = mmi_wt6670f_write_word(_wt, 0x83, step);
	else
		ret = -1;

	return ret;
}
EXPORT_SYMBOL_GPL(wt6670f_set_voltage);

int wt6670f_set_volt_count(int count)
{
        int ret=0;
        u16 step = abs(count);

        pr_err("Set vbus with %d pulse!\n!", count);

        if(count < 0)
        {
    _wt->count -= step;
                step &= 0x7FFF;
                step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);

    if(_wt->count < 0)
        _wt->count = 0;
        }
        else
        {
    _wt->count += step;
    step |= 0x8000;
    step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
        }
  pr_err("---->southchip count = %d   %04x\n", _wt->count, step);

	if(QC3P_WT6670F == g_qc3p_id)
		ret = mmi_wt6670f_write_word(_wt, 0xBB, step);
	else if(QC3P_Z350 == g_qc3p_id)
		ret = mmi_wt6670f_write_word(_wt, 0x83, step);

        return ret;
}
EXPORT_SYMBOL_GPL(wt6670f_set_volt_count);

static ssize_t wt6670f_show_test(struct device *dev,
						struct device_attribute *attr, char *buf)
{
//	struct wt6670f *wt = dev_get_drvdata(dev);

	int idx = 0;
	u8 data;

	data = wt6670f_get_protocol();
	idx = snprintf(buf, PAGE_SIZE, ">>> protocol = %02x\n", data);
	return idx;
}

static ssize_t wt6670f_store_test(struct device *dev,
								struct device_attribute *attr, const char *buf, size_t count)
{
//	struct wt6670f *wt = dev_get_drvdata(dev);

	u16 val;
	int ret;

	ret = sscanf(buf, "%d", &val);

	ret = wt6670f_set_voltage(val);

	return count;

}


static ssize_t wt6670f_show_registers(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct wt6670f *wt = dev_get_drvdata(dev);

	int idx = 0;
	int ret ;
	u16 data;
	if(QC3P_WT6670F == g_qc3p_id){
		ret = mmi_wt6670f_read_word(wt, 0xBD, &data);
		idx = snprintf(buf, PAGE_SIZE, ">>> reg[0xBD] = %04x\n", data);
		pr_err(">>>>>>>>>>WT6670F test southchip  0xBD = %04x\n", data);
	}

	if(QC3P_Z350 == g_qc3p_id){
		ret = mmi_wt6670f_read_word(wt, 0x11, &data);
		idx = snprintf(buf, PAGE_SIZE, ">>> reg[0x11] = %04x\n", data);
		pr_err(">>>>>>>>>>Z350 test southchip  0x11 = %04x\n", data);
	}

	return idx;
}

static ssize_t wt6670f_store_registers(struct device *dev,
								struct device_attribute *attr, const char *buf, size_t count)
{
	struct wt6670f *wt = dev_get_drvdata(dev);

	u16 val;
	int ret;

	ret = sscanf(buf, "%x", &val);

	if(QC3P_WT6670F == g_qc3p_id)
		ret = mmi_wt6670f_write_word(wt, 0xBB, val);

	if(QC3P_Z350 == g_qc3p_id)
		ret = mmi_wt6670f_write_word(wt, 0x83, val);

	return count;

}


static DEVICE_ATTR(test, 0660, wt6670f_show_test, wt6670f_store_test);
static DEVICE_ATTR(registers, 0660, wt6670f_show_registers, wt6670f_store_registers);

static void wt6670f_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_registers);
  device_create_file(dev, &dev_attr_test);
}

int wt6670f_do_reset(void)
{
	gpio_direction_output(_wt->reset_pin,1);
	msleep(10);
	gpio_direction_output(_wt->reset_pin,0);
	msleep(10);

	return 0;
}
EXPORT_SYMBOL_GPL(wt6670f_do_reset);

void wt6670f_reset_chg_type(void)
{
        _wt->chg_type = 0;
	_wt->chg_ready = false;
	m_chg_type = 0;
}
EXPORT_SYMBOL_GPL(wt6670f_reset_chg_type);

int moto_tcmd_wt6670f_get_firmware_version(void)
{
	int wt6670f_fm_ver = 0;
	int z350_fm_ver = 0;
	wt6670f_do_reset();
	wt6670f_fm_ver = wt6670f_get_firmware_version();
	z350_fm_ver = z350_get_firmware_version();
	pr_info("%s: get firmware wt6670f:%x,z350:%x!\n", __func__,wt6670f_fm_ver,z350_fm_ver);
	if((3 == wt6670f_fm_ver)||(0x080a == z350_fm_ver)||(0x0f0a == z350_fm_ver))
	return 1;
	else
	return 0;
}
EXPORT_SYMBOL_GPL(moto_tcmd_wt6670f_get_firmware_version);

static irqreturn_t wt6670f_intr_handler(int irq, void *data)
{
//	m_chg_type = 0xff;
	pr_info("%s,chg_type 0x:%x\n", __func__,m_chg_type);
	_wt->chg_ready = true;

	return IRQ_HANDLED;
}

static int wt6670f_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret = 0;

	pr_info("%s\n", __func__);
	if (!np) {
		pr_info("%s: no of node\n", __func__);
		return -ENODEV;
	}
#if 0
	wt6670f_reset_pin = of_get_named_gpio(np,"wt6670f-reset-gpio",0);
	if(wt6670f_reset_pin < 0){
	pr_info("%s: no reset\n", __func__);
	return -ENODATA;
	}
	else
	pr_info("%s: wt6670f-reset-gpio = %d\n", __func__,wt6670f_reset_pin);
	gpio_request(wt6670f_reset_pin,"wt6670f_reset");
	gpio_direction_output(wt6670f_reset_pin,0);
#endif

	wt6670f_int_pin = of_get_named_gpio(np,"wt6670f-int-gpio",0);
	if(wt6670f_int_pin < 0){
	pr_info("%s: no int\n", __func__);
	return -ENODATA;
	}
	else
	pr_info("%s: wt6670f-int-gpio = %d\n", __func__,wt6670f_int_pin);
	gpio_request(wt6670f_int_pin,"wt6670f_int");

	ret = request_irq(gpio_to_irq(wt6670f_int_pin), wt6670f_intr_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "wt6670f int", dev);
	enable_irq_wake(gpio_to_irq(wt6670f_int_pin));
	return 0;
}

//extern int wt6670f_isp_flow(struct wt6670f *chip);

static int wt6670f_suspend_notifier(struct notifier_block *nb,
				unsigned long event,void *dummy)
{
    struct wt6670f *wrt = container_of(nb, struct wt6670f, pm_nb);
	if (!wrt)
		return -ENOMEM;
    switch (event) {

    case PM_SUSPEND_PREPARE:
        pr_err("wt6670f PM_SUSPEND \n");

        wrt->wt6670f_suspend_flag = 1;

        return NOTIFY_OK;

    case PM_POST_SUSPEND:
        pr_err("wt6670f PM_RESUME \n");

        wrt->wt6670f_suspend_flag = 0;

        return NOTIFY_OK;

    default:
        return NOTIFY_DONE;
    }
}

static int wt6670f_i2c_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	int ret = 0;
	u16 firmware_version = 0;
	u16 wt6670f_id = 0;
	struct wt6670f *wt;

	pr_info("[%s]\n", __func__);
	if(is_already_probe_ok == 1){
		pr_info("[%s] is_already_probe_ok\n", __func__);
		return -1;
	}
	wt = devm_kzalloc(&client->dev, sizeof(struct wt6670f), GFP_KERNEL);
	if (!wt)
		return -ENOMEM;

	wt->dev = &client->dev;
	wt->client = client;
	i2c_set_clientdata(client, wt);
	mutex_init(&wt->i2c_rw_lock);

	wt->reset_pin = of_get_named_gpio(wt->dev->of_node, "wt6670f-reset-gpio", 0);
	if (wt->reset_pin < 0)
		pr_info("[%s] of get wt6670 gpio failed, reset_pin:%d\n",
			__func__, wt->reset_pin);

	if (gpio_request(wt->reset_pin, "wt6670_reset_pin"))
		pr_info("[%s] gpio_request reset failed", __func__);

	if (gpio_direction_output(wt->reset_pin, 0))
		pr_info("[%s] gpio_direction_output failed", __func__);

	i2c6_pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(i2c6_pinctrl)) {
		ret = PTR_ERR(i2c6_pinctrl);
		pr_info("fwq Cannot find i2c6_pinctrl!\n");
		return ret;
	}

	i2c6_scl_low = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_scl_low");
	if (IS_ERR(i2c6_scl_low)) {
		ret = PTR_ERR(i2c6_scl_low);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_scl_high = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_scl_high");
	if (IS_ERR(i2c6_scl_high)) {
		ret = PTR_ERR(i2c6_scl_high);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_sda_low = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_sda_low");
	if (IS_ERR(i2c6_sda_low)) {
		ret = PTR_ERR(i2c6_sda_low);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_sda_high = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_sda_high");
	if (IS_ERR(i2c6_sda_high)) {
		ret = PTR_ERR(i2c6_sda_high);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_i2c = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c");
	if (IS_ERR(i2c6_i2c)) {
		ret = PTR_ERR(i2c6_i2c);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	_wt = wt;

	ret = wt6670f_parse_dt(&client->dev);
	if (ret < 0)
		pr_info("%s: parse dt error\n", __func__);

	wt6670f_create_device_node(&(client->dev));

	wt6670f_do_reset();

	wt6670f_id = wt6670f_get_id(0x13);
	if(0x3349 == wt6670f_id){
		g_qc3p_id = QC3P_Z350;
		qc3p_z350_init_ok = true;
		gpio_direction_output(_wt->reset_pin, 1);
		pr_info("[%s] is z350\n", __func__);
		is_already_probe_ok = 1;
		goto probe_out;
	}

	firmware_version = wt6670f_get_firmware_version();
	wt6670f_reset_chg_type();
	pr_info("[%s] firmware_version = %d, chg_type = 0x%x\n", __func__,firmware_version, _wt->chg_type);
	if(firmware_version != WT6670_FIRMWARE_VERSION){
            pr_info("[%s]: firmware need upgrade, run wt6670_isp!", __func__);
            wt6670f_isp_flow(wt);
		wt6670f_do_reset();
		firmware_version = wt6670f_get_firmware_version();
		if(firmware_version != WT6670_FIRMWARE_VERSION){
            	pr_info("[%s]: firmware upgrade fail, run wt6670_isp again!", __func__);
            	wt6670f_isp_flow(wt);
        	}
        }
	wt6670f_id = wt6670f_get_id(0xBC);
	if(0x5457 == wt6670f_id){
		g_qc3p_id = QC3P_WT6670F;
		is_already_probe_ok = 1;
		pr_info("[%s] is wt6670f,firmware_version = %x\n", __func__,firmware_version);
	}else{
		devm_pinctrl_put(i2c6_pinctrl);
		pr_info("[%s] failed,release pinctrl\n", __func__);
	}
	wt->pm_nb.notifier_call = wt6670f_suspend_notifier;
	ret = register_pm_notifier(&wt->pm_nb);
	if (ret) {
		pr_err("register pm failed\n", __func__);
	}

probe_out:
	return 0;
}

static void wt6670f_shutdown(struct i2c_client *client)
{
	int ret = 0;
	u16 data = 0x01;
	u16 data_r = 0x00;

	if(QC3P_WT6670F == g_qc3p_id){
#ifdef CONFIG_MOTO_WT6670F_SHUTDOWN_ONLY_DO_SW_RESET
		ret = mmi_wt6670f_write_word(_wt, 0xB3, data); //software reset
		pr_info("[%s] wt6670f do sw reset\n", __func__);
#else
		ret = mmi_wt6670f_write_word(_wt, 0xB6, data);
		pr_info("%s set voltage ok wt6670\n", __func__);
#endif
	}else if(QC3P_Z350 == g_qc3p_id){
		ret = mmi_wt6670f_write_word(_wt, 0x02, data);
		pr_info("%s set voltage ok z350\n", __func__);
	}else{
		ret = mmi_wt6670f_write_word(_wt, 0xB6, data);
		pr_info("%s set voltage ok wt6670\n", __func__);
	}
#ifndef CONFIG_MOTO_WT6670F_SHUTDOWN_ONLY_DO_SW_RESET
	wt6670f_do_reset();
#endif
	if (ret < 0)
	{
#ifdef CONFIG_MOTO_WT6670F_SHUTDOWN_ONLY_DO_SW_RESET
		pr_err("%s wt6670f do sw reset fail, ret = %d\n", __func__, ret);
#else
		pr_info("%s set voltage fail\n",__func__);
#endif
	}
	if(QC3P_Z350 == g_qc3p_id) {
		ret = mmi_wt6670f_write_word(_wt, 0x06, data_r);
		pr_info("%s  wt6670 0x06 set 0 ok\n", __func__);
	}
}
//#define WT6670F_PM_OPS	(NULL)

static const struct i2c_device_id wt6670f_id_table[] = {
	{"wt6670f", 0},
	{},
};

static const struct of_device_id wt_match_table[] = {
	{.compatible = "mediatek,wt6670f_qc3p",},
	{},
};

static struct i2c_driver wt6670f_driver = {
	.driver = {
		.name = "wt6670f_qc3p",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = wt_match_table,
#endif
		//.pm = WT6670F_PM_OPS,
	},
	.probe = wt6670f_i2c_probe,
	//.remove = wt6670f_i2c_remove,
	.shutdown = wt6670f_shutdown,
	.id_table = wt6670f_id_table,
};

static int __init wt6670f_init(void)
{
	pr_info("[%s] init start with i2c DTS", __func__);
	if (i2c_add_driver(&wt6670f_driver) != 0) {
		pr_info(
			"[%s] failed to register wt6670f i2c driver.\n",__func__);
	} else {
		pr_info(
			"[%s] Success to register wt6670f i2c driver.\n",__func__);
	}
	
	return 0;
	//return i2c_add_driver(&wt6670f_driver);
}

static void __exit wt6670f_exit(void)
{
	i2c_del_driver(&wt6670f_driver);
}
module_init(wt6670f_init);
module_exit(wt6670f_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lai Du <dulai@longcheer.com>");
MODULE_DESCRIPTION("WT6670F QC3P Driver");


