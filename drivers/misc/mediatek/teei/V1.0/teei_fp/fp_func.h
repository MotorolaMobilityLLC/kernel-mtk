#ifndef TEEI_FUNC_H
#define TEEI_FUNC_H

#include <teei_ioc.h>
#define TEEI_IOC_MAXNR		(4)
#define MICROTRUST_FP_SIZE	0x80000
#define FP_BUFFER_OFFSET 	(0x10)
#define FP_LEN_MAX 			(MICROTRUST_FP_SIZE - FP_BUFFER_OFFSET)
#define FP_LEN_MIN 			0
#define FP_MAJOR		254
#define SHMEM_ENABLE		0
#define SHMEM_DISABLE		1
#define DEV_NAME		"teei_fp"
#define FP_DRIVER_ID		100
#define GK_DRIVER_ID		120

extern wait_queue_head_t __fp_open_wq;
extern int enter_tui_flag;

int send_fp_command(unsigned long share_memory_size);

#endif /* end of TEEI_FUNC_H */
