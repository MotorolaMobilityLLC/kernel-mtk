/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef _MTK_PCIE_
#define _MTK_PCIE_

#include <linux/irq.h>


extern unsigned int mtk_pci_dev_num;
extern unsigned long mtk_pci_dev_data[255];
extern int pci_find_ext_capability(struct pci_dev *dev, int cap);


#define PCI_SPEED_MASK  0x03
#define L2_SUSPEND_SECONDS	6
#define CHECK_ENTER_L2_COUNT	1000


extern void     __iomem *MTK_REG_BASE;
#define MTK_INT_MASK					(MTK_REG_BASE+0x0420)
#define MTK_INT_L2_WAKE					(0x40000000)
#define MTK_INT_LTR_MSG					(0x08000000)
#define MTK_INT_STATUS					(MTK_REG_BASE+0x0424)
#define MTK_ICMD					(MTK_REG_BASE+0x0434)
#define mtk_icmd_send_int				(0x0001)
#define mtk_icmd_int_state				(0x0002)
#define mtk_icmd_send_pme				(0x0004)
#define mtk_icmd_deassert_clkreq			(0x0008)
#define mtk_icmd_to_link				(0x0010)
#define MTK_PCI_RSTCR					(MTK_REG_BASE+0x0510)
#define MTK_PCI_PHY_RSTB				(0x0001)
#define MTK_PCI_PIPE_SRSTB				(0x0002)
#define MTK_PCI_MAC_SRSTB				(0x0004)
#define MTK_PCI_CRSTB					(0x0008)
#define MTK_PCI_PERSTB_PHY_ENABLE			(0x0010)
#define MTK_PCI_PERSTB_PIPE_ENABLE			(0x0020)
#define MTK_PCI_PERSTB_MAC_ENABLE			(0x0040)
#define MTK_PCI_PERSTB_CONF_ENABLE			(0x0080)
#define MTK_PCI_PERSTB					(0x0100)
#define MTK_REG_WAKE_CONTROL				(MTK_REG_BASE+0x052c)
#define mtk_rg_clkreq_n_enable				(0x0001)
#define mtk_rg_wake_n_enable				(0x0002)
#define mtk_rg_rc_drive_clkreq_dis			(0x0100)
#define MTK_TEST_OUT_00					(MTK_REG_BASE+0x0804)
#define state_detect_quiet				(0x0000)
#define state_detect_active				(0x0001)
#define state_polling_active				(0x0002)
#define state_polling_compliance			(0x0003)
#define state_polling_configuration			(0x0004)
#define state_config_linkwidthstart			(0x0006)
#define state_config_linkaccept				(0x0007)
#define state_config_lanenumaccept			(0x0008)
#define state_config_lanenumwait			(0x0009)
#define state_config_complete				(0x000A)
#define state_config_idle				(0x000B)
#define state_recovery_rcvlock				(0x000C)
#define state_recovery_rcvconfig			(0x000D)
#define state_recovery_idle				(0x000E)
#define state_L0					(0x000F)
#define state_disable					(0x0010)
#define state_loopback_entry				(0x0011)
#define state_loopback_active				(0x0012)
#define state_loopback_exit				(0x0013)
#define state_hot_reset					(0x0014)
#define state_L0s					(0x0015)
#define state_L1_entry					(0x0016)
#define state_L1_idle					(0x0017)
#define state_L2_idle					(0x0018)
#define state_recovery_speed				(0x001A)
#define ltssm_mask					(0x001F)
#define MTK_REG_LTR_LATENCY				(MTK_REG_BASE+0x0568)
#define MTK_REG_OBFF_0					(MTK_REG_BASE+0x0520)
#define csr_obff_state_mask				(0x0300)
#define csr_obff_state_cpu_active			(0x0000)
#define csr_obff_state_obff				(0x0100)
#define csr_obff_state_idle				(0x0200)
#define MTK_REG_MPCIE_EN				(MTK_REG_BASE+0x0570)
#define mtk_reg_mpcie_func_en				(0x0001)
#define mtk_reg_mpcie_sw_ltssm_en			(0x0002)
#define mtk_reg_mpcie_stall_en				(0x0004)
#define mtk_reg_mpcie_t2r_lpbk				(0x0100)
#define mtk_reg_mpcie_hsrate_ctrl			(0x3000000)
#define MTK_MPCIE_LINK_STATUS				(MTK_REG_BASE+0x059C)
#define mtk_reg_mpcie_cur_hsgear			(0x0007)
#define mtk_reg_mpcie_cur_hsrate			(0x0300)



#define  PCI_EXT_CAP_ID_L1PMSS			0x1E	/* Capability ID */
#define  PCI_L1PMSS_CAP				4	/* L1 PM Substates Capabilities Register */
#define  PCI_L1PM_CAP_PCIPM_L12			0x00000001	/* PCI-PM L1.2 is supported */
#define  PCI_L1PM_CAP_PCIPM_L11			0x00000002	/* PCI-PM L1.1 is supported */
#define  PCI_L1PM_CAP_ASPM_L12			0x00000004	/* ASPM L1.2 is supported */
#define  PCI_L1PM_CAP_ASPM_L11			0x00000008	/* ASPM L1.1 is supported */
#define  PCI_L1PM_CAP_L1PM_SS			0x00000010	/* support L1 PM Substates */
#define  PCI_L1PM_CAP_PORT_CMR_TIME_MASK	0x0000FF00	/* port common mode restore time */
#define  PCI_L1PM_CAP_PWR_ON_SCALE_MASK		0x00030000	/* LTR L1.2 threshold value */
#define  PCI_L1PM_CAP_PWR_ON_VALUE_MASK		0x00F80000	/* LTR L1.2 threshold scale */
#define  PCI_L1PMSS_CTR1			8	/* L1 PM Substates Control 1 Register */
#define  PCI_L1PM_CTR1_PCIPM_L12_EN		0x00000001	/* PCI-PM L1.2 Enable */
#define  PCI_L1PM_CTR1_PCIPM_L11_EN		0x00000002	/* PCI-PM L1.1 Enable */
#define  PCI_L1PM_CTR1_ASPM_L12_EN		0x00000004	/* ASPM L1.2 Enable */
#define  PCI_L1PM_CTR1_ASPM_L11_EN		0x00000008	/* ASPM L1.1 Enable */
#define  PCI_L1PM_CTR1_CMR_TIME_MASK		0x0000FF00	/* port common mode restore time */
#define  PCI_L1PM_CTR1_LTR_L12_TH_MASK		0x03FF0000	/* LTR L1.2 threshold value */
#define  PCI_L1PM_CTR1_LTR_L12_SCALE_MASK	0xE0000000	/* LTR L1.2 threshold scale */
#define  PCI_L1PMSS_CTR2			8	/* L1 PM Substates Control 2 Register */
#define  PCI_L1PM_CTR2_PWR_ON_SCALE_MASK	0x00000003	/* Power on value scale */
#define  PCI_L1PM_CTR2_PWR_ON_VALUE_MASK	0x000000F8	/* Power on value value */
#define  PCI_L1PMSS_ENABLE_MASK			(PCI_L1PM_CTR1_PCIPM_L12_EN |   \
						 PCI_L1PM_CTR1_PCIPM_L11_EN |   \
						 PCI_L1PM_CTR1_ASPM_L12_EN |    \
						 PCI_L1PM_CTR1_ASPM_L11_EN)


#define PCI_EXP_LTR_EN			0x400   /* Latency tolerance reporting */

#define PCI_LTR_MAX_SNOOP_LAT		0x4
#define PCI_LTR_MAX_NOSNOOP_LAT		0x6
#define PCI_LTR_VALUE_MASK		0x000003ff
#define PCI_LTR_SCALE_MASK		0x00001c00
#define PCI_LTR_SCALE_SHIFT		10

#define  PCI_EXP_OBFF_MASK		0xc0000 /* OBFF support mechanism */
#define  PCI_EXP_OBFF_MSG		0x40000 /* New message signaling */
#define  PCI_EXP_OBFF_WAKE		0x80000 /* Re-use WAKE# for OBFF */

#define  PCI_EXP_OBFF_MSGA_EN		0x2000  /* OBFF enable with Message type A */
#define  PCI_EXP_OBFF_MSGB_EN		0x4000  /* OBFF enable with Message type B */
#define  PCI_EXP_OBFF_WAKE_EN		0x6000  /* OBFF using WAKE# signaling */

#define  PCI_EXP_OBFF_CODE_IDLE		0x0
#define  PCI_EXP_OBFF_CODE_OBFF		0x1
#define  PCI_EXP_OBFF_CODE_CPU_ACTIVE	0xF


#define  PCI_EXT_CAP_ID_MPCIE		0x20	/* Capability ID */





#define MT_HIF_BASE			((mtk_dev->regs) + 0x4000)

/* HIF Sys Revision */
#define HIF_SYS_REV				(MT_HIF_BASE + 0x0000)

/* HIF Low Power Control Host Register */
#define CFG_PCIE_LPCR_HOST			(MT_HIF_BASE + 0x01F0)
/* CFG_PCIE_LPCR_HOST */
#define PCIE_LPCR_HOST_CLR_OWN			BIT(1)
#define PCIE_LPCR_HOST_SET_OWN			BIT(0)

/* HIF Low Power Control Fw Register */
#define CFG_PCIE_LPCR_FW			(MT_HIF_BASE + 0x01F4)
/* CFG_PCIE_LPCR_FW */
#define PCIE_LPCR_FW_CLR_OWN			BIT(0)

#define CFG_PCIE_MISC		(MT_HIF_BASE + 0x0014)

/*MCU Interrupt Control*/
#define CFG_MCU_INT_CTL		(MT_HIF_BASE + 0x01FC)

/*MCU Interrupt Event*/
#define CFG_MCU_INT_EVENT	(MT_HIF_BASE + 0x01F8)

#define MCU_CMD_INT_NUM		(MT_HIF_BASE + 0x0234)

/*PCIE_RDRBUF_ERRDET */
#define PCIE_RDRBUF_ERRDET (MT_HIF_BASE + 0x0258)
#define RDRBUF_ERRDET_INT_EN BIT(2) /* Interrupt enable to MCU when error detected */
#define RDRBUF_ERRDET_ENABLE BIT(1) /* Error detection mechanism enable */
#define RDRBUF_ERRDET_STATUS BIT(0) /* Interrupt status, write 1 clear the interrupt */


#define HIF_SYS_REV         (MT_HIF_BASE + 0x0000)

/* Interrupt Status */
#define MT_INT_SOURCE_CSR   (MT_HIF_BASE + 0x0200)

/* Interrupt Mask */
#define MT_INT_MASK_CSR     (MT_HIF_BASE + 0x0204)

/* Global Configuration */
#define MT_WPDMA_GLO_CFG    (MT_HIF_BASE + 0x0208)

/* Pointer Reset */
#define WPDMA_RST_PTR	(MT_HIF_BASE + 0x020c)

/* Configuration for WPDMA Delayed Interrupt */
#define MT_DELAY_INT_CFG    (MT_HIF_BASE + 0x0210)

/* TX Ring0 Control 0 */
#define MT_TX_RING_PTR		(MT_HIF_BASE + 0x0300)

/* TX Ring0 Control 1 */
#define MT_TX_RING_CNT		(MT_HIF_BASE + 0x0304)

/* TX Ring0 Control 2 */
#define MT_TX_RING_CIDX		(MT_HIF_BASE + 0x0308)

/* TX Ring0 Control 3 */
#define MT_TX_RING_DIDX		(MT_HIF_BASE + 0x030c)

/* RX Ring0 Control 0 */
#define MT_RX_RING_PTR		(MT_HIF_BASE + 0x400)

/* RX Ring0 Control 1 */
#define MT_RX_RING_CNT		(MT_HIF_BASE + 0x404)

/* RX Ring0 Control 2 */
#define MT_RX_RING_CIDX		(MT_HIF_BASE + 0x408)

/* RX Ring0 Control 3 */
#define MT_RX_RING_DIDX		(MT_HIF_BASE + 0x40c)

#define Tx_PDMA_done 0x55
#define Rx_PDMA_done 0xAA


enum mtk_pcie_obff_type {
	MTK_OBFF_TYPE_MSGA = 0,
	MTK_OBFF_TYPE_MSGB,
	MTK_OBFF_TYPE_WAKE,
};

enum mtk_pcie_obff_code {
	MTK_OBFF_CODE_IDLE = 0,
	MTK_OBFF_CODE_OBFF,
	MTK_OBFF_CODE_CPU_ACTIVE,
};

enum mtk_pci_bus_speed {
	MTK_SPEED_2_5GT = 1,
	MTK_SPEED_5_0GT,
	MTK_SPEED_UNKNOWN
};

enum mtk_pcie_reset {

	MTK_WARM_RESET,
	MTK_COLD_RESET,
	MTK_HOT_RESET,
	MTK_DISABLED,
	MTK_FLR
};

enum mtk_pcie_link_state {

	LINK_STATE_L0S = 1,
	LINK_STATE_L1,
	LINK_STATE_L2
};

enum mtk_pcie_pm_exit {

	EXIT_EP,
	EXIT_RC
};
enum mtk_pci_irq {
	TYPE_MSI,
	TYPE_MSIX,
	TYPE_INTx
};

enum mtk_pcie_mem {
	MTK_MEM_READ,
	MTK_MEM_WRITE
};

enum mtk_pcie_loopback {
	LP_NORMAL = 0,
	LP_ASPM_L0S,
	LP_ASPM_L1,
	LP_ASPM_BOTH,
	LP_PCIPM_L1,
	LP_PCIPM_L2
};

struct mkt_aspm_state {
	int	 aspm_us;
	int	 aspm_ds;
};
enum mtk_dma_mode {

	RANDOM_LOOPBACK,
	SCAN_LOOPBACK
};
enum mtk_l1pm_ss {
	L1SS_PCIPM_L12	 = PCI_L1PM_CTR1_PCIPM_L12_EN,
	L1SS_PCIPM_L11	 = PCI_L1PM_CTR1_PCIPM_L11_EN,
	L1SS_ASPM_L12	 = PCI_L1PM_CAP_ASPM_L12,
	L1SS_ASPM_L11	 = PCI_L1PM_CAP_ASPM_L11
};


extern int mtk_set_mps_mrrs(int size);
extern int mtk_pcie_aspm(struct mkt_aspm_state aspm);
extern int mtk_pcie_retrain(void);
extern int mtk_pcie_disabled(void);
extern int mtk_pcie_hot_reset(void);
extern int mtk_pcie_ut_warm_reset(void);
extern int mtk_function_level_reset(void);
extern int mtk_pcie_change_speed(int speed);
extern int mtk_pcie_configuration(void);
extern int mtk_pcie_link_width(void);
extern int mtk_pcie_reset_config(void);
extern int mtk_pcie_prepare_dma(int type, int length);
extern int save_dma_tx_data(void *buf);
extern int compare_dma_data(void *buf);
extern int mtk_pcie_dma_end(int type);
extern int wait_dma_done(void);
extern int mtk_pcie_memory(int type);
extern int mtk_pcie_go_l1(void);
extern int mtk_pcie_l2(int exit);
extern int mtk_pcie_wakeup(void);
extern int mtk_pcie_wait_l2_exit(struct pci_dev *dev);
extern void mtk_pme_interrupt_enable(struct pci_dev *dev, bool enable);
extern void mtk_clear_root_pme_status(struct pci_dev *dev);
extern irqreturn_t mtk_pme_irq(int irq, void *__res);
extern void mtk_enable_pme(void);
extern int mtk_wait_wakeup(void);
extern int mtk_setup_l1pm(int enable);
extern int mtk_setup_l1ss(int ds_mode, int us_mode);
extern int mtk_pci_parent_bus_reset(struct pci_dev *dev);
extern int mtk_pci_link_enabled(struct pci_dev *dev);
extern int mtk_pci_link_disabled(struct pci_dev *dev);
extern void mtk_pci_pe_reset(void);
extern int mtk_pci_check_bar0(struct pci_dev *dev);

extern int mtk_set_own(void);
extern int mtk_clear_own(void);
extern int mtk_pcie_test(void);

extern int gen_packet(int data_mode, int packet_length);
extern int init_txrx_ring(void);
extern int init_pdma(void);

extern int transmit_tx_packet(int iterations, int dma_mode);
extern int wait_dma_done_tx(void);
extern int wait_dma_done_rx(unsigned int Total_times);

extern struct pci_dev *mtk_get_pcie_resource(int class_code);

extern int mtk_create_txrx_ring(void);
extern int mtk_transmit_tx_packet(u32 pkt_len);
extern int mtk_gen_tx_packet(u32 pkt_len);
extern int mtk_free_txrx_ring(void);
extern int mtk_receive_rx_packet(u32 pkt_len);
extern int mtk_compare_txrx_packet(u32 pkt_len);

extern int mtk_pcie_enter_l2(struct pci_dev *dev);
extern int mtk_pcie_exit_l2(struct pci_dev *dev, int wake);

#endif
