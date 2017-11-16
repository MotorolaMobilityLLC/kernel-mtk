#ifndef TEEI_CAPI_H
#define TEEI_CAPI_H
#include "teei_smc_call.h"
struct teei_contexts_head_t {
	u32 dev_file_cnt;
	struct list_head context_list;
	struct rw_semaphore teei_contexts_sem;
};
extern struct teei_context *teei_create_context(int dev_count);
extern struct teei_session *teei_create_session(struct teei_context *cont);
extern struct teei_contexts_head_t teei_contexts_head;

extern void tz_free_shared_mem(void *addr, size_t size);
extern void *tz_malloc_shared_mem(size_t size, int flags);

#endif /* end of TEEI_CAPI_H */
