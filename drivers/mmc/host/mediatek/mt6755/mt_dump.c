/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>        /* for mdely */
#include <asm/io.h>             /* __raw_readl */
#include <asm/arch_timer.h>
#include <linux/types.h>
#include <mt-plat/partition.h>

#include "mt_sd.h"
#include <mt-plat/sd_misc.h>
#include "dbg.h"

#ifdef MTK_MSDC_USE_CACHE
unsigned int g_power_reset;
#endif

MODULE_LICENSE("GPL");
/*--------------------------------------------------------------------------*/
/* head file define                                                         */
/*--------------------------------------------------------------------------*/
#define MTK_MMC_DUMP_DBG        (0)
#if MTK_MMC_DUMP_DBG
#define MTK_DUMP_PR_ERR(fmt, args...)   pr_err(fmt, ##args)
#define MTK_DUMP_PR_DBG(fmt, args...)   pr_debug(fmt, ##args)
#else
#define MTK_MMC_DUMP(fmt, args...)
#define MTK_DUMP_PR_ERR(fmt, args...)
#define MTK_DUMP_PR_DBG(fmt, args...)
#endif

#define MAX_POLLING_STATUS      (50000)

struct simp_mmc_card;
struct simp_msdc_host;
#define MSDC_PS_DAT0            (0x1  << 16)    /* R  */
struct simp_mmc_host {
	int                     index;
	unsigned int            f_min;
	unsigned int            f_max;
	unsigned int            f_init;
	u32                     ocr_avail;

	unsigned int            caps;           /* Host capabilities */
	unsigned int            caps2;          /* More host capabilities */

	/* host specific block data */
	unsigned int            max_seg_size;   /* blk_queue_max_segment_size */
	unsigned short          max_segs;       /* blk_queue_max_segments */
	unsigned short          unused;
	unsigned int            max_req_size;   /* maximum bytes in one req */
	unsigned int            max_blk_size;   /* maximum size of one block */
	unsigned int            max_blk_count;  /* maximum blocks in one req */
	unsigned int            max_discard_to; /* max. discard timeout in ms */

	u32                     ocr;            /* the current OCR setting */

	struct simp_mmc_card    *card;          /* card attached to this host */

	unsigned int            actual_clock;   /* Actual HC clock */

	/* add msdc struct */
	struct simp_msdc_host   *mtk_host;
};

struct simp_mmc_card {
	struct simp_mmc_host    *host;          /* card's belonging host */
	unsigned int            rca;            /* relative card address */
	unsigned int            type;           /* card type */
	unsigned int            state;          /* (our) card state */
	unsigned int            quirks;         /* card quirks */

	unsigned int            erase_size;     /* erase size in sectors */
	unsigned int            erase_shift;    /* if erase unit is power 2 */
	unsigned int            pref_erase;     /* in sectors */
	u8                      erased_byte;    /* value of erased bytes */

	u32                     raw_cid[4];     /* raw card CID */
	u32                     raw_csd[4];     /* raw card CSD */
	u32                     raw_scr[2];     /* raw card SCR */
	struct mmc_cid          cid;            /* card identification */
	struct mmc_csd          csd;            /* card specific */
	struct mmc_ext_csd      ext_csd;        /* MMC extended card specific */
	struct sd_scr           scr;            /* extra SD information */
	struct sd_ssr           ssr;            /* yet more SD information */
	struct sd_switch_caps   sw_caps;        /* switch (CMD6) caps */

	unsigned int            sd_bus_speed;   /* Current Bus Speed Mode */
};

struct simp_msdc_card {
	unsigned int            rca;            /* relative card address */
	unsigned int            type;           /* card type */
	unsigned short          state;          /* card state */
	unsigned short          file_system;    /* FAT16/FAT32 */
	unsigned short          card_cap;       /* High Capcity/standard */
};

struct simp_msdc_host {
	struct simp_msdc_card   *card;
	void __iomem            *base;          /* host base address */
	unsigned char           id;             /* host id number */
	unsigned int            clk;            /* host clock value from
						   clock source */
	unsigned int            sclk;           /* working SD clock speed */
	unsigned char           clksrc;         /* clock source */
	void                    *priv;          /* private data */
};

enum {
	FAT16 = 0,
	FAT32 = 1,
	exFAT = 2,
	_RAW_ = 3,
};

enum {
	standard_capacity = 0,
	high_capacity = 1,
	extended_capacity = 2,
};

/* command argument */
#define CMD0_ARG                (0)
#define CMD2_ARG                (0)
#define CMD3_ARG                (0)
#define CMD7_ARG                (phost->card->rca)
#define CMD8_ARG_VOL_27_36      (0x100)
#define CMD8_ARG_PATTERN        (0x5a)          /* or 0xAA */
#define CMD8_ARG                (CMD8_ARG_VOL_27_36 | CMD8_ARG_PATTERN)
#define CMD9_ARG                (phost->card->rca)
#define CMD10_ARG               (phost->card->rca)
#define CMD12_ARG               (0)
#define CMD13_ARG               (phost->card->rca)
#define CMD55_ARG               (phost->card->rca)

#define ACMD6_ARG_BUS_WIDTH_4   (0x2)
#define ACMD6_ARG               (ACMD6_ARG_BUS_WIDTH_4)

#define ACMD41_ARG_HCS          (1 << 30)
#define ACMD41_ARG_VOL_27_36    (0xff8000)
#define ACMD41_ARG_20           (ACMD41_ARG_VOL_27_36 | ACMD41_ARG_HCS)
#define ACMD41_ARG_10           (ACMD41_ARG_VOL_27_36)

#define ACMD6_ARG_EMMC          ((MMC_SWITCH_MODE_WRITE_BYTE << 24) \
					| (EXT_CSD_BUS_WIDTH << 16) \
					| (EXT_CSD_BUS_WIDTH_4 << 8) \
					| EXT_CSD_CMD_SET_NORMAL)

#define ACMD6_ARG_DISABLE_CACHE ((MMC_SWITCH_MODE_WRITE_BYTE << 24) \
					| (EXT_CSD_CACHE_CTRL << 16) \
					| (0 << 8) | EXT_CSD_CMD_SET_NORMAL)

#define CMD_RAW(cmd, rspt, dtyp, rw, len, stop) \
				((cmd) | (rspt << 7) | (dtyp << 11) | \
				 (rw << 13) | (len << 16) | (stop << 14))

#define CMD0_RAW                CMD_RAW(0 , msdc_rsp[RESP_NONE], 0, 0, 0, 0)
#define CMD1_RAW                CMD_RAW(1 , msdc_rsp[RESP_R3]  , 0, 0, 0, 0)
#define CMD2_RAW                CMD_RAW(2 , msdc_rsp[RESP_R2]  , 0, 0, 0, 0)
#define CMD3_RAW                CMD_RAW(3 , msdc_rsp[RESP_R1]  , 0, 0, 0, 0)
#define CMD7_RAW                CMD_RAW(7 , msdc_rsp[RESP_R1]  , 0, 0, 0, 0)
#define CMD8_RAW                CMD_RAW(8 , msdc_rsp[RESP_R7]  , 0, 0, 0, 0)
#define CMD8_RAW_EMMC           CMD_RAW(8 , msdc_rsp[RESP_R1]  , 1, 0, 512, 0)
#define CMD9_RAW                CMD_RAW(9 , msdc_rsp[RESP_R2]  , 0, 0, 0, 0)
#define CMD10_RAW               CMD_RAW(10, msdc_rsp[RESP_R2]  , 0, 0, 0, 0)
#define CMD12_RAW               CMD_RAW(12, msdc_rsp[RESP_R1B] , 0, 0,   0, 1)
#define CMD13_RAW               CMD_RAW(13, msdc_rsp[RESP_R1]  , 0, 0, 0, 0)
#define CMD17_RAW               CMD_RAW(17, msdc_rsp[RESP_R1]  , 1, 0, 512, 0)
#define CMD18_RAW               CMD_RAW(18, msdc_rsp[RESP_R1]  , 2, 0, 512, 0)
#define CMD24_RAW               CMD_RAW(24, msdc_rsp[RESP_R1]  , 1, 1, 512, 0)
#define CMD25_RAW               CMD_RAW(25, msdc_rsp[RESP_R1]  , 2, 1, 512, 0)
#define CMD55_RAW               CMD_RAW(55, msdc_rsp[RESP_R1]  , 0, 0, 0, 0)

#define ACMD6_RAW               CMD_RAW(6 , msdc_rsp[RESP_R1]  , 0, 0, 0, 0)
#define ACMD6_RAW_EMMC          CMD_RAW(6 , msdc_rsp[RESP_R1B] , 0, 0, 0, 0)
#define ACMD41_RAW              CMD_RAW(41, msdc_rsp[RESP_R3]  , 0, 0, 0, 0)

/* command response */
#define R3_OCR_POWER_UP_BIT        (1 << 31)
#define R3_OCR_CARD_CAPACITY_BIT   (1 << 30)

/* some marco will be reuse with mmc subsystem */
static int simp_mmc_init(int card_type, bool boot);
static int module_init_emmc;
static int module_init_sd;
static int emmc_init;
static int sd_init;

#define SIMP_SUCCESS            (0)
#define SIMP_FAILED             (-1)

/* the base address of sd card slot */
#define BOOT_STORAGE_ID         (0)
#define EXTEND_STORAGE_ID       (1)

#define BLK_LEN                 (512)
#define MAX_SCLK                (52000000)
#define NORMAL_SCLK             (25000000)
#define MIN_SCLK                (260000)

#define MAX_DMA_CNT             (64 * 1024 - 512)

#define CMD_WAIT_RETRY          (0x8FFFFFFF)

static struct simp_msdc_host g_msdc_host[2];
static struct simp_msdc_card g_msdc_card[2];
static struct simp_msdc_host *pmsdc_boot_host = &g_msdc_host[BOOT_STORAGE_ID];
static struct simp_msdc_host *pmsdc_extend_host =
	&g_msdc_host[EXTEND_STORAGE_ID];
static struct simp_mmc_host g_mmc_host[2];
static struct simp_mmc_card g_mmc_card[2];
static struct simp_mmc_host *pmmc_boot_host = &g_mmc_host[BOOT_STORAGE_ID];
static struct simp_mmc_host *pmmc_extend_host =
	&g_mmc_host[EXTEND_STORAGE_ID];

static void msdc_mdelay(u32 time)
{
	u64 t_start = 0, t_end = 0;

	t_start = arch_counter_get_cntpct();
	t_end = t_start + time * 1000 * 1000 / 77;
	while (t_end > arch_counter_get_cntpct())
		cpu_relax();
}

#define mt_dump_msdc_retry(expr, retry, cnt, id) \
	do { \
		int backup = cnt; \
		while (retry) { \
			if (!(expr))\
				break; \
			if (cnt-- == 0) { \
				retry--;\
				msdc_mdelay(1);\
				cnt = backup; \
			} \
		} \
		if (retry == 0) \
			simp_msdc_dump_info(id); \
		WARN_ON(retry == 0); \
	} while (0)

#define mt_dump_msdc_clr_fifo(id) \
	do { \
		int retry = 3, cnt = 1000; \
		MSDC_SET_BIT32(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
		mt_dump_msdc_retry(MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, \
			retry, cnt, id); \
	} while (0)

static void simp_msdc_dump_info(unsigned int id)
{
	void __iomem *base = g_msdc_host[id].base;

	msdc_dump_register_core(id, base);
	msdc_dump_clock_sts();
	msdc_dump_padctl_by_id(id);
	msdc_dump_dbg_register_core(id, base);
}

static void simp_msdc_config_clksrc(struct simp_msdc_host *host,
	unsigned int clksrc)
{
	u32 *hclks = msdc_get_hclks(host->id);
	host->clksrc = clksrc;
	host->clk = hclks[clksrc];
}

static void simp_msdc_config_clock(struct simp_msdc_host *host, unsigned int hz)
{
	/* struct msdc_hw *hw = host->priv; */
	void __iomem *base = host->base;
	u32 mode;  /* use divisor or not */
	u32 div = 0;
	u32 sclk;
	u32 hclk = host->clk;
	u32 orig_clksrc = host->clksrc;

	if (hz >= hclk) {
		mode = 0x1; /* no divisor */
		sclk = hclk;
	} else {
		mode = 0x0; /* use divisor */
		if (hz >= (hclk >> 1)) {
			div  = 0;         /* mean div = 1/2 */
			sclk = hclk >> 1; /* sclk = clk / 2 */
		} else {
			div  = (hclk + ((hz << 2) - 1)) / (hz << 2);
			sclk = (hclk >> 2) / div;
		}
	}
	host->sclk  = sclk;

	/* set clock mode and divisor */
	/*simp_msdc_config_clksrc(host, MSDC_CLKSRC_NONE);*/

	/* designer said: best way is wait clk stable while modify clk
	   config bit */
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD|MSDC_CFG_CKDIV,
		(mode << 12)|(div % 0xfff));

	simp_msdc_config_clksrc(host, orig_clksrc);

	/* wait clock stable */
	while (!(MSDC_READ32(MSDC_CFG) & MSDC_CFG_CKSTB))
		;
}

static void msdc_set_timeout(struct simp_msdc_host *host, u32 ns, u32 clks)
{
	void __iomem *base = host->base;
	u32 timeout, clk_ns;

	clk_ns = 1000000000UL / host->sclk;
	timeout = ns / clk_ns + clks;
	timeout = timeout >> 20;        /* in 2^20 sclk cycle unit */
	timeout = timeout > 1 ? timeout - 1 : 0;
	timeout = timeout > 255 ? 255 : timeout;

	MSDC_SET_FIELD(SDC_CFG, SDC_CFG_DTOC, timeout);
}

#ifndef FPGA_PLATFORM
/* Note: shall be in drivers/misc/mediatek/pmic_wrap/mt6755/pwrap_hal.h */
#ifdef MT6351_INT_STA
#undef MT6351_INT_STA /*to avoid build warning about redefining in upmu_hw.h*/
#endif
#include <pwrap_hal.h>

#define msdc_power_set_field(reg, field, val) \
	do {    \
		unsigned int tv;  \
		pwrap_read_nochk(reg, &tv);     \
		tv &= ~(field); \
		tv |= ((val) << (uffs((unsigned int)field) - 1)); \
		pwrap_write_nochk(reg, tv); \
	} while (0)

#define msdc_power_get_field(reg, field, val) \
	do {    \
		unsigned int tv;  \
		pwrap_read_nochk(reg, &tv);     \
		val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
	} while (0)
#endif

static unsigned int simp_mmc_power_up(struct simp_mmc_host *host, bool on)
{
	MTK_DUMP_PR_ERR("[%s]: on=%d, start\n", __func__, on);

#ifdef FPGA_PLATFORM
	msdc_fpga_power(mtk_msdc_host[host->mtk_host->id], on);
#else
	switch (host->mtk_host->id) {
	case 0:
		if (on) {
			msdc_power_set_field(REG_VEMC_VOSEL,
				FIELD_VEMC_VOSEL, VEMC_VOSEL_3V);
			msdc_power_set_field(REG_VEMC_EN, FIELD_VEMC_EN, 0x1);
		} else {
			msdc_power_set_field(REG_VEMC_EN, FIELD_VEMC_EN, 0x0);
		}
		break;
	case 1:
		if (on) {
			msdc_power_set_field(REG_VMC_VOSEL, FIELD_VMC_VOSEL,
				VMC_VOSEL_3V);
			msdc_power_set_field(REG_VMC_EN, FIELD_VMC_EN, 0x1);
			msdc_power_set_field(REG_VMCH_VOSEL, FIELD_VMCH_VOSEL,
				VMCH_VOSEL_3V);
			msdc_power_set_field(REG_VMCH_EN, FIELD_VMCH_EN, 0x1);
		} else{
			msdc_power_set_field(REG_VMC_EN, FIELD_VMC_EN, 0x0);
			msdc_power_set_field(REG_VMCH_EN, FIELD_VMCH_EN, 0x0);
		}
		break;
	default:
		break;
	}
#endif

	MTK_DUMP_PR_ERR("[%s]: on=%d, end\n", __func__, on);

	return SIMP_SUCCESS;
}

#define clk_readl(addr) \
	MSDC_READ32(addr)

#define clk_setl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) | (val), addr)

#define clk_clrl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

static unsigned int simp_mmc_enable_clk(struct simp_mmc_host *host)
{
#ifndef FPGA_PLATFORM
	/*check base, cause KE too early to initial it at msdc_drv_probe*/
	if (NULL == apmixed_reg_base || NULL == topckgen_reg_base
		|| NULL == infracfg_ao_reg_base) {
		pr_err("apmixed_reg_base=%p, topckgen_reg_base=%p, infracfg_ao_reg_base=%p\n",
			apmixed_reg_base, topckgen_reg_base,
			infracfg_ao_reg_base);
		return SIMP_FAILED;
	}

	/* step1: open pll */
	clk_setl(apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET, 0x3);
	msdc_mdelay(1);
	clk_setl(apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET, 0x1);

	clk_setl(apmixed_reg_base + MSDCPLL_CON1_OFFSET, 0x80000000);

	clk_setl(apmixed_reg_base + MSDCPLL_CON0_OFFSET, 0x1);
	msdc_mdelay(1);

	/* step2: enable mux */
	mt_reg_sync_writel(0x07010100,
		topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET);

	mt_reg_sync_writel(0x01000107,
		topckgen_reg_base + MSDC_CLK_CFG_4_OFFSET);

	mt_reg_sync_writel(0x00006000,
		topckgen_reg_base + MSDC_CLK_UPDATE_OFFSET);

	mt_reg_sync_writel(0xFFFFFFFF,
		infracfg_ao_reg_base + MSDC_INFRA_PDN_CLR1_OFFSET);

#if MTK_MMC_DUMP_DBG
	msdc_dump_clock_sts();
#endif

#endif
	return SIMP_SUCCESS;
}

static unsigned int simp_mmc_hw_reset_for_init(struct simp_mmc_host *host)
{
	void __iomem *base;

	base = host->mtk_host->base;
	if (0 == host->mtk_host->id) {
		/* check emmc card support HW Rst_n or not is better way.
		 * but if the card does not support it , here just failed.
		 *     if the card support it, Rst_n function enable under DA
		 *      driver, see SDMMC_Download_BL_PostProcess_Internal() */
		/* >=1ms to trigger emmc enter pre-idle state */
		MSDC_SET_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);
		msdc_mdelay(1);
		MSDC_CLR_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);

		/* clock is need after Rst_n pull high, and the card need
		 * clock to calculate time for tRSCA, tRSTH */
		MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);
		msdc_mdelay(1);

		/* not to close, enable clock free run under mt_dump */
		/*MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_CKPDNT);*/
	}

	return SIMP_SUCCESS;
}

static void simp_msdc_dma_stop(struct simp_msdc_host *host)
{
	void __iomem *base = host->base;
	int retry = 500;
	int count = 1000;
	MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
	mt_dump_msdc_retry((MSDC_READ32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS),
		retry, count, host->id);
	if (retry == 0) {
		pr_err("#### Failed to stop DMA! please check it here ####");
		/*BUG();*/
	}
}

static unsigned int simp_msdc_init(struct simp_mmc_host *mmc_host)
{
	unsigned int ret = 0;
	void __iomem *base;
	struct simp_msdc_host *host = mmc_host->mtk_host;

	/*struct msdc_hw *hw;*/
	/* Todo1: struct msdc_hw in board.c */

	base = host->base;

	/* set to SD/MMC mode, the first step while operation msdc */
	MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_MODE, 1); /* MSDC_SDMMC */

	/* reset controller */
	msdc_reset(host->id);

	sd_debug_zone[host->id] = DBG_EVT_ALL;
	simp_msdc_dma_stop(host);

	/* clear FIFO */
	mt_dump_msdc_clr_fifo(host->id);

	/* Disable & Clear all interrupts */
	msdc_clr_int();
	MSDC_WRITE32(MSDC_INTEN, 0);

	/* reset tuning parameter */
	MSDC_WRITE32(MSDC_PAD_TUNE0,  0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY0, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY1, 0x00000000);
	MSDC_WRITE32(MSDC_IOCON,      0x00000000);
	MSDC_WRITE32(MSDC_PATCH_BIT0, 0x003C000F);

	/* PIO mode */
	MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO);

	/* sdio + inswkup*/
	MSDC_SET_BIT32(SDC_CFG, SDC_CFG_SDIO);
	MSDC_SET_BIT32(SDC_CFG, SDC_CFG_INSWKUP);

	/* disable async fifo use interl delay*/
	MSDC_CLR_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_RXDLYSEL);
	MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
	MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP);

	/* disable support 64G */
	MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_SUPPORT64G);

	/* set Driving SMT PUPD */

	msdc_set_driving_by_id(host->id, mtk_msdc_host[host->id]->hw, 0);

	msdc_set_smt_by_id(host->id, 1);
	msdc_pin_config_by_id(host->id, MSDC_PIN_PULL_UP);

	/* set sampling edge */
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, 0); /* rising: 0 */
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, 0);

	/* write crc timeout detection */
	MSDC_SET_FIELD(MSDC_PATCH_BIT0, 1 << 30, 1);

	/* Clock source select*/
	simp_msdc_config_clksrc(host, host->clksrc);

	/* Bus width to 1 bit*/
	MSDC_SET_FIELD(SDC_CFG, SDC_CFG_BUSWIDTH, 0);

	/* make sure the clock is 260K */
	simp_msdc_config_clock(host, MIN_SCLK);

	/* Set Timeout 100ms*/
	msdc_set_timeout(host, 100000000, 0);

	MSDC_CLR_BIT32(MSDC_PS, MSDC_PS_CDEN);

	/* detect card */
	/* need to check card is insert [Fix me] */

	/* check write protection [Fix me] */

#if 0
	/* simple test for clk output */
	MSDC_WRITE32(MSDC_PATCH_BIT0, 0xF3F);
	MSDC_WRITE32(MSDC_CFG, 0x10001013);
	MSDC_WRITE32(SDC_CFG, 0x0);
	MSDC_WRITE32(SDC_CMD, 0x0);
	MSDC_WRITE32(SDC_ARG, 0x0);

	/* dump register for debug */
	simp_msdc_dump_info(host->id);
#endif


	return ret;
}


static void simp_mmc_set_bus_mode(struct simp_mmc_host *host, unsigned int mode)
{
	/* mtk: msdc not support to modify bus mode */

}

/* =======================something for msdc cmd/data */
u8 simp_ext_csd[512];
static int simp_msdc_cmd(struct simp_msdc_host *host,
	unsigned int cmd, unsigned int raw,
	unsigned int arg, int rsptyp, unsigned int *resp)
{
	int retry = 5000; /*CMD_WAIT_RETRY; */
	void __iomem *base = host->base;
	unsigned int error = 0;
	unsigned int intsts = 0;
	unsigned int cmdsts =
		MSDC_INT_CMDRDY | MSDC_INT_CMDTMO | MSDC_INT_RSPCRCERR;

	/* wait before send command */
	if (cmd == MMC_SEND_STATUS) {
		while (retry--) {
			if (!sdc_is_cmd_busy())
				break;
			msdc_mdelay(1);
		}
		if (retry == 0) {
			error = 1;
			goto end;
		}
	} else {
		while (retry--) {
			if (!sdc_is_busy())
				break;
			msdc_mdelay(1);
		}
		if (retry == 0) {
			error = 2;
			goto end;
		}
	}

	sdc_send_cmd(raw, arg);

	MTK_DUMP_PR_ERR("cmd=0x%x, arg=0x%x\n", raw, arg);

	/* polling to check the interrupt */
	retry = 5000; /*CMD_WAIT_RETRY;*/
	while ((intsts & cmdsts) == 0) {
		intsts = MSDC_READ32(MSDC_INT);
		retry--;
#if MTK_MMC_DUMP_DBG
		if (retry % 1000 == 0) {
			pr_debug("int cmd=0x%x, arg=0x%x, retry=0x%x, intsts=0x%x\n",
				raw, arg, retry, intsts);
			simp_msdc_dump_info(host->id);
		}
#endif
		if (retry == 0) {
			error = 3;
			goto end;
		}
		msdc_mdelay(1);
	}

	intsts &= cmdsts;
	MSDC_WRITE32(MSDC_INT, intsts); /* clear interrupts */

	if (intsts & MSDC_INT_CMDRDY) {
		/* get the response */
		switch (rsptyp) {
		case RESP_NONE:
			break;
		case RESP_R2:
			*resp++ = MSDC_READ32(SDC_RESP3);
			*resp++ = MSDC_READ32(SDC_RESP2);
			*resp++ = MSDC_READ32(SDC_RESP1);
			*resp = MSDC_READ32(SDC_RESP0);
			break;
		default:        /* Response types 1, 3, 4, 5, 6, 7(1b) */
			*resp = MSDC_READ32(SDC_RESP0);
		}

		MTK_DUMP_PR_DBG("msdc cmd<%d> arg<0x%x> resp<0x%x>Ready \r\n",
			cmd, arg, *resp);

	} else {
		error = 4;
		goto end;
	}

	if (rsptyp == RESP_R1B) {
		retry = 9999;
		while ((MSDC_READ32(MSDC_PS) & MSDC_PS_DAT0) != MSDC_PS_DAT0) {
			retry--;
			if (retry % 5000 == 0) {
				pr_debug("int cmd=0x%x, arg=0x%x, retry=0x%x, intsts=0x%x\n",
					raw, arg, retry, intsts);
				simp_msdc_dump_info(host->id);
			}
			if (retry == 0) {
				error = 5;
				goto end;
			}
			msdc_mdelay(1);
		}

		MTK_DUMP_PR_DBG("msdc cmd<%d> done \r\n", cmd);

	}

end:
	if (error) {
		pr_err("cmd:%d, arg:0x%x, error=%d, intsts=0x%x\n",
			cmd, arg, error, intsts);
		simp_msdc_dump_info(host->id);
	}

	return error;
}

/* ======================= */

static int simp_mmc_go_idle(struct simp_mmc_host *host)
{
	int err = 0;
	unsigned int resp = 0;
	struct simp_msdc_host *phost = host->mtk_host;

	err = simp_msdc_cmd(phost, MMC_GO_IDLE_STATE, CMD0_RAW, CMD0_ARG,
		RESP_NONE, &resp);

	return err;
}

static unsigned int simp_mmc_get_status(struct simp_mmc_host *host,
	unsigned int *status)
{
	unsigned int resp = 0;
	unsigned int err = 0;
	struct simp_msdc_host *phost = host->mtk_host;
	unsigned int rca = 0;

#ifdef MTK_MSDC_USE_CACHE
	if (g_power_reset)
		rca = phost->card->rca << 16;
	else
		rca = mtk_msdc_host[host->index]->mmc->card->rca << 16;
#else
	rca = phost->card->rca << 16;
#endif

	/* pr_debug("rca=0x%x, mtk_msdc_host[%d]->mmc->card->rca=0x%x,
	 phost->card->rca=0x%x\n", rca, host->index,
	  mtk_msdc_host[host->index]->mmc->card->rca, phost->card->rca); */
	err = simp_msdc_cmd(phost, MMC_SEND_STATUS, CMD13_RAW, rca, RESP_R1,
		&resp);

	*status = resp;

	return err;
}

static unsigned int simp_mmc_send_stop(struct simp_mmc_host *host)
{
	unsigned int resp = 0;
	unsigned int err = 0;
	struct simp_msdc_host *phost = host->mtk_host;

	/* send command */
	err = simp_msdc_cmd(phost, MMC_STOP_TRANSMISSION, CMD12_RAW, 0,
		RESP_R1B, &resp);

	return err;
}

static int simp_mmc_send_op_cond(struct simp_mmc_host *host,
	unsigned int ocr, unsigned int *rocr)
{
	int err = 0, i;
	unsigned int resp = 0;
	struct simp_msdc_host *phost = host->mtk_host;

	for (i = 500; i; i--) {
		err = simp_msdc_cmd(phost, MMC_SEND_OP_COND, CMD1_RAW, ocr,
			RESP_R3, &resp);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (resp & MMC_CARD_BUSY)
			break;

		err = -ETIMEDOUT;

		msdc_mdelay(10);
	}

	if (rocr)
		*rocr = resp;

	if (i <= 400)
		pr_err("cmd1: resp(0x%x), i=%d\n", resp, i);

	return err;
}

static int simp_mmc_all_send_cid(struct simp_mmc_host *host, unsigned int *cid)
{
	int err = 0;

	err = simp_msdc_cmd(host->mtk_host, MMC_ALL_SEND_CID, CMD2_RAW,
		CMD2_ARG, RESP_R2, cid);

	MTK_DUMP_PR_DBG("cidp: 0x%x 0x%x 0x%x 0x%x\n",
		cid[0], cid[1], cid[2], cid[3]);

	return err;
}

#if 0
static int simp_mmc_set_relative_addr(struct simp_mmc_card *card)
{
	int err;
	unsigned int resp;

	err = simp_msdc_cmd(card->host->mtk_host, MMC_SET_RELATIVE_ADDR,
		CMD3_RAW, card->rca << 16, RESP_R1, &resp);

	return err;
}
#endif

#if 0
static int simp_mmc_send_csd(struct simp_mmc_card *card, unsigned int *csd)
{
	int err;
	unsigned int resp[4] = { 0 };

	err = simp_msdc_cmd(card->host->mtk_host, MMC_SEND_CSD, CMD9_RAW,
		card->rca << 16, RESP_R2, resp);

	memcpy(csd, resp, sizeof(u32) * 4);

	return err;
}
#endif

/*
static const unsigned int tran_exp[] = {
	10000, 100000, 1000000, 10000000,
	0, 0, 0, 0
};

static const unsigned char tran_mant[] = {
	0, 10, 12, 13, 15, 20, 25, 30,
	35, 40, 45, 50, 55, 60, 70, 80,
};

static const unsigned int tacc_exp[] = {
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0, 10, 12, 13, 15, 20, 25, 30,
	35, 40, 45, 50, 55, 60, 70, 80,
};*/

static int simp_mmc_decode_csd(struct simp_mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	/*
	unsigned int e, m, a, b;
	*/
	u32 *resp = card->raw_csd;

	/*
	 * We only understand CSD structure v1.1 and v1.2.
	 * v1.2 has extra information in bits 15, 11 and 10.
	 * We also support eMMC v4.4 & v4.41.
	 */
	csd->structure = UNSTUFF_BITS(resp, 126, 2);
	if (csd->structure == 0) {
		pr_err("unrecognised CSD structure version %d\n",
			csd->structure);
		return -EINVAL;
	}

	csd->mmca_vsn = UNSTUFF_BITS(resp, 122, 4);

/*
	m = UNSTUFF_BITS(resp, 115, 4);
	e = UNSTUFF_BITS(resp, 112, 3);
	csd->tacc_ns = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
	csd->tacc_clks = UNSTUFF_BITS(resp, 104, 8) * 100;

	m = UNSTUFF_BITS(resp, 99, 4);
	e = UNSTUFF_BITS(resp, 96, 3);
	csd->max_dtr = tran_exp[e] * tran_mant[m];
	csd->cmdclass = UNSTUFF_BITS(resp, 84, 12);

	e = UNSTUFF_BITS(resp, 47, 3);
	m = UNSTUFF_BITS(resp, 62, 12);
	csd->capacity = (1 + m) << (e + 2);

	csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
	csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
	csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
	csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
	csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
	csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
	csd->write_partial = UNSTUFF_BITS(resp, 21, 1);

	if (csd->write_blkbits >= 9) {
		a = UNSTUFF_BITS(resp, 42, 5);
		b = UNSTUFF_BITS(resp, 37, 5);
		csd->erase_size = (a + 1) * (b + 1);
		csd->erase_size <<= csd->write_blkbits - 9;
	}
*/

	return 0;
}

static int simp_mmc_select_card(struct simp_mmc_host *host,
	struct simp_mmc_card *card)
{
	int err;
	unsigned int resp;
	struct simp_msdc_host *phost = host->mtk_host;

	err = simp_msdc_cmd(phost, MMC_SELECT_CARD, CMD7_RAW, card->rca << 16,
		RESP_R1, &resp);

	return 0;
}

/*
 * Mask off any voltages we don't support and select
 * the lowest voltage
 */
static unsigned int simp_mmc_select_voltage(struct simp_mmc_host *host,
	unsigned int ocr)
{
#if 0
	int bit;

	ocr &= host->ocr_avail;

	bit = ffs(ocr);
	if (bit) {
		bit -= 1;

		ocr &= 3 << bit;

		mmc_host_clk_hold(host);
		host->ios.vdd = bit;
		mmc_set_ios(host);
		mmc_host_clk_release(host);
	} else {
		pr_warn("%s: host doesn't support card's voltages\n",
			mmc_hostname(host));
		ocr = 0;
	}
#endif

	return ocr;
}

static int simp_msdc_pio_read(struct simp_msdc_host *host,
	unsigned int *ptr, unsigned int size);
static void simp_msdc_set_blknum(struct simp_msdc_host *host,
	unsigned int blknum);

static int simp_mmc_read_ext_csd(struct simp_mmc_host *host,
	struct simp_mmc_card *card)
{
	int err = 0;
	unsigned int resp;
	struct simp_msdc_host *phost = host->mtk_host;
	void __iomem *base = phost->base;
	memset(simp_ext_csd, 0, 512);
	if (card->csd.mmca_vsn < CSD_SPEC_VER_4) {
		pr_debug("MSDC MMCA_VSN: %d. Skip EXT_CSD\n",
			card->csd.mmca_vsn);
		return 0;
	}
	mt_dump_msdc_clr_fifo(host->mtk_host->id);
	simp_msdc_set_blknum(phost, 1);
	msdc_set_timeout(phost, 100000000, 0);

	err = simp_msdc_cmd(phost, MMC_SEND_EXT_CSD, CMD8_RAW_EMMC, 0, RESP_R1,
		&resp);
	if (err)
		goto out;

	err = simp_msdc_pio_read(phost, (unsigned int *)(simp_ext_csd), 512);
	if (err) {
		pr_err("pio read ext csd error(0x%d)\n", err);
		goto out;
	}

 out:
	return err;
}

/* FIXME : consider to remove it since card->ext_csd.sectors
	   is never referenced */
static void simp_mmc_decode_ext_csd(struct simp_mmc_card *card)
{
	card->ext_csd.sectors =
		simp_ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
		simp_ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
		simp_ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
		simp_ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
}

static int simp_emmc_switch_bus(struct simp_mmc_host *host,
	struct simp_mmc_card *card)
{
	struct simp_msdc_host *phost = host->mtk_host;
	unsigned int resp;
	return simp_msdc_cmd(phost, MMC_SWITCH, ACMD6_RAW_EMMC,
		ACMD6_ARG_EMMC, RESP_R1B, &resp);
}

static int simp_mmc_init_card(struct simp_mmc_host *mmc, unsigned int ocr,
	struct simp_mmc_card *oldcard)
{
	int err = 0;
	unsigned int rocr;
	unsigned int cid[4];
	void __iomem *base;
	struct simp_mmc_card *card = mmc->card;

	struct simp_msdc_host *phost = mmc->mtk_host;
	unsigned int resp[4] = {0};

	base = mmc->mtk_host->base;

	/* Set correct bus mode for MMC before attempting init */
	/* NULL func now */
	simp_mmc_set_bus_mode(mmc, MMC_BUSMODE_OPENDRAIN);

	/*
	 * Since we're changing the OCR value, we seem to
	 * need to tell some cards to go back to the idle
	 * state.  We wait 1ms to give cards time to
	 * respond.
	 * mmc_go_idle is needed for eMMC that are asleep
	 */
	simp_mmc_go_idle(mmc);

	/* The extra bit indicates that we support high capacity */
	err = simp_mmc_send_op_cond(mmc, ocr | (1 << 30), &rocr);
	if (err)
		goto err;

	err = simp_mmc_all_send_cid(mmc, cid);

	if (err)
		goto err;

	card->type = MSDC_EMMC;
	card->rca = 1;
	mmc->mtk_host->card->rca = 1;
	memcpy(card->raw_cid, cid, sizeof(card->raw_cid));

	/*
	 * For native busses:  set card RCA and quit open drain mode.
	 */
	#if 0
	err = simp_mmc_set_relative_addr(card);
	#else
	err = simp_msdc_cmd(phost, MMC_SET_RELATIVE_ADDR, CMD3_RAW,
		card->rca << 16, RESP_R1, resp);
	#endif
	if (err)
		goto err;

	simp_mmc_set_bus_mode(mmc, MMC_BUSMODE_PUSHPULL);

	/*
	 * Fetch CSD from card.
	 */
	#if 0
	err = simp_mmc_send_csd(card, card->raw_csd);
	#else
	err = simp_msdc_cmd(mmc->mtk_host, MMC_SEND_CSD, CMD9_RAW,
		card->rca << 16, RESP_R2, card->raw_csd);
	#endif
	if (err)
		goto err;

	err = simp_mmc_decode_csd(card);
	if (err)
		goto err;

	err = simp_mmc_select_card(mmc, card);
	if (err)
		goto err;
	err = simp_mmc_read_ext_csd(mmc, card);
	if (err)
		goto err;
	simp_mmc_decode_ext_csd(card);

	err = simp_emmc_switch_bus(mmc, card);
	MSDC_SET_FIELD(SDC_CFG, SDC_CFG_BUSWIDTH, 1);   /* 1: 4 bits mode */
	simp_msdc_config_clock(mmc->mtk_host, NORMAL_SCLK);
	return SIMP_SUCCESS;

 err:
	return SIMP_FAILED;
}

#define ACMD41_RETRY   (20)
static int simp_mmc_sd_init(struct simp_mmc_host *host)
{
	struct simp_msdc_host *phost = host->mtk_host;
	u32 ACMD41_ARG = 0;
	u8 retry;
	void __iomem *base;
	unsigned int resp[4] = {0};
	int bRet = 0;

	base = phost->base;
	simp_mmc_go_idle(host);

	if (simp_msdc_cmd(phost, SD_SEND_IF_COND, CMD8_RAW, CMD8_ARG, RESP_R7,
		resp)) {
		/* SD v1.0 will not repsonse to CMD8, then clr HCS bit */
		/*pr_debug("SD v1.0, clr HCS bit\n");*/
		ACMD41_ARG = ACMD41_ARG_10;
	} else if (resp[0] == CMD8_ARG) {
		/*pr_debug("SD v2.0, set HCS bit\n");*/
		ACMD41_ARG = ACMD41_ARG_20;
	}

	retry = ACMD41_RETRY;
	while (retry--) {
		if (simp_msdc_cmd(phost, MMC_APP_CMD, CMD55_RAW,
			CMD55_ARG << 16, RESP_R1, resp))
			goto EXIT;
		if (simp_msdc_cmd(phost, SD_APP_OP_COND, ACMD41_RAW, ACMD41_ARG,
			RESP_R3, resp))
			goto EXIT;
		if (resp[0] & R3_OCR_POWER_UP_BIT) {
			phost->card->card_cap =
				((resp[0] & R3_OCR_CARD_CAPACITY_BIT)
				? high_capacity : standard_capacity);
			if (phost->card->card_cap == standard_capacity)
				pr_debug("Standard_capacity card!!\r\n");
			break;
		}
		msdc_mdelay(1000 / ACMD41_RETRY);
	}

	if (simp_mmc_all_send_cid(host, resp))
		goto EXIT;

	if (simp_msdc_cmd(phost, MMC_SET_RELATIVE_ADDR, CMD3_RAW, CMD3_ARG,
		RESP_R6, resp))
		goto EXIT;

	/* save the rca */
	phost->card->rca = (resp[0] & 0xffff0000) >> 16;   /* RCA[31:16] */

	if (simp_msdc_cmd(phost, MMC_SEND_CSD, CMD9_RAW, CMD9_ARG << 16,
		RESP_R2, resp))
		goto EXIT;

	if (simp_msdc_cmd(phost, MMC_SEND_STATUS, CMD13_RAW, CMD13_ARG << 16,
		RESP_R1, resp))
		goto EXIT;

	if (simp_msdc_cmd(phost, MMC_SELECT_CARD, CMD7_RAW, CMD7_ARG << 16,
		RESP_R1, resp))
		goto EXIT;

	/* dump register for debug */
	/*simp_msdc_dump_info(phost->id)*/

	msdc_mdelay(10);

	if (simp_msdc_cmd(phost, MMC_APP_CMD, CMD55_RAW, CMD55_ARG << 16,
		RESP_R1, resp))
		goto EXIT;

	if (simp_msdc_cmd(phost, SD_SWITCH, ACMD6_RAW, ACMD6_ARG, RESP_R1,
		resp))
		goto EXIT;

	/* set host bus width to 4 */
	MSDC_SET_FIELD(SDC_CFG, SDC_CFG_BUSWIDTH, 1);   /* 1: 4 bits mode */
	simp_msdc_config_clock(phost, NORMAL_SCLK);

	MTK_DUMP_PR_DBG("sd card inited\n");

	bRet = 1;

 EXIT:
	return bRet;
}

#ifdef MTK_MSDC_USE_CACHE
static int simp_mmc_disable_cache(struct simp_mmc_host *host)
{
	int err = 0;
	unsigned int resp;
	unsigned int status = 0;
	int polling = MAX_POLLING_STATUS;
	struct simp_msdc_host *phost = host->mtk_host;

	do {
		err = simp_mmc_get_status(host, &status);
		if (err)
			return -1;

		if (R1_CURRENT_STATE(status) == 5
			|| R1_CURRENT_STATE(status) == 6) {
			simp_mmc_send_stop(host);
		}
		/* msdc_mdelay(1); */
	} while (R1_CURRENT_STATE(status) == 7 && polling--);

	if (R1_CURRENT_STATE(status) == 7)
		return -2;

	err = simp_msdc_cmd(phost, MMC_SWITCH, ACMD6_RAW_EMMC,
		ACMD6_ARG_DISABLE_CACHE, RESP_R1B, &resp);

	if (!err) {
		polling = MAX_POLLING_STATUS;
		do {
			err = simp_mmc_get_status(host, &status);
			if (err)
				return -3;
			/*msdc_mdelay(1); */
		} while (R1_CURRENT_STATE(status) == 7 && polling--);

		if (status & 0xFDFFA000)
			pr_err("msdc unexpected status 0x%x after switch",
				status);
		if (status & R1_SWITCH_ERROR)
			return -4;
	}

	return err;
}
#endif

static unsigned int simple_mmc_attach_sd(struct simp_mmc_host *host)
{
	/* int err = SIMP_FAILED; */

	/* power up host */
	simp_mmc_power_up(host, 0);
	msdc_mdelay(20);
	simp_mmc_power_up(host, 1);

	/* enable clock */
	if (SIMP_FAILED ==  simp_mmc_enable_clk(host))
		return SIMP_FAILED;

	/*
	 * Some eMMCs (with VCCQ always on) may not be reset after power up, so
	 * do a hardware reset if possible.
	 */
	simp_mmc_hw_reset_for_init(host);

	/* init msdc host */
	simp_msdc_init(host);

	simp_mmc_sd_init(host);

	return SIMP_SUCCESS;
}

/* make clk & power always on */
static unsigned int simple_mmc_attach_mmc(struct simp_mmc_host *host)
{
	int err = 0;
	unsigned int ocr;

#ifdef MTK_MSDC_USE_CACHE
	struct msdc_host *host2 = mtk_msdc_host[host->index];

	g_power_reset = 0;
	/* turn off cache will trigger flushing of cache data to
	   non-volatile storage */
	if (!host2 || !host2->mmc || !host2->mmc->card) {
		pr_err("[%s]: host/mmc/card does not exist\n", __func__);
	} else if (host2->mmc->card->ext_csd.cache_ctrl & 0x1) {
		/* enable clock */
		if (SIMP_FAILED ==  simp_mmc_enable_clk(host))
			return SIMP_FAILED;

		/* init msdc host */
		simp_msdc_init(host);

		err = simp_mmc_disable_cache(host);
		if (err) {
			pr_err("[%s]: failed to disable cache ops, err = %d\n",
				__func__, err);
			simp_msdc_dump_info(host->mtk_host->id);
			err = 0;
		} else {
			MTK_DUMP_PR_DBG("[%s]: successfully disabled cache\n",
				__func__);
		}
	} else {
		MTK_DUMP_PR_DBG("[%s]: cache is not enabled, no need to disable\n",
			__func__);
	}
#endif

	/* power up host */
	simp_mmc_power_up(host, 1);
	msdc_mdelay(10);
#ifdef MTK_MSDC_USE_CACHE
	g_power_reset = 1;
#endif

	/*
	 * Some eMMCs (with VCCQ always on) may not be reset after power up, so
	 * do a hardware reset if possible.
	 */
	simp_mmc_hw_reset_for_init(host);

	/* enable clock */
	if (SIMP_FAILED ==  simp_mmc_enable_clk(host))
		return SIMP_FAILED;

	/* init msdc host */
	simp_msdc_init(host);

	/*=================== begin to init emmc card =======================*/

	/* Set correct bus mode for MMC before attempting attach */
	simp_mmc_set_bus_mode(host, MMC_BUSMODE_OPENDRAIN);

	simp_mmc_go_idle(host);

	err = simp_mmc_send_op_cond(host, 0, &ocr);

	/*=========ok*/

	/*
	 * Sanity check the voltages that the card claims to
	 * support.
	 */
	if (ocr & 0x7F) {
		MTK_DUMP_PR_DBG("msdc0: card claims to support voltages below defined range --> ignored.\n");
		ocr &= ~0x7F;
	}

	host->ocr = simp_mmc_select_voltage(host, ocr);

	/*
	 * Can we support the voltage of the card?
	 */
	if (!host->ocr) {
		pr_err("msdc0: card voltage not support\n");
		err = -EINVAL;
		goto err;
	}

	/*
	 * Detect and init the card.
	 */
	err = simp_mmc_init_card(host, host->ocr, NULL);
	if (err == SIMP_FAILED) {
		pr_err("init eMMC failed\n");
		goto err;
	}

	MTK_DUMP_PR_DBG("init eMMC success\n");

	/*=================== end mmc card init =============================*/
	return SIMP_SUCCESS;
 err:
	return SIMP_FAILED;
}

static unsigned int simp_init_try(int card_type, int *module_init_flag,
	int *dev_init_flag)
{
	int i = 0;
	int ret = 0;
	int boot;
	static struct simp_mmc_host *cur_mmc_host;
	char *type_str;

	if (card_type == MSDC_EMMC) {
		cur_mmc_host = &g_mmc_host[BOOT_STORAGE_ID];
		boot = 1;
		type_str = "eMMC";
	} else if (card_type == MSDC_SD) {
		cur_mmc_host = &g_mmc_host[EXTEND_STORAGE_ID];
		boot = 0;
		type_str = "SD";
	} else
		return 1;

	pr_err("Start init %s\n", type_str);
	if (0 == *module_init_flag) {
		if (SIMP_FAILED == simp_mmc_init(card_type, boot)) {
			ret = 1;
			goto out;
		}
	}

	for (i = 0; i < 3; i++) {
		if (card_type == MSDC_EMMC)
			ret = simple_mmc_attach_mmc(cur_mmc_host);
		else
			ret = simple_mmc_attach_sd(cur_mmc_host);
		if (ret == SIMP_SUCCESS)
			break;
	}
	if (0 == ret)
		*dev_init_flag = 1;

	pr_err("%s to init %s\n", (ret ? "Fail" : "Succeed"), type_str);
out:
	return ret;
}

unsigned int reset_boot_up_device(int type)
{
	int ret = 0;

	if (type == MMC_TYPE_MMC)
		ret = simp_init_try(MSDC_EMMC, &module_init_emmc, &emmc_init);
	else if (type == MMC_TYPE_SD)
		ret = simp_init_try(MSDC_SD, &module_init_sd, &sd_init);
	else {
		pr_err("invalid card type: %d\n", type);
		ret = 1;
	}

	return ret;
}
EXPORT_SYMBOL(reset_boot_up_device);

static int simp_msdc_pio_write(struct simp_msdc_host *host,
	unsigned int *ptr, unsigned int size)
{
	void __iomem *base = host->base;
	unsigned int left = size;
	unsigned int status = 0;
	unsigned char *u8ptr;
	int l_count = 0;
	int err = 0;
	int print_count = 2;

	while (1) {
		status = MSDC_READ32(MSDC_INT);
		MSDC_WRITE32(MSDC_INT, status);
		if (status & MSDC_INT_DATCRCERR) {
			pr_err("[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",
				host->id, status, left);
			err = -5;
			simp_msdc_dump_info(host->id);
			break;
		} else if (status & MSDC_INT_DATTMO) {
			pr_err("[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",
				host->id, status, left);
			err = -110;
			simp_msdc_dump_info(host->id);
			break;
		} else if (status & MSDC_INT_XFER_COMPL) {
			break;
		}
		if (left == 0)
			continue;
		if ((left >= MSDC_FIFO_SZ) && (msdc_txfifocnt() == 0)) {
			int count = MSDC_FIFO_SZ >> 2;
			do {
				msdc_fifo_write32(*ptr);
				ptr++;
			} while (--count);
			left -= MSDC_FIFO_SZ;
		} else if (left < MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
			while (left > 3) {
				msdc_fifo_write32(*ptr); ptr++;
				left -= 4;
			}

			u8ptr = (u8 *)ptr;
			while (left) {
				msdc_fifo_write8(*u8ptr);
				u8ptr++;
				left--;
			}
		} else {
			status = MSDC_READ32(MSDC_INT);

			if (status & MSDC_INT_DATCRCERR) {
				pr_err("[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",
					host->id, status, left);
				err = -5;
			}
			if (status & MSDC_INT_DATTMO) {
				pr_err("[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",
					host->id, status, left);
				err = -110;
			}

			if ((status & MSDC_INT_DATCRCERR)
				|| (status & MSDC_INT_DATTMO)) {
				simp_msdc_dump_info(host->id);

				MSDC_WRITE32(MSDC_INT, status);
				msdc_reset_hw(host->id);
				return err;
			}
		}

		l_count++;
		if (l_count > 50000) {
			l_count = 0;
			if (print_count > 0) {
				pr_err("size= %d, left= %d.\r\n", size, left);
				simp_msdc_dump_info(host->id);
				print_count--;
			}
		}
	}

	return err;
}

static int simp_msdc_pio_read(struct simp_msdc_host *host,
	unsigned int *ptr, unsigned int size)
{
	void __iomem *base = host->base;
	unsigned int left = size;
	unsigned int status = 0;
	unsigned char *u8ptr;
	int l_count = 0;
	int err = 0;
	int print_count = 2;
	int done = 0;

	while (1) {
		status = MSDC_READ32(MSDC_INT);
		MSDC_WRITE32(MSDC_INT, status);
		if (status & MSDC_INT_DATCRCERR) {
			pr_err("[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",
				host->id, status, left);
			err = -5;
			simp_msdc_dump_info(host->id);
			break;
		} else if (status & MSDC_INT_DATTMO) {
			pr_err("[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",
				host->id, status, left);
			err = -110;
			simp_msdc_dump_info(host->id);
			break;
		} else if (status & MSDC_INT_XFER_COMPL) {
			done = 1;
		}
		if (done && (left == 0))
			break;

		while (left) {
			/* pr_err("left(%d)/FIFO(%d)\n",
				left,msdc_rxfifocnt()); */
			if ((left >= MSDC_FIFO_THD) && (msdc_rxfifocnt()
				>= MSDC_FIFO_THD)) {
				int count = MSDC_FIFO_THD >> 2;
				do {
					*ptr++ = msdc_fifo_read32();
				} while (--count);
				left -= MSDC_FIFO_THD;
			} else if ((left < MSDC_FIFO_THD) && msdc_rxfifocnt()
				>= left) {
				while (left > 3) {
					*ptr++ = msdc_fifo_read32();
					left -= 4;
				}

				u8ptr = (u8 *)ptr;
				while (left) {
					*u8ptr++ = msdc_fifo_read8();
					left--;
				}
			} else {
				status = MSDC_READ32(MSDC_INT);

				if (status & MSDC_INT_DATCRCERR) {
					pr_err("[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",
						host->id, status, left);
					err = -5;
				}
				if (status & MSDC_INT_DATTMO) {
					pr_err("[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",
						host->id, status, left);
					err = -110;
				}

				if ((status & MSDC_INT_DATCRCERR)
					|| (status & MSDC_INT_DATTMO)) {
					simp_msdc_dump_info(host->id);

					MSDC_WRITE32(MSDC_INT, status);
					msdc_reset_hw(host->id);
					return err;
				}
			}

			/* timeout monitor*/
			l_count++;
			if (l_count > 50000) {
				l_count = 0;
				if (print_count > 0) {
					pr_err("size= %d, left= %d, done=%d.\n",
						size, left, done);
					simp_msdc_dump_info(host->id);
					print_count--;
				}
			}
		}
	}

	return err;
}

static void simp_msdc_set_blknum(struct simp_msdc_host *host,
	unsigned int blknum)
{
	void __iomem *base = host->base;
	MSDC_WRITE32(SDC_BLK_NUM, blknum);
}

static int simp_mmc_write(struct simp_mmc_host *host, unsigned int addr,
	void *buf, unsigned int nblk)
{
	unsigned int resp  = 0;
	unsigned int err = 0;
	/*unsigned int intsts = 0;*/
	struct simp_msdc_host *phost = host->mtk_host;
	void __iomem *base = phost->base;

	simp_msdc_set_blknum(phost, nblk);

	/* send command */
	if (nblk == 1)
		err = simp_msdc_cmd(phost, MMC_WRITE_BLOCK, CMD24_RAW, addr,
			RESP_R1, &resp);
	else
		err = simp_msdc_cmd(phost, MMC_WRITE_MULTIPLE_BLOCK, CMD25_RAW,
			addr, RESP_R1, &resp);

	/* write the data to FIFO */
	err = simp_msdc_pio_write(phost, (unsigned int *)buf, 512*nblk);
	if (err)
		pr_err("write data: error(%d)\n", err);

	/* make sure contents in fifo flushed to device */
	BUG_ON(msdc_txfifocnt());

	/* check and clear interrupt */

	if (nblk > 1)
		simp_mmc_send_stop(host);

	return err;
}

static int simp_mmc_read(struct simp_mmc_host *host, unsigned int addr,
	void *buf, unsigned int nblk)
{
	unsigned int resp  = 0;
	unsigned int err  = 0;
	struct simp_msdc_host *phost = host->mtk_host;

	simp_msdc_set_blknum(phost, nblk);

	/* send command */
	if (nblk == 1)
		err = simp_msdc_cmd(phost, MMC_READ_SINGLE_BLOCK, CMD17_RAW,
			addr, RESP_R1, &resp);
	else
		err = simp_msdc_cmd(phost, MMC_READ_MULTIPLE_BLOCK, CMD18_RAW,
			addr, RESP_R1, &resp);

	/* read the data out*/
	err = simp_msdc_pio_read(phost, (unsigned int *)buf, 512*nblk);
	if (err)
		pr_err("read data: error(%d)\n", err);

	if (nblk > 1)
		simp_mmc_send_stop(host);
	return err;
}


/* card_type tell to use which host, will support PANIC dump info to emmc card
 * and KDUMP info to sd card */
int msdc_init_panic(int dev)
{
	return 1;
}

static int simp_mmc_init(int card_type, bool boot)
{
	struct simp_msdc_host *cur_msdc_host;
	struct simp_mmc_host *cur_mmc_host;
	int id;
	u32 *hclks;

	if (boot) {
		cur_msdc_host = pmsdc_boot_host;
		cur_mmc_host = pmmc_boot_host;
		id = BOOT_STORAGE_ID;
	} else {
		cur_msdc_host = pmsdc_extend_host;
		cur_mmc_host = pmmc_extend_host;
		id = EXTEND_STORAGE_ID;
	}

	/* init some struct */
	cur_mmc_host->mtk_host = cur_msdc_host;
	cur_mmc_host->card = &g_mmc_card[id];
	cur_mmc_host->card->host = cur_mmc_host;

	memset(cur_msdc_host, 0, sizeof(struct simp_msdc_host));
	cur_msdc_host->id = id;
	if (NULL == mtk_msdc_host[id])
		goto out;

	cur_msdc_host->base = (mtk_msdc_host[id])->base;

	hclks = msdc_get_hclks(id);
	if (id == 0)
		cur_msdc_host->clksrc = MSDC0_CLKSRC_DEFAULT;
	else
		cur_msdc_host->clksrc = MSDC1_CLKSRC_DEFAULT;
	cur_msdc_host->clk = hclks[cur_msdc_host->clksrc];
	cur_msdc_host->card = &g_msdc_card[id];

	/* not use now, may be delete */
	memset(&g_msdc_card[id], 0, sizeof(struct simp_msdc_card));
	g_msdc_card[id].type = card_type;

	if (boot) {
		g_msdc_card[id].file_system = _RAW_;

		/* init host & card */
		module_init_emmc = 1;

	} else {
		g_msdc_card[id].file_system = FAT32;

		pmsdc_extend_host->card->card_cap = 1;

		module_init_sd = 0;
	}
	return SIMP_SUCCESS;
out:
	return SIMP_FAILED;
}


/*--------------------------------------------------------------------------*/
/* porting for panic dump interface                                         */
/*--------------------------------------------------------------------------*/
static sector_t lp_start_sect = (sector_t)(-1);
static sector_t lp_nr_sects = (sector_t)(-1);

#ifdef CONFIG_MTK_EMMC_SUPPORT
static int simp_emmc_dump_write(unsigned char *buf, unsigned int len,
	unsigned int offset, unsigned int dev)
{
	/* maybe delete in furture */
	unsigned int status = 0;
	int polling = MAX_POLLING_STATUS;
	unsigned long long l_start_offset;
	unsigned int l_addr;
	unsigned char *l_buf;
	unsigned int ret = 1;  /* != 0 means error occur */

	if (0 != len % 512) {
		/* emmc always in slot0 */
		pr_err("debug: parameter error!\n");
		return ret;
	}

	/* find the offset in emmc */
	if (lp_start_sect == 0) {
		pr_err("Illegal expdb start address (0x0)\n");
		return ret;
	}

	if (lp_start_sect == (sector_t)(-1) || lp_nr_sects == (sector_t)(-1)) {
		pr_err("partition not found error!\n");
		return ret;
	}

	if ((lp_nr_sects < (len >> 9)) ||
	    (lp_nr_sects < (offset >> 9)) ||
	    (lp_nr_sects < ((len + offset) >> 9))) {
		pr_err("write operation oversize!\n");
		return ret;
	}

	MTK_DUMP_PR_DBG("write start sector = %llu, part size = %llu\n",
		(u64)lp_start_sect, (u64)lp_nr_sects);

	l_start_offset = (u64)offset + (u64)(lp_start_sect << 9);

	MTK_DUMP_PR_DBG("write start address = %llu\n", l_start_offset);

	if (emmc_init == 0) {
		if (simp_init_try(MSDC_EMMC, &module_init_emmc, &emmc_init)
			!= 0)
			return ret;
	}

	l_addr = l_start_offset >> 9; /*blk address*/
	l_buf  = buf;

	MTK_DUMP_PR_DBG("l_start_offset =0x%x\n", l_addr);

	ret = simp_mmc_write(pmmc_boot_host, l_addr, l_buf, len / 512);

	do {
		simp_mmc_get_status(pmmc_boot_host, &status);
	} while (R1_CURRENT_STATE(status) == 7 && polling--);
	if (ret == 0)
		ret = SIMP_SUCCESS;
	return ret;

}
#endif /* end of CONFIG_MTK_EMMC_SUPPORT */

static int simp_sd_dump_write(unsigned char *buf, unsigned int len,
	unsigned int offset, unsigned int dev)
{
	/* unsigned int i; */
	unsigned int l_addr;
	unsigned char *l_buf;
	int polling = MAX_POLLING_STATUS;
	unsigned int status = 0;
	/* unsigned int l_start_offset; */
	unsigned int ret = SIMP_FAILED;
	int err = 0;

	if (0 != len % 512) {
		/* emmc always in slot0 */
		pr_err("debug: parameter error!\n");
		return ret;
	}
#if 0
	pr_debug("write data:");
	for (i = 0; i < 32; i++) {
		pr_debug("0x%x", buf[i]);
		if (0 == (i + 1) % 32)
			pr_debug("\n");
	}
#endif
	/* l_start_offset = offset; */
	l_buf = buf;
	if (pmsdc_extend_host->card->card_cap == standard_capacity)
		l_addr = offset << 9;
	else
		l_addr = offset;

	MTK_DUMP_PR_ERR("l_start_offset = 0x%x len = %d buf<0x%p>\n", l_addr,
		len, l_buf);

	err = simp_mmc_write(pmmc_extend_host, l_addr, l_buf, len / 512);

	do {
		simp_mmc_get_status(pmmc_extend_host, &status);
	} while (R1_CURRENT_STATE(status) == 7 && polling--);
	if (err == 0)
		ret = SIMP_SUCCESS;
	return ret;
}

static int simp_sd_dump_read(unsigned char *buf, unsigned int len,
	unsigned int offset)
{
	/* unsigned int i; */
	unsigned int l_addr;
	unsigned char *l_buf;
	/* unsigned int l_start_offset; */
	unsigned int ret = SIMP_FAILED;
	int err = 0;

	if (0 != len % 512) {
		pr_err("debug: parameter error!\n");
		return ret;
	}

	if (sd_init == 0) {
		if (simp_init_try(MSDC_SD, &module_init_sd, &sd_init) != 0)
			return ret;
	}
	/* l_start_offset = offset; */
	l_buf = buf;

	MTK_DUMP_PR_DBG("l_start_offset = 0x%x len = %d\n", offset, len);

	if (pmsdc_extend_host->card->card_cap == standard_capacity)
		l_addr = offset << 9;
	else
		l_addr = offset;

	err = simp_mmc_read(pmmc_extend_host, l_addr, l_buf, len / 512);

#if 0
	pr_debug("read data:");
	for (i = 0; i < 32; i++) {
		pr_debug("0x%x", buf[i]);
		if (0 == (i + 1) % 32)
			pr_debug("\n");
	}
#endif
	if (err == 0)
		ret = SIMP_SUCCESS;
	return ret;
}

#ifdef CONFIG_MTK_EMMC_SUPPORT
#define SD_FALSE                (-1)
#define SD_TRUE                 (0)
#define DEBUG_MMC_IOCTL         (0)
static int simp_emmc_dump_read(unsigned char *buf, unsigned int len,
	unsigned int offset, unsigned int slot)
{
	/* maybe delete in furture */
	struct msdc_ioctl msdc_ctl;
	unsigned int i;
	unsigned long long l_start_offset = 0;
	unsigned int ret = SD_FALSE;

	if ((0 != slot) || (0 != offset % 512) || (0 != len % 512)) {
		/* emmc always in slot0 */
		pr_err("debug: slot is not use for emmc!\n");
		return ret;
	}

	if (0 != len % 512) {
		/* emmc always in slot0 */
		pr_err("debug: parameter error!\n");
		return ret;
	}
	/* find the offset in emmc */
	if (lp_start_sect == (sector_t)(-1) || lp_nr_sects == (sector_t)(-1)) {
		pr_err("not find in scatter file error!\n");
		return ret;
	}
	if (lp_nr_sects < ((len + offset) >> 9)) {
		pr_err("read operation oversize!\n");
		return ret;
	}

	MTK_DUMP_PR_DBG("read start sector = %llu, part size = %llu\n",
		(u64)lp_start_sect, (u64)lp_nr_sects);

	l_start_offset = (u64)offset + (u64)(lp_start_sect << 9);

	MTK_DUMP_PR_DBG("read start address = %llu\n", l_start_offset);

	/* FIX ME: msdc_ctl.total_size as MAX_DMA_CNT so that CMD18 is used */
	msdc_ctl.partition = 0;
	msdc_ctl.iswrite = 0;
	msdc_ctl.host_num = slot;
	msdc_ctl.opcode = MSDC_CARD_DUNM_FUNC;
	msdc_ctl.total_size = MAX_DMA_CNT;
	msdc_ctl.trans_type = 0;
	msdc_ctl.address = l_start_offset >> 9;
	msdc_ctl.buffer = (u32 *) buf;
	for (i = 0; i < (len/MAX_DMA_CNT); i++) {
#if DEBUG_MMC_IOCTL
		pr_debug("l_start_offset = 0x%x\n", msdc_ctl.address);
#endif
		msdc_ctl.result = simple_sd_ioctl_rw(&msdc_ctl);
		msdc_ctl.address +=  MAX_DMA_CNT >> 9;
		msdc_ctl.buffer = (u32 *) (buf + MAX_DMA_CNT * (i+1));
	}

	msdc_ctl.total_size = len % MAX_DMA_CNT;
	if (0 != msdc_ctl.total_size) {
#if DEBUG_MMC_IOCTL
		pr_debug("l_start_offset = 0x%x\n", msdc_ctl.address);
#endif
		msdc_ctl.result = simple_sd_ioctl_rw(&msdc_ctl);
	}

#if DEBUG_MMC_IOCTL
	pr_debug("read data:");
	for (i = 0; i < 32; i++) {
		pr_debug("0x%x", buf[i]);
		if (0 == (i+1)%32)
			pr_debug("\n");
	}
#endif
	return SD_TRUE;
}
#endif

int card_dump_func_write(unsigned char *buf, unsigned int len,
	unsigned long long offset, int dev)
{
	int ret = SIMP_FAILED;
	unsigned int sec_offset = 0;

	MTK_DUMP_PR_DBG("card_dump_func_write len<%d> addr<%lld> type<%d>\n",
		len, offset, dev);

	if (offset % 512) {
		pr_err("Address isn't 512 alignment!\n");
		return SIMP_FAILED;
	}
	sec_offset = offset / 512;
	switch (dev) {
	case DUMP_INTO_BOOT_CARD_IPANIC:
#ifdef CONFIG_MTK_EMMC_SUPPORT
		ret = simp_emmc_dump_write(buf, len, (unsigned int)offset, dev);
#endif
		break;
	case DUMP_INTO_BOOT_CARD_KDUMP:
		break;
	case DUMP_INTO_EXTERN_CARD:
		ret = simp_sd_dump_write(buf, len, sec_offset, dev);
		break;
	default:
		pr_err("unknown card type, error!\n");
		break;
	}

	return ret;
}
EXPORT_SYMBOL(card_dump_func_write);

int card_dump_func_read(unsigned char *buf, unsigned int len,
	unsigned long long offset, int dev)
{
	unsigned int ret = SIMP_FAILED;
	unsigned int sec_offset = 0;

	MTK_DUMP_PR_DBG("card_dump_func_read len<%d> addr<%lld> type<%d>\n",
		len, offset, dev);

	if (offset % 512) {
		pr_err("Address isn't 512 alignment!\n");
		return SIMP_FAILED;
	}
	sec_offset = offset / 512;
	switch (dev) {
	case DUMP_INTO_BOOT_CARD_IPANIC:
#ifdef CONFIG_MTK_EMMC_SUPPORT
		ret = simp_emmc_dump_read(buf, len, (unsigned int)offset, dev);
#endif
		break;
	case DUMP_INTO_BOOT_CARD_KDUMP:
		break;
	case DUMP_INTO_EXTERN_CARD:
		ret = simp_sd_dump_read(buf, len, sec_offset);
		break;
	default:
		pr_err("unknown card type, error!\n");
		break;
	}
	return ret;

}
EXPORT_SYMBOL(card_dump_func_read);

int has_mt_dump_support(void)
{
	return 1;
}
EXPORT_SYMBOL(has_mt_dump_support);


/*--------------------------------------------------------------------------*/
/* porting for kdump interface                                              */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* module init/exit                                                         */
/*--------------------------------------------------------------------------*/
static int __init emmc_dump_init(void)
{
#ifdef CONFIG_MTK_EMMC_SUPPORT
	simp_mmc_init(MSDC_EMMC, 1);
#endif
	simp_mmc_init(MSDC_SD, 0);

	MTK_DUMP_PR_DBG("EMMC/SD dump init\n");

	return 0;
}

static void __exit emmc_dump_exit(void)
{
}
module_init(emmc_dump_init);
module_exit(emmc_dump_exit);

#ifdef CONFIG_MTK_EMMC_SUPPORT
/* @partition_ready_flag,
 *  = 0: partition init not ready
 *  = 1: partition init is done and succeed
 *  = -1: there is no expdb partition
 */
static int partition_ready_flag;
int get_emmc_dump_status(void)
{
	return partition_ready_flag;
}

static void get_emmc_dump_info(struct work_struct *work)
{
	struct hd_struct *lp_hd_struct = NULL;
	lp_hd_struct = get_part("expdb");
	if (likely(lp_hd_struct)) {
		lp_start_sect = lp_hd_struct->start_sect;
		lp_nr_sects = lp_hd_struct->nr_sects;
		put_part(lp_hd_struct);
		partition_ready_flag = 1;
		pr_err("get expdb info\n");
	} else {
		lp_start_sect = (sector_t)(-1);
		lp_nr_sects = (sector_t)(-1);
		partition_ready_flag = -1;
		pr_err("There is no expdb info\n");
	}

	partition_ready_flag = 1;
}

static struct delayed_work get_dump_info;
static int __init init_get_dump_work(void)
{
	INIT_DELAYED_WORK(&get_dump_info, get_emmc_dump_info);
	if (schedule_delayed_work(&get_dump_info, 100))
		return 1;

	return 0;
}
late_initcall_sync(init_get_dump_work);
#endif
