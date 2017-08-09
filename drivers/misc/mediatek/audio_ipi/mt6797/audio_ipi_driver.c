#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/sysfs.h>
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by kmalloc */
#include <linux/uaccess.h>      /* needed by copy_to_user */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/slab.h>         /* needed by kmalloc */
#include <linux/poll.h>         /* needed by poll */
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_fdt.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include <mt_spm_sleep.h>       /* for spm_ap_mdsrc_req */

#include "audio_messenger_ipi.h"

#include "scp_helper.h"


/*==============================================================================
 *                     MACRO
 *============================================================================*/

#define AUDIO_IPI_DEVICE_NAME "audio_ipi"
#define AUDIO_IPI_IOC_MAGIC 'i'

#define AUDIO_IPI_IOCTL_SEND_MSG_ONLY _IOW(AUDIO_IPI_IOC_MAGIC, 0, unsigned int)
#define AUDIO_IPI_IOCTL_SEND_PAYLOAD  _IOW(AUDIO_IPI_IOC_MAGIC, 1, unsigned int)
#define AUDIO_IPI_IOCTL_SEND_DRAM     _IOW(AUDIO_IPI_IOC_MAGIC, 2, unsigned int)

#define AUDIO_IPI_IOCTL_SPM_MDSRC_ON  _IOW(AUDIO_IPI_IOC_MAGIC, 99, unsigned int)

/*==============================================================================
 *                     private global members
 *============================================================================*/

typedef struct {
	char    *phy_addr;
	char    *vir_addr;
	uint32_t size;
} audio_resv_dram_t;

static audio_resv_dram_t resv_dram;


/*==============================================================================
 *                     functions declaration
 *============================================================================*/



/*==============================================================================
 *                     implementation
 *============================================================================*/

static void get_reserved_dram(void)
{
	resv_dram.phy_addr = (char *)get_reserve_mem_phys(OPENDSP_MEM_ID);
	resv_dram.vir_addr = (char *)get_reserve_mem_virt(OPENDSP_MEM_ID);
	resv_dram.size     = (uint32_t)get_reserve_mem_size(OPENDSP_MEM_ID);
	AUD_LOG_V("resv_dram: pa %p, va %p, sz 0x%x",
		  resv_dram.phy_addr, resv_dram.vir_addr, resv_dram.size);
}

static void parsing_ipi_msg_from_user_space(void __user *user_data_ptr)
{
	int ret = 0;

	ipi_msg_t ipi_msg;
	uint32_t msg_len = 0;

	/* TODO: read necessary len only */
	ret = copy_from_user(&ipi_msg, user_data_ptr, MAX_IPI_MSG_BUF_SIZE);
	if (ret != 0) {
		AUD_LOG_E("msg copy_from_user ret %d\n", ret);
		return;
	}

	/* get dram buf if need */
	/* TODO: ring buf */
	memset_io(resv_dram.vir_addr, 0, 1024);
	if (ipi_msg.data_type == AUDIO_IPI_DMA) {
		ret = copy_from_user(
			      resv_dram.vir_addr,
			      (void __user *)ipi_msg.dma_addr,
			      ipi_msg.param1);
		if (ret != 0) {
			AUD_LOG_E("dram copy_from_user ret %d\n", ret);
			return;
		}
		ipi_msg.dma_addr = resv_dram.phy_addr;
	}

	msg_len = get_message_buf_size(&ipi_msg);
	check_msg_format(&ipi_msg, msg_len);
	audio_send_ipi_msg_to_scp(&ipi_msg);
}


static long audio_ipi_driver_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	/* TODO: optimize here */
	case AUDIO_IPI_IOCTL_SEND_MSG_ONLY:
	case AUDIO_IPI_IOCTL_SEND_PAYLOAD:
	case AUDIO_IPI_IOCTL_SEND_DRAM: {
		parsing_ipi_msg_from_user_space((void __user *)arg);
		break;
	}
	case AUDIO_IPI_IOCTL_SPM_MDSRC_ON: {
		AUD_LOG_V("%s(), spm_ap_mdsrc_req(%lu)\n", __func__, arg);
		spm_ap_mdsrc_req(arg);
		break;
	}
	default:
		break;
	}
	return 0;
}

#ifdef CONFIG_COMPAT
static long audio_ipi_driver_compat_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op || !file->f_op->unlocked_ioctl) {
		AUD_LOG_E("op null\n");
		return -ENOTTY;
	}
	return file->f_op->unlocked_ioctl(file, cmd, arg);
}
#endif


static const struct file_operations audio_ipi_driver_ops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = audio_ipi_driver_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = audio_ipi_driver_compat_ioctl,
#endif
};


static struct miscdevice audio_ipi_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = AUDIO_IPI_DEVICE_NAME,
	.fops = &audio_ipi_driver_ops
};


static int __init audio_ipi_driver_init(void)
{
	int ret = 0;

	audio_messenger_ipi_init();
	get_reserved_dram();

	ret = misc_register(&audio_ipi_device);
	if (unlikely(ret != 0)) {
		AUD_LOG_E("[SCP] misc register failed\n");
		return ret;
	}

	return ret;
}


static void __exit audio_ipi_driver_exit(void)
{
	misc_deregister(&audio_ipi_device);
}


module_init(audio_ipi_driver_init);
module_exit(audio_ipi_driver_exit);

