#ifndef __MTK_NAND_CHIP_H__
#define __MTK_NAND_CHIP_H__

/* mtk_nand_chip_info option bit define*/
#define MTK_NAND_FDM_PARTIAL_SUPPORT		(1<<0)
#define MTK_NAND_PLANE_MODE_SUPPORT		(1<<1)
#define MTK_NAND_CACHE_PROGRAM_SUPPORT		(1<<2)
#define MTK_NAND_DISCONTIGOUS_BUFFER_SUPPORT	(1<<3)
#define MTK_NAND_MULTI_CHIP_SUPPORT		(1<<4)

enum nand_flash_type {
	MTK_NAND_FLASH_SLC = 0,
	MTK_NAND_FLASH_MLC,
	MTK_NAND_FLASH_TLC,
};

#ifndef ENANDFLIPS
#define ENANDFLIPS	1024	/* Too many bitflips, corrected */
#define ENANDREAD	1025	/* Read fail, can't correct */
#define ENANDWRITE	1026	/* Write fail */
#define ENANDERASE	1027	/* Erase fail */
#define ENANDBAD	1028	/* Bad block */
#endif

/**********  API For Wapper ***********/
struct mtk_nand_chip_bbt_info {
	unsigned int bad_block_num;
	unsigned short bad_block_table[1024];
};

struct mtk_nand_chip_info {
	int data_block_num;		/* Number of data blocks */
	int data_page_num;		/* Number of page in a data block */
	int data_page_size;		/* Data page size */
	int data_oob_size;		/* Data OOB size in a page in bytes */
	int data_block_size;		/* Data block size */

	int log_block_num;		/* Number of log blocks */
	int log_page_num;		/* Number of page in a log block */
	int log_page_size;		/* Log page size */
	int log_oob_size;		/* Log OOB size in a page in bytes */
	int log_block_size;		/* Log block size */
	unsigned int slc_ratio;		/* FTL SLC ratio here */
	unsigned int start_block;	/* FTL partition start block number */
	unsigned int sector_size_shift;	/* Minimum Data size shift */
	/*      (1<<sector_size_shift) is sector size */
	/*      9: 512Bytes, 10: 1KB, 11: 2KB, others: Reserved */
	unsigned int max_keep_pages;	/* Max page number for write operation */
	/*      1 : MLC with single-plane */
	/*      2 : MLC with multi-plane */
	/*      9 : TLC with single plane */
	/*      18: TLC with multi-plane */
	enum nand_flash_type types;	/* Define nand flash types */
	unsigned int plane_mask;	/* Plane mask information */
	unsigned int plane_num;	/* Plane number */
	/*      1: Single plane, 2: Multi-plane, 4: Quad plane */
	unsigned int chip_num;       /* Chip number, 1/2/4 or more */
	unsigned int option;            /* Chip ability information */
	/*      bit[0]: FDM partial read */
	/*              0: Not support, 1: Support(Next Gen IP) */
	/*      bit[1]: Plane mode, 0: Not support, 1: Support */
	/*      bit[2]: Cache program, 0: Not support, 1: Support */
	/*      bit[3]: Buffer discontiguous address. */
	/*              this requirement for dma address */
	/*              0: Not support, 1: support, next gen IP  */
	/*      bit[4]: Multi-chip mode */
	/*              0: Not support, 1: Support */
	/*      bit[5~31]: Reserved  */
};

/* struct mtk_nand_chip_info *mtk_nand_chip_init(void)
 * Init mntl_chip_info to nand wrapper, after nand driver init.
 * Return: On success, return mtk_nand_chip_info. On error, return error number.
 */
struct mtk_nand_chip_info *mtk_nand_chip_init(void);

/* mtk_nand_chip_read_page
 * Only one page data and FDM data read, support partial read.
 *
 * @info: NAND handler
 * @data_buffer/oob_buffer: Null for no need data/oob, must contiguous address space.
 * @block/page: The block/page to read data.
 * @offset: data offset to start read, and must be aligned to sector size.
 * @size: data size to read. size <= pagesize, less than page size will partial read,
 *    and OOB is only related sectors, uncompleted refer to whole page.
 * return : 0 on success, On error, return error number.
 */
int mtk_nand_chip_read_page(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page,
		unsigned int offset, unsigned int size);

/*
 * mtk_nand_callback_func
 * Callback for Nand async operation.
 * After Nand driver erase/write operation, callback function will be called.
 * @block/page: The block/page for current operation.
 * @data_buffer/oob_buffer: The block/page for current operation.
 * @status: operation status info from driver.
 * @* userdata : userdata for callback.
 * return : On success, return 0. On error, return error number
 */
typedef int (*mtk_nand_callback_func)(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page, int status, void *userdata);

/* mtk_nand_chip_write_page
 * write page API. Only one page data write, async mode.
 * Just return 0 and add to write worklist as below:
 *  a) For TLC WL write, NAND handler call nand driver WL write function.
 *  b) For Multi-plane program, if more_page is TRUE,
 *  wait for the next pages write and do multi-plane write on time.
 *  c) For cache  program, driver will depend on more_page flag for TLC program,
 *  can not used for SLC/MLC program.
 * after Nand driver erase/write operation, callback function will be done.
 *
 * @info: NAND handler
 * @data_buffer/oob_buffer: must contiguous address space.
 * @block/page: The block/page to write data.
 * @more_page: for TLC WL write and multi-plane program operation.
 *    if more_page is true, driver will wait complete operation and call driver function.
 * @*callback: callback for wrapper, called after driver finish the operation.
 * @*userdata : for callback function
 * return : 0 on success, On error, return error number casted by ERR_PTR
 */
int mtk_nand_chip_write_page(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page, bool more_page,
		mtk_nand_callback_func callback, void *userdata);

/*
 * mtk_nand_chip_erase_block
 * Erase API for nand wrapper, async mode for erase, just return success,
 * put erase operation into write worklist.
 * After Nand driver erase/write operation, callback function will be done.
 * @block: The block to erase
 * @*callback: Callback for wrapper, called after driver finish the operation.
 * @* userdata: for callback function
 * return : 0 on success, On error, return error number casted by ERR_PTR
 */
int mtk_nand_chip_erase_block(struct mtk_nand_chip_info *info,
		unsigned int block, unsigned int more_block,
		mtk_nand_callback_func callback, void *userdata);


/*
 * mtk_nand_chip_sync
 * flush all async worklist to nand driver.
 * return : On success, return 0. On error, return error number
 */
int mtk_nand_chip_sync(struct mtk_nand_chip_info *info);

/* mtk_nand_chip_bmt, bad block table maintained by driver, and read only for wrapper
 * @info: NAND handler
 * Return FTL partition's bad block table for nand wrapper.
 */
const struct mtk_nand_chip_bbt_info *mtk_nand_chip_bmt(
		struct mtk_nand_chip_info *info);

/* mtk_chip_mark_bad_block
 * Mark specific block as bad block,and update bad block list and bad block table.
 * @block: block address to markbad
 */
void mtk_chip_mark_bad_block(unsigned int block);

#endif
