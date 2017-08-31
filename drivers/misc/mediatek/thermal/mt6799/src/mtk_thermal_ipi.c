#include "mtk_thermal_ipi.h"
#include "mach/mtk_thermal.h"
#include "tscpu_settings.h"

#if THERMAL_ENABLE_TINYSYS_SSPM
/* ipi_send() return code
 * IPI_DONE 0
 * IPI_RETURN 1
 * IPI_BUSY -1
 * IPI_TIMEOUT_AVL -2
 * IPI_TIMEOUT_ACK -3
 * IPI_MODULE_ID_ERROR -4
 * IPI_HW_ERROR -5
 */
unsigned int thermal_to_sspm(unsigned int cmd, struct thermal_ipi_data *thermal_data)
{
	int ackData = -1;
	int ret;

	switch (cmd) {
	case THERMAL_IPI_INIT_GRP1:
	case THERMAL_IPI_INIT_GRP2:
	case THERMAL_IPI_INIT_GRP3:
	case THERMAL_IPI_INIT_GRP4:
	case THERMAL_IPI_INIT_GRP5:
	case THERMAL_IPI_INIT_GRP6:
		thermal_data->cmd = cmd;
		ret = sspm_ipi_send_sync(IPI_ID_THERMAL, IPI_OPT_LOCK_POLLING, thermal_data,
			THERMAL_SLOT_NUM, &ackData);
		if (ret != 0)
			tscpu_printk("sspm_ipi_send_sync error(THERMAL_IPI_INIT) ret:%d - %d\n",
					ret, ackData);
		else if (ackData < 0)
			tscpu_printk("cmd(%d) return error(%d)\n", cmd, ackData);
		break;

	case THERMAL_IPI_GET_TEMP:
		thermal_data->cmd = cmd;
		ret = sspm_ipi_send_sync(IPI_ID_THERMAL, IPI_OPT_LOCK_POLLING, thermal_data, 0, &ackData);
		if (ret != 0)
			tscpu_printk("sspm_ipi_send_sync error(THERMAL_IPI_GET_TEMP) ret:%d - %d\n",
					ret, ackData);
		else if (ackData < 0)
			tscpu_printk("cmd(%d) return error(%d)\n", cmd, ackData);
		break;
	default:
		tscpu_printk("cmd(%d) wrong!!\n", cmd);
		break;
	}

	return ackData;
}
#endif
