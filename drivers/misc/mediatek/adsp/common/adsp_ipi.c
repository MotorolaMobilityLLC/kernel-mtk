/*
 * Copyright (C) 2011-2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/mutex.h>
#include <mt-plat/sync_write.h>

#include <audio_ipi_platform.h>

#include "adsp_ipi.h"
#include "adsp_helper.h"
#include "adsp_excep.h"

#include <adsp_ipi_queue.h>


#define PRINT_THRESHOLD 10000
enum adsp_ipi_id adsp_ipi_id_record;
enum adsp_ipi_id adsp_ipi_mutex_owner[ADSP_CORE_TOTAL];
enum adsp_ipi_id adsp_ipi_owner[ADSP_CORE_TOTAL];

unsigned int adsp_ipi_id_record_count;
unsigned int adsp_to_ap_ipi_count;
unsigned int ap_to_adsp_ipi_count;

struct adsp_ipi_desc adsp_ipi_desc[ADSP_NR_IPI];
struct adsp_share_obj *adsp_send_obj[ADSP_CORE_TOTAL];
struct adsp_share_obj *adsp_rcv_obj[ADSP_CORE_TOTAL];
struct mutex adsp_ipi_mutex[ADSP_CORE_TOTAL];
/*
 * find an ipi handler and invoke it
 */
void adsp_A_ipi_handler(void)
{
#if ADSP_IPI_STAMP_SUPPORT
	unsigned int flag = 0;
#endif
	enum adsp_ipi_id adsp_id;

	adsp_id = adsp_rcv_obj[ADSP_A_ID]->id;
	/*pr_debug("adsp A ipi handler %d\n", adsp_id);*/
	if (adsp_id >= ADSP_NR_IPI || adsp_id <= 0) {
		/* ipi id abnormal*/
		pr_debug("[ADSP] A ipi handler id abnormal, id=%d\n", adsp_id);
	} else if (adsp_ipi_desc[adsp_id].handler) {
		memcpy_from_adsp(adsp_recv_buff[ADSP_A_ID],
				 (void *)adsp_rcv_obj[ADSP_A_ID]->share_buf,
				 adsp_rcv_obj[ADSP_A_ID]->len);

		adsp_ipi_desc[adsp_id].recv_count++;
		adsp_to_ap_ipi_count++;
#if ADSP_IPI_STAMP_SUPPORT
		flag = adsp_ipi_desc[adsp_id].recv_count %
		       ADSP_IPI_ID_STAMP_SIZE;
		if (flag < ADSP_IPI_ID_STAMP_SIZE) {
			adsp_ipi_desc[adsp_id].recv_flag[flag] =
				adsp_ipi_desc[adsp_id].recv_count;
			adsp_ipi_desc[adsp_id].handler_timestamp[flag] = 0;
			adsp_ipi_desc[adsp_id].recv_timestamp[flag] =
				arch_counter_get_cntvct();
		}
#endif
		scp_dispatch_ipi_hanlder_to_queue(
			AUDIO_OPENDSP_USE_HIFI3,
			adsp_id, /* the adsp_id here means ipi_id ... = =a */
			adsp_recv_buff[ADSP_A_ID],
			adsp_rcv_obj[ADSP_A_ID]->len,
			adsp_ipi_desc[adsp_id].handler);
#if ADSP_IPI_STAMP_SUPPORT
		if (flag < ADSP_IPI_ID_STAMP_SIZE)
			adsp_ipi_desc[adsp_id].handler_timestamp[flag] =
				arch_counter_get_cntvct();
#endif
	} else {
		/* adsp_ipi_handler is null or ipi id abnormal */
		pr_debug("[ADSP] A ipi handler is null or abnormal, id=%d\n",
			 adsp_id);
	}
	/* ADSP side write 1 to assert SPM wakeup src,
	 * while AP side write 0 to clear wakeup src.
	 */
	writel(0x0, ADSP_TO_SPM_REG);

	/*pr_debug("adsp_ipi_handler done\n");*/
}

/*
 * ipi initialize
 */
void adsp_A_ipi_init(void)
{
	int i = 0;
#if ADSP_IPI_STAMP_SUPPORT
	int j = 0;
#endif

	mutex_init(&adsp_ipi_mutex[ADSP_A_ID]);
	adsp_rcv_obj[ADSP_A_ID] = ADSP_A_IPC_BUFFER;
	adsp_send_obj[ADSP_A_ID] = adsp_rcv_obj[ADSP_A_ID] + 1;
	pr_debug("adsp_rcv_obj[ADSP_A_ID] = 0x%p\n", adsp_rcv_obj[ADSP_A_ID]);
	pr_debug("adsp_send_obj[ADSP_A_ID] = 0x%p\n", adsp_send_obj[ADSP_A_ID]);

	adsp_to_ap_ipi_count = 0;
	ap_to_adsp_ipi_count = 0;

	for (i = 0; i < ADSP_NR_IPI; i++) {
		adsp_ipi_desc[i].recv_count      = 0;
		adsp_ipi_desc[i].success_count  = 0;
		adsp_ipi_desc[i].busy_count      = 0;
		adsp_ipi_desc[i].error_count    = 0;
#if ADSP_IPI_STAMP_SUPPORT
		for (j = 0; j < ADSP_IPI_ID_STAMP_SIZE; j++) {
			adsp_ipi_desc[i].recv_timestamp[j] = 0;
			adsp_ipi_desc[i].send_timestamp[j] = 0;
			adsp_ipi_desc[i].recv_flag[j] = 0;
			adsp_ipi_desc[i].send_flag[j] = 0;
			adsp_ipi_desc[i].handler_timestamp[j] = 0;
		}
#endif
	}
}


/*
 * API let apps can register an ipi handler to receive IPI
 * @param id:      IPI ID
 * @param handler:  IPI handler
 * @param name:  IPI name
 */
enum adsp_ipi_status adsp_ipi_registration(
	enum adsp_ipi_id id,
	void (*ipi_handler)(int id, void *data, unsigned int len),
	const char *name)
{
	if (id < ADSP_NR_IPI) {
		adsp_ipi_desc[id].name = name;

		if (ipi_handler == NULL)
			return ADSP_IPI_ERROR;

		adsp_ipi_desc[id].handler = ipi_handler;
		return ADSP_IPI_DONE;
	} else
		return ADSP_IPI_ERROR;
}
EXPORT_SYMBOL_GPL(adsp_ipi_registration);

/*
 * API let apps unregister an ipi handler
 * @param id:      IPI ID
 */
enum adsp_ipi_status adsp_ipi_unregistration(enum adsp_ipi_id id)
{
	if (id < ADSP_NR_IPI) {
		adsp_ipi_desc[id].name = "";
		adsp_ipi_desc[id].handler = NULL;
		return ADSP_IPI_DONE;
	} else
		return ADSP_IPI_ERROR;
}
EXPORT_SYMBOL_GPL(adsp_ipi_unregistration);

/*
 * API for apps to send an IPI to adsp
 * @param id:   IPI ID
 * @param buf:  the pointer of data
 * @param len:  data length
 * @param wait: If true, wait (atomically) until data have been gotten by Host
 * @param len:  data length
 */
enum adsp_ipi_status adsp_ipi_send(enum adsp_ipi_id id, void *buf,
				   unsigned int  len, unsigned int wait,
				   enum adsp_core_id adsp_id)
{
	int retval = 0;

	/* wait at most 5 ms until IPC done */
	retval = scp_send_msg_to_queue(
			 AUDIO_OPENDSP_USE_HIFI3, id, buf, len, (wait) ? 5 : 0);

	return (retval == 0) ? ADSP_IPI_DONE : ADSP_IPI_ERROR;
}


enum adsp_ipi_status adsp_ipi_send_ipc(enum adsp_ipi_id id, void *buf,
				       unsigned int  len, unsigned int wait,
				       enum adsp_core_id adsp_id)
{

#if ADSP_IPI_STAMP_SUPPORT
	unsigned long flag = 0;
#endif

	/*avoid adsp log print too much*/
	if (adsp_ipi_id_record == id)
		adsp_ipi_id_record_count++;
	else
		adsp_ipi_id_record_count = 0;

	adsp_ipi_id_record = id;

	if (in_interrupt()) {
		if (wait) {
			pr_debug("adsp_ipi_send: cannot use in isr\n");
			adsp_ipi_desc[id].error_count++;
			return ADSP_IPI_ERROR;
		}
	}

	if (is_adsp_ready(adsp_id) == 0) {
		pr_warn("adsp_ipi_send: %s not enabled, id=%d\n",
		       adsp_core_ids[adsp_id], id);
		adsp_ipi_desc[id].error_count++;
		return ADSP_IPI_ERROR;
	}
	if (len > sizeof(adsp_send_obj[adsp_id]->share_buf) || buf == NULL) {
		pr_debug("adsp_ipi_send: %s buffer error\n",
		       adsp_core_ids[adsp_id]);
		adsp_ipi_desc[id].error_count++;
		return ADSP_IPI_ERROR;
	}

	if (mutex_trylock(&adsp_ipi_mutex[adsp_id]) == 0) {
		/*avoid adsp ipi send log print too much*/
		if ((adsp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
		    (adsp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
			pr_debug("adsp_ipi_send:%s %d mutex_trylock busy,owner=%d\n",
			       adsp_core_ids[adsp_id], id,
			       adsp_ipi_mutex_owner[adsp_id]);
		}
		adsp_ipi_desc[id].busy_count++;
		return ADSP_IPI_BUSY;
	}

	/*get adsp ipi mutex owner*/
	adsp_ipi_mutex_owner[adsp_id] = id;

	if ((readl(ADSP_SWINT_REG) & (1 << adsp_id)) > 0) {
		/*avoid adsp ipi send log print too much*/
		if ((adsp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
		    (adsp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
			pr_debug("adsp_ipi_send: %s %d host to adsp busy, ipi last time = %d\n",
			       adsp_core_ids[adsp_id], id,
			       adsp_ipi_owner[adsp_id]);
		}

		adsp_ipi_desc[id].busy_count++;
		mutex_unlock(&adsp_ipi_mutex[adsp_id]);
		return ADSP_IPI_BUSY;
	}
	/*get adsp ipi send owner*/
	adsp_ipi_owner[adsp_id] = id;

	memcpy(adsp_send_buff[adsp_id], buf, len);
	memcpy_to_adsp((void *)adsp_send_obj[adsp_id]->share_buf,
		       adsp_send_buff[adsp_id], len);
	adsp_send_obj[adsp_id]->len = len;
	adsp_send_obj[adsp_id]->id = id;
	dsb(SY);
	/*record timestamp*/
	adsp_ipi_desc[id].success_count++;
	ap_to_adsp_ipi_count++;

#if ADSP_IPI_STAMP_SUPPORT
	flag = adsp_ipi_desc[id].success_count % ADSP_IPI_ID_STAMP_SIZE;
	if (flag < ADSP_IPI_ID_STAMP_SIZE) {
		adsp_ipi_desc[id].send_flag[flag] =
			adsp_ipi_desc[id].success_count;
		adsp_ipi_desc[id].send_timestamp[flag] =
			arch_counter_get_cntvct();
	}
#endif
	/*send host to adsp ipi*/
	/*pr_debug("adsp_ipi_send: ADSP A send host to adsp ipi\n");*/
	writel((1 << adsp_id), ADSP_SWINT_REG);

	if (wait)
		while ((readl(ADSP_SWINT_REG) & (1 << adsp_id)) > 0)
			;
	/*send host to adsp ipi cpmplete, unlock mutex*/
#ifdef Liang_Check
	if (adsp_awake_unlock(adsp_id) == -1)
		pr_debug("adsp_ipi_send: awake unlock fail\n");
#endif
	mutex_unlock(&adsp_ipi_mutex[adsp_id]);

	return ADSP_IPI_DONE;
}
EXPORT_SYMBOL_GPL(adsp_ipi_send);

void adsp_ipi_info_dump(enum adsp_ipi_id id)
{
	pr_debug("%u\t%u\t%u\t%u\t%u\t%s\n\r",
		 id,
		 adsp_ipi_desc[id].recv_count,
		 adsp_ipi_desc[id].success_count,
		 adsp_ipi_desc[id].busy_count,
		 adsp_ipi_desc[id].error_count,
		 adsp_ipi_desc[id].name);
#if ADSP_IPI_STAMP_SUPPORT
	/*time stamp*/
	for (i = 0; i < ADSP_IPI_ID_STAMP_SIZE; i++) {
		if (adsp_ipi_desc[id].recv_timestamp[i] != 0) {
			pr_debug("adsp->ap recv count:%u, recv time:%llu handler finish time:%llu\n",
				 adsp_ipi_desc[id].recv_flag[i],
				 adsp_ipi_desc[id].recv_timestamp[i],
				 adsp_ipi_desc[id].handler_timestamp[i]);
		}
	}
	for (i = 0; i < ADSP_IPI_ID_STAMP_SIZE; i++) {
		if (adsp_ipi_desc[id].send_timestamp[i] != 0) {
			pr_debug("ap->adsp send count:%u send time:%llu\n",
				 adsp_ipi_desc[id].send_flag[i],
				 adsp_ipi_desc[id].send_timestamp[i]);
		}
	}
#endif

}

void adsp_ipi_status_dump_id(enum adsp_ipi_id id)
{
#if ADSP_IPI_STAMP_SUPPORT
	/*time stamp*/
	unsigned int i;
#endif

	pr_debug("[ADSP]id\trecv\tsuccess\tbusy\terror\tname\n\r");
	adsp_ipi_info_dump(id);

}


void adsp_ipi_status_dump(void)
{
	enum adsp_ipi_id id;
#if ADSP_IPI_STAMP_SUPPORT
	/*time stamp*/
	unsigned int i;
#endif

	pr_debug("[ADSP]id\trecv\tsuccess\tbusy\terror\tname\n\r");
	for (id = 0; id < ADSP_NR_IPI; id++) {
		if (adsp_ipi_desc[id].recv_count > 0    ||
		    adsp_ipi_desc[id].success_count > 0 ||
		    adsp_ipi_desc[id].busy_count > 0    ||
		    adsp_ipi_desc[id].error_count > 0)
			adsp_ipi_info_dump(id);
	}
	pr_debug("ap->adsp total=%u adsp->ap total=%u\n\r",
		 ap_to_adsp_ipi_count, adsp_to_ap_ipi_count);
}
