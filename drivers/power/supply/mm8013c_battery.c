// SPDX-License-Identifier: GPL-2.0
/*
 * MM8xxx battery driver
 */
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <asm/unaligned.h>
#include <linux/iio/consumer.h>
#include <linux/firmware.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/types.h>

#define MM8XXX_MANUFACTURER	"MITSUMI ELECTRIC"
#define MM8XXX_BATT_PHY "bms"
#define mm_info(fmt, arg...)  \
	printk("FG_MM8xxx : %s-%d : " fmt, __FUNCTION__ ,__LINE__,##arg)

static DEFINE_IDR(battery_id);
static DEFINE_MUTEX(battery_mutex);

enum mm8xxx_chip {
	MM8118G01 = 1,
	MM8118W02,
	MM8013C10,
};

struct mm8xxx_device_info;

struct mm8xxx_access_methods {
	int (*read)(struct mm8xxx_device_info *di, u8 cmd);
	int (*write)(struct mm8xxx_device_info *di, u8 cmd, int value);
};

struct mm8xxx_state_cache {
	int temperature;
	int avg_time_to_empty;
	int full_charge_capacity;
	int cycle_count;
	int soc;
	int flags;
	int health;
	int elapsed_months;
	int elapsed_days;
	int elapsed_hours;
};

struct mm8xxx_device_info {
	struct device *dev;
	int id;
	enum mm8xxx_chip chip;
	const char *name;
	struct mm8xxx_access_methods bus;
	struct mm8xxx_state_cache cache;
	int charge_design_full;
	unsigned long last_update;
	struct delayed_work work;
	struct power_supply *psy;
	struct power_supply *batt_psy;
	struct power_supply *usb_psy;
	struct power_supply *dc_psy;
	struct list_head list;
	struct mutex lock;
	u8 *cmds;
	struct iio_channel *Batt_NTC_channel;
	bool fake_battery;
	u32 latest_fw_version;
	u32 first_battery_param_ver;
	u32 second_battery_param_ver;
	u32 first_battery_id;
	u32 second_battery_id;
	struct mutex i2c_rw_lock;
};

struct mm8xxx_device_info *di = NULL;

static int mm8xxx_battery_read_Nbyte(struct mm8xxx_device_info *di, u8 cmd, unsigned char *data, unsigned char length);
static int mm8xxx_battery_read(struct mm8xxx_device_info *di, u8 cmd);
static int mm8xxx_battery_write(struct mm8xxx_device_info *di, u8 cmd,int value);
static int mm8xxx_battery_write_Nbyte(struct mm8xxx_device_info *di, u8 cmd,unsigned int value, int byte_num);
static int mm8xxx_battery_write_4byteCmd(struct mm8xxx_device_info *di, unsigned int cmd,unsigned int value);
static u32 mmi_get_battery_info(struct mm8xxx_device_info *di, u32 cmd);
/****************************FW / Parameter update****************************************/
#define ENABLE_VERIFICATION

#define MM8013_HW_VERSION 0x0021
#define MM8013_LATEST_FW_VERSION 0x0812
#define MM8013_PARAMETER_VERSION 0x0201

#define MM8013_DEFAULT_BATTERY_ID 0x0103
#define MM8013_1TH_BATTERY_ID 0x0103
#define MM8013_2TH_BATTERY_ID 0x0102

#define ADDRESS_PROGRAM		(0x00008000)
#define ADDRESS_PARAMETER	(0x00014000)
#define SIZE_PROGRAM		(0x4000)
#define SIZE_PARAMETER		(0x3C0)
#ifdef ENABLE_VERIFICATION
#define SIZE_READBUFFER		(16)
#endif

#define COMMAND_CONTROL		(0x00)
#define COMMAND_FGCONDITION	(0x6E)
#define COMMAND_MODECONTROL	(0x88)
#ifdef ENABLE_VERIFICATION
#define COMMAND_READNVMDATA	(0x8B)
#endif
#define COMMAND_ERASENVM	(0x8C)
#define COMMAND_WRITENVMDATA	(0x8D)
#define REQCODE_CONTROL_STATUS	(0x0000)
#define FG_CMD_READ_FAILED	(0x10000)
#define BIT_SS			(0x2000)

#define FW_VER_CMD		(0x0002)
#define HW_VER_CMD		(0x0003)
#define PARAM_VER_CMD		(0x000C)
#define BATTERY_ID_CMD		(0x0008)

enum PARTITION_INDEX {
	PARTITION_PROGRAM = 0,
	PARTITION_PARAMETER,
	PARTITION_MAX
};

enum UPDATE_INDEX {
	UPDATE_PROGRAM = 0,
	UPDATE_PARAMETER,
	UPDATE_ALL,
	UPDATE_NONE
};

static unsigned char chartoBcd(char iChar)
{
	unsigned char mBCD = 0;

	if (iChar >= '0' && iChar <= '9')
		mBCD = iChar - '0';
	else if (iChar >= 'A' && iChar <= 'F')
		mBCD = iChar - 'A' + 0x0A;
	else if (iChar >= 'a' && iChar <= 'f')
		mBCD = iChar - 'a' + 0x0a;

	return mBCD;
}

static int tohex(char *pbuf, unsigned char length)
{
	int i = 0;
	int  value = 0;

	if  (length >4)
		return -EINVAL;

	for(i = 0; i < length; i++)
	{
		value += (chartoBcd(pbuf[i]) << (4*(length -1-i)));
	}

	return value;
}
/**
 * parse a FG firmware / parameter file
 *  arguments:
 *    hexfile_path - file path of FGFW/parameter
 *
 *  return value:
 *    pointer of jagged arrays
 *    or NULL if it fails
 *    jagged arrays details are below.
 *      [0] - pointer of PROGRAM data array
 *            or NULL if the specfied file does not contain any PROGRAM data.
 *      [1] - pointer of PARAMETER data array
 *            or NULL if the specfied file does not contain any PARAMETER data.
 */
static unsigned char *parse_hexfile(char *hexfile_name, struct mm8xxx_device_info *di)
{
	unsigned char *result = NULL;
	char *fbuf = NULL;
	char *pbuf = NULL;
	char *ebuf = NULL;
	int i,j;
	unsigned int bcnt;
	unsigned int addr;
	unsigned int rtype;
	unsigned int csum;
	unsigned int eladdr;
	int offset;
	int cont;
	int tmp;
	int ptsize;
	int size = 0;
	int ret;
	const struct firmware *fw;

	ret = request_firmware(&fw, hexfile_name, di->dev);
	if (ret || fw->size <=0 ) {
		mm_info("Couldn't get firmware  rc=%d\n", ret);
		goto FAILED;
	}
	size =  fw->size;
	fbuf = kzalloc(size+1, GFP_KERNEL);
	memset(fbuf, 0, size+1);
	memcpy(fbuf, fw->data, size);
	fbuf[size] = '\0';
	j = 0;
	for (i = 0; i < size; i++) {
		if (iscntrl(fbuf[i]) || (fbuf[i] == ' ')) {
			continue;
		}
		fbuf[j++] = fbuf[i];
	}

	ebuf = fbuf + j;
	*ebuf = '\0';
	cont = 1;
	pbuf = fbuf;
	eladdr = 0;
	do {
		if ((ebuf - pbuf) < 11)
			goto FAILED;
		if ((*pbuf) != ':')
			goto FAILED;
		pbuf++;
		/* Byte count */
		if ((tmp = tohex(pbuf, 2)) < 0)
			goto FAILED;
		bcnt = tmp;
		pbuf += 2;

		if ((ebuf - pbuf) < (8 + bcnt * 2))
			goto FAILED;
		csum = bcnt;
		/* Address */
		if ((tmp = tohex(pbuf, 4)) < 0)
			goto FAILED;
		addr = tmp;
		pbuf += 4;

		csum = (csum + ((addr & 0xFF00) >> 8) + (addr & 0x00FF)) & 0xFF;
		/* Record type */
		if ((tmp = tohex(pbuf, 2)) < 0)
			goto FAILED;
		rtype = tmp;
		pbuf += 2;

		csum = (csum + rtype) & 0xFF;
		/* Data */
		switch (rtype)
		{
		case 0:
			/* Data */
			offset = addr + eladdr;
			if ((offset >= ADDRESS_PROGRAM) &&
			    ((offset + bcnt) <= (ADDRESS_PROGRAM + SIZE_PROGRAM))) {
				offset = offset - ADDRESS_PROGRAM;
				ptsize = SIZE_PROGRAM;
			} else if ((offset >= ADDRESS_PARAMETER) &&
				   ((offset + bcnt) <= (ADDRESS_PARAMETER + SIZE_PARAMETER))) {
				offset = offset - ADDRESS_PARAMETER;
				ptsize = SIZE_PARAMETER;
			} else {
				offset = -1;
				ptsize = 0;
			}

			if (offset >= 0) {
				if (!result) {
					result = (unsigned char *)kzalloc(4 * ptsize, GFP_KERNEL);
					for (i = 0; i < ptsize; i++) {
						result[i] = (unsigned char)0xFF;
					}
				}
			}

			for (i = 0; i < bcnt; i++) {
				if ((tmp = tohex(pbuf, 2)) < 0)
					goto FAILED;
				j = tmp;
				pbuf += 2;
				csum = (csum + j) & 0xFF;

				if ((0 <= offset) && (offset < ptsize))
					result[offset++] = (unsigned char)j;
			}
			break;
		case 1:
			/* EOF */
			cont = 0;
			break;
		case 2:
			/* Extended Segment Address */
			if (bcnt != 2)
				goto FAILED;
			if ((tmp = tohex(pbuf, 4)) < 0)
				goto FAILED;
			eladdr = tmp;
			pbuf += 4;
			csum = (csum + ((eladdr & 0xFF00) >> 8) + (eladdr & 0x00FF)) & 0xFF;
			eladdr <<= 4;
			break;
		case 4:
			/* Extended Linear Address */
			if (bcnt != 2)
				goto FAILED;
			if ((tmp = tohex(pbuf, 4)) < 0)
				goto FAILED;
			eladdr = tmp;
			pbuf += 4;
			csum = (csum + ((eladdr & 0xFF00) >> 8) + (eladdr & 0x00FF)) & 0xFF;
			eladdr <<= 16;
			break;
		case 3:
		case 5:
			/* skip: Start Segment Address */
			/* skip: Start Linear Address */
			for (i = 0; i < bcnt; i++) {
				if ((tmp = tohex(pbuf, 2)) < 0)
					goto FAILED;
				j = tmp;
				pbuf += 2;
				csum = (csum + j) & 0xFF;
			}
			break;
		default:
			goto FAILED;
		}

		/* Checksum */
		if ((tmp = tohex(pbuf, 2)) < 0)
			goto FAILED;
		j = tmp;
		pbuf += 2;
		if ((csum + j) & 0xFF)
			goto FAILED;
	} while (cont);

	kfree(fbuf);

	return result;
FAILED:
	if (fbuf) {
		kfree(fbuf);
		fbuf = NULL;
	}
	if (result) {
		kfree(result);
		result = NULL;
	}
	return NULL;
}

static int fg_read_control_status(struct mm8xxx_device_info *di)
{
	if (mm8xxx_battery_write(di, COMMAND_CONTROL,REQCODE_CONTROL_STATUS) < 0)
		return -EINVAL;

	return mm8xxx_battery_read(di, COMMAND_CONTROL);
}

/* Additional library functions */
/**
 * set FG to UNSEAL mode
 *
 *  arguments:
 *    fd         - file descriptor of an i2c device
 *    s2us_code  - Seal to Unseal code
 *
 *  return value:
 *    0 if it successes
 *    or 1 if it fails.
 *
 */
static int unseal_request(struct mm8xxx_device_info *di, unsigned int s2us_code)
{
	int ret;
	/* Check CONTROL_STATUS */
	ret=fg_read_control_status(di);
	if (  ret < 0 ) {
		return -EINVAL;
	}

	if (ret & BIT_SS) {
		if ((mm8xxx_battery_write(di, COMMAND_CONTROL,  s2us_code & 0xFFFF) < 0) ||
		     (mm8xxx_battery_write(di, COMMAND_CONTROL,  (s2us_code >> 16) & 0xFFFF) <0)) {
			return -EINVAL;
		}

		if ((ret = fg_read_control_status(di)) < 0) {
			return -EINVAL;
		}
		if (ret & BIT_SS) {
			/* failed to set to UNSEALED mode. */
			return -EINVAL;
		}
	}

	return 0;
}

static int Lock_release_request(struct mm8xxx_device_info *di)
{
	return mm8xxx_battery_write(di, COMMAND_FGCONDITION, 0x0340);
}

static int NVM_write_mode(struct mm8xxx_device_info *di)
{
	if (mm8xxx_battery_write(di, COMMAND_FGCONDITION, 0x00A0) < 0)
		return -EINVAL;

	mdelay(100);

	if (mm8xxx_battery_write_Nbyte(di, COMMAND_MODECONTROL, 0x88, 1) < 0)
		return -EINVAL;

	return 0;
}

static int erase_param_nvm(struct mm8xxx_device_info *di)
{

	if ( (mm8xxx_battery_write(di, 0x00, 0x2E03) < 0) ||
		(mm8xxx_battery_write(di, 0x02, 0x0000) < 0) ||
		(mm8xxx_battery_write(di, 0x04, 0xDE83) < 0) ||
		(mm8xxx_battery_write(di, 0x64, 0x0192) < 0)) {
		return -EINVAL;
	}
	mdelay(100);

	if ( (mm8xxx_battery_write(di, 0x00, 0x2F03) < 0) ||
		(mm8xxx_battery_write(di, 0x02, 0x0000) < 0) ||
		(mm8xxx_battery_write(di, 0x04, 0xDE83) < 0) ||
		(mm8xxx_battery_write(di, 0x64, 0x0193) < 0)) {
		return -EINVAL;
	}
	mdelay(100);

	return 0;
}

static int erase_program_nvm(struct mm8xxx_device_info *di)
{
	if (mm8xxx_battery_write_Nbyte(di, COMMAND_ERASENVM, 0x80, 1) < 0)
		return -EINVAL;
	mdelay(2000);

	return 0;
}

static int write_nvm(struct mm8xxx_device_info *di, enum PARTITION_INDEX ptindex, unsigned char *data)
{
	unsigned int offset;
	unsigned int size;
	int i;
#ifdef ENABLE_VERIFICATION
	int j;
	int addr;
	unsigned char rbuf[SIZE_READBUFFER];
#endif

	if (!data) {
		/* specified pointer is NULL */
		return  -EINVAL;
	}

	switch (ptindex) {
		case PARTITION_PROGRAM:
			offset = ADDRESS_PROGRAM;
			size = SIZE_PROGRAM;
			break;

		case PARTITION_PARAMETER:
			offset = ADDRESS_PARAMETER;
			size = SIZE_PARAMETER;
			break;

		default:
			/* unexpected partition index */
			return  -EINVAL;
	}

	for (i = 0; i < size; i += 4) {
		if (mm8xxx_battery_write_4byteCmd(di, offset + i,
			data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) | (data[i + 3] << 24)) < 0) {
			mm_info("%s, mm8xxx_battery_write_4byteCmd fail\n", __func__);
			return  -EINVAL;
		}
	}

#ifdef ENABLE_VERIFICATION
	for (i = 0; i < size; i += SIZE_READBUFFER) {
		addr = offset + i;

		if ((mm8xxx_battery_write_Nbyte(di, COMMAND_READNVMDATA, addr, 4) < 0) ||
		    (mm8xxx_battery_read_Nbyte(di, COMMAND_READNVMDATA, rbuf, SIZE_READBUFFER) < 0)) {
			mm_info("%s, verification fail\n", __func__);
			return  -EINVAL;
		}

		for (j = 0; j < SIZE_READBUFFER; j++) {
			if (data[i + j] != rbuf[j]) {
				mm_info("%s, verification fail\n", __func__);
				return  -EINVAL;
			}
		}
	}
#endif

	return 0;
}

static int Write_program_data(struct mm8xxx_device_info *di, unsigned char *array)
{
	return write_nvm(di, PARTITION_PROGRAM, array);
}
static int Write_parameter_data(struct mm8xxx_device_info *di, unsigned char *array)
{
	return write_nvm(di, PARTITION_PARAMETER, array);
}

static int write_program_and_parameter(struct mm8xxx_device_info *di,
					 unsigned char *program_array,
					 unsigned char *parameter_array)
{
	int ret = -EINVAL;

	/* Unseal Request */
	mm_info("Requesting set to Unseal mode ... ");
	if (unseal_request(di, 0x56781234) < 0) {
		mm_info("unseal_request failed\n");
		goto EXIT;
	}

	/* Lock Release Request */
	mm_info("Requesting lock releasing ... ");
	if (Lock_release_request(di) < 0) {
		mm_info("Lock_release_request failed\n");
		goto EXIT;
	}

	/* NVM Write mode Request */
	mm_info("Requesting set to NVM Write mode ... ");
	if (NVM_write_mode(di) < 0) {
		mm_info("Requesting set to NVM Write mode failed\n");
		goto EXIT;
	}

	/* Program Erase And wait 2 second */
	if (program_array) {
		mm_info("Erasing 'PROGRAM' partition ... ");
		if(erase_program_nvm(di) < 0) {
			mm_info("Erasing 'PROGRAM' failed\n");
			goto EXIT;
		}
	}

	/* Parameter Erase And wait 2 second */
	if (parameter_array) {
		mm_info("Erasing 'PARAMETER' partition ... ");
		if(erase_param_nvm(di) < 0) {
			mm_info("Erasing 'PARAMETER' failed\n");
			goto EXIT;
		}
	}

	/*
	 * Initialization of variables
	 * Copy the data of the target from the hex file
	 * Write Command 0x8D and Data
	 * Increment the write size
	 * 16384byte / 4 = 4096Loop
	 */
	if (program_array) {
		mm_info("Writing 'PROGRAM' partition ... ");
		if(Write_program_data(di, program_array) < 0) {
			mm_info("Writing 'PROGRAM' failed\n");
			goto EXIT;
		}
	}

	/*
	 * Initialization of variables
	 * Copy the data of the target from the hex file
	 * Write Command 0x8D and Data
	 * Increment the write size
	 * 960byte / 4 = 250Loop
	 */
	if (parameter_array) {
		mm_info("Writing 'PARAMETER' partition ... ");
		if(Write_parameter_data(di, parameter_array) < 0) {
			mm_info("Writing 'PARAMETER' failed\n");
			goto EXIT;
		}
	}

	/* System Reset Request */
	mm_info("Requesting system resetting ... ");
	if (mm8xxx_battery_write_Nbyte(di, COMMAND_MODECONTROL, 0x80, 1) < 0) {
		mm_info("Reset fg system failed\n");
		goto EXIT;
	}
	mdelay(100);
	ret = 0;

EXIT:
	return ret;
}

static const char *get_battery_serialnumber(void)
{
	struct device_node *np = of_find_node_by_path("/chosen");
	const char *battsn_buf;
	int retval;

	battsn_buf = NULL;

	if (np)
		retval = of_property_read_string(np, "mmi,battid",
						 &battsn_buf);
	else
		return NULL;

	if ((retval == -EINVAL) || !battsn_buf) {
		mm_info(" Battsn unused\n");
		of_node_put(np);
		return NULL;

	} else
		mm_info("Battsn = %s\n", battsn_buf);

	of_node_put(np);

	return battsn_buf;
}

static u32 seach_batt_id(void)
{
	struct device_node *np = di->dev->of_node;
	const char *dev_sn = NULL;
	const char *first_batt_sn = NULL;
	const char *second_batt_sn = NULL;
	u32 battery_id = di->first_battery_id;;

	dev_sn = get_battery_serialnumber();
	if (dev_sn != NULL) {
		of_property_read_string(np, "first_batt_sn",
					     &first_batt_sn);
		of_property_read_string(np, "second_batt_sn",
					     &second_batt_sn);
		if (first_batt_sn != NULL && second_batt_sn != NULL)
		{
			if (strnstr(dev_sn, first_batt_sn, 10))
				battery_id = di->first_battery_id;
			else if (strnstr(dev_sn, second_batt_sn, 10))
				battery_id = di->second_battery_id;
		}
	}

	return battery_id;
}

static int mm8xxx_battery_update_program_and_parameter(struct mm8xxx_device_info *di, enum UPDATE_INDEX update_index)
{
	unsigned char *fw_hexfile_data = NULL;
	unsigned char *param_hexfile_data = NULL;
	u32 battery_id;
	int i;
	char param_name[30] = {0};

	if (update_index != UPDATE_PROGRAM){
		battery_id = mmi_get_battery_info(di, BATTERY_ID_CMD);
		if (battery_id !=0)
		{
			if (battery_id != di->first_battery_id && battery_id != di->second_battery_id){
				 mm_info("Error: the battery is not suitable for this poject.\n");
				goto EXIT;
			}
		}
		else {
			battery_id = seach_batt_id();
                      mm_info("Error getting battery_id , try to get batt sn via utag info and get the correct batt_id=%04x\n", battery_id);
		}

		sprintf(param_name,"%s%04x%s", "mm8013c_parameter_", battery_id, ".hex");
		mm_info("battery parameter name=%s\n", param_name);
	}

	switch (update_index )
	{
		case UPDATE_PROGRAM:
			mm_info("Parsing the FW_HEX file ... ");
			if (!(fw_hexfile_data = parse_hexfile("mm8013c_fw.hex", di))) {
				mm_info("Parse fw hex data failed\n");
				goto EXIT;
			}
			break;
		case UPDATE_PARAMETER:
			mm_info("Parsing the parameter_HEX file ... ");
			if (!(param_hexfile_data = parse_hexfile(param_name, di))) {
				mm_info("Parse parameter hex date failed\n");
				goto EXIT;
			}
			break;
		case UPDATE_ALL:
			mm_info("Parsing the FW_HEX file ... ");
			if (!(fw_hexfile_data = parse_hexfile("mm8013c_fw.hex", di))) {
				mm_info("failed\n");
				goto EXIT;
			}
			mm_info("Parsing the parameter_HEX file ... ");
			if (!(param_hexfile_data = parse_hexfile(param_name, di))) {
				mm_info("failed\n");
				goto EXIT;
			}
			break;
		default:
		return -EINVAL;
	}

	for (i=1; i<= 3; i++)
	{
		/* start TRM sequence */
		if (write_program_and_parameter(di,
						fw_hexfile_data, param_hexfile_data) < 0) {
			mm_info("FAILED TO UPDATE program_and_parameter,fail count=%d\n", i);
		}
		else {
			mm_info("successfully update program_and_parameter !!\n");
			break;
		}
	}

EXIT:
	if (fw_hexfile_data) {
		kfree(fw_hexfile_data);
		fw_hexfile_data = NULL;
	}
	if (param_hexfile_data) {
		kfree(param_hexfile_data);
		param_hexfile_data = NULL;
	}

	return 0;
}

/********************************************************************/

static ssize_t  mm8xxx_battery_fw_ver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%04x\n", mmi_get_battery_info(di, FW_VER_CMD));
}
static DEVICE_ATTR(fw_ver, S_IRUGO| (S_IWUSR|S_IWGRP), mm8xxx_battery_fw_ver_show, NULL);

static ssize_t  mm8xxx_battery_param_ver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%04x\n", mmi_get_battery_info(di, PARAM_VER_CMD));
}
static DEVICE_ATTR(param_ver, S_IRUGO| (S_IWUSR|S_IWGRP), mm8xxx_battery_param_ver_show, NULL);

static ssize_t  mm8xxx_battery_batt_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%04x\n", mmi_get_battery_info(di, BATTERY_ID_CMD));
}
static DEVICE_ATTR(batt_id, S_IRUGO| (S_IWUSR|S_IWGRP), mm8xxx_battery_batt_id_show, NULL);

static struct attribute *mm8xxx_attributes[] = {
	&dev_attr_fw_ver.attr,
	&dev_attr_param_ver.attr,
	&dev_attr_batt_id.attr,
	NULL,
};

static struct attribute_group mm8xxx_attribute_group = {
	.attrs = mm8xxx_attributes,
};

static void mm8xxx_battery_update(struct mm8xxx_device_info *di);
static irqreturn_t mm8xxx_battery_irq_handler_thread(int irq, void *data)
{
	struct mm8xxx_device_info *di = data;
	mm8xxx_battery_update(di);
	return IRQ_HANDLED;
}

static int mm8xxx_battery_read(struct mm8xxx_device_info *di, u8 cmd)
{
#if 1
	int ret = 0;
	struct i2c_client *client = to_i2c_client(di->dev);

	mutex_lock(&di->i2c_rw_lock);
	ret = i2c_smbus_read_word_data(client, cmd);
	if (ret < 0)
		dev_info(&client->dev, "%s: err %d\n", __func__, ret);
	mutex_unlock(&di->i2c_rw_lock);

	return ret;
#else
	struct i2c_client *client = to_i2c_client(di->dev);
	struct i2c_msg msg[2];
	u8 data[2];
	int ret;

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = &cmd;
	msg[0].len = sizeof(cmd);
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = 2;

	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0)
		return ret;

	ret = get_unaligned_le16(data);

	return ret;
#endif
}
#ifdef ENABLE_VERIFICATION
static int mm8xxx_battery_read_Nbyte(struct mm8xxx_device_info *di, u8 cmd, unsigned char *data, unsigned char length)
{
	struct i2c_client *client = to_i2c_client(di->dev);
	struct i2c_msg msg[2];
	int ret;
//mm_info("%s, begin\n", __func__);
	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = &cmd;
	msg[0].len = sizeof(cmd);
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = length;

	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0)
		return ret;
//mm_info("%s, end\n", __func__);
	return ret;
}
#endif

static int mm8xxx_battery_write(struct mm8xxx_device_info *di, u8 cmd,
				int value)
{
#if 1
	int ret;
	struct i2c_client *client = to_i2c_client(di->dev);
	mutex_lock(&di->i2c_rw_lock);
	ret = i2c_smbus_write_word_data(client, cmd, value);
	if (ret < 0)
		dev_info(&client->dev, "%s: err %d\n", __func__, ret);
	mutex_unlock(&di->i2c_rw_lock);

	return ret;
#else	
	struct i2c_client *client = to_i2c_client(di->dev);
	struct i2c_msg msg;
	u8 data[4];
	int ret;

	if (!client->adapter)
		return -ENODEV;

	data[0] = cmd;
	put_unaligned_le16(value, &data[1]);
	msg.len = 3;

	msg.buf = data;
	msg.addr = client->addr;
	msg.flags = 0;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		return ret;
	else if (ret != 1)
		return -EINVAL;

	return 0;
#endif
}

static int mm8xxx_battery_write_Nbyte(struct mm8xxx_device_info *di, u8 cmd,
				unsigned int value, int byte_num)
{
	struct i2c_client *client = to_i2c_client(di->dev);
	struct i2c_msg msg;
	u8 data[16];
	int ret,i=0;

	if (byte_num > 15)
		return -ENODEV;
	if (!client->adapter)
		return -ENODEV;

	data[0] = cmd;
	for (i=0; i < byte_num; i++)
	{
		data[i+1] = (value >> (8 * i)) & 0xFF;
	}

	msg.len = byte_num+1;

	msg.buf = data;
	msg.addr = client->addr;
	msg.flags = 0;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		return ret;
	else if (ret != 1)
		return -EINVAL;

	return 0;
}

static int mm8xxx_battery_write_4byteCmd(struct mm8xxx_device_info *di, unsigned int cmd, unsigned int value)
{
	struct i2c_client *client = to_i2c_client(di->dev);
	struct i2c_msg msg;
	u8 data[9];
	int ret;

	if (!client->adapter)
		return -ENODEV;

	data[0] = COMMAND_WRITENVMDATA;
	data[1] = cmd & 0xFF;
	data[2] = (cmd >> 8) & 0xFF;
	data[3] = (cmd >> 16) & 0xFF;
	data[4] = (cmd >> 24) & 0xFF;

	data[5] = value & 0xFF;
	data[6] = (value >> 8) & 0xFF;
	data[7] = (value >> 16) & 0xFF;
	data[8] = (value >> 24) & 0xFF;

	msg.len = 9;
	msg.buf = data;
	msg.addr = client->addr;
	msg.flags = 0;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		return ret;
	else if (ret != 1)
		return -EINVAL;

	return 0;
}

/* MM8XXX Flags */
#define MM8XXX_FLAG_DSG		BIT(0)
#define MM8XXX_FLAG_SOCF	BIT(1)
#define MM8XXX_FLAG_SOC1	BIT(2)
#define MM8XXX_FLAG_UT		BIT(3)
#define MM8XXX_FLAG_OT		BIT(4)
#define MM8XXX_FLAG_ODC		BIT(5)
#define MM8XXX_FLAG_OCC		BIT(6)
#define MM8XXX_FLAG_OCVTAKEN	BIT(7)
#define MM8XXX_FLAG_CHG		BIT(8)
#define MM8XXX_FLAG_FC		BIT(9)
#define MM8XXX_FLAG_CHGINH	BIT(11)
#define MM8XXX_FLAG_BATLOW	BIT(12)
#define MM8XXX_FLAG_BATHI	BIT(13)
#define MM8XXX_FLAG_OTD		BIT(14)
#define MM8XXX_FLAG_OTC		BIT(15)

#define INVALID_COMMAND		0xff

enum mm8xxx_cmd_index {
	MM8XXX_CMD_CONTROL = 0,
	MM8XXX_CMD_TEMPERATURE,
	MM8XXX_CMD_VOLTAGE,
	MM8XXX_CMD_FLAGS,
	MM8XXX_CMD_REMAININGCAPACITY,
	MM8XXX_CMD_FULLCHARGECAPACITY,
	MM8XXX_CMD_AVERAGECURRENT,
	MM8XXX_CMD_AVERAGETIMETOEMPTY,
	MM8XXX_CMD_CHARGETYPE,
	MM8XXX_CMD_CYCLECOUNT,
	MM8XXX_CMD_STATEOFCHARGE,
	MM8XXX_CMD_CHARGEVOLTAGE,
	MM8XXX_CMD_DESIGNCAPACITY,
	MM8XXX_CMD_ELAPSEDTIMEM,
	MM8XXX_CMD_ELAPSEDTIMED,
	MM8XXX_CMD_ELAPSEDTIMEH,

	MM8XXX_CMD_EOI,
};

static u8
	mm8118g01_cmds[MM8XXX_CMD_EOI] = {
		[MM8XXX_CMD_CONTROL] 		= 0x00,
		[MM8XXX_CMD_TEMPERATURE] 	= 0x06,
		[MM8XXX_CMD_VOLTAGE] 		= 0x08,
		[MM8XXX_CMD_FLAGS] 		= 0x0A,
		[MM8XXX_CMD_REMAININGCAPACITY]	= 0x10,
		[MM8XXX_CMD_FULLCHARGECAPACITY]	= 0x12,
		[MM8XXX_CMD_AVERAGECURRENT]	= 0x14,
		[MM8XXX_CMD_AVERAGETIMETOEMPTY]	= 0x16,
		[MM8XXX_CMD_CYCLECOUNT]		= 0x2A,
		[MM8XXX_CMD_STATEOFCHARGE]	= 0x2C,
		[MM8XXX_CMD_CHARGEVOLTAGE]	= INVALID_COMMAND,
		[MM8XXX_CMD_DESIGNCAPACITY]	= 0x3C,
		[MM8XXX_CMD_ELAPSEDTIMEM]	= INVALID_COMMAND,
		[MM8XXX_CMD_ELAPSEDTIMED]	= INVALID_COMMAND,
		[MM8XXX_CMD_ELAPSEDTIMEH]	= INVALID_COMMAND,
	},
	mm8118w02_cmds[MM8XXX_CMD_EOI] = {
		[MM8XXX_CMD_CONTROL] 		= 0x00,
		[MM8XXX_CMD_TEMPERATURE] 	= 0x06,
		[MM8XXX_CMD_VOLTAGE] 		= 0x08,
		[MM8XXX_CMD_FLAGS] 		= 0x0A,
		[MM8XXX_CMD_REMAININGCAPACITY]	= 0x10,
		[MM8XXX_CMD_FULLCHARGECAPACITY]	= 0x12,
		[MM8XXX_CMD_AVERAGECURRENT]	= 0x14,
		[MM8XXX_CMD_AVERAGETIMETOEMPTY]	= 0x16,
		[MM8XXX_CMD_CYCLECOUNT]		= 0x2A,
		[MM8XXX_CMD_STATEOFCHARGE]	= 0x2C,
		[MM8XXX_CMD_CHARGEVOLTAGE]	= INVALID_COMMAND,
		[MM8XXX_CMD_DESIGNCAPACITY]	= 0x3C,
		[MM8XXX_CMD_ELAPSEDTIMEM]	= INVALID_COMMAND,
		[MM8XXX_CMD_ELAPSEDTIMED]	= INVALID_COMMAND,
		[MM8XXX_CMD_ELAPSEDTIMEH]	= INVALID_COMMAND,
	},
	mm8013c10_cmds[MM8XXX_CMD_EOI] = {
		[MM8XXX_CMD_CONTROL] 		= 0x00,
		[MM8XXX_CMD_TEMPERATURE] 	= 0x06,
		[MM8XXX_CMD_VOLTAGE] 		= 0x08,
		[MM8XXX_CMD_FLAGS] 		= 0x0A,
		[MM8XXX_CMD_REMAININGCAPACITY]	= 0x10,
		[MM8XXX_CMD_FULLCHARGECAPACITY]	= 0x12,
		[MM8XXX_CMD_AVERAGECURRENT]	= 0x14,
		[MM8XXX_CMD_AVERAGETIMETOEMPTY]	= 0x16,
		[MM8XXX_CMD_CHARGETYPE]		= 0x20,
		[MM8XXX_CMD_CYCLECOUNT]		= 0x2A,
		[MM8XXX_CMD_STATEOFCHARGE]	= 0x2C,
		[MM8XXX_CMD_CHARGEVOLTAGE]	= 0x30,
		[MM8XXX_CMD_DESIGNCAPACITY]	= 0x3C,
		[MM8XXX_CMD_ELAPSEDTIMEM]	= 0x74,
		[MM8XXX_CMD_ELAPSEDTIMED]	= 0x76,
		[MM8XXX_CMD_ELAPSEDTIMEH]	= 0x78,
	};

static enum power_supply_property mm8118g01_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_POWER_AVG,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_MANUFACTURER,
};
#define mm8118w02_props mm8118g01_props

static enum power_supply_property mm8013c10_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_MANUFACTURER,
};

#define MM8XXX_DATA(ref) { 			\
	.cmds = ref##_cmds,			\
	.props = ref##_props,			\
	.props_size = ARRAY_SIZE(ref##_props) }

static struct {
	u8 *cmds;
	enum power_supply_property *props;
	size_t props_size;
} mm8xxx_chip_data[] = {
	[MM8118G01]	= MM8XXX_DATA(mm8118g01),
	[MM8118W02]	= MM8XXX_DATA(mm8118w02),
	[MM8013C10]	= MM8XXX_DATA(mm8013c10),
};

static DEFINE_MUTEX(mm8xxx_list_lock);
static LIST_HEAD(mm8xxx_battery_devices);

#define MM8XXX_MSLEEP(i) usleep_range((i)*1000, (i)*1000+500)

static int poll_interval_param_set(const char *val,
				   const struct kernel_param *kp)
{
	struct mm8xxx_device_info *di;
	unsigned int prev_val = *(unsigned int *) kp->arg;
	int ret;

	ret = param_set_uint(val, kp);
	if ((ret < 0) || (prev_val == *(unsigned int *) kp->arg))
		return ret;

	mutex_lock(&mm8xxx_list_lock);
	list_for_each_entry(di, &mm8xxx_battery_devices, list) {
		cancel_delayed_work_sync(&di->work);
		schedule_delayed_work(&di->work, 0);
	}
	mutex_unlock(&mm8xxx_list_lock);

	return ret;
}

static const struct kernel_param_ops param_ops_poll_interval = {
	.get = param_get_uint,
	.set = poll_interval_param_set,
};

static unsigned int poll_interval = 360;
module_param_cb(poll_interval, &param_ops_poll_interval, &poll_interval, 0644);
MODULE_PARM_DESC(poll_interval,
		"battery poll interval in seconds - 0 disables polling");

static inline int mm8xxx_read(struct mm8xxx_device_info *di, int cmd_index)
{
	int ret;

	if ((!di) || (di->cmds[cmd_index] == INVALID_COMMAND))
		return -EINVAL;

	if (!di->bus.read)
		return -EPERM;

	ret = di->bus.read(di, di->cmds[cmd_index]);
	if (ret < 0)
		dev_dbg(di->dev, "failed to read command 0x%02x (index %d)\n",
			di->cmds[cmd_index], cmd_index);

	return ret;
}

static inline int mm8xxx_write(struct mm8xxx_device_info *di, int cmd_index,
			       u16 value)
{
	int ret;

	if ((!di) || (di->cmds[cmd_index] == INVALID_COMMAND))
		return -EINVAL;

	if (!di->bus.write)
		return -EPERM;

	ret = di->bus.write(di, di->cmds[cmd_index], value);
	if (ret < 0)
		dev_dbg(di->dev, "failed to write command 0x%02x (index %d)\n",
			di->cmds[cmd_index], cmd_index);

	return ret;
}

 static int mm8xxx_battery_temp(struct mm8xxx_device_info *di, int *temp)
{
#if 1
	int rc;
	union power_supply_propval prop = {0};

	if (di->batt_psy == NULL || IS_ERR(di->batt_psy)) {
		dev_err(di->dev,"%s Couldn't get bms\n", __func__);
		return -EINVAL;
	}

	rc = power_supply_get_property(di->batt_psy, POWER_SUPPLY_PROP_TEMP, &prop);
	*temp = prop.intval*100;

	return rc;
#else
	return iio_read_channel_processed(di->Batt_NTC_channel, temp);
#endif
}

static int mm8xxx_battery_temp_to_FG(struct mm8xxx_device_info *di)
{
	static int pre_temp=250;
	int temp = 0;
	int ret = 0;

	ret = mm8xxx_battery_temp(di, &temp);
	if (ret < 0) {
		dev_err(di->dev, "error mm8xxx_battery_temp \n");
		return ret;
	}
	temp = temp / 100;

	if(temp < 0) {
		temp = 65536 - temp;
	}

	if ( pre_temp != temp)
	{
		pre_temp = temp;
		ret =mm8xxx_write(di, MM8XXX_CMD_TEMPERATURE, (u16)temp);
		if (ret < 0) {
			dev_err(di->dev, "error writing temperature to fg \n");
		}
	}

	return ret;
}

static int mm8xxx_battery_chargeType_to_FG(struct mm8xxx_device_info *di, int type)
{
	int ret = 0;

	ret =mm8xxx_write(di, MM8XXX_CMD_CHARGETYPE, (u16)type);
	if (ret < 0) {
		dev_err(di->dev, "error writing chargeType to fg\n");
	}

	return ret;
}

/*
 * Return the battery State-of-Charge
 * Or < 0 if somthing fails.
 */
static int mm8xxx_battery_read_stateofcharge(struct mm8xxx_device_info *di)
{
	int soc;

	soc = mm8xxx_read(di, MM8XXX_CMD_STATEOFCHARGE);

	if (soc < 0)
		dev_dbg(di->dev, "error reading State-of-Charge\n");

	return soc;
}

/*
 * Return the battery Remaining Capacity in μAh
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_remainingcapacity(struct mm8xxx_device_info *di)
{
	int rc;

	rc = mm8xxx_read(di, MM8XXX_CMD_REMAININGCAPACITY);
	if (rc < 0)
		dev_dbg(di->dev, "error reading Remaining Capacity\n");

	rc *= 1000;

	return rc;
}

/*
 * Return the battery Full Charge Capacity in μAh
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_fullchargecapacity(struct mm8xxx_device_info *di)
{
	int fcc;

	fcc = mm8xxx_read(di, MM8XXX_CMD_FULLCHARGECAPACITY);
	if (fcc < 0)
		dev_dbg(di->dev, "error reading Full Charge Capacity\n");

	fcc *= 1000;

	return fcc;
}

/*
 * Return the battery Design Capacity in μAh
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_designcapacity(struct mm8xxx_device_info *di)
{
	int dc;

	dc = mm8xxx_read(di, MM8XXX_CMD_DESIGNCAPACITY);
	if (dc < 0)
		dev_dbg(di->dev, "error reading Design Capacity\n");

	dc *= 1000;

	return dc;
}

/*
 * Return the battery temperature in tenths of degree Kelvin
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_temperature(struct mm8xxx_device_info *di)
{
	int temp;

	temp = mm8xxx_read(di, MM8XXX_CMD_TEMPERATURE);
	if (temp < 0)
		dev_err(di->dev, "error reading temperature\n");

	return temp;
}

/*
 * Return the battery Cycle Count
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_cyclecount(struct mm8xxx_device_info *di)
{
	int cc;

	cc = mm8xxx_read(di, MM8XXX_CMD_CYCLECOUNT);
	if (cc < 0)
		dev_err(di->dev, "error reading cycle count\n");

	return cc;
}

/*
 * Return the battery usable time
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_averagetimetoempty(struct mm8xxx_device_info *di)
{
	int atte;

	atte = mm8xxx_read(di, MM8XXX_CMD_AVERAGETIMETOEMPTY);
	if (atte < 0) {
		dev_dbg(di->dev, "error reading average time to empty\n");
		return atte;
	}

	if (atte == 65535)
		return -ENODATA;

	return atte * 60;
}

/*
 * Returns true if a battery over temperature condition is detected.
 */
static inline bool mm8xxx_battery_overtemperature(u16 flags)
{
	return flags & MM8XXX_FLAG_OT;
}

/*
 * Returns true if a battery under temperature condition is detected.
 */
static inline bool mm8xxx_battery_undertemperature(u16 flags)
{
	return flags & MM8XXX_FLAG_UT;
}

/*
 * Returns true if a low state of charge condition is detected.
 */
static inline bool mm8xxx_battery_dead(u16 flags)
{
	return flags & MM8XXX_FLAG_SOCF;
}

/*
 * Returns POWER_SUPPLY_HEALTH_(OVERHEAT/COLD/DEAD) if a battery is in abnormal
 * condition, POWER_SUPPLY_HEALTH_GOOD otherwise.
 */
static int mm8xxx_battery_read_health(struct mm8xxx_device_info *di)
{
	if (unlikely(mm8xxx_battery_overtemperature(di->cache.flags)))
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	if (unlikely(mm8xxx_battery_undertemperature(di->cache.flags)))
		return POWER_SUPPLY_HEALTH_COLD;
	if (unlikely(mm8xxx_battery_dead(di->cache.flags)))
		return POWER_SUPPLY_HEALTH_DEAD;

	return POWER_SUPPLY_HEALTH_GOOD;
}

#ifdef CONFIG_BATTERY_MM8XXX_DYNAMIC_CHARGE_VOLTAGE
/*
 * Return the battery Charge Voltage
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_chargevoltage(struct mm8xxx_device_info *di)
{
	int cv;

	cv = mm8xxx_read(di, MM8XXX_CMD_CHARGEVOLTAGE);
	if (cv < 0)
		dev_err(di->dev, "error reading charge voltage\n");

	return cv;
}

/*
 * Write the battery Charge Voltage.
 */
static int mm8xxx_battery_write_chargevoltage(struct mm8xxx_device_info *di,
					      u16 cv)
{
	int ret;

	ret = mm8xxx_write(di, MM8XXX_CMD_CHARGEVOLTAGE, cv);
	if (ret < 0) {
		dev_err(di->dev, "error writing charge voltage\n");
		return ret;
	}

	return 0;
}
#endif

/*
 * Return the battery usage time in month
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_elapsedmonths(struct mm8xxx_device_info *di)
{
	int elapsed;

	elapsed = mm8xxx_read(di, MM8XXX_CMD_ELAPSEDTIMEM);
	if (elapsed < 0)
		dev_err(di->dev, "error reading elapsed months\n");

	return elapsed;
}

/*
 * Return the battery usage time in day
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_elapseddays(struct mm8xxx_device_info *di)
{
	int elapsed;

	elapsed = mm8xxx_read(di, MM8XXX_CMD_ELAPSEDTIMED);
	if (elapsed < 0)
		dev_err(di->dev, "error reading elapsed days\n");

	return elapsed;
}

/*
 * Return the battery usage time in hour
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_read_elapsedhours(struct mm8xxx_device_info *di)
{
	int elapsed;

	elapsed = mm8xxx_read(di, MM8XXX_CMD_ELAPSEDTIMEH);
	if (elapsed < 0)
		dev_err(di->dev, "error reading elapsed hours\n");

	return elapsed;
}

static bool is_input_present(struct mm8xxx_device_info *di)
{
	int rc = 0, input_present = 0;
	union power_supply_propval pval = {0, };

	if (!di->usb_psy)
		di->usb_psy = power_supply_get_by_name("usb");
	if (di->usb_psy) {
		rc = power_supply_get_property(di->usb_psy,
				POWER_SUPPLY_PROP_PRESENT, &pval);
		if (rc < 0)
			pr_err("Couldn't read USB Present status, rc=%d\n", rc);
		else
			input_present |= pval.intval;
	}

	if (!di->dc_psy)
		di->dc_psy = power_supply_get_by_name("dc");
	if (di->dc_psy) {
		rc = power_supply_get_property(di->dc_psy,
				POWER_SUPPLY_PROP_PRESENT, &pval);
		if (rc < 0)
			pr_err("Couldn't read DC Present status, rc=%d\n", rc);
		else
			input_present |= pval.intval;
	}
	pr_info("input present = %d\n", input_present);
	if (input_present)
		return true;

	return false;
}

#define CAP(min, max, value)			\
		((min > value) ? min : ((value > max) ? max : value))

static void mm8xxx_battery_update(struct mm8xxx_device_info *di)
{
	struct mm8xxx_state_cache cache = {0, };
	bool input_present = is_input_present(di);
#ifdef CONFIG_BATTERY_MM8XXX_DYNAMIC_CHARGE_VOLTAGE
	int cv;
	int req = 0;
#endif

	/* get battery power supply */
	if (!di->batt_psy) {
		di->batt_psy = power_supply_get_by_name("battery");
		if (!di->batt_psy)
			mm_info("get batt_psy fail\n");
	}

	cache.flags = mm8xxx_read(di, MM8XXX_CMD_FLAGS);
	if ((cache.flags & 0xFFFF) == 0xFFFF)
		cache.flags = -1;
	if (cache.flags < 0)
		goto out;

	cache.temperature = mm8xxx_battery_read_temperature(di);
	cache.avg_time_to_empty = mm8xxx_battery_read_averagetimetoempty(di);
	cache.soc = mm8xxx_battery_read_stateofcharge(di);
	cache.full_charge_capacity = mm8xxx_battery_read_fullchargecapacity(di);
	di->cache.flags = cache.flags;
	cache.health = mm8xxx_battery_read_health(di);
	cache.cycle_count = mm8xxx_battery_read_cyclecount(di);
	mm8xxx_battery_temp_to_FG(di);

	mm_info("soc = %d, ui_soc = %d\n", cache.soc, di->cache.soc);
	if (di->cache.soc == 0)
		di->cache.soc = cache.soc;

	if (cache.soc > di->cache.soc) {
		/* SOC increased */
		if (input_present) {/* Increment if input is present */
			cache.soc = di->cache.soc + 1;
		} else
			cache.soc = di->cache.soc;

	} else if (cache.soc < di->cache.soc) {
		/* SOC dropped */
		cache.soc = di->cache.soc - 1;
	}
	cache.soc = CAP(0, 100, cache.soc);

	if (di->charge_design_full <= 0)
		di->charge_design_full = mm8xxx_battery_read_designcapacity(di);

	if (di->cmds[MM8XXX_CMD_ELAPSEDTIMEM] != INVALID_COMMAND)
		cache.elapsed_months = mm8xxx_battery_read_elapsedmonths(di);
	if (di->cmds[MM8XXX_CMD_ELAPSEDTIMED] != INVALID_COMMAND)
		cache.elapsed_days = mm8xxx_battery_read_elapseddays(di);
	if (di->cmds[MM8XXX_CMD_ELAPSEDTIMEH] != INVALID_COMMAND)
		cache.elapsed_hours = mm8xxx_battery_read_elapsedhours(di);

#ifdef CONFIG_BATTERY_MM8XXX_DYNAMIC_CHARGE_VOLTAGE
	if (di->chip != MM8013C10)
		goto out;

	cv = mm8xxx_battery_read_chargevoltage(di);
	if (cv < 0)
		goto out;

	/*
	 * TODO: Change cycle-counts and voltages according to each devices.
	 */
	if ((cache.cycle_count < 250) && (cv != 4400))
		req = 4400;
	else if ((cache.cycle_count < 500) && (cv != 4350))
		req = 4350;
	else if ((cache.cycle_count < 800) && (cv != 4300))
		req = 4300;
	else if (cv != 4250)
		req = 4250;

	if (req == 0)
		goto out;

	mm8xxx_battery_write_chargevoltage(di, cv);
#endif

out:
	if ((di->cache.soc != cache.soc) ||
	    (di->cache.flags != cache.flags)) {
		if (di->batt_psy)
			power_supply_changed(di->batt_psy);
	}

	if (memcmp(&di->cache, &cache, sizeof(cache)) != 0)
		di->cache = cache;

	di->last_update = jiffies;
}

static void mm8xxx_battery_poll(struct work_struct *work)
{
	struct mm8xxx_device_info *di =
		container_of(work, struct mm8xxx_device_info, work.work);

	mm8xxx_battery_update(di);

	if (poll_interval > 0)
		schedule_delayed_work(&di->work, poll_interval * HZ);
}

/*
 * Gets the battery average current in μA and returns 0
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_current(struct mm8xxx_device_info *di,
				  union power_supply_propval *val)
{
	int curr;
	curr = mm8xxx_read(di, MM8XXX_CMD_AVERAGECURRENT);
	if (curr < 0) {
		dev_err(di->dev, "error reading current\n");
		return curr;
	}

	curr = (int)((s16)curr) * 1000;
	val->intval = curr;

	return 0;
}

static int mm8xxx_battery_status(struct mm8xxx_device_info *di,
				 union power_supply_propval *val)
{
	int status;

	if (di->cache.flags & MM8XXX_FLAG_CHG)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else if (di->cache.flags & MM8XXX_FLAG_DSG)
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (di->cache.flags & MM8XXX_FLAG_FC)
		status = POWER_SUPPLY_STATUS_FULL;
	else
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;

	val->intval = status;

	return 0;
}

static int mm8xxx_battery_capacity_level(struct mm8xxx_device_info *di,
					 union power_supply_propval *val)
{
	int level;

	if (di->cache.flags & MM8XXX_FLAG_FC)
		level = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
	else if (di->cache.flags & MM8XXX_FLAG_SOCF)
		level = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
	else
		level = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;

	val->intval = level;

	return 0;
}

/*
 * Gets the battery Voltage in μV and returns 0
 * Or < 0 if something fails.
 */
static int mm8xxx_battery_voltage(struct mm8xxx_device_info *di,
				  union power_supply_propval *val)
{
	int voltage;

	voltage = mm8xxx_read(di, MM8XXX_CMD_VOLTAGE);
	if (voltage < 0) {
		dev_err(di->dev, "error reading voltage\n");
		return voltage;
	}

	val->intval = voltage * 1000;

	return 0;
}

static int mm8xxx_simple_value(int value, union power_supply_propval *val)
{
	if (value < 0)
		return value;

	val->intval = value;

	return 0;
}


static int mm8xxx_battery_set_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       const union power_supply_propval *val)
{
	int ret = 0;
	struct mm8xxx_device_info *di = power_supply_get_drvdata(psy);

	if ((psp != POWER_SUPPLY_PROP_PRESENT) && (di->cache.flags < 0))
		return -ENODEV;

	switch (psp) {
	case POWER_SUPPLY_PROP_TEMP:
		mm8xxx_battery_temp_to_FG(di);
		break;
	case POWER_SUPPLY_PROP_TYPE:
		mm8xxx_battery_chargeType_to_FG(di, val->intval);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
static int mm8xxx_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;
	struct mm8xxx_device_info *di = power_supply_get_drvdata(psy);

	mutex_lock(&di->lock);
	if (time_is_before_jiffies(di->last_update + 5 * HZ)) {
		cancel_delayed_work_sync(&di->work);
		mm8xxx_battery_poll(&di->work.work);
	}
	mutex_unlock(&di->lock);

	if ((psp != POWER_SUPPLY_PROP_PRESENT) && (di->cache.flags < 0))
		return -ENODEV;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = mm8xxx_battery_status(di, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = mm8xxx_battery_voltage(di, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = di->cache.flags < 0 ? 0 : 1;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		ret = mm8xxx_battery_current(di, val);
		val->intval = val->intval * (-1);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = mm8xxx_simple_value(di->cache.soc, val);
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		ret = mm8xxx_battery_capacity_level(di, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		mm8xxx_battery_temp(di, &val->intval);
		val->intval = val->intval / 100;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = mm8xxx_simple_value(di->cache.avg_time_to_empty, val);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		ret = mm8xxx_simple_value(
			mm8xxx_battery_read_remainingcapacity(di), val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = mm8xxx_simple_value(di->cache.full_charge_capacity, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		ret = mm8xxx_simple_value(di->charge_design_full, val);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		ret = mm8xxx_simple_value(di->cache.cycle_count, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = mm8xxx_simple_value(di->cache.health, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = MM8XXX_MANUFACTURER;
		break;

	/*
	 * TODO: Implement appropriate POWER_SUPPLY_PROP property and read
	 * elapsed time.
	 *
	 *   di->cache.elapsed_months
	 *   di->cache.elapsed_days
	 *   di->cache.elapsed_hours
	 */

	default:
		return -EINVAL;
	}

	return ret;
}

static void mm8xxx_external_power_changed(struct power_supply *psy)
{
	struct mm8xxx_device_info *di = power_supply_get_drvdata(psy);

	cancel_delayed_work_sync(&di->work);
	schedule_delayed_work(&di->work, 0);
}

static int mm8xxx_battery_setup(struct mm8xxx_device_info *di)
{
	struct power_supply_desc *ps_desc;
	struct power_supply_config ps_cfg = {
		.of_node = di->dev->of_node,
		.drv_data = di,
	};

	INIT_DELAYED_WORK(&di->work, mm8xxx_battery_poll);
	mutex_init(&di->lock);

	ps_desc = devm_kzalloc(di->dev, sizeof(*ps_desc), GFP_KERNEL);
	if (!ps_desc)
		return -ENOMEM;

	ps_desc->name = MM8XXX_BATT_PHY;
	ps_desc->type = POWER_SUPPLY_TYPE_MAINS;
	ps_desc->properties = mm8xxx_chip_data[di->chip].props;
	ps_desc->num_properties = mm8xxx_chip_data[di->chip].props_size;
	ps_desc->get_property = mm8xxx_battery_get_property;
	ps_desc->set_property = mm8xxx_battery_set_property;
	ps_desc->external_power_changed = mm8xxx_external_power_changed;

	di->psy = power_supply_register(di->dev, ps_desc, &ps_cfg);
	if (IS_ERR(di->psy)) {
		dev_err(di->dev, "failed to register battery\n");
		return PTR_ERR(di->psy);
	}

	if (sysfs_create_group(&di->psy->dev.kobj, &mm8xxx_attribute_group)) {
		mm_info(" Error failed to creat attributes\n");
	}

	mm8xxx_battery_update(di);

	mutex_lock(&mm8xxx_list_lock);
	list_add(&di->list, &mm8xxx_battery_devices);
	mutex_unlock(&mm8xxx_list_lock);

	return 0;
}

static void mm8xxx_battery_teardown(struct mm8xxx_device_info *di)
{
	poll_interval = 0;

	cancel_delayed_work_sync(&di->work);
	power_supply_unregister(di->psy);

	mutex_lock(&mm8xxx_list_lock);
	list_del(&di->list);
	mutex_unlock(&mm8xxx_list_lock);

	mutex_destroy(&di->lock);
}

static void mm8xxx_fake_battery_update(struct mm8xxx_device_info *di)
{
	di->cache.temperature = 250;
	di->cache.avg_time_to_empty = 3600;
	di->cache.soc = 10;
	di->cache.full_charge_capacity = 4020000;
	di->cache.flags = 0;
	di->cache.health = POWER_SUPPLY_HEALTH_GOOD;
	di->cache.cycle_count = 0;

	di->charge_design_full = 4020000;

	di->last_update = jiffies;
}

static void mm8xxx_fake_battery_poll(struct work_struct *work)
{
	struct mm8xxx_device_info *di =
		container_of(work, struct mm8xxx_device_info, work.work);

	mm8xxx_fake_battery_update(di);

	if (poll_interval > 0)
		schedule_delayed_work(&di->work, poll_interval * HZ);
}

static int mm8xxx_fake_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;
	struct mm8xxx_device_info *di = power_supply_get_drvdata(psy);

	mutex_lock(&di->lock);
	if (time_is_before_jiffies(di->last_update + 5 * HZ)) {
		cancel_delayed_work_sync(&di->work);
		mm8xxx_fake_battery_poll(&di->work.work);
	}
	mutex_unlock(&di->lock);

	if ((psp != POWER_SUPPLY_PROP_PRESENT) && (di->cache.flags < 0))
		return -ENODEV;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = 4200 * 1000;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = mm8xxx_simple_value(di->cache.soc, val);
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = di->cache.temperature;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = mm8xxx_simple_value(di->cache.avg_time_to_empty, val);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = di->cache.full_charge_capacity * di->cache.soc / 100;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = mm8xxx_simple_value(di->cache.full_charge_capacity, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		ret = mm8xxx_simple_value(di->charge_design_full, val);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		ret = mm8xxx_simple_value(di->cache.cycle_count, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = mm8xxx_simple_value(di->cache.health, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = MM8XXX_MANUFACTURER;
		break;

	/*
	 * TODO: Implement appropriate POWER_SUPPLY_PROP property and read
	 * elapsed time.
	 *
	 *   di->cache.elapsed_months
	 *   di->cache.elapsed_days
	 *   di->cache.elapsed_hours
	 */

	default:
		return -EINVAL;
	}

	return ret;
}

static void mm8xxx_fake_external_power_changed(struct power_supply *psy)
{
	struct mm8xxx_device_info *di = power_supply_get_drvdata(psy);

	cancel_delayed_work_sync(&di->work);
	schedule_delayed_work(&di->work, 0);
}

static int mm8xxx_fake_battery_setup(struct mm8xxx_device_info *di)
{
	struct power_supply_desc *ps_desc;
	struct power_supply_config ps_cfg = {
		.of_node = di->dev->of_node,
		.drv_data = di,
	};

	INIT_DELAYED_WORK(&di->work, mm8xxx_fake_battery_poll);
	mutex_init(&di->lock);

	ps_desc = devm_kzalloc(di->dev, sizeof(*ps_desc), GFP_KERNEL);
	if (!ps_desc)
		return -ENOMEM;

	ps_desc->name = "bms";
	ps_desc->type = POWER_SUPPLY_TYPE_MAINS;
	ps_desc->properties = mm8xxx_chip_data[di->chip].props;
	ps_desc->num_properties = mm8xxx_chip_data[di->chip].props_size;
	ps_desc->get_property = mm8xxx_fake_battery_get_property;
	ps_desc->external_power_changed = mm8xxx_fake_external_power_changed;

	di->psy = power_supply_register(di->dev, ps_desc, &ps_cfg);
	if (IS_ERR(di->psy)) {
		dev_err(di->dev, "failed to register battery\n");
		return PTR_ERR(di->psy);
	}

	mm8xxx_fake_battery_update(di);

	mutex_lock(&mm8xxx_list_lock);
	list_add(&di->list, &mm8xxx_battery_devices);
	mutex_unlock(&mm8xxx_list_lock);

	return 0;
}

static u32 mmi_get_battery_info(struct mm8xxx_device_info *di, u32 cmd)
{
	int ret = 0;

	ret = mm8xxx_write(di, MM8XXX_CMD_CONTROL, cmd);
	if (ret < 0) {
		dev_err(di->dev, "error writing cmd control\n");
		return ret;
	}

	ret = mm8xxx_read(di, MM8XXX_CMD_CONTROL);
	if (ret < 0) {
		dev_err(di->dev, "error read cmd control\n");
		return ret;
	}

	return (u32)ret;
}

static int mm8xxx_battery_parse_dts(struct mm8xxx_device_info *di)
{
	struct device_node *np = di->dev->of_node;
	int rc;

	rc = of_property_read_u32(np, "latest_fw_version", &di->latest_fw_version);
	if(rc < 0){
		di->latest_fw_version = MM8013_LATEST_FW_VERSION;
		mm_info("dts no config fw version, use default fw version=0x%04x\n", di->latest_fw_version);
	}

	rc = of_property_read_u32(np, "first_battery_param_ver", &di->first_battery_param_ver);
	if (rc < 0) {
		di->first_battery_param_ver = MM8013_PARAMETER_VERSION;
		mm_info("dts no config parameter version, first_battery_param_ver=0x%04x\n", di->first_battery_param_ver);
	}

	rc = of_property_read_u32(np, "second_battery_param_ver", &di->second_battery_param_ver);
	if (rc < 0) {
		di->second_battery_param_ver = MM8013_PARAMETER_VERSION;
		mm_info("dts no config parameter version, second_battery_param_ver=0x%04x\n", di->second_battery_param_ver);
	}

	rc = of_property_read_u32(np, "first_battery_id", &di->first_battery_id);
	if (rc < 0) {
		di->first_battery_id = MM8013_1TH_BATTERY_ID;
	}

	rc = of_property_read_u32(np, "second_battery_id", &di->second_battery_id);
	if (rc < 0) {
		di->second_battery_id = MM8013_2TH_BATTERY_ID;
	}

	return rc;
}

bool is_factory_mode(void)
{
	struct device_node *np = of_find_node_by_path("/chosen");
	bool factory_mode = false;
	const char *bootargs = NULL;
	char *bootmode = NULL;
	char *end = NULL;

	if (!np)
		return factory_mode;

	if (!of_property_read_string(np, "bootargs", &bootargs)) {
		bootmode = strstr(bootargs, "androidboot.mode=");
		if (bootmode) {
			end = strpbrk(bootmode, " ");
			bootmode = strpbrk(bootmode, "=");
		}
		if (bootmode &&
		    end > bootmode &&
		    strnstr(bootmode, "mot-factory", end - bootmode)) {
				factory_mode = true;
		}
	}
	of_node_put(np);

	return factory_mode;
}

static int mm8xxx_battery_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	char *name;
	int num;
	u32 fg_fw_ver;
	u32 fg_param_ver;
	u32 fg_battery_id;
	u32 parameter_version = 0xFFFF;
	enum UPDATE_INDEX update_index =UPDATE_NONE;

	mm_info("MM8013 prob begin\n");

	mutex_lock(&battery_mutex);
	num = idr_alloc(&battery_id, client, 0, 0, GFP_KERNEL);
	mutex_unlock(&battery_mutex);
	if (num < 0)
		return num;

	name = devm_kasprintf(&client->dev, GFP_KERNEL, "%s-%d", id->name, num);
	if (!name)
		goto mem_err;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		goto mem_err;

	i2c_set_clientdata(client, di);

	mutex_init(&di->i2c_rw_lock);

	di->id = num;
	di->dev = &client->dev;
	di->chip = id->driver_data;
	di->name = name;

	di->bus.read = mm8xxx_battery_read;
	di->bus.write = mm8xxx_battery_write;
	di->cmds = mm8xxx_chip_data[di->chip].cmds;

	mm8xxx_battery_parse_dts(di);

	ret = mmi_get_battery_info(di, HW_VER_CMD);
	if (ret != MM8013_HW_VERSION) {
		di->fake_battery = true;
		mm_info("don't have real battery,use fake battery\n");
	}

	if (!di->fake_battery)
	{
		fg_fw_ver = mmi_get_battery_info(di, FW_VER_CMD);
		fg_param_ver = mmi_get_battery_info(di, PARAM_VER_CMD);
		fg_battery_id = mmi_get_battery_info(di, BATTERY_ID_CMD);
		mm_info("From fg ic,fg_fw_ver=0x%04x, battery_id=0x%04x,fg_param_ver=0x%04x\n", \
					fg_fw_ver, fg_battery_id, fg_param_ver);

		if (is_factory_mode())
		{
			if (fg_fw_ver < 0 || fg_param_ver < 0 || fg_battery_id < 0) {
				update_index = UPDATE_NONE;
				goto update_begin;
			}
			else if (fg_battery_id == di->first_battery_id) {
				parameter_version = di->first_battery_param_ver;
			}
			else if (fg_battery_id == di->second_battery_id) {
				parameter_version = di->second_battery_param_ver;
			}
			if (fg_fw_ver < di->latest_fw_version) {
				if (fg_param_ver < parameter_version)
					update_index = UPDATE_ALL;
				else if (fg_param_ver == parameter_version)
					update_index = UPDATE_PROGRAM;
				else if (fg_param_ver == 0xa200)
					update_index = UPDATE_ALL;
			}
			else if (fg_param_ver < parameter_version || fg_param_ver == 0xa200)
					update_index = UPDATE_PARAMETER;

			update_begin:
			if (update_index != UPDATE_NONE)
				mm8xxx_battery_update_program_and_parameter(di, update_index);
		}
	}

	di->Batt_NTC_channel = devm_iio_channel_get(&client->dev, "batt_therm");
	if (IS_ERR(di->Batt_NTC_channel)) {
		dev_err(&client->dev, "failed to get batt_therm IIO channel\n");
		ret = PTR_ERR(di->Batt_NTC_channel);
		//goto failed;
	}

	if (di->fake_battery)
		ret = mm8xxx_fake_battery_setup(di);
	else
		ret = mm8xxx_battery_setup(di);

	if (ret)
		goto failed;

	schedule_delayed_work(&di->work, 60 * HZ);

	if (client->irq) {
		ret = devm_request_threaded_irq(&client->dev, client->irq,
				NULL, mm8xxx_battery_irq_handler_thread,
				IRQF_ONESHOT,
				di->name, di);
		if (ret) {
			dev_err(&client->dev,
				"Unable to register IRQ %d error %d\n",
				client->irq, ret);

			return ret;
		}
	}

	mm_info("MM8013 driver probe success\n");
	return 0;

mem_err:
	ret = -ENOMEM;

failed:
	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, num);
	mutex_unlock(&battery_mutex);

	return ret;
}

static int mm8xxx_battery_remove(struct i2c_client *client)
{
	struct mm8xxx_device_info *di = i2c_get_clientdata(client);

	mm8xxx_battery_teardown(di);

	mutex_lock(&battery_mutex);
	idr_remove(&battery_id, di->id);
	mutex_unlock(&battery_mutex);

	return 0;
}

static const struct i2c_device_id mm8xxx_battery_id_table[] = {
	{ "mm8118g01", MM8118G01 },
	{ "mm8118w02", MM8118W02 },
	{ "mm8013c10", MM8013C10 },
	{},
};
MODULE_DEVICE_TABLE(i2c, mm8xxx_battery_id_table);

#ifdef CONFIG_OF
static const struct of_device_id mm8xxx_battery_of_match_table[] = {
	{ .compatible = "mitsumi,mm8118g01" },
	{ .compatible = "mitsumi,mm8118w02" },
	{ .compatible = "mitsumi,mm8013c10" },
	{},
};
MODULE_DEVICE_TABLE(of, mm8xxx_battery_of_match_table);
#endif

static struct i2c_driver mm8xxx_battery_driver = {
	.driver = {
		.name = "mm8xxx-battery",
		.of_match_table = of_match_ptr(mm8xxx_battery_of_match_table),
	},
	.probe = mm8xxx_battery_probe,
	.remove = mm8xxx_battery_remove,
	.id_table = mm8xxx_battery_id_table,
};

static int __init mm8xxx_charger_init(void)
{
	return i2c_add_driver(&mm8xxx_battery_driver);
}

static void __exit mm8xxx_charger_exit(void)
{
	i2c_del_driver(&mm8xxx_battery_driver);
}

//module_i2c_driver(mm8xxx_battery_driver);
device_initcall(mm8xxx_charger_init);
module_exit(mm8xxx_charger_exit);

MODULE_AUTHOR("Takayuki Sugaya");
MODULE_AUTHOR("Yasuhiro Kinoshita");
MODULE_DESCRIPTION("MM8xxx battery monitor");
MODULE_LICENSE("GPL");
