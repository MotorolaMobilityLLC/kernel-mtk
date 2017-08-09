#ifndef SHF_KERNEL_H
#define SHF_KERNEL_H

#include <linux/types.h>

/* IPI received data cache */
#define SHF_IPI_PROTOCOL_BYTES (48)
struct ipi_data_t {
	uint8_t data[SHF_IPI_PROTOCOL_BYTES];
	size_t size;
};

struct ipi_buffer_t {
	size_t head;
	size_t tail;
	size_t size;		/* data count */
	struct ipi_data_t *data;
};

#define SHF_IOW(num, dtype)     _IOW('S', num, dtype)
#define SHF_IOR(num, dtype)     _IOR('S', num, dtype)
#define SHF_IOWR(num, dtype)    _IOWR('S', num, dtype)
#define SHF_IO(num)             _IO('S', num)

#define SHF_IPI_SEND            SHF_IOW(1, struct ipi_data_t)
#define SHF_IPI_POLL            SHF_IOR(2, struct ipi_data_t)
#define SHF_GESTURE_ENABLE      SHF_IOW(3, int)

/* #ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT */
/* extern void tpd_scp_wakeup_enable(bool enable); */
/* #endif */

#endif				/* SHF_KERNEL_H */
