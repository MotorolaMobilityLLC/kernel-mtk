#ifndef HACC_TEE_H
#define HACC_TEE_H

extern u32 get_devinfo_with_index(u32 index);
int open_sdriver_connection(void);
int tee_secure_request(unsigned int user, unsigned char *data, unsigned int data_size,
		       unsigned int direction, unsigned char *seed, unsigned int seed_size);
int close_sdriver_connection(void);

#endif
