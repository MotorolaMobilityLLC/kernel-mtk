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
ipi_id scp_A_ipi_mutex_owner;
ipi_id scp_B_ipi_mutex_owner;
ipi_id scp_A_ipi_owner;
ipi_id scp_B_ipi_owner;
unsigned int scp_ipi_id_record_count;

struct scp_ipi_desc scp_ipi_desc[SCP_NR_IPI];
struct share_obj *scp_A_send_obj, *scp_A_rcv_obj;
struct share_obj *scp_B_send_obj, *scp_B_rcv_obj;
struct mutex scp_A_ipi_mutex, scp_B_ipi_mutex;

/*
 * find an ipi handler and invoke it
 */
void scp_A_ipi_handler(void)
{
	/*pr_debug("scp A ipi handler %d\n", scp_A_rcv_obj->id);*/
	if (scp_A_rcv_obj->id >= SCP_NR_IPI || scp_A_rcv_obj->id <= 0) {
		/* ipi id abnormal*/
		pr_err("[SCP] scp A ipi handler id abnormal, id = %d\n", scp_A_rcv_obj->id);
	} else if (scp_ipi_desc[scp_A_rcv_obj->id].handler) {
		memcpy_from_scp(scp_A_recv_buff, (void *)scp_A_rcv_obj->share_buf, scp_A_rcv_obj->len);
		scp_ipi_desc[scp_A_rcv_obj->id].handler(scp_A_rcv_obj->id, scp_A_recv_buff, scp_A_rcv_obj->len);
		/* After SCP IPI handler,
		 * send a awake ipi to avoid
		 * SCP keeping in ipi busy idle state
		 */
		/* WE1: set a direct IPI to awake SCP */
		writel((1 << SCP_A_IPI_AWAKE_NUM), SCP_GIPC_REG);
	} else {
		/* scp_ipi_handler is null or ipi id abnormal */
		pr_err("[SCP] scp A ipi handler is null or abnormal, id = %d\n", scp_A_rcv_obj->id);
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
	mutex_init(&scp_A_ipi_mutex);
	scp_A_rcv_obj = SCP_A_SHARE_BUFFER;
	scp_A_send_obj = scp_A_rcv_obj + 1;
	pr_debug("SCP A scp_A_rcv_obj = 0x%p\n", scp_A_rcv_obj);
	pr_debug("SCP A scp_A_send_obj = 0x%p\n", scp_A_send_obj);
	memset_io(scp_A_send_obj, 0, SHARE_BUF_SIZE);
}


/*
 * find an ipi handler and invoke it
 */
void scp_B_ipi_handler(void)
{
	/*pr_debug("scp B ipi handler %d\n", scp_B_rcv_obj->id);*/
	if (scp_B_rcv_obj->id >= SCP_NR_IPI || scp_B_rcv_obj->id <= 0) {
		/* ipi id abnormal*/
		pr_err("[SCP] scp B ipi handler id abnormal, id = %d\n", scp_B_rcv_obj->id);
	} else if (scp_ipi_desc[scp_B_rcv_obj->id].handler) {
		memcpy_from_scp(scp_B_recv_buff, (void *)scp_B_rcv_obj->share_buf, scp_B_rcv_obj->len);
		scp_ipi_desc[scp_B_rcv_obj->id].handler(scp_B_rcv_obj->id, scp_B_recv_buff, scp_B_rcv_obj->len);
		/* After SCP IPI handler,
		 * send a awake ipi to avoid
		 * SCP keeping in ipi busy idle state
		 */
		/* WE1: set a direct IPI to awake SCP */
		writel((1 << SCP_B_IPI_AWAKE_NUM), SCP_GIPC_REG);
	} else {
		/* scp_ipi_handler is null or ipi id abnormal */
		pr_err("[SCP] scp B ipi handler is null or abnormal, id = %d\n", scp_B_rcv_obj->id);
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
	mutex_init(&scp_B_ipi_mutex);
	scp_B_rcv_obj = SCP_B_SHARE_BUFFER;
	scp_B_send_obj = scp_B_rcv_obj + 1;
	pr_debug("SCP B scp_B_rcv_obj = 0x%p\n", scp_B_rcv_obj);
	pr_debug("SCP B scp_B_send_obj = 0x%p\n", scp_B_send_obj);
	memset_io(scp_B_send_obj, 0, SHARE_BUF_SIZE);
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
		pr_err("scp_ipi_send: SCP ID >= SCP_CORE_TOTAL\n");
		return ERROR;
	}

	if (in_interrupt()) {
		if (wait) {
			pr_err("scp_ipi_send: cannot busy wait in isr context\n");
			return ERROR;
		}
	}

	if (scp_id == SCP_A_ID) {
		/*Send IPI to SCP A */
		if (is_scp_ready(SCP_A_ID) == 0) {
			pr_err("scp_ipi_send: SCP A not enabled, id=%d\n", id);
			return ERROR;
		}

		if (id >= SCP_NR_IPI) {
			pr_err("scp_ipi_send: SCP A ipi id is incorrect\n");
			return ERROR;
		}

		/*pr_debug("scp_ipi_send: SCP A ipi id = %d\n", id);*/
		if (len > sizeof(scp_A_send_obj->share_buf) || buf == NULL) {
			pr_err("scp_ipi_send: SCP A buffer is error\n");
			return ERROR;
		}

		if (mutex_trylock(&scp_A_ipi_mutex) == 0) {
			/*avoid scp ipi send log print too much*/
			if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
				(scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
				pr_err("scp_ipi_send:SCP A %d mutex_trylock busy,mutex owner=%d\n",
					id, scp_A_ipi_mutex_owner);
				scp_A_dump_regs();
			}
			return BUSY;
		}

		/* keep scp awake for sram copy*/
		if (scp_awake_lock(SCP_A_ID) == -1) {
			mutex_unlock(&scp_A_ipi_mutex);
			pr_err("scp_ipi_send: SCP A ipi error, awake scp fail\n");
			return ERROR;
		}

		/*get scp ipi mutex owner*/
		scp_A_ipi_mutex_owner = id;

		if ((GIPC_TO_SCP_REG & HOST_TO_SCP_A) > 0) {
			scp_awake_unlock(SCP_A_ID);
			mutex_unlock(&scp_A_ipi_mutex);
			/*avoid scp ipi send log print too much*/
			if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
				(scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
				pr_err("scp_ipi_send: SCP A %d host to scp busy, ipi last time = %d\n", id,
					scp_A_ipi_owner);
				scp_A_dump_regs();
			}
			return BUSY;
		}
		/*get scp ipi send owner*/
		scp_A_ipi_owner = id;

		memcpy(scp_A_send_buff, buf, len);
		/*pr_debug("scp_ipi_send: SCP A memory copy to scp sram\n");*/
		memcpy_to_scp((void *)scp_A_send_obj->share_buf, scp_A_send_buff, len);
		scp_A_send_obj->len = len;
		scp_A_send_obj->id = id;
		dsb(SY);

		/*send host to scp ipi*/
		/*pr_debug("scp_ipi_send: SCP A send host to scp ipi\n");*/
		GIPC_TO_SCP_REG = HOST_TO_SCP_A;

		if (wait)
			while ((GIPC_TO_SCP_REG & HOST_TO_SCP_A) > 0)
				;
		/*send host to scp ipi cpmplete, unlock mutex*/
		scp_awake_unlock(SCP_A_ID);
		mutex_unlock(&scp_A_ipi_mutex);

		/*pr_debug("scp_ipi_send: SCP A ipi send id = %d done\n", id);*/

	} else {
		/*Send IPI to SCP B*/
		if (is_scp_ready(SCP_B_ID) == 0) {
			pr_err("scp_ipi_send: SCP B not enabled\n");
			return ERROR;
		}

		if (id >= SCP_NR_IPI) {
			pr_err("scp_ipi_send: SCP B ipi id is incorrect\n");
			return ERROR;
		}

		/*pr_debug("scp_ipi_send: SCP B ipi id = %d\n", id);*/
		if (len > sizeof(scp_B_send_obj->share_buf) || buf == NULL) {
			pr_err("scp_ipi_send: SCP B buffer is error\n");
			return ERROR;
		}

		if (mutex_trylock(&scp_B_ipi_mutex) == 0) {
			/*avoid scp ipi send log print too much*/
			if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
				(scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
				pr_err("scp_ipi_send: SCP B %d mutex_trylock busy, mutex owner = %d\n",
					id, scp_B_ipi_mutex_owner);
				scp_B_dump_regs();
			}
			return BUSY;
		}

		/* keep scp awake for sram copy*/
		if (scp_awake_lock(SCP_B_ID) == -1) {
			mutex_unlock(&scp_B_ipi_mutex);
			pr_err("scp_ipi_send: SCP B ipi error, awake scp fail\n");
			return ERROR;
		}

		/*get scp ipi mutex owner
		 */
		scp_B_ipi_mutex_owner = id;

		if ((GIPC_TO_SCP_REG & HOST_TO_SCP_A) > 0) {
			scp_awake_unlock(SCP_B_ID);
			mutex_unlock(&scp_B_ipi_mutex);
			/*avoid scp ipi send log print too much*/
			if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
				(scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
				pr_err("scp_ipi_send: SCP B %d host to scp busy, ipi last time = %d\n", id,
					scp_B_ipi_owner);
				scp_B_dump_regs();
			}
			return BUSY;
		}
		/*get scp ipi send owner*/
		scp_B_ipi_owner = id;

		memcpy(scp_B_send_buff, buf, len);
		/*pr_debug("scp_ipi_send: SCP B memory copy to scp sram\n");*/
		memcpy_to_scp((void *)scp_B_send_obj->share_buf, scp_B_send_buff, len);
		scp_B_send_obj->len = len;
		scp_B_send_obj->id = id;
		dsb(SY);

		/*send host to scp ipi*/
		/*pr_debug("scp_ipi_send: SCP B send host to scp ipi\n");*/
		GIPC_TO_SCP_REG = HOST_TO_SCP_B;

		if (wait)
			while ((GIPC_TO_SCP_REG & HOST_TO_SCP_B) > 0)
				;
		/*send host to scp ipi cpmplete, unlock mutex*/
		scp_awake_unlock(SCP_B_ID);
		mutex_unlock(&scp_B_ipi_mutex);

		/*pr_debug("scp_ipi_send: SCP B ipi send id = %d done\n", id);*/

	}
	return DONE;
}
EXPORT_SYMBOL_GPL(scp_ipi_send);
