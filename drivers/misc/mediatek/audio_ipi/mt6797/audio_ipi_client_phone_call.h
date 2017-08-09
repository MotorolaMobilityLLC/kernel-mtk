#ifndef AUDIO_IPI_CLIENT_PHONE_CALL_H
#define AUDIO_IPI_CLIENT_PHONE_CALL_H

#include <linux/fs.h>           /* needed by file_operations* */

void audio_ipi_client_phone_call_init(void);
void audio_ipi_client_phone_call_deinit(void);

ssize_t audio_ipi_client_phone_call_read(
	struct file *file, char *buf, size_t count, loff_t *ppos);

void open_dump_file(void);
void close_dump_file(void);


#endif /* end of AUDIO_IPI_CLIENT_PHONE_CALL_H */

