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

#define IPI_MBOX_TOTAL  5
#define IPI_MBOX0_64D   0
#define IPI_MBOX1_64D   1
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
#define PINS_SIZE_DCS            6  /* the following will use mbox 0 */
#define PINS_SIZE_MET            4
#define PINS_SIZE_PLATFORM       3
#define PINS_SIZE_PTPOD          4
#define PINS_SIZE_CPU_DVFS       4
#define PINS_SIZE_GPU_DVFS       3
#define PINS_SIZE_UNUSED1        0
#define PINS_SIZE_UNUSED2        0
/* ============================================================ */
#define PINS_SIZE_PMIC_WRAP      4  /* the following will use mbox 1 */
#define PINS_SIZE_CLOCK          3
#define PINS_SIZE_THERMAL        4
#define PINS_SIZE_DEEP_IDLE      8
#define PINS_SIZE_MCDI           8
#define PINS_SIZE_SODI           8
#define PINS_SIZE_SPM_SUSPEND    8
#define PINS_SIZE_FHCTL          9
#define PINS_SIZE_PMIC           5
#define PINS_SIZE_PPM            7
#define PINS_SIZE_OCP            1
#define PINS_SIZE_EXT_BUCK       1
/* ============================================================ */


/* definition of slot offset for PINs */
#define PINS_OFFSET_DCS          0  /* the following will use mbox 0 */
#define PINS_OFFSET_MET          (PINS_OFFSET_DCS + PINS_SIZE_DCS)
#define PINS_OFFSET_PLATFORM     (PINS_OFFSET_MET + PINS_SIZE_MET)
#define PINS_OFFSET_PTPOD        (PINS_OFFSET_PLATFORM + PINS_SIZE_PLATFORM)
#define PINS_OFFSET_CPU_DVFS     (PINS_OFFSET_PTPOD + PINS_SIZE_PTPOD)
#define PINS_OFFSET_GPU_DVFS     (PINS_OFFSET_CPU_DVFS + PINS_SIZE_CPU_DVFS)
#define PINS_OFFSET_UNUSED1      (PINS_OFFSET_GPU_DVFS + PINS_SIZE_GPU_DVFS)
#define PINS_OFFSET_UNUSED2      (PINS_OFFSET_UNUSED1 + PINS_SIZE_UNUSED1)
#define PINS_MBOX0_USED          (PINS_OFFSET_UNUSED2 + PINS_SIZE_UNUSED2)
#if (PINS_MBOX0_USED > IPI_MBOX0_SLOTS)
#error "MBOX0 cannot hold all pin definitions"
#endif
/* ============================================================ */
#define PINS_OFFSET_PMIC_WRAP    0  /* the following will use mbox 1 */
#define PINS_OFFSET_CLOCK        (PINS_OFFSET_PMIC_WRAP + PINS_SIZE_PMIC_WRAP)
#define PINS_OFFSET_THERMAL      (PINS_OFFSET_CLOCK + PINS_SIZE_CLOCK)
#define PINS_OFFSET_DEEP_IDLE    (PINS_OFFSET_THERMAL + PINS_SIZE_THERMAL)
#define PINS_OFFSET_MCDI         (PINS_OFFSET_THERMAL + PINS_SIZE_THERMAL)
#define PINS_OFFSET_SODI         (PINS_OFFSET_THERMAL + PINS_SIZE_THERMAL)
#define PINS_OFFSET_SPM_SUSPEND  (PINS_OFFSET_THERMAL + PINS_SIZE_THERMAL)
#define PINS_OFFSET_FHCTL        (PINS_OFFSET_SPM_SUSPEND + PINS_SIZE_SPM_SUSPEND)
#define PINS_OFFSET_PMIC         (PINS_OFFSET_FHCTL + PINS_SIZE_FHCTL)
#define PINS_OFFSET_PPM          (PINS_OFFSET_PMIC + PINS_SIZE_PMIC)
#define PINS_OFFSET_OCP          (PINS_OFFSET_PPM + PINS_SIZE_PPM)
#define PINS_OFFSET_EXT_BUCK     (PINS_OFFSET_OCP + PINS_SIZE_OCP)
#define PINS_MBOX1_USED          (PINS_OFFSET_EXT_BUCK + PINS_SIZE_EXT_BUCK)
#if (PINS_MBOX1_USED > IPI_MBOX1_SLOTS)
#error "MBOX1 cannot hold all pin definitions"
#endif
/* ============================================================ */

/* mutex_send, sema_ack, mbox, slot, size, shared, retdata, lock, share_grp, polling, unused */
struct _pin_send send_pintable[] = {
	{{{0} }, {0}, 0, PINS_OFFSET_DCS, PINS_SIZE_DCS, 0, 1, 0, 0, 0, 0}, /* DCS */
	{{{0} }, {0}, 0, PINS_OFFSET_MET, PINS_SIZE_MET, 0, 1, 0, 0, 0, 0}, /* on-die-met */
	{{{0} }, {0}, 0, PINS_OFFSET_PLATFORM, PINS_SIZE_PLATFORM, 0, 1, 0, 0, 0, 0}, /* platform */
	{{{0} }, {0}, 0, PINS_OFFSET_PTPOD,  PINS_SIZE_PTPOD, 0, 1, 0, 0, 0, 0},  /* PTPOD */
	{{{0} }, {0}, 0, PINS_OFFSET_CPU_DVFS, PINS_SIZE_CPU_DVFS, 0, 1, 0, 0, 0, 0}, /* CPU DVFS */
	{{{0} }, {0}, 0, PINS_OFFSET_GPU_DVFS, PINS_SIZE_GPU_DVFS, 0, 1, 0, 0, 0, 0}, /* GPU DVFS */
	{{{0} }, {0}, 0, PINS_OFFSET_UNUSED1, PINS_SIZE_UNUSED1, 0, 0, 0, 0, 0, 0}, /* Unused1 */
	{{{0} }, {0}, 0, PINS_OFFSET_UNUSED2, PINS_SIZE_UNUSED2, 0, 0, 0, 0, 0, 0}, /* Unused2 */
	/*====================================================================*/
	{{{0} }, {0}, 1, PINS_OFFSET_PMIC_WRAP,  PINS_SIZE_PMIC_WRAP, 0, 1, 0, 0, 0, 0},  /* PMIC WRAP*/
	{{{0} }, {0}, 1, PINS_OFFSET_CLOCK,  PINS_SIZE_CLOCK, 0, 1, 0, 0, 0, 0},  /* Clock management */
	{{{0} }, {0}, 1, PINS_OFFSET_THERMAL,  PINS_SIZE_THERMAL, 0, 1, 0, 0, 0, 0},  /* THERMAL */
	{{{0} }, {0}, 1, PINS_OFFSET_DEEP_IDLE, PINS_SIZE_DEEP_IDLE, 1, 1, 1, 1, 0, 0},  /* Deep Idle */
	{{{0} }, {0}, 1, PINS_OFFSET_MCDI, PINS_SIZE_MCDI, 1, 1, 1, 1, 0, 0},  /* MCDI */
	{{{0} }, {0}, 1, PINS_OFFSET_SODI, PINS_SIZE_SODI, 1, 1, 1, 1, 0, 0},  /* SODI */
	{{{0} }, {0}, 1, PINS_OFFSET_SPM_SUSPEND, PINS_SIZE_SPM_SUSPEND, 1, 1, 1, 1, 0, 0},   /* SPM suspend */
	{{{0} }, {0}, 1, PINS_OFFSET_FHCTL,  PINS_SIZE_FHCTL, 0, 1, 0, 0, 0, 0},  /* FHCTL */
	{{{0} }, {0}, 1, PINS_OFFSET_PMIC,  PINS_SIZE_PMIC, 0, 1, 0, 0, 0, 0},  /* PMIC */
	{{{0} }, {0}, 1, PINS_OFFSET_PPM,  PINS_SIZE_PPM, 0, 1, 0, 0, 0, 0},  /* PPM */
	{{{0} }, {0}, 1, PINS_OFFSET_OCP,  PINS_SIZE_OCP, 0, 1, 0, 0, 0, 0},  /* OCP */
	{{{0} }, {0}, 1, PINS_OFFSET_EXT_BUCK,  PINS_SIZE_EXT_BUCK, 0, 1, 0, 0, 0, 0},  /* OCP */
/*====================================================================*/
};
#define TOTAL_SEND_PIN      (sizeof(send_pintable)/sizeof(struct _pin_send))

/* definition of slot size for received PINs */
#define PINR_SIZE_DCS            6  /* the following will use mbox 2 */
#define PINR_SIZE_MET            4
#define PINR_SIZE_PLATFORM       3
#define PINR_SIZE_PTPOD          4
#define PINR_SIZE_CPU_DVFS       4
#define PINR_SIZE_GPU_DVFS       3
/* definition of slot offset for PINs */
#define PINR_OFFSET_DCS          0  /* the following will use mbox 0 */
#define PINR_OFFSET_MET          (PINR_OFFSET_DCS + PINR_SIZE_DCS)
#define PINR_OFFSET_PLATFORM     (PINR_OFFSET_MET + PINR_SIZE_MET)
#define PINR_OFFSET_PTPOD        (PINR_OFFSET_PLATFORM + PINR_SIZE_PLATFORM)
#define PINR_OFFSET_CPU_DVFS     (PINR_OFFSET_PTPOD + PINR_SIZE_PTPOD)
#define PINR_OFFSET_GPU_DVFS     (PINR_OFFSET_CPU_DVFS + PINR_SIZE_CPU_DVFS)
#define PINR_MBOX2_USED          (PINR_OFFSET_GPU_DVFS + PINR_SIZE_GPU_DVFS)
#if (PINR_MBOX2_USED > IPI_MBOX2_SLOTS)
#error "MBOX2 cannot hold all pin definitions"
#endif

/* act, mbox, slot, size, shared, retdata, lock, share_grp, unused */
struct _pin_recv recv_pintable[] = {
	{NULL, 2, PINR_OFFSET_DCS, PINR_SIZE_DCS, 0, 1, 0, 0, 0},   /* DCS */
	{NULL, 2, PINR_OFFSET_MET, PINR_SIZE_MET, 0, 1, 0, 0, 0},   /* on-die-met */
	{NULL, 2, PINR_OFFSET_PLATFORM, PINR_SIZE_PLATFORM, 0, 1, 0, 0, 0},   /* platform */
	{NULL, 2, PINR_OFFSET_PTPOD, PINR_SIZE_PTPOD, 0, 1, 0, 0, 0},   /* ptpod */
	{NULL, 2, PINR_OFFSET_CPU_DVFS, PINR_SIZE_CPU_DVFS, 0, 1, 0, 0, 0},   /* CPU DVFS */
	{NULL, 2, PINR_OFFSET_GPU_DVFS, PINR_SIZE_GPU_DVFS, 0, 1, 0, 0, 0},   /* GPU DVFS */
};
#define TOTAL_RECV_PIN      (sizeof(recv_pintable)/sizeof(struct _pin_recv))

/* info for all mbox: start, end, used_slot, mode, unused */
struct _mbox_info mbox_table[IPI_MBOX_TOTAL] = {
	{0, 7, PINS_MBOX0_USED, 2, 0},  /* mbox 0 for send */
	{8, 19, PINS_MBOX1_USED, 2, 0}, /* mbox 1 for send */
	{0, 5, PINR_MBOX2_USED, 1, 0},  /* mbox 2 for recv */
	{0, 0, 0, 0, 0}, /* mbox 3 */
	{0, 0, 0, 0, 0}, /* mbox 4 */
};

static char *pin_name[IPI_ID_TOTAL] = {
	"DCS",
	"MET",
	"PLATFORM",
	"PTPOD",
	"CPU_DVFS",
	"GPU_DVFS",
	"UNUSED1",
	"UNUSED2",
	"PMIC_WRAP",
	"CLOCK",
	"THERMAL",
	"DEEP_IDLE",
	"MCDI",
	"SODI",
	"SPM_SUSPEND",
	"FHCTL",
	"PMIC",
	"PPM",
	"OCP",
	"EXT_BUCK"
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

			if ((j >= IPI_ID_MCDI) && (j <= IPI_ID_SPM_SUSPEND)) {
				/* Ingnore the pins with the same slot space */
				continue;
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

#endif /* __SSPM_IPI_DEFINE_H__ */
