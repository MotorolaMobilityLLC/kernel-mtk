#if AUTO_ARBITRATION_REASONABLE_TEMP
#define XTAL_BTS_TEMP_DIFF 10000	/* 10 degree */

extern int mtktsxtal_get_xtal_temp(void);
#endif

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
extern int IMM_IsAdcInitReady(void);
