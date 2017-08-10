/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2012-2017 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

 #include "mali_internal_sync.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
#include <linux/ioctl.h>
#include <linux/export.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/anon_inodes.h>

#include "mali_osk.h"
#include "mali_kernel_common.h"
#if defined(DEBUG)
#include "mali_session.h"
#include "mali_timeline.h"
#endif

struct mali_internal_sync_merge_data {
        s32   fd;
        char    name[32];
        s32   fence;
};

struct mali_internal_sync_pt_info {
	u32	len;
	char	obj_name[32];
	char	driver_name[32];
	int	status;
	u64	timestamp_ns;
	u8	driver_data[0];
};

struct mali_internal_sync_info_data {
	u32 len;
	char name[32];
	int status;
	u8 sync_pt_info[0];
};

/**
 * Define the ioctl constant for sync fence wait.
 */
#define MALI_INTERNAL_SYNC_IOC_WAIT           _IOW('>', 0, s32)

/**
 * Define the ioctl constant for sync fence merge.
 */
#define MALI_INTERNAL_SYNC_IOC_MERGE          _IOWR('>', 1, struct mali_internal_sync_merge_data)

/**
 * Define the ioctl constant for sync fence info.
 */
 #define MALI_INTERNAL_SYNC_IOC_FENCE_INFO          _IOWR('>', 2, struct mali_internal_sync_info_data)

static const struct fence_ops fence_ops;
static const struct file_operations sync_fence_fops;

static struct mali_internal_sync_point *mali_internal_fence_to_sync_pt(struct fence *fence)
{
	MALI_DEBUG_ASSERT_POINTER(fence);
	return container_of(fence, struct mali_internal_sync_point, base);
}

static inline struct mali_internal_sync_timeline *mali_internal_sync_pt_to_sync_timeline(struct mali_internal_sync_point *sync_pt)
{
	MALI_DEBUG_ASSERT_POINTER(sync_pt);
	return container_of(sync_pt->base.lock, struct mali_internal_sync_timeline, sync_pt_list_lock);
}

static void mali_internal_sync_timeline_free(struct kref *kref_count)
{
	struct mali_internal_sync_timeline *sync_timeline;

	MALI_DEBUG_ASSERT_POINTER(kref_count);

	sync_timeline = container_of(kref_count, struct mali_internal_sync_timeline, kref_count);

	if (sync_timeline->ops->release_obj)
		sync_timeline->ops->release_obj(sync_timeline);

	kfree(sync_timeline);
}

static struct mali_internal_sync_fence *mali_internal_sync_fence_alloc(int size)
{
	struct mali_internal_sync_fence *sync_fence = NULL;

	sync_fence = kzalloc(size, GFP_KERNEL);
	if (NULL == sync_fence) {
		MALI_PRINT_ERROR(("Mali internal sync: Failed to allocate buffer  for the mali internal sync fence.\n"));
		goto err;
	}

	sync_fence->file = anon_inode_getfile("mali_sync_fence", &sync_fence_fops, sync_fence, 0);
	if (IS_ERR(sync_fence->file)) {
		MALI_PRINT_ERROR(("Mali internal sync: Failed to get file  for the mali internal sync fence: err %d.\n", IS_ERR(sync_fence->file)));
		goto err;
	}

	kref_init(&sync_fence->kref_count);
	init_waitqueue_head(&sync_fence->wq);

	return sync_fence;

err:
	if (NULL != sync_fence) {
		kfree(sync_fence);
	}
	return NULL;
}

static void mali_internal_fence_check_cb_func(struct fence *fence, struct fence_cb *cb)
{
	struct mali_internal_sync_fence_cb *check;
	struct mali_internal_sync_fence *sync_fence;

	MALI_DEBUG_ASSERT_POINTER(cb);
	MALI_IGNORE(fence);

	check = container_of(cb, struct mali_internal_sync_fence_cb, cb);
	sync_fence = check->sync_fence;

	if (atomic_dec_and_test(&sync_fence->status))
		wake_up_all(&sync_fence->wq);
}

static void mali_internal_sync_fence_add_fence(struct mali_internal_sync_fence *sync_fence, struct fence *sync_pt)
{
	int fence_num = 0;
	MALI_DEBUG_ASSERT_POINTER(sync_fence);
	MALI_DEBUG_ASSERT_POINTER(sync_pt);

	fence_num = atomic_read(&sync_fence->num_fences);
	
	sync_fence->cbs[fence_num].base = sync_pt;
	sync_fence->cbs[fence_num].sync_fence = sync_fence;

	if (!fence_add_callback(sync_pt, &sync_fence->cbs[fence_num].cb, mali_internal_fence_check_cb_func)) {
		fence_get(sync_pt);
		atomic_inc(&sync_fence->num_fences);
		atomic_inc(&sync_fence->status);
	}
}

static int mali_internal_sync_fence_wake_up_wq(wait_queue_t *curr, unsigned mode,
				 int wake_flags, void *key)
{
	struct mali_internal_sync_fence_waiter *wait;
	MALI_IGNORE(mode);
	MALI_IGNORE(wake_flags);
	MALI_IGNORE(key);

	wait = container_of(curr, struct mali_internal_sync_fence_waiter, work);
	list_del_init(&wait->work.task_list);

	wait->callback(wait->work.private, wait);
	return 1;
}

struct mali_internal_sync_timeline *mali_internal_sync_timeline_create(const struct mali_internal_sync_timeline_ops *ops,
					   int size, const char *name)
{
	struct mali_internal_sync_timeline *sync_timeline = NULL;

	MALI_DEBUG_ASSERT_POINTER(ops);

	if (size < sizeof(struct mali_internal_sync_timeline)) {
		MALI_PRINT_ERROR(("Mali internal sync:Invalid size to create the mali internal sync timeline.\n"));
		goto err;
	}

	sync_timeline = kzalloc(size, GFP_KERNEL);
	if (NULL == sync_timeline) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to  allocate buffer  for the mali internal sync timeline.\n"));
		goto err;
	}
	kref_init(&sync_timeline->kref_count);
	sync_timeline->ops = ops;
	sync_timeline->fence_context = fence_context_alloc(1);
	strlcpy(sync_timeline->name, name, sizeof(sync_timeline->name));

	INIT_LIST_HEAD(&sync_timeline->sync_pt_list_head);
	spin_lock_init(&sync_timeline->sync_pt_list_lock);

	return sync_timeline;
err:
	if (NULL != sync_timeline) {
		kfree(sync_timeline);
	}
	return NULL;
}

void mali_internal_sync_timeline_destroy(struct mali_internal_sync_timeline *sync_timeline)
{
	MALI_DEBUG_ASSERT_POINTER(sync_timeline);

	sync_timeline->destroyed = MALI_TRUE;

	smp_wmb();

	mali_internal_sync_timeline_signal(sync_timeline);
	kref_put(&sync_timeline->kref_count, mali_internal_sync_timeline_free);
}

void mali_internal_sync_timeline_signal(struct mali_internal_sync_timeline *sync_timeline)
{
	unsigned long flags;
	struct mali_internal_sync_point *sync_pt, *next;

	MALI_DEBUG_ASSERT_POINTER(sync_timeline);

	spin_lock_irqsave(&sync_timeline->sync_pt_list_lock, flags);

	list_for_each_entry_safe(sync_pt, next, &sync_timeline->sync_pt_list_head,
				 sync_pt_list) {
		if (fence_is_signaled_locked(&sync_pt->base))
			list_del_init(&sync_pt->sync_pt_list);
	}

	spin_unlock_irqrestore(&sync_timeline->sync_pt_list_lock, flags);
}

struct mali_internal_sync_point *mali_internal_sync_point_create(struct mali_internal_sync_timeline *sync_timeline, int size)
{
	unsigned long flags;
	struct mali_internal_sync_point *sync_pt = NULL;

	MALI_DEBUG_ASSERT_POINTER(sync_timeline);

	if (size < sizeof(struct mali_internal_sync_point)) {
		MALI_PRINT_ERROR(("Mali internal sync:Invalid size to create the mali internal sync point.\n"));
		goto err;
	}

	sync_pt = kzalloc(size, GFP_KERNEL);
	if (NULL == sync_pt) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to  allocate buffer  for the mali internal sync point.\n"));
		goto err;
	}
	spin_lock_irqsave(&sync_timeline->sync_pt_list_lock, flags);
	kref_get(&sync_timeline->kref_count);
	fence_init(&sync_pt->base, &fence_ops, &sync_timeline->sync_pt_list_lock,
		   sync_timeline->fence_context, ++sync_timeline->value);
	INIT_LIST_HEAD(&sync_pt->sync_pt_list);
	spin_unlock_irqrestore(&sync_timeline->sync_pt_list_lock, flags);

	return sync_pt;
err:
	if (NULL != sync_pt) {
		kfree(sync_pt);
	}
	return NULL;
}

struct mali_internal_sync_fence *mali_internal_sync_fence_create(struct mali_internal_sync_point *sync_pt)
{
	struct mali_internal_sync_fence *sync_fence = NULL;
	
	MALI_DEBUG_ASSERT_POINTER(sync_pt);

	sync_fence = mali_internal_sync_fence_alloc(offsetof(struct mali_internal_sync_fence, cbs[1]));
	if (NULL == sync_fence) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to  create the mali internal sync fence.\n"));
		return NULL;
	}

	atomic_set(&sync_fence->num_fences, 1);
	atomic_set(&sync_fence->status, 1);

	sync_fence->cbs[0].base = &sync_pt->base;
	sync_fence->cbs[0].sync_fence = sync_fence;
	if (fence_add_callback(&sync_pt->base, &sync_fence->cbs[0].cb,
			       mali_internal_fence_check_cb_func))
		atomic_dec(&sync_fence->status);

	return sync_fence;
}

struct mali_internal_sync_fence *mali_internal_sync_fence_fdget(int fd)
{
	struct file *file = fget(fd);

	if (NULL == file) {
		return NULL;
	}
	
	return file->private_data;
}

struct mali_internal_sync_fence *mali_internal_sync_fence_merge(
				    struct mali_internal_sync_fence *sync_fence1, struct mali_internal_sync_fence *sync_fence2)
{
	struct mali_internal_sync_fence *new_sync_fence;
	int i, j, num_fence1, num_fence2, total_fences;
	
	MALI_DEBUG_ASSERT_POINTER(sync_fence1);
	MALI_DEBUG_ASSERT_POINTER(sync_fence2);

	num_fence1 = atomic_read(&sync_fence1->num_fences);
	num_fence2= atomic_read(&sync_fence2->num_fences);

	total_fences = num_fence1 + num_fence2;

	new_sync_fence = mali_internal_sync_fence_alloc(offsetof(struct mali_internal_sync_fence, cbs[total_fences]));
	if (NULL == new_sync_fence) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to  create the mali internal sync fence when merging sync fence.\n"));
		return NULL;
	}

	for (i = j = 0; i < num_fence1 && j < num_fence2; ) {
		struct fence *fence1 = sync_fence1->cbs[i].base;
		struct fence *fence2 = sync_fence2->cbs[j].base;

		if (fence1->context < fence2->context) {
			mali_internal_sync_fence_add_fence(new_sync_fence, fence1);

			i++;
		} else if (fence1->context > fence2->context) {
			mali_internal_sync_fence_add_fence(new_sync_fence, fence2);

			j++;
		} else {
			if (fence1->seqno - fence2->seqno <= INT_MAX)
				mali_internal_sync_fence_add_fence(new_sync_fence, fence1);
			else
				mali_internal_sync_fence_add_fence(new_sync_fence, fence2);
			i++;
			j++;
		}
	}

	for (; i < num_fence1; i++)
		mali_internal_sync_fence_add_fence(new_sync_fence, sync_fence1->cbs[i].base);

	for (; j < num_fence2; j++)
		mali_internal_sync_fence_add_fence(new_sync_fence, sync_fence2->cbs[j].base);

	return new_sync_fence;
}

void mali_internal_sync_fence_waiter_init(struct mali_internal_sync_fence_waiter *waiter,
                                          mali_internal_sync_callback_t callback)
{
	MALI_DEBUG_ASSERT_POINTER(waiter);
	MALI_DEBUG_ASSERT_POINTER(callback);

	INIT_LIST_HEAD(&waiter->work.task_list);
	waiter->callback = callback;
}

int mali_internal_sync_fence_wait_async(struct mali_internal_sync_fence *sync_fence,
			  struct mali_internal_sync_fence_waiter *waiter)
{
	int err;
	unsigned long flags;

	MALI_DEBUG_ASSERT_POINTER(sync_fence);
	MALI_DEBUG_ASSERT_POINTER(waiter);

	err = atomic_read(&sync_fence->status);

	if (0 > err)
		return err;

	if (!err)
		return 1;

	init_waitqueue_func_entry(&waiter->work, mali_internal_sync_fence_wake_up_wq);
	waiter->work.private = sync_fence;

	spin_lock_irqsave(&sync_fence->wq.lock, flags);
	err = atomic_read(&sync_fence->status);
	if (0 < err)
		__add_wait_queue_tail(&sync_fence->wq, &waiter->work);
	spin_unlock_irqrestore(&sync_fence->wq.lock, flags);

	if (0 > err)
		return err;

	return !err;
}

int mali_internal_sync_fence_cancel_async(struct mali_internal_sync_fence *sync_fence,
			     struct mali_internal_sync_fence_waiter *waiter)
{
	unsigned long flags;
	int ret = 0;

	MALI_DEBUG_ASSERT_POINTER(sync_fence);
	MALI_DEBUG_ASSERT_POINTER(waiter);

	spin_lock_irqsave(&sync_fence->wq.lock, flags);
	if (!list_empty(&waiter->work.task_list))
		list_del_init(&waiter->work.task_list);
	else
		ret = -ENOENT;
	spin_unlock_irqrestore(&sync_fence->wq.lock, flags);

	return ret;
}

#if defined(DEBUG)
static void mali_internal_sync_timeline_show(void)
{
	struct mali_session_data *session, *tmp;
	u32 session_seq = 1;
	MALI_DEBUG_PRINT(2, ("timeline system info: \n=================\n\n"));

	mali_session_lock();
	MALI_SESSION_FOREACH(session, tmp, link) {
		MALI_DEBUG_PRINT(2, ("session %d <%p> start:\n", session_seq, session));
		mali_timeline_debug_print_system(session->timeline_system, NULL);
		MALI_DEBUG_PRINT(2, ("session %d end\n\n\n", session_seq++));
	}
	mali_session_unlock();
}
#endif
static int mali_internal_sync_fence_wait(struct mali_internal_sync_fence *sync_fence, long timeout)
{
	long ret;
	MALI_DEBUG_ASSERT_POINTER(sync_fence);
	
	if (0 > timeout)
		timeout = MAX_SCHEDULE_TIMEOUT;
	else
		timeout = msecs_to_jiffies(timeout);

	ret = wait_event_interruptible_timeout(sync_fence->wq,
		atomic_read(&sync_fence->status) <= 0, timeout);

	if (0  > ret) {
		return ret;
	} else if (ret == 0) {
		if (timeout) {
			int i;
			MALI_DEBUG_PRINT(2, ("Mali internal sync:fence timeout on [%p] after %dms\n",
				sync_fence, jiffies_to_msecs(timeout)));

			for (i = 0; i < atomic_read(&sync_fence->num_fences); ++i) {
				sync_fence->cbs[i].base->ops->fence_value_str(sync_fence->cbs[i].base, NULL, 0);
			}

#if defined(DEBUG)
			mali_internal_sync_timeline_show();
#endif

		}
		return -ETIME;
	}

	ret = atomic_read(&sync_fence->status);
	if (ret) {
		int i;
		MALI_DEBUG_PRINT(2, ("fence error %ld on [%p]\n", ret, sync_fence));
		for (i = 0; i < atomic_read(&sync_fence->num_fences); ++i) {
				sync_fence->cbs[i].base->ops->fence_value_str(sync_fence->cbs[i].base, NULL, 0);
			}
#if defined(DEBUG)
		mali_internal_sync_timeline_show();
#endif
	}
	return ret;
}

static const char *mali_internal_fence_get_driver_name(struct fence *fence)
{
	struct mali_internal_sync_point *sync_pt;
	struct mali_internal_sync_timeline *parent;

	MALI_DEBUG_ASSERT_POINTER(fence);
	
	sync_pt = mali_internal_fence_to_sync_pt(fence);
	parent = mali_internal_sync_pt_to_sync_timeline(sync_pt);

	return parent->ops->driver_name;
}

static const char *mali_internal_fence_get_timeline_name(struct fence *fence)
{
	struct mali_internal_sync_point *sync_pt;
	struct mali_internal_sync_timeline *parent;

	MALI_DEBUG_ASSERT_POINTER(fence);
	
	sync_pt = mali_internal_fence_to_sync_pt(fence);
	parent = mali_internal_sync_pt_to_sync_timeline(sync_pt);

	return parent->name;
}

static void mali_internal_fence_release(struct fence *fence)
{
	unsigned long flags;
	struct mali_internal_sync_point *sync_pt;
	struct mali_internal_sync_timeline *parent;

	MALI_DEBUG_ASSERT_POINTER(fence);
	
	sync_pt = mali_internal_fence_to_sync_pt(fence);
	parent = mali_internal_sync_pt_to_sync_timeline(sync_pt);


	spin_lock_irqsave(fence->lock, flags);
	if (WARN_ON_ONCE(!list_empty(&sync_pt->sync_pt_list)))
		list_del(&sync_pt->sync_pt_list);
	spin_unlock_irqrestore(fence->lock, flags);

	if (parent->ops->free_pt)
		parent->ops->free_pt(sync_pt);

	kref_put(&parent->kref_count, mali_internal_sync_timeline_free);
	fence_free(&sync_pt->base);
}

static bool mali_internal_fence_signaled(struct fence *fence)
{
	int ret;
	struct mali_internal_sync_point *sync_pt;
	struct mali_internal_sync_timeline *parent;

	MALI_DEBUG_ASSERT_POINTER(fence);
	
	sync_pt = mali_internal_fence_to_sync_pt(fence);
	parent = mali_internal_sync_pt_to_sync_timeline(sync_pt);

	ret = parent->ops->has_signaled(sync_pt);
	if (0 > ret)
		fence->status = ret;
	return ret;
}

static bool mali_internal_fence_enable_signaling(struct fence *fence)
{
	struct mali_internal_sync_point *sync_pt;
	struct mali_internal_sync_timeline *parent;

	MALI_DEBUG_ASSERT_POINTER(fence);
	
	sync_pt = mali_internal_fence_to_sync_pt(fence);
	parent = mali_internal_sync_pt_to_sync_timeline(sync_pt);

	if (mali_internal_fence_signaled(fence))
		return false;

	list_add_tail(&sync_pt->sync_pt_list, &parent->sync_pt_list_head);
	return true;
}

static void mali_internal_fence_value_str(struct fence *fence,
				    char *str, int size)
{
	struct mali_internal_sync_point *sync_pt;
	struct mali_internal_sync_timeline *parent;

	MALI_DEBUG_ASSERT_POINTER(fence);
	MALI_IGNORE(str);
	MALI_IGNORE(size);
	
	sync_pt = mali_internal_fence_to_sync_pt(fence);
	parent = mali_internal_sync_pt_to_sync_timeline(sync_pt);

	parent->ops->print_sync_pt(sync_pt);
}

static const struct fence_ops fence_ops = {
	.get_driver_name = mali_internal_fence_get_driver_name,
	.get_timeline_name = mali_internal_fence_get_timeline_name,
	.enable_signaling = mali_internal_fence_enable_signaling,
	.signaled = mali_internal_fence_signaled,
	.wait = fence_default_wait,
	.release = mali_internal_fence_release,
	.fence_value_str = mali_internal_fence_value_str,
};

static void mali_internal_sync_fence_free(struct kref *kref_count)
{
	struct mali_internal_sync_fence *sync_fence;
	int i, num_fences;
	
	MALI_DEBUG_ASSERT_POINTER(kref_count);

	sync_fence = container_of(kref_count, struct mali_internal_sync_fence, kref_count);
	num_fences = atomic_read(&sync_fence->num_fences);

	for (i = 0; i <num_fences; ++i) {
		fence_remove_callback(sync_fence->cbs[i].base, &sync_fence->cbs[i].cb);
		fence_put(sync_fence->cbs[i].base);
	}

	kfree(sync_fence);
}

static int mali_internal_sync_fence_release(struct inode *inode, struct file *file)
{
	struct mali_internal_sync_fence *sync_fence;
	MALI_IGNORE(inode);
	MALI_DEBUG_ASSERT_POINTER(file);
	sync_fence = file->private_data;
	kref_put(&sync_fence->kref_count, mali_internal_sync_fence_free);
	return 0;
}

static unsigned int mali_internal_sync_fence_poll(struct file *file, poll_table *wait)
{
	int status;
	struct mali_internal_sync_fence *sync_fence;

	MALI_DEBUG_ASSERT_POINTER(file);
	MALI_DEBUG_ASSERT_POINTER(wait);

	sync_fence = file->private_data;
	poll_wait(file, &sync_fence->wq, wait);
	status = atomic_read(&sync_fence->status);

	if (!status)
		return POLLIN;
	else if (status < 0)
		return POLLERR;
	return 0;
}

static long mali_internal_sync_fence_ioctl_wait(struct mali_internal_sync_fence *sync_fence, unsigned long arg)
{
	s32 value;
	MALI_DEBUG_ASSERT_POINTER(sync_fence);

	if (copy_from_user(&value, (void __user *)arg, sizeof(value))) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to copy from user when sync fence ioctl wait.\n"));
		return -EFAULT;
	}
	return mali_internal_sync_fence_wait(sync_fence, value);
}

static long mali_internal_sync_fence_ioctl_merge(struct mali_internal_sync_fence *old_sync_fence1, unsigned long arg)
{
	int err;
	struct mali_internal_sync_fence *old_sync_fence2, *new_sync_fence;
	struct mali_internal_sync_merge_data data;
	int fd;
	
	MALI_DEBUG_ASSERT_POINTER(old_sync_fence1);

	fd = get_unused_fd_flags(O_CLOEXEC);

	if (0 > fd) {
		MALI_PRINT_ERROR(("Mali internal sync:Invaid fd when sync fence ioctl merge.\n"));
		return fd;
	}
	if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to copy from user when sync fence ioctl merge.\n"));
		err = -EFAULT;
		goto copy_from_user_failed;
	}

	old_sync_fence2 = mali_internal_sync_fence_fdget(data.fd);
	if (NULL == old_sync_fence2) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to sync fence fdget when sync fence ioctl merge.\n"));
		err = -ENOENT;
		goto sync_fence_fdget_failed;
	}

	new_sync_fence = mali_internal_sync_fence_merge(old_sync_fence1, old_sync_fence2);
	if (NULL == new_sync_fence) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to sync fence merge when sync fence ioctl merge.\n"));
		err = -ENOMEM;
		goto sync_fence_merge_failed;
	}

	data.fence = fd;
	if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to copy to user when sync fence ioctl merge.\n"));
		err = -EFAULT;
		goto copy_to_user_failed;
	}

	fd_install(fd, new_sync_fence->file);
	fput(old_sync_fence2->file);
	return 0;

copy_to_user_failed:
	fput(new_sync_fence->file);
sync_fence_merge_failed:
	fput(old_sync_fence2->file);
sync_fence_fdget_failed:
copy_from_user_failed:
	put_unused_fd(fd);
	return err;
}

static long mali_internal_sync_fence_ioctl_fence_info(struct mali_internal_sync_fence *sync_fence, unsigned long arg)
{
	struct mali_internal_sync_info_data *sync_info_data;
	u32 size;
	char name[32]  = "mali_internal_fence";
	u32 len = sizeof(struct mali_internal_sync_info_data);
	int num_fences, err, i;

	if (copy_from_user(&size, (void __user *)arg, sizeof(size))) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to copy from user when sync fence ioctl fence data info.\n"));
		err = -EFAULT;
		goto copy_from_user_failed;
	}

	if (size < sizeof(struct mali_internal_sync_info_data)) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to data size check when sync fence ioctl fence data info.\n"));
		err=  -EINVAL;
		goto data_size_check_failed;
	}

	if (size > 4096)
		size = 4096;

	sync_info_data = kzalloc(size, GFP_KERNEL);
	if (sync_info_data  == NULL) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to allocate buffer  when sync fence ioctl fence data info.\n"));
		err = -ENOMEM;
		goto allocate_buffer_failed;
	}

	strlcpy(sync_info_data->name, name, sizeof(sync_info_data->name));

	sync_info_data->status = atomic_read(&sync_fence->status);
	if (sync_info_data->status >= 0)
		sync_info_data->status = !sync_info_data->status;

	num_fences = atomic_read(&sync_fence->num_fences);

	for (i = 0; i < num_fences; ++i) {
		struct mali_internal_sync_pt_info *sync_pt_info = NULL;
		struct fence *base = sync_fence->cbs[i].base;

		if ((size - len) < sizeof(struct mali_internal_sync_pt_info)) {
			MALI_PRINT_ERROR(("Mali internal sync:Failed to fence size check  when sync fence ioctl fence data info.\n"));
			err = -ENOMEM;
			goto fence_size_check_failed;
			
		}

		sync_pt_info = (struct mali_internal_sync_pt_info *)((u8 *)sync_info_data + len);
		sync_pt_info->len = sizeof(struct mali_internal_sync_pt_info);

		strlcpy(sync_pt_info->obj_name, base->ops->get_timeline_name(base), sizeof(sync_pt_info->obj_name));
		strlcpy(sync_pt_info->driver_name, base->ops->get_driver_name(base), sizeof(sync_pt_info->driver_name));
		
		if (fence_is_signaled(base))
			sync_pt_info->status = base->status >= 0 ? 1 : base->status;
		else
			sync_pt_info->status = 0;
		
		sync_pt_info->timestamp_ns = ktime_to_ns(base->timestamp);

		len += sync_pt_info->len;
	}

	sync_info_data->len = len;

	if (copy_to_user((void __user *)arg, sync_info_data, len)) {
		MALI_PRINT_ERROR(("Mali internal sync:Failed to copy to user when sync fence ioctl fence data info.\n"));
		err = -EFAULT;
		goto copy_to_user_failed;
	}

	err = 0;

copy_to_user_failed:
fence_size_check_failed:
	kfree(sync_info_data);
allocate_buffer_failed:
data_size_check_failed:
copy_from_user_failed:
	return err;
}

static long mali_internal_sync_fence_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	struct mali_internal_sync_fence *sync_fence = file->private_data;

	switch (cmd) {
	case MALI_INTERNAL_SYNC_IOC_WAIT:
		return mali_internal_sync_fence_ioctl_wait(sync_fence, arg);

	case MALI_INTERNAL_SYNC_IOC_MERGE:
		return mali_internal_sync_fence_ioctl_merge(sync_fence, arg);

	case MALI_INTERNAL_SYNC_IOC_FENCE_INFO:
		return mali_internal_sync_fence_ioctl_fence_info(sync_fence, arg);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations sync_fence_fops = {
	.release = mali_internal_sync_fence_release,
	.poll = mali_internal_sync_fence_poll,
	.unlocked_ioctl = mali_internal_sync_fence_ioctl,
	.compat_ioctl = mali_internal_sync_fence_ioctl,
};
#endif
