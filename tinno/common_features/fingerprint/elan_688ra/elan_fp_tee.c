#include <linux/spi/spi.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/of_gpio.h>
#include <linux/sched.h>
#include <linux/pm_wakeup.h>

#ifdef CONFIG_MTK_CLKMGR
#include "mach/mt_clkmgr.h"
#else
#include <linux/clk.h>
#endif

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#endif
#include <linux/miscdevice.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <linux/notifier.h>
#include <linux/fb.h>

#include "elan_fp_tee.h"
#include <linux/completion.h>
#include "elan_fp_tee.h"
//#include <linux/platform_device.h>
#include "../fp_drv/fp_drv.h" 
#include <teei_fp.h>
#include <tee_client_api.h>

#define MTK_PLAFROM             (1)
#define USE_SPI1_4GB            (0) // Use for MTK DMA (spi1)
#define GPIO_PINCTRL            (1)
#define HV_IC                   (0)

#define VERSION_LOG	"ELAN FINGER PRINT V1.4.4.1"

#define _ELAN_DEBUG_
#ifdef _ELAN_DEBUG_
	static int elan_debug = 1;
	#define ELAN_DEBUG(format, args ...) \
			do { \
					if (elan_debug) \
							printk(KERN_ERR "[ELAN] " format, ##args); \
			} while (0)		 
#else
         #define ELAN_DEBUG(format, args ...)
#endif

#define GPIO_FP_ID	880
#define KEY_FP_INT			KEY_POWER //KEY_WAKEUP // change by customer & framework support
#define KEY_FP_INT2			KEY_1 // change by customer & framework support
#define SPI_MAX_SPEED		3*1000*1000
#define BANPOD_READ_ID                   (0)


//struct completion cmd_done_irq;
static int factory_status = 0;
static int irq_status = 0;
static int irq_status_buffer = 0;

static DEFINE_MUTEX(elan_factory_mutex);
static struct fasync_struct *fasync_queue = NULL;
static int key_status = 0;
//for lib version
#define MAX_LIB_BUF 256
static char lib_ver_buf[MAX_LIB_BUF] = "unknow";
static DECLARE_WAIT_QUEUE_HEAD(elan_poll_wq);
static int elan_work_flag = 0;
#if BANPOD_READ_ID
static unsigned int IMG_WIDTH = 0;
static unsigned int IMG_HEIGHT = 0;
//static unsigned int IMG_SIZE = 0;
static unsigned int spi_speed = SPI_MAX_SPEED; // 3MHz
#endif
// guomingyi add.
static int get_finger_state(char *buf, void *args); 
static int read_chip_id(char *buf, void *args);

extern int get_fp_vendor(void);
//extern void set_fp_vendor(uint8_t fp_vendor_id);

enum {
    FP_VENDOR_INVALID = 0,
    FPC_VENDOR,
    ELAN_VENDOR,
    GOODIX_VENDOR,
};

struct efsa120s_data  {
	int 					irq_gpio;
	int						isr;
	int 					rst_gpio;
	int						irq_is_disable;
	struct miscdevice		efsa120_dev;	/* char device for ioctl */
	struct spi_device	*pdev;
	//struct platform_device	*pdev;
	struct input_dev		*input_dev;
	spinlock_t				irq_lock;
	wait_queue_head_t		efsa_wait;
	struct wakeup_source		wake_lock;
	struct wakeup_source        hal_wake_lock;
	struct regulator *reg;
	struct pinctrl *pinctrl1;
	struct pinctrl_state *pins_default;
#if 0
	struct pinctrl_state *eint_as_int, *eint_in_low, *eint_in_float, *fp_rst_low, *fp_rst_high,*pins_miso_spi,*pins_miso_pullhigh,*pins_miso_pulllow;
#else
	struct pinctrl_state *eint_as_int, *eint_in_low, *eint_in_float, *fp_rst_low, *fp_rst_high;
	struct pinctrl_state *fp_enable_low, *fp_enable_high;
#endif
	struct notifier_block notifier;
//TINNO BEGIN
//Add spi clk by yinglong.tang
#if !defined(CONFIG_MTK_CLKMGR)
	struct clk *clk_main;	/* main clock for spi bus */
	struct clk *parent_clk, *sel_clk, *spi_clk;
#endif
//TINNO END
};

static struct efsa120s_data *elan_fp = NULL;

//for power status detect
#define POWER_NOTIFY
static int is_screen_poweroff = 0;

static void elan_spi_clk_enable(struct efsa120s_data *fp, u8 bonoff);




static struct TEEC_UUID vendor_uuid = {
    0x7777c03f, 0xc30c, 0x4dd0, { 0xa3, 0x19, 0xea, 0x29, 0x64, 0x3d, 0x4d, 0x4b }
};
#if BANPOD_READ_ID
static int elan_spi_transfer(struct spi_device *spi, const char *txbuf, char *rxbuf, int len)
{
    struct spi_transfer t = {0};
    struct spi_message m;
    int ret = 0;

#if USE_SPI1_4GB
    /* map physical addr to virtual addr */
    if (NULL == spi_tx_local_buf) {
        spi_tx_local_buf = (char *)ioremap_nocache(SpiDmaBufTx_pa, 0xb000);
        if (!spi_tx_local_buf) {
            pr_err("SPI Failed to dma_alloc_coherent()\n");
            return -ENOMEM;
        }
    }
    if (NULL == spi_rx_local_buf) {
        spi_rx_local_buf = (char *)ioremap_nocache(SpiDmaBufRx_pa, 0xb000);
        if (!spi_rx_local_buf) {
            pr_err("SPI Failed to dma_alloc_coherent()\n");
            return -ENOMEM;
        }
    }
    if(txbuf != NULL){
        memcpy(spi_tx_local_buf, txbuf, len);
        t.tx_buf = spi_tx_local_buf;
        t.tx_dma = SpiDmaBufTx_pa;
    }
    if(rxbuf != NULL){
        t.rx_buf = spi_rx_local_buf;
        t.rx_dma = SpiDmaBufRx_pa;
    }
#else
    t.tx_buf = txbuf;
    t.rx_buf = rxbuf;
#endif        
    //memset(&t, 0, sizeof(t));
    spi_message_init(&m);
    t.bits_per_word = 8;
    t.len = len;
    t.speed_hz = spi_speed;
    spi_message_add_tail(&t, &m);
    ret = spi_sync(spi, &m);
    if (ret == 0) {
#if USE_SPI1_4GB
        if (rxbuf != NULL)
            memcpy(rxbuf, spi_rx_local_buf, len);
#endif
    } 
    else {
        pr_err("%s : spi_sync failed, ret = %d\n", __func__, ret);
    }
    return ret;
}
#endif

void efsa120s_irq_enable(void *_fp)
{
    struct efsa120s_data *fp = _fp;	
    unsigned long irqflags = 0;

//    if (fp->irq_is_disable) 
//    {
        ELAN_DEBUG("enable_irq_wake======.\n");
        spin_lock_irqsave(&fp->irq_lock, irqflags);
        enable_irq_wake(fp->isr);
        fp->irq_is_disable = 0; 
        spin_unlock_irqrestore(&fp->irq_lock, irqflags);
//    }
}

void efsa120s_irq_disable(void *_fp)
{
	struct efsa120s_data *fp = _fp;
	unsigned long irqflags;


    if (!fp->irq_is_disable)
    {
        spin_lock_irqsave(&fp->irq_lock, irqflags);
        fp->irq_is_disable = 1; 
        disable_irq_wake(fp->isr);
        spin_unlock_irqrestore(&fp->irq_lock, irqflags);
        ELAN_DEBUG("disable_irq_wake======.\n");
	}

}
/*
static ssize_t show_drv_version_value(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VERSION_LOG);
}
static DEVICE_ATTR(drv_version, S_IRUGO, show_drv_version_value, NULL);

#ifdef _ELAN_DEBUG_
static ssize_t elan_debug_value(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(elan_debug){
		elan_debug=0;
	} else {
		elan_debug=1;
	}
	return sprintf(buf, "[ELAN] elan debug %d\n", elan_debug);
}
static DEVICE_ATTR(elan_debug, S_IRUGO, elan_debug_value, NULL);
#endif

static struct attribute *efsa120s_attributes[] = {
	&dev_attr_drv_version.attr,
#ifdef _ELAN_DEBUG_
	&dev_attr_elan_debug.attr,
#endif
	NULL
};

static struct attribute_group efsa120s_attr_group = {
	.attrs = efsa120s_attributes,
};
*/
static void efsa120s_reset_output(struct efsa120s_data *fp, int level)
{
	//printk("[efsa120s]efsa120s_reset_output level = %d   ,%d,   %d\n", level,elan_fp->pinctrl1,elan_fp->fp_rst_low);

	if (level)
		pinctrl_select_state(fp->pinctrl1, fp->fp_rst_high);
	else
		pinctrl_select_state(fp->pinctrl1, fp->fp_rst_low);
}

static void efsa120s_reset(struct efsa120s_data *fp)
{
	/* Developement platform */
	efsa120s_reset_output(fp,0);
	mdelay(5);
	efsa120s_reset_output(fp,1);
	mdelay(50);
}

static int efsa120s_open(struct inode *inode, struct file *filp)
{
	struct efsa120s_data *fp = container_of(filp->private_data, struct efsa120s_data, efsa120_dev);	
	filp->private_data = fp;
	ELAN_DEBUG("%s()\n", __func__);
	return 0;
}

static int efsa120s_close(struct inode *inode, struct file *filp)
{
	ELAN_DEBUG("%s()\n", __func__);
	return 0;
}

static int elan_navigation_key_set(int key)
{
	if (key == ELAN_EVT_LEFT) {
		key_status = HW_EVT_MOVE_LEFT;
	} else if (key == ELAN_EVT_RIGHT) {
		key_status = HW_EVT_MOVE_RIGHT;
	} else if (key == ELAN_EVT_UP) {
		key_status = HW_EVT_MOVE_UP;
	} else if (key == ELAN_EVT_DOWN) {
		key_status = HW_EVT_MOVE_DOWN;
	} else {
		return 0;
	}

	return 1;
}

static int elan_read_key_status(void){
	ELAN_DEBUG("elan_read_key_status,key_status = %d\n", key_status);
	if(key_status == HW_EVT_MOVE_DOWN 
	|| key_status == HW_EVT_MOVE_LEFT
	|| key_status == HW_EVT_MOVE_RIGHT
	|| key_status == HW_EVT_MOVE_UP ) {  
		return  key_status;
	}			
	return (key_status == HW_EVT_DOWN ? HW_EVT_DOWN : HW_EVT_UP);
}


static long efsa120s_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct efsa120s_data *fp = filp->private_data;
	int keycode;
	int ret = 0;
	int wake_lock_arg;

	ELAN_DEBUG("%s() : cmd = [%04X]\n", __func__, cmd);

	switch(cmd)
	{
		case ID_IOCTL_RESET: //6
			efsa120s_reset(fp);
			ELAN_DEBUG("ID_IOCTL_RESET\n");
			break;
		case ID_IOCTL_POLL_INIT: //20
			//reinit_completion(&cmd_done_irq);
			elan_work_flag = 0;
			ELAN_DEBUG("ID_IOCTL_POLL_INIT\n");
			break;
		case ID_IOCTL_POLL_EXIT: //23
			//complete(&cmd_done_irq);
			elan_work_flag = 1;
			wake_up(&elan_poll_wq);
			ELAN_DEBUG("ID_IOCTL_POLL_EXIT\n");
			break;
        case ID_IOCTL_INPUT_KEYCODE: //add input keycode by herman KEY_CAMERA = 212
			keycode =(int __user)arg;
			ELAN_DEBUG("%s() : ID_IOCTL_INPUT_KEYCODE check keycode = %d \n",__func__, keycode);
			if (!keycode) {
				ELAN_DEBUG("Keycode %d not defined, ignored.\n", (int __user)arg);
				break ;
			}
			input_report_key(fp->input_dev, keycode, 1); // Added for KEY Event
			input_sync(fp->input_dev);
			input_report_key(fp->input_dev, keycode, 0); // Added for KEY Event
			input_sync(fp->input_dev);
			break;	
			
		case ID_IOCTL_SET_KEYCODE: //add for set keycode by herman KEY_CAMERA = 212
			keycode =(int __user)arg;
                        
			ELAN_DEBUG("%s() : ID_IOCTL_SET_KEYCODE check keycode = %d \n",__func__, keycode);
			if (!keycode) {
				ELAN_DEBUG("Keycode %d not defined, ignored.\n", (int __user)arg);
				break ;
			}
			if (elan_navigation_key_set(keycode) > 0) {
			    if (fasync_queue){
			        ELAN_DEBUG("IOCTL_WRITE_KEY_STATUS,kill_fasync to send key event\n");
			        kill_fasync(&fasync_queue, SIGIO, POLL_IN);
			    }	
			}
			
			break;

		case ID_IOCTL_WAKE_LOCK_UNLOCK: //41
		    wake_lock_arg = (int __user)arg;
		    if (!wake_lock_arg) {
		        __pm_relax(&fp->hal_wake_lock);
		        ELAN_DEBUG("[IOCTL] HAL WAKE UNLOCK = %d\n", wake_lock_arg);
		    }
		    else if (wake_lock_arg) {
		        __pm_stay_awake(&fp->hal_wake_lock);
		        ELAN_DEBUG("[IOCTL] HAL WAKE LOCK = %d\n", wake_lock_arg);
		    }
		    else
		        ELAN_DEBUG("[IOCTL] ERROR WAKE LOCK ARGUMENT\n");
		    break;

		case IOCTL_READ_KEY_STATUS:
			ELAN_DEBUG("IOCTL_READ_KEY_STATUS,key_status = %d\n", key_status);			
			return elan_read_key_status();

		case IOCTL_WRITE_KEY_STATUS:
			ELAN_DEBUG("IOCTL_WRITE_KEY_STATUS,arg = %ld\n", arg);						
			key_status = arg;
			// guomingyi add.
			fp_key_event_record(key_status == 1 ? HW_EVENT_DOWN : HW_EVENT_UP);//mingliang.tan 
			// FP_EVT_REPORT(key_status == 1 ? HW_EVENT_DOWN : HW_EVENT_UP);//mingliang.tan 
			input_report_key(fp->input_dev, 212, key_status); // Added for KEY Event
			input_sync(fp->input_dev);

//			if (fasync_queue){
//			ELAN_DEBUG("IOCTL_WRITE_KEY_STATUS,kill_fasync to send key event\n");
//				kill_fasync(&fasync_queue, SIGIO, POLL_IN);
//			}
			break;

        case ID_IOCTL_READ_FACTORY_STATUS:
            mutex_lock(&elan_factory_mutex);
            ELAN_DEBUG("READ_FACTORY_STATUS = %d", factory_status);
            mutex_unlock(&elan_factory_mutex);
            return factory_status;
            break;
        case ID_IOCTL_IOIRQ_STATUS:
         //   mutex_lock(&elan_factory_mutex);
            irq_status_buffer = irq_status;
            ELAN_DEBUG("READ_IRQ_STATUS = %d", irq_status);
            irq_status = 2;
          //  mutex_unlock(&elan_factory_mutex);
            return irq_status_buffer;
            break;						
        case ID_IOCTL_SET_VERSION: // 100
            ELAN_DEBUG("do nothing\n");
            break;
        case ID_IOCTL_SET_VERSION_0: // 99
            ret = copy_from_user(lib_ver_buf, (char *)arg, MAX_LIB_BUF);
            if (!ret) {
               // full_fp_chip_info(lib_ver_buf);//mingliang.tan 
               REGISTER_FP_DEV_INFO(NULL, lib_ver_buf, NULL, NULL, NULL);
               ELAN_DEBUG("lib_ver_buf= %s\n", lib_ver_buf);
            }
            break;
			
        case ID_IOCTL_GET_VERSION: // 101
            ret = copy_to_user((char *)arg, lib_ver_buf, MAX_LIB_BUF);
            break;

        case ID_IOCTL_WRITE_FACTORY_STATUS:
            mutex_lock(&elan_factory_mutex);
            factory_status = (int __user)arg;
            ELAN_DEBUG("WRITE_FACTORY_STATUS = %d\n", factory_status);
            mutex_unlock(&elan_factory_mutex);
            break;
		case ID_IOCTL_EN_IRQ: //55
			//efsa120s_irq_enable(fp);
			ELAN_DEBUG("ID_IOCTL_EN_IRQ\n");
			break;
		case ID_IOCTL_DIS_IRQ: //66
			//efsa120s_irq_disable(fp);
			ELAN_DEBUG("ID_IOCTL_DIS_IRQ\n");
			break;
		case ID_IOCTL_ENABLE_SPI_CLK:
			ELAN_DEBUG("%s() : ELAN_IOC_ENABLE_SPI_CLK ======\n", __func__);
			elan_spi_clk_enable(fp, 1);
			break;
		case ID_IOCTL_DISABLE_SPI_CLK:
			ELAN_DEBUG("%s() : ID_IOCTL_DISABLE_SPI_CLK ======\n", __func__);
			elan_spi_clk_enable(fp, 0);
			break;
		case ID_IOCTL_GET_SCREEN_STATUS:
			return is_screen_poweroff;
	
		default:
			ELAN_DEBUG("INVALID COMMAND\n");
			break;
	}

	return 0;
}




static unsigned int efsa120s_poll(struct file *file, poll_table *wait)
{

    struct efsa120s_data *fp = file->private_data;
    int ret = 0;
    int mask = 0;
    //ELAN_DEBUG("%s(): wake_unlock ==\n", __func__);
    __pm_relax(&fp->hal_wake_lock);
    ret = wait_event_interruptible(elan_poll_wq, elan_work_flag > 0);
    if (ret != 0) {
        ELAN_DEBUG("%s() ret = %d, elan_work_flag = %d.\n", __func__, ret, elan_work_flag);
    }

    if (elan_work_flag > 0) {
        mask |= POLLIN | POLLRDNORM;
    }

    elan_work_flag = 0;
    return mask;
}

static int elan_fp_fasync(int fd, struct file * filp, int on)
{
	//elan_info("%s enter \n",__func__);
	return fasync_helper(fd, filp, on, &fasync_queue);
}

static const struct file_operations efsa120s_fops = {
	.owner 			= THIS_MODULE,
	.open 			= efsa120s_open,
	.unlocked_ioctl = efsa120s_ioctl,
	.poll			= efsa120s_poll,
	.release 		= efsa120s_close,
	.fasync 		= elan_fp_fasync,
};



static irqreturn_t efsa120s_irq_handler(int irq, void *_fp)
{
	struct efsa120s_data *fp = _fp;

	ELAN_DEBUG("%s()\n", __func__);
	//guomingyi add.
	//FP_EVT_REPORT(HW_EVENT_WAKEUP);
	/* input power keyevent */
    __pm_wakeup_event(&fp->wake_lock, msecs_to_jiffies(3000));
	//complete(&cmd_done_irq);
	//if (fasync_queue){
	//	kill_fasync(&fasync_queue, SIGIO, POLL_IN);
	//}
	elan_work_flag = 1;
	if(factory_status == 0 || factory_status == 1)    
	    wake_up(&elan_poll_wq);
        else
            irq_status = 1;
	return IRQ_HANDLED;
}

static int efsa120s_setup_cdev(struct efsa120s_data *fp)
{
	
	fp->efsa120_dev.minor = MISC_DYNAMIC_MINOR;
	fp->efsa120_dev.name = "elan_fp";
	fp->efsa120_dev.fops = &efsa120s_fops;
	fp->efsa120_dev.mode = S_IFREG|S_IRWXUGO; 
	if (misc_register(&fp->efsa120_dev) < 0) {
  		ELAN_DEBUG("misc_register failed!!");
		return -1;		
	}
  	else {
		ELAN_DEBUG("misc_register finished!!");		
	}
	return 0;
}

#if 0
static int efsa120s_sysfs_create(struct efsa120s_data *sysfs)
{

	struct efsa120s_data *fp = spi_get_drvdata(sysfs->pdev);
	int error = 0;
	
	/* Register sysfs */
	error = sysfs_create_group(&fp->pdev->dev.kobj, &efsa120s_attr_group);
	if (error) {
		dev_err(&fp->pdev->dev, "[ELAN] Failed to create sysfs attributes, err: %d\n", error);
		goto fail_un;
	}
	return 0;
fail_un:
	/* Remove sysfs */
	sysfs_remove_group(&fp->pdev->dev.kobj, &efsa120s_attr_group);
		
	return error;
}
#endif

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
extern void mt_spi_enable_clk(void);
extern void mt_spi_disable_clk(void);
#endif

static void elan_spi_clk_enable(struct efsa120s_data *fp, u8 bonoff)
{

//TINNO BEGIN
//Add spi clk by yinglong.tang
#ifndef CONFIG_MTK_CLKMGR
    int ret;
    static int count = 0;
#endif
//TINNO END
#ifdef CONFIG_MTK_CLKMGR

    #if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
        if (bonoff) {
            mt_spi_enable_clk();
        } else {
            mt_spi_disable_clk();
        }
    #endif
#else

    //TINNO BEGIN
    //Add spi clk by yinglong.tang
    if (bonoff && (count == 0)) {
        ret = clk_prepare_enable(fp->spi_clk);
        count = 1;
    } else if ((count > 0) && (bonoff == 0)) {
        clk_disable_unprepare(fp->spi_clk );
        count = 0;
    }
    //TINNO END

#endif
}

static void efsa120s_gpio_as_int(struct efsa120s_data *fp)
{
	printk("[efsa120s]efsa120s_gpio_as_int\n");
	pinctrl_select_state(fp->pinctrl1, fp->eint_as_int);
}


static char efsa120s_gpio_config(struct efsa120s_data *fp)
{	
    int ret = -1;
    struct device_node *node;
    printk("[elan]:%s enter\n", __func__);
    node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
    if ( node)
    {
        efsa120s_gpio_as_int(fp);		
        fp->isr = irq_of_parse_and_map( node, 0);
        printk("ELAN efsa120s->irq = %d\n",  fp->isr);
        if (! fp->isr)
        {
            printk("ELAN irq_of_parse_and_map fail!!\n");
            return -1;
        }
    }
    else
    {
        printk("ELAN null irq node!!\n");
        return -1;
    }

    ret = request_irq(fp->isr, efsa120s_irq_handler, IRQF_TRIGGER_RISING | IRQF_ONESHOT, "elan_fp_irq", fp);
    if (ret) {
        ELAN_DEBUG("%s : =====EINT IRQ LINE NOT AVAILABLE  %d\n", __func__,ret);
    } else {
        ELAN_DEBUG("%s : ==11===set EINT finished, fp_irq=%d", __func__, fp->isr);
        efsa120s_irq_enable(fp);

    }

    return 0;
}

int elan_parse_dt(struct device *dev, struct efsa120s_data *pdata)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
	if (node) {
		int ret = 0;
		printk("[fp] mt_fp_pinctrl+++++++++++++++++\n");

		pdata->fp_rst_high = pinctrl_lookup_state(pdata->pinctrl1, "fp_rst_high");
		if (IS_ERR(pdata->fp_rst_high)) {
			ret = PTR_ERR(pdata->fp_rst_high);
			dev_err(&pdata->pdev->dev, "fwq Cannot find fp pinctrl fp_rst_high!\n");
			return ret;
		}
		pdata->fp_rst_low = pinctrl_lookup_state(pdata->pinctrl1, "fp_rst_low");
		if (IS_ERR(pdata->fp_rst_low)) {
			ret = PTR_ERR(pdata->fp_rst_low);
			dev_err(&pdata->pdev->dev, "fwq Cannot find fp pinctrl fp_rst_low!\n");
			return ret;
		}
		pdata->eint_as_int = pinctrl_lookup_state(pdata->pinctrl1, "eint_as_int");
		if (IS_ERR(pdata->eint_as_int)) {
			ret = PTR_ERR(pdata->eint_as_int);
			dev_err(&pdata->pdev->dev, "fwq Cannot find fp pinctrl eint_as_int!\n");
			return ret;
		}
		pdata->eint_in_low = pinctrl_lookup_state(pdata->pinctrl1, "eint_in_low");
		if (IS_ERR(pdata->eint_in_low)) {
			ret = PTR_ERR(pdata->eint_in_low);
			dev_err(&pdata->pdev->dev, "fwq Cannot find fp pinctrl eint_output_low!\n");
			return ret;
		}
		pdata->eint_in_float = pinctrl_lookup_state(pdata->pinctrl1, "eint_in_float");
		if (IS_ERR(pdata->eint_in_float)) {
			ret = PTR_ERR(pdata->eint_in_float);
			dev_err(&pdata->pdev->dev, "fwq Cannot find fp pinctrl eint_output_high!\n");
			return ret;
		}
		pdata->fp_enable_low = pinctrl_lookup_state(pdata->pinctrl1, "fp_enable_low");
        	if (IS_ERR(pdata->fp_enable_low)) {
        		ret = PTR_ERR(pdata->fp_enable_low);
        		dev_err(&pdata->pdev->dev, "Cannot find fp pinctrl fp_enable_low!\n");
        		return ret;
        	}
        	pdata->fp_enable_high = pinctrl_lookup_state(pdata->pinctrl1, "fp_enable_high");
        	if (IS_ERR(pdata->fp_enable_high)) {
        		ret = PTR_ERR(pdata->fp_enable_high);
        		dev_err(&pdata->pdev->dev, "Cannot find fp pinctrl fp_enable_high!\n");
        		return ret;
        	}

		return ret;
		printk("[FP] mt_fp_pinctrl----------\n");
	}else{
		
	}
	return 0;
}
int elan_power_init(struct efsa120s_data *pdata)
{
	int ret = 0;


	pdata->reg = regulator_get(pdata->efsa120_dev.this_device, "vio28");
    
	if (IS_ERR(pdata->reg)) {
		ret = PTR_ERR(pdata->reg);
		printk("elan Regulator get failed vdd ret=%d\n", ret);
		return ret;
	}
	
	if (regulator_count_voltages(pdata->reg) > 0) {
		ret = regulator_set_voltage(pdata->reg, 2800000,2800000);
		if (ret) {
			printk("elan Regulator set_vtg failed vdd ret=%d\n", ret);
			goto reg_vdd_put;
		}
	}
	ret = regulator_enable(pdata->reg);	/*enable regulator*/
	if (ret)
		printk("elan regulator_enable() failed!\n");
	
	printk("elan Regulator set_vtg OK vdd ret=%d \n", ret);
	return 0;
	
reg_vdd_put:
	regulator_put(pdata->reg);
	
	return ret;
}


void efsa120s_gpio_power(int onoff)
{
	int ret;
	printk("%s onoff = %d", __func__, onoff);
	if(onoff){
		ret = regulator_enable(elan_fp->reg);	/*enable regulator*/
		if (ret)
			printk("regulator_enable() failed!\n");
	}else{
		ret = regulator_disable(elan_fp->reg);	/*disable regulator*/
		if (ret)
			printk("regulator_disable() failed!\n");
	}
}

#ifdef POWER_NOTIFY 
static int elan_fb_state_chg_callback(struct notifier_block *nb,
	unsigned long val, void *data)
{
	struct fb_event *evdata = data;
	unsigned int blank;
	struct efsa120s_data *fp = container_of(nb, struct efsa120s_data, notifier);
	
	if (val != FB_EARLY_EVENT_BLANK){
		return 0;
	}	
	
	if (evdata && evdata->data && val == FB_EARLY_EVENT_BLANK && fp) {
		blank = *(int *)(evdata->data);
		switch (blank) {
			case FB_BLANK_POWERDOWN:
				is_screen_poweroff = 1;
				elan_work_flag = 1;
				wake_up(&elan_poll_wq);
				//is_interrupt |= 2;
				//wake_up(&fp->efsa_wait);
				break;
			case FB_BLANK_UNBLANK:
				is_screen_poweroff = 0;
				break;
			default:
				pr_info("%s defalut\n", __func__);
				break;
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block elan_noti_block = {
	.notifier_call = elan_fb_state_chg_callback,
};
#endif
//lijian add for check fp info start
extern char* saved_command_line;
#define MAX_PROJECT_NAME_LENGTH 64
static char tinno_fp_info[MAX_PROJECT_NAME_LENGTH+1] = {"fp"};
static char * tinno_get_fp_info(void)        
{
    char * p;
    int length,i;
    
    
    p = strstr(saved_command_line, "androidboot.fpid=");
    if( p == NULL)
    {
        printk(KERN_ERR "cmdline can not find dev config !\n");
        return NULL;
    }

    length = strlen("androidboot.fpid=");
    
    for(i = 0; i < MAX_PROJECT_NAME_LENGTH; i++)
    {
         if((*(p + length + i) == ' ') || (*(p + length + i) == '\0'))
         {
            break;
         }
         tinno_fp_info[i] = *(p + length + i);
    }
    return tinno_fp_info;
}
static int is_use_elanfp(void)
{
    char * tinno_fp_info;

    tinno_fp_info = tinno_get_fp_info();
    if(NULL == tinno_fp_info)
    {
        printk(KERN_INFO "ssss---cannot get elan--fpinfo \n");
        return 0;
    }
    printk(KERN_INFO "ssss---elanfp_688_is_open :fp_info = %s \n",tinno_fp_info);

    if (NULL != strstr(tinno_fp_info, "elan_fp"))
	return 1;
    else
	return 0;
}
//lijian add for check fp info end
static int elan_probe(struct spi_device *pdev)
{	
	struct efsa120s_data *fp = NULL;
	struct input_dev *input_dev = NULL;
	int err = 0,ret = 0;
#if BANPOD_READ_ID
	unsigned char rxbuf[8] = {0};
	unsigned char txbuf[8] = {0};	
#endif
	ELAN_DEBUG("=====%s() Start=====\n", __func__);

   if(0 == is_use_elanfp()){//lijian add for check fp info
   	ELAN_DEBUG("ssss---not use elan fp\n");
   	return (-ENODEV);
   }
   
#if BANPOD_READ_ID
    /* Setup SPI */
    pdev->mode = SPI_MODE_0; 		// set at spi_board_info
    pdev->max_speed_hz = SPI_MAX_SPEED; 	// set at spi_board_info
    pdev->chip_select = 0; 		// set at spi_board_info
    pdev->bits_per_word = 8;			// do not change

    ret = spi_setup(pdev);
    if (ret < 0)
        ELAN_DEBUG("spi_setup failed, ret = %d\n", ret);
#endif
	fp = kzalloc(sizeof(struct efsa120s_data), GFP_KERNEL);
	if(!fp)
	   ELAN_DEBUG("alloc efsa120s data fail.\n");
	elan_fp = fp;

	/* Parse Device Tree */
	pdev->dev.of_node=of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
	elan_power_init(fp);
//#if BANPOD_READ_ID
	
//Add spi clk by yinglong.tang
#if !defined(CONFIG_MTK_CLKMGR)
	fp->parent_clk = devm_clk_get(&pdev->dev, "parent-clk");
	if (IS_ERR(fp->parent_clk)) {
		ret = PTR_ERR(fp->parent_clk);
		dev_err(&pdev->dev, "failed to get parent-clk: %d\n", ret);
		return ret;
	}

	fp->sel_clk = devm_clk_get(&pdev->dev, "sel-clk");
	if (IS_ERR(fp->sel_clk)) {
		ret = PTR_ERR(fp->sel_clk);
		dev_err(&pdev->dev, "failed to get sel-clk: %d\n", ret);
		return ret;
	}

	fp->spi_clk = devm_clk_get(&pdev->dev, "spi-clk");
	if (IS_ERR(fp->spi_clk)) {
		ret = PTR_ERR(fp->spi_clk);
		dev_err(&pdev->dev, "failed to get spi-clk: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(fp->spi_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable spi_clk (%d)\n", ret);
		return ret;
	}

	ret = clk_set_parent(fp->sel_clk, fp->parent_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to clk_set_parent (%d)\n", ret);
		clk_disable_unprepare(fp->spi_clk);
		return ret;
	}
	clk_disable_unprepare(fp->spi_clk );
#endif
//add by yinglong.tang end

#if BANPOD_READ_ID


    efsa120s_reset(fp);
    txbuf[0] = 0xc1;
    ret = elan_spi_transfer(pdev, txbuf, rxbuf, 6);
    if (ret != 0)
        ELAN_DEBUG("spi_sync failed, ret = %d\n", ret);
    else {
        ELAN_DEBUG("read device %02x %02x %02x %02x.\n", rxbuf[2], rxbuf[3], rxbuf[4], rxbuf[5]);
        IMG_WIDTH = (unsigned int)(rxbuf[5] - rxbuf[4] + 1);
        IMG_HEIGHT = (unsigned int)(rxbuf[3] - rxbuf[2] + 1);
        ELAN_DEBUG("WIDTH(Y) = %d, HEIGHT(X) = %d\n", IMG_WIDTH, IMG_HEIGHT);
        if (IMG_WIDTH == 88 && IMG_HEIGHT == 64) {
	    memcpy(&uuid_fp, &vendor_uuid, sizeof(struct TEEC_UUID));
	    ELAN_DEBUG("get_fp_vendor =  %d\n", get_fp_vendor());
	    }else{
            goto fp_probe_fail;
	    }
    }
#else//lijian add uuid
	memcpy(&uuid_fp, &vendor_uuid, sizeof(struct TEEC_UUID));
	ELAN_DEBUG("get_fp_vendor =  %d\n", get_fp_vendor());
#endif

    //elan_power_init(fp);
    
	fp->pinctrl1 = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(fp->pinctrl1)) {
		err = PTR_ERR(fp->pinctrl1);
		dev_err(&pdev->dev, "fwq Cannot find fp pinctrl1!\n");
		return err;
	}    
	err = elan_parse_dt(&pdev->dev, fp);	
	/* Init EFSA120S GPIO */
	spin_lock_init(&fp->irq_lock);
	wakeup_source_init(&fp->wake_lock, "fp_wake_lock");
	wakeup_source_init(&fp->hal_wake_lock, "hal_fp_wake_lock");
	err = efsa120s_gpio_config(fp);
	if(err < 0) {
	    ELAN_DEBUG("GPIO request fail (%d).\n", err);
	    return err;
	}
	/* Init Input Device */

	input_dev = input_allocate_device();
	if (input_dev)
	input_set_capability(input_dev, EV_KEY, 212); // change by customer, send key event to framework. KEY_xxx could be changed.

    input_dev->name = "elan_camera";
    if(input_register_device(input_dev))
    {
        input_free_device(input_dev);
        input_dev = NULL;
        return (-ENODEV);
    }

	fp->input_dev = input_dev;



	/* Init Poll Wait */
	init_waitqueue_head(&fp->efsa_wait);

	/* Init Char Device */
	err = efsa120s_setup_cdev(fp);
	if(err < 0)
		ELAN_DEBUG("efsa120s setup device fail.\n");

#ifdef POWER_NOTIFY
	fp->notifier = elan_noti_block;
	fb_register_client(&fp->notifier);
#endif
    REGISTER_FP_DEV_INFO("elan_fp", NULL, pdev, get_finger_state, read_chip_id);
	return 0;
	
#if BANPOD_READ_ID
fp_probe_fail:
	spi_set_drvdata(pdev, NULL);
	if(input_dev !=NULL){
	    input_free_device(input_dev);
	    input_dev = NULL;
	}
	kfree(fp);
	fp = NULL;
	elan_fp = NULL;
	return -ENOMEM;
#endif
}

//static int efsa120s_remove(struct spi_device *pdev)
static int elan_remove(struct spi_device *pdev)
{
	struct efsa120s_data *fp = spi_get_drvdata(pdev);
	
	if (fp->isr)
		free_irq(fp->isr, fp);

	gpio_free(fp->irq_gpio);
	gpio_free(fp->rst_gpio);
	
	misc_deregister(&fp->efsa120_dev);
	input_free_device(fp->input_dev);
		
	kfree(fp);

	//platform_set_drvdata(pdev, NULL);
	spi_set_drvdata(pdev, NULL);
	return 0;
}

/** guomingyi add for test */
static int read_chip_id(char *buf, void *args)
{
    return 0;
}

// for ftm.
static int get_finger_state(char *buf, void *args) 
{
    return sprintf(buf, "%d", key_status);
}
/** guomingyi add for test */



#ifdef CONFIG_PM_SLEEP
static int efsa120s_suspend(struct device *dev)
{
	ELAN_DEBUG("efsa120s suspend!\n");
	return 0;
}

static int efsa120s_resume(struct device *dev)
{
	ELAN_DEBUG("efsa120s resume!\n");
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(efsa120s_pm_ops, efsa120s_suspend, efsa120s_resume);

#ifdef CONFIG_OF
static const struct of_device_id efsa120s_of_match[] = {
	{ .compatible = "mediatek,fingerprint", },
	{ .compatible = "mediatek,elan-fp", },
	{ .compatible = "elan,elan-fp", },
	{},
};
MODULE_DEVICE_TABLE(of, efsa120s_of_match);
#endif


static const struct platform_device_id efp_id[] = {
	{"elan_fp", 0},
	{}
};
MODULE_DEVICE_TABLE(spi, efp_id);

static struct spi_driver elan_driver = {
    .driver = {
        .name           = "elan_fp",
        .owner          = THIS_MODULE,
        .of_match_table = efsa120s_of_match
    },
    .probe      = elan_probe,
    .remove     = elan_remove,
};

static int __init efsa120s_init(void)
{
    int ret = 0;
    ELAN_DEBUG("%s() Start\n", __func__);

    ret = spi_register_driver(&elan_driver);
    if (ret < 0)
        ELAN_DEBUG("%s FAIL !\n", __func__);

    ELAN_DEBUG("%s() End\n", __func__);
    return 0;
}

static void __exit efsa120s_exist(void)
{	
	spi_unregister_driver(&elan_driver);

}

late_initcall(efsa120s_init);
module_exit(efsa120s_exist);

MODULE_AUTHOR("KennyKang <kenny.kang@emc.com.tw>");
MODULE_DESCRIPTION("ELAN SPI FingerPrint eFSA120S driver");
MODULE_VERSION(VERSION_LOG);
MODULE_LICENSE("GPL");
