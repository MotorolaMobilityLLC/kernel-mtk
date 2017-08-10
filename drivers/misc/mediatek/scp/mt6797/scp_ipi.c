/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/mutex.h>
#include <mt-plat/sync_write.h>
#include "scp_ipi.h"
#include "scp_helper.h"
#include "mt_spm.h"
#define SPM_SW_RSV_3                   (SPM_BASE + 0x614)


struct scp_ipi_desc scp_ipi_desc[SCP_NR_IPI];
struct share_obj *scp_send_obj, *scp_rcv_obj;
struct mutex scp_ipi_mutex;

static inline void ipi_host2scp(void)
{
	HOST_TO_SCP_REG = 0x1;
}

/*
 * find an ipi handler and invoke it
 */
void scp_ipi_handler(void)
{
	/*pr_debug("scp_ipi_handler %d\n", scp_rcv_obj->id);*/

	if (scp_ipi_desc[scp_rcv_obj->id].handler) {
		memcpy_from_scp(scp_recv_buff, (void *)scp_rcv_obj->share_buf, scp_rcv_obj->len);
		scp_ipi_desc[scp_rcv_obj->id].handler(scp_rcv_obj->id, scp_recv_buff, scp_rcv_obj->len);
	}
	/*pr_debug("spm_sw_rsv_3:0x%x\n", *(volatile unsigned int*)SPM_SW_RSV_3);*/
	if ((*(volatile unsigned int*)SPM_SW_RSV_3 & 0xBABE0000) == 0xBABE0000) {
		/* clear mailbox when it's a wakeup event from scp ipi
		 * SPM_SW_RSV_3[31:16] is 0xBABE -> scp wakeup event
		 * */
		*(volatile unsigned int*)SPM_SW_RSV_3 &= 0x0000FFFF;
		SCP_TO_SPM_REG = 0x0;
	}
	/*pr_debug("scp_ipi_handler done\n");*/
}

/*
 * ipi initialize
 */
void scp_ipi_init(void)
{
	mutex_init(&scp_ipi_mutex);
	scp_rcv_obj = SCP_SHARE_BUFFER;
	scp_send_obj = scp_rcv_obj + 1;
	pr_debug("scp_rcv_obj = 0x%p\n", scp_rcv_obj);
	pr_debug("scp_send_obj = 0x%p\n", scp_send_obj);
	memset_io(scp_send_obj, 0, SHARE_BUF_SIZE);

}

/*
 * API let apps can register an ipi handler to receive IPI
 * @param id:       IPI ID
 * @param handler:  IPI handler
 * @param name:     IPI name
 */
ipi_status scp_ipi_registration(ipi_id id, ipi_handler_t handler, const char *name)
{
	if (id < SCP_NR_IPI) {
		scp_ipi_desc[id].name = name;

		if (handler == NULL)
			return ERROR;

		scp_ipi_desc[id].handler = handler;
		return DONE;
	} else {
		return ERROR;
	}
}
EXPORT_SYMBOL_GPL(scp_ipi_registration);

/*
 * API for apps to send an IPI to scp
 * @param id:   IPI ID
 * @param buf:  the pointer of data
 * @param len:  data length
 * @param wait: If true, wait (atomically) until data have been gotten by Host
 */
ipi_status scp_ipi_send(ipi_id id, void *buf, unsigned int  len, unsigned int wait)
{
	if (is_scp_ready() == 0) {
		pr_err("scp_ipi_send: SCP not enabled\n");
		return ERROR;
	}

	if (in_interrupt()) {
		if (wait) {
			pr_err("scp_ipi_send: cannot wait in isr context\n");
			return ERROR;
		}
	}

	/* SRAM protection need to bypass ADB wake up cmd */
	if (SCP_SLEEP_DEBUG_REG & 0xE && id != IPI_DVFS_WAKE) {
		/* bit:
		 * [4]IN_ACTIVE
		 * [3]ENTERING_ACTIVE
		 * [2]IN_SLEEP
		 * [1]ENTERING_SLEEP
		 * [0]IN_DEBUG_IDLE
		 * */
		pr_err("scp_ipi_send: scp state = %x\n", SCP_SLEEP_DEBUG_REG);
		pr_err("scp_ipi_send: scp is not in [4]IN_ACTIVE or [0]IN_DEBUG_IDLE, it should not access scp\n");
		return ERROR;
	}

	if (id < SCP_NR_IPI) {
		pr_debug("scp_ipi_send: id = %d\n", id);
		if (len > sizeof(scp_send_obj->share_buf) || buf == NULL) {
			pr_err("scp_ipi_send: buffer is error\n");
			return ERROR;
		}

		if (mutex_trylock(&scp_ipi_mutex) == 0) {
			pr_debug("scp_ipi_send: host to scp busy\n");
			return BUSY;
		}

		if (HOST_TO_SCP_REG) {
			mutex_unlock(&scp_ipi_mutex);
			pr_debug("scp_ipi_send: host to scp busy with mutex_unlock\n");
			return BUSY;
		}

		memcpy(scp_send_buff, buf, len);

		pr_debug("scp_ipi_send: memory copy to scp sram\n");
		memcpy_to_scp((void *)scp_send_obj->share_buf, scp_send_buff, len);
		scp_send_obj->len = len;
		scp_send_obj->id = id;
		dsb(SY);

		pr_debug("scp_ipi_send: send host to scp ipi\n");
		ipi_host2scp();

		if (wait)
			while (HOST_TO_SCP_REG)
				;

		mutex_unlock(&scp_ipi_mutex);
	} else {
		pr_err("scp_ipi_send: id is incorrect\n");
		return ERROR;
	}

	pr_debug("scp_ipi_send: id = %d done\n", id);
	return DONE;
}
EXPORT_SYMBOL_GPL(scp_ipi_send);
