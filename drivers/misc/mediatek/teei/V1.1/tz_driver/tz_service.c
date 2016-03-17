#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include "teei_id.h"
#include "nt_smc_call.h"
#include "teei_debug.h"
#include "tz_service.h"
#include "teei_common.h"
#include "../tz_vfs/VFS.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/cpu.h>

#define MAX_BUFF_SIZE           (4096)
#define NQ_SIZE                 (4096)
#define CTL_BUFF_SIZE           (4096)
#define VDRV_MAX_SIZE           (0x80000)

#define MESSAGE_LENGTH		(4096)
#define NQ_VALID		1

#define SWITCH_IRQ		(282+50)

#define F_CREATE_NQ_ID		0x01
#define F_CREATE_CTL_ID		0x02
#define F_CREATE_VDRV_ID	0x04

#define MESSAGE_SIZE		(4096)
#define VALID_TYPE		(1)
#define INVALID_TYPE		(0)

#define FAST_CALL_TYPE		(0x100)
#define STANDARD_CALL_TYPE	(0x200)
#define TYPE_NONE		(0x300)

#define FAST_CREAT_NQ		(0x40)
#define FAST_ACK_CREAT_NQ	(0x41)
#define FAST_CREAT_VDRV		(0x42)
#define FAST_ACK_CREAT_VDRV	(0x43)
#define FAST_CREAT_SYS_CTL	(0x44)
#define FAST_ACK_CREAT_SYS_CTL	(0x45)
#define FAST_CREAT_FDRV		(0x46)
#define FAST_ACK_CREAT_FDRV	(0x47)

#define NQ_CALL_TYPE		(0x60)
#define VDRV_CALL_TYPE		(0x61)
#define SCHD_CALL_TYPE		(0x62)
#define FDRV_ACK_TYPE		(0x63)

#define STD_INIT_CONTEXT	(0x80)
#define STD_ACK_INIT_CONTEXT	(0x81)
#define STD_OPEN_SESSION	(0x82)
#define STD_ACK_OPEN_SESSION	(0x83)
#define STD_INVOKE_CMD		(0x84)
#define STD_ACK_INVOKE_CMD	(0x85)
#define STD_CLOSE_SESSION	(0x86)
#define STD_ACK_CLOSE_SESSION	(0x87)
#define STD_CLOSE_CONTEXT	(0x88)
#define STD_ACK_CLOSE_CONTEXT	(0x89)

#define FP_BUFF_SIZE		(512 * 1024)
#define FP_SYS_NO		(100)

#define START_STATUS		(0)
#define END_STATUS		(1)
#define VFS_SIZE		(512 * 1024)


#define CAPI_CALL       0x01
#define FDRV_CALL       0x02
#define BDRV_CALL       0x03
#define SCHED_CALL      0x04

#define FP_SYS_NO       100

#define VFS_SYS_NO      0x08
#define REETIME_SYS_NO  0x07 

unsigned long message_buff = 0;
unsigned long fdrv_message_buff = 0;
unsigned long bdrv_message_buff = 0;
unsigned long tlog_message_buff = 0;

static unsigned long nt_t_buffer;
unsigned long t_nt_buffer;

static unsigned long sys_ctl_buffer;
unsigned long fp_buff_addr = NULL;

extern int get_current_cpuid(void);

#define NQ_BUFF_SIZE		(4096)
#define NQ_BLOCK_SIZE		(32)
#define BLOCK_MAX_COUNT		(NQ_BUFF_SIZE / NQ_BLOCK_SIZE - 1)


#define STD_NQ_ACK_ID		0x01

#define TEE_NAME_SIZE		(255)

#define GLSCH_NEG		(0x03)
#define GLSCH_NONE		(0x00)
#define GLSCH_LOW		(0x01)
#define GLSCH_HIGH		(0x02)


struct NQ_head {
	unsigned int start_index;
	unsigned int end_index;
	unsigned int Max_count;
	unsigned char reserve[20];
};

struct NQ_entry {
	unsigned int valid_flag;
	unsigned int length;
	unsigned int buffer_addr;
	unsigned char reserve[20];
};


/******************************
 * Message header
 ******************************/

struct message_head {
	unsigned int invalid_flag;
	unsigned int message_type;
	unsigned int child_type;
	unsigned int param_length;
};


struct fdrv_message_head {
	unsigned int driver_type;
	unsigned int fdrv_param_length;
};

/******************************
 * Fast call structures
 ******************************/

struct create_NQ_struct {
	unsigned int n_t_nq_phy_addr;
	unsigned int n_t_size;
	unsigned int t_n_nq_phy_addr;
	unsigned int t_n_size;
};


struct create_vdrv_struct {
	unsigned int vdrv_type;
	unsigned int vdrv_phy_addr;
	unsigned int vdrv_size;
};

struct create_fdrv_struct {
	unsigned int fdrv_type;
	unsigned int fdrv_phy_addr;
	unsigned int fdrv_size;
};

struct create_sys_ctl_struct {
	unsigned int sys_ctl_phy_addr;
	unsigned int sys_ctl_size;
};


struct ack_fast_call_struct {
	int retVal;
};

/*********************************
 * Standard call structures
 *********************************/
struct ack_vdrv_struct {
	unsigned int sysno;
};

struct fdrv_struct {
	unsigned int driver_id;
};

struct TEEI_printer_command {
	int func;
	int cmd_size;

	union func_arg {
		struct func_write {
			int length;
			int timeout;
		} func_write_args;
	} args;

};

union TEEI_printer_response {
	int value;
};

struct reetime_handle_struct {
	struct service_handler *handler;
	int retVal;
};

struct reetime_handle_struct reetime_handle_entry;

struct vfs_handle_struct {
	struct service_handler *handler;
	int retVal;
};

struct vfs_handle_struct vfs_handle_entry;

static int serivce_cmd_flag;
static int smc_flag;
static int sys_call_no;
static long register_shared_param_buf(struct service_handler *handler);
static int register_interrupt_handler(void);
static int init_all_service_handlers(void);
static int start_teei_service(void);
static int printer_thread_function(unsigned long virt_addr, unsigned long para_vaddr, unsigned long buff_vaddr);

unsigned char *printer_share_mem = NULL;
EXPORT_SYMBOL_GPL(printer_share_mem);

unsigned int printer_shmem_flags = 0;
EXPORT_SYMBOL_GPL(printer_shmem_flags);

unsigned char *daulOS_share_mem = NULL;
EXPORT_SYMBOL_GPL(daulOS_share_mem);

unsigned char *daulOS_VFS_share_mem = NULL;
EXPORT_SYMBOL_GPL(daulOS_VFS_share_mem);

unsigned char *vfs_flush_address = NULL;
EXPORT_SYMBOL_GPL(vfs_flush_address);


unsigned char *daulOS_VFS_write_share_mem = NULL;
EXPORT_SYMBOL_GPL(daulOS_VFS_write_share_mem);

unsigned char *daulOS_VFS_read_share_mem = NULL;
EXPORT_SYMBOL_GPL(daulOS_VFS_read_share_mem);

#define SHMEM_ENABLE   0
#define SHMEM_DISABLE  1

struct work_entry {
	int call_no;

	struct work_struct work;
};

struct load_soter_entry {
	unsigned long vfs_addr;
	struct work_struct work;
};

static struct load_soter_entry load_ent;
struct work_queue *secure_wq = NULL;
static struct work_entry work_ent;

struct timeval stime;
struct timeval etime;
int vfs_write_flag = 0;
unsigned long teei_vfs_flag = 0;


struct bdrv_call_struct {
        int bdrv_call_type;
        struct service_handler *handler;
        int retVal;
};

extern int add_work_entry(int work_type, unsigned long buff);

#define printk(fmt, args...) printk("\033[;34m[TEEI][TZDriver]"fmt"\033[0m", ##args)
/*add by microtrust*/
static unsigned int get_master_cpu_id(unsigned int gic_irqs)
{
	unsigned int i;
	i = gic_irqs >> 2;
	unsigned long gic_base = 0x10231000 + 0x800 + i * 4 ;
	void __iomem *dist_base = (void  *)gic_base;


	unsigned int  target_id = readl_relaxed(dist_base);
	target_id = target_id >> ((gic_irqs & 0x03) * 8);
	target_id = target_id & 0xff;
	return target_id;
}
/****************************************
//extern u32 get_irq_target_microtrust(void);
//~ static int get_current_cpuid(void)
//~ {
	//~
	//~
	//~
	//~ #if 0
	//~ int cpu_id = get_irq_target_microtrust();
	//~ if (cpu_id == 0xff) {
	//~ printk("error cpu_id [0x%x]\n", cpu_id);
	//~ printk("error cpu_id [0x%x]\n", cpu_id);
	//~ printk("error cpu_id [0x%x]\n", cpu_id);
//~
	//~ }
	//~ printk(" Get current cpu id [0x%x]\n", cpu_id);
	//~ return cpu_id;
//~ #endif
//~ static  int  cpuid = 4;
//~ if(cpuid == 4)
//~ cpuid = 5;
//~ else
//~ cpuid = 4;
//~ return cpuid;
//~
//~ }
*/
int cal_time(struct timeval start_time, struct timeval end_time)
{
	int timestamp = 0;

	timestamp = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
	printk("function expend %d u-seconds! \n", timestamp);
	return timestamp;
}

static void secondary_teei_ack_invoke_drv(void)
{
	n_ack_t_invoke_drv(0, 0, 0);
	return;
}


static void post_teei_ack_invoke_drv(int cpu_id)
{
	get_online_cpus();
	smp_call_function_single(cpu_id,
				secondary_teei_ack_invoke_drv,
				NULL,
				1);
	put_online_cpus();
	
	return;
}



static void teei_ack_invoke_drv(void)
{
	int cpu_id = 0;
#if 0
	int cpu_id = smp_processor_id();
	/* int cpu_id = raw_smp_processor_id(); */

	if (cpu_id != 0) {
		/* call the mb() */
		mb();
		post_teei_ack_invoke_drv(0); /* post it to primary */
	} else {
		/* printk("[%s][%d]\n", __func__, __LINE__); */
		n_ack_t_invoke_drv(0, 0, 0); /* called directly on primary core */
	}

#else
	cpu_id = get_current_cpuid();
	post_teei_ack_invoke_drv(cpu_id);
#endif
	return;
}

void tz_down(struct semaphore *sem)
{
	unsigned long retVal = 1;
	do {
		retVal = down_trylock(sem);
		if (retVal == 0)
			break;
		else
			udelay(100);
	} while (true);
}


/*****************************Drivers**************************************/
static struct service_handler fiq_drivers;

/* #define printk(fmt, args...) printk("\033[;34m[TEEI][TZDriver]"fmt"\033[0m", ##args) */

static void fiq_drivers_init(struct service_handler *handler)
{
	register_shared_param_buf(handler);
}

static void fiq_drivers_deinit(struct service_handler *handler)
{
	return;
}

static int  fiq_drivers_handle(struct service_handler *handler)
{
	return 0;
}
/******************************TIME**************************************/
#include <linux/time.h>
void set_ack_vdrv_cmd(unsigned int sys_num)
{

	if (boot_soter_flag == START_STATUS) {

		struct message_head msg_head;
		struct ack_vdrv_struct ack_body;

		memset(&msg_head, 0, sizeof(struct message_head));

		msg_head.invalid_flag = VALID_TYPE;
		msg_head.message_type = STANDARD_CALL_TYPE;
		msg_head.child_type = N_ACK_T_INVOKE_DRV;
		msg_head.param_length = sizeof(struct ack_vdrv_struct);

		ack_body.sysno = sys_num;

		memcpy(message_buff, &msg_head, sizeof(struct message_head));
		memcpy(message_buff + sizeof(struct message_head), &ack_body, sizeof(struct ack_vdrv_struct));

		Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);
	} else {
		*((int *)bdrv_message_buff) = sys_num;
		Flush_Dcache_By_Area((unsigned long)bdrv_message_buff, (unsigned long)bdrv_message_buff + MESSAGE_SIZE);
	}

	return;
}

static struct service_handler reetime;
static void reetime_init(struct service_handler *handler)
{
	register_shared_param_buf(handler);

}

static void reetime_deinit(struct service_handler *handler)
{
	return;
}

int  __reetime_handle(struct service_handler *handler)
{
	struct timeval tv;
	void *ptr = NULL;
	int tv_sec;
	int tv_usec;
	do_gettimeofday(&tv);
	ptr = handler->param_buf;
	tv_sec = tv.tv_sec;
	*((int *)ptr) = tv_sec;
	tv_usec = tv.tv_usec;
	*((int *)ptr + 1) = tv_usec;

	Flush_Dcache_By_Area((unsigned long)handler->param_buf, (unsigned long)handler->param_buf + handler->size);

	set_ack_vdrv_cmd(handler->sysno);
	teei_vfs_flag = 0;
	/* down(&smc_lock); */
#if 0
	teei_ack_invoke_drv();
#else
	n_ack_t_invoke_drv(0, 0, 0);
#endif
	return 0;
}

static void secondary_reetime_handle(void *info)
{
	struct reetime_handle_struct *cd = (struct reetime_handle_struct *)info;

	/* with a rmb() */
	rmb();

	cd->retVal = __reetime_handle(cd->handler);

	/* with a wmb() */
	wmb();
}


static int reetime_handle(struct service_handler *handler)
{
	int cpu_id = 0;
	int retVal = 0;
	struct bdrv_call_struct *reetime_bdrv_ent = NULL;

	down(&smc_lock);

#if 0
	reetime_handle_entry.handler = handler;
#else
	reetime_bdrv_ent = (struct bdrv_call_struct *)kmalloc(sizeof(struct bdrv_call_struct), GFP_KERNEL);
	reetime_bdrv_ent->handler = handler;
	reetime_bdrv_ent->bdrv_call_type = REETIME_SYS_NO;
#endif
	/* with a wmb() */
	wmb();

#if 0
	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_reetime_handle, (void *)(&reetime_handle_entry), 1);
	put_online_cpus();
#else
	retVal = add_work_entry(BDRV_CALL, (unsigned long)reetime_bdrv_ent);
        if (retVal != 0) {
                up(&smc_lock);
                return retVal;
        }
#endif
	rmb();
#if 0
	return reetime_handle_entry.retVal;
#else
	return 0;
#endif 
}






/******************************SOCKET**************************************/
#include "SOCK.h"
static struct service_handler socket;
static unsigned long para_vaddr;
static unsigned long buff_vaddr;
static struct service_handler vfs_handler;

#define SOCKET_BASE 0xFE021000
#define PAGE_SIZE 4096
enum {
	FUCTION_socket =		0x0,
	FUCTION_connect =		0x04,
	FUCTION_send  =			0x08,
	FUCTION_recv =			0x0C,
	FUCTION_close =			0x20,
	SET_BUFFER_BASE	=		0xF0,
	SET_PARAM_BASE =		0xF4,
};



static void socket_init(struct service_handler *handler)
{
	unsigned long para_paddr = 0;
	unsigned long buff_paddr = 0;

	para_vaddr = (unsigned long)kmalloc(PAGE_SIZE, GFP_KERNEL);
	buff_vaddr = (unsigned long)kmalloc(PAGE_SIZE, GFP_KERNEL);
#if 0 /* for qemu */
	para_paddr = virt_to_phys((void *)para_vaddr);
	buff_paddr = virt_to_phys((void *)buff_vaddr);

	writel(para_paddr, SOCKET_BASE + SET_PARAM_BASE);
	writel(buff_paddr, SOCKET_BASE + SET_BUFFER_BASE);
#endif /* for qemu */

	register_shared_param_buf(handler);
}

static void socket_deinit(struct service_handler *handler)
{
	kfree((void *)para_vaddr);
	kfree((void *)buff_vaddr);
}

static int  socket_handle(struct service_handler *handler)
{
	/* socket_thread_function((unsigned long)handler->param_buf, para_vaddr, buff_vaddr); */
	return 0;
}

/********************************************************************
 *                      VFS functions                               *
 ********************************************************************/
int vfs_thread_function(unsigned long virt_addr, unsigned long para_vaddr, unsigned long buff_vaddr)
{

	/*printk("=========================================================================\n");*/
	daulOS_VFS_share_mem = virt_addr;
#ifdef VFS_RDWR_SEM
	up(&VFS_rd_sem);
	down_interruptible(&VFS_wr_sem);
#else
	complete(&VFS_rd_comp);
	wait_for_completion_interruptible(&VFS_wr_comp);
#endif
	/*printk("=============================2222222222222222222222222222222222============================================\n");*/
}



static void vfs_init(struct service_handler *handler) /*! init service */
{
	register_shared_param_buf(handler);
	vfs_flush_address = handler->param_buf;
	return;
}

static void vfs_deinit(struct service_handler *handler) /*! stop service  */
{
	return;
}


int __vfs_handle(struct service_handler *handler) /*! invoke handler */
{
	/* vfs_thread_function(handler->param_buf, para_vaddr, buff_vaddr); */

	Flush_Dcache_By_Area((unsigned long)handler->param_buf, (unsigned long)handler->param_buf + handler->size);

	set_ack_vdrv_cmd(handler->sysno);
	teei_vfs_flag = 0;
	/* down(&smc_lock); */

#if 0
	teei_ack_invoke_drv();
#else
	n_ack_t_invoke_drv(0, 0, 0);
#endif
	return 0;
}


static void secondary_vfs_handle(void *info)
{
	struct vfs_handle_struct *cd = (struct vfs_handle_struct *)info;
/* with a rmb() */
	rmb();

	cd->retVal = __vfs_handle(cd->handler);
	/* with a wmb() */
	wmb();
}


static int vfs_handle(struct service_handler *handler)
{
	int cpu_id = 0;
	int retVal = 0;
	struct bdrv_call_struct *vfs_bdrv_ent = NULL;

	vfs_thread_function(handler->param_buf, para_vaddr, buff_vaddr);
	down(&smc_lock);

#if 0
	vfs_handle_entry.handler = handler;
#else
	vfs_bdrv_ent = (struct bdrv_call_struct *)kmalloc(sizeof(struct bdrv_call_struct), GFP_KERNEL);
	vfs_bdrv_ent->handler = handler;
	vfs_bdrv_ent->bdrv_call_type = VFS_SYS_NO;
#endif
	/* with a wmb() */
	wmb();

#if 0
	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_vfs_handle, (void *)(&vfs_handle_entry), 1);
	put_online_cpus();
#else
	Flush_Dcache_By_Area((unsigned long)vfs_bdrv_ent, (unsigned long)vfs_bdrv_ent + sizeof(struct bdrv_call_struct));
	retVal = add_work_entry(BDRV_CALL, (unsigned long)vfs_bdrv_ent);
        if (retVal != 0) {
                up(&smc_lock);
                return retVal;
        }
#endif	
	rmb();

#if 0
	return vfs_handle_entry.retVal;
#else
	return 0;
#endif
}

/**********************************************************************
 *                     Printer functions                              *
 **********************************************************************/
static struct service_handler printer_driver;
static void printer_driver_init(struct service_handler *handler) /*init service */
{

	register_shared_param_buf(handler);
	printer_share_mem = handler->param_buf;
	return;
}

static void printer_driver_deinit(struct service_handler *handler) /*! stop service  */
{
	return;
}

static int printer_driver_handle(struct service_handler *handler) /* invoke handler */
{
	printer_thread_function(handler->param_buf, para_vaddr, buff_vaddr);
	return 0;
}

static int printer_thread_function(unsigned long virt_addr, unsigned long para_vaddr, unsigned long buff_vaddr)
{
	int retVal = 0;
	int timeout = 0;
	struct TEEI_printer_command *command = NULL;
#ifdef DEBUG
	int i = 0;

	for (i = 0; i < 100; i++)
		printk("param_buf[%d] = %d\n", i, *((unsigned char *)virt_addr + i));

#endif

	printer_shmem_flags = SHMEM_ENABLE;

	command = (struct TEEI_printer_command *)virt_addr;
	timeout = command->args.func_write_args.timeout;

	/* up(&printer_rd_sem); */
	/* retVal = down_timeout(&printer_wr_sem, HZ*timeout); */
	if (retVal < 0) {
		union TEEI_printer_response t_response;
		t_response.value = -ETIME;

		/* retVal = down_trylock(&printer_rd_sem); */
		if (retVal == 0)
			printk("[SEM status] Printer App is not RUNNING\n");
		else if (retVal == 1)
			printk("[SEM status] BLUE Printer is not Ready\n");

		memcpy(virt_addr, (void *)&t_response, sizeof(union TEEI_printer_response));
	}

	printer_shmem_flags = SHMEM_DISABLE;

}

/*****************************************************************************/


static void secondary_invoke_fastcall(void *info)
{
	n_invoke_t_fast_call(0, 0, 0);
}


static void invoke_fastcall(void)
{
	int cpu_id = 0;

	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_invoke_fastcall, NULL, 1);
	put_online_cpus();
}

static long register_shared_param_buf(struct service_handler *handler)
{

	long retVal = 0;
	unsigned long irq_flag = 0;
	struct message_head msg_head;
	struct create_vdrv_struct msg_body;
	struct ack_fast_call_struct msg_ack;

	if (message_buff == NULL) {
		printk("[%s][%d]: There is NO command buffer!.\n", __func__, __LINE__);
		return -EINVAL;
	}


	if (handler->size > VDRV_MAX_SIZE) {
		printk("[%s][%d]: The vDrv buffer is too large, DO NOT Allow to create it.\n", __FILE__, __LINE__);
		return -EINVAL;
	}


	handler->param_buf = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(handler->size, SZ_4K)));

	if (handler->param_buf == NULL) {
		printk("[%s][%d]: kmalloc vdrv_buffer failed.\n", __FILE__, __LINE__);
		return -ENOMEM;
	}

	memset(&msg_head, 0, sizeof(struct message_head));
	memset(&msg_body, 0, sizeof(struct create_vdrv_struct));
	memset(&msg_ack, 0, sizeof(struct ack_fast_call_struct));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = FAST_CALL_TYPE;
	msg_head.child_type = FAST_CREAT_VDRV;
	msg_head.param_length = sizeof(struct create_vdrv_struct);

	msg_body.vdrv_type = handler->sysno;
	msg_body.vdrv_phy_addr = virt_to_phys(handler->param_buf);
	msg_body.vdrv_size = handler->size;

	local_irq_save(irq_flag);

	/* Notify the T_OS that there is ctl_buffer to be created. */
	memcpy(message_buff, &msg_head, sizeof(struct message_head));
	memcpy(message_buff + sizeof(struct message_head), &msg_body, sizeof(struct create_vdrv_struct));
	Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);

	/* Call the smc_fast_call */
	/* n_invoke_t_fast_call(0, 0, 0); */

	down(&(boot_sema));
	down(&(smc_lock));
	/*down(&cpu_down_lock);*/

	invoke_fastcall();

	down(&(boot_sema));
	up(&(boot_sema));

	memcpy(&msg_head, message_buff, sizeof(struct message_head));
	memcpy(&msg_ack, message_buff + sizeof(struct message_head), sizeof(struct ack_fast_call_struct));

	local_irq_restore(irq_flag);
	/*up(&cpu_down_lock);*/

	/* Check the response from T_OS. */
	if ((msg_head.message_type == FAST_CALL_TYPE) && (msg_head.child_type == FAST_ACK_CREAT_VDRV)) {
		retVal = msg_ack.retVal;

		if (retVal == 0) {
			/* printk("[%s][%d]: %s end.\n", __FILE__, __LINE__, __func__); */
			return retVal;
		}
	} else {
		retVal = -EAGAIN;
	}

	/* Release the resource and return. */
	free_pages(handler->param_buf, get_order(ROUND_UP(handler->size, SZ_4K)));
	handler->param_buf = NULL;


	return retVal;


}

static void secondary_load_func(void)
{
	Flush_Dcache_By_Area((unsigned long)boot_vfs_addr, (unsigned long)boot_vfs_addr + VFS_SIZE);
	printk("[%s][%d]: %s end.\n", __func__, __LINE__, __func__);
	n_ack_t_load_img(0, 0, 0);

	return ;
}


static void load_func(struct work_struct *entry)
{
	int cpu_id = 0;

	vfs_thread_function(boot_vfs_addr, NULL, NULL);

	down(&smc_lock);

	get_online_cpus();
	cpu_id = get_current_cpuid();
	printk("[%s][%d]current cpu id[%d] \n", __func__, __LINE__, cpu_id);
	smp_call_function_single(cpu_id, secondary_load_func, NULL, 1);
	put_online_cpus();

	return;
}


static void work_func(struct work_struct *entry)
{

	struct work_entry *md = container_of(entry, struct work_entry, work);
	int sys_call_num = md->call_no;

	if (sys_call_num == socket.sysno) {
		socket.handle(&socket);
		Flush_Dcache_By_Area(socket.param_buf, socket.param_buf + socket.size);

	} else if (sys_call_num == reetime.sysno) {
		reetime.handle(&reetime);
		Flush_Dcache_By_Area(reetime.param_buf, reetime.param_buf + reetime.size);
	} else if (sys_call_num == vfs_handler.sysno) {
		vfs_handler.handle(&vfs_handler);
		Flush_Dcache_By_Area(vfs_handler.param_buf, vfs_handler.param_buf + vfs_handler.size);
	} else if (sys_call_num == printer_driver.sysno) {
		printer_driver.handle(&printer_driver);
		Flush_Dcache_By_Area(printer_driver.param_buf, printer_driver.param_buf + printer_driver.size);
	}

	serivce_cmd_flag = 0;
	smc_flag = 1;

	return;
}

void handle_dispatch(void)
{
	if (sys_call_no == socket.sysno) {
		socket.handle(&socket);
		Flush_Dcache_By_Area(socket.param_buf, socket.param_buf + socket.size);
	} else if (sys_call_no == reetime.sysno) {
		reetime.handle(&reetime);
		Flush_Dcache_By_Area(reetime.param_buf, reetime.param_buf + reetime.size);
	} else if (sys_call_no == vfs_handler.sysno) {
		vfs_handler.handle(&vfs_handler);
		Flush_Dcache_By_Area(vfs_handler.param_buf, vfs_handler.param_buf + vfs_handler.size);
	}
}

static void do_service(void *p)
{
	while (true) {
		if (serivce_cmd_flag == 1) {
			handle_dispatch();
			serivce_cmd_flag = 0;
			smc_flag = 1;
		} else
			schedule();
	}

	return 0;
}

static irqreturn_t drivers_interrupt(int irq, void *dummy)
{
	unsigned int p0, p1, p2, p3, p4, p5, p6;

	if (p1 == 0xffff)
		generic_handle_irq(p2);
	else {
		work_ent.call_no = p1;
		INIT_WORK(&(work_ent.work), work_func);
		queue_work(secure_wq, &(work_ent.work));
	}

	return IRQ_HANDLED;
}

static int register_interrupt_handler(void)
{

	int irq_no = 100;
	int ret = request_irq(irq_no, drivers_interrupt, 0, "tz_drivers_service", (void *)register_interrupt_handler);

	if (ret)
		TERR("ERROR for request_irq %d error code : %d ", irq_no, ret);
	else
		TINFO("request irq [ %d ] OK ", irq_no);

	return 0;
}

static int init_all_service_handlers(void)
{
	socket.init = socket_init;
	socket.deinit = socket_deinit;
	socket.handle = socket_handle;
	socket.size = 0x80000;
	socket.sysno  = 1;

	reetime.init = reetime_init;
	reetime.deinit = reetime_deinit;
	reetime.handle = reetime_handle;
	reetime.size = 0x1000;
	reetime.sysno  = 7;

	vfs_handler.init = vfs_init;
	vfs_handler.deinit = vfs_deinit;
	vfs_handler.handle = vfs_handle;
	vfs_handler.size = 0x80000;
	vfs_handler.sysno = 8;

	fiq_drivers.init = fiq_drivers_init;
	fiq_drivers.deinit = fiq_drivers_deinit;
	fiq_drivers.handle = fiq_drivers_handle;
	fiq_drivers.size = 0x1000;
	fiq_drivers.sysno = 9;

	printer_driver.init = printer_driver_init;
	printer_driver.deinit = printer_driver_deinit;
	printer_driver.handle = printer_driver_handle;
	printer_driver.size = 0x1000;
	printer_driver.sysno = 10;

	/* socket.init(&socket); */
	printk("[%s][%d] begin to init reetime service!\n", __func__, __LINE__);
	reetime.init(&reetime);
	printk("[%s][%d] init reetime service successfully!\n", __func__, __LINE__);

	printk("[%s][%d] begin to init vfs service!\n", __func__, __LINE__);
	vfs_handler.init(&vfs_handler);
	printk("[%s][%d] init vfs service successfully!\n", __func__, __LINE__);

	/*
	fiq_drivers.init(&fiq_drivers);
	printer_driver.init(&printer_driver);
	*/

	return 0;
}

/***********************************************************************

 create_notify_queue:
     Create the two way notify queues between T_OS and NT_OS.

 argument:
     size    the notify queue size.

 return value:
     EINVAL  invalid argument
     ENOMEM  no enough memory
     EAGAIN  The command ID in the response is NOT accordant to the request.

 ***********************************************************************/

static long create_notify_queue(unsigned long msg_buff, unsigned long size)
{
	long retVal = 0;
	unsigned long irq_flag = 0;
	struct message_head msg_head;
	struct create_NQ_struct msg_body;
	struct ack_fast_call_struct msg_ack;


	/* Check the argument */
	if (size > MAX_BUFF_SIZE) {
		printk("[%s][%d]: The NQ buffer size is too large, DO NOT Allow to create it.\n", __FILE__, __LINE__);
		retVal = -EINVAL;
		goto return_fn;
	}

	/* Create the double NQ buffer. */
	nt_t_buffer = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(size, SZ_4K)));

	if (nt_t_buffer == NULL) {
		printk("[%s][%d]: kmalloc nt_t_buffer failed.\n", __func__, __LINE__);
		retVal =  -ENOMEM;
		goto return_fn;
	}

	t_nt_buffer = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(size, SZ_4K)));

	if (t_nt_buffer == NULL) {
		printk("[%s][%d]: kmalloc t_nt_buffer failed.\n", __func__, __LINE__);
		retVal =  -ENOMEM;
		goto Destroy_nt_t_buffer;
	}

	memset(&msg_head, 0, sizeof(struct message_head));
	memset(&msg_body, 0, sizeof(struct create_NQ_struct));
	memset(&msg_ack, 0, sizeof(struct ack_fast_call_struct));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = FAST_CALL_TYPE;
	msg_head.child_type = FAST_CREAT_NQ;
	msg_head.param_length = sizeof(struct create_NQ_struct);

	msg_body.n_t_nq_phy_addr = virt_to_phys(nt_t_buffer);
	msg_body.n_t_size = size;
	msg_body.t_n_nq_phy_addr = virt_to_phys(t_nt_buffer);
	msg_body.t_n_size = size;

	local_irq_save(irq_flag);

	/* Notify the T_OS that there are two QN to be created. */
	memcpy(msg_buff, &msg_head, sizeof(struct message_head));
	memcpy(msg_buff + sizeof(struct message_head), &msg_body, sizeof(struct create_NQ_struct));
	Flush_Dcache_By_Area((unsigned long)msg_buff, (unsigned long)msg_buff + MESSAGE_SIZE);

	down(&(boot_sema));
	down(&(smc_lock));
	/*down(&cpu_down_lock);*/

	/* Call the smc_fast_call */
	/* n_invoke_t_fast_call(0, 0, 0); */
	invoke_fastcall();


	down(&(boot_sema));
	up(&(boot_sema));

	memcpy(&msg_head, msg_buff, sizeof(struct message_head));
	memcpy(&msg_ack, msg_buff + sizeof(struct message_head), sizeof(struct ack_fast_call_struct));

	local_irq_restore(irq_flag);

	/* Check the response from T_OS. */
	/*up(&cpu_down_lock);*/

	if ((msg_head.message_type == FAST_CALL_TYPE) && (msg_head.child_type == FAST_ACK_CREAT_NQ)) {
		retVal = msg_ack.retVal;

		if (retVal == 0)
			goto return_fn;
		else
			goto Destroy_t_nt_buffer;
	} else
		retVal = -EAGAIN;

/* Release the resource and return. */
Destroy_t_nt_buffer:
	free_pages(t_nt_buffer, get_order(ROUND_UP(size, SZ_4K)));
Destroy_nt_t_buffer:
	free_pages(nt_t_buffer, get_order(ROUND_UP(size, SZ_4K)));
return_fn:
	return retVal;
}


static long create_ctl_buffer(unsigned long msg_buff, unsigned long size)
{
	long retVal = 0;
	unsigned long irq_flag = 0;
	struct message_head msg_head;
	struct create_sys_ctl_struct msg_body;
	struct ack_fast_call_struct msg_ack;


	/* Check the argument */
	if (size > MAX_BUFF_SIZE) {
		printk("[%s][%d]: The CTL buffer size is too large, DO NOT Allow to create it.\n", __FILE__, __LINE__);
		return -EINVAL;
	}

	/* Create the ctl_buffer. */
	sys_ctl_buffer = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(size, SZ_4K)));

	if (sys_ctl_buffer == NULL) {
		printk("[%s][%d]: kmalloc ctl_buffer failed.\n", __FILE__, __LINE__);
		return -ENOMEM;
	}

	memset(&msg_head, 0, sizeof(struct message_head));
	memset(&msg_body, 0, sizeof(struct create_sys_ctl_struct));
	memset(&msg_ack, 0, sizeof(struct ack_fast_call_struct));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = FAST_CALL_TYPE;
	msg_head.child_type = FAST_CREAT_SYS_CTL;
	msg_head.param_length = sizeof(struct create_sys_ctl_struct);

	msg_body.sys_ctl_phy_addr = virt_to_phys(sys_ctl_buffer);
	msg_body.sys_ctl_size = size;


	local_irq_save(irq_flag);

	/* Notify the T_OS that there is ctl_buffer to be created. */
	memcpy(msg_buff, &msg_head, sizeof(struct message_head));
	memcpy(msg_buff + sizeof(struct message_head), &msg_body, sizeof(struct create_sys_ctl_struct));
	Flush_Dcache_By_Area((unsigned long)msg_buff, (unsigned long)msg_buff + MESSAGE_SIZE);

	memcpy(&msg_head, msg_buff, sizeof(struct message_head));
	memcpy(&msg_ack, msg_buff + sizeof(struct message_head), sizeof(struct ack_fast_call_struct));


	local_irq_restore(irq_flag);

	/* Check the response from T_OS. */

	if ((msg_head.message_type == FAST_CALL_TYPE) && (msg_head.child_type == FAST_ACK_CREAT_SYS_CTL)) {
		retVal = msg_ack.retVal;

		if (retVal == 0) {
			/* printk("[%s][%d]: %s end.\n", __FILE__, __LINE__, __func__); */
			return retVal;
		}
	} else {
		retVal = -EAGAIN;
	}

	/* Release the resource and return. */
	free_pages(sys_ctl_buffer, get_order(ROUND_UP(size, SZ_4K)));
	/* printk("[%s][%d]: %s end.\n", __FILE__, __LINE__, __func__); */
	return retVal;
}


void NQ_init(unsigned long NQ_buff)
{
	memset((char *)NQ_buff, 0, NQ_BUFF_SIZE);
}

long init_nq_head(unsigned char *buffer_addr)
{
	struct NQ_head *temp_head = NULL;

	temp_head = (struct NQ_head *)buffer_addr;
	memset(temp_head, 0, NQ_BLOCK_SIZE);
	temp_head->start_index = 0;
	temp_head->end_index = 0;
	temp_head->Max_count = BLOCK_MAX_COUNT;
	Flush_Dcache_By_Area((unsigned long)temp_head, (unsigned long)temp_head + NQ_BLOCK_SIZE);
	return 0;
}

static __always_inline unsigned int get_end_index(struct NQ_head *nq_head)
{
	if (nq_head->end_index == BLOCK_MAX_COUNT)
		return 1;
	else
		return nq_head->end_index + 1;

}


int add_nq_entry(unsigned char *command_buff, int command_length, int valid_flag)
{
	struct NQ_head *temp_head = NULL;
	struct NQ_entry *temp_entry = NULL;

	temp_head = (struct NQ_head *)nt_t_buffer;

	if (temp_head->start_index == ((temp_head->end_index + 1) % temp_head->Max_count))
		return -ENOMEM;
	temp_entry = nt_t_buffer + NQ_BLOCK_SIZE + temp_head->end_index * NQ_BLOCK_SIZE;

	temp_entry->valid_flag = valid_flag;
	temp_entry->length = command_length;
	temp_entry->buffer_addr = command_buff;

	temp_head->end_index = (temp_head->end_index + 1) % temp_head->Max_count;

	Flush_Dcache_By_Area((unsigned long)nt_t_buffer, (unsigned long)(nt_t_buffer + NQ_BUFF_SIZE));
}

unsigned char *get_nq_entry(unsigned char *buffer_addr)
{
	struct NQ_head *temp_head = NULL;
	struct NQ_entry *temp_entry = NULL;

	temp_head = (struct NQ_head *)buffer_addr;

	if (temp_head->start_index == temp_head->end_index)
		return NULL;

	temp_entry = buffer_addr + NQ_BLOCK_SIZE + temp_head->start_index * NQ_BLOCK_SIZE;
	temp_head->start_index = (temp_head->start_index + 1) % temp_head->Max_count;

	Flush_Dcache_By_Area((unsigned long)buffer_addr, (unsigned long)temp_head + NQ_BUFF_SIZE);

	return temp_entry;
}




static int create_nq_buffer(void)
{
	int retVal = 0;

	retVal = create_notify_queue(message_buff, NQ_SIZE);

	if (retVal < 0) {
		printk("[%s][%d]:create_notify_queue failed with errno %d.\n", __func__, __LINE__, retVal);
		return -EINVAL;
	}

	NQ_init(t_nt_buffer);
	NQ_init(nt_t_buffer);

	init_nq_head(t_nt_buffer);
	init_nq_head(nt_t_buffer);

	return 0;
}

void add_bdrv_queue(int bdrv_id)
{
	work_ent.call_no = bdrv_id;
	INIT_WORK(&(work_ent.work), work_func);
	queue_work(secure_wq, &(work_ent.work));

	return;
}


void set_fp_command(unsigned long memory_size)
{

	printk("[%s][%d]", __func__, __LINE__);
	struct fdrv_message_head fdrv_msg_head;

	memset(&fdrv_msg_head, 0, sizeof(struct fdrv_message_head));

	fdrv_msg_head.driver_type = FP_SYS_NO;
	fdrv_msg_head.fdrv_param_length = sizeof(unsigned int);

	memcpy(fdrv_message_buff, &fdrv_msg_head, sizeof(struct fdrv_message_head));

	Flush_Dcache_By_Area((unsigned long)fdrv_message_buff, (unsigned long)fdrv_message_buff + MESSAGE_SIZE);

	return;
}

void set_sch_nq_cmd(void)
{
	struct message_head msg_head;

	memset(&msg_head, 0, sizeof(struct message_head));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = STANDARD_CALL_TYPE;
	msg_head.child_type = N_INVOKE_T_NQ;

	memcpy(message_buff, &msg_head, sizeof(struct message_head));
	Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);

	return;

}


void set_sch_load_img_cmd(void)
{
	struct message_head msg_head;

	memset(&msg_head, 0, sizeof(struct message_head));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = STANDARD_CALL_TYPE;
	msg_head.child_type = N_INVOKE_T_LOAD_TEE;

	memcpy(message_buff, &msg_head, sizeof(struct message_head));
	Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);

	return;
}


struct teei_smc_cmd *get_response_smc_cmd(void)
{
	struct NQ_entry *nq_ent = NULL;

	/* mutex_lock(&t_nt_NQ_lock); */
	nq_ent = get_nq_entry(t_nt_buffer);

	/* mutex_unlock(&t_nt_NQ_lock); */
	if (nq_ent == NULL)
		return NULL;

	return (struct teei_smc_cmd *)phys_to_virt((unsigned long)(nq_ent->buffer_addr));
}

static irqreturn_t nt_switch_irq_handler(void)
{
	unsigned long irq_flag = 0;
	struct teei_smc_cmd *command = NULL;
	struct semaphore *cmd_sema = NULL;
	struct message_head *msg_head = NULL;
	struct ack_fast_call_struct *msg_ack = NULL;

	if (boot_soter_flag == START_STATUS) {
		/* printk("[%s][%d] ==== boot_soter_flag == START_STATUS ========\n", __func__, __LINE__); */
		INIT_WORK(&(load_ent.work), load_func);
		queue_work(secure_wq, &(load_ent.work));
		up(&smc_lock);

		return IRQ_HANDLED;
	} else {
		msg_head = (struct message_head *)message_buff;

		if (FAST_CALL_TYPE == msg_head->message_type) {
			/* printk("[%s][%d] ==== FAST_CALL_TYPE ACK ========\n", __func__, __LINE__); */
			return IRQ_HANDLED;
		} else if (STANDARD_CALL_TYPE == msg_head->message_type) {
			/* Get the smc_cmd struct */
			if (msg_head->child_type == VDRV_CALL_TYPE) {
				/* printk("[%s][%d] ==== VDRV_CALL_TYPE ========\n", __func__, __LINE__); */
				work_ent.call_no = msg_head->param_length;
				INIT_WORK(&(work_ent.work), work_func);
				queue_work(secure_wq, &(work_ent.work));
				up(&smc_lock);
#if 0
			} else if (msg_head->child_type == FDRV_ACK_TYPE) {
				/* printk("[%s][%d] ==== FDRV_ACK_TYPE ========\n", __func__, __LINE__); */
				/*
				if(forward_call_flag == GLSCH_NONE)
					forward_call_flag = GLSCH_NEG;
				else
					forward_call_flag = GLSCH_NONE;
				*/
				up(&boot_sema);
				up(&smc_lock);
#endif
			} else {
				/* printk("[%s][%d] ==== STANDARD_CALL_TYPE ACK ========\n", __func__, __LINE__); */

				forward_call_flag = GLSCH_NONE;
				command = get_response_smc_cmd();

				if (NULL == command)
					return IRQ_NONE;

				/* Get the semaphore */
				cmd_sema = (struct semaphore *)(command->teei_sema);

				/* Up the semaphore */
				up(cmd_sema);
				up(&smc_lock);
			}

			return IRQ_HANDLED;
		} else {
			printk("[%s][%d] ==== Unknown IRQ ========\n", __func__, __LINE__);
			return IRQ_NONE;
		}
	}
}

int register_switch_irq_handler(void)
{
	int retVal = 0;
	/* retVal = request_irq(SWITCH_IRQ, nt_switch_irq_handler, 0,
			"tz_drivers_service", (void *)register_switch_irq_handler); */

	retVal = request_irq(SWITCH_IRQ, nt_switch_irq_handler, 0, "tz_drivers_service", NULL);

	if (retVal)
		printk("ERROR for request_irq %d error code : %d.\n", SWITCH_IRQ, retVal);
	else
		printk("request irq [ %d ] OK.\n", SWITCH_IRQ);

	return 0;

}


#if 0
static int create_vDrv_vfs_buffer(void)
{
	int retVal = 0;

	retVal = create_vdrv_buffer(message_buff, TEEI_VFS_TYPE, VFS_BUFF_SIZE, &vDrv_vfs_buff);

	if (retVal < 0)
		printk("[%s][%d]:create_vdrv_buffer failed with errno %d.\n", __func__, __LINE__, retVal);

	return retVal;
}
#endif

static int start_teei_service(void)
{
	/* kernel_thread(do_service, NULL, 0); */
	return 0;
}

struct init_cmdbuf_struct {
	unsigned long phy_addr;
	unsigned long fdrv_phy_addr;
	unsigned long bdrv_phy_addr;
	unsigned long tlog_phy_addr;
};

struct init_cmdbuf_struct init_cmdbuf_entry;


static void secondary_init_cmdbuf(void *info)
{
	struct init_cmdbuf_struct *cd = (struct init_cmdbuf_struct *)info;

	/* with a rmb() */
	rmb();

		printk("[%s][%d] message = %lx,  fdrv message = %lx, bdrv_message = %lx, tlog_message = %lx.\n", __func__, __LINE__,
		(unsigned long)cd->phy_addr, (unsigned long)cd->fdrv_phy_addr,
		(unsigned long)cd->bdrv_phy_addr, (unsigned long)cd->tlog_phy_addr);

	n_init_t_fc_buf(cd->phy_addr, cd->fdrv_phy_addr, 0);

	n_init_t_fc_buf(cd->bdrv_phy_addr, cd->tlog_phy_addr, 0);


	/* with a wmb() */
	wmb();
}


static void init_cmdbuf(unsigned long phy_address, unsigned long fdrv_phy_address,
			unsigned long bdrv_phy_address, unsigned long tlog_phy_address)
{
	int cpu_id = 0;

	init_cmdbuf_entry.phy_addr = phy_address;
	init_cmdbuf_entry.fdrv_phy_addr = fdrv_phy_address;
	init_cmdbuf_entry.bdrv_phy_addr = bdrv_phy_address;
	init_cmdbuf_entry.tlog_phy_addr = tlog_phy_address;

	/* with a wmb() */
	wmb();

	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_init_cmdbuf, (void *)(&init_cmdbuf_entry), 1);
	put_online_cpus();

	/* with a rmb() */
	rmb();
}


long create_cmd_buff(void)
{
	unsigned long irq_status = 0;

	message_buff =  (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));

	if (message_buff == NULL) {
		printk("[%s][%d] Create message buffer failed!\n", __FILE__, __LINE__);
		return -ENOMEM;
	}

	fdrv_message_buff =  (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));

	if (fdrv_message_buff == NULL) {
		printk("[%s][%d] Create fdrv message buffer failed!\n", __FILE__, __LINE__);
		free_pages(message_buff, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
		return -ENOMEM;
	}

	bdrv_message_buff = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
	if (bdrv_message_buff == NULL) {
		printk("[%s][%d] Create bdrv message buffer failed!\n", __FILE__, __LINE__);
		free_pages(message_buff, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
		free_pages(fdrv_message_buff, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
		return -ENOMEM;
	}

	tlog_message_buff = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
	if (tlog_message_buff == NULL) {
		printk("[%s][%d] Create tlog message buffer failed!\n", __FILE__, __LINE__);
		free_pages(message_buff, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
		free_pages(fdrv_message_buff, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
		free_pages(bdrv_message_buff, get_order(ROUND_UP(MESSAGE_LENGTH, SZ_4K)));
		return -ENOMEM;
		}

	/* smc_call to notify SOTER the share memory(message_buff) */

	/* n_init_t_fc_buf((unsigned long)virt_to_phys(message_buff), 0, 0); */
			printk("[%s][%d] message = %lx,  fdrv message = %lx, bdrv_message = %lx, tlog_message = %lx\n", __func__, __LINE__,
			(unsigned long)virt_to_phys(message_buff),
			(unsigned long)virt_to_phys(fdrv_message_buff),
			(unsigned long)virt_to_phys(bdrv_message_buff),
			(unsigned long)virt_to_phys(tlog_message_buff));

	init_cmdbuf((unsigned long)virt_to_phys(message_buff), (unsigned long)virt_to_phys(fdrv_message_buff),
			(unsigned long)virt_to_phys(bdrv_message_buff), (unsigned long)virt_to_phys(tlog_message_buff));

	return 0;
}

unsigned long create_fp_fdrv(int buff_size)
{
	long retVal = 0;
	unsigned long irq_flag = 0;
	unsigned long temp_addr = 0;
	struct message_head msg_head;
	struct create_fdrv_struct msg_body;
	struct ack_fast_call_struct msg_ack;

	if (message_buff == NULL) {
		printk("[%s][%d]: There is NO command buffer!.\n", __func__, __LINE__);
		return NULL;
	}


	if (buff_size > VDRV_MAX_SIZE) {
		printk("[%s][%d]: FP Drv buffer is too large, Can NOT create it.\n", __FILE__, __LINE__);
		return NULL;
	}


	temp_addr = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(buff_size, SZ_4K)));

	if (temp_addr == NULL) {
		printk("[%s][%d]: kmalloc fp drv buffer failed.\n", __FILE__, __LINE__);
		return NULL;
	}

	memset(&msg_head, 0, sizeof(struct message_head));
	memset(&msg_body, 0, sizeof(struct create_fdrv_struct));
	memset(&msg_ack, 0, sizeof(struct ack_fast_call_struct));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = FAST_CALL_TYPE;
	msg_head.child_type = FAST_CREAT_FDRV;
	msg_head.param_length = sizeof(struct create_fdrv_struct);

	msg_body.fdrv_type = FP_SYS_NO;
	msg_body.fdrv_phy_addr = virt_to_phys(temp_addr);
	msg_body.fdrv_size = buff_size;

	local_irq_save(irq_flag);

	/* Notify the T_OS that there is ctl_buffer to be created. */
	memcpy(message_buff, &msg_head, sizeof(struct message_head));
	memcpy(message_buff + sizeof(struct message_head), &msg_body, sizeof(struct create_fdrv_struct));
	Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);

	/* Call the smc_fast_call */
	/* n_invoke_t_fast_call(0, 0, 0); */
	down(&(smc_lock));
	/*down(&cpu_down_lock);*/
	down(&(boot_sema));

	invoke_fastcall();

	down(&(boot_sema));
	up(&(boot_sema));

	memcpy(&msg_head, message_buff, sizeof(struct message_head));
	memcpy(&msg_ack, message_buff + sizeof(struct message_head), sizeof(struct ack_fast_call_struct));

	local_irq_restore(irq_flag);

	/*up(&cpu_down_lock);*/
	/* Check the response from T_OS. */
	if ((msg_head.message_type == FAST_CALL_TYPE) && (msg_head.child_type == FAST_ACK_CREAT_FDRV)) {
		retVal = msg_ack.retVal;

		if (retVal == 0) {
			/* printk("[%s][%d]: %s end.\n", __func__, __LINE__, __func__); */
			return temp_addr;
		}
	} else
		retVal = NULL;

	/* Release the resource and return. */
	free_pages(temp_addr, get_order(ROUND_UP(buff_size, SZ_4K)));

	printk("[%s][%d]: %s failed!\n", __func__, __LINE__, __func__);
	return retVal;
}

int teei_service_init(void)
{
	/**
	 * register interrupt handler
	 */
	/* register_switch_irq_handler(); */

	printk("[%s][%d] begin to create nq buffer!\n", __func__, __LINE__);
	create_nq_buffer();
	printk("[%s][%d] end of creating nq buffer!\n", __func__, __LINE__);

	if (soter_error_flag == 1)
		return -1;

	printk("[%s][%d] begin to create fp buffer!\n", __func__, __LINE__);
	fp_buff_addr = create_fp_fdrv(FP_BUFF_SIZE);

	if (soter_error_flag == 1)
		return -1;

	/**
	 * init service handler
	 */
	init_all_service_handlers();

	if (soter_error_flag == 1)
		return -1;
	/**
	 * start service thread
	 */
	start_teei_service();

	/**
	 * Create Work Queue
	 */
	/* secure_wq = create_workqueue("Secure Call"); */

	return 0;
}
