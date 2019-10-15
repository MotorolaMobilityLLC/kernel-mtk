/*
 * Copyright (C) 2018 Fourier Semiconductor Inc. All rights reserved.
 */
#define pr_fmt(fmt) "%s:%d: " fmt "\n", __func__, __LINE__
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#if defined(CONFIG_REGULATOR)
#include <linux/regulator/consumer.h>
#endif


#define FUNC_ENTER() pr_debug("enter.")
#define FUNC_EXIT(ret) \
do { \
	if((ret) != 0) { \
		pr_err("exit. ret: %d.", ret); \
	} \
} while(0)
#define FS_PRINT_VNAME(addr, val)	pr_info("[0x%02x] %s: %d.", addr, (#val), (val))

#define VOID_FUNC_ENTER()		pr_debug("enter.")
#define VOID_FUNC_EXIT()		pr_debug("exit.")

#define FS16XX_VERSION				"v1.0.1"
#define FS16XX_DRIVER_VERSION			"driver version: " FS16XX_VERSION

/* ------------ custom settings. -------------*/
#define FS16XX_MAX				2
#define FS16XX_I2C_NAME				"smartpa_i2c"
/* ------------ custom settings. -------------*/

#define FS1601_ID				0x0301
#define FS1601S_ID				0x0302
#define FS1603_ID				0x0501
#define FS1818_ID				0x0603

#define FS16XX_ID_REG				0x03

/* ioctl magic number and cmd. */
#define FS16XX_MAGIC				0xc7
#define FS16XX_IOC_I2C_SLAVE			0x0703
#define FS16XX_IOC_I2C_SLAVE_FORCE		0x0706

#define FS16XX_I2C_RETRY_DELAY			5	/*ms*/
#define FS16XX_I2C_RETRY_TIMES			10
#define FS16XX_ARRAY_SIZE(arr)			(sizeof(arr)/sizeof(arr[0]))

enum fs16xx_family
{
	FS16XX_FAM_UNKNOW = 0,
	FS16XX_FAM_FS1601,
	FS16XX_FAM_FS1601S,
	FS16XX_FAM_FS1603,
//	FS16XX_FAM_FS1801,
	FS16XX_FAM_FS1818,
	FS16XX_FAM_MAX,
};

struct fs16xx_set_mutex
{
	int count;
	struct mutex fsdev_lock;
};
typedef struct fs16xx_set_mutex fs16xx_set_mutex_t;

/* fs16xx private data. */
struct fs16xx_pdata
{
	uint8_t dev_num; /* device number we get. */
	uint8_t cur_dev; /* mark current device. */
	enum fs16xx_family dev_family[FS16XX_MAX]; /* mark device family for compatible. */
	struct i2c_client *client[FS16XX_MAX]; /* i2c client for i2c operation in kernel. */
	fs16xx_set_mutex_t fs_mutex; /* mutex lock avoid multi-thread. */
	struct miscdevice fsdev; /* misc device for hal layer operation. */
#if defined(CONFIG_REGULATOR)
	struct regulator *fsvdd; /* for 1.8V vdd voltage. */
#endif
};
typedef struct fs16xx_pdata fs16xx_data_t;


static fs16xx_data_t *g_fs16xx_pdata = NULL;


fs16xx_data_t *fs16xx_get_pdata(void)
{
	fs16xx_data_t *fs16xx = g_fs16xx_pdata;
	return fs16xx;
}

void fs16xx_set_pdata(fs16xx_data_t *data)
{
	g_fs16xx_pdata = data;
}

fs16xx_data_t *fs16xx_alloc_pdata(void)
{
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();
	uint8_t dev = 0;
	/* allocate struct data */
	if(fs16xx == NULL)
	{
		fs16xx = kzalloc(sizeof(fs16xx_data_t), GFP_KERNEL);
		if(!fs16xx)
		{
			pr_err("failed to allocate %d bytes.", (int)sizeof(fs16xx_data_t));
			return NULL;
		}
		fs16xx->dev_num = 0;
		fs16xx->cur_dev = 0;
		fs16xx->fsvdd = NULL;
		for(dev = 0; dev < FS16XX_MAX; dev++)
		{
			fs16xx->dev_family[dev] = FS16XX_FAM_UNKNOW;
			fs16xx->client[dev] = NULL;
		}
		mutex_init(&fs16xx->fs_mutex.fsdev_lock);
		fs16xx->fs_mutex.count = 0;
		fs16xx_set_pdata(fs16xx);
	}
	return fs16xx;
}

void fs16xx_set_client(uint8_t dev, struct i2c_client *client)
{
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();

	if(fs16xx == NULL)
		return;
	fs16xx->client[dev] = client;
}

struct i2c_client *fs16xx_get_client(uint8_t dev)
{
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();

	if(fs16xx == NULL)
		return NULL;
	return fs16xx->client[dev];
}

void fs16xx_delay_ms(uint32_t delay_ms)
{
	if(delay_ms <= 0) return;
	// usleep_range: usleep min time: 50us
	// usleep_range 用于非原子环境的睡眠，目前内核建议用这个函数替换之前udelay
	// 关闭中断时应用 mdelay
	// 1. 在原子上下文，延迟应该少于100微秒: 使用udelay
	// 2. 在非原子上下文延迟的使用
	//    0-100us： 使用udelay
	//    100us以上： 使用usleep_range
	//    20ms以上且不要求精确： 使用msleep
	//    msleep不精确，完全可以用usleep_range代替。
	usleep_range(delay_ms * 1000, delay_ms * 1000 + 1);
}

void fs16xx_lock(void)
{
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();

	if(fs16xx == NULL) return;

	fs16xx->fs_mutex.count++;
	mutex_lock(&fs16xx->fs_mutex.fsdev_lock);
	pr_debug("count: %d.", fs16xx->fs_mutex.count);
}

void fs16xx_unlock(void)
{
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();

	if(fs16xx == NULL) return;

	fs16xx->fs_mutex.count--;
	mutex_unlock(&fs16xx->fs_mutex.fsdev_lock);
	pr_debug("count: %d.", fs16xx->fs_mutex.count);
}

int fs16xx_read_register16(uint8_t dev, uint8_t reg_addr, uint16_t *read_data)
{
	int ret = 0;
	uint8_t retries = FS16XX_I2C_RETRY_TIMES;
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();
	uint8_t data[2] = {0};
	struct i2c_msg msgs[2];

	if(fs16xx == NULL || read_data == NULL || dev >= FS16XX_MAX) return -EINVAL;

	// write register address.
	msgs[0].addr = fs16xx->client[dev]->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg_addr;
	// read register data.
	msgs[1].addr = fs16xx->client[dev]->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = &data[0];

	do
	{
		fs16xx_lock();
		ret = i2c_transfer(fs16xx->client[dev]->adapter, &msgs[0], FS16XX_ARRAY_SIZE(msgs));
		fs16xx_unlock();
		if (ret != FS16XX_ARRAY_SIZE(msgs))
			fs16xx_delay_ms(FS16XX_I2C_RETRY_DELAY);
	}
	while ((ret != FS16XX_ARRAY_SIZE(msgs)) && (--retries > 0));

	if (ret != FS16XX_ARRAY_SIZE(msgs))
	{
		pr_err("[%d] read transfer error, ret: %d.", dev, ret);
		return -EIO;
	}

	*read_data = ((data[0] << 8) | data[1]);

	return 0;
}

int fs16xx_write_register16(uint8_t dev, uint8_t reg_addr, const uint16_t write_data)
{
	int ret = 0;
	uint8_t retries = FS16XX_I2C_RETRY_TIMES;
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();
	uint8_t data[3] = {0};
	struct i2c_msg msgs[1];

	if(fs16xx == NULL || dev >= FS16XX_MAX) return -EINVAL;

	// write register data: [addr, data_h, data_l].
	data[0] = reg_addr;
	data[1] = (write_data >> 8) & 0x00ff;
	data[2] = write_data & 0x00ff;

	msgs[0].addr = fs16xx->client[dev]->addr;
	msgs[0].flags = 0;
	msgs[0].len = 3;
	msgs[0].buf = &data[0];

	do
	{
		fs16xx_lock();
		ret = i2c_transfer(fs16xx->client[dev]->adapter, &msgs[0], FS16XX_ARRAY_SIZE(msgs));
		fs16xx_unlock();
		if (ret != FS16XX_ARRAY_SIZE(msgs))
			fs16xx_delay_ms(FS16XX_I2C_RETRY_DELAY);
	}
	while ((ret != FS16XX_ARRAY_SIZE(msgs)) && (--retries > 0));

	if (ret != FS16XX_ARRAY_SIZE(msgs))
	{
		pr_err("[%d] write transfer error, ret: %d.", dev, ret);
		return -EIO;
	}

	return 0;
}

int fs16xx_write_data(uint8_t dev, const uint8_t *write_data, int num_bytes)
{
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();
	int ret = 0, retries = FS16XX_I2C_RETRY_TIMES;
	bool result = false;

	if(fs16xx == NULL || write_data == NULL || dev >= FS16XX_MAX) return -EINVAL;

	do
	{
		// TODO use dma transfer for mtk platform.
		fs16xx_lock();
		ret = i2c_master_send(fs16xx->client[dev], write_data, num_bytes);
		fs16xx_unlock();
		result = (ret == num_bytes) ? true : false;
		if(!result) fs16xx_delay_ms(FS16XX_I2C_RETRY_DELAY);
	}
	while(!result && --retries > 0);
	if(retries == 0)
		pr_err("retry write data failed.");

	return 0;
}

int fs16xx_detect_sensor(uint8_t dev)
{
	int ret = 0;
	uint16_t reg_id = 0xffff;
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();
	enum fs16xx_family fs_fam = FS16XX_FAM_UNKNOW;

	if(fs16xx == NULL)
	{
		pr_err("[%d] bad parameter.", dev);
		return -EINVAL;
	}

	/* read id register. */
	ret = fs16xx_read_register16(dev, FS16XX_ID_REG, &reg_id);
	if(ret)
	{
		pr_err("[%d] read id failed, ret: %d.", dev, ret);
		return -ENODEV;
	}
	pr_info("[%d] read id success: 0x%04x.", dev, reg_id);
	/* check id. */
	switch(reg_id)
	{
		case FS1601S_ID:
			fs_fam = FS16XX_FAM_FS1601S;
			pr_info("[%d] FS1601S detected.", dev);
			break;
		case FS1601_ID:
			fs_fam = FS16XX_FAM_FS1601;
			pr_info("[%d] FS1601 detected.", dev);
			break;
		case FS1603_ID:
			fs_fam = FS16XX_FAM_FS1603;
			pr_info("[%d] FS1603 detected.", dev);
			break;
		case FS1818_ID:
			fs_fam = FS16XX_FAM_FS1818;
			pr_info("[%d] FS1818 detected.", dev);
			break;
		default:
			pr_err("id: 0x%04x is not FourSemi device!", reg_id);
			break;
	}
	fs16xx->dev_family[dev] = fs_fam;

	return ((fs_fam > FS16XX_FAM_UNKNOW && fs_fam < FS16XX_FAM_MAX) ? 0 : -1);
}

void fs16xx_byte_to_str(char *str, char *fmt, int fmt_len, const uint8_t *buf, int len)
{
	int i = 0;
	uint8_t wTmp;
	for(i = 0; i < len; i++)
	{
		wTmp = buf[i] & 0xff;
		sprintf(str, fmt, wTmp);
		str += fmt_len;
	}
}

void fs16xx_word_to_str(char *str, char *fmt, int fmt_len, const uint16_t *buf, int len)
{
	int i = 0;
	uint16_t wTmp;
	for(i = 0; i < len; i++)
	{
		wTmp = buf[i] & 0xffff;
		sprintf(str, fmt, wTmp);
		str += fmt_len;
	}
}

int fs16xx_dump_regs(uint8_t dev)
{
	int reg = 0;
	uint16_t value[0x10 + 1];
	char result[3 + 7 * 16 + 1];
	int ret = 0;

	for(reg = 0; reg <= 0xf; reg++)
		value[reg] = reg;
	sprintf(&result[0], "%s", "   ");
	fs16xx_word_to_str(&result[3], "[ -%1x ] ", 7, &value[0], 0x10);
	result[3 + 7 * 16] = '\0';
	pr_info("%s", result);
	for(reg = 0; reg <= 0xef; reg++) //FS16XX_REG_MAX
	{
		if(reg == 0x10)
			reg= 0x80;
		ret |= fs16xx_read_register16(dev, reg, &value[reg % 16]);
		//value[reg % 16] = reg; // for test.
		if(((reg + 1) % 0x10) == 0)
		{
			sprintf(&result[0], "%x- ", reg / 0x10);
			fs16xx_word_to_str(&result[3], "0x%04x ", 7, &value[0], 0x10);
			result[3 + 7 * 16] = '\0';
			pr_info("%s", result);
		}
	}

	return ret;
}

/*----------------- ioctl operation --------------------*/
static int fs16xx_dev_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	fs16xx_data_t *fs16xx = NULL;
	// fs16xx_data_t *pdata = container_of(filp->private_data, fs16xx_data_t, fsdev);
	FUNC_ENTER();
	fs16xx = fs16xx_get_pdata();
	if(fs16xx == NULL)
	{
		pr_err("fs16xx is null, return error.");
		filp->private_data = NULL;
		return -EINVAL;
	}
	fs16xx_lock();
	filp->private_data = fs16xx;
	fs16xx_unlock();

	return ret;
}
int fs16xx_set_i2c_slave(fs16xx_data_t *fs16xx, uint16_t addr)
{
	int ret = -ENODEV;
	uint8_t dev = 0;

	for(dev = 0; dev < fs16xx->dev_num; dev++)
	{
		if(dev > FS16XX_MAX || fs16xx->client[dev] == NULL)
		{
			ret = -1;
			break;
		}
		if(((fs16xx->client[dev]->flags & I2C_M_TEN) == 0) && addr > 0x7f)
		{
			ret = -EINVAL;
			continue;
		}
		if(addr == fs16xx->client[dev]->addr)
		{
			if(fs16xx->cur_dev != dev)
			{
				fs16xx->cur_dev = dev;
			}
			ret = 0;
			break;
		}
	}
	return ret;
}

static long fs16xx_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int8_t ret = -ENODEV;
	fs16xx_data_t *fs16xx = filp->private_data;

	pr_debug("cmd=0x%x, arg=0x%lx", cmd, arg);
	if(fs16xx == NULL) return -1;
	switch (cmd)
	{
		case FS16XX_IOC_I2C_SLAVE:
		case FS16XX_IOC_I2C_SLAVE_FORCE:
			if (arg > 0x3ff) return -EINVAL;
			fs16xx_lock();
			ret = fs16xx_set_i2c_slave(fs16xx, arg);
			fs16xx_unlock();
			break;
		default:
			pr_err("bad ioctl cmd: 0x%04x", cmd);
			break;
	}
	FUNC_EXIT(ret);

	return ret;
}

#if defined(CONFIG_COMPAT)
static long fs16xx_dev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return fs16xx_dev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static ssize_t fs16xx_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0, retries = FS16XX_I2C_RETRY_TIMES;
	uint8_t dev = 0;
	bool result = false;
	char *tmp;
	fs16xx_data_t *fs16xx = filp->private_data;

	//FUNC_ENTER();
	if(fs16xx == NULL) return -EINVAL;

	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;

	pr_debug("reading %zu bytes.", count);

	do
	{
		// TODO use dma transfer for mtk platform.
		fs16xx_lock();
		dev = fs16xx->cur_dev;
		if(dev < FS16XX_MAX && fs16xx->client[dev] != NULL)
			ret = i2c_master_recv(fs16xx->client[dev], tmp, count);
		fs16xx_unlock();
		result = (ret == count) ? true : false;
		if(!result)
			fs16xx_delay_ms(FS16XX_I2C_RETRY_DELAY);
	}
	while(!result && (--retries <= 0));
	if (ret > 0)
		ret = copy_to_user(buf, tmp, count) ? -EFAULT : ret;
	else
		pr_err("reading %zu bytes failed, ret: %d.", count, ret);
	if(tmp)
		kfree(tmp);

	return ret;
}

static ssize_t fs16xx_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0, retries = FS16XX_I2C_RETRY_TIMES;
	uint8_t dev = 0;
	bool result = false;
	char *tmp;
	fs16xx_data_t *fs16xx = filp->private_data;

	//FUNC_ENTER();
	if(fs16xx == NULL) return -EINVAL;

	if (count > 8192)
		count = 8192;

	tmp = memdup_user(buf, count);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	pr_debug("writing %zu bytes.", count);

	do
	{
		// TODO use dma transfer for mtk platform.
		fs16xx_lock();
		dev = fs16xx->cur_dev;
		if(dev < FS16XX_MAX && fs16xx->client[dev] != NULL)
			ret = i2c_master_send(fs16xx->client[dev], tmp, count);
		fs16xx_unlock();
		result = (ret == count) ? true : false;
		if(!result)
			fs16xx_delay_ms(FS16XX_I2C_RETRY_DELAY);
	}
	while(!result && (--retries <= 0));
	if(ret <= 0)
		pr_err("writing %zu bytes failed, ret: %d.", count, ret);
	if(tmp)
		kfree(tmp);

	return ret;
}

static const struct file_operations fs16xx_dev_fops =
{
	.owner	= THIS_MODULE,
	.open	= fs16xx_dev_open,
	.unlocked_ioctl = fs16xx_dev_ioctl,
#if defined(CONFIG_COMPAT)
	.compat_ioctl = fs16xx_dev_compat_ioctl,
#endif
	.llseek	= no_llseek,
	.read	= fs16xx_dev_read,
	.write	= fs16xx_dev_write,
};

int fs16xx_misc_init(fs16xx_data_t *fs16xx)
{
	int ret = 0;

	if(fs16xx == NULL) return -EINVAL;

	/* register misc device */
	fs16xx->fsdev.minor = MISC_DYNAMIC_MINOR;
	fs16xx->fsdev.name = FS16XX_I2C_NAME;
	fs16xx->fsdev.fops = &fs16xx_dev_fops;
	ret = misc_register(&fs16xx->fsdev);
	if (ret)
		pr_err("register misc device failed");

	return ret;
}

static uint8_t g_dump_dev_idx = 0;
static ssize_t fs16xx_dump_all(struct device *dev,struct device_attribute *attr, char *buf)
{
	char dump_buf[18 + (4 + 7 * 16) * 8 + 2];
	uint8_t dev_count = 0;
	uint16_t reg = 0, value = 0;
	int idx = 0;
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();
	uint8_t dev_idx = g_dump_dev_idx;

	if(fs16xx == NULL || fs16xx->client[dev_idx] == NULL)
	{
		pr_err("bad parameter.");
		return 0;
	}
	dev_count = fs16xx->dev_num;
	memset(dump_buf, 0, sizeof(dump_buf));
	sprintf(&dump_buf[idx], "Dump device[0x%02x]:", (uint8_t)fs16xx->client[dev_idx]->addr);
	idx += 18;
	for(reg = 0; reg <= 0xf; reg++)
	{
		if(reg == 0)
		{
			sprintf(&dump_buf[idx], "\n%s", "   ");
			idx += 4;
		}
		sprintf(&dump_buf[idx], "[ -%1x ] ", reg);
		idx += 7;
	}
	for(reg = 0; reg <=0xef; reg++)
	{
		if(reg == 0x10)
			reg = 0x90;
		if((reg % 0x10) == 0)
		{
			sprintf(&dump_buf[idx], "\n%1x- ", reg / 0x10);
			idx += 4;
		}
		fs16xx_read_register16(dev_idx, reg, &value);
		sprintf(&dump_buf[idx], "0x%04x ", value);
		idx += 7;
	}
	dump_buf[idx++] = '\n';
	dump_buf[idx] = '\0';
	pr_debug("dev_idx: %d, lengh: %d.", dev_idx, idx);
	dev_idx++;
	if(dev_idx == dev_count)
		dev_idx = 0;
	g_dump_dev_idx = dev_idx;

	return scnprintf(buf, PAGE_SIZE, "%s", &dump_buf[0]);
}

DEVICE_ATTR(dump_all, 0444, fs16xx_dump_all, NULL);

static struct attribute *fs16xx_attrs[] =
{
	&dev_attr_dump_all.attr,
	NULL
};

static const struct attribute_group fs16xx_attr_group =
{
	.attrs = fs16xx_attrs,
};

int fs16xx_init_probe(struct i2c_client *client)
{
	fs16xx_data_t *fs16xx = fs16xx_alloc_pdata();
	uint8_t dev = 0;
	int ret = 0;

	if(fs16xx == NULL || client == NULL)
	{
		pr_err("parameter is NULL!");
		return -EINVAL;
	}

	dev = fs16xx->dev_num;
	fs16xx_set_client(dev, client);
	ret = fs16xx_detect_sensor(dev);
	if(ret)
	{
		pr_err("detect sensor failed! ret: %d.", ret);
		fs16xx_set_client(dev, NULL);
		return ret;
	}
	fs16xx->cur_dev = dev;
	fs16xx->dev_num = dev + 1;

	return ret;
}

#if 0
struct regulator *fs16xx_power_on(struct i2c_client *i2c)
{
	struct regulator *fs16xx_vdd = NULL;
	int ret = 0;
	fs16xx_vdd = regulator_get(&i2c->dev, "fs16xx_vdd");
	if(IS_ERR(fs16xx_vdd) != 0)
	{
		pr_err("error getting vdd regulator.");
		ret = PTR_ERR(fs16xx_vdd);
		return NULL;
	}
	regulator_set_voltage(fs16xx_vdd, 1800000, 1800000);
	ret = regulator_enable(fs16xx_vdd);
	if (ret < 0) {
		pr_err("enabling vdd regulator failed, ret: %d.", ret);
	}
	return fs16xx_vdd;
}

void fs16xx_power_off(fs16xx_data_t *fs16xx)
{
	if(fs16xx != NULL && fs16xx->fsvdd != NULL)
	{
		regulator_disable(fs16xx->fsvdd);
		regulator_put(fs16xx->fsvdd);
		fs16xx->fsvdd = NULL;
	}
}
#endif

static int fs16xx_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	int ret = 0;
	uint8_t dev_current = 0;
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();

	pr_info("%s", FS16XX_DRIVER_VERSION);
	do
	{
		/* check i2c functionality */
		ret = i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C);
		if (!ret)
		{
			pr_err("check i2c functionality failed.");
			ret = -ENODEV;
			break;
		}

		if(fs16xx == NULL)
		{
			fs16xx = fs16xx_alloc_pdata();
			/* power on device. */
#if 0
			if(fs16xx != NULL && fs16xx->fsvdd == NULL)
				fs16xx->fsvdd = fs16xx_power_on(i2c);
#endif
			fs16xx_delay_ms(10);
		}

		/* initialize and detect sensor. */
		ret = fs16xx_init_probe(i2c);
		if(ret) break;

		dev_current = fs16xx->cur_dev;

		/* init misc device file in /dev/. */
		if(dev_current == 0)
		{
			ret = fs16xx_misc_init(fs16xx);
			if(ret) break;
			/* register dump all regiters sysfs file. */
			ret = sysfs_create_group(&fs16xx->fsdev.this_device->kobj, &fs16xx_attr_group);
			if(ret)
			{
				pr_err("create sysfs failed.");
			}
		}

		/* initialize i2c client data. */
		//i2c_set_clientdata(i2c, fs16xx);
	}
	while(0);

	if(!ret)
	{// ok
		pr_info("[%d] dev[0x%02x] probe finished.", dev_current, i2c->addr);
	}
	else
	{// failed
		if(fs16xx != NULL && fs16xx->dev_num <= 0)
		{
			pr_err("no device detected, free resource.");
#if 0
			fs16xx_power_off(fs16xx);
#endif
			kfree(fs16xx);
			fs16xx = NULL;
			fs16xx_set_pdata(NULL);
		}
	}

	FUNC_EXIT(ret);

	return ret;
}

static int fs16xx_i2c_remove(struct i2c_client *i2c)
{
	fs16xx_data_t *fs16xx = fs16xx_get_pdata();
	FUNC_ENTER();

	if(fs16xx != NULL)
	{
#if 0
		fs16xx_power_off(fs16xx);
#endif
		misc_deregister(&fs16xx->fsdev);
		kfree(fs16xx);
		fs16xx_set_pdata(NULL);
	}

	return 0;
}

static const struct i2c_device_id fs16xx_i2c_id[] =
{
	{ FS16XX_I2C_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fs16xx_i2c_id);

#ifdef CONFIG_OF
static struct of_device_id fs16xx_match_tbl[] =
{
	{ .compatible = "foursemi,fs16xx" },
	{ },
};
#endif

static struct i2c_driver fs16xx_i2c_driver =
{
    .driver =
	{
        .name = FS16XX_I2C_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(fs16xx_match_tbl),
    },
    .probe =    fs16xx_i2c_probe,
    .remove =   fs16xx_i2c_remove,
    .id_table = fs16xx_i2c_id,
};

#if !defined(module_i2c_driver)
static int __init fs16xx_i2c_init(void)
{
	int ret = 0;
	FUNC_ENTER();
	ret = i2c_add_driver(&fs16xx_i2c_driver);
	FUNC_EXIT(ret);
	return ret;
}

static void __exit fs16xx_i2c_exit(void)
{
	FUNC_ENTER();
	i2c_del_driver(&fs16xx_i2c_driver);
}

module_init(fs16xx_i2c_init);
module_exit(fs16xx_i2c_exit);
#else
module_i2c_driver(fs16xx_i2c_driver);
#endif

MODULE_DESCRIPTION("fs16xx Smart PA i2c driver");
MODULE_LICENSE("GPL");
