#ifndef TEEI_SMC_CALL_H
#define TEEI_SMC_CALL_H
#include <linux/semaphore.h>

#define NQ_VALID    1


/******************************
 * Message header
 ******************************/

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
                     struct semaphore *psema);

#endif // end of TEEI_SMC_CALL_H
