/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2012-2019, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
*
* File Name: Focaltech_ex_fun.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "focaltech_core.h"
 #include <linux/major.h>

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define PROC_UPGRADE                            0
#define PROC_READ_REGISTER                      1
#define PROC_WRITE_REGISTER                     2
#define PROC_AUTOCLB                            4
#define PROC_UPGRADE_INFO                       5
#define PROC_WRITE_DATA                         6
#define PROC_READ_DATA                          7
#define PROC_SET_TEST_FLAG                      8
#define PROC_SET_SLAVE_ADDR                     10
#define PROC_HW_RESET                           11
#define PROC_READ_STATUS                        12
#define PROC_SET_BOOT_MODE                      13
#define PROC_ENTER_TEST_ENVIRONMENT             14
#define PROC_NAME                               "ftxxxx-debug"
#define PROC_BUF_SIZE                           256
#define FTS_POWERMAX "touch_powermax"
#define FTS_POWERON "touch_poweron"
#define FTS_SLEEP "touch_sleep"
#define FTS_RESET "touch_reset"

static struct proc_dir_entry *FTS_proc_powermax_enter;
static struct proc_dir_entry *FTS_proc_poweron_enter;
static struct proc_dir_entry *FTS_proc_sleep_entry;
static struct proc_dir_entry *FTS_proc_reset_entry;

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
enum {
    RWREG_OP_READ = 0,
    RWREG_OP_WRITE = 1,
};

/*****************************************************************************
* Static variables
*****************************************************************************/
static struct rwreg_operation_t {
    int type;           /*  0: read, 1: write */
    int reg;            /*  register */
    int len;            /*  read/write length */
    int val;            /*  length = 1; read: return value, write: op return */
    int res;            /*  0: success, otherwise: fail */
    char *opbuf;        /*  length >= 1, read return value, write: op return */
} rw_op;

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static ssize_t fts_debug_write(
    struct file *filp, const char __user *buff, size_t count, loff_t *ppos)
{
    u8 *writebuf = NULL;
    u8 tmpbuf[PROC_BUF_SIZE] = { 0 };
    int buflen = count;
    int writelen = 0;
    int ret = 0;
    char tmp[PROC_BUF_SIZE];
    struct fts_ts_data *ts_data = fts_data;
    struct ftxxxx_proc *proc = &ts_data->proc;

    if ((buflen <= 1) || (buflen > PAGE_SIZE)) {
        FTS_ERROR("apk proc wirte count(%d>%d) fail", buflen, (int)PAGE_SIZE);
        return -EINVAL;
    }

    if (buflen > PROC_BUF_SIZE) {
        writebuf = (u8 *)kzalloc(buflen * sizeof(u8), GFP_KERNEL);
        if (NULL == writebuf) {
            FTS_ERROR("apk proc wirte buf zalloc fail");
            return -ENOMEM;
        }
    } else {
        writebuf = tmpbuf;
    }

    if (copy_from_user(writebuf, buff, buflen)) {
        FTS_ERROR("[APK]: copy from user error!!");
        ret = -EFAULT;
        goto proc_write_err;
    }

    proc->opmode = writebuf[0];
    switch (proc->opmode) {
    case PROC_SET_TEST_FLAG:
        FTS_DEBUG("[APK]: PROC_SET_TEST_FLAG = %x", writebuf[1]);
        if (writebuf[1] == 0) {
#if FTS_ESDCHECK_EN
            fts_esdcheck_switch(ENABLE);
#endif
        } else {
#if FTS_ESDCHECK_EN
            fts_esdcheck_switch(DISABLE);
#endif
        }
        break;

    case PROC_READ_REGISTER:
        proc->cmd[0] = writebuf[1];
        break;

    case PROC_WRITE_REGISTER:
        ret = fts_write_reg(writebuf[1], writebuf[2]);
        if (ret < 0) {
            FTS_ERROR("PROC_WRITE_REGISTER write error");
            goto proc_write_err;
        }
        break;

    case PROC_READ_DATA:
        writelen = buflen - 1;
        memcpy(proc->cmd, writebuf + 1, writelen);
        proc->cmd_len = writelen;
        break;

    case PROC_WRITE_DATA:
        writelen = buflen - 1;
        ret = fts_write(writebuf + 1, writelen);
        if (ret < 0) {
            FTS_ERROR("PROC_WRITE_DATA write error");
            goto proc_write_err;
        }
        break;

    case PROC_SET_SLAVE_ADDR:
        break;

    case PROC_HW_RESET:
        snprintf(tmp, PROC_BUF_SIZE, "%s", writebuf + 1);
        tmp[buflen - 1] = '\0';
        if (strncmp(tmp, "focal_driver", 12) == 0) {
            FTS_INFO("APK execute HW Reset");
            fts_reset_proc(0);
        }
        break;

    case PROC_SET_BOOT_MODE:
        FTS_DEBUG("[APK]: PROC_SET_BOOT_MODE = %x", writebuf[1]);
        if (0 == writebuf[1]) {
            ts_data->fw_is_running = true;
        } else {
            ts_data->fw_is_running = false;
        }
        break;
    case PROC_ENTER_TEST_ENVIRONMENT:
        FTS_DEBUG("[APK]: PROC_ENTER_TEST_ENVIRONMENT = %x", writebuf[1]);
        if (0 == writebuf[1]) {
            fts_enter_test_environment(0);
        } else {
            fts_enter_test_environment(1);
        }
        break;

    default:
        break;
    }

    ret = buflen;
proc_write_err:
    if ((buflen > PROC_BUF_SIZE) && writebuf) {
        kfree(writebuf);
        writebuf = NULL;
    }
    return ret;
}

static ssize_t fts_debug_read(
    struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
    int ret = 0;
    int num_read_chars = 0;
    int buflen = count;
    u8 *readbuf = NULL;
    u8 tmpbuf[PROC_BUF_SIZE] = { 0 };
    struct fts_ts_data *ts_data = fts_data;
    struct ftxxxx_proc *proc = &ts_data->proc;

    if ((buflen <= 0) || (buflen > PAGE_SIZE)) {
        FTS_ERROR("apk proc read count(%d>%d) fail", buflen, (int)PAGE_SIZE);
        return -EINVAL;
    }

    if (buflen > PROC_BUF_SIZE) {
        readbuf = (u8 *)kzalloc(buflen * sizeof(u8), GFP_KERNEL);
        if (NULL == readbuf) {
            FTS_ERROR("apk proc wirte buf zalloc fail");
            return -ENOMEM;
        }
    } else {
        readbuf = tmpbuf;
    }

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(1);
#endif

    switch (proc->opmode) {
    case PROC_READ_REGISTER:
        num_read_chars = 1;
        ret = fts_read_reg(proc->cmd[0], &readbuf[0]);
        if (ret < 0) {
            FTS_ERROR("PROC_READ_REGISTER read error");
            goto proc_read_err;
        }
        break;
    case PROC_WRITE_REGISTER:
        break;

    case PROC_READ_DATA:
        num_read_chars = buflen;
        ret = fts_read(proc->cmd, proc->cmd_len, readbuf, num_read_chars);
        if (ret < 0) {
            FTS_ERROR("PROC_READ_DATA read error");
            goto proc_read_err;
        }
        break;

    case PROC_WRITE_DATA:
        break;

    default:
        break;
    }

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(0);
#endif

    if (copy_to_user(buff, readbuf, num_read_chars)) {
        FTS_ERROR("copy to user error");
        ret = -EFAULT;
        goto proc_read_err;
    }

    ret = num_read_chars;
proc_read_err:
    if ((buflen > PROC_BUF_SIZE) && readbuf) {
        kfree(readbuf);
        readbuf = NULL;
    }
    return ret;
}

static const struct file_operations fts_proc_fops = {
    .owner  = THIS_MODULE,
    .read   = fts_debug_read,
    .write  = fts_debug_write,
};
#else
static int fts_debug_write(
    struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    u8 *writebuf = NULL;
    u8 tmpbuf[PROC_BUF_SIZE] = { 0 };
    int buflen = count;
    int writelen = 0;
    int ret = 0;
    char tmp[PROC_BUF_SIZE];
    struct fts_ts_data *ts_data = fts_data;
    struct ftxxxx_proc *proc = &ts_data->proc;

    if ((buflen <= 1) || (buflen > PAGE_SIZE)) {
        FTS_ERROR("apk proc wirte count(%d>%d) fail", buflen, (int)PAGE_SIZE);
        return -EINVAL;
    }

    if (buflen > PROC_BUF_SIZE) {
        writebuf = (u8 *)kzalloc(buflen * sizeof(u8), GFP_KERNEL);
        if (NULL == writebuf) {
            FTS_ERROR("apk proc wirte buf zalloc fail");
            return -ENOMEM;
        }
    } else {
        writebuf = tmpbuf;
    }

    if (copy_from_user(writebuf, buff, buflen)) {
        FTS_ERROR("[APK]: copy from user error!!");
        ret = -EFAULT;
        goto proc_write_err;
    }

    proc->opmode = writebuf[0];
    switch (proc->opmode) {
    case PROC_SET_TEST_FLAG:
        FTS_DEBUG("[APK]: PROC_SET_TEST_FLAG = %x", writebuf[1]);
        if (writebuf[1] == 0) {
#if FTS_ESDCHECK_EN
            fts_esdcheck_switch(ENABLE);
#endif
        } else {
#if FTS_ESDCHECK_EN
            fts_esdcheck_switch(DISABLE);
#endif
        }
        break;

    case PROC_READ_REGISTER:
        proc->cmd[0] = writebuf[1];
        break;

    case PROC_WRITE_REGISTER:
        ret = fts_write_reg(writebuf[1], writebuf[2]);
        if (ret < 0) {
            FTS_ERROR("PROC_WRITE_REGISTER write error");
            goto proc_write_err;
        }
        break;

    case PROC_READ_DATA:
        writelen = buflen - 1;
        memcpy(proc->cmd, writebuf + 1, writelen);
        proc->cmd_len = writelen;
        break;

    case PROC_WRITE_DATA:
        writelen = buflen - 1;
        ret = fts_write(writebuf + 1, writelen);
        if (ret < 0) {
            FTS_ERROR("PROC_WRITE_DATA write error");
            goto proc_write_err;
        }
        break;

    case PROC_SET_SLAVE_ADDR:
        break;

    case PROC_HW_RESET:
        snprintf(tmp, PROC_BUF_SIZE, "%s", writebuf + 1);
        tmp[buflen - 1] = '\0';
        if (strncmp(tmp, "focal_driver", 12) == 0) {
            FTS_INFO("APK execute HW Reset");
            fts_reset_proc(0);
        }
        break;

    case PROC_SET_BOOT_MODE:
        FTS_DEBUG("[APK]: PROC_SET_BOOT_MODE = %x", writebuf[1]);
        if (0 == writebuf[1]) {
            ts_data->fw_is_running = true;
        } else {
            ts_data->fw_is_running = false;
        }
        break;
    case PROC_ENTER_TEST_ENVIRONMENT:
        FTS_DEBUG("[APK]: PROC_ENTER_TEST_ENVIRONMENT = %x", writebuf[1]);
        if (0 == writebuf[1]) {
            fts_enter_test_environment(0);
        } else {
            fts_enter_test_environment(1);
        }
        break;

    default:
        break;
    }

    ret = buflen;
proc_write_err:
    if ((buflen > PROC_BUF_SIZE) && writebuf) {
        kfree(writebuf);
        writebuf = NULL;
    }
    return ret;
}

static int fts_debug_read(
    char *page, char **start, off_t off, int count, int *eof, void *data )
{
    int ret = 0;
    int num_read_chars = 0;
    int buflen = count;
    u8 *readbuf = NULL;
    u8 tmpbuf[PROC_BUF_SIZE] = { 0 };
    struct fts_ts_data *ts_data = fts_data;
    struct ftxxxx_proc *proc = &ts_data->proc;

    if ((buflen <= 0) || (buflen > PAGE_SIZE)) {
        FTS_ERROR("apk proc read count(%d>%d) fail", buflen, (int)PAGE_SIZE);
        return -EINVAL;
    }

    if (buflen > PROC_BUF_SIZE) {
        readbuf = (u8 *)kzalloc(buflen * sizeof(u8), GFP_KERNEL);
        if (NULL == readbuf) {
            FTS_ERROR("apk proc wirte buf zalloc fail");
            return -ENOMEM;
        }
    } else {
        readbuf = tmpbuf;
    }

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(1);
#endif

    switch (proc->opmode) {
    case PROC_READ_REGISTER:
        num_read_chars = 1;
        ret = fts_read_reg(proc->cmd[0], &readbuf[0]);
        if (ret < 0) {
            FTS_ERROR("PROC_READ_REGISTER read error");
            goto proc_read_err;
        }
        break;
    case PROC_WRITE_REGISTER:
        break;

    case PROC_READ_DATA:
        num_read_chars = buflen;
        ret = fts_read(proc->cmd, proc->cmd_len, readbuf, num_read_chars);
        if (ret < 0) {
            FTS_ERROR("PROC_READ_DATA read error");
            goto proc_read_err;
        }
        break;

    case PROC_WRITE_DATA:
        break;

    default:
        break;
    }

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(0);
#endif

    if (copy_to_user(buff, readbuf, num_read_chars)) {
        FTS_ERROR("copy to user error");
        ret = -EFAULT;
        goto proc_read_err;
    }

    ret = num_read_chars;
proc_read_err:
    if ((buflen > PROC_BUF_SIZE) && readbuf) {
        kfree(readbuf);
        readbuf = NULL;
    }
    return ret;
}
#endif

int fts_create_apk_debug_channel(struct fts_ts_data *ts_data)
{
    struct ftxxxx_proc *proc = &ts_data->proc;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
    proc->proc_entry = proc_create(PROC_NAME, 0777, NULL, &fts_proc_fops);
    if (NULL == proc->proc_entry) {
        FTS_ERROR("create proc entry fail");
        return -ENOMEM;
    }
#else
    proc->proc_entry = create_proc_entry(PROC_NAME, 0777, NULL);
    if (NULL == proc->proc_entry) {
        FTS_ERROR("create proc entry fail");
        return -ENOMEM;
    }
    proc->proc_entry->write_proc = fts_debug_write;
    proc->proc_entry->read_proc = fts_debug_read;
#endif

    FTS_INFO("Create proc entry success!");
    return 0;
}

void fts_release_apk_debug_channel(struct fts_ts_data *ts_data)
{
    struct ftxxxx_proc *proc = &ts_data->proc;

    if (proc->proc_entry) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
        proc_remove(proc->proc_entry);
#else
        remove_proc_entry(PROC_NAME, NULL);
#endif
    }
}

/************************************************************************
 * sysfs interface
 ***********************************************************************/
/* fts_hw_reset interface */
static ssize_t fts_hw_reset_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    struct input_dev *input_dev = fts_data->input_dev;
    ssize_t count = 0;

    mutex_lock(&input_dev->mutex);
    fts_reset_proc(0);
    count = snprintf(buf, PAGE_SIZE, "hw reset executed\n");
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_hw_reset_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

/* fts_irq interface */
static ssize_t fts_irq_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t count = 0;
    struct irq_desc *desc = irq_to_desc(fts_data->irq);

    count = snprintf(buf, PAGE_SIZE, "irq_depth:%d\n", desc->depth);

    return count;
}

static ssize_t fts_irq_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct input_dev *input_dev = fts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    if (FTS_SYSFS_ECHO_ON(buf)) {
        FTS_INFO("enable irq");
        fts_irq_enable();
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        FTS_INFO("disable irq");
        fts_irq_disable();
    }
    mutex_unlock(&input_dev->mutex);
    return count;
}

/* fts_boot_mode interface */
static ssize_t fts_bootmode_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct input_dev *input_dev = fts_data->input_dev;

    FTS_FUNC_ENTER();
    mutex_lock(&input_dev->mutex);
    if (FTS_SYSFS_ECHO_ON(buf)) {
        FTS_INFO("[EX-FUN]set to boot mode");
        fts_data->fw_is_running = false;
    } else if (FTS_SYSFS_ECHO_OFF(buf)) {
        FTS_INFO("[EX-FUN]set to fw mode");
        fts_data->fw_is_running = true;
    }
    mutex_unlock(&input_dev->mutex);
    FTS_FUNC_EXIT();

    return count;
}

static ssize_t fts_bootmode_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t count = 0;
    struct input_dev *input_dev = fts_data->input_dev;

    FTS_FUNC_ENTER();
    mutex_lock(&input_dev->mutex);
    if (true == fts_data->fw_is_running) {
        count = snprintf(buf, PAGE_SIZE, "tp is in fw mode\n");
    } else {
        count = snprintf(buf, PAGE_SIZE, "tp is in boot mode\n");
    }
    mutex_unlock(&input_dev->mutex);
    FTS_FUNC_EXIT();

    return count;
}

/* fts_tpfwver interface */
static ssize_t fts_tpfwver_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    struct fts_ts_data *ts_data = fts_data;
    struct input_dev *input_dev = ts_data->input_dev;
    ssize_t num_read_chars = 0;
    u8 fwver = 0;

    mutex_lock(&input_dev->mutex);

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(1);
#endif
    fts_read_reg(FTS_REG_FW_VER, &fwver);
#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(0);
#endif
    if ((fwver == 0xFF) || (fwver == 0x00))
        num_read_chars = snprintf(buf, PAGE_SIZE, "get tp fw version fail!\n");
    else
        num_read_chars = snprintf(buf, PAGE_SIZE, "%02x\n", fwver);

    mutex_unlock(&input_dev->mutex);
    return num_read_chars;
}

static ssize_t fts_tpfwver_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

/* fts_rw_reg */
static ssize_t fts_tprwreg_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count;
    int i;
    struct input_dev *input_dev = fts_data->input_dev;

    mutex_lock(&input_dev->mutex);

    if (rw_op.len < 0) {
        count = snprintf(buf, PAGE_SIZE, "Invalid cmd line\n");
    } else if (rw_op.len == 1) {
        if (RWREG_OP_READ == rw_op.type) {
            if (rw_op.res == 0) {
                count = snprintf(buf, PAGE_SIZE, "Read %02X: %02X\n", rw_op.reg, rw_op.val);
            } else {
                count = snprintf(buf, PAGE_SIZE, "Read %02X failed, ret: %d\n", rw_op.reg,  rw_op.res);
            }
        } else {
            if (rw_op.res == 0) {
                count = snprintf(buf, PAGE_SIZE, "Write %02X, %02X success\n", rw_op.reg,  rw_op.val);
            } else {
                count = snprintf(buf, PAGE_SIZE, "Write %02X failed, ret: %d\n", rw_op.reg,  rw_op.res);
            }
        }
    } else {
        if (RWREG_OP_READ == rw_op.type) {
            count = snprintf(buf, PAGE_SIZE, "Read Reg: [%02X]-[%02X]\n", rw_op.reg, rw_op.reg + rw_op.len);
            count += snprintf(buf + count, PAGE_SIZE, "Result: ");
            if (rw_op.res) {
                count += snprintf(buf + count, PAGE_SIZE, "failed, ret: %d\n", rw_op.res);
            } else {
                if (rw_op.opbuf) {
                    for (i = 0; i < rw_op.len; i++) {
                        count += snprintf(buf + count, PAGE_SIZE, "%02X ", rw_op.opbuf[i]);
                    }
                    count += snprintf(buf + count, PAGE_SIZE, "\n");
                }
            }
        } else {
            ;
            count = snprintf(buf, PAGE_SIZE, "Write Reg: [%02X]-[%02X]\n", rw_op.reg, rw_op.reg + rw_op.len - 1);
            count += snprintf(buf + count, PAGE_SIZE, "Write Data: ");
            if (rw_op.opbuf) {
                for (i = 1; i < rw_op.len; i++) {
                    count += snprintf(buf + count, PAGE_SIZE, "%02X ", rw_op.opbuf[i]);
                }
                count += snprintf(buf + count, PAGE_SIZE, "\n");
            }
            if (rw_op.res) {
                count += snprintf(buf + count, PAGE_SIZE, "Result: failed, ret: %d\n", rw_op.res);
            } else {
                count += snprintf(buf + count, PAGE_SIZE, "Result: success\n");
            }
        }
        /*if (rw_op.opbuf) {
            kfree(rw_op.opbuf);
            rw_op.opbuf = NULL;
        }*/
    }
    mutex_unlock(&input_dev->mutex);

    return count;
}

static int shex_to_int(const char *hex_buf, int size)
{
    int i;
    int base = 1;
    int value = 0;
    char single;

    for (i = size - 1; i >= 0; i--) {
        single = hex_buf[i];

        if ((single >= '0') && (single <= '9')) {
            value += (single - '0') * base;
        } else if ((single >= 'a') && (single <= 'z')) {
            value += (single - 'a' + 10) * base;
        } else if ((single >= 'A') && (single <= 'Z')) {
            value += (single - 'A' + 10) * base;
        } else {
            return -EINVAL;
        }

        base *= 16;
    }

    return value;
}


static u8 shex_to_u8(const char *hex_buf, int size)
{
    return (u8)shex_to_int(hex_buf, size);
}
/*
 * Format buf:
 * [0]: '0' write, '1' read(reserved)
 * [1-2]: addr, hex
 * [3-4]: length, hex
 * [5-6]...[n-(n+1)]: data, hex
 */
static int fts_parse_buf(const char *buf, size_t cmd_len)
{
    int length;
    int i;
    char *tmpbuf;

    rw_op.reg = shex_to_u8(buf + 1, 2);
    length = shex_to_int(buf + 3, 2);

    if (buf[0] == '1') {
        rw_op.len = length;
        rw_op.type = RWREG_OP_READ;
        FTS_DEBUG("read %02X, %d bytes", rw_op.reg, rw_op.len);
    } else {
        if (cmd_len < (length * 2 + 5)) {
            pr_err("data invalided!\n");
            return -EINVAL;
        }
        FTS_DEBUG("write %02X, %d bytes", rw_op.reg, length);

        /* first byte is the register addr */
        rw_op.type = RWREG_OP_WRITE;
        rw_op.len = length + 1;
    }

    if (rw_op.len > 0) {
        tmpbuf = (char *)kzalloc(rw_op.len, GFP_KERNEL);
        if (!tmpbuf) {
            FTS_ERROR("allocate memory failed!\n");
            return -ENOMEM;
        }

        if (RWREG_OP_WRITE == rw_op.type) {
            tmpbuf[0] = rw_op.reg & 0xFF;
            FTS_DEBUG("write buffer: ");
            for (i = 1; i < rw_op.len; i++) {
                tmpbuf[i] = shex_to_u8(buf + 5 + i * 2 - 2, 2);
                FTS_DEBUG("buf[%d]: %02X", i, tmpbuf[i] & 0xFF);
            }
        }
        rw_op.opbuf = tmpbuf;
    }

    return rw_op.len;
}

static ssize_t fts_tprwreg_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct input_dev *input_dev = fts_data->input_dev;
    ssize_t cmd_length = 0;

    mutex_lock(&input_dev->mutex);
    cmd_length = count - 1;

    if (rw_op.opbuf) {
        kfree(rw_op.opbuf);
        rw_op.opbuf = NULL;
    }

    FTS_DEBUG("cmd len: %d, buf: %s", (int)cmd_length, buf);
    /* compatible old ops */
    if (2 == cmd_length) {
        rw_op.type = RWREG_OP_READ;
        rw_op.len = 1;
        rw_op.reg = shex_to_int(buf, 2);
    } else if (4 == cmd_length) {
        rw_op.type = RWREG_OP_WRITE;
        rw_op.len = 1;
        rw_op.reg = shex_to_int(buf, 2);
        rw_op.val = shex_to_int(buf + 2, 2);
    } else if (cmd_length < 5) {
        FTS_ERROR("Invalid cmd buffer");
        mutex_unlock(&input_dev->mutex);
        return -EINVAL;
    } else {
        rw_op.len = fts_parse_buf(buf, cmd_length);
    }

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(1);
#endif
    if (rw_op.len < 0) {
        FTS_ERROR("cmd buffer error!");

    } else {
        if (RWREG_OP_READ == rw_op.type) {
            if (rw_op.len == 1) {
                u8 reg, val;
                reg = rw_op.reg & 0xFF;
                rw_op.res = fts_read_reg(reg, &val);
                rw_op.val = val;
            } else {
                char reg;
                reg = rw_op.reg & 0xFF;

                rw_op.res = fts_read(&reg, 1, rw_op.opbuf, rw_op.len);
            }

            if (rw_op.res < 0) {
                FTS_ERROR("Could not read 0x%02x", rw_op.reg);
            } else {
                FTS_INFO("read 0x%02x, %d bytes successful", rw_op.reg, rw_op.len);
                rw_op.res = 0;
            }

        } else {
            if (rw_op.len == 1) {
                u8 reg, val;
                reg = rw_op.reg & 0xFF;
                val = rw_op.val & 0xFF;
                rw_op.res = fts_write_reg(reg, val);
            } else {
                rw_op.res = fts_write(rw_op.opbuf, rw_op.len);
            }
            if (rw_op.res < 0) {
                FTS_ERROR("Could not write 0x%02x", rw_op.reg);

            } else {
                FTS_INFO("Write 0x%02x, %d bytes successful", rw_op.val, rw_op.len);
                rw_op.res = 0;
            }
        }
    }

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(0);
#endif
    mutex_unlock(&input_dev->mutex);

    return count;
}

/* fts_upgrade_bin interface */
static ssize_t fts_fwupgradebin_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    return -EPERM;
}

static ssize_t fts_fwupgradebin_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    char fwname[FILE_NAME_LENGTH] = { 0 };
    struct input_dev *input_dev = fts_data->input_dev;

    if ((count <= 1) || (count >= FILE_NAME_LENGTH - 32)) {
        FTS_ERROR("fw bin name's length(%d) fail", (int)count);
        return -EINVAL;
    }
    memset(fwname, 0, sizeof(fwname));
    snprintf(fwname, FILE_NAME_LENGTH, "%s", buf);
    fwname[count - 1] = '\0';

    strlcpy(fts_data->fts_firmware_name, fwname, sizeof(fts_data->fts_firmware_name));
    FTS_INFO("upgrade with bin file through sysfs node");
    mutex_lock(&input_dev->mutex);
    fts_upgrade_bin(fwname, 0);
    mutex_unlock(&input_dev->mutex);

    return count;
}

/* fts_force_upgrade interface */
static ssize_t fts_fwforceupg_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    return -EPERM;
}

static ssize_t fts_fwforceupg_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    char fwname[FILE_NAME_LENGTH];
    struct input_dev *input_dev = fts_data->input_dev;

    if ((count <= 1) || (count >= FILE_NAME_LENGTH - 32)) {
        FTS_ERROR("fw bin name's length(%d) fail", (int)count);
        return -EINVAL;
    }
    memset(fwname, 0, sizeof(fwname));
    snprintf(fwname, FILE_NAME_LENGTH, "%s", buf);
    fwname[count - 1] = '\0';

    FTS_INFO("force upgrade through sysfs node");
    mutex_lock(&input_dev->mutex);
    fts_upgrade_bin(fwname, 1);
    mutex_unlock(&input_dev->mutex);

    return count;
}

/* fts_driver_info interface */
static ssize_t fts_driverinfo_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    struct fts_ts_data *ts_data = fts_data;
    struct fts_ts_platform_data *pdata = ts_data->pdata;
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    count += snprintf(buf + count, PAGE_SIZE, "Driver Ver:%s\n", FTS_DRIVER_VERSION);

    count += snprintf(buf + count, PAGE_SIZE, "Resolution:(%d,%d)~(%d,%d)\n",
                      pdata->x_min, pdata->y_min, pdata->x_max, pdata->y_max);

    count += snprintf(buf + count, PAGE_SIZE, "Max Touchs:%d\n", pdata->max_touch_number);

    count += snprintf(buf + count, PAGE_SIZE, "reset gpio:%d,int gpio:%d,irq:%d\n",
                      pdata->reset_gpio, pdata->irq_gpio, ts_data->irq);

    count += snprintf(buf + count, PAGE_SIZE, "IC ID:0x%02x%02x\n",
                      ts_data->ic_info.ids.chip_idh, ts_data->ic_info.ids.chip_idl);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_driverinfo_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

/* fts_product_info interface */
#define PRODUCT_INFO "FT8006S"
static ssize_t fts_productinfo_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    struct fts_ts_data *ts_data = fts_data;
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    count += snprintf(buf + count, PAGE_SIZE, "%s\n", PRODUCT_INFO);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_productinfo_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

static ssize_t fts_poweron_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct fts_ts_data *data = fts_data;
    struct input_dev *input_dev = data->input_dev;
    bool val;

    mutex_lock(&input_dev->mutex);
    val = data->power_disabled;
    mutex_unlock(&input_dev->mutex);

    return scnprintf(buf, PAGE_SIZE, "%d\n", val == false);
}

static ssize_t fts_flashprog_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    struct fts_ts_data *ts_data = fts_data;
    struct input_dev *input_dev = ts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    if (fts_data->fw_loading) {
        FTS_INFO("fw is loading, not download again");
        count = snprintf(buf + count, PAGE_SIZE, "fw is loading\n");
    } else {
        count = snprintf(buf + count, PAGE_SIZE, "fw is not loading\n");
    }
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_flashprog_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

static ssize_t fts_buildid_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    struct fts_ts_data *ts_data = fts_data;
    struct input_dev *input_dev = ts_data->input_dev;
    ssize_t num_read_chars = 0;
    u8 fwver = 0;
    u8 fwver1 = 0;
    u8 fwver2 = 0;

    mutex_lock(&input_dev->mutex);

#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(1);
#endif
    fts_read_reg(FTS_REG_FW_VER, &fwver);
    fts_read_reg(FTS_REG_FW_VER1, &fwver1);
    fts_read_reg(FTS_REG_FW_VER2, &fwver2);
#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(0);
#endif
    if ((fwver == 0xFF) || (fwver == 0x00))
        num_read_chars = snprintf(buf, PAGE_SIZE, "get tp fw version fail!\n");
    else
        num_read_chars = snprintf(buf, PAGE_SIZE, "%02x%02x-%02x\n", fwver1,fwver2,fwver);

    mutex_unlock(&input_dev->mutex);
    return num_read_chars;
}

static ssize_t fts_buildid_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

static ssize_t fts_force_reflash_store(struct device *dev,struct device_attribute *arrt,const char *buf,size_t count)
{
    unsigned int input;

    if(kstrtouint(buf,10,&input)!=1) return -EINVAL;

    fts_data->force_reflash = (input == 0)?false:true;

    return count;
}

/* fts_dump_reg interface */
static ssize_t fts_dumpreg_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    u8 val = 0;
    struct input_dev *input_dev = fts_data->input_dev;

    mutex_lock(&input_dev->mutex);
#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(1);
#endif
    fts_read_reg(FTS_REG_POWER_MODE, &val);
    count += snprintf(buf + count, PAGE_SIZE, "Power Mode:0x%02x\n", val);

    fts_read_reg(FTS_REG_FW_VER, &val);
    count += snprintf(buf + count, PAGE_SIZE, "FW Ver:0x%02x\n", val);

    fts_read_reg(FTS_REG_LIC_VER, &val);
    count += snprintf(buf + count, PAGE_SIZE, "LCD Initcode Ver:0x%02x\n", val);

    fts_read_reg(FTS_REG_IDE_PARA_VER_ID, &val);
    count += snprintf(buf + count, PAGE_SIZE, "Param Ver:0x%02x\n", val);

    fts_read_reg(FTS_REG_IDE_PARA_STATUS, &val);
    count += snprintf(buf + count, PAGE_SIZE, "Param status:0x%02x\n", val);

    fts_read_reg(FTS_REG_VENDOR_ID, &val);
    count += snprintf(buf + count, PAGE_SIZE, "Vendor ID:0x%02x\n", val);

    fts_read_reg(FTS_REG_LCD_BUSY_NUM, &val);
    count += snprintf(buf + count, PAGE_SIZE, "LCD Busy Number:0x%02x\n", val);

    fts_read_reg(FTS_REG_GESTURE_EN, &val);
    count += snprintf(buf + count, PAGE_SIZE, "Gesture Mode:0x%02x\n", val);

    fts_read_reg(FTS_REG_CHARGER_MODE_EN, &val);
    count += snprintf(buf + count, PAGE_SIZE, "charge stat:0x%02x\n", val);

    fts_read_reg(FTS_REG_INT_CNT, &val);
    count += snprintf(buf + count, PAGE_SIZE, "INT count:0x%02x\n", val);

    fts_read_reg(FTS_REG_FLOW_WORK_CNT, &val);
    count += snprintf(buf + count, PAGE_SIZE, "ESD count:0x%02x\n", val);
#if FTS_ESDCHECK_EN
    fts_esdcheck_proc_busy(0);
#endif

    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_dumpreg_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

/* fts_dump_reg interface */
static ssize_t fts_tpbuf_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    int i = 0;
    struct input_dev *input_dev = fts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    count += snprintf(buf + count, PAGE_SIZE, "touch point buffer:\n");
    for (i = 0; i < fts_data->pnt_buf_size; i++) {
        count += snprintf(buf + count, PAGE_SIZE, "%02x ", fts_data->point_buf[i]);
    }
    count += snprintf(buf + count, PAGE_SIZE, "\n");
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_tpbuf_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

/* fts_log_level interface */
static ssize_t fts_log_level_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    struct input_dev *input_dev = fts_data->input_dev;

    mutex_lock(&input_dev->mutex);
    count += snprintf(buf + count, PAGE_SIZE, "log level:%d\n",
                      fts_data->log_level);
    mutex_unlock(&input_dev->mutex);

    return count;
}

static ssize_t fts_log_level_store(
    struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int value = 0;
    struct input_dev *input_dev = fts_data->input_dev;

    FTS_FUNC_ENTER();
    mutex_lock(&input_dev->mutex);
    sscanf(buf, "%d", &value);
    FTS_DEBUG("log level:%d->%d", fts_data->log_level, value);
    fts_data->log_level = value;
    mutex_unlock(&input_dev->mutex);
    FTS_FUNC_EXIT();

    return count;
}

/* get the fw version  example:cat fw_version */
static DEVICE_ATTR(fts_fw_version, S_IRUGO | S_IWUSR, fts_tpfwver_show, fts_tpfwver_store);

/* read and write register(s)
*   All data type is **HEX**
*   Single Byte:
*       read:   echo 88 > rw_reg ---read register 0x88
*       write:  echo 8807 > rw_reg ---write 0x07 into register 0x88
*   Multi-bytes:
*       [0:rw-flag][1-2: reg addr, hex][3-4: length, hex][5-6...n-n+1: write data, hex]
*       rw-flag: 0, write; 1, read
*       read:  echo 10005           > rw_reg ---read reg 0x00-0x05
*       write: echo 000050102030405 > rw_reg ---write reg 0x00-0x05 as 01,02,03,04,05
*  Get result:
*       cat rw_reg
*/
static DEVICE_ATTR(fts_rw_reg, S_IRUGO | S_IWUSR, fts_tprwreg_show, fts_tprwreg_store);
/*  upgrade from fw bin file   example:echo "*.bin" > fts_upgrade_bin */
static DEVICE_ATTR(fts_upgrade_bin, S_IRUGO | S_IWUSR, fts_fwupgradebin_show, fts_fwupgradebin_store);
static DEVICE_ATTR(fts_force_upgrade, S_IRUGO | S_IWUSR, fts_fwforceupg_show, fts_fwforceupg_store);
static DEVICE_ATTR(fts_driver_info, S_IRUGO | S_IWUSR, fts_driverinfo_show, fts_driverinfo_store);
static DEVICE_ATTR(fts_dump_reg, S_IRUGO | S_IWUSR, fts_dumpreg_show, fts_dumpreg_store);
static DEVICE_ATTR(fts_hw_reset, S_IRUGO | S_IWUSR, fts_hw_reset_show, fts_hw_reset_store);
static DEVICE_ATTR(fts_irq, S_IRUGO | S_IWUSR, fts_irq_show, fts_irq_store);
static DEVICE_ATTR(fts_boot_mode, S_IRUGO | S_IWUSR, fts_bootmode_show, fts_bootmode_store);
static DEVICE_ATTR(fts_touch_point, S_IRUGO | S_IWUSR, fts_tpbuf_show, fts_tpbuf_store);
static DEVICE_ATTR(fts_log_level, S_IRUGO | S_IWUSR, fts_log_level_show, fts_log_level_store);
static DEVICE_ATTR(buildid, S_IRUGO | S_IWUSR, fts_buildid_show, fts_buildid_store);
static DEVICE_ATTR(flashprog, S_IRUGO | S_IWUSR, fts_flashprog_show, fts_flashprog_store);
static DEVICE_ATTR(doreflash, S_IRUGO | S_IWUSR, fts_fwupgradebin_show, fts_fwupgradebin_store);
static DEVICE_ATTR(forcereflash, S_IRUGO | S_IWUSR, fts_fwforceupg_show, fts_force_reflash_store);
static DEVICE_ATTR(poweron, S_IRUGO, fts_poweron_show, NULL);
static DEVICE_ATTR(productinfo, S_IRUGO | S_IWUSR, fts_productinfo_show, fts_productinfo_store);

/* add your attr in here*/
static struct attribute *fts_attributes[] = {
    &dev_attr_fts_fw_version.attr,
    &dev_attr_fts_rw_reg.attr,
    &dev_attr_fts_dump_reg.attr,
    &dev_attr_fts_upgrade_bin.attr,
    &dev_attr_fts_force_upgrade.attr,
    &dev_attr_fts_driver_info.attr,
    &dev_attr_fts_hw_reset.attr,
    &dev_attr_fts_irq.attr,
    &dev_attr_fts_boot_mode.attr,
    &dev_attr_fts_touch_point.attr,
    &dev_attr_fts_log_level.attr,
    &dev_attr_poweron.attr,
    &dev_attr_productinfo.attr,
    &dev_attr_flashprog.attr,
    &dev_attr_doreflash.attr,
    &dev_attr_forcereflash.attr,
    &dev_attr_buildid.attr,
    NULL
};

static struct attribute_group fts_attribute_group = {
    .attrs = fts_attributes
};

static ssize_t path_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
        struct fts_ts_data *data = dev_get_drvdata(dev);
        ssize_t blen;
        const char *path;

        if (!data) {
                pr_err("cannot get ft_data pointer\n");
                return (ssize_t)0;
        }
        path = kobject_get_path(&data->spi->dev.kobj, GFP_KERNEL);
        blen = scnprintf(buf, PAGE_SIZE, "%s", path ? path : "na");
        kfree(path);
        return blen;
}

/* Attribute: vendor (RO) */
static ssize_t vendor_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
        return scnprintf(buf, PAGE_SIZE, "focaltech");
}

#define TSDEV_MINOR_BASE 128
#define TSDEV_MINOR_MAX 32

static struct device_attribute touchscreen_attributes[] = {
        __ATTR_RO(path),
        __ATTR_RO(vendor),
        __ATTR_NULL
};

int fts_create_sysfs(struct fts_ts_data *ts_data)
{
    int ret = 0;
    static int minor;
    static struct class *touchscreen_class;
    static struct device *ts_class_dev;
    struct device_attribute *attrs = touchscreen_attributes;
    int i, error = 0;

    minor = input_get_new_minor(TSDEV_MINOR_BASE,TSDEV_MINOR_MAX, true);
    touchscreen_class = class_create(THIS_MODULE, "touchscreen");
    if (IS_ERR(touchscreen_class)) {
        FTS_ERROR("[touchscreen]: class_create() failed!!");
        touchscreen_class = NULL;
        return -ENOMEM;
    }
    ts_class_dev = device_create(touchscreen_class, NULL,MKDEV(INPUT_MAJOR, minor),fts_data,"ft8006s");
    if (IS_ERR(ts_class_dev)) {
        FTS_ERROR("[touchscreen]: device_create() failed!!");
        ts_class_dev = NULL;
        return -ENOMEM;
    }
    for (i = 0; attrs[i].attr.name != NULL; ++i) {
        error = device_create_file(ts_class_dev, &attrs[i]);
        if (error)
            break;
    }
    if (error)
        goto device_destroy;
    ret = sysfs_create_group(&ts_class_dev->kobj, &fts_attribute_group);
    if (ret) {
        FTS_ERROR("[EX]: sysfs_create_touch_group() failed!!");
        sysfs_remove_group(&ts_class_dev->kobj, &fts_attribute_group);
        return -ENOMEM;
    } else {
        FTS_INFO("[EX]: sysfs_create_touch_group() succeeded!!");
    }

    ret = sysfs_create_group(&ts_data->dev->kobj, &fts_attribute_group);
    if (ret) {
        FTS_ERROR("[EX]: sysfs_create_group() failed!!");
        sysfs_remove_group(&ts_data->dev->kobj, &fts_attribute_group);
        return -ENOMEM;
    } else {
        FTS_INFO("[EX]: sysfs_create_group() succeeded!!");
    }

    return ret;

device_destroy:
    for (--i; i >= 0; --i)
        device_remove_file(ts_class_dev, &attrs[i]);
    device_destroy(touchscreen_class, MKDEV(INPUT_MAJOR, minor));
    ts_class_dev = NULL;
    class_unregister(touchscreen_class);
    FTS_ERROR("[touchscreen]: class_ungister()!!");
    return -ENOMEM;
}

int fts_remove_sysfs(struct fts_ts_data *ts_data)
{
    sysfs_remove_group(&ts_data->dev->kobj, &fts_attribute_group);
    return 0;
}

static int fts_monitor_control_show(struct seq_file *sfile, void *v) {
	seq_printf(sfile, "active or doze mode!\n");
	return 0;
}
static ssize_t fts_monitor_control_store(struct file *file, const char *buffer, size_t count, loff_t *pos) {
	int ret = 0;
	struct input_dev *input_dev = fts_data->input_dev;
	char *tmp = kzalloc((count+1), GFP_KERNEL);

	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp,buffer,1)) {
		kfree(tmp);
		return -EINVAL;
	}

	if(tmp[0] == '0') {
		mutex_lock(&input_dev->mutex);
		ret = fts_write_reg(FTS_REG_ACTIVE_MODE, FTS_REG_ACTIVE_MODE_VALUE);
		if (ret < 0)
			FTS_ERROR("set TP to active mode fail, ret=%d", ret);
		mutex_unlock(&input_dev->mutex);
	} else {
		mutex_lock(&input_dev->mutex);
		ret = fts_write_reg(FTS_REG_ACTIVE_MODE, FTS_REG_DOZE_MODE_VALUE);
		if (ret < 0)
			FTS_ERROR("set TP to doze mode fail, ret=%d", ret);
		mutex_unlock(&input_dev->mutex);
	}

	kfree(tmp);
	return count;

}
static int32_t fts_monitor_control_open(struct inode *inode, struct file *file) {
	return single_open(file, fts_monitor_control_show, NULL);
}
static const struct file_operations monitor_control_fops = {
	.owner = THIS_MODULE,
	.open = fts_monitor_control_open,
	.read = seq_read,
	.write = fts_monitor_control_store,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int fts_sleep_show(struct seq_file *sfile, void *v) {
	seq_printf(sfile, "fts_sleep!\n");
	return 0;
}
static ssize_t fts_sleep_store(struct file *file, const char *buffer, size_t count, loff_t *pos) {
	int ret = 0;
	struct input_dev *input_dev = fts_data->input_dev;
	char *tmp = kzalloc((count+1), GFP_KERNEL);

	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp,buffer,1)) {
		kfree(tmp);
		return -EINVAL;
	}

	if(tmp[0] == '1') {
		mutex_lock(&input_dev->mutex);
		ret = fts_write_reg(FTS_REG_POWER_MODE, FTS_REG_POWER_MODE_SLEEP_VALUE);
		if (ret < 0)
			FTS_ERROR("set TP to sleep mode fail, ret=%d", ret);
		mutex_unlock(&input_dev->mutex);
	}

	kfree(tmp);
	return count;

}
static int32_t fts_sleep_open(struct inode *inode, struct file *file) {
	return single_open(file, fts_sleep_show, NULL);
}
static const struct file_operations sleep_fops = {
	.owner = THIS_MODULE,
	.open = fts_sleep_open,
	.read = seq_read,
	.write = fts_sleep_store,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int fts_reset_show(struct seq_file *sfile, void *v) {
	seq_printf(sfile, "fts_reset!\n");
	return 0;
}
static ssize_t fts_reset_store(struct file *file, const char *buffer, size_t count, loff_t *pos) {
	struct input_dev *input_dev = fts_data->input_dev;
	char *tmp = kzalloc((count+1), GFP_KERNEL);

	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp,buffer,1)) {
		kfree(tmp);
		return -EINVAL;
	}

	if(tmp[0] == '1') {
		mutex_lock(&input_dev->mutex);
		fts_reset_proc(0);
		mutex_unlock(&input_dev->mutex);
	}

	kfree(tmp);
	return count;
}
static int32_t fts_reset_open(struct inode *inode, struct file *file) {
	return single_open(file, fts_reset_show, NULL);
}
static const struct file_operations reset_fops = {
	.owner = THIS_MODULE,
	.open = fts_reset_open,
	.read = seq_read,
	.write = fts_reset_store,
	.llseek = seq_lseek,
	.release = seq_release,
};

int32_t fts_extra_proc_init(void)
{
	FTS_proc_powermax_enter = proc_create(FTS_POWERMAX, 0644, NULL,&monitor_control_fops);
	if (FTS_proc_powermax_enter == NULL) {
		FTS_ERROR("create proc/FTS_monitor_control Failed!");
		return -ENOMEM;
	} else {
		FTS_INFO("create proc/FTS_monitor_control Succeeded!");
	}

	FTS_proc_poweron_enter = proc_create(FTS_POWERON, 0644, NULL,&monitor_control_fops);
	if (FTS_proc_poweron_enter == NULL) {
		FTS_ERROR("create proc/FTS_monitor_control Failed!");
		return -ENOMEM;
	} else {
		FTS_INFO("create proc/FTS_monitor_control Succeeded!");
	}

	FTS_proc_sleep_entry = proc_create(FTS_SLEEP, 0644, NULL,&sleep_fops);
	if (FTS_proc_sleep_entry == NULL) {
		FTS_ERROR("create proc/FTS_sleep Failed!");
		return -ENOMEM;
	} else {
		FTS_INFO("create proc/FTS_sleep Succeeded!");
	}

	FTS_proc_reset_entry = proc_create(FTS_RESET, 0644, NULL,&reset_fops);
	if (FTS_proc_reset_entry == NULL) {
		FTS_ERROR("create proc/FTS_reset Failed!");
		return -ENOMEM;
	} else {
		FTS_INFO("create proc/FTS_reset Succeeded!");
	}
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen extra function proc. file node
	deinitial function.

return:
	n.a.
*******************************************************/
void fts_extra_proc_deinit(void)
{
	if (FTS_proc_powermax_enter != NULL) {
		remove_proc_entry(FTS_POWERMAX, NULL);
		FTS_proc_powermax_enter = NULL;
		FTS_INFO("Removed /proc/%s", FTS_POWERMAX);
	}

	if (FTS_proc_poweron_enter != NULL) {
		remove_proc_entry(FTS_POWERON, NULL);
		FTS_proc_poweron_enter = NULL;
		FTS_INFO("Removed /proc/%s", FTS_POWERON);
	}

	if (FTS_proc_sleep_entry != NULL) {
		remove_proc_entry(FTS_SLEEP, NULL);
		FTS_proc_sleep_entry = NULL;
		FTS_INFO("Removed /proc/%s", FTS_SLEEP);
	}

	if (FTS_proc_reset_entry != NULL) {
		remove_proc_entry(FTS_RESET, NULL);
		FTS_proc_reset_entry = NULL;
		FTS_INFO("Removed /proc/%s", FTS_RESET);
	}

}
