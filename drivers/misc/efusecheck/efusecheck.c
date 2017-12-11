#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/of_fdt.h>


#define STRINGLEN 1024

char global_buffer[STRINGLEN] = {0};
char secure_boot_enable[32] =  "secure_boot, Yes";
char secure_boot_disable[32] = "secure_boot, No";
unsigned int g_sbc_flag;

#if 0
struct proc_dir_entry *efuse_dir = NULL;
struct proc_dir_entry *efuse_file = NULL;
#else
struct proc_dir_entry * efuse_entry = NULL;
#endif

#if 0
int proc_read_efuse(char *page, char **start, off_t off, int count, int *eof,
                void *data) {
    int len;
    len = sprintf(page, global_buffer);
    return len;
}

int proc_write_efuse(struct file *file, const char *buffer, unsigned long count,
                void *data) {
    int len;
    if (count = STRINGLEN)
        len = STRINGLEN-1;
    else
        len = count;
    copy_from_user(global_buffer, buffer, len);
    global_buffer[len] = '\0';
    return len;
}
#else
struct boot_tag_sbc_flag {
	u32 size;
	u32 tag;
	u32 sbc_flag;
};
static unsigned long sbc_node;

static int sbc_early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
        if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
                return 0;

        sbc_node = node;
        return 1;
}
static void dt_get_sbcboot_flag(void)
{
        struct boot_tag_sbc_flag *tags = NULL;
	if (of_scan_flat_dt(sbc_early_init_dt_get_chosen, NULL) > 0) {

	        tags = (struct boot_tag_sbc_flag *)of_get_flat_dt_prop(sbc_node, "atag,sbc_flag", NULL);

        	if (tags) {
                	g_sbc_flag = tags->sbc_flag;
        	} else {
			g_sbc_flag = 0;
                	printk("[Secure Boot]:Lenovo-sw quebs2 logged,'atag,sbc_flag' is not found\n");
        	}
	} 

	return;
}
ssize_t efuse_proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    /*Fetch sbc flag from LK DTS*/
    dt_get_sbcboot_flag();

    if (g_sbc_flag == 1) {
        printk("[Secure Boot]:Lenovo-sw quebs2 logged, Fuse has been blown with SBC \n");
        if (*f_pos > 0)
            return 0;
        if (copy_to_user((char*)buf,secure_boot_enable,strlen(secure_boot_enable)))
            return -EFAULT;
        *f_pos += strlen(secure_boot_enable);
        return strlen(secure_boot_enable);
    }
    else
    {
        printk("[Secure Boot]:Lenovo-sw quebs2 logged, Fuse has not been blown yet\n");
        if (*f_pos > 0)
            return 0;
        if (copy_to_user((char*)buf,secure_boot_disable,strlen(secure_boot_disable)))
            return -EFAULT;
        *f_pos += strlen(secure_boot_disable);
        return strlen(secure_boot_disable);
    }
}

ssize_t efuse_proc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int len;
    if (count == STRINGLEN)
        len = STRINGLEN-1;
    else
        len = count;
    if(copy_from_user(global_buffer, buf, len))
	return -EFAULT;
    global_buffer[len] = '\0';
    return len;
}

ssize_t efuse_proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t efuse_proc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static struct file_operations efuse_file_fops = {
        .read = efuse_proc_read,
        .write = efuse_proc_write,
};
#endif

static int __init proc_efuse_init(void) {
#if 0
    efuse_dir = proc_mkdir("efuse", NULL);
    efuse_file = create_proc_entry("sbcflag", S_IRUGO, NULL);
    strcpy(global_buffer, "hello");
    efuse_file->read_proc = proc_read_efuse;
    efuse_file->write_proc = proc_write_efuse;
#else
    efuse_entry = proc_create("efuse_flag", 0664, NULL, &efuse_file_fops);
    if (efuse_entry == NULL) {
        printk("[Secure Boot]:Unable to create /proc entry\n\r");
        return -1;
    }

    return 0;
#endif
}

static void __exit proc_efuse_exit(void) {
#if 0
    remove_proc_entry("sbcflag", efuse_dir);
    remove_proc_entry("efuse", NULL);
#else
    if (NULL != efuse_entry) {
        proc_remove(efuse_entry);
    }
#endif
}

module_init(proc_efuse_init);
module_exit(proc_efuse_exit);
