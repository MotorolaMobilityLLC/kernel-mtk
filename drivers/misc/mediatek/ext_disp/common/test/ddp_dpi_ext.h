#ifndef __DDP_DPI_EXT_H__
#define __DDP_DPI_EXT_H__

#include "lcm_drv.h"
#include "ddp_info.h"
#include "cmdq_record.h"
#include "dpi_dvt_test.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DPI_STATUS_OK = 0,
	DPI_STATUS_ERROR,
} DPI_STATUS;

/*************************for DPI DVT***************************/
typedef enum {
	acsRGB         = 0
	, acsYCbCr422    = 1
	, acsYCbCr444    = 2
	, acsFuture      = 3
} AviColorSpace_e;

DPI_STATUS DPI_EnableColorBar(unsigned int pattern);
DPI_STATUS DPI_DisableColorBar(void);
DPI_STATUS ddp_dpi_EnableColorBar_0(void);
DPI_STATUS ddp_dpi_EnableColorBar_16(void);

int configInterlaceMode(unsigned int resolution);
int config3DMode(unsigned int resolution);
int config3DInterlaceMode(unsigned int resolution);
unsigned int readDPIStatus(void);
unsigned int readDPITDLRStatus(void);
unsigned int clearDPIStatus(void);
unsigned int clearDPIIntrStatus(void);
unsigned int readDPIIntrStatus(void);
unsigned int ClearDPIIntrStatus(void);
unsigned int enableRGB2YUV(AviColorSpace_e format);
unsigned int enableSingleEdge(void);
int enableAndGetChecksum(void);
int enableAndGetChecksumCmdq(cmdqRecHandle cmdq_handle);
unsigned int configDpiRepetition(void);
unsigned int configDpiEmbsync(void);
unsigned int configDpiColorTransformToBT709(void);
unsigned int configDpiRGB888ToLimitRange(void);
/************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __DPI_DRV_H__ */
