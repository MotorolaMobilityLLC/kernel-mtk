#ifndef NOTIFY_QUEUE_H
#define NOTIFY_QUEUE_H

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

struct create_NQ_struct {
	unsigned int n_t_nq_phy_addr;
	unsigned int n_t_size;
	unsigned int t_n_nq_phy_addr;
	unsigned int t_n_size;
};

extern unsigned long t_nt_buffer;

int add_nq_entry(u32 command_buff, int command_length, int valid_flag);
unsigned char *get_nq_entry(unsigned char *buffer_addr);
long create_nq_buffer(void);

#endif // end of NOTIFY_QUEUE_H
