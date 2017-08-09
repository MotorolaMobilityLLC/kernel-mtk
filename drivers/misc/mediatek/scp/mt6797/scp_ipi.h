#ifndef __SCP_IPI_H
#define __SCP_IPI_H

#define SCP_TO_HOST_REG        (*(volatile unsigned int *)(scpreg.cfg + 0x001C))
#define SCP_SCP_TO_SPM_REG     (*(volatile unsigned int *)(scpreg.cfg + 0x0020))
#define HOST_TO_SCP_REG        (*(volatile unsigned int *)(scpreg.cfg + 0x0024))
#define SCP_SPM_TO_SCP_REG     (*(volatile unsigned int *)(scpreg.cfg + 0x0028))
#define SCP_DEBUG_PC_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00B4))
#define SCP_DEBUG_PSP_REG      (*(volatile unsigned int *)(scpreg.cfg + 0x00B0))
#define SCP_DEBUG_LR_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00AC))
#define SCP_DEBUG_SP_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00A8))
#define SCP_WDT_REG            (*(volatile unsigned int *)(scpreg.cfg + 0x0084))
#define SCP_TO_SPM_REG         (*(volatile unsigned int *)(scpreg.cfg + 0x0020))
#define SCP_GENERAL_REG0       (*(volatile unsigned int *)(scpreg.cfg + 0x0050))
#define SCP_SLEEP_DEBUG_REG    (*(volatile unsigned int *)(scpreg.clkctrl + 0x0028))

#define SCP_IRQ_MD2HOST     (1 << 0)
#define SCP_IRQ_WDT         (1 << 8)

typedef enum ipi_id {
	IPI_WDT = 0,
	IPI_TEST1,
	IPI_TEST2,
	IPI_LOGGER,
	IPI_SWAP,
	IPI_VOW,
	IPI_SPK_PROTECT,
	IPI_THERMAL,
	IPI_SPM,
	IPI_DVT_TEST,
	IPI_I2C,
	IPI_TOUCH,
	IPI_SENSOR,
	IPI_TIME_SYNC,
	IPI_GPIO,
	IPI_BUF_FULL,
	IPI_DFS,
	IPI_SHF,
	IPI_CONSYS,
	IPI_OCD,
	IPI_APDMA,
	IPI_IRQ_AST,
	IPI_DFS4MD,
	IPI_SCP_READY,
	IPI_ETM_DUMP,
	IPI_APCCCI,
	IPI_RAM_DUMP,
	IPI_DVFS_DEBUG,
	IPI_DVFS_FIX_OPP_SET,
	IPI_DVFS_FIX_OPP_EN,
	IPI_DVFS_LIMIT_OPP_SET,
	IPI_DVFS_LIMIT_OPP_EN,
	IPI_DVFS_DISABLE,
	IPI_DVFS_INFO_DUMP,
	IPI_DUMP_REG,
	IPI_SCP_STATE,
	SCP_NR_IPI,
} ipi_id;

typedef enum ipi_status {
	ERROR = -1,
	DONE,
	BUSY,
} ipi_status;

typedef void (*ipi_handler_t)(int id, void *data, unsigned int len);

struct scp_ipi_desc {
	ipi_handler_t handler;
	const char *name;
};

#define SHARE_BUF_SIZE 64
struct share_obj {
	enum ipi_id id;
	unsigned int len;
	unsigned char reserve[8];
	unsigned char share_buf[SHARE_BUF_SIZE - 16];
};

extern ipi_status scp_ipi_registration(enum ipi_id id, ipi_handler_t handler, const char *name);
extern ipi_status scp_ipi_send(enum ipi_id id, void *buf, unsigned int len, unsigned int wait);
extern void scp_ipi_handler(void);
extern int wake_up_scp(void);

extern unsigned char *scp_send_buff;
extern unsigned char *scp_recv_buff;

#endif
