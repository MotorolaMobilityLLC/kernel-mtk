/*
 * Driver for /dev/crypto device (aka CryptoDev)
 *
 * Copyright (c) 2009-2013 Nikos Mavrogiannopoulos <nmav@gnutls.org>
 * Copyright (c) 2010 Phil Sutter
 * Copyright (c) 2011, 2012 OpenSSL Software Foundation, Inc.
 *
 * This file is part of linux cryptodev.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/uaccess.h>
#include <crypto/scatterwalk.h>
#include <linux/scatterlist.h>
#include "cryptodev_int.h"
#include "zc.h"
#include "version.h"
#include "ssi_secure_key_kapi.h"
#include "secure_address_translation.h"

#if 0
extern int ssi_buffer_mgr_init_scatterlist_one(struct scatterlist *sgl, dma_addr_t addr, unsigned int length);

int ssi_buffer_mgr_init_scatterlist_one(
	struct scatterlist *sgl, dma_addr_t addr, unsigned int length)
{
	if (sgl == NULL) {
		return -EINVAL;
	}
	/* Init the table */
	sg_init_table(sgl, 1);
	/* set the dma addr */
	sg_dma_address(sgl) = addr;
	/* set the length */
	sg_dma_len(sgl) = length;

	return 0;
}
#endif
/* Helper functions to assist zero copy.
 * This needs to be redesigned and moved out of the session. --nmav
 */

/* offset of buf in it's first page */
#define PAGEOFFSET(buf) ((unsigned long)buf & ~PAGE_MASK)

/* fetch the pages addr resides in into pg and initialise sg with them */
int __get_userbuf(uint8_t __user *addr, uint32_t len, int write,
        unsigned int pgcount, struct page **pg, struct scatterlist *sg,
        struct task_struct *task, struct mm_struct *mm)
{
    int ret, pglen, i = 0;
    struct scatterlist *sgp;

    if (unlikely(!pgcount || !len || !addr)) {
        sg_mark_end(sg);
        return 0;
    }

    down_read(&mm->mmap_sem);
    ret = get_user_pages(task, mm,
            (unsigned long)addr, pgcount, write, 0, pg, NULL);
    up_read(&mm->mmap_sem);
    if (ret != pgcount)
        return -EINVAL;

    sg_init_table(sg, pgcount);

    pglen = min((ptrdiff_t)(PAGE_SIZE - PAGEOFFSET(addr)), (ptrdiff_t)len);
    sg_set_page(sg, pg[i++], pglen, PAGEOFFSET(addr));

    len -= pglen;
    for (sgp = sg_next(sg); len; sgp = sg_next(sgp)) {
        pglen = min((uint32_t)PAGE_SIZE, len);
        sg_set_page(sgp, pg[i++], pglen, 0);
        len -= pglen;
    }
    sg_mark_end(sg_last(sg, pgcount));
    return 0;
}

int adjust_sg_array(struct csession *ses, int pagecount)
{
    struct scatterlist *sg;
    struct page **pages;
    int array_size;

    for (array_size = ses->array_size; array_size < pagecount;
         array_size *= 2)
        ;
    ddebug(0, "reallocating from %d to %d pages",
            ses->array_size, array_size);
    pages = krealloc(ses->pages, array_size * sizeof(struct page *),
             GFP_KERNEL);
    if (unlikely(!pages))
        return -ENOMEM;
    ses->pages = pages;
    sg = krealloc(ses->sg, array_size * sizeof(struct scatterlist),
              GFP_KERNEL);
    if (unlikely(!sg))
        return -ENOMEM;
    ses->sg = sg;
    ses->array_size = array_size;

    return 0;
}

void release_user_pages(struct csession *ses)
{
    unsigned int i;

    for (i = 0; i < ses->used_pages; i++) {
        if (!PageReserved(ses->pages[i]))
            SetPageDirty(ses->pages[i]);

        if (ses->readonly_pages == 0)
            flush_dcache_page(ses->pages[i]);
        else
            ses->readonly_pages--;

        page_cache_release(ses->pages[i]);
    }
    ses->used_pages = 0;
}

/* make src and dst available in scatterlists.
 * dst might be the same as src.
 */
int get_userbuf(struct csession *ses,
                void *__user src, unsigned int src_len,
                void *__user dst, unsigned int dst_len,
                struct task_struct *task, struct mm_struct *mm,
                struct scatterlist **src_sg,
                struct scatterlist **dst_sg)
{
    int src_pagecount, dst_pagecount;
    int rc;

    dinfo(0, "get_userbuf src=%p src_len=%d dst=%p dst_len=%d", src, src_len, dst, dst_len);
    /* Empty input is a valid option to many algorithms & is tested by NIST/FIPS */
    /* Make sure NULL input has 0 length */
    if (!src && src_len)
        src_len = 0;

    /* I don't know that null output is ever useful, but we can handle it gracefully */
    /* Make sure NULL output has 0 length */
    if (!dst && dst_len)
        dst_len = 0;

    src_pagecount = PAGECOUNT(src, src_len);
    dst_pagecount = PAGECOUNT(dst, dst_len);

    ses->used_pages = (src == dst) ? max(src_pagecount, dst_pagecount)
                                   : src_pagecount + dst_pagecount;

    ses->readonly_pages = (src == dst) ? 0 : src_pagecount;

    if (ses->used_pages > ses->array_size) {
        rc = adjust_sg_array(ses, ses->used_pages);
        dinfo(0, "adjust_sg_array returns rc=0x%08x", rc);
        if (rc)
            return rc;
    }

    if (src == dst) {   /* inplace operation */
        /* When we encrypt for authenc modes we need to write
         * more data than the ones we read. */
        if (src_len < dst_len)
            src_len = dst_len;
        rc = __get_userbuf(src, src_len, 1, ses->used_pages,
                           ses->pages, ses->sg, task, mm);
        if (unlikely(rc)) {
            dinfo(0, "failed to get user pages for data IO. rc=0x%08x",rc);
            derr(1, "failed to get user pages for data IO");
            return rc;
        }
        (*src_sg) = (*dst_sg) = ses->sg;
        return 0;
    }

    *src_sg = NULL; /* default to no input */
    *dst_sg = NULL; /* default to ignore output */

    if (likely(src)) {
        rc = __get_userbuf(src, src_len, 0, ses->readonly_pages,
                       ses->pages, ses->sg, task, mm);
        if (unlikely(rc)) {
            dinfo(0, "failed to get user pages for data input. rc=0x%08x",rc);
            derr(1, "failed to get user pages for data input");
            return rc;
        }
        *src_sg = ses->sg;
    }

    if (likely(dst)) {
        const unsigned int writable_pages =
            ses->used_pages - ses->readonly_pages;
        struct page **dst_pages = ses->pages + ses->readonly_pages;
        *dst_sg = ses->sg + ses->readonly_pages;

        rc = __get_userbuf(dst, dst_len, 1, writable_pages,
                       dst_pages, *dst_sg, task, mm);
        if (unlikely(rc)) {
            dinfo(0, "failed to get user pages for data output. rc=0x%08x",rc);
            derr(1, "failed to get user pages for data output");
            release_user_pages(ses);  /* FIXME: use __release_userbuf(src, ...) */
            return rc;
        }
    }
    return 0;
}

/* make src and dst available in scatterlists.
if encrypt , src param must be a secure address.
if decrypt , dst param must be a secure address.
 */
int get_userbuf_and_securebuf(struct csession *ses,
                int operation,
                void *__user src, unsigned int src_len,
                void *__user dst, unsigned int dst_len,
                struct task_struct *task, struct mm_struct *mm,
                struct scatterlist **src_sg,
                struct scatterlist **dst_sg)
{
    int src_pagecount=0, dst_pagecount=0;
    int rc;
    struct page **dst_pages;
    //Check function is used only for secure ciphers
    if (ses->cdata.secure == 0)
        return -EPERM;

    //translate secure address handle into a Secured address 
    if (operation == COP_ENCRYPT)
        src = Get_Secure_Address_From_Handle(src); 
    else
        dst = Get_Secure_Address_From_Handle(dst); 

    dinfo(0, "op=%s src=%p src_len=%d dst=%p dst_len=%d", operation==COP_ENCRYPT?"COP_ENCRYPT":"COP_DECRYPT", src, src_len, dst, dst_len);

    if (unlikely(operation != COP_ENCRYPT && operation != COP_DECRYPT)) {
        dinfo(0, "invalid operation op=%u", operation);
        return -EINVAL;
    }
    
    /* Empty input is a valid option to many algorithms & is tested by NIST/FIPS */
    /* Make sure NULL input has 0 length */
    if (!src && src_len)
        src_len = 0;

    /* I don't know that null output is ever useful, but we can handle it gracefully */
    /* Make sure NULL output has 0 length */
    if (!dst && dst_len)
        dst_len = 0;

    if (operation == COP_ENCRYPT) {
        dst_pagecount = PAGECOUNT(dst, dst_len);
    } else {
        src_pagecount = PAGECOUNT(src, src_len);

    }

    ses->used_pages = src_pagecount + dst_pagecount;
    ses->readonly_pages = src_pagecount;

    dinfo(0, "src_pagecount=%d dst_pagecount=%d used_pages=%d readonly_pages=%d", src_pagecount, dst_pagecount, ses->used_pages, ses->readonly_pages);
    if (ses->used_pages > ses->array_size) {
        rc = adjust_sg_array(ses, ses->used_pages);
        dinfo(0, "adjust_sg_array returns rc=0x%08x", rc);
        if (rc)
            return rc;
    }

    //extend check so it check that one address in not from secure regions. meanwhile...
    if (src == dst) {   /* inplace operation */
        dinfo(0, "invalid operation src and dest should not be same ");
        return -EINVAL;
    }

    *src_sg = NULL; /* default to no input */
    *dst_sg = NULL; /* default to ignore output */

    if (likely(src)) {
        if (operation == COP_ENCRYPT) {
            dinfo(0, "calling ssi_buffer_mgr_init_scatterlist_one() on src %p", src);
            rc = ssi_buffer_mgr_init_scatterlist_one(ses->sg, (dma_addr_t)src, src_len);
        } else
            rc = __get_userbuf(src, src_len, 0, ses->readonly_pages,
                           ses->pages, ses->sg, task, mm);

        if (unlikely(rc)) {
            dinfo(0, "failed to get user pages for data input. rc=0x%08x",rc);
            derr(1, "failed to get user pages for data input");
            return rc;
        }
        *src_sg = ses->sg;
    }

    if (likely(dst)) {
        const unsigned int writable_pages =
            ses->used_pages - ses->readonly_pages;
        dinfo(0, "writable_pages=%d used_pages=%d readonly_pages=%d", writable_pages, ses->used_pages, ses->readonly_pages);
	dst_pages = ses->pages + ses->readonly_pages;
        *dst_sg = ses->sg + ses->readonly_pages;
        if ( *dst_sg == *src_sg )
            *dst_sg += 1;
        dinfo(0, "ses->sg=%p ses->readonly_pages=%d *dst_sg=%p", ses->sg, ses->readonly_pages, *dst_sg);

        if (operation == COP_ENCRYPT)
            rc = __get_userbuf(dst, dst_len, 1, writable_pages,
                           dst_pages, *dst_sg, task, mm);
        else {
            dinfo(0, "calling ssi_buffer_mgr_init_scatterlist_one() on dst %p", dst);
            rc = ssi_buffer_mgr_init_scatterlist_one(*dst_sg, (dma_addr_t)dst, dst_len);
        }

        if (unlikely(rc)) {
            dinfo(0, "failed to get user pages for data output. rc=0x%08x",rc);
            derr(1, "failed to get user pages for data output");
            release_user_pages(ses);  /* FIXME: use __release_userbuf(src, ...) */
            return rc;
        }
    }
    return 0;
}



