#ifndef MT_SD_H
#define MT_SD_H

#include <linux/bitops.h>
#include <linux/mmc/host.h>
#include <mt-plat/sync_write.h>
/* weiping fix */
#include "board.h"
#if !defined(CONFIG_MTK_CLKMGR)
#include <linux/clk.h>
#endif

/* #define MSDC_DMA_ADDR_DEBUG */
/* ccyeh #define MSDC_HQA */

#define MTK_MSDC_USE_CMD23
#if defined(CONFIG_MTK_EMMC_CACHE) && defined(MTK_MSDC_USE_CMD23)
#define MTK_MSDC_USE_CACHE
extern unsigned int g_emmc_cache_size;
#endif

#ifdef MTK_MSDC_USE_CMD23
#define MSDC_USE_AUTO_CMD23   (1)
#endif

#define HOST_MAX_NUM						(4)
#define MAX_REQ_SZ							(512 * 1024)

#define CMD_TUNE_UHS_MAX_TIME				(2*32*8*8)
#define CMD_TUNE_HS_MAX_TIME				(2*32)

#define READ_TUNE_UHS_CLKMOD1_MAX_TIME		(2*32*32*8)
#define READ_TUNE_UHS_MAX_TIME				(2*32*32)
#define READ_TUNE_HS_MAX_TIME				(2*32)

#define WRITE_TUNE_HS_MAX_TIME				(2*32)
#define WRITE_TUNE_UHS_MAX_TIME				(2*32*8)

#define MAX_HS400_TUNE_COUNT				(576)	/* (32*18) */

#define MAX_GPD_NUM							(1 + 1)	/* one null gpd */
#define MAX_BD_NUM							(1024)
#define MAX_BD_PER_GPD						(MAX_BD_NUM)
#define CLK_SRC_MAX_NUM						(1)

#define EINT_MSDC1_INS_POLARITY				(0)
#define SDIO_ERROR_BYPASS

/* #define MSDC_CLKSRC_REG     (0xf100000C)*/
#define MSDC_DESENSE_REG					(0xf0007070)

#ifdef CONFIG_SDIOAUTOK_SUPPORT
#define MTK_SDIO30_ONLINE_TUNING_SUPPORT
/* #define OT_LATENCY_TEST */
#endif
/* #define ONLINE_TUNING_DVTTEST */

#ifdef CONFIG_FPGA_EARLY_PORTING
#define FPGA_PLATFORM
#else
#define MTK_MSDC_BRINGUP_DEBUG
#endif
/* #define MTK_MSDC_DUMP_FIFO */

#define CMD_SET_FOR_MMC_TUNE_CASE1			(0x00000000FB260140ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE2			(0x0000000000000080ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE3			(0x0000000000001000ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE4			(0x0000000000000020ULL)
/* #define CMD_SET_FOR_MMC_TUNE_CASE5 (0x0000000000084000ULL) */

#define CMD_SET_FOR_SD_TUNE_CASE1			(0x000000007B060040ULL)
#define CMD_SET_FOR_APP_TUNE_CASE1			(0x0008000000402000ULL)

#define IS_IN_CMD_SET(cmd_num, set)			((0x1ULL << cmd_num) & (set))

#define MSDC_VERIFY_NEED_TUNE				(0)
#define MSDC_VERIFY_ERROR					(1)
#define MSDC_VERIFY_NEED_NOT_TUNE			(2)

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MSDC_FIFO_SZ						(128)
#define MSDC_FIFO_THD						(64)	/* (128) */
#define MSDC_NUM							(4)

/* No memory stick mode, 0 use to gate clock */
#define MSDC_MS								(0)
#define MSDC_SDMMC							(1)

#define MSDC_MODE_UNKNOWN					(0)
#define MSDC_MODE_PIO						(1)
#define MSDC_MODE_DMA_BASIC					(2)
#define MSDC_MODE_DMA_DESC					(3)
#define MSDC_MODE_DMA_ENHANCED				(4)
#define MSDC_MODE_MMC_STREAM				(5)

#define MSDC_BUS_1BITS						(0)
#define MSDC_BUS_4BITS						(1)
#define MSDC_BUS_8BITS						(2)

#define MSDC_BRUST_8B						(3)
#define MSDC_BRUST_16B						(4)
#define MSDC_BRUST_32B						(5)
#define MSDC_BRUST_64B						(6)

#define MSDC_PIN_PULL_NONE					(0)
#define MSDC_PIN_PULL_DOWN					(1)
#define MSDC_PIN_PULL_UP					(2)
#define MSDC_PIN_KEEP						(3)

#define MSDC_AUTOCMD12						(1)
#define MSDC_AUTOCMD23						(2)
#define MSDC_AUTOCMD19						(3)
#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
#define MSDC_AUTOCMD53						(4)
#endif

#define MSDC_EMMC_BOOTMODE0					(0)	/* Pull low CMD mode */
#define MSDC_EMMC_BOOTMODE1					(1)	/* Reset CMD mode */

enum {
	RESP_NONE = 0,
	RESP_R1,
	RESP_R2,
	RESP_R3,
	RESP_R4,
	RESP_R5,
	RESP_R6,
	RESP_R7,
	RESP_R1B
};

#define START_AT_RISING             (0x0)
#define START_AT_FALLING            (0x1)
#define START_AT_RISING_AND_FALLING (0x2)
#define START_AT_RISING_OR_FALLING  (0x3)

#define MSDC_SMPL_RISING        (0)
#define MSDC_SMPL_FALLING       (1)
#define MSDC_SMPL_SEPARATE      (2)

#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

#define CARD_READY_FOR_DATA             (1<<8)
#define CARD_CURRENT_STATE(x)           ((x&0x00001E00)>>9)

#define MSDC_PIN_PULL_NONE      (0)
#define MSDC_PIN_PULL_DOWN      (1)
#define MSDC_PIN_PULL_UP        (2)
#define MSDC_PIN_KEEP           (3)

/* add pull down/up mode define */
#define MSDC_GPIO_PULL_UP       (0)
#define MSDC_GPIO_PULL_DOWN     (1)

#include "msdc_reg.h"
#include "msdc_io.h"

/*--------------------------------------------------------------------------*/
/* Descriptor Structure                                                     */
/*--------------------------------------------------------------------------*/
struct gpd_t {
	u32 hwo:1;		/* could be changed by hw */
	u32 bdp:1;
	u32 rsv0:6;
	u32 chksum:8;
	u32 intr:1;
	u32 rsv1:7;
	u32 nexth4:4;
	u32 ptrh4:4;
	u32 next;
	u32 ptr;
	u32 buflen:24;
	u32 extlen:8;
	u32 arg;
	u32 blknum;
	u32 cmd;
};

struct bd_t {
	u32 eol:1;
	u32 rsv0:7;
	u32 chksum:8;
	u32 rsv1:1;
	u32 blkpad:1;
	u32 dwpad:1;
	u32 rsv2:5;
	u32 nexth4:4;
	u32 ptrh4:4;
	u32 next;
	u32 ptr;
	u32 buflen:24;
	u32 rsv3:8;
};

struct scatterlist_ex {
	u32 cmd;
	u32 arg;
	u32 sglen;
	struct scatterlist *sg;
};

#define DMA_FLAG_NONE       (0x00000000)
#define DMA_FLAG_EN_CHKSUM  (0x00000001)
#define DMA_FLAG_PAD_BLOCK  (0x00000002)
#define DMA_FLAG_PAD_DWORD  (0x00000004)

struct msdc_dma {
	u32 flags;		/* flags */
	u32 xfersz;		/* xfer size in bytes */
	u32 sglen;		/* size of scatter list */
	u32 blklen;		/* block size */
	struct scatterlist *sg;	/* I/O scatter list */
	struct scatterlist_ex *esg;	/* extended I/O scatter list */
	u8 mode;		/* dma mode        */
	u8 burstsz;		/* burst size      */
	u8 intr;		/* dma done interrupt */
	u8 padding;		/* padding */
	u32 cmd;		/* enhanced mode command */
	u32 arg;		/* enhanced mode arg */
	u32 rsp;		/* enhanced mode command response */
	u32 autorsp;		/* auto command response */

	struct gpd_t *gpd;		/* pointer to gpd array */
	struct bd_t *bd;		/* pointer to bd array */
	dma_addr_t gpd_addr;	/* the physical address of gpd array */
	dma_addr_t bd_addr;	/* the physical address of bd array */
	u32 used_gpd;		/* the number of used gpd elements */
	u32 used_bd;		/* the number of used bd elements */
};

struct tune_counter {
	u32 time_cmd;
	u32 time_read;
	u32 time_write;
	u32 time_hs400;
};
struct msdc_saved_para {
	u32 pad_tune0;
	u32 ddly0;
	u32 ddly1;
	u8 cmd_resp_ta_cntr;
	u8 wrdat_crc_ta_cntr;
	u8 suspend_flag;
	u32 msdc_cfg;
	u32 mode;
	u32 div;
	u32 sdc_cfg;
	u32 iocon;
	u8 timing;
	u32 hz;
	u8 int_dat_latch_ck_sel;
	u8 ckgen_msdc_dly_sel;
	u8 inten_sdio_irq;
	/* for write: 3T need wait before host check busy after crc status */
	u8 write_busy_margin;
	/* for write: host check timeout change to 16T */
	u8 write_crc_margin;
	u8 ds_dly1;
	u8 ds_dly3;
	u32 emmc50_pad_cmd_tune;
	u8 cfg_cmdrsp_path;
	u8 cfg_crcsts_path;
	u8 resp_wait_cnt;
};

#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)

#define DMA_ON 0
#define DMA_OFF 1

struct ot_work_t {
	struct msdc_host *host;
	int chg_volt;
	atomic_t ot_disable;
	atomic_t autok_done;
};
#endif

struct msdc_host {
	struct msdc_hw *hw;

	struct mmc_host *mmc;	/* mmc structure */
	struct mmc_command *cmd;
	struct mmc_data *data;
	struct mmc_request *mrq;
	int cmd_rsp;
	int cmd_rsp_done;
	int cmd_r1b_done;

	int error;
	spinlock_t lock;	/* mutex */
	spinlock_t clk_gate_lock;
	/*to solve removing bad card race condition with hot-plug enable */
	spinlock_t remove_bad_card;
	spinlock_t sdio_irq_lock; /* avoid race condition @ DATA-1 interrupt case */
	int clk_gate_count;
	struct semaphore sem;

	u32 blksz;		/* host block size */
	void __iomem *base;	/* host base address */
	int id;			/* host id */
	int pwr_ref;		/* core power reference count */

	u32 xfer_size;		/* total transferred size */

	struct msdc_dma dma;	/* dma channel */
	u32 dma_addr;		/* dma transfer address */
	u32 dma_left_size;	/* dma transfer left size */
	u32 dma_xfer_size;	/* dma transfer size in bytes */
	int dma_xfer;		/* dma transfer mode */

	u32 write_timeout_ms;
	u32 timeout_ns;		/* data timeout ns */
	u32 timeout_clks;	/* data timeout clks */

	atomic_t abort;		/* abort transfer */

	int irq;		/* host interrupt */

	struct tasklet_struct card_tasklet;

	/* struct delayed_work           remove_card; */
#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
	struct ot_work_t ot_work;
	atomic_t ot_done;
	u32 sdio_performance_vcore;	/* vcore_fixed_during_sdio_transfer */
	struct delayed_work set_vcore_workq; /* vcore_fixed_during_sdio_transfer */
#endif
	atomic_t sdio_stopping;

	struct completion cmd_done;
	struct completion xfer_done;
	struct pm_message pm_state;

	u8 timing;		/* timing specification used */
	u8 power_mode;		/* host power mode */
	u8 bus_width;		/* data bus width */
	u32 mclk;		/* mmc subsystem clock */
	u32 hclk;		/* host clock speed */
	u32 sclk;		/* SD/MS clock speed */
	u8 core_clkon;		/* Host core clock on ? */
	u8 card_clkon;		/* Card clock on ? */
	u8 core_power;		/* core power */
	u8 card_inserted;	/* card inserted ? */
	u8 suspend;		/* host suspended ? */
	u8 reserved;
	u8 app_cmd;		/* for app command */
	u32 app_cmd_arg;
	u64 starttime;
	struct timer_list timer;
	struct tune_counter t_counter;
	u32 rwcmd_time_tune;
	int read_time_tune;
	int write_time_tune;
	u32 write_timeout_uhs104;
	u32 read_timeout_uhs104;
	u32 write_timeout_emmc;
	u32 read_timeout_emmc;
	u8 autocmd;
	u32 sw_timeout;
	u32 power_cycle;	/* power cycle done in tuning flow */
	bool power_cycle_enable;	/*Enable power cycle */
	u32 continuous_fail_request_count;
	u32 sd_30_busy;
	bool tune;

#define MSDC_VIO18_MC1	(0)
#define MSDC_VIO18_MC2	(1)
#define MSDC_VIO28_MC1	(2)
#define MSDC_VIO28_MC2	(3)
#define MSDC_VMC		(4)
#define MSDC_VGP6		(5)

	int power_domain;
	struct msdc_saved_para saved_para;
	int sd_cd_polarity;
	/* to make sure insert mmc_rescan this work in start_host when boot up */
	int sd_cd_insert_work;
	/* driver will get a EINT(Level sensitive) when boot up with card insert */
	struct wakeup_source trans_lock;
	bool block_bad_card;
	struct delayed_work write_timeout;
#ifdef SDIO_ERROR_BYPASS
	int sdio_error;		/* sdio error can't recovery */
#endif
	void (*power_control)(struct msdc_host *host, u32 on);
	void (*power_switch)(struct msdc_host *host, u32 on);
#if !defined(CONFIG_MTK_CLKMGR)
	struct clk *clock_control;
#endif
	struct work_struct			work_tune; /* new thread tune */
	struct mmc_request			*mrq_tune; /* backup host->mrq */
};

struct tag_msdc_hw_para {
	unsigned int version;	/* msdc structure version info */
	unsigned int clk_src;	/* host clock source */
	unsigned int cmd_edge;	/* command latch edge */
	unsigned int rdata_edge;	/* read data latch edge */
	unsigned int wdata_edge;	/* write data latch edge */
	unsigned int clk_drv;	/* clock pad driving */
	unsigned int cmd_drv;	/* command pad driving */
	unsigned int dat_drv;	/* data pad driving */
	unsigned int rst_drv;	/* RST-N pad driving */
	unsigned int ds_drv;	/* eMMC5.0 DS pad driving */
	unsigned int clk_drv_sd_18;
	unsigned int cmd_drv_sd_18;
	unsigned int dat_drv_sd_18;
	unsigned int clk_drv_sd_18_sdr50;
	unsigned int cmd_drv_sd_18_sdr50;
	unsigned int dat_drv_sd_18_sdr50;
	unsigned int clk_drv_sd_18_ddr50;
	unsigned int cmd_drv_sd_18_ddr50;
	unsigned int dat_drv_sd_18_ddr50;
	unsigned int flags;	/* hardware capability flags */
	unsigned int data_pins;	/* data pins */
	unsigned int data_offset;	/* data address offset */
	unsigned int ddlsel;	/* data line delay line fine tune selecion */
	unsigned int rdsplsel;	/* read: latch data line rising or falling */
	unsigned int wdsplsel;	/* write: latch data line rising or falling */
	unsigned int dat0rddly;	/* read; range: 0~31 */
	unsigned int dat1rddly;	/* read; range: 0~31 */
	unsigned int dat2rddly;	/* read; range: 0~31 */
	unsigned int dat3rddly;	/* read; range: 0~31 */
	unsigned int dat4rddly;	/* read; range: 0~31 */
	unsigned int dat5rddly;	/* read; range: 0~31 */
	unsigned int dat6rddly;	/* read; range: 0~31 */
	unsigned int dat7rddly;	/* read; range: 0~31 */
	unsigned int datwrddly;	/* write; range: 0~31 */
	unsigned int cmdrrddly;	/* cmd; range: 0~31 */
	unsigned int cmdrddly;	/* cmd; range: 0~31 */
	unsigned int host_function;	/* define host function */
	unsigned int boot;	/* define boot host */
	unsigned int cd_level;	/* card detection level */
	unsigned int end_flag;	/* This struct end flag, should be 0x5a5a5a5a */
};

struct dma_addr {
	u32 start_address;
	u32 size;
	u8 end;
	struct dma_addr *next;
};

struct msdc_reg_control {
	ulong addr;
	u32 mask;
	u32 value;
	u32 default_value;
	int (*restore_func)(int restore);
};

static inline unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

#define sdr_read8(reg)        __raw_readb((const volatile void __iomem *)reg)
#define sdr_read16(reg)       __raw_readw((const volatile void __iomem *)reg)
#define sdr_read32(reg)       __raw_readl((const volatile void __iomem *)reg)

#define sdr_write8(reg, val)      mt_reg_sync_writeb(val, reg)
#define sdr_write16(reg, val)     mt_reg_sync_writew(val, reg)
#define sdr_write32(reg, val)     mt_reg_sync_writel(val, reg)

#define sdr_set_bits(reg, bs) \
do {\
	unsigned int tv = sdr_read32(reg);\
	tv |= (u32)(bs); \
	sdr_write32(reg, tv); \
} while (0)
#define sdr_clr_bits(reg, bs) \
do { \
	unsigned int tv = sdr_read32(reg);\
	tv &= ~((u32)(bs)); \
	sdr_write32(reg, tv); \
} while (0)
#define msdc_irq_save(val) \
do { \
	val = sdr_read32(MSDC_INTEN); \
	sdr_clr_bits(MSDC_INTEN, val); \
} while (0)

#define msdc_irq_restore(val)	sdr_set_bits(MSDC_INTEN, val)

#define sdr_set_field(reg, field, val) \
do { \
	unsigned int tv = sdr_read32(reg);    \
	tv &= ~(field); \
	tv |= ((val) << (uffs((unsigned int)field) - 1)); \
	sdr_write32(reg, tv); \
} while (0)
#define sdr_get_field(reg, field, val) \
do { \
	unsigned int tv = sdr_read32(reg);    \
	val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
} while (0)
#define sdr_set_field_discrete(reg, field, val) \
do { \
	unsigned int tv = sdr_read32(reg); \
	tv = (val == 1) ? (tv|(field)) : (tv & ~(field));\
	sdr_write32(reg, tv); \
} while (0)
#define sdr_get_field_discrete(reg, field, val) \
do {    \
	unsigned int tv = sdr_read32(reg); \
	val = tv & (field); \
	val = (val == field) ? 1 : 0; \
} while (0)

#define MSDC_SET_BIT32(reg, bs) sdr_set_bits(reg, bs)
#define MSDC_CLR_BIT32(reg, bs) sdr_clr_bits(reg, bs)
#define MSDC_SET_FIELD(reg, field, val) sdr_set_field(reg, field, val)
#define MSDC_GET_FIELD(reg, field, val) sdr_get_field(reg, field, val)
#define MSDC_READ8(reg)           __raw_readb((const volatile void *)reg)
#define MSDC_READ16(reg)          __raw_readw((const volatile void *)reg)
#define MSDC_READ32(reg)          __raw_readl((const volatile void *)reg)
#define MSDC_WRITE8(reg, val)     mt_reg_sync_writeb(val, reg)
#define MSDC_WRITE16(reg, val)    mt_reg_sync_writew(val, reg)
#define MSDC_WRITE32(reg, val)    mt_reg_sync_writel(val, reg)

#define UNSTUFF_BITS(resp, start, size)          \
({				  \
	const int __size = size;		\
	const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;  \
	const int __off = 3 - ((start) / 32);	   \
	const int __shft = (start) & 31;	  \
	u32 __res;			  \
		  \
	__res = resp[__off] >> __shft;		  \
	if (__size + __shft > 32)		 \
		__res |= resp[__off-1] << ((32 - __shft) % 32);  \
	__res & __mask; \
})
#define sdc_is_busy()          (sdr_read32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()      (sdr_read32(SDC_STS) & SDC_STS_CMDBUSY)

#define sdc_send_cmd(cmd, arg) \
do { \
	sdr_write32(SDC_ARG, (arg)); \
	sdr_write32(SDC_CMD, (cmd)); \
} while (0)

/* can modify to read h/w register */
/* #define is_card_present(h) ((sdr_read32(MSDC_PS) & MSDC_PS_CDSTS) ? 0 : 1);*/
#define is_card_present(h)     (((struct msdc_host *)(h))->card_inserted)
#define is_card_sdio(h)        (((struct msdc_host *)(h))->hw->register_pm)

/*sd card change voltage wait time= (1/freq) * SDC_VOL_CHG_CNT(default 0x145) */
#define msdc_set_vol_change_wait_count(count) sdr_set_field(SDC_VOL_CHG, \
	SDC_VOL_CHG_CNT, (count))

#define msdc_retry(expr, retry, cnt, id) \
do { \
	int backup = cnt; \
	while (retry) { \
		if (!(expr)) \
			break; \
		if (cnt-- == 0) { \
			retry--; mdelay(1); cnt = backup; \
		} \
	} \
	if (retry == 0) { \
		msdc_dump_info(id); \
	} \
	WARN_ON(retry == 0); \
} while (0)

#define msdc_reset(id) \
do { \
	int retry = 3, cnt = 1000; \
	sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
	mb(); /* need comment? */ \
	msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt, id); \
} while (0)

#define msdc_clr_int() \
do { \
	u32 val = sdr_read32(MSDC_INT); \
	sdr_write32(MSDC_INT, val); \
} while (0)

/* For Inhanced DMA */
#define msdc_init_gpd_ex(gpd, extlen, cmd, arg, blknum) \
do { \
	((struct gpd_t *)gpd)->extlen = extlen; \
	((struct gpd_t *)gpd)->cmd    = cmd; \
	((struct gpd_t *)gpd)->arg    = arg; \
	((struct gpd_t *)gpd)->blknum = blknum; \
} while (0)

#define msdc_init_bd(bd, blkpad, dwpad, dptr, dlen) \
do { \
	BUG_ON(dlen > 0xFFFFUL); \
	((struct bd_t *)bd)->blkpad = blkpad; \
	((struct bd_t *)bd)->dwpad  = dwpad; \
	((struct bd_t *)bd)->ptr	 = (u32)dptr; \
	((struct bd_t *)bd)->buflen = dlen; \
} while (0)

#ifdef CONFIG_NEED_SG_DMA_LENGTH
#define msdc_sg_len(sg, dma)         ((dma) ? (sg)->dma_length : (sg)->length)
#else
#define msdc_sg_len(sg, dma)         sg_dma_len(sg)
#endif
#define msdc_txfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_fifo_write32(v)   sdr_write32(MSDC_TXDATA, (v))
#define msdc_fifo_write8(v)    sdr_write8(MSDC_TXDATA, (v))
#define msdc_fifo_read32()   sdr_read32(MSDC_RXDATA)
#define msdc_fifo_read8()    sdr_read8(MSDC_RXDATA)

#define msdc_dma_on()        sdr_clr_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_off()       sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_status()    ((sdr_read32(MSDC_CFG) & MSDC_CFG_PIO) >> 3)

/* Debug message event */
#define MSDC_EVT_NONE        (0)	/* No event */
#define MSDC_EVT_DMA         (1 << 0)	/* DMA related event */
#define MSDC_EVT_CMD         (1 << 1)	/* MSDC CMD related event */
#define MSDC_EVT_RSP         (1 << 2)	/* MSDC CMD RSP related event */
#define MSDC_EVT_INT         (1 << 3)	/* MSDC INT event */
#define MSDC_EVT_CFG         (1 << 4)	/* MSDC CFG event */
#define MSDC_EVT_FUC         (1 << 5)	/* Function event */
#define MSDC_EVT_OPS         (1 << 6)	/* Read/Write operation event */
#define MSDC_EVT_FIO         (1 << 7)	/* FIFO operation event */
#define MSDC_EVT_WRN         (1 << 8)	/* Warning event */
#define MSDC_EVT_PWR         (1 << 9)	/* Power event */
#define MSDC_EVT_CLK         (1 << 10)	/* Trace clock gate/ungate operation */
#define MSDC_EVT_CHE         (1 << 11)	/* eMMC cache feature operation */
		/* ==================================================== */
#define MSDC_EVT_RW          (1 << 12)	/* Trace the Read/Write Command */
#define MSDC_EVT_NRW         (1 << 13)	/* Trace other Command */
#define MSDC_EVT_ALL         (0xffffffff)

#define MSDC_EVT_MASK        (MSDC_EVT_ALL)

extern unsigned int sd_debug_zone[HOST_MAX_NUM];

#define N_MSG(evt, fmt, args...) \
do { \
	if ((MSDC_EVT_##evt) & sd_debug_zone[host->id]) { \
		pr_err("msdc%d -> "fmt" <- %s() : L<%d> PID<%s><0x%x>\n", \
			host->id,  ##args , __func__, __LINE__, current->comm, \
			current->pid); \
	}	\
} while (0)

#define CMD_MSG(fmt, args...) \
do { \
	if (MSDC_EVT_CMD & sd_debug_zone[host->id]) {\
		pr_err("msdc%d -> "fmt"\n", host->id, ##args); \
	} \
} while (0)

#define ERR_MSG(fmt, args...) \
	pr_err("msdc%d -> "fmt" <- %s() : L<%d> PID<%s><0x%x>\n", \
		host->id,  ##args , __func__, __LINE__, current->comm, current->pid)

extern int drv_mode[HOST_MAX_NUM];
extern int msdc_latest_transfer_mode[HOST_MAX_NUM];
extern int msdc_latest_operation_type[HOST_MAX_NUM];

extern void mmc_remove_card(struct mmc_card *card);
extern void mmc_detach_bus(struct mmc_host *host);
extern void mmc_power_off(struct mmc_host *host);
extern void msdc_dump_gpd_bd(int id);

extern int msdc_tune_cmdrsp(struct msdc_host *host);
extern unsigned int msdc_do_command(struct msdc_host *host,
	struct mmc_command *cmd, int tune, unsigned long timeout);
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
extern unsigned int autok_get_current_vcore_offset(void);
#endif
#if defined(FEATURE_MET_MMC_INDEX)
extern void met_mmc_issue(struct mmc_host *host, struct mmc_request *req);
extern void met_mmc_dma_stop(struct mmc_host *host, u32 lba, unsigned int len,
	u32 opcode, unsigned int bd_num);
#endif
extern void init_tune_sdio(struct msdc_host *host);
extern int mmc_flush_cache(struct mmc_card *card);

#ifdef CONFIG_MTK_HIBERNATION
extern unsigned int mt_eint_get_polarity_external(unsigned int eint_num);
#endif
extern int msdc_cache_ctrl(struct msdc_host *host, unsigned int enable,
	u32 *status);
/* weiping fix */
#if defined(CFG_DEV_MSDC0)
extern struct msdc_hw msdc0_hw;
#endif
#if defined(CFG_DEV_MSDC1)
extern struct msdc_hw msdc1_hw;
#endif
#if defined(CFG_DEV_MSDC2)
extern struct msdc_hw msdc2_hw;
#endif
#if defined(CFG_DEV_MSDC3)
extern struct msdc_hw msdc3_hw;
#endif

extern int msdc_setting_parameter(struct msdc_hw *hw, unsigned int *para);
/*workaround for VMC 1.8v -> 1.84v */
extern void upmu_set_rg_vmc_184(unsigned char x);

extern void __iomem *gpio_reg_base;
extern void __iomem *infracfg_ao_reg_base;
extern void __iomem *infracfg_reg_base;
extern void __iomem *pericfg_reg_base;
extern void __iomem *emi_reg_base;
extern void __iomem *toprgu_reg_base;
extern void __iomem *apmixed_reg_base1;
extern void __iomem *topckgen_reg_base;

#endif				/* end of  MT_SD_H */
