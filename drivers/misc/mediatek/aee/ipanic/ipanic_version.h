#if !defined(__AEE_IPANIC_VERSION_H__)
#define __AEE_IPANIC_VERSION_H__

#include <linux/version.h>
#include <linux/kmsg_dump.h>

struct ipanic_log_index {
	u32 idx;
	u64 seq;
};

extern u32 log_first_idx;
extern u64 log_first_seq;
extern u32 log_next_idx;
extern u64 log_next_seq;
int ipanic_kmsg_dump3(struct kmsg_dumper *dumper, char *buf, size_t len);

#endif
