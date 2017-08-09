#ifndef __IRQ_H
#define __IRQ_H


#define FIQ_START 0
#define CPU_BRINGUP_SGI 1
#define FIQ_SMP_CALL_SGI 13
#define FIQ_DBG_SGI 14
#ifndef NR_IRQS
#define NR_IRQS (NR_MT_IRQ_LINE+220)
#endif
#define MT_EDGE_SENSITIVE 0
#define MT_LEVEL_SENSITIVE 1
#define MT_POLARITY_LOW   0
#define MT_POLARITY_HIGH  1

#if !defined(__ASSEMBLY__)

enum {
	IRQ_MASK_HEADER = 0xF1F1F1F1,
	IRQ_MASK_FOOTER = 0xF2F2F2F2
};

struct mtk_irq_mask {
	unsigned int header;	/* for error checking */
	__u32 mask0;
	__u32 mask1;
	__u32 mask2;
	__u32 mask3;
	__u32 mask4;
	__u32 mask5;
	__u32 mask6;
	__u32 mask7;
	__u32 mask8;
	unsigned int footer;	/* for error checking */
};

typedef void (*fiq_isr_handler) (void *arg, void *regs, void *svc_sp);

#endif				/* !__ASSEMBLY__ */


#define GIC_PRIVATE_SIGNALS     (32)
#define NR_GIC_SGI              (16)
#define NR_GIC_PPI              (16)
#define GIC_PPI_OFFSET          (27)
#define MT_NR_PPI               (5)
#define MT_NR_SPI               (256)
#define NR_MT_IRQ_LINE          (GIC_PPI_OFFSET + MT_NR_PPI + MT_NR_SPI)

#define GIC_PPI_GLOBAL_TIMER    (GIC_PPI_OFFSET + 0)
#define GIC_PPI_LEGACY_FIQ      (GIC_PPI_OFFSET + 1)
#define GIC_PPI_PRIVATE_TIMER   (GIC_PPI_OFFSET + 2)
#define GIC_PPI_WATCHDOG_TIMER  (GIC_PPI_OFFSET + 3)
#define GIC_PPI_LEGACY_IRQ      (GIC_PPI_OFFSET + 4)

#define MT_BTIF_IRQ_ID                  (GIC_PRIVATE_SIGNALS + 50)
#define MT_DMA_BTIF_TX_IRQ_ID              (GIC_PRIVATE_SIGNALS + 71)
#define MT_DMA_BTIF_RX_IRQ_ID              (GIC_PRIVATE_SIGNALS + 72)


#endif
