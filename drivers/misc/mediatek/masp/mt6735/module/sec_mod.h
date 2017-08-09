#ifndef SECMOD_H
#define SECMOD_H

#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>

struct sec_ops {
	int (*sec_get_rid)(unsigned int *rid);
};

struct sec_mod {
	dev_t id;
	int init;
	spinlock_t lock;
	const struct sec_ops *ops;
};

/**************************************************************************
 *  EXTERNAL VARIABLE
 **************************************************************************/
extern const struct sec_ops *sec_get_ops(void);
extern bool bMsg;
extern struct semaphore hacc_sem;
/**************************************************************************
 *  EXTERNAL FUNCTION
 **************************************************************************/
extern int sec_get_random_id(unsigned int *rid);
extern long sec_core_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
extern void sec_core_init(void);
extern void sec_core_exit(void);
#endif				/* end of SECMOD_H */
