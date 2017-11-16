#ifndef TEEI_SMC_CALL_H
#define TEEI_SMC_CALL_H
#include <linux/semaphore.h>

#define NQ_VALID                                1

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

extern int teei_smc_call(u32 teei_cmd_type,
			u32 dev_file_id,
			u32 svc_id,
			u32 cmd_id,
			u32 context,
			u32 enc_id,
			void *cmd_buf,
			size_t cmd_len,
			void *resp_buf,
			size_t resp_len,
			void *meta_data,
			void *info_data,
			size_t info_len,
			int *ret_resp_len,
			int *error_code,
			struct semaphore *psema);

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
			struct semaphore *psema);

#endif /* end of TEEI_SMC_CALL_H */
