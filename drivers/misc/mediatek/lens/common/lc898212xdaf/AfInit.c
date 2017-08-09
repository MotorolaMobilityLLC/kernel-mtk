//********************************************************************************
//
//		LC89821x Initialize Module
//
//	    Program Name	: AfInit.h
//		Design			: Rex.Tang
//		History			: First edition						2013.07.20 Rex.Tang
//
//		Description		: AF Initialize Functions and Definations
//********************************************************************************
//**************************
//	Include Header File		
//**************************
#include	"AfInit.h"
#include	"AfData.h"

/*-----------------------------
	Definations
------------------------------*/
#define		REG_ADDR_START	  	0x80		// REG Start address

/*====================================================================
	Interface functions (import)
=====================================================================*/
extern	void RamWriteA(unsigned short addr, unsigned short data);

extern	void RamReadA(unsigned short addr, unsigned short *data);

extern	void RegWriteA(unsigned short addr, unsigned char data);

extern	void RegReadA(unsigned short addr, unsigned char *data);

extern	void WaitTime(unsigned short msec);

//********************************************************************************
// Function Name 	: AfInit
// Retun Value		: NON
// Argment Value	: Hall Bias, Hall Offset
// Explanation		: Af Initial Function
// History			: First edition 					2013.07.20 Rex.Tang
//********************************************************************************   
void 	AfInit( unsigned char hall_off, unsigned char hall_bias )
{
    #define		DataLen		sizeof(Init_Table) / sizeof(IniData)
    unsigned short i;
    unsigned short pos;

    for(i = 0; i < DataLen; i++)
    {
        if(Init_Table[i].addr == WAIT)
        {
        	WaitTime(Init_Table[i].data);
            continue;
		}
			
        if(Init_Table[i].addr >= REG_ADDR_START)
        {
     		RegWriteA(Init_Table[i].addr, (unsigned char)(Init_Table[i].data & 0x00ff));
     	} else {
         	RamWriteA(Init_Table[i].addr, (unsigned short)Init_Table[i].data);
		} 
	}

	RegWriteA( 0x28, hall_off);		// Hall Offset
	RegWriteA( 0x29, hall_bias);	// Hall Bias

	RamReadA( 0x3C, &pos); 
	RamWriteA( 0x04, pos); // Direct move target position
	RamWriteA( 0x18, pos); // Step move start position	

	//WaitTime(5);
	//RegWriteA( 0x87, 0x85 );		// Servo ON					
}

//********************************************************************************
// Function Name 	: ServoON
// Retun Value		: NON
// Argment Value	: None
// Explanation		: Servo On function
// History			: First edition 					2013.07.20 Rex.Tang
//********************************************************************************
void ServoOn(void)
{
	RegWriteA( 0x85, 0x80);		// Clear PID Ram Data
	WaitTime(1);
   	RegWriteA( 0x87, 0x85 );		// Servo ON
}  

