#ifndef _MOT_S5KHM2QTECH_PDAF_H
#define _MOT_S5KHM2QTECH_PDAF_H
#define PDAF_SUPPORT 2
#if PDAF_SUPPORT
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info[2] = {
	{
		.i4OffsetX = 16,
		.i4OffsetY = 16,
		.i4PitchX = 8,
		.i4PitchY = 8,
		.i4SubBlkW = 8,
		.i4SubBlkH = 2,
		.i4PairNum = 4,
		.i4BlockNumX = 496,
		.i4BlockNumY = 371,
		.iMirrorFlip = 0,
		.i4PosL = {{23, 17}, {19, 18}, {17, 21}, {21, 22} },
		.i4PosR = {{22, 17}, {18, 18}, {16, 21}, {20, 22} },
		.i4Crop = { {0, 0}, {0, 0}, {0, 0}, {0, 0},
				{0, 0}, {0, 0}, {0, 0},
				{0, 0}, {0, 0}, {0, 0} },
	},
	{
		.i4OffsetX = 0,
		.i4OffsetY = 6,
		.i4PitchX = 4,
		.i4PitchY = 4,
		.i4SubBlkW = 4,
		.i4SubBlkH = 4,
		.i4PairNum = 1,
		.i4BlockNumX = 480,
		.i4BlockNumY = 268,
		.iMirrorFlip = 0,
		.i4PosL = {{3, 8}},
		.i4PosR = {{2, 8}},
		.i4Crop = { {0, 0}, {0, 0}, {0, 0}, {0, 0},
				{0, 0}, {0, 0}, {0, 0},
				{0, 0}, {0, 0}, {0, 0} },
	}
};

static struct SENSOR_VC_INFO_STRUCT vc_info[2] = {
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
		0x00, 0x2b, 0x0FA0, 0x0BB8,
		0x00, 0x00, 0x0000, 0x0000,
		0x01, 0x30, 0x09B0, 0x02E6,
		0x00, 0x00, 0x0000, 0x0000,
	},
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
		0x00, 0x2b, 0x0780, 0x0438,
		0x00, 0x00, 0x0000, 0x0000,
		0x01, 0x30, 0x04B0, 0x010C,
		0x00, 0x00, 0x0000, 0x0000,
	}
};
#endif
#endif
