/*****************************************************************************
*
* Filename:
* ---------
*   da9214.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   da9214 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _da9214_SW_H_
#define _da9214_SW_H_

extern void da9214_dump_register(void);
extern unsigned int da9214_read_interface(unsigned char RegNum, unsigned char *val,
					  unsigned char MASK, unsigned char SHIFT);
extern unsigned int da9214_config_interface(unsigned char RegNum, unsigned char val,
					    unsigned char MASK, unsigned char SHIFT);
extern int da9214_vosel_buck_a(unsigned long val);
extern int da9214_vosel_buck_b(unsigned long val);
extern int is_da9214_exist(void);
extern int is_da9214_sw_ready(void);

#endif				/* _da9214_SW_H_ */
