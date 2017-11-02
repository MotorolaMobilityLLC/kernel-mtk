//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fci_ringbuffer.c

	Description : source of data buffer control

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
	2010/11/25	aslan.cho	initial
*******************************************************************************/

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "fci_ringbuffer.h"

#define PKT_READY 0
#define PKT_DISPOSED 1

void fci_ringbuffer_init(struct fci_ringbuffer *rbuf, void *data, size_t len)
{
	rbuf->pread = rbuf->pwrite = 0;
	rbuf->data = data;
	rbuf->size = len;
	rbuf->error = 0;

	init_waitqueue_head(&rbuf->queue);

	spin_lock_init(&(rbuf->lock));
}

int fci_ringbuffer_empty(struct fci_ringbuffer *rbuf)
{
	return (rbuf->pread == rbuf->pwrite);
}

ssize_t fci_ringbuffer_free(struct fci_ringbuffer *rbuf)
{
	ssize_t free;

	free = rbuf->pread - rbuf->pwrite;
	if (free <= 0)
		free += rbuf->size;
	return free-1;
}

ssize_t fci_ringbuffer_avail(struct fci_ringbuffer *rbuf)
{
	ssize_t avail;

	avail = rbuf->pwrite - rbuf->pread;
	if (avail < 0)
		avail += rbuf->size;
	return avail;
}

void fci_ringbuffer_flush(struct fci_ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite;
	rbuf->error = 0;
}

void fci_ringbuffer_reset(struct fci_ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite = 0;
	rbuf->error = 0;
}

void fci_ringbuffer_flush_spinlock_wakeup(struct fci_ringbuffer *rbuf)
{
	unsigned long flags;

	spin_lock_irqsave(&rbuf->lock, flags);
	fci_ringbuffer_flush(rbuf);
	spin_unlock_irqrestore(&rbuf->lock, flags);

	wake_up(&rbuf->queue);
}

ssize_t fci_ringbuffer_read_user(struct fci_ringbuffer *rbuf
	, u8 __user *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
		if (copy_to_user(buf, rbuf->data+rbuf->pread, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		rbuf->pread = 0;
	}
	if (copy_to_user(buf, rbuf->data+rbuf->pread, todo))
		return -EFAULT;

	rbuf->pread = (rbuf->pread + todo) % rbuf->size;

	return len;
}

void fci_ringbuffer_read(struct fci_ringbuffer *rbuf, u8 *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
		memcpy(buf, rbuf->data+rbuf->pread, split);
		buf += split;
		todo -= split;
		rbuf->pread = 0;
	}
	memcpy(buf, rbuf->data+rbuf->pread, todo);

	rbuf->pread = (rbuf->pread + todo) % rbuf->size;
}

ssize_t fci_ringbuffer_write(struct fci_ringbuffer *rbuf
	, const u8 *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pwrite + len > rbuf->size)
		? rbuf->size - rbuf->pwrite : 0;

	if (split > 0) {
		memcpy(rbuf->data+rbuf->pwrite, buf, split);
		buf += split;
		todo -= split;
		rbuf->pwrite = 0;
	}
	memcpy(rbuf->data+rbuf->pwrite, buf, todo);
	rbuf->pwrite = (rbuf->pwrite + todo) % rbuf->size;

	return len;
}

ssize_t fci_ringbuffer_pkt_write(struct fci_ringbuffer *rbuf
	, u8 *buf, size_t len)
{
	int status;
	ssize_t oldpwrite = rbuf->pwrite;

	FCI_RINGBUFFER_WRITE_BYTE(rbuf, len >> 8);
	FCI_RINGBUFFER_WRITE_BYTE(rbuf, len & 0xff);
	FCI_RINGBUFFER_WRITE_BYTE(rbuf, PKT_READY);
	status = fci_ringbuffer_write(rbuf, buf, len);

	if (status < 0)
		rbuf->pwrite = oldpwrite;
	return status;
}

ssize_t fci_ringbuffer_pkt_read_user(struct fci_ringbuffer *rbuf, size_t idx,
				int offset, u8 __user *buf, size_t len)
{
	size_t todo;
	size_t split;
	size_t pktlen;

	pktlen = rbuf->data[idx] << 8;
	pktlen |= rbuf->data[(idx + 1) % rbuf->size];
	if (offset > pktlen)
		return -EINVAL;
	if ((offset + len) > pktlen)
		len = pktlen - offset;

	idx = (idx + FCI_RINGBUFFER_PKTHDRSIZE + offset) % rbuf->size;
	todo = len;
	split = ((idx + len) > rbuf->size) ? rbuf->size - idx : 0;
	if (split > 0) {
		if (copy_to_user(buf, rbuf->data+idx, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		idx = 0;
	}
	if (copy_to_user(buf, rbuf->data+idx, todo))
		return -EFAULT;

	return len;
}

ssize_t fci_ringbuffer_pkt_read(struct fci_ringbuffer *rbuf, size_t idx,
				int offset, u8 *buf, size_t len)
{
	size_t todo;
	size_t split;
	size_t pktlen;

	pktlen = rbuf->data[idx] << 8;
	pktlen |= rbuf->data[(idx + 1) % rbuf->size];
	if (offset > pktlen)
		return -EINVAL;
	if ((offset + len) > pktlen)
		len = pktlen - offset;

	idx = (idx + FCI_RINGBUFFER_PKTHDRSIZE + offset) % rbuf->size;
	todo = len;
	split = ((idx + len) > rbuf->size) ? rbuf->size - idx : 0;
	if (split > 0) {
		memcpy(buf, rbuf->data+idx, split);
		buf += split;
		todo -= split;
		idx = 0;
	}
	memcpy(buf, rbuf->data+idx, todo);
	return len;
}

void fci_ringbuffer_pkt_dispose(struct fci_ringbuffer *rbuf, size_t idx)
{
	size_t pktlen;

	rbuf->data[(idx + 2) % rbuf->size] = PKT_DISPOSED;

	while (fci_ringbuffer_avail(rbuf) > FCI_RINGBUFFER_PKTHDRSIZE) {
		if (FCI_RINGBUFFER_PEEK(rbuf, 2) == PKT_DISPOSED) {
			pktlen = FCI_RINGBUFFER_PEEK(rbuf, 0) << 8;
			pktlen |= FCI_RINGBUFFER_PEEK(rbuf, 1);
			FCI_RINGBUFFER_SKIP(rbuf
				, pktlen + FCI_RINGBUFFER_PKTHDRSIZE);
		} else
			break;
	}
}

ssize_t fci_ringbuffer_pkt_next(struct fci_ringbuffer *rbuf
	, size_t idx, size_t *pktlen)
{
	int consumed;
	int curpktlen;
	int curpktstatus;

	if (idx == -1)
		idx = rbuf->pread;
	else {
		curpktlen = rbuf->data[idx] << 8;
		curpktlen |= rbuf->data[(idx + 1) % rbuf->size];
		idx = (idx + curpktlen + FCI_RINGBUFFER_PKTHDRSIZE)
			% rbuf->size;
	}

	consumed = (idx - rbuf->pread) % rbuf->size;

	while ((fci_ringbuffer_avail(rbuf) - consumed)
		> FCI_RINGBUFFER_PKTHDRSIZE) {

		curpktlen = rbuf->data[idx] << 8;
		curpktlen |= rbuf->data[(idx + 1) % rbuf->size];
		curpktstatus = rbuf->data[(idx + 2) % rbuf->size];

		if (curpktstatus == PKT_READY) {
			*pktlen = curpktlen;
			return idx;
		}

		consumed += curpktlen + FCI_RINGBUFFER_PKTHDRSIZE;
		idx = (idx + curpktlen + FCI_RINGBUFFER_PKTHDRSIZE)
			% rbuf->size;
	}

	return -1;
}
//add dtv ---lenovo@lenovo.com ---20161119 end 