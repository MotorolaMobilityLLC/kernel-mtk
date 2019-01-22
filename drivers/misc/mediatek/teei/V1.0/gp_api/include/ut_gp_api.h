#include "teei_client.h"
#include "tee_client_api.h"

extern int ut_client_shared_mem_free(unsigned long dev_file_id, struct teei_session_shared_mem_info *mem_info);
extern long ut_client_open_dev(void);
extern int ut_client_session_close(unsigned long dev_file_id, struct ser_ses_id *ses_close);
extern int ut_client_context_close(unsigned long dev_file_id, struct ctx_data *ctx);
extern int ut_client_context_init(unsigned long dev_file_id, struct ctx_data *ctx);
extern int ut_client_release(long dev_file_id);
extern void *ut_client_map_mem(int dev_file_id, unsigned long size);
extern int ut_client_encode_mem_ref_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc);
extern int ut_client_session_init(unsigned long dev_file_id, struct user_ses_init *ses_init);
extern int ut_client_encode_uint32_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc);
extern int ut_client_decode_uint32(unsigned long dev_file_id, struct teei_client_encode_cmd *enc);
extern int ut_client_session_open(unsigned long dev_file_id, struct ser_ses_id *ses_open);
extern int ut_client_operation_release(unsigned long dev_file_id, struct teei_client_encode_cmd *enc);
extern inline uint32_t ut_enc_memory(int dev_file_id, EncodeCmd *enc, TEEC_Parameter params,
			uint32_t *returnOrigin, uint32_t inout, uint8_t isTemp);
extern uint32_t ut_dec_value(EncodeCmd *enc, int dev_file_id, uint32_t *returnOrigin);
extern int ut_client_decode_array_space(unsigned long dev_file_id, struct teei_client_encode_cmd *enc);
extern int ut_client_send_cmd(unsigned long dev_file_id, struct teei_client_encode_cmd *enc);

