/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MDLAIOCTL_
#define _MDLAIOCTL_

#include <linux/ioctl.h>
#include <linux/types.h>

/* Memory type for mdla_buf_alloc */
enum mem_type {
	MEM_DRAM,
	MEM_IOMMU,
	MEM_GSM
};

enum MDLA_PMU_INTERFACE {
	MDLA_PMU_IF_WDEC0 = 0xe,
	MDLA_PMU_IF_WDEC1 = 0xf,
	MDLA_PMU_IF_CBLD0 = 0x10,
	MDLA_PMU_IF_CBLD1 = 0x11,
	MDLA_PMU_IF_SBLD0 = 0x12,
	MDLA_PMU_IF_SBLD1 = 0x13,
	MDLA_PMU_IF_STE0 = 0x14,
	MDLA_PMU_IF_STE1 = 0x15,
	MDLA_PMU_IF_CMDE = 0x16,
	MDLA_PMU_IF_DDE = 0x17,
	MDLA_PMU_IF_CONV = 0x18,
	MDLA_PMU_IF_RQU = 0x19,
	MDLA_PMU_IF_POOLING = 0x1a,
	MDLA_PMU_IF_EWE = 0x1b,
	MDLA_PMU_IF_CFLD = 0x1c
};

enum MDLA_PMU_DDE_EVENT {
	MDLA_PMU_DDE_WORK_CYC = 0x0,
	MDLA_PMU_DDE_TILE_DONE_CNT,
	MDLA_PMU_DDE_EFF_WORK_CYC,
	MDLA_PMU_DDE_BLOCK_CNT,
	MDLA_PMU_DDE_READ_CB_WT_CNT,
	MDLA_PMU_DDE_READ_CB_ACT_CNT,
	MDLA_PMU_DDE_WAIT_CB_TOKEN_CNT,
	MDLA_PMU_DDE_WAIT_CONV_RDY_CNT,
	MDLA_PMU_DDE_WAIT_CB_FCWT_CNT,
};

enum MDLA_PMU_MODE {
	MDLA_PMU_ACC_MODE = 0x0,
	MDLA_PMU_INTERVAL_MODE = 0x1,
};

struct ioctl_malloc {
	__u32 size;  /* [in] allocate size */
	__u32 mva;   /* [out] device phyiscal address */
	void *kva;   /* [out] kernel virtual address */
	__u8 type;   /* [in] allocate memory type */
	void *data;  /* [out] userspace virtual address */
};

struct ioctl_run_cmd {
	__u64 kva;      /* [in] kernel virtual address */
	__u32 mva;      /* [in] device phyiscal address */
	__u32 count;    /* [in] # of commands */
	__u32 id;       /* [out] command id */
	__u64 khandle;  /* [in] ion kernel handle */
	__u8 type;      /* [in] allocate memory type */
};

struct ioctl_perf {
	int handle;
	__u32 interface;
	__u32 event;
	__u32 counter;
	__u32 start;
	__u32 end;
	__u32 mode;
};

struct ioctl_ion {
	int fd;         /* [in] user handle, eq. ion_user_handle_t */
	__u64 mva;      /* [in] phyiscal address */
	__u64 kva;      /* [in(unmap)/out(map)] kernel virtual address */
	__u64 khandle;  /* [in(unmap)/out(map)] kernel handle */
	size_t len;     /* [in] memory size */
};

#define IOC_MDLA ('\x1d')

#define IOCTL_MALLOC              _IOWR(IOC_MDLA, 0, struct ioctl_malloc)
#define IOCTL_FREE                _IOWR(IOC_MDLA, 1, struct ioctl_malloc)
#define IOCTL_RUN_CMD_SYNC        _IOWR(IOC_MDLA, 2, struct ioctl_run_cmd)
#define IOCTL_RUN_CMD_ASYNC       _IOWR(IOC_MDLA, 3, struct ioctl_run_cmd)
#define IOCTL_WAIT_CMD            _IOWR(IOC_MDLA, 4, struct ioctl_run_cmd)
#define IOCTL_PERF_SET_EVENT      _IOWR(IOC_MDLA, 5, struct ioctl_perf)
#define IOCTL_PERF_GET_EVENT      _IOWR(IOC_MDLA, 6, struct ioctl_perf)
#define IOCTL_PERF_GET_CNT        _IOWR(IOC_MDLA, 7, struct ioctl_perf)
#define IOCTL_PERF_UNSET_EVENT    _IOWR(IOC_MDLA, 8, struct ioctl_perf)
#define IOCTL_PERF_GET_START      _IOWR(IOC_MDLA, 9, struct ioctl_perf)
#define IOCTL_PERF_GET_END        _IOWR(IOC_MDLA, 10, struct ioctl_perf)
#define IOCTL_PERF_GET_CYCLE      _IOWR(IOC_MDLA, 11, struct ioctl_perf)
#define IOCTL_PERF_RESET_CNT      _IOWR(IOC_MDLA, 12, struct ioctl_perf)
#define IOCTL_PERF_RESET_CYCLE    _IOWR(IOC_MDLA, 13, struct ioctl_perf)
#define IOCTL_PERF_SET_MODE       _IOWR(IOC_MDLA, 14, struct ioctl_perf)
#define IOCTL_ION_KMAP            _IOWR(IOC_MDLA, 15, struct ioctl_ion)
#define IOCTL_ION_KUNMAP          _IOWR(IOC_MDLA, 16, struct ioctl_ion)

#endif
