/* SPDX-License-Identifier: GPL-2.0-only */
/* wrapfd.c
 *
 * Wrapfd
 *
 * Copyright (C) 2025 Google, Inc.
 */

#include <linux/anon_inodes.h>
#include <linux/bvec.h>
#include <linux/dma-buf.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/scatterlist.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/uio.h>
#include <linux/wrapfd.h>
#include <uapi/linux/wrapfd.h>

struct wrap_ctx;
struct wrap_content;
static const struct file_operations wrap_fops;

struct wrap_content_operations {
	int (*create_wrap)(struct wrap_content *content, struct wrap_ctx *ctx);
	int (*load)(struct wrap_content *content, struct file *file,
		    loff_t file_offs, loff_t buf_offs, loff_t len);
	int (*mmap_prepare)(struct wrap_content *content,
			    struct vm_area_struct *vma);
	int (*mmap)(struct wrap_content *content, struct vm_area_struct *vma);
	void (*free)(struct wrap_content *content);
	struct wrap_content *(*make_writable)(struct wrap_content *content,
			      bool writable);
	bool (*is_writable)(struct wrap_content *content);
	void (*show_fdinfo)(struct wrap_content *content, struct seq_file *m);
	int (*get_mappable)(struct wrap_content *content, struct device *dev,
			    union wrapfd_mappable *mappable);
	void (*put_mappable)(struct wrap_content *content,
			    union wrapfd_mappable *mappable);
	int (*ioctl)(struct wrap_content *content,
		     unsigned int cmd, unsigned long arg);

};

/* Abstract wrap content to be embedded in a concrete content object. */
struct wrap_content {
	struct wrap_content_operations *ops;
	bool close_on_exec;
};

/* dmabuf content */
struct wrap_content_dmabuf {
	struct wrap_content content;
	struct dma_buf *dmabuf;
	bool writable;
};

static int dmabuf_content_create_wrap(struct wrap_content *content,
				      struct wrap_ctx *ctx)
{
	struct wrap_content_dmabuf *dmabuf_content;
	struct file *file;
	int fd;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);

	fd = get_unused_fd_flags(0);
	if (fd < 0)
		return fd;

	file = anon_inode_getfile_secure("[wrapfd]", &wrap_fops, ctx,
					 dmabuf_content->writable ? O_RDWR : O_RDONLY, NULL);
	if (IS_ERR(file)) {
		put_unused_fd(fd);
		return PTR_ERR(file);
	}

	/*
	 * Anonymous inodes are created with size == 0. To ensure that calls
	 * like fstat() work as expected, copy the size from the buffer we are
	 * wrapping.
	 */
	i_size_write(file_inode(file), dmabuf_content->dmabuf->size);
	fd_install(fd, file);

	return fd;
}

static int dmabuf_content_load(struct wrap_content *content, struct file *file,
			       loff_t file_offs, loff_t buf_offs, loff_t len)
{
	struct wrap_content_dmabuf *dmabuf_content;
	struct iosys_map map;
	struct iov_iter iter;
	struct kiocb kiocb;
	loff_t bytes_read;
	struct kvec iov;
	loff_t end;
	int ret;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);

	if (check_add_overflow(buf_offs, len, &end))
		return -EINVAL;

	if (end > dmabuf_content->dmabuf->size)
		return -EINVAL;

	ret = dma_buf_begin_cpu_access(dmabuf_content->dmabuf,
				       DMA_BIDIRECTIONAL);
	if (ret)
		return ret;

	ret = dma_buf_vmap(dmabuf_content->dmabuf, &map);
	if (ret)
		goto err_end_access;

	if (map.is_iomem) {
		ret = -EINVAL;
		goto err_unmap;
	}

	iov.iov_base = (u8 *)map.vaddr + buf_offs;
	init_sync_kiocb(&kiocb, file);
	kiocb.ki_pos = file_offs;
	kiocb.ki_flags |= IOCB_DIRECT;

	while (len) {
		loff_t count = min_t(loff_t, MAX_RW_COUNT, len);

		iov.iov_len = count;
		iov_iter_kvec(&iter, ITER_DEST, &iov, 1, iov.iov_len);
		bytes_read = 0;
		while (bytes_read < count) {
			ssize_t sz = vfs_iocb_iter_read(file, &kiocb, &iter);

			if (sz <= 0) {
				ret = sz;
				goto err_unmap;
			}
			bytes_read += sz;
		}
		iov.iov_base += count;
		len -= count;
	}
err_unmap:
	dma_buf_vunmap(dmabuf_content->dmabuf, &map);
err_end_access:
	dma_buf_end_cpu_access(dmabuf_content->dmabuf, DMA_BIDIRECTIONAL);

	if (ret < 0)
		return ret;

	if (len)
		return -EINVAL; /* File was too short / early EOF */

	return 0;
}

static struct wrap_content *
dmabuf_content_make_writable(struct wrap_content *content, bool writable)
{
	struct wrap_content_dmabuf *dmabuf_content;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);
	dmabuf_content->writable = writable;

	return content;
}

static bool dmabuf_content_is_writable(struct wrap_content *content)
{
	struct wrap_content_dmabuf *dmabuf_content;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);

	return dmabuf_content->writable;
}

static int dmabuf_content_mmap(struct wrap_content *content,
			       struct vm_area_struct *vma)
{
	struct wrap_content_dmabuf *dmabuf_content;
	int ret;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);

	ret = dma_buf_mmap(dmabuf_content->dmabuf, vma, 0);
	if (ret)
		return ret;

	return 0;
}

static void dmabuf_content_free(struct wrap_content *content)
{
	struct wrap_content_dmabuf *dmabuf_content;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);
	if (dmabuf_content->dmabuf)
		dma_buf_put(dmabuf_content->dmabuf);
	kfree(dmabuf_content);
}

static void dmabuf_content_show_fdinfo(struct wrap_content *content,
				       struct seq_file *m)
{
	struct wrap_content_dmabuf *dmabuf_content;
	struct dma_buf *dmabuf;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);
	seq_printf(m, "type:\tdmabuf\n");

	dmabuf = dmabuf_content->dmabuf;
	seq_printf(m, "size:\t%zu\n", dmabuf->size);
	/* Don't count the temporary reference taken inside procfs seq_show */
	seq_printf(m, "count:\t%ld\n", file_count(dmabuf->file) - 1);
	seq_printf(m, "exp_name:\t%s\n", dmabuf->exp_name);
	spin_lock(&dmabuf->name_lock);
	if (dmabuf->name)
		seq_printf(m, "name:\t%s\n", dmabuf->name);
	spin_unlock(&dmabuf->name_lock);
}

static int
dmabuf_content_get_mappable(struct wrap_content *content, struct device *dev,
			    union wrapfd_mappable *mappable)
{
	struct wrap_content_dmabuf *dmabuf_content;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);
	mappable->dmabuf = dmabuf_content->dmabuf;

	return 0;
}

static int dmabuf_content_ioctl(struct wrap_content *content,
				unsigned int cmd, unsigned long arg)
{
	struct wrap_content_dmabuf *dmabuf_content;
	struct file *file;

	dmabuf_content = container_of(content, struct wrap_content_dmabuf,
				      content);
	file = dmabuf_content->dmabuf->file;

	return file->f_op->unlocked_ioctl(file, cmd, arg);
}

static struct wrap_content_operations dmabuf_content_ops = {
	.create_wrap		= dmabuf_content_create_wrap,
	.load			= dmabuf_content_load,
	.mmap			= dmabuf_content_mmap,
	.make_writable		= dmabuf_content_make_writable,
	.is_writable		= dmabuf_content_is_writable,
	.free			= dmabuf_content_free,
	.show_fdinfo		= dmabuf_content_show_fdinfo,
	.get_mappable		= dmabuf_content_get_mappable,
	.ioctl			= dmabuf_content_ioctl,
};

static struct wrap_content *alloc_dmabuf_content(struct dma_buf *dmabuf,
						 bool writable)
{
	struct wrap_content_dmabuf *dmabuf_content;

	dmabuf_content = kmalloc(sizeof(*dmabuf_content), GFP_KERNEL);
	if (!dmabuf_content)
		return NULL;

	get_dma_buf(dmabuf);
	dmabuf_content->dmabuf = dmabuf;
	dmabuf_content->writable = writable;
	dmabuf_content->content.ops = &dmabuf_content_ops;

	return &dmabuf_content->content;
}

/* Generic wrapfd */
struct wrap_owner {
	struct task_struct *task;
	struct device *dev;
};

struct wrap_ctx_mapping {
	refcount_t refcnt;
	struct wrap_ctx *ctx;
	const struct vm_operations_struct *content_vm_ops;
	struct vm_operations_struct vm_ops;
};

#define OP_BLOCKED_MODIFICATION	BIT(0)
#define OP_BLOCKED_MAPPING	BIT(1)

struct wrap_ctx {
	struct wrap_content *content;
	spinlock_t lock; /* protects all fields below */
	struct wrap_owner owner;
	bool allow_guests;
	unsigned long map_count;
	/*
	 * Mask of blocked operations when lock is not held due to possiblity
	 * of sleep during the ongoing operation.
	 */
	unsigned int block_mask;
};

static struct wrap_ctx *create_wrap_ctx(void)
{
	struct wrap_ctx *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return NULL;

	spin_lock_init(&ctx->lock);

	return ctx;
}

static inline bool is_owner(struct wrap_ctx *ctx)
{
	assert_spin_locked(&ctx->lock);
	return ctx->owner.task || ctx->owner.dev;
}

static inline bool is_owner_task(struct wrap_ctx *ctx,
				 struct task_struct *task)
{
	assert_spin_locked(&ctx->lock);
	return ctx->owner.task && ctx->owner.task->mm == task->mm;
}

static inline bool is_owner_dev(struct wrap_ctx *ctx,
				struct device *dev)
{
	assert_spin_locked(&ctx->lock);
	return ctx->owner.dev == dev;
}

static inline int publish_wrap(struct wrap_ctx *ctx,
			       struct wrap_content *content)
{
	int ret;

	ctx->content = content;
	ret = content->ops->create_wrap(content, ctx);
	/* Set FD_CLOEXEC flag for wrapfd the same as its content */
	if (ret >= 0)
		set_close_on_exec(ret, content->close_on_exec ? 1 : 0);

	return ret;
}

static int can_access(struct wrap_ctx *ctx, struct task_struct *task,
		      bool check_content)
{
	assert_spin_locked(&ctx->lock);
	if (!is_owner_task(ctx, task))
		return -EBUSY;

	if (ctx->map_count > 0)
		return -EINVAL;

	if (check_content && !ctx->content)
		return -ENOENT;

	return 0;
}

static bool can_map(struct wrap_ctx *ctx)
{
	assert_spin_locked(&ctx->lock);
	return (ctx->block_mask & OP_BLOCKED_MAPPING) == 0;
}

static int block_operations(struct wrap_ctx *ctx, unsigned int mask)
{
	int ret;

	assert_spin_locked(&ctx->lock);
	/* Any request should at least block modifications. */
	if (WARN_ON((mask & OP_BLOCKED_MODIFICATION) == 0))
		return -EINVAL;

	ret = can_access(ctx, current, true);
	if (ret)
		return ret;

	/*
	 * The task is the owner, the content can't be modified by other
	 * processes but racing threads of the owner process can still
	 * modify it. Use block_mask bitmask to prevent that.
	 */
	if (ctx->block_mask & mask)
		return -EAGAIN;

	ctx->block_mask |= mask;

	return 0;
}

static void unblock_operations(struct wrap_ctx *ctx)
{
	assert_spin_locked(&ctx->lock);
	if (WARN_ON(!ctx->block_mask))
		return;

	ctx->block_mask = 0;
}

static void wrap_vm_open(struct vm_area_struct *vma)
{
	struct wrap_ctx_mapping *mapping;

	mapping = container_of(vma->vm_ops, struct wrap_ctx_mapping, vm_ops);
	if (mapping->content_vm_ops && mapping->content_vm_ops->open)
		mapping->content_vm_ops->open(vma);

	spin_lock(&mapping->ctx->lock);
	mapping->ctx->map_count++;
	refcount_inc(&mapping->refcnt);
	spin_unlock(&mapping->ctx->lock);
}

static void wrap_vm_close(struct vm_area_struct *vma)
{
	struct wrap_ctx_mapping *mapping;
	struct wrap_ctx *ctx;

	mapping = container_of(vma->vm_ops, struct wrap_ctx_mapping, vm_ops);
	if (mapping->content_vm_ops && mapping->content_vm_ops->close)
		mapping->content_vm_ops->close(vma);

	ctx = mapping->ctx;
	spin_lock(&ctx->lock);
	if (ctx->map_count > 0)
		ctx->map_count--;
	else
		pr_warn("wrapfd map count underflow\n");
	if (refcount_dec_and_test(&mapping->refcnt))
		kfree(mapping);
	spin_unlock(&ctx->lock);
}

static int wrap_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct wrap_ctx *ctx = file->private_data;
	struct wrap_ctx_mapping *mapping;
	struct wrap_content *content;
	bool make_rdonly = false;
	int ret = 0;

	spin_lock(&ctx->lock);
	if (!ctx->allow_guests && is_owner(ctx) &&
	    !is_owner_task(ctx, current)) {
		ret = -EBUSY;
		goto unlock;
	}

	/*
	 * If mappings are blocked the content is being rewrapped or emptied.
	 * Treat this as if the wrap is already empty.
	 */
	if (!can_map(ctx)) {
		ret = -ENOENT;
		goto unlock;
	}

	content = ctx->content;
	if (!content) {
		ret = -ENOENT;
		goto unlock;
	}

	/* Handle read-only content */
	if (content->ops->is_writable &&
	    !content->ops->is_writable(content)) {
		if (vma->vm_flags & VM_WRITE) {
			ret = -EACCES;
			goto unlock;
		}
		make_rdonly = !!(vma->vm_flags & VM_MAYWRITE);
	}

	if (content->ops->mmap_prepare) {
		ret = content->ops->mmap_prepare(content, vma);
		if (ret) {
			ret = -EINVAL;
			goto unlock;
		}
	}
	/*
	 * Increased map_count prevents changes in the
	 * ownership, rewrapping or emptying the content.
	 * Therefore content is stable.
	 */
	ctx->map_count++;
unlock:
	spin_unlock(&ctx->lock);

	if (ret)
		goto err;

	/* If we reached here then ctx->map_count has been incremented */
	mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
	if (!mapping) {
		ret = -ENOMEM;
		goto err_dec;
	}

	ret = content->ops->mmap(content, vma);
	if (ret)
		goto err_free_mapping;

	if (make_rdonly) {
		/*
		 * content->ops->mmap should not be mapping read-only content
		 * as writable. Either content->ops->is_writable reports
		 * incorrect value or content->ops->mmap is misbehaving.
		 */
		if (unlikely(vma->vm_flags & VM_WRITE)) {
			pr_warn("wrapfd read-only content was mapped as writable\n");
			ret = -EACCES;
			goto err_free_mapping;
		}
		vm_flags_clear(vma, VM_MAYWRITE);
	}

	spin_lock(&ctx->lock);
	mapping->content_vm_ops = vma->vm_ops;
	if (vma->vm_ops)
		mapping->vm_ops = *vma->vm_ops;
	mapping->vm_ops.open = wrap_vm_open;
	mapping->vm_ops.close = wrap_vm_close;
	vma->vm_ops = &mapping->vm_ops;
	mapping->ctx = ctx;
	refcount_set(&mapping->refcnt, 1);
	spin_unlock(&ctx->lock);

	return 0;
err_free_mapping:
	kfree(mapping);
err_dec:
	spin_lock(&ctx->lock);
	ctx->map_count--;
	spin_unlock(&ctx->lock);
err:
	return ret;
}

static int wrap_release(struct inode *ignored, struct file *file)
{
	struct wrap_ctx *ctx = file->private_data;

	if (ctx->content)
		ctx->content->ops->free(ctx->content);
	kfree(ctx);

	return 0;
}

static int get_wrap_state(struct wrap_ctx *ctx,
			  struct wrapfd_get_state __user *user_wrapfd_get_state)
{
	struct wrapfd_get_state wrapfd_get_state;

	if (copy_from_user(&wrapfd_get_state, user_wrapfd_get_state,
			   sizeof(wrapfd_get_state)))
		return -EFAULT;

	if (wrapfd_get_state.reserved || wrapfd_get_state.pad)
		return -EINVAL;

	spin_lock(&ctx->lock);
	/*
	 * If mappings are blocked the content is being rewrapped or emptied.
	 * Treat this as if the wrap is already empty.
	 */
	if (ctx->content && can_map(ctx)) {
		if (ctx->content->ops->is_writable(ctx->content))
			wrapfd_get_state.state = WRAPFD_CONTENT_RDWR;
		else
			wrapfd_get_state.state = WRAPFD_CONTENT_RDONLY;
	} else {
		wrapfd_get_state.state = WRAPFD_CONTENT_EMPTY;
	}
	spin_unlock(&ctx->lock);

	if (copy_to_user(user_wrapfd_get_state, &wrapfd_get_state,
			 sizeof(wrapfd_get_state)))
		return -EFAULT;

	return 0;
}

static int wrap_file_acquire_ownership(struct wrap_ctx *ctx)
{
	int ret = 0;

	spin_lock(&ctx->lock);

	if (is_owner_task(ctx, current))
		goto unlock;

	if (is_owner(ctx)) {
		ret = -EBUSY;
		goto unlock;
	}

	if (ctx->map_count > 0) {
		ret = -EINVAL;
		goto unlock;
	}

	if (!ctx->content) {
		ret = -ENOENT;
		goto unlock;
	}

	ctx->owner.task = current;
unlock:
	spin_unlock(&ctx->lock);

	return ret;
}

static int wrap_file_release_ownership(struct wrap_ctx *ctx)
{
	int ret = 0;

	spin_lock(&ctx->lock);

	ret = can_access(ctx, current, false);
	if (ret)
		goto unlock;

	ctx->owner.task = NULL;
	ctx->allow_guests = false;
unlock:
	spin_unlock(&ctx->lock);

	return ret;
}

static int wrap_file_load(struct wrap_ctx *ctx,
			  struct wrapfd_load __user *user_wrapfd_load)
{
	struct wrapfd_load wrapfd_load;
	struct file *file;
	loff_t end;
	int ret = 0;

	if (copy_from_user(&wrapfd_load, user_wrapfd_load,
			   sizeof(wrapfd_load)))
		return -EFAULT;

	if (!PAGE_ALIGNED(wrapfd_load.file_offs))
		return -EINVAL;

	if (!PAGE_ALIGNED(wrapfd_load.buf_offs))
		return -EINVAL;

	if (wrapfd_load.reserved || wrapfd_load.pad)
		return -EINVAL;

	file = fget(wrapfd_load.fd);
	if (!file)
		return -EBADF;

	if (!(file->f_mode & FMODE_READ)) {
		ret = -EBADF;
		goto put_file;
	}

	if (!file->f_op->read_iter) {
		ret = -EINVAL;
		goto put_file;
	}

	if (!(file->f_mode & FMODE_CAN_READ)) {
		ret = -EINVAL;
		goto put_file;
	}

	if (!(file->f_mode & FMODE_CAN_ODIRECT)) {
		ret = -EINVAL;
		goto put_file;
	}

	/* Align the size to the page boundary */
	wrapfd_load.len = PAGE_ALIGN(wrapfd_load.len);

	if (check_add_overflow(wrapfd_load.file_offs, wrapfd_load.len,
			       &end)) {
		ret = -EINVAL;
		goto put_file;
	}

	if (end > i_size_read(file_inode(file))) {
		ret = -EINVAL;
		goto put_file;
	}

	spin_lock(&ctx->lock);
	ret = block_operations(ctx, OP_BLOCKED_MODIFICATION);
	spin_unlock(&ctx->lock);

	if (ret)
		goto put_file;

	ret = ctx->content->ops->load(ctx->content, file,
				      wrapfd_load.file_offs,
				      wrapfd_load.buf_offs,
				      wrapfd_load.len);
	spin_lock(&ctx->lock);
	unblock_operations(ctx);
	spin_unlock(&ctx->lock);
put_file:
	fput(file);

	return ret;
}

static int wrap_file_rewrap(struct wrap_ctx *ctx,
			    struct wrapfd_rewrap __user *user_wrapfd_rewrap)
{
	struct wrapfd_rewrap wrapfd_rewrap;
	struct wrap_content *new_content;
	struct wrap_content *content;
	struct wrap_ctx *new_ctx;
	int ret = 0;

	if (copy_from_user(&wrapfd_rewrap, user_wrapfd_rewrap,
			   sizeof(wrapfd_rewrap)))
		return -EFAULT;

	if (wrapfd_rewrap.prot & ~(PROT_WRITE | PROT_READ))
		return -EINVAL;

	if (wrapfd_rewrap.reserved || wrapfd_rewrap.pad)
		return -EINVAL;

	spin_lock(&ctx->lock);
	ret = block_operations(ctx,
			       OP_BLOCKED_MODIFICATION | OP_BLOCKED_MAPPING);
	if (!ret) {
		content = ctx->content;
		ctx->content = NULL;
	}
	spin_unlock(&ctx->lock);

	if (ret)
		goto out;

	new_content = content->ops->make_writable(content,
				(wrapfd_rewrap.prot & PROT_WRITE) != 0);
	if (!new_content) {
		ret = -ENOMEM;
		goto restore_content;
	}
	new_content->close_on_exec = content->close_on_exec;

	new_ctx = create_wrap_ctx();
	if (!new_ctx) {
		ret = -ENOMEM;
		goto free_new_content;
	}

	ret = publish_wrap(new_ctx, new_content);
	if (ret < 0)
		goto free_new_ctx;

	if (new_content != content)
		content->ops->free(content);

	spin_lock(&ctx->lock);
	unblock_operations(ctx);
	spin_unlock(&ctx->lock);

	return ret;

free_new_ctx:
	kfree(new_ctx);
free_new_content:
	if (new_content != content)
		new_content->ops->free(new_content);
restore_content:
	/*
	 * Restore original wrap. We are the owner and the wrap
	 * is empty, so it could not have changed from under us.
	 */
	spin_lock(&ctx->lock);
	ctx->content = content;
	unblock_operations(ctx);
	spin_unlock(&ctx->lock);
out:
	return ret;
}

static int wrap_file_empty(struct wrap_ctx *ctx)
{
	struct wrap_content *content;
	int ret = 0;

	spin_lock(&ctx->lock);

	ret = block_operations(ctx,
			       OP_BLOCKED_MODIFICATION | OP_BLOCKED_MAPPING);
	if (ret)
		goto unlock;

	content = ctx->content;
	ctx->content = NULL;
	unblock_operations(ctx);
unlock:
	spin_unlock(&ctx->lock);

	if (!ret)
		content->ops->free(content);

	return ret;
}

static int wrap_file_allow_guests(struct wrap_ctx *ctx, bool allow)
{
	int ret = 0;

	spin_lock(&ctx->lock);

	ret = can_access(ctx, current, true);
	if (ret)
		goto unlock;

	ctx->allow_guests = allow;
unlock:
	spin_unlock(&ctx->lock);

	return ret;
}

static int wrap_file_ioctl(struct wrap_ctx *ctx,
			   unsigned int cmd, unsigned long arg)
{
	if (!ctx->content)
		return -ENOENT; /* Wrap is empty */

	if (ctx->content->ops->ioctl)
		return ctx->content->ops->ioctl(ctx->content, cmd, arg);

	return -ENOIOCTLCMD;
}

static long wrap_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct wrap_ctx *ctx = file->private_data;
	long ret;

	switch (cmd) {
	case WRAPFD_DEV_IOC_GET_STATE:
		ret = get_wrap_state(ctx,
				     (struct wrapfd_get_state __user *)arg);
		break;
	case WRAPFD_DEV_IOC_ACQUIRE_OWNERSHIP:
		ret = wrap_file_acquire_ownership(ctx);
		break;
	case WRAPFD_DEV_IOC_RELEASE_OWNERSHIP:
		ret = wrap_file_release_ownership(ctx);
		break;
	case WRAPFD_DEV_IOC_LOAD:
		ret = wrap_file_load(ctx, (struct wrapfd_load __user *)arg);
		break;
	case WRAPFD_DEV_IOC_REWRAP:
		ret = wrap_file_rewrap(ctx,
				       (struct wrapfd_rewrap __user *)arg);
		break;
	case WRAPFD_DEV_IOC_EMPTY:
		ret = wrap_file_empty(ctx);
		break;
	case WRAPFD_DEV_IOC_ALLOW_GUESTS:
		ret = wrap_file_allow_guests(ctx, true);
		break;
	case WRAPFD_DEV_IOC_PROHIBIT_GUESTS:
		ret = wrap_file_allow_guests(ctx, false);
		break;
	default:
		ret = wrap_file_ioctl(ctx, cmd, arg);
		break;
	}

	return ret;
}

#ifdef CONFIG_PROC_FS
static void wrap_show_fdinfo(struct seq_file *m, struct file *file)
{
	struct wrap_ctx *ctx = file->private_data;

	spin_lock(&ctx->lock);
	if (ctx->owner.task) {
		seq_printf(m, "owner:\t%d\n", ctx->owner.task->pid);
	} else {
		if (ctx->owner.dev)
			seq_printf(m, "owner:\t<device>\n");
		else
			seq_printf(m, "owner:\t<none>\n");
	}
	seq_printf(m, "guests:\t%s\n", ctx->allow_guests ? "yes" : "no");
	seq_printf(m, "maps:\t%lu\n", ctx->map_count);
	seq_printf(m, "empty:\t%s\n", ctx->content ? "no" : "yes");
	if (ctx->content) {
		struct wrap_content *content = ctx->content;

		seq_printf(m, "rdonly:\t%s\n",
			   content->ops->is_writable(content) ? "no" : "yes");
		content->ops->show_fdinfo(content, m);
	}
	spin_unlock(&ctx->lock);
}
#endif

bool is_wrapfd_vma(struct vm_area_struct *vma)
{
	return (vma && (vma->vm_ops->open == wrap_vm_open));
}

int wrapfd_get_mappable(struct file *file, struct device *dev,
			union wrapfd_mappable *mappable)
{
	struct wrap_ctx *ctx;
	int ret;

	if (WARN_ON(!dev))
		return -ENODEV;

	if (file->f_op != &wrap_fops)
		return -EBADF;

	ctx = file->private_data;

	spin_lock(&ctx->lock);

	if (is_owner(ctx) && !is_owner_dev(ctx, dev)) {
		ret = -EBUSY;
		goto unlock;
	}

	if (ctx->map_count > 0) {
		ret = -EINVAL;
		goto unlock;
	}

	if (!ctx->content) {
		ret = -ENOENT;
		goto unlock;
	}

	ctx->owner.dev = dev;
	/* Device is the owner, context can't change from under us. */
	ret = 0;
unlock:
	spin_unlock(&ctx->lock);

	if (!ret) {
		ret = ctx->content->ops->get_mappable(ctx->content, dev,
						      mappable);
		if (ret) {
			spin_lock(&ctx->lock);
			ctx->owner.dev = NULL;
			spin_unlock(&ctx->lock);
		}
	}

	return ret;
}

int wrapfd_put_mappable(struct file *file, struct device *dev,
			union wrapfd_mappable *mappable)
{
	struct wrap_ctx *ctx;
	int ret;

	if (WARN_ON(!dev))
		return -ENODEV;

	if (file->f_op != &wrap_fops)
		return -EBADF;

	ctx = file->private_data;

	spin_lock(&ctx->lock);

	if (!is_owner_dev(ctx, dev)) {
		ret = -EBUSY;
		goto unlock;
	}

	ctx->owner.dev = NULL;
	ret = 0;
unlock:
	spin_unlock(&ctx->lock);

	if (!ret && ctx->content->ops->put_mappable)
		ctx->content->ops->put_mappable(ctx->content, mappable);

	return ret;
}

static const struct file_operations wrap_fops = {
	.owner		= THIS_MODULE,
	.mmap		= wrap_mmap,
	.release	= wrap_release,
	.unlocked_ioctl	= wrap_ioctl,
	.compat_ioctl	= wrap_ioctl,
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= wrap_show_fdinfo,
#endif
};

static struct wrap_content *create_content_for(int fd, unsigned long prot)
{
	struct wrap_content *content;
	struct dma_buf *dmabuf;

	dmabuf = dma_buf_get(fd);
	if (!IS_ERR(dmabuf)) {
		bool writable = !!(prot & PROT_WRITE);

		content = alloc_dmabuf_content(dmabuf, writable);
		dma_buf_put(dmabuf);

		return content ? content : ERR_PTR(-ENOMEM);
	}

	return ERR_PTR(-EINVAL);
}

static int wrap_file(struct wrap_ctx *ctx,
		     struct wrapfd_wrap __user *user_wrapfd_wrap)
{
	struct wrapfd_wrap wrapfd_wrap;
	struct wrap_content *content;
	int wrapfd;

	if (copy_from_user(&wrapfd_wrap, user_wrapfd_wrap,
			   sizeof(wrapfd_wrap)))
		return -EFAULT;

	if (wrapfd_wrap.prot & ~(PROT_WRITE | PROT_READ))
		return -EINVAL;

	if (wrapfd_wrap.reserved)
		return -EINVAL;

	content = create_content_for(wrapfd_wrap.fd, wrapfd_wrap.prot);
	if (IS_ERR(content))
		return PTR_ERR(content);

	content->close_on_exec = get_close_on_exec(wrapfd_wrap.fd);
	wrapfd = publish_wrap(ctx, content);
	if (wrapfd < 0) {
		ctx->content = NULL;
		content->ops->free(content);
	}

	return wrapfd;
}

static long wrapfd_dev_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	struct wrap_ctx *ctx;
	int ret;

	VM_WARN_ON_ONCE(!current->mm);

	switch (cmd) {
	case WRAPFD_DEV_IOC_WRAP:
		ctx = create_wrap_ctx();
		if (!ctx)
			return -ENOMEM;

		ret = wrap_file(ctx, (struct wrapfd_wrap __user *)arg);
		if (ret < 0)
			kfree(ctx);

		break;
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

static const struct file_operations wrapfd_dev_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = wrapfd_dev_ioctl,
	.compat_ioctl = wrapfd_dev_ioctl,
	.llseek = noop_llseek,
};

static struct miscdevice wrapfd_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "wrapfd",
	.fops = &wrapfd_dev_fops,
};

static int __init wrapfd_init(void)
{
	int ret;

	ret = misc_register(&wrapfd_misc);
	if (ret) {
		pr_err("failed to register misc device!\n");
		return ret;
	}

	return 0;
}
device_initcall(wrapfd_init);
