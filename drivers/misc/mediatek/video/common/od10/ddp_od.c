#include <linux/gfp.h>
#include <linux/types.h>
#include <linux/string.h> /* for test cases */
#include <asm/cacheflush.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
/* #include <mach/memory.h> */
/* #include "ddp_wdma.h" */
#include <asm/uaccess.h>
/* #include <mach/mt_irq.h> */
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
/* #include <mach/mt_typedefs.h> */
/* #include <mach/mt_spm_idle.h> */

/* #include <cmdq_def.h> */
#include <ddp_drv.h>
#include <ddp_reg.h>
#include <ddp_debug.h>
#include <ddp_log.h>
#include <lcm_drv.h>
#include <ddp_dither.h>
#include <ddp_od.h>

/* #include <mach/m4u.h> */
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kthread.h>

#if defined(MTK_FB_OD_SUPPORT)

/* ----------------------- Begin of register definitions ----------------------- */
#define OD_BASE      DISPSYS_OD_BASE /* 0x14023000 */

/* Page OD */
#define OD_REG00 (OD_BASE + 0x0700)
#define BYPASS_ALL           REG_FLD(1, 31) /* [31:31] */
#define OD_CLK_INV           REG_FLD(1, 30) /* [30:30] */
#define OD_CLK_EN            REG_FLD(1, 29) /* [29:29] */
#define DC_CNT               REG_FLD(5, 24) /* [28:24] */
#define BTC_ERR_CNT          REG_FLD(5, 16) /* [20:16] */
#define C_GAT_CNT            REG_FLD(5, 8)  /* [12:8] */
#define RG_Y5_MODE           REG_FLD(1, 6)  /* [6:6] */
#define UVRUD_DISA           REG_FLD(1, 5)  /* [5:5] */
#define OD_8BIT_SEL          REG_FLD(1, 4)  /* [4:4] */
#define OD_ENABLE            REG_FLD(1, 0)  /* [0:0] */

#define OD_REG01 (OD_BASE + 0x0704)
#define OD_MONSEL            REG_FLD(4, 28) /* [31:28] */
#define PKT_DBG              REG_FLD(1, 25) /* [25:25] */
#define OD_RST               REG_FLD(1, 24) /* [24:24] */
#define DMA_RD_MODE          REG_FLD(1, 20) /* [20:20] */
#define DMA_RD_THR           REG_FLD(4, 16) /* [19:16] */
#define ALBUF2_DLY           REG_FLD(5, 8)  /* [12:8] */
#define MOTION_THR           REG_FLD(8, 0)  /* [7:0] */

#define OD_REG02 (OD_BASE + 0x0708)
#define DISABLE_8B_BTC       REG_FLD(1, 31) /* [31:31] */
#define DISABLE_8B_DC        REG_FLD(1, 30) /* [30:30] */
#define FORCE_8B_BTC         REG_FLD(1, 29) /* [29:29] */
#define FORCE_8B_DC          REG_FLD(1, 28) /* [28:28] */
#define OD255_FIX            REG_FLD(1, 22) /* [22:22] */
#define UV_MODE_MASK         REG_FLD(3, 19) /* [21:19] */
#define Y_MODE_MASK          REG_FLD(3, 16) /* [18:16] */
#define DEMO_MODE            REG_FLD(1, 14) /* [14:14] */
#define FORCE_FPIN_RED       REG_FLD(1, 13) /* [13:13] */
#define DISPLAY_COMPRESSION  REG_FLD(1, 12) /* [12:12] */
#define FORCE_ENABLE_OD_MUX  REG_FLD(1, 11) /* [11:11] */
#define FBT_BYPASS           REG_FLD(1, 10) /* [10:10] */
#define ODT_BYPASS           REG_FLD(1, 9)  /* [9:9] */

#define OD_REG03 (OD_BASE + 0x070C)
#define INK_B_VALUE          REG_FLD(8, 24) /* [31:24] */
#define INK_G_VALUE          REG_FLD(8, 16) /* [23:16] */
#define INK_R_VALUE          REG_FLD(8, 8)  /* [15:8] */
#define CR_INK_SEL           REG_FLD(3, 2)  /* [4:2] */
#define CR_INK               REG_FLD(1, 1)  /* [1:1] */
#define ODT_INK_EN           REG_FLD(1, 0)  /* [0:0] */

#define OD_REG04 (OD_BASE + 0x0710)
#define TABLE_RW_SEL_PCIDB    REG_FLD(1, 27)  /* [27:27] */
#define TABLE_RW_SEL_PCIDG    REG_FLD(1, 26)  /* [26:26] */
#define TABLE_RW_SEL_PCIDR    REG_FLD(1, 25)  /* [25:25] */
#define TABLE_RW_SEL_PCID_BGR REG_FLD(3, 25)  /* [27:25] */
#define TABLE_ONLY_W_ADR_INC  REG_FLD(1, 23)  /* [23:23] */
#define TABLE_R_SEL_FB_EVEN   REG_FLD(1, 22)  /* [22:22] */
#define TABLE_RW_SEL_OD_B     REG_FLD(1, 21)  /* [21:21] */
#define TABLE_RW_SEL_OD_G     REG_FLD(1, 20)  /* [20:20] */
#define TABLE_RW_SEL_OD_R     REG_FLD(1, 19)  /* [19:19] */
#define TABLE_RW_SEL_OD_BGR   REG_FLD(3, 19)  /* [21:19] */
#define TABLE_RW_SEL_FB_B     REG_FLD(1, 18)  /* [18:18] */
#define TABLE_RW_SEL_FB_G     REG_FLD(1, 17)  /* [17:17] */
#define TABLE_RW_SEL_FB_R     REG_FLD(1, 16)  /* [16:16] */
#define TABLE_RW_SEL_FB_BGR   REG_FLD(3, 16)  /* [18:16] */
#define ADDR_Y                REG_FLD(6, 8)   /* [13:8] */
#define ADDR_X                REG_FLD(6, 0)   /* [5:0] */
#define ADDR_YX               REG_FLD(14, 0)  /* [13:0] */

#define OD_REG05 (OD_BASE + 0x0714)
#define TABLE_RW_DATA         REG_FLD(8, 0)  /* [7:0] */

#define OD_REG06 (OD_BASE + 0x0718)
#define RG_BASE_ADR           REG_FLD(28, 0) /* [27:0] */

#define OD_REG07 (OD_BASE + 0x071C)
#define VALIDTH               REG_FLD(8, 20) /* [27:20] */
#define DEBUG_SEL             REG_FLD(3, 17) /* [19:17] */
#define RTHRE                 REG_FLD(8, 9)  /* [16:9] */
#define SWRESET               REG_FLD(1, 8)  /* [8:8] */
#define WTHRESH               REG_FLD(8, 0)  /* [7:0] */

#define OD_REG08 (OD_BASE + 0x0720)
#define OD_H_ACTIVE           REG_FLD(12, 0) /* [11:0] */
#define OD_V_ACTIVE           REG_FLD(12, 16)/* [27:16] */

#define OD_REG09 (OD_BASE + 0x0724)
#define RG_H_BLANK           REG_FLD(12, 0)  /*[11:0] */
#define RG_H_OFFSET          REG_FLD(12, 16) /*[27:16] */

#define OD_REG10 (OD_BASE + 0x0728)
#define RG_H_BLANK_MAX       REG_FLD(12, 0)  /*[11:0] */
#define RG_V_BLANK_MAX       REG_FLD(20, 12) /*[31:12] */

#define OD_REG11 (OD_BASE + 0x072C)
#define RG_V_BLANK           REG_FLD(18, 0)  /* [17:0] */
#define RG_FRAME_SET         REG_FLD(4, 18)  /* [21:18] */

#define OD_REG12 (OD_BASE + 0x0730)
#define RG_OD_START          REG_FLD(1, 0)   /* [0:0] */
#define RG_OD_DRAM_MSB       REG_FLD(6, 1)   /* [6:1] */
#define RG_PAT_START         REG_FLD(1, 7)   /* [7:7] */

#define OD_REG13 (OD_BASE + 0x0734)
#define DMA_WDATA_MON        REG_FLD(32, 0)/* [31:0] */

#define OD_REG14 (OD_BASE + 0x0738)
#define RD_DBG              REG_FLD(8, 24) /* [31:24] */
#define RD_ADR              REG_FLD(18, 0) /* [17:0] */

#define OD_REG15 (OD_BASE + 0x073C)
#define WR_DBG              REG_FLD(8, 24) /* [31:24] */
#define WR_ADR              REG_FLD(18, 0) /* [17:0] */

#define OD_REG16 (OD_BASE + 0x0740)
#define DMA_RDATA0          REG_FLD(32, 0) /* [31:0] */

#define OD_REG17 (OD_BASE + 0x0744)
#define DMA_RDATA1          REG_FLD(32, 0)/* [31:0] */

#define OD_REG18 (OD_BASE + 0x0748)
#define DMA_RDATA2          REG_FLD(32, 0)/* [31:0] */

#define OD_REG19 (OD_BASE + 0x074C)
#define DMA_RDATA3          REG_FLD(32, 0)/* [31:0] */

#define OD_REG20 (OD_BASE + 0x0750)
#define DMA_WR_CNT          REG_FLD(32, 0)/* [31:0] */

#define OD_REG21 (OD_BASE + 0x0754)
#define RG_PAT_H_START       REG_FLD(12, 0)/* [11:0] */
#define RG_PAT_V_START       REG_FLD(12, 16)/* [27:16] */

#define OD_REG22 (OD_BASE + 0x0758)
#define RG_PAT_H_OFFSET      REG_FLD(12, 0)/* [11:0] */
#define RG_PAT_V_OFFSET      REG_FLD(12, 16)/* [27:16] */

#define OD_REG23 (OD_BASE + 0x075C)
#define RG_PAT_LENGTH        REG_FLD(12, 0)/* [11:0] */
#define RG_PAT_WIDTH         REG_FLD(12, 16)/* [27:16] */

#define OD_REG24 (OD_BASE + 0x0760)
#define RG_PAT_YIN0          REG_FLD(10, 0)/* [9:0] */
#define RG_PAT_CIN0          REG_FLD(10, 10)/* [19:10] */
#define RG_PAT_VIN0          REG_FLD(10, 20)/* [29:20] */

#define OD_REG25 (OD_BASE + 0x0764)
#define RG_PAT_YIN1          REG_FLD(10, 0)/* [9:0] */
#define RG_PAT_CIN1          REG_FLD(10, 10)/* [19:10] */
#define RG_PAT_VIN1          REG_FLD(10, 20)/* [29:20] */

#define OD_REG26 (OD_BASE + 0x0768)
#define RG_AGENT_FREQ        REG_FLD(9, 0)/*[8:0] */
#define RG_BLACK_AGENT       REG_FLD(1, 9)/*[9:9] */
#define RG_NO_BLACK          REG_FLD(1, 10)/*[10:10] */
#define RG_BLACK_PAT         REG_FLD(10, 16)/*[25:16] */

#define OD_REG28 (OD_BASE + 0x0770)
#define RG_TABLE_DMA_ADR_ST  REG_FLD(27, 0)/*[26:0] */

#define OD_REG29 (OD_BASE + 0x0774)
#define RG_TABLE_DMA_EN            REG_FLD(1, 0)/*[0:0] */
#define RG_TABLE_RGB_SEQ           REG_FLD(1, 1)/*[1:1] */
#define RG_TABLE_DMA_DONE_CLR      REG_FLD(1, 2)/*[2:2] */
#define RG_ODT_SIZE                REG_FLD(2, 3)/*[4:3] */

#define OD_REG30 (OD_BASE + 0x0778)
#define MANU_CPR                   REG_FLD(8, 0)/*[7:0] */

#define OD_REG31 (OD_BASE + 0x077C)
#define SYNC_V_EDGE                REG_FLD(1, 4)/*[4:4] */
#define SYNC_V_SRC                 REG_FLD(1, 5)/*[5:5] */
#define OD_H_DELAY                 REG_FLD(6, 6)/*[11:6] */
#define OD_V_DELAY                 REG_FLD(6, 12)/*[17:12] */
#define HI_POL                     REG_FLD(1, 18)/*[18:18] */
#define VI_POL                     REG_FLD(1, 19)/*[19:19] */
#define HO_POL                     REG_FLD(1, 20)/*[20:20] */
#define VO_POL                     REG_FLD(1, 21)/*[21:21] */
#define FORCE_INT                  REG_FLD(1, 22)/*[22:22] */
#define OD_SYNC_FEND               REG_FLD(1, 23)/*[23:23] */
#define OD_SYNC_POS                REG_FLD(8, 24)/*[31:24] */

#define OD_REG32 (OD_BASE + 0x0788)
#define OD_EXT_FP_EN               REG_FLD(1, 0)/*[0:0] */
#define OD_USE_EXT_DE_TOTAL        REG_FLD(1, 1)/*[1:1] */
#define OD255_FIX2_SEL             REG_FLD(2, 2)/*[3:2] */
#define FBT_BYPASS_FREQ            REG_FLD(4, 4)/*[7:4] */
#define OD_DE_WIDTH                REG_FLD(13, 8)/*[20:8] */
#define OD_LNR_DISABLE             REG_FLD(1, 21)/*[21:21] */
#define OD_IDX_17                  REG_FLD(1, 23)/*[23:23] */
#define OD_IDX_41                  REG_FLD(1, 24)/*[24:24] */
#define OD_IDX_41_SEL              REG_FLD(1, 25)/*[25:25] */
#define MERGE_RGB_LUT_EN           REG_FLD(1, 26)/*[26:26] */
#define OD_RDY_SYNC_V              REG_FLD(1, 27)/*[27:27] */
#define OD_CRC_START               REG_FLD(1, 28)/*[28:28] */
#define OD_CRC_CLR                 REG_FLD(1, 29)/*[29:29] */
#define OD_CRC_SEL                 REG_FLD(2, 30)/*[31:30] */

#define OD_REG33 (OD_BASE + 0x078C)
#define FORCE_Y_MODE               REG_FLD(4, 0)/*[3:0] */
#define FORCE_C_MODE               REG_FLD(4, 8)/*[11:8] */
#define ODR_DM_REQ_EN              REG_FLD(1, 12)/*[12:12] */
#define ODR_DM_TIM                 REG_FLD(4, 13)/*[16:13] */
#define ODW_DM_REQ_EN              REG_FLD(1, 17)/*[17:17] */
#define ODW_DM_TIM                 REG_FLD(4, 18)/*[21:18] */
#define DMA_DRAM_REQ_EN            REG_FLD(1, 22)/*[22:22] */
#define DMA_DRAM_TIM               REG_FLD(4, 23)/*[26:23] */

#define OD_REG34 (OD_BASE + 0x0790)
#define ODT_SB_TH0                 REG_FLD(5, 0)/*[4:0] */
#define ODT_SB_TH1                 REG_FLD(5, 5)/*[9:5] */
#define ODT_SB_TH2                 REG_FLD(5, 10)/*[14:10] */
#define ODT_SB_TH3                 REG_FLD(5, 15)/*[19:15] */
#define ODT_SOFT_BLEND_EN          REG_FLD(1, 20)/*[20:20] */
#define DET8B_BLK_NBW              REG_FLD(11, 21)/*[31:21] */

#define OD_REG35 (OD_BASE + 0x0794)
#define DET8B_DC_NUM               REG_FLD(18, 0)/*[17:0] */
#define DET8B_DC_THR               REG_FLD(3, 18)/*[20:18] */

#define OD_REG36 (OD_BASE + 0x0798)
#define DET8B_BTC_NUM              REG_FLD(18, 0)/*[17:0] */
#define DET8B_BTC_THR              REG_FLD(3, 18)/*[20:18] */
#define DET8B_SYNC_POS             REG_FLD(8, 21)/*[28:21] */
#define DET8B_HYST                 REG_FLD(3, 29)/*[31:29] */

#define OD_REG37 (OD_BASE + 0x079C)
#define ODT_MAX_RATIO              REG_FLD(4, 0)/*[3:0] */
#define DET8B_BIT_MGN              REG_FLD(20, 6)/*[25:6] */
#define DET8B_DC_OV_ALL            REG_FLD(1, 26)/*[26:26] */

#define OD_REG38 (OD_BASE + 0x07A0)
#define WR_BURST_LEN               REG_FLD(2, 0)/* [1:0] */
#define WR_PAUSE_LEN               REG_FLD(8, 2)/* [9:2] */
#define WFF_EMP_OPT                REG_FLD(1, 10)/* [10:10] */
#define RD_BURST_LEN               REG_FLD(2, 11)/* [12:11] */
#define LINE_SIZE                  REG_FLD(9, 13)/* [21:13] */
#define DRAM_CRC_CLR               REG_FLD(1, 22)/* [22:22] */
#define RD_PAUSE_LEN               REG_FLD(8, 23)/* [30:23] */

#define OD_REG39 (OD_BASE + 0x07A4)
#define OD_PAGE_MASK               REG_FLD(16, 0)/* [15:0] */
#define DRAM_CRC_CNT               REG_FLD(9, 16)/* [24:16] */
#define WDRAM_ZERO                 REG_FLD(1, 25)/* [25:25] */
#define WDRAM_FF                   REG_FLD(1, 26)/* [26:26] */
#define RDRAM_LEN_X4               REG_FLD(1, 27)/* [27:27] */
#define WDRAM_DIS                  REG_FLD(1, 28)/* [28:28] */
#define RDRAM_DIS                  REG_FLD(1, 29)/* [29:29] */
#define W_CHDEC_RST                REG_FLD(1, 30)/* [30:30] */
#define R_CHDEC_RST                REG_FLD(1, 31)/* [31:31] */

#define OD_REG40 (OD_BASE + 0x07A8)
#define GM_FORCE_VEC               REG_FLD(3, 0)  /*[2:0] */
#define GM_VEC_RST                 REG_FLD(1, 3)  /*[3:3] */
#define GM_EN                      REG_FLD(1, 4)  /*[4:4] */
#define GM_FORCE_EN                REG_FLD(1, 5)  /*[5:5] */
#define GM_AUTO_SHIFT              REG_FLD(1, 6)  /*[6:6] */
#define REP22_0                    REG_FLD(2, 7)  /*[8:7] */
#define REP22_1                    REG_FLD(2, 9)  /*[10:9] */
#define GM_R0_CENTER               REG_FLD(7, 11) /*[17:11] */
#define REP22_2                    REG_FLD(2, 18) /*[19:18] */
#define REP22_3                    REG_FLD(2, 20) /*[21:20] */
#define GM_R1_CENTER               REG_FLD(7, 22) /*[28:22] */
#define GM_TRACK_SEL               REG_FLD(1, 29) /*[29:29] */

#define OD_REG41 (OD_BASE + 0x07AC)
#define REP22_4                    REG_FLD(2, 0)  /*[1:0] */
#define REP22_5                    REG_FLD(2, 2)  /*[3:2] */
#define GM_R2_CENTER               REG_FLD(7, 4)  /*[10:4] */
#define REP22_6                    REG_FLD(2, 11) /*[12:11] */
#define REP22_7                    REG_FLD(2, 13) /*[14:13] */
#define GM_R3_CENTER               REG_FLD(7, 15) /*[21:15] */

#define OD_REG42 (OD_BASE + 0x07B0)
#define REP32_0                    REG_FLD(2, 0)  /*[1:0] */
#define REP32_1                    REG_FLD(2, 2)  /*[3:2] */
#define GM_R4_CENTER               REG_FLD(7, 4)  /*[10:4] */
#define GM_HYST_SEL                REG_FLD(4, 11) /*[14:11] */
#define GM_LGMIN_DIFF              REG_FLD(12, 16)/*[27:16] */
#define REP32_6                    REG_FLD(2, 28) /*[29:28] */
#define REP32_7                    REG_FLD(2, 30) /*[31:30] */

#define OD_REG43 (OD_BASE + 0x07B4)
#define GM_REP_MODE_DET            REG_FLD(1, 0)/* [0:0] */
#define RPT_MODE_EN                REG_FLD(1, 1)/* [1:1] */
#define GM_V_ST                    REG_FLD(9, 2)/* [10:2] */
#define RPT_MODE_HYST              REG_FLD(2, 11)/* [12:11] */
#define GM_V_END                   REG_FLD(9, 13)/* [21:13] */

#define OD_REG44 (OD_BASE + 0x07B8)
#define GM_LMIN_THR                REG_FLD(12, 0)/* [11:0] */
#define REP32_2                    REG_FLD(2, 12)/* [13:12] */
#define REP32_3                    REG_FLD(2, 14)/* [15:14] */
#define GM_GMIN_THR                REG_FLD(12, 16)/* [27:16] */
#define REP32_4                    REG_FLD(2, 28)/* [29:28] */
#define REP32_5                    REG_FLD(2, 30)/* [31:30] */

#define OD_REG45 (OD_BASE + 0x07BC)
#define REPEAT_HALF_SHIFT          REG_FLD(1, 0)/* [0:0] */
#define REPEAT_MODE_SEL            REG_FLD(1, 1)/* [1:1] */
#define GM_CENTER_OFFSET           REG_FLD(6, 2)/* [7:2] */
#define DET422_HYST                REG_FLD(2, 8)/* [9:8] */
#define DET422_FORCE               REG_FLD(1, 10)/* [10:10] */
#define DET422_EN                  REG_FLD(1, 11)/* [11:11] */
#define FORCE_RPT_MOTION           REG_FLD(1, 12)/* [12:12] */
#define FORCE_RPT_SEQ              REG_FLD(1, 13)/* [13:13] */
#define FORCE_32                   REG_FLD(1, 14)/* [14:14] */
#define FORCE_22                   REG_FLD(1, 15)/* [15:15] */
#define OD_PCID_ALIG_SEL           REG_FLD(1, 16)/* [16:16] */
#define OD_PCID_ALIG_SEL2          REG_FLD(1, 17)/* [17:17] */
#define OD_PCID_EN                 REG_FLD(1, 18)/* [18:18] */
#define OD_PCID_CSB                REG_FLD(1, 19)/* [19:19] */
#define OD_PCID255_FIX             REG_FLD(1, 20)/* [20:20] */
#define OD_PCID_BYPASS             REG_FLD(1, 21)/* [21:21] */
#define OD_PCID_SWAP_LINE          REG_FLD(1, 22)/* [22:22] */
#define MON_DATA_SEL               REG_FLD(3, 24)/* [26:24] */
#define MON_TIM_SEL                REG_FLD(4, 27)/* [30:27] */

#define OD_REG46 (OD_BASE + 0x07C0)
#define AUTO_Y5_MODE               REG_FLD(1, 0)/* [0:0] */
#define AUTO_Y5_HYST               REG_FLD(4, 1)/* [4:1] */
#define AUTO_Y5_SEL                REG_FLD(2, 5)/* [6:5] */
#define NO_MOTION_DET              REG_FLD(1, 8)/* [8:8] */
#define AUTO_Y5_NO_8B              REG_FLD(1, 9)/* [9:9] */
#define OD_OSD_SEL                 REG_FLD(3, 12)/* [14:12] */
 #define OD_OSD_LINE_EN             REG_FLD(1, 15)/* [15:15] */
#define AUTO_Y5_NUM                REG_FLD(16, 16)/* [31:16] */

#define OD_REG47 (OD_BASE + 0x07C4)
#define ODT_SB_TH4                 REG_FLD(5, 0)/* [4:0] */
#define ODT_SB_TH5                 REG_FLD(5, 5)/* [9:5] */
#define ODT_SB_TH6                 REG_FLD(5, 10)/* [14:10] */
#define ODT_SB_TH7                 REG_FLD(5, 15)/* [19:15] */
#define WOV_CLR                    REG_FLD(1, 20)/* [20:20] */
#define ROV_CLR                    REG_FLD(1, 21)/* [21:21] */
#define SB_INDV_ALPHA              REG_FLD(1, 22)/* [22:22] */
#define PRE_BW                     REG_FLD(6, 24)/* [29:24] */
#define ABTC_POST_FIX              REG_FLD(1, 30)/* [30:30] */
#define OD255_FIX2                 REG_FLD(1, 31)/* [31:31] */

#define OD_REG48 (OD_BASE + 0x07C8)
#define ODT_INDIFF_TH              REG_FLD(8, 0)/* [7:0] */
#define FBT_INDIFF_TH              REG_FLD(8, 8)/* [15:8] */
#define FP_RST_DISABLE             REG_FLD(1, 16)/* [16:16] */
#define FP_POST_RST_DISABLE        REG_FLD(1, 17)/* [17:17] */
#define FP_BYPASS_BLOCK            REG_FLD(1, 18)/* [18:18] */
#define ODT_CSB                    REG_FLD(1, 19)/* [19:19] */
#define FBT_CSB                    REG_FLD(1, 20)/* [20:20] */
#define ODT_FORCE_EN               REG_FLD(1, 22)/* [22:22] */
#define BLOCK_STA_SEL              REG_FLD(4, 23)/* [26:23] */
#define RDY_DELAY_1F               REG_FLD(1, 27)/* [27:27] */
#define HEDGE_SEL                  REG_FLD(1, 28)/* [28:28] */
#define ODT_255_TO_1023            REG_FLD(1, 29)/* [29:29] */
#define NO_RD_FIRST_BYPASS         REG_FLD(1, 30)/* [30:30] */

#define OD_REG49 (OD_BASE + 0x07CC)
#define ASYNC_ECO_DISABLE          REG_FLD(2, 0)/* [1:0] */
#define RDRAM_MODEL                REG_FLD(1, 14)/* [14:14] */
#define DE_PROTECT_EN              REG_FLD(1, 15)/* [15:15] */
#define VDE_PROTECT_EN             REG_FLD(1, 16)/* [16:16] */
#define INT_FP_MON_DE              REG_FLD(1, 17)/* [17:17] */
#define TABLE_MODEL                REG_FLD(1, 18)/* [18:18] */
#define STA_INT_WAIT_HEDGE         REG_FLD(1, 19)/* [19:19] */
#define STA_INT_WAIT_VEDGE         REG_FLD(1, 20)/* [20:20] */
#define ASYNC_OPT                  REG_FLD(1, 21)/* [21:21] */
#define LINE_BUF_AUTO_CLR          REG_FLD(1, 22)/* [22:22] */
#define FIX_INSERT_DE              REG_FLD(1, 23)/* [23:23] */
#define TOGGLE_DE_ERROR            REG_FLD(1, 25)/* [25:25] */
#define SM_ERR_RST_EN              REG_FLD(1, 26)/* [26:26] */
#define ODT_BYPASS_FSYNC           REG_FLD(1, 27)/* [27:27] */
#define FBT_BYPASS_FSYNC           REG_FLD(1, 28)/* [28:28] */
#define PCLK_EN                    REG_FLD(1, 30)/* [30:30] */
#define MCLK_EN                    REG_FLD(1, 31)/* [31:31] */

#define OD_REG50 (OD_BASE + 0x07D0)
#define DUMP_STADR_A               REG_FLD(16, 0)/* [15:0] */
#define DUMP_STADR_B               REG_FLD(16, 16)/* [31:16] */

#define OD_REG51 (OD_BASE + 0x07D4)
#define DUMP_STLINE                REG_FLD(11, 0)/* [10:0] */
#define DUMP_ENDLINE               REG_FLD(11, 11)/* [21:11] */
#define DUMP_DRAM_EN               REG_FLD(1, 22)/* [22:22] */
#define DUMP_BURST_LEN             REG_FLD(2, 23)/* [24:23] */
#define DUMP_OV_CLR                REG_FLD(1, 25)/* [25:25] */
#define DUMP_UD_CLR                REG_FLD(1, 26)/* [26:26] */
#define DUMP_12B_EXT               REG_FLD(2, 27)/* [28:27] */
#define DUMP_ONCE                  REG_FLD(1, 29)/* [29:29] */

#define OD_REG52 (OD_BASE + 0x07D8)
#define LINE_END_DLY               REG_FLD(2, 0)/* [1:0] */
#define AUTO_DET_CSKIP             REG_FLD(1, 2)/* [2:2] */
#define SKIP_COLOR_MODE_EN         REG_FLD(1, 3)/* [3:3] */
#define SKIP_COLOR_HYST            REG_FLD(4, 4)/* [7:4] */
#define DUMP_FIFO_DEPTH            REG_FLD(10, 8)/* [17:8] */
#define DUMP_FIFO_LAST_ADDR        REG_FLD(9, 18)/* [26:18] */
#define DUMP_FSYNC_SEL             REG_FLD(2, 27)/* [28:27] */
#define DUMP_WFF_FULL_CONF         REG_FLD(3, 29)/* [31:29] */

#define OD_REG53 (OD_BASE + 0x07DC)
#define AUTO_Y5_NUM_1              REG_FLD(16, 0)/* [15:0] */
#define FRAME_ERR_CON              REG_FLD(12, 16)/* [27:16] */
#define FP_ERR_STA_CLR             REG_FLD(1, 28)/* [28:28] */
#define FP_X_H                     REG_FLD(1, 29)/* [29:29] */
#define OD_START_SYNC_V            REG_FLD(1, 30)/* [30:30] */
#define OD_EN_SYNC_V               REG_FLD(1, 31)/* [31:31] */

#define OD_REG54 (OD_BASE + 0x07E0)
#define DET8B_BIT_MGN2             REG_FLD(20, 0)/* [19:0] */
#define BYPASS_ALL_SYNC_V          REG_FLD(1, 20)/* [20:20] */
#define OD_INT_MASK                REG_FLD(5, 21)/* [25:21] */
#define OD_STA_INT_CLR             REG_FLD(5, 26)/* [30:26] */
#define OD_NEW_YUV2RGB             REG_FLD(1, 31)/* [31:31] */

#define OD_REG55 (OD_BASE + 0x07E4)
#define OD_ECP_WD_RATIO            REG_FLD(10, 0)/* [9:0] */
#define OD_ECP_THR_HIG             REG_FLD(10, 10)/* [19:10] */
#define OD_ECP_THR_LOW             REG_FLD(10, 20)/* [29:20] */
#define OD_ECP_SEL                 REG_FLD(2, 30)/* [31:30] */

#define OD_REG56 (OD_BASE + 0x07E8)
#define DRAM_UPBOUND               REG_FLD(28, 0)/* [27:0] */
#define DRAM_PROT                  REG_FLD(1, 28)/* [28:28] */
#define OD_TRI_INTERP              REG_FLD(1, 29)/* [29:29] */
#define OD_ECP_EN                  REG_FLD(1, 30)/* [30:30] */
#define OD_ECP_ALL_ON              REG_FLD(1, 31)/* [31:31] */

#define OD_REG57 (OD_BASE + 0x07EC)
#define SKIP_COLOR_THR                REG_FLD(16, 0)/* [15:0] */
#define SKIP_COLOR_THR_1              REG_FLD(16, 16)/* [31:16] */

#define OD_REG62 (OD_BASE + 0x05C0)
#define RG_2CH_PTGEN_COLOR_BAR_TH     REG_FLD(12, 0)/* [11:0] */
#define RG_2CH_PTGEN_TYPE             REG_FLD(3, 12)/* [14:12] */
#define RG_2CH_PTGEN_START            REG_FLD(1, 15)/* [15:15] */
#define RG_2CH_PTGEN_HMOTION          REG_FLD(8, 16)/* [23:16] */
#define RG_2CH_PTGEN_VMOTION          REG_FLD(8, 24)/* [31:24] */

#define OD_REG63 (OD_BASE + 0x05C4)
#define RG_2CH_PTGEN_H_TOTAL          REG_FLD(13, 0)/* [12:0] */
#define RG_2CH_PTGEN_MIRROR           REG_FLD(1, 13)/* [13:13] */
#define RG_2CH_PTGEN_SEQ              REG_FLD(1, 14)/* [14:14] */
#define RG_2CH_PTGEN_2CH_EN           REG_FLD(1, 15)/* [15:15] */
#define RG_2CH_PTGEN_H_ACTIVE         REG_FLD(13, 16)/* [28:16] */
#define RG_2CH_PTGEN_OD_COLOR         REG_FLD(1, 29)/* [29:29] */

#define OD_REG64 (OD_BASE + 0x05C8)
#define RG_2CH_PTGEN_V_TOTAL          REG_FLD(12, 0)/* [11:0] */
#define RG_2CH_PTGEN_V_ACTIVE         REG_FLD(12, 12)/* [23:12] */

#define OD_REG65 (OD_BASE + 0x05CC)
#define RG_2CH_PTGEN_H_START          REG_FLD(13, 0)/* [12:0] */
#define RG_2CH_PTGEN_H_WIDTH          REG_FLD(13, 16)/* [28:16] */

#define OD_REG66 (OD_BASE + 0x05D0)
#define RG_2CH_PTGEN_V_START          REG_FLD(12, 0)/* [11:0] */
#define RG_2CH_PTGEN_V_WIDTH          REG_FLD(12, 12)/* [23:12] */

#define OD_REG67 (OD_BASE + 0x05D4)
#define RG_2CH_PTGEN_B                REG_FLD(10, 0)/* [9:0] */
#define RG_2CH_PTGEN_G                REG_FLD(10, 10)/* [19:10] */
#define RG_2CH_PTGEN_R                REG_FLD(10, 20)/* [29:20] */

#define OD_REG68 (OD_BASE + 0x05D8)
#define RG_2CH_PTGEN_B_BG             REG_FLD(10, 0)/* [9:0] */
#define RG_2CH_PTGEN_G_BG             REG_FLD(10, 10)/* [19:10] */
#define RG_2CH_PTGEN_R_BG             REG_FLD(10, 20)/* [29:20] */

#define OD_REG69 (OD_BASE + 0x05DC)
#define RG_2CH_PTGEN_H_BLOCK_WIDTH    REG_FLD(10, 0)/* [9:0] */
#define RG_2CH_PTGEN_V_BLOCK_WIDTH    REG_FLD(10, 10)/* [19:10] */

#define OD_REG70 (OD_BASE + 0x05E0)
#define RG_2CH_PTGEN_H_BLOCK_OFFSET   REG_FLD(13, 0)/* [12:0] */
#define RG_2CH_PTGEN_V_BLOCK_OFFSET   REG_FLD(12, 16)/* [27:16] */
#define RG_2CH_PTGEN_DIR              REG_FLD(1, 29)/* [29:29] */

#define OD_REG71 (OD_BASE + 0x05E4)
#define RG_WR_HIGH					REG_FLD(6, 0) /* [0:5] */
#define RG_WR_PRE_HIGH				REG_FLD(6, 6) /* [6:11] */
#define RG_WRULTRA_EN				REG_FLD(1, 12)/* [12:12] */
#define RG_WR_LOW					REG_FLD(6, 16)/* [16:21] */
#define RG_WR_PRELOW				REG_FLD(6, 22)/* [22:27] */
#define RG_WGPREULTRA_EN              REG_FLD(1, 28)/* [28:28] */
#define RG_WDRAM_HOLD_EN              REG_FLD(1, 29)/* [29:29] */
#define RG_WDRAM_LEN_X8               REG_FLD(1, 30)/* [30:30] */

#define OD_REG72 (OD_BASE + 0x05E8)
#define RG_RD_HIGH					REG_FLD(6, 0) /* [0:5] */
#define RG_RD_PRE_HIGH				REG_FLD(6, 6) /* [6:11] */
#define RG_RDULTRA_EN				REG_FLD(1, 12)/* [12:12] */
#define RG_RD_LOW					REG_FLD(6, 16)/* [16:21] */
#define RG_RD_PRELOW				REG_FLD(6, 22)/* [22:27] */
#define RG_RGPREULTRA_EN              REG_FLD(1, 28)/* [28:28] */
#define RG_RDRAM_HOLD_EN              REG_FLD(1, 29)/* [29:29] */
#define RG_RDRAM_LEN_X8               REG_FLD(1, 30)/* [30:30] */

#define OD_REG73 (OD_BASE + 0x05EC)
#define RG_WDRAM_HOLD_THR             REG_FLD(9, 0)/* [8:0] */
#define RG_RDRAM_HOLD_THR             REG_FLD(12, 16)/* [27:16] */

#define OD_REG74 (OD_BASE + 0x05F0)
#define OD_REG75 (OD_BASE + 0x05F4)

#define OD_REG76 (OD_BASE + 0x05F8)
#define CHG_Q_FREQ                    REG_FLD(2, 0)/* [1:0] */
#define IP_BTC_ERROR_CNT              REG_FLD(6, 2)/* [7:2] */
#define CURR_Q_UV                     REG_FLD(3, 9)/* [11:9] */
#define CURR_Q_BYPASS                 REG_FLD(3, 12)/* [14:12] */
#define RC_Y_SEL                      REG_FLD(1, 15)/* [15:15] */
#define RC_C_SEL                      REG_FLD(1, 16)/* [16:16] */
#define DUMMY                         REG_FLD(1, 17)/* [17:17] */
#define CURR_Q_UB                     REG_FLD(3, 18)/* [20:18] */
#define CURR_Q_LB                     REG_FLD(3, 21)/* [23:21] */
#define FRAME_INIT_Q                  REG_FLD(3, 24)/* [26:24] */
#define FORCE_CURR_Q_EN               REG_FLD(1, 27)/* [27:27] */
#define FORCE_CURR_Q                  REG_FLD(3, 28)/* [30:28] */
#define SRAM_ALWAYS_ON                REG_FLD(1, 31)/* [31:31] */

#define OD_REG77 (OD_BASE + 0x05FC)
#define RC_U_RATIO                    REG_FLD(9, 0)/* [8:0] */
#define RC_L_RATIO                    REG_FLD(9, 9)/* [17:9] */
#define IP_SAD_TH                     REG_FLD(7, 18)/* [24:18] */
#define VOTE_CHG                      REG_FLD(1, 28)/* [28:28] */
#define NO_CONSECUTIVE_CHG            REG_FLD(1, 29)/* [29:29] */
#define VOTE_THR_SEL                  REG_FLD(2, 30)/* [31:30] */

#define OD_REG78 (OD_BASE + 0x06C0)
#define IP_MODE_MASK                  REG_FLD(8, 0)/* [7:0] */
#define RC_U_RATIO_FIRST2             REG_FLD(9, 8)/* [16:8] */
#define RC_L_RATIO_FIRST2             REG_FLD(9, 17)/* [25:17] */
#define FORCE_1ST_FRAME_END           REG_FLD(2, 26)/* [27:26] */
#define OD_DEC_ECO_DISABLE            REG_FLD(1, 28)/* [28:28] */
#define OD_COMP_1ROW_MODE             REG_FLD(1, 29)/* [29:29] */
#define OD_IP_DATA_SEL                REG_FLD(2, 30)/* [31:30] */

#define OD_REG79 (OD_BASE + 0x06C4)
#define ROD_VIDX0                     REG_FLD(10, 1)/* [10:1] */
#define ROD_EN                        REG_FLD(1, 11)/* [11:11] */
#define ROD_VGAIN_DEC                 REG_FLD(1, 12)/* [12:12] */
#define ROD_VIDX1                     REG_FLD(10, 13)/* [22:13] */
#define ROD_VR2_BASE                  REG_FLD(8, 24)/* [31:24] */

#define OD_REG80 (OD_BASE + 0x06C8)
#define ROD_VR0_BASE                  REG_FLD(8, 0)/* [7:0] */
#define ROD_VR1_BASE                  REG_FLD(8, 8)/* [15:8] */
#define ROD_VR0_STEP                  REG_FLD(8, 16)/* [23:16] */
#define ROD_VR1_STEP                  REG_FLD(8, 24)/* [31:24] */

#define OD_REG81 (OD_BASE + 0x06CC)
#define ROD_IDX0                      REG_FLD(8, 0)/* [7:0] */
#define ROD_IDX1                      REG_REG_FLD(8, 8)/* [15:8] */
#define ROD_IDX2                      REG_FLD(8, 16)/* [23:16] */
#define ROD_VR2_STEP                  REG_FLD(8, 24)/* [31:24] */

#define OD_REG82 (OD_BASE + 0x06D0)
#define ROD_R0_GAIN                   REG_FLD(8, 0)/* [7:0] */
#define ROD_R1_GAIN                   REG_FLD(8, 8)/* [15:8] */
#define ROD_R2_GAIN                   REG_FLD(8, 16)/* [23:16] */
#define ROD_R3_GAIN                   REG_FLD(8, 24)/* [31:24] */

#define OD_REG83 (OD_BASE + 0x06D4)
#define ROD_R0_OFFSET                 REG_FLD(8, 0)/* [7:0] */
#define ROD_R1_OFFSET                 REG_FLD(8, 8)/* [15:8] */
#define ROD_R2_OFFSET                 REG_FLD(8, 16)/* [23:16] */
#define ROD_R3_OFFSET                 REG_FLD(8, 24)/* [31:24] */

#define OD_REG84 (OD_BASE + 0x06D8)
#define ROD_R4_GAIN                   REG_FLD(8, 0)/* [7:0] */
#define ROD_R5_GAIN                   REG_FLD(8, 8)/* [15:8] */
#define ROD_R6_GAIN                   REG_FLD(8, 16)/* [23:16] */
#define ROD_R7_GAIN                   REG_FLD(8, 24)/* [31:24] */

#define OD_REG85 (OD_BASE + 0x06DC)
#define ROD_R4_OFFSET                 REG_FLD(8, 0)/* [7:0] */
#define ROD_R5_OFFSET                 REG_FLD(8, 8)/* [15:8] */
#define ROD_R6_OFFSET                 REG_FLD(8, 16)/* [23:16] */
#define ROD_R7_OFFSET                 REG_FLD(8, 24)/* [31:24] */

#define OD_REG86 (OD_BASE + 0x06E0)
#define RFB_VIDX0                     REG_FLD(10, 1)/* [10:1] */
#define RFB_EN                        REG_FLD(1, 11)/* [11:11] */
#define RFB_VGAIN_DEC                 REG_FLD(1, 12)/* [12:12] */
#define RFB_VIDX1                     REG_FLD(10, 13)/* [22:13] */
#define RFB_VR2_BASE                  REG_FLD(8, 24)/* [31:24] */

#define OD_REG87 (OD_BASE + 0x06E4)
#define RFB_VR0_BASE                  REG_FLD(8, 0)/* [7:0] */
#define RFB_VR1_BASE                  REG_FLD(8, 8)/* [15:8] */
#define RFB_VR0_STEP                  REG_FLD(8, 16)/* [23:16] */
#define RFB_VR1_STEP                  REG_FLD(8, 24)/* [31:24] */

#define OD_REG88 (OD_BASE + 0x06E8)
#define IP_SAD_TH2                    REG_FLD(7, 0)/* [6:0] */
#define IP_SAD_TH2_UB                 REG_FLD(8, 8)/* [15:8] */
#define IP_SAD_TH2_LB                 REG_FLD(8, 16)/* [23:16] */
#define RFB_VR2_STEP                  REG_FLD(8, 24)/* [31:24] */

#define OD_REG89 (OD_BASE + 0x06EC)
#define IP_SAD_TH3                    REG_FLD(7, 0)/* [6:0] */
#define IP_SAD_TH3_UB                 REG_FLD(8, 8)/* [15:8] */
#define IP_SAD_TH3_LB                 REG_FLD(8, 16)/* [23:16] */

#define OD_REG_CRC32_0 (OD_BASE + 0x0580)
#define OD_CRC32_TOP_L_EN             REG_FLD(1, 0)/* [0:0] */
#define OD_CRC32_TOP_R_EN             REG_FLD(1, 1)/* [1:1] */
#define OD_CRC32_EVEN_LINE_EN         REG_FLD(1, 2)/* [2:2] */
#define OD_CRC32_ODD_LINE_EN          REG_FLD(1, 3)/* [3:3] */
#define OD_CRC32_CHECK_SUM_MODE       REG_FLD(1, 4)/* [4:4] */
#define OD_CRC32_STILL_CHECK_TRIG     REG_FLD(1, 5)/* [5:5] */
#define OD_CRC32_CLEAR_READY          REG_FLD(1, 6)/* [6:6] */
#define OD_CRC32_VSYNC_INV            REG_FLD(1, 7)/* [7:7] */
#define OD_CRC32_STILL_CHECK_MAX      REG_FLD(8, 8)/* [15:8] */

#define OD_REG_CRC32_1 (OD_BASE + 0x0584)
#define OD_CRC32_CLIP_H_START         REG_FLD(13, 0)/* [12:0] */
#define OD_CRC32_CLIP_V_START         REG_FLD(12, 16)/* [27:16] */

#define OD_REG_CRC32_2 (OD_BASE + 0x0588)
#define OD_CRC32_CLIP_H_END           REG_FLD(13, 0)/* [12:0] */
#define OD_CRC32_CLIP_V_END           REG_FLD(12, 16)/* [27:16] */

#define OD_REG_LT_00 (OD_BASE + 0x0500)
#define REGION_0_POS                  REG_FLD(13, 0)/* [12:0] */
#define REGION_1_POS                  REG_FLD(13, 13)/* [25:13] */
#define LOCA_TABLE_EN                 REG_FLD(1, 26)/* [26:26] */
#define REGION_H_BLEND_SEL            REG_FLD(2, 27)/* [28:27] */
#define LT_USE_PCID                   REG_FLD(1, 29)/* [29:29] */
#define LT_READ_NEW_TABLE             REG_FLD(1, 30)/* [30:30] */
#define SWITCH_TABLE_DMA_SRC          REG_FLD(1, 31)/* [31:31] */

#define OD_REG_LT_01 (OD_BASE + 0x0504)
#define REGION_2_POS                  REG_FLD(13, 0)/* [12:0] */
#define ROW_0_POS                     REG_FLD(12, 13)/* [24:13] */
#define LOCAL_TABLE_DELAY             REG_FLD(3, 25)/* [27:25] */
#define LT_DRAM_WAIT_SEL              REG_FLD(1, 28)/* [28:28] */
#define LT_DONE_MASK_SEL              REG_FLD(1, 29)/* [29:29] */

#define OD_REG_LT_02 (OD_BASE + 0x0508)
#define ROW_1_POS                     REG_FLD(12, 0)/* [11:0] */
#define ROW_2_POS                     REG_FLD(12, 12)/* [23:12] */

#define OD_REG_LT_03 (OD_BASE + 0x050C)
#define ROW_3_POS                     REG_FLD(12, 0)/* [11:0] */
#define LT_PAUSE_LEN                  REG_FLD(8, 24)/* [31:24] */

#define OD_REG_LT_04 (OD_BASE + 0x0510)
#define LT_BASE_ADDR                  REG_FLD(28, 0)/* [27:0] */
#define LT_BURST_LEN                  REG_FLD(2, 28)/* [29:28] */
#define LT_CLR_UNDERFLOW              REG_FLD(1, 30)/* [30:30] */
#define LT_RDRAM_X1                   REG_FLD(1, 31)/* [31:31] */

#define OD_STA00 (OD_BASE + 0x0680)
#define STA_GM_GMIN_422               REG_FLD(4, 0)/* [3:0] */
#define STA_GM_XV                     REG_FLD(3, 4)/* [6:4] */
#define OD_RDY                        REG_FLD(1, 7)/* [7:7] */
#define STA_GM_LMIN                   REG_FLD(4, 8)/* [11:8] */
#define STA_GM_GMIN                   REG_FLD(4, 12)/* [15:12] */
#define STA_GM_MISC                   REG_FLD(4, 16)/* [19:16] */
#define STA_CSKIP_DET                 REG_FLD(1, 20)/* [20:20] */
#define EFP_BYPASS                    REG_FLD(1, 21)/* [21:21] */
#define RD_ASF_UFLOW                  REG_FLD(1, 22)/* [22:22] */
#define DRAM_CRC_ERROR                REG_FLD(1, 23)/* [23:23] */
#define FP_BYPASS                     REG_FLD(1, 24)/* [24:24] */
#define FP_BYPASS_INT                 REG_FLD(1, 25)/* [25:25] */
#define COMP_Y5_MODE                  REG_FLD(1, 26)/* [26:26] */
#define BTC_8B                        REG_FLD(1, 27)/* [27:27] */
#define DE_8B                         REG_FLD(1, 28)/* [28:28] */
#define DET_Y5_MODE                   REG_FLD(1, 29)/* [29:29] */
#define DET_BTC_8B                    REG_FLD(1, 30)/* [30:30] */
#define DET_DC_8B                     REG_FLD(1, 31)/* [31:31] */

#define OD_STA01 (OD_BASE + 0x0684)
#define BLOCK_STA_CNT                 REG_FLD(18, 0)/* [17:0] */
#define DUMP_UD_FLAG                  REG_FLD(1, 18)/* [18:18] */
#define DUMP_OV_FLAG                  REG_FLD(1, 19)/* [19:19] */
#define STA_TIMING_H                  REG_FLD(12, 20)/* [31:20] */

#define OD_STA02 (OD_BASE + 0x0688)
#define STA_FRAME_BIT                 REG_FLD(13, 0)/* [12:0] */
#define R_UNDERFLOW                   REG_FLD(1, 13)/* [13:13] */
#define W_UNDERFLOW                   REG_FLD(1, 14)/* [14:14] */
#define MOT_DEBUG                     REG_FLD(1, 15)/* [15:15] */
#define DISP_MISMATCH                 REG_FLD(1, 16)/* [16:16] */
#define DE_MISMATCH                   REG_FLD(1, 17)/* [17:17] */
#define V_MISMATCH                    REG_FLD(1, 18)/* [18:18] */
#define H_MISMATCH                    REG_FLD(1, 19)/* [19:19] */
#define STA_TIMING_L                  REG_FLD(12, 20)/* [31:20] */

#define OD_STA03 (OD_BASE + 0x068C)
#define STA_LINE_BIT                  REG_FLD(15, 0)/* [14:0] */
#define SRAM_POOL_ACCESS_ERR          REG_FLD(1, 15)/* [15:15] */
#define RW_TABLE_ERROR                REG_FLD(1, 16)/* [16:16] */
#define RW_TABLE_RDRAM_UNDERFLOW      REG_FLD(1, 17)/* [17:17] */

#define OD_STA04 (OD_BASE + 0x0690)
#define STA_TIMING                    REG_FLD(1, 8)/* [8:8] */
#define STA_DATA                      REG_FLD(8, 0)/* [7:0] */
#define STA_DUMP_WDRAM_ADDR           REG_FLD(10, 9)/* [18:9] */
#define STA_DUMP_WDRAM_WDATA          REG_FLD(10, 19)/* [28:19] */
#define DUMP_WDRAM_REQ                REG_FLD(1, 29)/* [29:29] */
#define DUMP_WDRAM_ALE                REG_FLD(1, 30)/* [30:30] */
#define DUMP_WDRAM_SWITCH             REG_FLD(1, 31)/* [31:31] */

#define OD_STA05 (OD_BASE + 0x0694)
#define STA_IFM                       REG_FLD(32, 0)/* [31:0] */

#define OD_STA06 (OD_BASE + 0x0698)
#define STA_FP_ERR                    REG_FLD(12, 0)/* [11:0] */
#define STA_INT_WAIT_LCNT             REG_FLD(8, 12)/* [19:12] */
#define CURR_Q                        REG_FLD(3, 20)/* [22:20] */
#define TABLE_DMA_PERIOD_CONF         REG_FLD(1, 29)/* [29:29] */
#define TABLE_DMA_PERIOD              REG_FLD(1, 30)/* [30:30] */
#define TABLE_DMA_DONE                REG_FLD(1, 31)/* [31:31] */

#define OD_STA_CRC32_0 (OD_BASE + 0x0540)
#define STA_CRC32_CRC_OUT_H           REG_FLD(32, 0)  /*[31:0] */

#define OD_STA_CRC32_1 (OD_BASE + 0x0544)
#define STA_CRC32_CRC_OUT_V           REG_FLD(32, 0)  /*[31:0] */

#define OD_STA_CRC32_2 (OD_BASE + 0x0548)
#define STA_CRC32_NON_STILL_CNT       REG_FLD(4, 0)   /*[3:0] */
#define STA_CRC32_STILL_CEHCK_DONE    REG_FLD(1, 4)   /*[4:4] */
#define STA_CRC32_CRC_READY           REG_FLD(1, 5)   /*[5:5] */
#define STA_CRC32_CRC_OUT_V_READY     REG_FLD(1, 6)   /*[6:6] */
#define STA_CRC32_CRC_OUT_H_READY     REG_FLD(1, 7)   /*[7:7] */
#define LOCAL_TABLE_READ_DRAM_DONE    REG_FLD(1, 8)   /*[8:8] */
#define LOCAL_TABLE_READ_DRAM         REG_FLD(1, 9)   /*[9:9] */
#define LOCAL_TABLE_CURRENT_ROW       REG_FLD(3, 10)  /*[12:10] */
#define FIFO_FULL                     REG_FLD(1, 13)  /*[13:13] */
#define FIFO_EMPTY                    REG_FLD(1, 14)  /*[14:14] */
#define WRITE_TABLE                   REG_FLD(1, 15)  /*[15:15] */
#define START_READ_DRAM               REG_FLD(1, 16)  /*[16:16] */
#define WAIT_FIFO_M                   REG_FLD(1, 17)  /*[17:17] */
#define WAIT_FIFO                     REG_FLD(1, 18)  /*[18:18] */
#define READ_DRAM_DONE_MASK           REG_FLD(1, 19)  /*[19:19] */
#define WAIT_NEXT_START_READ_DRAM     REG_FLD(1, 20)  /*[20:20] */
#define RDRAM_REQ                     REG_FLD(1, 21)  /*[21:21] */
#define LOCAL_TABLE_DRAM_FSM          REG_FLD(2, 22)  /*[23:22] */
#define LOCAL_TABLE_TABLE_FSM         REG_FLD(6, 24)  /*[29:24] */
#define WRITE_TABLE_DONE              REG_FLD(1, 30)  /*[30:30] */
#define PKT_CRC_ERROR                 REG_FLD(1, 31)  /*[31:31] */
#define OD_FLD_ALL                    REG_FLD(32, 0) /*[31:0] */
/* ------------------------ End of register definitions ------------------------ */


#define OD_ALLOW_DEFAULT_TABLE
/* #define OD_LINEAR_TABLE_IF_NONE */


#define OD_USE_CMDQ     0

#define OD_TBL_S_DIM    17
#define OD_TBL_M_DIM    33

/* compression ratio */
#define OD_MANUAL_CPR   58 /* 38 */
#define OD_HSYNC_WIDTH  100
#define OD_LINESIZE_BUFFER 8


/* Additional space after the given memory to avoid unexpected memory corruption */
/* In the experiments, OD hardware overflew 240 bytes */
#define OD_ADDITIONAL_BUFFER 256

#define OD_GUARD_PATTERN 0x16881688
#define OD_GUARD_PATTERN_SIZE 4

#define OD_TBL_S_SIZE   (OD_TBL_S_DIM * OD_TBL_S_DIM)
#define OD_TBL_M_SIZE   (OD_TBL_M_DIM * OD_TBL_M_DIM)
#define FB_TBL_SIZE     (FB_TBL_DIM * FB_TBL_DIM)

#define OD_DBG_ALWAYS   0
#define OD_DBG_VERBOSE  1
#define OD_DBG_DEBUG    2

#define OD_ENABLE_FRMPERIOD_THRESHOLD 10000 /* period of 100 FPS = 100000 microsec */
#define OD_CHECK_FPS_INTERVAL 500000 /* 1000 microsec * 500 = 0.5 sec */
#define OD_CHECK_ERR_INTERVAL 8000 /* 1000 microsec * 8 = 8 mini sec */

#define OD_REG_SET_FIELD(cmdq, reg, val, field) DISP_REG_SET_FIELD(cmdq, field, reg, val)
#define OD_REG_GET(reg32) DISP_REG_GET((unsigned long)(reg32))
#define OD_REG_SET(handle, reg32, val) DISP_REG_SET(handle, (unsigned long)(reg32), val)


#define ABS(a) ((a > 0) ? a : -a)


/* debug macro */
#define ODDBG(level, fmt, arg...) \
	do { \
		if (od_debug_level >= (level)) \
			pr_debug("[OD] " fmt "\n", ##arg); \
	} while (0)
#define ODERR(fmt, arg...) pr_err("[OD] " fmt "\n", ##arg)
#define ODNOTICE(fmt, arg...) pr_notice("[OD] " fmt "\n", ##arg)


/* ioctl */
typedef enum {
	OD_CTL_READ_REG,
	OD_CTL_WRITE_REG,
	OD_CTL_ENABLE_DEMO_MODE,
	OD_CTL_RUN_TEST,
	OD_CTL_WRITE_TABLE,
	OD_CTL_CMD_NUM,
	OD_CTL_ENABLE
} DISP_OD_CMD_TYPE;

typedef enum {
	OD_CTL_ENABLE_OFF,
	OD_CTL_ENABLE_ON
} DISP_OD_ENABLE_STAGE;

enum {
	OD_RED,
	OD_GREEN,
	OD_BLUE,
	OD_ALL
};

enum {
	OD_TABLE_17,
	OD_TABLE_33
};

typedef enum {
	OD_STATUS_IDLE,
	OD_STATUS_ENABLE,
	OD_STATUS_DISABLE,
	OD_STATUS_BYPASS,
	OD_STATUS_RESET
} OD_STATUS;

static void od_dbg_dump(void);

typedef struct REG_INIT_TABLE_STRUCT {
	unsigned long reg_addr;
	unsigned int reg_value;
	unsigned int reg_mask;

} REG_INIT_TABLE_STRUCT;


/* dummy 17x17 */
static u8 OD_Table_dummy_17x17[OD_TBL_S_SIZE] = {
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255,
};


#if 0

/* 60 hz */
static u8 OD_Table_17x17[OD_TBL_S_SIZE] = {
0, 18, 36, 56, 76, 95, 114, 132, 148, 163, 177, 192, 208, 223, 236, 248, 255,
0, 16, 34, 53, 73, 92, 112, 129, 146, 161, 176, 191, 207, 220, 235, 248, 255,
0, 15, 32, 50, 69, 91, 109, 127, 143, 160, 174, 189, 205, 221, 234, 247, 255,
0, 14, 29, 48, 67, 87, 106, 123, 140, 156, 172, 187, 203, 220, 234, 247, 255,
0, 13, 27, 45, 64, 84, 103, 120, 138, 155, 170, 185, 202, 219, 233, 246, 255,
0, 10, 25, 43, 61, 80, 99, 117, 135, 152, 169, 184, 200, 217, 232, 246, 255,
0, 8, 24, 40, 58, 77, 96, 115, 133, 150, 167, 183, 199, 216, 231, 245, 255,
0, 6, 23, 39, 56, 74, 93, 112, 130, 148, 165, 182, 198, 214, 230, 245, 255,
0, 5, 22, 38, 54, 71, 89, 108, 128, 146, 164, 180, 197, 213, 229, 244, 255,
0, 4, 20, 35, 51, 68, 86, 105, 125, 144, 162, 179, 196, 212, 228, 243, 255,
0, 3, 18, 33, 48, 64, 83, 102, 122, 141, 160, 177, 194, 211, 227, 243, 255,
0, 1, 16, 31, 46, 63, 80, 99, 119, 138, 157, 176, 193, 210, 227, 242, 255,
0, 0, 13, 28, 43, 59, 77, 96, 116, 136, 155, 173, 192, 209, 226, 242, 255,
0, 0, 11, 26, 41, 56, 74, 93, 113, 133, 152, 171, 189, 208, 225, 241, 255,
0, 0, 9, 24, 39, 54, 71, 90, 110, 130, 150, 169, 187, 205, 224, 240, 255,
0, 0, 7, 21, 36, 50, 68, 86, 106, 126, 147, 166, 184, 202, 222, 240, 255,
0, 0, 4, 18, 32, 47, 62, 82, 103, 123, 143, 163, 181, 199, 219, 238, 255
};

#else

/* 120 hz */
static u8 OD_Table_17x17[OD_TBL_S_SIZE] = {
0, 21, 44, 71, 99, 122, 141, 160, 175, 190, 203, 216, 228, 239, 251, 255, 255,
0, 16, 38, 65, 90, 115, 136, 153, 171, 186, 200, 213, 226, 237, 249, 255, 255,
0, 10, 32, 56, 83, 107, 130, 148, 166, 182, 196, 210, 223, 236, 248, 255, 255,
0, 4, 25, 48, 73, 98, 120, 142, 159, 176, 191, 206, 219, 234, 246, 255, 255,
0, 0, 19, 41, 64, 88, 113, 135, 154, 171, 188, 203, 217, 231, 245, 255, 255,
0, 0, 14, 35, 56, 80, 104, 126, 148, 166, 184, 200, 214, 229, 243, 255, 255,
0, 0, 9, 28, 49, 71, 96, 119, 141, 161, 179, 196, 212, 227, 241, 255, 255,
0, 0, 4, 23, 42, 64, 87, 112, 135, 155, 175, 192, 210, 226, 240, 254, 255,
0, 0, 0, 18, 37, 55, 78, 103, 128, 150, 171, 190, 208, 226, 239, 253, 255,
0, 0, 0, 12, 31, 49, 71, 95, 119, 144, 166, 186, 205, 224, 238, 252, 255,
0, 0, 0, 8, 24, 42, 62, 86, 111, 136, 160, 181, 202, 222, 237, 251, 255,
0, 0, 0, 3, 19, 36, 54, 77, 102, 127, 152, 176, 197, 216, 235, 250, 255,
0, 0, 0, 0, 13, 28, 47, 68, 94, 120, 145, 169, 192, 213, 232, 248, 255,
0, 0, 0, 0, 8, 22, 40, 59, 83, 110, 136, 161, 183, 208, 228, 245, 255,
0, 0, 0, 0, 2, 16, 30, 50, 72, 99, 127, 154, 175, 196, 224, 242, 255,
0, 0, 0, 0, 0, 8, 22, 39, 60, 84, 112, 142, 165, 192, 218, 240, 255,
0, 0, 0, 0, 0, 1, 15, 28, 48, 70, 99, 130, 158, 188, 213, 235, 255
};

#endif


#if 0
/* dummy 33x33 */
static u8 OD_Table_33x33[OD_TBL_M_SIZE] = {
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255,
0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120, 128, 136,
144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255
};

#endif

#if 1
/* 60 hz */
static u8 OD_Table_33x33[OD_TBL_M_SIZE] = {
0, 9, 18, 26, 36, 46, 56, 66, 76, 86, 95, 105, 114, 123, 132, 140, 148,
155, 163, 170, 177, 185, 192, 200, 208, 216, 223, 230, 236, 242, 248, 254, 255,
0, 8, 17, 26, 35, 45, 55, 65, 75, 84, 94, 104, 113, 122, 131, 139, 147,
155, 162, 169, 177, 184, 192, 199, 207, 215, 222, 229, 236, 242, 248, 254, 255,
0, 8, 16, 25, 34, 44, 53, 63, 73, 83, 92, 103, 112, 121, 129, 137, 146,
154, 161, 169, 176, 183, 191, 198, 207, 215, 220, 229, 235, 241, 248, 254, 255,
0, 7, 16, 24, 33, 42, 52, 61, 71, 82, 91, 101, 110, 119, 128, 136, 144,
153, 161, 168, 175, 182, 190, 198, 206, 214, 220, 229, 235, 241, 247, 254, 255,
0, 7, 15, 24, 32, 41, 50, 59, 69, 80, 91, 100, 109, 118, 127, 135, 143,
152, 160, 167, 174, 182, 189, 197, 205, 213, 221, 228, 234, 241, 247, 253, 255,
0, 7, 15, 23, 31, 40, 49, 58, 68, 78, 89, 98, 107, 116, 125, 134, 142,
150, 158, 166, 173, 181, 188, 196, 204, 212, 221, 228, 234, 240, 247, 253, 255,
0, 6, 14, 22, 29, 39, 48, 57, 67, 77, 87, 96, 106, 115, 123, 132, 140,
149, 156, 165, 172, 180, 187, 195, 203, 212, 220, 227, 234, 240, 247, 253, 255,
0, 6, 13, 21, 28, 37, 47, 56, 65, 75, 85, 95, 104, 113, 122, 131, 139,
147, 155, 164, 171, 179, 186, 194, 202, 211, 219, 227, 233, 240, 246, 253, 255,
0, 5, 13, 20, 27, 36, 45, 55, 64, 74, 84, 94, 103, 111, 120, 129, 138,
146, 155, 163, 170, 178, 185, 193, 202, 210, 219, 226, 233, 239, 246, 253, 255,
0, 4, 11, 19, 26, 35, 44, 53, 63, 72, 82, 92, 101, 110, 119, 127, 136,
145, 154, 162, 169, 177, 185, 193, 201, 209, 218, 226, 232, 239, 246, 252, 255,
0, 3, 10, 18, 25, 33, 43, 52, 61, 71, 80, 90, 99, 108, 117, 126, 135,
144, 152, 161, 169, 176, 184, 192, 200, 209, 217, 225, 232, 239, 246, 252, 255,
0, 1, 9, 17, 24, 32, 42, 51, 60, 69, 79, 88, 98, 107, 116, 125, 134,
143, 151, 160, 168, 176, 184, 191, 200, 208, 216, 224, 231, 238, 245, 252, 255,
0, 0, 8, 16, 24, 32, 40, 49, 58, 68, 77, 86, 96, 105, 115, 124, 133,
142, 150, 159, 167, 175, 183, 191, 199, 207, 216, 224, 231, 238, 245, 252, 255,
0, 0, 7, 15, 23, 32, 40, 49, 57, 67, 76, 85, 94, 104, 113, 123, 132,
140, 149, 157, 166, 174, 182, 190, 199, 207, 215, 222, 230, 238, 245, 252, 255,
0, 0, 6, 15, 23, 31, 39, 48, 56, 66, 74, 83, 93, 102, 112, 121, 130,
139, 148, 156, 165, 174, 182, 190, 198, 206, 214, 222, 230, 237, 245, 252, 255,
0, 0, 6, 14, 22, 31, 39, 47, 55, 64, 73, 82, 91, 100, 110, 120, 129,
138, 147, 155, 164, 173, 181, 189, 198, 206, 214, 221, 229, 237, 244, 252, 255,
0, 0, 5, 13, 22, 30, 38, 46, 54, 62, 71, 80, 89, 99, 108, 118, 128,
137, 146, 155, 164, 172, 180, 189, 197, 205, 213, 221, 229, 236, 244, 251, 255,
0, 0, 4, 13, 21, 29, 37, 45, 53, 61, 69, 79, 88, 97, 107, 117, 126,
136, 145, 154, 163, 171, 180, 188, 196, 204, 213, 221, 229, 236, 244, 251, 255,
0, 0, 4, 12, 20, 27, 35, 43, 51, 59, 68, 77, 86, 96, 105, 115, 125,
135, 144, 153, 162, 170, 179, 187, 196, 204, 212, 220, 228, 236, 243, 251, 255,
0, 0, 3, 11, 19, 26, 34, 42, 50, 58, 66, 75, 85, 94, 104, 114, 123,
133, 143, 152, 161, 169, 178, 186, 195, 203, 211, 220, 228, 235, 243, 250, 255,
0, 0, 3, 10, 18, 25, 33, 41, 48, 56, 64, 73, 83, 92, 102, 112, 122,
132, 141, 151, 160, 169, 177, 186, 194, 202, 211, 219, 227, 235, 243, 250, 255,
0, 0, 2, 9, 17, 24, 32, 39, 47, 55, 64, 72, 81, 91, 101, 111, 120,
130, 140, 149, 159, 168, 177, 185, 194, 202, 210, 219, 227, 235, 242, 250, 255,
0, 0, 1, 8, 16, 23, 31, 38, 46, 53, 63, 70, 80, 89, 99, 109, 119,
129, 138, 148, 157, 167, 176, 185, 193, 202, 210, 218, 227, 234, 242, 250, 255,
0, 0, 0, 7, 15, 22, 30, 37, 45, 52, 61, 69, 78, 88, 98, 108, 118,
127, 137, 147, 156, 165, 175, 184, 193, 201, 210, 218, 226, 234, 242, 250, 255,
0, 0, 0, 6, 13, 21, 28, 36, 43, 51, 59, 67, 77, 86, 96, 106, 116,
126, 136, 145, 155, 164, 173, 183, 192, 201, 209, 218, 226, 234, 242, 250, 255,
0, 0, 0, 5, 12, 20, 27, 35, 42, 50, 57, 67, 75, 85, 95, 105, 115,
125, 134, 144, 154, 163, 172, 181, 191, 200, 209, 217, 225, 233, 241, 249, 255,
0, 0, 0, 4, 11, 19, 26, 34, 41, 49, 56, 66, 74, 83, 93, 103, 113,
123, 133, 143, 152, 162, 171, 180, 189, 199, 208, 217, 225, 233, 241, 249, 255,
0, 0, 0, 3, 10, 18, 25, 33, 40, 48, 55, 64, 72, 82, 92, 101, 111,
121, 131, 141, 151, 161, 170, 179, 188, 197, 207, 216, 224, 232, 240, 248, 255,
0, 0, 0, 2, 9, 17, 24, 32, 39, 47, 54, 61, 71, 80, 90, 100, 110,
120, 130, 140, 150, 160, 169, 178, 187, 196, 205, 215, 224, 232, 240, 248, 255,
0, 0, 0, 0, 8, 15, 23, 30, 37, 45, 52, 60, 70, 78, 88, 98, 108,
118, 128, 138, 148, 158, 167, 176, 185, 195, 204, 213, 223, 232, 240, 248, 255,
0, 0, 0, 0, 7, 14, 21, 29, 36, 43, 50, 58, 68, 76, 86, 96, 106,
116, 126, 137, 147, 156, 166, 175, 184, 193, 202, 212, 222, 231, 240, 248, 255,
0, 0, 0, 0, 5, 12, 20, 27, 34, 41, 49, 56, 65, 74, 84, 94, 104,
115, 125, 135, 145, 154, 164, 173, 182, 191, 201, 211, 220, 230, 239, 248, 255,
0, 0, 0, 0, 4, 11, 18, 25, 32, 40, 47, 54, 62, 71, 82, 92, 103,
113, 123, 133, 143, 153, 163, 172, 181, 189, 199, 209, 219, 229, 238, 247, 255
};

#else

/* 120 hz */
static u8 OD_Table_33x33[OD_TBL_M_SIZE] = {
0, 11, 21, 31, 44, 57, 71, 86, 99, 111, 122, 132, 141, 151, 160, 168, 175,
183, 190, 197, 203, 209, 216, 222, 228, 234, 239, 245, 251, 255, 255, 255, 255,
0, 8, 18, 28, 41, 54, 68, 81, 95, 107, 118, 128, 139, 148, 156, 164, 173,
181, 188, 194, 202, 208, 214, 221, 227, 233, 238, 244, 250, 255, 255, 255, 255,
0, 5, 16, 25, 38, 50, 65, 77, 90, 103, 115, 125, 136, 145, 153, 162, 171,
178, 186, 192, 200, 206, 213, 219, 226, 232, 237, 243, 249, 255, 255, 255, 255,
0, 2, 13, 24, 35, 47, 61, 73, 86, 100, 111, 122, 133, 142, 151, 159, 168,
176, 184, 190, 198, 205, 211, 218, 224, 231, 237, 243, 249, 255, 255, 255, 255,
0, 0, 10, 21, 32, 44, 56, 69, 83, 96, 107, 119, 130, 139, 148, 157, 166,
174, 182, 190, 196, 203, 210, 217, 223, 229, 236, 242, 248, 254, 255, 255, 255,
0, 0, 7, 18, 29, 40, 52, 65, 78, 91, 103, 115, 125, 135, 145, 154, 162,
171, 179, 187, 193, 201, 208, 215, 221, 228, 235, 241, 247, 254, 255, 255, 255,
0, 0, 4, 15, 25, 36, 48, 60, 73, 86, 98, 110, 120, 131, 142, 151, 159,
168, 176, 185, 191, 199, 206, 213, 219, 227, 234, 240, 246, 253, 255, 255, 255,
0, 0, 1, 12, 22, 33, 44, 56, 68, 81, 93, 105, 116, 128, 138, 148, 156,
166, 174, 182, 189, 197, 205, 212, 218, 226, 232, 239, 246, 252, 255, 255, 255,
0, 0, 0, 9, 19, 29, 41, 52, 64, 76, 88, 101, 113, 125, 135, 144, 154,
163, 171, 179, 188, 195, 203, 210, 217, 224, 231, 238, 245, 252, 255, 255, 255,
0, 0, 0, 7, 16, 26, 38, 48, 60, 72, 84, 96, 108, 120, 130, 141, 151,
159, 169, 177, 186, 193, 201, 209, 216, 223, 230, 237, 244, 251, 255, 255, 255,
0, 0, 0, 4, 14, 24, 35, 45, 56, 67, 80, 92, 104, 116, 126, 138, 148,
156, 166, 175, 184, 191, 200, 207, 214, 221, 229, 236, 243, 250, 255, 255, 255,
0, 0, 0, 2, 12, 21, 32, 41, 52, 63, 76, 88, 100, 112, 123, 134, 144,
153, 164, 173, 182, 189, 198, 206, 213, 220, 228, 235, 242, 249, 255, 255, 255,
0, 0, 0, 0, 9, 18, 28, 38, 49, 59, 71, 84, 96, 108, 119, 130, 141,
151, 161, 170, 179, 188, 196, 204, 212, 219, 227, 234, 241, 249, 255, 255, 255,
0, 0, 0, 0, 7, 16, 25, 36, 45, 56, 68, 79, 92, 104, 116, 127, 138,
148, 157, 168, 177, 186, 194, 203, 211, 219, 226, 234, 241, 248, 255, 255, 255,
0, 0, 0, 0, 4, 13, 23, 33, 42, 52, 64, 75, 87, 100, 112, 124, 135,
145, 155, 166, 175, 184, 192, 202, 210, 218, 226, 233, 240, 247, 254, 255, 255,
0, 0, 0, 0, 1, 11, 20, 30, 39, 49, 60, 70, 83, 95, 108, 120, 131,
142, 152, 163, 173, 182, 190, 200, 209, 218, 226, 233, 240, 247, 254, 255, 255,
0, 0, 0, 0, 0, 8, 18, 27, 37, 46, 55, 66, 78, 91, 103, 116, 128,
139, 150, 161, 171, 180, 190, 199, 208, 217, 226, 232, 239, 246, 253, 255, 255,
0, 0, 0, 0, 0, 6, 15, 24, 34, 43, 52, 63, 75, 86, 99, 111, 124,
136, 147, 158, 168, 178, 188, 197, 207, 216, 225, 232, 239, 246, 253, 255, 255,
0, 0, 0, 0, 0, 4, 12, 21, 31, 39, 49, 60, 71, 82, 95, 107, 119,
132, 144, 156, 166, 176, 186, 196, 205, 215, 224, 231, 238, 245, 252, 255, 255,
0, 0, 0, 0, 0, 2, 10, 18, 28, 36, 46, 56, 67, 78, 90, 103, 115,
128, 140, 152, 163, 173, 184, 194, 204, 214, 224, 231, 238, 245, 252, 255, 255,
0, 0, 0, 0, 0, 0, 8, 16, 24, 33, 42, 52, 62, 74, 86, 98, 111,
123, 136, 148, 160, 171, 181, 192, 202, 212, 222, 230, 237, 244, 251, 255, 255,
0, 0, 0, 0, 0, 0, 5, 13, 21, 31, 40, 48, 58, 70, 82, 94, 106,
119, 131, 144, 156, 168, 179, 189, 200, 210, 219, 229, 236, 243, 251, 255, 255,
0, 0, 0, 0, 0, 0, 3, 11, 19, 28, 36, 45, 54, 66, 77, 89, 102,
115, 127, 140, 152, 165, 176, 187, 197, 208, 216, 227, 235, 242, 250, 255, 255,
0, 0, 0, 0, 0, 0, 1, 8, 16, 25, 33, 41, 51, 61, 72, 85, 98,
111, 124, 136, 149, 161, 172, 184, 195, 205, 214, 225, 233, 241, 249, 255, 255,
0, 0, 0, 0, 0, 0, 0, 6, 13, 21, 28, 37, 47, 56, 68, 80, 94,
107, 120, 132, 145, 157, 169, 180, 192, 203, 213, 224, 232, 240, 248, 255, 255,
0, 0, 0, 0, 0, 0, 0, 3, 11, 18, 25, 35, 44, 52, 64, 76, 88,
101, 115, 127, 141, 153, 165, 176, 187, 200, 211, 222, 230, 238, 246, 255, 255,
0, 0, 0, 0, 0, 0, 0, 1, 8, 15, 22, 32, 40, 48, 59, 71, 83,
96, 110, 122, 136, 149, 161, 172, 183, 193, 208, 220, 228, 236, 245, 253, 255,
0, 0, 0, 0, 0, 0, 0, 0, 5, 12, 19, 28, 35, 44, 55, 65, 77,
90, 104, 118, 132, 145, 157, 169, 179, 189, 202, 216, 226, 235, 243, 252, 255,
0, 0, 0, 0, 0, 0, 0, 0, 2, 9, 16, 23, 30, 40, 50, 59, 72,
85, 99, 113, 127, 140, 154, 165, 175, 185, 196, 210, 224, 233, 242, 250, 255,
0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 12, 19, 26, 36, 45, 54, 66,
78, 91, 105, 120, 134, 148, 159, 169, 183, 194, 208, 221, 232, 241, 250, 255,
0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 8, 15, 22, 31, 39, 48, 60,
71, 84, 98, 112, 127, 142, 153, 165, 180, 192, 205, 218, 230, 240, 249, 255,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 11, 18, 26, 34, 42, 53,
64, 77, 91, 105, 120, 135, 148, 161, 177, 190, 203, 216, 227, 238, 248, 255,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 8, 15, 21, 28, 37, 48,
58, 70, 84, 99, 114, 130, 144, 158, 173, 188, 201, 213, 225, 235, 246, 255
};

#endif

/* init table */
static REG_INIT_TABLE_STRUCT OD_INIT_TABLE[] = {
	{0, 0x82040201, 0xffffffff},
	/* {OD_REG01, 0x00071401, 0x000000ff}, */
	{0, 0x00070E01, 0x000000ff},
	{0, 0x00000400, 0xffffffff},
	{0, 0x00E0FF04, 0xffffffff},

	{0, 0x00010026, 0xffffffff}, /*set cr rate ==0x42 */
    /* {OD_REG32, 0x04055600, 0xffffffff}, */
	{0, 0x00021000, 0x003fffff},
	{0, 0x0674A0E6, 0xffffffff},
	{0, 0x000622E0, 0xffffffff}, /* 0x00051170 */
	{0, 0xE0849C40, 0xffffffff}, /* DET8B_POS=4 */
	{0, 0x05CFDE08, 0xffffffff}, /* 0x04E6D998 */
	{0, 0x011F1013, 0xffffffff}, /* line size=0xf8 | (0x37 << 13) */
	{0, 0x00200000, 0x00ff0000}, /* dram_crc_cnt=0x20 */
	{0, 0x20000610, 0xffffffff}, /* enable GM */
	{0, 0x001E02D0, 0xffffffff},
	{0, 0x00327C7C, 0xffffffff},
	{0, 0x00180000, 0xffffffff}, /* disable film mode detection */
	{0, 0x006400C8, 0xffffffff},
	{0, 0x00210032, 0xfffffffc}, /* pcid_alig_sel=1 */
	{0, 0x4E00023F, 0xffffffff},
	{0, 0xC306B16A, 0xffffffff}, /* pre_bw=3 */
	{0, 0x10240408, 0xffffffff}, /* ???00210408??? */
	{0, 0xC5C00000, 0xC7C00000},
	{0, 0x21A1B800, 0xffffffff}, /* dump burst */
	{0, 0x60000044, 0xe00000ff}, /* DUMP_WFF_FULL_CONF=3 */
	{0, 0x2FFF3E00, 0xffffffff},
	{0, 0x80039000, 0xffffffff}, /* new 3x3 */
	{0, 0x352835CA, 0xffffffff}, /* skip color thr */
	{0, 0x00014438, 0xffffffff}, /* pattern gen */
	{0, 0x27800898, 0xffffffff},
	{0, 0x00438465, 0xffffffff},
	{0, 0x01180001, 0xffffffff},
	{0, 0x0002D000, 0xffffffff},
	{0, 0x3FFFFFFF, 0xffffffff},
	{0, 0x00000000, 0xffffffff},
	{0, 0x000200C0, 0xffffffff},
	/*{OD_REG78, 0x04000000, 0x0C000000}, *//*force_1st_frame_end */
};

static void od_init_table_init(void)
{
	OD_INIT_TABLE[0].reg_addr = OD_REG00;
	OD_INIT_TABLE[1].reg_addr = OD_REG01;
	OD_INIT_TABLE[2].reg_addr = OD_REG02;
	OD_INIT_TABLE[3].reg_addr = OD_REG03;
	OD_INIT_TABLE[4].reg_addr = OD_REG30;
	OD_INIT_TABLE[5].reg_addr = OD_REG33;
	OD_INIT_TABLE[6].reg_addr = OD_REG34;
	OD_INIT_TABLE[7].reg_addr = OD_REG35;
	OD_INIT_TABLE[8].reg_addr = OD_REG36;
	OD_INIT_TABLE[9].reg_addr = OD_REG37;
	OD_INIT_TABLE[10].reg_addr = OD_REG38;
	OD_INIT_TABLE[11].reg_addr = OD_REG39;
	OD_INIT_TABLE[12].reg_addr = OD_REG40;
	OD_INIT_TABLE[13].reg_addr = OD_REG41;
	OD_INIT_TABLE[14].reg_addr = OD_REG42;
	OD_INIT_TABLE[15].reg_addr = OD_REG43;
	OD_INIT_TABLE[16].reg_addr = OD_REG44;
	OD_INIT_TABLE[17].reg_addr = OD_REG45;
	OD_INIT_TABLE[18].reg_addr = OD_REG46;
	OD_INIT_TABLE[19].reg_addr = OD_REG47;
	OD_INIT_TABLE[20].reg_addr = OD_REG48;
	OD_INIT_TABLE[21].reg_addr = OD_REG49;
	OD_INIT_TABLE[22].reg_addr = OD_REG51;
	OD_INIT_TABLE[23].reg_addr = OD_REG52;
	OD_INIT_TABLE[24].reg_addr = OD_REG53;
	OD_INIT_TABLE[25].reg_addr = OD_REG54;
	OD_INIT_TABLE[26].reg_addr = OD_REG57;
	OD_INIT_TABLE[27].reg_addr = OD_REG62;
	OD_INIT_TABLE[28].reg_addr = OD_REG63;
	OD_INIT_TABLE[29].reg_addr = OD_REG64;
	OD_INIT_TABLE[30].reg_addr = OD_REG65;
	OD_INIT_TABLE[31].reg_addr = OD_REG66;
	OD_INIT_TABLE[32].reg_addr = OD_REG67;
	OD_INIT_TABLE[33].reg_addr = OD_REG68;
	OD_INIT_TABLE[34].reg_addr = OD_REG69;
}


static u32 g_od_buf_size;
static unsigned long g_od_buf_pa;
static void *g_od_buf_va;

static int od_debug_level = 3;
static int g_od_is_demo_mode;
static int g_od_is_debug_mode;
static int g_od_is_enabled; /* OD is disabled by default */
static int g_od_is_enabled_from_hwc = 1;
static int g_od_is_enabled_from_mhl = 1;
static int g_od_is_enabled_force = 2;
static int g_od_function_availible = 1;
static int g_od_reset_finish;
static int g_od_counter;
static int g_od_trigger;
static int g_od_initial_config;
static int g_od_frm_count;
static cmdqBackupSlotHandle g_od_hSlot;

static struct timeval g_od_time_pre, g_od_time_cur;

unsigned int g_rdma_error_cnt;
unsigned int g_rdma_normal_cnt = 0;

static OD_STATUS g_od_status;

static ddp_module_notify g_od_ddp_notify;
struct task_struct *disp_od_thread = NULL;

static void _od_reg_init(void *cmdq, const REG_INIT_TABLE_STRUCT *init_table, int tbl_size)
{
	int index = 0;

	for (index = 0; index < tbl_size; index++, init_table++)
		DISP_REG_SET(cmdq, init_table->reg_addr, (init_table->reg_value & init_table->reg_mask));

}


static void od_refresh_screen(void)
{
	if (g_od_ddp_notify != NULL)
		g_od_ddp_notify(DISP_MODULE_OD, DISP_PATH_EVENT_TRIGGER);
}

int _disp_od_read_dram_status(void *cmdq)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	int ret = 0;
	int i;
	int value;
	/* read dram status register value from slot */
	for (i = 0; i < 3; i++) {
		cmdqBackupReadSlot(g_od_hSlot, i, &value);
		ODERR("CMDQ: check DRAM status value=0x%x(%2d)\n", value, i);
		if ((value & 0x3C0000) != 0) {
			ret = -1;
			break;
		}
	}
	ODDBG(OD_DBG_ALWAYS, "_disp_od_read_dram_status value=%08x(%d)\n", value, ret);
	return ret;
#else
	return 0;
#endif
}

void _disp_od_backup_dram_status(void *cmdq)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	int i;
	/* backup OD dram status register to slot */
	for (i = 0; i < 3; i++)
		cmdqRecBackupRegisterToSlot(cmdq, g_od_hSlot, i, disp_addr_convert(OD_STA03));

#endif
}

int _disp_od_core_reset(DISP_MODULE_ENUM module, void *cmdq)
{
	unsigned int value = 0;
	int ret = 0;

	OD_REG_SET_FIELD(cmdq, OD_REG12, 0, RG_OD_START);/* rg_od_start=0 */
	OD_REG_SET_FIELD(cmdq, OD_REG00, 0, OD_ENABLE);/* rg_od_en=0 */

	/* [1]:can access OD table; [0]:can't access OD table */
	DISP_REG_SET(cmdq, DISP_OD_MISC, 1);

	DISP_REG_MASK(cmdq, DISP_OD_CFG, 0<<1, 0x2);/* core disable */

	/* SW reset = 1 */
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);

	/* polling until request of dram end */
	ODDBG(OD_DBG_ALWAYS, "_disp_od_core_enabled start polling dram req");
	cmdqRecPoll(cmdq, disp_addr_convert(OD_STA03), 0x0, 0x30000);

	/* SW reset = 0 */
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);

	/* rg_od_rst toggle = 1 */
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);

	/* rg_od_rst toggle = 0 */
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);

	/* rg_swreset = 0 */
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);

	/* rg_swreset = 1 */
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);

	/* [1]:can access OD table; [0]:can't access OD table */
	DISP_REG_SET(cmdq, DISP_OD_MISC, 0);

	/* dram and bypass setting */
	OD_REG_SET_FIELD(cmdq, OD_REG37, 0xf, ODT_MAX_RATIO);
	OD_REG_SET_FIELD(cmdq, OD_REG02, 0, ODT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG71, 1, RG_WDRAM_HOLD_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG72, 1, RG_RDRAM_HOLD_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG39, 1, WDRAM_DIS);
	OD_REG_SET_FIELD(cmdq, OD_REG39, 1, RDRAM_DIS);

	g_od_is_enabled = 0;

	/* return error if request and hold still high */
	if (ret == -1) {
		ODDBG(OD_DBG_ALWAYS, "disp_od_core_reset fail(%2d)", g_od_is_enabled);
		return -1;

	} else {
		ODDBG(OD_DBG_ALWAYS, "disp_od_core_reset success(%2d)", g_od_is_enabled);
		OD_REG_SET_FIELD(cmdq, OD_REG39, 0, WDRAM_DIS);
		OD_REG_SET_FIELD(cmdq, OD_REG39, 0, RDRAM_DIS);
		OD_REG_SET_FIELD(cmdq, OD_REG12, 1, RG_OD_START);
		OD_REG_SET_FIELD(cmdq, OD_REG00, 1, OD_ENABLE);
		return 0;
	}
}

void disp_od_core_reset(DISP_MODULE_ENUM module, void *cmdq)
{
	int ret = -1;

	ret = _disp_od_core_reset(module, cmdq);
	if (ret == -1)
		g_od_reset_finish = 0;
	else
		g_od_reset_finish = 1;
}

void od_debug_reg(void)
{
	ODDBG(OD_DBG_ALWAYS, "==DISP OD REGS==\n");
	ODDBG(OD_DBG_ALWAYS, "OD:0x000=0x%08x,0x004=0x%08x,0x008=0x%08x,0x00c=0x%08x\n",
		OD_REG_GET(DISP_REG_OD_EN),
		OD_REG_GET(DISP_REG_OD_RESET),
		OD_REG_GET(DISP_REG_OD_INTEN),
		OD_REG_GET(DISP_REG_OD_INTSTA));

	ODDBG(OD_DBG_ALWAYS, "OD:0x010=0x%08x,0x020=0x%08x,0x024=0x%08x,0x028=0x%08x\n",
		OD_REG_GET(DISP_REG_OD_STATUS),
		OD_REG_GET(DISP_REG_OD_CFG),
		OD_REG_GET(DISP_REG_OD_INPUT_COUNT),
		OD_REG_GET(DISP_REG_OD_OUTPUT_COUNT));

	ODDBG(OD_DBG_ALWAYS, "OD:0x02c=0x%08x,0x030=0x%08x,0x040=0x%08x,0x044=0x%08x\n",
		OD_REG_GET(DISP_REG_OD_CHKSUM),
		OD_REG_GET(DISP_REG_OD_SIZE),
		OD_REG_GET(DISP_REG_OD_HSYNC_WIDTH),
		OD_REG_GET(DISP_REG_OD_VSYNC_WIDTH));

	ODDBG(OD_DBG_ALWAYS, "OD:0x048=0x%08x,0x0C0=0x%08x\n",
		OD_REG_GET(DISP_REG_OD_MISC),
		OD_REG_GET(DISP_REG_OD_DUMMY_REG));

	ODDBG(OD_DBG_ALWAYS, "OD:0x684=0x%08x,0x688=0x%08x,0x68c=0x%08x,0x690=0x%08x\n",
		OD_REG_GET(DISPSYS_OD_BASE + 0x684),
		OD_REG_GET(DISPSYS_OD_BASE + 0x688),
		OD_REG_GET(DISPSYS_OD_BASE + 0x68c),
		OD_REG_GET(DISPSYS_OD_BASE + 0x690));

	ODDBG(OD_DBG_ALWAYS, "OD:0x694=0x%08x,0x698=0x%08x,0x700=0x%08x,0x704=0x%08x\n",
		OD_REG_GET(DISPSYS_OD_BASE + 0x694),
		OD_REG_GET(DISPSYS_OD_BASE + 0x698),
		OD_REG_GET(DISPSYS_OD_BASE + 0x700),
		OD_REG_GET(DISPSYS_OD_BASE + 0x704));

	ODDBG(OD_DBG_ALWAYS, "OD:0x708=0x%08x,0x778=0x%08x,0x78c=0x%08x,0x790=0x%08x\n",
		OD_REG_GET(DISPSYS_OD_BASE + 0x708),
		OD_REG_GET(DISPSYS_OD_BASE + 0x778),
		OD_REG_GET(DISPSYS_OD_BASE + 0x78c),
		OD_REG_GET(DISPSYS_OD_BASE + 0x790));

	ODDBG(OD_DBG_ALWAYS, "OD:0x7a0=0x%08x,0x7dc=0x%08x,0x7e8=0x%08x\n",
		OD_REG_GET(DISPSYS_OD_BASE + 0x7a0),
		OD_REG_GET(DISPSYS_OD_BASE + 0x7dc),
		OD_REG_GET(DISPSYS_OD_BASE + 0x7e8));

}

void _disp_od_core_enabled(void *cmdq, int enabled)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	unsigned int value = 0;
	int i;

	od_debug_reg();

	/* [1]:can access OD table; [0]:can't access OD table */
	DISP_REG_SET(cmdq, DISP_OD_MISC, 1);

	DISP_REG_MASK(cmdq, DISP_OD_CFG, 0<<1, 0x2);/* core disable */

	/* SW reset = 1 */
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x1, 0x1);

	/* polling until request of dram end */
	ODDBG(OD_DBG_ALWAYS, "_disp_od_core_enabled start polling dram req");
	cmdqRecPoll(cmdq, disp_addr_convert(OD_STA03), 0x0, 0x30000);

	/* SW reset = 0 */
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);
	DISP_REG_MASK(cmdq, DISP_OD_RESET, 0x0, 0x1);

	/* rg_od_rst toggle = 1 */
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 1, OD_RST);

	/* rg_od_rst toggle = 0 */
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);
	OD_REG_SET_FIELD(cmdq, OD_REG01, 0, OD_RST);

	/* rg_swreset = 0 */
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 0, SWRESET);

	/* rg_swreset = 1 */
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);
	OD_REG_SET_FIELD(cmdq, OD_REG07, 1, SWRESET);

	/* [1]:can access OD table; [0]:can't access OD table */
	DISP_REG_SET(cmdq, DISP_OD_MISC, 0);

	/* dram and bypass setting */
	OD_REG_SET_FIELD(cmdq, OD_REG37, 0x0, ODT_MAX_RATIO);
	OD_REG_SET_FIELD(cmdq, OD_REG02, 0, ODT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG71, 1, RG_WDRAM_HOLD_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG72, 1, RG_RDRAM_HOLD_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG39, 0, WDRAM_DIS);
	OD_REG_SET_FIELD(cmdq, OD_REG39, 0, RDRAM_DIS);

	if (enabled == 1) {
		DISP_REG_MASK(cmdq, OD_REG00, 0, (1 << 31)); /* bypass_all = 0 */
		DISP_REG_MASK(cmdq, OD_REG00, 1, 1); /* EN = 1 */
		DISP_REG_MASK(cmdq, DISP_OD_CFG, 1<<1, 0x2);/* core en */

	} else {
		DISP_REG_MASK(cmdq, OD_REG00, 1, (1 << 31)); /* bypass_all = 1 */
		DISP_REG_MASK(cmdq, OD_REG00, 0, 1); /* EN = 1 */
		DISP_REG_MASK(cmdq, DISP_OD_CFG, 0<<1, 0x2);/* core disable */

	}

	ODDBG(OD_DBG_ALWAYS, "_disp_od_core_enabled value=%d\n", enabled);
#else
	ODDBG(OD_DBG_ALWAYS, "_disp_od_core_enabled: CONFIG_MTK_OD_SUPPORT is not set");
#endif
	od_refresh_screen();
}

void _disp_od_start(void *cmdq)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	/* set max OD strength after bypass n frame */
	OD_REG_SET_FIELD(cmdq, OD_REG37, 0xf, ODT_MAX_RATIO);
	ODDBG(OD_DBG_ALWAYS, "_disp_od_start(%d)", g_od_is_enabled);
	od_refresh_screen();
#endif
}

void disp_od_irq_handler(void)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	unsigned int od_istra = DISP_REG_GET(DISP_REG_OD_INTSTA);
	int i = 0;
	unsigned int mask;
	/* clear interrupt flag if "1" */
	for (i = 0; i < 9; i++) {
		mask = 1<<i;
		if ((od_istra & mask) != 0) {
			mask = ~mask;
			DISP_REG_SET(NULL, DISP_REG_OD_INTSTA, (od_istra & mask));
		}
	}
	/* detect certain interrupt for action */
	if ((od_istra & 0x2) != 0) {/* check OD output frame end interrupt */
		g_od_trigger = 1;
		g_od_frm_count += 1;
		do_gettimeofday(&g_od_time_cur);
	}

#endif
	DISP_REG_SET(NULL, DISP_REG_OD_INTSTA, 0);
}

int disp_rdma_notify(unsigned int reg)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	if ((reg & (1 << 4)) != 0)
		g_rdma_error_cnt += 1;
	else if ((reg & (1 << 1)) != 0)
		g_rdma_normal_cnt += 1;
#endif
	return 0;
}

void _disp_od_err_handle(void *cmdq)
{
	if (g_od_status != OD_STATUS_RESET) {
		ODDBG(OD_DBG_ALWAYS, "disp_od_update_status detect_hold");
		OD_REG_SET_FIELD(NULL, OD_REG71, 0, RG_WDRAM_HOLD_EN);
		OD_REG_SET_FIELD(NULL, OD_REG72, 0, RG_RDRAM_HOLD_EN);
		OD_REG_SET_FIELD(NULL, OD_REG02, 1, ODT_BYPASS);
		g_od_counter = 0;
		g_od_function_availible = 0;
		g_od_reset_finish = 0;
		g_od_status = OD_STATUS_RESET;
		od_refresh_screen();
	}
}

/* OD kernel thread function for monitor OD status */
static int disp_od_update_worker(void)
{
	unsigned int disp_od_status = 0;
	unsigned int disp_od_input_count = 0;
	unsigned int disp_od_output_count = 0;
	unsigned int od_status_sta_3 = 0;

	unsigned long time_diff;
	unsigned long time_idle;
	unsigned long period_per_frame;

	int frm_count_pre = 0;
	int frm_count_statics = 0;
	int t = 1;
	int dram_hold_frm = 0;

	static struct timeval time_pre, time_cur;

	for (;;) {
		msleep(t);
		disp_od_status = DISP_REG_GET(DISP_OD_STATUS);/* input(valid)/output(ready) status */
		disp_od_input_count = DISP_REG_GET(DISP_OD_INPUT_COUNT);/* input pixel count */
		disp_od_output_count = DISP_REG_GET(DISP_OD_OUTPUT_COUNT);/* output pixel count */
		od_status_sta_3 = DISP_REG_GET(OD_STA03);/* WDRAM_HOLD AND RDRAM_HOLD flag */
#if 1
		/* workaround dram hold reset */
		if ((od_status_sta_3 & 0xc0000) != 0) {
			if (g_od_frm_count == dram_hold_frm) {/* detect dram hold status and keep */
				_disp_od_err_handle(NULL);
			}
		}
		dram_hold_frm = g_od_frm_count;
#endif
		/* get time difference between 2 iteration */
		do_gettimeofday(&time_cur);
		time_diff = (unsigned long)(time_cur.tv_sec-time_pre.tv_sec)*1000000 +
			(unsigned long)(time_cur.tv_usec-time_pre.tv_usec);

		if (time_diff >= OD_CHECK_FPS_INTERVAL) {
			frm_count_statics = g_od_frm_count - frm_count_pre;
			frm_count_pre = g_od_frm_count;
			time_pre = time_cur;

			ODDBG(OD_DBG_DEBUG, "frame diff=%d time diff=%d ODON=%d",
				(unsigned int)(frm_count_statics), (unsigned int)time_diff, g_od_is_enabled);

			if (frm_count_statics > 0) {
				period_per_frame = time_diff/frm_count_statics;

				ODDBG(OD_DBG_DEBUG, "frame period=%d", (unsigned int)period_per_frame);

				/* average period per frame check for avoid ghost side effect under low fps condition */
				if (period_per_frame > OD_ENABLE_FRMPERIOD_THRESHOLD)
					OD_REG_SET_FIELD(NULL, OD_REG02, 1, ODT_BYPASS);/* disable od table bypass */
				else
					OD_REG_SET_FIELD(NULL, OD_REG02, 0, ODT_BYPASS);/* enable od table bypass */

			}
		}
	}
	return 0;
}

int disp_od_update_status(void *cmdq)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	unsigned int od_status_sta_2 = DISP_REG_GET(OD_STA02);
	unsigned int od_status_sta_3 = DISP_REG_GET(OD_STA03);
	unsigned int dma_mon0        = DISP_REG_GET(OD_REG14);
	unsigned int dma_mon1        = DISP_REG_GET(OD_REG15);
	unsigned long time_diff = (unsigned long)(g_od_time_cur.tv_sec-g_od_time_pre.tv_sec)*1000000 +
			(unsigned long)(g_od_time_cur.tv_usec-g_od_time_pre.tv_usec);
	g_od_time_pre = g_od_time_cur;

	ODDBG(OD_DBG_DEBUG, "disp_od_update_status=%d rd_addr:0x%08x wr_addr:0x%08x t=%d",
		g_od_status, dma_mon0&0x3ffff, dma_mon1&0x3ffff, (unsigned int)time_diff);

	_disp_od_backup_dram_status(cmdq);/* backup OD dram status register to slot */

	if (((od_status_sta_3 & 0xc0000) != 0) && g_od_is_enabled) {
		ODDBG(OD_DBG_ALWAYS, "disp_od_update_status dram access hold(cpu)");
#if 0
		g_rdma_error_cnt  = 0;
		g_rdma_normal_cnt = 0;

		if (g_od_status != OD_STATUS_RESET) {
			ODDBG(OD_DBG_ALWAYS, "disp_od_update_status detect_hold");
			OD_REG_SET_FIELD(NULL, OD_REG71, 0, RG_WDRAM_HOLD_EN);
			OD_REG_SET_FIELD(NULL, OD_REG72, 0, RG_RDRAM_HOLD_EN);
			OD_REG_SET_FIELD(NULL, OD_REG02, 1, ODT_BYPASS);
			g_od_counter = 0;
			g_od_function_availible = 0;
			g_od_reset_finish = 0;
			g_od_status = OD_STATUS_RESET;
			od_refresh_screen();
		}
#endif
	} else {
		g_od_function_availible = 1;

		if (g_od_is_enabled != g_od_is_enabled_from_mhl) {
			if (g_od_is_enabled_from_mhl == 0)
				disp_od_set_enabled(cmdq, 0);

		}
	}

	if (g_od_trigger == 1 && g_od_status != OD_STATUS_IDLE) {
		ODDBG(OD_DBG_DEBUG, "disp_od_update_status=%d(available=%d,cnt=%d,tri=%d)sta1=0x%x sta2=0x%x",
			g_od_status, g_od_function_availible, g_od_counter, g_od_trigger,
			od_status_sta_2, od_status_sta_3);
	}

	switch (g_od_status) {
	case OD_STATUS_IDLE:
		if (g_od_trigger == 1) {
			g_od_counter = 0;
			g_od_trigger = 0;
		}
		break;

	case OD_STATUS_ENABLE:
		if (g_od_trigger == 1) {
			g_od_counter += 1;
			if (g_od_counter >= 8) {
				_disp_od_start(cmdq);
				g_od_status = OD_STATUS_IDLE;
			}
			g_od_trigger = 0;
			od_refresh_screen();
		}
		break;

	case OD_STATUS_BYPASS:
		g_od_counter += 1;
		break;

	case OD_STATUS_RESET:
		if (g_od_trigger == 1) {
			if (g_od_function_availible == 1 && g_od_reset_finish) {

				disp_od_core_reset(DISP_MODULE_OD, cmdq);
				g_od_counter = 0;
				g_od_status = OD_STATUS_IDLE;

			} else {
				g_od_counter += 1;
				g_od_counter &= 0x7;
				if (g_od_counter <= 0x4)
					disp_od_core_reset(DISP_MODULE_OD, cmdq);
				g_od_status = OD_STATUS_RESET;
			}
			g_od_trigger = 0;
			od_refresh_screen();
		}
		break;

	default:
		break;
	}

#endif
	return 0;
}


static void _od_set_dram_buffer_addr(void *cmdq, int manual_comp, int image_width, int image_height)
{
	u32 u4ODDramSize;
	u32 u4Linesize;
	u32 od_buf_pa_32;

	static int is_inited;

    /* set line size : ( h active/4* manual CR )/128  ==>linesize = (h active * manual CR)/512*/
	u4Linesize = ((image_width * manual_comp) >> 9) + 2;
	u4ODDramSize = u4Linesize * (image_height / 2) * 16;

	if (!is_inited) {
		void *va;
		dma_addr_t dma_addr;

		va = dma_alloc_coherent(NULL, u4ODDramSize + OD_ADDITIONAL_BUFFER + OD_GUARD_PATTERN_SIZE,
			&dma_addr, GFP_KERNEL);

		if (va == NULL) {
			ODDBG(OD_DBG_ALWAYS, "OD: MEM NOT ENOUGH %d", u4Linesize);
			BUG();
		}

		ODDBG(OD_DBG_ALWAYS, "OD: pa %08lx size %d order %d va %lx\n",
			(unsigned long)(dma_addr), u4ODDramSize, get_order(u4ODDramSize), (unsigned long)va);

		is_inited = 1;

		g_od_buf_size = u4ODDramSize;
		g_od_buf_pa = (unsigned long)dma_addr;
		g_od_buf_va = va;

		/* set guard pattern */
		*((u32 *)((unsigned long)va + u4ODDramSize)) = OD_GUARD_PATTERN;
		*((u32 *)((unsigned long)va + u4ODDramSize + OD_ADDITIONAL_BUFFER)) = OD_GUARD_PATTERN;

	}

	od_buf_pa_32 = (u32)g_od_buf_pa;

	OD_REG_SET_FIELD(cmdq, OD_REG06, (od_buf_pa_32 >> 4), RG_BASE_ADR);
	OD_REG_SET_FIELD(cmdq, OD_REG56, ((od_buf_pa_32 + u4ODDramSize) >> 4), DRAM_UPBOUND);
	OD_REG_SET_FIELD(cmdq, OD_REG56, 1, DRAM_PROT);
}



static void _od_set_frame_protect_init(void *cmdq, int image_width, int image_height)
{
	OD_REG_SET_FIELD(cmdq, OD_REG08, image_width, OD_H_ACTIVE);
	OD_REG_SET_FIELD(cmdq, OD_REG32, image_width, OD_DE_WIDTH);

	OD_REG_SET_FIELD(cmdq, OD_REG08, image_height, OD_V_ACTIVE);

	OD_REG_SET_FIELD(cmdq, OD_REG53, 0x0BFB, FRAME_ERR_CON);   /* don't care v blank */
	OD_REG_SET_FIELD(cmdq, OD_REG09, 0x01E, RG_H_BLANK);       /* h_blank  = htotal - h_active */
	OD_REG_SET_FIELD(cmdq, OD_REG09, 0x0A, RG_H_OFFSET);       /* tolerrance */

	OD_REG_SET_FIELD(cmdq, OD_REG10, 0xFFF, RG_H_BLANK_MAX);
	OD_REG_SET_FIELD(cmdq, OD_REG10, 0x3FFFF, RG_V_BLANK_MAX); /* pixel-based counter */

	OD_REG_SET_FIELD(cmdq, OD_REG11, 0xB000, RG_V_BLANK);      /* v_blank  = vtotal - v_active */
	OD_REG_SET_FIELD(cmdq, OD_REG11, 2, RG_FRAME_SET);
}


static void _od_set_param(void *cmdq, int manual_comp, int image_width, int image_height)
{
	u32 u4GMV_width;
	u32 u4Linesize;

    /* set gmv detection width */
	u4GMV_width = image_width / 6;

	OD_REG_SET_FIELD(cmdq, OD_REG40, (u4GMV_width*1)>>4, GM_R0_CENTER);
	OD_REG_SET_FIELD(cmdq, OD_REG40, (u4GMV_width*2)>>4, GM_R1_CENTER);
	OD_REG_SET_FIELD(cmdq, OD_REG41, (u4GMV_width*3)>>4, GM_R2_CENTER);
	OD_REG_SET_FIELD(cmdq, OD_REG41, (u4GMV_width*4)>>4, GM_R3_CENTER);
	OD_REG_SET_FIELD(cmdq, OD_REG42, (u4GMV_width*5)>>4, GM_R4_CENTER);

	OD_REG_SET_FIELD(cmdq, OD_REG43, 12 >> 2                 , GM_V_ST);
	OD_REG_SET_FIELD(cmdq, OD_REG43, (image_height-12)>>2    , GM_V_END);
	OD_REG_SET_FIELD(cmdq, OD_REG42, (100*image_height)/1080 , GM_LGMIN_DIFF);
	OD_REG_SET_FIELD(cmdq, OD_REG44, (400*image_height)/1080 , GM_LMIN_THR);
	OD_REG_SET_FIELD(cmdq, OD_REG44, (200*image_height)/1080 , GM_GMIN_THR);

    /* set compression ratio */
	OD_REG_SET_FIELD(cmdq, OD_REG30, manual_comp, MANU_CPR);

    /* set line size */
	/* linesize = ( h active/4* manual CR )/128  ==>linesize = (h active * manual CR)/512 */
	u4Linesize = ((image_width * manual_comp) >> 9) + 2;
	OD_REG_SET_FIELD(cmdq, OD_REG47, 3, PRE_BW);  /* vIO32WriteFldAlign(OD_REG47, 3, PRE_BW); */

	OD_REG_SET_FIELD(cmdq, OD_REG34, 0xF,  ODT_SB_TH0);
	OD_REG_SET_FIELD(cmdq, OD_REG34, 0x10, ODT_SB_TH1);
	OD_REG_SET_FIELD(cmdq, OD_REG34, 0x11, ODT_SB_TH2);
	OD_REG_SET_FIELD(cmdq, OD_REG34, 0x12, ODT_SB_TH3);

	OD_REG_SET_FIELD(cmdq, OD_REG47, 0x13, ODT_SB_TH4);
	OD_REG_SET_FIELD(cmdq, OD_REG47, 0x14, ODT_SB_TH5);
	OD_REG_SET_FIELD(cmdq, OD_REG47, 0x15, ODT_SB_TH6);
	OD_REG_SET_FIELD(cmdq, OD_REG47, 0x16, ODT_SB_TH7);

	OD_REG_SET_FIELD(cmdq, OD_REG38, u4Linesize, LINE_SIZE);

    /* use 64 burst length */
	OD_REG_SET_FIELD(cmdq, OD_REG38, 3, WR_BURST_LEN);
	OD_REG_SET_FIELD(cmdq, OD_REG38, 3, RD_BURST_LEN);

    /*set auto 8bit parameters */
	if (image_width > 1900) {
		OD_REG_SET_FIELD(cmdq, OD_REG35, (140000 << 0), DET8B_DC_NUM);
		OD_REG_SET_FIELD(cmdq, OD_REG36, (40000 << 18), DET8B_BTC_NUM);
		OD_REG_SET_FIELD(cmdq, OD_REG37, (1900000>>4) , DET8B_BIT_MGN);
	} else {
		OD_REG_SET_FIELD(cmdq, OD_REG35, 70000, DET8B_DC_NUM);
		OD_REG_SET_FIELD(cmdq, OD_REG36, 20000, DET8B_BTC_NUM);
		OD_REG_SET_FIELD(cmdq, OD_REG37, (950000>>4), DET8B_BIT_MGN);
	}

    /* set auto Y5 mode thr */
	OD_REG_SET_FIELD(cmdq, OD_REG46,  0x4E00, AUTO_Y5_NUM);
	OD_REG_SET_FIELD(cmdq, OD_REG53,  0x4E00, AUTO_Y5_NUM_1);

    /* set OD threshold */
	OD_REG_SET_FIELD(cmdq, OD_REG01, 10, MOTION_THR);
	OD_REG_SET_FIELD(cmdq, OD_REG48, 8, ODT_INDIFF_TH);
	OD_REG_SET_FIELD(cmdq, OD_REG02, 1, FBT_BYPASS);

    /* set dump param */
	OD_REG_SET_FIELD(cmdq, OD_REG51, 0, DUMP_STLINE);
	OD_REG_SET_FIELD(cmdq, OD_REG51, (image_height-1), DUMP_ENDLINE);

    /* set compression param */
	OD_REG_SET_FIELD(cmdq, OD_REG77, 0xfc, RC_U_RATIO);
	OD_REG_SET_FIELD(cmdq, OD_REG78, 0xfc, RC_U_RATIO_FIRST2);
	OD_REG_SET_FIELD(cmdq, OD_REG77, 0x68, RC_L_RATIO);

	OD_REG_SET_FIELD(cmdq, OD_REG78, 0x68, RC_L_RATIO_FIRST2);
	OD_REG_SET_FIELD(cmdq, OD_REG76, 0x3, CHG_Q_FREQ);

	OD_REG_SET_FIELD(cmdq, OD_REG76, 0x2, CURR_Q_UV);
	OD_REG_SET_FIELD(cmdq, OD_REG76, 0x2, CURR_Q_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG77, 0x8, IP_SAD_TH);
}


static void _od_write_table(void *cmdq, u8 TableSel, u8 ColorSel, u8 *pTable, int table_inverse)
{
	u32 i, u4TblSize;
	u32 u1ODBypass   = DISP_REG_GET(OD_REG02) & (1 << 9)  ? 1 : 0;
	u32 u1FBBypass   = DISP_REG_GET(OD_REG02) & (1 << 10) ? 1 : 0;
	u32 u1PCIDBypass = DISP_REG_GET(OD_REG02) & (1 << 21) ? 1 : 0;

	if (ColorSel > 3)
		return;

    /* disable OD_START */
	DISP_REG_SET(cmdq, OD_REG12, 0);

	OD_REG_SET_FIELD(cmdq, OD_REG02, 1, ODT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG02, 1, FBT_BYPASS);

	OD_REG_SET_FIELD(cmdq, OD_REG45, 0, OD_PCID_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG45, 1, OD_PCID_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG04, 1, TABLE_ONLY_W_ADR_INC);
	OD_REG_SET_FIELD(cmdq, OD_REG04, 0, ADDR_YX);
	OD_REG_SET_FIELD(cmdq, OD_REG04, 0x3, TABLE_RW_SEL_OD_BGR);

	if (ColorSel == 3)
		OD_REG_SET_FIELD(cmdq, OD_REG04, 7, TABLE_RW_SEL_OD_BGR);

	else
		OD_REG_SET_FIELD(cmdq, OD_REG04, (1 << ColorSel), TABLE_RW_SEL_OD_BGR);

	switch (TableSel) {
	case OD_TABLE_33:
		u4TblSize = OD_TBL_M_SIZE;
		OD_REG_SET_FIELD(cmdq, OD_REG32, 0, OD_IDX_41);
		OD_REG_SET_FIELD(cmdq, OD_REG32, 0, OD_IDX_17);
		break;

	case OD_TABLE_17:
		u4TblSize = OD_TBL_S_SIZE;
		OD_REG_SET_FIELD(cmdq, OD_REG32, 0, OD_IDX_41);
		OD_REG_SET_FIELD(cmdq, OD_REG32, 1, OD_IDX_17);
		break;

	default:
		return;
	}

	for (i = 0; i < u4TblSize; i++) {
		if (table_inverse) {
			u8 value = ABS(255 - *(pTable+i));

			DISP_REG_SET(cmdq, OD_REG05, value);
		} else {
			DISP_REG_SET(cmdq, OD_REG05, *(pTable+i));
		}
	}

	DISP_REG_SET(cmdq, OD_REG04, 0);
	OD_REG_SET_FIELD(cmdq, OD_REG02,  u1ODBypass, ODT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG02,  u1FBBypass, FBT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG45,  (!u1PCIDBypass), OD_PCID_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG45,  (u1PCIDBypass), OD_PCID_BYPASS);
}



static u8 _od_read_table(void *cmdq, u8 TableSel, u8 ColorSel, u8 *pTable, int table_inverse)
{
	u32 i, u4TblVal, u4TblSize, u4ErrCnt = 0;
	u32 mask;
	u32 u1ODBypass    = DISP_REG_GET(OD_REG02) & (1 << 9)  ? 1 : 0;
	u32 u1FBBypass    = DISP_REG_GET(OD_REG02) & (1 << 10) ? 1 : 0;
	u32 u1PCIDBypass  = DISP_REG_GET(OD_REG02) & (1 << 21) ? 1 : 0;

	if (ColorSel > 2)
		return 1;

	OD_REG_SET_FIELD(cmdq, OD_REG02, u1ODBypass, ODT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG02, u1FBBypass, FBT_BYPASS);

	OD_REG_SET_FIELD(cmdq, OD_REG04, 0, TABLE_ONLY_W_ADR_INC);
	OD_REG_SET_FIELD(cmdq, OD_REG04, 0, ADDR_YX);

	mask = ~(0x7 << 19);
	OD_REG_SET_FIELD(cmdq, OD_REG04, 7, TABLE_RW_SEL_OD_BGR);

	switch (TableSel) {
	case OD_TABLE_33:
		u4TblSize = OD_TBL_M_SIZE;
		OD_REG_SET_FIELD(cmdq, OD_REG32, 0, OD_IDX_41);
		OD_REG_SET_FIELD(cmdq, OD_REG32, 0, OD_IDX_17);
		break;

	case OD_TABLE_17:
		u4TblSize = OD_TBL_S_SIZE;
		OD_REG_SET_FIELD(cmdq, OD_REG32, 0, OD_IDX_41);
		OD_REG_SET_FIELD(cmdq, OD_REG32, 1, OD_IDX_17);
		break;

	default:
		return 0;
	}

	for (i = 0; i < u4TblSize; i++) {
		u4TblVal = DISP_REG_GET(OD_REG05);

		if (table_inverse) {
			u8 value = ABS(255 - *(pTable+i));

			if (value != u4TblVal)
				u4ErrCnt++;

		} else {
			if (*(pTable+i) != u4TblVal) {
				ODDBG(OD_DBG_ALWAYS, "OD %d TBL %d %d != %d\n", ColorSel, i, *(pTable+i), u4TblVal);
				u4ErrCnt++;
			}
		}
	}

	DISP_REG_SET(cmdq, OD_REG04, 0);
	OD_REG_SET_FIELD(cmdq, OD_REG02, u1ODBypass, ODT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG02, u1FBBypass, FBT_BYPASS);
	OD_REG_SET_FIELD(cmdq, OD_REG45, !u1PCIDBypass, OD_PCID_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG45, u1PCIDBypass, OD_PCID_BYPASS);

	return u4ErrCnt;
}


static void _od_set_table(void *cmdq, int tableSelect, u8 *od_table, int table_inverse)
{
    /* Write OD table */
	if (OD_TABLE_17 == tableSelect) {
		_od_write_table(cmdq, OD_TABLE_17, OD_ALL, od_table, table_inverse);

		/* Check OD table */
		/*
		#ifndef DEF_CMDQ
		_od_read_table(OD_TABLE_17, OD_RED, OD_Table_17x17, table_inverse);
		_od_read_table(OD_TABLE_17, OD_GREEN, OD_Table_17x17, table_inverse);
		_od_read_table(OD_TABLE_17, OD_BLUE, OD_Table_17x17, table_inverse);
		#endif
		*/

		DISP_REG_SET(cmdq, OD_REG02, (1 << 10));
		OD_REG_SET_FIELD(cmdq, OD_REG45, 1, OD_PCID_BYPASS);
		OD_REG_SET_FIELD(cmdq, OD_REG45, 0, OD_PCID_EN);
		DISP_REG_SET(cmdq, OD_REG12, 1 << 0);
	} else if (OD_TABLE_33 == tableSelect) {
		_od_write_table(cmdq, OD_TABLE_33, OD_ALL, od_table, table_inverse);

		/* Check OD table */
		/*
		#ifndef DEF_CMDQ
		 _od_read_table(OD_TABLE_33, OD_RED, OD_Table_33x33, table_inverse);
		 _od_read_table(OD_TABLE_33, OD_GREEN, OD_Table_33x33, table_inverse);
		_od_read_table(OD_TABLE_33, OD_BLUE, OD_Table_33x33, table_inverse);
		#endif
		*/

		DISP_REG_SET(cmdq, OD_REG02, (1 << 10));
		OD_REG_SET_FIELD(cmdq, OD_REG45, 1, OD_PCID_BYPASS);
		OD_REG_SET_FIELD(cmdq, OD_REG45, 0, OD_PCID_EN);
		DISP_REG_SET(cmdq, OD_REG12, 1 << 0);
	} else {
		ODDBG(OD_DBG_ALWAYS, "Error OD table\n");
		BUG();
	}
}

static void od_dbg_dump(void)
{
	ODDBG(OD_DBG_ALWAYS, "OD EN %d INPUT %d %d\n", DISP_REG_GET(DISP_OD_EN),
		DISP_REG_GET(DISP_OD_INPUT_COUNT) >> 16, DISP_REG_GET(DISP_OD_INPUT_COUNT) & 0xFFFF);
	ODDBG(OD_DBG_ALWAYS, "STA 0x%08x\n", DISP_REG_GET(OD_STA00));
	ODDBG(OD_DBG_ALWAYS, "REG49 0x%08x\n", DISP_REG_GET(OD_REG49));
}


static void od_test_stress_table(void *cmdq)
{
	int i;

	ODDBG(OD_DBG_ALWAYS, "OD TEST -- STRESS TABLE START\n");

	DISP_REG_SET(cmdq, DISP_OD_MISC, 1); /* [1]:can access OD table; [0]:can't access OD table */

	/* read/write table for 100 times, 17x17 and 33x33 50 times each */
	for (i = 0; i < 50; i++) {
		/* test 17 table */
		_od_set_table(cmdq, OD_TABLE_17, OD_Table_17x17, 0);
		_od_read_table(cmdq, OD_TABLE_17, 0, OD_Table_17x17, 0);
		_od_read_table(cmdq, OD_TABLE_17, 1, OD_Table_17x17, 0);
		_od_read_table(cmdq, OD_TABLE_17, 2, OD_Table_17x17, 0);

		/* test 33 table */
		_od_set_table(cmdq, OD_TABLE_33, OD_Table_33x33, 0); /* default use 17x17 table */
		_od_read_table(cmdq, OD_TABLE_33, 0, OD_Table_33x33, 0);
		_od_read_table(cmdq, OD_TABLE_33, 1, OD_Table_33x33, 0);
		_od_read_table(cmdq, OD_TABLE_33, 2, OD_Table_33x33, 0);
	}

	DISP_REG_SET(cmdq, DISP_OD_MISC, 0); /* [1]:can access OD table; [0]:can't access OD table */

	ODDBG(OD_DBG_ALWAYS, "OD TEST -- STRESS TABLE END\n");
}


void disp_config_od(unsigned int width, unsigned int height, void *cmdq, unsigned int od_table_size, void *od_table)
{
	int manual_cpr = OD_MANUAL_CPR;

	int od_table_select = 0;

	ODDBG(OD_DBG_ALWAYS, "OD conf start %lx %x %lx\n", (unsigned long)cmdq, od_table_size, (unsigned long)od_table);

	switch (od_table_size) {
	case 17*17:
		od_table_select = OD_TABLE_17;
		break;

	case 33*33:
		od_table_select = OD_TABLE_33;
		break;

    /* default linear table */
	default:
		od_table_select = OD_TABLE_17;
		od_table = OD_Table_dummy_17x17;
		break;
	}

	if (od_table == NULL) {
		ODDBG(OD_DBG_ALWAYS, "LCM NULL OD table\n");
		BUG();
	}

	DISP_REG_SET(cmdq, DISP_OD_EN, 1);

	DISP_REG_SET(cmdq, DISP_OD_SIZE, (width << 16) | height);
	DISP_REG_SET(cmdq, DISP_OD_HSYNC_WIDTH, OD_HSYNC_WIDTH);
	DISP_REG_SET(cmdq, DISP_OD_VSYNC_WIDTH, (OD_HSYNC_WIDTH << 16) | (width * 3 / 2));

	/* OD register init */
	od_init_table_init();

	_od_reg_init(cmdq, OD_INIT_TABLE, sizeof(OD_INIT_TABLE) / sizeof(OD_INIT_TABLE[0]));
	_od_set_dram_buffer_addr(cmdq, manual_cpr, width, height);


	_od_set_frame_protect_init(cmdq, width, height);

	/* OD on/off align to vsync */
	DISP_REG_SET(cmdq, OD_REG53, DISP_REG_GET(OD_REG53) | (1 << 30));

	/* _od_set_param(38, width, height); */
	_od_set_param(cmdq, manual_cpr, width, height);

	DISP_REG_SET(cmdq, OD_REG53, 0x6BFB7E00);

	DISP_REG_SET(cmdq, DISP_OD_MISC, 1); /* [1]:can access OD table; [0]:can't access OD table */

	/* _od_set_table(OD_TABLE_17, 0); /// default use 17x17 table */
	_od_set_table(cmdq, od_table_select, od_table, 0);

	DISP_REG_SET(cmdq, DISP_OD_MISC, 0); /* [1]:can access OD table; [0]:can't access OD table */

	/* modified ALBUF2_DLY OD_REG01 */

	/* OD_REG_SET_FIELD(cmdq, OD_REG01, (0xD), ALBUF2_DLY); // 1080w */
	OD_REG_SET_FIELD(cmdq, OD_REG01, (0xE), ALBUF2_DLY); /* 720w */

	/* enable INK */
    /* OD_REG_SET_FIELD(cmdq, OD_REG03, 1, ODT_INK_EN); */

    /* disable hold for debug */
    /*
	OD_REG_SET_FIELD(cmdq, OD_REG71, 0, RG_WDRAM_HOLD_EN);
	OD_REG_SET_FIELD(cmdq, OD_REG72, 0, RG_RDRAM_HOLD_EN);
	*/

	/* enable debug OSD for status reg */
    /* OD_REG_SET_FIELD(cmdq, OD_REG46, 1, OD_OSD_SEL); */

	/* lower hold threshold */
	/*
	OD_REG_SET_FIELD(cmdq, OD_REG73, 0, RG_WDRAM_HOLD_THR);
	OD_REG_SET_FIELD(cmdq, OD_REG73, 0, RG_RDRAM_HOLD_THR);
	*/

    /* restore demo mode for suspend / resume */
	if (g_od_is_demo_mode)
		OD_REG_SET_FIELD(cmdq, OD_REG02, 1, DEMO_MODE);

	OD_REG_SET_FIELD(cmdq, OD_REG00, 0, BYPASS_ALL);

    /* GO OD. relay = 0, od_core_en = 1, DITHER_EN = 1 */
	/* Dynamic turn on from ioctl */
    /* DISP_REG_SET(cmdq, DISP_OD_CFG, 2); */


	/* clear crc error first */
	OD_REG_SET_FIELD(cmdq, OD_REG38, 1, DRAM_CRC_CLR);


	if (od_debug_level >= OD_DBG_DEBUG)
		od_dbg_dump();

	ODDBG(OD_DBG_ALWAYS, "OD inited W %d H %d\n", width, height);

	g_od_function_availible = 1;
	g_od_counter = 0;
	g_od_status = OD_STATUS_IDLE;
	g_od_is_enabled = (g_od_is_enabled_force == 1) ? 1 : 0;

	DISP_REG_MASK(cmdq, DISP_REG_OD_INTEN, 0x43, 0xff);
#if 1
	/* OD kernel thread for monitor OD status */
	if (g_od_initial_config == 0) {
		disp_od_thread = kthread_create(disp_od_update_worker, NULL, "od_update_worker");
		ODDBG(OD_DBG_ALWAYS, "od kernel thread created");
		g_od_initial_config = 1;
		cmdqBackupAllocateSlot(&g_od_hSlot, 3);
	} else {
		ODDBG(OD_DBG_ALWAYS, "od kernel thread existed");
		/* wake_up_interruptible(&odevent_waitqueue); */
	}
	wake_up_process(disp_od_thread);

#endif
	/* workaround for debug (OSD+INK) */
	if (g_od_is_debug_mode == 1) {
		OD_REG_SET_FIELD(cmdq, OD_REG46, 1, OD_OSD_SEL);
		DISP_REG_MASK(cmdq, OD_REG03, 0, 0x1);
	} else if (g_od_is_debug_mode == 2) {
		OD_REG_SET_FIELD(cmdq, OD_REG46, 1, OD_OSD_SEL);
		DISP_REG_MASK(cmdq, OD_REG03, (0x33 << 24) | (0xaa << 16) | (0x55 << 8) | 1, 0xffffff01);
	} else {
		OD_REG_SET_FIELD(cmdq, OD_REG46, 0, OD_OSD_SEL);
		DISP_REG_MASK(cmdq, OD_REG03, 0, 0x1);
	}
}

int disp_od_get_enabled(void)
{
	return g_od_is_enabled;
}

void disp_od_mhl_force(int mhl)
{
	g_od_is_enabled_from_mhl = mhl;
}

void disp_od_set_enabled(void *cmdq, int enabled)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	if (g_od_function_availible == 0) {
		ODDBG(OD_DBG_ALWAYS, "disp_od_set_enabled un-availible");
		return;
	}

	if (g_od_is_enabled_force == 2)
		g_od_is_enabled = (enabled & 0x1 & g_od_is_enabled_from_mhl & g_od_is_enabled_from_hwc);
	else if (g_od_is_enabled_force == 1)
		g_od_is_enabled = 1;
	else
		g_od_is_enabled = 0;

	if (_disp_od_read_dram_status(cmdq) == -1) {
		g_od_is_enabled = 0;
		ODERR("dram status error: force disable OD\n");
	}

	ODDBG(OD_DBG_ALWAYS, "disp_od_set_enabled=%d (in:%d)(hwc:%d)(mhl:%d)\n",
		g_od_is_enabled, enabled, g_od_is_enabled_from_hwc, g_od_is_enabled_from_mhl);

	if (g_od_function_availible == 1) {
		_disp_od_core_enabled(cmdq, g_od_is_enabled);
		g_od_status = OD_STATUS_ENABLE;
	}

#endif
}

/*
 * Must be called after the 3rd frame after disp_od_set_enabled(1)
 * to enable OD function
 */
void disp_od_start_read(void *cmdq)
{
#if defined(CONFIG_MTK_OD_SUPPORT)
	OD_REG_SET_FIELD(cmdq, OD_REG37, 0xf, ODT_MAX_RATIO);
	ODDBG(OD_DBG_ALWAYS, "disp_od_start_read(%d)", g_od_is_enabled);
#endif
}

static void disp_od_set_enabled_user(unsigned long user_arg, void *cmdq)
{
	int enabled = 0;

	if (copy_from_user((void *)&enabled, (void *)user_arg, sizeof(int)))
		return -EFAULT;

	disp_od_set_enabled(cmdq, enabled);
}

static int disp_od_ioctl_ctlcmd(DISP_MODULE_ENUM module, int msg, unsigned long arg, void *cmdq)
{
	DISP_OD_CMD cmd;

	if (copy_from_user((void *)&cmd, (void *)arg, sizeof(DISP_OD_CMD)))
		return -EFAULT;

	ODDBG(OD_DBG_ALWAYS, "OD ioctl cmdq %lx\n", (unsigned long)cmdq);

	switch (cmd.type) {
	/* on/off OD */
	case OD_CTL_ENABLE:
	{
		if (cmd.param0 == OD_CTL_ENABLE_OFF) {
			g_od_is_enabled_from_hwc = 0;
			disp_od_set_enabled(cmdq, 0);
			ODDBG(OD_DBG_ALWAYS, "disp_od_set_disabled\n");
		} else if (cmd.param0 == OD_CTL_ENABLE_ON) {
			g_od_is_enabled_from_hwc = 1;
			disp_od_set_enabled(cmdq, 1);
			ODDBG(OD_DBG_ALWAYS, "disp_od_set_enable\n");
		} else {
			ODDBG(OD_DBG_ALWAYS, "unknown enable type command\n");
		}
	}
	break;
    /* read reg */
	case OD_CTL_READ_REG:
	{
		if (cmd.param0 < 0x1000) { /* deny OOB access */
			cmd.ret = OD_REG_GET(cmd.param0 + OD_BASE);
		} else {
			cmd.ret = 0;
		}
	}
	break;

    /* write reg */
	case OD_CTL_WRITE_REG:
	{
		if (cmd.param0 < 0x1000) { /* deny OOB access */
			OD_REG_SET(cmdq, cmd.param0 + OD_BASE, cmd.param1);
			cmd.ret = OD_REG_GET(cmd.param0 + OD_BASE);
		} else {
			cmd.ret = 0;
		}
	}
	break;

    /* enable split screen OD demo mode for miravision */
	case OD_CTL_ENABLE_DEMO_MODE:
	{
		switch (cmd.param0) {
		/* demo mode */
		case 0:
		case 1:
		{
			int enable = cmd.param0 ? 1 : 0;

			OD_REG_SET_FIELD(cmdq, OD_REG02, enable, DEMO_MODE);

			ODDBG(OD_DBG_ALWAYS, "OD demo %d\n", enable);
			/* save demo mode flag for suspend/resume */
			g_od_is_demo_mode = enable;
		}
		break;

		/* enable ink */
		case 2: /* off */
		case 3: /* on */
			OD_REG_SET_FIELD(cmdq, OD_REG03, (cmd.param0 - 2), ODT_INK_EN);
		break;

		/* eanble debug OSD */
		case 4: /* off */
		case 5: /* on */
			OD_REG_SET_FIELD(cmdq, OD_REG46, (cmd.param0 - 4), OD_OSD_SEL);
		break;

		default:
		break;
		}

	}
	break;

    /* write od table */
	case OD_CTL_WRITE_TABLE:
	{
		return -EFAULT;
	}
	break;

	default:
	break;
	}

	if (copy_to_user((void *)arg, (void *)&cmd, sizeof(DISP_OD_CMD)))
		return -EFAULT;

	return 0;
}

static int disp_od_ioctl(DISP_MODULE_ENUM module, int msg, unsigned long arg, void *cmdq)
{
	switch (msg) {
	case DISP_IOCTL_OD_CTL:
		return disp_od_ioctl_ctlcmd(module, msg, arg, cmdq);

	default:
		return -EFAULT;
	}

	return 0;
}

static void ddp_bypass_od(unsigned int width, unsigned int height, void *handle)
{
	ODNOTICE("ddp_bypass_od\n");
	DISP_REG_SET(handle, DISP_REG_OD_SIZE, (width << 16) | height);
	/* do not use OD relay mode (dither will be bypassed) od_core_en = 0 */
	DISP_REG_SET(handle, DISP_REG_OD_CFG, 0);
	DISP_REG_SET(handle, DISP_REG_OD_EN, 0x1);
}

static int od_config_od(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *cmdq)
{
	const LCM_PARAMS *lcm_param = &(pConfig->dispif_config);

	if (pConfig->dst_dirty) {
#if defined(CONFIG_MTK_OD_SUPPORT)
		unsigned int od_table_size = lcm_param->od_table_size;
		void *od_table = lcm_param->od_table;

    #if defined(OD_ALLOW_DEFAULT_TABLE)
		if (od_table == NULL) {
			od_table_size = 33 * 33;
			od_table = OD_Table_33x33;
		}
    #endif
    #if defined(OD_LINEAR_TABLE_IF_NONE)
		if (od_table == NULL) {
			od_table_size = 17 * 17;
			od_table = OD_Table_dummy_17x17;
		}
    #endif

		if (od_table != NULL) {
			spm_enable_sodi(0);
			disp_config_od(pConfig->dst_w, pConfig->dst_h, cmdq, od_table_size, od_table);
    #if 0
			/* For debug */
			DISP_REG_MASK(cmdq, DISP_OD_INTEN, (1<<6)|(1<<3)|(1<<2), (1<<6)|(1<<3)|(1<<2));
    #endif
		} else {
			ddp_bypass_od(pConfig->dst_w, pConfig->dst_h, cmdq);
		}

#else /* Not support OD */
		ddp_bypass_od(pConfig->dst_w, pConfig->dst_h, cmdq);
#endif

	}

	if (pConfig->dst_dirty) {
		/*
		dither0 is in the OD module, and it uses OD clk,
		such that the OD clock must be on when screen is on.
		*/
		disp_dither_init(DISP_DITHER0, pConfig->dst_w, pConfig->dst_h,
			pConfig->lcm_bpp, cmdq);

	}

#if defined(CONFIG_MTK_OD_SUPPORT)
	if (!pConfig->dst_dirty &&
		(lcm_param->type == LCM_TYPE_DSI) && (lcm_param->dsi.mode == CMD_MODE)) {
		if (pConfig->ovl_dirty || pConfig->rdma_dirty)
			od_refresh_screen();
	}
#endif

	return 0;
}

static int od_clock_on(DISP_MODULE_ENUM module, void *handle)
{
#ifdef ENABLE_CLK_MGR
	enable_clock(MT_CG_DISP0_DISP_OD, "od");
	DDPMSG("od_clock on CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
#endif
	return 0;
}

static int od_clock_off(DISP_MODULE_ENUM module, void *handle)
{
#ifdef ENABLE_CLK_MGR
	disable_clock(MT_CG_DISP0_DISP_OD , "od");
	DDPMSG("od_clock off CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
#endif
	return 0;
}

/* for SODI to check OD is enabled or not, this will be called when screen is on and disp clock is enabled */
int disp_od_is_enabled(void)
{
	return (DISP_REG_GET(DISP_OD_CFG) & (1 << 1)) ? 1 : 0;
}

static int od_set_listener(DISP_MODULE_ENUM module, ddp_module_notify notify)
{
	g_od_ddp_notify = notify;
	return 0;
}


/* OD driver module */
DDP_MODULE_DRIVER ddp_driver_od = {
	.init            = od_clock_on,
	.deinit          = od_clock_off,
	.config          = od_config_od,
	.start           = NULL,
	.trigger         = NULL,
	.stop            = NULL,
	.reset           = NULL,
	.power_on        = od_clock_on,
	.power_off       = od_clock_off,
	.is_idle         = NULL,
	.is_busy         = NULL,
	.dump_info       = NULL,
	.bypass          = NULL,
	.build_cmdq      = NULL,
	.set_lcm_utils   = NULL,
	.cmd             = disp_od_ioctl,
	.set_listener    = od_set_listener
};



/* ----------------------------------------------------------------------
// Test code
// Following is only for OD functional test, not normal code
// Will not be linked into user build.
// --------------------------------------------------------------------*/

#define OD_TLOG(fmt, arg...) pr_notice("[OD] " fmt "\n", ##arg)

static int od_parse_triple(const char *cmd, unsigned long *offset, unsigned int *value, unsigned int *mask)
{
	int count = 0;
	char *next = (char *)cmd;

	*value = 0;
	*mask = 0;
	*offset = (unsigned long)kstrtoul(next, &next, 0);

	if (*offset > 0x1000UL || (*offset & 0x3UL) != 0)  {
		*offset = 0UL;
		return 0;
	}

	count++;

	if (*next == ',')
		next++;

	*value = (unsigned int)kstrtoul(next, &next, 0);
	count++;

	if (*next == ',')
		next++;

	*mask = (unsigned int)kstrtoul(next, &next, 0);
	count++;

	return count;
}

static void od_dump_reg(const char *reg_list)
{
	unsigned long offset;
	unsigned int value;
	char *next = (char *)reg_list;

	OD_TLOG("OD reg base = %lx", (unsigned long)(OD_BASE));
	while (1) {
		offset = (unsigned long)kstrtoul(next, &next, 0);
		if (offset < 0x1000UL && (offset & 0x3UL) == 0) {
			value = DISP_REG_GET(OD_BASE + offset);
			OD_TLOG("[+0x%03lx] = 0x%08x(%d)", offset, value, value);
		}

		if (next[0] != ',')
			break;

		next++;
	}
}

static void od_test_slow_mode(void)
{
#if 0
	msleep(30);

	/* SLOW */
	WDMASlowMode(DISP_MODULE_WDMA0, 1, 4, 0xff, 0x7, NULL); /* time period between two write request is 0xff */

	ODDBG(OD_DBG_ALWAYS, "OD SLOW\n");

	msleep(2000);

	ODDBG(OD_DBG_ALWAYS, "OD OK\n");
	WDMASlowMode(DISP_MODULE_WDMA0, 0, 0, 0, 0x7, NULL);
#endif
}

static void od_verify_boundary(void)
{
	int guard1, guard2;

	guard1 = (*((u32 *)((unsigned long)g_od_buf_va + g_od_buf_size)) == OD_GUARD_PATTERN);
	guard2 = (*((u32 *)((unsigned long)g_od_buf_va + g_od_buf_size + OD_ADDITIONAL_BUFFER)) == OD_GUARD_PATTERN);

	OD_TLOG("od_verify_boundary(): guard 1 = %d, guard 2 = %d", guard1, guard2);
}

static void od_dump_all(void)
{
	int i;
	static const char *const od_addr_all[] = {
		"0x0000",
		"0x0004",
		"0x0008",
		"0x000C",
		"0x0010",
		"0x0020",
		"0x0024",
		"0x0028",
		"0x002C",
		"0x0030",
		"0x0040",
		"0x0044",
		"0x0048",
		"0x00C0",
		"0x0100",
		"0x0108",
		"0x010c",
		"0x0200",
		"0x0208",
		"0x020c",
		"0x0700",
		"0x0704",
		"0x0708",
		"0x070c",
		"0x0710",
		"0x0714",
		"0x0718",
		"0x071c",
		"0x0720",
		"0x0724",
		"0x0728",
		"0x072c",
		"0x0730",
		"0x0734",
		"0x0738",
		"0x073c",
		"0x0740",
		"0x0744",
		"0x0748",
		"0x074c",
		"0x0750",
		"0x0754",
		"0x0758",
		"0x075c",
		"0x0760",
		"0x0764",
		"0x0768",
		"0x076c",
		"0x0770",
		"0x0774",
		"0x0778",
		"0x077c",
		"0x0788",
		"0x078c",
		"0x0790",
		"0x0794",
		"0x0798",
		"0x079c",
		"0x07a0",
		"0x07a4",
		"0x07a8",
		"0x07ac",
		"0x07b0",
		"0x07b4",
		"0x07b8",
		"0x07bc",
		"0x07c0",
		"0x07c4",
		"0x07c8",
		"0x07cc",
		"0x07d0",
		"0x07d4",
		"0x07d8",
		"0x07dc",
		"0x07e0",
		"0x07e4",
		"0x07e8",
		"0x07ec",
		"0x05c0",
		"0x05c4",
		"0x05c8",
		"0x05cc",
		"0x05d0",
		"0x05d4",
		"0x05d8",
		"0x05dc",
		"0x05e0",
		"0x05e4",
		"0x05e8",
		"0x05ec",
		"0x05f0",
		"0x05f8",
		"0x05fc",
		"0x06c0",
		"0x06c4",
		"0x06c8",
		"0x06cc",
		"0x06d0",
		"0x06d4",
		"0x06d8",
		"0x06dc",
		"0x06e0",
		"0x06e4",
		"0x06e8",
		"0x06ec",
		"0x0580",
		"0x0584",
		"0x0588",
		"0x0500",
		"0x0504",
		"0x0508",
		"0x050c",
		"0x0510",
		"0x0680",
		"0x0684",
		"0x0688",
		"0x068c",
		"0x0690",
		"0x0694",
		"0x0698",
		"0x0540",
		"0x0544",
		"0x0548"
	};

	for (i = 0; i < 123; i++)
		od_dump_reg(od_addr_all[i]);

}

void od_test(const char *cmd, char *debug_output)
{
	unsigned long offset;
	unsigned int value, mask;

	int bbp = 0;

	OD_TLOG("od_test(%s)", cmd);

	debug_output[0] = '\0';

	DISP_CMDQ_BEGIN(cmdq, CMDQ_SCENARIO_DISP_CONFIG_OD)

	if (strncmp(cmd, "set:", 4) == 0) {
		int count = od_parse_triple(cmd + 4, &offset, &value, &mask);

		if (count == 3) {
			DISP_REG_MASK(NULL, OD_BASE + offset, value, mask);
		} else if (count == 2) {
			DISP_REG_SET(NULL, OD_BASE + offset, value);
			mask = 0xffffffff;
		}

		if (count >= 2)
			OD_TLOG("[+0x%03lx] = 0x%08x(%d) & 0x%08x", offset, value, value, mask);

	} else if (strncmp(cmd, "dump:", 5) == 0) {
		od_dump_reg(cmd + 5);
	} else if (strncmp(cmd, "dumpall", 7) == 0) {
		od_dump_all();
	} else if (strncmp(cmd, "stress", 6) == 0) {
		od_test_stress_table(NULL);
	} else if (strncmp(cmd, "slow_mode", 9) == 0) {
		od_test_slow_mode();
	} else if (strncmp(cmd, "boundary", 6) == 0) {
		od_verify_boundary();
	} else if (strncmp(cmd, "sodi:", 5) == 0) {
		int enabled = (cmd[5] == '1' ? 1 : 0);

		spm_enable_sodi(enabled);
	} else if (strncmp(cmd, "en:", 3) == 0) {
		int enabled = (cmd[3] == '1' ? 1 : 0);

		disp_od_set_enabled(cmdq, enabled);

	} else if (strncmp(cmd, "force:", 6) == 0) {
		if (cmd[6] == '0') {
			g_od_is_enabled_force = 0;
			disp_od_set_enabled(cmdq, g_od_is_enabled_force);
			_disp_od_start(cmdq);
		} else if (cmd[6] == '1') {
			g_od_is_enabled_force = 1;
			disp_od_set_enabled(cmdq, g_od_is_enabled_force);
			_disp_od_start(cmdq);
		} else {
			g_od_is_enabled_force = 2;
		}
	} else if (strncmp(cmd, "ink:", 4) == 0) {
		int enabled = (cmd[4] == '1' ? 1 : 0);

		if (enabled) {
			g_od_is_debug_mode = 1;
			DISP_REG_MASK(cmdq, OD_REG03, (0x33 << 24) | (0xaa << 16) | (0x55 << 8) | 1, 0xffffff01);
		} else {
			g_od_is_debug_mode = 0;
			DISP_REG_MASK(cmdq, OD_REG03, 0, 0x1);
		}
	} else if (strncmp(cmd, "osd:", 4) == 0) {
		if (cmd[4] == '1') {
			g_od_is_debug_mode = 1;
			OD_REG_SET_FIELD(cmdq, OD_REG46, 1, OD_OSD_SEL);
			DISP_REG_MASK(cmdq, OD_REG03, 0, 0x1);
		} else if (cmd[4] == '2') {
			g_od_is_debug_mode = 2;
			OD_REG_SET_FIELD(cmdq, OD_REG46, 1, OD_OSD_SEL);
			DISP_REG_MASK(cmdq, OD_REG03, (0x33 << 24) | (0xaa << 16) | (0x55 << 8) | 1, 0xffffff01);
		} else {
			g_od_is_debug_mode = 0;
			OD_REG_SET_FIELD(cmdq, OD_REG46, 0, OD_OSD_SEL);
			DISP_REG_MASK(cmdq, OD_REG03, 0, 0x1);
		}
	} else if (strncmp(cmd, "demo:", 5) == 0) {
		int enabled = (cmd[5] == '1' ? 1 : 0);

		OD_REG_SET_FIELD(cmdq, OD_REG02, enabled, DEMO_MODE);
		ODDBG(OD_DBG_ALWAYS, "OD demo %d\n", enabled);
		/* save demo mode flag for suspend/resume */
		g_od_is_demo_mode = enabled;
	}

	DISP_CMDQ_CONFIG_STREAM_DIRTY(cmdq);
	DISP_CMDQ_END(cmdq)

	od_refresh_screen();

}

#endif

