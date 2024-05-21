
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "omnivision_tcm_testing.h"

#define STRTOL_LEN 10

static void copy_this_line(char *dest, char *src)
{
	char *copy_from;
	char *copy_to;

	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while((*copy_from != '\n') && (*copy_from != '\r') && (*copy_from != '\0'));
	*copy_to = '\0';
}

static void goto_next_line(char **ptr)
{
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n' && **ptr != '\0');
	if (**ptr == '\0') {
		return;
	}
	*ptr = *ptr + 1;
}

static void parse_valid_data(char *buf_start, loff_t buf_size,
				char *ptr, int32_t* data, int rows)
{
	int i = 0;
	int j = 0;
	char *token = NULL;
	char *tok_ptr = NULL;
	char row_data[512] = {0};

	if(!ptr) {
		printk("ovt tcm csv parser:  %s, ptr is NULL\n", __func__);
		return;
	}
	if (!data) {
		printk("ovt tcm csv parser:  %s, data is NULL\n", __func__);
		return;
	}

	for (i = 0; i < rows; i++) {
		// copy this line to row_data buffer
		memset(row_data, 0, sizeof(row_data));
		copy_this_line(row_data, ptr);
		tok_ptr = row_data;
		while ((token = strsep(&tok_ptr,", \t\n\r\0"))) {
			if (strlen(token) == 0)
				continue;

			data[j] = (int32_t)simple_strtol(token, NULL, STRTOL_LEN);
			j ++;
		}
		goto_next_line(&ptr);				//next row
		if(!ptr || (0 == strlen(ptr)) || (ptr >= (buf_start + buf_size))) {
			printk("ovt tcm csv parser: invalid ptr, return\n");
			break;
		}
	}

	return;
}

static void print_data(char* target_name, int32_t* data, int rows, int columns)
{
	int i,j;

    printk("ovt tcm csv parser:  print data %s\n", target_name);
	if(NULL == data) {
		printk("ovt tcm csv parser:  rawdata is NULL\n");
		return;
	}

	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			printk("\t%d", data[i*columns + j]);
		}
		printk("\n");
	}

	return;
}


int ovt_tcm_parse_csvfile(char *file_path, char *target_name, int32_t  *data, int rows, int columns)
{
	struct file *fp = NULL;
	struct kstat stat;
	int ret = 0;
	int32_t read_ret = 0;
	char *buf = NULL;
	char *ptr = NULL;
	mm_segment_t org_fs;
	loff_t pos = 0;
	org_fs = get_fs();
	set_fs(KERNEL_DS);

	if(NULL == file_path) {
		printk("ovt tcm csv parser:  file path pointer is NULL\n");
		ret = -EPERM;
		goto exit_free;
	}

	if(NULL == target_name) {
		printk("ovt tcm csv parser:  target path pointer is NULL\n");
		ret = -EPERM;
		goto exit_free;
	}

	printk("ovt tcm csv parser: %s, file name is %s, target is %s.\n", __func__,file_path,target_name);

	fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		printk("ovt tcm csv parser:  %s, filp_open error, file name is %s.\n", __func__,file_path);
		ret = -EPERM;
		goto exit_free;
	}

	ret = vfs_stat(file_path, &stat);
	if(ret) {
		printk("ovt tcm csv parser:  %s, failed to get file stat.\n", __func__);
		ret = -ENOENT;
		goto exit_free;
	}

	buf = (char *)kzalloc(stat.size + 1, GFP_KERNEL);
	if(NULL == buf) {
		printk("ovt tcm csv parser:  %s: kzalloc %lld bytes failed.\n", __func__, stat.size);
		ret = -ESRCH;
		goto exit_free;
	}

	read_ret = vfs_read(fp, buf, stat.size, &pos);
	if (read_ret > 0) {
		buf[stat.size] = '\0';
		ptr = buf;
		ptr = strstr(ptr, target_name);
		if (ptr == NULL) {
			printk("ovt tcm csv parser:  %s: load %s failed 1!\n", __func__,target_name);
			ret = -EINTR;
			goto exit_free;
		}

	      // walk thru this line
		goto_next_line(&ptr);
		if ((NULL == ptr) || (0 == strlen(ptr))) {
			printk("ovt tcm csv parser:  %s: load %s failed 2!\n", __func__,target_name);
			ret = -EIO;
			goto exit_free;
		}

		//analyze the data
		if (data) {
			parse_valid_data(buf, stat.size, ptr, data, rows);
			print_data(target_name, data,  rows, columns);
		}else{
			printk("ovt tcm csv parser:  %s: load %s failed 3!\n", __func__,target_name);
			ret = -EINTR;
			goto exit_free;
		}
	}
	else {
		printk("ovt tcm csv parser:  %s: ret=%d,read_ret=%d, buf=%p, stat.size=%lld\n", __func__, ret, read_ret, buf, stat.size);
		ret = -ENXIO;
		goto exit_free;
	}
	ret = 0;
exit_free:
	printk("ovt tcm csv parser: %s exit free\n", __func__);
	set_fs(org_fs);
	if(buf) {
		printk("ovt tcm csv parser: kfree buf\n");
		kfree(buf);
		buf = NULL;
	}

	if (!IS_ERR_OR_NULL(fp)) {		//fp open fail not means fp is NULL, so free fp may cause Uncertainty
		printk("ovt tcm csv parser: filp close\n");
		filp_close(fp, NULL);
		fp = NULL;
	}
	return ret;
}


void ovt_tcm_store_to_buf(char *buffer, char* format, ...)
{
    va_list args;
    char buf[TMP_STRING_LEN_FOR_CSV] = {0};
    static int count = 0;

    if (buffer == NULL) {
        count = 0;
        return;
    }

    va_start(args, format);
    vsnprintf(buf, TMP_STRING_LEN_FOR_CSV, format, args);
    va_end(args);

    if (count + strlen(buf) + 2 > MAX_BUFFER_SIZE_FOR_CSV) {
        pr_err("exceed the max buffer size\n");
        count = 0;
        return;
    }

    memcpy(buffer + count, buf, strlen(buf));
    count += strlen(buf);
    *(buffer + count) = '\n';
    count++;
}

void ovt_tcm_store_to_file(struct file *fp, char* format, ...)
{
    va_list args;
    char buf[TMP_STRING_LEN_FOR_CSV] = {0};
    // mm_segment_t fs;  
    loff_t pos;
	// struct file *fp;

    va_start(args, format);
    vsnprintf(buf, TMP_STRING_LEN_FOR_CSV, format, args);
    va_end(args);


    // fp = filp_open(file_path, O_RDWR | O_CREAT, 0666);  
    // if (IS_ERR(fp)) {  
    //     printk("ovt tcm create file error\n");  
    //     return;  
    // } 

    // fs = get_fs();  
    // set_fs(KERNEL_DS);


	buf[TMP_STRING_LEN_FOR_CSV - 1] = 0;

	pos = fp->f_pos;
    vfs_write(fp, buf, strlen(buf), &pos);  
    fp->f_pos = pos;

    // set_fs(fs);

    // filp_close(fp, NULL);  
    return;  
}
