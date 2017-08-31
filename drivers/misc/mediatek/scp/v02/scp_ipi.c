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
#include "scp_excep.h"

#define PRINT_THRESHOLD 10000
ipi_id scp_ipi_id_record;
ipi_id scp_ipi_mutex_owner[SCP_CORE_TOTAL];
ipi_id scp_ipi_owner[SCP_CORE_TOTAL];

unsigned int scp_ipi_id_record_count;

struct scp_ipi_desc scp_ipi_desc[SCP_NR_IPI];
struct share_obj *scp_send_obj[SCP_CORE_TOTAL];
struct share_obj *scp_rcv_obj[SCP_CORE_TOTAL];
struct mutex scp_ipi_mutex[SCP_CORE_TOTAL];
/*
 * find an ipi handler and invoke it
 */
void scp_A_ipi_handler(void)
{
	/*pr_debug("scp A ipi handler %d\n", scp_rcv_obj[SCP_A_ID]->id);*/
	if (scp_rcv_obj[SCP_A_ID]->id >= SCP_NR_IPI || scp_rcv_obj[SCP_A_ID]->id <= 0) {
		/* ipi id abnormal*/
		pr_err("[SCP] A ipi handler id abnormal, id = %d\n", scp_rcv_obj[SCP_A_ID]->id);
	} else if (scp_ipi_desc[scp_rcv_obj[SCP_A_ID]->id].handler) {
		memcpy_from_scp(scp_recv_buff[SCP_A_ID], (void *)scp_rcv_obj[SCP_A_ID]->share_buf,
			scp_rcv_obj[SCP_A_ID]->len);
		scp_ipi_desc[scp_rcv_obj[SCP_A_ID]->id].handler(scp_rcv_obj[SCP_A_ID]->id, scp_recv_buff[SCP_A_ID],
			scp_rcv_obj[SCP_A_ID]->len);
		/* After SCP IPI handler,
		 * send a awake ipi to avoid
		 * SCP keeping in ipi busy idle state
		 */
		/* WE1: set a direct IPI to awake SCP */
		writel((1 << SCP_A_IPI_AWAKE_NUM), SCP_GIPC_REG);
	} else {
		/* scp_ipi_handler is null or ipi id abnormal */
		pr_err("[SCP] A ipi handler is null or abnormal, id = %d\n", scp_rcv_obj[SCP_A_ID]->id);
	}
	/* AP side write 1 to clear SCP to SPM reg.
	 * scp side write 1 to set SCP to SPM reg.
	 * scp set      bit[0]
	 * scp dual set bit[1]
	 */
	SCP_TO_SPM_REG = 0x1;

	/*pr_debug("scp_ipi_handler done\n");*/
}

/*
 * ipi initialize
 */
void scp_A_ipi_init(void)
{
	mutex_init(&scp_ipi_mutex[SCP_A_ID]);
	scp_rcv_obj[SCP_A_ID] = SCP_A_SHARE_BUFFER;
	scp_send_obj[SCP_A_ID] = scp_rcv_obj[SCP_A_ID] + 1;
	pr_debug("scp_rcv_obj[SCP_A_ID] = 0x%p\n", scp_rcv_obj[SCP_A_ID]);
	pr_debug("scp_send_obj[SCP_A_ID] = 0x%p\n", scp_send_obj[SCP_A_ID]);
	memset_io(scp_send_obj[SCP_A_ID], 0, SHARE_BUF_SIZE);
}


/*
 * find an ipi handler and invoke it
 */
void scp_B_ipi_handler(void)
{
	/*pr_debug("scp B ipi handler %d\n", scp_rcv_obj[SCP_B_ID]->id);*/
	if (scp_rcv_obj[SCP_B_ID]->id >= SCP_NR_IPI || scp_rcv_obj[SCP_B_ID]->id <= 0) {
		/* ipi id abnormal*/
		pr_err("[SCP] B ipi handler id abnormal, id = %d\n", scp_rcv_obj[SCP_B_ID]->id);
	} else if (scp_ipi_desc[scp_rcv_obj[SCP_B_ID]->id].handler) {
		memcpy_from_scp(scp_recv_buff[SCP_B_ID], (void *)scp_rcv_obj[SCP_B_ID]->share_buf,
			scp_rcv_obj[SCP_B_ID]->len);
		scp_ipi_desc[scp_rcv_obj[SCP_B_ID]->id].handler(scp_rcv_obj[SCP_B_ID]->id, scp_recv_buff[SCP_B_ID],
			scp_rcv_obj[SCP_B_ID]->len);
		/* After SCP IPI handler,
		 * send a awake ipi to avoid
		 * SCP keeping in ipi busy idle state
		 */
		/* WE1: set a direct IPI to awake SCP */
		writel((1 << SCP_B_IPI_AWAKE_NUM), SCP_GIPC_REG);
	} else {
		/* scp_ipi_handler is null or ipi id abnormal */
		pr_err("[SCP] B ipi handler is null or abnormal, id = %d\n", scp_rcv_obj[SCP_B_ID]->id);
	}
	/* AP side write 1 to clear SCP to SPM reg.
	 * scp side write 1 to set SCP to SPM reg.
	 * scp set      bit[0]
	 * scp dual set bit[1]
	 */
	SCP_TO_SPM_REG = 0x2;

	/*pr_debug("scp_ipi_handler done\n");*/
}

/*
 * ipi initialize
 */
void scp_B_ipi_init(void)
{
	mutex_init(&scp_ipi_mutex[SCP_B_ID]);
	scp_rcv_obj[SCP_B_ID] = SCP_B_SHARE_BUFFER;
	scp_send_obj[SCP_B_ID] = scp_rcv_obj[SCP_B_ID] + 1;
	pr_debug("scp_rcv_obj[SCP_B_ID] = 0x%p\n", scp_rcv_obj[SCP_B_ID]);
	pr_debug("scp_send_obj[SCP_B_ID] = 0x%p\n", scp_send_obj[SCP_B_ID]);
	memset_io(scp_send_obj[SCP_B_ID], 0, SHARE_BUF_SIZE);
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
 * API let apps unregister an ipi handler
 * @param id:       IPI ID
 */
ipi_status scp_ipi_unregistration(ipi_id id)
{
	if (id < SCP_NR_IPI) {
		scp_ipi_desc[id].name = "";
		scp_ipi_desc[id].handler = NULL;
		return DONE;
	} else {
		return ERROR;
	}
}
EXPORT_SYMBOL_GPL(scp_ipi_unregistration);

/*
 * API for apps to send an IPI to scp
 * @param id:   IPI ID
 * @param buf:  the pointer of data
 * @param len:  data length
 * @param wait: If true, wait (atomically) until data have been gotten by Host
 * @param len:  data length
 */
ipi_status scp_ipi_send(ipi_id id, void *buf, unsigned int  len, unsigned int wait, scp_core_id scp_id)
{
	/*avoid scp log print too much*/
	if (scp_ipi_id_record == id)
		scp_ipi_id_record_count++;
	else
		scp_ipi_id_record_count = 0;

	scp_ipi_id_record = id;

	if (scp_id >= SCP_CORE_TOTAL) {
		pr_err("scp_ipi_send: scp_id:%d wrong\n", scp_id);
		return ERROR;
	}

	if (in_interrupt()) {
		if (wait) {
			pr_err("scp_ipi_send: cannot use in isr\n");
			return ERROR;
		}
	}

	if (id >= SCP_NR_IPI) {
		pr_err("scp_ipi_send: ipi id %d wrong\n", id);
		return ERROR;
	}
	if (is_scp_ready(scp_id) == 0) {
		pr_err("scp_ipi_send: %s not enabled, id=%d\n", core_ids[scp_id], id);
		return ERROR;
	}
	if (len > sizeof(scp_send_obj[scp_id]->share_buf) || buf == NULL) {
		pr_err("scp_ipi_send: %s buffer error\n", core_ids[scp_id]);
		return ERROR;
	}

	if (mutex_trylock(&scp_ipi_mutex[scp_id]) == 0) {
		/*avoid scp ipi send log print too much*/
		if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
			(scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
			pr_err("scp_ipi_send:%s %d mutex_trylock busy, owner=%d\n",
				core_ids[scp_id], id, scp_ipi_mutex_owner[scp_id]);
		}
		return BUSY;
	}

	/* keep scp awake for sram copy*/
	if (scp_awake_lock(scp_id) == -1) {
		mutex_unlock(&scp_ipi_mutex[scp_id]);
		pr_err("scp_ipi_send: %s ipi error, awake scp fail\n", core_ids[scp_id]);
		return ERROR;
	}

	/*get scp ipi mutex owner*/
	scp_ipi_mutex_owner[scp_id] = id;

	if ((GIPC_TO_SCP_REG & (1<<scp_id)) > 0) {
		/*avoid scp ipi send log print too much*/
		if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
			(scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
			pr_err("scp_ipi_send: %s %d host to scp busy, ipi last time = %d\n", core_ids[scp_id], id,
				scp_ipi_owner[scp_id]);
			if (scp_id == SCP_A_ID)
				scp_A_dump_regs();
			else
				scp_B_dump_regs();
		}
		scp_awake_unlock(scp_id);
		mutex_unlock(&scp_ipi_mutex[scp_id]);
		return BUSY;
	}
	/*get scp ipi send owner*/
	scp_ipi_owner[scp_id] = id;

	memcpy(scp_send_buff[scp_id], buf, len);
	memcpy_to_scp((void *)scp_send_obj[scp_id]->share_buf, scp_send_buff[scp_id], len);
	scp_send_obj[scp_id]->len = len;
	scp_send_obj[scp_id]->id = id;
	dsb(SY);

	/*send host to scp ipi*/
	/*pr_debug("scp_ipi_send: SCP A send host to scp ipi\n");*/
	GIPC_TO_SCP_REG = (1<<scp_id);

	if (wait)
		while ((GIPC_TO_SCP_REG & (1<<scp_id)) > 0)
			;
	/*send host to scp ipi cpmplete, unlock mutex*/
	scp_awake_unlock(scp_id);
	mutex_unlock(&scp_ipi_mutex[scp_id]);

	return DONE;
}
EXPORT_SYMBOL_GPL(scp_ipi_send);

