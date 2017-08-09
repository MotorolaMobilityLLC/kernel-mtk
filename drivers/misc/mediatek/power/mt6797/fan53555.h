/*****************************************************************************
*
* Filename:
* ---------
*   fan53555.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   fan53555 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _fan53555_SW_H_
#define _fan53555_SW_H_

#define fan53555_REG_NUM 6

extern void fan53555_dump_register(void);
extern unsigned int fan53555_read_interface(unsigned char RegNum, unsigned char *val,
					    unsigned char MASK, unsigned char SHIFT);
extern unsigned int fan53555_config_interface(unsigned char RegNum, unsigned char val,
					      unsigned char MASK, unsigned char SHIFT);

#endif				/* _fan53555_SW_H_ */
