#include <linux/printk.h>
#include "dw9781_i2c.h"
#include "dw9781.h"

#define LOG_INF(format, args...) \
	printk(" [%s] " format, __func__, ##args)

#define RamWriteA write_reg_16bit_value_16bit
#define RamReadA read_reg_16bit_value_16bit

#define REF_ANGLE (1)
//#define REF_STROKE (130.56f)
#define REF_GYRO_RESULT (1000)	//Gyro 1 degree
#define GET_POSITION_COUNT (3)
#define AXIS_X (0)
#define AXIS_Y (1)
#define AXIS_ADC_X (2)
#define AXIS_ADC_Y (3)
#define H2D


static int REF_STROKE = 82; //Default

void SetRefStroke(int Stroke)
{
	REF_STROKE = Stroke;
	LOG_INF("REF_STROKE = %d\r\n", REF_STROKE );
}

void HAL_Delay(unsigned long ms)
{
	unsigned long us = ms*1000;
	usleep_range(us, us+2000);
}

int Cross_Motion_Test(unsigned short RADIUS, unsigned short ACCURACY,
                      unsigned short DEG_STEP, unsigned short WAIT_TIME1,
                      unsigned short WAIT_TIME2, motOISExtData *pResult)
{
	signed long REF_POSITION_RADIUS[2];
	signed short circle_cnt;
	signed short ADC_PER_MIC[2];
	signed short POSITION_ACCURACY[2];
	signed short RADIUS_GYRO_RESULT;
	signed short TARGET_RADIUS[2];
	unsigned short GYRO_GAIN[2];
	unsigned short TARGET_POSITION[2];
	unsigned short MEASURE_POSITION[2];
	signed short MAX_POS[2];
	signed short DIFF[2];
	signed short point;
	signed short x_points;
	signed short x_step;
	signed short y_points;
	signed short y_step;
	signed short get;
	//signed short X_NG, Y_NG;
	int NG_Points = 0;

	if (!pResult) {
		LOG_INF("FATAL ERROR: pResult is NULL.");
		return -1;
	}

	LOG_INF("OIS RADIUS:%d, ACCURACY:%d, DEG_STEP:%d, WAIT_TIME1:%d, WAIT_TIME2:%d, pResult:%p",
	            RADIUS, ACCURACY, DEG_STEP, WAIT_TIME1, WAIT_TIME2, pResult);

	if (DEG_STEP < 360/OIS_HEA_MAX_TEST_POINTS) {
		unsigned short min_step = 360/OIS_HEA_MAX_TEST_POINTS;
		LOG_INF("OIS override DEG_STEP from %d to %d.", DEG_STEP, min_step);
		DEG_STEP = min_step;
	}

	//Calculate points and steps.
	x_points = 360/DEG_STEP/2;
	x_step = 2*TARGET_RADIUS[AXIS_X]/x_points;
	y_points = 360/DEG_STEP/2;
	y_step = 2*TARGET_RADIUS[AXIS_Y]/y_points;

	//Read Gyro gain
	RamReadA(0x70FA, &GYRO_GAIN[AXIS_X]);
	RamReadA(0x70FB, &GYRO_GAIN[AXIS_Y]);

	LOG_INF("Gyro Gain X = 0x%04X -- Gyro Gain Y = 0x%04X\r\n", GYRO_GAIN[AXIS_X], GYRO_GAIN[AXIS_Y]);

	REF_POSITION_RADIUS[AXIS_X] = 1000*(REF_GYRO_RESULT * H2D(GYRO_GAIN[AXIS_X])) / 8192; //[code/deg]
	REF_POSITION_RADIUS[AXIS_Y] = 1000*(REF_GYRO_RESULT * H2D(GYRO_GAIN[AXIS_Y])) / 8192;

	LOG_INF("REF_POSITION_RADIUS_X = %f -- REF_POSITION_RADIUS_Y = %f\r\n", REF_POSITION_RADIUS[AXIS_X], REF_POSITION_RADIUS[AXIS_Y]);

	ADC_PER_MIC[AXIS_X] = (signed short)((signed long)REF_POSITION_RADIUS[AXIS_X] / REF_STROKE / 1000); //[code/um]
	ADC_PER_MIC[AXIS_Y] = (signed short)((signed long)REF_POSITION_RADIUS[AXIS_Y] / REF_STROKE / 1000);

	LOG_INF("ADC_PER_MIC_X = %06d[code/um] -- ADC_PER_MIC_Y = %06d[code/um]", ADC_PER_MIC[AXIS_X], ADC_PER_MIC[AXIS_Y]);

	POSITION_ACCURACY[AXIS_X] = (signed short)(ADC_PER_MIC[AXIS_X] * ACCURACY);
	POSITION_ACCURACY[AXIS_Y] = (signed short)(ADC_PER_MIC[AXIS_Y] * ACCURACY);

	LOG_INF("POSITION_ACCURACYX = %06d[code] -- POSITION_ACCURACY = %06d[code] -- ACCURACY : %.1f[um]", POSITION_ACCURACY[AXIS_X], POSITION_ACCURACY[AXIS_Y], ACCURACY);

	RADIUS_GYRO_RESULT = (signed short)((REF_GYRO_RESULT * RADIUS) / REF_STROKE); //[deg/um]

	LOG_INF("RADIUS_GYRO_RESULT = %06d\r\n", RADIUS_GYRO_RESULT);

	TARGET_RADIUS[AXIS_X] = RADIUS_GYRO_RESULT * GYRO_GAIN[AXIS_X] >> 13; //[code]
	TARGET_RADIUS[AXIS_Y] = RADIUS_GYRO_RESULT * GYRO_GAIN[AXIS_Y] >> 13;
	LOG_INF("TARGET_RADIUS_X = %06d[code] -- _Y = %06d[code]\r\n", TARGET_RADIUS[AXIS_X], TARGET_RADIUS[AXIS_Y]);

	RamWriteA(0x7025, 0x0000);
	RamWriteA(0x7026, 0x0000);

	circle_cnt = 0;

	// X Move Test
	for (point = 0; point < x_points; point++)
	{
		TARGET_POSITION[AXIS_X] = TARGET_RADIUS[AXIS_X] - x_step*point;
		TARGET_POSITION[AXIS_Y] = TARGET_RADIUS[AXIS_Y] * ((point > x_points/2) ? -1 : 1);

		pResult->hea_result.g_targetAdc[AXIS_X][circle_cnt] = TARGET_POSITION[AXIS_X];
		pResult->hea_result.g_targetAdc[AXIS_Y][circle_cnt] = TARGET_POSITION[AXIS_Y];

		RamWriteA(0x7025, TARGET_POSITION[AXIS_X]);
		RamWriteA(0x7026, TARGET_POSITION[AXIS_Y]);
		HAL_Delay(WAIT_TIME1);		//Delay for each step

		MAX_POS[AXIS_X] = -1;
		MAX_POS[AXIS_Y] = -1;

		for (get = 0; get < GET_POSITION_COUNT; get ++)
		{
			RamReadA(0x7049, &MEASURE_POSITION[AXIS_X]);	//Hall A/D code
			RamReadA(0x704A, &MEASURE_POSITION[AXIS_Y]);
			HAL_Delay(WAIT_TIME2);	//Delay for each Hall Read

			//Diff between target and hall [code]
			DIFF[AXIS_X] = abs((short)(TARGET_POSITION[AXIS_X]) - (short)(MEASURE_POSITION[AXIS_X]));
			DIFF[AXIS_Y] = abs((short)(TARGET_POSITION[AXIS_Y]) - (short)(MEASURE_POSITION[AXIS_Y]));

			if (DIFF[AXIS_X] > MAX_POS[AXIS_X])		// Record Max Diff X
			{
				MAX_POS[AXIS_X] = DIFF[AXIS_X];
				pResult->hea_result.g_targetAdc[AXIS_ADC_X][circle_cnt] = MEASURE_POSITION[AXIS_X];
				pResult->hea_result.g_diffs[AXIS_X][circle_cnt] = MAX_POS[AXIS_X];
			}

			if (DIFF[AXIS_Y] > MAX_POS[AXIS_Y])		// Record Max Diff Y
			{
				MAX_POS[AXIS_Y] = DIFF[AXIS_Y];
				pResult->hea_result.g_targetAdc[AXIS_ADC_Y][circle_cnt] = MEASURE_POSITION[AXIS_Y];
				pResult->hea_result.g_diffs[AXIS_Y][circle_cnt] = MAX_POS[AXIS_Y];
			}
		}

		LOG_INF("%6d, %6d, %6d, %6d, %6d, %6d, %6d\r\n",circle_cnt,
		            pResult->hea_result.g_targetAdc[AXIS_X][circle_cnt],
		            pResult->hea_result.g_targetAdc[AXIS_Y][circle_cnt],
		            MAX_POS[AXIS_X],
		            pResult->hea_result.g_targetAdc[AXIS_ADC_X][circle_cnt],
		            pResult->hea_result.g_targetAdc[AXIS_ADC_Y][circle_cnt],
		            MAX_POS[AXIS_Y]);

		circle_cnt++;

		// Calculate total points that over Spec
		if ((MAX_POS[AXIS_X] > POSITION_ACCURACY[AXIS_X]) ||  (MAX_POS[AXIS_Y] > POSITION_ACCURACY[AXIS_Y]))
		{
			NG_Points ++;
		}
	}

	RamWriteA(0x7025, 0x0000);
	RamWriteA(0x7026, 0x0000);

	// Y Move Test
	for (point = 0; point < y_points; point++)
	{
		TARGET_POSITION[AXIS_X] = TARGET_RADIUS[AXIS_X] * ((point > y_points/2) ? -1 : 1);
		TARGET_POSITION[AXIS_Y] = TARGET_RADIUS[AXIS_Y] - y_step*point;

		RamWriteA(0x7025, TARGET_POSITION[AXIS_X]);
		RamWriteA(0x7026, TARGET_POSITION[AXIS_Y]);
		HAL_Delay(WAIT_TIME1);		//Delay for each step

		MAX_POS[AXIS_X] = -1;
		MAX_POS[AXIS_Y] = -1;

		for (get = 0; get < GET_POSITION_COUNT; get ++)
		{
			RamReadA(0x7049, &MEASURE_POSITION[AXIS_X]);	//Hall A/D code
			RamReadA(0x704A, &MEASURE_POSITION[AXIS_Y]);
			HAL_Delay(WAIT_TIME2);	//Delay for each Hall Read

			//Diff between target and hall [code]
			DIFF[AXIS_X] = abs((short)(TARGET_POSITION[AXIS_X]) - (short)(MEASURE_POSITION[AXIS_X]));
			DIFF[AXIS_Y] = abs((short)(TARGET_POSITION[AXIS_Y]) - (short)(MEASURE_POSITION[AXIS_Y]));

			if (DIFF[AXIS_X] > MAX_POS[AXIS_X])		// Record Max Diff X
			{
				MAX_POS[AXIS_X] = DIFF[AXIS_X];
				pResult->hea_result.g_targetAdc[AXIS_ADC_X][circle_cnt] = MEASURE_POSITION[AXIS_X];
				pResult->hea_result.g_diffs[AXIS_X][circle_cnt] = MAX_POS[AXIS_X];
			}

			if (DIFF[AXIS_Y] > MAX_POS[AXIS_Y])		// Record Max Diff Y
			{
				MAX_POS[AXIS_Y] = DIFF[AXIS_Y];
				pResult->hea_result.g_targetAdc[AXIS_ADC_Y][circle_cnt] = MEASURE_POSITION[AXIS_Y];
				pResult->hea_result.g_diffs[AXIS_Y][circle_cnt] = MAX_POS[AXIS_Y];
			}
		}

		LOG_INF("%6d, %6d, %6d, %6d, %6d, %6d, %6d\r\n",circle_cnt,
		            pResult->hea_result.g_targetAdc[AXIS_X][circle_cnt],
		            pResult->hea_result.g_targetAdc[AXIS_Y][circle_cnt],
		            MAX_POS[AXIS_X],
		            pResult->hea_result.g_targetAdc[AXIS_ADC_X][circle_cnt],
		            pResult->hea_result.g_targetAdc[AXIS_ADC_Y][circle_cnt],
		            MAX_POS[AXIS_Y]);

		circle_cnt++;

		// Calculate total points that over Spec
		if ((MAX_POS[AXIS_X] > POSITION_ACCURACY[AXIS_X]) ||  (MAX_POS[AXIS_Y] > POSITION_ACCURACY[AXIS_Y]))
		{
			NG_Points ++;
		}
	}

	pResult->hea_result.ng_points = NG_Points;
	LOG_INF("NG points = %d\r\n", NG_Points );

	return NG_Points;
}
