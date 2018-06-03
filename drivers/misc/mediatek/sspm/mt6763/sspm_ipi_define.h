/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
#ifndef __SSPM_IPI_DEFINE_H__
#define __SSPM_IPI_DEFINE_H__

#define IPI_MBOX_TOTAL  3
#define IPI_MBOX0_64D   0
#define IPI_MBOX1_64D   0
#define IPI_MBOX2_64D   0
#define IPI_MBOX3_64D   0
#define IPI_MBOX4_64D   0
#define IPI_MBOX_MODE   ((IPI_MBOX4_64D<<4)|(IPI_MBOX3_64D<<3)|(IPI_MBOX2_64D<<2)|(IPI_MBOX1_64D<<1)|IPI_MBOX0_64D)

#define IPI_MBOX0_SLOTS ((IPI_MBOX0_64D+1)*32)
#define IPI_MBOX1_SLOTS ((IPI_MBOX1_64D+1)*32)
#define IPI_MBOX2_SLOTS ((IPI_MBOX2_64D+1)*32)
#define IPI_MBOX3_SLOTS ((IPI_MBOX3_64D+1)*32)
#define IPI_MBOX4_SLOTS ((IPI_MBOX4_64D+1)*32)


/* definition of slot size for received PINs */
#define PINS_SIZE_PLATFORM       3  /* the following will use mbox 0 */
#define PINS_SIZE_CPU_DVFS       4
#define PINS_SIZE_GPU_DVFS       3
#define PINS_SIZE_FHCTL          9
#define PINS_SIZE_PMIC           5
/* ============================================================ */
#define PINS_SIZE_MCDI           2  /* the following will use mbox 1 */
#define PINS_SIZE_SPM_SUSPEND    8
/* ============================================================ */


/* definition of slot offset for PINs */
#define PINS_OFFSET_PLATFORM     0  /* the following will use mbox 0 */
#define PINS_OFFSET_CPU_DVFS     (PINS_OFFSET_PLATFORM + PINS_SIZE_PLATFORM)
#define PINS_OFFSET_GPU_DVFS     (PINS_OFFSET_CPU_DVFS + PINS_SIZE_CPU_DVFS)
#define PINS_OFFSET_FHCTL        (PINS_OFFSET_GPU_DVFS + PINS_SIZE_GPU_DVFS)
#define PINS_OFFSET_PMIC         (PINS_OFFSET_FHCTL + PINS_SIZE_FHCTL)
#define PINS_MBOX0_USED          (PINS_OFFSET_PMIC + PINS_SIZE_PMIC)
#if (PINS_MBOX0_USED > IPI_MBOX0_SLOTS)
#error "MBOX0 cannot hold all pin definitions"
#endif
/* ============================================================ */
#define PINS_OFFSET_MCDI         0  /* the following will use mbox 1 */
#define PINS_OFFSET_SPM_SUSPEND  (PINS_OFFSET_MCDI + PINS_SIZE_MCDI)
#define PINS_MBOX1_USED          (PINS_OFFSET_SPM_SUSPEND + PINS_SIZE_SPM_SUSPEND)
#if (PINS_MBOX1_USED > IPI_MBOX1_SLOTS)
#error "MBOX1 cannot hold all pin definitions"
#endif
/* ============================================================ */

/* mutex_send, sema_ack, mbox, slot, size, shared, retdata, lock, share_grp, polling, unused */
struct _pin_send send_pintable[] = {
	{{{0} }, {0}, 0, PINS_OFFSET_PLATFORM, PINS_SIZE_PLATFORM, 0, 1, 0, 0, 0, 0}, /* platform */
	{{{0} }, {0}, 0, PINS_OFFSET_CPU_DVFS, PINS_SIZE_CPU_DVFS, 0, 1, 0, 0, 0, 0}, /* CPU DVFS */
	{{{0} }, {0}, 0, PINS_OFFSET_GPU_DVFS, PINS_SIZE_GPU_DVFS, 0, 1, 0, 0, 0, 0}, /* GPU DVFS */
	{{{0} }, {0}, 0, PINS_OFFSET_FHCTL,  PINS_SIZE_FHCTL, 0, 1, 0, 0, 0, 0},  /* FHCTL */
	{{{0} }, {0}, 0, PINS_OFFSET_PMIC,  PINS_SIZE_PMIC, 0, 1, 0, 0, 0, 0},  /* PMIC */
	/*====================================================================*/
	{{{0} }, {0}, 1, PINS_OFFSET_MCDI, PINS_SIZE_MCDI, 1, 1, 1, 1, 0, 0},  /* MCDI */
	{{{0} }, {0}, 1, PINS_OFFSET_SPM_SUSPEND, PINS_SIZE_SPM_SUSPEND, 1, 1, 1, 1, 0, 0},   /* SPM suspend */
/*====================================================================*/
};
#define TOTAL_SEND_PIN      (sizeof(send_pintable)/sizeof(struct _pin_send))

/* definition of slot size for received PINs */
#define PINR_SIZE_PLATFORM       3  /* the following will use mbox 2 */
#define PINR_SIZE_CPU_DVFS       4
#define PINR_SIZE_GPU_DVFS       3
/* definition of slot offset for PINs */
#define PINR_OFFSET_PLATFORM     0  /* the following will use mbox 0 */
#define PINR_OFFSET_CPU_DVFS     (PINR_OFFSET_PLATFORM + PINR_SIZE_PLATFORM)
#define PINR_OFFSET_GPU_DVFS     (PINR_OFFSET_CPU_DVFS + PINR_SIZE_CPU_DVFS)
#define PINR_MBOX2_USED          (PINR_OFFSET_GPU_DVFS + PINR_SIZE_GPU_DVFS)
#if (PINR_MBOX2_USED > IPI_MBOX2_SLOTS)
#error "MBOX2 cannot hold all pin definitions"
#endif

/* act, mbox, slot, size, shared, retdata, lock, share_grp, unused */
struct _pin_recv recv_pintable[] = {
	{NULL, 2, PINR_OFFSET_PLATFORM, PINR_SIZE_PLATFORM, 0, 1, 0, 0, 0},   /* platform */
	{NULL, 2, PINR_OFFSET_CPU_DVFS, PINR_SIZE_CPU_DVFS, 0, 1, 0, 0, 0},   /* CPU DVFS */
	{NULL, 2, PINR_OFFSET_GPU_DVFS, PINR_SIZE_GPU_DVFS, 0, 1, 0, 0, 0},   /* GPU DVFS */
};
#define TOTAL_RECV_PIN      (sizeof(recv_pintable)/sizeof(struct _pin_recv))

/* info for all mbox: start, end, used_slot, mode, unused */
struct _mbox_info mbox_table[IPI_MBOX_TOTAL] = {
	{0, 4, PINS_MBOX0_USED, 2, 0},  /* mbox 0 for send */
	{5, 6, PINS_MBOX1_USED, 2, 0}, /* mbox 1 for send */
	{0, 2, PINR_MBOX2_USED, 1, 0},  /* mbox 2 for recv */
};

static char *pin_name[IPI_ID_TOTAL] = {
	"PLATFORM",
	"CPU_DVFS",
	"GPU_DVFS",
	"FHCTL",
	"PMIC",
	"MCDI",
	"SPM_SUSPEND",
};

static inline int check_table_tag(int mcnt)
{
	int i, j = 0, k = 0, n = 0;
	uint32_t data, check;
	struct _pin_recv *rpin = NULL;
	struct _pin_send *spin = NULL;

	for (i = 0; i < mcnt; i++) {
		if (mbox_table[i].mode == 0)
			continue;

		/* Write init data into mailbox */
		j = mbox_table[i].start;
		if (mbox_table[i].mode == 1) {  /* for recev */
			rpin = &(recv_pintable[j]);
			data = 2;
		} else if (mbox_table[i].mode == 2) {  /* for send */
			spin = &(send_pintable[j]);
			data = 1;
		} else {
			pr_err("Error: mbox %d has unsupported mode=%d\n", i, mbox_table[i].mode);
			return -2;
		}

		data = ((data<<16)|('M'<<24));
		if (IPI_MBOX_MODE & (1 << i)) {
			/* 64 slots in the mailbox */
			data |= 0x00800000;
		}

		/* for each pin in the mbox */
		for (; j <= mbox_table[i].end; j++) {
			if (mbox_table[i].mode == 1) {  /* for recev */
				k = rpin->slot;
				n = rpin->size;
				rpin++;
			} else if (mbox_table[i].mode == 2) {  /* for send */
				k = spin->slot;
				n = spin->size;
				spin++;
			}

			/* for each slot in the pin */
			data &= ~0xFFFF;
			data |= (j << 8)|((uint8_t)(n));
			while (n) {
				sspm_mbox_read(i, k, &check, 1);
				if (check != data) {
					pr_err("Error: IPI Dismatch!! mbox:%d pin:%d slot=%d should be %08X but now %08X\n",
						   i, j, k, data, check);
					return -3;
				}
				k++;
				n--;
			}
		}
	}
	return 0;
}

#define sspm_ipi_lock_spm_scenario(start, id, opt, name)

#endif /* __SSPM_IPI_DEFINE_H__ */
