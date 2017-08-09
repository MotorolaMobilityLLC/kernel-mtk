#ifndef MT_SD_H
#define MT_SD_H

#ifdef CONFIG_FPGA_EARLY_PORTING
#define FPGA_PLATFORM
#else
#define MTK_MSDC_BRINGUP_DEBUG
#endif

#include <linux/bitops.h>
#include <linux/mmc/host.h>
#include <mt-plat/sync_write.h>
#include <linux/slab.h>
#if !defined(CONFIG_MTK_LEGACY)
#include <linux/clk.h>
#endif /* !defined(CONFIG_MTK_LEGACY) */
#include "board.h"
#define MSDC_DMA_ADDR_DEBUG
/*#define MSDC_HQA*/

#define MSDC_STRIPE_DT

#define MTK_MSDC_USE_CMD23
#if defined(CONFIG_MTK_EMMC_CACHE) && defined(MTK_MSDC_USE_CMD23)
/* FIXME: cache is ready */
/* #define MTK_MSDC_USE_CACHE */
#endif

#ifdef MTK_MSDC_USE_CMD23
#define MSDC_USE_AUTO_CMD23             (1)
#endif

#ifdef MTK_MSDC_USE_CACHE
#ifndef MMC_ENABLED_EMPTY_QUEUE_FLUSH
/* #define MTK_MSDC_FLUSH_BY_CLK_GATE */
#endif
#endif

#define HOST_MAX_NUM                    (4)
#define MAX_REQ_SZ                      (512 * 1024)

#ifdef FPGA_PLATFORM
#define HOST_MAX_MCLK			(200000000)
#else
#define HOST_MAX_MCLK			(200000000)
#endif
#define HOST_MIN_MCLK			(260000)

/* ================================= */

#define MAX_GPD_NUM                     (1 + 1) /* one null gpd */
#define MAX_BD_NUM                      (1024)
#define MAX_BD_PER_GPD                  (MAX_BD_NUM)
#define CLK_SRC_MAX_NUM                 (1)

#define EINT_MSDC1_INS_POLARITY         (0)
#define SDIO_ERROR_BYPASS

/*#define MSDC_CLKSRC_REG               (0xf100000C)*/

#ifdef CONFIG_SDIOAUTOK_SUPPORT
#define MTK_SDIO30_ONLINE_TUNING_SUPPORT
/*#define OT_LATENCY_TEST*/
#endif
/*#define ONLINE_TUNING_DVTTEST*/

/*#define MTK_MSDC_DUMP_FIFO*/

/*--------------------------------------------------------------------------*/
/* Common Macro                                                             */
/*--------------------------------------------------------------------------*/
#define REG_ADDR(x)                     ((volatile u32 *)(base + OFFSET_##x))

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD           (64)    /* (128) */
#define MSDC_NUM                (4)

#define MSDC_MS                 (0)     /* No memory stick mode,
					   0 use to gate clock */
#define MSDC_SDMMC              (1)

#define MSDC_MODE_UNKNOWN       (0)
#define MSDC_MODE_PIO           (1)
#define MSDC_MODE_DMA_BASIC     (2)
#define MSDC_MODE_DMA_DESC      (3)
#define MSDC_MODE_DMA_ENHANCED  (4)
#define MSDC_MODE_MMC_STREAM    (5)

#define MSDC_BUS_1BITS          (0)
#define MSDC_BUS_4BITS          (1)
#define MSDC_BUS_8BITS          (2)

#define MSDC_BRUST_8B           (3)
#define MSDC_BRUST_16B          (4)
#define MSDC_BRUST_32B          (5)
#define MSDC_BRUST_64B          (6)

#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0003)
#define MSDC_AUTOCMD53          (0x0004)

#define MSDC_EMMC_BOOTMODE0     (0)     /* Pull low CMD mode */
#define MSDC_EMMC_BOOTMODE1     (1)     /* Reset CMD mode */

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

#include "msdc_reg.h"

/* MSDC_CFG[START_BIT] value */
#define START_AT_RISING                 (0x0)
#define START_AT_FALLING                (0x1)
#define START_AT_RISING_AND_FALLING     (0x2)
#define START_AT_RISING_OR_FALLING      (0x3)

#define MSDC_SMPL_RISING                (0)
#define MSDC_SMPL_FALLING               (1)
#define MSDC_SMPL_SEPARATE              (2)

#define TYPE_CMD_RESP_EDGE              (0)
#define TYPE_WRITE_CRC_EDGE             (1)
#define TYPE_READ_DATA_EDGE             (2)
#define TYPE_WRITE_DATA_EDGE            (3)

#define CARD_READY_FOR_DATA             (1<<8)
#define CARD_CURRENT_STATE(x)           ((x&0x00001E00)>>9)

enum MSDC_POWER {
	MSDC_VIO18_MC1 = 0,
	MSDC_VIO18_MC2,
	MSDC_VIO28_MC1,
	MSDC_VIO28_MC2,
	MSDC_VMC,
	MSDC_VGP6,
};


/*--------------------------------------------------------------------------*/
/* Descriptor Structure                                                     */
/*--------------------------------------------------------------------------*/
struct gpd_t {
	u32  hwo:1; /* could be changed by hw */
	u32  bdp:1;
	u32  rsv0:6;
	u32  chksum:8;
	u32  intr:1;
	u32  rsv1:7;
	u32  nexth4:4;
	u32  ptrh4:4;
	u32  next;
	u32  ptr;
	u32  buflen:24;
	u32  extlen:8;
	u32  arg;
	u32  blknum;
	u32  cmd;
};

struct bd_t {
	u32  eol:1;
	u32  rsv0:7;
	u32  chksum:8;
	u32  rsv1:1;
	u32  blkpad:1;
	u32  dwpad:1;
	u32  rsv2:5;
	u32  nexth4:4;
	u32  ptrh4:4;
	u32  next;
	u32  ptr;
	u32  buflen:24;
	u32  rsv3:8;
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
	u32 flags;                   /* flags */
	u32 xfersz;                  /* xfer size in bytes */
	u32 sglen;                   /* size of scatter list */
	u32 blklen;                  /* block size */
	struct scatterlist *sg;      /* I/O scatter list */
	struct scatterlist_ex *esg;  /* extended I/O scatter list */
	u8  mode;                    /* dma mode        */
	u8  burstsz;                 /* burst size      */
	u8  intr;                    /* dma done interrupt */
	u8  padding;                 /* padding */
	u32 cmd;                     /* enhanced mode command */
	u32 arg;                     /* enhanced mode arg */
	u32 rsp;                     /* enhanced mode command response */
	u32 autorsp;                 /* auto command response */

	struct gpd_t *gpd;                  /* pointer to gpd array */
	struct bd_t  *bd;                   /* pointer to bd array */
	dma_addr_t gpd_addr;         /* the physical address of gpd array */
	dma_addr_t bd_addr;          /* the physical address of bd array */
	u32 used_gpd;                /* the number of used gpd elements */
	u32 used_bd;                 /* the number of used bd elements */
};

struct tune_counter {
	u32 time_cmd;
	u32 time_read;
	u32 time_write;
	u32 time_hs400;
};

/*FIX ME, consider to move it into msdc_tune.c*/
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
	u8  timing;
	u32 hz;
	u8 int_dat_latch_ck_sel;
	u8 ckgen_msdc_dly_sel;
	u8 inten_sdio_irq;
	u8 write_busy_margin;           /* for write: 3T need wait before host
					   check busy after crc status */
	u8 write_crc_margin;            /* for write: host check timeout change
					   to 16T */
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
	struct      msdc_host *host;
	int         chg_volt;
	atomic_t    ot_disable;
	atomic_t    autok_done;
};
#endif

struct msdc_host {
	struct msdc_hw          *hw;

	struct mmc_host         *mmc;           /* mmc structure */
	struct mmc_command      *cmd;
	struct mmc_data         *data;
	struct mmc_request      *mrq;
	int                     cmd_rsp;
	int                     cmd_rsp_done;
	int                     cmd_r1b_done;

	int                     error;
	spinlock_t              lock;           /* mutex */
	spinlock_t              clk_gate_lock;
	spinlock_t              remove_bad_card; /* to solve removing bad card
						    race condition
						    with hot-plug enable*/
	spinlock_t              sdio_irq_lock;  /* avoid race condition
						   at DAT1 interrupt case*/
	int                     clk_gate_count;
	struct semaphore        sem;

	u32                     blksz;          /* host block size */
	void __iomem            *base;          /* host base address */
	int                     id;             /* host id */
	int                     pwr_ref;        /* core power reference count */

	u32                     xfer_size;      /* total transferred size */

	struct msdc_dma         dma;            /* dma channel */
	u32                     dma_addr;       /* dma transfer address */
	u32                     dma_left_size;  /* dma transfer left size */
	u32                     dma_xfer_size;  /* dma transfer size in bytes */
	int                     dma_xfer;       /* dma transfer mode */

	u32                     timeout_ns;     /* data timeout ns */
	u32                     timeout_clks;   /* data timeout clks */

	atomic_t                abort;          /* abort transfer */

	int                     irq;            /* host interrupt */

	struct tasklet_struct   card_tasklet;
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
	struct tasklet_struct   flush_cache_tasklet;
#endif
#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
	struct ot_work_t        ot_work;
	atomic_t                ot_done;
	u32                     sdio_performance_vcore;
				/*vcore_fixed_during_sdio_transfer*/
	struct delayed_work     set_vcore_workq;
				/*vcore_fixed_during_sdio_transfer*/
#endif
	atomic_t                sdio_stopping;

	struct completion       cmd_done;
	struct completion       xfer_done;
	struct pm_message       pm_state;

	u32                     mclk;           /* mmc subsystem clock */
	u32                     hclk;           /* host clock speed */
	u32                     sclk;           /* SD/MS clock speed */
	u8                      core_clkon;     /* Host core clock on ? */
	u8                      card_clkon;     /* Card clock on ? */
	u8                      core_power;     /* core power */
	u8			timing;
	u8                      power_mode;     /* host power mode */
	u8			bus_width;
	u8                      card_inserted;  /* card inserted ? */
	u8                      suspend;        /* host suspended ? */
	u8                      reserved;
	u8                      app_cmd;        /* for app command */
	u32                     app_cmd_arg;
	u64                     starttime;
	struct timer_list       timer;
	struct tune_counter     t_counter;
	u32                     rwcmd_time_tune;
	int                     read_time_tune;
	int                     write_time_tune;
	u32                     write_timeout_uhs104;
	u32                     read_timeout_uhs104;
	u32                     write_timeout_emmc;
	u32                     read_timeout_emmc;
	u32			write_timeout_ms;     /* data write busy timeout ms */
	u8                      autocmd;
	u32                     sw_timeout;
	u32                     power_cycle;    /* power cycle done
						   in tuning flow*/
	bool                    power_cycle_enable; /*Enable power cycle*/
	bool                    error_tune_enable;  /* enable error tune flow */
	u32                     sd_30_busy;
	bool                    tune;
	unsigned int		power_domain;
	struct msdc_saved_para  saved_para;
	int                     sd_cd_polarity;
	int                     sd_cd_insert_work;
				/* to make sure insert mmc_rescan this work
				   in start_host when boot up.
				   Driver will get a EINT(Level sensitive)
				   when boot up phone with card insert */
	struct wakeup_source	trans_lock;
	bool                    block_bad_card;
	struct delayed_work	write_timeout;       /* check if write busy timeout */
#ifdef SDIO_ERROR_BYPASS
	int                     sdio_error;     /* sdio error can't recovery */
#endif
	void    (*power_control)(struct msdc_host *host, u32 on);
	void    (*power_switch)(struct msdc_host *host, u32 on);
#if !defined(CONFIG_MTK_LEGACY)
	struct clk *clock_control;
#endif /* !defined(CONFIG_MTK_LEGACY) */
	struct work_struct	work_tune; /* new thread tune */
	struct mmc_request	*mrq_tune; /* backup host->mrq */
};

struct tag_msdc_hw_para {
	unsigned int  version;          /* msdc structure version info */
	unsigned int  clk_src;          /* host clock source */
	unsigned int  cmd_edge;         /* command latch edge */
	unsigned int  rdata_edge;       /* read data latch edge */
	unsigned int  wdata_edge;       /* write data latch edge */
	unsigned int  clk_drv;          /* clock pad driving */
	unsigned int  cmd_drv;          /* command pad driving */
	unsigned int  dat_drv;          /* data pad driving */
	unsigned int  rst_drv;          /* RST-N pad driving */
	unsigned int  ds_drv;           /* eMMC5.0 DS pad driving */
	unsigned int  clk_drv_sd_18;    /* clock pad driving for SD card at
					   1.8v sdr104 mode */
	unsigned int  cmd_drv_sd_18;    /* command pad driving for SD card at
					   1.8v sdr104 mode */
	unsigned int  dat_drv_sd_18;    /* data pad driving for SD card at
					   1.8v sdr104 mode */
	unsigned int  clk_drv_sd_18_sdr50;    /* clock pad driving for SD card
						 at 1.8v sdr50 mode */
	unsigned int  cmd_drv_sd_18_sdr50;    /* command pad driving for SD
						 card at 1.8v sdr50 mode */
	unsigned int  dat_drv_sd_18_sdr50;    /* data pad driving for SD card
						 at 1.8v sdr50 mode */
	unsigned int  clk_drv_sd_18_ddr50;    /* clock pad driving for SD card
						 at 1.8v ddr50 mode */
	unsigned int  cmd_drv_sd_18_ddr50;    /* command pad driving for SD
						 card at 1.8v ddr50 mode */
	unsigned int  dat_drv_sd_18_ddr50;    /* data pad driving for SD card
						 at 1.8v ddr50 mode */
	unsigned int  flags;            /* hardware capability flags */
	unsigned int  data_pins;        /* data pins */
	unsigned int  data_offset;      /* data address offset */
	unsigned int  ddlsel;           /* data line delay shared or
					   separated selecion */
	unsigned int  rdsplsel;         /* read: data line rising or falling
					   latch selection */
	unsigned int  wdsplsel;         /* write: data line rising or falling
					   latch selection */
	unsigned int  dat0rddly;        /*read; range: 0~31 */
	unsigned int  dat1rddly;        /*read; range: 0~31 */
	unsigned int  dat2rddly;        /*read; range: 0~31 */
	unsigned int  dat3rddly;        /*read; range: 0~31 */
	unsigned int  dat4rddly;        /*read; range: 0~31 */
	unsigned int  dat5rddly;        /*read; range: 0~31 */
	unsigned int  dat6rddly;        /*read; range: 0~31 */
	unsigned int  dat7rddly;        /*read; range: 0~31 */
	unsigned int  datwrddly;        /*write; range: 0~31 */
	unsigned int  cmdrrddly;        /*cmd; range: 0~31 */
	unsigned int  cmdrddly;         /*cmd; range: 0~31 */
	unsigned int  host_function;    /* define host function*/
	unsigned int  boot;             /* define boot host*/
	unsigned int  cd_level;         /* card detection level*/
	unsigned int  end_flag;         /* This struct end flag,
					   should be 0x5a5a5a5a */
};

enum {
	cmd_counter = 0,
	read_counter,
	write_counter,
	all_counter,
};

enum {
	MSDC_STATE_DEFAULT = 0,
	MSDC_STATE_DDR,
	MSDC_STATE_HS200,
	MSDC_STATE_HS400
};

enum {
	TRAN_MOD_PIO,
	TRAN_MOD_DMA,
	TRAN_MOD_NUM
};

enum {
	OPER_TYPE_READ,
	OPER_TYPE_WRITE,
	OPER_TYPE_NUM
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

#define MSDC_READ8(reg)           __raw_readb((const volatile void *)reg)
#define MSDC_READ16(reg)          __raw_readw((const volatile void *)reg)
#define MSDC_READ32(reg)          __raw_readl((const volatile void *)reg)
#define MSDC_WRITE8(reg, val)     mt_reg_sync_writeb(val, reg)
#define MSDC_WRITE16(reg, val)    mt_reg_sync_writew(val, reg)
#define MSDC_WRITE32(reg, val)    mt_reg_sync_writel(val, reg)

#define UNSTUFF_BITS(resp, start, size) \
({ \
	const int __size = size; \
	const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1; \
	const int __off = 3 - ((start) / 32); \
	const int __shft = (start) & 31; \
	u32 __res; \
	__res = resp[__off] >> __shft; \
	if (__size + __shft > 32) \
		__res |= resp[__off-1] << ((32 - __shft) % 32); \
	__res & __mask; \
})

#define MSDC_SET_BIT32(reg, bs) \
	do { \
		volatile unsigned int tv = MSDC_READ32(reg);\
		tv |= (u32)(bs); \
		MSDC_WRITE32(reg, tv); \
	} while (0)

#define MSDC_CLR_BIT32(reg, bs) \
	do { \
		volatile unsigned int tv = MSDC_READ32(reg);\
		tv &= ~((u32)(bs)); \
		MSDC_WRITE32(reg, tv); \
	} while (0)

#define MSDC_SET_FIELD(reg, field, val) \
	do { \
		volatile unsigned int tv = MSDC_READ32(reg); \
		tv &= ~(field); \
		tv |= ((val) << (uffs((unsigned int)field) - 1)); \
		MSDC_WRITE32(reg, tv); \
	} while (0)

#define MSDC_GET_FIELD(reg, field, val) \
	do { \
		volatile unsigned int tv = MSDC_READ32(reg); \
		val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
	} while (0)

#define sdc_is_busy()		(MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()	(MSDC_READ32(SDC_STS) & SDC_STS_CMDBUSY)

#define sdc_send_cmd(cmd, arg) \
	do { \
		MSDC_WRITE32(SDC_ARG, (arg)); \
		MSDC_WRITE32(SDC_CMD, (cmd)); \
	} while (0)

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
		MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_RST); \
		msdc_retry(MSDC_READ32(MSDC_CFG) & MSDC_CFG_RST, retry, \
			cnt, id); \
	} while (0)

#define msdc_clr_int() \
	do { \
		volatile u32 val = MSDC_READ32(MSDC_INT); \
		MSDC_WRITE32(MSDC_INT, val); \
	} while (0)

#define msdc_reset_hw(id) \
	do { \
		msdc_reset(id); \
		msdc_clr_fifo(id); \
		msdc_clr_int(); \
	} while (0)

#define msdc_txfifocnt()        ((MSDC_READ32(MSDC_FIFOCS) \
				 & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()        ((MSDC_READ32(MSDC_FIFOCS) \
				 & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_fifo_write32(v)    MSDC_WRITE32(MSDC_TXDATA, (v))
#define msdc_fifo_write8(v)     MSDC_WRITE8(MSDC_TXDATA, (v))
#define msdc_fifo_read32()      MSDC_READ32(MSDC_RXDATA)
#define msdc_fifo_read8()       MSDC_READ8(MSDC_RXDATA)

/* can modify to read h/w register.*/
/*#define is_card_present(h) \
			((MSDC_READ32(MSDC_PS) & MSDC_PS_CDSTS) ? 0 : 1);*/
#define is_card_present(h)	(((struct msdc_host *)(h))->card_inserted)
#define is_card_sdio(h)		(((struct msdc_host *)(h))->hw->register_pm)

#define CMD_TIMEOUT             (HZ/10 * 5)     /* 100ms x5 */
#define DAT_TIMEOUT             (HZ    * 5)     /* 1000ms x5 */
#define CLK_TIMEOUT             (HZ    * 5)     /* 5s    */
#define POLLING_BUSY            (HZ    * 3)

#ifdef CONFIG_OF
#if defined(CFG_DEV_MSDC2)
extern struct msdc_hw msdc2_hw;
#endif
#if defined(CFG_DEV_MSDC3)
extern struct msdc_hw msdc3_hw;
#endif
#endif

extern struct msdc_host *mtk_msdc_host[];
extern unsigned int msdc_latest_transfer_mode[HOST_MAX_NUM];
#ifdef MSDC_DMA_ADDR_DEBUG
extern struct dma_addr msdc_latest_dma_address[MAX_BD_PER_GPD];
#endif
extern int g_dma_debug[HOST_MAX_NUM];

#ifdef CONFIG_MTK_EMMC_SUPPORT
extern u32 g_emmc_mode_switch;
#endif

enum {
	MODE_PIO = 0,
	MODE_DMA = 1,
	MODE_SIZE_DEP = 2,
};

/* Variable declared in dbg.c */
extern u32 msdc_host_mode[];
extern u32 msdc_host_mode2[];

extern unsigned int sd_debug_zone[];
extern u32 drv_mode[];
extern u32 dma_size[];
extern unsigned char msdc_clock_src[];

extern u32 sdio_pro_enable;

extern bool emmc_sleep_failed;

extern int msdc_rsp[];
extern u8 g_emmc_id;
/**********************************************************/
/* Functions                                               */
/**********************************************************/
#include "msdc_io.h"

/* Function provided by sd.c */
struct gendisk *mmc_get_disk(struct mmc_card *card);
int msdc_clk_stable(struct msdc_host *host, u32 mode, u32 div,
	u32 hs400_src);
void msdc_clr_fifo(unsigned int id);
unsigned int msdc_do_command(struct msdc_host *host,
	struct mmc_command *cmd,
	int                 tune,
	unsigned long       timeout);
int msdc_do_request(struct mmc_host *mmc, struct mmc_request *mrq);
void msdc_dump_gpd_bd(int id);
void msdc_dump_info(u32 id);
void msdc_dump_register(struct msdc_host *host);
void msdc_dump_register_core(u32 id, void __iomem *base);
void msdc_dump_dbg_register_core(u32 id, void __iomem *base);
int msdc_get_cache_region(void);
u64 msdc_get_capacity(int get_emmc_total);
int msdc_get_card_status(struct mmc_host *mmc, struct msdc_host *host,
	u32 *status);
struct dma_addr *msdc_get_dma_address(int host_id);
int msdc_get_dma_status(int host_id);
struct msdc_host *msdc_get_host(int host_function, bool boot,
	bool secondary);
int msdc_get_reserve(void);
int msdc_get_offset(void);
int msdc_pio_read(struct msdc_host *host, struct mmc_data *data);
int msdc_pio_write(struct msdc_host *host, struct mmc_data *data);
int msdc_reinit(struct msdc_host *host);
void msdc_select_clksrc(struct msdc_host *host, int clksrc);
void msdc_send_stop(struct msdc_host *host);
void msdc_set_mclk(struct msdc_host *host, unsigned char timing, u32 hz);
void msdc_set_smpl(struct msdc_host *host, u8 HS400, u8 mode, u8 type,
	u8 *edge);
void msdc_set_smpl_all(struct msdc_host *host, u8 HS400);
int msdc_switch_part(struct msdc_host *host, char part_id);

/* Function provided by msdc_partition.c */
void msdc_proc_emmc_create(void);
int msdc_can_apply_cache(unsigned long long start_addr,
	unsigned int size);

/* Function provided by ettagent.c */
int ettagent_init(void);
void ettagent_exit(void);

/* Function provided by sdio_mt_online_tuning_test.c */
int mt_msdc_online_tuning_test(struct msdc_host *host, u32 rawcmd,
	u32 rawarg, u8 rw);

/* Function provided by block.c */
u32 __mmc_sd_num_wr_blocks(struct mmc_card *card);

/* Function provided by mmc/core/sd.c */
int mmc_sd_power_cycle(struct mmc_host *host, u32 ocr,
	struct mmc_card *card);

/* Function provided by mmc/core/bus.c */
void mmc_remove_card(struct mmc_card *card);

/* Function provided by drivers/irqchip/irq-mt-eic.c */
int mt_eint_get_polarity_external(unsigned int irq_num);

/* FIXME: mt_dump.c use those API,
 * ask pwrap owner to add these 2 functions to a header file,
 * shall be in <mach/mt_pmic_wrap.h>, but commented-out in that file */
#ifndef FPGA_PLATFORM
S32 pwrap_read_nochk(U32  adr, U32 *rdata);
S32 pwrap_write_nochk(U32  adr, U32  wdata);
#endif

#define MET_USER_EVENT_SUPPORT
/* #include <linux/met_drv.h> */
#if defined(FEATURE_MET_MMC_INDEX)
void met_mmc_issue(struct mmc_host *host, struct mmc_request *req);
#endif

int msdc_cache_ctrl(struct msdc_host *host, unsigned int enable,
	u32 *status);
#define check_mmc_cache_ctrl(card) \
	(card && mmc_card_mmc(card) && (card->ext_csd.cache_ctrl & 0x1))
#define check_mmc_cache_flush_cmd(cmd) \
	((cmd->opcode == MMC_SWITCH) && \
	 (((cmd->arg >> 16) & 0xFF) == EXT_CSD_FLUSH_CACHE) && \
	 (((cmd->arg >> 8) & 0x1)))
#define check_mmc_cmd2425(opcode) \
	((opcode == MMC_WRITE_MULTIPLE_BLOCK) || \
	 (opcode == MMC_WRITE_BLOCK))
#define check_mmc_cmd1718(opcode) \
	((opcode == MMC_READ_MULTIPLE_BLOCK) || \
	 (opcode == MMC_READ_SINGLE_BLOCK))

#define check_mmc_cmd1825(opcode) \
	((opcode == MMC_READ_MULTIPLE_BLOCK) || \
	 (opcode == MMC_WRITE_MULTIPLE_BLOCK))
#endif /* end of  MT_SD_H */
