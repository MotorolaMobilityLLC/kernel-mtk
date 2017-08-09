
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
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <mach/mt_clkmgr.h>
#include <linux/of.h>
#include <linux/of_irq.h>
/*#include <mach/eint.h>*/
/*#include <mach/eint_drv.h>*/
#include <linux/irqchip/mt-eic.h>
#include <linux/compat.h>
#include <linux/freezer.h>
#include <linux/cpumask.h>
#include "tpd.h"
#include <linux/delay.h>

#include <linux/cpu.h>

#include "teei_client.h"
#include "teei_common.h"
#include "teei_id.h"
#include "teei_debug.h"
#include "smc_id.h"
/* #include "TEEI.h" */
#include "tz_service.h"
#include "nt_smc_call.h"
#include "teei_client_main.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/completion.h>

#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#define MAX_BUFF_SIZE			(4096)
#define NQ_SIZE					(4096)
#define CTL_BUFF_SIZE			(4096)
#define VDRV_MAX_SIZE			(0x80000)
#define NQ_VALID				1

#define TEEI_VFS_NUM			0x8

#define F_CREATE_NQ_ID			0x01
#define F_CREATE_CTL_ID			0x02
#define F_CREATE_VDRV_ID		0x04

#define MESSAGE_SIZE			(4096)
#define VALID_TYPE				(1)
#define INVALID_TYPE			(0)

#define FAST_CALL_TYPE			(0x100)
#define STANDARD_CALL_TYPE		(0x200)
#define TYPE_NONE				(0x300)

#define FAST_CREAT_NQ			(0x40)
#define FAST_ACK_CREAT_NQ		(0x41)
#define FAST_CREAT_VDRV			(0x42)
#define FAST_ACK_CREAT_VDRV		(0x43)
#define FAST_CREAT_SYS_CTL		(0x44)
#define FAST_ACK_CREAT_SYS_CTL	(0x45)

#define NQ_CALL_TYPE            (0x60)
#define VDRV_CALL_TYPE          (0x61)
#define SCHD_CALL_TYPE			(0x62)
#define FDRV_ACK_TYPE           (0x63)

#define STD_INIT_CONTEXT		(0x80)
#define	STD_ACK_INIT_CONTEXT	(0x81)
#define STD_OPEN_SESSION		(0x82)
#define STD_ACK_OPEN_SESSION	(0x83)
#define STD_INVOKE_CMD			(0x84)
#define STD_ACK_INVOKE_CMD		(0x85)
#define STD_CLOSE_SESSION		(0x86)
#define STD_ACK_CLOSE_SESSION	(0x87)
#define STD_CLOSE_CONTEXT		(0x88)
#define STD_ACK_CLOSE_CONTEXT	(0x89)

#define GLSCH_NEG				(0x03)
#define GLSCH_NONE				(0x00)
#define GLSCH_LOW				(0x01)
#define GLSCH_HIGH				(0x02)
#define GLSCH_FOR_SOTER			(0x04)

#define BOOT_IRQ				(283+50)
#define SCHED_IRQ				(284+50)
#define SOTER_IRQ				(285+50)
#define FP_ACK_IRQ				(287+50)
#define BDRV_IRQ				(278+50)
#define TEEI_LOG_IRQ				(277+50)
#define SOTER_ERROR_IRQ				(276+50)

#define TLOG_CONTEXT_LEN			(300)
#define TLOG_MAX_CNT				(50)

#define TLOG_UNUSE				(0)
#define TLOG_INUSE				(1)

#if 0
/******************************
 * Message header
 ******************************/

struct message_head {
	unsigned int invalid_flag;
	unsigned int message_type;
	unsigned int child_type;
	unsigned int param_length;
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
struct std_msg_struct {
	unsigned int direction_flag;
	unsigned int meg_type;
};
#endif

struct fdrv_call_struct {
        int fdrv_call_type;
        int fdrv_call_buff_size;
        int retVal;
};


#define CAPI_CALL       0x01
#define FDRV_CALL       0x02
#define BDRV_CALL       0x03
#define SCHED_CALL      0x04

#define FP_SYS_NO       100

#define VFS_SYS_NO      0x08
#define REETIME_SYS_NO  0x07 

unsigned long cpu_notify_flag = 0;
static  int current_cpu_id = 0x00;

static int tz_driver_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu);
static struct notifier_block tz_driver_cpu_notifer = {
	.notifier_call = tz_driver_cpu_callback,
};

struct smc_call_struct {
	unsigned long local_cmd;
	u32 teei_cmd_type;
	u32 dev_file_id;
	u32 svc_id;
	u32 cmd_id;
	u32 context;
	u32 enc_id;
	void *cmd_buf;
	size_t cmd_len;
	void *resp_buf;
	size_t resp_len;
	void *meta_data;
	void *info_data;
	size_t info_len;
	int *ret_resp_len;
	int *error_code;
	struct semaphore *psema;
	int retVal;
};

struct smc_call_struct smc_call_entry;

#define printk(fmt, args...) printk("\033[;34m[TEEI][TZDriver]"fmt"\033[0m", ##args)

asmlinkage long sys_setpriority(int which, int who, int niceval);
asmlinkage long sys_getpriority(int which, int who);
static struct teei_contexts_head {
	u32 dev_file_cnt;
	struct list_head context_list;
	struct rw_semaphore teei_contexts_sem;
} teei_contexts_head;

struct teei_shared_mem_head {
	int shared_mem_cnt;
	struct list_head shared_mem_list;
};

struct tlog_struct {
	int valid;
	struct work_struct work;
	char context[TLOG_CONTEXT_LEN];
};

#if 0
static int sub_pid;
#else
static struct task_struct *teei_fastcall_task;
static struct task_struct *teei_switch_task;

DEFINE_KTHREAD_WORKER(ut_fastcall_worker);

#endif
static 	struct cpumask mask = { CPU_BITS_NONE };

int switch_enable_flag = 0;
int forward_call_flag = 0;
int irq_call_flag = 0;
int fp_call_flag = 0;

unsigned long sys_ctl_buffer = NULL;
unsigned long vdrv_buffer = NULL;

unsigned long teei_config_flag = 0;
static struct tlog_struct tlog_ent[TLOG_MAX_CNT];

unsigned int soter_error_flag = 0;

DECLARE_COMPLETION(global_down_lock);
EXPORT_SYMBOL_GPL(global_down_lock);

extern int add_work_entry(int work_type, unsigned long buff);
/*
 * structures and MACROs for NQ buffer
 */
#define NQ_BUFF_SIZE	(4096)
#define NQ_BLOCK_SIZE	(32)
#define BLOCK_MAX_COUNT	(NQ_BUFF_SIZE / NQ_BLOCK_SIZE - 1)


#define STD_NQ_ACK_ID	0x01

#define TEE_NAME_SIZE	(255)
#define START_STATUS	(0)
#define END_STATUS		(1)
#define VFS_SIZE		(512 * 1024)
#define FP_BUFF_SIZE		(512 * 1024)

static DEFINE_MUTEX(nt_t_NQ_lock);
static DEFINE_MUTEX(t_nt_NQ_lock);


struct semaphore smc_lock;
//struct semaphore global_down_lock;
struct semaphore api_lock;
static int print_context(void);

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

static void *tz_malloc(size_t size, int flags)
{
	void *ptr = kmalloc(size, flags|GFP_ATOMIC);
	return ptr;
}
void *tz_malloc_shared_mem(size_t size, int flags)
{
	return (void *) __get_free_pages(flags, get_order(ROUND_UP(size, SZ_4K)));
}
void tz_free_shared_mem(void *addr, size_t size)
{
	free_pages((unsigned long)addr, get_order(ROUND_UP(size, SZ_4K)));
}

static struct class *driver_class;
static dev_t teei_client_device_no;
static struct cdev teei_client_cdev;

/* static u32 cacheline_size; */
unsigned long device_file_cnt = 0;

static struct teei_smc_cdata teei_smc_cd[NR_CPUS];

struct semaphore boot_sema;
struct semaphore fp_lock;
unsigned long boot_vfs_addr;
unsigned long boot_soter_flag;

extern struct mutex pm_mutex;

#if 1

/*
 * structures for context & session management
 */
struct teei_context {
	unsigned long cont_id;			/* ID */
	char tee_name[TEE_NAME_SIZE];		/* Name */
	unsigned long sess_cnt;			/* session counter */
	unsigned long shared_mem_cnt;		/* share memory counter */
	struct list_head link;			/* link list for teei_context */
	struct list_head sess_link;			/* link list for the sessions of this context */
	struct list_head shared_mem_list;	/* link list for the share memory of this context */
	struct semaphore cont_lock;
};


struct teei_session {
	int sess_id;			/* ID */
	struct teei_context *parent_cont;	/* the teei_context pointer of this session */
	struct list_head link;		/* link list for teei_session */
	struct list_head encode_list;	/* link list for the encode of this session */
	struct list_head shared_mem_list;	/* link list for the share memory of this session */
};

struct teei_context *teei_create_context(int dev_count)
{
	struct teei_context *cont = NULL;
	cont = kmalloc(sizeof(struct teei_context), GFP_KERNEL);
	if (cont == NULL)
		return NULL;

	cont->cont_id = dev_count;
	cont->sess_cnt = 0;
	memset(cont->tee_name, 0, TEE_NAME_SIZE);
	INIT_LIST_HEAD(&(cont->link));
	INIT_LIST_HEAD(&(cont->sess_link));
	INIT_LIST_HEAD(&(cont->shared_mem_list));

	list_add(&(cont->link), &(teei_contexts_head.context_list));
	teei_contexts_head.dev_file_cnt++;
	return cont;
}

struct teei_session *teei_create_session(struct teei_context *cont)
{
	struct teei_session *sess = NULL;
	sess = kmalloc(sizeof(struct teei_session), GFP_KERNEL);
	if (sess == NULL)
		return NULL;

	sess->sess_id = (unsigned long)sess;
	sess->parent_cont = cont;
	INIT_LIST_HEAD(&(sess->link));
	INIT_LIST_HEAD(&(sess->encode_list));
	INIT_LIST_HEAD(&(sess->shared_mem_list));

	list_add(&(sess->link), &(cont->sess_link));
	cont->sess_cnt = cont->sess_cnt + 1;

	return sess;
}

/*add by microtrust*/
static unsigned int get_master_cpu_id(unsigned int gic_irqs)
{
	unsigned int i;
	i = gic_irqs >> 2;
	unsigned long gic_base = 0x10231000 + 0x800 + i * 4 ;
	void __iomem *dist_base = (void __iomem *)gic_base;


	unsigned int  target_id = readl_relaxed(dist_base);
	target_id = target_id >> ((gic_irqs & 0x03) * 8);
	target_id = target_id & 0xff;
	return target_id;
}
/*****************************************/
/*extern u32 get_irq_target_microtrust(void);*/
 int get_current_cpuid(void)
{
	return current_cpu_id;
}




static void secondary_nt_sched_t(void *info)
{
	nt_sched_t();
}


void nt_sched_t_call(void)
{
#if 0
	nt_sched_t();
#else
	int cpu_id = 0;


#if 0
	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_nt_sched_t, NULL, 1);
	put_online_cpus();
#else
	int retVal = 0;

        retVal = add_work_entry(SCHED_CALL, NULL);
        if (retVal != 0) {
                printk("[%s][%d] add_work_entry function failed!\n", __func__, __LINE__);
        }
#endif

#endif

	return;
}



/*********************************************************************************
 *  global_fn				Global schedule thread function.
 *
 *  switch_enable_flag:		Waiting for the n_switch_to_t_os_stage2 finished
 *  forward_call_flag:		Forward direction call flag
 *
 *********************************************************************************/


int global_fn(void)
{
	int retVal = 0;

	/* forward_call_flag = GLSCH_NONE; */

	while (1) {
#if 0
		if ((forward_call_flag == GLSCH_NONE) && (fp_call_flag == GLSCH_NONE)) {
			set_freezable();
			set_current_state(TASK_INTERRUPTIBLE);
		} else {
			set_nofreezable();
			set_current_state(TASK_UNINTERRUPTIBLE);
		}
#else
		set_freezable();
		set_current_state(TASK_INTERRUPTIBLE);
#endif

		if (teei_config_flag == 1) {
			retVal = wait_for_completion_interruptible(&global_down_lock);
			if (retVal == -ERESTARTSYS) {
				printk("[%s][%d]*********down &global_down_lock failed *****************\n", __func__, __LINE__ );
				continue;
			}
		}

		/* down(&smc_lock); */
		retVal = down_interruptible(&smc_lock);
		if (retVal != 0) {
			printk("[%s][%d]*********down &smc_lock failed *****************\n", __func__, __LINE__ );
			complete(&global_down_lock);
			continue;
		}
		if (forward_call_flag == GLSCH_FOR_SOTER) {
			forward_call_flag = GLSCH_NONE;
			msleep(10);
			nt_sched_t_call();
		} else if (irq_call_flag == GLSCH_HIGH) {
			/* printk("[%s][%d]**************************\n", __func__, __LINE__ ); */
			irq_call_flag = GLSCH_NONE;
			nt_sched_t_call();
			/*msleep_interruptible(10);*/
		} else if (fp_call_flag == GLSCH_HIGH) {
			/* printk("[%s][%d]**************************\n", __func__, __LINE__ ); */
			if (teei_vfs_flag == 0) {
				nt_sched_t_call();
			} else {
				up(&smc_lock);
				msleep_interruptible(1);
			}
		} else if (forward_call_flag == GLSCH_LOW) {
			/* printk("[%s][%d]**************************\n", __func__, __LINE__ ); */
			if (teei_vfs_flag == 0)	{
				nt_sched_t_call();
			} else {
				up(&smc_lock);
				msleep_interruptible(1);
			}
		} else {
			/* printk("[%s][%d]**************************\n", __func__, __LINE__ ); */
			up(&smc_lock);
			msleep_interruptible(1);
		}
	}
}

struct teei_encode {
	struct list_head head;
	int encode_id;
	void *ker_req_data_addr;
	void *ker_res_data_addr;
	u32  enc_req_offset;
	u32  enc_res_offset;
	u32  enc_req_pos;
	u32  enc_res_pos;
	u32  dec_res_pos;
	u32  dec_offset;
	struct teei_encode_meta *meta;
};

struct teei_shared_mem {
	struct list_head head;
	struct list_head s_head;
	void *index;
	void *k_addr;
	void *u_addr;
	u32  len;
};

static int teei_client_prepare_encode(void *private_data,
		struct teei_client_encode_cmd *enc,
		struct teei_encode **penc_context,
		struct teei_session **psession);

/******************************************************************************/
static void post_smc_work(u32 cmd_addr);
static void init_smc_work(void);

struct tz_work_entry {
	u32 cmd_addr;
	struct work_struct work;
};

static struct work_queue *tz_wq;
static struct tz_work_entry tz_work_ent;
struct semaphore tz_sem;

static void tz_work_func(struct work_struct *entry)
{
	struct tz_work_entry *md = container_of(entry, struct tz_work_entry, work);
#if 0
	/* disable touchscreen interrupt */
	disable_irq_nosync(350);
	printk("=====TZ DISABLE================\n");
	n_send_mem_info_to_ta(md->cmd_addr, sizeof(struct teei_smc_cmd));
	/* enable touchscreen interrupt */
	enable_irq(350);
	printk("======TZ ENABLE================\n");
#endif
	up(&tz_sem);
}

static void post_smc_work(u32 cmd_addr)
{
	tz_work_ent.cmd_addr = cmd_addr;
	INIT_WORK(&(tz_work_ent.work), tz_work_func);
	queue_work(tz_wq, &(tz_work_ent.work));
	down(&tz_sem);
	wait_for_service_done();
}
static void init_smc_work(void)
{
	sema_init(&tz_sem, 0);
	tz_wq = create_workqueue("TZ SMC WORK");
}

/**
 * @brief
 *
 * @param cmd_addr phys address
 *
 * @return
 */

static u32 _teei_smc(u32 cmd_addr, int size, int valid_flag)
{
	int i = 0;
#if 1
	/* disable touchscreen interrupt */
	add_nq_entry(cmd_addr, size, valid_flag);
	set_sch_nq_cmd();
	Flush_Dcache_By_Area((unsigned long)t_nt_buffer, (unsigned long)t_nt_buffer + 0x1000);
	n_invoke_t_nq(0, 0, 0);
#else
	post_smc_work(cmd_addr);
#endif
	return 0;
}

/**
 * @brief
 *
 * @param teei_smc handler for secondary cores
 *
 * @return
 */
static void secondary_teei_smc_handler(void *info)
{
	struct teei_smc_cdata *cd = (struct teei_smc_cdata *)info;
	/* with a rmb() */
	rmb();
	TDEBUG("secondary teei smc handler...");
	cd->ret_val = _teei_smc(cd->cmd_addr, cd->size, cd->valid_flag);
	/* with a wmb() */
	wmb();
	TDEBUG("done smc on primary ");
}

/**
 * @brief
 *
 * @param This function takes care of posting the smc to the
 *        primary core
 *
 * @return
 */
static u32 post_teei_smc(int cpu_id, u32 cmd_addr, int size, int valid_flag)
{
	struct teei_smc_cdata *cd = &teei_smc_cd[cpu_id];
	TDEBUG("Post from secondary ...");
	cd->cmd_addr = cmd_addr;
	cd->size = size;
	cd->valid_flag = valid_flag;
	cd->ret_val  = 0;
	/* with a wmb() */
	wmb();

	get_online_cpus();
	smp_call_function_single(cpu_id, secondary_teei_smc_handler, (void *)cd, 1);
	put_online_cpus();

	/* with a rmb() */
	rmb();
	TDEBUG("completed smc on secondary ");
	return cd->ret_val;
}

/**
 * @brief
 *
 * @param teei_smc wrapper to handle the multi core case
 *
 * @return
 */
static u32 teei_smc(u32 cmd_addr, int size, int valid_flag)
{
#if 0
	int cpu_id = smp_processor_id();
	/* int cpu_id = raw_smp_processor_id(); */

	if (cpu_id != 0) {
		/* with mb */
		mb();
		printk("[%s][%d]\n", __func__, __LINE__);
		return post_teei_smc(0, cmd_addr, size, valid_flag); /* post it to primary */
	} else {
		printk("[%s][%d]\n", __func__, __LINE__);
		return _teei_smc(cmd_addr, size, valid_flag); /* called directly on primary core */
	}
#else
	return _teei_smc(cmd_addr, size, valid_flag);
	/* return post_teei_smc(0, cmd_addr, size, valid_flag); */
#endif
}

/**
 * @brief
 *      call smc
 * @param svc_id  - service identifier
 * @param cmd_id  - command identifier
 * @param context - session context
 * @param enc_id - encoder identifier
 * @param cmd_buf - command buffer
 * @param cmd_len - command buffer length
 * @param resp_buf - response buffer
 * @param resp_len - response buffer length
 * @param meta_data
 * @param ret_resp_len
 *
 * @return
 */
int __teei_smc_call(unsigned long local_smc_cmd,
		u32 teei_cmd_type,
		u32 dev_file_id,
		u32 svc_id,
		u32 cmd_id,
		u32 context,
		u32 enc_id,
		const void *cmd_buf,
		size_t cmd_len,
		void *resp_buf,
		size_t resp_len,
		const void *meta_data,
		const void *info_data,
		size_t info_len,
		int *ret_resp_len,
		int *error_code,
		struct semaphore *psema)
{
	int ret = 50;
	void *smc_cmd_phys = 0;
	struct teei_smc_cmd *smc_cmd = NULL;

	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_context *temp_cont = NULL;

#if 0
	smc_cmd = (struct teei_smc_cmd *)tz_malloc_shared_mem(sizeof(struct teei_smc_cmd), GFP_KERNEL);

	if (!smc_cmd) {
		TERR("tz_malloc failed for smc command");
		ret = -ENOMEM;
		goto out;
	}
#else
	smc_cmd = (struct teei_smc_cmd *)local_smc_cmd;
#endif

	if (ret_resp_len)
		*ret_resp_len = 0;

	smc_cmd->teei_cmd_type = teei_cmd_type;
	smc_cmd->dev_file_id = dev_file_id;
	smc_cmd->src_id = svc_id;
	smc_cmd->src_context = task_tgid_vnr(current);

	smc_cmd->id = cmd_id;
	smc_cmd->context = context;
	smc_cmd->enc_id = enc_id;
	smc_cmd->src_context = task_tgid_vnr(current);

	smc_cmd->req_buf_len = cmd_len;
	smc_cmd->resp_buf_len = resp_len;
	smc_cmd->info_buf_len = info_len;
	smc_cmd->ret_resp_buf_len = 0;

	if (NULL == psema)
		return -EINVAL;
	else
		smc_cmd->teei_sema = psema;

	if (cmd_buf != NULL) {
		smc_cmd->req_buf_phys = virt_to_phys((void *)cmd_buf);
		Flush_Dcache_By_Area((unsigned long)cmd_buf, (unsigned long)cmd_buf + cmd_len);
		Flush_Dcache_By_Area((unsigned long)&cmd_buf, (unsigned long)&cmd_buf + sizeof(int));
	} else
		smc_cmd->req_buf_phys = 0;

	if (resp_buf) {
		smc_cmd->resp_buf_phys = virt_to_phys((void *)resp_buf);
		Flush_Dcache_By_Area((unsigned long)resp_buf, (unsigned long)resp_buf + resp_len);
	} else
		smc_cmd->resp_buf_phys = 0;

	if (meta_data) {
		smc_cmd->meta_data_phys = virt_to_phys(meta_data);
		Flush_Dcache_By_Area((unsigned long)meta_data, (unsigned long)meta_data +
				sizeof(struct teei_encode_meta) * (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));
	} else
		smc_cmd->meta_data_phys = 0;

	if (info_data) {
		smc_cmd->info_buf_phys = virt_to_phys(info_data);
		Flush_Dcache_By_Area((unsigned long)info_data, (unsigned long)info_data + info_len);
	} else
		smc_cmd->info_buf_phys = 0;

	smc_cmd_phys = virt_to_phys((void *)smc_cmd);

	smc_cmd->error_code = 0;

	Flush_Dcache_By_Area((unsigned long)smc_cmd, (unsigned long)smc_cmd + sizeof(struct teei_smc_cmd));
	Flush_Dcache_By_Area((unsigned long)&smc_cmd, (unsigned long)&smc_cmd + sizeof(int));

	/* down(&smc_lock); */

	list_for_each_entry(temp_cont,
			&teei_contexts_head.context_list,
			link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_shared_mem,
					&temp_cont->shared_mem_list,
					head) {
				Flush_Dcache_By_Area((unsigned long)temp_shared_mem->k_addr, (unsigned long)temp_shared_mem->k_addr + temp_shared_mem->len);
			}
		}
	}

	forward_call_flag = GLSCH_LOW;
	ret = teei_smc(smc_cmd_phys, sizeof(struct teei_smc_cmd), NQ_VALID);

	/* down(psema); */

#if 0
	Invalidate_Dcache_By_Area((unsigned long)smc_cmd, (unsigned long)smc_cmd + sizeof(struct teei_smc_cmd));

	if (cmd_buf)
		Invalidate_Dcache_By_Area((unsigned long)cmd_buf, (unsigned long)cmd_buf + cmd_len);

	if (resp_buf)
		Invalidate_Dcache_By_Area((unsigned long)resp_buf, (unsigned long)resp_buf + resp_len);

	if (meta_data)
		Invalidate_Dcache_By_Area((unsigned long)meta_data, (unsigned long)meta_data +
				sizeof(struct teei_encode_meta) * (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));

	if (info_data)
		Invalidate_Dcache_By_Area((unsigned long)info_data, (unsigned long)info_data + info_len);

	if (error_code)
		*error_code = smc_cmd->error_code;

	if (ret) {
		printk("[%s][%d] smc_call returns error!\n", __func__, __LINE__);
		goto out;
	}

	if (ret_resp_len)
		*ret_resp_len = smc_cmd->ret_resp_buf_len;

out:
	if (smc_cmd)
		tz_free_shared_mem(smc_cmd, sizeof(struct teei_smc_cmd));
	return ret;
#endif
	return 0;
}

#if 1
static void secondary_teei_smc_call(void *info)
{
	struct smc_call_struct *cd = (struct smc_call_struct *)info;
	/* with a rmb() */
	rmb();
	TDEBUG("secondary teei smc call...");

	if (cd->cmd_buf != NULL) {
	/*	Flush_Dcache_By_Area((unsigned long)(cd->cmd_buf), (unsigned long)(cd->cmd_buf) + cd->cmd_len);*/
	}

	if (cd->resp_buf != NULL) {
	/*	Flush_Dcache_By_Area((unsigned long)(cd->resp_buf), (unsigned long)(cd->resp_buf) + cd->resp_len);*/
	}

	if (cd->meta_data != NULL) {
	/*	Flush_Dcache_By_Area((unsigned long)(cd->meta_data), (unsigned long)(cd->meta_data) +
			sizeof(struct teei_encode_meta) * (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));*/
		}

		if (cd->info_data != NULL) {
		/*        Flush_Dcache_By_Area((unsigned long)(cd->info_data), (unsigned long)(cd->info_data) + cd->info_len);*/
		}

	cd->retVal = __teei_smc_call(cd->local_cmd,
			cd->teei_cmd_type,
			cd->dev_file_id,
			cd->svc_id,
			cd->cmd_id,
			cd->context,
			cd->enc_id,
			cd->cmd_buf,
			cd->cmd_len,
			cd->resp_buf,
			cd->resp_len,
			cd->meta_data,
			cd->info_data,
			cd->info_len,
			cd->ret_resp_len,
			cd->error_code,
			cd->psema);
	/* with a wmb() */
	wmb();
	TDEBUG("done smc on primary ");
}

int teei_smc_call(u32 teei_cmd_type,
		u32 dev_file_id,
		u32 svc_id,
		u32 cmd_id,
		u32 context,
		u32 enc_id,
		const void *cmd_buf,
		size_t cmd_len,
		void *resp_buf,
		size_t resp_len,
		const void *meta_data,
		const void *info_data,
		size_t info_len,
		int *ret_resp_len,
		int *error_code,
		struct semaphore *psema)
{
	int cpu_id = 0;
	int retVal = 0;

	struct teei_smc_cmd *local_smc_cmd = (struct teei_smc_cmd *)tz_malloc_shared_mem(sizeof(struct teei_smc_cmd), GFP_KERNEL);
	if (local_smc_cmd == NULL) {
		printk("[%s][%d] tz_malloc_shared_mem failed!\n", __func__, __LINE__);
		return -1;
	}

	smc_call_entry.local_cmd = local_smc_cmd;
	smc_call_entry.teei_cmd_type = teei_cmd_type;
	smc_call_entry.dev_file_id = dev_file_id;
	smc_call_entry.svc_id = svc_id;
	smc_call_entry.cmd_id = cmd_id;
	smc_call_entry.context = context;
	smc_call_entry.enc_id = enc_id;
	smc_call_entry.cmd_buf = cmd_buf;
	smc_call_entry.cmd_len = cmd_len;
	smc_call_entry.resp_buf = resp_buf;
	smc_call_entry.resp_len = resp_len;
	smc_call_entry.meta_data = meta_data;
	smc_call_entry.info_data = info_data;
	smc_call_entry.info_len = info_len;
	smc_call_entry.ret_resp_len = ret_resp_len;
	smc_call_entry.error_code = error_code;
	smc_call_entry.psema = psema;

	if (teei_config_flag == 1) {
		complete(&global_down_lock);
	}

	down(&smc_lock);

	/* with a wmb() */
	wmb();
#if 0
	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_teei_smc_call, (void *)(&smc_call_entry), 1);
	put_online_cpus();
#else
	Flush_Dcache_By_Area((unsigned long)&smc_call_entry, (unsigned long)&smc_call_entry + sizeof(smc_call_entry));
	retVal = add_work_entry(CAPI_CALL, (unsigned long)&smc_call_entry);
        if (retVal != 0) {
                tz_free_shared_mem(local_smc_cmd, sizeof(struct teei_smc_cmd));
                return retVal;
        }
#endif

	down(psema);

	rmb();

	if (ret_resp_len)
		*ret_resp_len = local_smc_cmd->ret_resp_buf_len;

	tz_free_shared_mem(local_smc_cmd, sizeof(struct teei_smc_cmd));

	return smc_call_entry.retVal;
}

#endif

int service_smc_call(u32 teei_cmd_type, u32 dev_file_id, u32 svc_id,
		u32 cmd_id, u32 context, u32 enc_id,
		const void *cmd_buf,
		size_t cmd_len,
		void *resp_buf,
		size_t resp_len,
		const void *meta_data,
		int *ret_resp_len,
		void *wq,
		void *arg_lock, int *error_code)
{
	return 0;
}

static int teei_client_close_session_for_service(
		void *private_data,
		struct teei_session *temp_ses)
{
	struct ser_ses_id *ses_close = (struct ser_ses_id *)tz_malloc_shared_mem(sizeof(struct ser_ses_id), GFP_KERNEL);
	struct teei_context *curr_cont = NULL;
	struct teei_encode *temp_encode = NULL;
	struct teei_encode *enc_context = NULL;
	struct teei_shared_mem *shared_mem = NULL;
	struct teei_shared_mem *temp_shared = NULL;
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;
	int *res = (int *)tz_malloc_shared_mem(4, GFP_KERNEL);
	int error_code = 0;

	if (temp_ses == NULL)
		return -EINVAL;
	if (ses_close == NULL)
		return -ENOMEM;

	if (res == NULL)
		return -ENOMEM;
	ses_close->session_id = temp_ses->sess_id;
	printk("======== ses_close->session_id = %d =========\n", ses_close->session_id);
	curr_cont = temp_ses->parent_cont;

	retVal = teei_smc_call(TEEI_CMD_TYPE_CLOSE_SESSION,
			dev_file_id,
			0,
			TEEI_GLOBAL_CMD_ID_CLOSE_SESSION,
			0,
			0,
			ses_close,
			sizeof(struct ser_ses_id),
			res,
			sizeof(int),
			NULL,
			NULL,
			0,
			NULL,
			&error_code,
			&(curr_cont->cont_lock));

	if (!list_empty(&temp_ses->encode_list)) {

		list_for_each_entry_safe(enc_context,
				temp_encode,
				&temp_ses->encode_list,
				head) {
			if (enc_context) {
				list_del(&enc_context->head);
				kfree(enc_context);
			}
		}
	}

	if (!list_empty(&temp_ses->shared_mem_list)) {

		list_for_each_entry_safe(shared_mem,
				temp_shared,
				&temp_ses->shared_mem_list,
				s_head) {
			if (shared_mem == NULL)
				continue;

			list_del(&shared_mem->s_head);

			if (shared_mem->k_addr)
				free_pages((unsigned long)shared_mem->k_addr,
						get_order(ROUND_UP(shared_mem->len, SZ_4K)));

			kfree(shared_mem);
		}
	}

	list_del(&temp_ses->link);
	curr_cont->sess_cnt = curr_cont->sess_cnt - 1;

	kfree(temp_ses);
	tz_free_shared_mem(res, 4);
	tz_free_shared_mem(ses_close, sizeof(struct ser_ses_id));

	return 0;
}



static int teei_client_service_init(struct teei_context *dev_file, struct ser_ses_id *ses_open)
{
	return 0;
}


/**
 * @brief	close the context with context_ID equal private_data
 *
 * @return	EINVAL: Can not find the context with ID equal private_data
 *		0: success
 */
static int teei_client_service_exit(void *private_data)
{
	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_shared_mem *temp_pos = NULL;
	struct teei_context *temp_context = NULL;
	struct teei_context *temp_context_pos = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_session *temp_ses_pos = NULL;
	unsigned long dev_file_id = 0;

	dev_file_id = (unsigned long)(private_data);
	down_write(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry_safe(temp_context,
			temp_context_pos,
			&teei_contexts_head.context_list,
			link) {
		if (temp_context->cont_id == dev_file_id) {

			list_for_each_entry_safe(temp_shared_mem,
					temp_pos,
					&temp_context->shared_mem_list,
					head) {
				if (temp_shared_mem) {
					list_del(&(temp_shared_mem->head));
					if (temp_shared_mem->k_addr) {
						free_pages((unsigned long)temp_shared_mem->k_addr,
								get_order(ROUND_UP(temp_shared_mem->len, SZ_4K)));
					}
					kfree(temp_shared_mem);
				}
			}

			if (!list_empty(&temp_context->sess_link)) {
				list_for_each_entry_safe(temp_ses,
						temp_ses_pos,
						&temp_context->sess_link,
						link)
					teei_client_close_session_for_service(private_data, temp_ses);
			}

			list_del(&temp_context->link);
			kfree(temp_context);
			up_write(&(teei_contexts_head.teei_contexts_sem));
			return 0;
		}
	}

	return -EINVAL;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_session_init(void *private_data, void *argp)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *ses_new = NULL;
	struct user_ses_init ses_init;
	int ctx_found = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	TDEBUG("inside session init");

	if (copy_from_user(&ses_init, argp, sizeof(ses_init))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return  -EFAULT;
	}

	down_read(&(teei_contexts_head.teei_contexts_sem));
	/*print_context();*/
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}
	}
	up_read(&(teei_contexts_head.teei_contexts_sem));

	if (!ctx_found) {
		printk("[%s][%d] can't find context.\n", __func__, __LINE__);
		return -EINVAL;
	}

	ses_new = (struct teei_session *)tz_malloc(sizeof(struct teei_session), GFP_KERNEL);

	if (ses_new == NULL) {
		printk("[%s][%d] tz_malloc failed.\n", __func__, __LINE__);
		return -ENOMEM;
	}

	ses_init.session_id = (unsigned long)ses_new;
	ses_new->sess_id = (unsigned long)ses_new;
	ses_new->parent_cont = temp_cont;

	INIT_LIST_HEAD(&ses_new->link);
	INIT_LIST_HEAD(&ses_new->encode_list);
	INIT_LIST_HEAD(&ses_new->shared_mem_list);
	list_add_tail(&ses_new->link, &temp_cont->sess_link);

	if (copy_to_user(argp, &ses_init, sizeof(ses_init))) {
		list_del(&ses_new->link);
		kfree(ses_new);
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief			Open one session
 *
 * @param argp
 *
 * @return			0: success
 *				EFAULT: copy data from user space OR copy data back to the user space error
 *				EINVLA: can not find the context OR session structure.
 */
static int teei_client_session_open(void *private_data, void *argp)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *ses_new = NULL;
	struct teei_encode *enc_temp = NULL;

	struct ser_ses_id *ses_open = (struct ser_ses_id *)tz_malloc_shared_mem(sizeof(struct ser_ses_id), GFP_KERNEL);
	int ctx_found = 0;
	int sess_found = 0;
	int enc_found = 0;
	int retVal = 0;
	unsigned long dev_file_id = (unsigned long)private_data;
	printk("ses open [%ld]\n", (unsigned long)ses_open);
	if (ses_open == NULL) {
		return -EFAULT;
	}
	/* Get the paraments about this session from user space. */
	if (copy_from_user(ses_open, argp, sizeof(struct ser_ses_id))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		tz_free_shared_mem(ses_open, sizeof(struct ser_ses_id));
		return -EFAULT;
	}
	/* Search the teei_context structure */
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}
	}

	if (ctx_found == 0) {
		printk("[%s][%d] can't find context!\n", __func__, __LINE__);
		tz_free_shared_mem(ses_open, sizeof(struct ser_ses_id));
		return -EINVAL;
	}

	/* Search the teei_session structure */
	list_for_each_entry(ses_new, &temp_cont->sess_link, link) {
		if (ses_new->sess_id == ses_open->session_id) {
			sess_found = 1;
			break;
		}
	}

	if (sess_found == 0) {
		printk("[%s][%d] can't find session!\n", __func__, __LINE__);
		tz_free_shared_mem(ses_open, sizeof(struct ser_ses_id));
		return -EINVAL;
	}

	/* Search the teei_encode structure */
	list_for_each_entry(enc_temp, &ses_new->encode_list, head) {
		if (enc_temp->encode_id == ses_open->paramtype) {
			enc_found = 1;
			break;
		}
	}

	/* Invoke the smc_call */
	if (enc_found) {
		/* This session had been encoded. */
		retVal = teei_smc_call(TEEI_CMD_TYPE_OPEN_SESSION,
				dev_file_id,
				0,
				0,
				0,
				0,
				enc_temp->ker_req_data_addr,
				enc_temp->enc_req_offset,
				enc_temp->ker_res_data_addr,
				enc_temp->enc_res_offset,
				enc_temp->meta,
				ses_open,
				sizeof(struct ser_ses_id),
				NULL,
				NULL,
				&(temp_cont->cont_lock));

	} else {
		/* This session didn't have been encoded */
		retVal = teei_smc_call(TEEI_CMD_TYPE_OPEN_SESSION,
				dev_file_id,
				0,
				0,
				0,
				0,
				NULL,
				0,
				NULL,
				0,
				NULL,
				ses_open,
				sizeof(struct ser_ses_id),
				NULL,
				NULL,
				&(temp_cont->cont_lock));
	}

	if (retVal != SMC_SUCCESS) {
		printk("[%s][%d] open session smc error!\n", __func__, __LINE__);
		goto clean_hdr_buf;
	}

	if (ses_open->session_id == -1)
		printk("[%s][%d] invalid session id!\n", __func__, __LINE__);

	/* Copy the result back to the user space */
	ses_new->sess_id = ses_open->session_id;
	if (copy_to_user(argp, ses_open, sizeof(struct ser_ses_id))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		retVal = -EFAULT;
		goto clean_hdr_buf;
	}

	tz_free_shared_mem(ses_open, sizeof(struct ser_ses_id));
	return 0;

clean_hdr_buf:
	list_del(&ses_new->link);
	tz_free_shared_mem(ses_open, sizeof(struct ser_ses_id));
	kfree(ses_new);
	return retVal;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_session_close(void *private_data, void *argp)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	int retVal = -EFAULT;
	unsigned long dev_file_id = (unsigned long)private_data;

	struct ser_ses_id ses_close;

	if (copy_from_user(&ses_close, argp, sizeof(ses_close))) {
		printk("[%s][%d] copy from user failed.\n ", __func__, __LINE__);
		return -EFAULT;
	}

	down_read(&(teei_contexts_head.teei_contexts_sem));
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == ses_close.session_id) {
					teei_client_close_session_for_service(private_data, temp_ses);
					retVal = 0;
					goto copy_to_user;
				}
			}
		}
	}

copy_to_user:
	up_read(&(teei_contexts_head.teei_contexts_sem));

	if (copy_to_user(argp, &ses_close, sizeof(ses_close))) {
		printk("[%s][%d] copy from user failed.\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

/**
 * @brief	Map the vma with the free pages
 *
 * @param filp
 * @param vma
 *
 * @return	0: success
 *		EINVAL: Invalid parament
 *		ENOMEM: No enough memory
 */
static int teei_client_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int retVal = 0;
	struct teei_shared_mem *share_mem_entry = NULL;
	int context_found = 0;
	unsigned long alloc_addr = 0;
	struct teei_context *cont = NULL;
	long length = vma->vm_end - vma->vm_start;

	/* Reasch the context with ID equal filp->private_data */

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(cont, &(teei_contexts_head.context_list), link) {
		if (cont->cont_id == (unsigned long)filp->private_data) {
			context_found = 1;
			break;
		}

	}

	if (context_found == 0) {
		up_read(&(teei_contexts_head.teei_contexts_sem));
		return -EINVAL;
	}

	/* Alloc one teei_share_mem structure */
	share_mem_entry = tz_malloc(sizeof(struct teei_shared_mem), GFP_KERNEL);
	if (share_mem_entry == NULL) {
		printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
		up_read(&(teei_contexts_head.teei_contexts_sem));
		return -ENOMEM;
	}

	/* Get free pages from Kernel. */
	alloc_addr =  (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(length, SZ_4K)));
	if (alloc_addr == 0) {
		printk("[%s][%d] get free pages failed!\n", __func__, __LINE__);
		kfree(share_mem_entry);
		up_read(&(teei_contexts_head.teei_contexts_sem));
		return -ENOMEM;
	}

	/* Remap the free pages to the VMA */
	retVal = remap_pfn_range(vma, vma->vm_start, ((virt_to_phys((void *)alloc_addr)) >> PAGE_SHIFT),
			length, vma->vm_page_prot);

	if (retVal) {
		printk("[%s][%d] remap_pfn_range failed!\n", __func__, __LINE__);
		kfree(share_mem_entry);
		free_pages(alloc_addr, get_order(ROUND_UP(length, SZ_4K)));
		up_read(&(teei_contexts_head.teei_contexts_sem));
		return retVal;
	}

	/* Add the teei_share_mem into the teei_context struct */
	share_mem_entry->k_addr = (void *)alloc_addr;
	share_mem_entry->len = length;
	share_mem_entry->u_addr = (void *)vma->vm_start;
	share_mem_entry->index = share_mem_entry->u_addr;

	cont->shared_mem_cnt++;
	list_add_tail(&(share_mem_entry->head), &(cont->shared_mem_list));

	up_read(&(teei_contexts_head.teei_contexts_sem));

	return 0;
}

/**
 * @brief		Send a command to TEE
 *
 * @param argp
 *
 * @return		EINVAL: Invalid parament
 *                      EFAULT: copy data from user space OR copy data back to the user space error
 *                      0: success
 */
static int teei_client_send_cmd(void *private_data, void *argp)
{
	int retVal = 0;
	struct teei_client_encode_cmd enc;
	unsigned long dev_file_id = 0;
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_encode *enc_temp = NULL;
	int ctx_found = 0;
	int sess_found = 0;
	int enc_found = 0;

	dev_file_id = (unsigned long)private_data;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}

	}

	up_read(&(teei_contexts_head.teei_contexts_sem));

	if (ctx_found == 0) {
		printk("[%s][%d] can't find context data!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
		if (temp_ses->sess_id == enc.session_id) {
			sess_found = 1;
			break;
		}
	}

	if (sess_found == 0) {
		printk("[%s][%d] can't find session data!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (enc.encode_id != -1) {
		list_for_each_entry(enc_temp, &temp_ses->encode_list, head) {
			if (enc_temp->encode_id == enc.encode_id) {
				enc_found = 1;
				break;
			}
		}
	} else {
		retVal = teei_client_prepare_encode(private_data, &enc, &enc_temp, &temp_ses);
		if (retVal == 0)
			enc_found = 1;
	}

	if (enc_found == 0) {
		printk("[%s][%d] can't find encode data!\n", __func__, __LINE__);
		return -EINVAL;
	}

	retVal = teei_smc_call(TEEI_CMD_TYPE_INVOKE_COMMAND,
			dev_file_id,
			0,
			enc.cmd_id,
			enc.session_id,
			enc.encode_id,
			enc_temp->ker_req_data_addr,
			enc_temp->enc_req_offset,
			enc_temp->ker_res_data_addr,
			enc_temp->enc_res_offset,
			enc_temp->meta,
			NULL,
			0,
			&enc.return_value,
			&enc.return_origin,
			&(temp_cont->cont_lock));

	if (retVal != SMC_SUCCESS)
		printk("[%s][%d] send cmd secure call failed!\n", __func__, __LINE__);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		printk("[%s][%d] copy to user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

static int teei_client_context_init(void *private_data, void *argp)
{
	int retVal = 0;
	struct teei_context *temp_cont = NULL;
	unsigned long dev_file_id = (unsigned long)private_data;
	struct ctx_data ctx;
	int dev_found = 0;
	int error_code = 0;
	int *resp_flag = tz_malloc_shared_mem(4, GFP_KERNEL);
	char *name = tz_malloc_shared_mem(sizeof(ctx.name), GFP_KERNEL);
	if (resp_flag == NULL)
		return -ENOMEM;

	if (name == NULL)
		return -ENOMEM;
	if (copy_from_user(&ctx, argp, sizeof(ctx))) {
		printk("[%s][%d] copy from user failed.\n ", __func__, __LINE__);
		return -EFAULT;
	}

	memcpy(name, ctx.name, sizeof(ctx.name));
	printk("[%s][%d] context name = %s.\n ", __func__, __LINE__, name);
	Flush_Dcache_By_Area((unsigned long)name,
		(unsigned long)name+sizeof(ctx.name));

	down_write(&(teei_contexts_head.teei_contexts_sem));
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			dev_found = 1;
			break;
		}
	}
	up_write(&(teei_contexts_head.teei_contexts_sem));

	if (dev_found) {
		strcpy(temp_cont->tee_name, ctx.name);
		retVal = teei_smc_call(TEEI_CMD_TYPE_INITILIZE_CONTEXT, dev_file_id,
				0, 0, 0, 0, name, 255, resp_flag, 4, NULL,
				NULL, 0, NULL, &error_code, &(temp_cont->cont_lock));
	}

	ctx.ctx_ret = !(*resp_flag);

	tz_free_shared_mem(resp_flag, 4);
	tz_free_shared_mem(name, sizeof(ctx.name));

	if (copy_to_user(argp, &ctx, sizeof(ctx))) {
		printk("[%s][%d]copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;

}

static int teei_client_context_close(void *private_data, void *argp)
{
	int retVal = 0;
	struct teei_context *temp_cont = NULL;
	unsigned long dev_file_id = (unsigned long)private_data;
	struct ctx_data ctx;
	int dev_found = 0;
	int *resp_flag = (int *) tz_malloc_shared_mem(4, GFP_KERNEL);
	int error_code = 0;
	if (resp_flag == NULL)
		return -ENOMEM;
	if (copy_from_user(&ctx, argp, sizeof(ctx))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	down_write(&(teei_contexts_head.teei_contexts_sem));
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
	printk("cont_id = %ld =======\n", temp_cont->cont_id);
		if (temp_cont->cont_id == dev_file_id) {
			dev_found = 1;
			break;
		}
	}
	up_write(&(teei_contexts_head.teei_contexts_sem));

	if (dev_found) {
		strcpy(temp_cont->tee_name, ctx.name);
		retVal = teei_smc_call(TEEI_CMD_TYPE_FINALIZE_CONTEXT, dev_file_id,
				0, 0, 0, 0,
				NULL, 0, resp_flag, 4, NULL, NULL,
				0, NULL, &error_code, &(temp_cont->cont_lock));
	}

	ctx.ctx_ret = *resp_flag;
	tz_free_shared_mem(resp_flag, 4);


	if (copy_to_user(argp, &ctx, sizeof(ctx))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}


	return retVal;

}

/**
 * @brief		Delete the operation with the encode_id
 *
 * @param argp
 *
 * @return		EINVAL: Invalid parament
 *                      EFAULT: copy data from user space OR copy data back to the user space error
 *                      0: success
 */
static int teei_client_operation_release(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	struct teei_encode *enc_context = NULL;
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	int ctx_found = 0;
	int session_found = 0;
	int enc_found = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}
	}

	if (ctx_found == 0) {
		printk("[%s][%d] ctx_found failed!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
		if (temp_ses->sess_id == enc.session_id) {
			session_found = 1;
			break;
		}
	}

	if (session_found == 0) {
		printk("[%s][%d] session_found failed!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (enc.encode_id != -1) {
		list_for_each_entry(enc_context, &temp_ses->encode_list, head) {
			if (enc_context->encode_id == enc.encode_id) {
				enc_found = 1;
				break;
			}
		}
	}

	if (enc_found == 0) {
		printk("[%s][%d] enc_found failed!\n", __func__, __LINE__);
		return -EINVAL;
	} else {
		if (enc_context->ker_req_data_addr)
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);

		if (enc_context->ker_res_data_addr)
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);

		list_del(&enc_context->head);
		/* kfree(enc_context->meta); */
		tz_free_shared_mem(enc_context->meta, sizeof(struct teei_encode_meta) *
				(TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));
		kfree(enc_context);
	}

	return 0;
}

/**
 * @brief		search the teei_encode and teei_session structures
 *			if there is no structure, create one.
 * @param		private_data Context ID
 * @param		enc
 * @param		penc_context
 * @param		psession
 *
 * @return		EINVAL: Invalid parament
 *			ENOMEM: No enough memory
 *			0: success
 */
static int teei_client_prepare_encode(void *private_data,
		struct teei_client_encode_cmd *enc,
		struct teei_encode **penc_context,
		struct teei_session **psession)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_encode *enc_context = NULL;
	int session_found = 0;
	int enc_found = 0;
	int retVal = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	/* search the context session with private_data */
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == enc->session_id) {
					session_found = 1;
					break;
				}
			}
		}
		if (session_found == 1)
			break;
	}
	if (!session_found) {
		printk("[%s][%d] session (ID: %x) not found!\n", __func__, __LINE__, enc->session_id);
		return -EINVAL;
	}

	/*
	 * check if the enc struct had been inited.
	 */

	if (enc->encode_id != -1) {
		list_for_each_entry(enc_context, &temp_ses->encode_list, head) {
			if (enc_context->encode_id == enc->encode_id) {
				enc_found = 1;
				break;
			}
		}
	}

	/* create one command parament block */
	if (!enc_found) {
		enc_context = (struct teei_encode *)tz_malloc(sizeof(struct teei_encode), GFP_KERNEL);
		if (enc_context == NULL) {
			printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
			return -ENOMEM;
		}
		enc_context->meta = tz_malloc_shared_mem(sizeof(struct teei_encode_meta) *
				(TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS),
				GFP_KERNEL);

		if (enc_context->meta == NULL) {
			printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
			kfree(enc_context);
			return -ENOMEM;
		}

		memset(enc_context->meta, 0, sizeof(struct teei_encode_meta) *
				(TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));

		enc_context->encode_id = (unsigned long)enc_context;
		enc_context->ker_req_data_addr = NULL;
		enc_context->ker_res_data_addr = NULL;
		enc_context->enc_req_offset = 0;
		enc_context->enc_res_offset = 0;
		enc_context->enc_req_pos = 0;
		enc_context->enc_res_pos = TEEI_MAX_REQ_PARAMS;
		enc_context->dec_res_pos = TEEI_MAX_REQ_PARAMS;
		enc_context->dec_offset = 0;
		list_add_tail(&enc_context->head, &temp_ses->encode_list);
		enc->encode_id = enc_context->encode_id;
	}

	/* return the enc_context & temp_ses */

	*penc_context = enc_context;
	*psession = temp_ses;

	return retVal;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */

static int teei_client_encode_uint32(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;
	struct teei_session *session = NULL;
	struct teei_encode *enc_context = NULL;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = teei_client_prepare_encode(private_data, &enc, &enc_context, &session);
	if (retVal != 0) {
		printk("[%s][%d]  failed!\n", __func__, __LINE__);
		return retVal;
	}

	/*
	   TZDebug("enc.data= %p",(void *)enc.data);
	   printk("[%s][%d] enc.encode_id = %x\n", __func__, __LINE__, enc.encode_id);
	   printk("[%s][%d] enc_context->encode_id = %x\n", __func__, __LINE__, enc_context->encode_id);
	 */

	if (enc.param_type == TEEIC_PARAM_IN) {
		if (enc_context->ker_req_data_addr == NULL) {
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (enc_context->ker_req_data_addr == NULL) {
				printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				retVal = -ENOMEM;
				goto ret_encode_u32;
			}
		}

		if ((enc_context->enc_req_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			u64 addr = enc.data;
			void __user *pt = compat_ptr((unsigned int *)addr);
			u32 value = 0;
			copy_from_user(&value, pt, 4);
			*(u32 *)((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset) = value;
			TZDebug("value %x", value);
			enc_context->enc_req_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].type = TEEI_ENC_UINT32;
			enc_context->meta[enc_context->enc_req_pos].len = sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].value_flag = enc.value_flag;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc.param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc.param_pos_type;
			enc_context->enc_req_pos++;
		} else {
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			retVal = -ENOMEM;
			goto ret_encode_u32;
		}
	} else if (enc.param_type == TEEIC_PARAM_OUT) {
		if (!enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_res_data_addr) {
				printk("[%s][%d] tz_malloc failed\n", __func__, __LINE__);
				retVal = -ENOMEM;
				goto ret_encode_u32;
			}
		}
		if ((enc_context->enc_res_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_res_pos < (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			if (enc.data != NULL) {
				u64 addr = enc.data;
				void __user *pt = compat_ptr((unsigned int *)addr);
				/*
				   u32 value = 0;
				   copy_from_user(&value, pt, 4);
				   TZDebug("value %x", value);
				 */
				enc_context->meta[enc_context->enc_res_pos].usr_addr = pt;
			} else {
				enc_context->meta[enc_context->enc_res_pos].usr_addr = 0;
			}
			enc_context->enc_res_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].type = TEEI_ENC_UINT32;
			enc_context->meta[enc_context->enc_res_pos].len = sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].value_flag = enc.value_flag;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc.param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc.param_pos_type;
			enc_context->enc_res_pos++;
		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);
			retVal =  -ENOMEM;
			goto ret_encode_u32;
		}
	}


ret_encode_u32:
	if (copy_to_user(argp, &enc, sizeof(enc))) {
		TERR("copy from user failed ");
		retVal = -EFAULT;
	}

	return retVal;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_encode_array(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;
	struct teei_encode *enc_context = NULL;
	struct teei_session *session = NULL;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = teei_client_prepare_encode(private_data, &enc, &enc_context, &session);

	if (retVal)
		goto return_func;

	if (enc.param_type == TEEIC_PARAM_IN) {
		if (NULL == enc_context->ker_req_data_addr) {
			/* printk("[%s][%d] allocate req data data.\n", __func__, __LINE__); */
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_req_data_addr) {
				printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				retVal = -ENOMEM;
				goto ret_encode_array;
			}
		}

		if ((enc_context->enc_req_offset + enc.len <= TEEI_1K_SIZE) &&
				(enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			if (copy_from_user(
						(char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset,
						enc.data , enc.len)) {
				printk("[%s][%d] copy from user failed.\n", __func__, __LINE__);
				retVal = -EFAULT;
				goto ret_encode_array;
			}
			enc_context->enc_req_offset += enc.len;

			enc_context->meta[enc_context->enc_req_pos].type = TEEI_ENC_ARRAY;
			enc_context->meta[enc_context->enc_req_pos].len = enc.len;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc.param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc.param_pos_type;

			enc_context->enc_req_pos++;
		} else {
			/* kfree(enc_context->ker_req_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			retVal = -ENOMEM;
			goto ret_encode_array;
		}
	} else if (enc.param_type == TEEIC_PARAM_OUT) {
		if (NULL == enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (NULL == enc_context->ker_res_data_addr) {
				printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				retVal = -ENOMEM;
				goto ret_encode_array;
			}
		}
		if ((enc_context->enc_res_offset + enc.len <= TEEI_1K_SIZE) &&
				(enc_context->enc_res_pos <
				 (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			if (enc.data != NULL) {
				enc_context->meta[enc_context->enc_res_pos].usr_addr
					= (unsigned long)enc.data;
			} else {
				enc_context->meta[enc_context->enc_res_pos].usr_addr = 0;
			}
			enc_context->enc_res_offset += enc.len;
			enc_context->meta[enc_context->enc_res_pos].type = TEEI_ENC_ARRAY;
			enc_context->meta[enc_context->enc_res_pos].len = enc.len;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc.param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc.param_pos_type;

			enc_context->enc_res_pos++;
		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			retVal = -ENOMEM;
			goto ret_encode_array;
		}
	}

ret_encode_array:
	if (copy_to_user(argp, &enc, sizeof(enc))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

return_func:
	TDEBUG("[%s][%d] teei_client_encode_array end!\n", __func__, __LINE__);
	return retVal;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_encode_mem_ref(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;
	int shared_mem_found = 0;
	struct teei_encode *enc_context = NULL;
	struct teei_session *session = NULL;
	struct teei_shared_mem *temp_shared_mem = NULL;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = teei_client_prepare_encode(private_data, &enc, &enc_context, &session);

	if (retVal != 0)
		goto return_func;

	list_for_each_entry(temp_shared_mem, &session->shared_mem_list, s_head) {
		u64 addr = enc.data;
		if (temp_shared_mem && temp_shared_mem->index == (u32 *)addr) {
			shared_mem_found = 1;
			break;
		}
	}

	if (shared_mem_found == 0) {
		struct teei_context *temp_cont = NULL;
		list_for_each_entry(temp_cont,
				&teei_contexts_head.context_list,
				link) {
			if (temp_cont->cont_id == (unsigned long)private_data) {
				list_for_each_entry(temp_shared_mem,
						&temp_cont->shared_mem_list,
						head) {
					if (temp_shared_mem->index == (u32)enc.data) {
						shared_mem_found = 1;
						break;
					}
				}

				break;
			}
			if (shared_mem_found == 1)
				break;
		}
	}

	if (!shared_mem_found) {
		retVal = -EINVAL;
		goto return_func;
	}

	if (enc.param_type == TEEIC_PARAM_IN) {
		if (NULL == enc_context->ker_req_data_addr) {
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (NULL == enc_context->ker_req_data_addr) {
				printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				retVal = -ENOMEM;
				goto ret_encode_array;
			}
		}

		if ((enc_context->enc_req_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			*(u32 *)((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset)
				= virt_to_phys((char *)temp_shared_mem->k_addr+enc.offset);

			Flush_Dcache_By_Area((unsigned long)(temp_shared_mem->k_addr+enc.offset),
					(unsigned long)temp_shared_mem->k_addr+enc.offset+enc.len);
			enc_context->enc_req_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].usr_addr
				= (unsigned long)((char *)temp_shared_mem->u_addr + enc.offset);
			enc_context->meta[enc_context->enc_req_pos].type = TEEI_MEM_REF;
			enc_context->meta[enc_context->enc_req_pos].len = enc.len;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc.param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc.param_pos_type;

			enc_context->enc_req_pos++;

		} else {
			/* kfree(enc_context->ker_req_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			retVal = -ENOMEM;
			goto ret_encode_array;
		}
	} else if (enc.param_type == TEEIC_PARAM_OUT) {
		if (!enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_res_data_addr) {
				printk("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				retVal = -ENOMEM;
				goto ret_encode_array;
			}
		}
		if ((enc_context->enc_res_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_res_pos < (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			*(u32 *)((char *)enc_context->ker_res_data_addr +
					enc_context->enc_res_offset)
				= virt_to_phys((char *)temp_shared_mem->k_addr + enc.offset);

			enc_context->enc_res_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].usr_addr
				= (unsigned long)((char *)temp_shared_mem->u_addr + enc.offset);
			enc_context->meta[enc_context->enc_res_pos].type
				=  TEEI_MEM_REF;
			enc_context->meta[enc_context->enc_res_pos].len = enc.len;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc.param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc.param_pos_type;

			enc_context->enc_res_pos++;


		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);
			retVal = -ENOMEM;
			goto ret_encode_array;
		}
	}

ret_encode_array:
	if (copy_to_user(argp, &enc, sizeof(enc))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

return_func:
	/* printk("[%s][%d] teei_client_encode_mem_ref end.\n", __func__, __LINE__); */
	return retVal;
}


static int print_context(void)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_sess = NULL;
	struct teei_encode *dec_context = NULL;

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		printk("[%s][%d] context id [%lx]\n", __func__, __LINE__, temp_cont->cont_id);
		list_for_each_entry(temp_sess, &temp_cont->sess_link, link) {
			printk("[%s][%d] session id [%x]\n", __func__, __LINE__, temp_sess->sess_id);
/*
			list_for_each_entry(dec_context, &temp_sess->encode_list, head) {
				printk("[%s][%d] encode_id [%x]\n", __func__, __LINE__, dec_context->encode_id);
			}
*/
		}
	}
	return 0;
}


/**
 * @brief
 *
 * @param dec
 * @param pdec_context
 *
 * @return
 */
static int teei_client_prepare_decode(void *private_data,
		struct teei_client_encode_cmd *dec,
		struct teei_encode **pdec_context)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_encode *dec_context = NULL;
	int session_found = 0;
	int enc_found = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == dec->session_id) {
					session_found = 1;
					break;
				}
			}
			break;
		}
	}

	if (0 == session_found) {
		printk("[%s][%d] session not found!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (dec->encode_id != -1) {
		list_for_each_entry(dec_context, &temp_ses->encode_list, head) {
			if (dec_context->encode_id == dec->encode_id) {
				enc_found = 1;
				break;
			}
		}
	}

	/* print_context(); */
	if (0 == enc_found) {
		printk("[%s][%d] encode[%x] not found!\n", __func__, __LINE__, dec->encode_id);
		return -EINVAL;
	}

	*pdec_context = dec_context;

	return 0;
}

/**
 * @brief		Decode the uint32 result
 *
 * @param argp
 *
 * @return		EFAULT:	fail to copy data from user space OR to user space.
 *			0:	success
 */
static int teei_client_decode_uint32(void *private_data, void *argp)
{
	struct teei_client_encode_cmd dec;
	int retVal = 0;
	struct teei_encode *dec_context = NULL;

	if (copy_from_user(&dec, argp, sizeof(dec))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = teei_client_prepare_decode(private_data, &dec, &dec_context);

	if (retVal != 0) {
		printk("[%s][%d] teei_client_prepare_decode failed!\n", __func__, __LINE__);
		goto return_func;
	}

	if ((dec_context->dec_res_pos <= dec_context->enc_res_pos) &&
			(dec_context->meta[dec_context->dec_res_pos].type == TEEI_ENC_UINT32)) {
		if (dec_context->meta[dec_context->dec_res_pos].usr_addr) {
			dec.data = (void *)dec_context->meta[dec_context->dec_res_pos].usr_addr;
			TZDebug("data %x", dec.data);
		}

		u32 value = 0;
		u64 addr = dec.data;
		void __user *pt = compat_ptr((unsigned int *)addr);
		/* *(u32 *)dec.data = *((u32 *)((char *)dec_context->ker_res_data_addr + dec_context->dec_offset)); */
		value =  *((u32 *)((char *)dec_context->ker_res_data_addr + dec_context->dec_offset));
		copy_to_user(pt, &value, 4);
		dec_context->dec_offset += sizeof(u32);
		dec_context->dec_res_pos++;
	}

	if (copy_to_user(argp, &dec, sizeof(dec))) {
		printk("[%s][%d] copy to user failed.\n", __func__, __LINE__);
		return -EFAULT;
	}

return_func:
	return retVal;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_decode_array_space(void *private_data, void *argp)
{
	struct teei_client_encode_cmd dec;
	int retVal = 0;
	struct teei_encode *dec_context = NULL;

	if (copy_from_user(&dec, argp, sizeof(dec))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = teei_client_prepare_decode(private_data, &dec, &dec_context);

	if (retVal != 0)
		goto return_func;

	if ((dec_context->dec_res_pos <= dec_context->enc_res_pos) &&
			(dec_context->meta[dec_context->dec_res_pos].type
			 == TEEI_ENC_ARRAY)) {
		if (dec_context->meta[dec_context->dec_res_pos].len >=
				dec_context->meta[dec_context->dec_res_pos].ret_len) {
			if (dec_context->meta[dec_context->dec_res_pos].usr_addr)
				dec.data = (void *)dec_context->meta[dec_context->dec_res_pos].usr_addr;

			if (copy_to_user(dec.data, (char *)dec_context->ker_res_data_addr + dec_context->dec_offset,
						dec_context->meta[dec_context->dec_res_pos].ret_len)) {
				printk("[%s][%d] copy from user failed while copying array!\n", __func__, __LINE__);
				retVal = -EFAULT;
				goto return_func;
			}
		} else {

			printk("[%s][%d] buffer length is small. Length required %x and supplied length %x,pos %x ",
					__func__, __LINE__,
					dec_context->meta[dec_context->dec_res_pos].ret_len,
					dec_context->meta[dec_context->dec_res_pos].len,
					dec_context->dec_res_pos);

			retVal = -EFAULT;
			goto return_func;
		}

		dec.len = dec_context->meta[dec_context->dec_res_pos].ret_len;
		dec_context->dec_offset += dec_context->meta[dec_context->dec_res_pos].len;
		dec_context->dec_res_pos++;
	} else if ((dec_context->dec_res_pos <= dec_context->enc_res_pos) &&
			(dec_context->meta[dec_context->dec_res_pos].type == TEEI_MEM_REF)) {
		if (dec_context->meta[dec_context->dec_res_pos].len >=
				dec_context->meta[dec_context->dec_res_pos].ret_len) {
			dec.data = (void *)dec_context->meta[dec_context->dec_res_pos].usr_addr;
			unsigned long pmem = *(u32 *)((char *)dec_context->ker_res_data_addr + dec_context->dec_offset);
			char *mem = NULL;
			unsigned long addr = (unsigned long)phys_to_virt(pmem);
			mem = (char *)addr;
			Invalidate_Dcache_By_Area((unsigned long)mem,
					(unsigned long)mem+dec_context->meta[dec_context->dec_res_pos].ret_len + 1);
		} else {

			printk("[%s][%d] buffer length is small. Length required %x and supplied length %x",
					__func__, __LINE__,
					dec_context->meta[dec_context->dec_res_pos].ret_len,
					dec_context->meta[dec_context->dec_res_pos].len);

			retVal = -EFAULT;
			goto return_func;
		}

		dec.len = dec_context->meta[dec_context->dec_res_pos].ret_len;
		dec_context->dec_offset += sizeof(u32);
		dec_context->dec_res_pos++;
	}

	else {
		printk("[%s][%d] invalid data type or decoder at wrong position!\n", __func__, __LINE__);
		retVal = -EINVAL;
		goto return_func;
	}

	if (copy_to_user(argp, &dec, sizeof(dec))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		retVal = -EFAULT;
		goto return_func;
	}

return_func:
	TDEBUG("[%s][%d] teei_client_decode_array_space end.\n", __func__, __LINE__);
	return retVal;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_get_decode_type(void *private_data, void *argp)
{
	struct teei_client_encode_cmd dec;
	int retVal = 0;
	struct teei_encode *dec_context = NULL;


	if (copy_from_user(&dec, argp, sizeof(dec))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = teei_client_prepare_decode(private_data, &dec, &dec_context);

	if (retVal != 0)
		return retVal;

	if (dec_context->dec_res_pos <= dec_context->enc_res_pos)
		dec.data = (unsigned long)dec_context->meta[dec_context->dec_res_pos].type;
	else
		return -EINVAL;

	if (copy_to_user(argp, &dec, sizeof(dec))) {
		printk("[%s][%d] copy to user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}
/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_shared_mem_alloc(void *private_data, void *argp)
{
#if 0
	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_session_shared_mem_info mem_info;

	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	int  session_found = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	if (copy_from_user(&mem_info, argp, sizeof(mem_info))) {
		printk("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	/*
	   printk("[%s][%d] service id 0x%lx session id 0x%lx user mem addr 0x%p.\n",
	   __func__, __LINE__,
	   mem_info.service_id,
	   mem_info.session_id,
	   mem_info.user_mem_addr);
	 */
	list_for_each_entry(temp_cont,
			&teei_contexts_head.context_list,
			link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == mem_info.session_id) {
					session_found = 1;
					break;
				}
			}
			break;
		}
	}

	if (session_found == 0) {
		printk("[%s][%d] session not found!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_shared_mem, &temp_cont->shared_mem_list, head) {
		if (temp_shared_mem->index == (void *)mem_info.user_mem_addr) {
			list_del(&temp_shared_mem->head);
			temp_cont->shared_mem_cnt--;
			list_add_tail(&temp_shared_mem->s_head, &temp_ses->shared_mem_list);
			break;
		}
	}
#endif

	return 0;
}

/**
 * @brief
 *
 * @param argp
 *
 * @return
 */
static int teei_client_shared_mem_free(void *private_data, void *argp)
{
	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_session_shared_mem_info mem_info;
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_shared_mem *temp_pos = NULL;
	int session_found = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	if (copy_from_user(&mem_info, argp, sizeof(mem_info))) {
		printk("[%s][%d] copy from user failed.\n", __func__, __LINE__);
		return -EFAULT;
	}

	TZDebug("session id 0x%x user mem addr %x ",
			mem_info.session_id,
			mem_info.user_mem_addr);
	list_for_each_entry(temp_cont,
			&teei_contexts_head.context_list,
			link) {
		if (temp_cont->cont_id == dev_file_id) {
			TDEBUG("found file id");
			list_for_each_entry_safe(temp_shared_mem,
					temp_pos,
					&temp_cont->shared_mem_list,
					head) {
				if (temp_shared_mem && temp_shared_mem->u_addr == mem_info.user_mem_addr) {
					list_del(&temp_shared_mem->head);
					if (temp_shared_mem->k_addr)
						free_pages((u64)temp_shared_mem->k_addr,
								get_order(ROUND_UP(temp_shared_mem->len, SZ_4K)));
					kfree(temp_shared_mem);
				}
			}
		}
	}

#if 0
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				TZDebug("list:session id %x", temp_ses->sess_id);
				if (temp_ses->sess_id == mem_info.session_id) {
					session_found = 1;
					break;
				}
			}
			break;
		}
	}

	if (session_found == 0) {
		printk("[%s][%d] session not found!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_shared_mem, &temp_ses->shared_mem_list, s_head) {
		if (temp_shared_mem && temp_shared_mem->index == (void *)mem_info.user_mem_addr) {
			list_del(&temp_shared_mem->s_head);

			if (temp_shared_mem->k_addr)
				free_pages((unsigned long)temp_shared_mem->k_addr,
						get_order(ROUND_UP(temp_shared_mem->len, SZ_4K)));

			kfree(temp_shared_mem);
			break;
		}
	}
#endif
	return 0;
}

/**
 * @brief
 *
 * @param file
 * @param cmd
 * @param arg
 * @return
 */
static long teei_client_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	int retVal = 0;
	void *argp = (void __user *) arg;

	if (teei_config_flag == 0) {
		printk("Error: soter is NOT ready, Can not support IOCTL!\n");
		return -ECANCELED;
	}
	down(&api_lock);
	mutex_lock(&pm_mutex);
	switch (cmd) {

	case TEEI_CLIENT_IOCTL_INITCONTEXT_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_INITCONTEXT beginning .....\n", __func__, __LINE__);
			retVal = teei_client_context_init(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed init context %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_INITCONTEXT end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_CLOSECONTEXT_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_CLOSECONTEXT beginning .....\n", __func__, __LINE__);
			retVal = teei_client_context_close(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed close context: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_CLOSECONTEXT end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_SES_INIT_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_SES_INIT beginning .....\n", __func__, __LINE__);
			retVal = teei_client_session_init(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed session init: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_SES_INIT end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_SES_OPEN_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_SES_OPEN beginning .....\n", __func__, __LINE__);
			retVal = teei_client_session_open(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed session open: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_SES_OPEN end .....\n", __func__, __LINE__);
			break;


	case TEEI_CLIENT_IOCTL_SES_CLOSE_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_SES_CLOSE beginning .....\n", __func__, __LINE__);
			retVal = teei_client_session_close(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed session close: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_SES_CLOSE end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_OPERATION_RELEASE:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_OPERATION_RELEASE beginning .....\n", __func__, __LINE__);
			retVal = teei_client_operation_release(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed operation release: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_OPERATION_RELEASE end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_SEND_CMD_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_SEND_CMD beginning .....\n", __func__, __LINE__);
			retVal = teei_client_send_cmd(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed send cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_SEND_CMD end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_GET_DECODE_TYPE:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_GET_DECODE beginning .....\n", __func__, __LINE__);
			retVal = teei_client_get_decode_type(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed decode cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_GET_DECODE end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_ENC_UINT32:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_ENC_UINT32 beginning .....\n", __func__, __LINE__);
			retVal = teei_client_encode_uint32(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_encode_cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_ENC_UINT32 end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_DEC_UINT32:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_DEC_UINT32 beginning .....\n", __func__, __LINE__);
			retVal = teei_client_decode_uint32(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_decode_cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_DEC_UINT32 end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_ENC_ARRAY:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_ENC_ARRAY beginning .....\n", __func__, __LINE__);
			retVal = teei_client_encode_array(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_encode_cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_ENC_ARRAY end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_DEC_ARRAY_SPACE:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_DEC_ARRAY_SPACE beginning .....\n", __func__, __LINE__);
			retVal = teei_client_decode_array_space(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_decode_cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_DEC_ARRAY_SPACE end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_ENC_MEM_REF:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_DEC_MEM_REF beginning .....\n", __func__, __LINE__);
			retVal = teei_client_encode_mem_ref(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_encode_cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_DEC_MEM_REF end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_ENC_ARRAY_SPACE:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_ENC_ARRAY_SPACE beginning .....\n", __func__, __LINE__);
			retVal = teei_client_encode_mem_ref(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_encode_cmd: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_ENC_ARRAY_SPACE end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_SHR_MEM_ALLOCATE_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_SHR_MEM_ALLOCATE beginning .....\n", __func__, __LINE__);
			retVal = teei_client_shared_mem_alloc(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_shared_mem_alloc: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_SHR_MEM_ALLOCATE end .....\n", __func__, __LINE__);
			break;

	case TEEI_CLIENT_IOCTL_SHR_MEM_FREE_REQ:

			printk("[%s][%d] TEEI_CLIENT_IOCTL_SHR_MEM_FREE beginning .....\n", __func__, __LINE__);
			retVal = teei_client_shared_mem_free(file->private_data, argp);
			if (retVal != 0)
				printk("[%s][%d] failed teei_client_shared_mem_free: %x.\n", __func__, __LINE__, retVal);
			printk("[%s][%d] TEEI_CLIENT_IOCTL_SHR_MEM_FREE end .....\n", __func__, __LINE__);
			break;

	case TEEI_GET_TEEI_CONFIG_STAT:

			printk("[%s][%d] TEEI_GET_TEEI_CONFIG_STAT beginning .....\n", __func__, __LINE__);
			retVal = teei_config_flag;
			printk("[%s][%d] TEEI_GET_TEEI_CONFIG_STAT end .....\n", __func__, __LINE__);
			break;

	default:
			printk("[%s][%d] command not found!\n", __func__, __LINE__);
			retVal = -EINVAL;
	}
	mutex_unlock(&pm_mutex);
	up(&api_lock);
	return retVal;
}

/**
 * @brief		The open operation of /dev/teei_client device node.
 *
 * @param inode
 * @param file
 *
 * @return		ENOMEM: no enough memory in the linux kernel
 *			0: on success
 */

static int teei_client_open(struct inode *inode, struct file *file)
{
	struct teei_context *new_context = NULL;

	device_file_cnt++;
	file->private_data = (void *)device_file_cnt;

	new_context = (struct teei_context *)tz_malloc(sizeof(struct teei_context), GFP_KERNEL);
	if (new_context == NULL) {
		printk("tz_malloc failed for new dev file allocation!\n");
		return -ENOMEM;
	}

	new_context->cont_id = device_file_cnt;
	INIT_LIST_HEAD(&(new_context->sess_link));
	INIT_LIST_HEAD(&(new_context->link));

	new_context->shared_mem_cnt = 0;
	INIT_LIST_HEAD(&(new_context->shared_mem_list));

	sema_init(&(new_context->cont_lock), 0);

	down_write(&(teei_contexts_head.teei_contexts_sem));
	list_add(&(new_context->link), &(teei_contexts_head.context_list));
	teei_contexts_head.dev_file_cnt++;
	up_write(&(teei_contexts_head.teei_contexts_sem));

	return 0;
}

/**
 * @brief		The release operation of /dev/teei_client device node.
 *
 * @param		inode: device inode structure
 * @param		file:  struct file
 *
 * @return		0: on success
 */
static int teei_client_release(struct inode *inode, struct file *file)
{
	int retVal = 0;

	retVal = teei_client_service_exit(file->private_data);

	return retVal;
}

#endif

static irqreturn_t mt_deint_soft_revert_isr(unsigned irq, struct irq_desc *desc)
{
	irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
	return IRQ_HANDLED;
}

static void eint_gic_attach(void)
{
	int ret = 0;

	ret = mt_eint_set_deint(6, 189);
	if (ret == 0)
		printk("mt_eint_set_deint done\n");
	else {
		printk("mt_eint_set_deint fail\n");
		return;
	}
	mt_irq_set_polarity(189, IRQF_TRIGGER_FALLING); /*IRQF_TRIGGER_RISING*/
	/*
	   ret = request_irq(189, (irq_handler_t)mt_deint_soft_revert_isr,
	   IRQF_TRIGGER_RISING, "EINT-AUTOUNMASK", NULL);

	   if (ret > 0) {
	   printk(KERN_ERR "EINT IRQ LINE NOT AVAILABLE!!\n");
	   return;
	   }

	   printk("EINT %d request_irq done\n",eint_num);
	   irq_set_irq_type(189, IRQF_TRIGGER_RISING);

	   printk("trigger EINT %d done\n",eint_num);
	   printk("MASK 0x%x\n",mt_eint_get_mask(6));

	   mt_eint_clr_deint(eint_num);
	 */
}
/**
 * @brief
 *
 * @return
 */
static void teei_client_smc_init(void)
{
#if 0
	neu_disable_touch_irq();
	eint_gic_attach();
	enable_clock(25, "i2c");
	n_switch_to_t_os_stage2();

	switch_enable_flag = 1;
	mt_eint_clr_deint(6);
	neu_enable_touch_irq();
#endif
}


#if 0
void NQ_init(unsigned long NQ_buff)
{
	memset((char *)NQ_buff, 0, NQ_BUFF_SIZE);
}

static __always_inline unsigned int get_end_index(struct NQ_head *nq_head)
{
	if (nq_head->end_index == BLOCK_MAX_COUNT)
		return 1;
	else
		return nq_head->end_index + 1;

}
#endif

static irqreturn_t nt_sched_irq_handler(void)
{
	int cpu_id = raw_smp_processor_id();
	if (boot_soter_flag == START_STATUS) {
		forward_call_flag = GLSCH_FOR_SOTER;
		up(&smc_lock);
		return IRQ_HANDLED;
	} else {
		up(&smc_lock);
		return IRQ_HANDLED;
	}

}


int register_sched_irq_handler(void)
{

	int retVal = 0;
	retVal = request_irq(SCHED_IRQ, nt_sched_irq_handler, 0, "tz_drivers_service", NULL);

	if (retVal)
		printk("ERROR for request_irq %d error code : %d.\n", SCHED_IRQ, retVal);
	else
		printk("request irq [ %d ] OK.\n", SCHED_IRQ);

	return 0;

}


static irqreturn_t nt_soter_irq_handler(void)
{
	int cpu_id = raw_smp_processor_id();
	irq_call_flag = GLSCH_HIGH;
	up(&smc_lock);
	if (teei_config_flag == 1) {
		complete(&global_down_lock);
	}
	return IRQ_HANDLED;
}


int register_soter_irq_handler(void)
{
	int retVal = 0;
	retVal = request_irq(SOTER_IRQ, nt_soter_irq_handler, 0, "tz_drivers_service", NULL);

	if (retVal)
		printk("ERROR for request_irq %d error code : %d.\n", SOTER_IRQ, retVal);
	else
		printk("request irq [ %d ] OK.\n", SOTER_IRQ);

	return 0;
}


static irqreturn_t nt_error_irq_handler(void)
{
	printk("secure system ERROR !\n");
		soter_error_flag = 1;
	up(&(boot_sema));
		up(&smc_lock);
		return IRQ_HANDLED;
}


int register_error_irq_handler(void)
{
		int retVal = 0;
		retVal = request_irq(SOTER_ERROR_IRQ, nt_error_irq_handler, 0, "tz_drivers_service", NULL);

		if (retVal)
			printk("ERROR for request_irq %d error code : %d.\n", SOTER_ERROR_IRQ, retVal);
		else
			printk("request irq [ %d ] OK.\n", SOTER_ERROR_IRQ);

		return 0;
}


static irqreturn_t nt_fp_ack_handler(void)
{
	fp_call_flag = GLSCH_NONE;
	up(&boot_sema);
	up(&smc_lock);
	return IRQ_HANDLED;
}

int register_fp_ack_handler(void)
{
	int retVal = 0;
	retVal = request_irq(FP_ACK_IRQ, nt_fp_ack_handler, 0, "tz_drivers_service", NULL);

	if (retVal)
		printk("ERROR for request_irq %d error code : %d.\n", FP_ACK_IRQ, retVal);
	else
		printk("request irq [ %d ] OK.\n", FP_ACK_IRQ);

	return 0;
}

int get_bdrv_id(void)
{
	int driver_id = 0;
	driver_id = *((int *)bdrv_message_buff);
	return driver_id;
}


static irqreturn_t nt_bdrv_handler(void)
{
	int bdrv_id = 0;
	int cpu_id = raw_smp_processor_id();
	bdrv_id = get_bdrv_id();

#if 0
		if (bdrv_id == TEEI_VFS_NUM) {
			teei_vfs_flag = 1;

	}
#endif

	teei_vfs_flag = 1;

	add_bdrv_queue(bdrv_id);
	up(&smc_lock);

	return IRQ_HANDLED;
}


int register_bdrv_handler(void)
{
	int retVal = 0;
	retVal = request_irq(BDRV_IRQ, nt_bdrv_handler, 0, "tz_drivers_service", NULL);

	if (retVal)
		printk("ERROR for request_irq %d error code : %d.\n", BDRV_IRQ, retVal);
	else
		printk("request irq [ %d ] OK.\n", BDRV_IRQ);

	return 0;
}



static irqreturn_t nt_boot_irq_handler(void)
{
	int cpu_id = raw_smp_processor_id();
	if (boot_soter_flag == START_STATUS) {
		printk("boot irq  handler if\n");
		boot_soter_flag = END_STATUS;
		up(&smc_lock);
		up(&(boot_sema));
		return IRQ_HANDLED;
	} else {
		printk("boot irq hanler else\n");
		if (forward_call_flag == GLSCH_NONE)
			forward_call_flag = GLSCH_NEG;
		else
			forward_call_flag = GLSCH_NONE;

		up(&smc_lock);
		up(&(boot_sema));

		return IRQ_HANDLED;
	}
}

void init_tlog_entry(void)
{
	int i = 0;

	for (i = 0; i < TLOG_MAX_CNT; i++)
		tlog_ent[i].valid = TLOG_UNUSE;

	return;
}

int search_tlog_entry(void)
{
	int i = 0;

	for (i = 0; i < TLOG_MAX_CNT; i++) {
		if (tlog_ent[i].valid == TLOG_UNUSE) {
			tlog_ent[i].valid = TLOG_INUSE;
			return i;
		}
	}

	return -1;
}


void tlog_func(struct work_struct *entry)
{
	struct tlog_struct *ts = container_of(entry, struct tlog_struct, work);

	printk("TLOG %s", (char *)(ts->context));
	ts->valid = TLOG_UNUSE;
	return;
}


irqreturn_t tlog_handler(void)
{
	int pos = 0;
	printk("~tlog handler~\n");
	pos = search_tlog_entry();

	if (-1 != pos) {
		memset(tlog_ent[pos].context, 0, TLOG_CONTEXT_LEN);
		memcpy(tlog_ent[pos].context, (char *)tlog_message_buff, TLOG_CONTEXT_LEN);
		Flush_Dcache_By_Area((unsigned long)tlog_message_buff, (unsigned long)tlog_message_buff + TLOG_CONTEXT_LEN);
		INIT_WORK(&(tlog_ent[pos].work), tlog_func);
		queue_work(secure_wq, &(tlog_ent[pos].work));
	}
		irq_call_flag = GLSCH_HIGH;
		up(&smc_lock);

		return IRQ_HANDLED;
}

int register_tlog_handler(void)
{
		int retVal = 0;
		retVal = request_irq(TEEI_LOG_IRQ, tlog_handler, 0, "tz_drivers_service", NULL);

		if (retVal)
			printk("ERROR for request_irq %d error code : %d.\n", TEEI_LOG_IRQ, retVal);
		else
			printk("request irq [ %d ] OK.\n", TEEI_LOG_IRQ);

		return 0;
}

int register_boot_irq_handler(void)
{
	int retVal = 0;
	retVal = request_irq(BOOT_IRQ, nt_boot_irq_handler, 0, "tz_drivers_service", NULL);

	if (retVal)
		printk("ERROR for request_irq %d error code : %d.\n", BOOT_IRQ, retVal);
	else
		printk("request irq [ %d ] OK.\n", BOOT_IRQ);

	return 0;
}





static void secondary_boot_stage2(void *info)
{
	n_switch_to_t_os_stage2();
}


static void boot_stage2(void)
{
	int cpu_id = 0;

	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_boot_stage2, NULL, 1);
	put_online_cpus();
}

int switch_to_t_os_stages2(void)
{

	down(&(boot_sema));
	down(&(smc_lock));

	/* n_switch_to_t_os_stage2(); */
	boot_stage2();

	if (forward_call_flag == GLSCH_NONE)
		forward_call_flag = GLSCH_LOW;
	else if (forward_call_flag == GLSCH_NEG)
		forward_call_flag = GLSCH_NONE;
	else
		return -1;

	down(&(boot_sema));
	up(&(boot_sema));

	return 0;
}

static void secondary_load_tee(void *info)
{
	n_invoke_t_load_tee(0, 0, 0);
}


static void load_tee(void)
{
	int cpu_id = 0;

	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_load_tee, NULL, 1);
	put_online_cpus();
}



int t_os_load_image(void)
{
	/* down the boot_sema. */
	down(&(boot_sema));

	/* N_INVOKE_T_LOAD_TEE to TOS */
	set_sch_load_img_cmd();
	down(&smc_lock);
	/* n_invoke_t_load_tee(0, 0, 0); */
	load_tee();

	/* start HIGH level glschedule. */
	if (forward_call_flag == GLSCH_NONE)
		forward_call_flag = GLSCH_LOW;
	else if (forward_call_flag == GLSCH_NEG)
		forward_call_flag = GLSCH_NONE;
	else
		return -1;

	/* block here until the TOS ack N_SWITCH_TO_T_OS_STAGE2 */
	down(&(boot_sema));
	up(&(boot_sema));

	return 0;
}

/**
 * @brief
 */
static const struct file_operations teei_client_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = teei_client_ioctl,
	.compat_ioctl = teei_client_ioctl,
	.open = teei_client_open,
	.mmap = teei_client_mmap,
	.release = teei_client_release
};
#define TEEI_CONFIG_FULL_PATH_DEV_NAME "/dev/teei_config"
#define TEEI_CONFIG_DEV "teei_config"
#define TEEI_CONFIG_IOC_MAGIC 0x775B777E /* "TEEI Client" */


/**
 * @brief	Map the vma with the free pages
 *
 * @param	filp
 * @param	vma
 *
 * @return	0: success
 *		EINVAL: Invalid parament
 *		ENOMEM: No enough memory
 */
static int teei_config_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

static int init_teei_framework(void);
/**
 * @brief
 *
 * @param	file
 * @param	cmd
 * @param	arg
 *
 * @return
 */
#define TEEI_CONFIG_IOCTL_INIT_TEEI _IOWR(TEEI_CONFIG_IOC_MAGIC, 3, int)

unsigned int teei_flags = 0;
static long teei_config_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	int retVal = 0;
	void *argp = (void __user *) arg;

	switch (cmd) {

	case TEEI_CONFIG_IOCTL_INIT_TEEI:
			if (teei_flags == 1) {
				break;
			} else {
				init_teei_framework();
				teei_flags = 1;
			}

			break;
	default:
			printk("[%s][%d] command not found!\n", __func__, __LINE__);
			retVal = -EINVAL;
	}

	return retVal;
}

int is_teei_ready(void)
{
	return teei_flags;
}
EXPORT_SYMBOL(is_teei_ready);

/**
 * @brief		The open operation of /dev/teei_config device node.
 *
 * @param		inode
 * @param		file
 *
 * @return		ENOMEM: no enough memory in the linux kernel
 *			0: on success
 */

static int teei_config_open(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * @brief		The release operation of /dev/teei_config device node.
 *
 * @param		inode: device inode structure
 * @param		file:  struct file
 *
 * @return		0: on success
 */
static int teei_config_release(struct inode *inode, struct file *file)
{
	return 0;
}


/**
 * @brief
 */
static const struct file_operations teei_config_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = teei_config_ioctl,
	.open = teei_config_open,
	.mmap = teei_config_mmap,
	.release = teei_config_release
};

static void secondary_teei_invoke_drv(void)
{
	n_invoke_t_drv(0, 0, 0);
	return;
}


static void post_teei_invoke_drv(int cpu_id)
{
	get_online_cpus();
	smp_call_function_single(cpu_id,
			secondary_teei_invoke_drv,
			NULL,
			1);
	put_online_cpus();

	return;
}



static void teei_invoke_drv(void)
{
#if 0
	int cpu_id = smp_processor_id();
	/* int cpu_id = raw_smp_processor_id(); */

	if (cpu_id != 0) {
		/* call the mb function */
		mb();
		post_teei_invoke_drv(0); /* post it to primary */
	} else {
		/* printk("[%s][%d]\n", __func__, __LINE__); */
		n_invoke_t_drv(0, 0, 0); /* called directly on primary core */
	}
#else
	int cpu_id = 0;
	cpu_id = get_current_cpuid();
	post_teei_invoke_drv(cpu_id);
#endif
	return;
}

int __send_fp_command(unsigned long share_memory_size)
{

	/* down(&smc_lock); */
	/* down(&boot_sema); */

	set_fp_command(share_memory_size);
	Flush_Dcache_By_Area((unsigned long)fp_buff_addr, fp_buff_addr + FP_BUFF_SIZE);
	/* Flush_Dcache_By_Area((unsigned long)vfs_flush_address, vfs_flush_address + VFS_SIZE); */

#if 0
	teei_invoke_drv();
#else
	fp_call_flag = GLSCH_HIGH;
	n_invoke_t_drv(0, 0, 0);
#endif

	/* down(&boot_sema); */
	/* up(&boot_sema); */
	return 0;

}
struct fp_command_struct {
	unsigned long mem_size;
	int retVal;
};

struct fp_command_struct fp_command_entry;

static void secondary_send_fp_command(void *info)
{
	struct fp_command_struct *cd = (struct fp_command_struct *)info;

	/* with a rmb() */
	rmb();

	cd->retVal = __send_fp_command(cd->mem_size);

	/* with a wmb() */
	wmb();
}

int send_fp_command(unsigned long share_memory_size)
{
	int cpu_id = 0;
	int retVal = 0;
	struct fdrv_call_struct fdrv_ent;

	down(&fp_lock);
	mutex_lock(&pm_mutex);
	
	if (teei_config_flag == 1) {
		complete(&global_down_lock);
	}

	down(&smc_lock);
	down(&boot_sema);

#if 0
	fp_command_entry.mem_size = share_memory_size;
#else
	fdrv_ent.fdrv_call_type = FP_SYS_NO;
	fdrv_ent.fdrv_call_buff_size = share_memory_size;
#endif
	/* with a wmb() */
	wmb();

#if 0
	get_online_cpus();
	cpu_id = get_current_cpuid();
	smp_call_function_single(cpu_id, secondary_send_fp_command, (void *)(&fp_command_entry), 1);
	put_online_cpus();
#else
	Flush_Dcache_By_Area((unsigned long)&fdrv_ent, (unsigned long)&fdrv_ent + sizeof(struct fdrv_call_struct));
	retVal = add_work_entry(FDRV_CALL, (unsigned long)&fdrv_ent);
        if (retVal != 0) {
                mutex_unlock(&pm_mutex);
                up(&fp_lock);
                return retVal;
        }

#endif
	down(&boot_sema);
	up(&boot_sema);

	rmb();
	mutex_unlock(&pm_mutex);
	up(&fp_lock);

	return fdrv_ent.retVal;
}

struct boot_stage1_struct {
	unsigned long vir_addr;
};

struct boot_stage1_struct boot_stage1_entry;


static void secondary_boot_stage1(void *info)
{
	struct boot_stage1_struct *cd = (struct boot_stage1_struct *)info;

	/* with a rmb() */
	rmb();

	n_init_t_boot_stage1(cd->vir_addr, 0, 0);

	/* with a wmb() */
	wmb();

}


static void boot_stage1(unsigned long vir_address)
{
	int cpu_id = 0;
	boot_stage1_entry.vir_addr = vir_address;

	/* with a wmb() */
	wmb();

	get_online_cpus();
	cpu_id = get_current_cpuid();
	printk("current cpu id [%d]\n", cpu_id);
	smp_call_function_single(cpu_id, secondary_boot_stage1, (void *)(&boot_stage1_entry), 1);
	put_online_cpus();

	/* with a rmb() */
	rmb();
}


/**
 * @brief  init TEEI Framework
 * init Soter OS
 * init Global Schedule
 * init Forward Call Service
 * init CallBack Service
 * @return
 */

static int init_teei_framework(void)
{
	long retVal = 0;
	int i = 0;
	unsigned long secure_mem_p = 0;
	boot_soter_flag = START_STATUS;

	sema_init(&(boot_sema), 1);
	sema_init(&(fp_lock), 1);
	sema_init(&(api_lock), 1);
	register_boot_irq_handler();
	register_sched_irq_handler();
	register_switch_irq_handler();
	register_soter_irq_handler();
	register_fp_ack_handler();
	register_bdrv_handler();
	register_tlog_handler();
	register_error_irq_handler();

	secure_wq = create_workqueue("Secure Call");
	daulOS_VFS_write_share_mem = kmalloc(VDRV_MAX_SIZE, GFP_KERNEL);
	if (daulOS_VFS_write_share_mem == NULL) {
		printk("[%s][%d] kmalloc daulOS_VFS_write_share_mem failed!\n", __func__, __LINE__);
		return -1;
	}
	daulOS_VFS_read_share_mem = kmalloc(VDRV_MAX_SIZE, GFP_KERNEL);
	if (daulOS_VFS_read_share_mem == NULL) {
		printk("[%s][%d] kmalloc daulOS_VFS_read_share_mem failed!\n", __func__, __LINE__);
		return -1;
	}
	printk("[%s][%d]\n", __func__, __LINE__);
	printk("[%s][%d] VFS_SIZE = %d, SZ_4K = %d\n", __func__, __LINE__, VFS_SIZE, SZ_4K);
	printk("[%s][%d] ROUND_UP(VFS_SIZE, SZ_4K) = %d\n", __func__, __LINE__, ROUND_UP(VFS_SIZE, SZ_4K));
	printk("[%s][%d] get_order(ROUND_UP(VFS_SIZE, SZ_4K)) = %d\n", __func__, __LINE__, get_order(ROUND_UP(VFS_SIZE, SZ_4K)));
	boot_vfs_addr = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(VFS_SIZE, SZ_4K)));
	printk("[%s][%d]\n", __func__, __LINE__);
	if (boot_vfs_addr == NULL) {
		printk("[%s][%d]ERROR: There is no enough memory for booting Soter!\n", __func__, __LINE__);
		return -1;
	}
	printk("[%s][%d]\n", __func__, __LINE__);
	down(&(boot_sema));
	down(&(smc_lock));
	printk("[%s][%d]\n", __func__, __LINE__);
	/* n_init_t_boot_stage1((unsigned long)virt_to_phys(boot_vfs_addr), 0, 0); */

	boot_stage1((unsigned long)virt_to_phys(boot_vfs_addr));

	down(&(boot_sema));
	up(&(boot_sema));

	free_pages(boot_vfs_addr, get_order(ROUND_UP(VFS_SIZE, SZ_4K)));

	boot_soter_flag = END_STATUS;
		if (soter_error_flag == 1) {
			return -1;
		}
	/**printk("[%s][%d] begin to load Soter services.\n", __func__, __LINE__);
	**switch_to_t_os_stages2();
	**printk("[%s][%d] load Soter services successfully.\n", __func__, __LINE__);*/

	printk("[%s][%d] begin to create the command buffer!\n", __func__, __LINE__);
	retVal = create_cmd_buff();
	if (retVal < 0) {
		printk("[%s][%d] create_cmd_buff failed !\n", __func__, __LINE__);
		return retVal;
	}
	printk("[%s][%d] end of creating the command buffer!\n", __func__, __LINE__);

	printk("[%s][%d] begin to load Soter services.\n", __func__, __LINE__);
	switch_to_t_os_stages2();
	printk("[%s][%d] load Soter services successfully.\n", __func__, __LINE__);

		if (soter_error_flag == 1) {
		return -1;
		}
	init_smc_work();

	printk("[%s][%d] begin to init daulOS services.\n", __func__, __LINE__);
	retVal = teei_service_init();
	if (retVal == -1)
		return -1;

	printk("[%s][%d] init daulOS services successfully.\n", __func__, __LINE__);

	printk("[%s][%d] begin to load TEEs.\n", __func__, __LINE__);
	t_os_load_image();
	if (soter_error_flag == 1)
		return -1;

	printk("[%s][%d] load TEEs successfully.\n", __func__, __LINE__);

	teei_config_flag = 1;

	return 0;
}
static dev_t teei_config_device_no;
static struct cdev teei_config_cdev;
static struct class *config_driver_class;


/**
 * @brief TEEI Agent Driver initialization
 * initialize Microtrust Tee environment
 * @return
 **/
static int teei_config_init(void)
{
	int ret_code = 0;
	long retVal = 0;
	struct device *class_dev = NULL;

	ret_code = alloc_chrdev_region(&teei_config_device_no, 0, 1, TEEI_CONFIG_DEV);
	if (ret_code < 0) {
		printk("alloc_chrdev_region failed %x.\n", ret_code);
		return ret_code;
	}

	config_driver_class = class_create(THIS_MODULE, TEEI_CONFIG_DEV);
	if (IS_ERR(config_driver_class)) {
		ret_code = -ENOMEM;
		printk("class_create failed %x\n", ret_code);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(config_driver_class, NULL, teei_config_device_no, NULL, TEEI_CONFIG_DEV);
	if (NULL == class_dev) {
		printk("class_device_create failed %x\n", ret_code);
		ret_code = -ENOMEM;
		goto class_destroy;
	}

	cdev_init(&teei_config_cdev, &teei_config_fops);
	teei_config_cdev.owner = THIS_MODULE;

	ret_code = cdev_add(&teei_config_cdev, MKDEV(MAJOR(teei_config_device_no), 0), 1);
	if (ret_code < 0) {
		printk("cdev_add failed %x\n", ret_code);
		goto class_device_destroy;
	}

	goto return_fn;

class_device_destroy:
	device_destroy(driver_class, teei_config_device_no);
class_destroy:
	class_destroy(driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(teei_config_device_no, 1);
return_fn:
	return ret_code;
}



static int teei_cpu_id[] = {0x0000, 0x0001, 0x0002, 0x0003, 0x0100, 0x0101, 0x0102, 0x0103};
static int __cpuinit tz_driver_cpu_callback(struct notifier_block *self,
		unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	/*unsigned int sched_cpu = 4;*/
	unsigned int sched_cpu = get_current_cpuid();
	struct cpumask mtee_mask = { CPU_BITS_NONE };
	int retVal = 0;
	int i;
	switch (action) {
	case CPU_DOWN_PREPARE:
	case CPU_DOWN_PREPARE_FROZEN:
			if (cpu == sched_cpu) {
				printk("cpu down prepare ************************\n");
				retVal = down_trylock(&smc_lock);
				if (retVal == 1) {
					return NOTIFY_BAD;
				}
				else {
					cpu_notify_flag = 1;
					for_each_online_cpu(i) {
						printk("current on line cpu [%d]\n", i);
						if (i == cpu) {
							continue;
						}
						current_cpu_id = i;
					}
#if 1
					printk("[%s][%d]brefore cpumask set cpu\n", __func__, __LINE__);
					cpumask_set_cpu(current_cpu_id, &mtee_mask);
					/*cpumask_set_cpu(current_cpu_id, &mask);*/
					printk("[%s][%d]after cpumask set cpu\n", __func__, __LINE__);
#if 0
					if (sched_setaffinity(sub_pid, &mtee_mask) == -1)
						printk("warning: could not set CPU affinity, continuing...\n");
#endif

					set_cpus_allowed(teei_switch_task, mtee_mask);
#endif
					/* TODO smc_call to notify ATF to switch the CPU*/
					/* NT_switch_T(current_cpu_id);*/
					printk("current cpu id  \n");
					nt_sched_core(teei_cpu_id[current_cpu_id], teei_cpu_id[cpu], 0);
					printk("change cpu id = [%d]\n", current_cpu_id);
				}
			}
			break;

	case CPU_DOWN_FAILED:
			if (cpu_notify_flag == 1) {
				printk("cpu down failed *************************\n");
				up(&smc_lock);
				cpu_notify_flag = 0;
			}
			break;

	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
			if (cpu_notify_flag == 1) {
				printk("cpu down success ***********************\n");
				up(&smc_lock);
				cpu_notify_flag = 0;
			}
			break;
	}

	return NOTIFY_OK;
}

/**
 * @brief TEEI Agent Driver initialization
 * initialize service framework
 * @return
 */
static int teei_client_init(void)
{
	int ret_code = 0;
	long retVal = 0;
	struct device *class_dev = NULL;


	long prior = 0;

	unsigned long irq_status = 0;

	/* printk("TEEI Agent Driver Module Init ...\n"); */
	printk("=====================================\n");
	printk("~~~~~~~uTos version V0.6~~~~~~~\n");
	printk("=====================================\n");
	ret_code = alloc_chrdev_region(&teei_client_device_no, 0, 1, TEEI_CLIENT_DEV);
	if (ret_code < 0) {
		printk("alloc_chrdev_region failed %x\n", ret_code);
		return ret_code;
	}

	driver_class = class_create(THIS_MODULE, TEEI_CLIENT_DEV);
	if (IS_ERR(driver_class)) {
		ret_code = -ENOMEM;
		printk("class_create failed %x\n", ret_code);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, teei_client_device_no, NULL, TEEI_CLIENT_DEV);
	if (NULL == class_dev) {
		printk("class_device_create failed %x\n", ret_code);
		ret_code = -ENOMEM;
		goto class_destroy;
	}

	cdev_init(&teei_client_cdev, &teei_client_fops);
	teei_client_cdev.owner = THIS_MODULE;

	ret_code = cdev_add(&teei_client_cdev, MKDEV(MAJOR(teei_client_device_no), 0), 1);
	if (ret_code < 0) {
		printk("cdev_add failed %x\n", ret_code);
		goto class_device_destroy;
	}

	memset(&teei_contexts_head, 0, sizeof(teei_contexts_head));

	teei_contexts_head.dev_file_cnt = 0;
	init_rwsem(&teei_contexts_head.teei_contexts_sem);

	INIT_LIST_HEAD(&teei_contexts_head.context_list);

	init_tlog_entry();

	sema_init(&(smc_lock), 1);
	//sema_init(&(global_down_lock), 0);
	int i;
	for_each_online_cpu(i) {
		current_cpu_id = i;
		printk("init stage : current_cpu_id = %d\n", current_cpu_id);
	}
	printk("begin to create sub_thread.\n");
#if 0
	sub_pid = kernel_thread(global_fn, NULL, CLONE_KERNEL);
	retVal = sys_setpriority(PRIO_PROCESS, sub_pid, -3);
#else
	//struct sched_param param = {.sched_priority = -20 };
	teei_fastcall_task = kthread_create(global_fn, NULL, "teei_fastcall_thread");
	if (IS_ERR(teei_fastcall_task)) {
		printk("create fastcall thread failed: %d\n", PTR_ERR(teei_fastcall_task));
		goto fastcall_thread_fail;
	}

	//sched_setscheduler_nocheck(teei_fastcall_task, SCHED_NORMAL, &param);
	//get_task_struct(teei_fastcall_task);
	wake_up_process(teei_fastcall_task);
#endif
	printk("create the sub_thread successfully!\n");
#if 0
	cpumask_set_cpu(get_current_cpuid(), &mask);

	retVal = sched_setaffinity(sub_pid, &mask);
	if (retVal != 0)
		printk("warning: could not set CPU affinity, retVal = [%l] continuing...\n",  retVal);
#endif

	/* create the switch thread */
        teei_switch_task = kthread_create(kthread_worker_fn, &ut_fastcall_worker, "teei_switch_thread");
        if (IS_ERR(teei_switch_task)) {
                printk("create switch thread failed: %ld\n", PTR_ERR(teei_switch_task));
		teei_switch_task = NULL;
                goto fastcall_thread_fail;
        }

        //sched_setscheduler_nocheck(teei_switch_task, SCHED_NORMAL, &param);
        //get_task_struct(teei_switch_task);
        wake_up_process(teei_switch_task);


	cpumask_set_cpu(get_current_cpuid(), &mask);
        set_cpus_allowed(teei_switch_task, mask);


	register_cpu_notifier(&tz_driver_cpu_notifer);
	printk("after  register cpu notify\n");
	teei_config_init();

	goto return_fn;

fastcall_thread_fail:
class_device_destroy:
	device_destroy(driver_class, teei_client_device_no);
class_destroy:
	class_destroy(driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(teei_client_device_no, 1);
return_fn:
	return ret_code;
}

/**
 * @brief
 */
static void teei_client_exit(void)
{
	TINFO("teei_client exit");
	device_destroy(driver_class, teei_client_device_no);
	class_destroy(driver_class);
	unregister_chrdev_region(teei_client_device_no, 1);
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("TEEI <www.microtrust.com>");
MODULE_DESCRIPTION("TEEI Agent");
MODULE_VERSION("1.00");
module_init(teei_client_init);
module_exit(teei_client_exit);


