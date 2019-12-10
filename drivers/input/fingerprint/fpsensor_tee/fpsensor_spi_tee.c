#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <net/sock.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#include <teei_fp.h>
#include <tee_client_api.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/workqueue.h>

#include <linux/irq.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#ifdef CONFIG_PM_WAKELOCKS
#include <linux/pm_wakeup.h>
#else
#include <linux/wakelock.h>
#endif

#if defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

/* MTK header */
//#include "mach/gpio_const.h"

#include "fpsensor_spi_tee.h"
//#include "mach/mt_clkmgr.h" //kernel 3.18
extern void mt_spi_disable_master_clk(struct spi_device *ms);
extern void mt_spi_enable_master_clk(struct spi_device *spidev);
struct spi_device *g_fpsensor_spidev = NULL;
#define FPSENSOR_SPI_VERSION              "fpsensor_spi_tee_mtk_v1.23.1"
#define FP_NOTIFY                         1

//begin, add fpvendor for chipone by song.li
#include "ontim/ontim_dev_dgb.h"
#define FPC_HW_INFO "ICNF7332"
DEV_ATTR_DECLARE(fingersensor)
DEV_ATTR_DEFINE("vendor", FPC_HW_INFO)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(fingersensor, fingersensor, 8);
//end


/* global variables */
static fpsensor_data_t *g_fpsensor = NULL;
uint32_t g_cmd_sn = 0;

/* -------------------------------------------------------------------- */
/* fingerprint chip hardware configuration                              */
/* -------------------------------------------------------------------- */
static DEFINE_MUTEX(spidev_set_gpio_mutex);
static void spidev_gpio_as_int(fpsensor_data_t *fpsensor)
{
    fpsensor_debug(DEBUG_LOG, "[fpsensor]----%s---\n", __func__);
    mutex_lock(&spidev_set_gpio_mutex);
    fpsensor_debug(DEBUG_LOG,"[fpsensor]spidev_gpio_as_int\n");
    pinctrl_select_state(fpsensor->pinctrl1, fpsensor->eint_as_int);
    mutex_unlock(&spidev_set_gpio_mutex);
}
static int fpsensor_irq_gpio_cfg(fpsensor_data_t *fpsensor)
{
    struct device_node *node;
    //fpsensor_data_t *fpsensor;
    u32 ints[2] = {0, 0};
    fpsensor_debug(DEBUG_LOG,"%s\n", __func__);

    //fpsensor = g_fpsensor;

    spidev_gpio_as_int(fpsensor);

    node = of_find_compatible_node(NULL, NULL, "mediatek,fpsensor_pinctrl");
    if ( node) {
        of_property_read_u32_array( node, "debounce", ints, ARRAY_SIZE(ints));
        // gpio_request(ints[0], "fpsensor-irq");
        // gpio_set_debounce(ints[0], ints[1]);
        fpsensor_debug(DEBUG_LOG,"[fpsensor]ints[0] = %d,is irq_gpio , ints[1] = %d!!\n", ints[0], ints[1]);
        fpsensor->irq_gpio = ints[0];
        fpsensor->irq = irq_of_parse_and_map(node, 0);  // get irq number
        if (!fpsensor->irq) {
            fpsensor_debug(ERR_LOG,"fpsensor irq_of_parse_and_map fail!!\n");
            return -EINVAL;
        }
        fpsensor_debug(DEBUG_LOG," [fpsensor]fpsensor->irq= %d,fpsensor>irq_gpio = %d\n", fpsensor->irq,
                       fpsensor->irq_gpio);
    } else {
        fpsensor_debug(ERR_LOG,"fpsensor null irq node!!\n");
        return -EINVAL;
    }

    return 0 ;
}

void fpsensor_gpio_output_dts(int gpio, int level)
{
    mutex_lock(&spidev_set_gpio_mutex);
    fpsensor_debug(DEBUG_LOG, "[fpsensor]fpsensor_gpio_output_dts: gpio= %d, level = %d\n", gpio, level);
    if (gpio == FPSENSOR_RST_PIN) {
        if (level) {
            pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_rst_high);
        } else {
            pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_rst_low);
        }
    } else if (gpio == FPSENSOR_SPI_CS_PIN) {
	if(level)
	{
             pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_cs_high);
	}
	else
	{
    	    pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_cs_low);
	}
    } else if (gpio == FPSENSOR_SPI_MO_PIN) {
        pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_mo_low);
    } else if (gpio == FPSENSOR_SPI_CK_PIN) {
        pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_ck_low);
    } else if (gpio == FPSENSOR_SPI_MI_PIN) {
        pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_mi_low);
    }
    mutex_unlock(&spidev_set_gpio_mutex);
}
int fpsensor_gpio_wirte(int gpio, int value)
{
    fpsensor_gpio_output_dts(gpio, value);
    return 0;
}
int fpsensor_gpio_read(int gpio)
{
    return gpio_get_value(gpio);
}

int fpsensor_spidev_dts_init(fpsensor_data_t *fpsensor)
{
    struct device_node *node = NULL;
    struct platform_device *pdev = NULL;
    int ret = 0;
    fpsensor_debug(DEBUG_LOG, "%s\n", __func__);

    node = of_find_compatible_node(NULL, NULL, "mediatek,fpsensor_pinctrl");
    if (node) {
        pdev = of_find_device_by_node(node);
        if(pdev) {
            fpsensor->pinctrl1 = devm_pinctrl_get(&pdev->dev);
            if (IS_ERR(fpsensor->pinctrl1)) {
                ret = PTR_ERR(fpsensor->pinctrl1);
                fpsensor_debug(ERR_LOG,"fpsensor Cannot find fp pinctrl1.\n");
                return ret;
            }
        } else {
            fpsensor_debug(ERR_LOG,"fpsensor Cannot find device.\n");
            return -ENODEV;
        }
        fpsensor->eint_as_int = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_eint_as_int");
        if (IS_ERR(fpsensor->eint_as_int)) {
            ret = PTR_ERR(fpsensor->eint_as_int);
            fpsensor_debug(ERR_LOG, "fpsensor Cannot find fp pinctrl eint_as_int!\n");
            return ret;
        }
        fpsensor->fp_rst_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_finger_rst_low");
        if (IS_ERR(fpsensor->fp_rst_low)) {
            ret = PTR_ERR(fpsensor->fp_rst_low);
            fpsensor_debug(ERR_LOG,"fpsensor Cannot find fp pinctrl fp_rst_low!\n");
            return ret;
        }
        fpsensor->fp_rst_high = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_finger_rst_high");
        if (IS_ERR(fpsensor->fp_rst_high)) {
            ret = PTR_ERR(fpsensor->fp_rst_high);
            fpsensor_debug(ERR_LOG, "fpsensor Cannot find fp pinctrl fp_rst_high!\n");
            return ret;
        }

        fpsensor->fp_cs_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_cs_low");
        if (IS_ERR(fpsensor->fp_cs_low)) {
            ret = PTR_ERR(fpsensor->fp_cs_low);
            fpsensor_debug(ERR_LOG,"fpsensor Cannot find fp pinctrl fp_cs_low!\n");
            return ret;
        }

        fpsensor->fp_cs_high = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_cs_high");
        if (IS_ERR(fpsensor->fp_cs_high)) {
            ret = PTR_ERR(fpsensor->fp_cs_high);
            fpsensor_debug(ERR_LOG,"fpsensor Cannot find fp pinctrl fp_cs_high!\n");
            return ret;
        }

        fpsensor->fp_mo_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_mo_low");
        if (IS_ERR(fpsensor->fp_mo_low)) {
            ret = PTR_ERR(fpsensor->fp_mo_low);
            fpsensor_debug(ERR_LOG,"fpsensor Cannot find fp pinctrl fp_mo_low!\n");
            return ret;
        }

        fpsensor->fp_mi_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_mi_low");
        if (IS_ERR(fpsensor->fp_mi_low)) {
            ret = PTR_ERR(fpsensor->fp_mi_low);
            fpsensor_debug(ERR_LOG,"fpsensor Cannot find fp pinctrl fp_mi_low!\n");
            return ret;
        }

        fpsensor->fp_ck_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_mclk_low");
        if (IS_ERR(fpsensor->fp_ck_low)) {
            ret = PTR_ERR(fpsensor->fp_ck_low);
            fpsensor_debug(ERR_LOG,"fpsensor Cannot find fp pinctrl fp_ck_low!\n");
            return ret;
        }

        fpsensor_gpio_output_dts(FPSENSOR_SPI_MO_PIN, 0);
        fpsensor_gpio_output_dts(FPSENSOR_SPI_MI_PIN, 0);
        fpsensor_gpio_output_dts(FPSENSOR_SPI_CK_PIN, 0);
        fpsensor_gpio_output_dts(FPSENSOR_SPI_CS_PIN, 1);
        fpsensor_gpio_wirte(FPSENSOR_RST_PIN, 1);
    } else {
        fpsensor_debug(ERR_LOG,"fpsensor Cannot find node!\n");
        return -ENODEV;
    }
    return 0;
}
/* delay us after reset */
static void fpsensor_hw_reset(int delay)
{
    FUNC_ENTRY();

    fpsensor_gpio_wirte(FPSENSOR_RST_PIN,    1);
    udelay(100);
    fpsensor_gpio_wirte(FPSENSOR_RST_PIN,  0);
    udelay(1000);
    fpsensor_gpio_wirte(FPSENSOR_RST_PIN,  1);
    if (delay) {
        /* delay is configurable */
        udelay(delay);
    }

    FUNC_EXIT();
    return;
}
void fpsensor_spi_clk_enable(u8 bonoff)
{
     static int mt_spi_clk_flag = 0;   // 0 mean clock is diasble
     if(mt_spi_clk_flag == bonoff)
     {
	return;
     }
#if defined(USE_SPI_BUS)
    if (bonoff == 0) {
	mt_spi_clk_flag = 0;
        mt_spi_disable_master_clk(g_fpsensor_spidev);
    } else {
	mt_spi_clk_flag = 1;
        mt_spi_enable_master_clk(g_fpsensor_spidev);
    }
#elif defined(USE_PLATFORM_BUS)
    if (bonoff == 0) {
        //enable_clock(MT_CG_PERI_SPI0, "spi");
    } else {
        //disable_clock(MT_CG_PERI_SPI0, "spi");
    }
#endif
}
EXPORT_SYMBOL_GPL(fpsensor_spi_clk_enable);

static void setRcvIRQ(int val)
{
    fpsensor_data_t *fpsensor_dev = g_fpsensor;
    fpsensor_dev->RcvIRQ = val;
}

static void fpsensor_enable_irq(fpsensor_data_t *fpsensor_dev)
{
    FUNC_ENTRY();
    setRcvIRQ(0);
    /* Request that the interrupt should be wakeable */
    if (fpsensor_dev->irq_enabled == 0) {
        enable_irq(fpsensor_dev->irq);
        fpsensor_dev->irq_enabled = 1;
    }
    FUNC_EXIT();
    return;
}

static void fpsensor_disable_irq(fpsensor_data_t *fpsensor_dev)
{
    FUNC_ENTRY();

    if (0 == fpsensor_dev->device_available) {
        fpsensor_debug(ERR_LOG, "%s, devices not available\n", __func__);
        goto out;
    }

    if (0 == fpsensor_dev->irq_enabled) {
        fpsensor_debug(ERR_LOG, "%s, irq already disabled\n", __func__);
        goto out;
    }

    if (fpsensor_dev->irq) {
        disable_irq_nosync(fpsensor_dev->irq);
        fpsensor_debug(DEBUG_LOG, "%s disable interrupt!\n", __func__);
    }
    fpsensor_dev->irq_enabled = 0;

out:
    setRcvIRQ(0);
    FUNC_EXIT();
    return;
}

static irqreturn_t fpsensor_irq2(int irq, void *handle)
{
	return  IRQ_HANDLED;
}
static irqreturn_t fpsensor_irq(int irq, void *handle)
{
    fpsensor_data_t *fpsensor_dev = (fpsensor_data_t *)handle;

    /* Make sure 'wakeup_enabled' is updated before using it
    ** since this is interrupt context (other thread...) */
    smp_rmb();
#ifdef CONFIG_PM_WAKELOCKS
	__pm_wakeup_event(&fpsensor_dev->ttw_wl, jiffies_to_msecs(1*HZ));
#else
	wake_lock_timeout(&fpsensor_dev->ttw_wl, msecs_to_jiffies(1000)); // 2 seconds.
#endif


    setRcvIRQ(1);
    wake_up_interruptible(&fpsensor_dev->wq_irq_return);

    return IRQ_HANDLED;
}

// release and cleanup fpsensor char device
static void fpsensor_dev_cleanup(fpsensor_data_t *fpsensor)
{
    FUNC_ENTRY();

    cdev_del(&fpsensor->cdev);
    unregister_chrdev_region(fpsensor->devno, FPSENSOR_NR_DEVS);
    device_destroy(fpsensor->class, fpsensor->devno);
    class_destroy(fpsensor->class);

    FUNC_EXIT();
}

static long fpsensor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct TEEC_UUID vendor_uuid = { 0x7778c03f, 0xc30c, 0x4dd0, { 0xa3, 0x19, 0xea, 0x29, 0x64, 0x3d, 0x4d, 0xc0 }};
    fpsensor_data_t *fpsensor_dev = NULL;
    int retval = 0;
    uint32_t val = 0;
    int irqf;

    fpsensor_debug(INFO_LOG, "[rickon]: fpsensor ioctl cmd : 0x%x \n", cmd );
    fpsensor_dev = (fpsensor_data_t *)filp->private_data;
    fpsensor_dev->cancel = 0 ;
    switch (cmd) {
    case FPSENSOR_IOC_INIT:

        fpsensor_debug(INFO_LOG, "%s: fpsensor init started======\n", __func__);
        retval = fpsensor_spidev_dts_init(fpsensor_dev);
        if (retval) {
            break;
        }
        if(fpsensor_irq_gpio_cfg(fpsensor_dev) != 0) {
            break;
        }
        //regist irq
        irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
        retval = request_threaded_irq(fpsensor_dev->irq, fpsensor_irq, NULL,
                                      irqf, FPSENSOR_DEV_NAME, fpsensor_dev);
        if (retval == 0) {
            fpsensor_debug(INFO_LOG, " irq thread reqquest success!\n");
        } else {
            fpsensor_debug(ERR_LOG, " irq thread request failed , retval =%d \n", retval);
            break;
        }
        enable_irq_wake(g_fpsensor->irq);
        fpsensor_dev->device_available = 1;
        // fix Unbalanced enable for IRQ, disable irq at first
        fpsensor_dev->irq_enabled = 1;
        fpsensor_disable_irq(fpsensor_dev);
        fpsensor_debug(INFO_LOG, "%s: fpsensor init finished======\n", __func__);

		memcpy(&uuid_fp, &vendor_uuid, sizeof(struct TEEC_UUID));
        break;

    case FPSENSOR_IOC_EXIT:
        fpsensor_disable_irq(fpsensor_dev);
        if (fpsensor_dev->irq) {
            free_irq(fpsensor_dev->irq, fpsensor_dev);
            fpsensor_dev->irq_enabled = 0;
        }
        fpsensor_dev->device_available = 0;
        fpsensor_debug(INFO_LOG, "%s: fpsensor exit finished======\n", __func__);
        break;

    case FPSENSOR_IOC_RESET:
        fpsensor_debug(INFO_LOG, "%s: chip reset command\n", __func__);
        fpsensor_hw_reset(1250);
        break;

    case FPSENSOR_IOC_ENABLE_IRQ:
        fpsensor_debug(INFO_LOG, "%s: chip ENable IRQ command\n", __func__);
        fpsensor_enable_irq(fpsensor_dev);
        break;

    case FPSENSOR_IOC_DISABLE_IRQ:
        fpsensor_debug(INFO_LOG, "%s: chip disable IRQ command\n", __func__);
        fpsensor_disable_irq(fpsensor_dev);
        break;
    case FPSENSOR_IOC_GET_INT_VAL:
        val = gpio_get_value(fpsensor_dev->irq_gpio);
        if (copy_to_user((void __user *)arg, (void *)&val, sizeof(uint32_t))) {
            fpsensor_debug(ERR_LOG, "Failed to copy data to user\n");
            retval = -EFAULT;
            break;
        }
        retval = 0;
        break;
    case FPSENSOR_IOC_ENABLE_SPI_CLK:
        fpsensor_debug(INFO_LOG, "%s: ENABLE_SPI_CLK ======\n", __func__);
        fpsensor_spi_clk_enable(1);
        break;
    case FPSENSOR_IOC_DISABLE_SPI_CLK:
        fpsensor_debug(INFO_LOG, "%s: DISABLE_SPI_CLK ======\n", __func__);
        fpsensor_spi_clk_enable(0);
        break;
    case FPSENSOR_IOC_ENABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_ENABLE_POWER ======\n", __func__);
        break;
    case FPSENSOR_IOC_DISABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_DISABLE_POWER ======\n", __func__);
        break;
    case FPSENSOR_IOC_REMOVE:
        fpsensor_disable_irq(fpsensor_dev);
        if (fpsensor_dev->irq) {
            free_irq(fpsensor_dev->irq, fpsensor_dev);
            fpsensor_dev->irq_enabled = 0;
        }
#if  1
        irqf = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
		retval = request_threaded_irq(fpsensor_dev->irq, fpsensor_irq2, NULL,
				irqf, FPSENSOR_DEV_NAME, fpsensor_dev);
		if (retval == 0) {
			fpsensor_debug(ERR_LOG, " irq thread reqquest success!\n");
		} else {
			fpsensor_debug(ERR_LOG, " irq thread request failed , retval =%d \n", retval);
		}
		free_irq(fpsensor_dev->irq, fpsensor_dev);
#endif
        fpsensor_dev->device_available = 0;
        fpsensor_dev_cleanup(fpsensor_dev);
#if FP_NOTIFY
        fb_unregister_client(&fpsensor_dev->notifier);
#endif
        fpsensor_dev->free_flag = 1;
        fpsensor_debug(INFO_LOG, "%s remove finished\n", __func__);
        break;
    case FPSENSOR_IOC_CANCEL_WAIT:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR CANCEL WAIT\n", __func__);
        wake_up_interruptible(&fpsensor_dev->wq_irq_return);
        fpsensor_dev->cancel = 1;
        break;
#if FP_NOTIFY
    case FPSENSOR_IOC_GET_FP_STATUS :
        val = fpsensor_dev->fb_status;
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_GET_FP_STATUS  %d \n",__func__, fpsensor_dev->fb_status);
        if (copy_to_user((void __user *)arg, (void *)&val, sizeof(uint32_t))) {
            fpsensor_debug(ERR_LOG, "Failed to copy data to user\n");
            retval = -EFAULT;
            break;
        }
        retval = 0;
        break;
#endif
    case FPSENSOR_IOC_ENABLE_REPORT_BLANKON:
        if (copy_from_user(&val, (void __user *)arg, sizeof(uint32_t))) {
            retval = -EFAULT;
            break;
        }
        fpsensor_dev->enable_report_blankon = val;
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_ENABLE_REPORT_BLANKON: %d\n", __func__, val);
        break;
    case FPSENSOR_IOC_UPDATE_DRIVER_SN:
        if (copy_from_user(&g_cmd_sn, (void __user *)arg, sizeof(uint32_t))) {
            fpsensor_debug(ERR_LOG, "Failed to copy g_cmd_sn from user to kernel\n");
            retval = -EFAULT;
            break;
        }
        //fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_UPDATE_DRIVER_SN: %d\n", __func__, g_cmd_sn);
        break;
    case FPSENSOR_IOC_SET_HARDWARE_INFO:
		//add fpvendor for chipone by song.li
		fpsensor_debug(ERR_LOG, "add fpvendor for hwinfo\n");
		CHECK_THIS_DEV_DEBUG_AREADY_EXIT();
		REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
	break;
    default:
        fpsensor_debug(ERR_LOG, "fpsensor doesn't support this command(0x%x)\n", cmd);
        break;
    }

    //FUNC_EXIT();
    return retval;
}

static long fpsensor_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return fpsensor_ioctl(filp, cmd, (unsigned long)(arg));
}

static unsigned int fpsensor_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int ret = 0;

    ret |= POLLIN;
    poll_wait(filp, &g_fpsensor->wq_irq_return, wait);
    if (g_fpsensor->cancel == 1) {
        fpsensor_debug(ERR_LOG, " cancle\n");
        ret =  POLLERR;
        g_fpsensor->cancel = 0;
        return ret;
    }

    if ( g_fpsensor->RcvIRQ) {
        if (g_fpsensor->RcvIRQ == 2) {
            fpsensor_debug(ERR_LOG, " get fp on notify\n");
            ret |= POLLHUP;
        } else {
            fpsensor_debug(ERR_LOG, " get irq\n");
            ret |= POLLRDNORM;
        }
    } else {
        ret = 0;
    }
    return ret;
}

static int fpsensor_open(struct inode *inode, struct file *filp)
{
    fpsensor_data_t *fpsensor_dev;

    FUNC_ENTRY();
    fpsensor_dev = container_of(inode->i_cdev, fpsensor_data_t, cdev);
    fpsensor_dev->users++;
    fpsensor_dev->device_available = 1;
    filp->private_data = fpsensor_dev;
    FUNC_EXIT();
    return 0;
}

static int fpsensor_release(struct inode *inode, struct file *filp)
{
    fpsensor_data_t *fpsensor_dev;
    int    status = 0;

    FUNC_ENTRY();
    fpsensor_dev = filp->private_data;
    filp->private_data = NULL;

    /*last close??*/
    fpsensor_dev->users--;
    if (fpsensor_dev->users <= 0) {
        fpsensor_debug(INFO_LOG, "%s, disble_irq. irq = %d\n", __func__, fpsensor_dev->irq);
        fpsensor_disable_irq(fpsensor_dev);
    }
    fpsensor_dev->device_available = 0;
    FUNC_EXIT();
    return status;
}

static ssize_t fpsensor_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    fpsensor_debug(ERR_LOG, "Not support read opertion in TEE version\n");
    return -EFAULT;
}

static ssize_t fpsensor_write(struct file *filp, const char __user *buf, size_t count,
                              loff_t *f_pos)
{
    fpsensor_debug(ERR_LOG, "Not support write opertion in TEE version\n");
    return -EFAULT;
}

static const struct file_operations fpsensor_fops = {
    .owner          = THIS_MODULE,
    .write          = fpsensor_write,
    .read           = fpsensor_read,
    .unlocked_ioctl = fpsensor_ioctl,
    .compat_ioctl   = fpsensor_compat_ioctl,
    .open           = fpsensor_open,
    .release        = fpsensor_release,
    .poll           = fpsensor_poll,

};

// create and register a char device for fpsensor
static int fpsensor_dev_setup(fpsensor_data_t *fpsensor)
{
    int ret = 0;
    dev_t dev_no = 0;
    struct device *dev = NULL;
    int fpsensor_dev_major = FPSENSOR_DEV_MAJOR;
    int fpsensor_dev_minor = 0;

    FUNC_ENTRY();

    if (fpsensor_dev_major) {
        dev_no = MKDEV(fpsensor_dev_major, fpsensor_dev_minor);
        ret = register_chrdev_region(dev_no, FPSENSOR_NR_DEVS, FPSENSOR_DEV_NAME);
    } else {
        ret = alloc_chrdev_region(&dev_no, fpsensor_dev_minor, FPSENSOR_NR_DEVS, FPSENSOR_DEV_NAME);
        fpsensor_dev_major = MAJOR(dev_no);
        fpsensor_dev_minor = MINOR(dev_no);
        fpsensor_debug(INFO_LOG, "fpsensor device major is %d, minor is %d\n",
                       fpsensor_dev_major, fpsensor_dev_minor);
    }

    if (ret < 0) {
        fpsensor_debug(ERR_LOG, "can not get device major number %d\n", fpsensor_dev_major);
        goto out;
    }

    cdev_init(&fpsensor->cdev, &fpsensor_fops);
    fpsensor->cdev.owner = THIS_MODULE;
    fpsensor->cdev.ops   = &fpsensor_fops;
    fpsensor->devno      = dev_no;
    ret = cdev_add(&fpsensor->cdev, dev_no, FPSENSOR_NR_DEVS);
    if (ret) {
        fpsensor_debug(ERR_LOG, "add char dev for fpsensor failed\n");
        goto release_region;
    }

    fpsensor->class = class_create(THIS_MODULE, FPSENSOR_CLASS_NAME);
    if (IS_ERR(fpsensor->class)) {
        fpsensor_debug(ERR_LOG, "create fpsensor class failed\n");
        ret = PTR_ERR(fpsensor->class);
        goto release_cdev;
    }

    dev = device_create(fpsensor->class, &fpsensor->spi->dev, dev_no, fpsensor, FPSENSOR_DEV_NAME);
    if (IS_ERR(dev)) {
        fpsensor_debug(ERR_LOG, "create device for fpsensor failed\n");
        ret = PTR_ERR(dev);
        goto release_class;
    }
    FUNC_EXIT();
    return ret;

release_class:
    class_destroy(fpsensor->class);
    fpsensor->class = NULL;
release_cdev:
    cdev_del(&fpsensor->cdev);
release_region:
    unregister_chrdev_region(dev_no, FPSENSOR_NR_DEVS);
out:
    FUNC_EXIT();
    return ret;
}

#if FP_NOTIFY
static int fpsensor_fb_notifier_callback(struct notifier_block* self, unsigned long event, void* data)
{
    int retval = 0;
    static char screen_status[64] = { '\0' };
    struct fb_event* evdata = data;
    unsigned int blank;
    fpsensor_data_t *fpsensor_dev = g_fpsensor;

    fpsensor_debug(INFO_LOG,"%s enter.  event : 0x%x\n", __func__, (unsigned)event);
    if (event != FB_EVENT_BLANK /* FB_EARLY_EVENT_BLANK */) {
        return 0;
    }

    blank = *(int*)evdata->data;
    fpsensor_debug(INFO_LOG,"%s enter, blank=0x%x\n", __func__, blank);

    switch (blank) {
    case FB_BLANK_UNBLANK:
        fpsensor_debug(INFO_LOG,"%s: lcd on notify\n", __func__);
        sprintf(screen_status, "SCREEN_STATUS=%s", "ON");
        fpsensor_dev->fb_status = 1;
        if( fpsensor_dev->enable_report_blankon) {
            fpsensor_dev->RcvIRQ = 2;
            wake_up_interruptible(&fpsensor_dev->wq_irq_return);
        }
        break;

    case FB_BLANK_POWERDOWN:
        fpsensor_debug(INFO_LOG,"%s: lcd off notify\n", __func__);
        sprintf(screen_status, "SCREEN_STATUS=%s", "OFF");
        fpsensor_dev->fb_status = 0;
        break;

    default:
        fpsensor_debug(INFO_LOG,"%s: other notifier, ignore\n", __func__);
        break;
    }

    fpsensor_debug(INFO_LOG,"%s %s leave.\n", screen_status, __func__);
    return retval;
}
#endif
#if defined(USE_SPI_BUS)
static int fpsensor_probe(struct spi_device *spi)
#elif defined(USE_PLATFORM_BUS)
static int fpsensor_probe(struct platform_device *spi)
#endif
{
    int status = 0;
    fpsensor_data_t *fpsensor_dev = NULL;

    FUNC_ENTRY();

    /* Allocate driver data */
    fpsensor_dev = kzalloc(sizeof(*fpsensor_dev), GFP_KERNEL);
    if (!fpsensor_dev) {
        status = -ENOMEM;
        fpsensor_debug(ERR_LOG, "%s, Failed to alloc memory for fpsensor device.\n", __func__);
        goto out;
    }

    /* Initialize the driver data */
	g_fpsensor_spidev = spi;
    g_fpsensor = fpsensor_dev;
    fpsensor_dev->spi               = spi ;
    fpsensor_dev->device_available  = 0;
    fpsensor_dev->users             = 0;
    fpsensor_dev->irq               = 0;
    fpsensor_dev->irq_gpio          = 0;
    fpsensor_dev->irq_enabled       = 0;
    fpsensor_dev->free_flag         = 0;
    /* setup a char device for fpsensor */
    status = fpsensor_dev_setup(fpsensor_dev);
    if (status) {
        fpsensor_debug(ERR_LOG, "fpsensor setup char device failed, %d", status);
        goto release_drv_data;
    }
    fpsensor_spi_clk_enable(1);
	init_waitqueue_head(&fpsensor_dev->wq_irq_return);

#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&g_fpsensor->ttw_wl, "fpsensor_ttw_wl");
#else
	wake_lock_init(&g_fpsensor->ttw_wl, WAKE_LOCK_SUSPEND, "fpsensor_ttw_wl");
#endif


    fpsensor_dev->device_available = 1;
#if FP_NOTIFY
    fpsensor_dev->notifier.notifier_call = fpsensor_fb_notifier_callback;
    fb_register_client(&fpsensor_dev->notifier);
#endif

    fpsensor_debug(INFO_LOG, "%s finished, driver version: %s\n", __func__, FPSENSOR_SPI_VERSION);
    goto out;

release_drv_data:
    kfree(fpsensor_dev);
    fpsensor_dev = NULL;
out:
    FUNC_EXIT();
    return status;
}

#if defined(USE_SPI_BUS)
static int fpsensor_remove(struct spi_device *spi)
#elif defined(USE_PLATFORM_BUS)
static int fpsensor_remove(struct platform_device *spi)
#endif
{
    fpsensor_data_t *fpsensor_dev = g_fpsensor;

    FUNC_ENTRY();
    fpsensor_disable_irq(fpsensor_dev);
    if (fpsensor_dev->irq)
        free_irq(fpsensor_dev->irq, fpsensor_dev);
#if FP_NOTIFY
    fb_unregister_client(&fpsensor_dev->notifier);
#endif
    fpsensor_dev_cleanup(fpsensor_dev);

#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_trash(&fpsensor_dev->ttw_wl);
#else
	wake_lock_destroy(&fpsensor_dev->ttw_wl);
#endif
    kfree(fpsensor_dev);
    g_fpsensor = NULL;

    FUNC_EXIT();
    return 0;
}
#if 0
static int fpsensor_suspend(struct device *dev, pm_message_t state)
{
    return 0;
}

static int fpsensor_resume( struct device *dev)
{
    return 0;
}
#endif
#ifdef CONFIG_OF
static struct of_device_id fpsensor_of_match[] = {
    { .compatible = "mediatek,fpsensor", },
    {}
};
MODULE_DEVICE_TABLE(of, fpsensor_of_match);
#endif

#if defined(USE_SPI_BUS)
static struct spi_driver fpsensor_spi_driver = {
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        .bus = &spi_bus_type,
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = of_match_ptr(fpsensor_of_match),
#endif
    },
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
#if 0
    .suspend = fpsensor_suspend,
    .resume = fpsensor_resume,
#endif
};
#elif defined(USE_PLATFORM_BUS)
static struct platform_driver fpsensor_plat_driver = {
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        .bus    = &platform_bus_type,
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = of_match_ptr(fpsensor_of_match),
#endif
    },
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
#if 0
    .suspend = fpsensor_suspend,
    .resume = fpsensor_resume,
#endif
};
#endif

static int __init fpsensor_init(void)
{
    int status;
#if defined(USE_PLATFORM_BUS)
    status = platform_driver_register(&fpsensor_plat_driver);
#elif defined(USE_SPI_BUS)
    status = spi_register_driver(&fpsensor_spi_driver);
#endif
    if (status < 0) {
        fpsensor_debug(ERR_LOG, "%s, Failed to register TEE driver.\n", __func__);
    }

    return status;
}
module_init(fpsensor_init);

static void __exit fpsensor_exit(void)
{
#if defined(USE_PLATFORM_BUS)
    platform_driver_unregister(&fpsensor_plat_driver);
#elif defined(USE_SPI_BUS)
    spi_unregister_driver(&fpsensor_spi_driver);
#endif
}
module_exit(fpsensor_exit);

MODULE_AUTHOR("xhli");
MODULE_DESCRIPTION(" Fingerprint chip TEE driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:fpsensor-drivers");
