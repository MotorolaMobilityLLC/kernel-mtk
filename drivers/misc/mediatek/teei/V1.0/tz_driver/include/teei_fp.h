#ifndef TEEI_FP_H
#define TEEI_FP_H


struct fp_command_struct {
	unsigned long mem_size;
	int retVal;
};

extern unsigned long fp_buff_addr;
extern struct mutex pm_mutex;

unsigned long create_fp_fdrv(int buff_size);
int __send_fp_command(unsigned long share_memory_size);
int __send_fp_command(unsigned long share_memory_size);

#endif  // end of TEEI_FP_H
