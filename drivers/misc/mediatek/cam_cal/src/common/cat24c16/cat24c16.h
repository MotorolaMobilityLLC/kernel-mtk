/*****************************************************************************
 *
 * Filename:
 * ---------
 *   catc24c16.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Header file of CAM_CAL driver
 *
 *
 * Author:
 * -------
 * John Wei (MTK07407)
 *
 *============================================================================*/
#ifndef __CAT24C16_H
#define __CAT24C16_H
#include <linux/i2c.h>


unsigned int cat24c16_selective_read_region(struct i2c_client *client, unsigned int addr,
	unsigned char *data, unsigned int size);


#endif /* __CAM_CAL_H */

