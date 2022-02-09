#include <linux/printk.h>
#include "dw9781_i2c.h"
#include "dw9781.h"

#define LOG_INF(format, args...) \
	printk(" [%s] " format, __func__, ##args)
#define ois_printf(format, args...) printk(" [%s] " format, __func__, ##args)

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

static const int tab_sin[361] = {
	0,   17,   34,   52,   69,   87,  104,  121,  139,  156,  173,
	190,  207,  224,  241,  258,  275,  292,  309,  325,  342,
	358,  374,  390,  406,  422,  438,  453,  469,  484,  500,
	515,  529,  544,  559,  573,  587,  601,  615,  629,  642,
	656,  669,  681,  694,  707,  719,  731,  743,  754,  766,
	777,  788,  798,  809,  819,  829,  838,  848,  857,  866,
	874,  882,  891,  898,  906,  913,  920,  927,  933,  939,
	945,  951,  956,  961,  965,  970,  974,  978,  981,  984,
	987,  990,  992,  994,  996,  997,  998,  999,  999, 1000,
	999,  999,  998,  997,  996,  994,  992,  990,  987,  984,
	981,  978,  974,  970,  965,  961,  956,  951,  945,  939,
	933,  927,  920,  913,  906,  898,  891,  882,  874,  866,
	857,  848,  838,  829,  819,  809,  798,  788,  777,  766,
	754,  743,  731,  719,  707,  694,  681,  669,  656,  642,
	629,  615,  601,  587,  573,  559,  544,  529,  515,  500,
	484,  469,  453,  438,  422,  406,  390,  374,  358,  342,
	325,  309,  292,  275,  258,  241,  224,  207,  190,  173,
	156,  139,  121,  104,   87,   69,   52,   34,   17,    0,
	-17,  -34,  -52,  -69,  -87, -104, -121, -139, -156, -173,
	-190, -207, -224, -241, -258, -275, -292, -309, -325, -342,
	-358, -374, -390, -406, -422, -438, -453, -469, -484, -499,
	-515, -529, -544, -559, -573, -587, -601, -615, -629, -642,
	-656, -669, -681, -694, -707, -719, -731, -743, -754, -766,
	-777, -788, -798, -809, -819, -829, -838, -848, -857, -866,
	-874, -882, -891, -898, -906, -913, -920, -927, -933, -939,
	-945, -951, -956, -961, -965, -970, -974, -978, -981, -984,
	-987, -990, -992, -994, -996, -997, -998, -999, -999,-1000,
	-999, -999, -998, -997, -996, -994, -992, -990, -987, -984,
	-981, -978, -974, -970, -965, -961, -956, -951, -945, -939,
	-933, -927, -920, -913, -906, -898, -891, -882, -874, -866,
	-857, -848, -838, -829, -819, -809, -798, -788, -777, -766,
	-754, -743, -731, -719, -707, -694, -681, -669, -656, -642,
	-629, -615, -601, -587, -573, -559, -544, -529, -515, -500,
	-484, -469, -453, -438, -422, -406, -390, -374, -358, -342,
	-325, -309, -292, -275, -258, -241, -224, -207, -190, -173,
	-156, -139, -121, -104,  -87,  -69,  -52,  -34,  -17,    0
};

static const int tab_cos[361] = {
	1000,  999,  999,  998,  997,  996,  994,  992,  990,  987,  984,
	981,  978,  974,  970,  965,  961,  956,  951,  945,  939,
	933,  927,  920,  913,  906,  898,  891,  882,  874,  866,
	857,  848,  838,  829,  819,  809,  798,  788,  777,  766,
	754,  743,  731,  719,  707,  694,  681,  669,  656,  642,
	629,  615,  601,  587,  573,  559,  544,  529,  515,  499,
	484,  469,  453,  438,  422,  406,  390,  374,  358,  342,
	325,  309,  292,  275,  258,  241,  224,  207,  190,  173,
	156,  139,  121,  104,   87,   69,   52,   34,   17,    0,
	-17,  -34,  -52,  -69,  -87, -104, -121, -139, -156, -173,
	-190, -207, -224, -241, -258, -275, -292, -309, -325, -342,
	-358, -374, -390, -406, -422, -438, -453, -469, -484, -500,
	-515, -529, -544, -559, -573, -587, -601, -615, -629, -642,
	-656, -669, -681, -694, -707, -719, -731, -743, -754, -766,
	-777, -788, -798, -809, -819, -829, -838, -848, -857, -866,
	-874, -882, -891, -898, -906, -913, -920, -927, -933, -939,
	-945, -951, -956, -961, -965, -970, -974, -978, -981, -984,
	-987, -990, -992, -994, -996, -997, -998, -999, -999,-1000,
	-999, -999, -998, -997, -996, -994, -992, -990, -987, -984,
	-981, -978, -974, -970, -965, -961, -956, -951, -945, -939,
	-933, -927, -920, -913, -906, -898, -891, -882, -874, -866,
	-857, -848, -838, -829, -819, -809, -798, -788, -777, -766,
	-754, -743, -731, -719, -707, -694, -681, -669, -656, -642,
	-629, -615, -601, -587, -573, -559, -544, -529, -515, -499,
	-484, -469, -453, -438, -422, -406, -390, -374, -358, -342,
	-325, -309, -292, -275, -258, -241, -224, -207, -190, -173,
	-156, -139, -121, -104,  -87,  -69,  -52,  -34,  -17,    0,
	17,   34,   52,   69,   87,  104,  121,  139,  156,  173,
	190,  207,  224,  241,  258,  275,  292,  309,  325,  342,
	358,  374,  390,  406,  422,  438,  453,  469,  484,  499,
	515,  529,  544,  559,  573,  587,  601,  615,  629,  642,
	656,  669,  681,  694,  707,  719,  731,  743,  754,  766,
	777,  788,  798,  809,  819,  829,  838,  848,  857,  866,
	874,  882,  891,  898,  906,  913,  920,  927,  933,  939,
	945,  951,  956,  961,  965,  970,  974,  978,  981,  984,
	987,  990,  992,  994,  996,  997,  998,  999,  999, 1000
};

static short h2d( unsigned short u16_inpdat )
{
	short s16_temp;

	s16_temp = u16_inpdat;

	if( u16_inpdat > 32767 )
	{
		s16_temp = (unsigned short)(u16_inpdat - 65536L);
	}

	return s16_temp;
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


/**************************************************************************
Function	: Circle_Motion_Test
Date		: 2020.12.09
Version		: 0.001
History		:
***************************************************************************/
int square_motion_test (int radius, int accuracy, int deg_step, int w_time0,
                               int w_time1, int w_time2, int ref_stroke, motOISExtData *pResult)
{
	signed short REF_POSITION_RADIUS[2];
	signed short circle_adc[4][512];
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
	signed short deg;
	signed short get;
	int NG_Points = 0;
	unsigned short lens_ofst_x, lens_ofst_y;
	int hallx, hally;

	ois_printf("OIS raius:%d,accuracy:%d,deg/step:%d,wait0:%d,wait1:%d,wait2:%d, ref_stroke:%d",
					        radius, accuracy, deg_step, w_time0,
					        w_time1, w_time2, ref_stroke);

	REF_STROKE = ref_stroke;

	read_reg_16bit_value_16bit(0x7075, &lens_ofst_x);		/* lens offset */
	read_reg_16bit_value_16bit(0x7076, &lens_ofst_y);
	ois_printf("[square_motion_test] Lens ofst X = 0x%04X -- Lens ofst Y = 0x%04X\r\n", lens_ofst_x, lens_ofst_y);

	read_reg_16bit_value_16bit(0x70FA, &GYRO_GAIN[AXIS_X]);	//Read Gyro gain
	read_reg_16bit_value_16bit(0x70FB, &GYRO_GAIN[AXIS_Y]);

	ois_printf("[square_motion_test] Gyro Gain X = 0x%04X -- Gyro Gain Y = 0x%04X\r\n", GYRO_GAIN[AXIS_X], GYRO_GAIN[AXIS_Y]);

	REF_POSITION_RADIUS[AXIS_X] = REF_GYRO_RESULT * h2d(GYRO_GAIN[AXIS_X]) >> 13;  /* [code/deg] */
	REF_POSITION_RADIUS[AXIS_Y] = REF_GYRO_RESULT * h2d(GYRO_GAIN[AXIS_Y]) >> 13;

	ois_printf("[square_motion_test] REF_POSITION_RADIUS_X = %d -- REF_POSITION_RADIUS_Y = %d\r\n", REF_POSITION_RADIUS[AXIS_X], REF_POSITION_RADIUS[AXIS_Y]);

	ADC_PER_MIC[AXIS_X] = (signed short)REF_POSITION_RADIUS[AXIS_X] / REF_STROKE;  /* [code/um] */
	ADC_PER_MIC[AXIS_Y] = (signed short)REF_POSITION_RADIUS[AXIS_Y] / REF_STROKE;

	ois_printf("[square_motion_test] ADC_PER_MIC_X = %06d[code/um] -- ADC_PER_MIC_Y = %06d[code/um]\r\n", ADC_PER_MIC[AXIS_X], ADC_PER_MIC[AXIS_Y]);

	POSITION_ACCURACY[AXIS_X] = (signed short)ADC_PER_MIC[AXIS_X] * accuracy;
	POSITION_ACCURACY[AXIS_Y] = (signed short)ADC_PER_MIC[AXIS_Y] * accuracy;

	ois_printf("[square_motion_test] POSITION_ACCURACYX = %06d[code] -- POSITION_ACCURACY = %06d[code] -- ACCURACY : %d[um]\r\n", POSITION_ACCURACY[AXIS_X], POSITION_ACCURACY[AXIS_Y], accuracy);

	RADIUS_GYRO_RESULT = (signed short)REF_GYRO_RESULT * radius / REF_STROKE;  /* [deg/um] */

	ois_printf("[square_motion_test] RADIUS_GYRO_RESULT = %06d\r\n", RADIUS_GYRO_RESULT);

	TARGET_RADIUS[AXIS_X] = RADIUS_GYRO_RESULT * GYRO_GAIN[AXIS_X] >> 13;  /* code] */
	TARGET_RADIUS[AXIS_Y] = RADIUS_GYRO_RESULT * GYRO_GAIN[AXIS_Y] >> 13;

	ois_printf("[square_motion_test] TARGET_RADIUS_X = %06d[code] -- _Y = %06d[code]\r\n", TARGET_RADIUS[AXIS_X], TARGET_RADIUS[AXIS_Y]);

	deg = 0;
	TARGET_POSITION[AXIS_X] = TARGET_RADIUS[AXIS_X] * tab_cos[0] / 1000;
	TARGET_POSITION[AXIS_Y] = TARGET_RADIUS[AXIS_Y] * tab_sin[0] / 1000;

	//Enter Servo on OIS off mode, to let target pos can take effect.
	write_reg_16bit_value_16bit(0x7015, 0x0001);
	HAL_Delay(100);

	write_reg_16bit_value_16bit(0x7025, TARGET_POSITION[AXIS_X]);		/* target position */
	write_reg_16bit_value_16bit(0x7026, TARGET_POSITION[AXIS_Y]);

	HAL_Delay(w_time0);	/* move to 1st position */
	circle_cnt = 0;
	NG_Points = 0;

	ois_printf("STEP, DEGREE, TARGET_X, HALL_X, DIFF_X, TARGET_Y, HALL_Y, DIFF_Y\r\n");
	for (deg = 0; deg < 360; deg += deg_step)
	{
		if(deg <= 45)
			TARGET_POSITION[AXIS_Y] = (unsigned short)TARGET_RADIUS[AXIS_Y] * tab_sin[deg] / 1000 * 1417 / 1000;
		else if(45 < deg && deg <= 135)
			TARGET_POSITION[AXIS_X] = (unsigned short)TARGET_RADIUS[AXIS_X] * tab_cos[deg] / 1000 * 1417 / 1000;
		else if(135 < deg && deg <= 225)
			TARGET_POSITION[AXIS_Y] = (unsigned short)TARGET_RADIUS[AXIS_Y] * tab_sin[deg] / 1000 * 1417 / 1000;
		else if(225 < deg && deg <= 315)
			TARGET_POSITION[AXIS_X] = (unsigned short)TARGET_RADIUS[AXIS_X] * tab_cos[deg] / 1000 * 1417 / 1000;
		else if(315 < deg && deg <= 360)
			TARGET_POSITION[AXIS_Y] = (unsigned short)TARGET_RADIUS[AXIS_Y] * tab_sin[deg] / 1000 * 1417 / 1000;

		write_reg_16bit_value_16bit(0x7025, TARGET_POSITION[AXIS_X]);
		write_reg_16bit_value_16bit(0x7026, TARGET_POSITION[AXIS_Y]);
		HAL_Delay(w_time1); /* delay for each step */

		MAX_POS[AXIS_X] = -1;
		MAX_POS[AXIS_Y] = -1;

		DIFF[AXIS_X] = 0;
		DIFF[AXIS_Y] = 0;

		hallx = 0;
		hally = 0;

		for (get = 0; get < GET_POSITION_COUNT; get ++)
		{
			read_reg_16bit_value_16bit(0x7049, &MEASURE_POSITION[AXIS_X]);  /* Hall A/D code */
			read_reg_16bit_value_16bit(0x704A, &MEASURE_POSITION[AXIS_Y]);
			HAL_Delay(w_time2);	/* delay for each hall read */

			hallx += h2d(MEASURE_POSITION[AXIS_X]);
			hally += h2d(MEASURE_POSITION[AXIS_Y]);
		}

		DIFF[AXIS_X] = abs(h2d(TARGET_POSITION[AXIS_X]) - hallx/GET_POSITION_COUNT);
		DIFF[AXIS_Y] = abs(h2d(TARGET_POSITION[AXIS_Y]) - hally/GET_POSITION_COUNT);

		MAX_POS[AXIS_X] = DIFF[AXIS_X];
		MAX_POS[AXIS_Y] = DIFF[AXIS_Y];

		circle_adc[0][circle_cnt] = h2d(TARGET_POSITION[AXIS_X]);
		circle_adc[1][circle_cnt] = (signed short)hallx/GET_POSITION_COUNT;
		circle_adc[2][circle_cnt] = h2d(TARGET_POSITION[AXIS_Y]);
		circle_adc[3][circle_cnt] = (signed short)hally/GET_POSITION_COUNT;

		ois_printf("%6d, %6d, %6d, %6d, %6d, %6d, %6d, %6d\r\n",circle_cnt, deg, circle_adc[0][circle_cnt], circle_adc[1][circle_cnt],
			MAX_POS[AXIS_X], circle_adc[2][circle_cnt], circle_adc[3][circle_cnt], MAX_POS[AXIS_Y]);

		pResult->hea_result.g_targetAdc[0][circle_cnt] = circle_adc[0][circle_cnt];
		pResult->hea_result.g_targetAdc[1][circle_cnt] = circle_adc[1][circle_cnt];
		pResult->hea_result.g_targetAdc[2][circle_cnt] = circle_adc[2][circle_cnt];
		pResult->hea_result.g_targetAdc[3][circle_cnt] = circle_adc[3][circle_cnt];
		pResult->hea_result.g_diffs[AXIS_X][circle_cnt] = MAX_POS[AXIS_X];
		pResult->hea_result.g_diffs[AXIS_Y][circle_cnt] = MAX_POS[AXIS_Y];
		//HallAccuray.g_ADC_PER_MIC[AXIS_X] = ADC_PER_MIC[AXIS_X];
		//HallAccuray.g_ADC_PER_MIC[AXIS_Y] = ADC_PER_MIC[AXIS_Y];

		circle_cnt++;

		/* Calculate total points that over Spec */
		if ((MAX_POS[AXIS_X] > POSITION_ACCURACY[AXIS_X]) ||  (MAX_POS[AXIS_Y] > POSITION_ACCURACY[AXIS_Y]))
		{
			NG_Points ++;
		}
	}
	pResult->hea_result.ng_points = NG_Points;
	ois_printf("[square_motion_test] NG points = %d\r\n", NG_Points );

	write_reg_16bit_value_16bit(0x7025, 0);
	write_reg_16bit_value_16bit(0x7026, 0);

	return NG_Points;
}
