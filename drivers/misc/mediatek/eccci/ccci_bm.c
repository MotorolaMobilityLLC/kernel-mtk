#include <linux/skbuff.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <mt-plat/mt_ccci_common.h>
#include "ccci_config.h"
#include "ccci_bm.h"
#ifdef CCCI_BM_TRACE
#define CREATE_TRACE_POINTS
#include "ccci_bm_events.h"
#endif

#define REQ_MAGIC_HEADER 0xF111F111
#define REQ_MAGIC_FOOTER 0xF222F222

#define SKB_MAGIC_HEADER 0xF333F333
#define SKB_MAGIC_FOOTER 0xF444F444

struct ccci_req_queue req_pool;
struct ccci_skb_queue skb_pool_4K;
struct ccci_skb_queue skb_pool_1_5K;
struct ccci_skb_queue skb_pool_16;

struct workqueue_struct *pool_reload_work_queue;

#ifdef CCCI_BM_TRACE
struct timer_list ccci_bm_stat_timer;
void ccci_bm_stat_timer_func(unsigned long data)
{
	trace_ccci_bm(req_pool.count, skb_pool_4K.skb_list.qlen, skb_pool_1_5K.skb_list.qlen,
		      skb_pool_16.skb_list.qlen);
	mod_timer(&ccci_bm_stat_timer, jiffies + HZ / 2);
}
#endif

#ifdef CCCI_WP_DEBUG
#include <mt-plat/hw_watchpoint.h>

static struct wp_event wp_event;
static atomic_t hwp_enable = ATOMIC_INIT(0);

static int my_wp_handler(phys_addr_t addr)
{

	CCCI_INF_MSG(-1, BM, "[ccci/WP_LCH_DEBUG] access from 0x%p, call bug\n", (void *)addr);
	dump_stack();
	/*BUG();*/

	/* re-enable the watchpoint, since the auto-disable is not working */
	del_hw_watchpoint(&wp_event);
#if 0
	wp_err = add_hw_watchpoint(&wp_event);
	if (wp_err != 0)
		/* error */
		CCCI_INF_MSG(-1, BM, "[mydebug]watchpoint init fail\n");
	else
		/* success */
		CCCI_INF_MSG(-1, BM, "[mydebug]watchpoint init done\n");
#endif
	return 0;
}
/*
static void disable_watchpoint(void)
{
	if (atomic_read(&hwp_enable)) {
		del_hw_watchpoint(&wp_event);
		atomic_set(&hwp_enable, 0);
	}
}
*/
static void enable_watchpoint(void *address)
{
	int wp_err;

	if (atomic_read(&hwp_enable) == 0) {
		init_wp_event(&wp_event, (phys_addr_t) address, (phys_addr_t) address,
			WP_EVENT_TYPE_WRITE, my_wp_handler);
		atomic_set(&hwp_enable, 1);
		wp_err = add_hw_watchpoint(&wp_event);
		if (wp_err)
			CCCI_INF_MSG(-1, BM, "[mydebug]watchpoint init fail,addr=%p\n", address);
	}
}
#endif

#ifdef CCCI_MEM_BM_DEBUG
static int ccci_skb_addr_checker(struct sk_buff *newsk)
{
	unsigned long skb_addr_value;
	unsigned long queue16_addr_value;
	unsigned long queue1_5k_addr_value;
	unsigned long queue4k_addr_value;
	unsigned long req_pool_addr_value;

	skb_addr_value = (unsigned long)newsk;
	queue16_addr_value = (unsigned long)&skb_pool_16;
	queue1_5k_addr_value = (unsigned long)&skb_pool_1_5K;
	queue4k_addr_value = (unsigned long)&skb_pool_4K;
	req_pool_addr_value = (unsigned long)&req_pool;

	if ((skb_addr_value >= queue16_addr_value
			&& skb_addr_value < queue16_addr_value + sizeof(struct ccci_skb_queue))
		||
		(skb_addr_value >= queue1_5k_addr_value
			&& skb_addr_value < queue1_5k_addr_value + sizeof(struct ccci_skb_queue))
		||
		(skb_addr_value >= queue4k_addr_value
			&& skb_addr_value < queue4k_addr_value + sizeof(struct ccci_skb_queue))
		||
		(skb_addr_value >= req_pool_addr_value
			&& skb_addr_value < req_pool_addr_value + sizeof(struct ccci_req_queue))
		) {
		CCCI_INF_MSG(-1, BM, "Free wrong skb=%lx pointer in skb poool!\n", skb_addr_value);
		CCCI_INF_MSG(-1, BM, "skb=%lx, skb_pool_16=%lx,  skb_pool_1_5K=%lx, skb_pool_4K=%lx, req_pool=%lx!\n",
			skb_addr_value, queue16_addr_value, queue1_5k_addr_value,
			queue4k_addr_value,
			req_pool_addr_value);

		return 1;
	}
	return 0;
}
void ccci_magic_checker(void)
{
	if (req_pool.magic_header != REQ_MAGIC_HEADER || req_pool.magic_footer != REQ_MAGIC_FOOTER) {
		CCCI_INF_MSG(-1, BM, "req_pool magic error!\n");
		ccci_mem_dump(-1, &req_pool, sizeof(struct ccci_req_queue));
		dump_stack();
	}

	if (skb_pool_16.magic_header != SKB_MAGIC_HEADER || skb_pool_16.magic_footer != SKB_MAGIC_FOOTER) {
		CCCI_INF_MSG(-1, BM, "skb_pool_16 magic error!\n");
		ccci_mem_dump(-1, &skb_pool_16, sizeof(struct ccci_skb_queue));
		dump_stack();
	}

	if (skb_pool_1_5K.magic_header != SKB_MAGIC_HEADER || skb_pool_1_5K.magic_footer != SKB_MAGIC_FOOTER) {
		CCCI_INF_MSG(-1, BM, "skb_pool_1_5K magic error!\n");
		ccci_mem_dump(-1, &skb_pool_1_5K, sizeof(struct ccci_skb_queue));
		dump_stack();
	}

	if (skb_pool_4K.magic_header != SKB_MAGIC_HEADER || skb_pool_4K.magic_footer != SKB_MAGIC_FOOTER) {
		CCCI_INF_MSG(-1, BM, "skb_pool_4K magic error!\n");
		ccci_mem_dump(-1, &skb_pool_4K, sizeof(struct ccci_skb_queue));
		dump_stack();
	}
}
#endif

static struct ccci_request *ccci_req_dequeue(struct ccci_req_queue *queue)
{
	unsigned long flags;
	struct ccci_request *result = NULL;

#ifdef CCCI_MEM_BM_DEBUG
	if (queue->magic_header != REQ_MAGIC_HEADER
		|| queue->magic_footer != REQ_MAGIC_FOOTER) {
		ccci_mem_dump(-1, queue, sizeof(struct ccci_req_queue));
		dump_stack();
	}
#endif
	spin_lock_irqsave(&queue->req_lock, flags);
	if (list_empty(&queue->req_list))
		goto out;
	result = list_first_entry(&queue->req_list, struct ccci_request, entry);
	if (result) {
		queue->count--;
		list_del(&result->entry);
	}
out:
	spin_unlock_irqrestore(&queue->req_lock, flags);
	return result;
}

static void ccci_req_enqueue(struct ccci_req_queue *queue, struct ccci_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&queue->req_lock, flags);
	ccci_request_struct_init(req);
	list_add_tail(&req->entry, &queue->req_list);
	queue->count++;
	spin_unlock_irqrestore(&queue->req_lock, flags);
}

static void ccci_req_queue_init(struct ccci_req_queue *queue)
{
	int i;

	queue->magic_header = REQ_MAGIC_HEADER;
	queue->magic_footer = REQ_MAGIC_FOOTER;
	queue->max_len = BM_POOL_SIZE;
	INIT_LIST_HEAD(&queue->req_list);
	for (i = 0; i < queue->max_len; i++) {
		struct ccci_request *req = kmalloc(sizeof(struct ccci_request), GFP_KERNEL);

		ccci_request_struct_init(req);
		list_add_tail(&req->entry, &queue->req_list);
	}
	queue->count = queue->max_len;
	spin_lock_init(&queue->req_lock);
	init_waitqueue_head(&queue->req_wq);
}

static inline struct sk_buff *__alloc_skb_from_pool(int size)
{
	struct sk_buff *skb = NULL;

	if (size > SKB_1_5K)
		skb = ccci_skb_dequeue(&skb_pool_4K);
	else if (size > SKB_16)
		skb = ccci_skb_dequeue(&skb_pool_1_5K);
	else if (size > 0)
		skb = ccci_skb_dequeue(&skb_pool_16);
	return skb;
}

static inline struct sk_buff *__alloc_skb_from_kernel(int size, gfp_t gfp_mask)
{
	struct sk_buff *skb = NULL;

	if (size > SKB_1_5K)
		skb = __dev_alloc_skb(SKB_4K, gfp_mask);
	else if (size > SKB_16)
		skb = __dev_alloc_skb(SKB_1_5K, gfp_mask);
	else if (size > 0)
		skb = __dev_alloc_skb(SKB_16, gfp_mask);
	if (!skb)
		CCCI_ERR_MSG(-1, BM, "%ps alloc skb from kernel fail, size=%d\n", __builtin_return_address(0), size);
	return skb;
}

struct sk_buff *ccci_skb_dequeue(struct ccci_skb_queue *queue)
{
	unsigned long flags;
	struct sk_buff *result;

#ifdef CCCI_MEM_BM_DEBUG
	if (queue->magic_header != SKB_MAGIC_HEADER || queue->magic_footer != SKB_MAGIC_FOOTER) {
		CCCI_ERR_MSG(-1, BM,
			"ccci_skb_dequeue: queue=%lx, skb_pool_16=%lx,  skb_pool_1_5K=%lx, skb_pool_4K=%lx, req_pool=%lx!\n",
			(unsigned long)queue, (unsigned long)&skb_pool_16, (unsigned long)&skb_pool_1_5K,
			(unsigned long)&skb_pool_4K,
			(unsigned long)&req_pool);
		ccci_mem_dump(-1, queue, sizeof(struct ccci_skb_queue));
		dump_stack();
	}
#endif

	spin_lock_irqsave(&queue->skb_list.lock, flags);
	result = __skb_dequeue(&queue->skb_list);
	if (queue->pre_filled && queue->skb_list.qlen < queue->max_len / RELOAD_TH)
		queue_work(pool_reload_work_queue, &queue->reload_work);
	spin_unlock_irqrestore(&queue->skb_list.lock, flags);
	return result;
}

void ccci_skb_enqueue(struct ccci_skb_queue *queue, struct sk_buff *newsk)
{
	unsigned long flags;

	spin_lock_irqsave(&queue->skb_list.lock, flags);
	if (queue->skb_list.qlen < queue->max_len) {
		__skb_queue_tail(&queue->skb_list, newsk);
		if (queue->skb_list.qlen > queue->max_history)
			queue->max_history = queue->skb_list.qlen;
	} else {
#if 0
		if (queue->pre_filled) {
			CCCI_ERR_MSG(0, BM, "skb queue too long, max=%d\n", queue->max_len);
#else
		if (1) {
#endif

#ifdef CCCI_MEM_BM_DEBUG
			if (ccci_skb_addr_checker(newsk)) {
				CCCI_INF_MSG(-1, BM, "ccci_skb_enqueue:ccci_skb_addr_checker failed!\n");
				ccci_mem_dump(-1, queue, sizeof(struct ccci_skb_queue));
				dump_stack();
			}
#endif
			dev_kfree_skb_any(newsk);
		} else {
			__skb_queue_tail(&queue->skb_list, newsk);
		}
	}
	spin_unlock_irqrestore(&queue->skb_list.lock, flags);
}

void ccci_skb_queue_init(struct ccci_skb_queue *queue, unsigned int skb_size, unsigned int max_len,
	char fill_now)
{
	int i;

	queue->magic_header = SKB_MAGIC_HEADER;
	queue->magic_footer = SKB_MAGIC_FOOTER;
#ifdef CCCI_WP_DEBUG
	if (((unsigned long)queue) == ((unsigned long)(&skb_pool_16))) {
		CCCI_INF_MSG(-1, BM, "ccci_skb_queue_init: add hwp skb_pool_16.magic_footer=%p!\n",
			&queue->magic_footer);
		enable_watchpoint(&queue->magic_footer);
	}
#endif
	skb_queue_head_init(&queue->skb_list);
	queue->max_len = max_len;
	if (fill_now) {
		for (i = 0; i < queue->max_len; i++) {
			struct sk_buff *skb = __alloc_skb_from_kernel(skb_size, GFP_KERNEL);
			if (skb != NULL)
				skb_queue_tail(&queue->skb_list, skb);
		}
		queue->pre_filled = 1;
	} else {
		queue->pre_filled = 0;
	}
	queue->max_history = 0;
}

/* may return NULL, caller should check, network should always use blocking as we do not want it consume our own pool */
struct sk_buff *ccci_alloc_skb(int size, char from_pool, char blocking)
{
	int count = 0;
	struct sk_buff *skb = NULL;

#ifdef CCCI_MEM_BM_DEBUG
	ccci_magic_checker();
#endif
	if (size > SKB_4K || size < 0)
		goto err_exit;

	if (from_pool) {
 slow_retry:
		skb = __alloc_skb_from_pool(size);
		if (unlikely(!skb && blocking)) {
			CCCI_INF_MSG(-1, BM, "skb pool is empty! size=%d (%d)\n", size, count++);
			msleep(100);
			goto slow_retry;
		}
	} else {
		if (blocking) {
			skb = __alloc_skb_from_kernel(size, GFP_KERNEL);
		} else {
 fast_retry:
			skb = __alloc_skb_from_kernel(size, GFP_ATOMIC);
			if (!skb && count++ < 20)
				goto fast_retry;
		}
	}
 err_exit:
	if (unlikely(!skb))
		CCCI_ERR_MSG(-1, BM, "%ps alloc skb fail, size=%d\n", __builtin_return_address(0), size);
	else
		CCCI_DBG_MSG(-1, BM, "%ps alloc skb %p, size=%d\n", __builtin_return_address(0), skb, size);
	return skb;
}
EXPORT_SYMBOL(ccci_alloc_skb);

void ccci_free_skb(struct sk_buff *skb, DATA_POLICY policy)
{
	CCCI_DBG_MSG(-1, BM, "%ps free skb %p, policy=%d, len=%d\n", __builtin_return_address(0),
		     skb, policy, skb_size(skb));
	switch (policy) {
	case RECYCLE:
		/* 1. reset sk_buff (take __alloc_skb as ref.) */
		skb->data = skb->head;
		skb->len = 0;
		skb_reset_tail_pointer(skb);
		/* 2. enqueue */
		if (skb_size(skb) < SKB_1_5K)
			ccci_skb_enqueue(&skb_pool_16, skb);
		else if (skb_size(skb) < SKB_4K)
			ccci_skb_enqueue(&skb_pool_1_5K, skb);
		else
			ccci_skb_enqueue(&skb_pool_4K, skb);
		break;
	case FREE:
#ifdef CCCI_MEM_BM_DEBUG
		if (ccci_skb_addr_checker(skb)) {
			CCCI_INF_MSG(-1, BM, "ccci_skb_addr_checker failed\n");
			dump_stack();
		}
#endif
		dev_kfree_skb_any(skb);
		break;
	case NOOP:
	default:
		break;
	};
}
EXPORT_SYMBOL(ccci_free_skb);

static void __4K_reload_work(struct work_struct *work)
{
	struct sk_buff *skb;

	CCCI_DBG_MSG(-1, BM, "refill 4KB skb pool\n");
	while (skb_pool_4K.skb_list.qlen < SKB_POOL_SIZE_4K) {
		skb = __alloc_skb_from_kernel(SKB_4K, GFP_KERNEL);
		if (skb)
			skb_queue_tail(&skb_pool_4K.skb_list, skb);
		else
			CCCI_ERR_MSG(-1, BM, "fail to reload 4KB pool\n");
	}
}

static void __1_5K_reload_work(struct work_struct *work)
{
	struct sk_buff *skb;

	CCCI_DBG_MSG(-1, BM, "refill 1.5KB skb pool\n");
	while (skb_pool_1_5K.skb_list.qlen < SKB_POOL_SIZE_1_5K) {
		skb = __alloc_skb_from_kernel(SKB_1_5K, GFP_KERNEL);
		if (skb)
			skb_queue_tail(&skb_pool_1_5K.skb_list, skb);
		else
			CCCI_ERR_MSG(-1, BM, "fail to reload 1.5KB pool\n");

	}
}

static void __16_reload_work(struct work_struct *work)
{
	struct sk_buff *skb;

	CCCI_DBG_MSG(-1, BM, "refill 16B skb pool\n");
	while (skb_pool_16.skb_list.qlen < SKB_POOL_SIZE_16) {
		skb = __alloc_skb_from_kernel(SKB_16, GFP_KERNEL);
		if (skb)
			skb_queue_tail(&skb_pool_16.skb_list, skb);
		else
			CCCI_ERR_MSG(-1, BM, "fail to reload 16B pool\n");
	}
}

/*
 * a write operation may block at 3 stages:
 * 1. ccci_alloc_req
 * 2. wait until the queue has available slot (threshold check)
 * 3. wait until the SDIO transfer is complete --> abandoned, see the reason below.
 * the 1st one is decided by @blk1. and the 2nd and 3rd are decided by @blk2, waiting on @wq.
 * NULL is returned if no available skb, even when you set blk1=1.
 *
 * we removed the wait_queue_head_t in ccci_request, so user can NOT wait for certain request to
 * be completed. this is because request will be recycled and its state will be reset, so if a request
 * is completed and then used again, the poor guy who is waiting for it may never see the state
 * transition (FLYING->IDLE/COMPLETE->FLYING) and wait forever.
 */
struct ccci_request *ccci_alloc_req(DIRECTION dir, int size, char blk1, char blk2)
{
	struct ccci_request *req = NULL;

#ifdef CCCI_MEM_BM_DEBUG
	ccci_magic_checker();
#endif
 retry:
	req = ccci_req_dequeue(&req_pool);
	if (req) {
		if (size > 0) {
			req->skb = ccci_alloc_skb(size, 1, blk1);
			req->policy = RECYCLE;
			if (req->skb)
				CCCI_DBG_MSG(-1, BM, "alloc ok, req=%p skb=%p, len=%d\n", req, req->skb,
					     skb_size(req->skb));
		} else {
			req->skb = NULL;
			req->policy = NOOP;
		}
		req->blocking = blk2;
	} else {
		if (blk1) {
			wait_event_interruptible(req_pool.req_wq, (req_pool.count > 0));
			goto retry;
		}
		CCCI_INF_MSG(-1, BM, "fail to alloc req for %ps, no retry\n", __builtin_return_address(0));
	}
	if (unlikely(size > 0 && req && !req->skb)) {
		CCCI_ERR_MSG(-1, BM, "fail to alloc skb for %ps, size=%d\n", __builtin_return_address(0), size);
		req->policy = NOOP;
		ccci_free_req(req);
		req = NULL;
	}
	return req;
}
EXPORT_SYMBOL(ccci_alloc_req);

void ccci_free_req(struct ccci_request *req)
{
	CCCI_DBG_MSG(-1, BM, "%ps free req=%p, policy=%d, skb=%p\n", __builtin_return_address(0),
		     req, req->policy, req->skb);
	if (req->skb) {
		ccci_free_skb(req->skb, req->policy);
		req->skb = NULL;
	}
	if (req->entry.next != LIST_POISON1 || req->entry.prev != LIST_POISON2) {
		CCCI_ERR_MSG(-1, BM, "req %p entry not deleted yet, from %ps\n", req, __builtin_return_address(0));
		list_del(&req->entry);
	}
	ccci_req_enqueue(&req_pool, req);
	wake_up_all(&req_pool.req_wq);

}
EXPORT_SYMBOL(ccci_free_req);

void ccci_mem_dump(int md_id, void *start_addr, int len)
{
	unsigned int *curr_p = (unsigned int *)start_addr;
	unsigned char *curr_ch_p;
	int _16_fix_num = len / 16;
	int tail_num = len % 16;
	char buf[16];
	int i, j;

	if (NULL == curr_p) {
		CCCI_INF_MSG(md_id, BM, "NULL point to dump!\n");
		return;
	}
	if (0 == len) {
		CCCI_INF_MSG(md_id, BM, "Not need to dump\n");
		return;
	}

	CCCI_EXP_INF_MSG(md_id, BM, "Base: %p\n", start_addr);
	/* Fix section */
	for (i = 0; i < _16_fix_num; i++) {
		CCCI_DUMP_MSG2(md_id, BM, "%03X: %08X %08X %08X %08X\n",
		       i * 16, *curr_p, *(curr_p + 1), *(curr_p + 2), *(curr_p + 3));
		curr_p += 4;
	}

	/* Tail section */
	if (tail_num > 0) {
		curr_ch_p = (unsigned char *)curr_p;
		for (j = 0; j < tail_num; j++) {
			buf[j] = *curr_ch_p;
			curr_ch_p++;
		}
		for (; j < 16; j++)
			buf[j] = 0;
		curr_p = (unsigned int *)buf;
		CCCI_DUMP_MSG2(md_id, BM, "%03X: %08X %08X %08X %08X\n",
		       i * 16, *curr_p, *(curr_p + 1), *(curr_p + 2), *(curr_p + 3));
	}
}
EXPORT_SYMBOL(ccci_mem_dump);

void ccci_cmpt_mem_dump(int md_id, void *start_addr, int len)
{
	unsigned int *curr_p = (unsigned int *)start_addr;
	unsigned char *curr_ch_p;
	int _64_fix_num = len / 64;
	int tail_num = len % 64;
	char buf[64];
	int i, j;

	if (NULL == curr_p) {
		CCCI_INF_MSG(md_id, BM, "NULL point to dump!\n");
		return;
	}
	if (0 == len) {
		CCCI_INF_MSG(md_id, BM, "Not need to dump\n");
		return;
	}

	/* Fix section */
	for (i = 0; i < _64_fix_num; i++) {
		CCCI_INF_MSG(md_id, BM, "%03X: %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
		       i * 64,
		       *curr_p, *(curr_p + 1), *(curr_p + 2), *(curr_p + 3),
		       *(curr_p + 4), *(curr_p + 5), *(curr_p + 6), *(curr_p + 7),
		       *(curr_p + 8), *(curr_p + 9), *(curr_p + 10), *(curr_p + 11),
		       *(curr_p + 12), *(curr_p + 13), *(curr_p + 14), *(curr_p + 15));
		curr_p += 64/4;
	}

	/* Tail section */
	if (tail_num > 0) {
		curr_ch_p = (unsigned char *)curr_p;
		for (j = 0; j < tail_num; j++) {
			buf[j] = *curr_ch_p;
			curr_ch_p++;
		}
		for (; j < 64; j++)
			buf[j] = 0;
		curr_p = (unsigned int *)buf;
		CCCI_INF_MSG(md_id, BM, "%03X: %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
		       i * 64,
		       *curr_p, *(curr_p + 1), *(curr_p + 2), *(curr_p + 3),
		       *(curr_p + 4), *(curr_p + 5), *(curr_p + 6), *(curr_p + 7),
		       *(curr_p + 8), *(curr_p + 9), *(curr_p + 10), *(curr_p + 11),
		       *(curr_p + 12), *(curr_p + 13), *(curr_p + 14), *(curr_p + 15));
	}
}
EXPORT_SYMBOL(ccci_cmpt_mem_dump);

void ccci_dump_req(struct ccci_request *req)
{
	ccci_mem_dump(-1, req->skb->data, req->skb->len > 32 ? 32 : req->skb->len);
}
EXPORT_SYMBOL(ccci_dump_req);

int ccci_subsys_bm_init(void)
{
	/* init ccci_request */
	ccci_req_queue_init(&req_pool);
	CCCI_INF_MSG(-1, BM, "MTU=%d/%d, pool size %d/%d/%d/%d\n", CCCI_MTU, CCCI_NET_MTU,
		     SKB_POOL_SIZE_4K, SKB_POOL_SIZE_1_5K, SKB_POOL_SIZE_16, req_pool.max_len);
	/* init skb pool */
	ccci_skb_queue_init(&skb_pool_4K, SKB_4K, SKB_POOL_SIZE_4K, 1);
	ccci_skb_queue_init(&skb_pool_1_5K, SKB_1_5K, SKB_POOL_SIZE_1_5K, 1);
	ccci_skb_queue_init(&skb_pool_16, SKB_16, SKB_POOL_SIZE_16, 1);
	/* init pool reload work */
	pool_reload_work_queue = alloc_workqueue("pool_reload_work", WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	INIT_WORK(&skb_pool_4K.reload_work, __4K_reload_work);
	INIT_WORK(&skb_pool_1_5K.reload_work, __1_5K_reload_work);
	INIT_WORK(&skb_pool_16.reload_work, __16_reload_work);

#ifdef CCCI_BM_TRACE
	init_timer(&ccci_bm_stat_timer);
	ccci_bm_stat_timer.function = ccci_bm_stat_timer_func;
	mod_timer(&ccci_bm_stat_timer, jiffies + 10 * HZ);
#endif
	return 0;
}
