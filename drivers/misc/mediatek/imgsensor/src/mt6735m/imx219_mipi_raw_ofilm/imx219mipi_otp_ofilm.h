#ifndef _IMX219MIPI_OFILM_OTP_H_
#define _IMX219MIPI_OFILM_OTP_H_

#include "kd_camera_typedef.h"

#define VALID_OTP_OFILM          0x01
#define O_OFILM_ID           0x07
#define SLAVE_ID_OFILM 0x20

static unsigned short golden_r_Gr;
static unsigned short golden_Gb_Gr;
static unsigned short golden_b_Gr;

static unsigned short current_r_Gr;
static unsigned short current_Gb_Gr;
static unsigned short current_b_Gr;

#define PFX_OTP "IMX219_OTP_Ofilm"
#define OTP_LOG2(format, args...)	pr_debug(PFX_OTP "[%s] " format, __FUNCTION__, ##args)


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 get_Ofilm_otp_module_id(void);
int read_otp_info_ofilm(void);
int otp_update_ofilm(void);

#endif	//_IMX219MIPI_OFILM_OTP_H_
