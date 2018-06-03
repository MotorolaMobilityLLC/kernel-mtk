#ifndef TEEI_FUNC_H
#define TEEI_FUNC_H

#define MICROTRUST_FP_SIZE			    0x80000
#define CMD_MEM_CLEAR		_IO(0x5A777E, 0x1)
#define CMD_FP_CMD      	_IO(0x5A777E, 0x2)
#define CMD_GATEKEEPER_CMD	_IO(0x5A777E, 0x3)
#define CMD_LOAD_TEE		_IO(0x5A777E, 0x4)
#define FP_MAJOR		   254
#define SHMEM_ENABLE       0
#define SHMEM_DISABLE      1
#define DEV_NAME 		   "teei_fp"
#define FP_DRIVER_ID 	   100
#define GK_DRIVER_ID 	   120

extern wait_queue_head_t __fp_open_wq;
extern int enter_tui_flag;

int send_fp_command(unsigned long share_memory_size);

#endif /* end of TEEI_FUNC_H*/
