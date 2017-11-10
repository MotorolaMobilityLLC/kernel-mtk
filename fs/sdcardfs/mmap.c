/*
 * fs/sdcardfs/mmap.c
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd
 *   Authors: Daeho Jeong, Woojoong Lee, Seunghwan Hyun,
 *               Sunghwan Yun, Sungjong Seo
 *
 * This program has been developed as a stackable file system based on
 * the WrapFS which written by
 *
 * Copyright (c) 1998-2011 Erez Zadok
 * Copyright (c) 2009     Shrikar Archak
 * Copyright (c) 2003-2011 Stony Brook University
 * Copyright (c) 2003-2011 The Research Foundation of SUNY
 *
 * This file is dual licensed.  It may be redistributed and/or modified
 * under the terms of the Apache 2.0 License OR version 2 of the GNU
 * General Public License.
 */

#include "sdcardfs.h"

static int sdcardfs_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	int err;
	struct file *file;
	const struct vm_operations_struct *lower_vm_ops;

	file = (struct file *)vma->vm_private_data;
	lower_vm_ops = SDCARDFS_F(file)->lower_vm_ops;
	BUG_ON(!lower_vm_ops);

	err = lower_vm_ops->fault(vma, vmf);
	return err;
}

static void sdcardfs_vm_open(struct vm_area_struct *vma)
{
	struct file *file = (struct file *)vma->vm_private_data;

	get_file(file);
}

static void sdcardfs_vm_close(struct vm_area_struct *vma)
{
	struct file *file = (struct file *)vma->vm_private_data;

	fput(file);
}

static int sdcardfs_page_mkwrite(struct vm_area_struct *vma,
			       struct vm_fault *vmf)
{
	int err = 0;
	struct file *file;
	const struct vm_operations_struct *lower_vm_ops;

	file = (struct file *)vma->vm_private_data;
	lower_vm_ops = SDCARDFS_F(file)->lower_vm_ops;
	BUG_ON(!lower_vm_ops);
	if (!lower_vm_ops->page_mkwrite)
		goto out;

	err = lower_vm_ops->page_mkwrite(vma, vmf);
out:
	return err;
}

static ssize_t sdcardfs_direct_IO(int rw, struct kiocb *iocb,
		struct iov_iter *iter, loff_t pos)
{
	/*
     * This function returns zero on purpose in order to support direct IO.
	 * __dentry_open checks a_ops->direct_IO and returns EINVAL if it is null.
     *
	 * However, this function won't be called by certain file operations
     * including generic fs functions.  * reads and writes are delivered to
     * the lower file systems and the direct IOs will be handled by them.
	 *
     * NOTE: exceptionally, on the recent kernels (since Linux 3.8.x),
     * swap_writepage invokes this function directly.
	 */
	printk(KERN_INFO "%s, operation is not supported\n", __func__);
	return 0;
}

/*
 * XXX: the default address_space_ops for sdcardfs is empty.  We cannot set
 * our inode->i_mapping->a_ops to NULL because too many code paths expect
 * the a_ops vector to be non-NULL.
 */
const struct address_space_operations sdcardfs_aops = {
	/* empty on purpose */
	.direct_IO	= sdcardfs_direct_IO,
};

const struct vm_operations_struct sdcardfs_vm_ops = {
	.fault		= sdcardfs_fault,
	.page_mkwrite	= sdcardfs_page_mkwrite,
	.open		= sdcardfs_vm_open,
	.close		= sdcardfs_vm_close,
};
