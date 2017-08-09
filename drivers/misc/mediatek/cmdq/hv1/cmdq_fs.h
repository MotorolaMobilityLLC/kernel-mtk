#ifndef __CMDQ_FS_H__
#define __CMDQ_FS_H__
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/uaccess.h>

#define MAX_SIZE 100

struct fs_struct {
	mm_segment_t fs;
	struct file *fp;
	void (*fs_create)(struct fs_struct *file, const char *fileName);
	void (*fs_write)(struct fs_struct *file, char *buffer);
	void (*fs_close)(struct fs_struct *file);
};

void init_fs_struct(struct fs_struct *file);


#define fs_printf(file, buffer, args...)		\
do {											\
	char writeBuffer[100];						\
	memset(writeBuffer, 0, 100);				\
	sprintf(writeBuffer, buffer, ##args);		\
	file.fs_write(&file, writeBuffer);			\
} while (0)

#endif
