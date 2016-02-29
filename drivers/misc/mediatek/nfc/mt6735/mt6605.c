/***************************************************************************
 * Filename:
 * ---------
 *  mt6605.c
 *
 * Project:
 * --------
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 *  LiangChi Huang, ext 25609, LiangChi.Huang@mediatek.com, 2012-08-09
 *
 *****************************************************************************/

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt
/*****************************************************************************
 * Include
 *****************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* #include <mach/eint.h> */
/* #include <mach/mt6605.h> */

#ifndef CONFIG_MTK_FPGA
#include <mt_clkbuf_ctl.h>	/*  for clock buffer */
#endif

/* #include <cust_eint.h> */
/* #include <cust_i2c.h>  */

/*****************************************************************************
 * Define
 *****************************************************************************/

/* #define NFC_I2C_BUSNUM  I2C_NFC_CHANNEL */

#define I2C_ID_NAME "mt6605"

#define MAX_BUFFER_SIZE	255

#define NFC_CLIENT_TIMING 400	/* I2C speed */

enum {
	MTK_NFC_GPIO_EN_B = 0x0,
	MTK_NFC_GPIO_SYSRST_B,
	MTK_NFC_GPIO_EINT,
	MTK_NFC_GPIO_IRQ,
	MTK_NFC_GPIO_IOCTL,
	MTK_NFC_GPIO_MAX_NUM
};

enum {
	MTK_NFC_IOCTL_READ = 0x0,
	MTK_NFC_IOCTL_WRITE,
	MTK_NFC_IOCTL_MAX_NUM
};

enum {
	MTK_NFC_IOCTL_CMD_CLOCK_BUF_ENABLE = 0x0,
	MTK_NFC_IOCTL_CMD_CLOCK_BUF_DISABLE,
	MTK_NFC_IOCTL_CMD_EXIT_EINT,
	MTK_NFC_IOCTL_CMD_GET_CHIP_ID,
	MTK_NFC_IOCTL_CMD_READ_DATA,
	MTK_NFC_IOCTL_CMD_MAX_NUM
};

enum {
	MTK_NFC_PULL_LOW = 0x0,
	MTK_NFC_PULL_HIGH,
	MTK_NFC_PULL_INVALID,
};

enum {
	MTK_NFC_GPIO_DIR_IN = 0x0,
	MTK_NFC_GPIO_DIR_OUT,
	MTK_NFC_GPIO_DIR_INVALID,
};

/*****************************************************************************
 * Global Variable
 *****************************************************************************/
struct mt6605_dev *mt6605_dev_ptr = NULL;
struct mt6605_dev _gmt6605_dev;

#if !defined(CONFIG_MTK_LEGACY)
struct platform_device *nfc_plt_dev = NULL;
struct pinctrl *gpctrl = NULL;
struct pinctrl_state *st_ven_h = NULL;
struct pinctrl_state *st_ven_l = NULL;
struct pinctrl_state *st_rst_h = NULL;
struct pinctrl_state *st_rst_l = NULL;
struct pinctrl_state *st_eint_h = NULL;
struct pinctrl_state *st_eint_l = NULL;
struct pinctrl_state *st_irq_init = NULL;
#endif

/* static struct i2c_board_info nfc_board_info __initdata = */
/*    { I2C_BOARD_INFO(I2C_ID_NAME, I2C_NFC_SLAVE_7_BIT_ADDR) }; */

/* For DMA */
#ifdef CONFIG_MTK_I2C_EXTENSION
static char *I2CDMAWriteBuf;	/*= NULL;*//* unnecessary initialise */
static unsigned int I2CDMAWriteBuf_pa;	/* = NULL; */
static char *I2CDMAReadBuf;	/*= NULL;*//* unnecessary initialise */
static unsigned int I2CDMAReadBuf_pa;	/* = NULL; */
#else
static char I2CDMAWriteBuf[255];
static char I2CDMAReadBuf[255];
#endif
static int fgNfcChip;		/*= 0;*//* unnecessary initialise */
int forceExitBlockingRead = 0;

/*  NFC IRQ */
static u32 nfc_irq;
static int nfc_irq_count;	/*= 0;*//* unnecessary initialise */

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int mt6605_probe(struct i2c_client *client,
			const struct i2c_device_id *id);
static int mt6605_remove(struct i2c_client *client);
/* static int mt6605_detect(struct i2c_client *client, */
/*                                int kind, struct i2c_board_info *info); */
static int mt6605_dev_open(struct inode *inode, struct file *filp);
static long mt6605_dev_unlocked_ioctl(struct file *filp, unsigned int cmd,
				      unsigned long arg);
static ssize_t mt6605_dev_read(struct file *filp, char __user *buf,
			       size_t count, loff_t *offset);
static ssize_t mt6605_dev_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offset);

static int mt_nfc_probe(struct platform_device *pdev);
static int mt_nfc_remove(struct platform_device *pdev);

/* void mt6605_dev_irq_handler(void); */
static irqreturn_t mt6605_dev_irq_handler(int irq, void *data);	/*IRQ handler */

/* static void mt6605_disable_irq(struct mt6605_dev *mt6605_dev); */
#if !defined(CONFIG_MTK_LEGACY)
static int mt_nfc_pinctrl_init(struct platform_device *pdev);
static int mt_nfc_pinctrl_select(struct pinctrl *p, struct pinctrl_state *s);
static int mt_nfc_gpio_init(void);
#endif
static int mt_nfc_get_gpio_value(int gpio_num);
static int mt_nfc_get_gpio_dir(int gpio_num);

/*****************************************************************************
 * Data Structure
 *****************************************************************************/

struct mt6605_dev {
	wait_queue_head_t read_wq;
	struct mutex read_mutex;
	struct i2c_client *client;
	struct miscdevice mt6605_device;
	unsigned int ven_gpio;
	unsigned int sysrstb_gpio;
	unsigned int irq_gpio;	/* Chip inform Host */
	unsigned int eint_gpio;	/* Host inform Chip */
	bool irq_enabled;
	struct mutex irq_enabled_lock;
	/* spinlock_t            irq_enabled_lock; */
};

struct mt6605_i2c_platform_data {
	unsigned int irq_gpio;	/* Chip inform Host */
	unsigned int ven_gpio;
	unsigned int sysrstb_gpio;
	unsigned int eint_gpio;	/* Host inform Chip */
};

static const struct i2c_device_id mt6605_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id nfc_switch_of_match[] = {
{.compatible = "mediatek,nfc"},
	{},
};
#endif

/* #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)) */
/* static struct i2c_client_address_data addr_data = { .forces = forces,}; */
/* #endif */
static struct i2c_driver mt6605_dev_driver = {
	.id_table = mt6605_id,
	.probe = mt6605_probe,
	.remove = mt6605_remove,
	/* .detect               = mt6605_detect, */
	.driver = {
		   .name = "mt6605",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = nfc_switch_of_match,
#endif
		   },
};

/*  platform driver */
static const struct of_device_id nfc_dev_of_match[] = {
{.compatible = "mediatek,nfc-gpio-v2",},
	{},
};

static struct platform_driver mtk_nfc_platform_driver = {
	.probe = mt_nfc_probe,
	.remove = mt_nfc_remove,
	.driver = {
		   .name = I2C_ID_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = nfc_dev_of_match,
#endif
		   },
};

#if !defined(CONFIG_MTK_LEGACY)
struct mt6605_i2c_platform_data mt6605_platform_data;
#else
static struct mt6605_i2c_platform_data mt6605_platform_data = {
	.irq_gpio = GPIO_IRQ_NFC_PIN,
	.ven_gpio = GPIO_NFC_VENB_PIN,
	.sysrstb_gpio = GPIO_NFC_RST_PIN,
	.eint_gpio = GPIO_NFC_EINT_PIN,
};
#endif

static const struct file_operations mt6605_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = mt6605_dev_read,
	.write = mt6605_dev_write,
	.open = mt6605_dev_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mt6605_dev_unlocked_ioctl,
#endif
	.unlocked_ioctl = mt6605_dev_unlocked_ioctl,
};

/*****************************************************************************
 * Extern Area
 *****************************************************************************/

/*****************************************************************************
 * Function
 *****************************************************************************/
static void mt6605_enable_irq(u32 irq_line)
{
	/* pr_debug("%s : irq_line=%d, nfc_irq_count=%d.\n", __func__,
		 irq_line, nfc_irq_count); */

	nfc_irq_count++;	/* It must set before call enable_irq */

	enable_irq(irq_line);

	/* pr_debug("%s : nfc_irq_count = %d.\n", __func__, nfc_irq_count); */
}

static void mt6605_disable_irq(u32 irq_line)
{
	/* pr_debug("%s : irq_line=%d, nfc_irq_count=%d.\n", __func__,
		 irq_line, nfc_irq_count); */
	if (nfc_irq_count >= 1) {
		disable_irq_nosync(irq_line);
		nfc_irq_count--;
	} else {
		pr_debug("%s : disable irq fail.\n", __func__);
	}
	/* pr_debug("%s : nfc_irq_count = %d.\n", __func__, nfc_irq_count); */
}

static int mt6605_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = 0;
	struct mt6605_i2c_platform_data *platform_data;
	struct device_node *node;

	pr_debug("mt6605_dev_probe\n");

	platform_data = &mt6605_platform_data;

	if (platform_data == NULL) {
		pr_err("%s : nfc probe fail\n", __func__);
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return -ENODEV;
	}
#if defined(CONFIG_MTK_LEGACY)
	mt_set_gpio_mode(platform_data->irq_gpio, GPIO_IRQ_NFC_PIN_M_EINT);
	mt_set_gpio_dir(platform_data->irq_gpio, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(platform_data->irq_gpio, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(platform_data->irq_gpio, GPIO_PULL_DOWN);

	/* ven_gpio */
	mt_set_gpio_mode(platform_data->ven_gpio, GPIO_NFC_VENB_PIN_M_GPIO);
	mt_set_gpio_dir(platform_data->ven_gpio, GPIO_DIR_OUT);
	mt_set_gpio_out(platform_data->ven_gpio, GPIO_OUT_ONE);
	usleep_range(900, 1000);	/* debug */

	/* firm_gpio */
	mt_set_gpio_mode(platform_data->sysrstb_gpio, GPIO_NFC_RST_PIN_M_GPIO);
	mt_set_gpio_dir(platform_data->sysrstb_gpio, GPIO_DIR_OUT);
	mt_set_gpio_out(platform_data->sysrstb_gpio, GPIO_OUT_ONE);
	usleep_range(900, 1000);	/* debug */

	/* EINT_gpio */
	mt_set_gpio_mode(platform_data->eint_gpio, GPIO_NFC_EINT_PIN_M_GPIO);
	mt_set_gpio_dir(platform_data->eint_gpio, GPIO_DIR_OUT);
	/* Set output High */
	mt_set_gpio_out(platform_data->eint_gpio, GPIO_OUT_ZERO);

	_gmt6605_dev.irq_gpio = platform_data->irq_gpio;
	_gmt6605_dev.ven_gpio = platform_data->ven_gpio;
	_gmt6605_dev.sysrstb_gpio = platform_data->sysrstb_gpio;
	_gmt6605_dev.eint_gpio = platform_data->eint_gpio;
#endif

	_gmt6605_dev.client = client;
	/* init mutex and queues */
	init_waitqueue_head(&_gmt6605_dev.read_wq);
	mutex_init(&_gmt6605_dev.read_mutex);
	/* spin_lock_init(&mt6605_dev->irq_enabled_lock); */
	mutex_init(&_gmt6605_dev.irq_enabled_lock);

#if 0
	_gmt6605_dev.mt6605_device.minor = MISC_DYNAMIC_MINOR;
	_gmt6605_dev.mt6605_device.name = "mt6605";
	_gmt6605_dev.mt6605_device.fops = &mt6605_dev_fops;

	ret = misc_register(&_gmt6605_dev.mt6605_device);
	if (ret) {
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION
#ifdef CONFIG_64BIT
	I2CDMAWriteBuf =
	    (char *)dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAWriteBuf_pa,
				       GFP_KERNEL);
#else
	I2CDMAWriteBuf =
	    (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAWriteBuf_pa,
				       GFP_KERNEL);
#endif

	if (I2CDMAWriteBuf == NULL) {
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		mutex_destroy(&_gmt6605_dev.read_mutex);
		gpio_free(platform_data->sysrstb_gpio);
		return ret;
	}
#ifdef CONFIG_64BIT
	I2CDMAReadBuf =
	    (char *)dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAReadBuf_pa,
				       GFP_KERNEL);
#else
	I2CDMAReadBuf =
	    (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAReadBuf_pa,
				       GFP_KERNEL);
#endif

	if (I2CDMAReadBuf == NULL) {
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		mutex_destroy(&_gmt6605_dev.read_mutex);
		gpio_free(platform_data->sysrstb_gpio);
		return ret;
	}
	pr_debug("%s :I2CDMAWriteBuf_pa %d, I2CDMAReadBuf_pa,%d\n", __func__,
		 I2CDMAWriteBuf_pa, I2CDMAReadBuf_pa);
#else
	memset(I2CDMAWriteBuf, 0x00, sizeof(I2CDMAWriteBuf));
	memset(I2CDMAReadBuf, 0x00, sizeof(I2CDMAReadBuf));
#endif
	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */

	/* client->irq = CUST_EINT_IRQ_NFC_NUM; */
	/* pr_debug("%s : requesting IRQ %d\n", __func__, client->irq);  */
	/* pr_debug("mt6605_Prob2\n");     */
	/* mt_eint_set_hw_debounce(CUST_EINT_IRQ_NFC_NUM, */
	/*                              CUST_EINT_IRQ_NFC_DEBOUNCE_CN); */
	/* mt_eint_registration(CUST_EINT_IRQ_NFC_NUM,  */
	/*                   CUST_EINT_IRQ_NFC_TYPE, mt6605_dev_irq_handler, */
	/*                   0); */

	/*  NFC IRQ settings     */
	node = of_find_compatible_node(NULL, NULL, "mediatek,nfc-gpio-v2");
	if (node) {

		nfc_irq = irq_of_parse_and_map(node, 0);

		client->irq = nfc_irq;

		ret =
		    request_irq(nfc_irq, mt6605_dev_irq_handler,
				IRQF_TRIGGER_NONE, "IRQ_NFC-eint", NULL);

		if (ret) {

			pr_err("%s : EINT IRQ LINE NOT AVAILABLE\n", __func__);

		} else {

			pr_debug("%s : set EINT finished, nfc_irq=%d", __func__,
				 nfc_irq);
			nfc_irq_count++;
			mt6605_disable_irq(nfc_irq);
		}

	} else {
		pr_err("%s : can not find NFC eint compatible node\n",
		       __func__);
	}

	/* mt_eint_unmask(CUST_EINT_IRQ_NFC_NUM); */
	/* mt6605_disable_irq(mt6605_dev); */
	/* mt_eint_mask(CUST_EINT_IRQ_NFC_NUM); */

	i2c_set_clientdata(client, &_gmt6605_dev);

	forceExitBlockingRead = 0;

	return 0;
}

static int mt6605_remove(struct i2c_client *client)
{
	/* struct mt6605_dev *mt6605_dev; */

	pr_debug("mt6605_remove\n");

#ifdef CONFIG_MTK_I2C_EXTENSION
	if (I2CDMAWriteBuf) {
		#ifdef CONFIG_64BIT
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, I2CDMAWriteBuf,
				  I2CDMAWriteBuf_pa);
		#else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAWriteBuf,
				  I2CDMAWriteBuf_pa);
		#endif
		I2CDMAWriteBuf = NULL;
		I2CDMAWriteBuf_pa = 0;
	}

	if (I2CDMAReadBuf) {
		#ifdef CONFIG_64BIT
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, I2CDMAReadBuf,
				  I2CDMAReadBuf_pa);
		#else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAReadBuf,
				  I2CDMAReadBuf_pa);
		#endif
		I2CDMAReadBuf = NULL;
		I2CDMAReadBuf_pa = 0;
	}
#endif

	/* mt6605_dev = i2c_get_clientdata(client); */
	/* free_irq(client->irq, &_gmt6605_dev);  */
	free_irq(nfc_irq, &_gmt6605_dev);
	misc_deregister(&_gmt6605_dev.mt6605_device);
	mutex_destroy(&_gmt6605_dev.read_mutex);

#if defined(CONFIG_MTK_LEGACY)
	gpio_free(_gmt6605_dev.irq_gpio);
	gpio_free(_gmt6605_dev.ven_gpio);
	gpio_free(_gmt6605_dev.sysrstb_gpio);
	gpio_free(_gmt6605_dev.eint_gpio);
#endif

	/* kfree(mt6605_dev); */

	return 0;
}

static int mt_nfc_probe(struct platform_device *pdev)
{
	int ret = 0;

#if !defined(CONFIG_MTK_LEGACY)

	nfc_plt_dev = pdev;

	pr_debug("%s : &nfc_plt_dev=%p\n", __func__, nfc_plt_dev);

	/* pinctrl init */
	ret = mt_nfc_pinctrl_init(pdev);

	/* gpio init */
	if (mt_nfc_gpio_init() != 0)
		pr_debug("%s : mt_nfc_gpio_init err.\n", __func__);

#endif

	return 0;
}

static int mt_nfc_remove(struct platform_device *pdev)
{
	pr_debug("%s : &pdev=%p\n", __func__, pdev);
	return 0;
}

/* void mt6605_dev_irq_handler(void) */
irqreturn_t mt6605_dev_irq_handler(int irq, void *data)
{
	struct mt6605_dev *mt6605_dev = mt6605_dev_ptr;
	/* 2015/08/25 reduce print_log
	pr_debug("%s : &mt6605_dev=%p\n", __func__, mt6605_dev);
	*/

	if (NULL == mt6605_dev) {
		pr_debug("mt6605_dev NULL.\n");
		return IRQ_HANDLED;
	}
	/* mt6605_disable_irq(mt6605_dev); */
	mt6605_disable_irq(nfc_irq);

	wake_up(&mt6605_dev->read_wq);
	/* wake_up_interruptible(&mt6605_dev->read_wq); */

	/* pr_debug("%s : wake_up &read_wq=%p\n", __func__, &mt6605_dev->read_wq); */

	return IRQ_HANDLED;
}

static ssize_t mt6605_dev_read(struct file *filp, char __user *buf,
			       size_t count, loff_t *offset)
{
	struct mt6605_dev *mt6605_dev = filp->private_data;
	int ret = 0;
	int read_retry = 5;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (!mt_nfc_get_gpio_value(mt6605_dev->irq_gpio)) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			pr_debug("%s : goto fail.\n", __func__);
			goto fail;
		}
		/* mutex_lock(&mt6605_dev->irq_enabled_lock); */
		/* mt6605_dev->irq_enabled = true; */
		/* mutex_unlock(&mt6605_dev->irq_enabled_lock); */

		mutex_lock(&mt6605_dev->read_mutex);
		if (forceExitBlockingRead == 1) {
			pr_debug("%s :forceExitBlockingRead.\n", __func__);
			forceExitBlockingRead = 0;	/* clear flag */
			mutex_unlock(&mt6605_dev->read_mutex);
			goto fail;
		}
		mutex_unlock(&mt6605_dev->read_mutex);

		/* mt_eint_unmask(mt6605_dev->client->irq);     */
		/* pr_debug("%s : mt_eint_unmask %d, IRQ, %d\n", */
		/*                __func__,   */
		/*                mt6605_dev->client->irq, */
		/*                mt_get_gpio_in(mt6605_dev->irq_gpio)); */

		mt6605_enable_irq(nfc_irq);
/* 2015/06/11 reduce print
		pr_debug("%s : enable_irq %d, irq status=%d\n",
			 __func__,
			 nfc_irq, mt_nfc_get_gpio_value(mt6605_dev->irq_gpio));
*/
		ret = wait_event_interruptible(mt6605_dev->read_wq,
					       (mt_nfc_get_gpio_value
						(mt6605_dev->irq_gpio)
						|| forceExitBlockingRead));
/* 2015/06/11 reduce print
		pr_debug("%s : wait_event_interruptible ret=%d,irq status=%d\n",
			 __func__, ret,
			 mt_nfc_get_gpio_value(mt6605_dev->irq_gpio));
*/
		if (ret || forceExitBlockingRead) {
			mt6605_disable_irq(nfc_irq);
			pr_debug("%s : goto fail\n", __func__);
			mutex_lock(&mt6605_dev->read_mutex);
			if (forceExitBlockingRead == 1) {
				pr_debug
				    ("%s:clear flag,orceExitBlockingRead\n",
				     __func__);
				forceExitBlockingRead = 0;	/* clear flag */
			}
			mutex_unlock(&mt6605_dev->read_mutex);
			goto fail;
		}

	}
	#ifdef CONFIG_MTK_I2C_EXTENSION
	mt6605_dev->client->addr = (mt6605_dev->client->addr & I2C_MASK_FLAG);
	mt6605_dev->client->ext_flag |= I2C_DMA_FLAG;
	/* mt6605_dev->client->ext_flag |= I2C_DIRECTION_FLAG; */
	/* mt6605_dev->client->ext_flag |= I2C_A_FILTER_MSG; */
	mt6605_dev->client->timing = NFC_CLIENT_TIMING;

	/* Read data */
	ret =
	    i2c_master_recv(mt6605_dev->client,
			    (unsigned char *)(uintptr_t) I2CDMAReadBuf_pa,
			    count);

	#else
	while (read_retry) {
		ret =
		    i2c_master_recv(mt6605_dev->client,
				    (unsigned char *)(uintptr_t) I2CDMAReadBuf,
				    count);

		/* mutex_unlock(&mt6605_dev->read_mutex); */

		/*pr_debug("%s : i2c_master_recv returned=%d, irq status=%d\n", __func__,
			 ret, mt_nfc_get_gpio_value(mt6605_dev->irq_gpio));*/

		if (ret < 0) {
			pr_debug("%s: i2c_master_recv failed: %d, read_retry: %d\n",
				__func__, ret, read_retry);
			read_retry--;
			usleep_range(900, 1000);
			continue;
		}
		break;
	}
	#endif
	if (ret < 0) {
		pr_err("%s: i2c_master_recv failed: %d, read_retry: %d\n",
			__func__, ret, read_retry);
		return ret;
	}

	if (ret > count) {
		pr_debug("%s: received too many bytes from i2c (%d)\n",
			 __func__, ret);
		return -EIO;
	}

	if (copy_to_user(buf, I2CDMAReadBuf, ret)) {
		pr_debug("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}

	/* pr_debug("%s: return,ret,%d\n", __func__, ret); */

	return ret;

fail:
	/* mutex_unlock(&mt6605_dev->read_mutex); */
	pr_debug("%s: return,fail,%d\n", __func__, ret);
	return ret;

}

static ssize_t mt6605_dev_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offset)
{
	struct mt6605_dev *mt6605_dev;
	int ret = 0, ret_tmp = 0, count_ori = 0, count_remain = 0, idx = 0;

	mt6605_dev = filp->private_data;
	count_ori = count;
	count_remain = count_ori;

	if (count > MAX_BUFFER_SIZE) {
		count = MAX_BUFFER_SIZE;
		count_remain -= count;
	}

	while (1) {
		if (copy_from_user
		    (I2CDMAWriteBuf, &buf[(idx * MAX_BUFFER_SIZE)], count)) {
			pr_debug("%s : failed to copy from user space.\n",
				 __func__);
			return -EFAULT;
		}
/* 2015/06/11 reduce print_log
		pr_debug("%s : writing %zu bytes, remain bytes %d.\n", __func__,
			 count, count_remain);
*/
		/* Write data */
		#ifdef CONFIG_MTK_I2C_EXTENSION
		mt6605_dev->client->addr =
		    (mt6605_dev->client->addr & I2C_MASK_FLAG);

		mt6605_dev->client->ext_flag |= I2C_DMA_FLAG;
		/* mt6605_dev->client->ext_flag |= I2C_DIRECTION_FLAG; */
		/* mt6605_dev->client->ext_flag |= I2C_A_FILTER_MSG; */
		mt6605_dev->client->timing = NFC_CLIENT_TIMING;

		ret_tmp =
		    i2c_master_send(mt6605_dev->client,
				    (unsigned char *)(uintptr_t)
				    I2CDMAWriteBuf_pa, count);
		#else
		ret_tmp =
		    i2c_master_send(mt6605_dev->client,
				    (unsigned char *)(uintptr_t)
				    I2CDMAWriteBuf, count);
		#endif

		if (ret_tmp != count) {
			pr_debug("%s : i2c_master_send returned %d\n", __func__,
				 ret);
			ret = -EIO;
			return ret;
		}

		ret += ret_tmp;
		/* pr_debug("%s : ret_tmp=%d,ret=%d,count_ori=%d\n", __func__,
		   ret_tmp, ret, count_ori); */

		if (ret == count_ori) {
			/* pr_debug("%s : ret == count_ori\n", __func__); */
			break;
		} else {
			if (count_remain > MAX_BUFFER_SIZE) {
				count = MAX_BUFFER_SIZE;
				count_remain -= MAX_BUFFER_SIZE;
			} else {
				count = count_remain;
				count_remain = 0;
			}
			idx++;
		}
	}
/* 2015/06/11 reduce print_log
	pr_debug("%s : writing %d bytes. Status %d\n", __func__, count_ori,
		 ret);
*/
	return ret;
}

static int mt6605_dev_open(struct inode *inode, struct file *filp)
{

	struct mt6605_dev *mt6605_dev =
	    container_of(filp->private_data, struct mt6605_dev, mt6605_device);

	filp->private_data = mt6605_dev;
	mt6605_dev_ptr = mt6605_dev;

	pr_debug("mt6605_dev_open,%s : %d,%d, &mt6605_dev_open=%p\n", __func__,
		 imajor(inode), iminor(inode), mt6605_dev_ptr);

	forceExitBlockingRead = 0;
	return 0;
}

static long mt6605_dev_unlocked_ioctl(struct file *filp, unsigned int cmd,
				      unsigned long arg)
{

	struct mt6605_dev *mt6605_dev = filp->private_data;
	int result = 0;
	int gpio_dir, tmp_gpio;
	int gpio_num = 0;

	pr_debug("mt6605_dev_unlocked_ioctl: cmd=0x%04x,arg=0x%04lx.\n", cmd,
		 arg);

	if ((cmd & 0xFFFF) == 0xFE00) {

		mt6605_disable_irq(nfc_irq);

		pr_debug("mt6605_dev_unlocked_ioctl for disable_irq.\n");

		return 0;
	} else if ((cmd & 0xFFFF) == 0xFE01) {

		struct device_node *node;
		int ret;

#if !defined(CONFIG_MTK_LEGACY)
		mt_nfc_pinctrl_select(gpctrl, st_irq_init);
#else
		mt_set_gpio_mode(mt6605_dev->irq_gpio, GPIO_IRQ_NFC_PIN_M_EINT);
		mt_set_gpio_dir(mt6605_dev->irq_gpio, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(mt6605_dev->irq_gpio, GPIO_PULL_ENABLE);
		mt_set_gpio_pull_select(mt6605_dev->irq_gpio, GPIO_PULL_DOWN);
#endif

		/* mt_eint_set_hw_debounce(CUST_EINT_IRQ_NFC_NUM,  */
		/*                      CUST_EINT_IRQ_NFC_DEBOUNCE_CN); */
		/* mt_eint_registration(CUST_EINT_IRQ_NFC_NUM, */
		/*                      CUST_EINT_IRQ_NFC_TYPE, */
		/*                   mt6605_dev_irq_handler, 0);  */
		/* mt_eint_mask(CUST_EINT_IRQ_NFC_NUM); */

		/*  NFC IRQ settings */
		node =
		    of_find_compatible_node(NULL, NULL, "mediatek,nfc-gpio-v2");

		if (node) {

			nfc_irq = irq_of_parse_and_map(node, 0);

			/* client->irq = nfc_irq; */

			ret =
			    request_irq(nfc_irq, mt6605_dev_irq_handler,
					IRQF_TRIGGER_NONE, "IRQ_NFC-eint",
					NULL);

			if (ret) {
				pr_err("%s : EINT IRQ LINE NOT AVAILABLE\n",
				       __func__);
			} else {
				pr_debug
				    ("%s : set EINT finished, nfc_irq=%d.\n",
				     __func__, nfc_irq);
				nfc_irq_count++;
				mt6605_disable_irq(nfc_irq);
			}
		} else {
			pr_err("%s : can not find NFC eint compatible node.\n",
			       __func__);
		}

		/* 2015/08/25 reduce print_log
		pr_debug("mt6605_dev_unlocked_ioctl,Re-registered IRQ.\n");
		*/
		return 0;
	} else if ((cmd & 0xFFFF) == 0xFEFF) {	/* EXIT EINT */
		mutex_lock(&mt6605_dev->read_mutex);
		forceExitBlockingRead = 1;
		mutex_unlock(&mt6605_dev->read_mutex);

		wake_up_interruptible(&mt6605_dev->read_wq);
		pr_debug("mt6605_dev_unlocked_ioctl,SW Release IRQ.\n");
		return 0;
	} else if ((cmd & 0xFFFF) == 0xFEFE) {	/* Get ChipID */
		return fgNfcChip;
	} else if ((cmd & 0xFFFF) == 0xFEFD) {
		fgNfcChip = (arg & 0xFFFF);
		return 0;
	}

	tmp_gpio = (((arg & 0xFF00) >> 8) & 0x00FF);

	if (tmp_gpio == MTK_NFC_GPIO_EN_B) {
		gpio_num = mt6605_dev->ven_gpio;

	} else if (tmp_gpio == MTK_NFC_GPIO_SYSRST_B) {
		gpio_num = mt6605_dev->sysrstb_gpio;

	} else if (tmp_gpio == MTK_NFC_GPIO_EINT) {
		gpio_num = mt6605_dev->eint_gpio;

	} else if (tmp_gpio == MTK_NFC_GPIO_IRQ) {
		gpio_num = mt6605_dev->irq_gpio;

	} else if (tmp_gpio == MTK_NFC_GPIO_IOCTL) {
		/*  IOCTL  */
		/* int command = (arg & 0x00FF); */
		switch (arg & 0x00FF) {
		case MTK_NFC_IOCTL_CMD_CLOCK_BUF_ENABLE:
			pr_debug("%s, enable clock buffer.\n", __func__);
#ifndef CONFIG_MTK_FPGA
			/* enable nfc clock buffer */
			clk_buf_ctrl(CLK_BUF_NFC, 1);
#endif
			break;
		case MTK_NFC_IOCTL_CMD_CLOCK_BUF_DISABLE:
			pr_debug("%s, disable clock buffer.\n", __func__);
#ifndef CONFIG_MTK_FPGA
			/* disable nfc clock buffer */
			clk_buf_ctrl(CLK_BUF_NFC, 0);
#endif
			break;
		case MTK_NFC_IOCTL_CMD_EXIT_EINT:
			pr_debug("mt6605_dev_unlocked_ioctl, EXIT EINT.\n");
			mutex_lock(&mt6605_dev->read_mutex);
			forceExitBlockingRead = 1;
			mutex_unlock(&mt6605_dev->read_mutex);
			wake_up_interruptible(&mt6605_dev->read_wq);
			pr_debug("mt6605_dev_unlocked_ioctl,SW Release IRQ\n");
			break;
		case MTK_NFC_IOCTL_CMD_GET_CHIP_ID:
			return fgNfcChip;
			break;
		case MTK_NFC_IOCTL_CMD_READ_DATA:
			pr_debug("Call mt6605_dev_irq_handler. irq=%d.\n",
				 mt_nfc_get_gpio_value(mt6605_dev->irq_gpio));
			if (mt_nfc_get_gpio_value(mt6605_dev->irq_gpio))
				mt6605_dev_irq_handler(nfc_irq, NULL);
			break;
		default:
			break;
		}
		return 0;
	} else {
		result = MTK_NFC_PULL_INVALID;
		pr_debug("%s, invalid ioctl.\n", __func__);
		return result;
	}

	if (cmd == MTK_NFC_IOCTL_READ) {
		result = mt_nfc_get_gpio_value(gpio_num);
		/*
		   if ((gpio_dir == GPIO_DIR_IN)
		   || (gpio_dir == GPIO_DIR_OUT)) {
		   result = mt_get_gpio_in(gpio_num);
		   } else {
		   result = MTK_NFC_PULL_INVALID;
		   } */

		pr_debug("%s : get gpio value. %d\n", __func__, result);

		/*error handler for eint_registration abnormal case */
		if (tmp_gpio == MTK_NFC_GPIO_IRQ && result == 0x01) {
			pr_debug
			    ("%s,irq=igh call mt6605_dev_irq_handler\n",
			     __func__);
			mt6605_dev_irq_handler(nfc_irq, NULL);
		}

	} else if (cmd == MTK_NFC_IOCTL_WRITE) {
		gpio_dir = mt_nfc_get_gpio_dir(gpio_num);
		pr_debug("test2 MTK_NFC_IOCTL_WRITE, gpio_dir %d, gpio_num %d, tmp_gpio %d\n",
		    gpio_dir, gpio_num, tmp_gpio);
		if (gpio_dir == MTK_NFC_GPIO_DIR_OUT) {
			int gpio_pol = (arg & 0x00FF);

			pr_debug("test1 MTK_NFC_GPIO_DIR_OUT\n");
			if (gpio_pol == MTK_NFC_PULL_LOW) {
#if !defined(CONFIG_MTK_LEGACY)
				switch (tmp_gpio) {
				case MTK_NFC_GPIO_EN_B:
					result =
					    mt_nfc_pinctrl_select(gpctrl,
								  st_ven_l);
					break;

				case MTK_NFC_GPIO_SYSRST_B:
					result =
					    mt_nfc_pinctrl_select(gpctrl,
								  st_rst_l);
					break;

				case MTK_NFC_GPIO_EINT:
					result =
					    mt_nfc_pinctrl_select(gpctrl,
								  st_eint_l);
					break;

				case MTK_NFC_GPIO_IRQ:
					pr_debug
					    ("%s : IRQ cannot set.\n",
					     __func__);
					break;
				default:
					pr_debug("%s : default case.\n",
						 __func__);
					break;
				}
#else
				result =
				    mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);
				pr_debug("%s : call mtk legacy.\n", __func__);
#endif
			} else if (gpio_pol == MTK_NFC_PULL_HIGH) {
#if !defined(CONFIG_MTK_LEGACY)
				switch (tmp_gpio) {
				case MTK_NFC_GPIO_EN_B:
					result =
					    mt_nfc_pinctrl_select(gpctrl,
								  st_ven_h);
					break;

				case MTK_NFC_GPIO_SYSRST_B:
					result =
					    mt_nfc_pinctrl_select(gpctrl,
								  st_rst_h);
					break;

				case MTK_NFC_GPIO_EINT:
					result =
					    mt_nfc_pinctrl_select(gpctrl,
								  st_eint_h);
					break;

				case MTK_NFC_GPIO_IRQ:
					pr_debug
					    ("%s : IRQ cannot pull.\n",
					     __func__);
					break;
				default:
					pr_debug("%s : default case.\n",
						 __func__);
					break;
				}
#else
				result =
				    mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
				pr_debug("%s : call mtk legacy.\n", __func__);
#endif
			}
		} else {
			result = MTK_NFC_PULL_INVALID;
			pr_debug("test3\n");
		}
	} else {
		result = MTK_NFC_PULL_INVALID;
		pr_debug("test4\n");
	}

	pr_debug("mt6605_dev_unlocked_ioctl : result=%d\n", result);

	return result;
}

#if !defined(CONFIG_MTK_LEGACY)
static int mt_nfc_pinctrl_select(struct pinctrl *p, struct pinctrl_state *s)
{
	int ret = 0;

	if (p != NULL && s != NULL) {
		ret = pinctrl_select_state(p, s);
	} else {
		/* pr_debug("%s : pinctrl_select err\n", __func__); */
		ret = -1;
	}
	return ret;
}

static int mt_nfc_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	gpctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(gpctrl)) {
		dev_err(&pdev->dev, "Cannot find pinctrl!");
		ret = PTR_ERR(gpctrl);
		goto end;
	}

	st_ven_h = pinctrl_lookup_state(gpctrl, "ven_high");
	if (IS_ERR(st_ven_h)) {
		ret = PTR_ERR(st_ven_h);
		pr_debug("%s : pinctrl err, ven_high\n", __func__);
		goto end;
	}

	st_ven_l = pinctrl_lookup_state(gpctrl, "ven_low");
	if (IS_ERR(st_ven_l)) {
		ret = PTR_ERR(st_ven_l);
		pr_debug("%s : pinctrl err, ven_low\n", __func__);
		goto end;
	}

	st_rst_h = pinctrl_lookup_state(gpctrl, "rst_high");
	if (IS_ERR(st_rst_h)) {
		ret = PTR_ERR(st_rst_h);
		pr_debug("%s : pinctrl err, rst_high\n", __func__);
		goto end;
	}

	st_rst_l = pinctrl_lookup_state(gpctrl, "rst_low");
	if (IS_ERR(st_rst_l)) {
		ret = PTR_ERR(st_rst_l);
		pr_debug("%s : pinctrl err, rst_low\n", __func__);
		goto end;
	}

	st_eint_h = pinctrl_lookup_state(gpctrl, "eint_high");
	if (IS_ERR(st_eint_h)) {
		ret = PTR_ERR(st_eint_h);
		pr_debug("%s : pinctrl err, eint_high\n", __func__);
		goto end;
	}

	st_eint_l = pinctrl_lookup_state(gpctrl, "eint_low");
	if (IS_ERR(st_eint_l)) {
		ret = PTR_ERR(st_eint_l);
		pr_debug("%s : pinctrl err, eint_low\n", __func__);
		goto end;
	}

	st_irq_init = pinctrl_lookup_state(gpctrl, "irq_init");
	if (IS_ERR(st_irq_init)) {
		ret = PTR_ERR(st_irq_init);
		pr_debug("%s : pinctrl err, irq_init\n", __func__);
		goto end;
	}

	/* select state */
	ret = mt_nfc_pinctrl_select(gpctrl, st_irq_init);
	usleep_range(900, 1000);

	ret = mt_nfc_pinctrl_select(gpctrl, st_ven_h);
	usleep_range(900, 1000);

	ret = mt_nfc_pinctrl_select(gpctrl, st_rst_h);
	usleep_range(900, 1000);

	ret = mt_nfc_pinctrl_select(gpctrl, st_eint_l);

end:

	return ret;
}

static int mt_nfc_gpio_init(void)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,nfc-gpio-v2");
	if (node) {
		of_property_read_u32_array(node, "gpio-ven",
					   &(mt6605_platform_data.ven_gpio), 1);
		of_property_read_u32_array(node, "gpio-rst",
					   &(mt6605_platform_data.sysrstb_gpio),
					   1);
		of_property_read_u32_array(node, "gpio-eint",
					   &(mt6605_platform_data.eint_gpio),
					   1);
		of_property_read_u32_array(node, "gpio-irq",
					   &(mt6605_platform_data.irq_gpio), 1);
	} else {
		pr_debug("%s : get gpio num err.\n", __func__);
		return -1;
	}

	_gmt6605_dev.irq_gpio = mt6605_platform_data.irq_gpio;
	_gmt6605_dev.ven_gpio = mt6605_platform_data.ven_gpio;
	_gmt6605_dev.sysrstb_gpio = mt6605_platform_data.sysrstb_gpio;
	_gmt6605_dev.eint_gpio = mt6605_platform_data.eint_gpio;

	return 0;
}
#endif

static int mt_nfc_get_gpio_value(int gpio_num)
{
	int value = 0;

	if (mt_nfc_get_gpio_dir(gpio_num) != MTK_NFC_GPIO_DIR_INVALID) {
#if !defined(CONFIG_MTK_LEGACY)
		value = __gpio_get_value(gpio_num);
#else
		value = mt_get_gpio_in(gpio_num);
#endif
	}

	return value;
}

static int mt_nfc_get_gpio_dir(int gpio_num)
{
	if (gpio_num == mt6605_platform_data.irq_gpio) {
		return MTK_NFC_GPIO_DIR_IN;	/* input */

	} else if ((gpio_num == mt6605_platform_data.ven_gpio) ||
		   (gpio_num == mt6605_platform_data.sysrstb_gpio) ||
		   (gpio_num == mt6605_platform_data.eint_gpio)) {
		return MTK_NFC_GPIO_DIR_OUT;	/* output */

	} else {
		return MTK_NFC_GPIO_DIR_INVALID;

	}
}

/* return 0, success; return <0, fail
 * md_id        : modem id
 * md_state   : 0, on ; 1, off ;
 * vsim_state : 0, on ; 1, off;
 */
int inform_nfc_vsim_change(int md_id, int md_state, int vsim_state)
{
	char send_data[] = {
		0xaa, 0x0f, 0x03, 0x00, 0x03,
		0x00, 0x00, 0xaa, 0xf0
	};
	int ret = 0;
	int send_bytes = sizeof(send_data);
	int retry;

	/* AA 0F 03 00 03 00 xx AA F0   */
	/* where xx is defined:  */
	/* Bit[7:4] : md_id  */
	/* Bit[3:2] : md_state */
	/* Bit[1:0] : vsim_state */

	send_data[6] |= (md_id << 4);
	send_data[6] |= (md_state << 2);
	send_data[6] |= (vsim_state);

	pr_debug
	    ("%s, md_id,%d, md_state,%d, vsim_state,%d , send_data[6],0x%X.\n",
	     __func__, md_id, md_state, vsim_state, send_data[6]);

	/* send to mt6605 */
	_gmt6605_dev.client->addr = (_gmt6605_dev.client->addr & I2C_MASK_FLAG);
	_gmt6605_dev.client->ext_flag |= I2C_DMA_FLAG;
	_gmt6605_dev.client->timing = 400;
	memcpy(I2CDMAWriteBuf, send_data, send_bytes);

	/* eint pull high */
#if !defined(CONFIG_MTK_LEGACY)
	mt_nfc_pinctrl_select(gpctrl, st_eint_h);
#else
	ret = mt_set_gpio_out(mt6605_platform_data.eint_gpio, GPIO_OUT_ONE);
#endif

	/* sleep 5ms */
	/* msleep(5); */
	usleep_range(1800, 2000);

	for (retry = 0; retry < 10; retry++) {
		/* ret = i2c_master_send(_gmt6605_dev.client,  */
		/*      (unsigned char *)I2CDMAWriteBuf_pa, send_bytes); */
		ret =
		    i2c_master_send(_gmt6605_dev.client,
				    (unsigned char *)(uintptr_t)
				    I2CDMAWriteBuf_pa, send_bytes);
		pr_debug
			    ("%s, i2c_master_send return %d.\n",
			     __func__, ret);
		if (ret == send_bytes) {
			pr_debug
			    ("%s, send to mt6605 OK. retry %d.\n",
			     __func__, retry);
			break;
		} else {
			pr_debug
			    ("%s, send to mt6605 fail. retry %d, ret %d.\n",
			     __func__, retry, ret);
		}
		/* sleep 2ms */
		/* msleep(2); */
		usleep_range(1800, 2000);
	}
	/* eint pull low */

#if !defined(CONFIG_MTK_LEGACY)
	mt_nfc_pinctrl_select(gpctrl, st_eint_l);
#else
	ret = mt_set_gpio_out(mt6605_platform_data.eint_gpio, GPIO_OUT_ZERO);
#endif
	return 0;
}

/*
 * module load/unload record keeping
 */
static int __init mt6605_dev_init(void)
{
	int ret;

	pr_debug("mt6605_dev_init\n");
	/*  i2c_register_board_info(NFC_I2C_BUSNUM, &nfc_board_info, 1); */
	/*  pr_debug("mt6605_dev_init2\n"); */

	platform_driver_register(&mtk_nfc_platform_driver);

	i2c_add_driver(&mt6605_dev_driver);

	_gmt6605_dev.mt6605_device.minor = MISC_DYNAMIC_MINOR;
	_gmt6605_dev.mt6605_device.name = "mt6605";
	_gmt6605_dev.mt6605_device.fops = &mt6605_dev_fops;

	ret = misc_register(&_gmt6605_dev.mt6605_device);
	if (ret) {
		pr_debug("%s : misc_register failed\n", __FILE__);
		return ret;
	}

  /* 2015/08/25 reduce print_log
	pr_debug("mt6605_dev_init success\n");
	*/

	return 0;
}

static void __exit mt6605_dev_exit(void)
{
	pr_debug("mt6605_dev_exit\n");

	i2c_del_driver(&mt6605_dev_driver);
}

module_init(mt6605_dev_init);
module_exit(mt6605_dev_exit);

MODULE_AUTHOR("LiangChi Huang");
MODULE_DESCRIPTION("MTK NFC driver");
MODULE_LICENSE("GPL");
