#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/sysfs.h>
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by kmalloc */
#include <linux/uaccess.h>      /* needed by copy_to_user */
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

#include <scp_helper.h>
#include <mt_spm_sleep.h>       /* for spm_ap_mdsrc_req */


#include "audio_messenger_ipi.h"
#include "audio_dma_buf_control.h"

#include "audio_ipi_client_phone_call.h"
#include "audio_speech_msg_id.h"



/*==============================================================================
 *                     MACRO
 *============================================================================*/

#define AUDIO_IPI_DEVICE_NAME "audio_ipi"
#define AUDIO_IPI_IOC_MAGIC 'i'

#define AUDIO_IPI_IOCTL_SEND_MSG_ONLY _IOW(AUDIO_IPI_IOC_MAGIC, 0, unsigned int)
#define AUDIO_IPI_IOCTL_SEND_PAYLOAD  _IOW(AUDIO_IPI_IOC_MAGIC, 1, unsigned int)
#define AUDIO_IPI_IOCTL_SEND_DRAM     _IOW(AUDIO_IPI_IOC_MAGIC, 2, unsigned int)

#define AUDIO_IPI_IOCTL_DUMP_PCM      _IOW(AUDIO_IPI_IOC_MAGIC, 97, unsigned int)
#define AUDIO_IPI_IOCTL_REG_FEATURE   _IOW(AUDIO_IPI_IOC_MAGIC, 98, unsigned int)
#define AUDIO_IPI_IOCTL_SPM_MDSRC_ON  _IOW(AUDIO_IPI_IOC_MAGIC, 99, unsigned int)

/*==============================================================================
 *                     private global members
 *============================================================================*/

static bool b_speech_on;
static bool b_spm_ap_mdsrc_req_on;
static bool b_dump_pcm_enable;
static audio_resv_dram_t *p_resv_dram;


/*==============================================================================
 *                     functions declaration
 *============================================================================*/



/*==============================================================================
 *                     implementation
 *============================================================================*/



static int parsing_ipi_msg_from_user_space(
	void __user *user_data_ptr,
	audio_ipi_msg_data_t data_type)
{
	int retval = 0;

	ipi_msg_t ipi_msg;
	uint32_t msg_len = 0;

	/* get message size to read */
	msg_len = (data_type == AUDIO_IPI_MSG_ONLY)
		  ? IPI_MSG_HEADER_SIZE
		  : MAX_IPI_MSG_BUF_SIZE;

	retval = copy_from_user(&ipi_msg, user_data_ptr, msg_len);
	if (retval != 0) {
		AUD_LOG_E("msg copy_from_user retval %d\n", retval);
		goto parsing_exit;
	}

	/* double check */
	AUD_ASSERT(ipi_msg.data_type == data_type);

	/* get dram buf if need */
	if (ipi_msg.data_type == AUDIO_IPI_DMA) {
		if (ipi_msg.param1 > p_resv_dram->size) {
			AUD_LOG_E("dma_data_len %u > p_resv_dram->size %u\n",
				  ipi_msg.param1, p_resv_dram->size);
			retval = -1;
			goto parsing_exit;
		}
		retval = copy_from_user(
				 p_resv_dram->vir_addr,
				 (void __user *)ipi_msg.dma_addr,
				 ipi_msg.param1);
		if (retval != 0) {
			AUD_LOG_E("dram copy_from_user retval %d\n", retval);
			goto parsing_exit;
		}
		ipi_msg.dma_addr = p_resv_dram->phy_addr;
	}

	/* get message size to write */
	msg_len = get_message_buf_size(&ipi_msg);
	check_msg_format(&ipi_msg, msg_len);
	audio_send_ipi_msg_to_scp(&ipi_msg);


parsing_exit:
	return retval;
}


static long audio_ipi_driver_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	AUD_LOG_V("%s(), cmd = %u, arg = %lu\n", __func__, cmd, arg);

	switch (cmd) {
	case AUDIO_IPI_IOCTL_SEND_MSG_ONLY: {
		retval = parsing_ipi_msg_from_user_space(
				 (void __user *)arg, AUDIO_IPI_MSG_ONLY);
		break;
	}
	case AUDIO_IPI_IOCTL_SEND_PAYLOAD: {
		retval = parsing_ipi_msg_from_user_space(
				 (void __user *)arg, AUDIO_IPI_PAYLOAD);
		break;
	}
	case AUDIO_IPI_IOCTL_SEND_DRAM: {
		retval = parsing_ipi_msg_from_user_space(
				 (void __user *)arg, AUDIO_IPI_DMA);
		break;
	}
	/* TOOD: use message */
	case AUDIO_IPI_IOCTL_DUMP_PCM: {
		AUD_LOG_D("%s(), AUDIO_IPI_IOCTL_DUMP_PCM(%lu)\n", __func__, arg);
		b_dump_pcm_enable = arg;
		break;
	}
	/* TOOD: use message */
	case AUDIO_IPI_IOCTL_REG_FEATURE: {
		AUD_LOG_V("%s(), AUDIO_IPI_IOCTL_REG_FEATURE(%lu)\n", __func__, arg);
		if (arg) { /* enable scp speech */
			if (b_speech_on == false) {
				b_speech_on = true;
				register_feature(OPEN_DSP_FEATURE_ID);
				request_freq();
				if (b_dump_pcm_enable)
					open_dump_file();
			}
		} else { /* disable scp speech */
			if (b_speech_on == true) {
				b_speech_on = false;
				close_dump_file();
				deregister_feature(OPEN_DSP_FEATURE_ID);
				request_freq();
			}
		}
		break;
	}
	/* TOOD: use message */
	case AUDIO_IPI_IOCTL_SPM_MDSRC_ON: {
		if (arg) { /* enable scp speech */
			if (b_spm_ap_mdsrc_req_on == false) {
				b_spm_ap_mdsrc_req_on = true;
				AUD_LOG_D("%s(), spm_ap_mdsrc_req(%lu)\n", __func__, arg);
				spm_ap_mdsrc_req(arg);
			}
		} else { /* disable scp speech */
			if (b_spm_ap_mdsrc_req_on == true) { /* false: error handling when reboot */
				b_spm_ap_mdsrc_req_on = false;
				AUD_LOG_D("%s(), spm_ap_mdsrc_req(%lu)\n", __func__, arg);
				spm_ap_mdsrc_req(arg);
			}
		}
		break;
	}
	default:
		break;
	}
	return retval;
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


/* TODO: use ioctl */
static ssize_t audio_ipi_driver_read(
	struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return audio_ipi_client_phone_call_read(file, buf, count, ppos);
}



static const struct file_operations audio_ipi_driver_ops = {
	.owner          = THIS_MODULE,
	.read           = audio_ipi_driver_read,
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

	/* TODO: ring buf */
	init_reserved_dram();
	p_resv_dram = get_reserved_dram();
	AUD_ASSERT(p_resv_dram != NULL);

	audio_ipi_client_phone_call_init();

	ret = misc_register(&audio_ipi_device);
	if (unlikely(ret != 0)) {
		AUD_LOG_E("[SCP] misc register failed\n");
		return ret;
	}

	b_speech_on = false;
	b_spm_ap_mdsrc_req_on = false;
	b_dump_pcm_enable = false;

	return ret;
}


static void __exit audio_ipi_driver_exit(void)
{
	audio_ipi_client_phone_call_deinit();
	misc_deregister(&audio_ipi_device);
}


module_init(audio_ipi_driver_init);
module_exit(audio_ipi_driver_exit);

