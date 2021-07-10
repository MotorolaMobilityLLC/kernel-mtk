#include <linux/printk.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include "dw9781_i2c.h"
#include "dw9781.h"

#define LOG_INF(format, args...) \
	printk(" [%s] " format, __func__, ##args)

/* fw downlaod */
#define DW9781C_CHIP_ID_ADDRESS 0x7000
#define DW9781C_CHIP_ID 0x9781
#define FW_VER_CURR_ADDR 0x7001
#define FW_TYPE_ADDR 0x700D
#define SET_FW 0x8001
#define EOK 0
#define ERROR_SECOND_ID -1
#define ERROR_FW_VERIFY -2
#define MTP_START_ADDRESS 0x8000

int dw9781_check = 0;
static unsigned short checksum_value = 0;
typedef struct
{
	unsigned int driverIc;
	unsigned int size;
	unsigned short *fwContentPtr;
	unsigned short version;
}FirmwareContex;

extern struct i2c_client *g_pstAF_I2Cclient;
FirmwareContex g_firmwareContext;
unsigned short g_downloadByForce;

const struct firmware *dw9781cfw;

static motOISGOffsetResult dw9781GyroOffsetResult;

static void os_mdelay(unsigned long ms)
{
	unsigned long us = ms*1000;
	usleep_range(us, us+2000);
}

void ois_reset(void)
{
	LOG_INF("[dw9781c_ois_reset] ois reset\r\n");
	write_reg_16bit_value_16bit(0xD002, 0x0001); /* printfc reset */
	os_mdelay(4);
	write_reg_16bit_value_16bit(0xD001, 0x0001); /* Active mode (DSP ON) */
	os_mdelay(25); /* ST gyro - over wait 25ms, default Servo On */
	write_reg_16bit_value_16bit(0xEBF1, 0x56FA); /* User protection release */
}

int gyro_offset_calibrtion(void)
{
	unsigned short Addr, status;
	unsigned short xOffset, yOffset;
	unsigned int OverCnt;
	int msg;
	int i;

	LOG_INF("[dw9781c_gyro_offset_calibrtion] gyro_offset_calibrtion starting\r\n");

	for(i = 1; i < 3; i++) {
		/* Gyro offset */
		write_reg_16bit_value_16bit(0x7011, 0x4015);
		write_reg_16bit_value_16bit(0x7010, 0x8000);
		os_mdelay(100);
		msg = 0;
		OverCnt = 0;
		while (1) {
			Addr = 0x7036; status = 0;
			read_reg_16bit_value_16bit(Addr, &status);
			if (status & 0x8000) {
				break; /* it done! */
			} else {
				os_mdelay(10); /* 100msec waiting */
			}

			if (OverCnt++ > GYRO_OFST_CAL_OVERCNT) { /* when it take over 10sec .. break */
				msg = GYRO_CAL_TIME_OVER;
				break;
			}
		}

		if (msg == 0) {
			if (status & 0x8000) {
				if ( status == 0x8000 ) {
					msg = GYRO_OFFSET_CAL_OK;
					LOG_INF("[dw9781c_gyro_offset_calibrtion] GYRO_OFFSET_CAL_OK\r\n");
					break;
				} else {
					if (status & 0x1) {
						msg += X_GYRO_OFFSET_SPEC_OVER_NG;
						LOG_INF("[dw9781c_gyro_offset_calibrtion] X_GYRO_OFFSET_SPEC_OVER_NG\r\n");
					}
					if (status & 0x2) {
						msg += Y_GYRO_OFFSET_SPEC_OVER_NG;
						LOG_INF("[dw9781c_gyro_offset_calibrtion] Y_GYRO_OFFSET_SPEC_OVER_NG\r\n");
					}
					if (status & 0x10) {
						msg += X_GYRO_RAW_DATA_CHECK;
						LOG_INF("[dw9781c_gyro_offset_calibrtion] X_GYRO_RAW_DATA_CHECK\r\n");
					}
					if (status & 0x20) {
						msg += Y_GYRO_RAW_DATA_CHECK;
						LOG_INF("[dw9781c_gyro_offset_calibrtion] Y_GYRO_RAW_DATA_CHECK\r\n");
					}
					if (i >= 2) {
						LOG_INF("[dw9781c_gyro_offset_calibrtion] gyro offset calibration-retry NG (%d times)\r\n", i);
					} else {
						LOG_INF("[dw9781c_gyro_offset_calibrtion] gyro offset calibration-retry NG (%d times)\r\n", i);
						msg = 0;
					}
				}
			}
		}
	}

	read_reg_16bit_value_16bit(0x70F8, &xOffset);
	read_reg_16bit_value_16bit(0x70F9, &yOffset);

	LOG_INF("[dw9781c_gyro_offset_calibrtion] msg : %d\r\n", msg);
	LOG_INF("[dw9781c_gyro_offset_calibrtion] x_gyro_offset: 0x%04X, y_gyro_offset : 0x%04X\r\n", xOffset, yOffset);
	LOG_INF("[dw9781c_gyro_offset_calibrtion] gyro_offset_calibrtion finished...Status = 0x%04X\r\n", status);

	if (msg == EOK) {
		dw9781GyroOffsetResult.is_success = 0;
		dw9781GyroOffsetResult.x_offset = xOffset;
		dw9781GyroOffsetResult.y_offset = yOffset;
		calibration_save();
	} else {
		memset(&dw9781GyroOffsetResult, 0x0, sizeof(dw9781GyroOffsetResult));
		dw9781GyroOffsetResult.is_success = status;
	}

	return msg;
}

motOISGOffsetResult *dw9781_get_gyro_offset_result(void)
{
	return &dw9781GyroOffsetResult;
}

void calibration_save(void)
{
	LOG_INF("[dw9781c_calibration_save] calibration save starting\r\n");
	write_reg_16bit_value_16bit(0x7011, 0x00AA); /* select mode */
	os_mdelay(10);
	write_reg_16bit_value_16bit(0x7010, 0x8000); /* start mode */
	os_mdelay(100);
	ois_reset();
	LOG_INF("[dw9781c_calibration_save] calibration save finish\r\n");
}

void check_calibration_data(void)
{
	unsigned short tmp, tmp1;
	unsigned short XAmpGain = 0;
	unsigned short YAmpGain = 0;
	unsigned short XADCMax = 0;
	unsigned short XADCMin = 0;
	unsigned short YADCMax = 0;
	unsigned short YADCMin = 0;
	unsigned char X_HALL_BIAS = 0;
	unsigned char Y_HALL_BIAS = 0;
	int X_ADC_Range = 0;
	int Y_ADC_Range = 0;
	/* SAC_PARA] */
	LOG_INF("[flash_calibration_data] SAC_PARA\r\n");
	read_reg_16bit_value_16bit(0x701D, &tmp); LOG_INF("SAC_CFG: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x701E, &tmp); LOG_INF("SAC_TVIB: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x701F, &tmp); LOG_INF("SAC_CC: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7012, &tmp); LOG_INF("IMU_SENSOR_SEL: %04X\r\n", tmp);
	/* [CALIB_PARA] */
	LOG_INF("[flash_calibration_data] HALL DATA\r\n");
	read_reg_16bit_value_16bit(0x7003, &tmp); LOG_INF("MODULE_ID: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7004, &tmp); LOG_INF("ACTUATOR_ID: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x707A, &tmp); LOG_INF("X_HALL_POL: %04X\r\n", tmp & 0x0001);
	read_reg_16bit_value_16bit(0x707A, &tmp); LOG_INF("Y_HALL_POL: %04X\r\n", tmp>>4 & 0x0001);
	read_reg_16bit_value_16bit(0x707B, &tmp); LOG_INF("X_DAC_POL: %04X\r\n", tmp & 0x0001);
	read_reg_16bit_value_16bit(0x707B, &tmp); LOG_INF("Y_DAC_POL: %04X\r\n", tmp>>4 & 0x0001);
	read_reg_16bit_value_16bit(0x707B, &tmp); LOG_INF("Z_DAC_POL: %04X\r\n", tmp>>8 & 0x0001);
	/* Hall Bias Range Check */
	read_reg_16bit_value_16bit(0x7070, &tmp);
	X_HALL_BIAS = tmp & 0x00FF;
	Y_HALL_BIAS = tmp >> 8;
	LOG_INF("Hall Bias X = 0x%02X\r\n",X_HALL_BIAS);
	LOG_INF("Hall Bias Y = 0x%02X\r\n",Y_HALL_BIAS);
	/* Hall ADC Range Check */
	read_reg_16bit_value_16bit(0x708C, &XADCMax); // X axis ADC MAX
	read_reg_16bit_value_16bit(0x708D, &XADCMin); // X axis ADC MIN
	read_reg_16bit_value_16bit(0x709E, &YADCMax); // Y axis ADC MAX
	read_reg_16bit_value_16bit(0x709F, &YADCMin); // Y axis ADC MIN
	X_ADC_Range = abs(XADCMax) + abs(XADCMin);
	Y_ADC_Range = abs(YADCMax) + abs(YADCMin);
	LOG_INF("X ADC Max = 0x%04X (%d)\r\n",XADCMax, XADCMax);
	LOG_INF("X ADC Min = 0x%04X (%d)\r\n",XADCMin, XADCMin);
	LOG_INF("Y ADC Max = 0x%04X (%d)\r\n",YADCMax, YADCMax);
	LOG_INF("Y ADC Max = 0x%04X (%d)\r\n",YADCMin, YADCMin);
	LOG_INF("X_ADC_Range = 0x%04X (%d)\r\n",X_ADC_Range, X_ADC_Range);
	LOG_INF("Y_ADC_Range = 0x%04X (%d)\r\n",Y_ADC_Range, Y_ADC_Range);
	/* Hall Amp gain Check */
	read_reg_16bit_value_16bit(0x7071, &XAmpGain);
	read_reg_16bit_value_16bit(0x7072, &YAmpGain);
	LOG_INF("Amp gain X = 0x%04X\r\n",XAmpGain);
	LOG_INF("Amp gain Y = 0x%04X\r\n",YAmpGain);
	/* [PID_PARA] */
	LOG_INF("[flash_calibration_data] PID_PARA\r\n");
	read_reg_16bit_value_16bit(0x7260, &tmp); LOG_INF("X_SERVO_P_GAIN: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7261, &tmp); LOG_INF("X_SERVO_I_GAIN: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7262, &tmp); LOG_INF("X_SERVO_D_GAIN: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7263, &tmp); LOG_INF("X_SERVO_D_LPF:  %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7142, &tmp); read_reg_16bit_value_16bit(0x7143, &tmp1); LOG_INF("X_SERVO_BIQAUD_0 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7144, &tmp); read_reg_16bit_value_16bit(0x7145, &tmp1); LOG_INF("X_SERVO_BIQAUD_1 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7146, &tmp); read_reg_16bit_value_16bit(0x7147, &tmp1); LOG_INF("X_SERVO_BIQAUD_2 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7148, &tmp); read_reg_16bit_value_16bit(0x7149, &tmp1); LOG_INF("X_SERVO_BIQAUD_3 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x714A, &tmp); read_reg_16bit_value_16bit(0x714B, &tmp1); LOG_INF("X_SERVO_BIQAUD_4 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x714C, &tmp); read_reg_16bit_value_16bit(0x714D, &tmp1); LOG_INF("X_SERVO_BIQAUD_5 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x714E, &tmp); read_reg_16bit_value_16bit(0x714F, &tmp1); LOG_INF("X_SERVO_BIQAUD_6 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7150, &tmp); read_reg_16bit_value_16bit(0x7151, &tmp1); LOG_INF("X_SERVO_BIQAUD_7 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7152, &tmp); read_reg_16bit_value_16bit(0x7153, &tmp1); LOG_INF("X_SERVO_BIQAUD_8 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7154, &tmp); read_reg_16bit_value_16bit(0x7155, &tmp1); LOG_INF("X_SERVO_BIQAUD_9 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7156, &tmp); read_reg_16bit_value_16bit(0x7157, &tmp1); LOG_INF("X_SERVO_BIQAUD_10: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7158, &tmp); read_reg_16bit_value_16bit(0x7159, &tmp1); LOG_INF("X_SERVO_BIQAUD_11: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x715A, &tmp); read_reg_16bit_value_16bit(0x715B, &tmp1); LOG_INF("X_SERVO_BIQAUD_12: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x715C, &tmp); read_reg_16bit_value_16bit(0x715D, &tmp1); LOG_INF("X_SERVO_BIQAUD_13: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x715E, &tmp); read_reg_16bit_value_16bit(0x715F, &tmp1); LOG_INF("X_SERVO_BIQAUD_14: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7160, &tmp); read_reg_16bit_value_16bit(0x7161, &tmp1); LOG_INF("X_SERVO_BIQAUD_15: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7162, &tmp); read_reg_16bit_value_16bit(0x7163, &tmp1); LOG_INF("X_SERVO_BIQAUD_16: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7164, &tmp); read_reg_16bit_value_16bit(0x7165, &tmp1); LOG_INF("X_SERVO_BIQAUD_17: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7166, &tmp); read_reg_16bit_value_16bit(0x7167, &tmp1); LOG_INF("X_SERVO_BIQAUD_18: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7168, &tmp); read_reg_16bit_value_16bit(0x7169, &tmp1); LOG_INF("X_SERVO_BIQAUD_19: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7264, &tmp); LOG_INF("Y_SERVO_P_GAIN: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7265, &tmp); LOG_INF("Y_SERVO_I_GAIN: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7266, &tmp); LOG_INF("Y_SERVO_D_GAIN: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7267, &tmp); LOG_INF("Y_SERVO_D_LPF : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x719C, &tmp); read_reg_16bit_value_16bit(0x719D, &tmp1); LOG_INF("Y_SERVO_BIQAUD_0 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x719E, &tmp); read_reg_16bit_value_16bit(0x719F, &tmp1); LOG_INF("Y_SERVO_BIQAUD_1 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71A0, &tmp); read_reg_16bit_value_16bit(0x71A1, &tmp1); LOG_INF("Y_SERVO_BIQAUD_2 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71A2, &tmp); read_reg_16bit_value_16bit(0x71A3, &tmp1); LOG_INF("Y_SERVO_BIQAUD_3 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71A4, &tmp); read_reg_16bit_value_16bit(0x71A5, &tmp1); LOG_INF("Y_SERVO_BIQAUD_4 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71A6, &tmp); read_reg_16bit_value_16bit(0x71A7, &tmp1); LOG_INF("Y_SERVO_BIQAUD_5 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71A8, &tmp); read_reg_16bit_value_16bit(0x71A9, &tmp1); LOG_INF("Y_SERVO_BIQAUD_6 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71AA, &tmp); read_reg_16bit_value_16bit(0x71AB, &tmp1); LOG_INF("Y_SERVO_BIQAUD_7 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71AC, &tmp); read_reg_16bit_value_16bit(0x71AD, &tmp1); LOG_INF("Y_SERVO_BIQAUD_8 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71AE, &tmp); read_reg_16bit_value_16bit(0x71AF, &tmp1); LOG_INF("Y_SERVO_BIQAUD_9 : %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71B0, &tmp); read_reg_16bit_value_16bit(0x71B1, &tmp1); LOG_INF("Y_SERVO_BIQAUD_10: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71B2, &tmp); read_reg_16bit_value_16bit(0x71B3, &tmp1); LOG_INF("Y_SERVO_BIQAUD_11: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71B4, &tmp); read_reg_16bit_value_16bit(0x71B5, &tmp1); LOG_INF("Y_SERVO_BIQAUD_12: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71B6, &tmp); read_reg_16bit_value_16bit(0x71B7, &tmp1); LOG_INF("Y_SERVO_BIQAUD_13: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71B8, &tmp); read_reg_16bit_value_16bit(0x71B9, &tmp1); LOG_INF("Y_SERVO_BIQAUD_14: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71BA, &tmp); read_reg_16bit_value_16bit(0x71BB, &tmp1); LOG_INF("Y_SERVO_BIQAUD_15: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71BC, &tmp); read_reg_16bit_value_16bit(0x71BD, &tmp1); LOG_INF("Y_SERVO_BIQAUD_16: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71BE, &tmp); read_reg_16bit_value_16bit(0x71BF, &tmp1); LOG_INF("Y_SERVO_BIQAUD_17: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71C0, &tmp); read_reg_16bit_value_16bit(0x71C1, &tmp1); LOG_INF("Y_SERVO_BIQAUD_18: %08X\r\n",tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x71C2, &tmp); read_reg_16bit_value_16bit(0x71C3, &tmp1); LOG_INF("Y_SERVO_BIQAUD_19: %08X\r\n",tmp<<16 | tmp1);
	//[GYRO_PARA]
	LOG_INF("[flash_calibration_data] GYRO_PARA\r\n");
	read_reg_16bit_value_16bit(0x7084, &tmp); LOG_INF("ACT_GYRO_MAT_0 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7085, &tmp); LOG_INF("ACT_GYRO_MAT_1 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7086, &tmp); LOG_INF("ACT_GYRO_MAT_2 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7087, &tmp); LOG_INF("ACT_GYRO_MAT_3 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x71DA, &tmp); LOG_INF("X_GYRO_GAIN_POL: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x71DB, &tmp); LOG_INF("Y_GYRO_GAIN_POL: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x7138, &tmp); read_reg_16bit_value_16bit(0x7139, &tmp1); LOG_INF("X_GYRO_LPF_0: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x713A, &tmp); read_reg_16bit_value_16bit(0x713B, &tmp1); LOG_INF("X_GYRO_LPF_1: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x713C, &tmp); read_reg_16bit_value_16bit(0x713D, &tmp1); LOG_INF("X_GYRO_LPF_2: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x713E, &tmp); read_reg_16bit_value_16bit(0x713F, &tmp1); LOG_INF("X_GYRO_LPF_3: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7140, &tmp); read_reg_16bit_value_16bit(0x7141, &tmp1); LOG_INF("X_GYRO_LPF_4: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7110, &tmp); read_reg_16bit_value_16bit(0x7111, &tmp1); LOG_INF("X_GYRO_BIQAUD_0 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7112, &tmp); read_reg_16bit_value_16bit(0x7113, &tmp1); LOG_INF("X_GYRO_BIQAUD_1 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7114, &tmp); read_reg_16bit_value_16bit(0x7115, &tmp1); LOG_INF("X_GYRO_BIQAUD_2 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7116, &tmp); read_reg_16bit_value_16bit(0x7117, &tmp1); LOG_INF("X_GYRO_BIQAUD_3 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7118, &tmp); read_reg_16bit_value_16bit(0x7119, &tmp1); LOG_INF("X_GYRO_BIQAUD_4 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x711A, &tmp); read_reg_16bit_value_16bit(0x711B, &tmp1); LOG_INF("X_GYRO_BIQAUD_5 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x711C, &tmp); read_reg_16bit_value_16bit(0x711D, &tmp1); LOG_INF("X_GYRO_BIQAUD_6 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x711E, &tmp); read_reg_16bit_value_16bit(0x711F, &tmp1); LOG_INF("X_GYRO_BIQAUD_7 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7120, &tmp); read_reg_16bit_value_16bit(0x7121, &tmp1); LOG_INF("X_GYRO_BIQAUD_8 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7122, &tmp); read_reg_16bit_value_16bit(0x7123, &tmp1); LOG_INF("X_GYRO_BIQAUD_9 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7124, &tmp); read_reg_16bit_value_16bit(0x7125, &tmp1); LOG_INF("X_GYRO_BIQAUD_10: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7126, &tmp); read_reg_16bit_value_16bit(0x7127, &tmp1); LOG_INF("X_GYRO_BIQAUD_11: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7128, &tmp); read_reg_16bit_value_16bit(0x7129, &tmp1); LOG_INF("X_GYRO_BIQAUD_12: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x712A, &tmp); read_reg_16bit_value_16bit(0x712B, &tmp1); LOG_INF("X_GYRO_BIQAUD_13: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x712C, &tmp); read_reg_16bit_value_16bit(0x712D, &tmp1); LOG_INF("X_GYRO_BIQAUD_14: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x712E, &tmp); read_reg_16bit_value_16bit(0x712F, &tmp1); LOG_INF("X_GYRO_BIQAUD_15: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7130, &tmp); read_reg_16bit_value_16bit(0x7131, &tmp1); LOG_INF("X_GYRO_BIQAUD_16: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7132, &tmp); read_reg_16bit_value_16bit(0x7133, &tmp1); LOG_INF("X_GYRO_BIQAUD_17: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7134, &tmp); read_reg_16bit_value_16bit(0x7135, &tmp1); LOG_INF("X_GYRO_BIQAUD_18: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7136, &tmp); read_reg_16bit_value_16bit(0x7137, &tmp1); LOG_INF("X_GYRO_BIQAUD_19: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7192, &tmp); read_reg_16bit_value_16bit(0x7193, &tmp1); LOG_INF("Y_GYRO_LPF_0: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7194, &tmp); read_reg_16bit_value_16bit(0x7195, &tmp1); LOG_INF("Y_GYRO_LPF_1: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7196, &tmp); read_reg_16bit_value_16bit(0x7197, &tmp1); LOG_INF("Y_GYRO_LPF_2: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7198, &tmp); read_reg_16bit_value_16bit(0x7199, &tmp1); LOG_INF("Y_GYRO_LPF_3: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x719A, &tmp); read_reg_16bit_value_16bit(0x719B, &tmp1); LOG_INF("Y_GYRO_LPF_4: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x716A, &tmp); read_reg_16bit_value_16bit(0x716B, &tmp1); LOG_INF("Y_GYRO_BIQAUD_0 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x716C, &tmp); read_reg_16bit_value_16bit(0x716D, &tmp1); LOG_INF("Y_GYRO_BIQAUD_1 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x716E, &tmp); read_reg_16bit_value_16bit(0x716F, &tmp1); LOG_INF("Y_GYRO_BIQAUD_2 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7170, &tmp); read_reg_16bit_value_16bit(0x7171, &tmp1); LOG_INF("Y_GYRO_BIQAUD_3 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7172, &tmp); read_reg_16bit_value_16bit(0x7173, &tmp1); LOG_INF("Y_GYRO_BIQAUD_4 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7174, &tmp); read_reg_16bit_value_16bit(0x7175, &tmp1); LOG_INF("Y_GYRO_BIQAUD_5 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7176, &tmp); read_reg_16bit_value_16bit(0x7177, &tmp1); LOG_INF("Y_GYRO_BIQAUD_6 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7178, &tmp); read_reg_16bit_value_16bit(0x7179, &tmp1); LOG_INF("Y_GYRO_BIQAUD_7 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x717A, &tmp); read_reg_16bit_value_16bit(0x717B, &tmp1); LOG_INF("Y_GYRO_BIQAUD_8 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x717C, &tmp); read_reg_16bit_value_16bit(0x717D, &tmp1); LOG_INF("Y_GYRO_BIQAUD_9 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x717E, &tmp); read_reg_16bit_value_16bit(0x717F, &tmp1); LOG_INF("Y_GYRO_BIQAUD_10: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7180, &tmp); read_reg_16bit_value_16bit(0x7181, &tmp1); LOG_INF("Y_GYRO_BIQAUD_11: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7182, &tmp); read_reg_16bit_value_16bit(0x7183, &tmp1); LOG_INF("Y_GYRO_BIQAUD_12: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7184, &tmp); read_reg_16bit_value_16bit(0x7185, &tmp1); LOG_INF("Y_GYRO_BIQAUD_13: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7186, &tmp); read_reg_16bit_value_16bit(0x7187, &tmp1); LOG_INF("Y_GYRO_BIQAUD_14: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7188, &tmp); read_reg_16bit_value_16bit(0x7189, &tmp1); LOG_INF("Y_GYRO_BIQAUD_15: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x718A, &tmp); read_reg_16bit_value_16bit(0x718B, &tmp1); LOG_INF("Y_GYRO_BIQAUD_16: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x718C, &tmp); read_reg_16bit_value_16bit(0x718D, &tmp1); LOG_INF("Y_GYRO_BIQAUD_17: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x718E, &tmp); read_reg_16bit_value_16bit(0x718F, &tmp1); LOG_INF("Y_GYRO_BIQAUD_18: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7190, &tmp); read_reg_16bit_value_16bit(0x7191, &tmp1); LOG_INF("Y_GYRO_BIQAUD_19: %08X\r\n", tmp<<16 | tmp1);
	//[ACC_PARA]
	LOG_INF("[flash_calibration_data] ACC_PARA\r\n");
	read_reg_16bit_value_16bit(0x70E8, &tmp); LOG_INF("ACT_ACC_MAT_0 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x70E9, &tmp); LOG_INF("ACT_ACC_MAT_1 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x70EA, &tmp); LOG_INF("ACT_ACC_MAT_2 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x70EB, &tmp); LOG_INF("ACT_ACC_MAT_3 : %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x70E6, &tmp); LOG_INF("X_ACC_GAIN_POL: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x70E7, &tmp); LOG_INF("Y_ACC_GAIN_POL: %04X\r\n", tmp);
	read_reg_16bit_value_16bit(0x72B0, &tmp); read_reg_16bit_value_16bit(0x72B1, &tmp1); LOG_INF("X_ACC_BIQUAD_0 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72B2, &tmp); read_reg_16bit_value_16bit(0x72B3, &tmp1); LOG_INF("X_ACC_BIQUAD_1 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72B4, &tmp); read_reg_16bit_value_16bit(0x72B5, &tmp1); LOG_INF("X_ACC_BIQUAD_2 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72B6, &tmp); read_reg_16bit_value_16bit(0x72B7, &tmp1); LOG_INF("X_ACC_BIQUAD_3 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72B8, &tmp); read_reg_16bit_value_16bit(0x72B9, &tmp1); LOG_INF("X_ACC_BIQUAD_4 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72BA, &tmp); read_reg_16bit_value_16bit(0x72BB, &tmp1); LOG_INF("X_ACC_BIQUAD_5 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72BC, &tmp); read_reg_16bit_value_16bit(0x72BD, &tmp1); LOG_INF("X_ACC_BIQUAD_6 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72BE, &tmp); read_reg_16bit_value_16bit(0x72BF, &tmp1); LOG_INF("X_ACC_BIQUAD_7 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72C0, &tmp); read_reg_16bit_value_16bit(0x72C1, &tmp1); LOG_INF("X_ACC_BIQUAD_8 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72C2, &tmp); read_reg_16bit_value_16bit(0x72C3, &tmp1); LOG_INF("X_ACC_BIQUAD_9 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72C4, &tmp); read_reg_16bit_value_16bit(0x72C5, &tmp1); LOG_INF("X_ACC_BIQUAD_10: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72C6, &tmp); read_reg_16bit_value_16bit(0x72C7, &tmp1); LOG_INF("X_ACC_BIQUAD_11: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72C8, &tmp); read_reg_16bit_value_16bit(0x72C9, &tmp1); LOG_INF("X_ACC_BIQUAD_12: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72CA, &tmp); read_reg_16bit_value_16bit(0x72CB, &tmp1); LOG_INF("X_ACC_BIQUAD_13: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72CC, &tmp); read_reg_16bit_value_16bit(0x72CD, &tmp1); LOG_INF("X_ACC_BIQUAD_14: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72CE, &tmp); read_reg_16bit_value_16bit(0x72CF, &tmp1); LOG_INF("X_ACC_BIQUAD_15: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72D0, &tmp); read_reg_16bit_value_16bit(0x72D1, &tmp1); LOG_INF("X_ACC_BIQUAD_16: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72D2, &tmp); read_reg_16bit_value_16bit(0x72D3, &tmp1); LOG_INF("X_ACC_BIQUAD_17: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72D4, &tmp); read_reg_16bit_value_16bit(0x72D5, &tmp1); LOG_INF("X_ACC_BIQUAD_18: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72D6, &tmp); read_reg_16bit_value_16bit(0x72D7, &tmp1); LOG_INF("X_ACC_BIQUAD_19: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72D8, &tmp); read_reg_16bit_value_16bit(0x72D9, &tmp1); LOG_INF("X_ACC_BIQUAD_20: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72DA, &tmp); read_reg_16bit_value_16bit(0x72DB, &tmp1); LOG_INF("X_ACC_BIQUAD_21: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72DC, &tmp); read_reg_16bit_value_16bit(0x72DD, &tmp1); LOG_INF("X_ACC_BIQUAD_22: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72DE, &tmp); read_reg_16bit_value_16bit(0x72DF, &tmp1); LOG_INF("X_ACC_BIQUAD_23: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72E0, &tmp); read_reg_16bit_value_16bit(0x72E1, &tmp1); LOG_INF("X_ACC_BIQUAD_24: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72E2, &tmp); read_reg_16bit_value_16bit(0x72E3, &tmp1); LOG_INF("Y_ACC_BIQUAD_0 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72E4, &tmp); read_reg_16bit_value_16bit(0x72E5, &tmp1); LOG_INF("Y_ACC_BIQUAD_1 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72E6, &tmp); read_reg_16bit_value_16bit(0x72E7, &tmp1); LOG_INF("Y_ACC_BIQUAD_2 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72E8, &tmp); read_reg_16bit_value_16bit(0x72E9, &tmp1); LOG_INF("Y_ACC_BIQUAD_3 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72EA, &tmp); read_reg_16bit_value_16bit(0x72EB, &tmp1); LOG_INF("Y_ACC_BIQUAD_4 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72EC, &tmp); read_reg_16bit_value_16bit(0x72ED, &tmp1); LOG_INF("Y_ACC_BIQUAD_5 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72EE, &tmp); read_reg_16bit_value_16bit(0x72EF, &tmp1); LOG_INF("Y_ACC_BIQUAD_6 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72F0, &tmp); read_reg_16bit_value_16bit(0x72F1, &tmp1); LOG_INF("Y_ACC_BIQUAD_7 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72F2, &tmp); read_reg_16bit_value_16bit(0x72F3, &tmp1); LOG_INF("Y_ACC_BIQUAD_8 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72F4, &tmp); read_reg_16bit_value_16bit(0x72F5, &tmp1); LOG_INF("Y_ACC_BIQUAD_9 : %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72F6, &tmp); read_reg_16bit_value_16bit(0x72F7, &tmp1); LOG_INF("Y_ACC_BIQUAD_10: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72F8, &tmp); read_reg_16bit_value_16bit(0x72F9, &tmp1); LOG_INF("Y_ACC_BIQUAD_11: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72FA, &tmp); read_reg_16bit_value_16bit(0x72FB, &tmp1); LOG_INF("Y_ACC_BIQUAD_12: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72FC, &tmp); read_reg_16bit_value_16bit(0x72FD, &tmp1); LOG_INF("Y_ACC_BIQUAD_13: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x72FE, &tmp); read_reg_16bit_value_16bit(0x72FF, &tmp1); LOG_INF("Y_ACC_BIQUAD_14: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7300, &tmp); read_reg_16bit_value_16bit(0x7301, &tmp1); LOG_INF("Y_ACC_BIQUAD_15: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7302, &tmp); read_reg_16bit_value_16bit(0x7303, &tmp1); LOG_INF("Y_ACC_BIQUAD_16: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7304, &tmp); read_reg_16bit_value_16bit(0x7305, &tmp1); LOG_INF("Y_ACC_BIQUAD_17: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7306, &tmp); read_reg_16bit_value_16bit(0x7307, &tmp1); LOG_INF("Y_ACC_BIQUAD_18: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7308, &tmp); read_reg_16bit_value_16bit(0x7309, &tmp1); LOG_INF("Y_ACC_BIQUAD_19: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x730A, &tmp); read_reg_16bit_value_16bit(0x730B, &tmp1); LOG_INF("Y_ACC_BIQUAD_20: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x730C, &tmp); read_reg_16bit_value_16bit(0x730D, &tmp1); LOG_INF("Y_ACC_BIQUAD_21: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x730E, &tmp); read_reg_16bit_value_16bit(0x730F, &tmp1); LOG_INF("Y_ACC_BIQUAD_22: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7310, &tmp); read_reg_16bit_value_16bit(0x7311, &tmp1); LOG_INF("Y_ACC_BIQUAD_23: %08X\r\n", tmp<<16 | tmp1);
	read_reg_16bit_value_16bit(0x7312, &tmp); read_reg_16bit_value_16bit(0x7313, &tmp1); LOG_INF("Y_ACC_BIQUAD_24: %08X\r\n", tmp<<16 | tmp1);
}

int GenerateFirmwareContexts(void)
{
	int i4RetValue;
	g_firmwareContext.size = 10240; /* size: word */
	g_firmwareContext.driverIc = 0x9781;

	i4RetValue = request_firmware(&dw9781cfw, "mot_dw9781.prog", NULL);
	if(i4RetValue<0) {
		LOG_INF("get dw9781c firmware failed!\n");
	} else {
		int i = 0;
		g_firmwareContext.fwContentPtr = kmalloc(20480,GFP_KERNEL);
		if(!g_firmwareContext.fwContentPtr )
		{
			printk("kmalloc fwContentPtr failed in GenerateFirmwareContexts\n");
			return -1;
		}
		while(i<10240) {
			(g_firmwareContext.fwContentPtr)[i] = (((dw9781cfw->data)[2*i] << 8) + ((dw9781cfw->data)[(2*i+1)]));
			i++;
		}
		LOG_INF("get dw9781c firmware sucess,and size = %d\n",dw9781cfw->size);

		g_firmwareContext.version = *(g_firmwareContext.fwContentPtr+10235);
		checksum_value = *(g_firmwareContext.fwContentPtr+10234);
	}
	g_downloadByForce = 0;
	return i4RetValue;
}

void ois_ready_check(void)
{
	unsigned short fw_flag1, fw_flag2, fw_flag3;
	write_reg_16bit_value_16bit(0xD001, 0x0001); /* chip enable */
	os_mdelay(4);
	write_reg_16bit_value_16bit(0xD001, 0x0000); /* dsp off mode */
	write_reg_16bit_value_16bit(0xFAFA, 0x98AC); /* All protection(1) */
	write_reg_16bit_value_16bit(0xF053, 0x70BD); /* All protection(2) */
	read_reg_16bit_value_16bit(0x8000, &fw_flag1); /* check checksum flag1 */
	read_reg_16bit_value_16bit(0x8001, &fw_flag2); /* check checksum flag2 */
	read_reg_16bit_value_16bit(0xA7F9, &fw_flag3); /* check checksum flag3 */
	printk("[dw9781c_ois_ready_check] checksum flag1 : 0x%04x, checksum flag2 : 0x%04x, checksum flag3 : 0x%04x\n", fw_flag1, fw_flag2, fw_flag3);
	if((fw_flag1 == 0xF073) && (fw_flag2 == 0x2045) && (fw_flag3 == 0xCC33))
	{
		printk("[dw9781c_ois_ready_check] checksum flag is ok\n");
		ois_reset(); /* ois reset */
	} else {
		write_reg_16bit_value_16bit(0xD002, 0x0001); /* dw9781c reset */
		os_mdelay(4);
		printk("[dw9781c_ois_ready_check] previous firmware download fail\n");
	}
}

void all_protection_release(void)
{
	printk("[dw9781c_all_protection_release] execution\n");
	/* release all protection */
	write_reg_16bit_value_16bit(0xFAFA, 0x98AC);
	mdelay(1);
	write_reg_16bit_value_16bit(0xF053, 0x70BD);
	mdelay(1);
}

void erase_mtp(void)
{
	printk("[dw9781c_erase_mtp] start erasing firmware flash\n");
	/* 12c level adjust */
	write_reg_16bit_value_16bit(0xd005, 0x0001);
	write_reg_16bit_value_16bit(0xdd03, 0x0002);
	write_reg_16bit_value_16bit(0xdd04, 0x0002);

	/* 4k Sector_0 */
	write_reg_16bit_value_16bit(0xde03, 0x0000);
	/* 4k Sector Erase */
	write_reg_16bit_value_16bit(0xde04, 0x0002);
	os_mdelay(10);
	/* 4k Sector_1 */
	write_reg_16bit_value_16bit(0xde03, 0x0008);
	/* 4k Sector Erase */
	write_reg_16bit_value_16bit(0xde04, 0x0002);
	os_mdelay(10);
	/* 4k Sector_2 */
	write_reg_16bit_value_16bit(0xde03, 0x0010);
	/* 4k Sector Erase */
	write_reg_16bit_value_16bit(0xde04, 0x0002);
	os_mdelay(10);
	/* 4k Sector_3 */
	write_reg_16bit_value_16bit(0xde03, 0x0018);
	/* 4k Sector Erase */
	write_reg_16bit_value_16bit(0xde04, 0x0002);
	os_mdelay(10);
	/* 4k Sector_4 */
	write_reg_16bit_value_16bit(0xde03, 0x0020);
	/* 4k Sector Erase */
	write_reg_16bit_value_16bit(0xde04, 0x0002);
	os_mdelay(10);
	printk("[dw9781c_erase_mtp] complete erasing firmware flash\n");
}

int download_fw(void)
{
	unsigned char ret = ERROR_FW_VERIFY;
	unsigned short i;
	unsigned short addr;
	//unsigned short buf[g_firmwareContext.size];
	unsigned short* buf_temp = NULL;
	buf_temp = kmalloc(20480,GFP_KERNEL);
	if(buf_temp == NULL)
	{
		printk("kmalloc buf_temp failed in download_fw\n");
		return -1;
	}
	memset(buf_temp, 0, g_firmwareContext.size * sizeof(unsigned short));
	/* step 1: MTP Erase and DSP Disable for firmware 0x8000 write */
	write_reg_16bit_value_16bit(0xd001, 0x0000);
	/* step 2: MTP setup */
	all_protection_release();
	erase_mtp();
	printk("[dw9781c_download_fw] start firmware download\n");
	/* step 3: firmware sequential write to flash */

	for (i = 0; i < g_firmwareContext.size; i += DATPKT_SIZE)
	{
		addr = MTP_START_ADDRESS + i;
		i2c_block_write_reg( addr, g_firmwareContext.fwContentPtr + i, DATPKT_SIZE);
	}
	printk("[dw9781c_download_fw] write firmware to flash\n");
	/* step 4: firmware sequential read from flash */
	for (i = 0; i <  g_firmwareContext.size; i += DATPKT_SIZE)
	{
		addr = MTP_START_ADDRESS + i;
		i2c_block_read_reg(addr, buf_temp + i, DATPKT_SIZE);
	}
	printk("[dw9781c_download_fw] read firmware from flash\n");
	/* step 5: firmware verify */
	for (i = 0; i < g_firmwareContext.size; i++)
	{
		buf_temp[i] = ((buf_temp[i] << 8) & 0xff00) | ((buf_temp[i] >> 8) & 0x00ff);
		if (g_firmwareContext.fwContentPtr[i] != buf_temp[i])
		{
			printk("[dw9781c_download_fw] firmware verify NG!!! ADDR:%04X -- firmware:%04x -- READ:%04x\n", MTP_START_ADDRESS+i, g_firmwareContext.fwContentPtr[i], buf_temp[i]);
			kfree(buf_temp);
			buf_temp = NULL;
			return ERROR_FW_VERIFY;
		}else
			ret = EOK;
	}
	kfree(buf_temp);
	buf_temp = NULL;

	printk("[dw9781c_download_fw] firmware verification pass\n");
	printk("[dw9781c_download_fw] firmware download success\n");

	return ret;
}

unsigned short fw_checksum_verify(void)
{
	unsigned short data;

	/* FW checksum command */
	write_reg_16bit_value_16bit(0x7011, 0x2000);
	/* command  start */
	write_reg_16bit_value_16bit(0x7010, 0x8000);
	os_mdelay(10);
	/* calc the checksum to write the 0x7005 */
	read_reg_16bit_value_16bit(0x7005, &data);
	printk("F/W Checksum calculated value : 0x%04X\n", data);

	return data;
}

int erase_mtp_rewritefw(void)
{
	printk("dw9781c erase for rewritefw starting..");

	/* 512 byte page */
	write_reg_16bit_value_16bit(0xde03, 0x0027);
	/* page erase */
	write_reg_16bit_value_16bit(0xde04, 0x0008);
	os_mdelay(10);

	printk("dw9781c checksum flag erase : 0xCC33\n");
	return 0;
}

void switch_chip_register(void)
{
	unsigned short data;
	read_reg_16bit_value_16bit(0x7012, &data);
	if(data == 0x4) {
		write_reg_16bit_value_16bit(0x7012,0x0007);
		calibration_save();
	}
}

int dw9781c_download_ois_fw(void)
{
	unsigned char ret;
	unsigned short fwchecksum = 0;
	unsigned short first_chip_id = 0;
	unsigned short second_chip_id = 0;
	unsigned short fw_version_current = 0;
	unsigned short fw_version_latest = 0;
	unsigned short fw_type = 0;

	ois_ready_check();
	ret = GenerateFirmwareContexts();
	if(ret < 0)
		return ret;

	read_reg_16bit_value_16bit(DW9781C_CHIP_ID_ADDRESS, &first_chip_id);
	printk("[dw9781c] first_chip_id : 0x%x\n", first_chip_id);
	if (first_chip_id != DW9781C_CHIP_ID) { /* first_chip_id verification failed */
		all_protection_release();
		read_reg_16bit_value_16bit(0xd060, &second_chip_id); /* second_chip_id: 0x0020 */
		if ( second_chip_id == 0x0020 ) /* Check the second_chip_id*/
		{
			printk("[dw9781c] start flash download:: size:%d, version:0x%x\n",
				g_firmwareContext.size, g_firmwareContext.version);
			ret = download_fw(); /* Need to forced update OIS firmware again. */
			printk("[dw9781c] flash download::vendor_dw9781c\n");
			if (ret != 0x0) {
				erase_mtp_rewritefw();
				write_reg_16bit_value_16bit(0xd000, 0x0000); /* Shut download mode */
				printk("[dw9781c] firmware download error, ret = 0x%x\n", ret);
				printk("[dw9781c] change dw9781c state to shutdown mode\n");
				ret = ERROR_FW_VERIFY;
			} else {
				printk("[dw9781c] firmware download success\n");
			}
		} else {
			erase_mtp_rewritefw();
			write_reg_16bit_value_16bit(0xd000, 0x0000); /* Shut download mode */
			printk("[dw9781c] second_chip_id check fail,second_chip_id = 0x%x\n",second_chip_id);
			printk("[dw9781c] change dw9781c state to shutdown mode\n");
			ret = ERROR_SECOND_ID;
		}
	} else {
		fwchecksum = fw_checksum_verify();
		if(fwchecksum != checksum_value)
		{
			g_downloadByForce = 1;
			printk("[dw9781c] firmware checksum error 0x%04X, 0x%04X\n", checksum_value, fwchecksum);
		}

		read_reg_16bit_value_16bit(FW_TYPE_ADDR, &fw_type);
		if(fw_type != SET_FW)
		{
			g_downloadByForce = 1;
			printk("[dw9781c] Update to firmware for set\n");
		}
		read_reg_16bit_value_16bit(FW_VER_CURR_ADDR, &fw_version_current);
		fw_version_latest = g_firmwareContext.version; /*Enter the firmware version in VIVO*/
		printk("[dw9781c] fw_version_current = 0x%x, fw_version_latest = 0x%x\n",
			fw_version_current, fw_version_latest);
		/* download firmware, check if need update, download firmware to flash */
		if (g_downloadByForce || ((fw_version_current & 0xFF) != (fw_version_latest & 0xFF))) {
			printk("[dw9781c] start flash download:: size:%d, version:0x%x g_downloadByForce %d\n",
				g_firmwareContext.size, g_firmwareContext.version, g_downloadByForce);
			ret = download_fw();
			printk("[dw9781c] flash download::vendor_dw9781c\n");
			if (ret != EOK) {
				erase_mtp_rewritefw();
				write_reg_16bit_value_16bit(0xd000, 0x0000); /* Shut download mode */
				printk("[dw9781c] firmware download error, ret = 0x%x\n", ret);
				printk("[dw9781c] change dw9781c state to shutdown mode\n");
				ret = ERROR_FW_VERIFY;
			} else {
				printk("[dw9781c] firmware download success\n");
			}
		} else {
			printk("[dw9781c] ois firmware version is updated, skip download\n");
		}
	}
	kfree(g_firmwareContext.fwContentPtr);
	g_firmwareContext.fwContentPtr = NULL;
	dw9781_check = 1;

	switch_chip_register();

	return ret;
}

int ois_checksum(void)
{
	unsigned short fwchecksum = 0;
	fwchecksum = fw_checksum_verify();
	if(fwchecksum != checksum_value)
	{
		printk("[dw9781c] firmware checksum error 0x%04X, 0x%04X\n", checksum_value, fwchecksum);
		return -1;
	}
	return 0;
}
