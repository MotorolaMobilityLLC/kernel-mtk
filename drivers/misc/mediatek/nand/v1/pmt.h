
#ifndef _PMT_H
#define _PMT_H
#include <mach/mtk_nand.h>
#include <mtk_nand_util.h>
#include "partition_define.h"

#define MAX_PARTITION_NAME_LEN 64

int new_part_tab(u8 *buf, struct mtd_info *mtd);
int update_part_tab(struct mtd_info *mtd);
static int read_pmt(void __user *arg);
typedef u32(*GetLowPageNumber) (u32 pageNo);
extern GetLowPageNumber functArray[];
extern bool g_bInitDone;
extern struct mtk_nand_host *host;
extern flashdev_info gn_devinfo;
extern bool g_bInitDone;

typedef struct {
	unsigned char name[MAX_PARTITION_NAME_LEN];	/* partition name */
	unsigned long long size;	/* partition size */
	unsigned long long offset;	/* partition start */
	unsigned long long mask_flags;	/* partition flags */
} pt_resident;


#define DM_ERR_OK 0
#define DM_ERR_NO_VALID_TABLE 9
#define DM_ERR_NO_SPACE_FOUND 10
#define ERR_NO_EXIST  1

#define PT_SIG      0x50547632	/*"PTv2" */
#define MPT_SIG    0x4D505432	/*"MPT2" */
#define PT_SIG_SIZE 4
#define is_valid_mpt(buf) ((*(u32 *)(buf)) == MPT_SIG)
#define is_valid_pt(buf) ((*(u32 *)(buf)) == PT_SIG)
#define RETRY_TIMES 5


typedef struct _DM_PARTITION_INFO {
	char part_name[MAX_PARTITION_NAME_LEN];	/* the name of partition */
	unsigned long long start_addr;	/* the start address of partition */
	unsigned long long part_len;	/* the length of partition */
	unsigned char part_visibility;	/* part_visibility is 0: this partition is hidden and CANNOT download */
	/* part_visibility is 1: this partition is visible and can download */
	unsigned char dl_selected;	/* dl_selected is 0: this partition is NOT selected to download */
	/* dl_selected is 1: this partition is selected to download */
} DM_PARTITION_INFO;

typedef struct {
	unsigned int pattern;
	unsigned int part_num;	/* The actual number of partitions */
	DM_PARTITION_INFO part_info[PART_MAX_COUNT];
} DM_PARTITION_INFO_PACKET;

typedef struct {
	int sequencenumber:8;
	int tool_or_sd_update:8;
	int mirror_pt_dl:4;
	int mirror_pt_has_space:4;
	int pt_changed:4;
	int pt_has_space:4;
} pt_info;

#endif
